/*
 * vibracao.c
 * ------------------------------------------------------------------
 * As sensações do controle. Veja vibracao.h.
 *
 * A regra é o silêncio. Uma vibração que não para (a textura do asfalto o
 * tempo todo, um toque a cada troca de marcha, o peso da freada) deixa de
 * dizer qualquer coisa e só cansa a mão: medida numa corrida, a de antes
 * vibrava 100% do tempo. Agora, no asfalto em linha reta, o controle fica
 * quieto, e o que vibra é o que o piloto precisa sentir:
 *   - o chão mudando: a zebra (os relevos, no ritmo da velocidade), a grama,
 *     a terra e a areia (um chão irregular);
 *   - o pneu: no limite, um tremor irregular e leve;
 *   - o drift: um tranco quando a traseira solta, o pneu arrastando de lado
 *     do começo ao fim (o peso na mão de fora e o tremor da borracha), e
 *     outro toque quando ele agarra;
 *   - a arrancada (a roda patinando) e o nitro (o empurrão, depois um ronco
 *     baixo, e engasgando quando o tanque acaba);
 *   - as batidas: noutro carro ou no muro, um tranco quando o contato começa,
 *     que sempre se sente e cresce com a força; encostado noutro carro, a
 *     lataria raspando; no muro, ele raspando; e a rodada sacudindo;
 *   - na corrida, a contagem, a largada, a chegada e a pistola do Guido.
 * E de propósito NÃO vibram: trocar de marcha (o câmbio é automático),
 * frear, ultrapassar e andar reto no asfalto. O piloto já sabe disso.
 *
 * Três cuidados fazem a vibração dizer mais com menos:
 *   - o lado: a zebra, a grama, o muro, as batidas e o pneu estourado vibram
 *     mais (30%) na mão do lado em que acontecem (a mão esquerda é o motor
 *     grave, a direita o agudo);
 *   - a mão se acostuma: o que dura (a grama, a terra, o ronco do nitro)
 *     começa marcado e vai assentando em ~1,5 s, sem sumir; o drift assenta só
 *     um pouco, porque é para sentir do começo ao fim;
 *   - a força é a do que aconteceu: a batida leve se sente, a forte bem mais;
 *     a vitória ganha três pulsos de comemoração.
 * Cada sensação pede uma quantidade de vibração grave e aguda, e elas se
 * somam sem passar do máximo. Os pulsos ficam abaixo de uns 16 por segundo:
 * acima disso o controle não acompanha, e viram um zumbido sem forma.
 * ------------------------------------------------------------------
 */

#include <math.h>
#include <stddef.h>

#include "vibracao.h"
#include "pitstop.h"

#define INTENSIDADE_GERAL      0.85f   /* o volume de toda a vibração */

/* ---------- O chão ---------- */
typedef enum { SENTE_ASFALTO, SENTE_ZEBRA, SENTE_GRAMA, SENTE_TERRA, SENTE_AREIA, SENTE_ROCHA, SENTE_NO_AR } ChaoSentido;
#define INICIO_DA_ZEBRA        0.93f   /* em larguras: a roda de fora já pisa nela */
#define FIM_DA_ZEBRA           1.15f
#define METROS_ENTRE_RELEVOS   1.8f
#define PULSOS_MAXIMOS_DA_ZEBRA 13.0f  /* por segundo */
#define ZEBRA_GRAVE            0.34f
#define ZEBRA_AGUDO            0.16f
#define GRAMA_GRAVE            0.36f   /* sair da pista é dos avisos que mais importam */
#define GRAMA_AGUDO            0.16f
#define TERRA_GRAVE            0.16f
#define TERRA_AGUDO            0.05f
#define AREIA_GRAVE            0.20f
#define ROCHA_GRAVE            0.26f
#define ROCHA_AGUDO            0.12f

/* ---------- O pneu ---------- */
#define INICIO_DO_PATINAR      0.45f   /* derrapagem abaixo disso não se sente: só o pneu no limite */
#define TREMOR_DO_PNEU         0.24f
#define BATIDAS_DO_TREMOR      12.0f   /* por segundo, em média */
#define DRIFT_GRAVE            0.30f   /* de lado, o pneu arrastando: o peso na mão de fora... */
#define DRIFT_AGUDO            0.24f   /* ...e o tremor irregular da borracha raspando */
#define DRIFT_ASSENTA_ATE      0.80f   /* o drift é para sentir do começo ao fim: assenta só um pouco */
#define MAO_DE_DENTRO          0.35f   /* a mão de dentro da curva sente só uma parte */
#define GOLPE_QUEBRA_TRASEIRA  0.34f   /* a traseira solta */
#define GOLPE_PNEU_AGARRA      0.22f   /* e o pneu agarra de novo */
#define DRIFT_LIGADO           0.50f
#define DRIFT_ACABANDO         0.08f
#define RAPIDEZ_DA_ARRANCADA   0.13f   /* da velocidade máxima: abaixo disso, a roda ainda patina... */
#define PARADO                 0.03f   /* ...saindo de quase parado... */
#define SEGUNDOS_DA_ARRANCADA  1.2f    /* ...por no máximo tanto (e só de novo depois de soltar o acelerador) */
#define ARRANCADA_AGUDO        0.16f
#define ARRANCADA_GRAVE        0.06f
#define INICIO_DO_PNEU_GASTO   0.75f
#define PNEU_GASTO             0.14f   /* o pneu acabado escorrega antes: no volante ele já avisa */

