/*
 * linha_de_corrida.h
 * ------------------------------------------------------------------
 * A linha de corrida ideal de cada pista, para o traçado no asfalto (render.c):
 * por onde passar (abrindo antes da curva, tangenciando por dentro e abrindo de
 * novo na saída) e o raio dela em cada ponto, de onde sai a velocidade da curva.
 * Calculada uma vez por pista, na primeira vez que alguém pede, e guardada nela
 * (Pista.linhaDeCorrida, liberada com a pista).
 *
 * Não usa a raylib (regra 2 do roadmap).
 * ------------------------------------------------------------------
 */

#ifndef LINHA_DE_CORRIDA_H
#define LINHA_DE_CORRIDA_H

#include "pista.h"

/* Onde a linha passa: metros da linha da pista (positivo à direita) */
float lateralDaLinhaDeCorrida(Pista *pista, PosicaoNaPista lugar);

/* O raio da linha ali, em metros (muito grande na reta) */
float raioDaLinhaDeCorrida(Pista *pista, PosicaoNaPista lugar);

#endif
