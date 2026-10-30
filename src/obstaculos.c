/*
 * obstaculos.c
 * ------------------------------------------------------------------
 * Lista simples de objetos de cada segmento e a decoração de
 * Interlagos. Veja obstaculos.h para a ideia geral.
 * ------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include "obstaculos.h"

/* Onde cada coisa fica, em larguras de meia pista a partir do centro.
 * O asfalto vai até ±1 e o carro anda até ±2,5 (LIMITE_LATERAL em
 * carro.c): o que fica além de 2,5 nunca está no caminho do carro. */
#define LADO_PNEUS          2.7f   /* barreira de pneus por fora das curvas */
#define LADO_PLACAS         2.7f
#define LADO_ARVORE_MINIMO  3.0f
#define LADO_ARVORE_MAXIMO  6.0f

/* De quantos em quantos segmentos se repete cada coisa */
#define ESPACO_PNEUS        6

#define CHANCE_ARVORE       0.12f  /* em cada lado de cada segmento */
#define SEMENTE_DECORACAO   2026u  /* sorteio fixo: a pista é sempre igual */

/* ==================================================================
 * Lista simples de um segmento
 * ================================================================== */

/*
 * adicionarObstaculo
 * Cria um objeto e o coloca no começo da lista do segmento. Inserir no
 * começo é O(1): não precisa andar até o fim da lista.
 * Recebe: o segmento, o tipo do objeto e onde ele fica na lateral.
 * Devolve: nada.
 */
void adicionarObstaculo(Segmento *segmento, TipoObjeto tipo, float deslocamentoLateral) {
    Obstaculo *novo = (Obstaculo *) malloc(sizeof(Obstaculo));
    if (novo == NULL) {
        printf("Erro: memoria insuficiente para criar obstaculo.\n");
        exit(1);
    }

    novo->tipo = tipo;
    novo->deslocamentoLateral = deslocamentoLateral;
    novo->proximo = segmento->obstaculos;   /* o antigo primeiro vira o segundo */
    segmento->obstaculos = novo;
}

/*
 * contarObstaculos
 * Conta os objetos de um segmento, andando pela lista até o NULL.
 * Recebe: o segmento.
 * Devolve: quantos objetos ele tem.
 */
int contarObstaculos(Segmento *segmento) {
    int total = 0;
    for (Obstaculo *atual = segmento->obstaculos; atual != NULL; atual = atual->proximo) {
        total++;
    }
    return total;
}

/*
 * liberarObstaculos
 * Devolve a memória de todos os objetos de um segmento.
 * Recebe: o segmento.
 * Devolve: nada (a lista do segmento fica vazia).
 */
void liberarObstaculos(Segmento *segmento) {
    Obstaculo *atual = segmento->obstaculos;
    while (atual != NULL) {
        Obstaculo *seguinte = atual->proximo;   /* guarda antes de liberar */
        free(atual);
        atual = seguinte;
    }
    segmento->obstaculos = NULL;
}

/* ==================================================================
 * Decoração de Interlagos
 * ================================================================== */

/* Sorteio próprio (gerador congruente linear), em vez do rand(): com a
 * mesma semente ele dá sempre a mesma sequência em qualquer sistema,
 * então as árvores ficam sempre no mesmo lugar. */
static unsigned int semente;

/*
 * sortear
 * Recebe: nada.
 * Devolve: um número entre 0 e 1.
 */
static float sortear(void) {
    semente = semente * 1103515245u + 12345u;
    return ((semente >> 16) & 0x7FFF) / 32767.0f;
}

/*
 * colocarPlacas
 * Coloca as placas de 150, 100 e 50 metros antes da entrada de uma
 * curva, por fora dela (curva à esquerda: placas à direita). A placa
 * só entra se o lugar dela for reta: se antes da curva vem outra
 * curva logo em seguida, as placas mais distantes ficam de fora.
 * Recebe: o primeiro segmento da curva.
 * Devolve: nada.
 */
static void colocarPlacas(Segmento *entrada) {
    static const int METROS[] = { 50, 100, 150 };
    static const TipoObjeto PLACAS[] = { OBJETO_PLACA_50, OBJETO_PLACA_100, OBJETO_PLACA_150 };
    float lado = (entrada->curvatura < 0) ? LADO_PLACAS : -LADO_PLACAS;

    for (int p = 0; p < 3; p++) {
        int voltar = (int) (METROS[p] * SEGMENTOS_POR_METRO + 0.5f);
        Segmento *lugar = entrada;
        int ehReta = 1;
        for (int i = 0; i < voltar; i++) {
            lugar = segmentoAnterior(lugar);
            if (temZebra(lugar)) {
                ehReta = 0;   /* bateu em outra curva: não cabe esta placa */
            }
        }
        if (!ehReta) {
            return;   /* as placas maiores ficariam mais longe ainda */
        }
        adicionarObstaculo(lugar, PLACAS[p], lado);
    }
}

/*
 * marcarGrid
 * Pinta o colchete de cada lugar do grid, nos segmentos antes da linha
 * de chegada onde os carros largam (distanciaDoLugarAteALinha em pista.c).
 * Recebe: a pista.
 * Devolve: nada.
 */
static void marcarGrid(Pista *pista) {
    for (int lugar = 0; lugar < LUGARES_NO_GRID; lugar++) {
        int voltar = (int) (distanciaDoLugarAteALinha(lugar) / COMPRIMENTO_SEGMENTO + 0.5f);
        Segmento *segmento = pista->largada;
        for (int i = 0; i < voltar; i++) {
            segmento = segmentoAnterior(segmento);
        }
        adicionarObstaculo(segmento, OBJETO_MARCA_GRID, lateralDoLugar(lugar));
    }
}

/*
 * decorarPista
 * Põe os objetos da pista, numa volta pela lista:
 *   - o pórtico em cima da linha de chegada e o colchete de cada lugar
 *     do grid, antes dela;
 *   - barreiras de pneus por fora das curvas (as que têm zebra);
 *   - placas de 150, 100 e 50 m antes de cada curva;
 *   - árvores sorteadas dos dois lados.
 * Tudo fica além de onde o carro consegue chegar, menos o pórtico, que
 * passa por cima da pista, e as marcas do grid, que são tinta no chão.
 * Recebe: a pista (já montada).
 * Devolve: nada.
 */
void decorarPista(Pista *pista) {
    if (pista == NULL || pista->largada == NULL) {
        return;
    }
    semente = SEMENTE_DECORACAO;

    adicionarObstaculo(pista->largada, OBJETO_PORTICO, 0.0f);
    marcarGrid(pista);

    Segmento *atual = pista->largada;
    do {
        if (temZebra(atual)) {
            if (atual->indice % ESPACO_PNEUS == 0) {
                float fora = (atual->curvatura < 0) ? LADO_PNEUS : -LADO_PNEUS;
                adicionarObstaculo(atual, OBJETO_PNEUS, fora);
            }
            if (!temZebra(segmentoAnterior(atual))) {
                colocarPlacas(atual);
            }
        }

        /* Árvores: um sorteio para cada lado (-1 esquerda, +1 direita) */
        for (int lado = -1; lado <= 1; lado += 2) {
            if (sortear() < CHANCE_ARVORE) {
                float distancia = LADO_ARVORE_MINIMO
                                  + sortear() * (LADO_ARVORE_MAXIMO - LADO_ARVORE_MINIMO);
                adicionarObstaculo(atual, OBJETO_ARVORE, lado * distancia);
            }
        }

        atual = proximoSegmento(atual);
    } while (atual != pista->largada);
}
