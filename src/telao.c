/*
 * telao.c
 * ------------------------------------------------------------------
 * Os telões e o placar da Copa Pistão. Veja telao.h.
 *
 * O modelo do estádio traz três imagens fixas: o logo da Piston Cup no cubo
 * pendurado no meio e no telão do alto da arquibancada, e o placar ("LAP 423"),
 * na torre embaixo do cubo e no telão do alto. Aqui cada um vira uma imagem
 * desenhada pelo jogo (RenderTexture), trocada no material do modelo:
 *   - o quadro da transmissão: render.c desenha a corrida vista de uma câmera
 *     de TV dentro dele, e aqui entram as legendas;
 *   - o placar: a volta do líder e os quinze primeiros, em LED.
 * ------------------------------------------------------------------
 */

#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include "raylib.h"
#include "rlgl.h"
#include "telao.h"
#include "visual.h"
#include "hud.h"
#include "raymath.h"
#include "pista.h"
#define ESCALA_DO_ESTADIO ESCALA_COPA_PISTAO

/* Os materiais do modelo (o índice do glTF mais 1: a raylib põe o material padrão na frente) */
#define MATERIAL_DO_CUBO       12      /* Material.094: o logo da Piston Cup, no cubo do meio */
#define MATERIAL_DO_PLACAR     10      /* Material.095: a torre do placar */
#define LARGURA_DO_QUADRO      512
#define ALTURA_DO_QUADRO       312     /* o cubo é mais largo que alto */
#define LARGURA_DO_PLACAR      128
#define ALTURA_DO_PLACAR       512
#define QUADROS_POR_SEGUNDO    15.0f   /* a transmissão não precisa ser tão fluida quanto o jogo */
#define LINHAS_NO_PLACAR       15
#ifndef VIRAR
#define VIRAR                  -1.0f   /* a RenderTexture guarda de baixo para cima; o modelo lê de cima para baixo */
#endif

static RenderTexture2D quadro, placar, bruto, rascunho;
static Texture2D originais[2];
static Model *modeloDoEstadio = NULL;
static int carregado = 0;
static Shader shaderDaTela;

/* A tela de LED brilha sozinha: nem o sol nem a noite mudam a cor dela */
static const char *PIXELS_DA_TELA =
    "#version 330\n"
    "in vec2 fragTexCoord; uniform sampler2D texture0; uniform vec4 colDiffuse;\n"
    "out vec4 finalColor;\n"
    "void main() {\n"
    "    vec3 c = texture(texture0, fragTexCoord).rgb;\n"
    "    float l = dot(c, vec3(0.2126, 0.7152, 0.0722));\n"
    "    c = mix(vec3(l), c, 1.35);\n"                    /* cores vivas de LED */
    "    c = (c - 0.5) * 1.25 + 0.5;\n"                   /* contraste */
    "    c = c * 1.25 + 0.04;\n"                          /* acesa: nunca preta de verdade */
    "    finalColor = vec4(clamp(c, 0.0, 1.0) * colDiffuse.rgb, 1.0);\n"
    "}\n";

/*
 * shaderDaTelaQueBrilha
 * Recebe: nada.
 * Devolve: o shader das telas dos telões (sem luz: a tela emite).
 */
Shader shaderDaTelaQueBrilha(void) {
    if (shaderDaTela.id == 0) {
        shaderDaTela = LoadShaderFromMemory(NULL, PIXELS_DA_TELA);
    }
    return shaderDaTela;
}
static float desdeOUltimo = 1.0f;

/*
 * carregarTelao
 * Cria as imagens e troca as do modelo por elas.
 * Recebe: o modelo do estádio.
 * Devolve: nada.
 */
void carregarTelao(Model *estadio) {
    if (estadio->materialCount <= MATERIAL_DO_CUBO) {
        return;
    }
    modeloDoEstadio = estadio;
    quadro = LoadRenderTexture(LARGURA_DO_QUADRO, ALTURA_DO_QUADRO);
    placar = LoadRenderTexture(LARGURA_DO_PLACAR, ALTURA_DO_PLACAR);
    bruto = LoadRenderTexture(LARGURA_DO_QUADRO, ALTURA_DO_QUADRO);
    rascunho = LoadRenderTexture(LARGURA_DO_PLACAR, ALTURA_DO_PLACAR);
    RenderTexture2D *alvos[2] = { &quadro, &placar };
    int materiais[2] = { MATERIAL_DO_CUBO, MATERIAL_DO_PLACAR };
    for (int i = 0; i < 2; i++) {
        SetTextureFilter(alvos[i]->texture, TEXTURE_FILTER_BILINEAR);
        BeginTextureMode(*alvos[i]);
        ClearBackground(BLACK);
        EndTextureMode();
        Texture2D *textura = &estadio->materials[materiais[i]].maps[MATERIAL_MAP_ALBEDO].texture;
        originais[i] = *textura;
        *textura = alvos[i]->texture;
        estadio->materials[materiais[i]].maps[MATERIAL_MAP_ALBEDO].color = WHITE;
        estadio->materials[materiais[i]].shader = shaderDaTelaQueBrilha();
    }
    carregado = 1;
}

