/*
 * pitstop.c
 * ------------------------------------------------------------------
 * O caminho pelo pit lane e o tempo parado na vaga. Veja pitstop.h.
 * ------------------------------------------------------------------
 */

#include <math.h>

#include "pitstop.h"

#define FREIO_NO_PIT          6000.0f   /* o que a velocidade cai por segundo, entrando */
#define ACELERACAO_NO_PIT     2600.0f
#define VELOCIDADE_SEM_LIMITE 9000.0f   /* longe das garagens, no pit lane */
#define FOLGA_NA_FILA         2.5f      /* metros até o carro da frente */
#define FOLGA_SAINDO_DA_VAGA  0.7f      /* metros até o carro parado na vaga da frente */
#define METROS_SAINDO_DA_VAGA 6.0f
#define LONGE_DA_VAGA         10.0f  /* metros: daqui em diante, de volta à pista, o piloto assume */
#define FIM_DO_ENCOSTAR       2.0f   /* metros antes da vaga em que o carro já está no lado dela */
#define ANGULO_ENCOSTANDO     35.0f  /* graus: o bico entra na vaga com até tanto de ângulo */
#define ENDIREITANDO          0.35f  /* segundos: o bico volta a alinhar com o pit lane */
#define SEGUINDO_O_CAMINHO    0.12f  /* segundos: longe da vaga, o bico segue o caminho do pit lane */
#define GIRO_MAXIMO_NO_PIT    90.0f  /* graus por segundo que o bico gira, no máximo, andando no pit lane */
#define MENOR_FATOR_NO_PIT    0.1f   /* a saída passa bem por dentro do S do Senna: lá um metro da linha vale uns 25 cm */
#define ABRINDO_NA_FILA       0.6f   /* colado no da frente, anda a esta fração da velocidade dele */
#define PERTO_DO_CAMINHO      0.15f  /* metros: daqui para dentro o carro anda exatamente pelo caminho */
#define ACELERACAO_DE_LADO_NO_PIT 25.0f /* m/s²: nas curvas do caminho, ele anda no máximo tão rápido que isto */
#define FREADA_ANTES_DA_CURVA 8.0f   /* m/s²: ele chega na curva freando com calma, sem tranco */
#define OLHAR_A_CURVA         12.0f  /* metros do caminho entre os pontos que medem a curva à frente */
#define MUDANCA_DO_VOLANTE    5.0f   /* por segundo: o volante não pula de uma vez */
#define GUIDO_ENCOSTADO       0.15f     /* metros entre o nariz dele e o carro, trabalhando */
#define GUIDO_ESPERANDO       0.9f      /* metros a mais de folga, esperando */
#define GUIDO_FOLGA_NAS_PONTAS 0.5f     /* metros além do carro, dando a volta nele */
#define VELOCIDADE_ENCOSTANDO 1400.0f   /* devagar nesse trecho, para chegar até o fim da manobra */
#define PARADO_NA_VAGA        0.05f     /* metros: chegou */
#define PASSOU_DA_VAGA        3.0f      /* metros: entrou rápido demais e passou da vaga. Sem dar ré, segue
                                         * direto pelo pit lane sem trocar (e deve a parada ainda); parado
                                         * ali, travaria a fila de todo mundo atrás */
#define RAPIDEZ_DE_LADO       17.0f     /* metros por segundo, no máximo */
#define RAPIDEZ_NO_CAMINHO    24.0f     /* onde a estrada do pit dobra de verdade */
#define RESPOSTA_DE_LADO      2.5f      /* por segundo: quanto do caminho até a linha ele anda */

/*
 * carroNoPit
 * Recebe: o carro.
 * Devolve: 1 se ele está no pit lane, com o piloto automático ligado.
 */
int carroNoPit(const Carro *carro) {
    return carro->estadoDoPit != PIT_FORA;
}

/*
 * metrosNaVolta
 * Recebe: o carro.
 * Devolve: quantos metros ele está depois da linha de chegada.
 */
float metrosNaVolta(const Carro *carro) {
    return (carro->posicao.segmento->indice * COMPRIMENTO_SEGMENTO + carro->posicao.distanciaNoSegmento) / UNIDADES_POR_METRO;
}

/*
 * metroDaVagaDoCarro
 * Cada carro tem a sua garagem, todas antes da linha de chegada: assim quem para
 * na última volta ainda cruza a linha depois da troca.
 * Recebe: a pista e o número do carro.
 * Devolve: o lugar da vaga, em metros a partir da linha (negativo: antes dela).
 */
float metroDaVagaDoCarro(const Pista *pista, int numero) {
    return pista->pit.primeiraVaga + numero * pista->pit.entreVagas;
}

/*
 * dentroDoLimiteDoPit
 * Recebe: a pista e um lugar nela, em metros.
 * Devolve: 1 se ele está entre as duas linhas do limite de velocidade.
 */
int dentroDoLimiteDoPit(const Pista *pista, float metros) {
    float aqui = metrosDesdeAEntrada(pista, metros);
    return aqui >= metrosDesdeAEntrada(pista, pista->pit.inicioDoLimite)
           && aqui <= metrosDesdeAEntrada(pista, pista->pit.fimDoLimite);
}

/*
 * metrosAteAVaga
 * Recebe: a pista e o segmento.
 * Devolve: quantos metros a vaga fica da linha da pista, para o lado do pit lane.
 */
