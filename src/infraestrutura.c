/*
 * infraestrutura.c
 * ------------------------------------------------------------------
 * Veja infraestrutura.h. Cada coisa fica num lugar por um motivo: a tabela de posições
 * segue a pista, e a função livre() confere se o lugar está além do alcance do carro.
 * ------------------------------------------------------------------
 */

#include <math.h>
#include <stddef.h>

#include "raylib.h"
#include "raymath.h"
#include "infraestrutura.h"
#include "construcao.h"
#include "obstaculos.h"
#include "paisagem.h"
#include "visual.h"

/* ---------- Guard-rail e alambrado ---------- */
#define LATERAL_DO_GUARD_RAIL   2.6f      /* larguras: logo além de onde o carro chega (2,5) */
#define PASSO_DO_GUARD_RAIL     3         /* segmentos */
#define POSTE_DO_RAIL_A_CADA    2         /* passos */
#define POSTE_DA_CERCA_A_CADA   5
#define ALTURA_DO_RAIL          0.85f
#define ALTURA_DA_CERCA         3.6f
#define METROS_DA_CERCA         20.0f     /* a imagem do alambrado se repete a cada tanto */

/* ---------- Anúncios ---------- */
#define LINHAS_DE_ANUNCIO       8
#define PLACA_NO_RAIL           8.0f      /* metros de comprimento: a imagem é 8 por 1 */
#define ALTURA_DA_PLACA         1.0f
#define PLACAS_SEGUIDAS         7         /* tantas placas, e um trecho de guard-rail nu */
#define PLACAS_NO_CICLO         10
#define ALTURA_NA_ARQUIBANCADA  1.7f
#define BASE_NA_ARQUIBANCADA    1.15f
#define ALTURA_NO_MURO          1.2f
#define BASE_NO_MURO            0.12f

/* ---------- Pintura ---------- */
#define LATERAL_DA_LINHA        0.975f
#define LARGURA_DA_LINHA        0.022f    /* em larguras da pista */
#define BRITA_DE                1.75f     /* larguras */
#define BRITA_ATE               2.48f
#define JANELA_DA_BRITA         10        /* segmentos: a brita abre e fecha aos poucos */
#define LATERAL_DA_RUA          4.8f      /* a rua de serviço, atrás das arquibancadas */
#define LARGURA_DA_RUA          0.36f
#define RUA_DE                  3300.0f   /* metros */
#define RUA_ATE                 1400.0f

/* ---------- O resto ---------- */
#define TOTAL_DE_JARDINS        340
#define FISCAL_A_CADA           330.0f    /* metros */
#define VAO_DA_PASSARELA        92.0f     /* metros entre os pilares do modelo */
#define METROS_DA_GARAGEM       10.0f
#define PILHA_ATE_O_PREDIO      0.6f
#define METROS_DO_TELAO         305.0f
#define LATERAL_DO_TELAO        2.1f
#define COR_DO_RAIL             (Color){ 176, 182, 188, 255 }
#define COR_DO_POSTE            (Color){ 112, 116, 122, 255 }
#define COR_DA_BRITA            (Color){ 196, 174, 132, 255 }
#define COR_DA_RUA              (Color){ 84, 84, 88, 255 }
#define COR_BRANCA              (Color){ 238, 238, 234, 255 }

static Pista *pistaAtual;
static AlturaDaGrade alturaDaGrade;
static PlantarArvore plantarArvore;
static ColocarNoCenario colocar;
static MalhasDaInfraestrutura malhas;
static Construcao pintura = { 0 }, objetos = { 0 }, anuncios = { 0 }, cercas = { 0 };
static float largura, metrosDaVolta;
static unsigned int semente = 41u;

static float sortear(float minimo, float maximo) {
    semente = semente * 1103515245u + 12345u;
    return minimo + (maximo - minimo) * (((semente >> 16) & 0x7fff) / 32767.0f);
}

/*
 * pontoNaPista
 * Recebe: os metros a partir da linha de chegada, a lateral em larguras e onde guardar o rumo.
 * Devolve: o ponto no mundo, na altura da pista.
 */
static Vector3 pontoNaPista(float metros, float lateral, float *rumo) {
    PosicaoNaPista lugar = { pistaAtual->largada, 0.0f };
    moverNaPista(&lugar, metros * UNIDADES_POR_METRO);
    PontoNoMundo p = pontoNoMundo(lugar, lateral);
    if (rumo != NULL) {
        *rumo = p.rumo * DEG2RAD;
    }
    return (Vector3){ p.x, p.y, p.z };
}

