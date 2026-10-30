#ifndef CONFIG_H
#define CONFIG_H

/* Números com nome usados em mais de um módulo (regra 5 do roadmap). */

/* Janela */
#define LARGURA_TELA     800
#define ALTURA_TELA      600
#define QUADROS_POR_SEG  60
#define TITULO_JANELA    "KATCHAU"

/* Corrida */
#define TOTAL_VOLTAS     3
#define NUMEROS_DA_CONTAGEM   3      /* 3, 2, 1... */
#define SEGUNDOS_POR_NUMERO   1.5f   /* quanto cada número fica na tela: mais tempo, mais tensão */
#define DURACAO_GRITO_LARGADA 1.5f   /* segundos em que o KATCHAU! fica na tela depois da largada */

#endif
