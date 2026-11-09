/*
 * colisao.c
 * ------------------------------------------------------------------
 * A malha de colisão do mundo aberto. Veja colisao.h.
 *
 * O arquivo .col (versão 2):
 *   "KCOL", a versão, o número de triângulos, os limites (6 floats),
 *   a largada (x, y, z, rumo), o meio da pista de treino (x, y, z),
 *   a paleta (quantas entradas e, em cada uma, tipo de chão e cor RGB),
 *   os triângulos (9 floats cada) e, para cada triângulo, a entrada da paleta.
 * A versão 1 (só os triângulos) ainda é lida: tudo vira "outro".
 *
 * Cada triângulo entra em todas as células da grade que a caixa dele (vista
 * de cima) toca. As listas das células ficam juntas num vetor só (o começo
 * de cada uma em "inicio"). Um triângulo grande aparece em várias células:
 * a "marca" evita olhar o mesmo duas vezes na mesma pergunta.
 * ------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "colisao.h"

#define LADO_DA_CELULA     8.0f    /* metros */
#define NORMAL_DE_CHAO     0.55f   /* y da normal: daqui para cima, dá para andar em cima (~57 graus) */
#define PAREDE_RELATIVA    0.60f   /* cosseno: uma face mais inclinada que ~53 graus em relação ao carro é parede para ele */
#define MAIOR_PALETA       256

typedef struct {
    float a[3], b[3], c[3];
    float normal[3];   /* para cima (y >= 0) */
    float minimoY, maximoY;
    unsigned char superficie;
} Triangulo;

struct MalhaDeColisao {
    Triangulo *triangulos;
    int total;
    float minimo[3], maximo[3];
    int colunas, linhas;     /* a grade: colunas em x, linhas em z */
    int *inicio;             /* colunas * linhas + 1: onde começa a lista de cada célula */
    int *lista;              /* os índices dos triângulos, célula por célula */
    unsigned int *marca;     /* a última pergunta que olhou cada triângulo */
    unsigned int pergunta;
    int temPontos;
    float largada[4], arena[3];
    int totalDaPaleta;
    unsigned char paleta[MAIOR_PALETA][4];   /* tipo, r, g, b */
};

/* ------------------------------------------------------------------ */

static void subtrair(const float *a, const float *b, float *r) {
    r[0] = a[0] - b[0]; r[1] = a[1] - b[1]; r[2] = a[2] - b[2];
}

