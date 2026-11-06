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

#define TOTAL_RIVAIS   13
#define TOTAL_CARROS   (TOTAL_RIVAIS + 1)   /* rivais + jogador */

/* Os níveis de dificuldade, escolhidos antes da corrida */
typedef enum {
    NIVEL_FACIL,
    NIVEL_MEDIO,
    NIVEL_DIFICIL,
    TOTAL_NIVEIS
} NivelDaCorrida;

typedef struct {
    const char *nome;
    int voltas;
    float habilidadeDaPole;     /* de 0 a 1: quanto do limite do carro usa o rival que larga na frente */
    float habilidadeDoUltimo;   /* e o último dos rivais */
    int temParada;              /* 1: parada obrigatória, onde a pista tem pit lane */
    int gastaPneu;              /* 1: os pneus gastam (precisa de pit lane para trocar) */
    float pelotao;              /* de 0 a 1: o quanto os rivais esperam ou se esforçam para alcançar o jogador */
    float defesa;               /* de 0 a 1: a chance de um rival fechar a porta para quem vem atrás */
    float revide;               /* de 0 a 1: a força do revide de quem o jogador acabou de passar */
    float variacao;             /* o quanto o ritmo dos rivais muda de uma volta para a outra */
} FichaDoNivel;

typedef struct {
    Pista *pista;
    NivelDaCorrida nivel;
    int totalVoltas;
    int temParada;              /* 1: esta corrida tem pitstop */
    Carro jogador;
    Carro rivais[TOTAL_RIVAIS];

    /* Ponteiros para todos os carros, em ordem de corrida:
     * classificacao[0] é o 1º lugar. Reordenado a cada quadro
     * com Insertion Sort (ordenacao.c). */
    Carro *classificacao[TOTAL_CARROS];

    float contagemRegressiva;   /* segundos até a largada; a corrida só anda quando chega a 0 */
    float tempoDeCorrida;       /* segundos desde a largada (não para no fim) */
    int terminou;               /* 1 depois que o jogador completa todas as voltas */
    int desclassificado;        /* 1 se o jogador terminou sem fazer a parada obrigatória */
    int posicaoFinal;           /* posição do jogador na chegada (1 a 8) */
    int mcqueenLiderava;        /* 1 enquanto o jogador, sendo o McQueen, está em 1º (para as falas dele) */
    float mcqueenTempoNaPonta;  /* segundos seguidos em 1º */
    int mcqueenPosicao;         /* a posição do jogador (sendo o McQueen) que as falas já "viram" (0 = ainda não) */
    int mcqueenPosicaoNova;     /* a posição nova que está esperando para ser confirmada */
    float mcqueenTempoNaPosicaoNova;   /* segundos que ela já se mantém */

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
const FichaDoNivel *fichaDoNivel(NivelDaCorrida nivel);
void iniciarCorrida(Corrida *corrida, Pista *pista, int carroDoJogador, NivelDaCorrida nivel);
void atualizarCorrida(Corrida *corrida, Comandos comandosJogador, float tempoQuadro);
int todosChegaram(Corrida *corrida);
float batidaDoMcQueen(Corrida *corrida, int *contraCarro);
float rodadaDoMcQueen(Corrida *corrida);
int mudancaDePosicaoDoMcQueen(Corrida *corrida, float tempoQuadro);
int mudancaDeLiderancaDoMcQueen(Corrida *corrida, float tempoQuadro);
int voltaAtual(Carro *carro);
int posicaoDoCarro(Corrida *corrida, Carro *carro);
int penetracaoEntreCarros(const Corrida *corrida, const Carro *a, const Carro *b, float *naFrente, float *deLado);
float intervaloEmSegundos(Carro *carro, Carro *referencia);
SituacaoDoSetor situacaoDoSetor(Corrida *corrida, Carro *carro, int setor);
Bandeira bandeiraDaCorrida(Corrida *corrida);
int jogadorVenceu(Corrida *corrida);
float segundosAteAVitoria(Corrida *corrida);

#endif
