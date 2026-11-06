/*
 * marcas.h
 * ------------------------------------------------------------------
 * As marcas de pneu no asfalto: quando o pneu escorrega (drift, derrapagem,
 * arrancada, freada forte, rodada), cada roda deixa um rastro escuro que
 * some aos poucos.
 * ------------------------------------------------------------------
 */

#ifndef MARCAS_H
#define MARCAS_H

#include "raylib.h"

void carregarMarcas(void);
void descarregarMarcas(void);
void deixarMarca(int carro, int roda, Vector3 ponto, Vector3 frente, float forca);
void soltarRoda(int carro, int roda);
void atualizarMarcas(float tempoQuadro);
void desenharMarcas(void);
void limparMarcas(void);

#endif
