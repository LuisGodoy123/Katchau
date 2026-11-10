/*
 * posprocesso.c
 * ------------------------------------------------------------------
 * Os efeitos de tela. Veja posprocesso.h.
 *
 * O caminho de cada quadro:
 *   1) a cena 3D vai para "cena", do tamanho da janela;
 *   2) o que emite luz (refletores, flashes, chamas do nitro, telas dos telões)
 *      é desenhado de novo numa imagem de 1/4 do tamanho e borrado duas vezes
 *      em cada direção: esse é o halo (o branco só iluminado não brilha);
 *   3) na tela, um shader junta a cena e o halo, corrige a cor (o tom de cada
 *      pista, contraste e saturação), escurece as bordas (vinheta) e, em alta
 *      velocidade, borra as bordas na direção do centro.
 * ------------------------------------------------------------------
 */

#include <stddef.h>
#include "raylib.h"
#include "rlgl.h"
#include "posprocesso.h"
#include "pista.h"

#define REDUCAO_DO_HALO   4       /* o halo é feito em 1/4 da tela: barato e mais largo */

/* O tom de cada pista */
typedef struct {
    Vector3 tom;
    float contraste, saturacao, forcaDoHalo, limiar, vinheta;
} TomDaPista;

static const TomDaPista TONS[TOTAL_PISTAS] = {
    { { 1.04f, 1.00f, 0.94f }, 1.07f, 1.12f, 0.70f, 0.0f, 0.28f },   /* Interlagos: tarde quente */
    { { 0.95f, 0.99f, 1.07f }, 1.08f, 1.10f, 0.85f, 0.0f, 0.34f },   /* Copa Pistão: noite azulada, luzes fortes */
    { { 1.02f, 1.00f, 0.97f }, 1.05f, 1.16f, 0.70f, 0.0f, 0.26f },   /* Oval: dia de cartoon */
    { { 1.00f, 1.02f, 0.95f }, 1.06f, 1.12f, 0.70f, 0.0f, 0.26f },   /* Splat */
};

static const char *PIXELS_CLAROS =
    "#version 330\n"
    "in vec2 fragTexCoord; uniform sampler2D texture0; uniform float limiar;\n"
    "out vec4 finalColor;\n"
    "void main() { vec3 c = texture(texture0, fragTexCoord).rgb;\n"
    "              float l = dot(c, vec3(0.2126, 0.7152, 0.0722));\n"   /* pelo brilho, não pela cor: o céu azul não brilha */
    "              finalColor = vec4(c * smoothstep(limiar, limiar + 0.2, l), 1.0); }\n";

static const char *PIXELS_BORRAR =
    "#version 330\n"
    "in vec2 fragTexCoord; uniform sampler2D texture0; uniform vec2 passo;\n"
    "out vec4 finalColor;\n"
    "void main() {\n"
    "    float p[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);\n"
    "    vec3 c = texture(texture0, fragTexCoord).rgb * p[0];\n"
    "    for (int i = 1; i < 5; i++) {\n"
    "        c += texture(texture0, fragTexCoord + passo * float(i) * 1.5).rgb * p[i];\n"
    "        c += texture(texture0, fragTexCoord - passo * float(i) * 1.5).rgb * p[i];\n"
    "    }\n"
    "    finalColor = vec4(c, 1.0);\n"
    "}\n";

static const char *PIXELS_FINAL =
    "#version 330\n"
    "in vec2 fragTexCoord; uniform sampler2D texture0; uniform sampler2D halo;\n"
    "uniform vec3 tom; uniform float contraste, saturacao, forcaDoHalo, vinheta, desfoque, virarHalo;\n"
    "out vec4 finalColor;\n"
    "void main() {\n"
    "    vec2 uv = fragTexCoord;\n"
    "    vec3 c;\n"
    "    if (desfoque > 0.001) {\n"   /* desfoque de velocidade: só nas bordas, puxando para o centro */
    "        vec2 d = uv - vec2(0.5, 0.48);\n"
    "        float bordas = smoothstep(0.18, 0.75, length(d));\n"
    "        c = vec3(0.0);\n"
    "        for (int i = 0; i < 8; i++) c += texture(texture0, uv - d * desfoque * bordas * float(i) * 0.016).rgb;\n"
    "        c /= 8.0;\n"
    "    } else {\n"
    "        c = texture(texture0, uv).rgb;\n"
    "    }\n"
    "    vec2 uvHalo = (virarHalo > 0.5) ? vec2(uv.x, 1.0 - uv.y) : uv;\n"
    "    c += texture(halo, uvHalo).rgb * forcaDoHalo;\n"
    "    c *= tom;\n"
    "    float l = dot(c, vec3(0.2126, 0.7152, 0.0722));\n"
    "    c = mix(vec3(l), c, saturacao);\n"
    "    c = (c - 0.5) * contraste + 0.5;\n"
    "    c *= 1.0 - vinheta * smoothstep(0.42, 0.95, length((uv - 0.5) * vec2(1.25, 1.0)));\n"
    "    finalColor = vec4(clamp(c, 0.0, 1.0), 1.0);\n"
    "}\n";

