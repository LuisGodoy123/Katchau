/*
 * online.c
 * ------------------------------------------------------------------
 * A corrida em rede. Veja online.h.
 * ------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "online.h"
#include "sala.h"
#include "motor.h"
#include "ordenacao.h"   /* a classificação é reordenada com as distâncias que o dono mandou */

/* ---------- Ritmo ---------- */
#define RETRATOS_POR_SEGUNDO    30
#define SEGUNDOS_ENTRE_RETRATOS (1.0f / RETRATOS_POR_SEGUNDO)

/* ---------- Suavizar os carros dos outros ----------
 * O retrato chega 30 vezes por segundo e o jogo desenha 60 (ou 144): sem suavizar, o
 * carro do outro andaria aos pulos. Entre dois retratos ele segue andando com a última
 * velocidade, e o erro que o retrato novo revela é "sangrado" em SEGUNDOS_PARA_ALINHAR
 * em vez de aplicado de uma vez. */
#define SEGUNDOS_PARA_ALINHAR   0.10f

/* ---------- Corrigir o próprio carro ----------
 * Aqui a mão do jogador está no volante, e corrigir é mais delicado: abaixo da folga
 * não se corrige NADA (o jogador sentiria a correção mais que o erro); acima do limite
 * não se corrige, se PULA (bateu, rodou, levou empurrão — o que o dono viu é o que
 * aconteceu de verdade, e insistir na previsão seria mentir para o jogador). */
#define FOLGA_SEM_CORRIGIR      (0.5f * UNIDADES_POR_METRO)
#define LIMITE_PARA_PULAR       (8.0f * UNIDADES_POR_METRO)
#define SEGUNDOS_PARA_ME_ALINHAR 0.25f

/* ---------- Tamanhos ---------- */
#define CORPO_DO_QUADRO    (16 + TOTAL_CARROS * BYTES_DO_CARRO_NO_FIO)
#define CORPO_DOS_COMANDOS (18)

/* ==================================================================
 * O estado
 * ================================================================== */

/* A pista indexada: veja o requisito (1) em online.h */
static Segmento **tabelaDeSegmentos = NULL;
static int totalDeSegmentos = 0;
static float metrosDaVoltaEmUnidades = 0.0f;

/* O que o convidado lembra de cada carro, para ele não tremer na tela */
typedef struct {
    float naLinhaMostrada;      /* onde o carro está sendo desenhado (unidades desde a linha) */
    float velocidadeMostrada;
    float erroAoLongo;          /* o que falta corrigir para chegar ao que o dono disse */
    float erroDeLado;
    int tem;
} CarroSuavizado;

static CarroSuavizado suave[TOTAL_CARROS];
static float desdeORetrato = 0.0f;
static float desdeQueMandei = 0.0f;

/* As sequências: o UDP não promete ordem, e aplicar um retrato velho faria o carro
 * andar para trás. Cada lado tem o seu contador. */
static unsigned short ultimaDoRetrato = 0;
static int jaVeioRetrato = 0;
static unsigned short ultimaDoComando[MAXIMO_DE_JOGADORES];
static int jaVeioComando[MAXIMO_DE_JOGADORES];

/* O convidado guarda os últimos comandos para mandar vários por pacote */
static Comandos historico[QUADROS_DE_COMANDO];
static unsigned int meuQuadro = 0;

/* ==================================================================
 * A pista como um número só
 * ================================================================== */

/*
 * montarTabelaDeSegmentos
 * Uma volta na lista circular da pista, guardando o ponteiro de cada segmento no
 * índice dele. Depois disso, achar o segmento número N é uma indexação, e não um
 * passeio pela lista.
 * Recebe: a pista.
 * Devolve: 1 se montou.
 */
static int montarTabelaDeSegmentos(Pista *pista) {
    free(tabelaDeSegmentos);
    tabelaDeSegmentos = NULL;
    totalDeSegmentos = contarSegmentos(pista);
    if (totalDeSegmentos <= 0) {
        return 0;
    }
    tabelaDeSegmentos = (Segmento **) calloc((size_t) totalDeSegmentos, sizeof(Segmento *));
    if (tabelaDeSegmentos == NULL) {
        totalDeSegmentos = 0;
        return 0;
    }
    Segmento *atual = pista->largada;
    do {
        if (atual->indice >= 0 && atual->indice < totalDeSegmentos) {
            tabelaDeSegmentos[atual->indice] = atual;
        }
        atual = atual->proximo;
    } while (atual != pista->largada);
    metrosDaVoltaEmUnidades = totalDeSegmentos * COMPRIMENTO_SEGMENTO;
    return 1;
}

