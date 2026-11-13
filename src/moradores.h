/*
 * moradores.h
 * ------------------------------------------------------------------
 * Os moradores de Radiator Springs no mundo aberto: o McQueen (quando o
 * jogador está com outro carro), o Mate, a Sally, o Doc, o Luigi, o Guido,
 * o Ramone, a Flo, o Xerife, o Sargento, o Fillmore, a Lizzie e o Ruivo.
 * (O Mack fica parado na rua principal: passeio.c.)
 *
 * Cada um tem a sua casa (o lugar dele na cidade) e uma vida de dia: fica
 * em casa um tempo e sai para um dos programas dele, sorteado pelo gosto
 * de cada um (o café da Flo, a loja do Luigi, um passeio pela rodovia, um
 * treino no oval, a ronda do xerife...). Alguns programas são a dois: um
 * passa na casa do outro, conversam e vão juntos (o Mate chama o McQueen,
 * a Sally chama o McQueen para passear, o Doc chama o McQueen para
 * treinar); o Guido vai sempre atrás do Luigi. Estacionados perto, eles
 * conversam (olham um para o outro, cada um fala na sua vez). Com o
 * jogador: olham para ele quando ele para perto, cumprimentam, se
 * assustam quando ele passa rápido rente a eles, e o xerife vai atrás de
 * quem passa correndo perto do esconderijo dele.
 *
 * Cada um se mexe do seu jeito (gestos.c), pelo ânimo dele, que sobe e
 * desce com o que acontece: o jogador chegando devagar (companhia) ou
 * passando rente e rápido (perigo), uma batida, a fala dele e a de quem
 * conversa com ele, a noite.
 *
 * O jogador conversa com eles: apertando o botão de conversar perto de um,
 * na frente ou do lado (o quadrado, o X, ou a tecla E), até andando, os
 * dois trocam as falas de um dos papos dele (papo.h). Estacionado, o morador
 * fica até a conversa acabar; dirigindo, segue o caminho conversando.
 *
 * Entre um lugar e outro, andam pelas ruas, na faixa da direita, seguindo
 * o grafo das ruas, freiam atrás de quem vai na frente e esperam a vez de
 * entrar na rua.
 *
 * Os modelos 3D são os dos carros da corrida (carros3d.c) e mais os dos
 * figurantes, que não correm (a Flo, o Ruivo, o Xerife e a Lizzie), que
 * entram nos números depois dos carros da corrida.
 *
 * Não usa a raylib (regra 2 do roadmap): o desenho fica em passeio.c.
 * ------------------------------------------------------------------
 */

#ifndef MORADORES_H
#define MORADORES_H

#include "carro.h"
#include "colisao.h"
#include "gestos.h"
#include "papo.h"

/* Os figurantes: modelos de carros que não correm (os números deles vêm depois dos da corrida) */
#define TOTAL_DE_FIGURANTES 4
#define MODELO_DA_FLO       (TOTAL_CARROS + 0)
#define MODELO_DO_RUIVO     (TOTAL_CARROS + 1)
#define MODELO_DO_XERIFE    (TOTAL_CARROS + 2)
#define MODELO_DA_LIZZIE    (TOTAL_CARROS + 3)
#define TOTAL_DE_MODELOS    (TOTAL_CARROS + TOTAL_DE_FIGURANTES)

typedef struct {
    const char *arquivo;   /* em recursos/modelos/carros/ */
    const char *nome;
    float comprimento;     /* metros */
    float largura;
    int quartos;           /* quartos de volta para a frente ficar em +z: 0 (+z), 1 (+x), 2 (-z), 3 (-x) */
} FichaDeFigurante;

const FichaDeFigurante *fichaDoFigurante(int qual);   /* 0 a TOTAL_DE_FIGURANTES - 1 */
const char *nomeDoModelo(int modelo);
void medidasDoModelo(int modelo, float *comprimento, float *largura);

#define MAIS_MORADORES      16
#define MAIS_PONTOS_NO_CAMINHO 200

typedef struct {
    int modelo;              /* o carro (carros3d.c) */
    const char *nome;
    float x, y, z;           /* metros: o meio do carro, no chão */
    float rumo;              /* radianos: 0 olhando para -z, crescendo para a direita */
    float inclinacao;        /* radianos: o bico para cima, numa ladeira */
    float velocidade;        /* m/s */
    float volante;           /* -1 a +1 (as rodas da frente; andando, também os olhos) */
    float anguloDasRodas;    /* radianos */
    float comprimento, largura;
    float susto;             /* segundos: alguém bateu nele ou passou raspando */
    float batida;            /* 0 a 1: o jogador bateu nele agora (passeio.c conta; o corpo pula) */
    int buzinou;             /* 1 no quadro em que ele buzina (o jogador parado na frente dele) */

    /* A cara e o corpo, para o desenho */
    int olhaAlgo;            /* 1: os olhos vão para "olhar" (estacionado, olhando alguém), não para o volante */
    float olhar;             /* -1 (todo à esquerda) a +1 (todo à direita) */
    Pose pose;               /* o corpo (gestos.c): a suspensão, as rodas da frente como mãos e a emoção dos olhos */
    float aperto;            /* 0 a 1: os olhos apertados (o xerife atrás de alguém) */
    int perseguindo;         /* 1: o xerife atrás do jogador (no minimapa, em vermelho) */

    /* O que ele está dizendo (passeio.c mostra a fala de quem está perto do jogador) */
    const char *fala;        /* NULL: calado */
    float falaRestante;      /* segundos */
    int falaLonge;           /* 1: aparece mesmo longe do jogador (o xerife falando com ele) */

    /* A rotina e o caminho (moradores.c) */
    int pessoa;              /* quem é, na tabela das pessoas */
    int andando;
    float espera;            /* segundos que ainda faltam, parado */
    float bloqueado;         /* segundos parado atrás de alguém */
    float caminho[MAIS_PONTOS_NO_CAMINHO][2];
    int totalDoCaminho, proximo;
    float rumoDaChegada;     /* como ele estaciona no fim do caminho */
} Morador;

void iniciarMoradores(MalhaDeColisao *malha, int carroDoJogador);
/* Um passo da cidade. O jogador: onde está, para onde aponta e a velocidade (m/s) */
void atualizarMoradores(MalhaDeColisao *malha, float jogadorX, float jogadorZ, float jogadorRumo,
                        float jogadorVelocidade, float tempo);
int totalDeMoradores(void);
/* A hora do dia (0 a 24): de noitinha só se vai ao café da Flo, e tarde da noite todo mundo fica em casa */
void definirHoraDaCidade(float hora);
Morador *morador(int i);

/* A conversa com o jogador */
int moradorParaConversar(void);              /* quem está na frente ou do lado do jogador e pode conversar, até andando (-1: ninguém) */
int conversarComJogador(void);               /* o jogador apertou: começa a conversa ou passa a fala (1: há conversa) */
int moradorNaConversa(void);                 /* quem conversa com o jogador agora (-1: ninguém) */
const char *falaDoJogador(float *duracao);   /* a fala do jogador na conversa, agora (NULL: não é a vez dele) */
void encerrarConversa(void);
void enfaseDoJogador(float enfase);          /* a ênfase que o jogador deu agora: o morador responde com um "uhum" */
const Animo *animoDoMorador(int i);
float enfaseDoMorador(int i);
const Jeito *jeitoDoModelo(int modelo);      /* o jeito do carro (o do jogador, se ele é um dos moradores) */

#endif
