/*
 * carreta.h
 * ------------------------------------------------------------------
 * O lado de dentro da carreta do Mack, que aparece quando a porta de trás
 * abre (passeio.c): o "camarim" do McQueen, no clima do filme. Piso de
 * chapa xadrez, paredes de couro capitonê com neon roxo, prateleiras de
 * troféus da Copa Pistão, pôsteres, teto com spots, e no fundo o telão,
 * as vitrines e o letreiro; nas laterais, as caixas das rodas.
 *
 * É o modelo recursos/modelos/mack_interior.glb, feito no Blender por
 * ferramentas/montar_carreta_do_mack.py, com a luz assada nas cores dos
 * vértices: desenhado com o shader padrão da raylib (textura × cor do
 * vértice). Módulo de desenho: usa a raylib.
 * ------------------------------------------------------------------
 */

#ifndef CARRETA_H
#define CARRETA_H

#include "raylib.h"

void carregarInteriorDaCarreta(void);
void descarregarInteriorDaCarreta(void);

/* Desenha dentro de BeginMode3D. A matriz leva o lado de dentro (x de um lado ao outro, com 0 no
 * meio; y para cima, com 0 no piso; z para dentro, com 0 na porta), que é onde o modelo foi feito,
 * até o mundo */
void desenharInteriorDaCarreta(Matrix deDentro);

#endif
