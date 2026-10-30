#ifndef MENU_H
#define MENU_H

/*
 * Menu: menu principal, garagem, como jogar e tela de resultado (fase 6).
 * Módulo de desenho: pode usar a raylib.
 *
 * Cada tela tem duas funções, como o laço do main.c:
 *   atualizar...()  lê o teclado e devolve para qual tela ir;
 *   desenhar...()   só desenha, em coordenadas de uma tela 800x600 que o
 *                   main.c amplia para a tela real (comecarInterface).
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

#define TOTAL_OPCOES 4   /* CORRER, GARAGEM, COMO JOGAR, SAIR */

/* O que as telas de menu precisam lembrar entre um quadro e outro. */
typedef struct {
    int opcaoMenu;                              /* item marcado no menu principal */
    float destaque[TOTAL_OPCOES];               /* 0 a 1: quanto cada opção já "acendeu" (animação) */
    int carroEscolhido;                         /* número do carro do jogador (0 a 7) */
    Atributo criterio;                          /* por qual atributo a garagem está ordenada */
    const FichaTecnica *garagem[TOTAL_CARROS];  /* as fichas, na ordem do Merge Sort */
    int selecionado;                            /* linha marcada na garagem */
    float destaqueLinha[TOTAL_CARROS];          /* animação (0 a 1) de cada linha da garagem */
    float destaqueAba[TOTAL_ATRIBUTOS];         /* animação (0 a 1) de cada aba de critério */
    int abaAnterior;                            /* aba sob o mouse no quadro anterior (-1 = nenhuma) */
    int voltarAnterior;                         /* o mouse estava no VOLTAR no quadro anterior? */
    float destaqueVoltar;                       /* animação (0 a 1) do botão VOLTAR */
} EstadoMenu;

void iniciarMenu(EstadoMenu *menu);

Tela atualizarMenuPrincipal(EstadoMenu *menu);
void desenharMenuPrincipal(EstadoMenu *menu);

Tela atualizarGaragem(EstadoMenu *menu);
void desenharGaragem(EstadoMenu *menu);

Tela atualizarComoJogar(void);
void desenharComoJogar(void);

Tela atualizarResultado(void);
void desenharResultado(Corrida *corrida);

#endif
