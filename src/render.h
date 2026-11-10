/*
 * render.h
 * ------------------------------------------------------------------
 * Desenha a corrida em 3D, vista de trás do carro do jogador: o cenário
 * (mundo.c), os carros (carros3d.c) e a fumaça (fumaca.c). A câmera anda pela lista circular da pista.
 *
 * Cada jogador tem uma VisaoDoJogador: a câmera dele e o pedaço de tela dele. Num jogador é uma
 * visão na tela inteira; na tela dividida são duas, uma em cima e outra embaixo. O que ANDA com o
 * tempo do mundo (a fumaça, a torcida, a noite, os telões) fica em avancarMundoDaCorrida, chamado
 * uma vez por quadro — o desenho em si roda uma vez por visão.
 *
 * Este é um dos poucos módulos que chamam a raylib.
 * ------------------------------------------------------------------
 */

#ifndef RENDER_H
#define RENDER_H

#include "corrida.h"
#include "olhar.h"

#define TOTAL_DISTANCIAS_DA_CAMERA 3   /* perto, médio e longe */
#define MAXIMO_DE_VISOES  MAXIMO_DE_JOGADORES_LOCAIS   /* uma visão por jogador do sofá */

/* Um pedaço da tela, em pixels de verdade. Na tela inteira é 0, 0 e o tamanho da janela. */
typedef struct {
    int x, y, largura, altura;
} Janela;

/* Uma visão da corrida: a câmera de um jogador e o pedaço de tela dele. O que era estático em
 * render.c (o olhar, a distância, o rumo da câmera, a respiração) virou campo daqui, para duas
 * visões no mesmo quadro não se atrapalharem: com uma variável só, a câmera de um jogador
 * puxaria a do outro a cada quadro. */
typedef struct {
    int humano;                 /* qual jogador (índice em corrida->humanos) */
    Janela janela;              /* onde esta visão aparece na tela */
    Olhar olhar;                /* para onde este jogador está olhando (analógico direito) */
    int distanciaEscolhida;     /* perto, médio ou longe */
    float atras, altura;        /* a distância de agora, andando até a escolhida */
    float respiracao;           /* a câmera respirando com o pedal */
    float rumoDaCamera;         /* o rumo que ela segue, com atraso */
    int cameraComRumo;          /* 1 se ela já está seguindo um rumo */

    /* Preenchidos por atualizarVisao, usados por avancarMundoDaCorrida e desenharVisao */
    Camera3D camera;
    int festa;                  /* 1 na festa do pódio (só com um jogador) */
    int cortou;                 /* 1 no quadro em que a câmera da festa trocou de tomada */
    float escuro;               /* o escurecimento do corte (0 a 1) */
    float desdeAChegada;        /* segundos desde a bandeirada deste jogador */
} VisaoDoJogador;

void iniciarVisao(VisaoDoJogador *visao, int humano);
void repartirJanelas(int totalDeVisoes, Janela janelas[]);
void atualizarVisao(Corrida *corrida, VisaoDoJogador *visao);
void avancarMundoDaCorrida(Corrida *corrida, const VisaoDoJogador *principal);
void desenharVisao(Corrida *corrida, const VisaoDoJogador *visao);
int alternarDistanciaDaVisao(VisaoDoJogador *visao);
void desenharRiscoDaDivisao(int totalDeVisoes);

void desenharCorridaVistaDe(Corrida *corrida, Camera3D camera);
Camera3D cameraDaVisaoDoJogador(Corrida *corrida);   /* a do jogador 1: a apresentação termina nela */
void congelarCena(int pausada);
void desenharPistaDeFundo(Pista *pista);
void desenharFundoAlternado(Pista *pistas[]);
void desenharCarroParado(int numero, float centroX, float baseY, float escala);
Texture2D fotografarVitrine(int numero, float giro, Vector2 *chao);

#endif
