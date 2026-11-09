/*
 * colisao.h
 * ------------------------------------------------------------------
 * Os triângulos sólidos do mundo aberto (recursos/modelos/radiator_springs.col,
 * gerado por ferramentas/montar_mundo_aberto.py), e as perguntas que a
 * física do passeio livre faz a eles:
 *
 *   - onde é o chão embaixo deste ponto? (de cima para baixo)
 *   - um raio saindo daqui, nesta direção, bate em quê? (em qualquer direção:
 *     é o que deixa o carro andar em rampas, barrancos e paredes)
 *   - este pedaço do carro está encostado numa parede? (e para onde ela o empurra)
 *
 * Cada triângulo sabe de que chão ele é (asfalto, areia, terra, rocha) e a cor
 * média da textura dele: a aderência, a poeira e o som saem daí.
 *
 * Os triângulos ficam numa grade vista de cima: cada pergunta olha só as
 * células em volta. Tudo em metros, nos eixos do jogo (y para cima).
 * Não usa a raylib (regra 2 do roadmap): só lê o arquivo e faz geometria.
 * ------------------------------------------------------------------
 */

#ifndef COLISAO_H
#define COLISAO_H

#define SEM_CHAO (-1.0e9f)   /* alturaDoChaoNaMalha: não há chão embaixo do ponto */

/* Os tipos de chão (os mesmos números do .col) */
typedef enum {
    CHAO_ASFALTO,
    CHAO_AREIA,     /* areia solta: escorrega mais e levanta poeira */
    CHAO_TERRA,     /* terra batida (o oval de treino) */
    CHAO_ROCHA,
    CHAO_OUTRO,     /* prédios, cercas, placas */
    TOTAL_DE_CHAOS
} TipoDeChao;

/* O que um raio encontrou */
typedef struct {
    float ponto[3];
    float normal[3];       /* virada para o lado de onde o raio veio */
    float distancia;
    int superficie;        /* a entrada da paleta (tipoDoChao, corDoChao) */
} Acerto;

typedef struct MalhaDeColisao MalhaDeColisao;

MalhaDeColisao *carregarMalhaDeColisao(const char *arquivo);
MalhaDeColisao *criarMalhaDeColisao(const float *vertices, int totalDeTriangulos);
void liberarMalhaDeColisao(MalhaDeColisao *malha);
int triangulosDaMalha(const MalhaDeColisao *malha);
void limitesDaMalha(const MalhaDeColisao *malha, float minimo[3], float maximo[3]);
int pontosDaMalha(const MalhaDeColisao *malha, float largada[4], float arena[3]);
TipoDeChao tipoDoChao(const MalhaDeColisao *malha, int superficie);
void corDoChao(const MalhaDeColisao *malha, int superficie, unsigned char cor[3]);

float alturaDoChaoNaMalha(MalhaDeColisao *malha, float x, float z, float teto, float normal[3]);
int raioNaMalha(MalhaDeColisao *malha, const float origem[3], const float direcao[3], float alcance, Acerto *acerto);
int empurrarDasParedes(MalhaDeColisao *malha, const float centro[3], float raio, const float cima[3], float empurrao[3]);

#endif
