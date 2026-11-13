/*
 * luzes.c
 * ------------------------------------------------------------------
 * As luzes da cidade à noite (veja luzes.h).
 *
 * Cada lugar é um "conjunto": o modelo com os tubos e as lâmpadas
 * (recursos/modelos/luzes/<lugar>.glb) e as luzes pontuais em volta
 * (<lugar>.json: "acender_em" e a lista "luzes"). Na cor de cada vértice, o
 * RGB é a cor acesa e o alfa é a ordem de acender (0 primeiro, 1 por
 * último). Quando a hora pede as luzes, a cidade liga: cada lugar começa
 * "acender_em" segundos depois e leva DURACAO_DO_ACENDER para acender
 * inteiro, uma parte depois da outra, e cada parte pisca um pouco antes de
 * firmar. No amanhecer apagam, de trás para a frente. O semaforo.glb pisca
 * sempre, dia e noite.
 *
 * Uma malha com textura é a cópia de uma superfície pintada do mapa que
 * brilha de noite (o domo do Fillmore): ela só aparece acesa, por cima do
 * mapa, e de dia some (o mapa aparece como é).
 *
 * Há também os conjuntos que andam (registrarLuzesMoveis): as lâmpadas do
 * Mack, que vão com ele, e o lado de dentro da carreta, sempre aceso. O
 * jogo diz a cada quadro onde eles estão (moverLuzes); as luzes pontuais
 * deles não têm a vista medida (iluminam tudo em volta).
 *
 * O brilho em volta (o halo do pós-processo) é desenhado numa imagem que usa
 * a profundidade da cena (posprocesso.c): o que está na frente de uma luz (um
 * prédio, um carro, o Mack) a tapa.
 *
 * E a luz pontual de cada lâmpada não atravessa as paredes: ao carregar, de
 * cada lâmpada saem raios na malha de colisão em VISTA_LARGURA × VISTA_ALTURA
 * direções, medindo até onde ela chega em cada uma. Isso vai para uma imagem
 * (um quadro por lâmpada), e o shader só ilumina o ponto que a lâmpada
 * enxerga (hora.h, GLSL_DAS_LAMPADAS).
 * ------------------------------------------------------------------
 */

#include "luzes.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "raymath.h"
#include "rlgl.h"
#include "leitor_json.h"
#include "malhas.h"

#define PASTA_DAS_LUZES       "recursos/modelos/luzes"
#define MAIS_CONJUNTOS        24
#define MAIS_LAMPADAS_DO_LUGAR 48
#define LIMIAR_DE_ACENDER     0.35f     /* da luz da hora: a partir daí a cidade liga */
#define DURACAO_DO_ACENDER    5.0f      /* segundos para um lugar acender inteiro */
#define DURACAO_DO_APAGAR     2.5f
#define FIM_DO_ACENDER        1.15f     /* um pouco além de 1: a última parte firma */
#define DESENHA_ATE           1600.0f   /* metros: a cidade inteira, de qualquer ponto dela */
#define HALO_ATE              1200.0f
#define VISTA_LARGURA         64        /* direções em volta de cada lâmpada (latitude e longitude)... */
#define VISTA_ALTURA          32
#define QUADROS_POR_LINHA     16        /* ...e quantas lâmpadas por linha da imagem das vistas */
#define AFASTA_DA_LAMPADA     0.05f     /* metros: o raio começa um pouco fora da lâmpada */
#define LADO_DO_BRILHO        64        /* pixels da imagem do borrão */

typedef struct {
    char nome[48];
    Model modelo;
    int temModelo;
    BoundingBox caixa;
    float acenderEm;
    float acesa;                  /* o andamento do acender (0 apagado, FIM_DO_ACENDER aceso) */
    int pisca;
    int interior;                 /* só as luzes pontuais de um interior: acendem junto com a noite */
    int movel;                    /* anda (o Mack): a matriz diz onde ele está */
    int sempreAceso;              /* aceso de dia e de noite (o lado de dentro da carreta) */
    int visivel;                  /* 0: não desenha nem ilumina (longe, ou escondido) */
    Matrix matriz;                /* do modelo para o mundo (a identidade nos que não andam) */
    Lampada lampadas[MAIS_LAMPADAS_DO_LUGAR];
    int totalDeLampadas;
} Conjunto;

