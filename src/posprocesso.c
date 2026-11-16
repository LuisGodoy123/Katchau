/*
 * posprocesso.c
 * ------------------------------------------------------------------
 * Os efeitos de tela. Veja posprocesso.h.
 *
 * O caminho de cada quadro:
 *   1) a cena 3D vai para "cena", do tamanho da janela. A cor é guardada em meio float (16 bits por
 *      canal) com as cores HDR ligadas: o sol, os reflexos dele na lataria e as lâmpadas passam do
 *      branco sem serem cortados. A profundidade fica numa textura, que os shaders daqui leem;
 *   2) o que emite luz (refletores, flashes, chamas do nitro, telas dos telões)
 *      é desenhado de novo numa imagem de 1/4 do tamanho e borrado duas vezes
 *      em cada direção: esse é o halo (o branco só iluminado não brilha). No
 *      passeio (comecarLuzesComProfundidade), as luzes vão para uma imagem do
 *      tamanho da cena que usa a profundidade dela: o que está na frente (um
 *      prédio, um carro, o Mack) tapa a luz, e o halo não atravessa nada; essa
 *      imagem é reduzida a 1/4 antes de borrar. Com as cores HDR, o que passou do
 *      branco na cena (o sol, o reflexo dele num carro) entra no halo também;
 *   3) os raios de sol (no passeio, com o sol na frente): o céu em volta do sol, tapado pelo que está
 *      na frente dele, é espalhado a partir do sol, numa imagem de 1/4 do tamanho;
 *   4) na tela, um shader junta a cena e o halo, põe a neblina da distância (pela profundidade), o
 *      desfoque do fundo bem longe (opção), os raios e o reflexo do sol na lente, passa a luz pela curva
 *      de cinema (o que passou do branco chega nele aos poucos), corrige a cor (o tom de cada pista,
 *      ou o da hora no passeio, contraste e saturação), escurece as bordas (vinheta) e, em alta
 *      velocidade, borra as bordas na direção do centro.
 * ------------------------------------------------------------------
 */

#include <stddef.h>
#include <math.h>
#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"
#include "posprocesso.h"
#include "pista.h"
#include "configuracoes.h"

#define REDUCAO_DO_HALO   4       /* o halo é feito em 1/4 da tela: barato e mais largo */
#define CEU_A_PARTIR_DE   "7000.0"  /* metros: dali para lá é o céu (o domo fica a 9 km da câmera) */
#define LIMIAR_DO_HDR     1.0f    /* o que passa disso na cena (o branco) brilha */
#define FORCA_DO_HDR_NO_HALO 1.6f
#define EXPOSICAO         1.04f   /* com a curva de cinema: o meio-tom sobe um tico, o branco desce um tico */
#define FOCO_AO_LONGE     160.0f  /* metros: com a opção da profundidade de campo, o que passa disso desfoca... */
#define BORRAO_AO_LONGE   0.0030f /* ...até tanto (da altura da imagem) */
#define ALCANCE_DOS_RAIOS 0.85f   /* da distância até o sol: o comprimento dos raios */

/* O tom de cada pista */
typedef struct {
    Vector3 tom;
    float contraste, saturacao, forcaDoHalo, limiar, vinheta;
} TomDaPista;

static const TomDaPista TONS[TOTAL_PISTAS] = {
    { { 1.04f, 1.00f, 0.94f }, 1.07f, 1.12f, 0.70f, 0.0f, 0.28f },   /* Interlagos: tarde quente */
    { { 0.95f, 0.99f, 1.07f }, 1.08f, 1.10f, 0.85f, 0.0f, 0.34f },   /* Copa Pistão: noite azulada, luzes fortes */
    { { 1.02f, 1.00f, 0.97f }, 1.05f, 1.16f, 0.70f, 0.0f, 0.26f },   /* Oval: dia de cartoon */
    { { 1.02f, 1.00f, 0.97f }, 1.06f, 1.12f, 0.70f, 0.0f, 0.27f },   /* Montreal: fim de tarde de primavera, mais fresco */
};

/* A neblina de cada pista: a cor (perto da do horizonte do céu de cada uma), a densidade no chão, por
 * metro, e o máximo. Leve: a 1 km cobre um quinto, e só dá a distância às arquibancadas e morros do fundo */
typedef struct {
    Vector3 cor;
    float densidade, maximo;
} NeblinaDaPista;

static const NeblinaDaPista NEBLINAS[TOTAL_PISTAS] = {
    { { 0.76f, 0.80f, 0.86f }, 0.00026f, 0.55f },   /* Interlagos: a bruma da cidade */
    { { 0.05f, 0.06f, 0.12f }, 0.00030f, 0.60f },   /* Copa Pistão: a noite engole o longe */
    { { 0.74f, 0.83f, 0.95f }, 0.00020f, 0.50f },   /* Oval */
    { { 0.74f, 0.80f, 0.88f }, 0.00022f, 0.50f },   /* Montreal: o ar do rio e a cidade do outro lado */
};
#define ALTURA_DA_NEBLINA_DAS_PISTAS 140.0f   /* metros acima do chão da câmera em que ela fica e vezes mais rala */

/* O brilho que passa do branco (com as cores HDR): cada pixel do halo junta os 16 da cena embaixo dele em
 * quatro leituras filtradas (cada uma já é a média de 4), e cada leitura entra pelo quanto passou do limiar */
static const char *PIXELS_CLAROS =
    "#version 330\n"
    "in vec2 fragTexCoord; uniform sampler2D texture0; uniform vec2 passo; uniform float limiar; uniform float forca;\n"
    "out vec4 finalColor;\n"
    "vec3 acima(vec2 o) { return max(texture(texture0, fragTexCoord + passo * o).rgb - limiar, 0.0); }\n"
    "void main() {\n"
    "    vec3 soma = acima(vec2(-1.0, -1.0)) + acima(vec2(1.0, -1.0)) + acima(vec2(-1.0, 1.0)) + acima(vec2(1.0, 1.0));\n"
    "    finalColor = vec4(min(soma * 0.25, vec3(4.0)) * forca, 1.0);\n"
    "}\n";

static const char *PIXELS_BORRAR =
    "#version 330\n"
    "in vec2 fragTexCoord; uniform sampler2D texture0; uniform vec2 passo;\n"
    "out vec4 finalColor;\n"
    "void main() {\n"
    "    float p[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);\n"
    "    vec3 c = texture(texture0, fragTexCoord).rgb * p[0];\n"
    "    for (int i = 1; i < 5; i++) {\n"
    "        c += texture(texture0, fragTexCoord + passo * float(i) * 1.5).rgb * p[i];\n"
    "        c += texture(texture0, fragTexCoord - passo * float(i) * 1.5).rgb * p[i];\n"
    "    }\n"
    "    finalColor = vec4(c, 1.0);\n"
    "}\n";