/*
 * metroNaLinha
 * A posição do carro ao longo da pista como UM número: o índice do segmento vezes o
 * comprimento dele, mais o quanto já andou dentro. Com isso, interpolar duas posições
 * é uma subtração, e não um passeio pela lista circular.
 * Recebe: o índice do segmento e a distância dentro dele.
 * Devolve: as unidades de mundo desde a linha de chegada.
 */
static float metroNaLinha(int indice, float dentro) {
    return indice * COMPRIMENTO_SEGMENTO + dentro;
}

/*
 * encurtarNaVolta
 * Traz uma diferença de posição para a faixa de meia volta. Sem isto, dois carros nos
 * dois lados da linha de chegada pareceriam estar a uma volta inteira de distância, e a
 * suavização daria um pulo gigante a cada passagem pela linha.
 * Recebe: a diferença em unidades.
 * Devolve: a diferença mais curta, entre -meia volta e +meia volta.
 */
static float encurtarNaVolta(float diferenca) {
    if (metrosDaVoltaEmUnidades <= 0.0f) {
        return diferenca;
    }
    float meia = metrosDaVoltaEmUnidades / 2.0f;
    while (diferenca > meia) {
        diferenca -= metrosDaVoltaEmUnidades;
    }
    while (diferenca < -meia) {
        diferenca += metrosDaVoltaEmUnidades;
    }
    return diferenca;
}

/*
 * voltarParaAPista
 * O caminho de volta: do número só para o nó da lista mais a sobra, dando a volta
 * quando passa do fim (a pista é circular).
 *
 * NÃO conta voltas, de propósito: quem conta é o dono, e o número dele vem no retrato.
 * Recebe: as unidades desde a linha e a posição a preencher.
 * Devolve: nada.
 */
static void voltarParaAPista(float naLinha, PosicaoNaPista *posicao) {
    if (tabelaDeSegmentos == NULL || totalDeSegmentos <= 0) {
        return;
    }
    naLinha = fmodf(naLinha, metrosDaVoltaEmUnidades);
    if (naLinha < 0.0f) {
        naLinha += metrosDaVoltaEmUnidades;
    }
    int indice = (int) (naLinha / COMPRIMENTO_SEGMENTO);
    if (indice < 0) {
        indice = 0;
    }
    if (indice >= totalDeSegmentos) {
        indice = totalDeSegmentos - 1;
    }
    if (tabelaDeSegmentos[indice] == NULL) {
        return;
    }
    posicao->segmento = tabelaDeSegmentos[indice];
    posicao->distanciaNoSegmento = naLinha - indice * COMPRIMENTO_SEGMENTO;
}

/*
 * carroPorNumero
 * O retrato manda os carros na ordem do NÚMERO deles (0 a 13), que é igual em todas as
 * máquinas e não muda de quadro a quadro — ao contrário da classificação, que muda a
 * cada ultrapassagem.
 * Recebe: a corrida e o número do carro.
 * Devolve: o carro, ou NULL se esse número não está nesta corrida.
 */
static Carro *carroPorNumero(Corrida *corrida, int numero) {
    if (corrida->jogador.numero == numero) {
        return &corrida->jogador;
    }
    for (int i = 0; i < TOTAL_RIVAIS; i++) {
        if (corrida->rivais[i].numero == numero) {
            return &corrida->rivais[i];
        }
    }
    return NULL;
}

void prepararOnline(Corrida *corrida, Pista *pista) {
    montarTabelaDeSegmentos(pista);
    memset(suave, 0, sizeof(suave));
    desdeORetrato = 0.0f;
    desdeQueMandei = 0.0f;
    jaVeioRetrato = 0;
    ultimaDoRetrato = 0;
    meuQuadro = 0;
    for (int i = 0; i < MAXIMO_DE_JOGADORES; i++) {
        jaVeioComando[i] = 0;
        ultimaDoComando[i] = 0;
    }
    for (int i = 0; i < QUADROS_DE_COMANDO; i++) {
        historico[i] = (Comandos) { 0, 0, 0, 0, 0.0f, 0, 0 };
    }
    (void) corrida;
}