/*
 * noChao
 * Recebe: um ponto.
 * Devolve: o mesmo ponto, pousado no terreno.
 */
static Vector3 noChao(Vector3 p) {
    p.y = alturaDaGrade(p.x, p.z);
    return p;
}

/*
 * livre
 * Recebe: o ponto e a folga em metros.
 * Devolve: 1 se ele está além do alcance do carro em toda a pista, longe dos boxes e do pit lane.
 */
static int livre(Vector3 p, float folga) {
    Segmento *s = pistaAtual->largada;
    int i = 0;
    do {
        if (i++ % 3 == 0) {
            float alcance = largura * (s->temMuro ? s->lugarDoMuro : 2.5f) + folga;
            if (hypotf(s->xMundo - p.x, s->zMundo - p.z) < alcance) {
                return 0;
            }
        }
        s = s->proximo;
    } while (s != pistaAtual->largada);

    /* O pit lane e o prédio dos boxes */
    for (float m = ENTRADA_DO_PIT; m < metrosDaVolta + FIM_DO_PIT_LANE; m += 12.0f) {
        float metros = fmodf(m, metrosDaVolta), larguraDoPit = 0.0f;
        float lado = ladoDentroDoPit(metros, metrosDaVolta, &larguraDoPit);
        float meio = lado + larguraDoPit / largura / 2.0f;
        Vector3 c = pontoNaPista(metros, -meio, NULL);
        if (hypotf(c.x - p.x, c.z - p.z) < larguraDoPit / 2.0f + folga + 6.0f) {
            return 0;
        }
    }
    for (float m = -300.0f; m <= 190.0f; m += 10.0f) {
        Vector3 c = pontoNaPista(m, -1.0f - (LARGURA_DO_PIT_LANE + 25.0f) / largura, NULL);
        if (hypotf(c.x - p.x, c.z - p.z) < 38.0f + folga) {
            return 0;
        }
    }
    return 1;
}

/*
 * pertoDeArquibancada
 * Recebe: os metros, e quantos metros além dela ainda contam.
 * Devolve: 1 se há uma arquibancada ali, do lado direito da pista.
 */
static int pertoDeArquibancada(float metros, float folga) {
    int total;
    const Arquibancada *tabela = arquibancadasDaPista(&total);
    for (int i = 0; i < total; i++) {
        if (metros > tabela[i].inicio - folga && metros < tabela[i].fim + folga) {
            return 1;
        }
    }
    return 0;
}

/*
 * placa
 * Um pedaço de um anúncio, de pé. Quem olha da pista lê o texto na ordem certa: do lado
 * direito a leitura vai contra o sentido da corrida, e do esquerdo, a favor.
 * Recebe: os dois pontos de baixo, a altura, os metros de cada um ao longo da pista, o
 *         comprimento de uma placa e o lado (+1 direita).
 * Devolve: nada.
 */
static void placa(Vector3 a, Vector3 b, float altura, float metrosA, float metrosB, float comprimento, int lado) {
    int numero = (int) floorf(metrosA / comprimento);
    float u0 = metrosA / comprimento - numero, u1 = u0 + (metrosB - metrosA) / comprimento;
    if (lado > 0) {
        u0 = 1.0f - u0;
        u1 = 1.0f - u1;
    }
    float linha = (float) (((numero % LINHAS_DE_ANUNCIO) + LINHAS_DE_ANUNCIO) % LINHAS_DE_ANUNCIO);
    float v0 = linha / LINHAS_DE_ANUNCIO, v1 = (linha + 1.0f) / LINHAS_DE_ANUNCIO;
    Vector3 cima = { 0.0f, altura, 0.0f };
    adicionarQuadrilatero(&anuncios, a, b, Vector3Add(b, cima), Vector3Add(a, cima), (Vector2){ u0, v1 }, (Vector2){ u1, v1 },
                          (Vector2){ u1, v0 }, (Vector2){ u0, v0 }, WHITE);
}

/*
 * construirPintura
 * As linhas brancas nas bordas do asfalto e a brita por fora das curvas.
 * Recebe: nada.
 * Devolve: nada.
 */
