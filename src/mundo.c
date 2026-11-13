/*
 * mundo.c
 * ------------------------------------------------------------------
 * Constrói e desenha o cenário 3D das pistas. Veja mundo.h.
 *
 * Tudo está em metros. As partes com transparência (alambrado, morros)
 * usam um shader que descarta os pixels transparentes: assim
 * elas escrevem certo na profundidade e não precisam ser ordenadas.
 * ------------------------------------------------------------------
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "mundo.h"
#include "torcida3d.h"
#include "telao.h"
#include "tracado.h"
#include "oval.h"
#include "splat.h"
#include "cenario_mapas.h"
#include "construcao.h"
#include "interlagos.h"
#include "interlagos_materiais.h"
#include "pitstop.h"
#include "raymath.h"
#include "rlgl.h"
#include "obstaculos.h"
#include "noite.h"
#include "hora.h"
#include "corte.h"
#include "espera.h"
#include "exibicao.h"
#include "configuracoes.h"

#define PASTA_TEXTURAS        "recursos/texturas/"
#define ARQUIVO_DO_ESTADIO    "recursos/modelos/copa_pistao.glb"

/* ---------- Asfalto e pintura ---------- */
#define ALTURA_DA_PINTURA     0.04f     /* zebra e tinta ficam um pouco acima do asfalto */
#define LINHAS_DA_CHEGADA     2
#define COLUNAS_DA_CHEGADA    16
#define MEIA_LARGURA_COLCHETE 0.19f     /* colchete do grid, em larguras da pista */
#define FRENTE_COLCHETE       0.62f     /* barra da frente, em frações do segmento */
#define GROSSURA_COLCHETE     0.07f
#define GROSSURA_PERNA        0.025f
#define COR_TINTA             (Color){ 235, 235, 235, 255 }
#define COR_LARGADA_CLARA     (Color){ 240, 240, 240, 255 }
#define COR_LARGADA_ESCURA    (Color){  25,  25,  28, 255 }

/* ---------- Objetos e mapas de modelo 3D (metros) ---------- */
#define PASTA_DOS_OBJETOS     "recursos/modelos/objetos/"
#define MAXIMO_DE_LOTES       800
#define ARQUIVO_DO_OVAL       "recursos/modelos/oval.glb"
#define ARQUIVO_DO_SPLAT      "recursos/modelos/splat.glb"
#define ARQUIVO_DE_INTERLAGOS "recursos/modelos/interlagos.glb"
#define ARQUIVO_DO_MUNDO_ABERTO "recursos/modelos/radiator_springs.glb"
#define FIGURA_DE_INTERLAGOS  1.2f      /* os assentos do modelo têm o tamanho de verdade */
#define LADRILHO_DO_ASFALTO   6.0f      /* metros: a imagem do asfalto se repete a cada tanto */
#define LADRILHO_DA_GRAMA     9.0f
#define MARGEM_DA_SAIA        150.0f    /* metros do modelo além da pista: a borda do terreno mais perto é furo de dentro */
#define COMPRIMENTO_DA_SAIA   6000.0f   /* metros do modelo: até o horizonte */
#define DESCIDA_DA_SAIA       0.02f     /* ela desce de leve, para ficar embaixo de qualquer outro terreno */
#define LADRILHO_DA_SAIA      40.0f
#define COR_DA_AGUA_DO_LAGO   (Color){ 70, 118, 122, 255 }   /* multiplica a imagem azul-clara do modelo */
#define COR_DA_SAIA           (Color){ 120, 128, 104, 255 }   /* a cor média da foto do terreno, sob a grama do jogo */
#define LARGURA_DO_CONCRETO   256       /* pixels da imagem do concreto limpo */
#define ALTURA_DO_CONCRETO    64
#define FOLGA_DA_AREA_DO_AUTODROMO 30.0f  /* metros do modelo além da pista onde o terreno ainda é grama */
#define TRANSICAO_DO_AUTODROMO 120.0f     /* metros do modelo em que a grama vira a foto do terreno */
#define FORCA_DA_BORRACHA     "0.85"
#define FIGURA_DO_OVAL        1.5f      /* metros de largura de uma figura da torcida */
#define FIGURA_DO_SPLAT       1.1f
#define FIGURA_EM_PE          254       /* marca, no alfa, as figuras em pé da torcida (somem onde há torcida 3D) */
#define CARRINHO_POR_FIGURA   0.36f     /* o carrinho 3D, do tamanho da figura: o modelo tem 3 m */
#define ALTURA_DA_FIGURA      0.85f     /* em larguras */
#define PASSOS_PROCURANDO_A_PISTA 7     /* segmentos pulados, procurando o lugar mais perto */
#define METADE_DA_VAGA_PINTADA 4.2f     /* metros: do meio da vaga até a ponta */
#define LARGURA_DA_VAGA_PINTADA 4.6f
#define TRACO_DA_VAGA         0.3f
#define LARGURA_DA_LINHA_DO_LIMITE 4.0f /* metros que a linha do limite avança para o lado da pista */
#define METROS_DA_VAGA_ATE_A_TENDA 8.0f
#define METROS_DA_VAGA_ATE_OS_PNEUS 5.0f
#define TAMANHO_DA_TENDA_DA_EQUIPE 0.8f

/* ---------- Arquibancadas de Interlagos copiadas da da reta (metros do modelo) ----------
 * O módulo: uma seção inteira da arquibancada coberta da reta dos boxes (a estrutura dela vem em
 * seções de 37,5 m), do concreto da frente ao fundo. A pista fica para +x; o fundo, para -x */
#define MODULO_FRENTE_X       -495.5f   /* a frente (o alambrado da pista fica de fora) */
#define MODULO_FUNDO_X        -524.0f
#define MODULO_INICIO_Z        202.75f
#define MODULO_COMPRIMENTO      37.5f
#define MODULO_CHAO_Y           16.6f   /* o chão embaixo dela */
#define MODULO_ABAIXO_DO_CHAO    1.5f   /* o que estiver até aqui abaixo do chão ainda entra */
#define FATIAS_DO_MODULO        15      /* fatias ao longo: cada uma acompanha a curva da pista */
#define ALICERCE                10.0f   /* as paredes descem tanto, para não flutuar no terreno em declive */
#define ALEM_DO_MURO_DA_ARQUIBANCADA 6.0f  /* metros do jogo entre o muro mais longe do trecho e a frente dela */
#define ARQUIBANCADA_DE_DOIS_TELOES 300.0f  /* metros do jogo: mais comprida que isto, ganha dois telões */
#define PERTO_DO_TELAO        20.0f     /* metros para cada lado em que se procura o muro */
#define TELAO_ALEM_DO_MURO    14.0f     /* metros do muro de dentro até o telão */
#define TELAO_MAIS_LONGE      8.0f      /* se o lugar não serve, tenta tanto mais longe */
#define TELAO_LONGE_DE_OUTRA_PISTA 12.0f
#define LONGE_NA_VOLTA        150.0f    /* metros: o que estiver mais longe que isto na volta é "outro trecho" */
#define PASSO_DO_CHAO_DA_ARQUIBANCADA 5.0f /* metros do jogo entre as medidas do chão na frente dela */

/* ---------- Estádio (Copa Pistão) ---------- */
#define MATERIAL_DOS_ASSENTOS 7         /* Material.079 do estádio: a raylib põe o padrão na frente */
#define MAXIMO_DE_VERTICES    60000     /* uma malha da raylib aponta os vértices com 16 bits */

/* ---------- Céu (um domo) e fundo (cilindros), em volta da câmera ---------- */
#define LADOS_DO_CILINDRO     72
#define RAIO_DO_CEU           9000.0f
#define ANEIS_DO_CEU          32
#define ELEVACAO_DA_BASE_DO_CEU (-12.0f)  /* graus: o céu começa abaixo do horizonte */
#define COMECO_DA_IMAGEM_DO_CEU 0.02f     /* a imagem do céu vai até aqui, e daí para cima é a tampa */
#define COR_DO_TOPO_DO_CEU    (Color){ 52, 112, 196, 255 }   /* a cor da linha de cima de fundo/ceu.png */
#define REPETICOES_CEU        2.0f
#define ABAIXO_DA_CAMERA      300.0f    /* a base dos cilindros fica abaixo do horizonte */
#define PLANO_DE_LONGE        14000.0f
#define PLANO_DE_PERTO        0.5f

/* Descarta os pixels transparentes, em vez de misturar */
/* As sombras do sol: o cenário visto do sol, guardado num mapa de profundidade. Quem estiver
 * mais longe do sol do que o que o mapa guardou ali está na sombra de alguma coisa. A normal sai
 * das derivadas da posição e aponta sempre para a câmera: a face que a gente vê, se estiver de
 * costas para o sol, está na própria sombra (senão, nos trilhos e muros finos, o mapa dá um
 * serrilhado de claro e escuro) */
#define GLSL_DA_SOMBRA \
    GLSL_DO_SOL \
    "uniform mat4 luzVP; uniform sampler2D mapaDeSombra; uniform float sombraLigada;\n" \
    "float naSombra(vec3 p) {\n" \
    "    if (sombraLigada < 0.5) return 0.0;\n" \
    "    vec3 n = normalize(cross(dFdx(p), dFdy(p)));\n" \
    "    float deFrente = dot(n, solDoDia);\n" \
    "    vec4 q = luzVP * vec4(p + n * 0.15, 1.0);\n" \
    "    q.xyz = q.xyz / q.w * 0.5 + 0.5;\n" \
    "    if (q.x <= 0.0 || q.x >= 1.0 || q.y <= 0.0 || q.y >= 1.0 || q.z >= 1.0) return 0.0;\n" \
    "    float deCostas = smoothstep(0.02, -0.12, deFrente);\n" \
    "    if (deCostas >= 1.0) return 1.0;\n" \
    "    float vies = 0.0008 + 0.0030 * (1.0 - clamp(deFrente, 0.0, 1.0));\n" \
    "    vec2 passo = 1.0 / vec2(textureSize(mapaDeSombra, 0));\n" \
    "    float s = 0.0;\n" \
    "    for (int x = -1; x <= 1; x++) for (int y = -1; y <= 1; y++)\n" \
    "        s += (q.z - vies > texture(mapaDeSombra, q.xy + vec2(x, y) * passo).r) ? 1.0 : 0.0;\n" \
    "    float borda = min(min(q.x, 1.0 - q.x), min(q.y, 1.0 - q.y));\n" \
    "    return max(s / 9.0 * smoothstep(0.0, 0.08, borda), deCostas);\n" \
    "}\n"
#define FORCA_DA_SOMBRA "0.50"

static const char *VERTICES_DO_SHADER =
    "#version 330\n"
    "in vec3 vertexPosition; in vec2 vertexTexCoord; in vec4 vertexColor;\n"
    "uniform mat4 mvp; uniform mat4 matModel;\n"
    "out vec2 fragTexCoord; out vec4 fragColor; out vec3 fragPos;\n"
    "void main() { fragTexCoord = vertexTexCoord; fragColor = vertexColor;\n"
    "              fragPos = vec3(matModel * vec4(vertexPosition, 1.0));\n"
    "              gl_Position = mvp * vec4(vertexPosition, 1.0); }\n";
static const char *PIXELS_DO_SHADER =
    "#version 330\n"
    "in vec2 fragTexCoord; in vec4 fragColor; in vec3 fragPos;\n"
    "uniform sampler2D texture0; uniform vec4 colDiffuse;\n"
    "out vec4 finalColor;\n"
    GLSL_DA_SOMBRA GLSL_DA_LUZ_DO_DIA
    "void main() { vec4 cor = texture(texture0, fragTexCoord) * colDiffuse * fragColor;\n"
    "              if (cor.a < 0.5) discard;\n"
    "              finalColor = vec4(cor.rgb * (1.0 - " FORCA_DA_SOMBRA " * forcaDaSombra * naSombra(fragPos)), 1.0); }\n";

/* Para o mapa de sombra: só a profundidade, furando o que for transparente (as folhas) */
static const char *VERTICES_DA_PROFUNDIDADE =
    "#version 330\n"
    "in vec3 vertexPosition; in vec2 vertexTexCoord;\n"
    "uniform mat4 mvp;\n"
    "out vec2 fragTexCoord;\n"
    "void main() { fragTexCoord = vertexTexCoord; gl_Position = mvp * vec4(vertexPosition, 1.0); }\n";
static const char *PIXELS_DA_PROFUNDIDADE =
    "#version 330\n"
    "in vec2 fragTexCoord;\n"
    "uniform sampler2D texture0; uniform vec4 colDiffuse;\n"
    "out vec4 finalColor;\n"
    "void main() { if (texture(texture0, fragTexCoord).a * colDiffuse.a < 0.5) discard; finalColor = vec4(1.0); }\n";

/* O pré-passe de profundidade do mundo aberto: o mapa é desenhado antes só na profundidade, com este shader
 * (o mesmo furo dos pixels transparentes do shader dos mapas de dia) e empurrado um tiquinho para trás. Daí o
 * desenho de verdade só sombreia o que está à frente: sem isto, cada pixel era sombreado várias vezes (o shader
 * tem "discard", e o mapa é de dupla face e sem ordem), e de noite cada vez custava as lâmpadas. O empurrão
 * garante que o desenho de verdade nunca fique atrás da profundidade guardada (nada some por arredondamento). */
#define EMPURRAO_DO_PREPASSE "4.0e-6"
static const char *VERTICES_DO_PREPASSE =
    "#version 330\n"
    "in vec3 vertexPosition; in vec2 vertexTexCoord;\n"
    "uniform mat4 mvp;\n"
    "out vec2 fragTexCoord;\n"
    "void main() { fragTexCoord = vertexTexCoord; vec4 p = mvp * vec4(vertexPosition, 1.0);\n"
    "              p.z += " EMPURRAO_DO_PREPASSE " * p.w; gl_Position = p; }\n";
/* A variante para o estádio: o shader da noite multiplica a cor dos vértices antes de descartar, e o pré-passe
 * tem de descartar tudo o que o desenho de verdade descarta (senão escreveria profundidade onde nada é desenhado) */
static const char *VERTICES_DO_PREPASSE_COM_COR =
    "#version 330\n"
    "in vec3 vertexPosition; in vec2 vertexTexCoord; in vec4 vertexColor;\n"
    "uniform mat4 mvp;\n"
    "out vec2 fragTexCoord; out vec4 fragColor;\n"
    "void main() { fragTexCoord = vertexTexCoord; fragColor = vertexColor; vec4 p = mvp * vec4(vertexPosition, 1.0);\n"
    "              p.z += " EMPURRAO_DO_PREPASSE " * p.w; gl_Position = p; }\n";
static const char *PIXELS_DO_PREPASSE_COM_COR =
    "#version 330\n"
    "in vec2 fragTexCoord; in vec4 fragColor;\n"
    "uniform sampler2D texture0; uniform vec4 colDiffuse;\n"
    "out vec4 finalColor;\n"
    "void main() { if ((texture(texture0, fragTexCoord) * colDiffuse * fragColor).a < 0.5) discard; finalColor = vec4(1.0); }\n";
static Shader shaderDoPrepasse, shaderDoPrepasseComCor;
static int prepasseLigado = -1;   /* KATCHAU_SEM_PREPASSE desliga (para comparar) */

/* Os modelos dos objetos que os mapas põem no cenário (cenario_mapas.h) */
typedef enum {
    CENARIO_POSTO_FISCAL,
    CENARIO_TORRE_TV,
    CENARIO_TELAO,
    CENARIO_CAMINHAO_1,   /* e mais 3 */
    CENARIO_TENDA_1 = CENARIO_CAMINHAO_1 + 4,   /* e mais 2 */
    CENARIO_CARRO_1 = CENARIO_TENDA_1 + 3,      /* e mais 5 */
    CENARIO_MASTRO_1 = CENARIO_CARRO_1 + 6,     /* e mais 4 */
    CENARIO_PILHA_1 = CENARIO_MASTRO_1 + 5,     /* e mais 1 */
    TOTAL_DE_MODELOS_DO_CENARIO = CENARIO_PILHA_1 + 2
} ModeloDoCenario;

/* Os modelos .glb dos objetos */
typedef enum {
    MODELO_ARVORE_1,
    MODELO_ARVORE_2,
    MODELO_ARVORE_3,
    MODELO_PNEUS_VERMELHO,
    MODELO_PNEUS_BRANCO,
    MODELO_PORTICO,
    MODELO_ARQUIBANCADA,
    MODELO_CAMAROTE,
    MODELO_REFLETOR,
    MODELO_PINHEIRO_1,
    MODELO_PINHEIRO_2,
    MODELO_COPA_1,
    MODELO_COPA_2,
    MODELO_COPA_3,
    MODELO_TELAO_KATCHAU,
    MODELO_POSTE_RUA,
    MODELO_PRIMEIRO_DO_CENARIO,   /* na ordem de ModeloDoCenario */
    MODELO_FIM_DO_CENARIO = MODELO_PRIMEIRO_DO_CENARIO + TOTAL_DE_MODELOS_DO_CENARIO - 1,
    TOTAL_MODELOS_DE_OBJETO
} ModeloDeObjeto;

static const char *ARQUIVOS_DOS_OBJETOS[TOTAL_MODELOS_DE_OBJETO] = {
    "arvore_1.glb", "arvore_2.glb", "arvore_3.glb", "pneus_vermelho.glb", "pneus_branco.glb",
    "portico.glb", "arquibancada.glb", "camarote.glb", "refletor.glb",
    "pinheiro_1.glb", "pinheiro_2.glb", "copa_1.glb", "copa_2.glb", "copa_3.glb", "telao_katchau.glb", "poste_rua.glb",
    "posto_fiscal.glb", "torre_tv.glb", "telao.glb",
    "caminhao_1.glb", "caminhao_2.glb", "caminhao_3.glb", "caminhao_4.glb", "tenda_1.glb", "tenda_2.glb", "tenda_3.glb",
    "carro_rua_1.glb", "carro_rua_2.glb", "carro_rua_3.glb", "carro_rua_4.glb", "carro_rua_5.glb", "carro_rua_6.glb",
    "mastro_1.glb", "mastro_2.glb", "mastro_3.glb", "mastro_4.glb", "mastro_5.glb", "pilha_1.glb", "pilha_2.glb",
};

/* Centenas de cópias do mesmo modelo desenhadas uma a uma pesariam: elas são
 * juntadas em malhas únicas, os lotes, uma para cada textura */
typedef struct {
    Mesh malha;
    Texture2D textura;
    int brilha;          /* 1: a tela de um telão, que emite luz */
    BoundingBox caixa;   /* para o corte pelo campo de visão (corte.c) */
} LoteDeObjetos;

/* Um lote ainda sendo montado */
typedef struct {
    float *vertices, *texturas;
    unsigned char *cores;
    unsigned short *indices;
    int totalVertices, capacidadeVertices;
    int totalIndices, capacidadeIndices;
} LoteAberto;

static Shader shader;
static Shader shaderDosMapasDeDia;

/* As sombras do sol */
static int ladoDoMapaDeSombraAtual = 2048;   /* o lado do mapa de sombra: o da qualidade escolhida, definido ao carregar */
#define LADO_DO_MAPA_DE_SOMBRA ladoDoMapaDeSombraAtual
#define AREA_DA_SOMBRA        330.0f   /* metros: o quadrado em volta da câmera que tem sombra */
#define ADIANTE_DA_SOMBRA     90.0f    /* metros: o meio do quadrado fica à frente da câmera */
#define ALTURA_DO_SOL         500.0f   /* metros: de onde o sol "olha" */
#define COR_DA_TELA_NO_HALO   (Color){ 215, 215, 215, 255 }   /* as telas brilham, mas menos que um refletor */
#define REFAZER_A_SOMBRA      25.0f   /* metros que a câmera anda até o mapa de sombra ser refeito */
#define SOL_ANDOU             0.999996f /* cosseno (0,16°): o sol andou isso, e o mapa de sombra é refeito */
#define SLOT_DA_SOMBRA        15       /* a unidade de textura do mapa (as do material vão até 11) */
static Shader shaderDaProfundidade;
static RenderTexture2D mapaDeSombra;
static int temMapaDeSombra = 0;
static int sombraLigadaAgora = 0;
static void carregarSombras(void);
static void desenharLote(int i, Matrix origem);

/* As pistas de dia vêm com as imagens sem luz nenhuma: aqui entra o mesmo sol de luzDoSol,
 * com a direção da superfície saindo das derivadas da posição. No passeio, a luz muda com a
 * hora (hora.c): os multiplicadores valem 1 ao meio-dia, e o resultado é o de sempre */
static const char *VERTICES_DOS_MAPAS_DE_DIA =
    "#version 330\n"
    "in vec3 vertexPosition; in vec2 vertexTexCoord;\n"
    "uniform mat4 mvp; uniform mat4 matModel;\n"
    "out vec2 fragTexCoord; out vec3 fragPos;\n"
    "void main() { fragTexCoord = vertexTexCoord;\n"
    "              fragPos = vec3(matModel * vec4(vertexPosition, 1.0));\n"
    "              gl_Position = mvp * vec4(vertexPosition, 1.0); }\n";
