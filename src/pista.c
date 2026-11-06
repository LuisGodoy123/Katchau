/*
 * pista.c
 * ------------------------------------------------------------------
 * Implementação da pista como lista circular duplamente encadeada.
 * Veja pista.h para a explicação da estrutura.
 * ------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "pista.h"
#include "copa_pistao.h"
#include "oval.h"
#include "splat.h"

#define GRAUS_PARA_RADIANOS   0.017453292f
#define META_LARGURA_DA_INCLINACAO 3.0f   /* mede a inclinação 3 m para cada lado */
#define RAIO_DA_MEDIA_DO_NOME 40      /* segmentos para cada lado */
#define SUBPASSOS_DO_TRACADO  64      /* entre dois pontos da tabela: bem menor que um segmento */
#include "obstaculos.h"


/* ==================================================================
 * Funções auxiliares (static = só podem ser usadas dentro deste arquivo)
 * ================================================================== */

/*
 * criarSegmento
 * Reserva memória para um segmento solto, ainda fora da lista.
 * Recebe: a curvatura do segmento.
 * Devolve: o novo segmento, apontando para ele mesmo nos dois sentidos.
 */
static Segmento *criarSegmento(float curvatura) {
    Segmento *novo = (Segmento *) malloc(sizeof(Segmento));
    if (novo == NULL) {
        printf("Erro: memoria insuficiente para criar segmento.\n");
        exit(1);
    }

    novo->indice = 0;
    novo->curvatura = curvatura;
    novo->ehLinhaDeChegada = 0;
    novo->altura = 0.0f;
    novo->nomeTrecho = "";
    novo->temMuro = 0;
    novo->lugarDoMuro = 1.0f;
    novo->limiteDireita = 0.0f;
    novo->limiteEsquerda = 0.0f;
    novo->asfaltoEsquerda = 1.0f;
    novo->xMundo = 0.0f;
    novo->zMundo = 0.0f;
    novo->rumo = 0.0f;
    novo->larguraDireita = MEIA_LARGURA_PADRAO;
    novo->larguraEsquerda = MEIA_LARGURA_PADRAO;
    for (int i = 0; i < PONTOS_DO_PERFIL; i++) {
        novo->perfil[i] = 0.0f;   /* plano, até alguém dizer o contrário */
    }
    novo->obstaculos = NULL;
    novo->proximo = novo;    /* sozinho, ele é o próprio vizinho */
    novo->anterior = novo;
    return novo;
}

/*
 * renumerarSegmentos
 * Depois de inserir ou remover, os índices ficam fora de ordem.
 * Esta função percorre a pista uma vez a partir da largada e
 * escreve 0, 1, 2, ... em cada segmento.
 * Recebe: a pista.
 * Devolve: nada.
 */
static void renumerarSegmentos(Pista *pista) {
    if (pista->largada == NULL) {
        return;
    }

    Segmento *atual = pista->largada;
    int contador = 0;

    /* do-while: o corpo roda pelo menos uma vez, e o laço para
     * quando voltamos à largada (uma volta completa). */
    do {
        atual->indice = contador;
        contador++;
        atual = atual->proximo;
    } while (atual != pista->largada);
}

/* ==================================================================
 * Criação da pista
 * ================================================================== */

/*
 * criarPistaVazia
 * Cria uma pista sem nenhum segmento.
 * Recebe: nada.
 * Devolve: ponteiro para a pista nova (largada = NULL).
 */
Pista *criarPistaVazia(void) {
    Pista *pista = (Pista *) malloc(sizeof(Pista));
    if (pista == NULL) {
        printf("Erro: memoria insuficiente para criar a pista.\n");
        exit(1);
    }

    pista->largada = NULL;
    pista->id = PISTA_INTERLAGOS;
    pista->nome = "";
    pista->temPit = 0;
    pista->semDirecaoGuiada = 0;
    pista->metrosDaVolta = 0.0f;
    return pista;
}

/*
 * adicionarTrecho
 * Coloca vários segmentos no fim da pista (logo antes da largada), todos
 * com a mesma curvatura e o mesmo nome. Um "trecho" é, por exemplo, a
 * Reta Oposta ou o Bico de Pato.
 *
 * A altura vai de alturaInicial até alturaFinal em forma de "S"
 * (f * f * (3 - 2f), com f indo de 0 a 1): a rampa começa e termina
 * sem inclinação, então a estrada não dá trancos entre um trecho e outro.
 *
 * Recebe: a pista, o nome do trecho, quantos segmentos, a curvatura de
 *         cada um e a altura no começo e no fim do trecho.
 * Devolve: nada.
 */
void adicionarTrecho(Pista *pista, const char *nome, int quantidade, float curvatura,
                     float alturaInicial, float alturaFinal) {
    for (int i = 0; i < quantidade; i++) {
        float fracao = (float) i / quantidade;
        float suave = fracao * fracao * (3.0f - 2.0f * fracao);

        Segmento *novo = inserirSegmento(pista, NULL, curvatura);
        novo->altura = alturaInicial + (alturaFinal - alturaInicial) * suave;
        novo->nomeTrecho = nome;
    }
}

/* Metade da janela da suavização das curvas: cada segmento passa a ter a
 * média da curvatura dele e dos 10 vizinhos de cada lado (21 ao todo).
 * A entrada e a saída das curvas viram rampas de ~21 segmentos (~35 m). */
#define RAIO_DA_SUAVIZACAO  10

/*
 * suavizarCurvas
 * Sem isto, a curvatura salta de 0 para o valor da curva no primeiro
 * segmento dela: o carro leva o empurrão inteiro de uma vez e a estrada
 * "dobra" de repente na tela. Aqui cada segmento recebe a média da
 * curvatura dos vizinhos (média móvel), o que troca o degrau por uma
 * rampa. A média móvel numa lista circular não muda a soma das
 * curvaturas, então a volta continua virando -360 graus.
 *
 * As curvaturas originais vão antes para um vetor: a média precisa dos
 * valores antigos dos vizinhos, e a lista não tem acesso por índice.
 * Recebe: a pista.
 * Devolve: nada (altera a curvatura de cada segmento).
 */
