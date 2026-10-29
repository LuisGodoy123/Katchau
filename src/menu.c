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
 * ------------------------------------------------------------------
 */

#include <stdio.h>
#include "raylib.h"
#include "menu.h"
#include "render.h"
#include "hud.h"

/* ---------- Cores e tamanhos comuns ---------- */
#define COR_DESTAQUE          YELLOW
#define COR_TEXTO             RAYWHITE
#define COR_TEXTO_APAGADO     LIGHTGRAY
#define COR_PAINEL            Fade(BLACK, 0.65f)
#define COR_LINHA_MARCADA     Fade(YELLOW, 0.25f)
#define COR_BARRA_FUNDO       Fade(RAYWHITE, 0.2f)
#define TAMANHO_DICA          18
#define Y_DICA                568

/* ---------- Menu principal ---------- */
#define TOTAL_OPCOES          4
#define TAMANHO_LOGO          96
#define TAMANHO_OPCAO         28
#define ESPACO_OPCOES         48
#define Y_PRIMEIRA_OPCAO      250

/* ---------- Garagem ---------- */
#define Y_PRIMEIRA_LINHA      122
#define ALTURA_LINHA          52
#define X_LISTA               24
#define LARGURA_LISTA         404
#define X_BARRA_LISTA         175
#define LARGURA_BARRA_LISTA   140
#define X_FICHA               448
#define LARGURA_FICHA         328
#define LARGURA_BARRA_FICHA   150
#define FRACAO_MINIMA_BARRA   0.15f   /* o pior carro ainda mostra um pedacinho de barra */
#define UNIDADES_EM_100_KMH   4000.0f /* 100 km/h em unidades por segundo (KMH_POR_UNIDADE = 0,025) */

/* ---------- Resultado ---------- */
#define Y_CABECALHO           112
#define ALTURA_LINHA_RESULTADO 42
#define X_COLUNA_POSICAO      70
#define X_COLUNA_PILOTO       140
#define X_COLUNA_TEMPO        430
#define X_COLUNA_MELHOR       590

static const char *TEXTOS_OPCOES[TOTAL_OPCOES] = { "CORRER", "GARAGEM", "COMO JOGAR", "SAIR" };
static const Tela DESTINOS_OPCOES[TOTAL_OPCOES] = { TELA_CORRIDA, TELA_GARAGEM, TELA_COMO_JOGAR, TELA_SAIR };

/* ==================================================================
 * Funções auxiliares
 * ================================================================== */

/*
 * desenharTextoCentralizado
 * Escreve um texto centralizado na horizontal, com sombra preta.
 * Recebe: o texto, a altura (y), o tamanho da fonte e a cor.
 * Devolve: nada.
 */
static void desenharTextoCentralizado(const char *texto, int y, int tamanho, Color cor) {
    int x = (GetScreenWidth() - MeasureText(texto, tamanho)) / 2;
    DrawText(texto, x + 2, y + 2, tamanho, BLACK);
    DrawText(texto, x, y, tamanho, cor);
}

/*
 * desenharDica
 * Linha de ajuda com os controles, no pé da tela.
 * Recebe: o texto.
 * Devolve: nada.
 */
static void desenharDica(const char *texto) {
    DrawRectangle(0, Y_DICA - 8, GetScreenWidth(), GetScreenHeight() - Y_DICA + 8, COR_PAINEL);
    desenharTextoCentralizado(texto, Y_DICA, TAMANHO_DICA, COR_TEXTO_APAGADO);
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
    menu->carroEscolhido = 0;
    menu->criterio = ATRIBUTO_VELOCIDADE;
    for (int numero = 0; numero < TOTAL_CARROS; numero++) {
        menu->garagem[numero] = fichaDoCarro(numero);
    }
    ordenarFichasPorAtributo(menu->garagem, TOTAL_CARROS, menu->criterio);
    menu->selecionado = 0;
}

/*
 * atualizarMenuPrincipal
 * Cima/baixo trocam a opção; ENTER entra nela.
 * Recebe: o estado do menu.
 * Devolve: a próxima tela.
 */
