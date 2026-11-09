/*
 * torcida3d.c
 * ------------------------------------------------------------------
 * A torcida 3D. Veja torcida3d.h.
 *
 * Cada pista tem o seu conjunto de lugares: na Copa Pistão eles saem dos assentos do
 * modelo do estádio; em Interlagos, no Oval e no Splat, dos lugares medidos nos modelos. A cada quadro, os lugares perto da câmera viram
 * carrinhos: o completo até RAIO_3D e um bem simples até RAIO_DE_LONGE, cada grupo
 * desenhado de uma vez (instâncias). Mais longe continua a torcida pintada. Uma grade de
 * células deixa achar os lugares perto sem olhar um por um. Os lugares ficam na placa
 * de vídeo desde a carga: por quadro, só se escolhe quais trechos da lista desenhar.
 * ------------------------------------------------------------------
 */

#include <math.h>
#include <stdlib.h>
#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"
#include "torcida3d.h"
#include "noite.h"
#include "pista.h"

#define ARQUIVO_DO_CARRINHO   "recursos/modelos/objetos/carro_torcida.glb"
#define ARQUIVO_DE_LONGE      "recursos/modelos/objetos/carro_torcida_longe.glb"
#define CENTRO_X_DO_MODELO    (-0.66f)  /* o centro do estádio, em unidades do modelo (como em noite.c) */
#define FILA                  3.4f      /* metros entre uma fileira e a de trás, no estádio */
#define LUGAR                 2.2f      /* metros entre um carrinho e o do lado, no estádio */
#define RAIO_3D               200.0f    /* metros: até aqui o carrinho completo... */
#define RAIO_DE_LONGE         400.0f    /* ...e até aqui o simples; mais longe, a torcida pintada */
#define SURGINDO              30.0f     /* metros finais em que eles crescem ao aparecer */
#define CELULA                40.0f     /* metros: o lado de cada célula da grade */

typedef struct {
    float x, y, z, giro;
} Lugar;

/* Os lugares de uma pista */
typedef struct {
    Lugar *lugares;
    int total, capacidade;
    int *inicioDaCelula;     /* lugares ordenados por célula: o primeiro de cada uma */
    int celulasX, celulasZ;
    float origemX, origemZ;
    float tamanho;           /* o carrinho nessa pista, em relação ao modelo */
    unsigned int buffer;     /* os lugares, na placa de vídeo */
} Conjunto;

static Conjunto conjuntos[TOTAL_PISTAS];

static Mesh carrinho, carrinhoDeLonge;
static Shader shaderDoCarrinho;
static int temCarrinho = 0;
static int locTempo, locOnda, locFrente, locCamera, locFaixa, locTamanho, locMvp, locLugar;
static float tempo = 0.0f;

/* O que desenhar neste quadro: trechos contíguos da lista de lugares (uma linha da grade
 * perto da câmera), para o carrinho de perto e para o de longe */
#define MAXIMO_DE_TRECHOS     256
typedef struct {
    int inicio, quantos;
} Trecho;
static Trecho trechosDePerto[MAXIMO_DE_TRECHOS], trechosDeLonge[MAXIMO_DE_TRECHOS];
static int totalDePerto = 0, totalDeLonge = 0;
static Conjunto *conjuntoDoQuadro = NULL;
static Vector3 cameraDoQuadro;

/* Cada lugar chega como um vec4 (posição e giro), guardado na placa de vídeo uma vez só. O shader
 * gira, põe no lugar e escala o carrinho, e some com quem está fora da faixa de distância dele */
