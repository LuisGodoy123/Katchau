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

#include "tracado.h"

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
    PISTA_COPA_PISTAO,   /* o estádio do modelo 3D (copa_pistao.h) */
    PISTA_OVAL,          /* o oval de desenho animado (oval.h) */
    PISTA_SPLAT,         /* o circuito sinuoso, montado em blocos (splat.h) */
    TOTAL_PISTAS
} IdPista;

/* Comprimento de cada segmento em "unidades de mundo".
 * Serve para o desenho (Fase 2) e para medir quanto um carro andou. */
#define COMPRIMENTO_SEGMENTO  200.0f

/* Conversão das medidas reais do autódromo (em metros) para o jogo:
 * cada metro vira 0,6 segmento, ou seja, 1 metro = 120 unidades. */
#define SEGMENTOS_POR_METRO   0.6f
#define UNIDADES_POR_METRO    (COMPRIMENTO_SEGMENTO * SEGMENTOS_POR_METRO)

/* Largura de cada lado da pista, em metros, até alguém dizer o contrário */
#define MEIA_LARGURA_PADRAO   (2000.0f / UNIDADES_POR_METRO)

/* Interlagos vem do modelo 3D, que está em metros. Na medida real o asfalto tem de 10 a 16 m de
 * largura e, com 14 carros, quase não dá para ultrapassar: o modelo inteiro (traçado, prédios,
 * pit lane) é usado 40% maior. Para a medida real, é só voltar para 1 */
#define ESCALA_INTERLAGOS        1.4f    /* o traçado do modelo, 40% maior: na medida real a pista é estreita demais para 14 carros */

/* A Copa Pistão tem o comprimento real de Daytona (2,5 milhas): a pista do filme
 * não tem medida oficial. A escala do modelo 3D sai disso: metros de uma volta
 * dividido pelo comprimento da linha de copa_pistao.h, em unidades do modelo. */
#define COMPRIMENTO_COPA_PISTAO  4023.0f
#define ESCALA_COPA_PISTAO       (COMPRIMENTO_COPA_PISTAO / 93.67f)

/* As outras pistas de modelo 3D: a escala deixa o asfalto com uma largura boa para correr */
#define ESCALA_OVAL              2.0f
#define ESCALA_SPLAT             24.0f

/* Grid de largada, pintado no asfalto ANTES da linha de chegada, em
 * zigue-zague (pares à esquerda, ímpares à direita). O lugar 0 (pole) é
 * o mais perto da linha. As distâncias são múltiplas do segmento, então
 * cada lugar cai bem no começo de um segmento. Uma pista pode ter o
 * próprio grid (o que já vem pintado no modelo): veja GridDaPista. */
#define LUGARES_NO_GRID       17   /* um para cada carro (TOTAL_CARROS, em corrida.h) */
#define DISTANCIA_DA_POLE     COMPRIMENTO_SEGMENTO            /* da pole até a linha */
#define ESPACO_ENTRE_LUGARES  (2.0f * COMPRIMENTO_SEGMENTO)   /* de um lugar ao seguinte */
#define LATERAL_DO_GRID       0.4f                            /* -0,4 à esquerda, +0,4 à direita */

/* Perto do centro de uma curva fechada, a linha da pista anda no máximo tanto mais rápido que um
 * carro longe dela, por dentro (veja metrosNoMundoPorMetroDaPista) */
#define MENOR_FATOR_NO_MUNDO  0.3f

/* O chão pode não ser plano na largura (as curvas do oval): cada segmento
 * guarda a altura dele em tantos lugares, da borda esquerda à direita. */
#define PONTOS_DO_PERFIL      9

/* O chão além da borda, onde ele foi medido (Interlagos): a tantos metros da borda */
#define PONTOS_FORA_DA_PISTA  PONTOS_ALEM_DA_BORDA

/* O que o carro tem embaixo das rodas */
typedef enum {
    SUPERFICIE_ASFALTO,
    SUPERFICIE_ZEBRA,
    SUPERFICIE_ESCAPE,    /* área de escape asfaltada: fora da pista, mas sem grama */
    SUPERFICIE_GRAMA
} Superficie;

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
    float limiteDireita, limiteEsquerda; /* metros da linha até o que para o carro em cada lado; 0: não há medida */

    /* Onde o começo do segmento fica no mundo 3D, em metros */
    float xMundo, zMundo;           /* centro da pista, visto de cima (a altura é o campo "altura") */
    float rumo;                     /* para onde a pista aponta, em graus: 0 = -z, 90 = +x */
    float larguraDireita;           /* da linha da pista até a borda direita do asfalto */
    float larguraEsquerda;          /* ...e até a esquerda (no oval os dois lados são diferentes) */
    float perfil[PONTOS_DO_PERFIL]; /* altura do chão em relação à linha, em metros, da borda esquerda à direita */

    /* Além da borda do asfalto, onde o modelo foi medido (temBorda = 1). Metros a partir da linha */
    int temBorda;
    float zebraDireita, zebraEsquerda;     /* até onde vai a zebra; 0: ali não tem */
    float escapeDireita, escapeEsquerda;   /* até onde o chão é asfaltado */
    float foraDireita[PONTOS_FORA_DA_PISTA], foraEsquerda[PONTOS_FORA_DA_PISTA];   /* altura do chão além da borda */
    float escalaDaBorda;            /* as distâncias além da borda estão nas medidas do modelo: vezes isto */
    struct Obstaculo *obstaculos;   /* lista simples dos objetos da beira (obstaculos.c) */
    struct Segmento *proximo;       /* segmento seguinte (o último aponta para o primeiro) */
    struct Segmento *anterior;      /* segmento anterior (o primeiro aponta para o último) */
} Segmento;

