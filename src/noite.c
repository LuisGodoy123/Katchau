/*
 * noite.c
 * ------------------------------------------------------------------
 * A noite de corrida da Copa Pistão. Veja noite.h.
 * ------------------------------------------------------------------
 */

#include <math.h>
#include <stddef.h>
#include "noite.h"
#include "raymath.h"
#include "rlgl.h"

#define MAXIMO_DE_POSTES      128
#define MAXIMO_DE_SHADERS     8
#define PESO_ATRAS_DA_CAMERA  3.0f     /* um poste atrás da câmera conta como 3 vezes mais longe */

/* ---------- Flashes das câmeras ---------- */
#define MAXIMO_DE_FLASHES     700
#define FLASHES_POR_SEGUNDO   1800.0f
#define VIDA_MINIMA_FLASH     0.08f    /* segundos */
#define VIDA_MAXIMA_FLASH     0.30f
#define TAMANHO_DO_FLASH      0.042f   /* em relação à distância: todos parecem do mesmo tamanho */
#define COR_FLASH_BRANCO      (Color){ 255, 255, 255, 255 }
#define COR_FLASH_AZUL        (Color){ 120, 185, 255, 255 }

/* ---------- Refletores, no alto das arquibancadas ---------- */
#define FATIAS_DO_ANEL        48
#define RAIO_MINIMO_DO_ANEL   14.0f    /* em unidades do modelo: só o que está em volta da pista */
#define ACIMA_DOS_ASSENTOS    0.15f    /* em unidades do modelo */
#define HALO_DO_REFLETOR      125.0f   /* metros */
#define MIOLO_DO_REFLETOR     38.0f
#define ALTURA_DO_ANEL_DE_BAIXO 4.6f   /* em unidades do modelo: a beirada da arquibancada de baixo */
#define HALO_DE_BAIXO         95.0f
#define MIOLO_DE_BAIXO        24.0f
#define COR_DO_HALO           (Color){  90, 150, 255, 150 }
#define COR_DO_MIOLO          (Color){ 235, 245, 255, 255 }

/* ---------- Lâmpadas dos postes ---------- */
#define HALO_DO_POSTE         9.0f
#define MIOLO_DO_POSTE        2.2f
#define MENOR_TAMANHO_POSTE   0.012f   /* de longe, o halo não some: nunca menos que isso vezes a distância */
#define COR_DO_HALO_DO_POSTE  (Color){ 255, 225, 170, 120 }

/* ---------- Céu ---------- */
#define LARGURA_DO_CEU        2048
#define ALTURA_DO_CEU         512
#define ESTRELAS              1500
#define COR_DO_CEU_EM_CIMA    (Color){   3,   5,  14, 255 }
#define COR_DO_CEU_EMBAIXO    (Color){  10,  16,  34, 255 }

#define CENTRO_DO_OVAL_X      (-0.66f) /* em unidades do modelo */

static const char *VERTICES_DO_SHADER =
    "#version 330\n"
    "in vec3 vertexPosition; in vec2 vertexTexCoord; in vec4 vertexColor;\n"
    "uniform mat4 mvp; uniform mat4 matModel;\n"
    "out vec2 fragTexCoord; out vec4 fragColor; out vec3 fragPos;\n"
    "void main() { fragTexCoord = vertexTexCoord; fragColor = vertexColor;\n"
    "              fragPos = vec3(matModel * vec4(vertexPosition, 1.0));\n"
    "              gl_Position = mvp * vec4(vertexPosition, 1.0); }\n";

/* O cenário: a imagem do modelo, com a luz da noite. A direção da superfície
 * sai das derivadas da posição, porque nem toda malha traz as normais. */
static const char *PIXELS_DO_CENARIO =
    "#version 330\n"
    "in vec2 fragTexCoord; in vec4 fragColor; in vec3 fragPos;\n"
    "uniform sampler2D texture0; uniform vec4 colDiffuse;\n"
    "out vec4 finalColor;\n"
    GLSL_DA_LUZ_DA_NOITE
    "void main() { vec4 cor = texture(texture0, fragTexCoord) * colDiffuse * fragColor;\n"
    "              if (cor.a < 0.5) discard;\n"
    "              vec3 n = normalize(cross(dFdx(fragPos), dFdy(fragPos)));\n"
    "              if (n.y < 0.0) n = -n;\n"
    "              finalColor = vec4(cor.rgb * luzDaNoite(fragPos, n), 1.0); }\n";

