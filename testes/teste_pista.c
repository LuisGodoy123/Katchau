/*
 * teste_pista.c
 * ------------------------------------------------------------------
 * Testa no terminal, sem gráficos, a lista circular da pista e a
 * lógica que anda sobre ela (carro e corrida).
 *
 * Como rodar (na raiz do projeto):
 *     make testes                -> roda todos os testes
 *     ./build/teste_pista -v     -> imprime também cada segmento da volta 1
 *     make memoria               -> roda no valgrind (Linux/WSL)
 *
 * Cada verificação imprime [OK] ou [FALHOU]. No fim aparece o total.
 * ------------------------------------------------------------------
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "pista.h"
#include "carro.h"
#include "corrida.h"
#include "ordenacao.h"
#include "ia.h"

#define TOTAL_VOLTAS          3
#define GRAUS_VOLTA_COMPLETA  -360.0f  /* Interlagos é anti-horário: vira 360 graus para a esquerda */
#define TOLERANCIA            0.01f
#define TOLERANCIA_GRAUS      0.05f    /* erro de arredondamento ao somar ~2600 floats */
#define COMPRIMENTO_INTERLAGOS 4309.0f /* metros */
#define DESNIVEL_INTERLAGOS     56.0f  /* metros, do S do Senna à Descida do Lago */
#define PASSOS_PARA_TRAS      5

static int totalFalhas = 0;

/*
 * verificar
 * Imprime o resultado de uma verificação e conta as falhas.
 * Recebe: a condição (1 = passou) e a descrição do que foi testado.
 * Devolve: nada.
 */
static void verificar(int condicao, const char *descricao) {
    if (condicao) {
        printf("  [OK]     %s\n", descricao);
    } else {
        printf("  [FALHOU] %s\n", descricao);
        totalFalhas++;
    }
}

/*
 * descreverCurvatura
 * Traduz a curvatura em texto para facilitar a leitura.
 * Recebe: a curvatura.
 * Devolve: "reta", "curva a direita" ou "curva a esquerda".
 */
static const char *descreverCurvatura(float curvatura) {
    if (curvatura > 0) {
        return "curva a direita";
    }
    if (curvatura < 0) {
        return "curva a esquerda";
    }
    return "reta";
}

/*
 * estaNoTrecho
 * Diz se o segmento, ou o anterior a ele, pertence ao trecho pedido.
 * Recebe: o segmento e o nome do trecho.
 * Devolve: 1 se pertence, 0 se não.
 */
static int estaNoTrecho(Segmento *segmento, const char *nome) {
    return strcmp(segmento->nomeTrecho, nome) == 0 ||
           strcmp(segmentoAnterior(segmento)->nomeTrecho, nome) == 0;
}

/* ------------------------------------------------------------------ */

static void testarPistaVazia(void) {
    printf("\n1) Pista vazia\n");
    Pista *pista = criarPistaVazia();

    verificar(contarSegmentos(pista) == 0, "pista vazia tem 0 segmentos");
    verificar(buscarSegmento(pista, 0) == NULL, "buscar na pista vazia devolve NULL");
    verificar(removerSegmento(pista, NULL) == 0, "remover da pista vazia devolve 0");

    liberarPista(pista);
}

