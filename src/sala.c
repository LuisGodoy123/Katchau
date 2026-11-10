/*
 * sala.c
 * ------------------------------------------------------------------
 * As salas do modo online. Veja sala.h.
 *
 * O corpo de cada pacote está escrito campo por campo nas funções
 * "montar..." e "tratar...". Os tamanhos estão em #define ao lado de cada
 * par, e todo "tratar" confere o tamanho do datagrama ANTES de ler o corpo:
 * um pacote curto (estragado na rede, ou de um programa qualquer que caiu na
 * nossa porta) não pode fazer o jogo ler memória que não é dele.
 * ------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sala.h"
#include "online.h"   /* os pacotes da corrida chegam no mesmo soquete: veja esvaziarACaixa */

/* ---------- Ritmo dos pacotes periódicos ---------- */
#define SEGUNDOS_ENTRE_FAROIS      0.5f    /* o dono anuncia a sala 2x por segundo */
#define SEGUNDOS_ENTRE_PROCURAS    1.0f    /* o convidado pergunta 1x por segundo */
#define SEGUNDOS_ENTRE_ESTADOS     0.2f    /* o dono manda a sala inteira 5x por segundo */
#define SEGUNDOS_ENTRE_PINGS       1.0f
#define SEGUNDOS_SEM_FAROL         3.0f    /* sala calada por mais que isso sai da lista */
#define SEGUNDOS_PARA_DESISTIR     5.0f    /* o pedido de entrar não foi respondido */
#define SEGUNDOS_SEM_O_DONO        3.0f    /* sem ouvir o dono: ele saiu */
#define SEGUNDOS_PARA_SOLTAR_VAGA  5.0f    /* convidado calado: a vaga é liberada na sala de espera */
#define SEGUNDOS_DA_CONTAGEM       3.0f    /* do LARGAR até a corrida: dá tempo do pacote chegar a todos */
#define SEGUNDOS_DO_RECADO         4.0f
#define VEZES_QUE_REPETE_A_LARGADA 30      /* quadros repetindo o pacote de largada: perder 30 seguidos é improvável */

/* ---------- Tamanhos dos corpos (sem o cabeçalho) ---------- */
#define CORPO_DO_ANUNCIO   52
#define CORPO_DO_ENTRAR    28
#define CORPO_DO_ACEITOU    8
#define CORPO_DO_RECUSOU    4
#define CORPO_DA_SALA      (12 + MAXIMO_DE_JOGADORES * 24)
#define CORPO_DA_ESCOLHA    4
#define CORPO_DO_LARGAR    (12 + MAXIMO_DE_JOGADORES * 4)
#define CORPO_DO_SAIR       4
#define CORPO_DO_PING       8

/* ==================================================================
 * O estado
 * ================================================================== */

static Soquete *soquete = NULL;
static EstadoDaSala estado = SALA_DESLIGADA;
static char apelido[TAMANHO_DO_APELIDO];
static char recado[64];
static float tempoDoRecado = 0.0f;
static unsigned short sequencia = 0;

/* A lista da tela de salas */
static SalaNaRede salas[MAXIMO_DE_SALAS];
static int quantasSalas = 0;
static float desdeAProcura = 0.0f;

/* A sala em que estou (como dono ou como convidado) */
static unsigned int idDaSala = 0;
static char nomeDaSala[TAMANHO_DO_NOME_DA_SALA];
static JogadorDaSala jogadores[MAXIMO_DE_JOGADORES];
static int minhaVaga = -1;
static int pistaDaProva = 0;
static int nivelDaProva = 0;
static int carroQueQuero = 0;        /* no convidado é o PEDIDO; o dono confirma no pacote da sala */
static int souODono = 0;
static EnderecoDaRede doDono;
static float desdeOFarol = 0.0f, desdeOEstado = 0.0f, desdeOPing = 0.0f;
static float semOuvirODono = 0.0f, esperandoResposta = 0.0f;
static float contagem = -1.0f;       /* > 0: contando para largar */
static unsigned int pingMandadoEm = 0;

/* A largada */
static FormacaoOnline formacao;
static int vaiLargar = 0;            /* 1 até o main.c pegar com aSalaLargou() */
static int repetindoLargada = 0;

/*
 * dizer
 * Guarda um recado curto para a tela mostrar ("SALA CHEIA", "O DONO SAIU"...).
 * Recebe: o texto.
 * Devolve: nada.
 */
static void dizer(const char *texto) {
    snprintf(recado, sizeof(recado), "%s", texto);
    tempoDoRecado = SEGUNDOS_DO_RECADO;
}

/*
 * limparJogadores
 * Esvazia as vagas da sala.
 */
static void limparJogadores(void) {
    for (int i = 0; i < MAXIMO_DE_JOGADORES; i++) {
        memset(&jogadores[i], 0, sizeof(jogadores[i]));
    }
}

/*
 * carroEstaLivrePara
 * Recebe: a vaga que quer o carro e o número do carro.
 * Devolve: 1 se nenhuma OUTRA vaga já está com ele.
 */
static int carroEstaLivrePara(int vaga, int carro) {
    for (int i = 0; i < MAXIMO_DE_JOGADORES; i++) {
        if (jogadores[i].ligado && i != vaga && jogadores[i].numeroDoCarro == carro) {
            return 0;
        }
    }
    return 1;
}

/*
 * encostarAsVagas
 * Tira o buraco que um convidado deixa ao sair: as vagas seguintes descem uma
 * casa, e a sala volta a ser 0, 1, 2, 3 sem falhas.
 *
 * Isso não é arrumação: a formação da corrida é percorrida de 0 até
 * totalDeJogadores, e um buraco no meio viraria um CARRO FANTASMA no grid (vaga
 * vazia = carro 0 no lugar 0). Encostar as vagas aqui faz o índice da formação
 * ser sempre o mesmo que a vaga da sala, em todas as máquinas.
 *
 * O dono é sempre a vaga 0 e nunca se mexe; os convidados são de 1 para cima.
 * Recebe: nada.
 * Devolve: nada.
 */
static void encostarAsVagas(void) {
    for (int i = 1; i < MAXIMO_DE_JOGADORES; i++) {
        if (jogadores[i].ligado) {
            continue;
        }
        for (int j = i; j < MAXIMO_DE_JOGADORES - 1; j++) {
            jogadores[j] = jogadores[j + 1];
        }
        memset(&jogadores[MAXIMO_DE_JOGADORES - 1], 0, sizeof(jogadores[0]));
    }
}