/* Reduz a imagem das luzes a 1/4: a média de um quadrado de 4x4 (um tubo fino não some) */
static const char *PIXELS_REDUZIR =
    "#version 330\n"
    "in vec2 fragTexCoord; uniform sampler2D texture0; uniform vec2 passo;\n"
    "out vec4 finalColor;\n"
    "void main() {\n"
    "    vec3 c = texture(texture0, fragTexCoord + passo * vec2(-1.0, -1.0)).rgb + texture(texture0, fragTexCoord + passo * vec2(1.0, -1.0)).rgb\n"
    "           + texture(texture0, fragTexCoord + passo * vec2(-1.0, 1.0)).rgb + texture(texture0, fragTexCoord + passo * vec2(1.0, 1.0)).rgb;\n"
    "    finalColor = vec4(c * 0.25, 1.0);\n"
    "}\n";

/* Os raios de sol, 1: onde se vê o céu perto do sol (a profundidade diz o que é céu), pelo brilho dele: uma
 * nuvem na frente do sol corta os raios, e eles saem pelas frestas entre elas */
static const char *PIXELS_CEU_PERTO_DO_SOL =
    "#version 330\n"
    "in vec2 fragTexCoord; uniform sampler2D texture0; uniform sampler2D cena; uniform vec2 planos; uniform vec2 sol; uniform float aspecto;\n"
    "uniform vec3 raioFrente, raioDireita, raioCima;\n"
    "out vec4 finalColor;\n"
    "void main() {\n"
    "    float d = texture(texture0, fragTexCoord).r;\n"
    "    float z = planos.x * planos.y / (planos.y - d * (planos.y - planos.x));\n"
    "    vec3 raio = raioFrente + (fragTexCoord.x * 2.0 - 1.0) * raioDireita + (fragTexCoord.y * 2.0 - 1.0) * raioCima;\n"
    "    float ceu = step(" CEU_A_PARTIR_DE ", z * length(raio));\n"
    "    float perto = max(1.0 - length((fragTexCoord - sol) * vec2(aspecto, 1.0)) / 0.42, 0.0);\n"
    "    float claro = dot(texture(cena, fragTexCoord).rgb, vec3(0.2126, 0.7152, 0.0722));\n"
    "    finalColor = vec4(vec3(ceu * perto * perto * mix(0.30, 1.0, smoothstep(0.55, 1.10, claro))), 1.0);\n"
    "}\n";

/* Os raios de sol, 2: cada pixel junta o que está entre ele e o sol, cada vez mais fraco (feito duas vezes) */
static const char *PIXELS_RAIOS =
    "#version 330\n"
    "in vec2 fragTexCoord; uniform sampler2D texture0; uniform vec2 sol; uniform float alcance;\n"
    "out vec4 finalColor;\n"
    "void main() {\n"
    "    vec2 uv = fragTexCoord;\n"
    "    vec2 passo = (uv - sol) * alcance / 32.0;\n"
    "    uv -= passo * fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.5453);\n"   /* sem degraus */
    "    float soma = 0.0, peso = 1.0;\n"
    "    for (int i = 0; i < 32; i++) {\n"
    "        soma += texture(texture0, uv).r * peso;\n"
    "        peso *= 0.955;\n"
    "        uv -= passo;\n"
    "    }\n"
    "    finalColor = vec4(vec3(soma / 17.0), 1.0);\n"
    "}\n";