static void testarInserirERemover(void) {
    printf("\n2) Inserir e remover numa pista pequena\n");
    Pista *pista = criarPistaVazia();

    Segmento *a = inserirSegmento(pista, NULL, RETA);
    verificar(pista->largada == a, "primeiro segmento vira a largada");
    verificar(a->proximo == a && a->anterior == a, "sozinho, ele aponta para ele mesmo");
    verificar(a->ehLinhaDeChegada == 1, "a largada e a linha de chegada");

    Segmento *b = inserirSegmento(pista, NULL, CURVA_ABERTA);
    Segmento *c = inserirSegmento(pista, NULL, -CURVA_ABERTA);
    verificar(contarSegmentos(pista) == 3, "3 segmentos depois de 3 insercoes");
    verificar(c->proximo == a, "ultimo aponta para a largada (circular)");
    verificar(a->anterior == c, "largada aponta para tras para o ultimo");
    verificar(a->indice == 0 && b->indice == 1 && c->indice == 2, "indices 0, 1, 2");

    Segmento *meio = inserirSegmento(pista, a, CURVA_FECHADA);
    verificar(a->proximo == meio && meio->proximo == b, "insercao no meio religa os vizinhos");
    verificar(b->indice == 2 && c->indice == 3, "indices renumerados depois da insercao");

    verificar(removerSegmento(pista, a) == 1, "remover a largada");
    verificar(pista->largada == meio, "o seguinte vira a nova largada");
    verificar(meio->ehLinhaDeChegada == 1, "a linha de chegada passa adiante");
    verificar(c->proximo == meio && meio->anterior == c, "circulo continua fechado");
    verificar(contarSegmentos(pista) == 3, "sobram 3 segmentos");

    removerSegmento(pista, b);
    removerSegmento(pista, c);
    verificar(contarSegmentos(pista) == 1, "sobra 1 segmento");
    verificar(removerSegmento(pista, meio) == 1, "remover o unico segmento");
    verificar(pista->largada == NULL, "pista fica vazia (largada = NULL)");

    liberarPista(pista);
}

static void testarAutodromo(int modoDetalhado) {
    printf("\n3) Autodromo completo: Interlagos (criarPista)\n");
    Pista *pista = criarPista();
    int total = contarSegmentos(pista);
    float comprimentoMetros = total / SEGMENTOS_POR_METRO;
    printf("     total de segmentos: %d (%.0f m)\n", total, comprimentoMetros);
    verificar(fabsf(comprimentoMetros - COMPRIMENTO_INTERLAGOS) < 5.0f, "a volta tem ~4.309 m");

    /* Soma das curvaturas de uma volta: deve dar -360 graus.
     * Também guarda o ponto mais alto e o mais baixo. */
    float somaCurvaturas = 0.0f;
    Segmento *maisAlto = pista->largada;
    Segmento *maisBaixo = pista->largada;
    Segmento *atual = pista->largada;
    do {
        somaCurvaturas += atual->curvatura;
        if (atual->altura > maisAlto->altura) {
            maisAlto = atual;
        }
        if (atual->altura < maisBaixo->altura) {
            maisBaixo = atual;
        }
        atual = proximoSegmento(atual);
    } while (atual != pista->largada);
    printf("     soma das curvaturas: %.2f graus\n", somaCurvaturas);
    verificar(fabsf(somaCurvaturas - GRAUS_VOLTA_COMPLETA) < TOLERANCIA_GRAUS,
              "uma volta vira 360 graus para a esquerda (anti-horario)");

    float desnivelMetros = (maisAlto->altura - maisBaixo->altura) / UNIDADES_POR_METRO;
    printf("     mais alto: segmento %d (%s)  mais baixo: segmento %d (%s)  desnivel: %.1f m\n",
           maisAlto->indice, maisAlto->nomeTrecho, maisBaixo->indice, maisBaixo->nomeTrecho,
           desnivelMetros);
    verificar(fabsf(desnivelMetros - DESNIVEL_INTERLAGOS) < 0.5f, "desnivel de ~56 m");
    /* A altura é a do começo do segmento, então o pico pode cair no
     * primeiro segmento depois do trecho: olha o segmento e o anterior. */
    verificar(estaNoTrecho(maisAlto, "S do Senna"), "ponto mais alto no S do Senna");
    verificar(estaNoTrecho(maisBaixo, "Descida do Lago"), "ponto mais baixo na Descida do Lago");
    verificar(fabsf(pista->largada->anterior->altura) < 0.5f * UNIDADES_POR_METRO,
              "a volta termina na mesma altura da largada");

    verificar(buscarSegmento(pista, 0) == pista->largada, "buscar(0) devolve a largada");
    verificar(buscarSegmento(pista, total - 1) == pista->largada->anterior,
              "buscar(ultimo) devolve o anterior da largada");
    verificar(buscarSegmento(pista, total) == NULL, "buscar indice inexistente devolve NULL");

    /* Imprime os trechos: agrupa segmentos seguidos com a mesma curvatura */
    printf("\n     Trechos da pista:\n");
    Segmento *inicioTrecho = pista->largada;
    atual = pista->largada;
    do {
        if (modoDetalhado) {
            printf("       segmento %4d | curvatura %6.3f | altura %6.1f m | %s%s\n",
                   atual->indice, atual->curvatura, atual->altura / UNIDADES_POR_METRO,
                   descreverCurvatura(atual->curvatura),
                   atual->ehLinhaDeChegada ? "  <- LARGADA" : "");
        }
        Segmento *seguinte = proximoSegmento(atual);
        int trechoAcabou = (seguinte == pista->largada) ||
                           (seguinte->curvatura != atual->curvatura);
        if (trechoAcabou) {
            printf("       segmentos %4d a %4d: %-18s %-16s (%6.3f graus/segmento)\n",
                   inicioTrecho->indice, atual->indice, atual->nomeTrecho,
                   descreverCurvatura(atual->curvatura), atual->curvatura);
            inicioTrecho = seguinte;
        }
        atual = seguinte;
    } while (atual != pista->largada);

    /* Simula um carro andando 1 segmento por vez durante 3 voltas */
    printf("\n     Andando %d voltas:\n", TOTAL_VOLTAS);
    int voltas = 0;
    int passos = 0;
    atual = pista->largada;
    while (voltas < TOTAL_VOLTAS) {
        atual = proximoSegmento(atual);
        passos++;
        if (atual->ehLinhaDeChegada) {
            voltas++;
            printf("       volta %d completada (%d segmentos percorridos)\n", voltas, passos);
        }
    }
    verificar(passos == TOTAL_VOLTAS * total, "3 voltas = 3 x total de segmentos");
    verificar(atual == pista->largada, "terminou de volta na largada");

    /* Anda para trás a partir da largada: deve dar a volta pelo fim */
    atual = pista->largada;
    for (int i = 0; i < PASSOS_PARA_TRAS; i++) {
        atual = segmentoAnterior(atual);
    }
    printf("\n     %d passos para tras a partir da largada: segmento %d\n",
           PASSOS_PARA_TRAS, atual->indice);
    verificar(atual->indice == total - PASSOS_PARA_TRAS, "andar para tras atravessa a largada");

    liberarPista(pista);
}

