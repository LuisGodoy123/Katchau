/*
 * fogos.h
 * ------------------------------------------------------------------
 * Os fogos de artifício da vitória, como na festa de chegada dos jogos de corrida:
 * foguetes sobem deixando um rastro de brasas e estouram no céu em peônias (esferas),
 * salgueiros (chuva dourada que cai devagar) e anéis, em várias cores, com um clarão
 * e o estrondo de cada um.
 *
 * Os fogos nascem sempre na frente da câmera da vitória (a câmera manda onde a festa
 * está), então aparecem em qualquer tomada. Cada brasa é um risco brilhante, somado à
 * imagem (mistura aditiva), com largura que cresce com a distância para ser vista de longe.
 *
 * Módulo que usa a raylib.
 * ------------------------------------------------------------------
 */

#ifndef FOGOS_H
#define FOGOS_H

#include "raylib.h"

void limparFogos(void);
void soltarFogosDaVitoria(Camera3D camera, Vector3 velocidadeDaCamera, float alturaDoChao, int cortou, float tempoQuadro);
void atualizarFogos(float tempoQuadro);
void desenharFogos(Camera3D camera);

#endif
