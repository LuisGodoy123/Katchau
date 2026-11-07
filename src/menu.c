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
#include "carros3d.h"
#include "visual.h"
#include "audio.h"
#include "exibicao.h"
#include "controle.h"
#include "recordes.h"

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

/* ---------- Menu principal à esquerda (sobre o vídeo de fundo) ---------- */
#define X_MENU_PRINCIPAL      44.0f    /* a coluna dos botões, encostada à esquerda */
#define Y_MENU_PRINCIPAL      236.0f
#define LARGURA_BOTAO_MENU    300.0f
#define ALTURA_BOTAO_MENU     44.0f
#define ESPACO_BOTOES_MENU    54.0f
#define LARGURA_LOGO_MENU     300.0f
#define ITENS_PRINCIPAIS      2        /* os dois primeiros itens são grandes; o resto, menor, depois de um fio */
#define LARGURA_DA_FAIXA_ESCURA 520.0f  /* o degradê escuro atrás dos botões, para lerem sobre o vídeo */

/* ---------- Pausa ---------- */
#define Y_TITULO_PAUSA        176.0f
#define Y_PRIMEIRA_OPCAO_PAUSA 270.0f

/* ---------- Garagem ---------- */
#define LARGURA_VOLTAR        120.0f
#define ALTURA_VOLTAR         30.0f
#define X_VOLTAR              (LARGURA_TELA - 20.0f - LARGURA_VOLTAR)
#define Y_VOLTAR              (Y_CABECALHO + 12.0f)
#define FRACAO_MINIMA_BARRA   0.15f   /* o pior carro ainda mostra um pedacinho de barra */

/* A vitrine da garagem, na grade de 4 */
#define X_PALCO               24.0f    /* o carro girando */
#define Y_PALCO               96.0f
#define LARGURA_PALCO         480.0f
#define ALTURA_PALCO          300.0f
#define X_FICHA               536.0f   /* o nome e os atributos */
#define Y_FICHA               128.0f
#define LARGURA_FICHA         240.0f
#define ALTURA_ATRIBUTO       56.0f
#define Y_CARROSSEL           432.0f   /* as miniaturas */
#define LARGURA_CARTAO        96.0f
#define ALTURA_CARTAO         80.0f
#define VAO_DO_CARROSSEL      8.0f
#define X_ORDEM               (LARGURA_TELA - 24.0f - LARGURA_ORDEM)
#define Y_ORDEM               396.0f
#define LARGURA_ORDEM         180.0f
#define ALTURA_ORDEM          24.0f
#define GIRO_DA_VITRINE       0.55f    /* radianos por segundo */
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
 * O título da tela, no alto à esquerda, como a etiqueta das telas de configurações dos jogos de
 * corrida modernos: um retângulo vermelho de cantos arredondados com o título em branco.
 * Recebe: o título.
 * Devolve: nada.
 */
static void desenharCabecalho(const char *titulo) {
    float tamanho = 26.0f;
    Rectangle caixa = { 28.0f, Y_CABECALHO - 6.0f, larguraDoTexto(FONTE_TITULO, titulo, tamanho) + 44.0f, 40.0f };
    caixaArredondada(caixa, raioDoTamanho(caixa.width, caixa.height), COR_VERMELHO);
    escrever(FONTE_TITULO, titulo, caixa.x + 22.0f, caixa.y + (caixa.height - tamanho) / 2.0f - 2.0f, tamanho, WHITE);
}

/*
 * desenharCabecalhoDeEvento
 * O cabeçalho da escolha de evento: uma etiqueta vermelha pequena, o título grande em branco e,
 * Recebe: a etiqueta e o título.
 * Devolve: nada.
 */
static void desenharCabecalhoDeEvento(const char *etiqueta, const char *titulo) {
    float x = 40.0f;
    float larguraDaEtiqueta = larguraDoTexto(FONTE_FORTE, etiqueta, 14.0f) + 24.0f;
    caixaArredondada((Rectangle){ x, Y_CABECALHO - 6.0f, larguraDaEtiqueta, 22.0f }, RAIO_PEQUENO, COR_VERMELHO);
    escrever(FONTE_FORTE, etiqueta, x + 12.0f, Y_CABECALHO - 4.0f, 14.0f, WHITE);
    escrever(FONTE_TITULO, titulo, x - 1.0f, Y_CABECALHO + 18.0f, 36.0f, WHITE);
}

/*
 * desenharBarraDeDicas
 * A barra de dicas de teclas do pé da tela foi retirada: o padrão dos jogos de corrida modernos não
 * a mostra. A função continua para as telas poderem dizer o que cada botão faz, se um dia
 * voltarem a querer.
 * Recebe: as dicas e quantas são.
 * Devolve: nada.
 */
static void desenharBarraDeDicas(const Dica dicas[], int total) {
    (void) dicas;
    (void) total;
}

/*
 * maiusculas
 * Copia um texto em maiúsculas, inclusive as letras acentuadas (UTF-8 de 2 bytes: Ã, É, Ç...).
 * Recebe: onde guardar, o tamanho disso e o texto.
 * Devolve: nada.
 */
