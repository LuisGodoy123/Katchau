/*
 * passeio.h
 * ------------------------------------------------------------------
 * A tela do passeio livre: o carro escolhido na garagem, sozinho, no mundo
 * aberto de Radiator Springs (a cidade, o Willy's Butte, os cânions e a
 * arena do Cars 2). Sem voltas, sem rivais e sem relógio.
 *
 * A física fica em livre.c e colisao.c; aqui ficam a câmera, o desenho,
 * o HUD, a pausa e os sons.
 * ------------------------------------------------------------------
 */

#ifndef PASSEIO_H
#define PASSEIO_H

#include "menu.h"

void iniciarPasseio(int numeroDoCarro);
Tela atualizarPasseio(EstadoMenu *menu, float tempoQuadro);
void desenharPasseio(EstadoMenu *menu);
int passeioPausado(void);
void descarregarPasseio(void);

#endif
