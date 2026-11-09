/*
 * fogos.c
 * ------------------------------------------------------------------
 * Os fogos de artifício da vitória. Veja fogos.h.
 *
 * Tipos de fogo: peônia (esfera), crisântemo (cada brasa deixa um rastro de faíscas), salgueiro (chuva
 * dourada que cai devagar), anel, crossette (as brasas se partem em cruz no meio do caminho), brocado
 * (chuva que cintila em estrelinhas), coração e pistilo (duas cascas de cores diferentes com um miolo branco).
 * ------------------------------------------------------------------
 */

#include <math.h>
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "fogos.h"
#include "efeitos.h"

#define MAXIMO_DE_BRASAS       9000
#define MAXIMO_DE_FOGUETES     28
#define MAXIMO_DE_FULGORES     72
#define GRAVIDADE              9.8f
#define GRAVIDADE_DO_FOGUETE   4.9f    /* o foguete sobe "mais leve" que uma pedra: o motor ainda empurra */
#define ALTURA_MINIMA          28.0f   /* metros acima do chão onde um fogo estoura... */
#define ALTURA_MAXIMA          58.0f
#define DISTANCIA_MINIMA       50.0f   /* ...e a que distância da câmera */
#define DISTANCIA_MAXIMA       170.0f
#define PAUSA_MINIMA           0.13f   /* segundos entre um foguete e o próximo */
#define PAUSA_MAXIMA           0.38f
#define FOGUETES_DA_ABERTURA   6       /* a salva do primeiro instante da festa */
#define FOGOS_NO_CORTE         2       /* estouram na hora quando a câmera corta para outro lugar */
#define ALCANCE_DO_SOM         330.0f  /* metros: além disso o estrondo some */
#define FOLGA_DE_BRASAS        0.80f   /* com mais brasas que essa parte do limite, rastros e estrelinhas não nascem */

/* O que a brasa faz além de voar */
enum { BRASA_COMUM, BRASA_COM_RASTRO, BRASA_QUE_SE_PARTE, BRASA_COM_ESTRELINHAS };

typedef struct {
    Vector3 posicao;
    Vector3 velocidade;
    float vida;          /* segundos que faltam */
    float duracao;
    float arrasto;       /* por segundo: o ar freia a brasa */
    float peso;          /* quanto da gravidade a brasa sente */
    float cauda;         /* segundos de caminho que o risco mostra */
    float semente;       /* fase do cintilar */
    float proximoRastro; /* segundos até a próxima faísca de rastro */
    int cintila;         /* 1: no fim da vida a brasa pisca */
    int comportamento;   /* BRASA_* */
    Color cor;
} Brasa;

typedef struct {
    Vector3 posicao;
    Vector3 velocidade;
    float tempoAteEstourar;
    float sobraDoRastro;
    int tipo;
    Color cor, cor2;
} Foguete;

typedef struct {
    Vector3 posicao;
    float vida;
    float duracao;
    float tamanho;
    float forca;         /* 0 a 1: o quanto brilha */
    int anel;            /* 1: é o anel de luz que se abre, e não o disco */
    Color cor;
} Fulgor;

enum { FOGO_PEONIA, FOGO_CRISANTEMO, FOGO_SALGUEIRO, FOGO_ANEL, FOGO_CROSSETTE, FOGO_BROCADO, FOGO_CORACAO, FOGO_PISTILO, TOTAL_TIPOS_DE_FOGO };

static const Color PALETA[] = {
    { 255, 70, 60, 255 },    /* vermelho */
    { 255, 190, 60, 255 },   /* dourado */
    { 90, 255, 130, 255 },   /* verde */
    { 80, 150, 255, 255 },   /* azul */
    { 255, 90, 225, 255 },   /* magenta */
    { 255, 255, 235, 255 },  /* branco */
    { 90, 245, 255, 255 },   /* ciano */
    { 255, 130, 40, 255 },   /* laranja */
    { 190, 120, 255, 255 },  /* violeta */
    { 200, 255, 90, 255 },   /* limão */
};
#define TOTAL_DE_CORES ((int) (sizeof(PALETA) / sizeof(PALETA[0])))

static Brasa brasas[MAXIMO_DE_BRASAS];
static int totalDeBrasas = 0;
static Foguete foguetes[MAXIMO_DE_FOGUETES];
static int totalDeFoguetes = 0;
static Fulgor fulgores[MAXIMO_DE_FULGORES];
static int totalDeFulgores = 0;

static float tempoAteOProximo = 0.0f;
static int aberturaDada = 0;
static Camera3D ouvinte;                 /* de onde se ouvem os estrondos: a câmera da festa */
static int temOuvinte = 0;