Tela atualizarMenuPrincipal(EstadoMenu *menu) {
    menu->opcaoMenu = andarNaLista(menu->opcaoMenu, TOTAL_OPCOES);
    if (apertouConfirmar()) {
        return DESTINOS_OPCOES[menu->opcaoMenu];
    }
    return TELA_MENU;
}

/*
 * desenharMenuPrincipal
 * Logo, opções e o carro escolhido, com a pista passando ao fundo.
 * Recebe: o estado do menu e a pista (para o fundo).
 * Devolve: nada.
 */
void desenharMenuPrincipal(EstadoMenu *menu, Pista *pista) {
    desenharPistaDeFundo(pista);
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.3f));

    desenharTextoCentralizado("KATCHAU", 60, TAMANHO_LOGO, COR_DESTAQUE);
    desenharTextoCentralizado("corrida em Interlagos", 160, 22, COR_TEXTO);

    /* Painel das opções */
    int larguraPainel = 340;
    int alturaPainel = TOTAL_OPCOES * ESPACO_OPCOES + 24;
    int xPainel = (GetScreenWidth() - larguraPainel) / 2;
    DrawRectangleRounded((Rectangle){ xPainel, Y_PRIMEIRA_OPCAO - 16, larguraPainel, alturaPainel },
                         0.15f, 8, COR_PAINEL);

    for (int i = 0; i < TOTAL_OPCOES; i++) {
        int y = Y_PRIMEIRA_OPCAO + i * ESPACO_OPCOES;
        if (i == menu->opcaoMenu) {
            DrawRectangle(xPainel + 12, y - 8, larguraPainel - 24, ESPACO_OPCOES - 6, COR_LINHA_MARCADA);
            desenharTextoCentralizado(TextFormat("> %s <", TEXTOS_OPCOES[i]), y, TAMANHO_OPCAO, COR_DESTAQUE);
        } else {
            desenharTextoCentralizado(TEXTOS_OPCOES[i], y, TAMANHO_OPCAO, COR_TEXTO);
        }
    }

    /* Carro escolhido, embaixo, para lembrar com qual o jogador vai correr */
    const FichaTecnica *ficha = fichaDoCarro(menu->carroEscolhido);
    desenharTextoCentralizado(TextFormat("Seu carro: %s", ficha->nome), 462, 20, COR_TEXTO);
    desenharCarroParado(menu->carroEscolhido, GetScreenWidth() / 2.0f, 552, 0.8f);

    desenharDica("CIMA/BAIXO: escolher     ENTER: confirmar");
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
 * atualizarGaragem
 * Cima/baixo marcam um carro; esquerda/direita trocam o critério de
 * ordenação; ENTER escolhe o carro marcado; ESC volta ao menu.
 * Recebe: o estado do menu.
 * Devolve: a próxima tela.
 */
Tela atualizarGaragem(EstadoMenu *menu) {
    menu->selecionado = andarNaLista(menu->selecionado, TOTAL_CARROS);

    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
        menu->criterio = (menu->criterio + 1) % TOTAL_ATRIBUTOS;
        reordenarGaragem(menu);
    }
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
        menu->criterio = (menu->criterio + TOTAL_ATRIBUTOS - 1) % TOTAL_ATRIBUTOS;
        reordenarGaragem(menu);
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
 *   velocidade -> km/h; aceleração -> segundos de 0 a 100 km/h;
 *   aderência -> porcentagem.
 * Recebe: a ficha e o atributo.
 * Devolve: o texto (válido até a próxima chamada de TextFormat).
 */
