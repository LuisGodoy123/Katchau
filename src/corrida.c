/*
 * corrida.c
 * ------------------------------------------------------------------
 * Regras da corrida. Veja corrida.h.
 * ------------------------------------------------------------------
 */

#include <stddef.h>
#include "corrida.h"
#include "ia.h"
#include "ordenacao.h"

/* ---------- Grid de largada (as medidas ficam em pista.h) ---------- */
#define LUGAR_DO_JOGADOR       (TOTAL_CARROS - 1)   /* o jogador larga em último */

/* ---------- Colisão ---------- */
#define DISTANCIA_LATERAL_BATIDA  0.25f   /* mais perto que isso, os carros se tocam */
#define PERDA_NA_BATIDA           0.8f    /* quem bate atrás fica com 80% da velocidade do outro */

/* ---------- Intervalo entre carros ---------- */
#define VELOCIDADE_DE_REFERENCIA  9200.0f   /* média de uma volta (~517 mil unidades em ~56 s): converte distância em tempo */

/* ---------- Contagem regressiva ---------- */
#define SEGUNDOS_DE_CONTAGEM   3.0f    /* 3, 2, 1... KATCHAU! */

/* Os 8 carros do jogo. O número de cada um é a posição neste vetor e
 * define a cor no desenho (0 = vermelho, 1 = amarelo...). O jogador
 * escolhe um deles na garagem; os outros 7 viram os rivais.
 * A aderência pesa muito no tempo de volta, então ela varia pouco.
 * Os nomes e o jeito de cada um são originais do KATCHAU. */
static const FichaTecnica FICHAS[TOTAL_CARROS] = {
    /* nome         descrição                         vel.max   acel.    aderência */
    { "Katchau",    "o carro da casa, equilibrado",    12000.0f, 2400.0f, 0.50f },
    { "Barao",      "esportivo arrogante, voa na reta", 12500.0f, 2200.0f, 0.47f },
    { "Neon",       "gruda nas curvas",                11700.0f, 2400.0f, 0.55f },
    { "Trovao",     "arranca forte",                   12000.0f, 2700.0f, 0.49f },
    { "Ze Pistao",  "certinho em tudo",                11800.0f, 2300.0f, 0.51f },
    { "Fuscao",     "lento, mas nao sai da pista",     11100.0f, 2300.0f, 0.58f },
    { "Bolinha",    "pequeno e esperto na largada",    11400.0f, 2800.0f, 0.51f },
    { "Ferrugem",   "picape enferrujada",              11200.0f, 2100.0f, 0.54f },
};

/*
 * fichaDoCarro
 * Recebe: o número do carro (0 a TOTAL_CARROS - 1).
 * Devolve: a ficha técnica dele.
 */
const FichaTecnica *fichaDoCarro(int numero) {
    return &FICHAS[numero];
}

/*
 * colocarNoGrid
 * Põe o carro no seu lugar de largada, que fica ANTES da linha de
 * chegada (como num autódromo de verdade), no colchete pintado no
 * asfalto: o lugar 0 (pole) é o mais perto da linha.
 *
 * Como o carro ainda vai cruzar a linha logo depois da largada, ele
 * começa com voltas = -1: esse primeiro cruzamento só abre a volta 1
 * (veja registrarVolta). A distância percorrida começa negativa, do
 * tamanho do que falta até a linha, para a classificação da largada
 * sair certa (quem está mais perto da linha, na frente).
 * Recebe: o carro (já criado na linha de chegada) e o lugar (0 = pole).
 * Devolve: nada.
 */
static void colocarNoGrid(Carro *carro, int lugar) {
    float distanciaAteALinha = distanciaDoLugarAteALinha(lugar);

    moverNaPista(&carro->posicao, -distanciaAteALinha);   /* anda para trás, atravessando a linha */
    carro->distanciaPercorrida = -distanciaAteALinha;
    carro->voltas = -1;

    carro->deslocamentoLateral = lateralDoLugar(lugar);
    carro->linhaAlvo = carro->deslocamentoLateral;
}

