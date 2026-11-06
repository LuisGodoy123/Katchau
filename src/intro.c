/*
 * intro.c
 * ------------------------------------------------------------------
 * O vídeo de abertura. Veja intro.h.
 * ------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "raylib.h"
#include "raymath.h"
#include "intro.h"
#include "hud.h"
#include "visual.h"

/* O stb_image (domínio público, o mesmo que vem com a raylib) só para abrir JPEG. Ele fica
 * "static" aqui dentro, para não brigar com a cópia que a própria raylib tem. */
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

#define ARQUIVO_DOS_QUADROS   "recursos/video/intro.mjpeg"
#define ARQUIVO_DO_SOM        "recursos/video/intro.mp3"
#define QUADROS_POR_SEGUNDO   30.0f
#define VOLUME_DO_VIDEO       0.9f
#define ESCURECER_NO_FIM      0.45f   /* segundos escurecendo quando o vídeo acaba ou é pulado */
#define AVISO_DEPOIS_DE       0.8f    /* segundos até o aviso de pular aparecer */
#define PEDACO_DA_LEITURA     (1 << 20)

static FILE *arquivo = NULL;
static long *inicioDoQuadro = NULL;   /* onde cada JPEG começa no arquivo */
static int totalDeQuadros = 0;
static long tamanhoDoArquivo = 0;
static unsigned char *bufferDoQuadro = NULL;
static long capacidadeDoBuffer = 0;
static Texture2D textura = { 0 };
static int quadroMostrado = -1;
static Music som = { 0 };
static int temSom = 0;
static float relogio = 0.0f;          /* segundos de vídeo (sem som, conta pelo tempo do quadro) */
static float saindo = 0.0f;           /* > 0: escurecendo para acabar */
static int ativo = 0;

/* ==================================================================
 * Ler o arquivo de quadros
 * ================================================================== */

/*
 * indexarQuadros
 * Percorre o arquivo uma vez e anota onde começa cada JPEG (a marca FF D8 FF). Dentro dos dados
 * de um JPEG o byte FF nunca aparece seguido de D8, então a marca só aparece no começo de cada um.
 * Recebe: nada (usa o arquivo aberto).
 * Devolve: 1 se achou algum quadro.
 */
static int indexarQuadros(void) {
    unsigned char *pedaco = malloc(PEDACO_DA_LEITURA + 2);
    if (pedaco == NULL) {
        return 0;
    }
    int capacidade = 4096;
    inicioDoQuadro = malloc(sizeof(long) * capacidade);
    if (inicioDoQuadro == NULL) {
        free(pedaco);
        return 0;
    }

    long base = 0;
    size_t sobra = 0;   /* bytes do fim do pedaço anterior, para achar a marca que ficou dividida */
    for (;;) {
        size_t lidos = fread(pedaco + sobra, 1, PEDACO_DA_LEITURA, arquivo);
        size_t total = sobra + lidos;
        if (total < 3) {
            break;
        }
        for (size_t i = 0; i + 2 < total; i++) {
            if (pedaco[i] == 0xFF && pedaco[i + 1] == 0xD8 && pedaco[i + 2] == 0xFF) {
                if (totalDeQuadros == capacidade) {
                    capacidade *= 2;
                    long *maior = realloc(inicioDoQuadro, sizeof(long) * capacidade);
                    if (maior == NULL) {
                        free(pedaco);
                        return totalDeQuadros > 0;
                    }
                    inicioDoQuadro = maior;
                }
                inicioDoQuadro[totalDeQuadros++] = base - (long) sobra + (long) i;
            }
        }
        if (lidos == 0) {
            break;
        }
        /* Os dois últimos bytes voltam para o começo: a marca pode estar dividida entre pedaços */
        memmove(pedaco, pedaco + total - 2, 2);
        base += (long) lidos;
        sobra = 2;
    }
    free(pedaco);
    tamanhoDoArquivo = base;
    return totalDeQuadros > 0;
}

/*
 * abrirQuadro
 * Lê um JPEG do arquivo e o abre.
 * Recebe: o número do quadro e onde guardar a largura e a altura.
 * Devolve: os pixels em RGB (liberar com stbi_image_free), ou NULL.
 */
