/*
 * efeitos.c
 * ------------------------------------------------------------------
 * Os sons do carro e da corrida. Veja efeitos.h.
 * ------------------------------------------------------------------
 */

#include <math.h>
#include "raylib.h"
#include "efeitos.h"
#include "motor.h"
#include "pitstop.h"

/* ---------- Volumes (0 a 1) e ajustes ---------- */
#define VOLUME_MOTOR_DO_JOGADOR    0.85f
#define VOLUME_ALIVIADO            0.90f   /* o loop do pé levantado, em relação ao do pé no fundo */
#define VOLUME_MOTOR_DE_RIVAL      0.60f
#define ALCANCE_DO_MOTOR_DE_RIVAL  110.0f  /* metros: mais longe que isso o rival não é ouvido */
#define MAIS_PERTO_DO_DOPPLER      3.0f    /* metros: colado, o tom não muda (evita pulo ao emparelhar) */
#define EXAGERO_DO_DOPPLER         2.2f    /* o jogo exagera o efeito: na vida real ele é mais fraco */
#define VELOCIDADE_DO_SOM          340.0f  /* metros por segundo */
#define SUAVIZA_DOPPLER            14.0f   /* 1/s */
#define PITCH_MINIMO               0.30f
#define PITCH_MAXIMO               2.40f
#define ENTRADA_DO_MOTOR           4.0f    /* 1/s: o motor entra em ~0,25 s */
#define SAIDA_DO_MOTOR             3.0f    /* ...e sai em ~0,3 s */
#define VOLUME_VENTO               0.55f
#define VOLUME_PNEU                0.80f
#define VOLUME_GRAMA               0.75f
#define VOLUME_MURO                0.85f
#define VOLUME_NITRO               0.75f
#define VOLUME_EFEITOS_CURTOS      0.80f
#define SOBE_O_LOOP                10.0f   /* 1/s: os loops entram rápido... */
#define DESCE_O_LOOP                5.0f   /* ...e saem mais devagar */
#define PAUSA_ENTRE_CAMBIOS        0.22f   /* segundos */
#define VOZES_DE_RIVAL             3       /* quantos rivais soam ao mesmo tempo (os mais próximos) */

/* ---------- Motor: as faixas de rotação ---------- */
typedef struct {
    const char *arquivo;
    float rpmDeReferencia;   /* a rotação em que o loop foi gerado */
    Music musica;
    int carregada;
} FaixaDeMotor;

#define FAIXAS_ACELERANDO 3
#define FAIXAS_ALIVIANDO  2
#define FAIXAS_DE_RIVAL   2

static FaixaDeMotor acelerando[FAIXAS_ACELERANDO] = {
    { .arquivo = "recursos/sons/nascar/nascar_acel_baixo.wav", .rpmDeReferencia = 2400.0f },
    { .arquivo = "recursos/sons/nascar/nascar_acel_medio.wav", .rpmDeReferencia = 5200.0f },
    { .arquivo = "recursos/sons/nascar/nascar_acel_alto.wav", .rpmDeReferencia = 8200.0f },
};
static FaixaDeMotor aliviando[FAIXAS_ALIVIANDO] = {
    { .arquivo = "recursos/sons/nascar/nascar_alivio_baixo.wav", .rpmDeReferencia = 3400.0f },
    { .arquivo = "recursos/sons/nascar/nascar_alivio_alto.wav", .rpmDeReferencia = 6800.0f },
};

/* O motor de cada rival que soa: só as duas faixas de cima, que cobrem a rotação de corrida */
typedef struct {
    FaixaDeMotor faixas[FAIXAS_DE_RIVAL];
    int rival;           /* qual rival (índice em corrida->rivais), ou -1 */
    int tocando;
    float volume;        /* 0 a 1: o fade de entrada e saída */
    float doppler;       /* o fator de tom do efeito Doppler, suavizado */
} VozDeRival;

static VozDeRival vozes[VOZES_DE_RIVAL];

static int motorDoJogadorTocando = 0;
static float entradaDoMotor = 0.0f;
static int motorAtivoAntes = 0;

/* ---------- Loops por estado ---------- */
enum { LOOP_VENTO, LOOP_PNEU, LOOP_GRAMA, LOOP_MURO, LOOP_NITRO, TOTAL_LOOPS };

