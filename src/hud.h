/*
 * hud.h
 * ------------------------------------------------------------------
 * Informações por cima da corrida: volta, velocidade, tempo, o
 * mini-mapa com a posição dos carros e a tela de chegada. Versão simples da Fase 3; na Fase 5 entram o
 * velocímetro de verdade, a posição (5º/8) e o mini-mapa.
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