static const char *PIXELS_DOS_MAPAS_DE_DIA =
    "#version 330\n"
    "in vec2 fragTexCoord; in vec3 fragPos;\n"
    "uniform sampler2D texture0; uniform vec4 colDiffuse;\n"
    "out vec4 finalColor;\n"
    GLSL_DA_SOMBRA GLSL_DA_LUZ_DO_DIA GLSL_DAS_LAMPADAS
    "void main() { vec4 cor = texture(texture0, fragTexCoord) * colDiffuse;\n"
    "              if (cor.a < 0.5) discard;\n"
    "              cor.rgb *= 1.0 - " FORCA_DA_SOMBRA " * forcaDaSombra * naSombra(fragPos);\n"
    "              vec3 n = normalize(cross(dFdx(fragPos), dFdy(fragPos)));\n"
    "              vec3 luz = 0.58 * luzDeBase + 0.30 * (n.y * 0.5 + 0.5) * luzDoCeu\n"
    "                         + 0.38 * max(dot(n, solDoDia), 0.0) * luzDoSol + luzDasLampadas(fragPos, n);\n"
    "              finalColor = vec4(cor.rgb * luz * 0.92, 1.0); }\n";

/* Interlagos: o modelo veio de outro jogo, e cada material pede um jeito de desenhar
 * (veja interlagos_materiais.h). Todos têm a mesma luz e a mesma sombra dos mapas de dia */
#define GLSL_DA_LUZ_DOS_MAPAS \
    GLSL_DA_LUZ_DO_DIA \
    "vec3 iluminar(vec3 cor) {\n" \
    "    cor *= 1.0 - " FORCA_DA_SOMBRA " * forcaDaSombra * naSombra(fragPos);\n" \
    "    vec3 n = normalize(cross(dFdx(fragPos), dFdy(fragPos)));\n" \
    "    vec3 luz = 0.58 * luzDeBase + 0.30 * (n.y * 0.5 + 0.5) * luzDoCeu + 0.38 * max(dot(n, solDoDia), 0.0) * luzDoSol;\n" \
    "    return cor * luz * 0.92;\n" \
    "}\n"
#define GLSL_DO_COMECO_DE_INTERLAGOS \
    "#version 330\n" \
    "in vec2 fragTexCoord; in vec3 fragPos;\n" \
    "uniform sampler2D texture0; uniform sampler2D texture1; uniform vec4 colDiffuse;\n" \
    "uniform float metrosDoLadrilho;\n" \
    "out vec4 finalColor;\n" \
    GLSL_DA_SOMBRA GLSL_DA_LUZ_DOS_MAPAS
static const char *PIXELS_DE_INTERLAGOS[] = {
    [DESENHO_OPACO] = GLSL_DO_COMECO_DE_INTERLAGOS
        "void main() { vec4 cor = texture(texture0, fragTexCoord) * colDiffuse;\n"
        "              finalColor = vec4(iluminar(cor.rgb), 1.0); }\n",
    /* O furo vem de uma versão detalhada da imagem: nas reduzidas, a média dos fios de um
     * alambrado passa de meio e ele viraria uma parede cinza ao longe */
    [DESENHO_RECORTE] = GLSL_DO_COMECO_DE_INTERLAGOS
        "void main() { vec4 cor = texture(texture0, fragTexCoord) * colDiffuse;\n"
        "              vec2 tamanho = vec2(textureSize(texture0, 0));\n"
        "              vec2 dx = dFdx(fragTexCoord * tamanho), dy = dFdy(fragTexCoord * tamanho);\n"
        "              float nivel = 0.5 * log2(max(dot(dx, dx), dot(dy, dy)));\n"
        "              float alfa = textureLod(texture0, fragTexCoord, clamp(nivel, 0.0, 1.0)).a * colDiffuse.a;\n"
        "              if (alfa < 0.5) discard;\n"
        "              finalColor = vec4(iluminar(cor.rgb), 1.0); }\n",
    /* O asfalto e a grama do jogo, presos ao chão do mundo: a imagem tem sempre o mesmo tamanho */
    [DESENHO_ASFALTO] = GLSL_DO_COMECO_DE_INTERLAGOS
        "void main() { vec4 cor = texture(texture0, fragPos.xz / metrosDoLadrilho) * colDiffuse;\n"
        "              finalColor = vec4(iluminar(cor.rgb), 1.0); }\n",
    /* A tinta (o alfa da imagem) sobre o asfalto do jogo, que vem na textura 1 */
    [DESENHO_DECALQUE] = GLSL_DO_COMECO_DE_INTERLAGOS
        "void main() { vec4 tinta = texture(texture0, fragTexCoord);\n"
        "              vec3 asfalto = texture(texture1, fragPos.xz / metrosDoLadrilho).rgb;\n"
        "              vec3 cor = mix(asfalto, tinta.rgb, smoothstep(0.35, 0.65, tinta.a)) * colDiffuse.rgb;\n"
        "              finalColor = vec4(iluminar(cor), 1.0); }\n",
    /* A borracha do traçado e as sombras pintadas: só escurecem o que está embaixo */
    [DESENHO_SOMBRA] = GLSL_DO_COMECO_DE_INTERLAGOS
        "void main() { float a = texture(texture0, fragTexCoord).a * colDiffuse.a;\n"
        "              finalColor = vec4(0.0, 0.0, 0.0, a * " FORCA_DA_BORRACHA "); }\n",
    [DESENHO_TELA] = GLSL_DO_COMECO_DE_INTERLAGOS
        "void main() { vec4 cor = texture(texture0, fragTexCoord) * colDiffuse;\n"
        "              if (cor.a < 0.02) discard;\n"
        "              finalColor = vec4(iluminar(cor.rgb), cor.a); }\n",
    [DESENHO_GRAMA] = GLSL_DO_COMECO_DE_INTERLAGOS
        "void main() { vec4 cor = texture(texture0, fragPos.xz / metrosDoLadrilho) * colDiffuse;\n"
        "              finalColor = vec4(iluminar(cor.rgb), 1.0); }\n",
    /* A foto de satélite do terreno é borrada de perto: dentro da área do autódromo (e numa faixa
     * em volta, misturando aos poucos) entra a grama do jogo, que vem na textura 1 */
    [DESENHO_TERRENO] = GLSL_DO_COMECO_DE_INTERLAGOS
        "uniform vec4 areaDoAutodromo; uniform float transicaoDoAutodromo;\n"
        "void main() { vec3 foto = texture(texture0, fragTexCoord).rgb * colDiffuse.rgb;\n"
        "              vec3 grama = texture(texture1, fragPos.xz / metrosDoLadrilho).rgb;\n"
        "              vec2 dentro = min(fragPos.xz - areaDoAutodromo.xy, areaDoAutodromo.zw - fragPos.xz);\n"
        "              float perto = smoothstep(-transicaoDoAutodromo, 0.0, min(dentro.x, dentro.y));\n"
        "              float claro = dot(foto, vec3(0.333));\n"
        "              vec3 cor = mix(foto, grama * (0.85 + 0.3 * claro), perto);\n"
        "              finalColor = vec4(iluminar(cor), 1.0); }\n",
    [DESENHO_ESCONDIDO] = NULL,
};
#define TOTAL_DE_DESENHOS (DESENHO_ESCONDIDO + 1)
static Shader shadersDeInterlagos[TOTAL_DE_DESENHOS];

/* Os shaders que recebem a sombra do sol */
#define MAXIMO_QUE_RECEBEM_SOMBRA 12
static Shader recebemSombra[MAXIMO_QUE_RECEBEM_SOMBRA];
static int totalQueRecebemSombra = 0;
/* Onde cada um desses shaders guarda "sombraLigada" e "luzVP". Procurar pelo nome é uma
 * consulta por texto dentro do driver de vídeo, e isso acontecia a cada quadro, para os dez
 * shaders — mesmo com as sombras desligadas nas configurações. O lugar não muda: é achado
 * uma vez, no carregamento. */
static int locSombraLigada[MAXIMO_QUE_RECEBEM_SOMBRA];
static int locLuzVP[MAXIMO_QUE_RECEBEM_SOMBRA];

static Material material;   /* o mesmo material, trocando a textura a cada desenho */
static Model mundoAberto;   /* o passeio livre: carregado na primeira vez (carregarMundoAberto) */
static int temMundoAberto = 0;
static const Model *modelosNoMundoAberto[MODELOS_NO_MUNDO_ABERTO];   /* o Mack e as rodas (colocarNoMundoAberto) */
static const Matrix *matrizesNoMundoAberto[MODELOS_NO_MUNDO_ABERTO];
static Model estadio;
static BoundingBox *caixasDoEstadio = NULL;
static Mesh pinturaDoEstadio;
static Mesh ceuDeDia, tampaDoCeu, ceuDeNoite;
static Texture2D texturaCeu;

/* O céu do passeio muda com a hora (hora.c): a imagem de dia é recolorida (o céu num degradê do
 * horizonte para o alto, as nuvens, que são as partes sem cor da imagem, com a cor da hora), o lado
 * do sol brilha no fim da tarde, e de noite entram as estrelas e a lua. Ao meio-dia, a imagem como
 * ela é, mais o sol */
static Shader shaderDoCeu;
static Material materialDoCeu;
static int locMisturaDoDia, locCorDoHorizonte, locCorDoAlto, locCorDasNuvens, locCorDoBrilho, locEstrelas,
           locSolNoCeu, locCorDoSol, locLuaNoCeu, locBrilhoDaLua;
static const char *VERTICES_DO_CEU =
    "#version 330\n"
    "in vec3 vertexPosition; in vec2 vertexTexCoord; in vec4 vertexColor;\n"
    "uniform mat4 mvp;\n"
    "out vec2 fragTexCoord; out vec4 fragColor; out vec3 direcao;\n"
    "void main() { fragTexCoord = vertexTexCoord; fragColor = vertexColor; direcao = vertexPosition;\n"
    "              gl_Position = mvp * vec4(vertexPosition, 1.0); }\n";
static const char *PIXELS_DO_CEU =
    "#version 330\n"
    "in vec2 fragTexCoord; in vec4 fragColor; in vec3 direcao;\n"
    "uniform sampler2D texture0; uniform sampler2D texture1; uniform vec4 colDiffuse;\n"
    "uniform float misturaDoDia; uniform vec3 corDoHorizonte; uniform vec3 corDoAlto; uniform vec3 corDasNuvens;\n"
    "uniform vec3 corDoBrilho; uniform float estrelas; uniform vec3 solNoCeu; uniform vec3 corDoSol;\n"
    "uniform vec3 luaNoCeu; uniform float brilhoDaLua;\n"
    "out vec4 finalColor;\n"
    "void main() {\n"
    "    vec3 d = normalize(direcao);\n"
    "    vec3 imagem = (texture(texture0, fragTexCoord) * colDiffuse * fragColor).rgb;\n"
    "    float maior = max(imagem.r, max(imagem.g, imagem.b)), menor = min(imagem.r, min(imagem.g, imagem.b));\n"
    "    float nuvem = smoothstep(0.30, 0.10, (maior - menor) / max(maior, 0.001)) * smoothstep(0.55, 0.85, maior);\n"
    "    float alto = clamp(d.y, 0.0, 1.0);\n"
    "    vec2 h = normalize(d.xz + vec2(0.00001)), hs = normalize(solNoCeu.xz + vec2(0.00001));\n"
    "    float ladoDoSol = pow(max(dot(h, hs), 0.0), 3.0) * (1.0 - smoothstep(0.0, 0.45, alto));\n"
    "    vec3 ceu = mix(corDoHorizonte, corDoAlto, pow(alto, 0.55)) + corDoBrilho * ladoDoSol;\n"
    "    vec3 nuvens = corDasNuvens * (0.55 + 0.45 * maior) + corDoBrilho * ladoDoSol * 0.6;\n"
    "    vec3 cor = mix(mix(ceu, nuvens, nuvem), imagem, misturaDoDia);\n"
    "    vec2 uv = vec2(atan(d.x, d.z) * 0.15915, asin(clamp(d.y, -1.0, 1.0)) * 0.63662);\n"
    "    float estrela = texture(texture1, uv * vec2(2.0, 1.2)).r;\n"
    "    cor += vec3(estrela) * estrelas * 0.85 * (1.0 - nuvem) * smoothstep(0.05, 0.25, d.y);\n"
    "    float s = max(dot(d, solNoCeu), 0.0);\n"
    "    float acima = smoothstep(-0.02, 0.01, d.y) * (1.0 - 0.85 * nuvem);\n"
    "    cor += corDoSol * acima * (smoothstep(0.99980, 0.99988, s) * 1.6 + pow(s, 400.0) * 0.5 + pow(s, 12.0) * 0.15);\n"
    "    float l = max(dot(d, luaNoCeu), 0.0);\n"
    "    cor += vec3(0.92, 0.94, 1.0) * brilhoDaLua * (1.0 - 0.7 * nuvem)\n"
    "           * (smoothstep(0.99990, 0.99995, l) * 0.95 + pow(l, 300.0) * 0.12);\n"
    "    finalColor = vec4(cor, 1.0);\n"
    "}\n";
static Texture2D texturaAsfalto[2], texturaGrama;
static Texture2D texturaDoConcreto;   /* o concreto limpo dos muros de Interlagos (gerado) */
static Model modelosDosObjetos[TOTAL_MODELOS_DE_OBJETO];

/* Onde estão as telas dos telões de cada pista. A transmissão (telao.c) redesenha o mundo
 * inteiro para caber nelas, e isso custa quase tanto quanto desenhar o jogo; quando nenhuma
 * tela está no campo de visão, o que seria desenhado ali é invisível e não vale o preço. */
#define MAXIMO_DE_TELAS  24
static BoundingBox telasDoCenario[TOTAL_PISTAS][MAXIMO_DE_TELAS];
static int totalDeTelas[TOTAL_PISTAS];
static int telasDemais[TOTAL_PISTAS];   /* não couberam: a pista volta a desenhar sempre */
static IdPista pistaEmCarregamento = PISTA_COPA_PISTAO;

/*
 * registrarTela
 * Guarda a caixa de uma tela de telão da pista que está sendo carregada. Se não couber,
 * a pista é marcada e passa a desenhar a transmissão sempre: é melhor pagar o preço do que
 * deixar um telão com a imagem congelada à vista do jogador.
 * Recebe: a caixa, em coordenadas do mundo.
 * Devolve: nada.
 */
static void registrarTela(BoundingBox caixa) {
    int id = (int) pistaEmCarregamento;
    if (id < 0 || id >= TOTAL_PISTAS) {
        return;
    }
    if (totalDeTelas[id] >= MAXIMO_DE_TELAS) {
        telasDemais[id] = 1;
        TraceLog(LOG_WARNING, "MUNDO: telões demais na pista %d; aumente MAXIMO_DE_TELAS", id);
        return;
    }
    telasDoCenario[id][totalDeTelas[id]++] = caixa;
}
static LoteDeObjetos lotes[MAXIMO_DE_LOTES];
static int totalDeLotes = 0;
static int primeiroLoteDoEstadio = 0, fimDosLotesDoEstadio = 0;

/* As pistas de dia que vêm prontas em um modelo 3D: só falta o que é do jogo */
typedef struct {
    int existe;
    Model modelo;
    float escala;
    Mesh pintura;    /* a linha, o grid e as vagas */
    Mesh torcida;
    Mesh saia;                       /* o chão além do terreno do modelo, até o horizonte (Interlagos) */
    int primeiroLote, fimDosLotes;   /* as árvores e as estruturas */
    /* Para o corte pelo campo de visão (corte.c): a caixa de cada malha, já no mundo, e as
     * malhas separadas por tipo de desenho — assim o laço de desenho não percorre o modelo
     * inteiro três vezes procurando quem é de cada tipo */
    BoundingBox *caixas;
    int *opacas, totalOpacas;
    int *deSombra, totalDeSombra;
    int *deTela, totalDeTela;
} MapaDeDia;
static MapaDeDia mapasDeDia[TOTAL_PISTAS];
static void prepararMaterialDeInterlagos(Material *material, DesenhoDoMaterial desenho);
static void tirarParedesInvisiveis(Model *modelo);
static Texture2D gerarConcretoLimpo(void);
static void trocarImagensGastas(Model *modelo);
static void prepararCaixasDoMapa(MapaDeDia *mapa, IdPista id);
static void colocarTeloesDeInterlagos(Pista *pista, const Model *modelo);
static void colocarTeloesDasArquibancadas(Pista *pista, const Model *modelo);
static void definirAreaDoAutodromo(Pista *pista);
static int construirArquibancadasQueFaltam(Pista *pista, MapaDeDia *mapa, const LugarNaTorcida *lugares, int totalDeLugares,
                                           LugarNaTorcida **todosOsLugares);
static Mesh construirSaiaDoTerreno(const Model *modelo, float escala, Pista *pista);
static float chaoNoModelo(const Model *modelo, float escala, float x, float z);

/* ==================================================================
 * Construção de malhas
 * ================================================================== */

/*
 * pontoNaEstrada
 * Um ponto da pista como vetor 3D.
 * Recebe: o segmento, quanto andar nele (0 a 1), a lateral e quanto subir.
 * Devolve: o ponto.
 */
static Vector3 pontoNaEstrada(Segmento *segmento, float frente, float lateral, float subir) {
    PosicaoNaPista posicao = { segmento, frente * COMPRIMENTO_SEGMENTO };
    PontoNoMundo ponto = pontoNoMundo(posicao, lateral);
    return (Vector3){ ponto.x, ponto.y + subir, ponto.z };
}

/*
 * pintarRetangulo
 * Um retângulo de tinta no asfalto de um segmento.
 * Recebe: a construção, o segmento, de onde até onde na frente (0 a 1) e
 *         na lateral, e a cor.
 * Devolve: nada.
 */
static void pintarRetangulo(Construcao *c, Segmento *segmento, float frente0, float frente1,
                            float lado0, float lado1, Color cor) {
    Vector2 zero = { 0.0f, 0.0f };
    adicionarQuadrilatero(c, pontoNaEstrada(segmento, frente0, lado0, ALTURA_DA_PINTURA),
                          pontoNaEstrada(segmento, frente0, lado1, ALTURA_DA_PINTURA),
                          pontoNaEstrada(segmento, frente1, lado1, ALTURA_DA_PINTURA),
                          pontoNaEstrada(segmento, frente1, lado0, ALTURA_DA_PINTURA),
                          zero, zero, zero, zero, cor);
}

/*
 * pintarLargadaEGrid
 * A linha de chegada quadriculada e os colchetes do grid, em qualquer pista.
 * Recebe: a construção e a pista.
 * Devolve: nada.
 */
static void pintarLargadaEGrid(Construcao *c, Pista *pista) {
    for (int linha = 0; linha < LINHAS_DA_CHEGADA; linha++) {
        for (int coluna = 0; coluna < COLUNAS_DA_CHEGADA; coluna++) {
            float frente0 = (float) linha / LINHAS_DA_CHEGADA;
            float frente1 = (float) (linha + 1) / LINHAS_DA_CHEGADA;
            float lado0 = -1.0f + 2.0f * coluna / COLUNAS_DA_CHEGADA;
            float lado1 = -1.0f + 2.0f * (coluna + 1) / COLUNAS_DA_CHEGADA;
            Color cor = ((linha + coluna) % 2 == 0) ? COR_LARGADA_CLARA : COR_LARGADA_ESCURA;
            pintarRetangulo(c, pista->largada, frente0, frente1, lado0, lado1, cor);
        }
    }

    Segmento *atual = pista->largada;
    do {
        for (Obstaculo *objeto = atual->obstaculos; objeto != NULL; objeto = objeto->proximo) {
            if (objeto->tipo != OBJETO_MARCA_GRID) {
                continue;
            }
            float esquerda = objeto->deslocamentoLateral - MEIA_LARGURA_COLCHETE;
            float direita = objeto->deslocamentoLateral + MEIA_LARGURA_COLCHETE;
            pintarRetangulo(c, atual, FRENTE_COLCHETE - GROSSURA_COLCHETE, FRENTE_COLCHETE, esquerda, direita, COR_TINTA);
            pintarRetangulo(c, atual, 0.0f, FRENTE_COLCHETE, esquerda, esquerda + GROSSURA_PERNA, COR_TINTA);
            pintarRetangulo(c, atual, 0.0f, FRENTE_COLCHETE, direita - GROSSURA_PERNA, direita, COR_TINTA);
        }
        atual = atual->proximo;
    } while (atual != pista->largada);
}

/*
 * fecharLote
 * Envia um lote pronto para a placa de vídeo e deixa o aberto vazio, para
 * continuar montando. A Mesh passa a ser dona dos vetores.
 * Recebe: o lote aberto e a textura dele.
 * Devolve: nada.
 */
static void fecharLote(LoteAberto *aberto, Texture2D textura) {
    if (aberto->totalVertices == 0) {
        return;
    }
    if (totalDeLotes == MAXIMO_DE_LOTES) {
        TraceLog(LOG_ERROR, "MUNDO: lotes demais; aumente MAXIMO_DE_LOTES");
        return;
    }
    Mesh malha = { 0 };
    malha.vertexCount = aberto->totalVertices;
    malha.triangleCount = aberto->totalIndices / 3;
    malha.vertices = aberto->vertices;
    malha.texcoords = aberto->texturas;
    malha.colors = aberto->cores;
    malha.indices = aberto->indices;
    UploadMesh(&malha, false);
    lotes[totalDeLotes].malha = malha;
    lotes[totalDeLotes].textura = textura;
    lotes[totalDeLotes].brilha = 0;
    /* O lote já vem com os vértices no lugar deles no mundo (é desenhado com a matriz de origem) */
    lotes[totalDeLotes].caixa = caixaDaMalha(malha, MatrixIdentity());
    totalDeLotes++;
    *aberto = (LoteAberto){ 0 };
}

