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
#include <stdio.h>
#include <string.h>
#include "carros3d.h"
#include "raymath.h"
#include "rlgl.h"
#include "corrida.h"
#include "noite.h"

#define PASTA_DOS_MODELOS     "recursos/modelos/carros/"
#define LADO_DA_MINIATURA     512
#define GRAUS_DA_FOTO         20.0f     /* de longe, para o carro não ficar deformado */
#define DIRECAO_DA_FOTO       ((Vector3){ 0.95f, 0.42f, 1.15f })   /* de frente e um pouco de lado */
#define LARGURA_NA_FOTO       0.86f     /* fração da foto que o carro pode ocupar: sobra a margem */
#define ALTURA_NA_FOTO        0.68f     /* e na altura */
#define CHAO_NA_FOTO          (-0.62f)  /* a altura do chão na foto, de -1 (embaixo) a +1 */
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

/* Luz de um sol alto mais o céu, para os carros terem volume. Os shaders
 * só mudam no corte (um desenha só o opaco, o outro só o translúcido) e na
 * leitura da cor (a dos olhos junta três texturas, veja OLHOS_DO_SHADER). */
#define PIXELS_DO_SHADER_LENDO(DECLARACOES, LEITURA, CORTE, SAIDA) \
    "#version 330\n" \
    "in vec2 fragTexCoord; in vec4 fragColor; in vec3 fragNormal; in vec3 fragPos;\n" \
    "uniform sampler2D texture0; uniform vec4 colDiffuse; uniform vec3 olho; uniform float reflexoDaPintura;\n" \
    DECLARACOES \
    "out vec4 finalColor;\n" \
    GLSL_DA_LUZ_DA_NOITE \
    "void main() { vec4 cor = " LEITURA " * colDiffuse * fragColor;\n" \
    "              " CORTE "\n" \
    "              vec3 n = normalize(fragNormal);\n" \
    "              if (!gl_FrontFacing) n = -n;\n" \
    "              float sol = max(dot(n, normalize(vec3(0.35, 0.8, 0.45))), 0.0);\n" \
    "              float luz = 0.62 + 0.28 * (n.y * 0.5 + 0.5) + 0.42 * sol;\n" \
    "              vec3 comLuz = (noite > 0.5) ? luzDaNoite(fragPos, n) * 1.1 : vec3(luz);\n" \
    "              vec3 v = normalize(olho - fragPos);\n" \
    "              vec3 r = reflect(-v, n);\n" \
    "              float fresnel = pow(1.0 - max(dot(n, v), 0.0), 4.0);\n" \
    "              float lataria = smoothstep(0.12, 0.38, max(cor.r, max(cor.g, cor.b)));\n" \
    "              vec3 ceu = (noite > 0.5) ? mix(vec3(0.03, 0.04, 0.08), vec3(0.20, 0.26, 0.42), clamp(r.y, 0.0, 1.0))\n" \
    "                                       : mix(vec3(0.50, 0.52, 0.52), vec3(0.62, 0.80, 1.00), clamp(r.y * 1.5, 0.0, 1.0));\n" \
    "              float reflexo = lataria * (0.08 + 0.50 * fresnel) * reflexoDaPintura;\n" \
    "              float estrela = (noite > 0.5) ? 0.0 : pow(max(dot(r, normalize(vec3(0.35, 0.8, 0.45))), 0.0), 60.0) * lataria;\n" \
    "              vec3 base = mix(cor.rgb * comLuz, ceu, reflexo) + vec3(1.0, 0.97, 0.9) * estrela * 0.9;\n" \
    "              finalColor = vec4(base, " SAIDA "); }\n"
#define PIXELS_DO_SHADER(CORTE, SAIDA) \
    PIXELS_DO_SHADER_LENDO("", "texture(texture0, fragTexCoord)", CORTE, SAIDA)

/* ---------- Olhos ----------
 * Os olhos de cada carro são uma textura (o para-brisa) com o branco, as íris e a pálpebra.
 * Ela já vem separada em três (recursos/modelos/carros/olhos/, feitas a partir dela): o
 * fundo sem as íris (texture0), só as íris (texture1) e onde é branco do olho (texture2).
 * O shader desliza as íris pelo "desvio" e só as mostra onde é branco: assim elas olham
 * para o lado da curva e somem por baixo da pálpebra. Na sombra, a íris escurece junto. */
#define PASTA_DOS_OLHOS       PASTA_DOS_MODELOS "olhos/"
#define CURVA_DO_OLHAR        0.55f     /* < 1: um pouco de volante já vira bem os olhos */
#define SOBE_O_OLHAR          0.45f     /* quanto a íris sobe ou desce, em raios dela */
/* A cara em cada estado: o fechamento da pálpebra (0 normal, 1 fechada, < 0 arregalada),
 * a inclinação (> 0 o canto de dentro desce: determinado; < 0 sobe: preocupado), o arco
 * (> 0 o meio abre mais que os cantos) e o tamanho da íris */
#define CACA_FECHA            0.35f     /* "modo caça", em alta velocidade */
#define CACA_INCLINA          0.35f
#define CACA_IRIS             1.04f
#define SUSTO_FECHA          -0.40f     /* rodando, ou depois de uma batida forte */
#define SUSTO_INCLINA        -0.30f
#define SUSTO_ARCO            0.35f
#define SUSTO_IRIS            0.78f
#define SUSTO_OLHA_PARA_CIMA  0.35f
#define SUSTO_TREME           0.10f     /* a íris treme de lado (em frações do quanto ela anda) */
#define SUSTO_PASSA           0.80f     /* segundos para o susto passar */
/* As molas: cada parte da cara vai até o alvo como uma mola (frequência em Hz e
 * amortecimento: abaixo de 1, passa um pouco do ponto e volta). É isso que dá vida: o olho
 * chega no canto, passa um tico e assenta, em vez de deslizar em linha reta. */
#define MOLA_DO_OLHAR         3.2f, 0.55f
#define MOLA_DA_PALPEBRA      2.2f, 0.75f
#define MOLA_DA_INCLINACAO    2.0f, 0.80f
#define MOLA_DO_ARCO          2.0f, 0.60f
#define MOLA_DA_IRIS          4.0f, 0.50f
#define PASSO_DAS_MOLAS       (1.0f / 240.0f)
/* As piscadas: de tempos em tempos (às vezes duas seguidas), e uma no começo do susto */
#define PISCA_DE_TEMPOS       2.2f      /* segundos, no mínimo... */
#define PISCA_ATE             5.5f      /* ...e no máximo */
#define DURACAO_DA_PISCADA    0.17f
#define PISCADA_DUPLA         0.20f     /* a chance de piscar duas vezes */
/* Olhadelas: com o volante parado, o olhar não fica congelado; muda um pouco de vez em quando */
#define OLHADELA_DE_TEMPOS    0.5f
#define OLHADELA_ATE          1.8f
#define OLHADELA_DE_LADO      0.22f
#define OLHADELA_DE_ALTURA    0.15f

/* O shader dos olhos.
 * A íris: desliza pelo "desvio", escala em volta do meio dela e só aparece onde é branco
 *   do olho (a máscara); na sombra, escurece junto.
 * A pálpebra: cada coluna da textura sabe onde fica a borda da pálpebra e onde termina o
 *   branco do outro lado (texture3, medido ao carregar). Cada coluna fecha um tanto (a forma:
 *   fechamento, inclinação e arco, por olho): o desenho da pálpebra (com a borda e a sombra)
 *   anda inteiro até a nova borda, e o branco que sobra se espreme até o outro lado, que fica
 *   parado (o cílio da Sally não sai do lugar). As leituras usam as derivadas da coordenada
 *   original, senão o mipmap pisca na emenda. */
