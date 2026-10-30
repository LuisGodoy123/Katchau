/*
 * menu.c
 * ------------------------------------------------------------------
 * Telas fora da corrida: menu principal, garagem, como jogar e
 * resultado. Veja menu.h.
 *
 * A garagem é onde entra o MERGE SORT: a lista de carros é um vetor
 * de ponteiros para FichaTecnica, reordenado por ordenarFichasPorAtributo()
 * toda vez que o jogador troca o critério (velocidade, aceleração ou
 * aderência).
 *
 * O visual (cores da logo, fontes, painéis inclinados, quadriculado)
 * vem do visual.c.
 * ------------------------------------------------------------------
 */

#include <stdio.h>
#include <math.h>
#include "raylib.h"
#include "menu.h"
#include "render.h"
#include "hud.h"
#include "visual.h"
#include "audio.h"

/* ---------- Comuns ---------- */
#define INCLINACAO            12.0f   /* quanto os painéis "deitam" para a frente */
#define Y_BARRA_DICAS         560     /* topo da barra de dicas, no pé da tela */
#define TAMANHO_DICA          15.0f
#define ALTURA_CABECALHO      54.0f
#define ESPACO_ENTRE_LINHAS   1.05f   /* altura de uma linha de texto quebrado, em tamanhos de letra */
#define Y_CABECALHO           22.0f

/* ---------- Menu principal ---------- */
#define VELOCIDADE_DESTAQUE   7.0f    /* 1/s: a animação de uma opção leva ~0,15 s */
#define LARGURA_LOGO         440.0f
#define Y_PRIMEIRA_OPCAO      284.0f
#define ESPACO_OPCOES         56.0f
#define ALTURA_OPCAO          44.0f
#define LARGURA_OPCAO         280.0f
#define LARGURA_OPCAO_MARCADA 320.0f
#define X_CARTAO_CARRO        616.0f
#define Y_CARTAO_CARRO        300.0f
#define LARGURA_CARTAO_CARRO  168.0f
#define ALTURA_CARTAO_CARRO   196.0f

/* ---------- Garagem ---------- */
#define Y_ABAS                92.0f
#define Y_PRIMEIRA_LINHA      134.0f
#define ALTURA_LINHA          50.0f
#define X_LISTA               16.0f
#define LARGURA_LISTA         416.0f
#define LARGURA_VOLTAR        120.0f
#define ALTURA_VOLTAR         30.0f
#define X_VOLTAR              (LARGURA_TELA - 20.0f - LARGURA_VOLTAR)
#define Y_VOLTAR              (Y_CABECALHO + 12.0f)
#define ALTURA_ABA            28.0f
#define X_BARRA_LISTA         226.0f
#define LARGURA_BARRA_LISTA   120.0f
#define X_FICHA               448.0f
#define LARGURA_FICHA         336.0f
#define LARGURA_BARRA_FICHA   200.0f
#define FRACAO_MINIMA_BARRA   0.15f   /* o pior carro ainda mostra um pedacinho de barra */
#define UNIDADES_EM_100_KMH   4000.0f /* 100 km/h em unidades por segundo (KMH_POR_UNIDADE = 0,025) */

/* ---------- Resultado ---------- */
#define Y_TITULOS_TABELA      104.0f
#define Y_PRIMEIRO_RESULTADO  126.0f
#define ALTURA_LINHA_RESULTADO 46.0f
#define X_TABELA              40.0f
#define LARGURA_TABELA        720.0f
#define X_COLUNA_PILOTO       112.0f
#define X_COLUNA_TEMPO        470.0f
#define X_COLUNA_MELHOR       620.0f

static const char *TEXTOS_OPCOES[TOTAL_OPCOES] = { "CORRER", "GARAGEM", "COMO JOGAR", "SAIR" };
static const Tela DESTINOS_OPCOES[TOTAL_OPCOES] = { TELA_CORRIDA, TELA_GARAGEM, TELA_COMO_JOGAR, TELA_SAIR };

/* Os nomes dos critérios com acento (ordenacao.c, que não desenha, usa sem) */
static const char *NOMES_CRITERIO[TOTAL_ATRIBUTOS] = { "VELOCIDADE", "ACELERAÇÃO", "ADERÊNCIA" };

/* Uma dica da barra de baixo: até 4 teclas e o que elas fazem */
typedef struct {
    const char *teclas[4];   /* termina no primeiro NULL */
    const char *acao;
} Dica;

/* ==================================================================
 * Funções auxiliares
 * ================================================================== */

/*
 * Os menus são desenhados num quadro 800x600 no meio da tela (veja
 * comecarInterface() em hud.c). Em monitores largos sobra espaço dos
 * lados: margemX/margemY dizem quanto, em coordenadas virtuais.
 */
static int margemX(void) {
    return (larguraVirtual() - LARGURA_TELA) / 2;
}
static int margemY(void) {
    return (alturaVirtual() - ALTURA_TELA) / 2;
}

/*
 * escurecerTela
 * Cobre a tela inteira (inclusive as sobras dos lados) com um degradê
 * preto, para o texto se destacar da pista passando ao fundo.
 * Recebe: o quanto escurecer em cima, no meio e embaixo (0 a 1).
 * Devolve: nada.
 */
static void escurecerTela(float cima, float meio, float baixo) {
    int x = -margemX();
    int y = -margemY();
    int largura = larguraVirtual();
    int metade = alturaVirtual() / 2;
    DrawRectangleGradientV(x, y, largura, metade, Fade(BLACK, cima), Fade(BLACK, meio));
    DrawRectangleGradientV(x, y + metade, largura, alturaVirtual() - metade, Fade(BLACK, meio), Fade(BLACK, baixo));
}

/*
 * desenharCabecalho
 * Faixa vermelha inclinada com o título da tela, um fio amarelo embaixo
 * e o quadriculado de bandeira até a borda direita.
 * Recebe: o título.
 * Devolve: nada.
 */
static void desenharCabecalho(const char *titulo) {
    float tamanho = 44.0f;
    float fimDaFaixa = 40.0f + larguraDoTexto(FONTE_TITULO, titulo, tamanho) + 36.0f;
    float inicio = -margemX() - INCLINACAO * 2;

    desenharParalelogramo(inicio, Y_CABECALHO, fimDaFaixa - inicio, ALTURA_CABECALHO, INCLINACAO * 1.5f,
                          COR_VERMELHO);
    desenharParalelogramo(inicio, Y_CABECALHO, fimDaFaixa - inicio, ALTURA_CABECALHO / 2, INCLINACAO * 0.75f,
                          Fade(WHITE, 0.10f));   /* brilho na metade de cima */
    desenharParalelogramo(inicio, Y_CABECALHO + ALTURA_CABECALHO + 3, fimDaFaixa - inicio - 4, 5, 3,
                          COR_AMARELO);
    escrever(FONTE_TITULO, titulo, 40.0f, Y_CABECALHO + 3, tamanho, COR_TEXTO_CLARO);

    float xQuadriculado = fimDaFaixa + 40.0f;
    desenharQuadriculado(xQuadriculado, Y_CABECALHO + 15, LARGURA_TELA + margemX() - xQuadriculado,
                         24.0f, 12.0f);
}