static const char *VERTICES =
    "#version 330\n"
    "in vec3 vertexPosition; in vec3 vertexNormal; in vec4 vertexColor; in vec4 instanceLugar;\n"
    "uniform mat4 mvp; uniform float tempo; uniform vec3 camera; uniform vec3 faixa;\n"   /* faixa: de, até, e o raio onde eles somem */
    "uniform float tamanho;\n"
    "out vec3 fragPos; out vec3 fragNormal; out vec4 fragColor; out float semente;\n"
    "void main() {\n"
    "    vec3 base = instanceLugar.xyz;\n"
    "    float d = distance(base, camera);\n"
    "    if (d < faixa.x || d >= faixa.y) { gl_Position = vec4(2.0, 2.0, 2.0, 1.0); return; }\n"
    "    semente = fract(sin(dot(base.xz, vec2(12.9898, 78.233))) * 43758.5453);\n"
    "    float escala = tamanho * min((faixa.z - d) / 30.0, 1.0);\n"
    "    vec3 p = vertexPosition;\n"
    "    p.y += 0.07 * max(sin(tempo * (5.0 + 3.0 * semente) + semente * 40.0), 0.0);\n"
    "    float c = cos(instanceLugar.w), s = sin(instanceLugar.w);\n"
    "    vec3 girado = vec3(p.x * c + p.z * s, p.y, -p.x * s + p.z * c);\n"
    "    vec3 n = vertexNormal;\n"
    "    fragNormal = vec3(n.x * c + n.z * s, n.y, -n.x * s + n.z * c);\n"
    "    fragPos = base + girado * escala;\n"
    "    fragColor = vertexColor;\n"
    "    gl_Position = mvp * vec4(fragPos, 1.0);\n"
    "}\n";

static const char *PIXELS =
    "#version 330\n"
    "in vec3 fragPos; in vec3 fragNormal; in vec4 fragColor; in float semente;\n"
    "uniform vec3 onda; uniform float frenteDaOnda;\n"
    "out vec4 finalColor;\n"
    GLSL_DA_LUZ_DA_NOITE
    "vec3 corDoCarro(float s) {\n"
    "    if (s < 0.22) return vec3(0.20, 0.42, 0.85);\n"
    "    if (s < 0.36) return vec3(0.25, 0.70, 0.80);\n"
    "    if (s < 0.54) return vec3(0.82, 0.20, 0.20);\n"
    "    if (s < 0.64) return vec3(0.90, 0.90, 0.92);\n"
    "    if (s < 0.74) return vec3(0.22, 0.62, 0.34);\n"
    "    if (s < 0.84) return vec3(0.88, 0.70, 0.20);\n"
    "    if (s < 0.93) return vec3(0.55, 0.32, 0.70);\n"
    "    return vec3(0.12, 0.13, 0.16);\n"
    "}\n"
    "void main() {\n"
    "    vec3 n = normalize(fragNormal);\n"
    "    vec3 cor = fragColor.rgb;\n"
    "    bool ehCorpo = cor.r > 0.9 && cor.g > 0.9 && cor.b > 0.9;\n"
    "    bool ehFarol = cor.r > 0.9 && cor.g > 0.9 && cor.b < 0.1;\n"
    "    if (ehCorpo) cor = corDoCarro(semente);\n"
    "    vec3 luz = luzDaNoite(fragPos, n) * 0.95 + vec3(0.05);\n"
    "    if (noite < 0.5) {\n"   /* de dia, o mesmo sol do cenário */
    "        vec3 m = (n.y < -0.2) ? -n : n;\n"
    "        luz = vec3(0.58 + 0.30 * (m.y * 0.5 + 0.5) + 0.38 * max(dot(m, normalize(vec3(0.35, 0.8, 0.45))), 0.0)) * 0.92;\n"
    "    }\n"
    "    vec3 resultado = cor * luz;\n"
    "    if (ehFarol) {\n"
    "        float acende = 0.0;\n"
    "        if (onda.z > 0.0) {\n"
    "            float angulo = atan(fragPos.z - onda.y, fragPos.x - onda.x);\n"
    "            float atras = mod(frenteDaOnda - angulo - 0.05 * semente + 12.5663706, 6.2831853);\n"
    "            acende = onda.z * smoothstep(0.0, 0.04, atras) * (1.0 - smoothstep(0.12, 0.5, atras));\n"
    "        }\n"
    "        resultado = mix(vec3(0.30, 0.29, 0.22) * luz, vec3(1.0, 0.96, 0.80) * 5.0, acende);\n"
    "        if (noite < 0.5) resultado = vec3(0.85, 0.82, 0.65) * luz;\n"
    "    }\n"
    "    finalColor = vec4(resultado, 1.0);\n"
    "}\n";