void encerrarOnline(void) {
    free(tabelaDeSegmentos);
    tabelaDeSegmentos = NULL;
    totalDeSegmentos = 0;
}

float segundosSemRetrato(void) {
    return desdeORetrato;
}

/* ==================================================================
 * O dono manda o retrato
 * ================================================================== */

/*
 * mandarRetrato
 * Os 14 carros no fio, 30 vezes por segundo, para cada convidado.
 *
 * Vai o ESTADO, não as mudanças: perder um pacote custa 33 ms, e o seguinte conta a
 * mesma história inteira. É por isso que não existe retransmissão nenhuma aqui — num
 * jogo de corrida, um pacote de posição atrasado não vale nada, e reentregá-lo (como o
 * TCP faria) só atrasaria o próximo, que é o que importa.
 * Recebe: a corrida.
 * Devolve: nada.
 */
static void mandarRetrato(Corrida *corrida) {
    unsigned char fio[BYTES_DO_CABECALHO + CORPO_DO_QUADRO];
    int onde = escreverCabecalho(fio, PACOTE_QUADRO, proximaSequencia());
    escreverU32(fio, &onde, meuQuadro++);
    escreverF32(fio, &onde, corrida->tempoDeCorrida);
    escreverF32(fio, &onde, corrida->contagemRegressiva);
    escreverU8(fio, &onde, (unsigned int) (corrida->terminou != 0));
    escreverU8(fio, &onde, TOTAL_CARROS);
    escreverU16(fio, &onde, 0);
    for (int numero = 0; numero < TOTAL_CARROS; numero++) {
        Carro *carro = carroPorNumero(corrida, numero);
        CarroNoFio noFio;
        memset(&noFio, 0, sizeof(noFio));
        if (carro != NULL) {
            guardarCarroNoFio(carro, &noFio);
        }
        escreverCarroNoFio(fio, &onde, &noFio);
    }
    for (int vaga = 0; vaga < MAXIMO_DE_JOGADORES; vaga++) {
        EnderecoDaRede destino;
        if (enderecoDoConvidado(vaga, &destino)) {
            enviarPara(soqueteDaSala(), &destino, fio, onde);
        }
    }
}

/* ==================================================================
 * O convidado manda os comandos
 * ================================================================== */

/*
 * mandarComandos
 * O que o jogador está fazendo, um pacote por quadro.
 *
 * Vão os QUADROS_DE_COMANDO últimos quadros, não só o atual: um datagrama perdido
 * tiraria o pé do jogador do acelerador por um quadro inteiro, e repetir os últimos
 * custa 2 bytes cada. Como o dono só usa o mais novo que ainda não viu, repetir não
 * faz nada andar duas vezes.
 * Recebe: a corrida (para a vaga) e os comandos deste quadro.
 * Devolve: nada.
 */
static void mandarComandos(Corrida *corrida, Comandos meus) {
    for (int i = QUADROS_DE_COMANDO - 1; i > 0; i--) {
        historico[i] = historico[i - 1];
    }
    historico[0] = meus;

    unsigned char fio[BYTES_DO_CABECALHO + CORPO_DOS_COMANDOS];
    int onde = escreverCabecalho(fio, PACOTE_COMANDOS, proximaSequencia());
    escreverU32(fio, &onde, meuQuadro++);
    for (int i = 0; i < QUADROS_DE_COMANDO; i++) {
        escreverU8(fio, &onde, bitsDosComandos(&historico[i]));
        escreverU8(fio, &onde, (unsigned int) (unsigned char) direcaoNoFio(historico[i].direcao));
    }
    escreverU8(fio, &onde, (unsigned int) corrida->euSou);
    escreverU8(fio, &onde, 0);
    escreverU32(fio, &onde, 0);   /* reservado: o último retrato visto, quando medirmos o atraso */
    enviarPara(soqueteDaSala(), enderecoDoDono(), fio, onde);
}

/* ==================================================================
 * Receber
 * ================================================================== */

/*
 * tratarComandos
 * O dono recebe o que um convidado está fazendo e guarda para o próximo quadro da
 * física. Quem põe isso no carro é corrida.c (atualizarCorrida).
 */
