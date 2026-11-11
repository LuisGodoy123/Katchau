/*
 * posprocesso.h
 * ------------------------------------------------------------------
 * Os efeitos de tela: a cena 3D é desenhada numa imagem e passa por um
 * último shader antes de ir para a tela: brilho em volta das luzes (bloom),
 * correção de cor de cada pista, vinheta e desfoque de velocidade.
 * ------------------------------------------------------------------
 */

#ifndef POSPROCESSO_H
#define POSPROCESSO_H

/* O que o shader final faz com a cena. Na tela dividida o mundo é desenhado duas vezes, então o
 * halo (que é um desenho do mundo a mais, com quatro borrões) é o primeiro a sair; a cor da pista
 * e a vinheta ficam, porque são um passe só e são metade do visual do jogo. */
typedef enum {
    POSPROCESSO_SO_COPIA,   /* efeitos desligados nas configurações: a cena vai crua para a tela */
    POSPROCESSO_SEM_HALO,   /* cor, contraste, vinheta e desfoque; sem brilho */
    POSPROCESSO_COMPLETO
} AcabamentoDoPosProcesso;

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
void terminarPosProcesso(int pista, float desfoque, AcabamentoDoPosProcesso acabamento);

#endif
