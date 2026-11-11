/*
 * tratores.h
 * ------------------------------------------------------------------
 * Os tratores-vaca dos pastos em volta de Radiator Springs, como no filme:
 * de dia pastam (parados, piscando); de noite dormem, de olhos fechados.
 * Dormindo, se um carro parado perto deles ronca o motor (o freio de mão
 * puxado e o pé no acelerador), eles acordam assustados, dão um pulinho e
 * tombam de costas, uma cambalhota para trás, mugindo e soltando fumaça
 * pelo escapamento; depois de um tempo, desviram e voltam a dormir. Um
 * susto pega o rebanho todo em volta, um depois do outro.
 *
 * O modelo (recursos/modelos/trator/: corpo, olhos, rodas de trás e da
 * frente, e trator.json com as medidas) e os lugares (recursos/modelos/
 * tratores.json) saem de ferramentas/preparar_trator.py.
 * ------------------------------------------------------------------
 */

#ifndef TRATORES_H
#define TRATORES_H

#include "raylib.h"

void carregarTratores(void);
void descarregarTratores(void);

/* Um passo. noite: 0 de dia, 1 de noite (eles dormem); jogador: onde está o carro; roncando: 0 a 1, o quanto
 * o motor dele está roncando parado (CarroLivre.roncando) */
void atualizarTratores(float noite, Vector3 jogador, float roncando, float tempoQuadro);

/* Dentro do BeginMode3D */
void desenharTratores(Camera3D camera);

/* Para o carro não atravessar: a caixa de cada um agora (o meio no chão, o rumo, as meias medidas e o alto).
 * Devolve 0 se não há o trator i */
int totalDeTratores(void);
int caixaDoTrator(int i, float *x, float *z, float *rumo, float *meioComprimento, float *meiaLargura, float *alto);

#endif
