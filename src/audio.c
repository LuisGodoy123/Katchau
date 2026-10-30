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
#define VELOCIDADE_FADE_LARGADA 3.0f   /* 1/s: ao acabar, o som da largada some em ~0,3 s */

/* Os sons que acompanham a largada. Cada um começa do início quando o
 * jogo manda e some depressa quando o jogo avisa que acabou. São tocados
 * como "música" (e não como Sound) para poder baixar o volume aos poucos. */
typedef struct {
    const char *arquivo;
    float volumeMaximo;   /* 0 a 1 */
    Music musica;
    int carregado;
    int tocando;
    int ativoAntes;
    float volume;         /* 0 a 1: o fade de saída */
} SomDaLargada;

enum { LARGADA_MOTOR, LARGADA_CONTAGEM, TOTAL_SONS_LARGADA };

static SomDaLargada somLargada[TOTAL_SONS_LARGADA] = {
    [LARGADA_MOTOR] = { .arquivo = "recursos/sons/motor_largada.wav", .volumeMaximo = 0.5f },   /* provisório */
    [LARGADA_CONTAGEM] = { .arquivo = "recursos/sons/contagem_largada.mp3", .volumeMaximo = 1.0f },
};

static const char *ARQUIVOS_SONS[TOTAL_SONS] = {
    "recursos/sons/cursor.wav",
    "recursos/sons/confirmar.wav",
    "recursos/sons/voltar.wav"
};
static Sound sons[TOTAL_SONS];
static int somCarregado[TOTAL_SONS];
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

    for (int i = 0; i < TOTAL_SONS_LARGADA; i++) {
        if (FileExists(somLargada[i].arquivo)) {
            somLargada[i].musica = LoadMusicStream(somLargada[i].arquivo);
            somLargada[i].carregado = IsMusicValid(somLargada[i].musica);
            if (somLargada[i].carregado) {
                somLargada[i].musica.looping = false;
            }
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
 * Um quadro de um dos sons da largada: começa do início quando "ativo"
 * passa de 0 para 1 e, quando volta a 0, some em poucos décimos de segundo.
 * Recebe: o som e se ele deve estar tocando.
 * Devolve: nada.
 */
static void atualizarSomDaLargada(SomDaLargada *som, int ativo) {
    if (!som->carregado) {
        return;
    }

    if (ativo && !som->ativoAntes) {
        StopMusicStream(som->musica);
        PlayMusicStream(som->musica);
        som->tocando = 1;
        som->volume = 1.0f;
    } else if (!ativo && som->tocando) {
        som->volume -= GetFrameTime() * VELOCIDADE_FADE_LARGADA;
        if (som->volume <= 0.0f) {
            StopMusicStream(som->musica);
            som->tocando = 0;
            som->volume = 0.0f;
        }
    }
    som->ativoAntes = ativo;

    if (som->tocando) {
        SetMusicVolume(som->musica, som->volume * som->volumeMaximo);
        UpdateMusicStream(som->musica);
    }
}

/*
 * atualizarSonsDaLargada
 * Chamar uma vez por quadro.
 *   - o motor esquentando (provisório, até haver o som definitivo do
 *     carro) toca desde que a corrida abre até o fim do KATCHAU!;
 *   - o "3... 2... 1... GO!" toca a partir do momento em que o número 3
 *     aparece, e seus bips caem junto com os números na tela.
 * Recebe: se o motor deve tocar e se a contagem deve tocar (1 ou 0).
 * Devolve: nada.
 */
void atualizarSonsDaLargada(int motorAtivo, int contagemAtiva) {
    atualizarSomDaLargada(&somLargada[LARGADA_MOTOR], motorAtivo);
    atualizarSomDaLargada(&somLargada[LARGADA_CONTAGEM], contagemAtiva);
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
    for (int i = 0; i < TOTAL_SONS_LARGADA; i++) {
        if (somLargada[i].carregado) {
            UnloadMusicStream(somLargada[i].musica);
            somLargada[i].carregado = 0;
        }
    }
    if (musicaCarregada) {
        UnloadMusicStream(musicaMenu);
        musicaCarregada = 0;
    }
    CloseAudioDevice();
}
