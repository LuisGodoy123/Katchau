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
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "pista.h"
#include "cenario_mapas.h"
#include "carro.h"
#include "corrida.h"
#include "ordenacao.h"
#include "ia.h"
#include "obstaculos.h"
#include "motorgranular.h"
#include "motor.h"
#include "olhar.h"
#include "pitstop.h"
#include "vibracao.h"
#include "configuracoes.h"
#include "config.h"

#define TOTAL_VOLTAS  5   /* os testes correm no nivel medio */

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

    /* Imprime os trechos: agrupa segmentos seguidos com o mesmo nome.
     * Como as curvas são suavizadas (a curvatura muda aos poucos), o que
     * se mostra é quanto o trecho vira no total e o ponto mais fechado. */
    printf("\n     Trechos da pista:\n");
    Segmento *inicioTrecho = pista->largada;
    float anguloDoTrecho = 0.0f;
    float maisFechado = 0.0f;
    atual = pista->largada;
    do {
        anguloDoTrecho += atual->curvatura;
        if (fabsf(atual->curvatura) > fabsf(maisFechado)) {
            maisFechado = atual->curvatura;
        }
        if (modoDetalhado) {
            printf("       segmento %4d | curvatura %6.3f | altura %6.1f m | %s%s\n",
                   atual->indice, atual->curvatura, atual->altura / UNIDADES_POR_METRO,
                   descreverCurvatura(atual->curvatura),
                   atual->ehLinhaDeChegada ? "  <- LARGADA" : "");
        }
        Segmento *seguinte = proximoSegmento(atual);
        int trechoAcabou = (seguinte == pista->largada) ||
                           (strcmp(seguinte->nomeTrecho, atual->nomeTrecho) != 0);
        if (trechoAcabou) {
            printf("       segmentos %4d a %4d: %-18s %-16s (vira %7.1f graus; no maximo %6.3f/segmento)\n",
                   inicioTrecho->indice, atual->indice, atual->nomeTrecho,
                   descreverCurvatura(anguloDoTrecho), anguloDoTrecho, maisFechado);
            inicioTrecho = seguinte;
            anguloDoTrecho = 0.0f;
            maisFechado = 0.0f;
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
    Comandos acelerar = { 1, 0, 0, 0, 0.0f, 0, 0 };
    Comandos frear = { 0, 1, 0, 0, 0.0f, 0, 0 };
    Comandos virarDireita = { 0, 0, 0, 1, 0.0f, 0, 0 };

    /* Para medir velocidade, um "oval" só de retas: sem curva, nada
     * empurra o carro para a grama. */
    Pista *retaInfinita = criarPistaVazia();
    adicionarTrecho(retaInfinita, "Reta de teste", 50, RETA, 0.0f, 0.0f);

    Carro carro = criarCarro("Teste", 0, retaInfinita->largada, 0.0f, 12000.0f, 2400.0f, 0.5f);
    atualizarCarro(&carro, virarDireita, quadro);
    verificar(carro.deslocamentoLateral == 0.0f, "parado, o carro nao vira");

    /* Arrancada de F1: forte no começo, cada vez mais fraca perto da máxima */
    float tempo100 = 0.0f, tempo200 = 0.0f, tempo300 = 0.0f;
    for (int i = 1; i <= 60 * 20; i++) {
        atualizarCarro(&carro, acelerar, quadro);
        int kmh = velocidadeEmKmh(&carro);
        if (tempo100 == 0.0f && kmh >= 100) tempo100 = i * quadro;
        if (tempo200 == 0.0f && kmh >= 200) tempo200 = i * quadro;
        if (tempo300 == 0.0f && kmh >= 300) tempo300 = i * quadro;
    }
    printf("     0-100 km/h: %.1f s   0-200: %.1f s   0-300: %.1f s\n", tempo100, tempo200, tempo300);
    verificar(tempo100 > 3.0f && tempo100 < 4.0f, "0-100 km/h em 3 a 4 s (arrancada mais calma que a de um F1)");
    verificar(tempo300 > 13.0f && tempo300 < 17.0f, "0-300 km/h em 13 a 17 s, nao em 5");
    verificar(tempo200 - tempo100 < tempo300 - tempo200,
              "de 200 a 300 demora mais que de 100 a 200 (o ar segura o carro)");
    verificar(carro.velocidade == carro.velocidadeMaxima, "acelerando, chega na velocidade maxima e para ali");
    FichaTecnica ficha = { "Teste", "", 12000.0f, 2400.0f, 0.5f, 4.4f, 2.0f };
    float calculado = tempoAteVelocidade(&ficha, 4000.0f);
    printf("     0-100 km/h mostrado na garagem: %.2f s\n", calculado);
    verificar(fabsf(calculado - tempo100) < 0.05f, "a garagem mostra o mesmo 0-100 km/h da corrida");
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
    iniciarCorrida(&corrida, pista, 0, NIVEL_MEDIO);
    float quadro = 1.0f / 60.0f;
    int quadrosMaximos = 60 * 60 * 10;   /* 10 minutos de jogo */

    int quadros = 0;
    while (!corrida.terminou && quadros < quadrosMaximos) {
        /* Acelera sempre e corrige o volante para o centro */
        Comandos comandos = { 1, 0, 0, 0, 0.0f, 0, 0 };
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
    Comandos pisarNoFreio = { 0, 1, 0, 0, 0.0f, 0, 0 };
    atualizarCorrida(&corrida, pisarNoFreio, quadro);
    verificar(corrida.jogador.tempoDeChegada == tempoNaChegada, "depois do fim o tempo do jogador nao muda");
    verificar(!corrida.jogador.ultimosComandos.frear, "depois do fim o teclado e ignorado: quem dirige e a IA");

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
    Carro carros[TOTAL_CARROS] = { 0 };   /* tudo zerado: ninguem chegou ainda */
    Carro *vetor[TOTAL_CARROS];

    /* Embaralhado */
    float distancias[TOTAL_CARROS] = { 300, 900, 100, 700, 800, 200, 600, 400, 500, 50, 850, 750, 150, 650 };
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
    iniciarCorrida(&corrida, pista, 0, NIVEL_MEDIO);
    float quadro = 1.0f / 60.0f;

    verificar(posicaoDoCarro(&corrida, &corrida.jogador) == TOTAL_CARROS, "o jogador larga em ultimo");

    int sempreOrdenada = 1;
    int mudancasDePosicao = 0;
    int posicaoAnterior = posicaoDoCarro(&corrida, &corrida.jogador);
    int quadros = 0;
    while (!corrida.terminou && quadros < 60 * 60 * 10) {
        Comandos comandos = atualizarIA(&corrida.jogador, corrida.pista);
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
    verificar(corrida.posicaoFinal >= 1 && corrida.posicaoFinal <= TOTAL_CARROS, "posicao final entre 1 e o total de carros");
    verificar(mudancasDePosicao > 0, "houve ultrapassagens");

    liberarPista(pista);
}

/*
 * fichasEmOrdem
 * Recebe: um vetor de fichas, o tamanho e o atributo.
 * Devolve: 1 se estiver do maior para o menor valor do atributo.
 */
static int fichasEmOrdem(const FichaTecnica *fichas[], int quantidade, Atributo atributo) {
    for (int i = 1; i < quantidade; i++) {
        if (valorDoAtributo(fichas[i - 1], atributo) < valorDoAtributo(fichas[i], atributo)) {
            return 0;
        }
    }
    return 1;
}

static void testarMergeSort(void) {
    printf("\n9) Merge Sort da garagem (ordenarFichasPorAtributo)\n");
    const FichaTecnica *fichas[TOTAL_CARROS];

    for (int atributo = 0; atributo < TOTAL_ATRIBUTOS; atributo++) {
        for (int i = 0; i < TOTAL_CARROS; i++) {
            fichas[i] = fichaDoCarro(i);
        }
        ordenarFichasPorAtributo(fichas, TOTAL_CARROS, (Atributo) atributo);

        printf("     por %-10s:", nomeDoAtributo((Atributo) atributo));
        for (int i = 0; i < TOTAL_CARROS; i++) {
            printf(" %s", fichas[i]->nome);
        }
        printf("\n");
        char descricao[64];
        snprintf(descricao, sizeof(descricao), "garagem em ordem decrescente de %s",
                 nomeDoAtributo((Atributo) atributo));
        verificar(fichasEmOrdem(fichas, TOTAL_CARROS, (Atributo) atributo), descricao);
    }

    /* Estabilidade: com tudo empatado, a ordem original não muda */
    FichaTecnica iguais[4] = {
        { "A", "", 1.0f, 1.0f, 0.5f, 4.4f, 2.0f }, { "B", "", 1.0f, 1.0f, 0.5f, 4.4f, 2.0f },
        { "C", "", 1.0f, 1.0f, 0.5f, 4.4f, 2.0f }, { "D", "", 1.0f, 1.0f, 0.5f, 4.4f, 2.0f },
    };
    const FichaTecnica *vetorIguais[4] = { &iguais[0], &iguais[1], &iguais[2], &iguais[3] };
    ordenarFichasPorAtributo(vetorIguais, 4, ATRIBUTO_VELOCIDADE);
    verificar(vetorIguais[0] == &iguais[0] && vetorIguais[1] == &iguais[1] &&
              vetorIguais[2] == &iguais[2] && vetorIguais[3] == &iguais[3],
              "empates mantem a ordem original (Merge Sort e estavel)");

    /* Casos de borda: 1 elemento e 0 elementos não quebram */
    ordenarFichasPorAtributo(vetorIguais, 1, ATRIBUTO_ACELERACAO);
    ordenarFichasPorAtributo(vetorIguais, 0, ATRIBUTO_ACELERACAO);
    verificar(vetorIguais[0] == &iguais[0], "vetor com 1 ou 0 fichas fica como esta");
}

static void testarEscolhaDaGaragem(void) {
    printf("\n10) Garagem: correr com o carro escolhido\n");
    Pista *pista = criarPista();
    Corrida corrida;
    int escolhido = 3;   /* Trovao */
    iniciarCorrida(&corrida, pista, escolhido, NIVEL_MEDIO);

    verificar(corrida.jogador.numero == escolhido &&
              strcmp(corrida.jogador.nome, fichaDoCarro(escolhido)->nome) == 0,
              "o jogador corre com o carro escolhido");
    verificar(corrida.jogador.velocidadeMaxima == fichaDoCarro(escolhido)->velocidadeMaxima,
              "com a ficha tecnica dele");

    /* Os 7 rivais são os outros 7 carros, cada um uma vez */
    int apareceu[TOTAL_CARROS] = { 0 };
    apareceu[corrida.jogador.numero]++;
    for (int i = 0; i < TOTAL_RIVAIS; i++) {
        apareceu[corrida.rivais[i].numero]++;
    }
    int cadaUmUmaVez = 1;
    for (int i = 0; i < TOTAL_CARROS; i++) {
        if (apareceu[i] != 1) {
            cadaUmUmaVez = 0;
        }
    }
    verificar(cadaUmUmaVez, "os 8 carros estao na corrida, sem repetir");

    /* Contagem regressiva: ninguém anda antes de ela acabar */
    Comandos acelerar = { 1, 0, 0, 0, 0.0f, 0, 0 };
    float distanciaAntes = corrida.jogador.distanciaPercorrida;
    atualizarCorrida(&corrida, acelerar, 1.0f);
    verificar(corrida.jogador.distanciaPercorrida == distanciaAntes && corrida.tempoDeCorrida == 0.0f,
              "durante a contagem regressiva ninguem anda e o relogio nao corre");

    /* Corre até todo mundo chegar e confere a ordem de chegada */
    float quadro = 1.0f / 60.0f;
    int quadros = 0;
    while (!todosChegaram(&corrida) && quadros < 60 * 60 * 15) {
        Comandos comandos = atualizarIA(&corrida.jogador, corrida.pista);
        atualizarCorrida(&corrida, comandos, quadro);
        quadros++;
    }
    /* Depois de todos chegarem, os carros continuam andando: a ordem não pode mudar */
    for (int i = 0; i < 60 * 20; i++) {
        Comandos nenhum = { 0, 0, 0, 0, 0.0f, 0, 0 };
        atualizarCorrida(&corrida, nenhum, quadro);
    }

    int ordemPorTempo = 1;
    for (int i = 1; i < TOTAL_CARROS; i++) {
        if (corrida.classificacao[i - 1]->tempoDeChegada > corrida.classificacao[i]->tempoDeChegada) {
            ordemPorTempo = 0;
        }
    }
    printf("     resultado:");
    for (int i = 0; i < TOTAL_CARROS; i++) {
        printf(" %do %s (%.1f s)", i + 1, corrida.classificacao[i]->nome,
               corrida.classificacao[i]->tempoDeChegada);
    }
    printf("\n");
    verificar(todosChegaram(&corrida), "todos os carros completam a corrida");
    verificar(ordemPorTempo, "o resultado fica na ordem de chegada, mesmo com os carros andando depois");
    verificar(posicaoDoCarro(&corrida, &corrida.jogador) == corrida.posicaoFinal,
              "a posicao final do jogador bate com o resultado");

    liberarPista(pista);
}

static void testarVolante(void) {
    printf("\n11) Volante gradual\n");
    Pista *reta = criarPistaVazia();
    adicionarTrecho(reta, "Reta de teste", 50, RETA, 0.0f, 0.0f);
    Carro carro = criarCarro("Teste", 0, reta->largada, 0.0f, 12000.0f, 2400.0f, 0.5f);
    carro.velocidade = carro.velocidadeMaxima;
    float quadro = 1.0f / 60.0f;
    Comandos direita = { 0, 0, 0, 1, 0.0f, 0, 0 };
    Comandos nenhum = { 0, 0, 0, 0, 0.0f, 0, 0 };

    atualizarCarro(&carro, direita, quadro);
    verificar(carro.volante > 0.0f && carro.volante < 1.0f,
              "um quadro de seta gira o volante so um pouco");

    for (int i = 0; i < 30; i++) {
        atualizarCarro(&carro, direita, quadro);
    }
    verificar(carro.volante == 1.0f, "segurando a seta, o volante chega ao fim e para ali");

    for (int i = 0; i < 30; i++) {
        atualizarCarro(&carro, nenhum, quadro);
    }
    verificar(carro.volante == 0.0f, "soltando a seta, o volante volta sozinho ao centro");

    /* Analógico: o volante para onde o analógico manda, não no fim */
    Comandos meioDireita = { 0, 0, 0, 0, 0.5f, 0, 0 };
    for (int i = 0; i < 60; i++) {
        atualizarCarro(&carro, meioDireita, quadro);
    }
    verificar(fabsf(carro.volante - 0.5f) < 0.001f, "analogico pela metade: o volante para na metade");
    Carro inteiro = carro, metade = carro;
    inteiro.volante = 1.0f;
    inteiro.deslocamentoLateral = metade.deslocamentoLateral = 0.0f;
    Comandos tudoDireita = { 0, 0, 0, 0, 1.0f, 0, 0 };
    atualizarCarro(&inteiro, tudoDireita, quadro);
    atualizarCarro(&metade, meioDireita, quadro);
    verificar(fabsf(metade.deslocamentoLateral * 2.0f - inteiro.deslocamentoLateral) < 0.001f,
              "com metade do volante o carro vira metade");

    Comandos poucoDireita = { 0, 0, 0, 0, 0.2f, 0, 0 };
    for (int i = 0; i < 60; i++) {
        atualizarCarro(&carro, poucoDireita, quadro);
    }
    verificar(fabsf(carro.volante - 0.2f) < 0.001f, "aliviando o analogico, o volante desce junto (e para em 0,2)");

    Comandos setaEAnalogico = { 0, 0, 1, 0, 0.3f, 0, 0 };   /* seta para a esquerda, analógico um pouco à direita */
    for (int i = 0; i < 60; i++) {
        atualizarCarro(&carro, setaEAnalogico, quadro);
    }
    verificar(carro.volante == -1.0f, "seta e analogico juntos: vale o que pede mais");

    liberarPista(reta);
}

static void testarObjetosDaPista(void) {
    printf("\n12) Objetos da beira da pista (lista simples por segmento)\n");

    /* A lista de um segmento sozinha */
    Pista *reta = criarPistaVazia();
    adicionarTrecho(reta, "Reta de teste", 3, RETA, 0.0f, 0.0f);
    Segmento *segmento = reta->largada;
    verificar(contarObstaculos(segmento) == 0, "segmento novo nao tem objetos");
    adicionarObstaculo(segmento, OBJETO_ARVORE, 3.0f);
    adicionarObstaculo(segmento, OBJETO_PNEUS, -2.7f);
    verificar(contarObstaculos(segmento) == 2, "dois objetos adicionados, dois na lista");
    verificar(segmento->obstaculos->tipo == OBJETO_PNEUS,
              "o ultimo adicionado fica no comeco da lista");
    liberarObstaculos(segmento);
    verificar(segmento->obstaculos == NULL, "liberar deixa a lista vazia");
    adicionarObstaculo(segmento, OBJETO_ARVORE, 3.0f);
    liberarPista(reta);   /* tem que liberar os objetos junto (veja no valgrind) */

    /* Zebras: so nas curvas */
    Pista *pista = criarPista();
    int comZebra = 0;
    int zebraErrada = 0;
    Segmento *atual = pista->largada;
    do {
        if (temZebra(atual)) {
            comZebra++;
            if (atual->curvatura == RETA) {
                zebraErrada++;
            }
        }
        atual = proximoSegmento(atual);
    } while (atual != pista->largada);
    printf("     segmentos com zebra: %d de %d\n", comZebra, contarSegmentos(pista));
    verificar(!temZebra(pista->largada), "a reta dos boxes nao tem zebra");
    verificar(zebraErrada == 0, "nenhuma reta tem zebra");
    verificar(comZebra > 0 && comZebra < contarSegmentos(pista) / 2,
              "zebra em parte da pista, nao nela toda");

    /* Decoracao de Interlagos */
    int contagem[TOTAL_TIPOS_OBJETO] = { 0 };
    int noCaminho = 0;
    atual = pista->largada;
    do {
        for (Obstaculo *objeto = atual->obstaculos; objeto != NULL; objeto = objeto->proximo) {
            contagem[objeto->tipo]++;
            float lado = fabsf(objeto->deslocamentoLateral);
            int ficaDePe = objeto->tipo != OBJETO_PORTICO && objeto->tipo != OBJETO_MARCA_GRID
                           && objeto->tipo != OBJETO_ARQUIBANCADA && objeto->tipo != OBJETO_CAMAROTE;
            if (ficaDePe && !atual->temMuro && lado <= 2.5f) {
                noCaminho++;   /* o carro anda ate 2,5 (LIMITE_LATERAL em carro.c) */
            }
        }
        atual = proximoSegmento(atual);
    } while (atual != pista->largada);
    printf("     arvores: %d  pneus: %d  placas: %d/%d/%d\n",
           contagem[OBJETO_ARVORE], contagem[OBJETO_PNEUS],
           contagem[OBJETO_PLACA_150], contagem[OBJETO_PLACA_100], contagem[OBJETO_PLACA_50]);
    verificar(contagem[OBJETO_PORTICO] == 1, "um portico so");
    int porticoNaLargada = 0;
    for (Obstaculo *objeto = pista->largada->obstaculos; objeto != NULL; objeto = objeto->proximo) {
        if (objeto->tipo == OBJETO_PORTICO) {
            porticoNaLargada = 1;
        }
    }
    verificar(porticoNaLargada, "o portico fica na linha de chegada");
    verificar(contagem[OBJETO_ARVORE] > 100, "a pista tem arvores");
    verificar(contagem[OBJETO_PNEUS] > 0, "as curvas tem barreiras de pneus");
    verificar(contagem[OBJETO_PLACA_50] > 0 && contagem[OBJETO_PLACA_50] >= contagem[OBJETO_PLACA_150],
              "placas de frenagem antes das curvas (a de 50 m sempre que tem a de 150 m)");
    verificar(noCaminho == 0, "nenhum objeto de pe fica onde o carro pode passar (fora o portico)");

    /* Arquibancadas: cada setor do mapa está no trecho certo da pista */
    static const struct { char setor; const char *trecho; } SETORES[] = {
        { 'A', "Subida dos Boxes" }, { 'B', "Reta dos Boxes" }, { 'M', "Reta dos Boxes" }, { 'D', "S do Senna" },
        { 'H', "Curva do Sol" }, { 'N', "Curva do Sol" }, { 'R', "Reta Oposta" }, { 'G', "Reta Oposta" },
        { 'S', "Reta Oposta" }, { 'V', "Descida do Lago" },
    };
    int setoresCertos = 0;
    for (int i = 0; i < 10; i++) {
        atual = pista->largada;
        do {
            for (Obstaculo *objeto = atual->obstaculos; objeto != NULL; objeto = objeto->proximo) {
                if (objeto->setor == SETORES[i].setor && objeto->deslocamentoLateral > 0.0f) {
                    /* o módulo do meio pode cair no trecho seguinte: confere uns segmentos em volta */
                    Segmento *perto = atual;
                    int achou = 0;
                    for (int k = 0; k < 120; k++) {
                        perto = segmentoAnterior(perto);
                    }
                    for (int k = 0; k < 240; k++) {
                        achou = achou || strcmp(perto->nomeTrecho, SETORES[i].trecho) == 0;
                        perto = proximoSegmento(perto);
                    }
                    setoresCertos += achou;
                }
            }
            atual = proximoSegmento(atual);
        } while (atual != pista->largada);
    }
    printf("     arquibancadas: %d modulos, camarotes: %d; %d setores no trecho certo\n",
           contagem[OBJETO_ARQUIBANCADA], contagem[OBJETO_CAMAROTE], setoresCertos);
    verificar(setoresCertos == 10, "os 10 setores do mapa estao no trecho certo da pista");
    verificar(contagem[OBJETO_CAMAROTE] >= 2, "os dois camarotes do mapa existem");

    /* No mundo: nenhum objeto cai em cima de OUTRO trecho da pista, nem dentro do alcance do carro nele */
    int emCimaDaPista = 0, dentroDoAlcance = 0, total = 0;
    float maisPerto = 1e9f;
    atual = pista->largada;
    do {
        PosicaoNaPista lugar = { atual, 0.0f };
        for (Obstaculo *objeto = atual->obstaculos; objeto != NULL; objeto = objeto->proximo) {
            if (objeto->tipo == OBJETO_PORTICO || objeto->tipo == OBJETO_MARCA_GRID) {
                continue;
            }
            total++;
            PontoNoMundo ponto = pontoNoMundo(lugar, objeto->deslocamentoLateral);
            Segmento *outro = pista->largada;
            float menor = 1e9f, alcanceAli = 0.0f;
            do {
                float distanciaAqui = hypotf(outro->xMundo - ponto.x, outro->zMundo - ponto.z);
                if (distanciaAqui < menor) {
                    menor = distanciaAqui;
                    alcanceAli = outro->larguraDireita * (outro->temMuro ? outro->lugarDoMuro : 2.5f);
                }
                outro = proximoSegmento(outro);
            } while (outro != pista->largada);
            maisPerto = fminf(maisPerto, menor);
            emCimaDaPista += (menor < atual->larguraDireita * 1.15f);
            dentroDoAlcance += (menor < alcanceAli);
        }
        atual = proximoSegmento(atual);
    } while (atual != pista->largada);
    printf("     %d objetos; o mais perto da pista fica a %.1f m da linha dela\n", total, maisPerto);
    verificar(emCimaDaPista == 0, "nenhum objeto em cima do asfalto de outro trecho");
    verificar(dentroDoAlcance == 0, "nenhum objeto onde o carro alcanca, em nenhum trecho da pista");
    verificar(contagem[OBJETO_MARCA_GRID] == LUGARES_NO_GRID, "um colchete pintado para cada lugar do grid");

    Pista *outra = criarPista();
    verificar(contarObstaculos(buscarSegmento(outra, 500)) == contarObstaculos(buscarSegmento(pista, 500)),
              "a decoracao e sempre igual (sorteio com semente fixa)");
    liberarPista(outra);
    liberarPista(pista);
}

static void testarDirigibilidade(void) {
    printf("\n13) Dirigibilidade: curvas suaves, direcao e derrapagem\n");
    Pista *pista = criarPista();
    float quadro = 1.0f / 60.0f;

    /* Curvas suaves: entre dois segmentos vizinhos a curvatura muda pouco */
    float maiorSalto = 0.0f;
    Segmento *atual = pista->largada;
    do {
        float salto = fabsf(proximoSegmento(atual)->curvatura - atual->curvatura);
        if (salto > maiorSalto) {
            maiorSalto = salto;
        }
        atual = proximoSegmento(atual);
    } while (atual != pista->largada);
    printf("     maior mudanca de curvatura entre vizinhos: %.3f graus/segmento\n", maiorSalto);
    verificar(maiorSalto < 0.3f, "a curva entra e sai aos poucos (sem degrau)");

    /* Direção em baixa velocidade: a 100 km/h vira mais que 1/3 do máximo */
    Pista *reta = criarPistaVazia();
    adicionarTrecho(reta, "Reta de teste", 50, RETA, 0.0f, 0.0f);
    Carro devagar = criarCarro("Teste", 0, reta->largada, 0.0f, 12000.0f, 2400.0f, 0.5f);
    Carro rapido = devagar;
    devagar.velocidade = 4000.0f;   /* 100 km/h */
    rapido.velocidade = 12000.0f;   /* 300 km/h */
    devagar.volante = 1.0f;
    rapido.volante = 1.0f;
    Comandos direitaSemPedal = { 0, 0, 0, 1, 0.0f, 0, 0 };
    atualizarCarro(&devagar, direitaSemPedal, quadro);
    atualizarCarro(&rapido, direitaSemPedal, quadro);
    float proporcao = devagar.deslocamentoLateral / rapido.deslocamentoLateral;
    printf("     a 100 km/h o carro vira %.0f%% do que vira a 300 km/h\n", proporcao * 100.0f);
    verificar(proporcao > 0.5f, "a 100 km/h a direcao ja responde bem (mais de 50%)");

    /* Derrapagem: na reta não tem; na curva mais fechada, em alta, tem */
    verificar(rapido.derrapagem == 0.0f, "na reta o carro nao derrapa");
    Segmento *maisFechada = pista->largada;
    atual = pista->largada;
    do {
        if (fabsf(atual->curvatura) > fabsf(maisFechada->curvatura)) {
            maisFechada = atual;
        }
        atual = proximoSegmento(atual);
    } while (atual != pista->largada);
    Carro naCurva = criarCarro("Teste", 0, maisFechada, 0.0f, 12000.0f, 2400.0f, 0.5f);
    naCurva.velocidade = naCurva.velocidadeMaxima;
    Comandos nenhum = { 0, 0, 0, 0, 0.0f, 0, 0 };
    atualizarCarro(&naCurva, nenhum, quadro);
    printf("     %s a 300 km/h: derrapagem %.2f; velocidade segura %d km/h\n",
           maisFechada->nomeTrecho, naCurva.derrapagem,
           converterParaKmh(velocidadeSeguraNaCurva(&naCurva, maisFechada->curvatura)));
    verificar(naCurva.derrapagem == 1.0f, "a 300 km/h na curva mais fechada o carro derrapa");

    Carro seguro = criarCarro("Teste", 0, maisFechada, 0.0f, 12000.0f, 2400.0f, 0.5f);
    seguro.velocidade = velocidadeSeguraNaCurva(&seguro, maisFechada->curvatura) * 0.9f;
    atualizarCarro(&seguro, nenhum, quadro);
    verificar(seguro.derrapagem < 1.0f, "abaixo da velocidade segura o volante ainda vence a curva");

    liberarPista(reta);
    liberarPista(pista);
}

/* ---------- Som da batida: só o McQueen ---------- */
static void testarBatidaDoMcQueen(void) {
    printf("\n[Teste] Batida do McQueen (so ele tem som de batida)\n");
    Pista *pista = criarPista();
    Corrida corrida;

    /* O jogador é o McQueen: a batida dele toca, com a força dela, e some depois de lida */
    iniciarCorrida(&corrida, pista, 0, NIVEL_MEDIO);
    corrida.jogador.batidaNova = 0.8f;
    verificar(fabsf(batidaDoMcQueen(&corrida, NULL) - 0.8f) < 0.001f, "o McQueen jogador bate: o volume e a forca da batida");
    verificar(batidaDoMcQueen(&corrida, NULL) == 0.0f, "a batida so toca uma vez");

    /* O jogador é outro carro: a batida dele não toca */
    iniciarCorrida(&corrida, pista, 4, NIVEL_MEDIO);
    corrida.jogador.batidaNova = 1.0f;
    verificar(batidaDoMcQueen(&corrida, NULL) == 0.0f, "outro carro bate: sem som");

    /* O McQueen é rival: só soa se bater perto do jogador, e mais baixo */
    Carro *mcqueen = NULL;
    for (int r = 0; r < TOTAL_RIVAIS; r++) {
        if (corrida.rivais[r].numero == 0) {
            mcqueen = &corrida.rivais[r];
        }
    }
    verificar(mcqueen != NULL, "com outro carro escolhido, o McQueen e um rival");
    if (mcqueen != NULL) {
        mcqueen->distanciaPercorrida = corrida.jogador.distanciaPercorrida + 10.0f * UNIDADES_POR_METRO;
        mcqueen->batidaNova = 1.0f;
        float perto = batidaDoMcQueen(&corrida, NULL);
        verificar(perto > 0.0f && perto < 1.0f, "o McQueen rival bate perto: soa, mais baixo");
        mcqueen->distanciaPercorrida = corrida.jogador.distanciaPercorrida + 500.0f * UNIDADES_POR_METRO;
        mcqueen->batidaNova = 1.0f;
        verificar(batidaDoMcQueen(&corrida, NULL) == 0.0f, "o McQueen rival bate longe: ninguem ouve");
    }

    /* Batida de carro com carro: os dois ficam com a batida nova */
    iniciarCorrida(&corrida, pista, 0, NIVEL_MEDIO);
    corrida.jogador.batidaNova = 0.0f;
    verificar(batidaDoMcQueen(&corrida, NULL) == 0.0f, "sem batida, sem som");
    liberarPista(pista);
}

/* ---------- Falas do McQueen: ultrapassagens e batida contra carro ---------- */
static void testarUltrapassagensDoMcQueen(void) {
    printf("\n[Teste] Falas de ultrapassagem do McQueen (so uma por troca de posicao)\n");
    Pista *pista = criarPista();
    Corrida corrida;
    float quadro = 1.0f / 60.0f;

    iniciarCorrida(&corrida, pista, 0, NIVEL_MEDIO);
    verificar(mudancaDePosicaoDoMcQueen(&corrida, quadro) == 0, "na contagem nao ha fala de ultrapassagem");

    /* O jogador no meio da fila (5º), com a corrida andando */
    corrida.contagemRegressiva = 0.0f;
    Carro *jogador = &corrida.jogador;
    int ordem[TOTAL_CARROS];
    int quantos = 0;
    for (int i = 0; i < TOTAL_CARROS; i++) {
        if (corrida.classificacao[i] != jogador) {
            ordem[quantos++] = i;
        }
    }
    Carro *outros[TOTAL_CARROS];
    for (int i = 0; i < quantos; i++) {
        outros[i] = corrida.classificacao[ordem[i]];
    }
    #define COLOCAR_NA_POSICAO(p) do { \
        int usados = 0; \
        for (int k = 0; k < TOTAL_CARROS; k++) { \
            corrida.classificacao[k] = (k == (p) - 1) ? jogador : outros[usados++]; \
        } \
    } while (0)

    COLOCAR_NA_POSICAO(5);
    verificar(mudancaDePosicaoDoMcQueen(&corrida, quadro) == 0, "o primeiro quadro so acompanha a posicao");
    int falou = 0;
    for (int i = 0; i < 60; i++) {
        falou = falou || mudancaDePosicaoDoMcQueen(&corrida, quadro) != 0;
    }
    verificar(!falou, "parado na mesma posicao, nao fala");

    /* Ultrapassa um carro (5º para 4º): uma fala, depois de a posicao se manter um instante */
    COLOCAR_NA_POSICAO(4);
    int ganhou = 0, vezes = 0;
    for (int i = 0; i < 60; i++) {
        int r = mudancaDePosicaoDoMcQueen(&corrida, quadro);
        if (r != 0) {
            vezes++;
            ganhou = r;
        }
    }
    verificar(vezes == 1 && ganhou == 1, "subiu de posicao: uma fala de ultrapassagem");

    /* Ultrapassa varios de uma vez (4º para 1º... passando por 3º e 2º): uma fala so */
    COLOCAR_NA_POSICAO(3);
    int r1 = mudancaDePosicaoDoMcQueen(&corrida, quadro);
    COLOCAR_NA_POSICAO(2);
    int r2 = mudancaDePosicaoDoMcQueen(&corrida, quadro);
    COLOCAR_NA_POSICAO(2);
    vezes = 0;
    for (int i = 0; i < 60; i++) {
        vezes += mudancaDePosicaoDoMcQueen(&corrida, quadro) != 0;
    }
    verificar(r1 == 0 && r2 == 0 && vezes == 1, "passou varios quase juntos: uma fala so");

    /* Foi ultrapassado, em qualquer posição (2º para 5º, por varios): uma fala de ultrapassado */
    COLOCAR_NA_POSICAO(3);
    mudancaDePosicaoDoMcQueen(&corrida, quadro);
    COLOCAR_NA_POSICAO(4);
    mudancaDePosicaoDoMcQueen(&corrida, quadro);
    COLOCAR_NA_POSICAO(5);
    int perdeu = 0;
    vezes = 0;
    for (int i = 0; i < 60; i++) {
        int r = mudancaDePosicaoDoMcQueen(&corrida, quadro);
        if (r != 0) {
            vezes++;
            perdeu = r;
        }
    }
    verificar(vezes == 1 && perdeu == -1, "ultrapassado por varios: uma fala de ultrapassado so");

    /* Dois carros trocando de lugar a todo quadro: nenhuma fala */
    int piscou = 0;
    for (int i = 0; i < 120; i++) {
        COLOCAR_NA_POSICAO((i % 2) ? 5 : 6);
        piscou = piscou || mudancaDePosicaoDoMcQueen(&corrida, quadro) != 0;
    }
    verificar(!piscou, "troca de lugar a todo quadro nao gera fala");

    /* No pit lane, perder posição não conta como ser ultrapassado */
    COLOCAR_NA_POSICAO(5);
    for (int i = 0; i < 60; i++) {
        mudancaDePosicaoDoMcQueen(&corrida, quadro);
    }
    jogador->estadoDoPit = PIT_CHEGANDO;
    COLOCAR_NA_POSICAO(8);
    int noPit = 0;
    for (int i = 0; i < 60; i++) {
        noPit = noPit || mudancaDePosicaoDoMcQueen(&corrida, quadro) != 0;
    }
    verificar(!noPit, "no pit lane, perder posicao nao gera fala");
    jogador->estadoDoPit = PIT_FORA;
    #undef COLOCAR_NA_POSICAO

    /* Com outro carro escolhido, o McQueen rival não faz a voz do jogador falar */
    iniciarCorrida(&corrida, pista, 4, NIVEL_MEDIO);
    corrida.contagemRegressiva = 0.0f;
    verificar(mudancaDePosicaoDoMcQueen(&corrida, quadro) == 0, "jogador de outro carro: sem falas de ultrapassagem");

    /* A batida diz se foi contra outro carro (as falas de bater) ou no muro (os sons de batida) */
    iniciarCorrida(&corrida, pista, 0, NIVEL_MEDIO);
    int contraCarro = -1;
    corrida.jogador.batidaNova = 0.6f;
    corrida.jogador.batidaNovaDeCarro = 1;
    float v = batidaDoMcQueen(&corrida, &contraCarro);
    verificar(v > 0.5f && contraCarro == 1, "bateu em outro carro: a batida diz que foi contra carro");
    corrida.jogador.batidaNova = 0.6f;
    corrida.jogador.batidaNovaDeCarro = 0;
    v = batidaDoMcQueen(&corrida, &contraCarro);
    verificar(v > 0.5f && contraCarro == 0, "bateu no muro: a batida diz que nao foi contra carro");

    /* O grito de rodada: só quando o McQueen RODA, não quando bate num carro que gira */
    iniciarCorrida(&corrida, pista, 0, NIVEL_MEDIO);
    verificar(rodadaDoMcQueen(&corrida) == 0.0f, "sem rodada, sem grito");
    corrida.jogador.batidaNova = 1.0f;
    corrida.jogador.batidaNovaDeCarro = 1;
    fazerRodar(&corrida.rivais[0], 400.0f);   /* o McQueen bate e o OUTRO carro roda */
    verificar(corrida.rivais[0].rodando && rodadaDoMcQueen(&corrida) == 0.0f,
              "bater num carro que gira nao faz o McQueen gritar");
    fazerRodar(&corrida.jogador, 400.0f);     /* agora o McQueen roda */
    verificar(corrida.jogador.rodando && rodadaDoMcQueen(&corrida) == 1.0f, "o McQueen roda: grita");
    verificar(rodadaDoMcQueen(&corrida) == 0.0f, "o grito so toca uma vez por rodada");

    iniciarCorrida(&corrida, pista, 4, NIVEL_MEDIO);   /* outro carro escolhido: o jogador roda, sem grito */
    fazerRodar(&corrida.jogador, 400.0f);
    verificar(corrida.jogador.rodando && rodadaDoMcQueen(&corrida) == 0.0f, "outro carro rodando: sem grito");
    liberarPista(pista);
}

/* ---------- Falas do McQueen: liderança ---------- */
static void testarLiderancaDoMcQueen(void) {
    printf("\n[Teste] Falas de lideranca do McQueen (sem repetir na ultrapassagem em serie)\n");
    Pista *pista = criarPista();
    Corrida corrida;
    float quadro = 1.0f / 60.0f;

    /* Na contagem, nada: ele larga em último e ninguém lidera */
    iniciarCorrida(&corrida, pista, 0, NIVEL_MEDIO);
    verificar(mudancaDeLiderancaDoMcQueen(&corrida, quadro) == 0, "na contagem nao ha fala de lideranca");

    /* Assume a ponta: uma fala só, e depois silêncio enquanto continua lá */
    corrida.contagemRegressiva = 0.0f;
    Carro *primeiro = corrida.classificacao[0];
    int posicaoDele = 0;
    for (int i = 0; i < TOTAL_CARROS; i++) {
        if (corrida.classificacao[i] == &corrida.jogador) {
            posicaoDele = i;
        }
    }
    corrida.classificacao[0] = &corrida.jogador;
    corrida.classificacao[posicaoDele] = primeiro;
    verificar(mudancaDeLiderancaDoMcQueen(&corrida, quadro) == 1, "assumiu a lideranca: uma fala");
    int repetiu = 0;
    for (int i = 0; i < 60 * 3; i++) {
        repetiu = repetiu || mudancaDeLiderancaDoMcQueen(&corrida, quadro) != 0;
    }
    verificar(!repetiu, "continuando na ponta, nao fala de novo");

    /* Vários carros o passam em seguida: a perda é uma só */
    Carro *outro = corrida.classificacao[1];
    corrida.classificacao[0] = outro;
    corrida.classificacao[1] = &corrida.jogador;
    verificar(mudancaDeLiderancaDoMcQueen(&corrida, quadro) == -1, "perdeu a lideranca: uma fala");
    int quantas = 0;
    for (int i = 0; i < 60; i++) {
        corrida.classificacao[1] = corrida.classificacao[2];
        corrida.classificacao[2] = &corrida.jogador;   /* e continua caindo */
        quantas += mudancaDeLiderancaDoMcQueen(&corrida, quadro) != 0;
    }
    verificar(quantas == 0, "ultrapassado por varios, nao repete a fala");

    /* Ficou na ponta menos de 2 s e perdeu: nem conta como perda */
    corrida.classificacao[0] = &corrida.jogador;
    verificar(mudancaDeLiderancaDoMcQueen(&corrida, quadro) == 1, "voltou para a ponta: fala de assumir");
    corrida.classificacao[0] = outro;
    verificar(mudancaDeLiderancaDoMcQueen(&corrida, quadro) == 0, "ficou na ponta menos de 2 s: nao conta como perda");

    /* Outro carro escolhido: o McQueen rival liderando não faz a voz do jogador falar */
    iniciarCorrida(&corrida, pista, 4, NIVEL_MEDIO);
    corrida.contagemRegressiva = 0.0f;
    verificar(mudancaDeLiderancaDoMcQueen(&corrida, quadro) == 0, "jogador de outro carro: sem falas de lideranca");
    liberarPista(pista);
}

static void testarGridDeLargada(void) {
    printf("\n14) Grid de largada antes da linha de chegada\n");
    Pista *pista = criarPista();
    Corrida corrida;
    iniciarCorrida(&corrida, pista, 0, NIVEL_MEDIO);
    int total = contarSegmentos(pista);

    verificar(TOTAL_CARROS <= LUGARES_NO_GRID, "o grid tem lugar para todos os carros");

    int todosAntes = 1;
    int noColchete = 1;
    for (int i = 0; i < TOTAL_CARROS; i++) {
        Carro *carro = corrida.classificacao[i];
        /* antes da linha = nos últimos segmentos da volta */
        if (carro->posicao.segmento->indice < total / 2 || carro->voltas != -1) {
            todosAntes = 0;
        }
        int temMarca = 0;
        for (Obstaculo *objeto = carro->posicao.segmento->obstaculos; objeto != NULL; objeto = objeto->proximo) {
            if (objeto->tipo == OBJETO_MARCA_GRID && objeto->deslocamentoLateral == carro->deslocamentoLateral) {
                temMarca = 1;
            }
        }
        if (!temMarca) {
            noColchete = 0;
        }
    }
    verificar(todosAntes, "todos os carros largam antes da linha de chegada");
    verificar(noColchete, "cada carro larga em cima do seu colchete");
    verificar(corrida.classificacao[0]->distanciaPercorrida == -distanciaDoLugarAteALinha(0),
              "o pole e quem esta mais perto da linha");
    verificar(voltaAtual(&corrida.jogador) == 1, "no grid o HUD mostra a volta 1");

    /* Anda até o jogador cruzar a linha pela primeira vez */
    float quadro = 1.0f / 60.0f;
    Comandos acelerar = { 1, 0, 0, 0, 0.0f, 0, 0 };
    int quadros = 0;
    while (corrida.jogador.voltas < 0 && quadros < 60 * 30) {
        atualizarCorrida(&corrida, acelerar, quadro);
        quadros++;
    }
    verificar(corrida.jogador.voltas == 0 && voltaAtual(&corrida.jogador) == 1,
              "cruzar a linha saindo do grid nao conta volta: comeca a volta 1");
    verificar(corrida.jogador.temposDeVolta[0] == 0.0f, "e nenhum tempo de volta e registrado ainda");

    liberarPista(pista);
}

static void testarSetoresEIntervalos(void) {
    printf("\n15) Setores, melhor volta e intervalos (para o HUD)\n");
    Pista *pista = criarPista();
    Corrida corrida;
    iniciarCorrida(&corrida, pista, 0, NIVEL_MEDIO);
    float quadro = 1.0f / 60.0f;

    verificar(bandeiraDaCorrida(&corrida) == BANDEIRA_LARGADA, "na contagem regressiva a bandeira e de largada");
    for (int i = 0; i < 60 * 5; i++) {
        atualizarCorrida(&corrida, atualizarIA(&corrida.jogador, corrida.pista), quadro);
    }
    verificar(bandeiraDaCorrida(&corrida) == BANDEIRA_VERDE, "correndo, a bandeira e verde");

    /* Guarda os setores da volta 2 quando ela termina */
    float setoresDaVolta2[TOTAL_SETORES] = { 0 };
    int quadros = 0;
    while (!todosChegaram(&corrida) && quadros < 60 * 60 * 10) {
        int voltasAntes = corrida.jogador.voltas;
        float copia[TOTAL_SETORES];
        for (int s = 0; s < TOTAL_SETORES; s++) {
            copia[s] = corrida.jogador.setoresDaVolta[s];
        }
        float s3Antes = corrida.jogador.inicioDoSetor;
        atualizarCorrida(&corrida, atualizarIA(&corrida.jogador, corrida.pista), quadro);
        if (voltasAntes == 1 && corrida.jogador.voltas == 2) {
            for (int s = 0; s < TOTAL_SETORES - 1; s++) {
                setoresDaVolta2[s] = copia[s];
            }
            setoresDaVolta2[TOTAL_SETORES - 1] = corrida.tempoDeCorrida - s3Antes;
        }
        quadros++;
    }

    Carro *jogador = &corrida.jogador;
    float soma = setoresDaVolta2[0] + setoresDaVolta2[1] + setoresDaVolta2[2];
    printf("     volta 2: %.2f s = S1 %.2f + S2 %.2f + S3 %.2f\n", jogador->temposDeVolta[1],
           setoresDaVolta2[0], setoresDaVolta2[1], setoresDaVolta2[2]);
    verificar(fabsf(soma - jogador->temposDeVolta[1]) < 0.05f, "os 3 setores somam o tempo da volta");

    float menor = jogador->temposDeVolta[0];
    for (int i = 1; i < TOTAL_VOLTAS; i++) {
        menor = fminf(menor, jogador->temposDeVolta[i]);
    }
    verificar(fabsf(jogador->melhorVolta - menor) < 0.001f, "a melhor volta do jogador e a menor das 3");

    float maisRapida = 0.0f;
    for (int i = 0; i < TOTAL_CARROS; i++) {
        float melhor = corrida.classificacao[i]->melhorVolta;
        if (maisRapida == 0.0f || melhor < maisRapida) {
            maisRapida = melhor;
        }
    }
    printf("     volta mais rapida da corrida: %.2f s\n", corrida.voltaMaisRapida);
    verificar(fabsf(corrida.voltaMaisRapida - maisRapida) < 0.001f, "a volta mais rapida e a menor de todos");
    verificar(jogador->temDiferenca, "depois da 1a volta existe diferenca para a melhor volta");

    verificar(intervaloEmSegundos(jogador, jogador) == 0.0f, "o intervalo do jogador para ele mesmo e zero");
    verificar(intervaloEmSegundos(corrida.classificacao[0], corrida.classificacao[TOTAL_CARROS - 1]) > 0.0f,
              "o lider esta a frente (+) do ultimo");
    verificar(bandeiraDaCorrida(&corrida) == BANDEIRA_QUADRICULADA, "no fim, bandeira quadriculada");

    liberarPista(pista);
}

/* ---------- Motor (rotação, marcha e carga para o som) ---------- */
/*
 * testarVoltaDaVitoria
 * Com o carro 2 (que, dirigido pela IA, vence esta corrida), confere:
 *   - a estimativa de quanto falta para a linha, usada pela música da vitória;
 *   - depois da vitória, a IA segue dirigindo o carro do jogador sem parar,
 *     e essas voltas extras não mexem nos tempos da corrida.
 */
static void testarVoltaDaVitoria(void) {
    printf("\n16) Chegada em 1o: musica no ponto certo e volta da vitoria\n");
    Pista *pista = criarPista();
    static Corrida corrida;
    iniciarCorrida(&corrida, pista, 2, NIVEL_MEDIO);
    corrida.jogador.velocidadeMaxima *= 1.05f;   /* um pouco melhor que a IA, para vencer o pelotão */
    float quadro = 1.0f / 60.0f;
    float estimativaA6s = -1.0f, tempoA6s = 0.0f;

    while (!corrida.terminou && corrida.tempoDeCorrida < 600.0f) {
        if (corrida.jogador.voltas == TOTAL_VOLTAS - 1) {
            corrida.jogador.nitro = 0.0f;   /* o nitro é imprevisível: a música corrige a diferença até a linha */
        }
        float estimativa = segundosAteAVitoria(&corrida);
        if (estimativaA6s < 0.0f && estimativa >= 0.0f && estimativa <= 6.0f) {
            estimativaA6s = estimativa;
            tempoA6s = corrida.tempoDeCorrida;
        }
        atualizarCorrida(&corrida, atualizarIA(&corrida.jogador, corrida.pista), quadro);
    }
    verificar(jogadorVenceu(&corrida), "o carro 2 dirigido pela IA (5% mais rapido) vence");
    float real = corrida.jogador.tempoDeChegada - tempoA6s;
    printf("     a ~6 s da linha: estimativa %.2f s, real %.2f s\n", estimativaA6s, real);
    verificar(estimativaA6s >= 0.0f && fabsf(estimativaA6s - real) < 0.3f, "estimativa ate a linha erra menos de 0,3 s");
    verificar(segundosAteAVitoria(&corrida) < 0.0f, "depois da chegada nao ha mais estimativa");

    float melhorVolta = corrida.jogador.melhorVolta;
    float tempos[TOTAL_VOLTAS];
    for (int i = 0; i < TOTAL_VOLTAS; i++) {
        tempos[i] = corrida.jogador.temposDeVolta[i];
    }
    Comandos nenhumComando = { 0, 0, 0, 0, 0.0f, 0, 0 };
    for (int i = 0; i < 60 * 130; i++) {
        atualizarCorrida(&corrida, nenhumComando, quadro);   /* o teclado nao manda mais */
    }
    verificar(corrida.jogador.voltas >= TOTAL_VOLTAS + 2, "a IA leva o vencedor por mais voltas");
    verificar(corrida.jogador.velocidade > corrida.jogador.velocidadeMaxima * 0.2f, "e o carro nao para");
    int temposIguais = (corrida.jogador.melhorVolta == melhorVolta);
    for (int i = 0; i < TOTAL_VOLTAS; i++) {
        temposIguais = temposIguais && (corrida.jogador.temposDeVolta[i] == tempos[i]);
    }
    verificar(temposIguais, "as voltas depois da bandeirada nao mudam os tempos");
    verificar(corrida.posicaoFinal == 1, "a posicao final continua 1");

    liberarPista(pista);
}

static void testarMotor(void) {
    printf("\n[Teste] Motor do jogador (rotacao, marchas e carga)\n");

    Carro carro = { 0 };
    carro.velocidadeMaxima = 12000.0f;
    Motor motor;
    iniciarMotor(&motor);
    float quadro = 1.0f / 60.0f;

    /* Parado e sem acelerar: marcha lenta, na 1ª, sem carga */
    for (int i = 0; i < 60; i++) {
        atualizarMotor(&motor, carro.velocidade / carro.velocidadeMaxima,
                       carro.ultimosComandos.acelerar && !carro.ultimosComandos.frear, 0, 0, quadro);
    }
    verificar(motor.marcha == 0 && motor.giro < 0.2f && motor.carga < 0.05f, "parado: 1a marcha, rotacao baixa, sem carga");

    /* Pé no fundo parado: a rotação sobe antes do carro andar */
    carro.ultimosComandos.acelerar = 1;
    for (int i = 0; i < 60; i++) {
        atualizarMotor(&motor, carro.velocidade / carro.velocidadeMaxima,
                       carro.ultimosComandos.acelerar && !carro.ultimosComandos.frear, 0, 0, quadro);
    }
    verificar(motor.giro > 0.45f && motor.carga > 0.9f, "pe no fundo parado: o motor sobe e pega carga");

    /* Arrancada: a velocidade sobe até a máxima, as marchas vão passando */
    int trocas = 0, quedas = 0, ultimaMarcha = motor.marcha, giroMaximo = 0;
    float giroAntes = motor.giro, maiorQueda = 0.0f;
    int marchaMontonica = 1;
    int trocaNaLinha = 1;   /* toda subida de marcha acontece com o ponteiro em ~8000 rpm */
    for (int i = 0; i < 60 * 14; i++) {
        carro.velocidade += (carro.velocidadeMaxima / 12.0f) * quadro;
        if (carro.velocidade > carro.velocidadeMaxima) {
            carro.velocidade = carro.velocidadeMaxima;
        }
        atualizarMotor(&motor, carro.velocidade / carro.velocidadeMaxima,
                       carro.ultimosComandos.acelerar && !carro.ultimosComandos.frear, 0, 0, quadro);
        if (motor.marcha != ultimaMarcha) {
            float rpmNaTroca = giroAntes * RPM_MAXIMO;
            if (rpmNaTroca < RPM_DA_TROCA - 50.0f || rpmNaTroca > RPM_DA_TROCA + 300.0f) {
                trocaNaLinha = 0;
            }
            if (motor.marcha < ultimaMarcha) {
                marchaMontonica = 0;
            }
            trocas++;
            ultimaMarcha = motor.marcha;
        }
        if (motor.giro < giroAntes - 0.01f) {
            quedas++;
            if (giroAntes - motor.giro > maiorQueda) {
                maiorQueda = giroAntes - motor.giro;
            }
        }
        if (motor.giro > 1.0f) {
            giroMaximo = 1;
        }
        giroAntes = motor.giro;
    }
    verificar(motor.marcha == TOTAL_MARCHAS - 1, "na velocidade maxima o motor esta na ultima marcha");
    verificar(trocas == TOTAL_MARCHAS - 1 && marchaMontonica, "acelerando, o cambio sobe uma marcha por vez, sem voltar");
    verificar(quedas > 0 && motor.giro > 0.8f, "a rotacao cai quando o cambio troca e volta a subir");
    verificar(trocaNaLinha, "o cambio sobe a marcha na linha vermelha (8000 rpm)");
    verificar(!giroMaximo, "a rotacao nunca passa de 1");

    /* Soltar o pé: a carga some e a rotação cai */
    carro.ultimosComandos.acelerar = 0;
    for (int i = 0; i < 30; i++) {
        atualizarMotor(&motor, carro.velocidade / carro.velocidadeMaxima,
                       carro.ultimosComandos.acelerar && !carro.ultimosComandos.frear, 0, 0, quadro);
    }
    verificar(motor.carga < 0.05f, "pe fora do acelerador: sem carga");

    /* Freando forte, o motor reduz as marchas até a 1ª */
    carro.ultimosComandos.frear = 1;
    for (int i = 0; i < 60 * 4; i++) {
        carro.velocidade -= 12000.0f * quadro;
        if (carro.velocidade < 0.0f) {
            carro.velocidade = 0.0f;
        }
        atualizarMotor(&motor, carro.velocidade / carro.velocidadeMaxima,
                       carro.ultimosComandos.acelerar && !carro.ultimosComandos.frear, 0, 0, quadro);
    }
    verificar(motor.marcha == 0 && motor.giro < 0.2f, "parado de novo: volta para a 1a e para a marcha lenta");
}

/*
 * testarPeVacuoEDerrapagem
 * As três regras que deixam a pilotagem mais rica:
 *   - acelerando na curva o carro é mais empurrado para fora que aliviando;
 *   - passando do limite o pneu arrasta: perde velocidade e volta a obedecer;
 *   - no vácuo a velocidade máxima sobe, e a corrida mede quem está nele.
 */
static void testarPeVacuoEDerrapagem(void) {
    printf("\n17) Pe na curva, derrapagem e vacuo\n");
    float quadro = 1.0f / 60.0f;
    Pista *curva = criarPistaVazia();
    adicionarTrecho(curva, "Curva longa", 400, CURVA_FECHADA, 0.0f, 0.0f);
    Comandos acelerar = { 1, 0, 0, 0, 0.0f, 0, 0 };
    Comandos aliviar = { 0, 0, 0, 0, 0.0f, 0, 0 };

    /* Pé: mesma velocidade, só muda o pedal */
    Carro noFundo = criarCarro("Teste", 0, curva->largada, 0.0f, 12000.0f, 2400.0f, 0.5f);
    noFundo.velocidade = 6000.0f;
    Carro aliviando = noFundo;
    atualizarCarro(&noFundo, acelerar, quadro);
    atualizarCarro(&aliviando, aliviar, quadro);
    verificar(fabsf(noFundo.deslocamentoLateral) > fabsf(aliviando.deslocamentoLateral),
              "acelerando na curva o carro e mais empurrado para fora que aliviando");
    Carro referencia = noFundo;
    verificar(velocidadeSeguraNaCurva(&referencia, CURVA_FECHADA)
              > velocidadeSeguraAcelerando(&referencia, CURVA_FECHADA),
              "o limite da curva e mais alto com o pe fora do acelerador");

    /* Derrapagem: 20% acima do limite, o carro perde mais do que o atrito */
    float segura = velocidadeSeguraNaCurva(&referencia, CURVA_FECHADA);
    Carro dentro = criarCarro("Teste", 0, curva->largada, 0.0f, 12000.0f, 2400.0f, 0.5f);
    dentro.velocidade = segura * 0.9f;
    Carro passou = dentro;
    passou.velocidade = segura * 1.2f;
    float antesDentro = dentro.velocidade, antesPassou = passou.velocidade;
    atualizarCarro(&dentro, aliviar, quadro);
    atualizarCarro(&passou, aliviar, quadro);
    float perdaDentro = antesDentro - dentro.velocidade;
    float perdaPassou = antesPassou - passou.velocidade;
    printf("     perda por quadro: %.0f dentro do limite, %.0f passando 20%%\n", perdaDentro, perdaPassou);
    verificar(perdaPassou > 2.0f * perdaDentro, "passando do limite o pneu arrasta e tira velocidade");

    /* ...e com o volante contra a curva, ele se recupera antes da grama */
    Comandos contraACurva = { 0, 0, 0, 1, 0.0f, 0, 0 };   /* curva para a direita empurra para a esquerda */
    int quadros = 0;
    while (passou.derrapagem >= 1.0f && quadros < 60 * 5) {
        atualizarCarro(&passou, contraACurva, quadro);
        quadros++;
    }
    printf("     recuperou em %.2f s, a %.2f da borda\n", quadros * quadro, passou.deslocamentoLateral);
    verificar(passou.derrapagem < 1.0f && !estaForaDaPista(&passou),
              "segurando o volante, o carro volta a obedecer sem ir para a grama");

    /* Vácuo: na reta, quem está no vácuo passa da máxima da ficha */
    Pista *reta = criarPistaVazia();
    adicionarTrecho(reta, "Reta", 50, RETA, 0.0f, 0.0f);
    Carro arLimpo = criarCarro("Teste", 0, reta->largada, 0.0f, 12000.0f, 2400.0f, 0.5f);
    arLimpo.velocidade = arLimpo.velocidadeMaxima;
    Carro naEsteira = arLimpo;
    naEsteira.vacuo = 1.0f;
    for (int i = 0; i < 60 * 10; i++) {
        atualizarCarro(&arLimpo, acelerar, quadro);
        atualizarCarro(&naEsteira, acelerar, quadro);
    }
    printf("     maxima: %d km/h no ar limpo, %d km/h no vacuo\n",
           velocidadeEmKmh(&arLimpo), velocidadeEmKmh(&naEsteira));
    verificar(arLimpo.velocidade <= arLimpo.velocidadeMaxima, "no ar limpo o carro nao passa da maxima");
    verificar(naEsteira.velocidade > naEsteira.velocidadeMaxima * 1.04f, "no vacuo ele passa da maxima");

    /* Saindo do vácuo, o excesso some aos poucos (sem tranco) */
    naEsteira.vacuo = 0.0f;
    float antes = naEsteira.velocidade;
    atualizarCarro(&naEsteira, acelerar, quadro);
    verificar(naEsteira.velocidade > naEsteira.velocidadeMaxima && naEsteira.velocidade < antes,
              "saindo do vacuo a velocidade cai aos poucos");

    /* Na corrida: colado atrás de um rival, na mesma linha, pega vácuo */
    Pista *pista = criarPista();
    static Corrida corrida;
    iniciarCorrida(&corrida, pista, 0, NIVEL_MEDIO);
    corrida.contagemRegressiva = 0.0f;
    Carro *rival = &corrida.rivais[0];
    Carro *jogador = &corrida.jogador;
    moverNaPista(&rival->posicao, 20000.0f);   /* os dois bem à frente do resto do grid */
    rival->distanciaPercorrida += 20000.0f;
    jogador->posicao = rival->posicao;
    moverNaPista(&jogador->posicao, -1000.0f);
    jogador->distanciaPercorrida = rival->distanciaPercorrida - 1000.0f;
    jogador->deslocamentoLateral = rival->deslocamentoLateral;
    for (int i = 0; i < 20; i++) {
        atualizarCorrida(&corrida, aliviar, quadro);
    }
    printf("     vacuo do jogador a ~1000 unidades do rival: %.2f\n", jogador->vacuo);
    verificar(jogador->vacuo > 0.3f, "colado atras de um rival, o jogador pega vacuo");
    jogador->deslocamentoLateral = rival->deslocamentoLateral + 0.8f;
    for (int i = 0; i < 30; i++) {
        atualizarCorrida(&corrida, aliviar, quadro);
    }
    verificar(jogador->vacuo == 0.0f, "saindo de lado, o vacuo acaba");

    liberarPista(pista);
    liberarPista(reta);
    liberarPista(curva);
}

/*
 * testarFreioAlivioECambio
 *   - o freio é forte, mas não instantâneo: de 300 a 100 km/h leva
 *     mais de 50 m (as placas de 150/100/50 m servem para alguma coisa);
 *   - tirar o pé perde bem menos que frear;
 *   - todo carro tem câmbio, e na troca a potência é cortada;
 *   - a conta da IA para começar a frear bate com a frenagem de verdade.
 */
static void testarFreioAlivioECambio(void) {
    printf("\n18) Freio, alivio e cambio no carro\n");
    float quadro = 1.0f / 60.0f;
    Pista *reta = criarPistaVazia();
    adicionarTrecho(reta, "Reta", 400, RETA, 0.0f, 0.0f);
    Comandos frear = { 0, 1, 0, 0, 0.0f, 0, 0 };
    Comandos aliviar = { 0, 0, 0, 0, 0.0f, 0, 0 };
    Comandos acelerar = { 1, 0, 0, 0, 0.0f, 0, 0 };

    /* Freio: de 300 a 100 km/h */
    Carro freando = criarCarro("Teste", 0, reta->largada, 0.0f, 12000.0f, 2400.0f, 0.5f);
    freando.velocidade = 12000.0f;
    float inicio = freando.distanciaPercorrida;
    while (velocidadeEmKmh(&freando) > 100) {
        atualizarCarro(&freando, frear, quadro);
    }
    float metros = (freando.distanciaPercorrida - inicio) / UNIDADES_POR_METRO;
    printf("     de 300 a 100 km/h freando: %.0f m\n", metros);
    verificar(metros > 50.0f && metros < 120.0f, "frear de 300 a 100 km/h leva de 50 a 120 m");

    /* Alívio: um segundo sem pedal a 300 km/h */
    Carro aliviando = criarCarro("Teste", 0, reta->largada, 0.0f, 12000.0f, 2400.0f, 0.5f);
    aliviando.velocidade = 12000.0f;
    Carro comFreio = aliviando;
    for (int i = 0; i < 60; i++) {
        atualizarCarro(&aliviando, aliviar, quadro);
        atualizarCarro(&comFreio, frear, quadro);
    }
    printf("     1 s a partir de 300 km/h: sem pedal %d km/h, freando %d km/h\n",
           velocidadeEmKmh(&aliviando), velocidadeEmKmh(&comFreio));
    verificar(12000.0f - aliviando.velocidade < (12000.0f - comFreio.velocidade) / 4.0f,
              "tirar o pe perde menos de 1/4 do que o freio");
    verificar(velocidadeEmKmh(&aliviando) > 260, "tirando o pe a 300 km/h, ainda passa de 260 km/h 1 s depois");

    /* Câmbio: o carro troca de marcha sozinho e, na troca, acelera menos */
    Carro arrancando = criarCarro("Teste", 0, reta->largada, 0.0f, 12000.0f, 2400.0f, 0.5f);
    float ganhoEngatado = 0.0f, ganhoNaTroca = 0.0f;
    for (int i = 0; i < 60 * 4; i++) {
        float antes = arrancando.velocidade;
        atualizarCarro(&arrancando, acelerar, quadro);
        float ganho = arrancando.velocidade - antes;
        if (estaTrocandoDeMarcha(&arrancando.motor)) {
            ganhoNaTroca = ganho;
        } else if (ganhoNaTroca == 0.0f) {
            ganhoEngatado = ganho;   /* o último quadro engatado antes da primeira troca */
        }
    }
    printf("     4 s acelerando: %da marcha; ganho por quadro %.0f engatado, %.0f na troca\n",
           arrancando.motor.marcha + 1, ganhoEngatado, ganhoNaTroca);
    verificar(arrancando.motor.marcha >= 2, "o carro troca de marcha sozinho");
    verificar(ganhoNaTroca > 0.0f && ganhoNaTroca < ganhoEngatado * 0.5f, "na troca de marcha a potencia e cortada");

    /* A conta da IA: começando na velocidade calculada, chega na alvo bem no fim da distância */
    float alvo = 4000.0f, distancia = 6000.0f;
    Carro planejado = criarCarro("Teste", 0, reta->largada, 0.0f, 12000.0f, 2400.0f, 0.5f);
    planejado.velocidade = velocidadeParaFrearAte(alvo, distancia);
    inicio = planejado.distanciaPercorrida;
    while (planejado.distanciaPercorrida - inicio < distancia) {
        atualizarCarro(&planejado, frear, quadro);
    }
    printf("     freando de %d km/h por 50 m: chega a %d km/h (alvo %d)\n",
           converterParaKmh(velocidadeParaFrearAte(alvo, distancia)), velocidadeEmKmh(&planejado),
           converterParaKmh(alvo));
    verificar(fabsf(planejado.velocidade - alvo) < 200.0f, "a velocidade para frear ate a curva bate com o freio");

    /* Os rivais também têm câmbio */
    Pista *pista = criarPista();
    static Corrida corrida;
    iniciarCorrida(&corrida, pista, 0, NIVEL_MEDIO);
    for (int i = 0; i < 60 * 15; i++) {
        atualizarCorrida(&corrida, aliviar, quadro);
    }
    int todosTrocaram = 1;
    for (int r = 0; r < TOTAL_RIVAIS; r++) {
        if (corrida.rivais[r].motor.marcha < 2) {
            todosTrocaram = 0;
        }
    }
    verificar(todosTrocaram, "os rivais tambem trocam de marcha");

    liberarPista(pista);
    liberarPista(reta);
}

/*
 * testarPistasNoMundo
 * Cada ponto da pista tem um lugar no mundo 3D, nas duas pistas, e o oval
 * segue o traçado medido no modelo: fecha, tem muro e a IA corre nele.
 */
static void testarPistasNoMundo(void) {
    printf("\n19) As pistas no mundo 3D\n");
    float quadro = 1.0f / 60.0f;

    for (int id = 0; id < TOTAL_PISTAS; id++) {
        Pista *pista = criarPistaPorId((IdPista) id);
        int total = contarSegmentos(pista);
        float maiorPasso = 0.0f;
        Segmento *atual = pista->largada;
        do {
            PosicaoNaPista daqui = { atual, 0.0f }, dali = { atual->proximo, 0.0f };
            PontoNoMundo a = pontoNoMundo(daqui, 0.0f), b = pontoNoMundo(dali, 0.0f);
            float passo = sqrtf((b.x - a.x) * (b.x - a.x) + (b.z - a.z) * (b.z - a.z));
            maiorPasso = fmaxf(maiorPasso, passo);
            atual = proximoSegmento(atual);
        } while (atual != pista->largada);
        printf("     %s: %d segmentos, %.0f m; maior passo entre vizinhos %.2f m\n", pista->nome, total,
               total * COMPRIMENTO_SEGMENTO / UNIDADES_POR_METRO, maiorPasso);
        verificar((int) pista->id == id, "a pista sabe quem ela e");
        verificar(maiorPasso < 2.0f, "a pista e continua no mundo, inclusive na emenda da volta");

        /* O lado direito fica mesmo à direita de quem anda */
        PosicaoNaPista largada = { pista->largada, 0.0f };
        PosicaoNaPista frente = { pista->largada->proximo, 0.0f };
        PontoNoMundo centro = pontoNoMundo(largada, 0.0f), direita = pontoNoMundo(largada, 1.0f), adiante = pontoNoMundo(frente, 0.0f);
        float vira = (adiante.x - centro.x) * (direita.z - centro.z) - (adiante.z - centro.z) * (direita.x - centro.x);
        verificar(vira > 0.0f, "lateral positiva e a direita do sentido da corrida");
        liberarPista(pista);
    }

    Pista *oval = criarCopaPistao();
    verificar(fabsf(contarSegmentos(oval) * COMPRIMENTO_SEGMENTO / UNIDADES_POR_METRO - COMPRIMENTO_COPA_PISTAO) < 15.0f,
              "o oval tem o comprimento de Daytona: 4.023 m");
    float soma = 0.0f, menorAltura = 1e9f, maiorAltura = -1e9f, maiorInclinacao = 0.0f, menorLargura = 1e9f, maiorLargura = 0.0f;
    int soEsquerda = 1, comMuro = 1, comZebra = 0, curvas = 0;
    float maiorCurva = 0.0f, maiorDireita = 0.0f;
    Segmento *atual = oval->largada;
    do {
        soma += atual->curvatura;
        menorAltura = fminf(menorAltura, atual->altura);
        maiorAltura = fmaxf(maiorAltura, atual->altura);
        PosicaoNaPista aqui = { atual, 0.0f };
        maiorInclinacao = fmaxf(maiorInclinacao, pontoNoMundo(aqui, 0.6f).inclinacao);
        menorLargura = fminf(menorLargura, atual->larguraDireita + atual->larguraEsquerda);
        maiorLargura = fmaxf(maiorLargura, atual->larguraDireita + atual->larguraEsquerda);
        maiorCurva = fminf(maiorCurva, atual->curvatura);
        maiorDireita = fmaxf(maiorDireita, atual->curvatura);
        soEsquerda = soEsquerda && atual->curvatura < 0.3f;
        comMuro = comMuro && atual->temMuro;
        comZebra += temZebra(atual);
        curvas += (atual->nomeTrecho != proximoSegmento(atual)->nomeTrecho
                   && strncmp(proximoSegmento(atual)->nomeTrecho, "Curva", 5) == 0);
        atual = proximoSegmento(atual);
    } while (atual != oval->largada);
    printf("     oval: largura de %.1f a %.1f m, inclinacao ate %.1f graus, curva mais fechada %.2f, para a direita ate %.2f, %d curvas\n",
           menorLargura, maiorLargura, atanf(maiorInclinacao) * 57.3f, maiorCurva, maiorDireita, curvas);
    verificar(fabsf(soma - GRAUS_VOLTA_COMPLETA) < 1.0f, "o oval fecha: -360 graus, anti-horario");
    verificar(soEsquerda, "o oval so vira para a esquerda");
    verificar(comMuro && comZebra == 0, "muro na volta inteira, e sem zebra");
    verificar(curvas == 4, "as 4 curvas tem nome");
    static const char *ORDEM[] = { "Reta dos Boxes", "Curva 1", "Reta Curta", "Curva 2", "Reta Oposta", "Curva 3", "Reta Curta", "Curva 4", "Reta dos Boxes" };
    int noTrecho = 0, naOrdem = 1;
    atual = oval->largada;
    do {
        if (strcmp(atual->nomeTrecho, ORDEM[noTrecho]) != 0) {
            noTrecho++;
            naOrdem = naOrdem && noTrecho < 9 && strcmp(atual->nomeTrecho, ORDEM[noTrecho]) == 0;
        }
        atual = proximoSegmento(atual);
    } while (atual != oval->largada && naOrdem);
    verificar(naOrdem && noTrecho == 8, "os trechos do oval vem na ordem certa");
    verificar(maiorInclinacao > 0.05f, "as curvas sao inclinadas, como no modelo");

    /* O chão: bate com pontoNoMundo e não dá degraus ao longo da pista nem na largura */
    float maiorDegrau = 0.0f, maiorDiferenca = 0.0f;
    atual = oval->largada;
    do {
        PosicaoNaPista aqui = { atual, 0.0f }, dali = { atual->proximo, 0.0f };
        for (float lateral = -0.95f; lateral <= 0.95f; lateral += 0.05f) {
            maiorDegrau = fmaxf(maiorDegrau, fabsf(pontoNoMundo(dali, lateral).y - pontoNoMundo(aqui, lateral).y));
            float metros = lateral * larguraDoLado(atual, lateral);
            maiorDiferenca = fmaxf(maiorDiferenca, fabsf(alturaDoChao(aqui, metros) - pontoNoMundo(aqui, lateral).y));
        }
        atual = proximoSegmento(atual);
    } while (atual != oval->largada);
    printf("     chao: maior degrau entre segmentos vizinhos %.3f m\n", maiorDegrau);
    verificar(maiorDegrau < 0.5f, "o chao nao da degraus ao longo da pista");
    verificar(maiorDiferenca < 0.001f, "alturaDoChao e pontoNoMundo concordam");

    /* Muro dos dois lados: o carro jogado contra ele para ali e perde velocidade */
    for (int lado = 0; lado < 2; lado++) {
        Comandos virando = { 1, 0, lado == 0, lado == 1, 0.0f, 0, 0 };
        Carro carro = criarCarro("Teste", 0, oval->largada, 0.0f, 12000.0f, 2400.0f, 0.5f);
        carro.velocidade = 8000.0f;
        float antes = 0.0f;
        for (int i = 0; i < 60; i++) {
            antes = carro.velocidade;
            atualizarCarro(&carro, virando, quadro);
        }
        Segmento *ali = carro.posicao.segmento;
        float largura = (carro.deslocamentoLateral > 0.0f) ? ali->larguraDireita : ali->larguraEsquerda;
        printf("     contra o muro: sobra %.1f m do centro do carro ate ele\n", largura * (1.0f - fabsf(carro.deslocamentoLateral)));
        verificar(largura * (1.0f - fabsf(carro.deslocamentoLateral)) > 1.3f, "o carro para antes do muro, sem entrar nele");
        verificar(carro.velocidade < antes, "raspar no muro custa velocidade");
    }

    /* A IA corre o oval inteiro */
    static Corrida corrida;
    iniciarCorrida(&corrida, oval, 0, NIVEL_MEDIO);
    while (!todosChegaram(&corrida) && corrida.tempoDeCorrida < 600.0f) {
        atualizarCorrida(&corrida, atualizarIA(&corrida.jogador, corrida.pista), quadro);
    }
    printf("     volta mais rapida %.2f s; 1o em %.2f s, ultimo em %.2f s\n", corrida.voltaMaisRapida,
           corrida.classificacao[0]->tempoDeChegada, corrida.classificacao[TOTAL_CARROS - 1]->tempoDeChegada);
    verificar(todosChegaram(&corrida), "todos os carros completam as 3 voltas no oval");
    liberarPista(oval);
}

/*
 * testarDepoisDaChegada
 * Em qualquer posição e nas duas pistas, a IA leva o carro depois da chegada.
 */
static void testarDepoisDaChegada(void) {
    printf("\n22) Depois da chegada, a IA segue com o carro\n");
    float quadro = 1.0f / 60.0f;
    static Corrida corrida;
    /* Com habilidade baixa o piloto não vence; com ritmo perfeito e 5% a mais de carro, vence */
    struct { int carro; float habilidade; float ganho; int vence; } casos[2] = { { 7, 0.80f, 1.0f, 0 }, { 1, 1.0f, 1.05f, 1 } };

    for (int id = 0; id < TOTAL_PISTAS; id++) {
        Pista *pista = criarPistaPorId((IdPista) id);
        for (int k = 0; k < 2; k++) {
            iniciarCorrida(&corrida, pista, casos[k].carro, NIVEL_MEDIO);
            corrida.jogador.habilidade = casos[k].habilidade;
            corrida.jogador.velocidadeMaxima *= casos[k].ganho;
            while (!corrida.terminou && corrida.tempoDeCorrida < 600.0f) {
                atualizarCorrida(&corrida, atualizarIA(&corrida.jogador, corrida.pista), quadro);
            }
            int posicao = corrida.posicaoFinal;
            float chegada = corrida.jogador.tempoDeChegada;
            float andouAntes = corrida.jogador.distanciaPercorrida;
            int voltasAntes = corrida.jogador.voltas;

            Comandos semTeclado = { 0, 0, 0, 0, 0.0f, 0, 0 };
            float menor = 1e9f, depoisDaRodada = 10.0f;
            for (int i = 0; i < 60 * 150; i++) {
                atualizarCorrida(&corrida, semTeclado, quadro);
                /* Rodando depois de um toque, ele para um instante, e leva um pouco para embalar de novo */
                depoisDaRodada = corrida.jogador.rodando ? 0.0f : depoisDaRodada + quadro;
                if (depoisDaRodada > 1.0f) {
                    menor = fminf(menor, corrida.jogador.velocidade);
                }
            }
            float metros = (corrida.jogador.distanciaPercorrida - andouAntes) / UNIDADES_POR_METRO;
            printf("     %s, %do lugar: %.0f m em 150 s, menor velocidade %d km/h\n", pista->nome, posicao, metros, converterParaKmh(menor));
            if (id == PISTA_SPLAT && casos[k].vence) {
                /* Curvas fechadas e pista estreita: uma corrida só depende demais das batidas. Quem
                 * mede o ritmo no Splat é a média de 25 corridas, mais adiante, no teste do ritmo perfeito */
                verificar(posicao >= 1 && posicao <= TOTAL_CARROS, "a corrida termina com uma posicao valida");
            } else {
                verificar(jogadorVenceu(&corrida) == casos[k].vence, casos[k].vence ? "este caso e uma vitoria" : "este caso nao e uma vitoria");
            }
            verificar(menor > 0.0f && metros > 3000.0f, "o carro continua andando, sem parar nunca");
            verificar(corrida.jogador.voltas > voltasAntes, "e segue dando voltas");
            verificar(corrida.posicaoFinal == posicao && corrida.jogador.tempoDeChegada == chegada,
                      "sem mudar a posicao nem o tempo de chegada");
        }
        liberarPista(pista);
    }
}

/*
 * ladoALado
 * Põe o jogador e um rival numa reta, longe do resto, e separa os dois.
 * Recebe: a corrida, a pista, o rival, quantos metros o jogador fica à frente dele e a lateral dos dois, em metros.
 * Devolve: nada.
 */
static void ladoALado(Corrida *corrida, Pista *pista, Carro *rival, float aFrente, float lateralJogador, float lateralRival, float velocidadeJogador, float velocidadeRival) {
    iniciarCorrida(corrida, pista, 0, NIVEL_MEDIO);
    corrida->contagemRegressiva = 0.0f;
    for (int r = 0; r < TOTAL_RIVAIS; r++) {   /* os outros, parados bem longe */
        Carro *outro = &corrida->rivais[r];
        if (outro != rival) {
            moverNaPista(&outro->posicao, 200000.0f + r * 2000.0f);
            outro->distanciaPercorrida += 200000.0f + r * 2000.0f;
            outro->deslocamentoLateral = 0.0f;
            outro->velocidadeMaxima = 1.0f;
        }
    }
    Segmento *reta = buscarSegmento(pista, 600);
    PosicaoNaPista lugar = { reta, 0.0f };
    rival->posicao = lugar;
    rival->distanciaPercorrida = 20000.0f;
    corrida->jogador.posicao = lugar;
    moverNaPista(&corrida->jogador.posicao, aFrente * UNIDADES_POR_METRO);
    corrida->jogador.distanciaPercorrida = 20000.0f + aFrente * UNIDADES_POR_METRO;
    corrida->jogador.deslocamentoLateral = lateralJogador / larguraDoLado(reta, lateralJogador);
    rival->deslocamentoLateral = lateralRival / larguraDoLado(reta, lateralRival);
    rival->linhaAlvo = lateralRival;
    corrida->jogador.velocidade = velocidadeJogador;
    rival->velocidade = velocidadeRival;
}

/*
 * testarColisaoEEquilibrio
 * O carro é sólido por inteiro, a batida fora do centro faz rodar, e os carros
 * têm o mesmo ritmo: quem decide a corrida é o piloto.
 */
static void testarColisaoEEquilibrio(void) {
    printf("\n23) Colisao, rodada e equilibrio\n");
    float quadro = 1.0f / 60.0f;
    Pista *pista = criarPista();
    static Corrida corrida;
    Carro *rival = &corrida.rivais[0];
    float f, l;

    /* O caso do jogo: um pouquinho à frente, jogando o carro para cima do outro */
    ladoALado(&corrida, pista, rival, 1.0f, -4.0f, 0.0f, 6000.0f, 6000.0f);
    Comandos paraADireita = { 1, 0, 0, 1, 0.0f, 0, 0 };
    float maiorEntrada = 0.0f;
    int ladoTrocado = 0;
    for (int i = 0; i < 90; i++) {
        atualizarCorrida(&corrida, paraADireita, quadro);
        if (penetracaoEntreCarros(&corrida, &corrida.jogador, rival, &f, &l)) {
            maiorEntrada = fmaxf(maiorEntrada, fminf(f, l));
        }
        ladoTrocado = ladoTrocado || (corrida.jogador.deslocamentoLateral > rival->deslocamentoLateral && fabsf(rival->distanciaPercorrida - corrida.jogador.distanciaPercorrida) < 120.0f);
    }
    printf("     um pouco a frente, virando para cima dele: maior entrada %.2f m\n", maiorEntrada);
    verificar(!ladoTrocado, "um pouco a frente, nao atravessa o carro do lado");
    verificar(rival->deslocamentoLateral > 0.03f, "e empurra o outro para o lado");
    verificar(maiorEntrada < 0.6f, "as caixas quase nao entram uma na outra");

    /* Por trás, de centro a centro: empurra, não atravessa e ninguém roda */
    ladoALado(&corrida, pista, rival, -12.0f, 0.0f, 0.0f, 9000.0f, 3000.0f);
    rival->velocidadeMaxima = 3000.0f;
    Comandos reto = { 1, 0, 0, 0, 0.0f, 0, 0 };
    float antes = 9000.0f;
    int passou = 0;
    int primeiroContato = -1;     /* o quadro da batida de centro a centro */
    int rodouNoPrimeiroContato = 0;
    for (int i = 0; i < 90; i++) {
        atualizarCorrida(&corrida, reto, quadro);
        /* Só a primeira batida é de centro a centro: depois dela o rival se afasta da linha e,
         * se o jogador o alcança de novo, a segunda batida já é fora do centro e pode fazê-lo rodar */
        if (primeiroContato < 0 && corrida.jogador.velocidade < antes - 500.0f) {
            primeiroContato = i;
        }
        if (primeiroContato >= 0 && i == primeiroContato + 3) {
            rodouNoPrimeiroContato = rival->rodando || corrida.jogador.rodando;
        }
        /* Atravessar é cruzar o outro carro com as caixas alinhadas; ultrapassar pelo lado e ele voltar à linha depois não conta */
        passou = passou || (corrida.jogador.distanciaPercorrida > rival->distanciaPercorrida
                            && corrida.jogador.distanciaPercorrida < rival->distanciaPercorrida + rival->comprimento * UNIDADES_POR_METRO
                            && fabsf(corrida.jogador.deslocamentoLateral - rival->deslocamentoLateral) < 0.05f);
    }
    verificar(!passou, "batendo por tras, de frente, nao passa por dentro");
    verificar(corrida.jogador.velocidade < antes, "e perde velocidade");
    verificar(primeiroContato >= 0 && !rodouNoPrimeiroContato, "bem no centro, ninguem roda");

    /* Por trás, na quina: quem está na frente roda, perde velocidade e depois volta ao normal */
    ladoALado(&corrida, pista, rival, -6.0f, 1.4f, 0.0f, 10000.0f, 5000.0f);
    rival->velocidadeMaxima = 5000.0f;
    int rodou = 0;
    float velocidadeNaBatida = 0.0f;
    for (int i = 0; i < 40 && !rodou; i++) {
        atualizarCorrida(&corrida, reto, quadro);
        rodou = rival->rodando;
        velocidadeNaBatida = rival->velocidade;
    }
    verificar(rodou, "batendo na quina da traseira, o outro roda");
    int comandosIgnorados = 1;
    float menorVelocidade = velocidadeNaBatida, maiorGiro = 0.0f;
    int quadros = 0;
    Comandos solto = { 0, 0, 0, 0, 0.0f, 0, 0 };
    corrida.jogador.deslocamentoLateral = -0.9f;   /* o jogador sai da frente */
    while (rival->rodando && quadros < 60 * 8) {
        atualizarCorrida(&corrida, solto, quadro);
        menorVelocidade = fminf(menorVelocidade, rival->velocidade);
        maiorGiro = fmaxf(maiorGiro, fabsf(rival->giro));
        comandosIgnorados = comandosIgnorados && !rival->ultimosComandos.acelerar;
        quadros++;
    }
    printf("     a rodada: %.1f s, girou %.0f graus, de %d para %d km/h\n", quadros * quadro, maiorGiro, converterParaKmh(velocidadeNaBatida), converterParaKmh(menorVelocidade));
    verificar(maiorGiro > 180.0f, "o carro gira de verdade");
    verificar(comandosIgnorados, "rodando, ninguem controla o carro");
    verificar(menorVelocidade < velocidadeNaBatida * 0.8f && menorVelocidade > 0.0f, "perde velocidade, mas nao para de vez");
    verificar(!rival->rodando && rival->giro == 0.0f, "no fim ele se endireita sozinho");
    fazerRodar(rival, 800.0f);
    verificar(!rival->rodando, "logo depois de rodar, nao roda de novo");

    /* A caixa acompanha o carro virado */
    Carro deLado = criarCarro("Teste", 0, pista->largada, 0.0f, 12000.0f, 2400.0f, 0.5f);
    float meioNormal, larguraNormal, meioVirado, larguraVirado;
    meiasMedidasDoCarro(&deLado, &meioNormal, &larguraNormal);
    deLado.giro = 90.0f;
    meiasMedidasDoCarro(&deLado, &meioVirado, &larguraVirado);
    verificar(fabsf(meioVirado - larguraNormal) < 0.01f && fabsf(larguraVirado - meioNormal) < 0.01f, "de lado, a caixa troca comprimento por largura");

    /* Equilíbrio: a mesma volta com qualquer carro */
    Pista *pistas[TOTAL_PISTAS] = { pista, criarCopaPistao(), criarOval(), criarSplat() };
    for (int id = 0; id < TOTAL_PISTAS; id++) {
        float menor = 1e9f, maior = 0.0f;
        for (int n = 0; n < TOTAL_CARROS; n++) {
            const FichaTecnica *ficha = fichaDoCarro(n);
            Carro carro = criarCarro(ficha->nome, n, pistas[id]->largada, 0.0f, ficha->velocidadeMaxima, ficha->aceleracao, ficha->aderencia);
            float tempo = 0.0f, inicio = 0.0f;
            int voltas = 0;
            while (voltas < 2 && tempo < 300.0f) {
                tempo += quadro;
                if (atualizarCarro(&carro, atualizarIA(&carro, pistas[id]), quadro) > 0) {
                    voltas++;
                    inicio = (voltas == 1) ? tempo : inicio;
                }
            }
            menor = fminf(menor, tempo - inicio);
            maior = fmaxf(maior, tempo - inicio);
        }
        printf("     %s: volta limpa de %.2f a %.2f s\n", pistas[id]->nome, menor, maior);
        verificar(maior - menor < fmaxf(1.1f, 0.0275f * maior), "a diferenca entre o melhor e o pior carro e pequena");
    }

    /* Quem pilota melhor chega na frente, com qualquer carro; corrida limpa entre os rivais */
    for (int id = 0; id < TOTAL_PISTAS; id++) {
        int melhor = 0, pior = 0, pioresChegadas = 0;
        /* Cada carro em 5 ritmos de quadro: uma rodada num toque de azar não decide o teste */
        static const float QUADROS[5] = { 1.0f / 60.0f, 1.0f / 59.0f, 1.0f / 61.0f, 1.0f / 58.0f, 1.0f / 62.0f };
        int corridas = 0, longeDaFrente = 0;
        for (int n = 0; n < TOTAL_CARROS; n += 3) {
            for (int q = 0; q < 5; q++) {
                iniciarCorrida(&corrida, pistas[id], n, NIVEL_MEDIO);
                while (!corrida.terminou && corrida.tempoDeCorrida < 600.0f) {
                    atualizarCorrida(&corrida, atualizarIA(&corrida.jogador, corrida.pista), QUADROS[q]);
                }
                melhor += corrida.posicaoFinal;
                pior = (corrida.posicaoFinal > pior) ? corrida.posicaoFinal : pior;
                longeDaFrente += corrida.posicaoFinal > 5;
                corridas++;
            }

            iniciarCorrida(&corrida, pistas[id], n, NIVEL_MEDIO);
            corrida.jogador.habilidade = 0.80f;
            while (!corrida.terminou && corrida.tempoDeCorrida < 600.0f) {
                atualizarCorrida(&corrida, atualizarIA(&corrida.jogador, corrida.pista), quadro);
            }
            pioresChegadas += corrida.posicaoFinal;
        }
        printf("     %s, ritmo perfeito: media %.1f; %d de %d corridas fora dos 5 primeiros\n", pistas[id]->nome, (float) melhor / corridas, longeDaFrente, corridas);
        if (id == PISTA_SPLAT) {
            /* Curvas fechadas e pista estreita: largar atrás pesa mais que nas outras */
            verificar(melhor < 7 * corridas && longeDaFrente * 2 <= corridas, "com ritmo perfeito se chega perto da frente com qualquer carro");
        } else {
            verificar(melhor < 5 * corridas && longeDaFrente * 4 <= corridas, "com ritmo perfeito se chega na frente com qualquer carro (ao menos 3 em cada 4 corridas)");
        }
        verificar(pioresChegadas * 5 > melhor, "ritmo fraco chega mais atras, em media");
    }

    liberarPista(pistas[1]);
    liberarPista(pista);
}

/*
 * testarRetaDosBoxes
 * Interlagos: a reta tem muro, o funil não dá tranco, e as árvores só ficam nos bosques.
 */
static void testarRetaDosBoxes(void) {
    printf("\n25) Reta dos boxes de Interlagos\n");
    float quadro = 1.0f / 60.0f;
    Pista *pista = criarPista();
    int comMuro = 0, zebraNoMuro = 0, arvoresNaReta = 0;
    float maiorDegrau = 0.0f;
    Segmento *atual = pista->largada;
    do {
        comMuro += atual->temMuro;
        zebraNoMuro += (atual->temMuro && temZebra(atual));
        if (atual->temMuro) {
            maiorDegrau = fmaxf(maiorDegrau, fabsf(atual->lugarDoMuro - proximoSegmento(atual)->lugarDoMuro));
        }
        for (Obstaculo *o = atual->obstaculos; o != NULL; o = o->proximo) {
            arvoresNaReta += (o->tipo == OBJETO_ARVORE && strcmp(atual->nomeTrecho, "Reta dos Boxes") == 0);
        }
        atual = proximoSegmento(atual);
    } while (atual != pista->largada);
    printf("     %.0f m de muro\n", comMuro / SEGMENTOS_POR_METRO);
    verificar(pista->largada->temMuro && comMuro > 0, "a linha de chegada fica entre muros");
    verificar(zebraNoMuro == 0, "no trecho de muro nao ha zebra");
    verificar(maiorDegrau < 0.1f, "o muro abre em funil, sem degrau");
    verificar(arvoresNaReta == 0, "nenhuma arvore na reta dos boxes");

    /* Nenhuma arquibancada, da frente ao fundo, fica onde o carro consegue chegar */
    int totalTabela, invadindo = 0;
    const Arquibancada *tabela = arquibancadasDaPista(&totalTabela);
    for (int a = 0; a < totalTabela; a++) {
        float fundo = 2.8f + tabela[a].fileiras * 0.85f;
        for (int i = (int) (tabela[a].inicio * SEGMENTOS_POR_METRO); i <= (int) (tabela[a].fim * SEGMENTOS_POR_METRO); i += 4) {
            PosicaoNaPista lugar = { buscarSegmento(pista, i), 0.0f };
            int invadiu = 0;
            for (int parte = 0; parte <= 2; parte++) {
                PontoNoMundo ponto = pontoNoMundo(lugar, (tabela[a].distancia + fundo * parte / 2.0f) / lugar.segmento->larguraDireita);
                Segmento *outro = pista->largada;
                do {
                    float alcance = outro->larguraDireita * (outro->temMuro ? outro->lugarDoMuro : 2.5f);
                    invadiu |= hypotf(outro->xMundo - ponto.x, outro->zMundo - ponto.z) < alcance;
                    outro = proximoSegmento(outro);
                } while (outro != pista->largada);
            }
            if (invadiu) {
                printf("     setor %c invade em %.0f m\n", tabela[a].setor ? tabela[a].setor : '-', i / SEGMENTOS_POR_METRO);
                invadindo++;
            }
        }
    }
    verificar(invadindo == 0, "nenhuma arquibancada fica onde o carro chega");

    /* O carro não sai da reta, para os dois lados */
    for (int lado = 0; lado < 2; lado++) {
        Comandos virando = { 1, 0, lado == 0, lado == 1, 0.0f, 0, 0 };
        Carro carro = criarCarro("Teste", 0, buscarSegmento(pista, 2500), 0.0f, 12000.0f, 2400.0f, 0.5f);
        carro.velocidade = 8000.0f;
        for (int i = 0; i < 60; i++) {
            atualizarCarro(&carro, virando, quadro);
        }
        verificar(fabsf(carro.deslocamentoLateral) < 1.0f && !estaForaDaPista(&carro), "o muro segura o carro na reta");
    }

    /* Vindo pela grama, ele entra no funil e volta para o asfalto, sem salto */
    Carro pelaGrama = criarCarro("Teste", 0, buscarSegmento(pista, (int) (3560 * SEGMENTOS_POR_METRO)), 2.2f, 12000.0f, 2400.0f, 0.5f);
    pelaGrama.velocidade = 3000.0f;
    Comandos reto = { 1, 0, 0, 0, 0.0f, 0, 0 };
    float maiorSalto = 0.0f, antes = pelaGrama.deslocamentoLateral;
    for (int i = 0; i < 60 * 15; i++) {
        atualizarCarro(&pelaGrama, reto, quadro);
        maiorSalto = fmaxf(maiorSalto, fabsf(pelaGrama.deslocamentoLateral - antes));
        antes = pelaGrama.deslocamentoLateral;
    }
    printf("     pela grama, no funil: maior salto %.3f larguras em um quadro\n", maiorSalto);
    verificar(maiorSalto < 0.1f, "o funil leva o carro de volta sem salto");
    liberarPista(pista);
}

/*
 * testarCameraLivre
 * O analógico direito: um giro rápido para trás, sempre contínuo.
 */
static void testarCameraLivre(void) {
    printf("\n20) Camera livre (analogico direito)\n");
    float quadro = 1.0f / 60.0f;
    Olhar olhar = olharParaAFrente();

    atualizarOlhar(&olhar, 0.05f, -0.08f, quadro);
    verificar(olhar.giro == 0.0f && olhar.elevacao == 0.0f, "analogico solto (dentro da folga): a camera nao se mexe");

    /* Todo para baixo: chega rápido, mas sem salto */
    float maiorPasso = 0.0f, anterior = 0.0f, ateChegar = 0.0f;
    while (fabsf(olhar.giro) < 175.0f && ateChegar < 2.0f) {
        atualizarOlhar(&olhar, 0.0f, 1.0f, quadro);
        maiorPasso = fmaxf(maiorPasso, fabsf(olhar.giro - anterior));
        anterior = olhar.giro;
        ateChegar += quadro;
    }
    printf("     olhar para tras: %.2f s, maior passo em um quadro %.1f graus\n", ateChegar, maiorPasso);
    verificar(ateChegar < 0.3f, "todo para baixo: olha para tras em menos de 0,3 s");
    verificar(maiorPasso < 60.0f, "e o giro e continuo, sem salto de um quadro");

    float pontaAntes = fabsf(olhar.giro);
    for (int i = 0; i < 30; i++) {
        atualizarOlhar(&olhar, 0.02f, 0.99f, quadro);
    }
    verificar(fabsf(olhar.giro) > pontaAntes && fabsf(olhar.giro) > 178.0f, "segurando, ela fica para tras");

    int passou = 0;
    for (int i = 0; i < 60; i++) {
        atualizarOlhar(&olhar, 0.0f, 0.0f, quadro);
        passou = passou || (i > 30 && fabsf(olhar.giro) > 2.0f);
    }
    verificar(fabsf(olhar.giro) < 0.5f && !passou, "soltando: volta para a frente rapido, sem balancar");

    /* A mola não depende da taxa de quadros */
    Olhar a30 = olharParaAFrente(), a120 = olharParaAFrente();
    for (int i = 0; i < 9; i++) {
        atualizarOlhar(&a30, 1.0f, 0.0f, 1.0f / 30.0f);
    }
    for (int i = 0; i < 36; i++) {
        atualizarOlhar(&a120, 1.0f, 0.0f, 1.0f / 120.0f);
    }
    printf("     0,3 s para o lado: %.1f graus a 30 fps, %.1f a 120 fps\n", a30.giro, a120.giro);
    verificar(fabsf(a30.giro - a120.giro) < 1.0f, "o movimento e igual a 30 e a 120 quadros por segundo");

    for (int i = 0; i < 120; i++) {
        atualizarOlhar(&olhar, 1.0f, 0.0f, quadro);
    }
    verificar(fabsf(olhar.giro - 90.0f) < 0.5f, "todo para a direita: chega a 90 graus");
    for (int i = 0; i < 120; i++) {
        atualizarOlhar(&olhar, -0.5f, 0.0f, quadro);
    }
    verificar(olhar.giro < -5.0f && olhar.giro > -40.0f, "meio para a esquerda: gira menos que o lado todo");
    for (int i = 0; i < 120; i++) {
        atualizarOlhar(&olhar, 0.0f, 0.0f, quadro);
    }
    verificar(fabsf(olhar.giro) < 0.5f, "soltando de lado: volta suave para a frente");

    for (int i = 0; i < 120; i++) {
        atualizarOlhar(&olhar, 0.0f, -1.0f, quadro);
    }
    verificar(fabsf(olhar.elevacao - ELEVACAO_MAXIMA) < 0.5f && fabsf(olhar.giro) < 0.5f, "todo para cima: olha para o alto, sem girar");
}

/*
 * testarNitro
 * Gasta, passa da máxima, acaba, e recarrega mais rápido freando.
 */
static void testarNitro(void) {
    printf("\n21) Nitro\n");
    float quadro = 1.0f / 60.0f;
    Pista *reta = criarPistaVazia();
    adicionarTrecho(reta, "Reta", 400, RETA, 0.0f, 0.0f);
    Comandos acelerar = { 1, 0, 0, 0, 0.0f, 0, 0 };
    Comandos comNitro = { 1, 0, 0, 0, 0.0f, 1, 0 };
    Comandos frear = { 0, 1, 0, 0, 0.0f, 1, 0 };
    Comandos aliviar = { 0, 0, 0, 0, 0.0f, 0, 0 };

    Carro carro = criarCarro("Teste", 0, reta->largada, 0.0f, 12000.0f, 2400.0f, 0.5f);
    verificar(carro.nitro == 1.0f && !carro.nitroLigado, "larga com o tanque cheio e desligado");

    /* Sem nitro, chega na máxima e para ali */
    carro.velocidade = carro.velocidadeMaxima;
    Carro semNitro = carro;
    float duracao = 0.0f, maior = 0.0f;
    while (duracao < 20.0f) {
        atualizarCarro(&carro, comNitro, quadro);
        atualizarCarro(&semNitro, acelerar, quadro);
        maior = fmaxf(maior, carro.velocidade);
        duracao += quadro;
        if (!carro.nitroLigado) {
            break;
        }
    }
    printf("     tanque cheio: %.1f s, chegou a %d km/h (sem nitro: %d km/h)\n", duracao, converterParaKmh(maior), velocidadeEmKmh(&semNitro));
    verificar(duracao > 3.0f && duracao < 6.0f, "o tanque cheio dura de 3 a 6 s");
    verificar(maior > carro.velocidadeMaxima * 1.1f && semNitro.velocidade <= semNitro.velocidadeMaxima, "o nitro passa da velocidade maxima");

    /* Acabou: não religa logo em seguida, mesmo apertando */
    atualizarCarro(&carro, comNitro, quadro);
    verificar(!carro.nitroLigado, "com o tanque vazio, o nitro nao liga");
    float acimaDaMaxima = carro.velocidade;
    atualizarCarro(&carro, acelerar, quadro);
    verificar(carro.velocidade < acimaDaMaxima, "acabando o nitro, a velocidade cai aos poucos");

    /* Recarga: devagar sempre, mais rápido freando */
    Carro andando = criarCarro("Teste", 0, reta->largada, 0.0f, 12000.0f, 2400.0f, 0.5f);
    andando.nitro = 0.0f;
    andando.velocidade = 8000.0f;
    Carro freando = andando;
    for (int i = 0; i < 60; i++) {
        atualizarCarro(&andando, aliviar, quadro);
        atualizarCarro(&freando, frear, quadro);
    }
    printf("     1 s de recarga: %.0f%% andando, %.0f%% freando\n", andando.nitro * 100.0f, freando.nitro * 100.0f);
    verificar(andando.nitro > 0.0f && andando.nitro < 0.05f, "recarrega aos poucos andando");
    verificar(freando.nitro > andando.nitro * 3.0f, "freando, recarrega bem mais rapido");
    verificar(!freando.nitroLigado, "frear nao deixa o nitro ligar");

    /* Mesmo parado, frear não recarrega como freada */
    Carro parado = criarCarro("Teste", 0, reta->largada, 0.0f, 12000.0f, 2400.0f, 0.5f);
    parado.nitro = 0.0f;
    Comandos so = { 0, 1, 0, 0, 0.0f, 0, 0 };
    for (int i = 0; i < 60; i++) {
        atualizarCarro(&parado, so, quadro);
    }
    verificar(parado.nitro < 0.05f, "parado, segurar o freio nao enche o tanque rapido");

    /* Os rivais usam: numa corrida, alguém liga o nitro e ninguém fica sem usar */
    Pista *pista = criarPista();
    static Corrida corrida;
    iniciarCorrida(&corrida, pista, 0, NIVEL_MEDIO);
    int usaram[TOTAL_RIVAIS] = { 0 }, naGrama = 0;
    while (corrida.tempoDeCorrida < 60.0f) {
        atualizarCorrida(&corrida, atualizarIA(&corrida.jogador, corrida.pista), quadro);
        for (int r = 0; r < TOTAL_RIVAIS; r++) {
            usaram[r] = usaram[r] || corrida.rivais[r].nitroLigado;
            naGrama += estaForaDaPista(&corrida.rivais[r]);
        }
    }
    int quantos = 0;
    for (int r = 0; r < TOTAL_RIVAIS; r++) {
        quantos += usaram[r];
    }
    printf("     %d de %d rivais usaram nitro no primeiro minuto; %d quadros-carro na grama\n", quantos, TOTAL_RIVAIS, naGrama);
    verificar(quantos == TOTAL_RIVAIS, "todos os rivais usam o nitro");

    liberarPista(pista);
    liberarPista(reta);
}

/*
 * testarGiroDasRodas
 * As rodas giram com o carro, sem passar do que a tela consegue mostrar.
 */
static void testarGiroDasRodas(void) {
    printf("\n24) Giro das rodas\n");
    float quadro = 1.0f / 60.0f;
    verificar(rapidezDasRodas(0.0f, 0) == 0.0f, "parado e sem acelerar, a roda nao gira");
    verificar(rapidezDasRodas(0.0f, 1) > 10.0f, "acelerando na arrancada, a roda patina");
    verificar(rapidezDasRodas(2000.0f, 0) < rapidezDasRodas(6000.0f, 0) && rapidezDasRodas(6000.0f, 0) < rapidezDasRodas(12000.0f, 0),
              "quanto mais rapido o carro, mais rapido a roda");
    /* Mais de meio raio de um aro de 5 raios por quadro pareceria girar ao contrario */
    verificar(rapidezDasRodas(20000.0f, 1) * quadro < 3.14159f / 5.0f, "a 60 fps a roda nunca parece girar ao contrario");

    Pista *pista = criarPista();
    static Corrida corrida;
    iniciarCorrida(&corrida, pista, 0, NIVEL_MEDIO);
    Comandos acelerar = { 1, 0, 0, 0, 0.0f, 0, 0 };
    atualizarCorrida(&corrida, acelerar, quadro);
    verificar(corrida.jogador.anguloDasRodas == 0.0f, "no grid, na contagem, as rodas ficam paradas");
    corrida.contagemRegressiva = 0.0f;
    atualizarCorrida(&corrida, acelerar, quadro);
    verificar(corrida.jogador.anguloDasRodas > 0.0f, "largando, elas comecam a girar");
    verificar(corrida.rivais[0].anguloDasRodas > 0.0f, "as dos rivais tambem");
    liberarPista(pista);
}

/*
 * testarPitstop
 * A parada obrigatória: entrar, parar na vaga, trocar, voltar e a punição de quem não para.
 */
static void testarPitstop(void) {
    printf("\n26) Pitstop\n");
    float quadro = 1.0f / 60.0f;
    Pista *pista = criarPista();
    static Corrida corrida;
    Comandos acelerar = { 1, 0, 0, 0, 0.0f, 0, 0 };

    /* O jogador entra de propósito, e o carro segue sozinho */
    iniciarCorrida(&corrida, pista, 0, NIVEL_MEDIO);
    corrida.jogador.controladoPeloJogador = 0;   /* aqui é o piloto automático do pit que está em teste */
    corrida.contagemRegressiva = 0.0f;
    Carro *jogador = &corrida.jogador;
    verificar(!jogador->pitFeito && pista->temPit, "a corrida comeca com a parada pendente");
    jogador->voltas = 2;
    jogador->posicao.segmento = buscarSegmento(pista, (int) ((ENTRADA_DO_PIT - 100.0f) * SEGMENTOS_POR_METRO));
    jogador->posicao.distanciaNoSegmento = 0.0f;
    jogador->deslocamentoLateral = -0.5f;
    jogador->velocidade = 7000.0f;
    int quadros = 0;
    while (!carroNoPit(jogador) && quadros < 60 * 5) {
        Comandos paraEsquerda = { 1, 0, 1, 0, 0.0f, 0, 0 };
        atualizarCorrida(&corrida, paraEsquerda, quadro);
        quadros++;
    }
    verificar(carroNoPit(jogador), "virando para o pit lane, ele entra");

    float ondeFoiOSalto = 0.0f;
    float maiorVelocidadeNoLimite = 0.0f, maiorSalto = 0.0f, antes = jogador->deslocamentoLateral * jogador->posicao.segmento->larguraEsquerda;
    float metrosAntes = metrosNaVolta(jogador), maiorDistanciaAoChao = 0.0f;
    int chegouNaVaga = 0, voltasAntes = jogador->voltas, trocou = 0;
    float tempoEntrada = corrida.tempoDeCorrida;
    quadros = 0;
    while (carroNoPit(jogador) && quadros < 60 * 90) {
        atualizarCorrida(&corrida, acelerar, quadro);
        quadros++;
        float agora = jogador->deslocamentoLateral * jogador->posicao.segmento->larguraEsquerda;
        if (fabsf(agora - antes) > maiorSalto) {
            maiorSalto = fabsf(agora - antes);
            ondeFoiOSalto = metrosNaVolta(jogador);
        }
        antes = agora;
        float metros = metrosNaVolta(jogador);
        if (metros > pista->pit.inicioDoLimite + 1.0f || metros < pista->pit.fimDoLimite - 1.0f) {
            maiorVelocidadeNoLimite = fmaxf(maiorVelocidadeNoLimite, jogador->velocidade);
        }
        if (jogador->estadoDoPit == PIT_TROCANDO) {
            chegouNaVaga = 1;
            maiorDistanciaAoChao = fmaxf(maiorDistanciaAoChao, jogador->velocidade);
            int canto;
            float andamento;
            trocou |= faseDaTroca(jogador, &canto, &andamento);
        }
        (void) metrosAntes;
    }
    printf("     no pit lane: %.1f s (parado %.2f s); maior salto de lado %.2f m, aos %.0f m\n", corrida.tempoDeCorrida - tempoEntrada, jogador->ultimaTroca, maiorSalto, ondeFoiOSalto);
    verificar(chegouNaVaga && trocou, "ele para na vaga e o Guido troca as rodas");
    verificar(maiorDistanciaAoChao == 0.0f, "parado na vaga, a velocidade e zero");
    verificar(fabsf(jogador->ultimaTroca - DURACAO_DA_TROCA) < 0.05f, "a troca leva o tempo do Guido");
    verificar(DURACAO_DA_TROCA < 4.0f, "a troca e rapida, como no filme");
    verificar(maiorVelocidadeNoLimite <= LIMITE_DO_PIT + 1.0f, "no pit lane ele respeita o limite");
    verificar(maiorSalto < 0.3f, "ele anda de lado sem saltos");
    verificar(jogador->pitFeito && !carroNoPit(jogador), "ele volta para a pista com a parada feita");
    verificar(jogador->deslocamentoLateral >= -1.0f, "e volta na borda do asfalto");
    verificar(jogador->voltas == voltasAntes + 1, "passar pelo pit lane conta a volta na linha de chegada");

    /* De qualquer ponto da entrada, o carro nunca atravessa o muro, e sai na Reta Oposta */
    float metrosDaVolta = pista->metrosDaVolta;
    int atravessou = 0, entradas = 0, saiuNaReta = 1, longeDoMuro = 1;
    for (float inicio = pista->pit.inicioLivre; inicio <= pista->pit.fimDaZona - 40.0f; inicio += 20.0f) {
        float larguraAqui = 0.0f;
        iniciarCorrida(&corrida, pista, 0, NIVEL_MEDIO);
        corrida.jogador.controladoPeloJogador = 0;   /* aqui é o piloto automático do pit que está em teste */
        corrida.contagemRegressiva = 0.0f;
        jogador->voltas = 2;
        jogador->posicao.segmento = buscarSegmento(pista, (int) (inicio * SEGMENTOS_POR_METRO));
        jogador->posicao.distanciaNoSegmento = 0.0f;
        jogador->deslocamentoLateral = -(ladoDentroDoPit(inicio, metrosDaVolta, &larguraAqui) + 0.25f);
        jogador->velocidade = 8000.0f;
        atualizarCorrida(&corrida, acelerar, quadro);
        if (!carroNoPit(jogador)) {
            continue;
        }
        entradas++;
        for (int i = 0; i < 60 * 90 && carroNoPit(jogador); i++) {
            atualizarCorrida(&corrida, acelerar, quadro);
            float metros = metrosNaVolta(jogador);
            float muro = jogador->posicao.segmento->lugarDoMuro * jogador->posicao.segmento->larguraEsquerda;
            float lado = -jogador->deslocamentoLateral * jogador->posicao.segmento->larguraEsquerda;
            if (jogador->posicao.segmento->temMuro && metros >= INICIO_DO_FUNIL && lado < muro + 1.0f) {
                atravessou++;
            }
            if (metros > FIM_DO_DESVIO_DA_SAIDA && metros < INICIO_DA_VOLTA
                && lado < LADO_DA_SAIDA * jogador->posicao.segmento->larguraEsquerda - 2.0f) {
                longeDoMuro = 0;
            }
        }
        float ondeSaiu = metrosNaVolta(jogador);
        saiuNaReta &= (ondeSaiu >= FIM_DO_PIT_LANE - 5.0f && ondeSaiu < FIM_DO_PIT_LANE + 20.0f
                       && strcmp(jogador->posicao.segmento->nomeTrecho, "Reta Oposta") == 0);
    }
    printf("     %d entradas diferentes, %d quadros encostando no muro\n", entradas, atravessou);
    verificar(entradas >= 4, "da para entrar de varios pontos da entrada");
    verificar(atravessou == 0, "em nenhuma entrada o carro atravessa o muro");
    verificar(longeDoMuro, "depois da reta ele fica no pit lane, longe da ponta do muro");
    verificar(saiuNaReta, "ele volta a pista na Reta Oposta");

    /* O pit lane e bem largo logo depois de abrir, e nao tem nada no caminho */
    float larguraNaEntrada = 0.0f;
    ladoDentroDoPit(ENTRADA_DO_PIT + 50.0f, metrosDaVolta, &larguraNaEntrada);
    verificar(larguraNaEntrada > 0.0f, "logo depois de abrir, o pit lane ja tem largura");
    float larguraAberta = 0.0f;
    float ladoAberto = ladoDentroDoPit(ENTRADA_DO_PIT + METROS_ABRINDO_O_PIT, metrosDaVolta, &larguraAberta);
    verificar(larguraAberta >= LARGURA_DO_PIT_LANE - 0.01f && ladoAberto == 1.0f, "ele abre inteiro ainda colado na pista");
    float larguraFechando = 0.0f;
    float ladoFechando = ladoDentroDoPit(FIM_DA_VOLTA + 20.0f, metrosDaVolta, &larguraFechando);
    verificar(ladoFechando == 1.0f && larguraFechando > 5.0f, "na saida, ele volta a colar na pista e fecha colado");

    /* Nas duas ligacoes nao ha degrau: o caminho do pit lane muda aos poucos */
    float maiorMudanca = 0.0f, ladoAntes = 1.0f, larguraAntes = 0.0f;
    for (float m = ENTRADA_DO_PIT; m < metrosDaVolta + FIM_DO_PIT_LANE; m += 1.0f) {
        float largura = 0.0f;
        float lado = ladoDentroDoPit(fmodf(m, metrosDaVolta), metrosDaVolta, &largura) * pista->largada->larguraEsquerda;
        if (m > ENTRADA_DO_PIT) {
            maiorMudanca = fmaxf(maiorMudanca, fmaxf(fabsf(lado - ladoAntes), fabsf(largura - larguraAntes)));
        }
        ladoAntes = lado;
        larguraAntes = largura;
    }
    printf("     maior mudanca do pit lane por metro: %.2f m\n", maiorMudanca);
    verificar(maiorMudanca < 0.6f, "o pit lane nao tem degrau em nenhum ponto");

    /* Andar por onde o pit lane encosta na pista nao e andar na grama */
    Segmento *colado = buscarSegmento(pista, (int) ((ENTRADA_DO_PIT + 70.0f) * SEGMENTOS_POR_METRO));
    Carro noColado = criarCarro("Teste", 0, colado, -1.2f, 12000.0f, 2400.0f, 0.5f);
    verificar(!estaForaDaPista(&noColado), "o pit lane colado na pista e asfalto");
    Carro naGrama = criarCarro("Teste", 0, buscarSegmento(pista, 1400), -1.2f, 12000.0f, 2400.0f, 0.5f);
    verificar(estaForaDaPista(&naGrama), "longe dele, continua sendo grama");
    int objetosNoCaminho = 0, sobreAsfalto = 0;
    float larguraDaPista = pista->largada->larguraEsquerda;
    for (float m = ENTRADA_DO_PIT; m < metrosDaVolta + FIM_DO_PIT_LANE; m += 4.0f) {
        float metros = fmodf(m, metrosDaVolta), larguraDoPit = 0.0f;
        float borda = ladoDentroDoPit(metros, metrosDaVolta, &larguraDoPit);
        PosicaoNaPista lugar = { buscarSegmento(pista, (int) (metros * SEGMENTOS_POR_METRO)), 0.0f };
        for (int parte = 0; parte <= 4; parte++) {
            PontoNoMundo ponto = pontoNoMundo(lugar, -(borda + larguraDoPit / larguraDaPista * parte / 4.0f));
            Segmento *outro = pista->largada;
            do {
                float deste = hypotf(outro->xMundo - ponto.x, outro->zMundo - ponto.z);
                sobreAsfalto += (parte > 0 && metros > 330.0f && metros < 700.0f && deste < larguraDaPista - 0.5f);
                for (Obstaculo *o = outro->obstaculos; o != NULL; o = o->proximo) {
                    if (o->tipo == OBJETO_ARVORE || o->tipo == OBJETO_PNEUS || o->tipo == OBJETO_PLACA_50
                        || o->tipo == OBJETO_PLACA_100 || o->tipo == OBJETO_PLACA_150) {
                        PosicaoNaPista ali = { outro, 0.0f };
                        PontoNoMundo noObjeto = pontoNoMundo(ali, o->deslocamentoLateral);
                        objetosNoCaminho += hypotf(noObjeto.x - ponto.x, noObjeto.z - ponto.z) < 2.0f;
                    }
                }
                outro = proximoSegmento(outro);
            } while (outro != pista->largada);
        }
    }
    printf("     no caminho do pit lane: %d objetos, %d pontos sobre o asfalto da pista\n", objetosNoCaminho, sobreAsfalto);
    verificar(objetosNoCaminho == 0, "nenhum objeto no caminho do pit lane");
    verificar(sobreAsfalto == 0, "a saida nao passa por cima da pista");

    /* O Guido nunca entra no carro, em nenhum momento da troca, em nenhum carro */
    int dentroDoCarro = 0, deixouDeSair = 0;
    for (int n = 0; n < TOTAL_CARROS; n++) {
        const FichaTecnica *ficha = fichaDoCarro(n);
        float meiaLargura = ficha->largura / 2.0f, meioComprimento = ficha->comprimento / 2.0f;
        Carro parado = *jogador;
        parado.estadoDoPit = PIT_TROCANDO;
        float maiorX = 0.0f;
        for (float t = 0.0f; t < DURACAO_DA_TROCA; t += 0.005f) {
            parado.tempoDaTroca = t;
            LugarDoGuido g = lugarDoGuido(&parado, meiaLargura, meioComprimento, meioComprimento * 0.62f, -meioComprimento * 0.62f);
            maiorX = fmaxf(maiorX, fabsf(g.x));
            /* Os cantos e o meio do Guido, olhando para onde ele aponta */
            for (int fr = -2; fr <= 2; fr++) {
                for (int la = -1; la <= 1; la++) {
                    float noComprimento = fr / 2.0f * MEIO_COMPRIMENTO_DO_GUIDO, naLargura = la * MEIA_LARGURA_DO_GUIDO;
                    float x = g.x + sinf(g.olha) * noComprimento + cosf(g.olha) * naLargura;
                    float z = g.z + cosf(g.olha) * noComprimento - sinf(g.olha) * naLargura;
                    dentroDoCarro += (fabsf(x) < meiaLargura && fabsf(z) < meioComprimento);
                }
            }
        }
        deixouDeSair += (maiorX < meiaLargura);
    }
    printf("     pontos do Guido dentro do carro: %d\n", dentroDoCarro);
    verificar(dentroDoCarro == 0, "o Guido nunca entra dentro do carro");
    verificar(deixouDeSair == 0, "ele trabalha ao lado do carro");

    /* Cada carro tem a sua vaga, todas antes da linha */
    int vagasBoas = 1;
    for (int n = 0; n < TOTAL_CARROS; n++) {
        vagasBoas &= metroDaVagaDoCarro(pista, n) < -5.0f + 0.01f && metroDaVagaDoCarro(pista, n) >= PRIMEIRO_METRO_DOS_BOXES;
    }
    verificar(vagasBoas, "as vagas ficam nos boxes, antes da linha de chegada");

    /* Quem anda encostado na borda, sem querer entrar, continua na pista */
    iniciarCorrida(&corrida, pista, 0, NIVEL_MEDIO);
    corrida.contagemRegressiva = 0.0f;
    jogador->posicao.segmento = buscarSegmento(pista, (int) ((ENTRADA_DO_PIT - 20.0f) * SEGMENTOS_POR_METRO));
    jogador->deslocamentoLateral = -0.95f;
    jogador->velocidade = 7000.0f;
    int entrou = 0;
    for (int i = 0; i < 60 * 3; i++) {
        atualizarCorrida(&corrida, acelerar, quadro);
        entrou |= carroNoPit(jogador);
    }
    verificar(!entrou, "pela borda da pista ninguem entra sem querer");

    /* Quem nao para e desclassificado; e os rivais param todos */
    iniciarCorrida(&corrida, pista, 0, NIVEL_MEDIO);
    corrida.contagemRegressiva = 0.0f;
    quadros = 0;
    while (!corrida.terminou && quadros < 60 * 60 * 20) {
        Comandos semPit = atualizarIA(jogador, corrida.pista);
        jogador->querParar = 0;
        atualizarCorrida(&corrida, semPit, quadro);
        quadros++;
    }
    verificar(corrida.terminou && corrida.desclassificado, "sem parar nos boxes, ele e desclassificado");
    verificar(corrida.posicaoFinal == TOTAL_CARROS && !jogadorVenceu(&corrida), "e nao ganha a corrida");
    for (int i = 0; i < 60 * 60 * 10 && !todosChegaram(&corrida); i++) {
        atualizarCorrida(&corrida, acelerar, quadro);
    }
    int pararam = 0;
    for (int i = 0; i < TOTAL_RIVAIS; i++) {
        pararam += corrida.rivais[i].pitFeito;
    }
    printf("     %d de %d rivais pararam\n", pararam, TOTAL_RIVAIS);
    verificar(pararam == TOTAL_RIVAIS, "todos os rivais fazem a parada");

    /* Parando, ele nao fica desclassificado */
    iniciarCorrida(&corrida, pista, 0, NIVEL_MEDIO);
    corrida.contagemRegressiva = 0.0f;
    quadros = 0;
    while (!corrida.terminou && quadros < 60 * 60 * 20) {
        atualizarCorrida(&corrida, atualizarIA(jogador, corrida.pista), quadro);
        quadros++;
    }
    printf("     corrida de %d voltas com parada: %.0f s, chegou em %d\n", TOTAL_VOLTAS, corrida.tempoDeCorrida, corrida.posicaoFinal);
    verificar(corrida.terminou && !corrida.desclassificado && jogador->pitFeito, "parando, ele termina a corrida valendo");
    liberarPista(pista);

    /* Na Copa Pistao ainda nao ha pit lane */
    Pista *copa = criarPistaPorId(PISTA_COPA_PISTAO);
    iniciarCorrida(&corrida, copa, 0, NIVEL_MEDIO);
    verificar(copa->temPit && !corrida.jogador.pitFeito, "na Copa Pistao tambem ha parada obrigatoria");
    liberarPista(copa);
}

/*
 * testarFreioDeMao
 * O freio de mao solta a traseira e o carro vira mais, mas perde velocidade.
 */
static void testarFreioDeMao(void) {
    printf("\n27) Freio de mao e drift\n");
    float quadro = 1.0f / 60.0f;
    Pista *pista = criarPista();
    Comandos soFreioDeMao = { 0, 0, 0, 0, 0.0f, 0, 1 }, nada = { 0, 0, 0, 0, 0.0f, 0, 0 };
    Comandos virandoComFreio = { 1, 0, 0, 1, 0.0f, 0, 1 }, acelerando = { 1, 0, 0, 0, 0.0f, 0, 0 };

    /* Na reta, com o volante reto: so perde velocidade */
    Carro reto = criarCarro("Teste", 0, buscarSegmento(pista, 550), 0.0f, 12000.0f, 2400.0f, 0.5f);
    Carro semFreio = reto;
    reto.velocidade = semFreio.velocidade = 8000.0f;
    for (int i = 0; i < 60; i++) {
        atualizarCarro(&reto, soFreioDeMao, quadro);
        atualizarCarro(&semFreio, nada, quadro);
    }
    verificar(reto.velocidade < semFreio.velocidade - 200.0f, "o freio de mao tira velocidade");
    verificar(reto.forcaDoDrift == 0.0f && reto.anguloDeDrift == 0.0f, "com o volante reto, nao ha drift");
    verificar(fabsf(reto.deslocamentoLateral - semFreio.deslocamentoLateral) < 0.05f, "e o carro nao sai do rumo");

    /* Virando: a traseira solta, o carro fica de lado e sai fumaca */
    Carro drift = criarCarro("Teste", 0, buscarSegmento(pista, 550), 0.0f, 12000.0f, 2400.0f, 0.5f);
    Carro normal = drift;
    drift.velocidade = normal.velocidade = 8000.0f;
    Comandos virando = { 1, 0, 0, 1, 0.0f, 0, 0 };
    float maiorAngulo = 0.0f, maiorFumaca = 0.0f;
    for (int i = 0; i < 40; i++) {
        atualizarCarro(&drift, virandoComFreio, quadro);
        atualizarCarro(&normal, virando, quadro);
        maiorAngulo = fmaxf(maiorAngulo, drift.anguloDeDrift);
        maiorFumaca = fmaxf(maiorFumaca, drift.derrapagem);
    }
    printf("     angulo do drift: %.0f graus; de lado: %.2f contra %.2f sem freio\n", maiorAngulo, drift.deslocamentoLateral, normal.deslocamentoLateral);
    verificar(maiorAngulo > 25.0f, "virando, o carro fica de lado");
    verificar(maiorFumaca >= 0.99f, "e sai fumaca dos pneus");
    verificar(drift.deslocamentoLateral > normal.deslocamentoLateral * 0.7f, "de lado, ele continua virando");
    verificar(drift.velocidade < normal.velocidade, "mas perde velocidade");

    /* Quebrada a traseira, quem manda e o pe: acelerando de lado o drift continua */
    Comandos tocaOFreio = { 1, 0, 0, 1, 0.0f, 0, 1 }, acelerandoVirado = { 1, 0, 0, 1, 0.0f, 0, 0 };
    Comandos soVirado = { 0, 0, 0, 1, 0.0f, 0, 0 }, contraesterco = { 1, 0, 1, 0, 0.0f, 0, 0 };
    float segundosDeDrift[2];
    float anguloNoMeio = 0.0f;
    for (int caso = 0; caso < 2; caso++) {
        Carro preparado = criarCarro("Teste", 0, buscarSegmento(pista, 550), 0.0f, 12000.0f, 2400.0f, 0.5f);
        preparado.velocidade = 8000.0f;
        preparado.volante = 1.0f;
        for (int i = 0; i < 18; i++) {
            atualizarCarro(&preparado, tocaOFreio, quadro);
            preparado.deslocamentoLateral = 0.0f;
        }
        float tempoSolto = 0.0f;
        while (preparado.forcaDoDrift > 0.0f && tempoSolto < 8.0f) {
            atualizarCarro(&preparado, caso == 0 ? acelerandoVirado : soVirado, quadro);
            preparado.deslocamentoLateral = 0.0f;
            preparado.velocidade = 7000.0f;
            tempoSolto += quadro;
            if (caso == 0 && fabsf(tempoSolto - 1.0f) < quadro / 2.0f) {
                anguloNoMeio = preparado.anguloDeDrift;
            }
        }
        segundosDeDrift[caso] = tempoSolto;
    }
    printf("     depois de um toque no freio de mao: drift dura %.1f s acelerando, %.1f s sem acelerar; 1 s depois, %.0f graus\n",
           segundosDeDrift[0], segundosDeDrift[1], anguloNoMeio);
    verificar(segundosDeDrift[0] > 1.5f, "um toque no freio de mao basta: acelerando, o drift continua");
    verificar(segundosDeDrift[1] < segundosDeDrift[0] * 0.5f, "sem acelerar, ele acaba bem antes");
    verificar(anguloNoMeio > 15.0f, "e o carro segue de lado");

    /* O angulo tem inercia e o contraesterco o segura */
    Carro inercia = criarCarro("Teste", 0, buscarSegmento(pista, 550), 0.0f, 12000.0f, 2400.0f, 0.5f);
    inercia.velocidade = 8000.0f;
    inercia.volante = 1.0f;
    atualizarCarro(&inercia, tocaOFreio, quadro);
    verificar(inercia.anguloDeDrift < 3.0f, "o carro nao fica de lado de uma vez");
    for (int i = 0; i < 25; i++) {
        atualizarCarro(&inercia, tocaOFreio, quadro);
        inercia.deslocamentoLateral = 0.0f;
    }
    Carro semCorrigir = inercia;
    for (int i = 0; i < 30; i++) {
        atualizarCarro(&inercia, contraesterco, quadro);
        atualizarCarro(&semCorrigir, acelerandoVirado, quadro);
        inercia.deslocamentoLateral = semCorrigir.deslocamentoLateral = 0.0f;
        inercia.velocidade = semCorrigir.velocidade = 7000.0f;
    }
    printf("     meio segundo depois: %.0f graus com contraesterco, %.0f sem\n", inercia.anguloDeDrift, semCorrigir.anguloDeDrift);
    verificar(inercia.anguloDeDrift < semCorrigir.anguloDeDrift * 0.6f, "virando para o outro lado, o contraesterco segura o carro");

    /* Depois, tudo volta ao normal e o carro retoma */
    float velocidadeAoSoltar = inercia.velocidade;
    for (int i = 0; i < 60 * 5; i++) {
        atualizarCarro(&inercia, acelerando, quadro);
        inercia.deslocamentoLateral = 0.0f;
    }
    verificar(inercia.forcaDoDrift == 0.0f && fabsf(inercia.anguloDeDrift) < 0.5f, "reto, o carro alinha e o drift acaba");
    verificar(inercia.velocidade > velocidadeAoSoltar, "e o carro volta a acelerar");

    /* Devagar, nada de drift */
    Carro devagar = criarCarro("Teste", 0, buscarSegmento(pista, 550), 0.0f, 12000.0f, 2400.0f, 0.5f);
    devagar.velocidade = 800.0f;
    for (int i = 0; i < 30; i++) {
        atualizarCarro(&devagar, virandoComFreio, quadro);
    }
    verificar(devagar.forcaDoDrift == 0.0f, "muito devagar nao ha drift");

    /* O freio de mao salva uma curva fechada, entrando rapido demais */
    Segmento *curva = pista->largada;
    float maisFechada = 0.0f;
    do {
        if (fabsf(curva->curvatura) > fabsf(maisFechada)) {
            maisFechada = curva->curvatura;
        }
        curva = proximoSegmento(curva);
    } while (curva != pista->largada);
    Segmento *entrada = pista->largada;
    while (fabsf(entrada->curvatura) < fabsf(maisFechada) * 0.9f) {
        entrada = proximoSegmento(entrada);
    }
    for (int i = 0; i < 20; i++) {
        entrada = segmentoAnterior(entrada);
    }
    int paraDireita = maisFechada > 0.0f;
    Comandos noVolante = { 1, 0, !paraDireita, paraDireita, 0.0f, 0, 0 };
    Comandos comFreioDeMao = { 0, 0, !paraDireita, paraDireita, 0.0f, 0, 1 };
    Carro comDrift = criarCarro("Teste", 0, entrada, 0.0f, 12000.0f, 2400.0f, 0.5f);
    Carro semDrift = comDrift;
    comDrift.velocidade = semDrift.velocidade = 8000.0f;   /* 200 km/h */
    float foraComDrift = 0.0f, foraSemDrift = 0.0f, lado = paraDireita ? 1.0f : -1.0f;
    for (int i = 0; i < 70; i++) {
        atualizarCarro(&comDrift, comFreioDeMao, quadro);
        atualizarCarro(&semDrift, noVolante, quadro);
        foraComDrift = fmaxf(foraComDrift, -comDrift.deslocamentoLateral * lado);
        foraSemDrift = fmaxf(foraSemDrift, -semDrift.deslocamentoLateral * lado);
    }
    printf("     curva fechada, rapido demais: sai %.2f larguras sem drift, %.2f com drift (%d contra %d km/h)\n",
           foraSemDrift, foraComDrift, velocidadeEmKmh(&comDrift), velocidadeEmKmh(&semDrift));
    verificar(foraComDrift < 1.0f, "fazendo a curva de lado, rapido demais, o carro nao sai da pista");
    verificar(velocidadeEmKmh(&comDrift) > velocidadeEmKmh(&semDrift) + 15, "e passa por ela com mais velocidade");

    /* A IA nao usa */
    static Corrida corrida;
    iniciarCorrida(&corrida, pista, 0, NIVEL_MEDIO);
    Comandos daIA = atualizarIA(&corrida.rivais[0], corrida.pista);
    verificar(!daIA.freioDeMao, "a IA nao puxa o freio de mao");
    liberarPista(pista);
}

/*
 * correrComoIA
 * Uma corrida inteira, com a IA dirigindo o jogador.
 * Recebe: a corrida, a pista, o nivel, a habilidade do jogador e quantas paradas fazer.
 * Devolve: nada (a corrida fica no fim).
 */
static void correrComoIA(Corrida *corrida, Pista *pista, NivelDaCorrida nivel, float habilidade) {
    iniciarCorrida(corrida, pista, 0, nivel);
    corrida->contagemRegressiva = 0.0f;
    corrida->jogador.habilidade = habilidade;
    float quadro = 1.0f / 60.0f;
    for (int i = 0; i < 60 * 60 * 20 && !corrida->terminou; i++) {
        atualizarCorrida(corrida, atualizarIA(&corrida->jogador, corrida->pista), quadro);
    }
}

/*
 * testarNiveisEDesgaste
 * Os tres niveis e o que muda em cada um, e os pneus do dificil.
 */
static void testarNiveisEDesgaste(void) {
    printf("\n28) Niveis e desgaste dos pneus\n");
    float quadro = 1.0f / 60.0f;
    Pista *pista = criarPista();
    Pista *copa = criarPistaPorId(PISTA_COPA_PISTAO);
    static Corrida corrida;

    verificar(fichaDoNivel(NIVEL_FACIL)->voltas < fichaDoNivel(NIVEL_MEDIO)->voltas
              && fichaDoNivel(NIVEL_MEDIO)->voltas < fichaDoNivel(NIVEL_DIFICIL)->voltas, "quanto mais dificil, mais voltas");
    verificar(fichaDoNivel(NIVEL_FACIL)->habilidadeDaPole < fichaDoNivel(NIVEL_MEDIO)->habilidadeDaPole
              && fichaDoNivel(NIVEL_MEDIO)->habilidadeDaPole < fichaDoNivel(NIVEL_DIFICIL)->habilidadeDaPole, "e melhores os rivais");
    int voltasCertas = 1;
    for (int n = 0; n < TOTAL_NIVEIS; n++) {
        voltasCertas &= fichaDoNivel((NivelDaCorrida) n)->voltas <= MAXIMO_DE_VOLTAS;
    }
    verificar(voltasCertas, "nenhum nivel passa do maximo de voltas");

    iniciarCorrida(&corrida, pista, 0, NIVEL_FACIL);
    verificar(!corrida.jogador.gastaPneu && !corrida.rivais[0].gastaPneu, "no facil os pneus nao gastam");
    iniciarCorrida(&corrida, pista, 0, NIVEL_MEDIO);
    verificar(!corrida.jogador.gastaPneu, "no medio tambem nao");
    iniciarCorrida(&corrida, copa, 0, NIVEL_DIFICIL);
    verificar(corrida.jogador.gastaPneu, "e na Copa Pistao, que agora tem pit, eles gastam");
    iniciarCorrida(&corrida, pista, 0, NIVEL_DIFICIL);
    verificar(corrida.jogador.gastaPneu && corrida.rivais[12].gastaPneu, "no dificil em Interlagos, gastam para todos");

    /* O pitstop sai do facil */
    iniciarCorrida(&corrida, pista, 0, NIVEL_FACIL);
    verificar(!corrida.temParada && corrida.jogador.pitFeito && corrida.rivais[0].pitFeito && corrida.rivais[0].voltaDoPit == 0, "no facil nao ha pitstop");
    iniciarCorrida(&corrida, pista, 0, NIVEL_MEDIO);
    verificar(corrida.temParada && !corrida.jogador.pitFeito && corrida.rivais[0].voltaDoPit > 0, "no medio ha");
    iniciarCorrida(&corrida, pista, 0, NIVEL_DIFICIL);
    verificar(corrida.temParada && !corrida.jogador.pitFeito, "no dificil tambem");

    iniciarCorrida(&corrida, pista, 0, NIVEL_FACIL);
    corrida.contagemRegressiva = 0.0f;
    corrida.jogador.posicao.segmento = buscarSegmento(pista, (int) ((ENTRADA_DO_PIT + 40.0f) * SEGMENTOS_POR_METRO));
    corrida.jogador.deslocamentoLateral = -2.0f;
    corrida.jogador.velocidade = 6000.0f;
    Comandos naEntrada = { 1, 0, 0, 0, 0.0f, 0, 0 };
    int entrouNoFacil = 0;
    for (int i = 0; i < 60; i++) {
        atualizarCorrida(&corrida, naEntrada, quadro);
        entrouNoFacil |= carroNoPit(&corrida.jogador);
    }
    verificar(!entrouNoFacil, "no facil, passar pela entrada dos boxes nao leva para dentro");

    correrComoIA(&corrida, pista, NIVEL_FACIL, 0.97f);
    int alguemNoPit = carroNoPit(&corrida.jogador);
    for (int i = 0; i < TOTAL_RIVAIS; i++) {
        alguemNoPit |= corrida.rivais[i].ultimaTroca > 0.0f;
    }
    verificar(corrida.terminou && !corrida.desclassificado && !alguemNoPit && corrida.jogador.ultimaTroca == 0.0f, "no facil termina valendo, sem ninguem parar");

    /* O pneu gasto e mais lento numa volta */
    float tempos[2];
    for (int gasto = 0; gasto < 2; gasto++) {
        Carro carro = criarCarro("Teste", 0, pista->largada, 0.0f, 12000.0f, 2400.0f, 0.5f);
        carro.desgaste = gasto ? 1.0f : 0.0f;
        carro.habilidade = 1.0f;
        float tempo = 0.0f;
        while (carro.voltas < 2 && tempo < 400.0f) {
            carro.distanciaAFrente = -1.0f;
            atualizarCarro(&carro, atualizarIA(&carro, pista), quadro);
            tempo += quadro;
            if (carro.voltas == 1 && tempos[gasto] == 0.0f) {
                tempos[gasto] = 0.0f;
            }
        }
        tempos[gasto] = tempo / 2.0f;
    }
    printf("     volta com pneu novo: %.1f s; com pneu acabado: %.1f s\n", tempos[0], tempos[1]);
    verificar(tempos[1] > tempos[0] + 1.0f, "pneu acabado perde tempo na volta");

    /* Gasta com o caminho, e mais derrapando */
    Carro limpo = criarCarro("Teste", 0, buscarSegmento(pista, 1000), 0.0f, 12000.0f, 2400.0f, 0.5f);
    limpo.gastaPneu = 1;
    Carro deLado = limpo;
    limpo.velocidade = deLado.velocidade = 7000.0f;
    Comandos reto = { 1, 0, 0, 0, 0.0f, 0, 0 }, comFreio = { 1, 0, 0, 1, 0.0f, 0, 1 };
    for (int i = 0; i < 60; i++) {
        atualizarCarro(&limpo, reto, quadro);
        atualizarCarro(&deLado, comFreio, quadro);
    }
    verificar(limpo.desgaste > 0.0f && deLado.desgaste > limpo.desgaste * 2.0f, "de lado, o pneu gasta bem mais");

    /* A corrida inteira: o pneu gasta e o Guido zera */
    correrComoIA(&corrida, pista, NIVEL_DIFICIL, 0.97f);
    printf("     no fim do dificil: jogador com %.0f%% de desgaste (parou na volta %d)", corrida.jogador.desgaste * 100.0f, corrida.jogador.voltaDoPit);
    for (int i = 0; i < 3; i++) {
        printf("; rival parou na %d: %.0f%%", corrida.rivais[i].voltaDoPit, corrida.rivais[i].desgaste * 100.0f);
    }
    printf("\n");
    verificar(corrida.jogador.pitFeito && corrida.jogador.desgaste < 1.0f, "com a parada, os pneus aguentam ate o fim");
    int rivalGastou = 0;
    for (int i = 0; i < TOTAL_RIVAIS; i++) {
        rivalGastou |= corrida.rivais[i].desgaste > 0.1f;
    }
    verificar(rivalGastou, "os pneus dos rivais tambem gastam");

    iniciarCorrida(&corrida, pista, 0, NIVEL_DIFICIL);
    corrida.contagemRegressiva = 0.0f;
    corrida.jogador.desgaste = 0.8f;
    corrida.jogador.estadoDoPit = PIT_TROCANDO;
    corrida.jogador.tempoDaTroca = corrida.jogador.duracaoDaTroca;
    Comandos nada = { 0, 0, 0, 0, 0.0f, 0, 0 };
    atualizarCorrida(&corrida, nada, quadro);
    verificar(corrida.jogador.desgaste == 0.0f, "a troca deixa os pneus novos");

    /* Uma corrida em cada nivel, para ver o equilibrio */
    float habilidades[3] = { 0.90f, 0.95f, 1.00f };
    for (int n = 0; n < TOTAL_NIVEIS; n++) {
        for (int h = 0; h < 3; h++) {
            correrComoIA(&corrida, pista, (NivelDaCorrida) n, habilidades[h]);
            printf("     %-8s %d voltas, habilidade %.2f: chegou em %2d, em %.0f s\n", fichaDoNivel((NivelDaCorrida) n)->nome,
                   corrida.totalVoltas, habilidades[h], corrida.posicaoFinal, corrida.tempoDeCorrida);
            if (n == NIVEL_FACIL && h == 2) {
                verificar(corrida.posicaoFinal == 1, "no facil, sem erros, se ganha facil");
            }
        }
    }
    liberarPista(pista);
    liberarPista(copa);
}

/*
 * testarPneuEstourado
 * No dificil, com a vida do pneu perto de 20%, o pneu do jogador estoura: o carro perde velocidade,
 * puxa para um lado e so a troca nos boxes resolve.
 */
static void testarPneuEstourado(void) {
    printf("\n28b) Pneu estourado no dificil\n");
    float quadro = 1.0f / 60.0f;
    Pista *pista = criarPista();
    static Corrida corrida;
    Comandos acelerar = { 1, 0, 0, 0, 0.0f, 0, 0 };

    /* O pneu cruza os 80% de desgaste e estoura (so o do jogador, so no dificil) */
    iniciarCorrida(&corrida, pista, 0, NIVEL_DIFICIL);
    corrida.contagemRegressiva = 0.0f;
    corrida.jogador.velocidade = corrida.jogador.velocidadeMaxima;
    corrida.jogador.desgaste = 0.78f;
    verificar(corrida.jogador.pneuEstourado == 0, "comeca sem pneu estourado");
    for (int i = 0; i < 60 * 3; i++) {
        atualizarCorrida(&corrida, acelerar, quadro);
    }
    corrida.jogador.desgaste = 0.799f;
    int estourou = 0;
    for (int i = 0; i < 60 * 20 && !estourou; i++) {
        atualizarCorrida(&corrida, acelerar, quadro);
        estourou = corrida.jogador.pneuEstourado != 0;
    }
    verificar(estourou, "chegando aos 80% de desgaste o pneu estoura");
    verificar(corrida.jogador.pneuEstourado == 1 || corrida.jogador.pneuEstourado == 2, "e e um pneu dianteiro (1 ou 2)");
    verificar(!corrida.jogador.nitroLigado, "o nitro apaga");
    verificar(corrida.rivais[0].pneuEstourado == 0, "os rivais nao estouram");

    /* Rodando no aro, com alguem dirigindo (a IA): a velocidade cai ate perto de 55% do maximo, nao passa
     * disso e o carro continua na pista, ainda que puxe para o lado */
    Carro aro = criarCarro("Teste", 0, buscarSegmento(pista, 1000), 0.0f, 12000.0f, 2400.0f, 0.5f);
    aro.controladoPeloJogador = 1;
    aro.habilidade = 1.0f;
    aro.velocidade = aro.velocidadeMaxima;
    estourarPneu(&aro);
    float maior = 0.0f, menor = 1e9f;
    int saiu = 0;
    for (int i = 0; i < 60 * 25; i++) {
        aro.distanciaAFrente = -1.0f;
        atualizarCarro(&aro, atualizarIA(&aro, pista), quadro);
        if (i > 60 * 8) {
            maior = fmaxf(maior, aro.velocidade);
            menor = fminf(menor, aro.velocidade);
        }
        saiu |= fabsf(aro.deslocamentoLateral) > 1.3f;
    }
    float teto = aro.velocidadeMaxima * 0.55f;
    printf("     no aro, dirigido pela IA: velocidade entre %.0f e %.0f (teto %.0f, maxima %.0f)\n", menor, maior, teto, aro.velocidadeMaxima);
    verificar(maior <= teto * 1.02f, "no aro o carro nao passa de ~55% da velocidade maxima");
    verificar(menor > teto * 0.3f, "mas anda (da para chegar aos boxes)");
    verificar(!saiu, "e quem dirige consegue segurar o carro na pista");

    /* Puxa para o lado: sem mexer no volante o carro sai do lugar para o lado do pneu */
    Carro com = criarCarro("Teste", 0, buscarSegmento(pista, 1000), 0.0f, 12000.0f, 2400.0f, 0.5f);
    Carro sem = com;
    com.velocidade = sem.velocidade = 6000.0f;
    estourarPneu(&com);
    float lado = (com.pneuEstourado == 1) ? -1.0f : 1.0f;
    for (int i = 0; i < 60 * 2; i++) {
        atualizarCarro(&com, acelerar, quadro);
        atualizarCarro(&sem, acelerar, quadro);
    }
    printf("     deslocamento lateral em 2 s sem virar: com pneu estourado %.2f, normal %.2f (lado %+.0f)\n",
           com.deslocamentoLateral, sem.deslocamentoLateral, lado);
    verificar((com.deslocamentoLateral - sem.deslocamentoLateral) * lado > 0.3f, "o carro puxa para o lado do pneu estourado");

    /* Os quatro estouram aos poucos: um a cada marca de desgaste (80, 86, 92 e 97%), e cada um piora o carro */
    Carro aos = criarCarro("Teste", 0, buscarSegmento(pista, 1000), 0.0f, 12000.0f, 2400.0f, 0.5f);
    aos.controladoPeloJogador = 1;
    aos.gastaPneu = 1;
    aos.habilidade = 1.0f;
    aos.desgaste = 0.79f;
    aos.velocidade = aos.velocidadeMaxima;
    static const float MARCAS[4] = { 0.80f, 0.86f, 0.92f, 0.97f };
    static const float TETOS[5] = { 1.0f, 0.55f, 0.46f, 0.38f, 0.30f };
    int certo = 1, ordemCerta = 1, vistos = 0, anterior = 0;
    float maiorComQuatro = 0.0f;
    for (int i = 0; i < 60 * 60 * 6 && quantosPneusEstourados(&aos) < 4 + 1; i++) {
        aos.distanciaAFrente = -1.0f;
        atualizarCarro(&aos, atualizarIA(&aos, pista), quadro);
        int n = quantosPneusEstourados(&aos), devia = 0;
        for (int k = 0; k < 4; k++) {
            devia += (aos.desgaste >= MARCAS[k]);
        }
        certo &= (n == devia) && (n >= anterior);
        if (n > anterior) {
            ordemCerta &= (n > 1 || aos.pneuEstourado == 1 || aos.pneuEstourado == 2);   /* o primeiro e dianteiro */
            ordemCerta &= (n != 2 || (aos.pneuEstourado & 3) == 3);                        /* o segundo e o outro dianteiro */
            vistos |= 1 << n;
        }
        anterior = n;
        if (n == 4 && aos.tempoDoEstouro > 1.5f) {   /* depois da freada ate o teto novo */
            maiorComQuatro = fmaxf(maiorComQuatro, aos.velocidade);
            if (aos.desgaste >= 0.995f || i > 60 * 60 * 5) {
                break;
            }
        }
    }
    verificar(certo, "cada pneu estoura na sua marca de desgaste, na ordem, e nenhum volta sozinho");
    verificar(ordemCerta && aos.pneuEstourado == 15, "primeiro um dianteiro, depois o outro dianteiro, depois os de tras: os quatro");
    verificar(vistos == ((1 << 1) | (1 << 2) | (1 << 3) | (1 << 4)), "e estouraram um de cada vez (1, 2, 3 e 4 pneus)");
    printf("     com os quatro estourados: velocidade maxima vista %.0f (teto %.0f)\n", maiorComQuatro, aos.velocidadeMaxima * TETOS[4]);
    verificar(maiorComQuatro <= aos.velocidadeMaxima * TETOS[4] * 1.02f, "com os quatro no aro, nao passa de 30% da velocidade maxima");
    verificar(aderenciaAtual(&aos) < aderenciaAtual(&sem) * 0.6f, "e o carro segura bem menos");

    /* A troca nos boxes conserta */
    corrida.jogador.estadoDoPit = PIT_TROCANDO;
    corrida.jogador.tempoDaTroca = corrida.jogador.duracaoDaTroca;
    Comandos nada = { 0, 0, 0, 0, 0.0f, 0, 0 };
    atualizarCorrida(&corrida, nada, quadro);
    verificar(corrida.jogador.desgaste == 0.0f && corrida.jogador.pneuEstourado == 0, "a troca nos boxes conserta o pneu");

    /* No medio os pneus nao gastam, entao nunca estoura */
    iniciarCorrida(&corrida, pista, 0, NIVEL_MEDIO);
    corrida.contagemRegressiva = 0.0f;
    corrida.jogador.desgaste = 0.9f;
    for (int i = 0; i < 60 * 5; i++) {
        atualizarCorrida(&corrida, acelerar, quadro);
    }
    verificar(corrida.jogador.pneuEstourado == 0, "no medio nao estoura");

    /* A IA dirigindo o jogador no dificil, parando nos boxes na hora, nao estoura */
    correrComoIA(&corrida, pista, NIVEL_DIFICIL, 0.97f);
    verificar(corrida.terminou && corrida.jogador.pneuEstourado == 0, "parando no pit certo, a corrida inteira passa sem estourar");
    liberarPista(pista);
}

/*
 * vibracaoEm
 * Roda a vibração por um tempo, sem mexer no carro.
 * Recebe: a corrida, o estado e onde guardar o maior grave e o maior agudo.
 * Devolve: a vibracao do ultimo quadro.
 */
static Vibracao vibracaoEm(Corrida *corrida, EstadoDaVibracao *estado, float *maiorGrave, float *maiorAgudo) {
    Vibracao v = { 0.0f, 0.0f, 0.0f };
    *maiorGrave = 0.0f;
    *maiorAgudo = 0.0f;
    for (int i = 0; i < 40; i++) {
        v = calcularVibracao(estado, corrida, 1.0f / 60.0f);
        *maiorGrave = fmaxf(*maiorGrave, v.grave);
        *maiorAgudo = fmaxf(*maiorAgudo, v.agudo);
    }
    return v;
}

/*
 * testarVibracao
 * Cada sensacao do controle aparece quando deve, e so quando deve.
 */
static void testarVibracao(void) {
    printf("\n29) Vibracao do controle\n");
    Pista *pista = criarPista();
    static Corrida corrida;
    static EstadoDaVibracao estado;
    float grave, agudo;
    Carro *carro = &corrida.jogador;

    /* A base: reto, no meio da pista, numa reta */
    #define PREPARAR() do { \
        iniciarCorrida(&corrida, pista, 0, NIVEL_DIFICIL); \
        corrida.contagemRegressiva = 0.0f; \
        iniciarEstadoDaVibracao(&estado); \
        estado.numeroDaContagemAntes = 0; \
        carro->posicao.segmento = buscarSegmento(pista, 550); \
        carro->posicao.distanciaNoSegmento = 0.0f; \
        carro->deslocamentoLateral = 0.0f; \
        carro->velocidade = 7000.0f; \
        carro->motor.giro = 0.5f; \
        carro->motor.carga = 0.0f; \
        estado.marchaAntes = carro->motor.marcha; \
    } while (0)

    /* O asfalto: parado nao vibra; andando, esta sempre nas maos, e cresce com a velocidade */
    PREPARAR();
    carro->velocidade = 0.0f;
    vibracaoEm(&corrida, &estado, &grave, &agudo);
    verificar(grave < 0.02f && agudo < 0.02f, "parado, nao vibra");
    float velocidades[3] = { 3000.0f, 7000.0f, 11500.0f }, gravePorVelocidade[3], agudoPorVelocidade[3];
    for (int k = 0; k < 3; k++) {
        PREPARAR();
        carro->velocidade = velocidades[k];
        vibracaoEm(&corrida, &estado, &gravePorVelocidade[k], &agudoPorVelocidade[k]);
    }
    printf("     asfalto: grave %.2f / %.2f / %.2f, agudo %.2f / %.2f / %.2f (75, 175 e 290 km/h)\n",
           gravePorVelocidade[0], gravePorVelocidade[1], gravePorVelocidade[2], agudoPorVelocidade[0], agudoPorVelocidade[1], agudoPorVelocidade[2]);
    verificar(gravePorVelocidade[0] > 0.05f && agudoPorVelocidade[0] > 0.03f, "devagar, ja se sente o asfalto");
    verificar(gravePorVelocidade[2] > gravePorVelocidade[0] && agudoPorVelocidade[2] > agudoPorVelocidade[0] * 1.5f, "e mais, quanto mais rapido");
    verificar(gravePorVelocidade[2] < 0.3f, "sem cansar a mao");

    /* Ele tem textura: nao e uma vibracao lisa */
    PREPARAR();
    float menorAgudo = 1.0f, maiorAgudo = 0.0f;
    for (int i = 0; i < 60 * 2; i++) {
        Vibracao a = calcularVibracao(&estado, &corrida, 1.0f / 60.0f);
        menorAgudo = fminf(menorAgudo, a.agudo);
        maiorAgudo = fmaxf(maiorAgudo, a.agudo);
    }
    verificar(maiorAgudo > menorAgudo * 1.3f, "o asfalto tem textura");

    /* As juntas do piso: um toque de vez em quando */
    PREPARAR();
    int juntas = 0;
    for (int i = 0; i < 60 * 4; i++) {
        calcularVibracao(&estado, &corrida, 1.0f / 60.0f);
        juntas += (estado.golpeGrave > 0.1f);
    }
    printf("     quadros com junta em 4 s a 175 km/h: %d\n", juntas);
    verificar(juntas > 0, "as juntas do piso dao um toque");

    /* Derrapando, o chao some e entra o tremor */
    PREPARAR();
    vibracaoEm(&corrida, &estado, &grave, &agudo);
    float graveNormal = grave;
    PREPARAR();
    carro->derrapagem = 1.0f;
    float graveDerrapando, agudoDerrapando;
    vibracaoEm(&corrida, &estado, &graveDerrapando, &agudoDerrapando);
    printf("     derrapando: grave %.2f (no asfalto, %.2f), agudo %.2f\n", graveDerrapando, graveNormal, agudoDerrapando);
    verificar(graveDerrapando < graveNormal * 0.6f, "derrapando, o chao some da mao: o carro flutua");
    verificar(agudoDerrapando > 0.25f, "e o pneu treme na mao direita");
    float menorTremor = 1.0f, maiorTremor = 0.0f;
    for (int i = 0; i < 60; i++) {
        float a = calcularVibracao(&estado, &corrida, 1.0f / 60.0f).agudo;
        menorTremor = fminf(menorTremor, a);
        maiorTremor = fmaxf(maiorTremor, a);
    }
    verificar(maiorTremor > menorTremor * 1.6f, "e um tremor irregular, nao um zumbido liso");

    /* O drift: tranco ao soltar, peso na mao de fora, e tranco quando o pneu agarra */
    PREPARAR();
    calcularVibracao(&estado, &corrida, 1.0f / 60.0f);
    carro->forcaDoDrift = 1.0f;
    carro->anguloDeDrift = 30.0f;
    carro->derrapagem = 1.0f;
    float noTranco = calcularVibracao(&estado, &corrida, 1.0f / 60.0f).grave;
    verificar(noTranco > 0.4f, "a traseira soltando da um tranco");
    for (int i = 0; i < 40; i++) {
        calcularVibracao(&estado, &corrida, 1.0f / 60.0f);
    }
    float graveParaDireita = 0.0f, agudoParaDireita = 0.0f;
    vibracaoEm(&corrida, &estado, &graveParaDireita, &agudoParaDireita);
    carro->anguloDeDrift = -30.0f;
    float graveParaEsquerda = 0.0f, agudoParaEsquerda = 0.0f;
    vibracaoEm(&corrida, &estado, &graveParaEsquerda, &agudoParaEsquerda);
    verificar(graveParaDireita > graveParaEsquerda, "virando para a direita, a mao de fora e a esquerda");
    carro->forcaDoDrift = 0.0f;
    carro->anguloDeDrift = 0.0f;
    carro->derrapagem = 0.0f;
    float agarrou = calcularVibracao(&estado, &corrida, 1.0f / 60.0f).grave;
    verificar(agarrou > 0.25f, "o pneu agarrando da outro tranco");

    /* O nitro: empurrao, combustao pulsando, engasgo e cauda */
    PREPARAR();
    calcularVibracao(&estado, &corrida, 1.0f / 60.0f);
    carro->nitroLigado = 1;
    carro->nitro = 1.0f;
    float naIgnicao = calcularVibracao(&estado, &corrida, 1.0f / 60.0f).grave;
    for (int i = 0; i < 60; i++) {
        calcularVibracao(&estado, &corrida, 1.0f / 60.0f);
    }
    float menorNitro = 1.0f, maiorNitro = 0.0f;
    for (int i = 0; i < 30; i++) {
        float g = calcularVibracao(&estado, &corrida, 1.0f / 60.0f).grave;
        menorNitro = fminf(menorNitro, g);
        maiorNitro = fmaxf(maiorNitro, g);
    }
    printf("     nitro: ignicao %.2f, queimando %.2f a %.2f\n", naIgnicao, menorNitro, maiorNitro);
    verificar(naIgnicao > 0.7f && maiorNitro < naIgnicao * 0.75f, "ligando, o nitro da um empurrao maior que o resto");
    verificar(menorNitro > graveNormal && maiorNitro > menorNitro + 0.05f, "e segue empurrando, com a combustao pulsando");
    carro->nitro = 0.05f;
    float menorFim = 1.0f, maiorFim = 0.0f;
    for (int i = 0; i < 30; i++) {
        float g = calcularVibracao(&estado, &corrida, 1.0f / 60.0f).grave;
        menorFim = fminf(menorFim, g);
        maiorFim = fmaxf(maiorFim, g);
    }
    verificar(menorFim < maiorFim * 0.7f, "com o tanque acabando, ele engasga");
    carro->nitro = 1.0f;
    for (int i = 0; i < 10; i++) {
        calcularVibracao(&estado, &corrida, 1.0f / 60.0f);
    }
    carro->nitroLigado = 0;
    float cauda0 = calcularVibracao(&estado, &corrida, 1.0f / 60.0f).grave;
    for (int i = 0; i < 40; i++) {
        calcularVibracao(&estado, &corrida, 1.0f / 60.0f);
    }
    float cauda1 = calcularVibracao(&estado, &corrida, 1.0f / 60.0f).grave;
    verificar(cauda0 > cauda1 + 0.1f && cauda1 < 0.3f, "apagando, some aos poucos");

    /* Grama */
    PREPARAR();
    carro->deslocamentoLateral = 1.6f;
    vibracaoEm(&corrida, &estado, &grave, &agudo);
    verificar(grave > graveNormal * 1.3f && agudo > 0.1f, "a grama treme mais que o asfalto");

    /* Zebra: acha um segmento com zebra e conta os pulsos */
    PREPARAR();
    Segmento *comZebra = pista->largada;
    while (!temZebra(comZebra)) {
        comZebra = proximoSegmento(comZebra);
    }
    carro->posicao.segmento = comZebra;
    carro->deslocamentoLateral = 1.05f;
    int subidas = 0;
    float anterior = 0.0f, maiorZebra = 0.0f, menorZebra = 1.0f;
    for (int i = 0; i < 60; i++) {
        Vibracao z = calcularVibracao(&estado, &corrida, 1.0f / 60.0f);
        subidas += (z.grave > 0.35f && anterior <= 0.35f);
        anterior = z.grave;
        maiorZebra = fmaxf(maiorZebra, z.grave);
        menorZebra = fminf(menorZebra, z.grave);
    }
    printf("     zebra: %d pulsos por segundo\n", subidas);
    verificar(subidas >= 5 && subidas <= 16 && menorZebra < maiorZebra * 0.5f, "a zebra da pulsos que a mao consegue contar");
    PREPARAR();
    carro->posicao.segmento = comZebra;
    carro->deslocamentoLateral = 0.0f;
    vibracaoEm(&corrida, &estado, &grave, &agudo);
    verificar(grave < 0.3f, "no meio da pista, na mesma curva, nao tem zebra");

    /* Freada: o gatilho fica livre; o peso vai para a frente e o ABS so entra nas fortes */
    PREPARAR();
    carro->ultimosComandos.frear = 1;
    carro->velocidade = 10000.0f;
    vibracaoEm(&corrida, &estado, &grave, &agudo);
    float agudoFreando = agudo, graveFreando = grave;
    PREPARAR();
    carro->velocidade = 10000.0f;
    vibracaoEm(&corrida, &estado, &grave, &agudo);
    verificar(graveFreando > grave, "freando, o peso do carro vai para a frente");
    verificar(agudoFreando > agudo + 0.05f, "e numa freada forte o ABS pulsa");
    PREPARAR();
    carro->ultimosComandos.frear = 1;
    carro->velocidade = 2500.0f;
    vibracaoEm(&corrida, &estado, &grave, &agudo);
    verificar(agudo < 0.1f, "devagar, o ABS nao entra");

    /* Arrancada */
    PREPARAR();
    carro->ultimosComandos.acelerar = 1;
    carro->velocidade = 100.0f;
    Vibracao v = vibracaoEm(&corrida, &estado, &grave, &agudo);
    verificar(agudo > 0.2f && v.noAcelerador > 0.5f, "na arrancada, a roda patina na mao e no gatilho");

    /* Batida: de verdade, entre dois carros, mais forte com mais diferenca */
    float forcas[2];
    for (int k = 0; k < 2; k++) {
        PREPARAR();
        corrida.rivais[0].posicao = carro->posicao;
        corrida.rivais[0].distanciaPercorrida = carro->distanciaPercorrida;
        moverNaPista(&corrida.rivais[0].posicao, 2.0f * UNIDADES_POR_METRO);
        corrida.rivais[0].deslocamentoLateral = 0.0f;
        corrida.rivais[0].velocidade = 6900.0f - k * 3000.0f;
        for (int r = 1; r < TOTAL_RIVAIS; r++) {
            corrida.rivais[r].posicao.segmento = buscarSegmento(pista, 2000);
        }
        carro->pancada = 0.0f;
        Comandos nada = { 0, 0, 0, 0, 0.0f, 0, 0 };
        atualizarCorrida(&corrida, nada, 1.0f / 60.0f);
        forcas[k] = carro->pancada;
    }
    printf("     batida leve: %.2f; forte: %.2f\n", forcas[0], forcas[1]);
    verificar(forcas[0] > 0.0f && forcas[1] > forcas[0], "batendo, vibra mais forte com mais diferenca de velocidade");
    PREPARAR();
    carro->pancada = 1.0f;
    v = calcularVibracao(&estado, &corrida, 1.0f / 60.0f);
    float noInicio = v.grave;
    for (int i = 0; i < 30; i++) {
        v = calcularVibracao(&estado, &corrida, 1.0f / 60.0f);
    }
    verificar(noInicio > 0.8f && v.grave < noInicio * 0.35f && carro->pancada == 0.0f, "o tranco da batida some em meio segundo");

    /* Marcha */
    PREPARAR();
    calcularVibracao(&estado, &corrida, 1.0f / 60.0f);
    float graveAntesDaMarcha = calcularVibracao(&estado, &corrida, 1.0f / 60.0f).grave;
    carro->motor.marcha++;
    v = calcularVibracao(&estado, &corrida, 1.0f / 60.0f);
    verificar(v.grave > graveAntesDaMarcha + 0.05f, "cada troca de marcha da um toque");

    /* Muro e rodada */
    PREPARAR();
    carro->raspandoNoMuro = 1;
    vibracaoEm(&corrida, &estado, &grave, &agudo);
    verificar(agudo > 0.25f, "raspando no muro, vibra");
    PREPARAR();
    carro->rodando = 1;
    vibracaoEm(&corrida, &estado, &grave, &agudo);
    verificar(grave > 0.25f && agudo > 0.15f, "rodando, sacode");

    /* Pneu gasto, so virando */
    PREPARAR();
    carro->desgaste = 1.0f;
    vibracaoEm(&corrida, &estado, &grave, &agudo);
    float gastoReto = agudo;
    carro->volante = 1.0f;
    vibracaoEm(&corrida, &estado, &grave, &agudo);
    verificar(agudo > gastoReto + 0.03f, "o pneu gasto avisa ao virar");

    /* A pistola do Guido so pulsa na roda; nos boxes o resto nao vibra */
    PREPARAR();
    carro->estadoDoPit = PIT_TROCANDO;
    estado.estadoDoPitAntes = PIT_TROCANDO;
    carro->velocidade = 0.0f;
    carro->duracaoDaTroca = DURACAO_DA_TROCA;
    carro->tempoDaTroca = SEGUNDOS_PARA_A_RODA + 0.1f;
    vibracaoEm(&corrida, &estado, &grave, &agudo);
    verificar(agudo > 0.6f, "o Guido trocando a roda vibra forte");
    carro->tempoDaTroca = 0.05f;
    estado.golpeGrave = estado.golpeAgudo = 0.0f;
    float agudoIndo = calcularVibracao(&estado, &corrida, 1.0f / 60.0f).agudo;
    verificar(agudoIndo < 0.01f, "indo de uma roda para a outra, nao");

    /* Largada e chegada */
    iniciarCorrida(&corrida, pista, 0, NIVEL_DIFICIL);
    iniciarEstadoDaVibracao(&estado);
    int toques = 0;
    float graveAntes = 0.0f;
    Comandos nada2 = { 0, 0, 0, 0, 0.0f, 0, 0 };
    for (int i = 0; i < 60 * 8 && corrida.contagemRegressiva > -0.1f; i++) {
        atualizarCorrida(&corrida, nada2, 1.0f / 60.0f);
        float g = calcularVibracao(&estado, &corrida, 1.0f / 60.0f).grave;
        toques += (g > 0.2f && g > graveAntes + 0.1f);
        graveAntes = g;
        if (corrida.contagemRegressiva <= 0.0f) {
            corrida.contagemRegressiva = -1.0f;
        }
    }
    printf("     toques na contagem: %d\n", toques);
    verificar(toques >= 3, "a contagem da um toque a cada numero e um tranco no GO");

    PREPARAR();
    calcularVibracao(&estado, &corrida, 1.0f / 60.0f);
    corrida.terminou = 1;
    v = calcularVibracao(&estado, &corrida, 1.0f / 60.0f);
    float naChegada = v.grave;
    for (int i = 0; i < 60; i++) {
        v = calcularVibracao(&estado, &corrida, 1.0f / 60.0f);
    }
    verificar(naChegada > 0.6f && v.grave < 0.01f, "a bandeirada da um tranco, e depois o controle fica quieto");

    /* Nunca passa do maximo, com tudo ao mesmo tempo */
    PREPARAR();
    carro->deslocamentoLateral = 1.6f;
    carro->derrapagem = 1.0f;
    carro->forcaDoDrift = 1.0f;
    carro->nitroLigado = 1;
    carro->rodando = 1;
    carro->raspandoNoMuro = 1;
    carro->pancada = 1.0f;
    vibracaoEm(&corrida, &estado, &grave, &agudo);
    verificar(grave <= 1.0f && agudo <= 1.0f, "com tudo junto, nao passa do maximo");
    liberarPista(pista);
    #undef PREPARAR
}

/*
 * testarConfiguracoes
 * Trocar as teclas, salvar, carregar, e o que a vibracao obedece.
 */
static void testarConfiguracoes(void) {
    printf("\n30) Configuracoes\n");
    const char *arquivo = "build/teste_configuracoes.txt";
    Configuracoes config, lida;
    configuracoesPadrao(&config);

    /* Cada comando comeca com uma tecla e um botao, sem repetir */
    int semTecla = 0, repetidas = 0;
    for (int a = 0; a < TOTAL_ACOES; a++) {
        semTecla += (config.comandos[a][COLUNA_TECLA_1] == SEM_COMANDO) || (config.comandos[a][COLUNA_BOTAO_1] == SEM_COMANDO);
        for (int inicio = 0; inicio <= 2; inicio += 2) {
            for (int c = inicio; c < inicio + 2; c++) {
                for (int b = 0; b < TOTAL_ACOES; b++) {
                    for (int d = inicio; d < inicio + 2; d++) {
                        repetidas += (b != a || d != c) && config.comandos[a][c] != SEM_COMANDO && config.comandos[a][c] == config.comandos[b][d];
                    }
                }
            }
        }
    }
    verificar(semTecla == 0 && repetidas == 0, "o padrao tem tudo, sem nada repetido");

    /* No controle, um comando em cada botao, nos lugares de costume */
    verificar(config.comandos[ACAO_ACELERAR][COLUNA_BOTAO_1] == 12 && config.comandos[ACAO_FREAR][COLUNA_BOTAO_1] == 10, "R2 acelera e L2 freia");
    verificar(config.comandos[ACAO_FREIO_DE_MAO][COLUNA_BOTAO_1] == 7, "o freio de mao fica na Cruz");
    verificar(config.comandos[ACAO_NITRO][COLUNA_BOTAO_1] == 6, "o nitro fica na Bola");
    int segundoBotao = 0;
    for (int a = 0; a < TOTAL_ACOES; a++) {
        segundoBotao += config.comandos[a][COLUNA_BOTAO_2] != SEM_COMANDO;
    }
    verificar(segundoBotao == 0, "nenhum comando vem repetido em dois botoes");

    /* Trocando para uma tecla ja usada, as duas trocam de lugar */
    int doFreio = config.comandos[ACAO_FREAR][COLUNA_TECLA_1], doNitro = config.comandos[ACAO_NITRO][COLUNA_TECLA_1];
    definirComando(&config, ACAO_NITRO, COLUNA_TECLA_1, doFreio);
    verificar(config.comandos[ACAO_NITRO][COLUNA_TECLA_1] == doFreio && config.comandos[ACAO_FREAR][COLUNA_TECLA_1] == doNitro, "tecla repetida troca de lugar com a outra");
    definirComando(&config, ACAO_NITRO, COLUNA_TECLA_2, 70);
    verificar(config.comandos[ACAO_NITRO][COLUNA_TECLA_2] == 70, "tecla livre entra direto");

    /* A mesma tecla e o mesmo botao podem ter o mesmo numero: sao aparelhos diferentes */
    Configuracoes aparelhos;
    configuracoesPadrao(&aparelhos);
    int botaoDoAcelerar = aparelhos.comandos[ACAO_ACELERAR][COLUNA_BOTAO_1];
    definirComando(&aparelhos, ACAO_FREAR, COLUNA_TECLA_1, botaoDoAcelerar);
    verificar(aparelhos.comandos[ACAO_ACELERAR][COLUNA_BOTAO_1] == botaoDoAcelerar, "uma tecla nao mexe nos botoes do controle");

    /* Limpar: nunca a ultima do comando */
    configuracoesPadrao(&config);
    verificar(limparComando(&config, ACAO_ACELERAR, COLUNA_TECLA_2) && config.comandos[ACAO_ACELERAR][COLUNA_TECLA_2] == SEM_COMANDO, "da para tirar a segunda tecla");
    verificar(!limparComando(&config, ACAO_ACELERAR, COLUNA_TECLA_1) && config.comandos[ACAO_ACELERAR][COLUNA_TECLA_1] != SEM_COMANDO, "a ultima tecla de um comando nao sai");

    /* Salvar e carregar */
    configuracoesPadrao(&config);
    definirComando(&config, ACAO_NITRO, COLUNA_TECLA_1, 78);
    definirComando(&config, ACAO_NITRO, COLUNA_BOTAO_2, 16);
    config.vibracaoLigada = 0;
    config.forcaDaVibracao = 40;
    config.gatilhosAdaptativos = 0;
    config.zonaMorta = 20;
    config.respostaDoVolante = VOLANTE_DIRETO;
    config.inverterCameraY = 1;
    verificar(salvarConfiguracoes(&config, arquivo), "salva o arquivo");
    verificar(carregarConfiguracoes(&lida, arquivo) && memcmp(&config, &lida, sizeof(Configuracoes)) == 0, "o que voltou do arquivo e igual");

    /* Sem arquivo, ou com ele estragado */
    remove(arquivo);
    Configuracoes padrao;
    configuracoesPadrao(&padrao);
    verificar(!carregarConfiguracoes(&lida, arquivo) && memcmp(&padrao, &lida, sizeof(Configuracoes)) == 0, "sem arquivo, fica o padrao");
    FILE *estragado = fopen(arquivo, "w");
    fprintf(estragado, "lixo total\nvibracao 7\nforca 5000\nzona_morta -3\nvolante 99\ncomando nitro 0 0 0 0\ncomando inexistente 1 2 3 4\ncamera_y 1\n");
    fclose(estragado);
    carregarConfiguracoes(&lida, arquivo);
    verificar(lida.vibracaoLigada == 1 && lida.forcaDaVibracao == 100 && lida.zonaMorta == 12 && lida.respostaDoVolante == VOLANTE_NORMAL, "valores absurdos ficam no padrao");
    verificar(lida.comandos[ACAO_NITRO][COLUNA_TECLA_1] != SEM_COMANDO && lida.comandos[ACAO_NITRO][COLUNA_BOTAO_1] != SEM_COMANDO, "nenhum comando fica sem tecla, nem com o arquivo estragado");
    verificar(lida.inverterCameraY == 1, "e o que estava certo no arquivo vale");
    remove(arquivo);

    /* A vibracao obedece */
    Vibracao cheia = { 0.8f, 0.6f, 0.4f };
    configuracoesPadrao(&config);
    Vibracao v = vibracaoAoGosto(cheia, &config);
    verificar(v.grave == cheia.grave && v.noAcelerador == cheia.noAcelerador, "no padrao, a vibracao passa inteira");
    config.vibracaoLigada = 0;
    v = vibracaoAoGosto(cheia, &config);
    verificar(v.grave == 0.0f && v.agudo == 0.0f && v.noAcelerador == 0.0f, "desligada, nada vibra");
    config.vibracaoLigada = 1;
    config.forcaDaVibracao = 50;
    v = vibracaoAoGosto(cheia, &config);
    verificar(fabsf(v.grave - 0.4f) < 0.001f && fabsf(v.agudo - 0.3f) < 0.001f, "a forca escala a vibracao");
    config.forcaDaVibracao = 100;
    config.gatilhosAdaptativos = 0;
    v = vibracaoAoGosto(cheia, &config);
    verificar(v.grave == cheia.grave && v.noAcelerador == 0.0f, "sem o tremor no gatilho, as maos continuam vibrando");

    /* A exibicao: resolucao, modo de tela, vsync, limite de quadros e FPS */
    configuracoesPadrao(&config);
    verificar(config.resolucao == 0 && config.modoDeTela == MODO_TELA_CHEIA && config.sincroniaVertical == 1
              && quadrosPorSegundoDoLimite(config.limiteDeQuadros) == 60 && config.mostrarFps == 0,
              "padrao da exibicao: automatica, tela cheia, vsync ligado, 60 FPS, sem contador");
    int l = 0, a2 = 0, maiorL = 0, maiorA = 0, ordenada = 1, anteriorArea = 0;
    for (int i = 1; i < TOTAL_RESOLUCOES; i++) {
        verificar(resolucaoDaTabela(i, &l, &a2) && l > 0 && a2 > 0 && l <= MAIOR_LARGURA && a2 <= MAIOR_ALTURA, "toda resolucao da tabela fica ate 1920x1080");
        ordenada &= (l * a2 > anteriorArea);
        anteriorArea = l * a2;
        maiorL = l > maiorL ? l : maiorL;
        maiorA = a2 > maiorA ? a2 : maiorA;
    }
    verificar(ordenada && maiorL == 1920 && maiorA == 1080, "a tabela vai crescendo e a maior e 1920x1080");
    verificar(!resolucaoDaTabela(TOTAL_RESOLUCOES, &l, &a2) && !resolucaoDaTabela(-1, &l, &a2), "indice fora da tabela nao existe");
    verificar(resolucaoCabe(0, 640, 480) && !resolucaoCabe(6, 1600, 900) && resolucaoCabe(6, 1920, 1080) && resolucaoCabe(3, 1920, 1080), "cabe no monitor: so o que nao passa dele");

    /* A automatica: a maior que cabe (nunca passa de 1920x1080, nem num monitor 4K); em janela, com folga */
    config.resolucao = 0;
    config.modoDeTela = MODO_TELA_CHEIA;
    resolucaoEfetiva(&config, 1920, 1080, &l, &a2);
    verificar(l == 1920 && a2 == 1080, "tela cheia automatica num monitor 1080p: 1920x1080");
    resolucaoEfetiva(&config, 3840, 2160, &l, &a2);
    verificar(l == 1920 && a2 == 1080, "e num monitor 4K ela nao passa de 1920x1080");
    resolucaoEfetiva(&config, 1366, 768, &l, &a2);
    verificar(l == 1366 && a2 == 768, "num monitor 1366x768, o tamanho dele");
    config.modoDeTela = MODO_JANELA;
    resolucaoEfetiva(&config, 1920, 1080, &l, &a2);
    verificar(l == 1600 && a2 == 900, "em janela automatica num monitor 1080p: 1600x900, sobra espaco para as bordas");
    config.resolucao = 3;
    resolucaoEfetiva(&config, 1920, 1080, &l, &a2);
    verificar(l == 1280 && a2 == 720, "a resolucao escolhida vale como esta");
    resolucaoEfetiva(&config, 1024, 768, &l, &a2);
    verificar(l <= 1024 && a2 <= 768, "se ela nao cabe no monitor, cai para uma que cabe");

    verificar(quadrosPorSegundoDoLimite(0) == 30 && quadrosPorSegundoDoLimite(TOTAL_LIMITES_DE_QUADROS - 1) == 0
              && quadrosPorSegundoDoLimite(99) == 60, "limites de quadros: de 30 a sem limite; fora da tabela volta a 60");

    /* A exibicao vai e volta do arquivo; valores absurdos ficam no padrao */
    config.resolucao = 5;
    config.modoDeTela = MODO_JANELA;
    config.sincroniaVertical = 0;
    config.limiteDeQuadros = 3;
    config.mostrarFps = 1;
    verificar(salvarConfiguracoes(&config, arquivo) && carregarConfiguracoes(&lida, arquivo) && memcmp(&config, &lida, sizeof(Configuracoes)) == 0,
              "a exibicao volta igual do arquivo");
    estragado = fopen(arquivo, "w");
    fprintf(estragado, "resolucao 99\nmodo_de_tela 5\nsincronia_vertical 2\nlimite_de_quadros -1\nmostrar_fps 1\n");
    fclose(estragado);
    carregarConfiguracoes(&lida, arquivo);
    verificar(lida.resolucao == 0 && lida.modoDeTela == MODO_TELA_CHEIA && lida.sincroniaVertical == 1
              && lida.limiteDeQuadros == LIMITE_PADRAO_DE_QUADROS && lida.mostrarFps == 1, "exibicao estragada no arquivo: so o que esta certo vale");
    remove(arquivo);
}

/*
 * testarMotorGranular
 * O motor do jogador montado em pedacinhos da gravacao: o tom segue a rotacao, cada marcha usa a sua
 * arrancada, e a saida e limpa (sem estalos, sem passar do limite).
 */
static void testarMotorGranular(void) {
    printf("\n32b) Motor da gravacao (nitro_car): tom pela rotacao e marcha\n");
    verificar(fabsf(tomDoGiro(RPM_DA_TROCA / RPM_MAXIMO) - 322.0f) < 0.5f, "na linha vermelha do jogo, o tom e o da troca de marcha da gravacao");
    verificar(tomDoGiro(0.2f) < tomDoGiro(0.4f) && tomDoGiro(0.4f) < tomDoGiro(0.8f), "mais giro, mais tom");

    /* A tabela: o ponto da gravacao anda para a frente com o tom, e uma marcha de cima sobe como a de baixo */
    for (int marcha = 0; marcha < 3; marcha++) {
        float anterior = -1.0f, anteriorTom = 0.0f;
        int cresce = 1, tomCerto = 1;
        for (float tom = 60.0f; tom <= 322.0f; tom += 4.0f) {
            float segundos, tomDaGravacao;
            int dentro = pontoDaGravacao(marcha, tom, &segundos, &tomDaGravacao);
            if (dentro) {
                tomCerto &= fabsf(tomDaGravacao - tom) < 0.01f;   /* dentro da marcha, o ponto tem exatamente o tom pedido */
                if (anterior >= 0.0f && tomDaGravacao > anteriorTom) {
                    cresce &= (segundos >= anterior - 1.9f);       /* so a emenda da 1a marcha (1,3 s -> 3,1 s) pula, e para a frente */
                }
                anterior = segundos;
                anteriorTom = tomDaGravacao;
            }
        }
        verificar(cresce && tomCerto, "o ponto da gravacao acompanha o tom pedido");
    }
    float segundos, tomDaGravacao;
    verificar(pontoDaGravacao(1, 100.0f, &segundos, &tomDaGravacao) == 0 && tomDaGravacao > 200.0f, "tom que a marcha da gravacao nao tem: fica na ponta e toca mais lento");
    float s1, t1, s2, t2, s3, t3;
    pontoDaGravacao(0, 250.0f, &s1, &t1);
    pontoDaGravacao(1, 250.0f, &s2, &t2);
    pontoDaGravacao(4, 250.0f, &s3, &t3);
    verificar(s1 < 5.0f && s2 > 5.0f && s2 < 9.5f && s3 > 9.5f, "a 1a, a 2a e a 3a marcha usam subidas diferentes da gravacao (e da 3a em diante e a mesma)");

    /* A saida, com uma gravacao de mentira: um tom que sobe com o tempo, 20 s a 44100 Hz */
    int total = 20 * TAXA_DO_MOTOR_GRANULAR;
    float *fonte = malloc(sizeof(float) * (size_t) total);
    float fase = 0.0f;
    for (int i = 0; i < total; i++) {
        float t = (float) i / TAXA_DO_MOTOR_GRANULAR;
        fase += 2.0f * 3.14159265f * (100.0f + 12.0f * t) / TAXA_DO_MOTOR_GRANULAR;
        fonte[i] = 0.3f * sinf(fase) + 0.15f * sinf(2.0f * fase) + 0.07f * sinf(3.0f * fase);
    }
    MotorGranular motor;
    verificar(!iniciarMotorGranular(&motor, fonte, 5 * TAXA_DO_MOTOR_GRANULAR), "uma gravacao curta demais nao serve");
    verificar(!iniciarMotorGranular(&motor, NULL, 0), "sem gravacao nao serve");
    verificar(iniciarMotorGranular(&motor, fonte, total), "uma gravacao de 20 s serve");
    static float saida[TAXA_DO_MOTOR_GRANULAR];
    definirMotorGranular(&motor, 2, 0.6f, 0.0f);
    gerarMotorGranular(&motor, saida, 4096);
    float maior = 0.0f;
    for (int i = 0; i < 4096; i++) {
        maior = fmaxf(maior, fabsf(saida[i]));
    }
    verificar(maior == 0.0f, "com volume zero, silencio");
    int limpo = 1, finito = 1, barulho = 0;
    float picoDoSom = 0.0f, maiorSalto = 0.0f, anterior = 0.0f;
    for (int bloco = 0; bloco < 30; bloco++) {   /* meio segundo de motor, com a marcha e o giro mudando no meio */
        definirMotorGranular(&motor, (bloco < 15) ? 1 : 3, (bloco < 15) ? 0.55f : 0.70f, 1.0f);
        gerarMotorGranular(&motor, saida, 735);
        for (int i = 0; i < 735; i++) {
            finito &= isfinite(saida[i]);
            picoDoSom = fmaxf(picoDoSom, fabsf(saida[i]));
            if (bloco > 3) {
                maiorSalto = fmaxf(maiorSalto, fabsf(saida[i] - anterior));
            }
            anterior = saida[i];
            barulho |= fabsf(saida[i]) > 0.05f;
        }
    }
    limpo = (picoDoSom < 1.5f) && (maiorSalto < 0.6f);
    verificar(finito && barulho, "com volume, sai som, e so numeros normais");
    verificar(limpo, "sem estalos (nenhum salto grande de uma amostra para a outra) e sem passar do limite");
    free(fonte);
}

/*
 * testarCurvaNaMaoDoPiloto
 * Sem volante o carro nao faz a curva sozinho, e o limite de cada curva continua o mesmo.
 */
static void testarCurvaNaMaoDoPiloto(void) {
    printf("\n31) A curva na mao do piloto\n");
    float quadro = 1.0f / 60.0f;
    Pista *pista = criarPista();

    /* Acha uma curva leve e longa: a Subida dos Boxes */
    Segmento *leve = pista->largada;
    while (strcmp(leve->nomeTrecho, "Subida dos Boxes") != 0) {
        leve = proximoSegmento(leve);
    }
    Segmento *maisCurva = leve;
    for (Segmento *s = leve; strcmp(s->nomeTrecho, "Subida dos Boxes") == 0; s = proximoSegmento(s)) {
        if (fabsf(s->curvatura) > fabsf(maisCurva->curvatura)) {
            maisCurva = s;
        }
    }
    Comandos soAcelerar = { 1, 0, 0, 0, 0.0f, 0, 0 };

    /* Sem volante, ele sai da pista, mesmo devagar */
    Carro solto = criarCarro("Teste", 0, maisCurva, 0.0f, 12000.0f, 2400.0f, 0.5f);
    float segura = velocidadeSeguraNaCurva(&solto, maisCurva->curvatura);
    float velocidade = segura * 0.5f;
    float tempo = 0.0f, angulo = 0.0f;
    while (tempo < 12.0f && fabsf(solto.deslocamentoLateral) <= 1.0f) {
        solto.velocidade = velocidade;
        atualizarCarro(&solto, soAcelerar, quadro);
        tempo += quadro;
        angulo = fmaxf(angulo, fabsf(solto.anguloDeFuga));
    }
    printf("     Subida dos Boxes a %d km/h, sem volante: sai em %.1f s, bico %.0f graus para fora\n", converterParaKmh(velocidade), tempo, angulo);
    verificar(tempo < 12.0f, "numa curva leve, sem volante, o carro sai da pista");
    verificar(angulo > 2.0f, "e o bico nao acompanha a curva");
    float paraFora = (maisCurva->curvatura > 0.0f) ? -1.0f : 1.0f;
    verificar(solto.deslocamentoLateral * paraFora > 0.0f, "ele sai pelo lado de fora");

    /* Virando o bastante, ele faz a curva: o bico acompanha e fica na pista */
    Carro virando = criarCarro("Teste", 0, maisCurva, 0.0f, 12000.0f, 2400.0f, 0.5f);
    float direcao = (maisCurva->curvatura > 0.0f) ? 1.0f : -1.0f;
    Comandos noVolante = { 1, 0, 0, 0, direcao, 0, 0 };
    float maiorLado = 0.0f;
    for (int i = 0; i < 60 * 3; i++) {
        virando.velocidade = velocidade;
        noVolante.direcao = direcao * fminf(fmaxf(0.25f - virando.deslocamentoLateral * direcao * 2.0f, 0.0f), 1.0f);
        atualizarCarro(&virando, noVolante, quadro);
        maiorLado = fmaxf(maiorLado, fabsf(virando.deslocamentoLateral));
    }
    printf("     virando: lateral maxima %.2f, bico %.1f graus\n", maiorLado, virando.anguloDeFuga);
    verificar(maiorLado < 1.0f, "virando, ele faz a curva");
    verificar(fabsf(virando.anguloDeFuga) < angulo * 0.5f, "e o bico acompanha a pista");

    /* Na reta, nada disso existe */
    Segmento *reta = buscarSegmento(pista, 550);
    Carro naReta = criarCarro("Teste", 0, reta, 0.0f, 12000.0f, 2400.0f, 0.5f);
    naReta.velocidade = 8000.0f;
    atualizarCarro(&naReta, soAcelerar, quadro);
    verificar(fabsf(naReta.anguloDeFuga) < 0.5f, "na reta, o bico fica reto");

    /* A velocidade segura nao mudou: no limite, o volante todo segura a curva */
    Segmento *fechada = pista->largada;
    for (Segmento *s = pista->largada->proximo; s != pista->largada; s = proximoSegmento(s)) {
        if (fabsf(s->curvatura) > fabsf(fechada->curvatura)) {
            fechada = s;
        }
    }
    float sentido = (fechada->curvatura > 0.0f) ? 1.0f : -1.0f;
    Carro noLimite = criarCarro("Teste", 0, fechada, 0.0f, 12000.0f, 2400.0f, 0.5f);
    Comandos volanteTodo = { 1, 0, sentido < 0.0f, sentido > 0.0f, 0.0f, 0, 0 };
    float velocidadeSegura = velocidadeSeguraAcelerando(&noLimite, fechada->curvatura) * 0.98f;
    float inicio = 0.0f;
    for (int i = 0; i < 30; i++) {
        noLimite.posicao.segmento = fechada;
        noLimite.posicao.distanciaNoSegmento = 0.0f;
        noLimite.velocidade = velocidadeSegura;
        atualizarCarro(&noLimite, volanteTodo, quadro);
        if (i == 15) {
            inicio = noLimite.deslocamentoLateral;
        }
    }
    printf("     curva mais fechada, no limite, volante todo: de lado %+.3f -> %+.3f\n", inicio, noLimite.deslocamentoLateral);
    verificar((noLimite.deslocamentoLateral - inicio) * sentido >= -0.001f, "no limite, o volante todo ainda segura a curva");
    liberarPista(pista);
}

/*
 * testarCambio
 * Depois de subir, sobra rotacao; a reduzida depende do que o piloto esta fazendo.
 */
static void testarCambio(void) {
    printf("\n32) Cambio\n");
    float quadro = 1.0f / 60.0f;

    /* Acelerando: a marcha nova nasce longe do corte */
    Motor motor;
    iniciarMotor(&motor);
    float fracao = 0.0f, maiorDepois = 0.0f;
    int marcha = 0, trocas = 0;
    for (int i = 0; i < 60 * 30 && fracao < 1.0f; i++) {
        fracao += quadro * 0.06f;
        int antes = motor.marcha;
        atualizarMotor(&motor, fracao, 1, 0, 0, quadro);
        if (motor.marcha > antes) {
            marcha = motor.marcha;
            trocas++;
        }
        if (marcha > 0 && !estaTrocandoDeMarcha(&motor)) {
            printf("     %da marcha nasce em %.0f rpm\n", marcha + 1, motor.giro * RPM_MAXIMO);
            maiorDepois = fmaxf(maiorDepois, motor.giro * RPM_MAXIMO);
            marcha = 0;
        }
    }
    verificar(trocas == TOTAL_MARCHAS - 1, "acelerando, passa por todas as marchas");
    verificar(maiorDepois < 6900.0f, "depois de subir, a rotacao cai para longe do corte");
    atualizarMotor(&motor, 1.0f, 1, 0, 0, quadro);
    float naMaxima = 0.0f;
    for (int i = 0; i < 60; i++) {
        atualizarMotor(&motor, 1.0f, 1, 0, 0, quadro);
        naMaxima = motor.giro * RPM_MAXIMO;
    }
    printf("     na velocidade maxima: %.0f rpm\n", naMaxima);
    verificar(naMaxima < RPM_DA_TROCA, "na velocidade maxima, o motor nao bate no corte");

    /* Freando reduz mais cedo que aliviando */
    float reduziuFreando = 0.0f, reduziuAliviando = 0.0f;
    for (int caso = 0; caso < 2; caso++) {
        iniciarMotor(&motor);
        motor.marcha = TOTAL_MARCHAS - 1;
        motor.tempoNaMarcha = 1.0f;
        float f = 1.0f;
        while (f > 0.0f && motor.marcha == TOTAL_MARCHAS - 1) {
            f -= quadro * 0.1f;
            atualizarMotor(&motor, f, 0, caso == 0, 0, quadro);
        }
        if (caso == 0) {
            reduziuFreando = f;
        } else {
            reduziuAliviando = f;
        }
    }
    printf("     da 7a para a 6a: freando a %.0f%% da maxima, aliviando a %.0f%%\n", reduziuFreando * 100.0f, reduziuAliviando * 100.0f);
    verificar(reduziuFreando > reduziuAliviando + 0.1f, "freando, reduz bem antes do que aliviando");

    /* Aliviando, a marcha nao sobe nem reduz a toa */
    iniciarMotor(&motor);
    motor.marcha = 3;
    motor.tempoNaMarcha = 1.0f;
    motor.giro = 0.85f;
    for (int i = 0; i < 30; i++) {
        atualizarMotor(&motor, 0.40f, 0, 0, 0, quadro);
    }
    verificar(motor.marcha == 3, "aliviando, a marcha fica onde esta");

    /* Kickdown: pisando com o motor fraco, ele reduz; com o motor forte, nao */
    iniciarMotor(&motor);
    motor.marcha = 5;
    motor.tempoNaMarcha = 2.0f;
    motor.giro = 0.45f;
    float devagar = 0.30f;
    for (int i = 0; i < 40; i++) {
        atualizarMotor(&motor, devagar, 1, 0, 0, quadro);
    }
    printf("     retomada a 30%% da maxima: da 6a para a %da, %.0f rpm\n", motor.marcha + 1, motor.giro * RPM_MAXIMO);
    verificar(motor.marcha < 5, "pisando com o motor fraco, o cambio reduz");
    verificar(motor.giro * RPM_MAXIMO > 4700.0f && motor.giro * RPM_MAXIMO < RPM_DA_TROCA, "e o motor fica na faixa de forca");
    iniciarMotor(&motor);
    motor.marcha = 3;
    motor.tempoNaMarcha = 2.0f;
    motor.giro = 0.70f;
    for (int i = 0; i < 20; i++) {
        atualizarMotor(&motor, 0.34f, 1, 0, 0, quadro);
    }
    verificar(motor.marcha == 3, "com o motor forte, acelerar nao reduz");

    /* Sem trocar sem parar, numa velocidade que fica entre duas marchas */
    iniciarMotor(&motor);
    motor.marcha = 2;
    int mudancas = 0, ultima = motor.marcha;
    for (int i = 0; i < 60 * 6; i++) {
        float f = 0.29f + 0.015f * sinf(i * 0.08f);
        atualizarMotor(&motor, f, 1, 0, 0, quadro);
        mudancas += motor.marcha != ultima;
        ultima = motor.marcha;
    }
    printf("     6 s acelerando numa velocidade entre duas marchas: %d trocas\n", mudancas);
    verificar(mudancas <= 2, "o cambio nao fica trocando sem parar");

    /* De lado, a marcha fica; e a reduzida da uma acelerada */
    iniciarMotor(&motor);
    motor.marcha = 2;
    motor.giro = 0.95f;
    motor.tempoNaMarcha = 1.0f;
    atualizarMotor(&motor, 0.40f, 1, 0, 1, quadro);
    verificar(motor.marcha == 2, "no drift, a marcha nao sobe");
    iniciarMotor(&motor);
    motor.marcha = 4;
    motor.tempoNaMarcha = 1.0f;
    motor.giro = 0.55f;
    motor.carga = 0.0f;
    atualizarMotor(&motor, 0.30f, 0, 1, 0, quadro);
    float cargaNaReduzida = motor.carga;
    atualizarMotor(&motor, 0.30f, 0, 1, 0, quadro);
    verificar(motor.reduzindo && motor.carga > cargaNaReduzida, "ao reduzir, o motor da a acelerada de punta-taco");
}

/*
 * testarTransitoNoPit
 * Muitos carros nos boxes ao mesmo tempo: ninguem atravessa ninguem, e todos saem.
 */
static void testarTransitoNoPit(void) {
    printf("\n33) Transito no pit lane\n");
    float quadro = 1.0f / 60.0f;
    Pista *pista = criarPista();
    static Corrida corrida;
    Comandos nada = { 0, 0, 0, 0, 0.0f, 0, 0 };

    /* Todos entram quase juntos, um atras do outro, na mesma faixa */
    for (int espaco = 0; espaco < 2; espaco++) {
        float vao = espaco ? 12.0f : 7.0f;
        iniciarCorrida(&corrida, pista, 0, NIVEL_MEDIO);
        corrida.contagemRegressiva = 0.0f;
        corrida.terminou = 0;
        for (int i = 0; i < TOTAL_CARROS; i++) {
            Carro *carro = corrida.classificacao[i];
            carro->voltas = 1;
            carro->pitFeito = 0;
            carro->duracaoDaTroca = (i % 3 == 0) ? 3.2f : 5.0f;
            carro->posicao.segmento = buscarSegmento(pista, (int) ((ENTRADA_DO_PIT + 60.0f + (TOTAL_CARROS - i) * vao) * SEGMENTOS_POR_METRO));
            carro->posicao.distanciaNoSegmento = 0.0f;
            carro->distanciaPercorrida = (ENTRADA_DO_PIT + 60.0f + (TOTAL_CARROS - i) * vao) * UNIDADES_POR_METRO;
            carro->deslocamentoLateral = -3.1f;
            carro->velocidade = 3000.0f;
            entrarNoPit(carro);
        }
        float maiorEntrada = 0.0f;
        int quadrosEncostados = 0, moveuNaTroca = 0, naPista = 0;
        float lugarNaTroca[TOTAL_CARROS] = { 0.0f };
        for (int q = 0; q < 60 * 150 && naPista < TOTAL_CARROS; q++) {
            atualizarCorrida(&corrida, nada, quadro);
            naPista = 0;
            for (int i = 0; i < TOTAL_CARROS; i++) {
                Carro *a = corrida.classificacao[i];
                int indice = a->numero;
                naPista += !carroNoPit(a);
                if (!carroNoPit(a)) {
                    a->velocidade = 0.0f;         /* quem saiu sai do caminho, para o teste olhar só os boxes */
                    a->posicao.segmento = buscarSegmento(pista, 1400 + indice * 40);
                    a->distanciaPercorrida = 1e7f + indice * 1e5f;
                    continue;
                }
                if (a->estadoDoPit == PIT_TROCANDO) {
                    if (lugarNaTroca[indice] == 0.0f) {
                        lugarNaTroca[indice] = a->distanciaPercorrida;
                    }
                    moveuNaTroca += fabsf(a->distanciaPercorrida - lugarNaTroca[indice]) > 0.01f;
                }
                for (int j = i + 1; j < TOTAL_CARROS; j++) {
                    Carro *b = corrida.classificacao[j];
                    float f, l;
                    if (carroNoPit(b) && penetracaoEntreCarros(&corrida, a, b, &f, &l)) {
                        maiorEntrada = fmaxf(maiorEntrada, fminf(f, l));
                        quadrosEncostados++;
                    }
                }
            }
        }
        printf("     %d carros com %.0f m entre eles: %d na pista no fim; maior entrada %.2f m; %d quadros encostados\n", TOTAL_CARROS, vao, naPista, maiorEntrada, quadrosEncostados);
        verificar(naPista == TOTAL_CARROS, "ninguem fica preso nos boxes");
        verificar(maiorEntrada < 0.30f, "nenhum carro atravessa outro no pit lane");
        verificar(moveuNaTroca == 0, "o carro parado na troca nao sai do lugar");
    }

    /* Um carro trocando pneu nao segura quem passa na faixa ao lado */
    iniciarCorrida(&corrida, pista, 0, NIVEL_MEDIO);
    corrida.contagemRegressiva = 0.0f;
    Carro *parado = &corrida.rivais[0], *passando = &corrida.rivais[12];
    for (int r = 1; r < TOTAL_RIVAIS - 1; r++) {
        corrida.rivais[r].voltaDoPit = 0;
        corrida.rivais[r].posicao.segmento = buscarSegmento(pista, 700 + r * 30);
        corrida.rivais[r].distanciaPercorrida = 1e6f + r * 1e4f;
    }
    corrida.jogador.posicao.segmento = buscarSegmento(pista, 500);
    corrida.jogador.distanciaPercorrida = 2e6f;
    float noParado = metroDaVagaDoCarro(pista, parado->numero) - 12.0f + pista->metrosDaVolta;
    parado->voltas = passando->voltas = 1;
    parado->posicao.segmento = buscarSegmento(pista, (int) (noParado * SEGMENTOS_POR_METRO));
    parado->posicao.distanciaNoSegmento = 0.0f;
    parado->distanciaPercorrida = noParado * UNIDADES_POR_METRO;
    parado->deslocamentoLateral = -1.27f;
    parado->velocidade = 1200.0f;
    parado->duracaoDaTroca = 30.0f;
    entrarNoPit(parado);
    float atras = noParado - 60.0f;
    passando->posicao.segmento = buscarSegmento(pista, (int) (atras * SEGMENTOS_POR_METRO));
    passando->posicao.distanciaNoSegmento = 0.0f;
    passando->distanciaPercorrida = atras * UNIDADES_POR_METRO;
    passando->deslocamentoLateral = -1.27f;
    passando->velocidade = LIMITE_DO_PIT;
    entrarNoPit(passando);
    float menorVelocidadePassando = 1e9f;
    int passou = 0;
    for (int q = 0; q < 60 * 12 && !passou; q++) {
        atualizarCorrida(&corrida, nada, quadro);
        if (parado->estadoDoPit == PIT_TROCANDO && passando->estadoDoPit == PIT_CHEGANDO && metrosNaVolta(passando) < metroDaVagaDoCarro(pista, passando->numero) + pista->metrosDaVolta - 15.0f) {
            menorVelocidadePassando = fminf(menorVelocidadePassando, passando->velocidade);
        }
        passou = passando->distanciaPercorrida > parado->distanciaPercorrida + 10.0f * UNIDADES_POR_METRO;
    }
    printf("     passando ao lado de quem troca: menor velocidade %d km/h\n", converterParaKmh(menorVelocidadePassando));
    verificar(passou, "quem passa ao lado de um carro trocando pneu segue em frente");
    verificar(menorVelocidadePassando > LIMITE_DO_PIT * 0.8f, "e nem precisa reduzir por causa dele");

    /* Corridas de verdade: em toda a prova, ninguem se atravessa nos boxes */
    for (int n = NIVEL_MEDIO; n <= NIVEL_DIFICIL; n++) {
        iniciarCorrida(&corrida, pista, 0, (NivelDaCorrida) n);
        corrida.contagemRegressiva = 0.0f;
        float maior = 0.0f;
        for (int q = 0; q < 60 * 60 * 15 && !corrida.terminou; q++) {
            atualizarCorrida(&corrida, atualizarIA(&corrida.jogador, corrida.pista), quadro);
            for (int i = 0; i < TOTAL_CARROS; i++) {
                for (int j = i + 1; j < TOTAL_CARROS; j++) {
                    Carro *a = corrida.classificacao[i], *b = corrida.classificacao[j];
                    float f, l;
                    if (carroNoPit(a) && carroNoPit(b) && penetracaoEntreCarros(&corrida, a, b, &f, &l)) {
                        maior = fmaxf(maior, fminf(f, l));
                    }
                }
            }
        }
        int pararam = corrida.jogador.pitFeito;
        for (int i = 0; i < TOTAL_RIVAIS; i++) {
            pararam += corrida.rivais[i].pitFeito;
        }
        printf("     corrida %s: maior entrada nos boxes %.2f m; %d carros ja pararam; chegou em %d\n", fichaDoNivel((NivelDaCorrida) n)->nome, maior, pararam, corrida.posicaoFinal);
        verificar(maior < 0.30f, "numa corrida inteira, ninguem se atravessa nos boxes");
    }
    liberarPista(pista);
}

/*
 * testarPitEmTodasAsPistas
 * Em cada pista, com a parada obrigatória: todos param, ninguém empaca nem atravessa o
 * outro, e as vagas ficam antes da linha (senão a última volta acabaria sem a parada).
 */
static void testarPitEmTodasAsPistas(void) {
    printf("\n34) O pit lane de cada pista\n");
    static Corrida corrida;
    float quadro = 1.0f / 60.0f;
    for (int id = 0; id < TOTAL_PISTAS; id++) {
        Pista *pista = criarPistaPorId((IdPista) id);
        verificar(pista->temPit && pista->metrosDaVolta > 0.0f, "a pista tem pit lane");
        int antesDaLinha = 1;
        for (int n = 0; n < LUGARES_NO_GRID; n++) {
            float vaga = metrosDesdeAEntrada(pista, metroDaVagaDoCarro(pista, n));
            antesDaLinha &= vaga < metrosDesdeAEntrada(pista, 0.0f);
            antesDaLinha &= vaga < metrosDesdeAEntrada(pista, pista->pit.fim);
        }
        verificar(antesDaLinha, "todas as vagas ficam antes da linha e do fim do pit lane");

        iniciarCorrida(&corrida, pista, 0, NIVEL_DIFICIL);
        float maiorEntrada = 0.0f, maiorParado = 0.0f, parado[TOTAL_RIVAIS] = { 0 };
        while (!todosChegaram(&corrida) && corrida.tempoDeCorrida < 1500.0f) {
            atualizarCorrida(&corrida, atualizarIA(&corrida.jogador, corrida.pista), quadro);
            for (int r = 0; r < TOTAL_RIVAIS; r++) {
                Carro *rival = &corrida.rivais[r];
                parado[r] = (corrida.contagemRegressiva <= 0.0f && rival->velocidade < 100.0f && rival->tempoDeChegada <= 0.0f) ? parado[r] + quadro : 0.0f;
                maiorParado = fmaxf(maiorParado, parado[r]);
                float f = 0.0f, l = 0.0f;
                if (carroNoPit(rival) && carroNoPit(&corrida.jogador)
                    && penetracaoEntreCarros(&corrida, rival, &corrida.jogador, &f, &l)) {
                    maiorEntrada = fmaxf(maiorEntrada, fminf(f, l));
                }
            }
        }
        int pararam = corrida.jogador.pitFeito;
        for (int r = 0; r < TOTAL_RIVAIS; r++) {
            pararam &= corrida.rivais[r].pitFeito;
        }
        printf("     %s: fim em %.0f s; maior tempo parado %.1f s; maior entrada %.2f m\n", pista->nome, corrida.tempoDeCorrida, maiorParado, maiorEntrada);
        verificar(todosChegaram(&corrida), "todos terminam a corrida");
        verificar(pararam, "todos fizeram a parada");
        verificar(maiorParado < 25.0f, "ninguem fica empacado");
        liberarPista(pista);
    }
}

/*
 * testarCenarioDosMapas
 * Nenhuma árvore ou estrutura pode cair em cima do caminho dos carros, nem no pit lane.
 */
static void testarCenarioDosMapas(void) {
    printf("\n35) O cenario dos mapas\n");
    struct { IdPista id; const CoisaNoMapa *coisas; int total; } mapas[2] = {
        { PISTA_OVAL, CENARIO_OVAL, TOTAL_CENARIO_OVAL }, { PISTA_SPLAT, CENARIO_SPLAT, TOTAL_CENARIO_SPLAT }
    };
    for (int k = 0; k < 2; k++) {
        Pista *pista = criarPistaPorId(mapas[k].id);
        int naPista = 0, noPit = 0, arvores = 0, estruturas = 0, pneus = 0;
        float maisPerto = 1e9f;
        for (int i = 0; i < mapas[k].total; i++) {
            const CoisaNoMapa *coisa = &mapas[k].coisas[i];
            (coisa->tipo <= COISA_COPA_3) ? arvores++ : estruturas++;
            float menor = 1e9f, largura = 0.0f;
            Segmento *atual = pista->largada;
            do {
                float d = hypotf(coisa->x - atual->xMundo, coisa->z - atual->zMundo);
                if (d < menor) {
                    menor = d;
                    largura = fmaxf(atual->larguraEsquerda, atual->larguraDireita);
                }
                atual = atual->proximo;
            } while (atual != pista->largada);
            if (coisa->tipo == COISA_PNEUS_VERMELHO || coisa->tipo == COISA_PNEUS_BRANCO) {
                pneus++;     /* a fileira fica justo no limite da pista, de propósito */
                naPista += menor < 7.0f;
            } else {
                naPista += menor < largura;
            }
            maisPerto = fminf(maisPerto, menor);
            /* O pit lane: da faixa rápida até a vaga, em cada ponto entre as vagas */
            for (int n = 0; n < LUGARES_NO_GRID; n++) {
                PosicaoNaPista lugar = { pista->largada, 0.0f };
                moverNaPista(&lugar, metroDaVagaDoCarro(pista, n) * UNIDADES_POR_METRO);
                float l = pista->pit.lado * pista->pit.vaga
                          / ((pista->pit.lado < 0) ? lugar.segmento->larguraEsquerda : lugar.segmento->larguraDireita);
                PontoNoMundo vaga = pontoNoMundo(lugar, l);
                noPit += hypotf(coisa->x - vaga.x, coisa->z - vaga.z) < 5.0f;
            }
        }
        /* O caminho do pit lane não pode passar pelos pneus */
        static Corrida corrida;
        iniciarCorrida(&corrida, pista, 0, NIVEL_MEDIO);
        corrida.contagemRegressiva = 0.0f;
        Carro *jogador = &corrida.jogador;
        float antes = metrosDesdeAEntrada(pista, 0.0f) * 0.0f + pista->pit.entrada - 30.0f;
        jogador->posicao.segmento = buscarSegmento(pista, (int) (antes * SEGMENTOS_POR_METRO));
        jogador->posicao.distanciaNoSegmento = 0.0f;
        jogador->distanciaPercorrida = antes * UNIDADES_POR_METRO;
        jogador->voltas = 0;
        jogador->voltaDoPit = 1;
        jogador->pitFeito = 0;
        for (int r = 0; r < TOTAL_RIVAIS; r++) {
            corrida.rivais[r].voltaDoPit = 0;
            corrida.rivais[r].pitFeito = 1;
        }
        int bateuNosPneus = 0, entrou = 0;
        for (int q = 0; q < 60 * 60 && !(entrou && !carroNoPit(jogador)); q++) {
            atualizarCorrida(&corrida, atualizarIA(jogador, corrida.pista), 1.0f / 60.0f);
            entrou |= carroNoPit(jogador);
            PontoNoMundo aqui = pontoNoMundo(jogador->posicao, jogador->deslocamentoLateral);
            for (int i = 0; carroNoPit(jogador) && i < mapas[k].total; i++) {
                const CoisaNoMapa *coisa = &mapas[k].coisas[i];
                if ((coisa->tipo == COISA_PNEUS_VERMELHO || coisa->tipo == COISA_PNEUS_BRANCO)
                    && hypotf(coisa->x - aqui.x, coisa->z - aqui.z) < 1.8f) {
                    bateuNosPneus++;
                }
            }
        }
        printf("     %s: %d arvores, %d estruturas, %d pneus; o mais perto da linha da pista, a %.0f m\n", pista->nome, arvores, estruturas - pneus, pneus, maisPerto);
        verificar(entrou && !carroNoPit(jogador), "o carro faz a parada e sai");
        verificar(bateuNosPneus == 0, "o caminho do pit lane nao passa pelos pneus");
        verificar(arvores > 100 && estruturas > 20, "o mapa tem arvores e estruturas");
        verificar(naPista == 0, "nada em cima da pista");
        verificar(noPit == 0, "nada em cima das vagas do pit");
        liberarPista(pista);
    }
}

/*
 * andarSemVolante
 * Recebe: a pista, onde começa e a velocidade em km/h.
 * Devolve: quantos metros o carro anda sem tocar no volante, até a borda ou o muro.
 */
static float andarSemVolante(Pista *pista, int segmento, int kmh, int livre) {
    Carro carro = criarCarro("Teste", 0, buscarSegmento(pista, segmento), 0.0f, 12000.0f, 2400.0f, 0.5f);
    carro.direcaoLivre = livre;
    float velocidade = kmh / 3.6f * UNIDADES_POR_METRO, andou = 0.0f, quadro = 1.0f / 60.0f;
    Comandos nada = { 0, 0, 0, 0, 0.0f, 0, 0 };
    for (int i = 0; i < 60 * 30 && fabsf(carro.deslocamentoLateral) < 1.0f && !carro.raspandoNoMuro; i++) {
        carro.velocidade = velocidade;
        atualizarCarro(&carro, nada, quadro);
        andou += velocidade * quadro / UNIDADES_POR_METRO;
    }
    return andou;
}

/*
 * saidaDaReta
 * Sem volante, o caminho do carro no mundo tem que ser uma reta, sem o carro saber da curva.
 * Recebe: a pista, onde ele começa e a velocidade em km/h.
 * Devolve: o maior afastamento (em metros) do caminho em relação à reta entre o começo e o fim.
 */
static float saidaDaReta(Pista *pista, int segmento, int kmh, float *comprimento) {
    Carro carro = criarCarro("Teste", 0, buscarSegmento(pista, segmento), 0.0f, 12000.0f, 2400.0f, 0.5f);
    carro.direcaoLivre = 1;
    float velocidade = kmh / 3.6f * UNIDADES_POR_METRO, quadro = 1.0f / 60.0f;
    Comandos nada = { 0, 0, 0, 0, 0.0f, 0, 0 };
    static float xs[60 * 30], zs[60 * 30];
    int total = 0;
    for (int i = 0; i < 60 * 30 && !carro.raspandoNoMuro && fabsf(carro.deslocamentoLateral) < 2.4f; i++) {
        carro.velocidade = velocidade;
        atualizarCarro(&carro, nada, quadro);
        PontoNoMundo ponto = pontoNoMundo(carro.posicao, carro.deslocamentoLateral);
        xs[total] = ponto.x;
        zs[total] = ponto.z;
        total++;
    }
    float dx = xs[total - 1] - xs[0], dz = zs[total - 1] - zs[0], tamanho = hypotf(dx, dz), maior = 0.0f;
    for (int i = 0; i < total && tamanho > 0.0f; i++) {
        maior = fmaxf(maior, fabsf((xs[i] - xs[0]) * dz - (zs[i] - zs[0]) * dx) / tamanho);
    }
    *comprimento = tamanho;
    return maior;
}

#define MARGEM_DO_PILOTO 0.9f     /* o piloto de teste usa isto do que o pneu segura */
#define FREADA_DO_PILOTO 30.0f    /* m/s² */

/*
 * voltaComVolante
 * Sem a direção guiada, um piloto que olha adiante e corrige (como quem joga) precisa dar a volta.
 * Recebe: a pista, o carro, se usa só as setas (ligado ou desligado) e onde guardar os quadros raspando ou na grama.
 * Devolve: o tempo da volta em segundos, ou 0 se não completou.
 */
static float voltaComVolante(Pista *pista, int numero, int soSetas, int *raspando) {
    const FichaTecnica *ficha = fichaDoCarro(numero);
    Carro carro = criarCarro(ficha->nome, numero, pista->largada, 0.0f, ficha->velocidadeMaxima, ficha->aceleracao, ficha->aderencia);
    carro.direcaoLivre = 1;
    float tempo = 0.0f, inicio = 0.0f, quadro = 1.0f / 60.0f;
    int voltas = 0;
    *raspando = 0;
    while (voltas < 2 && tempo < 400.0f) {
        float rumoAntes = carro.rumoNoMundo, desvio = carro.desvioDoRumo;
        int tinhaRumo = carro.temRumoNoMundo;
        (void) rumoAntes;
        (void) desvio;
        Comandos comandos = { 0, 0, 0, 0, 0.0f, 0, 0 };
        /* Os pedais: freia a tempo da velocidade que a curva adiante aceita */
        float agora = carro.velocidade / UNIDADES_POR_METRO, segura = aceleracaoDeCurva(&carro) * MARGEM_DO_PILOTO;
        int frear = 0;
        Segmento *olhando = carro.posicao.segmento;
        for (int d = 0; d < (int) (200.0f * SEGMENTOS_POR_METRO); d += 4) {
            float soma = 0.0f;
            Segmento *medindo = olhando;
            for (int k = 0; k < 12; k++) {
                soma += medindo->curvatura;
                medindo = medindo->proximo;
            }
            float porMetro = fabsf(soma) / 12.0f * 0.0174533f * SEGMENTOS_POR_METRO;
            float distancia = d / SEGMENTOS_POR_METRO;
            if (porMetro > 0.0005f && agora * agora > segura / porMetro + 2.0f * FREADA_DO_PILOTO * distancia) {
                frear = 1;
            }
            for (int k = 0; k < 4; k++) {
                olhando = olhando->proximo;
            }
        }
        comandos.frear = frear;
        comandos.acelerar = !frear;
        /* Olha um ponto adiante, no meio da pista, e aponta para ele */
        float metrosAdiante = fmaxf(carro.velocidade / UNIDADES_POR_METRO * 0.55f, 9.0f);
        PosicaoNaPista alvo = carro.posicao;
        moverNaPista(&alvo, metrosAdiante * UNIDADES_POR_METRO);
        PontoNoMundo daqui = pontoNoMundo(carro.posicao, carro.deslocamentoLateral), dali = pontoNoMundo(alvo, 0.0f);
        float rumoQuerido = atan2f(dali.x - daqui.x, -(dali.z - daqui.z)) * 57.2958f;
        float erro = rumoQuerido - (tinhaRumo ? carro.rumoNoMundo : daqui.rumo);
        while (erro > 180.0f) {
            erro -= 360.0f;
        }
        while (erro < -180.0f) {
            erro += 360.0f;
        }
        float volante = erro * 0.09f;
        comandos.esquerda = comandos.direita = 0;
        comandos.direcao = fminf(fmaxf(volante, -1.0f), 1.0f);
        if (soSetas) {
            comandos.direcao = 0.0f;
            comandos.direita = volante > 0.15f;
            comandos.esquerda = volante < -0.15f;
        }
        tempo += quadro;
        *raspando += carro.raspandoNoMuro || estaForaDaPista(&carro);
        if (atualizarCarro(&carro, comandos, quadro) > 0) {
            voltas++;
            inicio = (voltas == 1) ? tempo : inicio;
        }
    }
    return (voltas == 2) ? tempo - inicio : 0.0f;
}

/*
 * testarDirecaoGuiada
 * Ligada, nada muda; desligada, sem volante o carro segue reto e sai como manda a geometria,
 * em qualquer velocidade, e quem usa o volante continua dando a volta.
 */
static void testarDirecaoGuiada(void) {
    printf("\n36) A direcao guiada\n");
    Configuracoes guardada = *configuracoesAtuais();
    verificar(guardada.direcaoGuiada == 1, "ela vem ligada");
    Configuracoes saida, entrada;
    configuracoesPadrao(&saida);
    configuracoesPadrao(&entrada);
    saida.direcaoGuiada = 0;
    verificar(salvarConfiguracoes(&saida, "build/config_teste.txt") && carregarConfiguracoes(&entrada, "build/config_teste.txt")
              && entrada.direcaoGuiada == 0, "a escolha fica salva");
    remove("build/config_teste.txt");

    /* Só o jogador: os rivais continuam iguais, e quando a IA assume o carro dele, depois da chegada, ela usa a guiada */
    static Corrida corrida;
    Pista *umaPista = criarPistaPorId(PISTA_OVAL);
    configuracoesAtuais()->direcaoGuiada = 0;
    iniciarCorrida(&corrida, umaPista, 0, NIVEL_FACIL);
    int soOJogador = corrida.jogador.direcaoLivre == 1;
    for (int r = 0; r < TOTAL_RIVAIS; r++) {
        soOJogador &= corrida.rivais[r].direcaoLivre == 0;
    }
    verificar(soOJogador, "desligada, vale so para o jogador");
    corrida.terminou = 1;
    corrida.contagemRegressiva = 0.0f;
    Comandos nadaAqui = { 0, 0, 0, 0, 0.0f, 0, 0 };
    atualizarCorrida(&corrida, nadaAqui, 1.0f / 60.0f);
    verificar(corrida.jogador.direcaoLivre == 0, "depois da chegada a IA dirige normalmente");
    configuracoesAtuais()->direcaoGuiada = 1;
    iniciarCorrida(&corrida, umaPista, 0, NIVEL_FACIL);
    verificar(corrida.jogador.direcaoLivre == 0, "ligada, tudo como antes");
    liberarPista(umaPista);

    /* No Splat ela não existe: nem com a opção ligada, e os rivais continuam com a deles */
    Pista *splat = criarPistaPorId(PISTA_SPLAT);
    iniciarCorrida(&corrida, splat, 0, NIVEL_FACIL);
    soOJogador = configuracoesAtuais()->direcaoGuiada == 1 && corrida.jogador.direcaoLivre == 1;
    for (int r = 0; r < TOTAL_RIVAIS; r++) {
        soOJogador &= corrida.rivais[r].direcaoLivre == 0;
    }
    verificar(soOJogador, "no Splat o jogador sempre dirige sem ela, mesmo com a opcao ligada");
    corrida.terminou = 1;
    corrida.contagemRegressiva = 0.0f;
    atualizarCorrida(&corrida, atualizarIA(&corrida.jogador, corrida.pista), 1.0f / 60.0f);
    verificar(corrida.jogador.direcaoLivre == 0, "e la tambem a IA leva o carro depois da chegada");
    /* Só acelerando, o carro não faz a curva: segue reto, sai do asfalto e para no fim da área de escape */
    iniciarCorrida(&corrida, splat, 0, NIVEL_FACIL);
    corrida.contagemRegressiva = 0.0f;
    Comandos soAcelerar = { 0, 0, 0, 0, 0.0f, 0, 0 };
    soAcelerar.acelerar = 1;
    int saiuDoAsfalto = 0;
    for (int q = 0; q < 60 * 30; q++) {
        atualizarCorrida(&corrida, soAcelerar, 1.0f / 60.0f);
        saiuDoAsfalto |= estaForaDaPista(&corrida.jogador);
    }
    /* Sem zigue-zague depois da largada: a linha da pista vira sempre para o mesmo lado */
    int semZigueZague = 1;
    for (int m = 0; m < 220; m += 20) {
        float agora = buscarSegmento(splat, (int) (m * SEGMENTOS_POR_METRO))->rumo;
        float depois = buscarSegmento(splat, (int) ((m + 20) * SEGMENTOS_POR_METRO))->rumo;
        semZigueZague &= depois - agora < 3.0f;
    }
    verificar(semZigueZague, "a linha do Splat nao faz zigue-zague depois da largada");

    /* Sozinho, largando do grid e só acelerando: o rumo que a tela usa não acompanha a curva da pista */
    Carro sozinho = criarCarro("Teste", 0, buscarSegmento(splat, contarSegmentos(splat) - 30), 0.0f, 12000.0f, 2400.0f, 0.5f);
    sozinho.direcaoLivre = 1;
    float rumoNaLargada = sozinho.posicao.segmento->rumo, rumoAoSair = rumoNaLargada, pistaAoSair = rumoNaLargada;
    sozinho.rumoNoMundo = rumoNaLargada;
    sozinho.temRumoNoMundo = 1;
    for (int q = 0; q < 60 * 12 && !sozinho.raspandoNoMuro; q++) {
        atualizarCarro(&sozinho, soAcelerar, 1.0f / 60.0f);
        rumoAoSair = sozinho.rumoNoMundo;
        pistaAoSair = sozinho.posicao.segmento->rumo;
    }
    printf("     Splat, so acelerando: a pista virou de %.0f para %.0f graus; o carro ficou em %.0f\n", rumoNaLargada, pistaAoSair, rumoAoSair);
    verificar(pistaAoSair < rumoNaLargada - 5.0f && rumoAoSair > pistaAoSair + 5.0f, "a pista faz a curva e o carro nao");

    float andouSoAcelerando = corrida.jogador.distanciaPercorrida / UNIDADES_POR_METRO;
    printf("     Splat, 30 s so acelerando: andou %.0f m e parou; saiu do asfalto: %d\n", andouSoAcelerando, saiuDoAsfalto);
    verificar(saiuDoAsfalto, "so acelerando, ele sai do asfalto na primeira curva");
    verificar(andouSoAcelerando < 200.0f, "e nao da a volta sozinho");
    Comandos sair = soAcelerar;
    sair.direcao = (corrida.jogador.deslocamentoLateral > 0.0f) ? -1.0f : 1.0f;
    float lateralParado = corrida.jogador.deslocamentoLateral * larguraDoLado(corrida.jogador.posicao.segmento, corrida.jogador.deslocamentoLateral);
    float segundosParaSair = -1.0f;
    for (int q = 0; q < 60 * 4 && segundosParaSair < 0.0f; q++) {
        atualizarCorrida(&corrida, sair, 1.0f / 60.0f);
        Carro *j = &corrida.jogador;
        float agora = j->deslocamentoLateral * larguraDoLado(j->posicao.segmento, j->deslocamentoLateral);
        if (j->velocidade > 1000.0f && fabsf(agora) < fabsf(lateralParado) - 3.0f) {
            segundosParaSair = q / 60.0f;
        }
    }
    printf("     saindo do limite com o volante: %.1f s\n", segundosParaSair);
    verificar(segundosParaSair >= 0.0f, "e sai de la com o volante");
    /* O limite nunca fica dentro de um carro parado: nem o que mediu um muro aperta demais */
    int limitesBons = 1;
    Segmento *atualDoSplat = splat->largada;
    do {
        limitesBons &= atualDoSplat->limiteDireita >= 9.0f && atualDoSplat->limiteEsquerda >= 9.0f
                       && atualDoSplat->limiteDireita <= 22.5f && atualDoSplat->limiteEsquerda <= 22.5f && !atualDoSplat->temMuro;
        atualDoSplat = atualDoSplat->proximo;
    } while (atualDoSplat != splat->largada);
    verificar(limitesBons, "os limites do Splat vao do muro real ate a area de escape");
    liberarPista(splat);
    for (int id = 0; id < TOTAL_PISTAS; id++) {
        Pista *outra = criarPistaPorId((IdPista) id);
        verificar(outra->semDirecaoGuiada == (id == PISTA_SPLAT), "so o Splat fica sem ela");
        liberarPista(outra);
    }

    for (int id = 0; id < TOTAL_PISTAS; id++) {
        Pista *pista = criarPistaPorId((IdPista) id);
        int total = contarSegmentos(pista), lugar = 0;
        float maior = 0.0f;
        for (int i = 0; i < total; i++) {
            Segmento *s = buscarSegmento(pista, i);
            float soma = 0.0f;
            for (int k = 0; k < 60; k++) {
                soma += s->curvatura;
                s = s->proximo;
            }
            if (fabsf(soma) > maior) {
                maior = fabsf(soma);
                lugar = i;
            }
        }
        float raio = 100.0f / (maior * 0.0174533f);
        float geometria = sqrtf(2.0f * raio * fmaxf(buscarSegmento(pista, lugar)->larguraDireita, buscarSegmento(pista, lugar)->larguraEsquerda));
        float guiada = andarSemVolante(pista, lugar, 60, 0), livre60 = andarSemVolante(pista, lugar, 60, 1), livre240 = andarSemVolante(pista, lugar, 240, 1);
        int raspando[2] = { 0, 0 };
        float analogico = voltaComVolante(pista, 0, 0, &raspando[0]), setas = voltaComVolante(pista, 0, 1, &raspando[1]);
        printf("     %s: sem volante, a 60 km/h: guiada %.0f m, desligada %.0f m (a 240: %.0f; reto daria %.0f); volta no analogico %.1f s, nas setas %.1f s; quadros fora da linha %d e %d\n",
               pista->nome, guiada, livre60, livre240, geometria, analogico, setas, raspando[0], raspando[1]);
        float tamanhoDaReta = 0.0f;
        float afastamento = saidaDaReta(pista, lugar, 120, &tamanhoDaReta);
        printf("     %s, sem volante a 120 km/h na curva mais fechada: anda %.0f m e se afasta %.2f m da linha reta\n", pista->nome, tamanhoDaReta, afastamento);
        verificar(afastamento < 0.2f, "sem volante, o caminho no mundo e uma reta");
        verificar(fabsf(livre240 - livre60) < 3.0f, "o lugar onde ele sai nao depende da velocidade, como numa reta");
        (void) geometria;
        verificar(analogico > 0.0f && setas > 0.0f, "com o volante da para dar a volta, no analogico e nas setas");
        verificar(raspando[0] < analogico * 60.0f * 0.05f, "e quem guia bem quase nao encosta no muro nem pisa na grama");
        liberarPista(pista);
    }
}

/*
 * testarSairDaVagaComCarroNaFrente
 * Em todas as pistas: quem acaba a troca antes do carro da vaga da frente sai sem esperar por ele.
 */
static void testarSairDaVagaComCarroNaFrente(void) {
    printf("\n37) Saindo da vaga com um carro parado na frente\n");
    static Corrida corrida;
    float quadro = 1.0f / 60.0f;
    Comandos nada = { 0, 0, 0, 0, 0.0f, 0, 0 };
    for (int id = 0; id < TOTAL_PISTAS; id++) {
        Pista *pista = criarPistaPorId((IdPista) id);
        float pior = 0.0f, maiorEntrada = 0.0f;
        int todos = 1;
        for (int numero = 0; numero + 1 < TOTAL_CARROS; numero += 3) {
            iniciarCorrida(&corrida, pista, numero, NIVEL_MEDIO);
            corrida.jogador.controladoPeloJogador = 0;   /* aqui é o piloto automático do pit que está em teste */
            corrida.contagemRegressiva = 0.0f;
            corrida.tempoDeCorrida = 30.0f;
            Carro *jogador = &corrida.jogador, *frente = NULL;
            for (int r = 0; r < TOTAL_RIVAIS; r++) {
                Carro *rival = &corrida.rivais[r];
                rival->voltaDoPit = 0;
                rival->pitFeito = 1;
                rival->posicao.segmento = buscarSegmento(pista, (int) (fmodf(pista->pit.fim + 300.0f + r * 60.0f, pista->metrosDaVolta) * SEGMENTOS_POR_METRO));
                rival->posicao.distanciaNoSegmento = 0.0f;
                rival->deslocamentoLateral = 0.0f;
                rival->distanciaPercorrida = 1e7f;
                if (rival->numero == numero + 1) {
                    frente = rival;
                }
            }
            Carro *parados[2] = { frente, jogador };
            float duracoes[2] = { 120.0f, 0.3f };
            for (int i = 0; i < 2; i++) {
                lugarDaVaga(pista, parados[i], &parados[i]->posicao, &parados[i]->deslocamentoLateral);
                parados[i]->estadoDoPit = PIT_TROCANDO;
                parados[i]->tempoDaTroca = 0.0f;
                parados[i]->duracaoDaTroca = duracoes[i];
                parados[i]->velocidade = 0.0f;
                parados[i]->distanciaPercorrida = metrosNaVolta(parados[i]) * UNIDADES_POR_METRO + 5e6f;
            }
            jogador->voltaDoPit = 99;
            float tempo = 0.0f;
            int passou = 0;
            float depoisDele = metrosDesdeAEntrada(pista, metroDaVagaDoCarro(pista, numero + 1)) + 8.0f;
            for (int q = 0; q < 60 * 30 && !passou; q++) {
                atualizarCorrida(&corrida, nada, quadro);
                tempo += quadro;
                float f = 0.0f, l = 0.0f;
                if (penetracaoEntreCarros(&corrida, jogador, frente, &f, &l)) {
                    maiorEntrada = fmaxf(maiorEntrada, fminf(f, l));
                }
                passou = jogador->estadoDoPit == PIT_SAINDO && metrosDesdeAEntrada(pista, metrosNaVolta(jogador)) > depoisDele;
            }
            todos &= passou;
            pior = fmaxf(pior, tempo);
        }
        printf("     %s: passa pelo carro parado em ate %.1f s; maior entrada %.2f m\n", pista->nome, pior, maiorEntrada);
        verificar(todos && pior < 5.0f, "ele sai sem esperar o carro da frente");
        verificar(maiorEntrada < 0.05f, "e sem encostar nele");
        liberarPista(pista);
    }
}

/*
 * testarGuidoForaDoCarro
 * Com qualquer carro e em todas as pistas, ao longo da troca inteira, o Guido não entra no
 * carro que ele está trocando: nem andando de canto a canto, nem virando com o pneu.
 */
static void testarGuidoForaDoCarro(void) {
    printf("\n38) O Guido nao entra no carro\n");
    static Corrida corrida;
    float pior = 0.0f, piorSemGarantia = 0.0f;
    for (int id = 0; id < TOTAL_PISTAS; id++) {
        Pista *pista = criarPistaPorId((IdPista) id);
        for (int numero = 0; numero < TOTAL_CARROS; numero++) {
            iniciarCorrida(&corrida, pista, numero, NIVEL_MEDIO);
            Carro *carro = &corrida.jogador;
            const FichaTecnica *ficha = fichaDoCarro(numero);
            float meiaLargura = ficha->largura / 2.0f, meioComprimento = ficha->comprimento / 2.0f;
            float zFrente = meioComprimento * 0.62f, zTras = -meioComprimento * 0.62f;
            lugarDaVaga(pista, carro, &carro->posicao, &carro->deslocamentoLateral);
            carro->estadoDoPit = PIT_TROCANDO;
            for (float t = 0.0f; t < DURACAO_DA_TROCA; t += 0.01f) {
                carro->tempoDaTroca = t;
                LugarDoGuido g = poseDoGuido(carro, meiaLargura, meioComprimento, zFrente, zTras);
                pior = fmaxf(pior, entradaDoGuidoNoCarro(g, meiaLargura, meioComprimento));
                LugarDoGuido semGarantia = lugarDoGuido(carro, meiaLargura, meioComprimento, zFrente, zTras);
                piorSemGarantia = fmaxf(piorSemGarantia, entradaDoGuidoNoCarro(semGarantia, meiaLargura, meioComprimento));
            }
        }
        liberarPista(pista);
    }
    printf("     maior entrada do Guido no carro: %.3f m (o caminho antigo, sem a garantia: %.3f m)\n", pior, piorSemGarantia);
    verificar(pior == 0.0f, "em nenhum carro, nenhuma pista e nenhum momento o Guido entra no carro");
}

/*
 * testarSuspensao
 * Numa volta da IA, o corpo inclina para fora nas curvas e mergulha nas freadas, sutil: poucos graus.
 */
static void testarSuspensao(void) {
    printf("\n39) A suspensao\n");
    static Corrida corrida;
    for (int id = 0; id < TOTAL_PISTAS; id++) {
        Pista *pista = criarPistaPorId((IdPista) id);
        iniciarCorrida(&corrida, pista, 0, NIVEL_MEDIO);
        float maiorRolagem = 0.0f, maiorArfagem = 0.0f, menorArfagem = 0.0f;
        int paraFora = 0, curvas = 0;
        float somaLeve = 0.0f;
        int leves = 0;
        for (int q = 0; q < 60 * 70; q++) {
            atualizarCorrida(&corrida, atualizarIA(&corrida.jogador, corrida.pista), 1.0f / 60.0f);
            Carro *j = &corrida.jogador;
            maiorRolagem = fmaxf(maiorRolagem, fabsf(j->rolagem));
            maiorArfagem = fmaxf(maiorArfagem, j->arfagem);
            menorArfagem = fminf(menorArfagem, j->arfagem);
            if (fabsf(j->posicao.segmento->curvatura) > 0.3f && j->velocidade > 3000.0f && !carroNoPit(j)) {
                curvas++;
                paraFora += (j->rolagem * j->posicao.segmento->curvatura > 0.0f);
            }
            if (fabsf(j->posicao.segmento->curvatura) > 0.05f && fabsf(j->posicao.segmento->curvatura) < 0.3f && j->velocidade > 3000.0f) {
                somaLeve += fabsf(j->rolagem);
                leves++;   /* curva à direita: inclina para a esquerda */
            }
        }
        printf("     %s: curva leve inclina em media %.1f graus\n", pista->nome, leves ? somaLeve / leves : 0.0f);
        printf("     %s: rolagem ate %.1f graus, arfagem de %.1f a %.1f; inclinou para fora em %d de %d quadros de curva\n",
               pista->nome, maiorRolagem, menorArfagem, maiorArfagem, paraFora, curvas);
        verificar(maiorRolagem > 0.5f && maiorRolagem <= 6.05f && maiorArfagem <= 3.55f && menorArfagem >= -3.55f, "a suspensao mexe, sem exagero");
        verificar(curvas == 0 || paraFora > curvas * 0.8f, "nas curvas o corpo inclina para fora");
        liberarPista(pista);
    }
}

/*
 * testarRe
 * Parado, segurando o freio, o jogador engata a ré: anda para trás até 25 km/h, vira de lado com
 * o volante, não atravessa a linha de chegada de ré e, acelerando, volta a andar para a frente.
 * A IA nunca dá ré.
 */
static void testarRe(void) {
    printf("\n40) A re\n");
    static Corrida corrida;
    Pista *pista = criarPistaPorId(PISTA_INTERLAGOS);
    iniciarCorrida(&corrida, pista, 0, NIVEL_MEDIO);
    corrida.contagemRegressiva = 0.0f;
    Carro *j = &corrida.jogador;
    j->posicao.segmento = buscarSegmento(pista, (int) (600 * SEGMENTOS_POR_METRO));
    j->velocidade = 0.0f;
    for (int r = 0; r < TOTAL_RIVAIS; r++) {
        corrida.rivais[r].posicao.segmento = buscarSegmento(pista, (int) (2000 * SEGMENTOS_POR_METRO));
    }
    float antes = metrosNaVolta(j), lateralAntes = j->deslocamentoLateral;
    Comandos c = { 0 };
    c.frear = 1;
    float maior = 0.0f;
    for (int q = 0; q < 60 * 4; q++) {
        c.direcao = (q > 120) ? -1.0f : 0.0f;
        atualizarCorrida(&corrida, c, 1.0f / 60.0f);
        maior = fmaxf(maior, j->re);
    }
    float voltou = antes - metrosNaVolta(j);
    printf("     de re por 4 s: %.1f m para tras, ate %d km/h, de lado %.2f\n", voltou, (int) (maior / 40.0f), j->deslocamentoLateral - lateralAntes);
    verificar(voltou > 5.0f && maior <= 1800.0f && maior > 1600.0f, "segurando o freio parado, ele anda de re ate 45 km/h");
    verificar(j->deslocamentoLateral < lateralAntes, "de re, o volante para a esquerda leva o carro para a esquerda");
    Comandos acelera = { 0 };
    acelera.acelerar = 1;
    for (int q = 0; q < 60 * 2; q++) {
        atualizarCorrida(&corrida, acelera, 1.0f / 60.0f);
    }
    verificar(j->re == 0.0f && j->velocidade > 1000.0f, "acelerando, a re para e ele volta a andar para a frente");

    /* Na linha de chegada, de re, ele para nela */
    j->posicao.segmento = buscarSegmento(pista, 2);
    j->posicao.distanciaNoSegmento = 0.0f;
    j->velocidade = 0.0f;
    int voltas = j->voltas;
    for (int q = 0; q < 60 * 4; q++) {
        atualizarCorrida(&corrida, c, 1.0f / 60.0f);
    }
    verificar(j->voltas == voltas, "de re, ele nao atravessa a linha de chegada");
    verificar(!corrida.rivais[0].controladoPeloJogador, "a IA nao da re");
    liberarPista(pista);
}

/*
 * testarSaidaDoPitNaMaoDoJogador
 * Em todas as pistas: o piloto automático leva o carro da vaga até a pista e devolve o volante
 * assim que ele está de volta ao asfalto da pista, sem limite de velocidade dali em diante.
 */
static void testarSaidaDoPitNaMaoDoJogador(void) {
    printf("\n41) O box devolve o carro assim que ele volta a pista\n");
    static Corrida k;
    for (int id = 0; id < TOTAL_PISTAS; id++) {
        Pista *p = criarPistaPorId((IdPista) id);
        iniciarCorrida(&k, p, 0, NIVEL_MEDIO);
        k.contagemRegressiva = 0.0f;
        Carro *j = &k.jogador;
        float antes = fmodf(p->pit.entrada - 250.0f + p->metrosDaVolta, p->metrosDaVolta);
        j->posicao.segmento = buscarSegmento(p, (int) (antes * SEGMENTOS_POR_METRO));
        j->distanciaPercorrida = antes * UNIDADES_POR_METRO;
        j->voltaDoPit = 1;
        for (int r = 0; r < TOTAL_RIVAIS; r++) {
            k.rivais[r].pitFeito = 1;
            k.rivais[r].posicao.segmento = buscarSegmento(p, (int) (fmodf(p->pit.fim + 400.0f + r * 80.0f, p->metrosDaVolta) * SEGMENTOS_POR_METRO));
        }
        float t = 0.0f, saiu = -1.0f, devolveu = -1.0f, voltou = -1.0f;
        int acimaDoLimite = 0;
        for (int q = 0; q < 60 * 120 && voltou < 0.0f; q++) {
            t += 1.0f / 60.0f;
            Comandos c = atualizarIA(j, k.pista);
            if (devolveu >= 0.0f) {
                c.acelerar = 1;
                c.frear = 0;
            }
            atualizarCorrida(&k, c, 1.0f / 60.0f);
            if (j->estadoDoPit == PIT_SAINDO && saiu < 0.0f) saiu = t;
            if (saiu >= 0.0f && !carroNoPit(j) && devolveu < 0.0f) {
                devolveu = t;
                acimaDoLimite = fabsf(j->deslocamentoLateral) > 1.0f;   /* devolvido fora da pista? */
            }
            if (devolveu >= 0.0f && voltou < 0.0f && t > devolveu + 3.0f) voltou = j->velocidade;
        }
        printf("     %s: o piloto assume %.1f s depois de sair da vaga; 3 s depois esta a %d km/h\n", p->nome, devolveu - saiu, (int) (voltou / 40.0f));
        verificar(devolveu >= 0.0f && acimaDoLimite == 0, "o volante volta com o carro dentro da pista");
        verificar(voltou > LIMITE_DO_PIT, "e dali em diante nao ha limite de velocidade");
        liberarPista(p);
    }
}

/*
 * testarPelotao
 * O rubber banding: com o jogador parado na largada, os rivais que abrem muito perdem um pouco de
 * motor (mais no fácil, menos no difícil); um rival muito atrás ganha um pouco; perto do jogador,
 * nada muda. E é pouco: nunca mais que 5%.
 */
static void testarPelotao(void) {
    printf("\n42) O pelotao espera e corre atras\n");
    static Corrida corrida;
    Pista *pista = criarPistaPorId(PISTA_INTERLAGOS);
    float noFacil = 0.0f, noDificil = 0.0f;
    for (int nivel = NIVEL_FACIL; nivel <= NIVEL_DIFICIL; nivel += 2) {
        iniciarCorrida(&corrida, pista, 0, (NivelDaCorrida) nivel);
        corrida.contagemRegressiva = 0.0f;
        Comandos parado = { 0 };
        for (int q = 0; q < 60 * 60; q++) {
            atualizarCorrida(&corrida, parado, 1.0f / 60.0f);
        }
        float menor = 1.0f;
        for (int r = 0; r < TOTAL_RIVAIS; r++) {
            menor = fminf(menor, corrida.rivais[r].ritmoDoPelotao);
        }
        printf("     %s: com o jogador parado, o ritmo dos rivais desce ate %.3f\n", fichaDoNivel((NivelDaCorrida) nivel)->nome, menor);
        if (nivel == NIVEL_FACIL) noFacil = menor; else noDificil = menor;
    }
    verificar(noFacil < 0.97f && noFacil >= 0.959f, "no facil, quem foge do jogador espera ate 4%");
    verificar(noDificil > noFacil && noDificil < 1.0f, "no dificil a espera e menor");

    /* Um rival muito atrás se esforça mais (pilota mais perto do limite), sem ganhar motor */
    iniciarCorrida(&corrida, pista, 0, NIVEL_MEDIO);
    corrida.contagemRegressiva = 0.0f;
    corrida.jogador.distanciaPercorrida += 900.0f * UNIDADES_POR_METRO;
    Comandos nada = { 0 };
    for (int q = 0; q < 60 * 5; q++) {
        corrida.jogador.distanciaPercorrida += 0.0f;
        atualizarCorrida(&corrida, nada, 1.0f / 60.0f);
    }
    float maior = 0.0f, maiorEsforco = 0.0f;
    for (int r = 0; r < TOTAL_RIVAIS; r++) {
        maior = fmaxf(maior, corrida.rivais[r].ritmoDoPelotao);
        maiorEsforco = fmaxf(maiorEsforco, corrida.rivais[r].esforco);
    }
    printf("     rivais 900 m atras do jogador: ritmo ate %.3f, esforco ate %.3f\n", maior, maiorEsforco);
    verificar(maior <= 1.0f, "quem ficou para tras nao ganha motor: nunca anda mais que o carro dele");
    verificar(maiorEsforco > 0.0f && maiorEsforco <= 0.06f, "quem ficou muito para tras se esforca mais, ate 6% mais perto do limite");
    liberarPista(pista);
}

/* ---------- Contato das batidas (faíscas e som de lata) ---------- */
static void testarContatoDasBatidas(void) {
    printf("\n[Teste] Contato das batidas (para as faiscas e o som de lata)\n");
    Pista *pista = criarPista();
    Corrida corrida;
    float quadro = 1.0f / 60.0f;
    Carro *rival = &corrida.rivais[0];
    ladoALado(&corrida, pista, rival, -8.0f, 0.0f, 0.0f, 9000.0f, 3000.0f);
    Comandos reto = { 1, 0, 0, 0, 0.0f, 0, 0 };
    int tocou = 0, comCarro = 0, osDois = 0, pertoDosCarros = 0;
    for (int i = 0; i < 90 && !tocou; i++) {
        atualizarCorrida(&corrida, reto, quadro);
        if (corrida.jogador.contato > 0.0f) {
            tocou = 1;
            comCarro = corrida.jogador.contatoComCarro;
            osDois = rival->contato > 0.0f && rival->contatoComCarro;
            PontoNoMundo p = pontoNoMundo(corrida.jogador.posicao, corrida.jogador.deslocamentoLateral);
            float dx = p.x - corrida.jogador.contatoX, dz = p.z - corrida.jogador.contatoZ;
            pertoDosCarros = sqrtf(dx * dx + dz * dz) < 6.0f;
        }
    }
    verificar(tocou && comCarro && osDois, "batida de carro: os dois carros registram o contato com carro");
    verificar(pertoDosCarros, "o ponto do contato (das faiscas) fica entre os carros");
    corrida.jogador.velocidade = 0.0f;
    rival->distanciaPercorrida += 100.0f * UNIDADES_POR_METRO;
    moverNaPista(&rival->posicao, 100.0f * UNIDADES_POR_METRO);
    Comandos nada = { 0, 0, 0, 0, 0.0f, 0, 0 };
    atualizarCorrida(&corrida, nada, quadro);
    verificar(corrida.jogador.contato == 0.0f, "sem encostar em nada, o contato volta a zero no quadro seguinte");
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
    testarMergeSort();
    testarEscolhaDaGaragem();
    testarVolante();
    testarObjetosDaPista();
    testarDirigibilidade();
    testarGridDeLargada();
    testarBatidaDoMcQueen();
    testarLiderancaDoMcQueen();
    testarUltrapassagensDoMcQueen();
    testarContatoDasBatidas();
    testarSetoresEIntervalos();
    testarMotor();
    testarVoltaDaVitoria();
    testarPeVacuoEDerrapagem();
    testarFreioAlivioECambio();
    testarPistasNoMundo();
    testarDepoisDaChegada();
    testarRetaDosBoxes();
    testarPitstop();
    testarFreioDeMao();
    testarNiveisEDesgaste();
    testarPneuEstourado();
    testarVibracao();
    testarConfiguracoes();
    testarMotorGranular();
    testarCurvaNaMaoDoPiloto();
    testarCambio();
    testarTransitoNoPit();
    testarPitEmTodasAsPistas();
    testarCenarioDosMapas();
    testarDirecaoGuiada();
    testarSairDaVagaComCarroNaFrente();
    testarGuidoForaDoCarro();
    testarSuspensao();
    testarRe();
    testarSaidaDoPitNaMaoDoJogador();
    testarPelotao();
    testarGiroDasRodas();
    testarColisaoEEquilibrio();
    testarCameraLivre();
    testarNitro();

    if (totalFalhas == 0) {
        printf("\nTodos os testes passaram.\n");
        return 0;
    }
    printf("\n%d teste(s) falharam.\n", totalFalhas);
    return 1;
}
