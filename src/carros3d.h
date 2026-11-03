/*
 * carros3d.h
 * ------------------------------------------------------------------
 * Os modelos 3D dos carros (recursos/modelos/carros). Cada arquivo vem com
 * a sua escala, a sua origem e o seu eixo: ao carregar, este módulo mede
 * cada um e deixa todos iguais, com o tamanho de verdade em metros, o
 * centro na origem, as rodas no chão e a frente para +z.
 *
 * Também prepara uma miniatura de cada carro, para a garagem e o menu.
 * ------------------------------------------------------------------
 */

#ifndef CARROS3D_H
#define CARROS3D_H

#include "raylib.h"

void carregarCarros3D(void);
void descarregarCarros3D(void);
float larguraDoCarro3D(int numero);
float comprimentoDoCarro3D(int numero);
void desenharCarro3D(int numero, Matrix noMundo);
void desenharMiniatura(int numero, float centroX, float baseY, float larguraNaTela);

#endif
