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

#define TOTAL_RIVAIS   16
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

/* Quem é quem numa corrida online: uma vaga por humano, na ordem da sala. O resto
 * dos 14 carros fica com a IA, como sempre. Quem monta isto é sala.c, a partir do
 * pacote de largada; a corrida só obedece. */
typedef struct {
    int totalDeJogadores;                        /* 1 a MAXIMO_DE_JOGADORES */
    int numeroDoCarro[MAXIMO_DE_JOGADORES];      /* o carro de cada um (nunca repetido) */
    int lugarNoGrid[MAXIMO_DE_JOGADORES];
    int euSou;                                   /* qual dos humanos é ESTA máquina */
    int souODono;                                /* 1: é esta máquina que roda a física de todos */
    unsigned int semente;                        /* o sorteio dos rivais, igual em todas as máquinas */
} FormacaoOnline;

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
    int posicaoFinal;           /* posição do jogador na chegada (1 a TOTAL_CARROS) */
    int mcqueenLiderava;        /* 1 enquanto o jogador, sendo o McQueen, está em 1º (para as falas dele) */
    float mcqueenTempoNaPonta;  /* segundos seguidos em 1º */
    int mcqueenPosicao;         /* a posição do jogador (sendo o McQueen) que as falas já "viram" (0 = ainda não) */
    int mcqueenPosicaoNova;     /* a posição nova que está esperando para ser confirmada */
    float mcqueenTempoNaPosicaoNova;   /* segundos que ela já se mantém */

    int totalSegmentos;                          /* tamanho da volta, para saber o setor de cada carro */
    float melhoresSetoresDaCorrida[TOTAL_SETORES];   /* o melhor de cada setor entre todos (0 = nenhum) */
    float voltaMaisRapida;                       /* a melhor volta entre todos (0 = nenhuma) */

    /* ---- Corrida em rede (online.c). Numa corrida de um jogador, tudo isto fica em 0 ----
     *
     * Este módulo NÃO conhece a rede: não inclui rede.h nem online.h. Os dois caminhos
     * novos de atualizarCorrida são escolhidos por DADO, e é online.c quem preenche os
     * dados antes de chamar. Assim corrida.c continua compilando nos testes de terminal,
     * que não linkam a rede. */
    int emRede;                                  /* 1: esta corrida é em rede */
    int souODono;                                /* 1: esta máquina roda a física de todos */
    int totalDeHumanos;
    int euSou;                                   /* qual dos humanos é o jogador desta máquina */
    Carro *humanos[MAXIMO_DE_JOGADORES];         /* humanos[euSou] é sempre &corrida->jogador */
    Comandos comandosDosHumanos[MAXIMO_DE_JOGADORES];   /* o dono preenche ANTES de atualizarCorrida */
    float segundosSemComandos[MAXIMO_DE_JOGADORES];     /* passou do limite: a IA assume aquele carro */
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
void iniciarCorridaOnline(Corrida *corrida, Pista *pista, NivelDaCorrida nivel, const FormacaoOnline *formacao);
void atualizarCorrida(Corrida *corrida, Comandos comandosJogador, float tempoQuadro);
int carroEDeHumano(const Corrida *corrida, const Carro *carro);
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
int jogadorNoPodio(Corrida *corrida);
float segundosAteAVitoria(Corrida *corrida);

#endif