/* A torcida: os blocos de assentos do modelo têm 13 filas por 14 lugares, com um
 * corredor no meio, e se repetem. Cada lugar sorteia uma cor pela posição, então o
 * padrão nunca se repete. De perto ele vira um carrinho (capô e para-brisa); de longe,
 * quando um lugar fica menor que um pixel, sorteia por grupos de lugares: assim a
 * multidão continua salpicada e não treme. */
static const char *PIXELS_DA_TORCIDA =
    "#version 330\n"
    "in vec2 fragTexCoord; in vec4 fragColor; in vec3 fragPos;\n"
    "uniform sampler2D texture0; uniform float tempo;\n"
    "out vec4 finalColor;\n"
    GLSL_DA_LUZ_DA_NOITE
    "float sorteio(vec2 c) { return fract(sin(dot(c, vec2(127.1, 311.7))) * 43758.5453); }\n"
    "vec3 corDoLugar(vec2 c) {\n"
    "    float s = sorteio(c);\n"
    "    if (s < 0.30) return vec3(0.20, 0.42, 0.85);\n"
    "    if (s < 0.46) return vec3(0.25, 0.70, 0.80);\n"
    "    if (s < 0.64) return vec3(0.82, 0.20, 0.20);\n"
    "    if (s < 0.74) return vec3(0.90, 0.90, 0.92);\n"
    "    if (s < 0.82) return vec3(0.22, 0.62, 0.34);\n"
    "    if (s < 0.90) return vec3(0.88, 0.70, 0.20);\n"
    "    if (s < 0.96) return vec3(0.55, 0.32, 0.70);\n"
    "    return vec3(0.06, 0.07, 0.10);\n"
    "}\n"
    "void main() {\n"
    "    vec2 lugares = fragTexCoord * vec2(60.0, 52.0);\n"
    "    vec2 pixelPorEixo = vec2(length(vec2(dFdx(lugares.x), dFdy(lugares.x))), length(vec2(dFdx(lugares.y), dFdy(lugares.y))));\n"
    "    float pixel = max(pixelPorEixo.x, pixelPorEixo.y);\n"
    "    vec2 tamanhoDoGrupo = pow(vec2(2.0), ceil(log2(max(pixelPorEixo, vec2(1.0)))));\n"
    "    vec2 grupo = floor(lugares / tamanhoDoGrupo) * tamanhoDoGrupo;\n"
    "    vec3 cor = corDoLugar(grupo + tamanhoDoGrupo);\n"
    "    cor = mix(cor, vec3(dot(cor, vec3(0.3, 0.6, 0.1))), 0.28) * (0.62 + 0.5 * sorteio(grupo + 7.0));\n"
    "    cor *= 1.0 + 0.14 * sin(tempo * 3.5 + sorteio(grupo) * 40.0);\n"
    "    float perto = 1.0 - smoothstep(0.15, 0.7, pixel);\n"
    "    if (perto > 0.0) {\n"
    "        vec2 l = fract(lugares);\n"
    "        float corpo = smoothstep(0.08, 0.16, l.x) * smoothstep(0.92, 0.84, l.x) * smoothstep(0.10, 0.18, l.y) * smoothstep(0.86, 0.72, l.y);\n"
    "        float vidro = step(0.26, l.x) * step(l.x, 0.74) * step(0.30, l.y) * step(l.y, 0.50);\n"
    "        vec3 carro = mix(cor, vec3(0.05, 0.07, 0.10), vidro * 0.85) * (1.15 - 0.45 * l.y);\n"
    "        cor = mix(cor, mix(vec3(0.03), carro, corpo), perto);\n"
    "    }\n"
    "    float corredor = abs(fract(fragTexCoord.x) - 0.5);\n"
    "    cor = mix(vec3(0.16, 0.17, 0.20), cor, smoothstep(0.015, 0.04, corredor));\n"
    "    vec3 n = normalize(cross(dFdx(fragPos), dFdy(fragPos)));\n"
    "    if (n.y < 0.0) n = -n;\n"
    "    finalColor = vec4(cor * (luzDaNoite(fragPos, n) * 0.8 + vec3(0.30, 0.34, 0.42)), 1.0);\n"
    "}\n";