static float aleatorio(float de, float ate) {
    return de + (ate - de) * (float) GetRandomValue(0, 10000) / 10000.0f;
}

/*
 * limparFogos
 * Apaga todos os fogos e recomeça a festa do zero (corrida nova).
 * Recebe: nada.
 * Devolve: nada.
 */
void limparFogos(void) {
    totalDeBrasas = 0;
    totalDeFoguetes = 0;
    totalDeFulgores = 0;
    tempoAteOProximo = 0.0f;
    aberturaDada = 0;
    temOuvinte = 0;
}

/* Uma direção qualquer, sorteada em toda a esfera */
static Vector3 direcaoNaEsfera(void) {
    float z = aleatorio(-1.0f, 1.0f), a = aleatorio(0.0f, 2.0f * PI), r = sqrtf(1.0f - z * z);
    return (Vector3){ r * cosf(a), z, r * sinf(a) };
}

/*
 * soltarBrasa
 * Uma brasa nova (com o limite cheio, ela é ignorada: o fogo só fica um pouco mais ralo).
 */
static void soltarBrasa(Vector3 posicao, Vector3 velocidade, float vida, float arrasto, float peso, float cauda,
                        int cintila, int comportamento, Color cor) {
    if (totalDeBrasas >= MAXIMO_DE_BRASAS) {
        return;
    }
    Brasa *b = &brasas[totalDeBrasas++];
    b->posicao = posicao;
    b->velocidade = velocidade;
    b->vida = b->duracao = vida;
    b->arrasto = arrasto;
    b->peso = peso;
    b->cauda = cauda;
    b->semente = aleatorio(0.0f, 100.0f);
    b->proximoRastro = aleatorio(0.0f, 0.05f);
    b->cintila = cintila;
    b->comportamento = comportamento;
    b->cor = cor;
}

static void soltarFulgor(Vector3 posicao, float tamanho, float vida, float forca, int anel, Color cor) {
    if (totalDeFulgores >= MAXIMO_DE_FULGORES) {
        return;
    }
    fulgores[totalDeFulgores++] = (Fulgor){ posicao, vida, vida, tamanho, forca, anel, cor };
}

/*
 * tocarEstrondo
 * O som de um fogo estourando (ou de um foguete saindo), pela distância e pelo lado em que fica
 * em relação à câmera.
 */
static void tocarEstrondo(Vector3 onde, int explosao) {
    if (!temOuvinte) {
        return;
    }
    Vector3 ate = Vector3Subtract(onde, ouvinte.position);
    float distancia = Vector3Length(ate);
    if (distancia > ALCANCE_DO_SOM) {
        return;
    }
    Vector3 frente = Vector3Normalize(Vector3Subtract(ouvinte.target, ouvinte.position));
    Vector3 direita = Vector3Normalize(Vector3CrossProduct(frente, ouvinte.up));
    float lado = Clamp(Vector3DotProduct(Vector3Normalize(ate), direita), -1.0f, 1.0f);
    float perto = 1.0f - distancia / ALCANCE_DO_SOM;
    tocarSomDeFogos(explosao, (explosao ? 0.95f : 0.45f) * (0.25f + 0.75f * perto), 0.5f + 0.4f * lado);
}

/* Uma casca de brasas em esfera: quantas, que velocidade, que vida e que comportamento */
static void cascaEsferica(Vector3 onde, int total, float vMin, float vMax, float vidaMin, float vidaMax, float arrasto,
                          float peso, float cauda, int cintila, int comportamento, Color cor, Color cor2, int cadaQuantasUsamACor2) {
    for (int k = 0; k < total; k++) {
        Color c = (cadaQuantasUsamACor2 > 0 && k % cadaQuantasUsamACor2 == 0) ? cor2 : cor;
        soltarBrasa(onde, Vector3Scale(direcaoNaEsfera(), aleatorio(vMin, vMax)), aleatorio(vidaMin, vidaMax), arrasto, peso, cauda,
                    cintila, comportamento, c);
    }
}

/*
 * estourar
 * O fogo abre no céu: um clarão colorido, o anel de luz que se abre, o brilho que ilumina o céu em volta e as
 * brasas saindo no desenho do tipo.
 * Recebe: onde, o tipo e as duas cores.
 * Devolve: nada.
 */
