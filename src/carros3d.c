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
#include <stdlib.h>
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

/* ---------- Rodas ---------- */
#define FIM_DA_RODA_VELHA     0.60f     /* na troca: a velha some aqui, a nova aparece ali */
#define INICIO_DA_RODA_NOVA   0.40f
#define VOO_DA_RODA_VELHA     2.6f      /* metros para o lado */
#define ALTURA_DA_RODA_VELHA  0.9f
#define ENTRADA_DA_RODA_NOVA  1.1f
#define GIRO_NA_TROCA         26.0f     /* radianos a mais durante a troca */
#define MAXIMO_DE_RODAS       12
#define MENOR_QUE_A_MAIOR     0.45f     /* peça redonda menor que isso, em relação à maior roda, não é roda */
#define MAXIMO_DE_PECAS_RODA  16        /* uma por material, somando todas as rodas */
#define PERTO_DO_CHAO         0.10f     /* a borracha encosta no chão: em alturas do carro */
#define MENOR_RODA            0.10f     /* em alturas do carro */
#define REDONDA               0.35f     /* altura e comprimento da peça podem diferir só isso, em relação à altura */
#define LARGURA_MAXIMA_RODA   0.40f     /* em larguras do carro */
#define MESMA_RODA_DE_LADO    0.14f     /* peças mais perto que isso, de lado, são a mesma roda (em larguras) */
#define FOLGA_DENTRO_DA_RODA  1.06f     /* a peça precisa caber no círculo da roda */
#define FOLGA_DE_LADO_DA_RODA 0.35f     /* e não passar mais que isso (em larguras de roda) para os lados */
#define GRAUS_DAS_RODAS_DA_FRENTE 24.0f /* com o volante todo */

/* Uma ilha: um pedaço contínuo de triângulos de uma malha */
typedef struct {
    int triangulos;
    Vector3 minimo, maximo;
} Ilha;

/* Uma roda: onde fica o eixo dela, no carro normalizado */
typedef struct {
    Vector3 centro;
    float raio, xMinimo, xMaximo;
} RodaDoCarro;

/* As peças de uma roda que usam o mesmo material */
typedef struct {
    Mesh malha;        /* com o centro da roda na origem */
    int material;
    int roda;
} PecaDeRoda;

/* Cada modelo, na ordem de FICHAS (corrida.c). O comprimento está
 * na ficha; "deitado" marca o modelo com a frente em +x. */
typedef struct {
    const char *arquivo;
    int deitado;
} ModeloDoCarro;

