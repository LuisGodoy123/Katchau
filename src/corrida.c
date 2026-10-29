/*
 * corrida.c
 * ------------------------------------------------------------------
 * Regras da corrida. Veja corrida.h.
 * ------------------------------------------------------------------
 */

#include <stddef.h>
#include "corrida.h"
#include "ia.h"
#include "ordenacao.h"

/* ---------- Grid de largada ---------- */
#define ESPACO_ENTRE_LUGARES   (1.5f * COMPRIMENTO_SEGMENTO)   /* cada lugar um pouco atrás do anterior */
#define LATERAL_DO_GRID        0.4f    /* esquerda -0,4 / direita +0,4 */
#define LUGAR_DO_JOGADOR       (TOTAL_CARROS - 1)   /* o jogador larga em último */

/* ---------- Colisão ---------- */
#define DISTANCIA_LATERAL_BATIDA  0.25f   /* mais perto que isso, os carros se tocam */
#define PERDA_NA_BATIDA           0.8f    /* quem bate atrás fica com 80% da velocidade do outro */

/* ---------- Contagem regressiva ---------- */
#define SEGUNDOS_DE_CONTAGEM   3.0f    /* 3, 2, 1... KATCHAU! */

/* Os 8 carros do jogo. O número de cada um é a posição neste vetor e
 * define a cor no desenho (0 = vermelho, 1 = amarelo...). O jogador
 * escolhe um deles na garagem; os outros 7 viram os rivais.
 * A aderência pesa muito no tempo de volta, então ela varia pouco.
 * Os nomes e o jeito de cada um são originais do KATCHAU. */
static const FichaTecnica FICHAS[TOTAL_CARROS] = {
    /* nome         descrição                         vel.max   acel.    aderência */
    { "Katchau",    "o carro da casa, equilibrado",    12000.0f, 2400.0f, 0.50f },
    { "Barao",      "esportivo arrogante, voa na reta", 12500.0f, 2200.0f, 0.47f },
    { "Neon",       "gruda nas curvas",                11700.0f, 2400.0f, 0.55f },
    { "Trovao",     "arranca forte",                   12000.0f, 2700.0f, 0.49f },
    { "Ze Pistao",  "certinho em tudo",                11800.0f, 2300.0f, 0.51f },
    { "Fuscao",     "lento, mas nao sai da pista",     11100.0f, 2300.0f, 0.58f },
    { "Bolinha",    "pequeno e esperto na largada",    11400.0f, 2800.0f, 0.51f },
    { "Ferrugem",   "picape enferrujada",              11200.0f, 2100.0f, 0.54f },
};

/*
 * fichaDoCarro
 * Recebe: o número do carro (0 a TOTAL_CARROS - 1).
 * Devolve: a ficha técnica dele.
 */
const FichaTecnica *fichaDoCarro(int numero) {
    return &FICHAS[numero];
}

/*
 * colocarNoGrid
 * Põe o carro no seu lugar de largada. O grid fica logo DEPOIS da
 * linha de chegada, em zigue-zague (um à esquerda, um à direita, cada
 * um um pouco atrás do outro): o lugar 0 (pole) é o mais à frente.
 * Quem larga na frente sai alguns metros adiantado, e a distância
 * percorrida já começa contando esses metros, para a classificação
 * da largada sair certa e sem empates.
 * Recebe: o carro (já criado na linha de chegada) e o lugar (0 = pole).
 * Devolve: nada.
 */
static void colocarNoGrid(Carro *carro, int lugar) {
    float distanciaDaLinha = (TOTAL_CARROS - 1 - lugar) * ESPACO_ENTRE_LUGARES;

    moverNaPista(&carro->posicao, distanciaDaLinha);
    carro->distanciaPercorrida = distanciaDaLinha;

    /* Lugares pares à esquerda, ímpares à direita */
    carro->deslocamentoLateral = (lugar % 2 == 0) ? -LATERAL_DO_GRID : LATERAL_DO_GRID;
    carro->linhaAlvo = carro->deslocamentoLateral;
}

/*
 * criarCarroDaFicha
 * Cria um carro na linha de chegada a partir de uma ficha técnica.
 * Recebe: a ficha, o número do carro e a pista.
 * Devolve: o carro.
 */
