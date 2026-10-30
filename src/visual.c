/*
 * visual.c
 * ------------------------------------------------------------------
 * Fontes, cores e formas da interface. Veja visual.h.
 * ------------------------------------------------------------------
 */

#include <string.h>
#include "visual.h"

#define PASTA_FONTES        "recursos/fontes/"
#define ARQUIVO_LOGO        "recursos/texturas/logo/katchau.png"

/* As fontes são geradas grandes e reduzidas com mipmaps: os menus são
 * ampliados para a tela real (até ~2x num monitor Full HD) e assim as
 * letras continuam lisas. */
#define TAMANHO_GERADO_TITULO  96
#define TAMANHO_GERADO_TEXTO   64
#define ESPACO_ENTRE_LETRAS    0.02f   /* fração do tamanho da letra */
#define DESLOCAMENTO_SOMBRA    0.06f   /* a sombra fica a 6% do tamanho, para baixo e para a direita */
#define BLOCOS_DA_BARRA        12
#define FOLGA_ENTRE_BLOCOS     2.0f
#define ALTURA_TECLA           22.0f
#define LETRA_TECLA            15.0f

/* Todos os caracteres que o jogo escreve: o ASCII visível e as letras
 * acentuadas do português (a fonte padrão da raylib não tem acento). */
static const char *CARACTERES =
    " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`"
    "abcdefghijklmnopqrstuvwxyz{|}~"
    "áàâãéêíóôõúüçÁÀÂÃÉÊÍÓÔÕÚÜÇºª";

static const char *ARQUIVOS_FONTE[TOTAL_FONTES] = {
    [FONTE_TITULO] = "racing_sans_one.ttf",
    [FONTE_TEXTO]  = "saira_condensed_semibold.ttf",
    [FONTE_FORTE]  = "saira_condensed_bold.ttf",
};

static Font fontes[TOTAL_FONTES];
static int fonteCarregada[TOTAL_FONTES];
static Texture2D texturaLogo;

/*
 * carregarVisual
 * Lê as fontes e a logo. Chamar depois de abrir a janela. Se algum
 * arquivo faltar, aquela fonte vira a padrão da raylib e a logo vira
 * o nome do jogo escrito: o jogo nunca quebra por falta de arquivo.
 * Recebe: nada.
 * Devolve: nada.
 */
void carregarVisual(void) {
    int quantos = 0;
    int *codigos = LoadCodepoints(CARACTERES, &quantos);

    for (int i = 0; i < TOTAL_FONTES; i++) {
        int tamanho = (i == FONTE_TITULO) ? TAMANHO_GERADO_TITULO : TAMANHO_GERADO_TEXTO;
        const char *caminho = TextFormat("%s%s", PASTA_FONTES, ARQUIVOS_FONTE[i]);
        fonteCarregada[i] = FileExists(caminho);
        if (fonteCarregada[i]) {
            fontes[i] = LoadFontEx(caminho, tamanho, codigos, quantos);
            GenTextureMipmaps(&fontes[i].texture);
            SetTextureFilter(fontes[i].texture, TEXTURE_FILTER_TRILINEAR);
        } else {
            fontes[i] = GetFontDefault();
        }
    }
    UnloadCodepoints(codigos);

    texturaLogo = LoadTexture(ARQUIVO_LOGO);
    if (texturaLogo.id != 0) {
        GenTextureMipmaps(&texturaLogo);
        SetTextureFilter(texturaLogo, TEXTURE_FILTER_TRILINEAR);
    }
}

/*
 * descarregarVisual
 * Devolve a memória das fontes e da logo. Chamar antes de CloseWindow.
 * (A fonte padrão da raylib não é descarregada: ela é da própria raylib.)
 * Recebe: nada.
 * Devolve: nada.
 */
void descarregarVisual(void) {
    for (int i = 0; i < TOTAL_FONTES; i++) {
        if (fonteCarregada[i]) {
            UnloadFont(fontes[i]);
        }
    }
    UnloadTexture(texturaLogo);
}

/* ==================================================================
 * Texto
 * ================================================================== */

/*
 * larguraDoTexto
 * Recebe: a fonte, o texto e o tamanho da letra.
 * Devolve: a largura que o texto ocupa.
 */
float larguraDoTexto(TipoFonte fonte, const char *texto, float tamanho) {
    return MeasureTextEx(fontes[fonte], texto, tamanho, tamanho * ESPACO_ENTRE_LETRAS).x;
}

/*
 * escrever
 * Escreve um texto com uma sombra escura embaixo, para ler bem por cima
 * da pista.
 * Recebe: a fonte, o texto, o canto superior esquerdo, o tamanho e a cor.
 * Devolve: nada.
 */
