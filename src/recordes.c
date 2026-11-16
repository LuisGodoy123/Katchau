/*
 * recordes.c
 * ------------------------------------------------------------------
 * Os recordes de volta. Veja recordes.h.
 * ------------------------------------------------------------------
 */

#include <stdio.h>
#include <string.h>
#include "recordes.h"

static float tempos[TOTAL_PISTAS];                                   /* 0 = ainda não há recorde */
static char donos[TOTAL_PISTAS][TAMANHO_DO_NOME_DO_RECORDE];

/* O número de cada pista no arquivo. O 3 era o Splat, que deu lugar a Montreal: um recorde dele que ainda esteja
 * num arquivo antigo não vale para a pista nova (a volta de Montreal é bem mais longa) */
static const int CODIGO_NO_ARQUIVO[TOTAL_PISTAS] = { [PISTA_INTERLAGOS] = 0, [PISTA_COPA_PISTAO] = 1, [PISTA_OVAL] = 2,
                                                     [PISTA_MONTREAL] = 4 };

/*
 * gravarRecordes
 * Escreve todos os recordes no arquivo. Se não der para abrir, os recordes valem só até fechar o jogo.
 * Recebe: nada.
 * Devolve: nada.
 */
static void gravarRecordes(void) {
    FILE *arquivo = fopen(ARQUIVO_DE_RECORDES, "w");
    if (arquivo == NULL) {
        return;
    }
    for (int i = 0; i < TOTAL_PISTAS; i++) {
        if (tempos[i] > 0.0f) {
            fprintf(arquivo, "%d %.3f %s\n", CODIGO_NO_ARQUIVO[i], tempos[i], donos[i]);
        }
    }
    fclose(arquivo);
}

/*
 * carregarRecordes
 * Lê o arquivo de recordes (se existir). Linhas estragadas são ignoradas.
 * Recebe: nada.
 * Devolve: nada.
 */
void carregarRecordes(void) {
    memset(tempos, 0, sizeof(tempos));
    memset(donos, 0, sizeof(donos));
    FILE *arquivo = fopen(ARQUIVO_DE_RECORDES, "r");
    if (arquivo == NULL) {
        return;
    }
    int codigo;
    float tempo;
    char nome[TAMANHO_DO_NOME_DO_RECORDE];
    while (fscanf(arquivo, "%d %f %31[^\n]", &codigo, &tempo, nome) == 3) {
        int id = -1;
        for (int i = 0; i < TOTAL_PISTAS; i++) {
            if (CODIGO_NO_ARQUIVO[i] == codigo) {
                id = i;
            }
        }
        if (id >= 0 && tempo > 0.0f) {
            tempos[id] = tempo;
            snprintf(donos[id], sizeof(donos[id]), "%s", nome);
        }
    }
    fclose(arquivo);
}

/*
 * recordeDaPista
 * Recebe: a pista, onde guardar o nome do carro recordista (pode ser NULL) e o tamanho disso.
 * Devolve: o recorde em segundos, ou 0 se ninguém fez volta nessa pista ainda.
 */
float recordeDaPista(IdPista id, char *dono, int tamanhoDoDono) {
    if (id < 0 || id >= TOTAL_PISTAS) {
        return 0.0f;
    }
    if (dono != NULL && tamanhoDoDono > 0) {
        snprintf(dono, (size_t) tamanhoDoDono, "%s", donos[id]);
    }
    return tempos[id];
}

/*
 * registrarRecorde
 * Se o tempo é melhor que o recorde da pista (ou não havia recorde), ele vira o recorde e é gravado.
 * Recebe: a pista, o tempo da volta em segundos e o nome do carro.
 * Devolve: 1 se foi um recorde novo, 0 se não.
 */
int registrarRecorde(IdPista id, float tempo, const char *dono) {
    if (id < 0 || id >= TOTAL_PISTAS || tempo <= 0.0f) {
        return 0;
    }
    if (tempos[id] > 0.0f && tempo >= tempos[id]) {
        return 0;
    }
    tempos[id] = tempo;
    snprintf(donos[id], sizeof(donos[id]), "%s", dono != NULL ? dono : "");
    gravarRecordes();
    return 1;
}