/*
 * desenharBarraDeDicas
 * Barra escura no pé da tela, com um fio quadriculado em cima e as
 * dicas de controle (teclas desenhadas + ação) centralizadas.
 * Recebe: as dicas e quantas são.
 * Devolve: nada.
 */
static void desenharBarraDeDicas(const Dica dicas[], int total) {
    float x0 = -margemX();
    float largura = larguraVirtual();
    DrawRectangleRec((Rectangle){ x0, Y_BARRA_DICAS, largura, ALTURA_TELA + margemY() - Y_BARRA_DICAS },
                     Fade(COR_GRAFITE, 0.92f));
    desenharQuadriculado(x0, Y_BARRA_DICAS - 8, largura, 8.0f, 4.0f);

    /* Primeiro mede tudo, para centralizar */
    float espacoTecla = 4.0f, espacoAcao = 8.0f, espacoDica = 30.0f;
    float larguraTotal = 0.0f;
    for (int i = 0; i < total; i++) {
        for (int t = 0; t < 4 && dicas[i].teclas[t] != NULL; t++) {
            larguraTotal += larguraDaTecla(dicas[i].teclas[t]) + espacoTecla;
        }
        larguraTotal += espacoAcao + larguraDoTexto(FONTE_FORTE, dicas[i].acao, TAMANHO_DICA);
        if (i < total - 1) {
            larguraTotal += espacoDica;
        }
    }

    float x = (LARGURA_TELA - larguraTotal) / 2.0f;
    float y = Y_BARRA_DICAS + 9.0f;
    for (int i = 0; i < total; i++) {
        for (int t = 0; t < 4 && dicas[i].teclas[t] != NULL; t++) {
            x += desenharTecla(x, y, dicas[i].teclas[t]) + espacoTecla;
        }
        x += espacoAcao;
        escrever(FONTE_FORTE, dicas[i].acao, x, y + 2, TAMANHO_DICA, COR_TEXTO_FRACO);
        x += larguraDoTexto(FONTE_FORTE, dicas[i].acao, TAMANHO_DICA) + espacoDica;
    }
}

/*
 * desenharSetas
 * Duas setinhas amarelas que andam sem parar, apontando para a opção
 * marcada (como num menu de fliperama).
 * Recebe: onde começam (x), a altura do meio (y) e o sentido (+1 aponta
 *         para a direita, -1 para a esquerda).
 * Devolve: nada.
 */
static void desenharSetas(float x, float centroY, float sentido) {
    float tempo = (float) GetTime();
    for (int i = 0; i < 2; i++) {
        float fase = fmodf(tempo * 1.6f + i * 0.5f, 1.0f);
        float ponta = x + sentido * (fase * 12.0f + 7.0f);
        float base = ponta - sentido * 7.0f;
        Color cor = Fade(COR_AMARELO, 1.0f - fase * 0.7f);
        DrawLineEx((Vector2){ base, centroY - 7 }, (Vector2){ ponta, centroY }, 3.0f, cor);
        DrawLineEx((Vector2){ ponta, centroY }, (Vector2){ base, centroY + 7 }, 3.0f, cor);
    }
}

/*
 * escreverComQuebra
 * Escreve um texto em várias linhas, quebrando entre as palavras para
 * não passar da largura dada.
 * Recebe: fonte, texto, canto superior esquerdo, tamanho, largura máxima e cor.
 * Devolve: a altura (y) logo abaixo da última linha.
 */
static float escreverComQuebra(TipoFonte fonte, const char *texto, float x, float y, float tamanho,
                               float larguraMaxima, Color cor) {
    char linha[256] = "";
    char tentativa[256];
    char palavra[64];
    const char *letra = texto;

    while (*letra != '\0') {
        while (*letra == ' ') {
            letra++;
        }
        int n = 0;
        while (*letra != '\0' && *letra != ' ' && n < (int) sizeof(palavra) - 1) {
            palavra[n++] = *letra++;
        }
        palavra[n] = '\0';
        if (n == 0) {
            break;
        }

        snprintf(tentativa, sizeof(tentativa), "%s%s%s", linha, linha[0] ? " " : "", palavra);
        if (linha[0] != '\0' && larguraDoTexto(fonte, tentativa, tamanho) > larguraMaxima) {
            escrever(fonte, linha, x, y, tamanho, cor);   /* a palavra não cabe: fecha a linha */
            y += tamanho * ESPACO_ENTRE_LINHAS;
            snprintf(linha, sizeof(linha), "%s", palavra);
        } else {
            snprintf(linha, sizeof(linha), "%s", tentativa);
        }
    }
    if (linha[0] != '\0') {
        escrever(fonte, linha, x, y, tamanho, cor);
        y += tamanho * ESPACO_ENTRE_LINHAS;
    }
    return y;
}

/*
 * desenharPainel
 * Painel escuro de fundo com um fio vermelho em cima.
 * Recebe: posição e tamanho.
 * Devolve: nada.
 */
static void desenharPainel(float x, float y, float largura, float altura) {
    DrawRectangleRec((Rectangle){ x, y, largura, altura }, COR_PAINEL);
    DrawRectangleRec((Rectangle){ x, y, largura, 3.0f }, COR_VERMELHO);
}

/*
 * apertouParaCima / apertouParaBaixo / apertouConfirmar / apertouVoltar
 * Teclas aceitas nas telas de menu (setas ou WASD).
 * Recebe: nada.
 * Devolve: 1 se a tecla foi apertada neste quadro.
 */
static int apertouParaCima(void) {
    return IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W);
}
static int apertouParaBaixo(void) {
    return IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S);
}
static int apertouConfirmar(void) {
    return IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_SPACE);
}
static int apertouVoltar(void) {
    return IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE);
}

/*
 * andarNaLista
 * Move a marcação para cima ou para baixo, dando a volta nas pontas.
 * Recebe: o índice marcado e o tamanho da lista.
 * Devolve: o novo índice.
 */
static int andarNaLista(int marcado, int tamanho) {
    if (apertouParaCima()) {
        marcado = (marcado - 1 + tamanho) % tamanho;
    }
    if (apertouParaBaixo()) {
        marcado = (marcado + 1) % tamanho;
    }
    return marcado;
}

/*
 * mouseSeMexeu
 * Recebe: nada.
 * Devolve: 1 se o mouse andou desde o quadro anterior.
 */