static RenderTexture2D cena, haloA, haloB;
static Shader shaderClaros, shaderBorrar, shaderFinal;
static int locLimiar, locPasso, locHalo, locTom, locContraste, locSaturacao, locForca, locVinheta, locDesfoque, locVirar;
static int largura = 0, altura = 0, carregado = 0, desenhando = 0;   /* desenhando: 1 a cena, 2 as luzes */
/* Onde a cena vai aparecer na tela. Largura 0 = a tela cheia (o normal). Na tela dividida as duas
 * metades têm o MESMO tamanho, então as imagens são criadas uma vez e não mudam mais de quadro
 * para quadro: o que muda é só o canto em que elas são desenhadas. */
static int areaX = 0, areaY = 0, areaLargura = 0, areaAltura = 0;

/*
 * larguraDaArea / alturaDaArea
 * Recebe: nada.
 * Devolve: o tamanho em que a cena é desenhada (a área escolhida, ou a janela toda).
 */
static int larguraDaArea(void) {
    return (areaLargura > 0) ? areaLargura : GetScreenWidth();
}
static int alturaDaArea(void) {
    return (areaAltura > 0) ? areaAltura : GetScreenHeight();
}

/*
 * definirAreaDoPosProcesso
 * Recebe: o pedaço da tela em que a cena vai aparecer (largura 0 = a tela cheia).
 * Devolve: nada.
 */
void definirAreaDoPosProcesso(int x, int y, int largura, int altura) {
    areaX = x;
    areaY = y;
    areaLargura = largura;
    areaAltura = altura;
}

/*
 * criarImagens
 * As imagens do tamanho da área de agora (refeitas se ela mudar).
 */