static const ModeloDoCarro MODELOS[TOTAL_CARROS] = {
    { "mcqueen.glb", 0 },
    { "the_king.glb", 0 },
    { "doc_hudson.glb", 0 },
    { "francesco.glb", 0 },
    { "sally.glb", 0 },
    { "ramone.glb", 0 },
    { "luigi.glb", 0 },
    { "guido.glb", 0 },
    { "mater.glb", 0 },
    { "sarge.glb", 0 },
    { "fillmore.glb", 0 },
    { "finn.glb", 1 },
    { "wingo.glb", 0 },
    { "snot_rod.glb", 0 },
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
    "              vec3 comLuz = (noite > 0.5) ? luzDaNoite(fragPos, n) * 1.1 : vec3(luz);\n" \
    "              finalColor = vec4(cor.rgb * comLuz, " SAIDA "); }\n"

static Model modelos[TOTAL_CARROS];
static Matrix ajuste[TOTAL_CARROS];     /* do modelo como veio para o modelo normalizado */
static float larguras[TOTAL_CARROS];    /* largura de verdade, em metros */
static RenderTexture2D miniaturas[TOTAL_CARROS];
static Shader shaderOpaco, shaderTranslucido, shaderDaSombra;
static Texture2D sombras[TOTAL_CARROS];
static RodaDoCarro rodas[TOTAL_CARROS][MAXIMO_DE_RODAS];
static int totalDeRodas[TOTAL_CARROS];
static PecaDeRoda pecas[TOTAL_CARROS][MAXIMO_DE_PECAS_RODA * MAXIMO_DE_RODAS];
static int totalDePecas[TOTAL_CARROS];
static int rodaEmTroca = -1;          /* a roda que está sendo trocada no próximo carro desenhado (-1 = nenhuma) */
static float andamentoDaTroca = 0.0f;
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
 * desenharPecaTrocando
 * A roda que o Guido está trocando: a velha sai voando para o lado, quicando, e a
 * nova entra, girando rápido.
 * Recebe: a peça, a roda, o modelo, a matriz do carro no mundo e o ângulo das rodas.
 * Devolve: nada.
 */
static void desenharPecaTrocando(PecaDeRoda *peca, RodaDoCarro *roda, Model modelo, Matrix noMundo, float anguloDasRodas) {
    float lado = (roda->centro.x > 0.0f) ? 1.0f : -1.0f;
    float giro = anguloDasRodas + andamentoDaTroca * GIRO_NA_TROCA;
    if (andamentoDaTroca < FIM_DA_RODA_VELHA) {
        float t = andamentoDaTroca / FIM_DA_RODA_VELHA;
        float ponta = (roda->centro.z > 0.0f) ? 1.0f : -1.0f;   /* para longe do Guido, que está de frente para a roda */
        Vector3 lugar = { roda->centro.x + lado * t * VOO_DA_RODA_VELHA, roda->centro.y + sinf(t * PI) * ALTURA_DA_RODA_VELHA, roda->centro.z + ponta * t * VOO_DA_RODA_VELHA };
        Matrix ali = MatrixMultiply(MatrixRotateX(giro), MatrixTranslate(lugar.x, lugar.y, lugar.z));
        DrawMesh(peca->malha, modelo.materials[peca->material], MatrixMultiply(ali, noMundo));
    }
    if (andamentoDaTroca > INICIO_DA_RODA_NOVA) {
        float t = (andamentoDaTroca - INICIO_DA_RODA_NOVA) / (1.0f - INICIO_DA_RODA_NOVA);
        Vector3 lugar = { roda->centro.x + lado * (1.0f - t) * ENTRADA_DA_RODA_NOVA, roda->centro.y, roda->centro.z };
        Matrix ali = MatrixMultiply(MatrixRotateX(giro), MatrixTranslate(lugar.x, lugar.y, lugar.z));
        DrawMesh(peca->malha, modelo.materials[peca->material], MatrixMultiply(ali, noMundo));
    }
}

/*
 * desenharRodas
 * Cada peça de roda gira em volta do eixo (x) e, nas rodas da frente, vira com
 * o volante (em volta de y), a partir do centro da roda.
 * Recebe: o número do carro, o modelo (já com os materiais no shader certo), a matriz
 *         do carro no mundo, o ângulo das rodas e o volante.
 * Devolve: nada.
 */
static void desenharRodas(int numero, Model modelo, Matrix noMundo, float anguloDasRodas, float volante) {
    for (int i = 0; i < totalDePecas[numero]; i++) {
        PecaDeRoda *peca = &pecas[numero][i];
        RodaDoCarro *roda = &rodas[numero][peca->roda];
        if (peca->roda == rodaEmTroca) {
            desenharPecaTrocando(peca, roda, modelo, noMundo, anguloDasRodas);
            continue;
        }
        Matrix giro = MatrixRotateX(anguloDasRodas);
        if (roda->centro.z > 0.0f) {
            giro = MatrixMultiply(giro, MatrixRotateY(-volante * GRAUS_DAS_RODAS_DA_FRENTE * DEG2RAD));
        }
        Matrix lugar = MatrixMultiply(giro, MatrixTranslate(roda->centro.x, roda->centro.y, roda->centro.z));
        DrawMesh(peca->malha, modelo.materials[peca->material], MatrixMultiply(lugar, noMundo));
    }
}

/*
 * desenharSombra
 * A sombra do carro: a foto dele visto de cima, desfocada, deitada no chão
 * embaixo dele. Tem o formato de cada modelo e bordas suaves.
 * Recebe: o número do carro e a matriz dele no mundo.
 * Devolve: nada.
 */
void desenharSombra(int numero, Matrix noMundo) {
    Vector3 grade[PONTOS_DA_SOMBRA * PONTOS_DA_SOMBRA];
    float meio = ladosDaSombra[numero] / 2.0f;
    for (int i = 0; i < PONTOS_DA_SOMBRA; i++) {
        for (int j = 0; j < PONTOS_DA_SOMBRA; j++) {
            float noCarroX = meio - 2.0f * meio * i / (PONTOS_DA_SOMBRA - 1);
            float noCarroZ = meio - 2.0f * meio * j / (PONTOS_DA_SOMBRA - 1);
            grade[i * PONTOS_DA_SOMBRA + j] = Vector3Transform((Vector3){ noCarroX, ALTURA_DA_SOMBRA, noCarroZ }, noMundo);
        }
    }
    desenharSombraNoChao(numero, grade);
}

/*
 * desenharSombraNoChao
 * Desenha a sombra como uma malha, com cada ponto já no lugar dele: assim ela dobra
 * junto com o chão, em vez de afundar num trecho inclinado.
 * Recebe: o número do carro e os pontos da malha. A primeira coluna é a esquerda do
 *         carro, e cada coluna vai da frente para trás.
 * Devolve: nada.
 */
void desenharSombraNoChao(int numero, const Vector3 *grade) {
    float passo = 1.0f / (PONTOS_DA_SOMBRA - 1);
    BeginBlendMode(BLEND_ALPHA);
    rlDisableDepthMask();
    rlSetTexture(sombras[numero].id);
    rlBegin(RL_QUADS);
    rlColor4ub(255, 255, 255, 255);
    for (int i = 0; i + 1 < PONTOS_DA_SOMBRA; i++) {
        for (int j = 0; j + 1 < PONTOS_DA_SOMBRA; j++) {
            Vector3 a = grade[i * PONTOS_DA_SOMBRA + j], b = grade[i * PONTOS_DA_SOMBRA + j + 1];
            Vector3 c = grade[(i + 1) * PONTOS_DA_SOMBRA + j + 1], d = grade[(i + 1) * PONTOS_DA_SOMBRA + j];
            float u0 = i * passo, u1 = (i + 1) * passo, v0 = j * passo, v1 = (j + 1) * passo;
            rlTexCoord2f(u0, v0); rlVertex3f(a.x, a.y, a.z);
            rlTexCoord2f(u0, v1); rlVertex3f(b.x, b.y, b.z);
            rlTexCoord2f(u1, v1); rlVertex3f(c.x, c.y, c.z);
            rlTexCoord2f(u1, v0); rlVertex3f(d.x, d.y, d.z);
        }
    }
    rlEnd();
    rlSetTexture(0);
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
    EndBlendMode();
}

/*
 * rodaDoCanto
 * Recebe: o carro e um canto (0 frente esquerda, 1 trás esquerda, 2 trás direita,
 *         3 frente direita) e onde guardar o centro da roda, no carro normalizado.
 * Devolve: o índice da roda mais perto desse canto (o Guido tem só 3 rodas: a da frente
 *          vale pelos dois cantos), ou -1 se o carro não tem rodas.
 */
int rodaDoCanto(int numero, int canto, Vector3 *centro) {
    float esquerda = (canto == 0 || canto == 1) ? 1.0f : -1.0f;
    float frente = (canto == 0 || canto == 3) ? 1.0f : -1.0f;
    float meioLado = larguraDoCarro3D(numero) / 2.0f, meioFrente = comprimentoDoCarro3D(numero) / 2.0f;
    float menor = 1e9f;
    int achou = -1;
    for (int r = 0; r < totalDeRodas[numero]; r++) {
        Vector3 c = rodas[numero][r].centro;
        float d = hypotf(c.x - esquerda * meioLado, c.z - frente * meioFrente);
        if (d < menor) {
            menor = d;
            achou = r;
        }
    }
    if (achou >= 0) {
        *centro = rodas[numero][achou].centro;
    }
    return achou;
}

/*
 * trocarRodaNoProximoDesenho
 * O próximo carro desenhado mostra esta roda sendo trocada.
 * Recebe: o índice da roda e o andamento da troca (0 a 1).
 * Devolve: nada.
 */
void trocarRodaNoProximoDesenho(int roda, float andamento) {
    rodaEmTroca = roda;
    andamentoDaTroca = andamento;
}

/*
 * ladoDaSombra
 * Recebe: o número do carro.
 * Devolve: o lado do quadrado da sombra, em metros.
 */
float ladoDaSombra(int numero) {
    return ladosDaSombra[numero];
}

/*
 * prepararSombra
 * Fotografa o carro de cima, só o contorno em preto, e desfoca. A foto tem
 * uma folga em volta, para o desfoque caber.
 * Recebe: o número do carro.
 * Devolve: nada.
 */
static void prepararSombra(int numero) {
    float lado = fichaDoCarro(numero)->comprimento * FOLGA_DA_SOMBRA;
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
void desenharCarro3D(int numero, Matrix noMundo, float anguloDasRodas, float volante) {
    rlDrawRenderBatchActive();   /* o que já estava na fila sai antes de mudar a profundidade */

    Model modelo = modelos[numero];
    modelo.transform = MatrixMultiply(ajuste[numero], noMundo);

    desenharComShader(modelo, shaderOpaco);
    desenharRodas(numero, modelo, noMundo, anguloDasRodas, volante);

    BeginBlendMode(BLEND_ALPHA);
    rlDisableDepthMask();
    desenharComShader(modelo, shaderTranslucido);
    desenharRodas(numero, modelo, noMundo, anguloDasRodas, volante);
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
    EndBlendMode();
    rodaEmTroca = -1;
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
    return fichaDoCarro(numero)->comprimento;
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
    float escala = fichaDoCarro(numero)->comprimento / (deitado ? dx : dz);

    Matrix centralizar = MatrixTranslate(-(caixa.min.x + caixa.max.x) / 2.0f, -caixa.min.y,
                                         -(caixa.min.z + caixa.max.z) / 2.0f);
    Matrix girar = deitado ? MatrixRotateY(-90.0f * DEG2RAD) : MatrixIdentity();
    ajuste[numero] = MatrixMultiply(MatrixMultiply(centralizar, girar), MatrixScale(escala, escala, escala));
    larguras[numero] = (deitado ? dz : dx) * escala;
}

/*
 * verticeNormalizado
 * Recebe: a malha, o índice do vértice e o ajuste do carro.
 * Devolve: o vértice no carro normalizado (rodas em y = 0, frente em +z).
 */
static Vector3 verticeNormalizado(Mesh malha, int vertice, Matrix ajusteDoCarro) {
    return Vector3Transform((Vector3){ malha.vertices[vertice * 3], malha.vertices[vertice * 3 + 1], malha.vertices[vertice * 3 + 2] }, ajusteDoCarro);
}

/*
 * verticeDoTriangulo
 * Recebe: a malha, o triângulo e qual dos 3 cantos.
 * Devolve: o índice do vértice (a malha pode ter índices ou não).
 */
static int verticeDoTriangulo(Mesh malha, int triangulo, int canto) {
    return (malha.indices != NULL) ? malha.indices[triangulo * 3 + canto] : triangulo * 3 + canto;
}

/*
 * acharRaiz
 * A busca do conjunto-união: anda até o representante do grupo.
 * Recebe: o vetor de pais e o elemento.
 * Devolve: o representante.
 */
static int acharRaiz(int *pai, int a) {
    while (pai[a] != a) {
        pai[a] = pai[pai[a]];
        a = pai[a];
    }
    return a;
}

/* Um vértice para ordenar por posição e achar os repetidos */
typedef struct {
    float x, y, z;
    int indice;
} VerticeOrdenado;

static int compararVertices(const void *a, const void *b) {
    const VerticeOrdenado *p = a, *q = b;
    if (p->x != q->x) {
        return (p->x < q->x) ? -1 : 1;
    }
    if (p->y != q->y) {
        return (p->y < q->y) ? -1 : 1;
    }
    if (p->z != q->z) {
        return (p->z < q->z) ? -1 : 1;
    }
    return 0;
}

/*
 * marcarIlhas
 * Separa os triângulos de uma malha em pedaços contínuos: dois triângulos estão
 * no mesmo pedaço se dividem um vértice, ou um vértice no mesmo lugar (as texturas
 * duplicam os vértices nas emendas).
 * Recebe: a malha e o vetor a preencher com o grupo de cada vértice.
 * Devolve: nada.
 */
static void marcarIlhas(Mesh malha, int *pai) {
    for (int i = 0; i < malha.vertexCount; i++) {
        pai[i] = i;
    }
    VerticeOrdenado *ordem = MemAlloc(malha.vertexCount * sizeof(VerticeOrdenado));
    for (int i = 0; i < malha.vertexCount; i++) {
        ordem[i] = (VerticeOrdenado){ malha.vertices[i * 3], malha.vertices[i * 3 + 1], malha.vertices[i * 3 + 2], i };
    }
    qsort(ordem, malha.vertexCount, sizeof(VerticeOrdenado), compararVertices);
    for (int i = 1; i < malha.vertexCount; i++) {
        if (compararVertices(&ordem[i - 1], &ordem[i]) == 0) {
            pai[acharRaiz(pai, ordem[i].indice)] = acharRaiz(pai, ordem[i - 1].indice);
        }
    }
    MemFree(ordem);
    for (int t = 0; t < malha.triangleCount; t++) {
        int a = acharRaiz(pai, verticeDoTriangulo(malha, t, 0));
        pai[acharRaiz(pai, verticeDoTriangulo(malha, t, 1))] = a;
        pai[acharRaiz(pai, verticeDoTriangulo(malha, t, 2))] = a;
    }
}

/*
 * acharRodas
 * Acha as rodas de um carro: pedaços redondos encostados no chão. Os pedaços
 * desse tipo no mesmo lugar (o pneu de dentro e o de fora) contam como uma roda só.
 * Recebe: o número do carro e as medidas dele.
 * Devolve: nada (preenche rodas e totalDeRodas).
 */
static void acharRodas(int numero, float largura, float altura) {
    Model modelo = modelos[numero];
    totalDeRodas[numero] = 0;
    for (int m = 0; m < modelo.meshCount; m++) {
        Mesh malha = modelo.meshes[m];
        int *pai = MemAlloc(malha.vertexCount * sizeof(int));
        Ilha *ilhas = MemAlloc(malha.vertexCount * sizeof(Ilha));
        marcarIlhas(malha, pai);
        for (int i = 0; i < malha.vertexCount; i++) {
            ilhas[i].triangulos = 0;
        }
        for (int t = 0; t < malha.triangleCount; t++) {
            int raiz = acharRaiz(pai, verticeDoTriangulo(malha, t, 0));
            for (int c = 0; c < 3; c++) {
                Vector3 v = verticeNormalizado(malha, verticeDoTriangulo(malha, t, c), ajuste[numero]);
                if (ilhas[raiz].triangulos == 0 && c == 0) {
                    ilhas[raiz].minimo = v;
                    ilhas[raiz].maximo = v;
                }
                ilhas[raiz].minimo = Vector3Min(ilhas[raiz].minimo, v);
                ilhas[raiz].maximo = Vector3Max(ilhas[raiz].maximo, v);
            }
            ilhas[raiz].triangulos++;
        }
        for (int i = 0; i < malha.vertexCount; i++) {
            if (ilhas[i].triangulos == 0) {
                continue;
            }
            Vector3 medida = Vector3Subtract(ilhas[i].maximo, ilhas[i].minimo);
            if (ilhas[i].minimo.y > altura * PERTO_DO_CHAO || medida.y < altura * MENOR_RODA
                || fabsf(medida.z - medida.y) > medida.y * REDONDA || medida.x > largura * LARGURA_MAXIMA_RODA) {
                continue;
            }
            Vector3 centro = Vector3Scale(Vector3Add(ilhas[i].minimo, ilhas[i].maximo), 0.5f);
            int achou = -1;
            for (int r = 0; r < totalDeRodas[numero]; r++) {
                RodaDoCarro *roda = &rodas[numero][r];
                if (fabsf(roda->centro.z - centro.z) < roda->raio && fabsf(roda->centro.x - centro.x) < largura * MESMA_RODA_DE_LADO) {
                    achou = r;
                }
            }
            if (achou < 0 && totalDeRodas[numero] < MAXIMO_DE_RODAS) {
                achou = totalDeRodas[numero]++;
                rodas[numero][achou].raio = 0.0f;
                rodas[numero][achou].xMinimo = ilhas[i].minimo.x;
                rodas[numero][achou].xMaximo = ilhas[i].maximo.x;
            }
            if (achou >= 0) {
                RodaDoCarro *roda = &rodas[numero][achou];
                roda->xMinimo = fminf(roda->xMinimo, ilhas[i].minimo.x);
                roda->xMaximo = fmaxf(roda->xMaximo, ilhas[i].maximo.x);
                if (medida.y / 2.0f > roda->raio) {   /* o eixo é o da peça maior: o pneu */
                    roda->raio = medida.y / 2.0f;
                    roda->centro = centro;
                }
            }
        }
        MemFree(pai);
        MemFree(ilhas);
    }

    /* Peças redondas bem menores que as rodas (escapamento, suporte) não são rodas */
    float maior = 0.0f;
    for (int r = 0; r < totalDeRodas[numero]; r++) {
        maior = fmaxf(maior, rodas[numero][r].raio);
    }
    int ficaram = 0;
    for (int r = 0; r < totalDeRodas[numero]; r++) {
        if (rodas[numero][r].raio >= maior * MENOR_QUE_A_MAIOR) {
            rodas[numero][ficaram++] = rodas[numero][r];
        }
    }
    totalDeRodas[numero] = ficaram;
}

/*
 * rodaDaIlha
 * Recebe: o carro, os limites de uma ilha e sua medida.
 * Devolve: a roda onde a ilha cabe inteira, ou -1 se ela é do resto do carro.
 */
static int rodaDaIlha(int numero, Ilha *ilha) {
    for (int r = 0; r < totalDeRodas[numero]; r++) {
        RodaDoCarro *roda = &rodas[numero][r];
        float limite = roda->raio * FOLGA_DENTRO_DA_RODA;
        float folgaDeLado = (roda->xMaximo - roda->xMinimo) * FOLGA_DE_LADO_DA_RODA;
        if (fabsf(ilha->minimo.y - roda->centro.y) < limite && fabsf(ilha->maximo.y - roda->centro.y) < limite
            && fabsf(ilha->minimo.z - roda->centro.z) < limite && fabsf(ilha->maximo.z - roda->centro.z) < limite
            && ilha->minimo.x > roda->xMinimo - folgaDeLado && ilha->maximo.x < roda->xMaximo + folgaDeLado) {
            return r;
        }
    }
    return -1;
}

/*
 * copiarTriangulo
 * Acrescenta um triângulo a uma malha em construção, sem índices.
 * Recebe: o destino (com espaço), a origem, o triângulo, a matriz dos vértices (a das
 *         normais é a mesma, sem deslocamento) e quantos triângulos o destino já tem.
 * Devolve: nada.
 */
static void copiarTriangulo(Mesh *destino, Mesh origem, int triangulo, Matrix lugar, int jaTem) {
    Matrix giro = lugar;
    giro.m12 = giro.m13 = giro.m14 = 0.0f;
    for (int c = 0; c < 3; c++) {
        int v = verticeDoTriangulo(origem, triangulo, c);
        int d = jaTem * 3 + c;
        Vector3 p = Vector3Transform((Vector3){ origem.vertices[v * 3], origem.vertices[v * 3 + 1], origem.vertices[v * 3 + 2] }, lugar);
        destino->vertices[d * 3] = p.x;
        destino->vertices[d * 3 + 1] = p.y;
        destino->vertices[d * 3 + 2] = p.z;
        destino->texcoords[d * 2] = (origem.texcoords != NULL) ? origem.texcoords[v * 2] : 0.0f;
        destino->texcoords[d * 2 + 1] = (origem.texcoords != NULL) ? origem.texcoords[v * 2 + 1] : 0.0f;
        Vector3 n = { 0.0f, 1.0f, 0.0f };
        if (origem.normals != NULL) {
            n = Vector3Normalize(Vector3Transform((Vector3){ origem.normals[v * 3], origem.normals[v * 3 + 1], origem.normals[v * 3 + 2] }, giro));
        }
        destino->normals[d * 3] = n.x;
        destino->normals[d * 3 + 1] = n.y;
        destino->normals[d * 3 + 2] = n.z;
    }
}

/*
 * malhaVazia
 * Recebe: quantos triângulos ela vai ter.
 * Devolve: uma malha sem índices, com espaço para eles.
 */
static Mesh malhaVazia(int triangulos) {
    Mesh malha = { 0 };
    malha.triangleCount = triangulos;
    malha.vertexCount = triangulos * 3;
    malha.vertices = MemAlloc(malha.vertexCount * 3 * sizeof(float));
    malha.texcoords = MemAlloc(malha.vertexCount * 2 * sizeof(float));
    malha.normals = MemAlloc(malha.vertexCount * 3 * sizeof(float));
    return malha;
}

/*
 * separarRodas
 * Tira as rodas das malhas do modelo: cada malha fica só com o resto do carro, e
 * as peças de cada roda viram malhas à parte, com o centro da roda na origem.
 * Recebe: o número do carro.
 * Devolve: nada (troca as malhas do modelo e preenche pecas).
 */
static void separarRodas(int numero) {
    Model *modelo = &modelos[numero];
    totalDePecas[numero] = 0;
    for (int m = 0; m < modelo->meshCount; m++) {
        Mesh malha = modelo->meshes[m];
        int *pai = MemAlloc(malha.vertexCount * sizeof(int));
        Ilha *ilhas = MemAlloc(malha.vertexCount * sizeof(Ilha));
        marcarIlhas(malha, pai);
        for (int i = 0; i < malha.vertexCount; i++) {
            ilhas[i].triangulos = 0;
        }
        for (int t = 0; t < malha.triangleCount; t++) {
            int raiz = acharRaiz(pai, verticeDoTriangulo(malha, t, 0));
            for (int c = 0; c < 3; c++) {
                Vector3 v = verticeNormalizado(malha, verticeDoTriangulo(malha, t, c), ajuste[numero]);
                if (ilhas[raiz].triangulos == 0 && c == 0) {
                    ilhas[raiz].minimo = v;
                    ilhas[raiz].maximo = v;
                }
                ilhas[raiz].minimo = Vector3Min(ilhas[raiz].minimo, v);
                ilhas[raiz].maximo = Vector3Max(ilhas[raiz].maximo, v);
            }
            ilhas[raiz].triangulos++;
        }

        /* Conta quantos triângulos vão para cada lugar */
        int *rodaDoTriangulo = MemAlloc(malha.triangleCount * sizeof(int));
        int noCorpo = 0, naRoda[MAXIMO_DE_RODAS] = { 0 };
        for (int t = 0; t < malha.triangleCount; t++) {
            rodaDoTriangulo[t] = rodaDaIlha(numero, &ilhas[acharRaiz(pai, verticeDoTriangulo(malha, t, 0))]);
            if (rodaDoTriangulo[t] < 0) {
                noCorpo++;
            } else {
                naRoda[rodaDoTriangulo[t]]++;
            }
        }

        /* As peças de cada roda, com o centro da roda na origem */
        for (int r = 0; r < totalDeRodas[numero]; r++) {
            if (naRoda[r] == 0) {
                continue;
            }
            Vector3 c = rodas[numero][r].centro;
            Matrix lugar = MatrixMultiply(ajuste[numero], MatrixTranslate(-c.x, -c.y, -c.z));
            Mesh peca = malhaVazia(naRoda[r]);
            int feitos = 0;
            for (int t = 0; t < malha.triangleCount; t++) {
                if (rodaDoTriangulo[t] == r) {
                    copiarTriangulo(&peca, malha, t, lugar, feitos++);
                }
            }
            UploadMesh(&peca, false);
            pecas[numero][totalDePecas[numero]++] = (PecaDeRoda){ peca, modelo->meshMaterial[m], r };
        }

        /* O resto do carro, no lugar da malha original (continua nas medidas do modelo) */
        Mesh corpo = malhaVazia(noCorpo);
        int feitos = 0;
        for (int t = 0; t < malha.triangleCount; t++) {
            if (rodaDoTriangulo[t] < 0) {
                copiarTriangulo(&corpo, malha, t, MatrixIdentity(), feitos++);
            }
        }
        UploadMesh(&corpo, false);
        UnloadMesh(malha);
        modelo->meshes[m] = corpo;

        MemFree(rodaDoTriangulo);
        MemFree(pai);
        MemFree(ilhas);
    }
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
    float tamanho = fichaDoCarro(numero)->comprimento;
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
    desenharSombra(numero, MatrixIdentity());
    desenharCarro3D(numero, MatrixIdentity(), 0.0f, 0.0f);
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
        BoundingBox caixa = GetModelBoundingBox(modelos[numero]);
        float escala = fichaDoCarro(numero)->comprimento / fmaxf(caixa.max.x - caixa.min.x, caixa.max.z - caixa.min.z);
        acharRodas(numero, fichaDoCarro(numero)->largura, (caixa.max.y - caixa.min.y) * escala);
        separarRodas(numero);
        TraceLog(LOG_INFO, "CARRO: %s tem %d rodas, em %d pecas", fichaDoCarro(numero)->nome, totalDeRodas[numero], totalDePecas[numero]);
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
        for (int i = 0; i < totalDePecas[numero]; i++) {
            UnloadMesh(pecas[numero][i].malha);
        }
        UnloadRenderTexture(miniaturas[numero]);
        UnloadTexture(sombras[numero]);
    }
    UnloadShader(shaderOpaco);
    UnloadShader(shaderTranslucido);
    UnloadShader(shaderDaSombra);
}
