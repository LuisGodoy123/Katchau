/*
 * ia.c
 * ------------------------------------------------------------------
 * Regras dos pilotos do computador. São só três perguntas por quadro:
 *
 *   1) Vem curva fechada aí? -> olha os próximos segmentos da lista e
 *      calcula a velocidade segura; acelera ou freia para chegar nela.
 *   2) Tem carro logo à frente, na mesma linha? -> muda de linha para
 *      ultrapassar.
 *   3) Estou fora da minha linha? -> vira o volante na direção dela.
 * ------------------------------------------------------------------
 */

#include <stddef.h>
#include "ia.h"

#define SEGMENTOS_OLHADOS         30       /* quantos segmentos à frente a IA enxerga (6000 unidades) */
#define MARGEM_DE_FREIO           1.05f    /* só freia se passar 5% da velocidade desejada */
#define DISTANCIA_PARA_ULTRAPASSAR 900.0f  /* carro da frente mais perto que isso: tentar passar */
#define LARGURA_OCUPADA           0.4f     /* dois carros mais próximos que isso estão na mesma linha */
#define DESVIO_ULTRAPASSAGEM      0.5f     /* quanto se afasta para o lado do carro da frente */
#define LIMITE_LINHA              0.75f    /* a IA nunca escolhe andar mais perto da borda que isso */
#define TOLERANCIA_LINHA          0.05f    /* perto assim da linha, não mexe no volante */

/*
 * velocidadeDesejada
 * Olha os próximos SEGMENTOS_OLHADOS nós da pista e devolve a menor
 * velocidade segura entre eles. Assim a IA começa a frear antes de
 * entrar na curva, e não no meio dela.
 * Recebe: o carro.
 * Devolve: a velocidade que a IA quer ter agora.
 */
static float velocidadeDesejada(Carro *carro) {
    float desejada = carro->velocidadeMaxima;
    Segmento *olhando = carro->posicao.segmento;

    for (int i = 0; i < SEGMENTOS_OLHADOS; i++) {
        float segura = velocidadeSeguraNaCurva(carro, olhando->curvatura);
        if (segura < desejada) {
            desejada = segura;
        }
        olhando = proximoSegmento(olhando);
    }
    return desejada;
}

/*
 * limitarLinha
 * Impede a IA de escolher uma linha muito perto da grama.
 * Recebe: a linha desejada.
 * Devolve: a linha dentro de [-LIMITE_LINHA, LIMITE_LINHA].
 */
static float limitarLinha(float linha) {
    if (linha < -LIMITE_LINHA) {
        return -LIMITE_LINHA;
    }
    if (linha > LIMITE_LINHA) {
        return LIMITE_LINHA;
    }
    return linha;
}

/*
 * escolherLinha
 * Se o carro da frente está perto e na mesma linha, escolhe o lado com
 * mais espaço para passar. Senão, mantém a linha que já tinha.
 * Recebe: o carro e o carro logo à frente na classificação (ou NULL).
 * Devolve: nada (altera carro->linhaAlvo).
 */
static void escolherLinha(Carro *carro, Carro *carroDaFrente) {
    if (carroDaFrente == NULL) {
        return;   /* está em 1º: ninguém para ultrapassar */
    }

    float distancia = carroDaFrente->distanciaPercorrida - carro->distanciaPercorrida;
    float diferencaLateral = carroDaFrente->deslocamentoLateral - carro->deslocamentoLateral;
    if (diferencaLateral < 0) {
        diferencaLateral = -diferencaLateral;
    }

    int estaPerto = distancia > 0 && distancia < DISTANCIA_PARA_ULTRAPASSAR;
    int mesmaLinha = diferencaLateral < LARGURA_OCUPADA;

    if (estaPerto && mesmaLinha) {
        /* Passa pelo lado oposto ao que o carro da frente está */
        if (carroDaFrente->deslocamentoLateral > 0) {
            carro->linhaAlvo = limitarLinha(carroDaFrente->deslocamentoLateral - DESVIO_ULTRAPASSAGEM);
        } else {
            carro->linhaAlvo = limitarLinha(carroDaFrente->deslocamentoLateral + DESVIO_ULTRAPASSAGEM);
        }
    }
}

/*
 * atualizarIA
 * Decide os comandos do piloto do computador neste quadro.
 * Recebe: o carro da IA e o carro logo à frente dele (NULL se for o 1º).
 * Devolve: os comandos (acelerar, frear, esquerda, direita).
 */
Comandos atualizarIA(Carro *carro, Carro *carroDaFrente) {
    Comandos comandos = { 0, 0, 0, 0 };

    /* 1) Velocidade: acelera abaixo da desejada, freia bem acima dela */
    float desejada = velocidadeDesejada(carro);
    if (carro->velocidade < desejada) {
        comandos.acelerar = 1;
    } else if (carro->velocidade > desejada * MARGEM_DE_FREIO) {
        comandos.frear = 1;
    }

    /* 2) Linha: tenta ultrapassar quem está na frente */
    escolherLinha(carro, carroDaFrente);

    /* 3) Volante: vira na direção da linha escolhida */
    float erro = carro->linhaAlvo - carro->deslocamentoLateral;
    if (erro > TOLERANCIA_LINHA) {
        comandos.direita = 1;
    } else if (erro < -TOLERANCIA_LINHA) {
        comandos.esquerda = 1;
    }

    return comandos;
}
