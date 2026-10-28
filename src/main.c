#include "raylib.h"
#include "config.h"

/*
 * Ponto de entrada do KATCHAU.
 * Abre a janela, roda o laço principal e fecha com ESC.
 * Nas próximas fases, troca entre as telas (menu, corrida, resultado).
 */
int main(void)
{
    InitWindow(LARGURA_TELA, ALTURA_TELA, TITULO_JANELA);
    SetTargetFPS(QUADROS_POR_SEG);

    const int tamanhoFonte = 60;
    int larguraTexto = MeasureText(TITULO_JANELA, tamanhoFonte);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);
        DrawText(TITULO_JANELA,
                 (LARGURA_TELA - larguraTexto) / 2,
                 (ALTURA_TELA - tamanhoFonte) / 2,
                 tamanhoFonte, YELLOW);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
