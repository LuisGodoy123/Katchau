/*
 * rede.c
 * ------------------------------------------------------------------
 * A casca do socket. Veja rede.h.
 * ------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "rede.h"

#ifdef _WIN32
    /* O Winsock2 TEM de vir antes de windows.h: o windows.h arrasta o winsock.h
     * (versão 1), e as duas definem as mesmas structs de maneiras diferentes. */
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>   /* GetTickCount, para o relógio do ping */
    typedef SOCKET Descritor;
    typedef int TamanhoDeEndereco;
    #define SOQUETE_INVALIDO  INVALID_SOCKET
    #define fecharDescritor   closesocket
#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <errno.h>
    #include <sys/time.h>
    typedef int Descritor;
    typedef socklen_t TamanhoDeEndereco;
    #define SOQUETE_INVALIDO  (-1)
    #define fecharDescritor   close
#endif

struct Soquete {
    Descritor descritor;
    int porta;             /* a porta que saiu de verdade, da faixa */
};

static int redeIniciada = 0;

/*
 * iniciarRede
 * Liga a pilha de rede. No Windows isso é obrigatório antes de qualquer socket
 * (o WSAStartup); nos outros sistemas não há nada a fazer e a função só marca
 * que está tudo certo.
 * Recebe: nada.
 * Devolve: 1 se dá para usar a rede, 0 se não.
 */
int iniciarRede(void) {
    if (redeIniciada) {
        return 1;
    }
#ifdef _WIN32
    WSADATA dados;
    if (WSAStartup(MAKEWORD(2, 2), &dados) != 0) {
        return 0;
    }
#endif
    redeIniciada = 1;
    return 1;
}

/*
 * encerrarRede
 * Devolve a pilha de rede ao sistema, no fim do jogo.
 * Recebe: nada.
 * Devolve: nada.
 */
void encerrarRede(void) {
    if (!redeIniciada) {
        return;
    }
#ifdef _WIN32
    WSACleanup();
#endif
    redeIniciada = 0;
}

/*
 * deixarNaoBloqueante
 * Põe o soquete no modo em que ler uma caixa de entrada vazia devolve erro na
 * hora, em vez de parar o programa até alguém mandar algo. Sem isto, um quadro
 * do jogo ficaria esperando a rede.
 * Recebe: o descritor.
 * Devolve: 1 se deu, 0 se não.
 */
static int deixarNaoBloqueante(Descritor descritor) {
#ifdef _WIN32
    unsigned long ligado = 1;
    return ioctlsocket(descritor, FIONBIO, &ligado) == 0;
#else
    int opcoes = fcntl(descritor, F_GETFL, 0);
    return opcoes >= 0 && fcntl(descritor, F_SETFL, opcoes | O_NONBLOCK) == 0;
#endif
}

/*
 * calarOAvisoDePortaFechada
 * A pedra do Windows em UDP: quando mandamos um pacote para um jogo que acabou de
 * fechar, a máquina dele responde um ICMP "porta inalcançável", e isso faz o
 * PRÓXIMO recvfrom DESTE soquete devolver erro (WSAECONNRESET) — num protocolo
 * que nem tem conexão. Sem desligar esse aviso, um convidado que fecha o jogo
 * derruba a leitura do dono, que é o contrário do que se quer.
 *
 * Nos outros sistemas não existe esse comportamento e a função não faz nada.
 * (Mesmo com ele desligado, "receber" ainda trata o erro como "não chegou nada":
 * duas cercas, porque esta aqui depende da versão do Windows.)
 * Recebe: o descritor.
 * Devolve: nada.
 */
static void calarOAvisoDePortaFechada(Descritor descritor) {
#ifdef _WIN32
    #ifndef SIO_UDP_CONNRESET
        #define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR, 12)
    #endif
    BOOL desligado = FALSE;
    DWORD devolvidos = 0;
    WSAIoctl(descritor, SIO_UDP_CONNRESET, &desligado, sizeof(desligado), NULL, 0, &devolvidos, NULL, NULL);
#else
    (void) descritor;
#endif
}

/*
 * abrirSoquete
 * Abre um soquete UDP pronto para mandar broadcast, na PRIMEIRA PORTA LIVRE da
 * faixa do jogo.
 *
 * Procurar a porta, em vez de fixar uma, é o que faz dois (ou dez) jogos
 * conviverem no mesmo PC sem configurar nada — e é assim que se testa o modo
 * online no dia a dia. Quem quiser forçar uma porta na depuração põe
 * KATCHAU_PORTA no ambiente.
 * Recebe: nada.
 * Devolve: o soquete, ou NULL se a faixa toda estiver ocupada.
 */