void iniciarSala(void) {
    nomeDoJogadorDoSistema(apelido, TAMANHO_DO_APELIDO);
    soquete = abrirSoquete();
    estado = SALA_DESLIGADA;
    recado[0] = '\0';
    limparJogadores();
    quantasSalas = 0;
    memset(&formacao, 0, sizeof(formacao));
    if (soquete == NULL) {
        dizer("A REDE NAO ABRIU");
    }
}

void encerrarSala(void) {
    if (estado == SALA_SOU_CONVIDADO || estado == SALA_SOU_DONO || estado == SALA_NA_CORRIDA) {
        sairDaSala();
    }
    fecharSoquete(soquete);
    soquete = NULL;
    estado = SALA_DESLIGADA;
}

EstadoDaSala estadoDaSala(void) { return estado; }
const char *recadoDaSala(void) { return (tempoDoRecado > 0.0f) ? recado : ""; }
const char *meuApelido(void) { return apelido; }
int aRedeAbriu(void) { return soquete != NULL; }
int souODonoDaSala(void) { return souODono; }
int minhaVagaNaSala(void) { return minhaVaga; }
int pistaDaSala(void) { return pistaDaProva; }
int nivelDaSala(void) { return nivelDaProva; }
float contagemParaLargar(void) { return contagem; }
Soquete *soqueteDaSala(void) { return soquete; }
unsigned short proximaSequencia(void) { return ++sequencia; }
const EnderecoDaRede *enderecoDoDono(void) { return &doDono; }
float segundosSemOuvirODono(void) { return semOuvirODono; }
int totalDeSalasAchadas(void) { return quantasSalas; }

const SalaNaRede *salaAchada(int i) {
    return (i >= 0 && i < quantasSalas) ? &salas[i] : NULL;
}

const JogadorDaSala *jogadorDaSala(int vaga) {
    return (vaga >= 0 && vaga < MAXIMO_DE_JOGADORES) ? &jogadores[vaga] : NULL;
}

int totalDeJogadoresDaSala(void) {
    int quantos = 0;
    for (int i = 0; i < MAXIMO_DE_JOGADORES; i++) {
        quantos += jogadores[i].ligado ? 1 : 0;
    }
    return quantos;
}

int emCorridaOnline(void) { return estado == SALA_NA_CORRIDA; }
const FormacaoOnline *formacaoDaSala(void) { return &formacao; }

int enderecoDoConvidado(int vaga, EnderecoDaRede *destino) {
    if (vaga < 0 || vaga >= MAXIMO_DE_JOGADORES || !jogadores[vaga].ligado || vaga == minhaVaga) {
        return 0;
    }
    *destino = jogadores[vaga].endereco;
    return 1;
}

/* ==================================================================
 * Mandar
 * ================================================================== */

/*
 * mandar
 * Fecha o pacote (o cabeçalho já está nele) e manda para um endereço.
 * Recebe: o fio, quantos bytes tem e o destino.
 * Devolve: nada.
 */
static void mandar(const unsigned char *fio, int tamanho, const EnderecoDaRede *destino) {
    if (soquete != NULL) {
        enviarPara(soquete, destino, fio, tamanho);
    }
}

/*
 * gritar
 * Manda para a rede toda, nas dez portas da faixa (veja gritarNaRede).
 */
static void gritar(const unsigned char *fio, int tamanho) {
    if (soquete != NULL) {
        gritarNaRede(soquete, fio, tamanho);
    }
}

/*
 * mandarParaTodosOsConvidados
 * O dono manda o mesmo pacote para cada convidado ligado.
 */
static void mandarParaTodosOsConvidados(const unsigned char *fio, int tamanho) {
    for (int i = 0; i < MAXIMO_DE_JOGADORES; i++) {
        if (jogadores[i].ligado && i != minhaVaga) {
            mandar(fio, tamanho, &jogadores[i].endereco);
        }
    }
}

static void mandarProcurar(void) {
    unsigned char fio[BYTES_DO_CABECALHO];
    int tamanho = escreverCabecalho(fio, PACOTE_PROCURAR, proximaSequencia());
    gritar(fio, tamanho);
}

/*
 * mandarAnuncio
 * O farol da sala: quem está procurando monta a lista com isto.
 * Recebe: para quem mandar (NULL = para a rede toda).
 * Devolve: nada.
 */
static void mandarAnuncio(const EnderecoDaRede *destino) {
    unsigned char fio[BYTES_DO_CABECALHO + CORPO_DO_ANUNCIO];
    int onde = escreverCabecalho(fio, PACOTE_ANUNCIO, proximaSequencia());
    escreverU32(fio, &onde, idDaSala);
    escreverTexto(fio, &onde, nomeDaSala, TAMANHO_DO_NOME_DA_SALA);
    escreverTexto(fio, &onde, apelido, TAMANHO_DO_APELIDO);
    escreverU8(fio, &onde, (unsigned int) pistaDaProva);
    escreverU8(fio, &onde, (unsigned int) nivelDaProva);
    escreverU8(fio, &onde, (unsigned int) fichaDoNivel((NivelDaCorrida) nivelDaProva)->voltas);
    escreverU8(fio, &onde, (unsigned int) totalDeJogadoresDaSala());
    escreverU8(fio, &onde, MAXIMO_DE_JOGADORES);
    escreverU8(fio, &onde, (unsigned int) (estado == SALA_NA_CORRIDA));
    escreverU16(fio, &onde, 0);   /* reservado */
    if (destino != NULL) {
        mandar(fio, onde, destino);
    } else {
        gritar(fio, onde);
    }
}

static void mandarEntrar(void) {
    unsigned char fio[BYTES_DO_CABECALHO + CORPO_DO_ENTRAR];
    int onde = escreverCabecalho(fio, PACOTE_ENTRAR, proximaSequencia());
    escreverU32(fio, &onde, idDaSala);
    escreverU32(fio, &onde, MARCA_DA_COMPILACAO);
    escreverTexto(fio, &onde, apelido, TAMANHO_DO_APELIDO);
    escreverU8(fio, &onde, (unsigned int) carroQueQuero);
    escreverU8(fio, &onde, 0);
    escreverU16(fio, &onde, 0);
    mandar(fio, onde, &doDono);
}

static void mandarAceitou(int vaga, const EnderecoDaRede *destino) {
    unsigned char fio[BYTES_DO_CABECALHO + CORPO_DO_ACEITOU];
    int onde = escreverCabecalho(fio, PACOTE_ACEITOU, proximaSequencia());
    escreverU32(fio, &onde, idDaSala);
    escreverU8(fio, &onde, (unsigned int) vaga);
    escreverU8(fio, &onde, (unsigned int) jogadores[vaga].numeroDoCarro);
    escreverU8(fio, &onde, MAXIMO_DE_JOGADORES);
    escreverU8(fio, &onde, 0);
    mandar(fio, onde, destino);
}