/*
 * descarregarTelao
 * Devolve as imagens originais ao modelo (ele as descarrega) e apaga as do jogo.
 * Recebe: nada.
 * Devolve: nada.
 */
void descarregarTelao(void) {
    if (!carregado) {
        return;
    }
    int materiais[2] = { MATERIAL_DO_CUBO, MATERIAL_DO_PLACAR };
    for (int i = 0; i < 2; i++) {
        modeloDoEstadio->materials[materiais[i]].maps[MATERIAL_MAP_ALBEDO].texture = originais[i];
    }
    for (int i = 0; i < 2; i++) {
        modeloDoEstadio->materials[materiais[i]].shader.id = rlGetShaderIdDefault();   /* o modelo não descarrega o nosso */
    }
    if (shaderDaTela.id != 0) {
        UnloadShader(shaderDaTela);
        shaderDaTela.id = 0;
    }
    UnloadRenderTexture(quadro);
    UnloadRenderTexture(placar);
    UnloadRenderTexture(rascunho);
    UnloadRenderTexture(bruto);
    carregado = 0;
}

/*
 * telaoPrecisaDeQuadro
 * Recebe: o tempo que passou.
 * Devolve: 1 se está na hora de desenhar mais um quadro da transmissão.
 */
int telaoPrecisaDeQuadro(float tempoQuadro) {
    if (!carregado || getenv("KATCHAU_SEM_TELAO") != NULL) {
        return 0;
    }
    desdeOUltimo += tempoQuadro;
    if (desdeOUltimo < 1.0f / QUADROS_POR_SEGUNDO) {
        return 0;
    }
    desdeOUltimo = 0.0f;
    return 1;
}

/*
 * comecarQuadroDoTelao
 * Começa a desenhar a transmissão: o que vier até terminarQuadroDoTelao vai para o rascunho.
 * Recebe: nada.
 * Devolve: nada.
 */
void comecarQuadroDoTelao(void) {
    rlDrawRenderBatchActive();
    BeginTextureMode(bruto);
    ClearBackground(BLACK);
}

/*
 * legenda
 * As legendas da transmissão, por cima da imagem: AO VIVO, o carro na tela e a câmera.
 */
static void legenda(Corrida *corrida, const Carro *mostrado, const char *camera) {
    float l = LARGURA_DO_QUADRO, a = ALTURA_DO_QUADRO;
    DrawRectangleRec((Rectangle){ 14, 14, 74, 22 }, COR_VERMELHO);
    DrawCircle(26, 25, 4.5f, (fmodf((float) GetTime(), 1.0f) < 0.6f) ? WHITE : Fade(WHITE, 0.3f));
    escrever(FONTE_FORTE, "AO VIVO", 35, 15, 17, COR_TEXTO_CLARO);
    int posicao = 1;
    for (int i = 0; i < TOTAL_CARROS; i++) {
        if (corrida->classificacao[i] == mostrado) {
            posicao = i + 1;
        }
    }
    DrawRectangleRec((Rectangle){ 14, a - 58, l * 0.62f, 42 }, Fade(COR_GRAFITE, 0.85f));
    DrawRectangleRec((Rectangle){ 14, a - 58, 44, 42 }, COR_VERMELHO);
    escreverCentralizado(FONTE_TITULO, TextFormat("%d", posicao), 36, a - 54, 28, COR_TEXTO_CLARO);
    escrever(FONTE_TITULO, mostrado->nome, 68, a - 56, 24, COR_TEXTO_CLARO);
    escrever(FONTE_FORTE, camera, 68, a - 31, 13, COR_AMARELO);
    escreverADireita(FONTE_TITULO, "KATCHAU", l - 16, 14, 18, Fade(COR_TEXTO_CLARO, 0.85f));
}

/*
 * terminarQuadroDoTelao
 * Põe as legendas e passa o quadro para os telões.
 * Recebe: a corrida, o carro na tela e o nome da câmera.
 * Devolve: nada.
 */