/*
 * adicionarLugarTorcida3D
 * Guarda um lugar da torcida de uma pista. Chamar enquanto o cenário é montado.
 * Recebe: a pista, o lugar no mundo e o giro (radianos: para onde a frente do carrinho aponta).
 * Devolve: nada.
 */
void adicionarLugarTorcida3D(int pista, float x, float y, float z, float giro) {
    if (pista < 0 || pista >= TOTAL_PISTAS) {
        return;
    }
    Conjunto *c = &conjuntos[pista];
    if (c->total == c->capacidade) {
        c->capacidade = (c->capacidade == 0) ? 4096 : c->capacidade * 2;
        c->lugares = (Lugar *) MemRealloc(c->lugares, c->capacidade * sizeof(Lugar));
    }
    c->lugares[c->total++] = (Lugar){ x, y, z, giro };
}

static Mesh juntarCarrinho(Model modelo) {
    int vertices = 0, triangulos = 0;
    for (int m = 0; m < modelo.meshCount; m++) {
        vertices += modelo.meshes[m].vertexCount;
        triangulos += modelo.meshes[m].triangleCount;
    }
    Mesh malha = { 0 };
    malha.vertexCount = vertices;
    malha.triangleCount = triangulos;
    malha.vertices = (float *) MemAlloc(vertices * 3 * sizeof(float));
    malha.normals = (float *) MemAlloc(vertices * 3 * sizeof(float));
    malha.colors = (unsigned char *) MemAlloc(vertices * 4);
    malha.indices = (unsigned short *) MemAlloc(triangulos * 3 * sizeof(unsigned short));
    int v = 0, t = 0;
    for (int m = 0; m < modelo.meshCount; m++) {
        Mesh peca = modelo.meshes[m];
        Color cor = modelo.materials[modelo.meshMaterial[m]].maps[MATERIAL_MAP_ALBEDO].color;
        for (int i = 0; i < peca.vertexCount; i++) {
            for (int k = 0; k < 3; k++) {
                malha.vertices[(v + i) * 3 + k] = peca.vertices[i * 3 + k];
                malha.normals[(v + i) * 3 + k] = peca.normals ? peca.normals[i * 3 + k] : (k == 1 ? 1.0f : 0.0f);
            }
            malha.colors[(v + i) * 4 + 0] = cor.r;
            malha.colors[(v + i) * 4 + 1] = cor.g;
            malha.colors[(v + i) * 4 + 2] = cor.b;
            malha.colors[(v + i) * 4 + 3] = 255;
        }
        for (int i = 0; i < peca.triangleCount * 3; i++) {
            int indice = peca.indices ? peca.indices[i] : i;
            malha.indices[t * 3 + i] = (unsigned short) (v + indice);
        }
        v += peca.vertexCount;
        t += peca.triangleCount;
    }
    UploadMesh(&malha, false);
    return malha;
}