static float escalar(const float *a, const float *b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

static void vetorial(const float *a, const float *b, float *r) {
    r[0] = a[1] * b[2] - a[2] * b[1];
    r[1] = a[2] * b[0] - a[0] * b[2];
    r[2] = a[0] * b[1] - a[1] * b[0];
}

static int celulaDe(const MalhaDeColisao *m, float valor, int eixo, int total) {
    int c = (int) floorf((valor - m->minimo[eixo]) / LADO_DA_CELULA);
    return c < 0 ? 0 : (c >= total ? total - 1 : c);
}

/*
 * montarMalha
 * Monta a malha a partir dos vértices (9 floats por triângulo) e da entrada da paleta de
 * cada um (NULL: todos na entrada 0). Triângulos degenerados ficam de fora.
 */
static MalhaDeColisao *montarMalha(const float *vertices, const unsigned char *superficies, int totalDeTriangulos) {
    MalhaDeColisao *m = calloc(1, sizeof(MalhaDeColisao));
    if (m == NULL || totalDeTriangulos <= 0) {
        free(m);
        return NULL;
    }
    m->triangulos = malloc(sizeof(Triangulo) * (size_t) totalDeTriangulos);
    if (m->triangulos == NULL) {
        free(m);
        return NULL;
    }
    m->totalDaPaleta = 1;
    m->paleta[0][0] = CHAO_OUTRO;
    m->paleta[0][1] = m->paleta[0][2] = m->paleta[0][3] = 160;
    for (int e = 0; e < 3; e++) {
        m->minimo[e] = 1e30f;
        m->maximo[e] = -1e30f;
    }
    for (int i = 0; i < totalDeTriangulos; i++) {
        Triangulo t;
        memcpy(t.a, vertices + i * 9, sizeof(float) * 3);
        memcpy(t.b, vertices + i * 9 + 3, sizeof(float) * 3);
        memcpy(t.c, vertices + i * 9 + 6, sizeof(float) * 3);
        float ab[3], ac[3], n[3];
        subtrair(t.b, t.a, ab);
        subtrair(t.c, t.a, ac);
        vetorial(ab, ac, n);
        float tamanho = sqrtf(escalar(n, n));
        if (tamanho < 1e-6f) {
            continue;
        }
        float sinal = (n[1] < 0.0f) ? -1.0f : 1.0f;   /* a face pode vir de cabeça para baixo */
        for (int e = 0; e < 3; e++) {
            t.normal[e] = n[e] / tamanho * sinal;
        }
        t.minimoY = fminf(t.a[1], fminf(t.b[1], t.c[1]));
        t.maximoY = fmaxf(t.a[1], fmaxf(t.b[1], t.c[1]));
        t.superficie = superficies ? superficies[i] : 0;
        for (int e = 0; e < 3; e++) {
            m->minimo[e] = fminf(m->minimo[e], fminf(t.a[e], fminf(t.b[e], t.c[e])));
            m->maximo[e] = fmaxf(m->maximo[e], fmaxf(t.a[e], fmaxf(t.b[e], t.c[e])));
        }
        m->triangulos[m->total++] = t;
    }
    if (m->total == 0) {
        liberarMalhaDeColisao(m);
        return NULL;
    }

    m->colunas = (int) ((m->maximo[0] - m->minimo[0]) / LADO_DA_CELULA) + 1;
    m->linhas = (int) ((m->maximo[2] - m->minimo[2]) / LADO_DA_CELULA) + 1;
    size_t celulas = (size_t) m->colunas * (size_t) m->linhas;
    m->inicio = calloc(celulas + 1, sizeof(int));
    m->marca = calloc((size_t) m->total, sizeof(unsigned int));
    if (m->inicio == NULL || m->marca == NULL) {
        liberarMalhaDeColisao(m);
        return NULL;
    }

    /* Duas passadas: conta quantos vão em cada célula, depois põe cada um no lugar */
    for (int passada = 0; passada < 2; passada++) {
        int *proximo = NULL;
        if (passada == 1) {
            for (size_t c = 0; c < celulas; c++) {
                m->inicio[c + 1] += m->inicio[c];
            }
            m->lista = malloc(sizeof(int) * (size_t) (m->inicio[celulas] > 0 ? m->inicio[celulas] : 1));
            proximo = malloc(sizeof(int) * celulas);
            if (m->lista == NULL || proximo == NULL) {
                free(proximo);
                liberarMalhaDeColisao(m);
                return NULL;
            }
            memcpy(proximo, m->inicio, sizeof(int) * celulas);
        }
        for (int i = 0; i < m->total; i++) {
            const Triangulo *t = &m->triangulos[i];
            int c0 = celulaDe(m, fminf(t->a[0], fminf(t->b[0], t->c[0])), 0, m->colunas);
            int c1 = celulaDe(m, fmaxf(t->a[0], fmaxf(t->b[0], t->c[0])), 0, m->colunas);
            int l0 = celulaDe(m, fminf(t->a[2], fminf(t->b[2], t->c[2])), 2, m->linhas);
            int l1 = celulaDe(m, fmaxf(t->a[2], fmaxf(t->b[2], t->c[2])), 2, m->linhas);
            for (int l = l0; l <= l1; l++) {
                for (int c = c0; c <= c1; c++) {
                    size_t celula = (size_t) l * (size_t) m->colunas + (size_t) c;
                    if (passada == 0) {
                        m->inicio[celula + 1]++;
                    } else {
                        m->lista[proximo[celula]++] = i;
                    }
                }
            }
        }
        free(proximo);
    }
    return m;
}

MalhaDeColisao *criarMalhaDeColisao(const float *vertices, int totalDeTriangulos) {
    return montarMalha(vertices, NULL, totalDeTriangulos);
}

/*
 * carregarMalhaDeColisao
 * Recebe: o caminho do arquivo .col.
 * Devolve: a malha (NULL se o arquivo faltar ou estiver estragado).
 */
MalhaDeColisao *carregarMalhaDeColisao(const char *arquivo) {
    FILE *f = fopen(arquivo, "rb");
    if (f == NULL) {
        return NULL;
    }
    char assinatura[4];
    unsigned int versao = 0, total = 0, totalDaPaleta = 0;
    float limites[6], largada[4] = { 0 }, arena[3] = { 0 };
    unsigned char paleta[MAIOR_PALETA][4];
    MalhaDeColisao *m = NULL;
    int ok = fread(assinatura, 1, 4, f) == 4 && memcmp(assinatura, "KCOL", 4) == 0 &&
             fread(&versao, sizeof versao, 1, f) == 1 && (versao == 1 || versao == 2) &&
             fread(&total, sizeof total, 1, f) == 1 && total > 0 && total < 50000000u &&
             fread(limites, sizeof(float), 6, f) == 6;
    if (ok && versao == 2) {
        ok = fread(largada, sizeof(float), 4, f) == 4 && fread(arena, sizeof(float), 3, f) == 3 &&
             fread(&totalDaPaleta, sizeof totalDaPaleta, 1, f) == 1 && totalDaPaleta > 0 && totalDaPaleta <= MAIOR_PALETA &&
             fread(paleta, 4, totalDaPaleta, f) == totalDaPaleta;
    }
    if (ok) {
        float *vertices = malloc(sizeof(float) * 9 * (size_t) total);
        unsigned char *superficies = (versao == 2) ? malloc(total) : NULL;
        if (vertices != NULL && fread(vertices, sizeof(float) * 9, total, f) == total &&
            (versao == 1 || (superficies != NULL && fread(superficies, 1, total, f) == total))) {
            if (superficies != NULL) {
                for (unsigned int i = 0; i < total; i++) {
                    if (superficies[i] >= totalDaPaleta) {
                        superficies[i] = 0;
                    }
                }
            }
            m = montarMalha(vertices, superficies, (int) total);
            if (m != NULL && versao == 2) {
                m->temPontos = 1;
                memcpy(m->largada, largada, sizeof largada);
                memcpy(m->arena, arena, sizeof arena);
                m->totalDaPaleta = (int) totalDaPaleta;
                memcpy(m->paleta, paleta, 4 * totalDaPaleta);
                for (int i = 0; i < m->totalDaPaleta; i++) {
                    if (m->paleta[i][0] >= TOTAL_DE_CHAOS) {
                        m->paleta[i][0] = CHAO_OUTRO;
                    }
                }
            }
        }
        free(vertices);
        free(superficies);
    }
    fclose(f);
    return m;
}

void liberarMalhaDeColisao(MalhaDeColisao *malha) {
    if (malha == NULL) {
        return;
    }
    free(malha->triangulos);
    free(malha->inicio);
    free(malha->lista);
    free(malha->marca);
    free(malha);
}

int triangulosDaMalha(const MalhaDeColisao *malha) {
    return malha ? malha->total : 0;
}

void limitesDaMalha(const MalhaDeColisao *malha, float minimo[3], float maximo[3]) {
    memcpy(minimo, malha->minimo, sizeof(float) * 3);
    memcpy(maximo, malha->maximo, sizeof(float) * 3);
}

/*
 * pontosDaMalha
 * Recebe: a malha e onde pôr a largada (x, y, z, rumo) e o meio da pista de treino (x, y, z).
 * Devolve: 1 se o arquivo trazia esses pontos.
 */
int pontosDaMalha(const MalhaDeColisao *malha, float largada[4], float arena[3]) {
    if (!malha->temPontos) {
        return 0;
    }
    memcpy(largada, malha->largada, sizeof malha->largada);
    memcpy(arena, malha->arena, sizeof malha->arena);
    return 1;
}

TipoDeChao tipoDoChao(const MalhaDeColisao *malha, int superficie) {
    if (superficie < 0 || superficie >= malha->totalDaPaleta) {
        return CHAO_OUTRO;
    }
    return (TipoDeChao) malha->paleta[superficie][0];
}

void corDoChao(const MalhaDeColisao *malha, int superficie, unsigned char cor[3]) {
    if (superficie < 0 || superficie >= malha->totalDaPaleta) {
        superficie = 0;
    }
    memcpy(cor, &malha->paleta[superficie][1], 3);
}

/* Uma pergunta nova: as marcas antigas deixam de valer (e, se o contador der a volta, zeram) */
static void novaPergunta(MalhaDeColisao *m) {
    if (++m->pergunta == 0) {
        memset(m->marca, 0, sizeof(unsigned int) * (size_t) m->total);
        m->pergunta = 1;
    }
}

/*
 * alturaDoChaoNaMalha
 * O chão é o triângulo mais alto embaixo do ponto (visto de cima) que não passa do teto:
 * assim a pergunta passa por baixo de uma marquise. Paredes em pé não contam como chão.
 * Recebe: a malha, o ponto (x, z), a altura máxima do chão e onde pôr a normal dele (pode ser NULL).
 * Devolve: a altura do chão, ou SEM_CHAO.
 */
float alturaDoChaoNaMalha(MalhaDeColisao *malha, float x, float z, float teto, float normal[3]) {
    if (x < malha->minimo[0] || x > malha->maximo[0] || z < malha->minimo[2] || z > malha->maximo[2]) {
        return SEM_CHAO;
    }
    novaPergunta(malha);
    int c = celulaDe(malha, x, 0, malha->colunas), l = celulaDe(malha, z, 2, malha->linhas);
    size_t celula = (size_t) l * (size_t) malha->colunas + (size_t) c;
    float melhor = SEM_CHAO;
    for (int k = malha->inicio[celula]; k < malha->inicio[celula + 1]; k++) {
        int i = malha->lista[k];
        if (malha->marca[i] == malha->pergunta) {
            continue;
        }
        malha->marca[i] = malha->pergunta;
        const Triangulo *t = &malha->triangulos[i];
        if (t->normal[1] < NORMAL_DE_CHAO || t->minimoY > teto || t->maximoY < melhor) {
            continue;
        }
        /* Baricêntricas vistas de cima */
        float v0x = t->b[0] - t->a[0], v0z = t->b[2] - t->a[2];
        float v1x = t->c[0] - t->a[0], v1z = t->c[2] - t->a[2];
        float v2x = x - t->a[0], v2z = z - t->a[2];
        float det = v0x * v1z - v1x * v0z;
        if (fabsf(det) < 1e-9f) {
            continue;
        }
        float u = (v2x * v1z - v1x * v2z) / det;
        float v = (v0x * v2z - v2x * v0z) / det;
        if (u < -1e-4f || v < -1e-4f || u + v > 1.0001f) {
            continue;
        }
        float y = t->a[1] + u * (t->b[1] - t->a[1]) + v * (t->c[1] - t->a[1]);
        if (y <= teto && y > melhor) {
            melhor = y;
            if (normal != NULL) {
                memcpy(normal, t->normal, sizeof(float) * 3);
            }
        }
    }
    return melhor;
}

/*
 * raioNaMalha
 * O primeiro triângulo que um raio curto encontra (Möller e Trumbore), de qualquer lado.
 * Recebe: a malha, a origem, a direção (de tamanho 1), até onde olhar e onde pôr o acerto.
 * Devolve: 1 se bateu em alguma coisa.
 */
int raioNaMalha(MalhaDeColisao *malha, const float origem[3], const float direcao[3], float alcance, Acerto *acerto) {
    float fim[3] = { origem[0] + direcao[0] * alcance, origem[1] + direcao[1] * alcance, origem[2] + direcao[2] * alcance };
    float x0 = fminf(origem[0], fim[0]), x1 = fmaxf(origem[0], fim[0]);
    float z0 = fminf(origem[2], fim[2]), z1 = fmaxf(origem[2], fim[2]);
    if (x1 < malha->minimo[0] || x0 > malha->maximo[0] || z1 < malha->minimo[2] || z0 > malha->maximo[2]) {
        return 0;
    }
    novaPergunta(malha);
    int c0 = celulaDe(malha, x0, 0, malha->colunas), c1 = celulaDe(malha, x1, 0, malha->colunas);
    int l0 = celulaDe(malha, z0, 2, malha->linhas), l1 = celulaDe(malha, z1, 2, malha->linhas);
    float y0 = fminf(origem[1], fim[1]), y1 = fmaxf(origem[1], fim[1]);
    float melhor = alcance;
    int achou = -1;
    for (int l = l0; l <= l1; l++) {
        for (int c = c0; c <= c1; c++) {
            size_t celula = (size_t) l * (size_t) malha->colunas + (size_t) c;
            for (int k = malha->inicio[celula]; k < malha->inicio[celula + 1]; k++) {
                int i = malha->lista[k];
                if (malha->marca[i] == malha->pergunta) {
                    continue;
                }
                malha->marca[i] = malha->pergunta;
                const Triangulo *t = &malha->triangulos[i];
                if (t->maximoY < y0 || t->minimoY > y1) {
                    continue;
                }
                float e1[3], e2[3], p[3], s[3], q[3];
                subtrair(t->b, t->a, e1);
                subtrair(t->c, t->a, e2);
                vetorial(direcao, e2, p);
                float det = escalar(e1, p);
                if (fabsf(det) < 1e-9f) {
                    continue;
                }
                float inverso = 1.0f / det;
                subtrair(origem, t->a, s);
                float u = escalar(s, p) * inverso;
                if (u < 0.0f || u > 1.0f) {
                    continue;
                }
                vetorial(s, e1, q);
                float v = escalar(direcao, q) * inverso;
                if (v < 0.0f || u + v > 1.0f) {
                    continue;
                }
                float d = escalar(e2, q) * inverso;
                if (d >= 0.0f && d < melhor) {
                    melhor = d;
                    achou = i;
                }
            }
        }
    }
    if (achou < 0) {
        return 0;
    }
    const Triangulo *t = &malha->triangulos[achou];
    float sinal = (escalar(t->normal, direcao) > 0.0f) ? -1.0f : 1.0f;
    for (int e = 0; e < 3; e++) {
        acerto->ponto[e] = origem[e] + direcao[e] * melhor;
        acerto->normal[e] = t->normal[e] * sinal;
    }
    acerto->distancia = melhor;
    acerto->superficie = t->superficie;
    return 1;
}

/*
 * pontoMaisPerto
 * O ponto do triângulo mais perto de p (Ericson, "Real-Time Collision Detection", 5.1.5).
 */
static void pontoMaisPerto(const float *p, const Triangulo *t, float *r) {
    float ab[3], ac[3], ap[3], bp[3], cp[3];
    subtrair(t->b, t->a, ab);
    subtrair(t->c, t->a, ac);
    subtrair(p, t->a, ap);
    float d1 = escalar(ab, ap), d2 = escalar(ac, ap);
    if (d1 <= 0.0f && d2 <= 0.0f) { memcpy(r, t->a, sizeof(float) * 3); return; }
    subtrair(p, t->b, bp);
    float d3 = escalar(ab, bp), d4 = escalar(ac, bp);
    if (d3 >= 0.0f && d4 <= d3) { memcpy(r, t->b, sizeof(float) * 3); return; }
    float vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
        float v = d1 / (d1 - d3);
        for (int e = 0; e < 3; e++) r[e] = t->a[e] + v * ab[e];
        return;
    }
    subtrair(p, t->c, cp);
    float d5 = escalar(ab, cp), d6 = escalar(ac, cp);
    if (d6 >= 0.0f && d5 <= d6) { memcpy(r, t->c, sizeof(float) * 3); return; }
    float vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
        float w = d2 / (d2 - d6);
        for (int e = 0; e < 3; e++) r[e] = t->a[e] + w * ac[e];
        return;
    }
    float va = d3 * d6 - d5 * d4;
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
        float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        for (int e = 0; e < 3; e++) r[e] = t->b[e] + w * (t->c[e] - t->b[e]);
        return;
    }
    float denom = 1.0f / (va + vb + vc);
    float v = vb * denom, w = vc * denom;
    for (int e = 0; e < 3; e++) r[e] = t->a[e] + ab[e] * v + ac[e] * w;
}

