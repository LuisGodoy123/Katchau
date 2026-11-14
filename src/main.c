/*
 * main.c
 * ------------------------------------------------------------------
 * Ponto de entrada do KATCHAU.
 * Abre a janela e troca entre as telas: menu, garagem, como jogar, o
 * mundo aberto (passeio.c), a escolha da corrida, a corrida e o
 * resultado. A variável "tela" diz em qual delas o jogador está; cada
 * tela devolve para qual ir no quadro seguinte.
 *
 * O jogo acontece no mundo aberto: JOGAR leva à cidade, a escolha da
 * corrida abre no Mack, o Mack leva o carro até a pista, e no fim da
 * corrida (ou saindo dela pela pausa) o carro volta para a cidade.
 *
 * O laço principal segue sempre a mesma ordem, uma vez por quadro:
 *   1) ler o teclado e atualizar a tela atual (lógica);
 *   2) desenhar a tela atual (render.c, hud.c, menu.c).
 * ------------------------------------------------------------------
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "raylib.h"
#include "config.h"
#include "pista.h"
#include "mundo.h"
#include "carros3d.h"
#include "fumaca.h"
#include "fogos.h"
#include "marcas.h"
#include "posprocesso.h"
#include "carro.h"
#include "corrida.h"
#include "pitstop.h"
#include "render.h"
#include "hud.h"
#include "menu.h"
#include "vibrar.h"
#include "configuracoes.h"
#include "visual.h"
#include "audio.h"
#include "luzes.h"
#include "efeitos.h"
#include "exibicao.h"
#include "apresentacao.h"
#include "intro.h"
#include "videofundo.h"
#include "recordes.h"
#include "controle.h"
#include "passeio.h"
#include "espera.h"
#include <ctype.h>
#include <math.h>

/* Na vitória, o painel de chegada espera um pouco para o jogador ver o
 * confete e o refrão entrando */
#define SEGUNDOS_ANTES_DE_SEGUIR 1.5f   /* depois da bandeirada */
#define FORCA_DO_TOQUE 0.7f   /* o toque de teste da vibração, nas configurações */
#define ATRASO_DO_AVISO_NA_FESTA  4.0f   /* segundos depois de cruzar a linha, no pódio, até aparecer o "aperte qualquer botão" (a festa passa limpa) */

/*
 * registrarTelemetria
 * Com a variável de ambiente KATCHAU_TELEMETRIA ligada, grava em telemetria.csv o que o carro
 * do jogador fez em cada quadro da corrida, para investigar como ele se comporta de verdade.
 * Recebe: a corrida e os comandos do quadro.
 * Devolve: nada.
 */
static void registrarTelemetria(const Corrida *corrida, const Comandos *comandos) {
    static FILE *arquivo = NULL;
    static int decidiu = 0;
    if (!decidiu) {
        decidiu = 1;
        if (getenv("KATCHAU_TELEMETRIA") != NULL) {
            arquivo = fopen("telemetria.csv", "w");
            if (arquivo != NULL) {
                fprintf(arquivo, "t,pista,livre,m,lateral,acelerar,frear,esq,dir,analogico,volante,kmh,rumo,rumoPista,desvio,muro,fora,pit,x,z\n");
            }
        }
    }
    if (arquivo == NULL || corrida->contagemRegressiva > 0.0f) {
        return;
    }
    const Carro *j = &corrida->jogador;
    PontoNoMundo p = pontoNoMundo(j->posicao, j->deslocamentoLateral);
    fprintf(arquivo, "%.3f,%d,%d,%.1f,%.3f,%d,%d,%d,%d,%.3f,%.3f,%d,%.1f,%.1f,%.1f,%d,%d,%d,%.2f,%.2f\n",
            corrida->tempoDeCorrida, corrida->pista->id, j->direcaoLivre, metrosNaVolta(j), j->deslocamentoLateral,
            comandos->acelerar, comandos->frear, comandos->esquerda, comandos->direita, comandos->direcao, j->volante,
            converterParaKmh(j->velocidade), j->rumoNoMundo, p.rumo, j->desvioDoRumo, j->raspandoNoMuro, estaForaDaPista((Carro *) j), j->estadoDoPit, p.x, p.z);
    fflush(arquivo);
}

/*
 * medirQuadro
 * Com a variável de ambiente KATCHAU_QUADROS ligada, guarda o tempo dos últimos quadros e
 * escreve no log o tempo médio, o do 1% pior (o percentil 99) e o maior. Serve para comparar
 * o desempenho antes e depois de uma mudança: o contador de FPS da tela mostra uma média
 * suavizada, que esconde justamente os trancos.
 *
 * Só guarda na memória: gravar em arquivo a cada quadro entraria na conta do próprio quadro.
 * Para medir de verdade, desligue o VSync e o limite de quadros nas configurações, senão o
 * tempo medido é o do monitor, não o do jogo.
 * Recebe: o tempo do quadro, e 1 para escrever o resumo agora (no fim do jogo).
 * Devolve: nada.
 */
#define QUADROS_MEDIDOS   1800    /* uns 30 segundos a 60 quadros por segundo */
#define SEGUNDOS_DO_RESUMO  5.0f

