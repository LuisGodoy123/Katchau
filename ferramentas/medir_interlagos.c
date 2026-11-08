/*
 * medir_interlagos.c
 * ------------------------------------------------------------------
 * Mede a pista no modelo recursos/modelos/interlagos.glb e escreve as tabelas
 * de src/interlagos.h: a linha da pista, a largura do asfalto, o perfil do chão,
 * as zebras, a área de escape, o que para o carro de cada lado, o caminho do
 * pit lane e os lugares da torcida.
 *
 * Usa a raylib só para carregar o modelo (assim os eixos são os mesmos do jogo).
 * O resto é geometria: os triângulos do chão e das barreiras ficam numa grade
 * vista de cima, e cada pergunta ("o que tem embaixo deste ponto?", "este passo
 * de lado atravessa um muro?") olha só as células por onde passa.
 *
 * Uso:
 *   make medir-interlagos
 *   ./build/medir_interlagos recursos/modelos/interlagos.glb src/interlagos.h
 *   ./build/medir_interlagos recursos/modelos/interlagos.glb --sonda x0 z0 x1 z1 n
 * ------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "raylib.h"
#include "../src/interlagos_materiais.h"

#define LADO_DA_CELULA     4.0f    /* metros */
#define PASSO_DA_LINHA     2.0f    /* metros entre dois pontos da tabela */
#define SALTO_DA_BORDA     2.5f    /* metros: a borda não muda tanto de um passo para o outro */
#define CORRECAO_MAXIMA    1.0f    /* metros por passo que a linha pode ir para o lado */
#define PASSO_DE_LADO      0.1f    /* metros: a procura da borda do asfalto */
#define MAIOR_MEIA_LARGURA 30.0f
#define PROCURA_DO_MURO    90.0f   /* metros: além disso, não há o que pare o carro */
#define PASSO_DO_MURO      0.25f
#define JANELA_DA_BORDA    20      /* pontos para cada lado: 40 m */
#define FOLGA_DA_BORDA     0.8f    /* metros além da mediana que a borda pode ir */
#define GRAMA_SEM_MURO     12.0f   /* metros de grama depois do escape, onde não há barreira */
#define MENOR_LIMITE_SEM_MURO 25.0f
#define PONTOS_DO_PERFIL   9
#define PONTOS_DE_FORA     13
#define MAXIMO_DE_PONTOS   8000

/* As distâncias além da borda do asfalto onde a altura do chão é medida */
static const float DISTANCIAS_DE_FORA[PONTOS_DE_FORA] = { 1.0f, 2.0f, 3.0f, 5.0f, 7.0f, 10.0f, 14.0f, 20.0f, 28.0f, 40.0f, 55.0f, 70.0f, 90.0f };

/* A linha de chegada, no meio do pórtico da largada, e o rumo da reta (para +z) */
#define LARGADA_X   (-480.0f)
#define LARGADA_Z   88.0f

/* Um ponto no meio do pit lane, na frente dos boxes */
#define LARGURA_MINIMA_DO_PIT 2.5f
#define PIT_X       (-458.0f)
#define PIT_Z       200.0f

typedef struct {
    Vector3 a, b, c;
    int material;
} Triangulo;

typedef struct {
    float x0, z0;
    int colunas, linhas;
    int *inicio;       /* inicio[celula]..inicio[celula + 1]: índices em "lista" */
    int *lista;
    Triangulo *triangulos;
    int total;
} Grade;

static Grade chao, barreiras, assentos, borracha;

/* ==================================================================
 * Montar as grades
 * ================================================================== */

static int naGrade(const Grade *g, float x, float z, int *coluna, int *linha) {
    *coluna = (int) floorf((x - g->x0) / LADO_DA_CELULA);
    *linha = (int) floorf((z - g->z0) / LADO_DA_CELULA);
    return *coluna >= 0 && *coluna < g->colunas && *linha >= 0 && *linha < g->linhas;
}

static void celulasDoTriangulo(const Grade *g, const Triangulo *t, int *c0, int *l0, int *c1, int *l1) {
    float xmin = fminf(t->a.x, fminf(t->b.x, t->c.x)), xmax = fmaxf(t->a.x, fmaxf(t->b.x, t->c.x));
    float zmin = fminf(t->a.z, fminf(t->b.z, t->c.z)), zmax = fmaxf(t->a.z, fmaxf(t->b.z, t->c.z));
    *c0 = (int) floorf((xmin - g->x0) / LADO_DA_CELULA);
    *c1 = (int) floorf((xmax - g->x0) / LADO_DA_CELULA);
    *l0 = (int) floorf((zmin - g->z0) / LADO_DA_CELULA);
    *l1 = (int) floorf((zmax - g->z0) / LADO_DA_CELULA);
    if (*c0 < 0) *c0 = 0;
    if (*l0 < 0) *l0 = 0;
    if (*c1 >= g->colunas) *c1 = g->colunas - 1;
    if (*l1 >= g->linhas) *l1 = g->linhas - 1;
}

