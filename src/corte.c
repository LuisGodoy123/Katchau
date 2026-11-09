/*
 * corte.c
 * ------------------------------------------------------------------
 * O corte pelo campo de visão. Veja corte.h.
 *
 * O campo de visão é uma pirâmide cortada, limitada por seis planos
 * (esquerda, direita, baixo, alto, perto e longe). Os seis saem direto
 * das matrizes de vista e de projeção da câmera, somando ou subtraindo
 * linhas delas: é a conta de Gribb e Hartmann. Cada plano é guardado
 * como (a, b, c, d), com a normal apontando para dentro, então um ponto
 * está do lado de dentro quando a*x + b*y + c*z + d >= 0.
 *
 * As matrizes vêm do próprio rlgl (as mesmas que o DrawMesh vai usar), e
 * não da struct Camera3D: a passagem do telão e a do mapa de sombra
 * desenham com outra projeção, e a conta tem de valer para as três.
 * ------------------------------------------------------------------
 */

#include <math.h>
#include <stdlib.h>
#include "corte.h"
#include "rlgl.h"
#include "raymath.h"

#define TOTAL_DE_PLANOS 6

static float planos[TOTAL_DE_PLANOS][4];
static int invertido = -1;   /* KATCHAU_SO_O_CORTADO: desenha só o que seria cortado */
static int conferir = -1;    /* KATCHAU_CONFERIR_CORTE: conta quanto está sendo cortado */
static int passaram = 0, cortados = 0;

/*
 * contar
 * Com KATCHAU_CONFERIR_CORTE ligado, escreve no log quanto do cenário está sendo cortado,
 * uma vez por segundo. Serve para ver se o corte está fazendo sentido: perto de 0% quer
 * dizer que ele não está pegando nada, e 100% que ele está comendo a tela inteira.
 * Recebe: 1 se a coisa passou no teste.
 * Devolve: nada.
 */
static void contar(int passou) {
    if (conferir <= 0) {
        return;
    }
    if (passou) {
        passaram++;
    } else {
        cortados++;
    }
    static double doUltimoAviso = 0.0;
    double agora = GetTime();
    if (agora - doUltimoAviso < 1.0) {
        return;
    }
    doUltimoAviso = agora;
    int total = passaram + cortados;
    TraceLog(LOG_INFO, "CORTE: %d de %d testes passaram (%.0f%% cortado)", passaram, total,
             total > 0 ? 100.0 * cortados / total : 0.0);
    passaram = 0;
    cortados = 0;
}

/*
 * guardarPlanos
 * Tira os seis planos da matriz de vista-projeção.
 * Recebe: a matriz de vista e a de projeção.
 * Devolve: nada.
 */
static void guardarPlanos(Matrix vista, Matrix projecao) {
    Matrix m = MatrixMultiply(vista, projecao);
    /* A Matrix da raylib guarda m0, m4, m8, m12 na primeira linha */
    const float linha[4][4] = {
        { m.m0, m.m4, m.m8,  m.m12 },   /* x */
        { m.m1, m.m5, m.m9,  m.m13 },   /* y */
        { m.m2, m.m6, m.m10, m.m14 },   /* z */
        { m.m3, m.m7, m.m11, m.m15 },   /* w */
    };
    for (int i = 0; i < 4; i++) {
        planos[0][i] = linha[3][i] + linha[0][i];   /* esquerda */
        planos[1][i] = linha[3][i] - linha[0][i];   /* direita */
        planos[2][i] = linha[3][i] + linha[1][i];   /* baixo */
        planos[3][i] = linha[3][i] - linha[1][i];   /* alto */
        planos[4][i] = linha[3][i] + linha[2][i];   /* perto */
        planos[5][i] = linha[3][i] - linha[2][i];   /* longe */
    }
    if (invertido < 0) {
        invertido = (getenv("KATCHAU_SO_O_CORTADO") != NULL);
        conferir = (getenv("KATCHAU_CONFERIR_CORTE") != NULL);
    }
}

/*
 * prepararCorte
 * Os planos da câmera que está desenhando agora. Chamar logo depois de BeginMode3D.
 * Recebe: nada.
 * Devolve: nada.
 */
void prepararCorte(void) {
    guardarPlanos(rlGetMatrixModelview(), rlGetMatrixProjection());
}

/*
 * prepararCorteDaCamera
 * Os planos de uma câmera que ainda não está desenhando (o telão decide se vale
 * desenhar antes de abrir a imagem dele). Monta as matrizes do mesmo jeito que o
 * BeginMode3D da raylib, com uma folga no ângulo para absorver qualquer diferença.
 * Recebe: a câmera, o campo de visão vertical em graus, a proporção da imagem e a
 *         folga (0,15 = 15% a mais de ângulo para cada lado).
 * Devolve: nada.
 */
