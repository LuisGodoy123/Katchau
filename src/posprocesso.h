/*
 * posprocesso.h
 * ------------------------------------------------------------------
 * Os efeitos de tela: a cena 3D é desenhada numa imagem e passa por um
 * último shader antes de ir para a tela: brilho em volta das luzes (bloom),
 * correção de cor de cada pista, vinheta e desfoque de velocidade.
 *
 * A imagem da cena guarda também a profundidade (quantos metros até cada
 * pixel) e, com as cores HDR ligadas, a luz acima do branco. Daí saem a
 * neblina da distância, a profundidade de campo de verdade, os raios de sol
 * e o reflexo do sol na lente; a luz acima do branco brilha (bloom) e uma
 * curva de cinema a suaviza até o branco.
 * ------------------------------------------------------------------
 */

#ifndef POSPROCESSO_H
#define POSPROCESSO_H

#include "raylib.h"

/* O que o shader final faz com a cena. Na tela dividida o mundo é desenhado duas vezes, então o
 * halo (que é um desenho do mundo a mais, com quatro borrões) é o primeiro a sair; a cor da pista
 * e a vinheta ficam, porque são um passe só e são metade do visual do jogo. */
typedef enum {
    POSPROCESSO_SO_COPIA,   /* efeitos desligados nas configurações: a cena vai crua para a tela */
    POSPROCESSO_SEM_HALO,   /* cor, contraste, vinheta e desfoque; sem brilho */
    POSPROCESSO_COMPLETO
} AcabamentoDoPosProcesso;

/* O que um quadro pede além do tom da pista. Quem desenha preenche antes de terminarPosProcesso
 * (definirCinemaDoQuadro); sem isso, o quadro sai sem neblina, sem sol e sem foco. */
typedef struct {
    Camera3D camera;          /* a do quadro: a profundidade vira metros e o sol vira um ponto na tela */
    /* A neblina da distância (a perspectiva do ar: o longe vai ficando da cor do horizonte) */
    Vector3 corDaNeblina;     /* 0 a 1 */
    float neblina;            /* densidade por metro, no chão (0: sem neblina) */
    float baseDaNeblina;      /* metros: a altura onde ela é mais densa */
    float alturaDaNeblina;    /* metros: subindo tanto, ela fica e (2,7) vezes mais rala */
    float maximoDaNeblina;    /* 0 a 1: o máximo que ela cobre (o longe ainda aparece) */
    /* O sol: os raios de luz pelas frestas e o reflexo na lente */
    Vector3 sol;              /* de onde vem a luz (normalizado) */
    Vector3 corDoSol;         /* 0 a 1 */
    float raiosDeSol;         /* 0 a 1: a força dos raios (0: nenhum) */
    float reflexoDoSol;       /* 0 a 1: a força do reflexo na lente (0: nenhum) */
    /* A profundidade de campo das cenas: o foco em quem importa */
    float foco;               /* metros da câmera até o que está em foco (0: sem a de cena) */
    float abertura;           /* 0 a 1: o quanto o que está fora de foco borra */
    /* A cor do quadro, no lugar do tom da pista (o passeio muda com a hora) */
    int temCor;
    Vector3 tom;
    float contraste, saturacao, vinheta;
} CinemaDoQuadro;

void carregarPosProcesso(void);
void descarregarPosProcesso(void);
/* O pedaço da tela em que a cena vai aparecer. Na tela dividida, cada metade desenha numa imagem
 * DO TAMANHO DELA, e não num pedaço de uma imagem da tela toda: assim a vinheta e o desfoque de
 * velocidade saem no meio da metade (e não no meio da tela, que é a linha da divisão), o halo tem
 * o mesmo tamanho de sempre em relação ao que se vê, e a luz de um jogador não vaza para a metade
 * do outro. Largura 0 volta para a tela cheia. */
void definirAreaDoPosProcesso(int x, int y, int largura, int altura);
void comecarPosProcesso(void);
void comecarLuzesDoPosProcesso(void);
void comecarLuzesComProfundidade(void);   /* as luzes tapadas pelo que está na frente (o passeio) */
/* A neblina de cada pista, para as corridas e os menus (o passeio monta a dele com a hora) */
void neblinaDaPista(int pista, CinemaDoQuadro *cinema);
void definirCinemaDoQuadro(const CinemaDoQuadro *cinema);   /* vale só para o próximo terminarPosProcesso */
void terminarPosProcesso(int pista, float desfoque, AcabamentoDoPosProcesso acabamento);

#endif