static Carro criarCarroDaFicha(FichaTecnica ficha, int numero, Pista *pista) {
    return criarCarro(ficha.nome, numero, pista->largada, 0.0f,
                      ficha.velocidadeMaxima, ficha.aceleracao, ficha.aderencia);
}

/*
 * iniciarCorrida
 * Prepara uma corrida nova: o jogador com o carro escolhido na garagem,
 * os outros 7 carros como rivais, todos no grid, relógio zerado,
 * contagem regressiva armada e a classificação montada.
 * Serve também para correr de novo.
 * Recebe: a corrida a preparar, a pista e o número do carro do jogador.
 * Devolve: nada.
 */
void iniciarCorrida(Corrida *corrida, Pista *pista, int carroDoJogador) {
    corrida->pista = pista;
    corrida->tempoDeCorrida = 0.0f;
    corrida->contagemRegressiva = SEGUNDOS_DE_CONTAGEM;
    corrida->terminou = 0;
    corrida->posicaoFinal = 0;

    corrida->jogador = criarCarroDaFicha(FICHAS[carroDoJogador], carroDoJogador, pista);
    colocarNoGrid(&corrida->jogador, LUGAR_DO_JOGADOR);
    corrida->classificacao[0] = &corrida->jogador;

    /* Rivais: todos os carros, menos o que o jogador pegou */
    int rival = 0;
    for (int numero = 0; numero < TOTAL_CARROS; numero++) {
        if (numero == carroDoJogador) {
            continue;
        }
        corrida->rivais[rival] = criarCarroDaFicha(FICHAS[numero], numero, pista);
        colocarNoGrid(&corrida->rivais[rival], rival);   /* rivais nos lugares 0 a 6 */
        corrida->classificacao[rival + 1] = &corrida->rivais[rival];
        rival++;
    }

    ordenarCarrosPorDistancia(corrida->classificacao, TOTAL_CARROS);
}

/*
 * registrarVolta
 * Guarda quanto tempo durou a volta que o carro acabou de completar.
 * Na última volta, guarda também o tempo de chegada.
 * Recebe: o carro (com carro->voltas já somado) e o tempo atual da corrida.
 * Devolve: nada.
 */
static void registrarVolta(Carro *carro, float tempoDeCorrida) {
    int indiceDaVolta = carro->voltas - 1;   /* volta 1 fica na posição 0 */
    if (indiceDaVolta >= 0 && indiceDaVolta < TOTAL_VOLTAS) {
        carro->temposDeVolta[indiceDaVolta] = tempoDeCorrida - carro->inicioDaVolta;
    }
    if (carro->voltas == TOTAL_VOLTAS) {
        carro->tempoDeChegada = tempoDeCorrida;
    }
    carro->inicioDaVolta = tempoDeCorrida;
}

/*
 * tratarBatida
 * Se dois carros estão no mesmo segmento e lado a lado, quem vem atrás
 * bate na traseira do outro e perde velocidade.
 * Recebe: os dois carros.
 * Devolve: nada.
 */
static void tratarBatida(Carro *a, Carro *b) {
    if (a->posicao.segmento != b->posicao.segmento) {
        return;
    }

    float diferencaLateral = a->deslocamentoLateral - b->deslocamentoLateral;
    if (diferencaLateral < 0) {
        diferencaLateral = -diferencaLateral;
    }
    if (diferencaLateral > DISTANCIA_LATERAL_BATIDA) {
        return;
    }

    /* Descobre quem está atrás dentro do segmento */
    Carro *atras = a;
    Carro *frente = b;
    if (a->posicao.distanciaNoSegmento > b->posicao.distanciaNoSegmento) {
        atras = b;
        frente = a;
    }

    /* Só é batida se quem está atrás vem mais rápido (está alcançando) */
    if (atras->velocidade > frente->velocidade) {
        atras->velocidade = frente->velocidade * PERDA_NA_BATIDA;
    }
}

/*
 * verificarBatidas
 * Testa todos os pares de carros (8 carros = 28 pares).
 * Recebe: a corrida.
 * Devolve: nada.
 */
static void verificarBatidas(Corrida *corrida) {
    for (int i = 0; i < TOTAL_CARROS; i++) {
        for (int j = i + 1; j < TOTAL_CARROS; j++) {
            tratarBatida(corrida->classificacao[i], corrida->classificacao[j]);
        }
    }
}