/* Junta os triângulos dos materiais que "servem" e espalha cada um pelas células que ele cobre */
static void montarGrade(Grade *g, Model *modelo, int (*serve)(int material)) {
    g->total = 0;
    int capacidade = 0;
    for (int m = 0; m < modelo->meshCount; m++) {
        if (serve(modelo->meshMaterial[m])) {
            capacidade += modelo->meshes[m].triangleCount;
        }
    }
    g->triangulos = malloc(sizeof(Triangulo) * (capacidade + 1));
    float xmin = 1e9f, zmin = 1e9f, xmax = -1e9f, zmax = -1e9f;
    for (int m = 0; m < modelo->meshCount; m++) {
        int material = modelo->meshMaterial[m];
        if (!serve(material)) {
            continue;
        }
        Mesh *malha = &modelo->meshes[m];
        for (int t = 0; t < malha->triangleCount; t++) {
            int i[3];
            for (int k = 0; k < 3; k++) {
                i[k] = malha->indices ? malha->indices[t * 3 + k] : t * 3 + k;
            }
            Triangulo *novo = &g->triangulos[g->total];
            Vector3 *v[3] = { &novo->a, &novo->b, &novo->c };
            float ymin = 1e9f, ymax = -1e9f;
            for (int k = 0; k < 3; k++) {
                *v[k] = (Vector3){ malha->vertices[i[k] * 3], malha->vertices[i[k] * 3 + 1], malha->vertices[i[k] * 3 + 2] };
                ymin = fminf(ymin, v[k]->y);
                ymax = fmaxf(ymax, v[k]->y);
            }
            if (material == MATERIAL_COM_PAREDES_INVISIVEIS && ymax - ymin > ALTURA_DA_PAREDE_INVISIVEL) {
                continue;   /* parede invisível do jogo original */
            }
            for (int k = 0; k < 3; k++) {
                xmin = fminf(xmin, v[k]->x); xmax = fmaxf(xmax, v[k]->x);
                zmin = fminf(zmin, v[k]->z); zmax = fmaxf(zmax, v[k]->z);
            }
            novo->material = material;
            g->total++;
        }
    }
    g->x0 = xmin - 1.0f;
    g->z0 = zmin - 1.0f;
    g->colunas = (int) ((xmax - g->x0) / LADO_DA_CELULA) + 2;
    g->linhas = (int) ((zmax - g->z0) / LADO_DA_CELULA) + 2;
    int celulas = g->colunas * g->linhas;
    g->inicio = calloc(celulas + 1, sizeof(int));
    int *conta = calloc(celulas, sizeof(int));
    for (int i = 0; i < g->total; i++) {
        int c0, l0, c1, l1;
        celulasDoTriangulo(g, &g->triangulos[i], &c0, &l0, &c1, &l1);
        for (int l = l0; l <= l1; l++) {
            for (int c = c0; c <= c1; c++) {
                conta[l * g->colunas + c]++;
            }
        }
    }
    for (int i = 0; i < celulas; i++) {
        g->inicio[i + 1] = g->inicio[i] + conta[i];
        conta[i] = 0;
    }
    g->lista = malloc(sizeof(int) * (g->inicio[celulas] + 1));
    for (int i = 0; i < g->total; i++) {
        int c0, l0, c1, l1;
        celulasDoTriangulo(g, &g->triangulos[i], &c0, &l0, &c1, &l1);
        for (int l = l0; l <= l1; l++) {
            for (int c = c0; c <= c1; c++) {
                int celula = l * g->colunas + c;
                g->lista[g->inicio[celula] + conta[celula]++] = i;
            }
        }
    }
    free(conta);
}

static int ehChao(int material) {
    if (material <= 0 || material >= TOTAL_MATERIAIS_INTERLAGOS) {
        return 0;
    }
    ChaoDoMaterial c = MATERIAIS_INTERLAGOS[material].chao;
    return c != CHAO_NENHUM && c != CHAO_MURO;
}

static int ehBarreira(int material) {
    return material > 0 && material < TOTAL_MATERIAIS_INTERLAGOS && MATERIAIS_INTERLAGOS[material].chao == CHAO_MURO;
}

/* A borracha do traçado cobre toda a pista de corrida, e só ela (o pit lane não tem) */
#define MATERIAL_DA_BORRACHA 19
static int ehBorracha(int material) {
    return material == MATERIAL_DA_BORRACHA;
}

/* As linhas brancas da borda da pista ficam fora da borracha */
static int ehLinhaDaBorda(int material) {
    return material == 166 || material == 177;
}

/* Os lugares da torcida: os assentos das arquibancadas */
static int ehAssento(int material) {
    return material == 29 || material == 175 || material == 61 || material == 118;
}

/* ==================================================================
 * Perguntas ao modelo
 * ================================================================== */

/* O chão embaixo de um ponto: o mais alto que não passa de "teto" */
typedef struct {
    int achou;
    float y;
    ChaoDoMaterial chao;
    int material;
} Embaixo;

static Embaixo chaoEm(float x, float z, float teto) {
    Embaixo e = { 0, -1e9f, CHAO_NENHUM, 0 };
    int c, l;
    if (!naGrade(&chao, x, z, &c, &l)) {
        return e;
    }
    int celula = l * chao.colunas + c;
    for (int k = chao.inicio[celula]; k < chao.inicio[celula + 1]; k++) {
        Triangulo *t = &chao.triangulos[chao.lista[k]];
        float d = (t->b.z - t->c.z) * (t->a.x - t->c.x) + (t->c.x - t->b.x) * (t->a.z - t->c.z);
        if (fabsf(d) < 1e-9f) {
            continue;
        }
        float u = ((t->b.z - t->c.z) * (x - t->c.x) + (t->c.x - t->b.x) * (z - t->c.z)) / d;
        float v = ((t->c.z - t->a.z) * (x - t->c.x) + (t->a.x - t->c.x) * (z - t->c.z)) / d;
        float w = 1.0f - u - v;
        if (u < -1e-5f || v < -1e-5f || w < -1e-5f) {
            continue;
        }
        float y = u * t->a.y + v * t->b.y + w * t->c.y;
        if (y > teto) {
            continue;
        }
        /* A grama de longe (a foto do terreno) só vale onde não há outra coisa */
        int ehTerreno = (t->material == 169 || t->material == 45);
        if (e.achou && ehTerreno && fabsf(y - e.y) < 3.0f) {
            continue;
        }
        if (!e.achou || y > e.y || (fabsf(y - e.y) < 3.0f && (e.material == 169 || e.material == 45))) {
            e.achou = 1;
            e.y = y;
            e.material = t->material;
            e.chao = MATERIAIS_INTERLAGOS[t->material].chao;
        }
    }
    return e;
}