/*
 * criarCarroDaFicha
 * Cria um carro na linha de chegada a partir de uma ficha técnica.
 * Recebe: a ficha, o número do carro e a pista.
 * Devolve: o carro.
 */
static Carro criarCarroDaFicha(FichaTecnica ficha, int numero, Pista *pista) {
    return criarCarro(ficha.nome, numero, pista->largada, 0.0f,
                      ficha.velocidadeMaxima, ficha.aceleracao, ficha.aderencia);
}

/*
 * iniciarCorrida
 * Prepara uma corrida nova: o jogador com o carro escolhido na garagem,
 * os outros 7 carros como rivais, todos no grid, relógio zerado,
 * contagem regressiva armada e a classificação montada.
 * Serve também para correr de novo.
 * Recebe: a corrida a preparar, a pista e o número do carro do jogador.
 * Devolve: nada.
 */
void iniciarCorrida(Corrida *corrida, Pista *pista, int carroDoJogador) {
    corrida->pista = pista;
    corrida->tempoDeCorrida = 0.0f;
    corrida->contagemRegressiva = SEGUNDOS_DE_CONTAGEM;
    corrida->terminou = 0;
    corrida->posicaoFinal = 0;

    corrida->jogador = criarCarroDaFicha(FICHAS[carroDoJogador], carroDoJogador, pista);
    colocarNoGrid(&corrida->jogador, LUGAR_DO_JOGADOR);
    corrida->classificacao[0] = &corrida->jogador;

    /* Rivais: todos os carros, menos o que o jogador pegou */
    int rival = 0;
    for (int numero = 0; numero < TOTAL_CARROS; numero++) {
        if (numero == carroDoJogador) {
            continue;
        }
        corrida->rivais[rival] = criarCarroDaFicha(FICHAS[numero], numero, pista);
        colocarNoGrid(&corrida->rivais[rival], rival);   /* rivais nos lugares 0 a 6 */
        corrida->classificacao[rival + 1] = &corrida->rivais[rival];
        rival++;
    }

    corrida->totalSegmentos = contarSegmentos(pista);
    corrida->voltaMaisRapida = 0.0f;
    for (int s = 0; s < TOTAL_SETORES; s++) {
        corrida->melhoresSetoresDaCorrida[s] = 0.0f;
    }
    for (int i = 0; i < TOTAL_CARROS; i++) {
        Carro *carro = corrida->classificacao[i];
        carro->setorAtual = carro->posicao.segmento->indice * TOTAL_SETORES / corrida->totalSegmentos;
    }

    ordenarCarrosPorDistancia(corrida->classificacao, TOTAL_CARROS);
}

/*
 * somaDosSetores
 * Recebe: um vetor de tempos de setor e até qual setor somar (inclusive).
 * Devolve: a soma.
 */
static float somaDosSetores(const float setores[], int ate) {
    float soma = 0.0f;
    for (int s = 0; s <= ate; s++) {
        soma += setores[s];
    }
    return soma;
}

/*
 * atualizarSetores
 * Vê se o carro passou para outro setor (a volta é dividida em 3 partes
 * iguais de segmentos) e, se passou, guarda o tempo do setor que acabou:
 *   - atualiza o melhor setor do carro e o melhor da corrida;
 *   - calcula a diferença para a melhor volta do carro naquele ponto
 *     (soma dos setores desta volta - soma dos da melhor volta).
 * Sair do grid e cruzar a linha pela primeira vez não fecha setor:
 * o setor 1 da volta 1 conta desde a largada, como a volta.
 * Chamada ANTES de registrarVolta, que ainda compara com a melhor volta antiga.
 * Recebe: a corrida, o carro e se ele acabou de cruzar a linha.
 * Devolve: nada.
 */
