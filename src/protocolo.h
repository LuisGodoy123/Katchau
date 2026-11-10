/*
 * protocolo.h
 * ------------------------------------------------------------------
 * O que viaja pela rede no modo online, e como virar bytes.
 *
 * Duas regras valem para tudo aqui:
 *
 *   1) NUNCA se copia uma struct para o fio. O compilador põe enchimento entre
 *      os campos onde quiser, e o tamanho de uma struct não é combinado entre
 *      dois programas compilados em máquinas diferentes. Cada campo é escrito
 *      e lido byte a byte, em little-endian, pelas funções deste módulo. Sai
 *      mais barato que depender da ordem de bytes do computador, e o pacote
 *      fica igual no Windows, no Linux e num ARM.
 *
 *   2) Todo pacote começa com o MESMO cabeçalho, e esse cabeçalho está
 *      congelado para sempre. É o que permite a uma versão nova reconhecer uma
 *      sala de versão velha e dizer "OUTRA VERSÃO" na tela, em vez de ignorar
 *      os pacotes em silêncio e o jogador achar que a rede não funciona.
 *
 * Este módulo NÃO usa a raylib.
 * ------------------------------------------------------------------
 */

#ifndef PROTOCOLO_H
#define PROTOCOLO_H

#include "carro.h"
#include "corrida.h"
#include "pista.h"

#define MARCA_DO_KATCHAU      0x4B544348u   /* 'K','T','C','H': o que não começa com isto é descartado sem olhar */
#define VERSAO_DO_PROTOCOLO   1
#define BYTES_DO_CABECALHO    8

/* MAXIMO_DE_JOGADORES fica em config.h: a corrida também precisa dele */
#define MAXIMO_DE_SALAS       12    /* quantas a lista da tela guarda */
#define TAMANHO_DO_NOME_DA_SALA 24
#define TAMANHO_DO_APELIDO    16    /* com o '\0'; cabe em TAMANHO_NOME (20), que é o do Carro */
#define QUADROS_DE_COMANDO    4     /* quantos quadros de comando cada pacote leva (veja abaixo) */
#define EVENTOS_POR_PACOTE    8

/* As constantes que, se forem diferentes nas duas máquinas, fazem um retrato ser
 * lido errado e os carros voarem pela tela. Vai no pedido de entrar: uma sala de
 * build incompatível é recusada com motivo, em vez de dar uma corrida maluca. */
#define MARCA_DA_COMPILACAO ((unsigned int) (VERSAO_DO_PROTOCOLO * 1000003u \
    + TOTAL_CARROS * 10007u + TOTAL_PISTAS * 101u + TOTAL_NIVEIS * 17u \
    + TOTAL_SETORES * 7u + MAXIMO_DE_VOLTAS))

typedef enum {
    PACOTE_PROCURAR  = 1,   /* convidado -> a rede toda: "tem alguma sala aí?" */
    PACOTE_ANUNCIO   = 2,   /* dono -> a rede toda, 2x por segundo */
    PACOTE_ENTRAR    = 3,   /* convidado -> dono */
    PACOTE_ACEITOU   = 4,
    PACOTE_RECUSOU   = 5,
    PACOTE_SALA      = 6,   /* dono -> todos, 5x por segundo: a sala inteira */
    PACOTE_ESCOLHA   = 7,   /* convidado -> dono: o carro que quer e se está pronto */
    PACOTE_LARGAR    = 8,   /* dono -> todos: comecem a corrida */
    PACOTE_COMANDOS  = 9,   /* convidado -> dono, a cada quadro */
    PACOTE_QUADRO    = 10,  /* dono -> todos, 30x por segundo: o retrato dos 14 carros */
    PACOTE_EVENTO    = 11,  /* dono -> todos: volta, chegada, batida, rodada, pit */
    PACOTE_SAIR      = 12,
    PACOTE_PING      = 13,
    PACOTE_PONGUE    = 14
} TipoDePacote;

typedef enum {
    RECUSA_SALA_CHEIA = 1,
    RECUSA_VERSAO,
    RECUSA_EM_CORRIDA
} MotivoDaRecusa;

/* Os acontecimentos que não podem viajar no retrato: cada um é consumido UMA vez
 * (o som da batida, a vibração, a fala do McQueen, o tempo da volta), e num
 * retrato a 30 Hz um pacote perdido os perderia para sempre. Vão repetidos em
 * quadros seguidos e são deduplicados pelo id. */
typedef enum {
    EVENTO_VOLTA = 1,       /* inteiro = a volta, valor = o tempo dela */
    EVENTO_CHEGADA,         /* inteiro = a posição final, valor = tempoDeChegada */
    EVENTO_PIT_FEITO,       /* valor = quanto durou a troca */
    EVENTO_BATIDA,          /* inteiro = 1 se foi contra outro carro, valor = a força (0 a 1) */
    EVENTO_RODADA,          /* valor = a velocidade em que rodou */
    EVENTO_PNEU_ESTOUROU
} TipoDeEvento;