/* ---------- Nitro: o empurrão, um ronco baixo e o tanque acabando ---------- */
#define GOLPE_DO_NITRO_GRAVE   0.42f
#define GOLPE_DO_NITRO_AGUDO   0.16f
#define NITRO_GRAVE            0.07f
#define NITRO_PULSO            0.05f   /* a combustão pulsando */
#define PULSOS_DO_NITRO        10.0f
#define NITRO_NO_FIM           0.12f   /* com tão pouco no tanque, o nitro engasga */
#define ENGASGOS_DO_NITRO      6.0f    /* por segundo */
#define ENGASGO                0.16f
#define CAUDA_DO_NITRO         0.10f
#define SUMICO_DA_CAUDA        7.0f    /* por segundo */

/* ---------- Trancos, que somem depressa ---------- */
#define SUMICO_DO_GOLPE        11.0f   /* por segundo */
#define RESTO                  0.02f   /* abaixo disso, a vibração é zero: um resto apagando não fica ligado */
#define MENOR_PANCADA          0.08f   /* um toque mais leve que isso não vibra */
#define PANCADA_GRAVE          0.90f   /* a pancada cheia */
#define PANCADA_AGUDO          0.60f
#define MENOR_TRANCO           0.28f   /* da pancada cheia: até a batida leve num carro se sente */
#define LATARIA_GRAVE          0.14f   /* encostado noutro carro, a lataria raspando */
#define LATARIA_AGUDO          0.24f
#define BATIDA_NO_MURO         0.25f   /* chegando no muro: isto, mais a rapidez */
#define GOLPE_DA_CONTAGEM      0.16f
#define GOLPE_DA_LARGADA       0.42f
#define GOLPE_DA_CHEGADA       0.50f
#define GOLPE_DO_MACACO        0.24f

/* ---------- O lado e a mão que se acostuma ---------- */
#define PESO_DO_LADO           0.30f   /* do lado do que acontece, a mão sente 30% a mais; do outro, 30% a menos (mais que
                                          isso, a sensação mudaria de motor: o da esquerda é o pesado, o da direita o leve) */
#define ASSENTA_EM             1.5f    /* segundos: o que dura vai assentando... */
#define ASSENTA_ATE            0.55f   /* ...até esta parte do começo */

/* ---------- O pneu estourado, a entrada nos boxes e a vitória ---------- */
#define GOLPE_DO_ESTOURO       0.60f
#define ARO_GRAVE              0.16f   /* o aro raspando no asfalto */
#define ARO_AGUDO              0.12f
#define GOLPE_DO_PIT           0.12f   /* entrando no pit lane: o piloto automático assumiu */
#define PULSOS_DA_VITORIA      2       /* depois do tranco da chegada */
#define ENTRE_OS_PULSOS        0.22f   /* segundos */
#define PULSO_DA_VITORIA       0.34f

/* ---------- O resto ---------- */
#define MURO_GRAVE             0.20f
#define MURO_AGUDO             0.28f
#define RODADA_GRAVE           0.32f
#define RODADA_AGUDO           0.18f
#define PULSOS_DA_PISTOLA      16.0f   /* a pistola de parafusos do Guido */
#define PISTOLA_AGUDO          0.30f
#define PISTOLA_GRAVE          0.08f

/*
 * somar
 * Junta duas vibrações sem passar de 1: quanto mais forte já está, menos a outra soma.
 * Recebe: as duas.
 * Devolve: a soma.
 */
static float somar(float a, float b) {
    b = fminf(fmaxf(b, 0.0f), 1.0f);
    return 1.0f - (1.0f - a) * (1.0f - b);
}

/*
 * pulso
 * Recebe: a fase, em ciclos.
 * Devolve: 1 na primeira metade de cada ciclo, 0 na outra.
 */
static float pulso(float fase) {
    return (fase - floorf(fase) < 0.5f) ? 1.0f : 0.0f;
}

/*
 * asperezaDoChao
 * Um chão irregular: a mesma posição sempre dá o mesmo valor, mas sem ritmo que se perceba.
 * Recebe: a fase do chão, em metros.
 * Devolve: de 0 a 1.
 */
