/*
 * teste_rede.c
 * ------------------------------------------------------------------
 * Testes do modo online, sem abrir o jogo.
 *
 * Duas partes:
 *
 *   make testar-rede          confere o protocolo sozinho: escreve cada pacote,
 *                             lê de volta e compara. Roda em meio segundo e não
 *                             toca na rede de verdade.
 *
 *   ./build/teste_rede dono        ) os dois juntos, em dois terminais: um cria a
 *   ./build/teste_rede convidado   ) sala e o outro tem de achá-la. É o teste que
 *                                    prova que a descoberta funciona de verdade
 *                                    nesta máquina (e que o firewall deixou).
 *
 * O segundo é o que importa, e é o que o loopback não consegue fingir quando os
 * dois estão em PCs diferentes: rode em duas máquinas da mesma Wi-Fi antes de
 * confiar no modo online.
 * ------------------------------------------------------------------
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "sala.h"
#include "protocolo.h"
#include "rede.h"
#include "corrida.h"
#ifdef _WIN32
    #include <windows.h>
#else
    #include <time.h>
#endif

static int falhas = 0;

/*
 * conferir
 * Imprime [OK] ou [FALHOU] e conta as falhas, como os outros testes do projeto.
 * Recebe: a condição e a descrição.
 * Devolve: nada.
 */
static void conferir(int condicao, const char *descricao) {
    printf("  %s %s\n", condicao ? "[OK]    " : "[FALHOU]", descricao);
    if (!condicao) {
        falhas++;
    }
}

/* ==================================================================
 * O protocolo
 * ================================================================== */

static void testarNumerosNoFio(void) {
    printf("\n1) Numeros no fio\n");
    unsigned char fio[64];
    int onde = 0;
    escreverU8(fio, &onde, 0xA5u);
    escreverU16(fio, &onde, 0xBEEFu);
    escreverU32(fio, &onde, 0xDEADBEEFu);
    escreverF32(fio, &onde, -1234.5f);
    conferir(onde == 11, "um u8, um u16, um u32 e um float ocupam 11 bytes");

    int lido = 0;
    conferir(lerU8(fio, &lido) == 0xA5u, "o u8 volta igual");
    conferir(lerU16(fio, &lido) == 0xBEEFu, "o u16 volta igual");
    conferir(lerU32(fio, &lido) == 0xDEADBEEFu, "o u32 volta igual");
    conferir(lerF32(fio, &lido) == -1234.5f, "o float volta igual");
    conferir(lido == onde, "ler consome exatamente o que escrever gravou");

    /* Little-endian na mao: o byte menos significativo primeiro, em qualquer maquina */
    onde = 0;
    escreverU32(fio, &onde, 0x04030201u);
    conferir(fio[0] == 1 && fio[1] == 2 && fio[2] == 3 && fio[3] == 4,
             "o u32 sai em little-endian, byte a byte (nao depende da maquina)");

    /* Um float estragado na rede nao pode entrar na fisica */
    onde = 0;
    escreverU32(fio, &onde, 0x7FC00000u);   /* um NaN */
    lido = 0;
    conferir(lerF32(fio, &lido) == 0.0f, "um NaN vindo do fio vira 0 (nao contamina a fisica)");
    onde = 0;
    escreverU32(fio, &onde, 0x7F800000u);   /* +infinito */
    lido = 0;
    conferir(lerF32(fio, &lido) == 0.0f, "um infinito vindo do fio vira 0");
}

