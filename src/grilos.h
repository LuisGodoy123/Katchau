/*
 * grilos.h
 * ------------------------------------------------------------------
 * O som da noite no passeio: grilos cantando, cada vez mais alto conforme
 * escurece. O som é gerado aqui mesmo (sem arquivo): alguns grilos, cada um
 * num tom e num ritmo, num laço de alguns segundos.
 * ------------------------------------------------------------------
 */

#ifndef GRILOS_H
#define GRILOS_H

void carregarGrilos(void);
void descarregarGrilos(void);
/* noite: 0 de dia, 1 de noite (o volume); ativo: 0 para calar (pausa, mapa, cenas) */
void atualizarGrilos(float noite, int ativo);

#endif
