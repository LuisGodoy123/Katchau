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
#define MAXIMO_DE_LOTES       160
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
#define RAIO_DA_CIDADE        4200.0f
#define RAIO_DOS_MORROS       3200.0f
#define ANEIS_DO_CEU          32
#define ELEVACAO_DA_BASE_DO_CEU (-12.0f)  /* graus: o céu começa abaixo do horizonte */
#define COMECO_DA_IMAGEM_DO_CEU 0.02f     /* a imagem do céu vai até aqui, e daí para cima é a tampa */
#define COR_DO_TOPO_DO_CEU    (Color){ 52, 112, 196, 255 }   /* a cor da linha de cima de fundo/ceu.png */
#define REPETICOES_CEU        2.0f
#define REPETICOES_CIDADE     5.0f
#define REPETICOES_MORROS     4.0f
#define ABAIXO_DA_CAMERA      300.0f    /* a base dos cilindros fica abaixo do horizonte */
#define PLANO_DE_LONGE        14000.0f
#define PLANO_DE_PERTO        0.5f

/* Descarta os pixels transparentes, em vez de misturar */
static const char *VERTICES_DO_SHADER =
    "#version 330\n"
    "in vec3 vertexPosition; in vec2 vertexTexCoord; in vec4 vertexColor;\n"
    "uniform mat4 mvp;\n"
    "out vec2 fragTexCoord; out vec4 fragColor;\n"
    "void main() { fragTexCoord = vertexTexCoord; fragColor = vertexColor;\n"
    "              gl_Position = mvp * vec4(vertexPosition, 1.0); }\n";
static const char *PIXELS_DO_SHADER =
    "#version 330\n"
    "in vec2 fragTexCoord; in vec4 fragColor;\n"
    "uniform sampler2D texture0; uniform vec4 colDiffuse;\n"
    "out vec4 finalColor;\n"
    "void main() { vec4 cor = texture(texture0, fragTexCoord) * colDiffuse * fragColor;\n"
    "              if (cor.a < 0.5) discard;\n"
    "              finalColor = vec4(cor.rgb, 1.0); }\n";

/* Uma malha em construção: vai crescendo triângulo a triângulo */
typedef struct {
    float *vertices;
    float *texturas;
    unsigned char *cores;
    int total;       /* vértices usados */
    int capacidade;
} Construcao;

/* As malhas geradas para Interlagos */
typedef struct {
    Mesh asfalto[2];        /* [0] escuro, [1] claro */
    Mesh pintura;           /* zebras, linha de chegada e grid: cores nos vértices */
    Mesh acostamento;
    Mesh terreno;
    Mesh base;
    Mesh muro;              /* o muro de concreto da reta dos boxes */
    Mesh alambrado;         /* e o alambrado em cima dele */
    Mesh pitLane;
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
    TOTAL_MODELOS_DE_OBJETO
} ModeloDeObjeto;

static const char *ARQUIVOS_DOS_OBJETOS[TOTAL_MODELOS_DE_OBJETO] = {
    "arvore_1.glb", "arvore_2.glb", "arvore_3.glb", "pneus_vermelho.glb", "pneus_branco.glb",
    "placa_50.glb", "placa_100.glb", "placa_150.glb", "portico.glb", "arquibancada.glb", "camarote.glb",
    "setor_A.glb", "setor_B.glb", "setor_D.glb", "setor_G.glb", "setor_H.glb",
    "setor_M.glb", "setor_N.glb", "setor_R.glb", "setor_S.glb", "setor_V.glb", "refletor.glb", "boxes.glb", "boxes_ponta.glb", "boxes_torre.glb",
};

/* Centenas de cópias do mesmo modelo desenhadas uma a uma pesariam: elas são
 * juntadas em malhas únicas, os lotes, uma para cada textura */
