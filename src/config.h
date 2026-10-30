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
#define ESPERA_ANTES_DA_CONTAGEM 1.2f  /* segundos só com o som do motor, antes do 3 aparecer */
#define NUMEROS_DA_CONTAGEM   3      /* 3, 2, 1... */
#define SEGUNDOS_POR_NUMERO   1.0f   /* quanto cada número fica na tela (o som da contagem tem 1 bip por segundo) */
#define DURACAO_GRITO_LARGADA 1.5f   /* segundos em que o KATCHAU! fica na tela depois da largada */

#endif
