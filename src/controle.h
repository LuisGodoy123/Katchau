#ifndef CONTROLE_H
#define CONTROLE_H

/*
 * controle.h
 * ------------------------------------------------------------------
 * O controle (gamepad) de Xbox, PlayStation e parecidos, ligado por
 * USB ou Bluetooth. Funciona junto com o teclado: o que um ou outro
 * apertar vale.
 *
 * Na corrida, o controle preenche os mesmos Comandos do teclado:
 *   RT ou A acelera, LT ou X freia, analógico esquerdo ou direcional
 *   vira, START abandona a corrida.
 * Nos menus, direcional ou analógico andam, A confirma e B volta.
 * (Os nomes são os do controle de Xbox; no de PlayStation, os mesmos
 * lugares são R2/✕, L2/□ e ○.)
 *
 * Módulo que usa a raylib (parte de entrada).
 * ------------------------------------------------------------------
 */

#include "carro.h"

/* Os botões que os menus perguntam se foram apertados */
typedef enum {
    BOTAO_CIMA,
    BOTAO_BAIXO,
    BOTAO_ESQUERDA,
    BOTAO_DIREITA,
    BOTAO_CONFIRMAR,   /* A */
    BOTAO_VOLTAR,      /* B */
    BOTAO_START,
    TOTAL_BOTOES
} BotaoDoControle;

void atualizarControle(void);
int controleConectado(void);
int controleApertou(BotaoDoControle botao);
void somarComandosDoControle(Comandos *comandos);
const char *nomeNoControle(const char *tecla);

#endif