static void mandarRecusou(MotivoDaRecusa motivo, const EnderecoDaRede *destino) {
    unsigned char fio[BYTES_DO_CABECALHO + CORPO_DO_RECUSOU];
    int onde = escreverCabecalho(fio, PACOTE_RECUSOU, proximaSequencia());
    escreverU8(fio, &onde, (unsigned int) motivo);
    escreverU8(fio, &onde, 0);
    escreverU16(fio, &onde, 0);
    mandar(fio, onde, destino);
}

/*
 * mandarEstadoDaSala
 * A sala inteira, do dono para todos: é a VERDADE sobre quem está, com qual
 * carro e quem já está pronto. Vai 5 vezes por segundo e é só estado (não é
 * um aviso de mudança), então perder um pacote não perde nada: o próximo
 * conta a mesma história. É por isso que não existe retransmissão aqui.
 */
static void mandarEstadoDaSala(void) {
    /* Um pacote por convidado, porque cada um leva A VAGA DELE: as vagas se mexem
     * quando alguém sai (veja encostarAsVagas), e o convidado precisa saber a nova
     * — é por ela que ele se acha na formação da corrida. São 116 bytes por
     * convidado, 5 vezes por segundo: não vale a pena economizar. */
    for (int quem = 1; quem < MAXIMO_DE_JOGADORES; quem++) {
        if (!jogadores[quem].ligado) {
            continue;
        }
        unsigned char fio[BYTES_DO_CABECALHO + CORPO_DA_SALA];
        int onde = escreverCabecalho(fio, PACOTE_SALA, proximaSequencia());
        escreverU32(fio, &onde, idDaSala);
        escreverU8(fio, &onde, (unsigned int) pistaDaProva);
        escreverU8(fio, &onde, (unsigned int) nivelDaProva);
        escreverU8(fio, &onde, (unsigned int) quem);   /* "a sua vaga é esta" */
        escreverU8(fio, &onde, (unsigned int) (estado == SALA_NA_CORRIDA));
        escreverF32(fio, &onde, contagem);
        for (int i = 0; i < MAXIMO_DE_JOGADORES; i++) {
            escreverU8(fio, &onde, (unsigned int) i);
            escreverU8(fio, &onde, (unsigned int) jogadores[i].numeroDoCarro);
            escreverU8(fio, &onde, (unsigned int) (jogadores[i].pronto != 0));
            escreverU8(fio, &onde, (unsigned int) (jogadores[i].ligado != 0));
            escreverTexto(fio, &onde, jogadores[i].nome, TAMANHO_DO_APELIDO);
            escreverU16(fio, &onde, jogadores[i].ping);
            escreverU16(fio, &onde, 0);
        }
        mandar(fio, onde, &jogadores[quem].endereco);
    }
}

static void mandarEscolha(void) {
    unsigned char fio[BYTES_DO_CABECALHO + CORPO_DA_ESCOLHA];
    int onde = escreverCabecalho(fio, PACOTE_ESCOLHA, proximaSequencia());
    escreverU8(fio, &onde, (unsigned int) carroQueQuero);
    escreverU8(fio, &onde, (unsigned int) (minhaVaga >= 0 && jogadores[minhaVaga].pronto));
    escreverU8(fio, &onde, (unsigned int) (minhaVaga < 0 ? 0 : minhaVaga));
    escreverU8(fio, &onde, 0);
    mandar(fio, onde, &doDono);
}

/*
 * mandarLargada
 * O pacote que começa a corrida. Leva a formação inteira (quem tem qual carro e
 * qual lugar do grid) e a semente do sorteio, para TODAS as máquinas montarem
 * exatamente o mesmo grid, com os mesmos rivais de IA e os mesmos ritmos.
 */
static void mandarLargada(void) {
    unsigned char fio[BYTES_DO_CABECALHO + CORPO_DO_LARGAR];
    int onde = escreverCabecalho(fio, PACOTE_LARGAR, proximaSequencia());
    escreverU32(fio, &onde, formacao.semente);
    escreverF32(fio, &onde, contagem);
    escreverU8(fio, &onde, (unsigned int) pistaDaProva);
    escreverU8(fio, &onde, (unsigned int) nivelDaProva);
    escreverU8(fio, &onde, (unsigned int) formacao.totalDeJogadores);
    escreverU8(fio, &onde, 0);
    for (int i = 0; i < MAXIMO_DE_JOGADORES; i++) {
        escreverU8(fio, &onde, (unsigned int) i);
        escreverU8(fio, &onde, (unsigned int) formacao.numeroDoCarro[i]);
        escreverU8(fio, &onde, (unsigned int) formacao.lugarNoGrid[i]);
        escreverU8(fio, &onde, (unsigned int) (jogadores[i].ligado != 0));
    }
    mandarParaTodosOsConvidados(fio, onde);
}

static void mandarSaida(void) {
    unsigned char fio[BYTES_DO_CABECALHO + CORPO_DO_SAIR];
    int onde = escreverCabecalho(fio, PACOTE_SAIR, proximaSequencia());
    escreverU8(fio, &onde, (unsigned int) (minhaVaga < 0 ? 0 : minhaVaga));
    escreverU8(fio, &onde, 0);
    escreverU16(fio, &onde, 0);
    if (souODono) {
        mandarParaTodosOsConvidados(fio, onde);
    } else {
        mandar(fio, onde, &doDono);
    }
}

static void mandarPing(TipoDePacote tipo, unsigned int marcaDeTempo, const EnderecoDaRede *destino) {
    unsigned char fio[BYTES_DO_CABECALHO + CORPO_DO_PING];
    int onde = escreverCabecalho(fio, tipo, proximaSequencia());
    escreverU32(fio, &onde, marcaDeTempo);
    escreverU8(fio, &onde, (unsigned int) (minhaVaga < 0 ? 0 : minhaVaga));
    escreverU8(fio, &onde, 0);
    escreverU16(fio, &onde, 0);
    mandar(fio, onde, destino);
}

/* ==================================================================
 * A lista de salas
 * ================================================================== */

void comecarAProcurar(void) {
    if (soquete == NULL) {
        dizer("A REDE NAO ABRIU");
        estado = SALA_DEU_ERRADO;
        return;
    }
    estado = SALA_PROCURANDO;
    quantasSalas = 0;
    souODono = 0;
    minhaVaga = -1;
    desdeAProcura = SEGUNDOS_ENTRE_PROCURAS;   /* pergunta já no primeiro quadro */
    mandarProcurar();
}