typedef struct {
    Mesh malha;
    Texture2D textura;
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
static Material material;   /* o mesmo material, trocando a textura a cada desenho */
static Model estadio;
static Mesh pinturaDoEstadio;
static MalhasDaPista interlagos;
static Mesh ceuDeDia, tampaDoCeu, ceuDeNoite, cilindroDaCidade, cilindroDosMorros;
static Texture2D texturaCeu, texturaCidade, texturaMorros;
static Texture2D texturaAsfalto[2], texturaGrama;
static Model modelosDosObjetos[TOTAL_MODELOS_DE_OBJETO];
static LoteDeObjetos lotes[MAXIMO_DE_LOTES];
static int totalDeLotes = 0;
static int lotesDeInterlagos = 0;   /* os primeiros lotes são de Interlagos; os demais, do oval */

/* ==================================================================
 * Construção de malhas
 * ================================================================== */

/*
 * adicionarVertice
 * Põe um vértice na malha em construção, dobrando o espaço quando acaba.
 * Recebe: a construção, a posição, a coordenada de textura e a cor.
 * Devolve: nada.
 */
static void adicionarVertice(Construcao *c, Vector3 p, Vector2 uv, Color cor) {
    if (c->total == c->capacidade) {
        c->capacidade = (c->capacidade == 0) ? 1024 : c->capacidade * 2;
        c->vertices = MemRealloc(c->vertices, c->capacidade * 3 * sizeof(float));
        c->texturas = MemRealloc(c->texturas, c->capacidade * 2 * sizeof(float));
        c->cores = MemRealloc(c->cores, c->capacidade * 4);
    }
    int i = c->total++;
    c->vertices[i * 3] = p.x;
    c->vertices[i * 3 + 1] = p.y;
    c->vertices[i * 3 + 2] = p.z;
    c->texturas[i * 2] = uv.x;
    c->texturas[i * 2 + 1] = uv.y;
    c->cores[i * 4] = cor.r;
    c->cores[i * 4 + 1] = cor.g;
    c->cores[i * 4 + 2] = cor.b;
    c->cores[i * 4 + 3] = cor.a;
}

/*
 * adicionarQuadrilatero
 * Um quadrilátero, em dois triângulos. Os vértices vêm em volta dele.
 * Recebe: a construção, os 4 pontos, os 4 pares de textura e a cor.
 * Devolve: nada.
 */
static void adicionarQuadrilatero(Construcao *c, Vector3 a, Vector3 b, Vector3 d, Vector3 e,
                                  Vector2 ua, Vector2 ub, Vector2 ud, Vector2 ue, Color cor) {
    adicionarVertice(c, a, ua, cor);
    adicionarVertice(c, b, ub, cor);
    adicionarVertice(c, d, ud, cor);
    adicionarVertice(c, a, ua, cor);
    adicionarVertice(c, d, ud, cor);
    adicionarVertice(c, e, ue, cor);
}

/*
 * terminarMalha
 * Envia a malha para a placa de vídeo. A Mesh passa a ser dona dos vetores.
 * Recebe: a construção.
 * Devolve: a malha pronta.
 */
static Mesh terminarMalha(Construcao *c) {
    Mesh malha = { 0 };
    malha.vertexCount = c->total;
    malha.triangleCount = c->total / 3;
    malha.vertices = c->vertices;
    malha.texcoords = c->texturas;
    malha.colors = c->cores;
    UploadMesh(&malha, false);
    return malha;
}

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
    return fminf(soma / somaPesos, limite);
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
 * Uma grade de quadradinhos de grama em volta da pista, com a altura vinda
 * de alturaDoTerreno. As células alternam um pouco o tom, o que ajuda a
 * perceber a velocidade.
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
            Color tom = ((k + l) % 2 == 0) ? WHITE : (Color){ 228, 236, 228, 255 };
            adicionarQuadrilatero(&c, a, b, d, e, ua, ub, ud, ue, tom);
        }
    }
    return terminarMalha(&c);   /* a grade fica para os objetos; construirObjetos a devolve */
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
        adicionarQuadrilatero(&asfalto[claro],
                              pontoNaEstrada(atual, 0.0f, -1.0f, 0.0f), pontoNaEstrada(atual, 0.0f, 1.0f, 0.0f),
                              pontoNaEstrada(atual, 1.0f, 1.0f, 0.0f), pontoNaEstrada(atual, 1.0f, -1.0f, 0.0f),
                              (Vector2){ 0.0f, 1.0f }, (Vector2){ REPETICOES_ASFALTO, 1.0f },
                              (Vector2){ REPETICOES_ASFALTO, 0.0f }, (Vector2){ 0.0f, 0.0f }, WHITE);

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
    interlagos.terreno = construirTerreno(pista);

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
 * luzDoSol
 * Quanto uma face clareia conforme olha para o sol. Os objetos não mudam
 * de lugar, então a luz é calculada uma vez, na construção.
 * Recebe: a direção para onde a face aponta.
 * Devolve: o fator de luz.
 */
