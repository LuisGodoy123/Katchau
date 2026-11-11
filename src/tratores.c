/*
 * tratores.c
 * ------------------------------------------------------------------
 * Os tratores-vaca (veja tratores.h).
 *
 * Cada trator é desenhado em quatro partes, todas no mesmo referencial do
 * modelo (em metros, a frente para +z, o chão em y = 0): o corpo, os olhos e
 * os dois pares de rodas, que giram em volta do eixo deles. Um shader só, com
 * a luz da hora (hora.h), faz também a pálpebra: nos olhos, o que fica acima
 * da beira dela (no referencial do modelo) leva a cor da lataria, e a beira
 * desce de y_cima (aberto) a y_baixo (fechado), com a linha dos cílios.
 *
 * A cambalhota: o trator gira para trás em volta da beira de baixo da
 * traseira (o pé das rodas grandes), a frente subindo, como uma vaca que
 * empina e cai de costas; a cada instante ele sobe o que for preciso para o
 * ponto mais baixo da caixa dele ficar no chão, então ele "rola" por cima da
 * traseira e cai deitado de costas, quica e fica lá, as rodas girando no ar.
 * ------------------------------------------------------------------
 */

#include "tratores.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "raymath.h"
#include "rlgl.h"
#include "hora.h"
#include "fumaca.h"
#include "leitor_json.h"

#define PASTA_DO_TRATOR       "recursos/modelos/trator"
#define ARQUIVO_DOS_LUGARES   "recursos/modelos/tratores.json"
#define MAIS_TRATORES         32
#define DESENHA_ATE           300.0f    /* metros */
#define DORME_COM             0.6f      /* da noite (luzDoDia->luzesDaCidade): daí para cima eles dormem... */
#define ACORDA_COM            0.3f      /* ...e daí para baixo acordam */
#define OUVE_ATE              16.0f     /* metros: o ronco de um carro parado mais perto que isso acorda */
#define RONCO_QUE_ASSUSTA     0.7f      /* do CarroLivre.roncando */
#define SUSTO_DO_VIZINHO      9.0f      /* metros: um que tomba assusta os que dormem mais perto que isso */
#define SEGUNDOS_DO_SUSTO     0.45f     /* o pulinho, de olho arregalado */
#define ALTURA_DO_PULO        0.35f
#define SEGUNDOS_TOMBANDO     1.05f
#define DE_COSTAS             (PI * 0.97f)
#define QUIQUE                0.16f     /* radianos que ele passa e volta, ao bater de costas */
#define DE_COSTAS_DE          14.0f     /* segundos deitado... */
#define DE_COSTAS_ATE         24.0f
#define LONGE_PARA_DESVIRAR   14.0f     /* metros: só desvira com o carro longe (ou depois de muito tempo) */
#define DESVIRA_DE_QUALQUER_JEITO 45.0f
#define SEGUNDOS_DESVIRANDO   1.8f
#define FECHA_O_OLHO          0.7f      /* por segundo, adormecendo */
#define MUGIDOS               3         /* vozes ao mesmo tempo */
#define OUVE_O_MUGIDO_ATE     80.0f
#define TAXA_DO_MUGIDO        22050

typedef enum { PASTANDO, DORMINDO, ASSUSTADO, TOMBANDO, TOMBADO, DESVIRANDO } EstadoDoTrator;

typedef struct {
    float x, y, z, rumo;          /* no chão */
    EstadoDoTrator estado;
    float tempo;                  /* segundos no estado */
    float palpebra;               /* 0 aberta, 1 fechada */
    float piscar;                 /* segundos até a próxima piscada (de dia) */
    float susto;                  /* > 0: segundos até ele se assustar (o ronco ou o vizinho) */
    float angulo;                 /* para trás: 0 em pé, DE_COSTAS deitado de costas */
    float anguloNoComeco;         /* desvirando: de onde ele saiu */
    float pulo;                   /* metros acima do chão */
    float giroDasRodas;           /* radianos (as de trás; as da frente giram mais) */
    float voltaDasRodas;          /* rad/s */
    float fumaca;                 /* segundos até a próxima baforada */
    float mugir;                  /* segundos até o próximo mugido, deitado */
    float deCostasPor;            /* quanto tempo ele fica deitado */
    unsigned int acaso;
} Trator;

typedef struct {
    float comprimento, largura, altura;
    Vector3 eixoTras, eixoFrente;   /* x 0; y e z do eixo */
    float raioTras, raioFrente;
    float traseiraZ, frenteZ;
    Vector3 escapamento;
    float olhoCima, olhoBaixo;
    Vector3 corDaPalpebra;
} MedidasDoTrator;

