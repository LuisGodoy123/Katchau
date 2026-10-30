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
#define TOTAL_SETORES       3      /* a volta é dividida em 3 setores, como no F1 */

/* Ficha técnica de um modelo de carro. A garagem mostra e ordena
 * as fichas; a corrida cria os carros a partir delas. */
typedef struct {
    const char *nome;
    const char *descricao;         /* o jeito do carro, mostrado na garagem */
    float velocidadeMaxima;        /* unidades por segundo */
    float aceleracao;              /* quanto a velocidade sobe por segundo */
    float aderencia;               /* de 0 a 1 */
} FichaTecnica;

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
    float volante;                 /* -1 todo à esquerda, 0 reto, +1 todo à direita; anda aos poucos */
    float derrapagem;              /* 0 com folga; sobe até 1 quando a curva está vencendo o volante */
    int voltas;                    /* voltas completadas */
    float temposDeVolta[TOTAL_VOLTAS];
    float inicioDaVolta;           /* momento da corrida em que a volta atual começou */
    float tempoDeChegada;          /* momento em que completou a última volta (0 = ainda correndo) */
    float melhorVolta;             /* menor tempo de volta até agora (0 = nenhuma volta completa) */
    Comandos ultimosComandos;      /* o que o piloto fez no último quadro (lanternas de freio) */
    float linhaAlvo;               /* só para a IA: em que ponto da largura ela quer andar */

    /* ---- Setores: a volta em 3 partes, para o painel da volta no HUD ---- */
    int setorAtual;                              /* 0, 1 ou 2 */
    float inicioDoSetor;                         /* momento da corrida em que o setor atual começou */
    float setoresDaVolta[TOTAL_SETORES];         /* setores já feitos nesta volta (0 = ainda não) */
    float melhoresSetores[TOTAL_SETORES];        /* melhor tempo do carro em cada setor (0 = nenhum) */
    float setoresDaMelhorVolta[TOTAL_SETORES];   /* os setores da melhor volta, para comparar */
    float diferenca;                             /* ganho (< 0) ou perda (> 0) contra a melhor volta, em s */
    int temDiferenca;                            /* 0 até existir uma melhor volta para comparar */

    /* ---- Ficha técnica (cada carro tem a sua; usada na garagem da Fase 6) ---- */
    float velocidadeMaxima;        /* unidades por segundo */
    float aceleracao;              /* força da arrancada (a aceleração cai perto da máxima) */
    float aderencia;               /* de 0 a 1: quanto resiste a ser jogado para fora na curva */
} Carro;

Carro criarCarro(const char *nome, int numero, Segmento *largada, float deslocamentoLateral,
                 float velocidadeMaxima, float aceleracao, float aderencia);
int atualizarCarro(Carro *carro, Comandos comandos, float tempoQuadro);
int estaForaDaPista(Carro *carro);
int velocidadeEmKmh(Carro *carro);
int converterParaKmh(float velocidade);
float velocidadeSeguraNaCurva(Carro *carro, float curvatura);
float tempoAteVelocidade(const FichaTecnica *ficha, float velocidade);

#endif