/* Tem borracha do traçado em cima deste ponto, perto da altura dada? */
static int temBorracha(float x, float z, float altura) {
    int c, l;
    if (!naGrade(&borracha, x, z, &c, &l)) {
        return 0;
    }
    int celula = l * borracha.colunas + c;
    for (int k = borracha.inicio[celula]; k < borracha.inicio[celula + 1]; k++) {
        Triangulo *t = &borracha.triangulos[borracha.lista[k]];
        float d = (t->b.z - t->c.z) * (t->a.x - t->c.x) + (t->c.x - t->b.x) * (t->a.z - t->c.z);
        if (fabsf(d) < 1e-9f) {
            continue;
        }
        float u = ((t->b.z - t->c.z) * (x - t->c.x) + (t->c.x - t->b.x) * (z - t->c.z)) / d;
        float v = ((t->c.z - t->a.z) * (x - t->c.x) + (t->a.x - t->c.x) * (z - t->c.z)) / d;
        if (u < -1e-5f || v < -1e-5f || u + v > 1.0f + 1e-5f) {
            continue;
        }
        float y = u * t->a.y + v * t->b.y + (1.0f - u - v) * t->c.y;
        if (fabsf(y - altura) < 1.5f) {
            return 1;
        }
    }
    return 0;
}

/* Möller-Trumbore: o segmento de "o" até "o + d" corta o triângulo? Devolve a fração, ou -1 */
static float cortaTriangulo(Vector3 o, Vector3 d, const Triangulo *t) {
    Vector3 e1 = { t->b.x - t->a.x, t->b.y - t->a.y, t->b.z - t->a.z };
    Vector3 e2 = { t->c.x - t->a.x, t->c.y - t->a.y, t->c.z - t->a.z };
    Vector3 p = { d.y * e2.z - d.z * e2.y, d.z * e2.x - d.x * e2.z, d.x * e2.y - d.y * e2.x };
    float det = e1.x * p.x + e1.y * p.y + e1.z * p.z;
    if (fabsf(det) < 1e-9f) {
        return -1.0f;
    }
    float inv = 1.0f / det;
    Vector3 s = { o.x - t->a.x, o.y - t->a.y, o.z - t->a.z };
    float u = (s.x * p.x + s.y * p.y + s.z * p.z) * inv;
    if (u < 0.0f || u > 1.0f) {
        return -1.0f;
    }
    Vector3 q = { s.y * e1.z - s.z * e1.y, s.z * e1.x - s.x * e1.z, s.x * e1.y - s.y * e1.x };
    float v = (d.x * q.x + d.y * q.y + d.z * q.z) * inv;
    if (v < 0.0f || u + v > 1.0f) {
        return -1.0f;
    }
    float f = (e2.x * q.x + e2.y * q.y + e2.z * q.z) * inv;
    return (f >= 0.0f && f <= 1.0f) ? f : -1.0f;
}

