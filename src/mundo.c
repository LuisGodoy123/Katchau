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
#include "raymath.h"
#include "rlgl.h"
#include "obstaculos.h"

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
#define SEMENTE_DOS_OBJETOS   2026u     /* sorteio fixo: as árvores são sempre iguais */
#define ABAIXO_DO_CHAO        1.0f      /* troncos e postes começam um pouco abaixo do terreno */
#define COR_DO_TRONCO         (Color){  92,  66,  42, 255 }
#define COR_DA_COPA           (Color){  52, 118,  44, 255 }
#define ALTURA_MINIMA_ARVORE  9.0f
#define ALTURA_MAXIMA_ARVORE  15.0f

#define PILHAS_POR_BARREIRA   7         /* a barreira vem a cada 6 segmentos (10 m): fica contínua */
#define PNEUS_POR_PILHA       3
#define RAIO_DO_PNEU          0.72f
#define ALTURA_DO_PNEU        0.48f
#define LADOS_DO_PNEU         8
#define COR_DA_BORRACHA       (Color){  42,  42,  48, 255 }
#define COR_DO_FURO           (Color){  12,  12,  14, 255 }
#define COR_PNEU_VERMELHO     (Color){ 205,  30,  36, 255 }
#define COR_PNEU_BRANCO       (Color){ 236, 236, 236, 255 }

#define COR_DO_METAL          (Color){ 130, 136, 148, 255 }
#define COR_DO_POSTE          (Color){  96, 100, 110, 255 }
#define COR_DAS_COSTAS        (Color){ 150,  36,  38, 255 }
#define ALTURA_DA_PLACA       5.6f      /* do chão até o alto da placa */
#define LADO_DA_PLACA         3.0f
#define ESPESSURA_DA_PLACA    0.12f
#define RECUO_DA_PLACA        0.22f     /* do centro do poste até as costas da placa */
#define FRACAO_DO_DESENHO     0.536f    /* a parte da imagem que é a placa (o resto é o poste do desenho) */

#define VAO_DO_PORTICO        13.0f     /* altura livre embaixo da viga */
#define LARGURA_DO_PORTICO    43.3f
#define ALTURA_DA_VIGA        5.4f
#define FUNDO_DA_VIGA         1.6f
#define LADO_DA_COLUNA        1.9f
#define TOPO_DA_VIGA_NA_IMAGEM 0.07f    /* onde a viga está na imagem do pórtico, de cima para baixo */
#define BASE_DA_VIGA_NA_IMAGEM 0.34f

/* ---------- Céu e fundo (cilindros em volta da câmera) ---------- */
#define LADOS_DO_CILINDRO     72
#define RAIO_DO_CEU           9000.0f
#define RAIO_DA_CIDADE        4200.0f
#define RAIO_DOS_MORROS       3200.0f
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
    Mesh objetos;           /* árvores, pneus, postes: formas com cores nos vértices */
    Mesh placas[3];         /* a frente de cada placa, com a imagem dela */
    Mesh viga;              /* a viga do pórtico, com a imagem dele */
} MalhasDaPista;

static Shader shader;
static Material material;   /* o mesmo material, trocando a textura a cada desenho */
static Model estadio;
static Mesh pinturaDoEstadio;
static MalhasDaPista interlagos;
static Mesh cilindroDoCeu, cilindroDaCidade, cilindroDosMorros;
static Texture2D texturaCeu, texturaCidade, texturaMorros;
static Texture2D texturaAsfalto[2], texturaGrama;
static Texture2D texturaPlacas[3], texturaPortico;

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
 * Objetos 3D de Interlagos
 * ================================================================== */

/* Sorteio próprio, como em obstaculos.c: a mesma semente dá sempre as mesmas formas */
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
 * corComLuz
 * Clareia ou escurece a cor conforme a face olha para o sol. Os objetos não
 * mudam de lugar, então a luz é calculada uma vez, na construção.
 * Recebe: a cor e a direção para onde a face aponta.
 * Devolve: a cor iluminada.
 */
