/*
 * fogos.c
 * ------------------------------------------------------------------
 * Os fogos de artifício da vitória. Veja fogos.h.
 * ------------------------------------------------------------------
 */

#include <math.h>
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "fogos.h"
#include "efeitos.h"

#define MAXIMO_DE_BRASAS       4200
#define MAXIMO_DE_FOGUETES     28
#define MAXIMO_DE_FULGORES     28
#define GRAVIDADE              9.8f
#define GRAVIDADE_DO_FOGUETE   4.9f    /* o foguete sobe "mais leve" que uma pedra: o motor ainda empurra */
#define ALTURA_MINIMA          28.0f   /* metros acima do chão onde um fogo estoura... */
#define ALTURA_MAXIMA          58.0f
#define DISTANCIA_MINIMA       50.0f   /* ...e a que distância da câmera */
#define DISTANCIA_MAXIMA       170.0f
#define PAUSA_MINIMA           0.11f   /* segundos entre um foguete e o próximo */
#define PAUSA_MAXIMA           0.34f
#define FOGUETES_DA_ABERTURA   6       /* a salva do primeiro instante da festa */
#define FOGOS_NO_CORTE         2       /* estouram na hora quando a câmera corta para outro lugar */
#define ALCANCE_DO_SOM         330.0f  /* metros: além disso o estrondo some */

typedef struct {
    Vector3 posicao;
    Vector3 velocidade;
    float vida;          /* segundos que faltam */
    float duracao;
    float arrasto;       /* por segundo: o ar freia a brasa */
    float peso;          /* quanto da gravidade a brasa sente */
    float cauda;         /* segundos de caminho que o risco mostra */
    float semente;       /* fase do cintilar */
    int cintila;         /* 1: no fim da vida a brasa pisca */
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
    Color cor;
} Fulgor;

enum { FOGO_PEONIA, FOGO_SALGUEIRO, FOGO_ANEL, TOTAL_TIPOS_DE_FOGO };

static const Color PALETA[] = {
    { 255, 70, 60, 255 },    /* vermelho */
    { 255, 190, 60, 255 },   /* dourado */
    { 90, 255, 130, 255 },   /* verde */
    { 80, 150, 255, 255 },   /* azul */
    { 255, 90, 225, 255 },   /* magenta */
    { 255, 255, 235, 255 },  /* branco */
    { 90, 245, 255, 255 },   /* ciano */
    { 255, 130, 40, 255 },   /* laranja */
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
                        int cintila, Color cor) {
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
    b->cintila = cintila;
    b->cor = cor;
}

