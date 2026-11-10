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
#include "config.h"
#include "configuracoes.h"

#define MAXIMO_DE_CONTROLES   4

/* De onde vêm os comandos de um jogador. Num jogador só, o jogo usa a entrada COMPLETA
 * (entradaDeTudo): as setas, o WASD e qualquer controle ligado, todos no mesmo carro, que é
 * exatamente o que ele sempre fez. Na tela dividida cada um fica com UMA coluna de teclas
 * (coluna 1 = setas, coluna 2 = WASD) e com UM controle. */
#define SEM_CONTROLE       (-1)   /* este jogador não tem controle */
#define QUALQUER_CONTROLE  (-2)   /* o primeiro ligado: o jogador único */
#define SEM_TECLADO        (-1)   /* este jogador não tem teclado */
#define TODAS_AS_TECLAS    (-2)   /* as duas colunas de teclas: o jogador único */

typedef struct {
    int controle;          /* 0 a MAXIMO_DE_CONTROLES-1, SEM_CONTROLE ou QUALQUER_CONTROLE */
    int colunaDeTeclas;    /* COLUNA_TECLA_1, COLUNA_TECLA_2, SEM_TECLADO ou TODAS_AS_TECLAS */
} EntradaDoJogador;

/* Os botões que os menus perguntam se foram apertados */
typedef enum {
    BOTAO_CIMA,
    BOTAO_BAIXO,
    BOTAO_ESQUERDA,
    BOTAO_DIREITA,
    BOTAO_CONFIRMAR,   /* A */
    BOTAO_VOLTAR,      /* B */
    BOTAO_START,
    BOTAO_LIMPAR,      /* Y */
    BOTAO_CAMERA,      /* touchpad do PlayStation (ou SELECT/BACK) */
    BOTAO_ABA_ANTERIOR,   /* LB / L1 */
    BOTAO_ABA_SEGUINTE,   /* RB / R1 */
    TOTAL_BOTOES
} BotaoDoControle;

void atualizarControle(void);
int controleConectado(void);
int controleApertou(BotaoDoControle botao);
void lerComandosDoTeclado(Comandos *comandos);
void somarComandosDoControle(Comandos *comandos);

/* ---- Um jogador por entrada: a tela dividida ---- */
EntradaDoJogador entradaDeTudo(void);
void repartirEntradas(EntradaDoJogador entradas[], int totalDeJogadores);
void lerComandosDoJogador(const EntradaDoJogador *entrada, Comandos *comandos);
void lerAnalogicoDaCameraDoJogador(const EntradaDoJogador *entrada, float *eixoX, float *eixoY);
int controleDoJogadorApertou(const EntradaDoJogador *entrada, BotaoDoControle botao);
int controleDoJogador(const EntradaDoJogador *entrada);   /* -1 = este jogador joga de teclado */
int quantosControles(void);
int teclaApertadaAgora(void);
int botaoApertadoAgora(void);
const char *nomeDaTecla(int tecla);
const char *nomeDoBotao(int botao);
void lerAnalogicoDaCamera(float *eixoX, float *eixoY);
const char *nomeNoControle(const char *tecla);

#endif