/* O cabeçalho de todo pacote. ESTE layout nunca muda. */
typedef struct {
    unsigned int marca;
    unsigned char versao;
    unsigned char tipo;
    unsigned short sequencia;   /* cresce 1 por pacote deste lado; dá a volta em 65535 */
} CabecalhoDeRede;

/* ---------- O carro no fio: 36 bytes ----------
 * Só o que o outro lado não consegue deduzir. Tudo o que é enfeite (o motor, as
 * rodas girando, a suspensão, o que a IA enxerga) é recalculado em cada máquina:
 * veja o comentário de aplicarRetratoNoCarro, em online.c. */
typedef struct {
    unsigned short indiceDoSegmento;
    unsigned short distanciaNoSegmento;   /* 0..COMPRIMENTO_SEGMENTO, em 65535 passos (~3 mm) */
    short deslocamentoLateral;            /* x8000; a faixa útil é +-4 */
    unsigned short velocidade;            /* x4: cabe até 16383 un/s, e o carro mais rápido faz 12120 */
    unsigned short rumoNoMundo;           /* graus normalizados x(65536/360) */
    short anguloDeDrift;                  /* graus x100 */
    short giro;                           /* graus x100: a rodada */
    signed char volante;                  /* x127: as rodas viradas, no desenho */
    unsigned char nitro;                  /* x255 */
    unsigned char derrapagem;             /* x255: fumaça e som de pneu */
    unsigned char forcaDoDrift;           /* x255 */
    unsigned char estado;                 /* os bits abaixo */
    unsigned char pit;                    /* estadoDoPit (2 bits) | pneuEstourado (4) | ladoDoPrimeiroEstouro (1) */
    signed char voltas;                   /* -1 a 8: AUTORITATIVO, é a contagem de voltas */
    unsigned char desgaste;               /* x255 */
    unsigned char contato;                /* x255: a força do contato deste quadro (faíscas e som de lata) */
    unsigned char vacuo;                  /* x255 */
    /* 7 u16 (14) + 10 u8 (10) + 3 float (12) = 36 bytes. O teste teste_rede.c confere esta
     * conta contra o que escreverCarroNoFio grava de verdade: errar por um byte estoura o
     * buffer do retrato no dono, 30 vezes por segundo, e não aparece de outra forma. */
    float distanciaPercorrida;            /* a chave da classificação e dos intervalos */
    float tempoDeChegada;
    float melhorVolta;
} CarroNoFio;

#define BYTES_DO_CARRO_NO_FIO  36

/* Os bits de "estado" do CarroNoFio */
#define FIO_NITRO_LIGADO     0x01
#define FIO_RODANDO          0x02
#define FIO_RASPANDO_NO_MURO 0x04
#define FIO_ACELERANDO       0x08
#define FIO_FREANDO          0x10
#define FIO_PIT_FEITO        0x20
#define FIO_CONTATO_DE_CARRO 0x40
#define FIO_DIRECAO_LIVRE    0x80

/* Os bits dos comandos, num byte só */
#define BIT_ACELERAR     0x01
#define BIT_FREAR        0x02
#define BIT_ESQUERDA     0x04
#define BIT_DIREITA      0x08
#define BIT_NITRO        0x10
#define BIT_FREIO_DE_MAO 0x20

/* ---------- Escrever e ler números no fio ---------- */
void escreverU8(unsigned char *fio, int *onde, unsigned int valor);
void escreverU16(unsigned char *fio, int *onde, unsigned int valor);
void escreverU32(unsigned char *fio, int *onde, unsigned int valor);
void escreverF32(unsigned char *fio, int *onde, float valor);
void escreverTexto(unsigned char *fio, int *onde, const char *texto, int quantos);
unsigned int lerU8(const unsigned char *fio, int *onde);
unsigned int lerU16(const unsigned char *fio, int *onde);
unsigned int lerU32(const unsigned char *fio, int *onde);
float lerF32(const unsigned char *fio, int *onde);
void lerTexto(const unsigned char *fio, int *onde, char *destino, int quantos);

/* ---------- Cabeçalho ---------- */
int escreverCabecalho(unsigned char *fio, TipoDePacote tipo, unsigned short sequencia);
int lerCabecalho(const unsigned char *fio, int tamanho, CabecalhoDeRede *cabecalho);
int sequenciaEMaisNova(unsigned short nova, unsigned short ultima);

/* ---------- Comandos ---------- */
unsigned char bitsDosComandos(const Comandos *comandos);
Comandos comandosDosBits(unsigned char bits, signed char direcao);
signed char direcaoNoFio(float direcao);

/* ---------- O carro ---------- */
void guardarCarroNoFio(const Carro *carro, CarroNoFio *fio);
void escreverCarroNoFio(unsigned char *fio, int *onde, const CarroNoFio *carro);
void lerCarroDoFio(const unsigned char *fio, int *onde, CarroNoFio *carro);

#endif