static const char *PIXELS_FINAL =
    "#version 330\n"
    "in vec2 fragTexCoord; uniform sampler2D texture0; uniform sampler2D halo; uniform sampler2D profundidade; uniform sampler2D raios;\n"
    "uniform vec3 tom; uniform float contraste, saturacao, forcaDoHalo, vinheta, desfoque;\n"
    "uniform vec2 pixel; uniform float tempo, granulado, distorcao, nitidez, suavizacao, oclusao;\n"
    "uniform vec2 planos;\n"                                   /* perto e longe da câmera (0: sem profundidade) */
    "uniform vec3 olho, raioFrente, raioDireita, raioCima;\n"  /* a câmera: o raio de cada pixel sai daí */
    "uniform vec4 neblina; uniform vec3 corDaNeblina;\n"        /* densidade, base, 1 / altura, máximo */
    "uniform vec2 foco;\n"                                     /* metros a partir dos quais desfoca, borrão máximo (pixels) */
    "uniform vec3 solNaTela; uniform vec3 corDosRaios; uniform vec3 corDoReflexo;\n"
    "uniform float exposicao;\n"                               /* 0: sem a curva de cinema (cena de 8 bits) */
    "out vec4 finalColor;\n"
    "vec3 amostra(vec2 p) { return textureLod(texture0, p, 0.0).rgb; }\n"
    "float brilho(vec3 c) { return dot(c, vec3(0.299, 0.587, 0.114)); }\n"
    "float metros(vec2 p) { float d = textureLod(profundidade, p, 0.0).r;\n"
    "                       return planos.x * planos.y / (planos.y - d * (planos.y - planos.x)); }\n"
    /* o raio da câmera por um ponto da imagem (a parte "para a frente" dele vale 1) */
    "vec3 raioPor(vec2 p) { return raioFrente + (p.x * 2.0 - 1.0) * raioDireita + (p.y * 2.0 - 1.0) * raioCima; }\n"
    /* o céu: o domo fica a 9 km em toda direção (pela profundidade só, os cantos da tela ficariam aquém) */
    "float ehCeu(vec2 p, float z) { return step(" CEU_A_PARTIR_DE ", z * length(raioPor(p))); }\n"
    /* o tamanho do borrão de um ponto a z metros, em pixels: só o fundo bem longe */
    "float circulo(float z) { return foco.y * smoothstep(foco.x, foco.x * 3.0, z); }\n"
    /* a curva de cinema: igual até o joelho; dali, o que passa do branco chega nele aos poucos */
    "vec3 curva(vec3 c) {\n"
    "    const float J = 0.72, B = 1.18;\n"
    "    return min(c, J) + (B - J) * (1.0 - exp(-max(c - J, 0.0) / (B - J)));\n"
    "}\n"
    /* um reflexo da lente: um disco macio */
    "vec3 fantasma(vec2 uv, vec2 meio, float raio, vec3 cor, vec2 aspecto) {\n"
    "    float d = length((uv - meio) * aspecto);\n"
    "    return cor * (smoothstep(raio, raio * 0.55, d) + 0.5 * smoothstep(raio * 0.12, 0.0, abs(d - raio * 0.9)));\n"
    "}\n"
    "void main() {\n"
    "    vec2 uv = fragTexCoord;\n"
    "    vec2 cc = uv - 0.5;\n"
    "    float r2 = dot(cc, cc);\n"
    "    if (distorcao > 0.5) uv = 0.5 + cc * (1.0 + 0.16 * r2) / 1.08;\n"   /* a distorção da lente: o meio um pouco maior, as bordas curvas */
    "    vec3 c;\n"
    "    if (desfoque > 0.001) {\n"   /* desfoque de velocidade: só nas bordas, puxando para o centro */
    "        vec2 d = uv - vec2(0.5, 0.48);\n"
    "        float bordas = smoothstep(0.18, 0.75, length(d));\n"
    "        c = vec3(0.0);\n"
    "        for (int i = 0; i < 8; i++) c += amostra(uv - d * desfoque * bordas * float(i) * 0.016);\n"
    "        c /= 8.0;\n"
    "    } else if (distorcao > 1.5) {\n"   /* a aberração cromática: o vermelho e o azul se separam, mais nas bordas */
    "        vec2 o = cc * (0.004 + 0.022 * r2);\n"
    "        c = vec3(amostra(uv + o).r, amostra(uv).g, amostra(uv - o).b);\n"
    "    } else {\n"
    "        c = amostra(uv);\n"
    "    }\n"
    "    if (suavizacao > 0.5 || nitidez > 0.0) {\n"
    "        vec3 n = amostra(uv + vec2(0.0, -pixel.y)), s = amostra(uv + vec2(0.0, pixel.y));\n"
    "        vec3 e = amostra(uv + vec2(pixel.x, 0.0)), w = amostra(uv + vec2(-pixel.x, 0.0));\n"
    "        if (suavizacao > 0.5) {\n"   /* FXAA simples: onde há uma borda forte, mistura com os vizinhos */
    "            float lc = brilho(c), ln = brilho(n), ls = brilho(s), le = brilho(e), lw = brilho(w);\n"
    "            float contr = max(max(max(ln, ls), max(le, lw)), lc) - min(min(min(ln, ls), min(le, lw)), lc);\n"
    "            if (contr > 0.05) c = mix(c, (n + s + e + w + 2.0 * c) / 6.0, clamp(contr * 3.0, 0.0, 0.7));\n"
    "        }\n"
    "        if (nitidez > 0.0) c += (c - (n + s + e + w) * 0.25) * nitidez;\n"   /* afiar a imagem, como o FidelityFX CAS */
    "    }\n"
    "    float z = (planos.y > 0.0) ? metros(uv) : 1.0e9;\n"
    /* O desfoque do fundo longe: os vizinhos num disco (a espiral do ângulo de ouro) entram pelo tamanho do
     * borrão DELES; o fundo não vaza por cima do que está na frente. O que está nítido nem procura */
    "    if (foco.y > 0.0 && planos.y > 0.0) {\n"
    "        float r = circulo(z);\n"
    "        if (r > 0.5) {\n"
    "            vec3 soma = c; float total = 1.0, raio = 1.0, angulo = 0.0;\n"
    "            for (int i = 0; i < 48 && raio < foco.y; i++) {\n"
    "                vec2 p = uv + vec2(cos(angulo), sin(angulo)) * pixel * raio;\n"
    "                float zp = metros(p);\n"
    "                float rp = circulo(zp);\n"
    "                if (zp > z) rp = min(rp, r * 2.0);\n"
    "                soma += mix(soma / total, amostra(p), smoothstep(raio - 0.5, raio + 0.5, rp));\n"
    "                total += 1.0;\n"
    "                raio += 1.0 / raio;\n"
    "                angulo += 2.39996323;\n"
    "            }\n"
    "            c = mix(c, soma / total, smoothstep(0.5, 1.5, r));\n"
    "        }\n"
    "    }\n"
    "    if (oclusao > 0.5) {\n"
    "        vec2 r = pixel * 6.0;\n"
    "        vec3 m = (amostra(uv + vec2(r.x, 0.0)) + amostra(uv - vec2(r.x, 0.0)) + amostra(uv + vec2(0.0, r.y)) + amostra(uv - vec2(0.0, r.y))) * 0.25;\n"
    "        c *= 1.0 + clamp(brilho(c) - brilho(m), -0.25, 0.0) * 1.6;\n"
    "    }\n"
    /* A neblina: a densidade cai com a altura, então o raio que sobe atravessa menos (a conta fechada da
     * integral). No céu, só a faixa do horizonte se junta a ela (senão a linha do horizonte apareceria) */
    "    if (neblina.x > 0.0 && planos.y > 0.0) {\n"
    "        vec3 raio = raioPor(uv);\n"
    "        float ceu = ehCeu(uv, z);\n"
    "        float distancia = min(z * length(raio), " CEU_A_PARTIR_DE ");\n"
    "        vec3 d = normalize(raio);\n"
    "        float subida = d.y * neblina.z * distancia;\n"
    "        float quanto = neblina.x * exp(-(olho.y - neblina.y) * neblina.z) * distancia\n"
    "                       * ((abs(subida) > 0.001) ? (1.0 - exp(-subida)) / subida : 1.0);\n"
    "        float f = min(1.0 - exp(-quanto), neblina.w);\n"
    "        if (ceu > 0.5) f *= 1.0 - smoothstep(0.0, 0.10, d.y);\n"
    "        c = mix(c, corDaNeblina, f);\n"
    "    }\n"
    "    if (dot(corDosRaios, vec3(1.0)) > 0.0) c += texture(raios, uv).rgb * corDosRaios;\n"
    "    c += texture(halo, uv).rgb * forcaDoHalo;\n"
    /* O reflexo do sol na lente: só com ele à vista (a profundidade em volta dele diz se é céu). Os
     * fantasmas ficam na linha do sol ao meio da tela, e a faixa deitada atravessa o sol */
    "    if (solNaTela.z > 0.5 && dot(corDoReflexo, vec3(1.0)) > 0.0 && planos.y > 0.0) {\n"
    "        vec2 s = solNaTela.xy;\n"
    "        float ve = 0.0;\n"
    "        for (int k = 0; k < 5; k++) {\n"
    "            vec2 q = s + vec2(float(k == 1) - float(k == 2), float(k == 3) - float(k == 4)) * pixel * 8.0;\n"
    "            float dentro = step(0.0, q.x) * step(q.x, 1.0) * step(0.0, q.y) * step(q.y, 1.0);\n"
    "            q = clamp(q, vec2(0.0), vec2(1.0));\n"
    "            ve += dentro * ehCeu(q, metros(q));\n"
    "        }\n"
    "        ve *= 0.2;\n"
    "        if (ve > 0.0) {\n"
    "            vec2 aspecto = vec2(pixel.y / pixel.x, 1.0);\n"
    "            vec2 eixo = vec2(0.5) - s;\n"
    "            vec3 f = fantasma(uv, s + eixo * 0.45, 0.022, vec3(1.0, 0.72, 0.40), aspecto) * 0.10\n"
    "                   + fantasma(uv, s + eixo * 0.80, 0.060, vec3(0.45, 0.85, 0.75), aspecto) * 0.05\n"
    "                   + fantasma(uv, s + eixo * 1.25, 0.035, vec3(0.70, 0.55, 1.00), aspecto) * 0.07\n"
    "                   + fantasma(uv, s + eixo * 1.65, 0.110, vec3(1.00, 0.60, 0.35), aspecto) * 0.035\n"
    "                   + fantasma(uv, s + eixo * 2.10, 0.050, vec3(0.50, 0.75, 1.00), aspecto) * 0.06;\n"
    "            vec2 ds = (uv - s) * aspecto;\n"
    "            f += vec3(0.60, 0.75, 1.00) * exp(-abs(ds.y) * 240.0) * exp(-abs(ds.x) * 2.6) * 0.22;\n"   /* a faixa deitada */
    "            f += vec3(1.00, 0.86, 0.66) * exp(-length(ds) * 8.0) * 0.14;\n"                                /* o clarão em volta */
    "            c += f * corDoReflexo * ve;\n"
    "        }\n"
    "    }\n"
    "    if (exposicao > 0.0) c = curva(c * exposicao);\n"
    "    c *= tom;\n"
    "    float l = dot(c, vec3(0.2126, 0.7152, 0.0722));\n"
    "    c = mix(vec3(l), c, saturacao);\n"
    "    c = (c - 0.5) * contraste + 0.5;\n"
    "    c *= 1.0 - vinheta * smoothstep(0.42, 0.95, length((uv - 0.5) * vec2(1.25, 1.0)));\n"
    "    if (granulado > 0.0) {\n"   /* granulado de filme: ruído que muda a cada quadro */
    "        float g = fract(sin(dot(fragTexCoord * vec2(1731.0, 1371.0) + tempo * vec2(37.0, 17.0), vec2(12.9898, 78.233))) * 43758.5453);\n"
    "        c += (g - 0.5) * granulado;\n"
    "    }\n"
    "    finalColor = vec4(clamp(c, 0.0, 1.0), 1.0);\n"
    "}\n";