static float asperezaDoChao(float metros) {
    float a = sinf(metros * 2.3f) + sinf(metros * 3.7f + 1.0f) + sinf(metros * 6.1f + 2.0f);
    return (a + 3.0f) / 6.0f;
}

/*
 * tremorIrregular
 * O pneu escorregando agarra e solta sem ritmo fixo: as batidas variam de força.
 * Recebe: o relógio.
 * Devolve: de 0 a 1.
 */
static float tremorIrregular(float relogio) {
    float batida = pulso(relogio * BATIDAS_DO_TREMOR + 0.35f * sinf(relogio * 5.3f));
    return batida * (0.6f + 0.4f * asperezaDoChao(relogio * 9.0f)) + (1.0f - batida) * 0.25f;
}

/*
 * iniciarEstadoDaVibracao
 * Recebe: o estado, no começo de cada corrida (e ao entrar no mundo aberto).
 * Devolve: nada.
 */
void iniciarEstadoDaVibracao(EstadoDaVibracao *estado) {
    estado->relogio = 0.0f;
    estado->faseDoChao = 0.0f;
    estado->golpeGrave = 0.0f;
    estado->golpeAgudo = 0.0f;
    estado->caudaDoNitro = 0.0f;
    estado->tempoComNitro = 0.0f;
    estado->nitroAntes = 0;
    estado->driftAntes = 0;
    estado->estadoDoPitAntes = PIT_FORA;
    estado->numeroDaContagemAntes = -1;
    estado->terminouAntes = 0;
    estado->pneusEstouradosAntes = 0;
    estado->segundosForaDoAsfalto = 0.0f;
    estado->segundosDeLado = 0.0f;
    estado->pulsosDaVitoria = 0;
    estado->proximoPulso = 0.0f;
    estado->arrancando = 0;
    estado->arrancadaArmada = 1;
    estado->segundosDeArrancada = 0.0f;
    estado->emBatida = 0;
    estado->noMuroAntes = 0;
}

/*
 * semResto
 * O que é fraco demais para sentir vira zero: os trancos somem aos poucos e nunca chegam a zero sozinhos, e um
 * resto deles prendia o motor do controle ligado, bem fraco, para sempre.
 * Recebe: a vibração.
 * Devolve: ela, sem os restos.
 */
static Vibracao semResto(Vibracao v) {
    v.grave = (v.grave < RESTO) ? 0.0f : v.grave;
    v.agudo = (v.agudo < RESTO) ? 0.0f : v.agudo;
    v.noAcelerador = (v.noAcelerador < RESTO) ? 0.0f : v.noAcelerador;
    return v;
}

/*
 * assentar
 * O que dura vai assentando: forte no começo, ASSENTA_ATE dele depois de uns segundos.
 * Recebe: há quantos segundos a sensação dura.
 * Devolve: o fator (de 1 a ASSENTA_ATE).
 */
static float assentar(float segundos) {
    return ASSENTA_ATE + (1.0f - ASSENTA_ATE) * expf(-segundos / ASSENTA_EM);
}

/*
 * somarDoLado
 * Uma sensação que acontece de um lado do carro: a mão desse lado sente mais.
 * Recebe: as vibrações a somar, o grave e o agudo da sensação e o lado (-1 esquerda, +1 direita, 0 no meio).
 * Devolve: nada.
 */
static void somarDoLado(float *grave, float *agudo, float daGrave, float daAgudo, float lado) {
    lado = fminf(fmaxf(lado, -1.0f), 1.0f);
    *grave = somar(*grave, daGrave * (1.0f - PESO_DO_LADO * lado));
    *agudo = somar(*agudo, daAgudo * (1.0f + PESO_DO_LADO * lado));
}

/*
 * darGolpe
 * Recebe: o estado, e a força do tranco no grave e no agudo.
 * Devolve: nada.
 */
static void darGolpe(EstadoDaVibracao *estado, float grave, float agudo) {
    estado->golpeGrave = fmaxf(estado->golpeGrave, grave);
    estado->golpeAgudo = fmaxf(estado->golpeAgudo, agudo);
}

/*
 * baterNaVibracao
 * Uma batida, na força dela: abaixo de MENOR_PANCADA (um raspão de nada), nada; acima, um tranco que se sente
 * sempre (MENOR_TRANCO da pancada cheia) e cresce com a força, mais forte na mão do lado em que bateu.
 * Recebe: o estado, a força da batida (0 a 1) e o lado (-1 esquerda, +1 direita, 0 de frente ou sem saber).
 * Devolve: nada.
 */
