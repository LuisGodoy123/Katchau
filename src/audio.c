/*
 * audio.c
 * ------------------------------------------------------------------
 * Sons e música dos menus. Veja audio.h.
 * ------------------------------------------------------------------
 */

#include <math.h>
#include "raylib.h"
#include "audio.h"
#include "efeitos.h"

#define ARQUIVO_MUSICA_MENU   "recursos/sons/real_gone.mp3"   /* Real Gone: a mesma do vídeo de abertura */
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
    [CORRIDA_MOTOR] = { .arquivo = "recursos/sons/nascar/nascar_largada.wav", .volumeMaximo = 0.8f },   /* o motor aquecendo */
    [CORRIDA_CONTAGEM] = { .arquivo = "recursos/sons/contagem_largada.mp3", .volumeMaximo = 1.0f },
    [CORRIDA_MUSICA] = { .arquivo = "recursos/sons/musica_corrida.mp3", .volumeMaximo = 0.5f,
                         .velocidadeFade = 1.0f, .repetir = 1 },
};

/* A música da vitória. Na última volta, com o jogador em 1º, ela começa
 * baixinho ANTECEDENCIA_DO_REFRAO segundos antes da linha, já no ponto
 * certo para o refrão ("LIFE IS A HIGHWAAAY", em 1:25) cair exatamente na
 * hora em que o carro cruza; aí ela sobe de vez. Se o jogador perde a
 * liderança antes da linha, ela some. */
#define ARQUIVO_MUSICA_VITORIA   "recursos/sons/life_is_a_highway.mp3"
#define MOMENTO_DO_REFRAO        85.0f   /* segundos: 1:25 */
#define ANTECEDENCIA_DO_REFRAO   6.0f    /* segundos antes da linha em que a música entra */
#define VOLUME_VITORIA           1.9f    /* volume máximo: acima de 1 porque o arquivo foi gravado
                                          * baixo (pico em 49% do máximo); 1,9 ainda não distorce */
#define VOLUME_CHEGANDO_INICIO   0.06f   /* fração do máximo quando ela entra... */
#define VOLUME_CHEGANDO_FIM      0.25f   /* ...e logo antes da linha */
#define TOLERANCIA_CHEGANDO      0.30f   /* s: diferença que faz a música pular para o ponto certo */
#define TOLERANCIA_NA_LINHA      0.05f   /* s: na linha, o acerto é fino */
#define VELOCIDADE_REFRAO        5.0f    /* 1/s: no refrão, sobe ao máximo em ~0,2 s */
#define VELOCIDADE_SAIDA_VITORIA 0.7f    /* 1/s: e sai em ~1,4 s */

typedef enum { VITORIA_PARADA, VITORIA_CHEGANDO, VITORIA_REFRAO, VITORIA_SAINDO } EstadoDaVitoria;

static Music musicaVitoria;
static int musicaVitoriaCarregada = 0;
static EstadoDaVitoria estadoVitoria = VITORIA_PARADA;
static float volumeVitoria = 0.0f;   /* 0 a 1, multiplicado por VOLUME_VITORIA */

/* As batidas do McQueen: 12 sons (ENG_LMby 1 a 11 e 13), um sorteado a cada batida */
#define TOTAL_SONS_DE_BATIDA     12
#define VOLUME_BATIDA            1.0f    /* volume máximo (0 a 1); a força da batida baixa a partir daí */
#define VOLUME_BATIDA_FRACA      0.45f   /* o volume da batida mais fraca, em relação à mais forte */
#define PAUSA_ENTRE_BATIDAS      0.35f   /* segundos: uma batida que dura vários quadros toca um som só */
static const int NUMEROS_DAS_BATIDAS[TOTAL_SONS_DE_BATIDA] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 13 };
/* As falas do McQueen: o grito da batida forte e as de quando ele assume ou perde a liderança.
 * Tudo que ele diz (inclusive os sons de batida acima) passa por uma só "voz": enquanto uma fala
 * toca, nenhuma outra começa, para nunca haver uma em cima da outra. */