/* Um passo curto, de "o" até "o + d", atravessa alguma barreira? */
static int atravessaBarreira(Vector3 o, Vector3 d) {
    int c, l;
    if (!naGrade(&barreiras, o.x + d.x * 0.5f, o.z + d.z * 0.5f, &c, &l)) {
        return 0;
    }
    for (int dl = -1; dl <= 1; dl++) {
        for (int dc = -1; dc <= 1; dc++) {
            int cc = c + dc, ll = l + dl;
            if (cc < 0 || ll < 0 || cc >= barreiras.colunas || ll >= barreiras.linhas) {
                continue;
            }
            int celula = ll * barreiras.colunas + cc;
            for (int k = barreiras.inicio[celula]; k < barreiras.inicio[celula + 1]; k++) {
                if (cortaTriangulo(o, d, &barreiras.triangulos[barreiras.lista[k]]) >= 0.0f) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

/* ==================================================================
 * A linha da pista
 * ================================================================== */

typedef struct {
    float x, z;
} Ponto2;

static Ponto2 linha[MAXIMO_DE_PONTOS];
static int totalDaLinha = 0;

/* A pista de corrida: asfalto com a borracha do traçado por cima */
static int ehAsfalto(float x, float z, float alturaPerto) {
    Embaixo e = chaoEm(x, z, alturaPerto + 4.0f);
    return e.achou && e.chao == CHAO_PISTA && fabsf(e.y - alturaPerto) < 4.0f && temBorracha(x, z, e.y);
}

/* Quanto o asfalto vai para um lado, a partir de (x, z), na direção (dx, dz): até onde vai a
 * borracha, mais a linha branca da borda, que fica fora dela */
static float ateABorda(float x, float z, float dx, float dz, float altura) {
    int fora = 0;
    float ultimoDentro = 0.0f;
    float s = 0.0f;
    for (; s < MAIOR_MEIA_LARGURA; s += PASSO_DE_LADO) {
        if (ehAsfalto(x + dx * s, z + dz * s, altura)) {
            ultimoDentro = s;
            fora = 0;
        } else if (++fora >= 4) {
            break;
        }
    }
    for (s = ultimoDentro + PASSO_DE_LADO; s < ultimoDentro + 2.0f; s += PASSO_DE_LADO) {
        Embaixo e = chaoEm(x + dx * s, z + dz * s, altura + 4.0f);
        if (!e.achou || !ehLinhaDaBorda(e.material)) {
            break;
        }
        ultimoDentro = s;
    }
    return ultimoDentro;
}

/*
 * andarPelaPista
 * Começa na linha de chegada, olhando para +z, e anda de 2 em 2 metros: a cada
 * passo acha as duas bordas do asfalto, corrige o ponto para o meio delas e
 * aponta para onde a pista vai. Para quando volta à largada.
 */
static void andarPelaPista(void) {
    float x = LARGADA_X, z = LARGADA_Z;
    float fx = 0.0f, fz = 1.0f;
    float altura = chaoEm(x, z, 1000.0f).y;
    float andou = 0.0f;
    float larguraDireita = 8.0f, larguraEsquerda = 8.0f;
    totalDaLinha = 0;
    while (totalDaLinha < MAXIMO_DE_PONTOS) {
        float rx = -fz, rz = fx;   /* a direita de quem olha para a frente (o rumo do jogo) */
        Embaixo aqui = chaoEm(x, z, altura + 4.0f);
        if (aqui.achou) {
            altura = aqui.y;
        }
        float direita = ateABorda(x, z, rx, rz, altura);
        float esquerda = ateABorda(x, z, -rx, -rz, altura);
        /* Onde outro asfalto encosta na pista (o traçado antigo, o pit lane), a borda daquele
         * lado salta: ali vale a largura de antes, e a linha segue a borda que não mudou */
        if (totalDaLinha > 0) {
            if (direita > larguraDireita + SALTO_DA_BORDA) direita = larguraDireita;
            if (esquerda > larguraEsquerda + SALTO_DA_BORDA) esquerda = larguraEsquerda;
        }
        if (direita + esquerda < 2.0f) {
            fprintf(stderr, "perdi a pista no passo %d (%.1f, %.1f)\n", totalDaLinha, x, z);
            exit(1);
        }
        float meio = (direita - esquerda) * 0.5f;
        meio = fmaxf(fminf(meio, CORRECAO_MAXIMA), -CORRECAO_MAXIMA);
        larguraDireita = 0.8f * larguraDireita + 0.2f * (direita - meio);
        larguraEsquerda = 0.8f * larguraEsquerda + 0.2f * (esquerda + meio);
        x += rx * meio;
        z += rz * meio;
        linha[totalDaLinha++] = (Ponto2){ x, z };
        if (getenv("DEPURAR")) {
            fprintf(stderr, "passo %d (%.1f, %.1f) y %.2f mat %d larg %.1f + %.1f\n", totalDaLinha, x, z, altura, aqui.material, esquerda, direita);
        }
        if (totalDaLinha > 8) {
            Ponto2 atras = linha[totalDaLinha - 5];
            float nx = x - atras.x, nz = z - atras.z, n = hypotf(nx, nz);
            if (n > 0.1f) {
                fx = 0.5f * fx + 0.5f * nx / n;
                fz = 0.5f * fz + 0.5f * nz / n;
                n = hypotf(fx, fz);
                fx /= n;
                fz /= n;
            }
        }
        x += fx * PASSO_DA_LINHA;
        z += fz * PASSO_DA_LINHA;
        andou += PASSO_DA_LINHA;
        if (andou > 1500.0f && hypotf(x - linha[0].x, z - linha[0].z) < PASSO_DA_LINHA * 1.5f) {
            break;
        }
    }
    fprintf(stderr, "linha: %d pontos, ~%.0f m\n", totalDaLinha, andou);
}

/* Suaviza a linha (média dos vizinhos) e a refaz com pontos a cada PASSO_DA_LINHA, partindo da largada */
static void suavizarEReamostrar(void) {
    static Ponto2 copia[MAXIMO_DE_PONTOS];
    int n = totalDaLinha;
    for (int vez = 0; vez < 6; vez++) {
        for (int i = 0; i < n; i++) {
            float sx = 0.0f, sz = 0.0f;
            for (int k = -2; k <= 2; k++) {
                Ponto2 p = linha[(i + k + n) % n];
                sx += p.x;
                sz += p.z;
            }
            copia[i] = (Ponto2){ sx / 5.0f, sz / 5.0f };
        }
        memcpy(linha, copia, sizeof(Ponto2) * n);
    }
    /* Comprimento total e nova amostragem */
    float total = 0.0f;
    for (int i = 0; i < n; i++) {
        Ponto2 a = linha[i], b = linha[(i + 1) % n];
        total += hypotf(b.x - a.x, b.z - a.z);
    }
    int novos = (int) roundf(total / PASSO_DA_LINHA);
    float passo = total / novos;
    int i = 0;
    float dentro = 0.0f;
    for (int k = 0; k < novos; k++) {
        float alvo = k * passo;
        while (1) {
            Ponto2 a = linha[i % n], b = linha[(i + 1) % n];
            float trecho = hypotf(b.x - a.x, b.z - a.z);
            if (dentro + trecho >= alvo || i >= 2 * n) {
                float t = (trecho > 0.0f) ? (alvo - dentro) / trecho : 0.0f;
                copia[k] = (Ponto2){ a.x + (b.x - a.x) * t, a.z + (b.z - a.z) * t };
                break;
            }
            dentro += trecho;
            i++;
        }
    }
    memcpy(linha, copia, sizeof(Ponto2) * novos);
    totalDaLinha = novos;
    fprintf(stderr, "volta: %.1f m, %d pontos a cada %.3f m\n", total, novos, passo);
}

/* ==================================================================
 * Medidas de cada ponto
 * ================================================================== */

typedef struct {
    float x, z;
    float direita, esquerda;      /* até a borda do asfalto */
    float altura;
    float perfil[PONTOS_DO_PERFIL];
    float zebraDireita, zebraEsquerda;     /* até onde vai a zebra; 0: não tem */
    float escapeDireita, escapeEsquerda;   /* até onde o chão é asfaltado (pista, zebra, escape) */
    float foraDireita[PONTOS_DE_FORA], foraEsquerda[PONTOS_DE_FORA];   /* altura além da borda */
    float muroDireita, muroEsquerda;
} Medida;

static Medida medidas[MAXIMO_DE_PONTOS];

static float alturaEm(float x, float z, float perto) {
    Embaixo e = chaoEm(x, z, perto + 6.0f);
    return e.achou ? e.y : perto;
}

/* Até onde, a partir da borda, a zebra continua (começando até 1,5 m antes ou 1 m depois da borda) */
static float ateOFimDaZebra(float x, float z, float dx, float dz, float borda, float altura) {
    int achou = 0;
    float fim = 0.0f;
    int fora = 0;
    for (float s = borda - 1.5f; s < borda + 6.0f; s += PASSO_DE_LADO) {
        Embaixo e = chaoEm(x + dx * s, z + dz * s, altura + 4.0f);
        int zebra = e.achou && e.chao == CHAO_ZEBRA;
        if (zebra) {
            achou = 1;
            fim = s;
            fora = 0;
        } else if (achou && ++fora >= 4) {
            break;
        } else if (!achou && s > borda + 1.0f) {
            break;
        }
    }
    return achou ? fim : 0.0f;
}

/* Até onde, a partir da borda, o chão é asfaltado (pista, zebra, escape ou pit) */
static float ateOFimDoEscape(float x, float z, float dx, float dz, float borda, float altura, float limite) {
    float fim = borda;
    int fora = 0;
    for (float s = borda; s < limite; s += PASSO_DE_LADO * 2.0f) {
        Embaixo e = chaoEm(x + dx * s, z + dz * s, altura + 6.0f);
        int pavimento = e.achou && (e.chao == CHAO_PISTA || e.chao == CHAO_ZEBRA || e.chao == CHAO_ESCAPE || e.chao == CHAO_PIT);
        if (pavimento) {
            fim = s;
            fora = 0;
        } else if (++fora >= 3) {
            break;
        }
    }
    return fim;
}

/* Até onde o carro anda de lado antes de bater numa barreira */
static float ateOMuro(float x, float z, float dx, float dz, float altura) {
    float chaoAntes = altura;
    for (float s = PASSO_DO_MURO; s < PROCURA_DO_MURO; s += PASSO_DO_MURO) {
        float px = x + dx * s, pz = z + dz * s;
        float g = alturaEm(px, pz, chaoAntes);
        float alturas[2] = { 0.35f, 0.9f };
        for (int k = 0; k < 2; k++) {
            Vector3 o = { px - dx * PASSO_DO_MURO, chaoAntes + alturas[k], pz - dz * PASSO_DO_MURO };
            Vector3 d = { dx * PASSO_DO_MURO, g - chaoAntes, dz * PASSO_DO_MURO };
            if (atravessaBarreira(o, d)) {
                return s - PASSO_DO_MURO;
            }
        }
        chaoAntes = g;
    }
    return PROCURA_DO_MURO;
}

/* A mediana de uma janela de valores (a lista é pequena: ordena por inserção) */
static float mediana(float *valores, int n) {
    for (int i = 1; i < n; i++) {
        float v = valores[i];
        int j = i - 1;
        while (j >= 0 && valores[j] > v) {
            valores[j + 1] = valores[j];
            j--;
        }
        valores[j + 1] = v;
    }
    return valores[n / 2];
}

/*
 * filtrarBordas
 * Onde outro asfalto encosta na pista (a entrada e a saída do pit lane, o traçado antigo),
 * a borda daquele lado salta para longe. A borda de verdade muda devagar: o que passa
 * da mediana dos vizinhos fica na mediana.
 */
static void filtrarBordas(float *borda, int n) {
    static float copia[MAXIMO_DE_PONTOS];
    float janela[2 * JANELA_DA_BORDA + 1];
    for (int i = 0; i < n; i++) {
        for (int k = -JANELA_DA_BORDA; k <= JANELA_DA_BORDA; k++) {
            janela[k + JANELA_DA_BORDA] = borda[(i + k + n) % n];
        }
        float m = mediana(janela, 2 * JANELA_DA_BORDA + 1);
        copia[i] = (borda[i] > m + FOLGA_DA_BORDA) ? m : borda[i];
    }
    memcpy(borda, copia, sizeof(float) * n);
}

static void medirPontos(void) {
    int n = totalDaLinha;
    static float direita[MAXIMO_DE_PONTOS], esquerda[MAXIMO_DE_PONTOS];
    static float rumoX[MAXIMO_DE_PONTOS], rumoZ[MAXIMO_DE_PONTOS];
    float altura = chaoEm(linha[0].x, linha[0].z, 1000.0f).y;

    /* 1) A altura e as bordas cruas */
    for (int i = 0; i < n; i++) {
        Ponto2 a = linha[(i - 1 + n) % n], b = linha[(i + 1) % n];
        float fx = b.x - a.x, fz = b.z - a.z, f = hypotf(fx, fz);
        rumoX[i] = fx / f;
        rumoZ[i] = fz / f;
        float rx = -rumoZ[i], rz = rumoX[i];
        Medida *m = &medidas[i];
        m->x = linha[i].x;
        m->z = linha[i].z;
        altura = alturaEm(m->x, m->z, altura);
        m->altura = altura;
        direita[i] = fmaxf(ateABorda(m->x, m->z, rx, rz, altura), 2.0f);
        esquerda[i] = fmaxf(ateABorda(m->x, m->z, -rx, -rz, altura), 2.0f);
    }
    filtrarBordas(direita, n);
    filtrarBordas(esquerda, n);

    /* 2) O resto, com as bordas de verdade */
    for (int i = 0; i < n; i++) {
        float rx = -rumoZ[i], rz = rumoX[i];
        Medida *m = &medidas[i];
        altura = m->altura;
        m->muroDireita = ateOMuro(m->x, m->z, rx, rz, altura);
        m->muroEsquerda = ateOMuro(m->x, m->z, -rx, -rz, altura);
        m->direita = fminf(direita[i], m->muroDireita);
        m->esquerda = fminf(esquerda[i], m->muroEsquerda);
        for (int k = 0; k < PONTOS_DO_PERFIL; k++) {
            float lado = -m->esquerda + (m->direita + m->esquerda) * k / (PONTOS_DO_PERFIL - 1);
            m->perfil[k] = alturaEm(m->x + rx * lado, m->z + rz * lado, altura) - altura;
        }
        m->zebraDireita = ateOFimDaZebra(m->x, m->z, rx, rz, m->direita, altura);
        m->zebraEsquerda = ateOFimDaZebra(m->x, m->z, -rx, -rz, m->esquerda, altura);
        m->escapeDireita = ateOFimDoEscape(m->x, m->z, rx, rz, m->direita, altura, m->muroDireita);
        m->escapeEsquerda = ateOFimDoEscape(m->x, m->z, -rx, -rz, m->esquerda, altura, m->muroEsquerda);
        /* Onde não há barreira, o carro vai até um pouco além do escape, e não atravessa o
         * gramado até outro trecho da pista */
        m->muroDireita = fminf(m->muroDireita, fmaxf(m->escapeDireita + GRAMA_SEM_MURO, MENOR_LIMITE_SEM_MURO));
        m->muroEsquerda = fminf(m->muroEsquerda, fmaxf(m->escapeEsquerda + GRAMA_SEM_MURO, MENOR_LIMITE_SEM_MURO));
        float ultimoD = m->perfil[PONTOS_DO_PERFIL - 1] + altura, ultimoE = m->perfil[0] + altura;
        for (int k = 0; k < PONTOS_DE_FORA; k++) {
            float sd = m->direita + DISTANCIAS_DE_FORA[k], se = m->esquerda + DISTANCIAS_DE_FORA[k];
            ultimoD = alturaEm(m->x + rx * sd, m->z + rz * sd, ultimoD);
            ultimoE = alturaEm(m->x - rx * se, m->z - rz * se, ultimoE);
            m->foraDireita[k] = ultimoD - altura;
            m->foraEsquerda[k] = ultimoE - altura;
        }
    }
}

/* ==================================================================
 * O pit lane
 * ================================================================== */

typedef struct {
    float metros;     /* a partir da linha de chegada, pela linha da pista */
    float dentro;     /* metros da linha da pista até a borda do pit lane do lado da pista */
    float fora;       /* ...e até a borda do outro lado (onde ficam as garagens) */
} PontoDoPit;

static PontoDoPit pit[MAXIMO_DE_PONTOS];
static int totalDoPit = 0;

static int ehPisoDoPit(float x, float z, float alturaPerto) {
    Embaixo e = chaoEm(x, z, alturaPerto + 3.0f);
    return e.achou && (e.chao == CHAO_PISTA || e.chao == CHAO_PIT) && fabsf(e.y - alturaPerto) < 3.0f
           && !temBorracha(x, z, e.y);
}

static float ateABordaDoPit(float x, float z, float dx, float dz, float altura) {
    int fora = 0;
    float ultimoDentro = 0.0f;
    for (float s = 0.0f; s < MAIOR_MEIA_LARGURA; s += PASSO_DE_LADO) {
        if (ehPisoDoPit(x + dx * s, z + dz * s, altura)) {
            ultimoDentro = s;
            fora = 0;
        } else if (++fora >= 4) {
            break;
        }
    }
    return ultimoDentro;
}

/* O ponto da linha da pista mais perto de (x, z), procurando perto de "palpite" */
static int pontoMaisPerto(float x, float z, int palpite, int raio) {
    int n = totalDaLinha, melhor = palpite;
    float menor = 1e18f;
    for (int k = -raio; k <= raio; k++) {
        int i = ((palpite + k) % n + n) % n;
        float d = (medidas[i].x - x) * (medidas[i].x - x) + (medidas[i].z - z) * (medidas[i].z - z);
        if (d < menor) {
            menor = d;
            melhor = i;
        }
    }
    return melhor;
}

/* Onde (x, z) fica em relação à pista: metros desde a linha e quanto para a direita */
static void naPista(float x, float z, int *indice, float *metros, float *lateral) {
    int n = totalDaLinha;
    *indice = pontoMaisPerto(x, z, *indice, (*indice < 0) ? n : 40);
    int i = *indice;
    Medida *a = &medidas[i], *b = &medidas[(i + 1) % n];
    float fx = b->x - a->x, fz = b->z - a->z, f = hypotf(fx, fz);
    fx /= f;
    fz /= f;
    float adiante = (x - a->x) * fx + (z - a->z) * fz;
    *metros = i * PASSO_DA_LINHA + adiante;
    *lateral = (x - a->x) * (-fz) + (z - a->z) * fx;
}

/*
 * andarPeloPit
 * Começa no meio do pit lane, na frente dos boxes, e anda nos dois sentidos até ele
 * encostar na pista. Guarda, a cada passo, onde ele fica em relação à pista.
 */
static void andarPeloPit(void) {
    static PontoDoPit ida[MAXIMO_DE_PONTOS], volta[MAXIMO_DE_PONTOS];
    int totais[2] = { 0, 0 };
    PontoDoPit *saida[2] = { volta, ida };
    float n = (float) totalDaLinha;
    for (int sentido = 0; sentido < 2; sentido++) {
        float x = PIT_X, z = PIT_Z, fz = sentido ? 1.0f : -1.0f, fx = 0.0f;
        float altura = alturaEm(x, z, 16.0f);
        float largD = 6.0f, largE = 6.0f;
        int indice = -1;
        int encostou = 0;
        Ponto2 atras[8];
        int passos = 0;
        while (passos < 1000) {
            float rx = -fz, rz = fx;
            altura = alturaEm(x, z, altura);
            float d = ateABordaDoPit(x, z, rx, rz, altura), e = ateABordaDoPit(x, z, -rx, -rz, altura);
            if (passos > 0) {
                if (d > largD + SALTO_DA_BORDA) d = largD;
                if (e > largE + SALTO_DA_BORDA) e = largE;
            }
            if (getenv("DEPURAR")) {
                Embaixo aqui = chaoEm(x, z, altura + 3.0f);
                fprintf(stderr, "pit %d sentido %d (%.1f, %.1f) y %.2f mat %d d %.1f e %.1f\n", passos, sentido, x, z, altura, aqui.material, d, e);
            }
            if (!ehPisoDoPit(x, z, altura)) {
                /* Saiu do piso (numa curva do pit lane): procura o piso mais perto, de lado */
                int achou = 0;
                for (float lado = PASSO_DE_LADO; lado < 6.0f && !achou; lado += PASSO_DE_LADO) {
                    for (int sinal = -1; sinal <= 1 && !achou; sinal += 2) {
                        float px = x + rx * lado * sinal, pz = z + rz * lado * sinal;
                        if (ehPisoDoPit(px, pz, altura)) {
                            x = px;
                            z = pz;
                            achou = 1;
                        }
                    }
                }
                d = ateABordaDoPit(x, z, rx, rz, altura);
                e = ateABordaDoPit(x, z, -rx, -rz, altura);
            }
            if (d + e < LARGURA_MINIMA_DO_PIT) {
                break;   /* afinou até acabar: é onde ele encosta na pista */
            }
            float meio = fmaxf(fminf((d - e) * 0.5f, CORRECAO_MAXIMA), -CORRECAO_MAXIMA);
            x += rx * meio;
            z += rz * meio;
            largD = 0.8f * largD + 0.2f * (d - meio);
            largE = 0.8f * largE + 0.2f * (e + meio);
            float metros, lateral;
            naPista(x, z, &indice, &metros, &lateral);
            /* O pit lane fica à esquerda da pista: as bordas, em distância para a esquerda */
            /* Andando para +z, a pista fica à direita; para -z, à esquerda */
            float paraAPista = sentido ? largD : largE, paraFora = sentido ? largE : largD;
            float bordaDentro = -lateral - paraAPista;
            float bordaFora = -lateral + paraFora;
            if (metros > n * PASSO_DA_LINHA * 0.5f) {
                metros -= n * PASSO_DA_LINHA;   /* antes da linha: negativo */
            }
            /* Encostado na pista (só a linha pintada entre os dois), o pit lane está na ponta:
             * se dali ele volta a se afastar, a caminhada entrou em outra rua */
            float bordaDaPista = medidas[indice].esquerda;
            if (bordaDentro < bordaDaPista + 1.5f) {
                encostou = 1;
            } else if (encostou && bordaDentro > bordaDaPista + 3.0f) {
                break;
            }
            PontoDoPit *novo = &saida[sentido][totais[sentido]++];
            novo->metros = metros;
            novo->dentro = bordaDentro;
            novo->fora = bordaFora;
            atras[passos % 8] = (Ponto2){ x, z };
            if (passos >= 8) {
                Ponto2 velho = atras[(passos + 1) % 8];
                float nx = x - velho.x, nz = z - velho.z, nn = hypotf(nx, nz);
                if (nn > 0.1f) {
                    fx = 0.5f * fx + 0.5f * nx / nn;
                    fz = 0.5f * fz + 0.5f * nz / nn;
                    nn = hypotf(fx, fz);
                    fx /= nn;
                    fz /= nn;
                }
            }
            x += fx * PASSO_DA_LINHA;
            z += fz * PASSO_DA_LINHA;
            passos++;
        }
    }
    /* Junta: a volta (de trás para a frente) e depois a ida */
    totalDoPit = 0;
    for (int i = totais[0] - 1; i >= 1; i--) {
        pit[totalDoPit++] = volta[i];
    }
    for (int i = 0; i < totais[1]; i++) {
        pit[totalDoPit++] = ida[i];
    }
    fprintf(stderr, "pit lane: %d pontos, de %.0f m a %.0f m\n", totalDoPit, pit[0].metros, pit[totalDoPit - 1].metros);
}

/* ==================================================================
 * A torcida
 * ================================================================== */

typedef struct {
    float x, y, z;
} Lugar;

static Lugar *lugares = NULL;
static int totalDeLugares = 0;

/* Um lugar a cada ~1,6 m dos assentos: o mais alto de cada quadradinho visto de cima */
static void acharLugaresDaTorcida(void) {
    const float lado = 1.6f;
    int capacidade = 20000;
    lugares = malloc(sizeof(Lugar) * capacidade);
    for (int l = 0; l < assentos.linhas; l++) {
        for (int c = 0; c < assentos.colunas; c++) {
            for (float sz = 0.0f; sz < LADO_DA_CELULA; sz += lado) {
                for (float sx = 0.0f; sx < LADO_DA_CELULA; sx += lado) {
                    float x = assentos.x0 + c * LADO_DA_CELULA + sx + lado * 0.5f;
                    float z = assentos.z0 + l * LADO_DA_CELULA + sz + lado * 0.5f;
                    int celula = l * assentos.colunas + c;
                    float melhor = -1e9f;
                    for (int k = assentos.inicio[celula]; k < assentos.inicio[celula + 1]; k++) {
                        Triangulo *t = &assentos.triangulos[assentos.lista[k]];
                        /* Só as faces de cima (o assento), não a frente dos degraus */
                        Vector3 e1 = { t->b.x - t->a.x, t->b.y - t->a.y, t->b.z - t->a.z };
                        Vector3 e2 = { t->c.x - t->a.x, t->c.y - t->a.y, t->c.z - t->a.z };
                        Vector3 nn = { e1.y * e2.z - e1.z * e2.y, e1.z * e2.x - e1.x * e2.z, e1.x * e2.y - e1.y * e2.x };
                        float comp = sqrtf(nn.x * nn.x + nn.y * nn.y + nn.z * nn.z);
                        if (comp < 1e-6f || fabsf(nn.y) / comp < 0.7f) {
                            continue;
                        }
                        float d = (t->b.z - t->c.z) * (t->a.x - t->c.x) + (t->c.x - t->b.x) * (t->a.z - t->c.z);
                        if (fabsf(d) < 1e-9f) continue;
                        float u = ((t->b.z - t->c.z) * (x - t->c.x) + (t->c.x - t->b.x) * (z - t->c.z)) / d;
                        float v = ((t->c.z - t->a.z) * (x - t->c.x) + (t->a.x - t->c.x) * (z - t->c.z)) / d;
                        if (u < 0 || v < 0 || u + v > 1) continue;
                        float y = u * t->a.y + v * t->b.y + (1 - u - v) * t->c.y;
                        if (y > melhor) melhor = y;
                    }
                    if (melhor > -1e8f && totalDeLugares < capacidade) {
                        lugares[totalDeLugares++] = (Lugar){ x, melhor, z };
                    }
                }
            }
        }
    }
    fprintf(stderr, "torcida: %d lugares\n", totalDeLugares);
}

/* ==================================================================
 * Saída
 * ================================================================== */

static void escrever(const char *arquivo) {
    FILE *f = fopen(arquivo, "w");
    if (f == NULL) {
        perror(arquivo);
        exit(1);
    }
    int n = totalDaLinha;
    fprintf(f, "/*\n * interlagos.h\n * ------------------------------------------------------------------\n");
    fprintf(f, " * Interlagos medido no modelo recursos/modelos/interlagos.glb, em metros e nos\n");
    fprintf(f, " * eixos do próprio modelo. GERADO por ferramentas/medir_interlagos.c: não edite\n");
    fprintf(f, " * à mão; rode \"make medir-interlagos\".\n *\n");
    fprintf(f, " * A linha passa no meio do asfalto, a cada ~%.0f m, a partir da linha de chegada\n", PASSO_DA_LINHA);
    fprintf(f, " * e no sentido da corrida (anti-horário).\n");
    fprintf(f, " * ------------------------------------------------------------------\n */\n\n");
    fprintf(f, "#ifndef INTERLAGOS_H\n#define INTERLAGOS_H\n\n#include \"tracado.h\"\n\n");

    fprintf(f, "static const PontoDoTracado TRACADO_INTERLAGOS[] = {\n");
    for (int i = 0; i < n; i++) {
        Medida *m = &medidas[i];
        fprintf(f, "    { %9.3ff, %9.3ff, %6.3ff, %6.3ff, %8.3ff, {", m->x, m->z, m->direita, m->esquerda, m->altura);
        for (int k = 0; k < PONTOS_DO_PERFIL; k++) {
            fprintf(f, " %7.3ff%s", m->perfil[k], k + 1 < PONTOS_DO_PERFIL ? "," : "");
        }
        fprintf(f, " } },\n");
    }
    fprintf(f, "};\n#define TOTAL_PONTOS_INTERLAGOS (int) (sizeof(TRACADO_INTERLAGOS) / sizeof(TRACADO_INTERLAGOS[0]))\n\n");

    fprintf(f, "/* Até o que para o carro (muro, guard-rail, pneus), em metros da linha: { direita, esquerda } */\n");
    fprintf(f, "static const float LIMITES_INTERLAGOS[][2] = {\n");
    for (int i = 0; i < n; i++) {
        fprintf(f, "    { %6.2ff, %6.2ff },\n", medidas[i].muroDireita, medidas[i].muroEsquerda);
    }
    fprintf(f, "};\n\n");

    fprintf(f, "/* O que fica além da borda do asfalto, em metros da linha */\n");
    fprintf(f, "static const BordaDoTracado BORDAS_INTERLAGOS[] = {\n");
    for (int i = 0; i < n; i++) {
        Medida *m = &medidas[i];
        fprintf(f, "    { %6.2ff, %6.2ff, %6.2ff, %6.2ff, {", m->zebraDireita, m->zebraEsquerda, m->escapeDireita, m->escapeEsquerda);
        for (int k = 0; k < PONTOS_DE_FORA; k++) fprintf(f, " %6.2ff%s", m->foraDireita[k], k + 1 < PONTOS_DE_FORA ? "," : "");
        fprintf(f, " }, {");
        for (int k = 0; k < PONTOS_DE_FORA; k++) fprintf(f, " %6.2ff%s", m->foraEsquerda[k], k + 1 < PONTOS_DE_FORA ? "," : "");
        fprintf(f, " } },\n");
    }
    fprintf(f, "};\n\n");

    fprintf(f, "/* O pit lane, da entrada à saída: { metros desde a linha (antes dela, negativo),\n");
    fprintf(f, " * metros da linha da pista até a borda do pit lane do lado da pista, até a outra borda } */\n");
    fprintf(f, "static const float PIT_INTERLAGOS[][3] = {\n");
    for (int i = 0; i < totalDoPit; i++) {
        fprintf(f, "    { %8.2ff, %6.2ff, %6.2ff },\n", pit[i].metros, pit[i].dentro, pit[i].fora);
    }
    fprintf(f, "};\n#define TOTAL_PONTOS_PIT_INTERLAGOS (int) (sizeof(PIT_INTERLAGOS) / sizeof(PIT_INTERLAGOS[0]))\n\n");

    fprintf(f, "/* Os assentos das arquibancadas do modelo */\n");
    fprintf(f, "static const LugarNaTorcida TORCIDA_INTERLAGOS[] = {\n");
    for (int i = 0; i < totalDeLugares; i++) {
        fprintf(f, "    { %8.2ff, %7.2ff, %8.2ff },\n", lugares[i].x, lugares[i].y, lugares[i].z);
    }
    fprintf(f, "};\n#define TOTAL_TORCIDA_INTERLAGOS (int) (sizeof(TORCIDA_INTERLAGOS) / sizeof(TORCIDA_INTERLAGOS[0]))\n\n");
    fprintf(f, "#endif\n");
    fclose(f);
}

/* ==================================================================
 * Sonda: o que tem no chão ao longo de uma linha (para estudar o modelo)
 * ================================================================== */

static void sondar(float x0, float z0, float x1, float z1, int n) {
    for (int k = 0; k < n; k++) {
        float t = (n > 1) ? (float) k / (n - 1) : 0.0f;
        float x = x0 + (x1 - x0) * t, z = z0 + (z1 - z0) * t;
        Embaixo e = chaoEm(x, z, 1000.0f);
        printf("%8.2f %8.2f  material %3d  chao %d  y %.2f\n", x, z, e.material, e.chao, e.y);
    }
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "uso: %s modelo.glb saida.h | --sonda x0 z0 x1 z1 n\n", argv[0]);
        return 1;
    }
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(64, 64, "medir_interlagos");
    Model modelo = LoadModel(argv[1]);
    if (modelo.materialCount != TOTAL_MATERIAIS_INTERLAGOS) {
        fprintf(stderr, "o modelo tem %d materiais; interlagos_materiais.h espera %d\n", modelo.materialCount,
                TOTAL_MATERIAIS_INTERLAGOS);
        return 1;
    }
    montarGrade(&chao, &modelo, ehChao);
    montarGrade(&barreiras, &modelo, ehBarreira);
    montarGrade(&assentos, &modelo, ehAssento);
    montarGrade(&borracha, &modelo, ehBorracha);
    fprintf(stderr, "chao: %d triangulos, barreiras: %d, assentos: %d\n", chao.total, barreiras.total, assentos.total);

    if (strcmp(argv[2], "--sonda") == 0 && argc >= 8) {
        sondar(atof(argv[3]), atof(argv[4]), atof(argv[5]), atof(argv[6]), atoi(argv[7]));
        return 0;
    }
    andarPelaPista();
    suavizarEReamostrar();
    medirPontos();
    andarPeloPit();
    acharLugaresDaTorcida();
    escrever(argv[2]);
    fprintf(stderr, "escrito: %s\n", argv[2]);
    return 0;
}
