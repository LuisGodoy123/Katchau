#ifndef AUDIO_H
#define AUDIO_H

/*
 * audio.h
 * ------------------------------------------------------------------
 * Sons do KATCHAU. Por enquanto só a música dos menus, que toca nas
 * telas de menu, some aos poucos quando a corrida começa e volta de
 * onde parou quando o jogador retorna aos menus.
 *
 * Módulo que usa a raylib (parte de áudio).
 * ------------------------------------------------------------------
 */

void carregarAudio(void);
void atualizarAudio(int musicaDoMenu);
void descarregarAudio(void);

#endif
