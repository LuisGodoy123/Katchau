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

#define MAXIMO_DE_SHADERS     8

/* ---------- Flashes das câmeras ---------- */
#define MAXIMO_DE_FLASHES     700
#define FLASHES_POR_SEGUNDO   1800.0f
#define VIDA_MINIMA_FLASH     0.08f    /* segundos */
#define VIDA_MAXIMA_FLASH     0.30f
#define TAMANHO_DO_FLASH      0.024f   /* em relação à distância: todos parecem do mesmo tamanho */
#define HALO_DO_FLASH         1.1f     /* o halo do flash, em relação à estrela */
#define COR_FLASH_BRANCO      (Color){ 255, 255, 255, 255 }
#define COR_FLASH_AZUL        (Color){ 120, 185, 255, 255 }

/* ---------- Refletores, no alto das arquibancadas ---------- */
#define FATIAS_DO_ANEL        32       /* um refletor por fatia: é o tamanho de "luzes" no shader */
#define RAIO_MINIMO_DO_ANEL   14.0f    /* em unidades do modelo: só o que está em volta da pista */
#define ACIMA_DOS_ASSENTOS    0.15f    /* em unidades do modelo */
#define HALO_DO_REFLETOR      0.17f    /* em relação à distância: de longe ele continua forte */
#define MIOLO_DO_REFLETOR     0.055f
#define NA_FRENTE_DO_REFLETOR 16.0f    /* metros: o brilho fica à frente da estrutura, senão ela o cobre */
#define MENOR_HALO            70.0f    /* metros */
#define MENOR_MIOLO           24.0f
#define COR_DO_HALO           (Color){  85, 145, 255, 170 }
#define COR_DO_MIOLO          (Color){ 235, 245, 255, 255 }

/* ---------- Céu ---------- */
#define LARGURA_DO_CEU        2048
#define ALTURA_DO_CEU         512
#define ESTRELAS              1500

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
 * sai das derivadas da posição, porque nem toda malha traz as normais: ela
 * sempre aponta para o lado de quem olha. */
static const char *PIXELS_DO_CENARIO =
    "#version 330\n"
    "in vec2 fragTexCoord; in vec4 fragColor; in vec3 fragPos;\n"
    "uniform sampler2D texture0; uniform vec4 colDiffuse;\n"
    "out vec4 finalColor;\n"
    GLSL_DA_LUZ_DA_NOITE
    "void main() { vec4 cor = texture(texture0, fragTexCoord) * colDiffuse * fragColor;\n"
    "              if (cor.a < 0.5) discard;\n"
    "              vec3 n = normalize(cross(dFdx(fragPos), dFdy(fragPos)));\n"
    "              finalColor = vec4(cor.rgb * luzDaNoite(fragPos, n), 1.0); }\n";

/* A torcida: os blocos de assentos do modelo têm 13 filas por 14 lugares, com um
 * corredor no meio, e se repetem. Cada lugar sorteia uma cor pela posição, então o
 * padrão nunca se repete. De perto ele vira um carrinho (capô e para-brisa); de longe,
 * quando um lugar fica menor que um pixel, sorteia por grupos de lugares: assim a
 * multidão continua salpicada e não treme. */