static void acharLugares(const float *v, const unsigned short *idx, int triangulos, float escala) {
    float cx = CENTRO_X_DO_MODELO * escala;
    for (int t = 0; t < triangulos; t++) {
        Vector3 p[3];
        for (int k = 0; k < 3; k++) {
            int i = idx ? idx[t * 3 + k] : t * 3 + k;
            p[k] = (Vector3){ v[i * 3] * escala, v[i * 3 + 1] * escala, v[i * 3 + 2] * escala };
        }
        Vector3 n = Vector3CrossProduct(Vector3Subtract(p[1], p[0]), Vector3Subtract(p[2], p[0]));
        float tamanho = Vector3Length(n);
        if (tamanho <= 0.0f) {
            continue;
        }
        n = Vector3Scale(n, 1.0f / tamanho);
        if (n.y < 0.0f) {
            n = Vector3Negate(n);
        }
        if (n.y < 0.3f || -n.x * (p[0].x - cx) - n.z * p[0].z <= 0.0f) {
            continue;   /* só o que está deitado e virado para a pista */
        }
        /* O pedaço de fileiras e de ângulos que o triângulo cobre */
        float raio[3], angulo[3];
        for (int k = 0; k < 3; k++) {
            raio[k] = hypotf(p[k].x - cx, p[k].z);
            angulo[k] = atan2f(p[k].z, p[k].x - cx);
        }
        for (int k = 1; k < 3; k++) {
            while (angulo[k] - angulo[0] > PI) angulo[k] -= 2.0f * PI;
            while (angulo[k] - angulo[0] < -PI) angulo[k] += 2.0f * PI;
        }
        float rMin = fminf(raio[0], fminf(raio[1], raio[2])), rMax = fmaxf(raio[0], fmaxf(raio[1], raio[2]));
        float aMin = fminf(angulo[0], fminf(angulo[1], angulo[2])), aMax = fmaxf(angulo[0], fmaxf(angulo[1], angulo[2]));
        float den = (p[1].z - p[2].z) * (p[0].x - p[2].x) + (p[2].x - p[1].x) * (p[0].z - p[2].z);
        if (fabsf(den) < 1e-6f) {
            continue;
        }
        for (int fila = (int) ceilf(rMin / FILA); fila * FILA <= rMax; fila++) {
            float r = fila * FILA, passo = LUGAR / r;
            for (int j = (int) ceilf(aMin / passo); j * passo <= aMax; j++) {
                float a = j * passo, x = cx + cosf(a) * r, z = sinf(a) * r;
                float l0 = ((p[1].z - p[2].z) * (x - p[2].x) + (p[2].x - p[1].x) * (z - p[2].z)) / den;
                float l1 = ((p[2].z - p[0].z) * (x - p[2].x) + (p[0].x - p[2].x) * (z - p[2].z)) / den;
                float l2 = 1.0f - l0 - l1;
                if (l0 < 0.0f || l1 < 0.0f || l2 < 0.0f) {
                    continue;
                }
                adicionarLugarTorcida3D(PISTA_COPA_PISTAO, x, l0 * p[0].y + l1 * p[1].y + l2 * p[2].y, z,
                                        atan2f(cx - x, -z));   /* a frente (+z do modelo) para o centro */
            }
        }
    }
}


/*
 * celulaDe
 * Recebe: o conjunto e um ponto do mundo.
 * Devolve: o número da célula da grade (ou -1 se fora dela).
 */
static int celulaDe(const Conjunto *c, float x, float z) {
    int i = (int) ((x - c->origemX) / CELULA), k = (int) ((z - c->origemZ) / CELULA);
    if (i < 0 || k < 0 || i >= c->celulasX || k >= c->celulasZ) {
        return -1;
    }
    return k * c->celulasX + i;
}

/*
 * ordenarPorCelula
 * Põe os lugares de um conjunto em ordem de célula (contagem): cada célula vira um trecho da lista.
 * Recebe: o conjunto.
 * Devolve: nada.
 */
static void ordenarPorCelula(Conjunto *c) {
    if (c->total == 0) {
        return;
    }
    float minX = 1e9f, maxX = -1e9f, minZ = 1e9f, maxZ = -1e9f;
    for (int i = 0; i < c->total; i++) {
        minX = fminf(minX, c->lugares[i].x);
        maxX = fmaxf(maxX, c->lugares[i].x);
        minZ = fminf(minZ, c->lugares[i].z);
        maxZ = fmaxf(maxZ, c->lugares[i].z);
    }
    c->origemX = minX;
    c->origemZ = minZ;
    c->celulasX = (int) ((maxX - minX) / CELULA) + 1;
    c->celulasZ = (int) ((maxZ - minZ) / CELULA) + 1;
    int total = c->celulasX * c->celulasZ;
    c->inicioDaCelula = (int *) MemAlloc((total + 1) * sizeof(int));
    for (int i = 0; i < c->total; i++) {
        c->inicioDaCelula[celulaDe(c, c->lugares[i].x, c->lugares[i].z) + 1]++;
    }
    for (int k = 0; k < total; k++) {
        c->inicioDaCelula[k + 1] += c->inicioDaCelula[k];
    }
    Lugar *ordenados = (Lugar *) MemAlloc(c->total * sizeof(Lugar));
    int *proximo = (int *) MemAlloc(total * sizeof(int));
    for (int k = 0; k < total; k++) {
        proximo[k] = c->inicioDaCelula[k];
    }
    for (int i = 0; i < c->total; i++) {
        ordenados[proximo[celulaDe(c, c->lugares[i].x, c->lugares[i].z)]++] = c->lugares[i];
    }
    MemFree(proximo);
    MemFree(c->lugares);
    c->lugares = ordenados;
}