/* Um flash aceso */
typedef struct {
    Vector3 lugar;
    float idade, vida;
    float tamanho;
    Color cor;
} Flash;

/* Um shader que recebe as luzes, e onde ficam as variáveis dele */
typedef struct {
    Shader shader;
    int locNoite, locTotal, locLuzes;
} ShaderIluminado;

static Shader shaderDoCenario, shaderDeTorcida;
static int locTempo;
static ShaderIluminado iluminados[MAXIMO_DE_SHADERS];
static int totalDeIluminados = 0;

static Vector3 postes[MAXIMO_DE_POSTES];
static int totalDePostes = 0;

static Texture2D imagemDoBrilho, imagemDaEstrela, ceuNoturno;
static Vector3 refletores[FATIAS_DO_ANEL];
static int totalDeRefletores = 0;
static Vector3 refletoresDeBaixo[FATIAS_DO_ANEL];
static int totalDeBaixo = 0;

/* Os triângulos dos assentos, para sortear onde cada flash acontece */
static float *verticesDosAssentos = NULL;
static unsigned short *indicesDosAssentos = NULL;
static int totalDeTriangulos = 0;
static int *triangulosVirados = NULL;     /* só os que olham para a pista */
static int totalDeVirados = 0;
static float escalaDoEstadio = 1.0f;

static Flash flashes[MAXIMO_DE_FLASHES];
static int proximoFlash = 0;
static float flashesParaSoltar = 0.0f;
static float tempoDaTorcida = 0.0f;

/*
 * gerarBrilho
 * Um borrão redondo que cai suavemente do centro para a borda.
 * Recebe: nada.
 * Devolve: a textura.
 */
static Texture2D gerarBrilho(void) {
    int lado = 128;
    Image imagem = GenImageColor(lado, lado, BLANK);
    for (int y = 0; y < lado; y++) {
        for (int x = 0; x < lado; x++) {
            float dx = (x - lado / 2.0f + 0.5f) / (lado / 2.0f), dy = (y - lado / 2.0f + 0.5f) / (lado / 2.0f);
            float cai = fmaxf(1.0f - sqrtf(dx * dx + dy * dy), 0.0f);
            ImageDrawPixel(&imagem, x, y, (Color){ 255, 255, 255, (unsigned char) (cai * cai * 255.0f) });
        }
    }
    Texture2D textura = LoadTextureFromImage(imagem);
    GenTextureMipmaps(&textura);
    SetTextureFilter(textura, TEXTURE_FILTER_TRILINEAR);
    UnloadImage(imagem);
    return textura;
}

/*
 * gerarEstrela
 * O flash: um miolo forte com quatro raios finos, como a luz numa lente.
 * Recebe: nada.
 * Devolve: a textura.
 */
static Texture2D gerarEstrela(void) {
    int lado = 128;
    Image imagem = GenImageColor(lado, lado, BLANK);
    for (int y = 0; y < lado; y++) {
        for (int x = 0; x < lado; x++) {
            float dx = fabsf(x - lado / 2.0f + 0.5f) / (lado / 2.0f), dy = fabsf(y - lado / 2.0f + 0.5f) / (lado / 2.0f);
            float d = sqrtf(dx * dx + dy * dy);
            float miolo = 0.016f / (d * d + 0.016f);
            float raioX = expf(-dy * 45.0f) * fmaxf(1.0f - dx, 0.0f);
            float raioY = expf(-dx * 45.0f) * fmaxf(1.0f - dy, 0.0f);
            float borda = fmaxf(1.0f - d, 0.0f);
            float alfa = fminf(miolo * borda + 0.75f * (raioX * raioX + raioY * raioY), 1.0f);
            ImageDrawPixel(&imagem, x, y, (Color){ 255, 255, 255, (unsigned char) (alfa * 255.0f) });
        }
    }
    Texture2D textura = LoadTextureFromImage(imagem);
    GenTextureMipmaps(&textura);
    SetTextureFilter(textura, TEXTURE_FILTER_TRILINEAR);
    UnloadImage(imagem);
    return textura;
}

