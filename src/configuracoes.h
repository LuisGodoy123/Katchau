/*
 * configuracoes.h
 * ------------------------------------------------------------------
 * O que o jogador escolhe na tela de configurações: as teclas e os botões de
 * cada comando e as opções do controle. Fica salvo em um arquivo de texto.
 *
 * Este módulo NÃO usa a raylib: os códigos das teclas e dos botões são
 * apenas números (os mesmos da raylib), e quem os lê é o controle.c.
 * ------------------------------------------------------------------
 */

#ifndef CONFIGURACOES_H
#define CONFIGURACOES_H

#define ARQUIVO_DE_CONFIGURACOES "configuracoes.txt"

/* Os comandos da corrida que dá para trocar. Pausar e os menus são fixos. */
typedef enum {
    ACAO_ACELERAR,
    ACAO_FREAR,
    ACAO_ESQUERDA,
    ACAO_DIREITA,
    ACAO_NITRO,
    ACAO_FREIO_DE_MAO,
    TOTAL_ACOES
} AcaoDeCorrida;

/* Cada comando tem duas teclas e dois botões: as 4 colunas da tela */
typedef enum {
    COLUNA_TECLA_1,
    COLUNA_TECLA_2,
    COLUNA_BOTAO_1,
    COLUNA_BOTAO_2,
    TOTAL_COLUNAS
} ColunaDeComando;

#define SEM_COMANDO  0    /* KEY_NULL e GAMEPAD_BUTTON_UNKNOWN, na raylib */

/* O jeito do volante no analógico */
typedef enum {
    VOLANTE_SUAVE,        /* bem fino perto do centro */
    VOLANTE_NORMAL,
    VOLANTE_DIRETO,       /* quanto inclina, tanto vira */
    TOTAL_VOLANTES
} RespostaDoVolante;

#define MENOR_FORCA_DA_VIBRACAO   10
#define PASSO_DA_FORCA            10
#define MENOR_ZONA_MORTA          2
#define MAIOR_ZONA_MORTA          30
#define PASSO_DA_ZONA_MORTA       2

typedef struct {
    int comandos[TOTAL_ACOES][TOTAL_COLUNAS];
    int vibracaoLigada;
    int forcaDaVibracao;      /* porcentagem, de 10 a 100 */
    int gatilhosAdaptativos;  /* 1: o gatilho do acelerador treme com a roda patinando (DualSense) */
    int zonaMorta;            /* porcentagem do analógico que conta como "reto" */
    int respostaDoVolante;    /* RespostaDoVolante */
    int inverterCameraX;
    int inverterCameraY;
} Configuracoes;

Configuracoes *configuracoesAtuais(void);
void configuracoesPadrao(Configuracoes *config);
void definirComando(Configuracoes *config, AcaoDeCorrida acao, ColunaDeComando coluna, int codigo);
int limparComando(Configuracoes *config, AcaoDeCorrida acao, ColunaDeComando coluna);
int salvarConfiguracoes(const Configuracoes *config, const char *arquivo);
int carregarConfiguracoes(Configuracoes *config, const char *arquivo);
const char *nomeDaAcao(AcaoDeCorrida acao);

#endif
