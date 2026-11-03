/*
 * mundo.h
 * ------------------------------------------------------------------
 * O cenário 3D de cada pista, desenhado com a raylib em 3D de verdade:
 *
 *   - Copa Pistão: o modelo do estádio (recursos/modelos/copa_pistao.glb);
 *   - Interlagos: malhas geradas a partir da lista de segmentos (asfalto,
 *     zebras, terreno, árvores, pneus, placas e pórtico), com o céu, a
 *     cidade e os morros em volta.
 *
 * A lógica do jogo continua na lista circular da pista. Este módulo só
 * transforma os segmentos em malhas, uma vez, no começo.
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

#endif
