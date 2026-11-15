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
 * "forcaDaSombra" multiplica a sombra do sol (0 de noite). Os carros refletem o chão ("ceuDeBaixo"), a
 * faixa clara do horizonte ("ceuDoHorizonte") e o céu ("ceuDeCima") */
#define GLSL_DO_SOL           "uniform vec3 solDoDia;\n"
#define GLSL_DA_LUZ_DO_DIA    "uniform vec3 luzDeBase; uniform vec3 luzDoCeu; uniform vec3 luzDoSol; uniform float forcaDaSombra;\n"
#define GLSL_DO_REFLEXO_DO_DIA "uniform vec3 ceuDeBaixo; uniform vec3 ceuDeCima; uniform vec3 ceuDoHorizonte;\n"

/* A sombra do sol, para quem a recebe (o cenário, o Mack, os carros e os moradores). O mapa de sombra
 * tem duas metades lado a lado: na da esquerda, um quadrado pequeno em volta da câmera, com o detalhe
 * fino (luzVPPerto); na da direita, o quadrado grande (luzVP). Cada comparação lê os 16 pixels em volta
 * do ponto, com pesos bilineares (é o mesmo que 3 x 3 comparações filtradas): a borda da sombra sai uma
 * linha, não uma escada de pixels. Perto da borda do quadrado pequeno, as duas metades se misturam.
 * sombraNoMapa devolve de 0 (sol) a 1 (sombra), ou -1 fora das duas. "n" afasta o ponto da superfície
 * (sem manchas da própria superfície na sombra) e "deFrente" é o cosseno entre ela e o sol (quanto mais
 * de lado, maior a folga). A profundidade do mapa vai de 1 a 1000 m: 0,001 é um metro */
#define GLSL_DO_MAPA_DE_SOMBRA \
    "uniform mat4 luzVP; uniform mat4 luzVPPerto; uniform sampler2D mapaDeSombra; uniform float sombraLigada;\n" \
    "float compararNoMapa(vec2 uv, float z, int metade) {\n" \
    "    ivec2 tamanho = textureSize(mapaDeSombra, 0);\n" \
    "    int lado = tamanho.y;\n" \
    "    vec2 t = uv * float(lado) - 0.5;\n" \
    "    vec2 f = t - floor(t);\n" \
    "    ivec2 b = ivec2(floor(t)) + ivec2(metade * lado - 1, -1);\n" \
    "    ivec2 menor = ivec2(metade * lado, 0), maior = ivec2(metade * lado + lado - 1, lado - 1);\n" \
    "    float s = 0.0;\n" \
    "    for (int y = 0; y < 4; y++) {\n" \
    "        float py = (y == 0) ? 1.0 - f.y : ((y == 3) ? f.y : 1.0);\n" \
    "        for (int x = 0; x < 4; x++) {\n" \
    "            float px = (x == 0) ? 1.0 - f.x : ((x == 3) ? f.x : 1.0);\n" \
    "            float d = texelFetch(mapaDeSombra, clamp(b + ivec2(x, y), menor, maior), 0).r;\n" \
    "            s += px * py * step(d, z);\n" \
    "        }\n" \
    "    }\n" \
    "    return s / 9.0;\n" \
    "}\n" \
    "float sombraNoMapa(vec3 p, vec3 n, float deFrente) {\n" \
    "    float deLado = 1.0 - clamp(deFrente, 0.0, 1.0);\n" \
    "    vec4 q = luzVPPerto * vec4(p + n * 0.04, 1.0);\n" \
    "    q.xyz = q.xyz / q.w * 0.5 + 0.5;\n" \
    "    float dentro = min(min(q.x, 1.0 - q.x), min(q.y, 1.0 - q.y));\n" \
    "    float perto = -1.0;\n" \
    "    if (dentro > 0.01 && q.z < 1.0) {\n" \
    "        perto = compararNoMapa(q.xy, q.z - 0.00006 - 0.00035 * deLado, 0);\n" \
    "        if (dentro > 0.08) return perto;\n" \
    "    }\n" \
    "    vec4 r = luzVP * vec4(p + n * 0.15, 1.0);\n" \
    "    r.xyz = r.xyz / r.w * 0.5 + 0.5;\n" \
    "    float borda = min(min(r.x, 1.0 - r.x), min(r.y, 1.0 - r.y));\n" \
    "    if (borda <= 0.0 || r.z >= 1.0) return perto;\n" \
    "    float longe = compararNoMapa(r.xy, r.z - 0.0008 - 0.003 * deLado, 1) * smoothstep(0.0, 0.08, borda);\n" \
    "    return (perto >= 0.0) ? mix(longe, perto, smoothstep(0.01, 0.08, dentro)) : longe;\n" \
    "}\n"