static void construirPintura(void) {
    int total = contarSegmentos(pistaAtual);
    float passo = COMPRIMENTO_SEGMENTO / UNIDADES_POR_METRO;
    Vector2 zero = { 0.0f, 0.0f };
    for (int lado = -1; lado <= 1; lado += 2) {
        for (int i = 0; i < total; i++) {
            float m = i * passo, m1 = (i + 1) * passo;
            float subir = 0.04f;
            Vector3 a = pontoNaPista(m, lado * (LATERAL_DA_LINHA - LARGURA_DA_LINHA), NULL), b = pontoNaPista(m, lado * LATERAL_DA_LINHA, NULL);
            Vector3 c = pontoNaPista(m1, lado * LATERAL_DA_LINHA, NULL), d = pontoNaPista(m1, lado * (LATERAL_DA_LINHA - LARGURA_DA_LINHA), NULL);
            a.y += subir; b.y += subir; c.y += subir; d.y += subir;
            adicionarQuadrilatero(&pintura, a, b, c, d, zero, zero, zero, zero, COR_BRANCA);

            /* Brita: por fora de quem está virando, abrindo aos poucos */
            int zebras = 0, doLado = 0;
            for (int j = -JANELA_DA_BRITA; j <= JANELA_DA_BRITA; j++) {
                Segmento *s = buscarSegmento(pistaAtual, (i + j + total) % total);
                if (temZebra(s)) {
                    zebras++;
                    doLado += (s->curvatura * lado < 0.0f);
                }
            }
            float abre = (float) doLado / (2 * JANELA_DA_BRITA + 1);
            if (abre > 0.15f) {
                float fim = BRITA_DE + (BRITA_ATE - BRITA_DE) * abre;
                Vector3 p0 = noChao(pontoNaPista(m, lado * BRITA_DE, NULL)), p1 = noChao(pontoNaPista(m, lado * fim, NULL));
                Vector3 p2 = noChao(pontoNaPista(m1, lado * fim, NULL)), p3 = noChao(pontoNaPista(m1, lado * BRITA_DE, NULL));
                if (livre(p1, -2.0f - largura * 0.0f + 1.0f) && !pertoDeArquibancada(m, 0.0f)) {
                    float tom = 0.92f + 0.16f * sinf(m * 2.7f) * cosf(m * 0.9f + lado);
                    Color cor = { (unsigned char) fminf(COR_DA_BRITA.r * tom, 255.0f), (unsigned char) fminf(COR_DA_BRITA.g * tom, 255.0f), (unsigned char) fminf(COR_DA_BRITA.b * tom, 255.0f), 255 };
                    p0.y += 0.12f; p1.y += 0.12f; p2.y += 0.12f; p3.y += 0.12f;
                    adicionarQuadrilatero(&pintura, p0, p1, p2, p3, zero, zero, zero, zero, cor);
                }
            }
            (void) zebras;
        }
    }

    /* A rua de serviço, atrás das arquibancadas, pelo lado de fora do circuito */
    for (float m = RUA_DE; m < metrosDaVolta + RUA_ATE; m += 6.0f) {
        Vector3 a = noChao(pontoNaPista(m, LATERAL_DA_RUA, NULL)), b = noChao(pontoNaPista(m, LATERAL_DA_RUA + LARGURA_DA_RUA, NULL));
        Vector3 c = noChao(pontoNaPista(m + 6.0f, LATERAL_DA_RUA + LARGURA_DA_RUA, NULL)), d = noChao(pontoNaPista(m + 6.0f, LATERAL_DA_RUA, NULL));
        if (foraDoAutodromo(b.x, b.z)) {
            continue;
        }
        a.y += 0.15f; b.y += 0.15f; c.y += 0.15f; d.y += 0.15f;
        adicionarQuadrilatero(&pintura, a, b, c, d, zero, zero, zero, zero, COR_DA_RUA);
    }
}

/*
 * construirGuardRail
 * Dos dois lados, onde couber: postes, trilhos, alambrado e, em boa parte, placas de patrocínio.
 * Recebe: nada.
 * Devolve: nada.
 */
