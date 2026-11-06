/*
 * ia.h
 * ------------------------------------------------------------------
 * Pilotos controlados pelo computador.
 *
 * A IA não mexe no carro diretamente: ela olha a pista e o carro da
 * frente e devolve os mesmos Comandos que o jogador gera com o
 * teclado (acelerar, frear, virar). Quem move o carro é carro.c.
 *
 * Este módulo NÃO usa a raylib.
 * ------------------------------------------------------------------
 */

#ifndef IA_H
#define IA_H

#include "carro.h"

/* A defesa: segura a linha por isso, e só defende de novo depois disso (corrida.c conta o tempo) */
#define SEGUNDOS_DE_DEFESA    1.6f
#define ESPERA_PARA_DEFENDER  3.5f

Comandos atualizarIA(Carro *carro, const Pista *pista);

#endif