#define OLHOS_DO_SHADER \
    "uniform sampler2D texture1; uniform sampler2D texture2; uniform sampler2D texture3;\n" \
    "uniform vec2 desvio; uniform vec4 meiosDasIris; uniform float tamanhoDaIris; uniform vec4 formaDaPalpebra;\n" \
    "float fechamentoDoOlho(float u, float meio, float outro) {\n" \
    "    float x = clamp((u - meio) / max(abs(outro - meio) * 0.5, 0.02) * sign(outro - meio), -1.3, 1.3);\n" \
    "    return formaDaPalpebra.x + 0.5 * formaDaPalpebra.y * x - 0.45 * formaDaPalpebra.z * (1.0 - x * x);\n" \
    "}\n" \
    "vec2 comPalpebra(vec2 uv) {\n" \
    "    vec3 borda = texture(texture3, vec2(uv.x, 0.5)).rgb;\n" \
    "    float d = borda.x - borda.y;\n" \
    "    if (borda.z < 0.5 || abs(d) < 0.002) return uv;\n" \
    "    float c1 = meiosDasIris.x, c2 = meiosDasIris.z;\n" \
    "    float t = smoothstep(-0.15, 0.15, (uv.x - 0.5 * (c1 + c2)) / max(abs(c2 - c1), 0.02) * sign(c2 - c1));\n" \
    "    float f = mix(fechamentoDoOlho(uv.x, c1, c2), fechamentoDoOlho(uv.x, c2, c1), t);\n" \
    "    float s = (uv.y - borda.y) / d;\n" \
    "    float e = 1.0 - f;\n" \
    "    float v = uv.y;\n" \
    "    if (s >= e) {\n" \
    "        /* a pálpebra anda inteira; o que passar do fim da textura repete o fundo dela */\n" \
    "        float fundoDaPalpebra = (d > 0.0) ? 0.97 - borda.x : borda.x - 0.03;\n" \
    "        float dentro = uv.y - (borda.y + e * d);\n" \
    "        v = borda.x + sign(dentro) * min(abs(dentro), max(fundoDaPalpebra, 0.0));\n" \
    "    }\n" \
    "    else if (s >= 0.0) v = borda.y + s / max(e, 0.001) * d;\n" \
    "    return vec2(uv.x, clamp(v, 0.002, 0.998));\n" \
    "}\n" \
    "vec4 corDosOlhos(vec2 uv) {\n" \
    "    vec2 dx = dFdx(uv), dy = dFdy(uv);\n" \
    "    /* a textura se repete: há modelo com a coordenada fora de 0 a 1 (a do Wingo vai de -1 a 0) */\n" \
    "    uv = fract(uv);\n" \
    "    vec2 p = comPalpebra(uv);\n" \
    "    vec4 fundo = textureGrad(texture0, p, dx, dy);\n" \
    "    vec2 q = uv - desvio;\n" \
    "    bool primeira = distance(q, meiosDasIris.xy) < distance(q, meiosDasIris.zw);\n" \
    "    vec2 meio = primeira ? meiosDasIris.xy : meiosDasIris.zw;\n" \
    "    vec2 outro = primeira ? meiosDasIris.zw : meiosDasIris.xy;\n" \
    "    vec2 amostra = meio + (q - meio) / tamanhoDaIris;\n" \
    "    vec4 iris = textureGrad(texture1, amostra, dx / tamanhoDaIris, dy / tamanhoDaIris);\n" \
    "    if (distance(amostra, outro) < distance(amostra, meio)) iris.a = 0.0; /* só a deste olho */\n" \
    "    float a = iris.a * textureGrad(texture2, p, dx, dy).r;\n" \
    "    float sombra = clamp(max(fundo.r, max(fundo.g, fundo.b)) / 0.97, 0.0, 1.0);\n" \
    "    return vec4(mix(fundo.rgb, iris.rgb * sombra, a), 1.0);\n" \
    "}\n"

typedef struct {
    int tem;           /* 1 se o carro tem os olhos preparados */
    int material;      /* o material dos olhos, no modelo da raylib */
    float anda;        /* quanto a íris anda de lado com o volante todo (fração da textura) */
    float lado;        /* +1 ou -1: para que lado da textura (u) fica a direita do carro */
    float ladoDeCima;  /* +1 ou -1: para que lado da textura (v) fica o alto do carro */
    float ladoDaPalpebra;   /* +1 embaixo do olho na textura, -1 em cima, 0 sem pálpebra */
    float linha;            /* a altura do meio do olho, na textura */
    float raio;             /* o raio da íris na altura, na textura */
    Vector4 meios;          /* o meio de cada íris (u, v, u, v) */
} OlhosDoCarro;

/* Uma mola: a posição e a velocidade */
typedef struct {
    float x, v;
} Mola;

/* A cara de um carro, viva: as molas, as piscadas e as olhadelas */
typedef struct {
    Mola olharX, olharY, fechar, inclinar, arco, iris;
    float alvoAperto, alvoSusto;   /* o pedido para o próximo desenho */
    float susto;                   /* o susto de agora (passa devagar) */
    float piscada;                 /* segundos desde o começo da piscada (< 0: sem piscada) */
    float proximaPiscada;          /* segundos até a próxima */
    float proximaOlhadela;         /* segundos até mudar a olhadela */
    float olhadelaX, olhadelaY;
    unsigned int sorteio;          /* para os sorteios do carro (não mexe no rand() do jogo) */
    double hora;                   /* do último desenho (para saber quanto tempo passou) */
} CaraDoCarro;

static OlhosDoCarro olhos[TOTAL_CARROS];
static CaraDoCarro caras[TOTAL_CARROS];
static float reflexos[TOTAL_CARROS];    /* quanto o céu se reflete na pintura de cada carro */
static Shader shaderDosOlhos;
static int locDoDesvio, locDosMeios, locDoTamanho, locDaForma;
static int locsDoReflexo[3];   /* reflexoDaPintura no opaco, no translúcido e no dos olhos */
static int locsDoOlho[3];      /* "olho" nos mesmos três: procurar pelo nome a cada quadro é caro */

/* ---------- Cores ----------
 * Um acerto na pintura de um carro, ao carregar: a cor (só nos pixels perto do tom
 * escolhido) e quanto ela reflete o céu. Numa cor clara, o reflexo azulado apaga a cor: o
 * Luigi saía quase cinza na tela (o capô, azulado), e no filme ele é amarelo-manteiga. */
typedef struct {
    const char *arquivo;
    int materiais[4];            /* os materiais da pintura, no .glb (-1 sobra) */
    float tomDe, largura;        /* graus: o tom a mexer e até quantos graus dele */
    float girar;                 /* graus a somar no tom */
    float saturacao, brilho;     /* multiplicam */
    float reflexo;               /* quanto o céu se reflete na pintura (1 = como nos outros carros) */
} AcertoDeCor;

static const AcertoDeCor ACERTOS_DE_COR[] = {
    { "luigi.glb", { 2, 3, -1, -1 }, 49.0f, 14.0f, -4.0f, 1.05f, 1.22f, 0.35f },
};