static void maiusculas(char *destino, int tamanho, const char *texto) {
    int n = 0;
    for (const unsigned char *c = (const unsigned char *) texto; *c != '\0' && n < tamanho - 2; c++) {
        if (*c == 0xC3 && c[1] >= 0xA0 && c[1] <= 0xBE && c[1] != 0xB7) {
            destino[n++] = (char) 0xC3;
            destino[n++] = (char) (c[1] - 0x20);
            c++;
        } else if (*c >= 'a' && *c <= 'z') {
            destino[n++] = (char) (*c - 32);
        } else {
            destino[n++] = (char) *c;
        }
    }
    destino[n] = '\0';
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
    float raio = raioDoTamanho(largura, altura);
    caixaArredondada((Rectangle){ x, y, largura, altura }, raio, COR_PAINEL);
    /* O fio vermelho fica no trecho reto da borda de cima, entre as curvas dos cantos */
    caixaArredondada((Rectangle){ x + raio, y, largura - 2.0f * raio, 4.0f }, 2.0f, COR_VERMELHO);
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
    menu->abaConfig = 0;
    menu->segundosParaAplicar = 0.0f;
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
 * atualizarTelaDeEntrada
 * Qualquer botão (tecla, clique ou botão do controle) leva ao menu principal. Só o primeiro aperto
 * conta: a tela troca uma vez e o menu começa limpo, sem repetir o aperto.
 * Recebe: nada.
 * Devolve: TELA_MENU quando algo foi apertado, TELA_ENTRADA enquanto isso não acontece.
 */
Tela atualizarTelaDeEntrada(void) {
    int apertou = 0;
    while (GetKeyPressed() != 0) {   /* esvazia a fila de teclas: nenhuma tecla desta vez chega ao menu */
        apertou = 1;
    }
    for (int botao = MOUSE_BUTTON_LEFT; botao <= MOUSE_BUTTON_BACK; botao++) {
        apertou = apertou || IsMouseButtonPressed(botao);
    }
    apertou = apertou || botaoApertadoAgora() != 0;
    return apertou ? TELA_MENU : TELA_ENTRADA;
}

/*
 * yDoItemDoMenu / alturaDoItemDoMenu
 * Onde fica cada item da coluna do menu principal: os principais são grandes; depois de um fio vêm
 * os secundários, menores.
 * Recebe: o número do item.
 * Devolve: o y do topo dele / a altura dele.
 */
static float yDoItemDoMenu(int i) {
    if (i < ITENS_PRINCIPAIS) {
        return Y_MENU_PRINCIPAL + i * 58.0f;
    }
    return Y_MENU_PRINCIPAL + ITENS_PRINCIPAIS * 58.0f + 24.0f + (i - ITENS_PRINCIPAIS) * 42.0f;
}

static float alturaDoItemDoMenu(int i) {
    return (i < ITENS_PRINCIPAIS) ? 52.0f : 36.0f;
}

static int botaoDoMenuSobOMouse(void) {
    float mouseX, mouseY;
    posicaoDoMouseNaInterface(&mouseX, &mouseY);
    for (int i = 0; i < TOTAL_OPCOES; i++) {
        if (dentroDoRetangulo(mouseX, mouseY, X_MENU_PRINCIPAL, yDoItemDoMenu(i), LARGURA_BOTAO_MENU + 12.0f, alturaDoItemDoMenu(i))) {
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

    int sobre = botaoDoMenuSobOMouse();
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
 * desenharBotaoPadrao
 * O botão dos menus, no padrão dos jogos de corrida modernos: retângulo escuro de cantos levemente
 * arredondados e texto branco. Quando marcado, ganha um contorno branco, uma barra vermelha à
 * esquerda e fica um pouco mais largo e claro (a animação é o destaque, de 0 a 1).
 * Recebe: o texto, a posição, a largura e o destaque (0 a 1).
 * Devolve: nada.
 */
static void desenharBotaoPadrao(const char *texto, float x, float y, float larguraBase, float destaque) {
    float suave = suavizar(destaque);
    float largura = larguraBase + 14.0f * suave;
    Rectangle r = { x, y, largura, ALTURA_BOTAO_MENU };
    float raio = raioDoTamanho(r.width, r.height);
    Color fundo = ColorLerp(Fade(COR_GRAFITE, 0.86f), Fade((Color){ 46, 50, 62, 255 }, 0.94f), suave);
    caixaArredondada(r, raio, fundo);
    contornoArredondado(r, raio, 1.0f, Fade(COR_CROMO, 0.16f * (1.0f - suave)));
    if (suave > 0.0f) {
        contornoArredondado(r, raio, 2.0f, Fade(WHITE, suave));
        caixaArredondada((Rectangle){ x + 4.0f, y + 8.0f, 4.0f * suave, ALTURA_BOTAO_MENU - 16.0f }, raioDeDentro(raio, 4.0f), Fade(COR_VERMELHO, suave));
    }
    float tamanho = 22.0f + 1.5f * suave;
    escrever(FONTE_TITULO, texto, x + 22.0f + 4.0f * suave, y + (ALTURA_BOTAO_MENU - tamanho) / 2.0f - 2.0f, tamanho,
             ColorLerp(COR_TEXTO_FRACO, WHITE, suave));
}

/*
 * desenharOpcao
 * Um botão da pausa (e de outras telas): o botão padrão, no meio da tela.
 * Recebe: o texto, a altura (y) e o destaque (0 a 1).
 * Devolve: nada.
 */
static void desenharOpcao(const char *texto, float y, float destaque) {
    desenharBotaoPadrao(texto, (LARGURA_TELA - LARGURA_BOTAO_MENU) / 2.0f - 7.0f, y, LARGURA_BOTAO_MENU, destaque);
}

/*
 * desenharItemDoMenu
 * Um item da coluna do menu principal, como no menu dos jogos de corrida modernos: o texto grande e
 * branco, sem caixa; o item marcado fica dentro de uma pílula cinza de contorno branco.
 * Recebe: o texto, o número do item e o destaque (0 a 1).
 * Devolve: nada.
 */
static void desenharItemDoMenu(const char *texto, int item, float destaque) {
    float suave = suavizar(destaque);
    int principal = item < ITENS_PRINCIPAIS;
    float y = yDoItemDoMenu(item), altura = alturaDoItemDoMenu(item);
    float largura = (principal ? LARGURA_BOTAO_MENU : LARGURA_BOTAO_MENU - 40.0f) + 10.0f * suave;
    Rectangle pilula = { X_MENU_PRINCIPAL - 6.0f, y, largura, altura };
    if (suave > 0.0f) {
        float raio = raioDoTamanho(pilula.width, pilula.height);
        caixaArredondada(pilula, raio, Fade((Color){ 82, 86, 98, 255 }, 0.80f * suave));
        contornoArredondado(pilula, raio, 2.0f, Fade(WHITE, suave));
    }
    float tamanho = principal ? 30.0f : 21.0f;
    escrever(FONTE_TITULO, texto, X_MENU_PRINCIPAL + 14.0f, y + (altura - tamanho) / 2.0f - 3.0f, tamanho,
             ColorLerp(Fade(WHITE, 0.88f), WHITE, suave));
}

/*
 * descricaoDoItem
 * Recebe: o número do item do menu.
 * Devolve: o texto que explica o que ele faz (embaixo, à esquerda).
 */
static const char *descricaoDoItem(int item) {
    static const char *DESCRICOES[TOTAL_OPCOES] = {
        "Escolha a pista e a dificuldade e largue em último contra 13 rivais.",
        "Compare a velocidade, a aceleração e a aderência dos carros e escolha o seu.",
        "Veja os controles e as dicas de pilotagem.",
        "Ajuste os comandos, a vibração, a direção e os gráficos.",
        "Fechar o jogo.",
    };
    return DESCRICOES[item];
}

/*
 * desenharTelaDeEntrada
 * A tela de entrada: a logo no alto e "APERTE QUALQUER BOTÃO PARA ENTRAR" embaixo, pulsando. O
 * vídeo de fundo é desenhado antes, pelo main.c.
 * Recebe: o tempo do quadro (para a pulsação).
 * Devolve: nada.
 */
void desenharTelaDeEntrada(float tempoQuadro) {
    (void) tempoQuadro;
    float telaA = (float) alturaVirtual(), telaL = (float) larguraVirtual();
    /* Um degradê escuro só embaixo, onde fica a mensagem */
    DrawRectangleGradientV(-margemX(), (int) (telaA * 0.66f), (int) telaL, (int) (telaA * 0.34f + margemY()),
                           Fade(BLACK, 0.0f), Fade(BLACK, 0.75f));
    float flutuar = sinf((float) GetTime() * 1.2f) * 2.0f;
    desenharLogo(LARGURA_TELA / 2.0f, 40.0f + flutuar, 380.0f);
    float pulso = 0.55f + 0.45f * (0.5f + 0.5f * sinf((float) GetTime() * 2.6f));
    const char *aviso = "APERTE QUALQUER BOTÃO PARA ENTRAR";
    escreverCentralizado(FONTE_TITULO, aviso, LARGURA_TELA / 2.0f, 520.0f, 30.0f, Fade(WHITE, pulso));
}

void desenharMenuPrincipal(EstadoMenu *menu) {
    /* Uma faixa escura à esquerda (o vídeo continua visível à direita e por trás) */
    DrawRectangleGradientH(-margemX(), -margemY(), LARGURA_DA_FAIXA_ESCURA, alturaVirtual(),
                           Fade(BLACK, 0.80f), Fade(BLACK, 0.0f));

    /* A logo, no alto à esquerda, flutua devagar */
    float flutuar = sinf((float) GetTime() * 1.6f) * 2.0f;
    desenharLogo(X_MENU_PRINCIPAL + LARGURA_LOGO_MENU / 2.0f, 22.0f + flutuar, LARGURA_LOGO_MENU);

    for (int i = 0; i < TOTAL_OPCOES; i++) {
        if (i == ITENS_PRINCIPAIS) {   /* o fio entre os itens principais e os secundários */
            float yFio = yDoItemDoMenu(i) - 14.0f;
            DrawRectangleRec((Rectangle){ X_MENU_PRINCIPAL, yFio, LARGURA_BOTAO_MENU - 20.0f, 1.0f }, Fade(WHITE, 0.45f));
        }
        desenharItemDoMenu(TEXTOS_OPCOES[i], i, menu->destaque[i]);
    }

    /* O que o item marcado faz, embaixo à esquerda */
    escreverComQuebra(FONTE_TEXTO, descricaoDoItem(menu->opcaoMenu), X_MENU_PRINCIPAL, 540.0f, 16.0f, 330.0f, COR_TEXTO_CLARO);

    /* O carro escolhido, em uma etiqueta no canto de cima, à direita */
    {
        const char *rotulo = "SEU CARRO";
        const char *nome = fichaDoCarro(menu->carroEscolhido)->nome;
        float larguraDaEtiqueta = larguraDoTexto(FONTE_FORTE, rotulo, 13.0f) + larguraDoTexto(FONTE_TITULO, nome, 20.0f) + 56.0f;
        float x = LARGURA_TELA + margemX() - larguraDaEtiqueta - 26.0f;
        caixaArredondada((Rectangle){ x, 24.0f, larguraDaEtiqueta, 32.0f }, RAIO_PEQUENO, Fade(COR_GRAFITE, 0.85f));
        caixaArredondada((Rectangle){ x + 8.0f, 32.0f, 4.0f, 16.0f }, raioDeDentro(RAIO_PEQUENO, 8.0f), COR_VERMELHO);
        escrever(FONTE_FORTE, rotulo, x + 20.0f, 32.0f, 13.0f, COR_TEXTO_FRACO);
        escrever(FONTE_TITULO, nome, x + 34.0f + larguraDoTexto(FONTE_FORTE, rotulo, 13.0f), 28.0f, 20.0f, WHITE);
    }
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


/*
 * xDaMiniatura / miniaturaSobOMouse / sobreAOrdem / sobreOVoltar
 * Onde fica cada miniatura do carrossel (o carro marcado no meio da tela) e o que está embaixo
 * do mouse na garagem (coordenadas do quadro 800x600).
 */
static float xDaMiniatura(const EstadoMenu *menu, int i) {
    return LARGURA_TELA / 2.0f + (i - menu->rolagemDaVitrine) * (LARGURA_CARTAO + VAO_DO_CARROSSEL) - LARGURA_CARTAO / 2.0f;
}
static int miniaturaSobOMouse(const EstadoMenu *menu, float mouseX, float mouseY) {
    for (int i = 0; i < TOTAL_CARROS; i++) {
        if (dentroDoRetangulo(mouseX, mouseY, xDaMiniatura(menu, i), Y_CARROSSEL, LARGURA_CARTAO, ALTURA_CARTAO)) {
            return i;
        }
    }
    return -1;
}
static int sobreAOrdem(float mouseX, float mouseY) {
    return dentroDoRetangulo(mouseX, mouseY, X_ORDEM, Y_ORDEM, LARGURA_ORDEM, ALTURA_ORDEM);
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
    /* Entrando agora (o carrossel não andou desde a última vez): começa no seu carro, já no meio */
    static double ultimoQuadro = -1.0;
    double agora = GetTime();
    if (ultimoQuadro < 0.0 || agora - ultimoQuadro > 0.25) {
        for (int i = 0; i < TOTAL_CARROS; i++) {
            if (numeroDaFicha(menu->garagem[i]) == menu->carroEscolhido) {
                menu->selecionado = i;
                menu->rolagemDaVitrine = (float) i;
            }
        }
    }
    ultimoQuadro = agora;
    int marcadoAntes = menu->selecionado;
    if (apertouParaDireita() && menu->selecionado < TOTAL_CARROS - 1) {
        menu->selecionado++;
    }
    if (apertouParaEsquerda() && menu->selecionado > 0) {
        menu->selecionado--;
    }

    float mouseX, mouseY;
    posicaoDoMouseNaInterface(&mouseX, &mouseY);
    int cartao = miniaturaSobOMouse(menu, mouseX, mouseY);
    int ordem = sobreAOrdem(mouseX, mouseY);
    int voltar = sobreOVoltar(mouseX, mouseY);
    if ((voltar && !menu->voltarAnterior) || (ordem && menu->abaAnterior != 1)) {
        tocarSom(SOM_CURSOR);
    }
    menu->abaAnterior = ordem;
    menu->voltarAnterior = voltar;

    /* A ordem do carrossel: cima/baixo (ou o clique no seletor) trocam o critério do Merge Sort */
    int trocarOrdem = 0;
    if (apertouParaBaixo() || (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && ordem)) {
        trocarOrdem = 1;
    } else if (apertouParaCima()) {
        trocarOrdem = -1;
    }
    if (trocarOrdem != 0) {
        menu->criterio = (Atributo) ((menu->criterio + TOTAL_ATRIBUTOS + trocarOrdem) % TOTAL_ATRIBUTOS);
        reordenarGaragem(menu);
        menu->rolagemDaVitrine = (float) menu->selecionado;   /* o mesmo carro, sem o carrossel correr */
        tocarSom(SOM_CURSOR);
        marcadoAntes = menu->selecionado;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (voltar) {
            tocarSom(SOM_VOLTAR);
            return TELA_MENU;
        }
        if (cartao >= 0) {
            if (cartao == menu->selecionado) {
                menu->carroEscolhido = numeroDaFicha(menu->garagem[cartao]);
                tocarSom(SOM_CONFIRMAR);
                return TELA_MENU;
            }
            menu->selecionado = cartao;   /* o primeiro clique traz para o meio; o segundo escolhe */
        }
    }
    if (menu->selecionado != marcadoAntes) {
        tocarSom(SOM_CURSOR);
    }

    /* O carrossel desliza até o marcado; o carro gira; os cartões acendem e apagam */
    float passo = fminf(GetFrameTime() * 10.0f, 1.0f);
    menu->rolagemDaVitrine += (menu->selecionado - menu->rolagemDaVitrine) * passo;
    menu->giroDaVitrine = fmodf(menu->giroDaVitrine + GIRO_DA_VITRINE * GetFrameTime(), 2.0f * PI);
    for (int i = 0; i < TOTAL_CARROS; i++) {
        menu->destaqueLinha[i] = animarDestaque(menu->destaqueLinha[i], i == menu->selecionado);
    }
    menu->destaqueOrdem = animarDestaque(menu->destaqueOrdem, ordem);
    menu->destaqueVoltar = animarDestaque(menu->destaqueVoltar, voltar);

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
 * notaGeral
 * Recebe: a ficha.
 * Devolve: a nota do carro, de 70 a 99, pela média dos três atributos (o selo "geral").
 */
static int notaGeral(const FichaTecnica *ficha) {
    float soma = 0.0f;
    for (int a = 0; a < TOTAL_ATRIBUTOS; a++) {
        soma += fracaoDoAtributo(ficha, (Atributo) a);
    }
    return 70 + (int) (soma / TOTAL_ATRIBUTOS * 29.0f + 0.5f);
}

/*
 * desenharCartaoDoCarro
 * A ficha do carro, como um cartão de piloto: o brilho na cor da equipe, o número grande ao fundo,
 * o nome, o selo da nota geral e os três atributos em barras de gomos. O atributo que ordena o
 * carrossel fica aceso.
 * Recebe: o estado do menu, a ficha e o número do carro.
 * Devolve: nada.
 */
static void desenharCartaoDoCarro(EstadoMenu *menu, const FichaTecnica *ficha, int numero) {
    const float x = X_FICHA - 16.0f, largura = LARGURA_FICHA + 32.0f, altura = 264.0f;
    const float y0 = Y_FICHA - 24.0f;
    Color equipe = corDoCarro(numero);

    /* O cartão e a faixa da equipe */
    Rectangle cartao = { x, y0, largura, altura };
    caixaArredondada(cartao, RAIO_GRANDE, Fade(COR_GRAFITE, 0.86f));
    caixaArredondada(cartao, RAIO_GRANDE, Fade(equipe, 0.07f));   /* um toque da cor da equipe no cartão todo */
    contornoArredondado(cartao, RAIO_GRANDE, 1.0f, Fade(COR_CROMO, 0.16f));
    /* O número da equipe, grande e apagado, atrás do nome */
    escreverADireita(FONTE_TITULO, TextFormat("%d", numero + 1), x + largura - 12.0f, y0 - 8.0f, 88.0f, Fade(COR_TEXTO_CLARO, 0.06f));

    /* O nome, a frase dele e o selo da nota */
    float y = y0 + 16.0f;
    escrever(FONTE_TITULO, ficha->nome, X_FICHA, y, 28.0f, COR_TEXTO_CLARO);
    escrever(FONTE_TEXTO, ficha->descricao, X_FICHA, y + 32.0f, 12.0f, COR_TEXTO_FRACO);
    float xSelo = X_FICHA + LARGURA_FICHA - 48.0f;
    caixaArredondada((Rectangle){ xSelo, y, 48.0f, 48.0f }, raioDeDentro(RAIO_GRANDE, 16.0f), equipe);   /* 16 do cartão */
    escreverCentralizado(FONTE_TITULO, TextFormat("%d", notaGeral(ficha)), xSelo + 24.0f, y + 2.0f, 28.0f, COR_TEXTO_CLARO);
    escreverCentralizado(FONTE_FORTE, "GERAL", xSelo + 24.0f, y + 32.0f, 10.0f, Fade(COR_TEXTO_CLARO, 0.8f));
    if (numero == menu->carroEscolhido) {
        caixaArredondada((Rectangle){ X_FICHA, y + 52.0f, 64.0f, 16.0f }, raioDeDentro(RAIO_GRANDE, 16.0f), COR_AMARELO);
        escreverCentralizado(FONTE_FORTE, "SEU CARRO", X_FICHA + 32.0f, y + 52.0f, 12.0f, COR_GRAFITE);
    }
    y += 84.0f;

    /* Os atributos: rótulo, valor, colocação e a barra em 12 gomos */
    const int gomos = 12;
    const float vao = 4.0f, larguraDoGomo = (LARGURA_FICHA - vao * (gomos - 1)) / gomos;
    for (int atributo = 0; atributo < TOTAL_ATRIBUTOS; atributo++) {
        int ordenando = (atributo == (int) menu->criterio);
        Color corDaBarra = ordenando ? COR_AMARELO : equipe;
        escrever(FONTE_FORTE, NOMES_CRITERIO[atributo], X_FICHA, y, 12.0f, ordenando ? COR_AMARELO : COR_TEXTO_FRACO);
        escreverADireita(FONTE_FORTE, textoDoAtributo(ficha, (Atributo) atributo), X_FICHA + LARGURA_FICHA, y - 4.0f, 20.0f, COR_TEXTO_CLARO);

        int cheios = (int) (fracaoDoAtributo(ficha, (Atributo) atributo) * gomos + 0.5f);
        for (int g = 0; g < gomos; g++) {
            float gx = X_FICHA + g * (larguraDoGomo + vao);
            Color cor = (g < cheios) ? Fade(corDaBarra, 0.55f + 0.45f * (g + 1) / gomos) : Fade(COR_TEXTO_CLARO, 0.08f);
            DrawRectangleRounded((Rectangle){ gx, y + 24.0f, larguraDoGomo, 8.0f }, 0.5f, 4, cor);   /* gomos em pílula */
        }
        y += ALTURA_ATRIBUTO;
    }
}

/*
 * desenharGaragem
 * Recebe: o estado do menu (o fundo é desenhado antes, pelo main.c).
 * Devolve: nada.
 */
void desenharGaragem(EstadoMenu *menu) {
    const FichaTecnica *ficha = menu->garagem[menu->selecionado];
    int numero = numeroDaFicha(ficha);

    /* A foto do carro girando sai antes: ela tem a câmera dela, fora do modo 2D */
    terminarInterface();
    Vector2 chao;
    Texture2D foto = fotografarVitrine(numero, menu->giroDaVitrine, &chao);
    comecarInterface(1);

    escurecerTela(0.7f, 0.55f, 0.75f);
    desenharCabecalho("GARAGEM");
    desenharBotaoVoltar(menu->destaqueVoltar);

    /* O palco: holofote, e o carro em cima dele */
    /* O chão do palco cai exatamente embaixo do carro (o ponto que a foto devolve) */
    float chaoX = X_PALCO + LARGURA_PALCO * chao.x, chaoY = Y_PALCO + ALTURA_PALCO * chao.y;
    desenharBrilho(chaoX, chaoY - 56.0f, 200.0f, Fade(COR_AMARELO, 0.14f));
    DrawEllipse((int) chaoX, (int) chaoY, 184, 24, Fade(BLACK, 0.40f));
    DrawTexturePro(foto, (Rectangle){ 0, 0, (float) foto.width, -(float) foto.height },
                   (Rectangle){ X_PALCO, Y_PALCO, LARGURA_PALCO, ALTURA_PALCO }, (Vector2){ 0 }, 0.0f, WHITE);

    desenharCartaoDoCarro(menu, ficha, numero);

    /* O seletor da ordem do carrossel, discreto, acima dele à direita */
    float suave = suavizar(menu->destaqueOrdem);
    caixaArredondada((Rectangle){ X_ORDEM, Y_ORDEM, LARGURA_ORDEM, ALTURA_ORDEM }, RAIO_PEQUENO, ColorLerp(Fade(COR_GRAFITE, 0.8f), COR_VERMELHO, suave));
    escrever(FONTE_FORTE, "ORDEM", X_ORDEM + 8.0f, Y_ORDEM + 4.0f, 12.0f, COR_TEXTO_FRACO);
    escreverADireita(FONTE_FORTE, NOMES_CRITERIO[menu->criterio], X_ORDEM + LARGURA_ORDEM - 8.0f, Y_ORDEM + 4.0f, 12.0f, COR_TEXTO_CLARO);

    /* O carrossel: o marcado no meio, aceso, com a faixa vermelha; os outros somem para as bordas */
    for (int i = 0; i < TOTAL_CARROS; i++) {
        float x = xDaMiniatura(menu, i);
        if (x + LARGURA_CARTAO < 0.0f || x > LARGURA_TELA) {
            continue;
        }
        float aceso = suavizar(menu->destaqueLinha[i]);
        float distancia = fabsf(x + LARGURA_CARTAO / 2.0f - LARGURA_TELA / 2.0f) / (LARGURA_TELA / 2.0f);
        float alfa = fmaxf(1.0f - distancia * 0.85f, 0.15f);
        int carro = numeroDaFicha(menu->garagem[i]);
        Rectangle cartao = { x, Y_CARROSSEL, LARGURA_CARTAO, ALTURA_CARTAO };
        caixaArredondada(cartao, RAIO_MEDIO, Fade(ColorLerp(COR_GRAFITE, COR_PAINEL, aceso), (0.55f + 0.35f * aceso) * alfa));
        BeginBlendMode(BLEND_ALPHA);
        desenharMiniatura(carro, x + LARGURA_CARTAO / 2.0f, Y_CARROSSEL + ALTURA_CARTAO - 12.0f, LARGURA_CARTAO - 16.0f);
        EndBlendMode();
        if (alfa < 1.0f) {   /* esmaece a miniatura junto com o cartão */
            caixaArredondada(cartao, RAIO_MEDIO, Fade(COR_GRAFITE, (1.0f - alfa) * 0.8f));
        }
        if (aceso > 0.01f) {   /* o marcado: um traço vermelho em pílula, centralizado embaixo */
            float traco = 48.0f * aceso;
            DrawRectangleRounded((Rectangle){ x + (LARGURA_CARTAO - traco) / 2.0f, Y_CARROSSEL + ALTURA_CARTAO + 8.0f, traco, 4.0f }, 1.0f, 4, COR_VERMELHO);
        }
        if (carro == menu->carroEscolhido) {   /* o seu carro: contornado em amarelo */
            contornoArredondado(cartao, RAIO_MEDIO, 2.0f, Fade(COR_AMARELO, alfa));
        }
    }

    Dica dicas[] = {
        { { "ESQ", "DIR", NULL }, "CARRO" },
        { { "CIMA", "BAIXO", NULL }, "ORDEM" },
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

#define Y_CARTAO_PISTA        452.0f   /* os cartões das pistas, em fila embaixo */
#define LARGURA_CARTAO_PISTA  182.0f
#define ALTURA_CARTAO_PISTA   118.0f
#define ESPACO_ENTRE_CARTOES  10.0f
#define X_INFO_PISTA          28.0f    /* o cartão de informações da pista marcada */
#define Y_INFO_PISTA          128.0f
#define LARGURA_INFO_PISTA    272.0f
#define ALTURA_INFO_PISTA     296.0f   /* as três linhas de informação e 16 de folga embaixo */
#define X_COLUNA_DIREITA      326.0f   /* a coluna da dificuldade e do CORRER */
#define LARGURA_COLUNA_DIREITA 440.0f
#define LADO_DO_TRACADO       96.0f
#define PASSO_DO_TRACADO      4        /* desenha o traçado de 4 em 4 segmentos */
#define Y_TITULO_NIVEL        132.0f
#define Y_BOTAO_NIVEL         162.0f
#define LARGURA_BOTAO_NIVEL   140.0f
#define ALTURA_BOTAO_NIVEL    46.0f
#define ESPACO_ENTRE_NIVEIS   10.0f
#define Y_RESUMO              222.0f
#define Y_BOTAO_CORRER        256.0f
#define X_BOTAO_CORRER        326.0f
#define LARGURA_BOTAO_CORRER  440.0f
#define ALTURA_BOTAO_CORRER   58.0f
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
static void desenharTracado(Pista *pista, float centroX, float centroY, float lado, Color cor, float espessura) {
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
            DrawLineEx(ponto, seguinte, espessura, cor);
            ponto = seguinte;
        }
    } while (atual != pista->largada);

    /* A linha de chegada: um pontinho vermelho */
    DrawCircleV(inicio, espessura * 1.1f, COR_VERMELHO);
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
    return X_COLUNA_DIREITA + nivel * (LARGURA_BOTAO_NIVEL + ESPACO_ENTRE_NIVEIS);
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
    int sobreCorrer = dentroDoRetangulo(mouseX, mouseY, X_BOTAO_CORRER, Y_BOTAO_CORRER, LARGURA_BOTAO_CORRER, ALTURA_BOTAO_CORRER);
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
 * O contorno de "você está aqui" é branco, e pulsa de leve, para chamar o olho.
 * Recebe: nada.
 * Devolve: a cor deste quadro.
 */
static Color corDoCursor(void) {
    return ColorLerp(COR_CROMO, WHITE, 0.55f + 0.45f * sinf((float) GetTime() * RAPIDEZ_DO_CURSOR));
}

/*
 * contarCurvas
 * Conta as curvas de uma pista pelos segmentos: um trecho seguido de segmentos curvos (juntando
 * os que têm menos de 25 retos entre eles) que, somados, viram pelo menos 25 graus. A conta é
 * feita uma vez por pista.
 * Recebe: a pista.
 * Devolve: quantas curvas ela tem.
 */
static int contarCurvas(const Pista *pista) {
    static int guardado[TOTAL_PISTAS];
    static int pronto[TOTAL_PISTAS];
    if (pronto[pista->id]) {
        return guardado[pista->id];
    }
    int curvas = 0, emCurva = 0, retos = 0;
    float soma = 0.0f;
    const Segmento *atual = pista->largada;
    do {
        float curvatura = fabsf(atual->curvatura);
        if (curvatura > 0.10f) {
            emCurva = 1;
            retos = 0;
            soma += curvatura;
        } else if (emCurva && ++retos > 25) {
            curvas += (soma >= 25.0f);
            emCurva = 0;
            soma = 0.0f;
        }
        atual = atual->proximo;
    } while (atual != pista->largada);
    curvas += (emCurva && soma >= 25.0f);
    guardado[pista->id] = curvas;
    pronto[pista->id] = 1;
    return curvas;
}

/*
 * desenharCartaoDePista
 * Um cartão da fila de baixo: o nome em maiúsculas no alto e o traçado da pista embaixo. O marcado
 * fica mais claro, com contorno branco.
 * Recebe: o cartão, a pista, o destaque (0 a 1) e se o cursor está nele.
 * Devolve: nada.
 */
static void desenharCartaoDePista(int cartao, Pista *pista, float destaque, int comCursor) {
    float suave = suavizar(destaque);
    float x = xDoCartao(cartao);
    Rectangle r = { x, Y_CARTAO_PISTA, LARGURA_CARTAO_PISTA, ALTURA_CARTAO_PISTA };
    float raio = raioDoTamanho(r.width, r.height);
    caixaArredondada(r, raio, ColorLerp(Fade(COR_GRAFITE, 0.84f), Fade((Color){ 56, 60, 72, 255 }, 0.96f), suave));
    contornoArredondado(r, raio, 1.0f, Fade(COR_CROMO, 0.18f * (1.0f - suave)));
    if (suave > 0.0f) {
        contornoArredondado(r, raio, comCursor ? 2.5f : 1.5f, Fade(comCursor ? corDoCursor() : COR_CROMO, suave));
    }
    char nome[48];
    maiusculas(nome, sizeof(nome), pista->nome);
    escrever(FONTE_FORTE, nome, x + 14.0f, Y_CARTAO_PISTA + 11.0f, 16.0f, ColorLerp(COR_TEXTO_FRACO, WHITE, 0.4f + 0.6f * suave));
    desenharTracado(pista, x + LARGURA_CARTAO_PISTA / 2.0f, Y_CARTAO_PISTA + 76.0f, 60.0f,
                    ColorLerp(Fade(COR_CROMO, 0.55f), WHITE, suave), 3.0f);
}

/*
 * desenharNomeGrandeDaPista
 * O nome da pista em letras enormes só de contorno, atravessando a tela atrás de tudo.
 * Recebe: o nome em maiúsculas.
 * Devolve: nada.
 */
static void desenharNomeGrandeDaPista(const char *nome) {
    float tamanho = 108.0f;
    float largura = larguraDoTexto(FONTE_TITULO, nome, tamanho);
    if (largura > 730.0f) {
        tamanho *= 730.0f / largura;
        largura = 730.0f;
    }
    float x = LARGURA_TELA + margemX() - 28.0f - largura, y = 338.0f;
    for (int k = 0; k < 8; k++) {   /* o contorno: o texto claro deslocado em volta... */
        float angulo = k * PI / 4.0f;
        escreverSemSombra(FONTE_TITULO, nome, x + cosf(angulo) * 1.6f, y + sinf(angulo) * 1.6f, tamanho, Fade(WHITE, 0.40f));
    }
    escreverSemSombra(FONTE_TITULO, nome, x, y, tamanho, Fade((Color){ 14, 14, 20, 255 }, 0.55f));   /* ...e o miolo escuro */
}

/*
 * desenharLinhaDoCartaoDeInfo
 * Um par "RÓTULO / valor" do cartão de informações, com um fio embaixo.
 * Recebe: o x, o y, a largura, o rótulo e o valor.
 * Devolve: nada.
 */
static void desenharLinhaDoCartaoDeInfo(float x, float y, float largura, const char *rotulo, const char *valor) {
    escrever(FONTE_FORTE, rotulo, x, y, 14.0f, WHITE);
    escrever(FONTE_TEXTO, valor, x, y + 17.0f, 17.0f, COR_CROMO);
    DrawRectangleRec((Rectangle){ x, y + 41.0f, largura, 1.0f }, Fade(COR_CROMO, 0.25f));
}

/*
 * desenharInfoDaPista
 * O cartão de informações da pista marcada: o traçado em cima e, embaixo, as curvas, o comprimento,
 * a dificuldade da pista e o recorde de volta.
 * Recebe: a pista.
 * Devolve: nada.
 */
static void desenharInfoDaPista(Pista *pista) {
    Rectangle r = { X_INFO_PISTA, Y_INFO_PISTA, LARGURA_INFO_PISTA, ALTURA_INFO_PISTA };
    caixaArredondada(r, raioDoTamanho(r.width, r.height), Fade((Color){ 22, 22, 30, 255 }, 0.84f));
    contornoArredondado(r, raioDoTamanho(r.width, r.height), 1.0f, Fade(COR_CROMO, 0.16f));
    desenharTracado(pista, X_INFO_PISTA + LARGURA_INFO_PISTA / 2.0f, Y_INFO_PISTA + 76.0f, LADO_DO_TRACADO, WHITE, 6.0f);

    float metros = contarSegmentos(pista) * COMPRIMENTO_SEGMENTO / UNIDADES_POR_METRO;
    int curvas = contarCurvas(pista);
    const char *dificuldade = (curvas <= 5) ? "FÁCIL" : (curvas <= 10) ? "MÉDIA" : "DIFÍCIL";
    char dono[TAMANHO_DO_NOME_DO_RECORDE];
    float recorde = recordeDaPista(pista->id, dono, sizeof(dono));
    char tempo[32] = "N/D", comprimento[32], quantas[16];
    if (recorde > 0.0f) {
        formatarTempo(recorde, tempo, sizeof(tempo));
    }
    snprintf(comprimento, sizeof(comprimento), "%.3f km", metros / 1000.0f);
    snprintf(quantas, sizeof(quantas), "%d", curvas);

    float x1 = X_INFO_PISTA + 18.0f, x2 = X_INFO_PISTA + LARGURA_INFO_PISTA / 2.0f + 6.0f;
    float larguraDaColuna = LARGURA_INFO_PISTA / 2.0f - 26.0f;
    float y0 = Y_INFO_PISTA + 142.0f;
    desenharLinhaDoCartaoDeInfo(x1, y0, larguraDaColuna, "CURVAS", quantas);
    desenharLinhaDoCartaoDeInfo(x2, y0, larguraDaColuna, "COMPRIMENTO", comprimento);
    desenharLinhaDoCartaoDeInfo(x1, y0 + 48.0f, larguraDaColuna, "DIFICULDADE", dificuldade);
    desenharLinhaDoCartaoDeInfo(x2, y0 + 48.0f, larguraDaColuna, "RECORDE DA VOLTA", tempo);
    desenharLinhaDoCartaoDeInfo(x1, y0 + 96.0f, larguraDaColuna * 2.0f + 14.0f, "RECORDISTA", (recorde > 0.0f && dono[0] != '\0') ? dono : "N/D");
}

/*
 * desenharBotaoDeNivel
 * Uma pílula da dificuldade: a escolhida fica vermelha; o contorno branco diz que o cursor está
 * nessa linha.
 * Recebe: o nível, o destaque (0 a 1) e se o cursor está nele.
 * Devolve: nada.
 */
static void desenharBotaoDeNivel(int nivel, float destaque, int comCursor) {
    float suave = suavizar(destaque);
    float x = xDoNivel(nivel);
    Rectangle r = { x, Y_BOTAO_NIVEL, LARGURA_BOTAO_NIVEL, ALTURA_BOTAO_NIVEL };
    float raio = raioDoTamanho(r.width, r.height);
    caixaArredondada(r, raio, ColorLerp(Fade(COR_GRAFITE, 0.86f), COR_VERMELHO, suave));
    contornoArredondado(r, raio, 1.0f, Fade(COR_CROMO, 0.18f * (1.0f - suave)));
    if (comCursor) {
        contornoArredondado(r, raio, 2.5f, corDoCursor());
    }
    escreverCentralizado(FONTE_TITULO, fichaDoNivel((NivelDaCorrida) nivel)->nome, x + LARGURA_BOTAO_NIVEL / 2.0f,
                         Y_BOTAO_NIVEL + (ALTURA_BOTAO_NIVEL - 24.0f) / 2.0f - 2.0f, 24.0f,
                         ColorLerp(COR_TEXTO_FRACO, WHITE, suave));
}

/*
 * desenharBotaoCorrer
 * Sempre vermelho, por ser a ação principal; mais apagado até o cursor chegar nele.
 * Recebe: se o cursor está nele.
 * Devolve: nada.
 */
static void desenharBotaoCorrer(int comCursor) {
    Rectangle r = { X_BOTAO_CORRER, Y_BOTAO_CORRER, LARGURA_BOTAO_CORRER, ALTURA_BOTAO_CORRER };
    float raio = raioDoTamanho(r.width, r.height);
    caixaArredondada(r, raio, comCursor ? COR_VERMELHO : ColorLerp(Fade(COR_GRAFITE, 0.9f), COR_VERMELHO, 0.55f));
    if (comCursor) {
        contornoArredondado(r, raio, 2.5f, corDoCursor());
    }
    escreverCentralizado(FONTE_TITULO, "CORRER", X_BOTAO_CORRER + LARGURA_BOTAO_CORRER / 2.0f, Y_BOTAO_CORRER + 12.0f, 32.0f,
                         comCursor ? WHITE : Fade(WHITE, 0.78f));
}

/*
 * desenharResumoDaCorrida
 * O que muda na corrida marcada, em uma linha, embaixo da dificuldade.
 * Recebe: o estado do menu.
 * Devolve: nada.
 */
static void desenharResumoDaCorrida(EstadoMenu *menu) {
    const FichaDoNivel *ficha = fichaDoNivel(menu->nivelEscolhido);
    int parada = ficha->temParada;
    int gasta = ficha->gastaPneu && parada;
    char voltas[32];
    snprintf(voltas, sizeof(voltas), "%d VOLTAS", ficha->voltas);
    const char *partes[3] = { voltas, parada ? "PITSTOP OBRIGATÓRIO" : "SEM PITSTOP", "PNEUS GASTAM" };
    int total = gasta ? 3 : 2;
    float x = X_COLUNA_DIREITA;
    for (int i = 0; i < total; i++) {
        escrever(FONTE_FORTE, partes[i], x, Y_RESUMO, 15.0f, COR_TEXTO_CLARO);
        x += larguraDoTexto(FONTE_FORTE, partes[i], 15.0f);
        if (i + 1 < total) {
            DrawRectangleRounded((Rectangle){ x + 11.0f, Y_RESUMO + 5.0f, 6.0f, 6.0f }, 1.0f, 4, COR_VERMELHO);
            x += 28.0f;
        }
    }
}

/*
 * desenharEscolhaDePista
 * A escolha do evento, no padrão dos jogos de corrida modernos: o cartão de informações da pista, a
 * dificuldade e o CORRER à direita, o nome da pista em letras de contorno atrás e a fila de pistas
 * embaixo.
 * Recebe: o estado do menu e as pistas.
 * Devolve: nada.
 */
void desenharEscolhaDePista(EstadoMenu *menu, Pista *pistas[]) {
    escurecerTela(0.45f, 0.15f, 0.55f);
    Pista *marcada = pistas[menu->pistaEscolhida];
    char nome[48];
    maiusculas(nome, sizeof(nome), marcada->nome);
    desenharCabecalhoDeEvento("CORRIDA", "ESCOLHER EVENTO");
    desenharBotaoVoltar(menu->destaqueVoltar);

    int foco = menu->focoDaCorrida;
    desenharNomeGrandeDaPista(nome);
    desenharInfoDaPista(marcada);

    escrever(FONTE_TITULO, "DIFICULDADE", X_COLUNA_DIREITA, Y_TITULO_NIVEL, 17.0f, WHITE);
    float fimDoTitulo = larguraDoTexto(FONTE_TITULO, "DIFICULDADE", 17.0f);
    DrawRectangleRec((Rectangle){ X_COLUNA_DIREITA + fimDoTitulo + 12.0f, Y_TITULO_NIVEL + 12.0f, LARGURA_COLUNA_DIREITA - fimDoTitulo - 12.0f, 1.0f },
                     Fade(COR_CROMO, 0.3f));
    for (int i = 0; i < TOTAL_NIVEIS; i++) {
        desenharBotaoDeNivel(i, menu->destaqueNivel[i], foco == FOCO_NO_NIVEL && i == (int) menu->nivelEscolhido);
    }
    desenharResumoDaCorrida(menu);
    desenharBotaoCorrer(foco == FOCO_NO_CORRER);

    for (int i = 0; i < TOTAL_PISTAS; i++) {
        desenharCartaoDePista(i, pistas[i], menu->destaquePista[i], foco == FOCO_NA_PISTA && i == menu->pistaEscolhida);
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
    LINHA_DIRECAO_GUIADA,
    LINHA_CAMERA_X,
    LINHA_CAMERA_Y,
    /* a aba de exibição começa aqui */
    LINHA_RESOLUCAO,
    LINHA_MODO_DE_TELA,
    LINHA_SINCRONIA,
    LINHA_LIMITE_DE_QUADROS,
    LINHA_MOSTRAR_FPS,
    LINHA_SOMBRAS,
    /* e esta vale nas duas abas */
    LINHA_PADRAO,
    TOTAL_LINHAS_CONFIG
};

/* As abas das configurações */
enum { ABA_CONTROLES, ABA_EXIBICAO, TOTAL_ABAS_CONFIG };
#define X_ABAS_CONFIG          330.0f
#define Y_ABAS_CONFIG          24.0f
#define LARGURA_ABA_CONFIG     150.0f
#define ALTURA_ABA_CONFIG      30.0f
#define Y_LINHAS_EXIBICAO      122.0f
#define ALTURA_LINHA_EXIBICAO  34.0f
#define SEGUNDOS_PARA_APLICAR  0.6f   /* a janela só muda depois de uns instantes sem mexer, para não piscar a cada clique */

static int abaAtualDaConfig = ABA_CONTROLES;   /* a aba que as funções abaixo estão atendendo agora */

#define Y_TITULO_COMANDOS     92.0f
#define Y_COMANDOS            114.0f
#define ALTURA_LINHA_COMANDO  29.0f
#define Y_TITULO_CONTROLE     294.0f
#define Y_OPCOES_CONTROLE     316.0f
#define ALTURA_LINHA_OPCAO    24.0f
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
    if (abaAtualDaConfig == ABA_EXIBICAO) {
        return Y_LINHAS_EXIBICAO + (linha - LINHA_RESOLUCAO) * ALTURA_LINHA_EXIBICAO + ((linha == LINHA_PADRAO) ? 14.0f : 0.0f);
    }
    int posicao = (linha == LINHA_PADRAO) ? LINHA_CAMERA_Y - TOTAL_ACOES + 1 : linha - TOTAL_ACOES;
    return Y_OPCOES_CONTROLE + posicao * ALTURA_LINHA_OPCAO;
}

/*
 * linhaNaAba
 * Recebe: a linha e a aba.
 * Devolve: 1 se a linha aparece nessa aba (a de voltar ao padrão aparece nas duas).
 */
static int linhaNaAba(int linha, int aba) {
    if (linha == LINHA_PADRAO) {
        return 1;
    }
    return (aba == ABA_EXIBICAO) ? linha >= LINHA_RESOLUCAO : linha < LINHA_RESOLUCAO;
}

/*
 * andarNaAba
 * Move a marcação entre as linhas da aba (dando a volta nas pontas).
 * Recebe: a linha marcada e a aba.
 * Devolve: a nova linha.
 */
static int andarNaAba(int linha, int aba) {
    int lista[TOTAL_LINHAS_CONFIG], total = 0, indice = 0;
    for (int i = 0; i < TOTAL_LINHAS_CONFIG; i++) {
        if (linhaNaAba(i, aba)) {
            if (i == linha) {
                indice = total;
            }
            lista[total++] = i;
        }
    }
    return lista[andarNaLista(indice, total)];
}

/*
 * alturaDaLinhaConfig
 * Recebe: a linha.
 * Devolve: a altura dela.
 */
static float alturaDaLinhaConfig(int linha) {
    if (linha < TOTAL_ACOES) {
        return ALTURA_LINHA_COMANDO;
    }
    return (abaAtualDaConfig == ABA_EXIBICAO) ? ALTURA_LINHA_EXIBICAO : ALTURA_LINHA_OPCAO;
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
        case LINHA_DIRECAO_GUIADA:
            config->direcaoGuiada = !config->direcaoGuiada;
            break;
        case LINHA_SOMBRAS: {
            /* ALTO (sombras e efeitos) -> MÉDIO (só efeitos) -> BAIXO (nada) -> ALTO */
            int nivel = config->sombras ? 2 : (config->efeitos ? 1 : 0);
            nivel = (nivel + 2) % 3;
            config->sombras = (nivel == 2);
            config->efeitos = (nivel >= 1);
            break;
        }
        case LINHA_RESOLUCAO: {
            /* anda pela tabela, pulando o que não cabe no monitor */
            int indice = config->resolucao;
            for (int tentativa = 0; tentativa < TOTAL_RESOLUCOES; tentativa++) {
                indice = (indice + sentido + TOTAL_RESOLUCOES) % TOTAL_RESOLUCOES;
                if (resolucaoCabeNoMonitor(indice)) {
                    break;
                }
            }
            config->resolucao = indice;
            menu->segundosParaAplicar = SEGUNDOS_PARA_APLICAR;
            break;
        }
        case LINHA_MODO_DE_TELA:
            config->modoDeTela = (config->modoDeTela + sentido + TOTAL_MODOS_DE_TELA) % TOTAL_MODOS_DE_TELA;
            menu->segundosParaAplicar = SEGUNDOS_PARA_APLICAR;
            break;
        case LINHA_SINCRONIA:
            config->sincroniaVertical = !config->sincroniaVertical;
            aplicarDesempenho(config);
            break;
        case LINHA_LIMITE_DE_QUADROS:
            config->limiteDeQuadros = (config->limiteDeQuadros + sentido + TOTAL_LIMITES_DE_QUADROS) % TOTAL_LIMITES_DE_QUADROS;
            aplicarDesempenho(config);
            break;
        case LINHA_MOSTRAR_FPS:
            config->mostrarFps = !config->mostrarFps;
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
            aplicarDesempenho(configuracoesAtuais());
            aplicarTela(configuracoesAtuais());
            menu->segundosParaAplicar = 0.0f;
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
    abaAtualDaConfig = menu->abaConfig;

    /* A janela muda quando o jogador para de mexer na resolução e no modo de tela */
    if (menu->segundosParaAplicar > 0.0f) {
        menu->segundosParaAplicar -= GetFrameTime();
        if (menu->segundosParaAplicar <= 0.0f) {
            aplicarTela(configuracoesAtuais());
            menu->segundosParaAplicar = 0.0f;
        }
    }

    /* Trocar de aba: TAB, os gatilhos LB e RB, ou clicar nela */
    float abaMouseX, abaMouseY;
    posicaoDoMouseNaInterface(&abaMouseX, &abaMouseY);
    int novaAba = menu->abaConfig;
    if (IsKeyPressed(KEY_TAB) || controleApertou(BOTAO_ABA_SEGUINTE) || controleApertou(BOTAO_ABA_ANTERIOR)) {
        novaAba = (menu->abaConfig + 1) % TOTAL_ABAS_CONFIG;
    }
    for (int aba = 0; aba < TOTAL_ABAS_CONFIG; aba++) {
        Rectangle r = { X_ABAS_CONFIG + aba * (LARGURA_ABA_CONFIG + 10.0f), Y_ABAS_CONFIG, LARGURA_ABA_CONFIG, ALTURA_ABA_CONFIG };
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && dentroDoRetangulo(abaMouseX, abaMouseY, r.x, r.y, r.width, r.height)) {
            novaAba = aba;
        }
    }
    if (novaAba != menu->abaConfig) {
        menu->abaConfig = novaAba;
        abaAtualDaConfig = novaAba;
        menu->linhaConfig = (novaAba == ABA_EXIBICAO) ? LINHA_RESOLUCAO : 0;
        menu->colunaConfig = 0;
        menu->confirmarPadrao = 0.0f;
        tocarSom(SOM_CURSOR);
    }

    int anterior = menu->linhaConfig, colunaAnterior = menu->colunaConfig;
    menu->linhaConfig = andarNaAba(menu->linhaConfig, menu->abaConfig);
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
        if (linhaNaAba(i, menu->abaConfig) && dentroDoRetangulo(mouseX, mouseY, X_LINHAS_CONFIG, yDaLinhaConfig(i), LARGURA_LINHAS_CONFIG, alturaDaLinhaConfig(i))) {
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
        if (menu->segundosParaAplicar > 0.0f) {   /* saiu antes da hora: a janela muda agora */
            aplicarTela(configuracoesAtuais());
            menu->segundosParaAplicar = 0.0f;
        }
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
    escrever(FONTE_TITULO, texto, X_LINHAS_CONFIG + 10.0f, y, 17.0f, COR_TEXTO_CLARO);
    float fimDoTexto = larguraDoTexto(FONTE_TITULO, texto, 17.0f);
    DrawRectangleRec((Rectangle){ X_LINHAS_CONFIG + 22.0f + fimDoTexto, y + 12.0f, LARGURA_LINHAS_CONFIG - fimDoTexto - 32.0f, 1.0f },
                     Fade(COR_CROMO, 0.3f));
}

/*
 * desenharFundoDaLinhaConfig
 * Recebe: a linha e se ela está marcada.
 * Devolve: nada.
 */
static void desenharFundoDaLinhaConfig(int linha, int marcada) {
    float y = yDaLinhaConfig(linha), altura = alturaDaLinhaConfig(linha);
    Rectangle fundo = { X_LINHAS_CONFIG, y, LARGURA_LINHAS_CONFIG, altura - 2.0f };
    float raio = raioDoTamanho(fundo.width, fundo.height);
    caixaArredondada(fundo, raio, marcada ? Fade((Color){ 46, 50, 62, 255 }, 0.95f) : Fade(COR_GRAFITE, 0.80f));
    if (marcada && linha >= TOTAL_ACOES) {
        contornoArredondado(fundo, raio, 2.0f, corDoCursor());
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
    float dy = (alturaDaLinhaConfig(linha) - 18.0f) / 2.0f;   /* o texto no meio da linha */
    escrever(FONTE_FORTE, nome, X_LINHAS_CONFIG + 12.0f, y + dy, 15.0f, corDoTexto);
    if (linha != LINHA_PADRAO) {   /* as setas "< >" ficam sempre ao lado do valor; brancas na linha marcada */
        Color corDasSetas = marcada ? COR_TEXTO_CLARO : Fade(COR_TEXTO_CLARO, 0.45f);
        escrever(FONTE_FORTE, "<", X_VALOR_OPCAO, y + dy, 15.0f, corDasSetas);
        escrever(FONTE_FORTE, ">", X_VALOR_OPCAO + 16.0f, y + dy, 15.0f, corDasSetas);
    }
    escrever(FONTE_FORTE, valor, X_VALOR_OPCAO + 44.0f, y + dy, 15.0f, apagada ? COR_TEXTO_FRACO : COR_TEXTO_CLARO);
}

/*
 * desenharConfiguracoes
 * Recebe: o estado do menu.
 * Devolve: nada.
 */
/*
 * desenharAbasDaConfig
 * As duas abas ao lado do título: CONTROLES e EXIBIÇÃO.
 * Recebe: a aba aberta.
 * Devolve: nada.
 */
static void desenharAbasDaConfig(int aberta) {
    static const char *NOMES[TOTAL_ABAS_CONFIG] = { "CONTROLES", "EXIBIÇÃO" };
    float mouseX, mouseY;
    posicaoDoMouseNaInterface(&mouseX, &mouseY);
    for (int aba = 0; aba < TOTAL_ABAS_CONFIG; aba++) {
        Rectangle r = { X_ABAS_CONFIG + aba * (LARGURA_ABA_CONFIG + 10.0f), Y_ABAS_CONFIG, LARGURA_ABA_CONFIG, ALTURA_ABA_CONFIG };
        int sobre = dentroDoRetangulo(mouseX, mouseY, r.x, r.y, r.width, r.height);
        Color fundo = (aba == aberta) ? COR_VERMELHO : Fade(COR_GRAFITE, sobre ? 0.98f : 0.85f);
        caixaArredondada(r, raioDoTamanho(r.width, r.height), fundo);
        escreverCentralizado(FONTE_FORTE, NOMES[aba], r.x + r.width / 2.0f, r.y + 6.0f, 16.0f, (aba == aberta || sobre) ? WHITE : COR_CROMO);
    }
    escrever(FONTE_FORTE, controleConectado() ? "LB / RB TROCA DE ABA" : "TAB TROCA DE ABA", X_ABAS_CONFIG + 4.0f, Y_ABAS_CONFIG + ALTURA_ABA_CONFIG + 4.0f, 11.0f, COR_TEXTO_FRACO);
}

/*
 * textoDaResolucao
 * Recebe: o índice na tabela de resoluções.
 * Devolve: o texto do valor ("AUTOMÁTICA (1920 x 1080)", "1280 x 720"...).
 */
static const char *textoDaResolucao(int indice) {
    int l = 0, a = 0;
    if (indice == 0) {
        int monitorL, monitorA;
        tamanhoDoMonitor(&monitorL, &monitorA);
        resolucaoEfetiva(configuracoesAtuais(), monitorL, monitorA, &l, &a);
        return TextFormat("AUTOMÁTICA (%d x %d)", l, a);
    }
    resolucaoDaTabela(indice, &l, &a);
    return TextFormat("%d x %d", l, a);
}

void desenharConfiguracoes(EstadoMenu *menu) {
    static const char *VOLANTES[TOTAL_VOLANTES] = { "SUAVE", "NORMAL", "DIRETO" };
    const Configuracoes *config = configuracoesAtuais();
    abaAtualDaConfig = menu->abaConfig;
    escurecerTela(0.75f, 0.6f, 0.8f);
    desenharCabecalho("CONFIGURAÇÕES");
    desenharAbasDaConfig(menu->abaConfig);
    desenharBotaoVoltar(menu->destaqueVoltar);

    if (menu->abaConfig == ABA_EXIBICAO) {
        static const char *MODOS[TOTAL_MODOS_DE_TELA] = { "TELA CHEIA", "JANELA" };
        desenharTituloDeSecao("EXIBIÇÃO", Y_TITULO_COMANDOS);
        desenharLinhaDeOpcao(menu, LINHA_RESOLUCAO, "RESOLUÇÃO", textoDaResolucao(config->resolucao), 0);
        desenharLinhaDeOpcao(menu, LINHA_MODO_DE_TELA, "MODO DE TELA", MODOS[config->modoDeTela], 0);
        desenharLinhaDeOpcao(menu, LINHA_SINCRONIA, "SINCRONIA VERTICAL (VSYNC)", config->sincroniaVertical ? "LIGADA" : "DESLIGADA", 0);
        int fps = quadrosPorSegundoDoLimite(config->limiteDeQuadros);
        desenharLinhaDeOpcao(menu, LINHA_LIMITE_DE_QUADROS, "LIMITE DE QUADROS POR SEGUNDO", fps > 0 ? TextFormat("%d FPS", fps) : "SEM LIMITE", 0);
        desenharLinhaDeOpcao(menu, LINHA_MOSTRAR_FPS, "MOSTRAR O CONTADOR DE FPS", config->mostrarFps ? "SIM" : "NÃO", 0);
        desenharLinhaDeOpcao(menu, LINHA_SOMBRAS, "GRÁFICOS (SOMBRAS E EFEITOS)",
                             config->sombras ? "ALTO" : (config->efeitos ? "MÉDIO" : "BAIXO"), 0);
        desenharLinhaDeOpcao(menu, LINHA_PADRAO, "VOLTAR TUDO AO PADRÃO", (menu->confirmarPadrao > 0.0f) ? "APERTE DE NOVO" : "", 0);
        escrever(FONTE_FORTE, TextFormat("AGORA: %d x %d%s  |  A MAIOR RESOLUÇÃO É %d x %d", GetScreenWidth(), GetScreenHeight(),
                                         IsWindowFullscreen() ? " (TELA CHEIA)" : (IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE) ? " (SEM BORDAS)" : " (JANELA)"),
                                         MAIOR_LARGURA, MAIOR_ALTURA),
                 X_LINHAS_CONFIG + 12.0f, Y_LINHAS_EXIBICAO + 8.0f * ALTURA_LINHA_EXIBICAO + 8.0f, 13.0f, COR_TEXTO_FRACO);
        return;
    }

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
    desenharLinhaDeOpcao(menu, LINHA_DIRECAO_GUIADA, "DIREÇÃO GUIADA", config->direcaoGuiada ? "LIGADA" : "DESLIGADA", 0);
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
        "Na grama (sai poeira) o carro não passa de 75 km/h. Touchpad (PlayStation) ou C: câmera perto, média ou longe.",
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
    usarFormasRetas(1);   /* desenhada dentro da corrida, onde as formas retas ficam desligadas */
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
    usarFormasRetas(0);
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
