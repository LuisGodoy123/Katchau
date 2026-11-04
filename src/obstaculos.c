/*
 * obstaculos.c
 * ------------------------------------------------------------------
 * Lista simples de objetos de cada segmento e a decoração de
 * Interlagos. Veja obstaculos.h para a ideia geral.
 * ------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "obstaculos.h"

/* Onde cada coisa fica, em larguras de meia pista a partir do centro.
 * O asfalto vai até ±1 e o carro anda até ±2,5 (LIMITE_LATERAL em
 * carro.c): o que fica além de 2,5 nunca está no caminho do carro. */
#define LADO_PNEUS          2.7f   /* barreira de pneus por fora das curvas */
#define LADO_PLACAS         2.7f
#define LADO_ARVORE_MINIMO  3.0f
#define LADO_ARVORE_MAXIMO  8.0f

/* Num circuito sinuoso, o lugar de um objeto pode cair em cima de OUTRO trecho
 * da pista. Por isso ele é conferido contra a volta inteira: precisa ficar além
 * de onde o carro consegue chegar em qualquer trecho, mais o tamanho do objeto. */
#define ALCANCE_DO_CARRO    2.5f   /* LIMITE_LATERAL em carro.c, em larguras da pista */
#define FOLGA_DA_ARVORE     7.0f   /* metros: o raio da copa */
#define FOLGA_DOS_PNEUS     1.0f
#define FOLGA_DAS_PLACAS    1.0f
#define LADO_DA_ARQUIBANCADA 3.0f  /* lateral da frente dela: ~50 m da linha da pista, além dos pneus */
#define RAIO_DO_MODULO       18.0f  /* metros, do centro de um módulo até o canto */
#define PASSO_DA_CONFERENCIA 3     /* confere um a cada 3 segmentos (5 m) */

/* De quantos em quantos segmentos se repete cada coisa */
#define ESPACO_PNEUS        6

#define CHANCE_ARVORE       0.30f  /* em cada lado de cada segmento */
#define SEMENTE_DECORACAO   2026u  /* sorteio fixo: a pista é sempre igual */

/* ==================================================================
 * Lista simples de um segmento
 * ================================================================== */

/*
 * adicionarObstaculo
 * Cria um objeto e o coloca no começo da lista do segmento. Inserir no
 * começo é O(1): não precisa andar até o fim da lista.
 * Recebe: o segmento, o tipo do objeto e onde ele fica na lateral.
 * Devolve: nada.
 */
void adicionarObstaculo(Segmento *segmento, TipoObjeto tipo, float deslocamentoLateral) {
    Obstaculo *novo = (Obstaculo *) malloc(sizeof(Obstaculo));
    if (novo == NULL) {
        printf("Erro: memoria insuficiente para criar obstaculo.\n");
        exit(1);
    }

    novo->tipo = tipo;
    novo->deslocamentoLateral = deslocamentoLateral;
    novo->setor = 0;
    novo->proximo = segmento->obstaculos;   /* o antigo primeiro vira o segundo */
    segmento->obstaculos = novo;
}

/*
 * contarObstaculos
 * Conta os objetos de um segmento, andando pela lista até o NULL.
 * Recebe: o segmento.
 * Devolve: quantos objetos ele tem.
 */
int contarObstaculos(Segmento *segmento) {
    int total = 0;
    for (Obstaculo *atual = segmento->obstaculos; atual != NULL; atual = atual->proximo) {
        total++;
    }
    return total;
}

/*
 * liberarObstaculos
 * Devolve a memória de todos os objetos de um segmento.
 * Recebe: o segmento.
 * Devolve: nada (a lista do segmento fica vazia).
 */
void liberarObstaculos(Segmento *segmento) {
    Obstaculo *atual = segmento->obstaculos;
    while (atual != NULL) {
        Obstaculo *seguinte = atual->proximo;   /* guarda antes de liberar */
        free(atual);
        atual = seguinte;
    }
    segmento->obstaculos = NULL;
}

/* ==================================================================
 * Decoração de Interlagos
 * ================================================================== */

/* Sorteio próprio (gerador congruente linear), em vez do rand(): com a
 * mesma semente ele dá sempre a mesma sequência em qualquer sistema,
 * então as árvores ficam sempre no mesmo lugar. */
static unsigned int semente;

