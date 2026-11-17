/*
 * palco.c
 * ------------------------------------------------------------------
 * A apresentação do carro na garagem. Veja palco.h.
 *
 * Cada entrada é uma curva no tempo (onde ele está, para onde o bico
 * aponta e o volante), e a física do palco vem dela: a velocidade e as
 * acelerações saem da diferença de um quadro para o outro, e uma mola de
 * suspensão transforma a freada em bico abaixando, a curva em corpo
 * deitando para fora e a aterrissagem em corpo afundando. Parado, quem
 * mexe é o corpo dele (gestos.c), com o jeito de cada um: o locutor
 * anuncia o nome, e ele ouve como quem conversa (acena nas ênfases, pega o
 * ânimo de quem fala), com os olhos na câmera e os tiques dele.
 * ------------------------------------------------------------------
 */

#include <math.h>
#include <string.h>
#include "palco.h"
#include "moradores.h"

#define PI_DO_PALCO          3.14159265f
#define RUMO_DA_POSE         (-2.47f)     /* o bico para a esquerda e para a câmera: 3/4 de frente, uns 30° da câmera */
#define RUMO_PARA_A_ESQUERDA (-1.5708f)   /* andando para a esquerda da tela */
#define RUMO_PARA_A_DIREITA  1.5708f
/* Como no vídeo de referência, mas espelhado: o cartão do carro fica à direita da tela, então ele entra pela
 * esquerda (à vista desde o começo), para virado para a direita (olhando para dentro da tela) e sai pela direita.
 * As curvas abaixo são escritas entrando pela direita; ESPELHO as vira */
#define ESPELHO              (-1.0f)
#define X_DE_ENTRADA         7.5f         /* metros: logo fora da tela */
#define X_DE_SAIDA           12.0f        /* do outro lado, atrás do cartão */
#define RAIO_DA_RODA         0.33f
#define SEGUNDOS_DA_SAIDA    0.6f
#define CORTA_A_ENTRADA      0.35f        /* trocou de carro no começo da entrada: o novo entra direto, sem esperar */
/* A suspensão do palco */
#define FREQUENCIA_DA_MOLA   2.2f         /* Hz */
#define AMORTECIMENTO        0.35f
#define MERGULHO_POR_ACELERACAO 0.006f    /* radianos por m/s²: freando, o bico desce */
#define MAIOR_MERGULHO       0.07f
#define ROLAGEM_POR_CURVA    0.004f       /* radianos por (m/s × rad/s): na curva, o corpo deita para fora */
#define MAIOR_ROLAGEM        0.08f
/* O anúncio do nome */
#define ANUNCIO_DEPOIS_DE    0.15f        /* segundos depois de parar */
#define SEGUNDOS_POR_LETRA   0.09f        /* sem a voz, o anúncio fica o tempo de dizer o nome */
#define MENOR_ANUNCIO        1.2f
#define FICA_DEPOIS_DO_ANUNCIO 1.2f
#define ENFASE_DO_LOCUTOR    0.9f         /* o locutor carrega no nome: o carro acena junto */
/* Para onde ele olha: a câmera fica à direita da frente dele */
/* De que lado do carro fica a câmera (+ a direita dele): com o carro de bico para a direita da tela, ela fica à
 * direita dele. Os olhos olham para ela pela conta do painel dos olhos (olhos.h, olharParaNoProximoDesenho); isto
 * é para o corpo (gestos.c), que vira para o lado de quem fala */
#define LADO_DA_CAMERA       (-ESPELHO)
#define OLHADA_PARA_O_LADO   0.9f      /* parado, de vez em quando: o quanto o olhar sai da câmera e volta */
#define PISCADELA            (1.0f * LADO_DA_CAMERA)   /* o olho que pisca é o que fica de frente para a câmera */
/* O "Katchau" do McQueen: ele vira mais de frente para a câmera e chega perto dela */
#define KATCHAU_DE_FRENTE    0.35f     /* radianos */
#define KATCHAU_CHEGA_PERTO  0.30f     /* metros */

typedef enum { ENTRA_DERRAPANDO, ENTRA_PULANDO, ENTRA_DE_RE, ENTRA_DEVAGAR, ENTRA_SALTITANDO, ENTRA_NA_HIDRAULICA } JeitoDeEntrar;

typedef struct {
    JeitoDeEntrar entrada;
    const char *nome;   /* o que o locutor anuncia */
    const char *voz;    /* recursos/sons/vozes/<voz>.wav */
    float deFrente;     /* radianos a mais de frente para a câmera, na pose (o que tampa os olhos de lado) */
} NoPalco;

