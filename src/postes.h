/*
 * postes.h
 * ------------------------------------------------------------------
 * Os postes de luz da rua principal e o semáforo do cruzamento
 * (recursos/modelos/postes.glb, de ferramentas/luzes/montar_postes.py): o
 * desenho deles, com a luz da hora e a sombra do sol como o resto do mapa, e
 * os troncos, que o carro não atravessa (recursos/modelos/postes.json). As
 * luminárias acesas e as lentes do semáforo são luzes da cidade (luzes.c:
 * luzes/postes.glb e luzes/semaforo.glb).
 * ------------------------------------------------------------------
 */

#ifndef POSTES_H
#define POSTES_H

#include "raylib.h"

typedef struct {
    float x, z;              /* o meio do tronco */
    float base, topo;        /* a altura do pé e do alto */
    float raio;
} Poste;

void carregarPostes(void);
void descarregarPostes(void);
const Model *modeloDosPostes(void);        /* NULL: não há postes */
const Matrix *matrizesDosPostes(void);     /* uma por malha (todas a identidade: o modelo já está no lugar) */
int totalDePostes(void);
const Poste *poste(int i);

#endif
