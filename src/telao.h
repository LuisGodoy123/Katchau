/*
 * telao.h
 * ------------------------------------------------------------------
 * Os telões e o placar do estádio da Copa Pistão: os telões passam a
 * transmissão da corrida (câmeras em vários carros e ângulos) e o placar
 * mostra a volta e a classificação de verdade.
 * ------------------------------------------------------------------
 */

#ifndef TELAO_H
#define TELAO_H

#include "raylib.h"
#include "corrida.h"

void carregarTelao(Model *estadio);
void descarregarTelao(void);
int telaoPrecisaDeQuadro(float tempoQuadro);
void comecarQuadroDoTelao(void);
void terminarQuadroDoTelao(Corrida *corrida, const Carro *mostrado, const char *camera);
void atualizarPlacar(Corrida *corrida);
Texture2D texturaDoTelao(void);
Shader shaderDaTelaQueBrilha(void);
void desenharTelasDoEstadio(void);

#endif