static Color corComLuz(Color cor, Vector3 normal) {
    Vector3 sol = Vector3Normalize((Vector3){ 0.35f, 0.8f, 0.45f });
    Vector3 n = Vector3Normalize(normal);
    float luz = 0.58f + 0.30f * (n.y * 0.5f + 0.5f) + 0.38f * fmaxf(Vector3DotProduct(n, sol), 0.0f);
    return (Color){ (unsigned char) fminf(cor.r * luz, 255.0f), (unsigned char) fminf(cor.g * luz, 255.0f),
                    (unsigned char) fminf(cor.b * luz, 255.0f), 255 };
}

/*
 * adicionarTriangulo
 * Um triângulo de uma cor só, iluminado pela própria face.
 * Recebe: a construção, os 3 pontos e a cor.
 * Devolve: nada.
 */
static void adicionarTriangulo(Construcao *c, Vector3 a, Vector3 b, Vector3 d, Color cor) {
    Vector3 normal = Vector3CrossProduct(Vector3Subtract(b, a), Vector3Subtract(d, a));
    Color comLuz = corComLuz(cor, normal);
    Vector2 zero = { 0.0f, 0.0f };
    adicionarVertice(c, a, zero, comLuz);
    adicionarVertice(c, b, zero, comLuz);
    adicionarVertice(c, d, zero, comLuz);
}

/*
 * adicionarFace
 * Um quadrilátero de uma cor só, iluminado pela própria face.
 * Recebe: a construção, os 4 pontos em volta dele e a cor.
 * Devolve: nada.
 */
static void adicionarFace(Construcao *c, Vector3 a, Vector3 b, Vector3 d, Vector3 e, Color cor) {
    adicionarTriangulo(c, a, b, d, cor);
    adicionarTriangulo(c, a, d, e, cor);
}

/*
 * adicionarCaixa
 * Uma caixa em pé, alinhada com as direções dadas.
 * Recebe: a construção, o centro da base, as direções do lado e da frente
 *         (já unitárias), a largura, o fundo, a altura e a cor.
 * Devolve: nada.
 */
static void adicionarCaixa(Construcao *c, Vector3 base, Vector3 lado, Vector3 frente,
                           float largura, float fundo, float altura, Color cor) {
    Vector3 l = Vector3Scale(lado, largura / 2.0f), f = Vector3Scale(frente, fundo / 2.0f);
    Vector3 cima = { 0.0f, altura, 0.0f };
    Vector3 p[4] = { Vector3Subtract(Vector3Subtract(base, l), f), Vector3Subtract(Vector3Add(base, l), f),
                     Vector3Add(Vector3Add(base, l), f), Vector3Add(Vector3Subtract(base, l), f) };
    Vector3 q[4];
    for (int i = 0; i < 4; i++) {
        q[i] = Vector3Add(p[i], cima);
    }
    for (int i = 0; i < 4; i++) {
        int j = (i + 1) % 4;
        /* A luz depende da direção da face: aqui ela é medida a partir do centro */
        Vector3 meio = Vector3Scale(Vector3Add(p[i], p[j]), 0.5f);
        Color comLuz = corComLuz(cor, Vector3Subtract(meio, base));
        Vector2 zero = { 0.0f, 0.0f };
        adicionarQuadrilatero(c, p[i], p[j], q[j], q[i], zero, zero, zero, zero, comLuz);
    }
    adicionarFace(c, q[0], q[3], q[2], q[1], cor);
}

/* Os 12 vértices e as 20 faces de um icosaedro: a base das copas */
#define P_ICO 1.618034f
static const Vector3 VERTICES_ICO[12] = {
    { -1,  P_ICO, 0 }, { 1,  P_ICO, 0 }, { -1, -P_ICO, 0 }, { 1, -P_ICO, 0 },
    { 0, -1,  P_ICO }, { 0, 1,  P_ICO }, { 0, -1, -P_ICO }, { 0, 1, -P_ICO },
    {  P_ICO, 0, -1 }, {  P_ICO, 0, 1 }, { -P_ICO, 0, -1 }, { -P_ICO, 0, 1 },
};
static const unsigned char FACES_ICO[20][3] = {
    {0,11,5},{0,5,1},{0,1,7},{0,7,10},{0,10,11},{1,5,9},{5,11,4},{11,10,2},{10,7,6},{7,1,8},
    {3,9,4},{3,4,2},{3,2,6},{3,6,8},{3,8,9},{4,9,5},{2,4,11},{6,2,10},{8,6,7},{9,8,1},
};

