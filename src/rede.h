/*
 * rede.h
 * ------------------------------------------------------------------
 * A casca do socket: mandar e receber pacotes UDP na rede local.
 *
 * É o ÚNICO arquivo do jogo que sabe que existem sistemas operacionais
 * diferentes (Winsock2 no Windows, sockets BSD no Linux e no macOS).
 * Quem está acima daqui — protocolo.c, sala.c, online.c — só vê pacotes
 * de bytes indo e vindo.
 *
 * Tudo é NÃO BLOQUEANTE, de propósito: o jogo tem 16 ms por quadro para
 * desenhar, e não pode esperar a rede. "receber" devolve 0 quando não há
 * nada para ler, e quem chama simplesmente segue o quadro. Nenhuma thread:
 * o laço principal esvazia a caixa de entrada uma vez por quadro.
 *
 * Por que UDP e não TCP: numa corrida, um pacote de posição que chega
 * atrasado não serve para nada — o certo é jogá-lo fora e usar o próximo.
 * O TCP faria o contrário: pararia tudo para reentregar o pacote velho.
 *
 * Este módulo NÃO usa a raylib.
 * ------------------------------------------------------------------
 */

#ifndef REDE_H
#define REDE_H

/* A faixa de portas do jogo. Cada processo pega a PRIMEIRA LIVRE da faixa, e tanto o farol do
 * dono quanto a procura do convidado são jogados nas dez portas. Custa 20 datagramas por segundo
 * (nada) e dá uma coisa que uma porta fixa não dá: quantos jogos se quiser no mesmo PC, donos
 * inclusive, sem variável de ambiente e sem um segundo soquete só para o farol. */
#define PRIMEIRA_PORTA_DA_REDE  50505
#define PORTAS_DA_REDE          10
#define MAIOR_PACOTE            1200    /* bytes: abaixo da MTU de qualquer rede, então nenhum pacote nosso é picado */
#define PACOTES_POR_QUADRO      64      /* o teto de leitura por quadro: um pico de tráfego não pode virar travamento */

/* De onde veio um pacote (ou para onde mandar um). O IPv4 e a porta ficam
 * guardados na ordem de bytes da rede, como o sistema os entrega: assim esta
 * struct não precisa dos headers do sistema para existir. */
typedef struct {
    unsigned int endereco;
    unsigned short porta;
} EnderecoDaRede;

/* O soquete em si. O conteúdo fica escondido em rede.c. */
typedef struct Soquete Soquete;

/* ---------- Abrir e fechar ---------- */
int iniciarRede(void);          /* 1 se a rede está disponível; no Windows é o WSAStartup */
void encerrarRede(void);
Soquete *abrirSoquete(void);    /* pega a primeira porta livre da faixa */
void fecharSoquete(Soquete *soquete);
int portaDoSoquete(const Soquete *soquete);

/* ---------- Mandar e receber ---------- */
int enviarPara(Soquete *soquete, const EnderecoDaRede *destino, const void *dados, int tamanho);
int gritarNaRede(Soquete *soquete, const void *dados, int tamanho);   /* para a faixa toda, em broadcast e no loopback */
int receber(Soquete *soquete, void *dados, int tamanho, EnderecoDaRede *remetente);

/* ---------- Endereços ---------- */
int mesmoEndereco(const EnderecoDaRede *a, const EnderecoDaRede *b);
void textoDoEndereco(const EnderecoDaRede *endereco, char *destino, int tamanho);
int enderecoPorTexto(const char *texto, int porta, EnderecoDaRede *destino);
void nomeDoJogadorDoSistema(char *destino, int tamanho);   /* o usuário do PC, em maiúsculas */
unsigned int milissegundosDeAgora(void);                   /* o relógio que mede o ping (só serve para diferenças) */

#endif