/*
 * carregarTorcidaDoEstadio
 * Os lugares da Copa Pistão, nos assentos do modelo do estádio.
 * Recebe: o modelo do estádio, o material dos assentos e a escala do modelo.
 * Devolve: nada.
 */
void carregarTorcidaDoEstadio(Model estadio, int materialDosAssentos, float escala) {
    for (int m = 0; m < estadio.meshCount; m++) {
        if (estadio.meshMaterial[m] == materialDosAssentos) {
            Mesh assentos = estadio.meshes[m];
            acharLugares(assentos.vertices, assentos.indices, assentos.triangleCount, escala);
        }
    }
    definirTamanhoTorcida3D(PISTA_COPA_PISTAO, 1.0f);
}

/*
 * definirTamanhoTorcida3D
 * Recebe: a pista e o tamanho do carrinho nela, em relação ao modelo (que tem 3 m).
 * Devolve: nada.
 */
void definirTamanhoTorcida3D(int pista, float tamanho) {
    if (pista >= 0 && pista < TOTAL_PISTAS) {
        conjuntos[pista].tamanho = tamanho;
    }
}

/*
 * iniciarTorcida3D
 * Carrega os carrinhos e o shader, e organiza os lugares de todas as pistas. Chamar depois
 * que o cenário de todas elas foi montado (e depois de carregarNoite).
 * Recebe: nada.
 * Devolve: nada.
 */
void iniciarTorcida3D(void) {
    if (!FileExists(ARQUIVO_DO_CARRINHO) || !FileExists(ARQUIVO_DE_LONGE)) {
        TraceLog(LOG_WARNING, "TORCIDA 3D: sem os modelos do carrinho");
        return;
    }
    for (int p = 0; p < TOTAL_PISTAS; p++) {
        ordenarPorCelula(&conjuntos[p]);
        if (conjuntos[p].total > 0) {
            conjuntos[p].buffer = rlLoadVertexBuffer(conjuntos[p].lugares, conjuntos[p].total * (int) sizeof(Lugar), false);
        }
        TraceLog(LOG_INFO, "TORCIDA 3D: pista %d com %d lugares", p, conjuntos[p].total);
    }
    Model modelo = LoadModel(ARQUIVO_DO_CARRINHO);
    carrinho = juntarCarrinho(modelo);
    UnloadModel(modelo);
    modelo = LoadModel(ARQUIVO_DE_LONGE);
    carrinhoDeLonge = juntarCarrinho(modelo);
    UnloadModel(modelo);
    shaderDoCarrinho = LoadShaderFromMemory(VERTICES, PIXELS);
    locMvp = GetShaderLocation(shaderDoCarrinho, "mvp");
    locCamera = GetShaderLocation(shaderDoCarrinho, "camera");
    locFaixa = GetShaderLocation(shaderDoCarrinho, "faixa");
    locTamanho = GetShaderLocation(shaderDoCarrinho, "tamanho");
    locLugar = GetShaderLocationAttrib(shaderDoCarrinho, "instanceLugar");
    locTempo = GetShaderLocation(shaderDoCarrinho, "tempo");
    locOnda = GetShaderLocation(shaderDoCarrinho, "onda");
    locFrente = GetShaderLocation(shaderDoCarrinho, "frenteDaOnda");
    registrarShaderIluminado(shaderDoCarrinho);
    temCarrinho = locLugar >= 0;
    if (!temCarrinho) {
        TraceLog(LOG_WARNING, "TORCIDA 3D: o shader não tem instanceLugar");
    }
}