static const char *PIXELS_DA_TORCIDA =
    "#version 330\n"
    "in vec2 fragTexCoord; in vec4 fragColor; in vec3 fragPos;\n"
    "uniform float tempo;\n"
    "uniform vec3 onda;\n"     /* x, y: o centro do estádio; z: a força da onda (0 = parada) */
    "uniform float frenteDaOnda;\n"   /* radianos: onde a frente da onda está, em volta do estádio */
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
    /* A onda: atrás da frente dela, os carros acendem os faróis; cada lugar tem um tiquinho de atraso */
    "    float acende = 0.0;\n"
    "    if (onda.z > 0.0 && noite > 0.5) {\n"
    "        float angulo = atan(fragPos.z - onda.y, fragPos.x - onda.x);\n"
    "        float atras = mod(frenteDaOnda - angulo - 0.06 * sorteio(grupo + 3.0) + 12.5663706, 6.2831853);\n"
    "        acende = onda.z * smoothstep(0.0, 0.05, atras) * (1.0 - smoothstep(0.10, 0.55, atras));\n"
    "    }\n"
    "    vec3 farois = vec3(0.0);\n"
    "    if (perto > 0.0) {\n"
    "        vec2 l = fract(lugares);\n"
    "        float corpo = smoothstep(0.08, 0.16, l.x) * smoothstep(0.92, 0.84, l.x) * smoothstep(0.10, 0.18, l.y) * smoothstep(0.86, 0.72, l.y);\n"
    "        float vidro = step(0.26, l.x) * step(l.x, 0.74) * step(0.30, l.y) * step(l.y, 0.50);\n"
    "        vec3 carro = mix(cor, vec3(0.05, 0.07, 0.10), vidro * 0.85) * (1.15 - 0.45 * l.y);\n"
    "        cor = mix(cor, mix(vec3(noite > 0.5 ? 0.03 : 0.30), carro, corpo), perto);\n"
    "        float farol = max(1.0 - length((l - vec2(0.27, 0.80)) / 0.11), 0.0) + max(1.0 - length((l - vec2(0.73, 0.80)) / 0.11), 0.0);\n"
    "        farois = vec3(1.0, 0.93, 0.70) * clamp(farol * 2.2, 0.0, 1.0) * corpo * perto;\n"
    "    }\n"
    "    float corredor = abs(fract(fragTexCoord.x) - 0.5);\n"
    "    cor = mix(vec3(0.16, 0.17, 0.20), cor, smoothstep(0.015, 0.04, corredor));\n"
    "    vec3 n = normalize(cross(dFdx(fragPos), dFdy(fragPos)));\n"
    "    if (noite < 0.5) cor = mix(cor, vec3(dot(cor, vec3(0.3, 0.6, 0.1))), 0.30) + vec3(0.03);\n"   /* de dia, o sol lava as cores */
    "    vec3 luz = (noite > 0.5) ? luzDaNoite(fragPos, n) * 0.72 + vec3(0.08, 0.09, 0.12) : vec3(0.85);\n"
    "    vec3 aceso = farois * 3.2 + vec3(1.0, 0.92, 0.72) * (0.55 * (1.0 - perto) + 0.22 * perto);\n"   /* de longe, o setor inteiro clareia */
    "    finalColor = vec4(cor * luz * fragColor.rgb * (1.0 + 0.6 * acende) + aceso * acende, 1.0);\n"
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

static Texture2D imagemDoBrilho, imagemDaEstrela, ceuNoturno;
static Vector3 refletores[FATIAS_DO_ANEL];
static int totalDeRefletores = 0;

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

/* A onda da torcida: os carros acendem os faróis um setor depois do outro, dando a volta no estádio */
#define SEGUNDOS_DA_ONDA      7.0f     /* uma volta completa */
#define SUBIDA_DA_ONDA        0.08f    /* da volta: o tanto que ela leva para ganhar força, e para sumir */
static float andamentoDaOnda = -1.0f;   /* de 0 a 1 pela volta; -1 = parada */
static float inicioDaOnda = 0.0f;       /* radianos: onde ela começou */
static int locOnda, locFrenteDaOnda;
static float flashesDaOnda = 0.0f;
#define FLASHES_NA_ONDA       1500.0f  /* por segundo, a mais, na faixa da onda */
#define TENTATIVAS_NA_ONDA    12
#define LARGURA_DOS_FLASHES_NA_ONDA 0.45f   /* radianos atrás da frente */

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
 * O céu preto, com estrelas de brilhos diferentes. É preto como o fundo da
 * tela, para não aparecer a emenda em cima do cilindro.
 * Recebe: nada.
 * Devolve: a textura.
 */