/* As luzes pontuais que os interiores jogam para fora, à noite (ferramentas montar_*: --noite) */
static const char *INTERIORES_DA_NOITE[] = {
    "recursos/modelos/loja_do_luigi_noite.json",
    "recursos/modelos/cozy_cone_quartos_noite.json",
    "recursos/modelos/cozy_cone_recepcao_noite.json",
};

static Conjunto conjuntos[MAIS_CONJUNTOS];
static int totalDeConjuntos = 0;
static Shader shaderDasLuzes;
static int locAcesa, locBrilho, locTempo, locPisca, locLuzApagada, locHalo, locCopia;
static int carregadas = 0;
static int ligadas = 0;
static float desdeQueMudou = 0.0f;
static float acesasAgora = 0.0f;
static float tempo = 0.0f;
static Texture2D imagemDoBrilho;
static Texture2D vistaDasLampadas;   /* até onde cada lâmpada enxerga, em cada direção (metros, R32) */
static int temVista = 0;

static const char *VERTICES_DAS_LUZES =
    "#version 330\n"
    "in vec3 vertexPosition; in vec2 vertexTexCoord; in vec4 vertexColor;\n"
    "uniform mat4 mvp; uniform mat4 matModel;\n"
    "out vec2 fragTexCoord; out vec4 fragColor; out vec3 fragPos;\n"
    "void main() { fragTexCoord = vertexTexCoord; fragColor = vertexColor;\n"
    "              fragPos = vec3(matModel * vec4(vertexPosition, 1.0));\n"
    "              gl_Position = mvp * vec4(vertexPosition, 1.0); }\n";

/* acesa: o andamento do acender do lugar; cada vértice liga quando ele passa da ordem dele (o alfa),
 * piscando enquanto está perto dela. halo: só a parte acesa (o passe do brilho) */
static const char *PIXELS_DAS_LUZES =
    "#version 330\n"
    "in vec2 fragTexCoord; in vec4 fragColor; in vec3 fragPos;\n"
    "uniform sampler2D texture0; uniform vec4 colDiffuse;\n"
    "uniform float acesa; uniform float brilho; uniform float tempo; uniform float pisca; uniform float halo;\n"
    "uniform float copia;\n"
    "uniform vec3 luzApagada;\n"
    "out vec4 finalColor;\n"
    "float acaso(float a, float b) { return fract(sin(a * 127.1 + b * 311.7) * 43758.5453); }\n"
    "void main() {\n"
    "    vec3 cor = texture(texture0, fragTexCoord).rgb * colDiffuse.rgb * fragColor.rgb;\n"
    "    float ordem = fragColor.a;\n"
    "    float depois = acesa - ordem;\n"
    "    float ligada = clamp(depois * 12.0, 0.0, 1.0);\n"
    "    if (depois > 0.0 && depois < 0.10) ligada *= step(0.4, acaso(floor(tempo * 14.0), ordem * 97.0));\n"
    "    if (pisca > 0.5) ligada = step(0.45, fract(tempo * 0.75));\n"
    "    vec3 acesoDeVerdade = (cor + vec3(max(cor.r, max(cor.g, cor.b)) * 0.18)) * brilho;\n"   /* o miolo mais branco; o escuro (fios) fica escuro */
    "    if (halo > 0.5) { finalColor = vec4(acesoDeVerdade * ligada, 1.0); return; }\n"
    "    if (copia > 0.5) { if (ligada < 0.01) discard; finalColor = vec4(acesoDeVerdade, ligada); return; }\n"
    "    vec3 n = normalize(cross(dFdx(fragPos), dFdy(fragPos)));\n"
    "    vec3 apagado = cor * 0.32 * luzApagada * (0.7 + 0.3 * abs(n.y));\n"
    "    finalColor = vec4(mix(apagado, acesoDeVerdade, ligada), 1.0);\n"
    "}\n";

/* ------------------------------------------------------------------
 * O JSON das luzes (um leitor pequeno: só os números das chaves que importam)
 * ------------------------------------------------------------------ */


/*
 * lerJson
 * Lê "acender_em" e a lista "luzes" (cada uma com x, y, z, r, g, b, raio e forca).
 * Recebe: o caminho e o conjunto. Devolve: 1 se o arquivo existe.
 */
