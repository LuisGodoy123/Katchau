/*
 * motor.c
 * ------------------------------------------------------------------
 * Rotação, marcha e carga do motor de cada carro. Veja motor.h.
 * ------------------------------------------------------------------
 */

#include "motor.h"

/* Até que fração da velocidade máxima cada marcha vai, com o motor no limite. Cada
 * marcha é bem mais longa que a anterior: depois de trocar, a rotação cai para uns
 * 6.000 a 6.600 rpm, longe do corte, e o motor tem por onde subir. A última passa da
 * velocidade máxima: o carro chega lá abaixo da linha vermelha, com folga para o nitro. */
static const float TETO_DA_MARCHA[TOTAL_MARCHAS] = { 0.155f, 0.235f, 0.345f, 0.486f, 0.666f, 0.90f, 1.20f };

#define GIRO_MARCHA_LENTA   0.12f   /* rotação parado, sem acelerar */
#define GIRO_ARRANCADA      0.55f   /* rotação com o pé no fundo parado ou saindo na 1ª */
#define GIRO_PARA_SUBIR     (RPM_DA_TROCA / RPM_MAXIMO)   /* na linha vermelha o câmbio engata a marcha seguinte */
#define GIRO_DA_REDUZIDA_FREANDO  0.80f   /* freando: reduz cedo, se a marcha de baixo ficar abaixo disso, para sair da curva já com giro */
#define GIRO_DA_REDUZIDA_ALIVIANDO 0.60f  /* aliviando: só quando o motor já estaria baixo */
#define GIRO_FRACO                0.52f   /* acelerando com o motor abaixo disso: falta força... */
#define GIRO_DA_RETOMADA          0.84f   /* ...e reduz, se a marcha de baixo não passar disso (kickdown) */
#define SEGUNDOS_ENTRE_TROCAS     0.28f   /* o câmbio não troca de novo antes disso */
#define SEGUNDOS_ANTES_DA_RETOMADA 0.70f  /* depois de subir, espera mais para poder reduzir acelerando */
#define DURACAO_DA_TROCA    0.16f   /* segundos de pé levantado durante a troca */
#define DURACAO_DA_REDUZIDA 0.10f   /* uma reduzida é mais rápida */
#define MAIOR_FRACAO        1.20f   /* com vácuo ou nitro, o carro passa da própria máxima */
#define CARGA_DA_PUNTA_TACO 0.55f   /* a acelerada que o motor dá ao reduzir */
#define SUBIDA_DO_GIRO      10.0f   /* quão rápido a rotação sobe (1/s) */
#define SUBIDA_NA_REDUZIDA  18.0f   /* na reduzida, a rotação pula para a marcha de baixo */
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
    motor->tempoNaMarcha = 0.0f;
    motor->reduzindo = 0;
}

/*
 * deveReduzir
 * A reduzida depende do que o piloto está fazendo:
 *   - freando, reduz cedo, para já sair da curva com o motor forte;
 *   - aliviando, deixa a marcha longa até o motor ficar baixo;
 *   - acelerando com o motor sem força, reduz para retomar (kickdown).
 * Recebe: o motor, a velocidade como fração da máxima e o que o piloto está fazendo.
 * Devolve: 1 se é hora de reduzir.
 */
static int deveReduzir(const Motor *motor, float fracao, int pedeAcelerar, int freando) {
    if (motor->marcha == 0) {
        return 0;
    }
    float naMarchaDeBaixo = giroNaMarcha(motor->marcha - 1, fracao);
    if (freando) {
        return naMarchaDeBaixo < GIRO_DA_REDUZIDA_FREANDO;
    }
    if (pedeAcelerar) {
        return motor->tempoNaMarcha > SEGUNDOS_ANTES_DA_RETOMADA && motor->giro < GIRO_FRACO
               && naMarchaDeBaixo < GIRO_DA_RETOMADA;
    }
    return naMarchaDeBaixo < GIRO_DA_REDUZIDA_ALIVIANDO;
}

