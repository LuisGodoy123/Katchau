/*
 * vibrar.h
 * ------------------------------------------------------------------
 * Manda a vibração para o controle. A raylib não sabe fazer isso no Mac, então no
 * macOS quem fala com o DualSense é o vibrar_mac.m, pelo sistema de controles da
 * Apple. Nos outros sistemas, vibrar_outros.c não faz nada.
 * No macOS, o touchpad do PlayStation também é lido por aqui (touchpadDoControleApertado).
 * ------------------------------------------------------------------
 */

#ifndef VIBRAR_H
#define VIBRAR_H

#include "vibracao.h"

void iniciarVibracao(void);
void aplicarVibracao(const Vibracao *vibracao);
void pararVibracao(void);
void encerrarVibracao(void);

#ifdef __APPLE__
int touchpadDoControleApertado(void);
#endif

#endif