static float metrosAteAVaga(const Pista *pista, const Segmento *segmento) {
    (void) segmento;
    return pista->pit.vaga;
}

/*
 * larguraDoLadoDoPit
 * Recebe: a pista e o segmento.
 * Devolve: a largura da pista para o lado do pit lane: é a medida da lateral de quem anda nele.
 */
static float larguraDoLadoDoPit(const Pista *pista, const Segmento *segmento) {
    return (pista->pit.lado < 0) ? segmento->larguraEsquerda : segmento->larguraDireita;
}

/*
 * lugarDaVaga
 * Recebe: a pista, o carro e onde guardar a posição e a lateral (em larguras) da vaga dele.
 * Devolve: nada.
 */
void lugarDaVaga(Pista *pista, const Carro *carro, PosicaoNaPista *lugar, float *lateral) {
    lugar->segmento = pista->largada;
    lugar->distanciaNoSegmento = 0.0f;
    moverNaPista(lugar, metroDaVagaDoCarro(pista, carro->numero) * UNIDADES_POR_METRO);
    *lateral = pista->pit.lado * metrosAteAVaga(pista, lugar->segmento) / larguraDoLadoDoPit(pista, lugar->segmento);
}

/*
 * querEntrarNoPit
 * A IA entra quando decidiu parar. O jogador entra pelo caminho: saindo da pista pela
 * esquerda, em cima do asfalto do pit lane.
 * Recebe: a pista e o carro.
 * Devolve: 1 se ele acabou de entrar na zona de entrada do pit lane.
 */
#define FRACAO_PARA_ENTRAR 0.6f   /* da largura do lado do pit, onde ela é menor que paraEntrar */
#define PIT_LANE_ABERTO    4.5f   /* metros além da borda da pista em que o limite do lado do pit ainda deixa andar */
#define FORA_DA_PISTA      0.5f   /* metros: o meio do carro já passou da borda da pista, para o lado do pit */
#define DENTRO_DO_PIT_LANE 4.0f   /* metros além da borda: em cima do pit lane, sem dúvida (quem já parou) */

/*
 * foraDaPistaNoPitLane
 * Depois do funil da entrada, o asfalto do pit lane continua aberto ao lado da pista até as
 * faixas o separarem. Quem passou do funil e está ali, fora da pista, já está no pit lane: se
 * seguisse pilotando, quando o pit lane se fecha o limite da pista o jogaria de volta nela, como
 * se tivesse batido no muro. Então entra também.
 * Recebe: a pista, o carro e quantos metros ele está depois da entrada.
 * Devolve: 1 se ele está fora da pista, em cima do pit lane ainda aberto.
 */
static int foraDaPistaNoPitLane(const Pista *pista, const Carro *carro, float depoisDaEntrada) {
    const Segmento *aqui = carro->posicao.segmento;
    if (aqui->limiteDireita <= 0.0f || depoisDaEntrada >= metrosDesdeAEntrada(pista, pista->pit.inicioDoLimite)) {
        return 0;   /* pista sem limites medidos, ou já perto das garagens */
    }
    float largura = (pista->pit.lado < 0) ? aqui->larguraEsquerda : aqui->larguraDireita;
    float limite = (pista->pit.lado < 0) ? aqui->limiteEsquerda : aqui->limiteDireita;
    if (limite < largura + PIT_LANE_ABERTO) {
        return 0;   /* aqui o pit lane já está separado da pista */
    }
    float paraOLado = pista->pit.lado * carro->deslocamentoLateral * largura;
    return paraOLado > largura + (carro->pitFeito ? DENTRO_DO_PIT_LANE : FORA_DA_PISTA);
}

int querEntrarNoPit(const Pista *pista, const Carro *carro) {
    if (!pista->temPit || carroNoPit(carro)) {
        return 0;
    }
    float metros = metrosNaVolta(carro);
    float depoisDaEntrada = metrosDesdeAEntrada(pista, metros);
    if (depoisDaEntrada > metrosDesdeAEntrada(pista, pista->pit.fimDaZona)) {
        return foraDaPistaNoPitLane(pista, carro, depoisDaEntrada);
    }
    if (carro->querParar) {
        return 1;
    }
    /* O asfalto do pit lane emenda no da pista: só entra quem ainda deve a parada */
    if (carro->pitFeito) {
        return 0;
    }
    if (depoisDaEntrada < metrosDesdeAEntrada(pista, pista->pit.inicioLivre)) {
        return 0;
    }
    /* Onde a pista se afunila na direção do pit lane, basta estar encostado no lado dele */
    float largura = larguraDoLadoDoPit(pista, carro->posicao.segmento);
    float precisa = pista->pit.soEncostadoNaBorda ? largura - pista->pit.paraEntrar
                                                  : fminf(pista->pit.paraEntrar, largura * FRACAO_PARA_ENTRAR);
    return pista->pit.lado * carro->deslocamentoLateral * largura >= precisa;
}

/*
 * entrarNoPit
 * Liga o piloto automático: o que acontecia com o carro na pista termina aqui.
 * Recebe: o carro.
 * Devolve: nada.
 */