typedef struct {
    const char *arquivo;
    Music musica;
    int carregado;
    int tocando;
    float volume;    /* o volume de agora (suavizado), de 0 a 1 */
} LoopDeEfeito;

static LoopDeEfeito loops[TOTAL_LOOPS] = {
    [LOOP_VENTO] = { .arquivo = "recursos/sons/efeitos/vento.wav" },
    [LOOP_PNEU] = { .arquivo = "recursos/sons/efeitos/pneu_cantando.wav" },
    [LOOP_GRAMA] = { .arquivo = "recursos/sons/efeitos/grama.wav" },
    [LOOP_MURO] = { .arquivo = "recursos/sons/efeitos/muro_raspando.wav" },
    [LOOP_NITRO] = { .arquivo = "recursos/sons/efeitos/nitro_queimando.wav" },
};

/* ---------- Efeitos curtos ---------- */
enum {
    CURTO_NITRO_INICIO, CURTO_NITRO_FIM, CURTO_VOLTA, CURTO_MELHOR_VOLTA, CURTO_PIT_AVISO, CURTO_PIT_PRONTO,
    CURTO_CAMBIO_SOBE_1, CURTO_CAMBIO_SOBE_2, CURTO_CAMBIO_DESCE_1, CURTO_CAMBIO_DESCE_2,
    CURTO_PISTOLA_1, CURTO_PISTOLA_2, TOTAL_CURTOS
};

static const char *ARQUIVOS_CURTOS[TOTAL_CURTOS] = {
    [CURTO_NITRO_INICIO] = "recursos/sons/efeitos/nitro_inicio.wav",
    [CURTO_NITRO_FIM] = "recursos/sons/efeitos/nitro_fim.wav",
    [CURTO_VOLTA] = "recursos/sons/efeitos/volta_completa.wav",
    [CURTO_MELHOR_VOLTA] = "recursos/sons/efeitos/melhor_volta.wav",
    [CURTO_PIT_AVISO] = "recursos/sons/efeitos/pit_aviso.wav",
    [CURTO_PIT_PRONTO] = "recursos/sons/efeitos/pit_pronto.wav",
    [CURTO_CAMBIO_SOBE_1] = "recursos/sons/efeitos/cambio_sobe_1.wav",
    [CURTO_CAMBIO_SOBE_2] = "recursos/sons/efeitos/cambio_sobe_2.wav",
    [CURTO_CAMBIO_DESCE_1] = "recursos/sons/efeitos/cambio_desce_1.wav",
    [CURTO_CAMBIO_DESCE_2] = "recursos/sons/efeitos/cambio_desce_2.wav",
    [CURTO_PISTOLA_1] = "recursos/sons/efeitos/pit_pistola_1.wav",
    [CURTO_PISTOLA_2] = "recursos/sons/efeitos/pit_pistola_2.wav",
};
static Sound curtos[TOTAL_CURTOS];
static int curtoCarregado[TOTAL_CURTOS];

/* O que os efeitos curtos lembram de um quadro para o outro (para ouvir só a mudança) */
typedef struct {
    int pronto;               /* 0 até o primeiro quadro da corrida: ele só copia o estado, sem som */
    int marcha;
    int nitroLigado;
    int voltas;
    float melhorVolta;
    EstadoDoPit pit;
    int cantoDaPistola;       /* a roda que a pistola está trocando (-1 = nenhuma) */
    double horaDoCambio;
} EstadoDosEfeitos;

static EstadoDosEfeitos estado;

/* ==================================================================
 * Utilidades
 * ================================================================== */

static float limitar(float valor, float menor, float maior) {
    return valor < menor ? menor : (valor > maior ? maior : valor);
}

/* Anda de "atual" até "alvo": sobe com "subida" e desce com "descida" (1/s). */
static float aproximar(float atual, float alvo, float subida, float descida, float tempo) {
    if (alvo > atual) {
        return fminf(atual + subida * tempo, alvo);
    }
    return fmaxf(atual - descida * tempo, alvo);
}

static void carregarFaixa(FaixaDeMotor *faixa) {
    faixa->carregada = 0;
    if (FileExists(faixa->arquivo)) {
        faixa->musica = LoadMusicStream(faixa->arquivo);
        faixa->carregada = IsMusicValid(faixa->musica);
        if (faixa->carregada) {
            faixa->musica.looping = true;
        }
    }
}