void pararDeProcurar(void) {
    if (estado == SALA_PROCURANDO) {
        estado = SALA_DESLIGADA;
    }
}

/*
 * acharOuAbrirLinha
 * A linha da lista de uma sala, pelo id dela. Sala nova entra no fim; se a lista
 * está cheia, a nova é ignorada (12 salas numa rede doméstica já é muito).
 * Recebe: o id da sala.
 * Devolve: a linha, ou NULL se não cabe.
 */
static SalaNaRede *acharOuAbrirLinha(unsigned int id) {
    for (int i = 0; i < quantasSalas; i++) {
        if (salas[i].id == id) {
            return &salas[i];
        }
    }
    if (quantasSalas >= MAXIMO_DE_SALAS) {
        return NULL;
    }
    SalaNaRede *nova = &salas[quantasSalas];
    memset(nova, 0, sizeof(*nova));
    quantasSalas++;
    return nova;
}

/*
 * esquecerSalasCaladas
 * Tira da lista a sala que parou de anunciar: o dono fechou o jogo, ou saiu do
 * alcance. Sem isto, a lista encheria de salas que não existem mais.
 */
static void esquecerSalasCaladas(float tempoQuadro) {
    for (int i = 0; i < quantasSalas; i++) {
        salas[i].semFarol += tempoQuadro;
        if (salas[i].semFarol > SEGUNDOS_SEM_FAROL) {
            for (int j = i; j < quantasSalas - 1; j++) {
                salas[j] = salas[j + 1];
            }
            quantasSalas--;
            i--;
        }
    }
}

void criarSala(int pista, int nivel, int carro) {
    if (soquete == NULL) {
        dizer("A REDE NAO ABRIU");
        estado = SALA_DEU_ERRADO;
        return;
    }
    /* O id separa duas salas no mesmo IP (duas janelas no mesmo PC) e faz um anúncio
     * velho, de uma sala que já fechou, não ser confundido com a nova */
    idDaSala = (unsigned int) (milissegundosDeAgora() * 2654435761u + (unsigned int) portaDoSoquete(soquete));
    snprintf(nomeDaSala, sizeof(nomeDaSala), "SALA DE %s", apelido);
    limparJogadores();
    souODono = 1;
    minhaVaga = 0;
    jogadores[0].ligado = 1;
    jogadores[0].pronto = 0;
    jogadores[0].numeroDoCarro = carro % TOTAL_CARROS;
    snprintf(jogadores[0].nome, TAMANHO_DO_APELIDO, "%s", apelido);
    carroQueQuero = jogadores[0].numeroDoCarro;
    pistaDaProva = pista % TOTAL_PISTAS;
    nivelDaProva = nivel % TOTAL_NIVEIS;
    contagem = -1.0f;
    vaiLargar = 0;
    estado = SALA_SOU_DONO;
    desdeOFarol = SEGUNDOS_ENTRE_FAROIS;
    mandarAnuncio(NULL);
}

void entrarNaSalaAchada(int i, int carro) {
    const SalaNaRede *sala = salaAchada(i);
    if (sala == NULL || soquete == NULL) {
        return;
    }
    if (sala->versaoDiferente) {
        dizer("OUTRA VERSAO DO JOGO");
        return;
    }
    if (sala->emCorrida) {
        dizer("A CORRIDA JA COMECOU");
        return;
    }
    if (sala->jogadores >= sala->limite) {
        dizer("SALA CHEIA");
        return;
    }
    idDaSala = sala->id;
    doDono = sala->endereco;
    snprintf(nomeDaSala, sizeof(nomeDaSala), "%s", sala->nome);
    pistaDaProva = sala->pista;
    nivelDaProva = sala->nivel;
    carroQueQuero = carro % TOTAL_CARROS;
    souODono = 0;
    minhaVaga = -1;
    limparJogadores();
    contagem = -1.0f;
    vaiLargar = 0;
    semOuvirODono = 0.0f;
    esperandoResposta = 0.0f;
    estado = SALA_ENTRANDO;
    mandarEntrar();
}

/* ==================================================================
 * A sala de espera
 * ================================================================== */

void sairDaSala(void) {
    if (estado == SALA_SOU_DONO || estado == SALA_SOU_CONVIDADO || estado == SALA_NA_CORRIDA
        || estado == SALA_ENTRANDO) {
        mandarSaida();
    }
    souODono = 0;
    minhaVaga = -1;
    limparJogadores();
    contagem = -1.0f;
    vaiLargar = 0;
    estado = SALA_DESLIGADA;
}

void escolherPistaDaSala(int pista) {
    if (souODono && estado == SALA_SOU_DONO && contagem < 0.0f) {
        pistaDaProva = ((pista % TOTAL_PISTAS) + TOTAL_PISTAS) % TOTAL_PISTAS;
    }
}

void escolherNivelDaSala(int nivel) {
    if (souODono && estado == SALA_SOU_DONO && contagem < 0.0f) {
        nivelDaProva = ((nivel % TOTAL_NIVEIS) + TOTAL_NIVEIS) % TOTAL_NIVEIS;
    }
}

int carroEstaLivreNaSala(int carro) {
    for (int i = 0; i < MAXIMO_DE_JOGADORES; i++) {
        if (jogadores[i].ligado && i != minhaVaga && jogadores[i].numeroDoCarro == carro) {
            return 0;
        }
    }
    return 1;
}

/*
 * escolherCarroDaSala
 * No dono, troca o carro na hora (se estiver livre). No convidado é só um PEDIDO:
 * ele guarda o desejo, o pacote de escolha o leva, e o carro de verdade é o que
 * vier no pacote da sala. É o que impede dois jogadores de acharem que ficaram
 * com o mesmo carro — não existe empate quando só um lado decide.
 * Recebe: o número do carro.
 * Devolve: nada.
 */
void escolherCarroDaSala(int carro) {
    carro = ((carro % TOTAL_CARROS) + TOTAL_CARROS) % TOTAL_CARROS;
    carroQueQuero = carro;
    if (souODono && minhaVaga >= 0 && carroEstaLivreNaSala(carro)) {
        jogadores[minhaVaga].numeroDoCarro = carro;
    }
}

void marcarProntoNaSala(int pronto) {
    if (minhaVaga >= 0) {
        jogadores[minhaVaga].pronto = pronto ? 1 : 0;
    }
}

int estouProntoNaSala(void) {
    return (minhaVaga >= 0) ? jogadores[minhaVaga].pronto : 0;
}