/*
 * gerarCeu
 * O céu escuro, um pouco mais claro no horizonte, com estrelas de brilhos
 * diferentes. Só a metade de cima tem estrelas: a de baixo é coberta.
 * Recebe: nada.
 * Devolve: a textura.
 */
static Texture2D gerarCeu(void) {
    Image imagem = GenImageGradientLinear(LARGURA_DO_CEU, ALTURA_DO_CEU, 0, COR_DO_CEU_EM_CIMA, COR_DO_CEU_EMBAIXO);
    for (int i = 0; i < ESTRELAS; i++) {
        unsigned char forca = (unsigned char) GetRandomValue(70, 255);
        ImageDrawPixel(&imagem, GetRandomValue(0, LARGURA_DO_CEU - 1), GetRandomValue(0, ALTURA_DO_CEU - 1),
                       (Color){ forca, forca, (unsigned char) fminf(forca + 20, 255), 255 });
    }
    Texture2D textura = LoadTextureFromImage(imagem);
    SetTextureWrap(textura, TEXTURE_WRAP_REPEAT);
    SetTextureFilter(textura, TEXTURE_FILTER_BILINEAR);
    UnloadImage(imagem);
    return textura;
}

/*
 * verticeDoAssento
 * Recebe: o índice do vértice.
 * Devolve: a posição dele, em unidades do modelo.
 */
static Vector3 verticeDoAssento(int indice) {
    return (Vector3){ verticesDosAssentos[indice * 3], verticesDosAssentos[indice * 3 + 1], verticesDosAssentos[indice * 3 + 2] };
}

/*
 * normalDoTriangulo
 * Recebe: o índice do triângulo dos assentos.
 * Devolve: a direção para onde ele olha, sempre para cima.
 */
static Vector3 normalDoTriangulo(int t) {
    Vector3 a = verticeDoAssento(indicesDosAssentos[t * 3]);
    Vector3 b = verticeDoAssento(indicesDosAssentos[t * 3 + 1]);
    Vector3 c = verticeDoAssento(indicesDosAssentos[t * 3 + 2]);
    Vector3 n = Vector3Normalize(Vector3CrossProduct(Vector3Subtract(b, a), Vector3Subtract(c, a)));
    return (n.y < 0.0f) ? Vector3Negate(n) : n;
}

/*
 * montarAnel
 * Acha um anel de refletores: em cada fatia em volta do oval, o assento mais
 * alto abaixo de um limite.
 * Recebe: a malha dos assentos, a altura limite (unidades do modelo) e onde guardar.
 * Devolve: quantos refletores saíram.
 */
static int montarAnel(Mesh malha, float limite, Vector3 anel[]) {
    Vector3 maisAlto[FATIAS_DO_ANEL];
    for (int f = 0; f < FATIAS_DO_ANEL; f++) {
        maisAlto[f] = (Vector3){ 0.0f, -1.0f, 0.0f };
    }
    for (int i = 0; i < malha.vertexCount; i++) {
        Vector3 v = verticeDoAssento(i);
        float dx = v.x - CENTRO_DO_OVAL_X;
        if (hypotf(dx, v.z) < RAIO_MINIMO_DO_ANEL || v.y > limite) {
            continue;
        }
        int fatia = (int) ((atan2f(v.z, dx) + PI) / (2.0f * PI) * FATIAS_DO_ANEL) % FATIAS_DO_ANEL;
        if (v.y > maisAlto[fatia].y) {
            maisAlto[fatia] = v;
        }
    }
    int total = 0;
    for (int f = 0; f < FATIAS_DO_ANEL; f++) {
        if (maisAlto[f].y > 0.0f) {
            anel[total++] = (Vector3){ maisAlto[f].x * escalaDoEstadio, (maisAlto[f].y + ACIMA_DOS_ASSENTOS) * escalaDoEstadio,
                                       maisAlto[f].z * escalaDoEstadio };
        }
    }
    return total;
}

