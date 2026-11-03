/*
 * carros3d.c
 * ------------------------------------------------------------------
 * Carrega e desenha os modelos 3D dos carros. Veja carros3d.h.
 *
 * Cada carro é desenhado em duas passadas: primeiro o que é opaco, que
 * escreve na profundidade; depois o que é translúcido (o vidro), misturado
 * por cima, sem escrever. Assim dá para ver através do vidro sem precisar
 * saber quais peças de cada modelo são de vidro.
 * ------------------------------------------------------------------
 */

#include <math.h>
#include "carros3d.h"
#include "raymath.h"
#include "rlgl.h"
#include "corrida.h"
#include "noite.h"

#define PASTA_DOS_MODELOS     "recursos/modelos/carros/"
#define LADO_DA_MINIATURA     512
#define ALTURA_DA_SOMBRA      0.05f     /* metros acima do chão */
#define LADO_DA_SOMBRA        128       /* pixels da foto de cima */
#define FOLGA_DA_SOMBRA       1.35f     /* a foto cobre 1,35 vezes o comprimento do carro */
#define RAIO_DO_DESFOQUE      4

/* Cada modelo, na ordem de FICHAS (corrida.c). O comprimento é o do
 * personagem, em metros; "deitado" marca o modelo com a frente em +x. */
typedef struct {
    const char *arquivo;
    float comprimento;
    int deitado;
} ModeloDoCarro;

static const ModeloDoCarro MODELOS[TOTAL_CARROS] = {
    { "mcqueen.glb",    4.4f, 0 },
    { "the_king.glb",   5.3f, 0 },
    { "doc_hudson.glb", 5.0f, 0 },
    { "francesco.glb",  5.0f, 0 },
    { "sally.glb",      4.4f, 0 },
    { "ramone.glb",     5.4f, 0 },
    { "luigi.glb",      3.0f, 0 },
    { "guido.glb",      2.4f, 0 },
    { "mater.glb",      5.2f, 0 },
    { "sarge.glb",      3.4f, 0 },
    { "fillmore.glb",   4.3f, 0 },
    { "finn.glb",       4.6f, 1 },
    { "wingo.glb",      4.4f, 0 },
    { "snot_rod.glb",   4.6f, 0 },
};

static const char *VERTICES_DO_SHADER =
    "#version 330\n"
    "in vec3 vertexPosition; in vec2 vertexTexCoord; in vec3 vertexNormal; in vec4 vertexColor;\n"
    "uniform mat4 mvp; uniform mat4 matModel;\n"
    "out vec2 fragTexCoord; out vec4 fragColor; out vec3 fragNormal; out vec3 fragPos;\n"
    "void main() { fragTexCoord = vertexTexCoord; fragColor = vertexColor;\n"
    "              fragNormal = mat3(matModel) * vertexNormal;\n"
    "              fragPos = vec3(matModel * vec4(vertexPosition, 1.0));\n"
    "              gl_Position = mvp * vec4(vertexPosition, 1.0); }\n";

/* Só o contorno do carro, em preto: é a foto da sombra */
#define PIXELS_DA_SOMBRA_TEXTO(OPACIDADE) \
    "#version 330\n" \
    "in vec2 fragTexCoord; in vec4 fragColor; in vec3 fragNormal;\n" \
    "uniform sampler2D texture0; uniform vec4 colDiffuse;\n" \
    "out vec4 finalColor;\n" \
    "void main() { if ((texture(texture0, fragTexCoord) * colDiffuse).a < 0.5) discard;\n" \
    "              finalColor = vec4(0.0, 0.0, 0.0, " OPACIDADE "); }\n"

/* Luz de um sol alto mais o céu, para os carros terem volume. Os dois shaders
 * só mudam no corte: um desenha só o opaco, o outro só o translúcido. */