/*
 * juntarCopia
 * Copia uma peça do modelo para o lote, já no lugar dela no mundo. A cor de
 * cada vértice leva a cor do material e a luz do sol.
 * Recebe: o lote, a peça, a cor do material, a textura e a matriz.
 * Devolve: nada.
 */
static void juntarCopia(LoteAberto *lote, Mesh peca, Color corDoMaterial, Texture2D textura, Matrix noMundo) {
    if (lote->totalVertices + peca.vertexCount > MAXIMO_DE_VERTICES) {
        fecharLote(lote, textura);
    }
    int totalNovo = lote->totalVertices + peca.vertexCount;
    if (totalNovo > lote->capacidadeVertices) {
        lote->capacidadeVertices = MAXIMO_DE_VERTICES;
        lote->vertices = MemRealloc(lote->vertices, lote->capacidadeVertices * 3 * sizeof(float));
        lote->texturas = MemRealloc(lote->texturas, lote->capacidadeVertices * 2 * sizeof(float));
        lote->cores = MemRealloc(lote->cores, lote->capacidadeVertices * 4);
    }
    int indicesDaPeca = (peca.indices != NULL) ? peca.triangleCount * 3 : peca.vertexCount;
    if (lote->totalIndices + indicesDaPeca > lote->capacidadeIndices) {
        lote->capacidadeIndices = (lote->capacidadeIndices == 0) ? 65536 : lote->capacidadeIndices * 2;
        lote->indices = MemRealloc(lote->indices, lote->capacidadeIndices * sizeof(unsigned short));
    }

    /* A rotação sem o deslocamento, para girar as normais */
    Matrix giro = noMundo;
    giro.m12 = giro.m13 = giro.m14 = 0.0f;

    for (int i = 0; i < peca.vertexCount; i++) {
        int v = lote->totalVertices + i;
        Vector3 ponto = Vector3Transform((Vector3){ peca.vertices[i * 3], peca.vertices[i * 3 + 1], peca.vertices[i * 3 + 2] }, noMundo);
        lote->vertices[v * 3] = ponto.x;
        lote->vertices[v * 3 + 1] = ponto.y;
        lote->vertices[v * 3 + 2] = ponto.z;
        lote->texturas[v * 2] = (peca.texcoords != NULL) ? peca.texcoords[i * 2] : 0.0f;
        lote->texturas[v * 2 + 1] = (peca.texcoords != NULL) ? peca.texcoords[i * 2 + 1] : 0.0f;

        Vector3 normal = { 0.0f, 1.0f, 0.0f };
        if (peca.normals != NULL) {
            normal = Vector3Transform((Vector3){ peca.normals[i * 3], peca.normals[i * 3 + 1], peca.normals[i * 3 + 2] }, giro);
        }
        float luz = luzDoSol(normal);
        lote->cores[v * 4] = (unsigned char) fminf(corDoMaterial.r * luz, 255.0f);
        lote->cores[v * 4 + 1] = (unsigned char) fminf(corDoMaterial.g * luz, 255.0f);
        lote->cores[v * 4 + 2] = (unsigned char) fminf(corDoMaterial.b * luz, 255.0f);
        lote->cores[v * 4 + 3] = 255;
    }
    for (int i = 0; i < indicesDaPeca; i++) {
        int original = (peca.indices != NULL) ? peca.indices[i] : i;
        lote->indices[lote->totalIndices + i] = (unsigned short) (lote->totalVertices + original);
    }
    lote->totalVertices = totalNovo;
    lote->totalIndices += indicesDaPeca;
}

/* Um lote aberto para cada peça de cada modelo */
#define MAXIMO_DE_PECAS 24
static LoteAberto lotesAbertos[TOTAL_MODELOS_DE_OBJETO][MAXIMO_DE_PECAS];

/*
 * colocarModelo
 * Põe uma cópia de um modelo no mundo.
 * Recebe: qual modelo, o pé dele, o giro em volta de y (radianos) e o tamanho.
 * Devolve: nada.
 */
static void colocarModelo(ModeloDeObjeto qual, Vector3 pe, float giro, float tamanho) {
    Model modelo = modelosDosObjetos[qual];
    Matrix noMundo = MatrixMultiply(MatrixMultiply(MatrixScale(tamanho, tamanho, tamanho), MatrixRotateY(giro)),
                                    MatrixTranslate(pe.x, pe.y, pe.z));
    if ((qual == MODELO_TELAO_KATCHAU || qual == MODELO_PRIMEIRO_DO_CENARIO + CENARIO_TELAO)
        && modelo.meshCount > 0) {
        BoundingBox tela = caixaDaMalha(modelo.meshes[0], noMundo);
        for (int i = 1; i < modelo.meshCount; i++) {
            BoundingBox peca = caixaDaMalha(modelo.meshes[i], noMundo);
            tela.min = Vector3Min(tela.min, peca.min);
            tela.max = Vector3Max(tela.max, peca.max);
        }
        registrarTela(tela);
    }
    for (int i = 0; i < modelo.meshCount && i < MAXIMO_DE_PECAS; i++) {
        Material peca = modelo.materials[modelo.meshMaterial[i]];
        juntarCopia(&lotesAbertos[qual][i], modelo.meshes[i], peca.maps[MATERIAL_MAP_ALBEDO].color,
                    peca.maps[MATERIAL_MAP_ALBEDO].texture, noMundo);
    }
}

/*
 * carregarModelosDosObjetos
 * Lê os .glb. As texturas ganham mipmaps, para não tremerem ao longe.
 * Recebe: nada.
 * Devolve: nada.
 */
#define LARGURA_DA_TELA_DE_ESPERA 320   /* a imagem da tela dos telões (gerar_telao3d.py) */
#define ALTURA_DA_TELA_DE_ESPERA  192

static void carregarModelosDosObjetos(void) {
    for (int i = 0; i < TOTAL_MODELOS_DE_OBJETO; i++) {
        modelosDosObjetos[i] = LoadModel(TextFormat("%s%s", PASTA_DOS_OBJETOS, ARQUIVOS_DOS_OBJETOS[i]));
        for (int m = 0; m < modelosDosObjetos[i].materialCount; m++) {
            Texture2D *textura = &modelosDosObjetos[i].materials[m].maps[MATERIAL_MAP_ALBEDO].texture;
            if (textura->id != 0 && textura->id != rlGetTextureIdDefault()) {
                GenTextureMipmaps(textura);
                SetTextureFilter(*textura, TEXTURE_FILTER_TRILINEAR);
                SetTextureWrap(*textura, TEXTURE_WRAP_REPEAT);
            }
        }
    }
}

/*
 * fecharTodosOsLotes
 * Termina os lotes que ainda estão sendo juntados.
 * Recebe: nada.
 * Devolve: nada.
 */
static void fecharTodosOsLotes(void) {
    for (int m = 0; m < TOTAL_MODELOS_DE_OBJETO; m++) {
        for (int i = 0; i < modelosDosObjetos[m].meshCount && i < MAXIMO_DE_PECAS; i++) {
            Material peca = modelosDosObjetos[m].materials[modelosDosObjetos[m].meshMaterial[i]];
            fecharLote(&lotesAbertos[m][i], peca.maps[MATERIAL_MAP_ALBEDO].texture);
        }
    }
}

/*
 * colocarNoCenario
 * Recebe: um modelo do cenário, o pé, o giro e o tamanho.
 * Devolve: nada.
 */
static void colocarNoCenario(ModeloDoCenario qual, Vector3 pe, float giro, float tamanho) {
    colocarModelo((ModeloDeObjeto) (MODELO_PRIMEIRO_DO_CENARIO + qual), pe, giro, tamanho);
}


/*
 * adicionarAneisDoCeu
 * Um pedaço do domo do céu, entre duas elevações. De dia, a imagem aparece como
 * num cilindro em volta da câmera; à noite, as estrelas vão por igual até o alto.
 * Recebe: a construção, as elevações em graus, a altura da imagem (0 = de noite) e a cor.
 * Devolve: nada.
 */
static void adicionarAneisDoCeu(Construcao *c, float de, float ate, float alturaDaImagem, Color cor) {
    for (int anel = 0; anel < ANEIS_DO_CEU; anel++) {
        float e0 = de + (ate - de) * anel / ANEIS_DO_CEU;
        float e1 = de + (ate - de) * (anel + 1) / ANEIS_DO_CEU;
        float v0, v1;
        if (alturaDaImagem > 0.0f) {
            v0 = 1.0f - (RAIO_DO_CEU * tanf(e0 * DEG2RAD) + ABAIXO_DA_CAMERA) / alturaDaImagem;
            v1 = 1.0f - (RAIO_DO_CEU * tanf(e1 * DEG2RAD) + ABAIXO_DA_CAMERA) / alturaDaImagem;
        } else {
            v0 = 1.0f - (e0 - de) / (ate - de);
            v1 = 1.0f - (e1 - de) / (ate - de);
        }
        for (int i = 0; i < LADOS_DO_CILINDRO; i++) {
            float a0 = 2.0f * PI * i / LADOS_DO_CILINDRO, a1 = 2.0f * PI * (i + 1) / LADOS_DO_CILINDRO;
            float u0 = REPETICOES_CEU * i / LADOS_DO_CILINDRO, u1 = REPETICOES_CEU * (i + 1) / LADOS_DO_CILINDRO;
            float r0 = cosf(e0 * DEG2RAD) * RAIO_DO_CEU, r1 = cosf(e1 * DEG2RAD) * RAIO_DO_CEU;
            float y0 = sinf(e0 * DEG2RAD) * RAIO_DO_CEU, y1 = sinf(e1 * DEG2RAD) * RAIO_DO_CEU;
            Vector3 p0 = { sinf(a0) * r0, y0, -cosf(a0) * r0 }, p1 = { sinf(a1) * r0, y0, -cosf(a1) * r0 };
            Vector3 p2 = { sinf(a1) * r1, y1, -cosf(a1) * r1 }, p3 = { sinf(a0) * r1, y1, -cosf(a0) * r1 };
            adicionarQuadrilatero(c, p3, p0, p1, p2, (Vector2){ u0, v1 }, (Vector2){ u0, v0 }, (Vector2){ u1, v0 }, (Vector2){ u1, v1 }, cor);
        }
    }
}

/*
 * construirCeuDeDia
 * O domo de dia: a imagem do céu até a linha de cima dela, que é uma cor só,
 * e daí até o alto uma tampa lisa dessa cor. Sem a tampa, a imagem se espremeria
 * em raios no ponto mais alto.
 * Recebe: a altura da imagem no mundo.
 * Devolve: nada (preenche ceuDeDia e tampaDoCeu).
 */
static void construirCeuDeDia(float alturaDaImagem) {
    float elevacaoDoTopo = atan2f((1.0f - COMECO_DA_IMAGEM_DO_CEU) * alturaDaImagem - ABAIXO_DA_CAMERA, RAIO_DO_CEU) * RAD2DEG;
    Construcao imagem = { 0 }, tampa = { 0 };
    adicionarAneisDoCeu(&imagem, ELEVACAO_DA_BASE_DO_CEU, elevacaoDoTopo, alturaDaImagem, WHITE);
    adicionarAneisDoCeu(&tampa, elevacaoDoTopo, 90.0f, 0.0f, COR_DO_TOPO_DO_CEU);
    ceuDeDia = terminarMalha(&imagem);
    tampaDoCeu = terminarMalha(&tampa);
}

/*
 * construirCeuDeNoite
 * Recebe: nada.
 * Devolve: o domo de noite, com as estrelas até o alto.
 */
static Mesh construirCeuDeNoite(void) {
    Construcao c = { 0 };
    adicionarAneisDoCeu(&c, ELEVACAO_DA_BASE_DO_CEU, 90.0f, 0.0f, WHITE);
    return terminarMalha(&c);
}

/* ==================================================================
 * Carregar e descarregar
 * ================================================================== */

/*
 * carregarTexturaRepetida
 * Recebe: o arquivo, a partir da pasta de texturas.
 * Devolve: a textura, repetindo e com mipmaps (para não tremer ao longe).
 */
static Texture2D carregarTexturaRepetida(const char *arquivo) {
    Texture2D textura = LoadTexture(TextFormat("%s%s", PASTA_TEXTURAS, arquivo));
    if (textura.id != 0) {
        SetTextureWrap(textura, TEXTURE_WRAP_REPEAT);
        GenTextureMipmaps(&textura);
        SetTextureFilter(textura, TEXTURE_FILTER_TRILINEAR);
    }
    return textura;
}

/*
 * construirRefletores
 * Põe o modelo 3D em cada refletor do alto do estádio, virado para o centro.
 * Recebe: nada.
 * Devolve: nada.
 */
static void construirRefletores(void) {
    const Vector3 *lugares;
    int total = refletoresDaNoite(&lugares);
    for (int i = 0; i < total; i++) {
        Vector3 pe = { lugares[i].x, lugares[i].y - ALTURA_DO_REFLETOR, lugares[i].z };
        colocarModelo(MODELO_REFLETOR, pe, atan2f(-pe.x, -pe.z), 1.0f);
    }
}

/*
 * lugarDoPit
 * Recebe: a pista, os metros a partir da linha, a distância até a linha da pista (para o
 *         lado do pit lane) e onde guardar o rumo (ou NULL).
 * Devolve: o lugar no mundo, no chão.
 */
static Vector3 lugarDoPit(Pista *pista, float metros, float aoLado, float *rumo) {
    PosicaoNaPista lugar = { pista->largada, 0.0f };
    moverNaPista(&lugar, metros * UNIDADES_POR_METRO);
    float largura = (pista->pit.lado < 0) ? lugar.segmento->larguraEsquerda : lugar.segmento->larguraDireita;
    PontoNoMundo ponto = pontoNoMundo(lugar, pista->pit.lado * aoLado / largura);
    if (rumo != NULL) {
        *rumo = ponto.rumo;
    }
    return (Vector3){ ponto.x, ponto.y, ponto.z };
}

/*
 * pintarNoPit
 * Um retângulo pintado no chão do pit lane.
 * Recebe: a construção, a pista, os metros onde começa e termina, as distâncias até a
 *         linha da pista (de perto e de longe) e a cor.
 * Devolve: nada.
 */
static void pintarNoPit(Construcao *c, Pista *pista, float metros0, float metros1, float perto, float longe, Color cor) {
    Vector2 zero = { 0.0f, 0.0f };
    Vector3 a = lugarDoPit(pista, metros0, perto, NULL), b = lugarDoPit(pista, metros0, longe, NULL);
    Vector3 d = lugarDoPit(pista, metros1, perto, NULL), e = lugarDoPit(pista, metros1, longe, NULL);
    a.y += ALTURA_DA_PINTURA;
    b.y += ALTURA_DA_PINTURA;
    d.y += ALTURA_DA_PINTURA;
    e.y += ALTURA_DA_PINTURA;
    if (pista->pit.lado < 0) {
        adicionarQuadrilatero(c, b, a, d, e, zero, zero, zero, zero, cor);
    } else {
        adicionarQuadrilatero(c, a, b, e, d, zero, zero, zero, zero, cor);
    }
}

/*
 * pintarVagas
 * As caixas das vagas e as duas linhas do limite de velocidade, nos pit lanes dos modelos.
 * Recebe: a construção e a pista.
 * Devolve: nada.
 */
static void pintarVagas(Construcao *c, Pista *pista) {
    const DadosDoPit *pit = &pista->pit;
    float metade = fminf(pit->entreVagas / 2.0f - 0.8f, METADE_DA_VAGA_PINTADA);
    for (int n = 0; n < LUGARES_NO_GRID; n++) {
        float meio = pit->primeiraVaga + n * pit->entreVagas;
        float perto = pit->vaga - LARGURA_DA_VAGA_PINTADA / 2.0f, longe = pit->vaga + LARGURA_DA_VAGA_PINTADA / 2.0f;
        pintarNoPit(c, pista, meio - metade, meio - metade + TRACO_DA_VAGA, perto, longe, COR_TINTA);
        pintarNoPit(c, pista, meio + metade - TRACO_DA_VAGA, meio + metade, perto, longe, COR_TINTA);
        pintarNoPit(c, pista, meio - metade, meio + metade, longe - TRACO_DA_VAGA, longe, COR_TINTA);
    }
    /* Se o limite vai até depois de ele deixar a faixa, a linha fica onde a faixa termina */
    float ateOFimDaFaixa = pit->noModelo ? metrosDesdeAEntrada(pista, pit->deixaAFaixa) : 1e9f;
    float metros[2] = { pit->inicioDoLimite,
                        (metrosDesdeAEntrada(pista, pit->fimDoLimite) > ateOFimDaFaixa) ? pit->deixaAFaixa : pit->fimDoLimite };
    for (int i = 0; i < 2; i++) {
        pintarNoPit(c, pista, metros[i], metros[i] + TRACO_DA_VAGA * 2.0f, pit->faixa - LARGURA_DA_LINHA_DO_LIMITE,
                    pit->vaga + LARGURA_DA_VAGA_PINTADA / 2.0f, COR_TINTA);
    }
}

/*
 * montarEquipeAoArLivre
 * Na Copa Pistão não há garagens: cada equipe arma a tenda atrás da vaga, no pátio,
 * com os pneus de reserva ao lado.
 * Recebe: a pista.
 * Devolve: nada.
 */
static void montarEquipeAoArLivre(Pista *pista) {
    const DadosDoPit *pit = &pista->pit;
    for (int n = 0; n < LUGARES_NO_GRID; n++) {
        float meio = pit->primeiraVaga + n * pit->entreVagas;
        float rumo = 0.0f;
        Vector3 tenda = lugarDoPit(pista, meio, pit->vaga + METROS_DA_VAGA_ATE_A_TENDA, &rumo);
        float giro = -rumo * DEG2RAD - pit->lado * PI / 2.0f;
        colocarNoCenario((ModeloDoCenario) (CENARIO_TENDA_1 + n % 3), tenda, giro, TAMANHO_DA_TENDA_DA_EQUIPE);
        Vector3 pneus = lugarDoPit(pista, meio + pit->entreVagas / 2.0f, pit->vaga + METROS_DA_VAGA_ATE_OS_PNEUS, NULL);
        colocarNoCenario((ModeloDoCenario) (CENARIO_PILHA_1 + n % 2), pneus, giro, 1.0f);
    }
}

/*
 * carregarEstadio
 * Carrega o modelo da Copa Pistão e deixa as texturas dele nítidas ao longe. A
 * noite ilumina o modelo inteiro, e os assentos viram a torcida.
 * Recebe: a pista (para pintar a largada e o grid).
 * Devolve: nada.
 */
static void carregarEstadio(Pista *pista) {
    pistaEmCarregamento = PISTA_COPA_PISTAO;
    primeiroLoteDoEstadio = totalDeLotes;
    estadio = LoadModel(ARQUIVO_DO_ESTADIO);
    carregarNoite(estadio, MATERIAL_DOS_ASSENTOS, ESCALA_COPA_PISTAO);
    carregarTorcidaDoEstadio(estadio, MATERIAL_DOS_ASSENTOS, ESCALA_COPA_PISTAO);
    for (int i = 0; i < estadio.materialCount; i++) {
        estadio.materials[i].shader = (i == MATERIAL_DOS_ASSENTOS) ? shaderDaTorcida() : shaderDaNoite();
        Texture2D *textura = &estadio.materials[i].maps[MATERIAL_MAP_ALBEDO].texture;
        if (textura->id != 0 && textura->id != rlGetTextureIdDefault()) {
            GenTextureMipmaps(textura);
            SetTextureFilter(*textura, TEXTURE_FILTER_TRILINEAR);
            SetTextureFilter(*textura, filtroDeTexturaDaConfig());
        }
    }
    Construcao pintura = { 0 };
    pintarLargadaEGrid(&pintura, pista);
    pintarVagas(&pintura, pista);
    pinturaDoEstadio = terminarMalha(&pintura);
    construirRefletores();
    montarEquipeAoArLivre(pista);
    fecharTodosOsLotes();
    fimDosLotesDoEstadio = totalDeLotes;
    carregarTelao(&estadio);
    /* As caixas das malhas do estádio, para o corte: a mesma matriz que o desenho usa.
     * As que receberam o shader da tela que brilha são o cubo e o placar: as telas daqui. */
    if (estadio.meshCount > 0) {
        Matrix noMundo = MatrixMultiply(estadio.transform,
                                        MatrixScale(ESCALA_COPA_PISTAO, ESCALA_COPA_PISTAO, ESCALA_COPA_PISTAO));
        caixasDoEstadio = malloc(estadio.meshCount * sizeof(BoundingBox));
        unsigned int shaderDaTela = shaderDaTelaQueBrilha().id;
        for (int i = 0; i < estadio.meshCount; i++) {
            BoundingBox caixa = caixaDaMalha(estadio.meshes[i], noMundo);
            if (caixasDoEstadio != NULL) {
                caixasDoEstadio[i] = caixa;
            }
            if (estadio.materials[estadio.meshMaterial[i]].shader.id == shaderDaTela) {
                registrarTela(caixa);
            }
        }
    }
}

