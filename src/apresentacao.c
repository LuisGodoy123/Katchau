/*
 * apresentacao.c
 * ------------------------------------------------------------------
 * A apresentação da largada: o jogo de câmeras e os letreiros. Veja apresentacao.h.
 * ------------------------------------------------------------------
 */

#include <math.h>
#include <stddef.h>
#include "raylib.h"
#include "raymath.h"
#include "apresentacao.h"
#include "render.h"
#include "hud.h"
#include "visual.h"
#include "controle.h"
#include "corrida.h"

/* Quanto dura cada tomada, em segundos */
static const float DURACAO_DA_TOMADA[TOTAL_TOMADAS] = {
    [TOMADA_PISTA] = 5.5f,
    [TOMADA_GRID] = 4.0f,
    [TOMADA_PILOTOS] = 11.5f,   /* cerca de 1,5 s por fila do grid, sem parar */
    [TOMADA_JOGADOR] = 4.5f,
    [TOMADA_PRONTO] = 1.6f,
};

#define ESCURECER              0.35f   /* segundos de tela preta entre uma tomada e outra (entrando e saindo) */
#define DURACAO_DO_PULO        0.45f   /* pulou: a tela escurece nesse tempo e a corrida começa */
#define ALTURA_DA_TARJA        0.11f   /* as tarjas pretas de cinema, em fração da altura da tela */
#define ENTRADA_DA_TARJA       0.7f    /* segundos para as tarjas entrarem */
#define SAIDA_DA_TRILHA        3.0f    /* segundos finais em que a música vai sumindo */
#define ENTRADA_DO_LETREIRO    0.5f    /* segundos para um letreiro deslizar para dentro */

/* ---------- As câmeras ---------- */
#define DRONE_DE               300.0f  /* metros à frente da pole onde o drone começa... */
#define DRONE_ATE              85.0f   /* ...e onde termina, já perto do grid */
#define DRONE_ALTURA_DE        58.0f
#define DRONE_ALTURA_ATE       30.0f
#define DRONE_LADO_DE          -38.0f  /* metros para o lado da pista (negativo = esquerda) */
#define DRONE_LADO_ATE         -18.0f
#define DRONE_OLHA_ADIANTE     110.0f  /* o drone olha para esse ponto da pista, na direção do grid */
#define GRUA_A_FRENTE          20.0f   /* metros à frente da pole */
#define GRUA_ALTURA_DE         8.5f    /* abaixo do pórtico da largada, que fica entre a grua e o grid */
#define GRUA_ALTURA_ATE        4.0f
#define GRUA_LADO              9.0f    /* a grua vai de um lado ao outro do grid */
#define CAMINHADA_ALTURA       1.15f   /* a câmera que atravessa o grid, quase rente ao chão */
#define CAMINHADA_OLHA         12.0f   /* metros adiante (para trás no grid) que ela olha */
#define CAMINHADA_FOLGA        5.0f    /* começa essa distância à frente da pole e acaba essa atrás do último */
#define FILAS_DO_GRID          ((TOTAL_CARROS + 1) / 2)   /* os carros largam em duplas, lado a lado */
#define DIANTE_DA_FILA         7.0f    /* metros: o menor recuo da câmera diante de uma dupla */
#define FOLGA_DOS_LADOS        3.0f    /* metros além do centro de cada carro que precisam caber no quadro */
#define METADE_DA_VISAO        0.55f   /* tangente da metade do ângulo de visão na horizontal, com margem */
#define ALCANCE_DO_NOME        1.0f    /* da distância entre filas: o nome fica até a metade do caminho para a próxima */
#define ATRASO_DO_NOME         1.3f    /* em filas: o nome segue a câmera, não chega antes dos carros */
#define TROCA_DO_NOME          0.08f   /* e a troca de uma dupla para a outra é rápida, sem se sobrepor */
#define ORBITA_RAIO_DE         6.8f
#define ORBITA_RAIO_ATE        5.2f
#define ORBITA_ALTURA_DE       1.9f
#define ORBITA_ALTURA_ATE      1.2f
#define ORBITA_ANGULO_DE       25.0f   /* graus a partir da frente do carro, girando para a direita... */
#define ORBITA_ANGULO_ATE      195.0f  /* ...até quase atrás dele, de onde a câmera da corrida assume */

/* ==================================================================
 * Utilidades
 * ================================================================== */

