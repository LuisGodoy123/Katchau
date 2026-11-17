/*
 * palco.h
 * ------------------------------------------------------------------
 * A apresentação do carro na garagem, como nas telas de escolha de
 * personagem: cada carro entra do jeito dele (derrapando,
 * pulando, de ré, devagar, aos pulinhos, na hidráulica), para no disco
 * virado para a câmera e o locutor anuncia o nome dele, com voz grossa,
 * como nos jogos de luta. Ele ouve: acena nas ênfases do locutor, se anima
 * e solta os tiques dele (gestos.c), e fica ali, vivo, até o jogador
 * escolher outro; aí ele sai e o próximo entra. Ele entra pela esquerda
 * e para olhando para a direita (o cartão do carro fica à direita da tela).
 *
 * O palco olha da câmera para o -z: a direita da tela é o +x. O rumo é o
 * do mundo do jogo (a frente é (sen r, 0, -cos r)).
 *
 * Não usa a raylib (regra 2 do roadmap): só a conta. O desenho é do
 * render.c (fotografarPalco) e a tela, do menu.c.
 * ------------------------------------------------------------------
 */

#ifndef PALCO_H
#define PALCO_H

#include "gestos.h"
#include "olhos.h"

typedef enum {
    PALCO_VAZIO,
    PALCO_ENTRANDO,
    PALCO_APRESENTANDO,   /* chegou: fala a frase dele */
    PALCO_PARADO,         /* vivo, olhando para quem escolhe */
    PALCO_SAINDO
} FaseDoPalco;

/* A firula: o número de cada personagem, que diz quem ele é (o McQueen convencido, o Ramone na hidráulica, o
 * Guido rodopiando...). É somada ao corpo dele (gestos.c), que continua respirando e reagindo por baixo */
typedef struct {
    float aceno;             /* radianos: + o bico desce (no eixo de trás) */
    float traseira;          /* radianos: a traseira sobe, girando no eixo da frente (a hidráulica) */
    float rolagem;           /* radianos: + o teto vai para a esquerda */
    float subida;            /* metros */
    float giro;              /* radianos somados ao rumo (o rodopio) */
    float avanco;            /* metros para a frente (o tranco) */
    float maos;              /* as rodas da frente gesticulando (-1 a +1) */
    float garfos;            /* 0 a 1: os garfos do Guido */
    float cruzaGarfos;       /* 0 a 1: os garfos do Guido cruzados como braços */
    float raio;              /* 0 a 1: o raio de luz de um flash no McQueen, piscando (o "Katchau") */
    float boca;              /* 0 a 1: no mínimo isto */
    float tremor;            /* 0 a 1: o motor acelerando, o corpo tremendo */
    float expressoes[TOTAL_EXPRESSOES];   /* o peso de cada expressão dos olhos (olhos.h), somada à da cara */
    float olhar;             /* somado ao para onde ele olha (de lado: + para a direita do carro) */
    float piscadela;         /* um olho fechando: + o direito do carro, - o esquerdo */
} Firula;

typedef struct {
    int numero;              /* o carro no palco (-1: nenhum) */
    int proximo;             /* o que entra quando este sair (-1: nenhum) */
    FaseDoPalco fase;
    float tempo;             /* segundos na fase */
    /* Onde ele está */
    float x, y;              /* metros: 0 é o meio do disco; y, a altura (no pulo) */
    float rumo;              /* radianos, como no mundo do jogo */
    float rumoNoComeco;      /* da saída: o rumo de onde ela começou */
    float velocidade;        /* m/s, para a frente (negativa: de ré) */
    float giroDasRodas;      /* radianos */
    float volante;           /* -1 (esquerda) a +1 */
    float olhar;             /* -1 a +1: para onde os olhos vão andando (o volante) */
    int naCamera;            /* 1: os olhos na câmera (o contato visual com quem escolhe) */
    float desvioDoOlhar;     /* somado, olhando para a câmera: a olhadela rápida para o lado, que volta */
    /* A suspensão do palco: as acelerações da entrada e da saída */
    float arfagem, velocidadeDaArfagem;   /* radianos: + o bico desce (freando), como o aceno de gestos.h */
    float rolagem, velocidadeDaRolagem;
    float subida, velocidadeDaSubida;     /* metros: os pulos (a aterrissagem, a hidráulica) */
    float pancada;           /* 0 a 1: chegou no chão agora (só no quadro da aterrissagem) */
    /* O corpo e a cara (gestos.c) */
    Jeito jeito;
    Corpo corpo;
    Pose pose;
    /* O anúncio do nome */
    const char *anuncio;     /* o nome sendo anunciado agora (NULL: nada) */
    float duracaoDoAnuncio;
    float tempoDoAnuncio;
    int anunciou;            /* já foi anunciado desta vez */
    int anuncioComecou;      /* 1 só no quadro em que o anúncio começa (a voz do locutor toca aí) */
    Firula firula;           /* o número dele agora, com a inércia do corpo (molas com a suspensão dele) */
    Firula velocidadeDaFirula;
    float velocidadeAntes;
} Palco;

void iniciarPalco(Palco *palco);
/* O carro que o jogador marcou: se é outro, o de agora sai pela direita e este entra pela esquerda */
void mostrarNoPalco(Palco *palco, int numero);
void atualizarPalco(Palco *palco, float tempoQuadro);
/* O nome que o locutor anuncia e o arquivo da voz dele (recursos/sons/vozes/<arquivo>.wav) */
const char *nomeNoPalco(int numero);
const char *vozDoCarroNoPalco(int numero);
/* Quanto tempo a voz do locutor durou (o anúncio fica na tela esse tempo; 0: o tempo das letras) */
void duracaoDoAnuncioNoPalco(Palco *palco, float segundos);

#endif