/*
 * construirTorcida
 * Uma figura em cada lugar da arquibancada, virada para a pista. Usa o shader da torcida:
 * cada lugar ganha uma casa só dele na grade de assentos (veja PIXELS_DA_TORCIDA).
 * Recebe: os lugares, quantos são, a escala do modelo, o tamanho da figura e a pista.
 * Devolve: a malha.
 */
static Mesh construirTorcida(const LugarNaTorcida *lugares, int total, float escala, float tamanho, Pista *pista) {
    Construcao c = { 0 };
    for (int i = 0; i < total; i++) {
        Vector3 pe = { lugares[i].x * escala, lugares[i].y * escala, lugares[i].z * escala };
        float melhor = 1e12f, olhaX = 1.0f, olhaZ = 0.0f;
        Segmento *atual = pista->largada;
        do {
            float dx = atual->xMundo - pe.x, dz = atual->zMundo - pe.z, d = dx * dx + dz * dz;
            if (d < melhor) {
                melhor = d;
                olhaX = dx;
                olhaZ = dz;
            }
            for (int k = 0; k < PASSOS_PROCURANDO_A_PISTA && atual->proximo != pista->largada; k++) {
                atual = atual->proximo;
            }
            atual = atual->proximo;
        } while (atual != pista->largada);
        float tam = sqrtf(melhor);
        olhaX /= tam;
        olhaZ /= tam;
        float ladoX = olhaZ * tamanho / 2.0f, ladoZ = -olhaX * tamanho / 2.0f;
        Vector3 a = { pe.x - ladoX, pe.y, pe.z - ladoZ }, b = { pe.x + ladoX, pe.y, pe.z + ladoZ };
        Vector3 d = { b.x, b.y + tamanho * ALTURA_DA_FIGURA, b.z }, e = { a.x, a.y + tamanho * ALTURA_DA_FIGURA, a.z };
        int coluna = i % 28;   /* foge do corredor, que o shader desenha no meio da grade */
        int linha = (i / 28) % 52;
        Vector2 uv0 = { coluna / 60.0f, (linha + 1) / 52.0f }, uv1 = { (coluna + 1) / 60.0f, (linha + 1) / 52.0f };
        Vector2 uv2 = { (coluna + 1) / 60.0f, linha / 52.0f }, uv3 = { coluna / 60.0f, linha / 52.0f };
        adicionarQuadrilatero(&c, a, b, d, e, uv0, uv1, uv2, uv3, (Color){ 255, 255, 255, FIGURA_EM_PE });
        adicionarLugarTorcida3D(pista->id, pe.x, pe.y, pe.z, atan2f(olhaX, olhaZ));
    }
    definirTamanhoTorcida3D(pista->id, tamanho * CARRINHO_POR_FIGURA);
    return terminarMalha(&c);
}

/*
 * modeloDaCoisa
 * Recebe: o tipo de objeto do mapa.
 * Devolve: o modelo 3D dele.
 */
static ModeloDeObjeto modeloDaCoisa(TipoDeCoisa tipo) {
    if (tipo <= COISA_ARVORE_3) {
        return (ModeloDeObjeto) (MODELO_ARVORE_1 + (tipo - COISA_ARVORE_1));
    }
    if (tipo <= COISA_PINHEIRO_2) {
        return (ModeloDeObjeto) (MODELO_PINHEIRO_1 + (tipo - COISA_PINHEIRO_1));
    }
    if (tipo <= COISA_COPA_3) {
        return (ModeloDeObjeto) (MODELO_COPA_1 + (tipo - COISA_COPA_1));
    }
    int cenario = CENARIO_POSTO_FISCAL;
    if (tipo <= COISA_CAMINHAO_4) {
        cenario = CENARIO_CAMINHAO_1 + (tipo - COISA_CAMINHAO_1);
    } else if (tipo <= COISA_TENDA_3) {
        cenario = CENARIO_TENDA_1 + (tipo - COISA_TENDA_1);
    } else if (tipo <= COISA_CARRO_6) {
        cenario = CENARIO_CARRO_1 + (tipo - COISA_CARRO_1);
    } else if (tipo <= COISA_MASTRO_5) {
        cenario = CENARIO_MASTRO_1 + (tipo - COISA_MASTRO_1);
    } else if (tipo <= COISA_PILHA_2) {
        cenario = CENARIO_PILHA_1 + (tipo - COISA_PILHA_1);
    } else {
        switch (tipo) {
            case COISA_PNEUS_VERMELHO: return MODELO_PNEUS_VERMELHO;
            case COISA_PNEUS_BRANCO: return MODELO_PNEUS_BRANCO;
            case COISA_TORRE_TV: cenario = CENARIO_TORRE_TV; break;
            case COISA_TELAO: return MODELO_TELAO_KATCHAU;
            case COISA_ARQUIBANCADA: return MODELO_ARQUIBANCADA;
            case COISA_CAMAROTE: return MODELO_CAMAROTE;
            case COISA_POSTE: return MODELO_POSTE_RUA;
            case COISA_PORTICO: return MODELO_PORTICO;
            default: break;
        }
    }
    return (ModeloDeObjeto) (MODELO_PRIMEIRO_DO_CENARIO + cenario);
}

/*
 * carregarMapaDeDia
 * Carrega o modelo 3D de uma pista de dia e constrói o que é do jogo por cima dele.
 * Recebe: a pista, o arquivo, a escala, os lugares da torcida, quantos são, o tamanho da
 *         figura, e as árvores e estruturas do mapa e quantas são.
 * Devolve: nada.
 */
static void carregarMapaDeDia(Pista *pista, const char *arquivo, float escala, const LugarNaTorcida *lugares,
                              int totalDeLugares, float tamanho, const CoisaNoMapa *coisas, int totalDeCoisas) {
    MapaDeDia *mapa = &mapasDeDia[pista->id];
    pistaEmCarregamento = pista->id;   /* para registrarTela saber de quem são os telões */
    mapa->existe = 1;
    mapa->escala = escala;
    mapa->modelo = LoadModel(arquivo);
    if (pista->id == PISTA_INTERLAGOS) {
        tirarParedesInvisiveis(&mapa->modelo);
        texturaDoConcreto = gerarConcretoLimpo();
        trocarImagensGastas(&mapa->modelo);
        mapa->modelo.materials[MATERIAL_DO_LAGO].maps[MATERIAL_MAP_ALBEDO].color = COR_DA_AGUA_DO_LAGO;
    }
    for (int i = 0; i < mapa->modelo.materialCount; i++) {
        mapa->modelo.materials[i].shader = shaderDosMapasDeDia;
        if (pista->id == PISTA_INTERLAGOS && i < TOTAL_MATERIAIS_INTERLAGOS) {
            prepararMaterialDeInterlagos(&mapa->modelo.materials[i], MATERIAIS_INTERLAGOS[i].desenho);
        }
        Texture2D *textura = &mapa->modelo.materials[i].maps[MATERIAL_MAP_ALBEDO].texture;
        if (textura->id != 0 && textura->id != rlGetTextureIdDefault()) {
            GenTextureMipmaps(textura);
            SetTextureFilter(*textura, TEXTURE_FILTER_TRILINEAR);
            SetTextureFilter(*textura, filtroDeTexturaDaConfig());
            SetTextureWrap(*textura, TEXTURE_WRAP_REPEAT);
        }
    }
    Construcao pintura = { 0 };
    if (!pista->largadaPintadaNoModelo) {
        pintarLargadaEGrid(&pintura, pista);
    }
    pintarVagas(&pintura, pista);
    mapa->pintura = terminarMalha(&pintura);
    LugarNaTorcida *maisLugares = NULL;
    if (pista->id == PISTA_INTERLAGOS) {
        definirAreaDoAutodromo(pista);
        totalDeLugares = construirArquibancadasQueFaltam(pista, mapa, lugares, totalDeLugares, &maisLugares);
        lugares = maisLugares;
    }
    mapa->torcida = construirTorcida(lugares, totalDeLugares, escala, tamanho, pista);
    free(maisLugares);

    mapa->primeiroLote = totalDeLotes;
    for (int i = 0; i < totalDeCoisas; i++) {
        Vector3 pe = { coisas[i].x, coisas[i].y, coisas[i].z };
        colocarModelo(modeloDaCoisa(coisas[i].tipo), pe, coisas[i].giro, coisas[i].tamanho);
    }
    if (pista->id == PISTA_INTERLAGOS) {
        colocarTeloesDeInterlagos(pista, &mapa->modelo);
        mapa->saia = construirSaiaDoTerreno(&mapa->modelo, escala, pista);
    }
    fecharTodosOsLotes();
    mapa->fimDosLotes = totalDeLotes;
    prepararCaixasDoMapa(mapa, pista->id);   /* depois das arquibancadas: elas acrescentam malhas ao modelo */
    TraceLog(LOG_INFO, "CORTE: pista %d com %d tela(s) de telão", pista->id, totalDeTelas[pista->id]);
}

/* ==================================================================
 * Interlagos
 * ================================================================== */

/*
 * prepararMaterialDeInterlagos
 * Põe em cada material do modelo o shader do jeito dele ser desenhado. O asfalto e a
 * grama trocam a imagem do modelo pela do jogo; a tinta sobre o asfalto leva o asfalto
 * do jogo na textura 1.
 * Recebe: o material e como ele é desenhado.
 * Devolve: nada.
 */
static void prepararMaterialDeInterlagos(Material *material, DesenhoDoMaterial desenho) {
    if (desenho == DESENHO_ESCONDIDO) {
        return;
    }
    material->shader = shadersDeInterlagos[desenho];
    if (desenho == DESENHO_ASFALTO) {
        material->maps[MATERIAL_MAP_ALBEDO].texture = texturaAsfalto[0];
    } else if (desenho == DESENHO_GRAMA) {
        material->maps[MATERIAL_MAP_ALBEDO].texture = texturaGrama;
    } else if (desenho == DESENHO_DECALQUE) {
        material->maps[MATERIAL_MAP_METALNESS].texture = texturaAsfalto[0];
    } else if (desenho == DESENHO_TERRENO) {
        material->maps[MATERIAL_MAP_METALNESS].texture = texturaGrama;
    }
}

/*
 * tirarParedesInvisiveis
 * O muro de concreto do modelo traz as paredes invisíveis do jogo original (triângulos de
 * dezenas de metros de altura): elas viram triângulos vazios, que a placa não desenha.
 * Recebe: o modelo de Interlagos.
 * Devolve: nada.
 */
static void tirarParedesInvisiveis(Model *modelo) {
    for (int m = 0; m < modelo->meshCount; m++) {
        Mesh *malha = &modelo->meshes[m];
        if (modelo->meshMaterial[m] != MATERIAL_COM_PAREDES_INVISIVEIS || malha->indices == NULL) {
            continue;
        }
        int tiradas = 0;
        for (int t = 0; t < malha->triangleCount; t++) {
            unsigned short *tri = &malha->indices[t * 3];
            float menor = 1e9f, maior = -1e9f;
            for (int k = 0; k < 3; k++) {
                float y = malha->vertices[tri[k] * 3 + 1];
                menor = fminf(menor, y);
                maior = fmaxf(maior, y);
            }
            if (maior - menor > ALTURA_DA_PAREDE_INVISIVEL) {
                tri[1] = tri[0];
                tri[2] = tri[0];
                tiradas++;
            }
        }
        UpdateMeshBuffer(*malha, 6, malha->indices, malha->triangleCount * 3 * sizeof(unsigned short), 0);
        TraceLog(LOG_INFO, "MUNDO: %d triangulos de paredes invisiveis tirados de Interlagos", tiradas);
    }
}

/*
 * gerarConcretoLimpo
 * Um painel de concreto claro, com uma variação leve, as juntas entre os painéis e o pé um
 * pouco mais escuro (a imagem repete ao longo do muro; v vai de baixo para cima).
 * Recebe: nada.
 * Devolve: a textura.
 */
static Texture2D gerarConcretoLimpo(void) {
    Image imagem = GenImageColor(LARGURA_DO_CONCRETO, ALTURA_DO_CONCRETO, (Color){ 0, 0, 0, 255 });
    Color *pixels = (Color *) imagem.data;
    unsigned int semente = 2026u;
    for (int y = 0; y < ALTURA_DO_CONCRETO; y++) {
        for (int x = 0; x < LARGURA_DO_CONCRETO; x++) {
            semente = semente * 1103515245u + 12345u;
            float grao = ((semente >> 16) & 0xFF) / 255.0f - 0.5f;
            float tom = 212.0f + grao * 10.0f;
            if (x % (LARGURA_DO_CONCRETO / 2) < 2) {
                tom -= 38.0f;   /* a junta entre dois painéis */
            }
            float alturaNaImagem = 1.0f - (float) y / ALTURA_DO_CONCRETO;   /* a linha de cima da imagem é o alto do muro */
            if (alturaNaImagem < 0.12f) {
                tom -= 30.0f * (1.0f - alturaNaImagem / 0.12f);   /* o pé, mais escuro */
            }
            if (alturaNaImagem > 0.94f) {
                tom += 12.0f;   /* o acabamento do alto */
            }
            unsigned char c = (unsigned char) fminf(fmaxf(tom, 0.0f), 255.0f);
            pixels[y * LARGURA_DO_CONCRETO + x] = (Color){ c, c, (unsigned char) fmaxf(c - 4, 0), 255 };
        }
    }
    Texture2D textura = LoadTextureFromImage(imagem);
    UnloadImage(imagem);
    GenTextureMipmaps(&textura);
    SetTextureFilter(textura, TEXTURE_FILTER_TRILINEAR);
    SetTextureWrap(textura, TEXTURE_WRAP_REPEAT);
    return textura;
}

/*
 * trocarImagensGastas
 * O concreto manchado, o enferrujado e o com musgo viram o concreto limpo; o guard-rail enferrujado vira o novo.
 * Recebe: o modelo de Interlagos.
 * Devolve: nada.
 */
static void trocarImagensGastas(Model *modelo) {
    int concretos[3] = { MATERIAL_DO_CONCRETO_MANCHADO, MATERIAL_DO_CONCRETO_ENFERRUJADO, MATERIAL_DO_CONCRETO_COM_MUSGO };
    for (int i = 0; i < 3; i++) {
        Texture2D *imagem = &modelo->materials[concretos[i]].maps[MATERIAL_MAP_ALBEDO].texture;
        UnloadTexture(*imagem);
        *imagem = texturaDoConcreto;
    }
    Texture2D *enferrujado = &modelo->materials[MATERIAL_DO_GUARD_RAIL_ENFERRUJADO].maps[MATERIAL_MAP_ALBEDO].texture;
    UnloadTexture(*enferrujado);
    *enferrujado = modelo->materials[MATERIAL_DO_GUARD_RAIL_NOVO].maps[MATERIAL_MAP_ALBEDO].texture;
}

/*
 * carregarShadersDeInterlagos
 * Recebe: nada.
 * Devolve: nada (preenche shadersDeInterlagos e os põe entre os que recebem sombra).
 */
static void carregarShadersDeInterlagos(void) {
    for (int d = 0; d < TOTAL_DE_DESENHOS; d++) {
        if (PIXELS_DE_INTERLAGOS[d] == NULL) {
            continue;
        }
        shadersDeInterlagos[d] = LoadShaderFromMemory(VERTICES_DOS_MAPAS_DE_DIA, PIXELS_DE_INTERLAGOS[d]);
        float ladrilho = (d == DESENHO_GRAMA || d == DESENHO_TERRENO) ? LADRILHO_DA_GRAMA : LADRILHO_DO_ASFALTO;
        SetShaderValue(shadersDeInterlagos[d], GetShaderLocation(shadersDeInterlagos[d], "metrosDoLadrilho"), &ladrilho,
                       SHADER_UNIFORM_FLOAT);
        recebemSombra[totalQueRecebemSombra++] = shadersDeInterlagos[d];
        registrarShaderDoDia(shadersDeInterlagos[d]);
    }
}

/*
 * desenhoDaMalha
 * Recebe: o modelo de Interlagos e uma malha dele.
 * Devolve: como ela é desenhada.
 */
static DesenhoDoMaterial desenhoDaMalha(const Model *modelo, int malha) {
    int qual = modelo->meshMaterial[malha];
    return (qual < TOTAL_MATERIAIS_INTERLAGOS) ? MATERIAIS_INTERLAGOS[qual].desenho : DESENHO_OPACO;
}

/*
 * prepararCaixasDoMapa
 * Mede cada malha do modelo da pista uma vez, no carregamento: a caixa que a cerca, já em
 * coordenadas do mundo, para o corte pelo campo de visão (corte.c) poder descartá-la sem
 * olhar os vértices. Em Interlagos, também separa as malhas por tipo de desenho.
 * A matriz é a mesma que o desenho usa, então a caixa cai exatamente onde a malha aparece.
 * Chamar depois de todas as malhas estarem no modelo.
 * Recebe: o mapa e a pista.
 * Devolve: nada.
 */
static void prepararCaixasDoMapa(MapaDeDia *mapa, IdPista id) {
    int total = mapa->modelo.meshCount;
    if (total <= 0) {
        return;
    }
    Matrix noMundo = MatrixMultiply(mapa->modelo.transform, MatrixScale(mapa->escala, mapa->escala, mapa->escala));
    mapa->caixas = malloc(total * sizeof(BoundingBox));
    if (mapa->caixas == NULL) {
        return;   /* sem as caixas, o desenho segue sem cortar nada */
    }
    for (int i = 0; i < total; i++) {
        mapa->caixas[i] = caixaDaMalha(mapa->modelo.meshes[i], noMundo);
    }
    if (id != PISTA_INTERLAGOS) {
        return;   /* as outras pistas são desenhadas num laço só, sem separar por tipo */
    }
    mapa->opacas = malloc(total * sizeof(int));
    mapa->deSombra = malloc(total * sizeof(int));
    mapa->deTela = malloc(total * sizeof(int));
    if (mapa->opacas == NULL || mapa->deSombra == NULL || mapa->deTela == NULL) {
        free(mapa->opacas);
        free(mapa->deSombra);
        free(mapa->deTela);
        mapa->opacas = mapa->deSombra = mapa->deTela = NULL;
        return;
    }
    for (int i = 0; i < total; i++) {
        DesenhoDoMaterial desenho = desenhoDaMalha(&mapa->modelo, i);
        if (desenho == DESENHO_SOMBRA) {
            mapa->deSombra[mapa->totalDeSombra++] = i;
        } else if (desenho == DESENHO_TELA) {
            mapa->deTela[mapa->totalDeTela++] = i;
        } else if (desenho != DESENHO_ESCONDIDO) {
            mapa->opacas[mapa->totalOpacas++] = i;
        }
    }
    TraceLog(LOG_INFO, "CORTE: Interlagos tem %d malhas (%d opacas, %d de sombra, %d de tela)",
             total, mapa->totalOpacas, mapa->totalDeSombra, mapa->totalDeTela);
}

/*
 * desenharModeloCortado
 * O modelo inteiro de uma pista, malha por malha, deixando de fora as que não podem
 * aparecer na tela. É o que o DrawModel faz (ele também chama DrawMesh por malha, com a
 * matriz do modelo vezes a escala), menos as malhas fora do campo de visão.
 * Recebe: o mapa.
 * Devolve: nada.
 */
static void desenharModeloCortado(MapaDeDia *mapa) {
    if (mapa->caixas == NULL) {
        DrawModel(mapa->modelo, (Vector3){ 0.0f, 0.0f, 0.0f }, mapa->escala, WHITE);
        return;
    }
    Matrix noMundo = MatrixMultiply(mapa->modelo.transform, MatrixScale(mapa->escala, mapa->escala, mapa->escala));
    for (int i = 0; i < mapa->modelo.meshCount; i++) {
        if (!caixaAVista(mapa->caixas[i])) {
            continue;
        }
        DrawMesh(mapa->modelo.meshes[i], mapa->modelo.materials[mapa->modelo.meshMaterial[i]], noMundo);
    }
}

/*
 * desenharModeloDeInterlagos
 * Primeiro tudo o que é sólido; depois, sem escrever na profundidade, o que só escurece
 * (a borracha do traçado em cima do asfalto) e os alambrados, transparentes. As faces valem dos dois lados, como no
 * jogo de onde o modelo veio.
 * Recebe: o mapa.
 * Devolve: nada.
 */
static void desenharMalhasDoMapa(MapaDeDia *mapa, const int *quais, int quantas, Matrix noMundo) {
    for (int k = 0; k < quantas; k++) {
        int i = quais[k];
        if (mapa->caixas != NULL && !caixaAVista(mapa->caixas[i])) {
            continue;   /* fora da tela: nem vale a troca de material e de shader */
        }
        DrawMesh(mapa->modelo.meshes[i], mapa->modelo.materials[mapa->modelo.meshMaterial[i]], noMundo);
    }
}