/*
 * sortear
 * Recebe: nada.
 * Devolve: um número entre 0 e 1.
 */
static float sortear(void) {
    semente = semente * 1103515245u + 12345u;
    return ((semente >> 16) & 0x7FFF) / 32767.0f;
}

/*
 * lugarLivre
 * Diz se um objeto pode ficar num lugar: longe, no mundo, de toda a pista.
 * Recebe: a pista, o segmento e a lateral do objeto, e o raio dele em metros.
 * Devolve: 1 se está livre, 0 se cairia onde o carro pode passar.
 */
int lugarLivre(Pista *pista, Segmento *segmento, float lateral, float folga) {
    PosicaoNaPista lugar = { segmento, 0.0f };
    PontoNoMundo ponto = pontoNoMundo(lugar, lateral);

    Segmento *atual = pista->largada;
    int indice = 0;
    do {
        if (indice++ % PASSO_DA_CONFERENCIA == 0) {
            float largura = (atual->larguraDireita > atual->larguraEsquerda) ? atual->larguraDireita : atual->larguraEsquerda;
            float alcance = largura * (atual->temMuro ? atual->lugarDoMuro : ALCANCE_DO_CARRO) + folga;
            float dx = atual->xMundo - ponto.x, dz = atual->zMundo - ponto.z;
            if (dx * dx + dz * dz < alcance * alcance) {
                return 0;
            }
        }
        atual = atual->proximo;
    } while (atual != pista->largada);
    return 1;
}

/*
 * colocarPlacas
 * Coloca as placas de 150, 100 e 50 metros antes da entrada de uma
 * curva, por fora dela (curva à esquerda: placas à direita). A placa
 * só entra se o lugar dela for reta: se antes da curva vem outra
 * curva logo em seguida, as placas mais distantes ficam de fora.
 * Recebe: a pista e o primeiro segmento da curva.
 * Devolve: nada.
 */
static void colocarPlacas(Pista *pista, Segmento *entrada) {
    static const int METROS[] = { 50, 100, 150 };
    static const TipoObjeto PLACAS[] = { OBJETO_PLACA_50, OBJETO_PLACA_100, OBJETO_PLACA_150 };
    float lado = (entrada->curvatura < 0) ? LADO_PLACAS : -LADO_PLACAS;

    for (int p = 0; p < 3; p++) {
        int voltar = (int) (METROS[p] * SEGMENTOS_POR_METRO + 0.5f);
        Segmento *lugar = entrada;
        int ehReta = 1;
        for (int i = 0; i < voltar; i++) {
            lugar = segmentoAnterior(lugar);
            if (temZebra(lugar)) {
                ehReta = 0;   /* bateu em outra curva: não cabe esta placa */
            }
        }
        if (!ehReta || !lugarLivre(pista, lugar, lado, FOLGA_DAS_PLACAS)) {
            return;   /* as placas maiores ficariam mais longe ainda */
        }
        adicionarObstaculo(lugar, PLACAS[p], lado);
    }
}

/*
 * marcarGrid
 * Pinta o colchete de cada lugar do grid, nos segmentos antes da linha
 * de chegada onde os carros largam (distanciaDoLugarAteALinha em pista.c).
 * Recebe: a pista.
 * Devolve: nada.
 */
static void marcarGrid(Pista *pista) {
    for (int lugar = 0; lugar < LUGARES_NO_GRID; lugar++) {
        int voltar = (int) (distanciaDoLugarAteALinha(lugar) / COMPRIMENTO_SEGMENTO + 0.5f);
        Segmento *segmento = pista->largada;
        for (int i = 0; i < voltar; i++) {
            segmento = segmentoAnterior(segmento);
        }
        adicionarObstaculo(segmento, OBJETO_MARCA_GRID, lateralDoLugar(lugar));
    }
}

/* As arquibancadas e o camarote do mapa oficial do GP: as posições foram lidas no mapa
 * e alinhadas pelo início e fim de cada trecho. Na reta dos boxes elas ficam logo atrás
 * do muro; fora dela, além de onde o carro chega pela grama. */
