#ifndef VISUAL_H
#define VISUAL_H

/*
 * visual.h
 * ------------------------------------------------------------------
 * A identidade visual do KATCHAU, usada pelos menus e pela tela de
 * chegada: as cores da logo (vermelho, amarelo, cromado e o preto do
 * quadriculado), as duas fontes e as formas "de corrida" (painéis
 * inclinados, faixa quadriculada, barras em blocos e teclas).
 *
 * Módulo de desenho: usa a raylib.
 * ------------------------------------------------------------------
 */

#include "raylib.h"

/* ---------- Cores (tiradas da logo) ---------- */
#define COR_VERMELHO         (Color){ 214,  28,  38, 255 }
#define COR_VERMELHO_ESCURO  (Color){ 110,  10,  18, 255 }
#define COR_AMARELO          (Color){ 255, 200,  20, 255 }
#define COR_CROMO            (Color){ 205, 210, 220, 255 }
#define COR_GRAFITE          (Color){  16,  18,  24, 255 }
#define COR_PAINEL           (Color){  12,  14,  20, 215 }
#define COR_TEXTO_CLARO      (Color){ 242, 243, 246, 255 }
#define COR_TEXTO_FRACO      (Color){ 150, 156, 170, 255 }
#define COR_OURO             (Color){ 255, 200,  20, 255 }
#define COR_PRATA            (Color){ 196, 202, 214, 255 }
#define COR_BRONZE           (Color){ 205, 127,  50, 255 }

/* As três letras do jogo */
typedef enum {
    FONTE_TITULO,     /* Racing Sans One: títulos, opções do menu, números grandes */
    FONTE_TEXTO,      /* Saira Condensed SemiBold: textos comuns */
    FONTE_FORTE,      /* Saira Condensed Bold: rótulos, valores, teclas */
    TOTAL_FONTES
} TipoFonte;

void carregarVisual(void);
void descarregarVisual(void);

/* ---------- Texto (com sombra) ---------- */
float larguraDoTexto(TipoFonte fonte, const char *texto, float tamanho);
void escrever(TipoFonte fonte, const char *texto, float x, float y, float tamanho, Color cor);
void escreverCentralizado(TipoFonte fonte, const char *texto, float centroX, float y, float tamanho, Color cor);
void escreverADireita(TipoFonte fonte, const char *texto, float direita, float y, float tamanho, Color cor);

/* ---------- Formas ---------- */
void desenharParalelogramo(float x, float y, float largura, float altura, float inclinacao, Color cor);
void desenharContornoParalelogramo(float x, float y, float largura, float altura, float inclinacao,
                                   float espessura, Color cor);
void desenharQuadriculado(float x, float y, float largura, float altura, float lado);
void desenharBarraDeBlocos(float x, float y, float largura, float altura, float fracao, Color cor);
float larguraDaTecla(const char *tecla);
float desenharTecla(float x, float y, const char *tecla);
void desenharLogo(float centroX, float y, float largura);
void desenharBrilho(float centroX, float centroY, float raio, Color cor);

#endif
