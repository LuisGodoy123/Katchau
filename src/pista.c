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
 * Coloca vários segmentos iguais no fim da pista (logo antes da largada).
 * Um "trecho" é, por exemplo, uma reta de 200 segmentos ou uma curva
 * de 30 segmentos com a mesma curvatura.
 * Recebe: a pista, quantos segmentos e a curvatura de cada um.
 * Devolve: nada.
 */
void adicionarTrecho(Pista *pista, int quantidade, float curvatura) {
    for (int i = 0; i < quantidade; i++) {
        inserirSegmento(pista, NULL, curvatura);
    }
}

/*
 * criarPista
 * Monta o autódromo do KATCHAU, corrido no sentido horário.
 *
 * O desenho tem 4 lados e 4 curvas de 90 graus à direita:
 *
 *        Curva 2 (aberta) ---- Reta oposta ---- Curva 3 (fechada)
 *              |                                      |
 *      Reta + chicane + reta                  Reta + esse + reta
 *              |                                      |
 *        Curva 1 (fechada) -- Reta principal -- Curva 4 (aberta)
 *                              ^ largada
 *
 * Lados opostos têm o mesmo comprimento (as retas dos lados 2 e 4
 * foram ajustadas para compensar a chicane e o esse), então o traçado fecha
 * (isso vai importar no mini-mapa da Fase 5).
 *
 * Recebe: nada.
 * Devolve: a pista completa.
 */
Pista *criarPista(void) {
    Pista *pista = criarPistaVazia();

    /* Lado 1: reta principal. O primeiro segmento vira a largada. */
    adicionarTrecho(pista, 200, RETA);

    /* Curva 1: fechada à direita (60 x 1,5 grau = 90 graus) */
    adicionarTrecho(pista, 60, CURVA_FECHADA);

    /* Lado 2: reta, chicane (esquerda-direita-esquerda) e reta */
    adicionarTrecho(pista, 136, RETA);
    adicionarTrecho(pista, 20, -CURVA_CHICANE);
    adicionarTrecho(pista, 40, CURVA_CHICANE);
    adicionarTrecho(pista, 20, -CURVA_CHICANE);
    adicionarTrecho(pista, 137, RETA);

    /* Curva 2: aberta à direita (180 x 0,5 grau = 90 graus) */
    adicionarTrecho(pista, 180, CURVA_ABERTA);

    /* Lado 3: reta oposta, do mesmo tamanho da reta principal */
    adicionarTrecho(pista, 200, RETA);

    /* Curva 3: fechada à direita, igual à curva 1 */
    adicionarTrecho(pista, 60, CURVA_FECHADA);

    /* Lado 4: reta, esse suave (esquerda-direita-esquerda) e reta */
    adicionarTrecho(pista, 60, RETA);
    adicionarTrecho(pista, 60, -CURVA_ESSE);
    adicionarTrecho(pista, 120, CURVA_ESSE);
    adicionarTrecho(pista, 60, -CURVA_ESSE);
    adicionarTrecho(pista, 60, RETA);

    /* Curva 4: aberta à direita, desemboca na reta principal */
    adicionarTrecho(pista, 180, CURVA_ABERTA);

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

    /* Fase 5: aqui também será liberada a lista de obstáculos */
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
            /* Fase 5: aqui também será liberada a lista de obstáculos */
            free(atual);
            atual = seguinte;
        }
    }

    free(pista);
}
