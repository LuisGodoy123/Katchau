/*
 * holofotes.h
 * ------------------------------------------------------------------
 * Os holofotes das corridas de noite nas pistas de dia (Interlagos,
 * Montreal e o Oval): poucas torres altas, de treliça, atrás do muro,
 * alternando os lados, com um painel de lâmpadas virado para o asfalto.
 * A luz delas é a das lâmpadas da cidade (hora.h): cada quadro leva as
 * que mais aparecem para a câmera, e o cenário e os carros as recebem no
 * shader. Aqui ficam as torres em 3D, as lâmpadas e o halo delas.
 * ------------------------------------------------------------------
 */

#ifndef HOLOFOTES_H
#define HOLOFOTES_H

#include "raylib.h"
#include "pista.h"

/* Liga as luzes dos holofotes da pista para esta câmera (desligado: nenhuma lâmpada) */
void acenderHolofotes(Pista *pista, Camera3D camera, int ligados);

/* As torres, as lâmpadas e o halo (dentro do BeginMode3D) */
void desenharHolofotes(Pista *pista, Camera3D camera);

/* Só as lâmpadas acesas, para a passada das luzes do pós-processamento (o halo nasce delas) */
void desenharLampadasDosHolofotes(Pista *pista, Camera3D camera);

void descarregarHolofotes(void);

#endif