enum {
    FALA_GRITO, FALA_LIDER_1, FALA_LIDER_2, FALA_PERDEU_1, FALA_PERDEU_2,
    FALA_ULTRAPASSA_1,                               /* 7 falas de quando ele ultrapassa alguém */
    FALA_ULTRAPASSADO_1 = FALA_ULTRAPASSA_1 + 7,     /* 4 falas de quando ele é ultrapassado */
    FALA_BATE_1 = FALA_ULTRAPASSADO_1 + 4,           /* 2 falas de quando ele bate em alguém */
    TOTAL_FALAS = FALA_BATE_1 + 2
};
#define TOTAL_FALAS_ULTRAPASSA    7
#define TOTAL_FALAS_ULTRAPASSADO  4
#define TOTAL_FALAS_BATE          2
static const char *ARQUIVOS_FALAS[TOTAL_FALAS] = {
    "recursos/sons/mcqueen_grito.wav",
    "recursos/sons/mcqueen_lider_1.wav",
    "recursos/sons/mcqueen_lider_2.wav",
    "recursos/sons/mcqueen_perdeu_1.wav",
    "recursos/sons/mcqueen_perdeu_2.wav",
    "recursos/sons/mcqueen_ultrapassa_13.wav",
    "recursos/sons/mcqueen_ultrapassa_11.wav",
    "recursos/sons/mcqueen_ultrapassa_10.wav",
    "recursos/sons/mcqueen_ultrapassa_9.wav",
    "recursos/sons/mcqueen_ultrapassa_5.wav",
    "recursos/sons/mcqueen_ultrapassa_3.wav",
    "recursos/sons/mcqueen_ultrapassa_16.wav",
    "recursos/sons/mcqueen_ultrapassado_n3.wav",
    "recursos/sons/mcqueen_ultrapassado_d11.wav",
    "recursos/sons/mcqueen_ultrapassado_d8.wav",
    "recursos/sons/mcqueen_ultrapassado_d9.wav",
    "recursos/sons/mcqueen_bate_6.wav",
    "recursos/sons/mcqueen_bate_4.wav",
};
#define PAUSA_ENTRE_GRITOS           3.0    /* segundos entre dois gritos de rodada */
#define PAUSA_DEPOIS_DA_FALA         0.5    /* segundos de silêncio depois de qualquer fala */
#define PAUSA_ENTRE_FALAS_DE_PONTA   8.0    /* segundos entre duas falas de liderança (ultrapassagens em série) */
#define PAUSA_ENTRE_FALAS_DE_POSICAO 3.0    /* segundos entre duas falas de ultrapassagem (do começo de uma ao da outra) */
static Sound falas[TOTAL_FALAS];
static int falaCarregada[TOTAL_FALAS];
static int ultimaFalaDeLider = -1;
static int ultimaFalaDePerda = -1;
static double fimDaVoz = 0.0;               /* hora em que a fala atual acaba (mais a pausa) */
static double horaDoUltimoGrito = -100.0;
static double horaDaUltimaFalaDePonta = -100.0;
static double horaDaUltimaFalaDePosicao = -100.0;
static int ultimaFalaQueUltrapassa = -1;
static int ultimaFalaDeUltrapassado = -1;
static int ultimaFalaDeBater = -1;

static Sound sonsDeBatida[TOTAL_SONS_DE_BATIDA];
static int somDeBatidaCarregado[TOTAL_SONS_DE_BATIDA];
static int ultimaBatida = -1;
static double horaDaUltimaBatida = -10.0;

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
/* A abertura: o áudio montado para a intro, que segue no menu; quando acaba, entra a Real Gone
 * (que fica repetindo), e soa como uma coisa só */
#ifndef ARQUIVO_DA_ABERTURA
#define ARQUIVO_DA_ABERTURA   "recursos/sons/abertura.mp3"
#endif
static Music abertura;
static int temAbertura = 0;
static int naAbertura = 0;   /* 1: a música do menu agora é a abertura */

/*
 * musicaAtual
 * Recebe: nada.
 * Devolve: a música do menu que está valendo agora (a abertura ou a Real Gone).
 */
