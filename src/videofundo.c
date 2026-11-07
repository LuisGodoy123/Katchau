/*
 * videofundo.c
 * ------------------------------------------------------------------
 * O vídeo de fundo em loop. Veja videofundo.h.
 *
 * Quem lê e abre os JPEGs é o mjpeg.c, numa thread à parte; aqui só pegamos o
 * quadro que já está pronto e o mandamos para a placa de vídeo.
 * ------------------------------------------------------------------
 */

#include <stdlib.h>
#include <math.h>
#include "raylib.h"
#include "videofundo.h"
#include "mjpeg.h"

#define QUADROS_POR_SEGUNDO   30.0

struct VideoDeFundo {
    Mjpeg *quadros;
    Texture2D textura;        /* o quadro atual, já na placa de vídeo */
    double relogio;           /* segundos desde que o vídeo abriu; só cresce (o mjpeg.c faz a volta do loop) */
};

/*
 * duracaoDoLoop
 * Recebe: o vídeo.
 * Devolve: quantos segundos uma volta inteira do vídeo leva.
 */
static double duracaoDoLoop(const VideoDeFundo *video) {
    int total = totalDeQuadrosMjpeg(video->quadros);
    return (total > 0) ? total / QUADROS_POR_SEGUNDO : 0.0;
}

/*
 * mostrarQuadro
 * Põe na textura o quadro pronto mais novo que não passe do pedido. Na primeira vez cria a
 * textura com o tamanho do quadro. Se nenhum quadro novo está pronto, a textura fica como está
 * (a tela repete o quadro anterior, em vez de o jogo esperar pela decodificação).
 * Recebe: o vídeo e o número do quadro que o relógio está pedindo.
 * Devolve: nada.
 */
static void mostrarQuadro(VideoDeFundo *video, int desejado) {
    int largura, altura;
    unsigned char *pixels = pegarQuadroMjpeg(video->quadros, desejado, &largura, &altura);
    if (pixels == NULL) {
        return;
    }
    if (video->textura.id == 0) {
        Image imagem = { pixels, largura, altura, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8 };
        video->textura = LoadTextureFromImage(imagem);
        SetTextureFilter(video->textura, TEXTURE_FILTER_BILINEAR);
    } else if (largura == video->textura.width && altura == video->textura.height) {
        UpdateTexture(video->textura, pixels);
    }
    devolverQuadroMjpeg(video->quadros, pixels);
}

VideoDeFundo *abrirVideoDeFundo(const char *arquivo) {
    VideoDeFundo *video = calloc(1, sizeof(VideoDeFundo));
    if (video == NULL) {
        return NULL;
    }
    video->quadros = abrirMjpeg(arquivo, 1);   /* 1: em loop */
    if (video->quadros == NULL) {
        free(video);
        return NULL;
    }
    esperarQuadroMjpeg(video->quadros);   /* o primeiro quadro é quem dá o tamanho da textura */
    mostrarQuadro(video, 0);
    if (video->textura.id == 0) {
        fecharVideoDeFundo(video);
        return NULL;
    }
    return video;
}

void reiniciarVideoDeFundo(VideoDeFundo *video) {
    if (video == NULL) {
        return;
    }
    /* O relógio nunca anda para trás (o mjpeg.c conta com isso), então para voltar ao primeiro
     * quadro ele pula para a frente, até o começo da volta seguinte do loop. */
    double duracao = duracaoDoLoop(video);
    video->relogio = (duracao > 0.0) ? (floor(video->relogio / duracao) + 1.0) * duracao : 0.0;
}

void atualizarVideoDeFundo(VideoDeFundo *video, float tempoQuadro) {
    if (video == NULL || totalDeQuadrosMjpeg(video->quadros) == 0) {
        return;
    }
    video->relogio += tempoQuadro;
    mostrarQuadro(video, (int) (video->relogio * QUADROS_POR_SEGUNDO));
}

void desenharVideoDeFundo(const VideoDeFundo *video, float alfa) {
    if (video == NULL || video->textura.id == 0 || alfa <= 0.0f) {
        return;
    }
    float telaL = (float) GetScreenWidth(), telaA = (float) GetScreenHeight();
    float escala = fmaxf(telaL / video->textura.width, telaA / video->textura.height);   /* cobre a tela inteira */
    float larguraVisivel = telaL / escala, alturaVisivel = telaA / escala;
    Rectangle origem = {
        (video->textura.width - larguraVisivel) / 2.0f,
        (video->textura.height - alturaVisivel) / 2.0f,
        larguraVisivel,
        alturaVisivel,
    };
    DrawTexturePro(video->textura, origem, (Rectangle){ 0, 0, telaL, telaA }, (Vector2){ 0, 0 }, 0.0f,
                   Fade(WHITE, alfa));
}

void fecharVideoDeFundo(VideoDeFundo *video) {
    if (video == NULL) {
        return;
    }
    if (video->textura.id != 0) {
        UnloadTexture(video->textura);
    }
    fecharMjpeg(video->quadros);
    free(video);
}