static void baterNaVibracao(EstadoDaVibracao *estado, float pancada, float lado) {
    if (pancada < MENOR_PANCADA) {
        return;
    }
    float forca = MENOR_TRANCO + (1.0f - MENOR_TRANCO) * fminf((pancada - MENOR_PANCADA) / (1.0f - MENOR_PANCADA), 1.0f);
    lado = fminf(fmaxf(lado, -1.0f), 1.0f) * PESO_DO_LADO;
    darGolpe(estado, fminf(PANCADA_GRAVE * forca * (1.0f - lado), 1.0f), fminf(PANCADA_AGUDO * forca * (1.0f + lado), 1.0f));
}

/*
 * O que o carro passa para as mãos (o mesmo na corrida e no mundo aberto)
 */
typedef struct {
    const Carro *carro;      /* o carro da corrida (no mundo aberto, o que dirige o carro livre) */
    float metrosPorSegundo;  /* a velocidade, para a frente */
    float rapidez;           /* 0 a 1: da velocidade máxima */
    ChaoSentido chao;
    float ladoDoChao;        /* -1 a +1: de que lado está a zebra ou a grama (0 no meio ou sem saber) */
    int raspando;            /* encostando num muro ou numa parede */
    float ladoDoMuro;        /* -1 a +1: de que lado está o muro */
} CarroSentido;

/*
 * sentirOCarro
 * O chão, o pneu, a arrancada, o nitro, o muro e a rodada.
 * Recebe: o estado, o carro, o tempo do quadro e as vibrações a somar.
 * Devolve: nada (soma em grave, agudo e noAcelerador).
 */