static void tratarComandos(const unsigned char *fio, int tamanho, const CabecalhoDeRede *cabecalho,
                           Corrida *corrida) {
    if (tamanho < BYTES_DO_CABECALHO + CORPO_DOS_COMANDOS || !corrida->emRede || !corrida->souODono) {
        return;
    }
    int onde = BYTES_DO_CABECALHO;
    (void) lerU32(fio, &onde);   /* o quadro do convidado: o cabeçalho já ordena */
    unsigned char bits[QUADROS_DE_COMANDO];
    signed char direcoes[QUADROS_DE_COMANDO];
    for (int i = 0; i < QUADROS_DE_COMANDO; i++) {
        bits[i] = (unsigned char) lerU8(fio, &onde);
        direcoes[i] = (signed char) lerU8(fio, &onde);
    }
    int vaga = (int) lerU8(fio, &onde);
    if (vaga < 0 || vaga >= MAXIMO_DE_JOGADORES || vaga == corrida->euSou) {
        return;
    }
    if (jaVeioComando[vaga] && !sequenciaEMaisNova(cabecalho->sequencia, ultimaDoComando[vaga])) {
        return;   /* chegou fora de ordem: o que vale é o mais novo que já aplicamos */
    }
    ultimaDoComando[vaga] = cabecalho->sequencia;
    jaVeioComando[vaga] = 1;
    corrida->comandosDosHumanos[vaga] = comandosDosBits(bits[0], direcoes[0]);
    corrida->segundosSemComandos[vaga] = 0.0f;
    ouviOConvidado(vaga);
}

/*
 * pularParaORetrato
 * O carro vai para onde o dono disse, de uma vez. Escreve segmento, distância, lateral
 * e rumo JUNTOS: deixar o carro meio corrigido faria andarNoMundo procurar o segmento a
 * partir de um lugar incoerente e, no pior caso, inventar uma volta.
 * Recebe: o carro, o retrato dele e a posição ao longo da pista.
 * Devolve: nada.
 */
static void pularParaORetrato(Carro *carro, const CarroNoFio *fio, float naLinha) {
    voltarParaAPista(naLinha, &carro->posicao);
    carro->deslocamentoLateral = fio->deslocamentoLateral / 8000.0f;
    carro->rumoNoMundo = fio->rumoNoMundo * (360.0f / 65536.0f);
    carro->temRumoNoMundo = 1;
    carro->velocidade = fio->velocidade / 4.0f;
    carro->anguloDeDrift = fio->anguloDeDrift / 100.0f;
    carro->giro = fio->giro / 100.0f;
    carro->rumoDaViagem = carro->rumoNoMundo + carro->anguloDeDrift;
}

/*
 * copiarOQueODonoDecide
 * Os campos em que o dono tem a palavra final, para TODO carro: é estado de prova, não
 * de movimento. Nada disso pode ser deduzido de novo em cada máquina sem divergir.
 *
 * "voltas" é o mais importante da lista: a contagem do jogo acontece dentro de
 * moverNaPista e de andarNoMundo, as duas contando "andando", e um salto de posição faz
 * andarNoMundo achar que cruzou a linha. Escrever o número do dono por cima, depois da
 * física, é o que mantém a volta certa nas duas telas.
 * Recebe: o carro e o retrato dele.
 * Devolve: nada.
 */
static void copiarOQueODonoDecide(Carro *carro, const CarroNoFio *fio) {
    carro->voltas = fio->voltas;
    carro->distanciaPercorrida = fio->distanciaPercorrida;
    carro->tempoDeChegada = fio->tempoDeChegada;
    carro->melhorVolta = fio->melhorVolta;
    carro->desgaste = fio->desgaste / 255.0f;
    carro->nitro = fio->nitro / 255.0f;
    carro->vacuo = fio->vacuo / 255.0f;
    carro->estadoDoPit = (EstadoDoPit) (fio->pit & 0x3u);
    carro->pneuEstourado = (int) ((fio->pit >> 2) & 0xFu);
    carro->ladoDoPrimeiroEstouro = (int) ((fio->pit >> 6) & 0x1u);
    carro->pitFeito = (fio->estado & FIO_PIT_FEITO) ? 1 : 0;
    carro->nitroLigado = (fio->estado & FIO_NITRO_LIGADO) ? 1 : 0;
    carro->rodando = (fio->estado & FIO_RODANDO) ? 1 : 0;
    carro->raspandoNoMuro = (fio->estado & FIO_RASPANDO_NO_MURO) ? 1 : 0;
    /* O contato deste quadro: é o que acende as faíscas e toca o som de lata. Vem do
     * dono porque é ele que julga as batidas. */
    carro->contato = fio->contato / 255.0f;
    carro->contatoComCarro = (fio->estado & FIO_CONTATO_DE_CARRO) ? 1 : 0;
}

