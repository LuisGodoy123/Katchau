/*
 * efeitos.c
 * ------------------------------------------------------------------
 * Os sons do carro e da corrida. Veja efeitos.h.
 * ------------------------------------------------------------------
 */

#include <math.h>
#include <stdlib.h>
#include "raylib.h"
#include "efeitos.h"
#include "motor.h"
#include "motorgranular.h"
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
#define VOLUME_ZEBRA               0.70f
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

/* ==================================================================
 * O motor de todos os carros, a partir de nitro_car.mp3
 * ================================================================== */

/* O motor do carro do jogador e o dos rivais que soam perto é a gravação nitro_car.mp3, tocada pelo
 * motorgranular.c: o tom segue a rotação de cada carro, cada marcha usa a sua arrancada da gravação e a troca de
 * marcha faz o som cair e subir como no carro gravado. Cada carro que soa tem a sua voz (um motor granular e um
 * fluxo de áudio de baixa latência); todas leem a mesma gravação. Se o arquivo faltar, o motor NASCAR de antes
 * continua tocando. */
#define ARQUIVO_DO_MOTOR           "recursos/sons/efeitos/nitro_car.mp3"
#define VOLUME_DO_MOTOR_GRANULAR   1.15f   /* a gravação já sai no volume dela (RMS 0,2); isto dá um pouco de presença */
#define NASCAR_SOB_O_MOTOR         0.0f    /* quanto do motor NASCAR (que tem outro tom) ainda toca junto nos carros */
#define VOZES_GRANULARES           (1 + VOZES_DE_RIVAL)   /* a voz 0 é a do jogador; as outras, as dos rivais */

typedef struct {
    MotorGranular motor;
    AudioStream fluxo;
    int valida;      /* o fluxo abriu */
    int tocando;
} VozGranular;

static float *gravacaoDoMotor = NULL;
static int totalDaGravacao = 0;
static VozGranular vozesGranulares[VOZES_GRANULARES];
static int motorGranularPronto = 0;

/*
 * carregarMotorGranular
 * Lê o mp3, deixa a gravação em mono a 44100 Hz e abre os fluxos de áudio de baixa latência por onde os motores
 * saem (blocos de PASSO_DO_MOTOR amostras, ~23 ms).
 */
static void carregarMotorGranular(void) {
    motorGranularPronto = 0;
    if (!FileExists(ARQUIVO_DO_MOTOR)) {
        TraceLog(LOG_WARNING, "MOTOR: %s nao existe: o motor NASCAR toca no lugar", ARQUIVO_DO_MOTOR);
        return;
    }
    Wave onda = LoadWave(ARQUIVO_DO_MOTOR);
    if (!IsWaveValid(onda) || onda.channels < 1 || onda.frameCount == 0) {
        TraceLog(LOG_WARNING, "MOTOR: nao deu para ler %s", ARQUIVO_DO_MOTOR);
        return;
    }
    float *amostras = LoadWaveSamples(onda);
    int canais = (int) onda.channels, quadros = (int) onda.frameCount;
    int destino = (int) ((long long) quadros * TAXA_DO_MOTOR_GRANULAR / (long long) onda.sampleRate);
    gravacaoDoMotor = malloc(sizeof(float) * (size_t) (destino + 4));
    if (amostras != NULL && gravacaoDoMotor != NULL) {
        for (int i = 0; i < destino; i++) {
            float onde = (float) i * (float) onda.sampleRate / (float) TAXA_DO_MOTOR_GRANULAR;   /* reamostra, se não for 44100 */
            int q = (int) onde;
            float f = onde - (float) q;
            int q2 = (q + 1 < quadros) ? q + 1 : q;
            float a = 0.0f, b = 0.0f;
            for (int c = 0; c < canais; c++) {
                a += amostras[q * canais + c];
                b += amostras[q2 * canais + c];
            }
            gravacaoDoMotor[i] = (a * (1.0f - f) + b * f) / (float) canais;
        }
        totalDaGravacao = destino;
        MotorGranular prova;
        if (iniciarMotorGranular(&prova, gravacaoDoMotor, destino)) {
            SetAudioStreamBufferSizeDefault(PASSO_DO_MOTOR);   /* blocos pequenos: o motor responde ao pedal sem demora */
            for (int v = 0; v < VOZES_GRANULARES; v++) {
                vozesGranulares[v].fluxo = LoadAudioStream(TAXA_DO_MOTOR_GRANULAR, 16, 1);
                vozesGranulares[v].valida = IsAudioStreamValid(vozesGranulares[v].fluxo);
                vozesGranulares[v].tocando = 0;
            }
            SetAudioStreamBufferSizeDefault(0);
            motorGranularPronto = vozesGranulares[0].valida;
        } else {
            TraceLog(LOG_WARNING, "MOTOR: a gravacao e curta demais (%d amostras)", destino);
        }
    }
    if (amostras != NULL) {
        UnloadWaveSamples(amostras);
    }
    UnloadWave(onda);
    if (!motorGranularPronto) {
        free(gravacaoDoMotor);
        gravacaoDoMotor = NULL;
    } else {
        TraceLog(LOG_INFO, "MOTOR: %s carregado (%.1f s); o motor dos carros e ele", ARQUIVO_DO_MOTOR, (float) totalDaGravacao / TAXA_DO_MOTOR_GRANULAR);
    }
}

