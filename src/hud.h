/*
 * hud.h
 * ------------------------------------------------------------------
 * Informações por cima da corrida: volta, velocidade e tempo, e a
 * tela de chegada. Versão simples da Fase 3; na Fase 5 entram o
 * velocímetro de verdade, a posição (5º/8) e o mini-mapa.
 * ------------------------------------------------------------------
 */

#ifndef HUD_H
#define HUD_H

#include "corrida.h"

void desenharHud(Corrida *corrida);
void desenharFimDeCorrida(Corrida *corrida);

#endif