/*
 * receberORetratoDeUmCarroRemoto
 * Guarda o que o dono disse sobre um carro que não é o desta máquina: o erro entre onde
 * ele está sendo desenhado e onde o dono o pôs. O erro é consumido aos poucos em
 * andarComOsCarrosRemotos — é isso que tira o tremor dos 30 Hz.
 */
static void receberORetratoDeUmCarroRemoto(int numero, Carro *carro, const CarroNoFio *fio) {
    CarroSuavizado *s = &suave[numero];
    float doDono = metroNaLinha(fio->indiceDoSegmento,
                                fio->distanciaNoSegmento / 65535.0f * COMPRIMENTO_SEGMENTO);
    float lateralDoDono = fio->deslocamentoLateral / 8000.0f;

    if (!s->tem) {
        s->tem = 1;
        s->naLinhaMostrada = doDono;
        s->erroAoLongo = 0.0f;
        s->erroDeLado = 0.0f;
        pularParaORetrato(carro, fio, doDono);
    } else {
        s->erroAoLongo = encurtarNaVolta(doDono - s->naLinhaMostrada);
        s->erroDeLado = lateralDoDono - carro->deslocamentoLateral;
        if (fabsf(s->erroAoLongo) > LIMITE_PARA_PULAR) {
            /* Longe demais para disfarçar (bateu, rodou, ou o pacote sumiu por um tempo):
             * é teleporte, e ainda assim é melhor que o carro atravessar a tela derrapando */
            s->naLinhaMostrada = doDono;
            s->erroAoLongo = 0.0f;
            s->erroDeLado = 0.0f;
            pularParaORetrato(carro, fio, doDono);
        }
    }
    s->velocidadeMostrada = fio->velocidade / 4.0f;
    carro->rumoNoMundo = fio->rumoNoMundo * (360.0f / 65536.0f);
    carro->temRumoNoMundo = 1;
    carro->anguloDeDrift = fio->anguloDeDrift / 100.0f;
    carro->giro = fio->giro / 100.0f;
    carro->volante = fio->volante / 100.0f;
    carro->derrapagem = fio->derrapagem / 255.0f;
    carro->forcaDoDrift = fio->forcaDoDrift / 255.0f;
    carro->direcaoLivre = (fio->estado & FIO_DIRECAO_LIVRE) ? 1 : 0;
    carro->ultimosComandos.acelerar = (fio->estado & FIO_ACELERANDO) ? 1 : 0;
    carro->ultimosComandos.frear = (fio->estado & FIO_FREANDO) ? 1 : 0;
    carro->rumoDaViagem = carro->rumoNoMundo + carro->anguloDeDrift;
    copiarOQueODonoDecide(carro, fio);
}

/*
 * receberORetratoDoMeuCarro
 * A reconciliação do carro do jogador desta máquina.
 *
 * Posição e velocidade são corrigidas; RUMO, VOLANTE, DRIFT e DERRAPAGEM não — esses
 * são o resultado direto das mãos do jogador, e mexer neles é exatamente o que faz um
 * jogo de corrida online dar a sensação de estar lutando com o volante. A exceção é o
 * pulo forte, em que vai tudo: aí o que o jogador acha que aconteceu já não é o que
 * aconteceu.
 */
static void receberORetratoDoMeuCarro(Carro *carro, const CarroNoFio *fio) {
    float doDono = metroNaLinha(fio->indiceDoSegmento,
                                fio->distanciaNoSegmento / 65535.0f * COMPRIMENTO_SEGMENTO);
    float meu = metroNaLinha(carro->posicao.segmento != NULL ? carro->posicao.segmento->indice : 0,
                             carro->posicao.distanciaNoSegmento);
    float erro = encurtarNaVolta(doDono - meu);

    /* No pitstop, o convidado se rende e segue o retrato como um carro qualquer:
     * atualizarCarroNoPit é uma coreografia roteirizada de vários segundos, sem entrada
     * do jogador, e prever isso só gera briga entre as duas máquinas. */
    int noPit = (fio->pit & 0x3u) != PIT_FORA;

    if (noPit || fabsf(erro) > LIMITE_PARA_PULAR) {
        pularParaORetrato(carro, fio, doDono);
        suave[carro->numero].erroAoLongo = 0.0f;
    } else if (fabsf(erro) > FOLGA_SEM_CORRIGIR) {
        suave[carro->numero].erroAoLongo = erro;   /* sangrado aos poucos no quadro (veja atualizarOnline) */
    } else {
        suave[carro->numero].erroAoLongo = 0.0f;   /* perto o bastante: corrigir seria pior que o erro */
    }
    copiarOQueODonoDecide(carro, fio);
}

