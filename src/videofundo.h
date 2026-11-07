/*
 * videofundo.h
 * ------------------------------------------------------------------
 * Um vídeo em loop, usado como fundo das telas de entrada e do menu principal.
 *
 * Os quadros ficam em um arquivo MJPEG (JPEGs um atrás do outro, a 30 quadros por
 * segundo), lidos do disco um por vez. Não há som aqui: a música das telas é
 * a do audio.c. Quando o vídeo chega ao fim, volta ao primeiro quadro.
 *
 * Módulo que usa a raylib.
 * ------------------------------------------------------------------
 */

#ifndef VIDEOFUNDO_H
#define VIDEOFUNDO_H

typedef struct VideoDeFundo VideoDeFundo;

/* Abre o arquivo e lê o índice dos quadros. Devolve NULL se não conseguir (a tela segue sem vídeo). */
VideoDeFundo *abrirVideoDeFundo(const char *arquivo);

/* Volta ao primeiro quadro (quando a tela aparece de novo). */
void reiniciarVideoDeFundo(VideoDeFundo *video);

/* Avança o relógio do vídeo e troca o quadro quando chega a hora dele. */
void atualizarVideoDeFundo(VideoDeFundo *video, float tempoQuadro);

/* Desenha o quadro atual cobrindo a tela inteira (sem distorcer; corta as sobras). alfa: 0 a 1. */
void desenharVideoDeFundo(const VideoDeFundo *video, float alfa);

void fecharVideoDeFundo(VideoDeFundo *video);

#endif