static void testarMoverNaPista(void) {
    printf("\n4) Andar pela pista com moverNaPista\n");
    Pista *pista = criarPista();
    int total = contarSegmentos(pista);
    float voltaCompleta = total * COMPRIMENTO_SEGMENTO;

    PosicaoNaPista posicao = { pista->largada, 0.0f };

    int voltas = moverNaPista(&posicao, COMPRIMENTO_SEGMENTO * 2.5f);
    verificar(voltas == 0 && posicao.segmento->indice == 2, "andar 2,5 segmentos para no segmento 2");
    verificar(fabsf(posicao.distanciaNoSegmento - COMPRIMENTO_SEGMENTO / 2) < TOLERANCIA,
              "e no meio dele");

    /* Uma volta inteira em passos pequenos, como o jogo faz a cada quadro */
    posicao.segmento = pista->largada;
    posicao.distanciaNoSegmento = 0.0f;
    voltas = 0;
    float passo = 37.0f;
    float andado = 0.0f;
    while (andado + passo <= voltaCompleta) {
        voltas += moverNaPista(&posicao, passo);
        andado += passo;
    }
    voltas += moverNaPista(&posicao, voltaCompleta - andado);
    verificar(voltas == 1, "uma volta em passos pequenos soma 1 volta");
    verificar(posicao.segmento == pista->largada, "e termina na largada");

    voltas = moverNaPista(&posicao, voltaCompleta * 2);
    verificar(voltas == 2, "andar 2 voltas de uma vez soma 2");

    /* Ré sobre a linha de chegada e de volta: não pode ganhar volta */
    voltas = moverNaPista(&posicao, -COMPRIMENTO_SEGMENTO);
    verificar(voltas == -1 && posicao.segmento->indice == total - 1,
              "dar re atravessando a largada desconta 1 volta");
    voltas = moverNaPista(&posicao, COMPRIMENTO_SEGMENTO);
    verificar(voltas == 1, "voltar para a frente devolve a volta (saldo zero)");

    liberarPista(pista);
}

