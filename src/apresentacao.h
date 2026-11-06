/*
 * apresentacao.h
 * ------------------------------------------------------------------
 * A apresentação da largada, como numa transmissão (Forza Horizon, The Crew):
 * antes da contagem, um jogo de câmeras mostra a pista e os carros no grid.
 *
 *   1) PISTA    um drone vem voando alto sobre a pista até o grid, com o nome
 *               do circuito, o nível e as voltas;
 *   2) GRID     uma grua passa por cima do grid, de frente para os carros;
 *   3) PILOTOS  a câmera atravessa o grid rente ao chão, entre as duas filas,
 *               de frente para os carros, e o nome de cada um aparece quando
 *               ela passa por ele;
 *   4) JOGADOR  a câmera gira em volta do carro do jogador ("SEU CARRO");
 *   5) PRONTO   a câmera desliza para trás do carro, onde a corrida começa.
 *
 * Durante a apresentação a corrida fica parada (a contagem só começa depois) e
 * o jogador pode pular com ENTER ou o botão de confirmar. Cada tomada tem um
 * número (TomadaDaApresentacao), para as falas de cada momento.
 *
 * Módulo que usa a raylib.
 * ------------------------------------------------------------------
 */

#ifndef APRESENTACAO_H
#define APRESENTACAO_H

#include "corrida.h"

typedef enum {
    TOMADA_PISTA,
    TOMADA_GRID,
    TOMADA_PILOTOS,
    TOMADA_JOGADOR,
    TOMADA_PRONTO,
    TOTAL_TOMADAS
} TomadaDaApresentacao;

typedef struct {
    int ativa;                 /* 1 enquanto a apresentação está passando */
    TomadaDaApresentacao tomada;
    float tempoNaTomada;       /* segundos desde que a tomada atual começou */
    int tomadaNova;            /* 1 no quadro em que uma tomada começa (para as falas) */
    float pulando;             /* > 0: o jogador pulou; a tela escurece e a apresentação acaba */
} Apresentacao;

void iniciarApresentacao(Apresentacao *apresentacao);
void atualizarApresentacao(Apresentacao *apresentacao, int pular, float tempoQuadro);
int apresentacaoAtiva(const Apresentacao *apresentacao);
void desenharApresentacao(const Apresentacao *apresentacao, Corrida *corrida);

#endif