static void construirGuardRail(void) {
    int total = contarSegmentos(pistaAtual);
    float passo = PASSO_DO_GUARD_RAIL * COMPRIMENTO_SEGMENTO / UNIDADES_POR_METRO;
    for (int lado = -1; lado <= 1; lado += 2) {
        int anteriorLivre = 0;
        Vector3 anterior = { 0 };
        int contagem = 0;
        for (int i = 0; i <= total; i += PASSO_DO_GUARD_RAIL) {
            float m = i / SEGMENTOS_POR_METRO;
            Segmento *s = buscarSegmento(pistaAtual, i % total);
            Vector3 aqui = noChao(pontoNaPista(m, lado * LATERAL_DO_GUARD_RAIL, NULL));
            int agoraLivre = !s->temMuro && livre(aqui, 0.3f);
            if (agoraLivre && anteriorLivre) {
                float mAntes = m - passo;
                int numero = (int) (mAntes / PLACA_NO_RAIL);
                Vector3 baixo = { 0.0f, 0.15f, 0.0f };
                if (numero % PLACAS_NO_CICLO < PLACAS_SEGUIDAS) {
                    placa(Vector3Add(lado > 0 ? anterior : anterior, baixo), Vector3Add(aqui, baixo), ALTURA_DA_PLACA, mAntes, m, PLACA_NO_RAIL, lado);
                } else {
                    Vector3 r0 = { 0.0f, 0.40f, 0.0f }, r1 = { 0.0f, ALTURA_DO_RAIL, 0.0f };
                    quadIluminado(&objetos, Vector3Add(anterior, r0), Vector3Add(aqui, r0), Vector3Add(aqui, r1), Vector3Add(anterior, r1), COR_DO_RAIL);
                }
                Vector3 t0 = { anterior.x, anterior.y + 1.15f, anterior.z }, t1 = { aqui.x, aqui.y + 1.15f, aqui.z };
                Vector3 t2 = { aqui.x, aqui.y + 1.15f + ALTURA_DA_CERCA, aqui.z }, t3 = { anterior.x, anterior.y + 1.15f + ALTURA_DA_CERCA, anterior.z };
                float v0 = mAntes / METROS_DA_CERCA, v1 = m / METROS_DA_CERCA;
                adicionarQuadrilatero(&cercas, t0, t1, t2, t3, (Vector2){ 0.0f, v0 }, (Vector2){ 0.0f, v1 }, (Vector2){ 1.0f, v1 }, (Vector2){ 1.0f, v0 }, WHITE);
                if (contagem % POSTE_DA_CERCA_A_CADA == 0) {
                    caixaIluminada(&objetos, aqui, 0.0f, 0.22f, 0.22f, 1.15f + ALTURA_DA_CERCA + 0.2f, COR_DO_POSTE);
                } else if (contagem % POSTE_DO_RAIL_A_CADA == 0) {
                    caixaIluminada(&objetos, aqui, 0.0f, 0.14f, 0.14f, 1.15f, COR_DO_POSTE);
                }
                contagem++;
            }
            anterior = aqui;
            anteriorLivre = agoraLivre;
        }
    }
}

/*
 * construirAnunciosFixos
 * As placas nos muros da reta e na frente das arquibancadas.
 * Recebe: nada.
 * Devolve: nada.
 */
static void construirAnunciosFixos(void) {
    int total = contarSegmentos(pistaAtual);
    float passo = COMPRIMENTO_SEGMENTO / UNIDADES_POR_METRO;
    for (int lado = -1; lado <= 1; lado += 2) {
        for (int i = 0; i < total; i++) {
            Segmento *s = buscarSegmento(pistaAtual, i), *t = proximoSegmento(s);
            if (!s->temMuro || !t->temMuro || (s->lugarDoMuro > 1.01f && lado < 0)) {
                continue;
            }
            float m = i * passo;
            float recuo = 0.08f / largura;
            Vector3 a = pontoNaPista(m, lado * (s->lugarDoMuro - recuo), NULL), b = pontoNaPista(m + passo, lado * (t->lugarDoMuro - recuo), NULL);
            a.y += BASE_NO_MURO; b.y += BASE_NO_MURO;
            placa(a, b, ALTURA_NO_MURO, m, m + passo, ALTURA_NO_MURO * LINHAS_DE_ANUNCIO, lado);
        }
    }
    int quantas;
    const Arquibancada *tabela = arquibancadasDaPista(&quantas);
    for (int q = 0; q < quantas; q++) {
        if (tabela[q].tipo != OBJETO_ARQUIBANCADA) {
            continue;
        }
        float lateral = (tabela[q].distancia - 0.08f) / largura;
        for (float m = tabela[q].inicio; m < tabela[q].fim; m += 3.3f) {
            Vector3 a = noChao(pontoNaPista(m, lateral, NULL)), b = noChao(pontoNaPista(m + 3.3f, lateral, NULL));
            /* A altura do muro é a do chão no começo de cada fatia da arquibancada: aqui basta o chão */
            a.y += BASE_NA_ARQUIBANCADA; b.y += BASE_NA_ARQUIBANCADA;
            placa(a, b, ALTURA_NA_ARQUIBANCADA, m, m + 3.3f, ALTURA_NA_ARQUIBANCADA * LINHAS_DE_ANUNCIO, 1);
        }

        /* As bandeiras, no alto do fundo */
        if (tabela[q].coberta) {
            float fundo = tabela[q].distancia + 2.8f + tabela[q].fileiras * 0.85f;
            float altura = 3.0f + tabela[q].fileiras * 0.5f + 6.0f;
            int n = 0;
            for (float m = tabela[q].inicio + 6.0f; m < tabela[q].fim; m += 16.0f, n++) {
                float rumo;
                Vector3 pe = noChao(pontoNaPista(m, fundo / largura, &rumo));
                pe.y += altura - 1.0f;
                colocar((ModeloDoCenario) (CENARIO_MASTRO_1 + n % 5), pe, -rumo, 1.0f);
            }
        }
    }
}