static RenderTexture2D cena, haloA, haloB, raiosA, raiosB;
static RenderTexture2D luzesCheias;   /* do tamanho da cena, com a profundidade dela (não é dona da profundidade) */
static Shader shaderClaros, shaderBorrar, shaderFinal, shaderReduzir, shaderCeuPertoDoSol, shaderRaios;
static int locLimiar, locForcaDosClaros, locPassoDosClaros, locPasso, locHalo, locTom, locContraste, locSaturacao, locForca, locVinheta, locDesfoque;
static int locPassoDaReducao;
static int locPixel, locTempo, locGranulado, locDistorcao, locNitidez, locSuavizacao, locOclusao;
static int locProfundidade, locRaios, locPlanos, locOlho, locRaioFrente, locRaioDireita, locRaioCima, locNeblina, locCorDaNeblina;
static int locFoco, locSolNaTela, locCorDosRaios, locCorDoReflexo, locExposicao;
static int locPlanosDoCeu, locSolDoCeu, locAspectoDoCeu, locSolDosRaios, locAlcanceDosRaios;
static int locFrenteDoCeu, locDireitaDoCeu, locCimaDoCeu, locCenaDoCeu;
static int largura = 0, altura = 0, carregado = 0;
static int hdrDasImagens = -1;          /* com que escolha de cores as imagens foram criadas */
static int cenaEmMeioFloat = 0;         /* 1: a cor da cena guarda o que passa do branco */
static int profundidadeEmTextura = 0;   /* 1: a profundidade da cena dá para ler (neblina, foco, sol) */
static int desenhando = 0;   /* 1 a cena, 2 as luzes (no halo), 3 as luzes (na imagem cheia, com profundidade) */
static CinemaDoQuadro cinemaPedido;
static int temCinema = 0;
/* Onde a cena vai aparecer na tela. Largura 0 = a tela cheia (o normal). Na tela dividida as duas
 * metades têm o MESMO tamanho, então as imagens são criadas uma vez e não mudam mais de quadro
 * para quadro: o que muda é só o canto em que elas são desenhadas. */
static int areaX = 0, areaY = 0, areaLargura = 0, areaAltura = 0;

/*
 * larguraDaArea / alturaDaArea
 * Recebe: nada.
 * Devolve: o tamanho em que a cena é desenhada (a área escolhida, ou a janela toda).
 */
static int larguraDaArea(void) {
    return (areaLargura > 0) ? areaLargura : GetScreenWidth();
}
static int alturaDaArea(void) {
    return (areaAltura > 0) ? areaAltura : GetScreenHeight();
}

/*
 * definirAreaDoPosProcesso
 * Recebe: o pedaço da tela em que a cena vai aparecer (largura 0 = a tela cheia).
 * Devolve: nada.
 */
void definirAreaDoPosProcesso(int x, int y, int largura, int altura) {
    areaX = x;
    areaY = y;
    areaLargura = largura;
    areaAltura = altura;
}

/*
 * larguraDaCena / alturaDaCena
 * Recebe: nada.
 * Devolve: o tamanho da imagem em que o mundo é desenhado: o da área vezes a escala de renderização das
 *          configurações (a 100% é o da área; abaixo disso o mundo é desenhado menor e ampliado, com mais FPS).
 */
static int larguraDaCena(void) {
    int w = larguraDaArea() * escalaDaImagemEmPorcento(configuracoesAtuais()->escalaDaImagem) / 100;
    return (w < 16) ? 16 : w;
}
static int alturaDaCena(void) {
    int h = alturaDaArea() * escalaDaImagemEmPorcento(configuracoesAtuais()->escalaDaImagem) / 100;
    return (h < 16) ? 16 : h;
}

/* As cores HDR valem com elas ligadas e com os efeitos (sem os efeitos, a cena vai crua para a tela) */
static int querHdr(void) {
    const Configuracoes *config = configuracoesAtuais();
    return config->coresHdr && config->efeitos;
}

/*
 * criarImagem
 * Uma imagem para desenhar: a cor no formato pedido e, se pedida, a profundidade (numa textura, que os
 * shaders leem, ou num renderbuffer, que só serve para o teste de profundidade).
 * Recebe: o tamanho, o formato da cor e a profundidade (0 nenhuma, 1 renderbuffer, 2 textura).
 * Devolve: a imagem (id 0 se a placa não aceitou a combinação).
 */
