/*
 * gestos.c
 * ------------------------------------------------------------------
 * A linguagem do corpo dos moradores. Veja gestos.h.
 *
 * Um passo do corpo:
 *   1. o ânimo: os estímulos do momento (a fala dele, a de quem conversa com
 *      ele, o jogador de visita ou passando rente e rápido, uma batida, o
 *      xerife atrás de alguém) o empurram, e ele volta devagar ao de sempre;
 *      o interesse vai para quem ele olha; o sono chega a quem está sozinho;
 *   2. a postura que sai do ânimo: bravo, o bico baixo; com medo ou sem
 *      graça, encolhido e longe de quem o assusta; contente, o corpo alto;
 *      interessado, inclinado para o outro; com sono, caído;
 *   3. o involuntário de quem está parado: a respiração (mais rápida
 *      agitado), o peso que muda de lado e o corpo que nunca para de todo
 *      (um ruído suave, diferente em cada um);
 *   4. andando: a arrancada, a freada, a curva e os solavancos da rua;
 *   5. a fala: cada palavra que conta (as maiores, as com "!") é uma ênfase,
 *      na hora em que é dita; o jeito da ênfase sai do ânimo (contente e
 *      agitado, pulinhos; bravo, acenos secos e o corpo sacudindo; calmo,
 *      acenos pequenos); a boca abre nas vogais e fecha no m, no b e no p;
 *      a pergunta deita a cabeça no fim;
 *   6. ouvindo: o "uhum" vem logo depois de uma ênfase de quem fala, se ele
 *      está prestando atenção e concorda;
 *   7. o reflexo do susto: o pulo para trás, na medida do perigo; o tremor
 *      do medo; o ar de um carro passando rente;
 *   8. os tiques, que nascem de vontades que o ânimo enche;
 *   9. as molas; 10. os olhos, do ânimo.
 * ------------------------------------------------------------------
 */

#include <ctype.h>
#include <math.h>
#include <string.h>
#include "gestos.h"

#define PI_G                  3.14159265f
#define PASSO_DAS_MOLAS       (1.0f / 240.0f)

/* Os limites (macios) da suspensão: mais que isso, a lataria engoliria as rodas */
#define ROLAGEM_MAXIMA        0.10f     /* radianos */
#define ACENO_MAXIMO          0.10f
#define SUBIDA_MAXIMA         0.12f     /* metros */

/* Quanto dura cada gesto (segundos); com a suspensão mole, pelo menos meia volta dela */
#define DURA_O_ACENO          0.28f
#define DURA_O_PULO           0.24f
#define DURA_O_LADO           0.36f
#define DURA_O_UHUM           0.34f
#define DURA_A_PREPARACAO     0.30f
#define DURA_O_SUSTO          0.14f     /* o reflexo é rápido, mas não é um tranco */
#define DURA_O_TIQUE          0.16f
#define DURA_O_SOLAVANCO      0.10f
#define MEIA_VOLTA            0.45f     /* de uma volta da suspensão: o gesto mais curto que ela deixa */
#define ENTRE_TOQUES          0.22f     /* segundos: as mãos mexem a cada palavra, mas não mais perto que isso */
#define FRASE_CAI             0.35f     /* as ênfases vão caindo até o fim da frase (tanto menores, no fim) */
#define RITMO_FOLGA           0.75f     /* as ênfases ficam a pelo menos 1 / (ritmo * isto) segundos uma da outra */

/* O ânimo volta ao de sempre em tantos segundos */
#define VOLTA_A_ALEGRIA       8.0f
#define VOLTA_A_AGITACAO      4.0f
#define VOLTA_O_CONFORTO      5.0f
/* Os estímulos, por segundo */
#define VISITA_ALEGRA         0.04f     /* companhia (menos para os tímidos)... (vezes o tempo da volta: +0,3 de alegria) */
#define VISITA_AGITA          0.05f
#define VISITA_ENCABULA       0.20f     /* ...e, para os tímidos, sem graça (vezes a timidez) */
#define PERIGO_ASSUSTA        2.5f
#define PERIGO_AGITA          2.5f
#define PERIGO_CHATEIA        0.8f
#define BRONCA_IRRITA         1.0f
#define FALAR_AGITA           0.03f
#define CONTAGIO              0.8f      /* vezes a empatia: o ânimo de quem fala passa para quem ouve */
/* O sono */
#define SONO_CHEGA_EM         25.0f     /* segundos, com toda a sonolência, de noite */
#define DORMIU                0.75f
#define ACORDOU               0.35f

/* A postura do ânimo (radianos e metros) */
#define BRAVO_ABAIXA          0.030f    /* bravo: o bico baixo, de quem vai para cima */
#define MEDO_ABAIXA           0.040f    /* com medo ou sem graça: encolhido... */
#define MEDO_ENCOLHE          0.060f
#define MEDO_AFASTA           0.035f    /* ...e deitado para longe do jogador */
#define SONO_CAI              0.030f
#define SONO_BAIXA            0.035f
#define TRISTE_CAI            0.020f
#define ORGULHO_ERGUE         0.015f    /* contente e agitado: o nariz em pé */
#define ALEGRIA_SOBE          0.015f
#define AGITACAO_SOBE         0.025f
#define VAI_PARA_O_OUTRO      0.030f    /* interessado: o corpo deita para o lado de quem ele olha... */
#define CHEGA_PERTO           0.012f    /* ...e o bico desce um pouco */
#define CABECA_DE_LADO        0.040f    /* a pergunta */

/* O involuntário, parado */
#define RESPIRO               0.006f    /* metros: o motor em marcha lenta */
#define RESPIRO_CALMO_HZ      0.18f
#define RESPIRO_AGITADO_HZ    0.65f
#define PESO_DE_LADO          0.018f    /* radianos: o peso muda de lado devagar */
#define MEXIDA                0.008f    /* o corpo que não para (radianos e metros) */
#define MAOS_INQUIETAS        0.45f     /* frações do volante */
#define TREMOR                0.006f    /* radianos, com todo o medo */
#define TREMOR_HZ             8.5f
#define VENTO                 1.5f      /* o ar de um carro passando rente, na rolagem */

