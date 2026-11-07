/*
 * motorgranular.h
 * ------------------------------------------------------------------
 * O som do motor do jogador a partir de uma gravação de um carro acelerando e passando as
 * marchas (nitro_car.mp3), tocada de um jeito que a gravação obedece ao carro, e não o contrário:
 *
 *   - cada marcha do jogo usa a "arrancada" da gravação que corresponde a ela (a 1ª marcha usa a
 *     primeira subida, a 2ª a segunda, e da 3ª em diante a terceira, que é a mais longa);
 *   - a rotação do motor do jogo escolhe o ponto da gravação cujo tom é o dela: quando o giro sobe,
 *     o som avança na gravação; quando o câmbio troca e o giro cai, o som volta para o ponto de tom
 *     igual na marcha nova. O tom sai sempre o da rotação, sem esticar nem encolher o som;
 *   - o som é montado com WSOLA: pedacinhos da gravação, de 46 ms, emendados com janelas suaves e
 *     deslocados até as ondas casarem, para não haver estalo nem "batimento" na emenda;
 *   - se a rotação pedir um tom que a marcha da gravação não tem, o pedacinho é tocado um pouco mais
 *     rápido ou mais lento (até 0,55x ~ 1,8x) para chegar ao tom certo.
 *
 * As tabelas "ponto da gravação x tom" (em Hz) foram medidas na própria gravação.
 *
 * Este módulo NÃO usa a raylib: quem carrega o arquivo e toca o resultado é o efeitos.c.
 * ------------------------------------------------------------------
 */

#ifndef MOTORGRANULAR_H
#define MOTORGRANULAR_H

#define TAXA_DO_MOTOR_GRANULAR  44100
#define GRAO_DO_MOTOR           2048    /* amostras de um pedacinho (a metade é o passo entre eles) */
#define PASSO_DO_MOTOR          (GRAO_DO_MOTOR / 2)

typedef struct {
    const float *fonte;              /* a gravação, mono, a 44100 Hz (não é copiada) */
    int total;                       /* quantas amostras ela tem */
    int marcha;                      /* a marcha do jogo, de 0 a TOTAL_MARCHAS - 1 */
    float giro;                      /* a rotação do jogo, de 0 a 1 */
    float volume;                    /* o volume pedido (0 a 1) */
    float volumeAtual;               /* o volume que já foi alcançado (anda até o pedido) */
    int temAnterior;                 /* já saiu algum pedacinho (há onde casar o próximo) */
    float inicioAnterior;            /* onde, na gravação, começou o último pedacinho */
    float taxaAnterior;              /* e a velocidade com que ele foi lido */
    float cauda[PASSO_DO_MOTOR];     /* a segunda metade, já com janela, do último pedacinho: soma no próximo */
    float pronto[PASSO_DO_MOTOR];    /* amostras já montadas e ainda não entregues */
    int restam;                      /* quantas delas faltam entregar (as últimas do vetor) */
    float tomPedido;                 /* o tom (Hz) do último pedacinho, para quem quiser conferir */
} MotorGranular;

int iniciarMotorGranular(MotorGranular *motor, const float *fonte, int total);
void definirMotorGranular(MotorGranular *motor, int marcha, float giro, float volume);
void gerarMotorGranular(MotorGranular *motor, float *saida, int quadros);
float tomDoGiro(float giro);
int pontoDaGravacao(int marcha, float tom, float *segundos, float *tomDaGravacao);

#endif
