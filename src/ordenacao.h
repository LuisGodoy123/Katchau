/*
 * ordenacao.h
 * ------------------------------------------------------------------
 * Algoritmos de ordenação do jogo.
 *   - Insertion Sort: posição dos carros na corrida (Fase 4).
 *   - Merge Sort: carros da garagem por atributo (Fase 6).
 *
 * Ordenamos um vetor de PONTEIROS para Carro, e não os carros em si:
 * trocar dois ponteiros é barato (8 bytes) e os carros continuam no
 * mesmo lugar da memória, então quem guarda um Carro* não se perde.
 *
 * Este módulo NÃO usa a raylib.
 * ------------------------------------------------------------------
 */

#ifndef ORDENACAO_H
#define ORDENACAO_H

#include "carro.h"

int ordenarCarrosPorDistancia(Carro *carros[], int quantidade);

#endif
