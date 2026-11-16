/*
 * holofotes.c
 * ------------------------------------------------------------------
 * As torres de holofotes das corridas de noite. Veja holofotes.h.
 *
 * Poucas e grandes, como as de estádio: uma a cada ENTRE_AS_TORRES metros de
 * pista, alternando os lados, atrás do muro (o "limite" medido de cada lado),
 * longe o bastante da borda para não ficarem em cima da área de escape. Ao
 * lado do pit lane elas vão para o outro lado, e onde atravessariam o cenário
 * (uma arquibancada, um prédio, uma árvore) procuram um lugar livre perto.
 *
 * A torre é uma malha 3D montada aqui, uma vez: a base de concreto, a treliça
 * (quatro pernas que afinam para cima, os anéis e as diagonais de cada face),
 * os braços até o painel, o painel inclinado para a pista com as lâmpadas em
 * grade e a passarela de manutenção embaixo dele. As faces vêm sombreadas pela
 * direção delas (o alto e o lado da pista mais claros), e as lâmpadas acesas.
 *
 * A luz de cada torre é uma lâmpada pontual de hora.h (grande e forte), e as
 * mais à vista vão ao shader; o halo de cada lâmpada só aparece de frente.
 * ------------------------------------------------------------------
 */

#include <math.h>
#include <stdlib.h>
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "holofotes.h"
#include "luzes.h"
#include "mundo.h"

/* Onde ficam */
#define ENTRE_AS_TORRES       240.0f   /* metros de pista de uma torre à próxima (do outro lado) */
#define ALEM_DO_MURO           10.0f   /* metros atrás do muro */
#define MAIS_PERTO_DA_BORDA    16.0f   /* metros da borda do asfalto */
#define MAIS_LONGE_DA_BORDA    40.0f
#define SEM_MURO_MEDIDO        22.0f   /* da borda, onde o muro não foi medido */
#define FOLGA_DO_PIT           60.0f   /* metros antes e depois do pit lane em que as torres mudam de lado */
#define MAIS_ATRAS             12.0f   /* metros: cada tentativa a mais de achar um lugar livre vai tanto para trás... */
#define PARA_O_LADO            30.0f   /* ...ou tanto para a frente ou para trás na pista */
#define MAIS_TORRES            128
/* A luz */
#define RAIO_DA_LUZ           220.0f   /* metros: até onde a luz de uma torre chega */
#define FORCA_DA_LUZ            1.6f
#define COR_DA_LUZ            (Vector3){ 1.00f, 0.985f, 0.95f }   /* branco: puxa um nada para o quente, contra o azul da lua */
#define TORRES_ACESAS           6      /* as que vão ao shader: as mais à vista (cada uma custa ~0,35 ms por quadro) */
#define VISTAS_ATE            2400.0f  /* metros: as torres mais longe que isso não são desenhadas */
/* A torre (no espaço dela: o pé na origem, o +z virado para a pista) */
#define ALTURA_DA_TRELICA      40.0f
#define BASE_DA_TRELICA         1.2f   /* a altura da base de concreto */
#define LARGURA_EMBAIXO         1.8f   /* metade do lado da treliça no pé... */
#define LARGURA_EM_CIMA         0.8f   /* ...e no alto */
#define ENTRE_OS_ANEIS          4.0f
#define MEIO_DO_PAINEL         (Vector3){ 0.0f, 45.0f, 1.2f }
#define LARGURA_DO_PAINEL      13.0f
#define ALTURA_DO_PAINEL        6.4f
#define INCLINACAO_DO_PAINEL   24.0f   /* graus: o painel olha para baixo, para o asfalto */
#define LINHAS_DE_LAMPADAS      3
#define COLUNAS_DE_LAMPADAS     6
#define TOTAL_DE_LAMPADAS      (LINHAS_DE_LAMPADAS * COLUNAS_DE_LAMPADAS)

