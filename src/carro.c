/*
 * carro.c
 * ------------------------------------------------------------------
 * Movimento do carro. A física é de fliperama, não realista:
 *
 *   - acelerar soma velocidade, como num F1: muita força na arrancada
 *     e cada vez menos perto da máxima (o ar segura o carro); frear e
 *     soltar o pedal tiram;
 *   - na grama a velocidade cai até um limite baixo;
 *   - virar muda a posição lateral; em baixa velocidade a direção já
 *     responde bem, e parado o carro não vira;
 *   - na curva o carro é empurrado para fora, e o empurrão cresce
 *     com o quadrado da velocidade: em alta, ou se freia antes da
 *     curva, ou se vai parar na grama. Perto desse limite o carro
 *     "derrapa" (carro->derrapagem), e o desenho solta fumaça.
 * ------------------------------------------------------------------
 */

#include <string.h>
#include "carro.h"

#define FORCA_FREIO            12000.0f  /* quanto a velocidade cai por segundo freando */
#define ATRITO                  2400.0f  /* quanto cai por segundo sem acelerar */
#define ATRITO_GRAMA            4500.0f  /* quanto cai por segundo na grama */
#define VELOCIDADE_MAXIMA_GRAMA 3000.0f  /* na grama o carro não passa disso */
#define VELOCIDADE_DE_VIRADA       2.0f  /* larguras de pista por segundo, na velocidade máxima */
#define GIRO_DO_VOLANTE            6.0f  /* com a seta apertada, o volante vai de 0 a 1 em ~0,17 s */
#define RETORNO_DO_VOLANTE         8.0f  /* sem seta, o volante volta ao centro em ~0,12 s */
#define FORCA_CENTRIFUGA           6.0f  /* intensidade do empurrão para fora nas curvas */
#define FORCA_DA_ARRANCADA         0.75f /* a aceleração parado é 75% da "aceleracao" da ficha... */
#define PERDA_NA_MAXIMA            0.8f  /* ...e na velocidade máxima cai 80% (sobram 20%) */
#define INICIO_DA_DERRAPAGEM       0.7f  /* a curva usando 70% do volante: começa a derrapar */
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
    carro.derrapagem = 0.0f;
    carro.voltas = 0;
    for (int i = 0; i < TOTAL_VOLTAS; i++) {
        carro.temposDeVolta[i] = 0.0f;
    }
    carro.inicioDaVolta = 0.0f;
    carro.melhorVolta = 0.0f;
    carro.setorAtual = 0;
    carro.inicioDoSetor = 0.0f;
    for (int s = 0; s < TOTAL_SETORES; s++) {
        carro.setoresDaVolta[s] = 0.0f;
        carro.melhoresSetores[s] = 0.0f;
        carro.setoresDaMelhorVolta[s] = 0.0f;
    }
    carro.diferenca = 0.0f;
    carro.temDiferenca = 0;
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
        /* Curva de F1: a aceleração cai com o quadrado da velocidade.
         * Com a ficha do Katchau: 0-100 km/h em ~2,3 s, 0-200 em ~5 s
         * e 0-300 em ~11 s (antes eram 5 s, com aceleração constante). */
        float proporcao = carro->velocidade / carro->velocidadeMaxima;
        float forca = 1.0f - PERDA_NA_MAXIMA * proporcao * proporcao;
        carro->velocidade += carro->aceleracao * FORCA_DA_ARRANCADA * forca * tempoQuadro;
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
 * respostaDaDirecao
 * Quanto o volante vira o carro, conforme a velocidade: p * (2 - p),
 * onde p vai de 0 (parado) a 1 (velocidade máxima). Parado dá 0, na
 * máxima dá 1, e no meio dá mais que p: a 100 km/h a direção já tem 56%
 * da força (com uma reta, p, seriam só 33%, e o carro ficava "duro").
 * Recebe: a proporção da velocidade máxima (0 a 1).
 * Devolve: de 0 a 1.
 */