static void testarTextoNoFio(void) {
    printf("\n2) Nomes no fio\n");
    unsigned char fio[64];
    char volta[TAMANHO_DO_APELIDO];
    int onde = 0;
    escreverTexto(fio, &onde, "JOAO", TAMANHO_DO_APELIDO);
    conferir(onde == TAMANHO_DO_APELIDO, "o nome ocupa sempre o tamanho todo (o pacote nao muda de tamanho)");
    int lido = 0;
    lerTexto(fio, &lido, volta, TAMANHO_DO_APELIDO);
    conferir(strcmp(volta, "JOAO") == 0, "o nome volta igual");

    /* Nome maior que o campo: cortado, e nunca sem terminador */
    onde = 0;
    escreverTexto(fio, &onde, "NOME ABSURDAMENTE COMPRIDO QUE NAO CABE", TAMANHO_DO_APELIDO);
    lido = 0;
    lerTexto(fio, &lido, volta, TAMANHO_DO_APELIDO);
    conferir((int) strlen(volta) < TAMANHO_DO_APELIDO, "nome comprido e cortado, e sempre termina");

    /* Byte estragado: este texto vai ser desenhado em OUTRA maquina, e um byte de
     * controle no meio de um nome viraria lixo (ou pior) na tela dela.
     * O nome e montado byte a byte de proposito: escrito como "\x01B" dentro de uma
     * string de C, o escape hexadecimal seria GANANCIOSO e engoliria o B (0x1B), e o
     * teste passaria a conferir outra coisa sem ninguem notar. */
    char estragado[8];
    estragado[0] = 'A';
    estragado[1] = (char) 1;    /* um byte de controle */
    estragado[2] = (char) 31;   /* o ultimo antes do espaco, a borda do imprimivel */
    estragado[3] = 'B';
    estragado[4] = '\0';
    onde = 0;
    escreverTexto(fio, &onde, estragado, TAMANHO_DO_APELIDO);
    conferir(fio[1] == (unsigned char) '?' && fio[2] == (unsigned char) '?',
             "byte nao imprimivel vira '?' no fio (nao vira lixo na tela do outro)");
    lido = 0;
    lerTexto(fio, &lido, volta, TAMANHO_DO_APELIDO);
    conferir(strcmp(volta, "A??B") == 0, "e o nome chega legivel do outro lado");

    /* E o caminho de volta: um byte de controle que chegue do fio nao pode passar */
    onde = 0;
    escreverU8(fio, &onde, (unsigned int) 'A');
    escreverU8(fio, &onde, 7u);   /* um byte de controle vindo de um pacote estragado */
    for (int i = onde; i < TAMANHO_DO_APELIDO; i++) {
        fio[i] = 0;
    }
    lido = 0;
    lerTexto(fio, &lido, volta, TAMANHO_DO_APELIDO);
    int soImprimivel = 1;
    for (int i = 0; volta[i] != '\0'; i++) {
        soImprimivel = soImprimivel && volta[i] >= 32 && volta[i] <= 126;
    }
    conferir(soImprimivel, "nenhum byte de controle vindo do fio sobrevive a leitura");
}

static void testarCabecalho(void) {
    printf("\n3) Cabecalho\n");
    unsigned char fio[64];
    int onde = escreverCabecalho(fio, PACOTE_ANUNCIO, 1234);
    conferir(onde == BYTES_DO_CABECALHO, "o cabecalho tem BYTES_DO_CABECALHO bytes");

    CabecalhoDeRede c;
    conferir(lerCabecalho(fio, onde, &c), "o cabecalho que escrevemos e aceito");
    conferir(c.tipo == PACOTE_ANUNCIO && c.sequencia == 1234 && c.versao == VERSAO_DO_PROTOCOLO,
             "tipo, sequencia e versao voltam iguais");

    conferir(!lerCabecalho(fio, BYTES_DO_CABECALHO - 1, &c), "pacote curto demais e recusado");
    unsigned char lixo[16] = { 0 };
    conferir(!lerCabecalho(lixo, 16, &c), "pacote sem a marca do KATCHAU e recusado (lixo da rede)");

    /* A versao errada ainda e LIDA: e o que deixa a sala aparecer marcada "OUTRA VERSAO" */
    fio[4] = VERSAO_DO_PROTOCOLO + 7;
    conferir(lerCabecalho(fio, onde, &c) && c.versao != VERSAO_DO_PROTOCOLO,
             "versao diferente ainda e lida (para a tela poder dizer OUTRA VERSAO)");
}

