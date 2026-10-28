/*
 * main.c
 * ------------------------------------------------------------------
 * Ponto de entrada do KATCHAU.
 * Fase 4: o jogador corre 3 voltas contra 7 rivais da IA; ao final
 *         aparece o painel de chegada e ENTER começa outra corrida.
 * Na Fase 6 este arquivo vai trocar entre as telas (menu, garagem,
 * corrida, resultado).
 *
 * O laço principal segue sempre a mesma ordem, uma vez por quadro:
 *   1) ler o teclado e transformar em Comandos;
 *   2) atualizar a lógica (corrida.c, carro.c, pista.c);
 *   3) desenhar (render.c, hud.c).
 * ------------------------------------------------------------------
 */

#include "raylib.h"
#include "config.h"
#include "pista.h"
#include "carro.h"
#include "corrida.h"
#include "render.h"
#include "hud.h"

/*
 * lerComandosDoTeclado
 * Traduz as teclas apertadas para os comandos do carro.
 * Setas ou WASD funcionam.
 * Recebe: nada.
 * Devolve: os comandos deste quadro.
 */
static Comandos lerComandosDoTeclado(void) {
    Comandos comandos;
    comandos.acelerar = IsKeyDown(KEY_UP) || IsKeyDown(KEY_W);
    comandos.frear = IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S);
    comandos.esquerda = IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A);
    comandos.direita = IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D);
    return comandos;
}

int main(void) {
    InitWindow(LARGURA_TELA, ALTURA_TELA, TITULO_JANELA);
    SetTargetFPS(QUADROS_POR_SEG);

    Pista *pista = criarPista();
    Corrida corrida;
    iniciarCorrida(&corrida, pista);

    /* Laço principal: roda uma vez por quadro até a janela fechar (ESC) */
    while (!WindowShouldClose()) {
        /* Tempo do último quadro, para a velocidade não depender do FPS */
        float tempoQuadro = GetFrameTime();

        /* ---- 1) Ler o teclado ---- */
        Comandos comandos = lerComandosDoTeclado();
        if (corrida.terminou && IsKeyPressed(KEY_ENTER)) {
            iniciarCorrida(&corrida, pista);
        }

        /* ---- 2) Atualizar ---- */
        atualizarCorrida(&corrida, comandos, tempoQuadro);

        /* ---- 3) Desenhar ---- */
        BeginDrawing();
        ClearBackground(BLACK);
        desenharVisaoDoJogador(&corrida);
        desenharHud(&corrida);
        if (corrida.terminou) {
            desenharFimDeCorrida(&corrida);
        }
        EndDrawing();
    }

    liberarPista(pista);
    CloseWindow();
    return 0;
}
