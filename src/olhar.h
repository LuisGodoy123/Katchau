/*
 * olhar.h
 * ------------------------------------------------------------------
 * Para onde o jogador está olhando, com o analógico direito: a câmera gira em
 * volta do carro. Quanto mais inclinado, mais ela gira: todo para o lado olha
 * para o lado, todo para cima olha para o alto e todo para baixo olha para
 * trás, para ver quem vem atrás. Soltando, ela volta para trás do carro.
 *
 * Este módulo NÃO usa a raylib: só transforma o analógico em ângulos.
 * ------------------------------------------------------------------
 */

#ifndef OLHAR_H
#define OLHAR_H

typedef struct {
    float giro;        /* graus em volta do carro: 0 = para a frente, 90 = direita, ±180 = para trás */
    float elevacao;    /* graus acima do normal (0 a ELEVACAO_MAXIMA) */
    float velocidadeDoGiro;        /* graus por segundo: a câmera acelera e freia, sem trancos */
    float velocidadeDaElevacao;
} Olhar;

#define ELEVACAO_MAXIMA  32.0f

Olhar olharParaAFrente(void);
void atualizarOlhar(Olhar *olhar, float eixoX, float eixoY, float tempoQuadro);

#endif