#define COR_DO_CONCRETO   (Color){ 112, 110, 104, 255 }
#define COR_DO_ACO        (Color){ 128, 134, 146, 255 }
#define COR_DO_PAINEL     (Color){  58,  61,  70, 255 }
#define COR_DA_CAIXA      (Color){  74,  77,  86, 255 }
#define COR_DA_LAMPADA    (Color){ 255, 251, 240, 255 }
#define COR_DO_HALO       (Color){ 255, 250, 238, 255 }

typedef struct {
    Vector3 pe;          /* no chão */
    float giro;          /* graus, em volta do eixo y: o que leva o +z da torre para a pista */
    Vector3 luz;         /* o meio do painel, no mundo */
    Vector3 facho;       /* para onde o painel olha, no mundo */
} Torre;

static Torre torres[MAIS_TORRES];
static Lampada luzes[MAIS_TORRES];
static int totalDeTorres = 0;
static int pistaDasTorres = -1;

static Model modeloDaTorre;
static int temModelo = 0;
static Vector3 lampadasNaTorre[TOTAL_DE_LAMPADAS];   /* o meio da frente de cada lâmpada, no espaço da torre */
static Vector3 meioDoPainel, frenteDoPainel;          /* no espaço da torre */
static Texture2D brilho;
static int temBrilho = 0;

/* ---------- A malha da torre ---------- */

typedef struct {
    float *vertices;
    unsigned char *cores;
    int total, cabe;
} Montagem;

static void pontoDaMalha(Montagem *m, Vector3 p, Color cor) {
    if (m->total >= m->cabe) {
        m->cabe = m->cabe ? m->cabe * 2 : 4096;
        m->vertices = realloc(m->vertices, sizeof(float) * 3 * (size_t) m->cabe);
        m->cores = realloc(m->cores, 4 * (size_t) m->cabe);
    }
    m->vertices[m->total * 3] = p.x;
    m->vertices[m->total * 3 + 1] = p.y;
    m->vertices[m->total * 3 + 2] = p.z;
    m->cores[m->total * 4] = cor.r;
    m->cores[m->total * 4 + 1] = cor.g;
    m->cores[m->total * 4 + 2] = cor.b;
    m->cores[m->total * 4 + 3] = 255;
    m->total++;
}

/*
 * sombrear
 * A cor de uma face pela direção dela: a de cima e a virada para a pista (que as outras torres iluminam) mais
 * claras, a de baixo e a de trás mais escuras.
 */
static Color sombrear(Color cor, Vector3 normal) {
    float k = 0.58f + 0.26f * normal.y + 0.18f * normal.z - 0.06f * fabsf(normal.x);
    k = Clamp(k, 0.22f, 1.0f);
    return (Color){ (unsigned char) (cor.r * k), (unsigned char) (cor.g * k), (unsigned char) (cor.b * k), 255 };
}

/* Uma face de quatro cantos, virada para "normal" (o sentido dos triângulos sai dela) */
static void face(Montagem *m, Vector3 a, Vector3 b, Vector3 c, Vector3 d, Vector3 normal, Color cor, int acesa) {
    Color c2 = acesa ? cor : sombrear(cor, normal);
    if (Vector3DotProduct(Vector3CrossProduct(Vector3Subtract(b, a), Vector3Subtract(c, a)), normal) < 0.0f) {
        Vector3 t = b;
        b = d;
        d = t;
    }
    pontoDaMalha(m, a, c2);
    pontoDaMalha(m, b, c2);
    pontoDaMalha(m, c, c2);
    pontoDaMalha(m, a, c2);
    pontoDaMalha(m, c, c2);
    pontoDaMalha(m, d, c2);
}

/*
 * caixa
 * Uma caixa com o meio, os três eixos (unitários) e a metade do tamanho em cada um.
 */