static const Arquibancada ARQUIBANCADAS[] = {
    { 'A', 3740, 4185, 19.5f, 26, 1, OBJETO_ARQUIBANCADA },   /* fim da Subida dos Boxes e começo da reta */
    { 'B', 4197, 4280, 19.5f, 24, 1, OBJETO_ARQUIBANCADA },   /* reta dos boxes, antes da linha */
    {  0,    24,   77, 19.5f,  0, 0, OBJETO_CAMAROTE },       /* Pit Stop Club, logo depois da linha */
    { 'M',  100,  270, 19.5f, 20, 1, OBJETO_ARQUIBANCADA },   /* reta dos boxes, chegando no S do Senna */
    { 'D',  341,  510, 46.0f, 20, 0, OBJETO_ARQUIBANCADA },   /* por fora do S do Senna */
    { 'H',  532,  607, 46.0f, 16, 0, OBJETO_ARQUIBANCADA },   /* Curva do Sol */
    { 'N',  636,  667, 46.0f, 12, 0, OBJETO_ARQUIBANCADA },   /* Curva do Sol */
    { 'R',  678,  833, 46.0f, 18, 1, OBJETO_ARQUIBANCADA },   /* saída da Curva do Sol */
    { 'G',  857, 1299, 46.0f, 20, 1, OBJETO_ARQUIBANCADA },   /* Reta Oposta */
    { 'S', 1330, 1358, 46.0f, 12, 0, OBJETO_ARQUIBANCADA },   /* fim da Reta Oposta */
    { 'V', 1431, 1463, 46.0f, 14, 0, OBJETO_ARQUIBANCADA },   /* Descida do Lago */
};
#define TOTAL_ARQUIBANCADAS  (int) (sizeof(ARQUIBANCADAS) / sizeof(ARQUIBANCADAS[0]))

/* Onde ficam os módulos, para as árvores não nascerem dentro deles */
#define MAXIMO_DE_MODULOS 256
static float xDosModulos[MAXIMO_DE_MODULOS], zDosModulos[MAXIMO_DE_MODULOS];
static int totalDeModulos;

/*
 * arquibancadasDaPista
 * Recebe: onde guardar quantas são.
 * Devolve: a tabela, para o desenho (mundo.c).
 */
const Arquibancada *arquibancadasDaPista(int *total) {
    *total = TOTAL_ARQUIBANCADAS;
    return ARQUIBANCADAS;
}

/*
 * colocarArquibancada
 * Marca o trecho da arquibancada na lista de objetos: um marcador a cada
 * LARGURA_DO_MODULO metros, para as árvores não nascerem dentro dela. O do meio
 * leva a letra do setor. Quem constrói a arquibancada de verdade é o mundo.c.
 * Recebe: a pista e a arquibancada.
 * Devolve: nada.
 */
static void colocarArquibancada(Pista *pista, const Arquibancada *arquibancada) {
    float lateral = arquibancada->distancia / pista->largada->larguraDireita;
    float fundo = FUNDO_DO_MODULO / pista->largada->larguraDireita;
    Obstaculo *colocados[MAXIMO_DE_MODULOS];
    int total = 0;
    int primeiro = (int) (arquibancada->inicio * SEGMENTOS_POR_METRO);
    int ultimo = (int) (arquibancada->fim * SEGMENTOS_POR_METRO);
    int passo = (int) (LARGURA_DO_MODULO * SEGMENTOS_POR_METRO);

    for (int i = primeiro; i <= ultimo; i += passo) {
        Segmento *atual = buscarSegmento(pista, i);
        if (atual == NULL) {
            break;
        }
        adicionarObstaculo(atual, arquibancada->tipo, lateral);
        if (total < MAXIMO_DE_MODULOS) {
            colocados[total++] = atual->obstaculos;
        }
        PosicaoNaPista lugar = { atual, 0.0f };
        if (totalDeModulos < MAXIMO_DE_MODULOS) {
            PontoNoMundo meio = pontoNoMundo(lugar, lateral + fundo / 2.0f);
            xDosModulos[totalDeModulos] = meio.x;
            zDosModulos[totalDeModulos] = meio.z;
            totalDeModulos++;
        }
    }
    if (total > 0) {
        colocados[total / 2]->setor = arquibancada->setor;
    }
}

/*
 * longeDasArquibancadas
 * Recebe: a pista, o segmento e a lateral de uma árvore.
 * Devolve: 1 se ela fica fora de todos os módulos, 0 se nasceria dentro de um.
 */