static void descarregarMotorGranular(void) {
    for (int v = 0; v < VOZES_GRANULARES; v++) {
        if (vozesGranulares[v].valida) {
            StopAudioStream(vozesGranulares[v].fluxo);
            UnloadAudioStream(vozesGranulares[v].fluxo);
            vozesGranulares[v].valida = 0;
        }
        vozesGranulares[v].tocando = 0;
    }
    free(gravacaoDoMotor);
    gravacaoDoMotor = NULL;
    motorGranularPronto = 0;
}

static void ligarMotorGranular(int v) {
    VozGranular *voz = &vozesGranulares[v];
    if (motorGranularPronto && voz->valida && !voz->tocando) {
        iniciarMotorGranular(&voz->motor, gravacaoDoMotor, totalDaGravacao);
        PlayAudioStream(voz->fluxo);
        voz->tocando = 1;
    }
}

static void desligarMotorGranular(int v) {
    VozGranular *voz = &vozesGranulares[v];
    if (voz->tocando) {
        StopAudioStream(voz->fluxo);
        voz->tocando = 0;
    }
}

static void desligarTodosOsMotoresGranulares(void) {
    for (int v = 0; v < VOZES_GRANULARES; v++) {
        desligarMotorGranular(v);
    }
}

/*
 * atualizarMotorGranular
 * Diz a uma voz a marcha, a rotação, o volume e o lado de agora e enche os blocos de áudio que o fluxo dela já
 * terminou de tocar.
 * Recebe: a voz, a marcha, a rotação (0 a 1, pode passar de 1 com o Doppler), o volume (0 a 1) e o lado (0 a 1, 0,5 no meio).
 */
static void atualizarMotorGranular(int v, int marcha, float giro, float volume, float lado) {
    VozGranular *voz = &vozesGranulares[v];
    if (!voz->tocando) {
        return;
    }
    definirMotorGranular(&voz->motor, marcha, giro, volume);
    SetAudioStreamPan(voz->fluxo, lado);
    for (int bloco = 0; bloco < 4 && IsAudioStreamProcessed(voz->fluxo); bloco++) {
        float mono[PASSO_DO_MOTOR];
        short pcm[PASSO_DO_MOTOR];
        gerarMotorGranular(&voz->motor, mono, PASSO_DO_MOTOR);
        for (int i = 0; i < PASSO_DO_MOTOR; i++) {
            float x = mono[i] * VOLUME_DO_MOTOR_GRANULAR;
            x = x > 1.0f ? 1.0f : (x < -1.0f ? -1.0f : x);
            pcm[i] = (short) (x * 32767.0f);
        }
        UpdateAudioStream(voz->fluxo, pcm, PASSO_DO_MOTOR);
    }
}

/* ---------- Loops por estado ---------- */
enum { LOOP_VENTO, LOOP_PNEU, LOOP_GRAMA, LOOP_ZEBRA, LOOP_MURO, LOOP_NITRO, TOTAL_LOOPS };

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
    [LOOP_ZEBRA] = { .arquivo = "recursos/sons/efeitos/zebra.wav" },   /* ferramentas/gerar_som_da_zebra.py */
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
    int pneuEstourado;        /* os pneus do jogador que já estavam estourados no quadro anterior (um bit para cada roda) */
} EstadoDosEfeitos;

static EstadoDosEfeitos estado;

/* ---------- Batidas de lata (todos os carros) ----------
 * O "crash" de metal das batidas, como nos jogos de corrida: um tranco grave, o ruído da
 * lata amassando, pedacinhos voando e o metal ressoando. Os sons são gerados aqui mesmo,
 * em C, quando o jogo abre (não há arquivo): 4 variações de batida forte e 4 de batida leve. */
#define VARIACOES_DE_LATA        4
#define VOLUME_LATA              0.95f
#define ALCANCE_DA_LATA          90.0f   /* metros: batidas de rivais mais longe que isso não são ouvidas */
#define LATA_FORTE               0.55f   /* força do contato a partir da qual a batida é "forte" */
#define PAUSA_ENTRE_LATAS        0.07    /* segundos: os dois carros de uma batida fazem um som só */
#define TAXA_DA_LATA             44100

static Sound latasFortes[VARIACOES_DE_LATA];
static Sound latasLeves[VARIACOES_DE_LATA];
static int latasProntas = 0;
static int latasAntes[TOTAL_CARROS];   /* por número do carro: o contato já vinha do quadro anterior */
static double horaDaUltimaLata = -10.0;
static int ultimaLata = -1;

