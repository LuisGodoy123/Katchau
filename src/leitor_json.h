/*
 * leitor_json.h
 * ------------------------------------------------------------------
 * Um leitor pequeno dos JSON que as ferramentas gravam (as luzes, os
 * postes, os tratores): só os números das chaves e os objetos de uma lista.
 * Não é um leitor de JSON de verdade: serve para arquivos simples, com
 * chaves que não se repetem dentro de um mesmo objeto.
 * Este módulo NÃO usa a raylib.
 * ------------------------------------------------------------------
 */

#ifndef LEITOR_JSON_H
#define LEITOR_JSON_H

/* O número depois de "chave": entre inicio e fim (padrao se não houver) */
float numeroDaChave(const char *inicio, const char *fim, const char *chave, float padrao);

/* Os números de uma lista depois de "chave" ("chave": [a, b, c]): quantos leu (até cabe) */
int numerosDaLista(const char *inicio, const char *fim, const char *chave, float *saida, int cabe);

/* O próximo objeto { ... } da lista que começa em "chave" ("chave": [ {...}, {...} ]). Na primeira vez,
 * *cursor deve ser NULL; devolve 1 e o começo e o fim do objeto, ou 0 quando a lista acabou */
int proximoObjeto(const char *texto, const char *chave, const char **cursor, const char **abre, const char **fecha);

#endif