int todosProntosNaSala(void) {
    int algum = 0;
    for (int i = 0; i < MAXIMO_DE_JOGADORES; i++) {
        if (jogadores[i].ligado) {
            algum = 1;
            if (!jogadores[i].pronto) {
                return 0;
            }
        }
    }
    return algum;
}

int podeLargar(void) {
    return souODono && estado == SALA_SOU_DONO && contagem < 0.0f && todosProntosNaSala();
}

/*
 * montarFormacao
 * Quem corre com qual carro e de qual lugar do grid.
 *
 * Os humanos pegam os ÚLTIMOS lugares (13, 12, 11, 10), na ordem em que entraram
 * na sala: ninguém larga na primeira fila de graça, e isso mantém o espírito do
 * jogo de um jogador, onde o jogador também larga em último.
 * Recebe: nada (lê as vagas da sala).
 * Devolve: nada (preenche a formação).
 */
static void montarFormacao(void) {
    memset(&formacao, 0, sizeof(formacao));
    /* A semente sai do id da sala, e não do relógio: o id é o MESMO em todas as
     * máquinas, então todas sorteiam os mesmos ritmos para os mesmos rivais de IA.
     * Usar o relógio daria um pelotão diferente em cada tela. */
    formacao.semente = idDaSala;
    formacao.souODono = souODono;
    formacao.euSou = (minhaVaga >= 0) ? minhaVaga : 0;
    encostarAsVagas();   /* garante vagas 0..n-1 sem buraco: veja o comentário dela */
    int quantos = 0;
    for (int i = 0; i < MAXIMO_DE_JOGADORES; i++) {
        if (!jogadores[i].ligado) {
            break;   /* sem buracos: a primeira vaga vazia é o fim da lista */
        }
        formacao.numeroDoCarro[i] = jogadores[i].numeroDoCarro;
        /* Os humanos pegam os ÚLTIMOS lugares do grid, na ordem da sala */
        formacao.lugarNoGrid[i] = TOTAL_CARROS - 1 - i;
        quantos++;
    }
    formacao.totalDeJogadores = quantos;
}

void mandarLargar(void) {
    if (!podeLargar()) {
        return;
    }
    contagem = SEGUNDOS_DA_CONTAGEM;
    montarFormacao();
    repetindoLargada = VEZES_QUE_REPETE_A_LARGADA;
}

int aSalaLargou(void) {
    if (!vaiLargar) {
        return 0;
    }
    vaiLargar = 0;   /* só uma vez: quem pergunta é o main.c, no quadro da troca de tela */
    return 1;
}

void acabouACorridaOnline(void) {
    if (estado == SALA_NA_CORRIDA) {
        sairDaSala();
    }
}

/* ==================================================================
 * Receber
 * ================================================================== */

static void tratarProcurar(const EnderecoDaRede *de) {
    /* Só o dono responde, e responde direto para quem perguntou: assim a resposta
     * não depende do broadcast voltar, que é justamente o que o firewall bloqueia */
    if (souODono && (estado == SALA_SOU_DONO || estado == SALA_NA_CORRIDA)) {
        mandarAnuncio(de);
    }
}

static void tratarAnuncio(const unsigned char *fio, int tamanho, const CabecalhoDeRede *cabecalho,
                          const EnderecoDaRede *de) {
    if (tamanho < BYTES_DO_CABECALHO + CORPO_DO_ANUNCIO) {
        return;
    }
    int onde = BYTES_DO_CABECALHO;
    unsigned int id = lerU32(fio, &onde);
    if (estado != SALA_PROCURANDO || id == idDaSala) {
        return;   /* a própria sala não entra na lista */
    }
    SalaNaRede *linha = acharOuAbrirLinha(id);
    if (linha == NULL) {
        return;
    }
    linha->id = id;
    lerTexto(fio, &onde, linha->nome, TAMANHO_DO_NOME_DA_SALA);
    lerTexto(fio, &onde, linha->dono, TAMANHO_DO_APELIDO);
    /* Todo índice que vem do fio é preso na faixa: um pacote estragado não pode
     * virar um acesso fora do vetor de pistas ou de níveis */
    linha->pista = (int) (lerU8(fio, &onde) % TOTAL_PISTAS);
    linha->nivel = (int) (lerU8(fio, &onde) % TOTAL_NIVEIS);
    linha->voltas = (int) lerU8(fio, &onde);
    linha->jogadores = (int) lerU8(fio, &onde);
    linha->limite = (int) lerU8(fio, &onde);
    linha->emCorrida = (int) lerU8(fio, &onde);
    linha->versaoDiferente = (cabecalho->versao != VERSAO_DO_PROTOCOLO);
    linha->endereco = *de;
    linha->semFarol = 0.0f;
}

static void tratarEntrar(const unsigned char *fio, int tamanho, const CabecalhoDeRede *cabecalho,
                         const EnderecoDaRede *de) {
    if (tamanho < BYTES_DO_CABECALHO + CORPO_DO_ENTRAR || !souODono) {
        return;
    }
    int onde = BYTES_DO_CABECALHO;
    unsigned int id = lerU32(fio, &onde);
    unsigned int marca = lerU32(fio, &onde);
    char nome[TAMANHO_DO_APELIDO];
    lerTexto(fio, &onde, nome, TAMANHO_DO_APELIDO);
    int carro = (int) (lerU8(fio, &onde) % TOTAL_CARROS);
    if (id != idDaSala) {
        return;   /* é para outra sala (outra janela no mesmo PC) */
    }
    if (cabecalho->versao != VERSAO_DO_PROTOCOLO || marca != MARCA_DA_COMPILACAO) {
        mandarRecusou(RECUSA_VERSAO, de);
        return;
    }
    if (estado == SALA_NA_CORRIDA || contagem >= 0.0f) {
        mandarRecusou(RECUSA_EM_CORRIDA, de);
        return;
    }

    /* Já está dentro? O pedido se repete até a resposta chegar, e aceitar duas vezes
     * daria duas vagas ao mesmo jogador */
    for (int i = 0; i < MAXIMO_DE_JOGADORES; i++) {
        if (jogadores[i].ligado && mesmoEndereco(&jogadores[i].endereco, de)) {
            mandarAceitou(i, de);
            return;
        }
    }
    /* A vaga 0 é do dono e nunca sai: os convidados vão de 1 para cima */
    int vaga = -1;
    for (int i = 1; i < MAXIMO_DE_JOGADORES && vaga < 0; i++) {
        vaga = jogadores[i].ligado ? vaga : i;
    }
    if (vaga < 0) {
        mandarRecusou(RECUSA_SALA_CHEIA, de);
        return;
    }
    jogadores[vaga].ligado = 1;
    jogadores[vaga].pronto = 0;
    jogadores[vaga].endereco = *de;
    jogadores[vaga].calado = 0.0f;
    jogadores[vaga].ping = 0;
    snprintf(jogadores[vaga].nome, TAMANHO_DO_APELIDO, "%s", nome[0] != '\0' ? nome : "PILOTO");
    /* O carro: o pedido, se estiver livre; senão o primeiro livre. Quem decide é o dono,
     * e é por isso que dois jogadores nunca ficam com o mesmo carro */
    jogadores[vaga].numeroDoCarro = carro;
    if (!carroEstaLivrePara(vaga, carro)) {
        for (int n = 0; n < TOTAL_CARROS; n++) {
            if (carroEstaLivrePara(vaga, n)) {
                jogadores[vaga].numeroDoCarro = n;
                break;
            }
        }
    }
    mandarAceitou(vaga, de);
    mandarEstadoDaSala();
}

