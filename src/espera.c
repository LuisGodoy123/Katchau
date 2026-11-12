/*
 * espera.c
 * ------------------------------------------------------------------
 * A tela de carregamento. Veja espera.h.
 * ------------------------------------------------------------------
 */

#include "raylib.h"
#include "config.h"
#include "espera.h"
#include "hud.h"
#include "visual.h"

#define LARGURA_DA_LOGO     420.0f
#define LARGURA_DA_BARRA    360.0f
#define ALTURA_DA_BARRA     10.0f
#define INTERVALO_MINIMO    0.05      /* segundos: desenhar mais que isso só atrasaria o carregamento (o VSync espera) */

static float faixaDe = 0.0f, faixaAte = 1.0f;
static double ultimoDesenho = -1.0;

/*
 * quadroPretoDaEspera
 * Um quadro preto logo que a janela abre: sem ele, a janela aparece branca até o primeiro desenho.
 * (Dois, porque a janela tem dois buffers.)
 * Recebe: nada. Devolve: nada.
 */
void quadroPretoDaEspera(void) {
    for (int i = 0; i < 2; i++) {
        BeginDrawing();
        ClearBackground(BLACK);
        EndDrawing();
    }
}

/*
 * faixaDaEspera
 * Reserva um pedaço da barra para a etapa que vem a seguir.
 * Recebe: onde a faixa começa e termina na barra (0 a 1).
 * Devolve: nada.
 */
void faixaDaEspera(float de, float ate) {
    faixaDe = de;
    faixaAte = ate;
}

/*
 * passoDaEspera
 * Desenha a tela de carregamento (e, de quebra, deixa a janela ouvir o Windows, que a marcaria como
 * "não respondendo" se ela passasse segundos sem desenhar).
 * Recebe: o texto da etapa e quanto da faixa atual já foi feito (0 a 1).
 * Devolve: nada.
 */
void passoDaEspera(const char *texto, float fracaoDaFaixa) {
    double agora = GetTime();
    if (ultimoDesenho >= 0.0 && agora - ultimoDesenho < INTERVALO_MINIMO && fracaoDaFaixa < 1.0f) {
        return;
    }
    ultimoDesenho = agora;
    float fracao = faixaDe + (faixaAte - faixaDe) * fracaoDaFaixa;
    fracao = (fracao < 0.0f) ? 0.0f : (fracao > 1.0f) ? 1.0f : fracao;

    BeginDrawing();
    ClearBackground(BLACK);
    comecarInterface(1);
    desenharLogo(LARGURA_TELA / 2.0f, 130.0f, LARGURA_DA_LOGO);
    escreverCentralizado(FONTE_FORTE, texto, LARGURA_TELA / 2.0f, 392.0f, 18.0f, COR_TEXTO_FRACO);
    Rectangle barra = { (LARGURA_TELA - LARGURA_DA_BARRA) / 2.0f, 430.0f, LARGURA_DA_BARRA, ALTURA_DA_BARRA };
    DrawRectangleRec(barra, (Color){ 40, 43, 54, 255 });
    DrawRectangle((int) barra.x, (int) barra.y, (int) (barra.width * fracao), (int) barra.height, COR_AMARELO);
    terminarInterface();
    EndDrawing();
}