static int mouseSeMexeu(void) {
    Vector2 movimento = GetMouseDelta();
    return movimento.x != 0.0f || movimento.y != 0.0f;
}

/*
 * animarDestaque
 * Faz o destaque de um item subir (aceso) ou descer (apagado) aos poucos.
 * Recebe: o destaque atual (0 a 1) e se o item deve estar aceso.
 * Devolve: o novo destaque.
 */
static float animarDestaque(float destaque, int aceso) {
    float passo = GetFrameTime() * VELOCIDADE_DESTAQUE;
    return aceso ? fminf(destaque + passo, 1.0f) : fmaxf(destaque - passo, 0.0f);
}

/*
 * suavizar
 * Faz a animação começar e terminar devagar.
 * Recebe: o destaque (0 a 1).
 * Devolve: o valor suavizado (0 a 1).
 */
static float suavizar(float destaque) {
    return destaque * destaque * (3.0f - 2.0f * destaque);
}

/*
 * dentroDoRetangulo
 * Recebe: o ponto (x, y) e o retângulo (canto superior esquerdo, largura, altura).
 * Devolve: 1 se o ponto está dentro.
 */
static int dentroDoRetangulo(float x, float y, float rx, float ry, float largura, float altura) {
    return x >= rx && x <= rx + largura && y >= ry && y <= ry + altura;
}

/*
 * numeroDaFicha
 * Descobre o número do carro (0 a 7) de uma ficha da garagem.
 * Recebe: a ficha.
 * Devolve: o número, ou 0 se não achar.
 */
static int numeroDaFicha(const FichaTecnica *ficha) {
    for (int numero = 0; numero < TOTAL_CARROS; numero++) {
        if (fichaDoCarro(numero) == ficha) {
            return numero;
        }
    }
    return 0;
}

/* ==================================================================
 * Menu principal
 * ================================================================== */

/*
 * iniciarMenu
 * Estado inicial: "CORRER" marcado, carro 0 escolhido e a garagem
 * ordenada por velocidade.
 * Recebe: o estado do menu.
 * Devolve: nada.
 */
void iniciarMenu(EstadoMenu *menu) {
    menu->opcaoMenu = 0;
    for (int i = 0; i < TOTAL_OPCOES; i++) {
        menu->destaque[i] = (i == menu->opcaoMenu) ? 1.0f : 0.0f;
    }
    menu->carroEscolhido = 0;
    menu->criterio = ATRIBUTO_VELOCIDADE;
    for (int numero = 0; numero < TOTAL_CARROS; numero++) {
        menu->garagem[numero] = fichaDoCarro(numero);
    }
    ordenarFichasPorAtributo(menu->garagem, TOTAL_CARROS, menu->criterio);
    menu->selecionado = 0;
    for (int i = 0; i < TOTAL_CARROS; i++) {
        menu->destaqueLinha[i] = (i == menu->selecionado) ? 1.0f : 0.0f;
    }
    for (int i = 0; i < TOTAL_ATRIBUTOS; i++) {
        menu->destaqueAba[i] = 0.0f;
    }
    menu->abaAnterior = -1;
    menu->voltarAnterior = 0;
    menu->destaqueVoltar = 0.0f;
}

/*
 * opcaoSobOMouse
 * Descobre qual botão do menu principal está embaixo do mouse. A área de
 * cada botão é sempre a do tamanho "marcado", para o botão não piscar
 * quando o mouse está na borda enquanto ele cresce.
 * Recebe: nada.
 * Devolve: o índice da opção, ou -1 se o mouse não está em nenhuma.
 */
static int opcaoSobOMouse(void) {
    float mouseX, mouseY;
    posicaoDoMouseNaInterface(&mouseX, &mouseY);

    float x = (LARGURA_TELA - LARGURA_OPCAO_MARCADA) / 2.0f - INCLINACAO / 2.0f;
    for (int i = 0; i < TOTAL_OPCOES; i++) {
        float y = Y_PRIMEIRA_OPCAO + i * ESPACO_OPCOES;
        if (dentroDoRetangulo(mouseX, mouseY, x, y, LARGURA_OPCAO_MARCADA + INCLINACAO, ALTURA_OPCAO)) {
            return i;
        }
    }
    return -1;
}

/*
 * atualizarMenuPrincipal
 * Cima/baixo ou o mouse trocam a opção; ENTER ou o clique entram nela.
 * O mouse só marca uma opção quando se mexe, para não brigar com o
 * teclado quando ele está parado em cima de um botão.
 * Recebe: o estado do menu.
 * Devolve: a próxima tela.
 */
Tela atualizarMenuPrincipal(EstadoMenu *menu) {
    int opcaoAnterior = menu->opcaoMenu;
    menu->opcaoMenu = andarNaLista(menu->opcaoMenu, TOTAL_OPCOES);

    int sobre = opcaoSobOMouse();
    if (sobre >= 0 && mouseSeMexeu()) {
        menu->opcaoMenu = sobre;
    }
    if (menu->opcaoMenu != opcaoAnterior) {
        tocarSomCursor();
    }

    /* Cada opção acende ao ser marcada e apaga ao perder a marcação */
    for (int i = 0; i < TOTAL_OPCOES; i++) {
        menu->destaque[i] = animarDestaque(menu->destaque[i], i == menu->opcaoMenu);
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && sobre >= 0) {
        menu->opcaoMenu = sobre;
        return DESTINOS_OPCOES[sobre];
    }
    if (apertouConfirmar()) {
        return DESTINOS_OPCOES[menu->opcaoMenu];
    }
    return TELA_MENU;
}

/*
 * desenharOpcao
 * Um botão do menu principal. Conforme o destaque sobe de 0 a 1, o botão
 * cresce (com um pequeno "tranco" no meio), fica vermelho, ganha o fio
 * amarelo na frente e, no fim, as setinhas andando dos dois lados.
 * Recebe: o texto, a altura (y) e o destaque (0 a 1).
 * Devolve: nada.
 */