/*
 * prepararAssentos
 * Guarda os triângulos dos assentos que olham para a pista, onde os flashes
 * podem acontecer, e acha o anel dos refletores: em cada fatia em volta do
 * oval, o assento mais alto.
 * Recebe: a malha dos assentos.
 * Devolve: nada.
 */
static void prepararAssentos(Mesh malha) {
    verticesDosAssentos = malha.vertices;
    indicesDosAssentos = malha.indices;
    totalDeTriangulos = malha.triangleCount;
    triangulosVirados = MemAlloc(totalDeTriangulos * sizeof(int));
    totalDeVirados = 0;

    for (int t = 0; t < totalDeTriangulos; t++) {
        Vector3 a = verticeDoAssento(indicesDosAssentos[t * 3]);
        Vector3 n = normalDoTriangulo(t);
        float paraOCentro = -n.x * (a.x - CENTRO_DO_OVAL_X) - n.z * a.z;
        if (n.y > 0.3f && paraOCentro > 0.0f) {
            triangulosVirados[totalDeVirados++] = t;
        }
    }

    totalDeRefletores = montarAnel(malha, 1000.0f, refletores);
    totalDeBaixo = montarAnel(malha, ALTURA_DO_ANEL_DE_BAIXO, refletoresDeBaixo);
}

/*
 * soltarFlash
 * Acende um flash num lugar sorteado dos assentos.
 * Recebe: nada.
 * Devolve: nada.
 */
static void soltarFlash(void) {
    if (totalDeVirados == 0) {
        return;
    }
    int t = triangulosVirados[GetRandomValue(0, totalDeVirados - 1)];
    float u = GetRandomValue(0, 1000) / 1000.0f, v = GetRandomValue(0, 1000) / 1000.0f;
    if (u + v > 1.0f) {   /* mantém o ponto dentro do triângulo */
        u = 1.0f - u;
        v = 1.0f - v;
    }
    Vector3 a = verticeDoAssento(indicesDosAssentos[t * 3]);
    Vector3 b = verticeDoAssento(indicesDosAssentos[t * 3 + 1]);
    Vector3 c = verticeDoAssento(indicesDosAssentos[t * 3 + 2]);
    Vector3 ponto = Vector3Add(a, Vector3Add(Vector3Scale(Vector3Subtract(b, a), u), Vector3Scale(Vector3Subtract(c, a), v)));
    ponto = Vector3Add(ponto, Vector3Scale(normalDoTriangulo(t), 0.03f));   /* um pouco à frente do assento */

    Flash *flash = &flashes[proximoFlash];
    proximoFlash = (proximoFlash + 1) % MAXIMO_DE_FLASHES;
    flash->lugar = Vector3Scale(ponto, escalaDoEstadio);
    flash->idade = 0.0f;
    flash->vida = VIDA_MINIMA_FLASH + (VIDA_MAXIMA_FLASH - VIDA_MINIMA_FLASH) * GetRandomValue(0, 1000) / 1000.0f;
    flash->tamanho = TAMANHO_DO_FLASH * (0.6f + 1.3f * GetRandomValue(0, 1000) / 1000.0f);
    flash->cor = (GetRandomValue(0, 2) == 0) ? COR_FLASH_AZUL : COR_FLASH_BRANCO;
}

/*
 * carregarNoite
 * Prepara os shaders, as imagens e os lugares dos flashes e dos refletores.
 * Recebe: o modelo do estádio, o material dos assentos e a escala dele.
 * Devolve: nada.
 */
void carregarNoite(Model estadio, int materialDosAssentos, float escala) {
    escalaDoEstadio = escala;
    shaderDoCenario = LoadShaderFromMemory(VERTICES_DO_SHADER, PIXELS_DO_CENARIO);
    shaderDeTorcida = LoadShaderFromMemory(VERTICES_DO_SHADER, PIXELS_DA_TORCIDA);
    locTempo = GetShaderLocation(shaderDeTorcida, "tempo");
    registrarShaderIluminado(shaderDoCenario);
    registrarShaderIluminado(shaderDeTorcida);

    imagemDoBrilho = gerarBrilho();
    imagemDaEstrela = gerarEstrela();
    ceuNoturno = gerarCeu();

    for (int i = 0; i < estadio.meshCount; i++) {
        if (estadio.meshMaterial[i] == materialDosAssentos) {
            prepararAssentos(estadio.meshes[i]);
            break;
        }
    }
    for (int i = 0; i < MAXIMO_DE_FLASHES; i++) {
        flashes[i].vida = 0.0f;
    }
}