/* As lâmpadas da noite: as luzes pontuais que mais aparecem para a câmera (os neons, as vitrines, as
 * janelas: lampadas = lugar e raio; coresDasLampadas = cor e, no alfa, a força + 8 × (quadro + 1)).
 * luzDasLampadas devolve a luz que elas somam num ponto, só onde a lâmpada enxerga: o quadro dela em
 * vistaDasLampadas (luzes.c) diz até onde ela chega em cada direção, e o que está atrás de uma parede
 * fica no escuro. Quadro -1: sem a vista, ilumina tudo */
#define MAIS_LAMPADAS         64
#define SLOT_DA_VISTA_DAS_LAMPADAS 14   /* a unidade de textura da imagem das vistas (a sombra do sol é a 15) */
#define SLOT_DA_LISTA_DAS_LAMPADAS 12   /* a unidade da imagem com as listas de lâmpadas de cada célula */
#define SLOT_DA_GRADE_DAS_LAMPADAS 13   /* a unidade da imagem com (começo, quantas) de cada célula */
#define LADO_DA_GRADE         64        /* células de cada lado da grade de lâmpadas */
#define LARGURA_DA_LISTA      256       /* a lista é uma imagem de LARGURA_DA_LISTA x ALTURA_DA_LISTA entradas */
#define ALTURA_DA_LISTA       8
/* A grade: vista de cima, cada célula guarda as lâmpadas cujo raio a alcança (na ordem das lâmpadas, então a
 * soma sai igual à do laço sobre todas). "gradeMedidas" = canto x, canto z, 1 / lado da célula, e 1 se a
 * grade vale (0: o laço percorre todas, como antes). Fora da grade nenhuma lâmpada chega. Cada célula é
 * começo * 64 + quantas, e a lista guarda os números das lâmpadas */
/* As luzes de dentro dos prédios (deDentro): as luminárias de verdade dos interiores (luzes.c, <interior>_luzes.json),
 * no lugar de cada uma no desenho. Só os carros as recebem (lampadasDeDentro = 1 neles, 0 no resto: o cenário de
 * dentro tem a luz delas assada), de dia e de noite. A luz delas cai com o quadrado da distância (o raio só a corta
 * onde ela já é fraca), e a dos focos fica no facho: focosDasLampadas = para onde ele aponta, com o comprimento
 * 1 + a borda macia (a diferença dos cossenos de dentro e de fora), e no w o cosseno da borda de fora (< -1: luz
 * em volta). Elas vão no alfa com o sinal trocado. As luzes que um interior joga para fora pelas portas, de noite
 * (soDoCenario), são só para o cenário: os carros recebem a luz das luminárias de verdade; elas vão no alfa com
 * 4096 a mais */
#define PERTO_DA_LUZ_DE_DENTRO "0.6"   /* m²: perto da lâmpada, a força para de crescer (a lâmpada tem tamanho) */
/* A luz das luminárias de dentro, somada, passa pela mesma curva com que as ferramentas põem a luz do Cycles na
 * cor dos vértices do interior (a escala da tela, ~ 1/2,2): assim o carro clareia como o chão assado em volta dele,
 * no meio de uma poça de luz e longe dela */