/* A fala */
#define LETRAS_POR_SEGUNDO_DE 9.0f
#define LETRAS_POR_SEGUNDO_ATE 16.0f
#define FALA_OCUPA            0.8f      /* a fala é dita em tanto do tempo em que fica na tela */
#define PAUSA_NA_VIRGULA      0.18f     /* segundos */
#define PAUSA_NO_PONTO        0.32f
#define PERGUNTA_ANTES        0.7f      /* segundos antes do "?": a cabeça começa a deitar */
#define ACENO_DA_FALA         0.060f    /* radianos: o tamanho de uma ênfase... */
#define PULO_DA_FALA          0.050f    /* metros: ...contente, um pulinho... */
#define LADO_DA_FALA          0.040f    /* ...e de lado (o requebro, o "não, não" de bravo) */
#define MAOS_NO_OUTRO         0.40f     /* as rodas da frente apontam para quem ouve... */
#define MAOS_FALANDO          0.85f     /* ...e gesticulam até tanto, um lado de cada vez (frações do volante) */
#define MAOS_DE_TODO_MUNDO    0.35f     /* até quem quase não gesticula mexe as mãos tanto assim (o resto é o jeito dele) */
#define MAOS_VOLTAM           2.5f      /* por segundo: depois do gesto, as mãos voltam a apontar para quem ouve */
#define ABRE_NO_PONTO         0.8f      /* um "!" forte abre as mãos tanto (os braços abertos)... */
#define ABRE_NA_PERGUNTA      0.9f      /* ...e a pergunta, no fim, tanto */
#define FALANDO_ANDANDO       0.4f      /* andando, as ênfases são menores */

/* Ouvindo */
#define UHUM                  0.028f
#define UHUM_DEPOIS_DE        0.25f     /* segundos depois da ênfase de quem fala */
#define UHUM_ATE              0.6f

/* O susto */
#define SUSTO_A_PARTIR        0.35f     /* de perigo: daí em diante, o reflexo (os tímidos, antes) */
#define PULO_DO_SUSTO         0.090f
#define RECUO_DO_SUSTO        0.060f    /* o bico sobe */
#define DESVIA_DO_SUSTO       0.060f    /* o corpo deita para longe */
#define SUSTO_DE_NOVO         1.5f      /* segundos sem outro reflexo */

/* Andando */
#define ARRANCADA_NO_BICO     0.0045f   /* radianos por m/s²: agacha atrás acelerando, mergulha freando */
#define ARRANCADA_MAXIMA      7.0f      /* m/s² (sair de ré é um degrau na velocidade) */
#define DEITA_NA_CURVA        0.06f     /* radianos, com o volante todo a 9 m/s */
#define SOLAVANCO             0.008f    /* metros, a 9 m/s */
#define SUSPENSAO_NORMAL      2.5f      /* Hz: quem tem a suspensão mais mole que isso balança mais andando */

/* O que não é suspensão: as mãos, os garfos e a boca vão com vontade; a pergunta, devagar */
#define MOLA_DAS_MAOS         3.2f, 0.62f   /* rápidas, passam um tico do ponto e assentam (como uma mão) */
#define MOLA_DA_ABERTURA      2.2f, 0.75f
#define MOLA_DOS_GARFOS       3.0f, 0.6f
#define MOLA_DA_BOCA          9.0f, 0.8f
#define MOLA_DA_PERGUNTA      1.5f, 0.9f

/* Os tiques */
#define ACELERADA             0.035f    /* o McQueen acelerando parado: o corpo torce com o motor... */
#define ACELERADA_AGACHA      0.015f    /* ...e agacha atrás */
#define ENTRE_ACELERADAS      0.45f     /* segundos */
#define HIDRAULICA_SOBE       0.10f     /* metros: o Ramone bombeando */
#define HIDRAULICA_HZ         2.0f
#define HIDRAULICA_DE_LADO    0.07f     /* radianos: um lado erguido */
#define DESLEIXA_EM           30.0f     /* segundos para o Sargento desleixar de todo */
#define MOTOR_VELHO_ROLA      0.0025f   /* a Lizzie: o tremor do motor velho */
#define MOTOR_VELHO_SOBE      0.0015f
#define GINGADO               0.030f    /* radianos: o Fillmore balançando */
#define GINGADO_HZ            0.32f
#define CHACOALHA_A_PARTIR    0.70f     /* radianos por segundo: um tranco (um susto, um solavanco forte), a lataria do Mate chacoalha */

/* O empurrão que cada humor de fala dá no ânimo, e se ele pensa no que diz */
typedef struct {
    float alegria, agitacao, conforto, pensa;
} EfeitoDoHumor;

static const EfeitoDoHumor EFEITOS[TOTAL_DE_HUMORES] = {
    [HUMOR_NEUTRO]    = { 0.0f, 0.0f, 0.0f, 0.0f },
    [HUMOR_FELIZ]     = { 0.35f, 0.10f, 0.10f, 0.0f },
    [HUMOR_EMPOLGADO] = { 0.50f, 0.45f, 0.10f, 0.0f },
    [HUMOR_BRAVO]     = { -0.50f, 0.40f, 0.0f, 0.0f },
    [HUMOR_ASSUSTADO] = { -0.30f, 0.50f, -0.60f, 0.0f },
    [HUMOR_TIMIDO]    = { 0.05f, -0.05f, -0.50f, 0.0f },
    [HUMOR_CURIOSO]   = { 0.05f, 0.10f, 0.0f, 0.0f },
    [HUMOR_PENSATIVO] = { 0.0f, -0.15f, 0.0f, 1.0f },
    [HUMOR_CALMO]     = { 0.15f, -0.25f, 0.20f, 0.0f },
    [HUMOR_TRISTE]    = { -0.45f, -0.20f, -0.10f, 0.3f },
    [HUMOR_SERIO]     = { -0.15f, 0.05f, 0.10f, 0.0f },
};

static float limitar(float v, float a, float b) {
    return (v < a) ? a : (v > b) ? b : v;
}

/* Um número entre a e b, dos sorteios só deste corpo (não mexe no rand() do jogo) */
static float sortear(Corpo *c, float a, float b) {
    c->sorteio = c->sorteio * 1664525u + 1013904223u;
    return a + (b - a) * (float) (c->sorteio >> 8) / 16777216.0f;
}

/*
 * molar
 * Leva a mola em direção ao alvo, em passos pequenos (assim não explode com um quadro lento), e
 * para no batente.
 */
static void molar(MolaDoCorpo *mola, float alvo, float frequencia, float amortecimento, float tempo,
                  float menor, float maior) {
    float w = 2.0f * PI_G * frequencia;
    for (float resta = tempo; resta > 0.0f; resta -= PASSO_DAS_MOLAS) {
        float h = fminf(resta, PASSO_DAS_MOLAS);
        mola->v += (w * w * (alvo - mola->x) - 2.0f * amortecimento * w * mola->v) * h;
        mola->x += mola->v * h;
    }
    if (mola->x < menor || mola->x > maior) {
        mola->x = limitar(mola->x, menor, maior);
        mola->v = 0.0f;
    }
}

