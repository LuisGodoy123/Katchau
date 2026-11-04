/*
 * vibracao.c
 * ------------------------------------------------------------------
 * As sensações do controle. A base é o asfalto: enquanto o carro anda, o chão
 * está sempre nas mãos, e é por isso que dá para sentir quando ele some. Em cima
 * dela entram o pneu, o nitro e os trancos. Cada sensação pede uma quantidade de
 * vibração grave e aguda, e elas se somam sem passar do máximo.
 *
 * Os pulsos ficam abaixo de uns 16 por segundo: acima disso o controle não
 * acompanha, e viram um zumbido sem forma.
 * ------------------------------------------------------------------
 */

#include <math.h>

#include "vibracao.h"
#include "pitstop.h"

#define INTENSIDADE_GERAL      0.90f   /* o volume de toda a vibração */

/* ---------- Asfalto: quanto mais rápido, mais o chão passa nas mãos ---------- */
#define ASFALTO_GRAVE_PARADO   0.03f
#define ASFALTO_GRAVE_RAPIDO   0.20f
#define ASFALTO_AGUDO_PARADO   0.02f
#define ASFALTO_AGUDO_RAPIDO   0.16f
#define METROS_POR_GRAO        2.6f    /* o grão do asfalto: cada tanto, um pulso bem leve */
#define MAIS_GRAOS_POR_SEGUNDO 15.0f
#define METROS_ENTRE_JUNTAS    26.0f   /* as juntas do piso, de vez em quando */
#define GOLPE_DA_JUNTA         0.14f
#define VELOCIDADE_DAS_JUNTAS  0.15f   /* devagar, nem se percebe */
#define PERDA_DE_CHAO          0.70f   /* escorregando, o asfalto some das mãos: o carro flutua */

/* ---------- Motor ---------- */
#define GIRO_DO_LIMITADOR      0.97f
#define LIMITADOR              0.22f
#define PULSOS_DO_LIMITADOR    14.0f
#define MOTOR_CHEIO_PARADO     0.16f   /* acelerando parado, na largada */

/* ---------- Chão fora do asfalto ---------- */
#define INICIO_DA_ZEBRA        0.93f   /* em larguras: a roda de fora já pisa nela */
#define FIM_DA_ZEBRA           1.15f
#define METROS_ENTRE_RELEVOS   1.8f
#define PULSOS_MAXIMOS_DA_ZEBRA 13.0f  /* por segundo */
#define ZEBRA_GRAVE            0.55f
#define ZEBRA_AGUDO            0.28f
#define GRAMA_GRAVE            0.50f
#define GRAMA_AGUDO            0.25f

/* ---------- Pneu escorregando: um tremor irregular, não um zumbido liso ---------- */
#define INICIO_DO_PATINAR      0.20f   /* derrapagem abaixo disso não se sente */
#define TREMOR_DO_PNEU         0.55f
#define BATIDAS_DO_TREMOR      12.0f   /* por segundo, em média */
#define DRIFT_GRAVE            0.28f   /* no drift, o peso do carro vai para o lado de fora */
#define DRIFT_AGUDO            0.22f
#define MAO_DE_DENTRO          0.35f   /* a mão de dentro da curva sente só uma parte */
#define GOLPE_QUEBRA_TRASEIRA  0.50f   /* a traseira solta */
#define GOLPE_PNEU_AGARRA      0.38f   /* e o pneu agarra de novo */
#define DRIFT_LIGADO           0.50f
#define DRIFT_ACABANDO         0.08f
#define VELOCIDADE_DA_ARRANCADA 1600.0f
#define ARRANCADA_AGUDO        0.40f
#define ARRANCADA_GRAVE        0.18f
#define INICIO_DO_PNEU_GASTO   0.70f
#define PNEU_GASTO             0.20f   /* o pneu acabado escorrega antes */

/* ---------- Freio: o peso do carro indo para a frente, e o ABS só nas freadas fortes ---------- */
#define PESO_NA_FREADA         0.16f
#define RAPIDEZ_DO_ABS         0.38f   /* abaixo disso da velocidade máxima, o ABS não entra */
#define PULSOS_DO_ABS          15.0f
#define ABS_AGUDO              0.20f