static unsigned char *abrirQuadro(int quadro, int *largura, int *altura) {
    long inicio = inicioDoQuadro[quadro];
    long fim = (quadro + 1 < totalDeQuadros) ? inicioDoQuadro[quadro + 1] : tamanhoDoArquivo;
    long tamanho = fim - inicio;
    if (tamanho <= 0) {
        return NULL;
    }
    if (tamanho > capacidadeDoBuffer) {
        unsigned char *maior = realloc(bufferDoQuadro, tamanho);
        if (maior == NULL) {
            return NULL;
        }
        bufferDoQuadro = maior;
        capacidadeDoBuffer = tamanho;
    }
    if (fseek(arquivo, inicio, SEEK_SET) != 0 || fread(bufferDoQuadro, 1, tamanho, arquivo) != (size_t) tamanho) {
        return NULL;
    }
    int canais;
    return stbi_load_from_memory(bufferDoQuadro, (int) tamanho, largura, altura, &canais, 3);
}

/* ==================================================================
 * Começar, andar e acabar
 * ================================================================== */

/*
 * iniciarIntro
 * Abre o vídeo e começa a tocar. Chamar uma vez, quando o jogo abre (com a janela e o som já
 * prontos).
 * Recebe: nada.
 * Devolve: 1 se o vídeo vai passar, 0 se não foi possível (aí o jogo segue direto para o menu).
 */
int iniciarIntro(void) {
    arquivo = fopen(ARQUIVO_DOS_QUADROS, "rb");
    if (arquivo == NULL) {
        return 0;
    }
    if (!indexarQuadros()) {
        encerrarIntro();
        return 0;
    }

    int largura, altura;
    unsigned char *pixels = abrirQuadro(0, &largura, &altura);
    if (pixels == NULL) {
        encerrarIntro();
        return 0;
    }
    Image imagem = { pixels, largura, altura, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8 };
    textura = LoadTextureFromImage(imagem);
    stbi_image_free(pixels);
    SetTextureFilter(textura, TEXTURE_FILTER_BILINEAR);
    quadroMostrado = 0;

    if (IsAudioDeviceReady() && FileExists(ARQUIVO_DO_SOM)) {
        som = LoadMusicStream(ARQUIVO_DO_SOM);
        temSom = IsMusicValid(som);
        if (temSom) {
            som.looping = false;
            SetMusicVolume(som, VOLUME_DO_VIDEO);
            PlayMusicStream(som);
        }
    }
    relogio = 0.0f;
    saindo = 0.0f;
    ativo = 1;
    return 1;
}

/*
 * apertouQualquerCoisa
 * Recebe: nada.
 * Devolve: 1 se neste quadro foi apertada qualquer tecla, botão do mouse ou botão de controle.
 */
static int apertouQualquerCoisa(void) {
    int apertou = 0;
    while (GetKeyPressed() != 0) {   /* esvazia a fila: a tecla que pulou não chega ao menu */
        apertou = 1;
    }
    for (int botao = MOUSE_BUTTON_LEFT; botao <= MOUSE_BUTTON_BACK; botao++) {
        apertou = apertou || IsMouseButtonPressed(botao);
    }
    for (int controle = 0; controle < 4; controle++) {
        if (!IsGamepadAvailable(controle)) {
            continue;
        }
        for (int botao = GAMEPAD_BUTTON_LEFT_FACE_UP; botao <= GAMEPAD_BUTTON_RIGHT_THUMB; botao++) {
            apertou = apertou || IsGamepadButtonPressed(controle, botao);
        }
    }
    return apertou;
}

/*
 * atualizarIntro
 * Um quadro do vídeo: anda o relógio (pelo som), troca a imagem quando chega a hora do próximo
 * quadro e vê se o jogador quer pular.
 * Recebe: o tempo do quadro.
 * Devolve: 1 enquanto o vídeo continua, 0 quando acabou (aí chame encerrarIntro).
 */