void escrever(TipoFonte fonte, const char *texto, float x, float y, float tamanho, Color cor) {
    float espaco = tamanho * ESPACO_ENTRE_LETRAS;
    float sombra = tamanho * DESLOCAMENTO_SOMBRA;
    Color corSombra = Fade(BLACK, 0.55f * cor.a / 255.0f);
    DrawTextEx(fontes[fonte], texto, (Vector2){ x + sombra, y + sombra }, tamanho, espaco, corSombra);
    DrawTextEx(fontes[fonte], texto, (Vector2){ x, y }, tamanho, espaco, cor);
}

void escreverCentralizado(TipoFonte fonte, const char *texto, float centroX, float y, float tamanho, Color cor) {
    escrever(fonte, texto, centroX - larguraDoTexto(fonte, texto, tamanho) / 2.0f, y, tamanho, cor);
}

void escreverADireita(TipoFonte fonte, const char *texto, float direita, float y, float tamanho, Color cor) {
    escrever(fonte, texto, direita - larguraDoTexto(fonte, texto, tamanho), y, tamanho, cor);
}

/* ==================================================================
 * Formas
 * ================================================================== */

/*
 * desenharParalelogramo
 * Um retângulo "deitado para a frente", como as faixas de jogo de
 * corrida: o lado de cima fica "inclinacao" pixels à direita do de baixo.
 * Recebe: x do canto de baixo à esquerda, y do topo, largura, altura,
 *         inclinação e cor.
 * Devolve: nada.
 */
void desenharParalelogramo(float x, float y, float largura, float altura, float inclinacao, Color cor) {
    Vector2 cimaEsquerda  = { x + inclinacao, y };
    Vector2 cimaDireita   = { x + inclinacao + largura, y };
    Vector2 baixoEsquerda = { x, y + altura };
    Vector2 baixoDireita  = { x + largura, y + altura };
    /* A raylib exige os vértices em sentido anti-horário na tela */
    DrawTriangle(cimaEsquerda, baixoEsquerda, baixoDireita, cor);
    DrawTriangle(cimaEsquerda, baixoDireita, cimaDireita, cor);
}

void desenharContornoParalelogramo(float x, float y, float largura, float altura, float inclinacao,
                                   float espessura, Color cor) {
    Vector2 cimaEsquerda  = { x + inclinacao, y };
    Vector2 cimaDireita   = { x + inclinacao + largura, y };
    Vector2 baixoEsquerda = { x, y + altura };
    Vector2 baixoDireita  = { x + largura, y + altura };
    DrawLineEx(cimaEsquerda, cimaDireita, espessura, cor);
    DrawLineEx(cimaDireita, baixoDireita, espessura, cor);
    DrawLineEx(baixoDireita, baixoEsquerda, espessura, cor);
    DrawLineEx(baixoEsquerda, cimaEsquerda, espessura, cor);
}

/*
 * desenharQuadriculado
 * A faixa de bandeira quadriculada, preto e branco.
 * Recebe: posição, largura, altura e o lado de cada quadrado.
 * Devolve: nada.
 */
void desenharQuadriculado(float x, float y, float largura, float altura, float lado) {
    int linhas = (int) (altura / lado + 0.5f);
    int colunas = (int) (largura / lado + 0.999f);
    DrawRectangleRec((Rectangle){ x, y, largura, altura }, COR_GRAFITE);
    for (int l = 0; l < linhas; l++) {
        for (int c = 0; c < colunas; c++) {
            if ((l + c) % 2 == 0) {
                float w = (c == colunas - 1) ? largura - c * lado : lado;   /* o último pode ser cortado */
                DrawRectangleRec((Rectangle){ x + c * lado, y + l * lado, w, lado }, COR_TEXTO_CLARO);
            }
        }
    }
}

/*
 * desenharBarraDeBlocos
 * Barra de atributo em blocos inclinados, como o "medidor" de um painel
 * de carro: os blocos cheios na cor dada, os vazios apagados.
 * Recebe: posição, largura, altura, fração cheia (0 a 1) e cor.
 * Devolve: nada.
 */
void desenharBarraDeBlocos(float x, float y, float largura, float altura, float fracao, Color cor) {
    float inclinacao = altura * 0.5f;
    float bloco = (largura - inclinacao - FOLGA_ENTRE_BLOCOS * (BLOCOS_DA_BARRA - 1)) / BLOCOS_DA_BARRA;
    float cheios = fracao * BLOCOS_DA_BARRA;

    for (int i = 0; i < BLOCOS_DA_BARRA; i++) {
        float bx = x + i * (bloco + FOLGA_ENTRE_BLOCOS);
        desenharParalelogramo(bx, y, bloco, altura, inclinacao, Fade(COR_TEXTO_CLARO, 0.12f));
        if (i < (int) cheios) {
            desenharParalelogramo(bx, y, bloco, altura, inclinacao, cor);
        } else if (i < cheios) {
            /* o último bloco cheio pela metade fica mais claro */
            desenharParalelogramo(bx, y, bloco, altura, inclinacao, Fade(cor, cheios - i));
        }
    }
}

