/*
 * interiores.h
 * ------------------------------------------------------------------
 * O lado de dentro dos prédios do mundo aberto em que o carro entra
 * dirigindo, pelas portas de verdade (sem teleporte): a Casa Della Tires
 * (a loja do Luigi e do Guido), os quartos dos cones e a recepção do
 * Cozy Cone (a da Sally).
 *
 * Cada um é um modelo nas coordenadas do mundo (recursos/modelos/
 * loja_do_luigi.glb, cozy_cone_quartos.glb e cozy_cone_recepcao.glb),
 * feito no Blender por uma ferramenta (ferramentas/montar_loja_do_luigi.py,
 * montar_quartos_do_cozy_cone.py e montar_recepcao_do_cozy_cone.py). O mapa
 * (ferramentas/montar_mundo_aberto.py) tira dos prédios as portas e os
 * vidros pintados e põe a colisão do lado de dentro no .col.
 *
 * A luz vem assada nas cores dos vértices (textura × cor do vértice). Com
 * a versão de noite (<interior>_noite.glb, gerada pelas mesmas ferramentas
 * com --noite), as duas luzes se misturam pela hora do dia.
 * Módulo de desenho: usa a raylib.
 * ------------------------------------------------------------------
 */

#ifndef INTERIORES_H
#define INTERIORES_H

#include "raylib.h"

void carregarInteriores(void);     /* os que existem em recursos/modelos/ (falta um, ele não aparece) */
void descarregarInteriores(void);

/* Desenha (dentro de BeginMode3D) os que estão perto da câmera */
/* luz: multiplica a luz assada, quando o interior não tem a versão de noite; noite: 0 a 1, a mistura da luz
 * de dia com a de noite (<interior>_noite.glb) */
void desenharInteriores(Vector3 camera, Color luz, float noite);

#endif
