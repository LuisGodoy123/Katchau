/*
 * render.c
 * ------------------------------------------------------------------
 * A visão da corrida, em 3D de verdade, e os carros. Veja render.h.
 *
 *   - O cenário fixo (estádio, asfalto, terreno, objetos, céu) vem de mundo.c.
 *   - A câmera anda pela lista circular da pista: fica atrás do carro,
 *     no ponto da pista CAMERA_ATRAS antes dele, e acima dele.
 *   - Todos os carros são modelos 3D (carros3d.c), colocados no mundo
 *     pela posição deles na lista.
 *   - A fumaça e a poeira são nuvens no mundo (fumaca.c).
 * ------------------------------------------------------------------
 */

#include <math.h>
#include <stddef.h>
#include "raylib.h"
#include "rlgl.h"
#include "render.h"
#include "mundo.h"
#include "carros3d.h"
#include "fumaca.h"
#include "noite.h"
#include "raymath.h"

/* ---------- Câmera ---------- */
#define GRAUS_DE_VISAO        65.0f    /* na vertical */
#define CAMERA_ATRAS          8.5f     /* metros atrás do centro do carro */
#define CAMERA_ALTURA         3.4f     /* metros acima do chão */
#define OLHAR_PARA_BAIXO      5.0f     /* graus abaixo do horizonte */
#define VELOCIDADE_CAMERA_MENU 3000.0f /* a câmera do fundo do menu anda a 75 km/h */
#define SEGUNDOS_POR_PISTA    14.0f    /* o fundo dos menus troca de pista depois disso */
#define DURACAO_DA_TROCA_DE_PISTA 0.7f /* segundos escurecendo e clareando */

/* ---------- Distâncias do que é desenhado (metros) ---------- */
#define DISTANCIA_DOS_RIVAIS  1500.0f
#define MENOR_DISTANCIA_SPRITE 1.5f    /* mais perto que isso da câmera, também não */
#define DISTANCIA_DA_FUMACA   200.0f

/* ---------- Carros no mundo ---------- */
#define ENTRE_EIXOS           0.62f    /* distância entre os eixos, em comprimentos do carro */
#define BITOLA                0.82f    /* distância entre as rodas de um eixo, em larguras do carro */
#define GRAUS_DO_VOLANTE      7.0f     /* o carro aponta para onde o volante manda */
#define GRAUS_DE_ROLAGEM      2.0f     /* e inclina para fora ao virar */
#define MERGULHO_NA_FREADA    0.025f   /* e abaixa o nariz ao frear */
#define TREPIDACAO_NA_GRAMA   0.06f    /* metros */

/* ---------- Fumaça ---------- */
#define INICIO_DA_FUMACA      0.55f    /* derrapagem abaixo disso não solta fumaça */
#define NUVENS_POR_SEGUNDO    55.0f    /* por roda, na derrapagem cheia */
#define VELOCIDADE_DA_ARRANCADA 1600.0f /* abaixo disso (40 km/h), acelerar patina um pouco */
#define ALTURA_DA_NUVEM       0.25f    /* metros acima do chão */

#define LARGURA_CARRO         170      /* tamanho de referência das fotos nos menus */

/*
 * aFrenteDaCamera
 * Recebe: a câmera e um ponto.
 * Devolve: a distância do ponto à frente da câmera (negativa se estiver atrás).
 */
static float aFrenteDaCamera(Camera3D camera, Vector3 ponto) {
    Vector3 frente = { camera.target.x - camera.position.x, camera.target.y - camera.position.y,
                       camera.target.z - camera.position.z };
    float tamanho = sqrtf(frente.x * frente.x + frente.y * frente.y + frente.z * frente.z);
    return ((ponto.x - camera.position.x) * frente.x + (ponto.y - camera.position.y) * frente.y
            + (ponto.z - camera.position.z) * frente.z) / tamanho;
}

static int cenaPausada = 0;

/*
 * montarCamera
 * Põe a câmera atrás de um ponto da pista, olhando para ele. De lado ela
 * fica presa ao carro, que por isso aparece sempre no centro da tela; nas
 * curvas inclinadas ela inclina junto com o chão.
 * Recebe: a posição do carro e o deslocamento lateral dele.
 * Devolve: a câmera.
 */