static float luzDoSol(Vector3 normal) {
    Vector3 sol = Vector3Normalize((Vector3){ 0.35f, 0.8f, 0.45f });
    Vector3 n = Vector3Normalize(normal);
    return 0.58f + 0.30f * (n.y * 0.5f + 0.5f) + 0.38f * fmaxf(Vector3DotProduct(n, sol), 0.0f);
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

/*
 * quadIluminado
 * Um quadrilátero de uma cor, com a luz do sol calculada pela face dele.
 * Recebe: a construção, os 4 cantos em volta e a cor.
 * Devolve: nada.
 */
static void quadIluminado(Construcao *c, Vector3 a, Vector3 b, Vector3 d, Vector3 e, Color cor) {
    Vector3 normal = Vector3Normalize(Vector3CrossProduct(Vector3Subtract(b, a), Vector3Subtract(d, a)));
    if (normal.y < -0.2f) {      /* só vira quem olha para baixo: uma parede em curva não fica em faixas */
        normal = Vector3Negate(normal);
    }
    float luz = luzDoSol(normal);
    Color comLuz = { (unsigned char) fminf(cor.r * luz, 255.0f), (unsigned char) fminf(cor.g * luz, 255.0f),
                     (unsigned char) fminf(cor.b * luz, 255.0f), 255 };
    Vector2 zero = { 0.0f, 0.0f };
    adicionarQuadrilatero(c, a, b, d, e, zero, zero, zero, zero, comLuz);
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

static void construirRetaDosBoxes(Pista *pista, Construcao *estruturas);

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
    construirRetaDosBoxes(pista, &estrutura);
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
 * construirRetaDosBoxes
 * O muro e o alambrado dos dois lados da reta; o pit lane, que sai da pista na Subida
 * dos Boxes e volta depois da reta; e o prédio dos boxes.
 * Recebe: a pista e a construção das estruturas (onde entra a barreira).
 * Devolve: nada.
 */
static void construirRetaDosBoxes(Pista *pista, Construcao *estruturas) {
    Construcao muro = { 0 }, alambrado = { 0 }, pit = { 0 }, marcas = { 0 };
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
            float fora0 = lado0 + largura0 / largura, fora1 = lado1 + largura1 / largura;
            Vector3 d0 = noChaoDoPit(atual, -lado0), d1 = noChaoDoPit(seguinte, -lado1);
            Vector3 f0 = noChaoDoPit(atual, -fora0), f1 = noChaoDoPit(seguinte, -fora1);
            adicionarQuadrilatero(&pit, f0, f1, d1, d0, (Vector2){ 0.0f, 0.0f }, (Vector2){ 0.0f, 1.0f },
                                  (Vector2){ REPETICOES_ASFALTO, 1.0f }, (Vector2){ REPETICOES_ASFALTO, 0.0f }, WHITE);

            /* Onde o pit lane fica acima do terreno, ele é fechado até o chão dos dois lados */
            Vector3 f0Chao = { f0.x, alturaNaGrade(f0.x, f0.z) - ABAIXO_DO_TERRENO_PILAR, f0.z }, f1Chao = { f1.x, alturaNaGrade(f1.x, f1.z) - ABAIXO_DO_TERRENO_PILAR, f1.z };
            quadIluminado(estruturas, f0Chao, f1Chao, f1, f0, COR_DO_CONCRETO);
            if (lado0 > 1.02f || lado1 > 1.02f) {
                Vector3 d0Chao = { d0.x, alturaNaGrade(d0.x, d0.z) - ABAIXO_DO_TERRENO_PILAR, d0.z }, d1Chao = { d1.x, alturaNaGrade(d1.x, d1.z) - ABAIXO_DO_TERRENO_PILAR, d1.z };
                quadIluminado(estruturas, d0Chao, d1Chao, d1, d0, COR_DO_CONCRETO);
            }

            /* As linhas: brancas nas bordas e, tracejada, a que separa a pista de trabalho */
            pintarFaixaDoPit(&marcas, d0, d1, LINHA_DO_PIT, COR_DA_FAIXA_DO_PIT);
            pintarFaixaDoPit(&marcas, f0, f1, LINHA_DO_PIT, COR_DA_FAIXA_DO_PIT);
            traco += metrosPorSegmento;
            if (fmodf(traco, TRACO_DO_PIT * 2.0f) < TRACO_DO_PIT && largura0 > LARGURA_DO_PIT_LANE * 0.9f) {
                float meio0 = fora0 - FUNDO_DA_VAGA / largura, meio1 = fora1 - FUNDO_DA_VAGA / largura;
                pintarFaixaDoPit(&marcas, noChaoDoPit(atual, -meio0), noChaoDoPit(seguinte, -meio1), LINHA_DO_PIT, COR_DA_FAIXA_DO_PIT);
            }
        }
        atual = seguinte;
    } while (atual != pista->largada);

    /* As duas linhas do limite de velocidade, de um lado ao outro do pit lane */
    float metrosDasLinhas[2] = { INICIO_DO_LIMITE_DO_PIT, FIM_DO_LIMITE_DO_PIT };
    for (int i = 0; i < 2; i++) {
        Segmento *naLinha = buscarSegmento(pista, (int) (metrosDasLinhas[i] * SEGMENTOS_POR_METRO));
        float bordaDaPista = -1.0f - LARGURA_DO_PIT_LANE / largura;
        pintarFaixaDoPit(&marcas, noChaoDoPit(naLinha, -1.0f), noChaoDoPit(naLinha, bordaDaPista), LINHA_DO_LIMITE, COR_DA_FAIXA_DO_PIT);
    }

    construirPredioDosBoxes(pista, &pit, &marcas);
    interlagos.muro = terminarMalha(&muro);
    interlagos.alambrado = terminarMalha(&alambrado);
    interlagos.pitLane = terminarMalha(&pit);
    interlagos.marcasDoPit = terminarMalha(&marcas);
}

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
                    int variacao = (int) sortearObjeto(0.0f, 2.999f);
                    colocarModelo((ModeloDeObjeto) (MODELO_ARVORE_1 + variacao), pe, sortearObjeto(0.0f, 2.0f * PI),
                                  sortearObjeto(MENOR_ARVORE, MAIOR_ARVORE));
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

    for (int m = 0; m < TOTAL_MODELOS_DE_OBJETO; m++) {
        for (int i = 0; i < modelosDosObjetos[m].meshCount && i < MAXIMO_DE_PECAS; i++) {
            Material peca = modelosDosObjetos[m].materials[modelosDosObjetos[m].meshMaterial[i]];
            fecharLote(&lotesAbertos[m][i], peca.maps[MATERIAL_MAP_ALBEDO].texture);
        }
    }

    MemFree(alturasDaGrade);
    alturasDaGrade = NULL;
    lotesDeInterlagos = totalDeLotes;
}