static void tocarFaixa(FaixaDeMotor *faixa) {
    if (faixa->carregada) {
        StopMusicStream(faixa->musica);
        PlayMusicStream(faixa->musica);
    }
}

static void pararFaixa(FaixaDeMotor *faixa) {
    if (faixa->carregada) {
        StopMusicStream(faixa->musica);
    }
}

static void descarregarFaixa(FaixaDeMotor *faixa) {
    if (faixa->carregada) {
        UnloadMusicStream(faixa->musica);
        faixa->carregada = 0;
    }
}

/*
 * misturarFaixas
 * Põe o som do motor a uma rotação: mistura as duas faixas vizinhas da rotação (mistura de
 * potência igual, sem queda de volume no meio) e muda o tom de cada uma para a rotação pedida.
 * Recebe: as faixas (da menor para a maior rotação de referência), quantas são, a rotação
 *         (rpm), o volume geral (0 a 1), o lado (0 a 1, 0,5 no meio) e o fator de tom extra.
 * Devolve: nada.
 */
static void misturarFaixas(FaixaDeMotor *faixas, int total, float rpm, float volume, float lado, float tom) {
    for (int i = 0; i < total; i++) {
        float peso = 0.0f;
        float centro = faixas[i].rpmDeReferencia;
        if (i == 0 && rpm <= centro) {
            peso = 1.0f;
        } else if (i == total - 1 && rpm >= centro) {
            peso = 1.0f;
        } else if (rpm < centro && i > 0) {
            float anterior = faixas[i - 1].rpmDeReferencia;
            peso = sqrtf(limitar((rpm - anterior) / (centro - anterior), 0.0f, 1.0f));
        } else if (rpm >= centro && i < total - 1) {
            float seguinte = faixas[i + 1].rpmDeReferencia;
            peso = sqrtf(limitar((seguinte - rpm) / (seguinte - centro), 0.0f, 1.0f));
        }
        if (!faixas[i].carregada) {
            continue;
        }
        SetMusicPitch(faixas[i].musica, limitar(rpm * tom / centro, PITCH_MINIMO, PITCH_MAXIMO));
        SetMusicVolume(faixas[i].musica, volume * peso);
        SetMusicPan(faixas[i].musica, lado);
        UpdateMusicStream(faixas[i].musica);
    }
}

static void tocarCurto(int som, float volume, float tom) {
    if (curtoCarregado[som]) {
        SetSoundVolume(curtos[som], limitar(volume, 0.0f, 1.0f) * VOLUME_EFEITOS_CURTOS);
        SetSoundPitch(curtos[som], tom);
        StopSound(curtos[som]);
        PlaySound(curtos[som]);
    }
}

/* ==================================================================
 * Carregar e descarregar
 * ================================================================== */

/*
 * carregarEfeitos
 * Lê todos os sons. Chamar depois de abrir o dispositivo de som. Arquivo que falta deixa só
 * aquele som mudo.
 * Recebe: nada.
 * Devolve: nada.
 */
void carregarEfeitos(void) {
    for (int i = 0; i < FAIXAS_ACELERANDO; i++) {
        carregarFaixa(&acelerando[i]);
    }
    for (int i = 0; i < FAIXAS_ALIVIANDO; i++) {
        carregarFaixa(&aliviando[i]);
    }
    for (int v = 0; v < VOZES_DE_RIVAL; v++) {
        /* O rival usa as faixas média e alta (as mesmas do jogador), cada voz com a sua cópia */
        vozes[v].faixas[0].arquivo = acelerando[1].arquivo;
        vozes[v].faixas[0].rpmDeReferencia = acelerando[1].rpmDeReferencia;
        vozes[v].faixas[1].arquivo = acelerando[2].arquivo;
        vozes[v].faixas[1].rpmDeReferencia = acelerando[2].rpmDeReferencia;
        for (int f = 0; f < FAIXAS_DE_RIVAL; f++) {
            carregarFaixa(&vozes[v].faixas[f]);
        }
        vozes[v].rival = -1;
        vozes[v].doppler = 1.0f;
    }
    for (int i = 0; i < TOTAL_LOOPS; i++) {
        loops[i].carregado = 0;
        if (FileExists(loops[i].arquivo)) {
            loops[i].musica = LoadMusicStream(loops[i].arquivo);
            loops[i].carregado = IsMusicValid(loops[i].musica);
            if (loops[i].carregado) {
                loops[i].musica.looping = true;
            }
        }
    }
    for (int i = 0; i < TOTAL_CURTOS; i++) {
        curtoCarregado[i] = 0;
        if (FileExists(ARQUIVOS_CURTOS[i])) {
            curtos[i] = LoadSound(ARQUIVOS_CURTOS[i]);
            curtoCarregado[i] = IsSoundValid(curtos[i]);
        }
    }
}