static Texture2D gerarCeu(void) {
    Image imagem = GenImageColor(LARGURA_DO_CEU, ALTURA_DO_CEU, BLACK);
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
 * Acha o lugar dos refletores: em cada fatia em volta do oval, em cima do
 * assento mais alto. O que fica guardado é o centro das lâmpadas.
 * Recebe: a malha dos assentos.
 * Devolve: nada (preenche refletores).
 */
static void montarAnel(Mesh malha) {
    Vector3 maisAlto[FATIAS_DO_ANEL];
    for (int f = 0; f < FATIAS_DO_ANEL; f++) {
        maisAlto[f] = (Vector3){ 0.0f, -1.0f, 0.0f };
    }
    for (int i = 0; i < malha.vertexCount; i++) {
        Vector3 v = verticeDoAssento(i);
        float dx = v.x - CENTRO_DO_OVAL_X;
        if (hypotf(dx, v.z) < RAIO_MINIMO_DO_ANEL) {
            continue;
        }
        int fatia = (int) ((atan2f(v.z, dx) + PI) / (2.0f * PI) * FATIAS_DO_ANEL) % FATIAS_DO_ANEL;
        if (v.y > maisAlto[fatia].y) {
            maisAlto[fatia] = v;
        }
    }
    totalDeRefletores = 0;
    for (int f = 0; f < FATIAS_DO_ANEL; f++) {
        if (maisAlto[f].y > 0.0f) {
            refletores[totalDeRefletores++] = (Vector3){ maisAlto[f].x * escalaDoEstadio,
                                                         (maisAlto[f].y + ACIMA_DOS_ASSENTOS) * escalaDoEstadio + ALTURA_DO_REFLETOR,
                                                         maisAlto[f].z * escalaDoEstadio };
        }
    }
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

    montarAnel(malha);
}

/*
 * soltarFlash
 * Acende um flash num lugar sorteado dos assentos.
 * Recebe: se ele tem que cair na faixa da onda, e onde está a frente dela (radianos).
 * Devolve: nada.
 */
static void soltarFlash(int naOnda, float frente) {
    if (totalDeVirados == 0) {
        return;
    }
    int t = triangulosVirados[GetRandomValue(0, totalDeVirados - 1)];
    for (int tentativa = 0; naOnda && tentativa < TENTATIVAS_NA_ONDA; tentativa++) {
        Vector3 v = verticeDoAssento(indicesDosAssentos[t * 3]);
        float atras = fmodf(frente - atan2f(v.z, v.x - CENTRO_DO_OVAL_X) + 4.0f * PI, 2.0f * PI);
        if (atras < LARGURA_DOS_FLASHES_NA_ONDA) {
            break;
        }
        t = triangulosVirados[GetRandomValue(0, totalDeVirados - 1)];
    }
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
    locOnda = GetShaderLocation(shaderDeTorcida, "onda");
    locFrenteDaOnda = GetShaderLocation(shaderDeTorcida, "frenteDaOnda");
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
 * refletoresDaNoite
 * Recebe: onde guardar o endereço da lista.
 * Devolve: quantos refletores há (mundo.c põe o modelo 3D em cada um).
 */
int refletoresDaNoite(const Vector3 **lista) {
    *lista = refletores;
    return totalDeRefletores;
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
 * Envia os refletores para os shaders. De dia não há nenhum.
 * Recebe: se é noite nesta pista.
 * Devolve: nada.
 */
void prepararLuzes(int ehNoite) {
    int total = ehNoite ? totalDeRefletores : 0;
    float valorDaNoite = ehNoite ? 1.0f : 0.0f;
    for (int i = 0; i < totalDeIluminados; i++) {
        SetShaderValue(iluminados[i].shader, iluminados[i].locNoite, &valorDaNoite, SHADER_UNIFORM_FLOAT);
        SetShaderValue(iluminados[i].shader, iluminados[i].locTotal, &total, SHADER_UNIFORM_INT);
        if (total > 0) {
            SetShaderValueV(iluminados[i].shader, iluminados[i].locLuzes, refletores, SHADER_UNIFORM_VEC3, total);
        }
    }
    SetShaderValue(shaderDeTorcida, locTempo, &tempoDaTorcida, SHADER_UNIFORM_FLOAT);

    /* A onda só existe à noite, no estádio */
    float forca = 0.0f, frente = 0.0f;
    if (ehNoite && andamentoDaOnda >= 0.0f) {
        float a = andamentoDaOnda;
        forca = fminf(fminf(a / SUBIDA_DA_ONDA, (1.0f - a) / SUBIDA_DA_ONDA), 1.0f);
        frente = inicioDaOnda + a * 2.0f * PI;
    }
    Vector3 onda = { CENTRO_DO_OVAL_X * escalaDoEstadio, 0.0f, forca };
    SetShaderValue(shaderDeTorcida, locOnda, &onda, SHADER_UNIFORM_VEC3);
    SetShaderValue(shaderDeTorcida, locFrenteDaOnda, &frente, SHADER_UNIFORM_FLOAT);
}

/*
 * comecarOndaDaTorcida
 * A torcida faz a onda: começa no lado dado e dá uma volta inteira no estádio.
 * Recebe: o ângulo, em volta do centro do estádio, de onde ela começa (radianos).
 * Devolve: nada.
 */
void comecarOndaDaTorcida(float angulo) {
    andamentoDaOnda = 0.0f;
    inicioDaOnda = angulo;
}

/*
 * anguloNoEstadio
 * Recebe: um ponto do mundo.
 * Devolve: o ângulo dele em volta do centro do estádio, como a onda mede (radianos).
 */
float anguloNoEstadio(float x, float z) {
    return atan2f(z, x - CENTRO_DO_OVAL_X * escalaDoEstadio);
}

/*
 * animarTorcida
 * A torcida se mexe de dia e de noite. Com tempo 0 (pausa), ela para.
 * Recebe: o tempo que passou.
 * Devolve: nada.
 */
void animarTorcida(float tempoQuadro) {
    tempoDaTorcida += tempoQuadro;
    if (andamentoDaOnda >= 0.0f) {
        andamentoDaOnda += tempoQuadro / SEGUNDOS_DA_ONDA;
        if (andamentoDaOnda >= 1.0f) {
            andamentoDaOnda = -1.0f;
        }
    }
}

/*
 * atualizarNoite
 * Solta os flashes novos e envelhece os acesos. Com tempo 0 (pausa), para tudo.
 * Recebe: o tempo que passou.
 * Devolve: nada.
 */
void atualizarNoite(float tempoQuadro) {
    for (int i = 0; i < MAXIMO_DE_FLASHES; i++) {
        flashes[i].idade += tempoQuadro;
    }
    flashesParaSoltar += FLASHES_POR_SEGUNDO * tempoQuadro;
    while (flashesParaSoltar >= 1.0f) {
        soltarFlash(0, 0.0f);
        flashesParaSoltar -= 1.0f;
    }
    /* Passando a onda, a torcida também dispara as câmeras */
    if (andamentoDaOnda >= 0.0f) {
        flashesDaOnda += FLASHES_NA_ONDA * tempoQuadro;
        while (flashesDaOnda >= 1.0f) {
            soltarFlash(1, inicioDaOnda + andamentoDaOnda * 2.0f * PI);
            flashesDaOnda -= 1.0f;
        }
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
 * O brilho dos refletores e os flashes, somando luz ao que está
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
        float distancia = Vector3Distance(refletores[i], camera.position);
        Vector3 paraACamera = Vector3Normalize(Vector3Subtract(camera.position, refletores[i]));
        Vector3 centro = Vector3Add(refletores[i], Vector3Scale(paraACamera, NA_FRENTE_DO_REFLETOR));
        desenharImagemNoAr(camera, imagemDoBrilho, centro, fmaxf(MENOR_HALO, distancia * HALO_DO_REFLETOR), COR_DO_HALO);
        desenharImagemNoAr(camera, imagemDoBrilho, centro, fmaxf(MENOR_MIOLO, distancia * MIOLO_DO_REFLETOR), COR_DO_MIOLO);
    }
    for (int i = 0; i < MAXIMO_DE_FLASHES; i++) {
        Flash *flash = &flashes[i];
        if (flash->vida <= 0.0f || flash->idade >= flash->vida) {
            continue;
        }
        float forca = 1.0f - flash->idade / flash->vida;
        float distancia = Vector3Distance(flash->lugar, camera.position);
        float tamanho = flash->tamanho * distancia;
        desenharImagemNoAr(camera, imagemDoBrilho, flash->lugar, tamanho * HALO_DO_FLASH, Fade(COR_FLASH_AZUL, forca * 0.40f));
        desenharImagemNoAr(camera, imagemDaEstrela, flash->lugar, tamanho, Fade(flash->cor, forca));
    }

    rlDrawRenderBatchActive();
    rlEnableDepthMask();
    EndBlendMode();
}