/* Os eixos dos gestos */
enum { ROLAGEM, ACENO, SUBIDA };

/*
 * picoDoPulso
 * Até onde a mola vai seguindo um pulso de alvo de altura 1 (sobe e volta em "duracao" segundos):
 * a mola mole fica para trás de um pulso curto. Com isso o gesto sai do tamanho pedido em qualquer
 * suspensão.
 */
static float picoDoPulso(float frequencia, float amortecimento, float duracao) {
    MolaDoCorpo m = { 0.0f, 0.0f };
    float w = 2.0f * PI_G * frequencia, pico = 0.0f;
    for (float t = 0.0f; t < duracao + 1.5f; t += PASSO_DAS_MOLAS) {
        float k = (t < duracao) ? sinf(PI_G * t / duracao) : 0.0f;
        m.v += (w * w * (k * k - m.x) - 2.0f * amortecimento * w * m.v) * PASSO_DAS_MOLAS;
        m.x += m.v * PASSO_DAS_MOLAS;
        pico = fmaxf(pico, fabsf(m.x));
    }
    return fmaxf(pico, 0.05f);
}

/*
 * gesto
 * Um gesto num eixo do corpo: o alvo da mola sobe e volta numa curva suave (nunca um tranco), e a
 * mola vai atrás dele, do jeito dela (a mole balança na volta). O gesto dura pelo menos meia volta da
 * suspensão, então quem é pesado e mole gesticula mais devagar; e sai do tamanho pedido (o pico).
 */
static void gesto(Corpo *c, const Jeito *j, int eixo, float quanto, float duracao) {
    if (fabsf(quanto) < 1e-5f) {
        return;
    }
    float t = fmaxf(duracao, MEIA_VOLTA / fmaxf(j->frequencia, 0.1f));
    Pulso *livre = &c->pulsos[eixo][0];
    for (int k = 0; k < MAIS_PULSOS; k++) {
        Pulso *p = &c->pulsos[eixo][k];
        if (p->duracao <= 0.0f) {
            livre = p;
            break;
        }
        if (p->inicio < livre->inicio) {
            livre = p;   /* cheio: troca o mais velho */
        }
    }
    livre->inicio = c->relogio;
    livre->duracao = t;
    livre->quanto = quanto / picoDoPulso(j->frequencia, j->amortecimento, t);
}

/* Quanto os gestos em andamento põem no alvo de um eixo agora */
static float pulsosAgora(Corpo *c, int eixo) {
    float soma = 0.0f;
    for (int k = 0; k < MAIS_PULSOS; k++) {
        Pulso *p = &c->pulsos[eixo][k];
        if (p->duracao <= 0.0f) {
            continue;
        }
        float t = (c->relogio - p->inicio) / p->duracao;
        if (t >= 1.0f) {
            p->duracao = 0.0f;
            continue;
        }
        float k2 = sinf(PI_G * t);
        soma += p->quanto * k2 * k2;
    }
    return soma;
}

/* Um limite macio: perto dele, o corpo vai cada vez menos (a suspensão endurecendo), sem bater */
static float macio(float x, float limite) {
    return limite * tanhf(x / limite);
}

/* Um ruído suave (de -1 a +1, mais ou menos), diferente em cada um: o corpo que nunca fica parado de todo */
static float ruido(const Corpo *c, int canal, float frequencia) {
    const float *f = c->fases[canal];
    float t = c->relogio * 2.0f * PI_G * frequencia;
    return 0.5f * sinf(t + f[0]) + 0.3f * sinf(t * 2.17f + f[1]) + 0.2f * sinf(t * 3.91f + f[2]);
}

/* ------------------------------------------------------------------
 * A fala
 * ------------------------------------------------------------------ */

static int continuacao(unsigned char ch) {
    return (ch & 0xC0) == 0x80;   /* o segundo byte de uma letra acentuada (UTF-8) */
}

static int pontuacao(unsigned char ch) {
    return ch == ',' || ch == '.' || ch == '!' || ch == '?' || ch == ';' || ch == ':' || ch == '(' || ch == ')';
}

/* O tempo de uma letra (com as pausas da pontuação) */
static float tempoDaLetra(const Corpo *c, unsigned char ch) {
    if (continuacao(ch)) {
        return 0.0f;
    }
    float t = c->tempoPorLetra;
    if (ch == ',' || ch == ';' || ch == ':') {
        t += PAUSA_NA_VIRGULA;
    } else if (ch == '.' || ch == '!' || ch == '?') {
        t += PAUSA_NO_PONTO;
    }
    return t;
}

/* A letra que está sendo dita agora (NULL: acabou) */
static const unsigned char *letraNoTempo(const Corpo *c, const char *fala, float tempo) {
    float t = 0.0f;
    for (const unsigned char *p = (const unsigned char *) fala; *p; p++) {
        t += tempoDaLetra(c, *p);
        if (tempo < t) {
            return p;
        }
    }
    return NULL;
}

/* O quanto a boca abre em cada letra: o a, o e e o o abrem; o i e o u menos; o m, o b e o p fecham */
static float aberturaDaLetra(const unsigned char *p) {
    unsigned char ch = p[0];
    if (ch == 0xC3) {   /* as acentuadas (á à â ã é ê í ó ô õ ú ç), em UTF-8 */
        unsigned char d = (unsigned char) (p[1] | 0x20);
        if (d >= 0xA0 && d <= 0xA5) {
            return 1.0f;
        }
        if (d >= 0xA8 && d <= 0xAB) {
            return 0.8f;
        }
        if (d >= 0xB2 && d <= 0xB6) {
            return 0.85f;
        }
        if ((d >= 0xAC && d <= 0xAF) || (d >= 0xB9 && d <= 0xBC)) {
            return 0.45f;
        }
        return 0.3f;
    }
    switch (tolower(ch)) {
        case 'a': return 1.0f;
        case 'o': return 0.85f;
        case 'e': return 0.8f;
        case 'i': case 'u': return 0.45f;
        case 'm': case 'b': case 'p': return 0.0f;
        default: break;
    }
    return (isalpha(ch) || ch >= 0x80) ? 0.25f : 0.0f;   /* os espaços e a pontuação: fechada */
}

/*
 * prepararFala
 * O ritmo de uma fala nova: a velocidade (cabe no tempo em que ela fica na tela, entre 9 e 16 letras
 * por segundo) e as ênfases, na hora de cada palavra que conta: as de 4 letras ou mais, as de 3 com
 * menos força, e mais forte a que vem antes de um "!". Duas mais perto que o ritmo da pessoa deixa
 * viram uma (a mais forte). Uma rubrica, "(sorri tímido)", não se fala.
 */
