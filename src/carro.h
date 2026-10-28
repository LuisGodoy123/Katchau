/*
 * carro.h
 * ------------------------------------------------------------------
 * O carro e o seu movimento pela pista.
 *
 * O carro não lê o teclado. Quem decide o que fazer preenche uma
 * struct Comandos (acelerar, frear, virar) e o carro só obedece.
 * Assim o mesmo código serve para o jogador (comandos vêm do teclado,
 * em main.c) e para os rivais (comandos vêm da IA, na Fase 4).
 *
 * Este módulo NÃO usa a raylib.
 * ------------------------------------------------------------------
 */

#ifndef CARRO_H
#define CARRO_H

#include "config.h"
#include "pista.h"

#define TAMANHO_NOME        20
#define LIMITE_PISTA        1.0f   /* além disso (para qualquer lado) é grama */

/* O que o piloto quer fazer neste quadro: 1 = sim, 0 = não. */
typedef struct {
    int acelerar;
    int frear;
    int esquerda;
    int direita;
} Comandos;

typedef struct {
    char nome[TAMANHO_NOME];
    int numero;                    /* 0 = jogador, 1 a 7 = rivais; define a cor no desenho */

    /* ---- Onde está ---- */
    PosicaoNaPista posicao;        /* segmento atual (nó da lista) e quanto andou nele */
    float distanciaPercorrida;     /* total desde a largada; ordena os carros na Fase 4 */
    float deslocamentoLateral;     /* 0 = centro, -1 = borda esquerda, +1 = borda direita */

    /* ---- Como está andando ---- */
    float velocidade;              /* unidades de mundo por segundo */
    int voltas;                    /* voltas completadas */
    float temposDeVolta[TOTAL_VOLTAS];
    float inicioDaVolta;           /* momento da corrida em que a volta atual começou */
    float tempoDeChegada;          /* momento em que completou a última volta (0 = ainda correndo) */
    Comandos ultimosComandos;      /* o que o piloto fez no último quadro (lanternas de freio) */
    float linhaAlvo;               /* só para a IA: em que ponto da largura ela quer andar */

    /* ---- Ficha técnica (cada carro tem a sua; usada na garagem da Fase 6) ---- */
    float velocidadeMaxima;        /* unidades por segundo */
    float aceleracao;              /* quanto a velocidade sobe por segundo */
    float aderencia;               /* de 0 a 1: quanto resiste a ser jogado para fora na curva */
} Carro;

Carro criarCarro(const char *nome, int numero, Segmento *largada, float deslocamentoLateral,
                 float velocidadeMaxima, float aceleracao, float aderencia);
int atualizarCarro(Carro *carro, Comandos comandos, float tempoQuadro);
int estaForaDaPista(Carro *carro);
int velocidadeEmKmh(Carro *carro);
float velocidadeSeguraNaCurva(Carro *carro, float curvatura);

#endif