static void tratarAceitou(const unsigned char *fio, int tamanho) {
    if (tamanho < BYTES_DO_CABECALHO + CORPO_DO_ACEITOU || estado != SALA_ENTRANDO) {
        return;
    }
    int onde = BYTES_DO_CABECALHO;
    unsigned int id = lerU32(fio, &onde);
    if (id != idDaSala) {
        return;
    }
    minhaVaga = (int) (lerU8(fio, &onde) % MAXIMO_DE_JOGADORES);
    carroQueQuero = (int) (lerU8(fio, &onde) % TOTAL_CARROS);
    estado = SALA_SOU_CONVIDADO;
    semOuvirODono = 0.0f;
    dizer("");
    tempoDoRecado = 0.0f;
}

static void tratarRecusou(const unsigned char *fio, int tamanho) {
    if (tamanho < BYTES_DO_CABECALHO + CORPO_DO_RECUSOU || estado != SALA_ENTRANDO) {
        return;
    }
    int onde = BYTES_DO_CABECALHO;
    int motivo = (int) lerU8(fio, &onde);
    if (motivo == RECUSA_SALA_CHEIA) {
        dizer("SALA CHEIA");
    } else if (motivo == RECUSA_EM_CORRIDA) {
        dizer("A CORRIDA JA COMECOU");
    } else {
        dizer("OUTRA VERSAO DO JOGO");
    }
    estado = SALA_PROCURANDO;   /* volta para a lista, com o recado na tela */
    minhaVaga = -1;
}

static void tratarEstadoDaSala(const unsigned char *fio, int tamanho) {
    if (tamanho < BYTES_DO_CABECALHO + CORPO_DA_SALA || souODono) {
        return;
    }
    if (estado != SALA_SOU_CONVIDADO && estado != SALA_NA_CORRIDA) {
        return;
    }
    int onde = BYTES_DO_CABECALHO;
    unsigned int id = lerU32(fio, &onde);
    if (id != idDaSala) {
        return;
    }
    pistaDaProva = (int) (lerU8(fio, &onde) % TOTAL_PISTAS);
    nivelDaProva = (int) (lerU8(fio, &onde) % TOTAL_NIVEIS);
    minhaVaga = (int) (lerU8(fio, &onde) % MAXIMO_DE_JOGADORES);   /* ela se mexe quando alguém sai */
    (void) lerU8(fio, &onde);   /* emCorrida: o pacote de largada é que manda largar */
    contagem = lerF32(fio, &onde);
    for (int i = 0; i < MAXIMO_DE_JOGADORES; i++) {
        (void) lerU8(fio, &onde);   /* a vaga: é a própria posição no vetor */
        int carro = (int) (lerU8(fio, &onde) % TOTAL_CARROS);
        int pronto = (int) lerU8(fio, &onde);
        int ligado = (int) lerU8(fio, &onde);
        char nome[TAMANHO_DO_APELIDO];
        lerTexto(fio, &onde, nome, TAMANHO_DO_APELIDO);
        unsigned int ping = lerU16(fio, &onde);
        (void) lerU16(fio, &onde);
        /* O que o dono diz é a verdade, inclusive sobre a MINHA vaga: é assim que o
         * seletor de carro "volta" na tela quando se pede um carro já tomado */
        jogadores[i].numeroDoCarro = carro;
        jogadores[i].ligado = ligado;
        jogadores[i].pronto = pronto;
        jogadores[i].ping = ping;
        snprintf(jogadores[i].nome, TAMANHO_DO_APELIDO, "%s", nome);
    }
    semOuvirODono = 0.0f;
}

static void tratarEscolha(const unsigned char *fio, int tamanho, const EnderecoDaRede *de) {
    if (tamanho < BYTES_DO_CABECALHO + CORPO_DA_ESCOLHA || !souODono || contagem >= 0.0f) {
        return;
    }
    int onde = BYTES_DO_CABECALHO;
    int carro = (int) (lerU8(fio, &onde) % TOTAL_CARROS);
    int pronto = (int) lerU8(fio, &onde);
    for (int i = 0; i < MAXIMO_DE_JOGADORES; i++) {
        if (!jogadores[i].ligado || !mesmoEndereco(&jogadores[i].endereco, de)) {
            continue;
        }
        jogadores[i].pronto = pronto ? 1 : 0;
        if (carroEstaLivrePara(i, carro)) {
            jogadores[i].numeroDoCarro = carro;   /* o pedido valia: o carro estava livre */
        }
        jogadores[i].calado = 0.0f;
        return;
    }
}

static void tratarLargada(const unsigned char *fio, int tamanho) {
    if (tamanho < BYTES_DO_CABECALHO + CORPO_DO_LARGAR || souODono) {
        return;
    }
    if (estado != SALA_SOU_CONVIDADO) {
        return;
    }
    int onde = BYTES_DO_CABECALHO;
    memset(&formacao, 0, sizeof(formacao));
    formacao.semente = lerU32(fio, &onde);
    contagem = lerF32(fio, &onde);
    pistaDaProva = (int) (lerU8(fio, &onde) % TOTAL_PISTAS);
    nivelDaProva = (int) (lerU8(fio, &onde) % TOTAL_NIVEIS);
    formacao.totalDeJogadores = (int) lerU8(fio, &onde);
    if (formacao.totalDeJogadores > MAXIMO_DE_JOGADORES) {
        formacao.totalDeJogadores = MAXIMO_DE_JOGADORES;
    }
    (void) lerU8(fio, &onde);
    for (int i = 0; i < MAXIMO_DE_JOGADORES; i++) {
        (void) lerU8(fio, &onde);   /* a vaga */
        formacao.numeroDoCarro[i] = (int) (lerU8(fio, &onde) % TOTAL_CARROS);
        formacao.lugarNoGrid[i] = (int) (lerU8(fio, &onde) % TOTAL_CARROS);
        jogadores[i].ligado = (int) lerU8(fio, &onde);
    }
    formacao.euSou = (minhaVaga >= 0) ? minhaVaga : 0;
    formacao.souODono = 0;
    estado = SALA_NA_CORRIDA;
    vaiLargar = 1;
}

