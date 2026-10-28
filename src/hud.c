/*
 * hud.c
 * ------------------------------------------------------------------
 * Desenho do HUD. Veja hud.h.
 * ------------------------------------------------------------------
 */

#include <stdio.h>
#include <math.h>
#include "raylib.h"
#include "hud.h"

#define TAMANHO_TEXTO_HUD     24
#define TAMANHO_TITULO_FIM    48
#define TAMANHO_TEXTO_FIM     22
#define MARGEM_HUD            14
#define COR_FUNDO_HUD         Fade(BLACK, 0.55f)
#define COR_TEXTO_HUD         RAYWHITE
#define COR_DESTAQUE          YELLOW

/* ---------- Mini-mapa ---------- */
#define MAX_PONTOS_MAPA       8192   /* um ponto por segmento; Interlagos tem ~2.600 */
#define TAMANHO_MAPA          150    /* lado do quadrado do mini-mapa, em pixels */
#define MARGEM_MAPA           12     /* folga entre o traçado e a borda do quadrado */
#define TOPO_MAPA             64     /* logo abaixo da faixa do HUD */
#define TAMANHO_TEXTO_MAPA    14
#define ESPESSURA_TRACADO     4.0f
#define RAIO_JOGADOR          6.0f
#define RAIO_RIVAL            3.5f
#define COR_FUNDO_MAPA        Fade(BLACK, 0.45f)
#define COR_TRACADO           Fade(RAYWHITE, 0.85f)
#define COR_JOGADOR_MAPA      RED
#define COR_RIVAL_MAPA        (Color){ 255, 220, 60, 255 }

/* Desenho da pista visto de cima, já na escala da tela.
 * pontosMapa[i] é onde começa o segmento de índice i. É calculado uma
 * vez só (na primeira vez que o HUD é desenhado com aquela pista). */
static Vector2 pontosMapa[MAX_PONTOS_MAPA];
static int totalPontosMapa = 0;
static Pista *pistaDoMapa = NULL;

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
 * calcularMapa
 * Desenha a pista "vista de cima" a partir da lista. Começa na largada
 * apontando para cima e, a cada segmento, anda um passo para a frente
 * e vira a curvatura dele (como uma tartaruga desenhando). Depois
 * ajusta o desenho para caber no quadrado do mini-mapa.
 * Recebe: a pista e o canto superior esquerdo do quadrado na tela.
 * Devolve: nada (o resultado fica em pontosMapa).
 */
static void calcularMapa(Pista *pista, float xQuadrado, float yQuadrado) {
    int total = contarSegmentos(pista);
    if (total > MAX_PONTOS_MAPA) {
        total = MAX_PONTOS_MAPA;
    }

    /* 1) Caminha pela lista guardando onde começa cada segmento */
    float x = 0.0f, y = 0.0f, direcao = 0.0f;   /* direção em graus; 0 = para cima */
    Segmento *atual = pista->largada;
    for (int i = 0; i < total; i++) {
        pontosMapa[i] = (Vector2){ x, y };
        x += sinf(direcao * DEG2RAD);
        y -= cosf(direcao * DEG2RAD);   /* na tela, y cresce para baixo */
        direcao += atual->curvatura;
        atual = atual->proximo;
    }

    /* 2) As medidas são aproximadas, então o fim pode não cair exatamente
     * no começo. Espalha o erro aos poucos para o desenho fechar. */
    for (int i = 0; i < total; i++) {
        float fracao = (float) i / total;
        pontosMapa[i].x -= x * fracao;
        pontosMapa[i].y -= y * fracao;
    }

    /* 3) Acha o retângulo que envolve o desenho e escala para o quadrado */
    float minX = pontosMapa[0].x, maxX = minX, minY = pontosMapa[0].y, maxY = minY;
    for (int i = 1; i < total; i++) {
        minX = fminf(minX, pontosMapa[i].x);
        maxX = fmaxf(maxX, pontosMapa[i].x);
        minY = fminf(minY, pontosMapa[i].y);
        maxY = fmaxf(maxY, pontosMapa[i].y);
    }
    float util = TAMANHO_MAPA - 2 * MARGEM_MAPA;
    float escala = util / fmaxf(maxX - minX, maxY - minY);
    /* Centraliza no lado que sobrar */
    float sobraX = (util - (maxX - minX) * escala) / 2.0f;
    float sobraY = (util - (maxY - minY) * escala) / 2.0f;
    for (int i = 0; i < total; i++) {
        pontosMapa[i].x = xQuadrado + MARGEM_MAPA + sobraX + (pontosMapa[i].x - minX) * escala;
        pontosMapa[i].y = yQuadrado + MARGEM_MAPA + sobraY + (pontosMapa[i].y - minY) * escala;
    }

    totalPontosMapa = total;
    pistaDoMapa = pista;
}