static void prepararFala(Corpo *c, const Jeito *j, const char *fala, float duracao) {
    int letras = 0;
    for (const unsigned char *p = (const unsigned char *) fala; *p; p++) {
        letras += !continuacao(*p);
    }
    c->tempoPorLetra = limitar(duracao * FALA_OCUPA / (float) (letras > 0 ? letras : 1),
                               1.0f / LETRAS_POR_SEGUNDO_ATE, 1.0f / LETRAS_POR_SEGUNDO_DE);
    c->totalDeBatidas = c->proximaBatida = 0;
    c->totalDeToques = c->proximoToque = 0;
    c->tempoDaFala = 0.0f;
    c->tempoDaPergunta = -1.0f;
    if (fala[0] == '(') {
        return;
    }
    float minimo = 1.0f / fmaxf(j->ritmo * RITMO_FOLGA, 0.3f);
    float total = (float) (letras > 0 ? letras : 1) * c->tempoPorLetra;
    float t = 0.0f, ultima = -10.0f, ultimoToque = -10.0f;
    int primeira = 1;
    const unsigned char *inicio = (const unsigned char *) fala;
    for (const unsigned char *p = inicio; *p; p++) {
        int comecaPalavra = !continuacao(*p) && *p != ' ' && !pontuacao(*p) && (p == inicio || p[-1] == ' ');
        if (comecaPalavra) {
            int tamanho = 0;
            const unsigned char *q = p;
            while (*q && *q != ' ' && !pontuacao(*q)) {
                tamanho += !continuacao(*q);
                q++;
            }
            /* as mãos: um gesto por palavra (as de uma letra não contam) */
            if (tamanho >= 2 && t - ultimoToque >= ENTRE_TOQUES && c->totalDeToques < MAIS_TOQUES) {
                c->toques[c->totalDeToques] = t;
                c->forcasDosToques[c->totalDeToques] = ((tamanho >= 4) ? 1.0f : 0.6f) * ((*q == '!') ? 1.4f : 1.0f);
                c->totalDeToques++;
                ultimoToque = t;
            }
            float forca = (tamanho >= 4) ? 1.0f : (tamanho == 3) ? 0.6f : 0.0f;
            if (*q == '!') {
                forca = fmaxf(forca, 0.7f) * 1.6f;
            }
            if (primeira) {
                forca *= 0.8f;   /* a primeira já teve a preparação */
            }
            if (*q != '!') {
                forca *= 1.0f - FRASE_CAI * limitar(t / total, 0.0f, 1.0f);   /* a frase vai caindo, como na fala */
            }
            primeira = 0;
            if (forca > 0.0f) {
                if (t - ultima < minimo && c->totalDeBatidas > 0) {
                    c->forcas[c->totalDeBatidas - 1] = fmaxf(c->forcas[c->totalDeBatidas - 1], forca);
                } else if (c->totalDeBatidas < MAIS_BATIDAS) {
                    c->batidas[c->totalDeBatidas] = t;
                    c->forcas[c->totalDeBatidas] = forca;
                    c->totalDeBatidas++;
                    ultima = t;
                }
            }
        }
        if (*p == '?') {
            c->tempoDaPergunta = fmaxf(0.0f, t - PERGUNTA_ANTES);
        }
        t += tempoDaLetra(c, *p);
    }
}

/*
 * enfatizar
 * Uma ênfase da fala, do tamanho da palavra e do ânimo: agitado, maior; com sono ou sem graça, menor.
 * O jeito também sai do ânimo: contente e agitado, um pulinho; bravo, um aceno seco e o corpo
 * sacudindo de lado; senão, um aceno. As rodas da frente gesticulam.
 */
static void enfatizar(Corpo *c, const Jeito *j, const Situacao *s, float forca) {
    const Animo *a = &c->animo;
    float alegre = fmaxf(a->alegria, 0.0f), chateado = fmaxf(-a->alegria, 0.0f), agit = a->agitacao;
    float g = forca * j->energia * (0.35f + 0.9f * agit) * (s->parado ? 1.0f : FALANDO_ANDANDO)
              * (1.0f - 0.5f * a->sono) * (1.0f - 0.5f * fmaxf(-a->conforto, 0.0f));
    float pulo = limitar(alegre * agit * 1.6f, 0.0f, 0.7f);   /* o aceno continua: a cabeça fala */
    float sacode = limitar(chateado * agit * 2.2f, 0.0f, 1.0f);
    gesto(c, j, ACENO, ACENO_DA_FALA * g * (1.0f - pulo) * (1.0f + 0.4f * sacode), DURA_O_ACENO);
    gesto(c, j, SUBIDA, PULO_DA_FALA * g * pulo, DURA_O_PULO);
    c->ladoDaEnfase = -c->ladoDaEnfase;
    gesto(c, j, ROLAGEM, c->ladoDaEnfase * LADO_DA_FALA * g * fminf(1.0f, j->requebro + sacode),
          DURA_O_LADO * (1.0f - 0.4f * sacode));   /* bravo, sacode mais rápido */
    if (s->parado && (forca > 1.3f || pulo > 0.5f)) {   /* a palavra forte abre as mãos */
        float dele = MAOS_DE_TODO_MUNDO + (1.0f - MAOS_DE_TODO_MUNDO) * j->maos;
        c->alvoDaAbertura = fmaxf(c->alvoDaAbertura, ABRE_NO_PONTO * dele * fminf(1.0f, 0.4f + agit));
    }
    c->alvoDosGarfos = limitar((0.3f + pulo) * (0.4f + 0.6f * agit), 0.0f, 1.0f);
    c->enfase = g;
}

/* ------------------------------------------------------------------
 * Os tiques
 * ------------------------------------------------------------------ */

/* O McQueen acelerando parado: o torque do motor torce o corpo para um lado e o agacha atrás */
static void acelerarParado(Corpo *c, const Jeito *j, float forca) {
    gesto(c, j, ROLAGEM, ACELERADA * forca, DURA_O_TIQUE);
    gesto(c, j, ACENO, -ACELERADA_AGACHA * forca, DURA_O_TIQUE * 1.5f);
}

/*
 * fazerTique
 * O jeito próprio de cada um. Cada tique nasce de uma vontade que o ânimo enche aos poucos (ou de
 * uma fala), e o tamanho dele vem do ânimo de agora: nada aqui tem hora marcada.
 */
