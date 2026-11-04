/*
 * construcao.c
 * ------------------------------------------------------------------
 * Montar malhas 3D aos poucos. Veja construcao.h.
 * ------------------------------------------------------------------
 */

#include <math.h>

#include "raylib.h"
#include "raymath.h"
#include "construcao.h"

/*
 * adicionarVertice
 * Põe um vértice na malha em construção, dobrando o espaço quando acaba.
 * Recebe: a construção, a posição, a coordenada de textura e a cor.
 * Devolve: nada.
 */
void adicionarVertice(Construcao *c, Vector3 p, Vector2 uv, Color cor) {
    if (c->total == c->capacidade) {
        c->capacidade = (c->capacidade == 0) ? 1024 : c->capacidade * 2;
        c->vertices = MemRealloc(c->vertices, c->capacidade * 3 * sizeof(float));
        c->texturas = MemRealloc(c->texturas, c->capacidade * 2 * sizeof(float));
        c->cores = MemRealloc(c->cores, c->capacidade * 4);
    }
    int i = c->total++;
    c->vertices[i * 3] = p.x;
    c->vertices[i * 3 + 1] = p.y;
    c->vertices[i * 3 + 2] = p.z;
    c->texturas[i * 2] = uv.x;
    c->texturas[i * 2 + 1] = uv.y;
    c->cores[i * 4] = cor.r;
    c->cores[i * 4 + 1] = cor.g;
    c->cores[i * 4 + 2] = cor.b;
    c->cores[i * 4 + 3] = cor.a;
}

/*
 * adicionarQuadrilatero
 * Um quadrilátero, em dois triângulos. Os vértices vêm em volta dele.
 * Recebe: a construção, os 4 pontos, os 4 pares de textura e a cor.
 * Devolve: nada.
 */
void adicionarQuadrilatero(Construcao *c, Vector3 a, Vector3 b, Vector3 d, Vector3 e,
                                  Vector2 ua, Vector2 ub, Vector2 ud, Vector2 ue, Color cor) {
    adicionarVertice(c, a, ua, cor);
    adicionarVertice(c, b, ub, cor);
    adicionarVertice(c, d, ud, cor);
    adicionarVertice(c, a, ua, cor);
    adicionarVertice(c, d, ud, cor);
    adicionarVertice(c, e, ue, cor);
}

/*
 * terminarMalha
 * Envia a malha para a placa de vídeo. A Mesh passa a ser dona dos vetores.
 * Recebe: a construção.
 * Devolve: a malha pronta.
 */
Mesh terminarMalha(Construcao *c) {
    Mesh malha = { 0 };
    malha.vertexCount = c->total;
    malha.triangleCount = c->total / 3;
    malha.vertices = c->vertices;
    malha.texcoords = c->texturas;
    malha.colors = c->cores;
    UploadMesh(&malha, false);
    return malha;
}

/*
 * luzDoSol
 * Quanto uma face clareia conforme olha para o sol. Os objetos não mudam
 * de lugar, então a luz é calculada uma vez, na construção.
 * Recebe: a direção para onde a face aponta.
 * Devolve: o fator de luz.
 */
float luzDoSol(Vector3 normal) {
    Vector3 sol = Vector3Normalize((Vector3){ 0.35f, 0.8f, 0.45f });
    Vector3 n = Vector3Normalize(normal);
    return 0.58f + 0.30f * (n.y * 0.5f + 0.5f) + 0.38f * fmaxf(Vector3DotProduct(n, sol), 0.0f);
}

/*
 * quadIluminado
 * Um quadrilátero de uma cor, com a luz do sol calculada pela face dele.
 * Recebe: a construção, os 4 cantos em volta e a cor.
 * Devolve: nada.
 */
void quadIluminado(Construcao *c, Vector3 a, Vector3 b, Vector3 d, Vector3 e, Color cor) {
    Vector3 normal = Vector3Normalize(Vector3CrossProduct(Vector3Subtract(b, a), Vector3Subtract(d, a)));
    if (normal.y < -0.2f) {      /* só vira quem olha para baixo: uma parede em curva não fica em faixas */
        normal = Vector3Negate(normal);
    }
    float luz = luzDoSol(normal);
    Color comLuz = { (unsigned char) fminf(cor.r * luz, 255.0f), (unsigned char) fminf(cor.g * luz, 255.0f),
                     (unsigned char) fminf(cor.b * luz, 255.0f), 255 };
    Vector2 zero = { 0.0f, 0.0f };
    adicionarQuadrilatero(c, a, b, d, e, zero, zero, zero, zero, comLuz);
}

/*
 * caixaIluminada
 * Uma caixa em pé no chão: paredes e teto, com a luz do sol.
 * Recebe: a construção, o centro da base, o giro em radianos, os tamanhos e a cor.
 * Devolve: nada.
 */
void caixaIluminada(Construcao *c, Vector3 pe, float giro, float largura, float fundo, float altura, Color cor) {
    float co = cosf(giro), se = sinf(giro);
    Vector3 lado = { co * largura / 2.0f, 0.0f, -se * largura / 2.0f }, frente = { se * fundo / 2.0f, 0.0f, co * fundo / 2.0f };
    Vector3 cantos[4] = {
        { pe.x - lado.x - frente.x, pe.y, pe.z - lado.z - frente.z }, { pe.x + lado.x - frente.x, pe.y, pe.z + lado.z - frente.z },
        { pe.x + lado.x + frente.x, pe.y, pe.z + lado.z + frente.z }, { pe.x - lado.x + frente.x, pe.y, pe.z - lado.z + frente.z },
    };
    Vector3 cima = { 0.0f, altura, 0.0f };
    for (int i = 0; i < 4; i++) {
        Vector3 a = cantos[i], b = cantos[(i + 1) % 4];
        quadIluminado(c, a, b, Vector3Add(b, cima), Vector3Add(a, cima), cor);
    }
    quadIluminado(c, Vector3Add(cantos[0], cima), Vector3Add(cantos[1], cima), Vector3Add(cantos[2], cima), Vector3Add(cantos[3], cima), cor);
}