static Music *musicaAtual(void) {
    return naAbertura ? &abertura : &musicaMenu;
}

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

    for (int i = 0; i < TOTAL_SONS_DE_BATIDA; i++) {
        const char *caminho = TextFormat("recursos/sons/batida_mcqueen_%d.wav", NUMEROS_DAS_BATIDAS[i]);
        if (FileExists(caminho)) {
            sonsDeBatida[i] = LoadSound(caminho);
            somDeBatidaCarregado[i] = IsSoundValid(sonsDeBatida[i]);
        }
    }

    for (int i = 0; i < TOTAL_FALAS; i++) {
        if (FileExists(ARQUIVOS_FALAS[i])) {
            falas[i] = LoadSound(ARQUIVOS_FALAS[i]);
            falaCarregada[i] = IsSoundValid(falas[i]);
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

    carregarEfeitos();   /* os motores NASCAR e os efeitos da corrida (efeitos.c) */

    if (FileExists(ARQUIVO_MUSICA_VITORIA)) {
        musicaVitoria = LoadMusicStream(ARQUIVO_MUSICA_VITORIA);
        musicaVitoriaCarregada = IsMusicValid(musicaVitoria);
        musicaVitoria.looping = false;   /* toca do refrão até o fim e para */
    }

    if (FileExists(ARQUIVO_DA_ABERTURA)) {
        abertura = LoadMusicStream(ARQUIVO_DA_ABERTURA);
        temAbertura = IsMusicValid(abertura);
        if (temAbertura) {
            abertura.looping = false;   /* toca uma vez: depois vem a Real Gone */
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
 * comecarAbertura
 * Começa a abertura, com volume cheio: ela é o som da intro e continua no menu.
 * Recebe: nada.
 * Devolve: nada.
 */
void comecarAbertura(void) {
    if (!temAbertura) {
        return;
    }
    PlayMusicStream(abertura);
    naAbertura = 1;
    iniciada = 1;
    pausada = 0;
    fade = 1.0f;
    SetMusicVolume(abertura, fade * VOLUME_MUSICA_MENU);
}

/*
 * pularAbertura
 * Quem pulou a intro não ouve o resto da abertura: ela para e a Real Gone começa do zero.
 * Recebe: nada.
 * Devolve: nada.
 */
void pularAbertura(void) {
    if (naAbertura) {
        StopMusicStream(abertura);
        naAbertura = 0;
    }
    if (!musicaCarregada) {
        iniciada = 0;
        return;
    }
    StopMusicStream(musicaMenu);
    PlayMusicStream(musicaMenu);   /* do começo */
    iniciada = 1;
    pausada = 0;
    fade = 1.0f;
    SetMusicVolume(musicaMenu, fade * VOLUME_MUSICA_MENU);
}

/*
 * tempoDaAbertura
 * Recebe: nada.
 * Devolve: em que ponto a abertura está, em segundos (-1 se ela não está tocando).
 */
float tempoDaAbertura(void) {
    if (!naAbertura || !IsMusicStreamPlaying(abertura)) {
        return -1.0f;
    }
    return GetMusicTimePlayed(abertura);
}

/*
 * musicaDoMenuTocando
 * Recebe: nada.
 * Devolve: 1 se a música do menu já começou e não está pausada.
 */
int musicaDoMenuTocando(void) {
    return (musicaCarregada || naAbertura) && iniciada && !pausada;
}

/*
 * tempoDaMusicaDoMenu
 * Recebe: nada.
 * Devolve: em que ponto a música do menu está, em segundos (0 se não começou).
 */
float tempoDaMusicaDoMenu(void) {
    return iniciada ? GetMusicTimePlayed(*musicaAtual()) : 0.0f;
}

/*
 * atualizarAudio
 * Chamar uma vez por quadro. Sobe o volume nos menus e desce fora deles;
 * quando chega a zero, pausa a música para retomá-la de onde parou.
 * Recebe: 1 se a tela atual é de menu, 0 se não.
 * Devolve: nada.
 */
void atualizarAudio(int musicaDoMenu) {
    if (!musicaCarregada && !naAbertura) {
        return;
    }

    float passo = GetFrameTime() * VELOCIDADE_FADE;
    if (musicaDoMenu) {
        fade = (fade + passo > 1.0f) ? 1.0f : fade + passo;
        if (!iniciada) {
            PlayMusicStream(*musicaAtual());
            iniciada = 1;
        } else if (pausada) {
            ResumeMusicStream(*musicaAtual());
            pausada = 0;
        }
    } else {
        fade = (fade - passo < 0.0f) ? 0.0f : fade - passo;
        if (fade == 0.0f && iniciada && !pausada) {
            PauseMusicStream(*musicaAtual());
            pausada = 1;
        }
    }

    SetMusicVolume(*musicaAtual(), fade * VOLUME_MUSICA_MENU);
    if (iniciada && !pausada) {
        UpdateMusicStream(*musicaAtual());   /* alimenta o som; precisa de uma chamada por quadro */
        if (naAbertura && !IsMusicStreamPlaying(abertura)) {
            /* A abertura acabou: entra a Real Gone, do começo, e ela fica repetindo */
            naAbertura = 0;
            if (musicaCarregada) {
                PlayMusicStream(musicaMenu);
                SetMusicVolume(musicaMenu, fade * VOLUME_MUSICA_MENU);
            } else {
                iniciada = 0;
            }
        }
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
 * falar
 * Toca uma fala do McQueen e ocupa a voz até ela acabar: nenhuma outra começa antes.
 * Recebe: o som, o volume (0 a 1) e a hora de agora.
 * Devolve: nada.
 */
static void falar(Sound som, float volume, double agora) {
    SetSoundVolume(som, volume);
    StopSound(som);
    PlaySound(som);
    fimDaVoz = agora + (double) som.frameCount / (double) som.stream.sampleRate + PAUSA_DEPOIS_DA_FALA;
}

/*
 * sortearFala
 * Escolhe uma fala de um grupo, sem repetir a última daquele grupo.
 * Recebe: a primeira fala do grupo, quantas são e onde está a última sorteada.
 * Devolve: o número da fala (e a guarda como a última), ou -1 se nenhuma do grupo foi carregada.
 */
static int sortearFala(int primeira, int total, int *ultima) {
    int carregadas = 0;
    for (int i = 0; i < total; i++) {
        carregadas += falaCarregada[primeira + i];
    }
    if (carregadas == 0) {
        return -1;
    }
    int escolhida;
    do {
        escolhida = primeira + GetRandomValue(0, total - 1);
    } while (!falaCarregada[escolhida] || (escolhida == *ultima && carregadas > 1));
    *ultima = escolhida;
    return escolhida;
}

/*
 * tocarSomDaBatida
 * O McQueen bateu. Se foi contra outro carro, ele fala uma das falas de "bati em alguém"; no
 * muro, toca um dos sons de batida,
 * sorteado (nunca o mesmo duas vezes seguidas). Uma batida que dura vários quadros, como um carro
 * raspando no outro, toca um som só. Se ele está falando, a batida não toca: nunca há uma fala em
 * cima da outra.
 * Recebe: o volume da batida (0 a 1, de batidaDoMcQueen) e se foi contra outro carro.
 * Devolve: nada.
 */
void tocarSomDaBatida(float volume, int contraCarro) {
    double agora = GetTime();
    if (volume <= 0.0f || agora - horaDaUltimaBatida < PAUSA_ENTRE_BATIDAS || agora < fimDaVoz) {
        return;
    }
    float forca = fminf(volume, 1.0f);
    float altura = VOLUME_BATIDA * (VOLUME_BATIDA_FRACA + (1.0f - VOLUME_BATIDA_FRACA) * forca);

    if (contraCarro) {
        int fala = sortearFala(FALA_BATE_1, TOTAL_FALAS_BATE, &ultimaFalaDeBater);
        if (fala >= 0) {
            horaDaUltimaBatida = agora;
            falar(falas[fala], altura, agora);
            return;
        }
    }

    int carregados = 0;
    for (int i = 0; i < TOTAL_SONS_DE_BATIDA; i++) {
        carregados += somDeBatidaCarregado[i];
    }
    if (carregados == 0) {
        return;
    }
    int escolhido;
    do {
        escolhido = GetRandomValue(0, TOTAL_SONS_DE_BATIDA - 1);
    } while (!somDeBatidaCarregado[escolhido] || (escolhido == ultimaBatida && carregados > 1));
    ultimaBatida = escolhido;
    horaDaUltimaBatida = agora;
    falar(sonsDeBatida[escolhido], altura, agora);
}

/*
 * tocarGritoDeRodada
 * O McQueen começou a rodar: ele grita (a fala do grito). Só vale para a rodada DELE; um carro que
 * gira por causa de uma batida do McQueen não faz o grito. Se ele já está falando, não toca: nunca
 * há uma fala em cima da outra.
 * Recebe: o volume do grito (0 a 1, de rodadaDoMcQueen).
 * Devolve: nada.
 */
void tocarGritoDeRodada(float volume) {
    double agora = GetTime();
    if (volume <= 0.0f || !falaCarregada[FALA_GRITO] || agora < fimDaVoz || agora - horaDoUltimoGrito < PAUSA_ENTRE_GRITOS) {
        return;
    }
    horaDoUltimoGrito = agora;
    falar(falas[FALA_GRITO], VOLUME_BATIDA * fminf(volume, 1.0f), agora);
}

/*
 * tocarFalaDeLideranca
 * O McQueen assume a liderança ou a perde: fala uma de duas frases, sorteada (não repete a
 * última do mesmo tipo). Se ele já está falando, ou falou de liderança há pouco (vários carros o
 * passando em seguida), fica quieto: nunca há uma fala em cima da outra.
 * Recebe: 1 se ele acabou de assumir a liderança, 0 se acabou de perdê-la.
 * Devolve: nada.
 */
void tocarFalaDeLideranca(int comecou) {
    double agora = GetTime();
    if (agora < fimDaVoz || agora - horaDaUltimaFalaDePonta < PAUSA_ENTRE_FALAS_DE_PONTA) {
        return;
    }
    int primeira = comecou ? FALA_LIDER_1 : FALA_PERDEU_1;
    int *ultima = comecou ? &ultimaFalaDeLider : &ultimaFalaDePerda;
    int escolhida = primeira + GetRandomValue(0, 1);
    if (escolhida == *ultima) {
        escolhida = (escolhida == primeira) ? primeira + 1 : primeira;   /* a outra frase */
    }
    if (!falaCarregada[escolhida]) {
        escolhida = (escolhida == primeira) ? primeira + 1 : primeira;
        if (!falaCarregada[escolhida]) {
            return;
        }
    }
    *ultima = escolhida;
    horaDaUltimaFalaDePonta = agora;
    falar(falas[escolhida], VOLUME_BATIDA, agora);
}

/*
 * tocarFalaDeUltrapassagem
 * O McQueen ultrapassou alguém ou foi ultrapassado (em qualquer posição): fala uma das falas do
 * caso, sorteada (não repete a última do mesmo tipo). Só fala se ele não estiver falando nada:
 * uma ultrapassagem que chega no meio de outra fala, ou vários carros o passando em seguida, não
 * geram fala nova, para nunca haver uma em cima da outra.
 * Recebe: 1 se ele ultrapassou alguém, 0 se foi ultrapassado.
 * Devolve: nada.
 */
void tocarFalaDeUltrapassagem(int ultrapassou) {
    double agora = GetTime();
    if (agora < fimDaVoz || agora - horaDaUltimaFalaDePosicao < PAUSA_ENTRE_FALAS_DE_POSICAO) {
        return;
    }
    int fala = ultrapassou ? sortearFala(FALA_ULTRAPASSA_1, TOTAL_FALAS_ULTRAPASSA, &ultimaFalaQueUltrapassa)
                           : sortearFala(FALA_ULTRAPASSADO_1, TOTAL_FALAS_ULTRAPASSADO, &ultimaFalaDeUltrapassado);
    if (fala < 0) {
        return;
    }
    horaDaUltimaFalaDePosicao = agora;
    falar(falas[fala], VOLUME_BATIDA, agora);
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
 * acertarMusicaDaVitoria
 * Se a música está longe demais do ponto em que deveria estar, pula
 * para ele.
 * Recebe: o ponto certo (em segundos) e a diferença tolerada.
 * Devolve: nada.
 */
static void acertarMusicaDaVitoria(float momento, float tolerancia) {
    if (fabsf(GetMusicTimePlayed(musicaVitoria) - momento) > tolerancia) {
        SeekMusicStream(musicaVitoria, momento);
    }
}

/*
 * atualizarMusicaDaVitoria
 * Chamar uma vez por quadro.
 *   - chegando: enquanto faltam até ANTECEDENCIA_DO_REFRAO segundos para
 *     o jogador (em 1º) cruzar a linha final, a música toca baixinho e é
 *     mantida em (1:25 - segundos que faltam), crescendo devagar;
 *   - venceu: no quadro da chegada ela é cravada em 1:25 (caso a
 *     estimativa tenha errado um pouco) e sobe ao volume máximo; daí segue
 *     tocando enquanto "venceu" continuar 1;
 *   - nenhum dos dois: some aos poucos e para.
 * Recebe: os segundos até a linha (negativo = o jogador não está chegando
 *         em 1º) e se ele já venceu (e a tela ainda é a da corrida ou a do
 *         resultado).
 * Devolve: nada.
 */
void atualizarMusicaDaVitoria(float segundosAteALinha, int venceu) {
    if (!musicaVitoriaCarregada) {
        return;
    }
    float passo = GetFrameTime();

    if (venceu) {
        if (estadoVitoria != VITORIA_REFRAO) {
            if (estadoVitoria == VITORIA_PARADA) {
                PlayMusicStream(musicaVitoria);   /* venceu sem ter passado pelo "chegando" */
            }
            acertarMusicaDaVitoria(MOMENTO_DO_REFRAO, TOLERANCIA_NA_LINHA);
            estadoVitoria = VITORIA_REFRAO;
        }
        volumeVitoria = fminf(volumeVitoria + passo * VELOCIDADE_REFRAO, 1.0f);
    } else if (segundosAteALinha >= 0.0f && segundosAteALinha <= ANTECEDENCIA_DO_REFRAO) {
        float momento = MOMENTO_DO_REFRAO - segundosAteALinha;
        if (estadoVitoria == VITORIA_PARADA) {
            PlayMusicStream(musicaVitoria);
            SeekMusicStream(musicaVitoria, momento);
            volumeVitoria = 0.0f;
        } else {
            acertarMusicaDaVitoria(momento, TOLERANCIA_CHEGANDO);
        }
        estadoVitoria = VITORIA_CHEGANDO;

        /* cresce de INICIO a FIM conforme a linha se aproxima */
        float chegada = 1.0f - segundosAteALinha / ANTECEDENCIA_DO_REFRAO;
        float alvo = VOLUME_CHEGANDO_INICIO + (VOLUME_CHEGANDO_FIM - VOLUME_CHEGANDO_INICIO) * chegada;
        volumeVitoria = (volumeVitoria < alvo) ? fminf(volumeVitoria + passo, alvo) : alvo;
    } else if (estadoVitoria != VITORIA_PARADA) {
        estadoVitoria = VITORIA_SAINDO;
        volumeVitoria -= passo * VELOCIDADE_SAIDA_VITORIA;
        if (volumeVitoria <= 0.0f) {
            StopMusicStream(musicaVitoria);
            estadoVitoria = VITORIA_PARADA;
            volumeVitoria = 0.0f;
        }
    }

    if (estadoVitoria != VITORIA_PARADA) {
        SetMusicVolume(musicaVitoria, volumeVitoria * VOLUME_VITORIA);
        UpdateMusicStream(musicaVitoria);
    }
}

/*
 * musicaDaVitoriaTocando
 * Recebe: nada.
 * Devolve: 1 se a música da vitória está chegando ou tocando (a música
 *          de fundo da corrida deve sair), 0 se não.
 */
int musicaDaVitoriaTocando(void) {
    return estadoVitoria == VITORIA_CHEGANDO || estadoVitoria == VITORIA_REFRAO;
}

/*
 * pausarSonsDaCorrida
 * Congela (ou solta) todos os sons da corrida de uma vez: os motores, a
 * contagem, a música de fundo e a da vitória. Ao soltar, cada um volta
 * do ponto exato em que parou (o "3, 2, 1" continua junto com os números
 * e o refrão continua sincronizado). Enquanto estiver pausado, o main.c
 * não chama as funções de atualizar os sons da corrida.
 * Recebe: 1 para pausar, 0 para continuar.
 * Devolve: nada.
 */
void pausarSonsDaCorrida(int pausar) {
    Music *musicas[TOTAL_SONS_CORRIDA + 3];
    int total = 0;
    for (int i = 0; i < TOTAL_SONS_CORRIDA; i++) {
        if (somCorrida[i].carregado) {
            musicas[total++] = &somCorrida[i].musica;
        }
    }
    if (musicaVitoriaCarregada) {
        musicas[total++] = &musicaVitoria;
    }

    pausarEfeitosDaCorrida(pausar);   /* os motores e os loops do efeitos.c */

    /* Pausar ou continuar um som que não está tocando não faz nada */
    for (int i = 0; i < total; i++) {
        if (pausar) {
            PauseMusicStream(*musicas[i]);
        } else {
            ResumeMusicStream(*musicas[i]);
        }
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
    for (int i = 0; i < TOTAL_FALAS; i++) {
        if (falaCarregada[i]) {
            UnloadSound(falas[i]);
            falaCarregada[i] = 0;
        }
    }
    for (int i = 0; i < TOTAL_SONS_DE_BATIDA; i++) {
        if (somDeBatidaCarregado[i]) {
            UnloadSound(sonsDeBatida[i]);
            somDeBatidaCarregado[i] = 0;
        }
    }
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
    descarregarEfeitos();
    if (musicaVitoriaCarregada) {
        UnloadMusicStream(musicaVitoria);
        musicaVitoriaCarregada = 0;
    }
    if (musicaCarregada) {
        UnloadMusicStream(musicaMenu);
        musicaCarregada = 0;
    }
    if (temAbertura) {
        UnloadMusicStream(abertura);
        temAbertura = 0;
        naAbertura = 0;
    }
    CloseAudioDevice();
}
