/*
 * papo.c
 * ------------------------------------------------------------------
 * Uma conversa do jogador com alguém da cidade. Veja papo.h.
 * ------------------------------------------------------------------
 */

#include <string.h>
#include "papo.h"

#define FALA_DURA_NO_MINIMO   2.4f      /* segundos */
#define SEGUNDOS_POR_LETRA    0.065f    /* o tempo de ler */
#define FALA_DURA_A_MAIS      1.0f

/* O tempo de ler uma fala */
static float tempoDeLer(const char *fala) {
    float t = FALA_DURA_A_MAIS + SEGUNDOS_POR_LETRA * (float) strlen(fala);
    return (t > FALA_DURA_NO_MINIMO) ? t : FALA_DURA_NO_MINIMO;
}

static void irParaAFala(Papo *papo, int qual) {
    papo->atual = qual;
    if (qual >= MAIS_FALAS_NO_PAPO || papo->roteiro->falas[qual] == NULL) {
        papo->roteiro = NULL;   /* acabou */
        return;
    }
    papo->duracao = papo->restante = tempoDeLer(papo->roteiro->falas[qual]);
}

void comecarPapo(Papo *papo, const RoteiroDoPapo *roteiro) {
    papo->roteiro = roteiro;
    if (roteiro != NULL) {
        irParaAFala(papo, 0);
    }
}

void passarPapo(Papo *papo, float tempo) {
    if (papo->roteiro == NULL) {
        return;
    }
    papo->restante -= tempo;
    if (papo->restante <= 0.0f) {
        irParaAFala(papo, papo->atual + 1);
    }
}

void avancarPapo(Papo *papo) {
    if (papo->roteiro != NULL) {
        irParaAFala(papo, papo->atual + 1);
    }
}

int papoAtivo(const Papo *papo) {
    return papo->roteiro != NULL;
}

const char *falaDoPapo(const Papo *papo, int *doJogador) {
    if (papo->roteiro == NULL) {
        return NULL;
    }
    const char *fala = papo->roteiro->falas[papo->atual];
    int dele = (fala[0] == '>');
    if (doJogador != NULL) {
        *doJogador = dele;
    }
    return dele ? fala + 1 : fala;
}

float duracaoDaFalaDoPapo(const Papo *papo) {
    return papo->duracao;
}