static void sentirOCarro(EstadoDaVibracao *estado, const CarroSentido *sentido, float tempoQuadro,
                         float *grave, float *agudo, float *noAcelerador) {
    const Carro *carro = sentido->carro;
    float relogio = estado->relogio;
    float rapidez = sentido->rapidez;
    int andando = sentido->metrosPorSegundo > 0.5f;

    /* A traseira soltando e agarrando de novo: um toque cada */
    if (!estado->driftAntes && carro->forcaDoDrift > DRIFT_LIGADO) {
        darGolpe(estado, GOLPE_QUEBRA_TRASEIRA, GOLPE_QUEBRA_TRASEIRA * 0.5f);
        estado->driftAntes = 1;
    } else if (estado->driftAntes && carro->forcaDoDrift < DRIFT_ACABANDO) {
        darGolpe(estado, GOLPE_PNEU_AGARRA, GOLPE_PNEU_AGARRA * 0.5f);
        estado->driftAntes = 0;
    }
    /* O nitro: o empurrão ao ligar */
    if (carro->nitroLigado && !estado->nitroAntes) {
        darGolpe(estado, GOLPE_DO_NITRO_GRAVE, GOLPE_DO_NITRO_AGUDO);
        estado->tempoComNitro = 0.0f;
    } else if (!carro->nitroLigado && estado->nitroAntes) {
        estado->caudaDoNitro = 1.0f;
    }
    estado->nitroAntes = carro->nitroLigado;

    /* O chão: no asfalto, nada; fora dele, o relevo ou a aspereza de cada um, do lado em que está. O que é
     * áspero (grama, terra, areia, rocha) vai assentando; a zebra, que passa logo, não */
    int foraDoAsfalto = andando && sentido->chao != SENTE_ASFALTO && sentido->chao != SENTE_ZEBRA && sentido->chao != SENTE_NO_AR;
    estado->segundosForaDoAsfalto = foraDoAsfalto ? estado->segundosForaDoAsfalto + tempoQuadro : 0.0f;
    if (andando) {
        float aspereza = asperezaDoChao(estado->faseDoChao);
        float forca = fminf(0.3f + rapidez * 1.5f, 1.0f) * assentar(estado->segundosForaDoAsfalto);
        float lado = sentido->ladoDoChao;
        switch (sentido->chao) {
            case SENTE_ZEBRA: {
                float porSegundo = fminf(sentido->metrosPorSegundo / METROS_ENTRE_RELEVOS, PULSOS_MAXIMOS_DA_ZEBRA);
                float relevo = pulso(relogio * porSegundo);
                float naZebra = 0.6f + 0.4f * rapidez;
                somarDoLado(grave, agudo, ZEBRA_GRAVE * naZebra * relevo, ZEBRA_AGUDO * naZebra * relevo, lado);
                break;
            }
            case SENTE_GRAMA:
                somarDoLado(grave, agudo, GRAMA_GRAVE * forca * (0.4f + 0.6f * aspereza), GRAMA_AGUDO * forca * aspereza, lado);
                break;
            case SENTE_TERRA:
                somarDoLado(grave, agudo, TERRA_GRAVE * forca * (0.4f + 0.6f * aspereza), TERRA_AGUDO * forca * aspereza, lado);
                break;
            case SENTE_AREIA:
                somarDoLado(grave, agudo, AREIA_GRAVE * forca * (0.6f + 0.4f * asperezaDoChao(estado->faseDoChao * 0.5f)), 0.0f, lado);
                break;
            case SENTE_ROCHA:
                somarDoLado(grave, agudo, ROCHA_GRAVE * forca * aspereza, ROCHA_AGUDO * forca * aspereza, lado);
                break;
            default:
                break;
        }
    }
    if (sentido->chao == SENTE_NO_AR) {
        return;   /* no ar, o pneu não sente nada (a chegada ao chão é uma batida) */
    }

    /* O pneu no limite: um tremor irregular e leve na mão direita */
    float escorregando = 0.0f;
    if (carro->derrapagem > INICIO_DO_PATINAR && carro->forcaDoDrift < DRIFT_LIGADO) {
        escorregando = (carro->derrapagem - INICIO_DO_PATINAR) / (1.0f - INICIO_DO_PATINAR);
        *agudo = somar(*agudo, TREMOR_DO_PNEU * escorregando * tremorIrregular(relogio) * (0.45f + 0.55f * rapidez));
    }
    /* De lado, o pneu arrasta: o peso do carro na mão de fora da curva e o tremor da borracha raspando, do começo
     * ao fim do drift (mais forte quanto mais de lado e mais rápido; assenta só um pouco) */
    estado->segundosDeLado = (carro->forcaDoDrift > 0.0f && andando) ? estado->segundosDeLado + tempoQuadro : 0.0f;
    if (carro->forcaDoDrift > 0.0f && andando) {
        float assenta = DRIFT_ASSENTA_ATE + (1.0f - DRIFT_ASSENTA_ATE) * expf(-estado->segundosDeLado / ASSENTA_EM);
        float deLado = fminf(fabsf(carro->anguloDeDrift) / 30.0f, 1.0f) * carro->forcaDoDrift * assenta * (0.55f + 0.45f * rapidez);
        float borracha = tremorIrregular(relogio);
        int viraParaADireita = (carro->anguloDeDrift > 0.0f);   /* a traseira vai para a esquerda */
        *grave = somar(*grave, DRIFT_GRAVE * deLado * (0.75f + 0.25f * borracha) * (viraParaADireita ? 1.0f : MAO_DE_DENTRO));
        *agudo = somar(*agudo, DRIFT_AGUDO * deLado * borracha * (viraParaADireita ? MAO_DE_DENTRO : 1.0f));
    }
    /* O pneu gasto avisa no volante */
    if (carro->desgaste > INICIO_DO_PNEU_GASTO && fabsf(carro->volante) > 0.3f) {
        float gasto = (carro->desgaste - INICIO_DO_PNEU_GASTO) / (1.0f - INICIO_DO_PNEU_GASTO);
        *agudo = somar(*agudo, PNEU_GASTO * gasto * fabsf(carro->volante) * rapidez * tremorIrregular(relogio));
    }
    /* A roda patinando na arrancada: só saindo do carro parado, por pouco tempo, e de novo só depois de soltar o
     * acelerador (andando devagar pela cidade, ou encostado numa parede acelerando, não vibra o tempo todo) */
    int acelerando = carro->ultimosComandos.acelerar;
    if (!acelerando) {
        estado->arrancadaArmada = 1;
        estado->arrancando = 0;
    } else if (!estado->arrancando && estado->arrancadaArmada && rapidez < PARADO) {
        estado->arrancando = 1;
        estado->arrancadaArmada = 0;
        estado->segundosDeArrancada = 0.0f;
    }
    float patinando = 0.0f;
    if (estado->arrancando) {
        estado->segundosDeArrancada += tempoQuadro;
        if (rapidez >= RAPIDEZ_DA_ARRANCADA || estado->segundosDeArrancada > SEGUNDOS_DA_ARRANCADA) {
            estado->arrancando = 0;
        } else {
            patinando = (1.0f - rapidez / RAPIDEZ_DA_ARRANCADA) * (1.0f - estado->segundosDeArrancada / SEGUNDOS_DA_ARRANCADA);
            *agudo = somar(*agudo, ARRANCADA_AGUDO * patinando);
            *grave = somar(*grave, ARRANCADA_GRAVE * patinando);
        }
    }
    *noAcelerador = fmaxf(patinando, carro->ultimosComandos.acelerar ? escorregando : 0.0f);

    /* O nitro: um ronco baixo, e engasgando quando o tanque acaba */
    if (carro->nitroLigado) {
        estado->tempoComNitro += tempoQuadro;
        if (carro->nitro < NITRO_NO_FIM) {
            *grave = somar(*grave, ENGASGO * pulso(relogio * ENGASGOS_DO_NITRO));
        } else {
            float ronco = assentar(estado->tempoComNitro);
            *grave = somar(*grave, (NITRO_GRAVE + NITRO_PULSO * pulso(relogio * PULSOS_DO_NITRO)) * ronco);
        }
    } else if (estado->caudaDoNitro > 0.0f) {
        *grave = somar(*grave, CAUDA_DO_NITRO * estado->caudaDoNitro);
        estado->caudaDoNitro *= expf(-SUMICO_DA_CAUDA * tempoQuadro);
        if (estado->caudaDoNitro < 0.01f) {
            estado->caudaDoNitro = 0.0f;
        }
    }

    /* O muro raspa (do lado dele) e a rodada sacode */
    if (sentido->raspando && andando) {
        float raspando = 0.5f + 0.5f * asperezaDoChao(estado->faseDoChao * 3.0f);
        somarDoLado(grave, agudo, MURO_GRAVE * raspando, MURO_AGUDO * raspando, sentido->ladoDoMuro);
    }
    if (carro->rodando) {
        float sacudindo = 0.5f + 0.5f * asperezaDoChao(relogio * 20.0f);
        *grave = somar(*grave, RODADA_GRAVE * sacudindo);
        *agudo = somar(*agudo, RODADA_AGUDO * sacudindo);
    }
}

