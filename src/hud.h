/*
 * hud.h
 * ------------------------------------------------------------------
 * Informações por cima da corrida, nos quatro cantos da tela:
 *   - em cima à esquerda: a tabela de posições (bandeira, tempo de prova,
 *     intervalo de cada carro para o jogador e a última volta);
 *   - em cima à direita: a volta do jogador (diferença para a melhor
 *     volta, tempos e os 3 setores coloridos, como no F1);
 *   - embaixo à esquerda: o mini-mapa; embaixo à direita: o painel do motor
 *     (velocidade, conta-giros e marcha).
 * E também a contagem regressiva e o painel de chegada.
 * ------------------------------------------------------------------
 */

#ifndef HUD_H
#define HUD_H

#include "corrida.h"
#include "controle.h"   /* a placa de controles da largada diz as teclas DAQUELE jogador */

void desenharHudDoJogador(Corrida *corrida, int humano);   /* na tela dividida, um por faixa */
void desenharPlacaDeControles(Corrida *corrida, int humano, const EntradaDoJogador *entrada);
void desenharPainelDoMotor(const Carro *carro, int kmh);   /* o velocímetro da corrida, sozinho (o mundo aberto usa) */
const char *codigoDoCarro(int numero);
Color corDoCarro(int numero);
void desenharFimDeCorridaDoJogador(Corrida *corrida, int humano);
void desenharAvisoParaContinuar(Corrida *corrida);
void formatarTempo(float segundos, char *texto, int tamanho);

/* A classificação da tela dividida: uma tira sobre a linha que separa as metades, desenhada UMA vez
 * por quadro (a torre de 17 linhas em cada faixa deixava o texto pequeno demais). A altura vem em
 * pixels de verdade porque o main.c usa metade dela para recuar o HUD de cada faixa. */
float alturaDaFaixaDeClassificacao(void);
void desenharFaixaDeClassificacao(Corrida *corrida);

/* Tela virtual de 800x600 para o HUD e os menus (veja hud.c). A ESCALA é sempre a da tela; na tela
 * dividida, a área é a faixa de cada jogador e muda só a origem e a extensão. */
void definirAreaDaInterface(int x, int y, int largura, int altura);   /* largura 0 = a tela cheia */
void comecarInterface(int centralizada);
void terminarInterface(void);
void posicaoDoMouseNaInterface(float *x, float *y);
int larguraVirtual(void);
int alturaVirtual(void);

#endif
