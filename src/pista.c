/*
 * pista.c
 * ------------------------------------------------------------------
 * Implementação da pista como lista circular duplamente encadeada.
 * Veja pista.h para a explicação da estrutura.
 * ------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include "pista.h"
#include "obstaculos.h"

/* ==================================================================
 * Funções auxiliares (static = só podem ser usadas dentro deste arquivo)
 * ================================================================== */

/*
 * criarSegmento
 * Reserva memória para um segmento solto, ainda fora da lista.
 * Recebe: a curvatura do segmento.
 * Devolve: o novo segmento, apontando para ele mesmo nos dois sentidos.
 */
static Segmento *criarSegmento(float curvatura) {
    Segmento *novo = (Segmento *) malloc(sizeof(Segmento));
    if (novo == NULL) {
        printf("Erro: memoria insuficiente para criar segmento.\n");
        exit(1);
    }

    novo->indice = 0;
    novo->curvatura = curvatura;
    novo->ehLinhaDeChegada = 0;
    novo->altura = 0.0f;
    novo->nomeTrecho = "";
    novo->obstaculos = NULL;
    novo->proximo = novo;    /* sozinho, ele é o próprio vizinho */
    novo->anterior = novo;
    return novo;
}

/*
 * renumerarSegmentos
 * Depois de inserir ou remover, os índices ficam fora de ordem.
 * Esta função percorre a pista uma vez a partir da largada e
 * escreve 0, 1, 2, ... em cada segmento.
 * Recebe: a pista.
 * Devolve: nada.
 */
static void renumerarSegmentos(Pista *pista) {
    if (pista->largada == NULL) {
        return;
    }

    Segmento *atual = pista->largada;
    int contador = 0;

    /* do-while: o corpo roda pelo menos uma vez, e o laço para
     * quando voltamos à largada (uma volta completa). */
    do {
        atual->indice = contador;
        contador++;
        atual = atual->proximo;
    } while (atual != pista->largada);
}

/* ==================================================================
 * Criação da pista
 * ================================================================== */

/*
 * criarPistaVazia
 * Cria uma pista sem nenhum segmento.
 * Recebe: nada.
 * Devolve: ponteiro para a pista nova (largada = NULL).
 */
Pista *criarPistaVazia(void) {
    Pista *pista = (Pista *) malloc(sizeof(Pista));
    if (pista == NULL) {
        printf("Erro: memoria insuficiente para criar a pista.\n");
        exit(1);
    }

    pista->largada = NULL;
    return pista;
}

/*
 * adicionarTrecho
 * Coloca vários segmentos no fim da pista (logo antes da largada), todos
 * com a mesma curvatura e o mesmo nome. Um "trecho" é, por exemplo, a
 * Reta Oposta ou o Bico de Pato.
 *
 * A altura vai de alturaInicial até alturaFinal em forma de "S"
 * (f * f * (3 - 2f), com f indo de 0 a 1): a rampa começa e termina
 * sem inclinação, então a estrada não dá trancos entre um trecho e outro.
 *
 * Recebe: a pista, o nome do trecho, quantos segmentos, a curvatura de
 *         cada um e a altura no começo e no fim do trecho.
 * Devolve: nada.
 */
void adicionarTrecho(Pista *pista, const char *nome, int quantidade, float curvatura,
                     float alturaInicial, float alturaFinal) {
    for (int i = 0; i < quantidade; i++) {
        float fracao = (float) i / quantidade;
        float suave = fracao * fracao * (3.0f - 2.0f * fracao);

        Segmento *novo = inserirSegmento(pista, NULL, curvatura);
        novo->altura = alturaInicial + (alturaFinal - alturaInicial) * suave;
        novo->nomeTrecho = nome;
    }
}

/* Metade da janela da suavização das curvas: cada segmento passa a ter a
 * média da curvatura dele e dos 10 vizinhos de cada lado (21 ao todo).
 * A entrada e a saída das curvas viram rampas de ~21 segmentos (~35 m). */
