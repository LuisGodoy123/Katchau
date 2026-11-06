/*
 * faiscas.c
 * ------------------------------------------------------------------
 * As faíscas das batidas. Veja faiscas.h.
 * ------------------------------------------------------------------
 */

#include <math.h>
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "faiscas.h"

#define MAXIMO_DE_FAISCAS      900
#define MAXIMO_DE_CLAROES      24
#define GRAVIDADE              9.8f    /* m/s² */
#define QUIQUE                 0.35f   /* quanto da velocidade vertical sobra quando a faísca bate no chão */
#define ATRITO_NO_CHAO         0.55f   /* e da velocidade de lado */
#define ALTURA_DO_CHAO         0.38f   /* metros abaixo do ponto do contato: o chão */
#define RAJADA_MINIMA          14      /* faíscas no começo de uma batida fraca... */
#define RAJADA_FORTE           70      /* ...e a mais numa batida forte */
#define CHUVA_POR_FORCA        260.0f  /* faíscas por segundo enquanto raspa, pela força do contato... */
#define CHUVA_POR_DESLIZE      9.0f    /* ...e por metro/s de lata escorregando */
#define DESLIZE_PARA_FAISCAR   1.2f    /* m/s: abaixo disso as latas encostam sem faiscar */
#define VIDA_MINIMA            0.22f   /* segundos */
#define VIDA_MAXIMA            0.65f
#define LARGURA_DO_RISCO       0.028f  /* metros */
#define COMPRIMENTO_DO_RISCO   0.035f  /* segundos de caminho que o risco mostra (o rastro) */
#define PARTE_DA_VELOCIDADE    0.55f   /* a faísca sai com parte da velocidade do carro */
#define VIDA_DO_CLARAO         0.09f
#define FAISCAS_ENTRE_CARROS   0.4f    /* batida de carro com carro: só essa parte das faíscas do muro
                                        * (os dois carros da batida soltam as suas, então já somam) */
#define TAMANHO_DO_CLARAO      0.75f   /* metros, numa batida forte */

typedef struct {
    Vector3 posicao;
    Vector3 velocidade;
    float vida;          /* segundos que faltam */
    float duracao;       /* a vida que ela tinha ao nascer */
    float chao;          /* a altura do chão onde ela nasceu */
    float calor;         /* 0 a 1: as mais quentes são mais brancas e mais largas */
} Faisca;

typedef struct {
    Vector3 posicao;
    float vida;
    float tamanho;
} Clarao;

static Faisca faiscas[MAXIMO_DE_FAISCAS];
static int totalDeFaiscas = 0;
static Clarao claroes[MAXIMO_DE_CLAROES];
static int totalDeClaroes = 0;
static int tinhaContato[TOTAL_CARROS];   /* por número do carro: o contato já vinha do quadro anterior */
static float sobraDaChuva[TOTAL_CARROS]; /* frações de faísca que ficam para o próximo quadro */

static float aleatorio(float de, float ate) {
    return de + (ate - de) * (float) GetRandomValue(0, 10000) / 10000.0f;
}

/*
 * limparFaiscas
 * Apaga todas as faíscas (corrida nova).
 * Recebe: nada.
 * Devolve: nada.
 */
void limparFaiscas(void) {
    totalDeFaiscas = 0;
    totalDeClaroes = 0;
    for (int i = 0; i < TOTAL_CARROS; i++) {
        tinhaContato[i] = 0;
        sobraDaChuva[i] = 0.0f;
    }
}

/*
 * velocidadeNoMundo
 * Recebe: o carro.
 * Devolve: a velocidade dele no mundo, em metros por segundo (para onde a pista aponta ali).
 */
static Vector3 velocidadeNoMundo(const Carro *carro) {
    PosicaoNaPista adiante = carro->posicao;
    moverNaPista(&adiante, UNIDADES_POR_METRO);
    PontoNoMundo aqui = pontoNoMundo(carro->posicao, carro->deslocamentoLateral);
    PontoNoMundo la = pontoNoMundo(adiante, carro->deslocamentoLateral);
    Vector3 frente = Vector3Normalize((Vector3){ la.x - aqui.x, 0.0f, la.z - aqui.z });
    return Vector3Scale(frente, carro->velocidade / UNIDADES_POR_METRO);
}

/*
 * soltarFaisca
 * Uma faísca nova: sai do ponto do contato com parte da velocidade do carro e um espirro para
 * cima e para os lados. Com o limite cheio, ela toma o lugar da mais velha.
 * Recebe: o ponto, a velocidade do carro e a força do contato (0 a 1).
 * Devolve: nada.
 */
