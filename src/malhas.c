/*
 * malhas.c
 * ------------------------------------------------------------------
 * Cortar uma malha pelas ilhas. Veja malhas.h.
 *
 * As ilhas saem de um conjunto-união: cada triângulo une os seus três
 * vértices, e os vértices no mesmo lugar (as emendas da textura, que
 * repetem o vértice) também se unem, achados ordenando por posição.
 * ------------------------------------------------------------------
 */

#include <stdlib.h>
#include <string.h>
#include "malhas.h"

static const float *posicoesParaOrdenar;

static int compararPosicao(const void *a, const void *b) {
    int i = *(const int *) a, j = *(const int *) b;
    for (int k = 0; k < 3; k++) {
        float d = posicoesParaOrdenar[i * 3 + k] - posicoesParaOrdenar[j * 3 + k];
        if (d != 0.0f) {
            return (d < 0.0f) ? -1 : 1;
        }
    }
    return 0;
}

static int raiz(int *pai, int a) {
    while (pai[a] != a) {
        pai[a] = pai[pai[a]];
        a = pai[a];
    }
    return a;
}

static void unir(int *pai, int a, int b) {
    a = raiz(pai, a);
    b = raiz(pai, b);
    if (a != b) {
        pai[a] = b;
    }
}

static int verticeDoTriangulo(Mesh malha, int triangulo, int canto) {
    return (malha.indices != NULL) ? malha.indices[triangulo * 3 + canto] : triangulo * 3 + canto;
}

/* Copia um atributo de cada vértice usado (componentes de "tamanho" bytes) */
static void *copiarAtributo(const void *de, int componentes, size_t tamanho, const int *antigo, int usados) {
    if (de == NULL) {
        return NULL;
    }
    size_t porVertice = (size_t) componentes * tamanho;
    unsigned char *para = (unsigned char *) RL_MALLOC((size_t) usados * porVertice);
    for (int v = 0; v < usados; v++) {
        memcpy(para + (size_t) v * porVertice, (const unsigned char *) de + (size_t) antigo[v] * porVertice, porVertice);
    }
    return para;
}

/* A malha com os triângulos de um lado, com os vértices renumerados, já na placa de vídeo */
static Mesh copiarLado(Mesh malha, int *pai, const unsigned char *lado, int qual, int triangulos) {
    int *novo = (int *) malloc((size_t) malha.vertexCount * sizeof(int));
    int *antigo = (int *) malloc((size_t) malha.vertexCount * sizeof(int));
    int usados = 0;
    for (int v = 0; v < malha.vertexCount; v++) {
        novo[v] = -1;
    }
    Mesh m = { 0 };
    m.triangleCount = triangulos;
    m.indices = (unsigned short *) RL_MALLOC((size_t) triangulos * 3 * sizeof(unsigned short));
    int t2 = 0;
    for (int t = 0; t < malha.triangleCount; t++) {
        if (lado[raiz(pai, verticeDoTriangulo(malha, t, 0))] != qual) {
            continue;
        }
        for (int k = 0; k < 3; k++) {
            int v = verticeDoTriangulo(malha, t, k);
            if (novo[v] < 0) {
                novo[v] = usados;
                antigo[usados++] = v;
            }
            m.indices[t2 * 3 + k] = (unsigned short) novo[v];
        }
        t2++;
    }
    m.vertexCount = usados;
    m.vertices = (float *) copiarAtributo(malha.vertices, 3, sizeof(float), antigo, usados);
    m.texcoords = (float *) copiarAtributo(malha.texcoords, 2, sizeof(float), antigo, usados);
    m.texcoords2 = (float *) copiarAtributo(malha.texcoords2, 2, sizeof(float), antigo, usados);
    m.normals = (float *) copiarAtributo(malha.normals, 3, sizeof(float), antigo, usados);
    m.tangents = (float *) copiarAtributo(malha.tangents, 4, sizeof(float), antigo, usados);
    m.colors = (unsigned char *) copiarAtributo(malha.colors, 4, 1, antigo, usados);
    free(novo);
    free(antigo);
    UploadMesh(&m, false);
    return m;
}