static void testarSequencias(void) {
    printf("\n4) Ordem dos pacotes\n");
    conferir(sequenciaEMaisNova(11, 10), "11 e mais novo que 10");
    conferir(!sequenciaEMaisNova(10, 11), "10 nao e mais novo que 11 (chegou fora de ordem)");
    conferir(!sequenciaEMaisNova(10, 10), "o mesmo numero nao e mais novo (pacote repetido)");
    /* O contador da a volta em 65535: depois dele vem 0, e 0 continua sendo mais novo */
    conferir(sequenciaEMaisNova(0, 65535), "0 e mais novo que 65535 (o contador deu a volta)");
    conferir(sequenciaEMaisNova(3, 65530), "3 e mais novo que 65530 (a volta nao trava a corrida)");
    conferir(!sequenciaEMaisNova(65530, 3), "e 65530 nao e mais novo que 3");
}

static void testarComandos(void) {
    printf("\n5) Comandos\n");
    Comandos c = { 1, 0, 0, 1, 0.5f, 1, 0 };
    unsigned char bits = bitsDosComandos(&c);
    Comandos volta = comandosDosBits(bits, direcaoNoFio(c.direcao));
    conferir(volta.acelerar == 1 && volta.frear == 0 && volta.direita == 1 && volta.esquerda == 0,
             "os pedais e as setas voltam iguais");
    conferir(volta.nitro == 1 && volta.freioDeMao == 0, "o nitro e o freio de mao voltam iguais");
    conferir(fabsf(volta.direcao - 0.5f) < 0.01f, "o analogico volta com erro menor que 1% do curso");

    conferir(direcaoNoFio(5.0f) == 100 && direcaoNoFio(-5.0f) == -100,
             "um analogico fora da faixa e preso em -1..+1 (nao da a volta no byte)");
}

static void testarCarroNoFio(void) {
    printf("\n6) O carro no fio\n");
    /* Uma pista de verdade, para o carro ter um segmento onde morar */
    Pista *pista = criarOval();
    Corrida corrida;
    iniciarCorrida(&corrida, pista, 0, NIVEL_FACIL);
    Carro *carro = &corrida.jogador;
    carro->velocidade = 9876.5f;
    carro->deslocamentoLateral = -0.375f;
    carro->rumoNoMundo = 247.5f;
    carro->anguloDeDrift = -12.25f;
    carro->voltas = 3;
    carro->nitro = 0.5f;
    carro->desgaste = 0.25f;
    carro->distanciaPercorrida = 123456.75f;
    carro->melhorVolta = 52.125f;
    carro->nitroLigado = 1;
    carro->estadoDoPit = PIT_TROCANDO;
    carro->pneuEstourado = 5;
    carro->posicao.distanciaNoSegmento = 123.5f;

    CarroNoFio noFio;
    guardarCarroNoFio(carro, &noFio);
    unsigned char fio[256];
    int onde = 0;
    escreverCarroNoFio(fio, &onde, &noFio);
    conferir(onde == BYTES_DO_CARRO_NO_FIO, "o carro ocupa BYTES_DO_CARRO_NO_FIO bytes no fio");

    CarroNoFio volta;
    int lido = 0;
    lerCarroDoFio(fio, &lido, &volta);
    conferir(lido == onde, "ler o carro consome exatamente o que escrever gravou");
    conferir(volta.indiceDoSegmento == noFio.indiceDoSegmento, "o segmento volta igual");
    conferir(volta.voltas == 3, "as voltas voltam iguais (e o campo AUTORITATIVO da corrida)");

    /* A quantizacao: o erro tem de ser menor do que o jogador consegue ver */
    float dentroVolta = volta.distanciaNoSegmento / 65535.0f * COMPRIMENTO_SEGMENTO;
    conferir(fabsf(dentroVolta - 123.5f) < COMPRIMENTO_SEGMENTO / 10000.0f,
             "a posicao dentro do segmento volta com erro menor que 2 cm");
    conferir(fabsf(volta.deslocamentoLateral / 8000.0f - (-0.375f)) < 0.001f,
             "o lateral volta com erro menor que 0,001 da largura");
    conferir(fabsf(volta.velocidade / 4.0f - 9876.5f) < 0.5f, "a velocidade volta com erro menor que 0,5 un/s");
    conferir(fabsf(volta.rumoNoMundo * (360.0f / 65536.0f) - 247.5f) < 0.02f,
             "o rumo volta com erro menor que 0,02 grau");
    conferir(fabsf(volta.anguloDeDrift / 100.0f - (-12.25f)) < 0.02f, "o angulo de drift volta com erro menor que 0,02 grau");
    conferir(volta.distanciaPercorrida == 123456.75f, "a distancia percorrida volta exata (vai como float)");
    conferir(volta.melhorVolta == 52.125f, "a melhor volta volta exata");
    conferir((volta.estado & FIO_NITRO_LIGADO) != 0, "o bit do nitro ligado volta");
    conferir((volta.pit & 0x3u) == PIT_TROCANDO, "o estado do pit volta");
    conferir(((volta.pit >> 2) & 0xFu) == 5u, "os pneus estourados voltam");

    /* O rumo dando a volta: 370 graus e 10 graus */
    carro->rumoNoMundo = 370.0f;
    guardarCarroNoFio(carro, &noFio);
    conferir(fabsf(noFio.rumoNoMundo * (360.0f / 65536.0f) - 10.0f) < 0.02f,
             "um rumo de 370 graus volta como 10 (o angulo e normalizado, nao estoura o campo)");
    carro->rumoNoMundo = -30.0f;
    guardarCarroNoFio(carro, &noFio);
    conferir(fabsf(noFio.rumoNoMundo * (360.0f / 65536.0f) - 330.0f) < 0.02f,
             "um rumo de -30 graus volta como 330");

    /* Velocidade absurda: presa, e nao dando a volta no campo (um carro a 20000 nao pode virar um carro parado) */
    carro->velocidade = 999999.0f;
    guardarCarroNoFio(carro, &noFio);
    conferir(noFio.velocidade == 65535, "uma velocidade absurda e presa no maximo do campo");

    liberarPista(pista);
}

