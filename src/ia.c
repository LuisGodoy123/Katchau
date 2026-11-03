/*
 * ia.c
 * ------------------------------------------------------------------
 * Regras dos pilotos do computador. São só três perguntas por quadro:
 *
 *   1) Vem curva fechada aí? -> olha os próximos segmentos da lista,
 *      calcula a velocidade segura de cada um e se já está na hora de
 *      frear para chegar nela; acelera ou freia conforme a resposta.
 *      No meio da curva, perto do limite, só alivia: acelerar ali faz
 *      o carro sair de frente (veja carro.c).
 *   2) Tem carro logo à frente, na mesma linha? -> muda de linha para
 *      ultrapassar.
 *   3) Estou fora da minha linha? -> vira o volante na direção dela.
 * ------------------------------------------------------------------
 */

#include <stddef.h>
#include "ia.h"

#define SEGMENTOS_OLHADOS         80       /* quantos segmentos à frente a IA enxerga (16000 unidades: */
                                           /* mais que a frenagem de 313 km/h até parar) */
#define FOLGA_NA_FRENAGEM         0.8f     /* a IA conta só 80% da distância: começa a frear um pouco antes */
#define MARGEM_DE_FREIO           1.05f    /* só freia se passar 5% da velocidade desejada */
#define DISTANCIA_PARA_ULTRAPASSAR 900.0f  /* carro da frente mais perto que isso: tentar passar */
#define LARGURA_OCUPADA           0.4f     /* dois carros mais próximos que isso estão na mesma linha */
#define DESVIO_ULTRAPASSAGEM      0.5f     /* quanto se afasta para o lado do carro da frente */
#define LIMITE_LINHA              0.75f    /* a IA nunca escolhe andar mais perto da borda que isso */
#define EMBALO_PARA_O_NITRO       0.70f    /* só usa o nitro acima de 70% da máxima */
#define TANQUE_PARA_O_NITRO       0.60f    /* e só começa com pelo menos isso no tanque */
#define TOLERANCIA_LINHA          0.05f    /* perto assim da linha, não mexe no volante */

/*
 * velocidadeDesejada
 * Olha os próximos SEGMENTOS_OLHADOS nós da pista e, para cada um,
 * calcula a maior velocidade de agora que ainda dá para frear até a
 * velocidade segura dele, na distância que falta até lá. A menor de
 * todas é a que a IA quer: assim ela freia na hora certa antes de cada
 * curva, nem cedo nem tarde, seja qual for o freio do carro.
 * Recebe: o carro.
 * Devolve: a velocidade que a IA quer ter agora.
 */
static float velocidadeDesejada(Carro *carro) {
    float desejada = carro->velocidadeMaxima;
    Segmento *olhando = carro->posicao.segmento;
    float distancia = -carro->posicao.distanciaNoSegmento;   /* até o começo de cada segmento olhado */

    for (int i = 0; i < SEGMENTOS_OLHADOS; i++) {
        float segura = velocidadeSeguraNaCurva(carro, olhando->curvatura);
        float agora = velocidadeParaFrearAte(segura, distancia * FOLGA_NA_FRENAGEM);
        if (agora < desejada) {
            desejada = agora;
        }
        distancia += COMPRIMENTO_SEGMENTO;
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
    Comandos comandos = { 0, 0, 0, 0, 0.0f, 0 };

    /* 1) Velocidade: acelera abaixo da desejada (e, dentro da curva,
     *    abaixo do limite de quem acelera), freia bem acima dela */
    float desejada = velocidadeDesejada(carro);
    float podeAcelerar = velocidadeSeguraAcelerando(carro, carro->posicao.segmento->curvatura);
    if (carro->velocidade < desejada && carro->velocidade < podeAcelerar) {
        comandos.acelerar = 1;
    } else if (desejada < carro->velocidadeMaxima && carro->velocidade > desejada * MARGEM_DE_FREIO) {
        comandos.frear = 1;   /* numa reta livre não freia: acima da máxima é vácuo ou nitro */
    }

    /* Nitro: só com a pista livre à frente, depois de embalar, e com um bom tanque */
    if (desejada >= carro->velocidadeMaxima && carro->velocidade > carro->velocidadeMaxima * EMBALO_PARA_O_NITRO
        && (carro->nitroLigado || carro->nitro >= TANQUE_PARA_O_NITRO)) {
        comandos.nitro = 1;
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
