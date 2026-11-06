/*
 * faiscas.h
 * ------------------------------------------------------------------
 * As faíscas de metal das batidas, como nos jogos de corrida (NASCAR, Forza,
 * The Crew): quando um carro bate em outro ou raspa no muro, sai uma rajada
 * de faíscas no ponto do contato, com um clarão; enquanto as latas continuam
 * se esfregando, sai uma chuva de faíscas que fica para trás do carro.
 *
 * Cada faísca é um risco curto e brilhante (amarelo-branco no começo,
 * laranja e vermelho no fim) que voa, cai com a gravidade, quica no chão e
 * apaga. Valem para todos os carros: o contato vem de carro.c e corrida.c.
 * ------------------------------------------------------------------
 */

#ifndef FAISCAS_H
#define FAISCAS_H

#include "raylib.h"
#include "corrida.h"

void limparFaiscas(void);
void soltarFaiscasDaCorrida(Corrida *corrida, float tempoQuadro);
void atualizarFaiscas(float tempoQuadro);
void desenharFaiscas(Camera3D camera);

#endif