/*
 * atualizarCorrida
 * Avança a corrida um quadro:
 *   1) move o jogador com os comandos do teclado;
 *   2) move cada rival com os comandos da IA;
 *   3) trata as batidas;
 *   4) reordena a classificação (Insertion Sort);
 *   5) verifica se o jogador terminou.
 * Antes disso, enquanto a contagem regressiva não acaba, ninguém sai
 * do lugar e o relógio não anda.
 * Depois do fim, o jogador segue sem acelerar até parar e os rivais
 * continuam correndo. O relógio também continua, para cada rival ter
 * o seu tempo de chegada; o do jogador fica guardado no carro dele.
 * Recebe: a corrida, os comandos do jogador e o tempo do quadro.
 * Devolve: nada.
 */
void atualizarCorrida(Corrida *corrida, Comandos comandosJogador, float tempoQuadro) {
    if (corrida->contagemRegressiva > 0.0f) {
        corrida->contagemRegressiva -= tempoQuadro;
        return;
    }

    corrida->tempoDeCorrida += tempoQuadro;
    if (corrida->terminou) {
        Comandos nenhumComando = { 0, 0, 0, 0 };
        comandosJogador = nenhumComando;
    }

    /* 1) Jogador */
    if (atualizarCarro(&corrida->jogador, comandosJogador, tempoQuadro) > 0) {
        registrarVolta(&corrida->jogador, corrida->tempoDeCorrida);
    }

    /* 2) Rivais: a IA olha o carro logo à frente de cada um */
    for (int i = 0; i < TOTAL_RIVAIS; i++) {
        Carro *rival = &corrida->rivais[i];
        Comandos comandosIA = atualizarIA(rival, carroDaFrente(corrida, rival));
        if (atualizarCarro(rival, comandosIA, tempoQuadro) > 0) {
            registrarVolta(rival, corrida->tempoDeCorrida);
        }
    }

    /* 3) Batidas */
    verificarBatidas(corrida);

    /* 4) Classificação */
    ordenarCarrosPorDistancia(corrida->classificacao, TOTAL_CARROS);

    /* 5) Fim de corrida */
    if (!corrida->terminou && corrida->jogador.voltas >= TOTAL_VOLTAS) {
        corrida->terminou = 1;
        corrida->posicaoFinal = posicaoDoCarro(corrida, &corrida->jogador);
    }
}

/*
 * todosChegaram
 * Recebe: a corrida.
 * Devolve: 1 se os 8 carros já completaram TOTAL_VOLTAS, 0 se não.
 */
int todosChegaram(Corrida *corrida) {
    for (int i = 0; i < TOTAL_CARROS; i++) {
        if (corrida->classificacao[i]->voltas < TOTAL_VOLTAS) {
            return 0;
        }
    }
    return 1;
}

/*
 * voltaAtual
 * Número da volta que o carro está correndo, para mostrar na tela.
 * Recebe: o carro.
 * Devolve: de 1 até TOTAL_VOLTAS.
 */
int voltaAtual(Carro *carro) {
    int volta = carro->voltas + 1;
    if (volta > TOTAL_VOLTAS) {
        volta = TOTAL_VOLTAS;
    }
    return volta;
}

/*
 * posicaoDoCarro
 * Procura o carro na classificação.
 * Recebe: a corrida e o carro.
 * Devolve: a posição (1 = primeiro), ou 0 se o carro não estiver nela.
 */
int posicaoDoCarro(Corrida *corrida, Carro *carro) {
    for (int i = 0; i < TOTAL_CARROS; i++) {
        if (corrida->classificacao[i] == carro) {
            return i + 1;
        }
    }
    return 0;
}

/*
 * carroDaFrente
 * Como a classificação está ordenada, o carro logo à frente é
 * simplesmente o anterior no vetor.
 * Recebe: a corrida e o carro.
 * Devolve: o carro logo à frente, ou NULL se ele for o 1º.
 */
Carro *carroDaFrente(Corrida *corrida, Carro *carro) {
    int posicao = posicaoDoCarro(corrida, carro);
    if (posicao <= 1) {
        return NULL;
    }
    return corrida->classificacao[posicao - 2];   /* posição 2 -> índice 0 */
}