/* Na ordem das fichas (corrida.c): o número do carro */
static const NoPalco NO_PALCO[] = {
    { ENTRA_DERRAPANDO, "LIGHTNING McQUEEN", "mcqueen", 0.0f },
    { ENTRA_DEVAGAR, "THE KING", "the_king", 0.0f },
    { ENTRA_DEVAGAR, "DOC HUDSON", "doc_hudson", 0.0f },
    { ENTRA_DERRAPANDO, "FRANCESCO BERNOULLI", "francesco", 0.0f },
    { ENTRA_DEVAGAR, "SALLY", "sally", 0.0f },
    { ENTRA_NA_HIDRAULICA, "RAMONE", "ramone", 0.0f },
    { ENTRA_SALTITANDO, "LUIGI", "luigi", 0.0f },
    { ENTRA_SALTITANDO, "GUIDO", "guido", 0.0f },
    { ENTRA_DE_RE, "MATER", "mater", 0.0f },
    { ENTRA_DEVAGAR, "SARGE", "sarge", 0.0f },
    { ENTRA_DEVAGAR, "FILLMORE", "fillmore", 0.0f },
    { ENTRA_PULANDO, "FINN McMISSILE", "finn_mcmissile", 0.0f },
    { ENTRA_DERRAPANDO, "WINGO", "wingo", 0.0f },
    { ENTRA_DERRAPANDO, "SNOT ROD", "snot_rod", 0.60f },   /* quase de frente: de lado, o compressor tampa os olhos */
    { ENTRA_DERRAPANDO, "JACKSON STORM", "jackson_storm", 0.0f },
    { ENTRA_DERRAPANDO, "BOOST", "boost", 0.0f },
    { ENTRA_SALTITANDO, "DJ", "dj", 0.0f },
};
#define TOTAL_NO_PALCO (int) (sizeof(NO_PALCO) / sizeof(NO_PALCO[0]))

/* O jeito dos carros que não moram na cidade (os outros vêm de moradores.c) */
typedef struct {
    int numero;
    Jeito jeito;
} JeitoDeFora;

static const JeitoDeFora JEITOS_DE_FORA[] = {
    { 1, { .frequencia = 2.2f, .amortecimento = 0.70f, .alegria = 0.35f, .agitacao = 0.25f, .conforto = 0.8f,     /* The King: calmo */
           .empatia = 0.7f, .timidez = 0.0f, .sonolencia = 0.2f, .inquietude = 0.15f, .energia = 0.80f, .ritmo = 1.5f,
           .postura = -0.004f, .altura = 0.0f, .maos = 0.30f, .requebro = 0.20f, .boca = 0.85f, .exclamando = HUMOR_FELIZ,
           .tique = TIQUE_NENHUM } },
    { 3, { .frequencia = 3.6f, .amortecimento = 0.40f, .alegria = 0.45f, .agitacao = 0.60f, .conforto = 0.9f,     /* Francesco: convencido */
           .empatia = 0.3f, .timidez = 0.0f, .sonolencia = 0.1f, .inquietude = 0.5f, .energia = 1.40f, .ritmo = 3.0f,
           .postura = -0.008f, .altura = -0.010f, .maos = 0.80f, .requebro = 0.60f, .boca = 1.1f, .exclamando = HUMOR_EMPOLGADO,
           .tique = TIQUE_NENHUM } },
    { 11, { .frequencia = 2.8f, .amortecimento = 0.70f, .alegria = 0.15f, .agitacao = 0.30f, .conforto = 0.8f,    /* Finn: o espião */
            .empatia = 0.4f, .timidez = 0.0f, .sonolencia = 0.1f, .inquietude = 0.10f, .energia = 0.85f, .ritmo = 1.6f,
            .postura = -0.006f, .altura = 0.0f, .maos = 0.25f, .requebro = 0.10f, .boca = 0.8f, .exclamando = HUMOR_SERIO,
            .tique = TIQUE_NENHUM } },
    { 12, { .frequencia = 3.8f, .amortecimento = 0.35f, .alegria = 0.35f, .agitacao = 0.70f, .conforto = 0.6f,    /* Wingo */
            .empatia = 0.5f, .timidez = 0.0f, .sonolencia = 0.1f, .inquietude = 0.70f, .energia = 1.40f, .ritmo = 3.0f,
            .postura = -0.004f, .altura = -0.015f, .maos = 0.70f, .requebro = 0.50f, .boca = 1.1f, .exclamando = HUMOR_EMPOLGADO,
            .tique = TIQUE_ACELERA } },
    { 13, { .frequencia = 2.6f, .amortecimento = 0.40f, .alegria = 0.30f, .agitacao = 0.75f, .conforto = 0.5f,    /* Snot Rod */
            .empatia = 0.5f, .timidez = 0.0f, .sonolencia = 0.1f, .inquietude = 0.60f, .energia = 1.50f, .ritmo = 2.4f,
            .postura = 0.004f, .altura = 0.0f, .maos = 0.60f, .requebro = 0.40f, .boca = 1.2f, .exclamando = HUMOR_EMPOLGADO,
            .tique = TIQUE_ACELERA } },
    { 14, { .frequencia = 3.2f, .amortecimento = 0.80f, .alegria = 0.00f, .agitacao = 0.20f, .conforto = 0.9f,    /* Storm: frio */
            .empatia = 0.2f, .timidez = 0.0f, .sonolencia = 0.1f, .inquietude = 0.10f, .energia = 0.70f, .ritmo = 1.4f,
            .postura = -0.010f, .altura = -0.010f, .maos = 0.15f, .requebro = 0.10f, .boca = 0.75f, .exclamando = HUMOR_SERIO,
            .tique = TIQUE_NENHUM } },
    { 15, { .frequencia = 3.8f, .amortecimento = 0.35f, .alegria = 0.30f, .agitacao = 0.80f, .conforto = 0.6f,    /* Boost */
            .empatia = 0.4f, .timidez = 0.0f, .sonolencia = 0.1f, .inquietude = 0.70f, .energia = 1.50f, .ritmo = 3.0f,
            .postura = -0.004f, .altura = -0.015f, .maos = 0.70f, .requebro = 0.50f, .boca = 1.1f, .exclamando = HUMOR_EMPOLGADO,
            .tique = TIQUE_ACELERA } },
    { 16, { .frequencia = 3.0f, .amortecimento = 0.40f, .alegria = 0.50f, .agitacao = 0.55f, .conforto = 0.7f,    /* DJ: no ritmo */
            .empatia = 0.6f, .timidez = 0.0f, .sonolencia = 0.1f, .inquietude = 0.60f, .energia = 1.30f, .ritmo = 2.8f,
            .postura = 0.0f, .altura = -0.010f, .maos = 0.60f, .requebro = 0.80f, .boca = 1.0f, .exclamando = HUMOR_FELIZ,
            .tique = TIQUE_GINGADO } },
};

