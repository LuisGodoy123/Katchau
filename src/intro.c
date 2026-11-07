/*
 * intro.c
 * ------------------------------------------------------------------
 * O vídeo de abertura: a largada da Copa Pistão do filme Carros (0:28 a 1:21 do vídeo
 * original). O som é a abertura (recursos/sons/abertura.mp3, em audio.c), que segue no menu.
 * Veja intro.h.
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
#include "audio.h"
#include "mjpeg.h"

#define ARQUIVO_DOS_QUADROS   "recursos/video/intro.mjpeg"
#define QUADROS_POR_SEGUNDO   30.0f
#define DURACAO_DA_DISSOLUCAO 1.1f    /* segundos: o vídeo se dissolve no menu */
#define AVANCO_NA_DISSOLUCAO  0.06f   /* o quadro cresce um pouco enquanto some, como a câmera entrando */
#define AVISO_DEPOIS_DE       0.8f    /* segundos até o aviso de pular aparecer */

static Mjpeg *quadros = NULL;         /* os JPEGs do vídeo, abertos numa thread à parte */
static Texture2D textura = { 0 };
static float relogio = 0.0f;          /* segundos de vídeo (sem som, conta pelo tempo do quadro) */
static int ativo = 0;
static float dissolvendo = 0.0f;     /* > 0: o último quadro sumindo por cima do menu */

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
    quadros = abrirMjpeg(ARQUIVO_DOS_QUADROS, 0);   /* 0: sem loop, acaba no fim */
    if (quadros == NULL) {
        return 0;
    }

    /* O primeiro quadro é quem dá o tamanho da textura, então aqui vale esperar por ele */
    esperarQuadroMjpeg(quadros);
    int largura, altura;
    unsigned char *pixels = pegarQuadroMjpeg(quadros, 0, &largura, &altura);
    if (pixels == NULL) {
        encerrarIntro();
        return 0;
    }
    Image imagem = { pixels, largura, altura, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8 };
    textura = LoadTextureFromImage(imagem);
    devolverQuadroMjpeg(quadros, pixels);
    SetTextureFilter(textura, TEXTURE_FILTER_BILINEAR);

    comecarAbertura();   /* o som da intro é a abertura (audio.c), que continua no menu */
    relogio = 0.0f;
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
    atualizarAudio(1);   /* alimenta a abertura */

    /* O relógio é o som da abertura (assim imagem e som nunca se separam); sem ela, o do quadro */
    float tocado = tempoDaAbertura();
    relogio = (tocado >= 0.0f) ? fmaxf(relogio, tocado) : relogio + tempoQuadro;
    int quadro = (int) (relogio * QUADROS_POR_SEGUNDO);
    int pulou = apertouQualquerCoisa();
    if (pulou) {
        pularAbertura();   /* pulou: a Real Gone começa do zero (sem pular, a abertura segue no menu) */
    }
    if (quadro >= totalDeQuadrosMjpeg(quadros) || pulou) {
        dissolvendo = DURACAO_DA_DISSOLUCAO;   /* o último quadro some por cima do menu (desenharSaidaDaIntro) */
        return 0;
    }

    /* O quadro já vem aberto pela thread do mjpeg.c. Se ainda não está pronto, a tela repete o
     * anterior: o relógio segue o som, então a imagem volta ao compasso no quadro seguinte. */
    int largura, altura;
    unsigned char *pixels = pegarQuadroMjpeg(quadros, quadro, &largura, &altura);
    if (pixels != NULL) {
        if (largura == textura.width && altura == textura.height) {
            UpdateTexture(textura, pixels);
        }
        devolverQuadroMjpeg(quadros, pixels);
    }
    return 1;
}

/*
 * desenharSaidaDaIntro
 * Depois do vídeo, o último quadro vai sumindo por cima do menu (que já está desenhado embaixo)
 * e crescendo um pouco, como a câmera entrando: sem passar pelo preto. Quando acaba, fecha a intro.
 * Chamar no fim do quadro, por cima de tudo.
 * Recebe: o tempo do quadro.
 * Devolve: nada.
 */
void desenharSaidaDaIntro(float tempoQuadro) {
    if (dissolvendo <= 0.0f) {
        return;
    }
    if (textura.id != 0) {
        float t = 1.0f - dissolvendo / DURACAO_DA_DISSOLUCAO;   /* 0 no começo, 1 no fim */
        float suave = t * t * (3.0f - 2.0f * t);
        float telaL = (float) GetScreenWidth(), telaA = (float) GetScreenHeight();
        float escala = fminf(telaL / textura.width, telaA / textura.height) * (1.0f + AVANCO_NA_DISSOLUCAO * suave);
        float largura = textura.width * escala, altura = textura.height * escala;
        DrawTexturePro(textura, (Rectangle){ 0, 0, (float) textura.width, (float) textura.height },
                       (Rectangle){ (telaL - largura) / 2.0f, (telaA - altura) / 2.0f, largura, altura },
                       (Vector2){ 0, 0 }, 0.0f, Fade(WHITE, 1.0f - suave));
    }
    dissolvendo -= tempoQuadro;
    if (dissolvendo <= 0.0f) {
        dissolvendo = 0.0f;
        encerrarIntro();
    }
}

/*
 * momentoDaIntro
 * Recebe: nada.
 * Devolve: quantos segundos de vídeo já passaram.
 */
float momentoDaIntro(void) {
    return relogio;
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
    terminarInterface();
}

/*
 * encerrarIntro
 * Fecha o vídeo e devolve a memória (a textura e o arquivo). A abertura continua tocando no menu.
 * Recebe: nada.
 * Devolve: nada.
 */
void encerrarIntro(void) {
    if (textura.id != 0) {
        UnloadTexture(textura);
        textura = (Texture2D){ 0 };
    }
    fecharMjpeg(quadros);   /* para a thread e devolve os quadros que sobraram na fila */
    quadros = NULL;
    ativo = 0;
}
