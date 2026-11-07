/*
 * intro.h
 * ------------------------------------------------------------------
 * O vídeo de abertura, que passa toda vez que o jogo abre (só ao abrir o
 * jogo, não a cada corrida). Qualquer tecla, botão do controle ou clique
 * pula o vídeo; o aviso fica no canto de baixo, à direita.
 *
 * A raylib não toca vídeo, então o vídeo vem em dois arquivos:
 *   recursos/video/intro.mjpeg   os quadros, um JPEG atrás do outro
 *                                (Motion JPEG, a 30 quadros por segundo);
 *   recursos/sons/abertura.mp3   o som (toca pelo audio.c e continua no menu).
 * O som é o relógio: cada quadro mostra a imagem do instante em que a
 * música está, então imagem e som nunca se separam. Os JPEGs são lidos
 * do disco um por vez (o arquivo é grande) e abertos com o stb_image.
 *
 * Para trocar o vídeo, gere os dois arquivos com o ffmpeg:
 *   ffmpeg -i video.mp4 -an -c:v mjpeg -q:v 6 -pix_fmt yuvj420p -f mjpeg intro.mjpeg
 *   ffmpeg -i video.mp4 -vn -c:a libmp3lame -q:a 3 intro.mp3
 * Se os arquivos faltarem, o jogo abre direto no menu.
 *
 * Módulo que usa a raylib.
 * ------------------------------------------------------------------
 */

#ifndef INTRO_H
#define INTRO_H

int iniciarIntro(void);
int atualizarIntro(float tempoQuadro);
void desenharIntro(void);
void encerrarIntro(void);
void desenharSaidaDaIntro(float tempoQuadro);
float momentoDaIntro(void);

#endif