static void testarCarro(void) {
    printf("\n5) Carro andando pela pista\n");
    Pista *pista = criarPista();
    float quadro = 1.0f / 60.0f;
    Comandos acelerar = { 1, 0, 0, 0 };
    Comandos frear = { 0, 1, 0, 0 };
    Comandos virarDireita = { 0, 0, 0, 1 };

    /* Para medir velocidade, um "oval" só de retas: sem curva, nada
     * empurra o carro para a grama. */
    Pista *retaInfinita = criarPistaVazia();
    adicionarTrecho(retaInfinita, "Reta de teste", 50, RETA, 0.0f, 0.0f);

    Carro carro = criarCarro("Teste", 0, retaInfinita->largada, 0.0f, 12000.0f, 2400.0f, 0.5f);
    atualizarCarro(&carro, virarDireita, quadro);
    verificar(carro.deslocamentoLateral == 0.0f, "parado, o carro nao vira");

    for (int i = 0; i < 60 * 10; i++) {
        atualizarCarro(&carro, acelerar, quadro);
    }
    verificar(carro.velocidade == carro.velocidadeMaxima, "acelerando, chega na velocidade maxima e para ali");
    verificar(velocidadeEmKmh(&carro) == 300, "velocidade maxima = 300 km/h");

    for (int i = 0; i < 60 * 2; i++) {
        atualizarCarro(&carro, frear, quadro);
    }
    verificar(carro.velocidade == 0.0f, "freando, para e nao anda de re");

    /* Na grama a velocidade cai para o limite da grama */
    carro.deslocamentoLateral = 2.0f;
    carro.velocidade = carro.velocidadeMaxima;
    for (int i = 0; i < 60 * 3; i++) {
        atualizarCarro(&carro, acelerar, quadro);
    }
    verificar(estaForaDaPista(&carro) && velocidadeEmKmh(&carro) <= 80,
              "na grama, mesmo acelerando, cai para ~75 km/h");

    /* Curva para a direita empurra para a esquerda.
     * Em Interlagos a primeira curva para a direita é o 2o S do Senna. */
    Segmento *curvaDireita = pista->largada;
    while (curvaDireita->curvatura <= 0) {
        curvaDireita = proximoSegmento(curvaDireita);
    }
    carro.posicao.segmento = curvaDireita;
    carro.posicao.distanciaNoSegmento = 0.0f;
    carro.deslocamentoLateral = 0.0f;
    carro.velocidade = carro.velocidadeMaxima;
    atualizarCarro(&carro, acelerar, quadro);
    verificar(carro.posicao.segmento->curvatura > 0 && carro.deslocamentoLateral < 0,
              "curva para a direita empurra o carro para a esquerda");

    liberarPista(retaInfinita);
    liberarPista(pista);
}

