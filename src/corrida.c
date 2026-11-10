/*
 * corrida.c
 * ------------------------------------------------------------------
 * Regras da corrida. Veja corrida.h.
 * ------------------------------------------------------------------
 */

#include <stddef.h>
#include <math.h>
#include "corrida.h"
#include "configuracoes.h"
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

/* ---------- Corrida em rede ---------- */
/* Tanto tempo sem chegar comando de um humano e a IA toma o volante dele. Dois segundos
 * é longo para um engasgo de WiFi (que dura décimos) e curto para um jogo que travou:
 * o carro não fica parado no meio da pista atrapalhando a corrida de todos. Voltando a
 * chegar comando, o jogador recebe o volante de volta. */
#define SEGUNDOS_ATE_A_IA_ASSUMIR  2.0f

/* ---------- Colisão ---------- */
#define RESTITUICAO               0.6f      /* na traseira, quanto da diferença de velocidade passa de um para o outro */
#define ATRITO_DE_LADO            3500.0f   /* raspando de lado, os dois perdem isso de velocidade por segundo */
#define TRANCO_MINIMO             2.0f      /* metros por segundo: toda batida de lado separa um pouco */
#define TRANCO_POR_FECHAMENTO     0.7f      /* mais isso vezes a velocidade com que se encontraram */
#define FORA_DO_CENTRO            0.30f     /* contato mais fora do centro que isso (0 a 1) pode fazer rodar */
#define GIRO_POR_TRASEIRA         0.55f     /* graus por segundo por unidade/s de batida na traseira, vezes a excentricidade */
#define GIRO_POR_LADO             75.0f     /* graus por segundo por metro/s de batida de lado, vezes a excentricidade */

/* ---------- Som da batida (só o McQueen) ---------- */
#define NUMERO_DO_MCQUEEN          0         /* o número dele na tabela FICHAS */
#define ALCANCE_DA_BATIDA_DE_RIVAL 60.0f     /* metros: o McQueen rival bate mais longe que isso, ninguém ouve */
#define SEGUNDOS_PARA_CONFIRMAR_POSICAO 0.4f /* uma troca de posição só vale se durar isso (vários carros o passando juntos viram uma troca só) */
#define SEGUNDOS_PARA_VALER_A_PONTA 2.0f     /* quem ficou em 1º menos que isso e perdeu a ponta não "perdeu a liderança" */
#define VOLUME_DA_BATIDA_DE_RIVAL  0.6f      /* a batida do McQueen rival, colado no jogador, soa assim, em vez de 1 */

/* ---------- O que a IA enxerga ---------- */
#define VISAO_PARA_TRAS_NO_PIT    22.0f     /* metros: quem sai da vaga não entra na frente de quem vem */
#define FOLGA_DE_FAIXA_NO_PIT     0.4f      /* metros de lado: mais que isso, passa ao lado */
#define ALCANCE_DA_VISAO          60.0f     /* metros à frente */
#define ALCANCE_PARA_TRAS         30.0f     /* metros atrás: quem vem perto assim pode ser bloqueado */
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

/* Os 17 carros do jogo. O número de cada um é a posição neste vetor e
 * define o modelo 3D (carros3d.c). O jogador escolhe um deles na garagem;
 * os outros 16 viram os rivais. Os carros têm jeitos diferentes, mas o mesmo
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
    { "Jackson Storm", "nova geracao, voa na reta", 12110.0f, 2400.0f, 0.489f, 4.5f, 2.17f },
    { "Boost", "lider dos tuners, arrancada brava", 12040.0f, 2750.0f, 0.472f, 4.5f, 2.31f },
    { "DJ", "tuner do som, liso nas curvas", 12000.0f, 2650.0f, 0.492f, 4.2f, 2.58f },
};

/* Os níveis: o fácil não tem pitstop, e só o difícil gasta os pneus. Quanto mais difícil, mais os
 * rivais defendem a posição e revidam uma ultrapassagem, e mais constante é o ritmo deles.
 *              nome      voltas pole   último  pit pneu pelotão defesa revide variação */