/* O pit lane de uma pista. Os metros são a partir da linha de chegada. O caminho
 * é uma faixa paralela à pista, na medida do que o modelo 3D mostra. */
typedef struct {
    int lado;              /* -1: à esquerda da pista; +1: à direita */
    int noModelo;          /* sempre 1: faixa paralela à pista, com as medidas abaixo */
    float entrada;         /* onde ele abre */
    float inicioLivre;     /* a partir daqui o jogador pode entrar */
    float fimDaZona;       /* até aqui ainda dá para entrar */
    float fimDaEntrada;    /* até aqui ele entra em velocidade reduzida */
    float velocidadeNaEntrada;
    float aviso;           /* a partir daqui o painel avisa do box */
    float inicioDoLimite, fimDoLimite;   /* as linhas do limite de velocidade */
    float fim;             /* onde o carro já voltou à pista */
    float primeiraVaga;    /* a vaga do carro 0: as outras seguem depois dela */
    float entreVagas;      /* metros de uma vaga até a próxima */
    float encostar;        /* a tantos metros da vaga ele deixa a faixa rápida */

    /* Só no pit lane do modelo: metros da linha da pista até cada coisa */
    float noCaminhoDaFaixa;  /* onde ele já está na faixa rápida */
    float deixaAFaixa;       /* onde começa a voltar para a pista */
    float borda;             /* o lugar na pista onde ele sai dela e volta */
    float faixa;             /* a faixa rápida */
    float vaga;              /* a vaga */
    float paraEntrar;        /* o jogador entra quando passa disto, na zona de entrada */
    int soEncostadoNaBorda;  /* 1: paraEntrar é a distância até a borda (só entra quem encosta na linha) */
    const float (*caminho)[2];  /* se a estrada dobra: { metros a partir da linha, metros até a linha da pista },
                                   começando na entrada */
    int pontosDoCaminho;
} DadosDoPit;

/* O grid que a pista já traz pintado (o do modelo de Interlagos). Com "existe" em 0,
 * vale o grid padrão do jogo, pintado por ele */
typedef struct {
    int existe;
    float pole;        /* metros do meio do carro da pole até a linha */
    float espaco;      /* metros de um lugar ao seguinte (que fica do outro lado) */
    float lateral;     /* metros da linha da pista até o meio de cada fila */
    int poleADireita;  /* 1: a pole fica à direita */
} GridDaPista;

/* A pista em si guarda só a "porta de entrada" da lista circular.
 * Ela existe como struct para que funções que mudam o início da lista
 * (inserir na pista vazia, remover a largada) possam atualizá-lo sem
 * usar ponteiro para ponteiro. */
typedef struct {
    Segmento *largada;   /* segmento da linha de largada/chegada; NULL se a pista estiver vazia */
    int temPit;          /* 1: tem pit lane e a parada é obrigatória */
    DadosDoPit pit;
    int semDirecaoGuiada; /* 1: aqui o jogador sempre dirige sem a direção guiada, seja qual for a opção */
    GridDaPista grid;
    int largadaPintadaNoModelo;   /* 1: a linha de chegada e o grid já vêm pintados no modelo */
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
Pista *criarOval(void);
Pista *criarSplat(void);
Pista *criarPistaPorId(IdPista id);
float metrosDesdeAEntrada(const Pista *pista, float metros);
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
Superficie superficieNaLateral(PosicaoNaPista posicao, float lateralEmMetros);
float distanciaDoLugarAteALinha(const Pista *pista, int lugar);
float lateralDoLugar(const Pista *pista, int lugar);
void liberarPista(Pista *pista);

/* ---------- Andar pela pista ---------- */
int moverNaPista(PosicaoNaPista *posicao, float distancia);
float alturaNaPosicao(PosicaoNaPista posicao);
PontoNoMundo pontoNoMundo(PosicaoNaPista posicao, float deslocamentoLateral);
float alturaDoChao(PosicaoNaPista posicao, float lateralEmMetros);
float metrosNoMundoPorMetroDaPista(PosicaoNaPista posicao, float lateralEmMetros);
float larguraDoLado(Segmento *segmento, float lateral);
float larguraNaPosicao(PosicaoNaPista posicao, float lateral);

#endif