static void estourar(Vector3 onde, int tipo, Color cor, Color cor2) {
    Color corDoClarao = (tipo == FOGO_SALGUEIRO || tipo == FOGO_BROCADO) ? PALETA[1] : cor;
    soltarFulgor(onde, 11.0f, 0.26f, 0.5f, 0, corDoClarao);
    soltarFulgor(onde, 48.0f, 0.60f, 0.06f, 0, corDoClarao);            /* a luz que o fogo joga no céu em volta */
    soltarFulgor(onde, 30.0f, 0.50f, 0.32f, 1, corDoClarao);            /* o anel de luz que se abre */
    tocarEstrondo(onde, 1);

    switch (tipo) {
        case FOGO_PEONIA:
            cascaEsferica(onde, 190, 30.0f, 46.0f, 1.7f, 2.5f, 1.7f, 0.42f, 0.045f, 1, BRASA_COMUM, cor, cor2, 4);
            break;
        case FOGO_CRISANTEMO:   /* cada brasa deixa um rastro de faíscas atrás */
            cascaEsferica(onde, 110, 28.0f, 42.0f, 2.0f, 2.8f, 1.6f, 0.40f, 0.05f, 0, BRASA_COM_RASTRO, cor, cor2, 5);
            break;
        case FOGO_SALGUEIRO:
            cascaEsferica(onde, 140, 18.0f, 31.0f, 2.8f, 3.8f, 1.5f, 0.30f, 0.17f, 0, BRASA_COMUM, PALETA[1], PALETA[1], 0);
            break;
        case FOGO_CROSSETTE:    /* as brasas se partem em quatro no meio do caminho */
            cascaEsferica(onde, 46, 26.0f, 38.0f, 1.9f, 2.4f, 1.5f, 0.38f, 0.05f, 0, BRASA_QUE_SE_PARTE, cor, cor2, 0);
            break;
        case FOGO_BROCADO:      /* chuva dourada que, ao apagar, vira estrelinhas que cintilam */
            cascaEsferica(onde, 90, 20.0f, 34.0f, 1.8f, 2.6f, 1.5f, 0.34f, 0.13f, 0, BRASA_COM_ESTRELINHAS, PALETA[1], PALETA[7], 6);
            break;
        case FOGO_PISTILO:      /* uma casca grande de uma cor, uma pequena de outra por dentro e um miolo branco */
            cascaEsferica(onde, 130, 34.0f, 46.0f, 1.7f, 2.3f, 1.7f, 0.42f, 0.05f, 1, BRASA_COMUM, cor, cor, 0);
            cascaEsferica(onde, 70, 12.0f, 20.0f, 1.4f, 2.0f, 1.8f, 0.45f, 0.045f, 1, BRASA_COMUM, cor2, cor2, 0);
            cascaEsferica(onde, 24, 2.0f, 7.0f, 0.9f, 1.4f, 2.2f, 0.5f, 0.04f, 1, BRASA_COMUM, PALETA[5], PALETA[5], 0);
            break;
        case FOGO_CORACAO: {    /* o coração desenhado no céu, virado para a câmera */
            Vector3 para = { ouvinte.position.x - onde.x, 0.0f, ouvinte.position.z - onde.z };
            para = (!temOuvinte || Vector3LengthSqr(para) < 1e-6f) ? (Vector3){ 0.0f, 0.0f, 1.0f } : Vector3Normalize(para);
            Vector3 direita = { para.z, 0.0f, -para.x };
            for (int k = 0; k < 130; k++) {
                float t = 2.0f * PI * (float) k / 130.0f;
                float x = 16.0f * powf(sinf(t), 3.0f), y = 13.0f * cosf(t) - 5.0f * cosf(2.0f * t) - 2.0f * cosf(3.0f * t) - cosf(4.0f * t);
                float v = 26.0f / 17.0f;
                Vector3 d = Vector3Add(Vector3Scale(direita, x * v), (Vector3){ 0.0f, (y + 3.0f) * v, 0.0f });
                d = Vector3Add(d, Vector3Scale(para, aleatorio(-1.5f, 1.5f)));
                soltarBrasa(onde, d, aleatorio(1.7f, 2.1f), 1.75f, 0.28f, 0.05f, 1, BRASA_COMUM, PALETA[0]);
            }
            for (int k = 0; k < 24; k++) {
                soltarBrasa(onde, Vector3Scale(direcaoNaEsfera(), aleatorio(2.0f, 9.0f)), aleatorio(0.9f, 1.4f), 2.0f, 0.5f, 0.04f, 1, BRASA_COMUM, PALETA[4]);
            }
            break;
        }
        default: {              /* anel */
            Vector3 n = direcaoNaEsfera();
            Vector3 u = Vector3Normalize(Vector3CrossProduct(n, (fabsf(n.y) < 0.9f) ? (Vector3){ 0.0f, 1.0f, 0.0f } : (Vector3){ 1.0f, 0.0f, 0.0f }));
            Vector3 w = Vector3CrossProduct(n, u);
            for (int k = 0; k < 90; k++) {
                float a = 2.0f * PI * (float) k / 90.0f;
                Vector3 d = Vector3Add(Vector3Scale(u, cosf(a)), Vector3Scale(w, sinf(a)));
                soltarBrasa(onde, Vector3Scale(d, aleatorio(36.0f, 42.0f)), aleatorio(1.7f, 2.3f), 1.6f, 0.40f, 0.05f, 1, BRASA_COMUM, cor);
            }
            cascaEsferica(onde, 34, 6.0f, 18.0f, 1.2f, 1.8f, 1.8f, 0.5f, 0.04f, 1, BRASA_COMUM, cor2, cor2, 0);
            break;
        }
    }
}

