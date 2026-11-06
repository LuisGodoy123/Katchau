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

void carregarPosProcesso(void);
void descarregarPosProcesso(void);
void comecarPosProcesso(void);
void comecarLuzesDoPosProcesso(void);
void terminarPosProcesso(int pista, float desfoque, int completo);

#endif
