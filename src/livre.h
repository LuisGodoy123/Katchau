/*
 * livre.h
 * ------------------------------------------------------------------
 * O carro do passeio livre: sem pista, sem lista circular. Ele anda em
 * qualquer direção, e o "para cima" dele é o do chão embaixo das rodas:
 * sobe rampas e barrancos e, rápido o bastante, anda nas paredes (wallride).
 * O chão e as paredes vêm da malha de colisão (colisao.h). No chão, ele
 * dirige com a mesma conta da corrida (a direção livre de carro.c): o
 * volante, a curva, o freio, a ré, o nitro e o drift são iguais.
 *
 * Eixos do jogo, em metros: y para cima. O rumo é 0 olhando para -z e
 * cresce virando à direita (como rumoNoMundo do carro da corrida).
 * Não usa a raylib (regra 2 do roadmap).
 * ------------------------------------------------------------------
 */

#ifndef LIVRE_H
#define LIVRE_H

#include "carro.h"
#include "motor.h"
#include "colisao.h"

typedef struct {
    float x, y, z;              /* metros: o meio do carro, no chão */
    float vx, vy, vz;           /* metros por segundo */
    float cima[3];              /* o "para cima" do carro: a normal do chão embaixo dele */
    float frente[3];            /* para onde o bico aponta (perpendicular a cima) */
    float rumo;                 /* radianos: o bico visto de cima (para a câmera e o mapa) */
    int noChao;                 /* 1: as rodas estão em alguma superfície (chão, rampa ou parede) */
    int naParede;               /* 1: andando numa superfície em pé (wallride) */
    float segundosNaParede;
    int superficie;             /* a entrada da paleta embaixo das rodas (tipoDoChao, corDoChao) */
    TipoDeChao chao;
    float volante;              /* -1 a +1 */
    Motor motor;
    float nitro;                /* 0 a 1 */
    int nitroLigado;
    float anguloDasRodas;       /* radianos (só para o desenho) */
    float escorregando;         /* 0 a 1: o quanto a traseira está solta (marca, fumaça e som) */
    float pancada;              /* 0 a 1: a força da última batida (o som consome e zera) */
    int raspando;               /* 1 enquanto encosta numa parede */
    float segundosNoAr;
    int acelerando, freando, freioDeMao;   /* os comandos do último quadro (fumaça e marcas) */
    int segurado;               /* 1: parado com o freio de mão puxado (o acelerador só faz o motor roncar) */
    float roncando;             /* 0 a 1: o quanto o motor está roncando assim, parado (os tratores escutam) */
    int quedas;                 /* quantas vezes caiu do mundo e voltou ao começo */
    float doDriftAntes;         /* m/s para o lado que o drift somou no passo anterior */

    /* O carro da corrida, sem pista: a aceleração, o freio, a ré, o nitro, o volante, a curva, o drift e a
     * suspensão são os dele (dirigirSemPista), para o carro responder igual nos dois modos. Os campos
     * motor, nitro, nitroLigado, volante e anguloDasRodas aqui em cima são cópias dele */
    Carro corrida;

    /* A ficha, já em metros */
    float velocidadeMaxima;     /* metros por segundo */
    float aceleracao;           /* metros por segundo ao quadrado, na arrancada */
    float aderencia;            /* 0 a 1 */
    float comprimento, largura; /* metros */

    /* Onde ele começou (para voltar se cair do mundo ou pedir) */
    float xInicial, yInicial, zInicial, rumoInicial;
} CarroLivre;

void iniciarCarroLivre(CarroLivre *carro, const FichaTecnica *ficha, float x, float y, float z, float rumo);
void atualizarCarroLivre(CarroLivre *carro, Comandos comandos, MalhaDeColisao *malha, float tempoQuadro);
void voltarAoComeco(CarroLivre *carro, MalhaDeColisao *malha);
void levarCarroPara(CarroLivre *carro, MalhaDeColisao *malha, float x, float z, float rumo);
float velocidadeParaFrente(const CarroLivre *carro);
float velocidadeDoCarroLivre(const CarroLivre *carro);

#endif
