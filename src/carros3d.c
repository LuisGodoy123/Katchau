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
#include "espera.h"
#include "raymath.h"
#include "rlgl.h"
#include "corrida.h"
#include "moradores.h"
#include "noite.h"
#include "hora.h"
#include "exibicao.h"

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
#define GRAUS_DAS_MAOS_ABERTAS 14.0f    /* as rodas da frente abertas para fora, cada uma para o seu lado (gestos.c) */
#define CURSO_DO_BICO         0.22f     /* metros: o quanto o bico de um morador sobe ou desce no eixo da frente */

/* Uma ilha: um pedaço contínuo de triângulos de uma malha */
typedef struct {
    int triangulos;
    Vector3 minimo, maximo;
} Ilha;

/* Uma roda: onde fica o eixo dela, no carro normalizado */
typedef struct {
    Vector3 centro;
    float raio, xMinimo, xMaximo;
    float raioDoAro;   /* onde acaba o aro e começa a borracha (o que sobra quando o pneu estoura) */
} RodaDoCarro;

/* As peças de uma roda que usam o mesmo material */
typedef struct {
    Mesh malha;        /* com o centro da roda na origem */
    Mesh aro;          /* só o aro (sem a borracha), para o pneu estourado */
    int temAro;
    int material;
    int roda;
} PecaDeRoda;

/* Cada modelo, na ordem de FICHAS (corrida.c). O comprimento está
 * na ficha; "deitado" marca o modelo com a frente em +x. Depois dos carros da corrida vêm os
 * figurantes (moradores.h), que só andam pela cidade: o arquivo, o tamanho e a frente estão na ficha deles. */
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
    { "jackson_storm.glb", 0 },
    { "boost.glb", 0 },
    { "dj.glb", 0 },
};

/* O arquivo, a frente (em quartos de volta: 0 = +z, 1 = +x, 2 = -z, 3 = -x) e as medidas de cada modelo */
static const char *arquivoDoModelo(int numero) {
    return (numero < TOTAL_CARROS) ? MODELOS[numero].arquivo : fichaDoFigurante(numero - TOTAL_CARROS)->arquivo;
}

static int quartosDoModelo(int numero) {
    return (numero < TOTAL_CARROS) ? MODELOS[numero].deitado : fichaDoFigurante(numero - TOTAL_CARROS)->quartos;
}

static float comprimentoDaFicha(int numero) {
    float comprimento, largura;
    medidasDoModelo(numero, &comprimento, &largura);
    return comprimento;
}

static float larguraDaFicha(int numero) {
    float comprimento, largura;
    medidasDoModelo(numero, &comprimento, &largura);
    return largura;
}

/* Os vértices. A boca: os da mandíbula (o peso de cada um vem no x de vertexTexCoord2, posto por
 * prepararBoca; nos outros é 0) giram em volta da dobradiça, na horizontal de lado a lado do carro, o
 * quanto a boca abre vezes o peso: a frente desce. A normal gira junto. O fragNaPeca (o corte do aro do
 * pneu estourado) continua sendo o ponto parado */
static const char *VERTICES_DO_SHADER =
    "#version 330\n"
    "in vec3 vertexPosition; in vec2 vertexTexCoord; in vec3 vertexNormal; in vec4 vertexColor; in vec2 vertexTexCoord2;\n"
    "uniform mat4 mvp; uniform mat4 matModel;\n"
    "uniform vec3 mandibula;\n" /* a dobradiça (y, z) e o ângulo da boca aberta (radianos) */
    "out vec2 fragTexCoord; out vec4 fragColor; out vec3 fragNormal; out vec3 fragPos; out vec3 fragNaPeca;\n"
    "void main() { fragTexCoord = vertexTexCoord; fragColor = vertexColor; fragNaPeca = vertexPosition;\n"
    "              vec3 p = vertexPosition, n = vertexNormal;\n"
    "              float a = mandibula.z * vertexTexCoord2.x;\n"
    "              if (a != 0.0) {\n"
    "                  float c = cos(a), s = sin(a);\n"
    "                  vec2 d = p.yz - mandibula.xy;\n"
    "                  p.yz = mandibula.xy + vec2(d.x * c - d.y * s, d.x * s + d.y * c);\n"
    "                  n.yz = vec2(n.y * c - n.z * s, n.y * s + n.z * c);\n"
    "              }\n"
    "              fragNormal = mat3(matModel) * n;\n"
    "              fragPos = vec3(matModel * vec4(p, 1.0));\n"
    "              gl_Position = mvp * vec4(p, 1.0); }\n";

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
 * leitura da cor (a dos olhos junta três texturas, veja OLHOS_DO_SHADER).
 * No passeio, o sol, a luz e o céu refletido mudam com a hora (hora.c). */
#define PIXELS_DO_SHADER_LENDO(DECLARACOES, LEITURA, CORTE, SAIDA) \
    "#version 330\n" \
    "in vec2 fragTexCoord; in vec4 fragColor; in vec3 fragNormal; in vec3 fragPos; in vec3 fragNaPeca;\n" \
    "uniform sampler2D texture0; uniform vec4 colDiffuse; uniform vec3 olho; uniform float reflexoDaPintura;\n" \
    "uniform float corteDoAro;\n" /* na roda de pneu estourado: o que passa desse raio (em volta do eixo x) some */ \
    DECLARACOES \
    "out vec4 finalColor;\n" \
    GLSL_DA_LUZ_DA_NOITE GLSL_DO_SOL GLSL_DA_LUZ_DO_DIA GLSL_DO_REFLEXO_DO_DIA GLSL_DAS_LAMPADAS \
    "void main() { vec4 cor = " LEITURA " * colDiffuse * fragColor;\n" \
    "              " CORTE "\n" \
    "              if (corteDoAro > 0.0 && length(fragNaPeca.yz) > corteDoAro) discard;\n" \
    "              vec3 n = normalize(fragNormal);\n" \
    "              if (!gl_FrontFacing) n = -n;\n" \
    "              float sol = max(dot(n, solDoDia), 0.0);\n" \
    "              vec3 luz = 0.62 * luzDeBase + 0.28 * (n.y * 0.5 + 0.5) * luzDoCeu + 0.42 * sol * luzDoSol;\n" \
    "              vec3 comLuz = (noite > 0.5) ? luzDaNoite(fragPos, n) * 1.1 : luz + luzDasLampadas(fragPos, n);\n" \
    "              vec3 v = normalize(olho - fragPos);\n" \
    "              vec3 r = reflect(-v, n);\n" \
    "              float fresnel = pow(1.0 - max(dot(n, v), 0.0), 4.0);\n" \
    "              float lataria = smoothstep(0.12, 0.38, max(cor.r, max(cor.g, cor.b)));\n" \
    "              vec3 ceu = (noite > 0.5) ? mix(vec3(0.03, 0.04, 0.08), vec3(0.20, 0.26, 0.42), clamp(r.y, 0.0, 1.0))\n" \
    "                                       : mix(ceuDeBaixo, ceuDeCima, clamp(r.y * 1.5, 0.0, 1.0));\n" \
    "              float reflexo = lataria * (0.08 + 0.50 * fresnel) * reflexoDaPintura;\n" \
    "              float estrela = (noite > 0.5) ? 0.0 : pow(max(dot(r, solDoDia), 0.0), 60.0) * lataria;\n" \
    "              vec3 base = mix(cor.rgb * comLuz, ceu, reflexo) + vec3(1.0, 0.97, 0.9) * luzDoSol * estrela * 0.9;\n" \
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
/* As emoções dos moradores (gestos.h), cada uma inteira: a pálpebra faz de sobrancelha */
#define ALEGRIA_FECHA         0.16f     /* contente: os olhos sorriem, um pouco apertados... */
#define ALEGRIA_INCLINA      -0.18f     /* ...com o canto de dentro erguido (a cara amiga) */
#define ALEGRIA_ARCO          0.12f
#define ALEGRIA_IRIS          1.06f
#define RAIVA_FECHA           0.28f     /* bravo: a pálpebra desce sobre o canto de dentro */
#define RAIVA_INCLINA         0.60f
#define RAIVA_ARCO           -0.10f
#define RAIVA_IRIS            0.90f
#define TRISTEZA_FECHA        0.28f     /* triste ou tímido: o canto de dentro sobe e o de fora cai */
#define TRISTEZA_INCLINA     -0.50f
#define TRISTEZA_IRIS         1.08f
#define ESPANTO_FECHA        -0.28f     /* curioso, perguntando: arregalado, a sobrancelha em arco */
#define ESPANTO_ARCO          0.30f
#define ESPANTO_IRIS          0.94f
#define SONO_FECHA            0.80f     /* calmo, pensativo, cochilando: as pálpebras caem */
#define MAIS_FECHADO          0.95f     /* somando tudo, a pálpebra para aqui... */
#define MAIS_ABERTO          -0.45f     /* ...e aqui */
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
    Emocao emocao;                 /* a emoção pedida para o próximo desenho (os moradores) */
    float susto;                   /* o susto de agora (passa devagar) */
    float piscada;                 /* segundos desde o começo da piscada (< 0: sem piscada) */
    float proximaPiscada;          /* segundos até a próxima */
    float proximaOlhadela;         /* segundos até mudar a olhadela */
    float olhadelaX, olhadelaY;
    float olharPedido;             /* para onde olhar no próximo desenho, no lugar do volante... */
    int temOlharPedido;            /* ...se pediram (olharNoProximoDesenho) */
    unsigned int sorteio;          /* para os sorteios do carro (não mexe no rand() do jogo) */
    double hora;                   /* do último desenho (para saber quanto tempo passou) */
} CaraDoCarro;

