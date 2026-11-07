/*
 * efeitos.h
 * ------------------------------------------------------------------
 * Os sons do carro e da corrida, ligados ao que está acontecendo:
 *
 *   - o motor estilo NASCAR do jogador: loops gravados em várias rotações; o jogo
 *     mistura os mais próximos da rotação do carro e ajusta o tom, e passa do loop
 *     "acelerando" para o "aliviando" quando o piloto levanta o pé;
 *   - o motor dos rivais mais próximos, com volume pela distância, lado (esquerda ou
 *     direita) e efeito Doppler quando passam por você;
 *   - o vento (cresce com a velocidade), o pneu cantando (derrapagem, drift, freada
 *     forte), a grama, o muro raspando e o nitro queimando;
 *   - os efeitos curtos: troca de marcha (estouros do escapamento), nitro ligando e
 *     apagando, volta completa, melhor volta e o pit stop (aviso, pistola das rodas e
 *     "pode ir").
 *
 * O motor do carro do jogador é a gravação recursos/sons/efeitos/nitro_car.mp3, tocada pelo
 * motorgranular.c: o tom segue a rotação do jogo e cada marcha usa a sua arrancada da gravação.
 *
 * Os outros arquivos são WAV em recursos/sons/nascar e recursos/sons/efeitos. Dá para trocar
 * qualquer um por outra gravação com o mesmo nome. Se um arquivo faltar, só aquele som
 * fica mudo.
 *
 * Módulo que usa a raylib (parte de áudio).
 * ------------------------------------------------------------------
 */

#ifndef EFEITOS_H
#define EFEITOS_H

#include "corrida.h"

void carregarEfeitos(void);
void iniciarEfeitosDaCorrida(void);
void atualizarEfeitosDaCorrida(Corrida *corrida, int ativo, float tempoQuadro);
void pausarEfeitosDaCorrida(int pausar);
void descarregarEfeitos(void);
void tocarSomDeFogos(int explosao, float volume, float lado);

#endif