/*
 * tratarRetrato
 * O convidado recebe o retrato dos 14 carros.
 */
static void tratarRetrato(const unsigned char *fio, int tamanho, const CabecalhoDeRede *cabecalho,
                          Corrida *corrida) {
    if (tamanho < BYTES_DO_CABECALHO + CORPO_DO_QUADRO || !corrida->emRede || corrida->souODono) {
        return;
    }
    if (jaVeioRetrato && !sequenciaEMaisNova(cabecalho->sequencia, ultimaDoRetrato)) {
        return;   /* velho: o que já está na tela é mais novo que este */
    }
    int onde = BYTES_DO_CABECALHO;
    (void) lerU32(fio, &onde);   /* o quadro do dono */
    float tempoDeCorrida = lerF32(fio, &onde);
    float contagemRegressiva = lerF32(fio, &onde);
    int terminou = (int) lerU8(fio, &onde);
    int quantosCarros = (int) lerU8(fio, &onde);
    (void) lerU16(fio, &onde);
    if (quantosCarros != TOTAL_CARROS) {
        return;   /* build diferente: ler isto daria carros voando pela tela */
    }
    ultimaDoRetrato = cabecalho->sequencia;
    jaVeioRetrato = 1;
    desdeORetrato = 0.0f;
    ouviODonoAgora();

    /* O relógio da prova vem do dono, sempre: GetFrameTime() não dá o mesmo número em
     * dois PCs, e integrar aqui faria as voltas e os intervalos divergirem devagar e
     * para sempre, sem ninguém notar até comparar as duas telas. */
    corrida->tempoDeCorrida = tempoDeCorrida;
    corrida->contagemRegressiva = contagemRegressiva;
    (void) terminou;   /* o fim do jogador local sai do tempoDeChegada do carro dele */

    for (int numero = 0; numero < TOTAL_CARROS; numero++) {
        CarroNoFio noFio;
        lerCarroDoFio(fio, &onde, &noFio);
        Carro *carro = carroPorNumero(corrida, numero);
        if (carro == NULL) {
            continue;
        }
        if (carro == &corrida->jogador) {
            receberORetratoDoMeuCarro(carro, &noFio);
        } else {
            receberORetratoDeUmCarroRemoto(numero, carro, &noFio);
        }
    }
    ordenarCarrosPorDistancia(corrida->classificacao, TOTAL_CARROS);
}

void receberPacoteDaCorrida(const unsigned char *fio, int tamanho, const CabecalhoDeRede *cabecalho,
                            const EnderecoDaRede *de, Corrida *corrida) {
    (void) de;
    if (corrida == NULL) {
        return;
    }
    if (cabecalho->tipo == PACOTE_COMANDOS) {
        tratarComandos(fio, tamanho, cabecalho, corrida);
    } else if (cabecalho->tipo == PACOTE_QUADRO) {
        tratarRetrato(fio, tamanho, cabecalho, corrida);
    }
}

/* ==================================================================
 * O quadro
 * ================================================================== */

/*
 * andarComOsCarrosRemotos
 * Entre dois retratos (33 ms) os carros dos outros continuam andando por conta própria,
 * com a última velocidade que o dono mandou, e o erro que o retrato revelou é consumido
 * aos poucos. Sem isto, eles andariam em 30 passos por segundo enquanto o jogo desenha
 * 60 ou mais — e pulo de posição é a coisa mais visível que existe num jogo de corrida.
 *
 * Também recalcula aqui o que NÃO viaja pela rede, porque cada máquina chega ao mesmo
 * resultado sozinha: o motor (pelo som), as rodas girando e a suspensão.
 * Recebe: a corrida e o tempo do quadro.
 * Devolve: nada.
 */
