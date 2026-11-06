/*
 * torcida3d.h
 * ------------------------------------------------------------------
 * A torcida 3D: carrinhos de verdade nos lugares das arquibancadas perto da
 * câmera, em todas as pistas. Mais longe continua a torcida pintada.
 * ------------------------------------------------------------------
 */

#ifndef TORCIDA3D_H
#define TORCIDA3D_H

#include "raylib.h"

void adicionarLugarTorcida3D(int pista, float x, float y, float z, float giro);
void definirTamanhoTorcida3D(int pista, float tamanho);
void carregarTorcidaDoEstadio(Model estadio, int materialDosAssentos, float escala);
void iniciarTorcida3D(void);
void prepararTorcida3D(int pista, Camera3D camera);
void desenharTorcida3D(void);
void descarregarTorcida3D(void);

#endif