static void atualizarSetores(Corrida *corrida, Carro *carro, int cruzouALinha) {
    int setor = carro->posicao.segmento->indice * TOTAL_SETORES / corrida->totalSegmentos;
    if (setor == carro->setorAtual) {
        return;
    }
    int terminado = carro->setorAtual;
    carro->setorAtual = setor;
    if (carro->voltas <= 0 && (cruzouALinha || carro->voltas < 0)) {
        return;   /* ainda saindo do grid: a volta 1 só começou agora */
    }

    float tempo = corrida->tempoDeCorrida - carro->inicioDoSetor;
    carro->inicioDoSetor = corrida->tempoDeCorrida;
    carro->setoresDaVolta[terminado] = tempo;

    if (carro->melhoresSetores[terminado] == 0.0f || tempo < carro->melhoresSetores[terminado]) {
        carro->melhoresSetores[terminado] = tempo;
    }
    if (corrida->melhoresSetoresDaCorrida[terminado] == 0.0f ||
        tempo < corrida->melhoresSetoresDaCorrida[terminado]) {
        corrida->melhoresSetoresDaCorrida[terminado] = tempo;
    }

    if (carro->melhorVolta > 0.0f) {
        carro->diferenca = somaDosSetores(carro->setoresDaVolta, terminado)
                           - somaDosSetores(carro->setoresDaMelhorVolta, terminado);
        carro->temDiferenca = 1;
    }
}

/*
 * registrarVolta
 * Guarda quanto tempo durou a volta que o carro acabou de completar.
 * Na última volta, guarda também o tempo de chegada. O primeiro
 * cruzamento da linha (saindo do grid) não é volta: como no F1, a
 * volta 1 é contada desde a largada até a linha na volta seguinte.
 * Também atualiza a melhor volta do carro (com os setores dela) e a
 * volta mais rápida da corrida.
 * Recebe: a corrida e o carro (com carro->voltas já somado).
 * Devolve: nada.
 */
static void registrarVolta(Corrida *corrida, Carro *carro) {
    float tempoDeCorrida = corrida->tempoDeCorrida;
    if (carro->voltas == 0) {
        return;   /* acabou de cruzar a linha saindo do grid: começou a volta 1, que conta desde a largada */
    }
    int indiceDaVolta = carro->voltas - 1;   /* volta 1 fica na posição 0 */
    float tempoDaVolta = tempoDeCorrida - carro->inicioDaVolta;
    if (indiceDaVolta >= 0 && indiceDaVolta < TOTAL_VOLTAS) {
        carro->temposDeVolta[indiceDaVolta] = tempoDaVolta;
    }

    /* Melhor volta: guarda também os setores dela, para comparar as próximas */
    if (carro->melhorVolta == 0.0f || tempoDaVolta < carro->melhorVolta) {
        carro->melhorVolta = tempoDaVolta;
        for (int s = 0; s < TOTAL_SETORES; s++) {
            carro->setoresDaMelhorVolta[s] = carro->setoresDaVolta[s];
        }
    }
    if (corrida->voltaMaisRapida == 0.0f || carro->melhorVolta < corrida->voltaMaisRapida) {
        corrida->voltaMaisRapida = carro->melhorVolta;
    }
    for (int s = 0; s < TOTAL_SETORES; s++) {
        carro->setoresDaVolta[s] = 0.0f;   /* volta nova, setores zerados */
    }
    if (carro->voltas == TOTAL_VOLTAS) {
        carro->tempoDeChegada = tempoDeCorrida;
    }
    carro->inicioDaVolta = tempoDeCorrida;
}

/*
 * tratarBatida
 * Se dois carros estão no mesmo segmento e lado a lado, quem vem atrás
 * bate na traseira do outro e perde velocidade.
 * Recebe: os dois carros.
 * Devolve: nada.
 */