static const NoPalco *doCarro(int numero) {
    return (numero >= 0 && numero < TOTAL_NO_PALCO) ? &NO_PALCO[numero] : &NO_PALCO[0];
}

/* O rumo da pose de cada um (antes do espelho): de frente para a câmera é -pi, então mais de frente é mais negativo */
static float rumoDaPose(int numero) {
    return RUMO_DA_POSE - doCarro(numero)->deFrente;
}

const char *nomeNoPalco(int numero) {
    return doCarro(numero)->nome;
}

const char *vozDoCarroNoPalco(int numero) {
    return doCarro(numero)->voz;
}

static float limitarNoPalco(float v, float a, float b) {
    return (v < a) ? a : (v > b) ? b : v;
}

static float suave(float s) {
    s = limitarNoPalco(s, 0.0f, 1.0f);
    return s * s * (3.0f - 2.0f * s);
}

static float chegando(float s) {   /* rápido no começo, devagar no fim */
    s = limitarNoPalco(s, 0.0f, 1.0f);
    float f = 1.0f - s;
    return 1.0f - f * f * f;
}

static float duracaoDaEntrada(JeitoDeEntrar e) {
    switch (e) {
        case ENTRA_DERRAPANDO: return 1.15f;
        case ENTRA_PULANDO: return 1.0f;
        case ENTRA_DE_RE: return 2.4f;
        case ENTRA_SALTITANDO: return 1.05f;
        default: return 1.6f;   /* devagar e na hidráulica */
    }
}

/*
 * curvaDaEntrada
 * Onde ele está, para onde o bico aponta e o volante, no andamento s (0 a 1) da entrada, até a pose dele.
 */
static void curvaDaEntrada(JeitoDeEntrar e, float pose, float s, float *x, float *y, float *rumo, float *volante) {
    *y = 0.0f;
    float r0 = RUMO_PARA_A_ESQUERDA, giro = pose - RUMO_PARA_A_ESQUERDA;
    switch (e) {
        case ENTRA_DERRAPANDO:
        case ENTRA_SALTITANDO: {
            /* Chega rápido e freia derrapando: a traseira passa um pouco da pose e volta */
            *x = X_DE_ENTRADA * (1.0f - chegando(s));
            float u = suave((s - 0.40f) / 0.60f);
            float passa = (e == ENTRA_DERRAPANDO) ? 0.60f : 0.30f;
            *rumo = r0 + giro * (u + passa * sinf(PI_DO_PALCO * u));
            *volante = 0.45f * sinf(PI_DO_PALCO * u);   /* contraesterçando */
            if (e == ENTRA_SALTITANDO) {
                *y = 0.10f * fabsf(sinf(s * PI_DO_PALCO * 6.0f)) * (1.0f - s);   /* aos pulinhos */
            }
            break;
        }
        case ENTRA_PULANDO: {
            /* Num salto, virando no ar, e aterrissa na pose */
            *x = X_DE_ENTRADA * (1.0f - chegando(s));
            *y = 4.0f * 1.0f * s * (1.0f - s);   /* 1 m de altura no meio do pulo */
            *rumo = r0 + giro * suave(s * 1.2f);
            *volante = 0.0f;
            break;
        }
        case ENTRA_DE_RE: {
            /* De ré, com o bico para a direita; parado, vira no lugar até a pose */
            if (s < 0.6f) {
                float a = s / 0.6f;
                *x = 0.6f + (X_DE_ENTRADA - 0.6f) * (1.0f - chegando(a));
                *rumo = RUMO_PARA_A_DIREITA;
                *volante = 0.0f;
            } else {
                float b = (s - 0.6f) / 0.4f;
                *x = 0.6f * (1.0f - suave(b));
                *rumo = RUMO_PARA_A_DIREITA + (pose + 2.0f * PI_DO_PALCO - RUMO_PARA_A_DIREITA) * suave(b);
                *volante = 0.8f * sinf(PI_DO_PALCO * b);
            }
            break;
        }
        default: {
            /* Devagar, fazendo a curva com calma */
            *x = X_DE_ENTRADA * (1.0f - chegando(s));
            float u = suave((s - 0.35f) / 0.65f);
            *rumo = r0 + giro * u;
            *volante = -0.5f * sinf(PI_DO_PALCO * u);
            break;
        }
    }
    *x *= ESPELHO;
    *rumo *= ESPELHO;
    *volante *= ESPELHO;
}

