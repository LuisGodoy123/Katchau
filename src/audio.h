#ifndef AUDIO_H
#define AUDIO_H

/*
 * audio.h
 * ------------------------------------------------------------------
 * Sons do KATCHAU: os efeitos dos menus (cursor, confirmar e voltar), o
 * motor do carro (que acompanha a rotação e a carga do jogador), o motor
 * esquentando e o "3... 2... 1... GO!" da largada (o motor é
 * provisório, até haver o som do carro), a música de fundo da corrida e a
 * música dos menus, que toca nas telas de menu, some aos poucos quando a
 * corrida começa e volta de onde parou quando o jogador retorna aos menus.
 * Quem vence ganha "Life Is a Highway": ela entra baixinho pouco antes da
 * linha e chega ao refrão (1:25) no instante em que o carro cruza.
 *
 * Módulo que usa a raylib (parte de áudio).
 * ------------------------------------------------------------------
 */

/* Os efeitos sonoros dos menus */
typedef enum {
    SOM_CURSOR,      /* o cursor passou para outra opção */
    SOM_CONFIRMAR,   /* apertou um botão */
    SOM_VOLTAR,      /* voltou / cancelou */
    TOTAL_SONS
} TipoSom;

void carregarAudio(void);
void atualizarAudio(int musicaDoMenu);
void tocarSom(TipoSom som);
void atualizarSonsDaCorrida(int motorAtivo, int contagemAtiva, int musicaAtiva);
void atualizarSomDoMotor(int ativo, float giro, float carga);
void atualizarMusicaDaVitoria(float segundosAteALinha, int venceu);
int musicaDaVitoriaTocando(void);
void pausarSonsDaCorrida(int pausar);
void descarregarAudio(void);

#endif