/* Um lugar a mais, depois dos carros: os olhos de um modelo que não é carro (o Mack, darOlhosAoModelo) */
#define CARA_DE_OUTRO_MODELO  TOTAL_DE_MODELOS
static OlhosDoCarro olhos[TOTAL_DE_MODELOS + 1];
static CaraDoCarro caras[TOTAL_DE_MODELOS + 1];
static float reflexos[TOTAL_DE_MODELOS];    /* quanto o céu se reflete na pintura de cada carro */
static Shader shaderDosOlhos;
static int locDoDesvio, locDosMeios, locDoTamanho, locDaForma;
static int locsDoReflexo[3];   /* reflexoDaPintura no opaco, no translúcido e no dos olhos */
static int locsDoCorte[2];     /* corteDoAro no opaco e no translúcido */
static int locsDoOlho[3];      /* "olho" nos mesmos três: procurar pelo nome a cada quadro é caro */
static int locsDaMandibula[2]; /* "mandibula" no opaco e no translúcido */

/* A boca de cada modelo (recursos/modelos/carros/bocas.txt, de ferramentas/medir_bocas.py) */
#define ARQUIVO_DAS_BOCAS     PASTA_DOS_MODELOS "bocas.txt"
#define MESMO_PONTO           2e-5f     /* unidades do modelo: o vértice da tabela e o da malha são o mesmo */
typedef struct {
    int tem;
    float dobradicaY, dobradicaZ;   /* no modelo como veio */
    float graus;                    /* a boca toda aberta */
} BocaDoCarro;
static BocaDoCarro bocas[TOTAL_DE_MODELOS];
static float aberturaNoProximo[TOTAL_DE_MODELOS];   /* 0 fechada a 1 aberta: vale só para o próximo desenho */

static Model modelos[TOTAL_DE_MODELOS];
static Matrix ajuste[TOTAL_DE_MODELOS];     /* do modelo como veio para o modelo normalizado */
static float larguras[TOTAL_DE_MODELOS];    /* largura de verdade, em metros */
static RenderTexture2D miniaturas[TOTAL_CARROS];
static BoundingBox caixasDosCarros[TOTAL_DE_MODELOS];   /* o volume de verdade, já com o ajuste do carro */
static Shader shaderOpaco, shaderTranslucido, shaderDaSombra;
static Texture2D sombras[TOTAL_DE_MODELOS];
static RodaDoCarro rodas[TOTAL_DE_MODELOS][MAXIMO_DE_RODAS];
static int totalDeRodas[TOTAL_DE_MODELOS];
static PecaDeRoda pecas[TOTAL_DE_MODELOS][MAXIMO_DE_PECAS_RODA * MAXIMO_DE_RODAS];
static int totalDePecas[TOTAL_DE_MODELOS];
/* Os garfos do Guido (os doGuido dele) vêm num modelo à parte, nos mesmos eixos: assim sobem e inclinam */
#define ARQUIVO_DOS_GARFOS    "guido_garfos.glb"
#define CARRO_COM_GARFOS      7          /* o Guido, na ordem de FICHAS */
static Model garfos;
static int temGarfos = 0;
static Matrix poseDosGarfos;             /* no carro normalizado; vale só para o próximo desenho */
static int temPoseDosGarfos = 0;
static Vector3 pontaDosGarfosNormal;     /* o meio das pás, em cima, no carro normalizado */
static int rodaEscondida = -1;           /* a roda que não está no carro no próximo desenho */
static int estouradosNoProximo = 0;      /* os pneus estourados no próximo desenho (bits, como Carro.pneuEstourado) */
static Matrix corpoInclinado;            /* a suspensão: só o corpo inclina, as rodas não */
static float aberturaDasRodas = 0.0f;    /* 0 a 1: as rodas da frente abertas (as mãos), no próximo desenho */
static int temCorpoInclinado = 0;

static unsigned char *translucidos[TOTAL_DE_MODELOS];   /* por material: 1 se tem algo translúcido */
static unsigned char *translucidosDosGarfos = NULL;
static int blocoDeSombras = 0;   /* 1: entre comecarSombrasNoChao e terminarSombrasNoChao */
static int rodaEmTroca = -1;          /* a roda que está sendo trocada no próximo carro desenhado (-1 = nenhuma) */
static float andamentoDaTroca = 0.0f;
static float ladosDaSombra[TOTAL_DE_MODELOS];

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
/* O bit da roda em Carro.pneuEstourado: 0 dianteiro esquerdo, 1 dianteiro direito, 2 traseiro esquerdo,
 * 3 traseiro direito (no carro normalizado, a frente é +z e a esquerda é +x) */
static int bitDaRoda(const RodaDoCarro *roda) {
    return 1 << (((roda->centro.z > 0.0f) ? 0 : 2) + ((roda->centro.x > 0.0f) ? 0 : 1));
}

static void desenharRodas(int numero, Model modelo, Matrix noMundo, float anguloDasRodas, float volante, int soTranslucidas,
                          const float *subida) {
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
        int estourado = (estouradosNoProximo & bitDaRoda(roda)) != 0;
        Matrix giro = MatrixRotateX(anguloDasRodas);
        if (roda->centro.z > 0.0f) {   /* a da frente vira com o volante e, abrindo as mãos, cada uma para o seu lado */
            float abre = (roda->centro.x > 0.0f) ? aberturaDasRodas : -aberturaDasRodas;   /* +x é a esquerda */
            giro = MatrixMultiply(giro, MatrixRotateY((-volante * GRAUS_DAS_RODAS_DA_FRENTE + abre * GRAUS_DAS_MAOS_ABERTAS) * DEG2RAD));
        }
        float y = roda->centro.y + ((subida != NULL) ? subida[peca->roda] : 0.0f);
        Matrix lugar = MatrixMultiply(giro, MatrixTranslate(roda->centro.x, y, roda->centro.z));
        Shader shader = modelo.materials[peca->material].shader;
        int corte = -1;
        if (estourado) {   /* sobra o aro: a borracha que é peça à parte nem vai, e o resto que passa do aro é cortado */
            corte = (shader.id == shaderOpaco.id) ? 0 : (shader.id == shaderTranslucido.id) ? 1 : -1;
            if (corte >= 0) {
                float raio = roda->raioDoAro * 1.01f;
                SetShaderValue(shader, locsDoCorte[corte], &raio, SHADER_UNIFORM_FLOAT);
            }
        }
        DrawMesh((estourado && peca->temAro) ? peca->aro : peca->malha, modelo.materials[peca->material], MatrixMultiply(lugar, noMundo));
        if (corte >= 0) {
            float semCorte = 0.0f;
            SetShaderValue(shader, locsDoCorte[corte], &semCorte, SHADER_UNIFORM_FLOAT);
        }
    }
}

/*
 * pneusEstouradosNoProximoDesenho
 * Os pneus que estouraram: no próximo desenho a borracha some e sobra o aro, e o carro baixa daquele canto
 * (o aro é menor que o pneu).
 * Recebe: os pneus, um bit para cada (como Carro.pneuEstourado).
 * Devolve: nada.
 */
void pneusEstouradosNoProximoDesenho(int pneus) {
    estouradosNoProximo = pneus;
}

/*
 * caimentoDoEstouro
 * Sem a borracha, a roda fica menor: o canto dela baixa o que o pneu tinha a mais que o aro. O carro, que é
 * rígido, inclina no plano que melhor passa pelos cantos (mínimos quadrados de altura = h + p·x + q·z), e o
 * que sobra em cada roda ela mesma sobe ou desce (a suspensão), para todas tocarem o chão.
 * Recebe: o número do carro e onde guardar quanto cada roda sobe.
 * Devolve: a matriz da inclinação, no carro normalizado.
 */