static void soltarFulgor(Vector3 posicao, float tamanho, float vida, Color cor) {
    if (totalDeFulgores >= MAXIMO_DE_FULGORES) {
        return;
    }
    fulgores[totalDeFulgores++] = (Fulgor){ posicao, vida, vida, tamanho, cor };
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

/*
 * estourar
 * O fogo abre no céu: um clarão colorido e as brasas saindo em esfera (peônia), em chuva dourada
 * (salgueiro) ou em círculo (anel).
 * Recebe: onde, o tipo e as duas cores.
 * Devolve: nada.
 */
static void estourar(Vector3 onde, int tipo, Color cor, Color cor2) {
    soltarFulgor(onde, 26.0f, 0.38f, (tipo == FOGO_SALGUEIRO) ? PALETA[1] : cor);
    tocarEstrondo(onde, 1);

    if (tipo == FOGO_PEONIA) {
        for (int k = 0; k < 170; k++) {
            Vector3 d = direcaoNaEsfera();
            float v = aleatorio(30.0f, 46.0f);
            soltarBrasa(onde, Vector3Scale(d, v), aleatorio(1.7f, 2.5f), 1.7f, 0.42f, 0.045f, 1, (k % 4 == 0) ? cor2 : cor);
        }
    } else if (tipo == FOGO_SALGUEIRO) {
        for (int k = 0; k < 125; k++) {
            Vector3 d = direcaoNaEsfera();
            float v = aleatorio(18.0f, 31.0f);
            soltarBrasa(onde, Vector3Scale(d, v), aleatorio(2.8f, 3.8f), 1.5f, 0.30f, 0.16f, 0, PALETA[1]);
        }
    } else {
        Vector3 n = direcaoNaEsfera();
        Vector3 u = Vector3Normalize(Vector3CrossProduct(n, (fabsf(n.y) < 0.9f) ? (Vector3){ 0.0f, 1.0f, 0.0f } : (Vector3){ 1.0f, 0.0f, 0.0f }));
        Vector3 w = Vector3CrossProduct(n, u);
        for (int k = 0; k < 76; k++) {
            float a = 2.0f * PI * (float) k / 76.0f;
            Vector3 d = Vector3Add(Vector3Scale(u, cosf(a)), Vector3Scale(w, sinf(a)));
            soltarBrasa(onde, Vector3Scale(d, aleatorio(36.0f, 42.0f)), aleatorio(1.7f, 2.3f), 1.6f, 0.40f, 0.05f, 1, cor);
        }
        for (int k = 0; k < 30; k++) {   /* o miolo do anel */
            soltarBrasa(onde, Vector3Scale(direcaoNaEsfera(), aleatorio(6.0f, 18.0f)), aleatorio(1.2f, 1.8f), 1.8f, 0.5f, 0.04f, 1, cor2);
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
    int tipo = GetRandomValue(0, 9);
    tipo = (tipo < 5) ? FOGO_PEONIA : ((tipo < 8) ? FOGO_SALGUEIRO : FOGO_ANEL);
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
 * Os foguetes sobem (e soltam o rastro) e estouram; as brasas voam, o ar as freia, caem e apagam;
 * os clarões somem.
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
        f->sobraDoRastro += 130.0f * tempoQuadro;
        while (f->sobraDoRastro >= 1.0f) {
            f->sobraDoRastro -= 1.0f;
            Vector3 solta = Vector3Add(Vector3Scale(direcaoNaEsfera(), aleatorio(0.4f, 2.2f)), (Vector3){ 0.0f, -2.5f, 0.0f });
            soltarBrasa(f->posicao, solta, aleatorio(0.5f, 0.95f), 0.6f, 0.5f, 0.05f, 0, (Color){ 255, 175, 70, 255 });
        }
        f->tempoAteEstourar -= tempoQuadro;
        if (f->tempoAteEstourar <= 0.0f) {
            estourar(f->posicao, f->tipo, f->cor, f->cor2);
            foguetes[i--] = foguetes[--totalDeFoguetes];
        }
    }
    for (int i = 0; i < totalDeBrasas; i++) {
        Brasa *b = &brasas[i];
        b->vida -= tempoQuadro;
        if (b->vida <= 0.0f) {
            brasas[i--] = brasas[--totalDeBrasas];
            continue;
        }
        b->velocidade = Vector3Scale(b->velocidade, expf(-b->arrasto * tempoQuadro));
        b->velocidade.y -= GRAVIDADE * b->peso * tempoQuadro;
        b->posicao = Vector3Add(b->posicao, Vector3Scale(b->velocidade, tempoQuadro));
    }
    for (int i = 0; i < totalDeFulgores; i++) {
        fulgores[i].vida -= tempoQuadro;
        if (fulgores[i].vida <= 0.0f) {
            fulgores[i--] = fulgores[--totalDeFulgores];
        }
    }
}

/*
 * riscoVirado
 * Um risco brilhante virado para a câmera, de "rabo" até "ponta".
 */
static void riscoVirado(Camera3D camera, Vector3 ponta, Vector3 rabo, float largura, Color cor) {
    Vector3 direcao = Vector3Subtract(ponta, rabo);
    if (Vector3LengthSqr(direcao) < 1e-8f) {
        direcao = (Vector3){ 0.0f, 0.01f, 0.0f };
        rabo = Vector3Subtract(ponta, direcao);
    }
    Vector3 lado = Vector3CrossProduct(direcao, Vector3Subtract(camera.position, ponta));
    if (Vector3LengthSqr(lado) < 1e-10f) {
        return;
    }
    lado = Vector3Scale(Vector3Normalize(lado), largura);
    Vector3 a = Vector3Add(ponta, lado), b = Vector3Subtract(ponta, lado);
    Vector3 c = Vector3Subtract(rabo, lado), d = Vector3Add(rabo, lado);
    DrawTriangle3D(a, b, c, cor);
    DrawTriangle3D(a, c, d, cor);
}

/*
 * desenharFogos
 * Desenha os foguetes, as brasas (como riscos) e os clarões (como discos), somando luz (mistura
 * aditiva). A largura dos riscos cresce com a distância, para os fogos longe não sumirem.
 * Chamar dentro do BeginMode3D.
 * Recebe: a câmera.
 * Devolve: nada.
 */
void desenharFogos(Camera3D camera) {
    if (totalDeBrasas == 0 && totalDeFoguetes == 0 && totalDeFulgores == 0) {
        return;
    }
    rlDrawRenderBatchActive();
    BeginBlendMode(BLEND_ADDITIVE);
    rlDisableDepthMask();
    float agora = (float) GetTime();

    for (int i = 0; i < totalDeFoguetes; i++) {
        const Foguete *f = &foguetes[i];
        float distancia = Vector3Distance(camera.position, f->posicao);
        Vector3 rabo = Vector3Subtract(f->posicao, Vector3Scale(f->velocidade, 0.075f));
        riscoVirado(camera, f->posicao, rabo, fmaxf(0.22f, distancia * 0.0042f), (Color){ 255, 235, 190, 255 });
    }

    for (int i = 0; i < totalDeBrasas; i++) {
        const Brasa *b = &brasas[i];
        float passou = 1.0f - b->vida / b->duracao;
        float brilho = Clamp((1.0f - passou) * 1.7f, 0.0f, 1.0f);
        if (b->cintila && passou > 0.55f) {
            brilho *= 0.55f + 0.45f * sinf(agora * 38.0f + b->semente);
        }
        Color cor = ColorLerp((Color){ 255, 250, 235, 255 }, b->cor, Clamp(passou * 9.0f, 0.0f, 1.0f));
        if (passou > 0.7f) {   /* apagando, a brasa esfria para o vermelho */
            cor = ColorLerp(cor, (Color){ 200, 50, 20, 255 }, (passou - 0.7f) / 0.3f * 0.6f);
        }
        cor.a = (unsigned char) (255.0f * brilho);
        float distancia = Vector3Distance(camera.position, b->posicao);
        float velocidade = Vector3Length(b->velocidade);
        float comprimento = fmaxf(velocidade * b->cauda, distancia * 0.0070f);
        Vector3 direcao = (velocidade > 1e-3f) ? Vector3Scale(b->velocidade, 1.0f / velocidade) : (Vector3){ 0.0f, -1.0f, 0.0f };
        Vector3 rabo = Vector3Subtract(b->posicao, Vector3Scale(direcao, comprimento));
        float largura = fmaxf(0.10f, distancia * 0.0048f) * (1.0f - 0.35f * passou);
        riscoVirado(camera, b->posicao, rabo, largura, cor);
    }

    Vector3 frente = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    Vector3 direita = Vector3Normalize(Vector3CrossProduct(frente, camera.up));
    Vector3 cima = Vector3CrossProduct(direita, frente);
    for (int i = 0; i < totalDeFulgores; i++) {
        const Fulgor *f = &fulgores[i];
        float resta = f->vida / f->duracao;
        float raio = f->tamanho * (0.35f + 0.65f * (1.0f - resta));
        Color centro = ColorLerp(WHITE, f->cor, 0.45f);
        centro.a = (unsigned char) (210.0f * resta * resta);
        Color borda = f->cor;
        borda.a = 0;
        const int lados = 16;
        for (int k = 0; k < lados; k++) {
            float a0 = 2.0f * PI * k / lados, a1 = 2.0f * PI * (k + 1) / lados;
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