/*
 * deformarNaBola
 * Põe um ponto na superfície de uma bola amassada. O amassado depende só da
 * direção, para as faces vizinhas continuarem encostadas.
 * Recebe: a direção, o centro, os raios e o desenho do amassado.
 * Devolve: o ponto.
 */
static Vector3 deformarNaBola(Vector3 direcao, Vector3 centro, Vector3 raios, float desenho) {
    Vector3 n = Vector3Normalize(direcao);
    float amassado = 1.0f + 0.13f * sinf(n.x * 5.3f + desenho) * cosf(n.y * 4.1f + desenho * 2.0f) * sinf(n.z * 4.7f + desenho * 0.5f);
    return (Vector3){ centro.x + n.x * raios.x * amassado, centro.y + n.y * raios.y * amassado,
                      centro.z + n.z * raios.z * amassado };
}

/*
 * adicionarBolaDeFolhas
 * Uma bola de folhas facetada: um icosaedro com cada face dividida em 4.
 * Recebe: a construção, o centro, os raios e a cor.
 * Devolve: nada.
 */
static void adicionarBolaDeFolhas(Construcao *c, Vector3 centro, Vector3 raios, Color cor) {
    float desenho = sortearObjeto(0.0f, 6.28f);
    for (int f = 0; f < 20; f++) {
        Vector3 a = VERTICES_ICO[FACES_ICO[f][0]], b = VERTICES_ICO[FACES_ICO[f][1]], d = VERTICES_ICO[FACES_ICO[f][2]];
        Vector3 ab = Vector3Add(a, b), bd = Vector3Add(b, d), da = Vector3Add(d, a);   /* os meios das arestas, antes de ir para a bola */
        Vector3 pa = deformarNaBola(a, centro, raios, desenho), pb = deformarNaBola(b, centro, raios, desenho);
        Vector3 pd = deformarNaBola(d, centro, raios, desenho), pab = deformarNaBola(ab, centro, raios, desenho);
        Vector3 pbd = deformarNaBola(bd, centro, raios, desenho), pda = deformarNaBola(da, centro, raios, desenho);

        /* Cada face tem um tom um pouco diferente, como tufos de folhas */
        float tom = sortearObjeto(0.86f, 1.14f);
        Color folhas = { (unsigned char) fminf(cor.r * tom, 255.0f), (unsigned char) fminf(cor.g * tom, 255.0f),
                         (unsigned char) fminf(cor.b * tom, 255.0f), 255 };
        adicionarTriangulo(c, pa, pab, pda, folhas);
        adicionarTriangulo(c, pab, pb, pbd, folhas);
        adicionarTriangulo(c, pda, pbd, pd, folhas);
        adicionarTriangulo(c, pab, pbd, pda, folhas);
    }
}

/*
 * adicionarArvore
 * Tronco afilado e uma copa feita de algumas bolas de folhas. Cada árvore sai
 * diferente da outra.
 * Recebe: a construção e o pé da árvore.
 * Devolve: nada.
 */
