/*
 * luzes.h
 * ------------------------------------------------------------------
 * As luzes da cidade à noite: os neons, as lâmpadas, as janelas acesas e o
 * semáforo de cada lugar de Radiator Springs (recursos/modelos/luzes/
 * <lugar>.glb, feitos pelas ferramentas de ferramentas/luzes/), e as luzes pontuais que
 * eles jogam em volta (<lugar>.json). No pôr do sol a cidade acende: cada
 * lugar na sua vez, cada letra e cada lâmpada na ordem dela (o alfa da cor
 * do vértice), piscando um pouco ao ligar; no amanhecer, apagam. De dia os
 * tubos aparecem apagados, como vidro colorido.
 *
 * Também as luzes que a noite dos interiores joga para fora
 * (recursos/modelos/<interior>_noite.json).
 * ------------------------------------------------------------------
 */

#ifndef LUZES_H
#define LUZES_H

#include "raylib.h"
#include "colisao.h"
#include "hora.h"

void carregarLuzesDaCidade(MalhaDeColisao *malha);   /* a malha: para a luz não atravessar as paredes */
void descarregarLuzesDaCidade(void);

/* Um passo: "acesas" é o quanto a hora pede as luzes (0 de dia, 1 de noite, luzDoDia->luzesDaCidade).
 * pular: sem a sequência (o começo do passeio, depois de dormir) */
void atualizarLuzesDaCidade(float acesas, float tempoQuadro, int pular);

/* Dentro do BeginMode3D, depois do mapa: os tubos (apagados de dia, acesos de noite). "luzDoDia" é a
 * luz que cai no tubo apagado (a base da luz da hora) */
void desenharLuzesDaCidade(Camera3D camera, Vector3 luzDoDia);

/* No passe das luzes do pós-processo (o brilho em volta): só o que está aceso */
void desenharHalosDaCidade(Camera3D camera);

/* Liga a imagem de até onde cada lâmpada enxerga (antes de desenhar o mapa e os carros) */
void ligarVistaDasLampadas(void);

/* Os conjuntos que andam (as lâmpadas do Mack, o lado de dentro da carreta): registrar depois de
 * carregarLuzesDaCidade (devolve o número, -1 se não deu) e, a cada quadro, dizer onde estão e se aparecem */
int registrarLuzesMoveis(const char *glb, const char *json, int sempreAceso);
void moverLuzes(int qual, Matrix matriz, int visiveis);
/* Tira de um conjunto que anda as lâmpadas (as ilhas da malha) com o meio além de zMinimo no eixo z do
 * modelo, num conjunto novo, que anda à parte (a cabine do Mack). Devolve o número dele (-1 se não deu) */
int separarLuzesMoveis(int qual, float zMinimo);

/* As luzes pontuais acesas agora (da cidade e dos interiores), com a força de agora. Devolve quantas */
int lampadasAcesas(Lampada *saida, int cabe);

/* Das lâmpadas dadas, as "cabe" que mais aparecem para a câmera (perto e na frente dela). Devolve quantas */
int escolherLampadas(Camera3D camera, const Lampada *todas, int total, Lampada *saida, int cabe);

/* Brilhos soltos: um borrão de luz somando ao que está atrás */
void comecarBrilhos(void);
void desenharBrilhoNoAr(Camera3D camera, Vector3 lugar, float tamanho, Color cor);
void terminarBrilhos(void);

#endif