static Model modelos[TOTAL_CARROS];
static Matrix ajuste[TOTAL_CARROS];     /* do modelo como veio para o modelo normalizado */
static float larguras[TOTAL_CARROS];    /* largura de verdade, em metros */
static RenderTexture2D miniaturas[TOTAL_CARROS];
static BoundingBox caixasDosCarros[TOTAL_CARROS];   /* o volume de verdade, já com o ajuste do carro */
static Shader shaderOpaco, shaderTranslucido, shaderDaSombra;
static Texture2D sombras[TOTAL_CARROS];
static RodaDoCarro rodas[TOTAL_CARROS][MAXIMO_DE_RODAS];
static int totalDeRodas[TOTAL_CARROS];
static PecaDeRoda pecas[TOTAL_CARROS][MAXIMO_DE_PECAS_RODA * MAXIMO_DE_RODAS];
static int totalDePecas[TOTAL_CARROS];
/* Os garfos do Guido (os doGuido dele) vêm num modelo à parte, nos mesmos eixos: assim sobem e inclinam */
#define ARQUIVO_DOS_GARFOS    "guido_garfos.glb"
#define CARRO_COM_GARFOS      7          /* o Guido, na ordem de FICHAS */
static Model garfos;
static int temGarfos = 0;
static Matrix poseDosGarfos;             /* no carro normalizado; vale só para o próximo desenho */
static int temPoseDosGarfos = 0;
static Vector3 pontaDosGarfosNormal;     /* o meio das pás, em cima, no carro normalizado */
static int rodaEscondida = -1;           /* a roda que não está no carro no próximo desenho */
static Matrix corpoInclinado;            /* a suspensão: só o corpo inclina, as rodas não */
static int temCorpoInclinado = 0;

static unsigned char *translucidos[TOTAL_CARROS];   /* por material: 1 se tem algo translúcido */
static unsigned char *translucidosDosGarfos = NULL;
static int blocoDeSombras = 0;   /* 1: entre comecarSombrasNoChao e terminarSombrasNoChao */
static int rodaEmTroca = -1;          /* a roda que está sendo trocada no próximo carro desenhado (-1 = nenhuma) */
static float andamentoDaTroca = 0.0f;
static float ladosDaSombra[TOTAL_CARROS];

/*
 * desenharComShader
 * Recebe: o modelo (já com a matriz certa), o shader da passada e o material dos olhos, que
 *         usa o shader dos olhos (-1 = nenhum).
 * Devolve: nada.
 */
static void desenharComShader(Model modelo, Shader shader, int materialDosOlhos) {
    for (int i = 0; i < modelo.materialCount; i++) {
        modelo.materials[i].shader = (i == materialDosOlhos) ? shaderDosOlhos : shader;
    }
    DrawModel(modelo, (Vector3){ 0.0f, 0.0f, 0.0f }, 1.0f, WHITE);
}

/*
 * desenharSoTranslucidos
 * A segunda passada: só as peças cujo material tem algo translúcido.
 * Recebe: o modelo, o shader e os materiais translúcidos.
 * Devolve: nada.
 */