static void desenharModeloDeInterlagos(MapaDeDia *mapa) {
    Matrix noMundo = MatrixMultiply(mapa->modelo.transform, MatrixScale(mapa->escala, mapa->escala, mapa->escala));
    rlDisableBackfaceCulling();
    if (mapa->opacas == NULL) {   /* sem as listas prontas: o modelo inteiro, como antes */
        for (int i = 0; i < mapa->modelo.meshCount; i++) {
            DesenhoDoMaterial desenho = desenhoDaMalha(&mapa->modelo, i);
            if (desenho != DESENHO_SOMBRA && desenho != DESENHO_TELA && desenho != DESENHO_ESCONDIDO) {
                DrawMesh(mapa->modelo.meshes[i], mapa->modelo.materials[mapa->modelo.meshMaterial[i]], noMundo);
            }
        }
        rlDrawRenderBatchActive();
        rlDisableDepthMask();
        DesenhoDoMaterial transparentes[2] = { DESENHO_SOMBRA, DESENHO_TELA };
        for (int vez = 0; vez < 2; vez++) {
            for (int i = 0; i < mapa->modelo.meshCount; i++) {
                if (desenhoDaMalha(&mapa->modelo, i) == transparentes[vez]) {
                    DrawMesh(mapa->modelo.meshes[i], mapa->modelo.materials[mapa->modelo.meshMaterial[i]], noMundo);
                }
            }
        }
    } else {
        desenharMalhasDoMapa(mapa, mapa->opacas, mapa->totalOpacas, noMundo);
        rlDrawRenderBatchActive();
        rlDisableDepthMask();
        desenharMalhasDoMapa(mapa, mapa->deSombra, mapa->totalDeSombra, noMundo);
        desenharMalhasDoMapa(mapa, mapa->deTela, mapa->totalDeTela, noMundo);
    }
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
    rlEnableBackfaceCulling();
}

/*
 * desenharSombraDeInterlagos
 * O que faz sombra: só o que fica de pé (o chão não faz sombra em si mesmo). Onde o alfa
 * da imagem não é recorte, a peça é inteira: vai com a imagem branca do padrão.
 * Recebe: o mapa.
 * Devolve: nada.
 */
static void desenharSombraDeInterlagos(MapaDeDia *mapa) {
    Matrix noMundo = MatrixMultiply(mapa->modelo.transform, MatrixScale(mapa->escala, mapa->escala, mapa->escala));
    Texture2D branca = { rlGetTextureIdDefault(), 1, 1, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };
    for (int i = 0; i < mapa->modelo.meshCount; i++) {
        DesenhoDoMaterial desenho = desenhoDaMalha(&mapa->modelo, i);
        if (desenho != DESENHO_OPACO && desenho != DESENHO_RECORTE) {
            continue;
        }
        if (mapa->caixas != NULL && !caixaAVista(mapa->caixas[i])) {
            continue;   /* fora do quadrado da luz: não escreve no mapa de sombra */
        }
        int qual = mapa->modelo.meshMaterial[i];
        if (qual < TOTAL_MATERIAIS_INTERLAGOS && MATERIAIS_INTERLAGOS[qual].chao != CHAO_NENHUM
            && MATERIAIS_INTERLAGOS[qual].chao != CHAO_MURO) {
            continue;
        }
        /* As imagens do material ficam num vetor apontado por ele: a troca é feita e desfeita no original */
        Material *original = &mapa->modelo.materials[qual];
        Shader shaderDele = original->shader;
        Texture2D imagemDele = original->maps[MATERIAL_MAP_ALBEDO].texture;
        original->shader = shaderDaProfundidade;
        if (desenho == DESENHO_OPACO) {
            original->maps[MATERIAL_MAP_ALBEDO].texture = branca;
        }
        DrawMesh(mapa->modelo.meshes[i], *original, noMundo);
        original->shader = shaderDele;
        original->maps[MATERIAL_MAP_ALBEDO].texture = imagemDele;
    }
}

/*
 * caixaDaMalhaEmCache
 * A caixa de uma malha do modelo, medida uma vez só: GetMeshBoundingBox percorre todos os vértices, e
 * as perguntas de chão e altura são feitas centenas de vezes (uma por medida das arquibancadas e dos
 * telões), cada uma para todas as malhas do modelo. O cache vale enquanto o vetor de malhas for o mesmo
 * (acrescentarMalhasAoModelo o troca, e o cache é refeito).
 * Recebe: o modelo e o número da malha.
 * Devolve: a caixa (nas medidas do modelo).
 */
static BoundingBox caixaDaMalhaEmCache(const Model *modelo, int m) {
    static const Mesh *deQuem = NULL;
    static int quantas = 0;
    static BoundingBox *caixas = NULL;
    static unsigned char *prontas = NULL;
    if (deQuem != modelo->meshes || quantas != modelo->meshCount) {
        free(caixas);
        free(prontas);
        deQuem = modelo->meshes;
        quantas = modelo->meshCount;
        caixas = malloc(sizeof(BoundingBox) * (size_t) (quantas > 0 ? quantas : 1));
        prontas = calloc((size_t) (quantas > 0 ? quantas : 1), 1);
    }
    if (caixas == NULL || prontas == NULL) {
        return GetMeshBoundingBox(modelo->meshes[m]);
    }
    if (!prontas[m]) {
        caixas[m] = GetMeshBoundingBox(modelo->meshes[m]);
        prontas[m] = 1;
    }
    return caixas[m];
}

/*
 * alturaNoModelo
 * O ponto mais alto do modelo embaixo de (x, z): o chão, ou o telhado do que estiver ali.
 * Árvores e o que é recortado (alambrados, folhas) não contam.
 * Recebe: o modelo e o ponto.
 * Devolve: a altura, ou -1e9 se não há nada.
 */
static float alturaNoModelo(const Model *modelo, float x, float z) {
    float maior = -1e9f;
    for (int m = 0; m < modelo->meshCount; m++) {
        DesenhoDoMaterial desenho = desenhoDaMalha(modelo, m);
        if (desenho != DESENHO_OPACO && desenho != DESENHO_ASFALTO && desenho != DESENHO_GRAMA
            && desenho != DESENHO_DECALQUE && desenho != DESENHO_TERRENO) {
            continue;
        }
        const Mesh *malha = &modelo->meshes[m];
        BoundingBox caixa = caixaDaMalhaEmCache(modelo, m);
        if (x < caixa.min.x || x > caixa.max.x || z < caixa.min.z || z > caixa.max.z) {
            continue;
        }
        for (int t = 0; t < malha->triangleCount; t++) {
            Vector3 v[3];
            for (int k = 0; k < 3; k++) {
                int i = (malha->indices != NULL) ? malha->indices[t * 3 + k] : t * 3 + k;
                v[k] = (Vector3){ malha->vertices[i * 3], malha->vertices[i * 3 + 1], malha->vertices[i * 3 + 2] };
            }
            float d = (v[1].z - v[2].z) * (v[0].x - v[2].x) + (v[2].x - v[1].x) * (v[0].z - v[2].z);
            if (fabsf(d) < 1e-9f) {
                continue;
            }
            float a = ((v[1].z - v[2].z) * (x - v[2].x) + (v[2].x - v[1].x) * (z - v[2].z)) / d;
            float b = ((v[2].z - v[0].z) * (x - v[2].x) + (v[0].x - v[2].x) * (z - v[2].z)) / d;
            if (a < 0.0f || b < 0.0f || a + b > 1.0f) {
                continue;
            }
            maior = fmaxf(maior, a * v[0].y + b * v[1].y + (1.0f - a - b) * v[2].y);
        }
    }
    return maior;
}

/* Os telões de Interlagos: os quatro do lado dos boxes olham para as arquibancadas da reta e da
 * Subida, do outro lado da pista; o do fim da reta olha para quem vem chegando no S do Senna */
typedef struct {
    float metros, lateral;            /* onde fica: metros desde a linha e metros da linha da pista */
    float olharMetros, olharLateral;  /* para onde a tela olha */
    float tamanho;
} TelaoDeInterlagos;

static const TelaoDeInterlagos TELOES_DE_INTERLAGOS[] = {
    { 3790.0f, -42.0f, 3800.0f,  24.0f, 1.0f },
    { 4060.0f, -44.0f, 4040.0f,  22.0f, 1.0f },
    {   95.0f, -46.0f,   95.0f,  27.0f, 1.0f },
    {  215.0f, -46.0f,  200.0f,  28.0f, 1.0f },
    {  300.0f,  34.0f,  130.0f,   0.0f, 1.0f },
};

/*
 * definirAreaDoAutodromo
 * O retângulo que contém a pista, com uma folga: dentro dele o terreno é a grama do jogo.
 * Recebe: a pista.
 * Devolve: nada.
 */
static void definirAreaDoAutodromo(Pista *pista) {
    float area[4] = { 1e9f, 1e9f, -1e9f, -1e9f };
    Segmento *atual = pista->largada;
    do {
        area[0] = fminf(area[0], atual->xMundo);
        area[1] = fminf(area[1], atual->zMundo);
        area[2] = fmaxf(area[2], atual->xMundo);
        area[3] = fmaxf(area[3], atual->zMundo);
        atual = atual->proximo;
    } while (atual != pista->largada);
    float folga = FOLGA_DA_AREA_DO_AUTODROMO * ESCALA_INTERLAGOS, transicao = TRANSICAO_DO_AUTODROMO * ESCALA_INTERLAGOS;
    area[0] -= folga;
    area[1] -= folga;
    area[2] += folga;
    area[3] += folga;
    Shader terreno = shadersDeInterlagos[DESENHO_TERRENO];
    SetShaderValue(terreno, GetShaderLocation(terreno, "areaDoAutodromo"), area, SHADER_UNIFORM_VEC4);
    SetShaderValue(terreno, GetShaderLocation(terreno, "transicaoDoAutodromo"), &transicao, SHADER_UNIFORM_FLOAT);
}

/* ---------- As arquibancadas que o modelo de Interlagos não tem ----------
 * O modelo só traz as da reta e da Subida (setores A, B e M). As do mapa oficial do GP por fora do
 * S do Senna, da Curva do Sol, da Reta Oposta e da Descida do Lago (D, H, N, R, G, S e V) são
 * cópias da arquibancada da reta: um vão dela (do meio de um vão entre pilares ao meio do seguinte,
 * com o concreto, as fileiras, as escadas, o alambrado e a cobertura de treliça, com os materiais e
 * as imagens do próprio modelo) repetido ao longo do trecho, acompanhando a pista. As cópias entram
 * no modelo como malhas a mais: são desenhadas, fazem e recebem sombra como o resto dele. */
typedef struct {
    float inicio, fim;   /* metros do modelo a partir da linha de chegada */
} ArquibancadaQueFalta;

static const ArquibancadaQueFalta ARQUIBANCADAS_QUE_FALTAM[] = {
    {  290.0f,  430.0f },   /* D: por fora do S do Senna */
    {  480.0f,  600.0f },   /* H: Curva do Sol */
    {  625.0f,  685.0f },   /* N */
    {  705.0f,  850.0f },   /* R: saída da Curva do Sol */
    {  875.0f, 1255.0f },   /* G: Reta Oposta */
    { 1280.0f, 1330.0f },   /* S: fim da Reta Oposta */
    { 1372.0f, 1428.0f },   /* V: Descida do Lago */
};

/* Um vértice do módulo, nos eixos dele: (fora, cima, ao longo), em metros do modelo, com a frente
 * em fora = 0, o chão em cima = 0 e a seção de 0 a MODULO_COMPRIMENTO */
typedef struct {
    float p[3], n[3], uv[2];
} VerticeDoModulo;

/* Os triângulos do módulo de um material */
typedef struct {
    int material;
    int triangulos, capacidade;
    VerticeDoModulo *vertices;   /* 3 por triângulo */
} PecaDoModulo;

#define MAXIMO_DE_PECAS_DO_MODULO 64

static void guardarTrianguloDoModulo(PecaDoModulo *peca, VerticeDoModulo a, VerticeDoModulo b, VerticeDoModulo c) {
    if (peca->triangulos == peca->capacidade) {
        peca->capacidade = peca->capacidade ? peca->capacidade * 2 : 256;
        peca->vertices = realloc(peca->vertices, sizeof(VerticeDoModulo) * 3 * (size_t) peca->capacidade);
    }
    peca->vertices[peca->triangulos * 3] = a;
    peca->vertices[peca->triangulos * 3 + 1] = b;
    peca->vertices[peca->triangulos * 3 + 2] = c;
    peca->triangulos++;
}

static VerticeDoModulo entreVertices(VerticeDoModulo a, VerticeDoModulo b, float t) {
    VerticeDoModulo v;
    for (int i = 0; i < 3; i++) {
        v.p[i] = a.p[i] + (b.p[i] - a.p[i]) * t;
        v.n[i] = a.n[i] + (b.n[i] - a.n[i]) * t;
    }
    v.uv[0] = a.uv[0] + (b.uv[0] - a.uv[0]) * t;
    v.uv[1] = a.uv[1] + (b.uv[1] - a.uv[1]) * t;
    return v;
}

/*
 * cortarPoligono
 * Corta um polígono plano por um plano "ao longo = limite", ficando com o lado pedido.
 * Recebe: os vértices, quantos são, o limite, o lado (+1: ao longo >= limite; -1: <=) e onde guardar.
 * Devolve: quantos vértices ficaram.
 */
static int cortarPoligono(const VerticeDoModulo *dentro, int quantos, float limite, float lado, VerticeDoModulo *fora) {
    int n = 0;
    for (int i = 0; i < quantos; i++) {
        VerticeDoModulo a = dentro[i], b = dentro[(i + 1) % quantos];
        float da = (a.p[2] - limite) * lado, db = (b.p[2] - limite) * lado;
        if (da >= 0.0f) {
            fora[n++] = a;
        }
        if ((da >= 0.0f) != (db >= 0.0f)) {
            fora[n++] = entreVertices(a, b, da / (da - db));
        }
    }
    return n;
}

/*
 * recortarModuloDaArquibancada
 * Copia do modelo a seção da arquibancada da reta, separada por material e cortada em fatias ao
 * longo (os triângulos compridos, como os da cobertura e dos trilhos, são cortados nas bordas da
 * seção e das fatias). Fica só o que é construção: nada de chão, asfalto ou grama.
 * Recebe: o modelo, onde guardar as peças e quantas cabem.
 * Devolve: quantas peças (materiais) o módulo tem.
 */
static int recortarModuloDaArquibancada(const Model *modelo, PecaDoModulo *pecas, int maximo) {
    int total = 0;
    int malhasDoModelo = modelo->meshCount;
    for (int m = 0; m < malhasDoModelo; m++) {
        int qual = modelo->meshMaterial[m];
        if (qual >= TOTAL_MATERIAIS_INTERLAGOS || MATERIAIS_INTERLAGOS[qual].desenho == DESENHO_ESCONDIDO
            || (MATERIAIS_INTERLAGOS[qual].chao != CHAO_NENHUM && MATERIAIS_INTERLAGOS[qual].chao != CHAO_MURO)) {
            continue;
        }
        const Mesh *malha = &modelo->meshes[m];
        BoundingBox caixa = caixaDaMalhaEmCache(modelo, m);
        if (caixa.min.x > MODULO_FRENTE_X || caixa.max.x < MODULO_FUNDO_X || caixa.max.z < MODULO_INICIO_Z
            || caixa.min.z > MODULO_INICIO_Z + MODULO_COMPRIMENTO || malha->normals == NULL) {
            continue;
        }
        PecaDoModulo *peca = NULL;
        for (int t = 0; t < malha->triangleCount; t++) {
            VerticeDoModulo tri[3];
            float menorX = 1e9f, maiorX = -1e9f, menorY = 1e9f, menorZ = 1e9f, maiorZ = -1e9f;
            for (int k = 0; k < 3; k++) {
                int i = (malha->indices != NULL) ? malha->indices[t * 3 + k] : t * 3 + k;
                float x = malha->vertices[i * 3], y = malha->vertices[i * 3 + 1], z = malha->vertices[i * 3 + 2];
                menorX = fminf(menorX, x);
                maiorX = fmaxf(maiorX, x);
                menorY = fminf(menorY, y);
                menorZ = fminf(menorZ, z);
                maiorZ = fmaxf(maiorZ, z);
                /* Para fora é -x; com o "para cima", isso faz o "ao longo" ir para +z (a mesma mão) */
                tri[k].p[0] = MODULO_FRENTE_X - x;
                tri[k].p[1] = y - MODULO_CHAO_Y;
                tri[k].p[2] = z - MODULO_INICIO_Z;
                tri[k].n[0] = -malha->normals[i * 3];
                tri[k].n[1] = malha->normals[i * 3 + 1];
                tri[k].n[2] = malha->normals[i * 3 + 2];
                tri[k].uv[0] = (malha->texcoords != NULL) ? malha->texcoords[i * 2] : 0.0f;
                tri[k].uv[1] = (malha->texcoords != NULL) ? malha->texcoords[i * 2 + 1] : 0.0f;
            }
            if (maiorX > MODULO_FRENTE_X || menorX < MODULO_FUNDO_X || menorY < MODULO_CHAO_Y - MODULO_ABAIXO_DO_CHAO
                || maiorZ <= MODULO_INICIO_Z || menorZ >= MODULO_INICIO_Z + MODULO_COMPRIMENTO) {
                continue;
            }
            if (peca == NULL) {
                for (int p = 0; p < total && peca == NULL; p++) {
                    if (pecas[p].material == qual) {
                        peca = &pecas[p];
                    }
                }
                if (peca == NULL) {
                    if (total == maximo) {
                        break;
                    }
                    peca = &pecas[total++];
                    memset(peca, 0, sizeof(*peca));
                    peca->material = qual;
                }
            }
            for (int f = 0; f < FATIAS_DO_MODULO; f++) {
                float de = MODULO_COMPRIMENTO * f / FATIAS_DO_MODULO, ate = MODULO_COMPRIMENTO * (f + 1) / FATIAS_DO_MODULO;
                if (maiorZ - MODULO_INICIO_Z <= de || menorZ - MODULO_INICIO_Z >= ate) {
                    continue;
                }
                VerticeDoModulo meio[8], cortado[8];
                int n = cortarPoligono(tri, 3, de, 1.0f, meio);
                n = cortarPoligono(meio, n, ate, -1.0f, cortado);
                for (int k = 1; k + 1 < n; k++) {
                    guardarTrianguloDoModulo(peca, cortado[0], cortado[k], cortado[k + 1]);
                }
            }
        }
    }
    return total;
}

/* Uma arquibancada copiada: em que trecho da pista, a que distância dela, e o chão na frente */
typedef struct {
    float inicio, fim;     /* metros do jogo, ao longo da pista */
    int sentido;           /* +1: o "ao longo" do módulo vai com a pista; -1: contra */
    int secoes;            /* quantas cópias da seção cabem */
    float distancia;       /* metros do jogo da linha da pista até a frente, para a direita */
    int medidas;
    float chao[512];       /* a altura do chão na frente, a cada PASSO_DO_CHAO_DA_ARQUIBANCADA */
} TrechoDaArquibancada;

/*
 * prepararTrechoDaArquibancada
 * Recebe: a pista, o modelo (para achar o chão), a arquibancada e onde guardar o trecho.
 * Devolve: nada.
 */
static void prepararTrechoDaArquibancada(Pista *pista, const Model *modelo, const ArquibancadaQueFalta *a, TrechoDaArquibancada *trecho) {
    const float e = ESCALA_INTERLAGOS;
    trecho->inicio = a->inicio * e;
    trecho->fim = a->fim * e;
    int primeiro = (int) (trecho->inicio * SEGMENTOS_POR_METRO), ultimo = (int) (trecho->fim * SEGMENTOS_POR_METRO);
    trecho->distancia = 0.0f;
    for (int i = primeiro; i <= ultimo; i++) {
        trecho->distancia = fmaxf(trecho->distancia, buscarSegmento(pista, i)->limiteDireita);
    }
    trecho->distancia += ALEM_DO_MURO_DA_ARQUIBANCADA;
    trecho->secoes = (int) ((a->fim - a->inicio) / MODULO_COMPRIMENTO + 0.5f);
    trecho->secoes = (trecho->secoes < 1) ? 1 : trecho->secoes;
    /* A mão dos eixos: andando com a pista, a direita é "para fora"? Então o "ao longo" do módulo
     * (que com o "para cima" dá o "para fora") vai com a pista */
    PosicaoNaPista lugar = { pista->largada, 0.0f };
    moverNaPista(&lugar, trecho->inicio * UNIDADES_POR_METRO);
    PosicaoNaPista adiante = lugar;
    moverNaPista(&adiante, UNIDADES_POR_METRO);
    PontoNoMundo aqui = pontoNoMundo(lugar, 0.0f), ali = pontoNoMundo(adiante, 0.0f), aDireita = pontoNoMundo(lugar, 0.5f);
    float frenteX = ali.x - aqui.x, frenteZ = ali.z - aqui.z, direitaX = aDireita.x - aqui.x, direitaZ = aDireita.z - aqui.z;
    trecho->sentido = (-frenteZ * direitaX + frenteX * direitaZ > 0.0f) ? 1 : -1;
    trecho->medidas = 0;
    for (float m = trecho->inicio; trecho->medidas < 512; m += PASSO_DO_CHAO_DA_ARQUIBANCADA) {
        PosicaoNaPista onde = { pista->largada, 0.0f };
        moverNaPista(&onde, fminf(m, trecho->fim) * UNIDADES_POR_METRO);
        PontoNoMundo frente = pontoNoMundo(onde, trecho->distancia / onde.segmento->larguraDireita);
        float chao = chaoNoModelo(modelo, e, frente.x, frente.z);
        trecho->chao[trecho->medidas++] = (chao > -1e8f) ? chao : frente.y;
        if (m >= trecho->fim) {
            break;
        }
    }
}