static void fazerTique(Corpo *c, const Jeito *j, const Situacao *s, int novaFala, float tempo,
                       float *alvoRolagem, float *alvoAceno, float *alvoSubida, float *tremor, float *tremorSobe) {
    const Animo *a = &c->animo;
    float alegre = fmaxf(a->alegria, 0.0f), agit = a->agitacao, medo = fmaxf(-a->conforto, 0.0f);
    int livre = s->parado && s->fala == NULL && !c->cochilando;
    int contente = novaFala && (s->humorDaFala == HUMOR_FELIZ || s->humorDaFala == HUMOR_EMPOLGADO);
    switch (j->tique) {
        case TIQUE_ACELERA:
            /* agitado e parado, a vontade de acelerar enche; uma fala empolgada já acelera */
            if (livre) {
                c->vontade += tempo * fmaxf(0.0f, agit - 0.3f) * (0.4f + alegre) * 0.35f;
            }
            if (novaFala && s->humorDaFala == HUMOR_EMPOLGADO) {
                c->vontade += 0.8f;
            }
            if (c->vontade >= 1.0f && s->parado && c->qualTique == 0) {
                c->vontade = 0.0f;
                c->qualTique = (agit > 0.6f) ? 2 : 1;   /* bem agitado, duas vezes */
                c->tique = 0.0f;
                c->forcaDoTique = 0.6f + 0.6f * agit;
            }
            if (c->qualTique > 0) {
                c->tique -= tempo;
                if (c->tique <= 0.0f) {
                    acelerarParado(c, j, c->forcaDoTique);
                    c->qualTique--;
                    c->tique = ENTRE_ACELERADAS;
                }
            }
            break;
        case TIQUE_HIDRAULICA:
            /* relaxado, ele desce mais; contente, a vontade de bombear enche (com visita, mais) */
            *alvoSubida -= 0.02f * (1.0f - agit);
            if (livre) {
                c->vontade += tempo * alegre * (0.3f + agit + s->visita) * 0.12f;
            }
            if (contente) {
                c->vontade += 0.5f;
            }
            if (c->vontade >= 1.0f && s->parado && c->tique <= 0.0f) {
                c->vontade = 0.0f;
                c->qualTique = (agit > 0.45f) ? 0 : 1;   /* agitado, bombeia; tranquilo, só ergue um lado */
                c->tique = (c->qualTique == 0) ? 1.0f + 1.2f * agit : 2.2f;
                c->forcaDoTique = 0.6f + 0.6f * alegre;
            }
            if (c->tique > 0.0f) {
                c->tique -= tempo;
                if (c->qualTique == 0) {
                    float sobe = 0.3f + 0.7f * sinf(c->tique * 2.0f * PI_G * HIDRAULICA_HZ);
                    float entra = limitar(c->tique / 0.3f, 0.0f, 1.0f);   /* começa e acaba macio */
                    *alvoSubida += c->forcaDoTique * HIDRAULICA_SOBE * sobe * entra * entra;
                } else {
                    float t = limitar(c->tique / 2.2f, 0.0f, 1.0f);
                    float lado = (s->atento && s->lado != 0.0f) ? -copysignf(1.0f, s->lado) : 1.0f;   /* o lado de quem ele olha */
                    *alvoRolagem += lado * HIDRAULICA_DE_LADO * c->forcaDoTique * sinf(t * PI_G);
                    *alvoSubida += 0.03f * sinf(t * PI_G);
                }
            }
            break;
        case TIQUE_SENTIDO:
            /* o corpo desleixa aos poucos; quando alguém chega, ou desleixou demais, ou vai dar uma ordem, se apruma */
            if (s->parado) {
                c->desleixo = fminf(1.0f, c->desleixo + tempo / DESLEIXA_EM * (1.0f - 0.5f * agit));
            }
            *alvoAceno += 0.025f * c->desleixo;
            *alvoSubida -= 0.02f * c->desleixo;
            {
                int chegou = a->interesse > 0.5f && c->interesseAntes <= 0.5f;
                int ordem = novaFala && (s->humorDaFala == HUMOR_BRAVO || s->humorDaFala == HUMOR_SERIO);
                if (c->desleixo >= 1.0f || (chegou && c->desleixo > 0.25f) || (ordem && c->desleixo > 0.1f)) {
                    gesto(c, j, ACENO, -0.05f * (0.4f + c->desleixo), DURA_O_TIQUE * 1.5f);
                    gesto(c, j, SUBIDA, 0.04f * (0.4f + c->desleixo), DURA_O_TIQUE * 1.5f);
                    c->desleixo = 0.0f;
                }
            }
            break;
        case TIQUE_MOTOR_VELHO:
            /* o motor velho treme sempre, e mais quando ela se agita */
            *tremor += MOTOR_VELHO_ROLA * (0.7f + 0.8f * agit) * sinf(c->relogio * 2.0f * PI_G * 7.3f);
            *tremorSobe += MOTOR_VELHO_SOBE * (0.7f + 0.8f * agit) * sinf(c->relogio * 2.0f * PI_G * 11.7f + 1.0f);
            break;
        case TIQUE_GINGADO: {
            /* à vontade e contente, balança; chateado, agitado ou com medo, para */
            float quanto = GINGADO * limitar(0.3f + alegre, 0.0f, 1.0f) * (1.0f - 0.8f * agit) * (1.0f - medo)
                           * (1.0f - a->sono) * (s->parado ? 1.0f : 0.4f);
            c->faseDoGingado += tempo * 2.0f * PI_G * GINGADO_HZ * (0.8f + 0.6f * agit);
            *alvoRolagem += quanto * sinf(c->faseDoGingado);
            *alvoAceno += 0.2f * quanto * sinf(2.0f * c->faseDoGingado);
            break;
        }
        case TIQUE_CHACOALHO: {
            /* a lataria solta chacoalha quando o corpo mexe forte (um gesto, um solavanco, um susto) */
            float mexendo = fabsf(c->rolagem.v) + fabsf(c->aceno.v) + 3.0f * fabsf(c->subida.v);
            c->tique -= tempo;
            if (mexendo > CHACOALHA_A_PARTIR && c->tique <= 0.0f) {
                c->chacoalho = fminf(1.0f, mexendo / (2.0f * CHACOALHA_A_PARTIR));
                c->tique = 0.35f;
            }
            c->chacoalho *= expf(-tempo * 7.0f);
            *tremor += 0.004f * c->chacoalho * sinf(c->relogio * 2.0f * PI_G * 13.0f);
            *tremorSobe += 0.003f * c->chacoalho * sinf(c->relogio * 2.0f * PI_G * 17.3f);
            break;
        }
        case TIQUE_INQUIETO:
            /* agitado, não para: pulinhos e os garfos subindo à toa */
            if (s->parado && sortear(c, 0.0f, 1.0f) < tempo * 1.5f * agit) {
                gesto(c, j, SUBIDA, 0.02f * agit, DURA_O_PULO);
                c->alvoDosGarfos = fmaxf(c->alvoDosGarfos, sortear(c, 0.0f, 0.5f) * agit);
            }
            break;
        default:
            break;
    }
    c->interesseAntes = a->interesse;
}

