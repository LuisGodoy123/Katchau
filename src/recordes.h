/*
 * recordes.h
 * ------------------------------------------------------------------
 * O recorde de volta de cada pista: o menor tempo de volta que um jogador
 * já fez nela e com qual carro. Fica gravado no arquivo recordes.txt (uma
 * linha por pista) e aparece no cartão de informações da escolha de pista.
 *
 * Este módulo NÃO usa a raylib.
 * ------------------------------------------------------------------
 */

#ifndef RECORDES_H
#define RECORDES_H

#include "pista.h"

#define ARQUIVO_DE_RECORDES "recordes.txt"
#define TAMANHO_DO_NOME_DO_RECORDE 32

void carregarRecordes(void);
float recordeDaPista(IdPista id, char *dono, int tamanhoDoDono);
int registrarRecorde(IdPista id, float tempo, const char *dono);

#endif