static void andarComOsCarrosRemotos(Corrida *corrida, float tempoQuadro) {
    float fatia = 1.0f - expf(-tempoQuadro / SEGUNDOS_PARA_ALINHAR);
    for (int numero = 0; numero < TOTAL_CARROS; numero++) {
        CarroSuavizado *s = &suave[numero];
        Carro *carro = carroPorNumero(corrida, numero);
        if (!s->tem || carro == NULL || carro == &corrida->jogador) {
            continue;
        }
        float passo = s->erroAoLongo * fatia;
        s->erroAoLongo -= passo;
        s->naLinhaMostrada += s->velocidadeMostrada * tempoQuadro + passo;
        voltarParaAPista(s->naLinhaMostrada, &carro->posicao);

        float deLado = s->erroDeLado * fatia;
        s->erroDeLado -= deLado;
        carro->deslocamentoLateral += deLado;
        carro->velocidade = s->velocidadeMostrada;

        /* O enfeite, recalculado: não gasta rede e sai igual em todas as máquinas */
        atualizarMotor(&carro->motor, carro->velocidade / carro->velocidadeMaxima,
                       carro->ultimosComandos.acelerar, carro->ultimosComandos.frear, 0, tempoQuadro);
        carro->anguloDasRodas = fmodf(carro->anguloDasRodas
                                      + rapidezDasRodas(carro->velocidade, carro->ultimosComandos.acelerar) * tempoQuadro
                                      + 2.0f * 3.14159265f, 2.0f * 3.14159265f);
        atualizarSuspensao(carro, tempoQuadro);
    }
}

/*
 * alinharOMeuCarro
 * Consome, aos poucos, o erro que o retrato apontou no carro do jogador. Separado dos
 * carros remotos porque aqui o ritmo é outro (mais lento) e só a posição é tocada.
 */
static void alinharOMeuCarro(Corrida *corrida, float tempoQuadro) {
    CarroSuavizado *s = &suave[corrida->jogador.numero];
    if (fabsf(s->erroAoLongo) < 0.01f) {
        return;
    }
    float passo = s->erroAoLongo * (1.0f - expf(-tempoQuadro / SEGUNDOS_PARA_ME_ALINHAR));
    s->erroAoLongo -= passo;
    /* moverNaPista contaria volta se o passo cruzasse a linha; o número de voltas é do
     * dono e já foi copiado, então aqui só a posição anda */
    float meu = metroNaLinha(corrida->jogador.posicao.segmento != NULL
                             ? corrida->jogador.posicao.segmento->indice : 0,
                             corrida->jogador.posicao.distanciaNoSegmento);
    voltarParaAPista(meu + passo, &corrida->jogador.posicao);
}

void atualizarOnline(Corrida *corrida, Comandos meusComandos, float tempoQuadro) {
    if (corrida == NULL || !corrida->emRede) {
        return;
    }
    desdeORetrato += tempoQuadro;

    if (corrida->souODono) {
        /* Os convidados calados: o relógio corre aqui, e corrida.c entrega o carro à IA
         * quando ele estoura. A vaga NÃO é liberada no meio da prova: tirar um carro do
         * grid mudaria a classificação de todos os outros. */
        for (int vaga = 0; vaga < MAXIMO_DE_JOGADORES; vaga++) {
            if (vaga != corrida->euSou) {
                corrida->segundosSemComandos[vaga] = segundosCaladoOConvidado(vaga);
            }
        }
        desdeQueMandei += tempoQuadro;
        if (desdeQueMandei >= SEGUNDOS_ENTRE_RETRATOS) {
            desdeQueMandei = 0.0f;
            mandarRetrato(corrida);
        }
        return;
    }

    /* Convidado */
    mandarComandos(corrida, meusComandos);
    alinharOMeuCarro(corrida, tempoQuadro);
    andarComOsCarrosRemotos(corrida, tempoQuadro);

    /* O dono sumiu: a corrida VIRA offline aqui, em vez de deixar o jogador parado numa
     * pista congelada. A física local assume tudo e a IA pega os carros que eram
     * humanos — ninguém é expulso no meio de uma curva. */
    if (segundosSemOuvirODono() > 3.0f) {
        corrida->emRede = 0;
        corrida->souODono = 1;
        corrida->totalDeHumanos = 1;
        for (int vaga = 0; vaga < MAXIMO_DE_JOGADORES; vaga++) {
            if (vaga != corrida->euSou && corrida->humanos[vaga] != NULL) {
                corrida->humanos[vaga]->controladoPeloJogador = 0;
                corrida->humanos[vaga] = NULL;
            }
        }
        perdiODono();
    }
}
