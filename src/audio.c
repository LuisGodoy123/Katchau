/*
 * audio.c
 * ------------------------------------------------------------------
 * Sons e música dos menus. Veja audio.h.
 * ------------------------------------------------------------------
 */

#include <math.h>
#include "raylib.h"
#include "audio.h"

#define ARQUIVO_MUSICA_MENU   "recursos/sons/menu_theme.mp3"
#define VOLUME_SONS           0.7f   /* volume dos efeitos (0 a 1) */
#define VOLUME_MUSICA_MENU    0.6f   /* volume máximo da música (0 a 1) */
#define VELOCIDADE_FADE       1.5f   /* 1/s: o fade leva ~0,7 s */
#define VELOCIDADE_FADE_PADRAO 3.0f   /* 1/s: ao acabar, o som da largada some em ~0,3 s */

/* Os sons da corrida: o motor e a contagem da largada e a música de fundo.
 * Cada um começa do início quando o jogo manda e some aos poucos quando o
 * jogo avisa que acabou. São tocados como "música" (e não como Sound) para
 * poder baixar o volume aos poucos. */
typedef struct {
    const char *arquivo;
    float volumeMaximo;     /* 0 a 1 */
    float velocidadeFade;   /* 1/s na saída; 0 usa VELOCIDADE_FADE_PADRAO */
    int repetir;            /* 1 para recomeçar sozinho quando chega ao fim */
    Music musica;
    int carregado;
    int tocando;
    int ativoAntes;
    float volume;         /* 0 a 1: o fade de saída */
} SomDeCorrida;

enum { CORRIDA_MOTOR, CORRIDA_CONTAGEM, CORRIDA_MUSICA, TOTAL_SONS_CORRIDA };

static SomDeCorrida somCorrida[TOTAL_SONS_CORRIDA] = {
    [CORRIDA_MOTOR] = { .arquivo = "recursos/sons/motor_largada.wav", .volumeMaximo = 0.5f },   /* provisório */
    [CORRIDA_CONTAGEM] = { .arquivo = "recursos/sons/contagem_largada.mp3", .volumeMaximo = 1.0f },
    [CORRIDA_MUSICA] = { .arquivo = "recursos/sons/musica_corrida.mp3", .volumeMaximo = 0.5f,
                         .velocidadeFade = 1.0f, .repetir = 1 },
};

/* O motor do carro: dois loops da mesma rotação, um com o motor sob carga e
 * outro aliviado (freio-motor). Os dois tocam sempre e o volume de cada um
 * segue o pé do piloto; o tom dos dois segue a rotação. */
#define ARQUIVO_MOTOR_ACELERANDO "recursos/sons/motor_acelerando.wav"
#define ARQUIVO_MOTOR_ALIVIANDO  "recursos/sons/motor_aliviando.wav"
#define VOLUME_MOTOR             0.65f   /* volume máximo do motor (0 a 1) */
#define VOLUME_ALIVIADO          0.80f   /* o loop aliviado soa mais forte que o outro; aqui ele é baixado */
#define PITCH_MARCHA_LENTA       0.70f   /* tom dos loops com a rotação mínima (1.0 = como gravado) */
#define PITCH_LIMITADOR          2.00f   /* ...e no limitador */
#define VELOCIDADE_ENTRADA_MOTOR 4.0f    /* 1/s: o motor entra em ~0,25 s */
#define VELOCIDADE_SAIDA_MOTOR   3.0f    /* 1/s: e sai em ~0,3 s */

