/*
 * main.c
 * ------------------------------------------------------------------
 * Ponto de entrada do KATCHAU.
 * Abre a janela e troca entre as telas: menu, garagem, como jogar,
 * corrida e resultado. A variável "tela" diz em qual delas o jogador
 * está; cada tela devolve para qual ir no quadro seguinte.
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
#include "efeitos.h"
#include "apresentacao.h"
#include "intro.h"
#include "videofundo.h"
#include "recordes.h"
#include "controle.h"

/* Na vitória, o painel de chegada espera um pouco para o jogador ver o
 * confete e o refrão entrando */
#define SEGUNDOS_ANTES_DE_SEGUIR 1.5f   /* depois da bandeirada */
#define FORCA_DO_TOQUE 0.7f   /* o toque de teste da vibração, nas configurações */
#define ATRASO_DO_PAINEL_NA_VITORIA  9.6f   /* segundos depois de cruzar a linha: as duas primeiras câmeras da festa passam limpas */

/*
 * lerComandos
 * Traduz as teclas apertadas para os comandos do carro.
 * As teclas e os botões são os escolhidos nas configurações (controle.c).
 * Recebe: nada.
 * Devolve: os comandos deste quadro.
 */
static Comandos lerComandos(void) {
    Comandos comandos = { 0, 0, 0, 0, 0.0f, 0, 0 };
    lerComandosDoTeclado(&comandos);
    somarComandosDoControle(&comandos);
    return comandos;
}

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
 * atualizarTelaDaCorrida
 * Um quadro da corrida: o jogador dirige, a IA dirige os rivais.
 * ESC (ou START) pausa: a corrida e os sons congelam e o jogador escolhe
 * entre continuar e voltar ao menu principal. Antes da contagem passa a
 * apresentação da largada (apresentacao.c). O touchpad (ou C) troca a
 * distância da câmera. Depois da chegada, ENTER (ou A) vai ao resultado.
 * Recebe: a corrida, o estado do menu (para a pausa), se a corrida está
 *         pausada (é alterado aqui) e o tempo do quadro.
 * Devolve: a próxima tela.
 */
static Tela atualizarTelaDaCorrida(Corrida *corrida, EstadoMenu *menu, int *pausada, Apresentacao *apresentacao,
                                   float tempoQuadro) {
    if (*pausada) {
        EscolhaDaPausa escolha = atualizarPausa(menu);
        if (escolha == PAUSA_CONTINUAR) {
            *pausada = 0;
            pausarSonsDaCorrida(0);
        } else if (escolha == PAUSA_SAIR) {
            *pausada = 0;   /* os sons pausados somem sozinhos na volta ao menu */
            return TELA_MENU;
        }
        return TELA_CORRIDA;
    }

    if (IsKeyPressed(KEY_ESCAPE) || controleApertou(BOTAO_START)) {
        tocarSom(SOM_CONFIRMAR);
        *pausada = 1;
        abrirPausa(menu);
        pausarSonsDaCorrida(1);
        return TELA_CORRIDA;
    }

    /* A apresentação da largada: o jogo de câmeras passa com a corrida parada (a contagem só
     * começa depois); ENTER (ou o botão de confirmar) pula */
    if (apresentacaoAtiva(apresentacao)) {
        int pular = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_SPACE) ||
                    controleApertou(BOTAO_CONFIRMAR);
        atualizarApresentacao(apresentacao, pular, tempoQuadro);
        return TELA_CORRIDA;
    }

    /* Touchpad (ou C): a câmera vai de perto para médio, longe e volta */
    if (IsKeyPressed(KEY_C) || controleApertou(BOTAO_CAMERA)) {
        alternarDistanciaDaCamera();
        tocarSom(SOM_CURSOR);
    }

    Comandos comandos = lerComandos();
    atualizarCorrida(corrida, comandos, tempoQuadro);
    /* O recorde da pista: quando o jogador termina, a melhor volta dele entra no recorde, se for a melhor de todas */
    static int recordeRegistrado = 0;
    if (corrida->contagemRegressiva > 0.0f) {
        recordeRegistrado = 0;
    } else if (corrida->terminou && !recordeRegistrado) {
        recordeRegistrado = 1;
        if (!corrida->desclassificado && corrida->jogador.melhorVolta > 0.0f) {
            registrarRecorde(corrida->pista->id, corrida->jogador.melhorVolta, corrida->jogador.nome);
        }
    }
    registrarTelemetria(corrida, &comandos);
    /* Espera um pouco: o botão de confirmar é também o freio de mão, e quem o aperta
     * na bandeirada não pode pular a tela da chegada sem querer */
    int podeSeguir = corrida->tempoDeCorrida - corrida->jogador.tempoDeChegada > SEGUNDOS_ANTES_DE_SEGUIR;
    if (corrida->terminou && podeSeguir && (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) ||
                                            controleApertou(BOTAO_CONFIRMAR))) {
        tocarSom(SOM_CONFIRMAR);
        return TELA_RESULTADO;
    }
    return TELA_CORRIDA;
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
    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(LARGURA_TELA, ALTURA_TELA, TITULO_JANELA);
    if (!IsWindowReady()) {
        return 0;   /* por exemplo, com a tela do computador desligada */
    }
    if (!emJanela) {
        ToggleBorderlessWindowed();   /* ocupa o monitor inteiro */
        HideCursor();
    }
    SetTargetFPS(QUADROS_POR_SEG);
    SetExitKey(KEY_NULL);   /* ESC volta de tela; para sair, use "SAIR" no menu */
    return 1;
}

