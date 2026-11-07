/*
 * exibicao.h
 * ------------------------------------------------------------------
 * Põe na tela o que o jogador escolheu nas configurações de exibição:
 *
 *   - o modo de tela: TELA CHEIA (se a resolução é a do monitor, é uma janela sem
 *     bordas do tamanho dele, então o Alt+Tab funciona sem travar; se é menor,
 *     o monitor muda para ela) ou JANELA, centralizada no monitor;
 *   - a resolução, de 800x600 até 1920x1080 (nunca mais que o monitor tem);
 *   - a sincronia vertical e o limite de quadros por segundo.
 *
 * As escolhas são lidas de Configuracoes (configuracoes.h). A interface e o mundo 3D
 * se ajustam sozinhos ao tamanho da janela, então qualquer resolução serve.
 *
 * Módulo que usa a raylib.
 * ------------------------------------------------------------------
 */

#ifndef EXIBICAO_H
#define EXIBICAO_H

#include "configuracoes.h"

void aplicarDesempenho(const Configuracoes *config);
void aplicarTela(const Configuracoes *config);
void tamanhoDoMonitor(int *largura, int *altura);
int resolucaoCabeNoMonitor(int indice);

#endif
