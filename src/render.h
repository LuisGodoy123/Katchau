/*
 * render.h
 * ------------------------------------------------------------------
 * Desenha a corrida em 3D, vista de trás do carro do jogador: o cenário
 * (mundo.c), os carros (carros3d.c) e a fumaça (fumaca.c). A câmera anda pela lista circular da pista.
 *
 * Este é um dos poucos módulos que chamam a raylib.
 * ------------------------------------------------------------------
 */

#ifndef RENDER_H
#define RENDER_H

#include "corrida.h"

void desenharVisaoDoJogador(Corrida *corrida);
void congelarCena(int pausada);
void desenharPistaDeFundo(Pista *pista);
void desenharCarroParado(int numero, float centroX, float baseY, float escala);

#endif