static const char *textoDoAtributo(const FichaTecnica *ficha, Atributo atributo) {
    switch (atributo) {
        case ATRIBUTO_VELOCIDADE:
            return TextFormat("%d km/h", converterParaKmh(ficha->velocidadeMaxima));
        case ATRIBUTO_ACELERACAO: {
            /* Com vírgula (1,43 s): o ponto da fonte da raylib quase some */
            int centesimos = (int) (UNIDADES_EM_100_KMH / ficha->aceleracao * 100 + 0.5f);
            return TextFormat("%d,%02d s", centesimos / 100, centesimos % 100);
        }
        case ATRIBUTO_ADERENCIA:
            return TextFormat("%d%%", (int) (ficha->aderencia * 100 + 0.5f));
        default:
            return "";
    }
}

/*
 * desenharBarra
 * Barra horizontal de atributo: fundo apagado e parte cheia colorida.
 * Recebe: posição, largura total, altura, fração cheia e cor.
 * Devolve: nada.
 */
static void desenharBarra(int x, int y, int largura, int altura, float fracao, Color cor) {
    DrawRectangle(x, y, largura, altura, COR_BARRA_FUNDO);
    DrawRectangle(x, y, (int) (largura * fracao), altura, cor);
}

/*
 * desenharAbasDeCriterio
 * Os três critérios lado a lado; o ativo fica destacado.
 * Recebe: o critério ativo.
 * Devolve: nada.
 */
static void desenharAbasDeCriterio(Atributo criterio) {
    int y = 74;
    int x = X_LISTA;
    DrawText("ORDENAR POR:", x, y + 4, 18, COR_TEXTO_APAGADO);
    x += MeasureText("ORDENAR POR:", 18) + 16;

    for (int atributo = 0; atributo < TOTAL_ATRIBUTOS; atributo++) {
        const char *nome = nomeDoAtributo((Atributo) atributo);
        int largura = MeasureText(nome, 20) + 20;
        if (atributo == (int) criterio) {
            DrawRectangle(x, y, largura, 28, COR_DESTAQUE);
            DrawText(nome, x + 10, y + 4, 20, BLACK);
        } else {
            DrawRectangleLines(x, y, largura, 28, COR_TEXTO_APAGADO);
            DrawText(nome, x + 10, y + 4, 20, COR_TEXTO);
        }
        x += largura + 10;
    }
    DrawText("(Merge Sort)", x + 6, y + 6, 16, COR_TEXTO_APAGADO);
}

/*
 * desenharListaDaGaragem
 * As 8 fichas, na ordem do Merge Sort, com a barra do critério atual.
 * Recebe: o estado do menu.
 * Devolve: nada.
 */
static void desenharListaDaGaragem(EstadoMenu *menu) {
    DrawRectangle(X_LISTA - 8, Y_PRIMEIRA_LINHA - 8, LARGURA_LISTA + 16,
                  TOTAL_CARROS * ALTURA_LINHA + 8, COR_PAINEL);

    for (int i = 0; i < TOTAL_CARROS; i++) {
        const FichaTecnica *ficha = menu->garagem[i];
        int y = Y_PRIMEIRA_LINHA + i * ALTURA_LINHA;
        int ehMarcado = (i == menu->selecionado);
        int ehEscolhido = (numeroDaFicha(ficha) == menu->carroEscolhido);

        if (ehMarcado) {
            DrawRectangle(X_LISTA - 4, y - 4, LARGURA_LISTA + 8, ALTURA_LINHA - 4, COR_LINHA_MARCADA);
        }

        Color corNome = ehMarcado ? COR_DESTAQUE : COR_TEXTO;
        DrawText(TextFormat("%d. %s", i + 1, ficha->nome), X_LISTA + 6, y + 4, 20, corNome);
        if (ehEscolhido) {
            DrawText("SEU CARRO", X_LISTA + 6, y + 26, 14, COR_DESTAQUE);
        }

        desenharBarra(X_BARRA_LISTA, y + 8, LARGURA_BARRA_LISTA, 14,
                      fracaoDoAtributo(ficha, menu->criterio), corNome);
        DrawText(textoDoAtributo(ficha, menu->criterio),
                 X_BARRA_LISTA + LARGURA_BARRA_LISTA + 10, y + 6, 18, COR_TEXTO);
    }
}

