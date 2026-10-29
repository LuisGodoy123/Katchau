/*
 * ordenacao.h
 * ------------------------------------------------------------------
 * Algoritmos de ordenação do jogo.
 *   - Insertion Sort: posição dos carros na corrida (Fase 4).
 *   - Merge Sort: carros da garagem por atributo (Fase 6).
 *
 * Os dois ordenam vetores de PONTEIROS (para Carro ou para FichaTecnica),
 * e não as structs em si: trocar dois ponteiros é barato (8 bytes) e os
 * dados continuam no mesmo lugar da memória, então quem guarda um
 * ponteiro para eles não se perde.
 *
 * Este módulo NÃO usa a raylib.
 * ------------------------------------------------------------------
 */

#ifndef ORDENACAO_H
#define ORDENACAO_H

#include "carro.h"

/* Critérios de ordenação da garagem */
typedef enum {
    ATRIBUTO_VELOCIDADE,
    ATRIBUTO_ACELERACAO,
    ATRIBUTO_ADERENCIA,
    TOTAL_ATRIBUTOS
} Atributo;

int ordenarCarrosPorDistancia(Carro *carros[], int quantidade);

void ordenarFichasPorAtributo(const FichaTecnica *fichas[], int quantidade, Atributo atributo);
float valorDoAtributo(const FichaTecnica *ficha, Atributo atributo);
const char *nomeDoAtributo(Atributo atributo);

#endif
