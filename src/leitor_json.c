/*
 * leitor_json.c
 * ------------------------------------------------------------------
 * Veja leitor_json.h.
 * ------------------------------------------------------------------
 */

#include "leitor_json.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Onde começa o valor de "chave" (depois dos dois-pontos), entre inicio e fim (NULL se não há) */
static const char *valorDaChave(const char *inicio, const char *fim, const char *chave) {
    char procurada[64];
    snprintf(procurada, sizeof(procurada), "\"%s\"", chave);
    size_t tamanho = strlen(procurada);
    for (const char *p = inicio; p + tamanho <= fim; p++) {
        if (strncmp(p, procurada, tamanho) != 0) {
            continue;
        }
        const char *q = p + tamanho;
        while (q < fim && (*q == ' ' || *q == '\t' || *q == '\n' || *q == '\r')) {
            q++;
        }
        if (q >= fim || *q != ':') {
            continue;   /* era um texto com esse nome, não uma chave */
        }
        q++;
        while (q < fim && (*q == ' ' || *q == '\t' || *q == '\n' || *q == '\r')) {
            q++;
        }
        return q;
    }
    return NULL;
}

float numeroDaChave(const char *inicio, const char *fim, const char *chave, float padrao) {
    const char *q = valorDaChave(inicio, fim, chave);
    if (q == NULL) {
        return padrao;
    }
    char *depois;
    float valor = strtof(q, &depois);
    return (depois != q) ? valor : padrao;
}

int numerosDaLista(const char *inicio, const char *fim, const char *chave, float *saida, int cabe) {
    const char *q = valorDaChave(inicio, fim, chave);
    if (q == NULL || *q != '[') {
        return 0;
    }
    q++;
    int lidos = 0;
    while (q < fim && *q != ']' && lidos < cabe) {
        char *depois;
        float valor = strtof(q, &depois);
        if (depois == q) {
            q++;
            continue;
        }
        saida[lidos++] = valor;
        q = depois;
    }
    return lidos;
}

int proximoObjeto(const char *texto, const char *chave, const char **cursor, const char **abre, const char **fecha) {
    const char *fim = texto + strlen(texto);
    if (*cursor == NULL) {
        const char *lista = valorDaChave(texto, fim, chave);
        if (lista == NULL || *lista != '[') {
            return 0;
        }
        *cursor = lista + 1;
    }
    const char *p = *cursor;
    while (p < fim && *p != '{' && *p != ']') {
        p++;
    }
    if (p >= fim || *p == ']') {
        return 0;
    }
    /* o fecha do objeto, contando as chaves de dentro */
    int fundo = 0;
    const char *q = p;
    for (; q < fim; q++) {
        if (*q == '{') {
            fundo++;
        } else if (*q == '}' && --fundo == 0) {
            break;
        }
    }
    if (q >= fim) {
        return 0;
    }
    *abre = p;
    *fecha = q;
    *cursor = q + 1;
    return 1;
}