/*
 * pontoDoCarroNoMapa
 * Onde um carro aparece no mini-mapa: entre o começo do segmento dele
 * e o começo do próximo, conforme o quanto já andou no segmento.
 * Recebe: o carro.
 * Devolve: o ponto na tela.
 */
static Vector2 pontoDoCarroNoMapa(Carro *carro) {
    int i = carro->posicao.segmento->indice % totalPontosMapa;
    Vector2 inicio = pontosMapa[i];
    Vector2 fim = pontosMapa[(i + 1) % totalPontosMapa];
    float fracao = carro->posicao.distanciaNoSegmento / COMPRIMENTO_SEGMENTO;
    return (Vector2){ inicio.x + (fim.x - inicio.x) * fracao,
                      inicio.y + (fim.y - inicio.y) * fracao };
}

/*
 * desenharMiniMapa
 * Quadrado no canto superior direito com o traçado da pista, a linha
 * de chegada, os rivais (pontos amarelos) e o jogador (ponto vermelho
 * maior). Embaixo, o nome do trecho onde o jogador está.
 * Recebe: a corrida.
 * Devolve: nada.
 */
static void desenharMiniMapa(Corrida *corrida) {
    float x = GetScreenWidth() - TAMANHO_MAPA - MARGEM_HUD;
    float y = TOPO_MAPA;
    if (pistaDoMapa != corrida->pista) {
        calcularMapa(corrida->pista, x, y);
    }

    DrawRectangleRounded((Rectangle){ x, y, TAMANHO_MAPA, TAMANHO_MAPA + TAMANHO_TEXTO_MAPA + 8 },
                         0.1f, 6, COR_FUNDO_MAPA);

    /* Traçado: uma linha entre cada par de pontos vizinhos (fechando a volta) */
    for (int i = 0; i < totalPontosMapa; i++) {
        DrawLineEx(pontosMapa[i], pontosMapa[(i + 1) % totalPontosMapa], ESPESSURA_TRACADO, COR_TRACADO);
    }

    /* Linha de chegada: um tracinho atravessado no ponto da largada */
    Vector2 largada = pontosMapa[0];
    Vector2 frente = pontosMapa[1 % totalPontosMapa];
    Vector2 direcao = { frente.x - largada.x, frente.y - largada.y };
    float tamanho = sqrtf(direcao.x * direcao.x + direcao.y * direcao.y);
    if (tamanho > 0.0f) {
        Vector2 lado = { -direcao.y / tamanho * 7.0f, direcao.x / tamanho * 7.0f };
        DrawLineEx((Vector2){ largada.x - lado.x, largada.y - lado.y },
                   (Vector2){ largada.x + lado.x, largada.y + lado.y }, 3.0f, BLACK);
    }

    for (int r = 0; r < TOTAL_RIVAIS; r++) {
        DrawCircleV(pontoDoCarroNoMapa(&corrida->rivais[r]), RAIO_RIVAL, COR_RIVAL_MAPA);
    }
    /* O jogador por último, para ficar por cima dos rivais */
    Vector2 jogador = pontoDoCarroNoMapa(&corrida->jogador);
    DrawCircleV(jogador, RAIO_JOGADOR + 2.0f, RAYWHITE);
    DrawCircleV(jogador, RAIO_JOGADOR, COR_JOGADOR_MAPA);

    /* Nome do trecho (ex.: "S do Senna") centralizado embaixo do mapa */
    const char *trecho = corrida->jogador.posicao.segmento->nomeTrecho;
    int larguraTexto = MeasureText(trecho, TAMANHO_TEXTO_MAPA);
    DrawText(trecho, (int) (x + (TAMANHO_MAPA - larguraTexto) / 2), (int) (y + TAMANHO_MAPA),
             TAMANHO_TEXTO_MAPA, COR_TEXTO_HUD);
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

    desenharMiniMapa(corrida);
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