/*
 * construirPostosETorres
 * Postos de fiscal ao longo da pista, alternando os lados, e torres de TV nas curvas.
 * Recebe: nada.
 * Devolve: nada.
 */
static void construirPostosETorres(void) {
    int lado = 1;
    for (float m = 200.0f; m < metrosDaVolta; m += FISCAL_A_CADA, lado = -lado) {
        for (int tentativa = 0; tentativa < 2; tentativa++) {
            int estaLado = (tentativa == 0) ? lado : -lado;
            float rumo;
            Vector3 pe = noChao(pontoNaPista(m, estaLado * (LATERAL_DO_GUARD_RAIL + 0.22f), &rumo));
            if (livre(pe, 3.0f) && !(estaLado > 0 && pertoDeArquibancada(m, 20.0f))) {
                colocar(CENARIO_POSTO_FISCAL, pe, -rumo, 1.0f);
                break;
            }
        }
    }
    static const float TORRES[] = { 470.0f, 1420.0f, 1900.0f, 2230.0f, 2600.0f, 2950.0f, 3230.0f, 3480.0f };
    for (size_t t = 0; t < sizeof(TORRES) / sizeof(TORRES[0]); t++) {
        for (int estaLado = 1; estaLado >= -1; estaLado -= 2) {
            float rumo;
            Vector3 pe = noChao(pontoNaPista(TORRES[t], estaLado * (LATERAL_DO_GUARD_RAIL + 0.4f), &rumo));
            if (livre(pe, 5.0f) && !(estaLado > 0 && pertoDeArquibancada(TORRES[t], 30.0f))) {
                colocar(CENARIO_TORRE_TV, pe, -rumo, 1.0f);
                break;
            }
        }
    }
}

/*
 * passarela
 * Uma passarela de pedestres sobre a pista, com um patrocinador de cada lado.
 * Recebe: os metros.
 * Devolve: 1 se coube ali.
 */
static int passarela(float metros) {
    float rumo;
    Vector3 esquerda = noChao(pontoNaPista(metros, -LATERAL_DO_GUARD_RAIL - 0.15f, &rumo));
    Vector3 direita = noChao(pontoNaPista(metros, LATERAL_DO_GUARD_RAIL + 0.15f, NULL));
    if (!livre(esquerda, 3.0f) || !livre(direita, 3.0f)) {
        return 0;
    }
    /* Os pilares do modelo descem bastante: o terreno esconde o que sobra */
    Vector3 meio = Vector3Lerp(esquerda, direita, 0.5f);
    meio.y = fmaxf(fmaxf(esquerda.y, direita.y), pontoNaPista(metros, 0.0f, NULL).y);
    colocar(CENARIO_PASSARELA, meio, -rumo, Vector3Distance(esquerda, direita) / VAO_DA_PASSARELA);
    return 1;
}

/*
 * construirTelao
 * O telão, de frente para a reta dos boxes, depois do fim da arquibancada M.
 * Recebe: nada.
 * Devolve: nada.
 */
static void construirTelao(void) {
    float rumo;
    Vector3 pe = pontoNaPista(METROS_DO_TELAO, LATERAL_DO_TELAO, &rumo);
    colocar(CENARIO_TELAO, pe, -rumo, 1.0f);
}