static void testarCorridaCompleta(void) {
    printf("\n6) Corrida de %d voltas com um piloto automatico simples\n", TOTAL_VOLTAS);
    Pista *pista = criarPista();
    Corrida corrida;
    iniciarCorrida(&corrida, pista);
    float quadro = 1.0f / 60.0f;
    int quadrosMaximos = 60 * 60 * 10;   /* 10 minutos de jogo */

    int quadros = 0;
    while (!corrida.terminou && quadros < quadrosMaximos) {
        /* Acelera sempre e corrige o volante para o centro */
        Comandos comandos = { 1, 0, 0, 0 };
        comandos.esquerda = corrida.jogador.deslocamentoLateral > 0.1f;
        comandos.direita = corrida.jogador.deslocamentoLateral < -0.1f;
        atualizarCorrida(&corrida, comandos, quadro);
        quadros++;
    }

    float somaVoltas = 0.0f;
    for (int i = 0; i < TOTAL_VOLTAS; i++) {
        printf("     volta %d: %.2f s\n", i + 1, corrida.jogador.temposDeVolta[i]);
        somaVoltas += corrida.jogador.temposDeVolta[i];
    }
    printf("     tempo total: %.2f s\n", corrida.jogador.tempoDeChegada);

    verificar(corrida.terminou, "a corrida termina");
    verificar(corrida.jogador.voltas == TOTAL_VOLTAS, "o jogador completou 3 voltas");
    verificar(fabsf(somaVoltas - corrida.jogador.tempoDeChegada) < 0.1f, "soma das voltas = tempo de chegada");
    verificar(voltaAtual(&corrida.jogador) == TOTAL_VOLTAS, "volta mostrada nunca passa de 3/3");

    float tempoNaChegada = corrida.jogador.tempoDeChegada;
    float velocidadeNaChegada = corrida.jogador.velocidade;
    Comandos acelerar = { 1, 0, 0, 0 };
    atualizarCorrida(&corrida, acelerar, quadro);
    verificar(corrida.jogador.tempoDeChegada == tempoNaChegada, "depois do fim o tempo do jogador nao muda");
    verificar(corrida.jogador.velocidade < velocidadeNaChegada, "depois do fim o acelerador e ignorado");

    liberarPista(pista);
}

/*
 * estaOrdenada
 * Recebe: um vetor de ponteiros para carro e o tamanho.
 * Devolve: 1 se estiver do que andou mais para o que andou menos.
 */
static int estaOrdenada(Carro *carros[], int quantidade) {
    for (int i = 1; i < quantidade; i++) {
        if (carros[i - 1]->distanciaPercorrida < carros[i]->distanciaPercorrida) {
            return 0;
        }
    }
    return 1;
}

static void testarInsertionSort(void) {
    printf("\n7) Insertion Sort (ordenarCarrosPorDistancia)\n");
    Carro carros[TOTAL_CARROS];
    Carro *vetor[TOTAL_CARROS];

    /* Embaralhado */
    float distancias[TOTAL_CARROS] = { 300, 900, 100, 700, 800, 200, 600, 400 };
    for (int i = 0; i < TOTAL_CARROS; i++) {
        carros[i].distanciaPercorrida = distancias[i];
        vetor[i] = &carros[i];
    }
    int deslocamentos = ordenarCarrosPorDistancia(vetor, TOTAL_CARROS);
    printf("     embaralhado: %d deslocamentos -> ", deslocamentos);
    for (int i = 0; i < TOTAL_CARROS; i++) {
        printf("%.0f ", vetor[i]->distanciaPercorrida);
    }
    printf("\n");
    verificar(estaOrdenada(vetor, TOTAL_CARROS), "vetor embaralhado fica em ordem decrescente");
    verificar(vetor[0] == &carros[1], "o 1o lugar e o carro que andou 900");
    verificar(carros[1].distanciaPercorrida == 900 && carros[2].distanciaPercorrida == 100,
              "os carros nao mudam de lugar na memoria: so os ponteiros sao trocados");

    /* Já ordenado: nenhum deslocamento (melhor caso, O(n)) */
    deslocamentos = ordenarCarrosPorDistancia(vetor, TOTAL_CARROS);
    printf("     ja ordenado: %d deslocamentos\n", deslocamentos);
    verificar(deslocamentos == 0, "vetor ja ordenado: 0 deslocamentos");

    /* Uma ultrapassagem: só um deslocamento (o caso de toda hora no jogo) */
    vetor[5]->distanciaPercorrida = vetor[4]->distanciaPercorrida + 1;
    deslocamentos = ordenarCarrosPorDistancia(vetor, TOTAL_CARROS);
    printf("     uma ultrapassagem: %d deslocamento(s)\n", deslocamentos);
    verificar(deslocamentos == 1 && estaOrdenada(vetor, TOTAL_CARROS), "uma ultrapassagem: 1 deslocamento");

    /* Ao contrário: pior caso, n(n-1)/2 deslocamentos (O(n²)) */
    for (int i = 0; i < TOTAL_CARROS; i++) {
        vetor[i]->distanciaPercorrida = (float) i;
    }
    deslocamentos = ordenarCarrosPorDistancia(vetor, TOTAL_CARROS);
    printf("     ao contrario: %d deslocamentos\n", deslocamentos);
    verificar(deslocamentos == TOTAL_CARROS * (TOTAL_CARROS - 1) / 2,
              "vetor ao contrario: 28 deslocamentos (pior caso)");
}