static void tratarSaida(int tamanho, const EnderecoDaRede *de) {
    if (tamanho < BYTES_DO_CABECALHO + CORPO_DO_SAIR) {
        return;
    }
    if (souODono) {
        for (int i = 1; i < MAXIMO_DE_JOGADORES; i++) {
            if (jogadores[i].ligado && mesmoEndereco(&jogadores[i].endereco, de)) {
                memset(&jogadores[i], 0, sizeof(jogadores[i]));
                /* Na sala de espera as vagas encostam, para a formação não ficar com
                 * buraco. Na corrida NÃO: mexer nas vagas no meio da prova trocaria os
                 * carros de dono na cara do jogador. */
                if (estado == SALA_SOU_DONO) {
                    encostarAsVagas();
                }
                mandarEstadoDaSala();
                return;
            }
        }
        return;
    }
    if (mesmoEndereco(&doDono, de)) {
        perdiODono();
    }
}

static void tratarPing(const unsigned char *fio, int tamanho, const EnderecoDaRede *de) {
    if (tamanho < BYTES_DO_CABECALHO + CORPO_DO_PING) {
        return;
    }
    int onde = BYTES_DO_CABECALHO;
    unsigned int marcaDeTempo = lerU32(fio, &onde);
    mandarPing(PACOTE_PONGUE, marcaDeTempo, de);   /* o eco devolve o MESMO número: quem mediu é quem perguntou */
}

static void tratarPongue(const unsigned char *fio, int tamanho, const EnderecoDaRede *de) {
    if (tamanho < BYTES_DO_CABECALHO + CORPO_DO_PING) {
        return;
    }
    int onde = BYTES_DO_CABECALHO;
    unsigned int marcaDeTempo = lerU32(fio, &onde);
    unsigned int ida = milissegundosDeAgora() - marcaDeTempo;
    if (ida > 9999u) {
        ida = 9999u;   /* o relógio deu a volta no meio da medição: esta não vale */
    }
    if (ida == 0u) {
        /* No loopback (dois jogos no mesmo PC) a ida e volta é menor que 1 ms e a conta dá
         * zero — e zero é o que a tela usa para "ainda não medi". Um milissegundo é a
         * menor medida que este relógio sabe dar, então é ela que vale. */
        ida = 1u;
    }
    /* Na lista, o ping é da sala; na sala de espera, é de cada jogador */
    for (int i = 0; i < quantasSalas; i++) {
        if (mesmoEndereco(&salas[i].endereco, de)) {
            salas[i].ping = ida;
        }
    }
    for (int i = 0; i < MAXIMO_DE_JOGADORES; i++) {
        if (jogadores[i].ligado && mesmoEndereco(&jogadores[i].endereco, de)) {
            jogadores[i].ping = ida;
            jogadores[i].calado = 0.0f;
        }
    }
    if (!souODono && mesmoEndereco(&doDono, de) && minhaVaga >= 0) {
        jogadores[minhaVaga].ping = ida;
    }
}

/* ==================================================================
 * O quadro
 * ================================================================== */

/*
 * esvaziarACaixa
 * Lê os pacotes que chegaram, até a caixa ficar vazia ou o teto do quadro.
 *
 * O teto existe porque esta função roda DENTRO do quadro do jogo: sem ele, um
 * pico de tráfego (ou outro programa gritando na nossa porta) viraria um
 * travamento de imagem. Os que sobrarem esperam o quadro seguinte.
 * Recebe: nada.
 * Devolve: nada.
 */
static void esvaziarACaixa(Corrida *corrida) {
    unsigned char fio[MAIOR_PACOTE];
    EnderecoDaRede de;
    for (int lidos = 0; lidos < PACOTES_POR_QUADRO; lidos++) {
        int tamanho = receber(soquete, fio, (int) sizeof(fio), &de);
        if (tamanho <= 0) {
            return;
        }
        CabecalhoDeRede cabecalho;
        if (!lerCabecalho(fio, tamanho, &cabecalho)) {
            continue;   /* não é nosso: lixo da rede, ou outro programa na porta */
        }
        /* O anúncio é o único que vale de uma versão diferente: é ele que deixa a
         * sala aparecer na lista marcada "OUTRA VERSÃO", em vez de o jogador olhar
         * uma lista vazia sem entender por quê. O resto não dá para interpretar. */
        if (cabecalho.versao != VERSAO_DO_PROTOCOLO
            && cabecalho.tipo != PACOTE_ANUNCIO && cabecalho.tipo != PACOTE_ENTRAR) {
            continue;
        }
        switch (cabecalho.tipo) {
            case PACOTE_PROCURAR: tratarProcurar(&de); break;
            case PACOTE_ANUNCIO:  tratarAnuncio(fio, tamanho, &cabecalho, &de); break;
            case PACOTE_ENTRAR:   tratarEntrar(fio, tamanho, &cabecalho, &de); break;
            case PACOTE_ACEITOU:  tratarAceitou(fio, tamanho); break;
            case PACOTE_RECUSOU:  tratarRecusou(fio, tamanho); break;
            case PACOTE_SALA:     tratarEstadoDaSala(fio, tamanho); break;
            case PACOTE_ESCOLHA:  tratarEscolha(fio, tamanho, &de); break;
            case PACOTE_LARGAR:   tratarLargada(fio, tamanho); break;
            case PACOTE_SAIR:     tratarSaida(tamanho, &de); break;
            case PACOTE_PING:     tratarPing(fio, tamanho, &de); break;
            case PACOTE_PONGUE:   tratarPongue(fio, tamanho, &de); break;
            default:
                /* Comandos e retratos são da corrida, não da sala. Eles chegam no MESMO
                 * soquete (um por processo), e a sala é quem o tem: por isso ela esvazia
                 * a caixa e repassa o que é de online.c. Jogá-los fora aqui faria a
                 * corrida em rede nunca receber nada. */
                receberPacoteDaCorrida(fio, tamanho, &cabecalho, &de, corrida);
                break;
        }
    }
}