/* Gerador de números do som (cada variação tem a sua semente: o som sai sempre igual) */
static unsigned int sementeDaLata;
static float aleatorioDaLata(void) {
    sementeDaLata = sementeDaLata * 1664525u + 1013904223u;
    return (float) ((sementeDaLata >> 8) & 0xFFFFFF) / (float) 0xFFFFFF;   /* 0 a 1 */
}

/* Um filtro de banda (biquad), para dar "cor" ao ruído */
typedef struct { float b0, b1, b2, a1, a2, x1, x2, y1, y2; } FiltroDeBanda;

static FiltroDeBanda filtroDeBanda(float centro, float q) {
    float w0 = 2.0f * PI * centro / TAXA_DA_LATA;
    float alfa = sinf(w0) / (2.0f * q);
    float a0 = 1.0f + alfa;
    FiltroDeBanda f = { alfa / a0, 0.0f, -alfa / a0, -2.0f * cosf(w0) / a0, (1.0f - alfa) / a0, 0, 0, 0, 0 };
    return f;
}

static float filtrar(FiltroDeBanda *f, float x) {
    float y = f->b0 * x + f->b1 * f->x1 + f->b2 * f->x2 - f->a1 * f->y1 - f->a2 * f->y2;
    f->x2 = f->x1;
    f->x1 = x;
    f->y2 = f->y1;
    f->y1 = y;
    return y;
}

/*
 * gerarBatidaDeLata
 * Sintetiza o som de uma batida de carro: o tranco grave (o peso do carro), o ruído da lata
 * amassando (duas bandas de ruído), os pedacinhos voando (estalos curtos) e o metal ressoando
 * (tons sem harmonia que apagam devagar).
 * Recebe: a semente (cada uma dá uma variação) e se é uma batida forte.
 * Devolve: o som.
 */
static Sound gerarBatidaDeLata(unsigned int semente, int forte) {
    sementeDaLata = semente * 2654435761u + 12345u;
    float duracao = forte ? 0.95f : 0.5f;
    int n = (int) (duracao * TAXA_DA_LATA);
    float *x = calloc((size_t) n, sizeof(float));
    short *dados = malloc(sizeof(short) * (size_t) n);
    if (x == NULL || dados == NULL) {
        free(x);
        free(dados);
        return (Sound){ 0 };
    }

    /* O tranco grave, que cai de tom */
    float fase = 0.0f;
    for (int i = 0; i < n; i++) {
        float t = (float) i / TAXA_DA_LATA;
        float freq = forte ? 50.0f + 55.0f * expf(-t / 0.05f) : 85.0f + 60.0f * expf(-t / 0.03f);
        fase += 2.0f * PI * freq / TAXA_DA_LATA;
        x[i] += (forte ? 1.0f : 0.55f) * sinf(fase) * expf(-t / (forte ? 0.11f : 0.06f));
    }

    /* A lata amassando: ruído em duas bandas, uma média e uma aguda */
    FiltroDeBanda media = filtroDeBanda(forte ? 1500.0f : 2100.0f, 0.8f);
    FiltroDeBanda aguda = filtroDeBanda(4800.0f, 1.1f);
    for (int i = 0; i < n; i++) {
        float t = (float) i / TAXA_DA_LATA;
        float ruido = aleatorioDaLata() * 2.0f - 1.0f;
        float ataque = 1.0f - expf(-t / 0.0015f);
        x[i] += 2.2f * filtrar(&media, ruido) * ataque * expf(-t / (forte ? 0.09f : 0.05f));
        x[i] += 1.4f * filtrar(&aguda, ruido) * ataque * expf(-t / 0.025f);
    }

    /* Os pedacinhos voando: estalos curtos espalhados logo depois da batida */
    int estalos = forte ? 26 : 10;
    for (int k = 0; k < estalos; k++) {
        float quando = aleatorioDaLata() * (forte ? 0.38f : 0.18f);
        int inicio = (int) (quando * TAXA_DA_LATA);
        float forca = (0.25f + 0.5f * aleatorioDaLata()) * (1.0f - quando / 0.45f);
        int tamanho = (int) (0.004f * TAXA_DA_LATA);
        FiltroDeBanda clique = filtroDeBanda(2500.0f + 4000.0f * aleatorioDaLata(), 2.0f);
        for (int j = 0; j < tamanho && inicio + j < n; j++) {
            float ruido = aleatorioDaLata() * 2.0f - 1.0f;
            x[inicio + j] += 2.0f * forca * filtrar(&clique, ruido) * expf(-(float) j / (0.0012f * TAXA_DA_LATA));
        }
    }

    /* O metal ressoando: tons fora de harmonia, que apagam devagar */
    static const float TONS[5] = { 410.0f, 970.0f, 1560.0f, 2390.0f, 3420.0f };
    for (int k = 0; k < 5; k++) {
        float freq = TONS[k] * (0.92f + 0.16f * aleatorioDaLata());
        float peso = 0.10f + 0.12f * aleatorioDaLata();
        float apaga = (forte ? 0.18f : 0.10f) + 0.20f * aleatorioDaLata();
        float inicioDaFase = 2.0f * PI * aleatorioDaLata();
        for (int i = 0; i < n; i++) {
            float t = (float) i / TAXA_DA_LATA;
            x[i] += peso * sinf(2.0f * PI * freq * t + inicioDaFase) * expf(-t / apaga) * (1.0f - expf(-t / 0.003f));
        }
    }

    /* Corta os picos de leve (a "sujeira" de batida) e acerta o volume */
    float maior = 1e-6f;
    for (int i = 0; i < n; i++) {
        maior = fmaxf(maior, fabsf(x[i]));
    }
    for (int i = 0; i < n; i++) {
        float t = (float) i / TAXA_DA_LATA;
        float fim = fminf(1.0f, (duracao - t) / 0.02f);   /* apaga no finalzinho, sem estalo */
        float y = tanhf(1.6f * x[i] / maior) / tanhf(1.6f) * 0.9f * fim;
        dados[i] = (short) (y * 32767.0f);
    }
    free(x);

    Wave onda = { (unsigned int) n, TAXA_DA_LATA, 16, 1, dados };
    Sound som = LoadSoundFromWave(onda);
    free(dados);
    return som;
}