static Model corpo, olhos, rodasTras, rodasFrente;
static int temCorpo = 0, temOlhos = 0, temRodasTras = 0, temRodasFrente = 0;
static MedidasDoTrator medidas;
static BoundingBox caixaDoModelo;
static Trator tratores[MAIS_TRATORES];
static int total = 0;
static Shader shader;
static int locPalpebra, locEhOlho, locAlturaDosOlhos, locCorDaPalpebra;
static int carregados = 0;
static Sound mugido, vozes[MUGIDOS];
static int temMugido = 0, proximaVoz = 0;
static Texture2D sombra;
static Vector3 jogadorAgora;

static const char *VERTICES_DO_TRATOR =
    "#version 330\n"
    "in vec3 vertexPosition; in vec2 vertexTexCoord; in vec3 vertexNormal;\n"
    "uniform mat4 mvp; uniform mat4 matModel; uniform mat4 matNormal;\n"
    "out vec2 fragTexCoord; out vec3 fragNormal; out vec3 fragPos; out vec3 noModelo;\n"
    "void main() { fragTexCoord = vertexTexCoord;\n"
    "              fragNormal = normalize(vec3(matNormal * vec4(vertexNormal, 0.0)));\n"
    "              fragPos = vec3(matModel * vec4(vertexPosition, 1.0));\n"
    "              noModelo = vertexPosition;\n"
    "              gl_Position = mvp * vec4(vertexPosition, 1.0); }\n";

static const char *PIXELS_DO_TRATOR =
    "#version 330\n"
    "in vec2 fragTexCoord; in vec3 fragNormal; in vec3 fragPos; in vec3 noModelo;\n"
    "uniform sampler2D texture0; uniform vec4 colDiffuse;\n"
    "uniform float palpebra; uniform float ehOlho; uniform vec2 alturaDosOlhos; uniform vec3 corDaPalpebra;\n"
    GLSL_DO_SOL GLSL_DA_LUZ_DO_DIA GLSL_DAS_LAMPADAS
    "out vec4 finalColor;\n"
    "void main() {\n"
    "    vec4 cor = texture(texture0, fragTexCoord) * colDiffuse;\n"
    "    if (cor.a < 0.3) discard;\n"
    "    vec3 n = normalize(fragNormal);\n"
    "    if (!gl_FrontFacing) n = -n;\n"
    "    if (ehOlho > 0.5) {\n"
    "        float alto = alturaDosOlhos.x - alturaDosOlhos.y;\n"
    "        float beira = alturaDosOlhos.x - palpebra * alto;\n"
    "        if (noModelo.y > beira) cor.rgb = corDaPalpebra;\n"
    "        else if (palpebra > 0.03 && noModelo.y > beira - 0.06 * alto) cor.rgb *= 0.2;\n"
    "    }\n"
    "    float sol = max(dot(n, solDoDia), 0.0);\n"
    "    vec3 luz = 0.62 * luzDeBase + 0.28 * (n.y * 0.5 + 0.5) * luzDoCeu + 0.42 * sol * luzDoSol + luzDasLampadas(fragPos, n);\n"
    "    finalColor = vec4(cor.rgb * luz, 1.0);\n"
    "}\n";

/* ------------------------------------------------------------------
 * Acaso de cada trator (o mesmo a cada vez que o jogo abre)
 * ------------------------------------------------------------------ */

static float sortear(Trator *t, float de, float ate) {
    t->acaso = t->acaso * 1664525u + 1013904223u;
    return de + (ate - de) * (float) ((t->acaso >> 8) & 0xFFFF) / 65535.0f;
}

