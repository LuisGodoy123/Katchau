/*
 * audio.c
 * ------------------------------------------------------------------
 * Sons e música dos menus. Veja audio.h.
 * ------------------------------------------------------------------
 */

#include "raylib.h"
#include "audio.h"

#define ARQUIVO_MUSICA_MENU   "recursos/sons/menu_theme.mp3"
#define ARQUIVO_SOM_CURSOR    "recursos/sons/cursor.wav"
#define VOLUME_SOM_CURSOR     0.7f
#define VOLUME_MUSICA_MENU    0.6f   /* volume máximo da música (0 a 1) */
#define VELOCIDADE_FADE       1.5f   /* 1/s: o fade leva ~0,7 s */

static Sound somCursor;
static int somCursorCarregado = 0;
static Music musicaMenu;
static int musicaCarregada = 0;
static int iniciada = 0;     /* já foi dado o Play alguma vez */
static int pausada = 0;      /* está pausada (volume zerado fora dos menus) */
static float fade = 0.0f;    /* 0 (mudo) a 1 (volume máximo) */

/*
 * carregarAudio
 * Abre o dispositivo de som e lê a música do menu. Chamar depois de abrir
 * a janela. Se não houver som ou o arquivo faltar, o jogo segue mudo: nunca
 * quebra por falta de arquivo.
 * Recebe: nada.
 * Devolve: nada.
 */
void carregarAudio(void) {
    InitAudioDevice();
    if (!IsAudioDeviceReady()) {
        return;
    }

    if (FileExists(ARQUIVO_SOM_CURSOR)) {
        somCursor = LoadSound(ARQUIVO_SOM_CURSOR);
        somCursorCarregado = IsSoundValid(somCursor);
        if (somCursorCarregado) {
            SetSoundVolume(somCursor, VOLUME_SOM_CURSOR);
        }
    }

    if (!FileExists(ARQUIVO_MUSICA_MENU)) {
        return;
    }
    musicaMenu = LoadMusicStream(ARQUIVO_MUSICA_MENU);
    musicaCarregada = IsMusicValid(musicaMenu);
    if (musicaCarregada) {
        musicaMenu.looping = true;
    }
}

/*
 * atualizarAudio
 * Chamar uma vez por quadro. Sobe o volume nos menus e desce fora deles;
 * quando chega a zero, pausa a música para retomá-la de onde parou.
 * Recebe: 1 se a tela atual é de menu, 0 se não.
 * Devolve: nada.
 */
void atualizarAudio(int musicaDoMenu) {
    if (!musicaCarregada) {
        return;
    }

    float passo = GetFrameTime() * VELOCIDADE_FADE;
    if (musicaDoMenu) {
        fade = (fade + passo > 1.0f) ? 1.0f : fade + passo;
        if (!iniciada) {
            PlayMusicStream(musicaMenu);
            iniciada = 1;
        } else if (pausada) {
            ResumeMusicStream(musicaMenu);
            pausada = 0;
        }
    } else {
        fade = (fade - passo < 0.0f) ? 0.0f : fade - passo;
        if (fade == 0.0f && iniciada && !pausada) {
            PauseMusicStream(musicaMenu);
            pausada = 1;
        }
    }

    SetMusicVolume(musicaMenu, fade * VOLUME_MUSICA_MENU);
    if (iniciada && !pausada) {
        UpdateMusicStream(musicaMenu);   /* alimenta o som; precisa de uma chamada por quadro */
    }
}

/*
 * tocarSomCursor
 * O "tic" de quando o cursor passa para outra opção do menu. Se tocar de
 * novo antes de acabar, recomeça (passar rápido pelas opções não empilha sons).
 * Recebe: nada.
 * Devolve: nada.
 */
void tocarSomCursor(void) {
    if (somCursorCarregado) {
        StopSound(somCursor);
        PlaySound(somCursor);
    }
}

/*
 * descarregarAudio
 * Devolve a memória dos sons e fecha o dispositivo de som. Chamar antes
 * de CloseWindow.
 * Recebe: nada.
 * Devolve: nada.
 */
void descarregarAudio(void) {
    if (somCursorCarregado) {
        UnloadSound(somCursor);
        somCursorCarregado = 0;
    }
    if (musicaCarregada) {
        UnloadMusicStream(musicaMenu);
        musicaCarregada = 0;
    }
    CloseAudioDevice();
}
