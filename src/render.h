/*
 * render.h
 * ------------------------------------------------------------------
 * Desenha a estrada em pseudo-3D (visão de trás, estilo 16 bits) e
 * os carros sobre ela.
 *
 * A ideia em uma frase: a partir do segmento onde a câmera está,
 * pegamos os próximos DISTANCIA_VISIVEL nós da lista circular,
 * calculamos onde cada um aparece na tela (quanto mais longe, mais
 * alto e mais estreito) e desenhamos do mais longe para o mais perto.
 *
 * Este é um dos poucos módulos que chamam a raylib.
 * ------------------------------------------------------------------
 */

#ifndef RENDER_H
#define RENDER_H

#include "corrida.h"

void carregarTexturas(void);
void descarregarTexturas(void);
void desenharVisaoDoJogador(Corrida *corrida);

#endif