/*
 * descarregarEfeitos
 * Devolve a memória de todos os sons. Chamar antes de fechar o dispositivo de som.
 * Recebe: nada.
 * Devolve: nada.
 */
void descarregarEfeitos(void) {
    for (int i = 0; i < FAIXAS_ACELERANDO; i++) {
        descarregarFaixa(&acelerando[i]);
    }
    for (int i = 0; i < FAIXAS_ALIVIANDO; i++) {
        descarregarFaixa(&aliviando[i]);
    }
    for (int v = 0; v < VOZES_DE_RIVAL; v++) {
        for (int f = 0; f < FAIXAS_DE_RIVAL; f++) {
            descarregarFaixa(&vozes[v].faixas[f]);
        }
    }
    for (int i = 0; i < TOTAL_LOOPS; i++) {
        if (loops[i].carregado) {
            UnloadMusicStream(loops[i].musica);
            loops[i].carregado = 0;
        }
    }
    for (int i = 0; i < TOTAL_CURTOS; i++) {
        if (curtoCarregado[i]) {
            UnloadSound(curtos[i]);
            curtoCarregado[i] = 0;
        }
    }
}

/*
 * iniciarEfeitosDaCorrida
 * Corrida nova: para os sons de estado e zera o que os efeitos curtos lembram. Chamar quando
 * a corrida começa.
 * Recebe: nada.
 * Devolve: nada.
 */
void iniciarEfeitosDaCorrida(void) {
    for (int i = 0; i < FAIXAS_ACELERANDO; i++) {
        pararFaixa(&acelerando[i]);
    }
    for (int i = 0; i < FAIXAS_ALIVIANDO; i++) {
        pararFaixa(&aliviando[i]);
    }
    motorDoJogadorTocando = 0;
    motorAtivoAntes = 0;
    entradaDoMotor = 0.0f;
    for (int v = 0; v < VOZES_DE_RIVAL; v++) {
        for (int f = 0; f < FAIXAS_DE_RIVAL; f++) {
            pararFaixa(&vozes[v].faixas[f]);
        }
        vozes[v].rival = -1;
        vozes[v].tocando = 0;
        vozes[v].volume = 0.0f;
        vozes[v].doppler = 1.0f;
    }
    for (int i = 0; i < TOTAL_LOOPS; i++) {
        if (loops[i].carregado) {
            StopMusicStream(loops[i].musica);
        }
        loops[i].tocando = 0;
        loops[i].volume = 0.0f;
    }
    estado.pronto = 0;
}

/*
 * pausarEfeitosDaCorrida
 * Congela (ou solta) os motores e os loops. Os efeitos curtos são rápidos demais para pausar.
 * Recebe: 1 para pausar, 0 para continuar.
 * Devolve: nada.
 */
void pausarEfeitosDaCorrida(int pausar) {
    for (int i = 0; i < FAIXAS_ACELERANDO; i++) {
        if (acelerando[i].carregada && motorDoJogadorTocando) {
            pausar ? PauseMusicStream(acelerando[i].musica) : ResumeMusicStream(acelerando[i].musica);
        }
    }
    for (int i = 0; i < FAIXAS_ALIVIANDO; i++) {
        if (aliviando[i].carregada && motorDoJogadorTocando) {
            pausar ? PauseMusicStream(aliviando[i].musica) : ResumeMusicStream(aliviando[i].musica);
        }
    }
    for (int v = 0; v < VOZES_DE_RIVAL; v++) {
        for (int f = 0; f < FAIXAS_DE_RIVAL; f++) {
            if (vozes[v].faixas[f].carregada && vozes[v].tocando) {
                pausar ? PauseMusicStream(vozes[v].faixas[f].musica) : ResumeMusicStream(vozes[v].faixas[f].musica);
            }
        }
    }
    for (int i = 0; i < TOTAL_LOOPS; i++) {
        if (loops[i].carregado && loops[i].tocando) {
            pausar ? PauseMusicStream(loops[i].musica) : ResumeMusicStream(loops[i].musica);
        }
    }
}