/* ---------- Nitro: empurrão, combustão e cauda ---------- */
#define GOLPE_DO_NITRO_GRAVE   0.85f
#define GOLPE_DO_NITRO_AGUDO   0.45f
#define SEGUNDOS_DO_EMPURRAO   0.45f   /* logo depois de ligar, o carro é jogado para a frente */
#define EMPURRAO_DO_NITRO      0.25f
#define NITRO_GRAVE            0.26f
#define NITRO_PULSO            0.14f   /* a combustão pulsando */
#define PULSOS_DO_NITRO        10.0f
#define NITRO_AGUDO            0.13f
#define NITRO_NO_FIM           0.12f   /* com tão pouco no tanque, o nitro engasga */
#define ENGASGOS_DO_NITRO      6.0f    /* por segundo */
#define CAUDA_DO_NITRO         0.30f
#define SUMICO_DA_CAUDA        6.0f    /* por segundo */

/* ---------- Trancos, que somem aos poucos ---------- */
#define SUMICO_DO_GOLPE        9.0f    /* por segundo */
#define GOLPE_SUBINDO_MARCHA   0.20f
#define GOLPE_DESCENDO_MARCHA  0.26f   /* a reduzida, com a acelerada do motor, se sente mais */
#define GOLPE_DA_CONTAGEM      0.28f
#define GOLPE_DA_LARGADA       0.75f
#define GOLPE_DA_CHEGADA       0.85f
#define GOLPE_DO_MACACO        0.40f

/* ---------- O resto ---------- */
#define MURO_GRAVE             0.30f
#define MURO_AGUDO             0.55f
#define RODADA_GRAVE           0.55f
#define RODADA_AGUDO           0.40f
#define LIMITE_DOS_BOXES       0.10f
#define PULSOS_DA_PISTOLA      16.0f   /* a pistola de parafusos do Guido */
#define PISTOLA_AGUDO          0.85f
#define PISTOLA_GRAVE          0.30f

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
 * Recebe: o estado, no começo de cada corrida.
 * Devolve: nada.
 */
