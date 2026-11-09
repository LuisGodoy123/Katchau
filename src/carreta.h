/*
 * carreta.h
 * ------------------------------------------------------------------
 * O lado de dentro da carreta do Mack, que aparece quando a porta de trás
 * abre (passeio.c): o "camarim" do carro, no clima do filme. Paredes de
 * chapa escura com neon roxo, prateleiras com troféus e miniaturas,
 * quadros, um teto estrelado com spots e um telão no fundo.
 *
 * Tudo é montado aqui mesmo: a geometria com a rlgl e as imagens (os
 * quadros, o piso, a chapa, o teto e o telão) desenhadas ao carregar.
 * Módulo de desenho: usa a raylib.
 * ------------------------------------------------------------------
 */

#ifndef CARRETA_H
#define CARRETA_H

#include "raylib.h"

void carregarInteriorDaCarreta(void);
void descarregarInteriorDaCarreta(void);

/* Desenha dentro de BeginMode3D. A matriz leva o lado de dentro (x de um lado ao outro, com 0 no
 * meio; y para cima, com 0 no piso; z para dentro, com 0 na porta) até o mundo */
void desenharInteriorDaCarreta(Matrix deDentro, float largura, float altura, float fundo);

/* As caixas das rodas da carreta (os pneus sobem acima do piso): blocos de chapa escura com um friso de
 * neon, de "de" até "ate" (os dois cantos opostos), nas mesmas medidas do lado de dentro */
void desenharCaixasDeRoda(Matrix deDentro, const Vector3 *de, const Vector3 *ate, int total);

#endif