static void medirQuadro(float tempoQuadro, int agora) {
    static float tempos[QUADROS_MEDIDOS];
    static int total = 0, proximo = 0, ligado = -1;
    static float desdeOResumo = 0.0f;
    if (ligado < 0) {
        ligado = (getenv("KATCHAU_QUADROS") != NULL);
    }
    if (!ligado) {
        return;
    }
    if (tempoQuadro > 0.0f) {
        tempos[proximo] = tempoQuadro;
        proximo = (proximo + 1) % QUADROS_MEDIDOS;
        if (total < QUADROS_MEDIDOS) {
            total++;
        }
        desdeOResumo += tempoQuadro;
    }
    if (total == 0 || (!agora && desdeOResumo < SEGUNDOS_DO_RESUMO)) {
        return;
    }
    desdeOResumo = 0.0f;

    /* Ordenar uma cópia: o percentil 99 é o quadro que só 1% dos quadros supera */
    static float ordenados[QUADROS_MEDIDOS];
    double soma = 0.0;
    for (int i = 0; i < total; i++) {
        ordenados[i] = tempos[i];
        soma += tempos[i];
    }
    for (int i = 1; i < total; i++) {   /* inserção: poucos milhares de itens, uma vez a cada 5 segundos */
        float guardado = ordenados[i];
        int j = i - 1;
        while (j >= 0 && ordenados[j] > guardado) {
            ordenados[j + 1] = ordenados[j];
            j--;
        }
        ordenados[j + 1] = guardado;
    }
    float media = (float) (soma / total);
    TraceLog(LOG_INFO, "QUADROS: %d medidos | media %.2f ms (%.0f FPS) | 1%% pior %.2f ms | maior %.2f ms",
             total, media * 1000.0f, media > 0.0f ? 1.0f / media : 0.0f,
             ordenados[(int) (total * 0.99f)] * 1000.0f, ordenados[total - 1] * 1000.0f);
}

/*
 * marcarEtapa
 * Escreve no log quanto tempo levou cada etapa do carregamento (desde a anterior).
 * Recebe: o nome da etapa. Devolve: nada.
 */
static void marcarEtapa(const char *nome) {
    static double anterior = -1.0;
    double agora = GetTime();
    if (anterior < 0.0) {
        anterior = 0.0;
    }
    TraceLog(LOG_INFO, "CARGA: %-22s %6.2f s (total %.2f s)", nome, agora - anterior, agora);
    anterior = agora;
}

/*
 * atualizarTelaDaCorrida
 * Um quadro da corrida: o jogador dirige, a IA dirige os rivais.
 * ESC (ou START) pausa: a corrida e os sons congelam e o jogador escolhe
 * entre continuar e voltar ao menu principal. Antes da contagem passa a
 * apresentação da largada (apresentacao.c). O touchpad (ou C, e V para o
 * jogador 2 na tela dividida) troca a distância da câmera. Depois da chegada
 * dos dois, ENTER (ou A) vai ao resultado.
 * Recebe: a corrida, o estado do menu (para a pausa), se a corrida está
 *         pausada (é alterado aqui), a apresentação, de onde vêm os comandos de cada jogador,
 *         as visões (a câmera de cada um) e o tempo do quadro.
 * Devolve: a próxima tela.
 */