/*
 * lancar
 * Um foguete sai do chão e vai estourar no céu, no campo de visão da câmera (à frente dela, na distância
 * e na altura que sorteia). Se a câmera anda (a de perseguição), o ponto é adiantado o tanto que ela
 * andar até o foguete estourar.
 * Recebe: a câmera, a velocidade dela, a altura do chão e se o fogo estoura já (sem foguete).
 * Devolve: nada.
 */
static void lancar(Camera3D camera, Vector3 velocidadeDaCamera, float alturaDoChao, int naHora) {
    Vector3 frente = { camera.target.x - camera.position.x, 0.0f, camera.target.z - camera.position.z };
    frente = (Vector3LengthSqr(frente) < 1e-6f) ? (Vector3){ 0.0f, 0.0f, -1.0f } : Vector3Normalize(frente);
    Vector3 direita = { -frente.z, 0.0f, frente.x };

    float distancia = aleatorio(DISTANCIA_MINIMA, DISTANCIA_MAXIMA);
    float lado = aleatorio(-0.55f, 0.55f) * distancia;
    float altura = aleatorio(ALTURA_MINIMA, ALTURA_MAXIMA);
    float tempo = aleatorio(1.3f, 1.9f);

    Vector3 alvo = {
        camera.position.x + frente.x * distancia + direita.x * lado + velocidadeDaCamera.x * (naHora ? 0.0f : tempo),
        alturaDoChao + altura,
        camera.position.z + frente.z * distancia + direita.z * lado + velocidadeDaCamera.z * (naHora ? 0.0f : tempo),
    };
    /* O tipo, com peso: as peônias e os crisântemos são os mais comuns, o coração e o pistilo os mais raros */
    static const int PESOS[TOTAL_TIPOS_DE_FOGO] = { 22, 20, 14, 10, 12, 10, 5, 7 };
    int sorteio = GetRandomValue(0, 99), tipo = 0, soma = 0;
    for (int k = 0; k < TOTAL_TIPOS_DE_FOGO; k++) {
        soma += PESOS[k];
        if (sorteio < soma) {
            tipo = k;
            break;
        }
    }
    Color cor = PALETA[GetRandomValue(0, TOTAL_DE_CORES - 1)];
    Color cor2 = PALETA[GetRandomValue(0, TOTAL_DE_CORES - 1)];

    if (naHora) {
        estourar(alvo, tipo, cor, cor2);
        return;
    }
    if (totalDeFoguetes >= MAXIMO_DE_FOGUETES) {
        return;
    }
    Foguete *f = &foguetes[totalDeFoguetes++];
    f->posicao = (Vector3){ alvo.x, alturaDoChao, alvo.z };
    f->velocidade = (Vector3){ aleatorio(-1.5f, 1.5f), (altura + 0.5f * GRAVIDADE_DO_FOGUETE * tempo * tempo) / tempo, aleatorio(-1.5f, 1.5f) };
    f->posicao.x -= f->velocidade.x * tempo;
    f->posicao.z -= f->velocidade.z * tempo;
    f->tempoAteEstourar = tempo;
    f->sobraDoRastro = 0.0f;
    f->tipo = tipo;
    f->cor = cor;
    f->cor2 = cor2;
    tocarEstrondo(f->posicao, 0);
}

/*
 * soltarFogosDaVitoria
 * A festa: no primeiro instante uma salva de foguetes, depois um foguete a cada poucos décimos de
 * segundo, sempre à frente da câmera. Quando a câmera corta para outro lugar, uns fogos estouram na
 * hora para a nova imagem já nascer com festa.
 * Recebe: a câmera, a velocidade dela (metros por segundo), a altura do chão, se houve um corte
 *         neste quadro e o tempo do quadro (0 na pausa).
 * Devolve: nada.
 */