/*
 * prepararTorcida3D
 * Escolhe os carrinhos deste quadro (perto da câmera e não atrás dela) e avisa a torcida
 * pintada onde ela deve sumir. Chamar antes de desenhar o cenário.
 * Recebe: a pista e a câmera.
 * Devolve: nada.
 */
void prepararTorcida3D(int pista, Camera3D camera) {
    totalDePerto = 0;
    totalDeLonge = 0;
    conjuntoDoQuadro = NULL;
    Conjunto *c = (pista >= 0 && pista < TOTAL_PISTAS) ? &conjuntos[pista] : NULL;
    /* As variáveis de ambiente são lidas uma vez: getenv percorre o ambiente comparando
     * nomes, e isto é um caminho de desenho */
    static int desligada = -1;
    if (desligada < 0) {
        desligada = (getenv("KATCHAU_SEM_TORCIDA3D") != NULL);
    }
    int ativa = temCarrinho && c != NULL && c->total > 0 && !desligada;
    esconderTorcidaPintada(camera.position, ativa ? RAIO_DE_LONGE : 0.0f);
    if (!ativa) {
        return;
    }
    conjuntoDoQuadro = c;
    cameraDoQuadro = camera.position;
    tempo = (float) GetTime();   /* o telão também desenha a torcida: o relógio não pode andar duas vezes */

    /* Cada linha da grade perto da câmera: as células de cada linha estão juntas na lista, então
     * as que ficam dentro do raio (e não atrás da câmera) viram um trecho só */
    Vector3 frente = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    float meiaDiagonal = CELULA * 0.7072f;
    for (int faixa = 0; faixa < 2; faixa++) {
        float raio = (faixa == 0) ? RAIO_3D : RAIO_DE_LONGE;
        float desde = (faixa == 0) ? 0.0f : RAIO_3D;
        Trecho *trechos = (faixa == 0) ? trechosDePerto : trechosDeLonge;
        int *total = (faixa == 0) ? &totalDePerto : &totalDeLonge;
        int c0 = (int) floorf((camera.position.x - raio - c->origemX) / CELULA), c1 = (int) floorf((camera.position.x + raio - c->origemX) / CELULA);
        int k0 = (int) floorf((camera.position.z - raio - c->origemZ) / CELULA), k1 = (int) floorf((camera.position.z + raio - c->origemZ) / CELULA);
        for (int k = (k0 < 0 ? 0 : k0); k <= k1 && k < c->celulasZ; k++) {
            int abertoDesde = -1;
            for (int i = (c0 < 0 ? 0 : c0); i <= c1 + 1; i++) {
                int serve = 0;
                if (i <= c1 && i < c->celulasX) {
                    float cx = c->origemX + (i + 0.5f) * CELULA - camera.position.x;
                    float cz = c->origemZ + (k + 0.5f) * CELULA - camera.position.z;
                    float distancia = sqrtf(cx * cx + cz * cz);
                    serve = distancia < raio + meiaDiagonal && distancia > desde - meiaDiagonal
                            && cx * frente.x + cz * frente.z > -meiaDiagonal - 8.0f;
                }
                int celula = k * c->celulasX + i;
                if (serve && abertoDesde < 0) {
                    abertoDesde = c->inicioDaCelula[celula];
                } else if (!serve && abertoDesde >= 0) {
                    int fim = c->inicioDaCelula[k * c->celulasX + ((i < c->celulasX) ? i : c->celulasX)];
                    if (fim > abertoDesde && *total < MAXIMO_DE_TRECHOS) {
                        trechos[(*total)++] = (Trecho){ abertoDesde, fim - abertoDesde };
                    }
                    abertoDesde = -1;
                }
                if (i >= c->celulasX) {
                    break;
                }
            }
        }
    }

    float forca, frenteDaOnda, centroX;
    estadoDaOnda(&forca, &frenteDaOnda, &centroX);
    if (pista != PISTA_COPA_PISTAO) {
        forca = 0.0f;   /* a onda é da Copa Pistão */
    }
    Vector3 onda = { centroX, 0.0f, forca };
    SetShaderValue(shaderDoCarrinho, locTempo, &tempo, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shaderDoCarrinho, locOnda, &onda, SHADER_UNIFORM_VEC3);
    SetShaderValue(shaderDoCarrinho, locFrente, &frenteDaOnda, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shaderDoCarrinho, locCamera, &cameraDoQuadro, SHADER_UNIFORM_VEC3);
    SetShaderValue(shaderDoCarrinho, locTamanho, &c->tamanho, SHADER_UNIFORM_FLOAT);
}