static void caixa(Montagem *m, Vector3 meio, Vector3 ex, Vector3 ey, Vector3 ez, Vector3 metade, Color cor) {
    Vector3 x = Vector3Scale(ex, metade.x), y = Vector3Scale(ey, metade.y), z = Vector3Scale(ez, metade.z);
    Vector3 eixos[3] = { x, y, z };
    Vector3 unitarios[3] = { ex, ey, ez };
    for (int e = 0; e < 3; e++) {
        Vector3 u = eixos[(e + 1) % 3], v = eixos[(e + 2) % 3];
        for (int lado = -1; lado <= 1; lado += 2) {
            Vector3 c = Vector3Add(meio, Vector3Scale(eixos[e], (float) lado));
            Vector3 a = Vector3Subtract(Vector3Subtract(c, u), v), b = Vector3Subtract(Vector3Add(c, u), v);
            Vector3 cc = Vector3Add(Vector3Add(c, u), v), d = Vector3Add(Vector3Subtract(c, u), v);
            face(m, a, b, cc, d, Vector3Scale(unitarios[e], (float) lado), cor, 0);
        }
    }
}

/* Uma viga de seção quadrada de um ponto a outro */
static void viga(Montagem *m, Vector3 de, Vector3 ate, float grossura, Color cor) {
    Vector3 eixo = Vector3Subtract(ate, de);
    float comprimento = Vector3Length(eixo);
    if (comprimento < 0.001f) {
        return;
    }
    Vector3 ez = Vector3Scale(eixo, 1.0f / comprimento);
    Vector3 cima = (fabsf(ez.y) > 0.9f) ? (Vector3){ 1.0f, 0.0f, 0.0f } : (Vector3){ 0.0f, 1.0f, 0.0f };
    Vector3 ex = Vector3Normalize(Vector3CrossProduct(cima, ez));
    Vector3 ey = Vector3CrossProduct(ez, ex);
    caixa(m, Vector3Lerp(de, ate, 0.5f), ex, ey, ez, (Vector3){ grossura / 2.0f, grossura / 2.0f, comprimento / 2.0f }, cor);
}

/* O canto da treliça numa altura: a perna que afina para cima */
static Vector3 cantoDaTrelica(int canto, float y) {
    float t = (y - BASE_DA_TRELICA) / (ALTURA_DA_TRELICA - BASE_DA_TRELICA);
    float meia = LARGURA_EMBAIXO + (LARGURA_EM_CIMA - LARGURA_EMBAIXO) * t;
    float sx = (canto == 0 || canto == 3) ? -1.0f : 1.0f, sz = (canto < 2) ? -1.0f : 1.0f;
    return (Vector3){ sx * meia, y, sz * meia };
}