static const FichaDoNivel NIVEIS[TOTAL_NIVEIS] = {
    { "FÁCIL",   3, 0.78f, 0.70f, 0, 0, 1.0f, 0.00f, 0.00f, 0.020f },
    { "MÉDIO",   5, 0.90f, 0.83f, 1, 0, 0.7f, 0.40f, 0.65f, 0.012f },
    { "DIFÍCIL", 8, 0.94f, 0.88f, 1, 1, 0.5f, 0.75f, 1.00f, 0.006f },
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
 * Recebe: o carro (já criado na linha de chegada), a pista e o lugar (0 = pole).
 * Devolve: nada.
 */
static void colocarNoGrid(Carro *carro, const Pista *pista, int lugar) {
    float distanciaAteALinha = distanciaDoLugarAteALinha(pista, lugar);

    moverNaPista(&carro->posicao, -distanciaAteALinha);   /* anda para trás, atravessando a linha */
    carro->distanciaPercorrida = -distanciaAteALinha;
    carro->voltas = -1;

    carro->deslocamentoLateral = lateralDoLugar(pista, lugar);
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
 * escalaDoDesgaste
 * Os pneus duram cerca de 5 voltas de Interlagos. Como lá o modelo é usado maior que o de verdade
 * (ESCALA_INTERLAGOS), a volta é mais longa e os pneus andam mais na mesma proporção.
 * Recebe: a pista.
 * Devolve: por quanto multiplicar a distância que um jogo de pneus anda.
 */
static float escalaDoDesgaste(const Pista *pista) {
    return (pista->id == PISTA_INTERLAGOS) ? ESCALA_INTERLAGOS : 1.0f;
}

/*
 * montarCorrida
 * Prepara uma corrida nova: o jogador com o carro escolhido na garagem, os outros
 * carros como rivais, todos no grid, relógio zerado, contagem regressiva armada e
 * a classificação montada. Serve também para correr de novo.
 *
 * É a base das duas corridas do jogo, e a formação é o que as separa:
 *   - formacao NULL: a corrida de um jogador, igual a sempre (1 humano + 13 da IA);
 *   - formacao preenchida: a corrida em rede, com até MAXIMO_DE_JOGADORES humanos
 *     ocupando vagas de rivais[] e a IA preenchendo o resto do grid.
 * Recebe: a corrida a preparar, a pista, o carro do jogador (ignorado quando há
 *         formação, que já diz o carro de cada um), o nível e a formação.
 * Devolve: nada.
 */
static void montarCorrida(Corrida *corrida, Pista *pista, int carroDoJogador, NivelDaCorrida nivel,
                          const FormacaoOnline *formacao) {
    static unsigned int corridasIniciadas = 0;   /* cada corrida sorteia ritmos diferentes */
    corridasIniciadas++;
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
    corrida->mcqueenLiderava = 0;
    corrida->mcqueenTempoNaPonta = 0.0f;
    corrida->mcqueenPosicao = 0;
    corrida->mcqueenPosicaoNova = 0;
    corrida->mcqueenTempoNaPosicaoNova = 0.0f;
    corrida->desclassificado = 0;
    corrida->posicaoFinal = 0;

    /* ---- Quem é quem: os carros e os lugares do grid que os humanos tomaram ---- */
    int humanos = (formacao != NULL) ? formacao->totalDeJogadores : 1;
    int euSou = (formacao != NULL) ? formacao->euSou : 0;
    unsigned int semente = (formacao != NULL) ? formacao->semente : corridasIniciadas;
    int carroDoLocal = (formacao != NULL) ? formacao->numeroDoCarro[euSou] : carroDoJogador;
    int lugarDoLocal = (formacao != NULL) ? formacao->lugarNoGrid[euSou] : LUGAR_DO_JOGADOR;

    corrida->emRede = (formacao != NULL);
    corrida->souODono = (formacao != NULL) ? formacao->souODono : 0;
    corrida->totalDeHumanos = humanos;
    corrida->euSou = euSou;
    for (int h = 0; h < MAXIMO_DE_JOGADORES; h++) {
        corrida->humanos[h] = NULL;
        corrida->comandosDosHumanos[h] = (Comandos) { 0, 0, 0, 0, 0.0f, 0, 0 };
        corrida->segundosSemComandos[h] = 0.0f;
    }

    int carroTomado[TOTAL_CARROS], lugarTomado[TOTAL_CARROS];
    for (int i = 0; i < TOTAL_CARROS; i++) {
        carroTomado[i] = 0;
        lugarTomado[i] = 0;
    }
    for (int h = 0; h < humanos; h++) {
        int numero = (formacao != NULL) ? formacao->numeroDoCarro[h] : carroDoJogador;
        int lugar = (formacao != NULL) ? formacao->lugarNoGrid[h] : LUGAR_DO_JOGADOR;
        carroTomado[numero % TOTAL_CARROS] = 1;
        lugarTomado[lugar % TOTAL_CARROS] = 1;
    }

    /* Numa corrida em rede, TODOS os carros andam sem direção guiada, mesmo que alguém
     * a tenha ligado nas configurações. Ela muda a física, e o dono roda a física de
     * todo mundo: deixar cada máquina com um modelo diferente faria o carro do convidado
     * ser corrigido para sempre, contra a mão dele. (E a guiada existe para os testes
     * automáticos, não para a corrida.) */
    int semGuiada = (formacao != NULL) || pista->semDirecaoGuiada || !configuracoesAtuais()->direcaoGuiada;

    corrida->jogador = criarCarroDaFicha(FICHAS[carroDoLocal], carroDoLocal, pista);
    corrida->jogador.controladoPeloJogador = 1;
    colocarNoGrid(&corrida->jogador, pista, lugarDoLocal);
    corrida->jogador.duracaoDaTroca = DURACAO_DA_TROCA;
    corrida->jogador.voltaDoPit = temParada ? (ficha->voltas + 1) / 2 : 0;   /* se a IA dirigir */
    corrida->jogador.voltasDaProva = ficha->voltas;
    corrida->jogador.gastaPneu = gastaPneu;
    corrida->jogador.metrosDeUmJogoDePneus *= escalaDoDesgaste(pista);
    corrida->jogador.pitFeito = !temParada;
    corrida->jogador.direcaoLivre = semGuiada;
    corrida->classificacao[0] = &corrida->jogador;
    corrida->humanos[euSou] = &corrida->jogador;

    int rival = 0;   /* a vaga em rivais[] que está sendo preenchida */

    /* ---- Os outros humanos: carros iguais ao do jogador, só que dirigidos pela rede ----
     * Eles NÃO levam habilidade, defesa, revide nem variação: nada de IA os toca enquanto
     * os comandos deles estiverem chegando (veja atualizarCorrida). */
    for (int h = 0; h < humanos; h++) {
        if (h == euSou) {
            continue;
        }
        int numero = formacao->numeroDoCarro[h] % TOTAL_CARROS;
        Carro *dele = &corrida->rivais[rival];
        *dele = criarCarroDaFicha(FICHAS[numero], numero, pista);
        colocarNoGrid(dele, pista, formacao->lugarNoGrid[h]);
        dele->controladoPeloJogador = 1;
        dele->voltasDaProva = ficha->voltas;
        dele->gastaPneu = gastaPneu;
        dele->metrosDeUmJogoDePneus *= escalaDoDesgaste(pista);
        dele->pitFeito = !temParada;
        dele->duracaoDaTroca = DURACAO_DA_TROCA;   /* a troca de um humano é a do jogador, não a dos rivais */
        dele->voltaDoPit = 0;                      /* humano decide sozinho quando entrar no box */
        dele->direcaoLivre = semGuiada;
        /* A habilidade só vale se a IA tiver de assumir o carro (conexão caiu): fica no meio
         * da tabela do nível, para o carro não virar uma pedra nem um foguete */
        dele->habilidade = (ficha->habilidadeDaPole + ficha->habilidadeDoUltimo) / 2.0f;
        dele->sorteio = 2654435761u * (unsigned int) (h + 1) + 40503u * semente;
        corrida->humanos[h] = dele;
        corrida->classificacao[rival + 1] = dele;
        rival++;
    }

    /* ---- Os rivais de IA: todo carro que nenhum humano pegou ---- */
    int totalDaIA = TOTAL_CARROS - humanos;
    int ultimoDaIA = (totalDaIA > 1) ? totalDaIA - 1 : 1;   /* o divisor da escala de habilidade */
    int ordemDaIA = 0;     /* 0 é o mais rápido da IA (a "pole" dela) */
    int lugarDaIA = 0;     /* o primeiro lugar do grid que nenhum humano tomou */
    for (int numero = 0; numero < TOTAL_CARROS; numero++) {
        if (carroTomado[numero]) {
            continue;
        }
        while (lugarDaIA < TOTAL_CARROS && lugarTomado[lugarDaIA]) {
            lugarDaIA++;
        }
        Carro *oRival = &corrida->rivais[rival];
        *oRival = criarCarroDaFicha(FICHAS[numero], numero, pista);
        colocarNoGrid(oRival, pista, lugarDaIA++);
        oRival->habilidade = ficha->habilidadeDaPole
            - (ficha->habilidadeDaPole - ficha->habilidadeDoUltimo) * ordemDaIA / ultimoDaIA;
        oRival->voltasDaProva = ficha->voltas;
        oRival->defesa = ficha->defesa;
        oRival->vontadeDeRevide = ficha->revide;
        oRival->variacao = ficha->variacao;
        oRival->sorteio = 2654435761u * (unsigned int) (ordemDaIA + 1) + 40503u * semente;
        oRival->gastaPneu = gastaPneu;
        oRival->metrosDeUmJogoDePneus *= escalaDoDesgaste(pista);
        /* De propósito SEM mexer em direcaoLivre: criarCarro deixa em 0, e a IA sempre
         * dirigiu com a direção guiada — é o modelo para o qual ela foi acertada. Pôr a
         * IA no rumo livre a faria sair da pista nas curvas. */
        /* Cada um para numa volta, para os boxes não lotarem; na troca, eles são mais lentos que o Guido */
        oRival->voltaDoPit = temParada ? 2 + ordemDaIA * 5 % (ficha->voltas - 2) : 0;
        oRival->pitFeito = !temParada;
        oRival->duracaoDaTroca = TROCA_MAIS_RAPIDA_DOS_RIVAIS + (ordemDaIA % 6) * PASSO_DA_TROCA_DOS_RIVAIS;
        corrida->classificacao[rival + 1] = oRival;
        rival++;
        ordemDaIA++;
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
 * iniciarCorrida
 * A corrida de um jogador: ele e os 13 rivais de IA. Veja montarCorrida.
 * Recebe: a corrida a preparar, a pista, o carro do jogador e o nível.
 * Devolve: nada.
 */
void iniciarCorrida(Corrida *corrida, Pista *pista, int carroDoJogador, NivelDaCorrida nivel) {
    montarCorrida(corrida, pista, carroDoJogador, nivel, NULL);
}

/*
 * iniciarCorridaOnline
 * A corrida em rede: os humanos da formação nas vagas que o dono da sala deu, e a
 * IA no resto do grid. Quem monta a formação é sala.c, do pacote de largada — e ela
 * é IGUAL em todas as máquinas, então todas montam exatamente o mesmo grid.
 * Recebe: a corrida a preparar, a pista, o nível e a formação.
 * Devolve: nada.
 */
void iniciarCorridaOnline(Corrida *corrida, Pista *pista, NivelDaCorrida nivel, const FormacaoOnline *formacao) {
    if (formacao == NULL || formacao->totalDeJogadores < 1) {
        iniciarCorrida(corrida, pista, 0, nivel);   /* formação estragada: não dá para largar em rede */
        return;
    }
    montarCorrida(corrida, pista, formacao->numeroDoCarro[formacao->euSou], nivel, formacao);
}

/*
 * carroEDeHumano
 * Recebe: a corrida e um carro dela.
 * Devolve: 1 se este carro é de um jogador de verdade (de qualquer máquina), 0 se é da IA.
 */
int carroEDeHumano(const Corrida *corrida, const Carro *carro) {
    if (!corrida->emRede) {
        return carro == &corrida->jogador;
    }
    for (int h = 0; h < corrida->totalDeHumanos; h++) {
        if (corrida->humanos[h] == carro) {
            return 1;
        }
    }
    return 0;
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
 * mudancaDeLiderancaDoMcQueen
 * Diz se o McQueen do jogador acabou de assumir a liderança da corrida ou de perdê-la, para as
 * falas dele. Chamar uma vez por quadro. Só vale com a corrida andando, antes da bandeirada, e só
 * conta como perda se ele chegou a ficar um tempo na ponta (dois carros trocando de lugar a todo
 * instante não viram uma fala a cada troca). Se vários carros o passam seguidos, é uma perda só:
 * ele já deixou de liderar no primeiro.
 * Recebe: a corrida e o tempo do quadro.
 * Devolve: +1 quando assume a liderança, -1 quando a perde, 0 no resto do tempo.
 */
int mudancaDeLiderancaDoMcQueen(Corrida *corrida, float tempoQuadro) {
    int lidera = corrida->jogador.numero == NUMERO_DO_MCQUEEN && corrida->classificacao[0] == &corrida->jogador
                 && corrida->contagemRegressiva <= 0.0f && !corrida->terminou;
    if (corrida->terminou) {
        corrida->mcqueenLiderava = 0;   /* a bandeirada fecha a conversa: nada de fala depois dela */
        return 0;
    }
    if (lidera && !corrida->mcqueenLiderava) {
        corrida->mcqueenLiderava = 1;
        corrida->mcqueenTempoNaPonta = 0.0f;
        return 1;
    }
    if (lidera) {
        corrida->mcqueenTempoNaPonta += tempoQuadro;
    } else if (corrida->mcqueenLiderava) {
        corrida->mcqueenLiderava = 0;
        return (corrida->mcqueenTempoNaPonta >= SEGUNDOS_PARA_VALER_A_PONTA) ? -1 : 0;
    }
    return 0;
}

/*
 * mudancaDePosicaoDoMcQueen
 * Diz se o McQueen do jogador ganhou ou perdeu posição (ultrapassou ou foi ultrapassado), em
 * qualquer lugar da fila, para as falas dele. Chamar uma vez por quadro. A posição nova só vale se
 * se mantiver por um instante: vários carros passando juntos, ou dois trocando de lugar a todo
 * quadro, viram uma troca só (ou nenhuma). Não vale na contagem, depois da bandeirada, nem no
 * pit lane (quem para perde posições sem ter sido ultrapassado de verdade).
 * Recebe: a corrida e o tempo do quadro.
 * Devolve: +1 se ganhou posição, -1 se perdeu, 0 no resto do tempo.
 */
int mudancaDePosicaoDoMcQueen(Corrida *corrida, float tempoQuadro) {
    Carro *jogador = &corrida->jogador;
    int posicao = 0;
    for (int i = 0; i < TOTAL_CARROS; i++) {
        if (corrida->classificacao[i] == jogador) {
            posicao = i + 1;
        }
    }
    int vale = jogador->numero == NUMERO_DO_MCQUEEN && corrida->contagemRegressiva <= 0.0f && !corrida->terminou
               && !carroNoPit(jogador) && posicao > 0;
    if (!vale || corrida->mcqueenPosicao == 0) {
        corrida->mcqueenPosicao = vale ? posicao : 0;   /* só acompanha: nada a dizer */
        corrida->mcqueenPosicaoNova = posicao;
        corrida->mcqueenTempoNaPosicaoNova = 0.0f;
        return 0;
    }
    if (posicao == corrida->mcqueenPosicao) {
        corrida->mcqueenPosicaoNova = posicao;
        corrida->mcqueenTempoNaPosicaoNova = 0.0f;
        return 0;
    }
    if (posicao != corrida->mcqueenPosicaoNova) {
        corrida->mcqueenPosicaoNova = posicao;   /* mudou de novo antes de confirmar: recomeça a contar */
        corrida->mcqueenTempoNaPosicaoNova = 0.0f;
        return 0;
    }
    corrida->mcqueenTempoNaPosicaoNova += tempoQuadro;
    if (corrida->mcqueenTempoNaPosicaoNova < SEGUNDOS_PARA_CONFIRMAR_POSICAO) {
        return 0;
    }
    int ganhou = posicao < corrida->mcqueenPosicao;
    corrida->mcqueenPosicao = posicao;
    corrida->mcqueenTempoNaPosicaoNova = 0.0f;
    return ganhou ? 1 : -1;
}

/*
 * proximidadeDoMcQueen
 * O quanto o jogador ouve o McQueen: ele mesmo, em cheio; um McQueen rival, só se estiver perto e
 * mais baixo.
 * Recebe: a corrida e o carro (o jogador ou um rival).
 * Devolve: de 0 a 1.
 */
static float proximidadeDoMcQueen(Corrida *corrida, const Carro *carro) {
    if (carro == &corrida->jogador) {
        return 1.0f;
    }
    float metros = fabsf(carro->distanciaPercorrida - corrida->jogador.distanciaPercorrida) / UNIDADES_POR_METRO;
    return fmaxf(1.0f - metros / ALCANCE_DA_BATIDA_DE_RIVAL, 0.0f) * VOLUME_DA_BATIDA_DE_RIVAL;
}

/*
 * rodadaDoMcQueen
 * Diz se o McQueen começou a rodar (girar descontrolado) desde a última vez que isto foi chamado.
 * Só a rodada DELE conta: um carro que gira porque o McQueen bateu nele não faz o McQueen gritar.
 * Chamar uma vez por quadro: zera o aviso de rodada de todos os carros.
 * Recebe: a corrida.
 * Devolve: o volume do grito (0 a 1; 0 = não rodou, ou rodou longe demais para ser ouvido).
 */
float rodadaDoMcQueen(Corrida *corrida) {
    float volume = 0.0f;
    for (int i = -1; i < TOTAL_RIVAIS; i++) {
        Carro *carro = (i < 0) ? &corrida->jogador : &corrida->rivais[i];
        int rodou = carro->rodadaNova;
        carro->rodadaNova = 0;
        if (rodou && carro->numero == NUMERO_DO_MCQUEEN) {
            volume = fmaxf(volume, proximidadeDoMcQueen(corrida, carro));
        }
    }
    return volume;
}

/*
 * batidaDoMcQueen
 * Diz se o McQueen bateu em alguma coisa (em outro carro ou no muro) desde a última vez que
 * isto foi chamado. Os sons de batida são dele: se o jogador escolheu outro carro, só
 * soam quando o McQueen rival bate perto o bastante para ser ouvido. Chamar uma vez por
 * quadro: zera as batidas novas de todos os carros.
 * Recebe: a corrida e onde guardar se a batida foi contra outro carro (1) ou no muro (0); pode ser NULL.
 * Devolve: o volume da batida (0 a 1; 0 = não bateu, ou bateu longe demais).
 */
float batidaDoMcQueen(Corrida *corrida, int *contraCarro) {
    float volume = 0.0f;
    if (contraCarro != NULL) {
        *contraCarro = 0;
    }
    for (int i = -1; i < TOTAL_RIVAIS; i++) {
        Carro *carro = (i < 0) ? &corrida->jogador : &corrida->rivais[i];
        float batida = carro->batidaNova;
        int deCarro = carro->batidaNovaDeCarro;
        carro->batidaNova = 0.0f;
        carro->batidaNovaDeCarro = 0;
        if (carro->numero != NUMERO_DO_MCQUEEN || batida <= 0.0f) {
            continue;
        }
        float proximidade = proximidadeDoMcQueen(corrida, carro);
        if (batida * proximidade > volume) {
            volume = batida * proximidade;
            if (contraCarro != NULL) {
                *contraCarro = deCarro;
            }
        }
    }
    return volume;
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
    a->batidaNova = fmaxf(a->batidaNova, pancada);
    b->batidaNova = fmaxf(b->batidaNova, pancada);
    a->batidaNovaDeCarro = 1;
    b->batidaNovaDeCarro = 1;

    /* O contato, para as faíscas e o som de lata: no meio dos dois carros, na altura do para-choque */
    PontoNoMundo pa = pontoNoMundo(a->posicao, a->deslocamentoLateral);
    PontoNoMundo pb = pontoNoMundo(b->posicao, b->deslocamentoLateral);
    float deslize = (fabsf(a->velocidade - b->velocidade) + fabsf(a->velocidade + b->velocidade) * 0.5f * (deLado < naFrente ? 0.5f : 0.1f))
                    / UNIDADES_POR_METRO;
    Carro *dupla[2] = { a, b };
    for (int k = 0; k < 2; k++) {
        if (pancada >= dupla[k]->contato) {
            dupla[k]->contato = pancada;
            dupla[k]->contatoComCarro = 1;
            dupla[k]->contatoX = (pa.x + pb.x) * 0.5f;
            dupla[k]->contatoY = (pa.y + pb.y) * 0.5f + 0.4f;
            dupla[k]->contatoZ = (pa.z + pb.z) * 0.5f;
            dupla[k]->contatoDeslize = deslize;
        }
    }

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
 * mesma linha, quem está mais perto à frente e atrás em qualquer linha, e se há
 * alguém encostado em cada lado.
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
        carro->distanciaAtras = -1.0f;
        carro->distanciaDoDaFrente = -1.0f;
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
            /* No pit lane, quem está parado na vaga só conta como "na frente" para quem está de
             * fato na mesma faixa: quem passa ao lado segue em frente */
            int mesmaFaixa = vaoDeLado < DISTANCIA_AO_LADO;
            if (carroNoPit(carro) && carroNoPit(outro) && outro->velocidade <= 0.0f) {
                mesmaFaixa = vaoDeLado < FOLGA_DE_FAIXA_NO_PIT;
            }
            /* Em qualquer linha: o mais perto à frente (para o vácuo) e atrás (para a defesa) */
            if (!carroNoPit(carro) && !carroNoPit(outro)) {
                if (d > 0.0f && vaoNaFrente < ALCANCE_DA_VISAO
                    && (carro->distanciaDoDaFrente < 0.0f || vaoNaFrente < carro->distanciaDoDaFrente)) {
                    carro->distanciaDoDaFrente = fmaxf(vaoNaFrente, 0.0f);
                    carro->velocidadeDoDaFrente = outro->velocidade;
                    carro->lateralDoDaFrente = lateralEmMetros(outro);
                }
                if (d < 0.0f && vaoNaFrente < ALCANCE_PARA_TRAS
                    && (carro->distanciaAtras < 0.0f || vaoNaFrente < carro->distanciaAtras)) {
                    carro->distanciaAtras = fmaxf(vaoNaFrente, 0.0f);
                    carro->velocidadeAtras = outro->velocidade;
                    carro->lateralAtras = lateralEmMetros(outro);
                }
            }
            if (d > 0.0f && mesmaFaixa && vaoNaFrente < ALCANCE_DA_VISAO
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

/* O pelotão: quem abriu muito na frente do jogador alivia um pouco o motor (só para baixo: nunca
 * anda mais que o carro dele), e quem ficou muito para trás se esforça mais, pilotando mais perto
 * do limite do próprio carro (freia mais tarde, contorna mais rápido). Sem motor a mais: dá para
 * ver o carro forçando, não acelerando do nada. A força vem do nível */
#define FOLGA_NA_FRENTE         80.0f    /* metros à frente do jogador sem ajuste nenhum */
#define FOLGA_ATRAS             60.0f    /* e atrás */
#define METROS_ATE_O_AJUSTE_TODO 400.0f
#define MAIOR_ESPERA            0.04f    /* até 4% a menos de motor, para quem está bem na frente */
#define MAIOR_ESFORCO           0.06f    /* até 6% mais perto do limite, para quem ficou bem para trás */
#define MUDANCA_DO_RITMO        0.02f    /* por segundo: o ajuste chega aos poucos */
#define MUDANCA_DO_ESFORCO      0.05f    /* por segundo */
/* O revide: só quem o jogador acabou de passar reage. Por alguns segundos ele força mais, freia mais
 * tarde (ia.c) e usa o nitro para tentar a posição de volta */
#define PERTO_PARA_REVIDAR      25.0f    /* metros: passado de perto assim, revida */
#define SEGUNDOS_DE_REVIDE      7.0f
#define ESFORCO_NO_REVIDE       0.05f

/*
 * ajustarPelotao
 * O esforço e o motor de cada rival: o pelotão (a distância até o jogador) e o revide (quem o
 * jogador acabou de passar). Depois da chegada do jogador, ninguém se ajusta.
 * Recebe: a corrida e o tempo do quadro.
 * Devolve: nada.
 */
static void ajustarPelotao(Corrida *corrida, float tempoQuadro) {
    const FichaDoNivel *ficha = fichaDoNivel(corrida->nivel);
    float forca = corrida->terminou ? 0.0f : ficha->pelotao;
    for (int i = 0; i < TOTAL_RIVAIS; i++) {
        Carro *rival = &corrida->rivais[i];
        float diferenca = (rival->distanciaPercorrida - corrida->jogador.distanciaPercorrida) / UNIDADES_POR_METRO;

        /* O revide: estava na frente, o jogador passou, e de perto */
        int aFrente = diferenca > 0.0f;
        if (rival->aFrenteDoJogador && !aFrente && diferenca > -PERTO_PARA_REVIDAR && !corrida->terminou
            && !carroNoPit(rival) && !carroNoPit(&corrida->jogador)) {
            rival->revide = SEGUNDOS_DE_REVIDE * rival->vontadeDeRevide;
        }
        rival->aFrenteDoJogador = aFrente;
        rival->revide = fmaxf(0.0f, rival->revide - tempoQuadro);
        if (aFrente) {
            rival->revide = 0.0f;   /* conseguiu: volta ao normal */
        }

        /* O relógio da defesa (ia.c): segurando a linha, e depois esperando para defender de novo */
        if (rival->defendendo > 0.0f) {
            rival->defendendo -= tempoQuadro;
            if (rival->defendendo <= 0.0f) {
                rival->defendendo = -ESPERA_PARA_DEFENDER;
            }
        } else if (rival->defendendo < 0.0f) {
            rival->defendendo = fminf(0.0f, rival->defendendo + tempoQuadro);
        }

        float ritmo = 1.0f, esforco = 0.0f;
        if (diferenca > FOLGA_NA_FRENTE) {
            ritmo -= MAIOR_ESPERA * forca * fminf((diferenca - FOLGA_NA_FRENTE) / METROS_ATE_O_AJUSTE_TODO, 1.0f);
        } else if (diferenca < -FOLGA_ATRAS) {
            esforco += MAIOR_ESFORCO * forca * fminf((-diferenca - FOLGA_ATRAS) / METROS_ATE_O_AJUSTE_TODO, 1.0f);
        }
        if (rival->revide > 0.0f) {
            esforco = fmaxf(esforco, ESFORCO_NO_REVIDE * rival->vontadeDeRevide);
        }
        float passo = MUDANCA_DO_RITMO * tempoQuadro;
        rival->ritmoDoPelotao += fmaxf(fminf(ritmo - rival->ritmoDoPelotao, passo), -passo);
        passo = MUDANCA_DO_ESFORCO * tempoQuadro;
        rival->esforco += fmaxf(fminf(esforco - rival->esforco, passo), -passo);
    }
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
 *
 * Numa corrida em rede há dois desvios (veja os campos de rede em corrida.h):
 *   - no DONO, os carros dos outros humanos são movidos pelos comandos que chegaram
 *     pela rede, no lugar da IA;
 *   - no CONVIDADO, só o carro local é movido aqui, e sai pelo caminho curto: quem
 *     move os outros 13 é online.c, com o retrato que o dono mandou.
 * Recebe: a corrida, os comandos do jogador e o tempo do quadro.
 * Devolve: nada.
 */
void atualizarCorrida(Corrida *corrida, Comandos comandosJogador, float tempoQuadro) {
    if (corrida->contagemRegressiva > 0.0f) {
        /* Em rede a contagem vem do retrato do dono, não do relógio desta máquina:
         * GetFrameTime() não dá o mesmo número em dois PCs, e integrar aqui faria as
         * largadas e os tempos de volta divergirem devagar e para sempre */
        if (!corrida->emRede || corrida->souODono) {
            corrida->contagemRegressiva -= tempoQuadro;
        }
        /* Parado no grid: acelerar só sobe o conta-giros (e acende as lanternas) */
        corrida->jogador.ultimosComandos = comandosJogador;
        atualizarMotor(&corrida->jogador.motor, 0.0f, comandosJogador.acelerar && !comandosJogador.frear, 0, 0, tempoQuadro);
        return;
    }

    if (!corrida->emRede || corrida->souODono) {
        corrida->tempoDeCorrida += tempoQuadro;
    }
    /* olharEmVolta só alimenta a IA: no convidado não roda IA nenhuma */
    if (!corrida->emRede || corrida->souODono) {
        olharEmVolta(corrida);
    }
    if (corrida->terminou) {
        comandosJogador = atualizarIA(&corrida->jogador, corrida->pista);
        corrida->jogador.controladoPeloJogador = 0;   /* agora quem dirige é a IA */
        corrida->jogador.re = 0.0f;
    }

    /* O contato (faíscas e som de lata) vale só para este quadro: o muro e as batidas o preenchem */
    for (int i = -1; i < TOTAL_RIVAIS; i++) {
        Carro *carro = (i < 0) ? &corrida->jogador : &corrida->rivais[i];
        carro->contato = 0.0f;
        carro->contatoComCarro = 0;
        carro->contatoDeslize = 0.0f;
    }

    /* 0) Vácuo, antes de mover: todos medem com os carros no mesmo lugar */
    if (!corrida->emRede || corrida->souODono) {
        atualizarVacuo(corrida, tempoQuadro);
    }

    /* 1) Jogador */
    int cruzou = moverCarro(corrida, &corrida->jogador, comandosJogador, tempoQuadro);
    atualizarSetores(corrida, &corrida->jogador, cruzou);
    if (cruzou) {
        registrarVolta(corrida, &corrida->jogador);
    }

    /* O caminho curto do convidado: ele move só o próprio carro (previsão local, para o
     * volante não ter atraso) e para aqui. Mover os rivais seria inventar uma corrida
     * paralela; julgar batidas, uma briga com o dono; e ordenar a classificação, ordenar
     * dados velhos. Tudo isso é online.c, com o retrato que chegou. */
    if (corrida->emRede && !corrida->souODono) {
        return;
    }

    /* 2') Os outros humanos: eles não têm IA. Quem dirige cada um é o comando que chegou
     * pela rede neste quadro — ou o anterior, se o pacote atrasou (online.c repete), ou a
     * IA, se o jogador sumiu de vez ou já cruzou a linha. */
    for (int h = 0; h < corrida->totalDeHumanos; h++) {
        if (h == corrida->euSou || corrida->humanos[h] == NULL) {
            continue;   /* esse é o carro do jogador, já movido no item 1 */
        }
        Carro *dele = corrida->humanos[h];
        Comandos comandos = corrida->comandosDosHumanos[h];
        if (corrida->segundosSemComandos[h] > SEGUNDOS_ATE_A_IA_ASSUMIR || dele->tempoDeChegada > 0.0f) {
            /* Sem mexer em direcaoLivre: a IA já dirige o carro do jogador em rumo livre
             * depois da bandeirada (veja o bloco de corrida->terminou), e isso funciona. */
            dele->controladoPeloJogador = 0;
            dele->re = 0.0f;
            comandos = atualizarIA(dele, corrida->pista);
        }
        cruzou = moverCarro(corrida, dele, comandos, tempoQuadro);
        atualizarSetores(corrida, dele, cruzou);
        if (cruzou) {
            registrarVolta(corrida, dele);
        }
    }

    /* 2) Rivais: o pelotão se ajusta ao jogador, e a IA olha o carro logo à frente de cada um */
    ajustarPelotao(corrida, tempoQuadro);
    for (int i = 0; i < TOTAL_RIVAIS; i++) {
        Carro *rival = &corrida->rivais[i];
        if (carroEDeHumano(corrida, rival)) {
            continue;   /* já foi movido acima, pelos comandos que vieram da rede */
        }
        Comandos comandosIA = atualizarIA(rival, corrida->pista);
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
 * jogadorNoPodio
 * Recebe: a corrida.
 * Devolve: 1 se o jogador já cruzou a linha final em 1º, 2º ou 3º lugar (e não foi desclassificado), 0 se não.
 */
int jogadorNoPodio(Corrida *corrida) {
    return corrida->terminou && !corrida->desclassificado && corrida->posicaoFinal >= 1 && corrida->posicaoFinal <= 3;
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