static Matrix caimentoDoEstouro(int numero, float *subida) {
    int n = totalDeRodas[numero];
    float desce[MAXIMO_DE_RODAS];
    double s1 = 0, sx = 0, sz = 0, sxx = 0, szz = 0, sxz = 0, sd = 0, sxd = 0, szd = 0;
    for (int r = 0; r < n; r++) {
        const RodaDoCarro *roda = &rodas[numero][r];
        desce[r] = (estouradosNoProximo & bitDaRoda(roda)) ? -(roda->raio - roda->raioDoAro) : 0.0f;
        double x = roda->centro.x, z = roda->centro.z, d = desce[r];
        s1 += 1; sx += x; sz += z; sxx += x * x; szz += z * z; sxz += x * z; sd += d; sxd += x * d; szd += z * d;
    }
    /* as equações normais (3 x 3), pela regra de Cramer */
    double a[3][3] = { { s1, sx, sz }, { sx, sxx, sxz }, { sz, sxz, szz } }, b[3] = { sd, sxd, szd };
    double det = a[0][0] * (a[1][1] * a[2][2] - a[1][2] * a[2][1]) - a[0][1] * (a[1][0] * a[2][2] - a[1][2] * a[2][0])
                 + a[0][2] * (a[1][0] * a[2][1] - a[1][1] * a[2][0]);
    float h = (n > 0) ? (float) (sd / s1) : 0.0f, p = 0.0f, q = 0.0f;
    if (fabs(det) > 1e-9) {
        double x[3];
        for (int k = 0; k < 3; k++) {
            double m[3][3];
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    m[i][j] = (j == k) ? b[i] : a[i][j];
                }
            }
            x[k] = (m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) - m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0])
                    + m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0])) / det;
        }
        h = (float) x[0];
        p = (float) x[1];
        q = (float) x[2];
    }
    for (int r = 0; r < n; r++) {
        subida[r] = desce[r] - (h + p * rodas[numero][r].centro.x + q * rodas[numero][r].centro.z);
    }
    /* (x, 0, 0) sobe p·x girando em z; (0, 0, z) sobe q·z girando em x para o outro lado */
    return MatrixMultiply(MatrixMultiply(MatrixRotateZ(asinf(Clamp(p, -0.5f, 0.5f))), MatrixRotateX(-asinf(Clamp(q, -0.5f, 0.5f)))),
                          MatrixTranslate(0.0f, h, 0.0f));
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
    float lado = comprimentoDaFicha(numero) * FOLGA_DA_SOMBRA;
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
 * posturaNoProximoDesenho
 * O corpo sobre as rodas, com a suspensão (como inclinarCorpoNoProximoDesenho) e os gestos
 * (gestos.c), como a Pixar fazia em Carros: o eixo de trás é o quadril, e o bico abaixa e sobe
 * girando em volta dele (o carro inteiro é a cabeça); o corpo deita de lado em volta do meio, na
 * altura dos eixos, e sobe ou desce sobre as rodas. As rodas ficam no chão. A arfagem da suspensão
 * gira no meio, como sempre: assim o carro do jogador não muda de giro quando os gestos entram.
 * Recebe: o carro, a rolagem (radianos, positivo: o teto vai para a esquerda), a arfagem da
 *         suspensão e o aceno do gesto (radianos, positivo: o bico desce) e a subida (metros).
 * Devolve: nada.
 */
