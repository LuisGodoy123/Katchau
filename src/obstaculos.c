/*
 * obstaculos.c
 * ------------------------------------------------------------------
 * A lista simples de objetos de cada segmento e as marcas do grid
 * de largada. Veja obstaculos.h para a ideia geral.
 * ------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include "obstaculos.h"

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
 * O grid
 * ================================================================== */

/*
 * marcarGrid
 * Pinta o colchete de cada lugar do grid, nos segmentos antes da linha
 * de chegada onde os carros largam (distanciaDoLugarAteALinha em pista.c).
 * Recebe: a pista.
 * Devolve: nada.
 */
static void marcarGrid(Pista *pista) {
    for (int lugar = 0; lugar < LUGARES_NO_GRID; lugar++) {
        int voltar = (int) (distanciaDoLugarAteALinha(pista, lugar) / COMPRIMENTO_SEGMENTO + 0.5f);
        Segmento *segmento = pista->largada;
        for (int i = 0; i < voltar; i++) {
            segmento = segmentoAnterior(segmento);
        }
        adicionarObstaculo(segmento, OBJETO_MARCA_GRID, lateralDoLugar(pista, lugar));
    }
}

/*
 * decorarOval
 * Nas pistas que não trazem o grid pintado no modelo: só as marcas dele. Todo o
 * resto do cenário (muros, placas, arquibancadas) já está no modelo 3D.
 * Recebe: a pista (já montada).
 * Devolve: nada.
 */
void decorarOval(Pista *pista) {
    if (pista != NULL && pista->largada != NULL) {
        marcarGrid(pista);
    }
}
