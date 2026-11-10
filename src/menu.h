#ifndef MENU_H
#define MENU_H

/*
 * Menu: menu principal, garagem, escolha da pista, como jogar e tela de resultado (fase 6).
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
    TELA_ENTRADA,      /* "APERTE QUALQUER BOTÃO PARA ENTRAR", com o vídeo de entrada em loop */
    TELA_MENU,
    TELA_GARAGEM,
    TELA_PISTAS,
    TELA_COMO_JOGAR,
    TELA_CONFIGURACOES,
    TELA_CORRIDA,
    TELA_RESULTADO,
    TELA_PASSEIO,      /* o passeio livre pelo mundo aberto (passeio.c) */
    TELA_SAIR
} Tela;

#define TOTAL_OPCOES 5   /* JOGAR, GARAGEM, COMO JOGAR, CONFIGURAÇÕES, SAIR */
#define TOTAL_OPCOES_PAUSA 2   /* CONTINUAR e sair (MENU PRINCIPAL na cidade, VOLTAR À CIDADE na corrida) */

/* O que o jogador escolheu na pausa neste quadro */
typedef enum {
    PAUSA_NADA,        /* ainda escolhendo */
    PAUSA_CONTINUAR,   /* volta para a corrida, de onde parou */
    PAUSA_SAIR         /* sai: da corrida, de volta à cidade; da cidade, ao menu principal */
} EscolhaDaPausa;

/* O que as telas de menu precisam lembrar entre um quadro e outro. */
typedef struct {
    int opcaoMenu;                              /* item marcado no menu principal */
    float destaque[TOTAL_OPCOES];               /* 0 a 1: quanto cada opção já "acendeu" (animação) */
    int carroEscolhido;                         /* número do carro do jogador */
    Atributo criterio;                          /* por qual atributo a garagem está ordenada */
    const FichaTecnica *garagem[TOTAL_CARROS];  /* as fichas, na ordem do Merge Sort */
    int selecionado;                            /* linha marcada na garagem */
    float destaqueLinha[TOTAL_CARROS];          /* animação (0 a 1) de cada linha da garagem */
    float destaqueAba[TOTAL_ATRIBUTOS];         /* animação (0 a 1) de cada aba de critério */
    int abaAnterior;                            /* aba sob o mouse no quadro anterior (-1 = nenhuma) */
    int voltarAnterior;                         /* o mouse estava no VOLTAR no quadro anterior? */
    float destaqueVoltar;                       /* animação (0 a 1) do botão VOLTAR */
    float rolagemDaVitrine;                     /* o carrossel da garagem, andando até o carro marcado */
    float giroDaVitrine;                        /* o carro da vitrine girando (radianos) */
    float destaqueOrdem;                        /* animação (0 a 1) do seletor de ordem */
    int opcaoPausa;                             /* item marcado na pausa */
    const char *saidaDaPausa;                   /* o texto do segundo botão da pausa (abrirPausa põe MENU PRINCIPAL) */
    float destaquePausa[TOTAL_OPCOES_PAUSA];    /* animação (0 a 1) de cada botão da pausa */
    int abaConfig;                              /* a aba das configurações: 0 = controles, 1 = exibição */
    float segundosParaAplicar;                  /* > 0: a resolução ou o modo de tela mudou e a janela muda quando chegar a zero */
    int linhaConfig;                            /* a linha marcada nas configurações */
    int colunaConfig;                           /* a coluna marcada, nas linhas de comandos */
    int esperandoComando;                       /* 1: esperando apertar a tecla ou o botão novo */
    float tempoEsperando;                       /* há quanto tempo está esperando */
    float confirmarPadrao;                      /* segundos para confirmar a volta ao padrão (0 = não pediu) */
    float pulsoDeVibracao;                      /* segundos do toque de teste da vibração */
    int pistaEscolhida;                         /* em qual pista correr (IdPista) */
    NivelDaCorrida nivelEscolhido;              /* fácil, médio ou difícil */
    float destaqueNivel[TOTAL_NIVEIS];          /* animação (0 a 1) de cada botão de nível */
    int focoDaCorrida;                          /* a linha marcada na tela da corrida: pista, nível ou CORRER */
    float destaqueCorrer;                       /* animação (0 a 1) do botão CORRER */
    float destaquePista[TOTAL_PISTAS];          /* animação (0 a 1) de cada cartão de pista */

    /* ---- A tela dividida, escolhida no Mack ---- */
    int telaDividida;                           /* 1: a próxima corrida é de dois no mesmo aparelho */
    int carroDoJogador2;                        /* o carro do jogador 2 (sempre != carroEscolhido) */
    float destaqueDividida;                     /* animação (0 a 1) do botão da tela dividida */
    float destaqueCarro2;                       /* animação (0 a 1) da linha do carro do jogador 2 */
} EstadoMenu;

void iniciarMenu(EstadoMenu *menu);

Tela atualizarTelaDeEntrada(void);
void desenharTelaDeEntrada(float tempoQuadro);

Tela atualizarMenuPrincipal(EstadoMenu *menu);
void desenharMenuPrincipal(EstadoMenu *menu);

Tela atualizarGaragem(EstadoMenu *menu);
void desenharGaragem(EstadoMenu *menu);

Tela atualizarEscolhaDePista(EstadoMenu *menu);
void desenharEscolhaDePista(EstadoMenu *menu, Pista *pistas[]);

Tela atualizarConfiguracoes(EstadoMenu *menu);
void desenharConfiguracoes(EstadoMenu *menu);

Tela atualizarComoJogar(void);
void desenharComoJogar(void);

void abrirPausa(EstadoMenu *menu);
void abrirPausaDaCorrida(EstadoMenu *menu);   /* a saída é VOLTAR À CIDADE */
EscolhaDaPausa atualizarPausa(EstadoMenu *menu);
void desenharPausa(EstadoMenu *menu);

Tela atualizarResultado(void);
void desenharResultado(Corrida *corrida);

#endif