static void montarModeloDaTorre(void) {
    Montagem m = { 0 };
    Vector3 X = { 1.0f, 0.0f, 0.0f }, Y = { 0.0f, 1.0f, 0.0f }, Z = { 0.0f, 0.0f, 1.0f };

    /* A base de concreto */
    caixa(&m, (Vector3){ 0.0f, BASE_DA_TRELICA / 2.0f, 0.0f }, X, Y, Z,
          (Vector3){ LARGURA_EMBAIXO + 0.6f, BASE_DA_TRELICA / 2.0f, LARGURA_EMBAIXO + 0.6f }, COR_DO_CONCRETO);
    /* A treliça: as pernas, os anéis e as diagonais (uma por face e por andar, alternando o sentido) */
    for (int canto = 0; canto < 4; canto++) {
        viga(&m, cantoDaTrelica(canto, BASE_DA_TRELICA), cantoDaTrelica(canto, ALTURA_DA_TRELICA), 0.34f, COR_DO_ACO);
    }
    int andares = (int) ((ALTURA_DA_TRELICA - BASE_DA_TRELICA) / ENTRE_OS_ANEIS);
    for (int a = 0; a <= andares; a++) {
        float y = BASE_DA_TRELICA + (ALTURA_DA_TRELICA - BASE_DA_TRELICA) * (float) a / (float) andares;
        float acima = BASE_DA_TRELICA + (ALTURA_DA_TRELICA - BASE_DA_TRELICA) * (float) (a + 1) / (float) andares;
        for (int canto = 0; canto < 4; canto++) {
            int proximo = (canto + 1) % 4;
            viga(&m, cantoDaTrelica(canto, y), cantoDaTrelica(proximo, y), 0.16f, COR_DO_ACO);
            if (a < andares) {
                int deCima = (a % 2 == 0);
                viga(&m, cantoDaTrelica(deCima ? canto : proximo, y), cantoDaTrelica(deCima ? proximo : canto, acima), 0.13f, COR_DO_ACO);
            }
        }
    }

    /* O painel: inclinado para baixo, olhando para a pista (+z) */
    float inclinacao = INCLINACAO_DO_PAINEL * DEG2RAD;
    Vector3 px = X;
    Vector3 py = { 0.0f, cosf(inclinacao), sinf(inclinacao) };
    Vector3 pz = { 0.0f, -sinf(inclinacao), cosf(inclinacao) };
    Vector3 meio = MEIO_DO_PAINEL;
    meioDoPainel = meio;
    frenteDoPainel = pz;
    float meiaLargura = LARGURA_DO_PAINEL / 2.0f, meiaAltura = ALTURA_DO_PAINEL / 2.0f;
    /* A chapa de trás e a moldura */
    caixa(&m, Vector3Subtract(meio, Vector3Scale(pz, 0.35f)), px, py, pz, (Vector3){ meiaLargura, meiaAltura, 0.12f }, COR_DO_PAINEL);
    Vector3 cantos[4];
    for (int c = 0; c < 4; c++) {
        float sx = (c == 0 || c == 3) ? -1.0f : 1.0f, sy = (c < 2) ? -1.0f : 1.0f;
        cantos[c] = Vector3Add(meio, Vector3Add(Vector3Scale(px, sx * meiaLargura), Vector3Scale(py, sy * meiaAltura)));
    }
    for (int c = 0; c < 4; c++) {
        viga(&m, cantos[c], cantos[(c + 1) % 4], 0.32f, COR_DO_ACO);
    }
    for (int linha = 1; linha < LINHAS_DE_LAMPADAS; linha++) {   /* as travessas entre as linhas de lâmpadas */
        float sy = -meiaAltura + ALTURA_DO_PAINEL * (float) linha / (float) LINHAS_DE_LAMPADAS;
        Vector3 c = Vector3Add(meio, Vector3Scale(py, sy));
        viga(&m, Vector3Subtract(c, Vector3Scale(px, meiaLargura)), Vector3Add(c, Vector3Scale(px, meiaLargura)), 0.18f, COR_DO_ACO);
    }
    /* As lâmpadas: a caixa de cada uma e, na frente, o vidro aceso */
    float largura = LARGURA_DO_PAINEL / (float) COLUNAS_DE_LAMPADAS, altura = ALTURA_DO_PAINEL / (float) LINHAS_DE_LAMPADAS;
    for (int linha = 0; linha < LINHAS_DE_LAMPADAS; linha++) {
        for (int coluna = 0; coluna < COLUNAS_DE_LAMPADAS; coluna++) {
            float sx = -meiaLargura + largura * ((float) coluna + 0.5f), sy = -meiaAltura + altura * ((float) linha + 0.5f);
            Vector3 c = Vector3Add(meio, Vector3Add(Vector3Scale(px, sx), Vector3Scale(py, sy)));
            caixa(&m, Vector3Add(c, Vector3Scale(pz, 0.05f)), px, py, pz, (Vector3){ largura * 0.42f, altura * 0.40f, 0.40f }, COR_DA_CAIXA);
            Vector3 vidro = Vector3Add(c, Vector3Scale(pz, 0.47f));
            Vector3 u = Vector3Scale(px, largura * 0.34f), v = Vector3Scale(py, altura * 0.32f);
            face(&m, Vector3Subtract(Vector3Subtract(vidro, u), v), Vector3Subtract(Vector3Add(vidro, u), v),
                 Vector3Add(Vector3Add(vidro, u), v), Vector3Add(Vector3Subtract(vidro, u), v), pz, COR_DA_LAMPADA, 1);
            lampadasNaTorre[linha * COLUNAS_DE_LAMPADAS + coluna] = Vector3Add(vidro, Vector3Scale(pz, 0.05f));
        }
    }
    /* Os braços do alto da treliça até as costas do painel */
    Vector3 costas = Vector3Subtract(meio, Vector3Scale(pz, 0.5f));
    for (int lado = -1; lado <= 1; lado += 2) {
        Vector3 noPainelEmBaixo = Vector3Add(costas, Vector3Add(Vector3Scale(px, lado * meiaLargura * 0.6f), Vector3Scale(py, -meiaAltura)));
        Vector3 noPainelEmCima = Vector3Add(costas, Vector3Add(Vector3Scale(px, lado * meiaLargura * 0.6f), Vector3Scale(py, meiaAltura * 0.6f)));
        viga(&m, cantoDaTrelica(lado < 0 ? 0 : 1, ALTURA_DA_TRELICA), noPainelEmBaixo, 0.30f, COR_DO_ACO);
        viga(&m, cantoDaTrelica(lado < 0 ? 3 : 2, ALTURA_DA_TRELICA), noPainelEmCima, 0.30f, COR_DO_ACO);
        viga(&m, cantoDaTrelica(lado < 0 ? 0 : 1, ALTURA_DA_TRELICA), noPainelEmCima, 0.20f, COR_DO_ACO);
    }
    /* A passarela de manutenção embaixo do painel, com o guarda-corpo */
    Vector3 piso = Vector3Add(Vector3Subtract(meio, Vector3Scale(py, meiaAltura + 0.5f)), Vector3Scale(Z, 0.6f));
    piso.y = fminf(piso.y, ALTURA_DA_TRELICA + 0.6f);
    caixa(&m, piso, X, Y, Z, (Vector3){ meiaLargura + 0.3f, 0.07f, 0.75f }, COR_DO_ACO);
    for (int lado = -1; lado <= 1; lado += 2) {
        Vector3 frente = Vector3Add(piso, (Vector3){ lado * (meiaLargura + 0.3f), 0.0f, 0.7f });
        viga(&m, frente, Vector3Add(frente, (Vector3){ 0.0f, 1.1f, 0.0f }), 0.08f, COR_DO_ACO);
    }
    Vector3 corrimao = Vector3Add(piso, (Vector3){ 0.0f, 1.1f, 0.7f });
    viga(&m, Vector3Add(corrimao, (Vector3){ -(meiaLargura + 0.3f), 0.0f, 0.0f }), Vector3Add(corrimao, (Vector3){ meiaLargura + 0.3f, 0.0f, 0.0f }),
         0.08f, COR_DO_ACO);
    /* A escada: do chão até a passarela, num lado da treliça */
    viga(&m, (Vector3){ -0.25f, BASE_DA_TRELICA, -LARGURA_EMBAIXO - 0.25f }, (Vector3){ -0.25f, ALTURA_DA_TRELICA, -LARGURA_EM_CIMA - 0.25f }, 0.07f, COR_DO_ACO);
    viga(&m, (Vector3){ 0.25f, BASE_DA_TRELICA, -LARGURA_EMBAIXO - 0.25f }, (Vector3){ 0.25f, ALTURA_DA_TRELICA, -LARGURA_EM_CIMA - 0.25f }, 0.07f, COR_DO_ACO);

    Mesh malha = { 0 };
    malha.vertexCount = m.total;
    malha.triangleCount = m.total / 3;
    malha.vertices = m.vertices;
    malha.colors = m.cores;
    UploadMesh(&malha, false);
    modeloDaTorre = LoadModelFromMesh(malha);
    temModelo = 1;
}

