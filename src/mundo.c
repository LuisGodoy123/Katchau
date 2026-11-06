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
#include <math.h>
#include "mundo.h"
#include "torcida3d.h"
#include "telao.h"
#include "tracado.h"
#include "oval.h"
#include "splat.h"
#include "cenario_mapas.h"
#include "construcao.h"
#include "paisagem.h"
#include "infraestrutura.h"
#include "pitstop.h"
#include "raymath.h"
#include "rlgl.h"
#include "obstaculos.h"
#include "noite.h"

#define PASTA_TEXTURAS        "recursos/texturas/"
#define ARQUIVO_DO_ESTADIO    "recursos/modelos/copa_pistao.glb"
#define COR_CEU_DO_ESTADIO    (Color){ 135, 190, 245, 255 }

/* ---------- Asfalto e pintura ---------- */
#define SEGMENTOS_POR_FAIXA   3         /* asfalto e zebra alternam a cada 3 segmentos */
#define REPETICOES_ASFALTO    4.0f      /* quantas vezes a textura do asfalto se repete na largura */
#define LARGURA_ZEBRA         0.15f     /* em larguras da pista, além da borda */
#define ALTURA_DA_PINTURA     0.04f     /* zebra e tinta ficam um pouco acima do asfalto */
#define LINHAS_DA_CHEGADA     2
#define COLUNAS_DA_CHEGADA    16
#define MEIA_LARGURA_COLCHETE 0.19f     /* colchete do grid, em larguras da pista */
#define FRENTE_COLCHETE       0.62f     /* barra da frente, em frações do segmento */
#define GROSSURA_COLCHETE     0.07f
#define GROSSURA_PERNA        0.025f
#define COR_TINTA             (Color){ 235, 235, 235, 255 }
#define COR_ZEBRA_CLARA       (Color){ 245, 245, 245, 255 }
#define COR_ZEBRA_ESCURA      (Color){ 200,  20,  20, 255 }
#define COR_LARGADA_CLARA     (Color){ 240, 240, 240, 255 }
#define COR_LARGADA_ESCURA    (Color){  25,  25,  28, 255 }

/* ---------- Terreno de Interlagos ---------- */
#define FAIXAS_DO_ASFALTO     8         /* na largura da pista */
#define JANELA_DA_BORRACHA    12        /* segmentos, para cada lado */
#define GANHO_DA_BORRACHA     1.1f
#define LARGURA_DA_BORRACHA   0.30f     /* em larguras */
#define FORCA_DA_BORRACHA     0.24f
#define LADO_DA_CELULA        30.0f     /* metros */
#define MARGEM_DO_TERRENO     1800.0f   /* quanto o terreno passa da pista */
#define PASSO_NA_PISTA        4         /* olha um a cada 4 segmentos para achar a altura */
#define SUAVIDADE_DO_TERRENO  900.0f
#define ABAIXO_DA_PISTA       0.6f      /* o terreno fica sempre abaixo da pista */
#define FOLGA_DO_TERRENO      40.0f     /* até esta distância da borda ele respeita a pista */
#define LARGURA_DO_ACOSTAMENTO 12.0f    /* faixa de grama que liga a borda da pista ao terreno */
#define METROS_POR_LADRILHO   12.0f
#define TAMANHO_DA_BASE       9000.0f   /* chão plano que cobre até o horizonte */

/* ---------- Objetos da beira de Interlagos (metros) ---------- */
#define PASTA_DOS_OBJETOS     "recursos/modelos/objetos/"
#define SEMENTE_DOS_OBJETOS   2026u     /* sorteio fixo: as árvores são sempre iguais */
#define PILHAS_POR_BARREIRA   7         /* a barreira vem a cada 6 segmentos (10 m): fica contínua */
#define SEGMENTOS_DA_BARREIRA 6
#define MENOR_ARVORE          0.82f     /* cada árvore tem um tamanho, em relação ao modelo */
#define MAIOR_ARVORE          1.22f
#define AFUNDAR_NO_CHAO       0.05f
#define MAXIMO_DE_LOTES       800
#define ARQUIVO_DO_OVAL       "recursos/modelos/oval.glb"
#define ARQUIVO_DO_SPLAT      "recursos/modelos/splat.glb"
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
#define MATERIAL_DO_MURO      23        /* Material.108 do estádio: o mesmo concreto... */
#define MATERIAL_DO_ALAMBRADO 25        /* ...e o mesmo alambrado (Material.110) */

/* ---------- Reta dos boxes de Interlagos (metros) ---------- */
#define TAMANHO_DO_PORTICO    0.85f     /* os pés ficam em cima dos muros, sem invadir a pista nem o pit lane */
#define ALTURA_DO_MURO        1.5f
#define ALTURA_DO_ALAMBRADO   4.5f
#define METROS_POR_TEXTURA    20.0f     /* ao longo do muro, a imagem se repete a cada tanto */
#define MODULOS_DOS_BOXES     7
#define METROS_DO_MODULO      60.0f
#define GARAGENS_POR_MODULO   6
#define MODULO_DA_TORRE       3         /* o do meio */
#define METROS_DA_PONTA       6.0f
#define FUNDO_DOS_BOXES       36.0f
#define FUNDO_DO_PATIO        13.0f     /* o pátio de asfalto atrás do prédio */
#define PATIO_ALEM_DO_PREDIO  14.0f     /* e a rua de serviço nas pontas, que liga o pátio ao pit lane */
#define PASSO_DO_PATIO        20.0f
#define ALTURA_DA_BORDA_DO_PATIO 1.5f  /* a viga da beira do pátio */
#define LAJE_DOS_BOXES        1.2f      /* a laje embaixo do prédio e do pátio: espessura */
#define PASSO_DOS_PILARES     20.0f     /* metros entre uma fila de pilares e a outra */
#define LADO_DO_PILAR         1.4f
#define LARGURA_DA_VIGA       0.9f
#define ALTURA_DA_VIGA        1.1f
#define VAO_PARA_TRAVAR       10.0f     /* pilares mais altos que isso ganham uma viga de travamento no meio */
#define ABAIXO_DO_TERRENO_PILAR 1.0f
#define COR_DO_PILAR          (Color){ 150, 150, 146, 255 }
#define COR_DA_VIGA           (Color){ 132, 132, 128, 255 }
#define COR_DA_LAJE           (Color){  74,  74,  72, 255 }
#define FUNDO_DA_VAGA         8.0f      /* a marca no chão na frente de cada garagem */
#define LARGURA_DA_VAGA       8.0f
#define LINHA_DO_PIT          0.25f
#define LINHA_DO_LIMITE       0.8f
#define LINHA_DA_LISTRA       0.35f      /* as listras da ilha entre a pista e o pit lane */
#define METROS_ENTRE_LISTRAS  4.0f
#define SEGMENTOS_DA_LISTRA   3
#define RAMPA_DO_TALUDE       2.5f      /* metros de rampa por metro de altura */
#define METROS_DA_GRAMA       12.0f     /* a imagem da grama se repete a cada tanto */
#define PILHAS_NA_PONTA       3
#define METROS_ENTRE_PILHAS   1.5f
#define FOLGA_DAS_PILHAS      0.8f      /* metros da ponta do muro para o lado da pista */
#define TRACO_DO_PIT          6.0f      /* traço e vão da linha que separa a pista de trabalho */
#define COR_DA_VAGA           (Color){ 232, 196,  40, 255 }
#define COR_DA_ZONA_DE_TRABALHO (Color){ 92, 92, 96, 255 }
#define COR_DA_FAIXA_DO_PIT   (Color){ 235, 235, 235, 255 }

/* ---------- Arquibancadas de Interlagos (metros) ---------- */
#define PASSO_DA_ARQUIBANCADA 6         /* segmentos entre um corte e outro: acompanha a curva */
#define ALTURA_DA_FRENTE      3.0f      /* do chão até a primeira fileira */
#define PASSARELA             1.4f      /* corredor na frente e atrás das fileiras */
#define FUNDO_DA_FILEIRA      0.85f
#define ALTURA_DA_FILEIRA     0.50f
#define METROS_POR_LUGAR      0.60f
#define FILEIRAS_POR_CARRINHO 3         /* a torcida 3D: um carrinho a cada tantas fileiras... */
#define METROS_ENTRE_CARRINHOS 2.0f     /* ...e a cada tantos metros ao longo delas */
#define CARRINHO_DE_INTERLAGOS 0.72f    /* do tamanho do modelo (3 m): cabe no fundo de 3 fileiras */
#define FILEIRAS_POR_CORREDOR 60.0f     /* a torcida se repete em blocos, com um corredor no meio */
#define ALTURA_DA_COBERTURA   5.0f      /* acima da última fileira */
#define VAO_ENTRE_COLUNAS     5         /* uma coluna a cada tantos cortes */
#define ABAIXO_DO_TERRENO     2.0f
#define COR_DO_CONCRETO       (Color){ 176, 176, 170, 255 }
#define COR_DO_FUNDO          (Color){ 120, 122, 126, 255 }
#define COR_DA_FAIXA          (Color){ 190,  28,  34, 255 }
#define COR_DA_COBERTURA      (Color){ 214, 216, 220, 255 }
#define COR_EMBAIXO_COBERTURA (Color){  70,  74,  82, 255 }
#define COR_DA_COLUNA         (Color){ 110, 114, 122, 255 }
#define SOMBRA_DA_COBERTURA   0.62f     /* a torcida embaixo da cobertura fica mais escura */
#define MATERIAL_DOS_ASSENTOS 7         /* Material.079 do estádio: a raylib põe o padrão na frente */
#define TOPO_DA_ARQUIBANCADA  10.3f     /* a placa do setor fica no alto do fundo */
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
 * mais longe do sol do que o que o mapa guardou ali está na sombra de alguma coisa */
#define GLSL_DA_SOMBRA \
    "uniform mat4 luzVP; uniform sampler2D mapaDeSombra; uniform float sombraLigada;\n" \
    "float naSombra(vec3 p) {\n" \
    "    if (sombraLigada < 0.5) return 0.0;\n" \
    "    vec3 n = normalize(cross(dFdx(p), dFdy(p)));\n" \
    "    if (n.y < 0.0) n = -n;\n" \
    "    float deFrente = dot(n, normalize(vec3(0.35, 0.8, 0.45)));\n" \
    "    vec4 q = luzVP * vec4(p + n * 0.15, 1.0);\n" \
    "    q.xyz = q.xyz / q.w * 0.5 + 0.5;\n" \
    "    if (q.x <= 0.0 || q.x >= 1.0 || q.y <= 0.0 || q.y >= 1.0 || q.z >= 1.0) return 0.0;\n" \
    "    float vies = 0.0008 + 0.0030 * (1.0 - clamp(deFrente, 0.0, 1.0));\n" \
    "    vec2 passo = 1.0 / vec2(textureSize(mapaDeSombra, 0));\n" \
    "    float s = 0.0;\n" \
    "    for (int x = -1; x <= 1; x++) for (int y = -1; y <= 1; y++)\n" \
    "        s += (q.z - vies > texture(mapaDeSombra, q.xy + vec2(x, y) * passo).r) ? 1.0 : 0.0;\n" \
    "    float borda = min(min(q.x, 1.0 - q.x), min(q.y, 1.0 - q.y));\n" \
    "    return s / 9.0 * smoothstep(0.0, 0.08, borda);\n" \
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
    GLSL_DA_SOMBRA
    "void main() { vec4 cor = texture(texture0, fragTexCoord) * colDiffuse * fragColor;\n"
    "              if (cor.a < 0.5) discard;\n"
    "              finalColor = vec4(cor.rgb * (1.0 - " FORCA_DA_SOMBRA " * naSombra(fragPos)), 1.0); }\n";

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