static Camera3D montarCamera(PosicaoNaPista carro, float lateral) {
    PosicaoNaPista atras = carro;
    moverNaPista(&atras, -CAMERA_ATRAS * UNIDADES_POR_METRO);   /* as voltas devolvidas não importam aqui */
    PontoNoMundo deCamera = pontoNoMundo(atras, lateral);
    PontoNoMundo doCarro = pontoNoMundo(carro, lateral);

    float dx = doCarro.x - deCamera.x, dz = doCarro.z - deCamera.z;
    float comprimento = hypotf(dx, dz);
    float ladeira = (doCarro.y - deCamera.y) / comprimento;

    Camera3D camera = { 0 };
    camera.position = (Vector3){ deCamera.x, deCamera.y + CAMERA_ALTURA, deCamera.z };
    camera.target = (Vector3){ camera.position.x + dx / comprimento,
                               camera.position.y + ladeira - tanf(OLHAR_PARA_BAIXO * DEG2RAD),
                               camera.position.z + dz / comprimento };

    /* O "para cima" da câmera é perpendicular ao chão inclinado */
    float rumo = doCarro.rumo * DEG2RAD;
    float inclinacao = atanf(doCarro.inclinacao);
    camera.up = (Vector3){ -cosf(rumo) * sinf(inclinacao), cosf(inclinacao), -sinf(rumo) * sinf(inclinacao) };
    camera.fovy = GRAUS_DE_VISAO;
    camera.projection = CAMERA_PERSPECTIVE;
    return camera;
}

/*
 * matrizDoCarro
 * Assenta o carro no chão: mede a altura do asfalto embaixo das quatro
 * rodas e, com elas, acha a altura, a ladeira e a inclinação do carro. Em
 * cima disso, ele aponta para onde o volante manda, inclina um pouco para
 * fora ao virar e abaixa o nariz ao frear.
 * Recebe: o carro.
 * Devolve: a matriz que leva o carro normalizado até lá.
 */
static Matrix matrizDoCarro(Carro *carro) {
    PontoNoMundo aqui = pontoNoMundo(carro->posicao, carro->deslocamentoLateral);
    float meiaDistancia = comprimentoDoCarro3D(carro->numero) * ENTRE_EIXOS / 2.0f;
    float meiaBitola = larguraDoCarro3D(carro->numero) * BITOLA / 2.0f;
    float lateral = carro->deslocamentoLateral * larguraDoLado(carro->posicao.segmento, carro->deslocamentoLateral);

    PosicaoNaPista eixoDaFrente = carro->posicao, eixoDeTras = carro->posicao;
    moverNaPista(&eixoDaFrente, meiaDistancia * UNIDADES_POR_METRO);
    moverNaPista(&eixoDeTras, -meiaDistancia * UNIDADES_POR_METRO);
    float frenteEsquerda = alturaDoChao(eixoDaFrente, lateral - meiaBitola);
    float frenteDireita = alturaDoChao(eixoDaFrente, lateral + meiaBitola);
    float trasEsquerda = alturaDoChao(eixoDeTras, lateral - meiaBitola);
    float trasDireita = alturaDoChao(eixoDeTras, lateral + meiaBitola);

    float ladeira = ((frenteEsquerda + frenteDireita) - (trasEsquerda + trasDireita)) / (4.0f * meiaDistancia);
    float inclinacao = ((frenteDireita + trasDireita) - (frenteEsquerda + trasEsquerda)) / (4.0f * meiaBitola);
    float y = (frenteEsquerda + frenteDireita + trasEsquerda + trasDireita) / 4.0f;

    if (carro->ultimosComandos.frear && carro->velocidade > 0.0f) {
        ladeira -= MERGULHO_NA_FREADA;
    }
    if (estaForaDaPista(carro) && carro->velocidade > 0.0f) {
        y += TREPIDACAO_NA_GRAMA * sinf(carro->distanciaPercorrida * 0.05f);
    }

    float rumo = (aqui.rumo + carro->volante * GRAUS_DO_VOLANTE) * DEG2RAD;
    Vector3 frente = Vector3Normalize((Vector3){ sinf(rumo), ladeira, -cosf(rumo) });

    float rolagem = atanf(inclinacao) + carro->volante * GRAUS_DE_ROLAGEM * DEG2RAD;
    Vector3 cima = { -cosf(rumo) * sinf(rolagem), cosf(rolagem), -sinf(rumo) * sinf(rolagem) };
    Vector3 lado = Vector3Normalize(Vector3CrossProduct(cima, frente));   /* a esquerda do carro é +x no modelo */
    cima = Vector3CrossProduct(frente, lado);

    Matrix m = { 0 };
    m.m0 = lado.x;   m.m1 = lado.y;   m.m2 = lado.z;
    m.m4 = cima.x;   m.m5 = cima.y;   m.m6 = cima.z;
    m.m8 = frente.x; m.m9 = frente.y; m.m10 = frente.z;
    m.m12 = aqui.x;  m.m13 = y;       m.m14 = aqui.z;
    m.m15 = 1.0f;
    return m;
}