/*
 * atualizarMotor
 * Um quadro do motor: escolhe a marcha, leva a rotação para onde a marcha manda (com
 * inércia, então ela cai de verdade quando o câmbio troca) e acompanha o pé do piloto.
 * Recebe: o motor, a velocidade como fração da máxima do carro, se o piloto está
 *         acelerando, se está freando, se a marcha deve ficar onde está (o carro de lado,
 *         com a roda girando mais que o chão) e o tempo do quadro.
 * Devolve: nada.
 */
void atualizarMotor(Motor *motor, float fracaoDaVelocidade, int pedeAcelerar, int freando, int segurarMarcha, float tempoQuadro) {
    float fracao = fracaoDaVelocidade;
    if (fracao < 0.0f) {
        fracao = 0.0f;
    }
    if (fracao > MAIOR_FRACAO) {
        fracao = MAIOR_FRACAO;
    }
    motor->tempoNaMarcha += tempoQuadro;

    /* Câmbio: sobe quando o ponteiro chega na linha vermelha (a rotação de verdade, com
     * inércia, e não a que a marcha pede), só com o pé no fundo: aliviando, a marcha não sobe */
    if (motor->tempoDeTroca > 0.0f) {
        motor->tempoDeTroca -= tempoQuadro;
        if (motor->tempoDeTroca <= 0.0f) {
            motor->tempoDeTroca = 0.0f;
            motor->reduzindo = 0;
        }
    } else if (!segurarMarcha && motor->tempoNaMarcha > SEGUNDOS_ENTRE_TROCAS) {
        int podeSubir = pedeAcelerar || giroNaMarcha(motor->marcha, fracao) >= 1.0f;
        if (podeSubir && motor->marcha < TOTAL_MARCHAS - 1 && motor->giro >= GIRO_PARA_SUBIR
            && giroNaMarcha(motor->marcha, fracao) >= GIRO_PARA_SUBIR) {
            motor->marcha++;
            motor->tempoDeTroca = DURACAO_DA_TROCA;
            motor->tempoNaMarcha = 0.0f;
        } else if (deveReduzir(motor, fracao, pedeAcelerar, freando)) {
            motor->marcha--;
            /* Retomando, pula direto para a marcha que tem força, como um kickdown */
            while (pedeAcelerar && motor->marcha > 0 && giroNaMarcha(motor->marcha - 1, fracao) < GIRO_DA_RETOMADA) {
                motor->marcha--;
            }
            motor->tempoDeTroca = DURACAO_DA_REDUZIDA;
            motor->tempoNaMarcha = 0.0f;
            motor->reduzindo = 1;
        }
    }

    /* Rotação */
    float alvo = giroNaMarcha(motor->marcha, fracao);
    if (motor->marcha == 0 && pedeAcelerar && !freando && alvo < GIRO_ARRANCADA) {
        alvo = GIRO_ARRANCADA;   /* saindo da largada: o motor sobe antes do carro */
    }
    float rapidez = motor->reduzindo ? SUBIDA_NA_REDUZIDA : SUBIDA_DO_GIRO;
    if (alvo < motor->giro) {
        rapidez = (motor->tempoDeTroca > 0.0f) ? QUEDA_NA_TROCA : QUEDA_DO_GIRO;
    }
    motor->giro = aproximar(motor->giro, alvo, rapidez, tempoQuadro);

    /* Carga: pé no fundo, menos durante a troca */
    float alvoCarga = (pedeAcelerar && motor->tempoDeTroca <= 0.0f) ? 1.0f : 0.0f;
    if (motor->reduzindo) {
        alvoCarga = CARGA_DA_PUNTA_TACO;   /* a rotação sobe com uma acelerada, como um piloto faz */
    }
    float rapidezCarga = (alvoCarga > motor->carga) ? SUBIDA_DA_CARGA : QUEDA_DA_CARGA;
    motor->carga = aproximar(motor->carga, alvoCarga, rapidezCarga, tempoQuadro);
}

/*
 * estaTrocandoDeMarcha
 * Recebe: o motor.
 * Devolve: 1 durante a troca de marcha (a potência está cortada), 0 engatado.
 */
int estaTrocandoDeMarcha(const Motor *motor) {
    return motor->tempoDeTroca > 0.0f;
}
