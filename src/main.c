/*
 * main.c
 * ------------------------------------------------------------------
 * Ponto de entrada do KATCHAU.
 * Abre a janela e troca entre as telas: menu, garagem, como jogar,
 * corrida e resultado. A variável "tela" diz em qual delas o jogador
 * está; cada tela devolve para qual ir no quadro seguinte.
 *
 * O laço principal segue sempre a mesma ordem, uma vez por quadro:
 *   1) ler o teclado e atualizar a tela atual (lógica);
 *   2) desenhar a tela atual (render.c, hud.c, menu.c).
 * ------------------------------------------------------------------
 */

#include <string.h>
#include "raylib.h"
#include "config.h"
#include "pista.h"
#include "carro.h"
#include "corrida.h"
#include "render.h"
#include "hud.h"
#include "menu.h"
#include "visual.h"

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

/*
 * atualizarTelaDaCorrida
 * Um quadro da corrida: o jogador dirige, a IA dirige os rivais.
 * ESC abandona a corrida; depois da chegada, ENTER vai ao resultado.
 * Recebe: a corrida e o tempo do quadro.
 * Devolve: a próxima tela.
 */
static Tela atualizarTelaDaCorrida(Corrida *corrida, float tempoQuadro) {
    atualizarCorrida(corrida, lerComandosDoTeclado(), tempoQuadro);

    if (IsKeyPressed(KEY_ESCAPE)) {
        return TELA_MENU;
    }
    if (corrida->terminou && (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER))) {
        return TELA_RESULTADO;
    }
    return TELA_CORRIDA;
}

/*
 * abrirJanela
 * Abre o jogo em tela cheia, na resolução do monitor. É uma "tela cheia
 * sem bordas" (borderless): uma janela do tamanho do monitor, sem mudar
 * o modo de vídeo do Windows, então o Alt+Tab funciona sem travar.
 * Com --janela na linha de comando, abre numa janela 800x600 (útil para
 * testar e depurar).
 * Recebe: 1 para abrir em janela, 0 para tela cheia.
 * Devolve: nada.
 */
static void abrirJanela(int emJanela) {
    /* VSync: cada quadro sai junto com a atualização do monitor, em
     * intervalos regulares, sem os "trancos" de quadros irregulares.
     * Precisa vir antes do InitWindow. O SetTargetFPS logo abaixo
     * continua valendo como limite, caso o driver de vídeo ignore o VSync. */
    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(LARGURA_TELA, ALTURA_TELA, TITULO_JANELA);
    if (!emJanela) {
        ToggleBorderlessWindowed();   /* ocupa o monitor inteiro */
        HideCursor();
    }
    SetTargetFPS(QUADROS_POR_SEG);
    SetExitKey(KEY_NULL);   /* ESC volta de tela; para sair, use "SAIR" no menu */
}

int main(int argc, char *argv[]) {
    int emJanela = (argc > 1 && strcmp(argv[1], "--janela") == 0);
    abrirJanela(emJanela);

    /* As texturas são procuradas em recursos/, ao lado do executável.
     * Mudar para a pasta do executável faz o jogo achá-las mesmo quando
     * é aberto de outro lugar (por exemplo, com duplo clique). */
    ChangeDirectory(GetApplicationDirectory());
    carregarTexturas();
    carregarVisual();   /* fontes e logo dos menus */

    Pista *pista = criarPista();
    Corrida corrida;
    EstadoMenu menu;
    iniciarMenu(&menu);
    Tela tela = TELA_MENU;

    /* Laço principal: roda uma vez por quadro até escolher SAIR ou fechar a janela */
    while (tela != TELA_SAIR && !WindowShouldClose()) {
        /* Tempo do último quadro, para a velocidade não depender do FPS */
        float tempoQuadro = GetFrameTime();

        /* ---- 1) Atualizar a tela atual e descobrir a próxima ---- */
        Tela proxima = tela;
        switch (tela) {
            case TELA_MENU:
                proxima = atualizarMenuPrincipal(&menu);
                break;
            case TELA_GARAGEM:
                proxima = atualizarGaragem(&menu);
                break;
            case TELA_COMO_JOGAR:
                proxima = atualizarComoJogar();
                break;
            case TELA_CORRIDA:
                proxima = atualizarTelaDaCorrida(&corrida, tempoQuadro);
                break;
            case TELA_RESULTADO: {
                /* A corrida continua ao fundo até todos os rivais chegarem */
                Comandos nenhumComando = { 0, 0, 0, 0 };
                atualizarCorrida(&corrida, nenhumComando, tempoQuadro);
                proxima = atualizarResultado();
                break;
            }
            default:
                break;
        }

        /* Entrar na corrida (do menu ou do "correr de novo") sempre começa uma nova */
        if (proxima == TELA_CORRIDA && tela != TELA_CORRIDA) {
            iniciarCorrida(&corrida, pista, menu.carroEscolhido);
        }
        tela = proxima;

        /* O mouse só aparece (e só funciona) no menu principal e na garagem */
        int usaMouse = (tela == TELA_MENU || tela == TELA_GARAGEM);
        if (usaMouse && IsCursorHidden()) {
            ShowCursor();
        } else if (!usaMouse && !IsCursorHidden()) {
            HideCursor();
        }

        /* ---- 2) Desenhar a tela atual ----
         * Primeiro o "mundo" (pista e carros), na resolução real da tela.
         * Depois a interface (HUD e menus), na tela virtual 800x600 que
         * comecarInterface() amplia para o tamanho da tela. */
        BeginDrawing();
        ClearBackground(BLACK);
        switch (tela) {
            case TELA_MENU:
                desenharPistaDeFundo(pista);
                comecarInterface(1);
                desenharMenuPrincipal(&menu);
                terminarInterface();
                break;
            case TELA_GARAGEM:
                desenharPistaDeFundo(pista);
                comecarInterface(1);
                desenharGaragem(&menu);
                terminarInterface();
                break;
            case TELA_COMO_JOGAR:
                desenharPistaDeFundo(pista);
                comecarInterface(1);
                desenharComoJogar();
                terminarInterface();
                break;
            case TELA_CORRIDA:
                desenharVisaoDoJogador(&corrida);
                comecarInterface(0);
                desenharHud(&corrida);
                if (corrida.terminou) {
                    desenharFimDeCorrida(&corrida);
                }
                terminarInterface();
                break;
            case TELA_RESULTADO:
                desenharVisaoDoJogador(&corrida);   /* a corrida continua ao fundo */
                comecarInterface(1);
                desenharResultado(&corrida);
                terminarInterface();
                break;
            default:
                break;
        }
        EndDrawing();
    }

    liberarPista(pista);
    descarregarTexturas();
    descarregarVisual();
    CloseWindow();
    return 0;
}