int separarMalhaPorIlhas(Mesh malha, EscolhaDaIlha escolha, void *dados, Mesh *primeira, Mesh *segunda) {
    int nv = malha.vertexCount, nt = malha.triangleCount;
    if (malha.vertices == NULL || nv <= 0 || nt <= 0 || nv > 65535) {
        return 0;
    }
    int *pai = (int *) malloc((size_t) nv * sizeof(int));
    int *ordem = (int *) malloc((size_t) nv * sizeof(int));
    Vector3 *minimo = (Vector3 *) malloc((size_t) nv * sizeof(Vector3));
    Vector3 *maximo = (Vector3 *) malloc((size_t) nv * sizeof(Vector3));
    unsigned char *visto = (unsigned char *) calloc((size_t) nv, 1);
    unsigned char *lado = (unsigned char *) calloc((size_t) nv, 1);
    for (int v = 0; v < nv; v++) {
        pai[v] = ordem[v] = v;
    }
    posicoesParaOrdenar = malha.vertices;
    qsort(ordem, (size_t) nv, sizeof(int), compararPosicao);
    for (int k = 1; k < nv; k++) {
        if (compararPosicao(&ordem[k - 1], &ordem[k]) == 0) {
            unir(pai, ordem[k - 1], ordem[k]);
        }
    }
    for (int t = 0; t < nt; t++) {
        int a = verticeDoTriangulo(malha, t, 0);
        unir(pai, a, verticeDoTriangulo(malha, t, 1));
        unir(pai, a, verticeDoTriangulo(malha, t, 2));
    }
    /* A caixa de cada ilha */
    for (int t = 0; t < nt; t++) {
        for (int k = 0; k < 3; k++) {
            int v = verticeDoTriangulo(malha, t, k), r = raiz(pai, v);
            Vector3 p = { malha.vertices[v * 3], malha.vertices[v * 3 + 1], malha.vertices[v * 3 + 2] };
            if (!visto[r]) {
                visto[r] = 1;
                minimo[r] = maximo[r] = p;
            } else {
                minimo[r] = (Vector3){ (p.x < minimo[r].x) ? p.x : minimo[r].x, (p.y < minimo[r].y) ? p.y : minimo[r].y,
                                       (p.z < minimo[r].z) ? p.z : minimo[r].z };
                maximo[r] = (Vector3){ (p.x > maximo[r].x) ? p.x : maximo[r].x, (p.y > maximo[r].y) ? p.y : maximo[r].y,
                                       (p.z > maximo[r].z) ? p.z : maximo[r].z };
            }
        }
    }
    for (int r = 0; r < nv; r++) {
        if (visto[r]) {
            lado[r] = escolha(minimo[r], maximo[r], dados) ? 1 : 0;
        }
    }
    int triangulos[2] = { 0, 0 };
    for (int t = 0; t < nt; t++) {
        triangulos[lado[raiz(pai, verticeDoTriangulo(malha, t, 0))]]++;
    }
    int deu = triangulos[0] > 0 && triangulos[1] > 0;
    if (deu) {
        *primeira = copiarLado(malha, pai, lado, 0, triangulos[0]);
        *segunda = copiarLado(malha, pai, lado, 1, triangulos[1]);
    }
    free(pai);
    free(ordem);
    free(minimo);
    free(maximo);
    free(visto);
    free(lado);
    return deu;
}

int separarModeloPorIlhas(Model *modelo, int qual, EscolhaDaIlha escolha, void *dados) {
    if (qual < 0 || qual >= modelo->meshCount) {
        return -1;
    }
    Mesh a, b;
    if (!separarMalhaPorIlhas(modelo->meshes[qual], escolha, dados, &a, &b)) {
        return -1;
    }
    Mesh *malhas = (Mesh *) RL_REALLOC(modelo->meshes, (size_t) (modelo->meshCount + 1) * sizeof(Mesh));
    if (malhas == NULL) {
        UnloadMesh(a);
        UnloadMesh(b);
        return -1;
    }
    modelo->meshes = malhas;
    int *materiais = (int *) RL_REALLOC(modelo->meshMaterial, (size_t) (modelo->meshCount + 1) * sizeof(int));
    if (materiais == NULL) {
        UnloadMesh(a);
        UnloadMesh(b);
        return -1;
    }
    modelo->meshMaterial = materiais;
    UnloadMesh(modelo->meshes[qual]);
    modelo->meshes[qual] = a;
    modelo->meshes[modelo->meshCount] = b;
    modelo->meshMaterial[modelo->meshCount] = modelo->meshMaterial[qual];
    return modelo->meshCount++;
}