static void desenharOpcao(const char *texto, float y, float destaque) {
    float centro = LARGURA_TELA / 2.0f;
    float suave = suavizar(destaque);
    float tranco = sinf(destaque * PI) * 6.0f;                      /* incha um pouco no meio da animação */
    float largura = LARGURA_OPCAO + (LARGURA_OPCAO_MARCADA - LARGURA_OPCAO) * suave + tranco;
    float x = centro - largura / 2.0f - INCLINACAO / 2.0f;

    desenharParalelogramo(x, y, largura, ALTURA_OPCAO, INCLINACAO, ColorLerp(COR_PAINEL, COR_VERMELHO, suave));
    if (suave < 1.0f) {
        desenharContornoParalelogramo(x, y, largura, ALTURA_OPCAO, INCLINACAO, 1.5f,
                                      Fade(COR_CROMO, 0.35f * (1.0f - suave)));
    }
    if (suave > 0.0f) {
        float recuo = (1.0f - suave) * 10.0f;   /* os fios amarelos deslizam para o lugar */
        desenharParalelogramo(x + INCLINACAO / 2, y, largura, ALTURA_OPCAO / 2, INCLINACAO / 2,
                              Fade(WHITE, 0.14f * suave));
        desenharParalelogramo(x - 12 + recuo, y, 7, ALTURA_OPCAO, INCLINACAO, Fade(COR_AMARELO, suave));
        desenharParalelogramo(x + largura + 5 - recuo, y, 7, ALTURA_OPCAO, INCLINACAO, Fade(COR_AMARELO, suave));
    }

    float tamanho = 30.0f + 6.0f * suave;
    escreverCentralizado(FONTE_TITULO, texto, centro, y + 7 - 4 * suave, tamanho,
                         ColorLerp(COR_TEXTO_FRACO, COR_TEXTO_CLARO, suave));

    if (destaque >= 1.0f) {
        desenharSetas(x - 40, y + ALTURA_OPCAO / 2, 1.0f);
        desenharSetas(x + largura + INCLINACAO + 40, y + ALTURA_OPCAO / 2, -1.0f);
    }
}

/*
 * desenharCartaoDoCarro
 * Cartão no canto, para lembrar com qual carro o jogador vai correr.
 * Recebe: o número do carro.
 * Devolve: nada.
 */
static void desenharCartaoDoCarro(int numero) {
    float x = X_CARTAO_CARRO, y = Y_CARTAO_CARRO;
    float centro = x + LARGURA_CARTAO_CARRO / 2.0f;

    desenharParalelogramo(x, y, LARGURA_CARTAO_CARRO, ALTURA_CARTAO_CARRO, 0, COR_PAINEL);
    desenharParalelogramo(x - 6, y, LARGURA_CARTAO_CARRO + 6, 26, 8, COR_VERMELHO);
    escreverCentralizado(FONTE_FORTE, "SEU CARRO", centro, y + 3, 17.0f, COR_TEXTO_CLARO);

    desenharBrilho(centro, y + 96, 76, Fade(COR_AMARELO, 0.35f));
    DrawEllipse((int) centro, (int) (y + 140), 70, 9, Fade(BLACK, 0.55f));
    desenharCarroParado(numero, centro, y + 142, 0.85f);

    escreverCentralizado(FONTE_TITULO, fichaDoCarro(numero)->nome, centro, y + 148, 30.0f, COR_AMARELO);
}

/*
 * desenharMenuPrincipal
 * Logo, botões e o carro escolhido. A pista passando ao fundo é
 * desenhada antes, pelo main.c.
 * Recebe: o estado do menu.
 * Devolve: nada.
 */
void desenharMenuPrincipal(EstadoMenu *menu) {
    escurecerTela(0.55f, 0.15f, 0.7f);

    /* A logo flutua devagar */
    float flutuar = sinf((float) GetTime() * 1.6f) * 3.0f;
    desenharLogo(LARGURA_TELA / 2.0f, 6.0f + flutuar, LARGURA_LOGO);

    /* "CORRIDA EM INTERLAGOS" entre dois quadriculados */
    const char *subtitulo = "CORRIDA EM INTERLAGOS";
    float tamanho = 20.0f;
    float meio = larguraDoTexto(FONTE_FORTE, subtitulo, tamanho) / 2.0f;
    escreverCentralizado(FONTE_FORTE, subtitulo, LARGURA_TELA / 2.0f, 228.0f, tamanho, COR_CROMO);
    desenharQuadriculado(LARGURA_TELA / 2.0f - meio - 86, 238.0f, 70.0f, 10.0f, 5.0f);
    desenharQuadriculado(LARGURA_TELA / 2.0f + meio + 16, 238.0f, 70.0f, 10.0f, 5.0f);

    for (int i = 0; i < TOTAL_OPCOES; i++) {
        desenharOpcao(TEXTOS_OPCOES[i], Y_PRIMEIRA_OPCAO + i * ESPACO_OPCOES, menu->destaque[i]);
    }

    desenharCartaoDoCarro(menu->carroEscolhido);

    Dica dicas[] = {
        { { "CIMA", "BAIXO", NULL }, "ESCOLHER" },
        { { "ENTER", NULL }, "CONFIRMAR" },
    };
    desenharBarraDeDicas(dicas, 2);
}

/* ==================================================================
 * Garagem
 * ================================================================== */

/*
 * reordenarGaragem
 * Ordena a garagem pelo critério atual com Merge Sort e mantém marcado
 * o mesmo carro que estava marcado antes (ele só muda de linha).
 * Recebe: o estado do menu.
 * Devolve: nada.
 */
static void reordenarGaragem(EstadoMenu *menu) {
    const FichaTecnica *marcado = menu->garagem[menu->selecionado];

    ordenarFichasPorAtributo(menu->garagem, TOTAL_CARROS, menu->criterio);

    for (int i = 0; i < TOTAL_CARROS; i++) {
        if (menu->garagem[i] == marcado) {
            menu->selecionado = i;
        }
    }
}

#define TAMANHO_ABA 19.0f

/*
 * geometriaDaAba
 * Onde fica a etiqueta de um critério (as abas ficam lado a lado, cada
 * uma com a largura do seu texto). Serve para desenhar e para o mouse.
 * Recebe: o atributo e onde guardar o x e a largura.
 * Devolve: nada.
 */
static void geometriaDaAba(int atributo, float *x, float *largura) {
    float xAtual = 24.0f + larguraDoTexto(FONTE_FORTE, "ORDENAR POR", 15.0f) + 16.0f;
    for (int i = 0; i < atributo; i++) {
        xAtual += larguraDoTexto(FONTE_FORTE, NOMES_CRITERIO[i], TAMANHO_ABA) + 28.0f + 10.0f;
    }
    *x = xAtual;
    *largura = larguraDoTexto(FONTE_FORTE, NOMES_CRITERIO[atributo], TAMANHO_ABA) + 28.0f;
}

/*
 * abaSobOMouse / linhaSobOMouse / sobreOVoltar
 * Dizem o que está embaixo do mouse na garagem (coordenadas do quadro 800x600).
 * Recebe: a posição do mouse.
 * Devolve: o índice da aba / da linha (ou -1), e 1 ou 0 para o botão VOLTAR.
 */