static int lerJson(const char *caminho, Conjunto *c) {
    if (!FileExists(caminho)) {
        return 0;
    }
    char *texto = LoadFileText(caminho);
    if (texto == NULL) {
        return 0;
    }
    const char *fim = texto + strlen(texto);
    const char *lista = strstr(texto, "\"luzes\"");
    c->acenderEm = numeroDaChave(texto, lista != NULL ? lista : fim, "acender_em", 0.0f);
    if (lista != NULL) {
        const char *p = strchr(lista, '[');
        while (p != NULL && p < fim && c->totalDeLampadas < MAIS_LAMPADAS_DO_LUGAR) {
            const char *abre = strchr(p, '{');
            const char *fecha = (abre != NULL) ? strchr(abre, '}') : NULL;
            const char *fimDaLista = strchr(p, ']');
            if (abre == NULL || fecha == NULL || (fimDaLista != NULL && abre > fimDaLista)) {
                break;
            }
            Lampada *l = &c->lampadas[c->totalDeLampadas++];
            l->lugar = (Vector3){ numeroDaChave(abre, fecha, "x", 0.0f), numeroDaChave(abre, fecha, "y", 0.0f),
                                  numeroDaChave(abre, fecha, "z", 0.0f) };
            l->cor = (Vector3){ numeroDaChave(abre, fecha, "r", 1.0f), numeroDaChave(abre, fecha, "g", 1.0f),
                                numeroDaChave(abre, fecha, "b", 1.0f) };
            l->raio = numeroDaChave(abre, fecha, "raio", 10.0f);
            l->forca = numeroDaChave(abre, fecha, "forca", 1.0f);
            l->quadro = -1;   /* sem a vista medida: ilumina tudo em volta */
            p = fecha + 1;
        }
    }
    UnloadFileText(texto);
    return 1;
}

/* ------------------------------------------------------------------
 * Carregar
 * ------------------------------------------------------------------ */

static Texture2D gerarImagemDoBrilho(void) {
    Image imagem = GenImageColor(LADO_DO_BRILHO, LADO_DO_BRILHO, BLANK);
    for (int y = 0; y < LADO_DO_BRILHO; y++) {
        for (int x = 0; x < LADO_DO_BRILHO; x++) {
            float dx = (x + 0.5f) / LADO_DO_BRILHO * 2.0f - 1.0f, dy = (y + 0.5f) / LADO_DO_BRILHO * 2.0f - 1.0f;
            float d = sqrtf(dx * dx + dy * dy);
            float a = Clamp(1.0f - d, 0.0f, 1.0f);
            a = a * a * (0.35f + 0.65f * a * a) + expf(-d * d * 60.0f) * 0.6f;   /* o miolo forte e a borda macia */
            ImageDrawPixel(&imagem, x, y, (Color){ 255, 255, 255, (unsigned char) (Clamp(a, 0.0f, 1.0f) * 255.0f) });
        }
    }
    Texture2D textura = LoadTextureFromImage(imagem);
    SetTextureFilter(textura, TEXTURE_FILTER_BILINEAR);
    UnloadImage(imagem);
    return textura;
}

static void carregarModeloDoConjunto(Conjunto *c, const char *caminho) {
    c->modelo = LoadModel(caminho);
    if (c->modelo.meshCount <= 0) {
        return;
    }
    c->temModelo = 1;
    c->caixa = GetModelBoundingBox(c->modelo);
    for (int i = 0; i < c->modelo.materialCount; i++) {
        c->modelo.materials[i].shader = shaderDasLuzes;
        Texture2D *t = &c->modelo.materials[i].maps[MATERIAL_MAP_ALBEDO].texture;
        if (t->id != 0 && t->id != rlGetTextureIdDefault()) {
            GenTextureMipmaps(t);
            SetTextureFilter(*t, TEXTURE_FILTER_TRILINEAR);
        }
    }
}

/*
 * medirVistas
 * Para cada lâmpada, até onde ela enxerga em cada direção (raios na malha de colisão, até o raio dela):
 * um quadro de VISTA_LARGURA × VISTA_ALTURA na imagem vistaDasLampadas. A direção do texel (i, j) tem a
 * longitude (i + 0,5) / VISTA_LARGURA da volta (atan(z, x)) e a distância do zênite (j + 0,5) / VISTA_ALTURA
 * de meia volta (acos(y)), a mesma conta do shader.
 * Recebe: a malha de colisão.
 * Devolve: nada (marca o quadro de cada lâmpada).
 */
