/*
 * hud.c
 * ------------------------------------------------------------------
 * Desenho do HUD. Veja hud.h.
 * ------------------------------------------------------------------
 */

#include <stdio.h>
#include <math.h>
#include "raylib.h"
#include "config.h"
#include "hud.h"
#include "visual.h"

/* ==================================================================
 * Tela virtual: o HUD e os menus foram desenhados para 800x600.
 * Em vez de recalcular cada posição para cada monitor, eles são
 * desenhados nessas coordenadas e a câmera 2D da raylib amplia tudo
 * para o tamanho real da tela.
 * ================================================================== */

/*
 * escalaDaInterface
 * Quanto ampliar o 800x600 para caber na tela sem cortar nada.
 * Recebe: nada.
 * Devolve: 1 em 800x600, 1,8 em 1920x1080...
 */
static float escalaDaInterface(void) {
    return fminf(GetScreenWidth() / (float) LARGURA_TELA, GetScreenHeight() / (float) ALTURA_TELA);
}

/*
 * larguraVirtual / alturaVirtual
 * Tamanho da tela inteira em coordenadas virtuais. Num monitor 16:9 a
 * largura virtual passa de 800 (dá ~1067), e o HUD usa o espaço todo.
 * Recebe: nada.
 * Devolve: a largura ou a altura virtual.
 */
int larguraVirtual(void) {
    return (int) (GetScreenWidth() / escalaDaInterface());
}
int alturaVirtual(void) {
    return (int) (GetScreenHeight() / escalaDaInterface());
}

/*
 * comecarInterface
 * Daqui até terminarInterface(), tudo é desenhado em coordenadas
 * virtuais e ampliado para a tela real.
 *   - centralizada = 0: a origem fica no canto da tela (o HUD usa a
 *     largura toda, com o mini-mapa e o velocímetro nos cantos de verdade);
 *   - centralizada = 1: um quadro 800x600 fica no meio da tela (menus).
 * Recebe: o modo.
 * Devolve: nada.
 */
void comecarInterface(int centralizada) {
    float zoom = escalaDaInterface();
    Camera2D camera = { 0 };
    camera.zoom = zoom;
    if (centralizada) {
        camera.offset.x = (GetScreenWidth() - LARGURA_TELA * zoom) / 2.0f;
        camera.offset.y = (GetScreenHeight() - ALTURA_TELA * zoom) / 2.0f;
    }
    BeginMode2D(camera);
}

/*
 * posicaoDoMouseNaInterface
 * Onde o mouse está, em coordenadas do quadro 800x600 centralizado dos
 * menus (o mesmo sistema de comecarInterface(1)).
 * Recebe: onde guardar o x e o y.
 * Devolve: nada.
 */
void posicaoDoMouseNaInterface(float *x, float *y) {
    float zoom = escalaDaInterface();
    float deslocX = (GetScreenWidth() - LARGURA_TELA * zoom) / 2.0f;
    float deslocY = (GetScreenHeight() - ALTURA_TELA * zoom) / 2.0f;
    *x = (GetMouseX() - deslocX) / zoom;
    *y = (GetMouseY() - deslocY) / zoom;
}

/*
 * terminarInterface
 * Volta a desenhar em pixels reais da tela.
 * Recebe: nada.
 * Devolve: nada.
 */
void terminarInterface(void) {
    EndMode2D();
}

#define TAMANHO_TITULO_FIM    48
#define TAMANHO_TEXTO_FIM     22
#define MARGEM_HUD            12
#define COR_TEXTO_HUD         COR_TEXTO_CLARO
#define TAMANHO_CONTAGEM      96

/* O HUD é discreto, como nas transmissões: painéis pequenos, letra
 * pequena (Saira Condensed) e nada de faixas grandes por cima da pista. */