void soltarFogosDaVitoria(Camera3D camera, Vector3 velocidadeDaCamera, float alturaDoChao, int cortou, float tempoQuadro) {
    ouvinte = camera;
    temOuvinte = 1;
    if (tempoQuadro <= 0.0f) {
        return;
    }
    if (!aberturaDada) {
        for (int k = 0; k < FOGUETES_DA_ABERTURA; k++) {
            lancar(camera, velocidadeDaCamera, alturaDoChao, 0);
        }
        aberturaDada = 1;
    }
    if (cortou) {
        for (int k = 0; k < FOGOS_NO_CORTE; k++) {
            lancar(camera, velocidadeDaCamera, alturaDoChao, 1);
        }
    }
    tempoAteOProximo -= tempoQuadro;
    while (tempoAteOProximo <= 0.0f) {
        lancar(camera, velocidadeDaCamera, alturaDoChao, 0);
        tempoAteOProximo += aleatorio(PAUSA_MINIMA, PAUSA_MAXIMA);
    }
}

/*
 * atualizarFogos
 * Os foguetes sobem (e soltam o rastro) e estouram; as brasas voam, o ar as freia, caem, soltam rastro,
 * se partem ou viram estrelinhas e apagam; os clarões e os anéis de luz se abrem e somem.
 * Recebe: o tempo do quadro (0 na pausa: tudo para no ar).
 * Devolve: nada.
 */
void atualizarFogos(float tempoQuadro) {
    if (tempoQuadro <= 0.0f) {
        return;
    }
    for (int i = 0; i < totalDeFoguetes; i++) {
        Foguete *f = &foguetes[i];
        f->velocidade.y -= GRAVIDADE_DO_FOGUETE * tempoQuadro;
        f->posicao = Vector3Add(f->posicao, Vector3Scale(f->velocidade, tempoQuadro));
        f->sobraDoRastro += 150.0f * tempoQuadro;
        while (f->sobraDoRastro >= 1.0f) {
            f->sobraDoRastro -= 1.0f;
            Vector3 solta = Vector3Add(Vector3Scale(direcaoNaEsfera(), aleatorio(0.4f, 2.6f)), (Vector3){ 0.0f, -2.5f, 0.0f });
            Color brasa = (GetRandomValue(0, 3) == 0) ? (Color){ 255, 235, 170, 255 } : (Color){ 255, 160, 60, 255 };
            soltarBrasa(f->posicao, solta, aleatorio(0.5f, 1.1f), 0.6f, 0.5f, 0.05f, 0, BRASA_COMUM, brasa);
        }
        f->tempoAteEstourar -= tempoQuadro;
        if (f->tempoAteEstourar <= 0.0f) {
            estourar(f->posicao, f->tipo, f->cor, f->cor2);
            foguetes[i--] = foguetes[--totalDeFoguetes];
        }
    }
    int cabeMais = totalDeBrasas < (int) (MAXIMO_DE_BRASAS * FOLGA_DE_BRASAS);
    for (int i = 0; i < totalDeBrasas; i++) {
        Brasa b = brasas[i];
        float antes = 1.0f - b.vida / b.duracao;
        b.vida -= tempoQuadro;
        float agora = 1.0f - b.vida / b.duracao;
        if (b.vida <= 0.0f) {
            if (b.comportamento == BRASA_COM_ESTRELINHAS && cabeMais) {   /* o fim da chuva: estrelinhas que cintilam caindo */
                for (int k = 0; k < 3; k++) {
                    Vector3 sai = Vector3Add(Vector3Scale(b.velocidade, 0.2f), Vector3Scale(direcaoNaEsfera(), aleatorio(0.5f, 3.0f)));
                    soltarBrasa(b.posicao, sai, aleatorio(0.7f, 1.5f), 1.2f, 0.6f, 0.025f, 1, BRASA_COMUM, (Color){ 255, 235, 170, 255 });
                }
            }
            brasas[i--] = brasas[--totalDeBrasas];
            continue;
        }
        if (b.comportamento == BRASA_QUE_SE_PARTE && antes < 0.42f && agora >= 0.42f && cabeMais) {   /* a cruz */
            Vector3 eixo = Vector3Normalize(Vector3CrossProduct(b.velocidade, (Vector3){ 0.0f, 1.0f, 0.1f }));
            Vector3 outro = Vector3Normalize(Vector3CrossProduct(b.velocidade, eixo));
            for (int k = 0; k < 4; k++) {
                Vector3 direcao = (k == 0) ? eixo : (k == 1) ? Vector3Negate(eixo) : (k == 2) ? outro : Vector3Negate(outro);
                soltarBrasa(b.posicao, Vector3Add(Vector3Scale(b.velocidade, 0.35f), Vector3Scale(direcao, 12.0f)), aleatorio(0.9f, 1.3f),
                            1.8f, 0.40f, 0.06f, 1, BRASA_COMUM, b.cor);
            }
            brasas[i--] = brasas[--totalDeBrasas];
            continue;
        }
        if (b.comportamento == BRASA_COM_RASTRO && cabeMais) {   /* deixa para trás uma faísca que fica parada e apaga */
            b.proximoRastro -= tempoQuadro;
            while (b.proximoRastro <= 0.0f) {
                b.proximoRastro += 0.03f;
                Color rastro = ColorLerp(b.cor, (Color){ 255, 200, 110, 255 }, 0.5f);
                soltarBrasa(b.posicao, Vector3Scale(b.velocidade, 0.04f), aleatorio(0.45f, 0.85f), 3.0f, 0.3f, 0.02f, 0, BRASA_COMUM, rastro);
            }
        }
        b.velocidade = Vector3Scale(b.velocidade, expf(-b.arrasto * tempoQuadro));
        b.velocidade.y -= GRAVIDADE * b.peso * tempoQuadro;
        b.posicao = Vector3Add(b.posicao, Vector3Scale(b.velocidade, tempoQuadro));
        brasas[i] = b;
    }
    for (int i = 0; i < totalDeFulgores; i++) {
        fulgores[i].vida -= tempoQuadro;
        if (fulgores[i].vida <= 0.0f) {
            fulgores[i--] = fulgores[--totalDeFulgores];
        }
    }
}