/* ---------- A firula de cada um ---------- */
#define FIRULA_DE_NOVO_A_CADA 9.0f    /* segundos: parado, ele repete o número (mais leve) */
#define FIRULA_ESPERA         5.0f    /* ...começando tanto depois de parar */
#define FIRULA_REPETIDA       0.65f   /* a força da repetição */
#define FIRULA_NO_MAXIMO      6.0f    /* segundos: a apresentação espera o número acabar, até isto */
#define DOIS_PI               6.2831853f

/* 0 fora de [a, b], 1 dentro, com as bordas macias */
static float janela(float t, float a, float b) {
    if (t <= a || t >= b) {
        return 0.0f;
    }
    float borda = fminf(0.15f, (b - a) / 2.0f);
    return suave((t - a) / borda) * suave((b - t) / borda);
}

/* Uma subida e descida macia entre a e b */
static float bossa(float t, float a, float b) {
    if (t <= a || t >= b) {
        return 0.0f;
    }
    float v = sinf(PI_DO_PALCO * (t - a) / (b - a));
    return v * v;
}

static float onda(float t, float vezesPorSegundo) {
    return sinf(DOIS_PI * vezesPorSegundo * t);
}

/*
 * calcularFirula
 * O número de cada personagem, no tempo t (segundos desde que ele parou no disco; o locutor começa em 0,15 s).
 * Cada um diz quem ele é com o corpo que um carro tem: a suspensão, as rodas da frente, os olhos e a boca.
 * Recebe: o carro, o tempo e onde guardar.
 * Devolve: nada.
 */