static void adicionarArvore(Construcao *c, Vector3 pe) {
    float altura = sortearObjeto(ALTURA_MINIMA_ARVORE, ALTURA_MAXIMA_ARVORE);
    float raioDaCopa = altura * sortearObjeto(0.28f, 0.38f);
    float raioDoTronco = altura * 0.035f;
    float topoDoTronco = altura * 0.55f;

    /* Tronco: 6 lados, mais fino em cima */
    for (int i = 0; i < 6; i++) {
        float a0 = 2.0f * PI * i / 6.0f, a1 = 2.0f * PI * (i + 1) / 6.0f;
        Vector3 b0 = { pe.x + cosf(a0) * raioDoTronco, pe.y - ABAIXO_DO_CHAO, pe.z + sinf(a0) * raioDoTronco };
        Vector3 b1 = { pe.x + cosf(a1) * raioDoTronco, pe.y - ABAIXO_DO_CHAO, pe.z + sinf(a1) * raioDoTronco };
        Vector3 t0 = { pe.x + cosf(a0) * raioDoTronco * 0.6f, pe.y + topoDoTronco, pe.z + sinf(a0) * raioDoTronco * 0.6f };
        Vector3 t1 = { pe.x + cosf(a1) * raioDoTronco * 0.6f, pe.y + topoDoTronco, pe.z + sinf(a1) * raioDoTronco * 0.6f };
        adicionarFace(c, b0, t0, t1, b1, COR_DO_TRONCO);
    }

    /* Copa: uma bola grande no alto e outras em volta, mais baixas */
    float variacao = sortearObjeto(0.85f, 1.1f);
    Color copa = { (unsigned char) (COR_DA_COPA.r * variacao), (unsigned char) (COR_DA_COPA.g * variacao),
                   (unsigned char) (COR_DA_COPA.b * variacao), 255 };
    float yCopa = pe.y + altura - raioDaCopa * 0.75f;
    adicionarBolaDeFolhas(c, (Vector3){ pe.x, yCopa, pe.z },
                          (Vector3){ raioDaCopa * 0.8f, raioDaCopa * 0.78f, raioDaCopa * 0.8f }, copa);
    for (int i = 0; i < 3; i++) {
        float angulo = 2.0f * PI * i / 3.0f + sortearObjeto(-0.5f, 0.5f);
        float raio = raioDaCopa * sortearObjeto(0.55f, 0.72f);
        Vector3 centro = { pe.x + cosf(angulo) * raioDaCopa * 0.55f, yCopa - raioDaCopa * sortearObjeto(0.15f, 0.45f),
                           pe.z + sinf(angulo) * raioDaCopa * 0.55f };
        adicionarBolaDeFolhas(c, centro, (Vector3){ raio, raio * 0.85f, raio }, copa);
    }
}

/*
 * adicionarPilhaDePneus
 * Uma pilha de pneus deitados. Cada pneu é um anel com a banda arredondada,
 * e o de cima é pintado, com o furo escuro no meio.
 * Recebe: a construção, a base da pilha e a cor do pneu de cima.
 * Devolve: nada.
 */
static void adicionarPilhaDePneus(Construcao *c, Vector3 base, Color corDoTopo) {
    for (int p = 0; p < PNEUS_POR_PILHA; p++) {
        Color cor = (p == PNEUS_POR_PILHA - 1) ? corDoTopo : COR_DA_BORRACHA;
        float y0 = base.y + p * ALTURA_DO_PNEU, yMeio = y0 + ALTURA_DO_PNEU / 2.0f, y1 = y0 + ALTURA_DO_PNEU;
        float raioNaEmenda = RAIO_DO_PNEU * 0.86f;
        for (int i = 0; i < LADOS_DO_PNEU; i++) {
            float a0 = 2.0f * PI * i / LADOS_DO_PNEU, a1 = 2.0f * PI * (i + 1) / LADOS_DO_PNEU;
            float c0 = cosf(a0), s0 = sinf(a0), c1 = cosf(a1), s1 = sinf(a1);
            Vector3 em0 = { base.x + c0 * raioNaEmenda, y0, base.z + s0 * raioNaEmenda };
            Vector3 em1 = { base.x + c1 * raioNaEmenda, y0, base.z + s1 * raioNaEmenda };
            Vector3 me0 = { base.x + c0 * RAIO_DO_PNEU, yMeio, base.z + s0 * RAIO_DO_PNEU };
            Vector3 me1 = { base.x + c1 * RAIO_DO_PNEU, yMeio, base.z + s1 * RAIO_DO_PNEU };
            Vector3 al0 = { base.x + c0 * raioNaEmenda, y1, base.z + s0 * raioNaEmenda };
            Vector3 al1 = { base.x + c1 * raioNaEmenda, y1, base.z + s1 * raioNaEmenda };
            adicionarFace(c, em0, me0, me1, em1, cor);
            adicionarFace(c, me0, al0, al1, me1, cor);
        }
    }

    /* O alto: o anel do pneu e o furo */
    float yAlto = base.y + PNEUS_POR_PILHA * ALTURA_DO_PNEU;
    float raioNaEmenda = RAIO_DO_PNEU * 0.86f, raioDoFuro = RAIO_DO_PNEU * 0.48f;
    Vector3 centro = { base.x, yAlto - ALTURA_DO_PNEU * 0.7f, base.z };
    for (int i = 0; i < LADOS_DO_PNEU; i++) {
        float a0 = 2.0f * PI * i / LADOS_DO_PNEU, a1 = 2.0f * PI * (i + 1) / LADOS_DO_PNEU;
        Vector3 fora0 = { base.x + cosf(a0) * raioNaEmenda, yAlto, base.z + sinf(a0) * raioNaEmenda };
        Vector3 fora1 = { base.x + cosf(a1) * raioNaEmenda, yAlto, base.z + sinf(a1) * raioNaEmenda };
        Vector3 furo0 = { base.x + cosf(a0) * raioDoFuro, yAlto, base.z + sinf(a0) * raioDoFuro };
        Vector3 furo1 = { base.x + cosf(a1) * raioDoFuro, yAlto, base.z + sinf(a1) * raioDoFuro };
        adicionarFace(c, fora0, furo0, furo1, fora1, corDoTopo);
        adicionarTriangulo(c, furo0, centro, furo1, COR_DO_FURO);
    }
}