/*
 * empurrarDasParedes
 * Uma esfera (um pedaço do carro) contra as paredes em volta. "Parede" é relativa ao carro:
 * qualquer face muito inclinada em relação ao "para cima" dele (no chão é o céu; numa parede,
 * é a normal dela). Soma, no plano do chão do carro, o quanto cada uma o empurra para fora.
 * Recebe: a malha, o centro e o raio da esfera, o "para cima" do carro e onde pôr o empurrão.
 * Devolve: 1 se encostou em alguma parede.
 */
int empurrarDasParedes(MalhaDeColisao *malha, const float centro[3], float raio, const float cima[3], float empurrao[3]) {
    empurrao[0] = empurrao[1] = empurrao[2] = 0.0f;
    novaPergunta(malha);
    int c0 = celulaDe(malha, centro[0] - raio, 0, malha->colunas), c1 = celulaDe(malha, centro[0] + raio, 0, malha->colunas);
    int l0 = celulaDe(malha, centro[2] - raio, 2, malha->linhas), l1 = celulaDe(malha, centro[2] + raio, 2, malha->linhas);
    int encostou = 0;
    for (int l = l0; l <= l1; l++) {
        for (int c = c0; c <= c1; c++) {
            size_t celula = (size_t) l * (size_t) malha->colunas + (size_t) c;
            for (int k = malha->inicio[celula]; k < malha->inicio[celula + 1]; k++) {
                int i = malha->lista[k];
                if (malha->marca[i] == malha->pergunta) {
                    continue;
                }
                malha->marca[i] = malha->pergunta;
                const Triangulo *t = &malha->triangulos[i];
                if (fabsf(escalar(t->normal, cima)) > PAREDE_RELATIVA || t->maximoY < centro[1] - raio || t->minimoY > centro[1] + raio) {
                    continue;
                }
                float perto[3], d[3];
                pontoMaisPerto(centro, t, perto);
                subtrair(centro, perto, d);
                float distancia = sqrtf(escalar(d, d));
                if (distancia >= raio) {
                    continue;
                }
                /* Para fora, no plano do chão do carro: pela linha até o ponto, ou (rente) pela normal */
                float h[3];
                float acima = escalar(d, cima);
                for (int e = 0; e < 3; e++) {
                    h[e] = d[e] - cima[e] * acima;
                }
                float tamanho = sqrtf(escalar(h, h));
                if (tamanho < 0.2f * (distancia + 1e-6f)) {
                    float na = escalar(t->normal, cima);
                    for (int e = 0; e < 3; e++) {
                        h[e] = t->normal[e] - cima[e] * na;
                    }
                    tamanho = sqrtf(escalar(h, h));
                    if (tamanho < 1e-6f) {
                        continue;
                    }
                    if (escalar(h, d) < 0.0f) {   /* a normal aponta para o lado de onde o carro veio */
                        for (int e = 0; e < 3; e++) {
                            h[e] = -h[e];
                        }
                    }
                }
                float entrou = raio - distancia;
                for (int e = 0; e < 3; e++) {
                    empurrao[e] += h[e] / tamanho * entrou;
                }
                encostou = 1;
            }
        }
    }
    return encostou;
}