Soquete *abrirSoquete(void) {
    if (!iniciarRede()) {
        return NULL;
    }

    /* KATCHAU_PORTA força uma porta só; sem ela, tenta a faixa inteira em ordem */
    int primeira = PRIMEIRA_PORTA_DA_REDE, quantas = PORTAS_DA_REDE;
    const char *forcada = getenv("KATCHAU_PORTA");
    if (forcada != NULL) {
        int porta = atoi(forcada);
        if (porta > 0 && porta < 65536) {
            primeira = porta;
            quantas = 1;
        }
    }

    for (int tentativa = 0; tentativa < quantas; tentativa++) {
        Descritor descritor = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (descritor == SOQUETE_INVALIDO) {
            return NULL;   /* não é a porta: a pilha de rede não quer abrir nada */
        }

        /* Sem SO_BROADCAST o sistema recusa mandar para 255.255.255.255 */
        int ligado = 1;
        setsockopt(descritor, SOL_SOCKET, SO_BROADCAST, (const char *) &ligado, sizeof(ligado));
        /* Uns 256 kB de caixa de entrada: o padrão de 8 kB do Windows começa a descartar
         * cedo demais quando três convidados mandam ao mesmo tempo, e aí a perda parece
         * ser da rede quando é do nosso próprio soquete */
        int caixa = 256 * 1024;
        setsockopt(descritor, SOL_SOCKET, SO_RCVBUF, (const char *) &caixa, sizeof(caixa));
        calarOAvisoDePortaFechada(descritor);

        struct sockaddr_in meu;
        memset(&meu, 0, sizeof(meu));
        meu.sin_family = AF_INET;
        meu.sin_addr.s_addr = htonl(INADDR_ANY);   /* todas as placas: WiFi, cabo e VPN */
        meu.sin_port = htons((unsigned short) (primeira + tentativa));
        if (bind(descritor, (struct sockaddr *) &meu, sizeof(meu)) != 0) {
            fecharDescritor(descritor);
            continue;   /* porta ocupada: provavelmente outro KATCHAU neste PC */
        }
        if (!deixarNaoBloqueante(descritor)) {
            fecharDescritor(descritor);
            return NULL;
        }

        Soquete *soquete = (Soquete *) malloc(sizeof(Soquete));
        if (soquete == NULL) {
            fecharDescritor(descritor);
            return NULL;
        }
        soquete->descritor = descritor;
        soquete->porta = primeira + tentativa;
        return soquete;
    }
    return NULL;
}

void fecharSoquete(Soquete *soquete) {
    if (soquete == NULL) {
        return;
    }
    fecharDescritor(soquete->descritor);
    free(soquete);
}

int portaDoSoquete(const Soquete *soquete) {
    return (soquete == NULL) ? 0 : soquete->porta;
}

/*
 * enviarPara
 * Manda um pacote para um endereço certo.
 * Recebe: o soquete, o destino, os bytes e quantos são.
 * Devolve: 1 se saiu, 0 se não (a rede caiu, ou o pacote é grande demais).
 */
int enviarPara(Soquete *soquete, const EnderecoDaRede *destino, const void *dados, int tamanho) {
    if (soquete == NULL || destino == NULL || tamanho <= 0 || tamanho > MAIOR_PACOTE) {
        return 0;
    }
    struct sockaddr_in para;
    memset(&para, 0, sizeof(para));
    para.sin_family = AF_INET;
    para.sin_addr.s_addr = destino->endereco;
    para.sin_port = destino->porta;
    int enviados = (int) sendto(soquete->descritor, (const char *) dados, (size_t) tamanho, 0,
                                (struct sockaddr *) &para, sizeof(para));
    return enviados == tamanho;
}

/*
 * gritarNaRede
 * Grita o pacote para quem estiver ouvindo, sem saber o endereço de ninguém: é
 * assim que o convidado acha as salas e o dono anuncia a sua.
 *
 * Como cada jogo pega uma porta qualquer da faixa, o grito vai para as DEZ
 * portas dela, e em DOIS endereços cada:
 *   - 255.255.255.255, o broadcast limitado, para os outros PCs do WiFi;
 *   - 127.0.0.1, a própria máquina, porque broadcast nem sempre volta para si
 *     e dois jogos no mesmo PC é o teste que mais se faz.
 * São 20 datagramas de ~60 bytes por grito, e o grito acontece 2 vezes por
 * segundo: 2,4 kB/s. Em troca, ninguém precisa digitar porta nem IP.
 * Recebe: o soquete, os bytes e quantos são.
 * Devolve: 1 se pelo menos um datagrama saiu.
 */
int gritarNaRede(Soquete *soquete, const void *dados, int tamanho) {
    int foi = 0;
    for (int i = 0; i < PORTAS_DA_REDE; i++) {
        EnderecoDaRede todos, aqui;
        unsigned short porta = htons((unsigned short) (PRIMEIRA_PORTA_DA_REDE + i));
        todos.endereco = htonl(INADDR_BROADCAST);
        todos.porta = porta;
        aqui.endereco = htonl(INADDR_LOOPBACK);
        aqui.porta = porta;
        foi |= enviarPara(soquete, &todos, dados, tamanho);
        foi |= enviarPara(soquete, &aqui, dados, tamanho);
    }
    return foi;
}