void posturaNoProximoDesenho(int numero, float rolagem, float arfagem, float aceno, float subida) {
    float eixo = (totalDeRodas[numero] > 0) ? rodas[numero][0].centro.y : 0.3f;
    float quadril = -comprimentoDaFicha(numero) * 0.3f;   /* sem rodas achadas: mais ou menos ali */
    float frente = comprimentoDaFicha(numero) * 0.3f;
    for (int r = 0; r < totalDeRodas[numero]; r++) {
        quadril = (r == 0) ? rodas[numero][r].centro.z : fminf(quadril, rodas[numero][r].centro.z);
        frente = (r == 0) ? rodas[numero][r].centro.z : fmaxf(frente, rodas[numero][r].centro.z);
    }
    /* O bico não passa do curso da suspensão no eixo da frente: num carro comprido (o Ruivo), o mesmo
     * ângulo afundaria a frente nas rodas. O limite é macio, para o gesto não bater num muro */
    float limite = CURSO_DO_BICO / fmaxf(frente - quadril, 0.5f);
    aceno = limite * tanhf(aceno / limite);
    Matrix cabeca = MatrixMultiply(MatrixMultiply(MatrixTranslate(0.0f, -eixo, -quadril), MatrixRotateX(aceno)),
                                   MatrixTranslate(0.0f, eixo, quadril));
    Matrix suspensao = MatrixMultiply(MatrixMultiply(MatrixMultiply(MatrixTranslate(0.0f, -eixo, 0.0f), MatrixRotateX(arfagem)),
                                                     MatrixRotateZ(-rolagem)),
                                      MatrixTranslate(0.0f, eixo, 0.0f));
    corpoInclinado = MatrixMultiply(MatrixMultiply(cabeca, suspensao), MatrixTranslate(0.0f, subida, 0.0f));
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
 * emocaoNoProximoDesenho
 * A emoção da cara no próximo desenho (os moradores: contente, bravo, triste, curioso, com
 * sono), somada ao aperto e ao susto. A cara vai até lá com as molas; sem chamar, volta à
 * normal.
 * Recebe: o carro e a emoção (gestos.h).
 * Devolve: nada.
 */
void emocaoNoProximoDesenho(int numero, const Emocao *emocao) {
    caras[numero].emocao = *emocao;
}

/*
 * abrirRodasNoProximoDesenho
 * As rodas da frente como mãos abertas (os braços abertos de quem pergunta ou se empolga): cada uma vira
 * para o seu lado, por cima do volante, no próximo desenho.
 * Recebe: o quanto (0 a 1).
 * Devolve: nada.
 */
void abrirRodasNoProximoDesenho(float abertura) {
    aberturaDasRodas = fmaxf(0.0f, fminf(1.0f, abertura));
}

/*
 * bocaNoProximoDesenho
 * O quanto a boca do carro está aberta no próximo desenho (gestos.c: as vogais da fala). Sem chamar, ela
 * fica fechada. Só os modelos com a boca medida (bocas.txt) abrem.
 * Recebe: o carro e a abertura (0 fechada a 1 aberta).
 * Devolve: nada.
 */
void bocaNoProximoDesenho(int numero, float abertura) {
    aberturaNoProximo[numero] = fmaxf(0.0f, fminf(1.0f, abertura));
}

/*
 * olharNoProximoDesenho
 * Para onde os olhos do carro vão no próximo desenho, no lugar do volante (que continua virando as
 * rodas): um morador estacionado olhando para quem conversa com ele, ou para o jogador.
 * Recebe: o carro e o olhar (-1 todo à esquerda, +1 todo à direita).
 * Devolve: nada.
 */
void olharNoProximoDesenho(int numero, float olhar) {
    caras[numero].olharPedido = fmaxf(-1.0f, fminf(1.0f, olhar));
    caras[numero].temOlharPedido = 1;
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
    Emocao e = c->emocao;   /* o susto passa por cima de tudo */
    float semSusto = 1.0f - s;
    c->alvoAperto = 0.0f;
    c->alvoSusto = 0.0f;
    memset(&c->emocao, 0, sizeof(c->emocao));

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

    /* As emoções somam por cima do aperto (a cara de quem não está assustado) */
    float fechar = CACA_FECHA * a + SUSTO_FECHA * s
                   + semSusto * (ALEGRIA_FECHA * e.alegria + RAIVA_FECHA * e.raiva + TRISTEZA_FECHA * e.tristeza
                                 + ESPANTO_FECHA * e.espanto + SONO_FECHA * e.sono);
    float inclinar = CACA_INCLINA * a + SUSTO_INCLINA * s
                     + semSusto * (ALEGRIA_INCLINA * e.alegria + RAIVA_INCLINA * e.raiva + TRISTEZA_INCLINA * e.tristeza);
    float arco = SUSTO_ARCO * s + semSusto * (ALEGRIA_ARCO * e.alegria + RAIVA_ARCO * e.raiva + ESPANTO_ARCO * e.espanto);
    float iris = 1.0f + (CACA_IRIS - 1.0f) * a + (SUSTO_IRIS - 1.0f) * s
                 + semSusto * ((ALEGRIA_IRIS - 1.0f) * e.alegria + (RAIVA_IRIS - 1.0f) * e.raiva
                               + (TRISTEZA_IRIS - 1.0f) * e.tristeza + (ESPANTO_IRIS - 1.0f) * e.espanto);
    molar(&c->olharX, olhar * (1.0f - 0.3f * s) + c->olhadelaX * solto + tremor, MOLA_DO_OLHAR, tempo);
    molar(&c->olharY, SUSTO_OLHA_PARA_CIMA * s + semSusto * e.olharParaCima + c->olhadelaY * solto, MOLA_DO_OLHAR, tempo);
    molar(&c->fechar, fmaxf(MAIS_ABERTO, fminf(MAIS_FECHADO, fechar)), MOLA_DA_PALPEBRA, tempo);
    molar(&c->inclinar, fmaxf(-0.7f, fminf(0.7f, inclinar)), MOLA_DA_INCLINACAO, tempo);
    molar(&c->arco, fmaxf(-0.3f, fminf(0.5f, arco)), MOLA_DO_ARCO, tempo);
    molar(&c->iris, iris, MOLA_DA_IRIS, tempo);
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
    animarOlhos(numero, c->temOlharPedido ? c->olharPedido : volante, tempo);
    c->temOlharPedido = 0;

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

    /* Com pneu estourado, o carro baixa daquele canto (a roda vira só o aro) */
    float subida[MAXIMO_DE_RODAS] = { 0 };
    if (estouradosNoProximo) {
        noMundo = MatrixMultiply(caimentoDoEstouro(numero, subida), noMundo);
    }

    Model modelo = modelos[numero];
    Matrix corpo = temCorpoInclinado ? MatrixMultiply(ajuste[numero], corpoInclinado) : ajuste[numero];
    modelo.transform = MatrixMultiply(corpo, noMundo);
    Model doGuido = garfos;
    int comGarfos = (numero == CARRO_COM_GARFOS && temGarfos);
    if (comGarfos) {
        Matrix pose = temPoseDosGarfos ? MatrixMultiply(ajuste[numero], poseDosGarfos) : ajuste[numero];
        if (temCorpoInclinado) {
            pose = MatrixMultiply(pose, corpoInclinado);   /* os garfos vão com o corpo */
        }
        doGuido.transform = MatrixMultiply(pose, noMundo);
    }

    int materialDosOlhos = -1;
    if (olhos[numero].tem) {
        prepararOlhos(numero, volante);
        materialDosOlhos = olhos[numero].material;
    }
    SetShaderValue(shaderOpaco, locsDoReflexo[0], &reflexos[numero], SHADER_UNIFORM_FLOAT);
    SetShaderValue(shaderDosOlhos, locsDoReflexo[2], &reflexos[numero], SHADER_UNIFORM_FLOAT);
    /* A boca: a mandíbula gira em volta da dobradiça (só neste carro; depois volta a fechada) */
    Vector3 mandibula = { 0.0f, 0.0f, 0.0f };
    if (bocas[numero].tem) {
        mandibula = (Vector3){ bocas[numero].dobradicaY, bocas[numero].dobradicaZ,
                               aberturaNoProximo[numero] * bocas[numero].graus * DEG2RAD };
    }
    SetShaderValue(shaderOpaco, locsDaMandibula[0], &mandibula, SHADER_UNIFORM_VEC3);
    SetShaderValue(shaderTranslucido, locsDaMandibula[1], &mandibula, SHADER_UNIFORM_VEC3);
    desenharComShader(modelo, shaderOpaco, materialDosOlhos);
    if (comGarfos) {
        desenharComShader(doGuido, shaderOpaco, -1);
    }
    desenharRodas(numero, modelo, noMundo, anguloDasRodas, volante, 0, subida);
    float normal = 1.0f;   /* o shaderOpaco também desenha outras coisas, como a roda solta */
    SetShaderValue(shaderOpaco, locsDoReflexo[0], &normal, SHADER_UNIFORM_FLOAT);

    BeginBlendMode(BLEND_ALPHA);
    rlDisableDepthMask();
    desenharSoTranslucidos(modelo, shaderTranslucido, translucidos[numero]);
    if (comGarfos) {
        desenharSoTranslucidos(doGuido, shaderTranslucido, translucidosDosGarfos);
    }
    desenharRodas(numero, modelo, noMundo, anguloDasRodas, volante, 1, subida);
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
    EndBlendMode();
    rodaEmTroca = -1;
    rodaEscondida = -1;
    estouradosNoProximo = 0;
    temPoseDosGarfos = 0;
    temCorpoInclinado = 0;
    aberturaNoProximo[numero] = 0.0f;
    aberturaDasRodas = 0.0f;
    if (bocas[numero].tem) {   /* a miniatura e as rodas soltas, desenhadas depois com os mesmos shaders, de boca fechada */
        Vector3 fechada = { 0.0f, 0.0f, 0.0f };
        SetShaderValue(shaderOpaco, locsDaMandibula[0], &fechada, SHADER_UNIFORM_VEC3);
        SetShaderValue(shaderTranslucido, locsDaMandibula[1], &fechada, SHADER_UNIFORM_VEC3);
    }
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
    return comprimentoDaFicha(numero);
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
    int quartos = quartosDoModelo(numero);
    int deitado = quartos % 2;   /* a frente em x: o comprimento é o x do modelo */
    float escala = comprimentoDaFicha(numero) / (deitado ? dx : dz);

    Matrix centralizar = MatrixTranslate(-(caixa.min.x + caixa.max.x) / 2.0f, -caixa.min.y,
                                         -(caixa.min.z + caixa.max.z) / 2.0f);
    Matrix girar = MatrixRotateY(-90.0f * quartos * DEG2RAD);   /* a frente vai para +z */
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
            pecas[numero][totalDePecas[numero]++] = (PecaDeRoda){ .malha = peca, .temAro = 0, .material = modelo->meshMaterial[m], .roda = r };
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
 * separarAros
 * Em cada roda, o que é pneu e o que é aro, para o pneu estourado sumir e sobrar o aro.
 * Quando o pneu é uma peça à parte (um anel que vai até a banda e não chega no meio), ele sai inteiro, e o
 * aro vai até onde vão as outras peças. Quando a roda é um cilindro só, com o aro pintado na tampa, o raio do
 * aro vem da cor: de fora para dentro, as faixas de raio com a cor da borracha (a da banda) são do pneu, e a
 * primeira que não é marca onde o aro acaba (uma faixa só de outra cor, como as letras, não conta). Na hora
 * de desenhar, o que passa do raio do aro é cortado (o shader descarta).
 * Recebe: o número do carro.
 * Devolve: nada.
 */
#define FAIXAS_DO_RAIO        24
#define PERTO_DA_BORRACHA     0.16f    /* a distância de cor (0 a 1 em cada canal) que ainda é borracha */
#define BORRACHA_NA_FAIXA     0.50f    /* da área da faixa: mais que isso com a cor da borracha, a faixa é do pneu */
#define ANEL_DO_PNEU_DENTRO   0.35f    /* do raio: o anel do pneu não chega mais perto do eixo que isso... */
#define ANEL_DO_PNEU_FORA     0.86f    /* ...e vai pelo menos até aqui */
#define ARO_QUANDO_NAO_ACHA   0.62f    /* do raio, quando nem a forma nem a cor separam o aro */

typedef struct {
    Color *pixels;
    int largura, altura;
} PixelsDoMaterial;

/* A cor da roda num ponto de um triângulo (a textura do material, com a tinta dele) */
static Vector3 corNaRoda(const PixelsDoMaterial *imagem, Color tinta, float u, float v) {
    Vector3 cor = { tinta.r / 255.0f, tinta.g / 255.0f, tinta.b / 255.0f };
    if (imagem->pixels != NULL) {
        int x = (int) ((u - floorf(u)) * imagem->largura), y = (int) ((v - floorf(v)) * imagem->altura);
        x = (x < 0) ? 0 : (x >= imagem->largura) ? imagem->largura - 1 : x;
        y = (y < 0) ? 0 : (y >= imagem->altura) ? imagem->altura - 1 : y;
        Color px = imagem->pixels[y * imagem->largura + x];
        cor = (Vector3){ cor.x * px.r / 255.0f, cor.y * px.g / 255.0f, cor.z * px.b / 255.0f };
    }
    return cor;
}

static float Vector3Area(Vector3 a, Vector3 b, Vector3 c);   /* a área de um triângulo */

/*
 * amostrarRoda
 * Passa por toda a área dos triângulos de uma peça (vários pontos em cada um, para as tampas em leque, que vão
 * do eixo até a borda num triângulo só, contarem em cada raio) e soma, por faixa de raio, a área e a área com
 * a cor da borracha. Sem a cor da borracha ainda (borracha NULL), soma a cor da banda (de 0,88 do raio para fora).
 * Recebe: a peça, os triângulos que ficam de fora (NULL: nenhum), a imagem e a tinta do material, o raio da roda,
 *         a cor da borracha e onde somar.
 * Devolve: nada.
 */
static void amostrarRoda(Mesh m, const unsigned char *fora, const PixelsDoMaterial *imagem, Color tinta, float raio,
                         const Vector3 *borracha, float *area, float *comBorracha, Vector3 *corDaBanda, float *areaDaBanda) {
    for (int t = 0; t < m.vertexCount / 3; t++) {
        if (fora != NULL && fora[t]) {
            continue;
        }
        Vector3 p[3];
        Vector2 uv[3];
        for (int c = 0; c < 3; c++) {
            int k = t * 3 + c;
            p[c] = (Vector3){ m.vertices[k * 3], m.vertices[k * 3 + 1], m.vertices[k * 3 + 2] };
            uv[c] = (m.texcoords != NULL) ? (Vector2){ m.texcoords[k * 2], m.texcoords[k * 2 + 1] } : (Vector2){ 0.0f, 0.0f };
        }
        float tamanho = Vector3Area(p[0], p[1], p[2]);
        float maior = fmaxf(Vector3Distance(p[0], p[1]), fmaxf(Vector3Distance(p[1], p[2]), Vector3Distance(p[2], p[0])));
        int n = (int) Clamp(ceilf(maior / (raio / FAIXAS_DO_RAIO)), 1.0f, 10.0f);
        float peso = tamanho / (float) (n * (n + 1) / 2);
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n - i; j++) {
                float a = (i + 1.0f / 3.0f) / n, b = (j + 1.0f / 3.0f) / n, c = 1.0f - a - b;
                Vector3 ponto = Vector3Add(Vector3Add(Vector3Scale(p[0], a), Vector3Scale(p[1], b)), Vector3Scale(p[2], c));
                float u = uv[0].x * a + uv[1].x * b + uv[2].x * c, v = uv[0].y * a + uv[1].y * b + uv[2].y * c;
                Vector3 cor = corNaRoda(imagem, tinta, u, v);
                float noRaio = hypotf(ponto.y, ponto.z) / raio;
                if (borracha == NULL) {
                    if (noRaio > 0.88f) {
                        *corDaBanda = Vector3Add(*corDaBanda, Vector3Scale(cor, peso));
                        *areaDaBanda += peso;
                    }
                    continue;
                }
                int faixa = (int) Clamp(noRaio * FAIXAS_DO_RAIO, 0.0f, FAIXAS_DO_RAIO - 1.0f);
                area[faixa] += peso;
                if (Vector3Distance(cor, *borracha) < PERTO_DA_BORRACHA) {
                    comBorracha[faixa] += peso;
                }
            }
        }
    }
}

