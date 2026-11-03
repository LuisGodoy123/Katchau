/*
 * olhar.c
 * ------------------------------------------------------------------
 * A câmera livre do jogador. Veja olhar.h.
 *
 * A câmera se move como uma mola amortecida: acelera, chega e para sem
 * balançar. O movimento é sempre contínuo, mesmo no giro de 180 graus, que é
 * só mais rápido. É isso que o deixa fluido em qualquer taxa de quadros.
 * ------------------------------------------------------------------
 */

#include <math.h>
#include "olhar.h"

#define ZONA_MORTA          0.15f   /* o analógico nunca para exatamente no centro */
#define TEMPO_NORMAL        0.10f   /* segundos: quanto menor, mais rápido a câmera chega no alvo */
#define TEMPO_PARA_TRAS     0.055f  /* olhando para trás (ou voltando de lá), ela vai bem mais rápido */
#define GIRO_DE_OLHAR_TRAS  120.0f  /* falta mais que isso para o alvo: é um giro grande */
#define MAIOR_PASSO         0.05f   /* segundos: um quadro lento não faz a mola disparar */
#define BAIXO_PARA_TRAS     0.80f   /* analógico para baixo além disso, e pouco para o lado... */
#define LADO_PARA_TRAS      0.55f   /* ...olha direto para trás */
#define GRAUS_POR_RADIANO   57.29578f

/*
 * olharParaAFrente
 * Recebe: nada.
 * Devolve: a câmera na posição normal, atrás do carro.
 */
Olhar olharParaAFrente(void) {
    Olhar olhar = { 0.0f, 0.0f, 0.0f, 0.0f };
    return olhar;
}

/*
 * molaAmortecida
 * Leva um valor até o alvo: a solução exata da mola com amortecimento
 * crítico, que não passa do alvo e não depende do tamanho do quadro.
 * Recebe: quanto falta (alvo - valor), o valor e a velocidade dele, o tempo
 *         típico de chegada e o tempo do quadro.
 * Devolve: nada (altera o valor e a velocidade).
 */
static void molaAmortecida(float falta, float *valor, float *velocidade, float tempoDeChegada, float tempoQuadro) {
    float w = 2.0f / tempoDeChegada;
    float k = (*velocidade - w * falta) * tempoQuadro;
    float queda = expf(-w * tempoQuadro);
    *valor += falta - (falta - k) * queda;
    *velocidade = (*velocidade - w * k) * queda;
}

/*
 * atualizarOlhar
 * Um quadro da câmera. O quadrado da inclinação deixa os toques pequenos bem
 * finos. Para trás e de volta, o caminho é o mais curto e o giro é mais rápido.
 * Recebe: o olhar, o analógico (y > 0 é para baixo) e o tempo do quadro.
 * Devolve: nada.
 */
void atualizarOlhar(Olhar *olhar, float eixoX, float eixoY, float tempoQuadro) {
    float tamanho = sqrtf(eixoX * eixoX + eixoY * eixoY);
    float alvoDoGiro = 0.0f, alvoDaElevacao = 0.0f;

    if (eixoY > BAIXO_PARA_TRAS && fabsf(eixoX) < LADO_PARA_TRAS) {
        alvoDoGiro = 180.0f;
    } else if (tamanho >= ZONA_MORTA) {
        float inclinado = (fminf(tamanho, 1.0f) - ZONA_MORTA) / (1.0f - ZONA_MORTA);
        alvoDoGiro = atan2f(eixoX, -eixoY) * GRAUS_POR_RADIANO * inclinado * inclinado;
        if (eixoY < 0.0f) {
            alvoDaElevacao = ELEVACAO_MAXIMA * inclinado * (-eixoY / tamanho);
        }
    }

    float falta = alvoDoGiro - olhar->giro;
    while (falta > 180.0f) {
        falta -= 360.0f;
    }
    while (falta < -180.0f) {
        falta += 360.0f;
    }

    float passo = fminf(tempoQuadro, MAIOR_PASSO);
    float tempoDeChegada = (fabsf(falta) > GIRO_DE_OLHAR_TRAS) ? TEMPO_PARA_TRAS : TEMPO_NORMAL;
    molaAmortecida(falta, &olhar->giro, &olhar->velocidadeDoGiro, tempoDeChegada, passo);
    molaAmortecida(alvoDaElevacao - olhar->elevacao, &olhar->elevacao, &olhar->velocidadeDaElevacao, TEMPO_NORMAL, passo);

    if (olhar->giro > 180.0f) {
        olhar->giro -= 360.0f;
    } else if (olhar->giro < -180.0f) {
        olhar->giro += 360.0f;
    }
}
