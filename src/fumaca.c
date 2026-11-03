/*
 * fumaca.c
 * ------------------------------------------------------------------
 * As nuvens de fumaça e de poeira. Veja fumaca.h.
 *
 * As nuvens ficam num vetor de tamanho fixo. Quando ele enche, a nuvem nova
 * ocupa o lugar da mais antiga. A imagem de cada nuvem é gerada ao abrir
 * o jogo: um borrão redondo, de bordas suaves, recortado com ruído para
 * não parecer uma bola perfeita.
 * ------------------------------------------------------------------
 */

#include <math.h>
#include "fumaca.h"
#include "rlgl.h"

#define MAXIMO_DE_NUVENS     900
#define LADO_DA_IMAGEM       96
#define VARIACOES_DE_IMAGEM  3

#define VIDA_DO_PNEU         1.5f      /* segundos */
#define VIDA_DA_POEIRA       1.1f
#define TAMANHO_INICIAL      0.45f     /* metros */
#define TAMANHO_FINAL_PNEU   4.2f
#define TAMANHO_FINAL_POEIRA 3.2f
#define OPACIDADE_PNEU       0.46f
#define OPACIDADE_POEIRA     0.40f
#define SUBIDA               0.9f      /* metros por segundo */
#define ARRASTO_DO_CARRO     0.20f     /* a nuvem nasce com esta fração da velocidade do carro... */
#define FREIO_DO_AR          2.2f      /* ...e o ar a segura (1/s) */
#define ESPALHAR             0.9f      /* metros por segundo, para qualquer lado */
#define COR_DO_PNEU          (Color){ 232, 232, 236, 255 }
#define COMPRIMENTO_DA_CHAMA  1.5f      /* metros */
#define LARGURA_DA_CHAMA      0.70f
#define PEDACOS_DA_CHAMA      9
#define COR_DA_CHAMA_PERTO    (Color){ 130, 205, 255, 255 }
#define COR_DA_CHAMA_LONGE    (Color){  20,  70, 255, 255 }
#define COR_DA_POEIRA        (Color){ 150, 122,  78, 255 }

typedef struct {
    Vector3 posicao;
    Vector3 velocidade;
    float idade;         /* segundos desde que nasceu */
    float vida;          /* segundos até sumir (0 = lugar livre) */
    float forca;         /* de 0 a 1: quanto ela é densa */
    float giro;          /* graus, para as nuvens não ficarem iguais */
    float giroPorSegundo;
    int imagem;
    TipoDeNuvem tipo;
} Nuvem;

static Nuvem nuvens[MAXIMO_DE_NUVENS];
static int proximaNuvem = 0;
static Texture2D imagens[VARIACOES_DE_IMAGEM];

/*
 * sortear
 * Recebe: os limites.
 * Devolve: um número entre eles.
 */
static float sortear(float minimo, float maximo) {
    return minimo + (maximo - minimo) * GetRandomValue(0, 1000) / 1000.0f;
}

/*
 * gerarImagem
 * Um borrão branco: a opacidade cai do centro para a borda, e o ruído
 * desfaz o contorno.
 * Recebe: a semente do ruído (muda o desenho).
 * Devolve: a textura.
 */
static Texture2D gerarImagem(int semente) {
    Image ruido = GenImagePerlinNoise(LADO_DA_IMAGEM, LADO_DA_IMAGEM, semente * 97, semente * 53, 3.0f);
    Color *cores = LoadImageColors(ruido);
    Image imagem = GenImageColor(LADO_DA_IMAGEM, LADO_DA_IMAGEM, BLANK);
    float meio = (LADO_DA_IMAGEM - 1) / 2.0f;
    for (int y = 0; y < LADO_DA_IMAGEM; y++) {
        for (int x = 0; x < LADO_DA_IMAGEM; x++) {
            float dx = (x - meio) / meio, dy = (y - meio) / meio;
            float distancia = sqrtf(dx * dx + dy * dy);
            float redondo = fmaxf(1.0f - distancia, 0.0f);
            redondo = redondo * redondo * (3.0f - 2.0f * redondo);   /* suaviza a queda */
            float textura = 0.45f + 0.75f * cores[y * LADO_DA_IMAGEM + x].r / 255.0f;
            float alfa = fminf(redondo * textura * 1.25f, 1.0f);
            ImageDrawPixel(&imagem, x, y, (Color){ 255, 255, 255, (unsigned char) (alfa * 255.0f) });
        }
    }
    Texture2D textura = LoadTextureFromImage(imagem);
    GenTextureMipmaps(&textura);
    SetTextureFilter(textura, TEXTURE_FILTER_TRILINEAR);
    UnloadImageColors(cores);
    UnloadImage(ruido);
    UnloadImage(imagem);
    return textura;
}