static void separarAros(int numero) {
    Model *modelo = &modelos[numero];
    PixelsDoMaterial *imagens = MemAlloc(modelo->materialCount * sizeof(PixelsDoMaterial));
    for (int m = 0; m < modelo->materialCount; m++) {
        Texture2D textura = modelo->materials[m].maps[MATERIAL_MAP_ALBEDO].texture;
        imagens[m] = (PixelsDoMaterial){ NULL, 0, 0 };
        int usada = 0;
        for (int i = 0; i < totalDePecas[numero]; i++) {
            usada |= pecas[numero][i].material == m;
        }
        if (usada && textura.id != 0 && textura.id != rlGetTextureIdDefault()) {
            Image imagem = LoadImageFromTexture(textura);
            imagens[m] = (PixelsDoMaterial){ LoadImageColors(imagem), imagem.width, imagem.height };
            UnloadImage(imagem);
        }
    }
    for (int r = 0; r < totalDeRodas[numero]; r++) {
        RodaDoCarro *roda = &rodas[numero][r];
        float raio = fmaxf(roda->raio, 0.01f);
        float bordaDoPneu = 2.0f, maiorAro = 0.0f;
        unsigned char *doAnel[MAXIMO_DE_PECAS_RODA * MAXIMO_DE_RODAS] = { 0 };

        /* 1. As ilhas de cada peça: o anel do pneu sai inteiro */
        for (int i = 0; i < totalDePecas[numero]; i++) {
            PecaDeRoda *peca = &pecas[numero][i];
            if (peca->roda != r) {
                continue;
            }
            Mesh m = peca->malha;
            int triangulos = m.vertexCount / 3;
            int *pai = MemAlloc(m.vertexCount * sizeof(int));
            float *menor = MemAlloc(m.vertexCount * sizeof(float)), *maior = MemAlloc(m.vertexCount * sizeof(float));
            int *naIlha = MemAlloc(m.vertexCount * sizeof(int));
            marcarIlhas(m, pai);
            for (int v = 0; v < m.vertexCount; v++) {
                menor[v] = 10.0f;
                maior[v] = 0.0f;
                naIlha[v] = 0;
            }
            for (int t = 0; t < triangulos; t++) {
                int raiz = acharRaiz(pai, t * 3);
                naIlha[raiz]++;
                for (int c = 0; c < 3; c++) {
                    int k = t * 3 + c;
                    float noRaio = hypotf(m.vertices[k * 3 + 1], m.vertices[k * 3 + 2]) / raio;
                    menor[raiz] = fminf(menor[raiz], noRaio);
                    maior[raiz] = fmaxf(maior[raiz], noRaio);
                }
            }
            doAnel[i] = MemAlloc(triangulos);
            int noAnel = 0;
            for (int t = 0; t < triangulos; t++) {
                int raiz = acharRaiz(pai, t * 3);
                doAnel[i][t] = menor[raiz] >= ANEL_DO_PNEU_DENTRO && maior[raiz] >= ANEL_DO_PNEU_FORA;
                noAnel += doAnel[i][t];
            }
            for (int v = 0; v < m.vertexCount; v++) {
                if (naIlha[v] == 0) {
                    continue;
                }
                if (menor[v] >= ANEL_DO_PNEU_DENTRO && maior[v] >= ANEL_DO_PNEU_FORA) {
                    bordaDoPneu = fminf(bordaDoPneu, menor[v]);
                } else if (naIlha[v] >= 8 && maior[v] >= 0.40f && maior[v] < 0.93f) {
                    maiorAro = fmaxf(maiorAro, maior[v]);
                }
            }
            peca->temAro = 0;
            if (noAnel > 0 && noAnel < triangulos) {
                Mesh soAro = malhaVazia(triangulos - noAnel);
                int feitos = 0;
                for (int t = 0; t < triangulos; t++) {
                    if (!doAnel[i][t]) {
                        copiarTriangulo(&soAro, m, t, MatrixIdentity(), feitos++);
                    }
                }
                UploadMesh(&soAro, false);
                peca->aro = soAro;
                peca->temAro = 1;
            }
            MemFree(pai);
            MemFree(menor);
            MemFree(maior);
            MemFree(naIlha);
        }

        /* 2. O raio do aro */
        float aro;
        if (bordaDoPneu < 1.5f) {
            aro = fmaxf(bordaDoPneu, maiorAro);   /* o aro pode passar um pouco da borda de dentro do pneu */
        } else {
            /* um cilindro só: a cor da banda é a da borracha, e de fora para dentro procura onde ela acaba */
            Vector3 corDaBanda = { 0 };
            float areaDaBanda = 0.0f, area[FAIXAS_DO_RAIO] = { 0 }, comBorracha[FAIXAS_DO_RAIO] = { 0 };
            for (int passada = 0; passada < 2; passada++) {
                Vector3 borracha = Vector3Scale(corDaBanda, 1.0f / fmaxf(areaDaBanda, 1e-9f));
                for (int i = 0; i < totalDePecas[numero]; i++) {
                    PecaDeRoda *peca = &pecas[numero][i];
                    if (peca->roda != r) {
                        continue;
                    }
                    amostrarRoda(peca->malha, doAnel[i], &imagens[peca->material], modelo->materials[peca->material].maps[MATERIAL_MAP_ALBEDO].color,
                                 raio, (passada == 0) ? NULL : &borracha, area, comBorracha, &corDaBanda, &areaDaBanda);
                }
            }
            int b = FAIXAS_DO_RAIO - 1;
            while (b > 0 && area[b] <= 0.0f) {
                b--;
            }
            int ultimaDoPneu = b + 1;
            for (int falhas = 0; b >= 0; b--) {
                if (area[b] <= 0.0f) {
                    continue;
                }
                if (comBorracha[b] >= BORRACHA_NA_FAIXA * area[b]) {
                    ultimaDoPneu = b;
                    falhas = 0;
                } else if (++falhas >= 2) {
                    break;
                }
            }
            aro = (float) ultimaDoPneu / FAIXAS_DO_RAIO;
            if (aro < 0.40f) {
                aro = (maiorAro > 0.0f) ? maiorAro : ARO_QUANDO_NAO_ACHA;   /* o aro também é escuro: a cor não separa */
            }
        }
        roda->raioDoAro = Clamp(aro, 0.40f, 0.90f) * raio;
        for (int i = 0; i < totalDePecas[numero]; i++) {
            MemFree(doAnel[i]);
        }
    }
    for (int m = 0; m < modelo->materialCount; m++) {
        if (imagens[m].pixels != NULL) {
            UnloadImageColors(imagens[m].pixels);
        }
    }
    MemFree(imagens);
}