#define GAMA_DA_LUZ_DE_DENTRO "0.4545"
#define GLSL_DAS_LAMPADAS \
    "uniform int totalDeLampadas; uniform vec4 lampadas[64]; uniform vec4 coresDasLampadas[64]; uniform vec4 focosDasLampadas[64];\n" \
    "uniform sampler2D vistaDasLampadas; uniform sampler2D gradeDasLampadas; uniform sampler2D listaDasLampadas;\n" \
    "uniform vec4 gradeMedidas; uniform float lampadasDeDentro;\n" \
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
    "    vec3 soma = vec3(0.0), somaDeDentro = vec3(0.0);\n" \
    "    bool porGrade = gradeMedidas.w > 0.5;\n" \
    "    int primeira = 0, quantas = totalDeLampadas;\n" \
    "    if (porGrade) {\n" \
    "        ivec2 celula = ivec2(floor((p.xz - gradeMedidas.xy) * gradeMedidas.z));\n" \
    "        if (celula.x < 0 || celula.y < 0 || celula.x >= 64 || celula.y >= 64) return soma;\n" \
    "        int guardado = int(texelFetch(gradeDasLampadas, celula, 0).r + 0.5);\n" \
    "        primeira = guardado / 64; quantas = guardado - primeira * 64;\n" \
    "    }\n" \
    "    for (int k = 0; k < quantas; k++) {\n" \
    "        int i = k;\n" \
    "        if (porGrade) { int lugar = primeira + k; i = int(texelFetch(listaDasLampadas, ivec2(lugar & 255, lugar >> 8), 0).r + 0.5); }\n" \
    "        float alfa = coresDasLampadas[i].a;\n" \
    "        bool deDentro = alfa < 0.0;\n" \
    "        alfa = abs(alfa);\n" \
    "        bool doCenario = alfa >= 4096.0;\n" \
    "        if (doCenario) alfa -= 4096.0;\n" \
    "        if (deDentro ? lampadasDeDentro < 0.5 : (doCenario && lampadasDeDentro > 0.5)) continue;\n" \
    "        vec3 d = lampadas[i].xyz - p;\n" \
    "        float q = dot(d, d) / (lampadas[i].w * lampadas[i].w);\n" \
    "        if (q >= 1.0) continue;\n" \
    "        float quadro = floor(alfa / 8.0);\n" \
    "        float forca = alfa - quadro * 8.0;\n" \
    "        float distancia = length(d);\n" \
    "        float ve = lampadaVe(quadro - 1.0, -d, distancia);\n" \
    "        if (ve <= 0.0) continue;\n" \
    "        vec3 l = d / max(distancia, 0.0001);\n" \
    "        float virado = max(dot(n, l), 0.0) * 0.75 + 0.25;\n" \
    "        float queda;\n" \
    "        if (deDentro) {\n" \
    "            queda = (1.0 - q * q) * (1.0 - q * q) / (dot(d, d) + " PERTO_DA_LUZ_DE_DENTRO ");\n" \
    "            vec4 f = focosDasLampadas[i];\n" \
    "            if (f.w > -1.5) {\n" \
    "                float comprimento = length(f.xyz);\n" \
    "                queda *= smoothstep(f.w, f.w + max(comprimento - 1.0, 0.01), dot(-l, f.xyz / comprimento));\n" \
    "            }\n" \
    "        } else {\n" \
    "            queda = (1.0 - q) * (1.0 - q);\n" \
    "        }\n" \
    "        vec3 luz = coresDasLampadas[i].rgb * forca * queda * virado * ve;\n" \
    "        if (deDentro) somaDeDentro += luz; else soma += luz;\n" \
    "    }\n" \
    "    return soma + pow(somaDeDentro, vec3(" GAMA_DA_LUZ_DE_DENTRO "));\n" \
    "}\n"

typedef struct {
    Vector3 lugar;
    Vector3 cor;            /* 0 a 1 */
    float raio;             /* metros: até onde ela chega */
    float forca;            /* 0 a 7 */
    int quadro;             /* o quadro dela na imagem das vistas (-1: sem vista) */
    int deDentro;           /* 1: uma luminária de dentro de um prédio (só os carros a recebem; veja acima) */
    int soDoCenario;        /* 1: a luz que um prédio joga para fora de noite (os carros não a recebem) */
    Vector3 foco;           /* as de dentro: para onde o facho aponta (zero: luz em volta) */
    float cosDoFacho;       /* o cosseno da metade da abertura do facho (a borda de fora)... */
    float cosDeDentro;      /* ...e o de onde ele já é pleno (a borda macia fica entre os dois) */
    float forcaDeNoite;     /* as de dentro: a força de noite (a de dia é "forca") */
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
    /* A imagem da hora (o pós-processamento do passeio): a cor da tela, como a de cada pista nas corridas,
     * e a neblina da distância (a cor dela é a do horizonte) */
    Vector3 tomDaTela;
    float contrasteDaTela, saturacaoDaTela, vinhetaDaTela;
    float neblina;          /* densidade por metro, no chão: a bruma da manhã é a mais forte */
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
/* As luminárias de dentro dos prédios (deDentro) só iluminam os shaders marcados aqui (os dos carros) */
void verLampadasDeDentro(Shader shader);
void ligarGradeDasLampadas(void);   /* liga as imagens da grade (chamar a cada quadro, junto da vista) */

#endif
