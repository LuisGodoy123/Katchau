/*
 * motor.c
 * ------------------------------------------------------------------
 * Rotação, marcha e carga do motor do jogador. Veja motor.h.
 * ------------------------------------------------------------------
 */

#include "motor.h"

/* Até que fração da velocidade máxima cada marcha vai, com o motor no limite */
static const float TETO_DA_MARCHA[TOTAL_MARCHAS] = { 0.22f, 0.38f, 0.54f, 0.70f, 0.85f, 1.00f };

#define GIRO_MARCHA_LENTA   0.12f   /* rotação parado, sem acelerar */
#define GIRO_ARRANCADA      0.55f   /* rotação com o pé no fundo parado ou saindo na 1ª */
#define GIRO_PARA_SUBIR     0.96f   /* a esta rotação o câmbio engata a marcha seguinte */
#define GIRO_PARA_REDUZIR   0.72f   /* abaixo desta (na marcha de baixo) reduz */
#define DURACAO_DA_TROCA    0.16f   /* segundos de pé levantado durante a troca */
#define SUBIDA_DO_GIRO      10.0f   /* quão rápido a rotação sobe (1/s) */
#define QUEDA_DO_GIRO        6.0f   /* ...e desce, aliviando */
#define QUEDA_NA_TROCA      16.0f   /* ...e desce, durante a troca */
#define SUBIDA_DA_CARGA    14.0f
#define QUEDA_DA_CARGA      9.0f

/*
 * giroNaMarcha
 * Rotação que o motor teria em uma marcha a esta velocidade.
 * Recebe: a marcha e a velocidade como fração da máxima do carro (0 a 1).
 * Devolve: a rotação, de GIRO_MARCHA_LENTA a 1.
 */
static float giroNaMarcha(int marcha, float fracaoDaVelocidade) {
    float giro = GIRO_MARCHA_LENTA + (1.0f - GIRO_MARCHA_LENTA) * fracaoDaVelocidade / TETO_DA_MARCHA[marcha];
    return giro > 1.0f ? 1.0f : giro;
}

/*
 * aproximar
 * Anda de "atual" em direção a "alvo" suavemente (inércia do motor).
 * Recebe: o valor atual, o alvo, a rapidez (1/s) e o tempo do quadro.
 * Devolve: o novo valor.
 */
static float aproximar(float atual, float alvo, float rapidez, float tempoQuadro) {
    float passo = rapidez * tempoQuadro;
    if (passo > 1.0f) {
        passo = 1.0f;
    }
    return atual + (alvo - atual) * passo;
}

/*
 * iniciarMotor
 * Motor parado em marcha lenta, na 1ª.
 * Recebe: o motor.
 * Devolve: nada.
 */
void iniciarMotor(Motor *motor) {
    motor->marcha = 0;
    motor->giro = GIRO_MARCHA_LENTA;
    motor->carga = 0.0f;
    motor->tempoDeTroca = 0.0f;
}

/*
 * atualizarMotor
 * Um quadro do motor: escolhe a marcha pela velocidade, leva a rotação
 * para onde a marcha manda (com inércia, então ela cai de verdade quando o
 * câmbio troca) e acompanha o pé do piloto. Quem freia ou solta o
 * acelerador alivia a carga.
 * Recebe: o motor, o carro do jogador e o tempo do quadro.
 * Devolve: nada.
 */
void atualizarMotor(Motor *motor, const Carro *carro, float tempoQuadro) {
    float fracao = carro->velocidade / carro->velocidadeMaxima;
    if (fracao < 0.0f) {
        fracao = 0.0f;
    }
    if (fracao > 1.0f) {
        fracao = 1.0f;
    }

    /* Câmbio: sobe ao chegar perto do limite, reduz quando a rotação da marcha de baixo seria boa */
    if (motor->tempoDeTroca > 0.0f) {
        motor->tempoDeTroca -= tempoQuadro;
        if (motor->tempoDeTroca < 0.0f) {
            motor->tempoDeTroca = 0.0f;
        }
    } else if (motor->marcha < TOTAL_MARCHAS - 1 && giroNaMarcha(motor->marcha, fracao) >= GIRO_PARA_SUBIR) {
        motor->marcha++;
        motor->tempoDeTroca = DURACAO_DA_TROCA;
    } else if (motor->marcha > 0 && giroNaMarcha(motor->marcha - 1, fracao) < GIRO_PARA_REDUZIR) {
        motor->marcha--;
        motor->tempoDeTroca = DURACAO_DA_TROCA;
    }

    /* Rotação */
    int pedeAcelerar = carro->ultimosComandos.acelerar && !carro->ultimosComandos.frear;
    float alvo = giroNaMarcha(motor->marcha, fracao);
    if (motor->marcha == 0 && pedeAcelerar && alvo < GIRO_ARRANCADA) {
        alvo = GIRO_ARRANCADA;   /* saindo da largada: o motor sobe antes do carro */
    }
    float rapidez = SUBIDA_DO_GIRO;
    if (alvo < motor->giro) {
        rapidez = (motor->tempoDeTroca > 0.0f) ? QUEDA_NA_TROCA : QUEDA_DO_GIRO;
    }
    motor->giro = aproximar(motor->giro, alvo, rapidez, tempoQuadro);

    /* Carga: pé no fundo, menos durante a troca */
    float alvoCarga = (pedeAcelerar && motor->tempoDeTroca <= 0.0f) ? 1.0f : 0.0f;
    float rapidezCarga = (alvoCarga > motor->carga) ? SUBIDA_DA_CARGA : QUEDA_DA_CARGA;
    motor->carga = aproximar(motor->carga, alvoCarga, rapidezCarga, tempoQuadro);
}
