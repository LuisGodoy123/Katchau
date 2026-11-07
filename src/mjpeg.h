/*
 * mjpeg.h
 * ------------------------------------------------------------------
 * Leitor de vídeo MJPEG, com a decodificação em segundo plano.
 *
 * Um arquivo .mjpeg é só uma fila de JPEGs, um por quadro. Abrir um JPEG de
 * 1920x1080 custa de 40 a 60 ms, bem mais do que os 33 ms que um quadro tem a
 * 30 por segundo, então fazer isso no meio do laço principal trava o jogo.
 * Aqui uma thread vai abrindo os quadros adiantada e deixa alguns prontos; o
 * laço principal só pega o quadro já pronto e o manda para a placa de vídeo.
 *
 * Se a thread não der conta (máquina fraca, quadro pesado), ela pula para o
 * quadro que o relógio está pedindo em vez de ficar atrasada: o vídeo mostra
 * menos quadros por segundo, mas nunca segura o laço principal nem sai do
 * compasso do som.
 *
 * Os quadros são pedidos por um número que só cresce (0, 1, 2, ...). Num vídeo
 * em loop esse número passa do fim e volta ao começo sozinho, então quem chama
 * não precisa fazer a conta da volta.
 *
 * Módulo de lógica: não usa a raylib (quem chama é que cria a textura).
 * ------------------------------------------------------------------
 */

#ifndef MJPEG_H
#define MJPEG_H

typedef struct Mjpeg Mjpeg;

/* Abre o arquivo, lê o índice dos quadros e põe a thread para trabalhar.
 * emLoop: 1 para o vídeo voltar ao começo sem parar, 0 para acabar no fim.
 * Devolve NULL se não conseguir (aí a tela segue sem vídeo). */
Mjpeg *abrirMjpeg(const char *arquivo, int emLoop);

/* Quantos quadros o arquivo tem. */
int totalDeQuadrosMjpeg(const Mjpeg *video);

/* Espera até existir um quadro pronto. Serve para o primeiro quadro, que é
 * quem dá o tamanho da textura. Devolve 1 se há quadro pronto. */
int esperarQuadroMjpeg(Mjpeg *video);

/* Pega o quadro pronto mais novo que não passe de `desejado`, em RGB; os mais
 * antigos que ele são descartados. Devolve NULL se nenhum quadro novo está
 * pronto (aí quem chama simplesmente mantém na tela o quadro anterior).
 * O número pedido também avisa a thread até onde o relógio já chegou.
 * Devolver os pixels com devolverQuadroMjpeg. */
unsigned char *pegarQuadroMjpeg(Mjpeg *video, int desejado, int *largura, int *altura);

/* Devolve ao leitor os pixels de pegarQuadroMjpeg. */
void devolverQuadroMjpeg(Mjpeg *video, unsigned char *pixels);

/* Para a thread, fecha o arquivo e devolve a memória. */
void fecharMjpeg(Mjpeg *video);

#endif