/* ==================================================================
 * O motor do jogador
 * ================================================================== */

/*
 * atualizarMotorDoJogador
 * O motor NASCAR do carro do jogador: o tom segue a rotação, o volume segue o quanto o motor
 * gira e a mistura passa do loop "acelerando" para o "aliviando" conforme o pé.
 * Recebe: se o motor deve soar, o carro e o tempo do quadro.
 * Devolve: nada.
 */
static void atualizarMotorDoJogador(int ativo, const Carro *jogador, float tempo) {
    if (ativo && !motorAtivoAntes) {
        for (int i = 0; i < FAIXAS_ACELERANDO; i++) {
            tocarFaixa(&acelerando[i]);
        }
        for (int i = 0; i < FAIXAS_ALIVIANDO; i++) {
            tocarFaixa(&aliviando[i]);
        }
        motorDoJogadorTocando = 1;
        entradaDoMotor = 0.0f;
    }
    motorAtivoAntes = ativo;
    if (!motorDoJogadorTocando) {
        return;
    }

    entradaDoMotor = ativo ? fminf(entradaDoMotor + ENTRADA_DO_MOTOR * tempo, 1.0f) : entradaDoMotor - SAIDA_DO_MOTOR * tempo;
    if (entradaDoMotor <= 0.0f) {
        for (int i = 0; i < FAIXAS_ACELERANDO; i++) {
            pararFaixa(&acelerando[i]);
        }
        for (int i = 0; i < FAIXAS_ALIVIANDO; i++) {
            pararFaixa(&aliviando[i]);
        }
        motorDoJogadorTocando = 0;
        entradaDoMotor = 0.0f;
        return;
    }

    float giro = limitar(jogador->motor.giro, 0.0f, 1.0f);
    float carga = limitar(jogador->motor.carga, 0.0f, 1.0f);
    float rpm = giro * RPM_MAXIMO;
    float forca = VOLUME_MOTOR_DO_JOGADOR * entradaDoMotor * (0.55f + 0.45f * giro);

    misturarFaixas(acelerando, FAIXAS_ACELERANDO, rpm, forca * sqrtf(carga), 0.5f, 1.0f);
    misturarFaixas(aliviando, FAIXAS_ALIVIANDO, rpm, forca * VOLUME_ALIVIADO * sqrtf(1.0f - carga), 0.5f, 1.0f);
}

/* ==================================================================
 * O motor dos rivais (os mais próximos, com lado e Doppler)
 * ================================================================== */

/*
 * metrosAteORival
 * Recebe: a corrida e o rival.
 * Devolve: a distância na pista, em metros (positiva se o rival está na frente do jogador).
 */
static float metrosAteORival(const Corrida *corrida, const Carro *rival) {
    return (rival->distanciaPercorrida - corrida->jogador.distanciaPercorrida) / UNIDADES_POR_METRO;
}

/*
 * atualizarMotoresDosRivais
 * Escolhe os rivais mais próximos (até VOZES_DE_RIVAL, dentro do alcance), dá a cada um uma voz e
 * ajusta o som: mais alto quanto mais perto, do lado em que ele está, com o tom subindo quando
 * ele se aproxima e descendo quando se afasta (Doppler).
 * Recebe: a corrida, se os motores devem soar e o tempo do quadro.
 * Devolve: nada.
 */
