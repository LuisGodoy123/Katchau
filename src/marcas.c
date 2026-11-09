/*
 * marcas.c
 * ------------------------------------------------------------------
 * As marcas de pneu. Veja marcas.h.
 *
 * Cada roda lembra o último ponto onde marcou: o pedaço novo vai dali até o
 * ponto de agora, como uma faixa. As marcas ficam numa fila circular: quando
 * ela enche, a mais velha dá lugar à nova.
 * ------------------------------------------------------------------
 */

#include <math.h>
#include "marcas.h"
#include "rlgl.h"
#include "raymath.h"
#include "corte.h"

#define MAXIMO_DE_MARCAS      6000
#define CARROS                16
#define RODAS                 4
#define LARGURA_DA_MARCA      0.34f    /* metros: a banda de rodagem */
#define ACIMA_DO_CHAO         0.03f    /* metros: para não piscar com o asfalto */
#define PASSO_DA_MARCA        0.6f     /* metros entre um pedaço e o próximo */
#define SALTO_MAXIMO          4.0f     /* metros: mais que isso, a faixa recomeça */
#define VIDA_DA_MARCA         45.0f    /* segundos até sumir de vez */
#define SUMINDO               12.0f    /* segundos finais, desbotando */
#define ESCURIDAO             0.80f    /* o quanto a marca mais forte escurece o asfalto */
#define COMPRIMENTO_DA_IMAGEM 1.6f     /* metros de rastro que a imagem cobre, antes de repetir */
/* A folga do corte: a meia-diagonal de um pedaço (0,34 de largura por 0,6 de passo é 0,35),
 * arredondada para cima — assim o teste é conservador e nunca corta uma marca que aparece */
#define FOLGA_DO_CORTE        0.5f
#define LARGURA_DA_IMAGEM     64
#define ALTURA_DA_IMAGEM      256

typedef struct {
    Vector3 cantos[4];
    Vector3 centro;   /* o meio dos quatro cantos, para o corte pelo campo de visão (corte.c) */
    float v0, v1;     /* onde a imagem começa e termina, ao longo do rastro */
    float forca;
    float idade;
} Marca;

typedef struct {
    int marcando;
    Vector3 ultimo;
    Vector3 ultimaEsquerda, ultimaDireita;
    float v;          /* quanto da imagem o rastro já usou: o pedaço novo continua de onde parou */
} Roda;

static Texture2D imagemDaMarca = { 0 };

static Marca marcas[MAXIMO_DE_MARCAS];
static int proxima = 0, total = 0;
static Roda rodas[CARROS][RODAS];

/*
 * deixarMarca
 * A roda está escorregando neste quadro: liga o rastro dela até aqui.
 * Recebe: o número do carro, a roda (0 a 3), o ponto no chão, a frente do carro e a força (0 a 1).
 * Devolve: nada.
 */
void deixarMarca(int carro, int roda, Vector3 ponto, Vector3 frente, float forca) {
    if (carro < 0 || carro >= CARROS || roda < 0 || roda >= RODAS) {
        return;
    }
    Roda *r = &rodas[carro][roda];
    ponto.y += ACIMA_DO_CHAO;
    Vector3 lado = Vector3Normalize((Vector3){ -frente.z, 0.0f, frente.x });
    lado = Vector3Scale(lado, LARGURA_DA_MARCA / 2.0f);
    Vector3 esquerda = Vector3Subtract(ponto, lado), direita = Vector3Add(ponto, lado);
    float andou = Vector3Distance(ponto, r->ultimo);
    if (!r->marcando || andou > SALTO_MAXIMO) {
        r->marcando = 1;
        r->v = GetRandomValue(0, 1000) / 1000.0f;   /* cada rastro começa num ponto da imagem */
        r->ultimo = ponto;
        r->ultimaEsquerda = esquerda;
        r->ultimaDireita = direita;
        return;
    }
    if (andou < PASSO_DA_MARCA) {
        return;
    }
    Marca *m = &marcas[proxima];
    m->cantos[0] = r->ultimaEsquerda;
    m->cantos[1] = r->ultimaDireita;
    m->cantos[2] = direita;
    m->cantos[3] = esquerda;
    m->centro = Vector3Scale(Vector3Add(Vector3Add(m->cantos[0], m->cantos[1]),
                                        Vector3Add(m->cantos[2], m->cantos[3])), 0.25f);
    m->v0 = r->v;
    r->v += andou / COMPRIMENTO_DA_IMAGEM;
    m->v1 = r->v;
    m->forca = fminf(fmaxf(forca, 0.0f), 1.0f);
    m->idade = 0.0f;
    proxima = (proxima + 1) % MAXIMO_DE_MARCAS;
    if (total < MAXIMO_DE_MARCAS) {
        total++;
    }
    r->ultimo = ponto;
    r->ultimaEsquerda = esquerda;
    r->ultimaDireita = direita;
}

/*
 * soltarRoda
 * A roda parou de escorregar: o próximo rastro dela começa de novo, sem emendar.
 * Recebe: o número do carro e a roda.
 * Devolve: nada.
 */
void soltarRoda(int carro, int roda) {
    if (carro >= 0 && carro < CARROS && roda >= 0 && roda < RODAS) {
        rodas[carro][roda].marcando = 0;
    }
}

