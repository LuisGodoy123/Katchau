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
#define GIRO_DO_VOLANTE            6.0f  /* com a seta apertada, o volante vai de 0 a 1 em ~0,17 s */
#define RETORNO_DO_VOLANTE         8.0f  /* sem seta, o volante volta ao centro em ~0,12 s */
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
    carro.volante = 0.0f;
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
 * aproximar
 * Leva um valor na direção de um alvo, no máximo "passo" por chamada,
 * sem passar do alvo.
 * Recebe: o valor atual, o alvo e o passo máximo.
 * Devolve: o novo valor.
 */
static float aproximar(float valor, float alvo, float passo) {
    if (valor < alvo) {
        return (valor + passo > alvo) ? alvo : valor + passo;
    }
    return (valor - passo < alvo) ? alvo : valor - passo;
}

/*
 * atualizarVolante
 * O volante não vai direto para o fim: com a seta apertada ele gira aos
 * poucos até -1 (esquerda) ou +1 (direita); sem seta, volta sozinho ao
 * centro. É como a velocidade, que também sobe e desce aos poucos.
 * Isso deixa a curva suave e a pose do carro na tela sem trancos.
 * Recebe: o carro, os comandos e o tempo do quadro em segundos.
 * Devolve: nada (altera carro->volante).
 */
static void atualizarVolante(Carro *carro, Comandos comandos, float tempoQuadro) {
    float alvo = (float) (comandos.direita - comandos.esquerda);   /* -1, 0 ou +1 */

    if (alvo == 0.0f) {
        carro->volante = aproximar(carro->volante, 0.0f, RETORNO_DO_VOLANTE * tempoQuadro);
    } else {
        carro->volante = aproximar(carro->volante, alvo, GIRO_DO_VOLANTE * tempoQuadro);
    }
}

/*
 * atualizarLateral
 * Vira o carro conforme o volante e aplica o empurrão da curva.
 * Recebe: o carro e o tempo do quadro em segundos.
 * Devolve: nada (altera carro->deslocamentoLateral).
 */
static void atualizarLateral(Carro *carro, float tempoQuadro) {
    /* 0 parado, 1 na velocidade máxima */
    float proporcaoVelocidade = carro->velocidade / carro->velocidadeMaxima;

    /* Volante: o quanto vira depende de quanto o volante está girado */
    float virada = VELOCIDADE_DE_VIRADA * proporcaoVelocidade * carro->volante * tempoQuadro;
    carro->deslocamentoLateral += virada;

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
 * Avança o carro um quadro: velocidade, volante, lateral e depois anda
 * pela lista da pista com moverNaPista().
 * Recebe: o carro, os comandos e o tempo do quadro em segundos.
 * Devolve: quantas voltas o carro completou neste quadro (0 ou 1).
 */
int atualizarCarro(Carro *carro, Comandos comandos, float tempoQuadro) {
    carro->ultimosComandos = comandos;
    atualizarVelocidade(carro, comandos, tempoQuadro);
    atualizarVolante(carro, comandos, tempoQuadro);
    atualizarLateral(carro, tempoQuadro);

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
