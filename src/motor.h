/*
 * motor.h
 * ------------------------------------------------------------------
 * O motor do carro do jogador, só a parte que o som precisa: em que
 * rotação ele está, qual marcha e se o piloto está com o pé no fundo.
 *
 * O jogo não simula câmbio de verdade (o carro tem só velocidade). Aqui a
 * rotação é deduzida da velocidade: cada marcha cobre uma faixa de
 * velocidade, a rotação sobe até perto do limite, o câmbio troca e a
 * rotação cai, como num carro de verdade. Cada carro usa a própria
 * velocidade máxima como fim da última marcha.
 *
 * Este módulo NÃO usa a raylib: o audio.c transforma estes números em som.
 * ------------------------------------------------------------------
 */

#ifndef MOTOR_H
#define MOTOR_H

#include "carro.h"

#define TOTAL_MARCHAS 6

typedef struct {
    int marcha;            /* 0 a TOTAL_MARCHAS - 1 */
    float giro;            /* rotação: 0 = o mínimo da marcha lenta, 1 = limitador */
    float carga;           /* 0 = aliviado / freando, 1 = pé no fundo (com folga na troca) */
    float tempoDeTroca;    /* segundos que faltam para acabar a troca de marcha (0 = engatado) */
} Motor;

void iniciarMotor(Motor *motor);
void atualizarMotor(Motor *motor, const Carro *carro, float tempoQuadro);

#endif
