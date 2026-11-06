/*
 * infraestrutura.h
 * ------------------------------------------------------------------
 * O que um autódromo de verdade tem em volta da pista, em Interlagos: guard-rail,
 * alambrado, placas de patrocínio, brita nas curvas, postos de fiscal, torres de TV,
 * passarelas, telão, bandeiras, a rua de serviço, tendas, estacionamento e os
 * caminhões das equipes. Nada disso muda a física: é tudo além de onde o carro chega.
 *
 * O que acompanha a forma da pista é montado aqui (linhas, brita, guard-rail, placas);
 * o resto são modelos .glb, e aqui só se decide onde cada um fica.
 * ------------------------------------------------------------------
 */

#ifndef INFRAESTRUTURA_H
#define INFRAESTRUTURA_H

#include "raylib.h"
#include "pista.h"

typedef float (*AlturaDaGrade)(float x, float z);
typedef void (*PlantarArvore)(Vector3 pe);

/* Os modelos .glb do cenário, na mesma ordem da lista de arquivos em mundo.c */
typedef enum {
    CENARIO_POSTO_FISCAL,
    CENARIO_TORRE_TV,
    CENARIO_PASSARELA,
    CENARIO_TELAO,
    CENARIO_CAMINHAO_1,   /* e mais 3 */
    CENARIO_TENDA_1 = CENARIO_CAMINHAO_1 + 4,   /* e mais 2 */
    CENARIO_CARRO_1 = CENARIO_TENDA_1 + 3,      /* e mais 5 */
    CENARIO_BANCADA_1 = CENARIO_CARRO_1 + 6,    /* e mais 3 */
    CENARIO_MASTRO_1 = CENARIO_BANCADA_1 + 4,   /* e mais 4 */
    CENARIO_PILHA_1 = CENARIO_MASTRO_1 + 5,     /* e mais 1 */
    TOTAL_DE_MODELOS_DO_CENARIO = CENARIO_PILHA_1 + 2
} ModeloDoCenario;

typedef void (*ColocarNoCenario)(ModeloDoCenario qual, Vector3 pe, float giro, float tamanho);

typedef struct {
    Mesh pintura;      /* linhas brancas, brita e rua: cores nos vértices */
    Mesh objetos;      /* postes e trilhos do guard-rail: cores nos vértices */
    Mesh anuncios;     /* as placas: usam a imagem "anuncios" */
    Mesh cercas;       /* alambrado: usa a imagem do alambrado do estádio */
    Texture2D imagemDosAnuncios;
} MalhasDaInfraestrutura;

const MalhasDaInfraestrutura *construirInfraestrutura(Pista *pista, AlturaDaGrade altura, PlantarArvore plantar, ColocarNoCenario colocar);
void descarregarInfraestrutura(void);

int naAreaDeUmTelao(float x, float z);
int teloesDeInterlagos(Vector3 *pes, float *giros, int maximo);

#endif
