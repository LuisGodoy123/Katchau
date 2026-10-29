/*
 * carro.c
 * ------------------------------------------------------------------
 * Movimento do carro. A física é de fliperama, não realista:
 *
 *   - acelerar soma velocidade; frear e soltar o pedal tiram;
 *   - na grama a velocidade cai até um limite baixo;
 *   - virar muda a posição lateral, mais rápido quanto mais rápido
 *     o carro anda (parado, o carro não vira);
 *   - na curva o carro é empurrado para fora, e o empurrão cresce
 *     com o quadrado da velocidade: em alta, ou se freia antes da
 *     curva, ou se vai parar na grama.
 * ------------------------------------------------------------------
 */

#include <string.h>
#include "carro.h"

#define FORCA_FREIO            12000.0f  /* quanto a velocidade cai por segundo freando */
#define ATRITO                  2400.0f  /* quanto cai por segundo sem acelerar */
#define ATRITO_GRAMA            6000.0f  /* quanto cai por segundo na grama */
#define VELOCIDADE_MAXIMA_GRAMA 3000.0f  /* na grama o carro não passa disso */
#define VELOCIDADE_DE_VIRADA       2.0f  /* larguras de pista por segundo, na velocidade máxima */
#define FORCA_CENTRIFUGA           4.8f  /* intensidade do empurrão para fora nas curvas */
#define LIMITE_LATERAL             2.5f  /* até onde o carro pode ir pela grama */
#define KMH_POR_UNIDADE            0.025f /* 12000 unidades/s viram 300 km/h no velocímetro */

/*
 * criarCarro
 * Monta um carro parado na largada.
 * Recebe: nome, segmento de largada, posição lateral no grid e a ficha técnica.
 * Devolve: o carro pronto (a struct inteira, por valor).
 */
Carro criarCarro(const char *nome, int numero, Segmento *largada, float deslocamentoLateral,
                 float velocidadeMaxima, float aceleracao, float aderencia) {
    Carro carro;

    strncpy(carro.nome, nome, TAMANHO_NOME - 1);
    carro.nome[TAMANHO_NOME - 1] = '\0';
    carro.numero = numero;

    carro.posicao.segmento = largada;
    carro.posicao.distanciaNoSegmento = 0.0f;
    carro.distanciaPercorrida = 0.0f;
    carro.deslocamentoLateral = deslocamentoLateral;

    carro.velocidade = 0.0f;
    carro.voltas = 0;
    for (int i = 0; i < TOTAL_VOLTAS; i++) {
        carro.temposDeVolta[i] = 0.0f;
    }
    carro.inicioDaVolta = 0.0f;
    carro.tempoDeChegada = 0.0f;
    Comandos nenhumComando = { 0, 0, 0, 0 };
    carro.ultimosComandos = nenhumComando;
    carro.linhaAlvo = deslocamentoLateral;

    carro.velocidadeMaxima = velocidadeMaxima;
    carro.aceleracao = aceleracao;
    carro.aderencia = aderencia;
    return carro;
}

/*
 * limitar
 * Prende um valor entre um mínimo e um máximo.
 * Recebe: o valor e os limites.
 * Devolve: o valor dentro dos limites.
 */
static float limitar(float valor, float minimo, float maximo) {
    if (valor < minimo) {
        return minimo;
    }
    if (valor > maximo) {
        return maximo;
    }
    return valor;
}

/*
 * estaForaDaPista
 * Recebe: o carro.
 * Devolve: 1 se o carro está na grama, 0 se está no asfalto.
 */
int estaForaDaPista(Carro *carro) {
    return carro->deslocamentoLateral < -LIMITE_PISTA ||
           carro->deslocamentoLateral > LIMITE_PISTA;
}

/*
 * atualizarVelocidade
 * Aplica acelerador, freio, atrito e grama à velocidade.
 * Recebe: o carro, os comandos e o tempo do quadro em segundos.
 * Devolve: nada (altera carro->velocidade).
 */
static void atualizarVelocidade(Carro *carro, Comandos comandos, float tempoQuadro) {
    if (comandos.acelerar) {
        carro->velocidade += carro->aceleracao * tempoQuadro;
    } else if (comandos.frear) {
        carro->velocidade -= FORCA_FREIO * tempoQuadro;
    } else {
        carro->velocidade -= ATRITO * tempoQuadro;
    }

    if (estaForaDaPista(carro) && carro->velocidade > VELOCIDADE_MAXIMA_GRAMA) {
        carro->velocidade -= ATRITO_GRAMA * tempoQuadro;
    }

    carro->velocidade = limitar(carro->velocidade, 0.0f, carro->velocidadeMaxima);
}