void iniciarEstadoDaVibracao(EstadoDaVibracao *estado) {
    estado->relogio = 0.0f;
    estado->faseDoChao = 0.0f;
    estado->golpeGrave = 0.0f;
    estado->golpeAgudo = 0.0f;
    estado->caudaDoNitro = 0.0f;
    estado->tempoComNitro = 0.0f;
    estado->juntaAntes = 0;
    estado->marchaAntes = 0;
    estado->nitroAntes = 0;
    estado->driftAntes = 0;
    estado->estadoDoPitAntes = PIT_FORA;
    estado->numeroDaContagemAntes = -1;
    estado->terminouAntes = 0;
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
 * verOsTrancos
 * Procura o que aconteceu neste quadro e vira um tranco: a batida, a marcha, o nitro,
 * a traseira soltando e o pneu agarrando, o macaco dos boxes, a contagem e a chegada.
 * Recebe: o estado e a corrida.
 * Devolve: nada.
 */
static void verOsTrancos(EstadoDaVibracao *estado, Corrida *corrida) {
    Carro *carro = &corrida->jogador;

    if (carro->pancada > 0.0f) {
        darGolpe(estado, carro->pancada, carro->pancada * 0.8f);
        carro->pancada = 0.0f;
    }
    if (carro->motor.marcha > estado->marchaAntes) {
        darGolpe(estado, GOLPE_SUBINDO_MARCHA, GOLPE_SUBINDO_MARCHA * 0.5f);
    } else if (carro->motor.marcha < estado->marchaAntes) {
        darGolpe(estado, GOLPE_DESCENDO_MARCHA, GOLPE_DESCENDO_MARCHA * 0.6f);
    }
    estado->marchaAntes = carro->motor.marcha;

    if (carro->nitroLigado && !estado->nitroAntes) {
        darGolpe(estado, GOLPE_DO_NITRO_GRAVE, GOLPE_DO_NITRO_AGUDO);
        estado->tempoComNitro = 0.0f;
    } else if (!carro->nitroLigado && estado->nitroAntes) {
        estado->caudaDoNitro = 1.0f;
    }
    estado->nitroAntes = carro->nitroLigado;

    if (!estado->driftAntes && carro->forcaDoDrift > DRIFT_LIGADO) {
        darGolpe(estado, GOLPE_QUEBRA_TRASEIRA, GOLPE_QUEBRA_TRASEIRA * 0.5f);
        estado->driftAntes = 1;
    } else if (estado->driftAntes && carro->forcaDoDrift < DRIFT_ACABANDO) {
        darGolpe(estado, GOLPE_PNEU_AGARRA, GOLPE_PNEU_AGARRA * 0.5f);
        estado->driftAntes = 0;
    }

    if (carro->estadoDoPit != (EstadoDoPit) estado->estadoDoPitAntes
        && (carro->estadoDoPit == PIT_TROCANDO || carro->estadoDoPit == PIT_SAINDO)) {
        darGolpe(estado, GOLPE_DO_MACACO, GOLPE_DO_MACACO * 0.4f);   /* o carro sobe e desce no macaco */
    }
    estado->estadoDoPitAntes = carro->estadoDoPit;

    /* Um toque a cada número da contagem, e um tranco na largada */
    int numero = (corrida->contagemRegressiva > 0.0f) ? (int) ceilf(corrida->contagemRegressiva) : 0;
    if (numero != estado->numeroDaContagemAntes && estado->numeroDaContagemAntes >= 0) {
        if (numero == 0) {
            darGolpe(estado, GOLPE_DA_LARGADA, GOLPE_DA_LARGADA * 0.6f);
        } else if (numero <= 3) {
            darGolpe(estado, GOLPE_DA_CONTAGEM, 0.0f);
        }
    }
    estado->numeroDaContagemAntes = numero;

    if (corrida->terminou && !estado->terminouAntes) {
        darGolpe(estado, GOLPE_DA_CHEGADA, GOLPE_DA_CHEGADA);
    }
    estado->terminouAntes = corrida->terminou;
}

/*
 * calcularVibracao
 * A vibração deste quadro.
 * Recebe: o que ela lembra do quadro anterior, a corrida e o tempo do quadro.
 * Devolve: a vibração.
 */
Vibracao calcularVibracao(EstadoDaVibracao *estado, Corrida *corrida, float tempoQuadro) {
    Carro *carro = &corrida->jogador;
    Vibracao v = { 0.0f, 0.0f, 0.0f };
    float relogio = (estado->relogio += tempoQuadro);
    float grave = 0.0f, agudo = 0.0f;
    float metrosPorSegundo = carro->velocidade / UNIDADES_POR_METRO;
    float rapidez = fminf(carro->velocidade / carro->velocidadeMaxima, 1.0f);
    estado->faseDoChao += metrosPorSegundo * tempoQuadro;

    verOsTrancos(estado, corrida);
    float some = expf(-SUMICO_DO_GOLPE * tempoQuadro);
    grave = somar(grave, estado->golpeGrave);
    agudo = somar(agudo, estado->golpeAgudo);
    estado->golpeGrave *= some;
    estado->golpeAgudo *= some;

    /* A corrida acabou: depois do tranco da chegada, o controle fica quieto (a IA está dirigindo) */
    if (corrida->terminou) {
        v.grave = grave * INTENSIDADE_GERAL;
        v.agudo = agudo * INTENSIDADE_GERAL;
        return v;
    }

    /* O motor na largada e o limitador no fim do giro */
    if (carro->motor.giro > GIRO_DO_LIMITADOR && carro->motor.carga > 0.5f) {
        agudo = somar(agudo, LIMITADOR * pulso(relogio * PULSOS_DO_LIMITADOR));
    }
    if (corrida->contagemRegressiva > 0.0f) {
        grave = somar(grave, MOTOR_CHEIO_PARADO * carro->motor.giro * carro->motor.carga);
        v.grave = grave * INTENSIDADE_GERAL;
        v.agudo = agudo * INTENSIDADE_GERAL;
        return v;
    }

    /* Nos boxes é o piloto automático: o chão, e a pistola do Guido em cada roda */
    if (carroNoPit(carro)) {
        grave = somar(grave, LIMITE_DOS_BOXES * rapidez * 3.0f);
        int canto;
        float andamento;
        if (carro->estadoDoPit == PIT_TROCANDO && faseDaTroca(carro, &canto, &andamento)) {
            float pistola = pulso(relogio * PULSOS_DA_PISTOLA);
            agudo = somar(agudo, PISTOLA_AGUDO * pistola);
            grave = somar(grave, PISTOLA_GRAVE * pistola);
        }
        v.grave = grave * INTENSIDADE_GERAL;
        v.agudo = agudo * INTENSIDADE_GERAL;
        return v;
    }

    /* Quanto o pneu está escorregando: é o que tira o chão das mãos */
    float escorregando = 0.0f;
    if (carro->derrapagem > INICIO_DO_PATINAR) {
        escorregando = (carro->derrapagem - INICIO_DO_PATINAR) / (1.0f - INICIO_DO_PATINAR);
    }

    /* O chão: fora do asfalto, grama ou zebra; no asfalto, a textura, sempre presente */
    float lado = fabsf(carro->deslocamentoLateral);
    int naZebra = (lado > INICIO_DA_ZEBRA && lado < FIM_DA_ZEBRA && temZebra(carro->posicao.segmento));
    int andando = (carro->velocidade > 0.0f);
    if (estaForaDaPista(carro) && !naZebra) {
        float aspereza = asperezaDoChao(estado->faseDoChao);
        float forca = fminf(0.35f + rapidez * 2.0f, 1.0f) * andando;
        grave = somar(grave, GRAMA_GRAVE * forca * (0.4f + 0.6f * aspereza));
        agudo = somar(agudo, GRAMA_AGUDO * forca * aspereza);
    } else if (andando) {
        float chao = 1.0f - PERDA_DE_CHAO * escorregando;
        float porSegundo = fminf(metrosPorSegundo / METROS_POR_GRAO, MAIS_GRAOS_POR_SEGUNDO);
        float grao = 0.75f + 0.25f * pulso(relogio * porSegundo);
        float aspereza = 0.65f + 0.35f * asperezaDoChao(estado->faseDoChao * 0.4f);
        float cresce = sqrtf(rapidez);
        grave = somar(grave, (ASFALTO_GRAVE_PARADO + (ASFALTO_GRAVE_RAPIDO - ASFALTO_GRAVE_PARADO) * cresce) * aspereza * chao);
        agudo = somar(agudo, (ASFALTO_AGUDO_PARADO + (ASFALTO_AGUDO_RAPIDO - ASFALTO_AGUDO_PARADO) * rapidez) * grao * aspereza * chao);

        /* As juntas do piso: um toque de vez em quando */
        int junta = (int) (estado->faseDoChao / METROS_ENTRE_JUNTAS);
        if (junta != estado->juntaAntes && rapidez > VELOCIDADE_DAS_JUNTAS) {
            darGolpe(estado, GOLPE_DA_JUNTA * chao, GOLPE_DA_JUNTA * 0.7f * chao);
        }
        estado->juntaAntes = junta;
    }
    if (naZebra && andando) {
        float porSegundo = fminf(metrosPorSegundo / METROS_ENTRE_RELEVOS, PULSOS_MAXIMOS_DA_ZEBRA);
        float relevo = pulso(relogio * porSegundo);
        float forca = 0.5f + 0.5f * rapidez;
        grave = somar(grave, ZEBRA_GRAVE * forca * relevo);
        agudo = somar(agudo, ZEBRA_AGUDO * forca * relevo);
    }

    /* O pneu perdendo o chão: um tremor irregular na mão direita */
    agudo = somar(agudo, TREMOR_DO_PNEU * escorregando * tremorIrregular(relogio) * (0.45f + 0.55f * rapidez));

    /* De lado, o peso do carro vai para fora da curva: a mão de fora sente mais */
    if (carro->forcaDoDrift > 0.0f) {
        float deLado = fminf(fabsf(carro->anguloDeDrift) / 30.0f, 1.0f) * carro->forcaDoDrift;
        int viraParaADireita = (carro->anguloDeDrift > 0.0f);   /* a traseira vai para a esquerda */
        grave = somar(grave, DRIFT_GRAVE * deLado * (viraParaADireita ? 1.0f : MAO_DE_DENTRO));
        agudo = somar(agudo, DRIFT_AGUDO * deLado * (viraParaADireita ? MAO_DE_DENTRO : 1.0f));
    }

    /* O pneu gasto escorrega antes de chegar ao limite: no volante ele já avisa */
    if (carro->desgaste > INICIO_DO_PNEU_GASTO && fabsf(carro->volante) > 0.3f) {
        float gasto = (carro->desgaste - INICIO_DO_PNEU_GASTO) / (1.0f - INICIO_DO_PNEU_GASTO);
        agudo = somar(agudo, PNEU_GASTO * gasto * fabsf(carro->volante) * rapidez * tremorIrregular(relogio));
    }

    /* A roda patinando na arrancada */
    float patinando = 0.0f;
    if (carro->ultimosComandos.acelerar && carro->velocidade < VELOCIDADE_DA_ARRANCADA) {
        patinando = 1.0f - carro->velocidade / VELOCIDADE_DA_ARRANCADA;
        agudo = somar(agudo, ARRANCADA_AGUDO * patinando);
        grave = somar(grave, ARRANCADA_GRAVE * patinando);
    }
    v.noAcelerador = fmaxf(patinando, carro->ultimosComandos.acelerar ? escorregando : 0.0f);

    /* A freada: o peso vai para a frente; e, nas fortes, o ABS pulsa */
    if (carro->ultimosComandos.frear && andando) {
        grave = somar(grave, PESO_NA_FREADA * rapidez);
        if (rapidez > RAPIDEZ_DO_ABS) {
            agudo = somar(agudo, ABS_AGUDO * (rapidez - RAPIDEZ_DO_ABS) / (1.0f - RAPIDEZ_DO_ABS) * pulso(relogio * PULSOS_DO_ABS));
        }
    }

    /* O nitro: o empurrão, a combustão pulsando, e engasgando quando o tanque acaba */
    if (carro->nitroLigado) {
        estado->tempoComNitro += tempoQuadro;
        float empurrao = fmaxf(1.0f - estado->tempoComNitro / SEGUNDOS_DO_EMPURRAO, 0.0f);
        float combustao = pulso(relogio * PULSOS_DO_NITRO);
        float tanque = (carro->nitro < NITRO_NO_FIM) ? pulso(relogio * ENGASGOS_DO_NITRO) : 1.0f;
        grave = somar(grave, (NITRO_GRAVE + NITRO_PULSO * combustao + EMPURRAO_DO_NITRO * empurrao) * tanque);
        agudo = somar(agudo, NITRO_AGUDO * (0.5f + 0.5f * combustao) * tanque);
    } else if (estado->caudaDoNitro > 0.0f) {
        grave = somar(grave, CAUDA_DO_NITRO * estado->caudaDoNitro);
        estado->caudaDoNitro *= expf(-SUMICO_DA_CAUDA * tempoQuadro);
        if (estado->caudaDoNitro < 0.01f) {
            estado->caudaDoNitro = 0.0f;
        }
    }

    /* O muro raspa e a rodada sacode */
    if (carro->raspandoNoMuro && andando) {
        float raspando = 0.5f + 0.5f * asperezaDoChao(estado->faseDoChao * 3.0f);
        grave = somar(grave, MURO_GRAVE * raspando);
        agudo = somar(agudo, MURO_AGUDO * raspando);
    }
    if (carro->rodando) {
        float sacudindo = 0.5f + 0.5f * asperezaDoChao(relogio * 20.0f);
        grave = somar(grave, RODADA_GRAVE * sacudindo);
        agudo = somar(agudo, RODADA_AGUDO * sacudindo);
    }

    v.grave = grave * INTENSIDADE_GERAL;
    v.agudo = agudo * INTENSIDADE_GERAL;
    return v;
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
