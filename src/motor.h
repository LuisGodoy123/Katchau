/*
 * motor.h
 * ------------------------------------------------------------------
 * O motor e o câmbio automático de cada carro: em que rotação ele está,
 * qual marcha e se o piloto está com o pé no fundo.
 *
 * A rotação é deduzida da velocidade: cada marcha cobre uma faixa de
 * velocidade, a rotação sobe até a linha vermelha, o câmbio troca e a
 * rotação cai, como num carro de verdade. Cada carro usa a própria
 * velocidade máxima como fim da última marcha.
 *
 * Todo carro tem um motor (carro.h). Durante a troca de marcha o carro.c
 * corta a potência por um instante; o audio.c transforma a rotação e a
 * carga do motor do jogador em som, e o hud.c mostra o conta-giros.
 *
 * Este módulo NÃO usa a raylib.
 * ------------------------------------------------------------------
 */

#ifndef MOTOR_H
#define MOTOR_H


#define TOTAL_MARCHAS 7
#define RPM_MAXIMO    9000.0f   /* giro 1 = 9 mil rpm (o fim do conta-giros) */
#define RPM_DA_TROCA  8000.0f   /* a linha vermelha: o câmbio sobe a marcha aqui */

typedef struct {
    int marcha;            /* 0 a TOTAL_MARCHAS - 1 */
    float giro;            /* rotação: 0 a 1 (1 = RPM_MAXIMO, o limitador) */
    float carga;           /* 0 = aliviado / freando, 1 = pé no fundo (com folga na troca) */
    float tempoNaMarcha;   /* segundos desde a última troca: evita trocar sem parar */
    int reduzindo;         /* 1 durante uma reduzida: o motor dá a acelerada de punta-taco */
    float tempoDeTroca;    /* segundos que faltam para acabar a troca de marcha (0 = engatado) */
} Motor;

void iniciarMotor(Motor *motor);
void atualizarMotor(Motor *motor, float fracaoDaVelocidade, int pedeAcelerar, int freando, int segurarMarcha, float tempoQuadro);
int estaTrocandoDeMarcha(const Motor *motor);

#endif