/*
 * tocarBatidasDeLata
 * No começo de cada contato (carro com carro ou carro no muro), de qualquer carro perto do
 * jogador, toca uma batida de lata: forte ou leve pela força, mais baixa quanto mais longe.
 * Os dois carros de uma mesma batida fazem um som só.
 * Recebe: a corrida e se a corrida está andando.
 * Devolve: nada.
 */
static void tocarBatidasDeLata(Corrida *corrida, int ativo) {
    if (!latasProntas) {
        return;
    }
    float melhorVolume = 0.0f, forcaDaMelhor = 0.0f;
    for (int i = -1; i < TOTAL_RIVAIS; i++) {
        Carro *carro = (i < 0) ? &corrida->jogador : &corrida->rivais[i];
        int numero = carro->numero;
        if (numero < 0 || numero >= TOTAL_CARROS) {
            continue;
        }
        int temContato = ativo && carro->contato > 0.0f;
        if (temContato && !latasAntes[numero]) {
            float perto = 1.0f;
            if (i >= 0) {
                float metros = fabsf(carro->distanciaPercorrida - corrida->jogador.distanciaPercorrida) / UNIDADES_POR_METRO;
                perto = fminf(fmaxf(1.0f - metros / ALCANCE_DA_LATA, 0.0f), 1.0f);
                perto *= perto;
            }
            float volume = perto * (0.35f + 0.65f * fminf(fmaxf(carro->contato, 0.0f), 1.0f));
            if (volume > melhorVolume) {
                melhorVolume = volume;
                forcaDaMelhor = carro->contato;
            }
        }
        latasAntes[numero] = temContato;
    }

    double agora = GetTime();
    if (melhorVolume < 0.02f || agora - horaDaUltimaLata < PAUSA_ENTRE_LATAS) {
        return;
    }
    int variacao;
    do {
        variacao = GetRandomValue(0, VARIACOES_DE_LATA - 1);
    } while (variacao == ultimaLata);
    ultimaLata = variacao;
    Sound som = (forcaDaMelhor >= LATA_FORTE) ? latasFortes[variacao] : latasLeves[variacao];
    SetSoundVolume(som, VOLUME_LATA * melhorVolume);
    SetSoundPitch(som, 0.9f + 0.2f * (float) GetRandomValue(0, 100) / 100.0f);
    StopSound(som);
    PlaySound(som);
    horaDaUltimaLata = agora;
}

/* ---------- Pneu estourado e fogos de artifício (sons gerados aqui, em C) ----------
 * Quatro sons que também não têm arquivo: o estouro do pneu (tiro seco + chiado do ar), a
 * pancada da borracha solta batendo no asfalto (repetida enquanto o carro roda no aro), o
 * assobio do foguete subindo e a explosão do fogo (estrondo grave + chuva de estalinhos). */
#define VARIACOES_DE_FOGO        3

static Sound somDoEstouro, somDaBorracha, somDoFoguete;
static Sound somDeFogoExplodindo[VARIACOES_DE_FOGO];
static int somDePneuPronto = 0, somDeFogosPronto = 0;
static double horaDaBorracha = 0.0;

/*
 * somDeBuffer
 * Normaliza o que foi sintetizado (corte suave dos picos, apagando no finalzinho) e vira Sound.
 * Recebe: as amostras (liberadas aqui), quantas são, a duração e o volume máximo (0 a 1).
 * Devolve: o som (vazio se faltou memória).
 */