/* ---------- Onde ficam ---------- */

/*
 * pertoDoPit
 * Recebe: a pista e um lugar dela, em metros da largada.
 * Devolve: 1 se ali o pit lane corre ao lado (com a folga antes e depois).
 */
static int pertoDoPit(const Pista *pista, float metros) {
    if (!pista->temPit) {
        return 0;
    }
    float volta = pista->metrosDaVolta;
    float de = pista->pit.entrada - FOLGA_DO_PIT;
    float tamanho = fmodf(pista->pit.fim + FOLGA_DO_PIT - de + 2.0f * volta, volta);
    return fmodf(metros - de + 2.0f * volta, volta) <= tamanho;
}

/* Um ponto do espaço da torre levado ao mundo */
static Vector3 naTorreParaOMundo(const Torre *t, Vector3 p) {
    float g = t->giro * DEG2RAD;
    return (Vector3){ t->pe.x + p.x * cosf(g) + p.z * sinf(g), t->pe.y + p.y, t->pe.z - p.x * sinf(g) + p.z * cosf(g) };
}

static Vector3 direcaoParaOMundo(const Torre *t, Vector3 d) {
    float g = t->giro * DEG2RAD;
    return (Vector3){ d.x * cosf(g) + d.z * sinf(g), d.y, -d.x * sinf(g) + d.z * cosf(g) };
}