#define PIXELS_DO_SHADER(CORTE, SAIDA) \
    "#version 330\n" \
    "in vec2 fragTexCoord; in vec4 fragColor; in vec3 fragNormal; in vec3 fragPos;\n" \
    "uniform sampler2D texture0; uniform vec4 colDiffuse;\n" \
    "out vec4 finalColor;\n" \
    GLSL_DA_LUZ_DA_NOITE \
    "void main() { vec4 cor = texture(texture0, fragTexCoord) * colDiffuse * fragColor;\n" \
    "              " CORTE "\n" \
    "              vec3 n = normalize(fragNormal);\n" \
    "              if (!gl_FrontFacing) n = -n;\n" \
    "              float sol = max(dot(n, normalize(vec3(0.35, 0.8, 0.45))), 0.0);\n" \
    "              float luz = 0.62 + 0.28 * (n.y * 0.5 + 0.5) + 0.42 * sol;\n" \
    "              vec3 comLuz = (noite > 0.5) ? luzDaNoite(fragPos, n) * 1.45 : vec3(luz);\n" \
    "              finalColor = vec4(cor.rgb * comLuz, " SAIDA "); }\n"

static Model modelos[TOTAL_CARROS];
static Matrix ajuste[TOTAL_CARROS];     /* do modelo como veio para o modelo normalizado */
static float larguras[TOTAL_CARROS];    /* largura de verdade, em metros */
static RenderTexture2D miniaturas[TOTAL_CARROS];
static Shader shaderOpaco, shaderTranslucido, shaderDaSombra;
static Texture2D sombras[TOTAL_CARROS];
static float ladosDaSombra[TOTAL_CARROS];

/*
 * desenharComShader
 * Recebe: o modelo (já com a matriz certa) e o shader da passada.
 * Devolve: nada.
 */
static void desenharComShader(Model modelo, Shader shader) {
    for (int i = 0; i < modelo.materialCount; i++) {
        modelo.materials[i].shader = shader;
    }
    DrawModel(modelo, (Vector3){ 0.0f, 0.0f, 0.0f }, 1.0f, WHITE);
}

/*
 * desenharSombra
 * A sombra do carro: a foto dele visto de cima, desfocada, deitada no chão
 * embaixo dele. Tem o formato de cada modelo e bordas suaves.
 * Recebe: o número do carro e a matriz dele no mundo.
 * Devolve: nada.
 */
static void desenharSombra(int numero, Matrix noMundo) {
    float meio = ladosDaSombra[numero] / 2.0f;
    /* Na foto, a imagem cresce para a direita em -x e para baixo em -z */
    Vector3 a = Vector3Transform((Vector3){  meio, ALTURA_DA_SOMBRA,  meio }, noMundo);
    Vector3 b = Vector3Transform((Vector3){  meio, ALTURA_DA_SOMBRA, -meio }, noMundo);
    Vector3 c = Vector3Transform((Vector3){ -meio, ALTURA_DA_SOMBRA, -meio }, noMundo);
    Vector3 d = Vector3Transform((Vector3){ -meio, ALTURA_DA_SOMBRA,  meio }, noMundo);

    BeginBlendMode(BLEND_ALPHA);
    rlDisableDepthMask();
    rlSetTexture(sombras[numero].id);
    rlBegin(RL_QUADS);
    rlColor4ub(255, 255, 255, 255);
    rlTexCoord2f(0.0f, 0.0f); rlVertex3f(a.x, a.y, a.z);
    rlTexCoord2f(0.0f, 1.0f); rlVertex3f(b.x, b.y, b.z);
    rlTexCoord2f(1.0f, 1.0f); rlVertex3f(c.x, c.y, c.z);
    rlTexCoord2f(1.0f, 0.0f); rlVertex3f(d.x, d.y, d.z);
    rlEnd();
    rlSetTexture(0);
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
    EndBlendMode();
}

/*
 * prepararSombra
 * Fotografa o carro de cima, só o contorno em preto, e desfoca. A foto tem
 * uma folga em volta, para o desfoque caber.
 * Recebe: o número do carro.
 * Devolve: nada.
 */