static void atualizarMotoresDosRivais(Corrida *corrida, int ativo, float tempo) {
    /* Os mais próximos */
    int escolhidos[VOZES_DE_RIVAL];
    float distancias[VOZES_DE_RIVAL];
    for (int v = 0; v < VOZES_DE_RIVAL; v++) {
        escolhidos[v] = -1;
        distancias[v] = ALCANCE_DO_MOTOR_DE_RIVAL;
    }
    if (ativo) {
        for (int r = 0; r < TOTAL_RIVAIS; r++) {
            if (carroNoPit(&corrida->rivais[r])) {
                continue;
            }
            float d = fabsf(metrosAteORival(corrida, &corrida->rivais[r]));
            int posicao = -1;
            for (int v = 0; v < VOZES_DE_RIVAL; v++) {
                if (d < distancias[v]) {
                    posicao = v;
                    break;
                }
            }
            if (posicao >= 0) {
                for (int v = VOZES_DE_RIVAL - 1; v > posicao; v--) {
                    escolhidos[v] = escolhidos[v - 1];
                    distancias[v] = distancias[v - 1];
                }
                escolhidos[posicao] = r;
                distancias[posicao] = d;
            }
        }
    }

    /* Quem já tem voz fica com ela; quem não tem pega uma voz livre */
    for (int v = 0; v < VOZES_DE_RIVAL; v++) {
        int segue = 0;
        for (int e = 0; e < VOZES_DE_RIVAL; e++) {
            segue = segue || (escolhidos[e] >= 0 && escolhidos[e] == vozes[v].rival);
        }
        if (!segue) {
            vozes[v].rival = -1;
        }
    }
    for (int e = 0; e < VOZES_DE_RIVAL; e++) {
        if (escolhidos[e] < 0) {
            continue;
        }
        int tem = 0;
        for (int v = 0; v < VOZES_DE_RIVAL; v++) {
            tem = tem || vozes[v].rival == escolhidos[e];
        }
        for (int v = 0; v < VOZES_DE_RIVAL && !tem; v++) {
            if (vozes[v].rival < 0) {
                vozes[v].rival = escolhidos[e];
                vozes[v].doppler = 1.0f;
                tem = 1;
            }
        }
    }

    /* O som de cada voz */
    const Carro *jogador = &corrida->jogador;
    for (int v = 0; v < VOZES_DE_RIVAL; v++) {
        VozDeRival *voz = &vozes[v];
        float alvo = 0.0f, lado = 0.5f, rpm = 3000.0f, fatorDoppler = 1.0f;

        if (voz->rival >= 0) {
            const Carro *rival = &corrida->rivais[voz->rival];
            float metros = metrosAteORival(corrida, rival);
            float perto = limitar(1.0f - fabsf(metros) / ALCANCE_DO_MOTOR_DE_RIVAL, 0.0f, 1.0f);
            rpm = limitar(rival->motor.giro, 0.0f, 1.0f) * RPM_MAXIMO;
            alvo = VOLUME_MOTOR_DE_RIVAL * perto * sqrtf(perto) * (0.5f + 0.5f * rival->motor.giro);
            lado = 0.5f + 0.38f * limitar((rival->deslocamentoLateral - jogador->deslocamentoLateral) * 1.2f, -1.0f, 1.0f);

            /* Doppler: quanto os dois se aproximam, em metros por segundo (positivo = se aproximando) */
            if (fabsf(metros) > MAIS_PERTO_DO_DOPPLER) {
                float aproximando = (jogador->velocidade - rival->velocidade) * ((metros > 0.0f) ? 1.0f : -1.0f)
                                    / UNIDADES_POR_METRO;
                fatorDoppler = limitar(1.0f / (1.0f - EXAGERO_DO_DOPPLER * aproximando / VELOCIDADE_DO_SOM), 0.85f, 1.2f);
            }
            voz->doppler += (fatorDoppler - voz->doppler) * limitar(SUAVIZA_DOPPLER * tempo, 0.0f, 1.0f);
        }

        voz->volume = aproximar(voz->volume, alvo, ENTRADA_DO_MOTOR, SAIDA_DO_MOTOR, tempo);
        if (voz->volume > 0.002f && !voz->tocando) {
            for (int f = 0; f < FAIXAS_DE_RIVAL; f++) {
                tocarFaixa(&voz->faixas[f]);
            }
            voz->tocando = 1;
        } else if (voz->volume <= 0.002f && alvo <= 0.0f && voz->tocando) {
            for (int f = 0; f < FAIXAS_DE_RIVAL; f++) {
                pararFaixa(&voz->faixas[f]);
            }
            voz->tocando = 0;
        }
        if (voz->tocando) {
            misturarFaixas(voz->faixas, FAIXAS_DE_RIVAL, rpm, voz->volume, lado, voz->doppler);
        }
    }
}

/* ==================================================================
 * Os loops de estado
 * ================================================================== */

/*
 * atualizarLoop
 * Um loop de efeito: o volume anda suavemente até o alvo; liga quando passa de zero e desliga
 * quando some.
 * Recebe: o loop, o volume alvo (0 a 1), o tom e o tempo do quadro.
 * Devolve: nada.
 */