static float suavizar(float t) {
    t = Clamp(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

static float misturar(float de, float ate, float t) {
    return de + (ate - de) * t;
}

/*
 * naPista
 * Recebe: um ponto da pista e quantos metros andar (negativo = para trás).
 * Devolve: o ponto depois de andar.
 */
static PosicaoNaPista naPista(PosicaoNaPista base, float metros) {
    moverNaPista(&base, metros * UNIDADES_POR_METRO);
    return base;
}

/*
 * frenteDaPista
 * Recebe: um ponto da pista e onde guardar a frente e a direita dela (vistas de cima, x e z).
 * Devolve: nada.
 */
static void frenteDaPista(PosicaoNaPista lugar, float *frenteX, float *frenteZ, float *direitaX, float *direitaZ) {
    PontoNoMundo aqui = pontoNoMundo(lugar, 0.0f);
    PontoNoMundo adiante = pontoNoMundo(naPista(lugar, 2.0f), 0.0f);
    float dx = adiante.x - aqui.x, dz = adiante.z - aqui.z;
    float tamanho = hypotf(dx, dz);
    if (tamanho < 1e-4f) {
        dx = 0.0f;
        dz = -1.0f;
        tamanho = 1.0f;
    }
    *frenteX = dx / tamanho;
    *frenteZ = dz / tamanho;
    *direitaX = -*frenteZ;
    *direitaZ = *frenteX;
}

/*
 * pontoAoLado
 * Um ponto no mundo a partir da pista: tantos metros para o lado da linha do meio e tantos
 * acima do chão que está ali.
 * Recebe: o ponto da pista, os metros para a direita (negativo = esquerda) e a altura.
 * Devolve: o ponto, em metros.
 */
static Vector3 pontoAoLado(PosicaoNaPista lugar, float lateral, float altura) {
    PontoNoMundo meio = pontoNoMundo(lugar, 0.0f);
    float fx, fz, rx, rz;
    frenteDaPista(lugar, &fx, &fz, &rx, &rz);
    return (Vector3){ meio.x + rx * lateral, alturaDoChao(lugar, lateral) + altura, meio.z + rz * lateral };
}

static Vector3 centroDoCarro(const Carro *carro, float altura) {
    PontoNoMundo p = pontoNoMundo(carro->posicao, carro->deslocamentoLateral);
    return (Vector3){ p.x, p.y + altura, p.z };
}

static Camera3D novaCamera(Vector3 posicao, Vector3 alvo, float grausDeVisao) {
    Camera3D camera = { 0 };
    camera.position = posicao;
    camera.target = alvo;
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = grausDeVisao;
    camera.projection = CAMERA_PERSPECTIVE;
    return camera;
}

/* O carro da pole (o mais à frente no grid) e quantos metros o grid tem, da pole ao último */
static const Carro *poleDoGrid(const Corrida *corrida) {
    return corrida->classificacao[0];
}

static float metrosDoGrid(const Corrida *corrida) {
    const Carro *ultimo = corrida->classificacao[TOTAL_CARROS - 1];
    return (poleDoGrid(corrida)->distanciaPercorrida - ultimo->distanciaPercorrida) / UNIDADES_POR_METRO;
}

/* ==================================================================
 * As tomadas
 * ================================================================== */

/* 1) O drone voa alto sobre a pista, vindo de longe até o grid */
static Camera3D cameraDaPista(const Corrida *corrida, float t) {
    float e = suavizar(t);
    PosicaoNaPista pole = poleDoGrid(corrida)->posicao;
    float onde = misturar(DRONE_DE, DRONE_ATE, e);
    PosicaoNaPista lugar = naPista(pole, onde);
    Vector3 posicao = pontoAoLado(lugar, misturar(DRONE_LADO_DE, DRONE_LADO_ATE, e), misturar(DRONE_ALTURA_DE, DRONE_ALTURA_ATE, e));
    Vector3 alvo = pontoAoLado(naPista(pole, onde - DRONE_OLHA_ADIANTE), 0.0f, 0.0f);
    return novaCamera(posicao, alvo, 55.0f);
}

/* 2) A grua passa por cima do grid, de frente para os carros, descendo */
static Camera3D cameraDoGrid(const Corrida *corrida, float t) {
    float e = suavizar(t);
    PosicaoNaPista pole = poleDoGrid(corrida)->posicao;
    Vector3 posicao = pontoAoLado(naPista(pole, GRUA_A_FRENTE), misturar(-GRUA_LADO, GRUA_LADO, e),
                                  misturar(GRUA_ALTURA_DE, GRUA_ALTURA_ATE, e));
    Vector3 alvo = pontoAoLado(naPista(pole, -metrosDoGrid(corrida) * 0.45f), 0.0f, 0.4f);
    return novaCamera(posicao, alvo, 50.0f);
}

/* 3) A câmera atravessa o grid de fila em fila, de frente para os carros: anda até a dupla, para
 * diante dela (os dois nomes na tela) e segue para a próxima */
/*
 * dianteDaDupla
 * Quanto a câmera recua diante de uma dupla para os dois carros caberem inteiros: em pistas de
 * grid largo (Interlagos) ela fica mais longe.
 * Recebe: a corrida.
 * Devolve: metros.
 */
static float dianteDaDupla(const Corrida *corrida) {
    float maisLonge = 0.0f;
    for (int i = 0; i < TOTAL_CARROS; i++) {
        const Carro *c = corrida->classificacao[i];
        float lado = fabsf(c->deslocamentoLateral * larguraDoLado(c->posicao.segmento, c->deslocamentoLateral));
        maisLonge = fmaxf(maisLonge, lado);
    }
    return fmaxf(DIANTE_DA_FILA, (maisLonge + FOLGA_DOS_LADOS) / METADE_DA_VISAO);
}

static float metrosDaFila(const Corrida *corrida, int fila) {
    const Carro *pole = poleDoGrid(corrida);
    const Carro *primeiro = corrida->classificacao[2 * fila];
    return (primeiro->distanciaPercorrida - pole->distanciaPercorrida) / UNIDADES_POR_METRO;
}

static float ondeEstaACaminhada(const Corrida *corrida, float t) {
    /* Velocidade quase constante, com uma entrada e uma saída suaves: a câmera nunca para */
    float e = misturar(t, suavizar(t), 0.35f);
    float de = CAMINHADA_FOLGA + dianteDaDupla(corrida);
    /* termina com o nome da última dupla inteiro na tela (o nome segue a câmera com um atraso) */
    float entreFilas = fabsf(metrosDaFila(corrida, 1) - metrosDaFila(corrida, 0));
    float ate = metrosDaFila(corrida, FILAS_DO_GRID - 1) + dianteDaDupla(corrida) - (ATRASO_DO_NOME + 0.3f) * entreFilas;
    return misturar(de, ate, e);
}

/*
 * presencaDaFila
 * O quanto o nome de uma dupla está na tela: cheio quando a câmera passa diante dela, sumindo
 * na metade do caminho até a próxima, numa troca rápida (sem as duas na tela ao mesmo tempo).
 * Recebe: a corrida, a fila, o andamento da tomada e onde guardar de que lado ela sai (+1 entrando, -1 saindo).
 * Devolve: de 0 a 1.
 */
static float presencaDaFila(const Corrida *corrida, int fila, float t, float *lado) {
    float onde = ondeEstaACaminhada(corrida, t);
    float diante = metrosDaFila(corrida, fila) + dianteDaDupla(corrida);
    float entreFilas = fabsf(metrosDaFila(corrida, 1) - metrosDaFila(corrida, 0));
    if (entreFilas < 1.0f) {
        entreFilas = 8.0f;
    }
    /* > 0: a câmera ainda vem chegando; < 0: já passou. O nome fica centrado um pouco depois do
     * ponto diante da dupla, para só entrar quando os dois carros já estão no quadro */
    float d = (onde - diante) / entreFilas + ATRASO_DO_NOME;
    *lado = (d > 0.0f) ? 1.0f : -1.0f;
    float meio = ALCANCE_DO_NOME / 2.0f;
    /* termina de sumir exatamente no meio do caminho, onde a próxima começa a entrar: nunca as duas juntas */
    return 1.0f - suavizar(Clamp((fabsf(d) - (meio - 2.0f * TROCA_DO_NOME)) / (2.0f * TROCA_DO_NOME), 0.0f, 1.0f));
}

static Camera3D cameraDosPilotos(const Corrida *corrida, float t) {
    PosicaoNaPista pole = poleDoGrid(corrida)->posicao;
    float onde = ondeEstaACaminhada(corrida, t);
    Vector3 posicao = pontoAoLado(naPista(pole, onde), 0.0f, CAMINHADA_ALTURA + 0.6f);
    Vector3 alvo = pontoAoLado(naPista(pole, onde - fmaxf(CAMINHADA_OLHA, dianteDaDupla(corrida))), 0.0f, 0.5f);   /* mira a dupla */
    return novaCamera(posicao, alvo, 62.0f);
}

/* 4) A câmera gira em volta do carro do jogador */
static Camera3D cameraDoJogador(const Corrida *corrida, float t) {
    float e = suavizar(t);
    const Carro *jogador = &corrida->jogador;
    float fx, fz, rx, rz;
    frenteDaPista(jogador->posicao, &fx, &fz, &rx, &rz);
    float angulo = misturar(ORBITA_ANGULO_DE, ORBITA_ANGULO_ATE, e) * DEG2RAD;
    float raio = misturar(ORBITA_RAIO_DE, ORBITA_RAIO_ATE, e);
    Vector3 centro = centroDoCarro(jogador, 0.6f);
    Vector3 posicao = { centro.x + raio * (cosf(angulo) * fx + sinf(angulo) * rx),
                        centro.y + misturar(ORBITA_ALTURA_DE, ORBITA_ALTURA_ATE, e),
                        centro.z + raio * (cosf(angulo) * fz + sinf(angulo) * rz) };
    return novaCamera(posicao, centro, 48.0f);
}

/* 5) Do fim da órbita até a câmera de corrida, atrás do carro */
static Camera3D cameraPronto(Corrida *corrida, float t) {
    float e = suavizar(t);
    Camera3D de = cameraDoJogador(corrida, 1.0f);
    Camera3D ate = cameraDaVisaoDoJogador(corrida);
    Camera3D camera = ate;
    camera.position = Vector3Lerp(de.position, ate.position, e);
    camera.target = Vector3Lerp(de.target, ate.target, e);
    camera.up = Vector3Normalize(Vector3Lerp(de.up, ate.up, e));
    camera.fovy = misturar(de.fovy, ate.fovy, e);
    return camera;
}

/* ==================================================================
 * O jogo de câmeras da vitória
 * ================================================================== */

#define TOMADAS_DA_VITORIA     4
#define FADE_DO_CORTE          0.22f   /* segundos de escuro saindo do corte */
#define FADE_ANTES_DO_CORTE    0.12f   /* e entrando nele */

static const float DURACAO_DA_TOMADA_DA_VITORIA[TOMADAS_DA_VITORIA] = { 4.8f, 4.6f, 4.2f, 5.6f };

/*
 * cameraDaVitoria
 * As quatro câmeras da festa, uma depois da outra e de novo (cada vez do outro lado da pista):
 *   0) de frente      baixa, à frente do carro, afastando devagar;
 *   1) perseguição    atrás e um pouco acima, com a pista e o céu adiante;
 *   2) lateral        anda junto, à frente e ao lado do carro, abrindo devagar;
 *   3) grua           gira devagar em volta do carro, subindo.
 * Em todas o carro fica no terço de baixo, com tamanho parecido, e o cenário e o céu dos fogos em cima.
 * Recebe: a corrida, os segundos desde que o jogador cruzou a linha, onde guardar se houve corte
 *         neste quadro e quanto a imagem está escura (0 a 1, perto dos cortes).
 * Devolve: a câmera.
 */
Camera3D cameraDaVitoria(Corrida *corrida, float segundosDesdeAChegada, int *cortou, float *escuro) {
    static int tomadaAnterior = -1, cicloAnterior = -1;
    static float tempoAnterior = 0.0f;

    float ciclo = 0.0f;
    for (int i = 0; i < TOMADAS_DA_VITORIA; i++) {
        ciclo += DURACAO_DA_TOMADA_DA_VITORIA[i];
    }
    float tempo = fmaxf(segundosDesdeAChegada, 0.0f);
    if (tempo < tempoAnterior - 0.5f) {   /* corrida nova */
        tomadaAnterior = -1;
        cicloAnterior = -1;
    }
    tempoAnterior = tempo;

    int voltas = (int) (tempo / ciclo);
    float noCiclo = tempo - (float) voltas * ciclo;
    int tomada = 0;
    while (tomada < TOMADAS_DA_VITORIA - 1 && noCiclo >= DURACAO_DA_TOMADA_DA_VITORIA[tomada]) {
        noCiclo -= DURACAO_DA_TOMADA_DA_VITORIA[tomada];
        tomada++;
    }
    float duracao = DURACAO_DA_TOMADA_DA_VITORIA[tomada];

    const Carro *jogador = &corrida->jogador;
    float lado = (voltas % 2 == 0) ? 1.0f : -1.0f;   /* a cada ciclo, o outro lado da pista */
    int tomadaNova = (tomada != tomadaAnterior || voltas != cicloAnterior);
    if (tomadaNova) {
        tomadaAnterior = tomada;
        cicloAnterior = voltas;
    }
    if (cortou != NULL) {
        *cortou = tomadaNova;
    }
    if (escuro != NULL) {
        float entrando = 1.0f - noCiclo / FADE_DO_CORTE;
        float saindo = (noCiclo - (duracao - FADE_ANTES_DO_CORTE)) / FADE_ANTES_DO_CORTE;
        *escuro = Clamp(fmaxf(entrando, saindo), 0.0f, 1.0f);
    }

    float fx, fz, rx, rz;
    frenteDaPista(jogador->posicao, &fx, &fz, &rx, &rz);
    Vector3 carro = centroDoCarro(jogador, 0.5f);
    Vector3 frente = { fx, 0.0f, fz }, direita = { rx, 0.0f, rz };
    float e = suavizar(noCiclo / duracao);

    switch (tomada) {
        case 0: {   /* de frente: baixa, anda à frente do carro e vai se afastando devagar */
            float adiante = misturar(8.0f, 13.0f, e);
            Vector3 posicao = Vector3Add(Vector3Add(carro, Vector3Scale(frente, adiante)), Vector3Add(Vector3Scale(direita, 2.5f * lado), (Vector3){ 0.0f, 1.0f, 0.0f }));
            Vector3 alvo = Vector3Add(carro, (Vector3){ 0.0f, 1.5f, 0.0f });
            return novaCamera(posicao, alvo, 46.0f);
        }
        case 1: {   /* perseguição: atrás e um pouco acima, o carro embaixo e a pista e o céu adiante */
            Vector3 posicao = Vector3Add(Vector3Add(carro, Vector3Scale(frente, -11.0f)), Vector3Add(Vector3Scale(direita, 2.5f * lado), (Vector3){ 0.0f, 3.2f, 0.0f }));
            Vector3 alvo = Vector3Add(Vector3Add(carro, Vector3Scale(frente, 18.0f)), (Vector3){ 0.0f, 3.4f, 0.0f });
            return novaCamera(posicao, alvo, 52.0f);
        }
        case 2: {   /* lateral: anda junto, à frente e ao lado, abrindo devagar */
            float adiante = misturar(7.0f, 10.0f, e);
            Vector3 posicao = Vector3Add(Vector3Add(carro, Vector3Scale(frente, adiante)), Vector3Add(Vector3Scale(direita, 6.0f * lado), (Vector3){ 0.0f, 1.3f, 0.0f }));
            Vector3 alvo = Vector3Add(carro, (Vector3){ 0.0f, 1.9f, 0.0f });
            return novaCamera(posicao, alvo, 50.0f);
        }
        default: {  /* grua: gira devagar em volta do carro, subindo, com a festa atrás */
            float angulo = (misturar(-35.0f, 35.0f, e) + (lado > 0.0f ? 0.0f : 180.0f)) * DEG2RAD;
            float raio = misturar(14.0f, 18.0f, e);
            Vector3 volta = Vector3Add(Vector3Scale(frente, cosf(angulo) * raio), Vector3Scale(direita, sinf(angulo) * raio));
            Vector3 posicao = Vector3Add(Vector3Add(carro, volta), (Vector3){ 0.0f, misturar(4.0f, 7.0f, e), 0.0f });
            Vector3 alvo = Vector3Add(carro, (Vector3){ 0.0f, 2.6f, 0.0f });
            return novaCamera(posicao, alvo, 50.0f);
        }
    }
}

/* ==================================================================
 * Andar no tempo
 * ================================================================== */

/*
 * iniciarApresentacao
 * Começa a apresentação do zero (na primeira tomada). Chamar quando a corrida começa.
 * Recebe: a apresentação.
 * Devolve: nada.
 */
void iniciarApresentacao(Apresentacao *apresentacao) {
    apresentacao->ativa = 1;
    apresentacao->tomada = TOMADA_PISTA;
    apresentacao->tempoNaTomada = 0.0f;
    apresentacao->tomadaNova = 1;
    apresentacao->pulando = 0.0f;
}

/*
 * apresentacaoAtiva
 * Recebe: a apresentação.
 * Devolve: 1 enquanto ela está passando (a corrida fica parada).
 */
int apresentacaoAtiva(const Apresentacao *apresentacao) {
    return apresentacao->ativa;
}

/*
 * atualizarApresentacao
 * Um quadro da apresentação: passa de uma tomada para a outra e, no fim, libera a corrida.
 * Pular escurece a tela e acaba em seguida.
 * Recebe: a apresentação, se o jogador pediu para pular e o tempo do quadro.
 * Devolve: nada.
 */
void atualizarApresentacao(Apresentacao *apresentacao, int pular, float tempoQuadro) {
    apresentacao->tomadaNova = 0;
    if (!apresentacao->ativa) {
        return;
    }
    if (apresentacao->pulando > 0.0f) {
        apresentacao->pulando -= tempoQuadro;
        if (apresentacao->pulando <= 0.0f) {
            apresentacao->ativa = 0;
        }
        return;
    }
    if (pular) {
        apresentacao->pulando = DURACAO_DO_PULO;
        return;
    }
    apresentacao->tempoNaTomada += tempoQuadro;
    if (apresentacao->tempoNaTomada >= DURACAO_DA_TOMADA[apresentacao->tomada]) {
        if (apresentacao->tomada == TOTAL_TOMADAS - 1) {
            apresentacao->ativa = 0;
            return;
        }
        apresentacao->tomada++;
        apresentacao->tempoNaTomada = 0.0f;
        apresentacao->tomadaNova = 1;
    }
}

/* ==================================================================
 * Os letreiros
 * ================================================================== */

/* Quanto o letreiro já entrou (0 a 1) e quanto já saiu, pelo tempo dentro da tomada */
static float entradaDoLetreiro(float tempo, float atraso, float duracao) {
    float entrou = suavizar((tempo - atraso) / ENTRADA_DO_LETREIRO);
    float saiu = suavizar((tempo - (duracao - ESCURECER - ENTRADA_DO_LETREIRO * 0.6f)) / (ENTRADA_DO_LETREIRO * 0.6f));
    return entrou * (1.0f - saiu);
}

/*
 * desenharLetreiroGrande
 * O letreiro do canto de baixo, à esquerda: uma faixa vermelha com um título grande e uma linha
 * menor embaixo, que desliza da esquerda.
 * Recebe: a linha de cima (pequena, pode ser NULL), o título, a linha de baixo (pode ser NULL),
 *         quanto já entrou (0 a 1) e a base da tarja de baixo.
 * Devolve: nada.
 */
static void desenharLetreiroGrande(const char *chapeu, const char *titulo, const char *linha, float entrou, float baseY) {
    if (entrou <= 0.0f) {
        return;
    }
    float tamanho = 46.0f;
    float largura = fmaxf(larguraDoTexto(FONTE_TITULO, titulo, tamanho), linha ? larguraDoTexto(FONTE_FORTE, linha, 18.0f) : 0.0f) + 70.0f;
    float x = -largura + (largura + 40.0f) * entrou;
    float y = baseY - 104.0f;
    Color alfa = Fade(WHITE, entrou);

    if (chapeu != NULL) {
        float lc = larguraDoTexto(FONTE_FORTE, chapeu, 15.0f) + 26.0f;
        desenharParalelogramo(x + 10.0f, y - 24.0f, lc, 22.0f, 7.0f, Fade(COR_AMARELO, entrou));
        escrever(FONTE_FORTE, chapeu, x + 23.0f, noMeio(FONTE_FORTE, 15.0f, y - 13.0f), 15.0f, Fade(COR_GRAFITE, entrou));
    }
    desenharParalelogramo(x, y, largura, 58.0f, 14.0f, Fade(COR_VERMELHO, entrou));
    desenharParalelogramo(x, y, largura, 29.0f, 7.0f, Fade(WHITE, 0.10f * entrou));
    desenharParalelogramo(x - 12.0f, y, 7.0f, 58.0f, 14.0f, Fade(COR_AMARELO, entrou));
    escrever(FONTE_TITULO, titulo, x + 24.0f, noMeio(FONTE_TITULO, tamanho, y + 29.0f), tamanho, Fade(COR_TEXTO_CLARO, alfa.a / 255.0f));
    if (linha != NULL) {
        desenharParalelogramo(x + 6.0f, y + 62.0f, largura - 30.0f, 26.0f, 8.0f, Fade(COR_PAINEL, 0.92f * entrou));
        escrever(FONTE_FORTE, linha, x + 24.0f, noMeio(FONTE_FORTE, 18.0f, y + 75.0f), 18.0f, Fade(COR_CROMO, entrou));
    }
}

/*
 * desenharPlacaDoPiloto
 * A placa de um piloto da dupla: a posição de largada e o nome, numa caixa arredondada; a do
 * jogador fica vermelha, com a etiqueta VOCÊ em cima.
 * Recebe: o carro, a posição (1 = pole), se é o jogador, o centro x, a base, quanto está na tela e o
 *         lado (+1 entrando, de baixo; -1 saindo, para cima).
 * Devolve: nada.
 */
static void desenharPlacaDoPiloto(const Carro *carro, int lugar, int ehJogador, float centroX, float baseY, float naTela, float lado) {
    const float altura = 48.0f, tamanho = 32.0f, folga = 8.0f, larguraDoLugar = 56.0f;
    float largura = larguraDoLugar + larguraDoTexto(FONTE_TITULO, carro->nome, tamanho) + 32.0f;
    float x = roundf((centroX - largura / 2.0f) / 4.0f) * 4.0f;
    float y = baseY - altura - 24.0f + lado * 16.0f * (1.0f - naTela);   /* entra de baixo, sai para cima */
    Rectangle placa = { x, y, largura, altura };
    float raio = raioDoTamanho(placa.width, placa.height);
    caixaArredondada(placa, raio, Fade(ehJogador ? COR_VERMELHO : COR_GRAFITE, 0.92f * naTela));
    caixaArredondada((Rectangle){ x + folga, y + folga, larguraDoLugar - folga, altura - 2.0f * folga }, raioDeDentro(raio, folga),
                     Fade(lugar <= 3 ? COR_AMARELO : COR_TEXTO_CLARO, naTela));
    escreverCentralizado(FONTE_TITULO, TextFormat("%dº", lugar), x + folga + (larguraDoLugar - folga) / 2.0f, noMeio(FONTE_TITULO, 24.0f, y + altura / 2.0f), 24.0f,
                         Fade(COR_GRAFITE, naTela));
    escrever(FONTE_TITULO, carro->nome, x + larguraDoLugar + 16.0f, noMeio(FONTE_TITULO, tamanho, y + altura / 2.0f), tamanho, Fade(COR_TEXTO_CLARO, naTela));
    if (ehJogador) {
        caixaArredondada((Rectangle){ x, y - 24.0f, 56.0f, 16.0f }, RAIO_PEQUENO, Fade(COR_AMARELO, naTela));
        escreverCentralizado(FONTE_FORTE, "VOCÊ", x + 28.0f, noMeio(FONTE_FORTE, 12.0f, y - 16.0f), 12.0f, Fade(COR_GRAFITE, naTela));
    }
}

/*
 * desenharDupla
 * Na caminhada pelo grid: os dois carros da fila, cada placa embaixo do seu lado da tela (o carro
 * de dentro à esquerda, o de fora à direita, como a câmera os vê).
 * Recebe: a corrida, a fila, quanto está na tela, de que lado ela entra ou sai e a base da tarja de baixo.
 * Devolve: nada.
 */
static void desenharDupla(Corrida *corrida, int fila, float naTela, float lado, float baseY) {
    if (naTela <= 0.0f) {
        return;
    }
    float meio = larguraVirtual() / 2.0f;
    for (int qual = 0; qual < 2; qual++) {
        int posicao = 2 * fila + qual;
        if (posicao >= TOTAL_CARROS) {
            break;
        }
        const Carro *carro = corrida->classificacao[posicao];
        /* Quem está à esquerda da pista aparece à esquerda da tela (a câmera olha para trás no grid) */
        int naEsquerda = carro->deslocamentoLateral > 0.0f;
        float centroX = naEsquerda ? meio - larguraVirtual() * 0.22f : meio + larguraVirtual() * 0.22f;
        desenharPlacaDoPiloto(carro, posicao + 1, carro == &corrida->jogador, centroX, baseY, naTela, lado);
    }
}

/*
 * volumeDaTrilhaNaApresentacao
 * A música do menu segue de trilha na apresentação e vai sumindo nos últimos segundos dela (ou
 * no pulo), para não cortar de uma vez quando a corrida começa.
 * Recebe: a apresentação.
 * Devolve: o volume, de 0 a 1.
 */
float volumeDaTrilhaNaApresentacao(const Apresentacao *apresentacao) {
    if (!apresentacao->ativa) {
        return 0.0f;
    }
    if (apresentacao->pulando > 0.0f) {
        return apresentacao->pulando / DURACAO_DO_PULO;
    }
    float falta = DURACAO_DA_TOMADA[apresentacao->tomada] - apresentacao->tempoNaTomada;
    for (int t = apresentacao->tomada + 1; t < TOTAL_TOMADAS; t++) {
        falta += DURACAO_DA_TOMADA[t];
    }
    return Clamp(falta / SAIDA_DA_TRILHA, 0.0f, 1.0f);
}

/* O selo de transmissão no canto de cima: "AO VIVO" com a bolinha piscando */
static void desenharSeloAoVivo(float topoY, float entrou) {
    float x = larguraVirtual() - 210.0f;
    float y = topoY + 12.0f;
    desenharParalelogramo(x, y, 170.0f, 26.0f, 8.0f, Fade(COR_PAINEL, 0.9f * entrou));
    float pisca = (fmodf((float) GetTime(), 1.0f) < 0.6f) ? 1.0f : 0.25f;
    DrawCircle((int) (x + 22.0f), (int) (y + 13.0f), 6.0f, Fade(COR_VERMELHO, pisca * entrou));
    escrever(FONTE_FORTE, "AO VIVO  |  KATCHAU", x + 36.0f, noMeio(FONTE_FORTE, 16.0f, y + 13.0f), 16.0f, Fade(COR_TEXTO_CLARO, entrou));
}

/* A dica para pular, no canto de baixo à direita */
static void desenharDicaDePular(float baseY, float entrou) {
    const char *tecla = nomeNoControle("ENTER");
    float largura = larguraDaTecla(tecla) + 8.0f + larguraDoTexto(FONTE_FORTE, "PULAR", 16.0f);
    float x = larguraVirtual() - 30.0f - largura;
    float y = baseY + (alturaVirtual() * ALTURA_DA_TARJA - 28.0f) / 2.0f;
    if (entrou <= 0.01f) {
        return;
    }
    x += desenharTecla(x, y, tecla) + 8.0f;
    escrever(FONTE_FORTE, "PULAR", x, noMeio(FONTE_FORTE, 16.0f, meioDaTecla(y)), 16.0f, Fade(COR_TEXTO_FRACO, entrou));
}

/* ==================================================================
 * Desenhar
 * ================================================================== */

/*
 * desenharApresentacao
 * Desenha um quadro da apresentação: o mundo pela câmera da tomada, as tarjas de cinema, os
 * letreiros e o escurecer entre as tomadas.
 * Recebe: a apresentação e a corrida.
 * Devolve: nada.
 */
void desenharApresentacao(const Apresentacao *apresentacao, Corrida *corrida) {
    TomadaDaApresentacao tomada = apresentacao->tomada;
    float duracao = DURACAO_DA_TOMADA[tomada];
    float tempo = apresentacao->tempoNaTomada;
    float t = Clamp(tempo / duracao, 0.0f, 1.0f);

    Camera3D camera;
    switch (tomada) {
        case TOMADA_PISTA:   camera = cameraDaPista(corrida, t); break;
        case TOMADA_GRID:    camera = cameraDoGrid(corrida, t); break;
        case TOMADA_PILOTOS: camera = cameraDosPilotos(corrida, t); break;
        case TOMADA_JOGADOR: camera = cameraDoJogador(corrida, t); break;
        default:             camera = cameraPronto(corrida, t); break;
    }
    desenharCorridaVistaDe(corrida, camera);

    comecarInterface(0);
    float largura = (float) larguraVirtual();
    float altura = (float) alturaVirtual();

    /* As tarjas de cinema: entram no começo e saem na última tomada, quando o jogo assume */
    float tarja = 1.0f;
    if (tomada == TOMADA_PISTA) {
        tarja = suavizar(tempo / ENTRADA_DA_TARJA);
    } else if (tomada == TOMADA_PRONTO) {
        tarja = 1.0f - suavizar(t);
    }
    float alturaDaTarja = altura * ALTURA_DA_TARJA * tarja;
    DrawRectangleRec((Rectangle){ 0, 0, largura, alturaDaTarja }, BLACK);
    DrawRectangleRec((Rectangle){ 0, altura - alturaDaTarja, largura, alturaDaTarja }, BLACK);
    float baseDosLetreiros = altura - altura * ALTURA_DA_TARJA;

    /* Os letreiros de cada tomada */
    const FichaDoNivel *nivel = fichaDoNivel(corrida->nivel);
    switch (tomada) {
        case TOMADA_PISTA:
            desenharLetreiroGrande("A CORRIDA", corrida->pista->nome,
                                   TextFormat("NÍVEL %s  |  %d VOLTAS  |  %d CARROS", nivel->nome, corrida->totalVoltas, TOTAL_CARROS),
                                   entradaDoLetreiro(tempo, 0.9f, duracao), baseDosLetreiros);
            break;
        case TOMADA_GRID:
            desenharLetreiroGrande("A LARGADA", "GRID DE LARGADA",
                                   TextFormat("POLE POSITION: %s", poleDoGrid(corrida)->nome),
                                   entradaDoLetreiro(tempo, 0.5f, duracao), baseDosLetreiros);
            break;
        case TOMADA_PILOTOS:
            for (int fila = 0; fila < FILAS_DO_GRID; fila++) {
                float lado = 1.0f;
                float naTela = presencaDaFila(corrida, fila, t, &lado);
                desenharDupla(corrida, fila, naTela, lado, baseDosLetreiros);
            }
            break;
        case TOMADA_JOGADOR: {
            int lugar = TOTAL_CARROS;
            for (int i = 0; i < TOTAL_CARROS; i++) {
                if (corrida->classificacao[i] == &corrida->jogador) {
                    lugar = i + 1;
                }
            }
            desenharLetreiroGrande("SEU CARRO", corrida->jogador.nome, TextFormat("LARGA EM %dº DE %d", lugar, TOTAL_CARROS),
                                   entradaDoLetreiro(tempo, 0.4f, duracao + ESCURECER), baseDosLetreiros);
            break;
        }
        default:
            break;
    }

    desenharSeloAoVivo(alturaDaTarja - altura * ALTURA_DA_TARJA, tarja);
    desenharDicaDePular(altura - alturaDaTarja, tarja);

    /* O escurecer: entra do preto no começo de cada tomada e vai para o preto no fim dela (menos
     * entre a órbita e a câmera da corrida, que é uma passagem contínua) */
    float preto = 0.0f;
    if (tomada != TOMADA_PRONTO && tempo < ESCURECER) {
        preto = 1.0f - tempo / ESCURECER;
    }
    if (tomada < TOMADA_JOGADOR && tempo > duracao - ESCURECER) {
        preto = fmaxf(preto, (tempo - (duracao - ESCURECER)) / ESCURECER);
    }
    if (apresentacao->pulando > 0.0f) {
        preto = fmaxf(preto, 1.0f - apresentacao->pulando / DURACAO_DO_PULO);
    }
    if (preto > 0.0f) {
        DrawRectangleRec((Rectangle){ 0, 0, largura, altura }, Fade(BLACK, Clamp(preto, 0.0f, 1.0f)));
    }
    terminarInterface();
}
