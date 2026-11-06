/*
 * fumaca.h
 * ------------------------------------------------------------------
 * A fumaça dos pneus e a poeira da grama: nuvens soltas no mundo 3D. Cada
 * uma nasce na roda, fica para trás, sobe, cresce e some. O carro segue
 * em frente e a nuvem continua no lugar onde nasceu.
 * Aqui também fica a chama azul do nitro, que sai do escapamento.
 * ------------------------------------------------------------------
 */

#ifndef FUMACA_H
#define FUMACA_H

#include "raylib.h"

typedef enum {
    NUVEM_DE_PNEU,     /* branca e densa: derrapagem e arrancada */
    NUVEM_DE_POEIRA,   /* marrom: roda na grama */
    NUVEM_DE_DRIFT     /* branca, grande e demorada: o carro de lado */
} TipoDeNuvem;

void carregarFumaca(void);
void descarregarFumaca(void);
void limparFumaca(void);
void soltarNuvem(Vector3 roda, Vector3 velocidadeDoCarro, float forca, TipoDeNuvem tipo);
void atualizarFumaca(float tempoQuadro);
void desenharFumaca3D(Camera3D camera);
void comecarChamas(void);
void terminarChamas(void);
void desenharChamaDoNitro(Camera3D camera, Vector3 escape, Vector3 paraTras);

#endif