static float respostaDaDirecao(float proporcaoVelocidade) {
    return proporcaoVelocidade * (2.0f - proporcaoVelocidade);
}

/*
 * atualizarLateral
 * Vira o carro conforme o volante e aplica o empurrão da curva.
 * Também mede a derrapagem: quanto do volante a curva está "gastando".
 * Recebe: o carro e o tempo do quadro em segundos.
 * Devolve: nada (altera carro->deslocamentoLateral e carro->derrapagem).
 */
static void atualizarLateral(Carro *carro, float tempoQuadro) {
    /* 0 parado, 1 na velocidade máxima */
    float proporcaoVelocidade = carro->velocidade / carro->velocidadeMaxima;
    float direcao = VELOCIDADE_DE_VIRADA * respostaDaDirecao(proporcaoVelocidade);

    /* Volante: o quanto vira depende de quanto o volante está girado */
    carro->deslocamentoLateral += direcao * carro->volante * tempoQuadro;

    /* Força centrífuga: curva para a direita (curvatura > 0) empurra
     * para a esquerda, e vice-versa. Mais aderência, menos empurrão. */
    float curvatura = carro->posicao.segmento->curvatura;
    float empurrao = curvatura * FORCA_CENTRIFUGA * proporcaoVelocidade * proporcaoVelocidade
                     * (1.0f - carro->aderencia);
    carro->deslocamentoLateral -= empurrao * tempoQuadro;

    /* Derrapagem: 0 enquanto o empurrão gasta menos de 70% da direção,
     * 1 quando gasta tudo (dali em diante o carro vai para fora) */
    float uso = 0.0f;
    if (direcao > 0.0f) {
        uso = (empurrao < 0 ? -empurrao : empurrao) / direcao;
    }
    carro->derrapagem = limitar((uso - INICIO_DA_DERRAPAGEM) / (1.0f - INICIO_DA_DERRAPAGEM), 0.0f, 1.0f);

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
 * Vem de igualar as duas fórmulas de atualizarLateral(), chamando
 * V = VELOCIDADE_DE_VIRADA e K = curvatura * FORCA_CENTRIFUGA * (1 - aderencia):
 *
 *     volante  = V * p * (2 - p)
 *     empurrão = K * p * p
 *
 *   volante = empurrão  ->  V * (2 - p) = K * p  ->  p = 2V / (K + V)
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

    float k = curvatura * FORCA_CENTRIFUGA * (1.0f - carro->aderencia);
    float proporcao = 2.0f * VELOCIDADE_DE_VIRADA / (k + VELOCIDADE_DE_VIRADA);
    if (proporcao > 1.0f) {
        proporcao = 1.0f;   /* curva suave: dá para fazer no máximo */
    }
    return proporcao * carro->velocidadeMaxima;
}

/*
 * tempoAteVelocidade
 * Quantos segundos o carro leva, saindo parado e acelerando sempre, para
 * chegar a uma velocidade. A garagem usa para mostrar o 0 a 100 km/h.
 * Como a aceleração muda com a velocidade (curva de F1), a conta é feita
 * andando em passinhos de 1/240 s com a mesma fórmula de atualizarVelocidade().
 * Recebe: a ficha do carro e a velocidade alvo (unidades por segundo).
 * Devolve: o tempo em segundos (0 se o carro nunca chega nela).
 */
float tempoAteVelocidade(const FichaTecnica *ficha, float velocidade) {
    if (velocidade > ficha->velocidadeMaxima) {
        return 0.0f;
    }
    float passo = 1.0f / 240.0f;
    float atual = 0.0f;
    float tempo = 0.0f;
    while (atual < velocidade) {
        float proporcao = atual / ficha->velocidadeMaxima;
        float forca = 1.0f - PERDA_NA_MAXIMA * proporcao * proporcao;
        atual += ficha->aceleracao * FORCA_DA_ARRANCADA * forca * passo;
        tempo += passo;
    }
    return tempo;
}