static Music motorAcelerando;
static Music motorAliviando;
static int motorSomCarregado = 0;
static int motorSomTocando = 0;
static int motorSomAtivoAntes = 0;
static float entradaMotor = 0.0f;   /* 0 a 1: o fade de entrada e saída */

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

    for (int i = 0; i < TOTAL_SONS_CORRIDA; i++) {
        if (FileExists(somCorrida[i].arquivo)) {
            somCorrida[i].musica = LoadMusicStream(somCorrida[i].arquivo);
            somCorrida[i].carregado = IsMusicValid(somCorrida[i].musica);
            if (somCorrida[i].carregado) {
                somCorrida[i].musica.looping = somCorrida[i].repetir;
            }
        }
    }

    if (FileExists(ARQUIVO_MOTOR_ACELERANDO) && FileExists(ARQUIVO_MOTOR_ALIVIANDO)) {
        motorAcelerando = LoadMusicStream(ARQUIVO_MOTOR_ACELERANDO);
        motorAliviando = LoadMusicStream(ARQUIVO_MOTOR_ALIVIANDO);
        motorSomCarregado = IsMusicValid(motorAcelerando) && IsMusicValid(motorAliviando);
        motorAcelerando.looping = true;
        motorAliviando.looping = true;
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
 * atualizarSomDeCorrida
 * Um quadro de um dos sons da corrida: começa do início quando "ativo"
 * passa de 0 para 1 e, quando volta a 0, vai sumindo até parar.
 * Recebe: o som e se ele deve estar tocando.
 * Devolve: nada.
 */
static void atualizarSomDeCorrida(SomDeCorrida *som, int ativo) {
    if (!som->carregado) {
        return;
    }

    if (ativo && !som->ativoAntes) {
        StopMusicStream(som->musica);
        PlayMusicStream(som->musica);
        som->tocando = 1;
        som->volume = 1.0f;
    } else if (!ativo && som->tocando) {
        float velocidade = (som->velocidadeFade > 0.0f) ? som->velocidadeFade : VELOCIDADE_FADE_PADRAO;
        som->volume -= GetFrameTime() * velocidade;
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
 * atualizarSonsDaCorrida
 * Chamar uma vez por quadro.
 *   - o motor esquentando (provisório, até haver o som definitivo do
 *     carro) toca desde que a corrida abre até o fim do KATCHAU!;
 *   - o "3... 2... 1... GO!" toca a partir do momento em que o número 3
 *     aparece, e seus bips caem junto com os números na tela;
 *   - a música de fundo toca durante a corrida, da largada até o jogador
 *     cruzar a linha de chegada.
 * Recebe: se o motor, a contagem e a música devem tocar (1 ou 0).
 * Devolve: nada.
 */
void atualizarSonsDaCorrida(int motorAtivo, int contagemAtiva, int musicaAtiva) {
    atualizarSomDeCorrida(&somCorrida[CORRIDA_MOTOR], motorAtivo);
    atualizarSomDeCorrida(&somCorrida[CORRIDA_CONTAGEM], contagemAtiva);
    atualizarSomDeCorrida(&somCorrida[CORRIDA_MUSICA], musicaAtiva);
}

/*
 * atualizarSomDoMotor
 * Chamar uma vez por quadro. Liga os dois loops do motor do carro e faz
 * o som acompanhar o motor:
 *   - o tom sobe com a rotação (e cai quando o câmbio troca de marcha);
 *   - com o pé no fundo toca o loop "acelerando" e, ao soltar ou frear,
 *     o som passa aos poucos para o "aliviando" (mistura de potência igual);
 *   - o motor fica mais alto quanto mais gira.
 * Ao ficar inativo, o som some em ~0,3 s e para.
 * Recebe: se o motor deve soar, a rotação e a carga (0 a 1, de motor.h).
 * Devolve: nada.
 */
void atualizarSomDoMotor(int ativo, float giro, float carga) {
    if (!motorSomCarregado) {
        return;
    }

    if (ativo && !motorSomAtivoAntes) {
        StopMusicStream(motorAcelerando);
        StopMusicStream(motorAliviando);
        PlayMusicStream(motorAcelerando);
        PlayMusicStream(motorAliviando);
        motorSomTocando = 1;
        entradaMotor = 0.0f;
    }
    motorSomAtivoAntes = ativo;
    if (!motorSomTocando) {
        return;
    }

    float passo = GetFrameTime();
    if (ativo) {
        entradaMotor = fminf(entradaMotor + passo * VELOCIDADE_ENTRADA_MOTOR, 1.0f);
    } else {
        entradaMotor -= passo * VELOCIDADE_SAIDA_MOTOR;
        if (entradaMotor <= 0.0f) {
            StopMusicStream(motorAcelerando);
            StopMusicStream(motorAliviando);
            motorSomTocando = 0;
            entradaMotor = 0.0f;
            return;
        }
    }

    giro = fminf(fmaxf(giro, 0.0f), 1.0f);
    carga = fminf(fmaxf(carga, 0.0f), 1.0f);
    float pitch = PITCH_MARCHA_LENTA + (PITCH_LIMITADOR - PITCH_MARCHA_LENTA) * giro;
    float forca = VOLUME_MOTOR * entradaMotor * (0.5f + 0.5f * giro);

    SetMusicPitch(motorAcelerando, pitch);
    SetMusicPitch(motorAliviando, pitch);
    SetMusicVolume(motorAcelerando, forca * sqrtf(carga));
    SetMusicVolume(motorAliviando, forca * VOLUME_ALIVIADO * sqrtf(1.0f - carga));
    UpdateMusicStream(motorAcelerando);
    UpdateMusicStream(motorAliviando);
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
    for (int i = 0; i < TOTAL_SONS_CORRIDA; i++) {
        if (somCorrida[i].carregado) {
            UnloadMusicStream(somCorrida[i].musica);
            somCorrida[i].carregado = 0;
        }
    }
    if (motorSomCarregado) {
        UnloadMusicStream(motorAcelerando);
        UnloadMusicStream(motorAliviando);
        motorSomCarregado = 0;
    }
    if (musicaCarregada) {
        UnloadMusicStream(musicaMenu);
        musicaCarregada = 0;
    }
    CloseAudioDevice();
}
