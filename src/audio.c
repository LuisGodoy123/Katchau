/*
 * audio.c
 * ------------------------------------------------------------------
 * Sons e música dos menus. Veja audio.h.
 * ------------------------------------------------------------------
 */

#include "raylib.h"
#include "audio.h"

#define ARQUIVO_MUSICA_MENU   "recursos/sons/menu_theme.mp3"
#define VOLUME_SONS           0.7f   /* volume dos efeitos (0 a 1) */
#define VOLUME_MUSICA_MENU    0.6f   /* volume máximo da música (0 a 1) */
#define VELOCIDADE_FADE       1.5f   /* 1/s: o fade leva ~0,7 s */
#define ARQUIVO_MOTOR_LARGADA "recursos/sons/motor_largada.wav"
#define VOLUME_MOTOR_LARGADA  0.8f   /* volume do motor esquentando (0 a 1) */
#define VELOCIDADE_FADE_MOTOR 3.0f   /* 1/s: ao acabar a largada, o motor some em ~0,3 s */

static const char *ARQUIVOS_SONS[TOTAL_SONS] = {
    "recursos/sons/cursor.wav",
    "recursos/sons/confirmar.wav",
    "recursos/sons/voltar.wav"
};
static Sound sons[TOTAL_SONS];
static int somCarregado[TOTAL_SONS];
static Music motorLargada;   /* tocado como "música" para poder baixar o volume aos poucos */
static int motorCarregado = 0;
static int motorTocando = 0;
static int largadaAtivaAntes = 0;
static float volumeMotor = 0.0f;
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

    for (int i = 0; i < TOTAL_SONS; i++) {
        if (FileExists(ARQUIVOS_SONS[i])) {
            sons[i] = LoadSound(ARQUIVOS_SONS[i]);
            somCarregado[i] = IsSoundValid(sons[i]);
            if (somCarregado[i]) {
                SetSoundVolume(sons[i], VOLUME_SONS);
            }
        }
    }

    if (FileExists(ARQUIVO_MOTOR_LARGADA)) {
        motorLargada = LoadMusicStream(ARQUIVO_MOTOR_LARGADA);
        motorCarregado = IsMusicValid(motorLargada);
        if (motorCarregado) {
            motorLargada.looping = false;
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
 * tocarSom
 * Toca um efeito dos menus. Se o mesmo som tocar de novo antes de acabar,
 * recomeça (passar rápido pelas opções não empilha sons).
 * Recebe: qual som.
 * Devolve: nada.
 */
void tocarSom(TipoSom som) {
    if (somCarregado[som]) {
        StopSound(sons[som]);
        PlaySound(sons[som]);
    }
}

/*
 * atualizarSomDaLargada
 * Chamar uma vez por quadro. O som do motor esquentando começa do início
 * quando a contagem começa e, quando a largada acaba, some em poucos
 * décimos de segundo. (Provisório: não é o som definitivo do carro.)
 * Recebe: 1 durante o 3, 2, 1 e o KATCHAU!, 0 no resto do jogo.
 * Devolve: nada.
 */
void atualizarSomDaLargada(int ativo) {
    if (!motorCarregado) {
        return;
    }

    if (ativo && !largadaAtivaAntes) {
        StopMusicStream(motorLargada);
        PlayMusicStream(motorLargada);
        motorTocando = 1;
        volumeMotor = 1.0f;
    } else if (!ativo && motorTocando) {
        volumeMotor -= GetFrameTime() * VELOCIDADE_FADE_MOTOR;
        if (volumeMotor <= 0.0f) {
            StopMusicStream(motorLargada);
            motorTocando = 0;
            volumeMotor = 0.0f;
        }
    }
    largadaAtivaAntes = ativo;

    if (motorTocando) {
        SetMusicVolume(motorLargada, volumeMotor * VOLUME_MOTOR_LARGADA);
        UpdateMusicStream(motorLargada);
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
    for (int i = 0; i < TOTAL_SONS; i++) {
        if (somCarregado[i]) {
            UnloadSound(sons[i]);
            somCarregado[i] = 0;
        }
    }
    if (motorCarregado) {
        UnloadMusicStream(motorLargada);
        motorCarregado = 0;
    }
    if (musicaCarregada) {
        UnloadMusicStream(musicaMenu);
        musicaCarregada = 0;
    }
    CloseAudioDevice();
}
