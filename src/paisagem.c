/*
 * paisagem.c
 * ------------------------------------------------------------------
 * Veja paisagem.h. Tudo é sorteado com semente fixa, na hora de carregar: fica sempre
 * igual. A distância é pintada nos próprios vértices: o canal alfa diz quanto daquele
 * ponto o ar já escondeu, e o shader mistura com a cor do horizonte.
 * ------------------------------------------------------------------
 */

#include <math.h>
#include <stdlib.h>

#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"
#include "paisagem.h"
#include "construcao.h"

#define SEMENTE                 20261104u

/* ---------- O limite do autódromo ---------- */
#define ANGULOS_DO_LIMITE       180
#define FOLGA_DO_LIMITE         170.0f    /* metros além da pista */
#define JANELA_DO_LIMITE        7         /* quantos ângulos para cada lado entram na conta: arredonda */
#define ALTURA_DO_MURO_EXTERNO  4.0f
#define INICIO_DA_AVENIDA       5.0f      /* metros além do muro */
#define FIM_DA_AVENIDA          24.0f
#define INICIO_DAS_CASAS        34.0f
#define POSTE_A_CADA            3         /* pontos do limite */
#define CARROS_NA_AVENIDA       150

/* ---------- Morros ---------- */
#define RAIOS_DO_ANEL           200
#define RAIO_FINAL              8000.0f   /* o céu fica a 9 km da câmera: tudo tem que caber antes dele */
#define ALTURA_DOS_MORROS_PERTO 55.0f
#define ALTURA_DOS_MORROS_MEIO  170.0f
#define ALTURA_DA_SERRA         380.0f
#define ESCALA_DOS_MORROS       750.0f    /* metros: o tamanho de um morro */

/* ---------- Bairros ---------- */
#define PASTA_DOS_MODELOS       "recursos/modelos/objetos/"
#define MAXIMO_DE_IMAGENS       8
#define ALCANCE_DAS_CASAS_PERTO 340.0f    /* metros além do muro: até aqui, as casas com detalhe */
#define LADO_DO_MODELO_DA_QUADRA 84.0f
#define PASSO_DAS_CASAS         17.0f
#define CASAS_POR_QUADRA        6         /* a cada tantas, passa uma rua */
#define ALCANCE_DAS_CASAS       1100.0f   /* metros além do muro */
#define ALCANCE_DAS_QUADRAS     3600.0f
#define CHANCE_DE_CASA          0.82f
#define TOTAL_DE_GRUPOS         46        /* grupos de prédios */
#define PREDIOS_POR_GRUPO_MAX   22
#define PREDIOS_NO_CENTRO       70        /* o centro, ao longe */

/* ---------- Árvores ---------- */
#define TOTAL_DE_COPAS          6000
#define VERDE_PARA_MATA         0.60f     /* acima disso, no sorteio do relevo, é mata */

/* ---------- O lago ---------- */
#define FOLGA_DO_LAGO           64.0f     /* metros da pista até a margem: além do alcance do carro */
#define MAIOR_LAGO              80.0f
#define FUNDO_DO_LAGO           5.0f
#define ALTURA_DA_MARGEM        0.8f      /* acima da água */
#define FIM_DA_MARGEM           1.6f      /* em raios do lago */
#define LADOS_DO_LAGO           48
#define BORDA_DA_AGUA           1.12f     /* em raios: um pouco por baixo da margem */

/* ---------- O ar ---------- */
#define COR_DO_HORIZONTE_TEXTO  "vec3(0.66, 0.78, 0.90)"
#define INICIO_DO_AR            500.0f    /* metros do centro */
#define ALCANCE_DO_AR           8500.0f
#define MAIOR_AR                0.86f

static const char *VERTICES =
    "#version 330\n"
    "in vec3 vertexPosition; in vec2 vertexTexCoord; in vec4 vertexColor;\n"
    "uniform mat4 mvp;\n"
    "out vec2 fragTexCoord; out vec4 fragColor;\n"
    "void main() { fragTexCoord = vertexTexCoord; fragColor = vertexColor;\n"
    "              gl_Position = mvp * vec4(vertexPosition, 1.0); }\n";
static const char *PIXELS =
    "#version 330\n"
    "in vec2 fragTexCoord; in vec4 fragColor;\n"
    "uniform sampler2D texture0;\n"
    "out vec4 finalColor;\n"
    "void main() { vec3 cor = texture(texture0, fragTexCoord).rgb * fragColor.rgb;\n"
    "              finalColor = vec4(mix(cor, " COR_DO_HORIZONTE_TEXTO ", fragColor.a), 1.0); }\n";

static float centroX, centroZ;
static float limite[ANGULOS_DO_LIMITE];
static unsigned int semente = SEMENTE;
static float lagoX, lagoZ, raioDoLago = 0.0f, nivelDoLago = 0.0f;
static int nivelDefinido = 0;

static Shader shaderDaPaisagem;
static Material materialDaPaisagem;
static Construcao chaoDaCidade = { 0 };
static Mesh malhaDoLago, malhaDoChao, malhaDosMorros, malhaDoLimite, malhaDaCidade;

/* Os modelos da cidade, em recursos/modelos/objetos */
typedef enum {
    MODELO_POSTE,
    MODELO_CARRO_1, MODELO_CARRO_2, MODELO_CARRO_3, MODELO_CARRO_4, MODELO_CARRO_5, MODELO_CARRO_6,
    MODELO_CASA_1, MODELO_CASA_2, MODELO_CASA_3, MODELO_CASA_4, MODELO_CASA_5,
    MODELO_CASA_LONGE_1, MODELO_CASA_LONGE_2, MODELO_CASA_LONGE_3,
    MODELO_QUADRA_1, MODELO_QUADRA_2, MODELO_QUADRA_3,
    MODELO_PREDIO_1, MODELO_PREDIO_2, MODELO_PREDIO_3, MODELO_PREDIO_4, MODELO_PREDIO_5, MODELO_PREDIO_6,
    MODELO_COPA_1, MODELO_COPA_2, MODELO_COPA_3,
    TOTAL_DE_MODELOS
} ModeloDaPaisagem;