static int abaSobOMouse(float mouseX, float mouseY) {
    for (int atributo = 0; atributo < TOTAL_ATRIBUTOS; atributo++) {
        float x, largura;
        geometriaDaAba(atributo, &x, &largura);
        if (dentroDoRetangulo(mouseX, mouseY, x, Y_ABAS, largura + 9.0f, ALTURA_ABA)) {
            return atributo;
        }
    }
    return -1;
}
static int linhaSobOMouse(float mouseX, float mouseY) {
    for (int i = 0; i < TOTAL_CARROS; i++) {
        if (dentroDoRetangulo(mouseX, mouseY, X_LISTA, Y_PRIMEIRA_LINHA + i * ALTURA_LINHA, LARGURA_LISTA,
                              ALTURA_LINHA)) {
            return i;
        }
    }
    return -1;
}
static int sobreOVoltar(float mouseX, float mouseY) {
    return dentroDoRetangulo(mouseX, mouseY, X_VOLTAR, Y_VOLTAR, LARGURA_VOLTAR + INCLINACAO, ALTURA_VOLTAR);
}

/*
 * atualizarGaragem
 * Cima/baixo marcam um carro; esquerda/direita trocam o critério de
 * ordenação; ENTER escolhe o carro marcado; ESC volta ao menu.
 * Com o mouse: passar por cima marca a linha; clicar numa linha escolhe o
 * carro, numa aba troca o critério e em VOLTAR volta ao menu.
 * Recebe: o estado do menu.
 * Devolve: a próxima tela.
 */
Tela atualizarGaragem(EstadoMenu *menu) {
    int linhaAnterior = menu->selecionado;
    menu->selecionado = andarNaLista(menu->selecionado, TOTAL_CARROS);

    float mouseX, mouseY;
    posicaoDoMouseNaInterface(&mouseX, &mouseY);
    int linha = linhaSobOMouse(mouseX, mouseY);
    int aba = abaSobOMouse(mouseX, mouseY);
    int voltar = sobreOVoltar(mouseX, mouseY);
    if (linha >= 0 && mouseSeMexeu()) {
        menu->selecionado = linha;
    }
    if (menu->selecionado != linhaAnterior) {
        tocarSomCursor();
    }
    /* O mouse entrando numa aba ou no VOLTAR também faz o "tic" */
    if ((aba >= 0 && aba != menu->abaAnterior) || (voltar && !menu->voltarAnterior)) {
        tocarSomCursor();
    }
    menu->abaAnterior = aba;
    menu->voltarAnterior = voltar;

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (voltar) {
            return TELA_MENU;
        }
        if (aba >= 0 && aba != (int) menu->criterio) {
            menu->criterio = (Atributo) aba;
            reordenarGaragem(menu);
        } else if (aba < 0 && linha >= 0) {
            menu->carroEscolhido = numeroDaFicha(menu->garagem[linha]);
            return TELA_MENU;
        }
    }

    /* Linhas, abas e o botão acendem com o mouse (ou a marcação) e apagam ao sair */
    for (int i = 0; i < TOTAL_CARROS; i++) {
        menu->destaqueLinha[i] = animarDestaque(menu->destaqueLinha[i], i == menu->selecionado);
    }
    for (int i = 0; i < TOTAL_ATRIBUTOS; i++) {
        menu->destaqueAba[i] = animarDestaque(menu->destaqueAba[i], i == aba);
    }
    menu->destaqueVoltar = animarDestaque(menu->destaqueVoltar, voltar);

    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
        menu->criterio = (menu->criterio + 1) % TOTAL_ATRIBUTOS;
        reordenarGaragem(menu);
        tocarSomCursor();
    }
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
        menu->criterio = (menu->criterio + TOTAL_ATRIBUTOS - 1) % TOTAL_ATRIBUTOS;
        reordenarGaragem(menu);
        tocarSomCursor();
    }

    if (apertouConfirmar()) {
        menu->carroEscolhido = numeroDaFicha(menu->garagem[menu->selecionado]);
        return TELA_MENU;
    }
    if (apertouVoltar()) {
        return TELA_MENU;
    }
    return TELA_GARAGEM;
}

/*
 * fracaoDoAtributo
 * Quanto da barra preencher: 0 para o pior carro naquele atributo e 1
 * para o melhor (com um mínimo, para a barra nunca sumir).
 * Recebe: a ficha e o atributo.
 * Devolve: um valor entre FRACAO_MINIMA_BARRA e 1.
 */
static float fracaoDoAtributo(const FichaTecnica *ficha, Atributo atributo) {
    float menor = valorDoAtributo(fichaDoCarro(0), atributo);
    float maior = menor;
    for (int numero = 1; numero < TOTAL_CARROS; numero++) {
        float valor = valorDoAtributo(fichaDoCarro(numero), atributo);
        if (valor < menor) {
            menor = valor;
        }
        if (valor > maior) {
            maior = valor;
        }
    }
    if (maior == menor) {
        return 1.0f;
    }
    float fracao = (valorDoAtributo(ficha, atributo) - menor) / (maior - menor);
    return FRACAO_MINIMA_BARRA + fracao * (1.0f - FRACAO_MINIMA_BARRA);
}

/*
 * textoDoAtributo
 * O valor do atributo do jeito que o jogador entende:
 *   velocidade -> km/h; aceleração -> segundos de 0 a 100 km/h (com a
 *   mesma curva de aceleração da corrida); aderência -> porcentagem.
 * Recebe: a ficha e o atributo.
 * Devolve: o texto (válido até a próxima chamada de TextFormat).
 */
static const char *textoDoAtributo(const FichaTecnica *ficha, Atributo atributo) {
    switch (atributo) {
        case ATRIBUTO_VELOCIDADE:
            return TextFormat("%d km/h", converterParaKmh(ficha->velocidadeMaxima));
        case ATRIBUTO_ACELERACAO: {
            int centesimos = (int) (tempoAteVelocidade(ficha, UNIDADES_EM_100_KMH) * 100 + 0.5f);
            return TextFormat("%d,%02d s", centesimos / 100, centesimos % 100);
        }
        case ATRIBUTO_ADERENCIA:
            return TextFormat("%d%%", (int) (ficha->aderencia * 100 + 0.5f));
        default:
            return "";
    }
}

/*
 * desenharAbasDeCriterio
 * Os três critérios lado a lado, em etiquetas inclinadas; o ativo fica
 * amarelo e a que está embaixo do mouse cresce e fica vermelha.
 * Recebe: o estado do menu.
 * Devolve: nada.
 */
