/*
 * hud.h
 * ------------------------------------------------------------------
 * Informações por cima da corrida, nos quatro cantos da tela:
 *   - em cima à esquerda: a tabela de posições (bandeira, tempo de prova,
 *     intervalo de cada carro para o jogador e a última volta);
 *   - em cima à direita: a volta do jogador (diferença para a melhor
 *     volta, tempos e os 3 setores coloridos, como no F1);
 *   - embaixo à esquerda: o mini-mapa; embaixo à direita: o velocímetro.
 * E também a contagem regressiva e o painel de chegada.
 * ------------------------------------------------------------------
 */

#ifndef HUD_H
#define HUD_H

#include "corrida.h"

void desenharHud(Corrida *corrida);
void desenharFimDeCorrida(Corrida *corrida);
void formatarTempo(float segundos, char *texto, int tamanho);

/* Tela virtual de 800x600 para o HUD e os menus (veja hud.c) */
void comecarInterface(int centralizada);
void terminarInterface(void);
int larguraVirtual(void);
int alturaVirtual(void);

#endif
