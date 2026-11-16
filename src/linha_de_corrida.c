/*
 * linha_de_corrida.c
 * ------------------------------------------------------------------
 * A linha de corrida ideal. Veja linha_de_corrida.h.
 *
 * A linha é a de menor curvatura que cabe no asfalto: um ponto a cada
 * AMOSTRA_EM_SEGMENTOS segmentos, cada um preso à perpendicular da pista ali,
 * entre as duas bordas (menos a folga do carro). Cada ponto é puxado, muitas
 * vezes, para onde a curva da linha em volta dele fica mais suave: o ponto que
 * a média ponderada dos vizinhos (-1, 4, 4, -1)/6 pede, que é o que anula a
 * variação da curvatura. Começando com vizinhos longe (a forma da curva toda)
 * e terminando com os de perto (o detalhe), a linha sai como a de verdade:
 * aberta na entrada, por dentro no meio da curva, aberta de novo na saída, e
 * nas chicanes, quase reta por cima das zebras.
 *
 * O raio de cada ponto sai de três pontos da linha (a 10 m um do outro), e a
 * curvatura é alisada com os vizinhos: a velocidade da curva é a raiz de
 * (o que o pneu segura vezes o raio), conta que render.c faz com o carro.
 * ------------------------------------------------------------------
 */

#include <math.h>
#include <stdlib.h>
#include "linha_de_corrida.h"

#define AMOSTRA_EM_SEGMENTOS  3       /* um ponto da linha a cada 3 segmentos (5 m) */
#define FOLGA_DA_BORDA        1.4f    /* metros: o meio do carro fica tanto para dentro da borda do asfalto */
#define PASSADAS_POR_ESCALA   300
#define PUXAO                 0.5f    /* quanto de cada vez o ponto anda para onde a média pede */
#define RAIO_DA_RETA          1.0e6f
#define GRAUS_PARA_RADIANOS   0.017453292f

/*
 * calcularLinha
 * Recebe: a pista.
 * Devolve: o vetor da linha, por segmento: { lateral, raio } (NULL se faltar memória).
 */