/*
 * ancoraDaSecao
 * O lugar na pista (por metros) do começo de uma seção de arquibancada, medido uma vez e guardado.
 * Recebe: a pista, o trecho, a seção e os metros do começo dela.
 * Devolve: o lugar (vale para sempre, no carregamento), ou NULL se o cache está cheio.
 */
#define MAXIMO_DE_ANCORAS 256
static PosicaoNaPista *ancoraDaSecao(Pista *pista, const TrechoDaArquibancada *trecho, int secao, float metros) {
    static struct { const TrechoDaArquibancada *trecho; int secao; PosicaoNaPista lugar; } ancoras[MAXIMO_DE_ANCORAS];
    static int total = 0;
    for (int i = 0; i < total; i++) {
        if (ancoras[i].trecho == trecho && ancoras[i].secao == secao) {
            return &ancoras[i].lugar;
        }
    }
    if (total == MAXIMO_DE_ANCORAS) {
        return NULL;
    }
    ancoras[total].trecho = trecho;
    ancoras[total].secao = secao;
    ancoras[total].lugar = (PosicaoNaPista){ pista->largada, 0.0f };
    moverNaPista(&ancoras[total].lugar, metros * UNIDADES_POR_METRO);
    return &ancoras[total++].lugar;
}

/*
 * noTrecho
 * Leva um ponto do módulo (fora, cima, ao longo, em metros do modelo) de uma das seções para o
 * mundo: o "ao longo" anda pela pista, o "fora" se afasta dela, e a altura parte do chão na frente.
 * Recebe: a pista, o trecho, qual seção e o ponto.
 * Devolve: o ponto, nos eixos (e metros) do modelo.
 */
static Vector3 noTrecho(Pista *pista, const TrechoDaArquibancada *trecho, int secao, const float *p) {
    const float e = ESCALA_INTERLAGOS;
    float aoLongo = (secao + p[2] / MODULO_COMPRIMENTO) / trecho->secoes;   /* 0 a 1 no trecho */
    if (trecho->sentido < 0) {
        aoLongo = 1.0f - aoLongo;
    }
    float metros = trecho->inicio + (trecho->fim - trecho->inicio) * aoLongo;
    /* moverNaPista anda segmento por segmento a partir de onde está: partir da largada para cada vértice
     * de cada triângulo de cada seção levava segundos. Cada seção guarda o lugar do começo dela (o menor
     * dos metros que ela alcança) e só anda o resto. */
    PosicaoNaPista onde = { pista->largada, 0.0f };
    float metrosDoComeco = 0.0f;
    float noInicio = (secao + 0.0f) / trecho->secoes, noFim = (secao + 1.0f) / trecho->secoes;
    if (trecho->sentido < 0) {
        noInicio = 1.0f - noInicio;
        noFim = 1.0f - noFim;
    }
    float menorMetros = trecho->inicio + (trecho->fim - trecho->inicio) * fminf(noInicio, noFim);
    PosicaoNaPista *ancora = ancoraDaSecao(pista, trecho, secao, menorMetros);
    if (ancora != NULL && metros >= menorMetros) {
        onde = *ancora;
        metrosDoComeco = menorMetros;
    }
    moverNaPista(&onde, (metros - metrosDoComeco) * UNIDADES_POR_METRO);
    float lateral = trecho->distancia + p[0] * e;
    PontoNoMundo ponto = pontoNoMundo(onde, lateral / onde.segmento->larguraDireita);
    float medida = fminf(fmaxf((metros - trecho->inicio) / PASSO_DO_CHAO_DA_ARQUIBANCADA, 0.0f), trecho->medidas - 1.001f);
    int i = (int) medida;
    float chao = trecho->chao[i] + (trecho->chao[i + 1 < trecho->medidas ? i + 1 : i] - trecho->chao[i]) * (medida - i);
    return (Vector3){ ponto.x / e, (chao + p[1] * e) / e, ponto.z / e };
}

/*
 * acrescentarMalhasAoModelo
 * Põe as malhas no modelo, cada uma com o material dela.
 * Recebe: o modelo, as malhas, os materiais e quantas são.
 * Devolve: nada.
 */
static void acrescentarMalhasAoModelo(Model *modelo, Mesh *malhas, const int *materiais, int quantas) {
    int antes = modelo->meshCount;
    modelo->meshes = RL_REALLOC(modelo->meshes, sizeof(Mesh) * (size_t) (antes + quantas));
    modelo->meshMaterial = RL_REALLOC(modelo->meshMaterial, sizeof(int) * (size_t) (antes + quantas));
    for (int i = 0; i < quantas; i++) {
        modelo->meshes[antes + i] = malhas[i];
        modelo->meshMaterial[antes + i] = materiais[i];
    }
    modelo->meshCount = antes + quantas;
}

/*
 * construirArquibancadasQueFaltam
 * Copia a seção da arquibancada da reta ao longo de cada trecho em que falta arquibancada, junta
 * as cópias por material em malhas novas do modelo, e copia também os lugares da torcida dela.
 * Recebe: a pista, o mapa de Interlagos, os lugares da torcida do modelo e quantos são, e onde
 *         devolver a lista com os novos lugares (que o chamador libera).
 * Devolve: quantos lugares a lista devolvida tem.
 */
static int construirArquibancadasQueFaltam(Pista *pista, MapaDeDia *mapa, const LugarNaTorcida *lugares, int totalDeLugares,
                                           LugarNaTorcida **todosOsLugares) {
    static PecaDoModulo pecas[MAXIMO_DE_PECAS_DO_MODULO];
    int totalDePecas = recortarModuloDaArquibancada(&mapa->modelo, pecas, MAXIMO_DE_PECAS_DO_MODULO);

    /* Os lugares da torcida que ficam na seção, nos eixos dela */
    int naSecao = 0;
    float (*torcidaDaSecao)[3] = malloc(sizeof(float) * 3 * (size_t) (totalDeLugares > 0 ? totalDeLugares : 1));
    for (int i = 0; i < totalDeLugares; i++) {
        const LugarNaTorcida *l = &lugares[i];
        if (l->x <= MODULO_FRENTE_X && l->x >= MODULO_FUNDO_X && l->z >= MODULO_INICIO_Z && l->z < MODULO_INICIO_Z + MODULO_COMPRIMENTO) {
            torcidaDaSecao[naSecao][0] = MODULO_FRENTE_X - l->x;
            torcidaDaSecao[naSecao][1] = l->y - MODULO_CHAO_Y;
            torcidaDaSecao[naSecao][2] = l->z - MODULO_INICIO_Z;
            naSecao++;
        }
    }

    int quantas = (int) (sizeof(ARQUIBANCADAS_QUE_FALTAM) / sizeof(ARQUIBANCADAS_QUE_FALTAM[0]));
    static TrechoDaArquibancada trechos[sizeof(ARQUIBANCADAS_QUE_FALTAM) / sizeof(ARQUIBANCADAS_QUE_FALTAM[0])];
    int secoes = 0;
    for (int i = 0; i < quantas; i++) {
        prepararTrechoDaArquibancada(pista, &mapa->modelo, &ARQUIBANCADAS_QUE_FALTAM[i], &trechos[i]);
        secoes += trechos[i].secoes;
    }

    /* Uma malha por material, com todas as cópias */
    Mesh malhas[MAXIMO_DE_PECAS_DO_MODULO];
    int materiais[MAXIMO_DE_PECAS_DO_MODULO], feitas = 0;
    for (int p = 0; p < totalDePecas; p++) {
        PecaDoModulo *peca = &pecas[p];
        Mesh malha = { 0 };
        malha.triangleCount = peca->triangulos * secoes;
        malha.vertexCount = malha.triangleCount * 3;
        if (malha.vertexCount == 0) {
            free(peca->vertices);
            continue;
        }
        malha.vertices = RL_MALLOC(sizeof(float) * 3 * (size_t) malha.vertexCount);
        malha.normals = RL_MALLOC(sizeof(float) * 3 * (size_t) malha.vertexCount);
        malha.texcoords = RL_MALLOC(sizeof(float) * 2 * (size_t) malha.vertexCount);
        int v = 0;
        for (int i = 0; i < quantas; i++) {
            const TrechoDaArquibancada *trecho = &trechos[i];
            for (int secao = 0; secao < trecho->secoes; secao++) {
                for (int t = 0; t < peca->triangulos; t++) {
                    const VerticeDoModulo *tri = &peca->vertices[t * 3];
                    /* As paredes descem até bem abaixo do chão: num terreno em declive, nada flutua */
                    float ux = tri[1].p[0] - tri[0].p[0], uy = tri[1].p[1] - tri[0].p[1], uz = tri[1].p[2] - tri[0].p[2];
                    float wx = tri[2].p[0] - tri[0].p[0], wy = tri[2].p[1] - tri[0].p[1], wz = tri[2].p[2] - tri[0].p[2];
                    float cx = uy * wz - uz * wy, cy = uz * wx - ux * wz, cz = ux * wy - uy * wx;
                    float tamanho = sqrtf(cx * cx + cy * cy + cz * cz);
                    int dePe = tamanho > 0.0f && fabsf(cy) / tamanho < 0.5f;
                    Vector3 cantos[3];
                    for (int c = 0; c < 3; c++) {
                        float p3[3] = { tri[c].p[0], tri[c].p[1], tri[c].p[2] };
                        if (dePe && p3[1] < MODULO_ABAIXO_DO_CHAO) {
                            p3[1] -= ALICERCE;
                        }
                        cantos[c] = noTrecho(pista, trecho, secao, p3);
                    }
                    /* A normal: os eixos do módulo levados para o lugar dele (o fora, o ao longo e o cima ali) */
                    float p0[3] = { tri[0].p[0], 0.0f, tri[0].p[2] }, p1[3] = { tri[0].p[0] + 1.0f, 0.0f, tri[0].p[2] };
                    float p2[3] = { tri[0].p[0], 0.0f, tri[0].p[2] + 1.0f };
                    Vector3 o = noTrecho(pista, trecho, secao, p0);
                    Vector3 eixoFora = Vector3Normalize(Vector3Subtract(noTrecho(pista, trecho, secao, p1), o));
                    Vector3 eixoAoLongo = Vector3Normalize(Vector3Subtract(noTrecho(pista, trecho, secao, p2), o));
                    for (int c = 0; c < 3; c++, v++) {
                        malha.vertices[v * 3] = cantos[c].x;
                        malha.vertices[v * 3 + 1] = cantos[c].y;
                        malha.vertices[v * 3 + 2] = cantos[c].z;
                        Vector3 n = Vector3Add(Vector3Add(Vector3Scale(eixoFora, tri[c].n[0]), (Vector3){ 0.0f, tri[c].n[1], 0.0f }),
                                               Vector3Scale(eixoAoLongo, tri[c].n[2]));
                        n = Vector3Normalize(n);
                        malha.normals[v * 3] = n.x;
                        malha.normals[v * 3 + 1] = n.y;
                        malha.normals[v * 3 + 2] = n.z;
                        malha.texcoords[v * 2] = tri[c].uv[0];
                        malha.texcoords[v * 2 + 1] = tri[c].uv[1];
                    }
                }
            }
        }
        free(peca->vertices);
        UploadMesh(&malha, false);
        malhas[feitas] = malha;
        materiais[feitas++] = peca->material;
    }
    acrescentarMalhasAoModelo(&mapa->modelo, malhas, materiais, feitas);

    /* A torcida: os lugares do modelo e os das cópias */
    int total = totalDeLugares + naSecao * secoes;
    *todosOsLugares = malloc(sizeof(LugarNaTorcida) * (size_t) (total > 0 ? total : 1));
    memcpy(*todosOsLugares, lugares, sizeof(LugarNaTorcida) * (size_t) totalDeLugares);
    int n = totalDeLugares;
    for (int i = 0; i < quantas; i++) {
        for (int secao = 0; secao < trechos[i].secoes; secao++) {
            for (int l = 0; l < naSecao; l++) {
                Vector3 p = noTrecho(pista, &trechos[i], secao, torcidaDaSecao[l]);
                (*todosOsLugares)[n++] = (LugarNaTorcida){ p.x, p.y, p.z };
            }
        }
    }
    free(torcidaDaSecao);
    return total;
}

/*
 * chaoNoModelo
 * O chão do modelo embaixo de (x, z), em metros do jogo: só o que é chão (não telhados nem muros).
 * Recebe: o modelo, a escala e o ponto.
 * Devolve: a altura, ou -1e9 se não há chão ali.
 */
static float chaoNoModelo(const Model *modelo, float escala, float x, float z) {
    float maior = -1e9f;
    x /= escala;
    z /= escala;
    for (int m = 0; m < modelo->meshCount; m++) {
        int qual = modelo->meshMaterial[m];
        if (qual >= TOTAL_MATERIAIS_INTERLAGOS || MATERIAIS_INTERLAGOS[qual].chao == CHAO_NENHUM
            || MATERIAIS_INTERLAGOS[qual].chao == CHAO_MURO) {
            continue;
        }
        const Mesh *malha = &modelo->meshes[m];
        BoundingBox caixa = caixaDaMalhaEmCache(modelo, m);
        if (x < caixa.min.x || x > caixa.max.x || z < caixa.min.z || z > caixa.max.z) {
            continue;
        }
        for (int t = 0; t < malha->triangleCount; t++) {
            Vector3 v[3];
            for (int k = 0; k < 3; k++) {
                int i = (malha->indices != NULL) ? malha->indices[t * 3 + k] : t * 3 + k;
                v[k] = (Vector3){ malha->vertices[i * 3], malha->vertices[i * 3 + 1], malha->vertices[i * 3 + 2] };
            }
            float d = (v[1].z - v[2].z) * (v[0].x - v[2].x) + (v[2].x - v[1].x) * (v[0].z - v[2].z);
            if (fabsf(d) < 1e-9f) {
                continue;
            }
            float a = ((v[1].z - v[2].z) * (x - v[2].x) + (v[2].x - v[1].x) * (z - v[2].z)) / d;
            float b = ((v[2].z - v[0].z) * (x - v[2].x) + (v[0].x - v[2].x) * (z - v[2].z)) / d;
            if (a < 0.0f || b < 0.0f || a + b > 1.0f) {
                continue;
            }
            maior = fmaxf(maior, a * v[0].y + b * v[1].y + (1.0f - a - b) * v[2].y);
        }
    }
    return (maior > -1e8f) ? maior * escala : maior;
}

/* ---------- A saia do terreno de Interlagos ----------
 * O terreno do modelo (a foto de satélite) acaba a algumas centenas de metros da pista: de alguns
 * lugares (a Descida do Lago, a reta) dá para ver o vazio além dele. A saia estica cada aresta da
 * borda de fora do terreno para longe do autódromo, quase plana, até o horizonte. */
typedef struct {
    long long a, b;        /* as pontas, como chaves de posição (a menor primeiro) */
    int malha, i0, i1;     /* de onde ela veio */
} ArestaDoTerreno;

static long long chaveDaPosicao(const float *v) {
    long long x = (long long) llroundf(v[0] * 100.0f), y = (long long) llroundf(v[1] * 100.0f), z = (long long) llroundf(v[2] * 100.0f);
    return ((x & 0x1FFFFF) << 42) | ((y & 0x1FFFFF) << 21) | (z & 0x1FFFFF);
}

static int compararArestas(const void *p, const void *q) {
    const ArestaDoTerreno *a = p, *b = q;
    if (a->a != b->a) return (a->a < b->a) ? -1 : 1;
    if (a->b != b->b) return (a->b < b->b) ? -1 : 1;
    return 0;
}

/* (x, z) fica em cima de algum triângulo desta malha? */
static int cobertoPelaMalha(const Mesh *malha, float x, float z) {
    for (int t = 0; t < malha->triangleCount; t++) {
        const float *v[3];
        for (int k = 0; k < 3; k++) {
            v[k] = &malha->vertices[malha->indices[t * 3 + k] * 3];
        }
        float d = (v[1][2] - v[2][2]) * (v[0][0] - v[2][0]) + (v[2][0] - v[1][0]) * (v[0][2] - v[2][2]);
        if (fabsf(d) < 1e-9f) {
            continue;
        }
        float a = ((v[1][2] - v[2][2]) * (x - v[2][0]) + (v[2][0] - v[1][0]) * (z - v[2][2])) / d;
        float b = ((v[2][2] - v[0][2]) * (x - v[2][0]) + (v[0][0] - v[2][0]) * (z - v[2][2])) / d;
        if (a >= -1e-4f && b >= -1e-4f && a + b <= 1.0f + 1e-4f) {
            return 1;
        }
    }
    return 0;
}

/*
 * construirSaiaDoTerreno
 * Recebe: o modelo, a escala e a pista (o centro do autódromo e a área dele).
 * Devolve: a malha da saia, já na escala do jogo.
 */
static Mesh construirSaiaDoTerreno(const Model *modelo, float escala, Pista *pista) {
    int terrenos[TOTAL_MATERIAIS_INTERLAGOS], totalDeTerrenos = 0;
    int totalDeArestas = 0;
    for (int m = 0; m < modelo->meshCount; m++) {
        int qual = modelo->meshMaterial[m];
        if (qual < TOTAL_MATERIAIS_INTERLAGOS && MATERIAIS_INTERLAGOS[qual].desenho == DESENHO_TERRENO && modelo->meshes[m].indices) {
            terrenos[totalDeTerrenos++] = m;
            totalDeArestas += modelo->meshes[m].triangleCount * 3;
        }
    }
    ArestaDoTerreno *arestas = MemAlloc(sizeof(ArestaDoTerreno) * (totalDeArestas + 1));
    int n = 0;
    for (int k = 0; k < totalDeTerrenos; k++) {
        const Mesh *malha = &modelo->meshes[terrenos[k]];
        for (int t = 0; t < malha->triangleCount; t++) {
            for (int lado = 0; lado < 3; lado++) {
                int i0 = malha->indices[t * 3 + lado], i1 = malha->indices[t * 3 + (lado + 1) % 3];
                long long c0 = chaveDaPosicao(&malha->vertices[i0 * 3]), c1 = chaveDaPosicao(&malha->vertices[i1 * 3]);
                arestas[n++] = (ArestaDoTerreno){ (c0 < c1) ? c0 : c1, (c0 < c1) ? c1 : c0, terrenos[k], i0, i1 };
            }
        }
    }
    qsort(arestas, n, sizeof(ArestaDoTerreno), compararArestas);

    /* O centro e a área do autódromo, nas medidas do modelo */
    float area[4] = { 1e9f, 1e9f, -1e9f, -1e9f };
    Segmento *atual = pista->largada;
    do {
        area[0] = fminf(area[0], atual->xMundo / escala);
        area[1] = fminf(area[1], atual->zMundo / escala);
        area[2] = fmaxf(area[2], atual->xMundo / escala);
        area[3] = fmaxf(area[3], atual->zMundo / escala);
        atual = atual->proximo;
    } while (atual != pista->largada);
    float centroX = (area[0] + area[2]) / 2.0f, centroZ = (area[1] + area[3]) / 2.0f;

    Construcao saia = { 0 };
    int esticadas = 0;
    for (int i = 0; i < n; i++) {
        int repetida = (i > 0 && compararArestas(&arestas[i], &arestas[i - 1]) == 0)
                    || (i + 1 < n && compararArestas(&arestas[i], &arestas[i + 1]) == 0);
        if (repetida) {
            continue;   /* tem triângulo dos dois lados: não é borda */
        }
        const Mesh *malha = &modelo->meshes[arestas[i].malha];
        const float *p0 = &malha->vertices[arestas[i].i0 * 3], *p1 = &malha->vertices[arestas[i].i1 * 3];
        float meioX = (p0[0] + p1[0]) / 2.0f, meioZ = (p0[2] + p1[2]) / 2.0f;
        /* Só a borda de fora: longe do autódromo, e sem outro terreno em cima */
        if (meioX > area[0] - MARGEM_DA_SAIA && meioX < area[2] + MARGEM_DA_SAIA
            && meioZ > area[1] - MARGEM_DA_SAIA && meioZ < area[3] + MARGEM_DA_SAIA) {
            continue;
        }
        int coberta = 0;
        for (int k = 0; k < totalDeTerrenos && !coberta; k++) {
            if (terrenos[k] != arestas[i].malha) {
                coberta = cobertoPelaMalha(&modelo->meshes[terrenos[k]], meioX + (meioX - centroX) * 0.01f,
                                           meioZ + (meioZ - centroZ) * 0.01f);
            }
        }
        if (coberta) {
            continue;
        }
        Vector3 a = { p0[0], p0[1], p0[2] }, b = { p1[0], p1[1], p1[2] };
        Vector3 longeA = a, longeB = b;
        Vector3 *pontas[2] = { &longeA, &longeB };
        for (int k = 0; k < 2; k++) {
            float dx = pontas[k]->x - centroX, dz = pontas[k]->z - centroZ, d = hypotf(dx, dz);
            pontas[k]->x += dx / d * COMPRIMENTO_DA_SAIA;
            pontas[k]->z += dz / d * COMPRIMENTO_DA_SAIA;
            pontas[k]->y -= COMPRIMENTO_DA_SAIA * DESCIDA_DA_SAIA;
        }
        Vector3 q[4] = { a, b, longeB, longeA };
        Vector2 uv[4];
        for (int k = 0; k < 4; k++) {
            q[k] = Vector3Scale(q[k], escala);
            uv[k] = (Vector2){ q[k].x / LADRILHO_DA_SAIA, q[k].z / LADRILHO_DA_SAIA };
        }
        adicionarQuadrilatero(&saia, q[0], q[1], q[2], q[3], uv[0], uv[1], uv[2], uv[3], COR_DA_SAIA);
        esticadas++;
    }
    MemFree(arestas);
    TraceLog(LOG_INFO, "MUNDO: saia do terreno com %d arestas da borda", esticadas);
    return terminarMalha(&saia);
}