static void soltarFaisca(Vector3 ponto, Vector3 velocidadeDoCarro, float forca) {
    Faisca *f;
    if (totalDeFaiscas < MAXIMO_DE_FAISCAS) {
        f = &faiscas[totalDeFaiscas++];
    } else {
        int maisVelha = 0;
        for (int i = 1; i < MAXIMO_DE_FAISCAS; i++) {
            if (faiscas[i].vida < faiscas[maisVelha].vida) {
                maisVelha = i;
            }
        }
        f = &faiscas[maisVelha];
    }
    float angulo = aleatorio(0.0f, 2.0f * PI);
    float espirro = aleatorio(2.0f, 6.5f) * (0.6f + 0.8f * forca);
    f->velocidade = (Vector3){
        velocidadeDoCarro.x * PARTE_DA_VELOCIDADE + cosf(angulo) * espirro,
        aleatorio(1.0f, 4.5f) * (0.6f + 0.6f * forca),
        velocidadeDoCarro.z * PARTE_DA_VELOCIDADE + sinf(angulo) * espirro,
    };
    f->posicao = (Vector3){ ponto.x + aleatorio(-0.15f, 0.15f), ponto.y + aleatorio(-0.1f, 0.15f),
                            ponto.z + aleatorio(-0.15f, 0.15f) };
    f->duracao = aleatorio(VIDA_MINIMA, VIDA_MAXIMA) * (0.8f + 0.5f * forca);
    f->vida = f->duracao;
    f->chao = ponto.y - ALTURA_DO_CHAO;
    f->calor = aleatorio(0.4f, 1.0f);
}

static void soltarClarao(Vector3 ponto, float forca) {
    if (totalDeClaroes >= MAXIMO_DE_CLAROES) {
        return;
    }
    claroes[totalDeClaroes++] = (Clarao){ ponto, VIDA_DO_CLARAO, TAMANHO_DO_CLARAO * (0.4f + 0.6f * forca) };
}

/*
 * soltarFaiscasDaCorrida
 * Olha o contato de cada carro neste quadro: no começo de uma batida, solta a rajada e o clarão;
 * enquanto as latas se esfregam, solta a chuva de faíscas, mais forte quanto mais escorregam.
 * Recebe: a corrida e o tempo do quadro (0 na pausa: nada nasce).
 * Devolve: nada.
 */
void soltarFaiscasDaCorrida(Corrida *corrida, float tempoQuadro) {
    if (tempoQuadro <= 0.0f) {
        return;
    }
    for (int i = -1; i < TOTAL_RIVAIS; i++) {
        Carro *carro = (i < 0) ? &corrida->jogador : &corrida->rivais[i];
        int numero = carro->numero;
        if (numero < 0 || numero >= TOTAL_CARROS) {
            continue;
        }
        int temContato = carro->contato > 0.0f;
        if (!temContato) {
            tinhaContato[numero] = 0;
            sobraDaChuva[numero] = 0.0f;
            continue;
        }
        Vector3 ponto = { carro->contatoX, carro->contatoY, carro->contatoZ };
        Vector3 velocidade = velocidadeNoMundo(carro);
        float forca = Clamp(carro->contato, 0.0f, 1.0f);
        float quantidade = carro->contatoComCarro ? FAISCAS_ENTRE_CARROS : 1.0f;

        if (!tinhaContato[numero]) {
            /* A batida: rajada e clarão (o outro carro da batida faz a dele também: fica mais cheia) */
            int rajada = (int) ((RAJADA_MINIMA + RAJADA_FORTE * forca * forca) * quantidade);
            for (int k = 0; k < rajada; k++) {
                soltarFaisca(ponto, velocidade, forca);
            }
            soltarClarao(ponto, forca);
        }
        tinhaContato[numero] = 1;

        /* A raspada: chuva de faíscas enquanto a lata escorrega */
        if (carro->contatoDeslize > DESLIZE_PARA_FAISCAR) {
            float porSegundo = (CHUVA_POR_FORCA * forca + CHUVA_POR_DESLIZE * carro->contatoDeslize) * quantidade;
            sobraDaChuva[numero] += porSegundo * tempoQuadro;
            while (sobraDaChuva[numero] >= 1.0f) {
                soltarFaisca(ponto, velocidade, forca * 0.7f);
                sobraDaChuva[numero] -= 1.0f;
            }
        }
    }
}

/*
 * atualizarFaiscas
 * As faíscas voam, caem, quicam no chão e apagam; os clarões somem.
 * Recebe: o tempo do quadro (0 na pausa: tudo para no ar).
 * Devolve: nada.
 */