void entrarNoPit(Carro *carro) {
    /* O bico continua para onde estava: o pit lane endireita ele aos poucos */
    float rumoDaPista = pontoNoMundo(carro->posicao, carro->deslocamentoLateral).rumo;
    float desenhado = (carro->direcaoLivre && carro->temRumoNoMundo) ? carro->rumoNoMundo - rumoDaPista
                    : carro->anguloDeFuga + carro->anguloDeDrift + carro->giro + carro->volante * GRAUS_DO_VOLANTE;
    desenhado -= 360.0f * roundf(desenhado / 360.0f);
    carro->estadoDoPit = PIT_CHEGANDO;
    carro->rodando = 0;
    carro->giro = 0.0f;
    carro->velocidadeDoGiro = 0.0f;
    carro->nitroLigado = 0;
    carro->derrapagem = 0.0f;
    carro->forcaDoDrift = 0.0f;
    carro->anguloDeFuga = desenhado;
    carro->volante = 0.0f;
    carro->desvioDoRumo = 0.0f;
    carro->temRumoNoMundo = 0;
    carro->anguloDeDrift = 0.0f;
    carro->giroDoDrift = 0.0f;
    carro->velocidadeLateral = 0.0f;
    carro->empurraoPendente = 0.0f;
}

/*
 * aproximarDe
 * Recebe: o valor, o alvo e o passo máximo.
 * Devolve: o valor, um passo mais perto do alvo.
 */
static float aproximarDe(float valor, float alvo, float passo) {
    if (valor < alvo) {
        return fminf(valor + passo, alvo);
    }
    return fmaxf(valor - passo, alvo);
}

/*
 * ladoNoCaminho
 * Recebe: a pista e quantos metros o carro está depois da entrada do pit lane.
 * Devolve: a que distância da linha da pista a estrada do pit passa nesse lugar.
 */
static float ladoNoCaminho(const Pista *pista, float noPit) {
    const DadosDoPit *pit = &pista->pit;
    float inicio = pit->caminho[0][0];
    float anterior = pit->caminho[0][1];
    float lugarAnterior = 0.0f;
    for (int i = 1; i < pit->pontosDoCaminho; i++) {
        float lugar = pit->caminho[i][0] - inicio;
        if (noPit < lugar) {
            float t = fmaxf(noPit - lugarAnterior, 0.0f) / (lugar - lugarAnterior);
            return anterior + (pit->caminho[i][1] - anterior) * t;
        }
        anterior = pit->caminho[i][1];
        lugarAnterior = lugar;
    }
    return anterior;
}

/*
 * ladoNoCaminhoEm
 * Recebe: a pista e quantos metros depois da entrada do pit lane.
 * Devolve: a lateral (em metros, com o sinal do lado do pit) da estrada do pit ali.
 */
static float ladoNoCaminhoEm(const Pista *pista, float noPit) {
    return pista->pit.lado * ladoNoCaminho(pista, noPit);
}

/*
 * suaveNoPit
 * Recebe: um andamento (fora de 0 a 1, fica na ponta).
 * Devolve: o andamento, começando e terminando devagar.
 */