static void desenharAbasDeCriterio(EstadoMenu *menu) {
    escrever(FONTE_FORTE, "ORDENAR POR", 24.0f, Y_ABAS + 5, 15.0f, COR_TEXTO_FRACO);

    float xFim = 0.0f;
    for (int atributo = 0; atributo < TOTAL_ATRIBUTOS; atributo++) {
        float x, largura;
        geometriaDaAba(atributo, &x, &largura);
        float suave = suavizar(menu->destaqueAba[atributo]);
        float cresce = 6.0f * suave;
        x -= cresce / 2.0f;
        largura += cresce;
        float y = Y_ABAS - suave * 2.0f;

        if (atributo == (int) menu->criterio) {
            desenharParalelogramo(x, y, largura, ALTURA_ABA, 9, ColorLerp(COR_AMARELO, WHITE, suave * 0.35f));
            escreverCentralizado(FONTE_FORTE, NOMES_CRITERIO[atributo], x + 4 + largura / 2, y + 2, TAMANHO_ABA,
                                 COR_GRAFITE);
        } else {
            desenharParalelogramo(x, y, largura, ALTURA_ABA, 9, ColorLerp(COR_PAINEL, COR_VERMELHO, suave));
            desenharContornoParalelogramo(x, y, largura, ALTURA_ABA, 9, 1.5f,
                                          Fade(COR_CROMO, 0.4f * (1.0f - suave)));
            escreverCentralizado(FONTE_FORTE, NOMES_CRITERIO[atributo], x + 4 + largura / 2, y + 2, TAMANHO_ABA,
                                 COR_TEXTO_CLARO);
        }
        xFim = x + largura + 10.0f;
    }
    escrever(FONTE_TEXTO, "ordenado com Merge Sort", xFim + 8, Y_ABAS + 6, 14.0f, COR_TEXTO_FRACO);
}

/*
 * desenharBotaoVoltar
 * Botão no canto do cabeçalho. Ao passar o mouse ele cresce, fica amarelo
 * e a setinha da esquerda dá um pulinho.
 * Recebe: o destaque (0 a 1).
 * Devolve: nada.
 */
static void desenharBotaoVoltar(float destaque) {
    float suave = suavizar(destaque);
    float cresce = 10.0f * suave;
    float x = X_VOLTAR - cresce / 2.0f;
    float largura = LARGURA_VOLTAR + cresce;

    desenharParalelogramo(x, Y_VOLTAR, largura, ALTURA_VOLTAR, 9, ColorLerp(COR_GRAFITE, COR_AMARELO, suave));
    desenharContornoParalelogramo(x, Y_VOLTAR, largura, ALTURA_VOLTAR, 9, 1.5f,
                                  ColorLerp(Fade(COR_CROMO, 0.5f), COR_AMARELO, suave));
    escreverCentralizado(FONTE_FORTE, "VOLTAR", x + 8 + largura / 2, Y_VOLTAR + 3, 18.0f,
                         ColorLerp(COR_TEXTO_CLARO, COR_GRAFITE, suave));

    /* Setinha para a esquerda, que se afasta e volta enquanto o mouse está em cima */
    float pulo = suave * (2.0f + 3.0f * fabsf(sinf((float) GetTime() * 6.0f)));
    float ponta = x + 14.0f - pulo;
    float meio = Y_VOLTAR + ALTURA_VOLTAR / 2.0f;
    Color cor = ColorLerp(COR_AMARELO, COR_GRAFITE, suave);
    DrawLineEx((Vector2){ ponta + 6, meio - 6 }, (Vector2){ ponta, meio }, 2.5f, cor);
    DrawLineEx((Vector2){ ponta, meio }, (Vector2){ ponta + 6, meio + 6 }, 2.5f, cor);
}

/*
 * desenharListaDaGaragem
 * As 8 fichas, na ordem do Merge Sort, com a barra do critério atual.
 * Recebe: o estado do menu.
 * Devolve: nada.
 */
static void desenharListaDaGaragem(EstadoMenu *menu) {
    desenharPainel(X_LISTA, Y_PRIMEIRA_LINHA - 6, LARGURA_LISTA, TOTAL_CARROS * ALTURA_LINHA + 10);

    for (int i = 0; i < TOTAL_CARROS; i++) {
        const FichaTecnica *ficha = menu->garagem[i];
        float y = Y_PRIMEIRA_LINHA + i * ALTURA_LINHA;
        int ehEscolhido = (numeroDaFicha(ficha) == menu->carroEscolhido);
        float suave = suavizar(menu->destaqueLinha[i]);

        if (suave > 0.0f) {
            /* A marcação entra deslizando da esquerda enquanto acende */
            float recuo = (1.0f - suave) * 16.0f;
            desenharParalelogramo(X_LISTA + 4 - recuo, y + 2, LARGURA_LISTA - 16, ALTURA_LINHA - 4, 10,
                                  Fade(COR_VERMELHO, suave));
            desenharParalelogramo(X_LISTA - 2 - recuo, y + 2, 6, ALTURA_LINHA - 4, 10, Fade(COR_AMARELO, suave));
        }

        /* Número da posição numa etiqueta */
        Color fundoNumero = ColorLerp(COR_GRAFITE, COR_AMARELO, suave);
        Color corNumero = ColorLerp(COR_CROMO, COR_GRAFITE, suave);
        desenharParalelogramo(X_LISTA + 14, y + 9, 34, 30, 7, fundoNumero);
        escreverCentralizado(FONTE_TITULO, TextFormat("%d", i + 1), X_LISTA + 34, y + 8, 26.0f, corNumero);

        escrever(FONTE_TITULO, ficha->nome, X_LISTA + 60, y + (ehEscolhido ? 2 : 9), 26.0f,
                 ColorLerp(COR_CROMO, COR_TEXTO_CLARO, suave));
        if (ehEscolhido) {
            escrever(FONTE_FORTE, "SEU CARRO", X_LISTA + 61, y + 29, 13.0f, COR_AMARELO);
        }

        desenharBarraDeBlocos(X_BARRA_LISTA, y + 18, LARGURA_BARRA_LISTA, 13,
                              fracaoDoAtributo(ficha, menu->criterio), ColorLerp(COR_CROMO, COR_AMARELO, suave));
        escreverADireita(FONTE_FORTE, textoDoAtributo(ficha, menu->criterio),
                         X_LISTA + LARGURA_LISTA - 14, y + 12, 21.0f, COR_TEXTO_CLARO);
    }
}

/*
 * desenharFichaMarcada
 * Painel da direita: o carro marcado em tamanho grande, iluminado, e os
 * três atributos.
 * Recebe: o estado do menu.
 * Devolve: nada.
 */