#define RAIO_DA_SUAVIZACAO  10

/*
 * suavizarCurvas
 * Sem isto, a curvatura salta de 0 para o valor da curva no primeiro
 * segmento dela: o carro leva o empurrão inteiro de uma vez e a estrada
 * "dobra" de repente na tela. Aqui cada segmento recebe a média da
 * curvatura dos vizinhos (média móvel), o que troca o degrau por uma
 * rampa. A média móvel numa lista circular não muda a soma das
 * curvaturas, então a volta continua virando -360 graus.
 *
 * As curvaturas originais vão antes para um vetor: a média precisa dos
 * valores antigos dos vizinhos, e a lista não tem acesso por índice.
 * Recebe: a pista.
 * Devolve: nada (altera a curvatura de cada segmento).
 */
static void suavizarCurvas(Pista *pista) {
    int total = contarSegmentos(pista);
    if (total == 0) {
        return;
    }

    float *original = (float *) malloc(total * sizeof(float));
    if (original == NULL) {
        printf("Erro: memoria insuficiente para suavizar a pista.\n");
        exit(1);
    }

    Segmento *atual = pista->largada;
    for (int i = 0; i < total; i++) {
        original[i] = atual->curvatura;
        atual = atual->proximo;
    }

    int janela = 2 * RAIO_DA_SUAVIZACAO + 1;
    for (int i = 0; i < total; i++) {
        float soma = 0.0f;
        for (int vizinho = -RAIO_DA_SUAVIZACAO; vizinho <= RAIO_DA_SUAVIZACAO; vizinho++) {
            soma += original[(i + vizinho + total) % total];   /* dá a volta, como a lista */
        }
        atual->curvatura = soma / janela;   /* "atual" voltou à largada, e anda junto com i */
        atual = atual->proximo;
    }

    free(original);
}

/* Um trecho do autódromo com as medidas do mundo real. */
typedef struct {
    const char *nome;
    float comprimentoMetros;
    float anguloGraus;      /* quanto a pista vira no trecho: < 0 esquerda, > 0 direita */
    float desnivelMetros;   /* quanto sobe (> 0) ou desce (< 0) até o fim do trecho */
} TrechoReal;

/*
 * Interlagos, na ordem em que é corrido (sentido anti-horário), a partir
 * da linha de chegada. Dados oficiais (Prefeitura de São Paulo):
 *   - 4.309 metros de extensão;
 *   - 11 curvas (o S do Senna conta como duas);
 *   - desnível de ~56 m: ponto mais alto na entrada do S do Senna (+4 m)
 *     e mais baixo na Descida do Lago (-52 m).
 * O comprimento e o ângulo de cada trecho são aproximados a partir da
 * planta do circuito, ajustados para o desenho fechar (soma dos
 * ângulos = -360 graus, soma dos desníveis = 0, soma dos comprimentos =
 * 4.309 m).
 */
static const TrechoReal TRECHOS_INTERLAGOS[] = {
    /* nome                         metros  ângulo  desnível   altura no fim */
    { "Reta dos Boxes",               329,     0,      4 },  /*  +4 (mais alto) */
    { "S do Senna",                    87,   -95,     -9 },  /*  -5 */
    { "S do Senna",                    77,    60,     -7 },  /* -12 */
    { "Curva do Sol",                 248,  -110,    -10 },  /* -22 */
    { "Reta Oposta",                  598,   -15,    -18 },  /* -40 */
    { "Descida do Lago",              115,   -90,     -8 },  /* -48 */
    { "Descida do Lago",              155,   -60,     -4 },  /* -52 (mais baixo) */
    { "Reta",                         118,     0,      3 },  /* -49 */
    { "Ferradura",                    298,   135,      5 },  /* -44 */
    { "Reta",                         111,     0,      4 },  /* -40 */
    { "Laranjinha",                   106,    80,      4 },  /* -36 */
    { "Reta",                         151,     0,      3 },  /* -33 */
    { "Pinheirinho",                  100,  -130,      1 },  /* -32 */
    { "Reta",                         141,     0,      0 },  /* -32 */
    { "Bico de Pato",                  70,   150,      0 },  /* -32 */
    { "Reta",                         111,     0,     -2 },  /* -34 */
    { "Mergulho",                     165,   -95,     -8 },  /* -42 */
    { "Reta",                         110,     0,     -2 },  /* -44 */
    { "Juncao",                       117,  -110,     -2 },  /* -46 */
    { "Subida dos Boxes",             823,   -80,     38 },  /*  -8 */
    { "Reta dos Boxes",               279,     0,      8 },  /*   0 (de volta à largada) */
};
#define TOTAL_TRECHOS_INTERLAGOS  (int) (sizeof(TRECHOS_INTERLAGOS) / sizeof(TRECHOS_INTERLAGOS[0]))

