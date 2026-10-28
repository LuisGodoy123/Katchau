/*
 * corrida.h
 * ------------------------------------------------------------------
 * Regras da corrida: grid, tempo, voltas, colisão, classificação e fim.
 * Na Fase 5 entram o turbo (com vácuo) e os obstáculos.
 *
 * Este módulo NÃO usa a raylib.
 * ------------------------------------------------------------------
 */

#ifndef CORRIDA_H
#define CORRIDA_H

#include "pista.h"
#include "carro.h"

#define TOTAL_RIVAIS   7
#define TOTAL_CARROS   (TOTAL_RIVAIS + 1)   /* rivais + jogador */

typedef struct {
    Pista *pista;
    Carro jogador;
    Carro rivais[TOTAL_RIVAIS];

    /* Ponteiros para os 8 carros, em ordem de corrida:
     * classificacao[0] é o 1º lugar. Reordenado a cada quadro
     * com Insertion Sort (ordenacao.c). */
    Carro *classificacao[TOTAL_CARROS];

    float tempoDeCorrida;   /* segundos desde a largada (não para no fim) */
    int terminou;           /* 1 depois que o jogador completa TOTAL_VOLTAS */
    int posicaoFinal;       /* posição do jogador na chegada (1 a 8) */
} Corrida;

void iniciarCorrida(Corrida *corrida, Pista *pista);
void atualizarCorrida(Corrida *corrida, Comandos comandosJogador, float tempoQuadro);
int voltaAtual(Carro *carro);
int posicaoDoCarro(Corrida *corrida, Carro *carro);
Carro *carroDaFrente(Corrida *corrida, Carro *carro);

#endif