static void desenharFichaMarcada(EstadoMenu *menu) {
    const FichaTecnica *ficha = menu->garagem[menu->selecionado];
    int numero = numeroDaFicha(ficha);
    float topo = Y_PRIMEIRA_LINHA - 6;
    float centroX = X_FICHA + LARGURA_FICHA / 2.0f;

    desenharPainel(X_FICHA, topo, LARGURA_FICHA, TOTAL_CARROS * ALTURA_LINHA + 10);
    escrever(FONTE_FORTE, "FICHA TÉCNICA", X_FICHA + 14, topo + 10, 14.0f, COR_TEXTO_FRACO);

    /* Holofote e sombra embaixo do carro */
    desenharBrilho(centroX, topo + 110, 120, Fade(COR_AMARELO, 0.32f));
    DrawEllipse((int) centroX, (int) (topo + 168), 110, 12, Fade(BLACK, 0.6f));
    desenharCarroParado(numero, centroX, topo + 170, 1.15f);

    escreverCentralizado(FONTE_TITULO, ficha->nome, centroX, topo + 172, 42.0f, COR_AMARELO);
    escreverCentralizado(FONTE_TEXTO, ficha->descricao, centroX, topo + 220, 18.0f, COR_CROMO);

    float y = topo + 256;
    for (int atributo = 0; atributo < TOTAL_ATRIBUTOS; atributo++) {
        float linha = y + atributo * 46;
        int ativo = (atributo == (int) menu->criterio);
        Color cor = ativo ? COR_AMARELO : COR_CROMO;
        escrever(FONTE_FORTE, NOMES_CRITERIO[atributo], X_FICHA + 16, linha, 15.0f,
                 ativo ? COR_AMARELO : COR_TEXTO_FRACO);
        desenharBarraDeBlocos(X_FICHA + 16, linha + 21, LARGURA_BARRA_FICHA, 14,
                              fracaoDoAtributo(ficha, (Atributo) atributo), cor);
        escreverADireita(FONTE_FORTE, textoDoAtributo(ficha, (Atributo) atributo),
                         X_FICHA + LARGURA_FICHA - 16, linha + 14, 22.0f, COR_TEXTO_CLARO);
    }
    escrever(FONTE_TEXTO, "aceleração: segundos de 0 a 100 km/h", X_FICHA + 16, y + 3 * 46 - 2, 14.0f,
             COR_TEXTO_FRACO);
}

/*
 * desenharGaragem
 * Recebe: o estado do menu (o fundo é desenhado antes, pelo main.c).
 * Devolve: nada.
 */
void desenharGaragem(EstadoMenu *menu) {
    escurecerTela(0.7f, 0.55f, 0.75f);

    desenharCabecalho("GARAGEM");
    desenharBotaoVoltar(menu->destaqueVoltar);
    desenharAbasDeCriterio(menu);
    desenharListaDaGaragem(menu);
    desenharFichaMarcada(menu);

    Dica dicas[] = {
        { { "CIMA", "BAIXO", NULL }, "CARRO" },
        { { "ESQ", "DIR", NULL }, "ORDENAR" },
        { { "ENTER", NULL }, "ESCOLHER" },
        { { "ESC", NULL }, "VOLTAR" },
    };
    desenharBarraDeDicas(dicas, 4);
}

/* ==================================================================
 * Como jogar
 * ================================================================== */

/*
 * atualizarComoJogar
 * Qualquer uma das teclas de confirmar ou voltar retorna ao menu.
 * Recebe: nada.
 * Devolve: a próxima tela.
 */
Tela atualizarComoJogar(void) {
    if (apertouConfirmar() || apertouVoltar()) {
        return TELA_MENU;
    }
    return TELA_COMO_JOGAR;
}

/*
 * desenharTituloDePainel
 * Título amarelo de um painel, com um traço vermelho embaixo.
 * Recebe: o texto e o canto superior esquerdo.
 * Devolve: nada.
 */
static void desenharTituloDePainel(const char *texto, float x, float y) {
    escrever(FONTE_TITULO, texto, x, y, 28.0f, COR_AMARELO);
    desenharParalelogramo(x, y + 36, 60, 4, 3, COR_VERMELHO);
}

/*
 * desenharComoJogar
 * Controles (com as teclas desenhadas) e dicas de pilotagem.
 * Recebe: nada (o fundo é desenhado antes, pelo main.c).
 * Devolve: nada.
 */
void desenharComoJogar(void) {
    escurecerTela(0.7f, 0.55f, 0.75f);
    desenharCabecalho("COMO JOGAR");

    /* ---- Controles ---- */
    float x = 24.0f, topo = 100.0f;
    desenharPainel(x, topo, 352, 444);
    desenharTituloDePainel("CONTROLES", x + 20, topo + 14);

    const char *teclas[][4] = {
        { "CIMA", "W", NULL },
        { "BAIXO", "S", NULL },
        { "ESQ", "DIR", "A", "D" },
        { "ESC", NULL },
        { "ENTER", NULL },
    };
    const char *acoes[] = { "ACELERAR", "FREAR", "VIRAR", "ABANDONAR A CORRIDA", "CONFIRMAR NOS MENUS" };
    for (int i = 0; i < 5; i++) {
        float y = topo + 78 + i * 70;
        float xTecla = x + 20;
        for (int t = 0; t < 4 && teclas[i][t] != NULL; t++) {
            xTecla += desenharTecla(xTecla, y, teclas[i][t]) + 5;
        }
        escrever(FONTE_TITULO, acoes[i], x + 140, y - 2, 20.0f, COR_TEXTO_CLARO);
        if (i < 4) {
            DrawRectangleRec((Rectangle){ x + 20, y + 44, 312, 1 }, Fade(COR_CROMO, 0.15f));
        }
    }

    /* ---- Dicas ---- */
    x = 392.0f;
    desenharPainel(x, topo, 384, 444);
    desenharTituloDePainel("DICAS", x + 20, topo + 14);

    const char *dicas[] = {
        TextFormat("%d voltas em Interlagos contra 7 rivais. Você larga em último.", TOTAL_VOLTAS),
        "O carro acelera como um F1: forte na saída e cada vez menos perto dos 300 km/h.",
        "Freie antes das curvas fechadas. Fumaça nos pneus quer dizer que o carro está quase escapando.",
        "Na grama (sai poeira) o carro não passa de 75 km/h.",
        "Bater na traseira de outro carro tira velocidade.",
        "Na garagem, compare velocidade, aceleração e aderência antes de escolher.",
    };
    float y = topo + 72;
    for (int i = 0; i < 6; i++) {
        desenharParalelogramo(x + 20, y + 6, 10, 10, 4, COR_VERMELHO);
        y = escreverComQuebra(FONTE_TEXTO, dicas[i], x + 40, y, 19.0f, 320, COR_TEXTO_CLARO) + 14;
    }

    Dica barra[] = {
        { { "ENTER", "ESC", NULL }, "VOLTAR AO MENU" },
    };
    desenharBarraDeDicas(barra, 1);
}

/* ==================================================================
 * Resultado
 * ================================================================== */

/*
 * atualizarResultado
 * ENTER corre de novo (com o mesmo carro); ESC volta ao menu.
 * Recebe: nada.
 * Devolve: a próxima tela.
 */
Tela atualizarResultado(void) {
    if (apertouConfirmar()) {
        return TELA_CORRIDA;
    }
    if (apertouVoltar()) {
        return TELA_MENU;
    }
    return TELA_RESULTADO;
}

