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

/* As pistas do jogo */
typedef enum {
    PISTA_INTERLAGOS,
    PISTA_COPA_PISTAO,   /* o oval do modelo 3D (copa_pistao.h) */
    TOTAL_PISTAS
} IdPista;

/* Comprimento de cada segmento em "unidades de mundo".
 * Serve para o desenho (Fase 2) e para medir quanto um carro andou. */
#define COMPRIMENTO_SEGMENTO  200.0f

/* Conversão das medidas reais do autódromo (em metros) para o jogo:
 * cada metro vira 0,6 segmento, ou seja, 1 metro = 120 unidades. */
#define SEGMENTOS_POR_METRO   0.6f
#define UNIDADES_POR_METRO    (COMPRIMENTO_SEGMENTO * SEGMENTOS_POR_METRO)

/* Largura de cada lado da pista, em metros, onde não há outra medida (Interlagos) */
#define FIM_DO_MURO           320.0f    /* metros depois da linha: onde o muro da reta acaba */
#define INICIO_DO_FUNIL       3650.0f   /* metros: o muro da reta dos boxes começa aqui, no fim da Subida */
#define COMPRIMENTO_DO_FUNIL  70.0f
#define ABERTURA_DO_FUNIL     2.5f      /* larguras da pista: até onde o carro pode ir pela grama */
#define LARGURA_DO_PIT_LANE   14.0f
#define ENTRADA_DO_PIT        3400.0f   /* metros: o pit lane abre colado na pista, na Subida dos Boxes */
#define METROS_ABRINDO_O_PIT  90.0f     /* em tantos metros ele já tem a largura toda, ainda colado */
#define INICIO_DA_SEPARACAO   3490.0f   /* aqui ele começa a se afastar da pista */
#define FIM_DA_SAIDA_DA_PISTA 3640.0f   /* e já se afastou */
#define LADO_DA_ILHA          1.20f     /* larguras: até aqui, entre a pista e o pit lane, é asfalto pintado */
#define PIT_ATRAS_DO_FUNIL    2.62f     /* larguras da pista: passa por trás do muro do funil */
#define INICIO_DO_DESVIO_DA_SAIDA 200.0f /* metros: depois dos boxes, ele se afasta do muro da reta */
#define FIM_DO_DESVIO_DA_SAIDA 320.0f   /* e já passou da ponta do muro */
#define LADO_DA_SAIDA         1.6f      /* larguras da pista: por dentro do S do Senna e da Curva do Sol */
#define INICIO_DA_VOLTA       700.0f    /* metros: na Reta Oposta, ele começa a se aproximar da pista */
#define FIM_DA_VOLTA          800.0f    /* e já está colado nela */
#define FIM_DO_PIT_LANE       900.0f    /* daqui até o fim ele fecha, colado, como uma faixa de aceleração */
#define METROS_DO_RETAO_DO_PIT 40.0f    /* depois do funil, ele encosta no muro */
#define PRIMEIRO_METRO_DOS_BOXES (-270.0f)  /* a partir da linha de chegada */
#define MEIA_LARGURA_PADRAO   (2000.0f / UNIDADES_POR_METRO)

/* A Copa Pistão tem o comprimento real de Daytona (2,5 milhas): a pista do filme
 * não tem medida oficial. A escala do modelo 3D sai disso: metros de uma volta
 * dividido pelo comprimento da linha de copa_pistao.h, em unidades do modelo. */
#define COMPRIMENTO_COPA_PISTAO  4023.0f
#define ESCALA_COPA_PISTAO       (COMPRIMENTO_COPA_PISTAO / 93.67f)

/* Grid de largada, pintado no asfalto ANTES da linha de chegada, em
 * zigue-zague (pares à esquerda, ímpares à direita). O lugar 0 (pole) é
 * o mais perto da linha. As distâncias são múltiplas do segmento, então
 * cada lugar cai bem no começo de um segmento. */
#define LUGARES_NO_GRID       14
#define DISTANCIA_DA_POLE     COMPRIMENTO_SEGMENTO            /* da pole até a linha */
#define ESPACO_ENTRE_LUGARES  (2.0f * COMPRIMENTO_SEGMENTO)   /* de um lugar ao seguinte */
#define LATERAL_DO_GRID       0.4f                            /* -0,4 à esquerda, +0,4 à direita */

/* O chão pode não ser plano na largura (as curvas do oval): cada segmento
 * guarda a altura dele em tantos lugares, da borda esquerda à direita. */
#define PONTOS_DO_PERFIL      9

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
    float asfaltoEsquerda;          /* até onde é asfalto, à esquerda, em larguras: mais que 1 onde o pit lane encosta */
    int temMuro;                    /* 1: muro nos dois lados; 0: grama além do asfalto */
    float lugarDoMuro;              /* onde fica o muro, em larguras da pista (1 = na borda do asfalto) */

    /* Onde o começo do segmento fica no mundo 3D, em metros */
    float xMundo, zMundo;           /* centro da pista, visto de cima (a altura é o campo "altura") */
    float rumo;                     /* para onde a pista aponta, em graus: 0 = -z, 90 = +x */
    float larguraDireita;           /* da linha da pista até a borda direita do asfalto */
    float larguraEsquerda;          /* ...e até a esquerda (no oval os dois lados são diferentes) */
    float perfil[PONTOS_DO_PERFIL]; /* altura do chão em relação à linha, em metros, da borda esquerda à direita */
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
    int temPit;          /* 1: tem pit lane e a parada é obrigatória */
    float metrosDaVolta;
    IdPista id;          /* qual pista é (define o cenário 3D em mundo.c) */
    const char *nome;    /* o nome que aparece no menu */
} Pista;

/* Onde algo (a câmera, um carro) está na pista:
 * em qual nó da lista e quanto já andou dentro dele. */
typedef struct {
    Segmento *segmento;          /* nó da lista onde está */
    float distanciaNoSegmento;   /* de 0 até COMPRIMENTO_SEGMENTO */
} PosicaoNaPista;

/* Um ponto da pista no mundo 3D, em metros (y é a altura) */
typedef struct {
    float x, y, z;
    float rumo;         /* para onde a pista aponta ali, em graus */
    float inclinacao;   /* quanto o chão sobe para a direita, por metro (média em volta do ponto) */
} PontoNoMundo;

/* ---------- Criação ---------- */
Pista *criarPistaVazia(void);
Pista *criarPista(void);
Pista *criarCopaPistao(void);
Pista *criarPistaPorId(IdPista id);
float ladoDentroDoPit(float metros, float metrosDaVolta, float *largura);
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
PontoNoMundo pontoNoMundo(PosicaoNaPista posicao, float deslocamentoLateral);
float alturaDoChao(PosicaoNaPista posicao, float lateralEmMetros);
float larguraDoLado(Segmento *segmento, float lateral);

#endif