/*
 * descarregarNoite
 * Devolve a memória. Chamar antes de CloseWindow.
 * Recebe: nada.
 * Devolve: nada.
 */
void descarregarNoite(void) {
    MemFree(triangulosVirados);
    triangulosVirados = NULL;
    UnloadTexture(imagemDoBrilho);
    UnloadTexture(imagemDaEstrela);
    UnloadTexture(ceuNoturno);
    UnloadShader(shaderDoCenario);
    UnloadShader(shaderDeTorcida);
}

/*
 * registrarShaderIluminado
 * Inclui um shader entre os que recebem as luzes a cada quadro.
 * Recebe: o shader (ele precisa usar GLSL_DA_LUZ_DA_NOITE).
 * Devolve: nada.
 */
void registrarShaderIluminado(Shader shader) {
    if (totalDeIluminados == MAXIMO_DE_SHADERS) {
        return;
    }
    ShaderIluminado *novo = &iluminados[totalDeIluminados++];
    novo->shader = shader;
    novo->locNoite = GetShaderLocation(shader, "noite");
    novo->locTotal = GetShaderLocation(shader, "totalLuzes");
    novo->locLuzes = GetShaderLocation(shader, "luzes");
}

/*
 * adicionarLuzDePoste
 * Recebe: onde fica a lâmpada de um poste, em metros.
 * Devolve: nada.
 */
void adicionarLuzDePoste(Vector3 lampada) {
    if (totalDePostes < MAXIMO_DE_POSTES) {
        postes[totalDePostes++] = lampada;
    }
}

/*
 * shaderDaNoite / shaderDaTorcida / texturaDoCeuNoturno
 * Recebem: nada.
 * Devolvem: o que o nome diz, para o cenário (mundo.c).
 */
Shader shaderDaNoite(void) {
    return shaderDoCenario;
}
Shader shaderDaTorcida(void) {
    return shaderDeTorcida;
}
Texture2D texturaDoCeuNoturno(void) {
    return ceuNoturno;
}

/*
 * prepararLuzes
 * Escolhe os postes mais perto da câmera e envia para os shaders. Um poste
 * atrás dela pesa menos, porque o que está à frente é o que aparece.
 * Recebe: a câmera e se é noite nesta pista.
 * Devolve: nada.
 */
void prepararLuzes(Camera3D camera, int ehNoite) {
    Vector3 escolhidos[MAXIMO_DE_LUZES_NO_SHADER];
    float distancias[MAXIMO_DE_LUZES_NO_SHADER];
    int total = 0;

    if (ehNoite) {
        Vector3 frente = Vector3Subtract(camera.target, camera.position);
        for (int i = 0; i < totalDePostes; i++) {
            Vector3 ate = Vector3Subtract(postes[i], camera.position);
            float distancia = Vector3Length(ate) * ((Vector3DotProduct(ate, frente) < 0.0f) ? PESO_ATRAS_DA_CAMERA : 1.0f);
            /* Inserção ordenada, ficando só com os mais perto */
            int lugar = total;
            while (lugar > 0 && distancias[lugar - 1] > distancia) {
                lugar--;
            }
            if (lugar == MAXIMO_DE_LUZES_NO_SHADER) {
                continue;
            }
            for (int j = (total < MAXIMO_DE_LUZES_NO_SHADER ? total : MAXIMO_DE_LUZES_NO_SHADER - 1); j > lugar; j--) {
                distancias[j] = distancias[j - 1];
                escolhidos[j] = escolhidos[j - 1];
            }
            distancias[lugar] = distancia;
            escolhidos[lugar] = postes[i];
            if (total < MAXIMO_DE_LUZES_NO_SHADER) {
                total++;
            }
        }
    }

    float valorDaNoite = ehNoite ? 1.0f : 0.0f;
    for (int i = 0; i < totalDeIluminados; i++) {
        SetShaderValue(iluminados[i].shader, iluminados[i].locNoite, &valorDaNoite, SHADER_UNIFORM_FLOAT);
        SetShaderValue(iluminados[i].shader, iluminados[i].locTotal, &total, SHADER_UNIFORM_INT);
        if (total > 0) {
            SetShaderValueV(iluminados[i].shader, iluminados[i].locLuzes, escolhidos, SHADER_UNIFORM_VEC3, total);
        }
    }
    SetShaderValue(shaderDeTorcida, locTempo, &tempoDaTorcida, SHADER_UNIFORM_FLOAT);
}