static Tela atualizarTelaDaCorrida(Corrida *corrida, EstadoMenu *menu, int *pausada, Apresentacao *apresentacao,
                                   const EntradaDoJogador entradas[], VisaoDoJogador visoes[],
                                   float tempoQuadro) {
    if (*pausada) {
        EscolhaDaPausa escolha = atualizarPausa(menu);
        if (escolha == PAUSA_CONTINUAR) {
            *pausada = 0;
            pausarSonsDaCorrida(0);
        } else if (escolha == PAUSA_SAIR) {
            *pausada = 0;   /* os sons pausados somem sozinhos na volta à cidade */
            return TELA_PASSEIO;
        }
        return TELA_CORRIDA;
    }

    int pediuPausa = IsKeyPressed(KEY_ESCAPE) || controleApertou(BOTAO_START);
    for (int v = 1; v < corrida->totalDeHumanos; v++) {
        pediuPausa = pediuPausa || controleDoJogadorApertou(&entradas[v], BOTAO_START);   /* o jogador 2 também pausa */
    }
    if (pediuPausa) {
        tocarSom(SOM_CONFIRMAR);
        *pausada = 1;
        abrirPausaDaCorrida(menu);
        pausarSonsDaCorrida(1);
        return TELA_CORRIDA;
    }

    /* A apresentação da largada: o jogo de câmeras passa com a corrida parada (a contagem só
     * começa depois); ENTER (ou o botão de confirmar) pula */
    if (apresentacaoAtiva(apresentacao)) {
        int pular = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_SPACE) ||
                    controleApertou(BOTAO_CONFIRMAR) || getenv("KATCHAU_CORRIDA") != NULL;   /* o teste automático pula */
        for (int v = 1; v < corrida->totalDeHumanos; v++) {
            pular = pular || controleDoJogadorApertou(&entradas[v], BOTAO_CONFIRMAR);
        }
        atualizarApresentacao(apresentacao, pular, tempoQuadro);
        return TELA_CORRIDA;
    }

    /* Touchpad (ou C): a câmera vai de perto para médio, longe e volta. Na tela dividida, cada
     * jogador troca a dele: o 1 no C ou no controle dele, o 2 no V ou no controle dele */
    for (int v = 0; v < corrida->totalDeHumanos; v++) {
        int tecla = (v == 0) ? KEY_C : KEY_V;
        if (IsKeyPressed(tecla) || controleDoJogadorApertou(&entradas[v], BOTAO_CAMERA)) {
            alternarDistanciaDaVisao(&visoes[v]);
            tocarSom(SOM_CURSOR);
        }
    }

    /* Os comandos de cada jogador. O do jogador 1 entra por parâmetro, como sempre; os outros,
     * pelo vetor que atualizarCorrida lê */
    Comandos comandos;
    lerComandosDoJogador(&entradas[0], &comandos);
    for (int h = 1; h < corrida->totalDeHumanos; h++) {
        lerComandosDoJogador(&entradas[h], &corrida->comandosDosHumanos[h]);
    }
    atualizarCorrida(corrida, comandos, tempoQuadro);
    /* O recorde da pista: quando o jogador termina, a melhor volta dele entra no recorde, se for a
     * melhor de todas. Na tela dividida não vale: a corrida se ajusta a dois humanos, o trânsito é
     * outro e a volta não é comparável com a tabela de um jogador */
    static int recordeRegistrado = 0;
    if (corrida->contagemRegressiva > 0.0f) {
        recordeRegistrado = 0;
    } else if (corrida->terminou && !recordeRegistrado) {
        recordeRegistrado = 1;
        if (corrida->totalDeHumanos == 1 && !corrida->desclassificado && corrida->jogador.melhorVolta > 0.0f) {
            registrarRecorde(corrida->pista->id, corrida->jogador.melhorVolta, corrida->jogador.nome);
        }
    }
    registrarTelemetria(corrida, &comandos);
    /* Espera um pouco: o botão de confirmar é também o freio de mão, e quem o aperta
     * na bandeirada não pode pular a tela da chegada sem querer */
    /* A espera conta do ÚLTIMO que chegou: na tela dividida, quem terminou primeiro não leva os
     * dois para a tela de resultado enquanto o outro ainda está na pista */
    float ultimaChegada = corrida->jogador.tempoDeChegada;
    for (int h = 1; h < corrida->totalDeHumanos; h++) {
        ultimaChegada = fmaxf(ultimaChegada, corrida->humanos[h]->tempoDeChegada);
    }
    int podeSeguir = todosOsHumanosChegaram(corrida)
                     && corrida->tempoDeCorrida - ultimaChegada > SEGUNDOS_ANTES_DE_SEGUIR;
    if (jogadorNoPodio(corrida)) {
        /* No pódio (1º, 2º ou 3º) a festa de câmeras e fogos passa sozinha; depois de uns segundos, qualquer botão leva ao resultado */
        int apertou = 0;
        while (GetKeyPressed() != 0) {
            apertou = 1;
        }
        for (int botao = MOUSE_BUTTON_LEFT; botao <= MOUSE_BUTTON_BACK; botao++) {
            apertou = apertou || IsMouseButtonPressed(botao);
        }
        apertou = apertou || botaoApertadoAgora() != 0;
        if (apertou && corrida->tempoDeCorrida - corrida->jogador.tempoDeChegada > ATRASO_DO_AVISO_NA_FESTA) {
            tocarSom(SOM_CONFIRMAR);
            return TELA_RESULTADO;
        }
    } else if (podeSeguir && (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) ||
                              controleApertou(BOTAO_CONFIRMAR))) {
        tocarSom(SOM_CONFIRMAR);
        return TELA_RESULTADO;
    }
    return TELA_CORRIDA;
}

/*
 * recadoDaCorrida
 * O que aparece na volta à cidade: a pista e como o jogador foi.
 * Recebe: a corrida.
 * Devolve: o texto (guardado aqui dentro, até a próxima chamada).
 */
static const char *recadoDaCorrida(const Corrida *corrida) {
    static char texto[96];
    char pista[48];
    int i = 0;
    for (; corrida->pista->nome[i] != '\0' && i < (int) sizeof(pista) - 1; i++) {
        pista[i] = (char) toupper((unsigned char) corrida->pista->nome[i]);
    }
    pista[i] = '\0';
    if (!corrida->terminou) {
        snprintf(texto, sizeof(texto), "%s: CORRIDA ABANDONADA", pista);
    } else if (corrida->totalDeHumanos > 1) {
        /* Na tela dividida o recado é dos dois: quem volta à cidade é o carro do jogador 1, mas a
         * corrida foi dos dois e o resultado de um só não diria nada */
        snprintf(texto, sizeof(texto), "%s: %dº E %dº LUGAR", pista,
                 corrida->posicaoFinalDosHumanos[0], corrida->posicaoFinalDosHumanos[1]);
    } else if (corrida->desclassificado) {
        snprintf(texto, sizeof(texto), "%s: DESCLASSIFICADO", pista);
    } else if (corrida->posicaoFinal == 1) {
        snprintf(texto, sizeof(texto), "%s: VITÓRIA!", pista);
    } else {
        snprintf(texto, sizeof(texto), "%s: %dº LUGAR", pista, corrida->posicaoFinal);
    }
    return texto;
}

/*
 * abrirJanela
 * Abre o jogo em tela cheia, na resolução do monitor. É uma "tela cheia
 * sem bordas" (borderless): uma janela do tamanho do monitor, sem mudar
 * o modo de vídeo do Windows, então o Alt+Tab funciona sem travar.
 * Com --janela na linha de comando, abre numa janela 800x600 (útil para
 * testar e depurar).
 * Recebe: 1 para abrir em janela, 0 para tela cheia.
 * Devolve: 1 se a janela abriu, 0 se não (por exemplo, com a tela desligada).
 */
