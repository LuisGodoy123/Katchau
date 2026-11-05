/*
 * tracado.h
 * ------------------------------------------------------------------
 * O formato das tabelas medidas nos modelos 3D das pistas (copa_pistao.h,
 * oval.h e splat.h), nas unidades e nos eixos do próprio modelo.
 * ------------------------------------------------------------------
 */

#ifndef TRACADO_H
#define TRACADO_H

/* Um ponto da linha da pista */
typedef struct {
    float x, z;              /* a linha de referência, vista de cima */
    float ateMuroDireita;    /* da linha até o muro de fora */
    float ateMuroEsquerda;   /* da linha até o muro de dentro */
    float altura;            /* altura do asfalto na linha */
    float perfil[9];         /* altura em relação à linha, da borda esquerda à direita */
} PontoDoTracado;

/* Um lugar da torcida na arquibancada */
typedef struct {
    float x, y, z;
} LugarNaTorcida;

/* Os objetos que enfeitam os mapas (cenario_mapas.h); mundo.c sabe o modelo de cada um */
typedef enum {
    COISA_ARVORE_1, COISA_ARVORE_2, COISA_ARVORE_3,
    COISA_PINHEIRO_1, COISA_PINHEIRO_2,
    COISA_COPA_1, COISA_COPA_2, COISA_COPA_3,
    COISA_CAMINHAO_1, COISA_CAMINHAO_2, COISA_CAMINHAO_3, COISA_CAMINHAO_4,
    COISA_TENDA_1, COISA_TENDA_2, COISA_TENDA_3,
    COISA_CARRO_1, COISA_CARRO_2, COISA_CARRO_3, COISA_CARRO_4, COISA_CARRO_5, COISA_CARRO_6,
    COISA_MASTRO_1, COISA_MASTRO_2, COISA_MASTRO_3, COISA_MASTRO_4, COISA_MASTRO_5,
    COISA_PILHA_1, COISA_PILHA_2,
    COISA_PNEUS_VERMELHO, COISA_PNEUS_BRANCO,
    COISA_POSTO_FISCAL, COISA_TORRE_TV, COISA_TELAO, COISA_ARQUIBANCADA, COISA_CAMAROTE, COISA_POSTE, COISA_PORTICO,
    TOTAL_DE_COISAS
} TipoDeCoisa;

/* Um objeto no mapa, em metros, nos eixos do mundo */
typedef struct {
    TipoDeCoisa tipo;
    float x, y, z;
    float giro;      /* radianos: para onde a frente dele aponta */
    float tamanho;
} CoisaNoMapa;

#endif
