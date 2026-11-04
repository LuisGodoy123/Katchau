/*
 * paisagem.h
 * ------------------------------------------------------------------
 * O que fica em volta de Interlagos, em 3D de verdade: o muro do autódromo,
 * a avenida, os bairros, os prédios, as matas e os morros até o horizonte.
 * Só o céu continua sendo uma imagem.
 * ------------------------------------------------------------------
 */

#ifndef PAISAGEM_H
#define PAISAGEM_H

#include "raylib.h"
#include "pista.h"

typedef float (*AlturaNoMundo)(Pista *pista, float x, float z);

void prepararPaisagem(Pista *pista);
float relevoDaPaisagem(float x, float z);
int foraDoAutodromo(float x, float z);
int dentroDoLago(float x, float z);
void lugarDoLago(float *x, float *z);
void definirNivelDoLago(float nivel);
float alturaComLago(float altura, float x, float z);
Color corDaGramaNoAutodromo(float x, float z);
void adicionarChaoDaCidade(Vector3 a, Vector3 b, Vector3 d, Vector3 e);
void construirPaisagem(Pista *pista, AlturaNoMundo altura, float minX, float maxX, float minZ, float maxZ);
void desenharPaisagem(void);
void descarregarPaisagem(void);

#endif
