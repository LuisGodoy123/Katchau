/*
 * espera.h
 * ------------------------------------------------------------------
 * A tela de carregamento: a logo, o que está sendo carregado e uma barra.
 * Sem ela, o jogo ficava com a janela preta (ou branca) até carregar tudo.
 *
 * Como usar: abrir a janela, chamar quadroPretoDaEspera() e, entre as
 * etapas do carregamento, passoDaEspera(texto, fracao). Uma etapa longa
 * pode reservar um pedaço da barra com faixaDaEspera(de, ate) e daí em
 * diante informar só o andamento dela (0 a 1).
 *
 * Módulo de desenho: usa a raylib.
 * ------------------------------------------------------------------
 */

#ifndef ESPERA_H
#define ESPERA_H

void quadroPretoDaEspera(void);
void faixaDaEspera(float de, float ate);
void passoDaEspera(const char *texto, float fracaoDaFaixa);

#endif