/*
 * criarPista
 * Monta o Autódromo de Interlagos a partir da tabela TRECHOS_INTERLAGOS,
 * suaviza a entrada e a saída das curvas e põe os objetos da beira da pista.
 * Cada trecho em metros vira um bloco de segmentos:
 *   quantidade = metros * SEGMENTOS_POR_METRO
 *   curvatura  = ângulo do trecho / quantidade (graus por segmento)
 *   altura     = soma dos desníveis até ali, em unidades de mundo
 * Recebe: nada.
 * Devolve: a pista completa (o primeiro segmento é a largada).
 */
Pista *criarPista(void) {
    Pista *pista = criarPistaVazia();
    float alturaAtual = 0.0f;   /* a linha de chegada fica na altura 0 */

    for (int i = 0; i < TOTAL_TRECHOS_INTERLAGOS; i++) {
        TrechoReal trecho = TRECHOS_INTERLAGOS[i];

        int quantidade = (int) (trecho.comprimentoMetros * SEGMENTOS_POR_METRO + 0.5f);
        float curvatura = trecho.anguloGraus / quantidade;
        float alturaFinal = alturaAtual + trecho.desnivelMetros * UNIDADES_POR_METRO;

        adicionarTrecho(pista, trecho.nome, quantidade, curvatura, alturaAtual, alturaFinal);
        alturaAtual = alturaFinal;
    }

    suavizarCurvas(pista);
    decorarPista(pista);   /* árvores, pórtico, placas... (depois: as zebras dependem das curvas) */
    return pista;
}

/* ==================================================================
 * Funções sobre a lista circular
 * ================================================================== */

/*
 * inserirSegmento
 * Cria um segmento e o encaixa logo depois de "depoisDe".
 *   - Se a pista estiver vazia, o novo segmento vira a largada e
 *     aponta para ele mesmo.
 *   - Se "depoisDe" for NULL, o segmento entra no fim da pista
 *     (entre o último segmento e a largada).
 * Recebe: a pista, o segmento de referência (ou NULL) e a curvatura.
 * Devolve: o segmento criado.
 */
Segmento *inserirSegmento(Pista *pista, Segmento *depoisDe, float curvatura) {
    Segmento *novo = criarSegmento(curvatura);

    /* Caso 1: pista vazia */
    if (pista->largada == NULL) {
        novo->ehLinhaDeChegada = 1;
        pista->largada = novo;
        return novo;   /* já aponta para ele mesmo; índice 0 */
    }

    /* Caso 2: NULL significa "no fim", ou seja, depois do último */
    if (depoisDe == NULL) {
        depoisDe = pista->largada->anterior;
    }

    /* Caso 3: encaixar entre depoisDe e o seguinte dele.
     *   antes:  depoisDe <-> seguinte
     *   depois: depoisDe <-> novo <-> seguinte                */
    Segmento *seguinte = depoisDe->proximo;

    novo->anterior = depoisDe;
    novo->proximo = seguinte;
    depoisDe->proximo = novo;
    seguinte->anterior = novo;

    renumerarSegmentos(pista);
    return novo;
}