/*
 * somarOsGolpes
 * Os trancos guardados entram na vibração e vão sumindo.
 */
static void somarOsGolpes(EstadoDaVibracao *estado, float tempoQuadro, float *grave, float *agudo) {
    *grave = somar(*grave, estado->golpeGrave);
    *agudo = somar(*agudo, estado->golpeAgudo);
    float some = expf(-SUMICO_DO_GOLPE * tempoQuadro);
    estado->golpeGrave *= some;
    estado->golpeAgudo *= some;
    estado->golpeGrave = (estado->golpeGrave < 0.005f) ? 0.0f : estado->golpeGrave;
    estado->golpeAgudo = (estado->golpeAgudo < 0.005f) ? 0.0f : estado->golpeAgudo;
}

/*
 * verOsMomentosDaCorrida
 * A contagem, a largada, a chegada e o macaco dos boxes.
 * Recebe: o estado e a corrida.
 * Devolve: nada.
 */
static void verOsMomentosDaCorrida(EstadoDaVibracao *estado, Corrida *corrida, float tempoQuadro) {
    Carro *carro = &corrida->jogador;
    if (carro->estadoDoPit != (EstadoDoPit) estado->estadoDoPitAntes) {
        if (carro->estadoDoPit == PIT_TROCANDO || carro->estadoDoPit == PIT_SAINDO) {
            darGolpe(estado, GOLPE_DO_MACACO, GOLPE_DO_MACACO * 0.4f);   /* o carro sobe e desce no macaco */
        } else if (carro->estadoDoPit == PIT_CHEGANDO) {
            darGolpe(estado, GOLPE_DO_PIT, GOLPE_DO_PIT * 0.5f);   /* o piloto automático assumiu */
        }
    }
    estado->estadoDoPitAntes = carro->estadoDoPit;

    /* Um pneu estourando: o tranco do lado dele (bits 0 e 2 à esquerda, 1 e 3 à direita) */
    int novos = carro->pneuEstourado & ~estado->pneusEstouradosAntes;
    if (novos) {
        float lado = ((novos & 0x5) ? -1.0f : 0.0f) + ((novos & 0xA) ? 1.0f : 0.0f);
        baterNaVibracao(estado, GOLPE_DO_ESTOURO + MENOR_PANCADA, lado);
    }
    estado->pneusEstouradosAntes = carro->pneuEstourado;

    /* A vitória: depois do tranco da chegada, mais dois pulsos */
    if (estado->pulsosDaVitoria > 0) {
        estado->proximoPulso -= tempoQuadro;
        if (estado->proximoPulso <= 0.0f) {
            darGolpe(estado, PULSO_DA_VITORIA, PULSO_DA_VITORIA * 0.7f);
            estado->pulsosDaVitoria--;
            estado->proximoPulso = ENTRE_OS_PULSOS;
        }
    }

    /* Um toque a cada número da contagem, e um tranco na largada */
    int numero = (corrida->contagemRegressiva > 0.0f) ? (int) ceilf(corrida->contagemRegressiva) : 0;
    if (numero != estado->numeroDaContagemAntes && estado->numeroDaContagemAntes >= 0) {
        if (numero == 0) {
            darGolpe(estado, GOLPE_DA_LARGADA, GOLPE_DA_LARGADA * 0.5f);
        } else if (numero <= 3) {
            darGolpe(estado, GOLPE_DA_CONTAGEM, 0.0f);
        }
    }
    estado->numeroDaContagemAntes = numero;

    if (corrida->terminou && !estado->terminouAntes) {
        darGolpe(estado, GOLPE_DA_CHEGADA, GOLPE_DA_CHEGADA * 0.6f);
        if (jogadorVenceu(corrida)) {
            estado->pulsosDaVitoria = PULSOS_DA_VITORIA;
            estado->proximoPulso = ENTRE_OS_PULSOS;
        }
    }
    estado->terminouAntes = corrida->terminou;
}

