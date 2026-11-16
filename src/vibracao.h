/*
 * vibracao.h
 * ------------------------------------------------------------------
 * O que o piloto sente no controle: transforma o que está acontecendo com
 * o carro em vibração. A regra é o silêncio: no asfalto liso, em linha reta,
 * o controle fica quieto, e vibra quando há algo a dizer — a zebra, a grama,
 * o pneu no limite, a traseira soltando, o nitro, o muro, as batidas e os
 * momentos da corrida (a contagem, a largada, a chegada, o Guido).
 *
 * A corrida e o mundo aberto sentem o carro do mesmo jeito: só muda de onde
 * vem o chão (a pista ou a malha do mundo) e o que é da corrida.
 *
 * Este módulo NÃO usa a raylib nem fala com o controle: só decide a
 * vibração. Quem manda para o DualSense é o vibrar_mac.m (veja vibrar.h).
 * ------------------------------------------------------------------
 */

#ifndef VIBRACAO_H
#define VIBRACAO_H

#include "corrida.h"
#include "configuracoes.h"
#include "livre.h"

/* A vibração de um quadro. Tudo de 0 a 1. */
typedef struct {
    float grave;          /* a mão esquerda: o peso do carro, o chão, as pancadas */
    float agudo;          /* a mão direita: o que é fino, como o pneu escorregando */
    float noAcelerador;   /* o gatilho do acelerador treme: a roda patinando */
} Vibracao;

/* O que a vibração precisa lembrar de um quadro para o outro */
typedef struct {
    float relogio;             /* segundos: os pulsos que dependem do tempo */
    float faseDoChao;          /* metros andados: os pulsos que dependem do caminho */
    float golpeGrave, golpeAgudo;   /* os trancos, que somem aos poucos */
    float caudaDoNitro;        /* o nitro apagando aos poucos */
    float tempoComNitro;       /* segundos desde que o nitro ligou */
    int nitroAntes;
    int driftAntes;
    int estadoDoPitAntes;
    int numeroDaContagemAntes;
    int terminouAntes;
    int pneusEstouradosAntes;
    float segundosForaDoAsfalto;   /* as sensações que duram vão assentando (a mão se acostuma) */
    float segundosDeLado;
    int pulsosDaVitoria;           /* a comemoração: os pulsos que ainda faltam... */
    float proximoPulso;            /* ...e em quantos segundos vem o próximo */
    int arrancando;                /* 1: saindo do carro parado, com a roda patinando */
    int arrancadaArmada;           /* 1: a próxima arrancada pode vir (soltou o acelerador desde a última) */
    int emBatida;                  /* 1: encostado em algo neste quadro (a batida só dá o tranco quando começa) */
    int noMuroAntes;
    float segundosDeArrancada;
} EstadoDaVibracao;

void iniciarEstadoDaVibracao(EstadoDaVibracao *estado);
Vibracao calcularVibracao(EstadoDaVibracao *estado, Corrida *corrida, float tempoQuadro);
/* No mundo aberto: o carro livre e a batida deste quadro (o passeio a consome para o som) */
Vibracao calcularVibracaoDoPasseio(EstadoDaVibracao *estado, const CarroLivre *carro, float pancada, float tempoQuadro);
Vibracao vibracaoAoGosto(Vibracao vibracao, const Configuracoes *config);

#endif