/*
 * atualizarLateral
 * Vira o carro e aplica o empurrão da curva.
 * Recebe: o carro, os comandos e o tempo do quadro em segundos.
 * Devolve: nada (altera carro->deslocamentoLateral).
 */
static void atualizarLateral(Carro *carro, Comandos comandos, float tempoQuadro) {
    /* 0 parado, 1 na velocidade máxima */
    float proporcaoVelocidade = carro->velocidade / carro->velocidadeMaxima;

    /* Volante */
    float virada = VELOCIDADE_DE_VIRADA * proporcaoVelocidade * tempoQuadro;
    if (comandos.esquerda) {
        carro->deslocamentoLateral -= virada;
    }
    if (comandos.direita) {
        carro->deslocamentoLateral += virada;
    }

    /* Força centrífuga: curva para a direita (curvatura > 0) empurra
     * para a esquerda, e vice-versa. Mais aderência, menos empurrão. */
    float curvatura = carro->posicao.segmento->curvatura;
    float empurrao = curvatura * FORCA_CENTRIFUGA * proporcaoVelocidade * proporcaoVelocidade
                     * (1.0f - carro->aderencia) * tempoQuadro;
    carro->deslocamentoLateral -= empurrao;

    carro->deslocamentoLateral = limitar(carro->deslocamentoLateral, -LIMITE_LATERAL, LIMITE_LATERAL);
}

/*
 * atualizarCarro
 * Avança o carro um quadro: velocidade, lateral e depois anda pela
 * lista da pista com moverNaPista().
 * Recebe: o carro, os comandos e o tempo do quadro em segundos.
 * Devolve: quantas voltas o carro completou neste quadro (0 ou 1).
 */
int atualizarCarro(Carro *carro, Comandos comandos, float tempoQuadro) {
    carro->ultimosComandos = comandos;
    atualizarVelocidade(carro, comandos, tempoQuadro);
    atualizarLateral(carro, comandos, tempoQuadro);

    float distancia = carro->velocidade * tempoQuadro;
    carro->distanciaPercorrida += distancia;

    int voltasGanhas = moverNaPista(&carro->posicao, distancia);
    carro->voltas += voltasGanhas;
    return voltasGanhas;
}

/*
 * velocidadeEmKmh
 * Converte a velocidade interna para o velocímetro.
 * Recebe: o carro.
 * Devolve: a velocidade em km/h, arredondada.
 */
int velocidadeEmKmh(Carro *carro) {
    return converterParaKmh(carro->velocidade);
}

/*
 * converterParaKmh
 * Converte qualquer velocidade interna (por exemplo, a velocidade
 * máxima de uma ficha na garagem) para km/h.
 * Recebe: a velocidade em unidades por segundo.
 * Devolve: a velocidade em km/h, arredondada.
 */
int converterParaKmh(float velocidade) {
    return (int) (velocidade * KMH_POR_UNIDADE + 0.5f);
}

/*
 * velocidadeSeguraNaCurva
 * A maior velocidade em que o volante ainda vence o empurrão da curva.
 * Vem de igualar as duas fórmulas de atualizarLateral():
 *
 *     volante  = VELOCIDADE_DE_VIRADA * p
 *     empurrão = curvatura * FORCA_CENTRIFUGA * p * p * (1 - aderencia)
 *
 *   volante = empurrão  ->  p = VELOCIDADE_DE_VIRADA / (curvatura * FORCA_CENTRIFUGA * (1 - aderencia))
 *
 * onde p é a proporção da velocidade máxima (0 a 1).
 * A IA usa isso para saber quanto frear antes de uma curva.
 * Recebe: o carro e a curvatura da curva.
 * Devolve: a velocidade segura, em unidades por segundo.
 */
float velocidadeSeguraNaCurva(Carro *carro, float curvatura) {
    if (curvatura < 0) {
        curvatura = -curvatura;   /* para a esquerda ou para a direita, tanto faz */
    }
    if (curvatura == 0) {
        return carro->velocidadeMaxima;   /* reta: sem limite */
    }

    float proporcao = VELOCIDADE_DE_VIRADA / (curvatura * FORCA_CENTRIFUGA * (1.0f - carro->aderencia));
    if (proporcao > 1.0f) {
        proporcao = 1.0f;   /* curva suave: dá para fazer no máximo */
    }
    return proporcao * carro->velocidadeMaxima;
}