static void atualizarLoop(LoopDeEfeito *loop, float alvo, float tom, float tempo) {
    if (!loop->carregado) {
        return;
    }
    loop->volume = aproximar(loop->volume, limitar(alvo, 0.0f, 1.0f), SOBE_O_LOOP, DESCE_O_LOOP, tempo);
    if (loop->volume > 0.003f && !loop->tocando) {
        StopMusicStream(loop->musica);
        PlayMusicStream(loop->musica);
        loop->tocando = 1;
    } else if (loop->volume <= 0.003f && alvo <= 0.0f && loop->tocando) {
        StopMusicStream(loop->musica);
        loop->tocando = 0;
    }
    if (loop->tocando) {
        SetMusicVolume(loop->musica, loop->volume);
        SetMusicPitch(loop->musica, limitar(tom, PITCH_MINIMO, PITCH_MAXIMO));
        UpdateMusicStream(loop->musica);
    }
}

/*
 * atualizarLoopsDoJogador
 * O vento, o pneu, a grama, o muro e o nitro do carro do jogador, cada um pelo estado dele.
 * Recebe: a corrida, se a corrida está andando e o tempo do quadro.
 * Devolve: nada.
 */
static void atualizarLoopsDoJogador(Corrida *corrida, int ativo, float tempo) {
    Carro *jogador = &corrida->jogador;
    float f = ativo ? limitar(jogador->velocidade / jogador->velocidadeMaxima, 0.0f, 1.0f) : 0.0f;

    /* O vento: cresce com o quadrado da velocidade, e o nitro o empurra */
    float vento = VOLUME_VENTO * f * f * (1.0f + (jogador->nitroLigado ? 0.25f : 0.0f));
    atualizarLoop(&loops[LOOP_VENTO], ativo ? vento : 0.0f, 0.75f + 0.55f * f, tempo);

    /* O pneu: o drift, a curva vencendo o volante, a rodada e a freada forte */
    float escorrega = fmaxf(jogador->forcaDoDrift, limitar((jogador->derrapagem - 0.35f) / 0.65f, 0.0f, 1.0f));
    if (jogador->rodando) {
        escorrega = 1.0f;
    }
    if (jogador->ultimosComandos.frear && f > 0.45f) {
        escorrega = fmaxf(escorrega, 0.40f * limitar((f - 0.45f) / 0.35f, 0.0f, 1.0f));
    }
    float pneu = VOLUME_PNEU * escorrega * limitar(f / 0.2f, 0.0f, 1.0f);
    atualizarLoop(&loops[LOOP_PNEU], ativo ? pneu : 0.0f, 0.85f + 0.35f * f, tempo);

    /* A grama (e a zebra): fora do asfalto */
    int fora = ativo && estaForaDaPista(jogador) && f > 0.04f && !carroNoPit(jogador);
    atualizarLoop(&loops[LOOP_GRAMA], fora ? VOLUME_GRAMA * powf(f, 0.7f) : 0.0f, 0.7f + 0.7f * f, tempo);

    /* O muro */
    int raspando = ativo && jogador->raspandoNoMuro && f > 0.03f;
    atualizarLoop(&loops[LOOP_MURO], raspando ? VOLUME_MURO * powf(f, 0.6f) : 0.0f, 0.8f + 0.4f * f, tempo);

    /* O nitro queimando */
    atualizarLoop(&loops[LOOP_NITRO], (ativo && jogador->nitroLigado) ? VOLUME_NITRO : 0.0f, 0.9f + 0.3f * f, tempo);
}

/* ==================================================================
 * Os efeitos curtos
 * ================================================================== */

/*
 * sincronizarEstado
 * Copia o estado atual do carro para o que os efeitos curtos lembram, sem tocar nada.
 * Recebe: o carro do jogador.
 * Devolve: nada.
 */
static void sincronizarEstado(const Carro *jogador) {
    estado.pronto = 1;
    estado.marcha = jogador->motor.marcha;
    estado.nitroLigado = jogador->nitroLigado;
    estado.voltas = jogador->voltas;
    estado.melhorVolta = jogador->melhorVolta;
    estado.pit = jogador->estadoDoPit;
    estado.cantoDaPistola = -1;
}

/*
 * dispararEfeitosCurtos
 * Toca o que mudou desde o quadro anterior: a troca de marcha, o nitro, a volta completa e o
 * pit stop.
 * Recebe: o carro do jogador.
 * Devolve: nada.
 */
