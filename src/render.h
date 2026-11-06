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
#include "olhar.h"

#define TOTAL_DISTANCIAS_DA_CAMERA 3   /* perto, médio e longe */

void desenharVisaoDoJogador(Corrida *corrida);
void desenharCorridaVistaDe(Corrida *corrida, Camera3D camera);
Camera3D cameraDaVisaoDoJogador(Corrida *corrida);
int alternarDistanciaDaCamera(void);
void congelarCena(int pausada);
void definirOlhar(Olhar olhar);
void desenharPistaDeFundo(Pista *pista);
void desenharFundoAlternado(Pista *pistas[]);
void desenharCarroParado(int numero, float centroX, float baseY, float escala);

#endif
