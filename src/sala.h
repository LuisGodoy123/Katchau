/*
 * sala.h
 * ------------------------------------------------------------------
 * As salas do modo online: achar, criar, entrar e combinar a corrida.
 *
 * Vale da tela de salas até o instante da largada. A partir da largada,
 * quem cuida da sincronia é online.c — mas a sala continua viva ao lado,
 * medindo o ping e vendo quem caiu.
 *
 * O modelo é "o dono manda": quem cria a sala escolhe pista e nível, decide
 * qual carro cabe a quem (dois jogadores não podem pegar o mesmo) e dá a
 * largada. Os convidados PEDEM as coisas; o que volta no pacote da sala é a
 * verdade. Isso tira qualquer disputa: não existe o caso de duas máquinas
 * acharem que acertaram o mesmo carro.
 *
 * Este módulo NÃO usa a raylib.
 * ------------------------------------------------------------------
 */

#ifndef SALA_H
#define SALA_H

#include "corrida.h"
#include "protocolo.h"
#include "rede.h"

/* Onde esta máquina está no modo online */
typedef enum {
    SALA_DESLIGADA,       /* fora do modo online */
    SALA_PROCURANDO,      /* na lista, ouvindo os faróis da rede */
    SALA_ENTRANDO,        /* pediu para entrar e espera a resposta do dono */
    SALA_SOU_DONO,        /* criei a sala e espero gente */
    SALA_SOU_CONVIDADO,   /* entrei na sala de alguém */
    SALA_NA_CORRIDA,      /* a corrida largou */
    SALA_DEU_ERRADO       /* recusado, o dono sumiu, a rede não abriu: veja recadoDaSala */
} EstadoDaSala;

/* Uma sala vista na rede (uma linha da lista) */
typedef struct {
    unsigned int id;
    char nome[TAMANHO_DO_NOME_DA_SALA];
    char dono[TAMANHO_DO_APELIDO];
    int pista, nivel, voltas;
    int jogadores, limite;
    int emCorrida;
    int versaoDiferente;     /* 1: build diferente; aparece na lista, mas não dá para entrar */
    unsigned int ping;       /* milissegundos (0 = ainda não mediu) */
    float semFarol;          /* segundos desde o último anúncio; passou do limite, sai da lista */
    EnderecoDaRede endereco;
} SalaNaRede;

/* Um jogador dentro da sala */
typedef struct {
    char nome[TAMANHO_DO_APELIDO];
    int numeroDoCarro;
    int pronto;
    int ligado;              /* 0: vaga vazia */
    unsigned int ping;
    EnderecoDaRede endereco; /* só o dono usa */
    float calado;            /* só o dono usa: segundos sem ouvir este jogador */
} JogadorDaSala;

/* ---------- Ligar e desligar ---------- */
void iniciarSala(void);
void encerrarSala(void);
void atualizarSala(Corrida *corrida, float tempoQuadro);   /* uma vez por quadro: esvazia a caixa e manda os periódicos */
EstadoDaSala estadoDaSala(void);
const char *recadoDaSala(void);          /* "SALA CHEIA", "O DONO SAIU"... ou "" */
const char *meuApelido(void);
int aRedeAbriu(void);                    /* 0: nem o soquete abriu (o modo online fica apagado) */

/* ---------- A lista de salas ---------- */
void comecarAProcurar(void);
void pararDeProcurar(void);
int totalDeSalasAchadas(void);
const SalaNaRede *salaAchada(int i);
void criarSala(int pista, int nivel, int carro);
void entrarNaSalaAchada(int i, int carro);

/* ---------- A sala de espera ---------- */
void sairDaSala(void);
int souODonoDaSala(void);
int totalDeJogadoresDaSala(void);
const JogadorDaSala *jogadorDaSala(int vaga);
int minhaVagaNaSala(void);
int pistaDaSala(void);
int nivelDaSala(void);
void escolherPistaDaSala(int pista);     /* só o dono; nos outros, não faz nada */
void escolherNivelDaSala(int nivel);     /* só o dono */
void escolherCarroDaSala(int carro);     /* no convidado é um pedido: o dono confirma */
int carroEstaLivreNaSala(int carro);
void marcarProntoNaSala(int pronto);
int estouProntoNaSala(void);
int todosProntosNaSala(void);
int podeLargar(void);                    /* 1 no dono quando todos estão prontos */
void mandarLargar(void);                 /* só o dono */
float contagemParaLargar(void);          /* > 0: a sala está contando para largar */

/* ---------- A largada e a corrida ---------- */
int aSalaLargou(void);                   /* 1 uma vez: o main.c leva todos para a corrida */
const FormacaoOnline *formacaoDaSala(void);
int emCorridaOnline(void);
void acabouACorridaOnline(void);

/* ---------- O que online.c precisa da sala ---------- */
Soquete *soqueteDaSala(void);
unsigned short proximaSequencia(void);
const EnderecoDaRede *enderecoDoDono(void);
int enderecoDoConvidado(int vaga, EnderecoDaRede *destino);
void ouviOConvidado(int vaga);        /* chegou pacote dele: segura o relógio da IA */
void ouviODonoAgora(void);            /* chegou retrato do dono: ele está vivo */
float segundosCaladoOConvidado(int vaga);
float segundosSemOuvirODono(void);
void perdiODono(void);

#endif