/*
 * ladoDoContato
 * De que lado do carro foi o contato deste quadro (com outro carro ou com o muro).
 * Recebe: o carro.
 * Devolve: -1 (esquerda) a +1 (direita); 0 de frente, de trás ou sem contato.
 */
static float ladoDoContato(const Carro *carro) {
    if (carro->contato <= 0.0f || carro->posicao.segmento == NULL) {
        return 0.0f;
    }
    PontoNoMundo aqui = pontoNoMundo(carro->posicao, carro->deslocamentoLateral);
    float rumo = (carro->temRumoNoMundo ? carro->rumoNoMundo : aqui.rumo) * 0.0174533f;
    float paraOLado = (carro->contatoX - aqui.x) * cosf(rumo) + (carro->contatoZ - aqui.z) * sinf(rumo);
    return fminf(fmaxf(paraOLado / 1.0f, -1.0f), 1.0f);   /* a 1 m do meio já é o lado */
}

/*
 * calcularVibracao
 * A vibração deste quadro, na corrida.
 * Recebe: o que ela lembra do quadro anterior, a corrida e o tempo do quadro.
 * Devolve: a vibração.
 */
Vibracao calcularVibracao(EstadoDaVibracao *estado, Corrida *corrida, float tempoQuadro) {
    Carro *carro = &corrida->jogador;
    Vibracao v = { 0.0f, 0.0f, 0.0f };
    estado->relogio += tempoQuadro;
    float grave = 0.0f, agudo = 0.0f;
    float metrosPorSegundo = carro->velocidade / UNIDADES_POR_METRO;
    estado->faseDoChao += metrosPorSegundo * tempoQuadro;

    verOsMomentosDaCorrida(estado, corrida, tempoQuadro);
    /* A batida noutro carro: o tranco quando o contato começa (a corrida marca a pancada a cada quadro em que eles
     * se encostam); enquanto se esfregam, a lataria raspando, do lado do contato */
    int encostado = carro->pancada >= MENOR_PANCADA;
    if (encostado && !estado->emBatida) {
        baterNaVibracao(estado, carro->pancada, ladoDoContato(carro));
    }
    estado->emBatida = encostado;
    carro->pancada = 0.0f;
    if (encostado && carro->contatoComCarro && metrosPorSegundo > 0.5f) {
        float raspa = 0.5f + 0.5f * asperezaDoChao(estado->faseDoChao * 4.0f);
        float rapidezDoRaspao = 0.5f + 0.5f * fminf(carro->contatoDeslize / 10.0f, 1.0f);
        somarDoLado(&grave, &agudo, LATARIA_GRAVE * raspa * rapidezDoRaspao, LATARIA_AGUDO * raspa * rapidezDoRaspao, ladoDoContato(carro));
    }
    /* Chegando no muro: um tranco na força da velocidade, do lado dele */
    float rapidezAgora = fminf(carro->velocidade / carro->velocidadeMaxima, 1.0f);
    if (carro->raspandoNoMuro && !estado->noMuroAntes && rapidezAgora > 0.08f) {
        baterNaVibracao(estado, BATIDA_NO_MURO + rapidezAgora, (carro->deslocamentoLateral >= 0.0f) ? 1.0f : -1.0f);
    }
    estado->noMuroAntes = carro->raspandoNoMuro;

    /* Antes da largada, depois da chegada (a IA dirige) e nos boxes (o piloto automático): só os momentos */
    int soOsMomentos = corrida->contagemRegressiva > 0.0f || corrida->terminou || carroNoPit(carro);
    if (!soOsMomentos) {
        float ladoDaPista = (carro->deslocamentoLateral > 0.5f) ? 1.0f : (carro->deslocamentoLateral < -0.5f) ? -1.0f : 0.0f;
        CarroSentido sentido = { carro, metrosPorSegundo, fminf(carro->velocidade / carro->velocidadeMaxima, 1.0f), SENTE_ASFALTO,
                                 ladoDaPista, carro->raspandoNoMuro, (carro->deslocamentoLateral >= 0.0f) ? 1.0f : -1.0f };
        /* Onde a pista mediu a zebra (Interlagos, Montreal), vale a medida; nas outras, a borda das curvas */
        Superficie embaixo = superficieDoCarro(carro);
        int naZebra;
        if (carro->posicao.segmento->temBorda) {
            naZebra = (embaixo == SUPERFICIE_ZEBRA);
        } else {
            float lado = fabsf(carro->deslocamentoLateral);
            naZebra = (lado > INICIO_DA_ZEBRA && lado < FIM_DA_ZEBRA && temZebra(carro->posicao.segmento));
        }
        sentido.chao = naZebra ? SENTE_ZEBRA : (embaixo == SUPERFICIE_GRAMA) ? SENTE_GRAMA : SENTE_ASFALTO;
        sentirOCarro(estado, &sentido, tempoQuadro, &grave, &agudo, &v.noAcelerador);

        /* O aro raspando no asfalto, do lado dos pneus estourados */
        if (carro->pneuEstourado && metrosPorSegundo > 0.5f) {
            int p = carro->pneuEstourado;
            float lado = (float) (((p >> 1) & 1) + ((p >> 3) & 1) - (p & 1) - ((p >> 2) & 1)) / 2.0f;
            float raspa = 0.6f + 0.4f * asperezaDoChao(estado->faseDoChao * 2.0f);
            somarDoLado(&grave, &agudo, ARO_GRAVE * raspa, ARO_AGUDO * raspa, lado);
        }
    }
    /* A pistola do Guido em cada roda */
    int canto;
    float andamento;
    if (carro->estadoDoPit == PIT_TROCANDO && faseDaTroca(carro, &canto, &andamento)) {
        float pistola = pulso(estado->relogio * PULSOS_DA_PISTOLA);
        agudo = somar(agudo, PISTOLA_AGUDO * pistola);
        grave = somar(grave, PISTOLA_GRAVE * pistola);
    }
    somarOsGolpes(estado, tempoQuadro, &grave, &agudo);
    v.grave = grave * INTENSIDADE_GERAL;
    v.agudo = agudo * INTENSIDADE_GERAL;
    return semResto(v);
}