void perdiODono(void) {
    if (estado == SALA_NA_CORRIDA) {
        /* No meio da corrida ninguém é expulso de uma curva: a corrida VIRA offline e
         * o jogador termina a prova. Quem faz essa virada é online.c, que vê o estado. */
        dizer("O DONO SAIU DA CORRIDA");
        estado = SALA_DEU_ERRADO;
        return;
    }
    dizer("O DONO SAIU DA SALA");
    estado = SALA_DEU_ERRADO;
    souODono = 0;
    minhaVaga = -1;
    limparJogadores();
}

/*
 * atualizarSala
 * Um quadro da sala: lê o que chegou e manda os periódicos.
 * Recebe: o tempo do quadro.
 * Devolve: nada.
 */
void atualizarSala(Corrida *corrida, float tempoQuadro) {
    if (soquete == NULL) {
        return;
    }
    if (tempoDoRecado > 0.0f) {
        tempoDoRecado -= tempoQuadro;
    }
    esvaziarACaixa(corrida);

    desdeOPing += tempoQuadro;
    int horaDoPing = desdeOPing >= SEGUNDOS_ENTRE_PINGS;
    if (horaDoPing) {
        desdeOPing = 0.0f;
        pingMandadoEm = milissegundosDeAgora();
    }

    if (estado == SALA_PROCURANDO) {
        esquecerSalasCaladas(tempoQuadro);
        desdeAProcura += tempoQuadro;
        if (desdeAProcura >= SEGUNDOS_ENTRE_PROCURAS) {
            desdeAProcura = 0.0f;
            mandarProcurar();
        }
        if (horaDoPing) {
            for (int i = 0; i < quantasSalas; i++) {
                mandarPing(PACOTE_PING, pingMandadoEm, &salas[i].endereco);
            }
        }
        return;
    }

    if (estado == SALA_ENTRANDO) {
        esperandoResposta += tempoQuadro;
        /* O pedido se repete: é UDP, e o primeiro pode ter se perdido */
        desdeAProcura += tempoQuadro;
        if (desdeAProcura >= SEGUNDOS_ENTRE_PROCURAS) {
            desdeAProcura = 0.0f;
            mandarEntrar();
        }
        if (esperandoResposta > SEGUNDOS_PARA_DESISTIR) {
            dizer("A SALA NAO RESPONDEU");
            estado = SALA_PROCURANDO;
        }
        return;
    }

    if (souODono && (estado == SALA_SOU_DONO || estado == SALA_NA_CORRIDA)) {
        desdeOFarol += tempoQuadro;
        if (desdeOFarol >= SEGUNDOS_ENTRE_FAROIS) {
            desdeOFarol = 0.0f;
            mandarAnuncio(NULL);
        }
        desdeOEstado += tempoQuadro;
        if (desdeOEstado >= SEGUNDOS_ENTRE_ESTADOS) {
            desdeOEstado = 0.0f;
            mandarEstadoDaSala();
        }
        /* Os convidados calados: na sala de espera a vaga é liberada (ninguém fica
         * esperando um fantasma dar PRONTO). Na corrida é online.c que decide, e lá
         * a vaga NÃO é liberada: tirar um carro no meio da prova mudaria a
         * classificação de todos. */
        for (int i = 0; i < MAXIMO_DE_JOGADORES; i++) {
            if (!jogadores[i].ligado || i == minhaVaga) {
                continue;
            }
            jogadores[i].calado += tempoQuadro;
            if (estado == SALA_SOU_DONO && jogadores[i].calado > SEGUNDOS_PARA_SOLTAR_VAGA) {
                memset(&jogadores[i], 0, sizeof(jogadores[i]));
                mandarEstadoDaSala();
            }
        }
        if (horaDoPing) {
            for (int i = 0; i < MAXIMO_DE_JOGADORES; i++) {
                if (jogadores[i].ligado && i != minhaVaga) {
                    mandarPing(PACOTE_PING, pingMandadoEm, &jogadores[i].endereco);
                }
            }
        }
        /* A contagem para largar: o pacote de largada vai repetido por vários quadros,
         * e no fim da contagem o dono entra na corrida junto com todos */
        if (contagem > 0.0f) {
            contagem -= tempoQuadro;
            if (repetindoLargada > 0) {
                repetindoLargada--;
                mandarLargada();
            }
            if (contagem <= 0.0f) {
                contagem = 0.0f;
                montarFormacao();   /* de novo: agora com souODono certo e as vagas finais */
                formacao.souODono = 1;
                estado = SALA_NA_CORRIDA;
                vaiLargar = 1;
            }
        }
        return;
    }

    if (estado == SALA_SOU_CONVIDADO || estado == SALA_NA_CORRIDA) {
        semOuvirODono += tempoQuadro;
        if (estado == SALA_SOU_CONVIDADO) {
            desdeOEstado += tempoQuadro;
            if (desdeOEstado >= SEGUNDOS_ENTRE_ESTADOS) {
                desdeOEstado = 0.0f;
                mandarEscolha();   /* o pedido é idempotente: repetir é a própria garantia */
            }
            if (contagem > 0.0f) {
                contagem -= tempoQuadro;
            }
        }
        if (horaDoPing) {
            mandarPing(PACOTE_PING, pingMandadoEm, &doDono);
        }
        if (semOuvirODono > SEGUNDOS_SEM_O_DONO) {
            perdiODono();
        }
    }
}

/* ==================================================================
 * O que online.c precisa da sala
 *
 * A ordem das sequências é de online.c: cada pacote dele (comandos e retratos)
 * tem o seu próprio contador, e misturar com os da sala só confundiria. O que a
 * sala precisa saber é apenas que o jogador DEU SINAL DE VIDA — é isso que
 * segura o relógio que entrega o carro dele à IA.
 * ================================================================== */

/*
 * ouviOConvidado
 * Chegou um pacote de comandos deste jogador: ele está vivo.
 * Recebe: a vaga dele na sala.
 * Devolve: nada.
 */
void ouviOConvidado(int vaga) {
    if (vaga >= 0 && vaga < MAXIMO_DE_JOGADORES) {
        jogadores[vaga].calado = 0.0f;
    }
}

/*
 * ouviODonoAgora
 * Chegou um retrato do dono: ele está vivo. Segura o relógio que, estourado,
 * transforma a corrida em offline.
 * Recebe: nada.
 * Devolve: nada.
 */
void ouviODonoAgora(void) {
    semOuvirODono = 0.0f;
}

/*
 * segundosCaladoOConvidado
 * Recebe: a vaga.
 * Devolve: quantos segundos faz que esse jogador não manda nada (0 na própria vaga).
 */
float segundosCaladoOConvidado(int vaga) {
    if (vaga < 0 || vaga >= MAXIMO_DE_JOGADORES || vaga == minhaVaga) {
        return 0.0f;
    }
    return jogadores[vaga].calado;
}
