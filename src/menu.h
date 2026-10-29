#ifndef MENU_H
#define MENU_H

/*
 * Menu: menu principal, garagem, como jogar e tela de resultado (fase 6).
 * Módulo de desenho: pode usar a raylib.
 *
 * Cada tela tem duas funções, como o laço do main.c:
 *   atualizar...()  lê o teclado e devolve para qual tela ir;
 *   desenhar...()   só desenha.
 */

#include "corrida.h"
#include "ordenacao.h"

/* As telas do jogo. O main.c guarda em qual delas o jogador está. */
typedef enum {
    TELA_MENU,
    TELA_GARAGEM,
    TELA_COMO_JOGAR,
    TELA_CORRIDA,
    TELA_RESULTADO,
    TELA_SAIR
} Tela;

/* O que as telas de menu precisam lembrar entre um quadro e outro. */
typedef struct {
    int opcaoMenu;                              /* item marcado no menu principal */
    int carroEscolhido;                         /* número do carro do jogador (0 a 7) */
    Atributo criterio;                          /* por qual atributo a garagem está ordenada */
    const FichaTecnica *garagem[TOTAL_CARROS];  /* as fichas, na ordem do Merge Sort */
    int selecionado;                            /* linha marcada na garagem */
} EstadoMenu;

void iniciarMenu(EstadoMenu *menu);

Tela atualizarMenuPrincipal(EstadoMenu *menu);
void desenharMenuPrincipal(EstadoMenu *menu, Pista *pista);

Tela atualizarGaragem(EstadoMenu *menu);
void desenharGaragem(EstadoMenu *menu, Pista *pista);

Tela atualizarComoJogar(void);
void desenharComoJogar(Pista *pista);

Tela atualizarResultado(void);
void desenharResultado(Corrida *corrida);

#endif