static void criarImagens(void) {
    if (largura == larguraDaArea() && altura == alturaDaArea()) {
        return;
    }
    if (largura != 0) {
        UnloadRenderTexture(cena);
        UnloadRenderTexture(haloA);
        UnloadRenderTexture(haloB);
    }
    largura = larguraDaArea();
    altura = alturaDaArea();
    cena = LoadRenderTexture(largura, altura);
    haloA = LoadRenderTexture(largura / REDUCAO_DO_HALO, altura / REDUCAO_DO_HALO);
    haloB = LoadRenderTexture(largura / REDUCAO_DO_HALO, altura / REDUCAO_DO_HALO);
    SetTextureFilter(cena.texture, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(haloA.texture, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(haloB.texture, TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(cena.texture, TEXTURE_WRAP_CLAMP);
    SetTextureWrap(haloA.texture, TEXTURE_WRAP_CLAMP);
    SetTextureWrap(haloB.texture, TEXTURE_WRAP_CLAMP);
}

/*
 * carregarPosProcesso
 * Recebe: nada. Devolve: nada.
 */
void carregarPosProcesso(void) {
    shaderClaros = LoadShaderFromMemory(NULL, PIXELS_CLAROS);
    shaderBorrar = LoadShaderFromMemory(NULL, PIXELS_BORRAR);
    shaderFinal = LoadShaderFromMemory(NULL, PIXELS_FINAL);
    locLimiar = GetShaderLocation(shaderClaros, "limiar");
    locPasso = GetShaderLocation(shaderBorrar, "passo");
    locHalo = GetShaderLocation(shaderFinal, "halo");
    locTom = GetShaderLocation(shaderFinal, "tom");
    locContraste = GetShaderLocation(shaderFinal, "contraste");
    locSaturacao = GetShaderLocation(shaderFinal, "saturacao");
    locForca = GetShaderLocation(shaderFinal, "forcaDoHalo");
    locVinheta = GetShaderLocation(shaderFinal, "vinheta");
    locDesfoque = GetShaderLocation(shaderFinal, "desfoque");
    locVirar = GetShaderLocation(shaderFinal, "virarHalo");
    carregado = 1;
}

/*
 * descarregarPosProcesso
 * Recebe: nada. Devolve: nada.
 */
void descarregarPosProcesso(void) {
    if (!carregado) {
        return;
    }
    if (largura != 0) {
        UnloadRenderTexture(cena);
        UnloadRenderTexture(haloA);
        UnloadRenderTexture(haloB);
        largura = altura = 0;
    }
    UnloadShader(shaderClaros);
    UnloadShader(shaderBorrar);
    UnloadShader(shaderFinal);
    carregado = 0;
}

/*
 * comecarPosProcesso
 * O que for desenhado até terminarPosProcesso vai para a imagem da cena.
 * Recebe: nada. Devolve: nada.
 */
void comecarPosProcesso(void) {
    if (!carregado) {
        return;
    }
    criarImagens();
    rlDrawRenderBatchActive();
    BeginTextureMode(cena);
    ClearBackground(BLACK);
    desenhando = 1;
}

/*
 * comecarLuzesDoPosProcesso
 * Depois da cena: o que emite luz (refletores, flashes, chamas, telas) é desenhado de novo, numa
 * imagem pequena só dele. O halo nasce só daí: o branco que só está iluminado não brilha.
 * Recebe: nada. Devolve: nada.
 */
void comecarLuzesDoPosProcesso(void) {
    if (desenhando != 1) {
        return;
    }
    rlDrawRenderBatchActive();
    EndTextureMode();
    BeginTextureMode(haloA);
    ClearBackground(BLACK);
    desenhando = 2;
}

/*
 * passar
 * Desenha uma imagem inteira em outra com um shader (de cabeça para baixo, como sai da RenderTexture).
 */
static void passar(Texture2D de, RenderTexture2D para, Shader shader) {
    BeginTextureMode(para);
    BeginShaderMode(shader);
    DrawTexturePro(de, (Rectangle){ 0, 0, (float) de.width, (float) -de.height },
                   (Rectangle){ 0, 0, (float) para.texture.width, (float) para.texture.height }, (Vector2){ 0 }, 0.0f, WHITE);
    EndShaderMode();
    EndTextureMode();
}

/*
 * terminarPosProcesso
 * Faz o halo e põe a cena na tela (na área escolhida), com o tom da pista.
 * Recebe: a pista (o tom), o desfoque de velocidade (0 = nenhum, 1 = o máximo) e o acabamento:
 *         só copiar, sem o halo (a tela dividida) ou completo.
 * Devolve: nada.
 */
void terminarPosProcesso(int pista, float desfoque, AcabamentoDoPosProcesso acabamento) {
    if (!desenhando) {
        return;
    }
    int comLuzes = (desenhando == 2);
    desenhando = 0;
    rlDrawRenderBatchActive();
    EndTextureMode();
    /* De onde sai e para onde vai: a imagem inteira, no pedaço de tela da área */
    Rectangle daImagem = { 0, 0, (float) largura, (float) -altura };
    Rectangle naTela = { (float) areaX, (float) areaY, (float) largura, (float) altura };
    if (acabamento == POSPROCESSO_SO_COPIA) {
        /* Sem os efeitos: só copia a cena para a tela (desenhar direto na janela é mais lento) */
        DrawTexturePro(cena.texture, daImagem, naTela, (Vector2){ 0 }, 0.0f, WHITE);
        return;
    }
    const TomDaPista *t = &TONS[(pista >= 0 && pista < TOTAL_PISTAS) ? pista : 0];
    int comHalo = (acabamento == POSPROCESSO_COMPLETO) && comLuzes;

    /* O halo: as luzes, borradas duas vezes em cada direção */
    if (comHalo) {
        Vector2 horizontal = { 1.0f / haloA.texture.width, 0.0f }, vertical = { 0.0f, 1.0f / haloA.texture.height };
        for (int i = 0; i < 2; i++) {
            SetShaderValue(shaderBorrar, locPasso, &horizontal, SHADER_UNIFORM_VEC2);
            passar(haloA.texture, haloB, shaderBorrar);
            SetShaderValue(shaderBorrar, locPasso, &vertical, SHADER_UNIFORM_VEC2);
            passar(haloB.texture, haloA, shaderBorrar);
        }
    }

    /* Na tela */
    float virar = 0.0f;   /* as luzes e a cena foram desenhadas do mesmo jeito: nada a virar */
    float forcaDoHalo = comHalo ? t->forcaDoHalo : 0.0f;
    SetShaderValue(shaderFinal, locTom, &t->tom, SHADER_UNIFORM_VEC3);
    SetShaderValue(shaderFinal, locContraste, &t->contraste, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shaderFinal, locSaturacao, &t->saturacao, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shaderFinal, locForca, &forcaDoHalo, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shaderFinal, locVinheta, &t->vinheta, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shaderFinal, locDesfoque, &desfoque, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shaderFinal, locVirar, &virar, SHADER_UNIFORM_FLOAT);
    BeginShaderMode(shaderFinal);
    SetShaderValueTexture(shaderFinal, locHalo, haloA.texture);   /* sem halo, a força é 0: a imagem não pesa */
    DrawTexturePro(cena.texture, daImagem, naTela, (Vector2){ 0 }, 0.0f, WHITE);
    EndShaderMode();
}
