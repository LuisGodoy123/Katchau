/*
 * grilos.c
 * ------------------------------------------------------------------
 * Os grilos da noite (veja grilos.h). Cada grilo canta trios de pulsos
 * curtos (um tom de 4 a 5 kHz que sobe e desce em ~18 ms), repetidos no
 * ritmo dele, com um pouco de variação. O laço tem LACO segundos e os
 * cantos que passam do fim continuam no começo, para não haver emenda.
 * ------------------------------------------------------------------
 */

#include "grilos.h"

#include <math.h>
#include <stdlib.h>

#include "raylib.h"

#define TAXA                  44100
#define LACO                  7.0f      /* segundos */
#define VOLUME_DOS_GRILOS     0.32f
#define PULSOS_POR_CANTO      3
#define DURACAO_DO_PULSO      0.018f
#define ENTRE_PULSOS          0.022f

typedef struct {
    float tom;           /* Hz */
    float periodo;       /* segundos entre um canto e o outro */
    float volume;
    float comeco;        /* o primeiro canto, em segundos */
} Grilo;

static const Grilo GRILOS[] = {
    { 4300.0f, 0.55f, 0.24f, 0.00f },
    { 4720.0f, 0.71f, 0.15f, 0.21f },
    { 5080.0f, 0.63f, 0.18f, 0.37f },
    { 3920.0f, 0.93f, 0.10f, 0.12f },
    { 4510.0f, 1.17f, 0.08f, 0.53f },
};
#define TOTAL_DE_GRILOS (int) (sizeof(GRILOS) / sizeof(GRILOS[0]))

static Sound grilos;
static int carregados = 0, tocando = 0;

/* Um acaso que se repete sempre igual (o som é o mesmo a cada vez que o jogo abre) */
static float acaso(unsigned int *semente) {
    *semente = *semente * 1664525u + 1013904223u;
    return (float) ((*semente >> 8) & 0xFFFF) / 65535.0f;
}

void carregarGrilos(void) {
    if (carregados) {
        return;
    }
    int amostras = (int) (TAXA * LACO);
    float *soma = (float *) calloc((size_t) amostras, sizeof(float));
    if (soma == NULL) {
        return;
    }
    unsigned int semente = 20061;
    for (int g = 0; g < TOTAL_DE_GRILOS; g++) {
        const Grilo *grilo = &GRILOS[g];
        for (float t = grilo->comeco; t < LACO; t += grilo->periodo * (0.9f + 0.2f * acaso(&semente))) {
            if (acaso(&semente) < 0.12f) {
                continue;   /* às vezes ele fica quieto um canto */
            }
            float forca = grilo->volume * (0.75f + 0.25f * acaso(&semente));
            for (int p = 0; p < PULSOS_POR_CANTO; p++) {
                float inicio = t + p * (DURACAO_DO_PULSO + ENTRE_PULSOS);
                int primeiro = (int) (inicio * TAXA), total = (int) (DURACAO_DO_PULSO * TAXA);
                for (int i = 0; i < total; i++) {
                    float s = (float) i / TAXA;
                    float envelope = sinf(PI * i / total);
                    envelope *= envelope;
                    float tom = grilo->tom * (1.0f + 0.03f * sinf(PI * i / total));   /* o tom sobe e desce no pulso */
                    float valor = sinf(2.0f * PI * tom * s) * envelope * forca;
                    soma[(primeiro + i) % amostras] += valor;
                }
            }
        }
    }
    short *dados = (short *) MemAlloc((unsigned int) amostras * sizeof(short));
    for (int i = 0; i < amostras; i++) {
        float v = soma[i];
        v = v / (1.0f + fabsf(v));   /* sem estourar onde os cantos se encontram */
        dados[i] = (short) (v * 32000.0f);
    }
    free(soma);
    Wave onda = { (unsigned int) amostras, TAXA, 16, 1, dados };
    grilos = LoadSoundFromWave(onda);
    UnloadWave(onda);
    carregados = grilos.frameCount > 0;
}

void descarregarGrilos(void) {
    if (!carregados) {
        return;
    }
    StopSound(grilos);
    UnloadSound(grilos);
    carregados = 0;
    tocando = 0;
}

void atualizarGrilos(float noite, int ativo) {
    if (!carregados) {
        return;
    }
    float volume = VOLUME_DOS_GRILOS * powf(fminf(fmaxf(noite, 0.0f), 1.0f), 1.5f);
    if (!ativo || volume < 0.005f) {
        if (tocando) {
            PauseSound(grilos);
            tocando = 0;
        }
        return;
    }
    SetSoundVolume(grilos, volume);
    if (!tocando) {
        ResumeSound(grilos);
        tocando = 1;
    }
    if (!IsSoundPlaying(grilos)) {
        PlaySound(grilos);   /* acabou o laço: de novo */
    }
}