static void calcularFirula(int numero, float t, Firula *f) {
    memset(f, 0, sizeof(*f));
    if (t < 0.0f) {
        return;
    }
    float w;
    switch (numero) {
        case 0:   /* McQueen, o "Katchau": acelera mostrando o motor, vira para a câmera e chega perto, com o bico baixo e
                   * o corpo deitando para ela, os olhos bem abertos e o sorriso de quem se acha. No "Katchau", o raio de
                   * luz de um flash pisca três vezes no alto do para-lama da frente (render.c) */
            w = suave((t - 0.85f) / 0.45f) * suave((3.3f - t) / 0.6f);   /* a pose: entra e sai macia */
            f->tremor = 0.8f * janela(t, 0.15f, 0.9f);
            f->giro = KATCHAU_DE_FRENTE * w;
            f->avanco = KATCHAU_CHEGA_PERTO * w;
            f->aceno = -0.03f * janela(t, 0.15f, 0.9f) + 0.06f * w;
            f->rolagem = -0.05f * LADO_DA_CAMERA * w;                       /* o teto para o lado da câmera */
            f->boca = 0.5f * janela(t, 1.0f, 1.6f) + 0.12f * w;
            f->expressoes[EXPRESSAO_FELIZ] = 0.9f * janela(t, 0.2f, 3.4f);
            f->expressoes[EXPRESSAO_CONVENCIDA] = 0.35f * w;
            f->raio = janela(t, 1.0f, 1.8f) * (0.5f + 0.5f * cosf(DOIS_PI * 4.0f * (t - 1.05f)));
            break;
        case 1:   /* The King, sereno: um aceno lento e digno */
            f->aceno = 0.035f * bossa(t, 0.4f, 1.3f) + 0.025f * bossa(t, 1.5f, 2.3f);
            f->expressoes[EXPRESSAO_FELIZ] = 0.55f * janela(t, 0.2f, 3.0f);
            f->boca = 0.15f * janela(t, 0.4f, 2.4f);
            break;
        case 2:   /* Doc, desconfiado: recua o bico, aperta os olhos e dá um "hm" curto */
            f->aceno = -0.035f * janela(t, 0.2f, 1.2f) + 0.045f * bossa(t, 1.3f, 1.7f);
            f->expressoes[EXPRESSAO_DESCONFIADA] = 0.9f * janela(t, 0.2f, 2.6f);
            break;
        case 3:   /* Francesco, metido: empina o bico e rebola */
            w = janela(t, 0.2f, 2.6f);
            f->aceno = -0.05f * w;
            f->rolagem = 0.06f * onda(t, 1.5f) * w;
            f->tremor = 0.4f * janela(t, 0.15f, 0.7f);
            f->expressoes[EXPRESSAO_FELIZ] = 0.6f * w;
            f->expressoes[EXPRESSAO_CONVENCIDA] = 0.8f * w;   /* metido */
            f->boca = 0.35f * w;
            break;
        case 4:   /* Sally, simpática: inclina de lado, sorri e dá um pulinho */
            w = janela(t, 0.3f, 2.6f);
            f->rolagem = 0.03f * w;
            f->subida = 0.03f * bossa(t, 0.35f, 0.7f);
            f->expressoes[EXPRESSAO_FELIZ] = 0.9f * w;
            f->boca = 0.2f * w;
            break;
        case 5:   /* Ramone, o lowrider: a bomba da hidráulica leva a traseira lá para cima (a mola do corpo faz a
                   * subida macia e o balanço lá em cima), ele bombeia no ritmo, desce, e fecha com o "três rodas":
                   * deita para um lado e empina, com uma roda da frente no ar */
            w = janela(t, 0.25f, 2.0f);
            f->traseira = (0.36f + 0.05f * onda(t - 0.9f, 1.1f) * janela(t, 0.9f, 2.0f)) * w;
            f->rolagem = 0.15f * janela(t, 2.4f, 3.5f);
            f->aceno = -0.06f * janela(t, 2.4f, 3.5f);
            f->expressoes[EXPRESSAO_CONVENCIDA] = 0.6f * janela(t, 0.2f, 3.8f);   /* o olhar de quem é estiloso */
            f->expressoes[EXPRESSAO_FELIZ] = 0.5f * janela(t, 0.2f, 3.8f);
            f->boca = 0.1f * janela(t, 0.2f, 3.8f);
            break;
        case 6:   /* Luigi, empolgado: pulinhos e as rodas da frente gesticulando */
            w = janela(t, 0.2f, 1.8f);
            f->subida = 0.05f * fabsf(onda(t, 1.5f)) * w;
            f->maos = 0.6f * onda(t, 2.5f) * w;
            f->expressoes[EXPRESSAO_FELIZ] = 1.0f * janela(t, 0.2f, 2.8f);
            f->expressoes[EXPRESSAO_ESPANTADA] = 0.3f * w;
            f->boca = 0.5f * w;
            break;
        case 7:   /* Guido: rodopia no lugar com os garfos erguidos e, parado, cruza os garfos como braços e encara a
                   * câmera de pálpebra pesada, o bico para cima: "e daí?" (o Guido do filme, sem se impressionar) */
            f->giro = DOIS_PI * suave((t - 0.3f) / 0.7f);
            f->garfos = janela(t, 0.25f, 1.25f);
            f->subida = 0.04f * fabsf(onda(t, 2.0f)) * janela(t, 0.3f, 1.1f);
            f->expressoes[EXPRESSAO_FELIZ] = 1.0f * janela(t, 0.2f, 1.3f);
            f->boca = 0.6f * janela(t, 0.3f, 1.1f);
            w = suave((t - 1.35f) / 0.5f) * suave((4.6f - t) / 0.5f);    /* os braços cruzados */
            f->cruzaGarfos = w;
            f->aceno = -0.03f * w;
            f->expressoes[EXPRESSAO_ENTEDIADA] = w;
            break;
        case 8:   /* Mate, bobo e feliz: chacoalha, balança, olha para todo lado com o sorriso dentuço */
            w = janela(t, 0.2f, 2.4f);
            f->tremor = 0.6f * w;
            f->rolagem = 0.06f * onda(t, 1.0f) * w;
            f->olhar = 0.5f * onda(t, 0.8f) * w;
            f->avanco = -0.3f * bossa(t, 0.3f, 0.8f);
            f->boca = 0.7f * janela(t, 0.6f, 2.2f);
            f->expressoes[EXPRESSAO_FELIZ] = 1.0f * w;
            break;
        case 9:   /* Sargento, militar: estufa o peito, firme, e bate continência com o corpo */
            w = janela(t, 0.2f, 2.4f);
            f->subida = 0.04f * w;
            f->aceno = -0.035f * w + 0.06f * bossa(t, 0.9f, 1.15f);
            f->expressoes[EXPRESSAO_CONCENTRADA] = 0.7f * w;   /* firme */
            f->expressoes[EXPRESSAO_BRAVA] = 0.15f * w;
            break;
        case 10:  /* Fillmore, zen: balança devagar, de olhos meio fechados */
            w = janela(t, 0.2f, 3.4f);
            f->rolagem = 0.05f * onda(t, 0.4f) * w;
            f->expressoes[EXPRESSAO_SONOLENTA] = 0.5f * w;
            f->expressoes[EXPRESSAO_FELIZ] = 0.6f * w;
            f->boca = 0.25f * w;
            break;
        case 11:  /* Finn, espião: se abaixa, pronto, e confere um lado e o outro */
            w = janela(t, 0.2f, 2.0f);
            f->subida = -0.03f * w;
            f->aceno = 0.02f * w;
            f->olhar = -0.6f * janela(t, 0.6f, 1.0f) + 0.6f * janela(t, 1.0f, 1.4f);
            f->expressoes[EXPRESSAO_CONCENTRADA] = 0.5f * janela(t, 0.2f, 2.6f);
            f->expressoes[EXPRESSAO_DESCONFIADA] = 0.4f * janela(t, 0.2f, 1.4f);   /* conferindo */
            break;
        case 12:  /* Wingo, exibido: quica na suspensão e acelera */
            w = janela(t, 0.2f, 1.8f);
            f->subida = 0.05f * fabsf(onda(t, 1.25f)) * w;
            f->tremor = 0.7f * w;
            f->expressoes[EXPRESSAO_FELIZ] = 0.8f * w;
            f->expressoes[EXPRESSAO_CONVENCIDA] = 0.35f * w;   /* exibido */
            f->boca = 0.4f * w;
            break;
        case 13:  /* Snot Rod, o espirro: prende (o bico sobe, os olhos fecham)... ATCHIM! (o tranco) e sacode */
            f->aceno = -0.07f * janela(t, 0.2f, 1.0f) + 0.10f * bossa(t, 1.0f, 1.3f);
            f->avanco = 0.9f * bossa(t, 1.0f, 1.4f);
            f->tremor = 1.0f * janela(t, 1.0f, 1.4f);
            f->rolagem = 0.05f * onda(t, 4.0f) * janela(t, 1.3f, 1.8f);
            f->expressoes[EXPRESSAO_SONOLENTA] = 0.5f * janela(t, 0.2f, 1.0f) + 1.0f * janela(t, 1.0f, 1.3f);
            f->boca = 0.3f * janela(t, 0.4f, 1.0f) + 0.8f * janela(t, 1.0f, 1.2f);
            break;
        case 14:  /* Storm, arrogante: quase não se mexe; o bico erguido e o meio sorriso dizem tudo */
            w = janela(t, 0.2f, 3.0f);
            f->aceno = -0.025f * w;
            f->expressoes[EXPRESSAO_CONVENCIDA] = 0.9f * w;   /* arrogante */
            f->boca = 0.1f * w;
            break;
        case 15:  /* Boost, agressivo: acelera e avança para intimidar */
            f->tremor = 1.0f * janela(t, 0.2f, 1.2f);
            f->avanco = 0.35f * bossa(t, 0.6f, 1.0f);
            f->aceno = 0.05f * bossa(t, 0.6f, 1.0f);
            f->expressoes[EXPRESSAO_BRAVA] = 0.5f * janela(t, 0.2f, 2.4f);
            f->boca = 0.3f * janela(t, 0.5f, 1.1f);
            break;
        case 16:  /* DJ, no ritmo: dança na batida */
            w = janela(t, 0.2f, 3.4f);
            f->subida = 0.035f * fabsf(onda(t, 1.0f)) * w;
            f->rolagem = 0.05f * onda(t, 1.0f) * w;
            f->expressoes[EXPRESSAO_FELIZ] = 0.9f * w;
            f->expressoes[EXPRESSAO_SONOLENTA] = 0.2f * w;
            f->boca = 0.3f * w;
            break;
        default:
            break;
    }
}