/*
 * desenharCarroNoMundo
 * Recebe: a câmera e o carro.
 * Devolve: nada (carro atrás da câmera ou longe demais não é desenhado).
 */
static void desenharCarroNoMundo(Camera3D camera, Carro *carro) {
    PontoNoMundo ponto = pontoNoMundo(carro->posicao, carro->deslocamentoLateral);
    float distancia = aFrenteDaCamera(camera, (Vector3){ ponto.x, ponto.y, ponto.z });
    if (distancia < -6.0f || distancia > DISTANCIA_DOS_RIVAIS) {
        return;
    }
    desenharCarro3D(carro->numero, matrizDoCarro(carro));
}

/*
 * soltarFumacaDoCarro
 * Solta as nuvens nas rodas de trás, de acordo com o que o carro está fazendo:
 * poeira na grama, fumaça na derrapagem e na arrancada forte. O número de
 * nuvens depende do tempo que passou, não de quantos quadros o jogo desenha.
 * Recebe: a câmera, o carro e o tempo que passou na corrida.
 * Devolve: nada.
 */
static void soltarFumacaDoCarro(Camera3D camera, Carro *carro, float tempoQuadro) {
    PontoNoMundo ponto = pontoNoMundo(carro->posicao, carro->deslocamentoLateral);
    float distancia = aFrenteDaCamera(camera, (Vector3){ ponto.x, ponto.y, ponto.z });
    if (distancia < -12.0f || distancia > DISTANCIA_DA_FUMACA || carro->velocidade <= 0.0f) {
        return;
    }

    float forca = 0.0f;
    TipoDeNuvem tipo = NUVEM_DE_PNEU;
    if (estaForaDaPista(carro)) {
        forca = 0.9f;
        tipo = NUVEM_DE_POEIRA;
    } else if (carro->derrapagem > INICIO_DA_FUMACA) {
        forca = (carro->derrapagem - INICIO_DA_FUMACA) / (1.0f - INICIO_DA_FUMACA);
    } else if (carro->ultimosComandos.acelerar && carro->velocidade < VELOCIDADE_DA_ARRANCADA) {
        forca = 0.55f * (1.0f - carro->velocidade / VELOCIDADE_DA_ARRANCADA);
    }
    if (forca <= 0.0f) {
        return;
    }

    float metrosPorSegundo = carro->velocidade / UNIDADES_POR_METRO;
    float rumo = ponto.rumo * DEG2RAD;
    Vector3 velocidade = { sinf(rumo) * metrosPorSegundo, 0.0f, -cosf(rumo) * metrosPorSegundo };

    /* Quantas nuvens neste quadro: a parte fracionária é sorteada */
    float quantas = NUVENS_POR_SEGUNDO * tempoQuadro * (0.4f + 0.6f * forca);
    int inteiras = (int) quantas;
    if (GetRandomValue(0, 1000) / 1000.0f < quantas - inteiras) {
        inteiras++;
    }

    Matrix noMundo = matrizDoCarro(carro);
    float meiaBitola = larguraDoCarro3D(carro->numero) * BITOLA / 2.0f;
    float eixoDeTras = -comprimentoDoCarro3D(carro->numero) * ENTRE_EIXOS / 2.0f;
    for (int i = 0; i < inteiras; i++) {
        for (int lado = -1; lado <= 1; lado += 2) {
            /* Sorteia onde, ao longo do caminho do quadro, ela nasce: senão as nuvens saem em fila */
            float recuo = -metrosPorSegundo * tempoQuadro * GetRandomValue(0, 1000) / 1000.0f;
            Vector3 roda = Vector3Transform((Vector3){ lado * meiaBitola, ALTURA_DA_NUVEM, eixoDeTras + recuo }, noMundo);
            soltarNuvem(roda, velocidade, forca, tipo);
        }
    }
}

/*
 * congelarCena
 * Na pausa, os flashes e a torcida param junto com a corrida.
 * Recebe: 1 se o jogo está pausado.
 * Devolve: nada.
 */