/*
 * receber
 * Pega UM pacote da caixa de entrada, se houver.
 *
 * Quem chama deve chamar em laço até receber 0: num quadro podem ter chegado
 * vários pacotes, e deixar os antigos na caixa só aumenta o atraso.
 * Recebe: o soquete, onde guardar os bytes, o tamanho desse espaço e onde
 *         anotar quem mandou.
 * Devolve: quantos bytes chegaram, ou 0 se não havia nada (ou o pacote é lixo).
 */
int receber(Soquete *soquete, void *dados, int tamanho, EnderecoDaRede *remetente) {
    if (soquete == NULL || dados == NULL || tamanho <= 0) {
        return 0;
    }
    struct sockaddr_in de;
    TamanhoDeEndereco tamanhoDoEndereco = sizeof(de);
    memset(&de, 0, sizeof(de));
    int lidos = (int) recvfrom(soquete->descritor, (char *) dados, (size_t) tamanho, 0,
                               (struct sockaddr *) &de, &tamanhoDoEndereco);
    if (lidos <= 0) {
        /* Todo erro aqui vale "não chegou nada", e isso é de propósito. Os dois que
         * acontecem de verdade são o "esperaria bloquear" (caixa vazia, o caso normal
         * 60 vezes por segundo) e, no Windows, o WSAECONNRESET de alguém que fechou o
         * jogo — veja calarOAvisoDePortaFechada. Nenhum dos dois é motivo para o jogo
         * desistir da rede, e tratar os dois igual é a cerca mais simples. */
        return 0;
    }
    if (remetente != NULL) {
        remetente->endereco = de.sin_addr.s_addr;
        remetente->porta = de.sin_port;
    }
    return lidos;
}

int mesmoEndereco(const EnderecoDaRede *a, const EnderecoDaRede *b) {
    return a != NULL && b != NULL && a->endereco == b->endereco && a->porta == b->porta;
}

/*
 * textoDoEndereco
 * Recebe: o endereço e onde escrever.
 * Devolve: nada (escreve "192.168.0.7:25000" em destino).
 */
void textoDoEndereco(const EnderecoDaRede *endereco, char *destino, int tamanho) {
    if (destino == NULL || tamanho <= 0) {
        return;
    }
    if (endereco == NULL) {
        snprintf(destino, (size_t) tamanho, "-");
        return;
    }
    unsigned int cru = ntohl(endereco->endereco);
    snprintf(destino, (size_t) tamanho, "%u.%u.%u.%u:%u",
             (cru >> 24) & 0xFFu, (cru >> 16) & 0xFFu, (cru >> 8) & 0xFFu, cru & 0xFFu,
             (unsigned int) ntohs(endereco->porta));
}

/*
 * enderecoPorTexto
 * Monta um endereço a partir de um IP escrito ("192.168.0.7"), para quando o
 * broadcast não passa no WiFi da casa e o jogador digita o IP do dono.
 * Recebe: o texto, a porta e onde guardar.
 * Devolve: 1 se o texto é um IP válido, 0 se não.
 */
int enderecoPorTexto(const char *texto, int porta, EnderecoDaRede *destino) {
    if (texto == NULL || destino == NULL) {
        return 0;
    }
    unsigned int a = 0, b = 0, c = 0, d = 0;
    if (sscanf(texto, "%u.%u.%u.%u", &a, &b, &c, &d) != 4 || a > 255 || b > 255 || c > 255 || d > 255) {
        return 0;
    }
    destino->endereco = htonl((a << 24) | (b << 16) | (c << 8) | d);
    destino->porta = htons((unsigned short) porta);
    return 1;
}

/*
 * milissegundosDeAgora
 * Um relógio em milissegundos para medir o ping. Não precisa bater com o relógio
 * de ninguém: o número volta no eco e só a DIFERENÇA entre ida e volta é usada,
 * então basta ele andar sempre para a frente nesta máquina.
 * Recebe: nada.
 * Devolve: os milissegundos (dá a volta em ~49 dias, e uma volta no meio de uma
 *          medição estraga só aquele ping).
 */
unsigned int milissegundosDeAgora(void) {
#ifdef _WIN32
    return (unsigned int) GetTickCount();
#else
    struct timeval agora;
    gettimeofday(&agora, NULL);
    return (unsigned int) (agora.tv_sec * 1000u + agora.tv_usec / 1000u);
#endif
}

/*
 * nomeDoJogadorDoSistema
 * O nome com que o jogador aparece na sala: o usuário do PC, em maiúsculas (o
 * resto dos menus é todo em maiúsculas). Assim ninguém precisa digitar nada.
 * Recebe: onde escrever e o tamanho do espaço.
 * Devolve: nada.
 */
void nomeDoJogadorDoSistema(char *destino, int tamanho) {
    if (destino == NULL || tamanho <= 0) {
        return;
    }
    const char *nome = getenv("USERNAME");       /* Windows */
    if (nome == NULL || nome[0] == '\0') {
        nome = getenv("USER");                   /* Linux e macOS */
    }
    if (nome == NULL || nome[0] == '\0') {
        nome = "JOGADOR";
    }
    int i = 0;
    for (; nome[i] != '\0' && i < tamanho - 1; i++) {
        destino[i] = (char) toupper((unsigned char) nome[i]);
    }
    destino[i] = '\0';
}