/* ------------------------------------------------------------------
 * O corpo
 * ------------------------------------------------------------------ */

void iniciarCorpo(Corpo *corpo, const Jeito *jeito, unsigned int semente) {
    memset(corpo, 0, sizeof(*corpo));
    corpo->sorteio = semente * 2654435761u + 12345u;
    corpo->ladoDaEnfase = 1.0f;
    corpo->tempoDaPergunta = -1.0f;
    corpo->relogio = sortear(corpo, 0.0f, 100.0f);   /* os ciclos não começam juntos */
    for (int i = 0; i < 4; i++) {
        for (int k = 0; k < 3; k++) {
            corpo->fases[i][k] = sortear(corpo, 0.0f, 2.0f * PI_G);
        }
    }
    corpo->animo.alegria = jeito->alegria;
    corpo->animo.agitacao = jeito->agitacao;
    corpo->animo.conforto = jeito->conforto;
    corpo->vontade = sortear(corpo, 0.0f, 0.6f);
}

/*
 * mudarOAnimo
 * Os estímulos do momento empurram o ânimo, e ele volta devagar ao temperamento da pessoa (de noite,
 * mais calmo). O interesse vai para quem ele olha; o sono chega a quem está sozinho e quieto, e
 * qualquer coisa o espanta.
 * Devolve: 1 se ele acabou de acordar de um cochilo.
 */
static int mudarOAnimo(Corpo *c, const Jeito *j, const Situacao *s, int novaFala, float tempo) {
    Animo *a = &c->animo;
    int falando = (s->fala != NULL);
    if (novaFala) {
        const EfeitoDoHumor *e = &EFEITOS[s->humorDaFala];
        a->alegria += e->alegria;
        a->agitacao += e->agitacao;
        a->conforto += e->conforto;
        c->pensando = fmaxf(c->pensando, e->pensa);
        if (strstr(s->fala, "...") != NULL) {
            c->pensando = 1.0f;
        }
    }
    if (s->ouvindo) {
        float k = fminf(1.0f, tempo * CONTAGIO * j->empatia);
        a->alegria += (s->alegriaDoOutro - a->alegria) * k;
        a->agitacao += (s->agitacaoDoOutro - a->agitacao) * k * 0.6f;
    }
    a->alegria += (s->visita * (1.0f - j->timidez) * VISITA_ALEGRA - s->perigo * PERIGO_CHATEIA - s->bronca * BRONCA_IRRITA) * tempo
                  - 0.6f * s->batida;
    a->agitacao += (s->visita * VISITA_AGITA + s->perigo * PERIGO_AGITA + s->bronca * BRONCA_IRRITA + falando * FALAR_AGITA) * tempo
                   + s->batida;
    a->conforto -= (s->visita * j->timidez * VISITA_ENCABULA + s->perigo * PERIGO_ASSUSTA) * tempo + s->batida;
    float agitacaoDeSempre = j->agitacao * (1.0f - 0.5f * s->noite);
    a->alegria += (j->alegria - a->alegria) * fminf(1.0f, tempo / VOLTA_A_ALEGRIA);
    a->agitacao += (agitacaoDeSempre - a->agitacao) * fminf(1.0f, tempo / VOLTA_A_AGITACAO);
    a->conforto += (j->conforto - a->conforto) * fminf(1.0f, tempo / VOLTA_O_CONFORTO);
    a->alegria = limitar(a->alegria, -1.0f, 1.0f);
    a->agitacao = limitar(a->agitacao, 0.0f, 1.0f);
    a->conforto = limitar(a->conforto, -1.0f, 1.0f);

    float alvo = s->atento ? 0.35f + 0.65f * s->perto : 0.0f;
    if (s->ouvindo) {
        alvo = fmaxf(alvo, 0.8f);
    }
    a->interesse += (alvo - a->interesse) * fminf(1.0f, tempo * 2.5f);

    int quieto = s->parado && !falando && !s->ouvindo && s->sozinho && s->perigo < 0.05f && s->visita < 0.05f && s->batida <= 0.0f;
    if (quieto) {
        a->sono += j->sonolencia * (0.25f + 0.75f * s->noite) * (1.0f - 0.7f * a->agitacao) * tempo / SONO_CHEGA_EM;
    } else {
        a->sono -= tempo * (0.1f + 4.0f * (s->perigo + s->visita + s->batida) + ((falando || s->ouvindo) ? 0.8f : 0.0f));
    }
    a->sono = limitar(a->sono, 0.0f, 1.0f);
    if (a->sono > DORMIU) {
        c->cochilando = 1;
    } else if (c->cochilando && a->sono < ACORDOU) {
        c->cochilando = 0;
        return 1;
    }
    return 0;
}

/*
 * moverCorpo
 * Um passo do corpo de um morador.
 * Recebe: o corpo, o jeito da pessoa, o que está acontecendo, o tempo que passou (segundos) e
 *         onde pôr a pose.
 * Devolve: nada (preenche a pose).
 */