/*
 * construirPaddock
 * Os caminhões das equipes no pátio dos boxes, as tendas atrás da arquibancada principal,
 * o estacionamento atrás da Reta Oposta e os equipamentos da mureta dos boxes.
 * Recebe: nada.
 * Devolve: nada.
 */
static void construirPaddock(void) {
    /* Caminhões no pátio de trás do prédio */
    int n = 0;
    for (float m = -258.0f; m < 150.0f; m += 19.0f, n++) {
        float rumo;
        Vector3 pe = pontoNaPista(m, -1.0f - (LARGURA_DO_PIT_LANE + 42.5f) / largura, &rumo);
        colocar((ModeloDoCenario) (CENARIO_CAMINHAO_1 + n % 4), pe, -rumo, 1.0f);
    }

    /* Na mureta dos boxes: as bancadas das equipes, com toldo */
    n = 0;
    for (float m = -262.0f; m < 148.0f; m += 20.0f, n++) {
        float rumo;
        Vector3 pe = pontoNaPista(m, -1.0f - 0.9f / largura, &rumo);
        colocar((ModeloDoCenario) (CENARIO_BANCADA_1 + n % 4), pe, -rumo, 1.0f);
    }

    /* Pilhas de pneus coladas no prédio, entre uma garagem e a outra: fora das vagas */
    n = 0;
    for (float m = PRIMEIRO_METRO_DOS_BOXES; m <= PRIMEIRO_METRO_DOS_BOXES + 420.0f; m += METROS_DA_GARAGEM, n++) {
        Vector3 pilha = pontoNaPista(m, -1.0f - (LARGURA_DO_PIT_LANE - PILHA_ATE_O_PREDIO) / largura, NULL);
        colocar((ModeloDoCenario) (CENARIO_PILHA_1 + n % 2), pilha, 0.0f, 1.0f);
    }

    /* Tendas de hospitalidade, atrás das arquibancadas da reta */
    n = 0;
    for (float m = 3800.0f; m < metrosDaVolta + 270.0f; m += 13.0f, n++) {
        float rumo;
        Vector3 pe = noChao(pontoNaPista(m, (LATERAL_DA_RUA + LARGURA_DA_RUA + 0.6f), &rumo));
        if (!foraDoAutodromo(pe.x, pe.z) && (n % 5 != 4)) {
            colocar((ModeloDoCenario) (CENARIO_TENDA_1 + n % 3), pe, PI - rumo, 1.0f);
        }
    }

    /* Estacionamento, atrás da Reta Oposta */
    for (float lateral = LATERAL_DA_RUA + LARGURA_DA_RUA + 0.5f; lateral < 8.5f; lateral += 0.42f) {
        for (float m = 870.0f; m < 1290.0f; m += 3.1f) {
            float rumo;
            Vector3 pe = noChao(pontoNaPista(m, lateral, &rumo));
            float vira = (sortear(0.0f, 1.0f) < 0.5f) ? PI / 2.0f : -PI / 2.0f;
            int modelo = (int) sortear(0.0f, 5.99f);
            if (!foraDoAutodromo(pe.x, pe.z) && sortear(0.0f, 1.0f) < 0.72f) {
                colocar((ModeloDoCenario) (CENARIO_CARRO_1 + modelo), pe, -rumo + vira, 1.0f);
            }
        }
    }
}

/*
 * ocupado / ocupar
 * Um mapa grosseiro do que já foi construído dentro do autódromo, para as árvores não
 * nascerem em cima de nada.
 */
#define CELULAS_DO_MAPA  400
#define METROS_DA_CELULA 10.0f
#define ORIGEM_DO_MAPA   (-2000.0f)
static unsigned char mapaOcupado[CELULAS_DO_MAPA][CELULAS_DO_MAPA];

static void ocupar(Vector3 p, float raio) {
    int r = (int) (raio / METROS_DA_CELULA) + 1;
    int cx = (int) ((p.x - ORIGEM_DO_MAPA) / METROS_DA_CELULA), cz = (int) ((p.z - ORIGEM_DO_MAPA) / METROS_DA_CELULA);
    for (int z = cz - r; z <= cz + r; z++) {
        for (int x = cx - r; x <= cx + r; x++) {
            if (x >= 0 && z >= 0 && x < CELULAS_DO_MAPA && z < CELULAS_DO_MAPA) {
                mapaOcupado[z][x] = 1;
            }
        }
    }
}