/*
 * construirCilindro
 * Um cilindro aberto em volta da câmera, para o céu e as camadas do fundo.
 * Recebe: o raio, a altura e quantas vezes a imagem se repete na volta.
 * Devolve: a malha.
 */
static Mesh construirCilindro(float raio, float altura, float repeticoes) {
    Construcao c = { 0 };
    for (int i = 0; i < LADOS_DO_CILINDRO; i++) {
        float a0 = 2.0f * PI * i / LADOS_DO_CILINDRO, a1 = 2.0f * PI * (i + 1) / LADOS_DO_CILINDRO;
        float u0 = repeticoes * i / LADOS_DO_CILINDRO, u1 = repeticoes * (i + 1) / LADOS_DO_CILINDRO;
        Vector3 embaixo0 = { sinf(a0) * raio, 0.0f, -cosf(a0) * raio };
        Vector3 embaixo1 = { sinf(a1) * raio, 0.0f, -cosf(a1) * raio };
        Vector3 emcima0 = { embaixo0.x, altura, embaixo0.z };
        Vector3 emcima1 = { embaixo1.x, altura, embaixo1.z };
        adicionarQuadrilatero(&c, emcima0, embaixo0, embaixo1, emcima1, (Vector2){ u0, 0.0f }, (Vector2){ u0, 1.0f },
                              (Vector2){ u1, 1.0f }, (Vector2){ u1, 0.0f }, WHITE);
    }
    return terminarMalha(&c);
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

    Model modelo = modelosDosObjetos[MODELO_REFLETOR];
    for (int i = 0; i < modelo.meshCount && i < MAXIMO_DE_PECAS; i++) {
        fecharLote(&lotesAbertos[MODELO_REFLETOR][i], modelo.materials[modelo.meshMaterial[i]].maps[MATERIAL_MAP_ALBEDO].texture);
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
    pinturaDoEstadio = terminarMalha(&pintura);
    construirRefletores();
}

/*
 * carregarMundo
 * Prepara o cenário das pistas. Chamar uma vez, depois de criar a janela.
 * Recebe: as pistas, na ordem de IdPista.
 * Devolve: nada.
 */
void carregarMundo(Pista *pistas[]) {
    shader = LoadShaderFromMemory(VERTICES_DO_SHADER, PIXELS_DO_SHADER);
    material = LoadMaterialDefault();
    material.shader = shader;

    texturaCeu = carregarTexturaRepetida("fundo/ceu.png");
    texturaCidade = carregarTexturaRepetida("fundo/cidade.png");
    texturaMorros = carregarTexturaRepetida("fundo/morros.png");
    texturaAsfalto[0] = carregarTexturaRepetida("estrada/asfalto_escuro.png");
    texturaAsfalto[1] = carregarTexturaRepetida("estrada/asfalto_claro.png");
    texturaGrama = carregarTexturaRepetida("estrada/grama_clara.png");

    /* A imagem ocupa a altura proporcional à largura de cada repetição, sem esticar */
    float voltaCeu = 2.0f * PI * RAIO_DO_CEU / REPETICOES_CEU;
    float voltaCidade = 2.0f * PI * RAIO_DA_CIDADE / REPETICOES_CIDADE;
    float voltaMorros = 2.0f * PI * RAIO_DOS_MORROS / REPETICOES_MORROS;
    construirCeuDeDia(voltaCeu * texturaCeu.height / texturaCeu.width);
    ceuDeNoite = construirCeuDeNoite();
    /* Na vertical a imagem não se repete: a borda de cima não pode misturar com a de baixo */
    rlTextureParameters(texturaCeu.id, RL_TEXTURE_WRAP_T, RL_TEXTURE_WRAP_CLAMP);
    cilindroDaCidade = construirCilindro(RAIO_DA_CIDADE, voltaCidade * texturaCidade.height / texturaCidade.width, REPETICOES_CIDADE);
    cilindroDosMorros = construirCilindro(RAIO_DOS_MORROS, voltaMorros * texturaMorros.height / texturaMorros.width, REPETICOES_MORROS);

    construirInterlagos(pistas[PISTA_INTERLAGOS]);
    carregarEstadio(pistas[PISTA_COPA_PISTAO]);
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
    UnloadModel(estadio);
    UnloadMesh(pinturaDoEstadio);
    UnloadMesh(interlagos.asfalto[0]);
    UnloadMesh(interlagos.asfalto[1]);
    UnloadMesh(interlagos.pintura);
    UnloadMesh(interlagos.acostamento);
    UnloadMesh(interlagos.terreno);
    UnloadMesh(interlagos.base);
    UnloadMesh(interlagos.muro);
    UnloadMesh(interlagos.alambrado);
    UnloadMesh(interlagos.pitLane);
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
    UnloadMesh(cilindroDaCidade);
    UnloadMesh(cilindroDosMorros);
    UnloadTexture(texturaCeu);
    UnloadTexture(texturaCidade);
    UnloadTexture(texturaMorros);
    UnloadTexture(texturaAsfalto[0]);
    UnloadTexture(texturaAsfalto[1]);
    UnloadTexture(texturaGrama);
    material.maps[MATERIAL_MAP_ALBEDO].texture.id = rlGetTextureIdDefault();
    material.shader.id = rlGetShaderIdDefault();
    UnloadMaterial(material);
    UnloadShader(shader);
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
 * desenharCenario
 * Desenha o cenário fixo da pista. Chamar entre BeginMode3D e EndMode3D.
 * Recebe: a pista e a posição da câmera (o céu e o fundo andam com ela).
 * Devolve: nada.
 */
void desenharCenario(IdPista id, Vector3 camera) {
    Texture2D semTextura = { 0 };
    Matrix origem = MatrixIdentity();
    Matrix nosCilindros = MatrixTranslate(camera.x, camera.y - ABAIXO_DA_CAMERA, camera.z);
    Matrix naCamera = MatrixTranslate(camera.x, camera.y, camera.z);

    if (id == PISTA_COPA_PISTAO) {
        desenharMalha(ceuDeNoite, texturaDoCeuNoturno(), naCamera);
        DrawModel(estadio, (Vector3){ 0.0f, 0.0f, 0.0f }, ESCALA_COPA_PISTAO, WHITE);
        material.shader = shaderDaNoite();
        desenharMalha(pinturaDoEstadio, semTextura, origem);
        for (int i = lotesDeInterlagos; i < totalDeLotes; i++) {
            desenharMalha(lotes[i].malha, lotes[i].textura, origem);
        }
        material.shader = shader;
        return;
    }

    desenharMalha(ceuDeDia, texturaCeu, naCamera);
    desenharMalha(tampaDoCeu, semTextura, naCamera);
    desenharMalha(cilindroDaCidade, texturaCidade, nosCilindros);
    desenharMalha(cilindroDosMorros, texturaMorros, nosCilindros);
    desenharMalha(interlagos.base, texturaGrama, origem);
    desenharMalha(interlagos.terreno, texturaGrama, origem);
    desenharMalha(interlagos.acostamento, texturaGrama, origem);
    desenharMalha(interlagos.asfalto[0], texturaAsfalto[0], origem);
    desenharMalha(interlagos.asfalto[1], texturaAsfalto[1], origem);
    desenharMalha(interlagos.pitLane, texturaAsfalto[0], origem);
    desenharMalha(interlagos.marcasDoPit, semTextura, origem);
    desenharMalha(interlagos.pintura, semTextura, origem);
    desenharMalha(interlagos.muro, estadio.materials[MATERIAL_DO_MURO].maps[MATERIAL_MAP_ALBEDO].texture, origem);
    desenharMalha(interlagos.alambrado, estadio.materials[MATERIAL_DO_ALAMBRADO].maps[MATERIAL_MAP_ALBEDO].texture, origem);
    desenharMalha(interlagos.estruturas, semTextura, origem);
    material.shader = shaderDaTorcida();
    desenharMalha(interlagos.torcida, semTextura, origem);
    material.shader = shader;
    for (int i = 0; i < lotesDeInterlagos; i++) {
        desenharMalha(lotes[i].malha, lotes[i].textura, origem);
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