/*
 * firulaAcabou
 * O número dele já acabou no tempo t? Todo canal parado (o rodopio, de volta inteira). A apresentação espera por ele:
 * o Guido de garfos cruzados e o "três rodas" do Ramone passam do anúncio do nome.
 * Recebe: o carro e o tempo (segundos desde que parou).
 * Devolve: 1 se acabou; 0 se não.
 */
static int firulaAcabou(int numero, float t) {
    if (t >= FIRULA_NO_MAXIMO) {
        return 1;
    }
    Firula f;
    calcularFirula(numero, t, &f);
    f.giro = remainderf(f.giro, DOIS_PI);
    const float *canais = &f.aceno;
    for (size_t i = 0; i < sizeof(Firula) / sizeof(float); i++) {
        if (fabsf(canais[i]) > 0.001f) {
            return 0;
        }
    }
    return 1;
}

/* Uma mola da suspensão do palco: vai para o alvo com o balanço dela */
static void mola(float *x, float *v, float alvo, float tempo) {
    float w = 2.0f * PI_DO_PALCO * FREQUENCIA_DA_MOLA;
    *v += (w * w * (alvo - *x) - 2.0f * AMORTECIMENTO * w * *v) * tempo;
    *x += *v * tempo;
}

static void comecarEntrada(Palco *p, int numero) {
    p->numero = numero;
    p->proximo = -1;
    p->fase = PALCO_ENTRANDO;
    p->tempo = 0.0f;
    float y, rumo, volante, x;
    curvaDaEntrada(doCarro(numero)->entrada, rumoDaPose(numero), 0.0f, &x, &y, &rumo, &volante);
    p->x = x;
    p->y = y;
    p->rumo = rumo;
    p->volante = volante;
    p->velocidade = 0.0f;
    p->velocidadeAntes = 0.0f;
    p->arfagem = p->velocidadeDaArfagem = 0.0f;
    p->rolagem = p->velocidadeDaRolagem = 0.0f;
    p->subida = p->velocidadeDaSubida = 0.0f;
    p->pancada = 0.0f;
    p->olhar = 0.0f;
    p->anuncio = NULL;
    p->anuncioComecou = 0;
    p->anunciou = 0;
    p->duracaoDoAnuncio = 0.0f;
    p->tempoDoAnuncio = 0.0f;
    memset(&p->firula, 0, sizeof(p->firula));
    memset(&p->velocidadeDaFirula, 0, sizeof(p->velocidadeDaFirula));
    p->jeito = *jeitoDoModelo(numero);
    for (size_t i = 0; i < sizeof(JEITOS_DE_FORA) / sizeof(JEITOS_DE_FORA[0]); i++) {
        if (JEITOS_DE_FORA[i].numero == numero) {
            p->jeito = JEITOS_DE_FORA[i].jeito;
        }
    }
    p->jeito.sonolencia = 0.0f;   /* no palco ninguém cochila */
    iniciarCorpo(&p->corpo, &p->jeito, 7u + (unsigned int) numero * 131u);
    memset(&p->pose, 0, sizeof(p->pose));
}

void iniciarPalco(Palco *palco) {
    memset(palco, 0, sizeof(*palco));
    palco->numero = -1;
    palco->proximo = -1;
    palco->fase = PALCO_VAZIO;
}

void mostrarNoPalco(Palco *palco, int numero) {
    if (palco->fase == PALCO_VAZIO) {
        comecarEntrada(palco, numero);
        return;
    }
    if (palco->fase == PALCO_SAINDO) {
        palco->proximo = (numero == palco->numero) ? -1 : numero;
        return;
    }
    if (numero == palco->numero) {
        return;
    }
    /* Mal começou a entrar: o novo entra no lugar dele, sem esperar a saída */
    if (palco->fase == PALCO_ENTRANDO && palco->tempo < CORTA_A_ENTRADA * duracaoDaEntrada(doCarro(palco->numero)->entrada)) {
        comecarEntrada(palco, numero);
        return;
    }
    palco->fase = PALCO_SAINDO;
    palco->tempo = 0.0f;
    palco->rumoNoComeco = palco->rumo;
    palco->proximo = numero;
    palco->anuncio = NULL;
}

