/*
 * hud.c
 * ------------------------------------------------------------------
 * Desenho do HUD. Veja hud.h.
 * ------------------------------------------------------------------
 */

#include <stdio.h>
#include "raylib.h"
#include "hud.h"

#define TAMANHO_TEXTO_HUD     24
#define TAMANHO_TITULO_FIM    48
#define TAMANHO_TEXTO_FIM     22
#define MARGEM_HUD            14
#define COR_FUNDO_HUD         Fade(BLACK, 0.55f)
#define COR_TEXTO_HUD         RAYWHITE
#define COR_DESTAQUE          YELLOW

/*
 * formatarTempo
 * Escreve um tempo em segundos no formato minutos:segundos.centésimos.
 * Exemplo: 83.456 vira "1:23.45".
 * Recebe: o tempo, o texto de saída e o tamanho dele.
 * Devolve: nada (o resultado fica em "texto").
 */
static void formatarTempo(float segundos, char *texto, int tamanho) {
    int minutos = (int) (segundos / 60);
    float resto = segundos - minutos * 60;
    snprintf(texto, tamanho, "%d:%05.2f", minutos, resto);
}

/*
 * desenharTextoCentralizado
 * Escreve um texto centralizado na horizontal.
 * Recebe: o texto, a altura (y), o tamanho da fonte e a cor.
 * Devolve: nada.
 */
static void desenharTextoCentralizado(const char *texto, int y, int tamanho, Color cor) {
    int largura = MeasureText(texto, tamanho);
    DrawText(texto, (GetScreenWidth() - largura) / 2, y, tamanho, cor);
}

/*
 * desenharHud
 * Mostra no topo da tela a posição, a volta, o tempo e a velocidade.
 * Recebe: a corrida.
 * Devolve: nada.
 */
void desenharHud(Corrida *corrida) {
    Carro *jogador = &corrida->jogador;
    char textoTempo[32];
    char textoVolta[32];
    char textoVelocidade[32];

    /* Depois da chegada, o relógio do HUD fica parado no tempo do jogador */
    float tempo = corrida->terminou ? jogador->tempoDeChegada : corrida->tempoDeCorrida;
    int posicao = corrida->terminou ? corrida->posicaoFinal : posicaoDoCarro(corrida, jogador);

    formatarTempo(tempo, textoTempo, sizeof(textoTempo));
    snprintf(textoVolta, sizeof(textoVolta), "POS %d/%d   VOLTA %d/%d",
             posicao, TOTAL_CARROS, voltaAtual(jogador), TOTAL_VOLTAS);
    snprintf(textoVelocidade, sizeof(textoVelocidade), "%3d km/h", velocidadeEmKmh(jogador));

    int larguraTela = GetScreenWidth();
    DrawRectangle(0, 0, larguraTela, TAMANHO_TEXTO_HUD + 2 * MARGEM_HUD, COR_FUNDO_HUD);

    DrawText(textoVolta, MARGEM_HUD, MARGEM_HUD, TAMANHO_TEXTO_HUD, COR_DESTAQUE);
    desenharTextoCentralizado(textoTempo, MARGEM_HUD, TAMANHO_TEXTO_HUD, COR_TEXTO_HUD);

    int larguraVelocidade = MeasureText(textoVelocidade, TAMANHO_TEXTO_HUD);
    DrawText(textoVelocidade, larguraTela - larguraVelocidade - MARGEM_HUD, MARGEM_HUD,
             TAMANHO_TEXTO_HUD, COR_TEXTO_HUD);

    if (estaForaDaPista(jogador)) {
        desenharTextoCentralizado("FORA DA PISTA!", 70, TAMANHO_TEXTO_HUD, COR_DESTAQUE);
    }
}

/*
 * desenharFimDeCorrida
 * Painel de chegada: posição, tempo total, tempo de cada volta e a
 * melhor volta. A tabela com todos os carros fica para a Fase 6.
 * Recebe: a corrida (já terminada).
 * Devolve: nada.
 */
void desenharFimDeCorrida(Corrida *corrida) {
    Carro *jogador = &corrida->jogador;
    char texto[64];
    char tempo[32];

    int largura = 420;
    int altura = 330;
    int x = (GetScreenWidth() - largura) / 2;
    int y = (GetScreenHeight() - altura) / 2 - 40;
    DrawRectangle(x, y, largura, altura, Fade(BLACK, 0.75f));

    /* Só o 1º lugar ganha o grito de KATCHAU! */
    if (corrida->posicaoFinal == 1) {
        desenharTextoCentralizado("KATCHAU!", y + 20, TAMANHO_TITULO_FIM, COR_DESTAQUE);
    } else {
        snprintf(texto, sizeof(texto), "%do LUGAR", corrida->posicaoFinal);
        desenharTextoCentralizado(texto, y + 20, TAMANHO_TITULO_FIM, COR_DESTAQUE);
    }

    formatarTempo(jogador->tempoDeChegada, tempo, sizeof(tempo));
    snprintf(texto, sizeof(texto), "Tempo total: %s", tempo);
    desenharTextoCentralizado(texto, y + 90, TAMANHO_TEXTO_FIM, COR_TEXTO_HUD);

    /* Procura a melhor volta enquanto escreve cada uma */
    int melhorVolta = 0;
    for (int i = 0; i < TOTAL_VOLTAS; i++) {
        if (jogador->temposDeVolta[i] < jogador->temposDeVolta[melhorVolta]) {
            melhorVolta = i;
        }
    }
    for (int i = 0; i < TOTAL_VOLTAS; i++) {
        formatarTempo(jogador->temposDeVolta[i], tempo, sizeof(tempo));
        snprintf(texto, sizeof(texto), "Volta %d: %s%s", i + 1, tempo,
                 i == melhorVolta ? "  (melhor)" : "");
        Color cor = (i == melhorVolta) ? COR_DESTAQUE : COR_TEXTO_HUD;
        desenharTextoCentralizado(texto, y + 135 + i * 32, TAMANHO_TEXTO_FIM, cor);
    }

    desenharTextoCentralizado("ENTER para correr de novo", y + altura - 40, 20, LIGHTGRAY);
}