static void tratarBatida(Carro *a, Carro *b) {
    if (a->posicao.segmento != b->posicao.segmento) {
        return;
    }

    float diferencaLateral = a->deslocamentoLateral - b->deslocamentoLateral;
    if (diferencaLateral < 0) {
        diferencaLateral = -diferencaLateral;
    }
    if (diferencaLateral > DISTANCIA_LATERAL_BATIDA) {
        return;
    }

    /* Descobre quem está atrás dentro do segmento */
    Carro *atras = a;
    Carro *frente = b;
    if (a->posicao.distanciaNoSegmento > b->posicao.distanciaNoSegmento) {
        atras = b;
        frente = a;
    }

    /* Só é batida se quem está atrás vem mais rápido (está alcançando) */
    if (atras->velocidade > frente->velocidade) {
        atras->velocidade = frente->velocidade * PERDA_NA_BATIDA;
    }
}

/*
 * verificarBatidas
 * Testa todos os pares de carros (8 carros = 28 pares).
 * Recebe: a corrida.
 * Devolve: nada.
 */
static void verificarBatidas(Corrida *corrida) {
    for (int i = 0; i < TOTAL_CARROS; i++) {
        for (int j = i + 1; j < TOTAL_CARROS; j++) {
            tratarBatida(corrida->classificacao[i], corrida->classificacao[j]);
        }
    }
}

/*
 * atualizarCorrida
 * Avança a corrida um quadro:
 *   1) move o jogador com os comandos do teclado;
 *   2) move cada rival com os comandos da IA;
 *   3) trata as batidas;
 *   4) reordena a classificação (Insertion Sort);
 *   5) verifica se o jogador terminou.
 * Antes disso, enquanto a contagem regressiva não acaba, ninguém sai
 * do lugar e o relógio não anda.
 * Depois do fim, o jogador segue sem acelerar até parar e os rivais
 * continuam correndo. O relógio também continua, para cada rival ter
 * o seu tempo de chegada; o do jogador fica guardado no carro dele.
 * Recebe: a corrida, os comandos do jogador e o tempo do quadro.
 * Devolve: nada.
 */
void atualizarCorrida(Corrida *corrida, Comandos comandosJogador, float tempoQuadro) {
    if (corrida->contagemRegressiva > 0.0f) {
        corrida->contagemRegressiva -= tempoQuadro;
        return;
    }

    corrida->tempoDeCorrida += tempoQuadro;
    if (corrida->terminou) {
        Comandos nenhumComando = { 0, 0, 0, 0 };
        comandosJogador = nenhumComando;
    }

    /* 1) Jogador */
    int cruzou = atualizarCarro(&corrida->jogador, comandosJogador, tempoQuadro) > 0;
    atualizarSetores(corrida, &corrida->jogador, cruzou);
    if (cruzou) {
        registrarVolta(corrida, &corrida->jogador);
    }

    /* 2) Rivais: a IA olha o carro logo à frente de cada um */
    for (int i = 0; i < TOTAL_RIVAIS; i++) {
        Carro *rival = &corrida->rivais[i];
        Comandos comandosIA = atualizarIA(rival, carroDaFrente(corrida, rival));
        cruzou = atualizarCarro(rival, comandosIA, tempoQuadro) > 0;
        atualizarSetores(corrida, rival, cruzou);
        if (cruzou) {
            registrarVolta(corrida, rival);
        }
    }

    /* 3) Batidas */
    verificarBatidas(corrida);

    /* 4) Classificação */
    ordenarCarrosPorDistancia(corrida->classificacao, TOTAL_CARROS);

    /* 5) Fim de corrida */
    if (!corrida->terminou && corrida->jogador.voltas >= TOTAL_VOLTAS) {
        corrida->terminou = 1;
        corrida->posicaoFinal = posicaoDoCarro(corrida, &corrida->jogador);
    }
}

/*
 * todosChegaram
 * Recebe: a corrida.
 * Devolve: 1 se os 8 carros já completaram TOTAL_VOLTAS, 0 se não.
 */