/*
 * removerSegmento
 * Tira um segmento da pista e religa os vizinhos dele.
 * Como a lista é dupla, o segmento já sabe quem é o anterior,
 * então não é preciso percorrer a pista para achá-lo.
 *   - Se for o único segmento, a pista fica vazia.
 *   - Se for a largada, o segmento seguinte vira a nova largada.
 * Recebe: a pista e o segmento a remover (que deve estar nela).
 * Devolve: 1 se removeu, 0 se não havia o que remover.
 */
int removerSegmento(Pista *pista, Segmento *segmento) {
    if (pista == NULL || pista->largada == NULL || segmento == NULL) {
        return 0;
    }

    /* Caso 1: único segmento (ele é o próprio vizinho) */
    if (segmento->proximo == segmento) {
        pista->largada = NULL;
        liberarObstaculos(segmento);
        free(segmento);
        return 1;
    }

    /* Caso 2: removendo a largada, a linha de chegada passa adiante */
    if (segmento == pista->largada) {
        pista->largada = segmento->proximo;
        pista->largada->ehLinhaDeChegada = 1;
    }

    /* Religa os vizinhos, pulando o segmento removido:
     *   antes:  anterior <-> segmento <-> proximo
     *   depois: anterior <-> proximo                          */
    segmento->anterior->proximo = segmento->proximo;
    segmento->proximo->anterior = segmento->anterior;

    liberarObstaculos(segmento);
    free(segmento);

    renumerarSegmentos(pista);
    return 1;
}

/*
 * buscarSegmento
 * Procura o segmento com o índice pedido, andando a partir da largada.
 * Recebe: a pista e o índice procurado.
 * Devolve: o segmento, ou NULL se o índice não existir.
 */
Segmento *buscarSegmento(Pista *pista, int indice) {
    if (pista == NULL || pista->largada == NULL || indice < 0) {
        return NULL;
    }

    Segmento *atual = pista->largada;
    do {
        if (atual->indice == indice) {
            return atual;
        }
        atual = atual->proximo;
    } while (atual != pista->largada);

    return NULL;   /* deu a volta inteira e não achou */
}

/*
 * proximoSegmento
 * Anda um segmento para a frente. Depois do último vem a largada,
 * e é assim que o carro "dá a volta" no circuito.
 * Recebe: o segmento atual.
 * Devolve: o segmento seguinte.
 */
Segmento *proximoSegmento(Segmento *segmento) {
    return segmento->proximo;
}

/*
 * segmentoAnterior
 * Anda um segmento para trás. Antes da largada vem o último segmento.
 * Recebe: o segmento atual.
 * Devolve: o segmento anterior.
 */
Segmento *segmentoAnterior(Segmento *segmento) {
    return segmento->anterior;
}

/*
 * contarSegmentos
 * Conta quantos segmentos a pista tem, dando uma volta completa.
 * Recebe: a pista.
 * Devolve: o número de segmentos (0 se estiver vazia).
 */
int contarSegmentos(Pista *pista) {
    if (pista == NULL || pista->largada == NULL) {
        return 0;
    }

    int total = 0;
    Segmento *atual = pista->largada;
    do {
        total++;
        atual = atual->proximo;
    } while (atual != pista->largada);

    return total;
}

/*
 * temZebra
 * Diz se o segmento é parte de uma curva, onde a pista tem zebra.
 * Recebe: o segmento.
 * Devolve: 1 se tem zebra, 0 se é reta (ou quase).
 */
int temZebra(Segmento *segmento) {
    return segmento->curvatura >= CURVATURA_MINIMA_ZEBRA ||
           segmento->curvatura <= -CURVATURA_MINIMA_ZEBRA;
}