static void dispararEfeitosCurtos(Carro *jogador) {
    double agora = GetTime();

    /* Câmbio: sobe com um estouro seco, desce com a acelerada e a rajada de estouros */
    if (jogador->motor.marcha != estado.marcha) {
        if (agora - estado.horaDoCambio > PAUSA_ENTRE_CAMBIOS && jogador->velocidade > 400.0f) {
            float giro = limitar(jogador->motor.giro, 0.0f, 1.0f);
            int variante = GetRandomValue(0, 1);
            if (jogador->motor.marcha > estado.marcha) {
                tocarCurto(CURTO_CAMBIO_SOBE_1 + variante, 0.45f + 0.35f * giro, 1.0f);
            } else {
                tocarCurto(CURTO_CAMBIO_DESCE_1 + variante, 0.60f + 0.25f * giro, 0.95f + 0.1f * (float) GetRandomValue(0, 100) / 100.0f);
            }
            estado.horaDoCambio = agora;
        }
        estado.marcha = jogador->motor.marcha;
    }

    /* Nitro */
    if (jogador->nitroLigado && !estado.nitroLigado) {
        tocarCurto(CURTO_NITRO_INICIO, 1.0f, 1.0f);
    } else if (!jogador->nitroLigado && estado.nitroLigado) {
        tocarCurto(CURTO_NITRO_FIM, 0.9f, 1.0f);
    }
    estado.nitroLigado = jogador->nitroLigado;

    /* Volta completa: o "bip" de sempre, ou o "ding" se foi a melhor até agora (a partir da 2ª) */
    if (jogador->voltas > estado.voltas) {
        int melhorou = jogador->voltas >= 2 && jogador->melhorVolta > 0.0f &&
                       (estado.melhorVolta <= 0.0f || jogador->melhorVolta < estado.melhorVolta);
        tocarCurto(melhorou ? CURTO_MELHOR_VOLTA : CURTO_VOLTA, 0.9f, 1.0f);
    }
    estado.voltas = jogador->voltas;
    estado.melhorVolta = jogador->melhorVolta;

    /* Pit stop: o aviso do limitador, a pistola em cada roda e o "pode ir" */
    if (jogador->estadoDoPit != estado.pit) {
        if (estado.pit == PIT_FORA && jogador->estadoDoPit == PIT_CHEGANDO) {
            tocarCurto(CURTO_PIT_AVISO, 0.8f, 1.0f);
        } else if (estado.pit == PIT_TROCANDO && jogador->estadoDoPit == PIT_SAINDO) {
            tocarCurto(CURTO_PIT_PRONTO, 0.9f, 1.0f);
        }
        estado.cantoDaPistola = -1;
        estado.pit = jogador->estadoDoPit;
    }
    if (jogador->estadoDoPit == PIT_TROCANDO) {
        int canto;
        float andamento;
        if (faseDaTroca(jogador, &canto, &andamento) && canto != estado.cantoDaPistola) {
            tocarCurto(CURTO_PISTOLA_1 + GetRandomValue(0, 1), 0.95f, 0.95f + 0.1f * (float) GetRandomValue(0, 100) / 100.0f);
            estado.cantoDaPistola = canto;
        }
    }
}

/* ==================================================================
 * Um quadro
 * ================================================================== */

/*
 * atualizarEfeitosDaCorrida
 * Chamar uma vez por quadro com a corrida andando (fora da pausa). Com "ativo" em 0 (contagem,
 * vitória, resultado), os motores e os loops somem aos poucos e nenhum efeito curto toca.
 * Recebe: a corrida, se os sons de carro devem soar (1 do GO até o fim da prova) e o tempo do quadro.
 * Devolve: nada.
 */
void atualizarEfeitosDaCorrida(Corrida *corrida, int ativo, float tempoQuadro) {
    Carro *jogador = &corrida->jogador;

    atualizarMotorDoJogador(ativo, jogador, tempoQuadro);
    atualizarMotoresDosRivais(corrida, ativo, tempoQuadro);
    atualizarLoopsDoJogador(corrida, ativo, tempoQuadro);

    if (!estado.pronto || !ativo) {
        sincronizarEstado(jogador);   /* sem som: só acompanha, para ouvir apenas o que mudar depois */
    } else {
        dispararEfeitosCurtos(jogador);
    }
}