/*
 * carregarFumaca
 * Gera as imagens das nuvens. Chamar depois de criar a janela.
 * Recebe: nada.
 * Devolve: nada.
 */
void carregarFumaca(void) {
    for (int i = 0; i < VARIACOES_DE_IMAGEM; i++) {
        imagens[i] = gerarImagem(i + 1);
    }
    limparFumaca();
}

/*
 * descarregarFumaca
 * Recebe: nada.
 * Devolve: nada.
 */
void descarregarFumaca(void) {
    for (int i = 0; i < VARIACOES_DE_IMAGEM; i++) {
        UnloadTexture(imagens[i]);
    }
}

/*
 * limparFumaca
 * Tira todas as nuvens (corrida nova).
 * Recebe: nada.
 * Devolve: nada.
 */
void limparFumaca(void) {
    for (int i = 0; i < MAXIMO_DE_NUVENS; i++) {
        nuvens[i].vida = 0.0f;
    }
}

/*
 * soltarNuvem
 * Solta uma nuvem nova na roda de um carro.
 * Recebe: onde está a roda, a velocidade do carro, a densidade (0 a 1) e o tipo.
 * Devolve: nada.
 */
void soltarNuvem(Vector3 roda, Vector3 velocidadeDoCarro, float forca, TipoDeNuvem tipo) {
    Nuvem *nuvem = &nuvens[proximaNuvem];
    proximaNuvem = (proximaNuvem + 1) % MAXIMO_DE_NUVENS;

    nuvem->posicao = roda;
    nuvem->velocidade = (Vector3){ velocidadeDoCarro.x * ARRASTO_DO_CARRO + sortear(-ESPALHAR, ESPALHAR),
                                   SUBIDA * sortear(0.6f, 1.4f),
                                   velocidadeDoCarro.z * ARRASTO_DO_CARRO + sortear(-ESPALHAR, ESPALHAR) };
    nuvem->idade = 0.0f;
    nuvem->vida = ((tipo == NUVEM_DE_PNEU) ? VIDA_DO_PNEU : VIDA_DA_POEIRA) * sortear(0.8f, 1.2f);
    nuvem->forca = forca;
    nuvem->giro = sortear(0.0f, 360.0f);
    nuvem->giroPorSegundo = sortear(-40.0f, 40.0f);
    nuvem->imagem = GetRandomValue(0, VARIACOES_DE_IMAGEM - 1);
    nuvem->tipo = tipo;
}

/*
 * atualizarFumaca
 * Envelhece e move as nuvens. Com tempo 0 (pausa), elas ficam paradas no ar.
 * Recebe: o tempo que passou na corrida.
 * Devolve: nada.
 */
void atualizarFumaca(float tempoQuadro) {
    float freio = expf(-FREIO_DO_AR * tempoQuadro);
    for (int i = 0; i < MAXIMO_DE_NUVENS; i++) {
        Nuvem *nuvem = &nuvens[i];
        if (nuvem->vida <= 0.0f) {
            continue;
        }
        nuvem->idade += tempoQuadro;
        if (nuvem->idade >= nuvem->vida) {
            nuvem->vida = 0.0f;
            continue;
        }
        nuvem->posicao.x += nuvem->velocidade.x * tempoQuadro;
        nuvem->posicao.y += nuvem->velocidade.y * tempoQuadro;
        nuvem->posicao.z += nuvem->velocidade.z * tempoQuadro;
        nuvem->velocidade.x *= freio;
        nuvem->velocidade.z *= freio;
        nuvem->giro += nuvem->giroPorSegundo * tempoQuadro;
    }
}