/*
 * distanciaDoLugarAteALinha
 * Quanto um lugar do grid fica ANTES da linha de chegada.
 * Recebe: o lugar (0 = pole).
 * Devolve: a distância em unidades de mundo (sempre múltipla do segmento).
 */
float distanciaDoLugarAteALinha(int lugar) {
    return DISTANCIA_DA_POLE + lugar * ESPACO_ENTRE_LUGARES;
}

/*
 * lateralDoLugar
 * Recebe: o lugar do grid (0 = pole).
 * Devolve: o deslocamento lateral dele (pares à esquerda, ímpares à direita).
 */
float lateralDoLugar(int lugar) {
    return (lugar % 2 == 0) ? -LATERAL_DO_GRID : LATERAL_DO_GRID;
}

/*
 * moverNaPista
 * Anda uma distância para a frente (positiva) ou para trás (negativa).
 * Toda vez que a distância passa do fim do segmento, a posição pula
 * para o próximo nó da lista; se fica negativa, volta para o anterior.
 * Como a lista é circular, nunca existe "fim da pista".
 *
 * Também conta as passagens pela linha de chegada:
 *   - entrar na largada andando para a frente soma +1;
 *   - sair da largada andando para trás soma -1 (assim dar ré
 *     sobre a linha e voltar não ganha uma volta de graça).
 *
 * Recebe: a posição (é alterada) e a distância a andar.
 * Devolve: quantas voltas foram ganhas (ou perdidas) neste movimento.
 */
int moverNaPista(PosicaoNaPista *posicao, float distancia) {
    int voltas = 0;
    posicao->distanciaNoSegmento += distancia;

    /* Para a frente: passou do fim do segmento atual */
    while (posicao->distanciaNoSegmento >= COMPRIMENTO_SEGMENTO) {
        posicao->distanciaNoSegmento -= COMPRIMENTO_SEGMENTO;
        posicao->segmento = proximoSegmento(posicao->segmento);
        if (posicao->segmento->ehLinhaDeChegada) {
            voltas++;
        }
    }

    /* Para trás: voltou antes do início do segmento atual */
    while (posicao->distanciaNoSegmento < 0) {
        if (posicao->segmento->ehLinhaDeChegada) {
            voltas--;
        }
        posicao->distanciaNoSegmento += COMPRIMENTO_SEGMENTO;
        posicao->segmento = segmentoAnterior(posicao->segmento);
    }

    return voltas;
}

/*
 * alturaNaPosicao
 * Altura do chão num ponto exato da pista: mistura a altura do começo do
 * segmento com a do começo do próximo, conforme o quanto já se andou nele.
 * Recebe: a posição na pista.
 * Devolve: a altura em unidades de mundo.
 */
float alturaNaPosicao(PosicaoNaPista posicao) {
    float alturaAqui = posicao.segmento->altura;
    float alturaSeguinte = proximoSegmento(posicao.segmento)->altura;
    float fracao = posicao.distanciaNoSegmento / COMPRIMENTO_SEGMENTO;
    return alturaAqui + (alturaSeguinte - alturaAqui) * fracao;
}

/*
 * liberarPista
 * Devolve toda a memória da pista.
 * Para não andar em círculos para sempre, primeiro "corta" o círculo:
 * o último segmento passa a apontar para NULL. Aí a lista vira uma
 * lista comum, e basta liberar até encontrar NULL.
 * Recebe: a pista.
 * Devolve: nada (o ponteiro da pista não pode mais ser usado).
 */
void liberarPista(Pista *pista) {
    if (pista == NULL) {
        return;
    }

    if (pista->largada != NULL) {
        Segmento *ultimo = pista->largada->anterior;
        ultimo->proximo = NULL;   /* corta o círculo */

        Segmento *atual = pista->largada;
        while (atual != NULL) {
            Segmento *seguinte = atual->proximo;   /* guarda antes de liberar */
            liberarObstaculos(atual);
            free(atual);
            atual = seguinte;
        }
    }

    free(pista);
}
