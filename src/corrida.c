/*
 * corrida.c
 * ------------------------------------------------------------------
 * Regras da corrida. Veja corrida.h.
 * ------------------------------------------------------------------
 */

#include <stddef.h>
#include <math.h>
#include "corrida.h"
#include "pitstop.h"
#include "ia.h"
#include "ordenacao.h"

/* ---------- Grid de largada (as medidas ficam em pista.h) ---------- */
#define VELOCIDADE_DA_PANCADA_FORTE 3000.0f  /* diferença de velocidade numa batida forte */
#define LADO_DA_PANCADA_FORTE       10.0f    /* metros por segundo, de lado */
#define PANCADA_MINIMA              0.25f
#define TROCA_MAIS_RAPIDA_DOS_RIVAIS 4.5f  /* segundos parados na vaga */
#define PASSO_DA_TROCA_DOS_RIVAIS    0.3f
#define LUGAR_DO_JOGADOR       (TOTAL_CARROS - 1)   /* o jogador larga em último */

/* ---------- Colisão ---------- */
#define RESTITUICAO               0.6f      /* na traseira, quanto da diferença de velocidade passa de um para o outro */
#define ATRITO_DE_LADO            3500.0f   /* raspando de lado, os dois perdem isso de velocidade por segundo */
#define TRANCO_MINIMO             2.0f      /* metros por segundo: toda batida de lado separa um pouco */
#define TRANCO_POR_FECHAMENTO     0.7f      /* mais isso vezes a velocidade com que se encontraram */
#define FORA_DO_CENTRO            0.30f     /* contato mais fora do centro que isso (0 a 1) pode fazer rodar */
#define GIRO_POR_TRASEIRA         0.55f     /* graus por segundo por unidade/s de batida na traseira, vezes a excentricidade */
#define GIRO_POR_LADO             75.0f     /* graus por segundo por metro/s de batida de lado, vezes a excentricidade */

/* ---------- O que a IA enxerga ---------- */
#define VISAO_PARA_TRAS_NO_PIT    22.0f     /* metros: quem sai da vaga não entra na frente de quem vem */
#define ALCANCE_DA_VISAO          60.0f     /* metros à frente */
#define DISTANCIA_AO_LADO         1.5f      /* metros de folga: mais perto que isso, o lado está ocupado */

/* ---------- Dificuldade ---------- */

/* ---------- Vácuo ---------- */
#define DISTANCIA_DO_VACUO        3600.0f   /* ~30 m: mais perto que isso atrás de alguém, pega vácuo */
#define LARGURA_DO_VACUO          3.0f      /* metros: tem que estar quase na mesma linha do carro da frente */
#define RAPIDEZ_DO_VACUO          3.0f      /* o vácuo entra e sai em ~1/3 s, sem tranco */

/* ---------- Intervalo entre carros ---------- */
#define VELOCIDADE_DE_REFERENCIA  9900.0f   /* média de uma volta (~517 mil unidades em ~52 s): converte distância em tempo */

/* ---------- Contagem regressiva ---------- */
/* Um respiro só com o motor, depois 3, 2, 1... KATCHAU! */
#define SEGUNDOS_DE_CONTAGEM   (ESPERA_ANTES_DA_CONTAGEM + NUMEROS_DA_CONTAGEM * SEGUNDOS_POR_NUMERO)

/* Os 14 carros do jogo. O número de cada um é a posição neste vetor e
 * define o modelo 3D (carros3d.c). O jogador escolhe um deles na garagem;
 * os outros 13 viram os rivais. Os carros têm jeitos diferentes, mas o mesmo
 * ritmo: a aderência de cada um foi calculada para a volta limpa dar o mesmo
 * tempo em Interlagos, e a velocidade máxima varia pouco porque decide o oval. */