static int ocupado(Vector3 p) {
    int cx = (int) ((p.x - ORIGEM_DO_MAPA) / METROS_DA_CELULA), cz = (int) ((p.z - ORIGEM_DO_MAPA) / METROS_DA_CELULA);
    return cx < 0 || cz < 0 || cx >= CELULAS_DO_MAPA || cz >= CELULAS_DO_MAPA || mapaOcupado[cz][cx];
}

/*
 * marcarOQueExiste
 * Marca no mapa as arquibancadas, os boxes, a rua, as tendas, o estacionamento, as árvores da pista.
 * Recebe: nada.
 * Devolve: nada.
 */
static void marcarOQueExiste(void) {
    int total;
    const Arquibancada *tabela = arquibancadasDaPista(&total);
    for (int q = 0; q < total; q++) {
        float fundo = tabela[q].distancia + 4.0f + tabela[q].fileiras * 0.85f;
        for (float m = tabela[q].inicio; m < tabela[q].fim; m += 8.0f) {
            for (float d = tabela[q].distancia; d < fundo; d += 8.0f) {
                ocupar(pontoNaPista(m, d / largura, NULL), 8.0f);
            }
        }
    }
    for (float m = RUA_DE; m < metrosDaVolta + RUA_ATE; m += 8.0f) {
        ocupar(pontoNaPista(m, LATERAL_DA_RUA + LARGURA_DA_RUA / 2.0f, NULL), 10.0f);
    }
    for (float m = 3800.0f; m < metrosDaVolta + 270.0f; m += 8.0f) {
        ocupar(pontoNaPista(m, LATERAL_DA_RUA + LARGURA_DA_RUA + 0.6f, NULL), 10.0f);
    }
    for (float m = 870.0f; m < 1290.0f; m += 8.0f) {
        for (float l = LATERAL_DA_RUA; l < 8.6f; l += 0.4f) {
            ocupar(pontoNaPista(m, l, NULL), 6.0f);
        }
    }
    for (float m = 3400.0f; m < metrosDaVolta + 900.0f; m += 8.0f) {
        ocupar(pontoNaPista(m, -3.0f, NULL), 25.0f);
    }
    Segmento *s = pistaAtual->largada;
    do {
        PosicaoNaPista lugar = { s, 0.0f };
        for (Obstaculo *o = s->obstaculos; o != NULL; o = o->proximo) {
            PontoNoMundo p = pontoNoMundo(lugar, o->deslocamentoLateral);
            ocupar((Vector3){ p.x, p.y, p.z }, 6.0f);
        }
        s = s->proximo;
    } while (s != pistaAtual->largada);
}

/*
 * construirJardins
 * Manchas de árvores no que sobrou do gramado, em volta do lago e nos cantos, com os
 * mesmos modelos das árvores da beira da pista.
 * Recebe: nada.
 * Devolve: nada.
 */
static void construirJardins(void) {
    marcarOQueExiste();
    int plantadas = 0;
    for (int tentativa = 0; tentativa < 40000 && plantadas < TOTAL_DE_JARDINS; tentativa++) {
        float x = sortear(-1500.0f, 900.0f), z = sortear(-1300.0f, 1700.0f);
        Vector3 p = { x, 0.0f, z };
        float mancha = sinf(x * 0.021f + 1.3f) * sinf(z * 0.017f + 0.4f) + 0.5f * sinf(x * 0.052f) * sinf(z * 0.047f + 2.0f);
        if (mancha < 0.28f || foraDoAutodromo(x, z) || dentroDoLago(x, z) || ocupado(p)) {
            continue;
        }
        p = noChao(p);
        if (livre(p, 6.0f)) {
            plantarArvore(p);
            ocupar(p, 8.0f);
            plantadas++;
        }
    }
    TraceLog(LOG_INFO, "INFRA: %d arvores nos jardins", plantadas);
}

/*
 * gerarAnuncios
 * A imagem dos patrocinadores (fictícios). Fica pronta ao carregar o jogo.
 * Recebe: nada.
 * Devolve: nada.
 */
