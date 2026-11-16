/*
 * passeio.h
 * ------------------------------------------------------------------
 * O mundo aberto de Radiator Springs (a cidade, o Willy's Butte, os
 * cânions e a arena do Cars 2), onde o jogo acontece: o carro escolhido
 * na garagem anda à vontade, sem voltas, sem rivais e sem relógio.
 *
 * As corridas começam no Mack, estacionado na rua principal: chegando
 * devagar atrás da carreta, abre a escolha da corrida (TELA_PISTAS);
 * escolhida, o carro entra na carreta e o Mack parte com ele até a
 * pista. No fim da corrida, o carro desce da carreta de volta na cidade.
 *
 * A física fica em livre.c e colisao.c; aqui ficam o Mack, a câmera, o
 * desenho, o HUD, a pausa e os sons.
 * ------------------------------------------------------------------
 */

#ifndef PASSEIO_H
#define PASSEIO_H

#include "menu.h"
#include "vibracao.h"

void iniciarPasseio(int numeroDoCarro);
Tela atualizarPasseio(EstadoMenu *menu, float tempoQuadro);
void desenharPasseio(EstadoMenu *menu);
int passeioPausado(void);
Vibracao vibracaoDoPasseio(void);   /* a do quadro em que o carro andou (e só nele: depois, nada) */
void descarregarPasseio(void);

/* As corridas, pelo Mack */
void voltarDaEscolhaDaCorrida(void);                 /* desistiu na escolha: o carro fica onde estava */
void partirComOMack(const char *nomeDaPista);        /* escolheu: o carro entra na carreta e o Mack parte */
void voltarDaCorrida(int numeroDoCarro, const char *recado);   /* o carro desce da carreta, na cidade */
int cenaDoMackComMusica(void);                       /* 1 embarcando e na partida do Mack (a música toca) */
void sairDaLuzDoPasseio(void);                       /* fora dele, a luz de sempre e nenhuma lâmpada */

/* O benchmark (--benchmark): um percurso fixo pela cidade, gravado quadro a quadro (desempenho.c) */
void comecarBenchmarkDoPasseio(void);
int andarNoBenchmark(float tempoQuadro);              /* depois de atualizarPasseio; 0 quando terminou */

#endif
