/*
 * corrida.h
 * ------------------------------------------------------------------
 * Regras da corrida: fichas dos carros, grid, contagem regressiva,
 * tempo, voltas, colisão, classificação e fim.
 * Na Fase 5 entram o turbo (com vácuo) e os obstáculos.
 *
 * Este módulo NÃO usa a raylib.
 * ------------------------------------------------------------------
 */

#ifndef CORRIDA_H
#define CORRIDA_H

#include "pista.h"
#include "carro.h"

#define TOTAL_RIVAIS   7
#define TOTAL_CARROS   (TOTAL_RIVAIS + 1)   /* rivais + jogador */

typedef struct {
    Pista *pista;
    Carro jogador;
    Carro rivais[TOTAL_RIVAIS];

    /* Ponteiros para os 8 carros, em ordem de corrida:
     * classificacao[0] é o 1º lugar. Reordenado a cada quadro
     * com Insertion Sort (ordenacao.c). */
    Carro *classificacao[TOTAL_CARROS];

    float contagemRegressiva;   /* segundos até a largada; a corrida só anda quando chega a 0 */
    float tempoDeCorrida;       /* segundos desde a largada (não para no fim) */
    int terminou;               /* 1 depois que o jogador completa TOTAL_VOLTAS */
    int posicaoFinal;           /* posição do jogador na chegada (1 a 8) */

    int totalSegmentos;                          /* tamanho da volta, para saber o setor de cada carro */
    float melhoresSetoresDaCorrida[TOTAL_SETORES];   /* o melhor de cada setor entre todos (0 = nenhum) */
    float voltaMaisRapida;                       /* a melhor volta entre todos (0 = nenhuma) */
} Corrida;

/* Como foi um setor da volta atual, para pintar no painel da volta */
typedef enum {
    SETOR_PENDENTE,          /* ainda não chegou nele */
    SETOR_MAIS_LENTO,        /* fez, mas mais devagar que o próprio melhor */
    SETOR_MELHOR_PESSOAL,    /* o melhor do carro nesse setor */
    SETOR_MELHOR_DA_CORRIDA  /* o melhor de todos nesse setor */
} SituacaoDoSetor;

/* Estado da prova, para a etiqueta do painel de posições */
typedef enum {
    BANDEIRA_LARGADA,        /* contagem regressiva */
    BANDEIRA_VERDE,          /* correndo */
    BANDEIRA_QUADRICULADA    /* o 1º colocado já recebeu a bandeirada */
} Bandeira;

const FichaTecnica *fichaDoCarro(int numero);
void iniciarCorrida(Corrida *corrida, Pista *pista, int carroDoJogador);
void atualizarCorrida(Corrida *corrida, Comandos comandosJogador, float tempoQuadro);
int todosChegaram(Corrida *corrida);
int voltaAtual(Carro *carro);
int posicaoDoCarro(Corrida *corrida, Carro *carro);
Carro *carroDaFrente(Corrida *corrida, Carro *carro);
float intervaloEmSegundos(Carro *carro, Carro *referencia);
SituacaoDoSetor situacaoDoSetor(Corrida *corrida, Carro *carro, int setor);
Bandeira bandeiraDaCorrida(Corrida *corrida);
int jogadorVenceu(Corrida *corrida);
float segundosAteAVitoria(Corrida *corrida);

#endif