void duracaoDoAnuncioNoPalco(Palco *palco, float segundos) {
    if (segundos > 0.0f && palco->anuncio != NULL) {
        palco->duracaoDoAnuncio = segundos;
    }
}

void atualizarPalco(Palco *p, float tempoQuadro) {
    p->anuncioComecou = 0;
    if (p->fase == PALCO_VAZIO || p->numero < 0 || tempoQuadro <= 0.0f) {
        return;
    }
    tempoQuadro = fminf(tempoQuadro, 0.05f);
    p->tempo += tempoQuadro;
    const NoPalco *dele = doCarro(p->numero);
    float xAntes = p->x, rumoAntes = p->rumo;
    p->pancada = 0.0f;

    if (p->fase == PALCO_ENTRANDO) {
        float duracao = duracaoDaEntrada(dele->entrada);
        float s = p->tempo / duracao;
        float yAntes = p->y;
        curvaDaEntrada(dele->entrada, rumoDaPose(p->numero), s, &p->x, &p->y, &p->rumo, &p->volante);
        if (s >= 1.0f) {
            p->x = 0.0f;
            p->y = 0.0f;
            p->rumo = ESPELHO * rumoDaPose(p->numero);
            p->volante = 0.0f;
            p->fase = PALCO_APRESENTANDO;
            p->tempo = 0.0f;
            /* Chegou animado: os gestos e os tiques saem maiores */
            p->corpo.animo.alegria = fminf(p->jeito.alegria + 0.4f, 1.0f);
            p->corpo.animo.agitacao = fminf(p->jeito.agitacao + 0.35f, 1.0f);
            p->corpo.animo.interesse = 1.0f;
            if (dele->entrada == ENTRA_PULANDO) {
                p->pancada = 0.6f;                 /* a aterrissagem: o corpo afunda e volta */
                p->velocidadeDaSubida -= 1.6f;
            }
        } else if (dele->entrada == ENTRA_PULANDO && yAntes > 0.0f && p->y <= 0.0f) {
            p->pancada = 0.6f;
        }
    } else if (p->fase == PALCO_SAINDO) {
        float s = p->tempo / SEGUNDOS_DA_SAIDA;
        float u = fminf(s * 1.6f, 1.0f);
        p->x = X_DE_SAIDA * s * s;
        p->rumo = p->rumoNoComeco + (ESPELHO * RUMO_PARA_A_ESQUERDA - p->rumoNoComeco) * suave(u);
        p->volante = ESPELHO * 0.5f * sinf(PI_DO_PALCO * u);
        p->anuncio = NULL;
        if (s >= 1.0f) {
            int proximo = p->proximo;
            if (proximo >= 0) {
                comecarEntrada(p, proximo);
            } else {
                iniciarPalco(p);
            }
            return;
        }
    } else {
        /* Parado: o anúncio do nome, e depois vivo, olhando para quem escolhe */
        if (p->fase == PALCO_APRESENTANDO) {
            float antes = p->tempo - tempoQuadro;
            if (!p->anunciou && p->tempo >= ANUNCIO_DEPOIS_DE) {
                p->anunciou = 1;
                p->anuncio = dele->nome;
                p->anuncioComecou = 1;
                p->tempoDoAnuncio = 0.0f;
                p->duracaoDoAnuncio = fmaxf((float) strlen(dele->nome) * SEGUNDOS_POR_LETRA, MENOR_ANUNCIO);
            } else if (p->anunciou) {
                p->tempoDoAnuncio += tempoQuadro;
                if (p->anuncio != NULL && p->tempoDoAnuncio >= p->duracaoDoAnuncio) {
                    p->anuncio = NULL;
                }
                if (p->tempoDoAnuncio >= p->duracaoDoAnuncio + FICA_DEPOIS_DO_ANUNCIO && firulaAcabou(p->numero, p->tempo)) {
                    p->fase = PALCO_PARADO;
                    p->tempo = 0.0f;
                }
            }
            /* O Ramone chega bombeando a hidráulica: dois pulos */
            if (dele->entrada == ENTRA_NA_HIDRAULICA && ((antes < 0.25f && p->tempo >= 0.25f) || (antes < 0.75f && p->tempo >= 0.75f))) {
                p->velocidadeDaSubida += 1.4f;
            }
        }
    }

    /* A física do palco: a velocidade, as acelerações e a suspensão */
    float frente = (p->x - xAntes) * sinf(p->rumo);   /* o que andou para onde o bico aponta */
    p->velocidade = frente / tempoQuadro;
    p->giroDasRodas += frente / RAIO_DA_RODA;
    float aceleracao = (p->velocidade - p->velocidadeAntes) / tempoQuadro;
    p->velocidadeAntes = p->velocidade;
    float giro = (p->rumo - rumoAntes) / tempoQuadro;
    float alvoDoMergulho = limitarNoPalco(-aceleracao * MERGULHO_POR_ACELERACAO * ((p->velocidade >= 0.0f) ? 1.0f : -1.0f),
                                          -MAIOR_MERGULHO, MAIOR_MERGULHO);
    float alvoDaRolagem = limitarNoPalco(giro * fabsf(p->velocidade) * ROLAGEM_POR_CURVA, -MAIOR_ROLAGEM, MAIOR_ROLAGEM);
    if (p->fase == PALCO_APRESENTANDO || p->fase == PALCO_PARADO) {
        alvoDoMergulho = 0.0f;
        alvoDaRolagem = 0.0f;
    }
    mola(&p->arfagem, &p->velocidadeDaArfagem, alvoDoMergulho, tempoQuadro);
    mola(&p->rolagem, &p->velocidadeDaRolagem, alvoDaRolagem, tempoQuadro);
    mola(&p->subida, &p->velocidadeDaSubida, 0.0f, tempoQuadro);

    /* O corpo dele (gestos.c): ouvindo o locutor, acena nas ênfases dele (no começo do nome e no sobrenome) e pega
     * o ânimo de quem anuncia; os olhos na câmera e os tiques */
    Situacao agora = { 0 };
    agora.parado = (p->fase == PALCO_APRESENTANDO || p->fase == PALCO_PARADO);
    agora.velocidade = fabsf(p->velocidade);
    agora.volante = p->volante;
    if (p->anuncio != NULL) {
        float t = p->tempoDoAnuncio, d = p->duracaoDoAnuncio;
        agora.ouvindo = 1;
        agora.alegriaDoOutro = 0.8f;
        agora.agitacaoDoOutro = 0.9f;
        int naEnfase = (t - tempoQuadro < 0.05f && t >= 0.05f) || (t - tempoQuadro < 0.55f * d && t >= 0.55f * d);
        agora.enfaseDoOutro = naEnfase ? ENFASE_DO_LOCUTOR : 0.0f;
    }
    agora.atento = agora.parado;
    agora.lado = 0.7f * LADO_DA_CAMERA;
    agora.perto = 0.9f;
    agora.visita = agora.parado ? 0.7f : 0.0f;
    agora.ladoDoJogador = 0.7f * LADO_DA_CAMERA;
    agora.batida = p->pancada;
    agora.sozinho = 0;
    moverCorpo(&p->corpo, &p->jeito, &agora, tempoQuadro, &p->pose);

    /* Os olhos: entrando, para onde vira; parado, nos olhos de quem escolhe (o contato visual), e só de vez em
     * quando uma olhada rápida para o lado, que volta */
    p->naCamera = (p->fase == PALCO_APRESENTANDO || p->fase == PALCO_PARADO);
    p->olhar += (p->volante * 0.6f - p->olhar) * fminf(tempoQuadro * 8.0f, 1.0f);
    float olhada = (p->fase == PALCO_PARADO) ? janela(fmodf(p->tempo, 6.5f), 3.6f, 4.3f) : 0.0f;
    p->desvioDoOlhar = -OLHADA_PARA_O_LADO * LADO_DA_CAMERA * olhada;

    /* A firula: chegando, a dele inteira; parado, de novo de tempos em tempos, mais leve (o rodopio e os garfos
     * cruzados não ficam pela metade: são inteiros ou nada) */
    Firula alvo;
    if (p->fase == PALCO_APRESENTANDO) {
        calcularFirula(p->numero, p->tempo, &alvo);
    } else if (p->fase == PALCO_PARADO) {
        calcularFirula(p->numero, fmodf(p->tempo, FIRULA_DE_NOVO_A_CADA) - FIRULA_ESPERA, &alvo);
        float giro = alvo.giro, cruzaGarfos = alvo.cruzaGarfos;
        float *canais = &alvo.aceno;   /* a Firula é toda de floats */
        for (size_t i = 0; i < sizeof(Firula) / sizeof(float); i++) {
            canais[i] *= FIRULA_REPETIDA;
        }
        alvo.giro = giro;
        alvo.cruzaGarfos = cruzaGarfos;   /* os garfos pela metade se cruzariam um dentro do outro */
    } else {
        memset(&alvo, 0, sizeof(alvo));
    }
    /* O corpo vai atrás do número com a inércia dele: uma mola com a suspensão de cada um (o Mate mole, balança e
     * demora a parar; o Sargento seco). O rodopio, a boca, os olhos e o tremor vão direto */
    float frequencia = limitarNoPalco(p->jeito.frequencia * 0.9f, 1.6f, 4.0f);
    float amortecimento = limitarNoPalco(p->jeito.amortecimento, 0.35f, 0.85f);
    float w = 2.0f * PI_DO_PALCO * frequencia;
    float *x = &p->firula.aceno, *v = &p->velocidadeDaFirula.aceno;
    const float *a = &alvo.aceno;
    for (size_t i = 0; i < sizeof(Firula) / sizeof(float); i++) {
        const float *canal = &a[i];
        int comMola = (canal == &alvo.aceno || canal == &alvo.traseira || canal == &alvo.rolagem || canal == &alvo.subida
                       || canal == &alvo.avanco || canal == &alvo.maos || canal == &alvo.garfos);
        if (comMola) {
            v[i] += (w * w * (a[i] - x[i]) - 2.0f * amortecimento * w * v[i]) * tempoQuadro;
            x[i] += v[i] * tempoQuadro;
        } else {
            x[i] = a[i];
            v[i] = 0.0f;
        }
    }
}