static void medirVistas(MalhaDeColisao *malha) {
    int total = 0;
    for (int i = 0; i < totalDeConjuntos; i++) {
        total += conjuntos[i].totalDeLampadas;
    }
    if (malha == NULL || total == 0) {
        return;
    }
    int linhas = (total + QUADROS_POR_LINHA - 1) / QUADROS_POR_LINHA;
    int largura = QUADROS_POR_LINHA * VISTA_LARGURA, altura = linhas * VISTA_ALTURA;
    float *vistas = (float *) malloc((size_t) largura * (size_t) altura * sizeof(float));
    if (vistas == NULL) {
        return;
    }
    for (int i = 0; i < largura * altura; i++) {
        vistas[i] = 0.0f;
    }
    /* as direções, uma vez só */
    static float direcoes[VISTA_ALTURA][VISTA_LARGURA][3];
    for (int j = 0; j < VISTA_ALTURA; j++) {
        float zenite = (j + 0.5f) / VISTA_ALTURA * PI;
        for (int i = 0; i < VISTA_LARGURA; i++) {
            float volta = ((i + 0.5f) / VISTA_LARGURA - 0.5f) * 2.0f * PI;
            direcoes[j][i][0] = cosf(volta) * sinf(zenite);
            direcoes[j][i][1] = cosf(zenite);
            direcoes[j][i][2] = sinf(volta) * sinf(zenite);
        }
    }
    double comeco = GetTime();
    int quadro = 0;
    for (int c = 0; c < totalDeConjuntos; c++) {
        for (int k = 0; k < conjuntos[c].totalDeLampadas; k++, quadro++) {
            Lampada *l = &conjuntos[c].lampadas[k];
            l->quadro = quadro;
            int x0 = (quadro % QUADROS_POR_LINHA) * VISTA_LARGURA, y0 = (quadro / QUADROS_POR_LINHA) * VISTA_ALTURA;
            for (int j = 0; j < VISTA_ALTURA; j++) {
                for (int i = 0; i < VISTA_LARGURA; i++) {
                    const float *d = direcoes[j][i];
                    float origem[3] = { l->lugar.x + d[0] * AFASTA_DA_LAMPADA, l->lugar.y + d[1] * AFASTA_DA_LAMPADA,
                                        l->lugar.z + d[2] * AFASTA_DA_LAMPADA };
                    Acerto acerto;
                    float chega = l->raio + 1.0f;
                    if (raioNaMalha(malha, origem, d, l->raio, &acerto)) {
                        chega = acerto.distancia + AFASTA_DA_LAMPADA;
                    }
                    vistas[(y0 + j) * largura + x0 + i] = chega;
                }
            }
        }
    }
    Image imagem = { vistas, largura, altura, 1, PIXELFORMAT_UNCOMPRESSED_R32 };
    vistaDasLampadas = LoadTextureFromImage(imagem);
    free(vistas);
    SetTextureFilter(vistaDasLampadas, TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(vistaDasLampadas, TEXTURE_WRAP_CLAMP);
    temVista = vistaDasLampadas.id != 0;
    if (!temVista) {
        for (int c = 0; c < totalDeConjuntos; c++) {
            for (int k = 0; k < conjuntos[c].totalDeLampadas; k++) {
                conjuntos[c].lampadas[k].quadro = -1;
            }
        }
    }
    TraceLog(LOG_INFO, "LUZES: a vista de %d lâmpadas medida em %.2f s (%dx%d)", total, GetTime() - comeco, largura, altura);
}

void carregarLuzesDaCidade(MalhaDeColisao *malha) {
    if (carregadas) {
        return;
    }
    carregadas = 1;
    shaderDasLuzes = LoadShaderFromMemory(VERTICES_DAS_LUZES, PIXELS_DAS_LUZES);
    locAcesa = GetShaderLocation(shaderDasLuzes, "acesa");
    locBrilho = GetShaderLocation(shaderDasLuzes, "brilho");
    locTempo = GetShaderLocation(shaderDasLuzes, "tempo");
    locPisca = GetShaderLocation(shaderDasLuzes, "pisca");
    locLuzApagada = GetShaderLocation(shaderDasLuzes, "luzApagada");
    locHalo = GetShaderLocation(shaderDasLuzes, "halo");
    locCopia = GetShaderLocation(shaderDasLuzes, "copia");
    imagemDoBrilho = gerarImagemDoBrilho();

    totalDeConjuntos = 0;
    if (DirectoryExists(PASTA_DAS_LUZES)) {
        FilePathList arquivos = LoadDirectoryFilesEx(PASTA_DAS_LUZES, ".glb", false);
        for (unsigned int i = 0; i < arquivos.count && totalDeConjuntos < MAIS_CONJUNTOS; i++) {
            Conjunto *c = &conjuntos[totalDeConjuntos];
            memset(c, 0, sizeof(*c));
            snprintf(c->nome, sizeof(c->nome), "%s", GetFileNameWithoutExt(arquivos.paths[i]));
            c->pisca = (strcmp(c->nome, "semaforo") == 0);
            c->matriz = MatrixIdentity();
            c->visivel = 1;
            carregarModeloDoConjunto(c, arquivos.paths[i]);
            char json[512];
            snprintf(json, sizeof(json), "%s/%s.json", PASTA_DAS_LUZES, c->nome);
            lerJson(json, c);
            if (c->temModelo || c->totalDeLampadas > 0) {
                TraceLog(LOG_INFO, "LUZES: %s, %d malhas, %d luzes pontuais, acende em %.1f s", c->nome,
                         c->temModelo ? c->modelo.meshCount : 0, c->totalDeLampadas, c->acenderEm);
                totalDeConjuntos++;
            }
        }
        UnloadDirectoryFiles(arquivos);
    }
    for (int i = 0; i < (int) (sizeof(INTERIORES_DA_NOITE) / sizeof(INTERIORES_DA_NOITE[0])); i++) {
        if (totalDeConjuntos == MAIS_CONJUNTOS) {
            break;
        }
        Conjunto *c = &conjuntos[totalDeConjuntos];
        memset(c, 0, sizeof(*c));
        snprintf(c->nome, sizeof(c->nome), "%s", GetFileNameWithoutExt(INTERIORES_DA_NOITE[i]));
        c->interior = 1;
        c->matriz = MatrixIdentity();
        c->visivel = 1;
        if (lerJson(INTERIORES_DA_NOITE[i], c) && c->totalDeLampadas > 0) {
            totalDeConjuntos++;
        }
    }
    medirVistas(malha);
}

/*
 * registrarLuzesMoveis
 * Um conjunto que anda: o modelo e as luzes pontuais no referencial dele (o jogo diz onde ele está com
 * moverLuzes). Chamar depois de carregarLuzesDaCidade.
 * Recebe: o .glb, o .json (pode faltar) e se fica sempre aceso (senão, acende com a cidade).
 * Devolve: o número dele (-1 se não deu).
 */
int registrarLuzesMoveis(const char *glb, const char *json, int sempreAceso) {
    if (!carregadas || totalDeConjuntos == MAIS_CONJUNTOS || !FileExists(glb)) {
        return -1;
    }
    Conjunto *c = &conjuntos[totalDeConjuntos];
    memset(c, 0, sizeof(*c));
    snprintf(c->nome, sizeof(c->nome), "%s", GetFileNameWithoutExt(glb));
    c->movel = 1;
    c->sempreAceso = sempreAceso;
    c->matriz = MatrixIdentity();
    c->visivel = 0;
    carregarModeloDoConjunto(c, glb);
    if (json != NULL) {
        lerJson(json, c);
    }
    if (!c->temModelo) {
        return -1;
    }
    c->acesa = sempreAceso ? FIM_DO_ACENDER : (ligadas ? FIM_DO_ACENDER : 0.0f);
    TraceLog(LOG_INFO, "LUZES: %s (anda), %d malhas, %d luzes pontuais", c->nome, c->modelo.meshCount, c->totalDeLampadas);
    return totalDeConjuntos++;
}

/* Uma ilha de lâmpadas vai para o conjunto novo se o meio dela passa de zMinimo */
static int alemDoZ(Vector3 minimo, Vector3 maximo, void *dados) {
    return (minimo.z + maximo.z) * 0.5f > *(const float *) dados;
}

int separarLuzesMoveis(int qual, float zMinimo) {
    if (qual < 0 || qual >= totalDeConjuntos || !conjuntos[qual].movel || totalDeConjuntos == MAIS_CONJUNTOS) {
        return -1;
    }
    Conjunto *c = &conjuntos[qual];
    if (!c->temModelo || c->modelo.meshCount != 1) {
        return -1;   /* as lâmpadas acesas vêm numa malha só (ferramentas/luzes) */
    }
    Mesh fica, vai;
    if (!separarMalhaPorIlhas(c->modelo.meshes[0], alemDoZ, &zMinimo, &fica, &vai)) {
        return -1;
    }
    UnloadMesh(c->modelo.meshes[0]);
    c->modelo.meshes[0] = fica;
    c->caixa = GetModelBoundingBox(c->modelo);
    Conjunto *novo = &conjuntos[totalDeConjuntos];
    memset(novo, 0, sizeof(*novo));
    snprintf(novo->nome, sizeof(novo->nome), "%.40s_2", c->nome);
    novo->modelo = LoadModelFromMesh(vai);
    novo->modelo.materials[0].shader = shaderDasLuzes;
    novo->temModelo = 1;
    novo->caixa = GetModelBoundingBox(novo->modelo);
    novo->acenderEm = c->acenderEm;
    novo->acesa = c->acesa;
    novo->movel = 1;
    novo->sempreAceso = c->sempreAceso;
    novo->matriz = c->matriz;
    TraceLog(LOG_INFO, "LUZES: %s separado em dois (%d e %d triângulos)", c->nome, fica.triangleCount, vai.triangleCount);
    return totalDeConjuntos++;
}

void moverLuzes(int qual, Matrix matriz, int visiveis) {
    if (qual < 0 || qual >= totalDeConjuntos || !conjuntos[qual].movel) {
        return;
    }
    conjuntos[qual].matriz = matriz;
    conjuntos[qual].visivel = visiveis;
}

void ligarVistaDasLampadas(void) {
    if (!temVista) {
        return;
    }
    rlActiveTextureSlot(SLOT_DA_VISTA_DAS_LAMPADAS);
    rlEnableTexture(vistaDasLampadas.id);
    rlActiveTextureSlot(0);
}

void descarregarLuzesDaCidade(void) {
    if (!carregadas) {
        return;
    }
    for (int i = 0; i < totalDeConjuntos; i++) {
        Conjunto *c = &conjuntos[i];
        if (c->temModelo) {
            for (int m = 0; m < c->modelo.materialCount; m++) {
                c->modelo.materials[m].shader.id = rlGetShaderIdDefault();   /* o shader é um só, de todos */
            }
            UnloadModel(c->modelo);
        }
    }
    totalDeConjuntos = 0;
    if (temVista) {
        UnloadTexture(vistaDasLampadas);
        temVista = 0;
    }
    UnloadShader(shaderDasLuzes);
    UnloadTexture(imagemDoBrilho);
    carregadas = 0;
}

/* ------------------------------------------------------------------
 * Acender
 * ------------------------------------------------------------------ */

void atualizarLuzesDaCidade(float acesas, float tempoQuadro, int pular) {
    tempo += tempoQuadro;
    acesasAgora = Clamp(acesas, 0.0f, 1.0f);
    int querLigar = acesas > LIMIAR_DE_ACENDER;
    if (querLigar != ligadas) {
        ligadas = querLigar;
        desdeQueMudou = 0.0f;
    }
    desdeQueMudou += tempoQuadro;
    for (int i = 0; i < totalDeConjuntos; i++) {
        Conjunto *c = &conjuntos[i];
        if (c->sempreAceso) {
            c->acesa = FIM_DO_ACENDER;
        } else if (c->interior) {
            c->acesa = acesasAgora;
        } else if (pular) {
            c->acesa = ligadas ? FIM_DO_ACENDER : 0.0f;
        } else if (ligadas) {
            if (desdeQueMudou >= c->acenderEm) {
                c->acesa = fminf(FIM_DO_ACENDER, c->acesa + tempoQuadro / DURACAO_DO_ACENDER);
            }
        } else {
            c->acesa = fmaxf(0.0f, c->acesa - tempoQuadro / DURACAO_DO_APAGAR);
        }
    }
}

/* ------------------------------------------------------------------
 * Desenhar
 * ------------------------------------------------------------------ */

static float distanciaDaCaixa(Vector3 camera, BoundingBox caixa) {
    return Vector3Distance(camera, Vector3Clamp(camera, caixa.min, caixa.max));
}

/* A distância da câmera ao conjunto (nos que andam, pela esfera em volta da caixa, levada pela matriz) */
static float distanciaDoConjunto(Vector3 camera, const Conjunto *c) {
    if (!c->movel) {
        return distanciaDaCaixa(camera, c->caixa);
    }
    Vector3 meio = Vector3Transform(Vector3Scale(Vector3Add(c->caixa.min, c->caixa.max), 0.5f), c->matriz);
    return fmaxf(Vector3Distance(camera, meio) - Vector3Distance(c->caixa.min, c->caixa.max) * 0.5f, 0.0f);
}

/* Onde a lâmpada está no mundo */
static Vector3 lugarDaLampada(const Conjunto *c, const Lampada *l) {
    return c->movel ? Vector3Transform(l->lugar, c->matriz) : l->lugar;
}

static void mandarUniformes(const Conjunto *c, float halo, Vector3 luzDoDia) {
    float pisca = c->pisca ? 1.0f : 0.0f;
    float brilho = c->pisca ? 1.25f : 1.0f + 0.6f * acesasAgora;
    SetShaderValue(shaderDasLuzes, locAcesa, &c->acesa, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shaderDasLuzes, locBrilho, &brilho, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shaderDasLuzes, locTempo, &tempo, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shaderDasLuzes, locPisca, &pisca, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shaderDasLuzes, locHalo, &halo, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shaderDasLuzes, locLuzApagada, &luzDoDia, SHADER_UNIFORM_VEC3);
}

/* A malha é a cópia de uma superfície pintada do mapa (tem textura)? */
static int ehCopia(const Conjunto *c, int malha) {
    unsigned int id = c->modelo.materials[c->modelo.meshMaterial[malha]].maps[MATERIAL_MAP_ALBEDO].texture.id;
    return id != 0 && id != rlGetTextureIdDefault();
}

/* Desenha as malhas do conjunto, avisando o shader de quais são cópias */
static void desenharMalhas(const Conjunto *c) {
    Matrix origem = c->matriz;
    float copiaAntes = -1.0f;
    for (int m = 0; m < c->modelo.meshCount; m++) {
        float copia = ehCopia(c, m) ? 1.0f : 0.0f;
        if (copia != copiaAntes) {
            SetShaderValue(shaderDasLuzes, locCopia, &copia, SHADER_UNIFORM_FLOAT);
            copiaAntes = copia;
        }
        DrawMesh(c->modelo.meshes[m], c->modelo.materials[c->modelo.meshMaterial[m]], origem);
    }
}

void desenharLuzesDaCidade(Camera3D camera, Vector3 luzDoDia) {
    rlDrawRenderBatchActive();
    for (int i = 0; i < totalDeConjuntos; i++) {
        const Conjunto *c = &conjuntos[i];
        if (!c->temModelo || !c->visivel || distanciaDoConjunto(camera.position, c) > DESENHA_ATE) {
            continue;
        }
        mandarUniformes(c, 0.0f, luzDoDia);
        desenharMalhas(c);
    }
}

void desenharHalosDaCidade(Camera3D camera) {
    rlDrawRenderBatchActive();
    for (int i = 0; i < totalDeConjuntos; i++) {
        const Conjunto *c = &conjuntos[i];
        if (!c->temModelo || !c->visivel || (c->acesa <= 0.0f && !c->pisca) || distanciaDoConjunto(camera.position, c) > HALO_ATE) {
            continue;
        }
        mandarUniformes(c, 1.0f, (Vector3){ 1.0f, 1.0f, 1.0f });
        desenharMalhas(c);
    }
    /* Em volta de cada luz pontual acesa, um borrão da cor dela: o tubo fino some na imagem pequena do
     * halo, e assim o letreiro brilha mesmo de longe. O borrão vem para a frente da luz (para a câmera)
     * metade do tamanho dele, senão a parede atrás da luz o cortaria; o que está entre a câmera e a luz
     * o tapa */
    comecarBrilhos();
    for (int i = 0; i < totalDeConjuntos; i++) {
        const Conjunto *c = &conjuntos[i];
        float forca = (c->interior || !c->visivel) ? 0.0f : Clamp(c->acesa, 0.0f, 1.0f);
        if (forca <= 0.0f) {
            continue;
        }
        for (int k = 0; k < c->totalDeLampadas; k++) {
            const Lampada *l = &c->lampadas[k];
            Vector3 aqui = lugarDaLampada(c, l);
            float distancia = Vector3Distance(camera.position, aqui);
            if (distancia > HALO_ATE || distancia < 1.0f) {
                continue;
            }
            Color cor = { (unsigned char) (Clamp(l->cor.x, 0.0f, 1.0f) * 255.0f), (unsigned char) (Clamp(l->cor.y, 0.0f, 1.0f) * 255.0f),
                          (unsigned char) (Clamp(l->cor.z, 0.0f, 1.0f) * 255.0f),
                          (unsigned char) (Clamp(0.22f * l->forca * forca, 0.0f, 1.0f) * 255.0f) };
            /* de longe ele cresce um pouco, senão some na imagem pequena do halo */
            float tamanho = fmaxf(Clamp(l->raio * 0.45f, 2.5f, 4.5f), distancia * 0.012f);
            Vector3 paraACamera = Vector3Scale(Vector3Subtract(camera.position, aqui), 1.0f / distancia);
            Vector3 lugar = Vector3Add(aqui, Vector3Scale(paraACamera, fminf(tamanho * 0.5f, distancia * 0.5f)));
            desenharBrilhoNoAr(camera, lugar, tamanho, cor);
        }
    }
    terminarBrilhos();
}

/* ------------------------------------------------------------------
 * As luzes pontuais
 * ------------------------------------------------------------------ */

int lampadasAcesas(Lampada *saida, int cabe) {
    int total = 0;
    for (int i = 0; i < totalDeConjuntos && total < cabe; i++) {
        const Conjunto *c = &conjuntos[i];
        float forca = c->interior ? acesasAgora : Clamp(c->acesa, 0.0f, 1.0f);
        if (forca <= 0.01f || !c->visivel) {
            continue;
        }
        for (int k = 0; k < c->totalDeLampadas && total < cabe; k++) {
            saida[total] = c->lampadas[k];
            saida[total].lugar = lugarDaLampada(c, &c->lampadas[k]);
            saida[total].forca *= forca;
            total++;
        }
    }
    return total;
}

int escolherLampadas(Camera3D camera, const Lampada *todas, int total, Lampada *saida, int cabe) {
    float notas[MAIS_LAMPADAS];
    int escolhidas = 0;
    if (cabe > MAIS_LAMPADAS) {
        cabe = MAIS_LAMPADAS;
    }
    Vector3 olhar = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    for (int i = 0; i < total; i++) {
        /* a que mais aparece: a luz que alcança mais perto da câmera e está na frente dela (a de trás
         * só se sobrar lugar: o chão iluminado por ela não aparece na tela) */
        Vector3 ate = Vector3Subtract(todas[i].lugar, camera.position);
        float distancia = Vector3Length(ate);
        float nota = distancia - todas[i].raio * 1.5f;
        if (distancia > todas[i].raio && Vector3DotProduct(ate, olhar) < -todas[i].raio) {
            nota += 400.0f;
        }
        if (escolhidas < cabe) {
            saida[escolhidas] = todas[i];
            notas[escolhidas] = nota;
            escolhidas++;
            continue;
        }
        int pior = 0;
        for (int k = 1; k < escolhidas; k++) {
            if (notas[k] > notas[pior]) {
                pior = k;
            }
        }
        if (nota < notas[pior]) {
            saida[pior] = todas[i];
            notas[pior] = nota;
        }
    }
    return escolhidas;
}

/* ------------------------------------------------------------------
 * Brilhos soltos
 * ------------------------------------------------------------------ */

void comecarBrilhos(void) {
    rlDrawRenderBatchActive();
    BeginBlendMode(BLEND_ADDITIVE);
    rlDisableDepthMask();
}

void desenharBrilhoNoAr(Camera3D camera, Vector3 lugar, float tamanho, Color cor) {
    if (!carregadas) {
        return;
    }
    Rectangle origem = { 0.0f, 0.0f, (float) imagemDoBrilho.width, (float) imagemDoBrilho.height };
    DrawBillboardRec(camera, imagemDoBrilho, origem, lugar, (Vector2){ tamanho, tamanho }, cor);
}

void terminarBrilhos(void) {
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
    EndBlendMode();
}
