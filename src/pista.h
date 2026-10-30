/*
 * pista.h
 * ------------------------------------------------------------------
 * A pista do KATCHAU é uma LISTA CIRCULAR DUPLAMENTE ENCADEADA.
 *
 *   - Cada nó (Segmento) é um pedaço curto do autódromo.
 *   - "Circular": o último segmento aponta de volta para o primeiro,
 *     então andar para a frente sem parar dá voltas no circuito.
 *   - "Duplamente encadeada": cada segmento conhece o próximo E o
 *     anterior, o que permite andar para trás e remover um segmento
 *     sem precisar percorrer a lista atrás do vizinho.
 *
 * Este módulo NÃO usa a raylib. Ele só guarda e manipula dados,
 * por isso pode ser testado no terminal (testes/teste_pista.c).
 * ------------------------------------------------------------------
 */

#ifndef PISTA_H
#define PISTA_H

/* Curvatura = quantos graus a pista vira em cada segmento.
 * Negativo vira para a esquerda, positivo para a direita, zero é reta.
 * Como o circuito é fechado, a soma das curvaturas de uma volta dá
 * 360 graus. Interlagos é corrido no sentido anti-horário (vira mais
 * para a esquerda), então lá a soma dá -360. */
#define RETA            0.0f
#define CURVA_FECHADA   1.5f   /* 60 segmentos fazem 90 graus  */
#define CURVA_ABERTA    0.5f   /* 180 segmentos fazem 90 graus */

/* Na vida real a zebra só existe nas curvas. Aqui, um segmento tem zebra
 * quando vira pelo menos isto: a Reta Oposta (-0,04) e a Subida dos
 * Boxes (-0,16) quase não viram e ficam sem; todas as curvas passam de 0,5. */
#define CURVATURA_MINIMA_ZEBRA  0.3f

/* Comprimento de cada segmento em "unidades de mundo".
 * Serve para o desenho (Fase 2) e para medir quanto um carro andou. */
#define COMPRIMENTO_SEGMENTO  200.0f

/* Conversão das medidas reais do autódromo (em metros) para o jogo:
 * cada metro vira 0,6 segmento, ou seja, 1 metro = 120 unidades. */
#define SEGMENTOS_POR_METRO   0.6f
#define UNIDADES_POR_METRO    (COMPRIMENTO_SEGMENTO * SEGMENTOS_POR_METRO)

/* Grid de largada, pintado no asfalto ANTES da linha de chegada, em
 * zigue-zague (pares à esquerda, ímpares à direita). O lugar 0 (pole) é
 * o mais perto da linha. As distâncias são múltiplas do segmento, então
 * cada lugar cai bem no começo de um segmento. */
#define LUGARES_NO_GRID       8
#define DISTANCIA_DA_POLE     COMPRIMENTO_SEGMENTO            /* da pole até a linha */
#define ESPACO_ENTRE_LUGARES  (2.0f * COMPRIMENTO_SEGMENTO)   /* de um lugar ao seguinte */
#define LATERAL_DO_GRID       0.4f                            /* -0,4 à esquerda, +0,4 à direita */

/* Declarada só pelo nome: a struct fica em obstaculos.h (objetos da
 * beira da pista), que por sua vez precisa do Segmento daqui. */
struct Obstaculo;

/* Um nó da lista: um pedaço curto da pista. */
typedef struct Segmento {
    int indice;                     /* posição do segmento na pista (0 = largada) */
    float curvatura;                /* graus por segmento: < 0 esquerda, > 0 direita */
    int ehLinhaDeChegada;           /* 1 no segmento da largada/chegada, 0 nos outros */
    float altura;                   /* altura do chão no começo do segmento (unidades de mundo) */
    const char *nomeTrecho;         /* nome da parte do autódromo ("S do Senna", "Junção"...) */
    struct Obstaculo *obstaculos;   /* lista simples dos objetos da beira (obstaculos.c) */
    struct Segmento *proximo;       /* segmento seguinte (o último aponta para o primeiro) */
    struct Segmento *anterior;      /* segmento anterior (o primeiro aponta para o último) */
} Segmento;

/* A pista em si guarda só a "porta de entrada" da lista circular.
 * Ela existe como struct para que funções que mudam o início da lista
 * (inserir na pista vazia, remover a largada) possam atualizá-lo sem
 * usar ponteiro para ponteiro. */
typedef struct {
    Segmento *largada;   /* segmento da linha de largada/chegada; NULL se a pista estiver vazia */
} Pista;

/* Onde algo (a câmera, um carro) está na pista:
 * em qual nó da lista e quanto já andou dentro dele. */
typedef struct {
    Segmento *segmento;          /* nó da lista onde está */
    float distanciaNoSegmento;   /* de 0 até COMPRIMENTO_SEGMENTO */
} PosicaoNaPista;

/* ---------- Criação ---------- */
Pista *criarPistaVazia(void);
Pista *criarPista(void);
void adicionarTrecho(Pista *pista, const char *nome, int quantidade, float curvatura,
                     float alturaInicial, float alturaFinal);

/* ---------- Funções sobre a lista circular ---------- */
Segmento *inserirSegmento(Pista *pista, Segmento *depoisDe, float curvatura);
int removerSegmento(Pista *pista, Segmento *segmento);
Segmento *buscarSegmento(Pista *pista, int indice);
Segmento *proximoSegmento(Segmento *segmento);
Segmento *segmentoAnterior(Segmento *segmento);
int contarSegmentos(Pista *pista);
int temZebra(Segmento *segmento);
float distanciaDoLugarAteALinha(int lugar);
float lateralDoLugar(int lugar);
void liberarPista(Pista *pista);

/* ---------- Andar pela pista ---------- */
int moverNaPista(PosicaoNaPista *posicao, float distancia);
float alturaNaPosicao(PosicaoNaPista posicao);

#endif