/*
 * desenharTrechos
 * Desenha um carrinho em cada lugar dos trechos, apontando o atributo do lugar para o
 * começo de cada trecho dentro do buffer.
 * Recebe: a malha, os trechos, quantos são e a faixa de distância (de, até).
 * Devolve: nada.
 */
static void desenharTrechos(Mesh malha, const Trecho *trechos, int total, float de, float ate) {
    if (total == 0) {
        return;
    }
    Vector3 faixa = { de, ate, RAIO_DE_LONGE };
    SetShaderValue(shaderDoCarrinho, locFaixa, &faixa, SHADER_UNIFORM_VEC3);
    rlEnableShader(shaderDoCarrinho.id);
    rlSetUniformMatrix(locMvp, MatrixMultiply(rlGetMatrixModelview(), rlGetMatrixProjection()));
    rlEnableVertexArray(malha.vaoId);
    rlEnableVertexBuffer(conjuntoDoQuadro->buffer);
    rlEnableVertexAttribute(locLugar);
    rlSetVertexAttributeDivisor(locLugar, 1);
    for (int t = 0; t < total; t++) {
        rlSetVertexAttribute(locLugar, 4, RL_FLOAT, false, (int) sizeof(Lugar), trechos[t].inicio * (int) sizeof(Lugar));
        rlDrawVertexArrayElementsInstanced(0, malha.triangleCount * 3, 0, trechos[t].quantos);
    }
    rlDisableVertexAttribute(locLugar);
    rlDisableVertexBuffer();
    rlDisableVertexArray();
    rlDisableShader();
}

/*
 * desenharTorcida3D
 * Os carrinhos escolhidos, de uma vez cada grupo. Chamar entre BeginMode3D e EndMode3D.
 * Recebe: nada.
 * Devolve: nada.
 */
void desenharTorcida3D(void) {
    if (conjuntoDoQuadro == NULL) {
        return;
    }
    static int depurar = -1;
    if (depurar < 0) {
        depurar = (getenv("KATCHAU_DEPURAR_TORCIDA") != NULL);
    }
    if (depurar) {
        TraceLog(LOG_INFO, "TORCIDA 3D: %d trechos perto, %d longe", totalDePerto, totalDeLonge);
    }
    rlDrawRenderBatchActive();
    desenharTrechos(carrinho, trechosDePerto, totalDePerto, 0.0f, RAIO_3D);
    desenharTrechos(carrinhoDeLonge, trechosDeLonge, totalDeLonge, RAIO_3D, RAIO_DE_LONGE);
}

/*
 * descarregarTorcida3D
 * Recebe: nada.
 * Devolve: nada.
 */
void descarregarTorcida3D(void) {
    if (temCarrinho) {
        UnloadMesh(carrinho);
        UnloadMesh(carrinhoDeLonge);
        UnloadShader(shaderDoCarrinho);
    }
    for (int p = 0; p < TOTAL_PISTAS; p++) {
        if (conjuntos[p].buffer != 0) {
            rlUnloadVertexBuffer(conjuntos[p].buffer);
        }
        MemFree(conjuntos[p].lugares);
        MemFree(conjuntos[p].inicioDaCelula);
        conjuntos[p] = (Conjunto){ 0 };
    }
    temCarrinho = 0;
}