static int longeDasArquibancadas(Segmento *segmento, float lateral) {
    PosicaoNaPista lugar = { segmento, 0.0f };
    PontoNoMundo ponto = pontoNoMundo(lugar, lateral);
    for (int i = 0; i < totalDeModulos; i++) {
        if (hypotf(ponto.x - xDosModulos[i], ponto.z - zDosModulos[i]) < RAIO_DO_MODULO + FOLGA_DA_ARVORE) {
            return 0;
        }
    }
    return 1;
}

/* Onde há mata de verdade em volta do autódromo: de onde até onde, em metros a
 * partir da linha, e de que lado (-1 dentro do circuito, +1 fora). Longe disso,
 * o campo é aberto, como na vida real. */
typedef struct {
    int inicio, fim, lado;
} Bosque;

static const Bosque BOSQUES[] = {
    {  720, 1320, -1 },   /* a mata de dentro, ao longo da Reta Oposta */
    { 1480, 1720, -1 },   /* em volta do Lago */
    { 1500, 1800,  1 },   /* por fora, atrás do Lago */
    { 3250, 3640,  1 },   /* o morro atrás da Subida dos Boxes */
};
#define TOTAL_BOSQUES  (int) (sizeof(BOSQUES) / sizeof(BOSQUES[0]))

/*
 * decorarPista
 * Põe os objetos da pista, numa volta pela lista:
 *   - o pórtico em cima da linha de chegada e o colchete de cada lugar
 *     do grid, antes dela;
 *   - barreiras de pneus por fora das curvas (as que têm zebra);
 *   - placas de 150, 100 e 50 m antes de cada curva;
 *   - arquibancadas e camarotes nos lugares do mapa oficial;
 *   - árvores sorteadas dos dois lados.
 * Tudo fica além de onde o carro consegue chegar, menos o pórtico, que
 * passa por cima da pista, e as marcas do grid, que são tinta no chão.
 * Recebe: a pista (já montada).
 * Devolve: nada.
 */
void decorarPista(Pista *pista) {
    if (pista == NULL || pista->largada == NULL) {
        return;
    }
    semente = SEMENTE_DECORACAO;

    adicionarObstaculo(pista->largada, OBJETO_PORTICO, 0.0f);
    marcarGrid(pista);
    totalDeModulos = 0;
    for (int i = 0; i < TOTAL_ARQUIBANCADAS; i++) {
        colocarArquibancada(pista, &ARQUIBANCADAS[i]);
    }

    Segmento *atual = pista->largada;
    do {
        if (temZebra(atual)) {
            if (atual->indice % ESPACO_PNEUS == 0) {
                float fora = (atual->curvatura < 0) ? LADO_PNEUS : -LADO_PNEUS;
                if (lugarLivre(pista, atual, fora, FOLGA_DOS_PNEUS)) {
                    adicionarObstaculo(atual, OBJETO_PNEUS, fora);
                }
            }
            if (!temZebra(segmentoAnterior(atual))) {
                colocarPlacas(pista, atual);
            }
        }

        /* Árvores: só nos bosques, com um sorteio para cada segmento */
        for (int b = 0; b < TOTAL_BOSQUES; b++) {
            float metros = atual->indice / SEGMENTOS_POR_METRO;
            int lado = BOSQUES[b].lado;
            if (metros >= BOSQUES[b].inicio && metros <= BOSQUES[b].fim && sortear() < CHANCE_ARVORE) {
                float distancia = LADO_ARVORE_MINIMO
                                  + sortear() * (LADO_ARVORE_MAXIMO - LADO_ARVORE_MINIMO);
                if (lugarLivre(pista, atual, lado * distancia, FOLGA_DA_ARVORE) && longeDasArquibancadas(atual, lado * distancia)) {
                    adicionarObstaculo(atual, OBJETO_ARVORE, lado * distancia);
                }
            }
        }

        atual = proximoSegmento(atual);
    } while (atual != pista->largada);
}

/*
 * decorarOval
 * No oval, só as marcas do grid: todo o resto do cenário (muros, placas,
 * arquibancadas) já está no modelo 3D.
 * Recebe: a pista (já montada).
 * Devolve: nada.
 */
void decorarOval(Pista *pista) {
    if (pista != NULL && pista->largada != NULL) {
        marcarGrid(pista);
    }
}
