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
#include "configuracoes.h"
#include "render.h"
#include "hud.h"
#include "visual.h"
#include "audio.h"
#include "controle.h"

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
#define ESPACO_OPCOES         50.0f
#define FOCO_INICIAL_DA_CORRIDA 0        /* a linha da pista */
#define ALTURA_OPCAO          44.0f
#define LARGURA_OPCAO         280.0f
#define LARGURA_OPCAO_MARCADA 320.0f
#define X_CARTAO_CARRO        616.0f
#define Y_CARTAO_CARRO        300.0f
#define LARGURA_CARTAO_CARRO  168.0f
#define ALTURA_CARTAO_CARRO   196.0f

/* ---------- Pausa ---------- */
#define Y_TITULO_PAUSA        176.0f
#define Y_PRIMEIRA_OPCAO_PAUSA 270.0f

/* ---------- Garagem ---------- */
#define Y_ABAS                92.0f
#define Y_PRIMEIRA_LINHA      134.0f
#define ALTURA_LINHA          29.5f
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
#define ALTURA_LINHA_RESULTADO 30.0f
#define X_TABELA              40.0f
#define LARGURA_TABELA        720.0f
#define X_COLUNA_PILOTO       112.0f
#define X_COLUNA_TEMPO        470.0f
#define X_COLUNA_MELHOR       620.0f

static const char *TEXTOS_OPCOES[TOTAL_OPCOES] = { "CORRER", "GARAGEM", "COMO JOGAR", "CONFIGURAÇÕES", "SAIR" };
static const Tela DESTINOS_OPCOES[TOTAL_OPCOES] = { TELA_PISTAS, TELA_GARAGEM, TELA_COMO_JOGAR, TELA_CONFIGURACOES, TELA_SAIR };
static const char *TEXTOS_PAUSA[TOTAL_OPCOES_PAUSA] = { "CONTINUAR", "MENU PRINCIPAL" };
static const EscolhaDaPausa ESCOLHAS_PAUSA[TOTAL_OPCOES_PAUSA] = { PAUSA_CONTINUAR, PAUSA_SAIR };

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
            larguraTotal += larguraDaTecla(nomeNoControle(dicas[i].teclas[t])) + espacoTecla;
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
            x += desenharTecla(x, y, nomeNoControle(dicas[i].teclas[t])) + espacoTecla;
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
 * apertouParaCima / apertouParaBaixo / apertouParaEsquerda / apertouParaDireita /
 * apertouConfirmar / apertouVoltar
 * Teclas aceitas nas telas de menu (setas ou WASD) e os botões do
 * controle que fazem o mesmo (direcional ou analógico, A e B).
 * Recebe: nada.
 * Devolve: 1 se a tecla ou o botão foi apertado neste quadro.
 */
static int apertouParaCima(void) {
    return IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W) || controleApertou(BOTAO_CIMA);
}
static int apertouParaBaixo(void) {
    return IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S) || controleApertou(BOTAO_BAIXO);
}
static int apertouParaEsquerda(void) {
    return IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A) || controleApertou(BOTAO_ESQUERDA);
}
static int apertouParaDireita(void) {
    return IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D) || controleApertou(BOTAO_DIREITA);
}
static int apertouConfirmar(void) {
    return IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_SPACE) ||
           controleApertou(BOTAO_CONFIRMAR);
}
static int apertouVoltar(void) {
    return IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE) || controleApertou(BOTAO_VOLTAR);
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
    abrirPausa(menu);
    menu->linhaConfig = 0;
    menu->colunaConfig = 0;
    menu->esperandoComando = 0;
    menu->tempoEsperando = 0.0f;
    menu->confirmarPadrao = 0.0f;
    menu->pulsoDeVibracao = 0.0f;
    menu->pistaEscolhida = PISTA_INTERLAGOS;
    menu->nivelEscolhido = NIVEL_MEDIO;
    menu->focoDaCorrida = FOCO_INICIAL_DA_CORRIDA;
    menu->destaqueCorrer = 0.0f;
    for (int i = 0; i < TOTAL_NIVEIS; i++) {
        menu->destaqueNivel[i] = (i == (int) menu->nivelEscolhido) ? 1.0f : 0.0f;
    }
    for (int i = 0; i < TOTAL_PISTAS; i++) {
        menu->destaquePista[i] = (i == menu->pistaEscolhida) ? 1.0f : 0.0f;
    }
}

/*
 * opcaoSobOMouse
 * Descobre qual botão de uma coluna de botões (a do menu principal ou a
 * da pausa) está embaixo do mouse. A área de cada botão é sempre a do
 * tamanho "marcado", para o botão não piscar quando o mouse está na
 * borda enquanto ele cresce.
 * Recebe: a altura (y) do primeiro botão e quantos botões há.
 * Devolve: o índice da opção, ou -1 se o mouse não está em nenhuma.
 */