/*
 * riscoComRabo
 * Um risco de luz virado para a câmera: a cabeça com a cor cheia e a largura toda, e o rabo afinando e
 * sumindo (cada ponta com a sua cor).
 */
static void riscoComRabo(Camera3D camera, Vector3 cabeca, Vector3 rabo, float largura, Color corDaCabeca, Color corDoRabo) {
    Vector3 direcao = Vector3Subtract(cabeca, rabo);
    Vector3 lado = Vector3CrossProduct(direcao, Vector3Subtract(camera.position, cabeca));
    if (Vector3LengthSqr(lado) < 1e-12f) {
        return;
    }
    lado = Vector3Scale(Vector3Normalize(lado), largura);
    Vector3 a = Vector3Add(cabeca, lado), b = Vector3Subtract(cabeca, lado);
    Vector3 c = Vector3Subtract(rabo, Vector3Scale(lado, 0.2f)), d = Vector3Add(rabo, Vector3Scale(lado, 0.2f));
    rlBegin(RL_TRIANGLES);
    rlColor4ub(corDaCabeca.r, corDaCabeca.g, corDaCabeca.b, corDaCabeca.a);
    rlVertex3f(a.x, a.y, a.z);
    rlVertex3f(b.x, b.y, b.z);
    rlColor4ub(corDoRabo.r, corDoRabo.g, corDoRabo.b, corDoRabo.a);
    rlVertex3f(c.x, c.y, c.z);
    rlColor4ub(corDaCabeca.r, corDaCabeca.g, corDaCabeca.b, corDaCabeca.a);
    rlVertex3f(a.x, a.y, a.z);
    rlColor4ub(corDoRabo.r, corDoRabo.g, corDoRabo.b, corDoRabo.a);
    rlVertex3f(c.x, c.y, c.z);
    rlVertex3f(d.x, d.y, d.z);
    rlEnd();
}

/*
 * brilhoMacio
 * Um brilho macio (losango que some nas pontas) com o centro na cor cheia.
 */
static void brilhoMacio(Vector3 centro, Vector3 eixoA, Vector3 eixoB, Color cor) {
    Vector3 pontas[4] = { Vector3Add(centro, eixoA), Vector3Add(centro, eixoB), Vector3Subtract(centro, eixoA), Vector3Subtract(centro, eixoB) };
    for (int k = 0; k < 4; k++) {
        Vector3 p0 = pontas[k], p1 = pontas[(k + 1) % 4];
        rlBegin(RL_TRIANGLES);
        rlColor4ub(cor.r, cor.g, cor.b, cor.a);
        rlVertex3f(centro.x, centro.y, centro.z);
        rlColor4ub(cor.r, cor.g, cor.b, 0);
        rlVertex3f(p0.x, p0.y, p0.z);
        rlVertex3f(p1.x, p1.y, p1.z);
        rlEnd();
    }
}

/*
 * desenharFogos
 * Desenha os foguetes, as brasas (riscos com rabo, halo, miolo branco e a cabeça brilhando) e os clarões
 * (disco, halo e anel), somando luz (mistura aditiva). A largura dos riscos cresce com a distância, para
 * os fogos longe não sumirem. Chamar dentro do BeginMode3D.
 * Recebe: a câmera.
 * Devolve: nada.
 */