static Sound somDeBuffer(float *x, int n, float duracao, float ganho) {
    short *dados = malloc(sizeof(short) * (size_t) n);
    if (dados == NULL) {
        free(x);
        return (Sound){ 0 };
    }
    float maior = 1e-6f;
    for (int i = 0; i < n; i++) {
        maior = fmaxf(maior, fabsf(x[i]));
    }
    for (int i = 0; i < n; i++) {
        float t = (float) i / TAXA_DA_LATA;
        float fim = fminf(1.0f, (duracao - t) / 0.02f);
        float y = tanhf(1.5f * x[i] / maior) / tanhf(1.5f) * ganho * fim;
        dados[i] = (short) (y * 32767.0f);
    }
    free(x);
    Wave onda = { (unsigned int) n, TAXA_DA_LATA, 16, 1, dados };
    Sound som = LoadSoundFromWave(onda);
    free(dados);
    return som;
}

/* O estouro do pneu: o tiro seco (grave que cai + estalo largo), o ar saindo (chiado agudo) e a borracha
 * batendo no asfalto nas primeiras voltas da roda. */
static Sound gerarEstouroDePneu(void) {
    sementeDaLata = 777u;
    float duracao = 1.3f;
    int n = (int) (duracao * TAXA_DA_LATA);
    float *x = calloc((size_t) n, sizeof(float));
    if (x == NULL) {
        return (Sound){ 0 };
    }
    FiltroDeBanda estalo = filtroDeBanda(1800.0f, 0.6f);
    FiltroDeBanda chiado = filtroDeBanda(5200.0f, 0.7f);
    FiltroDeBanda sopro = filtroDeBanda(900.0f, 0.9f);
    float fase = 0.0f;
    for (int i = 0; i < n; i++) {
        float t = (float) i / TAXA_DA_LATA;
        float ruido = aleatorioDaLata() * 2.0f - 1.0f;
        float freq = 42.0f + 90.0f * expf(-t / 0.035f);
        fase += 2.0f * PI * freq / TAXA_DA_LATA;
        x[i] += 1.6f * sinf(fase) * expf(-t / 0.14f);                                        /* o tranco grave */
        x[i] += 3.2f * filtrar(&estalo, ruido) * (1.0f - expf(-t / 0.0007f)) * expf(-t / 0.045f);   /* o estalo */
        x[i] += 1.5f * filtrar(&sopro, ruido) * expf(-t / 0.22f);                            /* o sopro do ar */
        x[i] += 1.1f * filtrar(&chiado, ruido) * (1.0f - expf(-t / 0.02f)) * expf(-t / 0.55f);     /* o chiado */
    }
    /* Pedaços de borracha batendo: pancadas surdas cada vez mais espaçadas */
    float quando = 0.09f;
    for (int k = 0; k < 7; k++) {
        int inicio = (int) (quando * TAXA_DA_LATA);
        float forca = 0.9f * expf(-quando / 0.35f);
        for (int j = 0; j < (int) (0.06f * TAXA_DA_LATA) && inicio + j < n; j++) {
            float tj = (float) j / TAXA_DA_LATA;
            x[inicio + j] += forca * sinf(2.0f * PI * (95.0f - 200.0f * tj) * tj) * expf(-tj / 0.02f);
        }
        quando += 0.07f + 0.035f * (float) k;
    }
    return somDeBuffer(x, n, duracao, 0.95f);
}

/* A pancada da borracha solta batendo no chão: uma só, curtinha (a corrida repete conforme a roda gira) */
static Sound gerarBatidaDeBorracha(void) {
    sementeDaLata = 4242u;
    float duracao = 0.13f;
    int n = (int) (duracao * TAXA_DA_LATA);
    float *x = calloc((size_t) n, sizeof(float));
    if (x == NULL) {
        return (Sound){ 0 };
    }
    FiltroDeBanda batida = filtroDeBanda(520.0f, 0.9f);
    float fase = 0.0f;
    for (int i = 0; i < n; i++) {
        float t = (float) i / TAXA_DA_LATA;
        fase += 2.0f * PI * (75.0f + 70.0f * expf(-t / 0.02f)) / TAXA_DA_LATA;
        float ruido = aleatorioDaLata() * 2.0f - 1.0f;
        x[i] += sinf(fase) * expf(-t / 0.035f);
        x[i] += 1.6f * filtrar(&batida, ruido) * (1.0f - expf(-t / 0.001f)) * expf(-t / 0.03f);
    }
    return somDeBuffer(x, n, duracao, 0.9f);
}

