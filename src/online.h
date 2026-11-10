/*
 * online.h
 * ------------------------------------------------------------------
 * A corrida em rede: o que viaja entre as máquinas durante a prova.
 *
 * A divisão de trabalho é a mesma de qualquer jogo de corrida online:
 *
 *   O DONO roda a física de verdade. Ele recebe os comandos dos convidados,
 *   põe cada um no carro certo (corrida.c faz isso), e 30 vezes por segundo
 *   manda um RETRATO dos 14 carros.
 *
 *   O CONVIDADO não espera o retrato para dirigir: ele roda a física do
 *   próprio carro todo quadro, com o próprio teclado — se esperasse, o
 *   volante teria o atraso da rede. Quando o retrato chega, ele corrige o
 *   próprio carro de leve na direção do que o dono disse, e põe os outros 13
 *   onde o dono mandou, interpolando para não tremerem.
 *
 * Duas coisas aqui não são detalhe, são requisito:
 *
 *   1) A tabela de segmentos. buscarSegmento anda a lista circular da pista
 *      desde a largada, e Interlagos tem milhares de segmentos: 14 carros a
 *      30 Hz derrubariam o FPS. A tabela de ponteiros é montada uma vez.
 *
 *   2) voltas vem SEMPRE do dono. A contagem de volta do jogo mora dentro de
 *      moverNaPista e de andarNoMundo, e as duas contam "andando": escrever
 *      uma posição distante faz andarNoMundo achar que o carro cruzou a linha
 *      e dar uma volta que não existiu. Por isso o número do dono é escrito
 *      por cima, DEPOIS da física local.
 *
 * Este módulo NÃO usa a raylib.
 * ------------------------------------------------------------------
 */

#ifndef ONLINE_H
#define ONLINE_H

#include "corrida.h"
#include "pista.h"
#include "protocolo.h"
#include "rede.h"

void prepararOnline(Corrida *corrida, Pista *pista);
void encerrarOnline(void);

/* Chamada por sala.c, que é quem tem o soquete: os pacotes da corrida chegam
 * misturados com os da sala, e é lá que a caixa de entrada é esvaziada. */
void receberPacoteDaCorrida(const unsigned char *fio, int tamanho,
                            const CabecalhoDeRede *cabecalho, const EnderecoDaRede *de,
                            Corrida *corrida);

/* Uma vez por quadro, DEPOIS de atualizarCorrida: o convidado aplica o retrato e
 * manda os comandos; o dono manda o retrato. */
void atualizarOnline(Corrida *corrida, Comandos meusComandos, float tempoQuadro);

float segundosSemRetrato(void);

#endif
