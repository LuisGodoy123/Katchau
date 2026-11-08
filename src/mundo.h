/*
 * mundo.h
 * ------------------------------------------------------------------
 * O cenário 3D de cada pista, desenhado com a raylib em 3D de verdade:
 *
 *   - Copa Pistão: o modelo do estádio (recursos/modelos/copa_pistao.glb);
 *   - Interlagos, Oval e Splat: o modelo 3D de cada uma, de dia, com o que o
 *     jogo põe em cima (telões, torcida, vagas do pit e objetos dos mapas).
 *
 * A lógica do jogo continua na lista circular da pista. Este módulo só
 * carrega os modelos e monta, uma vez, no começo, o que vai em cima deles.
 * ------------------------------------------------------------------
 */

#ifndef MUNDO_H
#define MUNDO_H

#include "raylib.h"
#include "pista.h"

void carregarMundo(Pista *pistas[]);
void descarregarMundo(void);
void desenharCenario(IdPista id, Vector3 camera);
void prepararPlanosDaCamera(void);
void desenharTelasQueBrilham(IdPista id);
void prepararSombras(IdPista id, Camera3D camera, int ligadas);

#endif