static int opcaoSobOMouse(float yPrimeira, int totalOpcoes) {
    float mouseX, mouseY;
    posicaoDoMouseNaInterface(&mouseX, &mouseY);

    float x = (LARGURA_TELA - LARGURA_OPCAO_MARCADA) / 2.0f - INCLINACAO / 2.0f;
    for (int i = 0; i < totalOpcoes; i++) {
        float y = yPrimeira + i * ESPACO_OPCOES;
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

    int sobre = opcaoSobOMouse(Y_PRIMEIRA_OPCAO, TOTAL_OPCOES);
    if (sobre >= 0 && mouseSeMexeu()) {
        menu->opcaoMenu = sobre;
    }
    if (menu->opcaoMenu != opcaoAnterior) {
        tocarSom(SOM_CURSOR);
    }

    /* Cada opção acende ao ser marcada e apaga ao perder a marcação */
    for (int i = 0; i < TOTAL_OPCOES; i++) {
        menu->destaque[i] = animarDestaque(menu->destaque[i], i == menu->opcaoMenu);
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && sobre >= 0) {
        menu->opcaoMenu = sobre;
        tocarSom(SOM_CONFIRMAR);
        return DESTINOS_OPCOES[sobre];
    }
    if (apertouConfirmar()) {
        tocarSom(SOM_CONFIRMAR);
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
    desenharCarroParado(numero, centro, y + 140, 0.86f);

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
        tocarSom(SOM_CURSOR);
    }
    /* O mouse entrando numa aba ou no VOLTAR também faz o "tic" */
    if ((aba >= 0 && aba != menu->abaAnterior) || (voltar && !menu->voltarAnterior)) {
        tocarSom(SOM_CURSOR);
    }
    menu->abaAnterior = aba;
    menu->voltarAnterior = voltar;

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (voltar) {
            tocarSom(SOM_VOLTAR);
            return TELA_MENU;
        }
        if (aba >= 0 && aba != (int) menu->criterio) {
            menu->criterio = (Atributo) aba;
            reordenarGaragem(menu);
            tocarSom(SOM_CONFIRMAR);
        } else if (aba < 0 && linha >= 0) {
            menu->carroEscolhido = numeroDaFicha(menu->garagem[linha]);
            tocarSom(SOM_CONFIRMAR);
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

    if (apertouParaDireita()) {
        menu->criterio = (menu->criterio + 1) % TOTAL_ATRIBUTOS;
        reordenarGaragem(menu);
        tocarSom(SOM_CURSOR);
    }
    if (apertouParaEsquerda()) {
        menu->criterio = (menu->criterio + TOTAL_ATRIBUTOS - 1) % TOTAL_ATRIBUTOS;
        reordenarGaragem(menu);
        tocarSom(SOM_CURSOR);
    }

    if (apertouConfirmar()) {
        menu->carroEscolhido = numeroDaFicha(menu->garagem[menu->selecionado]);
        tocarSom(SOM_CONFIRMAR);
        return TELA_MENU;
    }
    if (apertouVoltar()) {
        tocarSom(SOM_VOLTAR);
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
        desenharParalelogramo(X_LISTA + 14, y + 4, 30, 21, 5, fundoNumero);
        escreverCentralizado(FONTE_TITULO, TextFormat("%d", i + 1), X_LISTA + 30, y + 4, 17.0f, corNumero);

        escrever(FONTE_TITULO, ficha->nome, X_LISTA + 52, y + 4, 18.0f, ColorLerp(COR_CROMO, COR_TEXTO_CLARO, suave));
        if (ehEscolhido) {
            float x = X_LISTA + 52 + larguraDoTexto(FONTE_TITULO, ficha->nome, 18.0f) + 8;
            escrever(FONTE_FORTE, "SEU", x, y + 7, 12.0f, COR_AMARELO);
        }

        desenharBarraDeBlocos(X_BARRA_LISTA, y + 9, LARGURA_BARRA_LISTA, 11,
                              fracaoDoAtributo(ficha, menu->criterio), ColorLerp(COR_CROMO, COR_AMARELO, suave));
        escreverADireita(FONTE_FORTE, textoDoAtributo(ficha, menu->criterio),
                         X_LISTA + LARGURA_LISTA - 14, y + 5, 15.0f, COR_TEXTO_CLARO);
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
    desenharCarroParado(numero, centroX, topo + 176, 1.22f);

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
 * Preparar a corrida: pista e dificuldade, na mesma tela
 * ================================================================== */

/* As linhas da tela */
enum { FOCO_NA_PISTA, FOCO_NO_NIVEL, FOCO_NO_CORRER, TOTAL_FOCOS };

#define Y_CARTAO_PISTA        112.0f
#define LARGURA_CARTAO_PISTA  350.0f
#define ALTURA_CARTAO_PISTA   190.0f
#define ESPACO_ENTRE_CARTOES  20.0f
#define LADO_DO_TRACADO       112.0f
#define PASSO_DO_TRACADO      4        /* desenha o traçado de 4 em 4 segmentos */
#define Y_TITULO_PISTA        90.0f
#define Y_TITULO_NIVEL        316.0f
#define Y_BOTAO_NIVEL         338.0f
#define LARGURA_BOTAO_NIVEL   224.0f
#define ALTURA_BOTAO_NIVEL    52.0f
#define ESPACO_ENTRE_NIVEIS   24.0f
#define Y_RESUMO              404.0f
#define Y_BOTAO_CORRER        452.0f
#define FOLGA_DO_CURSOR       5.0f     /* o contorno amarelo fica um pouco para fora do item */
#define GROSSURA_DO_CURSOR    3.0f
#define RAPIDEZ_DO_CURSOR     5.0f     /* a pulsação dele */
#define X_SETA_ESQUERDA       20.0f    /* as setas da linha marcada: ela muda para os lados */
#define X_SETA_DIREITA        780.0f

/*
 * desenharTracado
 * A planta da pista, a partir da posição de cada segmento no mundo: mede o
 * tamanho do desenho e o escala para caber no quadrado.
 * Recebe: a pista, o centro do quadrado, o lado dele e a cor.
 * Devolve: nada.
 */
static void desenharTracado(Pista *pista, float centroX, float centroY, float lado, Color cor) {
    float minX = 1e9f, maxX = -1e9f, minZ = 1e9f, maxZ = -1e9f;
    Segmento *atual = pista->largada;
    do {
        minX = fminf(minX, atual->xMundo);
        maxX = fmaxf(maxX, atual->xMundo);
        minZ = fminf(minZ, atual->zMundo);
        maxZ = fmaxf(maxZ, atual->zMundo);
        atual = atual->proximo;
    } while (atual != pista->largada);

    float escala = lado / fmaxf(maxX - minX, maxZ - minZ);
    float meioX = (minX + maxX) / 2.0f, meioZ = (minZ + maxZ) / 2.0f;
    Vector2 inicio = { centroX + (atual->xMundo - meioX) * escala, centroY + (atual->zMundo - meioZ) * escala };
    Vector2 ponto = inicio;
    int passo = 0;
    do {
        atual = atual->proximo;
        if (++passo % PASSO_DO_TRACADO == 0 || atual == pista->largada) {
            Vector2 seguinte = { centroX + (atual->xMundo - meioX) * escala, centroY + (atual->zMundo - meioZ) * escala };
            DrawLineEx(ponto, seguinte, 4.0f, cor);
            ponto = seguinte;
        }
    } while (atual != pista->largada);

    /* A linha de chegada: um pontinho vermelho */
    DrawCircleV(inicio, 5.0f, COR_VERMELHO);
}

/*
 * xDoCartao
 * Recebe: o número do cartão.
 * Devolve: o x do canto esquerdo dele (os cartões ficam centralizados).
 */
static float xDoCartao(int cartao) {
    float total = TOTAL_PISTAS * LARGURA_CARTAO_PISTA + (TOTAL_PISTAS - 1) * ESPACO_ENTRE_CARTOES;
    return (LARGURA_TELA - total) / 2.0f + cartao * (LARGURA_CARTAO_PISTA + ESPACO_ENTRE_CARTOES);
}

/*
 * xDoNivel
 * Recebe: o nível.
 * Devolve: o x do canto esquerdo do botão dele.
 */
static float xDoNivel(int nivel) {
    /* O paralelogramo passa da largura pela inclinação: desconta metade, para o conjunto ficar no meio */
    float total = TOTAL_NIVEIS * LARGURA_BOTAO_NIVEL + (TOTAL_NIVEIS - 1) * ESPACO_ENTRE_NIVEIS;
    return (LARGURA_TELA - total) / 2.0f - INCLINACAO / 4.0f + nivel * (LARGURA_BOTAO_NIVEL + ESPACO_ENTRE_NIVEIS);
}

/*
 * cartaoSobOMouse
 * Recebe: a posição do mouse na tela virtual.
 * Devolve: o número do cartão sob o mouse, ou -1.
 */
static int cartaoSobOMouse(float mouseX, float mouseY) {
    for (int i = 0; i < TOTAL_PISTAS; i++) {
        if (dentroDoRetangulo(mouseX, mouseY, xDoCartao(i), Y_CARTAO_PISTA, LARGURA_CARTAO_PISTA, ALTURA_CARTAO_PISTA)) {
            return i;
        }
    }
    return -1;
}

/*
 * nivelSobOMouse
 * Recebe: a posição do mouse.
 * Devolve: o nível sob ele, ou -1.
 */
static int nivelSobOMouse(float mouseX, float mouseY) {
    for (int i = 0; i < TOTAL_NIVEIS; i++) {
        if (dentroDoRetangulo(mouseX, mouseY, xDoNivel(i), Y_BOTAO_NIVEL, LARGURA_BOTAO_NIVEL, ALTURA_BOTAO_NIVEL)) {
            return i;
        }
    }
    return -1;
}

/*
 * atualizarEscolhaDePista
 * Cima e baixo trocam de linha; esquerda e direita mudam a pista ou o nível. Confirmar
 * passa para a linha seguinte, e só no CORRER começa a corrida: ninguém larga sem querer.
 * Recebe: o estado do menu.
 * Devolve: a tela do próximo quadro.
 */
Tela atualizarEscolhaDePista(EstadoMenu *menu) {
    int pistaAntes = menu->pistaEscolhida, nivelAntes = menu->nivelEscolhido, focoAntes = menu->focoDaCorrida;
    menu->focoDaCorrida = andarNaLista(menu->focoDaCorrida, TOTAL_FOCOS);
    int mudou = apertouParaDireita() ? 1 : (apertouParaEsquerda() ? -1 : 0);
    if (menu->focoDaCorrida == FOCO_NA_PISTA && mudou != 0) {
        menu->pistaEscolhida = (menu->pistaEscolhida + mudou + TOTAL_PISTAS) % TOTAL_PISTAS;
    } else if (menu->focoDaCorrida == FOCO_NO_NIVEL && mudou != 0) {
        menu->nivelEscolhido = (NivelDaCorrida) ((menu->nivelEscolhido + mudou + TOTAL_NIVEIS) % TOTAL_NIVEIS);
    }

    /* O mouse: passar marca a linha; clicar escolhe; só o CORRER começa a corrida */
    float mouseX, mouseY;
    posicaoDoMouseNaInterface(&mouseX, &mouseY);
    int cartao = cartaoSobOMouse(mouseX, mouseY);
    int nivel = nivelSobOMouse(mouseX, mouseY);
    int sobreCorrer = opcaoSobOMouse(Y_BOTAO_CORRER, 1) == 0;
    int voltar = sobreOVoltar(mouseX, mouseY);
    if (mouseSeMexeu()) {
        menu->focoDaCorrida = (cartao >= 0) ? FOCO_NA_PISTA : (nivel >= 0) ? FOCO_NO_NIVEL : sobreCorrer ? FOCO_NO_CORRER : menu->focoDaCorrida;
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (cartao >= 0) {
            menu->pistaEscolhida = cartao;
            menu->focoDaCorrida = FOCO_NA_PISTA;
        } else if (nivel >= 0) {
            menu->nivelEscolhido = (NivelDaCorrida) nivel;
            menu->focoDaCorrida = FOCO_NO_NIVEL;
        }
    }
    if (menu->pistaEscolhida != pistaAntes || (int) menu->nivelEscolhido != nivelAntes || menu->focoDaCorrida != focoAntes
        || (voltar && !menu->voltarAnterior)) {
        tocarSom(SOM_CURSOR);
    }
    menu->voltarAnterior = voltar;

    for (int i = 0; i < TOTAL_PISTAS; i++) {
        menu->destaquePista[i] = animarDestaque(menu->destaquePista[i], i == menu->pistaEscolhida);
    }
    for (int i = 0; i < TOTAL_NIVEIS; i++) {
        menu->destaqueNivel[i] = animarDestaque(menu->destaqueNivel[i], i == (int) menu->nivelEscolhido);
    }
    menu->destaqueCorrer = animarDestaque(menu->destaqueCorrer, menu->focoDaCorrida == FOCO_NO_CORRER);
    menu->destaqueVoltar = animarDestaque(menu->destaqueVoltar, voltar);

    if (apertouVoltar() || (voltar && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) {
        tocarSom(SOM_VOLTAR);
        menu->focoDaCorrida = FOCO_NA_PISTA;   /* na próxima vez, começa pela primeira linha */
        return TELA_MENU;
    }
    if (sobreCorrer && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        tocarSom(SOM_CONFIRMAR);
        return TELA_CORRIDA;
    }
    if (apertouConfirmar()) {
        tocarSom(SOM_CONFIRMAR);
        if (menu->focoDaCorrida == FOCO_NO_CORRER) {
            return TELA_CORRIDA;
        }
        menu->focoDaCorrida++;
    }
    return TELA_PISTAS;
}

/*
 * corDoCursor
 * O contorno de "você está aqui" é sempre amarelo, e pulsa de leve, para chamar o olho.
 * Recebe: nada.
 * Devolve: a cor deste quadro.
 */
static Color corDoCursor(void) {
    return ColorLerp(COR_AMARELO, WHITE, 0.25f + 0.25f * sinf((float) GetTime() * RAPIDEZ_DO_CURSOR));
}

/*
 * desenharTituloDaLinha
 * Recebe: o texto, a altura e se o cursor está nesta linha.
 * Devolve: nada.
 */
static void desenharTituloDaLinha(const char *texto, float y, int marcada) {
    escreverCentralizado(FONTE_FORTE, texto, LARGURA_TELA / 2.0f, y, 16.0f, marcada ? COR_TEXTO_CLARO : COR_TEXTO_FRACO);
}

/*
 * desenharSetasDaLinha
 * As setas nas pontas da linha marcada: dizem que ela muda para os lados.
 * Recebe: o centro da linha, na altura.
 * Devolve: nada.
 */
static void desenharSetasDaLinha(float y) {
    desenharSetas(X_SETA_ESQUERDA, y, -1.0f);
    desenharSetas(X_SETA_DIREITA, y, 1.0f);
}

/*
 * desenharCartaoDePista
 * Só o que importa: o traçado, o nome e o tamanho. O vermelho diz qual está escolhida;
 * o contorno amarelo, que o cursor está aqui.
 * Recebe: o cartão, a pista, o destaque (0 a 1) e se o cursor está nele.
 * Devolve: nada.
 */
static void desenharCartaoDePista(int cartao, Pista *pista, float destaque, int comCursor) {
    float suave = suavizar(destaque);
    float x = xDoCartao(cartao);
    Rectangle cartaoInteiro = { x, Y_CARTAO_PISTA, LARGURA_CARTAO_PISTA, ALTURA_CARTAO_PISTA };
    DrawRectangleRec(cartaoInteiro, COR_PAINEL);
    DrawRectangleRec(cartaoInteiro, Fade(COR_VERMELHO, 0.28f * suave));
    DrawRectangleLinesEx(cartaoInteiro, 1.0f + suave, ColorLerp(Fade(COR_CROMO, 0.25f), COR_VERMELHO, suave));
    if (comCursor) {
        DrawRectangleLinesEx((Rectangle){ x - FOLGA_DO_CURSOR, Y_CARTAO_PISTA - FOLGA_DO_CURSOR, LARGURA_CARTAO_PISTA + FOLGA_DO_CURSOR * 2.0f,
                                          ALTURA_CARTAO_PISTA + FOLGA_DO_CURSOR * 2.0f }, GROSSURA_DO_CURSOR, corDoCursor());
    }

    float centroX = x + LARGURA_CARTAO_PISTA / 2.0f;
    desenharTracado(pista, centroX, Y_CARTAO_PISTA + 70.0f, LADO_DO_TRACADO, ColorLerp(COR_TEXTO_FRACO, COR_TEXTO_CLARO, suave));

    float metros = contarSegmentos(pista) * COMPRIMENTO_SEGMENTO / UNIDADES_POR_METRO;
    escreverCentralizado(FONTE_TITULO, pista->nome, centroX, Y_CARTAO_PISTA + 134.0f, 22.0f, ColorLerp(COR_TEXTO_FRACO, COR_TEXTO_CLARO, suave));
    escreverCentralizado(FONTE_FORTE, TextFormat("%.2f km", metros / 1000.0f), centroX, Y_CARTAO_PISTA + 162.0f, 16.0f, ColorLerp(COR_TEXTO_FRACO, COR_TEXTO_CLARO, suave));
}

/*
 * desenharBotaoDeNivel
 * Recebe: o nível, o destaque (0 a 1) e se o cursor está nele.
 * Devolve: nada.
 */
static void desenharBotaoDeNivel(int nivel, float destaque, int comCursor) {
    float suave = suavizar(destaque);
    float x = xDoNivel(nivel);
    desenharParalelogramo(x, Y_BOTAO_NIVEL, LARGURA_BOTAO_NIVEL, ALTURA_BOTAO_NIVEL, INCLINACAO / 2.0f, ColorLerp(COR_PAINEL, COR_VERMELHO, suave));
    if (suave < 1.0f) {
        desenharContornoParalelogramo(x, Y_BOTAO_NIVEL, LARGURA_BOTAO_NIVEL, ALTURA_BOTAO_NIVEL, INCLINACAO / 2.0f, 1.5f, Fade(COR_CROMO, 0.35f * (1.0f - suave)));
    }
    if (comCursor) {
        desenharContornoParalelogramo(x - FOLGA_DO_CURSOR, Y_BOTAO_NIVEL - FOLGA_DO_CURSOR, LARGURA_BOTAO_NIVEL + FOLGA_DO_CURSOR * 2.0f,
                                      ALTURA_BOTAO_NIVEL + FOLGA_DO_CURSOR * 2.0f, INCLINACAO / 2.0f, GROSSURA_DO_CURSOR, corDoCursor());
    }
    escreverCentralizado(FONTE_TITULO, fichaDoNivel((NivelDaCorrida) nivel)->nome, x + (LARGURA_BOTAO_NIVEL + INCLINACAO / 2.0f) / 2.0f, Y_BOTAO_NIVEL + 12.0f, 24.0f,
                         ColorLerp(COR_TEXTO_FRACO, COR_TEXTO_CLARO, suave));
}

/*
 * desenharBotaoCorrer
 * Sempre vermelho, por ser a ação principal; mais apagado até o cursor chegar nele.
 * Recebe: se o cursor está nele.
 * Devolve: nada.
 */
static void desenharBotaoCorrer(int comCursor) {
    float largura = LARGURA_OPCAO_MARCADA;
    float x = LARGURA_TELA / 2.0f - largura / 2.0f - INCLINACAO / 2.0f;
    desenharParalelogramo(x, Y_BOTAO_CORRER, largura, ALTURA_OPCAO, INCLINACAO, comCursor ? COR_VERMELHO : ColorLerp(COR_PAINEL, COR_VERMELHO, 0.45f));
    if (comCursor) {
        desenharContornoParalelogramo(x - FOLGA_DO_CURSOR, Y_BOTAO_CORRER - FOLGA_DO_CURSOR, largura + FOLGA_DO_CURSOR * 2.0f,
                                      ALTURA_OPCAO + FOLGA_DO_CURSOR * 2.0f, INCLINACAO, GROSSURA_DO_CURSOR, corDoCursor());
    }
    escreverCentralizado(FONTE_TITULO, "CORRER", LARGURA_TELA / 2.0f, Y_BOTAO_CORRER + 3.0f, 36.0f, comCursor ? COR_TEXTO_CLARO : Fade(COR_TEXTO_CLARO, 0.65f));
}

/*
 * desenharResumoDaCorrida
 * O que muda na corrida marcada, em uma linha.
 * Recebe: o estado do menu.
 * Devolve: nada.
 */
static void desenharResumoDaCorrida(EstadoMenu *menu) {
    const FichaDoNivel *ficha = fichaDoNivel(menu->nivelEscolhido);
    int temPit = (menu->pistaEscolhida == PISTA_INTERLAGOS);
    int parada = ficha->temParada && temPit;
    int gasta = ficha->gastaPneu && parada;
    const char *partes[3] = { TextFormat("%d VOLTAS", ficha->voltas), parada ? "PITSTOP OBRIGATÓRIO" : "SEM PITSTOP", "PNEUS GASTAM" };
    char voltas[32];
    snprintf(voltas, sizeof(voltas), "%s", partes[0]);   /* TextFormat reaproveita o próprio texto */
    partes[0] = voltas;
    int total = gasta ? 3 : 2;
    float tamanho = 18.0f, espaco = 34.0f, largura = (total - 1) * espaco;
    for (int i = 0; i < total; i++) {
        largura += larguraDoTexto(FONTE_FORTE, partes[i], tamanho);
    }
    float x = (LARGURA_TELA - largura) / 2.0f;
    for (int i = 0; i < total; i++) {
        escrever(FONTE_FORTE, partes[i], x, Y_RESUMO, tamanho, (i == 2) ? COR_AMARELO : COR_TEXTO_CLARO);
        x += larguraDoTexto(FONTE_FORTE, partes[i], tamanho);
        if (i + 1 < total) {
            desenharParalelogramo(x + espaco / 2.0f - 4.0f, Y_RESUMO + 7.0f, 8.0f, 8.0f, 3.0f, COR_VERMELHO);
            x += espaco;
        }
    }
}

/*
 * desenharEscolhaDePista
 * Recebe: o estado do menu e as pistas.
 * Devolve: nada.
 */
void desenharEscolhaDePista(EstadoMenu *menu, Pista *pistas[]) {
    escurecerTela(0.7f, 0.55f, 0.75f);
    desenharCabecalho("CORRIDA");
    desenharBotaoVoltar(menu->destaqueVoltar);

    int foco = menu->focoDaCorrida;
    desenharTituloDaLinha("PISTA", Y_TITULO_PISTA, foco == FOCO_NA_PISTA);
    for (int i = 0; i < TOTAL_PISTAS; i++) {
        desenharCartaoDePista(i, pistas[i], menu->destaquePista[i], foco == FOCO_NA_PISTA && i == menu->pistaEscolhida);
    }
    desenharTituloDaLinha("DIFICULDADE", Y_TITULO_NIVEL, foco == FOCO_NO_NIVEL);
    for (int i = 0; i < TOTAL_NIVEIS; i++) {
        desenharBotaoDeNivel(i, menu->destaqueNivel[i], foco == FOCO_NO_NIVEL && i == (int) menu->nivelEscolhido);
    }
    desenharResumoDaCorrida(menu);
    desenharBotaoCorrer(foco == FOCO_NO_CORRER);

    if (foco == FOCO_NA_PISTA) {
        desenharSetasDaLinha(Y_CARTAO_PISTA + ALTURA_CARTAO_PISTA / 2.0f);
    } else if (foco == FOCO_NO_NIVEL) {
        desenharSetasDaLinha(Y_BOTAO_NIVEL + ALTURA_BOTAO_NIVEL / 2.0f);
    }

    /* As dicas dizem o que cada botão faz na linha em que o cursor está */
    if (foco == FOCO_NO_CORRER) {
        Dica dicas[] = {
            { { "CIMA", NULL }, "SUBIR" },
            { { "ENTER", NULL }, "CORRER" },
            { { "ESC", NULL }, "VOLTAR" },
        };
        desenharBarraDeDicas(dicas, 3);
    } else {
        Dica dicas[] = {
            { { "ESQ", "DIR", NULL }, (foco == FOCO_NA_PISTA) ? "TROCAR A PISTA" : "TROCAR A DIFICULDADE" },
            { { "ENTER", NULL }, "PRÓXIMO" },
            { { "ESC", NULL }, "VOLTAR" },
        };
        desenharBarraDeDicas(dicas, 3);
    }
}

/* ==================================================================
 * Configurações
 * ================================================================== */

/* As linhas da tela, depois das 6 de comandos */
enum {
    LINHA_VIBRACAO = TOTAL_ACOES,
    LINHA_FORCA,
    LINHA_GATILHOS,
    LINHA_ZONA_MORTA,
    LINHA_VOLANTE,
    LINHA_CAMERA_X,
    LINHA_CAMERA_Y,
    LINHA_PADRAO,
    TOTAL_LINHAS_CONFIG
};

#define Y_TITULO_COMANDOS     92.0f
#define Y_COMANDOS            114.0f
#define ALTURA_LINHA_COMANDO  29.0f
#define Y_TITULO_CONTROLE     294.0f
#define Y_OPCOES_CONTROLE     316.0f
#define ALTURA_LINHA_OPCAO    26.0f
#define X_LINHAS_CONFIG       24.0f
#define LARGURA_LINHAS_CONFIG 752.0f
#define X_COLUNAS_COMANDO     212.0f
#define LARGURA_COLUNA_COMANDO 138.0f
#define X_VALOR_OPCAO         560.0f
#define LARGURA_VALOR_OPCAO   190.0f
#define SEGUNDOS_ESPERANDO    6.0f    /* sem apertar nada, desiste de trocar */
#define SEGUNDOS_PARA_CONFIRMAR 3.0f
#define SEGUNDOS_DO_TOQUE     0.3f

/*
 * yDaLinhaConfig
 * Recebe: a linha.
 * Devolve: o topo dela na tela.
 */
static float yDaLinhaConfig(int linha) {
    if (linha < TOTAL_ACOES) {
        return Y_COMANDOS + linha * ALTURA_LINHA_COMANDO;
    }
    return Y_OPCOES_CONTROLE + (linha - TOTAL_ACOES) * ALTURA_LINHA_OPCAO;
}

/*
 * alturaDaLinhaConfig
 * Recebe: a linha.
 * Devolve: a altura dela.
 */
static float alturaDaLinhaConfig(int linha) {
    return (linha < TOTAL_ACOES) ? ALTURA_LINHA_COMANDO : ALTURA_LINHA_OPCAO;
}

/*
 * guardarConfiguracoes
 * Salva a cada mudança: fechar o jogo não perde nada.
 * Recebe: nada.
 * Devolve: nada.
 */
static void guardarConfiguracoes(void) {
    salvarConfiguracoes(configuracoesAtuais(), ARQUIVO_DE_CONFIGURACOES);
}

/*
 * mudarOpcao
 * Muda o valor de uma opção do controle, nos dois sentidos.
 * Recebe: o estado do menu, a linha e o sentido (+1 ou -1).
 * Devolve: nada.
 */
static void mudarOpcao(EstadoMenu *menu, int linha, int sentido) {
    Configuracoes *config = configuracoesAtuais();
    int vibrou = 0;
    switch (linha) {
        case LINHA_VIBRACAO:
            config->vibracaoLigada = !config->vibracaoLigada;
            vibrou = 1;
            break;
        case LINHA_FORCA:
            config->forcaDaVibracao += sentido * PASSO_DA_FORCA;
            if (config->forcaDaVibracao > 100) {
                config->forcaDaVibracao = MENOR_FORCA_DA_VIBRACAO;
            } else if (config->forcaDaVibracao < MENOR_FORCA_DA_VIBRACAO) {
                config->forcaDaVibracao = 100;
            }
            vibrou = 1;
            break;
        case LINHA_GATILHOS:
            config->gatilhosAdaptativos = !config->gatilhosAdaptativos;
            break;
        case LINHA_ZONA_MORTA:
            config->zonaMorta += sentido * PASSO_DA_ZONA_MORTA;
            if (config->zonaMorta > MAIOR_ZONA_MORTA) {
                config->zonaMorta = MENOR_ZONA_MORTA;
            } else if (config->zonaMorta < MENOR_ZONA_MORTA) {
                config->zonaMorta = MAIOR_ZONA_MORTA;
            }
            break;
        case LINHA_VOLANTE:
            config->respostaDoVolante = (config->respostaDoVolante + sentido + TOTAL_VOLANTES) % TOTAL_VOLANTES;
            break;
        case LINHA_CAMERA_X:
            config->inverterCameraX = !config->inverterCameraX;
            break;
        case LINHA_CAMERA_Y:
            config->inverterCameraY = !config->inverterCameraY;
            break;
        default:
            return;
    }
    if (vibrou) {
        menu->pulsoDeVibracao = SEGUNDOS_DO_TOQUE;
    }
    tocarSom(SOM_CURSOR);
    guardarConfiguracoes();
}

/*
 * esperarComando
 * Esperando a tecla ou o botão novo. ESC ou START desistem; se ele não apertar nada, a
 * espera acaba sozinha.
 * Recebe: o estado do menu.
 * Devolve: nada.
 */
static void esperarComando(EstadoMenu *menu) {
    AcaoDeCorrida acao = (AcaoDeCorrida) menu->linhaConfig;
    ColunaDeComando coluna = (ColunaDeComando) menu->colunaConfig;
    int ehBotao = (coluna >= COLUNA_BOTAO_1);
    menu->tempoEsperando += GetFrameTime();
    if (IsKeyPressed(KEY_ESCAPE) || controleApertou(BOTAO_START) || menu->tempoEsperando > SEGUNDOS_ESPERANDO
        || (ehBotao && !controleConectado())) {
        menu->esperandoComando = 0;
        tocarSom(SOM_VOLTAR);
        return;
    }
    int codigo = ehBotao ? botaoApertadoAgora() : teclaApertadaAgora();
    if (codigo != SEM_COMANDO && codigo != KEY_ESCAPE) {
        definirComando(configuracoesAtuais(), acao, coluna, codigo);
        guardarConfiguracoes();
        menu->esperandoComando = 0;
        tocarSom(SOM_CONFIRMAR);
    }
}

/*
 * colunaSobOMouse
 * Recebe: a posição do mouse.
 * Devolve: a coluna de comando sob ele, ou -1.
 */
static int colunaSobOMouse(float mouseX) {
    if (mouseX < X_COLUNAS_COMANDO || mouseX >= X_COLUNAS_COMANDO + TOTAL_COLUNAS * LARGURA_COLUNA_COMANDO) {
        return -1;
    }
    return (int) ((mouseX - X_COLUNAS_COMANDO) / LARGURA_COLUNA_COMANDO);
}

/*
 * apertarLinhaConfig
 * Confirmou na linha marcada: troca o comando, muda a opção, ou volta ao padrão.
 * Recebe: o estado do menu.
 * Devolve: nada.
 */
static void apertarLinhaConfig(EstadoMenu *menu) {
    if (menu->linhaConfig < TOTAL_ACOES) {
        if (menu->colunaConfig >= COLUNA_BOTAO_1 && !controleConectado()) {
            tocarSom(SOM_VOLTAR);   /* sem controle, não há como apertar o botão novo */
            return;
        }
        menu->esperandoComando = 1;
        menu->tempoEsperando = 0.0f;
        while (GetKeyPressed() != 0) {
            /* a tecla que abriu a espera não conta */
        }
        tocarSom(SOM_CONFIRMAR);
    } else if (menu->linhaConfig == LINHA_PADRAO) {
        if (menu->confirmarPadrao > 0.0f) {
            configuracoesPadrao(configuracoesAtuais());
            guardarConfiguracoes();
            menu->confirmarPadrao = 0.0f;
            menu->pulsoDeVibracao = SEGUNDOS_DO_TOQUE;
            tocarSom(SOM_CONFIRMAR);
        } else {
            menu->confirmarPadrao = SEGUNDOS_PARA_CONFIRMAR;
            tocarSom(SOM_CURSOR);
        }
    } else {
        mudarOpcao(menu, menu->linhaConfig, 1);
    }
}

/*
 * atualizarConfiguracoes
 * Recebe: o estado do menu.
 * Devolve: a tela do próximo quadro.
 */
Tela atualizarConfiguracoes(EstadoMenu *menu) {
    if (menu->esperandoComando) {
        esperarComando(menu);
        return TELA_CONFIGURACOES;
    }
    menu->confirmarPadrao = fmaxf(menu->confirmarPadrao - GetFrameTime(), 0.0f);

    int anterior = menu->linhaConfig, colunaAnterior = menu->colunaConfig;
    menu->linhaConfig = andarNaLista(menu->linhaConfig, TOTAL_LINHAS_CONFIG);
    if (menu->linhaConfig < TOTAL_ACOES) {
        if (apertouParaDireita()) {
            menu->colunaConfig = (menu->colunaConfig + 1) % TOTAL_COLUNAS;
        }
        if (apertouParaEsquerda()) {
            menu->colunaConfig = (menu->colunaConfig + TOTAL_COLUNAS - 1) % TOTAL_COLUNAS;
        }
    } else if (apertouParaDireita()) {
        mudarOpcao(menu, menu->linhaConfig, 1);
    } else if (apertouParaEsquerda()) {
        mudarOpcao(menu, menu->linhaConfig, -1);
    }

    /* O mouse: passar marca, e clicar faz o mesmo que confirmar */
    float mouseX, mouseY;
    posicaoDoMouseNaInterface(&mouseX, &mouseY);
    int voltar = sobreOVoltar(mouseX, mouseY);
    int sobre = -1;
    for (int i = 0; i < TOTAL_LINHAS_CONFIG; i++) {
        if (dentroDoRetangulo(mouseX, mouseY, X_LINHAS_CONFIG, yDaLinhaConfig(i), LARGURA_LINHAS_CONFIG, alturaDaLinhaConfig(i))) {
            sobre = i;
        }
    }
    int sobreColuna = colunaSobOMouse(mouseX);
    if (sobre >= 0 && mouseSeMexeu()) {
        menu->linhaConfig = sobre;
        if (sobre < TOTAL_ACOES && sobreColuna >= 0) {
            menu->colunaConfig = sobreColuna;
        }
    }
    if (menu->linhaConfig != anterior || menu->colunaConfig != colunaAnterior || (voltar && !menu->voltarAnterior)) {
        tocarSom(SOM_CURSOR);
    }
    if (menu->linhaConfig != anterior) {
        menu->confirmarPadrao = 0.0f;
    }
    menu->voltarAnterior = voltar;
    menu->destaqueVoltar = animarDestaque(menu->destaqueVoltar, voltar);

    if (apertouVoltar() || (voltar && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) {
        tocarSom(SOM_VOLTAR);
        return TELA_MENU;
    }

    /* Tirar uma tecla do comando: não vale para a última que ele tem */
    if (menu->linhaConfig < TOTAL_ACOES && (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_DELETE) || controleApertou(BOTAO_LIMPAR))) {
        if (limparComando(configuracoesAtuais(), (AcaoDeCorrida) menu->linhaConfig, (ColunaDeComando) menu->colunaConfig)) {
            guardarConfiguracoes();
            tocarSom(SOM_VOLTAR);
        }
    }

    if (apertouConfirmar() || (sobre >= 0 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && (sobre >= TOTAL_ACOES || sobreColuna >= 0))) {
        apertarLinhaConfig(menu);
    }
    return TELA_CONFIGURACOES;
}

/*
 * desenharTituloDeSecao
 * Recebe: o texto e a altura.
 * Devolve: nada.
 */
static void desenharTituloDeSecao(const char *texto, float y) {
    escrever(FONTE_TITULO, texto, X_LINHAS_CONFIG + 10.0f, y, 17.0f, COR_AMARELO);
}

/*
 * desenharFundoDaLinhaConfig
 * Recebe: a linha e se ela está marcada.
 * Devolve: nada.
 */
static void desenharFundoDaLinhaConfig(int linha, int marcada) {
    float y = yDaLinhaConfig(linha), altura = alturaDaLinhaConfig(linha);
    DrawRectangleRec((Rectangle){ X_LINHAS_CONFIG, y, LARGURA_LINHAS_CONFIG, altura - 2.0f },
                     marcada ? Fade(COR_CROMO, 0.14f) : Fade(COR_GRAFITE, 0.78f));
    if (marcada && linha >= TOTAL_ACOES) {
        DrawRectangleLinesEx((Rectangle){ X_LINHAS_CONFIG, y, LARGURA_LINHAS_CONFIG, altura - 2.0f }, 2.0f, corDoCursor());
    }
}

/*
 * desenharLinhaDeComando
 * Um comando e as suas 4 colunas.
 * Recebe: o estado do menu e a linha.
 * Devolve: nada.
 */
static void desenharLinhaDeComando(EstadoMenu *menu, int linha) {
    const Configuracoes *config = configuracoesAtuais();
    int marcada = (linha == menu->linhaConfig);
    float y = yDaLinhaConfig(linha);
    desenharFundoDaLinhaConfig(linha, marcada);
    escrever(FONTE_FORTE, nomeDaAcao((AcaoDeCorrida) linha), X_LINHAS_CONFIG + 12.0f, y + 5.0f, 15.0f, marcada ? COR_TEXTO_CLARO : COR_CROMO);

    for (int c = 0; c < TOTAL_COLUNAS; c++) {
        float x = X_COLUNAS_COMANDO + c * LARGURA_COLUNA_COMANDO;
        int codigo = config->comandos[linha][c];
        int aqui = marcada && c == menu->colunaConfig;
        if (aqui) {
            DrawRectangleLinesEx((Rectangle){ x + 2.0f, y + 1.0f, LARGURA_COLUNA_COMANDO - 4.0f, ALTURA_LINHA_COMANDO - 4.0f }, 2.0f, corDoCursor());
        }
        if (aqui && menu->esperandoComando) {
            const char *pedido = (c >= COLUNA_BOTAO_1) ? "APERTE UM BOTÃO" : "APERTE UMA TECLA";
            escreverCentralizado(FONTE_FORTE, pedido, x + LARGURA_COLUNA_COMANDO / 2.0f, y + 6.0f, 12.0f, COR_AMARELO);
        } else if (codigo == SEM_COMANDO) {
            escreverCentralizado(FONTE_FORTE, "---", x + LARGURA_COLUNA_COMANDO / 2.0f, y + 5.0f, 15.0f, COR_TEXTO_FRACO);
        } else {
            const char *nome = (c >= COLUNA_BOTAO_1) ? nomeDoBotao(codigo) : nomeDaTecla(codigo);
            desenharTecla(x + (LARGURA_COLUNA_COMANDO - larguraDaTecla(nome)) / 2.0f, y + 2.0f, nome);
        }
    }
}

/*
 * desenharLinhaDeOpcao
 * Recebe: o estado do menu, a linha, o nome, o valor e se ela está apagada (depende de outra).
 * Devolve: nada.
 */
static void desenharLinhaDeOpcao(EstadoMenu *menu, int linha, const char *nome, const char *valor, int apagada) {
    int marcada = (linha == menu->linhaConfig);
    float y = yDaLinhaConfig(linha);
    Color corDoTexto = apagada ? COR_TEXTO_FRACO : (marcada ? COR_TEXTO_CLARO : COR_CROMO);
    desenharFundoDaLinhaConfig(linha, marcada);
    escrever(FONTE_FORTE, nome, X_LINHAS_CONFIG + 12.0f, y + 3.0f, 15.0f, corDoTexto);
    escreverCentralizado(FONTE_FORTE, valor, X_VALOR_OPCAO + LARGURA_VALOR_OPCAO / 2.0f, y + 3.0f, 15.0f,
                         apagada ? COR_TEXTO_FRACO : COR_AMARELO);
    if (marcada && linha != LINHA_PADRAO) {
        escrever(FONTE_FORTE, "<", X_VALOR_OPCAO, y + 3.0f, 15.0f, COR_AMARELO);
        escreverADireita(FONTE_FORTE, ">", X_VALOR_OPCAO + LARGURA_VALOR_OPCAO, y + 3.0f, 15.0f, COR_AMARELO);
    }
}

/*
 * desenharConfiguracoes
 * Recebe: o estado do menu.
 * Devolve: nada.
 */
void desenharConfiguracoes(EstadoMenu *menu) {
    static const char *VOLANTES[TOTAL_VOLANTES] = { "SUAVE", "NORMAL", "DIRETO" };
    const Configuracoes *config = configuracoesAtuais();
    escurecerTela(0.75f, 0.6f, 0.8f);
    desenharCabecalho("CONFIGURAÇÕES");
    desenharBotaoVoltar(menu->destaqueVoltar);

    /* Comandos */
    desenharTituloDeSecao("COMANDOS", Y_TITULO_COMANDOS);
    static const char *COLUNAS[TOTAL_COLUNAS] = { "TECLA", "TECLA 2", "CONTROLE", "CONTROLE 2" };
    for (int c = 0; c < TOTAL_COLUNAS; c++) {
        escreverCentralizado(FONTE_FORTE, COLUNAS[c], X_COLUNAS_COMANDO + (c + 0.5f) * LARGURA_COLUNA_COMANDO, Y_TITULO_COMANDOS + 2.0f, 13.0f, COR_TEXTO_FRACO);
    }
    for (int i = 0; i < TOTAL_ACOES; i++) {
        desenharLinhaDeComando(menu, i);
    }

    /* Controle */
    int vibra = config->vibracaoLigada;
    desenharTituloDeSecao("CONTROLE", Y_TITULO_CONTROLE);
    desenharLinhaDeOpcao(menu, LINHA_VIBRACAO, "VIBRAÇÃO", vibra ? "LIGADA" : "DESLIGADA", 0);
    desenharLinhaDeOpcao(menu, LINHA_FORCA, "FORÇA DA VIBRAÇÃO", TextFormat("%d%%", config->forcaDaVibracao), !vibra);
    desenharLinhaDeOpcao(menu, LINHA_GATILHOS, "TREMOR NO ACELERADOR (DUALSENSE)", config->gatilhosAdaptativos ? "LIGADO" : "DESLIGADO", !vibra);
    desenharLinhaDeOpcao(menu, LINHA_ZONA_MORTA, "ZONA MORTA DO ANALÓGICO", TextFormat("%d%%", config->zonaMorta), 0);
    desenharLinhaDeOpcao(menu, LINHA_VOLANTE, "RESPOSTA DO VOLANTE NO ANALÓGICO", VOLANTES[config->respostaDoVolante], 0);
    desenharLinhaDeOpcao(menu, LINHA_CAMERA_X, "INVERTER A CÂMERA NA HORIZONTAL", config->inverterCameraX ? "SIM" : "NÃO", 0);
    desenharLinhaDeOpcao(menu, LINHA_CAMERA_Y, "INVERTER A CÂMERA NA VERTICAL", config->inverterCameraY ? "SIM" : "NÃO", 0);
    desenharLinhaDeOpcao(menu, LINHA_PADRAO, "VOLTAR TUDO AO PADRÃO", (menu->confirmarPadrao > 0.0f) ? "APERTE DE NOVO" : "", 0);

    if (menu->esperandoComando) {
        /* Aqui o botão de voltar seria gravado como o botão novo: quem desiste é o da pausa */
        Dica esperando[] = { { { controleConectado() ? nomeDoBotao(GAMEPAD_BUTTON_MIDDLE_RIGHT) : "TECLA ESC", NULL }, "DESISTIR" } };
        desenharBarraDeDicas(esperando, 1);
    } else if (menu->linhaConfig < TOTAL_ACOES) {
        Dica dicas[] = {
            { { "ENTER", NULL }, "TROCAR" },
            { { controleConectado() ? nomeDoBotao(GAMEPAD_BUTTON_RIGHT_FACE_UP) : "APAGAR", NULL }, "LIMPAR" },
            { { "ESC", NULL }, "VOLTAR" },
        };
        desenharBarraDeDicas(dicas, 3);
    } else if (menu->linhaConfig == LINHA_PADRAO) {
        Dica dicas[] = {
            { { "ENTER", NULL }, "VOLTAR AO PADRÃO" },
            { { "ESC", NULL }, "VOLTAR" },
        };
        desenharBarraDeDicas(dicas, 2);
    } else {
        Dica dicas[] = {
            { { "ESQ", "DIR", NULL }, "MUDAR" },
            { { "ESC", NULL }, "VOLTAR" },
        };
        desenharBarraDeDicas(dicas, 2);
    }
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
        tocarSom(SOM_VOLTAR);
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
 * Controles (com as teclas desenhadas, ou os botões, se houver um
 * controle ligado) e dicas de pilotagem.
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

    /* O que está nas configurações: de cada comando, as teclas, ou os botões com um controle ligado */
    const Configuracoes *config = configuracoesAtuais();
    int ligado = controleConectado();
    int primeira = ligado ? COLUNA_BOTAO_1 : COLUNA_TECLA_1;
    static const AcaoDeCorrida LINHAS[][2] = {
        { ACAO_ACELERAR, ACAO_ACELERAR }, { ACAO_FREAR, ACAO_FREAR }, { ACAO_ESQUERDA, ACAO_DIREITA },
        { ACAO_NITRO, ACAO_NITRO }, { ACAO_FREIO_DE_MAO, ACAO_FREIO_DE_MAO },
    };
    const char *acoes[] = { "ACELERAR", "FREAR", "VIRAR", "NITRO", "FREIO DE MÃO", "PAUSAR A CORRIDA", "CONFIRMAR NOS MENUS", "OLHAR EM VOLTA" };
    int linhas = ligado ? 8 : 7;   /* a câmera livre só existe no controle */
    for (int i = 0; i < linhas; i++) {
        float y = topo + 72 + i * (ligado ? 45 : 51);
        float xTecla = x + 20;
        if (i < 5) {
            /* Virar mostra os dois sentidos; os outros, as duas teclas do comando */
            if (ligado && LINHAS[i][0] == ACAO_ESQUERDA) {
                xTecla += desenharTecla(xTecla, y, nomeNoControle("ANALOG E")) + 5;
            }
            for (int lado = 0; lado < ((LINHAS[i][0] == LINHAS[i][1]) ? 1 : 2); lado++) {
                for (int c = primeira; c < primeira + ((LINHAS[i][0] == LINHAS[i][1]) ? 2 : 1); c++) {
                    int codigo = config->comandos[LINHAS[i][lado]][c];
                    if (codigo != SEM_COMANDO) {
                        const char *nome = ligado ? nomeDoBotao(codigo) : nomeDaTecla(codigo);
                        xTecla += desenharTecla(xTecla, y, nome) + 5;
                    }
                }
            }
        } else {
            const char *fixas[] = { ligado ? nomeDoBotao(GAMEPAD_BUTTON_MIDDLE_RIGHT) : "ESC", ligado ? nomeNoControle("ENTER") : "ENTER", nomeNoControle("ANALOG D") };
            xTecla += desenharTecla(xTecla, y, fixas[i - 5]) + 5;
        }
        escrever(FONTE_TITULO, acoes[i], x + 150, y - 2, 20.0f, COR_TEXTO_CLARO);
        if (i < 5) {
            DrawRectangleRec((Rectangle){ x + 20, y + 40, 312, 1 }, Fade(COR_CROMO, 0.15f));
        }
    }

    /* ---- Dicas ---- */
    x = 392.0f;
    desenharPainel(x, topo, 384, 444);
    desenharTituloDePainel("DICAS", x + 20, topo + 14);

    const char *dicas[] = {
        TextFormat("Você larga em último, contra %d rivais. O nível muda as voltas e os rivais; no difícil, os pneus gastam.", TOTAL_RIVAIS),
        "Em Interlagos, no médio e no difícil, a parada é obrigatória: saia pela esquerda na Subida dos Boxes. Quem não para é desclassificado.",
        "Freio de mão com o volante virado: a traseira solta e o carro entra em drift. Vira mais, mas perde velocidade e gasta pneu.",
        "Freie antes das curvas e alivie no meio: acelerando, o carro sai de frente. Fumaça = pneu no limite; passar dele custa velocidade.",
        "Na grama (sai poeira) o carro não passa de 75 km/h.",
        "Colado atrás de alguém você pega vácuo. Os carros são sólidos: batendo na quina ou de lado, quem toma a batida roda.",
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
 * Pausa
 * ================================================================== */

/*
 * abrirPausa
 * Prepara a pausa: "CONTINUAR" já vem marcado.
 * Recebe: o estado do menu.
 * Devolve: nada.
 */
void abrirPausa(EstadoMenu *menu) {
    menu->opcaoPausa = 0;
    for (int i = 0; i < TOTAL_OPCOES_PAUSA; i++) {
        menu->destaquePausa[i] = (i == 0) ? 1.0f : 0.0f;
    }
}

/*
 * atualizarPausa
 * Cima/baixo ou o mouse trocam a opção; ENTER (A) ou o clique escolhem.
 * ESC (B ou START no controle) fecha a pausa e continua a corrida.
 * Recebe: o estado do menu.
 * Devolve: o que o jogador escolheu (PAUSA_NADA enquanto não escolhe).
 */
EscolhaDaPausa atualizarPausa(EstadoMenu *menu) {
    int opcaoAnterior = menu->opcaoPausa;
    menu->opcaoPausa = andarNaLista(menu->opcaoPausa, TOTAL_OPCOES_PAUSA);

    int sobre = opcaoSobOMouse(Y_PRIMEIRA_OPCAO_PAUSA, TOTAL_OPCOES_PAUSA);
    if (sobre >= 0 && mouseSeMexeu()) {
        menu->opcaoPausa = sobre;
    }
    if (menu->opcaoPausa != opcaoAnterior) {
        tocarSom(SOM_CURSOR);
    }
    for (int i = 0; i < TOTAL_OPCOES_PAUSA; i++) {
        menu->destaquePausa[i] = animarDestaque(menu->destaquePausa[i], i == menu->opcaoPausa);
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && sobre >= 0) {
        tocarSom(SOM_CONFIRMAR);
        return ESCOLHAS_PAUSA[sobre];
    }
    if (apertouConfirmar()) {
        tocarSom(SOM_CONFIRMAR);
        return ESCOLHAS_PAUSA[menu->opcaoPausa];
    }
    if (apertouVoltar() || controleApertou(BOTAO_START)) {
        tocarSom(SOM_VOLTAR);
        return PAUSA_CONTINUAR;
    }
    return PAUSA_NADA;
}

/*
 * desenharPausa
 * Escurece a corrida parada e mostra "PAUSA" com os dois botões, no
 * mesmo estilo do menu principal. A corrida (congelada) é desenhada
 * antes, pelo main.c.
 * Recebe: o estado do menu.
 * Devolve: nada.
 */
void desenharPausa(EstadoMenu *menu) {
    escurecerTela(0.75f, 0.6f, 0.8f);

    float centro = LARGURA_TELA / 2.0f;
    float tamanho = 64.0f;
    float meio = larguraDoTexto(FONTE_TITULO, "PAUSA", tamanho) / 2.0f;
    escreverCentralizado(FONTE_TITULO, "PAUSA", centro, Y_TITULO_PAUSA, tamanho, COR_TEXTO_CLARO);
    desenharQuadriculado(centro - meio - 96, Y_TITULO_PAUSA + 32, 76.0f, 10.0f, 5.0f);
    desenharQuadriculado(centro + meio + 20, Y_TITULO_PAUSA + 32, 76.0f, 10.0f, 5.0f);

    for (int i = 0; i < TOTAL_OPCOES_PAUSA; i++) {
        desenharOpcao(TEXTOS_PAUSA[i], Y_PRIMEIRA_OPCAO_PAUSA + i * ESPACO_OPCOES, menu->destaquePausa[i]);
    }

    Dica dicas[] = {
        { { "CIMA", "BAIXO", NULL }, "ESCOLHER" },
        { { "ENTER", NULL }, "CONFIRMAR" },
        { { "ESC", NULL }, "CONTINUAR" },
    };
    desenharBarraDeDicas(dicas, 3);
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
        tocarSom(SOM_CONFIRMAR);
        return TELA_CORRIDA;
    }
    if (apertouVoltar()) {
        tocarSom(SOM_VOLTAR);
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
    for (int i = 0; i < carro->voltas && i < carro->voltasDaProva; i++) {
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
    desenharParalelogramo(X_TABELA + 12, y + 5, 38, 20, 5, podio);
    escreverCentralizado(FONTE_TITULO, TextFormat("%dº", posicao + 1), X_TABELA + 33, y + 4, 16.0f,
                         posicao < 3 ? COR_GRAFITE : COR_CROMO);

    escrever(FONTE_TITULO, carro->nome, X_COLUNA_PILOTO, y + 5, 18.0f, COR_TEXTO_CLARO);
    if (ehJogador) {
        float x = X_COLUNA_PILOTO + larguraDoTexto(FONTE_TITULO, carro->nome, 18.0f) + 10;
        desenharParalelogramo(x, y + 7, 42, 16, 4, COR_AMARELO);
        escreverCentralizado(FONTE_FORTE, "VOCÊ", x + 23, y + 7, 12.0f, COR_GRAFITE);
    }

    Color corTempo = ehJogador ? COR_TEXTO_CLARO : COR_CROMO;
    if (carro->voltas >= carro->voltasDaProva) {
        formatarTempo(carro->tempoDeChegada, tempo, sizeof(tempo));
        escrever(FONTE_FORTE, tempo, X_COLUNA_TEMPO, y + 6, 16.0f, corTempo);
    } else {
        escrever(FONTE_FORTE, TextFormat("VOLTA %d/%d", voltaAtual(carro), carro->voltasDaProva),
                 X_COLUNA_TEMPO, y + 6, 16.0f, COR_TEXTO_FRACO);
    }

    float melhor = melhorVolta(carro);
    if (melhor > 0.0f) {
        formatarTempo(melhor, tempo, sizeof(tempo));
        escrever(FONTE_FORTE, tempo, X_COLUNA_MELHOR, y + 6, 16.0f, corTempo);
    } else {
        escrever(FONTE_FORTE, "-", X_COLUNA_MELHOR, y + 6, 16.0f, COR_TEXTO_FRACO);
    }
}

/*
 * desenharResultado
 * Tabela de chegada com todos os carros, por cima da corrida (que continua
 * andando ao fundo: os rivais que ainda não chegaram terminam a prova
 * e a tabela vai se completando).
 * A ordem vem direto de corrida->classificacao, já ordenada pelo
 * Insertion Sort: quem chegou fica na frente, pelo tempo de chegada.
 * Recebe: a corrida.
 * Devolve: nada.
 */
void desenharResultado(Corrida *corrida) {
    escurecerTela(0.75f, 0.6f, 0.8f);

    if (corrida->desclassificado) {
        desenharCabecalho("DESCLASSIFICADO: SEM PARADA");
    } else if (corrida->posicaoFinal == 1) {
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
        desenharParalelogramo(LARGURA_TELA / 2.0f - largura / 2, 76, largura, 20, 6, Fade(COR_GRAFITE, 0.9f));
        escreverCentralizado(FONTE_FORTE, aviso, LARGURA_TELA / 2.0f + 4, 77, 14.0f, Fade(COR_CROMO, alfa));
    }

    Dica dicas[] = {
        { { "ENTER", NULL }, "CORRER DE NOVO" },
        { { "ESC", NULL }, "MENU" },
    };
    desenharBarraDeDicas(dicas, 2);
}