static void gerarAnuncios(void) {
    typedef struct { const char *texto; Color fundo, letra; } Anuncio;
    static const Anuncio ANUNCIOS[LINHAS_DE_ANUNCIO] = {
        { "RUST-EZE", { 180, 20, 26, 255 }, { 255, 225, 90, 255 } },
        { "DINOCO", { 22, 90, 180, 255 }, { 250, 250, 250, 255 } },
        { "LIGHTYEAR", { 26, 26, 30, 255 }, { 255, 205, 40, 255 } },
        { "KATCHAU!", { 240, 196, 30, 255 }, { 170, 20, 26, 255 } },
        { "GP INTERLAGOS", { 16, 110, 70, 255 }, { 250, 220, 60, 255 } },
        { "TURBO COLA", { 228, 228, 232, 255 }, { 190, 24, 30, 255 } },
        { "VELOX PNEUS", { 232, 120, 20, 255 }, { 24, 24, 28, 255 } },
        { "RADIO PISTA 95", { 70, 30, 120, 255 }, { 250, 250, 250, 255 } },
    };
    int largo = 1024, linha = 128;
    Image imagem = GenImageColor(largo, linha * LINHAS_DE_ANUNCIO, BLACK);
    Font fonte = fonteDoJogo(FONTE_TITULO);
    for (int i = 0; i < LINHAS_DE_ANUNCIO; i++) {
        const Anuncio *a = &ANUNCIOS[i];
        ImageDrawRectangle(&imagem, 0, i * linha, largo, linha, a->fundo);
        ImageDrawRectangle(&imagem, 0, i * linha, largo, 8, COR_BRANCA);
        ImageDrawRectangle(&imagem, 0, (i + 1) * linha - 8, largo, 8, (Color){ 20, 20, 24, 255 });
        float tamanho = 92.0f;
        Vector2 medida = MeasureTextEx(fonte, a->texto, tamanho, 2.0f);
        ImageDrawTextEx(&imagem, fonte, a->texto, (Vector2){ (largo - medida.x) / 2.0f, i * linha + (linha - medida.y) / 2.0f }, tamanho, 2.0f, a->letra);
    }

    malhas.imagemDosAnuncios = LoadTextureFromImage(imagem);
    UnloadImage(imagem);
    GenTextureMipmaps(&malhas.imagemDosAnuncios);
    SetTextureFilter(malhas.imagemDosAnuncios, TEXTURE_FILTER_TRILINEAR);
}

/*
 * construirInfraestrutura
 * Levanta tudo, uma vez, ao carregar.
 * Recebe: a pista e a função da altura do terreno.
 * Devolve: as malhas, para o mundo.c desenhar.
 */
const MalhasDaInfraestrutura *construirInfraestrutura(Pista *pista, AlturaDaGrade altura, PlantarArvore plantar, ColocarNoCenario colocarModelo) {
    plantarArvore = plantar;
    colocar = colocarModelo;
    pistaAtual = pista;
    alturaDaGrade = altura;
    largura = pista->largada->larguraEsquerda;
    metrosDaVolta = pista->metrosDaVolta;
    gerarAnuncios();
    construirPintura();
    construirGuardRail();
    construirAnunciosFixos();
    construirPostosETorres();
    construirTelao();
    construirPaddock();
    construirJardins();
    static const float PASSARELAS[] = { 3300.0f, 3250.0f, 3350.0f, 1960.0f, 1920.0f, 2000.0f };
    int feitas = 0;
    for (int i = 0; i < 6 && feitas < 2; i++) {
        if ((i == 3 && feitas == 1) || i < 3) {
            feitas += (feitas == 0 || i >= 3) ? passarela(PASSARELAS[i]) : 0;
        } else if (feitas == 1) {
            feitas += passarela(PASSARELAS[i]);
        }
    }
    malhas.pintura = terminarMalha(&pintura);
    malhas.objetos = terminarMalha(&objetos);
    malhas.anuncios = terminarMalha(&anuncios);
    malhas.cercas = terminarMalha(&cercas);
    TraceLog(LOG_INFO, "INFRA: passarelas %d; objetos %d, anuncios %d, cercas %d, pintura %d vertices", feitas,
             malhas.objetos.vertexCount, malhas.anuncios.vertexCount, malhas.cercas.vertexCount, malhas.pintura.vertexCount);
    return &malhas;
}

/*
 * descarregarInfraestrutura
 * Recebe: nada.
 * Devolve: nada.
 */
void descarregarInfraestrutura(void) {
    UnloadMesh(malhas.pintura);
    UnloadMesh(malhas.objetos);
    UnloadMesh(malhas.anuncios);
    UnloadMesh(malhas.cercas);
    UnloadTexture(malhas.imagemDosAnuncios);
}