static RenderTexture2D criarImagem(int w, int h, int formato, int profundidade) {
    RenderTexture2D r = { 0 };
    r.id = rlLoadFramebuffer();
    if (r.id == 0) {
        return r;
    }
    r.texture.id = rlLoadTexture(NULL, w, h, formato, 1);
    r.texture.width = w;
    r.texture.height = h;
    r.texture.mipmaps = 1;
    r.texture.format = formato;
    rlFramebufferAttach(r.id, r.texture.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0);
    if (profundidade > 0) {
        r.depth.id = rlLoadTextureDepth(w, h, profundidade == 1);
        r.depth.width = w;
        r.depth.height = h;
        r.depth.mipmaps = 1;
        r.depth.format = 19;   /* como a raylib marca a profundidade */
        rlFramebufferAttach(r.id, r.depth.id, RL_ATTACHMENT_DEPTH,
                            (profundidade == 1) ? RL_ATTACHMENT_RENDERBUFFER : RL_ATTACHMENT_TEXTURE2D, 0);
    }
    if (r.texture.id == 0 || !rlFramebufferComplete(r.id)) {
        UnloadRenderTexture(r);
        return (RenderTexture2D){ 0 };
    }
    SetTextureFilter(r.texture, TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(r.texture, TEXTURE_WRAP_CLAMP);
    return r;
}

/* A imagem das luzes do tamanho da cena: a cor dela, e a profundidade emprestada da cena */
static void criarLuzesCheias(void) {
    luzesCheias.id = rlLoadFramebuffer();
    luzesCheias.texture.id = rlLoadTexture(NULL, largura, altura, cena.texture.format, 1);
    luzesCheias.texture.width = largura;
    luzesCheias.texture.height = altura;
    luzesCheias.texture.mipmaps = 1;
    luzesCheias.texture.format = cena.texture.format;
    luzesCheias.depth = cena.depth;
    rlFramebufferAttach(luzesCheias.id, luzesCheias.texture.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0);
    rlFramebufferAttach(luzesCheias.id, cena.depth.id, RL_ATTACHMENT_DEPTH,
                        profundidadeEmTextura ? RL_ATTACHMENT_TEXTURE2D : RL_ATTACHMENT_RENDERBUFFER, 0);
    if (!rlFramebufferComplete(luzesCheias.id)) {
        TraceLog(LOG_WARNING, "POSPROCESSO: a imagem das luzes com profundidade não ficou completa");
    }
    SetTextureFilter(luzesCheias.texture, TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(luzesCheias.texture, TEXTURE_WRAP_CLAMP);
}

static void descarregarLuzesCheias(void) {
    /* a profundidade é da cena: solta antes, senão rlUnloadFramebuffer a apagaria junto */
    rlFramebufferAttach(luzesCheias.id, 0, RL_ATTACHMENT_DEPTH,
                        profundidadeEmTextura ? RL_ATTACHMENT_TEXTURE2D : RL_ATTACHMENT_RENDERBUFFER, 0);
    rlUnloadFramebuffer(luzesCheias.id);
    rlUnloadTexture(luzesCheias.texture.id);
}

static void descarregarImagens(void) {
    if (largura == 0) {
        return;
    }
    descarregarLuzesCheias();
    UnloadRenderTexture(cena);
    UnloadRenderTexture(haloA);
    UnloadRenderTexture(haloB);
    UnloadRenderTexture(raiosA);
    UnloadRenderTexture(raiosB);
    largura = altura = 0;
}

/*
 * criarImagens
 * As imagens do tamanho da área de agora (refeitas se ela mudar, ou se as cores HDR forem ligadas ou
 * desligadas). A cena tenta, nesta ordem: meio float com a profundidade numa textura, 8 bits com a
 * profundidade numa textura, e o de sempre (8 bits, profundidade num renderbuffer: sem neblina, foco nem sol).
 */
static void criarImagens(void) {
    int hdr = querHdr();
    if (largura == larguraDaCena() && altura == alturaDaCena() && hdr == hdrDasImagens) {
        return;
    }
    descarregarImagens();
    largura = larguraDaCena();
    altura = alturaDaCena();
    hdrDasImagens = hdr;
    cena = (RenderTexture2D){ 0 };
    if (hdr) {
        cena = criarImagem(largura, altura, PIXELFORMAT_UNCOMPRESSED_R16G16B16A16, 2);
    }
    cenaEmMeioFloat = (cena.id != 0);
    profundidadeEmTextura = 1;
    if (cena.id == 0) {
        cena = criarImagem(largura, altura, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 2);
    }
    if (cena.id == 0) {
        TraceLog(LOG_WARNING, "POSPROCESSO: sem a profundidade em textura: sem neblina, foco nem raios de sol");
        profundidadeEmTextura = 0;
        cena = criarImagem(largura, altura, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);
    }
    if (hdr && !cenaEmMeioFloat) {
        TraceLog(LOG_WARNING, "POSPROCESSO: a placa não aceitou a cena em meio float: cores HDR desligadas");
    }
    criarLuzesCheias();
    int formatoDoHalo = cenaEmMeioFloat ? PIXELFORMAT_UNCOMPRESSED_R16G16B16A16 : PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    int w = largura / REDUCAO_DO_HALO, h = altura / REDUCAO_DO_HALO;
    w = (w < 1) ? 1 : w;
    h = (h < 1) ? 1 : h;
    haloA = criarImagem(w, h, formatoDoHalo, 1);   /* com profundidade: nas corridas as luzes são desenhadas nela */
    haloB = criarImagem(w, h, formatoDoHalo, 0);
    if (haloA.id == 0 || haloB.id == 0) {   /* o meio float não pegou aqui: 8 bits */
        UnloadRenderTexture(haloA);
        UnloadRenderTexture(haloB);
        haloA = criarImagem(w, h, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);
        haloB = criarImagem(w, h, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 0);
    }
    raiosA = criarImagem(w, h, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 0);
    raiosB = criarImagem(w, h, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 0);
}

/*
 * carregarPosProcesso
 * Recebe: nada. Devolve: nada.
 */
void carregarPosProcesso(void) {
    shaderClaros = LoadShaderFromMemory(NULL, PIXELS_CLAROS);
    shaderBorrar = LoadShaderFromMemory(NULL, PIXELS_BORRAR);
    shaderFinal = LoadShaderFromMemory(NULL, PIXELS_FINAL);
    shaderReduzir = LoadShaderFromMemory(NULL, PIXELS_REDUZIR);
    shaderCeuPertoDoSol = LoadShaderFromMemory(NULL, PIXELS_CEU_PERTO_DO_SOL);
    shaderRaios = LoadShaderFromMemory(NULL, PIXELS_RAIOS);
    locPassoDaReducao = GetShaderLocation(shaderReduzir, "passo");
    locLimiar = GetShaderLocation(shaderClaros, "limiar");
    locForcaDosClaros = GetShaderLocation(shaderClaros, "forca");
    locPassoDosClaros = GetShaderLocation(shaderClaros, "passo");
    locPasso = GetShaderLocation(shaderBorrar, "passo");
    locPlanosDoCeu = GetShaderLocation(shaderCeuPertoDoSol, "planos");
    locSolDoCeu = GetShaderLocation(shaderCeuPertoDoSol, "sol");
    locAspectoDoCeu = GetShaderLocation(shaderCeuPertoDoSol, "aspecto");
    locFrenteDoCeu = GetShaderLocation(shaderCeuPertoDoSol, "raioFrente");
    locDireitaDoCeu = GetShaderLocation(shaderCeuPertoDoSol, "raioDireita");
    locCimaDoCeu = GetShaderLocation(shaderCeuPertoDoSol, "raioCima");
    locCenaDoCeu = GetShaderLocation(shaderCeuPertoDoSol, "cena");
    locSolDosRaios = GetShaderLocation(shaderRaios, "sol");
    locAlcanceDosRaios = GetShaderLocation(shaderRaios, "alcance");
    locHalo = GetShaderLocation(shaderFinal, "halo");
    locProfundidade = GetShaderLocation(shaderFinal, "profundidade");
    locRaios = GetShaderLocation(shaderFinal, "raios");
    locTom = GetShaderLocation(shaderFinal, "tom");
    locContraste = GetShaderLocation(shaderFinal, "contraste");
    locSaturacao = GetShaderLocation(shaderFinal, "saturacao");
    locForca = GetShaderLocation(shaderFinal, "forcaDoHalo");
    locVinheta = GetShaderLocation(shaderFinal, "vinheta");
    locDesfoque = GetShaderLocation(shaderFinal, "desfoque");
    locPixel = GetShaderLocation(shaderFinal, "pixel");
    locTempo = GetShaderLocation(shaderFinal, "tempo");
    locGranulado = GetShaderLocation(shaderFinal, "granulado");
    locDistorcao = GetShaderLocation(shaderFinal, "distorcao");
    locNitidez = GetShaderLocation(shaderFinal, "nitidez");
    locSuavizacao = GetShaderLocation(shaderFinal, "suavizacao");
    locOclusao = GetShaderLocation(shaderFinal, "oclusao");
    locPlanos = GetShaderLocation(shaderFinal, "planos");
    locOlho = GetShaderLocation(shaderFinal, "olho");
    locRaioFrente = GetShaderLocation(shaderFinal, "raioFrente");
    locRaioDireita = GetShaderLocation(shaderFinal, "raioDireita");
    locRaioCima = GetShaderLocation(shaderFinal, "raioCima");
    locNeblina = GetShaderLocation(shaderFinal, "neblina");
    locCorDaNeblina = GetShaderLocation(shaderFinal, "corDaNeblina");
    locFoco = GetShaderLocation(shaderFinal, "foco");
    locSolNaTela = GetShaderLocation(shaderFinal, "solNaTela");
    locCorDosRaios = GetShaderLocation(shaderFinal, "corDosRaios");
    locCorDoReflexo = GetShaderLocation(shaderFinal, "corDoReflexo");
    locExposicao = GetShaderLocation(shaderFinal, "exposicao");
    carregado = 1;
}

/*
 * descarregarPosProcesso
 * Recebe: nada. Devolve: nada.
 */
void descarregarPosProcesso(void) {
    if (!carregado) {
        return;
    }
    descarregarImagens();
    hdrDasImagens = -1;
    UnloadShader(shaderReduzir);
    UnloadShader(shaderClaros);
    UnloadShader(shaderBorrar);
    UnloadShader(shaderFinal);
    UnloadShader(shaderCeuPertoDoSol);
    UnloadShader(shaderRaios);
    carregado = 0;
}

/*
 * comecarPosProcesso
 * O que for desenhado até terminarPosProcesso vai para a imagem da cena.
 * Recebe: nada. Devolve: nada.
 */
void comecarPosProcesso(void) {
    if (!carregado) {
        return;
    }
    criarImagens();
    rlDrawRenderBatchActive();
    BeginTextureMode(cena);
    ClearBackground(BLACK);
    desenhando = 1;
}

/*
 * comecarLuzesDoPosProcesso
 * Depois da cena: o que emite luz (refletores, flashes, chamas, telas) é desenhado de novo, numa
 * imagem pequena só dele. O halo nasce só daí: o branco que só está iluminado não brilha.
 * Recebe: nada. Devolve: nada.
 */
void comecarLuzesDoPosProcesso(void) {
    if (desenhando != 1) {
        return;
    }
    rlDrawRenderBatchActive();
    EndTextureMode();
    BeginTextureMode(haloA);
    ClearBackground(BLACK);
    desenhando = 2;
}

/*
 * comecarLuzesComProfundidade
 * Como comecarLuzesDoPosProcesso, mas as luzes vão para uma imagem do tamanho da cena que usa a
 * profundidade dela: o que está na frente de uma luz a tapa (chamar BeginMode3D depois, como na cena).
 * Recebe: nada. Devolve: nada.
 */
void comecarLuzesComProfundidade(void) {
    if (desenhando != 1) {
        return;
    }
    rlDrawRenderBatchActive();
    EndTextureMode();
    BeginTextureMode(luzesCheias);
    /* só a cor é limpa (a profundidade é a da cena): um retângulo preto, sem teste de profundidade */
    rlDisableDepthTest();
    DrawRectangle(0, 0, largura, altura, BLACK);
    rlDrawRenderBatchActive();
    desenhando = 3;
}

/*
 * neblinaDaPista
 * Recebe: a pista e o cinema do quadro (com a câmera já posta).
 * Devolve: nada (preenche a neblina: a da pista, mais densa perto do chão em que a câmera está).
 */
void neblinaDaPista(int pista, CinemaDoQuadro *cinema) {
    const NeblinaDaPista *n = &NEBLINAS[(pista >= 0 && pista < TOTAL_PISTAS) ? pista : 0];
    cinema->corDaNeblina = n->cor;
    cinema->neblina = n->densidade;
    cinema->maximoDaNeblina = n->maximo;
    cinema->baseDaNeblina = cinema->camera.position.y - 4.0f;
    cinema->alturaDaNeblina = ALTURA_DA_NEBLINA_DAS_PISTAS;
}

/*
 * definirCinemaDoQuadro
 * Recebe: o que o próximo terminarPosProcesso usa além do tom da pista (NULL: nada).
 * Devolve: nada.
 */
void definirCinemaDoQuadro(const CinemaDoQuadro *cinema) {
    temCinema = (cinema != NULL);
    if (cinema != NULL) {
        cinemaPedido = *cinema;
    }
}

/*
 * passar
 * Desenha uma imagem inteira em outra com um shader (de cabeça para baixo, como sai da RenderTexture).
 * "somando": soma ao que já está lá, em vez de cobrir.
 */
static void passarComo(Texture2D de, RenderTexture2D para, Shader shader, int somando) {
    BeginTextureMode(para);
    if (somando) {
        BeginBlendMode(BLEND_ADDITIVE);
    }
    BeginShaderMode(shader);
    DrawTexturePro(de, (Rectangle){ 0, 0, (float) de.width, (float) -de.height },
                   (Rectangle){ 0, 0, (float) para.texture.width, (float) para.texture.height }, (Vector2){ 0 }, 0.0f, WHITE);
    EndShaderMode();
    if (somando) {
        EndBlendMode();
    }
    EndTextureMode();
}

static void passar(Texture2D de, RenderTexture2D para, Shader shader) {
    passarComo(de, para, shader, 0);
}

/* Os eixos da câmera: para a frente, e para a direita e para cima já do tamanho da borda da imagem */
typedef struct {
    Vector3 frente, direita, cima;
} EixosDaCamera;

static EixosDaCamera eixosDaCamera(Camera3D camera, float aspecto) {
    EixosDaCamera e;
    e.frente = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    Vector3 direita = Vector3Normalize(Vector3CrossProduct(e.frente, camera.up));
    Vector3 cima = Vector3CrossProduct(direita, e.frente);
    float tanY = tanf(camera.fovy * 0.5f * DEG2RAD);
    e.direita = Vector3Scale(direita, tanY * aspecto);
    e.cima = Vector3Scale(cima, tanY);
    return e;
}

/*
 * solNaTela
 * Recebe: os eixos da câmera e a direção do sol.
 * Devolve: onde o sol fica na imagem (0 a 1, como as coordenadas da textura da cena) e, em z, 1 se
 *          ele está na frente da câmera (0 se atrás).
 */
static Vector3 solNaTela(EixosDaCamera e, Vector3 sol) {
    float frente = Vector3DotProduct(sol, e.frente);
    if (frente <= 0.05f) {
        return (Vector3){ 0.5f, 0.5f, 0.0f };
    }
    float x = Vector3DotProduct(sol, e.direita) / Vector3LengthSqr(e.direita) / frente;
    float y = Vector3DotProduct(sol, e.cima) / Vector3LengthSqr(e.cima) / frente;
    return (Vector3){ 0.5f + 0.5f * x, 0.5f + 0.5f * y, 1.0f };
}

/*
 * terminarPosProcesso
 * Faz o halo e põe a cena na tela (na área escolhida), com o tom da pista.
 * Recebe: a pista (o tom), o desfoque de velocidade (0 = nenhum, 1 = o máximo) e o acabamento:
 *         só copiar, sem o halo (a tela dividida) ou completo.
 * Devolve: nada.
 */
void terminarPosProcesso(int pista, float desfoque, AcabamentoDoPosProcesso acabamento) {
    if (!desenhando) {
        return;
    }
    int comLuzes = (desenhando >= 2);
    int luzesNaImagemCheia = (desenhando == 3);
    desenhando = 0;
    CinemaDoQuadro cinema = cinemaPedido;   /* o cinema vale só para este quadro */
    int comCinema = temCinema && profundidadeEmTextura;
    temCinema = 0;
    rlDrawRenderBatchActive();
    EndTextureMode();
    /* De onde sai e para onde vai: a imagem inteira, no pedaço de tela da área */
    Rectangle daImagem = { 0, 0, (float) largura, (float) -altura };
    Rectangle naTela = { (float) areaX, (float) areaY, (float) larguraDaArea(), (float) alturaDaArea() };
    const Configuracoes *config = configuracoesAtuais();
    int focoAoLonge = comCinema && config->profundidadeDeCampo && acabamento != POSPROCESSO_SO_COPIA;
    float nitidez = (config->nitidez == 1) ? 0.45f : ((config->nitidez >= 2) ? 0.9f : 0.0f);
    int comExtras = config->suavizacao || nitidez > 0.0f || config->granulado || focoAoLonge || config->distorcaoDaLente || config->oclusaoAmbiental;
    if (acabamento == POSPROCESSO_SO_COPIA && !comExtras) {
        /* Sem os efeitos: só copia a cena para a tela (desenhar direto na janela é mais lento) */
        DrawTexturePro(cena.texture, daImagem, naTela, (Vector2){ 0 }, 0.0f, WHITE);
        return;
    }
    if (luzesNaImagemCheia && acabamento == POSPROCESSO_COMPLETO) {
        Vector2 passo = { 1.0f / largura, 1.0f / altura };
        SetShaderValue(shaderReduzir, locPassoDaReducao, &passo, SHADER_UNIFORM_VEC2);
        passar(luzesCheias.texture, haloA, shaderReduzir);
    }
    /* Com os efeitos da cor desligados mas com algum acabamento (suavização, nitidez...), o shader roda com a cor neutra */
    static const TomDaPista NEUTRO = { { 1.0f, 1.0f, 1.0f }, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f };
    TomDaPista t = (acabamento == POSPROCESSO_SO_COPIA) ? NEUTRO : TONS[(pista >= 0 && pista < TOTAL_PISTAS) ? pista : 0];
    if (comCinema && cinema.temCor && acabamento != POSPROCESSO_SO_COPIA) {
        t.tom = cinema.tom;
        t.contraste = cinema.contraste;
        t.saturacao = cinema.saturacao;
        t.vinheta = cinema.vinheta;
    }
    int comHalo = (acabamento == POSPROCESSO_COMPLETO) && comLuzes && config->bloom;
    int hdr = cenaEmMeioFloat && acabamento != POSPROCESSO_SO_COPIA;

    /* O halo: as luzes e, com as cores HDR, o que passou do branco na cena; borrados duas vezes em cada direção */
    if (comHalo) {
        if (hdr) {
            float limiar = LIMIAR_DO_HDR, forca = FORCA_DO_HDR_NO_HALO;
            SetShaderValue(shaderClaros, locLimiar, &limiar, SHADER_UNIFORM_FLOAT);
            SetShaderValue(shaderClaros, locForcaDosClaros, &forca, SHADER_UNIFORM_FLOAT);
            Vector2 passoDosClaros = { 1.0f / largura, 1.0f / altura };
            SetShaderValue(shaderClaros, locPassoDosClaros, &passoDosClaros, SHADER_UNIFORM_VEC2);
            passarComo(cena.texture, haloA, shaderClaros, 1);
        }
        Vector2 horizontal = { 1.0f / haloA.texture.width, 0.0f }, vertical = { 0.0f, 1.0f / haloA.texture.height };
        for (int i = 0; i < 2; i++) {
            SetShaderValue(shaderBorrar, locPasso, &horizontal, SHADER_UNIFORM_VEC2);
            passar(haloA.texture, haloB, shaderBorrar);
            SetShaderValue(shaderBorrar, locPasso, &vertical, SHADER_UNIFORM_VEC2);
            passar(haloB.texture, haloA, shaderBorrar);
        }
    }

    /* A câmera: os eixos (o raio de cada pixel) e onde o sol cai na imagem */
    float aspecto = (float) largura / (float) altura;
    Vector2 planos = { 0.0f, 0.0f };
    EixosDaCamera eixos = { { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } };
    Vector3 sol = { 0.5f, 0.5f, 0.0f };
    if (comCinema) {
        planos = (Vector2){ (float) rlGetCullDistanceNear(), (float) rlGetCullDistanceFar() };
        eixos = eixosDaCamera(cinema.camera, aspecto);
        sol = solNaTela(eixos, Vector3Normalize(cinema.sol));
    }
    int solPerto = sol.z > 0.5f && sol.x > -0.4f && sol.x < 1.4f && sol.y > -0.4f && sol.y < 1.4f;
    Vector3 corDosRaios = { 0.0f, 0.0f, 0.0f }, corDoReflexo = { 0.0f, 0.0f, 0.0f };
    if (comCinema && acabamento == POSPROCESSO_COMPLETO && solPerto) {
        if (config->raiosDeSol && cinema.raiosDeSol > 0.0f && raiosA.id != 0 && raiosB.id != 0) {
            /* Os raios: o céu perto do sol (tapado pelo que está na frente), espalhado a partir dele duas vezes */
            Texture2D profundidade = cena.depth;
            Vector2 noCeu = { sol.x, sol.y };
            SetShaderValue(shaderCeuPertoDoSol, locPlanosDoCeu, &planos, SHADER_UNIFORM_VEC2);
            SetShaderValue(shaderCeuPertoDoSol, locSolDoCeu, &noCeu, SHADER_UNIFORM_VEC2);
            SetShaderValue(shaderCeuPertoDoSol, locAspectoDoCeu, &aspecto, SHADER_UNIFORM_FLOAT);
            SetShaderValue(shaderCeuPertoDoSol, locFrenteDoCeu, &eixos.frente, SHADER_UNIFORM_VEC3);
            SetShaderValue(shaderCeuPertoDoSol, locDireitaDoCeu, &eixos.direita, SHADER_UNIFORM_VEC3);
            SetShaderValue(shaderCeuPertoDoSol, locCimaDoCeu, &eixos.cima, SHADER_UNIFORM_VEC3);
            BeginTextureMode(raiosA);   /* como passar(), mas com a cena junto (o brilho do céu) */
            BeginShaderMode(shaderCeuPertoDoSol);
            SetShaderValueTexture(shaderCeuPertoDoSol, locCenaDoCeu, cena.texture);
            DrawTexturePro(profundidade, (Rectangle){ 0, 0, (float) profundidade.width, (float) -profundidade.height },
                           (Rectangle){ 0, 0, (float) raiosA.texture.width, (float) raiosA.texture.height }, (Vector2){ 0 }, 0.0f, WHITE);
            EndShaderMode();
            EndTextureMode();
            float alcance = ALCANCE_DOS_RAIOS;
            SetShaderValue(shaderRaios, locSolDosRaios, &noCeu, SHADER_UNIFORM_VEC2);
            SetShaderValue(shaderRaios, locAlcanceDosRaios, &alcance, SHADER_UNIFORM_FLOAT);
            passar(raiosA.texture, raiosB, shaderRaios);
            alcance = ALCANCE_DOS_RAIOS * 0.35f;
            SetShaderValue(shaderRaios, locAlcanceDosRaios, &alcance, SHADER_UNIFORM_FLOAT);
            passar(raiosB.texture, raiosA, shaderRaios);
            /* Com o sol fora da tela, os raios sairiam de uma fresta de céu na borda (uma coluna de luz):
             * eles somem conforme ele sai */
            float fora = fmaxf(fmaxf(-sol.x, sol.x - 1.0f), fmaxf(-sol.y, sol.y - 1.0f));
            float naTela = 1.0f - Clamp(fora / 0.12f, 0.0f, 1.0f);
            corDosRaios = Vector3Scale(cinema.corDoSol, cinema.raiosDeSol * naTela * naTela);
        }
        if (config->lensFlare && cinema.reflexoDoSol > 0.0f) {
            corDoReflexo = Vector3Scale(cinema.corDoSol, cinema.reflexoDoSol);
        }
    }

    /* Na tela */
    float forcaDoHalo = comHalo ? t.forcaDoHalo : 0.0f;
    SetShaderValue(shaderFinal, locTom, &t.tom, SHADER_UNIFORM_VEC3);
    SetShaderValue(shaderFinal, locContraste, &t.contraste, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shaderFinal, locSaturacao, &t.saturacao, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shaderFinal, locForca, &forcaDoHalo, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shaderFinal, locVinheta, &t.vinheta, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shaderFinal, locDesfoque, &desfoque, SHADER_UNIFORM_FLOAT);
    Vector2 pixel = { 1.0f / (float) largura, 1.0f / (float) altura };
    float tempo = (float) fmod(GetTime(), 100.0);
    float granulado = config->granulado ? 0.07f : 0.0f;
    float distorcao = (float) config->distorcaoDaLente, suavizacao = (float) config->suavizacao;
    SetShaderValue(shaderFinal, locPixel, &pixel, SHADER_UNIFORM_VEC2);
    SetShaderValue(shaderFinal, locTempo, &tempo, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shaderFinal, locGranulado, &granulado, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shaderFinal, locDistorcao, &distorcao, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shaderFinal, locNitidez, &nitidez, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shaderFinal, locSuavizacao, &suavizacao, SHADER_UNIFORM_FLOAT);
    float oclusao = (float) config->oclusaoAmbiental;
    SetShaderValue(shaderFinal, locOclusao, &oclusao, SHADER_UNIFORM_FLOAT);
    /* A profundidade e a câmera */
    SetShaderValue(shaderFinal, locPlanos, &planos, SHADER_UNIFORM_VEC2);
    SetShaderValue(shaderFinal, locOlho, &cinema.camera.position, SHADER_UNIFORM_VEC3);
    SetShaderValue(shaderFinal, locRaioFrente, &eixos.frente, SHADER_UNIFORM_VEC3);
    SetShaderValue(shaderFinal, locRaioDireita, &eixos.direita, SHADER_UNIFORM_VEC3);
    SetShaderValue(shaderFinal, locRaioCima, &eixos.cima, SHADER_UNIFORM_VEC3);
    int comNeblina = comCinema && acabamento != POSPROCESSO_SO_COPIA && cinema.neblina > 0.0f;
    float neblina[4] = { comNeblina ? cinema.neblina : 0.0f, cinema.baseDaNeblina,
                         1.0f / fmaxf(cinema.alturaDaNeblina, 1.0f), cinema.maximoDaNeblina };
    SetShaderValue(shaderFinal, locNeblina, neblina, SHADER_UNIFORM_VEC4);
    SetShaderValue(shaderFinal, locCorDaNeblina, &cinema.corDaNeblina, SHADER_UNIFORM_VEC3);
    float foco[2] = { FOCO_AO_LONGE, focoAoLonge ? BORRAO_AO_LONGE * (float) altura : 0.0f };
    SetShaderValue(shaderFinal, locFoco, foco, SHADER_UNIFORM_VEC2);
    SetShaderValue(shaderFinal, locSolNaTela, &sol, SHADER_UNIFORM_VEC3);
    SetShaderValue(shaderFinal, locCorDosRaios, &corDosRaios, SHADER_UNIFORM_VEC3);
    SetShaderValue(shaderFinal, locCorDoReflexo, &corDoReflexo, SHADER_UNIFORM_VEC3);
    float exposicao = hdr ? EXPOSICAO : 0.0f;
    SetShaderValue(shaderFinal, locExposicao, &exposicao, SHADER_UNIFORM_FLOAT);
    BeginShaderMode(shaderFinal);
    SetShaderValueTexture(shaderFinal, locHalo, haloA.texture);   /* sem halo, a força é 0: a imagem não pesa */
    /* Sem profundidade em textura, as amostras dela nunca são lidas (planos = 0): qualquer imagem serve */
    SetShaderValueTexture(shaderFinal, locProfundidade, profundidadeEmTextura ? cena.depth : haloA.texture);
    SetShaderValueTexture(shaderFinal, locRaios, (raiosA.id != 0) ? raiosA.texture : haloA.texture);
    DrawTexturePro(cena.texture, daImagem, naTela, (Vector2){ 0 }, 0.0f, WHITE);
    EndShaderMode();
}