/*
 * adicionarBarreira
 * Uma barreira de pneus acompanhando a pista: as pilhas seguem a curva, cada
 * uma no seu lugar, com o topo vermelho e branco alternando.
 * Recebe: a construção, o segmento e a lateral da barreira.
 * Devolve: nada.
 */
static void adicionarBarreira(Construcao *c, Segmento *segmento, float lateral) {
    for (int i = 0; i < PILHAS_POR_BARREIRA; i++) {
        PosicaoNaPista lugar = { segmento, 0.0f };
        moverNaPista(&lugar, (float) i / PILHAS_POR_BARREIRA * 6.0f * COMPRIMENTO_SEGMENTO);
        PontoNoMundo ponto = pontoNoMundo(lugar, lateral);
        Vector3 base = { ponto.x, alturaNaGrade(ponto.x, ponto.z) - 0.05f, ponto.z };
        int vermelho = (segmento->indice / 6 + i) % 2;
        adicionarPilhaDePneus(c, base, vermelho ? COR_PNEU_VERMELHO : COR_PNEU_BRANCO);
    }
}

/*
 * adicionarPlaca
 * Uma placa de frenagem: poste, costas pintadas e, numa malha à parte, a
 * frente com a imagem do número, olhando para os carros que vêm.
 * Recebe: a malha da frente, a de cores, o ponto e a direção da pista.
 * Devolve: nada.
 */
static void adicionarPlaca(Construcao *frente, Construcao *cores, Vector3 pe, float rumo) {
    Vector3 sentido = { sinf(rumo), 0.0f, -cosf(rumo) };   /* para onde os carros vão */
    Vector3 lado = { cosf(rumo), 0.0f, sinf(rumo) };
    float topo = pe.y + ALTURA_DA_PLACA, base = topo - LADO_DA_PLACA;
    Vector3 poste = { pe.x, pe.y - ABAIXO_DO_CHAO, pe.z };
    adicionarCaixa(cores, poste, lado, sentido, 0.32f, 0.32f, ALTURA_DA_PLACA - 0.8f + ABAIXO_DO_CHAO, COR_DO_POSTE);

    /* Costas da placa, para ela ter corpo: a placa fica na frente do poste, do lado dos carros */
    Vector3 atras = Vector3Subtract(pe, Vector3Scale(sentido, RECUO_DA_PLACA));
    adicionarCaixa(cores, (Vector3){ atras.x, base, atras.z }, lado, sentido, LADO_DA_PLACA * 0.94f, ESPESSURA_DA_PLACA,
                   LADO_DA_PLACA * 0.94f, COR_DAS_COSTAS);

    Vector3 l = Vector3Scale(lado, LADO_DA_PLACA / 2.0f);
    Vector3 face = Vector3Subtract(pe, Vector3Scale(sentido, RECUO_DA_PLACA + ESPESSURA_DA_PLACA * 0.6f));
    Vector3 a = { face.x - l.x, topo, face.z - l.z }, b = { face.x - l.x, base, face.z - l.z };
    Vector3 d = { face.x + l.x, base, face.z + l.z }, e = { face.x + l.x, topo, face.z + l.z };
    adicionarQuadrilatero(frente, a, b, d, e, (Vector2){ 0.0f, 0.0f }, (Vector2){ 0.0f, FRACAO_DO_DESENHO },
                          (Vector2){ 1.0f, FRACAO_DO_DESENHO }, (Vector2){ 1.0f, 0.0f }, WHITE);
}