int atualizarIntro(float tempoQuadro) {
    if (!ativo) {
        return 0;
    }
    if (temSom) {
        UpdateMusicStream(som);
    }

    if (saindo > 0.0f) {
        saindo -= tempoQuadro;
        if (temSom) {
            SetMusicVolume(som, VOLUME_DO_VIDEO * fmaxf(saindo / ESCURECER_NO_FIM, 0.0f));
        }
        return saindo > 0.0f;
    }

    relogio = temSom ? GetMusicTimePlayed(som) : relogio + tempoQuadro;
    int quadro = (int) (relogio * QUADROS_POR_SEGUNDO);
    int somAcabou = temSom && !IsMusicStreamPlaying(som) && relogio > 1.0f;
    if (quadro >= totalDeQuadros || somAcabou || apertouQualquerCoisa()) {
        saindo = ESCURECER_NO_FIM;
        return 1;
    }

    if (quadro != quadroMostrado) {
        int largura, altura;
        unsigned char *pixels = abrirQuadro(quadro, &largura, &altura);
        if (pixels != NULL) {
            if (largura == textura.width && altura == textura.height) {
                UpdateTexture(textura, pixels);
            }
            stbi_image_free(pixels);
            quadroMostrado = quadro;
        }
    }
    return 1;
}

/*
 * desenharIntro
 * O quadro do vídeo no meio da tela, do maior tamanho que cabe sem distorcer (com faixas pretas
 * onde sobrar), e o aviso de pular no canto de baixo, à direita.
 * Recebe: nada.
 * Devolve: nada.
 */
void desenharIntro(void) {
    if (!ativo) {
        return;
    }
    float telaL = (float) GetScreenWidth(), telaA = (float) GetScreenHeight();
    float escala = fminf(telaL / textura.width, telaA / textura.height);
    float largura = textura.width * escala, altura = textura.height * escala;
    DrawTexturePro(textura, (Rectangle){ 0, 0, (float) textura.width, (float) textura.height },
                   (Rectangle){ (telaL - largura) / 2.0f, (telaA - altura) / 2.0f, largura, altura },
                   (Vector2){ 0, 0 }, 0.0f, WHITE);

    comecarInterface(0);
    float alfa = Clamp((relogio - AVISO_DEPOIS_DE) / 0.5f, 0.0f, 1.0f) * (0.75f + 0.25f * sinf((float) GetTime() * 3.0f));
    if (saindo > 0.0f) {
        alfa = 0.0f;
    }
    if (alfa > 0.0f) {
        const char *aviso = "APERTE QUALQUER BOTÃO PARA PULAR";
        float tamanho = 17.0f;
        float larguraDoAviso = larguraDoTexto(FONTE_FORTE, aviso, tamanho) + 36.0f;
        float x = larguraVirtual() - larguraDoAviso - 24.0f;
        float y = alturaVirtual() - 54.0f;
        desenharParalelogramo(x, y, larguraDoAviso, 30.0f, 9.0f, Fade(COR_PAINEL, 0.85f * alfa));
        desenharParalelogramo(x - 9.0f, y, 5.0f, 30.0f, 9.0f, Fade(COR_AMARELO, alfa));
        escrever(FONTE_FORTE, aviso, x + 18.0f, y + 4.0f, tamanho, Fade(COR_TEXTO_CLARO, alfa));
    }
    if (saindo > 0.0f) {
        DrawRectangle(0, 0, larguraVirtual(), alturaVirtual(), Fade(BLACK, 1.0f - saindo / ESCURECER_NO_FIM));
    }
    terminarInterface();
}

/*
 * encerrarIntro
 * Fecha o vídeo e devolve a memória (a textura, o som e o arquivo).
 * Recebe: nada.
 * Devolve: nada.
 */
void encerrarIntro(void) {
    if (temSom) {
        StopMusicStream(som);
        UnloadMusicStream(som);
        temSom = 0;
    }
    if (textura.id != 0) {
        UnloadTexture(textura);
        textura = (Texture2D){ 0 };
    }
    if (arquivo != NULL) {
        fclose(arquivo);
        arquivo = NULL;
    }
    free(inicioDoQuadro);
    inicioDoQuadro = NULL;
    totalDeQuadros = 0;
    free(bufferDoQuadro);
    bufferDoQuadro = NULL;
    capacidadeDoBuffer = 0;
    ativo = 0;
}