void prepararCorteDaCamera(Camera3D camera, float graus, float aspecto, float folga) {
    float abertura = graus * (1.0f + folga);
    if (abertura > 170.0f) {
        abertura = 170.0f;
    }
    Matrix vista = MatrixLookAt(camera.position, camera.target, camera.up);
    Matrix projecao = MatrixPerspective(abertura * DEG2RAD, aspecto * (1.0f + folga),
                                        rlGetCullDistanceNear(), rlGetCullDistanceFar());
    guardarPlanos(vista, projecao);
}

/*
 * caixaAVista
 * Recebe: uma caixa alinhada aos eixos, em coordenadas do mundo.
 * Devolve: 1 se ela pode aparecer na tela.
 */
int caixaAVista(BoundingBox caixa) {
    for (int p = 0; p < TOTAL_DE_PLANOS; p++) {
        /* O canto da caixa que mais avança na direção da normal: se nem ele está
         * do lado de dentro, nenhum dos oito está, e a caixa inteira ficou fora */
        float x = (planos[p][0] >= 0.0f) ? caixa.max.x : caixa.min.x;
        float y = (planos[p][1] >= 0.0f) ? caixa.max.y : caixa.min.y;
        float z = (planos[p][2] >= 0.0f) ? caixa.max.z : caixa.min.z;
        if (planos[p][0] * x + planos[p][1] * y + planos[p][2] * z + planos[p][3] < 0.0f) {
            contar(0);
            return invertido > 0;
        }
    }
    contar(1);
    return invertido > 0 ? 0 : 1;
}

/*
 * esferaAVista
 * Recebe: o centro e o raio, em metros.
 * Devolve: 1 se a esfera pode aparecer na tela.
 */
int esferaAVista(Vector3 centro, float raio) {
    for (int p = 0; p < TOTAL_DE_PLANOS; p++) {
        /* Os planos não estão normalizados, então a distância é comparada na mesma escala deles */
        float a = planos[p][0], b = planos[p][1], c = planos[p][2];
        float tamanho = sqrtf(a * a + b * b + c * c);
        if (a * centro.x + b * centro.y + c * centro.z + planos[p][3] < -raio * tamanho) {
            return invertido > 0;
        }
    }
    return invertido > 0 ? 0 : 1;
}

/*
 * pontoAVista
 * Recebe: o ponto e uma folga em metros (o raio do que está nele).
 * Devolve: 1 se ele pode aparecer na tela.
 */
int pontoAVista(Vector3 ponto, float folga) {
    return esferaAVista(ponto, folga);
}

/*
 * caixaDaMalha
 * A caixa que cerca uma malha, em coordenadas do mundo.
 * Recebe: a malha e a matriz com que ela é desenhada.
 * Devolve: a caixa no mundo, ou uma caixa infinita se a malha não tem os vértices na memória.
 */
BoundingBox caixaDaMalha(Mesh malha, Matrix noMundo) {
    if (malha.vertices == NULL || malha.vertexCount <= 0) {
        /* Sem os vértices na memória não há como medir. Uma caixa que cobre tudo nunca é
         * cortada: na dúvida, desenhar é o lado seguro — cortar faria a malha desaparecer. */
        BoundingBox tudo = { { -1e30f, -1e30f, -1e30f }, { 1e30f, 1e30f, 1e30f } };
        return tudo;
    }
    return caixaNoMundo(GetMeshBoundingBox(malha), noMundo);
}

/*
 * caixaNoMundo
 * Leva uma caixa de coordenadas da malha para coordenadas do mundo, passando os
 * oito cantos pela matriz e pegando os extremos (girar uma caixa não dá uma caixa).
 * Recebe: a caixa local e a matriz do modelo.
 * Devolve: a caixa no mundo.
 */
BoundingBox caixaNoMundo(BoundingBox local, Matrix noMundo) {
    BoundingBox fora = { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
    for (int i = 0; i < 8; i++) {
        Vector3 canto = { (i & 1) ? local.max.x : local.min.x,
                          (i & 2) ? local.max.y : local.min.y,
                          (i & 4) ? local.max.z : local.min.z };
        canto = Vector3Transform(canto, noMundo);
        if (i == 0) {
            fora.min = canto;
            fora.max = canto;
            continue;
        }
        fora.min.x = fminf(fora.min.x, canto.x);
        fora.min.y = fminf(fora.min.y, canto.y);
        fora.min.z = fminf(fora.min.z, canto.z);
        fora.max.x = fmaxf(fora.max.x, canto.x);
        fora.max.y = fmaxf(fora.max.y, canto.y);
        fora.max.z = fmaxf(fora.max.z, canto.z);
    }
    return fora;
}