static void desenharSoTranslucidos(Model modelo, Shader shader, const unsigned char *translucido) {
    for (int i = 0; i < modelo.meshCount; i++) {
        int m = modelo.meshMaterial[i];
        if (translucido == NULL || translucido[m]) {
            Material material = modelo.materials[m];
            material.shader = shader;
            DrawMesh(modelo.meshes[i], material, modelo.transform);
        }
    }
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
static void desenharRodas(int numero, Model modelo, Matrix noMundo, float anguloDasRodas, float volante, int soTranslucidas) {
    for (int i = 0; i < totalDePecas[numero]; i++) {
        PecaDeRoda *peca = &pecas[numero][i];
        if (soTranslucidas && translucidos[numero] != NULL && !translucidos[numero][peca->material]) {
            continue;
        }
        RodaDoCarro *roda = &rodas[numero][peca->roda];
        if (peca->roda == rodaEmTroca) {
            desenharPecaTrocando(peca, roda, modelo, noMundo, anguloDasRodas);
            continue;
        }
        if (peca->roda == rodaEscondida) {
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
    if (!blocoDeSombras) {
        BeginBlendMode(BLEND_ALPHA);
        rlDisableDepthMask();
    }
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
    if (!blocoDeSombras) {
        rlDrawRenderBatchActive();
        rlEnableDepthMask();
        EndBlendMode();
    }
}

/*
 * comecarSombrasNoChao / terminarSombrasNoChao
 * As sombras de todos os carros vão juntas, num só bloco: sem esvaziar a fila a cada carro.
 */
void comecarSombrasNoChao(void) {
    rlDrawRenderBatchActive();
    BeginBlendMode(BLEND_ALPHA);
    rlDisableDepthMask();
    blocoDeSombras = 1;
}

void terminarSombrasNoChao(void) {
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
    EndBlendMode();
    blocoDeSombras = 0;
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
    desenharComShader(modelo, shaderDaSombra, -1);
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
 * alturaDoCarro3D
 * Recebe: o número do carro.
 * Devolve: a altura do teto, em metros.
 */
float alturaDoCarro3D(int numero) {
    return caixasDosCarros[numero].max.y;
}

/*
 * definirOlhoDosCarros
 * Onde está a câmera, para o reflexo na pintura.
 * Recebe: a posição da câmera.
 * Devolve: nada.
 */
void definirOlhoDosCarros(Vector3 olho) {
    SetShaderValue(shaderOpaco, locsDoOlho[0], &olho, SHADER_UNIFORM_VEC3);
    SetShaderValue(shaderTranslucido, locsDoOlho[1], &olho, SHADER_UNIFORM_VEC3);
    SetShaderValue(shaderDosOlhos, locsDoOlho[2], &olho, SHADER_UNIFORM_VEC3);
}

/*
 * esconderRodaNoProximoDesenho
 * Recebe: a roda que está fora do carro (na troca), no próximo desenho.
 * Devolve: nada.
 */
void esconderRodaNoProximoDesenho(int roda) {
    rodaEscondida = roda;
}

/*
 * inclinarCorpoNoProximoDesenho
 * A suspensão: o corpo do carro gira em volta de um ponto na altura dos eixos, e as rodas
 * ficam no chão.
 * Recebe: a rolagem (radianos, positivo: o teto vai para a esquerda do carro) e a arfagem
 *         (radianos, positivo: o bico desce).
 * Devolve: nada.
 */
void inclinarCorpoNoProximoDesenho(int numero, float rolagem, float arfagem) {
    float eixo = (totalDeRodas[numero] > 0) ? rodas[numero][0].centro.y : 0.3f;
    corpoInclinado = MatrixMultiply(MatrixMultiply(MatrixMultiply(MatrixTranslate(0.0f, -eixo, 0.0f), MatrixRotateX(arfagem)),
                                                   MatrixRotateZ(-rolagem)),
                                    MatrixTranslate(0.0f, eixo, 0.0f));
    temCorpoInclinado = 1;
}

/*
 * moverGarfosNoProximoDesenho
 * Os doGuido do Guido: os garfos sobem e inclinam para trás, girando em volta da base deles.
 * Recebe: quanto sobem (metros) e quanto inclinam para trás (radianos).
 * Devolve: nada.
 */
void moverGarfosNoProximoDesenho(float subida, float inclinacao) {
    Vector3 base = { 0.0f, pontaDosGarfosNormal.y, pontaDosGarfosNormal.z - 0.35f };
    poseDosGarfos = MatrixMultiply(MatrixMultiply(MatrixMultiply(MatrixTranslate(0.0f, -base.y, -base.z), MatrixRotateX(-inclinacao)),
                                                  MatrixTranslate(0.0f, base.y, base.z)),
                                   MatrixTranslate(0.0f, subida, 0.0f));
    temPoseDosGarfos = 1;
}

/*
 * pontaDosGarfos
 * Recebe: nada.
 * Devolve: o meio das pás dos garfos do Guido, em cima, no carro normalizado, parados.
 */
Vector3 pontaDosGarfos(void) {
    return pontaDosGarfosNormal;
}

/*
 * raioDaRoda
 * Recebe: o carro e a roda.
 * Devolve: o raio dela, em metros.
 */
float raioDaRoda(int numero, int roda) {
    return (roda >= 0 && roda < totalDeRodas[numero]) ? rodas[numero][roda].raio : 0.3f;
}

/*
 * desenharRodaSolta
 * Uma roda fora do carro (no garfo do Guido ou no chão).
 * Recebe: o carro, a roda e onde ela fica no mundo (a origem é o centro dela; o eixo, o x).
 * Devolve: nada.
 */
void desenharRodaSolta(int numero, int roda, Matrix noMundo) {
    Model modelo = modelos[numero];
    for (int i = 0; i < totalDePecas[numero]; i++) {
        PecaDeRoda *peca = &pecas[numero][i];
        if (peca->roda != roda) {
            continue;
        }
        Material material = modelo.materials[peca->material];
        material.shader = shaderOpaco;
        DrawMesh(peca->malha, material, noMundo);
    }
}

/*
 * expressaoNoProximoDesenho
 * A cara do carro no próximo desenho: o quanto ele aperta os olhos (o "modo caça", em alta
 * velocidade) e o quanto está assustado (rodando, batendo). A cara vai até lá com as molas.
 * Sem chamar, o carro volta à cara normal.
 * Recebe: o carro, o aperto e o susto (de 0 a 1).
 * Devolve: nada.
 */
void expressaoNoProximoDesenho(int numero, float aperto, float susto) {
    caras[numero].alvoAperto = fmaxf(0.0f, fminf(1.0f, aperto));
    caras[numero].alvoSusto = fmaxf(0.0f, fminf(1.0f, susto));
}

/*
 * sortear
 * Recebe: a cara (que guarda o sorteio dela), e o menor e o maior valor.
 * Devolve: um valor entre os dois.
 */
static float sortear(CaraDoCarro *cara, float menor, float maior) {
    cara->sorteio = cara->sorteio * 1664525u + 1013904223u;
    return menor + (maior - menor) * (float) (cara->sorteio >> 8) / 16777216.0f;
}

/*
 * molar
 * Leva a mola em direção ao alvo, em passos pequenos (assim não explode com um quadro lento).
 * Recebe: a mola, o alvo, a frequência (Hz), o amortecimento (1: chega sem passar do ponto)
 *         e o tempo que passou.
 * Devolve: nada.
 */
static void molar(Mola *mola, float alvo, float frequencia, float amortecimento, float tempo) {
    float w = 2.0f * PI * frequencia;
    for (float resta = tempo; resta > 0.0f; resta -= PASSO_DAS_MOLAS) {
        float h = fminf(resta, PASSO_DAS_MOLAS);
        mola->v += (w * w * (alvo - mola->x) - 2.0f * amortecimento * w * mola->v) * h;
        mola->x += mola->v * h;
    }
}

/*
 * fechamentoDaPiscada
 * Recebe: os segundos desde o começo da piscada.
 * Devolve: o quanto ela fecha o olho (0 a 1): fecha rápido e abre um pouco mais devagar.
 */
static float fechamentoDaPiscada(float t) {
    if (t < 0.0f || t > DURACAO_DA_PISCADA) {
        return 0.0f;
    }
    float fecha = DURACAO_DA_PISCADA * 0.4f;
    float k = (t < fecha) ? t / fecha : 1.0f - (t - fecha) / (DURACAO_DA_PISCADA - fecha);
    return k * k * (3.0f - 2.0f * k);
}

/*
 * animarOlhos
 * Um passo da cara do carro: os alvos de cada parte (o volante, o aperto, o susto, as
 * olhadelas), as molas até eles, e as piscadas.
 * Recebe: o carro, o volante (-1 a +1) e o tempo que passou.
 * Devolve: nada.
 */
static void animarOlhos(int numero, float volante, float tempo) {
    CaraDoCarro *c = &caras[numero];
    float comecouSusto = (c->alvoSusto > 0.5f && c->susto < 0.5f);
    c->susto = fmaxf(c->alvoSusto, c->susto - tempo / SUSTO_PASSA);
    float s = c->susto;
    float a = c->alvoAperto * (1.0f - s);
    c->alvoAperto = 0.0f;
    c->alvoSusto = 0.0f;

    /* As piscadas: de tempos em tempos, menos no "modo caça" (concentrado), e no susto */
    c->proximaPiscada -= tempo;
    if (c->piscada >= 0.0f) {
        c->piscada += tempo;
        if (c->piscada > DURACAO_DA_PISCADA) {
            c->piscada = -1.0f;
        }
    }
    if (comecouSusto || (c->proximaPiscada <= 0.0f && c->piscada < 0.0f)) {
        c->piscada = 0.0f;
        int dupla = sortear(c, 0.0f, 1.0f) < PISCADA_DUPLA;
        c->proximaPiscada = dupla ? DURACAO_DA_PISCADA + 0.08f
                                  : sortear(c, PISCA_DE_TEMPOS, PISCA_ATE) * (1.0f + 0.8f * a);
    }

    /* As olhadelas: com o volante parado, o olhar muda um pouco de vez em quando */
    c->proximaOlhadela -= tempo;
    if (c->proximaOlhadela <= 0.0f) {
        c->olhadelaX = sortear(c, -OLHADELA_DE_LADO, OLHADELA_DE_LADO);
        c->olhadelaY = sortear(c, -OLHADELA_DE_ALTURA, OLHADELA_DE_ALTURA);
        c->proximaOlhadela = sortear(c, OLHADELA_DE_TEMPOS, OLHADELA_ATE);
    }
    float v = fmaxf(-1.0f, fminf(1.0f, volante));
    float olhar = copysignf(powf(fabsf(v), CURVA_DO_OLHAR), v);
    float solto = (1.0f - fabsf(olhar)) * (1.0f - a * 0.7f);   /* concentrado, olha menos em volta */
    float tremor = s * SUSTO_TREME * sinf((float) c->hora * 47.0f + (float) numero * 1.7f);

    molar(&c->olharX, olhar * (1.0f - 0.3f * s) + c->olhadelaX * solto + tremor, MOLA_DO_OLHAR, tempo);
    molar(&c->olharY, SUSTO_OLHA_PARA_CIMA * s + c->olhadelaY * solto, MOLA_DO_OLHAR, tempo);
    molar(&c->fechar, CACA_FECHA * a + SUSTO_FECHA * s, MOLA_DA_PALPEBRA, tempo);
    molar(&c->inclinar, CACA_INCLINA * a + SUSTO_INCLINA * s, MOLA_DA_INCLINACAO, tempo);
    molar(&c->arco, SUSTO_ARCO * s, MOLA_DO_ARCO, tempo);
    molar(&c->iris, 1.0f + (CACA_IRIS - 1.0f) * a + (SUSTO_IRIS - 1.0f) * s, MOLA_DA_IRIS, tempo);
}

/*
 * prepararOlhos
 * Anda a cara do carro e passa ela para o shader dos olhos: para onde as íris olham, o
 * tamanho delas e a forma da pálpebra (com a piscada por cima).
 * Recebe: o carro e o volante (-1 a +1).
 * Devolve: nada.
 */
static void prepararOlhos(int numero, float volante) {
    OlhosDoCarro *o = &olhos[numero];
    CaraDoCarro *c = &caras[numero];
    double agora = GetTime();
    if (c->hora <= 0.0) {   /* a primeira vez: começa parado, com sorteios só deste carro */
        c->sorteio = 2654435761u * (unsigned int) (numero + 1);
        c->iris.x = 1.0f;
        c->piscada = -1.0f;
        c->proximaPiscada = sortear(c, 0.5f, PISCA_ATE);
    }
    float tempo = (c->hora > 0.0) ? (float) fmin(0.1, agora - c->hora) : 0.0f;
    c->hora = agora;
    animarOlhos(numero, volante, tempo);

    float piscada = fechamentoDaPiscada(c->piscada);
    Vector4 forma = { c->fechar.x + (1.08f - c->fechar.x) * piscada, c->inclinar.x * (1.0f - piscada),
                      c->arco.x * (1.0f - piscada), 0.0f };
    Vector2 desvio = { o->lado * o->anda * c->olharX.x, o->ladoDeCima * o->raio * SOBE_O_OLHAR * c->olharY.x };
    float tamanho = fmaxf(0.5f, c->iris.x);
    SetShaderValue(shaderDosOlhos, locDoDesvio, &desvio, SHADER_UNIFORM_VEC2);
    SetShaderValue(shaderDosOlhos, locDosMeios, &o->meios, SHADER_UNIFORM_VEC4);
    SetShaderValue(shaderDosOlhos, locDoTamanho, &tamanho, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shaderDosOlhos, locDaForma, &forma, SHADER_UNIFORM_VEC4);
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
    Matrix corpo = temCorpoInclinado ? MatrixMultiply(ajuste[numero], corpoInclinado) : ajuste[numero];
    modelo.transform = MatrixMultiply(corpo, noMundo);
    Model doGuido = garfos;
    int comGarfos = (numero == CARRO_COM_GARFOS && temGarfos);
    if (comGarfos) {
        Matrix pose = temPoseDosGarfos ? MatrixMultiply(ajuste[numero], poseDosGarfos) : ajuste[numero];
        doGuido.transform = MatrixMultiply(pose, noMundo);
    }

    int materialDosOlhos = -1;
    if (olhos[numero].tem) {
        prepararOlhos(numero, volante);
        materialDosOlhos = olhos[numero].material;
    }
    SetShaderValue(shaderOpaco, locsDoReflexo[0], &reflexos[numero], SHADER_UNIFORM_FLOAT);
    SetShaderValue(shaderDosOlhos, locsDoReflexo[2], &reflexos[numero], SHADER_UNIFORM_FLOAT);
    desenharComShader(modelo, shaderOpaco, materialDosOlhos);
    if (comGarfos) {
        desenharComShader(doGuido, shaderOpaco, -1);
    }
    desenharRodas(numero, modelo, noMundo, anguloDasRodas, volante, 0);
    float normal = 1.0f;   /* o shaderOpaco também desenha outras coisas, como a roda solta */
    SetShaderValue(shaderOpaco, locsDoReflexo[0], &normal, SHADER_UNIFORM_FLOAT);

    BeginBlendMode(BLEND_ALPHA);
    rlDisableDepthMask();
    desenharSoTranslucidos(modelo, shaderTranslucido, translucidos[numero]);
    if (comGarfos) {
        desenharSoTranslucidos(doGuido, shaderTranslucido, translucidosDosGarfos);
    }
    desenharRodas(numero, modelo, noMundo, anguloDasRodas, volante, 1);
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
    EndBlendMode();
    rodaEmTroca = -1;
    rodaEscondida = -1;
    temPoseDosGarfos = 0;
    temCorpoInclinado = 0;
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
    if (numero == CARRO_COM_GARFOS && temGarfos) {
        /* o Guido mede com os garfos, como o modelo inteiro vinha */
        BoundingBox dosGarfos = GetModelBoundingBox(garfos);
        caixa.min = Vector3Min(caixa.min, dosGarfos.min);
        caixa.max = Vector3Max(caixa.max, dosGarfos.max);
    }
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
 * medirCaixaDoCarro
 * Mede o volume de verdade do carro, no carro normalizado (antes de separar as rodas).
 * Recebe: o número do carro.
 * Devolve: nada.
 */
static void medirCaixaDoCarro(int numero) {
    Model modelo = modelos[numero];
    BoundingBox caixa = { { 1e9f, 1e9f, 1e9f }, { -1e9f, -1e9f, -1e9f } };
    for (int m = 0; m < modelo.meshCount; m++) {
        Mesh malha = modelo.meshes[m];
        for (int v = 0; v < malha.vertexCount; v++) {
            Vector3 ponto = verticeNormalizado(malha, v, ajuste[numero]);
            caixa.min = Vector3Min(caixa.min, ponto);
            caixa.max = Vector3Max(caixa.max, ponto);
        }
    }
    caixasDosCarros[numero] = caixa;
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
/*
 * medirTranslucidos
 * Quais materiais do modelo têm algo translúcido (vidro, faróis): só eles entram na segunda
 * passada do desenho. Olha a cor do material, as cores dos vértices e os pixels da textura.
 * Recebe: o modelo.
 * Devolve: um vetor com 1 para cada material translúcido (MemFree para liberar).
 */
static unsigned char *medirTranslucidos(Model modelo) {
    unsigned char *translucido = (unsigned char *) MemAlloc(modelo.materialCount);
    for (int m = 0; m < modelo.materialCount; m++) {
        MaterialMap mapa = modelo.materials[m].maps[MATERIAL_MAP_ALBEDO];
        if (mapa.color.a < 230) {
            translucido[m] = 1;
            continue;
        }
        if (mapa.texture.id != 0 && mapa.texture.id != rlGetTextureIdDefault()) {
            Image imagem = LoadImageFromTexture(mapa.texture);
            Color *pixels = LoadImageColors(imagem);
            for (int i = 0; i < imagem.width * imagem.height; i++) {
                if (pixels[i].a > 5 && pixels[i].a < 230) {
                    translucido[m] = 1;
                    break;
                }
            }
            UnloadImageColors(pixels);
            UnloadImage(imagem);
        }
    }
    for (int i = 0; i < modelo.meshCount; i++) {
        Mesh malha = modelo.meshes[i];
        if (malha.colors == NULL) {
            continue;
        }
        for (int v = 0; v < malha.vertexCount; v++) {
            if (malha.colors[v * 4 + 3] < 230) {
                translucido[modelo.meshMaterial[i]] = 1;
                break;
            }
        }
    }
    return translucido;
}

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
 * extremosNaFoto
 * Onde os cantos do carro caem na foto, com a câmera numa certa posição.
 * Recebe: o carro, a câmera, a direção dela para a frente, a direita, o cima, a tangente
 *         da metade do ângulo de visão e onde guardar os extremos (-1 a +1).
 * Devolve: nada.
 */
static void extremosNaFoto(int numero, Vector3 posicao, Vector3 frente, Vector3 direita, Vector3 cima, float tangente,
                           float *esquerda, float *direitaMax, float *baixo, float *alto) {
    BoundingBox caixa = caixasDosCarros[numero];
    *esquerda = *baixo = 1e9f;
    *direitaMax = *alto = -1e9f;
    for (int i = 0; i < 8; i++) {
        Vector3 canto = { (i & 1) ? caixa.max.x : caixa.min.x, (i & 2) ? caixa.max.y : caixa.min.y, (i & 4) ? caixa.max.z : caixa.min.z };
        Vector3 relativo = Vector3Subtract(canto, posicao);
        float profundidade = Vector3DotProduct(relativo, frente);
        float x = Vector3DotProduct(relativo, direita) / (profundidade * tangente);
        float y = Vector3DotProduct(relativo, cima) / (profundidade * tangente);
        *esquerda = fminf(*esquerda, x);
        *direitaMax = fmaxf(*direitaMax, x);
        *baixo = fminf(*baixo, y);
        *alto = fmaxf(*alto, y);
    }
}

/*
 * prepararMiniatura
 * Tira uma "foto" do carro, de frente e um pouco de lado, numa textura. O enquadramento
 * segue o volume de verdade de cada carro (a asa do Wingo, os garfos do Guido): a câmera
 * se afasta até ele caber na área da foto, e se move para deixá-lo centralizado, com as
 * rodas sempre na mesma altura.
 * Recebe: o número do carro.
 * Devolve: nada.
 */
static void prepararMiniatura(int numero) {
    BoundingBox caixa = caixasDosCarros[numero];
    Vector3 centro = Vector3Scale(Vector3Add(caixa.min, caixa.max), 0.5f);
    Vector3 paraCamera = Vector3Normalize(DIRECAO_DA_FOTO);
    Vector3 frente = Vector3Negate(paraCamera);
    Vector3 direita = Vector3Normalize(Vector3CrossProduct(frente, (Vector3){ 0.0f, 1.0f, 0.0f }));
    Vector3 cima = Vector3CrossProduct(direita, frente);
    float tangente = tanf(GRAUS_DA_FOTO * DEG2RAD / 2.0f);

    /* Primeiro a distância: o mais perto possível, sem sair da área da foto */
    float perto = 1.0f, longe = 200.0f;
    for (int passo = 0; passo < 30; passo++) {
        float distancia = (perto + longe) / 2.0f;
        Vector3 tentativa = Vector3Add(centro, Vector3Scale(paraCamera, distancia));
        float e, d, b, a;
        extremosNaFoto(numero, tentativa, frente, direita, cima, tangente, &e, &d, &b, &a);
        if (d - e > LARGURA_NA_FOTO * 2.0f || a - b > ALTURA_NA_FOTO * 2.0f) {
            perto = distancia;
        } else {
            longe = distancia;
        }
    }

    /* Depois, centraliza na horizontal e põe as rodas na linha do chão da foto */
    Vector3 posicao = Vector3Add(centro, Vector3Scale(paraCamera, longe));
    for (int passo = 0; passo < 4; passo++) {
        float e, d, b, a;
        extremosNaFoto(numero, posicao, frente, direita, cima, tangente, &e, &d, &b, &a);
        float profundidade = Vector3DotProduct(Vector3Subtract(centro, posicao), frente);
        posicao = Vector3Add(posicao, Vector3Scale(direita, (e + d) / 2.0f * profundidade * tangente));
        posicao = Vector3Add(posicao, Vector3Scale(cima, (b - CHAO_NA_FOTO) * profundidade * tangente));
    }

    Camera3D camera = { 0 };
    camera.position = posicao;
    camera.target = Vector3Add(posicao, frente);
    camera.up = cima;
    camera.fovy = GRAUS_DA_FOTO;
    camera.projection = CAMERA_PERSPECTIVE;

    definirOlhoDosCarros(camera.position);   /* o reflexo da pintura com a câmera da foto */
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

    /* Com mipmaps, a foto grande fica nítida também em tamanho pequeno */
    GenTextureMipmaps(&miniaturas[numero].texture);
    SetTextureFilter(miniaturas[numero].texture, TEXTURE_FILTER_TRILINEAR);
}

/*
 * desenharMiniatura
 * A foto de um carro, em 2D, para as telas de menu. Na foto o chão fica sempre na mesma
 * altura: é ele que vai na base pedida.
 * Recebe: o número do carro, o centro, a base e a largura na tela.
 * Devolve: nada.
 */
void desenharMiniatura(int numero, float centroX, float baseY, float larguraNaTela) {
    Texture2D foto = miniaturas[numero].texture;
    Rectangle origem = { 0.0f, 0.0f, (float) foto.width, -(float) foto.height };   /* a foto vem de cabeça para baixo */
    float lado = larguraNaTela / LARGURA_NA_FOTO;
    float chao = (1.0f - CHAO_NA_FOTO) / 2.0f;     /* de 0 (em cima) a 1 (embaixo) */
    Rectangle destino = { centroX - lado / 2.0f, baseY - lado * chao, lado, lado };
    DrawTexturePro(foto, origem, destino, (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);
}

/*
 * bordaDaMiniatura
 * Para conferir o enquadramento: procura pixels do carro encostados na borda da foto.
 * Recebe: o número do carro.
 * Devolve: quantos pixels bem opacos estão na borda.
 */
int bordaDaMiniatura(int numero) {
    Image foto = LoadImageFromTexture(miniaturas[numero].texture);
    ImageFormat(&foto, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    Color *pixels = (Color *) foto.data;
    int total = 0;
    for (int i = 0; i < foto.width; i++) {
        total += pixels[i].a > 200;
        total += pixels[(foto.height - 1) * foto.width + i].a > 200;
        total += pixels[i * foto.width].a > 200;
        total += pixels[i * foto.width + foto.width - 1].a > 200;
    }
    UnloadImage(foto);
    return total;
}

/*
 * trocarTextura
 * Põe outra textura num mapa do material, devolvendo a que estava lá (se não for a padrão
 * da raylib nem a mesma que outro mapa do material ainda usa).
 * Recebe: o material, o mapa e a textura nova.
 * Devolve: nada.
 */
static void trocarTextura(Material *material, int mapa, Texture2D nova) {
    Texture2D velha = material->maps[mapa].texture;
    material->maps[mapa].texture = nova;
    if (velha.id == 0 || velha.id == rlGetTextureIdDefault()) {
        return;
    }
    for (int m = 0; m <= MATERIAL_MAP_BRDF; m++) {   /* os mapas que a raylib conhece */
        if (material->maps[m].texture.id == velha.id) {
            return;
        }
    }
    UnloadTexture(velha);
}

/*
 * acertarCores
 * Aplica os ACERTOS_DE_COR do carro (se tiver) em todas as texturas dele. Só mexe nos
 * pixels perto do tom escolhido, e cada vez menos quanto mais longe dele.
 * Recebe: o número do carro (o modelo já carregado).
 * Devolve: nada.
 */
static void acertarCores(int numero) {
    reflexos[numero] = 1.0f;
    for (int a = 0; a < (int) (sizeof(ACERTOS_DE_COR) / sizeof(ACERTOS_DE_COR[0])); a++) {
        const AcertoDeCor *acerto = &ACERTOS_DE_COR[a];
        if (strcmp(acerto->arquivo, MODELOS[numero].arquivo) != 0) {
            continue;
        }
        reflexos[numero] = acerto->reflexo;
        for (int k = 0; k < 4 && acerto->materiais[k] >= 0; k++) {
            int m = acerto->materiais[k] + 1;   /* a raylib põe um material padrão antes dos do .glb */
            if (m >= modelos[numero].materialCount) {
                continue;
            }
            Texture2D textura = modelos[numero].materials[m].maps[MATERIAL_MAP_ALBEDO].texture;
            if (textura.id == 0 || textura.id == rlGetTextureIdDefault()) {
                continue;
            }
            Image imagem = LoadImageFromTexture(textura);
            Color *pixels = LoadImageColors(imagem);
            int mexeu = 0;
            for (int i = 0; i < imagem.width * imagem.height; i++) {
                Vector3 hsv = ColorToHSV(pixels[i]);   /* tom em graus, saturação e brilho de 0 a 1 */
                float distancia = fabsf(fmodf(hsv.x - acerto->tomDe + 540.0f, 360.0f) - 180.0f);
                if (distancia >= acerto->largura || hsv.y < 0.15f) {
                    continue;
                }
                float peso = 1.0f - distancia / acerto->largura;
                peso = peso * peso * (3.0f - 2.0f * peso) * fminf(1.0f, (hsv.y - 0.15f) / 0.10f);
                float tom = hsv.x + acerto->girar * peso;
                float saturacao = fminf(1.0f, hsv.y * (1.0f + (acerto->saturacao - 1.0f) * peso));
                float brilho = fminf(1.0f, hsv.z * (1.0f + (acerto->brilho - 1.0f) * peso));
                Color nova = ColorFromHSV(fmodf(tom + 360.0f, 360.0f), saturacao, brilho);
                nova.a = pixels[i].a;
                pixels[i] = nova;
                mexeu = 1;
            }
            if (mexeu) {
                Image acertada = { pixels, imagem.width, imagem.height, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };
                trocarTextura(&modelos[numero].materials[m], MATERIAL_MAP_ALBEDO, LoadTextureFromImage(acertada));
            }
            UnloadImageColors(pixels);
            UnloadImage(imagem);
        }
    }
}

/*
 * medirBordasDaPalpebra
 * Para cada coluna da textura dos olhos, o maior trecho de branco do olho (na máscara): de um
 * lado ele acaba na pálpebra, do outro no fim do olho. O shader usa isso para fechar a pálpebra
 * coluna por coluna (veja OLHOS_DO_SHADER).
 * Recebe: a máscara (o arquivo) e de que lado está a pálpebra (+1 embaixo na textura, -1 em
 *         cima, 0 nenhum).
 * Devolve: uma textura de uma linha: em cada coluna, a altura da borda da pálpebra, a do fim
 *          do olho e 1 se a coluna tem olho (0 se não).
 */
static Texture2D medirBordasDaPalpebra(const char *mascara, int ladoDaPalpebra) {
    Image imagem = LoadImage(mascara);
    ImageFormat(&imagem, PIXELFORMAT_UNCOMPRESSED_GRAYSCALE);
    const unsigned char *pixels = (const unsigned char *) imagem.data;
    int largura = imagem.width, altura = imagem.height;
    float *bordas = (float *) MemAlloc(sizeof(float) * 3 * largura);
    for (int x = 0; x < largura; x++) {
        int melhorInicio = 0, melhorTamanho = 0, inicio = -1;
        for (int y = 0; y <= altura; y++) {
            int branco = (y < altura) && pixels[y * largura + x] >= 128;
            if (branco && inicio < 0) {
                inicio = y;
            } else if (!branco && inicio >= 0) {
                if (y - inicio > melhorTamanho) {
                    melhorInicio = inicio;
                    melhorTamanho = y - inicio;
                }
                inicio = -1;
            }
        }
        float de = (float) melhorInicio / altura, ate = (float) (melhorInicio + melhorTamanho) / altura;
        int temOlho = ladoDaPalpebra != 0 && melhorTamanho >= 3;
        bordas[x * 3 + 0] = (ladoDaPalpebra > 0) ? ate : de;   /* a borda da pálpebra */
        bordas[x * 3 + 1] = (ladoDaPalpebra > 0) ? de : ate;   /* o fim do olho, do outro lado */
        bordas[x * 3 + 2] = temOlho ? 1.0f : 0.0f;
    }
    Image linha = { bordas, largura, 1, 1, PIXELFORMAT_UNCOMPRESSED_R32G32B32 };
    Texture2D textura = LoadTextureFromImage(linha);
    SetTextureFilter(textura, TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(textura, TEXTURE_WRAP_CLAMP);
    MemFree(bordas);
    UnloadImage(imagem);
    return textura;
}

/*
 * carregarOlhos
 * Procura o carro na tabela dos olhos (recursos/modelos/carros/olhos/olhos.txt) e troca
 * a textura dos olhos dele pelas três camadas: o fundo, as íris e a máscara do branco.
 * Sem a tabela ou sem as imagens, o carro fica com os olhos parados, como vieram.
 * Recebe: o número do carro e o texto da tabela (NULL se não há).
 * Devolve: nada.
 */
static void carregarOlhos(int numero, const char *tabela) {
    olhos[numero].tem = 0;
    if (tabela == NULL) {
        return;
    }
    char nome[64];
    strncpy(nome, GetFileNameWithoutExt(MODELOS[numero].arquivo), sizeof(nome) - 1);
    nome[sizeof(nome) - 1] = '\0';
    int material = -1;
    OlhosDoCarro achados = { 0 };
    for (const char *linha = tabela; *linha != '\0'; ) {
        char carro[64];
        int noGlb, ladoDaPalpebra;
        OlhosDoCarro lidos = { 0 };
        if (*linha != '#'
            && sscanf(linha, "%63s %d %f %d %f %f %f %f %f %f", carro, &noGlb, &lidos.anda, &ladoDaPalpebra,
                      &lidos.linha, &lidos.raio, &lidos.meios.x, &lidos.meios.y, &lidos.meios.z, &lidos.meios.w) == 10
            && strcmp(carro, nome) == 0) {
            material = noGlb + 1;   /* a raylib põe um material padrão antes dos do .glb */
            lidos.ladoDaPalpebra = (float) ladoDaPalpebra;
            achados = lidos;
            break;
        }
        const char *fim = strchr(linha, '\n');
        if (fim == NULL) {
            break;
        }
        linha = fim + 1;
    }
    Model *modelo = &modelos[numero];
    if (material < 1 || material >= modelo->materialCount) {
        return;
    }
    static const char *CAMADAS[3] = { "fundo", "iris", "mascara" };
    char caminhos[3][256];
    for (int i = 0; i < 3; i++) {
        snprintf(caminhos[i], sizeof(caminhos[i]), "%s%s_%s.png", PASTA_DOS_OLHOS, nome, CAMADAS[i]);
        if (!FileExists(caminhos[i])) {
            TraceLog(LOG_WARNING, "CARRO: falta %s, os olhos de %s ficam parados", caminhos[i], nome);
            return;
        }
    }
    Texture2D original = modelo->materials[material].maps[MATERIAL_MAP_ALBEDO].texture;
    Texture2D camadas[3];
    for (int i = 0; i < 3; i++) {
        camadas[i] = LoadTexture(caminhos[i]);
    }
    if (camadas[0].width != original.width || camadas[0].height != original.height) {
        TraceLog(LOG_WARNING, "CARRO: os olhos de %s nao batem com a textura do modelo (%dx%d e %dx%d)",
                 nome, camadas[0].width, camadas[0].height, original.width, original.height);
        for (int i = 0; i < 3; i++) {
            UnloadTexture(camadas[i]);
        }
        return;
    }
    /* o fundo ganha mipmaps em prepararTexturas, como as outras texturas de cor */
    for (int i = 1; i < 3; i++) {
        GenTextureMipmaps(&camadas[i]);
        SetTextureFilter(camadas[i], TEXTURE_FILTER_TRILINEAR);
    }
    Material *olho = &modelo->materials[material];
    trocarTextura(olho, MATERIAL_MAP_ALBEDO, camadas[0]);
    trocarTextura(olho, MATERIAL_MAP_METALNESS, camadas[1]);   /* o texture1 do shader */
    trocarTextura(olho, MATERIAL_MAP_NORMAL, camadas[2]);      /* o texture2 */
    trocarTextura(olho, MATERIAL_MAP_ROUGHNESS, medirBordasDaPalpebra(caminhos[2], (int) achados.ladoDaPalpebra));   /* o texture3 */
    achados.tem = 1;
    achados.material = material;
    olhos[numero] = achados;   /* o lado é medido depois, com o carro normalizado */
}

/*
 * medirLadoDosOlhos
 * Para que lado da textura dos olhos fica a direita do carro: compara, nos vértices da
 * peça dos olhos, a coordenada da textura com a posição de lado no carro normalizado
 * (frente em +z e y para cima: a direita do carro é o -x).
 * Recebe: o número do carro (já normalizado).
 * Devolve: nada.
 */
static void medirLadoDosOlhos(int numero) {
    /* (e o mesmo na altura: para que lado da textura fica o alto do carro) */
    if (!olhos[numero].tem) {
        return;
    }
    Model modelo = modelos[numero];
    double somaX = 0.0, somaU = 0.0, somaXU = 0.0, somaXX = 0.0;
    double somaY = 0.0, somaV = 0.0, somaYV = 0.0;
    long total = 0;
    for (int i = 0; i < modelo.meshCount; i++) {
        Mesh malha = modelo.meshes[i];
        if (modelo.meshMaterial[i] != olhos[numero].material || malha.texcoords == NULL) {
            continue;
        }
        for (int v = 0; v < malha.vertexCount; v++) {
            double x = verticeNormalizado(malha, v, ajuste[numero]).x;
            double u = malha.texcoords[v * 2];
            double y = verticeNormalizado(malha, v, ajuste[numero]).y;
            double altura = malha.texcoords[v * 2 + 1];
            somaY += y;
            somaV += altura;
            somaYV += y * altura;
            somaX += x;
            somaU += u;
            somaXU += x * u;
            somaXX += x * x;
            total++;
        }
    }
    double variacao = (total > 0) ? somaXX - somaX * somaX / total : 0.0;
    double junto = (total > 0) ? somaXU - somaX * somaU / total : 0.0;
    if (total < 3 || variacao <= 0.0 || fabs(junto) < 1e-9) {
        olhos[numero].tem = 0;
        TraceLog(LOG_WARNING, "CARRO: nao deu para medir os olhos de %s", MODELOS[numero].arquivo);
        return;
    }
    olhos[numero].lado = (junto > 0.0) ? -1.0f : 1.0f;   /* andar para -x é andar para a direita */
    olhos[numero].ladoDeCima = (somaYV - somaY * somaV / total > 0.0) ? 1.0f : -1.0f;
    TraceLog(LOG_INFO, "CARRO: olhos de %s no material %d, a direita fica no %s da textura",
             MODELOS[numero].arquivo, olhos[numero].material, (olhos[numero].lado < 0.0f) ? "u menor" : "u maior");
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
    shaderDosOlhos = LoadShaderFromMemory(VERTICES_DO_SHADER,
                                          PIXELS_DO_SHADER_LENDO(OLHOS_DO_SHADER, "corDosOlhos(fragTexCoord)", "", "1.0"));
    locDoDesvio = GetShaderLocation(shaderDosOlhos, "desvio");
    locDosMeios = GetShaderLocation(shaderDosOlhos, "meiosDasIris");
    locDoTamanho = GetShaderLocation(shaderDosOlhos, "tamanhoDaIris");
    locDaForma = GetShaderLocation(shaderDosOlhos, "formaDaPalpebra");
    /* a raylib só liga sozinha texture0 a texture2; a texture3 (as bordas) é o mapa de rugosidade */
    shaderDosOlhos.locs[SHADER_LOC_MAP_ROUGHNESS] = GetShaderLocation(shaderDosOlhos, "texture3");
    registrarShaderIluminado(shaderOpaco);
    registrarShaderIluminado(shaderTranslucido);
    registrarShaderIluminado(shaderDosOlhos);
    Shader comReflexo[3] = { shaderOpaco, shaderTranslucido, shaderDosOlhos };
    float reflexoNormal = 1.0f;   /* sem isto o uniform começa em 0: nenhum carro refletiria */
    for (int i = 0; i < 3; i++) {
        locsDoReflexo[i] = GetShaderLocation(comReflexo[i], "reflexoDaPintura");
        locsDoOlho[i] = GetShaderLocation(comReflexo[i], "olho");
        SetShaderValue(comReflexo[i], locsDoReflexo[i], &reflexoNormal, SHADER_UNIFORM_FLOAT);
    }
    char *tabelaDosOlhos = FileExists(PASTA_DOS_OLHOS "olhos.txt") ? LoadFileText(PASTA_DOS_OLHOS "olhos.txt") : NULL;
    shaderDaSombra = LoadShaderFromMemory(VERTICES_DO_SHADER, PIXELS_DA_SOMBRA_TEXTO("0.82"));

    for (int numero = 0; numero < TOTAL_CARROS; numero++) {
        modelos[numero] = LoadModel(TextFormat("%s%s", PASTA_DOS_MODELOS, MODELOS[numero].arquivo));
        acertarCores(numero);
        carregarOlhos(numero, tabelaDosOlhos);
        prepararTexturas(modelos[numero]);
        translucidos[numero] = medirTranslucidos(modelos[numero]);
        int quantos = 0;
        for (int m = 0; m < modelos[numero].materialCount; m++) {
            quantos += translucidos[numero][m];
        }
        TraceLog(LOG_INFO, "CARRO: %d de %d materiais com algo translucido", quantos, modelos[numero].materialCount);
        if (numero == CARRO_COM_GARFOS && FileExists(TextFormat("%s%s", PASTA_DOS_MODELOS, ARQUIVO_DOS_GARFOS))) {
            garfos = LoadModel(TextFormat("%s%s", PASTA_DOS_MODELOS, ARQUIVO_DOS_GARFOS));
            prepararTexturas(garfos);
            translucidosDosGarfos = medirTranslucidos(garfos);
            temGarfos = 1;
        }
        normalizarModelo(numero);
        medirLadoDosOlhos(numero);
        if (numero == CARRO_COM_GARFOS && temGarfos) {
            /* a ponta das pás: o meio delas, em cima, no carro normalizado */
            BoundingBox g = GetModelBoundingBox(garfos);
            Vector3 a = Vector3Transform(g.min, ajuste[numero]), b = Vector3Transform(g.max, ajuste[numero]);
            pontaDosGarfosNormal = (Vector3){ 0.0f, fminf(a.y, b.y) + 0.05f, fmaxf(a.z, b.z) - 0.25f };
        }
        prepararSombra(numero);
        BoundingBox caixa = GetModelBoundingBox(modelos[numero]);
        float escala = fichaDoCarro(numero)->comprimento / fmaxf(caixa.max.x - caixa.min.x, caixa.max.z - caixa.min.z);
        medirCaixaDoCarro(numero);
        acharRodas(numero, fichaDoCarro(numero)->largura, (caixa.max.y - caixa.min.y) * escala);
        separarRodas(numero);
        TraceLog(LOG_INFO, "CARRO: %s tem %d rodas, em %d pecas", fichaDoCarro(numero)->nome, totalDeRodas[numero], totalDePecas[numero]);
        prepararMiniatura(numero);
    }
    if (tabelaDosOlhos != NULL) {
        UnloadFileText(tabelaDosOlhos);
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
        MemFree(translucidos[numero]);
        translucidos[numero] = NULL;
        if (numero == CARRO_COM_GARFOS && temGarfos) {
            UnloadModel(garfos);
            MemFree(translucidosDosGarfos);
            translucidosDosGarfos = NULL;
            temGarfos = 0;
        }
        for (int i = 0; i < totalDePecas[numero]; i++) {
            UnloadMesh(pecas[numero][i].malha);
        }
        UnloadRenderTexture(miniaturas[numero]);
        UnloadTexture(sombras[numero]);
    }
    UnloadShader(shaderOpaco);
    UnloadShader(shaderTranslucido);
    UnloadShader(shaderDosOlhos);
    UnloadShader(shaderDaSombra);
}