/*
 * adicionarPortico
 * O pórtico da largada: duas colunas e uma viga sólidas sobre a pista, com a
 * imagem do pórtico na frente e atrás da viga.
 * Recebe: a malha da viga, a de cores e a posição dele na pista.
 * Devolve: nada.
 */
static void adicionarPortico(Construcao *viga, Construcao *cores, PosicaoNaPista lugar) {
    PontoNoMundo ponto = pontoNoMundo(lugar, 0.0f);
    float rumo = ponto.rumo * DEG2RAD;
    Vector3 sentido = { sinf(rumo), 0.0f, -cosf(rumo) };
    Vector3 lado = { cosf(rumo), 0.0f, sinf(rumo) };
    Vector3 meio = { ponto.x, ponto.y, ponto.z };
    float meiaLargura = LARGURA_DO_PORTICO / 2.0f;
    float altoDaViga = ponto.y + VAO_DO_PORTICO + ALTURA_DA_VIGA;

    for (int s = -1; s <= 1; s += 2) {
        Vector3 pe = Vector3Add(meio, Vector3Scale(lado, s * (meiaLargura - LADO_DA_COLUNA)));
        pe.y -= ABAIXO_DO_CHAO;
        adicionarCaixa(cores, pe, lado, sentido, LADO_DA_COLUNA, LADO_DA_COLUNA,
                       VAO_DO_PORTICO + ABAIXO_DO_CHAO, COR_DO_METAL);
    }

    /* O corpo da viga, um pouco menor que as faces com a imagem */
    adicionarCaixa(cores, (Vector3){ meio.x, ponto.y + VAO_DO_PORTICO + 0.02f, meio.z }, lado, sentido,
                   LARGURA_DO_PORTICO - 0.04f, FUNDO_DA_VIGA - 0.04f, ALTURA_DA_VIGA - 0.04f, COR_DA_BORRACHA);

    /* A imagem nas duas faces. Quem olha a face de trás tem a direita trocada;
     * por isso o lado inverte, e o texto é lido certo de qualquer direção */
    for (int s = -1; s <= 1; s += 2) {
        Vector3 centro = Vector3Add(meio, Vector3Scale(sentido, s * FUNDO_DA_VIGA / 2.0f));
        Vector3 l = Vector3Scale(lado, s * meiaLargura);
        Vector3 a = { centro.x + l.x, altoDaViga, centro.z + l.z };
        Vector3 b = { centro.x + l.x, altoDaViga - ALTURA_DA_VIGA, centro.z + l.z };
        Vector3 d = { centro.x - l.x, altoDaViga - ALTURA_DA_VIGA, centro.z - l.z };
        Vector3 e = { centro.x - l.x, altoDaViga, centro.z - l.z };
        adicionarQuadrilatero(viga, a, b, d, e, (Vector2){ 0.0f, TOPO_DA_VIGA_NA_IMAGEM }, (Vector2){ 0.0f, BASE_DA_VIGA_NA_IMAGEM },
                              (Vector2){ 1.0f, BASE_DA_VIGA_NA_IMAGEM }, (Vector2){ 1.0f, TOPO_DA_VIGA_NA_IMAGEM }, WHITE);
    }
}

/*
 * construirObjetos
 * Anda pela lista de objetos de cada segmento e monta as formas 3D deles.
 * Recebe: a pista.
 * Devolve: nada (preenche as malhas de "interlagos").
 */