int todosChegaram(Corrida *corrida) {
    for (int i = 0; i < TOTAL_CARROS; i++) {
        if (corrida->classificacao[i]->voltas < TOTAL_VOLTAS) {
            return 0;
        }
    }
    return 1;
}

/*
 * voltaAtual
 * Número da volta que o carro está correndo, para mostrar na tela.
 * Recebe: o carro.
 * Devolve: de 1 até TOTAL_VOLTAS.
 */
int voltaAtual(Carro *carro) {
    int volta = carro->voltas + 1;
    if (volta < 1) {
        volta = 1;   /* ainda no grid, antes de cruzar a linha */
    }
    if (volta > TOTAL_VOLTAS) {
        volta = TOTAL_VOLTAS;
    }
    return volta;
}

/*
 * posicaoDoCarro
 * Procura o carro na classificação.
 * Recebe: a corrida e o carro.
 * Devolve: a posição (1 = primeiro), ou 0 se o carro não estiver nela.
 */
int posicaoDoCarro(Corrida *corrida, Carro *carro) {
    for (int i = 0; i < TOTAL_CARROS; i++) {
        if (corrida->classificacao[i] == carro) {
            return i + 1;
        }
    }
    return 0;
}

/*
 * carroDaFrente
 * Como a classificação está ordenada, o carro logo à frente é
 * simplesmente o anterior no vetor.
 * Recebe: a corrida e o carro.
 * Devolve: o carro logo à frente, ou NULL se ele for o 1º.
 */
Carro *carroDaFrente(Corrida *corrida, Carro *carro) {
    int posicao = posicaoDoCarro(corrida, carro);
    if (posicao <= 1) {
        return NULL;
    }
    return corrida->classificacao[posicao - 2];   /* posição 2 -> índice 0 */
}

/*
 * intervaloEmSegundos
 * Quanto um carro está à frente (> 0) ou atrás (< 0) de outro, em
 * segundos, para o painel de posições. Se os dois já chegaram, é a
 * diferença dos tempos de chegada; senão, a diferença de distância
 * convertida pela velocidade média de uma volta.
 * Recebe: o carro e o carro de referência (o jogador).
 * Devolve: o intervalo em segundos.
 */
float intervaloEmSegundos(Carro *carro, Carro *referencia) {
    if (carro->tempoDeChegada > 0.0f && referencia->tempoDeChegada > 0.0f) {
        return referencia->tempoDeChegada - carro->tempoDeChegada;
    }
    return (carro->distanciaPercorrida - referencia->distanciaPercorrida) / VELOCIDADE_DE_REFERENCIA;
}

/*
 * situacaoDoSetor
 * Como foi um setor na volta que o carro está fazendo agora.
 * Recebe: a corrida, o carro e o setor (0 a 2).
 * Devolve: pendente, mais lento, melhor pessoal ou melhor da corrida.
 */
SituacaoDoSetor situacaoDoSetor(Corrida *corrida, Carro *carro, int setor) {
    float tempo = carro->setoresDaVolta[setor];
    if (tempo == 0.0f) {
        return SETOR_PENDENTE;
    }
    if (tempo <= corrida->melhoresSetoresDaCorrida[setor]) {
        return SETOR_MELHOR_DA_CORRIDA;
    }
    if (tempo <= carro->melhoresSetores[setor]) {
        return SETOR_MELHOR_PESSOAL;
    }
    return SETOR_MAIS_LENTO;
}

/*
 * bandeiraDaCorrida
 * Recebe: a corrida.
 * Devolve: largada (na contagem), verde (correndo) ou quadriculada
 *          (depois que o 1º colocado completou a prova).
 */
Bandeira bandeiraDaCorrida(Corrida *corrida) {
    if (corrida->contagemRegressiva > 0.0f) {
        return BANDEIRA_LARGADA;
    }
    if (corrida->classificacao[0]->tempoDeChegada > 0.0f) {
        return BANDEIRA_QUADRICULADA;
    }
    return BANDEIRA_VERDE;
}