/*
 * desenharFichaMarcada
 * Painel da direita: o carro marcado em tamanho grande e os três atributos.
 * Recebe: o estado do menu.
 * Devolve: nada.
 */
static void desenharFichaMarcada(EstadoMenu *menu) {
    const FichaTecnica *ficha = menu->garagem[menu->selecionado];
    int numero = numeroDaFicha(ficha);

    DrawRectangle(X_FICHA - 8, Y_PRIMEIRA_LINHA - 8, LARGURA_FICHA + 16,
                  TOTAL_CARROS * ALTURA_LINHA + 8, COR_PAINEL);

    float centroX = X_FICHA + LARGURA_FICHA / 2.0f;
    desenharCarroParado(numero, centroX, 260, 1.3f);

    int y = 278;
    int largura = MeasureText(ficha->nome, 32);
    DrawText(ficha->nome, (int) (centroX - largura / 2.0f), y, 32, COR_DESTAQUE);
    largura = MeasureText(ficha->descricao, 18);
    DrawText(ficha->descricao, (int) (centroX - largura / 2.0f), y + 40, 18, COR_TEXTO);

    y += 82;
    for (int atributo = 0; atributo < TOTAL_ATRIBUTOS; atributo++) {
        int linha = y + atributo * 46;
        Color cor = (atributo == (int) menu->criterio) ? COR_DESTAQUE : COR_TEXTO;
        DrawText(nomeDoAtributo((Atributo) atributo), X_FICHA + 8, linha, 16, cor);
        desenharBarra(X_FICHA + 8, linha + 20, LARGURA_BARRA_FICHA, 12,
                      fracaoDoAtributo(ficha, (Atributo) atributo), cor);
        DrawText(textoDoAtributo(ficha, (Atributo) atributo),
                 X_FICHA + 18 + LARGURA_BARRA_FICHA, linha + 16, 18, COR_TEXTO);
    }
    DrawText("aceleracao: segundos de 0 a 100 km/h", X_FICHA + 8, y + 3 * 46 - 4, 14, COR_TEXTO_APAGADO);
}

/*
 * desenharGaragem
 * Recebe: o estado do menu e a pista (para o fundo).
 * Devolve: nada.
 */
void desenharGaragem(EstadoMenu *menu, Pista *pista) {
    desenharPistaDeFundo(pista);
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.45f));

    DrawText("GARAGEM", X_LISTA + 2, 22, 40, BLACK);
    DrawText("GARAGEM", X_LISTA, 20, 40, COR_DESTAQUE);

    desenharAbasDeCriterio(menu->criterio);
    desenharListaDaGaragem(menu);
    desenharFichaMarcada(menu);

    desenharDica("CIMA/BAIXO: carro   ESQ/DIR: ordenar   ENTER: escolher   ESC: voltar");
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
 * desenharComoJogar
 * Controles e dicas.
 * Recebe: a pista (para o fundo).
 * Devolve: nada.
 */
void desenharComoJogar(Pista *pista) {
    desenharPistaDeFundo(pista);
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.45f));

    desenharTextoCentralizado("COMO JOGAR", 30, 40, COR_DESTAQUE);

    int x = 110;
    DrawRectangleRounded((Rectangle){ x - 30, 96, GetScreenWidth() - 2 * (x - 30), 440 }, 0.06f, 8, COR_PAINEL);

    const char *teclas[] = { "CIMA ou W", "BAIXO ou S", "ESQ/DIR ou A/D", "ESC" };
    const char *acoes[] = { "acelerar", "frear", "virar", "abandonar a corrida" };
    for (int i = 0; i < 4; i++) {
        DrawText(teclas[i], x, 120 + i * 36, 22, COR_DESTAQUE);
        DrawText(acoes[i], x + 240, 120 + i * 36, 22, COR_TEXTO);
    }

    const char *dicas[] = {
        TextFormat("- %d voltas em Interlagos contra 7 rivais.", TOTAL_VOLTAS),
        "  Voce larga em ultimo.",
        "- Curva fechada em alta velocidade joga o carro",
        "  para fora: freie antes de entrar nela.",
        "- Na grama o carro nao passa de 75 km/h.",
        "- Bater na traseira de outro carro tira velocidade.",
        "- Na garagem, compare velocidade, aceleracao",
        "  e aderencia antes de escolher o seu carro.",
    };
    for (int i = 0; i < 8; i++) {
        DrawText(dicas[i], x, 280 + i * 30, 20, COR_TEXTO);
    }

    desenharDica("ENTER ou ESC: voltar ao menu");
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
 * desenharLinhaDoResultado
 * Uma linha da tabela: posição, piloto, tempo total e melhor volta.
 * Quem ainda não chegou mostra em que volta está.
 * Recebe: a corrida, a posição (0 = 1º lugar) e a altura da linha.
 * Devolve: nada.
 */