/*
 * ehSeta
 * Recebe: o nome de uma tecla.
 * Devolve: 1 se ela é uma das setas (desenhadas, não escritas).
 */
static int ehSeta(const char *tecla) {
    return strcmp(tecla, "CIMA") == 0 || strcmp(tecla, "BAIXO") == 0 ||
           strcmp(tecla, "ESQ") == 0 || strcmp(tecla, "DIR") == 0;
}

/*
 * larguraDaTecla
 * Recebe: o nome da tecla.
 * Devolve: quanto ela ocupa na horizontal (para centralizar uma fila de dicas).
 */
float larguraDaTecla(const char *tecla) {
    return ehSeta(tecla) ? ALTURA_TECLA : larguraDoTexto(FONTE_FORTE, tecla, LETRA_TECLA) + 14.0f;
}

/*
 * desenharTecla
 * Uma tecla de teclado, para as dicas de controle. "CIMA", "BAIXO",
 * "ESQ" e "DIR" viram setas desenhadas; o resto é escrito na tecla.
 * Recebe: o canto superior esquerdo e o nome da tecla.
 * Devolve: a largura da tecla (para desenhar a próxima ao lado).
 */
float desenharTecla(float x, float y, const char *tecla) {
    float altura = ALTURA_TECLA;
    float tamanhoLetra = LETRA_TECLA;
    float largura = larguraDaTecla(tecla);

    /* Corpo com uma borda mais escura embaixo, como uma tecla de verdade */
    DrawRectangleRounded((Rectangle){ x, y + 2, largura, altura }, 0.3f, 4, (Color){ 120, 124, 136, 255 });
    DrawRectangleRounded((Rectangle){ x, y, largura, altura }, 0.3f, 4, COR_CROMO);

    if (ehSeta(tecla)) {
        float cx = x + largura / 2.0f, cy = y + altura / 2.0f, r = 5.0f;
        Vector2 a, b, c;
        if (strcmp(tecla, "CIMA") == 0) {
            a = (Vector2){ cx, cy - r }; b = (Vector2){ cx - r, cy + r * 0.7f }; c = (Vector2){ cx + r, cy + r * 0.7f };
        } else if (strcmp(tecla, "BAIXO") == 0) {
            a = (Vector2){ cx, cy + r }; b = (Vector2){ cx + r, cy - r * 0.7f }; c = (Vector2){ cx - r, cy - r * 0.7f };
        } else if (strcmp(tecla, "ESQ") == 0) {
            a = (Vector2){ cx - r, cy }; b = (Vector2){ cx + r * 0.7f, cy + r }; c = (Vector2){ cx + r * 0.7f, cy - r };
        } else {
            a = (Vector2){ cx + r, cy }; b = (Vector2){ cx - r * 0.7f, cy - r }; c = (Vector2){ cx - r * 0.7f, cy + r };
        }
        DrawTriangle(a, b, c, COR_GRAFITE);
    } else {
        Vector2 posicao = { x + 7.0f, y + (altura - tamanhoLetra) / 2.0f - 1.0f };
        DrawTextEx(fontes[FONTE_FORTE], tecla, posicao, tamanhoLetra, tamanhoLetra * ESPACO_ENTRE_LETRAS,
                   COR_GRAFITE);
    }
    return largura;
}

/*
 * desenharLogo
 * A logo do jogo na largura pedida (a altura segue a proporção). Sem o
 * arquivo, escreve o nome do jogo no lugar.
 * Recebe: o centro na horizontal, o topo e a largura.
 * Devolve: nada.
 */
void desenharLogo(float centroX, float y, float largura) {
    if (texturaLogo.id == 0) {
        escreverCentralizado(FONTE_TITULO, "KATCHAU", centroX, y, largura * 0.25f, COR_AMARELO);
        return;
    }
    float altura = largura * texturaLogo.height / texturaLogo.width;
    Rectangle origem = { 0.0f, 0.0f, (float) texturaLogo.width, (float) texturaLogo.height };
    Rectangle destino = { centroX - largura / 2.0f, y, largura, altura };
    DrawTexturePro(texturaLogo, origem, destino, (Vector2){ 0, 0 }, 0.0f, WHITE);
}

/*
 * desenharBrilho
 * Um holofote redondo, forte no meio e sumindo na borda. Feito com
 * círculos sobrepostos (e não com DrawCircleGradient, que mudou de
 * parâmetros entre as versões 5 e 6 da raylib).
 * Recebe: o centro, o raio e a cor no meio.
 * Devolve: nada.
 */
void desenharBrilho(float centroX, float centroY, float raio, Color cor) {
    int camadas = 16;
    for (int i = 0; i < camadas; i++) {
        float fracao = 1.0f - (float) i / camadas;   /* de 1 (borda) até perto de 0 (meio) */
        DrawCircleV((Vector2){ centroX, centroY }, raio * fracao, Fade(cor, (float) cor.a / 255.0f / camadas));
    }
}