/* As malhas geradas para Interlagos */
typedef struct {
    Mesh asfalto[2];        /* [0] escuro, [1] claro */
    Mesh pintura;           /* zebras, linha de chegada e grid: cores nos vértices */
    Mesh acostamento;
    Mesh terreno;
    Mesh base;
    Mesh muro;              /* o muro de concreto da reta dos boxes */
    Mesh alambrado;         /* e o alambrado em cima dele */
    Mesh pitLane[2];        /* com os dois asfaltos da pista, nas mesmas faixas */
    Mesh taludes;           /* a grama descendo do pit lane até o terreno */
    Mesh marcasDoPit;
    Mesh estruturas;        /* arquibancadas: concreto, coberturas e colunas */
    Mesh torcida;           /* os lugares das arquibancadas */
} MalhasDaPista;

/* Os modelos .glb dos objetos de Interlagos */
typedef enum {
    MODELO_ARVORE_1,
    MODELO_ARVORE_2,
    MODELO_ARVORE_3,
    MODELO_PNEUS_VERMELHO,
    MODELO_PNEUS_BRANCO,
    MODELO_PLACA_50,
    MODELO_PLACA_100,
    MODELO_PLACA_150,
    MODELO_PORTICO,
    MODELO_ARQUIBANCADA,
    MODELO_CAMAROTE,
    MODELO_SETOR_A,
    MODELO_SETOR_B,
    MODELO_SETOR_D,
    MODELO_SETOR_G,
    MODELO_SETOR_H,
    MODELO_SETOR_M,
    MODELO_SETOR_N,
    MODELO_SETOR_R,
    MODELO_SETOR_S,
    MODELO_SETOR_V,
    MODELO_REFLETOR,
    MODELO_BOXES,
    MODELO_BOXES_PONTA,
    MODELO_BOXES_TORRE,
    MODELO_PINHEIRO_1,
    MODELO_PINHEIRO_2,
    MODELO_COPA_1,
    MODELO_COPA_2,
    MODELO_COPA_3,
    MODELO_TELAO_KATCHAU,
    MODELO_POSTE_RUA,
    MODELO_PRIMEIRO_DO_CENARIO,   /* na ordem de ModeloDoCenario (infraestrutura.h) */
    MODELO_FIM_DO_CENARIO = MODELO_PRIMEIRO_DO_CENARIO + TOTAL_DE_MODELOS_DO_CENARIO - 1,
    TOTAL_MODELOS_DE_OBJETO
} ModeloDeObjeto;

static const char *ARQUIVOS_DOS_OBJETOS[TOTAL_MODELOS_DE_OBJETO] = {
    "arvore_1.glb", "arvore_2.glb", "arvore_3.glb", "pneus_vermelho.glb", "pneus_branco.glb",
    "placa_50.glb", "placa_100.glb", "placa_150.glb", "portico.glb", "arquibancada.glb", "camarote.glb",
    "setor_A.glb", "setor_B.glb", "setor_D.glb", "setor_G.glb", "setor_H.glb",
    "setor_M.glb", "setor_N.glb", "setor_R.glb", "setor_S.glb", "setor_V.glb", "refletor.glb", "boxes.glb", "boxes_ponta.glb", "boxes_torre.glb",
    "pinheiro_1.glb", "pinheiro_2.glb", "copa_1.glb", "copa_2.glb", "copa_3.glb", "telao_katchau.glb", "poste_rua.glb",
    "posto_fiscal.glb", "torre_tv.glb", "passarela.glb", "telao.glb",
    "caminhao_1.glb", "caminhao_2.glb", "caminhao_3.glb", "caminhao_4.glb", "tenda_1.glb", "tenda_2.glb", "tenda_3.glb",
    "carro_rua_1.glb", "carro_rua_2.glb", "carro_rua_3.glb", "carro_rua_4.glb", "carro_rua_5.glb", "carro_rua_6.glb",
    "bancada_1.glb", "bancada_2.glb", "bancada_3.glb", "bancada_4.glb",
    "mastro_1.glb", "mastro_2.glb", "mastro_3.glb", "mastro_4.glb", "mastro_5.glb", "pilha_1.glb", "pilha_2.glb",
};

/* Centenas de cópias do mesmo modelo desenhadas uma a uma pesariam: elas são
 * juntadas em malhas únicas, os lotes, uma para cada textura */