static const char *ARQUIVOS[TOTAL_DE_MODELOS] = {
    "poste_rua.glb", "carro_rua_1.glb", "carro_rua_2.glb", "carro_rua_3.glb", "carro_rua_4.glb", "carro_rua_5.glb", "carro_rua_6.glb",
    "casa_1.glb", "casa_2.glb", "casa_3.glb", "casa_4.glb", "casa_5.glb", "casa_longe_1.glb", "casa_longe_2.glb", "casa_longe_3.glb",
    "quadra_1.glb", "quadra_2.glb", "quadra_3.glb", "predio_1.glb", "predio_2.glb", "predio_3.glb", "predio_4.glb", "predio_5.glb", "predio_6.glb",
    "copa_1.glb", "copa_2.glb", "copa_3.glb",
};

static Model modelos[TOTAL_DE_MODELOS];
static Construcao semImagem = { 0 }, comImagem[MAXIMO_DE_IMAGENS] = { { 0 } };
static Texture2D imagens[MAXIMO_DE_IMAGENS];
static Mesh malhasComImagem[MAXIMO_DE_IMAGENS];
static int totalDeImagens = 0;

/*
 * sortear
 * Recebe: os limites.
 * Devolve: um número entre eles, sempre a mesma sequência.
 */
static float sortear(float minimo, float maximo) {
    semente = semente * 1103515245u + 12345u;
    float t = ((semente >> 16) & 0x7fff) / 32767.0f;
    return minimo + (maximo - minimo) * t;
}

/*
 * ruidoDeValor
 * Um valor de 0 a 1 que muda suavemente pelo mapa, sempre igual em cada lugar.
 * Recebe: o ponto, em células.
 * Devolve: o valor.
 */
static float ruidoDeValor(float x, float z) {
    float cx = floorf(x), cz = floorf(z);
    float fx = x - cx, fz = z - cz;
    fx = fx * fx * (3.0f - 2.0f * fx);
    fz = fz * fz * (3.0f - 2.0f * fz);
    float valores[4];
    for (int i = 0; i < 4; i++) {
        float h = sinf((cx + (i & 1)) * 127.1f + (cz + (i >> 1)) * 311.7f) * 43758.5453f;
        valores[i] = h - floorf(h);
    }
    float a = valores[0] + (valores[1] - valores[0]) * fx;
    float b = valores[2] + (valores[3] - valores[2]) * fx;
    return a + (b - a) * fz;
}

/*
 * ruidoEmCamadas
 * Recebe: o ponto, em células.
 * Devolve: morros grandes com morros menores em cima, de 0 a 1.
 */
static float ruidoEmCamadas(float x, float z) {
    return ruidoDeValor(x, z) * 0.58f + ruidoDeValor(x * 2.1f + 17.0f, z * 2.1f + 5.0f) * 0.29f
           + ruidoDeValor(x * 4.3f + 31.0f, z * 4.3f + 11.0f) * 0.13f;
}

/*
 * suaveEntre
 * Recebe: um valor e a faixa.
 * Devolve: 0 antes da faixa, 1 depois, e um caminho suave no meio.
 */
static void acharLago(Pista *pista);
static float raioRelativoDoLago(float x, float z);

