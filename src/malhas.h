/*
 * malhas.h
 * ------------------------------------------------------------------
 * Cortar uma malha em duas pelas ilhas dela (os pedaços de triângulos
 * ligados, contando como ligados os vértices no mesmo lugar). Quem chama
 * diz, pela caixa de cada ilha, para que lado ela vai. Serve para separar
 * a cabine do Mack da carreta, no corpo e nas lâmpadas, sem refazer os
 * modelos: assim a cabine (a cabeça dele) pode se mexer sozinha.
 * ------------------------------------------------------------------
 */

#ifndef MALHAS_H
#define MALHAS_H

#include "raylib.h"

/* Para que lado vai uma ilha, pela caixa dela: 1 vai para a segunda malha */
typedef int (*EscolhaDaIlha)(Vector3 minimo, Vector3 maximo, void *dados);

/* Corta a malha em duas (as duas já vão para a placa de vídeo). Devolve 0 se um lado ficaria vazio */
int separarMalhaPorIlhas(Mesh malha, EscolhaDaIlha escolha, void *dados, Mesh *primeira, Mesh *segunda);
/* Corta a malha "qual" do modelo: a primeira parte fica no lugar dela e a segunda vai para o fim, com o
 * mesmo material. Devolve o número da malha nova (-1 se não cortou) */
int separarModeloPorIlhas(Model *modelo, int qual, EscolhaDaIlha escolha, void *dados);

#endif