/* O foguete subindo: assobio que sobe de tom + o rastro de fogo (ruído) */
static Sound gerarFoguete(void) {
    sementeDaLata = 31337u;
    float duracao = 1.4f;
    int n = (int) (duracao * TAXA_DA_LATA);
    float *x = calloc((size_t) n, sizeof(float));
    if (x == NULL) {
        return (Sound){ 0 };
    }
    FiltroDeBanda fogo = filtroDeBanda(2600.0f, 0.5f);
    float fase = 0.0f;
    for (int i = 0; i < n; i++) {
        float t = (float) i / TAXA_DA_LATA;
        float u = t / duracao;
        float entra = fminf(t / 0.08f, 1.0f) * fminf((duracao - t) / 0.25f, 1.0f);
        fase += 2.0f * PI * (900.0f + 2300.0f * u * u) / TAXA_DA_LATA;
        float ruido = aleatorioDaLata() * 2.0f - 1.0f;
        x[i] += 0.45f * sinf(fase) * entra;
        x[i] += 1.1f * filtrar(&fogo, ruido) * entra * (0.6f + 0.4f * u);
    }
    return somDeBuffer(x, n, duracao, 0.55f);
}

/* A explosão do fogo: o estrondo (grave que cai, mais longo que o do pneu), o estouro de ruído e
 * os estalinhos das faíscas caindo, cada vez menos. */
static Sound gerarExplosaoDeFogo(unsigned int semente) {
    sementeDaLata = semente * 2654435761u + 99u;
    float duracao = 2.0f;
    int n = (int) (duracao * TAXA_DA_LATA);
    float *x = calloc((size_t) n, sizeof(float));
    if (x == NULL) {
        return (Sound){ 0 };
    }
    FiltroDeBanda estouro = filtroDeBanda(700.0f + 400.0f * aleatorioDaLata(), 0.5f);
    FiltroDeBanda brilho = filtroDeBanda(3500.0f, 0.8f);
    float fase = 0.0f;
    for (int i = 0; i < n; i++) {
        float t = (float) i / TAXA_DA_LATA;
        float ruido = aleatorioDaLata() * 2.0f - 1.0f;
        fase += 2.0f * PI * (38.0f + 70.0f * expf(-t / 0.06f)) / TAXA_DA_LATA;
        x[i] += 1.5f * sinf(fase) * expf(-t / 0.28f);
        x[i] += 3.0f * filtrar(&estouro, ruido) * (1.0f - expf(-t / 0.002f)) * expf(-t / 0.16f);
        x[i] += 1.0f * filtrar(&brilho, ruido) * (1.0f - expf(-t / 0.004f)) * expf(-t / 0.35f);
    }
    for (int k = 0; k < 90; k++) {   /* os estalinhos, mais seguidos no começo */
        float u = aleatorioDaLata();
        float quando = 0.12f + 1.6f * u * u;
        int inicio = (int) (quando * TAXA_DA_LATA);
        float forca = (0.15f + 0.45f * aleatorioDaLata()) * expf(-quando / 0.8f);
        FiltroDeBanda clique = filtroDeBanda(2800.0f + 4500.0f * aleatorioDaLata(), 2.0f);
        for (int j = 0; j < (int) (0.003f * TAXA_DA_LATA) && inicio + j < n; j++) {
            float ruido = aleatorioDaLata() * 2.0f - 1.0f;
            x[inicio + j] += 1.8f * forca * filtrar(&clique, ruido) * expf(-(float) j / (0.0008f * TAXA_DA_LATA));
        }
    }
    return somDeBuffer(x, n, duracao, 0.9f);
}

/*
 * tocarSomDeFogos
 * Toca o foguete subindo ou o fogo explodindo (uma das variações, com o tom um pouco diferente).
 * Recebe: 1 para explosão e 0 para foguete, o volume (0 a 1) e o lado (0 esquerda, 0,5 meio, 1 direita).
 * Devolve: nada.
 */
void tocarSomDeFogos(int explosao, float volume, float lado) {
    if (!somDeFogosPronto || volume < 0.01f) {
        return;
    }
    Sound som = explosao ? somDeFogoExplodindo[GetRandomValue(0, VARIACOES_DE_FOGO - 1)] : somDoFoguete;
    SetSoundVolume(som, fminf(volume, 1.0f));
    SetSoundPitch(som, 0.88f + 0.24f * (float) GetRandomValue(0, 100) / 100.0f);
    SetSoundPan(som, fminf(fmaxf(lado, 0.0f), 1.0f));
    PlaySound(som);
}

/*
 * tocarPneuEstourado
 * Enquanto o carro roda no aro: a borracha solta batendo no asfalto, mais rápido quanto mais veloz.
 * Recebe: o carro do jogador e se a corrida está andando.
 * Devolve: nada.
 */