void congelarCena(int pausada) {
    cenaPausada = pausada;
}

/*
 * desenharMundo3D
 * O cenário e os carros, vistos pela câmera.
 * Recebe: a câmera, a pista e a corrida (NULL nos menus: sem carros).
 * Devolve: nada.
 */
static void desenharMundo3D(Camera3D camera, Pista *pista, Corrida *corrida) {
    int ehNoite = (pista->id == PISTA_COPA_PISTAO);
    if (ehNoite) {
        atualizarNoite(cenaPausada ? 0.0f : GetFrameTime());
    }
    prepararLuzes(ehNoite);

    prepararPlanosDaCamera();
    BeginMode3D(camera);
    rlDisableBackfaceCulling();
    desenharCenario(pista->id, camera.position);

    if (corrida != NULL) {
        for (int r = 0; r < TOTAL_RIVAIS; r++) {
            desenharCarroNoMundo(camera, &corrida->rivais[r]);
        }
        desenharCarroNoMundo(camera, &corrida->jogador);
        desenharFumaca3D(camera);
    }
    if (ehNoite) {
        desenharBrilhos(camera);
    }

    rlEnableBackfaceCulling();
    EndMode3D();
}

/*
 * desenharVisaoDoJogador
 * Desenha o que o jogador vê: o mundo pela câmera atrás do carro.
 * Recebe: a corrida.
 * Devolve: nada.
 */
void desenharVisaoDoJogador(Corrida *corrida) {
    Carro *jogador = &corrida->jogador;
    Camera3D camera = montarCamera(jogador->posicao, jogador->deslocamentoLateral);

    /* A fumaça anda com o relógio da corrida: na pausa e na contagem, ele para.
     * Se ele voltou atrás, é uma corrida nova. */
    static float relogioAnterior = 0.0f;
    float passou = corrida->tempoDeCorrida - relogioAnterior;
    relogioAnterior = corrida->tempoDeCorrida;
    if (passou < 0.0f) {
        limparFumaca();
        passou = 0.0f;
    }
    if (passou > 0.0f) {
        for (int r = 0; r < TOTAL_RIVAIS; r++) {
            soltarFumacaDoCarro(camera, &corrida->rivais[r], passou);
        }
        soltarFumacaDoCarro(camera, jogador, passou);
    }
    atualizarFumaca(passou);

    desenharMundo3D(camera, corrida->pista, corrida);
}

/*
 * desenharPistaDeFundo
 * Fundo das telas de menu: a câmera passeia sozinha pela pista, devagar,
 * pela linha dela. Ela anda pela lista com moverNaPista(), então dá voltas
 * infinitas no circuito. Cada pista lembra onde o passeio parou.
 * Recebe: a pista.
 * Devolve: nada.
 */
void desenharPistaDeFundo(Pista *pista) {
    static PosicaoNaPista passeios[TOTAL_PISTAS];
    PosicaoNaPista *passeio = &passeios[pista->id];
    if (passeio->segmento == NULL) {
        passeio->segmento = pista->largada;
    }
    moverNaPista(passeio, VELOCIDADE_CAMERA_MENU * GetFrameTime());
    desenharMundo3D(montarCamera(*passeio, 0.0f), pista, NULL);
}

/*
 * desenharFundoAlternado
 * O fundo dos menus: mostra cada pista por um tempo, trocando com um
 * escurecimento rápido para a troca não ser um salto.
 * Recebe: as pistas, na ordem de IdPista.
 * Devolve: nada.
 */
void desenharFundoAlternado(Pista *pistas[]) {
    static float relogio = 0.0f;
    relogio += GetFrameTime();
    int qual = (int) (relogio / SEGUNDOS_POR_PISTA) % TOTAL_PISTAS;
    float noTurno = fmodf(relogio, SEGUNDOS_POR_PISTA);

    desenharPistaDeFundo(pistas[qual]);

    float escuro = 1.0f - fminf(fminf(noTurno, SEGUNDOS_POR_PISTA - noTurno) / DURACAO_DA_TROCA_DE_PISTA, 1.0f);
    if (escuro > 0.0f) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, escuro));
    }
}

/*
 * desenharCarroParado
 * A miniatura de um carro, para a garagem e o menu.
 * Recebe: o número do carro, o centro da base na tela e a escala.
 * Devolve: nada.
 */
void desenharCarroParado(int numero, float centroX, float baseY, float escala) {
    desenharMiniatura(numero, centroX, baseY, LARGURA_CARRO * escala);
}
