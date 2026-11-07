/*
 * videofundo.c
 * ------------------------------------------------------------------
 * O vídeo de fundo em loop. Veja videofundo.h.
 * ------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "raylib.h"
#include "videofundo.h"

/* O stb_image (domínio público, o mesmo que a raylib usa) só para abrir os JPEGs. */
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#if defined(__GNUC__)   /* vale para o arquivo todo: o GCC avisa das funções que não usamos só no fim dele */
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif
#include "externo/stb_image.h"

#define QUADROS_POR_SEGUNDO   30.0f
#define PEDACO_DA_LEITURA     (1 << 20)

struct VideoDeFundo {
    FILE *arquivo;
    long *inicioDoQuadro;     /* onde cada JPEG começa no arquivo */
    int totalDeQuadros;
    long tamanhoDoArquivo;
    unsigned char *buffer;    /* os bytes do JPEG que está sendo lido */
    long capacidadeDoBuffer;
    Texture2D textura;        /* o quadro atual, já na placa de vídeo */
    int quadroMostrado;
    float relogio;            /* segundos desde o começo do loop */
};

/*
 * indexarQuadros
 * Percorre o arquivo uma vez e anota onde começa cada JPEG (a marca FF D8 FF). Dentro de um
 * JPEG o byte FF nunca vem seguido de D8 e FF, então a marca só aparece no começo de cada quadro.
 * Recebe: o vídeo, com o arquivo aberto.
 * Devolve: 1 se achou algum quadro.
 */
static int indexarQuadros(VideoDeFundo *video) {
    unsigned char *pedaco = malloc(PEDACO_DA_LEITURA + 2);
    if (pedaco == NULL) {
        return 0;
    }
    int capacidade = 1024;
    video->inicioDoQuadro = malloc(sizeof(long) * capacidade);
    if (video->inicioDoQuadro == NULL) {
        free(pedaco);
        return 0;
    }

    long base = 0;
    size_t sobra = 0;   /* bytes do fim do pedaço anterior: a marca pode ter ficado dividida */
    for (;;) {
        size_t lidos = fread(pedaco + sobra, 1, PEDACO_DA_LEITURA, video->arquivo);
        size_t total = sobra + lidos;
        if (total < 3) {
            break;
        }
        for (size_t i = 0; i + 2 < total; i++) {
            if (pedaco[i] == 0xFF && pedaco[i + 1] == 0xD8 && pedaco[i + 2] == 0xFF) {
                if (video->totalDeQuadros == capacidade) {
                    capacidade *= 2;
                    long *maior = realloc(video->inicioDoQuadro, sizeof(long) * capacidade);
                    if (maior == NULL) {
                        free(pedaco);
                        return video->totalDeQuadros > 0;
                    }
                    video->inicioDoQuadro = maior;
                }
                video->inicioDoQuadro[video->totalDeQuadros++] = base - (long) sobra + (long) i;
            }
        }
        if (lidos == 0) {
            break;
        }
        memmove(pedaco, pedaco + total - 2, 2);   /* os dois últimos bytes voltam para o começo */
        base += (long) lidos;
        sobra = 2;
    }
    free(pedaco);
    video->tamanhoDoArquivo = base;
    return video->totalDeQuadros > 0;
}

/*
 * lerQuadro
 * Lê um JPEG do arquivo e o decodifica para RGB.
 * Recebe: o vídeo, o número do quadro e onde guardar a largura e a altura.
 * Devolve: os pixels em RGB (liberar com stbi_image_free), ou NULL.
 */
static unsigned char *lerQuadro(VideoDeFundo *video, int quadro, int *largura, int *altura) {
    long inicio = video->inicioDoQuadro[quadro];
    long fim = (quadro + 1 < video->totalDeQuadros) ? video->inicioDoQuadro[quadro + 1] : video->tamanhoDoArquivo;
    long tamanho = fim - inicio;
    if (tamanho <= 0) {
        return NULL;
    }
    if (tamanho > video->capacidadeDoBuffer) {
        unsigned char *maior = realloc(video->buffer, tamanho);
        if (maior == NULL) {
            return NULL;
        }
        video->buffer = maior;
        video->capacidadeDoBuffer = tamanho;
    }
    if (fseek(video->arquivo, inicio, SEEK_SET) != 0 ||
        fread(video->buffer, 1, tamanho, video->arquivo) != (size_t) tamanho) {
        return NULL;
    }
    int canais;
    return stbi_load_from_memory(video->buffer, (int) tamanho, largura, altura, &canais, 3);
}

/*
 * mostrarQuadro
 * Põe o quadro pedido na textura. Na primeira vez cria a textura com o tamanho do quadro.
 * Recebe: o vídeo e o número do quadro.
 * Devolve: nada.
 */
static void mostrarQuadro(VideoDeFundo *video, int quadro) {
    int largura, altura;
    unsigned char *pixels = lerQuadro(video, quadro, &largura, &altura);
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
    stbi_image_free(pixels);
    video->quadroMostrado = quadro;
}

VideoDeFundo *abrirVideoDeFundo(const char *arquivo) {
    VideoDeFundo *video = calloc(1, sizeof(VideoDeFundo));
    if (video == NULL) {
        return NULL;
    }
    video->arquivo = fopen(arquivo, "rb");
    if (video->arquivo == NULL || !indexarQuadros(video)) {
        fecharVideoDeFundo(video);
        return NULL;
    }
    video->quadroMostrado = -1;
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
    video->relogio = 0.0f;
}

void atualizarVideoDeFundo(VideoDeFundo *video, float tempoQuadro) {
    if (video == NULL || video->totalDeQuadros == 0) {
        return;
    }
    video->relogio += tempoQuadro;
    float duracao = video->totalDeQuadros / QUADROS_POR_SEGUNDO;
    while (video->relogio >= duracao) {
        video->relogio -= duracao;   /* loop */
    }
    int quadro = (int) (video->relogio * QUADROS_POR_SEGUNDO);
    if (quadro >= video->totalDeQuadros) {
        quadro = video->totalDeQuadros - 1;
    }
    if (quadro != video->quadroMostrado) {
        mostrarQuadro(video, quadro);
    }
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
    if (video->arquivo != NULL) {
        fclose(video->arquivo);
    }
    free(video->inicioDoQuadro);
    free(video->buffer);
    free(video);
}