static void tocarPneuEstourado(const Carro *jogador, int ativo) {
    if (!somDePneuPronto || !ativo || !jogador->pneuEstourado || carroNoPit(jogador) || jogador->velocidade < 300.0f) {
        return;
    }
    float f = fminf(jogador->velocidade / jogador->velocidadeMaxima, 1.0f);
    double agora = GetTime();
    int estourados = quantosPneusEstourados(jogador);
    if (agora - horaDaBorracha < 1.0 / ((3.0 + 8.0 * f) * (1.0 + 0.35 * (estourados - 1)))) {   /* mais pneus no aro, mais batidas */
        return;
    }
    horaDaBorracha = agora;
    SetSoundVolume(somDaBorracha, 0.55f + 0.3f * f);
    SetSoundPitch(somDaBorracha, 0.8f + 0.5f * f + 0.15f * (float) GetRandomValue(0, 100) / 100.0f);
    PlaySound(somDaBorracha);
}

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
    carregarMotorGranular();
    for (int v = 0; v < VARIACOES_DE_LATA; v++) {
        latasFortes[v] = gerarBatidaDeLata(100u + (unsigned int) v, 1);
        latasLeves[v] = gerarBatidaDeLata(200u + (unsigned int) v, 0);
    }
    latasProntas = 1;
    for (int v = 0; v < VARIACOES_DE_LATA; v++) {
        latasProntas = latasProntas && IsSoundValid(latasFortes[v]) && IsSoundValid(latasLeves[v]);
    }
    somDoEstouro = gerarEstouroDePneu();
    somDaBorracha = gerarBatidaDeBorracha();
    somDePneuPronto = IsSoundValid(somDoEstouro) && IsSoundValid(somDaBorracha);
    somDoFoguete = gerarFoguete();
    somDeFogosPronto = IsSoundValid(somDoFoguete);
    for (int v = 0; v < VARIACOES_DE_FOGO; v++) {
        somDeFogoExplodindo[v] = gerarExplosaoDeFogo(300u + (unsigned int) v);
        somDeFogosPronto = somDeFogosPronto && IsSoundValid(somDeFogoExplodindo[v]);
    }
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
    descarregarMotorGranular();
    for (int v = 0; v < VARIACOES_DE_LATA; v++) {
        if (IsSoundValid(latasFortes[v])) {
            UnloadSound(latasFortes[v]);
        }
        if (IsSoundValid(latasLeves[v])) {
            UnloadSound(latasLeves[v]);
        }
    }
    latasProntas = 0;
    if (IsSoundValid(somDoEstouro)) {
        UnloadSound(somDoEstouro);
    }
    if (IsSoundValid(somDaBorracha)) {
        UnloadSound(somDaBorracha);
    }
    if (IsSoundValid(somDoFoguete)) {
        UnloadSound(somDoFoguete);
    }
    for (int v = 0; v < VARIACOES_DE_FOGO; v++) {
        if (IsSoundValid(somDeFogoExplodindo[v])) {
            UnloadSound(somDeFogoExplodindo[v]);
        }
    }
    somDePneuPronto = 0;
    somDeFogosPronto = 0;
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
    desligarTodosOsMotoresGranulares();
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
    for (int v = 0; v < VOZES_GRANULARES; v++) {
        if (vozesGranulares[v].tocando) {
            pausar ? PauseAudioStream(vozesGranulares[v].fluxo) : ResumeAudioStream(vozesGranulares[v].fluxo);
        }
    }
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
            if (vozes[v].faixas[f].carregada && vozes[v].tocando && !vozesGranulares[1 + v].valida) {
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
        ligarMotorGranular(0);
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
        desligarMotorGranular(0);
        return;
    }

    float giro = limitar(jogador->motor.giro, 0.0f, 1.0f);
    float carga = limitar(jogador->motor.carga, 0.0f, 1.0f);
    float rpm = giro * RPM_MAXIMO;
    float forca = VOLUME_MOTOR_DO_JOGADOR * entradaDoMotor * (0.55f + 0.45f * giro);
    if (vozesGranulares[0].tocando) {
        /* O motor do jogador é o da gravação: o volume segue a entrada, a rotação e a carga (pé no fundo ou levantado) */
        atualizarMotorGranular(0, jogador->motor.marcha, giro, entradaDoMotor * (0.55f + 0.45f * giro) * (0.35f + 0.65f * sqrtf(carga)), 0.5f);
        forca *= NASCAR_SOB_O_MOTOR;
    }

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
        int comGravacao = vozesGranulares[1 + v].valida;   /* o motor do rival também é a gravação, quando ela existe */
        if (voz->volume > 0.002f && !voz->tocando) {
            if (comGravacao) {
                ligarMotorGranular(1 + v);
            } else {
                for (int f = 0; f < FAIXAS_DE_RIVAL; f++) {
                    tocarFaixa(&voz->faixas[f]);
                }
            }
            voz->tocando = 1;
        } else if (voz->volume <= 0.002f && alvo <= 0.0f && voz->tocando) {
            if (comGravacao) {
                desligarMotorGranular(1 + v);
            } else {
                for (int f = 0; f < FAIXAS_DE_RIVAL; f++) {
                    pararFaixa(&voz->faixas[f]);
                }
            }
            voz->tocando = 0;
        }
        if (voz->tocando) {
            if (comGravacao) {
                const Carro *dono = (voz->rival >= 0) ? &corrida->rivais[voz->rival] : NULL;
                /* o Doppler muda o tom do mesmo jeito que mudaria a rotação */
                atualizarMotorGranular(1 + v, (dono != NULL) ? dono->motor.marcha : 0,
                                       (dono != NULL) ? dono->motor.giro * voz->doppler : 0.2f, voz->volume, lado);
            } else {
                misturarFaixas(voz->faixas, FAIXAS_DE_RIVAL, rpm, voz->volume, lado, voz->doppler);
            }
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

    /* A grama e a zebra: o chão embaixo das rodas (a área de escape é asfalto) */
    Superficie embaixo = superficieDoCarro(jogador);
    int naGrama = ativo && embaixo == SUPERFICIE_GRAMA && f > 0.04f && !carroNoPit(jogador);
    atualizarLoop(&loops[LOOP_GRAMA], naGrama ? VOLUME_GRAMA * powf(f, 0.7f) : 0.0f, 0.7f + 0.7f * f, tempo);
    int naZebra = ativo && embaixo == SUPERFICIE_ZEBRA && f > 0.04f && !carroNoPit(jogador);
    atualizarLoop(&loops[LOOP_ZEBRA], naZebra ? VOLUME_ZEBRA * powf(f, 0.5f) : 0.0f, 0.55f + 1.0f * f, tempo);

    /* O muro */
    int raspando = ativo && f > 0.03f && (jogador->raspandoNoMuro ||
                                         (jogador->contatoComCarro && jogador->contatoDeslize > 1.5f) ||
                                         (jogador->pneuEstourado && !carroNoPit(jogador)));   /* o aro no asfalto */
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
    estado.pneuEstourado = jogador->pneuEstourado;
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

    /* O pneu estoura: o tiro e o chiado (depois, a borracha batendo vem de tocarPneuEstourado) */
    if ((jogador->pneuEstourado & ~estado.pneuEstourado) && somDePneuPronto) {   /* cada pneu que estoura faz o seu estouro */
        int primeiro = (estado.pneuEstourado == 0);
        SetSoundVolume(somDoEstouro, primeiro ? 1.0f : 0.85f);
        SetSoundPitch(somDoEstouro, primeiro ? 1.0f : 0.88f + 0.08f * (float) quantosPneusEstourados(jogador));
        PlaySound(somDoEstouro);
    }
    estado.pneuEstourado = jogador->pneuEstourado;

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
    tocarBatidasDeLata(corrida, ativo);
    tocarPneuEstourado(jogador, ativo);

    if (!estado.pronto || !ativo) {
        sincronizarEstado(jogador);   /* sem som: só acompanha, para ouvir apenas o que mudar depois */
    } else {
        dispararEfeitosCurtos(jogador);
    }
}