/*
 * atualizarMarcas
 * Recebe: o tempo que passou na corrida.
 * Devolve: nada.
 */
void atualizarMarcas(float tempoQuadro) {
    for (int i = 0; i < total; i++) {
        marcas[i].idade += tempoQuadro;
    }
}

/*
 * desenharMarcas
 * Desenha as marcas no chão. Chamar entre BeginMode3D e EndMode3D, depois do cenário.
 * Recebe: nada.
 * Devolve: nada.
 */
void desenharMarcas(void) {
    if (total == 0) {
        return;
    }
    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    rlSetTexture(imagemDaMarca.id);
    rlBegin(RL_QUADS);
    for (int i = 0; i < total; i++) {
        Marca *m = &marcas[i];
        if (m->idade >= VIDA_DA_MARCA) {
            continue;
        }
        /* Fora da tela não produz nenhum pixel. Com 45 segundos de vida, as marcas mais
         * velhas estão a quilômetros de distância, atrás da câmera. A folga cobre a
         * meia-diagonal do quadrilátero, então nenhuma marca que toque a tela é cortada. */
        if (!pontoAVista(m->centro, FOLGA_DO_CORTE)) {
            continue;
        }
        float desbota = fminf((VIDA_DA_MARCA - m->idade) / SUMINDO, 1.0f);
        rlColor4ub(255, 255, 255, (unsigned char) (255.0f * ESCURIDAO * m->forca * desbota));
        static const float U[4] = { 0.0f, 1.0f, 1.0f, 0.0f };
        for (int k = 0; k < 4; k++) {
            rlTexCoord2f(U[k], (k < 2) ? m->v0 : m->v1);
            rlVertex3f(m->cantos[k].x, m->cantos[k].y, m->cantos[k].z);
        }
    }
    rlEnd();
    rlSetTexture(0);
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}

/*
 * carregarMarcas
 * A imagem do rastro: a banda de rodagem de um pneu de corrida, borrada pela borracha.
 * Na largura, as bordas são suaves e há três sulcos; no comprimento, os blocos da banda
 * em V, e por cima a borracha manchada, com grão e riscos no sentido em que o pneu arrastou.
 * Recebe: nada.
 * Devolve: nada.
 */
void carregarMarcas(void) {
    Image imagem = GenImageColor(LARGURA_DA_IMAGEM, ALTURA_DA_IMAGEM, BLANK);
    Color *pixels = (Color *) imagem.data;
    float riscos[LARGURA_DA_IMAGEM];
    for (int x = 0; x < LARGURA_DA_IMAGEM; x++) {
        riscos[x] = 0.75f + 0.25f * GetRandomValue(0, 1000) / 1000.0f;   /* cada fio da largura arrasta diferente */
    }
    for (int y = 0; y < ALTURA_DA_IMAGEM; y++) {
        float v = (float) y / ALTURA_DA_IMAGEM;
        for (int x = 0; x < LARGURA_DA_IMAGEM; x++) {
            float u = (x + 0.5f) / LARGURA_DA_IMAGEM;
            float borda = fminf(u, 1.0f - u);
            float forca = fminf(fmaxf((borda - 0.03f) / 0.12f, 0.0f), 1.0f);   /* bordas suaves */
            for (int s = 1; s <= 3; s++) {                                      /* os sulcos */
                float d = fabsf(u - s * 0.25f);
                forca *= 1.0f - 0.55f * fmaxf(1.0f - d / 0.035f, 0.0f);
            }
            float bloco = fmodf(v * 10.0f + fabsf(u - 0.5f) * 1.6f, 1.0f);       /* os blocos em V */
            forca *= (bloco < 0.18f) ? 0.55f : 1.0f;
            float grao = 0.70f + 0.30f * GetRandomValue(0, 1000) / 1000.0f;
            float mancha = 0.80f + 0.20f * sinf(v * 6.2831853f * 3.0f + u * 5.0f) * sinf(v * 6.2831853f * 2.0f);
            forca *= grao * mancha * riscos[x];
            pixels[y * LARGURA_DA_IMAGEM + x] = (Color){ 16, 15, 14, (unsigned char) (255.0f * fminf(fmaxf(forca, 0.0f), 1.0f)) };
        }
    }
    imagemDaMarca = LoadTextureFromImage(imagem);
    UnloadImage(imagem);
    GenTextureMipmaps(&imagemDaMarca);
    SetTextureFilter(imagemDaMarca, TEXTURE_FILTER_TRILINEAR);
    SetTextureWrap(imagemDaMarca, TEXTURE_WRAP_REPEAT);
}

/*
 * descarregarMarcas
 * Recebe: nada.
 * Devolve: nada.
 */
void descarregarMarcas(void) {
    UnloadTexture(imagemDaMarca);
}

/*
 * limparMarcas
 * Uma corrida nova começa sem marcas.
 * Recebe: nada.
 * Devolve: nada.
 */
void limparMarcas(void) {
    proxima = 0;
    total = 0;
    for (int c = 0; c < CARROS; c++) {
        for (int r = 0; r < RODAS; r++) {
            rodas[c][r].marcando = 0;
        }
    }
}