static void prepararSombra(int numero) {
    float lado = MODELOS[numero].comprimento * FOLGA_DA_SOMBRA;
    Camera3D camera = { 0 };
    camera.position = (Vector3){ 0.0f, 20.0f, 0.0f };
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 0.0f, 1.0f };
    camera.fovy = lado;
    camera.projection = CAMERA_ORTHOGRAPHIC;

    RenderTexture2D foto = LoadRenderTexture(LADO_DA_SOMBRA, LADO_DA_SOMBRA);
    BeginTextureMode(foto);
    ClearBackground(BLANK);
    BeginMode3D(camera);
    rlDisableBackfaceCulling();
    Model modelo = modelos[numero];
    modelo.transform = ajuste[numero];
    desenharComShader(modelo, shaderDaSombra);
    rlEnableBackfaceCulling();
    EndMode3D();
    EndTextureMode();

    Image imagem = LoadImageFromTexture(foto.texture);
    ImageFlipVertical(&imagem);
    ImageBlurGaussian(&imagem, RAIO_DO_DESFOQUE);
    sombras[numero] = LoadTextureFromImage(imagem);
    GenTextureMipmaps(&sombras[numero]);
    SetTextureFilter(sombras[numero], TEXTURE_FILTER_TRILINEAR);
    ladosDaSombra[numero] = lado;
    UnloadImage(imagem);
    UnloadRenderTexture(foto);
}

/*
 * desenharCarro3D
 * Desenha um carro em qualquer lugar do mundo. Chamar dentro de BeginMode3D.
 * Recebe: o número do carro e a matriz que leva o carro normalizado (centro
 *         na origem, rodas em y = 0, frente em +z) até o lugar dele.
 * Devolve: nada.
 */
void desenharCarro3D(int numero, Matrix noMundo) {
    rlDrawRenderBatchActive();   /* o que já estava na fila sai antes de mudar a profundidade */
    desenharSombra(numero, noMundo);

    Model modelo = modelos[numero];
    modelo.transform = MatrixMultiply(ajuste[numero], noMundo);

    desenharComShader(modelo, shaderOpaco);

    BeginBlendMode(BLEND_ALPHA);
    rlDisableDepthMask();
    desenharComShader(modelo, shaderTranslucido);
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
    EndBlendMode();
}

/*
 * larguraDoCarro3D
 * Recebe: o número do carro.
 * Devolve: a largura dele, em metros.
 */
float larguraDoCarro3D(int numero) {
    return larguras[numero];
}

/*
 * comprimentoDoCarro3D
 * Recebe: o número do carro.
 * Devolve: o comprimento dele, em metros.
 */
float comprimentoDoCarro3D(int numero) {
    return MODELOS[numero].comprimento;
}

/*
 * normalizarModelo
 * Mede o modelo e monta a matriz que o deixa com o comprimento de verdade,
 * centrado, com as rodas no chão e a frente para +z.
 * Recebe: o número do carro.
 * Devolve: nada (preenche ajuste e larguras).
 */
static void normalizarModelo(int numero) {
    BoundingBox caixa = GetModelBoundingBox(modelos[numero]);
    float dx = caixa.max.x - caixa.min.x;
    float dz = caixa.max.z - caixa.min.z;
    int deitado = MODELOS[numero].deitado;
    float escala = MODELOS[numero].comprimento / (deitado ? dx : dz);

    Matrix centralizar = MatrixTranslate(-(caixa.min.x + caixa.max.x) / 2.0f, -caixa.min.y,
                                         -(caixa.min.z + caixa.max.z) / 2.0f);
    Matrix girar = deitado ? MatrixRotateY(-90.0f * DEG2RAD) : MatrixIdentity();
    ajuste[numero] = MatrixMultiply(MatrixMultiply(centralizar, girar), MatrixScale(escala, escala, escala));
    larguras[numero] = (deitado ? dz : dx) * escala;
}

/*
 * prepararTexturas
 * Mipmaps e filtro nas texturas do modelo, para ficarem nítidas ao longe.
 * Recebe: o modelo.
 * Devolve: nada.
 */
static void prepararTexturas(Model modelo) {
    for (int i = 0; i < modelo.materialCount; i++) {
        Texture2D *textura = &modelo.materials[i].maps[MATERIAL_MAP_ALBEDO].texture;
        if (textura->id != 0 && textura->id != rlGetTextureIdDefault()) {
            GenTextureMipmaps(textura);
            SetTextureFilter(*textura, TEXTURE_FILTER_TRILINEAR);
            SetTextureFilter(*textura, TEXTURE_FILTER_ANISOTROPIC_8X);
        }
    }
}

