/*
 * carros3d.h
 * ------------------------------------------------------------------
 * Os modelos 3D dos carros (recursos/modelos/carros). Cada arquivo vem com
 * a sua escala, a sua origem e o seu eixo: ao carregar, este módulo mede
 * cada um e deixa todos iguais, com o tamanho de verdade em metros, o
 * centro na origem, as rodas no chão e a frente para +z.
 *
 * As rodas são separadas do resto do modelo, para girarem de verdade: as de
 * trás só giram, e as da frente também viram com o volante.
 *
 * Também prepara uma miniatura de cada carro, para a garagem e o menu.
 * ------------------------------------------------------------------
 */

#ifndef CARROS3D_H
#define CARROS3D_H

#include "raylib.h"
#include "gestos.h"

void carregarCarros3D(void);
void descarregarCarros3D(void);
float larguraDoCarro3D(int numero);
float comprimentoDoCarro3D(int numero);
float alturaDoCarro3D(int numero);
#define PONTOS_DA_SOMBRA  7   /* a malha da sombra tem tantos pontos em cada lado */
void desenharSombraNoChao(int numero, const Vector3 *grade);
/* A cara do carro no próximo desenho: aperto (0 a 1, os olhos apertados em alta velocidade)
 * e susto (0 a 1, os olhos arregalados ao rodar). Ela muda aos poucos. */
void expressaoNoProximoDesenho(int numero, float aperto, float susto);
/* As rodas da frente abertas para fora no próximo desenho (0 a 1): as mãos abertas (gestos.c) */
void abrirRodasNoProximoDesenho(float abertura);
/* O quanto a boca abre no próximo desenho (0 a 1; só os modelos com a boca medida, bocas.txt) */
void bocaNoProximoDesenho(int numero, float abertura);
/* A emoção da cara no próximo desenho (os moradores: gestos.h), somada ao aperto e ao susto */
void emocaoNoProximoDesenho(int numero, const Emocao *emocao);
/* Para onde os olhos vão no próximo desenho (-1 a +1), no lugar do volante (as rodas seguem o volante) */
void olharNoProximoDesenho(int numero, float olhar);
void comecarSombrasNoChao(void);
void terminarSombrasNoChao(void);
float ladoDaSombra(int numero);
void desenharSombra(int numero, Matrix noMundo);
int rodaDoCanto(int numero, int canto, Vector3 *centro);
void trocarRodaNoProximoDesenho(int roda, float andamento);
void esconderRodaNoProximoDesenho(int roda);
void pneusEstouradosNoProximoDesenho(int pneus);   /* os bits de Carro.pneuEstourado: sobra o aro e o canto baixa */
void definirOlhoDosCarros(Vector3 olho);

/* Os olhos vivos dos carros num modelo que não é carro (o Mack, em passeio.c) */
int darOlhosAoModelo(Model *modelo, const char *nome);
void prepararOlhosDoModelo(float olhar, float aperto, float susto);
void emocaoDoModelo(const Emocao *emocao);   /* a emoção dele no próximo desenho (antes de prepararOlhosDoModelo) */
/* A boca dele (bocas.txt): a mandíbula gira na própria malha. darBocaAoModelo devolve 0 se ele não está na tabela */
int darBocaAoModelo(Model *modelo, const char *nome);
void abrirBocaDoModelo(float abertura);   /* 0 fechada a 1 aberta */
void descarregarBocaDoModelo(void);
void inclinarCorpoNoProximoDesenho(int numero, float rolagem, float arfagem);
/* O corpo com a suspensão (a arfagem gira no meio) e os gestos (gestos.c: o aceno gira no eixo de trás) */
void posturaNoProximoDesenho(int numero, float rolagem, float arfagem, float aceno, float subida);
void moverGarfosNoProximoDesenho(float subida, float inclinacao);
Vector3 pontaDosGarfos(void);
float raioDaRoda(int numero, int roda);
void desenharRodaSolta(int numero, int roda, Matrix noMundo);
void desenharCarro3D(int numero, Matrix noMundo, float anguloDasRodas, float volante);
void desenharMiniatura(int numero, float centroX, float baseY, float larguraNaTela);
int bordaDaMiniatura(int numero);

#endif