static float suave(float t) {
    t = Clamp(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

/* ------------------------------------------------------------------
 * O mugido (gerado aqui: um som de vogal "mmmuuu", grave, que sobe e desce)
 * ------------------------------------------------------------------ */

static Sound gerarMugido(void) {
    float duracao = 1.5f;
    int amostras = (int) (TAXA_DO_MUGIDO * duracao);
    short *dados = (short *) MemAlloc((unsigned int) amostras * sizeof(short));
    float *som = (float *) calloc((size_t) amostras, sizeof(float));
    double fase = 0.0;
    float maior = 0.0001f;
    unsigned int semente = 1977;
    for (int i = 0; i < amostras; i++) {
        float t = (float) i / TAXA_DO_MUGIDO;
        float f0 = (t < 0.2f) ? 128.0f + 22.0f * (t / 0.2f)
                 : (t < 0.85f) ? 150.0f - 10.0f * (t - 0.2f) / 0.65f
                 : 140.0f - 35.0f * (t - 0.85f) / 0.65f;
        f0 *= 1.0f + 0.015f * sinf(2.0f * PI * 5.5f * t);   /* o tremido da voz */
        fase += 2.0 * PI * f0 / TAXA_DO_MUGIDO;
        float abre = suave(t / 0.3f);                       /* da boca fechada ("mm") para aberta ("uu") */
        float f1 = 300.0f + 300.0f * abre, f2 = 850.0f + 200.0f * abre;
        float s = 0.0f;
        for (int k = 1; k <= 30; k++) {
            float fk = k * f0;
            if (fk > 4500.0f) {
                break;
            }
            float a1 = (fk - f1) / 170.0f, a2 = (fk - f2) / 240.0f;
            float a = (expf(-a1 * a1) + 0.55f * expf(-a2 * a2) + 0.06f) / (float) k;
            s += a * sinf((float) (k * fase));
        }
        semente = semente * 1664525u + 1013904223u;
        float sopro = ((float) ((semente >> 9) & 0x3FFF) / 8192.0f - 1.0f) * 0.03f * (1.0f - abre);
        float envelope = suave(t / 0.1f) * (1.0f - suave((t - 1.05f) / 0.45f));
        som[i] = (s + sopro) * envelope;
        maior = fmaxf(maior, fabsf(som[i]));
    }
    for (int i = 0; i < amostras; i++) {
        dados[i] = (short) (som[i] / maior * 0.8f * 32000.0f);
    }
    free(som);
    Wave onda = { (unsigned int) amostras, TAXA_DO_MUGIDO, 16, 1, dados };
    Sound resultado = LoadSoundFromWave(onda);
    UnloadWave(onda);
    return resultado;
}

static void mugir(const Trator *t, float forca) {
    if (!temMugido) {
        return;
    }
    float distancia = Vector3Distance(jogadorAgora, (Vector3){ t->x, t->y, t->z });
    float volume = Clamp(1.0f - distancia / OUVE_O_MUGIDO_ATE, 0.0f, 1.0f);
    if (volume <= 0.01f) {
        return;
    }
    Sound *voz = &vozes[proximaVoz];
    proximaVoz = (proximaVoz + 1) % MUGIDOS;
    SetSoundVolume(*voz, volume * volume * forca * 0.9f);
    SetSoundPitch(*voz, 0.85f + 0.3f * (float) ((t->acaso >> 4) & 0xFF) / 255.0f);
    PlaySound(*voz);
}

/* ------------------------------------------------------------------
 * Carregar
 * ------------------------------------------------------------------ */

static int carregarParte(Model *modelo, const char *nome) {
    char caminho[256];
    snprintf(caminho, sizeof(caminho), "%s/%s", PASTA_DO_TRATOR, nome);
    if (!FileExists(caminho)) {
        return 0;
    }
    *modelo = LoadModel(caminho);
    if (modelo->meshCount <= 0) {
        return 0;
    }
    for (int i = 0; i < modelo->materialCount; i++) {
        modelo->materials[i].shader = shader;
        Texture2D *t = &modelo->materials[i].maps[MATERIAL_MAP_ALBEDO].texture;
        if (t->id != 0 && t->id != rlGetTextureIdDefault()) {
            GenTextureMipmaps(t);
            SetTextureFilter(*t, (t->mipmaps > 1) ? TEXTURE_FILTER_TRILINEAR : TEXTURE_FILTER_BILINEAR);
        }
    }
    return 1;
}

static void lerMedidas(void) {
    /* o que vale se faltar o arquivo: um trator de 3,2 m */
    medidas = (MedidasDoTrator){ 3.2f, 2.0f, 2.5f, { 0.0f, 0.65f, -0.7f }, { 0.0f, 0.3f, 1.1f }, 0.65f, 0.3f,
                                 -1.6f, 1.6f, { 0.3f, 2.4f, 0.6f }, 2.3f, 1.9f, { 0.85f, 0.62f, 0.60f } };
    char caminho[256];
    snprintf(caminho, sizeof(caminho), "%s/trator.json", PASTA_DO_TRATOR);
    if (!FileExists(caminho)) {
        return;
    }
    char *texto = LoadFileText(caminho);
    if (texto == NULL) {
        return;
    }
    const char *fim = texto + strlen(texto);
    medidas.comprimento = numeroDaChave(texto, fim, "comprimento", medidas.comprimento);
    medidas.largura = numeroDaChave(texto, fim, "largura", medidas.largura);
    medidas.altura = numeroDaChave(texto, fim, "altura", medidas.altura);
    medidas.traseiraZ = numeroDaChave(texto, fim, "traseira_z", medidas.traseiraZ);
    medidas.frenteZ = numeroDaChave(texto, fim, "frente_z", medidas.frenteZ);
    const char *p;
    if ((p = strstr(texto, "\"eixo_tras\"")) != NULL) {
        const char *f = strchr(p, '}');
        medidas.eixoTras.y = numeroDaChave(p, f, "y", medidas.eixoTras.y);
        medidas.eixoTras.z = numeroDaChave(p, f, "z", medidas.eixoTras.z);
        medidas.raioTras = numeroDaChave(p, f, "raio", medidas.raioTras);
    }
    if ((p = strstr(texto, "\"eixo_frente\"")) != NULL) {
        const char *f = strchr(p, '}');
        medidas.eixoFrente.y = numeroDaChave(p, f, "y", medidas.eixoFrente.y);
        medidas.eixoFrente.z = numeroDaChave(p, f, "z", medidas.eixoFrente.z);
        medidas.raioFrente = numeroDaChave(p, f, "raio", medidas.raioFrente);
    }
    if ((p = strstr(texto, "\"escapamento\"")) != NULL) {
        const char *f = strchr(p, '}');
        medidas.escapamento = (Vector3){ numeroDaChave(p, f, "x", medidas.escapamento.x), numeroDaChave(p, f, "y", medidas.escapamento.y),
                                         numeroDaChave(p, f, "z", medidas.escapamento.z) };
    }
    if ((p = strstr(texto, "\"olhos\"")) != NULL) {
        const char *f = strchr(p, '}');
        medidas.olhoCima = numeroDaChave(p, f, "y_cima", medidas.olhoCima);
        medidas.olhoBaixo = numeroDaChave(p, f, "y_baixo", medidas.olhoBaixo);
        float cor[3];
        if (numerosDaLista(p, f, "cor_da_palpebra", cor, 3) == 3) {
            medidas.corDaPalpebra = (Vector3){ cor[0], cor[1], cor[2] };
        }
    }
    UnloadFileText(texto);
}

static void lerLugares(void) {
    total = 0;
    if (!FileExists(ARQUIVO_DOS_LUGARES)) {
        return;
    }
    char *texto = LoadFileText(ARQUIVO_DOS_LUGARES);
    if (texto == NULL) {
        return;
    }
    const char *cursor = NULL, *abre, *fecha;
    while (total < MAIS_TRATORES && proximoObjeto(texto, "tratores", &cursor, &abre, &fecha)) {
        Trator *t = &tratores[total];
        memset(t, 0, sizeof(*t));
        t->x = numeroDaChave(abre, fecha, "x", 0.0f);
        t->y = numeroDaChave(abre, fecha, "y", 0.0f);
        t->z = numeroDaChave(abre, fecha, "z", 0.0f);
        t->rumo = numeroDaChave(abre, fecha, "rumo", 0.0f);
        t->acaso = 2654435761u * (unsigned int) (total + 11);
        t->estado = PASTANDO;
        t->piscar = sortear(t, 1.0f, 5.0f);
        total++;
    }
    UnloadFileText(texto);
}

static Texture2D gerarSombra(void) {
    Image imagem = GenImageColor(64, 64, BLANK);
    for (int y = 0; y < 64; y++) {
        for (int x = 0; x < 64; x++) {
            float dx = (x + 0.5f) / 32.0f - 1.0f, dy = (y + 0.5f) / 32.0f - 1.0f;
            float a = Clamp(1.0f - sqrtf(dx * dx + dy * dy), 0.0f, 1.0f);
            ImageDrawPixel(&imagem, x, y, (Color){ 0, 0, 0, (unsigned char) (suave(a * 1.6f) * 255.0f) });
        }
    }
    Texture2D textura = LoadTextureFromImage(imagem);
    SetTextureFilter(textura, TEXTURE_FILTER_BILINEAR);
    UnloadImage(imagem);
    return textura;
}

void carregarTratores(void) {
    if (carregados) {
        return;
    }
    shader = LoadShaderFromMemory(VERTICES_DO_TRATOR, PIXELS_DO_TRATOR);
    locPalpebra = GetShaderLocation(shader, "palpebra");
    locEhOlho = GetShaderLocation(shader, "ehOlho");
    locAlturaDosOlhos = GetShaderLocation(shader, "alturaDosOlhos");
    locCorDaPalpebra = GetShaderLocation(shader, "corDaPalpebra");
    registrarShaderDoDia(shader);
    temCorpo = carregarParte(&corpo, "corpo.glb");
    temOlhos = carregarParte(&olhos, "olhos.glb");
    temRodasTras = carregarParte(&rodasTras, "rodas_tras.glb");
    temRodasFrente = carregarParte(&rodasFrente, "rodas_frente.glb");
    carregados = 1;
    if (!temCorpo) {
        TraceLog(LOG_INFO, "TRATORES: sem %s/corpo.glb (gere com ferramentas/preparar_trator.py)", PASTA_DO_TRATOR);
        return;
    }
    lerMedidas();
    caixaDoModelo = GetModelBoundingBox(corpo);
    if (temRodasTras) {
        BoundingBox r = GetModelBoundingBox(rodasTras);
        caixaDoModelo.min = Vector3Min(caixaDoModelo.min, r.min);
        caixaDoModelo.max = Vector3Max(caixaDoModelo.max, r.max);
    }
    if (temRodasFrente) {
        BoundingBox r = GetModelBoundingBox(rodasFrente);
        caixaDoModelo.min = Vector3Min(caixaDoModelo.min, r.min);
        caixaDoModelo.max = Vector3Max(caixaDoModelo.max, r.max);
    }
    lerLugares();
    mugido = gerarMugido();
    temMugido = mugido.frameCount > 0;
    for (int i = 0; i < MUGIDOS && temMugido; i++) {
        vozes[i] = LoadSoundAlias(mugido);
    }
    sombra = gerarSombra();
    TraceLog(LOG_INFO, "TRATORES: %d no pasto", total);
}

void descarregarTratores(void) {
    if (!carregados) {
        return;
    }
    Model *partes[4] = { &corpo, &olhos, &rodasTras, &rodasFrente };
    int tem[4] = { temCorpo, temOlhos, temRodasTras, temRodasFrente };
    for (int k = 0; k < 4; k++) {
        if (!tem[k]) {
            continue;
        }
        for (int i = 0; i < partes[k]->materialCount; i++) {
            partes[k]->materials[i].shader.id = rlGetShaderIdDefault();
        }
        UnloadModel(*partes[k]);
    }
    if (temCorpo) {
        for (int i = 0; i < MUGIDOS && temMugido; i++) {
            UnloadSoundAlias(vozes[i]);
        }
        if (temMugido) {
            UnloadSound(mugido);
        }
        UnloadTexture(sombra);
    }
    UnloadShader(shader);
    temCorpo = temOlhos = temRodasTras = temRodasFrente = 0;
    temMugido = 0;
    total = 0;
    carregados = 0;
}

/* ------------------------------------------------------------------
 * A pose (a cambalhota) e as matrizes
 * ------------------------------------------------------------------ */

/* A volta para trás em volta do pé da traseira, sem a subida */
static Matrix girarParaTras(float angulo) {
    float py = caixaDoModelo.min.y, pz = medidas.traseiraZ;
    return MatrixMultiply(MatrixMultiply(MatrixTranslate(0.0f, -py, -pz), MatrixRotateX(-angulo)), MatrixTranslate(0.0f, py, pz));
}

/* Os 8 cantos da caixa do modelo, girados: o mais baixo e a pegada (para o chão e a colisão) */
static void caixaGirada(float angulo, float *maisBaixo, float *maisAlto, float *zDe, float *zAte) {
    Matrix giro = girarParaTras(angulo);
    *maisBaixo = 1e9f;
    *maisAlto = -1e9f;
    *zDe = 1e9f;
    *zAte = -1e9f;
    for (int k = 0; k < 8; k++) {
        Vector3 canto = { (k & 1) ? caixaDoModelo.max.x : caixaDoModelo.min.x, (k & 2) ? caixaDoModelo.max.y : caixaDoModelo.min.y,
                          (k & 4) ? caixaDoModelo.max.z : caixaDoModelo.min.z };
        Vector3 c = Vector3Transform(canto, giro);
        *maisBaixo = fminf(*maisBaixo, c.y);
        *maisAlto = fmaxf(*maisAlto, c.y);
        *zDe = fminf(*zDe, c.z);
        *zAte = fmaxf(*zAte, c.z);
    }
}

static Matrix noLugar(const Trator *t) {
    return MatrixMultiply(MatrixRotateY(PI - t->rumo), MatrixTranslate(t->x, t->y, t->z));
}

/* A matriz do trator no mundo: girado para trás, subido para não entrar no chão, no lugar dele */
static Matrix matrizDoTrator(const Trator *t) {
    float maisBaixo, maisAlto, zDe, zAte;
    caixaGirada(t->angulo, &maisBaixo, &maisAlto, &zDe, &zAte);
    float subida = (caixaDoModelo.min.y - maisBaixo) + t->pulo;
    return MatrixMultiply(MatrixMultiply(girarParaTras(t->angulo), MatrixTranslate(0.0f, subida, 0.0f)), noLugar(t));
}

static Matrix matrizDaRoda(Matrix doTrator, Vector3 eixo, float giro) {
    Matrix roda = MatrixMultiply(MatrixMultiply(MatrixTranslate(0.0f, -eixo.y, -eixo.z), MatrixRotateX(giro)),
                                 MatrixTranslate(0.0f, eixo.y, eixo.z));
    return MatrixMultiply(roda, doTrator);
}

/* ------------------------------------------------------------------
 * A vida no pasto
 * ------------------------------------------------------------------ */

static void assustar(Trator *t, float daqui) {
    if (t->estado != DORMINDO || t->susto > 0.0f) {
        return;
    }
    t->susto = daqui;
}

static void soltarBaforada(const Trator *t, float forca) {
    Vector3 boca = Vector3Transform(medidas.escapamento, matrizDoTrator(t));
    soltarNuvemColorida(boca, (Vector3){ 0.0f, 2.2f, 0.0f }, forca, NUVEM_DE_PNEU, (Color){ 120, 122, 132, 255 });
}

static void atualizarUm(Trator *t, float noite, float roncando, float dt) {
    t->tempo += dt;
    float distancia = hypotf(jogadorAgora.x - t->x, jogadorAgora.z - t->z);
    float alvoDaPalpebra = 0.0f;
    switch (t->estado) {
        case PASTANDO:
            /* de dia: pisca de vez em quando */
            t->piscar -= dt;
            if (t->piscar < 0.0f) {
                alvoDaPalpebra = 1.0f;
                if (t->piscar < -0.14f) {
                    t->piscar = sortear(t, 2.0f, 6.0f);
                }
            }
            t->angulo = 0.006f * sinf(t->tempo * 0.9f + (float) (t->acaso & 15));   /* respirando */
            /* cada um pega no sono numa hora (uns antes, outros depois) */
            if (noite >= DORME_COM + 0.35f * (float) (t->acaso & 255) / 255.0f) {
                t->estado = DORMINDO;
                t->tempo = 0.0f;
            }
            break;
        case DORMINDO:
            alvoDaPalpebra = 1.0f;
            t->angulo = 0.012f * sinf(t->tempo * 1.3f + (float) (t->acaso & 15));   /* o sono pesado */
            if (roncando >= RONCO_QUE_ASSUSTA && distancia < OUVE_ATE) {
                assustar(t, 0.1f + 0.03f * distancia + sortear(t, 0.0f, 0.25f));
            }
            if (t->susto > 0.0f) {
                t->susto -= dt;
                if (t->susto <= 0.0f) {
                    t->susto = 0.0f;
                    t->estado = ASSUSTADO;
                    t->tempo = 0.0f;
                    t->palpebra = 0.0f;   /* o olho abre de uma vez */
                    mugir(t, 1.0f);
                    /* o susto passa para os vizinhos que dormem */
                    for (int i = 0; i < total; i++) {
                        Trator *v = &tratores[i];
                        float d = hypotf(v->x - t->x, v->z - t->z);
                        if (v != t && d < SUSTO_DO_VIZINHO) {
                            assustar(v, 0.35f + 0.05f * d + sortear(v, 0.0f, 0.3f));
                        }
                    }
                }
            } else if (noite <= ACORDA_COM - 0.25f * (float) (t->acaso & 255) / 255.0f) {
                t->estado = PASTANDO;
                t->tempo = 0.0f;
            }
            break;
        case ASSUSTADO: {
            /* o pulinho, de olho arregalado */
            float k = t->tempo / SEGUNDOS_DO_SUSTO;
            t->pulo = ALTURA_DO_PULO * sinf(PI * Clamp(k, 0.0f, 1.0f));
            t->angulo = 0.12f * sinf(PI * Clamp(k, 0.0f, 1.0f));   /* a frente já começa a subir */
            if (k >= 1.0f) {
                t->estado = TOMBANDO;
                t->tempo = 0.0f;
                t->pulo = 0.0f;
                t->voltaDasRodas = 9.0f;
                soltarBaforada(t, 0.9f);
            }
            break;
        }
        case TOMBANDO: {
            /* a frente sobe devagar e cai depressa, como quem perde o equilíbrio */
            float k = Clamp(t->tempo / SEGUNDOS_TOMBANDO, 0.0f, 1.0f);
            t->angulo = 0.12f + (DE_COSTAS - 0.12f) * k * k;
            if (k >= 1.0f) {
                t->estado = TOMBADO;
                t->tempo = 0.0f;
                t->deCostasPor = sortear(t, DE_COSTAS_DE, DE_COSTAS_ATE);
                t->fumaca = 0.2f;
                t->mugir = sortear(t, 3.0f, 7.0f);
                soltarBaforada(t, 1.0f);
            }
            break;
        }
        case TOMBADO:
            /* de costas: quica e fica, as rodas girando no ar, soltando fumaça e mugindo */
            t->angulo = DE_COSTAS + QUIQUE * expf(-t->tempo * 4.0f) * sinf(t->tempo * 13.0f);
            t->pulo = 0.12f * expf(-t->tempo * 5.0f) * fabsf(sinf(t->tempo * 11.0f));
            t->piscar -= dt;
            if (t->piscar < 0.0f) {
                alvoDaPalpebra = 1.0f;
                if (t->piscar < -0.14f) {
                    t->piscar = sortear(t, 1.5f, 4.0f);
                }
            }
            t->fumaca -= dt;
            if (t->fumaca <= 0.0f) {
                soltarBaforada(t, 0.55f);
                t->fumaca = sortear(t, 0.6f, 1.3f);
            }
            t->mugir -= dt;
            if (t->mugir <= 0.0f) {
                mugir(t, 0.7f);
                t->mugir = sortear(t, 6.0f, 11.0f);
            }
            if ((t->tempo > t->deCostasPor && distancia > LONGE_PARA_DESVIRAR) || t->tempo > DESVIRA_DE_QUALQUER_JEITO) {
                t->estado = DESVIRANDO;
                t->tempo = 0.0f;
                t->anguloNoComeco = t->angulo;
                t->voltaDasRodas = -5.0f;
            }
            break;
        case DESVIRANDO: {
            /* rola de volta (devagar no começo, firme no fim) e assenta com um tranquinho */
            float k = Clamp(t->tempo / SEGUNDOS_DESVIRANDO, 0.0f, 1.0f);
            t->angulo = t->anguloNoComeco * (1.0f - suave(k));
            t->pulo = (k > 0.85f) ? 0.1f * sinf(PI * (k - 0.85f) / 0.15f) : 0.0f;
            if (k >= 1.0f) {
                t->angulo = 0.0f;
                t->pulo = 0.0f;
                t->estado = (noite >= DORME_COM) ? DORMINDO : PASTANDO;
                t->tempo = 0.0f;
            }
            break;
        }
    }
    /* as pálpebras: adormecendo, fecham devagar; piscando e acordando, depressa */
    float passo = (alvoDaPalpebra > t->palpebra && t->estado == DORMINDO) ? FECHA_O_OLHO * dt : 9.0f * dt;
    t->palpebra += Clamp(alvoDaPalpebra - t->palpebra, -passo, passo);
    /* as rodas: giram com o tranco e param aos poucos */
    t->giroDasRodas += t->voltaDasRodas * dt;
    t->voltaDasRodas *= expf(-dt * ((t->estado == TOMBADO) ? 0.35f : 2.5f));
}

void atualizarTratores(float noite, Vector3 jogador, float roncando, float tempoQuadro) {
    if (!temCorpo) {
        return;
    }
    jogadorAgora = jogador;
    for (int i = 0; i < total; i++) {
        atualizarUm(&tratores[i], noite, roncando, tempoQuadro);
    }
}

/* ------------------------------------------------------------------
 * Desenhar
 * ------------------------------------------------------------------ */

static void desenharParte(const Model *modelo, Matrix matriz) {
    for (int i = 0; i < modelo->meshCount; i++) {
        DrawMesh(modelo->meshes[i], modelo->materials[modelo->meshMaterial[i]], matriz);
    }
}

static void desenharSombra(const Trator *t, float meioZ, float meioComprimento) {
    Matrix lugar = noLugar(t);
    float meiaLargura = medidas.largura * 0.5f + 0.35f;
    meioComprimento += 0.35f;
    Vector3 cantos[4] = {
        Vector3Transform((Vector3){ -meiaLargura, 0.04f, meioZ - meioComprimento }, lugar),
        Vector3Transform((Vector3){ meiaLargura, 0.04f, meioZ - meioComprimento }, lugar),
        Vector3Transform((Vector3){ meiaLargura, 0.04f, meioZ + meioComprimento }, lugar),
        Vector3Transform((Vector3){ -meiaLargura, 0.04f, meioZ + meioComprimento }, lugar),
    };
    rlSetTexture(sombra.id);
    rlBegin(RL_QUADS);
    rlColor4ub(255, 255, 255, 150);
    rlTexCoord2f(0.0f, 0.0f); rlVertex3f(cantos[0].x, cantos[0].y, cantos[0].z);
    rlTexCoord2f(0.0f, 1.0f); rlVertex3f(cantos[3].x, cantos[3].y, cantos[3].z);
    rlTexCoord2f(1.0f, 1.0f); rlVertex3f(cantos[2].x, cantos[2].y, cantos[2].z);
    rlTexCoord2f(1.0f, 0.0f); rlVertex3f(cantos[1].x, cantos[1].y, cantos[1].z);
    rlEnd();
    rlSetTexture(0);
}

void desenharTratores(Camera3D camera) {
    if (!temCorpo || total == 0) {
        return;
    }
    Vector3 olhar = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    Vector2 alturaDosOlhos = { medidas.olhoCima, medidas.olhoBaixo };
    SetShaderValue(shader, locAlturaDosOlhos, &alturaDosOlhos, SHADER_UNIFORM_VEC2);
    SetShaderValue(shader, locCorDaPalpebra, &medidas.corDaPalpebra, SHADER_UNIFORM_VEC3);
    /* as sombras no chão primeiro (por baixo de tudo, sem escrever na profundidade) */
    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    for (int i = 0; i < total; i++) {
        const Trator *t = &tratores[i];
        Vector3 onde = { t->x, t->y + 1.0f, t->z };
        if (Vector3Distance(onde, camera.position) > DESENHA_ATE * 0.4f) {
            continue;
        }
        float maisBaixo, maisAlto, zDe, zAte;
        caixaGirada(t->angulo, &maisBaixo, &maisAlto, &zDe, &zAte);
        desenharSombra(t, (zDe + zAte) * 0.5f, (zAte - zDe) * 0.5f);
    }
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
    for (int i = 0; i < total; i++) {
        const Trator *t = &tratores[i];
        Vector3 onde = { t->x, t->y + 1.0f, t->z };
        Vector3 ate = Vector3Subtract(onde, camera.position);
        float distancia = Vector3Length(ate);
        if (distancia > DESENHA_ATE || (distancia > 15.0f && Vector3DotProduct(ate, olhar) < -0.2f * distancia)) {
            continue;
        }
        Matrix doTrator = matrizDoTrator(t);
        float nao = 0.0f, sim = 1.0f;
        SetShaderValue(shader, locEhOlho, &nao, SHADER_UNIFORM_FLOAT);
        desenharParte(&corpo, doTrator);
        if (temRodasTras) {
            desenharParte(&rodasTras, matrizDaRoda(doTrator, medidas.eixoTras, t->giroDasRodas));
        }
        if (temRodasFrente) {
            float giroDaFrente = t->giroDasRodas * medidas.raioTras / fmaxf(medidas.raioFrente, 0.05f);
            desenharParte(&rodasFrente, matrizDaRoda(doTrator, medidas.eixoFrente, giroDaFrente));
        }
        if (temOlhos) {
            SetShaderValue(shader, locEhOlho, &sim, SHADER_UNIFORM_FLOAT);
            SetShaderValue(shader, locPalpebra, &t->palpebra, SHADER_UNIFORM_FLOAT);
            desenharParte(&olhos, doTrator);
        }
    }
}

/* ------------------------------------------------------------------
 * Colisão
 * ------------------------------------------------------------------ */

int totalDeTratores(void) {
    return temCorpo ? total : 0;
}

int caixaDoTrator(int i, float *x, float *z, float *rumo, float *meioComprimento, float *meiaLargura, float *alto) {
    if (!temCorpo || i < 0 || i >= total) {
        return 0;
    }
    const Trator *t = &tratores[i];
    float maisBaixo, maisAlto, zDe, zAte;
    caixaGirada(t->angulo, &maisBaixo, &maisAlto, &zDe, &zAte);
    Vector3 meio = Vector3Transform((Vector3){ 0.0f, 0.0f, (zDe + zAte) * 0.5f }, noLugar(t));
    *x = meio.x;
    *z = meio.z;
    *rumo = t->rumo;
    *meioComprimento = (zAte - zDe) * 0.5f;
    *meiaLargura = (caixaDoModelo.max.x - caixaDoModelo.min.x) * 0.5f;
    *alto = t->y + (maisAlto - maisBaixo) + t->pulo;
    return 1;
}