/*
 * prepararMiniatura
 * Tira uma "foto" do carro, de frente e um pouco de lado, numa textura.
 * O enquadramento segue o comprimento do carro: todos cabem na foto.
 * Recebe: o número do carro.
 * Devolve: nada.
 */
static void prepararMiniatura(int numero) {
    float tamanho = MODELOS[numero].comprimento;
    Camera3D camera = { 0 };
    camera.position = (Vector3){ tamanho * 0.95f, tamanho * 0.48f, tamanho * 1.15f };
    camera.target = (Vector3){ 0.0f, tamanho * 0.13f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 34.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    miniaturas[numero] = LoadRenderTexture(LADO_DA_MINIATURA, LADO_DA_MINIATURA);
    BeginTextureMode(miniaturas[numero]);
    ClearBackground(BLANK);
    BeginMode3D(camera);
    rlDisableBackfaceCulling();
    desenharCarro3D(numero, MatrixIdentity());
    rlEnableBackfaceCulling();
    EndMode3D();
    EndTextureMode();
    SetTextureFilter(miniaturas[numero].texture, TEXTURE_FILTER_BILINEAR);
}

/*
 * desenharMiniatura
 * A foto de um carro, em 2D, para as telas de menu.
 * Recebe: o número do carro, o centro, a base e a largura na tela.
 * Devolve: nada.
 */
void desenharMiniatura(int numero, float centroX, float baseY, float larguraNaTela) {
    Texture2D foto = miniaturas[numero].texture;
    Rectangle origem = { 0.0f, 0.0f, (float) foto.width, -(float) foto.height };   /* a foto vem de cabeça para baixo */
    float lado = larguraNaTela * 1.25f;
    Rectangle destino = { centroX - lado / 2.0f, baseY - lado * 0.78f, lado, lado };
    DrawTexturePro(foto, origem, destino, (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);
}

/*
 * carregarCarros3D
 * Carrega os modelos e prepara as miniaturas. Chamar depois de criar a janela.
 * Recebe: nada.
 * Devolve: nada.
 */
void carregarCarros3D(void) {
    const char *pixelsOpaco = PIXELS_DO_SHADER("if (cor.a < 0.9) discard;", "1.0");
    const char *pixelsTranslucido = PIXELS_DO_SHADER("if (cor.a >= 0.9 || cor.a < 0.02) discard;", "cor.a");
    shaderOpaco = LoadShaderFromMemory(VERTICES_DO_SHADER, pixelsOpaco);
    shaderTranslucido = LoadShaderFromMemory(VERTICES_DO_SHADER, pixelsTranslucido);
    registrarShaderIluminado(shaderOpaco);
    registrarShaderIluminado(shaderTranslucido);
    shaderDaSombra = LoadShaderFromMemory(VERTICES_DO_SHADER, PIXELS_DA_SOMBRA_TEXTO("0.82"));

    for (int numero = 0; numero < TOTAL_CARROS; numero++) {
        modelos[numero] = LoadModel(TextFormat("%s%s", PASTA_DOS_MODELOS, MODELOS[numero].arquivo));
        prepararTexturas(modelos[numero]);
        normalizarModelo(numero);
        prepararSombra(numero);
        prepararMiniatura(numero);
    }
}

/*
 * descarregarCarros3D
 * Devolve a memória de vídeo dos carros. Chamar antes de CloseWindow.
 * Recebe: nada.
 * Devolve: nada.
 */
void descarregarCarros3D(void) {
    for (int numero = 0; numero < TOTAL_CARROS; numero++) {
        /* Os shaders são compartilhados: são devolvidos uma vez só, aqui embaixo */
        for (int i = 0; i < modelos[numero].materialCount; i++) {
            modelos[numero].materials[i].shader.id = rlGetShaderIdDefault();
        }
        UnloadModel(modelos[numero]);
        UnloadRenderTexture(miniaturas[numero]);
        UnloadTexture(sombras[numero]);
    }
    UnloadShader(shaderOpaco);
    UnloadShader(shaderTranslucido);
    UnloadShader(shaderDaSombra);
}
