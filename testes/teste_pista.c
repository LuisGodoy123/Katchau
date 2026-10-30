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
#include "obstaculos.h"
#include "motor.h"

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
    verificar(tempo100 > 2.0f && tempo100 < 3.0f, "0-100 km/h em 2 a 3 s (como um F1)");
    verificar(tempo300 > 9.0f && tempo300 < 13.0f, "0-300 km/h em 9 a 13 s, nao em 5");
    verificar(tempo200 - tempo100 < tempo300 - tempo200,
              "de 200 a 300 demora mais que de 100 a 200 (o ar segura o carro)");
    verificar(carro.velocidade == carro.velocidadeMaxima, "acelerando, chega na velocidade maxima e para ali");
    FichaTecnica ficha = { "Teste", "", 12000.0f, 2400.0f, 0.5f };
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
    iniciarCorrida(&corrida, pista, 0);
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
    Carro carros[TOTAL_CARROS] = { 0 };   /* tudo zerado: ninguem chegou ainda */
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
    iniciarCorrida(&corrida, pista, 0);
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
        { "A", "", 1.0f, 1.0f, 0.5f }, { "B", "", 1.0f, 1.0f, 0.5f },
        { "C", "", 1.0f, 1.0f, 0.5f }, { "D", "", 1.0f, 1.0f, 0.5f },
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
    iniciarCorrida(&corrida, pista, escolhido);

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
    Comandos acelerar = { 1, 0, 0, 0 };
    float distanciaAntes = corrida.jogador.distanciaPercorrida;
    atualizarCorrida(&corrida, acelerar, 1.0f);
    verificar(corrida.jogador.distanciaPercorrida == distanciaAntes && corrida.tempoDeCorrida == 0.0f,
              "durante a contagem regressiva ninguem anda e o relogio nao corre");

    /* Corre até todo mundo chegar e confere a ordem de chegada */
    float quadro = 1.0f / 60.0f;
    int quadros = 0;
    while (!todosChegaram(&corrida) && quadros < 60 * 60 * 15) {
        Comandos comandos = atualizarIA(&corrida.jogador, carroDaFrente(&corrida, &corrida.jogador));
        atualizarCorrida(&corrida, comandos, quadro);
        quadros++;
    }
    /* Depois de todos chegarem, os carros continuam andando: a ordem não pode mudar */
    for (int i = 0; i < 60 * 20; i++) {
        Comandos nenhum = { 0, 0, 0, 0 };
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
    Comandos direita = { 0, 0, 0, 1 };
    Comandos nenhum = { 0, 0, 0, 0 };

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
            int ficaDePe = objeto->tipo != OBJETO_PORTICO && objeto->tipo != OBJETO_MARCA_GRID;
            if (ficaDePe && lado <= 2.5f) {
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
    Comandos direitaSemPedal = { 0, 0, 0, 1 };
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
    Comandos nenhum = { 0, 0, 0, 0 };
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

static void testarGridDeLargada(void) {
    printf("\n14) Grid de largada antes da linha de chegada\n");
    Pista *pista = criarPista();
    Corrida corrida;
    iniciarCorrida(&corrida, pista, 0);
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
    Comandos acelerar = { 1, 0, 0, 0 };
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
    iniciarCorrida(&corrida, pista, 0);
    float quadro = 1.0f / 60.0f;

    verificar(bandeiraDaCorrida(&corrida) == BANDEIRA_LARGADA, "na contagem regressiva a bandeira e de largada");
    for (int i = 0; i < 60 * 5; i++) {
        atualizarCorrida(&corrida, atualizarIA(&corrida.jogador, carroDaFrente(&corrida, &corrida.jogador)), quadro);
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
        atualizarCorrida(&corrida, atualizarIA(&corrida.jogador, carroDaFrente(&corrida, &corrida.jogador)), quadro);
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
    iniciarCorrida(&corrida, pista, 2);
    float quadro = 1.0f / 60.0f;
    float estimativaA6s = -1.0f, tempoA6s = 0.0f;

    while (!corrida.terminou && corrida.tempoDeCorrida < 600.0f) {
        float estimativa = segundosAteAVitoria(&corrida);
        if (estimativaA6s < 0.0f && estimativa >= 0.0f && estimativa <= 6.0f) {
            estimativaA6s = estimativa;
            tempoA6s = corrida.tempoDeCorrida;
        }
        atualizarCorrida(&corrida, atualizarIA(&corrida.jogador, carroDaFrente(&corrida, &corrida.jogador)), quadro);
    }
    verificar(jogadorVenceu(&corrida), "o carro 2 dirigido pela IA vence");
    float real = corrida.jogador.tempoDeChegada - tempoA6s;
    printf("     a ~6 s da linha: estimativa %.2f s, real %.2f s\n", estimativaA6s, real);
    verificar(estimativaA6s >= 0.0f && fabsf(estimativaA6s - real) < 0.3f, "estimativa ate a linha erra menos de 0,3 s");
    verificar(segundosAteAVitoria(&corrida) < 0.0f, "depois da chegada nao ha mais estimativa");

    float melhorVolta = corrida.jogador.melhorVolta;
    float tempos[TOTAL_VOLTAS];
    for (int i = 0; i < TOTAL_VOLTAS; i++) {
        tempos[i] = corrida.jogador.temposDeVolta[i];
    }
    Comandos nenhumComando = { 0, 0, 0, 0 };
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
        atualizarMotor(&motor, &carro, quadro);
    }
    verificar(motor.marcha == 0 && motor.giro < 0.2f && motor.carga < 0.05f, "parado: 1a marcha, rotacao baixa, sem carga");

    /* Pé no fundo parado: a rotação sobe antes do carro andar */
    carro.ultimosComandos.acelerar = 1;
    for (int i = 0; i < 60; i++) {
        atualizarMotor(&motor, &carro, quadro);
    }
    verificar(motor.giro > 0.45f && motor.carga > 0.9f, "pe no fundo parado: o motor sobe e pega carga");

    /* Arrancada: a velocidade sobe até a máxima, as marchas vão passando */
    int trocas = 0, quedas = 0, ultimaMarcha = motor.marcha, giroMaximo = 0;
    float giroAntes = motor.giro, maiorQueda = 0.0f;
    int marchaMontonica = 1;
    for (int i = 0; i < 60 * 14; i++) {
        carro.velocidade += (carro.velocidadeMaxima / 12.0f) * quadro;
        if (carro.velocidade > carro.velocidadeMaxima) {
            carro.velocidade = carro.velocidadeMaxima;
        }
        atualizarMotor(&motor, &carro, quadro);
        if (motor.marcha != ultimaMarcha) {
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
    verificar(quedas > 0 && motor.giro > 0.9f, "a rotacao cai quando o cambio troca e volta ao limite no fim");
    verificar(!giroMaximo, "a rotacao nunca passa de 1");

    /* Soltar o pé: a carga some e a rotação cai */
    carro.ultimosComandos.acelerar = 0;
    for (int i = 0; i < 30; i++) {
        atualizarMotor(&motor, &carro, quadro);
    }
    verificar(motor.carga < 0.05f, "pe fora do acelerador: sem carga");

    /* Freando forte, o motor reduz as marchas até a 1ª */
    carro.ultimosComandos.frear = 1;
    for (int i = 0; i < 60 * 4; i++) {
        carro.velocidade -= 12000.0f * quadro;
        if (carro.velocidade < 0.0f) {
            carro.velocidade = 0.0f;
        }
        atualizarMotor(&motor, &carro, quadro);
    }
    verificar(motor.marcha == 0 && motor.giro < 0.2f, "parado de novo: volta para a 1a e para a marcha lenta");
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
    testarSetoresEIntervalos();
    testarMotor();
    testarVoltaDaVitoria();

    if (totalFalhas == 0) {
        printf("\nTodos os testes passaram.\n");
        return 0;
    }
    printf("\n%d teste(s) falharam.\n", totalFalhas);
    return 1;
}