static float Vector3Area(Vector3 a, Vector3 b, Vector3 c) {
    return 0.5f * Vector3Length(Vector3CrossProduct(Vector3Subtract(b, a), Vector3Subtract(c, a)));
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
        /* Uma textura sem canal alfa (RGB, cinza) tem alfa 255 em todo pixel: não há o que procurar, e ler a
         * imagem de volta da placa (o que custa) seria à toa */
        int semAlfa = (mapa.texture.format == PIXELFORMAT_UNCOMPRESSED_R8G8B8 || mapa.texture.format == PIXELFORMAT_UNCOMPRESSED_GRAYSCALE
                       || mapa.texture.format == PIXELFORMAT_UNCOMPRESSED_R5G6B5 || mapa.texture.format == PIXELFORMAT_UNCOMPRESSED_R32G32B32
                       || mapa.texture.format == PIXELFORMAT_UNCOMPRESSED_R16G16B16 || mapa.texture.format == PIXELFORMAT_UNCOMPRESSED_R32
                       || mapa.texture.format == PIXELFORMAT_UNCOMPRESSED_R16);
        if (!semAlfa && mapa.texture.id != 0 && mapa.texture.id != rlGetTextureIdDefault()) {
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
            SetTextureFilter(*textura, filtroDeTexturaDaConfig());
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
 * acharOlhosNaTabela
 * Procura um nome na tabela dos olhos (recursos/modelos/carros/olhos/olhos.txt).
 * Recebe: o texto da tabela (NULL se não há), o nome e onde pôr o que achou.
 * Devolve: o material dos olhos, no modelo da raylib (-1 se não achou).
 */
static int acharOlhosNaTabela(const char *tabela, const char *nome, OlhosDoCarro *achados) {
    if (tabela == NULL) {
        return -1;
    }
    for (const char *linha = tabela; *linha != '\0'; ) {
        char carro[64];
        int noGlb, ladoDaPalpebra;
        OlhosDoCarro lidos = { 0 };
        if (*linha != '#'
            && sscanf(linha, "%63s %d %f %d %f %f %f %f %f %f", carro, &noGlb, &lidos.anda, &ladoDaPalpebra,
                      &lidos.linha, &lidos.raio, &lidos.meios.x, &lidos.meios.y, &lidos.meios.z, &lidos.meios.w) == 10
            && strcmp(carro, nome) == 0) {
            lidos.ladoDaPalpebra = (float) ladoDaPalpebra;
            *achados = lidos;
            return noGlb + 1;   /* a raylib põe um material padrão antes dos do .glb */
        }
        const char *fim = strchr(linha, '\n');
        if (fim == NULL) {
            break;
        }
        linha = fim + 1;
    }
    return -1;
}

/*
 * trocarPelasCamadas
 * Troca a textura dos olhos do modelo pelas três camadas (recursos/modelos/carros/olhos/):
 * o fundo, as íris e a máscara do branco, e mais as bordas da pálpebra medidas na máscara.
 * Recebe: o modelo, o material dos olhos, o nome (das imagens) e o lado da pálpebra.
 * Devolve: 1 se trocou; 0 se faltou alguma imagem ou o tamanho não bate (o modelo fica como estava).
 */
static int trocarPelasCamadas(Model *modelo, int material, const char *nome, float ladoDaPalpebra) {
    if (material < 1 || material >= modelo->materialCount) {
        return 0;
    }
    static const char *CAMADAS[3] = { "fundo", "iris", "mascara" };
    char caminhos[3][256];
    for (int i = 0; i < 3; i++) {
        snprintf(caminhos[i], sizeof(caminhos[i]), "%s%s_%s.png", PASTA_DOS_OLHOS, nome, CAMADAS[i]);
        if (!FileExists(caminhos[i])) {
            TraceLog(LOG_WARNING, "CARRO: falta %s, os olhos de %s ficam parados", caminhos[i], nome);
            return 0;
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
        return 0;
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
    trocarTextura(olho, MATERIAL_MAP_ROUGHNESS, medirBordasDaPalpebra(caminhos[2], (int) ladoDaPalpebra));   /* o texture3 */
    return 1;
}

/*
 * carregarOlhos
 * Procura o carro na tabela dos olhos e troca a textura dos olhos dele pelas três camadas.
 * Sem a tabela ou sem as imagens, o carro fica com os olhos parados, como vieram.
 * Recebe: o número do carro e o texto da tabela (NULL se não há).
 * Devolve: nada.
 */
static void carregarOlhos(int numero, const char *tabela) {
    olhos[numero].tem = 0;
    char nome[64];
    strncpy(nome, GetFileNameWithoutExt(arquivoDoModelo(numero)), sizeof(nome) - 1);
    nome[sizeof(nome) - 1] = '\0';
    OlhosDoCarro achados = { 0 };
    int material = acharOlhosNaTabela(tabela, nome, &achados);
    if (!trocarPelasCamadas(&modelos[numero], material, nome, achados.ladoDaPalpebra)) {
        return;
    }
    achados.tem = 1;
    achados.material = material;
    olhos[numero] = achados;   /* o lado é medido depois, com o carro normalizado */
}

/*
 * medirLadoDosOlhosDoModelo
 * Para que lado da textura dos olhos fica a direita do modelo: compara, nos vértices da
 * peça dos olhos, a coordenada da textura com a posição de lado no modelo normalizado
 * (frente em +z e y para cima: a direita é o -x). E o mesmo na altura.
 * Recebe: o modelo, a matriz que o normaliza e os olhos dele (o material já achado).
 * Devolve: 1 se mediu (e põe o lado nos olhos); 0 se não deu.
 */
static int medirLadoDosOlhosDoModelo(Model modelo, Matrix ajusteDoModelo, OlhosDoCarro *o) {
    double somaX = 0.0, somaU = 0.0, somaXU = 0.0, somaXX = 0.0;
    double somaY = 0.0, somaV = 0.0, somaYV = 0.0;
    long total = 0;
    for (int i = 0; i < modelo.meshCount; i++) {
        Mesh malha = modelo.meshes[i];
        if (modelo.meshMaterial[i] != o->material || malha.texcoords == NULL) {
            continue;
        }
        for (int v = 0; v < malha.vertexCount; v++) {
            double x = verticeNormalizado(malha, v, ajusteDoModelo).x;
            double u = malha.texcoords[v * 2];
            double y = verticeNormalizado(malha, v, ajusteDoModelo).y;
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
        return 0;
    }
    o->lado = (junto > 0.0) ? -1.0f : 1.0f;   /* andar para -x é andar para a direita */
    o->ladoDeCima = (somaYV - somaY * somaV / total > 0.0) ? 1.0f : -1.0f;
    return 1;
}

/*
 * medirLadoDosOlhos
 * O lado dos olhos de um carro (já normalizado). Sem dar para medir, os olhos ficam parados.
 * Recebe: o número do carro.
 * Devolve: nada.
 */
static void medirLadoDosOlhos(int numero) {
    if (!olhos[numero].tem) {
        return;
    }
    if (!medirLadoDosOlhosDoModelo(modelos[numero], ajuste[numero], &olhos[numero])) {
        olhos[numero].tem = 0;
        TraceLog(LOG_WARNING, "CARRO: nao deu para medir os olhos de %s", arquivoDoModelo(numero));
        return;
    }
    TraceLog(LOG_INFO, "CARRO: olhos de %s no material %d, a direita fica no %s da textura",
             arquivoDoModelo(numero), olhos[numero].material, (olhos[numero].lado < 0.0f) ? "u menor" : "u maior");
}

/*
 * darOlhosAoModelo
 * Os olhos vivos dos carros num modelo que não é carro (o Mack): as mesmas três camadas, o mesmo
 * shader e a mesma cara (as molas, as piscadas e as olhadelas). O modelo já deve estar com a
 * frente em +z, y para cima e as rodas em y = 0; a linha dele na tabela dos olhos tem o nome dado.
 * Recebe: o modelo e o nome (na tabela e nas imagens).
 * Devolve: 1 se deu; 0 se não (os olhos ficam como vieram no modelo).
 */
int darOlhosAoModelo(Model *modelo, const char *nome) {
    olhos[CARA_DE_OUTRO_MODELO].tem = 0;
    char *tabela = FileExists(PASTA_DOS_OLHOS "olhos.txt") ? LoadFileText(PASTA_DOS_OLHOS "olhos.txt") : NULL;
    OlhosDoCarro achados = { 0 };
    int material = acharOlhosNaTabela(tabela, nome, &achados);
    if (tabela != NULL) {
        UnloadFileText(tabela);
    }
    if (!trocarPelasCamadas(modelo, material, nome, achados.ladoDaPalpebra)) {
        return 0;
    }
    achados.material = material;
    if (!medirLadoDosOlhosDoModelo(*modelo, MatrixIdentity(), &achados)) {
        TraceLog(LOG_WARNING, "CARRO: nao deu para medir os olhos de %s", nome);
        return 0;
    }
    Texture2D *fundo = &modelo->materials[material].maps[MATERIAL_MAP_ALBEDO].texture;
    GenTextureMipmaps(fundo);
    SetTextureFilter(*fundo, TEXTURE_FILTER_TRILINEAR);
    modelo->materials[material].shader = shaderDosOlhos;
    achados.tem = 1;
    olhos[CARA_DE_OUTRO_MODELO] = achados;
    memset(&caras[CARA_DE_OUTRO_MODELO], 0, sizeof(caras[CARA_DE_OUTRO_MODELO]));
    return 1;
}

/*
 * prepararOlhosDoModelo
 * A cara do modelo de darOlhosAoModelo no próximo desenho: para onde ele olha (como o volante
 * nos carros: -1 todo à esquerda, +1 todo à direita), o aperto e o susto. Chamar logo antes
 * de desenhá-lo.
 * Recebe: o olhar, o aperto e o susto (0 a 1).
 * Devolve: nada.
 */
void emocaoDoModelo(const Emocao *emocao) {
    caras[CARA_DE_OUTRO_MODELO].emocao = *emocao;
}

void prepararOlhosDoModelo(float olhar, float aperto, float susto) {
    if (!olhos[CARA_DE_OUTRO_MODELO].tem) {
        return;
    }
    expressaoNoProximoDesenho(CARA_DE_OUTRO_MODELO, aperto, susto);
    prepararOlhos(CARA_DE_OUTRO_MODELO, olhar);
    float reflexo = 0.6f;
    SetShaderValue(shaderDosOlhos, locsDoReflexo[2], &reflexo, SHADER_UNIFORM_FLOAT);
}

/* Os pontos da mandíbula de um modelo, ordenados pelo x, para achar cada vértice pela posição */
typedef struct {
    float x, y, z, peso;
} PontoDaBoca;

static int compararPontoDaBoca(const void *a, const void *b) {
    float d = ((const PontoDaBoca *) a)->x - ((const PontoDaBoca *) b)->x;
    return (d < 0.0f) ? -1 : (d > 0.0f) ? 1 : 0;
}

/* O peso do vértice na mandíbula (0 se ele não é dela): procura pelo x e confere o y e o z */
static float pesoNaBoca(const PontoDaBoca *pontos, int total, float x, float y, float z) {
    int a = 0, b = total;
    while (a < b) {   /* o primeiro ponto com x >= x - MESMO_PONTO */
        int meio = (a + b) / 2;
        if (pontos[meio].x < x - MESMO_PONTO) {
            a = meio + 1;
        } else {
            b = meio;
        }
    }
    for (int i = a; i < total && pontos[i].x <= x + MESMO_PONTO; i++) {
        if (fabsf(pontos[i].y - y) <= MESMO_PONTO && fabsf(pontos[i].z - z) <= MESMO_PONTO) {
            return pontos[i].peso;
        }
    }
    return 0.0f;
}

/* Põe os pesos numa malha que já está na placa de vídeo: no x do vertexTexCoord2 (o atributo 5) */
static void porPesosNaMalha(Mesh *malha, float *pesos) {
    int tamanho = malha->vertexCount * 2 * (int) sizeof(float);
    if (malha->texcoords2 != NULL) {   /* já tinha a segunda coordenada (nenhum carro tem): troca o conteúdo */
        memcpy(malha->texcoords2, pesos, (size_t) tamanho);
        RL_FREE(pesos);
        rlUpdateVertexBuffer(malha->vboId[RL_DEFAULT_SHADER_ATTRIB_LOCATION_TEXCOORD2], malha->texcoords2, tamanho, 0);
        return;
    }
    malha->texcoords2 = pesos;   /* a UnloadMesh libera junto */
    rlEnableVertexArray(malha->vaoId);
    malha->vboId[RL_DEFAULT_SHADER_ATTRIB_LOCATION_TEXCOORD2] = rlLoadVertexBuffer(malha->texcoords2, tamanho, false);
    rlSetVertexAttribute(RL_DEFAULT_SHADER_ATTRIB_LOCATION_TEXCOORD2, 2, RL_FLOAT, 0, 0, 0);
    rlEnableVertexAttribute(RL_DEFAULT_SHADER_ATTRIB_LOCATION_TEXCOORD2);
    rlDisableVertexArray();
}

/*
 * prepararBoca
 * A boca do modelo, da tabela das bocas: a dobradiça, o quanto abre e o peso de cada ponto da mandíbula
 * (o lábio de baixo, o queixo, os dentes de baixo e a língua; veja ferramentas/medir_bocas.py). Cada
 * vértice das malhas do modelo que está num ponto da tabela leva o peso dele.
 * Recebe: o carro e o texto da tabela (NULL: sem tabela).
 * Devolve: nada.
 */
/*
 * lerBoca
 * A linha de um modelo na tabela das bocas e os pontos dela, ordenados pelo x.
 * Recebe: a tabela, o nome do modelo e onde pôr a dobradiça, os graus e o total de pontos lidos.
 * Devolve: os pontos (RL_FREE depois), ou NULL se o modelo não está na tabela.
 */
static PontoDaBoca *lerBoca(const char *tabela, const char *nome, float *y, float *z, float *graus, int *lidos) {
    if (tabela == NULL) {
        return NULL;
    }
    char procura[80];
    snprintf(procura, sizeof(procura), "\nboca %s ", nome);
    const char *linha = strstr(tabela, procura);
    int total;
    if (linha == NULL || sscanf(linha + strlen(procura), "%f %f %f %d", y, z, graus, &total) != 4 || total <= 0) {
        return NULL;
    }
    PontoDaBoca *pontos = (PontoDaBoca *) RL_MALLOC((size_t) total * sizeof(PontoDaBoca));
    const char *p = strchr(linha + 1, '\n');
    *lidos = 0;
    while (p != NULL && *lidos < total) {
        PontoDaBoca q;
        if (sscanf(p + 1, "%f %f %f %f", &q.x, &q.y, &q.z, &q.peso) != 4) {
            break;
        }
        pontos[(*lidos)++] = q;
        p = strchr(p + 1, '\n');
    }
    qsort(pontos, (size_t) *lidos, sizeof(PontoDaBoca), compararPontoDaBoca);
    return pontos;
}

static void prepararBoca(int numero, const char *tabela) {
    bocas[numero].tem = 0;
    float y, z, graus;
    int lidos = 0;
    PontoDaBoca *pontos = lerBoca(tabela, GetFileNameWithoutExt(arquivoDoModelo(numero)), &y, &z, &graus, &lidos);
    if (pontos == NULL) {
        return;
    }
    int achados = 0;
    Model *modelo = &modelos[numero];
    for (int m = 0; m < modelo->meshCount; m++) {
        Mesh *malha = &modelo->meshes[m];
        float *pesos = NULL;
        for (int v = 0; v < malha->vertexCount; v++) {
            float w = pesoNaBoca(pontos, lidos, malha->vertices[v * 3], malha->vertices[v * 3 + 1], malha->vertices[v * 3 + 2]);
            if (w > 0.0f) {
                if (pesos == NULL) {
                    pesos = (float *) RL_CALLOC((size_t) malha->vertexCount * 2, sizeof(float));
                }
                pesos[v * 2] = w;
                achados++;
            }
        }
        if (pesos != NULL) {
            porPesosNaMalha(malha, pesos);
        }
    }
    RL_FREE(pontos);
    if (achados > 0) {
        bocas[numero] = (BocaDoCarro){ 1, y, z, graus };
    }
    TraceLog(LOG_INFO, "CARRO: %s: a boca tem %d vértices na mandíbula (%d pontos na tabela)", nomeDoModelo(numero), achados, lidos);
}

/* A boca de um modelo que não é carro (o Mack): ele não usa o shader dos carros, então a mandíbula gira
 * na própria malha (só quando a abertura muda) */
#define MAIS_MALHAS_COM_BOCA  4
typedef struct {
    int tem;
    float dobradicaY, dobradicaZ, graus, anguloAgora;
    Model *modelo;
    int malhas[MAIS_MALHAS_COM_BOCA], totalDeMalhas;
    int *vertices[MAIS_MALHAS_COM_BOCA];     /* os da mandíbula em cada malha... */
    float *pesos[MAIS_MALHAS_COM_BOCA];      /* ...o peso de cada um... */
    float *parados[MAIS_MALHAS_COM_BOCA];    /* ...e onde ficam de boca fechada: a posição e a normal (6 números) */
    int quantos[MAIS_MALHAS_COM_BOCA];
} BocaDeOutroModelo;
static BocaDeOutroModelo bocaDoOutro;

/*
 * darBocaAoModelo
 * A boca da tabela das bocas num modelo que não é carro (o Mack): os vértices da mandíbula em cada
 * malha, pela posição. Chamar depois de cortar as malhas do modelo, se for cortar.
 * Recebe: o modelo (fica guardado: tem de durar até descarregarBocaDoModelo) e o nome na tabela.
 * Devolve: 1 se deu; 0 se o modelo não está na tabela (a boca fica fechada).
 */
int darBocaAoModelo(Model *modelo, const char *nome) {
    descarregarBocaDoModelo();
    char *tabela = FileExists(ARQUIVO_DAS_BOCAS) ? LoadFileText(ARQUIVO_DAS_BOCAS) : NULL;
    float y, z, graus;
    int lidos = 0;
    PontoDaBoca *pontos = lerBoca(tabela, nome, &y, &z, &graus, &lidos);
    if (tabela != NULL) {
        UnloadFileText(tabela);
    }
    if (pontos == NULL) {
        return 0;
    }
    BocaDeOutroModelo *b = &bocaDoOutro;
    for (int m = 0; m < modelo->meshCount && b->totalDeMalhas < MAIS_MALHAS_COM_BOCA; m++) {
        Mesh *malha = &modelo->meshes[m];
        int k = b->totalDeMalhas, n = 0;
        for (int v = 0; v < malha->vertexCount; v++) {
            n += pesoNaBoca(pontos, lidos, malha->vertices[v * 3], malha->vertices[v * 3 + 1], malha->vertices[v * 3 + 2]) > 0.0f;
        }
        if (n == 0 || malha->normals == NULL) {
            continue;
        }
        b->vertices[k] = (int *) RL_MALLOC((size_t) n * sizeof(int));
        b->pesos[k] = (float *) RL_MALLOC((size_t) n * sizeof(float));
        b->parados[k] = (float *) RL_MALLOC((size_t) n * 6 * sizeof(float));
        int i = 0;
        for (int v = 0; v < malha->vertexCount; v++) {
            float w = pesoNaBoca(pontos, lidos, malha->vertices[v * 3], malha->vertices[v * 3 + 1], malha->vertices[v * 3 + 2]);
            if (w > 0.0f) {
                b->vertices[k][i] = v;
                b->pesos[k][i] = w;
                memcpy(&b->parados[k][i * 6], &malha->vertices[v * 3], 3 * sizeof(float));
                memcpy(&b->parados[k][i * 6 + 3], &malha->normals[v * 3], 3 * sizeof(float));
                i++;
            }
        }
        b->malhas[k] = m;
        b->quantos[k] = n;
        b->totalDeMalhas++;
    }
    RL_FREE(pontos);
    b->tem = b->totalDeMalhas > 0;
    b->modelo = modelo;
    b->dobradicaY = y;
    b->dobradicaZ = z;
    b->graus = graus;
    b->anguloAgora = 0.0f;
    TraceLog(LOG_INFO, "BOCA: %s: a mandíbula em %d malhas", nome, b->totalDeMalhas);
    return b->tem;
}

/*
 * abrirBocaDoModelo
 * Abre a boca do modelo de darBocaAoModelo: gira os vértices da mandíbula (e as normais) em volta da
 * dobradiça, o quanto ela abre vezes o peso de cada um, e manda a malha de novo para a placa de vídeo.
 * Recebe: a abertura (0 fechada a 1 aberta).
 * Devolve: nada.
 */
void abrirBocaDoModelo(float abertura) {
    BocaDeOutroModelo *b = &bocaDoOutro;
    if (!b->tem) {
        return;
    }
    float angulo = fmaxf(0.0f, fminf(1.0f, abertura)) * b->graus * DEG2RAD;
    if (fabsf(angulo - b->anguloAgora) < 0.002f) {
        return;   /* quase igual: não vale mandar a malha de novo */
    }
    b->anguloAgora = angulo;
    for (int k = 0; k < b->totalDeMalhas; k++) {
        Mesh *malha = &b->modelo->meshes[b->malhas[k]];
        for (int i = 0; i < b->quantos[k]; i++) {
            const float *p = &b->parados[k][i * 6];
            float a = angulo * b->pesos[k][i], c = cosf(a), s = sinf(a);
            float dy = p[1] - b->dobradicaY, dz = p[2] - b->dobradicaZ;
            int v = b->vertices[k][i];
            malha->vertices[v * 3 + 1] = b->dobradicaY + dy * c - dz * s;
            malha->vertices[v * 3 + 2] = b->dobradicaZ + dy * s + dz * c;
            malha->normals[v * 3 + 1] = p[4] * c - p[5] * s;
            malha->normals[v * 3 + 2] = p[4] * s + p[5] * c;
        }
        UpdateMeshBuffer(*malha, 0, malha->vertices, malha->vertexCount * 3 * (int) sizeof(float), 0);
        UpdateMeshBuffer(*malha, 2, malha->normals, malha->vertexCount * 3 * (int) sizeof(float), 0);
    }
}

void descarregarBocaDoModelo(void) {
    BocaDeOutroModelo *b = &bocaDoOutro;
    for (int k = 0; k < b->totalDeMalhas; k++) {
        RL_FREE(b->vertices[k]);
        RL_FREE(b->pesos[k]);
        RL_FREE(b->parados[k]);
    }
    memset(b, 0, sizeof(*b));
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
    registrarShaderDoDia(shaderOpaco);
    registrarShaderDoDia(shaderTranslucido);
    registrarShaderDoDia(shaderDosOlhos);
    Shader comReflexo[3] = { shaderOpaco, shaderTranslucido, shaderDosOlhos };
    float reflexoNormal = 1.0f;   /* sem isto o uniform começa em 0: nenhum carro refletiria */
    for (int i = 0; i < 3; i++) {
        locsDoReflexo[i] = GetShaderLocation(comReflexo[i], "reflexoDaPintura");
        if (i < 2) {
            float semCorte = 0.0f;
            locsDoCorte[i] = GetShaderLocation(comReflexo[i], "corteDoAro");
            SetShaderValue(comReflexo[i], locsDoCorte[i], &semCorte, SHADER_UNIFORM_FLOAT);
        }
        locsDoOlho[i] = GetShaderLocation(comReflexo[i], "olho");
        if (i < 2) {
            locsDaMandibula[i] = GetShaderLocation(comReflexo[i], "mandibula");
        }
        SetShaderValue(comReflexo[i], locsDoReflexo[i], &reflexoNormal, SHADER_UNIFORM_FLOAT);
    }
    char *tabelaDosOlhos = FileExists(PASTA_DOS_OLHOS "olhos.txt") ? LoadFileText(PASTA_DOS_OLHOS "olhos.txt") : NULL;
    char *tabelaDasBocas = FileExists(ARQUIVO_DAS_BOCAS) ? LoadFileText(ARQUIVO_DAS_BOCAS) : NULL;
    shaderDaSombra = LoadShaderFromMemory(VERTICES_DO_SHADER, PIXELS_DA_SOMBRA_TEXTO("0.82"));

    for (int numero = 0; numero < TOTAL_DE_MODELOS; numero++) {
        passoDaEspera("CARREGANDO CARROS", (float) numero / TOTAL_DE_MODELOS);
        modelos[numero] = LoadModel(TextFormat("%s%s", PASTA_DOS_MODELOS, arquivoDoModelo(numero)));
        reflexos[numero] = 1.0f;   /* todos refletem o céu do mesmo jeito (o Mack, nos olhos, menos) */
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
        float escala = comprimentoDaFicha(numero) / fmaxf(caixa.max.x - caixa.min.x, caixa.max.z - caixa.min.z);
        medirCaixaDoCarro(numero);
        acharRodas(numero, larguraDaFicha(numero), (caixa.max.y - caixa.min.y) * escala);
        separarRodas(numero);
        separarAros(numero);
        prepararBoca(numero, tabelaDasBocas);   /* depois das rodas: o corpo já é a malha que fica */
        TraceLog(LOG_INFO, "CARRO: %s tem %d rodas, em %d pecas (o aro vai até %.0f%% do raio)", nomeDoModelo(numero),
                 totalDeRodas[numero], totalDePecas[numero],
                 (totalDeRodas[numero] > 0) ? 100.0f * rodas[numero][0].raioDoAro / fmaxf(rodas[numero][0].raio, 0.01f) : 0.0f);
        if (numero < TOTAL_CARROS) {
            prepararMiniatura(numero);   /* a garagem só mostra os carros da corrida */
        }
    }
    if (tabelaDosOlhos != NULL) {
        UnloadFileText(tabelaDosOlhos);
    }
    if (tabelaDasBocas != NULL) {
        UnloadFileText(tabelaDasBocas);
    }
}

/*
 * descarregarCarros3D
 * Devolve a memória de vídeo dos carros. Chamar antes de CloseWindow.
 * Recebe: nada.
 * Devolve: nada.
 */
void descarregarCarros3D(void) {
    for (int numero = 0; numero < TOTAL_DE_MODELOS; numero++) {
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
            if (pecas[numero][i].temAro) {
                UnloadMesh(pecas[numero][i].aro);
            }
        }
        if (numero < TOTAL_CARROS) {
            UnloadRenderTexture(miniaturas[numero]);
        }
        UnloadTexture(sombras[numero]);
    }
    UnloadShader(shaderOpaco);
    UnloadShader(shaderTranslucido);
    UnloadShader(shaderDosOlhos);
    UnloadShader(shaderDaSombra);
}