static int abrirJanela(int emJanela) {
    /* VSync: cada quadro sai junto com a atualização do monitor, em
     * intervalos regulares, sem os "trancos" de quadros irregulares.
     * Precisa vir antes do InitWindow. O SetTargetFPS logo abaixo
     * continua valendo como limite, caso o driver de vídeo ignore o VSync. */
    if (configuracoesAtuais()->sincroniaVertical) {
        SetConfigFlags(FLAG_VSYNC_HINT);
    }
    InitWindow(LARGURA_TELA, ALTURA_TELA, TITULO_JANELA);
    if (!IsWindowReady()) {
        return 0;   /* por exemplo, com a tela do computador desligada */
    }
    if (!emJanela) {
        aplicarTela(configuracoesAtuais());   /* o modo e a resolução escolhidos nas configurações (padrão: o monitor inteiro) */
        HideCursor();
    }
    aplicarDesempenho(configuracoesAtuais());
    SetExitKey(KEY_NULL);   /* ESC volta de tela; para sair, use "SAIR" no menu */
    return 1;
}

int main(int argc, char *argv[]) {
    int emJanela = 0, diretoNoPasseio = 0;
    for (int i = 1; i < argc; i++) {
        emJanela |= (strcmp(argv[i], "--janela") == 0);
        diretoNoPasseio |= (strcmp(argv[i], "--passeio") == 0);   /* abre direto no passeio livre (para testar) */
    }
    /* As configurações (inclusive a exibição) são lidas antes de abrir a janela, para ela já abrir como o jogador quer */
    ChangeDirectory(GetApplicationDirectory());
    carregarConfiguracoes(configuracoesAtuais(), ARQUIVO_DE_CONFIGURACOES);
    if (!abrirJanela(emJanela)) {
        printf("Nao deu para abrir a janela do jogo. A tela do computador esta ligada?\n");
        return 1;
    }

    /* As texturas são procuradas em recursos/, ao lado do executável.
     * Mudar para a pasta do executável faz o jogo achá-las mesmo quando
     * é aberto de outro lugar (por exemplo, com duplo clique). */
    ChangeDirectory(GetApplicationDirectory());
    quadroPretoDaEspera();   /* a janela nasce preta, e não branca */
    iniciarSinteseDosEfeitos();   /* os sons sintetizados nascem numa thread enquanto os gráficos carregam */
    marcarEtapa("janela");
    carregarVisual();   /* fontes e logo dos menus */
    marcarEtapa("visual");
    faixaDaEspera(0.0f, 1.0f);
    passoDaEspera("CARREGANDO VIDEOS", 0.02f);   /* daqui em diante, a tela de carregamento mostra o andamento */
    /* Os vídeos de fundo: a tela de entrada e o menu principal (em loop; se faltarem, a tela segue sem vídeo) */
    VideoDeFundo *videoDaEntrada = abrirVideoDeFundo("recursos/video/tela_entrada.mjpeg");
    VideoDeFundo *videoDoMenu = abrirVideoDeFundo("recursos/video/tela_menu.mjpeg");
    marcarEtapa("videos de fundo");
    float transicaoAlfa = 0.0f;   /* 1 logo depois de sair da entrada: a tela clareia do preto até o menu */

    Pista *pistas[TOTAL_PISTAS];
    for (int i = 0; i < TOTAL_PISTAS; i++) {
        pistas[i] = criarPistaPorId((IdPista) i);
    }
    marcarEtapa("pistas");
    faixaDaEspera(0.12f, 0.50f);
    carregarMundo(pistas);   /* informa o andamento dele à tela de carregamento */
    marcarEtapa("mundo (cenarios)");
    faixaDaEspera(0.50f, 0.95f);
    carregarCarros3D();
    faixaDaEspera(0.0f, 1.0f);
    marcarEtapa("carros 3D");
    passoDaEspera("CARREGANDO SONS", 0.96f);
    carregarAudio();    /* música dos menus (por último: a thread dos sons sintetizados já acabou) */
    marcarEtapa("audio");
    passoDaEspera("QUASE PRONTO", 0.99f);
    carregarFumaca();
    carregarMarcas();
    carregarPosProcesso();
    marcarEtapa("fumaca/marcas/pos");
    Corrida corrida;
    EstadoMenu menu;
    iniciarMenu(&menu);
    carregarRecordes();
    Tela tela = TELA_ENTRADA;   /* primeiro a tela de entrada; depois do vídeo de abertura, dissolve nela */
    int venceuAntes = 0;   /* para soltar o confete uma vez só, no quadro da vitória */
    /* Uma visão (câmera) e uma entrada (teclas e controle) por jogador do sofá. Com um jogador só
     * o vetor tem um item, e tudo funciona como sempre */
    VisaoDoJogador visoes[MAXIMO_DE_VISOES];
    Janela janelas[MAXIMO_DE_VISOES];
    EntradaDoJogador entradas[MAXIMO_DE_JOGADORES_LOCAIS];
    for (int v = 0; v < MAXIMO_DE_VISOES; v++) {
        iniciarVisao(&visoes[v], v);
    }
    repartirEntradas(entradas, 1);
    int pausada = 0;       /* a corrida está na pausa (ESC) */
    Apresentacao apresentacao = { 0 };   /* o jogo de câmeras antes da largada */
    EstadoDaVibracao estadoDaVibracao;
    iniciarEstadoDaVibracao(&estadoDaVibracao);
    iniciarVibracao();
    marcarEtapa("menu/recordes/resto");

    /* O vídeo de abertura: passa só quando o jogo abre (não a cada corrida); qualquer botão pula */
    int naIntro = diretoNoPasseio ? 0 : iniciarIntro();
    if (diretoNoPasseio) {
        tela = TELA_PASSEIO;
        iniciarPasseio(menu.carroEscolhido);
    } else if (!naIntro) {
        comecarAbertura();   /* sem o vídeo, a abertura toca no menu mesmo assim */
    }

    marcarEtapa("intro");
    /* KATCHAU_CORRIDA=N: abre direto numa corrida na pista N (0 Interlagos, 1 Copa Pistão, 2 Oval, 3 Splat), para medir */
    int corridaDeTeste = (getenv("KATCHAU_CORRIDA") != NULL) ? atoi(getenv("KATCHAU_CORRIDA")) : -1;
    if (corridaDeTeste >= 0 && corridaDeTeste < TOTAL_PISTAS) {
        menu.pistaEscolhida = corridaDeTeste;
        tela = TELA_MENU;
        naIntro = 0;
    } else {
        corridaDeTeste = -1;
    }
    /* KATCHAU_SAIR_EM=N: fecha o jogo sozinho N segundos depois do primeiro quadro (para medir sem teclado) */
    const char *sairEm = getenv("KATCHAU_SAIR_EM");
    double fimDaMedicao = (sairEm != NULL) ? GetTime() + atof(sairEm) : -1.0;
    /* Laço principal: roda uma vez por quadro até escolher SAIR ou fechar a janela */
    while (tela != TELA_SAIR && !WindowShouldClose() && (fimDaMedicao < 0.0 || GetTime() < fimDaMedicao)) {
        /* Tempo do último quadro, para a velocidade não depender do FPS */
        float tempoQuadro = GetFrameTime();

        if (naIntro) {
            naIntro = atualizarIntro(tempoQuadro);
            if (!IsCursorHidden()) {
                HideCursor();
            }
            BeginDrawing();
            ClearBackground(BLACK);
            desenharIntro();
            EndDrawing();
            continue;   /* quando acaba, a intro se dissolve por cima do menu (desenharSaidaDaIntro) */
        }
        atualizarControle();   /* acha os controles e lê o analógico, antes das telas perguntarem */
        /* Quem joga com o quê, todo quadro: assim ligar um controle no meio da corrida já vale */
        repartirEntradas(entradas, (tela == TELA_CORRIDA || tela == TELA_RESULTADO) ? corrida.totalDeHumanos : 1);
        /* Os vídeos de fundo avançam só enquanto a tela deles está aberta */
        if (tela == TELA_ENTRADA) {
            atualizarVideoDeFundo(videoDaEntrada, tempoQuadro);
        } else if (tela == TELA_MENU) {
            atualizarVideoDeFundo(videoDoMenu, tempoQuadro);
        }

        /* ---- 1) Atualizar a tela atual e descobrir a próxima ---- */
        Tela proxima = tela;
        switch (tela) {
            case TELA_ENTRADA:
                proxima = atualizarTelaDeEntrada();
                break;
            case TELA_MENU:
                proxima = atualizarMenuPrincipal(&menu);
                break;
            case TELA_GARAGEM:
                proxima = atualizarGaragem(&menu);
                break;
            case TELA_PISTAS:
                proxima = atualizarEscolhaDePista(&menu);
                break;
            case TELA_CONFIGURACOES:
                proxima = atualizarConfiguracoes(&menu);
                break;
            case TELA_COMO_JOGAR:
                proxima = atualizarComoJogar();
                break;
            case TELA_CORRIDA:
                proxima = atualizarTelaDaCorrida(&corrida, &menu, &pausada, &apresentacao, entradas, visoes,
                                                 tempoQuadro);
                break;
            case TELA_PASSEIO:
                proxima = atualizarPasseio(&menu, tempoQuadro);
                break;
            case TELA_RESULTADO: {
                /* A corrida continua ao fundo até todos os rivais chegarem */
                Comandos nenhumComando = { 0, 0, 0, 0, 0.0f, 0, 0 };
                atualizarCorrida(&corrida, nenhumComando, tempoQuadro);
                proxima = atualizarResultado();
                break;
            }
            default:
                break;
        }

        if (corridaDeTeste >= 0) {
            proxima = TELA_CORRIDA;
            corridaDeTeste = -1;
        }
        /* A escolha da corrida abre no Mack: escolhida, o carro entra na carreta e o Mack parte
         * (o mundo aberto leva à corrida no fim da cena); desistindo, o carro fica na vaga */
        if (tela == TELA_PISTAS && proxima == TELA_CORRIDA) {
            partirComOMack(pistas[menu.pistaEscolhida]->nome);
            proxima = TELA_PASSEIO;
        } else if (tela == TELA_PISTAS && proxima == TELA_PASSEIO) {
            voltarDaEscolhaDaCorrida();
        }
        /* Da corrida (pelo resultado ou pela pausa) de volta à cidade: o carro desce da carreta */
        if ((tela == TELA_CORRIDA || tela == TELA_RESULTADO) && proxima == TELA_PASSEIO) {
            voltarDaCorrida(menu.carroEscolhido, recadoDaCorrida(&corrida));
        }
        /* Entrar na corrida (pelo Mack ou pelo "correr de novo") sempre começa uma nova */
        if (proxima == TELA_CORRIDA && tela != TELA_CORRIDA) {
            if (menu.telaDividida) {
                FormacaoLocal formacao = { MAXIMO_DE_JOGADORES_LOCAIS,
                                           { menu.carroEscolhido, menu.carroDoJogador2 } };
                iniciarCorridaDividida(&corrida, pistas[menu.pistaEscolhida], menu.nivelEscolhido, &formacao);
            } else {
                iniciarCorrida(&corrida, pistas[menu.pistaEscolhida], menu.carroEscolhido, menu.nivelEscolhido);
            }
            for (int v = 0; v < corrida.totalDeHumanos; v++) {
                iniciarVisao(&visoes[v], v);
            }
            limparConfetes();
            limparFumaca();
            limparFogos();
            iniciarEstadoDaVibracao(&estadoDaVibracao);
            iniciarEfeitosDaCorrida();
            iniciarApresentacao(&apresentacao);
        }
        /* JOGAR: o carro escolhido na garagem, na rua principal */
        if (proxima == TELA_PASSEIO && tela == TELA_MENU) {
            iniciarPasseio(menu.carroEscolhido);
        }
        /* Troca de tela: o vídeo da tela nova recomeça do início; sair da entrada clareia até o menu */
        if (proxima != tela) {
            if (proxima == TELA_ENTRADA) {
                reiniciarVideoDeFundo(videoDaEntrada);
            } else if (proxima == TELA_MENU) {
                reiniciarVideoDeFundo(videoDoMenu);
            }
            if (tela == TELA_ENTRADA && proxima == TELA_MENU) {
                transicaoAlfa = 1.0f;
            }
        }
        tela = proxima;

        /* Venceu: a festa começa no quadro em que o jogador cruza a linha */
        int naCorridaOuResultado = (tela == TELA_CORRIDA || tela == TELA_RESULTADO);
        int venceu = naCorridaOuResultado && jogadorVenceu(&corrida);          /* 1º lugar: a música da vitória */
        int festa = naCorridaOuResultado && jogadorNoPodio(&corrida);          /* 1º, 2º ou 3º: confete, fogos e câmeras da festa */
        if (festa && !venceuAntes) {
            soltarConfetes((float) larguraVirtual(), (float) alturaVirtual());
        }
        venceuAntes = festa;

        /* O mouse só aparece (e só funciona) no menu principal, na garagem e na pausa */
        int usaMouse = (tela == TELA_MENU || tela == TELA_GARAGEM || tela == TELA_PISTAS || tela == TELA_CONFIGURACOES
                        || (tela == TELA_CORRIDA && pausada) || (tela == TELA_PASSEIO && passeioPausado()));
        if (usaMouse && IsCursorHidden()) {
            ShowCursor();
        } else if (!usaMouse && !IsCursorHidden()) {
            HideCursor();
        }

        /* O McQueen bateu em alguma coisa (carro ou muro), assumiu ou perdeu a liderança: ele reage */
        if (tela == TELA_CORRIDA && !pausada) {
            tocarGritoDeRodada(rodadaDoMcQueen(&corrida));   /* antes da batida: quando ele roda, o grito vale mais */
            int batidaContraCarro = 0;
            float batida = batidaDoMcQueen(&corrida, &batidaContraCarro);
            tocarSomDaBatida(batida, batidaContraCarro);
            int lideranca = mudancaDeLiderancaDoMcQueen(&corrida, tempoQuadro);
            if (lideranca != 0) {
                tocarFalaDeLideranca(lideranca > 0);
            }
            /* Ultrapassou ou foi ultrapassado em qualquer posição (depois da fala de liderança, que vale mais) */
            int posicao = mudancaDePosicaoDoMcQueen(&corrida, tempoQuadro);
            if (posicao != 0) {
                tocarFalaDeUltrapassagem(posicao > 0);
            }
        }

        /* O controle só vibra com a corrida andando: nos menus e na pausa, fica quieto */
        if (tela == TELA_CORRIDA && !pausada) {
            Vibracao vibracao = calcularVibracao(&estadoDaVibracao, &corrida, tempoQuadro);
            vibracao = vibracaoAoGosto(vibracao, configuracoesAtuais());
            aplicarVibracao(&vibracao);
        } else if (tela == TELA_CONFIGURACOES && menu.pulsoDeVibracao > 0.0f) {
            /* Mudou a vibração: um toque, para sentir como ficou */
            Vibracao toque = { FORCA_DO_TOQUE, FORCA_DO_TOQUE, 0.0f };
            toque = vibracaoAoGosto(toque, configuracoesAtuais());
            aplicarVibracao(&toque);
            menu.pulsoDeVibracao -= tempoQuadro;
        } else {
            pararVibracao();
        }

        /* A música toca nos menus e some quando a corrida começa */
        int apresentando = (tela == TELA_CORRIDA && apresentacaoAtiva(&apresentacao));
        if (apresentando) {
            definirVolumeDaTrilha(volumeDaTrilhaNaApresentacao(&apresentacao));   /* some nos últimos segundos */
        } else if (tela != TELA_CORRIDA) {
            definirVolumeDaTrilha(1.0f);
        }
        atualizarAudio(tela == TELA_ENTRADA || tela == TELA_MENU || tela == TELA_GARAGEM || tela == TELA_PISTAS || tela == TELA_CONFIGURACOES || tela == TELA_COMO_JOGAR
                       || (tela == TELA_PASSEIO && cenaDoMackComMusica())   /* da escolha da corrida, a música segue na estrada com o Mack */
                       || apresentando);   /* e na apresentação da largada */
        /* A noite da cidade: os neons acendem e toca a Sh-Boom, até amanhecer (nas cenas do Mack, a música do menu) */
        atualizarMusicaDaNoite(luzesDaCidadeLigadas(), tela == TELA_PASSEIO && !cenaDoMackComMusica());

        /* Sons da corrida:
         *  - antes do GO, o motor em marcha lenta (som provisório) e o "3... 2... 1... GO!",
         *    que começa junto com o número 3 na tela e vai até o fim do KATCHAU!;
         *  - do GO em diante, o motor de verdade, que segue a rotação e a carga do carro do
         *    jogador, e a música de fundo (que sai quando o jogador chega);
         *  - chegando em 1º na última volta, a música da vitória entra baixinho no lugar
         *    da música de fundo e o refrão cai junto com a bandeirada; daí em diante só
         *    ela toca (o motor sai, mesmo com a IA levando o carro) */
        if (!pausada) {   /* na pausa, os sons da corrida ficam congelados (pausarSonsDaCorrida) */
            int naCorrida = (tela == TELA_CORRIDA);
            int antesDoGo = (naCorrida && !apresentando && corrida.contagemRegressiva > 0.0f);
            int depoisDoGo = (naCorrida && corrida.contagemRegressiva <= 0.0f);
            int naContagem = (naCorrida && !apresentando && corrida.contagemRegressiva < NUMEROS_DA_CONTAGEM * SEGUNDOS_POR_NUMERO &&
                              (antesDoGo || corrida.tempoDeCorrida < DURACAO_GRITO_LARGADA));
            float segundosAteALinha = naCorrida ? segundosAteAVitoria(&corrida) : -1.0f;
            atualizarMusicaDaVitoria(segundosAteALinha, venceu);
            atualizarSonsDaCorrida(antesDoGo, naContagem,
                                   depoisDoGo && !corrida.terminou && !musicaDaVitoriaTocando());
            if (naCorrida) {
                /* Os motores NASCAR (o do jogador e os dos rivais mais próximos), o vento, o pneu,
                 * a grama, o muro, o nitro, o câmbio, as voltas e o pit stop: do GO até a bandeirada */
                atualizarEfeitosDaCorrida(&corrida, depoisDoGo && !festa, tempoQuadro);
            }
        }

        /* ---- 2) Desenhar a tela atual ----
         * Primeiro o "mundo" (pista e carros), na resolução real da tela.
         * Depois a interface (HUD e menus), na tela virtual 800x600 que
         * comecarInterface() amplia para o tamanho da tela. */
        BeginDrawing();
        ClearBackground(BLACK);
        congelarCena(tela == TELA_CORRIDA && pausada);
        usarFormasRetas(tela != TELA_CORRIDA);   /* os menus usam retângulos arredondados; o HUD da corrida, não */

        /* A câmera livre vale na corrida; na pausa ela fica onde está. Uma por jogador: na tela
         * dividida, o analógico direito de cada controle mexe só na metade de quem o segura */
        if (!pausada) {
            int quantas = (tela == TELA_CORRIDA || tela == TELA_RESULTADO) ? corrida.totalDeHumanos : 1;
            for (int v = 0; v < quantas && v < MAXIMO_DE_VISOES; v++) {
                float eixoX = 0.0f, eixoY = 0.0f;
                if (tela == TELA_CORRIDA) {
                    lerAnalogicoDaCameraDoJogador(&entradas[v], &eixoX, &eixoY);
                }
                atualizarOlhar(&visoes[v].olhar, eixoX, eixoY, tempoQuadro);
            }
        }
        switch (tela) {
            case TELA_ENTRADA:
                if (videoDaEntrada != NULL) {
                    desenharVideoDeFundo(videoDaEntrada, 1.0f);
                }
                comecarInterface(1);
                desenharTelaDeEntrada(tempoQuadro);
                terminarInterface();
                break;
            case TELA_MENU:
                if (videoDoMenu != NULL) {
                    desenharVideoDeFundo(videoDoMenu, 1.0f);   /* o vídeo do carro, por trás de tudo */
                } else {
                    desenharFundoAlternado(pistas);
                }
                comecarInterface(1);
                desenharMenuPrincipal(&menu);
                terminarInterface();
                break;
            case TELA_GARAGEM:
                desenharFundoAlternado(pistas);
                comecarInterface(1);
                desenharGaragem(&menu);
                terminarInterface();
                break;
            case TELA_PISTAS:
                desenharPistaDeFundo(pistas[menu.pistaEscolhida]);
                comecarInterface(1);
                desenharEscolhaDePista(&menu, pistas);
                terminarInterface();
                break;
            case TELA_CONFIGURACOES:
                desenharFundoAlternado(pistas);
                comecarInterface(1);
                desenharConfiguracoes(&menu);
                terminarInterface();
                break;
            case TELA_COMO_JOGAR:
                desenharFundoAlternado(pistas);
                comecarInterface(1);
                desenharComoJogar();
                terminarInterface();
                break;
            case TELA_CORRIDA:
                if (apresentando) {
                    desenharApresentacao(&apresentacao, &corrida);
                    if (pausada) {
                        comecarInterface(1);
                        desenharPausa(&menu);
                        terminarInterface();
                    }
                    break;
                }
                /* O mundo anda uma vez por quadro; depois cada jogador tem a sua faixa de tela,
                 * com a câmera e o HUD dele dentro. Com um jogador é uma faixa só: a tela inteira */
                repartirJanelas(corrida.totalDeHumanos, janelas);
                for (int v = 0; v < corrida.totalDeHumanos; v++) {
                    visoes[v].janela = janelas[v];
                    atualizarVisao(&corrida, &visoes[v]);
                }
                avancarMundoDaCorrida(&corrida, &visoes[0]);
                /* Na tela dividida o HUD de cada faixa recua metade da tira da classificação: o 3D
                 * usa a faixa inteira, mas o HUD tem de se afastar da linha do meio, senão a tira
                 * cobriria o pé do mini-mapa e do velocímetro do jogador de cima */
                int recuoDaTira = (corrida.totalDeHumanos > 1)
                                  ? (int) (alturaDaFaixaDeClassificacao() / 2.0f) : 0;
                for (int v = 0; v < corrida.totalDeHumanos; v++) {
                    desenharVisao(&corrida, &visoes[v]);
                    Janela daInterface = janelas[v];
                    if (v > 0) {
                        daInterface.y += recuoDaTira;
                    }
                    daInterface.altura -= recuoDaTira;
                    definirAreaDaInterface(daInterface.x, daInterface.y, daInterface.largura, daInterface.altura);
                    comecarInterface(0);
                    if (!festa) {   /* na festa do pódio a tela fica limpa: só a imagem, os fogos e o aviso */
                        desenharHudDoJogador(&corrida, v);
                        /* Na largada, quem é esta faixa e com o que ela se joga (a entrada mora aqui) */
                        desenharPlacaDeControles(&corrida, v, &entradas[v]);
                    }
                    if (festa) {
                        if (corrida.tempoDeCorrida - corrida.jogador.tempoDeChegada > ATRASO_DO_AVISO_NA_FESTA) {
                            desenharAvisoParaContinuar(&corrida);   /* o painel de chegada some: o resultado vem depois do botão */
                        }
                    } else if (corrida.terminouPara[v]) {
                        desenharFimDeCorridaDoJogador(&corrida, v);
                    }
                    terminarInterface();   /* devolve a interface à tela cheia */
                }
                comecarInterface(0);
                if (corrida.totalDeHumanos > 1) {
                    /* A tira é a própria divisão: ela separa as metades e tapa a linha de sobra de
                     * uma altura de tela ímpar, que era o outro motivo do risco preto */
                    desenharFaixaDeClassificacao(&corrida);
                }
                desenharConfetes(pausada ? 0.0f : tempoQuadro);   /* na pausa, o confete para no ar */
                terminarInterface();
                if (pausada) {
                    comecarInterface(1);
                    desenharPausa(&menu);
                    terminarInterface();
                }
                break;
            case TELA_PASSEIO:
                desenharPasseio(&menu);
                break;
            case TELA_RESULTADO:
                /* A corrida continua ao fundo, na câmera do jogador 1 e na tela inteira */
                visoes[0].janela = (Janela){ 0, 0, GetScreenWidth(), GetScreenHeight() };
                atualizarVisao(&corrida, &visoes[0]);
                avancarMundoDaCorrida(&corrida, &visoes[0]);
                desenharVisao(&corrida, &visoes[0]);
                comecarInterface(1);
                desenharResultado(&corrida);
                desenharConfetes(tempoQuadro);
                terminarInterface();
                break;
            default:
                break;
        }
        desenharSaidaDaIntro(tempoQuadro);   /* o fim do vídeo se dissolvendo no menu */
        if (configuracoesAtuais()->mostrarFps) {   /* o contador de quadros por segundo, no canto de cima */
            comecarInterface(0);
            escrever(FONTE_FORTE, TextFormat("%d FPS", GetFPS()), larguraVirtual() - 78.0f, 8.0f, 16.0f, COR_AMARELO);
            terminarInterface();
        }
        /* A transição: a tela clareia do preto (depois de sair da entrada) */
        if (transicaoAlfa > 0.0f) {
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, transicaoAlfa));
            transicaoAlfa -= tempoQuadro * 1.6f;
        }
        EndDrawing();
        /* KATCHAU_FOTO=arquivo.png tira uma foto da tela depois de KATCHAU_FOTO_EM segundos (para comparar imagens) */
        static int tirouAFoto = 0;
        if (!tirouAFoto && getenv("KATCHAU_FOTO") != NULL
            && GetTime() >= fimDaMedicao - atof(sairEm != NULL ? sairEm : "0") + atof(getenv("KATCHAU_FOTO_EM") != NULL ? getenv("KATCHAU_FOTO_EM") : "8")) {
            tirouAFoto = 1;
            TakeScreenshot(getenv("KATCHAU_FOTO"));
        }
        medirQuadro(tempoQuadro, 0);
    }
    medirQuadro(0.0f, 1);   /* o resumo do que foi medido até aqui */
    encerrarIntro();   /* se a janela fechou no meio da dissolução */

    descarregarPasseio();
    descarregarFumaca();
    descarregarMarcas();
    descarregarPosProcesso();
    descarregarCarros3D();
    encerrarVibracao();
    descarregarMundo();
    for (int i = 0; i < TOTAL_PISTAS; i++) {
        liberarPista(pistas[i]);
    }
    descarregarVisual();
    fecharVideoDeFundo(videoDaEntrada);
    fecharVideoDeFundo(videoDoMenu);
    descarregarAudio();
    CloseWindow();
    return 0;
}