/*
 * colocarTorre
 * A torre num lugar: o pé, o giro para a pista, a luz e o facho.
 * Recebe: a torre, o segmento, o lado (-1 ou +1) e quantos metros além da borda.
 * Devolve: 0 se ali não dá (a pista estreita demais para medir).
 */
static int colocarTorre(Torre *t, Segmento *s, int lado, float alem) {
    float largura = (lado > 0) ? s->larguraDireita : s->larguraEsquerda;
    if (largura < 1.0f) {
        return 0;
    }
    PosicaoNaPista lugar = { 0 };
    lugar.segmento = s;
    PontoNoMundo pe = pontoNoMundo(lugar, (float) lado * (largura + alem) / largura);
    PontoNoMundo meio = pontoNoMundo(lugar, 0.0f);
    t->pe = (Vector3){ pe.x, pe.y, pe.z };
    Vector3 paraAPista = Vector3Normalize((Vector3){ meio.x - pe.x, 0.0f, meio.z - pe.z });
    t->giro = atan2f(paraAPista.x, paraAPista.z) * RAD2DEG;
    t->luz = naTorreParaOMundo(t, meioDoPainel);
    t->facho = direcaoParaOMundo(t, frenteDoPainel);
    return 1;
}

/*
 * torreLivre
 * Se a torre não atravessa nada do cenário (uma arquibancada, um prédio, uma árvore): o eixo dela, uma diagonal
 * da base e a linha do painel.
 * Recebe: a pista e a torre.
 * Devolve: 1 se ali ela cabe.
 */
static int torreLivre(const Pista *pista, const Torre *t) {
    Vector3 eixoDe = Vector3Add(t->pe, (Vector3){ 0.0f, 3.0f, 0.0f });
    Vector3 eixoAte = Vector3Add(t->pe, (Vector3){ 0.0f, ALTURA_DA_TRELICA + 12.0f, 0.0f });
    if (!visaoLivreNoCenario(pista->id, eixoDe, eixoAte)) {
        return 0;
    }
    Vector3 base1 = naTorreParaOMundo(t, (Vector3){ -LARGURA_EMBAIXO - 4.0f, 7.0f, -LARGURA_EMBAIXO - 4.0f });
    Vector3 base2 = naTorreParaOMundo(t, (Vector3){ LARGURA_EMBAIXO + 4.0f, 7.0f, LARGURA_EMBAIXO + 4.0f });
    if (!visaoLivreNoCenario(pista->id, base1, base2)) {
        return 0;
    }
    Vector3 meio = MEIO_DO_PAINEL;
    Vector3 esquerda = naTorreParaOMundo(t, (Vector3){ -LARGURA_DO_PAINEL / 2.0f - 4.0f, meio.y, meio.z });
    Vector3 direita = naTorreParaOMundo(t, (Vector3){ LARGURA_DO_PAINEL / 2.0f + 4.0f, meio.y, meio.z });
    return visaoLivreNoCenario(pista->id, esquerda, direita);
}

