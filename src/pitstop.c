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
#define METROS_SAINDO_DA_VAGA 6.0f
#define VELOCIDADE_NA_ENTRADA 5000.0f   /* até passar do funil */
#define FIM_DA_ENTRADA        3730.0f   /* metros: daqui em diante ele já está na reta do pit lane */
#define GUIDO_ENCOSTADO       0.15f     /* metros entre o nariz dele e o carro, trabalhando */
#define GUIDO_ESPERANDO       0.9f      /* metros a mais de folga, esperando */
#define GUIDO_FOLGA_NAS_PONTAS 0.5f     /* metros além do carro, dando a volta nele */
#define METROS_DA_ENTRADA_NA_VAGA 14.0f /* a essa distância da vaga ele encosta: menos que o vão entre as vagas, para não invadir a do lado */
#define VELOCIDADE_ENCOSTANDO 1400.0f   /* devagar nesse trecho, para chegar até o fim da manobra */
#define SEGUNDOS_OLHANDO_ADIANTE 1.2f
#define BORDA_DEPOIS_DA_SAIDA 0.9f      /* larguras da pista: um pouco para dentro do asfalto */
#define SEGUNDOS_VOLTANDO_A_PISTA 0.5f
#define PARADO_NA_VAGA        0.05f     /* metros: chegou */
#define FAIXA_RAPIDA          4.5f      /* metros do muro até o centro do carro, andando no pit lane */
#define RAPIDEZ_DE_LADO       17.0f     /* metros por segundo, no máximo */
#define RESPOSTA_DE_LADO      2.5f      /* por segundo: quanto do caminho até a linha ele anda */
#define METROS_ENTRE_VAGAS    20.0f     /* uma garagem sim, outra não */
#define PRIMEIRA_VAGA         (PRIMEIRO_METRO_DOS_BOXES + 5.0f)   /* o centro da primeira garagem */

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
 * Recebe: o número do carro.
 * Devolve: o lugar da vaga, em metros a partir da linha (negativo: antes dela).
 */
float metroDaVagaDoCarro(int numero) {
    return PRIMEIRA_VAGA + numero * METROS_ENTRE_VAGAS;
}

/*
 * lugarDaVaga
 * Recebe: a pista, o carro e onde guardar a posição e a lateral (em larguras) da vaga dele.
 * Devolve: nada.
 */
void lugarDaVaga(Pista *pista, const Carro *carro, PosicaoNaPista *lugar, float *lateral) {
    lugar->segmento = pista->largada;
    lugar->distanciaNoSegmento = 0.0f;
    moverNaPista(lugar, metroDaVagaDoCarro(carro->numero) * UNIDADES_POR_METRO);
    float largura = lugar->segmento->larguraEsquerda;
    *lateral = -(largura + LARGURA_DO_PIT_LANE - METROS_DA_VAGA_AO_PREDIO) / largura;
}

/*
 * querEntrarNoPit
 * A IA entra quando decidiu parar. O jogador entra pelo caminho: saindo da pista pela
 * esquerda, em cima do asfalto do pit lane.
 * Recebe: a pista e o carro.
 * Devolve: 1 se ele acabou de entrar na zona de entrada do pit lane.
 */
int querEntrarNoPit(const Pista *pista, const Carro *carro) {
    if (!pista->temPit || carroNoPit(carro)) {
        return 0;
    }
    float metros = metrosNaVolta(carro);
    if (metros < ENTRADA_DO_PIT || metros > FIM_DA_ZONA_DE_ENTRADA) {
        return 0;
    }
    if (carro->querParar) {
        return 1;
    }
    float largura = 0.0f;
    float borda = ladoDentroDoPit(metros, pista->metrosDaVolta, &largura);
    float dentro = METROS_PARA_ENTRAR / carro->posicao.segmento->larguraEsquerda;
    return metros >= INICIO_DA_ENTRADA_LIVRE && carro->deslocamentoLateral < -(borda + dentro);
}

/*
 * entrarNoPit
 * Liga o piloto automático: o que acontecia com o carro na pista termina aqui.
 * Recebe: o carro.
 * Devolve: nada.
 */