static const FichaTecnica FICHAS[TOTAL_CARROS] = {
    /* nome             descrição                          vel.max   acel.    aderência */
    { "McQueen", "o carro da casa, equilibrado", 12000.0f, 2400.0f, 0.500f, 4.4f, 2.01f },
    { "The King", "veterano, o melhor na reta", 12120.0f, 2250.0f, 0.499f, 5.3f, 2.32f },
    { "Doc Hudson", "lenda, gruda nas curvas", 11900.0f, 2450.0f, 0.511f, 5.0f, 2.19f },
    { "Francesco", "formula, arranca forte", 11980.0f, 2650.0f, 0.491f, 5.0f, 2.49f },
    { "Sally", "certinha em tudo", 11950.0f, 2450.0f, 0.504f, 4.4f, 2.06f },
    { "Ramone", "estiloso e estavel", 11930.0f, 2400.0f, 0.510f, 5.4f, 2.32f },
    { "Luigi", "pequeno e esperto na largada", 11880.0f, 2700.0f, 0.499f, 3.0f, 1.7f },
    { "Guido", "minusculo, melhor arrancada", 11880.0f, 2750.0f, 0.498f, 2.4f, 1.16f },
    { "Mater", "guincho firme nas curvas", 11940.0f, 2300.0f, 0.515f, 5.2f, 2.46f },
    { "Sarge", "jipe militar, firme no chao", 11920.0f, 2350.0f, 0.514f, 3.4f, 1.88f },
    { "Fillmore", "sem pressa, gruda na pista", 11900.0f, 2350.0f, 0.516f, 4.3f, 2.06f },
    { "Finn McMissile", "espiao, rapido na reta", 12060.0f, 2380.0f, 0.498f, 4.6f, 2.12f },
    { "Wingo", "tuner, nervoso na arrancada", 12000.0f, 2550.0f, 0.494f, 4.4f, 2.67f },
    { "Snot Rod", "motorzao, dificil de segurar", 12100.0f, 2500.0f, 0.485f, 4.6f, 2.53f },
};

/* Os níveis: o médio é o equilíbrio original; o fácil não tem pitstop, e só o difícil gasta os pneus */
static const FichaDoNivel NIVEIS[TOTAL_NIVEIS] = {
    { "FÁCIL",   3, 0.87f, 0.80f, 0, 0 },
    { "MÉDIO",   5, 0.93f, 0.86f, 1, 0 },
    { "DIFÍCIL", 8, 0.97f, 0.91f, 1, 1 },
};

/*
 * fichaDoNivel
 * Recebe: o nível.
 * Devolve: a ficha dele.
 */
const FichaDoNivel *fichaDoNivel(NivelDaCorrida nivel) {
    return &NIVEIS[nivel];
}

/*
 * fichaDoCarro
 * Recebe: o número do carro (0 a TOTAL_CARROS - 1).
 * Devolve: a ficha técnica dele.
 */
const FichaTecnica *fichaDoCarro(int numero) {
    return &FICHAS[numero];
}

/*
 * lateralEmMetros
 * A largura da pista muda de um trecho para outro, então comparar dois
 * carros só faz sentido em metros.
 * Recebe: o carro.
 * Devolve: o quanto ele está à direita (+) ou à esquerda (-) da linha da pista.
 */
static float lateralEmMetros(Carro *carro) {
    return carro->deslocamentoLateral * larguraDoLado(carro->posicao.segmento, carro->deslocamentoLateral);
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
    carro->linhaAlvo = lateralEmMetros(carro);
}

/*
 * criarCarroDaFicha
 * Cria um carro na linha de chegada a partir de uma ficha técnica.
 * Recebe: a ficha, o número do carro e a pista.
 * Devolve: o carro.
 */