static void testarTamanhoDosPacotes(void) {
    printf("\n7) Tamanho dos pacotes\n");
    int retrato = BYTES_DO_CABECALHO + 16 + TOTAL_CARROS * BYTES_DO_CARRO_NO_FIO;
    printf("     o retrato dos %d carros tem %d bytes; a %d Hz da %.1f kB/s por convidado\n",
           TOTAL_CARROS, retrato, 30, retrato * 30.0f / 1024.0f);
    conferir(retrato < MAIOR_PACOTE,
             "o retrato cabe num datagrama (abaixo da MTU: nenhum pacote e picado pela rede)");
    conferir(MAIOR_PACOTE <= 1400, "o teto de pacote fica abaixo da MTU de 1500 de qualquer Wi-Fi");
}

static void testarFormacao(void) {
    printf("\n8) O grid da corrida em rede\n");
    Pista *pista = criarOval();
    Corrida corrida;

    /* Dois humanos: eles pegam os ULTIMOS lugares do grid, e a IA o resto */
    FormacaoOnline f;
    memset(&f, 0, sizeof(f));
    f.totalDeJogadores = 2;
    f.numeroDoCarro[0] = 0;    /* McQueen */
    f.numeroDoCarro[1] = 5;    /* Ramone */
    f.lugarNoGrid[0] = TOTAL_CARROS - 1;
    f.lugarNoGrid[1] = TOTAL_CARROS - 2;
    f.euSou = 1;
    f.souODono = 0;
    f.semente = 12345u;

    iniciarCorridaOnline(&corrida, pista, NIVEL_FACIL, &f);
    conferir(corrida.emRede == 1 && corrida.souODono == 0, "a corrida sabe que e em rede e que nao sou o dono");
    conferir(corrida.totalDeHumanos == 2 && corrida.euSou == 1, "a corrida tem 2 humanos, e eu sou o segundo");
    conferir(corrida.jogador.numero == 5, "o carro DESTA maquina e o da minha vaga (Ramone)");
    conferir(corrida.humanos[1] == &corrida.jogador, "humanos[euSou] aponta para corrida->jogador");
    conferir(corrida.humanos[0] != NULL && corrida.humanos[0]->numero == 0,
             "o outro humano esta numa vaga de rivais, com o carro dele");
    conferir(corrida.humanos[0]->controladoPeloJogador == 1, "o carro do outro humano nao e da IA");
    conferir(carroEDeHumano(&corrida, corrida.humanos[0]) && carroEDeHumano(&corrida, &corrida.jogador),
             "os dois carros humanos sao reconhecidos como humanos");

    /* Nenhum carro repetido, e nenhum lugar de grid repetido: e o que garante que o
     * grid montado em cada maquina e exatamente o mesmo */
    int vezesDoCarro[TOTAL_CARROS] = { 0 };
    for (int i = 0; i < TOTAL_CARROS; i++) {
        vezesDoCarro[corrida.classificacao[i]->numero]++;
    }
    int repetido = 0, faltando = 0;
    for (int i = 0; i < TOTAL_CARROS; i++) {
        repetido += vezesDoCarro[i] > 1;
        faltando += vezesDoCarro[i] == 0;
    }
    conferir(!repetido && !faltando, "os 14 carros estao no grid, cada um uma vez so");

    int daIA = 0;
    for (int i = 0; i < TOTAL_RIVAIS; i++) {
        daIA += !carroEDeHumano(&corrida, &corrida.rivais[i]);
    }
    conferir(daIA == TOTAL_CARROS - 2, "as vagas que sobraram viraram rivais de IA (o grid segue cheio)");

    /* A corrida de um jogador NAO pode ter mudado: e a mesma funcao que monta as duas */
    Corrida sozinho;
    iniciarCorrida(&sozinho, pista, 0, NIVEL_FACIL);
    conferir(sozinho.emRede == 0 && sozinho.totalDeHumanos == 1, "a corrida de um jogador nao e em rede");
    conferir(sozinho.jogador.numero == 0, "o carro escolhido continua sendo o do jogador");
    int rivaisDaIA = 0;
    for (int i = 0; i < TOTAL_RIVAIS; i++) {
        rivaisDaIA += sozinho.rivais[i].habilidade > 0.0f && !sozinho.rivais[i].controladoPeloJogador;
    }
    conferir(rivaisDaIA == TOTAL_RIVAIS, "e os 13 rivais continuam sendo da IA");
    conferir(sozinho.rivais[0].direcaoLivre == 0,
             "os rivais de IA seguem com a direcao guiada (o modelo para o qual a IA foi acertada)");

    liberarPista(pista);
}