void entrarNoPit(Carro *carro) {
    carro->estadoDoPit = PIT_CHEGANDO;
    carro->rodando = 0;
    carro->giro = 0.0f;
    carro->velocidadeDoGiro = 0.0f;
    carro->nitroLigado = 0;
    carro->derrapagem = 0.0f;
    carro->forcaDoDrift = 0.0f;
    carro->anguloDeFuga = 0.0f;
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
 * atualizarCarroNoPit
 * Um quadro do piloto automático: freia até o limite, anda na faixa rápida, encosta
 * na vaga, espera a troca e sai.
 * Recebe: o carro, a pista e o tempo do quadro.
 * Devolve: quantas vezes cruzou a linha de chegada neste quadro.
 */
int atualizarCarroNoPit(Carro *carro, Pista *pista, float tempoQuadro) {
    float metrosDaVolta = pista->metrosDaVolta;
    float metros = metrosNaVolta(carro);
    float noPit = (metros >= ENTRADA_DO_PIT) ? metros - ENTRADA_DO_PIT : metros + metrosDaVolta - ENTRADA_DO_PIT;
    float vaga = metroDaVagaDoCarro(carro->numero) + metrosDaVolta - ENTRADA_DO_PIT;
    float fim = FIM_DO_PIT_LANE + metrosDaVolta - ENTRADA_DO_PIT;
    Comandos comandos = { 0, 0, 0, 0, 0.0f, 0, 0 };
    float distanciaDaVaga = fabsf(vaga - noPit);

    /* Velocidade */
    int dentroDoLimite = (metros >= INICIO_DO_LIMITE_DO_PIT || metros <= FIM_DO_LIMITE_DO_PIT);
    float alvo = dentroDoLimite ? LIMITE_DO_PIT : VELOCIDADE_SEM_LIMITE;
    if (metros >= ENTRADA_DO_PIT && metros < FIM_DA_ENTRADA) {
        alvo = VELOCIDADE_NA_ENTRADA;   /* até passar do funil: dá tempo de sair da frente do muro */
    }
    if (!dentroDoLimite && metros < INICIO_DO_LIMITE_DO_PIT && carro->estadoDoPit == PIT_CHEGANDO) {
        /* Freia a tempo de chegar à linha já no limite */
        float ateALinha = (INICIO_DO_LIMITE_DO_PIT - metros) * UNIDADES_POR_METRO;
        alvo = fminf(alvo, sqrtf(LIMITE_DO_PIT * LIMITE_DO_PIT + 2.0f * FREIO_NO_PIT * ateALinha));
    }
    if (carro->estadoDoPit == PIT_CHEGANDO) {
        float falta = fmaxf(vaga - noPit, 0.0f) * UNIDADES_POR_METRO;
        alvo = fminf(alvo, sqrtf(2.0f * FREIO_NO_PIT * falta));
        if (distanciaDaVaga < METROS_DA_ENTRADA_NA_VAGA) {
            alvo = fminf(alvo, VELOCIDADE_ENCOSTANDO);
        }
        if (vaga - noPit < PARADO_NA_VAGA) {
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
        }
    }

    /* O trânsito: reduz atrás de quem está na mesma faixa, e fica na vaga até poder sair */
    if (carro->estadoDoPit != PIT_TROCANDO && carro->distanciaAFrente >= 0.0f) {
        float folga = fmaxf(carro->distanciaAFrente - FOLGA_NA_FILA, 0.0f) * UNIDADES_POR_METRO;
        alvo = fminf(alvo, sqrtf(carro->velocidadeAFrente * carro->velocidadeAFrente + 2.0f * FREIO_NO_PIT * folga));
    }
    if (carro->estadoDoPit == PIT_SAINDO && distanciaDaVaga < METROS_SAINDO_DA_VAGA && carro->vemAlguemAtras) {
        alvo = 0.0f;
    }

    int freando = carro->velocidade > alvo;
    comandos.acelerar = carro->velocidade < alvo;
    comandos.frear = freando;
    carro->velocidade = aproximarDe(carro->velocidade, alvo,
                                    (freando ? FREIO_NO_PIT : ACELERACAO_NO_PIT) * tempoQuadro);

    /* Lado: na faixa rápida, perto do muro; encostando na frente da própria vaga */
    float largura = 0.0f;
    /* Ele olha adiante para já estar longe do muro quando o pit lane se afasta da pista;
     * mas só encosta nela quando o pit lane encosta, para não cortar caminho pela grama.
     * Só nos últimos metros, voltando à pista, ele antecipa um pouco. */
    float metrosPorSegundoAgora = carro->velocidade / UNIDADES_POR_METRO;
    float borda = ladoDentroDoPit(metros, metrosDaVolta, &largura);
    if (carro->estadoDoPit == PIT_SAINDO && borda == 0.0f) {
        borda = BORDA_DEPOIS_DA_SAIDA;   /* passou do fim do pit lane: termina de entrar na pista */
    }
    float noFim = INICIO_DA_VOLTA;
    if (carro->estadoDoPit == PIT_SAINDO && metros < FIM_DO_PIT_LANE && metros > noFim) {
        float metrosAdiante = fminf(metros + metrosPorSegundoAgora * SEGUNDOS_VOLTANDO_A_PISTA, FIM_DO_PIT_LANE);
        borda = ladoDentroDoPit(metrosAdiante, metrosDaVolta, &largura);
    } else {
        float metrosAdiante = fmodf(metros + metrosPorSegundoAgora * SEGUNDOS_OLHANDO_ADIANTE, metrosDaVolta);
        float larguraAdiante = 0.0f;
        float bordaAdiante = ladoDentroDoPit(metrosAdiante, metrosDaVolta, &larguraAdiante);
        if (bordaAdiante > borda) {
            borda = bordaAdiante;
        }
    }
    float larguraDaPista = carro->posicao.segmento->larguraEsquerda;
    float naFaixa = fminf(FAIXA_RAPIDA, largura / 2.0f);
    float linha = -(borda * larguraDaPista + naFaixa);
    float naVaga = -(larguraDaPista + LARGURA_DO_PIT_LANE - METROS_DA_VAGA_AO_PREDIO);
    float atePerto = fabsf(vaga - noPit);
    if (atePerto < METROS_DA_ENTRADA_NA_VAGA) {
        float encostar = 1.0f - atePerto / METROS_DA_ENTRADA_NA_VAGA;
        encostar = encostar * encostar * (3.0f - 2.0f * encostar);
        linha += (naVaga - linha) * encostar;
    }
    float agora = carro->deslocamentoLateral * larguraDaPista;
    float rapidez = 0.0f;
    if (carro->velocidade > 0.0f) {
        rapidez = fminf(fmaxf((linha - agora) * RESPOSTA_DE_LADO, -RAPIDEZ_DE_LADO), RAPIDEZ_DE_LADO);
    }
    /* Saindo, só muda de faixa com o lado livre */
    if (carro->estadoDoPit == PIT_SAINDO && ((rapidez > 0.0f && carro->ocupadoDireita) || (rapidez < 0.0f && carro->ocupadoEsquerda))) {
        rapidez = 0.0f;
    }
    agora += rapidez * tempoQuadro;
    carro->deslocamentoLateral = agora / larguraDaPista;
    carro->velocidadeDeLado = rapidez;

    /* O nariz aponta para onde ele está indo */
    float metrosPorSegundo = carro->velocidade / UNIDADES_POR_METRO;
    carro->volante = (metrosPorSegundo > 1.0f)
        ? fminf(fmaxf(atan2f(rapidez, metrosPorSegundo) * 57.2958f / GRAUS_DO_VOLANTE, -1.0f), 1.0f) : 0.0f;

    carro->ultimosComandos = comandos;
    atualizarMotor(&carro->motor, carro->velocidade / carro->velocidadeMaxima, comandos.acelerar, comandos.frear, 0, tempoQuadro);
    carro->anguloDasRodas = fmodf(carro->anguloDasRodas + rapidezDasRodas(carro->velocidade, 0) * tempoQuadro,
                                  2.0f * 3.14159265f);

    /* Anda */
    float distancia = carro->velocidade * tempoQuadro;
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
    carro->voltas += voltasGanhas;

    /* Chegou ao fim do pit lane: de volta à pista, e o piloto volta a mandar */
    if (carro->estadoDoPit == PIT_SAINDO && noPit >= fim && carro->deslocamentoLateral >= -1.0f) {
        carro->estadoDoPit = PIT_FORA;
        carro->volante = 0.0f;
        carro->velocidadeDeLado = 0.0f;
        carro->linhaAlvo = carro->deslocamentoLateral * carro->posicao.segmento->larguraEsquerda;
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