/*
 * montarTorres
 * As torres da pista, uma vez por pista (o cenário já tem que estar carregado: as torres desviam dele). Cada uma
 * tenta o lugar de sempre e, se ele atravessa o cenário, mais para trás, do outro lado e um pouco para a frente ou
 * para trás na pista; sem lugar livre, ela fica no de sempre.
 * Recebe: a pista.
 * Devolve: nada.
 */
static void montarTorres(Pista *pista) {
    totalDeTorres = 0;
    pistaDasTorres = (int) pista->id;
    int segmentos = contarSegmentos(pista);
    if (segmentos <= 0 || pista->largada == NULL) {
        return;
    }
    float metrosPorSegmento = pista->metrosDaVolta / (float) segmentos;
    int passo = (int) fmaxf(roundf(ENTRE_AS_TORRES / metrosPorSegmento), 1.0f);
    int voltas = segmentos / passo;
    passo = segmentos / (voltas > 0 ? voltas : 1);   /* a mesma distância entre todas, a última também */
    int paraOLado = (int) roundf(PARA_O_LADO / metrosPorSegmento);
    static const int ANDAR[5] = { 0, 1, -1, 2, -2 };   /* em PARA_O_LADO */
    Segmento *s = pista->largada;
    for (int i = 0; i < segmentos && totalDeTorres < MAIS_TORRES; i++, s = s->proximo) {
        if (i % passo != passo / 2) {   /* começa no meio do primeiro trecho: longe da linha de chegada */
            continue;
        }
        int preferido = ((i / passo) % 2 == 0) ? 1 : -1;
        Torre *t = &torres[totalDeTorres];
        int achou = 0, temAlguma = 0;
        Torre primeira = { 0 };
        for (int a = 0; a < 5 && !achou; a++) {
            Segmento *aqui = s;
            for (int k = 0; k < abs(ANDAR[a]) * paraOLado; k++) {
                aqui = (ANDAR[a] > 0) ? aqui->proximo : aqui->anterior;
            }
            float metros = (float) ((i + ANDAR[a] * paraOLado + segmentos) % segmentos) * metrosPorSegmento;
            for (int l = 0; l < 2 && !achou; l++) {
                int lado = (l == 0) ? preferido : -preferido;
                if (pertoDoPit(pista, metros) && lado == pista->pit.lado) {
                    continue;   /* ao lado do pit lane, só do outro lado */
                }
                float largura = (lado > 0) ? aqui->larguraDireita : aqui->larguraEsquerda;
                float limite = (lado > 0) ? aqui->limiteDireita : aqui->limiteEsquerda;
                float alem = (limite > largura) ? limite - largura + ALEM_DO_MURO : SEM_MURO_MEDIDO;
                alem = Clamp(alem, MAIS_PERTO_DA_BORDA, MAIS_LONGE_DA_BORDA);
                for (int atras = 0; atras < 3 && !achou; atras++) {
                    if (!colocarTorre(t, aqui, lado, alem + MAIS_ATRAS * (float) atras)) {
                        continue;
                    }
                    if (!temAlguma) {
                        primeira = *t;
                        temAlguma = 1;
                    }
                    achou = torreLivre(pista, t);
                }
            }
        }
        if (!temAlguma) {
            continue;
        }
        if (!achou) {
            *t = primeira;
        }
        Lampada *l = &luzes[totalDeTorres];
        *l = (Lampada){ 0 };
        l->lugar = Vector3Add(t->luz, Vector3Scale(t->facho, 3.0f));
        l->cor = COR_DA_LUZ;
        l->raio = RAIO_DA_LUZ;
        l->forca = FORCA_DA_LUZ;
        l->quadro = -1;   /* sem a vista: ilumina tudo no raio */
        totalDeTorres++;
    }
}

static void prepararTorres(Pista *pista) {
    if (!temModelo) {
        montarModeloDaTorre();
    }
    if (pistaDasTorres != (int) pista->id) {
        montarTorres(pista);
    }
}