/* ==================================================================
 * A rede de verdade: um cria a sala, o outro acha
 * ================================================================== */

#define SEGUNDOS_DO_QUADRO  (1.0f / 60.0f)
#define QUADROS_DE_ESPERA   600   /* uns 10 segundos */

/*
 * dormirUmQuadro
 * Espera de verdade o tempo de um quadro.
 *
 * Sem isto o teste não testa nada: atualizarSala não bloqueia (é essa a graça dela no
 * jogo), então um laço de 1800 "quadros" acaba em microssegundos, o dono fecha a sala
 * antes de o convidado começar a procurar, e a descoberta parece quebrada quando o
 * que faltou foi o tempo passar. Aqui, ao contrário do jogo, não há quadro para
 * desenhar — quem dá o compasso tem de ser esta espera.
 * Recebe: nada.
 * Devolve: nada.
 */
static void dormirUmQuadro(void) {
#ifdef _WIN32
    Sleep(1000 / 60);
#else
    struct timespec quanto = { 0, 1000000000L / 60 };
    nanosleep(&quanto, NULL);
#endif
}

static void rodarComoDono(void) {
    iniciarSala();
    if (!aRedeAbriu()) {
        printf("ERRO: a rede nao abriu nesta maquina.\n");
        falhas++;
        return;
    }
    printf("Sala criada por %s. Esperando alguem achar...\n", meuApelido());
    printf("(Se o Windows perguntar do firewall, LIBERE em redes privadas.)\n");
    criarSala(PISTA_OVAL, NIVEL_FACIL, 0);
    Corrida corrida;
    memset(&corrida, 0, sizeof(corrida));
    int ultimos = 0, maximo = 0;
    for (int quadro = 0; quadro < QUADROS_DE_ESPERA * 3; quadro++) {
        atualizarSala(&corrida, SEGUNDOS_DO_QUADRO);
        dormirUmQuadro();
        int agora = totalDeJogadoresDaSala();
        if (agora > maximo) {
            maximo = agora;
        }
        if (agora != ultimos) {
            ultimos = agora;
            printf("  agora somos %d na sala\n", agora);
            for (int i = 0; i < MAXIMO_DE_JOGADORES; i++) {
                const JogadorDaSala *j = jogadorDaSala(i);
                if (j != NULL && j->ligado) {
                    printf("    vaga %d: %s, carro %d, %s, ping %u ms\n", i, j->nome, j->numeroDoCarro,
                           j->pronto ? "PRONTO" : "escolhendo", j->ping);
                }
            }
        }
    }
    /* O MAXIMO, e nao o ultimo: o convidado entra, o teste dele acaba e ele sai, e a vaga
     * e liberada (o que esta certo). Olhar o ultimo valor acusaria falha num acerto. */
    conferir(maximo >= 2, "um convidado entrou na sala");
    encerrarSala();
    encerrarRede();
}

