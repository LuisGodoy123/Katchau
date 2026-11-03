#ifndef OBSTACULOS_H
#define OBSTACULOS_H

/*
 * obstaculos.h
 * ------------------------------------------------------------------
 * Objetos da pista: os da beira (árvores, pórtico, pneus, placas) e as
 * marcas do grid de largada, pintadas no asfalto.
 *
 * Cada segmento guarda os seus numa LISTA SIMPLESMENTE ENCADEADA:
 * o segmento aponta para o primeiro objeto, e cada objeto aponta para
 * o próximo (o último aponta para NULL). Lista simples basta aqui:
 * os objetos só são percorridos do começo ao fim, para desenhar.
 *
 * Módulo de lógica: não usa a raylib. Só decide QUE objeto fica ONDE;
 * a forma 3D de cada tipo fica no mundo.c.
 * ------------------------------------------------------------------
 */

#include "pista.h"

/* Um módulo de arquibancada: largura e fundo, em metros (os mesmos do modelo 3D) */
#define LARGURA_DO_MODULO     24.0f
#define FUNDO_DO_MODULO       13.0f

/* O que é o objeto (o mundo.c tem uma forma 3D para cada tipo) */
typedef enum {
    OBJETO_ARVORE,
    OBJETO_PORTICO,      /* pórtico da largada, por cima da pista */
    OBJETO_PNEUS,        /* barreira de pneus por fora das curvas */
    OBJETO_PLACA_50,     /* placas de frenagem antes das curvas */
    OBJETO_PLACA_100,
    OBJETO_PLACA_150,
    OBJETO_MARCA_GRID,   /* colchete de um lugar do grid: pintado no chão, não fica de pé */
    OBJETO_ARQUIBANCADA, /* um módulo de arquibancada, de frente para a pista */
    OBJETO_CAMAROTE,     /* um módulo de camarote */
    TOTAL_TIPOS_OBJETO
} TipoObjeto;

/* Um nó da lista de objetos de um segmento */
typedef struct Obstaculo {
    TipoObjeto tipo;
    float deslocamentoLateral;    /* como o do carro: 0 no meio, ±1 nas bordas do asfalto */
    char setor;                   /* no módulo do meio de uma arquibancada: a letra da placa (senão 0) */
    struct Obstaculo *proximo;    /* próximo objeto do mesmo segmento (NULL no último) */
} Obstaculo;

/* ---------- Lista de um segmento ---------- */
void adicionarObstaculo(Segmento *segmento, TipoObjeto tipo, float deslocamentoLateral);
int contarObstaculos(Segmento *segmento);
void liberarObstaculos(Segmento *segmento);

/* ---------- A pista inteira ---------- */
void decorarPista(Pista *pista);
void decorarOval(Pista *pista);
int lugarLivre(Pista *pista, Segmento *segmento, float lateral, float folga);

#endif