static void construirObjetos(Pista *pista) {
    Construcao cores = { 0 }, viga = { 0 }, placas[3] = { { 0 }, { 0 }, { 0 } };
    sementeDosObjetos = SEMENTE_DOS_OBJETOS;

    Segmento *atual = pista->largada;
    do {
        PosicaoNaPista lugar = { atual, 0.0f };
        for (Obstaculo *objeto = atual->obstaculos; objeto != NULL; objeto = objeto->proximo) {
            PontoNoMundo ponto = pontoNoMundo(lugar, objeto->deslocamentoLateral);
            Vector3 pe = { ponto.x, alturaNaGrade(ponto.x, ponto.z), ponto.z };
            switch (objeto->tipo) {
                case OBJETO_ARVORE:
                    adicionarArvore(&cores, pe);
                    break;
                case OBJETO_PNEUS:
                    adicionarBarreira(&cores, atual, objeto->deslocamentoLateral);
                    break;
                case OBJETO_PLACA_50:
                case OBJETO_PLACA_100:
                case OBJETO_PLACA_150:
                    adicionarPlaca(&placas[objeto->tipo - OBJETO_PLACA_50], &cores, pe, ponto.rumo * DEG2RAD);
                    break;
                case OBJETO_PORTICO:
                    adicionarPortico(&viga, &cores, lugar);
                    break;
                default:
                    break;   /* as marcas do grid são pintadas no asfalto */
            }
        }
        atual = atual->proximo;
    } while (atual != pista->largada);

    MemFree(alturasDaGrade);
    alturasDaGrade = NULL;

    interlagos.objetos = terminarMalha(&cores);
    interlagos.viga = terminarMalha(&viga);
    for (int i = 0; i < 3; i++) {
        interlagos.placas[i] = terminarMalha(&placas[i]);
    }
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
 * carregarEstadio
 * Carrega o modelo da Copa Pistão e deixa as texturas dele nítidas ao longe.
 * Recebe: a pista (para pintar a largada e o grid).
 * Devolve: nada.
 */
static void carregarEstadio(Pista *pista) {
    estadio = LoadModel(ARQUIVO_DO_ESTADIO);
    for (int i = 0; i < estadio.materialCount; i++) {
        estadio.materials[i].shader = shader;
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
    texturaPlacas[0] = carregarTexturaRepetida("sprites/placa_50.png");
    texturaPlacas[1] = carregarTexturaRepetida("sprites/placa_100.png");
    texturaPlacas[2] = carregarTexturaRepetida("sprites/placa_150.png");
    texturaPortico = carregarTexturaRepetida("sprites/portico.png");

    /* A imagem ocupa a altura proporcional à largura de cada repetição, sem esticar */
    float voltaCeu = 2.0f * PI * RAIO_DO_CEU / REPETICOES_CEU;
    float voltaCidade = 2.0f * PI * RAIO_DA_CIDADE / REPETICOES_CIDADE;
    float voltaMorros = 2.0f * PI * RAIO_DOS_MORROS / REPETICOES_MORROS;
    cilindroDoCeu = construirCilindro(RAIO_DO_CEU, voltaCeu * texturaCeu.height / texturaCeu.width, REPETICOES_CEU);
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
    UnloadMesh(interlagos.objetos);
    UnloadMesh(interlagos.viga);
    for (int i = 0; i < 3; i++) {
        UnloadMesh(interlagos.placas[i]);
        UnloadTexture(texturaPlacas[i]);
    }
    UnloadTexture(texturaPortico);
    UnloadMesh(cilindroDoCeu);
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

    desenharMalha(cilindroDoCeu, texturaCeu, nosCilindros);
    if (id == PISTA_COPA_PISTAO) {
        DrawModel(estadio, (Vector3){ 0.0f, 0.0f, 0.0f }, ESCALA_COPA_PISTAO, WHITE);
        desenharMalha(pinturaDoEstadio, semTextura, origem);
        return;
    }

    desenharMalha(cilindroDaCidade, texturaCidade, nosCilindros);
    desenharMalha(cilindroDosMorros, texturaMorros, nosCilindros);
    desenharMalha(interlagos.base, texturaGrama, origem);
    desenharMalha(interlagos.terreno, texturaGrama, origem);
    desenharMalha(interlagos.acostamento, texturaGrama, origem);
    desenharMalha(interlagos.asfalto[0], texturaAsfalto[0], origem);
    desenharMalha(interlagos.asfalto[1], texturaAsfalto[1], origem);
    desenharMalha(interlagos.pintura, semTextura, origem);
    desenharMalha(interlagos.objetos, semTextura, origem);
    desenharMalha(interlagos.viga, texturaPortico, origem);
    for (int i = 0; i < 3; i++) {
        desenharMalha(interlagos.placas[i], texturaPlacas[i], origem);
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