static float suaveNoPit(float t) {
    t = fminf(fmaxf(t, 0.0f), 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

/*
 * linhaNoPit
 * Por onde o piloto automático anda no pit lane, de lado, onde o carro está agora (sem
 * contar o encostar na vaga).
 * Recebe: a pista, o carro, se ele está saindo e a velocidade dele em metros por segundo.
 * Devolve: o lado, em metros (negativo à esquerda).
 */
float linhaNoPit(const Pista *pista, const Carro *carro, int saindo, float metrosPorSegundoAgora) {
    float noPit = metrosDesdeAEntrada(pista, metrosNaVolta(carro));
    const DadosDoPit *pit = &pista->pit;
    float fim = metrosDesdeAEntrada(pista, pit->fim);
    /* Ele olha adiante o tanto que demora para responder: assim o carro passa mesmo por
     * onde o pit lane passa, e não fica para trás cortando pela grama */
    (void) saindo;
    float adiante = noPit + metrosPorSegundoAgora / RESPOSTA_DE_LADO;
    float linha;
    if (pit->pontosDoCaminho > 0) {
        linha = pit->lado * ladoNoCaminho(pista, adiante);
    } else {
        float chegou = suaveNoPit(adiante / metrosDesdeAEntrada(pista, pit->noCaminhoDaFaixa));
        float deixaAFaixa = metrosDesdeAEntrada(pista, pit->deixaAFaixa);
        float voltou = suaveNoPit((fim - adiante) / (fim - deixaAFaixa));
        linha = pit->lado * (pit->borda + (pit->faixa - pit->borda) * fminf(chegou, voltou));
    }
    return linha;
}



/*
 * pontoDoCaminho
 * Recebe: a pista, o carro (para achar a pista a partir dele) e quantos metros depois da entrada.
 * Devolve: o ponto da estrada do pit ali, no mundo.
 */
static PontoNoMundo pontoDoCaminho(const Pista *pista, const Carro *carro, float noPit) {
    PosicaoNaPista lugar = carro->posicao;
    moverNaPista(&lugar, (noPit - metrosDesdeAEntrada(pista, metrosNaVolta(carro))) * UNIDADES_POR_METRO);
    float lateral = ladoNoCaminhoEm(pista, noPit);
    return pontoNoMundo(lugar, lateral / larguraNaPosicao(lugar, lateral));
}

/*
 * velocidadeNaCurvaDoCaminho
 * Mede no mundo o quanto a estrada do pit vira logo à frente, e a velocidade que faz essa curva
 * com no máximo ACELERACAO_DE_LADO_NO_PIT.
 * Recebe: a pista, o carro e quantos metros ele está depois da entrada.
 * Devolve: a velocidade, em unidades por segundo.
 */
static float velocidadeNaCurvaDoCaminho(const Pista *pista, const Carro *carro, float noPit) {
    float menor = 1e9f;
    for (float adiante = 0.0f; adiante <= 100.0f; adiante += OLHAR_A_CURVA / 2.0f) {
        PontoNoMundo a = pontoDoCaminho(pista, carro, noPit + adiante);
        PontoNoMundo b = pontoDoCaminho(pista, carro, noPit + adiante + OLHAR_A_CURVA);
        PontoNoMundo c = pontoDoCaminho(pista, carro, noPit + adiante + 2.0f * OLHAR_A_CURVA);
        float ab = hypotf(b.x - a.x, b.z - a.z), bc = hypotf(c.x - b.x, c.z - b.z);
        if (ab < 0.5f || bc < 0.5f) {
            continue;
        }
        float rumo1 = atan2f(b.x - a.x, a.z - b.z), rumo2 = atan2f(c.x - b.x, b.z - c.z);
        float vira = fabsf(rumo2 - rumo1);
        vira = fminf(vira, 2.0f * 3.14159265f - vira);
        if (vira < 0.01f) {
            continue;
        }
        float raio = (ab + bc) / 2.0f / vira;
        /* A curva mais adiante pode esperar: dá tempo de frear até ela */
        float metrosPorSegundo = sqrtf(ACELERACAO_DE_LADO_NO_PIT * raio);
        float ateLa = adiante;
        float podeVir = sqrtf(metrosPorSegundo * metrosPorSegundo + 2.0f * FREADA_ANTES_DA_CURVA * ateLa);
        menor = fminf(menor, podeVir);
    }
    return menor * UNIDADES_POR_METRO;
}

/*
 * atualizarCarroNoPit
 * Um quadro do piloto automático: freia até o limite, anda na faixa rápida, encosta
 * na vaga, espera a troca e sai.
 * Recebe: o carro, a pista e o tempo do quadro.
 * Devolve: quantas vezes cruzou a linha de chegada neste quadro.
 */
int atualizarCarroNoPit(Carro *carro, Pista *pista, float tempoQuadro) {
    float metros = metrosNaVolta(carro);
    const DadosDoPit *pit = &pista->pit;
    float noPit = metrosDesdeAEntrada(pista, metros);
    float vaga = metrosDesdeAEntrada(pista, metroDaVagaDoCarro(pista, carro->numero));
    float fim = metrosDesdeAEntrada(pista, pit->fim);
    float linhaDoLimite = metrosDesdeAEntrada(pista, pit->inicioDoLimite);
    Comandos comandos = { 0, 0, 0, 0, 0.0f, 0, 0 };
    float distanciaDaVaga = fabsf(vaga - noPit);

    /* Velocidade */
    int dentroDoLimite = dentroDoLimiteDoPit(pista, metros);
    float alvo = dentroDoLimite ? LIMITE_DO_PIT : VELOCIDADE_SEM_LIMITE;
    if (noPit < metrosDesdeAEntrada(pista, pit->fimDaEntrada)) {
        alvo = pit->velocidadeNaEntrada;   /* até passar do funil: dá tempo de sair da frente do muro */
    }
    if (!dentroDoLimite && noPit < linhaDoLimite && carro->estadoDoPit == PIT_CHEGANDO) {
        /* Freia a tempo de chegar à linha já no limite */
        float ateALinha = (linhaDoLimite - noPit) * UNIDADES_POR_METRO;
        alvo = fminf(alvo, sqrtf(LIMITE_DO_PIT * LIMITE_DO_PIT + 2.0f * FREIO_NO_PIT * ateALinha));
    }
    if (carro->estadoDoPit == PIT_CHEGANDO && noPit - vaga > PASSOU_DA_VAGA) {
        carro->estadoDoPit = PIT_SAINDO;
    }
    if (carro->estadoDoPit == PIT_CHEGANDO) {
        float falta = fmaxf(vaga - noPit, 0.0f) * UNIDADES_POR_METRO;
        alvo = fminf(alvo, sqrtf(2.0f * FREIO_NO_PIT * falta));
        /* O trecho em que ele vai de lado até a vaga (como o de baixo): chega nele já devagar */
        float deLado = fabsf(pit->lado * metrosAteAVaga(pista, carro->posicao.segmento) - linhaNoPit(pista, carro, 0, 0.0f));
        float trecho = fmaxf(pit->encostar, deLado / tanf(ANGULO_ENCOSTANDO * 0.0174533f) + FIM_DO_ENCOSTAR);
        float ateOTrecho = fmaxf(distanciaDaVaga - trecho, 0.0f) * UNIDADES_POR_METRO;
        alvo = fminf(alvo, sqrtf(VELOCIDADE_ENCOSTANDO * VELOCIDADE_ENCOSTANDO + 2.0f * FREIO_NO_PIT * ateOTrecho));
        Segmento *aqui = carro->posicao.segmento;
        float faltaDeLado = fabsf(carro->deslocamentoLateral * larguraDoLadoDoPit(pista, aqui) - pit->lado * metrosAteAVaga(pista, aqui));
        if (vaga - noPit < PARADO_NA_VAGA && faltaDeLado < 0.25f) {
            carro->estadoDoPit = PIT_TROCANDO;
            carro->tempoDaTroca = 0.0f;
            carro->velocidade = 0.0f;
            alvo = 0.0f;
        }
    } else if (carro->estadoDoPit == PIT_TROCANDO) {
        alvo = 0.0f;
        carro->velocidade = 0.0f;
        carro->tempoDaTroca += tempoQuadro;
        if (carro->tempoDaTroca >= carro->duracaoDaTroca) {
            carro->estadoDoPit = PIT_SAINDO;
            carro->pitFeito = 1;
            carro->querParar = 0;
            carro->ultimaTroca = carro->tempoDaTroca;
            carro->desgaste = 0.0f;   /* pneus novos */
            carro->pneuEstourado = 0;
            carro->ladoDoPrimeiroEstouro = 0;
            carro->tempoDoEstouro = 0.0f;
        }
    }

    /* O trânsito: reduz atrás de quem está na mesma faixa, e fica na vaga até poder sair */
    if (carro->estadoDoPit != PIT_TROCANDO && carro->distanciaAFrente >= 0.0f) {
        /* Saindo da vaga, com um carro parado logo à frente: chega perto, porque precisa de
         * espaço para esterçar e passar por ele */
        int saindoPertoDele = carro->estadoDoPit == PIT_SAINDO && carro->velocidadeAFrente <= 0.0f;
        float folga = fmaxf(carro->distanciaAFrente - (saindoPertoDele ? FOLGA_SAINDO_DA_VAGA : FOLGA_NA_FILA), 0.0f) * UNIDADES_POR_METRO;
        alvo = fminf(alvo, sqrtf(carro->velocidadeAFrente * carro->velocidadeAFrente + 2.0f * FREIO_NO_PIT * folga));
        if (folga <= 0.0f && !saindoPertoDele) {
            /* Já colado: a velocidade é a do mundo, e onde a estrada do pit vira em relação à pista
             * o da frente anda menos na pista que ela; fica um pouco mais devagar, para abrir */
            alvo = fminf(alvo, carro->velocidadeAFrente * ABRINDO_NA_FILA);
        }
    }
    if (carro->estadoDoPit == PIT_SAINDO && distanciaDaVaga < METROS_SAINDO_DA_VAGA && carro->vemAlguemAtras) {
        alvo = 0.0f;
    }

    /* Nas curvas do caminho (a saída de Interlagos serpenteia por dentro do S do Senna), ele anda
     * no máximo na velocidade que faz a curva sem tranco */
    if (pit->pontosDoCaminho > 0 && carro->estadoDoPit != PIT_TROCANDO) {
        alvo = fminf(alvo, velocidadeNaCurvaDoCaminho(pista, carro, noPit));
    }

    int freando = carro->velocidade > alvo;
    comandos.acelerar = carro->velocidade < alvo;
    comandos.frear = freando;
    carro->velocidade = aproximarDe(carro->velocidade, alvo,
                                    (freando ? FREIO_NO_PIT : ACELERACAO_NO_PIT) * tempoQuadro);

    /* Lado: na faixa rápida; encostando na frente da própria vaga */
    float metrosPorSegundoAgora = carro->velocidade / UNIDADES_POR_METRO;
    float larguraDaPista = larguraNaPosicao(carro->posicao, pista->pit.lado);
    /* Onde há caminho medido, ele vai até o caminho dali (sem olhar adiante): assim, quando começa a
     * encostar na vaga, a linha não salta do caminho lá na frente para o daqui */
    float linha = linhaNoPit(pista, carro, carro->estadoDoPit == PIT_SAINDO, (pit->pontosDoCaminho > 0) ? 0.0f : metrosPorSegundoAgora);
    float naVaga = pit->lado * metrosAteAVaga(pista, carro->posicao.segmento);
    float atePerto = fabsf(vaga - noPit);
    /* De frente: o lado muda junto com o andar, num ângulo que o bico consegue fazer, e termina
     * um pouco antes da vaga, para o carro chegar nela já reto */
    int chegando = (carro->estadoDoPit == PIT_CHEGANDO);
    float fimDoEncostar = chegando ? FIM_DO_ENCOSTAR : 0.0f;   /* saindo, já começa a sair do lado da vaga */
    float trechoDeEncostar = chegando ? fmaxf(pit->encostar, fabsf(naVaga - linha) / tanf(ANGULO_ENCOSTANDO * 0.0174533f) + FIM_DO_ENCOSTAR)
                                      : pit->encostar;
    if (atePerto < trechoDeEncostar) {
        float encostar = fminf(1.0f - (atePerto - fimDoEncostar) / (trechoDeEncostar - fimDoEncostar), 1.0f);
        encostar = encostar * encostar * (3.0f - 2.0f * encostar);
        linha += (naVaga - linha) * encostar;
    }
    float agora = carro->deslocamentoLateral * larguraDaPista;
    float rapidez = 0.0f;
    /* Preso ao caminho: já em cima dele e longe da vaga, o carro anda exatamente por ele (sem atraso,
     * que faria ele balançar); só para chegar até ele, ou encostar e sair da vaga, é que ele muda de
     * lado aos poucos */
    int presoAoCaminho = pit->pontosDoCaminho > 0 && atePerto >= trechoDeEncostar && carro->estadoDoPit != PIT_TROCANDO
                         && fabsf(linhaNoPit(pista, carro, carro->estadoDoPit == PIT_SAINDO, 0.0f) - agora) < PERTO_DO_CAMINHO;
    if (!presoAoCaminho && (carro->velocidade > 0.0f || (carro->estadoDoPit != PIT_TROCANDO && atePerto < trechoDeEncostar))) {
        float maximo = (pit->pontosDoCaminho > 0) ? RAPIDEZ_NO_CAMINHO : RAPIDEZ_DE_LADO;
        rapidez = fminf(fmaxf((linha - agora) * RESPOSTA_DE_LADO, -maximo), maximo);
        if (carro->estadoDoPit == PIT_CHEGANDO && atePerto < trechoDeEncostar && tempoQuadro > 0.0f) {
            /* Encostando, ele segue a curva até a vaga sem atraso: assim chega nela já de frente */
            rapidez = fminf(fmaxf((linha - agora) / tempoQuadro, -RAPIDEZ_NO_CAMINHO), RAPIDEZ_NO_CAMINHO);
        }
    }
    /* Saindo, só muda de faixa com o lado livre */
    if (carro->estadoDoPit == PIT_SAINDO && ((rapidez > 0.0f && carro->ocupadoDireita) || (rapidez < 0.0f && carro->ocupadoEsquerda))) {
        rapidez = 0.0f;
    }

    carro->ultimosComandos = comandos;
    atualizarMotor(&carro->motor, carro->velocidade / carro->velocidadeMaxima, comandos.acelerar, comandos.frear, 0, tempoQuadro);
    carro->anguloDasRodas = fmodf(carro->anguloDasRodas + rapidezDasRodas(carro->velocidade, 0) * tempoQuadro,
                                  2.0f * 3.14159265f);

    /* Anda. Onde há caminho medido, o passo na linha da pista é o que faz o carro andar no mundo o
     * que a velocidade manda: longe da linha, por dentro de uma curva (a saída de Interlagos, por
     * dentro do S do Senna), um metro da linha vale bem menos que um metro do carro */
    PontoNoMundo antes = pontoNoMundo(carro->posicao, carro->deslocamentoLateral);
    float distancia = carro->velocidade * tempoQuadro;
    if (pit->pontosDoCaminho > 0 && distancia > 0.0f) {
        PosicaoNaPista adiante = carro->posicao;
        moverNaPista(&adiante, UNIDADES_POR_METRO);
        float lateralAdiante = presoAoCaminho ? ladoNoCaminhoEm(pista, noPit + 1.0f) : agora;   /* mudando de lado por conta própria, só a curva conta */
        PontoNoMundo ali = pontoNoMundo(adiante, lateralAdiante / larguraNaPosicao(adiante, lateralAdiante));
        float noMundo = fminf(fmaxf(hypotf(ali.x - antes.x, ali.z - antes.z), MENOR_FATOR_NO_PIT), 1.0f / MENOR_FATOR_NO_PIT);
        distancia /= noMundo;
        /* E confere no passo de verdade: onde a pista muda de jeito no meio do passo (a emenda da
         * volta, na linha de chegada, tem um segmento sem comprimento no mundo), corrige */
        float quer = carro->velocidade * tempoQuadro / UNIDADES_POR_METRO;
        for (int vez = 0; vez < 2; vez++) {
            PosicaoNaPista tentativa = carro->posicao;
            moverNaPista(&tentativa, distancia);
            float lateralLa = presoAoCaminho ? ladoNoCaminhoEm(pista, noPit + distancia / UNIDADES_POR_METRO) : agora;
            PontoNoMundo la = pontoNoMundo(tentativa, lateralLa / larguraNaPosicao(tentativa, lateralLa));
            float andou = hypotf(la.x - antes.x, la.z - antes.z);
            if (andou > 0.001f) {
                distancia *= fminf(fmaxf(quer / andou, 0.5f), 2.0f);
            }
        }
    }
    if (carro->estadoDoPit != PIT_TROCANDO) {
        distancia += carro->empurraoPendente;
    }
    carro->empurraoPendente = 0.0f;
    carro->velocidadeLateral = 0.0f;
    if (carro->posicao.segmento->ehLinhaDeChegada && carro->posicao.distanciaNoSegmento + distancia < 0.0f) {
        distancia = -carro->posicao.distanciaNoSegmento;
    }
    carro->distanciaPercorrida += distancia;
    int voltasGanhas = moverNaPista(&carro->posicao, distancia);
    float larguraDepois = larguraNaPosicao(carro->posicao, pista->pit.lado);
    if (presoAoCaminho) {
        agora = ladoNoCaminhoEm(pista, metrosDesdeAEntrada(pista, metrosNaVolta(carro)));
    } else {
        agora += rapidez * tempoQuadro;
    }
    carro->deslocamentoLateral = agora / larguraDepois;
    carro->velocidadeDeLado = (tempoQuadro > 0.0f) ? rapidez : 0.0f;

    /* O bico aponta para onde ele andou de verdade no mundo, virando aos poucos e sem passar de
     * um giro máximo (perto da vaga, mais devagar ainda, como numa manobra) */
    PontoNoMundo depois = pontoNoMundo(carro->posicao, carro->deslocamentoLateral);
    float andou = hypotf(depois.x - antes.x, depois.z - antes.z);
    if (andou > 0.001f && tempoQuadro > 0.0f) {
        float apontar = atan2f(depois.x - antes.x, antes.z - depois.z) * 57.2958f - depois.rumo;
        apontar -= 360.0f * roundf(apontar / 360.0f);
        float diferenca = apontar - carro->anguloDeFuga;
        diferenca -= 360.0f * roundf(diferenca / 360.0f);
        float resposta = (atePerto < trechoDeEncostar) ? ENDIREITANDO : SEGUINDO_O_CAMINHO;
        float passo = diferenca * (1.0f - expf(-tempoQuadro / resposta));
        float maximo = GIRO_MAXIMO_NO_PIT * tempoQuadro;
        carro->anguloDeFuga += fminf(fmaxf(passo, -maximo), maximo);
        float volanteAlvo = fminf(fmaxf(diferenca / GRAUS_DO_VOLANTE, -1.0f), 1.0f);
        carro->volante = aproximarDe(carro->volante, volanteAlvo, MUDANCA_DO_VOLANTE * tempoQuadro);
    }
    carro->voltas += voltasGanhas;
    atualizarSuspensao(carro, tempoQuadro);

    /* Chegou ao fim do pit lane: de volta à pista, e o piloto volta a mandar */
    /* Ou antes, assim que o pit lane já devolveu o carro à pista (o fim do muro de pit) */
    int devolvido = noPit > vaga + pit->encostar + LONGE_DA_VAGA && fabsf(carro->deslocamentoLateral) <= 1.0f
                    && fabsf(linhaNoPit(pista, carro, 1, 0.0f)) <= larguraDaPista;
    if (carro->estadoDoPit == PIT_SAINDO && (noPit >= fim || devolvido) && fabsf(carro->deslocamentoLateral) <= 1.0f) {
        carro->estadoDoPit = PIT_FORA;
        carro->volante = 0.0f;
        carro->velocidadeDeLado = 0.0f;
        carro->linhaAlvo = carro->deslocamentoLateral * larguraDaPista;
    }
    return voltasGanhas;
}

/*
 * faseDaTroca
 * Onde o Guido está na troca: indo para um canto do carro, ou trocando a roda dele.
 * Recebe: o carro e onde guardar o canto (0 a 3; 4 = voltando para o lugar dele) e o
 *         andamento dessa fase (0 a 1).
 * Devolve: 1 se está trocando a roda, 0 se está indo de uma para a outra.
 */
int faseDaTroca(const Carro *carro, int *canto, float *andamento) {
    float tempo = carro->tempoDaTroca;
    for (int i = 0; i < CANTOS_DO_CARRO; i++) {
        if (tempo < SEGUNDOS_PARA_A_RODA) {
            *canto = i;
            *andamento = tempo / SEGUNDOS_PARA_A_RODA;
            return 0;
        }
        tempo -= SEGUNDOS_PARA_A_RODA;
        if (tempo < SEGUNDOS_POR_RODA) {
            *canto = i;
            *andamento = tempo / SEGUNDOS_POR_RODA;
            return 1;
        }
        tempo -= SEGUNDOS_POR_RODA;
    }
    *canto = CANTOS_DO_CARRO;
    *andamento = fminf(tempo / SEGUNDOS_PARA_A_RODA, 1.0f);
    return 0;
}

/*
 * pontoNoCaminho
 * Recebe: os pontos de um caminho, quantos são e o andamento (0 a 1) pelo comprimento dele.
 * Devolve: o lugar e a direção do trecho onde ele está.
 */
static LugarDoGuido pontoNoCaminho(const float pontos[][2], int total, float andamento) {
    float comprimento = 0.0f;
    for (int i = 0; i + 1 < total; i++) {
        comprimento += hypotf(pontos[i + 1][0] - pontos[i][0], pontos[i + 1][1] - pontos[i][1]);
    }
    float falta = andamento * comprimento;
    LugarDoGuido lugar = { pontos[0][0], pontos[0][1], 0.0f };
    for (int i = 0; i + 1 < total; i++) {
        float dx = pontos[i + 1][0] - pontos[i][0], dz = pontos[i + 1][1] - pontos[i][1];
        float trecho = hypotf(dx, dz);
        if (trecho <= 0.0f) {
            continue;
        }
        float t = fminf(falta / trecho, 1.0f);
        lugar.x = pontos[i][0] + dx * t;
        lugar.z = pontos[i][1] + dz * t;
        lugar.olha = atan2f(dx, dz);
        if (falta <= trecho) {
            break;
        }
        falta -= trecho;
    }
    return lugar;
}

/*
 * lugarDoGuido
 * Trabalhando, ele fica de frente para a roda, com o nariz encostado no carro. Para
 * trocar de lado, ele sai para além da ponta do carro, cruza e volta: nunca por dentro.
 * Recebe: o carro, a meia largura e o meio comprimento dele (metros) e onde ficam as
 *         rodas da frente e de trás.
 * Devolve: o lugar do Guido.
 */
LugarDoGuido lugarDoGuido(const Carro *carro, float meiaLargura, float meioComprimento, float zDaFrente, float zDeTras) {
    float aoLado = meiaLargura + MEIO_COMPRIMENTO_DO_GUIDO + GUIDO_ENCOSTADO;
    float noCanto[CANTOS_DO_CARRO + 1][2] = {
        { aoLado, zDaFrente }, { aoLado, zDeTras }, { -aoLado, zDeTras }, { -aoLado, zDaFrente },
        { aoLado + GUIDO_ESPERANDO, 0.0f }
    };
    if (carro->estadoDoPit != PIT_TROCANDO) {
        LugarDoGuido esperando = { noCanto[CANTOS_DO_CARRO][0], noCanto[CANTOS_DO_CARRO][1], -1.5708f };
        return esperando;
    }
    int canto;
    float andamento;
    if (faseDaTroca(carro, &canto, &andamento)) {
        LugarDoGuido trabalhando = { noCanto[canto][0], noCanto[canto][1], (noCanto[canto][0] > 0.0f) ? -1.5708f : 1.5708f };
        return trabalhando;
    }
    const float *ate = noCanto[canto];
    const float *de = noCanto[(canto + CANTOS_DO_CARRO) % (CANTOS_DO_CARRO + 1)];
    float pontos[4][2] = { { de[0], de[1] }, { ate[0], ate[1] }, { ate[0], ate[1] }, { ate[0], ate[1] } };
    if ((de[0] > 0.0f) != (ate[0] > 0.0f)) {
        float fora = meioComprimento + MEIO_COMPRIMENTO_DO_GUIDO + GUIDO_FOLGA_NAS_PONTAS;
        fora = (de[1] + ate[1] < 0.0f) ? -fora : fora;
        pontos[1][0] = de[0];
        pontos[1][1] = fora;
        pontos[2][0] = ate[0];
        pontos[2][1] = fora;
    }
    return pontoNoCaminho(pontos, 4, andamento * andamento * (3.0f - 2.0f * andamento));
}

/*
 * suaveEntre
 * Recebe: o valor e o trecho (de, até).
 * Devolve: de 0 a 1 ao longo do trecho, começando e terminando devagar.
 */
static float suaveEntre(float a, float de, float ate) {
    float t = fminf(fmaxf((a - de) / (ate - de), 0.0f), 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

/*
 * entradaDoGuidoNoCarro
 * O quanto o retângulo do Guido entra no do carro, vistos de cima (eixos separados).
 * Recebe: o Guido (no carro: x para a esquerda, z para a frente) e as metades do carro.
 * Devolve: a menor entrada em metros (0 se não se tocam).
 */
float entradaDoGuidoNoCarro(LugarDoGuido guido, float meiaLargura, float meioComprimento) {
    float c = cosf(guido.olha), s = sinf(guido.olha);
    /* os eixos: os do carro (x, z) e os do Guido (frente e lado) */
    float eixos[4][2] = { { 1.0f, 0.0f }, { 0.0f, 1.0f }, { s, c }, { c, -s } };
    float menor = 1e9f;
    for (int i = 0; i < 4; i++) {
        float ex = eixos[i][0], ez = eixos[i][1];
        float doCarro = fabsf(ex) * meiaLargura + fabsf(ez) * meioComprimento;
        float doGuido = fabsf(ex * s + ez * c) * MEIO_COMPRIMENTO_DO_GUIDO + fabsf(ex * c - ez * s) * MEIA_LARGURA_DO_GUIDO;
        float entre = fabsf(guido.x * ex + guido.z * ez);
        float entrada = doCarro + doGuido - entre;
        if (entrada <= 0.0f) {
            return 0.0f;
        }
        menor = fminf(menor, entrada);
    }
    return menor;
}

/*
 * poseDoGuido
 * Onde o Guido está e para onde olha, em todo o pit stop: o caminho de canto a canto
 * (lugarDoGuido) e, em cada roda, um puxão para arrancar o pneu velho e a volta para encaixar
 * o novo, que cai do alto. Se alguma pose encostar no carro, ele é empurrado para fora:
 * nunca entra no carro.
 * Recebe: o carro e as medidas dele (as do modelo 3D) e o z das rodas da frente e de trás.
 * Devolve: o lugar do Guido no carro.
 */
LugarDoGuido poseDoGuido(const Carro *carro, float meiaLargura, float meioComprimento, float zDaFrente, float zDeTras) {
    LugarDoGuido lugar = lugarDoGuido(carro, meiaLargura, meioComprimento, zDaFrente, zDeTras);
    int canto;
    float a;
    if (carro->estadoDoPit == PIT_TROCANDO && faseDaTroca(carro, &canto, &a)) {
        float lado = (lugar.x > 0.0f) ? 1.0f : -1.0f;
        /* arranca o velho recuando um pouco, e volta encaixando o novo, que cai do alto */
        float recuo = PUXADA * (suaveEntre(a, 0.0f, ARRANCOU) - suaveEntre(a, ARRANCOU, ENCAIXOU));
        lugar.x += lado * recuo;
    }

    /* A garantia: encostou no carro, sai para o lado dele, até não tocar */
    float lado = (lugar.x >= 0.0f) ? 1.0f : -1.0f;
    for (int i = 0; i < 40 && entradaDoGuidoNoCarro(lugar, meiaLargura, meioComprimento) > 0.0f; i++) {
        lugar.x += lado * 0.05f;
    }
    return lugar;
}