static void testarCorridaComRivais(void) {
    printf("\n8) Corrida com %d carros (o jogador tambem dirigido pela IA)\n", TOTAL_CARROS);
    Pista *pista = criarPista();
    Corrida corrida;
    iniciarCorrida(&corrida, pista);
    float quadro = 1.0f / 60.0f;

    verificar(posicaoDoCarro(&corrida, &corrida.jogador) == TOTAL_CARROS, "o jogador larga em ultimo");
    verificar(carroDaFrente(&corrida, corrida.classificacao[0]) == NULL, "o 1o lugar nao tem carro da frente");

    int sempreOrdenada = 1;
    int mudancasDePosicao = 0;
    int posicaoAnterior = posicaoDoCarro(&corrida, &corrida.jogador);
    int quadros = 0;
    while (!corrida.terminou && quadros < 60 * 60 * 10) {
        Comandos comandos = atualizarIA(&corrida.jogador, carroDaFrente(&corrida, &corrida.jogador));
        atualizarCorrida(&corrida, comandos, quadro);
        quadros++;

        if (!estaOrdenada(corrida.classificacao, TOTAL_CARROS)) {
            sempreOrdenada = 0;
        }
        int posicao = posicaoDoCarro(&corrida, &corrida.jogador);
        if (posicao != posicaoAnterior) {
            mudancasDePosicao++;
            posicaoAnterior = posicao;
        }
    }

    printf("     chegada (tempo do jogador: %.2f s):\n", corrida.jogador.tempoDeChegada);
    for (int i = 0; i < TOTAL_CARROS; i++) {
        Carro *carro = corrida.classificacao[i];
        printf("       %do %-10s %d voltas  %.0f unidades\n", i + 1, carro->nome,
               carro->voltas, carro->distanciaPercorrida);
    }
    printf("     o jogador mudou de posicao %d vezes\n", mudancasDePosicao);

    verificar(corrida.terminou, "a corrida com 8 carros termina");
    verificar(sempreOrdenada, "a classificacao ficou ordenada em todos os quadros");
    verificar(corrida.posicaoFinal >= 1 && corrida.posicaoFinal <= TOTAL_CARROS, "posicao final entre 1 e 8");
    verificar(mudancasDePosicao > 0, "houve ultrapassagens");

    liberarPista(pista);
}

int main(int argc, char *argv[]) {
    int modoDetalhado = (argc > 1 && strcmp(argv[1], "-v") == 0);

    printf("=== Teste da pista (lista circular duplamente encadeada) ===\n");
    testarPistaVazia();
    testarInserirERemover();
    testarAutodromo(modoDetalhado);
    testarMoverNaPista();
    testarCarro();
    testarCorridaCompleta();
    testarInsertionSort();
    testarCorridaComRivais();

    if (totalFalhas == 0) {
        printf("\nTodos os testes passaram.\n");
        return 0;
    }
    printf("\n%d teste(s) falharam.\n", totalFalhas);
    return 1;
}