/* ---------- Painel de posições (canto superior esquerdo) ---------- */
#define LARGURA_POSICOES      236.0f
#define ALTURA_CABECALHO_HUD  20.0f
#define ALTURA_SUBCABECALHO   16.0f
#define ALTURA_LINHA_HUD      16.0f
#define LARGURA_BANDEIRA      78.0f
#define X_COLUNA_INTERVALO    146.0f   /* a partir da esquerda do painel */
#define X_COLUNA_ULTIMA       190.0f
#define LETRA_CABECALHO       14.0f
#define LETRA_HUD             12.0f
#define LETRA_PEQUENA         10.5f

/* ---------- Painel da volta (canto superior direito) ---------- */
#define LARGURA_PAINEL_VOLTA  236.0f
#define ALTURA_LINHA_VOLTA    20.0f
#define ALTURA_TEMPOS_VOLTA   32.0f
#define ALTURA_SETORES        14.0f

/* ---------- Velocímetro (canto inferior direito) ---------- */
#define RAIO_VELOCIMETRO      62.0f
#define KMH_DO_MOSTRADOR      350.0f   /* o arco vai de 0 a 350 km/h */
#define KMH_DA_FAIXA_VERMELHA 300.0f
#define ANGULO_DO_ZERO        135.0f   /* graus: 0 km/h embaixo à esquerda... */
#define VARREDURA             270.0f   /* ...e o arco gira 270 graus até embaixo à direita */

/* ---------- Cores do HUD ---------- */
#define COR_FUNDO_HUD         Fade(COR_GRAFITE, 0.78f)
#define COR_VERDE_HUD         (Color){  40, 200,  70, 255 }
#define COR_ROXO_HUD          (Color){ 165,  70, 220, 255 }

/* ---------- Mini-mapa (canto inferior esquerdo) ---------- */
#define MAX_PONTOS_MAPA       8192   /* um ponto por segmento; Interlagos tem ~2.600 */
#define TAMANHO_MAPA          150    /* lado do quadrado do mini-mapa, em pixels */
#define MARGEM_MAPA           8      /* folga entre o traçado e a borda do quadrado */
#define TAMANHO_TEXTO_MAPA    12
#define ESPESSURA_TRACADO     3.0f
#define RAIO_CARRO_MAPA       6.5f

/* Desenho da pista visto de cima, já na escala da tela.
 * pontosMapa[i] é onde começa o segmento de índice i. É calculado uma
 * vez só (na primeira vez que o HUD é desenhado com aquela pista). */
static Vector2 pontosMapa[MAX_PONTOS_MAPA];
static int totalPontosMapa = 0;
static Pista *pistaDoMapa = NULL;
static Vector2 cantoDoMapa = { -1.0f, -1.0f };   /* onde o mapa foi calculado (muda se a janela mudar) */

/*
 * formatarTempo
 * Escreve um tempo em segundos no formato minutos:segundos.centésimos.
 * Exemplo: 83.456 vira "1:23.45".
 * Recebe: o tempo, o texto de saída e o tamanho dele.
 * Devolve: nada (o resultado fica em "texto").
 */