static void suavizarCurvas(Pista *pista) {
    int total = contarSegmentos(pista);
    if (total == 0) {
        return;
    }

    float *original = (float *) malloc(total * sizeof(float));
    if (original == NULL) {
        printf("Erro: memoria insuficiente para suavizar a pista.\n");
        exit(1);
    }

    Segmento *atual = pista->largada;
    for (int i = 0; i < total; i++) {
        original[i] = atual->curvatura;
        atual = atual->proximo;
    }

    int janela = 2 * RAIO_DA_SUAVIZACAO + 1;
    for (int i = 0; i < total; i++) {
        float soma = 0.0f;
        for (int vizinho = -RAIO_DA_SUAVIZACAO; vizinho <= RAIO_DA_SUAVIZACAO; vizinho++) {
            soma += original[(i + vizinho + total) % total];   /* dá a volta, como a lista */
        }
        atual->curvatura = soma / janela;   /* "atual" voltou à largada, e anda junto com i */
        atual = atual->proximo;
    }

    free(original);
}

/* Um trecho do autódromo com as medidas do mundo real. */
typedef struct {
    const char *nome;
    float comprimentoMetros;
    float anguloGraus;      /* quanto a pista vira no trecho: < 0 esquerda, > 0 direita */
    float desnivelMetros;   /* quanto sobe (> 0) ou desce (< 0) até o fim do trecho */
} TrechoReal;

/*
 * Interlagos, na ordem em que é corrido (sentido anti-horário), a partir
 * da linha de chegada. Dados oficiais (Prefeitura de São Paulo):
 *   - 4.309 metros de extensão;
 *   - 11 curvas (o S do Senna conta como duas);
 *   - desnível de ~56 m: ponto mais alto na entrada do S do Senna (+4 m)
 *     e mais baixo na Descida do Lago (-52 m).
 * O comprimento e o ângulo de cada trecho são aproximados a partir da
 * planta do circuito, ajustados para o desenho fechar (soma dos
 * ângulos = -360 graus, soma dos desníveis = 0, soma dos comprimentos =
 * 4.309 m).
 */
static const TrechoReal TRECHOS_INTERLAGOS[] = {
    /* nome                         metros  ângulo  desnível   altura no fim */
    { "Reta dos Boxes",               329,     0,      4 },  /*  +4 (mais alto) */
    { "S do Senna",                    87,   -95,     -9 },  /*  -5 */
    { "S do Senna",                    77,    60,     -7 },  /* -12 */
    { "Curva do Sol",                 248,  -110,    -10 },  /* -22 */
    { "Reta Oposta",                  598,   -15,    -18 },  /* -40 */
    { "Descida do Lago",              115,   -90,     -8 },  /* -48 */
    { "Descida do Lago",              155,   -60,     -4 },  /* -52 (mais baixo) */
    { "Reta",                         118,     0,      3 },  /* -49 */
    { "Ferradura",                    298,   135,      5 },  /* -44 */
    { "Reta",                         111,     0,      4 },  /* -40 */
    { "Laranjinha",                   106,    80,      4 },  /* -36 */
    { "Reta",                         151,     0,      3 },  /* -33 */
    { "Pinheirinho",                  100,  -130,      1 },  /* -32 */
    { "Reta",                         141,     0,      0 },  /* -32 */
    { "Bico de Pato",                  70,   150,      0 },  /* -32 */
    { "Reta",                         111,     0,     -2 },  /* -34 */
    { "Mergulho",                     165,   -95,     -8 },  /* -42 */
    { "Reta",                         110,     0,     -2 },  /* -44 */
    { "Juncao",                       117,  -110,     -2 },  /* -46 */
    { "Subida dos Boxes",             823,   -80,     38 },  /*  -8 */
    { "Reta dos Boxes",               279,     0,      8 },  /*   0 (de volta à largada) */
};
#define TOTAL_TRECHOS_INTERLAGOS  (int) (sizeof(TRECHOS_INTERLAGOS) / sizeof(TRECHOS_INTERLAGOS[0]))

/*
 * calcularPosicoesNoMundo
 * Acha onde cada segmento fica no mundo 3D andando pela lista, como uma
 * tartaruga: anda um segmento para a frente e vira a curvatura dele. As
 * medidas são aproximadas e o fim pode não cair exatamente no começo, então
 * o erro é espalhado aos poucos para a pista fechar.
 * Recebe: a pista.
 * Devolve: nada (preenche xMundo, zMundo e rumo).
 */
static void calcularPosicoesNoMundo(Pista *pista) {
    float passo = COMPRIMENTO_SEGMENTO / UNIDADES_POR_METRO;
    int total = contarSegmentos(pista);
    float x = 0.0f, z = 0.0f, rumo = 0.0f;

    Segmento *atual = pista->largada;
    do {
        atual->xMundo = x;
        atual->zMundo = z;
        atual->rumo = rumo;
        x += sinf(rumo * GRAUS_PARA_RADIANOS) * passo;
        z -= cosf(rumo * GRAUS_PARA_RADIANOS) * passo;
        rumo += atual->curvatura;
        atual = atual->proximo;
    } while (atual != pista->largada);

    /* Agora x e z são o erro de fechamento */
    int i = 0;
    do {
        float fracao = (float) i / total;
        atual->xMundo -= x * fracao;
        atual->zMundo -= z * fracao;
        i++;
        atual = atual->proximo;
    } while (atual != pista->largada);
}

/*
 * murarRetaDosBoxes
 * A reta dos boxes tem muro dos dois lados, como na vida real. Ele começa no fim da
 * Subida dos Boxes num funil, para quem vem pela grama voltar aos poucos, e termina
 * antes do S do Senna, onde a área de escape abre.
 * Recebe: a pista.
 * Devolve: nada.
 */