/*
 * desenharFumaca3D
 * Desenha as nuvens, misturadas com o que está atrás. Elas respeitam o que
 * está na frente (um muro esconde a fumaça), mas não escrevem na
 * profundidade, para uma não recortar a outra. Chamar dentro de BeginMode3D,
 * depois de tudo o que é sólido.
 * Recebe: a câmera.
 * Devolve: nada.
 */
void desenharFumaca3D(Camera3D camera) {
    rlDrawRenderBatchActive();
    BeginBlendMode(BLEND_ALPHA);
    rlDisableDepthMask();
    for (int i = 0; i < MAXIMO_DE_NUVENS; i++) {
        Nuvem *nuvem = &nuvens[i];
        if (nuvem->vida <= 0.0f) {
            continue;
        }
        float fase = nuvem->idade / nuvem->vida;
        int ehPneu = (nuvem->tipo == NUVEM_DE_PNEU);
        float tamanhoFinal = ehPneu ? TAMANHO_FINAL_PNEU : TAMANHO_FINAL_POEIRA;
        float tamanho = TAMANHO_INICIAL + (tamanhoFinal - TAMANHO_INICIAL) * sqrtf(fase);   /* cresce rápido e depois devagar */
        float entrada = fminf(fase / 0.08f, 1.0f);                                          /* não aparece de repente */
        float opacidade = (ehPneu ? OPACIDADE_PNEU : OPACIDADE_POEIRA) * nuvem->forca * entrada * (1.0f - fase) * (1.0f - fase);

        Texture2D imagem = imagens[nuvem->imagem];
        Rectangle origem = { 0.0f, 0.0f, (float) imagem.width, (float) imagem.height };
        Vector2 medida = { tamanho, tamanho };
        Vector2 meio = { tamanho / 2.0f, tamanho / 2.0f };
        Vector3 cima = { 0.0f, 1.0f, 0.0f };
        DrawBillboardPro(camera, imagem, origem, nuvem->posicao, cima, medida, meio, nuvem->giro,
                         Fade(ehPneu ? COR_DO_PNEU : COR_DA_POEIRA, opacidade));
    }
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
    EndBlendMode();
}

/*
 * desenharChamaDoNitro
 * A chama azul: vários borrões somando luz, do escapamento para trás, cada vez
 * menores e mais azuis. O comprimento treme a cada quadro, como fogo.
 * Chamar dentro de BeginMode3D, depois de tudo o que é sólido.
 * Recebe: a câmera, a saída do escapamento e a direção para trás do carro (unitária).
 * Devolve: nada.
 */
void desenharChamaDoNitro(Camera3D camera, Vector3 escape, Vector3 paraTras) {
    rlDrawRenderBatchActive();
    BeginBlendMode(BLEND_ADDITIVE);
    rlDisableDepthMask();
    float comprimento = COMPRIMENTO_DA_CHAMA * sortear(0.75f, 1.2f);
    for (int i = 0; i < PEDACOS_DA_CHAMA; i++) {
        float fase = (float) i / (PEDACOS_DA_CHAMA - 1);
        float distancia = comprimento * fase;
        Vector3 centro = { escape.x + paraTras.x * distancia, escape.y + paraTras.y * distancia + sortear(-0.03f, 0.03f),
                           escape.z + paraTras.z * distancia };
        float tamanho = LARGURA_DA_CHAMA * (1.0f - 0.72f * fase) * sortear(0.85f, 1.15f);
        Color cor = ColorLerp(COR_DA_CHAMA_PERTO, COR_DA_CHAMA_LONGE, fase);
        Texture2D imagem = imagens[GetRandomValue(0, VARIACOES_DE_IMAGEM - 1)];
        Rectangle origem = { 0.0f, 0.0f, (float) imagem.width, (float) imagem.height };
        DrawBillboardRec(camera, imagem, origem, centro, (Vector2){ tamanho, tamanho }, Fade(cor, 1.0f - 0.55f * fase));
    }
    /* O miolo branco, bem na boca do escapamento */
    Texture2D imagem = imagens[0];
    Rectangle origem = { 0.0f, 0.0f, (float) imagem.width, (float) imagem.height };
    DrawBillboardRec(camera, imagem, origem, escape, (Vector2){ LARGURA_DA_CHAMA * 0.55f, LARGURA_DA_CHAMA * 0.55f }, WHITE);
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
    EndBlendMode();
}
