/*
 * construcao.h
 * ------------------------------------------------------------------
 * Montar malhas 3D aos poucos, triângulo a triângulo, para o cenário
 * (mundo.c, paisagem.c e infraestrutura.c).
 * ------------------------------------------------------------------
 */

#ifndef CONSTRUCAO_H
#define CONSTRUCAO_H

#include "raylib.h"

/* Uma malha em construção: vai crescendo triângulo a triângulo */
typedef struct {
    float *vertices;
    float *texturas;
    unsigned char *cores;
    int total;       /* vértices usados */
    int capacidade;
} Construcao;

void adicionarVertice(Construcao *c, Vector3 p, Vector2 uv, Color cor);
void adicionarQuadrilatero(Construcao *c, Vector3 a, Vector3 b, Vector3 d, Vector3 e,
                           Vector2 ua, Vector2 ub, Vector2 ud, Vector2 ue, Color cor);
Mesh terminarMalha(Construcao *c);
float luzDoSol(Vector3 normal);
void quadIluminado(Construcao *c, Vector3 a, Vector3 b, Vector3 d, Vector3 e, Color cor);
void caixaIluminada(Construcao *c, Vector3 pe, float giro, float largura, float fundo, float altura, Color cor);

#endif