void desenharFogos(Camera3D camera) {
    if (totalDeBrasas == 0 && totalDeFoguetes == 0 && totalDeFulgores == 0) {
        return;
    }
    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    float agora = (float) GetTime();
    Vector3 frente = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    Vector3 direita = Vector3Normalize(Vector3CrossProduct(frente, camera.up));
    Vector3 cima = Vector3CrossProduct(direita, frente);

    /* Duas camadas: primeiro o corpo das brasas com mistura normal (a cor aparece até sobre o céu claro do dia), depois
     * os halos, as cabeças brilhando e os clarões somando luz (mistura aditiva) */
    for (int fase = 0; fase < 2; fase++) {
        if (fase == 0) {
            BeginBlendMode(BLEND_ALPHA);
        } else {
            rlDrawRenderBatchActive();
            EndBlendMode();
            BeginBlendMode(BLEND_ADDITIVE);
        }
        for (int i = 0; i < totalDeFoguetes; i++) {
            const Foguete *f = &foguetes[i];
            float distancia = Vector3Distance(camera.position, f->posicao);
            float largura = fmaxf(0.22f, distancia * 0.0042f);
            Vector3 rabo = Vector3Subtract(f->posicao, Vector3Scale(f->velocidade, 0.09f));
            if (fase == 0) {
                riscoComRabo(camera, f->posicao, rabo, largura, (Color){ 255, 235, 190, 255 }, (Color){ 255, 150, 60, 0 });
            } else {
                riscoComRabo(camera, f->posicao, rabo, largura * 3.0f, (Color){ 255, 150, 60, 70 }, (Color){ 255, 120, 40, 0 });
                float r = largura * 3.4f;
                brilhoMacio(f->posicao, Vector3Scale(direita, r), Vector3Scale(cima, r), (Color){ 255, 225, 160, 190 });
            }
        }

        for (int i = 0; i < totalDeBrasas; i++) {
            const Brasa *b = &brasas[i];
            float passou = 1.0f - b->vida / b->duracao;
            float intensidade = Clamp((1.0f - passou) * 1.7f, 0.0f, 1.0f);
            if (b->cintila && passou > 0.5f) {
                intensidade *= 0.5f + 0.5f * sinf(agora * 38.0f + b->semente);
            }
            Color cor = ColorLerp((Color){ 255, 250, 235, 255 }, b->cor, Clamp(passou * 9.0f, 0.0f, 1.0f));
            if (passou > 0.7f) {   /* apagando, a brasa esfria para o vermelho */
                cor = ColorLerp(cor, (Color){ 200, 50, 20, 255 }, (passou - 0.7f) / 0.3f * 0.6f);
            }
            cor.a = (unsigned char) (255.0f * intensidade);
            float distancia = Vector3Distance(camera.position, b->posicao);
            float velocidade = Vector3Length(b->velocidade);
            float comprimento = fmaxf(velocidade * b->cauda, distancia * 0.0070f);
            Vector3 direcao = (velocidade > 1e-3f) ? Vector3Scale(b->velocidade, 1.0f / velocidade) : (Vector3){ 0.0f, -1.0f, 0.0f };
            Vector3 rabo = Vector3Subtract(b->posicao, Vector3Scale(direcao, comprimento));
            float largura = fmaxf(0.10f, distancia * 0.0046f) * (1.0f - 0.35f * passou);

            if (fase == 0) {   /* o corpo, na cor da brasa, com o rabo sumindo, e o miolo branco e fino */
                riscoComRabo(camera, b->posicao, rabo, largura * 1.1f, cor, (Color){ cor.r, (unsigned char) (cor.g * 0.7f), (unsigned char) (cor.b * 0.5f), 0 });
                if (passou < 0.6f) {
                    riscoComRabo(camera, b->posicao, Vector3Lerp(b->posicao, rabo, 0.6f), largura * 0.36f,
                                 (Color){ 255, 252, 240, (unsigned char) (cor.a * (1.0f - passou / 0.6f)) }, (Color){ 255, 250, 230, 0 });
                }
            } else {           /* o halo colorido, largo e fraco, e a cabeça brilhando */
                Color halo = { b->cor.r, b->cor.g, b->cor.b, (unsigned char) (cor.a * 0.13f) };
                riscoComRabo(camera, b->posicao, rabo, largura * 2.8f, halo, (Color){ halo.r, halo.g, halo.b, 0 });
                if (passou < 0.3f && distancia > 25.0f) {
                    float r = largura * 2.4f;
                    brilhoMacio(b->posicao, Vector3Scale(direita, r), Vector3Scale(cima, r), (Color){ cor.r, cor.g, cor.b, (unsigned char) (cor.a * 0.28f) });
                }
            }
        }
    }

    for (int i = 0; i < totalDeFulgores; i++) {
        const Fulgor *f = &fulgores[i];
        float resta = f->vida / f->duracao;
        float aberto = 1.0f - resta;
        if (f->anel) {
            /* o anel de luz que se abre: uma coroa fina, mais forte no meio e sumindo nas bordas */
            float raio = f->tamanho * (0.25f + 0.75f * sqrtf(aberto)), espessura = raio * 0.07f;
            Color meio = f->cor;
            meio.a = (unsigned char) (150.0f * f->forca * resta);
            const int lados = 36;
            for (int k = 0; k < lados; k++) {
                float a0 = 2.0f * PI * (float) k / lados, a1 = 2.0f * PI * (float) (k + 1) / lados;
                Vector3 d0 = Vector3Add(Vector3Scale(direita, cosf(a0)), Vector3Scale(cima, sinf(a0)));
                Vector3 d1 = Vector3Add(Vector3Scale(direita, cosf(a1)), Vector3Scale(cima, sinf(a1)));
                Vector3 i0 = Vector3Add(f->posicao, Vector3Scale(d0, raio - espessura)), i1 = Vector3Add(f->posicao, Vector3Scale(d1, raio - espessura));
                Vector3 m0 = Vector3Add(f->posicao, Vector3Scale(d0, raio)), m1 = Vector3Add(f->posicao, Vector3Scale(d1, raio));
                Vector3 o0 = Vector3Add(f->posicao, Vector3Scale(d0, raio + espessura)), o1 = Vector3Add(f->posicao, Vector3Scale(d1, raio + espessura));
                rlBegin(RL_TRIANGLES);
                rlColor4ub(meio.r, meio.g, meio.b, 0);
                rlVertex3f(i0.x, i0.y, i0.z);
                rlColor4ub(meio.r, meio.g, meio.b, meio.a);
                rlVertex3f(m0.x, m0.y, m0.z);
                rlVertex3f(m1.x, m1.y, m1.z);
                rlColor4ub(meio.r, meio.g, meio.b, 0);
                rlVertex3f(i0.x, i0.y, i0.z);
                rlVertex3f(i1.x, i1.y, i1.z);
                rlColor4ub(meio.r, meio.g, meio.b, meio.a);
                rlVertex3f(m1.x, m1.y, m1.z);
                rlColor4ub(meio.r, meio.g, meio.b, meio.a);
                rlVertex3f(m0.x, m0.y, m0.z);
                rlColor4ub(meio.r, meio.g, meio.b, 0);
                rlVertex3f(o0.x, o0.y, o0.z);
                rlVertex3f(o1.x, o1.y, o1.z);
                rlColor4ub(meio.r, meio.g, meio.b, meio.a);
                rlVertex3f(m0.x, m0.y, m0.z);
                rlColor4ub(meio.r, meio.g, meio.b, 0);
                rlVertex3f(o1.x, o1.y, o1.z);
                rlColor4ub(meio.r, meio.g, meio.b, meio.a);
                rlVertex3f(m1.x, m1.y, m1.z);
                rlEnd();
            }
            continue;
        }
        float raio = f->tamanho * (0.35f + 0.65f * aberto);
        Color centro = ColorLerp(WHITE, f->cor, 0.45f);
        centro.a = (unsigned char) (210.0f * f->forca * resta * resta);
        Color borda = f->cor;
        borda.a = 0;
        const int lados = 18;
        for (int k = 0; k < lados; k++) {
            float a0 = 2.0f * PI * (float) k / lados, a1 = 2.0f * PI * (float) (k + 1) / lados;
            Vector3 p0 = Vector3Add(f->posicao, Vector3Add(Vector3Scale(direita, cosf(a0) * raio), Vector3Scale(cima, sinf(a0) * raio)));
            Vector3 p1 = Vector3Add(f->posicao, Vector3Add(Vector3Scale(direita, cosf(a1) * raio), Vector3Scale(cima, sinf(a1) * raio)));
            rlBegin(RL_TRIANGLES);
            rlColor4ub(centro.r, centro.g, centro.b, centro.a);
            rlVertex3f(f->posicao.x, f->posicao.y, f->posicao.z);
            rlColor4ub(borda.r, borda.g, borda.b, borda.a);
            rlVertex3f(p0.x, p0.y, p0.z);
            rlVertex3f(p1.x, p1.y, p1.z);
            rlEnd();
        }
    }

    rlDrawRenderBatchActive();
    rlEnableDepthMask();
    EndBlendMode();
}