void atualizarFaiscas(float tempoQuadro) {
    if (tempoQuadro <= 0.0f) {
        return;
    }
    for (int i = 0; i < totalDeFaiscas; i++) {
        Faisca *f = &faiscas[i];
        f->vida -= tempoQuadro;
        if (f->vida <= 0.0f) {
            faiscas[i--] = faiscas[--totalDeFaiscas];
            continue;
        }
        f->velocidade.y -= GRAVIDADE * tempoQuadro;
        f->posicao = Vector3Add(f->posicao, Vector3Scale(f->velocidade, tempoQuadro));
        if (f->posicao.y < f->chao) {
            f->posicao.y = f->chao;
            f->velocidade.y = -f->velocidade.y * QUIQUE;
            f->velocidade.x *= ATRITO_NO_CHAO;
            f->velocidade.z *= ATRITO_NO_CHAO;
        }
    }
    for (int i = 0; i < totalDeClaroes; i++) {
        claroes[i].vida -= tempoQuadro;
        if (claroes[i].vida <= 0.0f) {
            claroes[i--] = claroes[--totalDeClaroes];
        }
    }
}

/*
 * corDaFaisca
 * Recebe: quanto da vida já passou (0 nascendo, 1 apagando) e o calor dela.
 * Devolve: a cor: branco-amarelo quente, depois laranja, depois vermelho apagando.
 */
static Color corDaFaisca(float passou, float calor) {
    float brilho = 1.0f - passou;
    Color quente = { 255, (unsigned char) (235 + 20 * calor), (unsigned char) (150 + 90 * calor), 255 };
    Color laranja = { 255, 140, 30, 255 };
    Color vermelho = { 200, 40, 10, 255 };
    Color cor = (passou < 0.45f) ? ColorLerp(quente, laranja, passou / 0.45f)
                                 : ColorLerp(laranja, vermelho, (passou - 0.45f) / 0.55f);
    cor.a = (unsigned char) (255.0f * Clamp(brilho * 1.4f, 0.0f, 1.0f));
    return cor;
}

/*
 * desenharFaiscas
 * Desenha as faíscas como riscos virados para a câmera e os clarões como um brilho redondo,
 * somando luz (mistura aditiva). Chamar dentro do BeginMode3D.
 * Recebe: a câmera.
 * Devolve: nada.
 */
void desenharFaiscas(Camera3D camera) {
    if (totalDeFaiscas == 0 && totalDeClaroes == 0) {
        return;
    }
    rlDrawRenderBatchActive();
    BeginBlendMode(BLEND_ADDITIVE);
    rlDisableDepthMask();

    for (int i = 0; i < totalDeFaiscas; i++) {
        const Faisca *f = &faiscas[i];
        float passou = 1.0f - f->vida / f->duracao;
        Color cor = corDaFaisca(passou, f->calor);
        Vector3 rabo = Vector3Subtract(f->posicao, Vector3Scale(f->velocidade, COMPRIMENTO_DO_RISCO));
        Vector3 direcao = Vector3Subtract(f->posicao, rabo);
        if (Vector3LengthSqr(direcao) < 1e-6f) {
            direcao = (Vector3){ 0.0f, 0.02f, 0.0f };
            rabo = Vector3Subtract(f->posicao, direcao);
        }
        Vector3 paraACamera = Vector3Subtract(camera.position, f->posicao);
        Vector3 lado = Vector3Normalize(Vector3CrossProduct(direcao, paraACamera));
        float largura = LARGURA_DO_RISCO * (0.7f + 0.8f * f->calor) * (1.0f - 0.5f * passou);
        lado = Vector3Scale(lado, largura);
        Vector3 a = Vector3Add(f->posicao, lado), b = Vector3Subtract(f->posicao, lado);
        Vector3 c = Vector3Subtract(rabo, lado), d = Vector3Add(rabo, lado);
        DrawTriangle3D(a, b, c, cor);
        DrawTriangle3D(a, c, d, cor);
    }

    /* Os clarões: um disco virado para a câmera, que some em menos de um décimo de segundo */
    Vector3 frente = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    Vector3 direita = Vector3Normalize(Vector3CrossProduct(frente, camera.up));
    Vector3 cima = Vector3CrossProduct(direita, frente);
    for (int i = 0; i < totalDeClaroes; i++) {
        const Clarao *c = &claroes[i];
        float resta = c->vida / VIDA_DO_CLARAO;
        float raio = c->tamanho * (0.6f + 0.4f * (1.0f - resta));
        Color centro = { 255, 240, 200, (unsigned char) (230 * resta) };
        Color borda = { 255, 150, 40, 0 };
        const int lados = 14;
        for (int k = 0; k < lados; k++) {
            float a0 = 2.0f * PI * k / lados, a1 = 2.0f * PI * (k + 1) / lados;
            Vector3 p0 = Vector3Add(c->posicao, Vector3Add(Vector3Scale(direita, cosf(a0) * raio), Vector3Scale(cima, sinf(a0) * raio)));
            Vector3 p1 = Vector3Add(c->posicao, Vector3Add(Vector3Scale(direita, cosf(a1) * raio), Vector3Scale(cima, sinf(a1) * raio)));
            rlBegin(RL_TRIANGLES);
            rlColor4ub(centro.r, centro.g, centro.b, centro.a);
            rlVertex3f(c->posicao.x, c->posicao.y, c->posicao.z);
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
