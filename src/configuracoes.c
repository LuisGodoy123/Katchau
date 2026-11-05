/*
 * configuracoes.c
 * ------------------------------------------------------------------
 * As escolhas do jogador e o arquivo onde elas ficam. Veja configuracoes.h.
 * ------------------------------------------------------------------
 */

#include <stdio.h>
#include <string.h>

#include "configuracoes.h"

/* Os códigos das teclas e dos botões, iguais aos da raylib */
#define TECLA_A          65
#define TECLA_D          68
#define TECLA_S          83
#define TECLA_W          87
#define TECLA_ESPACO     32
#define TECLA_DIREITA    262
#define TECLA_ESQUERDA   263
#define TECLA_BAIXO      264
#define TECLA_CIMA       265
#define TECLA_SHIFT_E    340
#define TECLA_SHIFT_D    344
#define BOTAO_DIRECIONAL_ESQUERDA  4     /* GAMEPAD_BUTTON_LEFT_FACE_LEFT */
#define BOTAO_DIRECIONAL_DIREITA   2     /* GAMEPAD_BUTTON_LEFT_FACE_RIGHT */
#define BOTAO_FACE_DIREITA 6             /* B, bola */
#define BOTAO_FACE_BAIXO   7             /* A, cruz */
#define BOTAO_GATILHO_E    10            /* LT, L2 */
#define BOTAO_GATILHO_D    12            /* RT, R2 */

static const char *CHAVES_DAS_ACOES[TOTAL_ACOES] = { "acelerar", "frear", "esquerda", "direita", "nitro", "freio_de_mao" };
static const char *NOMES_DAS_ACOES[TOTAL_ACOES] = { "ACELERAR", "FREAR", "VIRAR À ESQUERDA", "VIRAR À DIREITA", "NITRO", "FREIO DE MÃO" };

/*
 * configuracoesAtuais
 * Recebe: nada.
 * Devolve: as configurações que o jogo está usando.
 */
Configuracoes *configuracoesAtuais(void) {
    static Configuracoes atuais;
    static int prontas = 0;
    if (!prontas) {
        configuracoesPadrao(&atuais);
        prontas = 1;
    }
    return &atuais;
}

/*
 * configuracoesPadrao
 * Recebe: as configurações a preencher.
 * Devolve: nada.
 */
void configuracoesPadrao(Configuracoes *config) {
    /* No controle, como nos jogos de corrida: R2 acelera, L2 freia, Cruz é o freio de
     * mão e Bola o nitro, um comando em cada botão. Os outros ficam livres para o jogador. */
    static const int PADRAO[TOTAL_ACOES][TOTAL_COLUNAS] = {
        { TECLA_CIMA,     TECLA_W,       BOTAO_GATILHO_D,           SEM_COMANDO },
        { TECLA_BAIXO,    TECLA_S,       BOTAO_GATILHO_E,           SEM_COMANDO },
        { TECLA_ESQUERDA, TECLA_A,       BOTAO_DIRECIONAL_ESQUERDA, SEM_COMANDO },
        { TECLA_DIREITA,  TECLA_D,       BOTAO_DIRECIONAL_DIREITA,  SEM_COMANDO },
        { TECLA_ESPACO,   SEM_COMANDO,   BOTAO_FACE_DIREITA,        SEM_COMANDO },
        { TECLA_SHIFT_E,  TECLA_SHIFT_D, BOTAO_FACE_BAIXO,          SEM_COMANDO },
    };
    memcpy(config->comandos, PADRAO, sizeof(PADRAO));
    config->vibracaoLigada = 1;
    config->forcaDaVibracao = 100;
    config->gatilhosAdaptativos = 1;
    config->zonaMorta = 12;
    config->respostaDoVolante = VOLANTE_NORMAL;
    config->direcaoGuiada = 1;
    config->inverterCameraX = 0;
    config->inverterCameraY = 0;
}

/*
 * primeiraColunaDoAparelho / colunasDoMesmoAparelho
 * Recebe: uma coluna.
 * Devolve: a primeira coluna do aparelho dela (teclado ou controle).
 */
static int primeiraColunaDoAparelho(int coluna) {
    return (coluna < COLUNA_BOTAO_1) ? COLUNA_TECLA_1 : COLUNA_BOTAO_1;
}

/*
 * definirComando
 * Põe uma tecla ou um botão num comando. Se ele já era de outro lugar, os dois trocam
 * de lugar: assim uma tecla nunca faz duas coisas, e nenhum comando fica sem tecla.
 * Recebe: as configurações, o comando, a coluna e o código novo.
 * Devolve: nada.
 */
void definirComando(Configuracoes *config, AcaoDeCorrida acao, ColunaDeComando coluna, int codigo) {
    int antigo = config->comandos[acao][coluna];
    int inicio = primeiraColunaDoAparelho(coluna);
    for (int a = 0; a < TOTAL_ACOES; a++) {
        for (int c = inicio; c < inicio + 2; c++) {
            if (config->comandos[a][c] == codigo) {
                config->comandos[a][c] = antigo;
            }
        }
    }
    config->comandos[acao][coluna] = codigo;
}

/*
 * limparComando
 * Tira uma tecla ou um botão de um comando, desde que sobre outro no mesmo aparelho.
 * Recebe: as configurações, o comando e a coluna.
 * Devolve: 1 se limpou, 0 se era o último daquele comando.
 */