/*
 * calcularVibracaoDoPasseio
 * A vibração deste quadro, no mundo aberto: o mesmo carro, o chão da malha do mundo (o asfalto quieto; a terra,
 * a areia e a rocha ásperas) e as batidas nas paredes, nos postes e no chão depois de um pulo.
 * Recebe: o que ela lembra do quadro anterior, o carro livre, a batida deste quadro (0 a 1) e o tempo do quadro.
 * Devolve: a vibração.
 */
Vibracao calcularVibracaoDoPasseio(EstadoDaVibracao *estado, const CarroLivre *carro, float pancada, float tempoQuadro) {
    Vibracao v = { 0.0f, 0.0f, 0.0f };
    estado->relogio += tempoQuadro;
    float grave = 0.0f, agudo = 0.0f;
    float metrosPorSegundo = fabsf(velocidadeDoCarroLivre(carro));
    estado->faseDoChao += metrosPorSegundo * tempoQuadro;
    int encostado = pancada >= MENOR_PANCADA;
    if (encostado && !estado->emBatida) {
        baterNaVibracao(estado, pancada, 0.0f);
    }
    estado->emBatida = encostado;

    ChaoSentido chao = SENTE_ASFALTO;
    if (!carro->noChao) {
        chao = SENTE_NO_AR;
    } else if (carro->chao == CHAO_AREIA) {
        chao = SENTE_AREIA;
    } else if (carro->chao == CHAO_TERRA) {
        chao = SENTE_TERRA;
    } else if (carro->chao == CHAO_ROCHA) {
        chao = SENTE_ROCHA;
    }
    float maxima = (carro->velocidadeMaxima > 0.1f) ? carro->velocidadeMaxima : 1.0f;
    CarroSentido sentido = { &carro->corrida, metrosPorSegundo, fminf(metrosPorSegundo / maxima, 1.0f), chao, 0.0f, carro->raspando, 0.0f };
    sentirOCarro(estado, &sentido, tempoQuadro, &grave, &agudo, &v.noAcelerador);
    somarOsGolpes(estado, tempoQuadro, &grave, &agudo);
    v.grave = grave * INTENSIDADE_GERAL;
    v.agudo = agudo * INTENSIDADE_GERAL;
    return semResto(v);
}

/*
 * vibracaoAoGosto
 * Aplica o que o jogador escolheu: desligada, mais fraca, ou sem o tremor no gatilho.
 * Recebe: a vibração e as configurações.
 * Devolve: a vibração que vai para o controle.
 */
Vibracao vibracaoAoGosto(Vibracao vibracao, const Configuracoes *config) {
    Vibracao nada = { 0.0f, 0.0f, 0.0f };
    if (!config->vibracaoLigada) {
        return nada;
    }
    float forca = config->forcaDaVibracao / 100.0f;
    vibracao.grave *= forca;
    vibracao.agudo *= forca;
    vibracao.noAcelerador *= forca;
    if (!config->gatilhosAdaptativos) {
        vibracao.noAcelerador = 0.0f;
    }
    return vibracao;
}
