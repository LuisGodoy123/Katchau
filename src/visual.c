/*
 * visual.c
 * ------------------------------------------------------------------
 * Fontes, cores e formas da interface. Veja visual.h.
 * ------------------------------------------------------------------
 */

#include <math.h>
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
#define LARGURA_DO_OMBRO       30.0f     /* os botões de cima do controle */
#define COR_CORPO_DO_ICONE     (Color){ 34, 37, 46, 255 }
#define COR_DO_DIRECIONAL      (Color){ 125, 130, 145, 255 }

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

/* ---------- Ícones dos botões do controle ---------- */

/*
 * ehGatilhoOuOmbro
 * Recebe: o nome do ícone, sem o @.
 * Devolve: 1 se é um dos botões de cima do controle.
 */
static int ehGatilhoOuOmbro(const char *nome) {
    static const char *NOMES[] = { "L1", "L2", "R1", "R2", "LB", "LT", "RB", "RT" };
    for (int i = 0; i < 8; i++) {
        if (strcmp(nome, NOMES[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

/*
 * larguraDoIcone
 * Recebe: o nome do ícone, sem o @.
 * Devolve: a largura dele.
 */
static float larguraDoIcone(const char *nome) {
    if (ehGatilhoOuOmbro(nome)) {
        return LARGURA_DO_OMBRO;
    }
    return ALTURA_TECLA + 2.0f;
}

/*
 * letraNoCentro
 * Recebe: o texto, o centro e a cor.
 * Devolve: nada.
 */
static void letraNoCentro(const char *texto, Vector2 centro, float tamanho, Color cor) {
    float largura = larguraDoTexto(FONTE_FORTE, texto, tamanho);
    DrawTextEx(fontes[FONTE_FORTE], texto, (Vector2){ centro.x - largura / 2.0f, centro.y - tamanho / 2.0f - 1.0f }, tamanho,
               tamanho * ESPACO_ENTRE_LETRAS, cor);
}

/*
 * desenharDirecional
 * Uma cruz, com o braço do lado apertado aceso.
 * Recebe: o centro e o lado ("CIMA", "BAIXO", "ESQ" ou "DIR").
 * Devolve: nada.
 */
static void desenharDirecional(Vector2 c, const char *lado) {
    float meio = 3.5f, braco = 10.0f;
    DrawRectangleRec((Rectangle){ c.x - meio, c.y - braco, meio * 2.0f, braco * 2.0f }, COR_DO_DIRECIONAL);
    DrawRectangleRec((Rectangle){ c.x - braco, c.y - meio, braco * 2.0f, meio * 2.0f }, COR_DO_DIRECIONAL);
    Rectangle aceso = { c.x + meio, c.y - meio, braco - meio, meio * 2.0f };
    if (strcmp(lado, "ESQ") == 0) {
        aceso.x = c.x - braco;
    } else if (strcmp(lado, "CIMA") == 0) {
        aceso = (Rectangle){ c.x - meio, c.y - braco, meio * 2.0f, braco - meio };
    } else if (strcmp(lado, "BAIXO") == 0) {
        aceso = (Rectangle){ c.x - meio, c.y + meio, meio * 2.0f, braco - meio };
    }
    DrawRectangleRec(aceso, COR_AMARELO);
}

/*
 * desenharIconeDoControle
 * O desenho de um botão: a forma dele no controle de verdade, para reconhecer sem ler.
 * Recebe: o canto superior esquerdo e o nome, sem o @.
 * Devolve: a largura do ícone.
 */
static float desenharIconeDoControle(float x, float y, const char *nome) {
    float largura = larguraDoIcone(nome);
    Vector2 c = { x + largura / 2.0f, y + ALTURA_TECLA / 2.0f + 1.0f };
    float raio = ALTURA_TECLA / 2.0f + 1.0f;

    /* Os de cima: um "gatilho" (2) é mais alto e arredondado que um "ombro" (1) */
    if (ehGatilhoOuOmbro(nome)) {
        int gatilho = (nome[1] == '2' || nome[1] == 'T');
        Rectangle corpo = { x, y + (gatilho ? 0.0f : 3.0f), largura, ALTURA_TECLA + (gatilho ? 2.0f : -2.0f) };
        DrawRectangleRounded(corpo, gatilho ? 0.65f : 0.35f, 6, COR_CORPO_DO_ICONE);
        DrawRectangleRoundedLinesEx(corpo, gatilho ? 0.65f : 0.35f, 6, 1.0f, COR_CROMO);
        letraNoCentro(nome, c, LETRA_TECLA, COR_TEXTO_CLARO);
        return largura;
    }
    if (strncmp(nome, "DPAD ", 5) == 0) {
        desenharDirecional(c, nome + 5);
        return largura;
    }

    /* Os analógicos: o pino visto de cima */
    if (strncmp(nome, "ANALOG ", 7) == 0 || !strcmp(nome, "L3") || !strcmp(nome, "R3") || !strcmp(nome, "LS") || !strcmp(nome, "RS")) {
        DrawCircleV(c, raio, COR_CORPO_DO_ICONE);
        DrawRing(c, raio - 4.0f, raio - 2.5f, 0.0f, 360.0f, 28, COR_CROMO);
        const char *texto = (nome[0] == 'A') ? (nome[7] == 'E' ? "L" : "R") : nome;
        letraNoCentro(texto, c, (nome[0] == 'A') ? 13.0f : 11.0f, COR_TEXTO_CLARO);
        return largura;
    }

    /* Os do meio */
    if (!strcmp(nome, "OPTIONS") || !strcmp(nome, "START") || !strcmp(nome, "CRIAR") || !strcmp(nome, "BACK")) {
        DrawRectangleRounded((Rectangle){ x + 2.0f, y + 3.0f, largura - 4.0f, ALTURA_TECLA - 4.0f }, 0.45f, 6, COR_CORPO_DO_ICONE);
        if (!strcmp(nome, "OPTIONS") || !strcmp(nome, "START")) {
            for (int i = -1; i <= 1; i++) {
                DrawLineEx((Vector2){ c.x - 5.5f, c.y + i * 4.0f }, (Vector2){ c.x + 5.5f, c.y + i * 4.0f }, 1.6f, COR_TEXTO_CLARO);
            }
        } else {
            DrawRectangleLinesEx((Rectangle){ c.x - 5.0f, c.y - 4.0f, 7.0f, 7.0f }, 1.3f, COR_TEXTO_CLARO);
            DrawRectangleLinesEx((Rectangle){ c.x - 2.0f, c.y - 1.0f, 7.0f, 7.0f }, 1.3f, COR_TEXTO_CLARO);
        }
        return largura;
    }
    if (!strcmp(nome, "PS") || !strcmp(nome, "GUIA")) {
        DrawCircleV(c, raio, COR_CORPO_DO_ICONE);
        letraNoCentro(!strcmp(nome, "PS") ? "PS" : "X", c, 12.0f, COR_TEXTO_CLARO);
        return largura;
    }

    /* Os da frente, no DualSense: a forma e a cor de cada um */
    DrawCircleV(c, raio, COR_CORPO_DO_ICONE);
    float r = raio * 0.52f, grossura = 2.1f;
    if (!strcmp(nome, "CRUZ")) {
        Color cor = { 120, 170, 240, 255 };
        DrawLineEx((Vector2){ c.x - r, c.y - r }, (Vector2){ c.x + r, c.y + r }, grossura, cor);
        DrawLineEx((Vector2){ c.x - r, c.y + r }, (Vector2){ c.x + r, c.y - r }, grossura, cor);
    } else if (!strcmp(nome, "BOLA")) {
        DrawRing(c, r - 0.2f, r + grossura - 0.2f, 0.0f, 360.0f, 28, (Color){ 240, 90, 100, 255 });
    } else if (!strcmp(nome, "QUADRADO")) {
        DrawRectangleLinesEx((Rectangle){ c.x - r, c.y - r, r * 2.0f, r * 2.0f }, grossura, (Color){ 230, 130, 200, 255 });
    } else if (!strcmp(nome, "TRIANGULO")) {
        Color cor = { 90, 210, 170, 255 };
        Vector2 a = { c.x, c.y - r - 1.5f }, b = { c.x - r - 1.0f, c.y + r - 0.5f }, d = { c.x + r + 1.0f, c.y + r - 0.5f };
        DrawLineEx(a, b, grossura, cor);
        DrawLineEx(b, d, grossura, cor);
        DrawLineEx(d, a, grossura, cor);
    } else {
        /* No Xbox: a letra, na cor do botão */
        Color cor = COR_TEXTO_CLARO;
        if (!strcmp(nome, "A")) {
            cor = (Color){ 90, 200, 90, 255 };
        } else if (!strcmp(nome, "B")) {
            cor = (Color){ 240, 90, 90, 255 };
        } else if (!strcmp(nome, "X")) {
            cor = (Color){ 100, 160, 240, 255 };
        } else if (!strcmp(nome, "Y")) {
            cor = (Color){ 240, 200, 70, 255 };
        }
        letraNoCentro(nome, c, 15.0f, cor);
    }
    return largura;
}

/*
 * larguraDaTecla
 * Recebe: o nome da tecla.
 * Devolve: quanto ela ocupa na horizontal (para centralizar uma fila de dicas).
 */
float larguraDaTecla(const char *tecla) {
    if (tecla[0] == '@') {
        return larguraDoIcone(tecla + 1);
    }
    return ehSeta(tecla) ? ALTURA_TECLA : larguraDoTexto(FONTE_FORTE, tecla, LETRA_TECLA) + 14.0f;
}

/*
 * desenharTecla
 * Uma tecla de teclado, para as dicas de controle. "CIMA", "BAIXO",
 * "ESQ" e "DIR" viram setas desenhadas; o resto é escrito na tecla. Com um @ na
 * frente, é um botão do controle, e vira o ícone dele.
 * Recebe: o canto superior esquerdo e o nome da tecla.
 * Devolve: a largura da tecla (para desenhar a próxima ao lado).
 */
float desenharTecla(float x, float y, const char *tecla) {
    if (tecla[0] == '@') {
        return desenharIconeDoControle(x, y, tecla + 1);
    }
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

/* ==================================================================
 * Confete
 * ================================================================== */

/* Dois canhões nos cantos de baixo atiram para cima e para dentro e,
 * por alguns segundos, mais confete cai do alto da tela. Cada papelzinho
 * gira no plano da tela e também "vira" (a largura encolhe e cresce com
 * o cosseno), que é o que dá a impressão de papel caindo. */
#define MAXIMO_CONFETES        420
#define CONFETES_POR_CANHAO    130
#define CONFETES_DA_CHUVA_POR_S 45.0f
#define DURACAO_DA_CHUVA       5.0f     /* s */
#define GRAVIDADE_CONFETE      420.0f   /* px/s² (na tela virtual) */
#define QUEDA_MAXIMA_CONFETE   110.0f   /* px/s: o ar segura o papel */
#define ARRASTO_CONFETE        1.6f     /* 1/s: quanto a velocidade lateral cai */
#define BALANCO_CONFETE        28.0f    /* px/s: o vaivém de folha caindo */

typedef struct {
    float x, y;            /* centro, na tela virtual */
    float vx, vy;          /* px/s */
    float angulo;          /* graus, no plano da tela */
    float giro;            /* graus/s */
    float virada;          /* fase da virada (rad) */
    float velocidadeVirada;
    float largura, altura;
    Color cor;
    int ativo;
} Confete;

static const Color CORES_CONFETE[] = {
    COR_VERMELHO, COR_AMARELO, COR_CROMO, { 255, 255, 255, 255 },
    { 40, 140, 255, 255 }, { 60, 200, 110, 255 }, { 255, 120, 30, 255 }
};
#define TOTAL_CORES_CONFETE ((int) (sizeof(CORES_CONFETE) / sizeof(CORES_CONFETE[0])))

static Confete confetes[MAXIMO_CONFETES];
static float tempoDeChuva = 0.0f;    /* segundos de chuva que ainda faltam */
static float sobraDaChuva = 0.0f;    /* fração de confete acumulada entre quadros */
static float larguraConfete = 800.0f;
static float alturaConfete = 600.0f;

/*
 * sortearEntre
 * Recebe: o mínimo e o máximo.
 * Devolve: um número qualquer entre eles.
 */
static float sortearEntre(float minimo, float maximo) {
    return minimo + (maximo - minimo) * (GetRandomValue(0, 10000) / 10000.0f);
}

/*
 * criarConfete
 * Põe um papelzinho novo numa vaga livre (se não houver vaga, não cria).
 * Recebe: onde ele nasce e a velocidade inicial.
 * Devolve: nada.
 */
static void criarConfete(float x, float y, float vx, float vy) {
    for (int i = 0; i < MAXIMO_CONFETES; i++) {
        if (!confetes[i].ativo) {
            Confete *c = &confetes[i];
            c->x = x;
            c->y = y;
            c->vx = vx;
            c->vy = vy;
            c->angulo = sortearEntre(0.0f, 360.0f);
            c->giro = sortearEntre(-360.0f, 360.0f);
            c->virada = sortearEntre(0.0f, 2.0f * PI);
            c->velocidadeVirada = sortearEntre(5.0f, 12.0f);
            c->largura = sortearEntre(5.0f, 8.0f);
            c->altura = sortearEntre(9.0f, 13.0f);
            c->cor = CORES_CONFETE[GetRandomValue(0, TOTAL_CORES_CONFETE - 1)];
            c->ativo = 1;
            return;
        }
    }
}

/*
 * soltarConfetes
 * Dispara os dois canhões e começa a chuva de confete.
 * Recebe: o tamanho da tela virtual (largura e altura).
 * Devolve: nada.
 */
void soltarConfetes(float largura, float altura) {
    larguraConfete = largura;
    alturaConfete = altura;
    for (int lado = 0; lado < 2; lado++) {
        float x = (lado == 0) ? 0.0f : largura;
        float sentido = (lado == 0) ? 1.0f : -1.0f;   /* para dentro da tela */
        for (int i = 0; i < CONFETES_POR_CANHAO; i++) {
            float angulo = sortearEntre(50.0f, 80.0f) * DEG2RAD;   /* acima da horizontal */
            float forca = sortearEntre(380.0f, 760.0f);
            criarConfete(x, altura + 10.0f, sentido * cosf(angulo) * forca, -sinf(angulo) * forca);
        }
    }
    tempoDeChuva = DURACAO_DA_CHUVA;
    sobraDaChuva = 0.0f;
}

/*
 * desenharConfetes
 * Move e desenha o confete. Chamar uma vez por quadro, por cima da
 * interface (já na tela virtual). O papel que sai por baixo da tela é
 * apagado.
 * Recebe: o tempo do quadro.
 * Devolve: nada.
 */
void desenharConfetes(float tempoQuadro) {
    if (tempoDeChuva > 0.0f) {
        tempoDeChuva -= tempoQuadro;
        sobraDaChuva += CONFETES_DA_CHUVA_POR_S * tempoQuadro;
        while (sobraDaChuva >= 1.0f) {
            criarConfete(sortearEntre(0.0f, larguraConfete), -12.0f,
                         sortearEntre(-30.0f, 30.0f), sortearEntre(20.0f, 80.0f));
            sobraDaChuva -= 1.0f;
        }
    }

    for (int i = 0; i < MAXIMO_CONFETES; i++) {
        Confete *c = &confetes[i];
        if (!c->ativo) {
            continue;
        }
        c->vx -= c->vx * ARRASTO_CONFETE * tempoQuadro;
        c->vy = fminf(c->vy + GRAVIDADE_CONFETE * tempoQuadro, QUEDA_MAXIMA_CONFETE);
        c->virada += c->velocidadeVirada * tempoQuadro;
        c->angulo += c->giro * tempoQuadro;
        c->x += (c->vx + sinf(c->virada * 0.5f) * BALANCO_CONFETE) * tempoQuadro;
        c->y += c->vy * tempoQuadro;
        if (c->y > alturaConfete + 20.0f && c->vy > 0.0f) {
            c->ativo = 0;
            continue;
        }

        /* A virada: o papel fica fino quando está de lado e mais escuro
         * quando mostra o verso */
        float face = cosf(c->virada);
        float largura = c->largura * fmaxf(fabsf(face), 0.15f);
        Color cor = (face >= 0.0f) ? c->cor : ColorBrightness(c->cor, -0.35f);
        DrawRectanglePro((Rectangle){ c->x, c->y, largura, c->altura },
                         (Vector2){ largura / 2.0f, c->altura / 2.0f }, c->angulo, cor);
    }
}

/*
 * limparConfetes
 * Apaga todo o confete (ao começar uma corrida nova).
 * Recebe: nada.
 * Devolve: nada.
 */
void limparConfetes(void) {
    for (int i = 0; i < MAXIMO_CONFETES; i++) {
        confetes[i].ativo = 0;
    }
    tempoDeChuva = 0.0f;
}