/*
 * pontoDaPistaEmMetros
 * Recebe: a pista, os metros desde a linha e a lateral em metros.
 * Devolve: o ponto no mundo.
 */
static PontoNoMundo pontoDaPistaEmMetros(Pista *pista, float metros, float lateral) {
    PosicaoNaPista lugar = { pista->largada, 0.0f };
    moverNaPista(&lugar, fmodf(metros + pista->metrosDaVolta, pista->metrosDaVolta) * UNIDADES_POR_METRO);
    return pontoNoMundo(lugar, lateral / larguraDoLado(lugar.segmento, lateral));
}

/*
 * colocarTeloesDeInterlagos
 * Recebe: a pista e o modelo (o pé de cada telão vai no chão, ou no telhado, do que está ali).
 * Devolve: nada.
 */
static void colocarTeloesDeInterlagos(Pista *pista, const Model *modelo) {
    int total = (int) (sizeof(TELOES_DE_INTERLAGOS) / sizeof(TELOES_DE_INTERLAGOS[0]));
    for (int i = 0; i < total; i++) {
        const TelaoDeInterlagos *t = &TELOES_DE_INTERLAGOS[i];
        const float e = ESCALA_INTERLAGOS;   /* a tabela está nas medidas do modelo */
        PontoNoMundo pe = pontoDaPistaEmMetros(pista, t->metros * e, t->lateral * e);
        PontoNoMundo alvo = pontoDaPistaEmMetros(pista, t->olharMetros * e, t->olharLateral * e);
        float altura = alturaNoModelo(modelo, pe.x / e, pe.z / e);
        Vector3 lugar = { pe.x, (altura > -1e8f) ? altura * e : pe.y, pe.z };
        colocarNoCenario(CENARIO_TELAO, lugar, atan2f(alvo.x - pe.x, alvo.z - pe.z), t->tamanho * e);
    }
    colocarTeloesDasArquibancadas(pista, modelo);
}

/*
 * longeDeOutraPista
 * Recebe: a pista, um ponto do mundo, o segmento perto do qual ele foi posto e a folga.
 * Devolve: 1 se nenhum outro trecho da pista (longe deste) passa a menos da folga dele.
 */
static int longeDeOutraPista(Pista *pista, float x, float z, int indice, float folga) {
    int total = (int) (pista->metrosDaVolta * SEGMENTOS_POR_METRO);
    Segmento *atual = pista->largada;
    do {
        int distancia = abs(atual->indice - indice);
        distancia = (distancia < total - distancia) ? distancia : total - distancia;
        if (distancia > LONGE_NA_VOLTA * SEGMENTOS_POR_METRO) {
            float largura = fmaxf(atual->limiteDireita, atual->limiteEsquerda);
            if (hypotf(atual->xMundo - x, atual->zMundo - z) < largura + folga) {
                return 0;
            }
        }
        atual = atual->proximo;
    } while (atual != pista->largada);
    return 1;
}

/*
 * longeDoPitLane
 * Recebe: a pista, um ponto do mundo e a folga.
 * Devolve: 1 se a estrada do pit lane (o caminho medido) não passa a menos da folga dele.
 */
static int longeDoPitLane(Pista *pista, float x, float z, float folga) {
    const DadosDoPit *pit = &pista->pit;
    for (int i = 0; i < pit->pontosDoCaminho; i++) {
        float lateral = pit->lado * pit->caminho[i][1];
        PontoNoMundo ponto = pontoDaPistaEmMetros(pista, pit->entrada + pit->caminho[i][0] - pit->caminho[0][0], lateral);
        if (hypotf(ponto.x - x, ponto.z - z) < folga) {
            return 0;
        }
    }
    return 1;
}

/*
 * colocarTeloesDasArquibancadas
 * Um telão de frente para cada arquibancada que o jogo levanta (dois nas compridas), do outro
 * lado da pista, além do muro: de qualquer lugar dela dá para ver um. Se o lugar cai perto de
 * outro trecho da pista, ele vai mais para longe; se não houver lugar, fica sem.
 * Recebe: a pista e o modelo (o pé vai no chão).
 * Devolve: nada.
 */
static void colocarTeloesDasArquibancadas(Pista *pista, const Model *modelo) {
    const float e = ESCALA_INTERLAGOS;
    int quantas = (int) (sizeof(ARQUIBANCADAS_QUE_FALTAM) / sizeof(ARQUIBANCADAS_QUE_FALTAM[0]));
    for (int i = 0; i < quantas; i++) {
        const ArquibancadaQueFalta *a = &ARQUIBANCADAS_QUE_FALTAM[i];
        float comprimento = (a->fim - a->inicio) * e;
        int teloes = (comprimento > ARQUIBANCADA_DE_DOIS_TELOES) ? 2 : 1;
        for (int t = 0; t < teloes; t++) {
            float metros = (a->inicio * e) + comprimento * (t + 0.5f) / teloes;
            int primeiro = (int) ((metros - PERTO_DO_TELAO) * SEGMENTOS_POR_METRO), ultimo = (int) ((metros + PERTO_DO_TELAO) * SEGMENTOS_POR_METRO);
            float muro = 0.0f, frente = 0.0f;
            for (int k = primeiro; k <= ultimo; k++) {
                Segmento *s = buscarSegmento(pista, k);
                muro = fmaxf(muro, s->limiteEsquerda);
                frente = fmaxf(frente, s->limiteDireita);
            }
            for (float alem = TELAO_ALEM_DO_MURO; alem <= TELAO_ALEM_DO_MURO + 3.0f * TELAO_MAIS_LONGE; alem += TELAO_MAIS_LONGE) {
                PontoNoMundo pe = pontoDaPistaEmMetros(pista, metros, -(muro + alem));
                if (!longeDeOutraPista(pista, pe.x, pe.z, (int) (metros * SEGMENTOS_POR_METRO), TELAO_LONGE_DE_OUTRA_PISTA)
                    || !longeDoPitLane(pista, pe.x, pe.z, TELAO_LONGE_DE_OUTRA_PISTA)) {
                    continue;
                }
                PontoNoMundo alvo = pontoDaPistaEmMetros(pista, metros, frente + ALEM_DO_MURO_DA_ARQUIBANCADA + 10.0f);
                float chao = chaoNoModelo(modelo, e, pe.x, pe.z);
                Vector3 lugar = { pe.x, (chao > -1e8f) ? chao : pe.y, pe.z };
                colocarNoCenario(CENARIO_TELAO, lugar, atan2f(alvo.x - pe.x, alvo.z - pe.z), e);
                break;
            }
        }
    }
}

/*
 * ligarTeloesDoCenario
 * Os telões das pistas (telao.glb e telao_katchau.glb) têm uma imagem de espera na tela, só
 * deles: onde ela estiver nos lotes, entra a transmissão ao vivo (telao.c).
 * Recebe: nada.
 * Devolve: nada.
 */
static void ligarTeloesDoCenario(void) {
    Texture2D transmissao = texturaDoTelao();
    if (transmissao.id == 0) {
        return;
    }
    int trocados = 0;
    for (int q = 0; q < TOTAL_MODELOS_DE_OBJETO; q++) {
        Model *modelo = &modelosDosObjetos[q];
        for (int m = 0; m < modelo->materialCount; m++) {
            Texture2D tela = modelo->materials[m].maps[MATERIAL_MAP_ALBEDO].texture;
            if (tela.width != LARGURA_DA_TELA_DE_ESPERA || tela.height != ALTURA_DA_TELA_DE_ESPERA) {
                continue;
            }
            for (int i = 0; i < totalDeLotes; i++) {
                if (lotes[i].textura.id == tela.id) {
                    lotes[i].textura = transmissao;
                    lotes[i].brilha = 1;
                    trocados++;
                }
            }
        }
    }
    TraceLog(LOG_INFO, "MUNDO: %d lotes de tela de telao ligados a transmissao", trocados);
}

/*
 * carregarCeuDoPasseio
 * O shader e o material do céu do passeio (veja PIXELS_DO_CEU).
 */
static void carregarCeuDoPasseio(void) {
    shaderDoPrepasse = LoadShaderFromMemory(VERTICES_DO_PREPASSE, PIXELS_DA_PROFUNDIDADE);
    shaderDoPrepasseComCor = LoadShaderFromMemory(VERTICES_DO_PREPASSE_COM_COR, PIXELS_DO_PREPASSE_COM_COR);
    shaderDoCeu = LoadShaderFromMemory(VERTICES_DO_CEU, PIXELS_DO_CEU);
    materialDoCeu = LoadMaterialDefault();
    materialDoCeu.shader = shaderDoCeu;
    locMisturaDoDia = GetShaderLocation(shaderDoCeu, "misturaDoDia");
    locCorDoHorizonte = GetShaderLocation(shaderDoCeu, "corDoHorizonte");
    locCorDoAlto = GetShaderLocation(shaderDoCeu, "corDoAlto");
    locCorDasNuvens = GetShaderLocation(shaderDoCeu, "corDasNuvens");
    locCorDoBrilho = GetShaderLocation(shaderDoCeu, "corDoBrilho");
    locEstrelas = GetShaderLocation(shaderDoCeu, "estrelas");
    locSolNoCeu = GetShaderLocation(shaderDoCeu, "solNoCeu");
    locCorDoSol = GetShaderLocation(shaderDoCeu, "corDoSol");
    locLuaNoCeu = GetShaderLocation(shaderDoCeu, "luaNoCeu");
    locBrilhoDaLua = GetShaderLocation(shaderDoCeu, "brilhoDaLua");
}

/*
 * desenharCeuDoPasseio
 * O céu do passeio, com a luz do dia aplicada (luzAplicada).
 * Recebe: a matriz que põe o céu em volta da câmera.
 */
static void desenharCeuDoPasseio(Matrix naCamera) {
    const LuzDoDia *luz = luzAplicada();
    SetShaderValue(shaderDoCeu, locMisturaDoDia, &luz->misturaDoDia, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shaderDoCeu, locCorDoHorizonte, &luz->horizonte, SHADER_UNIFORM_VEC3);
    SetShaderValue(shaderDoCeu, locCorDoAlto, &luz->alto, SHADER_UNIFORM_VEC3);
    SetShaderValue(shaderDoCeu, locCorDasNuvens, &luz->nuvens, SHADER_UNIFORM_VEC3);
    SetShaderValue(shaderDoCeu, locCorDoBrilho, &luz->brilho, SHADER_UNIFORM_VEC3);
    SetShaderValue(shaderDoCeu, locEstrelas, &luz->estrelas, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shaderDoCeu, locSolNoCeu, &luz->solNoCeu, SHADER_UNIFORM_VEC3);
    SetShaderValue(shaderDoCeu, locCorDoSol, &luz->corDoSol, SHADER_UNIFORM_VEC3);
    SetShaderValue(shaderDoCeu, locLuaNoCeu, &luz->luaNoCeu, SHADER_UNIFORM_VEC3);
    SetShaderValue(shaderDoCeu, locBrilhoDaLua, &luz->lua, SHADER_UNIFORM_FLOAT);
    Texture2D branca = { rlGetTextureIdDefault(), 1, 1, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };
    materialDoCeu.maps[MATERIAL_MAP_METALNESS].texture = texturaDoCeuNoturno();
    materialDoCeu.maps[MATERIAL_MAP_ALBEDO].texture = texturaCeu;
    DrawMesh(ceuDeDia, materialDoCeu, naCamera);
    materialDoCeu.maps[MATERIAL_MAP_ALBEDO].texture = branca;
    DrawMesh(tampaDoCeu, materialDoCeu, naCamera);
}

/*
 * carregarMundo
 * Prepara o cenário das pistas. Chamar uma vez, depois de criar a janela.
 * Recebe: as pistas, na ordem de IdPista.
 * Devolve: nada.
 */
void carregarMundo(Pista *pistas[]) {
    double tInicio = GetTime();
    shader = LoadShaderFromMemory(VERTICES_DO_SHADER, PIXELS_DO_SHADER);
    shaderDosMapasDeDia = LoadShaderFromMemory(VERTICES_DOS_MAPAS_DE_DIA, PIXELS_DOS_MAPAS_DE_DIA);
    totalQueRecebemSombra = 0;
    recebemSombra[totalQueRecebemSombra++] = shader;
    recebemSombra[totalQueRecebemSombra++] = shaderDosMapasDeDia;
    registrarShaderDoDia(shader);
    registrarShaderDoDia(shaderDosMapasDeDia);
    carregarShadersDeInterlagos();
    carregarSombras();
    material = LoadMaterialDefault();
    material.shader = shader;

    texturaCeu = carregarTexturaRepetida("fundo/ceu.png");
    texturaAsfalto[0] = carregarTexturaRepetida("estrada/asfalto_escuro.png");
    texturaAsfalto[1] = carregarTexturaRepetida("estrada/asfalto_claro.png");
    texturaGrama = carregarTexturaRepetida("estrada/grama_clara.png");

    /* A imagem ocupa a altura proporcional à largura de cada repetição, sem esticar */
    float voltaCeu = 2.0f * PI * RAIO_DO_CEU / REPETICOES_CEU;
    construirCeuDeDia(voltaCeu * texturaCeu.height / texturaCeu.width);
    ceuDeNoite = construirCeuDeNoite();
    /* Na vertical a imagem não se repete: a borda de cima não pode misturar com a de baixo */
    rlTextureParameters(texturaCeu.id, RL_TEXTURE_WRAP_T, RL_TEXTURE_WRAP_CLAMP);
    carregarCeuDoPasseio();

    TraceLog(LOG_INFO, "CARGA-MUNDO: shaders/texturas/ceu %.2f s", GetTime() - tInicio); tInicio = GetTime();
    passoDaEspera("CARREGANDO CENARIOS", 0.02f);
    carregarModelosDosObjetos();
    TraceLog(LOG_INFO, "CARGA-MUNDO: objetos %.2f s", GetTime() - tInicio); tInicio = GetTime();
    passoDaEspera("CARREGANDO INTERLAGOS", 0.08f);
    carregarMapaDeDia(pistas[PISTA_INTERLAGOS], ARQUIVO_DE_INTERLAGOS, ESCALA_INTERLAGOS, TORCIDA_INTERLAGOS,
                      TOTAL_TORCIDA_INTERLAGOS, FIGURA_DE_INTERLAGOS * ESCALA_INTERLAGOS, NULL, 0);
    TraceLog(LOG_INFO, "CARGA-MUNDO: interlagos %.2f s", GetTime() - tInicio); tInicio = GetTime();
    passoDaEspera("CARREGANDO COPA PISTAO", 0.70f);
    carregarEstadio(pistas[PISTA_COPA_PISTAO]);
    passoDaEspera("CARREGANDO OVAL", 0.80f);
    TraceLog(LOG_INFO, "CARGA-MUNDO: estadio %.2f s", GetTime() - tInicio); tInicio = GetTime();
    carregarMapaDeDia(pistas[PISTA_OVAL], ARQUIVO_DO_OVAL, ESCALA_OVAL, TORCIDA_OVAL, TOTAL_TORCIDA_OVAL, FIGURA_DO_OVAL,
                      CENARIO_OVAL, TOTAL_CENARIO_OVAL);
    TraceLog(LOG_INFO, "CARGA-MUNDO: oval %.2f s", GetTime() - tInicio); tInicio = GetTime();
    passoDaEspera("CARREGANDO SPLAT", 0.92f);
    carregarMapaDeDia(pistas[PISTA_SPLAT], ARQUIVO_DO_SPLAT, ESCALA_SPLAT, TORCIDA_SPLAT, TOTAL_TORCIDA_SPLAT, FIGURA_DO_SPLAT,
                      CENARIO_SPLAT, TOTAL_CENARIO_SPLAT);
    TraceLog(LOG_INFO, "CARGA-MUNDO: splat %.2f s", GetTime() - tInicio); tInicio = GetTime();
    iniciarTorcida3D();
    ligarTeloesDoCenario();
    TraceLog(LOG_INFO, "CARGA-MUNDO: torcida/telões %.2f s", GetTime() - tInicio); tInicio = GetTime();
}

/*
 * descarregarMundo
 * Devolve a memória de vídeo do cenário. Chamar antes de CloseWindow.
 * Recebe: nada.
 * Devolve: nada.
 */
/*
 * carregarMundoAberto
 * Carrega o mapa do passeio livre na primeira vez que ele é pedido (são dezenas de MB: quem
 * só corre não paga por ele). Todas as malhas usam o sol e a sombra dos mapas de dia.
 * Recebe: nada.
 * Devolve: 1 se o mapa está pronto, 0 se o arquivo não existe.
 */
/*
 * desenharMapaDoMundoAberto
 * Só o mapa do passeio, sem céu (para fotografá-lo de cima, no minimapa). Chamar entre
 * BeginMode3D e EndMode3D.
 * Recebe: nada.
 * Devolve: nada.
 */
void desenharMapaDoMundoAberto(void) {
    if (temMundoAberto) {
        DrawModel(mundoAberto, (Vector3){ 0.0f, 0.0f, 0.0f }, 1.0f, WHITE);
    }
}

/*
 * prepararModeloDoMundoAberto
 * Um modelo que vai andar pelo mundo aberto (o Mack) ganha a mesma luz e a mesma sombra do mapa.
 * Recebe: o modelo.
 * Devolve: nada.
 */
void prepararModeloDoMundoAberto(Model *modelo) {
    for (int i = 0; i < modelo->materialCount; i++) {
        modelo->materials[i].shader = shaderDosMapasDeDia;
        Texture2D *textura = &modelo->materials[i].maps[MATERIAL_MAP_ALBEDO].texture;
        if (textura->id != 0 && textura->id != rlGetTextureIdDefault()) {
            GenTextureMipmaps(textura);
            SetTextureFilter(*textura, TEXTURE_FILTER_TRILINEAR);
        }
    }
}

/*
 * colocarNoMundoAberto / desenharModelosNoMundoAberto
 * Onde cada malha do modelo está agora: ele faz sombra no mapa (desenharQuemFazSombra) e é
 * desenhado aqui. As matrizes são lidas na hora do desenho: quem chama as mantém vivas.
 * Recebe: o lugar (0 a MODELOS_NO_MUNDO_ABERTO - 1), o modelo (NULL tira) e uma matriz por malha.
 */
void colocarNoMundoAberto(int qual, const Model *modelo, const Matrix *matrizDeCadaMalha) {
    if (qual < 0 || qual >= MODELOS_NO_MUNDO_ABERTO) {
        return;
    }
    modelosNoMundoAberto[qual] = (matrizDeCadaMalha != NULL) ? modelo : NULL;
    matrizesNoMundoAberto[qual] = matrizDeCadaMalha;
}

/* Desenha os modelos colocados; com um material dado (a sombra), todos com ele */
static void desenharModelosColocados(const Material *trocado) {
    for (int qual = 0; qual < MODELOS_NO_MUNDO_ABERTO; qual++) {
        const Model *modelo = modelosNoMundoAberto[qual];
        if (modelo == NULL) {
            continue;
        }
        for (int i = 0; i < modelo->meshCount; i++) {
            Material usado = (trocado != NULL) ? *trocado : modelo->materials[modelo->meshMaterial[i]];
            DrawMesh(modelo->meshes[i], usado, MatrixMultiply(modelo->transform, matrizesNoMundoAberto[qual][i]));
        }
    }
}

void desenharModelosNoMundoAberto(void) {
    desenharModelosColocados(NULL);
}

int carregarMundoAberto(void) {
    if (temMundoAberto) {
        return 1;
    }
    if (!FileExists(ARQUIVO_DO_MUNDO_ABERTO)) {
        TraceLog(LOG_WARNING, "MUNDO: falta %s (gere com ferramentas/montar_mundo_aberto.py)", ARQUIVO_DO_MUNDO_ABERTO);
        return 0;
    }
    mundoAberto = LoadModel(ARQUIVO_DO_MUNDO_ABERTO);
    for (int i = 0; i < mundoAberto.materialCount; i++) {
        mundoAberto.materials[i].shader = shaderDosMapasDeDia;
        Texture2D *textura = &mundoAberto.materials[i].maps[MATERIAL_MAP_ALBEDO].texture;
        if (textura->id != 0 && textura->id != rlGetTextureIdDefault()) {
            GenTextureMipmaps(textura);
            SetTextureFilter(*textura, TEXTURE_FILTER_TRILINEAR);
            SetTextureFilter(*textura, filtroDeTexturaDaConfig());
            SetTextureWrap(*textura, TEXTURE_WRAP_REPEAT);
        }
    }
    temMundoAberto = 1;
    return 1;
}

