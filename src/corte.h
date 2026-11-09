/*
 * corte.h
 * ------------------------------------------------------------------
 * O corte pelo campo de visão (o "frustum"): dizer se uma coisa pode
 * aparecer na tela, para não mandar desenhar o que está atrás da câmera
 * ou fora da borda.
 *
 * Não é uma economia de triângulos, é uma economia de chamadas: cada
 * malha mandada para a placa de vídeo custa uma troca de material, de
 * shader e de buffers, e num OpenGL emulado (ARM64) isso é o que pesa.
 * O modelo de Interlagos tem 177 malhas e, em média, só umas 78 estão
 * à vista.
 *
 * Como usar: chamar prepararCorte() logo depois de BeginMode3D, e depois
 * perguntar por cada coisa antes de desenhá-la. O teste é conservador:
 * o que toca a tela passa sempre; só é cortado o que não pode aparecer.
 *
 * Duas variáveis de ambiente para conferir o corte:
 *   KATCHAU_SO_O_CORTADO  inverte o teste. A tela tem de ficar vazia; se
 *                         aparecer pedaço de pista, muro ou árvore, o
 *                         corte está comendo coisa visível — e dá para
 *                         ver qual.
 *   KATCHAU_CONFERIR_CORTE  escreve no log, por segundo, quanto está
 *                         sendo cortado. Perto de 0% quer dizer que o
 *                         corte não está pegando nada; 100%, que algo
 *                         está errado nos planos.
 * ------------------------------------------------------------------
 */

#ifndef CORTE_H
#define CORTE_H

#include "raylib.h"

void prepararCorte(void);
void prepararCorteDaCamera(Camera3D camera, float graus, float aspecto, float folga);
int caixaAVista(BoundingBox caixa);
int esferaAVista(Vector3 centro, float raio);
int pontoAVista(Vector3 ponto, float folga);
BoundingBox caixaNoMundo(BoundingBox local, Matrix noMundo);
BoundingBox caixaDaMalha(Mesh malha, Matrix noMundo);

#endif