/*
 * atualizarEfeitosDoPasseio
 * Os sons do modo livre: o motor do jogador, o vento, o pneu escorregando, a areia e a parede.
 * Não há pista nem rivais: os loops seguem o que o carro livre conta.
 * Recebe: o carro (motor, velocidade e nitro; a velocidade nas unidades da corrida),
 *         o quanto ele escorrega (0 a 1), se raspa numa parede, o quanto o chão é de terra solta (0 a 1),
 *         se os sons valem e o tempo do quadro.
 * Devolve: nada.
 */
void atualizarEfeitosDoPasseio(const Carro *carro, float escorrega, int raspando, float naTerra, int ativo, float tempoQuadro) {
    atualizarMotorDoJogador(ativo, carro, tempoQuadro);
    float f = ativo ? limitar(carro->velocidade / carro->velocidadeMaxima, 0.0f, 1.0f) : 0.0f;
    float vento = VOLUME_VENTO * f * f * (1.0f + (carro->nitroLigado ? 0.25f : 0.0f));
    atualizarLoop(&loops[LOOP_VENTO], ativo ? vento : 0.0f, 0.75f + 0.55f * f, tempoQuadro);
    float pneu = VOLUME_PNEU * limitar(escorrega, 0.0f, 1.0f) * limitar(f / 0.2f, 0.0f, 1.0f);
    atualizarLoop(&loops[LOOP_PNEU], ativo ? pneu : 0.0f, 0.85f + 0.35f * f, tempoQuadro);
    atualizarLoop(&loops[LOOP_GRAMA], (ativo && f > 0.04f) ? VOLUME_GRAMA * limitar(naTerra, 0.0f, 1.0f) * powf(f, 0.7f) : 0.0f, 0.7f + 0.7f * f, tempoQuadro);
    atualizarLoop(&loops[LOOP_ZEBRA], 0.0f, 1.0f, tempoQuadro);
    atualizarLoop(&loops[LOOP_MURO], (ativo && raspando && f > 0.03f) ? VOLUME_MURO * powf(f, 0.6f) : 0.0f, 0.8f + 0.4f * f, tempoQuadro);
    atualizarLoop(&loops[LOOP_NITRO], (ativo && carro->nitroLigado) ? VOLUME_NITRO : 0.0f, 0.9f + 0.3f * f, tempoQuadro);
}