/*
 * atualizarNoite
 * Solta os flashes novos e envelhece os acesos. Com tempo 0 (pausa), para tudo.
 * Recebe: o tempo que passou.
 * Devolve: nada.
 */
void atualizarNoite(float tempoQuadro) {
    tempoDaTorcida += tempoQuadro;
    for (int i = 0; i < MAXIMO_DE_FLASHES; i++) {
        flashes[i].idade += tempoQuadro;
    }
    flashesParaSoltar += FLASHES_POR_SEGUNDO * tempoQuadro;
    while (flashesParaSoltar >= 1.0f) {
        soltarFlash();
        flashesParaSoltar -= 1.0f;
    }
}

/*
 * desenharImagemNoAr
 * Uma imagem que encara a câmera.
 * Recebe: a câmera, a imagem, o centro, o tamanho e a cor.
 * Devolve: nada.
 */
static void desenharImagemNoAr(Camera3D camera, Texture2D imagem, Vector3 centro, float tamanho, Color cor) {
    Rectangle origem = { 0.0f, 0.0f, (float) imagem.width, (float) imagem.height };
    DrawBillboardRec(camera, imagem, origem, centro, (Vector2){ tamanho, tamanho }, cor);
}

/*
 * desenharBrilhos
 * Os refletores, as lâmpadas dos postes e os flashes, somando luz ao que está
 * atrás. Eles respeitam o que está na frente, mas não escrevem na profundidade.
 * Chamar dentro de BeginMode3D, depois de tudo o que é sólido.
 * Recebe: a câmera.
 * Devolve: nada.
 */
void desenharBrilhos(Camera3D camera) {
    rlDrawRenderBatchActive();
    BeginBlendMode(BLEND_ADDITIVE);
    rlDisableDepthMask();

    for (int i = 0; i < totalDeRefletores; i++) {
        desenharImagemNoAr(camera, imagemDoBrilho, refletores[i], HALO_DO_REFLETOR, COR_DO_HALO);
        desenharImagemNoAr(camera, imagemDoBrilho, refletores[i], MIOLO_DO_REFLETOR, COR_DO_MIOLO);
    }
    for (int i = 0; i < totalDeBaixo; i++) {
        desenharImagemNoAr(camera, imagemDoBrilho, refletoresDeBaixo[i], HALO_DE_BAIXO, COR_DO_HALO);
        desenharImagemNoAr(camera, imagemDoBrilho, refletoresDeBaixo[i], MIOLO_DE_BAIXO, COR_DO_MIOLO);
    }
    for (int i = 0; i < totalDePostes; i++) {
        float distancia = Vector3Distance(postes[i], camera.position);
        float halo = fmaxf(HALO_DO_POSTE, distancia * MENOR_TAMANHO_POSTE * 2.5f);
        desenharImagemNoAr(camera, imagemDoBrilho, postes[i], halo, COR_DO_HALO_DO_POSTE);
        desenharImagemNoAr(camera, imagemDoBrilho, postes[i], fmaxf(MIOLO_DO_POSTE, distancia * MENOR_TAMANHO_POSTE), WHITE);
    }
    for (int i = 0; i < MAXIMO_DE_FLASHES; i++) {
        Flash *flash = &flashes[i];
        if (flash->vida <= 0.0f || flash->idade >= flash->vida) {
            continue;
        }
        float forca = 1.0f - flash->idade / flash->vida;
        float distancia = Vector3Distance(flash->lugar, camera.position);
        desenharImagemNoAr(camera, imagemDaEstrela, flash->lugar, flash->tamanho * distancia, Fade(flash->cor, forca));
    }

    rlDrawRenderBatchActive();
    rlEnableDepthMask();
    EndBlendMode();
}