int main(int argc, char *argv[]) {
    int emJanela = (argc > 1 && strcmp(argv[1], "--janela") == 0);
    if (!abrirJanela(emJanela)) {
        printf("Nao deu para abrir a janela do jogo. A tela do computador esta ligada?\n");
        return 1;
    }

    /* As texturas são procuradas em recursos/, ao lado do executável.
     * Mudar para a pasta do executável faz o jogo achá-las mesmo quando
     * é aberto de outro lugar (por exemplo, com duplo clique). */
    ChangeDirectory(GetApplicationDirectory());
    carregarVisual();   /* fontes e logo dos menus */
    carregarAudio();    /* música dos menus */
    /* Os vídeos de fundo: a tela de entrada e o menu principal (em loop; se faltarem, a tela segue sem vídeo) */
    VideoDeFundo *videoDaEntrada = abrirVideoDeFundo("recursos/video/tela_entrada.mjpeg");
    VideoDeFundo *videoDoMenu = abrirVideoDeFundo("recursos/video/tela_menu.mjpeg");
    float transicaoAlfa = 0.0f;   /* 1 logo depois de sair da entrada: a tela clareia do preto até o menu */

    Pista *pistas[TOTAL_PISTAS];
    for (int i = 0; i < TOTAL_PISTAS; i++) {
        pistas[i] = criarPistaPorId((IdPista) i);
    }
    carregarMundo(pistas);
    carregarCarros3D();
    carregarFumaca();
    carregarMarcas();
    carregarPosProcesso();
    Corrida corrida;
    EstadoMenu menu;
    iniciarMenu(&menu);
    carregarConfiguracoes(configuracoesAtuais(), ARQUIVO_DE_CONFIGURACOES);
    carregarRecordes();
    Tela tela = TELA_ENTRADA;   /* primeiro a tela de entrada; depois do vídeo de abertura, dissolve nela */
    int venceuAntes = 0;   /* para soltar o confete uma vez só, no quadro da vitória */
    Olhar olhar = olharParaAFrente();
    int pausada = 0;       /* a corrida está na pausa (ESC) */
    Apresentacao apresentacao = { 0 };   /* o jogo de câmeras antes da largada */
    EstadoDaVibracao estadoDaVibracao;
    iniciarEstadoDaVibracao(&estadoDaVibracao);
    iniciarVibracao();

    /* O vídeo de abertura: passa só quando o jogo abre (não a cada corrida); qualquer botão pula */
    int naIntro = iniciarIntro();
    if (!naIntro) {
        comecarAbertura();   /* sem o vídeo, a abertura toca no menu mesmo assim */
    }

    /* Laço principal: roda uma vez por quadro até escolher SAIR ou fechar a janela */
    while (tela != TELA_SAIR && !WindowShouldClose()) {
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
        atualizarControle();   /* acha o controle e lê o analógico, antes das telas perguntarem */
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
                proxima = atualizarTelaDaCorrida(&corrida, &menu, &pausada, &apresentacao, tempoQuadro);
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

        /* Entrar na corrida (do menu ou do "correr de novo") sempre começa uma nova */
        if (proxima == TELA_CORRIDA && tela != TELA_CORRIDA) {
            iniciarCorrida(&corrida, pistas[menu.pistaEscolhida], menu.carroEscolhido, menu.nivelEscolhido);
            limparConfetes();
            limparFumaca();
            limparFogos();
            olhar = olharParaAFrente();
            iniciarEstadoDaVibracao(&estadoDaVibracao);
            iniciarEfeitosDaCorrida();
            iniciarApresentacao(&apresentacao);
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
        int venceu = naCorridaOuResultado && jogadorVenceu(&corrida);
        if (venceu && !venceuAntes) {
            soltarConfetes((float) larguraVirtual(), (float) alturaVirtual());
        }
        venceuAntes = venceu;

        /* O mouse só aparece (e só funciona) no menu principal, na garagem e na pausa */
        int usaMouse = (tela == TELA_MENU || tela == TELA_GARAGEM || tela == TELA_PISTAS || tela == TELA_CONFIGURACOES
                        || (tela == TELA_CORRIDA && pausada));
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
        atualizarAudio(tela == TELA_ENTRADA || tela == TELA_MENU || tela == TELA_GARAGEM || tela == TELA_PISTAS || tela == TELA_CONFIGURACOES || tela == TELA_COMO_JOGAR
                       || apresentando);   /* na apresentação da largada, a música segue de trilha */

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
                atualizarEfeitosDaCorrida(&corrida, depoisDoGo && !venceu, tempoQuadro);
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

        /* A câmera livre vale na corrida; na pausa ela fica onde está */
        if (!pausada) {
            float eixoX = 0.0f, eixoY = 0.0f;
            if (tela == TELA_CORRIDA) {
                lerAnalogicoDaCamera(&eixoX, &eixoY);
            }
            atualizarOlhar(&olhar, eixoX, eixoY, tempoQuadro);
            definirOlhar(olhar);
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
                desenharVisaoDoJogador(&corrida);
                comecarInterface(0);
                if (!venceu) {   /* na festa da vitória a tela fica limpa: só a imagem, os fogos e o painel */
                    desenharHud(&corrida);
                }
                if (corrida.terminou && (!venceu || corrida.tempoDeCorrida - corrida.jogador.tempoDeChegada
                                                    > ATRASO_DO_PAINEL_NA_VITORIA)) {
                    desenharFimDeCorrida(&corrida);
                }
                desenharConfetes(pausada ? 0.0f : tempoQuadro);   /* na pausa, o confete para no ar */
                terminarInterface();
                if (pausada) {
                    comecarInterface(1);
                    desenharPausa(&menu);
                    terminarInterface();
                }
                break;
            case TELA_RESULTADO:
                desenharVisaoDoJogador(&corrida);   /* a corrida continua ao fundo */
                comecarInterface(1);
                desenharResultado(&corrida);
                desenharConfetes(tempoQuadro);
                terminarInterface();
                break;
            default:
                break;
        }
        desenharSaidaDaIntro(tempoQuadro);   /* o fim do vídeo se dissolvendo no menu */
        /* A transição: a tela clareia do preto (depois de sair da entrada) */
        if (transicaoAlfa > 0.0f) {
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, transicaoAlfa));
            transicaoAlfa -= tempoQuadro * 1.6f;
        }
        EndDrawing();
    }
    encerrarIntro();   /* se a janela fechou no meio da dissolução */

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