/*
 * melhorVolta
 * Recebe: o carro.
 * Devolve: o menor tempo entre as voltas já completadas (0 se nenhuma).
 */
static float melhorVolta(Carro *carro) {
    float melhor = 0.0f;
    for (int i = 0; i < carro->voltas && i < TOTAL_VOLTAS; i++) {
        if (melhor == 0.0f || carro->temposDeVolta[i] < melhor) {
            melhor = carro->temposDeVolta[i];
        }
    }
    return melhor;
}

/*
 * corDoPodio
 * Recebe: a posição (0 = 1º lugar).
 * Devolve: ouro, prata ou bronze para o pódio; grafite para o resto.
 */
static Color corDoPodio(int posicao) {
    switch (posicao) {
        case 0: return COR_OURO;
        case 1: return COR_PRATA;
        case 2: return COR_BRONZE;
        default: return COR_GRAFITE;
    }
}

/*
 * desenharLinhaDoResultado
 * Uma linha da tabela: posição (com a cor do pódio), piloto, tempo total
 * e melhor volta. Quem ainda não chegou mostra em que volta está.
 * Recebe: a corrida, a posição (0 = 1º lugar) e a altura da linha.
 * Devolve: nada.
 */
static void desenharLinhaDoResultado(Corrida *corrida, int posicao, float y) {
    Carro *carro = corrida->classificacao[posicao];
    int ehJogador = (carro == &corrida->jogador);
    char tempo[32];

    if (ehJogador) {
        desenharParalelogramo(X_TABELA, y + 2, LARGURA_TABELA, ALTURA_LINHA_RESULTADO - 6, 10, COR_VERMELHO);
        desenharParalelogramo(X_TABELA - 8, y + 2, 6, ALTURA_LINHA_RESULTADO - 6, 10, COR_AMARELO);
    } else {
        desenharParalelogramo(X_TABELA, y + 2, LARGURA_TABELA, ALTURA_LINHA_RESULTADO - 6, 10,
                              Fade(COR_GRAFITE, posicao % 2 == 0 ? 0.85f : 0.7f));
    }

    Color podio = corDoPodio(posicao);
    desenharParalelogramo(X_TABELA + 12, y + 7, 46, 30, 7, podio);
    escreverCentralizado(FONTE_TITULO, TextFormat("%dº", posicao + 1), X_TABELA + 38, y + 6, 24.0f,
                         posicao < 3 ? COR_GRAFITE : COR_CROMO);

    escrever(FONTE_TITULO, carro->nome, X_COLUNA_PILOTO, y + 6, 26.0f, COR_TEXTO_CLARO);
    if (ehJogador) {
        float x = X_COLUNA_PILOTO + larguraDoTexto(FONTE_TITULO, carro->nome, 26.0f) + 12;
        desenharParalelogramo(x, y + 12, 52, 20, 5, COR_AMARELO);
        escreverCentralizado(FONTE_FORTE, "VOCÊ", x + 28, y + 12, 15.0f, COR_GRAFITE);
    }

    Color corTempo = ehJogador ? COR_TEXTO_CLARO : COR_CROMO;
    if (carro->voltas >= TOTAL_VOLTAS) {
        formatarTempo(carro->tempoDeChegada, tempo, sizeof(tempo));
        escrever(FONTE_FORTE, tempo, X_COLUNA_TEMPO, y + 8, 22.0f, corTempo);
    } else {
        escrever(FONTE_FORTE, TextFormat("VOLTA %d/%d", voltaAtual(carro), TOTAL_VOLTAS),
                 X_COLUNA_TEMPO, y + 8, 22.0f, COR_TEXTO_FRACO);
    }

    float melhor = melhorVolta(carro);
    if (melhor > 0.0f) {
        formatarTempo(melhor, tempo, sizeof(tempo));
        escrever(FONTE_FORTE, tempo, X_COLUNA_MELHOR, y + 8, 22.0f, corTempo);
    } else {
        escrever(FONTE_FORTE, "-", X_COLUNA_MELHOR, y + 8, 22.0f, COR_TEXTO_FRACO);
    }
}

/*
 * desenharResultado
 * Tabela de chegada com os 8 carros, por cima da corrida (que continua
 * andando ao fundo: os rivais que ainda não chegaram terminam a prova
 * e a tabela vai se completando).
 * A ordem vem direto de corrida->classificacao, já ordenada pelo
 * Insertion Sort: quem chegou fica na frente, pelo tempo de chegada.
 * Recebe: a corrida.
 * Devolve: nada.
 */
void desenharResultado(Corrida *corrida) {
    escurecerTela(0.75f, 0.6f, 0.8f);

    if (corrida->posicaoFinal == 1) {
        desenharCabecalho("VOCÊ VENCEU!");
    } else {
        desenharCabecalho(TextFormat("VOCÊ CHEGOU EM %dº", corrida->posicaoFinal));
    }

    escrever(FONTE_FORTE, "POS", X_TABELA + 22, Y_TITULOS_TABELA - 8, 14.0f, COR_TEXTO_FRACO);
    escrever(FONTE_FORTE, "PILOTO", X_COLUNA_PILOTO, Y_TITULOS_TABELA - 8, 14.0f, COR_TEXTO_FRACO);
    escrever(FONTE_FORTE, "TEMPO", X_COLUNA_TEMPO, Y_TITULOS_TABELA - 8, 14.0f, COR_TEXTO_FRACO);
    escrever(FONTE_FORTE, "MELHOR VOLTA", X_COLUNA_MELHOR, Y_TITULOS_TABELA - 8, 14.0f, COR_TEXTO_FRACO);

    for (int posicao = 0; posicao < TOTAL_CARROS; posicao++) {
        desenharLinhaDoResultado(corrida, posicao, Y_PRIMEIRO_RESULTADO + posicao * ALTURA_LINHA_RESULTADO);
    }

    if (!todosChegaram(corrida)) {
        /* Pisca devagar, para mostrar que a tabela ainda vai mudar */
        float alfa = 0.55f + 0.45f * sinf((float) GetTime() * 3.0f);
        const char *aviso = "OS OUTROS CARROS AINDA ESTÃO TERMINANDO A PROVA...";
        float largura = larguraDoTexto(FONTE_FORTE, aviso, 16.0f) + 40;
        desenharParalelogramo(LARGURA_TELA / 2.0f - largura / 2, 502, largura, 26, 8, Fade(COR_GRAFITE, 0.9f));
        escreverCentralizado(FONTE_FORTE, aviso, LARGURA_TELA / 2.0f + 4, 505, 16.0f, Fade(COR_CROMO, alfa));
    }

    Dica dicas[] = {
        { { "ENTER", NULL }, "CORRER DE NOVO" },
        { { "ESC", NULL }, "MENU" },
    };
    desenharBarraDeDicas(dicas, 2);
}