void formatarTempo(float segundos, char *texto, int tamanho) {
    int minutos = (int) (segundos / 60);
    float resto = segundos - minutos * 60;
    snprintf(texto, tamanho, "%d:%05.2f", minutos, resto);
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
    cantoDoMapa = (Vector2){ xQuadrado, yQuadrado };
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
 * desenharCarroNoMapa
 * Uma bolinha com a posição do carro escrita dentro.
 * Recebe: onde, a posição (1 a 8), a cor da bolinha e a do número.
 * Devolve: nada.
 */
static void desenharCarroNoMapa(Vector2 ponto, int posicao, Color fundo, Color numero) {
    DrawCircleV(ponto, RAIO_CARRO_MAPA + 1.0f, COR_GRAFITE);
    DrawCircleV(ponto, RAIO_CARRO_MAPA, fundo);
    const char *texto = TextFormat("%d", posicao);
    float largura = larguraDoTexto(FONTE_FORTE, texto, 11.0f);
    escrever(FONTE_FORTE, texto, ponto.x - largura / 2.0f, ponto.y - 7.5f, 11.0f, numero);
}

/*
 * desenharMiniMapa
 * No canto inferior esquerdo, sem painel por trás: só o traçado da pista
 * (branco, com borda escura para aparecer sobre a grama e o asfalto), a
 * linha de chegada e os carros como bolinhas com a posição de cada um
 * (o jogador em vermelho, por cima). Embaixo, o nome do trecho.
 * Recebe: a corrida.
 * Devolve: nada.
 */
static void desenharMiniMapa(Corrida *corrida) {
    float x = MARGEM_HUD;
    float y = alturaVirtual() - MARGEM_HUD - TAMANHO_MAPA - TAMANHO_TEXTO_MAPA - 4;
    if (pistaDoMapa != corrida->pista || cantoDoMapa.x != x || cantoDoMapa.y != y) {
        calcularMapa(corrida->pista, x, y);
    }

    /* Traçado: borda escura por baixo e a linha branca por cima */
    for (int i = 0; i < totalPontosMapa; i++) {
        DrawLineEx(pontosMapa[i], pontosMapa[(i + 1) % totalPontosMapa], ESPESSURA_TRACADO + 3.0f,
                   Fade(COR_GRAFITE, 0.7f));
    }
    for (int i = 0; i < totalPontosMapa; i++) {
        DrawLineEx(pontosMapa[i], pontosMapa[(i + 1) % totalPontosMapa], ESPESSURA_TRACADO, COR_TEXTO_CLARO);
    }

    /* Linha de chegada: um tracinho atravessado no ponto da largada */
    Vector2 largada = pontosMapa[0];
    Vector2 frente = pontosMapa[1 % totalPontosMapa];
    Vector2 direcao = { frente.x - largada.x, frente.y - largada.y };
    float tamanho = sqrtf(direcao.x * direcao.x + direcao.y * direcao.y);
    if (tamanho > 0.0f) {
        Vector2 lado = { -direcao.y / tamanho * 6.0f, direcao.x / tamanho * 6.0f };
        DrawLineEx((Vector2){ largada.x - lado.x, largada.y - lado.y },
                   (Vector2){ largada.x + lado.x, largada.y + lado.y }, 2.5f, COR_VERMELHO);
    }

    /* Do último para o primeiro, para o líder ficar por cima; o jogador por último de todos */
    for (int i = TOTAL_CARROS - 1; i >= 0; i--) {
        Carro *carro = corrida->classificacao[i];
        if (carro != &corrida->jogador) {
            desenharCarroNoMapa(pontoDoCarroNoMapa(carro), i + 1, COR_TEXTO_CLARO, COR_GRAFITE);
        }
    }
    desenharCarroNoMapa(pontoDoCarroNoMapa(&corrida->jogador), posicaoDoCarro(corrida, &corrida->jogador),
                        COR_VERMELHO, COR_TEXTO_CLARO);

    const char *trecho = corrida->jogador.posicao.segmento->nomeTrecho;
    escrever(FONTE_FORTE, trecho, x + 2, y + TAMANHO_MAPA + 2, TAMANHO_TEXTO_MAPA, COR_TEXTO_HUD);
}

/*
 * textoDoTempo
 * Um tempo no formato do formatarTempo, ou "--" se ainda não existe (0).
 * Recebe: o tempo, o texto de saída e o tamanho dele.
 * Devolve: o próprio texto, para usar direto numa chamada.
 */
static const char *textoDoTempo(float segundos, char *texto, int tamanho) {
    if (segundos <= 0.0f) {
        snprintf(texto, tamanho, "--");
    } else {
        formatarTempo(segundos, texto, tamanho);
    }
    return texto;
}

/*
 * ultimaVolta
 * Recebe: o carro.
 * Devolve: o tempo da última volta completada (0 se nenhuma).
 */
static float ultimaVolta(Carro *carro) {
    int voltas = carro->voltas < TOTAL_VOLTAS ? carro->voltas : TOTAL_VOLTAS;
    return voltas >= 1 ? carro->temposDeVolta[voltas - 1] : 0.0f;
}

/*
 * desenharBandeira
 * A etiqueta do estado da prova no painel de posições.
 * Recebe: posição e tamanho da etiqueta e a bandeira.
 * Devolve: nada.
 */
static void desenharBandeira(float x, float y, float largura, float altura, Bandeira bandeira) {
    float centro = x + largura / 2.0f;
    float letra = LETRA_PEQUENA;
    float yTexto = y + (altura - letra) / 2.0f - 2.0f;
    if (bandeira == BANDEIRA_LARGADA) {
        DrawRectangleRec((Rectangle){ x, y, largura, altura }, COR_VERMELHO);
        escreverCentralizado(FONTE_FORTE, "LARGADA", centro, yTexto, letra, COR_TEXTO_CLARO);
    } else if (bandeira == BANDEIRA_VERDE) {
        DrawRectangleRec((Rectangle){ x, y, largura, altura }, COR_VERDE_HUD);
        escreverCentralizado(FONTE_FORTE, "BANDEIRA VERDE", centro, yTexto, letra, COR_GRAFITE);
    } else {
        desenharQuadriculado(x, y, largura, altura, altura / 2.0f);
        float meia = larguraDoTexto(FONTE_FORTE, "CHEGADA", letra) / 2.0f + 5.0f;
        DrawRectangleRec((Rectangle){ centro - meia, y + 3, 2 * meia, altura - 6 }, COR_GRAFITE);
        escreverCentralizado(FONTE_FORTE, "CHEGADA", centro, yTexto, letra, COR_TEXTO_CLARO);
    }
}

/*
 * desenharPainelDePosicoes
 * Tabela pequena no canto superior esquerdo, como nas transmissões:
 * cabeçalho com a volta e a bandeira, o tempo de prova e, para cada
 * carro (na ordem da classificação do Insertion Sort), a posição, o
 * nome, o intervalo para o jogador (+ à frente, - atrás) e a última volta.
 * Recebe: a corrida.
 * Devolve: nada.
 */
static void desenharPainelDePosicoes(Corrida *corrida) {
    Carro *jogador = &corrida->jogador;
    float x = MARGEM_HUD, y = MARGEM_HUD;
    float larguraTitulo = LARGURA_POSICOES - LARGURA_BANDEIRA;
    char texto[32];

    /* Cabeçalho */
    DrawRectangleRec((Rectangle){ x, y, larguraTitulo, ALTURA_CABECALHO_HUD }, COR_GRAFITE);
    escrever(FONTE_FORTE, "CORRIDA", x + 7, y + 1, LETRA_CABECALHO, COR_TEXTO_CLARO);
    escreverADireita(FONTE_FORTE, TextFormat("VOLTA %d/%d", voltaAtual(jogador), TOTAL_VOLTAS),
                     x + larguraTitulo - 7, y + 1, LETRA_CABECALHO, COR_TEXTO_CLARO);
    desenharBandeira(x + larguraTitulo, y, LARGURA_BANDEIRA, ALTURA_CABECALHO_HUD, bandeiraDaCorrida(corrida));
    y += ALTURA_CABECALHO_HUD;

    /* Tempo de prova e títulos das colunas. Depois da chegada, o relógio
     * fica parado no tempo do jogador. */
    float tempo = corrida->terminou ? jogador->tempoDeChegada : corrida->tempoDeCorrida;
    DrawRectangleRec((Rectangle){ x, y, LARGURA_POSICOES, ALTURA_SUBCABECALHO }, COR_GRAFITE);
    formatarTempo(tempo, texto, sizeof(texto));
    escrever(FONTE_FORTE, texto, x + 7, y + 1, LETRA_HUD, COR_TEXTO_CLARO);
    escrever(FONTE_FORTE, "Interv.", x + X_COLUNA_INTERVALO, y + 1, LETRA_HUD, COR_TEXTO_CLARO);
    escrever(FONTE_FORTE, "Última", x + X_COLUNA_ULTIMA, y + 1, LETRA_HUD, COR_TEXTO_CLARO);
    y += ALTURA_SUBCABECALHO;

    for (int i = 0; i < TOTAL_CARROS; i++) {
        Carro *carro = corrida->classificacao[i];
        int ehJogador = (carro == jogador);
        Color corTexto = ehJogador ? COR_AMARELO : COR_TEXTO_CLARO;

        DrawRectangleRec((Rectangle){ x, y, LARGURA_POSICOES, ALTURA_LINHA_HUD }, COR_FUNDO_HUD);
        DrawRectangleRec((Rectangle){ x + 3, y + 2, 13, ALTURA_LINHA_HUD - 4 },
                         ehJogador ? COR_AMARELO : COR_TEXTO_CLARO);
        escreverCentralizado(FONTE_FORTE, TextFormat("%d", i + 1), x + 9.5f, y + 1, LETRA_PEQUENA, COR_GRAFITE);
        escrever(FONTE_TEXTO, carro->nome, x + 22, y + 1, LETRA_HUD, corTexto);

        const char *intervalo = ehJogador ? "--" : TextFormat("%+.2f", intervaloEmSegundos(carro, jogador));
        escrever(FONTE_TEXTO, intervalo, x + X_COLUNA_INTERVALO, y + 1, LETRA_HUD, corTexto);
        escrever(FONTE_TEXTO, textoDoTempo(ultimaVolta(carro), texto, sizeof(texto)),
                 x + X_COLUNA_ULTIMA, y + 1, LETRA_HUD, corTexto);
        y += ALTURA_LINHA_HUD;
    }
}

/*
 * corDoSetor
 * Recebe: a situação de um setor.
 * Devolve: roxo (melhor da corrida), verde (melhor pessoal), amarelo
 *          (mais lento) ou apagado (ainda não feito), como no F1.
 */
static Color corDoSetor(SituacaoDoSetor situacao) {
    switch (situacao) {
        case SETOR_MELHOR_DA_CORRIDA: return COR_ROXO_HUD;
        case SETOR_MELHOR_PESSOAL:    return COR_VERDE_HUD;
        case SETOR_MAIS_LENTO:        return COR_AMARELO;
        default:                      return COR_FUNDO_HUD;
    }
}

/*
 * desenharPainelDaVolta
 * Painel pequeno no canto superior direito com a volta do jogador:
 *   - a diferença para a melhor volta dele no último setor fechado
 *     (verde se está ganhando tempo, vermelho se está perdendo);
 *   - tempo da volta atual, da última, da melhor e a mais rápida da corrida;
 *   - os três setores, pintados conforme foram.
 * Recebe: a corrida.
 * Devolve: nada.
 */
static void desenharPainelDaVolta(Corrida *corrida) {
    Carro *jogador = &corrida->jogador;
    float largura = LARGURA_PAINEL_VOLTA;
    float metade = largura / 2.0f;
    float x = larguraVirtual() - MARGEM_HUD - largura;
    float y = MARGEM_HUD;
    char texto[32];

    /* Linha 1: volta e diferença */
    DrawRectangleRec((Rectangle){ x, y, metade, ALTURA_LINHA_VOLTA }, COR_GRAFITE);
    escrever(FONTE_FORTE, TextFormat("VOLTA %d", voltaAtual(jogador)), x + 7, y + 1, LETRA_CABECALHO,
             COR_TEXTO_CLARO);

    Color corDiferenca = COR_FUNDO_HUD;
    Color corTextoDiferenca = COR_TEXTO_FRACO;
    const char *diferenca = "--";
    if (jogador->temDiferenca) {
        diferenca = TextFormat("%+.3f", jogador->diferenca);
        corDiferenca = (jogador->diferenca <= 0.0f) ? COR_VERDE_HUD : COR_VERMELHO;
        corTextoDiferenca = (jogador->diferenca <= 0.0f) ? COR_GRAFITE : COR_TEXTO_CLARO;
    }
    DrawRectangleRec((Rectangle){ x + metade, y, metade, ALTURA_LINHA_VOLTA }, corDiferenca);
    escreverCentralizado(FONTE_FORTE, diferenca, x + metade * 1.5f, y + 1, LETRA_CABECALHO, corTextoDiferenca);
    y += ALTURA_LINHA_VOLTA;

    /* Linhas 2 e 3: tempos */
    DrawRectangleRec((Rectangle){ x, y, largura, ALTURA_TEMPOS_VOLTA }, COR_FUNDO_HUD);
    DrawRectangleRec((Rectangle){ x + metade, y + 4, 1, ALTURA_TEMPOS_VOLTA - 8 }, Fade(COR_CROMO, 0.3f));

    float atual = 0.0f;
    if (corrida->contagemRegressiva <= 0.0f && !corrida->terminou) {
        atual = corrida->tempoDeCorrida - jogador->inicioDaVolta;
    }
    const char *rotulos[4] = { "Atual:", "Última:", "Melhor:", "Mais rápida:" };
    float valores[4] = { atual, ultimaVolta(jogador), jogador->melhorVolta, corrida->voltaMaisRapida };
    for (int i = 0; i < 4; i++) {
        float coluna = x + (i < 2 ? 0.0f : metade);
        float linha = y + 1 + (i % 2) * 15;
        escrever(FONTE_TEXTO, rotulos[i], coluna + 7, linha, LETRA_HUD, COR_TEXTO_CLARO);
        Color cor = (i == 3 && valores[3] > 0.0f && valores[3] == jogador->melhorVolta) ? COR_ROXO_HUD : COR_TEXTO_CLARO;
        escreverADireita(FONTE_TEXTO, textoDoTempo(valores[i], texto, sizeof(texto)), coluna + metade - 7,
                         linha, LETRA_HUD, cor);
    }
    y += ALTURA_TEMPOS_VOLTA;

    /* Setores, colados um no outro */
    float larguraSetor = largura / TOTAL_SETORES;
    for (int s = 0; s < TOTAL_SETORES; s++) {
        SituacaoDoSetor situacao = situacaoDoSetor(corrida, jogador, s);
        float sx = x + s * larguraSetor;
        DrawRectangleRec((Rectangle){ sx, y, larguraSetor, ALTURA_SETORES }, corDoSetor(situacao));
        if (s > 0) {
            DrawRectangleRec((Rectangle){ sx, y, 1, ALTURA_SETORES }, COR_GRAFITE);
        }
        Color corTexto = (situacao == SETOR_PENDENTE || situacao == SETOR_MELHOR_DA_CORRIDA)
                         ? COR_TEXTO_CLARO : COR_GRAFITE;
        escreverCentralizado(FONTE_FORTE, TextFormat("S%d", s + 1), sx + larguraSetor / 2.0f, y,
                             LETRA_PEQUENA, corTexto);
    }
}

/*
 * pontoNoMostrador
 * Recebe: o centro do velocímetro, o raio e a fração do mostrador
 *         (0 = 0 km/h, 1 = KMH_DO_MOSTRADOR).
 * Devolve: o ponto na tela (y cresce para baixo, então o ângulo gira
 *          no sentido horário).
 */
static Vector2 pontoNoMostrador(Vector2 centro, float raio, float fracao) {
    float angulo = (ANGULO_DO_ZERO + VARREDURA * fracao) * DEG2RAD;
    return (Vector2){ centro.x + cosf(angulo) * raio, centro.y + sinf(angulo) * raio };
}

/*
 * desenharArco
 * Um pedaço do mostrador, feito de tracinhos curtos.
 * Recebe: centro, raio, de que fração até que fração, grossura e cor.
 * Devolve: nada.
 */
static void desenharArco(Vector2 centro, float raio, float de, float ate, float grossura, Color cor) {
    int passos = (int) ((ate - de) * 80.0f) + 1;
    for (int i = 0; i < passos; i++) {
        float f0 = de + (ate - de) * i / passos;
        float f1 = de + (ate - de) * (i + 1) / passos;
        DrawLineEx(pontoNoMostrador(centro, raio, f0), pontoNoMostrador(centro, raio, f1), grossura, cor);
    }
}

/*
 * desenharVelocimetro
 * No canto inferior direito, só o essencial: um arco que enche com a
 * velocidade (vermelho a partir de 300 km/h), a velocidade em números
 * grandes no meio e a posição do jogador embaixo.
 * Recebe: a corrida.
 * Devolve: nada.
 */
static void desenharVelocimetro(Corrida *corrida) {
    Carro *jogador = &corrida->jogador;
    float raio = RAIO_VELOCIMETRO;
    Vector2 centro = { larguraVirtual() - MARGEM_HUD - raio - 4, alturaVirtual() - MARGEM_HUD - raio + 6 };
    int kmh = velocidadeEmKmh(jogador);
    float fracao = fminf(kmh / KMH_DO_MOSTRADOR, 1.0f);
    float faixaVermelha = KMH_DA_FAIXA_VERMELHA / KMH_DO_MOSTRADOR;

    desenharBrilho(centro.x, centro.y, raio + 6, Fade(BLACK, 0.55f));
    desenharArco(centro, raio, 0.0f, 1.0f, 5.0f, Fade(COR_TEXTO_CLARO, 0.25f));
    if (fracao > 0.0f) {
        desenharArco(centro, raio, 0.0f, fminf(fracao, faixaVermelha), 5.0f, COR_TEXTO_CLARO);
        if (fracao > faixaVermelha) {
            desenharArco(centro, raio, faixaVermelha, fracao, 5.0f, COR_VERMELHO);
        }
    }

    escreverCentralizado(FONTE_FORTE, TextFormat("%d", kmh), centro.x, centro.y - 30, 38.0f, COR_TEXTO_CLARO);
    escreverCentralizado(FONTE_TEXTO, "km/h", centro.x, centro.y + 10, 12.0f, COR_TEXTO_FRACO);

    int posicao = corrida->terminou ? corrida->posicaoFinal : posicaoDoCarro(corrida, jogador);
    escreverCentralizado(FONTE_FORTE, TextFormat("P%d", posicao), centro.x, centro.y + 26, 20.0f, COR_AMARELO);
}

/*
 * desenharContagemRegressiva
 * Na largada, mostra 3, 2, 1 no meio da tela e, no primeiro segundo
 * de corrida, o grito de KATCHAU!
 * Recebe: a corrida.
 * Devolve: nada.
 */
static void desenharContagemRegressiva(Corrida *corrida) {
    const char *texto = NULL;
    if (corrida->contagemRegressiva > 0.0f) {
        /* Nos primeiros ESPERA_ANTES_DA_CONTAGEM segundos não aparece nada (só o motor).
         * Depois cada número ocupa SEGUNDOS_POR_NUMERO: com 1 s por número, 2,9 s
         * restantes mostra "3"; 1,9 s mostra "2"; 0,9 s mostra "1" */
        if (corrida->contagemRegressiva < NUMEROS_DA_CONTAGEM * SEGUNDOS_POR_NUMERO) {
            texto = TextFormat("%d", (int) (corrida->contagemRegressiva / SEGUNDOS_POR_NUMERO) + 1);
        }
    } else if (corrida->tempoDeCorrida < DURACAO_GRITO_LARGADA) {
        texto = "KATCHAU!";
    }
    if (texto == NULL) {
        return;
    }

    float y = alturaVirtual() / 2.0f - TAMANHO_CONTAGEM;
    escreverCentralizado(FONTE_TITULO, texto, larguraVirtual() / 2.0f, y, TAMANHO_CONTAGEM, COR_AMARELO);
}

/*
 * desenharHud
 * O HUD da corrida, nos quatro cantos da tela: posições (em cima à
 * esquerda), a volta (em cima à direita), o mapa (embaixo à esquerda)
 * e o velocímetro (embaixo à direita).
 * Recebe: a corrida.
 * Devolve: nada.
 */
void desenharHud(Corrida *corrida) {
    desenharPainelDePosicoes(corrida);
    desenharPainelDaVolta(corrida);
    desenharMiniMapa(corrida);
    desenharVelocimetro(corrida);

    if (estaForaDaPista(&corrida->jogador)) {
        escreverCentralizado(FONTE_FORTE, "FORA DA PISTA", larguraVirtual() / 2.0f, alturaVirtual() * 0.30f,
                             20.0f, COR_AMARELO);
    }
    desenharContagemRegressiva(corrida);
}

/*
 * desenharFimDeCorrida
 * Painel de chegada: posição, tempo total, tempo de cada volta e a
 * melhor volta. A tabela com todos os carros fica na tela de
 * resultado (menu.c).
 * Recebe: a corrida (já terminada).
 * Devolve: nada.
 */
void desenharFimDeCorrida(Corrida *corrida) {
    Carro *jogador = &corrida->jogador;
    char tempo[32];

    float largura = 440.0f;
    float altura = 316.0f;
    float x = (larguraVirtual() - largura) / 2.0f;
    float y = (alturaVirtual() - altura) / 2.0f - 30.0f;
    float centro = x + largura / 2.0f;

    DrawRectangleRec((Rectangle){ x, y, largura, altura }, COR_PAINEL);
    desenharQuadriculado(x, y, largura, 12.0f, 6.0f);

    /* Faixa do título: só o 1º lugar ganha o grito de KATCHAU! */
    desenharParalelogramo(x - 14, y + 24, largura + 14, 62, 14, COR_VERMELHO);
    desenharParalelogramo(x - 7, y + 24, largura + 14, 31, 7, Fade(WHITE, 0.12f));
    const char *titulo = (corrida->posicaoFinal == 1) ? "KATCHAU!" : TextFormat("%dº LUGAR", corrida->posicaoFinal);
    escreverCentralizado(FONTE_TITULO, titulo, centro, y + 26, TAMANHO_TITULO_FIM, COR_TEXTO_CLARO);

    formatarTempo(jogador->tempoDeChegada, tempo, sizeof(tempo));
    escrever(FONTE_FORTE, "TEMPO TOTAL", x + 40, y + 106, 16.0f, COR_TEXTO_FRACO);
    escreverADireita(FONTE_TITULO, tempo, x + largura - 40, y + 98, 30.0f, COR_AMARELO);

    /* Procura a melhor volta antes de escrever cada uma */
    int melhorVolta = 0;
    for (int i = 0; i < TOTAL_VOLTAS; i++) {
        if (jogador->temposDeVolta[i] < jogador->temposDeVolta[melhorVolta]) {
            melhorVolta = i;
        }
    }
    for (int i = 0; i < TOTAL_VOLTAS; i++) {
        float linha = y + 150 + i * 38;
        int ehMelhor = (i == melhorVolta);
        DrawRectangleRec((Rectangle){ x + 30, linha - 4, largura - 60, 32 }, Fade(COR_GRAFITE, ehMelhor ? 0.95f : 0.6f));
        escrever(FONTE_FORTE, TextFormat("VOLTA %d", i + 1), x + 42, linha, TAMANHO_TEXTO_FIM, COR_CROMO);
        if (ehMelhor) {
            desenharParalelogramo(x + 150, linha + 2, 70, 20, 5, COR_AMARELO);
            escreverCentralizado(FONTE_FORTE, "MELHOR", x + 188, linha + 2, 15.0f, COR_GRAFITE);
        }
        formatarTempo(jogador->temposDeVolta[i], tempo, sizeof(tempo));
        escreverADireita(FONTE_FORTE, tempo, x + largura - 42, linha, TAMANHO_TEXTO_FIM,
                         ehMelhor ? COR_AMARELO : COR_TEXTO_CLARO);
    }

    float larguraDica = larguraDaTecla("ENTER") + 8 + larguraDoTexto(FONTE_FORTE, "VER O RESULTADO", 15.0f);
    float xDica = centro - larguraDica / 2.0f;
    xDica += desenharTecla(xDica, y + altura - 40, "ENTER") + 8;
    escrever(FONTE_FORTE, "VER O RESULTADO", xDica, y + altura - 39, 15.0f, COR_TEXTO_FRACO);
}
