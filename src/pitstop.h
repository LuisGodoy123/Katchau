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

#define SEGUNDOS_PARA_A_RODA   0.20f   /* o Guido indo de uma roda para a outra */
#define SEGUNDOS_POR_RODA      0.55f   /* trocando uma roda */
#define CANTOS_DO_CARRO        4       /* frente esquerda, trás esquerda, trás direita, frente direita */
#define DURACAO_DA_TROCA       ((CANTOS_DO_CARRO + 1) * SEGUNDOS_PARA_A_RODA + CANTOS_DO_CARRO * SEGUNDOS_POR_RODA)
#define METROS_DA_VAGA_AO_PREDIO 4.5f  /* o carro para com o centro a essa distância do prédio */
#define FIM_DA_ZONA_DE_ENTRADA 3600.0f /* metros: até aqui ainda dá para entrar no pit lane */
#define MEIO_COMPRIMENTO_DO_GUIDO 1.2f /* metros */
#define MEIA_LARGURA_DO_GUIDO  0.58f
#define LIMITE_DO_PIT          4000.0f /* unidades por segundo: 100 km/h, perto das garagens */
#define INICIO_DO_LIMITE_DO_PIT 3990.0f /* metros: a linha do limite, antes da primeira garagem */
#define FIM_DO_LIMITE_DO_PIT   160.0f   /* e a outra, depois da última */
#define INICIO_DA_ENTRADA_LIVRE 3490.0f /* antes disso o pit lane ainda está abrindo */
#define METROS_PARA_ENTRAR     3.0f    /* ele entra com o carro essa distância dentro do pit lane: ninguém entra sem querer */

/* Onde o Guido está, em relação ao carro parado: x para a esquerda, z para a frente */
typedef struct {
    float x, z;
    float olha;        /* radianos: para onde a frente dele aponta (0 = frente do carro, positivo = esquerda) */
} LugarDoGuido;

int carroNoPit(const Carro *carro);
float metrosNaVolta(const Carro *carro);
float metroDaVagaDoCarro(int numero);
void lugarDaVaga(Pista *pista, const Carro *carro, PosicaoNaPista *lugar, float *lateral);
int querEntrarNoPit(const Pista *pista, const Carro *carro);
void entrarNoPit(Carro *carro);
int atualizarCarroNoPit(Carro *carro, Pista *pista, float tempoQuadro);
int faseDaTroca(const Carro *carro, int *canto, float *andamento);
LugarDoGuido lugarDoGuido(const Carro *carro, float meiaLargura, float meioComprimento, float zDaFrente, float zDeTras);

#endif