static float *calcularLinha(Pista *pista) {
    int segmentos = contarSegmentos(pista);
    int n = segmentos / AMOSTRA_EM_SEGMENTOS;
    if (n < 8) {
        return NULL;
    }
    float *bx = malloc(sizeof(float) * (size_t) n * 8);
    float *saida = malloc(sizeof(float) * (size_t) segmentos * 2);
    if (bx == NULL || saida == NULL) {
        free(bx);
        free(saida);
        return NULL;
    }
    float *bz = bx + n, *nx = bx + 2 * n, *nz = bx + 3 * n, *menor = bx + 4 * n, *maior = bx + 5 * n;
    float *lateral = bx + 6 * n, *curvatura = bx + 7 * n;

    /* A pista nos pontos da linha: o meio, a direita de quem anda e até onde dá para ir de cada lado */
    Segmento *atual = pista->largada;
    for (int j = 0; j < n; j++) {
        float rumo = atual->rumo * GRAUS_PARA_RADIANOS;
        bx[j] = atual->xMundo;
        bz[j] = atual->zMundo;
        nx[j] = cosf(rumo);
        nz[j] = sinf(rumo);
        menor[j] = -(atual->larguraEsquerda - FOLGA_DA_BORDA);
        maior[j] = atual->larguraDireita - FOLGA_DA_BORDA;
        if (menor[j] > maior[j]) {
            menor[j] = maior[j] = (menor[j] + maior[j]) / 2.0f;
        }
        lateral[j] = 0.0f;
        for (int k = 0; k < AMOSTRA_EM_SEGMENTOS; k++) {
            atual = atual->proximo;
        }
    }

    /* A linha, de longe para perto */
    static const int ESCALAS[] = { 16, 8, 4, 2, 1 };
    for (int e = 0; e < (int) (sizeof(ESCALAS) / sizeof(ESCALAS[0])); e++) {
        int h = ESCALAS[e];
        for (int passada = 0; passada < PASSADAS_POR_ESCALA; passada++) {
            for (int j = 0; j < n; j++) {
                int a = (j - 2 * h + 2 * n) % n, b = (j - h + n) % n, c = (j + h) % n, d = (j + 2 * h) % n;
                float wax = bx[a] + lateral[a] * nx[a], waz = bz[a] + lateral[a] * nz[a];
                float wbx = bx[b] + lateral[b] * nx[b], wbz = bz[b] + lateral[b] * nz[b];
                float wcx = bx[c] + lateral[c] * nx[c], wcz = bz[c] + lateral[c] * nz[c];
                float wdx = bx[d] + lateral[d] * nx[d], wdz = bz[d] + lateral[d] * nz[d];
                float alvoX = (-wax + 4.0f * wbx + 4.0f * wcx - wdx) / 6.0f;
                float alvoZ = (-waz + 4.0f * wbz + 4.0f * wcz - wdz) / 6.0f;
                float quer = (alvoX - bx[j]) * nx[j] + (alvoZ - bz[j]) * nz[j];
                float novo = lateral[j] + PUXAO * (quer - lateral[j]);
                lateral[j] = fminf(fmaxf(novo, menor[j]), maior[j]);
            }
        }
    }

    /* A curvatura da linha (com sinal: positiva para a direita), de três pontos a 10 m, alisada */
    for (int j = 0; j < n; j++) {
        int a = (j - 2 + n) % n, c = (j + 2) % n;
        float ax = bx[a] + lateral[a] * nx[a], az = bz[a] + lateral[a] * nz[a];
        float px = bx[j] + lateral[j] * nx[j], pz = bz[j] + lateral[j] * nz[j];
        float cx = bx[c] + lateral[c] * nx[c], cz = bz[c] + lateral[c] * nz[c];
        float ux = px - ax, uz = pz - az, vx = cx - px, vz = cz - pz, wx = cx - ax, wz = cz - az;
        float cruz = ux * vz - uz * vx;
        float produto = sqrtf((ux * ux + uz * uz) * (vx * vx + vz * vz) * (wx * wx + wz * wz));
        curvatura[j] = (produto > 1e-6f) ? 2.0f * cruz / produto : 0.0f;
    }
    float *alisada = malloc(sizeof(float) * (size_t) n);
    if (alisada != NULL) {
        for (int j = 0; j < n; j++) {
            float soma = 0.0f;
            for (int k = -2; k <= 2; k++) {
                soma += curvatura[(j + k + n) % n];
            }
            alisada[j] = soma / 5.0f;
        }
        for (int j = 0; j < n; j++) {
            curvatura[j] = alisada[j];
        }
        free(alisada);
    }

    /* Em cada segmento, entre os dois pontos da linha em volta dele */
    for (int i = 0; i < segmentos; i++) {
        int j = (i / AMOSTRA_EM_SEGMENTOS) % n, k = (j + 1) % n;
        float t = (float) (i % AMOSTRA_EM_SEGMENTOS) / AMOSTRA_EM_SEGMENTOS;
        if (i >= n * AMOSTRA_EM_SEGMENTOS) {
            /* Os últimos segmentos (a sobra da divisão): entre o último ponto e o primeiro */
            j = n - 1;
            k = 0;
            t = (float) (i - j * AMOSTRA_EM_SEGMENTOS) / (float) (segmentos - j * AMOSTRA_EM_SEGMENTOS);
        }
        saida[2 * i] = lateral[j] + (lateral[k] - lateral[j]) * t;
        float kappa = fabsf(curvatura[j] + (curvatura[k] - curvatura[j]) * t);
        saida[2 * i + 1] = (kappa > 1.0f / RAIO_DA_RETA) ? 1.0f / kappa : RAIO_DA_RETA;
    }
    free(bx);
    return saida;
}

static const float *linhaDaPista(Pista *pista) {
    if (pista->linhaDeCorrida == NULL && pista->largada != NULL) {
        pista->linhaDeCorrida = calcularLinha(pista);
    }
    return pista->linhaDeCorrida;
}

float lateralDaLinhaDeCorrida(Pista *pista, PosicaoNaPista lugar) {
    const float *linha = linhaDaPista(pista);
    if (linha == NULL || lugar.segmento == NULL) {
        return 0.0f;
    }
    int i = lugar.segmento->indice, k = lugar.segmento->proximo->indice;
    float t = lugar.distanciaNoSegmento / COMPRIMENTO_SEGMENTO;
    return linha[2 * i] + (linha[2 * k] - linha[2 * i]) * t;
}

float raioDaLinhaDeCorrida(Pista *pista, PosicaoNaPista lugar) {
    const float *linha = linhaDaPista(pista);
    if (linha == NULL || lugar.segmento == NULL) {
        return RAIO_DA_RETA;
    }
    int i = lugar.segmento->indice, k = lugar.segmento->proximo->indice;
    float t = lugar.distanciaNoSegmento / COMPRIMENTO_SEGMENTO;
    float a = 1.0f / linha[2 * i + 1], b = 1.0f / linha[2 * k + 1];
    float kappa = a + (b - a) * t;
    return (kappa > 1.0f / RAIO_DA_RETA) ? 1.0f / kappa : RAIO_DA_RETA;
}