static float suaveEntre(float valor, float de, float ate) {
    float t = fminf(fmaxf((valor - de) / (ate - de), 0.0f), 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

/*
 * limiteNoAngulo
 * Recebe: a direção a partir do centro, em radianos.
 * Devolve: a que distância do centro fica o muro do autódromo.
 */
static float limiteNoAngulo(float angulo) {
    float passo = 2.0f * PI / ANGULOS_DO_LIMITE;
    float posicao = fmodf(angulo + 4.0f * PI, 2.0f * PI) / passo;
    int a = (int) posicao % ANGULOS_DO_LIMITE;
    int b = (a + 1) % ANGULOS_DO_LIMITE;
    return limite[a] + (limite[b] - limite[a]) * (posicao - floorf(posicao));
}

/*
 * alemDoMuro
 * Recebe: o ponto.
 * Devolve: quantos metros ele está além do muro do autódromo (negativo: dentro).
 */
static float alemDoMuro(float x, float z) {
    float dx = x - centroX, dz = z - centroZ;
    return hypotf(dx, dz) - limiteNoAngulo(atan2f(dz, dx));
}

/*
 * prepararPaisagem
 * Acha o contorno do autódromo: um pouco além do ponto mais afastado da pista em cada
 * direção, com os ângulos vizinhos misturados para ficar arredondado.
 * Recebe: a pista.
 * Devolve: nada.
 */
void prepararPaisagem(Pista *pista) {
    float minX = 1e9f, maxX = -1e9f, minZ = 1e9f, maxZ = -1e9f;
    Segmento *atual = pista->largada;
    do {
        minX = fminf(minX, atual->xMundo);
        maxX = fmaxf(maxX, atual->xMundo);
        minZ = fminf(minZ, atual->zMundo);
        maxZ = fmaxf(maxZ, atual->zMundo);
        atual = atual->proximo;
    } while (atual != pista->largada);
    centroX = (minX + maxX) / 2.0f;
    centroZ = (minZ + maxZ) / 2.0f;

    float maisLonge[ANGULOS_DO_LIMITE] = { 0.0f };
    do {
        float dx = atual->xMundo - centroX, dz = atual->zMundo - centroZ;
        float angulo = fmodf(atan2f(dz, dx) + 2.0f * PI, 2.0f * PI);
        int i = (int) (angulo / (2.0f * PI) * ANGULOS_DO_LIMITE) % ANGULOS_DO_LIMITE;
        maisLonge[i] = fmaxf(maisLonge[i], hypotf(dx, dz));
        atual = atual->proximo;
    } while (atual != pista->largada);

    float aberto[ANGULOS_DO_LIMITE];
    for (int i = 0; i < ANGULOS_DO_LIMITE; i++) {
        aberto[i] = 0.0f;
        for (int j = -JANELA_DO_LIMITE; j <= JANELA_DO_LIMITE; j++) {
            aberto[i] = fmaxf(aberto[i], maisLonge[(i + j + ANGULOS_DO_LIMITE) % ANGULOS_DO_LIMITE]);
        }
    }
    for (int i = 0; i < ANGULOS_DO_LIMITE; i++) {
        float soma = 0.0f;
        for (int j = -3; j <= 3; j++) {
            soma += aberto[(i + j + ANGULOS_DO_LIMITE) % ANGULOS_DO_LIMITE];
        }
        limite[i] = soma / 7.0f + FOLGA_DO_LIMITE;
    }
    acharLago(pista);
}

/*
 * raioRelativoDoLago
 * Recebe: o ponto.
 * Devolve: a distância ao centro do lago, onde 1 é a margem (ela é irregular).
 */
static float raioRelativoDoLago(float x, float z) {
    if (raioDoLago <= 0.0f) {
        return 1e9f;
    }
    float dx = x - lagoX, dz = z - lagoZ;
    float angulo = atan2f(dz, dx);
    float margem = raioDoLago * (1.0f + 0.16f * sinf(angulo * 3.0f + 1.0f) + 0.09f * sinf(angulo * 5.0f));
    return hypotf(dx * 0.85f, dz * 1.15f) / margem;
}

/*
 * lugarDoLago
 * Recebe: onde guardar o centro do lago.
 * Devolve: nada.
 */
void lugarDoLago(float *x, float *z) {
    *x = lagoX;
    *z = lagoZ;
}

/*
 * definirNivelDoLago
 * Recebe: a altura do terreno no centro do lago, de onde sai o nível da água.
 * Devolve: nada.
 */
void definirNivelDoLago(float nivel) {
    nivelDoLago = nivel;
    nivelDefinido = 1;
}

/*
 * alturaComLago
 * Escava a bacia: o fundo no meio, uma margem um pouco acima da água em volta dela, e
 * daí o terreno volta aos poucos ao que era. Com a margem em toda a volta, a água não
 * fica pendurada, mesmo com o terreno inclinado.
 * Recebe: a altura do terreno sem o lago e o ponto.
 * Devolve: a altura com o lago.
 */
float alturaComLago(float altura, float x, float z) {
    float r = raioRelativoDoLago(x, z);
    if (!nivelDefinido || r > FIM_DA_MARGEM) {
        return altura;
    }
    float bacia = nivelDoLago - FUNDO_DO_LAGO + (FUNDO_DO_LAGO + ALTURA_DA_MARGEM) * suaveEntre(r, 0.6f, 1.2f);
    return bacia + (altura - bacia) * suaveEntre(r, 1.2f, FIM_DA_MARGEM);
}

/*
 * dentroDoLago
 * Recebe: o ponto.
 * Devolve: 1 se ele cairia na água ou na margem: nenhuma árvore nasce ali.
 */
int dentroDoLago(float x, float z) {
    return raioRelativoDoLago(x, z) < 1.3f;
}

/*
 * acharLago
 * O lago da Descida do Lago: no lugar dentro do circuito mais longe da pista, por ali.
 * Recebe: a pista.
 * Devolve: nada.
 */
static void acharLago(Pista *pista) {
    float melhor = 0.0f;
    for (float m = 1480.0f; m < 1740.0f; m += 30.0f) {
        for (float afastar = 3.0f; afastar < 11.0f; afastar += 0.6f) {
            PosicaoNaPista lugar = { pista->largada, 0.0f };
            moverNaPista(&lugar, m * UNIDADES_POR_METRO);
            PontoNoMundo p = pontoNoMundo(lugar, -afastar);
            float menor = 1e9f;
            Segmento *s = pista->largada;
            int i = 0;
            do {
                if (i++ % 3 == 0) {
                    menor = fminf(menor, hypotf(s->xMundo - p.x, s->zMundo - p.z));
                }
                s = s->proximo;
            } while (s != pista->largada);
            if (menor > melhor) {
                melhor = menor;
                lagoX = p.x;
                lagoZ = p.z;
            }
        }
    }
    raioDoLago = fminf((melhor - FOLGA_DO_LAGO) / 1.3f, MAIOR_LAGO);   /* a margem irregular chega a 1,3 raio */
    TraceLog(LOG_INFO, "PAISAGEM: lago em (%.0f, %.0f), raio %.0f m, %.0f m da pista", lagoX, lagoZ, raioDoLago, melhor);
}

/*
 * relevoDaPaisagem
 * Quanto o chão sobe por causa dos morros. Dentro do autódromo, nada.
 * Recebe: o ponto.
 * Devolve: os metros.
 */
float relevoDaPaisagem(float x, float z) {
    float d = alemDoMuro(x, z);
    if (d <= 0.0f) {
        return 0.0f;
    }
    float amplitude = ALTURA_DOS_MORROS_PERTO * suaveEntre(d, 0.0f, 1200.0f)
                      + ALTURA_DOS_MORROS_MEIO * suaveEntre(d, 1000.0f, 3500.0f)
                      + ALTURA_DA_SERRA * suaveEntre(d, 3500.0f, 7000.0f);
    return amplitude * ruidoEmCamadas(x / ESCALA_DOS_MORROS, z / ESCALA_DOS_MORROS);
}

/*
 * foraDoAutodromo
 * Recebe: o ponto.
 * Devolve: 1 se ele já está na cidade.
 */
int foraDoAutodromo(float x, float z) {
    return alemDoMuro(x, z) > INICIO_DA_AVENIDA;
}

/*
 * corDaGramaNoAutodromo
 * Recebe: o ponto.
 * Devolve: o tom da grama ali: muda aos poucos, com manchas naturais, em vez de um xadrez.
 */
Color corDaGramaNoAutodromo(float x, float z) {
    float mancha = ruidoEmCamadas(x / 70.0f + 40.0f, z / 70.0f + 9.0f);
    float t = 0.74f + 0.26f * mancha;
    return (Color){ (unsigned char) fminf(255.0f * t * 1.02f, 255.0f), (unsigned char) fminf(255.0f * t * 0.90f, 255.0f),
                    (unsigned char) fminf(255.0f * t * 0.78f, 255.0f), 255 };   /* a imagem é verde demais: puxa para o verde de um gramado de verdade */
}

/*
 * arNoPonto
 * Recebe: o ponto.
 * Devolve: quanto do ponto o ar esconde, no alfa da cor.
 */
static unsigned char arNoPonto(Vector3 p) {
    float d = hypotf(p.x - centroX, p.z - centroZ);
    float t = fminf(fmaxf((d - INICIO_DO_AR) / ALCANCE_DO_AR, 0.0f), 1.0f);
    return (unsigned char) (255.0f * MAIOR_AR * sqrtf(t));
}

/*
 * corDoSolo
 * A cor de um pedaço de chão visto de longe: bairro ou mata, conforme o sorteio do relevo.
 * Recebe: o ponto.
 * Devolve: a cor, com o ar no alfa.
 */
static Color corDoSolo(Vector3 p) {
    float verde = ruidoEmCamadas(p.x / 480.0f + 3.0f, p.z / 480.0f + 7.0f);
    float longe = suaveEntre(alemDoMuro(p.x, p.z), 2500.0f, 6000.0f);
    float mata = suaveEntre(verde + longe * 0.22f, VERDE_PARA_MATA - 0.06f, VERDE_PARA_MATA + 0.06f);
    float grao = 0.86f + 0.28f * ruidoDeValor(p.x / 45.0f, p.z / 45.0f);
    Vector3 cidade = { 132.0f, 112.0f, 96.0f }, floresta = { 56.0f, 92.0f, 52.0f };
    Vector3 cor = Vector3Lerp(cidade, floresta, mata);
    return (Color){ (unsigned char) fminf(cor.x * grao, 255.0f), (unsigned char) fminf(cor.y * grao, 255.0f),
                    (unsigned char) fminf(cor.z * grao, 255.0f), arNoPonto(p) };
}

/*
 * adicionarChaoDaCidade
 * Uma célula do terreno que caiu fora do autódromo.
 * Recebe: os 4 cantos.
 * Devolve: nada.
 */
void adicionarChaoDaCidade(Vector3 a, Vector3 b, Vector3 d, Vector3 e) {
    Vector2 zero = { 0.0f, 0.0f };
    adicionarVertice(&chaoDaCidade, a, zero, corDoSolo(a));
    adicionarVertice(&chaoDaCidade, b, zero, corDoSolo(b));
    adicionarVertice(&chaoDaCidade, d, zero, corDoSolo(d));
    adicionarVertice(&chaoDaCidade, a, zero, corDoSolo(a));
    adicionarVertice(&chaoDaCidade, d, zero, corDoSolo(d));
    adicionarVertice(&chaoDaCidade, e, zero, corDoSolo(e));
}

/*
 * corComSol
 * Recebe: a cor, a direção da face e o ponto.
 * Devolve: a cor com a luz do sol e o ar no alfa.
 */
static Color corComSol(Color cor, Vector3 normal, Vector3 p) {
    float luz = luzDoSol(normal);
    return (Color){ (unsigned char) fminf(cor.r * luz, 255.0f), (unsigned char) fminf(cor.g * luz, 255.0f),
                    (unsigned char) fminf(cor.b * luz, 255.0f), arNoPonto(p) };
}

/*
 * carregarModelosDaPaisagem
 * Recebe: nada.
 * Devolve: nada.
 */
static void carregarModelosDaPaisagem(void) {
    for (int i = 0; i < TOTAL_DE_MODELOS; i++) {
        modelos[i] = LoadModel(TextFormat("%s%s", PASTA_DOS_MODELOS, ARQUIVOS[i]));
        for (int m = 0; m < modelos[i].materialCount; m++) {
            Texture2D *textura = &modelos[i].materials[m].maps[MATERIAL_MAP_ALBEDO].texture;
            if (textura->id != 0 && textura->id != rlGetTextureIdDefault()) {
                SetTextureWrap(*textura, TEXTURE_WRAP_REPEAT);
                GenTextureMipmaps(textura);
                SetTextureFilter(*textura, TEXTURE_FILTER_TRILINEAR);
            }
        }
    }
}

/*
 * destinoDaTextura
 * Recebe: a textura de uma peça.
 * Devolve: a construção onde juntar as peças com esta textura: uma para cada imagem,
 *          e todas as sem imagem numa só.
 */
static Construcao *destinoDaTextura(Texture2D textura) {
    if (textura.id == 0 || textura.id == rlGetTextureIdDefault()) {
        return &semImagem;
    }
    for (int i = 0; i < totalDeImagens; i++) {
        if (imagens[i].id == textura.id) {
            return &comImagem[i];
        }
    }
    if (totalDeImagens == MAXIMO_DE_IMAGENS) {
        return &semImagem;
    }
    imagens[totalDeImagens] = textura;
    return &comImagem[totalDeImagens++];
}

/*
 * copiarModelo
 * Copia um modelo para as malhas da paisagem, já no lugar, com o sol e o ar nos vértices:
 * milhares de casas viram uma malha só, e a placa de vídeo desenha tudo de uma vez.
 * Recebe: o modelo, o pé, o giro e o tamanho.
 * Devolve: nada.
 */
static void copiarModelo(ModeloDaPaisagem qual, Vector3 pe, float giro, float tamanho) {
    Model modelo = modelos[qual];
    Matrix noMundo = MatrixMultiply(MatrixMultiply(MatrixScale(tamanho, tamanho, tamanho), MatrixRotateY(giro)), MatrixTranslate(pe.x, pe.y, pe.z));
    Matrix soGiro = MatrixRotateY(giro);
    for (int i = 0; i < modelo.meshCount; i++) {
        Mesh peca = modelo.meshes[i];
        Material material = modelo.materials[modelo.meshMaterial[i]];
        Color cor = material.maps[MATERIAL_MAP_ALBEDO].color;
        Construcao *destino = destinoDaTextura(material.maps[MATERIAL_MAP_ALBEDO].texture);
        int total = (peca.indices != NULL) ? peca.triangleCount * 3 : peca.vertexCount;
        for (int k = 0; k < total; k++) {
            int v = (peca.indices != NULL) ? peca.indices[k] : k;
            Vector3 ponto = Vector3Transform((Vector3){ peca.vertices[v * 3], peca.vertices[v * 3 + 1], peca.vertices[v * 3 + 2] }, noMundo);
            Vector3 normal = { 0.0f, 1.0f, 0.0f };
            if (peca.normals != NULL) {
                normal = Vector3Transform((Vector3){ peca.normals[v * 3], peca.normals[v * 3 + 1], peca.normals[v * 3 + 2] }, soGiro);
            }
            Vector2 uv = { 0.0f, 0.0f };
            if (peca.texcoords != NULL) {
                uv = (Vector2){ peca.texcoords[v * 2], peca.texcoords[v * 2 + 1] };
            }
            adicionarVertice(destino, ponto, uv, corComSol(cor, normal, ponto));
        }
    }
}

/*
 * construirCasas
 * Os bairros em volta do autódromo: quadras de casas, com as ruas passando entre elas. As
 * perto têm janelas e muro; mais longe, formas simples; e bem longe, a quadra inteira.
 * Recebe: a pista e a altura.
 * Devolve: nada.
 */
static void construirCasas(Pista *pista, AlturaNoMundo altura) {
    float raio = 0.0f;
    for (int i = 0; i < ANGULOS_DO_LIMITE; i++) {
        raio = fmaxf(raio, limite[i]);
    }
    raio += ALCANCE_DAS_CASAS;
    int fileiras = (int) (raio / PASSO_DAS_CASAS);
    for (int l = -fileiras; l <= fileiras; l++) {
        if ((l + 1000 * CASAS_POR_QUADRA) % CASAS_POR_QUADRA == 0) {
            continue;   /* uma rua */
        }
        for (int k = -fileiras; k <= fileiras; k++) {
            if ((k + 1000 * CASAS_POR_QUADRA) % CASAS_POR_QUADRA == 0) {
                continue;
            }
            float x = centroX + k * PASSO_DAS_CASAS, z = centroZ + l * PASSO_DAS_CASAS;
            float d = alemDoMuro(x, z);
            float sorte = sortear(0.0f, 1.0f), variacao = sortear(0.0f, 1.0f), tamanho = sortear(0.92f, 1.12f);
            float sobra = sortear(-1.5f, 1.5f), giro = (l % 2 == 0) ? 0.0f : PI;
            if (d < INICIO_DAS_CASAS || d > ALCANCE_DAS_CASAS || sorte > CHANCE_DE_CASA * (1.0f - 0.45f * d / ALCANCE_DAS_CASAS)) {
                continue;
            }
            Vector3 pe = { x + sobra, altura(pista, x, z) - 0.2f, z };
            if (d < ALCANCE_DAS_CASAS_PERTO) {
                copiarModelo((ModeloDaPaisagem) (MODELO_CASA_1 + (int) (variacao * 4.99f)), pe, giro, tamanho);
            } else {
                copiarModelo((ModeloDaPaisagem) (MODELO_CASA_LONGE_1 + (int) (variacao * 2.99f)), pe, giro, tamanho);
            }
        }
    }

    /* Mais longe, cada quadra vira um modelo só: de lá, ninguém conta casa por casa */
    float passoDaQuadra = PASSO_DAS_CASAS * CASAS_POR_QUADRA;
    raio += ALCANCE_DAS_QUADRAS - ALCANCE_DAS_CASAS;
    int quadras = (int) (raio / passoDaQuadra);
    for (int l = -quadras; l <= quadras; l++) {
        for (int k = -quadras; k <= quadras; k++) {
            float x = centroX + (k + 0.5f) * passoDaQuadra, z = centroZ + (l + 0.5f) * passoDaQuadra;
            float d = alemDoMuro(x, z);
            float verde = ruidoEmCamadas(x / 480.0f + 3.0f, z / 480.0f + 7.0f);
            float sorte = sortear(0.0f, 1.0f), variacao = sortear(0.0f, 2.99f), vira = roundf(sortear(0.0f, 3.0f)) * PI / 2.0f;
            if (d < ALCANCE_DAS_CASAS || d > ALCANCE_DAS_QUADRAS || verde > VERDE_PARA_MATA || sorte > 0.85f) {
                continue;
            }
            Vector3 pe = { x, altura(pista, x, z) - 0.3f, z };
            copiarModelo((ModeloDaPaisagem) (MODELO_QUADRA_1 + (int) variacao), pe, vira, passoDaQuadra / LADO_DO_MODELO_DA_QUADRA * 0.9f);
        }
    }
}

/*
 * construirPredios
 * Grupos de prédios espalhados pelos bairros (mais baixos perto do autódromo) e, ao longe,
 * o centro da cidade, com as torres mais altas.
 * Recebe: a pista e a altura.
 * Devolve: nada.
 */
static void construirPredios(Pista *pista, AlturaNoMundo altura) {
    static const ModeloDaPaisagem PERTO[] = { MODELO_PREDIO_1, MODELO_PREDIO_2, MODELO_PREDIO_5, MODELO_PREDIO_1 };
    static const ModeloDaPaisagem LONGE[] = { MODELO_PREDIO_2, MODELO_PREDIO_3, MODELO_PREDIO_4, MODELO_PREDIO_5 };
    static const ModeloDaPaisagem CENTRO[] = { MODELO_PREDIO_3, MODELO_PREDIO_4, MODELO_PREDIO_6, MODELO_PREDIO_6 };
    for (int g = 0; g < TOTAL_DE_GRUPOS; g++) {
        float angulo = sortear(0.0f, 2.0f * PI), alem = sortear(90.0f, 3400.0f);
        float raio = limiteNoAngulo(angulo) + alem;
        float grupoX = centroX + cosf(angulo) * raio, grupoZ = centroZ + sinf(angulo) * raio;
        int quantos = (int) sortear(5.0f, (float) PREDIOS_POR_GRUPO_MAX);
        for (int i = 0; i < quantos; i++) {
            float x = grupoX + sortear(-170.0f, 170.0f), z = grupoZ + sortear(-170.0f, 170.0f);
            float tamanho = sortear(0.85f, 1.2f), giro = roundf(sortear(0.0f, 3.0f)) * 0.26f;
            int qual = (int) sortear(0.0f, 3.99f);
            if (alemDoMuro(x, z) < 60.0f) {
                continue;
            }
            Vector3 pe = { x, altura(pista, x, z) - 1.0f, z };
            copiarModelo((alem < 900.0f) ? PERTO[qual] : LONGE[qual], pe, giro, tamanho);
        }
    }

    float anguloDoCentro = 3.6f, raioDoCentro = limiteNoAngulo(anguloDoCentro) + 3600.0f;
    float cx = centroX + cosf(anguloDoCentro) * raioDoCentro, cz = centroZ + sinf(anguloDoCentro) * raioDoCentro;
    for (int i = 0; i < PREDIOS_NO_CENTRO; i++) {
        float d = sortear(0.0f, 1.0f), direcao = sortear(0.0f, 2.0f * PI);
        float x = cx + cosf(direcao) * d * 720.0f, z = cz + sinf(direcao) * d * 420.0f;
        float tamanho = sortear(0.95f, 1.5f), giro = roundf(sortear(0.0f, 3.0f)) * 0.26f;
        int qual = (int) sortear(0.0f, 3.99f);
        Vector3 pe = { x, altura(pista, x, z) - 1.0f, z };
        copiarModelo(CENTRO[qual], pe, giro, tamanho);
    }
}

/*
 * construirCopas
 * As matas e as árvores de rua, em manchas.
 * Recebe: a pista e a altura.
 * Devolve: nada.
 */
static void construirCopas(Pista *pista, AlturaNoMundo altura) {
    int plantadas = 0;
    for (int tentativa = 0; tentativa < TOTAL_DE_COPAS * 12 && plantadas < TOTAL_DE_COPAS; tentativa++) {
        float angulo = sortear(0.0f, 2.0f * PI), alem = sortear(0.0f, 1.0f);
        alem = 30.0f + alem * alem * 4200.0f;       /* mais árvores perto, onde se vê cada uma */
        float raio = limiteNoAngulo(angulo) + alem;
        float x = centroX + cosf(angulo) * raio, z = centroZ + sinf(angulo) * raio;
        float verde = ruidoEmCamadas(x / 480.0f + 3.0f, z / 480.0f + 7.0f);
        float sorte = sortear(0.0f, 1.0f), qual = sortear(0.0f, 1.0f), giro = sortear(0.0f, 2.0f * PI), tamanho = sortear(0.8f, 1.5f);
        if (verde < VERDE_PARA_MATA - 0.05f && sorte > 0.05f) {
            continue;
        }
        Vector3 pe = { x, altura(pista, x, z) - 0.5f, z };
        ModeloDaPaisagem modelo = (qual < 0.7f) ? MODELO_COPA_1 : (qual < 0.9f) ? MODELO_COPA_3 : MODELO_COPA_2;
        copiarModelo(modelo, pe, giro, tamanho * (1.0f + alem / 1400.0f));   /* de longe, cada uma vale por um grupo */
        plantadas++;
    }
}

/*
 * construirMorros
 * Um anel do fim do terreno até o horizonte. Ele usa a mesma conta de altura que o terreno,
 * então os dois se encontram sem degrau; o primeiro círculo fica um pouco abaixo e por dentro.
 * Recebe: a pista, a altura e os limites do terreno.
 * Devolve: nada.
 */
static void construirMorros(Pista *pista, AlturaNoMundo altura, float minX, float maxX, float minZ, float maxZ) {
    static const float PASSOS[] = { -40.0f, 40.0f, 120.0f, 240.0f, 400.0f, 600.0f, 850.0f, 1150.0f, 1500.0f, 1900.0f, 2350.0f, 2850.0f, 3400.0f, 4000.0f, 4700.0f, 5500.0f };
    enum { TOTAL_DE_PASSOS = sizeof(PASSOS) / sizeof(PASSOS[0]) };
    Vector3 anterior[RAIOS_DO_ANEL + 1];
    Construcao morros = { 0 };
    for (int anel = 0; anel < TOTAL_DE_PASSOS; anel++) {
        Vector3 atual[RAIOS_DO_ANEL + 1];
        for (int r = 0; r <= RAIOS_DO_ANEL; r++) {
            float angulo = 2.0f * PI * (r % RAIOS_DO_ANEL) / RAIOS_DO_ANEL;
            float cosseno = cosf(angulo), seno = sinf(angulo);

            /* Onde este raio sai do retângulo do terreno */
            float ateX = (cosseno > 0.0f) ? (maxX - centroX) / cosseno : (cosseno < 0.0f) ? (minX - centroX) / cosseno : 1e9f;
            float ateZ = (seno > 0.0f) ? (maxZ - centroZ) / seno : (seno < 0.0f) ? (minZ - centroZ) / seno : 1e9f;
            float distancia = fminf(fminf(ateX, ateZ) + PASSOS[anel], RAIO_FINAL);
            float x = centroX + cosseno * distancia, z = centroZ + seno * distancia;
            atual[r] = (Vector3){ x, altura(pista, x, z) - ((anel == 0) ? 2.0f : 0.0f), z };
        }
        if (anel > 0) {
            Vector2 zero = { 0.0f, 0.0f };
            for (int r = 0; r < RAIOS_DO_ANEL; r++) {
                Vector3 a = anterior[r], b = atual[r], d = atual[r + 1], e = anterior[r + 1];
                Vector3 normal = Vector3CrossProduct(Vector3Subtract(d, a), Vector3Subtract(b, a));
                float luz = luzDoSol(normal.y < 0.0f ? Vector3Negate(normal) : normal);
                Color cores[4] = { corDoSolo(a), corDoSolo(b), corDoSolo(d), corDoSolo(e) };
                for (int i = 0; i < 4; i++) {
                    cores[i].r = (unsigned char) fminf(cores[i].r * luz, 255.0f);
                    cores[i].g = (unsigned char) fminf(cores[i].g * luz, 255.0f);
                    cores[i].b = (unsigned char) fminf(cores[i].b * luz, 255.0f);
                }
                adicionarVertice(&morros, a, zero, cores[0]);
                adicionarVertice(&morros, b, zero, cores[1]);
                adicionarVertice(&morros, d, zero, cores[2]);
                adicionarVertice(&morros, a, zero, cores[0]);
                adicionarVertice(&morros, d, zero, cores[2]);
                adicionarVertice(&morros, e, zero, cores[3]);
            }
        }
        for (int r = 0; r <= RAIOS_DO_ANEL; r++) {
            anterior[r] = atual[r];
        }
    }
    malhaDosMorros = terminarMalha(&morros);
}

/*
 * construirLimite
 * O muro do autódromo, a avenida que passa em volta, os postes e o trânsito.
 * Recebe: a pista e a altura.
 * Devolve: nada.
 */
static void construirLimite(Pista *pista, AlturaNoMundo altura) {
    Construcao c = { 0 };
    Vector2 zero = { 0.0f, 0.0f };
    Color concreto = { 172, 170, 162, 0 }, asfalto = { 72, 72, 76, 0 }, calcada = { 158, 154, 146, 0 };
    for (int i = 0; i < ANGULOS_DO_LIMITE * 2; i++) {
        float passo = PI / ANGULOS_DO_LIMITE;
        float a0 = i * passo, a1 = (i + 1) * passo;
        Vector3 muro[2], faixas[2][4];
        for (int k = 0; k < 2; k++) {
            float angulo = k ? a1 : a0, raio = limiteNoAngulo(angulo);
            float cosseno = cosf(angulo), seno = sinf(angulo);
            float x = centroX + cosseno * raio, z = centroZ + seno * raio;
            muro[k] = (Vector3){ x, altura(pista, x, z), z };
            float larguras[4] = { 0.0f, INICIO_DA_AVENIDA, FIM_DA_AVENIDA, INICIO_DAS_CASAS };
            for (int j = 0; j < 4; j++) {
                float xj = centroX + cosseno * (raio + larguras[j]), zj = centroZ + seno * (raio + larguras[j]);
                faixas[k][j] = (Vector3){ xj, altura(pista, xj, zj) + 0.9f, zj };
            }
        }
        Vector3 cima = { 0.0f, ALTURA_DO_MURO_EXTERNO, 0.0f };
        Vector3 abaixo = { 0.0f, -3.0f, 0.0f };
        Vector3 normal = Vector3CrossProduct(Vector3Subtract(muro[1], muro[0]), (Vector3){ 0.0f, 1.0f, 0.0f });
        adicionarQuadrilatero(&c, Vector3Add(muro[0], abaixo), Vector3Add(muro[1], abaixo), Vector3Add(muro[1], cima), Vector3Add(muro[0], cima),
                              zero, zero, zero, zero, corComSol(concreto, normal, muro[0]));
        for (int j = 0; j < 3; j++) {
            Color cor = (j == 1) ? asfalto : calcada;
            adicionarQuadrilatero(&c, faixas[0][j], faixas[1][j], faixas[1][j + 1], faixas[0][j + 1], zero, zero, zero, zero,
                                  corComSol(cor, (Vector3){ 0.0f, 1.0f, 0.0f }, faixas[0][j]));
        }
        if (i % POSTE_A_CADA == 0) {
            Vector3 pe = Vector3Lerp(faixas[0][0], faixas[0][1], 0.5f);
            copiarModelo(MODELO_POSTE, pe, atan2f(cosf(a0), sinf(a0)), 1.0f);   /* o braço do poste olha para a avenida */
        }
    }

    /* O trânsito, na avenida */
    for (int i = 0; i < CARROS_NA_AVENIDA; i++) {
        float angulo = sortear(0.0f, 2.0f * PI), raio = limiteNoAngulo(angulo) + sortear(INICIO_DA_AVENIDA + 3.0f, FIM_DA_AVENIDA - 3.0f);
        float x = centroX + cosf(angulo) * raio, z = centroZ + sinf(angulo) * raio;
        Vector3 pe = { x, altura(pista, x, z) + 0.9f, z };
        copiarModelo((ModeloDaPaisagem) (MODELO_CARRO_1 + i % 6), pe, -angulo, 1.0f);
    }
    malhaDoLimite = terminarMalha(&c);
}

/*
 * soltarCopiaDaMalha
 * A cidade tem milhões de vértices: depois de enviados à placa de vídeo, a cópia na
 * memória não serve mais para nada.
 * Recebe: a malha.
 * Devolve: nada.
 */
static void soltarCopiaDaMalha(Mesh *malha) {
    MemFree(malha->vertices);
    MemFree(malha->texcoords);
    MemFree(malha->colors);
    malha->vertices = NULL;
    malha->texcoords = NULL;
    malha->colors = NULL;
}

/*
 * construirPaisagem
 * Levanta tudo. Chamar depois de construir o terreno.
 * Recebe: a pista, a função da altura do chão e os limites do terreno.
 * Devolve: nada.
 */
void construirPaisagem(Pista *pista, AlturaNoMundo altura, float minX, float maxX, float minZ, float maxZ) {
    shaderDaPaisagem = LoadShaderFromMemory(VERTICES, PIXELS);
    materialDaPaisagem = LoadMaterialDefault();
    materialDaPaisagem.shader = shaderDaPaisagem;

    carregarModelosDaPaisagem();
    malhaDoChao = terminarMalha(&chaoDaCidade);
    construirMorros(pista, altura, minX, maxX, minZ, maxZ);
    construirLimite(pista, altura);
    construirCasas(pista, altura);
    construirPredios(pista, altura);
    construirCopas(pista, altura);
    malhaDaCidade = terminarMalha(&semImagem);
    soltarCopiaDaMalha(&malhaDaCidade);
    for (int i = 0; i < totalDeImagens; i++) {
        malhasComImagem[i] = terminarMalha(&comImagem[i]);
        soltarCopiaDaMalha(&malhasComImagem[i]);
    }

    /* A água, plana: quem desenha a beira é o próprio terreno, subindo até a margem */
    float nivel = nivelDoLago;
    Construcao lago = { 0 };
    Vector2 zero = { 0.0f, 0.0f };
    Vector3 centro = { lagoX, nivel, lagoZ };
    for (int i = 0; i < LADOS_DO_LAGO; i++) {
        Vector3 pontos[2];
        for (int k = 0; k < 2; k++) {
            /* Acha, em cada direção, onde cai a borda: a água segue a margem irregular */
            float a = 2.0f * PI * (i + k) / LADOS_DO_LAGO, co = cosf(a), se = sinf(a);
            float margem = raioDoLago * (1.0f + 0.16f * sinf(a * 3.0f + 1.0f) + 0.09f * sinf(a * 5.0f));
            float r = BORDA_DA_AGUA * margem / hypotf(co * 0.85f, se * 1.15f);
            pontos[k] = (Vector3){ lagoX + co * r, nivel, lagoZ + se * r };
        }
        adicionarVertice(&lago, centro, zero, (Color){ 44, 96, 128, 0 });
        adicionarVertice(&lago, pontos[1], zero, (Color){ 64, 124, 150, 0 });
        adicionarVertice(&lago, pontos[0], zero, (Color){ 64, 124, 150, 0 });
    }
    malhaDoLago = terminarMalha(&lago);

    TraceLog(LOG_INFO, "PAISAGEM: cidade %d vertices, %d imagens", malhaDaCidade.vertexCount, totalDeImagens);
}

/*
 * desenharMalhaDaPaisagem
 * Recebe: a malha e a textura (id 0 = sem textura).
 * Devolve: nada.
 */
static void desenharMalhaDaPaisagem(Mesh malha, Texture2D textura) {
    Texture2D branca = { rlGetTextureIdDefault(), 1, 1, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };
    materialDaPaisagem.maps[MATERIAL_MAP_ALBEDO].texture = (textura.id != 0) ? textura : branca;
    DrawMesh(malha, materialDaPaisagem, MatrixIdentity());
}

/*
 * desenharPaisagem
 * Recebe: nada.
 * Devolve: nada.
 */
void desenharPaisagem(void) {
    Texture2D semTextura = { 0 };
    desenharMalhaDaPaisagem(malhaDoLago, semTextura);
    desenharMalhaDaPaisagem(malhaDosMorros, semTextura);
    desenharMalhaDaPaisagem(malhaDoChao, semTextura);
    desenharMalhaDaPaisagem(malhaDoLimite, semTextura);
    desenharMalhaDaPaisagem(malhaDaCidade, semTextura);
    for (int i = 0; i < totalDeImagens; i++) {
        desenharMalhaDaPaisagem(malhasComImagem[i], imagens[i]);
    }
}

/*
 * descarregarPaisagem
 * Recebe: nada.
 * Devolve: nada.
 */
void descarregarPaisagem(void) {
    UnloadMesh(malhaDoLago);
    UnloadMesh(malhaDoChao);
    UnloadMesh(malhaDosMorros);
    UnloadMesh(malhaDaCidade);
    UnloadMesh(malhaDoLimite);
    for (int i = 0; i < totalDeImagens; i++) {
        UnloadMesh(malhasComImagem[i]);
    }
    for (int i = 0; i < TOTAL_DE_MODELOS; i++) {
        UnloadModel(modelos[i]);
    }
    materialDaPaisagem.maps[MATERIAL_MAP_ALBEDO].texture.id = rlGetTextureIdDefault();
    materialDaPaisagem.shader.id = rlGetShaderIdDefault();
    UnloadMaterial(materialDaPaisagem);
    UnloadShader(shaderDaPaisagem);
}
