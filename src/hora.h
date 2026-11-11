/*
 * hora.h
 * ------------------------------------------------------------------
 * A hora do dia no passeio: um dia inteiro de Radiator Springs dura 48
 * minutos (28 de dia, 6 de pôr do sol, 10 de noite e 4 de amanhecer), e a
 * luz acompanha: o sol anda pelo céu (nasce às 6h30, sobe até as 13h e se
 * põe às 19h30), fica laranja no fim da tarde, e de noite a cidade fica
 * azulada, com a lua e as estrelas. O relógio só anda no passeio.
 *
 * A luz entra nos shaders do mundo como uniformes (multiplicadores da luz do
 * meio-dia, que é a de sempre): os do mapa, os dos carros e os dos
 * moradores. Fora do passeio (as corridas, os menus) vale sempre a do
 * meio-dia.
 * ------------------------------------------------------------------
 */

#ifndef HORA_H
#define HORA_H

#include "raylib.h"

/* Os pedaços de GLSL da luz do dia. "solDoDia" é para onde fica a luz (o sol, ou a lua de noite);
 * "luzDeBase", "luzDoCeu" e "luzDoSol" multiplicam as três partes da luz de cada shader (1 ao meio-dia);
 * "forcaDaSombra" multiplica a sombra do sol (0 de noite). Os carros refletem o céu entre
 * "ceuDeBaixo" e "ceuDeCima" */
#define GLSL_DO_SOL           "uniform vec3 solDoDia;\n"
#define GLSL_DA_LUZ_DO_DIA    "uniform vec3 luzDeBase; uniform vec3 luzDoCeu; uniform vec3 luzDoSol; uniform float forcaDaSombra;\n"
#define GLSL_DO_REFLEXO_DO_DIA "uniform vec3 ceuDeBaixo; uniform vec3 ceuDeCima;\n"

/* As lâmpadas da noite: as luzes pontuais que mais aparecem para a câmera (os neons, as vitrines, as
 * janelas: lampadas = lugar e raio; coresDasLampadas = cor e, no alfa, a força + 8 × (quadro + 1)).
 * luzDasLampadas devolve a luz que elas somam num ponto, só onde a lâmpada enxerga: o quadro dela em
 * vistaDasLampadas (luzes.c) diz até onde ela chega em cada direção, e o que está atrás de uma parede
 * fica no escuro. Quadro -1: sem a vista, ilumina tudo */
#define MAIS_LAMPADAS         64
#define SLOT_DA_VISTA_DAS_LAMPADAS 14   /* a unidade de textura da imagem das vistas (a sombra do sol é a 15) */
#define GLSL_DAS_LAMPADAS \
    "uniform int totalDeLampadas; uniform vec4 lampadas[64]; uniform vec4 coresDasLampadas[64];\n" \
    "uniform sampler2D vistaDasLampadas;\n" \
    "float lampadaVe(float quadro, vec3 deLa, float distancia) {\n" \
    "    if (quadro < 0.0) return 1.0;\n" \
    "    vec3 u = deLa / max(distancia, 0.0001);\n" \
    "    vec2 dentro = vec2(atan(u.z, u.x) * 0.1591549 + 0.5, acos(clamp(u.y, -1.0, 1.0)) * 0.3183099) * vec2(64.0, 32.0);\n" \
    "    dentro = clamp(dentro, vec2(0.5), vec2(63.5, 31.5));\n" \
    "    vec2 canto = vec2(mod(quadro, 16.0), floor(quadro / 16.0)) * vec2(64.0, 32.0);\n" \
    "    float chega = texture(vistaDasLampadas, (canto + dentro) / vec2(textureSize(vistaDasLampadas, 0))).r;\n" \
    "    return 1.0 - smoothstep(chega + 0.25, chega + 0.8 + 0.05 * chega, distancia);\n" \
    "}\n" \
    "vec3 luzDasLampadas(vec3 p, vec3 n) {\n" \
    "    vec3 soma = vec3(0.0);\n" \
    "    for (int i = 0; i < totalDeLampadas; i++) {\n" \
    "        vec3 d = lampadas[i].xyz - p;\n" \
    "        float q = dot(d, d) / (lampadas[i].w * lampadas[i].w);\n" \
    "        if (q >= 1.0) continue;\n" \
    "        float alfa = coresDasLampadas[i].a;\n" \
    "        float quadro = floor(alfa / 8.0);\n" \
    "        float forca = alfa - quadro * 8.0;\n" \
    "        float distancia = length(d);\n" \
    "        float ve = lampadaVe(quadro - 1.0, -d, distancia);\n" \
    "        if (ve <= 0.0) continue;\n" \
    "        float virado = max(dot(n, d / max(distancia, 0.0001)), 0.0) * 0.75 + 0.25;\n" \
    "        soma += coresDasLampadas[i].rgb * forca * (1.0 - q) * (1.0 - q) * virado * ve;\n" \
    "    }\n" \
    "    return soma;\n" \
    "}\n"

typedef struct {
    Vector3 lugar;
    Vector3 cor;            /* 0 a 1 */
    float raio;             /* metros: até onde ela chega */
    float forca;            /* 0 a 7 */
    int quadro;             /* o quadro dela na imagem das vistas (-1: sem vista) */
} Lampada;

typedef struct {
    Vector3 sol;            /* de onde vem a luz principal (o sol ou a lua), normalizado */
    Vector3 base, ceu, luzDoSol;   /* multiplicadores das partes da luz */
    float sombra;           /* força da sombra do sol (0 a 1) */
    Vector3 ceuDeBaixo, ceuDeCima; /* o céu refletido nos carros */
    /* O céu do passeio */
    Vector3 solNoCeu;       /* onde o sol está de verdade (pode estar abaixo do horizonte) */
    Vector3 luaNoCeu;
    Vector3 horizonte, alto, nuvens, brilho, corDoSol;
    float misturaDoDia;     /* 1: a imagem do céu de dia como ela é; 0: o céu recolorido */
    float estrelas, lua;
    Vector3 interior;       /* a cor que multiplica os interiores (a luz deles é assada de dia) */
    float luzesDaCidade;    /* 0 apagadas, 1 acesas (os neons, as lâmpadas) */
} LuzDoDia;

void iniciarRelogio(void);                 /* a primeira vez: 10h da manhã; depois, continua de onde parou */
void avancarRelogio(float segundos);
float horaDoDia(void);                     /* 0 a 24 */
void definirHoraDoDia(float hora);
void sortearHoraDoDia(void);               /* uma hora qualquer do dia de 48 minutos (cada parte na proporção do tempo dela) */
void escreverHora(char *texto, int tamanho, float hora);   /* "19:42" */

const LuzDoDia *luzDaHora(void);           /* a luz da hora de agora */
const LuzDoDia *luzDoMeioDia(void);        /* a de sempre (corridas, menus) */
const LuzDoDia *luzAplicada(void);         /* a última mandada aos shaders */

/* Os shaders que recebem a luz do dia. Ao serem registrados, ganham a do meio-dia */
void registrarShaderDoDia(Shader shader);
void aplicarLuzDoDia(const LuzDoDia *luz);   /* NULL: a do meio-dia */
/* As lâmpadas (até MAIS_LAMPADAS). Fora do passeio: nenhuma */
void aplicarLampadas(const Lampada *lista, int total);

#endif