static Carro criarCarroDaFicha(FichaTecnica ficha, int numero, Pista *pista) {
    Carro carro = criarCarro(ficha.nome, numero, pista->largada, 0.0f,
                             ficha.velocidadeMaxima, ficha.aceleracao, ficha.aderencia);
    carro.comprimento = ficha.comprimento;
    carro.largura = ficha.largura;
    return carro;
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
void iniciarCorrida(Corrida *corrida, Pista *pista, int carroDoJogador, NivelDaCorrida nivel) {
    const FichaDoNivel *ficha = fichaDoNivel(nivel);
    int temParada = ficha->temParada && pista->temPit;
    int gastaPneu = ficha->gastaPneu && temParada;   /* sem boxes, não haveria como trocar */
    corrida->temParada = temParada;
    corrida->pista = pista;
    corrida->nivel = nivel;
    corrida->totalVoltas = ficha->voltas;
    corrida->tempoDeCorrida = 0.0f;
    corrida->contagemRegressiva = SEGUNDOS_DE_CONTAGEM;
    corrida->terminou = 0;
    corrida->desclassificado = 0;
    corrida->posicaoFinal = 0;

    corrida->jogador = criarCarroDaFicha(FICHAS[carroDoJogador], carroDoJogador, pista);
    colocarNoGrid(&corrida->jogador, LUGAR_DO_JOGADOR);
    corrida->jogador.duracaoDaTroca = DURACAO_DA_TROCA;
    corrida->jogador.voltaDoPit = temParada ? (ficha->voltas + 1) / 2 : 0;   /* se a IA dirigir */
    corrida->jogador.voltasDaProva = ficha->voltas;
    corrida->jogador.gastaPneu = gastaPneu;
    corrida->jogador.pitFeito = !temParada;
    corrida->classificacao[0] = &corrida->jogador;

    /* Rivais: todos os carros, menos o que o jogador pegou */
    int rival = 0;
    for (int numero = 0; numero < TOTAL_CARROS; numero++) {
        if (numero == carroDoJogador) {
            continue;
        }
        corrida->rivais[rival] = criarCarroDaFicha(FICHAS[numero], numero, pista);
        colocarNoGrid(&corrida->rivais[rival], rival);   /* rivais nos lugares 0 a 6 */
        corrida->rivais[rival].habilidade = ficha->habilidadeDaPole
            - (ficha->habilidadeDaPole - ficha->habilidadeDoUltimo) * rival / (TOTAL_RIVAIS - 1);
        corrida->rivais[rival].voltasDaProva = ficha->voltas;
        corrida->rivais[rival].gastaPneu = gastaPneu;
        /* Cada um para numa volta, para os boxes não lotarem; na troca, eles são mais lentos que o Guido */
        corrida->rivais[rival].voltaDoPit = temParada ? 2 + rival * 5 % (ficha->voltas - 2) : 0;
        corrida->rivais[rival].pitFeito = !temParada;
        corrida->rivais[rival].duracaoDaTroca = TROCA_MAIS_RAPIDA_DOS_RIVAIS + (rival % 6) * PASSO_DA_TROCA_DOS_RIVAIS;
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
    if (carro->voltas > carro->voltasDaProva || (carro->voltas == carro->voltasDaProva && !cruzouALinha)) {
        return;   /* já recebeu a bandeirada: as voltas depois do fim não contam */
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
    if (carro->voltas > carro->voltasDaProva) {
        return;   /* volta depois da bandeirada: não entra nos tempos nem na melhor volta */
    }
    int indiceDaVolta = carro->voltas - 1;   /* volta 1 fica na posição 0 */
    float tempoDaVolta = tempoDeCorrida - carro->inicioDaVolta;
    if (indiceDaVolta >= 0 && indiceDaVolta < carro->voltasDaProva) {
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
    if (carro->voltas == carro->voltasDaProva) {
        carro->tempoDeChegada = tempoDeCorrida;
    }
    carro->inicioDaVolta = tempoDeCorrida;
}

/*
 * distanciaNaPista
 * A distância de um carro até outro na pista, em metros: quem está uma volta atrás,
 * no mesmo lugar, também encosta.
 * Recebe: a corrida e os dois carros.
 * Devolve: de -meia volta a +meia volta (positivo = "b" está à frente de "a").
 */
static float distanciaNaPista(const Corrida *corrida, const Carro *a, const Carro *b) {
    float volta = corrida->totalSegmentos * COMPRIMENTO_SEGMENTO;
    float d = fmodf(b->distanciaPercorrida - a->distanciaPercorrida, volta);
    if (d > volta / 2.0f) {
        d -= volta;
    } else if (d < -volta / 2.0f) {
        d += volta;
    }
    return d / UNIDADES_POR_METRO;
}

/*
 * penetracaoEntreCarros
 * O quanto as caixas de dois carros entram uma na outra.
 * Recebe: a corrida, os dois carros e onde guardar as entradas na frente e de lado.
 * Devolve: 1 se elas se tocam.
 */
int penetracaoEntreCarros(const Corrida *corrida, const Carro *a, const Carro *b, float *naFrente, float *deLado) {
    float meioA, larguraA, meioB, larguraB;
    meiasMedidasDoCarro(a, &meioA, &larguraA);
    meiasMedidasDoCarro(b, &meioB, &larguraB);
    *naFrente = meioA + meioB - fabsf(distanciaNaPista(corrida, a, b));
    *deLado = larguraA + larguraB - fabsf(lateralEmMetros((Carro *) a) - lateralEmMetros((Carro *) b));
    return *naFrente > 0.0f && *deLado > 0.0f;
}

/*
 * baterNaTraseira
 * Quem vem atrás passa velocidade para quem está na frente, e os dois se separam.
 * Pegando fora do centro, a traseira de quem está na frente escapa: ele roda.
 * Recebe: quem está atrás, quem está na frente, quanto as caixas entraram e se pode rodar.
 * Devolve: nada.
 */
static void baterNaTraseira(Carro *atras, Carro *frente, float entrada, int podeRodar) {
    atras->empurraoPendente -= entrada / 2.0f * UNIDADES_POR_METRO;
    frente->empurraoPendente += entrada / 2.0f * UNIDADES_POR_METRO;

    float fechando = atras->velocidade - frente->velocidade;
    if (fechando <= 0.0f) {
        return;
    }
    atras->velocidade -= fechando * RESTITUICAO;
    frente->velocidade += fechando * RESTITUICAO;

    /* Excentricidade: 0 de centro a centro, 1 raspando na quina */
    float diferenca = lateralEmMetros(frente) - lateralEmMetros(atras);
    float excentricidade = fabsf(diferenca) / ((atras->largura + frente->largura) / 2.0f);
    if (podeRodar && excentricidade > FORA_DO_CENTRO) {
        float impulso = fechando * GIRO_POR_TRASEIRA * excentricidade;
        fazerRodar(frente, (diferenca > 0.0f) ? -impulso : impulso);
    }
}

/*
 * baterDeLado
 * Os dois se separam e levam um tranco para os lados. Quem toma a batida atrás do
 * centro, com força, roda: é a mesma manobra das corridas de verdade.
 * Recebe: a corrida, os dois carros (b à direita de a), quanto as caixas entraram, se pode rodar
 *         e o tempo do quadro.
 * Devolve: nada.
 */
static void baterDeLado(const Corrida *corrida, Carro *a, Carro *b, float entrada, int podeRodar, float tempoQuadro) {
    a->deslocamentoLateral -= entrada / 2.0f / larguraDoLado(a->posicao.segmento, a->deslocamentoLateral);
    b->deslocamentoLateral += entrada / 2.0f / larguraDoLado(b->posicao.segmento, b->deslocamentoLateral);

    float perda = ATRITO_DE_LADO * tempoQuadro;
    a->velocidade = fmaxf(a->velocidade - perda, 0.0f);
    b->velocidade = fmaxf(b->velocidade - perda, 0.0f);

    float fechando = fmaxf(a->velocidadeDeLado - b->velocidadeDeLado, 0.0f);
    float tranco = TRANCO_MINIMO + fechando * TRANCO_POR_FECHAMENTO;
    a->velocidadeLateral = fminf(a->velocidadeLateral, -tranco / 2.0f);
    b->velocidadeLateral = fmaxf(b->velocidadeLateral, tranco / 2.0f);

    /* Quem está à frente tomou a batida na parte de trás do carro dele */
    float d = distanciaNaPista(corrida, a, b);
    float excentricidade = fabsf(d) / ((a->comprimento + b->comprimento) / 2.0f);
    if (podeRodar && excentricidade > FORA_DO_CENTRO) {
        float impulso = fechando * GIRO_POR_LADO * excentricidade;
        if (d > 0.0f) {
            fazerRodar(b, -impulso);     /* a traseira de b foi empurrada para a direita: o nariz vira para a esquerda */
        } else {
            fazerRodar(a, impulso);      /* a traseira de a foi empurrada para a esquerda: o nariz vira para a direita */
        }
    }
}

/*
 * separarNoPit
 * Dois carros encostando no pit lane. A fila já evita isso (pitstop.c); se acontecer, só
 * os separa: sem tranco nem rodada, e o carro parado na troca nunca sai do lugar.
 * Recebe: a corrida, os dois carros e quanto as caixas entraram.
 * Devolve: nada.
 */
static void separarNoPit(const Corrida *corrida, Carro *a, Carro *b, float naFrente, float deLado) {
    if (naFrente < deLado) {
        Carro *atras = (distanciaNaPista(corrida, a, b) > 0.0f) ? a : b;
        Carro *frente = (atras == a) ? b : a;
        if (atras->estadoDoPit != PIT_TROCANDO) {
            atras->empurraoPendente -= naFrente * UNIDADES_POR_METRO;
            atras->velocidade = fminf(atras->velocidade, frente->velocidade);
        } else {
            frente->empurraoPendente += naFrente * UNIDADES_POR_METRO;
        }
        return;
    }
    Carro *esquerda = (lateralEmMetros(a) < lateralEmMetros(b)) ? a : b;
    Carro *direita = (esquerda == a) ? b : a;
    if (esquerda->estadoDoPit == PIT_TROCANDO) {
        direita->deslocamentoLateral += deLado / larguraDoLado(direita->posicao.segmento, direita->deslocamentoLateral);
    } else if (direita->estadoDoPit == PIT_TROCANDO) {
        esquerda->deslocamentoLateral -= deLado / larguraDoLado(esquerda->posicao.segmento, esquerda->deslocamentoLateral);
    } else {
        esquerda->deslocamentoLateral -= deLado / 2.0f / larguraDoLado(esquerda->posicao.segmento, esquerda->deslocamentoLateral);
        direita->deslocamentoLateral += deLado / 2.0f / larguraDoLado(direita->posicao.segmento, direita->deslocamentoLateral);
    }
}

/*
 * tratarBatida
 * Se as caixas de dois carros se tocam, separa os dois pelo lado em que entraram
 * menos: é por ali que eles acabaram de se encontrar.
 * Recebe: a corrida, os dois carros e o tempo do quadro.
 * Devolve: nada.
 */
static void tratarBatida(Corrida *corrida, Carro *a, Carro *b, float tempoQuadro) {
    float naFrente, deLado;
    if (!penetracaoEntreCarros(corrida, a, b, &naFrente, &deLado)) {
        return;
    }
    if (carroNoPit(a) && carroNoPit(b)) {
        separarNoPit(corrida, a, b, naFrente, deLado);
        return;
    }

    /* A força da batida, para a vibração do controle: a diferença de velocidade entre os dois */
    float choque = fabsf(a->velocidade - b->velocidade) / VELOCIDADE_DA_PANCADA_FORTE
                   + fabsf(a->velocidadeDeLado - b->velocidadeDeLado) / LADO_DA_PANCADA_FORTE;
    float pancada = fminf(PANCADA_MINIMA + choque, 1.0f);
    a->pancada = fmaxf(a->pancada, pancada);
    b->pancada = fmaxf(b->pancada, pancada);

    /* Só roda quem bate com o jogador: rodadas entre rivais só atrapalhariam a corrida
     * longe dos olhos dele, e fechariam a pista para todo mundo */
    int podeRodar = (a == &corrida->jogador || b == &corrida->jogador) && !carroNoPit(a) && !carroNoPit(b);
    if (naFrente < deLado) {
        if (distanciaNaPista(corrida, a, b) > 0.0f) {
            baterNaTraseira(a, b, naFrente, podeRodar);
        } else {
            baterNaTraseira(b, a, naFrente, podeRodar);
        }
    } else if (lateralEmMetros(a) < lateralEmMetros(b)) {
        baterDeLado(corrida, a, b, deLado, podeRodar, tempoQuadro);
    } else {
        baterDeLado(corrida, b, a, deLado, podeRodar, tempoQuadro);
    }
}

/*
 * verificarBatidas
 * Testa todos os pares de carros (14 carros = 91 pares).
 * Recebe: a corrida e o tempo do quadro.
 * Devolve: nada.
 */
static void verificarBatidas(Corrida *corrida, float tempoQuadro) {
    for (int i = 0; i < TOTAL_CARROS; i++) {
        for (int j = i + 1; j < TOTAL_CARROS; j++) {
            tratarBatida(corrida, corrida->classificacao[i], corrida->classificacao[j], tempoQuadro);
        }
    }
}

/*
 * olharEmVolta
 * Preenche o que cada carro enxerga para a IA decidir: quem está à frente na
 * mesma linha, e se há alguém encostado em cada lado.
 * Recebe: a corrida.
 * Devolve: nada.
 */
static void olharEmVolta(Corrida *corrida) {
    for (int i = 0; i < TOTAL_CARROS; i++) {
        Carro *carro = corrida->classificacao[i];
        carro->distanciaAFrente = -1.0f;
        carro->ocupadoEsquerda = 0;
        carro->ocupadoDireita = 0;
        carro->vemAlguemAtras = 0;
        float meio, meiaLargura;
        meiasMedidasDoCarro(carro, &meio, &meiaLargura);

        for (int j = 0; j < TOTAL_CARROS; j++) {
            Carro *outro = corrida->classificacao[j];
            if (outro == carro) {
                continue;
            }
            float meioOutro, larguraOutro;
            meiasMedidasDoCarro(outro, &meioOutro, &larguraOutro);
            float d = distanciaNaPista(corrida, carro, outro);
            float lado = lateralEmMetros(outro) - lateralEmMetros(carro);
            float vaoNaFrente = fabsf(d) - meio - meioOutro;
            float vaoDeLado = fabsf(lado) - meiaLargura - larguraOutro;

            if (vaoNaFrente < DISTANCIA_AO_LADO && vaoDeLado >= -0.5f && vaoDeLado < DISTANCIA_AO_LADO * 2.0f) {
                if (lado > 0.0f) {
                    carro->ocupadoDireita = 1;
                } else {
                    carro->ocupadoEsquerda = 1;
                }
            }
            if (d < 0.0f && vaoNaFrente < VISAO_PARA_TRAS_NO_PIT && carroNoPit(carro) && carroNoPit(outro) && outro->velocidade > 0.0f && outro->estadoDoPit != PIT_SAINDO) {
                carro->vemAlguemAtras = 1;
            }
            if (d > 0.0f && vaoDeLado < DISTANCIA_AO_LADO && vaoNaFrente < ALCANCE_DA_VISAO
                && (carro->distanciaAFrente < 0.0f || vaoNaFrente < carro->distanciaAFrente)) {
                carro->distanciaAFrente = fmaxf(vaoNaFrente, 0.0f);
                carro->velocidadeAFrente = outro->velocidade;
                carro->lateralAFrente = lateralEmMetros(outro);
            }
        }
    }
}

/*
 * forcaDoVacuo
 * Procura o carro mais próximo logo à frente, na mesma linha. Quanto
 * mais perto, mais forte o vácuo (1 colado, 0 a DISTANCIA_DO_VACUO).
 * Recebe: a corrida e o carro.
 * Devolve: de 0 a 1.
 */
static float forcaDoVacuo(Corrida *corrida, Carro *carro) {
    float forca = 0.0f;
    for (int i = 0; i < TOTAL_CARROS; i++) {
        Carro *outro = corrida->classificacao[i];
        float distancia = outro->distanciaPercorrida - carro->distanciaPercorrida;
        float diferencaLateral = fabsf(lateralEmMetros(outro) - lateralEmMetros(carro));
        if (outro == carro || carroNoPit(outro) || carroNoPit(carro) || distancia <= 0.0f || distancia >= DISTANCIA_DO_VACUO
            || diferencaLateral >= LARGURA_DO_VACUO) {
            continue;
        }
        float destaVez = 1.0f - distancia / DISTANCIA_DO_VACUO;
        if (destaVez > forca) {
            forca = destaVez;
        }
    }
    return forca;
}

/*
 * atualizarVacuo
 * Leva o vácuo de cada carro, aos poucos, para a força de agora. O
 * carro usa esse valor para subir a velocidade máxima (veja carro.c).
 * Recebe: a corrida e o tempo do quadro.
 * Devolve: nada (altera o campo vacuo de cada carro).
 */
static void atualizarVacuo(Corrida *corrida, float tempoQuadro) {
    float passo = RAPIDEZ_DO_VACUO * tempoQuadro;
    for (int i = 0; i < TOTAL_CARROS; i++) {
        Carro *carro = corrida->classificacao[i];
        float alvo = forcaDoVacuo(corrida, carro);
        if (carro->vacuo < alvo) {
            carro->vacuo = fminf(carro->vacuo + passo, alvo);
        } else {
            carro->vacuo = fmaxf(carro->vacuo - passo, alvo);
        }
    }
}

/*
 * moverCarro
 * No pit lane, o piloto automático anda com o carro; na pista, o piloto. Depois vê
 * se ele acabou de entrar no pit lane.
 * Recebe: a corrida, o carro, os comandos e o tempo do quadro.
 * Devolve: 1 se cruzou a linha de chegada.
 */
static int moverCarro(Corrida *corrida, Carro *carro, Comandos comandos, float tempoQuadro) {
    int cruzou;
    if (carroNoPit(carro)) {
        cruzou = atualizarCarroNoPit(carro, corrida->pista, tempoQuadro) > 0;
    } else {
        cruzou = atualizarCarro(carro, comandos, tempoQuadro) > 0;
        if (corrida->temParada && querEntrarNoPit(corrida->pista, carro)) {
            entrarNoPit(carro);
        }
    }
    return cruzou;
}

/*
 * atualizarCorrida
 * Avança a corrida um quadro:
 *   0) mede o vácuo de cada carro;
 *   1) move o jogador com os comandos do teclado;
 *   2) move cada rival com os comandos da IA;
 *   3) trata as batidas;
 *   4) reordena a classificação (Insertion Sort);
 *   5) verifica se o jogador terminou.
 * Antes disso, enquanto a contagem regressiva não acaba, ninguém sai
 * do lugar e o relógio não anda.
 * Depois do fim, em qualquer posição, a IA assume o carro do jogador para uma
 * volta sem fim, e os rivais continuam correndo. O relógio também continua, para cada rival ter
 * o seu tempo de chegada; o do jogador fica guardado no carro dele.
 * Recebe: a corrida, os comandos do jogador e o tempo do quadro.
 * Devolve: nada.
 */
void atualizarCorrida(Corrida *corrida, Comandos comandosJogador, float tempoQuadro) {
    if (corrida->contagemRegressiva > 0.0f) {
        corrida->contagemRegressiva -= tempoQuadro;
        /* Parado no grid: acelerar só sobe o conta-giros (e acende as lanternas) */
        corrida->jogador.ultimosComandos = comandosJogador;
        atualizarMotor(&corrida->jogador.motor, 0.0f, comandosJogador.acelerar && !comandosJogador.frear, 0, 0, tempoQuadro);
        return;
    }

    corrida->tempoDeCorrida += tempoQuadro;
    olharEmVolta(corrida);
    if (corrida->terminou) {
        comandosJogador = atualizarIA(&corrida->jogador);
    }

    /* 0) Vácuo, antes de mover: todos medem com os carros no mesmo lugar */
    atualizarVacuo(corrida, tempoQuadro);

    /* 1) Jogador */
    int cruzou = moverCarro(corrida, &corrida->jogador, comandosJogador, tempoQuadro);
    atualizarSetores(corrida, &corrida->jogador, cruzou);
    if (cruzou) {
        registrarVolta(corrida, &corrida->jogador);
    }

    /* 2) Rivais: a IA olha o carro logo à frente de cada um */
    for (int i = 0; i < TOTAL_RIVAIS; i++) {
        Carro *rival = &corrida->rivais[i];
        Comandos comandosIA = atualizarIA(rival);
        cruzou = moverCarro(corrida, rival, comandosIA, tempoQuadro);
        atualizarSetores(corrida, rival, cruzou);
        if (cruzou) {
            registrarVolta(corrida, rival);
        }
    }

    /* 3) Batidas */
    verificarBatidas(corrida, tempoQuadro);

    /* 4) Classificação */
    ordenarCarrosPorDistancia(corrida->classificacao, TOTAL_CARROS);

    /* 5) Fim de corrida */
    if (!corrida->terminou && corrida->jogador.voltas >= corrida->totalVoltas) {
        corrida->terminou = 1;
        corrida->posicaoFinal = posicaoDoCarro(corrida, &corrida->jogador);
        if (!corrida->jogador.pitFeito) {
            corrida->desclassificado = 1;   /* não fez a parada obrigatória */
            corrida->posicaoFinal = TOTAL_CARROS;
        }
    }
}

/*
 * todosChegaram
 * Recebe: a corrida.
 * Devolve: 1 se todos os carros já completaram todas as voltas, 0 se não.
 */
int todosChegaram(Corrida *corrida) {
    for (int i = 0; i < TOTAL_CARROS; i++) {
        if (corrida->classificacao[i]->voltas < corrida->totalVoltas) {
            return 0;
        }
    }
    return 1;
}

/*
 * voltaAtual
 * Número da volta que o carro está correndo, para mostrar na tela.
 * Recebe: o carro.
 * Devolve: de 1 até todas as voltas.
 */
int voltaAtual(Carro *carro) {
    int volta = carro->voltas + 1;
    if (volta < 1) {
        volta = 1;   /* ainda no grid, antes de cruzar a linha */
    }
    if (volta > carro->voltasDaProva) {
        volta = carro->voltasDaProva;
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

/*
 * jogadorVenceu
 * Recebe: a corrida.
 * Devolve: 1 se o jogador já cruzou a linha final em 1º lugar, 0 se não.
 */
int jogadorVenceu(Corrida *corrida) {
    return corrida->terminou && corrida->posicaoFinal == 1;
}

/*
 * segundosAteAVitoria
 * Quanto falta para o jogador cruzar a linha final, se ele está na última
 * volta e em 1º lugar. É uma estimativa: a distância que falta dividida
 * pela velocidade. Como na reta o carro ainda está ganhando velocidade, a
 * conta usa a média entre a velocidade atual e a máxima (nunca menos que
 * um terço da máxima, para não explodir com o carro quase parado).
 * Serve para o som começar a música da vitória no ponto certo.
 * Recebe: a corrida.
 * Devolve: os segundos que faltam, ou -1 se ele não está nessa situação.
 */
float segundosAteAVitoria(Corrida *corrida) {
    Carro *jogador = &corrida->jogador;
    if (corrida->terminou || jogador->voltas != corrida->totalVoltas - 1 ||
        posicaoDoCarro(corrida, jogador) != 1) {
        return -1.0f;
    }
    float comprimentoDaVolta = corrida->totalSegmentos * COMPRIMENTO_SEGMENTO;
    float falta = corrida->totalVoltas * comprimentoDaVolta - jogador->distanciaPercorrida;
    float velocidade = (jogador->velocidade + jogador->velocidadeMaxima) / 2.0f;
    velocidade = fmaxf(velocidade, jogador->velocidadeMaxima / 3.0f);
    return fmaxf(falta, 0.0f) / velocidade;
}