static void murarRetaDosBoxes(Pista *pista) {
    Segmento *atual = pista->largada;
    do {
        float metros = atual->indice * COMPRIMENTO_SEGMENTO / UNIDADES_POR_METRO;
        if (metros >= INICIO_DO_FUNIL || metros <= FIM_DO_MURO) {
            atual->temMuro = 1;
            float noFunil = (metros < INICIO_DO_FUNIL + COMPRIMENTO_DO_FUNIL && metros >= INICIO_DO_FUNIL)
                            ? 1.0f - (metros - INICIO_DO_FUNIL) / COMPRIMENTO_DO_FUNIL : 0.0f;
            atual->lugarDoMuro = 1.0f + (ABERTURA_DO_FUNIL - 1.0f) * noFunil;
        }
        atual = atual->proximo;
    } while (atual != pista->largada);
}

/*
 * suave
 * Recebe: um valor de 0 a 1.
 * Devolve: o mesmo caminho de 0 a 1, começando e terminando devagar.
 */
static float suave(float t) {
    t = fminf(fmaxf(t, 0.0f), 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

/*
 * ladoDentroDoPit
 * O traçado do pit lane: abre colado na pista, na Subida dos Boxes, como uma faixa de
 * desaceleração; se afasta, passa por trás do funil, encosta no muro da reta, se afasta
 * de novo depois dos boxes e, por dentro do S do Senna e da Curva do Sol, volta a colar
 * na pista na Reta Oposta, como uma faixa de aceleração, e fecha.
 * Recebe: os metros desde a linha de chegada, o comprimento da volta e onde guardar a
 *         largura do pit lane ali (em metros).
 * Devolve: onde fica a borda dele perto da pista, em larguras (ou 0 se ali não há pit lane).
 */
float ladoDentroDoPit(float metros, float metrosDaVolta, float *largura) {
    int depoisDaLinha = (metros <= FIM_DO_PIT_LANE);
    float noPit = depoisDaLinha ? metros + metrosDaVolta : metros;
    if (!depoisDaLinha && metros < ENTRADA_DO_PIT) {
        return 0.0f;
    }
    float abrindo = suave((noPit - ENTRADA_DO_PIT) / METROS_ABRINDO_O_PIT);
    float fechando = suave((metrosDaVolta + FIM_DO_PIT_LANE - noPit) / (FIM_DO_PIT_LANE - FIM_DA_VOLTA));
    *largura = LARGURA_DO_PIT_LANE * fminf(abrindo, fechando);

    if (depoisDaLinha) {
        float afastado = suave((metros - INICIO_DO_DESVIO_DA_SAIDA) / (FIM_DO_DESVIO_DA_SAIDA - INICIO_DO_DESVIO_DA_SAIDA));
        float voltando = suave((metros - INICIO_DA_VOLTA) / (FIM_DA_VOLTA - INICIO_DA_VOLTA));
        return 1.0f + (LADO_DA_SAIDA - 1.0f) * afastado * (1.0f - voltando);
    }

    float fimDoFunil = INICIO_DO_FUNIL + COMPRIMENTO_DO_FUNIL;
    if (noPit < FIM_DA_SAIDA_DA_PISTA) {
        return 1.0f + (PIT_ATRAS_DO_FUNIL - 1.0f) * suave((noPit - INICIO_DA_SEPARACAO) / (FIM_DA_SAIDA_DA_PISTA - INICIO_DA_SEPARACAO));
    }
    if (noPit < INICIO_DO_FUNIL) {
        return PIT_ATRAS_DO_FUNIL;
    }
    if (noPit < fimDoFunil) {
        float noFunil = 1.0f - (noPit - INICIO_DO_FUNIL) / COMPRIMENTO_DO_FUNIL;
        return 1.0f + (PIT_ATRAS_DO_FUNIL - 1.0f) * noFunil + (PIT_ATRAS_DO_FUNIL - ABERTURA_DO_FUNIL) * (1.0f - noFunil);
    }
    float encostando = fminf((noPit - fimDoFunil) / METROS_DO_RETAO_DO_PIT, 1.0f);
    return 1.0f + (PIT_ATRAS_DO_FUNIL - ABERTURA_DO_FUNIL) * (1.0f - encostando);
}

/*
 * marcarAsfaltoDoPit
 * Onde o pit lane encosta na pista, é tudo asfalto: quem passa por ali não está na grama.
 * Recebe: a pista.
 * Devolve: nada.
 */
static void marcarAsfaltoDoPit(Pista *pista) {
    Segmento *atual = pista->largada;
    do {
        float metros = atual->indice * COMPRIMENTO_SEGMENTO / UNIDADES_POR_METRO;
        float largura = 0.0f;
        float lado = ladoDentroDoPit(metros, pista->metrosDaVolta, &largura);
        if (lado > 0.0f && lado <= LADO_DA_ILHA && !atual->temMuro) {
            atual->asfaltoEsquerda = lado + largura / atual->larguraEsquerda;
        }
        atual = atual->proximo;
    } while (atual != pista->largada);
}

/*
 * criarPista
 * Monta o Autódromo de Interlagos a partir da tabela TRECHOS_INTERLAGOS,
 * suaviza a entrada e a saída das curvas, acha o lugar de cada segmento no
 * mundo 3D e põe os objetos da beira da pista.
 * Cada trecho em metros vira um bloco de segmentos:
 *   quantidade = metros * SEGMENTOS_POR_METRO
 *   curvatura  = ângulo do trecho / quantidade (graus por segmento)
 *   altura     = soma dos desníveis até ali, em unidades de mundo
 * Recebe: nada.
 * Devolve: a pista completa (o primeiro segmento é a largada).
 */
Pista *criarPista(void) {
    Pista *pista = criarPistaVazia();
    pista->id = PISTA_INTERLAGOS;
    pista->nome = "Interlagos";
    float alturaAtual = 0.0f;   /* a linha de chegada fica na altura 0 */

    for (int i = 0; i < TOTAL_TRECHOS_INTERLAGOS; i++) {
        TrechoReal trecho = TRECHOS_INTERLAGOS[i];

        int quantidade = (int) (trecho.comprimentoMetros * SEGMENTOS_POR_METRO + 0.5f);
        float curvatura = trecho.anguloGraus / quantidade;
        float alturaFinal = alturaAtual + trecho.desnivelMetros * UNIDADES_POR_METRO;

        adicionarTrecho(pista, trecho.nome, quantidade, curvatura, alturaAtual, alturaFinal);
        alturaAtual = alturaFinal;
    }

    suavizarCurvas(pista);
    calcularPosicoesNoMundo(pista);
    murarRetaDosBoxes(pista);
    pista->temPit = 1;
    DadosDoPit pit = { 0 };
    pit.lado = -1;
    pit.entrada = ENTRADA_DO_PIT;
    pit.inicioLivre = 3440.0f;
    pit.fimDaZona = 3600.0f;
    pit.fimDaEntrada = 3730.0f;
    pit.velocidadeNaEntrada = 5000.0f;
    pit.aviso = 3150.0f;
    pit.inicioDoLimite = 3990.0f;
    pit.fimDoLimite = 160.0f;
    pit.fim = FIM_DO_PIT_LANE;
    pit.primeiraVaga = PRIMEIRO_METRO_DOS_BOXES + 5.0f;
    pit.entreVagas = 20.0f;
    pit.encostar = 14.0f;
    pista->pit = pit;
    pista->metrosDaVolta = contarSegmentos(pista) * COMPRIMENTO_SEGMENTO / UNIDADES_POR_METRO;
    marcarAsfaltoDoPit(pista);
    decorarPista(pista);   /* árvores, pórtico, placas... (depois: as zebras dependem das curvas) */
    return pista;
}

/*
 * catmullRom
 * Curva suave que passa pelos pontos b e c da tabela (a e d só dão a direção).
 * Recebe: os quatro valores e a fração entre b e c (0 a 1).
 * Devolve: o valor interpolado.
 */
static float catmullRom(float a, float b, float c, float d, float t) {
    return 0.5f * (2.0f * b + (c - a) * t + (2.0f * a - 5.0f * b + 4.0f * c - d) * t * t
                   + (3.0f * b - a - 3.0f * c + d) * t * t * t);
}

/*
 * pontoDaTabela
 * Recebe: um índice qualquer (a tabela é circular).
 * Devolve: o ponto do traçado da Copa Pistão.
 */
static PontoDoTracado pontoDaTabela(const PontoDoTracado *tabela, int total, int indice) {
    return tabela[(indice + total) % total];
}

/*
 * criarPistaDaTabela
 * Monta a pista a partir do traçado medido no modelo 3D: anda pela tabela criando
 * um segmento a cada COMPRIMENTO_SEGMENTO, já na posição exata do modelo, e deduz a
 * curvatura de cada um da mudança de rumo. Assim a física anda sobre a mesma linha
 * que a imagem mostra.
 * Recebe: a tabela, os limites de cada lado (NULL: muro na borda do asfalto), quantos pontos
 *         ela tem, a escala do modelo, qual pista é e o nome.
 * Devolve: a pista completa.
 */
static Pista *criarPistaDaTabela(const PontoDoTracado *tabela, const float (*limites)[2], int totalDePontos, float escala,
                                 IdPista id, const char *nome) {
    Pista *pista = criarPistaVazia();
    pista->id = id;
    pista->nome = nome;
    float passo = COMPRIMENTO_SEGMENTO / UNIDADES_POR_METRO;

    /* 1) Anda pela tabela em passos curtos, medindo o caminho em metros */
    float ultimoX = pontoDaTabela(tabela, totalDePontos, 0).x * escala, ultimoZ = pontoDaTabela(tabela, totalDePontos, 0).z * escala;
    float andou = passo;   /* força um segmento logo no primeiro ponto */
    for (int i = 0; i < totalDePontos; i++) {
        PontoDoTracado a = pontoDaTabela(tabela, totalDePontos, i - 1), b = pontoDaTabela(tabela, totalDePontos, i),
                       c = pontoDaTabela(tabela, totalDePontos, i + 1), d = pontoDaTabela(tabela, totalDePontos, i + 2);
        for (int sub = 0; sub < SUBPASSOS_DO_TRACADO; sub++) {
            float t = (float) sub / SUBPASSOS_DO_TRACADO;
            float x = catmullRom(a.x, b.x, c.x, d.x, t) * escala;
            float z = catmullRom(a.z, b.z, c.z, d.z, t) * escala;
            andou += hypotf(x - ultimoX, z - ultimoZ);
            ultimoX = x;
            ultimoZ = z;
            if (andou < passo) {
                continue;
            }
            andou -= passo;
            Segmento *novo = inserirSegmento(pista, pista->largada == NULL ? NULL : pista->largada->anterior, 0.0f);
            novo->temMuro = (limites == NULL);
            if (limites != NULL) {
                const float *l0 = limites[i], *l1 = limites[(i + 1) % totalDePontos];
                novo->limiteDireita = (l0[0] + (l1[0] - l0[0]) * t) * escala;
                novo->limiteEsquerda = (l0[1] + (l1[1] - l0[1]) * t) * escala;
            }
            novo->xMundo = x;
            novo->zMundo = z;
            novo->altura = (b.altura + (c.altura - b.altura) * t) * escala * UNIDADES_POR_METRO;
            for (int k = 0; k < PONTOS_DO_PERFIL; k++) {
                novo->perfil[k] = (b.perfil[k] + (c.perfil[k] - b.perfil[k]) * t) * escala;
            }
            novo->larguraDireita = (b.ateMuroDireita + (c.ateMuroDireita - b.ateMuroDireita) * t) * escala;
            novo->larguraEsquerda = (b.ateMuroEsquerda + (c.ateMuroEsquerda - b.ateMuroEsquerda) * t) * escala;
        }
    }

    /* 2) Rumo de cada segmento (até o próximo) e curvatura (quanto o rumo muda) */
    Segmento *atual = pista->largada;
    do {
        atual->rumo = atan2f(atual->proximo->xMundo - atual->xMundo,
                             atual->zMundo - atual->proximo->zMundo) / GRAUS_PARA_RADIANOS;
        atual = atual->proximo;
    } while (atual != pista->largada);
    do {
        float mudou = atual->proximo->rumo - atual->rumo;
        while (mudou > 180.0f) {
            mudou -= 360.0f;
        }
        while (mudou < -180.0f) {
            mudou += 360.0f;
        }
        atual->curvatura = mudou;
        atual = atual->proximo;
    } while (atual != pista->largada);
    suavizarCurvas(pista);

    /* 3) Nome de cada trecho: conta as curvas, na ordem. A curva é decidida pela
     * média em volta, para uma variação pequena não partir uma curva em duas */
    static const char *CURVAS[] = { "Curva 1", "Curva 2", "Curva 3", "Curva 4", "Curva 5", "Curva 6", "Curva 7", "Curva 8",
                                    "Curva 9", "Curva 10", "Curva 11", "Curva 12", "Curva 13", "Curva 14", "Curva 15", "Curva 16",
                                    "Curva 17", "Curva 18", "Curva 19", "Curva 20", "Curva 21", "Curva 22", "Curva 23", "Curva 24" };
    static const char *RETAS_DO_ESTADIO[] = { "Reta dos Boxes", "Reta Curta", "Reta Oposta", "Reta Curta", "Reta dos Boxes" };
    int maiorCurva = (int) (sizeof(CURVAS) / sizeof(CURVAS[0])) - 1;
    int curvasFeitas = 0, naCurva = 0;
    do {
        float soma = 0.0f;
        Segmento *vizinho = atual;
        for (int i = 0; i < RAIO_DA_MEDIA_DO_NOME; i++) {
            vizinho = vizinho->anterior;
        }
        for (int i = 0; i < 2 * RAIO_DA_MEDIA_DO_NOME + 1; i++) {
            soma += vizinho->curvatura;
            vizinho = vizinho->proximo;
        }
        int ehCurva = fabsf(soma) / (2 * RAIO_DA_MEDIA_DO_NOME + 1) >= CURVATURA_MINIMA_ZEBRA;
        if (naCurva && !ehCurva) {
            curvasFeitas++;
        }
        naCurva = ehCurva;
        int qual = (curvasFeitas < maiorCurva) ? curvasFeitas : maiorCurva;
        atual->nomeTrecho = ehCurva ? CURVAS[qual]
                                    : ((id == PISTA_COPA_PISTAO && qual < 5) ? RETAS_DO_ESTADIO[qual] : "Reta");
        atual = atual->proximo;
    } while (atual != pista->largada);

    decorarOval(pista);
    pista->metrosDaVolta = contarSegmentos(pista) * COMPRIMENTO_SEGMENTO / UNIDADES_POR_METRO;
    return pista;
}

/*
 * criarCopaPistao
 * O estádio: o pit lane é o do próprio modelo, atrás do muro de pit da reta da largada,
 * com as equipes montadas ao ar livre no pátio logo depois dos boxes.
 * Recebe: nada.
 * Devolve: a pista completa.
 */
Pista *criarCopaPistao(void) {
    Pista *pista = criarPistaDaTabela(TRACADO_COPA_PISTAO, NULL, TOTAL_PONTOS_COPA_PISTAO, ESCALA_COPA_PISTAO,
                                      PISTA_COPA_PISTAO, "Copa Pistão");
    float volta = pista->metrosDaVolta;
    /* O pit lane do estádio é o corredor entre o muro de pit (a uns 10 m da pista, de 325 m antes
     * da linha até 325 m depois) e o muro do pátio (a uns 40 m). Entra e sai pelas pontas abertas
     * do muro de pit, sem atravessar nenhum dos dois */
    DadosDoPit pit = { 0 };
    pit.lado = -1;
    pit.noModelo = 1;
    pit.entrada = volta - 500.0f;
    pit.inicioLivre = pit.entrada;
    pit.fimDaZona = volta - 338.0f;       /* até perto da abertura do corredor, onde a pista se afunila */
    pit.fimDaEntrada = volta - 335.0f;
    pit.velocidadeNaEntrada = 3600.0f;
    pit.aviso = volta - 720.0f;
    pit.inicioDoLimite = volta - 350.0f;
    pit.fimDoLimite = 330.0f;            /* o limite acaba no fim do muro de pit */
    pit.fim = 440.0f;
    pit.primeiraVaga = -260.0f;          /* todas antes da linha: quem para na última volta ainda cruza */
    pit.entreVagas = 18.0f;
    pit.encostar = 12.0f;
    pit.noCaminhoDaFaixa = volta - 360.0f;   /* já no corredor antes do muro de pit começar */
    pit.deixaAFaixa = 330.0f;                /* só sai do corredor depois que o muro de pit acaba */
    pit.borda = 8.0f;
    pit.faixa = 22.0f;
    pit.vaga = 32.0f;                    /* encostado no muro do pátio, que fica a uns 40 m */
    pit.paraEntrar = 12.0f;
    pista->pit = pit;
    pista->temPit = 1;
    return pista;
}

/*
 * criarOval
 * O pit lane do modelo fica por fora da reta da largada, entre o muro dos boxes e as
 * garagens: abre em x = -125 e fecha em x = 140 (metros aqui a partir da linha, x = 15,7).
 * Recebe: nada.
 * Devolve: a pista completa.
 */
Pista *criarOval(void) {
    Pista *pista = criarPistaDaTabela(TRACADO_OVAL, NULL, TOTAL_PONTOS_OVAL, ESCALA_OVAL, PISTA_OVAL, "Oval");
    float volta = pista->metrosDaVolta;
    DadosDoPit pit = { 0 };
    pit.lado = 1;
    pit.noModelo = 1;
    pit.entrada = volta - 280.0f;
    pit.inicioLivre = pit.entrada;
    pit.fimDaZona = volta - 225.0f;
    pit.fimDaEntrada = volta - 205.0f;
    pit.velocidadeNaEntrada = 5000.0f;
    pit.aviso = volta - 500.0f;
    pit.inicioDoLimite = volta - 185.0f;
    pit.fimDoLimite = 60.0f;
    pit.fim = 248.0f;
    pit.primeiraVaga = -160.0f;
    pit.entreVagas = 11.0f;
    pit.encostar = 9.0f;
    pit.noCaminhoDaFaixa = volta - 205.0f;
    pit.deixaAFaixa = 90.0f;
    pit.borda = 12.0f;
    pit.faixa = 27.0f;
    pit.vaga = 35.0f;
    pit.paraEntrar = 13.0f;
    pista->pit = pit;
    pista->temPit = 1;
    return pista;
}

/* A estrada do pit do Splat, medida no modelo: passa pela abertura entre os dois muros da
 * ilha que a separa da pista (uns 220 m antes da linha), sai pela ponte sobre a estrada de baixo
 * e só volta depois do fim do muro da saída */
static const float CAMINHO_DO_PIT_DO_SPLAT[][2] = {
    { -240.0f, 8.0f }, { -228.0f, 10.0f }, { -220.0f, 14.5f }, { -210.0f, 19.0f }, { -200.0f, 21.0f }, { -190.0f, 26.0f },
    { -180.0f, 35.0f }, { -170.0f, 42.0f }, { -160.0f, 44.0f }, { -8.0f, 44.0f }, { -2.0f, 42.0f }, { 3.0f, 38.0f },
    { 8.0f, 33.0f }, { 12.0f, 29.0f }, { 16.0f, 26.0f }, { 20.0f, 23.0f }, { 24.0f, 21.0f }, { 28.0f, 20.0f }, { 40.0f, 19.0f },
    { 48.0f, 17.0f }, { 52.0f, 14.0f }, { 60.0f, 9.0f }, { 70.0f, 8.0f }
};

/*
 * criarSplat
 * A pista é corrida no sentido em que o grid fica antes da linha, no começo da reta.
 * O pit lane é a estrada paralela à reta, à direita, na frente do prédio das garagens.
 * Recebe: nada.
 * Devolve: a pista completa.
 */
Pista *criarSplat(void) {
    Pista *pista = criarPistaDaTabela(TRACADO_SPLAT, LIMITES_SPLAT, TOTAL_PONTOS_SPLAT, ESCALA_SPLAT, PISTA_SPLAT, "Splat");
    pista->semDirecaoGuiada = 1;   /* nas curvas fechadas dele, a guiada deixa o carro parecer preso a um traçado */
    float volta = pista->metrosDaVolta;
    DadosDoPit pit = { 0 };
    pit.lado = 1;
    pit.noModelo = 1;
    pit.entrada = volta - 240.0f;
    pit.inicioLivre = pit.entrada;
    pit.fimDaZona = volta - 225.0f;
    pit.fimDaEntrada = volta - 172.0f;
    pit.velocidadeNaEntrada = 3200.0f;   /* a estrada dobra logo depois de sair da pista */
    pit.aviso = volta - 440.0f;
    pit.inicioDoLimite = volta - 172.0f;
    pit.fimDoLimite = 62.0f;             /* e dobra de novo para voltar: só dá para fazer devagar */
    pit.fim = 72.0f;
    pit.primeiraVaga = -165.0f;
    pit.entreVagas = 11.0f;
    pit.encostar = 9.0f;
    pit.noCaminhoDaFaixa = volta - 160.0f;
    pit.deixaAFaixa = volta - 10.0f;
    pit.borda = 8.0f;
    pit.faixa = 44.0f;
    pit.vaga = 48.5f;
    pit.paraEntrar = 9.0f;
    pit.caminho = CAMINHO_DO_PIT_DO_SPLAT;
    pit.pontosDoCaminho = (int) (sizeof(CAMINHO_DO_PIT_DO_SPLAT) / sizeof(CAMINHO_DO_PIT_DO_SPLAT[0]));
    pista->pit = pit;
    pista->temPit = 1;
    return pista;
}

/*
 * metrosDesdeAEntrada
 * Recebe: a pista e um lugar, em metros a partir da linha (pode ser negativo).
 * Devolve: quantos metros esse lugar está depois da entrada do pit lane, dando a volta na linha.
 */
float metrosDesdeAEntrada(const Pista *pista, float metros) {
    float depois = fmodf(metros - pista->pit.entrada, pista->metrosDaVolta);
    return (depois < 0.0f) ? depois + pista->metrosDaVolta : depois;
}

/*
 * criarPistaPorId
 * Recebe: qual pista criar.
 * Devolve: a pista completa.
 */
Pista *criarPistaPorId(IdPista id) {
    switch (id) {
        case PISTA_COPA_PISTAO: return criarCopaPistao();
        case PISTA_OVAL: return criarOval();
        case PISTA_SPLAT: return criarSplat();
        default: return criarPista();
    }
}

/* ==================================================================
 * Funções sobre a lista circular
 * ================================================================== */

/*
 * inserirSegmento
 * Cria um segmento e o encaixa logo depois de "depoisDe".
 *   - Se a pista estiver vazia, o novo segmento vira a largada e
 *     aponta para ele mesmo.
 *   - Se "depoisDe" for NULL, o segmento entra no fim da pista
 *     (entre o último segmento e a largada).
 * Recebe: a pista, o segmento de referência (ou NULL) e a curvatura.
 * Devolve: o segmento criado.
 */
Segmento *inserirSegmento(Pista *pista, Segmento *depoisDe, float curvatura) {
    Segmento *novo = criarSegmento(curvatura);

    /* Caso 1: pista vazia */
    if (pista->largada == NULL) {
        novo->ehLinhaDeChegada = 1;
        pista->largada = novo;
        return novo;   /* já aponta para ele mesmo; índice 0 */
    }

    /* Caso 2: NULL significa "no fim", ou seja, depois do último */
    if (depoisDe == NULL) {
        depoisDe = pista->largada->anterior;
    }

    /* Caso 3: encaixar entre depoisDe e o seguinte dele.
     *   antes:  depoisDe <-> seguinte
     *   depois: depoisDe <-> novo <-> seguinte                */
    Segmento *seguinte = depoisDe->proximo;

    novo->anterior = depoisDe;
    novo->proximo = seguinte;
    depoisDe->proximo = novo;
    seguinte->anterior = novo;

    renumerarSegmentos(pista);
    return novo;
}

/*
 * removerSegmento
 * Tira um segmento da pista e religa os vizinhos dele.
 * Como a lista é dupla, o segmento já sabe quem é o anterior,
 * então não é preciso percorrer a pista para achá-lo.
 *   - Se for o único segmento, a pista fica vazia.
 *   - Se for a largada, o segmento seguinte vira a nova largada.
 * Recebe: a pista e o segmento a remover (que deve estar nela).
 * Devolve: 1 se removeu, 0 se não havia o que remover.
 */
int removerSegmento(Pista *pista, Segmento *segmento) {
    if (pista == NULL || pista->largada == NULL || segmento == NULL) {
        return 0;
    }

    /* Caso 1: único segmento (ele é o próprio vizinho) */
    if (segmento->proximo == segmento) {
        pista->largada = NULL;
        liberarObstaculos(segmento);
        free(segmento);
        return 1;
    }

    /* Caso 2: removendo a largada, a linha de chegada passa adiante */
    if (segmento == pista->largada) {
        pista->largada = segmento->proximo;
        pista->largada->ehLinhaDeChegada = 1;
    }

    /* Religa os vizinhos, pulando o segmento removido:
     *   antes:  anterior <-> segmento <-> proximo
     *   depois: anterior <-> proximo                          */
    segmento->anterior->proximo = segmento->proximo;
    segmento->proximo->anterior = segmento->anterior;

    liberarObstaculos(segmento);
    free(segmento);

    renumerarSegmentos(pista);
    return 1;
}

/*
 * buscarSegmento
 * Procura o segmento com o índice pedido, andando a partir da largada.
 * Recebe: a pista e o índice procurado.
 * Devolve: o segmento, ou NULL se o índice não existir.
 */
Segmento *buscarSegmento(Pista *pista, int indice) {
    if (pista == NULL || pista->largada == NULL || indice < 0) {
        return NULL;
    }

    Segmento *atual = pista->largada;
    do {
        if (atual->indice == indice) {
            return atual;
        }
        atual = atual->proximo;
    } while (atual != pista->largada);

    return NULL;   /* deu a volta inteira e não achou */
}

/*
 * proximoSegmento
 * Anda um segmento para a frente. Depois do último vem a largada,
 * e é assim que o carro "dá a volta" no circuito.
 * Recebe: o segmento atual.
 * Devolve: o segmento seguinte.
 */
Segmento *proximoSegmento(Segmento *segmento) {
    return segmento->proximo;
}

/*
 * segmentoAnterior
 * Anda um segmento para trás. Antes da largada vem o último segmento.
 * Recebe: o segmento atual.
 * Devolve: o segmento anterior.
 */
Segmento *segmentoAnterior(Segmento *segmento) {
    return segmento->anterior;
}

/*
 * contarSegmentos
 * Conta quantos segmentos a pista tem, dando uma volta completa.
 * Recebe: a pista.
 * Devolve: o número de segmentos (0 se estiver vazia).
 */
int contarSegmentos(Pista *pista) {
    if (pista == NULL || pista->largada == NULL) {
        return 0;
    }

    int total = 0;
    Segmento *atual = pista->largada;
    do {
        total++;
        atual = atual->proximo;
    } while (atual != pista->largada);

    return total;
}

/*
 * temZebra
 * Diz se o segmento é parte de uma curva, onde a pista tem zebra.
 * Recebe: o segmento.
 * Devolve: 1 se tem zebra, 0 se é reta (ou quase).
 */
int temZebra(Segmento *segmento) {
    if (segmento->temMuro) {
        return 0;   /* no oval, a borda é o muro */
    }
    return segmento->curvatura >= CURVATURA_MINIMA_ZEBRA ||
           segmento->curvatura <= -CURVATURA_MINIMA_ZEBRA;
}

/*
 * distanciaDoLugarAteALinha
 * Quanto um lugar do grid fica ANTES da linha de chegada.
 * Recebe: o lugar (0 = pole).
 * Devolve: a distância em unidades de mundo (sempre múltipla do segmento).
 */
float distanciaDoLugarAteALinha(int lugar) {
    return DISTANCIA_DA_POLE + lugar * ESPACO_ENTRE_LUGARES;
}

/*
 * lateralDoLugar
 * Recebe: o lugar do grid (0 = pole).
 * Devolve: o deslocamento lateral dele (pares à esquerda, ímpares à direita).
 */
float lateralDoLugar(int lugar) {
    return (lugar % 2 == 0) ? -LATERAL_DO_GRID : LATERAL_DO_GRID;
}

/*
 * moverNaPista
 * Anda uma distância para a frente (positiva) ou para trás (negativa).
 * Toda vez que a distância passa do fim do segmento, a posição pula
 * para o próximo nó da lista; se fica negativa, volta para o anterior.
 * Como a lista é circular, nunca existe "fim da pista".
 *
 * Também conta as passagens pela linha de chegada:
 *   - entrar na largada andando para a frente soma +1;
 *   - sair da largada andando para trás soma -1 (assim dar ré
 *     sobre a linha e voltar não ganha uma volta de graça).
 *
 * Recebe: a posição (é alterada) e a distância a andar.
 * Devolve: quantas voltas foram ganhas (ou perdidas) neste movimento.
 */
int moverNaPista(PosicaoNaPista *posicao, float distancia) {
    int voltas = 0;
    posicao->distanciaNoSegmento += distancia;

    /* Para a frente: passou do fim do segmento atual */
    while (posicao->distanciaNoSegmento >= COMPRIMENTO_SEGMENTO) {
        posicao->distanciaNoSegmento -= COMPRIMENTO_SEGMENTO;
        posicao->segmento = proximoSegmento(posicao->segmento);
        if (posicao->segmento->ehLinhaDeChegada) {
            voltas++;
        }
    }

    /* Para trás: voltou antes do início do segmento atual */
    while (posicao->distanciaNoSegmento < 0) {
        if (posicao->segmento->ehLinhaDeChegada) {
            voltas--;
        }
        posicao->distanciaNoSegmento += COMPRIMENTO_SEGMENTO;
        posicao->segmento = segmentoAnterior(posicao->segmento);
    }

    return voltas;
}

/*
 * alturaNaPosicao
 * Altura do chão num ponto exato da pista: mistura a altura do começo do
 * segmento com a do começo do próximo, conforme o quanto já se andou nele.
 * Recebe: a posição na pista.
 * Devolve: a altura em unidades de mundo.
 */
float alturaNaPosicao(PosicaoNaPista posicao) {
    float alturaAqui = posicao.segmento->altura;
    float alturaSeguinte = proximoSegmento(posicao.segmento)->altura;
    float fracao = posicao.distanciaNoSegmento / COMPRIMENTO_SEGMENTO;
    return alturaAqui + (alturaSeguinte - alturaAqui) * fracao;
}

/*
 * liberarPista
 * Devolve toda a memória da pista.
 * Para não andar em círculos para sempre, primeiro "corta" o círculo:
 * o último segmento passa a apontar para NULL. Aí a lista vira uma
 * lista comum, e basta liberar até encontrar NULL.
 * Recebe: a pista.
 * Devolve: nada (o ponteiro da pista não pode mais ser usado).
 */
void liberarPista(Pista *pista) {
    if (pista == NULL) {
        return;
    }

    if (pista->largada != NULL) {
        Segmento *ultimo = pista->largada->anterior;
        ultimo->proximo = NULL;   /* corta o círculo */

        Segmento *atual = pista->largada;
        while (atual != NULL) {
            Segmento *seguinte = atual->proximo;   /* guarda antes de liberar */
            liberarObstaculos(atual);
            free(atual);
            atual = seguinte;
        }
    }

    free(pista);
}

/*
 * larguraDoLado
 * A pista pode ser mais larga de um lado que do outro (o oval).
 * Recebe: o segmento e a lateral (só importa o sinal).
 * Devolve: a largura, em metros, do lado onde a lateral está.
 */
float larguraDoLado(Segmento *segmento, float lateral) {
    return (lateral >= 0.0f) ? segmento->larguraDireita : segmento->larguraEsquerda;
}

/*
 * perfilNaLateral
 * A altura do chão num lugar da largura, interpolando o perfil do segmento.
 * Além da borda, vale a altura da borda.
 * Recebe: o segmento e a lateral (-1 borda esquerda, +1 borda direita).
 * Devolve: a altura em relação à linha da pista, em metros.
 */
static float perfilNaLateral(Segmento *segmento, float lateral) {
    float lugar = (lateral + 1.0f) / 2.0f * (PONTOS_DO_PERFIL - 1);
    if (lugar <= 0.0f) {
        return segmento->perfil[0];
    }
    if (lugar >= PONTOS_DO_PERFIL - 1) {
        return segmento->perfil[PONTOS_DO_PERFIL - 1];
    }
    int k = (int) lugar;
    return segmento->perfil[k] + (segmento->perfil[k + 1] - segmento->perfil[k]) * (lugar - k);
}

/*
 * alturaNaLateral
 * A altura do chão numa posição da pista e num lugar da largura.
 * Recebe: a posição na pista e a lateral (-1 a +1).
 * Devolve: a altura no mundo, em metros.
 */
static float alturaNaLateral(PosicaoNaPista posicao, float lateral) {
    Segmento *a = posicao.segmento;
    float t = posicao.distanciaNoSegmento / COMPRIMENTO_SEGMENTO;
    float perfil = perfilNaLateral(a, lateral) + (perfilNaLateral(a->proximo, lateral) - perfilNaLateral(a, lateral)) * t;
    return alturaNaPosicao(posicao) / UNIDADES_POR_METRO + perfil;
}

/*
 * alturaDoChao
 * A altura do chão a tantos metros da linha da pista. É o que assenta as
 * rodas dos carros no asfalto.
 * Recebe: a posição na pista e a lateral em metros (+ direita, - esquerda).
 * Devolve: a altura no mundo, em metros.
 */
float alturaDoChao(PosicaoNaPista posicao, float lateralEmMetros) {
    return alturaNaLateral(posicao, lateralEmMetros / larguraDoLado(posicao.segmento, lateralEmMetros));
}

/*
 * pontoNoMundo
 * Onde um ponto da pista fica no mundo 3D: interpola entre o começo do
 * segmento e o começo do seguinte, anda para o lado e sobe até o chão.
 * Recebe: a posição na pista e o deslocamento lateral (0 = linha da pista, +1 = borda direita, -1 = esquerda).
 * Devolve: o ponto, em metros.
 */
PontoNoMundo pontoNoMundo(PosicaoNaPista posicao, float deslocamentoLateral) {
    Segmento *a = posicao.segmento;
    Segmento *b = a->proximo;
    float t = posicao.distanciaNoSegmento / COMPRIMENTO_SEGMENTO;

    float mudou = b->rumo - a->rumo;
    while (mudou > 180.0f) {
        mudou -= 360.0f;
    }
    while (mudou < -180.0f) {
        mudou += 360.0f;
    }

    PontoNoMundo ponto;
    ponto.rumo = a->rumo + mudou * t;
    float largura = larguraDoLado(a, deslocamentoLateral) + (larguraDoLado(b, deslocamentoLateral) - larguraDoLado(a, deslocamentoLateral)) * t;
    float lateral = deslocamentoLateral * largura;
    float angulo = ponto.rumo * GRAUS_PARA_RADIANOS;

    /* A direita de quem olha para o rumo */
    ponto.x = a->xMundo + (b->xMundo - a->xMundo) * t + cosf(angulo) * lateral;
    ponto.z = a->zMundo + (b->zMundo - a->zMundo) * t + sinf(angulo) * lateral;
    ponto.y = alturaNaLateral(posicao, deslocamentoLateral);

    /* A inclinação em volta do ponto, para a câmera acompanhar sem tremer */
    float direita = alturaDoChao(posicao, lateral + META_LARGURA_DA_INCLINACAO);
    float esquerda = alturaDoChao(posicao, lateral - META_LARGURA_DA_INCLINACAO);
    ponto.inclinacao = (direita - esquerda) / (2.0f * META_LARGURA_DA_INCLINACAO);
    return ponto;
}