int limparComando(Configuracoes *config, AcaoDeCorrida acao, ColunaDeComando coluna) {
    int inicio = primeiraColunaDoAparelho(coluna);
    int outra = ((int) coluna == inicio) ? inicio + 1 : inicio;
    if (config->comandos[acao][outra] == SEM_COMANDO) {
        return 0;
    }
    config->comandos[acao][coluna] = SEM_COMANDO;
    return 1;
}

/*
 * salvarConfiguracoes
 * Recebe: as configurações e o nome do arquivo.
 * Devolve: 1 se salvou.
 */
int salvarConfiguracoes(const Configuracoes *config, const char *arquivo) {
    FILE *saida = fopen(arquivo, "w");
    if (saida == NULL) {
        return 0;
    }
    fprintf(saida, "# Configuracoes do KATCHAU. Apague este arquivo para voltar ao padrao.\n");
    for (int a = 0; a < TOTAL_ACOES; a++) {
        fprintf(saida, "comando %s %d %d %d %d\n", CHAVES_DAS_ACOES[a], config->comandos[a][0], config->comandos[a][1],
                config->comandos[a][2], config->comandos[a][3]);
    }
    fprintf(saida, "vibracao %d\nforca %d\ngatilhos %d\nzona_morta %d\nvolante %d\ndirecao_guiada %d\ncamera_x %d\ncamera_y %d\n",
            config->vibracaoLigada, config->forcaDaVibracao, config->gatilhosAdaptativos, config->zonaMorta,
            config->respostaDoVolante, config->direcaoGuiada, config->inverterCameraX, config->inverterCameraY);
    fclose(saida);
    return 1;
}

/*
 * valorValido
 * Recebe: o valor, o mínimo e o máximo, e onde guardar.
 * Devolve: nada (fora dos limites, fica o que estava).
 */
static void valorValido(int valor, int minimo, int maximo, int *destino) {
    if (valor >= minimo && valor <= maximo) {
        *destino = valor;
    }
}

/*
 * carregarConfiguracoes
 * Lê o arquivo. O que faltar ou vier estragado fica no padrão, para um arquivo mexido
 * à mão nunca impedir o jogo de abrir, nem deixar um comando sem tecla.
 * Recebe: onde guardar e o nome do arquivo.
 * Devolve: 1 se o arquivo existia.
 */
int carregarConfiguracoes(Configuracoes *config, const char *arquivo) {
    configuracoesPadrao(config);
    FILE *entrada = fopen(arquivo, "r");
    if (entrada == NULL) {
        return 0;
    }
    Configuracoes padrao = *config;
    char linha[128], chave[32], acao[32];
    while (fgets(linha, sizeof(linha), entrada) != NULL) {
        int v[TOTAL_COLUNAS];
        int valor;
        if (sscanf(linha, "comando %31s %d %d %d %d", acao, &v[0], &v[1], &v[2], &v[3]) == 5) {
            for (int a = 0; a < TOTAL_ACOES; a++) {
                if (strcmp(acao, CHAVES_DAS_ACOES[a]) == 0) {
                    for (int c = 0; c < TOTAL_COLUNAS; c++) {
                        valorValido(v[c], 0, 1000, &config->comandos[a][c]);
                    }
                }
            }
        } else if (sscanf(linha, "%31s %d", chave, &valor) == 2) {
            if (strcmp(chave, "vibracao") == 0) {
                valorValido(valor, 0, 1, &config->vibracaoLigada);
            } else if (strcmp(chave, "forca") == 0) {
                valorValido(valor, MENOR_FORCA_DA_VIBRACAO, 100, &config->forcaDaVibracao);
            } else if (strcmp(chave, "gatilhos") == 0) {
                valorValido(valor, 0, 1, &config->gatilhosAdaptativos);
            } else if (strcmp(chave, "zona_morta") == 0) {
                valorValido(valor, MENOR_ZONA_MORTA, MAIOR_ZONA_MORTA, &config->zonaMorta);
            } else if (strcmp(chave, "volante") == 0) {
                valorValido(valor, 0, TOTAL_VOLANTES - 1, &config->respostaDoVolante);
            } else if (strcmp(chave, "direcao_guiada") == 0) {
                valorValido(valor, 0, 1, &config->direcaoGuiada);
            } else if (strcmp(chave, "camera_x") == 0) {
                valorValido(valor, 0, 1, &config->inverterCameraX);
            } else if (strcmp(chave, "camera_y") == 0) {
                valorValido(valor, 0, 1, &config->inverterCameraY);
            }
        }
    }
    fclose(entrada);

    /* Nenhum comando pode ficar sem tecla, nem sem botão */
    for (int a = 0; a < TOTAL_ACOES; a++) {
        for (int inicio = COLUNA_TECLA_1; inicio <= COLUNA_BOTAO_1; inicio += 2) {
            if (config->comandos[a][inicio] == SEM_COMANDO && config->comandos[a][inicio + 1] == SEM_COMANDO) {
                config->comandos[a][inicio] = padrao.comandos[a][inicio];
                config->comandos[a][inicio + 1] = padrao.comandos[a][inicio + 1];
            }
        }
    }
    return 1;
}

/*
 * nomeDaAcao
 * Recebe: o comando.
 * Devolve: o nome dele, para a tela.
 */
const char *nomeDaAcao(AcaoDeCorrida acao) {
    return NOMES_DAS_ACOES[acao];
}