typedef struct {
    Mesh malha;
    Texture2D textura;
    int brilha;      /* 1: a tela de um telão, que emite luz */
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
#define LADO_DO_MAPA_DE_SOMBRA 2048
#define AREA_DA_SOMBRA        330.0f   /* metros: o quadrado em volta da câmera que tem sombra */
#define ADIANTE_DA_SOMBRA     90.0f    /* metros: o meio do quadrado fica à frente da câmera */
#define ALTURA_DO_SOL         500.0f   /* metros: de onde o sol "olha" */
#define COR_DA_TELA_NO_HALO   (Color){ 215, 215, 215, 255 }   /* as telas brilham, mas menos que um refletor */
#define REFAZER_A_SOMBRA      25.0f   /* metros que a câmera anda até o mapa de sombra ser refeito */
#define SLOT_DA_SOMBRA        15       /* a unidade de textura do mapa (as do material vão até 11) */
static Shader shaderDaProfundidade;
static RenderTexture2D mapaDeSombra;
static int temMapaDeSombra = 0;
static int sombraLigadaAgora = 0;
static void carregarSombras(void);
static void desenharLote(int i, Matrix origem);

/* As pistas de dia vêm com as imagens sem luz nenhuma: aqui entra o mesmo sol de luzDoSol,
 * com a direção da superfície saindo das derivadas da posição */
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
    GLSL_DA_SOMBRA
    "void main() { vec4 cor = texture(texture0, fragTexCoord) * colDiffuse;\n"
    "              if (cor.a < 0.5) discard;\n"
    "              cor.rgb *= 1.0 - " FORCA_DA_SOMBRA " * naSombra(fragPos);\n"
    "              vec3 n = normalize(cross(dFdx(fragPos), dFdy(fragPos)));\n"
    "              if (n.y < 0.0) n = -n;\n"
    "              float luz = 0.58 + 0.30 * (n.y * 0.5 + 0.5) + 0.38 * max(dot(n, normalize(vec3(0.35, 0.8, 0.45))), 0.0);\n"
    "              finalColor = vec4(cor.rgb * luz * 0.92, 1.0); }\n";
static Material material;   /* o mesmo material, trocando a textura a cada desenho */
static Model estadio;
static Mesh pinturaDoEstadio;
static MalhasDaPista interlagos;
static Mesh ceuDeDia, tampaDoCeu, ceuDeNoite;
static Texture2D texturaCeu;
static Texture2D texturaAsfalto[2], texturaGrama;
static Model modelosDosObjetos[TOTAL_MODELOS_DE_OBJETO];
static LoteDeObjetos lotes[MAXIMO_DE_LOTES];
static int totalDeLotes = 0;
static int fimDosLotesDoEstadio = 0;
static int lotesDeInterlagos = 0;   /* os primeiros lotes são de Interlagos; os demais, do estádio */

/* As pistas de dia que vêm prontas em um modelo 3D: só falta o que é do jogo */
typedef struct {
    int existe;
    Model modelo;
    float escala;
    Mesh pintura;    /* a linha, o grid e as vagas */
    Mesh torcida;
    int primeiroLote, fimDosLotes;   /* as árvores e as estruturas */
} MapaDeDia;
static MapaDeDia mapasDeDia[TOTAL_PISTAS];

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

/* ==================================================================
 * Interlagos: malhas geradas a partir da lista
 * ================================================================== */

static void construirObjetos(Pista *pista);

/* A grade de alturas do terreno de Interlagos: existe só durante a construção,
 * para os objetos pousarem exatamente no chão que é desenhado */
static float *alturasDaGrade = NULL;
static float origemDaGradeX, origemDaGradeZ;
static int colunasDaGrade, linhasDaGrade;
static float fimDaGradeX, fimDaGradeZ;
static const MalhasDaInfraestrutura *infra = NULL;

/*
 * alturaDoTerreno
 * A altura do terreno num ponto: uma média das alturas da pista, pesando
 * mais os trechos perto. Perto da pista ele fica sempre abaixo dela, para
 * não cobrir o asfalto.
 * Recebe: a pista e o ponto (x, z).
 * Devolve: a altura, em metros.
 */
static float alturaDoTerreno(Pista *pista, float x, float z) {
    float somaPesos = 0.0f, soma = 0.0f, limite = 1e9f;
    Segmento *atual = pista->largada;
    int indice = 0;
    do {
        if (indice++ % PASSO_NA_PISTA == 0) {
            float dx = atual->xMundo - x, dz = atual->zMundo - z;
            float d2 = dx * dx + dz * dz;
            float peso = 1.0f / ((d2 + SUAVIDADE_DO_TERRENO) * (d2 + SUAVIDADE_DO_TERRENO));
            float altura = atual->altura / UNIDADES_POR_METRO;
            somaPesos += peso;
            soma += peso * altura;
            float alcance = atual->larguraDireita + FOLGA_DO_TERRENO;
            if (d2 < alcance * alcance && altura - ABAIXO_DA_PISTA < limite) {
                limite = altura - ABAIXO_DA_PISTA;
            }
        }
        atual = atual->proximo;
    } while (atual != pista->largada);
    return fminf(alturaComLago(soma / somaPesos, x, z), limite) + relevoDaPaisagem(x, z);
}

/*
 * alturaNaGrade
 * A altura exata do terreno desenhado: cada célula são dois triângulos, com a
 * diagonal do canto de menor x e z ao canto oposto (como em construirTerreno).
 * Recebe: o ponto (x, z).
 * Devolve: a altura, em metros.
 */
static float alturaNaGrade(float x, float z) {
    float cx = (x - origemDaGradeX) / LADO_DA_CELULA, cz = (z - origemDaGradeZ) / LADO_DA_CELULA;
    int k = (int) fminf(fmaxf(cx, 0.0f), colunasDaGrade - 1.0f);
    int l = (int) fminf(fmaxf(cz, 0.0f), linhasDaGrade - 1.0f);
    float fx = cx - k, fz = cz - l;
    int largura = colunasDaGrade + 1;
    float a = alturasDaGrade[l * largura + k];               /* x0, z0 */
    float b = alturasDaGrade[(l + 1) * largura + k];         /* x0, z1 */
    float d = alturasDaGrade[(l + 1) * largura + k + 1];     /* x1, z1 */
    float e = alturasDaGrade[l * largura + k + 1];           /* x1, z0 */
    if (fz >= fx) {
        return a + (d - b) * fx + (b - a) * fz;   /* triângulo a, b, d */
    }
    return a + (e - a) * fx + (d - e) * fz;       /* triângulo a, d, e */
}

/*
 * construirTerreno
 * Uma grade em volta da pista, com a altura vinda de alturaDoTerreno: grama dentro do
 * autódromo e, fora dele, o chão da cidade (paisagem.c).
 * Recebe: a pista.
 * Devolve: a malha.
 */
static Mesh construirTerreno(Pista *pista) {
    float minX = 1e9f, maxX = -1e9f, minZ = 1e9f, maxZ = -1e9f;
    Segmento *atual = pista->largada;
    do {
        minX = fminf(minX, atual->xMundo);
        maxX = fmaxf(maxX, atual->xMundo);
        minZ = fminf(minZ, atual->zMundo);
        maxZ = fmaxf(maxZ, atual->zMundo);
        atual = atual->proximo;
    } while (atual != pista->largada);
    minX -= MARGEM_DO_TERRENO;
    minZ -= MARGEM_DO_TERRENO;
    int colunas = (int) ((maxX + MARGEM_DO_TERRENO - minX) / LADO_DA_CELULA);
    int linhas = (int) ((maxZ + MARGEM_DO_TERRENO - minZ) / LADO_DA_CELULA);

    float *alturas = MemAlloc((linhas + 1) * (colunas + 1) * sizeof(float));
    alturasDaGrade = alturas;
    origemDaGradeX = minX;
    origemDaGradeZ = minZ;
    colunasDaGrade = colunas;
    linhasDaGrade = linhas;
    for (int l = 0; l <= linhas; l++) {
        for (int k = 0; k <= colunas; k++) {
            alturas[l * (colunas + 1) + k] = alturaDoTerreno(pista, minX + k * LADO_DA_CELULA, minZ + l * LADO_DA_CELULA);
        }
    }

    fimDaGradeX = minX + colunas * LADO_DA_CELULA;
    fimDaGradeZ = minZ + linhas * LADO_DA_CELULA;
    Construcao c = { 0 };
    for (int l = 0; l < linhas; l++) {
        for (int k = 0; k < colunas; k++) {
            float x0 = minX + k * LADO_DA_CELULA, x1 = x0 + LADO_DA_CELULA;
            float z0 = minZ + l * LADO_DA_CELULA, z1 = z0 + LADO_DA_CELULA;
            Vector3 a = { x0, alturas[l * (colunas + 1) + k], z0 };
            Vector3 b = { x0, alturas[(l + 1) * (colunas + 1) + k], z1 };
            Vector3 d = { x1, alturas[(l + 1) * (colunas + 1) + k + 1], z1 };
            Vector3 e = { x1, alturas[l * (colunas + 1) + k + 1], z0 };
            Vector2 ua = { x0 / METROS_POR_LADRILHO, z0 / METROS_POR_LADRILHO };
            Vector2 ub = { x0 / METROS_POR_LADRILHO, z1 / METROS_POR_LADRILHO };
            Vector2 ud = { x1 / METROS_POR_LADRILHO, z1 / METROS_POR_LADRILHO };
            Vector2 ue = { x1 / METROS_POR_LADRILHO, z0 / METROS_POR_LADRILHO };
            /* Fora do autódromo é a cidade; dentro, a grama, com manchas em vez de um xadrez */
            if (foraDoAutodromo(x0 + LADO_DA_CELULA / 2.0f, z0 + LADO_DA_CELULA / 2.0f)) {
                adicionarChaoDaCidade(a, b, d, e);
                continue;
            }
            adicionarVertice(&c, a, ua, corDaGramaNoAutodromo(a.x, a.z));
            adicionarVertice(&c, b, ub, corDaGramaNoAutodromo(b.x, b.z));
            adicionarVertice(&c, d, ud, corDaGramaNoAutodromo(d.x, d.z));
            adicionarVertice(&c, a, ua, corDaGramaNoAutodromo(a.x, a.z));
            adicionarVertice(&c, d, ud, corDaGramaNoAutodromo(d.x, d.z));
            adicionarVertice(&c, e, ue, corDaGramaNoAutodromo(e.x, e.z));
        }
    }
    return terminarMalha(&c);   /* a grade fica para os objetos; construirObjetos a devolve */
}

/*
 * linhaDeBorracha
 * Por onde os carros passam: para dentro nas curvas, e no meio nas retas.
 * Recebe: o segmento.
 * Devolve: a lateral, em larguras.
 */
static float linhaDeBorracha(Segmento *segmento) {
    float soma = 0.0f;
    Segmento *s = segmento;
    for (int i = 0; i < JANELA_DA_BORRACHA; i++) {
        s = s->proximo;
    }
    for (int i = 0; i < JANELA_DA_BORRACHA * 2; i++) {
        soma += s->curvatura;
        s = segmentoAnterior(s);
    }
    return fminf(fmaxf(soma / (JANELA_DA_BORRACHA * 2) * GANHO_DA_BORRACHA, -0.6f), 0.6f);
}

/*
 * tomDoAsfalto
 * Recebe: a lateral e onde está a linha de borracha.
 * Devolve: a cor que vai multiplicar a imagem do asfalto.
 */
static Color tomDoAsfalto(float lateral, float linha) {
    float d = (lateral - linha) / LARGURA_DA_BORRACHA;
    float t = 1.0f - FORCA_DA_BORRACHA * expf(-d * d);
    unsigned char v = (unsigned char) (255.0f * t);
    return (Color){ v, v, v, 255 };
}

/*
 * construirInterlagos
 * Anda pela lista criando, para cada segmento, o asfalto, as zebras e o
 * acostamento de grama que liga a borda ao terreno.
 * Recebe: a pista.
 * Devolve: nada (preenche "interlagos").
 */
static void construirInterlagos(Pista *pista) {
    Construcao asfalto[2] = { { 0 }, { 0 } };
    Construcao pintura = { 0 }, acostamento = { 0 };

    Segmento *atual = pista->largada;
    do {
        int claro = (atual->indice / SEGMENTOS_POR_FAIXA) % 2;
        /* Em faixas na largura: onde os carros passam, a borracha escurece o asfalto */
        float linha0 = linhaDeBorracha(atual), linha1 = linhaDeBorracha(atual->proximo);
        for (int f = 0; f < FAIXAS_DO_ASFALTO; f++) {
            float l0 = -1.0f + 2.0f * f / FAIXAS_DO_ASFALTO, l1 = -1.0f + 2.0f * (f + 1) / FAIXAS_DO_ASFALTO;
            float u0 = (l0 + 1.0f) / 2.0f * REPETICOES_ASFALTO, u1 = (l1 + 1.0f) / 2.0f * REPETICOES_ASFALTO;
            Vector3 p[4] = { pontoNaEstrada(atual, 0.0f, l0, 0.0f), pontoNaEstrada(atual, 0.0f, l1, 0.0f),
                             pontoNaEstrada(atual, 1.0f, l1, 0.0f), pontoNaEstrada(atual, 1.0f, l0, 0.0f) };
            Vector2 uv[4] = { { u0, 1.0f }, { u1, 1.0f }, { u1, 0.0f }, { u0, 0.0f } };
            Color cor[4] = { tomDoAsfalto(l0, linha0), tomDoAsfalto(l1, linha0), tomDoAsfalto(l1, linha1), tomDoAsfalto(l0, linha1) };
            int ordem[6] = { 0, 1, 2, 0, 2, 3 };
            for (int v = 0; v < 6; v++) {
                adicionarVertice(&asfalto[claro], p[ordem[v]], uv[ordem[v]], cor[ordem[v]]);
            }
        }

        float borda = 1.0f;
        if (temZebra(atual)) {
            Color cor = claro ? COR_ZEBRA_CLARA : COR_ZEBRA_ESCURA;
            pintarRetangulo(&pintura, atual, 0.0f, 1.0f, -1.0f - LARGURA_ZEBRA, -1.0f, cor);
            pintarRetangulo(&pintura, atual, 0.0f, 1.0f, 1.0f, 1.0f + LARGURA_ZEBRA, cor);
            borda += LARGURA_ZEBRA;
        }

        for (int lado = -1; lado <= 1; lado += 2) {
            float fora = lado * (borda + LARGURA_DO_ACOSTAMENTO / atual->larguraDireita);
            Vector3 p0 = pontoNaEstrada(atual, 0.0f, lado * borda, -0.02f);
            Vector3 p1 = pontoNaEstrada(atual, 0.0f, fora, -ABAIXO_DA_PISTA);
            Vector3 p2 = pontoNaEstrada(atual, 1.0f, fora, -ABAIXO_DA_PISTA);
            Vector3 p3 = pontoNaEstrada(atual, 1.0f, lado * borda, -0.02f);
            adicionarQuadrilatero(&acostamento, p0, p1, p2, p3,
                                  (Vector2){ p0.x / METROS_POR_LADRILHO, p0.z / METROS_POR_LADRILHO },
                                  (Vector2){ p1.x / METROS_POR_LADRILHO, p1.z / METROS_POR_LADRILHO },
                                  (Vector2){ p2.x / METROS_POR_LADRILHO, p2.z / METROS_POR_LADRILHO },
                                  (Vector2){ p3.x / METROS_POR_LADRILHO, p3.z / METROS_POR_LADRILHO }, WHITE);
        }
        atual = atual->proximo;
    } while (atual != pista->largada);
    pintarLargadaEGrid(&pintura, pista);

    interlagos.asfalto[0] = terminarMalha(&asfalto[0]);
    interlagos.asfalto[1] = terminarMalha(&asfalto[1]);
    interlagos.pintura = terminarMalha(&pintura);
    interlagos.acostamento = terminarMalha(&acostamento);
    prepararPaisagem(pista);
    float lagoX, lagoZ;
    lugarDoLago(&lagoX, &lagoZ);
    definirNivelDoLago(alturaDoTerreno(pista, lagoX, lagoZ));
    interlagos.terreno = construirTerreno(pista);
    construirPaisagem(pista, alturaDoTerreno, origemDaGradeX, fimDaGradeX, origemDaGradeZ, fimDaGradeZ);

    /* Um chão plano, bem abaixo, cobre o que o terreno não alcança */
    float menor = 1e9f;
    do {
        menor = fminf(menor, atual->altura / UNIDADES_POR_METRO);
        atual = atual->proximo;
    } while (atual != pista->largada);
    float y = menor - 30.0f, t = TAMANHO_DA_BASE, r = t / METROS_POR_LADRILHO;
    Construcao base = { 0 };
    adicionarQuadrilatero(&base, (Vector3){ -t, y, -t }, (Vector3){ -t, y, t }, (Vector3){ t, y, t }, (Vector3){ t, y, -t },
                          (Vector2){ -r, -r }, (Vector2){ -r, r }, (Vector2){ r, r }, (Vector2){ r, -r }, WHITE);
    interlagos.base = terminarMalha(&base);

    construirObjetos(pista);
}

/* ==================================================================
 * Objetos 3D de Interlagos: modelos .glb juntados em lotes
 * ================================================================== */

/* Sorteio próprio, como em obstaculos.c: a mesma semente dá sempre as mesmas árvores */
static unsigned int sementeDosObjetos;

/*
 * sortearObjeto
 * Recebe: os limites.
 * Devolve: um número entre eles.
 */
static float sortearObjeto(float minimo, float maximo) {
    sementeDosObjetos = sementeDosObjetos * 1103515245u + 12345u;
    return minimo + (maximo - minimo) * ((sementeDosObjetos >> 16) & 0x7FFF) / 32767.0f;
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
 * adicionarBarreira
 * Uma barreira de pneus acompanhando a pista: as pilhas seguem a curva, cada
 * uma no seu lugar, com o topo vermelho e branco alternando.
 * Recebe: o segmento e a lateral da barreira.
 * Devolve: nada.
 */
static void adicionarBarreira(Segmento *segmento, float lateral) {
    for (int i = 0; i < PILHAS_POR_BARREIRA; i++) {
        PosicaoNaPista lugar = { segmento, 0.0f };
        moverNaPista(&lugar, (float) i / PILHAS_POR_BARREIRA * SEGMENTOS_DA_BARREIRA * COMPRIMENTO_SEGMENTO);
        PontoNoMundo ponto = pontoNoMundo(lugar, lateral);
        Vector3 pe = { ponto.x, alturaNaGrade(ponto.x, ponto.z) - AFUNDAR_NO_CHAO, ponto.z };
        int vermelho = (segmento->indice / SEGMENTOS_DA_BARREIRA + i) % 2;
        colocarModelo(vermelho ? MODELO_PNEUS_VERMELHO : MODELO_PNEUS_BRANCO, pe, sortearObjeto(0.0f, 2.0f * PI), 1.0f);
    }
}

/*
 * modeloDoSetor
 * Recebe: a letra do setor.
 * Devolve: o modelo da placa dele.
 */
static ModeloDeObjeto modeloDoSetor(char letra) {
    static const char LETRAS[] = "ABDGHMNRSV";
    for (int i = 0; LETRAS[i] != '\0'; i++) {
        if (LETRAS[i] == letra) {
            return (ModeloDeObjeto) (MODELO_SETOR_A + i);
        }
    }
    return MODELO_SETOR_A;
}


/* Um corte da arquibancada: onde a frente dela está, e para onde é "para fora" */
typedef struct {
    Vector3 frente;
    Vector3 fora;
    float metros;     /* ao longo da pista */
} CorteDaArquibancada;

/*
 * noCorte
 * Recebe: o corte e quanto andar para fora e para cima.
 * Devolve: o ponto.
 */
static Vector3 noCorte(CorteDaArquibancada corte, float fora, float cima) {
    return (Vector3){ corte.frente.x + corte.fora.x * fora, corte.frente.y + cima, corte.frente.z + corte.fora.z * fora };
}

/*
 * faixaEntreCortes
 * Uma faixa da arquibancada entre dois cortes vizinhos.
 * Recebe: a construção, os cortes, os dois pontos do perfil (fora e cima) e a cor.
 * Devolve: nada.
 */
static void faixaEntreCortes(Construcao *c, CorteDaArquibancada a, CorteDaArquibancada b,
                             float fora0, float cima0, float fora1, float cima1, Color cor) {
    quadIluminado(c, noCorte(a, fora0, cima0), noCorte(a, fora1, cima1), noCorte(b, fora1, cima1), noCorte(b, fora0, cima0), cor);
}

/*
 * lugaresNaFileira
 * Os lugares da torcida 3D num pedaço de arquibancada de Interlagos: um carrinho a cada
 * FILEIRAS_POR_CARRINHO fileiras e a cada METROS_ENTRE_CARRINHOS ao longo delas, de frente para a pista.
 * Recebe: os dois cortes do pedaço e quantas fileiras a arquibancada tem.
 * Devolve: nada.
 */
static void lugaresNaFileira(CorteDaArquibancada anterior, CorteDaArquibancada b, int fileiras) {
    float comprimento = Vector3Distance(anterior.frente, b.frente);
    int quantos = (int) (comprimento / METROS_ENTRE_CARRINHOS + 0.5f);
    for (int f = FILEIRAS_POR_CARRINHO / 2; f < fileiras; f += FILEIRAS_POR_CARRINHO) {
        float fora = PASSARELA + (f + 0.5f) * FUNDO_DA_FILEIRA, cima = ALTURA_DA_FRENTE + (f + 0.5f) * ALTURA_DA_FILEIRA;
        Vector3 p0 = noCorte(anterior, fora, cima), p1 = noCorte(b, fora, cima);
        for (int j = 0; j < quantos; j++) {
            float t = (j + 0.5f) / quantos;
            Vector3 lugar = Vector3Lerp(p0, p1, t);
            Vector3 paraAPista = Vector3Negate(Vector3Lerp(anterior.fora, b.fora, t));
            adicionarLugarTorcida3D(PISTA_INTERLAGOS, lugar.x, lugar.y, lugar.z, atan2f(paraAPista.x, paraAPista.z));
        }
    }
}

/*
 * construirUmaArquibancada
 * Levanta uma arquibancada contínua, cortada em fatias ao longo da pista: muro da
 * frente com a faixa colorida, corredor, fileiras com a torcida, corredor de trás,
 * parede de fundo e, se tiver, cobertura em cima de colunas. A placa com a letra
 * fica no alto, no meio.
 * Recebe: a pista, as construções da estrutura e da torcida, e a arquibancada.
 * Devolve: nada.
 */
static void construirUmaArquibancada(Pista *pista, Construcao *estrutura, Construcao *torcida, const Arquibancada *a) {
    int primeiro = (int) (a->inicio * SEGMENTOS_POR_METRO);
    int ultimo = (int) (a->fim * SEGMENTOS_POR_METRO);
    float lateral = a->distancia / pista->largada->larguraDireita;
    float fundoDasFileiras = a->fileiras * FUNDO_DA_FILEIRA, alturaDasFileiras = a->fileiras * ALTURA_DA_FILEIRA;
    float foraDoFundo = PASSARELA * 2.0f + fundoDasFileiras;
    float topo = ALTURA_DA_FRENTE + alturaDasFileiras;
    float cobertura = topo + ALTURA_DA_COBERTURA;
    float sombra = a->coberta ? SOMBRA_DA_COBERTURA : 1.0f;
    Color corDaTorcida = { (unsigned char) (255 * sombra), (unsigned char) (255 * sombra), (unsigned char) (255 * sombra), 255 };

    CorteDaArquibancada anterior = { 0 };
    int fatia = 0;
    for (int i = primeiro; i <= ultimo; i += PASSO_DA_ARQUIBANCADA, fatia++) {
        PosicaoNaPista lugar = { buscarSegmento(pista, i), 0.0f };
        PontoNoMundo ponto = pontoNoMundo(lugar, lateral);
        float rumo = ponto.rumo * DEG2RAD;
        CorteDaArquibancada corte;
        corte.fora = (Vector3){ cosf(rumo), 0.0f, sinf(rumo) };
        corte.frente = (Vector3){ ponto.x, alturaNaGrade(ponto.x, ponto.z), ponto.z };
        corte.metros = i / SEGMENTOS_POR_METRO;

        if (fatia > 0) {
            CorteDaArquibancada b = corte;
            /* Muro da frente, com a faixa colorida no alto */
            faixaEntreCortes(estrutura, anterior, b, 0.0f, -ABAIXO_DO_TERRENO, 0.0f, ALTURA_DA_FRENTE - 1.1f, COR_DO_CONCRETO);
            faixaEntreCortes(estrutura, anterior, b, 0.0f, ALTURA_DA_FRENTE - 1.1f, 0.0f, ALTURA_DA_FRENTE, COR_DA_FAIXA);
            faixaEntreCortes(estrutura, anterior, b, 0.0f, ALTURA_DA_FRENTE, PASSARELA, ALTURA_DA_FRENTE, COR_DO_FUNDO);

            /* As fileiras: a torcida, com um lugar a cada METROS_POR_LUGAR */
            float u0 = anterior.metros / METROS_POR_LUGAR / FILEIRAS_POR_CORREDOR;
            float u1 = corte.metros / METROS_POR_LUGAR / FILEIRAS_POR_CORREDOR;
            float v = a->fileiras / 52.0f;
            lugaresNaFileira(anterior, b, a->fileiras);
            adicionarQuadrilatero(torcida, noCorte(anterior, PASSARELA, ALTURA_DA_FRENTE), noCorte(anterior, PASSARELA + fundoDasFileiras, topo),
                                  noCorte(b, PASSARELA + fundoDasFileiras, topo), noCorte(b, PASSARELA, ALTURA_DA_FRENTE),
                                  (Vector2){ u0, 0.0f }, (Vector2){ u0, v }, (Vector2){ u1, v }, (Vector2){ u1, 0.0f }, corDaTorcida);

            /* Corredor de trás, parede de fundo e o lado de fora */
            faixaEntreCortes(estrutura, anterior, b, PASSARELA + fundoDasFileiras, topo, foraDoFundo, topo, COR_DO_FUNDO);
            faixaEntreCortes(estrutura, anterior, b, foraDoFundo, topo, foraDoFundo, topo + 2.2f, COR_DO_CONCRETO);
            faixaEntreCortes(estrutura, anterior, b, foraDoFundo, -ABAIXO_DO_TERRENO, foraDoFundo, topo, COR_DO_FUNDO);

            if (a->coberta) {
                float balanco = PASSARELA + fundoDasFileiras * 0.25f;
                faixaEntreCortes(estrutura, anterior, b, foraDoFundo + 0.6f, topo + 2.2f, balanco, cobertura + 0.5f, COR_DA_COBERTURA);
                faixaEntreCortes(estrutura, anterior, b, balanco, cobertura + 0.1f, foraDoFundo, topo + 1.9f, COR_EMBAIXO_COBERTURA);
                faixaEntreCortes(estrutura, anterior, b, balanco, cobertura - 0.5f, balanco, cobertura + 0.5f, COR_DA_FAIXA);
            }
        }

        /* As colunas da cobertura */
        if (a->coberta && fatia % VAO_ENTRE_COLUNAS == 0) {
            float lados[4][2] = { { -0.25f, -0.25f }, { 0.25f, -0.25f }, { 0.25f, 0.25f }, { -0.25f, 0.25f } };
            Vector3 aoLongo = { -corte.fora.z, 0.0f, corte.fora.x };
            for (int l = 0; l < 4; l++) {
                int m = (l + 1) % 4;
                Vector3 c0 = noCorte(corte, foraDoFundo - 0.3f + lados[l][0], topo), c1 = noCorte(corte, foraDoFundo - 0.3f + lados[m][0], topo);
                c0 = Vector3Add(c0, Vector3Scale(aoLongo, lados[l][1]));
                c1 = Vector3Add(c1, Vector3Scale(aoLongo, lados[m][1]));
                Vector3 t0 = { c0.x, c0.y + cobertura - topo - 1.0f, c0.z }, t1 = { c1.x, c1.y + cobertura - topo - 1.0f, c1.z };
                quadIluminado(estrutura, c0, c1, t1, t0, COR_DA_COLUNA);
            }
        }

        /* As pontas: uma parede de concreto fechando cada lado */
        if (fatia == 0 || i + PASSO_DA_ARQUIBANCADA > ultimo) {
            quadIluminado(estrutura, noCorte(corte, 0.0f, -ABAIXO_DO_TERRENO), noCorte(corte, foraDoFundo, -ABAIXO_DO_TERRENO),
                          noCorte(corte, foraDoFundo, topo + 2.2f), noCorte(corte, 0.0f, ALTURA_DA_FRENTE), COR_DO_CONCRETO);
        }

        /* A placa do setor, no alto, no meio da arquibancada */
        if (a->setor != 0 && i == primeiro + (ultimo - primeiro) / PASSO_DA_ARQUIBANCADA / 2 * PASSO_DA_ARQUIBANCADA) {
            Vector3 frenteDaPlaca = Vector3Negate(corte.fora);
            colocarModelo(modeloDoSetor(a->setor), noCorte(corte, foraDoFundo - 0.5f, a->coberta ? cobertura + 0.5f : topo + 1.5f),
                          atan2f(frenteDaPlaca.x, frenteDaPlaca.z), 1.0f);
        }
        anterior = corte;
    }
}

static void construirRetaDosBoxes(Pista *pista);

/*
 * construirArquibancadas
 * Recebe: a pista.
 * Devolve: nada (preenche as malhas das arquibancadas).
 */
static void construirArquibancadas(Pista *pista) {
    Construcao estrutura = { 0 }, torcida = { 0 };
    int total;
    const Arquibancada *tabela = arquibancadasDaPista(&total);
    for (int i = 0; i < total; i++) {
        if (tabela[i].tipo == OBJETO_ARQUIBANCADA) {
            construirUmaArquibancada(pista, &estrutura, &torcida, &tabela[i]);
        }
    }
    construirRetaDosBoxes(pista);
    interlagos.estruturas = terminarMalha(&estrutura);
    interlagos.torcida = terminarMalha(&torcida);
}

/*
 * noChaoDoPit
 * O chão do pit lane: acompanha a pista, mas não fica abaixo do terreno.
 * Recebe: o segmento e o lado (negativo: esquerda), em larguras.
 * Devolve: o ponto, um pouco acima do chão.
 */
static Vector3 noChaoDoPit(Segmento *segmento, float lateral) {
    Vector3 ponto = pontoNaEstrada(segmento, 0.0f, lateral, 0.0f);
    float acimaDoAsfalto = fabsf(lateral) <= 1.02f ? -0.02f : 0.04f;
    ponto.y = fmaxf(ponto.y + acimaDoAsfalto, (fabsf(lateral) > 1.5f) ? alturaNaGrade(ponto.x, ponto.z) + 0.04f : ponto.y + acimaDoAsfalto);
    return ponto;
}

/*
 * pintarFaixaDoPit
 * Uma linha pintada entre dois pontos no chão.
 * Recebe: a construção, os pontos, a largura e a cor.
 * Devolve: nada.
 */
static void pintarFaixaDoPit(Construcao *marcas, Vector3 a, Vector3 b, float largura, Color cor) {
    Vector3 lado = Vector3Scale(Vector3Normalize(Vector3CrossProduct(Vector3Subtract(b, a), (Vector3){ 0.0f, 1.0f, 0.0f })), largura / 2.0f);
    Vector3 subir = { 0.0f, 0.03f, 0.0f };
    Vector2 zero = { 0.0f, 0.0f };
    a = Vector3Add(a, subir);
    b = Vector3Add(b, subir);
    adicionarQuadrilatero(marcas, Vector3Subtract(a, lado), Vector3Add(a, lado), Vector3Add(b, lado), Vector3Subtract(b, lado), zero, zero, zero, zero, cor);
}

/*
 * noPredioDosBoxes
 * Recebe: os metros a partir da linha (negativo: antes dela) e a distância para fora da
 *         frente do prédio, em metros.
 * Devolve: o ponto na altura da pista.
 */
static Vector3 noPredioDosBoxes(Pista *pista, float metros, float paraFora) {
    PosicaoNaPista lugar = { pista->largada, 0.0f };
    moverNaPista(&lugar, metros * UNIDADES_POR_METRO);
    float largura = lugar.segmento->larguraEsquerda;
    PontoNoMundo ponto = pontoNoMundo(lugar, -1.0f - (LARGURA_DO_PIT_LANE + paraFora) / largura);
    return (Vector3){ ponto.x, ponto.y, ponto.z };
}

/*
 * blocoDeConcreto
 * Uma peça comprida de seção retangular: um pilar (de pé) ou uma viga (deitada).
 * Recebe: a construção, os centros das duas pontas, dois vetores perpendiculares à
 *         peça, com o tamanho da meia seção, e a cor.
 * Devolve: nada.
 */
static void blocoDeConcreto(Construcao *c, Vector3 a, Vector3 b, Vector3 u, Vector3 v, Color cor) {
    Vector3 cantos[4] = { Vector3Add(u, v), Vector3Subtract(u, v), Vector3Negate(Vector3Add(u, v)), Vector3Subtract(v, u) };
    for (int i = 0; i < 4; i++) {
        Vector3 c0 = cantos[i], c1 = cantos[(i + 1) % 4];
        quadIluminado(c, Vector3Add(a, c0), Vector3Add(a, c1), Vector3Add(b, c1), Vector3Add(b, c0), cor);
    }
}

/*
 * construirEstruturaDosBoxes
 * O que sustenta o prédio e o pátio onde o terreno desce: a laje fechada por baixo,
 * filas de pilares até o chão, vigas nas duas direções e, nos pilares altos, uma
 * viga de travamento no meio da altura.
 * Recebe: a pista, a construção, o começo e o fim (metros a partir da linha) e até onde
 *         a estrutura vai para fora.
 * Devolve: nada.
 */
static void construirEstruturaDosBoxes(Pista *pista, Construcao *c, float inicio, float fim, float fora) {
    static const float LINHAS[] = { 1.0f, 12.0f, 24.0f, 36.0f };   /* distância da frente do prédio; a última é a beira do pátio */
    enum { TOTAL_DE_LINHAS = 5 };
    Vector3 anteriorTopo[TOTAL_DE_LINHAS], anteriorChao[TOTAL_DE_LINHAS];
    int fila = 0;

    for (float a = inicio; a <= fim + 0.01f; a += PASSO_DOS_PILARES, fila++) {
        float aqui = fminf(a, fim);
        Vector3 frente = noPredioDosBoxes(pista, aqui, 0.0f), longe = noPredioDosBoxes(pista, aqui, fora);
        Vector3 paraFora = Vector3Normalize(Vector3Subtract(longe, frente));
        Vector3 aoLongo = Vector3Normalize(Vector3CrossProduct(paraFora, (Vector3){ 0.0f, 1.0f, 0.0f }));
        Vector3 cima = { 0.0f, 1.0f, 0.0f };
        Vector3 topo[TOTAL_DE_LINHAS], chao[TOTAL_DE_LINHAS];

        for (int l = 0; l < TOTAL_DE_LINHAS; l++) {
            float distancia = (l < 4) ? LINHAS[l] : fora - LADO_DO_PILAR / 2.0f;
            topo[l] = noPredioDosBoxes(pista, aqui, distancia);
            topo[l].y -= LAJE_DOS_BOXES;
            chao[l] = topo[l];
            chao[l].y = alturaNaGrade(topo[l].x, topo[l].z) - ABAIXO_DO_TERRENO_PILAR;
            if (chao[l].y < topo[l].y) {
                blocoDeConcreto(c, chao[l], topo[l], Vector3Scale(paraFora, LADO_DO_PILAR / 2.0f), Vector3Scale(aoLongo, LADO_DO_PILAR / 2.0f), COR_DO_PILAR);
            }
        }

        /* As vigas de um lado ao outro: a de cima e, nos pilares altos, a de travamento */
        Vector3 metadeLargura = Vector3Scale(aoLongo, LARGURA_DA_VIGA / 2.0f), metadeAltura = Vector3Scale(cima, ALTURA_DA_VIGA / 2.0f);
        Vector3 metadeLarguraLonga = Vector3Scale(paraFora, LARGURA_DA_VIGA / 2.0f);
        for (int l = 0; l + 1 < TOTAL_DE_LINHAS; l++) {
            Vector3 v0 = { topo[l].x, topo[l].y - ALTURA_DA_VIGA / 2.0f, topo[l].z }, v1 = { topo[l + 1].x, topo[l + 1].y - ALTURA_DA_VIGA / 2.0f, topo[l + 1].z };
            blocoDeConcreto(c, v0, v1, metadeLargura, metadeAltura, COR_DA_VIGA);
            float vao0 = topo[l].y - chao[l].y, vao1 = topo[l + 1].y - chao[l + 1].y;
            if (vao0 > VAO_PARA_TRAVAR && vao1 > VAO_PARA_TRAVAR) {
                Vector3 m0 = { topo[l].x, chao[l].y + vao0 / 2.0f, topo[l].z }, m1 = { topo[l + 1].x, chao[l + 1].y + vao1 / 2.0f, topo[l + 1].z };
                blocoDeConcreto(c, m0, m1, metadeLargura, metadeAltura, COR_DA_VIGA);
            }
        }

        /* As vigas ao longo do prédio, e a laje entre esta fila e a anterior */
        if (fila > 0) {
            for (int l = 0; l < TOTAL_DE_LINHAS; l++) {
                Vector3 v0 = { anteriorTopo[l].x, anteriorTopo[l].y - ALTURA_DA_VIGA / 2.0f, anteriorTopo[l].z }, v1 = { topo[l].x, topo[l].y - ALTURA_DA_VIGA / 2.0f, topo[l].z };
                blocoDeConcreto(c, v0, v1, metadeLarguraLonga, metadeAltura, COR_DA_VIGA);
                float vao0 = anteriorTopo[l].y - anteriorChao[l].y, vao1 = topo[l].y - chao[l].y;
                if (vao0 > VAO_PARA_TRAVAR && vao1 > VAO_PARA_TRAVAR) {
                    Vector3 m0 = { anteriorTopo[l].x, anteriorChao[l].y + vao0 / 2.0f, anteriorTopo[l].z }, m1 = { topo[l].x, chao[l].y + vao1 / 2.0f, topo[l].z };
                    blocoDeConcreto(c, m0, m1, metadeLarguraLonga, metadeAltura, COR_DA_VIGA);
                }
            }
            Vector3 l0 = noPredioDosBoxes(pista, aqui - PASSO_DOS_PILARES, 0.0f), l1 = noPredioDosBoxes(pista, aqui, 0.0f);
            Vector3 l2 = noPredioDosBoxes(pista, aqui, fora), l3 = noPredioDosBoxes(pista, aqui - PASSO_DOS_PILARES, fora);
            l0.y -= LAJE_DOS_BOXES; l1.y -= LAJE_DOS_BOXES; l2.y -= LAJE_DOS_BOXES; l3.y -= LAJE_DOS_BOXES;
            quadIluminado(c, l0, l1, l2, l3, COR_DA_LAJE);
        }
        for (int l = 0; l < TOTAL_DE_LINHAS; l++) {
            anteriorTopo[l] = topo[l];
            anteriorChao[l] = chao[l];
        }
    }
}

/*
 * construirPredioDosBoxes
 * O prédio em módulos, cada um na altura da reta, com uma ponta em cada extremidade e
 * a torre de controle no meio; as marcas de cada garagem no chão; e, atrás, o pátio
 * de asfalto ligado ao pit lane por ruas de serviço nas pontas.
 * Recebe: a pista, a malha do pit lane (onde entra o pátio) e a das marcas.
 * Devolve: nada.
 */
static void construirPredioDosBoxes(Pista *pista, Construcao *pit, Construcao *marcas) {
    Vector3 antes = noPredioDosBoxes(pista, PRIMEIRO_METRO_DOS_BOXES, 0.0f);
    Vector3 depois = noPredioDosBoxes(pista, PRIMEIRO_METRO_DOS_BOXES + 20.0f, 0.0f);
    float giro = atan2f(-(depois.z - antes.z), depois.x - antes.x);   /* a frente olha para o pit lane */
    float primeiro = PRIMEIRO_METRO_DOS_BOXES;
    float ultimo = primeiro + MODULOS_DOS_BOXES * METROS_DO_MODULO;
    float largura = pista->largada->larguraEsquerda;
    float metrosPorGaragem = METROS_DO_MODULO / GARAGENS_POR_MODULO;

    for (int i = 0; i < MODULOS_DOS_BOXES; i++) {
        float meio = primeiro + (i + 0.5f) * METROS_DO_MODULO;
        colocarModelo(MODELO_BOXES, noPredioDosBoxes(pista, meio, 0.0f), giro, 1.0f);
        if (i == MODULO_DA_TORRE) {
            colocarModelo(MODELO_BOXES_TORRE, noPredioDosBoxes(pista, meio, 0.0f), giro, 1.0f);
        }
        for (int g = 0; g < GARAGENS_POR_MODULO; g++) {
            float centro = primeiro + i * METROS_DO_MODULO + (g + 0.5f) * metrosPorGaragem;
            float a0 = centro - LARGURA_DA_VAGA / 2.0f, a1 = centro + LARGURA_DA_VAGA / 2.0f;
            Vector3 p00 = noPredioDosBoxes(pista, a0, 0.0f), p10 = noPredioDosBoxes(pista, a1, 0.0f);
            Vector3 p01 = noPredioDosBoxes(pista, a0, -FUNDO_DA_VAGA), p11 = noPredioDosBoxes(pista, a1, -FUNDO_DA_VAGA);
            pintarFaixaDoPit(marcas, p00, p01, LINHA_DO_PIT, COR_DA_VAGA);
            pintarFaixaDoPit(marcas, p10, p11, LINHA_DO_PIT, COR_DA_VAGA);
            pintarFaixaDoPit(marcas, p01, p11, LINHA_DO_PIT, COR_DA_VAGA);
        }
    }
    float metadeDaPonta = METROS_DA_PONTA / 2.0f;
    colocarModelo(MODELO_BOXES_PONTA, noPredioDosBoxes(pista, primeiro - metadeDaPonta, 0.0f), giro, 1.0f);
    colocarModelo(MODELO_BOXES_PONTA, noPredioDosBoxes(pista, ultimo + metadeDaPonta, 0.0f), giro, 1.0f);

    /* O pátio de trás e as ruas de serviço das pontas */
    float inicioDoPatio = primeiro - METROS_DA_PONTA - PATIO_ALEM_DO_PREDIO;
    float fimDoPatio = ultimo + METROS_DA_PONTA + PATIO_ALEM_DO_PREDIO;
    float foraDoPatio = FUNDO_DOS_BOXES + FUNDO_DO_PATIO;
    for (float a = inicioDoPatio; a < fimDoPatio; a += PASSO_DO_PATIO) {
        float b = fminf(a + PASSO_DO_PATIO, fimDoPatio);
        int naPonta = (a < primeiro - METROS_DA_PONTA) || (b > ultimo + METROS_DA_PONTA);
        float dentro = naPonta ? 0.0f : FUNDO_DOS_BOXES;
        Vector3 d0 = noPredioDosBoxes(pista, a, dentro), d1 = noPredioDosBoxes(pista, b, dentro);
        Vector3 f0 = noPredioDosBoxes(pista, a, foraDoPatio), f1 = noPredioDosBoxes(pista, b, foraDoPatio);
        d0.y = fmaxf(d0.y, alturaNaGrade(d0.x, d0.z)) + 0.05f;
        d1.y = fmaxf(d1.y, alturaNaGrade(d1.x, d1.z)) + 0.05f;
        f0.y = fmaxf(f0.y, alturaNaGrade(f0.x, f0.z)) + 0.05f;
        f1.y = fmaxf(f1.y, alturaNaGrade(f1.x, f1.z)) + 0.05f;
        float u = (foraDoPatio - dentro) / largura * REPETICOES_ASFALTO / 2.0f, v0 = a / 12.0f, v1 = b / 12.0f;
        adicionarQuadrilatero(pit, d0, d1, f1, f0, (Vector2){ 0.0f, v0 }, (Vector2){ 0.0f, v1 }, (Vector2){ u, v1 }, (Vector2){ u, v0 }, WHITE);

        /* A viga da beira do pátio */
        quadIluminado(marcas, (Vector3){ f0.x, f0.y - ALTURA_DA_BORDA_DO_PATIO, f0.z }, (Vector3){ f1.x, f1.y - ALTURA_DA_BORDA_DO_PATIO, f1.z }, f1, f0, COR_DO_CONCRETO);
        if (a == inicioDoPatio || b >= fimDoPatio) {
            float ponta = (a == inicioDoPatio) ? a : b;
            Vector3 dentroNaPonta = noPredioDosBoxes(pista, ponta, 0.0f), foraNaPonta = (a == inicioDoPatio) ? f0 : f1;
            dentroNaPonta.y = fmaxf(dentroNaPonta.y, alturaNaGrade(dentroNaPonta.x, dentroNaPonta.z)) + 0.05f;
            quadIluminado(marcas, (Vector3){ dentroNaPonta.x, dentroNaPonta.y - ALTURA_DA_BORDA_DO_PATIO, dentroNaPonta.z },
                          (Vector3){ foraNaPonta.x, foraNaPonta.y - ALTURA_DA_BORDA_DO_PATIO, foraNaPonta.z }, foraNaPonta, dentroNaPonta, COR_DO_CONCRETO);
        }
    }
    construirEstruturaDosBoxes(pista, marcas, inicioDoPatio, fimDoPatio, foraDoPatio);
}

/*
 * adicionarTalude
 * A rampa de grama que desce de uma borda do pit lane até o terreno.
 * Recebe: a construção, os dois pontos da borda, as laterais deles, os dois segmentos e
 *         para que lado ela desce (-1 para fora da pista, +1 de volta para ela).
 * Devolve: nada.
 */
static void adicionarTalude(Construcao *c, Vector3 a, Vector3 b, float lateralA, float lateralB, Segmento *sa, Segmento *sb, float sentido) {
    Vector3 pontos[2] = { a, b };
    float laterais[2] = { lateralA, lateralB };
    Segmento *segmentos[2] = { sa, sb };
    Vector3 baixo[2];
    for (int i = 0; i < 2; i++) {
        float queda = fmaxf(pontos[i].y - alturaNaGrade(pontos[i].x, pontos[i].z), 0.2f);
        float lateral = -laterais[i];
        float longe = lateral + sentido * queda * RAMPA_DO_TALUDE / segmentos[i]->larguraEsquerda;
        baixo[i] = pontoNaEstrada(segmentos[i], 0.0f, longe, 0.0f);
        baixo[i].y = alturaNaGrade(baixo[i].x, baixo[i].z) - 0.05f;
    }
    Vector2 u0 = { a.x / METROS_DA_GRAMA, a.z / METROS_DA_GRAMA }, u1 = { b.x / METROS_DA_GRAMA, b.z / METROS_DA_GRAMA };
    Vector2 u2 = { baixo[1].x / METROS_DA_GRAMA, baixo[1].z / METROS_DA_GRAMA }, u3 = { baixo[0].x / METROS_DA_GRAMA, baixo[0].z / METROS_DA_GRAMA };
    adicionarQuadrilatero(c, a, b, baixo[1], baixo[0], u0, u1, u2, u3, WHITE);
}

/*
 * construirRetaDosBoxes
 * O muro e o alambrado dos dois lados da reta; o pit lane, que sai da pista na Subida
 * dos Boxes e volta depois da reta; e o prédio dos boxes.
 * Recebe: a pista e a construção das estruturas (onde entra a barreira).
 * Devolve: nada.
 */
static void construirRetaDosBoxes(Pista *pista) {
    Construcao muro = { 0 }, alambrado = { 0 }, pit[2] = { { 0 }, { 0 } }, marcas = { 0 }, taludes = { 0 };
    float metrosPorSegmento = COMPRIMENTO_SEGMENTO / UNIDADES_POR_METRO;
    float metrosDaVolta = contarSegmentos(pista) * metrosPorSegmento;
    float largura = pista->largada->larguraEsquerda;
    float traco = 0.0f;

    Segmento *atual = pista->largada;
    do {
        Segmento *seguinte = atual->proximo;
        float metros = atual->indice * metrosPorSegmento;
        float v0 = metros / METROS_POR_TEXTURA, v1 = (metros + metrosPorSegmento) / METROS_POR_TEXTURA;
        for (int lado = -1; lado <= 1; lado += 2) {
            if (!(atual->temMuro && seguinte->temMuro)) {
                continue;
            }
            float l0 = atual->lugarDoMuro, l1 = seguinte->lugarDoMuro;
            Vector3 p0 = pontoNaEstrada(atual, 0.0f, lado * l0, 0.0f), p1 = pontoNaEstrada(seguinte, 0.0f, lado * l1, 0.0f);
            Vector3 m0 = { p0.x, p0.y + ALTURA_DO_MURO, p0.z }, m1 = { p1.x, p1.y + ALTURA_DO_MURO, p1.z };
            Vector3 t0 = { p0.x, m0.y + ALTURA_DO_ALAMBRADO, p0.z }, t1 = { p1.x, m1.y + ALTURA_DO_ALAMBRADO, p1.z };
            /* O muro desce até o terreno, mesmo onde ele fica bem abaixo da pista */
            Vector3 b0 = { p0.x, fminf(p0.y - ABAIXO_DA_PISTA, alturaNaGrade(p0.x, p0.z)) - 1.0f, p0.z };
            Vector3 b1 = { p1.x, fminf(p1.y - ABAIXO_DA_PISTA, alturaNaGrade(p1.x, p1.z)) - 1.0f, p1.z };
            adicionarQuadrilatero(&muro, b0, b1, m1, m0, (Vector2){ 0.0f, v0 }, (Vector2){ 0.0f, v1 }, (Vector2){ 1.0f, v1 }, (Vector2){ 1.0f, v0 }, WHITE);
            adicionarQuadrilatero(&alambrado, m0, m1, t1, t0, (Vector2){ 0.0f, v0 }, (Vector2){ 0.0f, v1 }, (Vector2){ 1.0f, v1 }, (Vector2){ 1.0f, v0 }, WHITE);
        }

        float metros1 = (seguinte == pista->largada) ? metrosDaVolta : metros + metrosPorSegmento;
        float largura0 = 0.0f, largura1 = 0.0f;
        float lado0 = ladoDentroDoPit(metros, metrosDaVolta, &largura0), lado1 = ladoDentroDoPit(metros1, metrosDaVolta, &largura1);
        if (lado0 > 0.0f && lado1 > 0.0f) {
            int claro = (atual->indice / SEGMENTOS_POR_FAIXA) % 2;
            float fora0 = lado0 + largura0 / largura, fora1 = lado1 + largura1 / largura;
            Vector3 d0 = noChaoDoPit(atual, -lado0), d1 = noChaoDoPit(seguinte, -lado1);
            Vector3 f0 = noChaoDoPit(atual, -fora0), f1 = noChaoDoPit(seguinte, -fora1);

            /* O asfalto é o mesmo da pista, com a imagem continuando de um para o outro */
            #define U_NA_LATERAL(lateral) ((1.0f - (lateral)) * 0.5f * REPETICOES_ASFALTO)
            adicionarQuadrilatero(&pit[claro], f0, f1, d1, d0, (Vector2){ U_NA_LATERAL(fora0), 1.0f }, (Vector2){ U_NA_LATERAL(fora1), 0.0f },
                                  (Vector2){ U_NA_LATERAL(lado1), 0.0f }, (Vector2){ U_NA_LATERAL(lado0), 1.0f }, WHITE);

            /* A ilha: enquanto os dois ainda estão perto, o que fica no meio é asfalto, pintado */
            int naIlha = (lado0 > 1.01f || lado1 > 1.01f) && lado0 <= LADO_DA_ILHA && lado1 <= LADO_DA_ILHA && !atual->temMuro;
            if (naIlha) {
                Vector3 b0 = noChaoDoPit(atual, -1.0f), b1 = noChaoDoPit(seguinte, -1.0f);
                adicionarQuadrilatero(&pit[claro], d0, d1, b1, b0, (Vector2){ U_NA_LATERAL(lado0), 1.0f }, (Vector2){ U_NA_LATERAL(lado1), 0.0f },
                                      (Vector2){ U_NA_LATERAL(1.0f), 0.0f }, (Vector2){ U_NA_LATERAL(1.0f), 1.0f }, WHITE);
                if (fmodf(metros, METROS_ENTRE_LISTRAS) < metrosPorSegmento) {
                    /* A listra vai da pista até o pit lane, sempre inclinada para a frente */
                    Segmento *alvo = buscarSegmento(pista, (atual->indice + SEGMENTOS_DA_LISTRA) % contarSegmentos(pista));
                    float larguraDoAlvo = 0.0f;
                    float ladoDoAlvo = ladoDentroDoPit(alvo->indice * metrosPorSegmento, metrosDaVolta, &larguraDoAlvo);
                    if (ladoDoAlvo > 1.03f && ladoDoAlvo <= LADO_DA_ILHA) {
                        pintarFaixaDoPit(&marcas, b0, noChaoDoPit(alvo, -ladoDoAlvo), LINHA_DA_LISTRA, COR_DA_FAIXA_DO_PIT);
                    }
                }
            }
            #undef U_NA_LATERAL

            /* A grama desce do asfalto até o terreno, dos dois lados: nada fica flutuando */
            adicionarTalude(&taludes, f0, f1, fora0, fora1, atual, seguinte, -1.0f);
            if (lado0 > LADO_DA_ILHA && lado1 > LADO_DA_ILHA) {
                adicionarTalude(&taludes, d0, d1, lado0, lado1, atual, seguinte, 1.0f);
            }

            /* As linhas: branca por fora; por dentro, branca se está separado da pista, e
             * tracejada onde se pode passar de um para o outro */
            if (largura0 > LINHA_DO_PIT * 4.0f) {
                pintarFaixaDoPit(&marcas, f0, f1, LINHA_DO_PIT, COR_DA_FAIXA_DO_PIT);
            }
            traco += metrosPorSegmento;
            int colado = (lado0 <= 1.01f && lado1 <= 1.01f) && !atual->temMuro;
            if (!colado || fmodf(traco, TRACO_DO_PIT * 2.0f) < TRACO_DO_PIT) {
                pintarFaixaDoPit(&marcas, d0, d1, LINHA_DO_PIT, COR_DA_FAIXA_DO_PIT);
            }
            if (fmodf(traco, TRACO_DO_PIT * 2.0f) < TRACO_DO_PIT && largura0 > LARGURA_DO_PIT_LANE * 0.9f && !colado) {
                float meio0 = fora0 - FUNDO_DA_VAGA / largura, meio1 = fora1 - FUNDO_DA_VAGA / largura;
                pintarFaixaDoPit(&marcas, noChaoDoPit(atual, -meio0), noChaoDoPit(seguinte, -meio1), LINHA_DO_PIT, COR_DA_FAIXA_DO_PIT);
            }
        }
        atual = seguinte;
    } while (atual != pista->largada);

    /* As duas linhas do limite de velocidade, de um lado ao outro do pit lane */
    float metrosDasLinhas[2] = { pista->pit.inicioDoLimite, pista->pit.fimDoLimite };
    for (int i = 0; i < 2; i++) {
        Segmento *naLinha = buscarSegmento(pista, (int) (metrosDasLinhas[i] * SEGMENTOS_POR_METRO));
        float bordaDaPista = -1.0f - LARGURA_DO_PIT_LANE / largura;
        pintarFaixaDoPit(&marcas, noChaoDoPit(naLinha, -1.0f), noChaoDoPit(naLinha, bordaDaPista), LINHA_DO_LIMITE, COR_DA_FAIXA_DO_PIT);
    }

    /* Os pneus de proteção na ponta dos muros, do lado da pista: nunca no caminho do pit lane */
    for (int k = 0; k < PILHAS_NA_PONTA; k++) {
        Vector3 noFunil = pontoNaEstrada(buscarSegmento(pista, (int) ((INICIO_DO_FUNIL - k * METROS_ENTRE_PILHAS) * SEGMENTOS_POR_METRO)), 0.0f, -ABERTURA_DO_FUNIL + FOLGA_DAS_PILHAS / largura, 0.0f);
        Vector3 noFim = pontoNaEstrada(buscarSegmento(pista, (int) ((FIM_DO_MURO + k * METROS_ENTRE_PILHAS) * SEGMENTOS_POR_METRO)), 0.0f, -1.0f, 0.0f);
        ModeloDeObjeto modelo = (k % 2 == 0) ? MODELO_PNEUS_VERMELHO : MODELO_PNEUS_BRANCO;
        colocarModelo(modelo, noFunil, 0.0f, 1.0f);
        colocarModelo(modelo, noFim, 0.0f, 1.0f);
    }

    construirPredioDosBoxes(pista, &pit[0], &marcas);
    interlagos.muro = terminarMalha(&muro);
    interlagos.alambrado = terminarMalha(&alambrado);
    interlagos.pitLane[0] = terminarMalha(&pit[0]);
    interlagos.pitLane[1] = terminarMalha(&pit[1]);
    interlagos.taludes = terminarMalha(&taludes);
    interlagos.marcasDoPit = terminarMalha(&marcas);
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
 * Recebe: um modelo do cenário, o pé, o giro e o tamanho (quem escolhe é a infraestrutura).
 * Devolve: nada.
 */
static void colocarNoCenario(ModeloDoCenario qual, Vector3 pe, float giro, float tamanho) {
    colocarModelo((ModeloDeObjeto) (MODELO_PRIMEIRO_DO_CENARIO + qual), pe, giro, tamanho);
}

/*
 * plantarArvoreDoJardim
 * Recebe: o pé da árvore (quem escolhe o lugar é a infraestrutura).
 * Devolve: nada.
 */
static void plantarArvoreDoJardim(Vector3 pe) {
    int variacao = (int) sortearObjeto(0.0f, 2.999f);
    pe.y -= AFUNDAR_NO_CHAO;
    colocarModelo((ModeloDeObjeto) (MODELO_ARVORE_1 + variacao), pe, sortearObjeto(0.0f, 2.0f * PI), sortearObjeto(MENOR_ARVORE, MAIOR_ARVORE));
}

/* As árvores da beira da pista esperam os telões escolherem o lugar (veja construirObjetos) */
#define MAXIMO_DE_ARVORES_ADIADAS 4096
typedef struct {
    Vector3 pe;
    float giro, tamanho;
    int variacao;
} ArvoreAdiada;
static ArvoreAdiada arvoresAdiadas[MAXIMO_DE_ARVORES_ADIADAS];
static int totalDeArvoresAdiadas = 0;

/*
 * construirObjetos
 * Anda pela lista de objetos de cada segmento, põe uma cópia do modelo de
 * cada um no lugar dele e fecha os lotes. As placas e o pórtico olham para
 * os carros que vêm: a frente do modelo (+z) fica contra o rumo da pista.
 * Recebe: a pista.
 * Devolve: nada (preenche "lotes").
 */
static void construirObjetos(Pista *pista) {
    sementeDosObjetos = SEMENTE_DOS_OBJETOS;
    carregarModelosDosObjetos();

    Segmento *atual = pista->largada;
    do {
        PosicaoNaPista lugar = { atual, 0.0f };
        for (Obstaculo *objeto = atual->obstaculos; objeto != NULL; objeto = objeto->proximo) {
            PontoNoMundo ponto = pontoNoMundo(lugar, objeto->deslocamentoLateral);
            Vector3 pe = { ponto.x, alturaNaGrade(ponto.x, ponto.z) - AFUNDAR_NO_CHAO, ponto.z };
            float contraORumo = -ponto.rumo * DEG2RAD;
            switch (objeto->tipo) {
                case OBJETO_ARVORE: {
                    if (dentroDoLago(pe.x, pe.z)) {
                        break;
                    }
                    int variacao = (int) sortearObjeto(0.0f, 2.999f);
                    float giro = sortearObjeto(0.0f, 2.0f * PI), tamanho = sortearObjeto(MENOR_ARVORE, MAIOR_ARVORE);
                    if (totalDeArvoresAdiadas < MAXIMO_DE_ARVORES_ADIADAS) {
                        arvoresAdiadas[totalDeArvoresAdiadas++] = (ArvoreAdiada){ pe, giro, tamanho, variacao };
                    }
                    break;
                }
                case OBJETO_PNEUS:
                    adicionarBarreira(atual, objeto->deslocamentoLateral);
                    break;
                case OBJETO_PLACA_50:
                case OBJETO_PLACA_100:
                case OBJETO_PLACA_150:
                    colocarModelo((ModeloDeObjeto) (MODELO_PLACA_50 + (objeto->tipo - OBJETO_PLACA_50)), pe, contraORumo, 1.0f);
                    break;
                case OBJETO_CAMAROTE: {
                    float rumo = ponto.rumo * DEG2RAD;
                    colocarModelo(MODELO_CAMAROTE, pe, atan2f(-cosf(rumo), -sinf(rumo)), 1.0f);
                    break;
                }
                case OBJETO_PORTICO:
                    colocarModelo(MODELO_PORTICO, (Vector3){ ponto.x, ponto.y, ponto.z }, contraORumo, TAMANHO_DO_PORTICO);
                    break;
                default:
                    break;   /* as marcas do grid são pintadas no asfalto */
            }
        }
        atual = atual->proximo;
    } while (atual != pista->largada);

    construirArquibancadas(pista);
    infra = construirInfraestrutura(pista, alturaNaGrade, plantarArvoreDoJardim, colocarNoCenario);

    /* As árvores da beira da pista entram depois dos telões: onde fica um, não nasce árvore */
    for (int i = 0; i < totalDeArvoresAdiadas; i++) {
        ArvoreAdiada *a = &arvoresAdiadas[i];
        if (!naAreaDeUmTelao(a->pe.x, a->pe.z)) {
            colocarModelo((ModeloDeObjeto) (MODELO_ARVORE_1 + a->variacao), a->pe, a->giro, a->tamanho);
        }
    }
    totalDeArvoresAdiadas = 0;

    fecharTodosOsLotes();

    MemFree(alturasDaGrade);
    alturasDaGrade = NULL;
    lotesDeInterlagos = totalDeLotes;
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
    estadio = LoadModel(ARQUIVO_DO_ESTADIO);
    carregarNoite(estadio, MATERIAL_DOS_ASSENTOS, ESCALA_COPA_PISTAO);
    carregarTorcidaDoEstadio(estadio, MATERIAL_DOS_ASSENTOS, ESCALA_COPA_PISTAO);
    for (int i = 0; i < estadio.materialCount; i++) {
        estadio.materials[i].shader = (i == MATERIAL_DOS_ASSENTOS) ? shaderDaTorcida() : shaderDaNoite();
        Texture2D *textura = &estadio.materials[i].maps[MATERIAL_MAP_ALBEDO].texture;
        if (textura->id != 0 && textura->id != rlGetTextureIdDefault()) {
            GenTextureMipmaps(textura);
            SetTextureFilter(*textura, TEXTURE_FILTER_TRILINEAR);
            SetTextureFilter(*textura, TEXTURE_FILTER_ANISOTROPIC_8X);
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
    mapa->existe = 1;
    mapa->escala = escala;
    mapa->modelo = LoadModel(arquivo);
    for (int i = 0; i < mapa->modelo.materialCount; i++) {
        mapa->modelo.materials[i].shader = shaderDosMapasDeDia;
        Texture2D *textura = &mapa->modelo.materials[i].maps[MATERIAL_MAP_ALBEDO].texture;
        if (textura->id != 0 && textura->id != rlGetTextureIdDefault()) {
            GenTextureMipmaps(textura);
            SetTextureFilter(*textura, TEXTURE_FILTER_TRILINEAR);
            SetTextureFilter(*textura, TEXTURE_FILTER_ANISOTROPIC_8X);
            SetTextureWrap(*textura, TEXTURE_WRAP_REPEAT);
        }
    }
    Construcao pintura = { 0 };
    pintarLargadaEGrid(&pintura, pista);
    pintarVagas(&pintura, pista);
    mapa->pintura = terminarMalha(&pintura);
    mapa->torcida = construirTorcida(lugares, totalDeLugares, escala, tamanho, pista);

    mapa->primeiroLote = totalDeLotes;
    for (int i = 0; i < totalDeCoisas; i++) {
        Vector3 pe = { coisas[i].x, coisas[i].y, coisas[i].z };
        colocarModelo(modeloDaCoisa(coisas[i].tipo), pe, coisas[i].giro, coisas[i].tamanho);
    }
    fecharTodosOsLotes();
    mapa->fimDosLotes = totalDeLotes;
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
 * carregarMundo
 * Prepara o cenário das pistas. Chamar uma vez, depois de criar a janela.
 * Recebe: as pistas, na ordem de IdPista.
 * Devolve: nada.
 */
void carregarMundo(Pista *pistas[]) {
    shader = LoadShaderFromMemory(VERTICES_DO_SHADER, PIXELS_DO_SHADER);
    shaderDosMapasDeDia = LoadShaderFromMemory(VERTICES_DOS_MAPAS_DE_DIA, PIXELS_DOS_MAPAS_DE_DIA);
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

    construirInterlagos(pistas[PISTA_INTERLAGOS]);
    carregarEstadio(pistas[PISTA_COPA_PISTAO]);
    carregarMapaDeDia(pistas[PISTA_OVAL], ARQUIVO_DO_OVAL, ESCALA_OVAL, TORCIDA_OVAL, TOTAL_TORCIDA_OVAL, FIGURA_DO_OVAL,
                      CENARIO_OVAL, TOTAL_CENARIO_OVAL);
    definirTamanhoTorcida3D(PISTA_INTERLAGOS, CARRINHO_DE_INTERLAGOS);
    carregarMapaDeDia(pistas[PISTA_SPLAT], ARQUIVO_DO_SPLAT, ESCALA_SPLAT, TORCIDA_SPLAT, TOTAL_TORCIDA_SPLAT, FIGURA_DO_SPLAT,
                      CENARIO_SPLAT, TOTAL_CENARIO_SPLAT);
    iniciarTorcida3D();
    ligarTeloesDoCenario();
}

/*
 * descarregarMundo
 * Devolve a memória de vídeo do cenário. Chamar antes de CloseWindow.
 * Recebe: nada.
 * Devolve: nada.
 */
void descarregarMundo(void) {
    /* O shader é compartilhado: é devolvido uma vez só, aqui embaixo */
    for (int i = 0; i < estadio.materialCount; i++) {
        estadio.materials[i].shader.id = rlGetShaderIdDefault();
    }
    descarregarTorcida3D();
    descarregarTelao();
    UnloadModel(estadio);
    for (int i = 0; i < TOTAL_PISTAS; i++) {
        if (mapasDeDia[i].existe) {
            for (int m = 0; m < mapasDeDia[i].modelo.materialCount; m++) {
                mapasDeDia[i].modelo.materials[m].shader.id = rlGetShaderIdDefault();
            }
            UnloadModel(mapasDeDia[i].modelo);
            UnloadMesh(mapasDeDia[i].pintura);
            UnloadMesh(mapasDeDia[i].torcida);
        }
    }
    UnloadMesh(pinturaDoEstadio);
    UnloadMesh(interlagos.asfalto[0]);
    UnloadMesh(interlagos.asfalto[1]);
    UnloadMesh(interlagos.pintura);
    UnloadMesh(interlagos.acostamento);
    UnloadMesh(interlagos.terreno);
    UnloadMesh(interlagos.base);
    UnloadMesh(interlagos.muro);
    UnloadMesh(interlagos.alambrado);
    UnloadMesh(interlagos.pitLane[0]);
    UnloadMesh(interlagos.pitLane[1]);
    UnloadMesh(interlagos.taludes);
    UnloadMesh(interlagos.marcasDoPit);
    UnloadMesh(interlagos.estruturas);
    UnloadMesh(interlagos.torcida);
    for (int i = 0; i < totalDeLotes; i++) {
        UnloadMesh(lotes[i].malha);
    }
    for (int i = 0; i < TOTAL_MODELOS_DE_OBJETO; i++) {
        UnloadModel(modelosDosObjetos[i]);
    }
    UnloadMesh(ceuDeDia);
    UnloadMesh(tampaDoCeu);
    UnloadMesh(ceuDeNoite);
    descarregarPaisagem();
    descarregarInfraestrutura();
    UnloadTexture(texturaCeu);
    UnloadTexture(texturaAsfalto[0]);
    UnloadTexture(texturaAsfalto[1]);
    UnloadTexture(texturaGrama);
    material.maps[MATERIAL_MAP_ALBEDO].texture.id = rlGetTextureIdDefault();
    material.shader.id = rlGetShaderIdDefault();
    UnloadMaterial(material);
    UnloadShader(shader);
    UnloadShader(shaderDosMapasDeDia);
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
    int primeiro = 0, fim = lotesDeInterlagos;
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
    Shader recebem[2] = { shader, shaderDosMapasDeDia };
    int slot = SLOT_DA_SOMBRA;
    for (int i = 0; i < 2; i++) {
        SetShaderValue(recebem[i], GetShaderLocation(recebem[i], "mapaDeSombra"), &slot, SHADER_UNIFORM_INT);
    }
}

/*
 * desenharQuemFazSombra
 * O que faz sombra no chão: as estruturas, as árvores e os objetos de cada pista.
 * Recebe: a pista.
 * Devolve: nada.
 */
static void desenharQuemFazSombra(IdPista id) {
    Texture2D semTextura = { 0 };
    Matrix origem = MatrixIdentity();
    Shader antes = material.shader;
    material.shader = shaderDaProfundidade;
    if (mapasDeDia[id].existe) {
        MapaDeDia *mapa = &mapasDeDia[id];
        Shader originais[256];
        int total = mapa->modelo.materialCount < 256 ? mapa->modelo.materialCount : 256;
        for (int i = 0; i < total; i++) {
            originais[i] = mapa->modelo.materials[i].shader;
            mapa->modelo.materials[i].shader = shaderDaProfundidade;
        }
        DrawModel(mapa->modelo, (Vector3){ 0.0f, 0.0f, 0.0f }, mapa->escala, WHITE);
        for (int i = 0; i < total; i++) {
            mapa->modelo.materials[i].shader = originais[i];
        }
        for (int i = mapa->primeiroLote; i < mapa->fimDosLotes; i++) {
            desenharLote(i, origem);
        }
    } else if (id == PISTA_INTERLAGOS) {
        desenharMalha(interlagos.muro, semTextura, origem);
        desenharMalha(interlagos.alambrado, estadio.materials[MATERIAL_DO_ALAMBRADO].maps[MATERIAL_MAP_ALBEDO].texture, origem);
        desenharMalha(interlagos.estruturas, semTextura, origem);
        desenharMalha(infra->objetos, semTextura, origem);
        desenharMalha(infra->anuncios, semTextura, origem);
        for (int i = 0; i < lotesDeInterlagos; i++) {
            desenharLote(i, origem);
        }
    }
    material.shader = antes;
}

/*
 * prepararSombras
 * Desenha o cenário visto do sol, num quadrado em volta da câmera, e passa o resultado
 * aos shaders do cenário. Chamar antes de BeginMode3D. À noite, e com a opção desligada,
 * não há sombra do sol.
 * Recebe: a pista, a câmera e se as sombras estão ligadas nas configurações.
 * Devolve: nada.
 */
void prepararSombras(IdPista id, Camera3D camera, int ligadas) {
    sombraLigadaAgora = ligadas && temMapaDeSombra && id != PISTA_COPA_PISTAO;
    float ligada = sombraLigadaAgora ? 1.0f : 0.0f;
    Shader recebem[2] = { shader, shaderDosMapasDeDia };
    for (int i = 0; i < 2; i++) {
        SetShaderValue(recebem[i], GetShaderLocation(recebem[i], "sombraLigada"), &ligada, SHADER_UNIFORM_FLOAT);
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
    static int pistaAnterior = -1;
    if ((int) id == pistaAnterior && hypotf(meio.x - meioAnterior.x, meio.z - meioAnterior.z) < REFAZER_A_SOMBRA) {
        return;   /* luzVP e o mapa continuam valendo */
    }
    meioAnterior = meio;
    pistaAnterior = id;
    Vector3 sol = Vector3Normalize((Vector3){ 0.35f, 0.8f, 0.45f });
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
    rlDisableBackfaceCulling();
    desenharQuemFazSombra(id);
    rlEnableBackfaceCulling();
    EndMode3D();
    EndTextureMode();

    Matrix luzVP = MatrixMultiply(vista, projecao);
    for (int i = 0; i < 2; i++) {
        SetShaderValueMatrix(recebem[i], GetShaderLocation(recebem[i], "luzVP"), luzVP);
    }
}

/*
 * desenharCenario
 * Desenha o cenário fixo da pista. Chamar entre BeginMode3D e EndMode3D.
 * Recebe: a pista e a posição da câmera (o céu e o fundo andam com ela).
 * Devolve: nada.
 */
void desenharCenario(IdPista id, Vector3 camera) {
    if (sombraLigadaAgora) {
        rlActiveTextureSlot(SLOT_DA_SOMBRA);
        rlEnableTexture(mapaDeSombra.depth.id);
        rlActiveTextureSlot(0);
    }
    Texture2D semTextura = { 0 };
    Matrix origem = MatrixIdentity();
    Matrix naCamera = MatrixTranslate(camera.x, camera.y, camera.z);

    if (id == PISTA_COPA_PISTAO) {
        desenharMalha(ceuDeNoite, texturaDoCeuNoturno(), naCamera);
        DrawModel(estadio, (Vector3){ 0.0f, 0.0f, 0.0f }, ESCALA_COPA_PISTAO, WHITE);
        material.shader = shaderDaNoite();
        desenharMalha(pinturaDoEstadio, semTextura, origem);
        for (int i = lotesDeInterlagos; i < fimDosLotesDoEstadio; i++) {
            desenharLote(i, origem);
        }
        material.shader = shader;
        return;
    }

    if (mapasDeDia[id].existe) {
        MapaDeDia *mapa = &mapasDeDia[id];
        desenharMalha(ceuDeDia, texturaCeu, naCamera);
        desenharMalha(tampaDoCeu, semTextura, naCamera);
        DrawModel(mapa->modelo, (Vector3){ 0.0f, 0.0f, 0.0f }, mapa->escala, WHITE);
        desenharMalha(mapa->pintura, semTextura, origem);
        for (int i = mapa->primeiroLote; i < mapa->fimDosLotes; i++) {
            desenharLote(i, origem);
        }
        material.shader = shaderDaTorcida();
        desenharMalha(mapa->torcida, semTextura, origem);
        material.shader = shader;
        return;
    }

    desenharMalha(ceuDeDia, texturaCeu, naCamera);
    desenharMalha(tampaDoCeu, semTextura, naCamera);
    desenharPaisagem();
    desenharMalha(interlagos.base, texturaGrama, origem);
    desenharMalha(interlagos.terreno, texturaGrama, origem);
    desenharMalha(interlagos.acostamento, texturaGrama, origem);
    desenharMalha(infra->pintura, semTextura, origem);
    desenharMalha(interlagos.asfalto[0], texturaAsfalto[0], origem);
    desenharMalha(interlagos.asfalto[1], texturaAsfalto[1], origem);
    desenharMalha(interlagos.pitLane[0], texturaAsfalto[0], origem);
    desenharMalha(interlagos.pitLane[1], texturaAsfalto[1], origem);
    desenharMalha(interlagos.taludes, texturaGrama, origem);
    desenharMalha(interlagos.marcasDoPit, semTextura, origem);
    desenharMalha(interlagos.pintura, semTextura, origem);
    desenharMalha(interlagos.muro, estadio.materials[MATERIAL_DO_MURO].maps[MATERIAL_MAP_ALBEDO].texture, origem);
    desenharMalha(interlagos.alambrado, estadio.materials[MATERIAL_DO_ALAMBRADO].maps[MATERIAL_MAP_ALBEDO].texture, origem);
    desenharMalha(interlagos.estruturas, semTextura, origem);
    desenharMalha(infra->objetos, semTextura, origem);
    desenharMalha(infra->anuncios, infra->imagemDosAnuncios, origem);
    desenharMalha(infra->cercas, estadio.materials[MATERIAL_DO_ALAMBRADO].maps[MATERIAL_MAP_ALBEDO].texture, origem);
    material.shader = shaderDaTorcida();
    desenharMalha(interlagos.torcida, semTextura, origem);
    material.shader = shader;
    for (int i = 0; i < lotesDeInterlagos; i++) {
        desenharLote(i, origem);
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
