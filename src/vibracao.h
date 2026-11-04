/*
 * vibracao.h
 * ------------------------------------------------------------------
 * O que o piloto sente no controle: transforma o que está acontecendo com
 * o carro em vibração, como nos jogos de corrida: o asfalto passando, o pneu
 * perdendo o chão, o nitro, as batidas, as marchas, a zebra e a grama.
 *
 * Este módulo NÃO usa a raylib nem fala com o controle: só decide a
 * vibração. Quem manda para o DualSense é o vibrar_mac.m (veja vibrar.h).
 * ------------------------------------------------------------------
 */

#ifndef VIBRACAO_H
#define VIBRACAO_H

#include "corrida.h"
#include "configuracoes.h"

/* A vibração de um quadro. Tudo de 0 a 1. */
typedef struct {
    float grave;          /* a mão esquerda: o peso do carro, o chão, as pancadas */
    float agudo;          /* a mão direita: o que é fino, como a textura do asfalto e o pneu */
    float noAcelerador;   /* o gatilho do acelerador treme: a roda patinando */
} Vibracao;

/* O que a vibração precisa lembrar de um quadro para o outro */
typedef struct {
    float relogio;             /* segundos: os pulsos que dependem do tempo */
    float faseDoChao;          /* metros andados: os pulsos que dependem do caminho */
    float golpeGrave, golpeAgudo;   /* os trancos, que somem aos poucos */
    float caudaDoNitro;        /* o nitro apagando aos poucos */
    float tempoComNitro;       /* segundos desde que o nitro ligou */
    int juntaAntes;            /* em que junta do asfalto o carro estava */
    int marchaAntes;
    int nitroAntes;
    int driftAntes;
    int estadoDoPitAntes;
    int numeroDaContagemAntes;
    int terminouAntes;
} EstadoDaVibracao;

void iniciarEstadoDaVibracao(EstadoDaVibracao *estado);
Vibracao calcularVibracao(EstadoDaVibracao *estado, Corrida *corrida, float tempoQuadro);
Vibracao vibracaoAoGosto(Vibracao vibracao, const Configuracoes *config);

#endif