static void rodarComoConvidado(void) {
    iniciarSala();
    if (!aRedeAbriu()) {
        printf("ERRO: a rede nao abriu nesta maquina.\n");
        falhas++;
        return;
    }
    printf("Procurando salas no wi-fi, como %s...\n", meuApelido());
    comecarAProcurar();
    Corrida corrida;
    memset(&corrida, 0, sizeof(corrida));
    int achou = 0;
    for (int quadro = 0; quadro < QUADROS_DE_ESPERA && !achou; quadro++) {
        atualizarSala(&corrida, SEGUNDOS_DO_QUADRO);
        dormirUmQuadro();
        if (totalDeSalasAchadas() > 0) {
            achou = 1;
            for (int i = 0; i < totalDeSalasAchadas(); i++) {
                const SalaNaRede *s = salaAchada(i);
                char endereco[32];
                textoDoEndereco(&s->endereco, endereco, sizeof(endereco));
                printf("  achei: \"%s\" de %s em %s, %d/%d, ping %u ms%s\n", s->nome, s->dono, endereco,
                       s->jogadores, s->limite, s->ping, s->versaoDiferente ? " (OUTRA VERSAO)" : "");
            }
        }
    }
    conferir(achou, "achei a sala do dono na rede local");
    if (achou) {
        printf("Entrando na sala...\n");
        entrarNaSalaAchada(0, 5);
        int dentro = 0;
        for (int quadro = 0; quadro < QUADROS_DE_ESPERA && !dentro; quadro++) {
            atualizarSala(&corrida, SEGUNDOS_DO_QUADRO);
            dormirUmQuadro();
            dentro = (estadoDaSala() == SALA_SOU_CONVIDADO);
        }
        conferir(dentro, "entrei na sala (o dono me aceitou e me deu uma vaga)");
        if (dentro) {
            printf("  entrei na vaga %d\n", minhaVagaNaSala());
            const JogadorDaSala *eu = jogadorDaSala(minhaVagaNaSala());
            conferir(eu != NULL && eu->ligado, "a minha vaga esta preenchida na lista da sala");
            /* Espera uns segundos trocando pacotes, para o ping ser medido */
            for (int quadro = 0; quadro < 180; quadro++) {
                atualizarSala(&corrida, SEGUNDOS_DO_QUADRO);
                dormirUmQuadro();
            }
            printf("  o dono escolheu a pista %d, nivel %d\n", pistaDaSala(), nivelDaSala());
        }
    }
    encerrarSala();
    encerrarRede();
}

int main(int argc, char *argv[]) {
    if (argc > 1 && strcmp(argv[1], "dono") == 0) {
        printf("=== KATCHAU: teste de rede, como DONO da sala ===\n");
        rodarComoDono();
    } else if (argc > 1 && strcmp(argv[1], "convidado") == 0) {
        printf("=== KATCHAU: teste de rede, como CONVIDADO ===\n");
        rodarComoConvidado();
    } else {
        printf("=== KATCHAU: teste do protocolo do modo online ===\n");
        testarNumerosNoFio();
        testarTextoNoFio();
        testarCabecalho();
        testarSequencias();
        testarComandos();
        testarCarroNoFio();
        testarTamanhoDosPacotes();
        testarFormacao();
        printf("\nPara testar a rede DE VERDADE, em dois terminais:\n");
        printf("  ./build/teste_rede dono\n");
        printf("  ./build/teste_rede convidado\n");
    }
    printf("\n%d teste(s) falharam.\n", falhas);
    return falhas > 0 ? 1 : 0;
}