static void desenharLinhaDoResultado(Corrida *corrida, int posicao, int y) {
    Carro *carro = corrida->classificacao[posicao];
    int ehJogador = (carro == &corrida->jogador);
    Color cor = ehJogador ? COR_DESTAQUE : COR_TEXTO;
    char tempo[32];

    if (ehJogador) {
        DrawRectangle(X_COLUNA_POSICAO - 16, y - 8, GetScreenWidth() - 2 * (X_COLUNA_POSICAO - 16),
                      ALTURA_LINHA_RESULTADO - 4, COR_LINHA_MARCADA);
    }

    DrawText(TextFormat("%do", posicao + 1), X_COLUNA_POSICAO, y, 22, cor);
    DrawText(ehJogador ? TextFormat("%s (VOCE)", carro->nome) : carro->nome,
             X_COLUNA_PILOTO, y, 22, cor);

    if (carro->voltas >= TOTAL_VOLTAS) {
        formatarTempo(carro->tempoDeChegada, tempo, sizeof(tempo));
        DrawText(tempo, X_COLUNA_TEMPO, y, 22, cor);
    } else {
        DrawText(TextFormat("volta %d/%d", voltaAtual(carro), TOTAL_VOLTAS),
                 X_COLUNA_TEMPO, y, 22, COR_TEXTO_APAGADO);
    }

    float melhor = melhorVolta(carro);
    if (melhor > 0.0f) {
        formatarTempo(melhor, tempo, sizeof(tempo));
        DrawText(tempo, X_COLUNA_MELHOR, y, 22, cor);
    } else {
        DrawText("-", X_COLUNA_MELHOR, y, 22, COR_TEXTO_APAGADO);
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
    desenharVisaoDoJogador(corrida);
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.6f));

    if (corrida->posicaoFinal == 1) {
        desenharTextoCentralizado("KATCHAU! VOCE VENCEU!", 28, 40, COR_DESTAQUE);
    } else {
        desenharTextoCentralizado(TextFormat("VOCE CHEGOU EM %do", corrida->posicaoFinal), 28, 40, COR_DESTAQUE);
    }

    int y = Y_CABECALHO - 30;
    DrawText("POS", X_COLUNA_POSICAO, y, 18, COR_TEXTO_APAGADO);
    DrawText("PILOTO", X_COLUNA_PILOTO, y, 18, COR_TEXTO_APAGADO);
    DrawText("TEMPO", X_COLUNA_TEMPO, y, 18, COR_TEXTO_APAGADO);
    DrawText("MELHOR VOLTA", X_COLUNA_MELHOR, y, 18, COR_TEXTO_APAGADO);

    for (int posicao = 0; posicao < TOTAL_CARROS; posicao++) {
        desenharLinhaDoResultado(corrida, posicao, Y_CABECALHO + posicao * ALTURA_LINHA_RESULTADO);
    }

    if (!todosChegaram(corrida)) {
        desenharTextoCentralizado("os outros carros ainda estao terminando a prova...", 462, 18,
                                  COR_TEXTO_APAGADO);
    }

    desenharDica("ENTER: correr de novo     ESC: menu");
}