void terminarQuadroDoTelao(Corrida *corrida, const Carro *mostrado, const char *camera) {
    legenda(corrida, mostrado, camera);
    rlDrawRenderBatchActive();
    EndTextureMode();

    /* O quadro, virado do jeito que o modelo lê a imagem */
    Rectangle pedaco = { 0.0f, 0.0f, LARGURA_DO_QUADRO, VIRAR * ALTURA_DO_QUADRO };
    BeginTextureMode(quadro);
    DrawTexturePro(bruto.texture, pedaco, (Rectangle){ 0, 0, LARGURA_DO_QUADRO, ALTURA_DO_QUADRO }, (Vector2){ 0 }, 0.0f, WHITE);
    EndTextureMode();

}

/*
 * desenharTelasDoEstadio
 * Só as telas do estádio (o cubo e o placar), mais fracas, para o halo.
 * Recebe: nada. Devolve: nada.
 */
void desenharTelasDoEstadio(void) {
    if (!carregado) {
        return;
    }
    int materiais[2] = { MATERIAL_DO_CUBO, MATERIAL_DO_PLACAR };
    for (int i = 0; i < modeloDoEstadio->meshCount; i++) {
        int m = modeloDoEstadio->meshMaterial[i];
        if (m != materiais[0] && m != materiais[1]) {
            continue;
        }
        Material material = modeloDoEstadio->materials[m];
        material.maps[MATERIAL_MAP_ALBEDO].color = (Color){ 215, 215, 215, 255 };
        DrawMesh(modeloDoEstadio->meshes[i], material, MatrixMultiply(modeloDoEstadio->transform, MatrixScale(ESCALA_DO_ESTADIO, ESCALA_DO_ESTADIO, ESCALA_DO_ESTADIO)));
    }
}

/*
 * texturaDoTelao
 * Recebe: nada.
 * Devolve: o último quadro da transmissão (para mostrar fora do estádio ou conferir).
 */
Texture2D texturaDoTelao(void) {
    return quadro.texture;
}

/*
 * atualizarPlacar
 * O placar da torre: a volta do líder e os quinze primeiros, com o código de cada carro, em
 * LED âmbar sobre preto. O primeiro fica no alto.
 * Recebe: a corrida.
 * Devolve: nada.
 */
void atualizarPlacar(Corrida *corrida) {
    if (!carregado) {
        return;
    }
    static float desde = 1.0f;
    desde += GetFrameTime();
    if (desde < 0.25f) {
        return;
    }
    desde = 0.0f;
    Color led = { 255, 176, 40, 255 }, apagado = { 60, 34, 10, 255 };
    BeginTextureMode(rascunho);
    ClearBackground(BLACK);
    float l = LARGURA_DO_PLACAR, a = ALTURA_DO_PLACAR;
    int voltas = corrida->classificacao[0]->voltas + 1;
    if (voltas > corrida->totalVoltas) {
        voltas = corrida->totalVoltas;
    }
    escreverCentralizado(FONTE_FORTE, "VOLTA", l / 2.0f, 6, 16, apagado);
    escreverCentralizado(FONTE_FORTE, "VOLTA", l / 2.0f, 6, 16, led);
    escreverCentralizado(FONTE_TITULO, TextFormat("%d/%d", voltas, corrida->totalVoltas), l / 2.0f, 22, 30, led);
    float alturaDaLinha = (a - 64.0f) / LINHAS_NO_PLACAR;
    for (int i = 0; i < LINHAS_NO_PLACAR && i < TOTAL_CARROS; i++) {
        Carro *carro = corrida->classificacao[i];
        float y = 62.0f + i * alturaDaLinha;
        Color cor = (carro == &corrida->jogador) ? (Color){ 255, 240, 120, 255 } : led;
        escreverADireita(FONTE_FORTE, TextFormat("%d", i + 1), 30, y, alturaDaLinha * 0.8f, Fade(cor, 0.7f));
        escrever(FONTE_TITULO, codigoDoCarro(carro->numero), 42, y - 2, alturaDaLinha * 0.95f, cor);
    }
    rlDrawRenderBatchActive();
    EndTextureMode();
    BeginTextureMode(placar);
    ClearBackground(BLACK);
    DrawTexturePro(rascunho.texture, (Rectangle){ 0, 0, l, VIRAR * a }, (Rectangle){ 0, 0, l, a }, (Vector2){ 0 }, 0.0f, WHITE);
    EndTextureMode();
}