void moverCorpo(Corpo *c, const Jeito *j, const Situacao *s, float tempo, Pose *pose) {
    tempo = limitar(tempo, 0.0f, 0.1f);
    c->relogio += tempo;
    c->enfase = 0.0f;
    int falando = (s->fala != NULL);
    int novaFala = falando && s->fala != c->falaAntes;
    c->falaAntes = s->fala;
    if (novaFala) {
        prepararFala(c, j, s->fala, s->duracaoDaFala);
    }

    /* 1. O ânimo */
    int acordou = mudarOAnimo(c, j, s, novaFala, tempo);
    const Animo *a = &c->animo;
    float alegre = fmaxf(a->alegria, 0.0f), chateado = fmaxf(-a->alegria, 0.0f), medo = fmaxf(-a->conforto, 0.0f);
    float agit = a->agitacao;
    float f = j->frequencia, z = j->amortecimento;
    float moleza = limitar(SUSPENSAO_NORMAL / fmaxf(f, 0.1f), 0.5f, 2.0f);

    /* 2. A postura do ânimo */
    float alvoAceno = j->postura + BRAVO_ABAIXA * chateado * agit + MEDO_ABAIXA * medo + SONO_CAI * a->sono
                      + TRISTE_CAI * chateado * (1.0f - agit) - ORGULHO_ERGUE * alegre * agit
                      + CHEGA_PERTO * a->interesse * fmaxf(a->conforto, 0.0f);
    float alvoSubida = j->altura + ALEGRIA_SOBE * alegre + AGITACAO_SOBE * (agit - j->agitacao) - MEDO_ENCOLHE * medo
                       - SONO_BAIXA * a->sono;
    float alvoRolagem = s->ladoDoJogador * MEDO_AFASTA * medo;
    if (s->atento) {   /* para o lado de quem ele olha (com medo, para longe) */
        alvoRolagem -= s->lado * VAI_PARA_O_OUTRO * a->interesse * limitar(a->conforto + 0.3f, -1.0f, 1.0f);
    }
    float perguntando = (falando && c->tempoDaPergunta >= 0.0f && c->tempoDaFala >= c->tempoDaPergunta) ? 1.0f : 0.0f;
    molar(&c->pergunta, perguntando, MOLA_DA_PERGUNTA, tempo, 0.0f, 1.0f);
    alvoRolagem += CABECA_DE_LADO * c->pergunta.x * ((s->lado < 0.0f) ? -1.0f : 1.0f);
    c->pensando = fmaxf(0.0f, c->pensando - tempo * (falando ? 0.15f : 0.6f));

    /* 3. O involuntário, parado: a respiração, o peso de lado e o corpo que não para */
    c->faseDoRespiro += tempo * 2.0f * PI_G * (RESPIRO_CALMO_HZ + (RESPIRO_AGITADO_HZ - RESPIRO_CALMO_HZ) * agit);
    float maosParado = 0.0f;
    if (s->parado) {
        float respiro = RESPIRO * (0.6f + 0.8f * agit);
        alvoSubida += respiro * sinf(c->faseDoRespiro);
        alvoAceno += 0.6f * respiro * sinf(c->faseDoRespiro - 0.8f);
        alvoRolagem += PESO_DE_LADO * (0.5f + j->inquietude) * ruido(c, 0, 0.07f);
        float mexe = MEXIDA * j->inquietude * (0.3f + agit) * (1.0f - 0.7f * a->sono);
        alvoAceno += mexe * ruido(c, 1, 0.45f);
        alvoSubida += 0.6f * mexe * ruido(c, 2, 0.6f);
        if (s->atento) {
            float dele = MAOS_DE_TODO_MUNDO + (1.0f - MAOS_DE_TODO_MUNDO) * j->maos;
            maosParado = s->lado * MAOS_NO_OUTRO * dele * a->interesse * (falando ? 1.0f : 0.3f);
        }
        maosParado += MAOS_INQUIETAS * j->inquietude * j->maos * agit * ruido(c, 3, 0.8f) * (falando ? 0.3f : 1.0f);
        c->arrancada = 0.0f;
    } else {
        /* 4. Andando: a arrancada e a freada no bico, a curva de lado e os solavancos da rua */
        if (tempo > 0.0f) {
            float aceleracao = limitar((s->velocidade - c->velocidadeAntes) / tempo, -ARRANCADA_MAXIMA, ARRANCADA_MAXIMA);
            c->arrancada += (aceleracao - c->arrancada) * fminf(1.0f, tempo * 5.0f);
        }
        alvoAceno -= c->arrancada * ARRANCADA_NO_BICO * moleza;
        alvoRolagem -= s->volante * fminf(fabsf(s->velocidade) / 9.0f, 1.0f) * DEITA_NA_CURVA * moleza;
        c->proximoSolavanco -= tempo;
        if (c->proximoSolavanco <= 0.0f) {
            c->proximoSolavanco = sortear(c, 0.15f, 0.6f);
            float forte = fminf(fabsf(s->velocidade) / 9.0f, 1.5f) * moleza;
            gesto(c, j, SUBIDA, sortear(c, -1.0f, 1.0f) * SOLAVANCO * forte, DURA_O_SOLAVANCO);
            gesto(c, j, ROLAGEM, sortear(c, -1.0f, 1.0f) * SOLAVANCO * forte, DURA_O_SOLAVANCO);
        }
        c->alvoDasMaos = 0.0f;
        c->alvoDosGarfos = 0.0f;
    }
    c->velocidadeAntes = s->velocidade;

    /* 5. A fala: a preparação, as ênfases nas palavras e a boca nas vogais */
    float alvoBoca = 0.0f;
    if (novaFala && s->fala[0] != '(') {
        gesto(c, j, ACENO, -0.4f * ACENO_DA_FALA * j->energia * (0.35f + 0.9f * agit), DURA_A_PREPARACAO);   /* o bico sobe antes */
        if (s->parado) {
            c->alvoDasMaos = s->lado * MAOS_FALANDO * (MAOS_DE_TODO_MUNDO + (1.0f - MAOS_DE_TODO_MUNDO) * j->maos) * (0.5f + 0.5f * agit);
        }
    }
    if (falando) {
        c->tempoDaFala += tempo;
        while (c->proximaBatida < c->totalDeBatidas && c->batidas[c->proximaBatida] <= c->tempoDaFala) {
            enfatizar(c, j, s, c->forcas[c->proximaBatida]);
            c->proximaBatida++;
        }
        /* as mãos, a cada palavra: um lado de cada vez, puxando para quem ouve, do tamanho da palavra */
        while (c->proximoToque < c->totalDeToques && c->toques[c->proximoToque] <= c->tempoDaFala) {
            if (s->parado) {
                float dele = MAOS_DE_TODO_MUNDO + (1.0f - MAOS_DE_TODO_MUNDO) * j->maos;
                c->ladoDaMao = (c->ladoDaMao > 0.0f) ? -1.0f : 1.0f;
                float vai = c->ladoDaMao * MAOS_FALANDO * sortear(c, 0.6f, 1.0f) * fminf(c->forcasDosToques[c->proximoToque], 1.3f);
                c->alvoDasMaos = limitar((0.5f * s->lado * MAOS_NO_OUTRO + vai) * dele * (0.5f + 0.7f * agit)
                                         * (1.0f - 0.5f * fmaxf(-a->conforto, 0.0f)), -1.0f, 1.0f);   /* sem graça, gesticula menos */
            }
            c->proximoToque++;
        }
        const unsigned char *letra = letraNoTempo(c, s->fala, c->tempoDaFala);
        if (letra != NULL && s->fala[0] != '(') {
            alvoBoca = aberturaDaLetra(letra) * j->boca * (0.5f + 0.5f * agit);
        }
    }
    molar(&c->boca, alvoBoca, MOLA_DA_BOCA, tempo, 0.0f, 1.2f);
    c->alvoDasMaos += (maosParado - c->alvoDasMaos) * fminf(1.0f, tempo * MAOS_VOLTAM);
    if (s->parado) {   /* a pergunta abre as mãos no fim ("e aí?"); o resto vai fechando */
        c->alvoDaAbertura = fmaxf(c->alvoDaAbertura, ABRE_NA_PERGUNTA * c->pergunta.x * (0.6f + 0.4f * j->maos));
    }
    c->alvoDaAbertura = s->parado ? c->alvoDaAbertura * expf(-tempo * 1.8f) : 0.0f;
    c->alvoDosGarfos += (0.0f - c->alvoDosGarfos) * fminf(1.0f, tempo * MAOS_VOLTAM * 0.5f);

    /* 6. Ouvindo: o "uhum" logo depois da ênfase de quem fala, se ele presta atenção e concorda */
    if (s->ouvindo && s->parado && s->enfaseDoOutro > 0.0f && c->uhum <= 0.0f) {
        float chance = a->interesse * (0.3f + 0.5f * alegre) * (0.4f + 0.8f * j->empatia) * (1.0f - medo);
        if (sortear(c, 0.0f, 1.0f) < chance) {
            c->uhum = sortear(c, UHUM_DEPOIS_DE, UHUM_ATE);
            c->forcaDoUhum = fminf(s->enfaseDoOutro, 1.5f);
        }
    }
    if (c->uhum > 0.0f) {
        c->uhum -= tempo;
        if (c->uhum <= 0.0f) {
            gesto(c, j, ACENO, UHUM * c->forcaDoUhum * j->energia * (0.5f + agit), DURA_O_UHUM);
        }
    }

    /* 7. O reflexo do susto, na medida do perigo (os tímidos se assustam antes); o tremor do medo; o ar */
    c->refratario -= tempo;
    float limiar = SUSTO_A_PARTIR * (1.0f - 0.5f * j->timidez);
    float susto = 0.0f, deOnde = s->ladoDoJogador;
    if (s->batida > 0.0f) {
        susto = fmaxf(s->batida, 0.5f);
    } else if (s->perigo > limiar && c->perigoAntes <= limiar && c->refratario <= 0.0f) {
        susto = s->perigo;
    }
    if (acordou && susto <= 0.0f) {
        susto = 0.5f;   /* "hã? o quê?" */
        deOnde = 0.0f;
    }
    c->perigoAntes = s->perigo;
    if (susto > 0.0f) {
        gesto(c, j, SUBIDA, PULO_DO_SUSTO * susto, DURA_O_SUSTO);
        gesto(c, j, ACENO, -RECUO_DO_SUSTO * susto, DURA_O_SUSTO * 1.5f);
        gesto(c, j, ROLAGEM, DESVIA_DO_SUSTO * susto * deOnde, DURA_O_SUSTO * 2.0f);
        c->espanto = fmaxf(c->espanto, susto);
        c->refratario = SUSTO_DE_NOVO;
    }
    c->espanto = fmaxf(0.0f, c->espanto - tempo * 0.8f);
    float tremendo = limitar((medo - 0.45f) / 0.55f, 0.0f, 1.0f);   /* só o medo de verdade treme; a timidez, não */
    float tremor = TREMOR * tremendo * tremendo * sinf(c->relogio * 2.0f * PI_G * TREMOR_HZ), tremorSobe = 0.0f;
    c->rolagem.v += s->vento * VENTO * tempo;

    /* 8. Os tiques */
    fazerTique(c, j, s, novaFala, tempo, &alvoRolagem, &alvoAceno, &alvoSubida, &tremor, &tremorSobe);

    /* 9. As molas */
    alvoRolagem += pulsosAgora(c, ROLAGEM);
    alvoAceno += pulsosAgora(c, ACENO);
    alvoSubida += pulsosAgora(c, SUBIDA);
    molar(&c->rolagem, alvoRolagem, f, z, tempo, -3.0f * ROLAGEM_MAXIMA, 3.0f * ROLAGEM_MAXIMA);
    molar(&c->aceno, alvoAceno, f, z, tempo, -3.0f * ACENO_MAXIMO, 3.0f * ACENO_MAXIMO);
    molar(&c->subida, alvoSubida, f, z, tempo, -3.0f * SUBIDA_MAXIMA, 3.0f * SUBIDA_MAXIMA);
    molar(&c->maos, limitar(c->alvoDasMaos, -1.0f, 1.0f), MOLA_DAS_MAOS, tempo, -1.0f, 1.0f);
    molar(&c->garfos, limitar(c->alvoDosGarfos, 0.0f, 1.0f), MOLA_DOS_GARFOS, tempo, -0.2f, 1.2f);
    molar(&c->abreAsMaos, limitar(c->alvoDaAbertura, 0.0f, 1.0f), MOLA_DA_ABERTURA, tempo, -0.2f, 1.2f);

    pose->rolagem = macio(c->rolagem.x, ROLAGEM_MAXIMA) + tremor;
    pose->aceno = macio(c->aceno.x, ACENO_MAXIMO);
    pose->subida = macio(c->subida.x, SUBIDA_MAXIMA) + tremorSobe;
    pose->maos = c->maos.x;
    pose->abreAsMaos = limitar(c->abreAsMaos.x, 0.0f, 1.0f);
    pose->garfos = limitar(c->garfos.x, 0.0f, 1.0f);
    pose->boca = limitar(c->boca.x, 0.0f, 1.0f);

    /* 10. Os olhos, do ânimo: contente sorri; chateado e agitado é raiva; chateado e parado, tristeza;
     * com medo, as sobrancelhas de preocupado e o olhar para baixo; calmo, as pálpebras caem */
    Emocao *e = &pose->olhos;
    e->alegria = alegre * (0.6f + 0.4f * agit);
    e->raiva = chateado * limitar(agit * 1.6f, 0.0f, 1.0f) * (1.0f - 0.7f * medo);
    e->tristeza = limitar(chateado * (1.0f - agit) * 0.9f + medo * 0.6f, 0.0f, 1.0f);
    e->espanto = limitar(0.4f * limitar((agit - 0.7f) / 0.3f, 0.0f, 1.0f) + c->espanto + 0.6f * c->pergunta.x, 0.0f, 1.0f);
    e->sono = c->cochilando ? 1.0f : limitar(a->sono * 1.1f + limitar(0.35f - agit, 0.0f, 0.35f) * 1.2f, 0.0f, 1.0f);
    e->olharParaCima = limitar(-0.8f * medo - 0.4f * e->tristeza + 0.6f * c->pensando - 0.4f * a->sono + 0.1f * alegre * agit,
                               -1.0f, 1.0f);
}
