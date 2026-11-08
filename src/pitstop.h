/*
 * pitstop.h
 * ------------------------------------------------------------------
 * O pitstop: quem entra no pit lane segue sozinho, no limite de velocidade,
 * até a vaga da sua equipe. Lá o Guido passa por todas as rodas, e o carro
 * volta para a pista pela saída do pit lane.
 *
 * Este módulo NÃO usa a raylib: o desenho do Guido fica em render.c.
 * ------------------------------------------------------------------
 */

#ifndef PITSTOP_H
#define PITSTOP_H

#include "pista.h"
#include "carro.h"

#define SEGUNDOS_PARA_A_RODA   0.22f   /* o Guido indo de uma roda para a outra */
#define SEGUNDOS_POR_RODA      0.62f   /* trocando uma roda: arranca a velha, a nova cai e ele encaixa */
#define CANTOS_DO_CARRO        4       /* frente esquerda, trás esquerda, trás direita, frente direita */
#define DURACAO_DA_TROCA       ((CANTOS_DO_CARRO + 1) * SEGUNDOS_PARA_A_RODA + CANTOS_DO_CARRO * SEGUNDOS_POR_RODA)
#define MEIO_COMPRIMENTO_DO_GUIDO 1.2f /* metros */
#define MEIA_LARGURA_DO_GUIDO  0.58f
#define LIMITE_DO_PIT          4000.0f /* unidades por segundo: 100 km/h, perto das garagens */

/* Onde o Guido está, em relação ao carro parado: x para a esquerda, z para a frente */
typedef struct {
    float x, z;
    float olha;        /* radianos: para onde a frente dele aponta (0 = frente do carro, positivo = esquerda) */
} LugarDoGuido;

/* A coreografia, como no filme: o Guido chega com os quatro pneus novos nos garfos, joga
 * os quatro para cima e, enquanto eles caem, arranca cada velho; cada novo cai na sua roda
 * quando ele chega nela. Em frações da troca de cada roda: */
#define ARRANCOU        0.30f    /* o pneu velho saiu do cubo, voando */
#define ENCAIXOU        0.70f    /* o novo, caindo do alto, encaixou no cubo */
#define PUXADA          0.45f    /* metros que o Guido recua arrancando */
#define LANCOU          0.14f    /* segundos: os garfos jogam os quatro pneus para cima */

int carroNoPit(const Carro *carro);
float metrosNaVolta(const Carro *carro);
float metroDaVagaDoCarro(const Pista *pista, int numero);
int dentroDoLimiteDoPit(const Pista *pista, float metros);
float linhaNoPit(const Pista *pista, const Carro *carro, int saindo, float metrosPorSegundoAgora);
void lugarDaVaga(Pista *pista, const Carro *carro, PosicaoNaPista *lugar, float *lateral);
int querEntrarNoPit(const Pista *pista, const Carro *carro);
void entrarNoPit(Carro *carro);
int atualizarCarroNoPit(Carro *carro, Pista *pista, float tempoQuadro);
int faseDaTroca(const Carro *carro, int *canto, float *andamento);
LugarDoGuido poseDoGuido(const Carro *carro, float meiaLargura, float meioComprimento, float zDaFrente, float zDeTras);
float entradaDoGuidoNoCarro(LugarDoGuido guido, float meiaLargura, float meioComprimento);
LugarDoGuido lugarDoGuido(const Carro *carro, float meiaLargura, float meioComprimento, float zDaFrente, float zDeTras);

#endif