void acenderHolofotes(Pista *pista, Camera3D camera, int ligados) {
    if (!ligados || pista == NULL) {
        aplicarLampadas(NULL, 0);
        return;
    }
    prepararTorres(pista);
    static Lampada escolhidas[MAIS_LAMPADAS];
    int quantas = escolherLampadas(camera, luzes, totalDeTorres, escolhidas, TORRES_ACESAS);
    aplicarLampadas(escolhidas, quantas);
    ligarGradeDasLampadas();
}

/* ---------- O desenho ---------- */

static void prepararBrilho(void) {
    if (temBrilho) {
        return;
    }
    Image imagem = GenImageGradientRadial(64, 64, 0.0f, WHITE, BLANK);
    brilho = LoadTextureFromImage(imagem);
    UnloadImage(imagem);
    SetTextureFilter(brilho, TEXTURE_FILTER_BILINEAR);
    temBrilho = 1;
}

/*
 * desenharBrilhoDasLampadas
 * O brilho de cada lâmpada e, em volta do painel, o halo grande (só na passada principal); só de frente para
 * elas (de trás, o painel as esconde).
 * Recebe: a câmera, a força do halo grande (0: sem ele) e a das lâmpadas.
 * Devolve: nada.
 */
static void desenharBrilhoDasLampadas(Camera3D camera, float forcaDoHalo, float forcaDasLampadas) {
    rlDrawRenderBatchActive();
    BeginBlendMode(BLEND_ADDITIVE);
    rlDisableDepthMask();
    Rectangle inteira = { 0.0f, 0.0f, (float) brilho.width, (float) brilho.height };
    for (int i = 0; i < totalDeTorres; i++) {
        const Torre *t = &torres[i];
        Vector3 ate = Vector3Subtract(camera.position, t->luz);
        float distancia = Vector3Length(ate);
        if (distancia > VISTAS_ATE || distancia < 1.0f) {
            continue;
        }
        float deFrente = Vector3DotProduct(Vector3Scale(ate, 1.0f / distancia), t->facho);
        if (deFrente <= 0.0f) {
            continue;
        }
        float forca = sqrtf(deFrente);
        Vector3 naFrente = Vector3Add(t->luz, Vector3Scale(t->facho, 1.5f));
        float halo = fmaxf(30.0f, distancia * 0.09f);
        if (forcaDoHalo > 0.0f) {
            DrawBillboardRec(camera, brilho, inteira, naFrente, (Vector2){ halo, halo }, Fade(COR_DO_HALO, forcaDoHalo * forca));
        }
        float miolo = fmaxf(2.2f, distancia * 0.012f);
        for (int k = 0; k < TOTAL_DE_LAMPADAS; k++) {
            Vector3 lampada = naTorreParaOMundo(t, lampadasNaTorre[k]);
            DrawBillboardRec(camera, brilho, inteira, lampada, (Vector2){ miolo, miolo }, Fade(COR_DO_HALO, forcaDasLampadas * forca));
        }
    }
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
    EndBlendMode();
}

void desenharHolofotes(Pista *pista, Camera3D camera) {
    prepararTorres(pista);
    prepararBrilho();
    for (int i = 0; i < totalDeTorres; i++) {
        const Torre *t = &torres[i];
        if (Vector3Distance(t->pe, camera.position) > VISTAS_ATE) {
            continue;
        }
        DrawModelEx(modeloDaTorre, t->pe, (Vector3){ 0.0f, 1.0f, 0.0f }, t->giro, (Vector3){ 1.0f, 1.0f, 1.0f }, WHITE);
    }
    desenharBrilhoDasLampadas(camera, 0.16f, 0.45f);
}

void desenharLampadasDosHolofotes(Pista *pista, Camera3D camera) {
    prepararTorres(pista);
    prepararBrilho();
    desenharBrilhoDasLampadas(camera, 0.0f, 0.35f);
}

void descarregarHolofotes(void) {
    if (temBrilho) {
        UnloadTexture(brilho);
        temBrilho = 0;
    }
    if (temModelo) {
        UnloadModel(modeloDaTorre);
        temModelo = 0;
    }
    totalDeTorres = 0;
    pistaDasTorres = -1;
}
