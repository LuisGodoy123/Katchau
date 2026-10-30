#ifndef AUDIO_H
#define AUDIO_H

/*
 * audio.h
 * ------------------------------------------------------------------
 * Sons do KATCHAU: os efeitos dos menus (cursor, confirmar e voltar) e a
 * música dos menus, que toca nas telas de menu, some aos poucos quando a
 * corrida começa e volta de onde parou quando o jogador retorna aos menus.
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
void descarregarAudio(void);

#endif