void descarregarMundo(void) {
    if (temMundoAberto) {
        for (int i = 0; i < mundoAberto.materialCount; i++) {
            mundoAberto.materials[i].shader.id = rlGetShaderIdDefault();   /* o shader é devolvido lá embaixo */
        }
        UnloadModel(mundoAberto);
        temMundoAberto = 0;
    }
    /* O shader é compartilhado: é devolvido uma vez só, aqui embaixo */
    for (int i = 0; i < estadio.materialCount; i++) {
        estadio.materials[i].shader.id = rlGetShaderIdDefault();
    }
    descarregarTorcida3D();
    descarregarTelao();
    UnloadModel(estadio);
    free(caixasDoEstadio);
    caixasDoEstadio = NULL;
    for (int i = 0; i < TOTAL_PISTAS; i++) {
        if (mapasDeDia[i].existe) {
            for (int m = 0; m < mapasDeDia[i].modelo.materialCount; m++) {
                Material *qual = &mapasDeDia[i].modelo.materials[m];
                qual->shader.id = rlGetShaderIdDefault();
                if (i == PISTA_INTERLAGOS && m == MATERIAL_DO_GUARD_RAIL_ENFERRUJADO) {
                    qual->maps[MATERIAL_MAP_ALBEDO].texture.id = 0;   /* é a do guard-rail novo: devolvida com ele */
                }
                /* O asfalto e a grama do jogo são devolvidos lá embaixo, uma vez só */
                for (int k = 0; k <= MATERIAL_MAP_BRDF; k++) {
                    unsigned int id = qual->maps[k].texture.id;
                    if (id == texturaAsfalto[0].id || id == texturaGrama.id || id == texturaDoConcreto.id) {
                        qual->maps[k].texture.id = 0;
                    }
                }
            }
            UnloadModel(mapasDeDia[i].modelo);
            free(mapasDeDia[i].caixas);
            free(mapasDeDia[i].opacas);
            free(mapasDeDia[i].deSombra);
            free(mapasDeDia[i].deTela);
            UnloadMesh(mapasDeDia[i].pintura);
            UnloadMesh(mapasDeDia[i].torcida);
            if (mapasDeDia[i].saia.vertexCount > 0) {
                UnloadMesh(mapasDeDia[i].saia);
            }
        }
    }
    UnloadMesh(pinturaDoEstadio);
    for (int i = 0; i < totalDeLotes; i++) {
        UnloadMesh(lotes[i].malha);
    }
    for (int i = 0; i < TOTAL_MODELOS_DE_OBJETO; i++) {
        UnloadModel(modelosDosObjetos[i]);
    }
    UnloadMesh(ceuDeDia);
    UnloadMesh(tampaDoCeu);
    UnloadMesh(ceuDeNoite);
    UnloadTexture(texturaCeu);
    UnloadTexture(texturaAsfalto[0]);
    UnloadTexture(texturaAsfalto[1]);
    UnloadTexture(texturaGrama);
    UnloadTexture(texturaDoConcreto);
    material.maps[MATERIAL_MAP_ALBEDO].texture.id = rlGetTextureIdDefault();
    material.shader.id = rlGetShaderIdDefault();
    UnloadMaterial(material);
    materialDoCeu.maps[MATERIAL_MAP_ALBEDO].texture.id = rlGetTextureIdDefault();   /* as imagens são de outros */
    materialDoCeu.maps[MATERIAL_MAP_METALNESS].texture.id = rlGetTextureIdDefault();
    materialDoCeu.shader.id = rlGetShaderIdDefault();
    UnloadMaterial(materialDoCeu);
    UnloadShader(shaderDoCeu);
    UnloadShader(shaderDoPrepasse);
    UnloadShader(shaderDoPrepasseComCor);
    UnloadShader(shader);
    UnloadShader(shaderDosMapasDeDia);
    for (int d = 0; d < TOTAL_DE_DESENHOS; d++) {
        if (PIXELS_DE_INTERLAGOS[d] != NULL) {
            UnloadShader(shadersDeInterlagos[d]);
        }
    }
    UnloadShader(shaderDaProfundidade);
    if (temMapaDeSombra) {
        rlUnloadTexture(mapaDeSombra.depth.id);
        rlUnloadFramebuffer(mapaDeSombra.id);
    }
    descarregarNoite();
}

/* ==================================================================
 * Desenho
 * ================================================================== */

/*
 * desenharMalha
 * Recebe: a malha, a textura (id 0 = branco, só as cores dos vértices) e a matriz.
 * Devolve: nada.
 */
static void desenharMalha(Mesh malha, Texture2D textura, Matrix posicao) {
    if (malha.vertexCount == 0) {
        return;   /* uma malha que a pista não tem (as arquibancadas construídas, fora de Interlagos) */
    }
    Texture2D branca = { rlGetTextureIdDefault(), 1, 1, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };
    material.maps[MATERIAL_MAP_ALBEDO].texture = (textura.id != 0) ? textura : branca;
    DrawMesh(malha, material, posicao);
}

/*
 * desenharLote
 * Recebe: o lote e a matriz.
 * Devolve: nada (as telas dos telões vão sem luz: elas brilham sozinhas).
 */
static void desenharLote(int i, Matrix origem) {
    if (!caixaAVista(lotes[i].caixa)) {
        return;
    }
    if (lotes[i].brilha) {
        Shader antes = material.shader;
        material.shader = shaderDaTelaQueBrilha();
        desenharMalha(lotes[i].malha, lotes[i].textura, origem);
        material.shader = antes;
        return;
    }
    desenharMalha(lotes[i].malha, lotes[i].textura, origem);
}

/*
 * desenharTelasQueBrilham
 * Só as telas dos telões, mais fracas, para o halo (veja posprocesso.c).
 * Recebe: a pista.
 * Devolve: nada.
 */
void desenharTelasQueBrilham(IdPista id) {
    if (id == PISTA_COPA_PISTAO) {
        desenharTelasDoEstadio();
        return;
    }
    int primeiro = 0, fim = 0;
    if (mapasDeDia[id].existe) {
        primeiro = mapasDeDia[id].primeiroLote;
        fim = mapasDeDia[id].fimDosLotes;
    }
    Shader antes = material.shader;
    Color corAntes = material.maps[MATERIAL_MAP_ALBEDO].color;
    material.shader = shaderDaTelaQueBrilha();
    material.maps[MATERIAL_MAP_ALBEDO].color = COR_DA_TELA_NO_HALO;
    for (int i = primeiro; i < fim; i++) {
        if (lotes[i].brilha) {
            desenharMalha(lotes[i].malha, lotes[i].textura, MatrixIdentity());
        }
    }
    material.shader = antes;
    material.maps[MATERIAL_MAP_ALBEDO].color = corAntes;
}

/*
 * carregarSombras
 * O mapa de sombra: um framebuffer só com profundidade.
 * Recebe: nada.
 * Devolve: nada.
 */
static void carregarSombras(void) {
    ladoDoMapaDeSombraAtual = ladoDoMapaDeSombra(configuracoesAtuais()->qualidadeDasSombras);
    if (ladoDoMapaDeSombraAtual < 1024) {
        ladoDoMapaDeSombraAtual = 1024;   /* sem sombras no começo: o mapa pequeno fica, caso elas sejam ligadas depois */
    }
    shaderDaProfundidade = LoadShaderFromMemory(VERTICES_DA_PROFUNDIDADE, PIXELS_DA_PROFUNDIDADE);
    mapaDeSombra.id = rlLoadFramebuffer();
    mapaDeSombra.texture.width = LADO_DO_MAPA_DE_SOMBRA;
    mapaDeSombra.texture.height = LADO_DO_MAPA_DE_SOMBRA;
    mapaDeSombra.depth.id = rlLoadTextureDepth(LADO_DO_MAPA_DE_SOMBRA, LADO_DO_MAPA_DE_SOMBRA, false);
    mapaDeSombra.depth.width = LADO_DO_MAPA_DE_SOMBRA;
    mapaDeSombra.depth.height = LADO_DO_MAPA_DE_SOMBRA;
    rlFramebufferAttach(mapaDeSombra.id, mapaDeSombra.depth.id, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_TEXTURE2D, 0);
    temMapaDeSombra = rlFramebufferComplete(mapaDeSombra.id);
    if (!temMapaDeSombra) {
        TraceLog(LOG_WARNING, "MUNDO: sem mapa de sombra; o cenário fica sem sombras");
    }
    int slot = SLOT_DA_SOMBRA;
    for (int i = 0; i < totalQueRecebemSombra; i++) {
        SetShaderValue(recebemSombra[i], GetShaderLocation(recebemSombra[i], "mapaDeSombra"), &slot, SHADER_UNIFORM_INT);
        locSombraLigada[i] = GetShaderLocation(recebemSombra[i], "sombraLigada");
        locLuzVP[i] = GetShaderLocation(recebemSombra[i], "luzVP");
    }
}

/*
 * desenharQuemFazSombra
 * O que faz sombra no chão: as estruturas, as árvores e os objetos de cada pista.
 * Recebe: a pista.
 * Devolve: nada.
 */
static void desenharQuemFazSombra(IdPista id) {
    Matrix origem = MatrixIdentity();
    Shader antes = material.shader;
    material.shader = shaderDaProfundidade;
    if ((int) id == (int) CENARIO_DO_MUNDO_ABERTO) {
        if (temMundoAberto) {
            Shader originais[256];
            int total = mundoAberto.materialCount < 256 ? mundoAberto.materialCount : 256;
            for (int i = 0; i < total; i++) {
                originais[i] = mundoAberto.materials[i].shader;
                mundoAberto.materials[i].shader = shaderDaProfundidade;
            }
            DrawModel(mundoAberto, (Vector3){ 0.0f, 0.0f, 0.0f }, 1.0f, WHITE);
            for (int i = 0; i < total; i++) {
                mundoAberto.materials[i].shader = originais[i];
            }
        }
        desenharModelosColocados(&material);   /* o Mack também faz sombra */
    } else if (mapasDeDia[id].existe) {
        MapaDeDia *mapa = &mapasDeDia[id];
        if (id == PISTA_INTERLAGOS) {
            desenharSombraDeInterlagos(mapa);
        } else {
            Shader originais[256];
            int total = mapa->modelo.materialCount < 256 ? mapa->modelo.materialCount : 256;
            for (int i = 0; i < total; i++) {
                originais[i] = mapa->modelo.materials[i].shader;
                mapa->modelo.materials[i].shader = shaderDaProfundidade;
            }
            desenharModeloCortado(mapa);
            for (int i = 0; i < total; i++) {
                mapa->modelo.materials[i].shader = originais[i];
            }
        }
        for (int i = mapa->primeiroLote; i < mapa->fimDosLotes; i++) {
            desenharLote(i, origem);
        }
    }
    material.shader = antes;
}

/*
 * prepararSombras
 * Desenha o cenário visto do sol, num quadrado em volta da câmera, e passa o resultado
 * aos shaders do cenário. Chamar antes de BeginMode3D. À noite, e com a opção desligada,
 * não há sombra do sol. O sol é o da luz do dia aplicada (hora.c): no passeio ele anda com a
 * hora, e o mapa é refeito quando ele anda um pouco.
 * Recebe: a pista, a câmera e se as sombras estão ligadas nas configurações.
 * Devolve: nada.
 */
void prepararSombras(IdPista id, Camera3D camera, int ligadas) {
    const LuzDoDia *luzDoDia = luzAplicada();
    sombraLigadaAgora = ligadas && temMapaDeSombra && id != PISTA_COPA_PISTAO && luzDoDia->sombra > 0.005f;
    float ligada = sombraLigadaAgora ? 1.0f : 0.0f;
    /* O valor quase nunca muda: mandá-lo de novo não faz nada */
    static float ultimaLigada = -1.0f;
    if (ligada != ultimaLigada) {
        ultimaLigada = ligada;
        for (int i = 0; i < totalQueRecebemSombra; i++) {
            SetShaderValue(recebemSombra[i], locSombraLigada[i], &ligada, SHADER_UNIFORM_FLOAT);
        }
    }
    if (!sombraLigadaAgora) {
        return;
    }

    /* O meio do quadrado: à frente da câmera, preso à grade do mapa (senão a borda das sombras treme) */
    Vector3 frente = Vector3Subtract(camera.target, camera.position);
    frente.y = 0.0f;
    frente = Vector3Normalize(frente);
    float passo = AREA_DA_SOMBRA / LADO_DO_MAPA_DE_SOMBRA * 4.0f;
    Vector3 meio = { camera.position.x + frente.x * ADIANTE_DA_SOMBRA, camera.position.y, camera.position.z + frente.z * ADIANTE_DA_SOMBRA };
    meio.x = roundf(meio.x / passo) * passo;
    meio.z = roundf(meio.z / passo) * passo;

    /* O cenário não se mexe: o mapa só é refeito quando a câmera anda bastante (ou muda a pista) */
    static Vector3 meioAnterior = { 1e9f, 0.0f, 1e9f };
    static Vector3 solAnterior = { 0.0f, 0.0f, 0.0f };
    static int pistaAnterior = -1;
    Vector3 sol = luzDoDia->sol;
    if ((int) id == pistaAnterior && hypotf(meio.x - meioAnterior.x, meio.z - meioAnterior.z) < REFAZER_A_SOMBRA
        && Vector3DotProduct(sol, solAnterior) > SOL_ANDOU) {
        return;   /* luzVP e o mapa continuam valendo */
    }
    meioAnterior = meio;
    solAnterior = sol;
    pistaAnterior = id;
    Camera3D luz = { 0 };
    luz.position = Vector3Add(meio, Vector3Scale(sol, ALTURA_DO_SOL));
    luz.target = meio;
    luz.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    luz.fovy = AREA_DA_SOMBRA;
    luz.projection = CAMERA_ORTHOGRAPHIC;

    rlDrawRenderBatchActive();
    BeginTextureMode(mapaDeSombra);
    rlClearScreenBuffers();
    rlSetClipPlanes(1.0, ALTURA_DO_SOL * 2.0);
    BeginMode3D(luz);
    Matrix vista = rlGetMatrixModelview();
    Matrix projecao = rlGetMatrixProjection();
    prepararCorte();   /* o mapa de sombra corta pelo quadrado da luz, não pela tela */
    rlDisableBackfaceCulling();
    desenharQuemFazSombra(id);
    rlEnableBackfaceCulling();
    EndMode3D();
    EndTextureMode();

    Matrix luzVP = MatrixMultiply(vista, projecao);
    for (int i = 0; i < totalQueRecebemSombra; i++) {
        SetShaderValueMatrix(recebemSombra[i], locLuzVP[i], luzVP);
    }
}

/*
 * desenharCenario
 * Desenha o cenário fixo da pista. Chamar entre BeginMode3D e EndMode3D.
 * Recebe: a pista e a posição da câmera (o céu e o fundo andam com ela).
 * Devolve: nada.
 */
/*
 * teloesAVista
 * Recebe: a pista e a câmera do jogador.
 * Devolve: 1 se alguma tela de telão pode aparecer na tela deste quadro — ou seja, se vale
 *          desenhar mais um quadro da transmissão. Chamar depois de prepararCorteDaCamera.
 */
int teloesAVista(IdPista id) {
    if ((int) id < 0 || (int) id >= TOTAL_PISTAS) {
        return 1;
    }
    if (totalDeTelas[id] == 0 || telasDemais[id]) {
        return 1;   /* não sabemos onde estão todos: na dúvida, desenha (como era antes) */
    }
    for (int i = 0; i < totalDeTelas[id]; i++) {
        if (caixaAVista(telasDoCenario[id][i])) {
            return 1;
        }
    }
    return 0;
}

/*
 * prepasseDoEstadio
 * O estádio só na profundidade (veja VERTICES_DO_PREPASSE), só as malhas à vista. Chamar dentro de BeginMode3D,
 * depois de prepararCorte e antes do desenho de verdade.
 * Recebe: nada. Devolve: nada.
 */
static void prepasseDoEstadio(void) {
    if (prepasseLigado < 0) {
        prepasseLigado = (getenv("KATCHAU_SEM_PREPASSE") == NULL);
    }
    if (!prepasseLigado || shaderDoPrepasseComCor.id == 0 || estadio.meshCount <= 0) {
        return;
    }
    Matrix noMundo = MatrixMultiply(estadio.transform,
                                    MatrixScale(ESCALA_COPA_PISTAO, ESCALA_COPA_PISTAO, ESCALA_COPA_PISTAO));
    rlDrawRenderBatchActive();
    rlColorMask(false, false, false, false);
    for (int i = 0; i < estadio.meshCount; i++) {
        if (caixasDoEstadio != NULL && !caixaAVista(caixasDoEstadio[i])) {
            continue;
        }
        Material material = estadio.materials[estadio.meshMaterial[i]];
        material.shader = shaderDoPrepasseComCor;
        DrawMesh(estadio.meshes[i], material, noMundo);
    }
    rlDrawRenderBatchActive();
    rlColorMask(true, true, true, true);
}

static void desenharEstadioCortado(void) {
    if (caixasDoEstadio == NULL) {
        DrawModel(estadio, (Vector3){ 0.0f, 0.0f, 0.0f }, ESCALA_COPA_PISTAO, WHITE);
        return;
    }
    Matrix noMundo = MatrixMultiply(estadio.transform,
                                    MatrixScale(ESCALA_COPA_PISTAO, ESCALA_COPA_PISTAO, ESCALA_COPA_PISTAO));
    for (int i = 0; i < estadio.meshCount; i++) {
        if (!caixaAVista(caixasDoEstadio[i])) {
            continue;
        }
        DrawMesh(estadio.meshes[i], estadio.materials[estadio.meshMaterial[i]], noMundo);
    }
}

/*
 * prepasseDoMundoAberto
 * O mapa do passeio só na profundidade (veja VERTICES_DO_PREPASSE). Chamar dentro de BeginMode3D, antes do
 * desenho de verdade.
 * Recebe: nada. Devolve: nada.
 */
static void prepasseDoMundoAberto(void) {
    if (prepasseLigado < 0) {
        prepasseLigado = (getenv("KATCHAU_SEM_PREPASSE") == NULL);
    }
    if (!prepasseLigado || !temMundoAberto || shaderDoPrepasse.id == 0) {
        return;
    }
    Shader originais[256];
    int total = mundoAberto.materialCount < 256 ? mundoAberto.materialCount : 256;
    for (int i = 0; i < total; i++) {
        originais[i] = mundoAberto.materials[i].shader;
        mundoAberto.materials[i].shader = shaderDoPrepasse;
    }
    rlDrawRenderBatchActive();
    rlColorMask(false, false, false, false);
    DrawModel(mundoAberto, (Vector3){ 0.0f, 0.0f, 0.0f }, 1.0f, WHITE);
    rlDrawRenderBatchActive();
    rlColorMask(true, true, true, true);
    for (int i = 0; i < total; i++) {
        mundoAberto.materials[i].shader = originais[i];
    }
}

void desenharCenario(IdPista id, Vector3 camera) {
    if (sombraLigadaAgora) {
        rlActiveTextureSlot(SLOT_DA_SOMBRA);
        rlEnableTexture(mapaDeSombra.depth.id);
        rlActiveTextureSlot(0);
    }
    Texture2D semTextura = { 0 };
    Matrix origem = MatrixIdentity();
    Matrix naCamera = MatrixTranslate(camera.x, camera.y, camera.z);

    if ((int) id == (int) CENARIO_DO_MUNDO_ABERTO) {
        prepasseDoMundoAberto();
        desenharCeuDoPasseio(naCamera);
        if (temMundoAberto) {
            DrawModel(mundoAberto, (Vector3){ 0.0f, 0.0f, 0.0f }, 1.0f, WHITE);
        }
        return;
    }

    if (id == PISTA_COPA_PISTAO) {
        prepasseDoEstadio();
        desenharMalha(ceuDeNoite, texturaDoCeuNoturno(), naCamera);
        desenharEstadioCortado();
        material.shader = shaderDaNoite();
        desenharMalha(pinturaDoEstadio, semTextura, origem);
        for (int i = primeiroLoteDoEstadio; i < fimDosLotesDoEstadio; i++) {
            desenharLote(i, origem);
        }
        material.shader = shader;
        return;
    }

    if (mapasDeDia[id].existe) {
        MapaDeDia *mapa = &mapasDeDia[id];
        desenharMalha(ceuDeDia, texturaCeu, naCamera);
        desenharMalha(tampaDoCeu, semTextura, naCamera);
        if (id == PISTA_INTERLAGOS) {
            desenharMalha(mapa->saia, texturaGrama, origem);
            desenharModeloDeInterlagos(mapa);
        } else {
            desenharModeloCortado(mapa);
        }
        desenharMalha(mapa->pintura, semTextura, origem);
        for (int i = mapa->primeiroLote; i < mapa->fimDosLotes; i++) {
            desenharLote(i, origem);
        }
        material.shader = shaderDaTorcida();
        desenharMalha(mapa->torcida, semTextura, origem);
        material.shader = shader;
    }
}

/*
 * prepararPlanos
 * Até onde a câmera enxerga: longe o bastante para o céu.
 * Recebe: nada.
 * Devolve: nada.
 */
void prepararPlanosDaCamera(void) {
    rlSetClipPlanes(PLANO_DE_PERTO, PLANO_DE_LONGE);
}
