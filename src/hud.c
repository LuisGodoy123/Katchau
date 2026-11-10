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
#include "pitstop.h"
#include "visual.h"
#include "controle.h"

/* ==================================================================
 * Tela virtual: o HUD e os menus foram desenhados para 800x600.
 * Em vez de recalcular cada posição para cada monitor, eles são
 * desenhados nessas coordenadas e a câmera 2D da raylib amplia tudo
 * para o tamanho real da tela.
 * ================================================================== */

/* O pedaço da tela em que a interface está sendo desenhada. Largura 0 = a tela cheia, que é o
 * normal. Na tela dividida, o HUD de cada jogador é desenhado na faixa dele.
 *
 * A ESCALA não sai da área: sai da TELA (veja escalaDaInterface). A área muda só a origem e a
 * extensão. Tirando a escala da faixa — que foi a primeira tentativa — ela caía pela metade (0,90
 * em vez de 1,80 em 1080p) e o HUD saía ilegível, com letra de 11px virando 10 pixels de verdade.
 * Com a escala da tela, o HUD tem o mesmo tamanho físico do modo de um jogador e o que muda é o
 * ESPAÇO: alturaVirtual() numa faixa dá ~290 em vez de 600, e é com isso que ele tem de se
 * arranjar — daí a classificação compartilhada, o mini-mapa menor e o cartão de chegada compacto. */
static int areaX = 0, areaY = 0, areaLargura = 0, areaAltura = 0;
static int comRecorte = 0;   /* 1 se comecarInterface ligou o recorte da área (veja lá) */

/*
 * larguraDaArea / alturaDaArea
 * Recebe: nada.
 * Devolve: o tamanho em pixels reais da área da interface (a janela toda, se não houver área).
 */
static int larguraDaArea(void) {
    return (areaLargura > 0) ? areaLargura : GetScreenWidth();
}
static int alturaDaArea(void) {
    return (areaAltura > 0) ? areaAltura : GetScreenHeight();
}

/*
 * definirAreaDaInterface
 * Recebe: o pedaço da tela em que a interface vai ser desenhada (largura 0 = a tela cheia).
 * Devolve: nada.
 */
void definirAreaDaInterface(int x, int y, int largura, int altura) {
    areaX = x;
    areaY = y;
    areaLargura = largura;
    areaAltura = altura;
}

/*
 * escalaDaInterface
 * Quanto ampliar o 800x600 para caber na TELA sem cortar nada. É a tela, e não a área: o HUD de uma
 * faixa da tela dividida tem de sair do mesmo tamanho físico do de um jogador.
 * Recebe: nada.
 * Devolve: 1 em 800x600, 1,8 em 1920x1080...
 */
static float escalaDaInterface(void) {
    return fminf(GetScreenWidth() / (float) LARGURA_TELA, GetScreenHeight() / (float) ALTURA_TELA);
}

/*
 * larguraVirtual / alturaVirtual
 * Tamanho da área em coordenadas virtuais. Num monitor 16:9 a largura
 * virtual passa de 800 (dá ~1067), e o HUD usa o espaço todo. Numa faixa
 * da tela dividida a largura é a mesma e a altura cai pela metade.
 * Recebe: nada.
 * Devolve: a largura ou a altura virtual.
 */
int larguraVirtual(void) {
    return (int) (larguraDaArea() / escalaDaInterface());
}
int alturaVirtual(void) {
    return (int) (alturaDaArea() / escalaDaInterface());
}


/*
 * comecarInterface
 * Daqui até terminarInterface(), tudo é desenhado em coordenadas
 * virtuais e ampliado para a tela real.
 *   - centralizada = 0: a origem fica no canto da área (o HUD usa a
 *     largura toda, com o mini-mapa e o velocímetro nos cantos de verdade);
 *   - centralizada = 1: um quadro 800x600 fica no meio dela (menus).
 * Recebe: o modo.
 * Devolve: nada.
 */
void comecarInterface(int centralizada) {
    float zoom = escalaDaInterface();
    /* Com uma área (a faixa de um jogador), o espaço virtual é MENOR que o da tela, mas a escala é
     * a da tela: o que for desenhado fora da faixa cai na faixa do vizinho. E tem coisa que passa —
     * o brilho do mostrador do velocímetro tem raio 82 e nasce a 60 do pé da faixa. O recorte
     * segura tudo dentro, e é ele que torna seguro reposicionar o resto. */
    comRecorte = (areaLargura > 0 && areaAltura > 0);
    if (comRecorte) {
        BeginScissorMode(areaX, areaY, areaLargura, areaAltura);
    }
    Camera2D camera = { 0 };
    camera.zoom = zoom;
    camera.offset.x = (float) areaX;
    camera.offset.y = (float) areaY;
    if (centralizada) {
        camera.offset.x += (larguraDaArea() - LARGURA_TELA * zoom) / 2.0f;
        camera.offset.y += (alturaDaArea() - ALTURA_TELA * zoom) / 2.0f;
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
    if (comRecorte) {
        EndScissorMode();
        comRecorte = 0;
    }
    /* Volta para a tela cheia: há código que mede larguraVirtual() fora de um comecarInterface
     * (os confetes, no main.c), e ele não deve herdar a faixa do último jogador desenhado */
    definirAreaDaInterface(0, 0, 0, 0);
}

#define TAMANHO_TITULO_FIM    48
#define TAMANHO_TEXTO_FIM     22
#define MARGEM_HUD            12
#define COR_TEXTO_HUD         COR_TEXTO_CLARO
#define TAMANHO_CONTAGEM      96

/* Menos que isto de altura virtual é uma faixa da tela dividida, e não a tela inteira: a faixa tem
 * metade dos 600 de sempre (~290 depois do recuo da classificação compartilhada). */
#define ALTURA_DA_FAIXA_ESTREITA  (ALTURA_TELA * 3 / 4)
/* O pé do HUD na faixa precisa de mais folga: a classificação compartilhada fica sobre a linha da
 * divisão, e o brilho do mostrador do velocímetro (raio 82) chegaria nela. */
#define MARGEM_DE_BAIXO_NA_FAIXA  26.0f

/*
 * naFaixaEstreita
 * Recebe: nada.
 * Devolve: 1 se o HUD está sendo desenhado numa faixa da tela dividida. É por aqui que as peças
 *          que não cabem em meia altura sabem que têm de se arranjar de outro jeito.
 */
static int naFaixaEstreita(void) {
    return alturaVirtual() < ALTURA_DA_FAIXA_ESTREITA;
}

/*
 * margemDeBaixo
 * Recebe: nada.
 * Devolve: a folga de baixo do HUD: a de sempre na tela cheia, maior na faixa da tela dividida.
 */
static float margemDeBaixo(void) {
    return naFaixaEstreita() ? MARGEM_DE_BAIXO_NA_FAIXA : (float) MARGEM_HUD;
}

/* O HUD é discreto, como nas transmissões: painéis pequenos, letra
 * pequena (Saira Condensed) e nada de faixas grandes por cima da pista. */

/* ---------- Painel de posições (canto superior esquerdo) ---------- */
#define LARGURA_POSICOES      236.0f
#define ALTURA_CABECALHO_HUD  20.0f
#define ALTURA_SUBCABECALHO   16.0f
#define ALTURA_LINHA_HUD      14.0f
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

/* ---------- Painel do motor (canto inferior direito) ---------- */
#define RAIO_VELOCIMETRO      64.0f
#define ANGULO_DO_ZERO        135.0f   /* graus: 0 rpm embaixo à esquerda... */
#define VARREDURA             270.0f   /* ...e o arco gira 270 graus até embaixo à direita */
#define INICIO_DO_PONTEIRO    0.62f    /* o ponteiro vai de 62% do raio até a borda: não cobre a velocidade */
#define RAIO_DA_MARCHA        13.0f
#define RPM_DA_LUZ_DE_TROCA   7400.0f  /* daqui até a linha vermelha, fora da última marcha, a marcha pisca */
#define PISCADAS_POR_SEGUNDO  10.0f
#define DURACAO_DO_PULO       0.25f    /* a marcha nova cresce e volta ao tamanho em 1/4 s */
#define TAMANHO_VELOCIDADE    36.0f
#define COR_TRACO             Fade(COR_TEXTO_CLARO, 0.55f)

#define DISTANCIA_DO_ARCO_NITRO 11.0f  /* do aro do mostrador até o arco do nitro */
#define ANGULO_DO_ARCO_NITRO  52.0f    /* graus para cima e para baixo do meio */
#define TAMANHO_DO_LOSANGO    9.0f
#define COR_NITRO             (Color){  60, 150, 255, 255 }
#define COR_NITRO_LIGADO      (Color){ 150, 225, 255, 255 }

/* ---------- Cores do HUD ---------- */
#define COR_FUNDO_HUD         Fade(COR_GRAFITE, 0.78f)
#define COR_VERDE_HUD         (Color){  40, 200,  70, 255 }
#define COR_ROXO_HUD          (Color){ 165,  70, 220, 255 }

/* ---------- Mini-mapa (canto inferior esquerdo) ---------- */
#define PONTOS_DESENHADOS_NO_MAPA 400
#define MAX_PONTOS_MAPA       8192   /* um ponto por segmento; Interlagos tem ~3.200 */
#define TAMANHO_MAPA          150.0f  /* lado do quadrado do mini-mapa, em pixels */
/* Na tela dividida a faixa tem ~290 de altura virtual, e os 150 do quadrado (166 com o nome do
 * trecho) comeriam 58% dela. Com 100 fica em 40%, e o traçado continua legível. */
#define TAMANHO_MAPA_DIVIDIDO 100.0f
#define MARGEM_MAPA           8      /* folga entre o traçado e a borda do quadrado */
#define TAMANHO_TEXTO_MAPA    12
#define ESPESSURA_TRACADO     3.0f
#define RAIO_CARRO_MAPA       6.5f

/* Desenho da pista visto de cima, em coordenadas DO QUADRADO do mini-mapa (0 a lado), e não da
 * tela: o canto entra só na hora de desenhar. pontosMapa[i] é onde começa o segmento de índice i.
 * Assim a conta (até 8192 pontos) é feita uma vez por pista, e não a cada vez que o quadrado muda
 * de lugar — o que, na tela dividida, seriam dois cantos a cada quadro. O lado entra na chave do
 * cache junto da pista, porque é com ele que os pontos são escalados; ele é constante dentro de
 * uma corrida, então na prática o cálculo continua acontecendo uma vez só. */
static Vector2 pontosMapa[MAX_PONTOS_MAPA];
static int totalPontosMapa = 0;
static Pista *pistaDoMapa = NULL;
static float ladoDoMapaCalculado = 0.0f;

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
 * Recebe: a pista e o lado do quadrado (ele é menor na tela dividida).
 * Devolve: nada (o resultado fica em pontosMapa, em coordenadas do quadrado).
 */
static void calcularMapa(Pista *pista, float lado) {
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
    float util = lado - 2 * MARGEM_MAPA;
    float escala = util / fmaxf(maxX - minX, maxY - minY);
    /* Centraliza no lado que sobrar */
    float sobraX = (util - (maxX - minX) * escala) / 2.0f;
    float sobraY = (util - (maxY - minY) * escala) / 2.0f;
    for (int i = 0; i < total; i++) {
        pontosMapa[i].x = MARGEM_MAPA + sobraX + (pontosMapa[i].x - minX) * escala;
        pontosMapa[i].y = MARGEM_MAPA + sobraY + (pontosMapa[i].y - minY) * escala;
    }

    totalPontosMapa = total;
    pistaDoMapa = pista;
    ladoDoMapaCalculado = lado;
}

/*
 * noQuadradoDoMapa
 * Recebe: um ponto em coordenadas do mapa e o canto superior esquerdo do quadrado na tela.
 * Devolve: o ponto na tela virtual.
 */
static Vector2 noQuadradoDoMapa(Vector2 ponto, float x, float y) {
    return (Vector2){ x + ponto.x, y + ponto.y };
}

/*
 * pontoDoCarroNoMapa
 * Onde um carro aparece no mini-mapa: entre o começo do segmento dele
 * e o começo do próximo, conforme o quanto já andou no segmento.
 * Recebe: o carro.
 * Devolve: o ponto em coordenadas do mapa.
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
static void desenharCarroNoMapa(Vector2 ponto, int posicao, Color fundo, Color numero, float proporcao) {
    float raio = RAIO_CARRO_MAPA * proporcao;
    float corpo = 11.0f * proporcao;
    DrawCircleV(ponto, raio + 1.0f, COR_GRAFITE);
    DrawCircleV(ponto, raio, fundo);
    const char *texto = TextFormat("%d", posicao);
    float largura = larguraDoTexto(FONTE_FORTE, texto, corpo);
    escrever(FONTE_FORTE, texto, ponto.x - largura / 2.0f, noMeio(FONTE_FORTE, corpo, ponto.y), corpo, numero);
}

/*
 * desenharMiniMapa
 * No canto inferior esquerdo, sem painel por trás: só o traçado da pista
 * (branco, com borda escura para aparecer sobre a grama e o asfalto), a
 * linha de chegada e os carros como bolinhas com a posição de cada um
 * (o carro deste jogador em vermelho, por cima). Embaixo, o nome do trecho.
 * Recebe: a corrida e o carro deste jogador.
 * Devolve: nada.
 */
static void desenharMiniMapa(Corrida *corrida, Carro *jogador) {
    /* Na faixa da tela dividida o quadrado é menor, e com ele diminuem a grossura do traçado e as
     * bolinhas dos carros — senão as bolinhas de tamanho cheio cobririam o desenho da pista */
    float lado = naFaixaEstreita() ? TAMANHO_MAPA_DIVIDIDO : TAMANHO_MAPA;
    float proporcao = lado / TAMANHO_MAPA;
    float x = MARGEM_HUD;
    float y = alturaVirtual() - margemDeBaixo() - lado - TAMANHO_TEXTO_MAPA - 4;
    if (pistaDoMapa != corrida->pista || ladoDoMapaCalculado != lado) {
        calcularMapa(corrida->pista, lado);
    }

    /* Traçado: borda escura por baixo e a linha branca por cima. Um ponto por metro é demais para
     * 150 pixels: o desenho pula de alguns em alguns (as posições dos carros usam todos) */
    int passo = (totalPontosMapa > PONTOS_DESENHADOS_NO_MAPA) ? totalPontosMapa / PONTOS_DESENHADOS_NO_MAPA : 1;
    for (int camada = 0; camada < 2; camada++) {
        float grossura = ((camada == 0) ? ESPESSURA_TRACADO + 3.0f : ESPESSURA_TRACADO) * proporcao;
        Color cor = (camada == 0) ? Fade(COR_GRAFITE, 0.7f) : COR_TEXTO_CLARO;
        for (int i = 0; i < totalPontosMapa; i += passo) {
            int proximo = (i + passo < totalPontosMapa) ? i + passo : 0;
            DrawLineEx(noQuadradoDoMapa(pontosMapa[i], x, y), noQuadradoDoMapa(pontosMapa[proximo], x, y),
                       grossura, cor);
        }
    }

    /* Linha de chegada: um tracinho atravessado no ponto da largada */
    Vector2 largada = noQuadradoDoMapa(pontosMapa[0], x, y);
    Vector2 frente = noQuadradoDoMapa(pontosMapa[1 % totalPontosMapa], x, y);
    Vector2 direcao = { frente.x - largada.x, frente.y - largada.y };
    float tamanho = sqrtf(direcao.x * direcao.x + direcao.y * direcao.y);
    if (tamanho > 0.0f) {
        float braco = 6.0f * proporcao;
        Vector2 lado2 = { -direcao.y / tamanho * braco, direcao.x / tamanho * braco };
        DrawLineEx((Vector2){ largada.x - lado2.x, largada.y - lado2.y },
                   (Vector2){ largada.x + lado2.x, largada.y + lado2.y }, 2.5f * proporcao, COR_VERMELHO);
    }

    /* Do último para o primeiro, para o líder ficar por cima; o deste jogador por último de todos */
    for (int i = TOTAL_CARROS - 1; i >= 0; i--) {
        Carro *carro = corrida->classificacao[i];
        if (carro != jogador) {
            desenharCarroNoMapa(noQuadradoDoMapa(pontoDoCarroNoMapa(carro), x, y), i + 1,
                                COR_TEXTO_CLARO, COR_GRAFITE, proporcao);
        }
    }
    desenharCarroNoMapa(noQuadradoDoMapa(pontoDoCarroNoMapa(jogador), x, y), posicaoDoCarro(corrida, jogador),
                        COR_VERMELHO, COR_TEXTO_CLARO, proporcao);

    const char *trecho = jogador->posicao.segmento->nomeTrecho;
    escrever(FONTE_FORTE, trecho, x + 2, y + lado + 2, TAMANHO_TEXTO_MAPA, COR_TEXTO_HUD);
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

/* A torre de posições, no estilo das transmissões de F1: uma coluna escura, sem enfeite */
#define LARGURA_TORRE         124.0f
#define ALTURA_LOGO_TORRE     64.0f   /* a logo, com 8 de folga em cima */
#define LARGURA_LOGO_TORRE    80.0f
#define ALTURA_VOLTA_TORRE    16.0f
#define ALTURA_LINHA_TORRE    14.5f
#define LARGURA_DO_DESTAQUE   150.0f   /* a barra branca do jogador sai um pouco da coluna */
#define COR_DA_TORRE          (Color){  14,  15,  20, 215 }
#define LETRA_TORRE           11.5f

/* Código de três letras e a cor de cada carro, na ordem das fichas */
static const char *CODIGOS[TOTAL_CARROS] = {
    "MCQ", "KNG", "DOC", "FRA", "SAL", "RAM", "LUI", "GUI", "MAT", "SAR", "FIL", "FIN", "WIN", "SNO", "STO", "BOO", "DJ"
};
static const Color CORES_DAS_EQUIPES[TOTAL_CARROS] = {
    { 224,  30,  36, 255 }, {  40, 140, 230, 255 }, {  20,  50, 140, 255 }, { 200,  20,  30, 255 },
    { 110, 190, 240, 255 }, { 150,  60, 200, 255 }, { 250, 210,  40, 255 }, {  60, 110, 220, 255 },
    { 150,  90,  50, 255 }, {  80, 110,  50, 255 }, { 120, 200, 110, 255 }, { 170, 185, 200, 255 },
    {  80, 210,  90, 255 }, { 255, 130,  20, 255 }, {   0,  95, 255, 255 }, { 195, 145, 215, 255 },
    {   0, 200, 215, 255 }
};

/*
 * codigoDoCarro
 * Recebe: o número do carro.
 * Devolve: o código de três letras dele (MCQ, KNG...).
 */
const char *codigoDoCarro(int numero) {
    return CODIGOS[numero % TOTAL_CARROS];
}

/*
 * corDoCarro
 * Recebe: o número do carro.
 * Devolve: a cor da equipe dele (a mesma da torre de posições).
 */
Color corDoCarro(int numero) {
    return CORES_DAS_EQUIPES[numero % TOTAL_CARROS];
}

/*
 * desenharEtiqueta
 * Uma etiqueta pequena do lado de fora da torre (volta mais rápida, boxes).
 * Recebe: onde, o texto, a cor do fundo e a do texto.
 * Devolve: nada.
 */
static void desenharEtiqueta(float x, float y, const char *texto, Color fundo, Color letra) {
    float largura = larguraDoTexto(FONTE_FORTE, texto, 9.5f) + 8.0f;
    caixaArredondada((Rectangle){ x, y + 2.0f, largura, ALTURA_LINHA_TORRE - 4.0f }, RAIO_PEQUENO, fundo);
    escrever(FONTE_FORTE, texto, x + 4.0f, noMeio(FONTE_FORTE, 9.5f, y + ALTURA_LINHA_TORRE / 2.0f), 9.5f, letra);
}

/*
 * desenharCronometro
 * O ícone da volta mais rápida: um cronômetro branco num quadrado roxo.
 * Recebe: o canto do ícone.
 * Devolve: nada.
 */
static void desenharCronometro(float x, float y) {
    float lado = ALTURA_LINHA_TORRE - 2.0f;
    caixaArredondada((Rectangle){ x, y + 1.0f, lado, lado }, RAIO_PEQUENO, COR_ROXO_HUD);
    Vector2 centro = { x + lado / 2.0f, y + 1.0f + lado / 2.0f + 0.5f };
    DrawRing(centro, 3.2f, 4.4f, 0.0f, 360.0f, 16, COR_TEXTO_CLARO);
    DrawLineEx(centro, (Vector2){ centro.x + 1.8f, centro.y - 1.8f }, 1.2f, COR_TEXTO_CLARO);
    DrawRectangleRec((Rectangle){ centro.x - 1.0f, centro.y - 6.4f, 2.0f, 1.6f }, COR_TEXTO_CLARO);
}

/*
 * desenharPainelDePosicoes
 * A torre de posições, como nas transmissões de F1: o logo e a volta no topo; em cada linha,
 * a posição, a cor e o código do carro e o intervalo para o carro da frente. O líder tem a
 * posição em vermelho; o jogador ganha uma barra branca com o nome inteiro. Do lado de fora,
 * uma etiqueta roxa marca a volta mais rápida, e uma amarela, quem está nos boxes.
 * Recebe: a corrida e o carro deste jogador (a barra branca é dele).
 * Devolve: nada.
 */
static void desenharPainelDePosicoes(Corrida *corrida, Carro *jogador) {
    float x = MARGEM_HUD, y = MARGEM_HUD;
    float altura = ALTURA_LOGO_TORRE + ALTURA_VOLTA_TORRE + TOTAL_CARROS * ALTURA_LINHA_TORRE + 6.0f;
    float raioDaTorre = raioDoTamanho(LARGURA_TORRE, altura);
    caixaArredondada((Rectangle){ x, y, LARGURA_TORRE, altura }, raioDaTorre, COR_DA_TORRE);

    /* O logo e a volta */
    float meio = x + LARGURA_TORRE / 2.0f;
    if (desenharLogoPreta(meio, y + 8.0f, LARGURA_LOGO_TORRE) <= 0.0f) {
        escreverCentralizado(FONTE_TITULO, "KATCHAU", meio, y + 2.0f, 17.0f, COR_TEXTO_CLARO);   /* sem o arquivo */
    }
    y += ALTURA_LOGO_TORRE;
    const char *volta = TextFormat("%d", voltaAtual(jogador));
    char numero[8];
    snprintf(numero, sizeof(numero), "%s", volta);
    const char *total = TextFormat("/%d", corrida->totalVoltas);
    float l1 = larguraDoTexto(FONTE_TEXTO, "VOLTA ", 11.0f), l2 = larguraDoTexto(FONTE_FORTE, numero, 14.0f);
    float l3 = larguraDoTexto(FONTE_TEXTO, total, 11.0f), xVolta = meio - (l1 + l2 + l3) / 2.0f;
    /* Tamanhos diferentes na mesma linha: todos na mesma linha de base, com o número grande no meio */
    float base = noMeio(FONTE_FORTE, 14.0f, y + ALTURA_VOLTA_TORRE / 2.0f) + 14.0f * 0.72f;
    escrever(FONTE_TEXTO, "VOLTA ", xVolta, base - 11.0f * 0.72f, 11.0f, Fade(COR_TEXTO_CLARO, 0.8f));
    escrever(FONTE_FORTE, numero, xVolta + l1, base - 14.0f * 0.72f, 14.0f, COR_TEXTO_CLARO);
    escrever(FONTE_TEXTO, total, xVolta + l1 + l2, base - 11.0f * 0.72f, 11.0f, Fade(COR_TEXTO_CLARO, 0.8f));
    y += ALTURA_VOLTA_TORRE;

    for (int i = 0; i < TOTAL_CARROS; i++) {
        Carro *carro = corrida->classificacao[i];
        int n = carro->numero % TOTAL_CARROS;
        int ehJogador = (carro == jogador);
        Color letra = COR_TEXTO_CLARO;

        if (ehJogador) {
            caixaArredondada((Rectangle){ x, y, LARGURA_DO_DESTAQUE, ALTURA_LINHA_TORRE }, RAIO_PEQUENO, COR_TEXTO_CLARO);
            letra = COR_GRAFITE;
        }
        if (i == 0) {
            caixaArredondada((Rectangle){ x + 2.0f, y + 1.0f, 16.0f, ALTURA_LINHA_TORRE - 2.0f }, raioDeDentro(RAIO_PEQUENO, 2.0f), COR_VERMELHO);
        }
        float yLetra = noMeio(FONTE_FORTE, LETRA_TORRE, y + ALTURA_LINHA_TORRE / 2.0f);   /* no meio da linha */
        escreverCentralizado(FONTE_FORTE, TextFormat("%d", i + 1), x + 10.0f, yLetra, LETRA_TORRE,
                             (i == 0) ? COR_TEXTO_CLARO : letra);
        DrawRectangleRec((Rectangle){ x + 22.0f, y + 3.0f, 3.0f, ALTURA_LINHA_TORRE - 6.0f }, CORES_DAS_EQUIPES[n]);

        if (ehJogador) {
            /* O jogador: o nome inteiro, e a barra fala por ele */
            escrever(FONTE_FORTE, TextFormat("%s", carro->nome), x + 30.0f, yLetra, LETRA_TORRE, letra);
        } else {
            escrever(FONTE_FORTE, CODIGOS[n], x + 30.0f, yLetra, LETRA_TORRE, letra);
            const char *intervalo = (i == 0) ? "Intervalo"
                                             : TextFormat("+%.3f", fabsf(intervaloEmSegundos(carro, corrida->classificacao[i - 1])));
            escreverADireita(FONTE_TEXTO, intervalo, x + LARGURA_TORRE - 9.0f, yLetra, LETRA_TORRE,
                             (i == 0) ? Fade(COR_TEXTO_CLARO, 0.75f) : letra);
            DrawRectangleRec((Rectangle){ x + LARGURA_TORRE - 4.0f, y + 4.0f, 1.0f, ALTURA_LINHA_TORRE - 8.0f },
                             Fade(COR_TEXTO_CLARO, 0.5f));
        }

        /* Do lado de fora: boxes e volta mais rápida */
        float fora = x + (ehJogador ? LARGURA_DO_DESTAQUE : LARGURA_TORRE) + 4.0f;
        if (carroNoPit(carro)) {
            desenharEtiqueta(fora, y, "BOX", COR_AMARELO, COR_GRAFITE);
        } else if (corrida->voltaMaisRapida > 0.0f && carro->melhorVolta == corrida->voltaMaisRapida) {
            desenharCronometro(fora, y);
        }
        y += ALTURA_LINHA_TORRE;
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

/* ==================================================================
 * A classificação da tela dividida
 *
 * Com dois jogadores, a torre de 17 linhas era desenhada DUAS vezes, uma em cada faixa, e aí o
 * texto ficava pequeno demais. Ela sai das faixas e vira uma tira horizontal em cima da linha que
 * separa as metades: não é de nenhum dos dois, os dois leem igual, e não gasta nenhum dos quatro
 * cantos — que ficam para o que é de cada um (velocidade, nitro, mapa, volta).
 *
 * O que é de cada um e saiu da torre (a volta e a posição) vai para a plaquinha do jogador, ao lado
 * do painel da volta.
 * ================================================================== */
#define ALTURA_DA_FAIXA_DA_ORDEM  24.0f   /* a tira, em coordenadas virtuais */
#define LETRA_DA_FAIXA_DA_ORDEM   11.0f
#define LISTRA_DA_FAIXA_DA_ORDEM  3.0f    /* a listra de cor da equipe, em cada célula */

/*
 * alturaDaFaixaDeClassificacao
 * Recebe: nada.
 * Devolve: a altura da tira em PIXELS DE VERDADE. O main.c usa metade dela para recuar o HUD de
 *          cada faixa da linha do meio, senão a tira cobriria o pé do mini-mapa e do velocímetro
 *          do jogador de cima.
 */
float alturaDaFaixaDeClassificacao(void) {
    return ALTURA_DA_FAIXA_DA_ORDEM * escalaDaInterface();
}

/*
 * desenharFaixaDeClassificacao
 * A ordem da corrida numa tira sobre a linha da divisão: uma célula por carro, com a posição, a
 * listra da equipe e o código de três letras. A célula de cada humano vem clara e com a etiqueta
 * dele; quem está nos boxes fica amarelo e a volta mais rápida ganha contorno roxo — são as duas
 * marcas que a torre fazia com etiquetas de fora, que não caberiam numa célula de 61.
 * Chamar UMA vez por quadro, em coordenadas de tela cheia (fora das faixas).
 * Recebe: a corrida.
 * Devolve: nada.
 */
void desenharFaixaDeClassificacao(Corrida *corrida) {
    float largura = larguraVirtual() - 2.0f * MARGEM_HUD;
    float celula = largura / TOTAL_CARROS;
    /* O meio sai da divisão DE VERDADE (o mesmo GetScreenHeight()/2 de repartirJanelas): numa
     * altura de tela ímpar ela não cai exatamente em alturaVirtual()/2 */
    float meioDaTela = (GetScreenHeight() / 2) / escalaDaInterface();
    float y = roundf(meioDaTela - ALTURA_DA_FAIXA_DA_ORDEM / 2.0f);
    /* A letra encolhe um pouco em 4:3, onde a célula cai de 61 para 46 */
    float letra = (celula >= 54.0f) ? LETRA_DA_FAIXA_DA_ORDEM : LETRA_DA_FAIXA_DA_ORDEM - 1.5f;
    float xDoCodigo = (celula >= 54.0f) ? 29.0f : 26.0f;

    /* O fundo vai de ponta a ponta: ele é a separação das duas metades (e tapa a linha de sobra
     * quando a altura da tela é ímpar), com um fio claro em cima e embaixo para ler como borda */
    DrawRectangleRec((Rectangle){ 0.0f, y, (float) larguraVirtual(), ALTURA_DA_FAIXA_DA_ORDEM }, COR_DA_TORRE);
    DrawRectangleRec((Rectangle){ 0.0f, y, (float) larguraVirtual(), 1.0f }, Fade(COR_CROMO, 0.22f));
    DrawRectangleRec((Rectangle){ 0.0f, y + ALTURA_DA_FAIXA_DA_ORDEM - 1.0f, (float) larguraVirtual(), 1.0f },
                     Fade(COR_CROMO, 0.22f));

    for (int i = 0; i < TOTAL_CARROS; i++) {
        Carro *carro = corrida->classificacao[i];
        int n = carro->numero % TOTAL_CARROS;
        float x = MARGEM_HUD + i * celula;
        float meio = noMeio(FONTE_FORTE, letra, y + ALTURA_DA_FAIXA_DA_ORDEM / 2.0f);
        Rectangle dentro = { x + 1.0f, y + 3.0f, celula - 2.0f, ALTURA_DA_FAIXA_DA_ORDEM - 6.0f };

        /* Qual humano é este carro (-1 = nenhum) */
        int humano = -1;
        for (int h = 0; h < corrida->totalDeHumanos; h++) {
            if (corrida->humanos[h] == carro) {
                humano = h;
            }
        }

        Color cor = COR_TEXTO_CLARO;
        if (humano >= 0) {
            caixaArredondada(dentro, RAIO_PEQUENO, COR_TEXTO_CLARO);
            cor = COR_GRAFITE;
        } else if (carroNoPit(carro)) {
            caixaArredondada(dentro, RAIO_PEQUENO, COR_AMARELO);
            cor = COR_GRAFITE;
        } else if (i == 0) {
            caixaArredondada(dentro, RAIO_PEQUENO, COR_VERMELHO);
        }
        if (corrida->voltaMaisRapida > 0.0f && carro->melhorVolta == corrida->voltaMaisRapida) {
            contornoArredondado(dentro, RAIO_PEQUENO, 1.5f, COR_ROXO_HUD);
        }

        escrever(FONTE_FORTE, TextFormat("%d", i + 1), x + 6.0f, meio, letra, cor);
        DrawRectangleRec((Rectangle){ x + 22.0f, y + 6.0f, LISTRA_DA_FAIXA_DA_ORDEM,
                                      ALTURA_DA_FAIXA_DA_ORDEM - 12.0f }, CORES_DAS_EQUIPES[n]);
        const char *nome = (humano >= 0) ? TextFormat("J%d", humano + 1) : CODIGOS[n];
        escrever(FONTE_FORTE, nome, x + xDoCodigo, meio, letra, cor);
    }
}

/* O painel da volta, no mesmo estilo da torre */
#define LARGURA_DA_VOLTA      150.0f

/* A plaquinha do jogador, encostada no painel da volta */
#define LARGURA_DA_PLACA      84.0f
#define ALTURA_DA_PLACA       70.0f

/*
 * desenharPlacaDoJogador
 * No alto da faixa, ao lado do painel da volta: quem é este jogador, em que posição está e em que
 * volta. Na tela dividida é ela que faz o papel da barra branca e do "VOLTA n/N" da torre, que
 * agora é compartilhada e não tem como marcar os dois. O fundo é a cor do carro, a mesma listra que
 * ele tem na faixa compartilhada, para o jogador achar a própria célula de olho.
 * Recebe: a corrida, o carro deste jogador e qual humano ele é.
 * Devolve: nada.
 */
static void desenharPlacaDoJogador(Corrida *corrida, Carro *jogador, int humano) {
    float x = larguraVirtual() - MARGEM_HUD - LARGURA_DA_VOLTA - 6.0f - LARGURA_DA_PLACA;
    float y = MARGEM_HUD;
    Rectangle placa = { x, y, LARGURA_DA_PLACA, ALTURA_DA_PLACA };
    float raio = raioDoTamanho(LARGURA_DA_PLACA, ALTURA_DA_PLACA);
    float meio = x + LARGURA_DA_PLACA / 2.0f;
    caixaArredondada(placa, raio, COR_DA_TORRE);
    /* A faixa de cor do carro no topo, como a listra da célula dele na faixa compartilhada */
    DrawRectangleRec((Rectangle){ x + 8.0f, y + 6.0f, LARGURA_DA_PLACA - 16.0f, 3.0f }, corDoCarro(jogador->numero));

    escreverCentralizado(FONTE_FORTE, TextFormat("JOGADOR %d", humano + 1), meio,
                         noMeio(FONTE_FORTE, 10.0f, y + 17.0f), 10.0f, Fade(COR_TEXTO_CLARO, 0.75f));
    escreverCentralizado(FONTE_TITULO, TextFormat("%dº", posicaoDoCarro(corrida, jogador)), meio,
                         noMeio(FONTE_TITULO, 28.0f, y + 38.0f), 28.0f, COR_TEXTO_CLARO);
    escreverCentralizado(FONTE_FORTE, TextFormat("VOLTA %d/%d", voltaAtual(jogador), corrida->totalVoltas), meio,
                         noMeio(FONTE_FORTE, 11.0f, y + 58.0f), 11.0f, Fade(COR_TEXTO_CLARO, 0.8f));
}

/*
 * desenharPainelDaVolta
 * A volta do jogador, no canto superior direito: o tempo da volta de agora em destaque,
 * a diferença para a melhor dele (verde ganhando tempo, só quando já há uma melhor), a
 * melhor volta (roxa se é o recorde da corrida) e os três setores, pintados como no F1.
 * Recebe: a corrida e o carro deste jogador.
 * Devolve: nada.
 */
static void desenharPainelDaVolta(Corrida *corrida, Carro *jogador) {
    float largura = LARGURA_DA_VOLTA;
    float x = larguraVirtual() - MARGEM_HUD - largura;
    float y = MARGEM_HUD;
    char texto[32];
    caixaArredondada((Rectangle){ x, y, largura, 70.0f }, raioDoTamanho(largura, 70.0f), COR_DA_TORRE);

    escrever(FONTE_TEXTO, "SUA VOLTA", x + 8.0f, noMeio(FONTE_TEXTO, 11.0f, y + 10.0f), 11.0f, Fade(COR_TEXTO_CLARO, 0.75f));
    if (jogador->temDiferenca) {
        int ganhando = jogador->diferenca <= 0.0f;
        escreverADireita(FONTE_FORTE, TextFormat("%+.3f", jogador->diferenca), x + largura - 8.0f, noMeio(FONTE_FORTE, 12.0f, y + 10.0f), 12.0f,
                         ganhando ? COR_VERDE_HUD : COR_VERMELHO);
    }
    float atual = 0.0f;
    if (corrida->contagemRegressiva <= 0.0f && !corrida->terminou) {
        atual = corrida->tempoDeCorrida - jogador->inicioDaVolta;
    }
    escrever(FONTE_TITULO, textoDoTempo(atual, texto, sizeof(texto)), x + 8.0f, y + 16.0f, 22.0f, COR_TEXTO_CLARO);
    if (jogador->melhorVolta > 0.0f) {
        Color corDaMelhor = (jogador->melhorVolta == corrida->voltaMaisRapida) ? COR_ROXO_HUD : Fade(COR_TEXTO_CLARO, 0.75f);
        escrever(FONTE_TEXTO, "MELHOR", x + 8.0f, noMeio(FONTE_TEXTO, 11.0f, y + 47.0f), 11.0f, Fade(COR_TEXTO_CLARO, 0.75f));
        escreverADireita(FONTE_FORTE, textoDoTempo(jogador->melhorVolta, texto, sizeof(texto)), x + largura - 8.0f,
                         noMeio(FONTE_FORTE, 11.5f, y + 47.0f), 11.5f, corDaMelhor);
    }

    /* Setores: três barras finas */
    float vao = 3.0f, larguraSetor = (largura - 16.0f - vao * (TOTAL_SETORES - 1)) / TOTAL_SETORES;
    for (int s = 0; s < TOTAL_SETORES; s++) {
        SituacaoDoSetor situacao = situacaoDoSetor(corrida, jogador, s);
        Color cor = (situacao == SETOR_PENDENTE) ? Fade(COR_TEXTO_CLARO, 0.15f) : corDoSetor(situacao);
        int agora = (s == jogador->setorAtual && !corrida->terminou && corrida->contagemRegressiva <= 0.0f);
        if (agora && situacao == SETOR_PENDENTE) {
            cor = Fade(COR_TEXTO_CLARO, 0.45f);
        }
        caixaArredondada((Rectangle){ x + 8.0f + s * (larguraSetor + vao), y + 60.0f, larguraSetor, 4.0f }, 2.0f, cor);   /* pílula */
    }
}

/*
 * pontoNoMostrador
 * Recebe: o centro do mostrador, o raio e a fração do arco
 *         (0 = 0 rpm, 1 = RPM_MAXIMO).
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
 * centroDoMostrador
 * Recebe: nada.
 * Devolve: o centro do mostrador na tela virtual (o VÁCUO também usa).
 */
static Vector2 centroDoMostrador(void) {
    return (Vector2){ larguraVirtual() - MARGEM_HUD - RAIO_VELOCIMETRO - 12,
                      alturaVirtual() - margemDeBaixo() - RAIO_VELOCIMETRO + 4 };
}

/*
 * desenharContaGiros
 * Minimalista: um arco fino com um tracinho a cada 500 rpm (os de mil
 * maiores, com o número por dentro), a faixa vermelha de RPM_DA_TROCA
 * (onde o câmbio sobe) até o fim e um
 * ponteiro branco curto, que corre por dentro do arco sem passar dele
 * e sem cobrir a velocidade no meio.
 * Recebe: o centro, o raio e a rotação (0 a 1).
 * Devolve: nada.
 */
static void desenharContaGiros(Vector2 centro, float raio, float giro) {
    float vermelho = RPM_DA_TROCA / RPM_MAXIMO;   /* a faixa vermelha começa onde o câmbio troca */
    desenharArco(centro, raio, 0.0f, 1.0f, 1.5f, COR_TRACO);
    desenharArco(centro, raio - 2.0f, vermelho, 1.0f, 5.0f, COR_VERMELHO);

    int totalTracos = (int) (RPM_MAXIMO / 500.0f);
    for (int t = 0; t <= totalTracos; t++) {
        float fracao = (float) t / totalTracos;
        int deMil = (t % 2 == 0);
        float comprimento = deMil ? 7.0f : 4.0f;
        Color cor = (fracao >= vermelho) ? COR_VERMELHO : COR_TRACO;
        DrawLineEx(pontoNoMostrador(centro, raio, fracao), pontoNoMostrador(centro, raio - comprimento, fracao),
                   deMil ? 2.0f : 1.0f, cor);
        if (deMil) {
            Vector2 ponto = pontoNoMostrador(centro, raio - 15.0f, fracao);
            escreverCentralizado(FONTE_TEXTO, TextFormat("%d", t / 2), ponto.x, noMeio(FONTE_TEXTO, 13.0f, ponto.y), 13.0f,
                                 Fade(COR_TEXTO_CLARO, 0.8f));
        }
    }

    float fracao = fminf(fmaxf(giro, 0.0f), 1.0f);
    DrawLineEx(pontoNoMostrador(centro, raio * INICIO_DO_PONTEIRO, fracao),
               pontoNoMostrador(centro, raio - 1.0f, fracao), 3.0f, COR_TEXTO_CLARO);
}

/*
 * desenharMarcha
 * A marcha, num círculo vermelho no vão de baixo do arco. Perto do
 * limite, fora da última marcha, ele pisca em branco (hora de trocar; o
 * câmbio do motor.c é automático e troca logo em seguida). Quando troca,
 * o círculo dá um pulo. Na contagem regressiva o carro está no grid: N.
 * Recebe: o centro do círculo, o motor, se a corrida já largou, se está de ré e qual jogador
 *         (o pulo é um por jogador: na tela dividida os dois trocam de marcha em momentos
 *         diferentes, e com um só o pulo de um saltaria no painel do outro).
 * Devolve: nada.
 */
static void desenharMarcha(Vector2 centro, const Motor *motor, int largou, int deRe, int jogador) {
    static int marchaAnterior[MAXIMO_DE_JOGADORES_LOCAIS];
    static float pulos[MAXIMO_DE_JOGADORES_LOCAIS];   /* de 1 (acabou de trocar) a 0 */
    int qual = (jogador >= 0 && jogador < MAXIMO_DE_JOGADORES_LOCAIS) ? jogador : 0;
    if (motor->marcha != marchaAnterior[qual]) {
        marchaAnterior[qual] = motor->marcha;
        pulos[qual] = 1.0f;
    }
    pulos[qual] = fmaxf(pulos[qual] - GetFrameTime() / DURACAO_DO_PULO, 0.0f);
    float pulo = pulos[qual];

    int pedeTroca = motor->giro * RPM_MAXIMO >= RPM_DA_LUZ_DE_TROCA && motor->marcha < TOTAL_MARCHAS - 1;
    int brancoNaPiscada = pedeTroca && fmodf((float) GetTime() * PISCADAS_POR_SEGUNDO, 2.0f) < 1.0f;
    Color fundo = brancoNaPiscada ? COR_TEXTO_CLARO : COR_VERMELHO;
    Color numero = brancoNaPiscada ? COR_VERMELHO : COR_TEXTO_CLARO;

    float raio = RAIO_DA_MARCHA * (1.0f + 0.2f * pulo);
    DrawCircleV(centro, raio, fundo);
    DrawRing(centro, raio, raio + 1.5f, 0.0f, 360.0f, 48, Fade(COR_TEXTO_CLARO, 0.8f));

    const char *texto = deRe ? "R" : (largou ? TextFormat("%d", motor->marcha + 1) : "N");
    float tamanho = 19.0f * (1.0f + 0.2f * pulo);
    escreverCentralizado(FONTE_TITULO, texto, centro.x + 1.0f, noMeio(FONTE_TITULO, tamanho, centro.y), tamanho, numero);
}

/*
 * desenharVelocimetro
 * O painel do motor, no canto inferior direito, tudo dentro de um
 * mostrador redondo: o conta-giros com ponteiro em volta, a velocidade
 * grande no meio e a marcha no vão de baixo do arco.
 * Recebe: o carro do jogador, a velocidade a mostrar, se a corrida já largou e qual jogador
 *         (para o pulo da marcha).
 * Devolve: nada.
 */
static void desenharVelocimetro(const Carro *jogador, int velocidade, int largou, int jogadorNumero) {
    const Motor *motor = &jogador->motor;
    Vector2 centro = centroDoMostrador();
    float raio = RAIO_VELOCIMETRO;

    desenharBrilho(centro.x, centro.y, raio + 18.0f, Fade(BLACK, 0.45f));
    desenharContaGiros(centro, raio, motor->giro);

    /* Velocidade no meio: amarela no vácuo, quando passa da máxima da ficha */
    const char *kmh = TextFormat("%d", velocidade);
    Color cor = (jogador->vacuo > 0.3f) ? COR_AMARELO : COR_TEXTO_CLARO;
    float y = centro.y - TAMANHO_VELOCIDADE * 0.62f;
    escreverCentralizado(FONTE_TITULO, kmh, centro.x + 1.5f, y + 1.5f, TAMANHO_VELOCIDADE, Fade(BLACK, 0.5f));
    escreverCentralizado(FONTE_TITULO, kmh, centro.x, y, TAMANHO_VELOCIDADE, cor);
    escreverCentralizado(FONTE_FORTE, "KM/H", centro.x, centro.y + 12.0f, 11.0f, Fade(COR_TEXTO_CLARO, 0.7f));

    Vector2 marcha = { centro.x, centro.y + raio * 0.72f };
    desenharMarcha(marcha, motor, largou, jogador->re > 0.0f, jogadorNumero);
}

/*
 * desenharArcoDoNitro
 * Um pedaço do arco do nitro, por fora do mostrador. Os ângulos são em graus,
 * com 0 apontando para a direita e crescendo para baixo.
 * Recebe: o centro, o raio, de que ângulo até que ângulo, a grossura e a cor.
 * Devolve: nada.
 */
static void desenharArcoDoNitro(Vector2 centro, float raio, float de, float ate, float grossura, Color cor) {
    int passos = (int) (fabsf(ate - de) / 3.0f) + 1;
    for (int i = 0; i < passos; i++) {
        float a0 = (de + (ate - de) * i / passos) * DEG2RAD;
        float a1 = (de + (ate - de) * (i + 1) / passos) * DEG2RAD;
        DrawLineEx((Vector2){ centro.x + cosf(a0) * raio, centro.y + sinf(a0) * raio },
                   (Vector2){ centro.x + cosf(a1) * raio, centro.y + sinf(a1) * raio }, grossura, cor);
    }
}

/*
 * desenharNitro
 * O nitro, colado no mostrador: um arco fino do lado direito, que enche de baixo
 * para cima, com um losango e um raio no meio. Queimando, ele acende; sem
 * o mínimo para ligar, fica apagado.
 * Recebe: o carro do jogador.
 * Devolve: nada.
 */
static void desenharNitro(const Carro *jogador) {
    Vector2 centro = centroDoMostrador();
    float raio = RAIO_VELOCIMETRO + DISTANCIA_DO_ARCO_NITRO;

    Color cor = COR_NITRO;
    if (jogador->nitroLigado) {
        cor = COR_NITRO_LIGADO;
    } else if (jogador->nitro < 0.10f) {
        cor = Fade(COR_NITRO, 0.5f);
    }

    /* O arco: a parte vazia em cima, escura, e a cheia subindo de baixo */
    float cheio = ANGULO_DO_ARCO_NITRO - 2.0f * ANGULO_DO_ARCO_NITRO * jogador->nitro;
    desenharArcoDoNitro(centro, raio, -ANGULO_DO_ARCO_NITRO, ANGULO_DO_ARCO_NITRO, 3.5f, Fade(COR_GRAFITE, 0.8f));
    if (jogador->nitro > 0.0f) {
        desenharArcoDoNitro(centro, raio, ANGULO_DO_ARCO_NITRO, cheio, 3.5f, cor);
    }

    /* As duas pontas, viradas para fora */
    for (int ponta = -1; ponta <= 1; ponta += 2) {
        float a = ponta * ANGULO_DO_ARCO_NITRO * DEG2RAD;
        DrawLineEx((Vector2){ centro.x + cosf(a) * (raio - 1.5f), centro.y + sinf(a) * (raio - 1.5f) },
                   (Vector2){ centro.x + cosf(a) * (raio + 5.0f), centro.y + sinf(a) * (raio + 5.0f) }, 2.5f,
                   (ponta > 0) ? cor : COR_TEXTO_FRACO);
    }

    /* O losango com o raio, no meio do arco */
    Vector2 meio = { centro.x + raio, centro.y };
    if (jogador->nitroLigado) {
        desenharBrilho(meio.x, meio.y, 20.0f, Fade(COR_NITRO, 0.7f));
    }
    DrawPoly(meio, 4, TAMANHO_DO_LOSANGO, 0.0f, COR_GRAFITE);
    DrawPolyLinesEx(meio, 4, TAMANHO_DO_LOSANGO, 0.0f, 1.5f, cor);
    Vector2 raioDoIcone[4] = { { meio.x + 1.6f, meio.y - 4.6f }, { meio.x - 1.8f, meio.y + 0.4f },
                               { meio.x + 1.8f, meio.y - 0.4f }, { meio.x - 1.6f, meio.y + 4.6f } };
    for (int i = 0; i < 3; i++) {
        DrawLineEx(raioDoIcone[i], raioDoIcone[i + 1], 1.5f, jogador->nitroLigado ? COR_TEXTO_CLARO : cor);
    }
}

/*
 * desenharVacuo
 * Em cima do velocímetro, "VÁCUO" aparece aos poucos conforme o jogador
 * cola atrás de outro carro (e some quando ele sai da esteira).
 * Recebe: a corrida.
 * Devolve: nada.
 */
static void desenharVacuo(Carro *jogador) {
    float forca = jogador->vacuo;
    if (forca < 0.05f) {
        return;
    }
    Vector2 centro = centroDoMostrador();
    escreverCentralizado(FONTE_FORTE, "VÁCUO", centro.x, centro.y - RAIO_VELOCIMETRO - 30.0f, 18.0f,
                         Fade(COR_AMARELO, forca));
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
 * avisoDoBox
 * Recebe: a pista e onde o carro está, em metros.
 * Devolve: 1 se ele está chegando ao pit lane, entre o aviso e o fim da zona de entrada.
 */
static int avisoDoBox(const Pista *pista, float metros) {
    float depois = fmodf(metros - pista->pit.aviso + pista->metrosDaVolta, pista->metrosDaVolta);
    float fim = fmodf(pista->pit.fimDaZona - pista->pit.aviso + pista->metrosDaVolta, pista->metrosDaVolta);
    return depois < fim;
}

/*
 * desenharAvisosDoPit
 * O que falta da parada obrigatória, o caminho até os boxes, o limite do pit lane e
 * o cronômetro da troca de pneus.
 * Recebe: a corrida.
 * Devolve: nada.
 */
static void desenharAvisosDoPit(Corrida *corrida, Carro *jogador) {
    if (!corrida->temParada || corrida->contagemRegressiva > 0.0f || jogador->tempoDeChegada > 0.0f) {
        return;
    }
    float centro = larguraVirtual() / 2.0f;
    float direita = larguraVirtual() - 24.0f;
    float metros = metrosNaVolta(jogador);
    /* Na faixa da tela dividida o velocímetro sobe (a faixa tem metade da altura) e os avisos da
     * direita batem nele: o bloco dos pneus vai para a esquerda, onde a torre deixou de ficar, e o
     * estado da parada sobe para debaixo do painel da volta. Na tela cheia, os Y são os de sempre. */
    int estreita = naFaixaEstreita();
    float yEstado = estreita ? 92.0f : 134.0f;
    float yPneus = estreita ? 128.0f : 158.0f;
    float yRodas = estreita ? 124.0f : 154.0f;
    float yBarra = estreita ? 148.0f : 178.0f;

    /* Os pneus, só quando eles gastam: a barra esvazia, e o verde vira vermelho */
    if (jogador->gastaPneu) {
        float sobra = 1.0f - jogador->desgaste;
        Color cor = (sobra > 0.5f) ? COR_VERDE_HUD : (sobra > 0.25f) ? COR_AMARELO : COR_VERMELHO;
        float larguraDaBarra = 150.0f;
        float xDaBarra = estreita ? MARGEM_HUD + 30.0f : direita - larguraDaBarra;
        escrever(FONTE_FORTE, TextFormat("PNEUS %d%%", (int) (sobra * 100.0f + 0.5f)), xDaBarra, yPneus, 15.0f, cor);
        /* As quatro rodas, vistas de cima: a que estourou fica vermelha */
        for (int roda = 0; roda < 4; roda++) {
            Rectangle quadro = { xDaBarra - 30.0f + (float) (roda % 2) * 13.0f, yRodas + (float) (roda / 2) * 17.0f, 9.0f, 14.0f };
            caixaArredondada(quadro, 3.0f, (jogador->pneuEstourado & (1 << roda)) ? COR_VERMELHO : Fade(COR_TEXTO_CLARO, 0.55f));
        }
        caixaArredondada((Rectangle){ xDaBarra, yBarra, larguraDaBarra, 8.0f }, 4.0f, Fade(COR_GRAFITE, 0.85f));
        caixaArredondada((Rectangle){ xDaBarra, yBarra, larguraDaBarra * sobra, 8.0f }, 4.0f, cor);
        if (jogador->pneuEstourado && !carroNoPit(jogador)) {   /* o aviso pisca */
            float pisca = 0.55f + 0.45f * sinf((float) GetTime() * 9.0f);
            int estourados = quantosPneusEstourados(jogador);
            escreverCentralizado(FONTE_TITULO, (estourados == 1) ? "PNEU ESTOURADO!" : TextFormat("%d PNEUS ESTOURADOS!", estourados),
                                 centro, 100.0f, 30.0f, Fade(COR_VERMELHO, pisca));
            escreverCentralizado(FONTE_FORTE, "VÁ PARA OS BOXES", centro, 134.0f, 18.0f, COR_TEXTO_CLARO);
        } else if (sobra <= 0.25f && !carroNoPit(jogador)) {
            escreverCentralizado(FONTE_FORTE, "PNEUS NO FIM: PARE NOS BOXES", centro, 108.0f, 20.0f, COR_VERMELHO);
        }
    }

    /* O estado da parada, embaixo do painel da volta */
    if (jogador->pitFeito) {
        escreverADireita(FONTE_FORTE, TextFormat("PIT FEITO  %.2f s", jogador->ultimaTroca), direita, yEstado, 16.0f, COR_VERDE_HUD);
    } else if (voltaAtual(jogador) >= corrida->totalVoltas) {
        escreverADireita(FONTE_FORTE, "ÚLTIMA VOLTA: PARE OU SERÁ DESCLASSIFICADO", direita, yEstado, 16.0f, COR_VERMELHO);
    } else {
        escreverADireita(FONTE_FORTE, "PIT OBRIGATÓRIO", direita, yEstado, 16.0f, COR_AMARELO);
    }

    if (jogador->estadoDoPit == PIT_TROCANDO) {
        escreverCentralizado(FONTE_TITULO, TextFormat("%.2f", jogador->tempoDaTroca), centro, 70.0f, 52.0f, COR_AMARELO);
        escreverCentralizado(FONTE_FORTE, "GUIDO TROCANDO OS PNEUS!", centro, 130.0f, 20.0f, COR_TEXTO_CLARO);
    } else if (carroNoPit(jogador)) {
        if (dentroDoLimiteDoPit(corrida->pista, metros)) {
            escreverCentralizado(FONTE_FORTE, "LIMITE DOS BOXES: 100 KM/H", centro, 70.0f, 24.0f, COR_AMARELO);
        } else {
            escreverCentralizado(FONTE_FORTE, "PIT LANE", centro, 70.0f, 24.0f, COR_TEXTO_CLARO);
        }
    } else if (!jogador->pitFeito && avisoDoBox(corrida->pista, metros)) {
        escreverCentralizado(FONTE_TITULO, (corrida->pista->pit.lado < 0) ? "BOX PELA ESQUERDA" : "BOX PELA DIREITA", centro, 70.0f, 30.0f, COR_AMARELO);
    }
}

/* ---------- A placa de controles da largada ---------- */
#define ALTURA_DA_PLACA_DE_CONTROLES  52.0f
#define LETRA_DO_COMANDO              12.0f
#define VAO_ENTRE_GRUPOS              16.0f

/*
 * umGrupoDeControles
 * Uma dupla "tecla(s) + nome do comando" da placa da largada. Mede quando quadro é NULL e desenha
 * quando não é, para a placa poder se medir antes de saber onde se centralizar.
 * Recebe: o x onde começa, o y da fila, os nomes das teclas (até 4, terminando em NULL), o nome do
 *         comando e 1 para desenhar (0 só mede).
 * Devolve: a largura que o grupo ocupa.
 */
static float umGrupoDeControles(float x, float y, const char *teclas[], const char *comando, int desenhar) {
    float largura = 0.0f;
    for (int t = 0; teclas[t] != NULL; t++) {
        if (desenhar) {
            largura += desenharTecla(x + largura, y, teclas[t]) + 3.0f;
        } else {
            largura += larguraDaTecla(teclas[t]) + 3.0f;
        }
    }
    largura += 3.0f;
    if (desenhar) {
        escrever(FONTE_FORTE, comando, x + largura, noMeio(FONTE_FORTE, LETRA_DO_COMANDO, meioDaTecla(y)),
                 LETRA_DO_COMANDO, COR_TEXTO_FRACO);
    }
    return largura + larguraDoTexto(FONTE_FORTE, comando, LETRA_DO_COMANDO);
}

/*
 * desenharPlacaDeControles
 * Na largada, dentro da faixa de cada um: quem ele é e com o que ele joga. É a resposta para "de
 * quem é o WASD?" — com dois no mesmo aparelho ninguém adivinha, e foi justamente isso que faltou
 * no primeiro teste. Some quando a corrida larga.
 *
 * As teclas saem das configurações (a coluna daquele jogador), então remapear já aparece aqui; com
 * controle, saem os botões daquele controle. É o mesmo caminho da tela COMO JOGAR.
 * Recebe: a corrida, qual humano e de onde vêm os comandos dele.
 * Devolve: nada.
 */
void desenharPlacaDeControles(Corrida *corrida, int humano, const EntradaDoJogador *entrada) {
    if (corrida->totalDeHumanos < 2 || corrida->tempoDeCorrida > 0.0f) {
        return;   /* largou: a placa sai e a tela fica para a corrida */
    }
    const Configuracoes *config = configuracoesAtuais();
    int comControle = (entrada != NULL && controleDoJogador(entrada) >= 0);
    int coluna = (entrada != NULL && entrada->colunaDeTeclas >= 0) ? entrada->colunaDeTeclas : COLUNA_TECLA_1;
    int colunaDoBotao = COLUNA_BOTAO_1;

    /* Os nomes de cada grupo. Com controle, o analógico esquerdo faz o lugar das quatro setas */
    const char *direcao[5];
    if (comControle) {
        direcao[0] = "@ANALOG E";
        direcao[1] = NULL;
    } else {
        direcao[0] = nomeDaTecla(config->comandos[ACAO_ACELERAR][coluna]);
        direcao[1] = nomeDaTecla(config->comandos[ACAO_FREAR][coluna]);
        direcao[2] = nomeDaTecla(config->comandos[ACAO_ESQUERDA][coluna]);
        direcao[3] = nomeDaTecla(config->comandos[ACAO_DIREITA][coluna]);
        direcao[4] = NULL;
    }
    const char *doNitro[2] = { comControle ? nomeDoBotao(config->comandos[ACAO_NITRO][colunaDoBotao])
                                           : nomeDaTecla(config->comandos[ACAO_NITRO][coluna]), NULL };
    const char *doFreio[2] = { comControle ? nomeDoBotao(config->comandos[ACAO_FREIO_DE_MAO][colunaDoBotao])
                                           : nomeDaTecla(config->comandos[ACAO_FREIO_DE_MAO][coluna]), NULL };

    /* Mede a fila toda para centralizar, depois desenha */
    float fila = umGrupoDeControles(0.0f, 0.0f, direcao, "DIRIGIR", 0) + VAO_ENTRE_GRUPOS
               + umGrupoDeControles(0.0f, 0.0f, doNitro, "NITRO", 0) + VAO_ENTRE_GRUPOS
               + umGrupoDeControles(0.0f, 0.0f, doFreio, "FREIO DE MÃO", 0);
    float larguraDaPlaca = fila + 24.0f;
    float x = roundf((larguraVirtual() - larguraDaPlaca) / 2.0f);
    float y = alturaVirtual() - margemDeBaixo() - ALTURA_DA_PLACA_DE_CONTROLES;

    caixaArredondada((Rectangle){ x, y, larguraDaPlaca, ALTURA_DA_PLACA_DE_CONTROLES },
                     raioDoTamanho(larguraDaPlaca, ALTURA_DA_PLACA_DE_CONTROLES), COR_DA_TORRE);
    escrever(FONTE_FORTE, TextFormat("JOGADOR %d", humano + 1), x + 12.0f, y + 5.0f, 11.0f,
             corDoCarro(carroDoHumano(corrida, humano)->numero));

    float noFio = x + 12.0f, yFio = y + 21.0f;
    noFio += umGrupoDeControles(noFio, yFio, direcao, "DIRIGIR", 1) + VAO_ENTRE_GRUPOS;
    noFio += umGrupoDeControles(noFio, yFio, doNitro, "NITRO", 1) + VAO_ENTRE_GRUPOS;
    umGrupoDeControles(noFio, yFio, doFreio, "FREIO DE MÃO", 1);
}

/*
 * desenharHud
 * O HUD da corrida, nos quatro cantos da tela: posições (em cima à
 * esquerda), a volta (em cima à direita), o mapa (embaixo à esquerda)
 * e o painel do motor (embaixo à direita). Na tela dividida é chamado uma vez por jogador,
 * dentro da faixa dele (definirAreaDaInterface), e tudo sai do carro daquele jogador.
 * Recebe: a corrida e qual humano (0 = o jogador 1, o de sempre).
 * Devolve: nada.
 */
void desenharHudDoJogador(Corrida *corrida, int humano) {
    Carro *jogador = carroDoHumano(corrida, humano);
    if (corrida->totalDeHumanos > 1) {
        /* A classificação é compartilhada (a faixa sobre a divisão); o que é de cada um fica aqui */
        desenharPlacaDoJogador(corrida, jogador, humano);
    } else {
        desenharPainelDePosicoes(corrida, jogador);
    }
    desenharPainelDaVolta(corrida, jogador);
    desenharMiniMapa(corrida, jogador);
    desenharVelocimetro(jogador, velocidadeEmKmh(jogador), corrida->contagemRegressiva <= 0.0f, humano);
    desenharVacuo(jogador);
    desenharNitro(jogador);

    if (estaForaDaPista(jogador)) {
        escreverCentralizado(FONTE_FORTE, "FORA DA PISTA", larguraVirtual() / 2.0f, alturaVirtual() * 0.30f,
                             20.0f, COR_AMARELO);
    }
    desenharAvisosDoPit(corrida, jogador);
    desenharContagemRegressiva(corrida);
}

/*
 * desenharAvisoParaContinuar
 * Na festa do pódio, no lugar do painel de chegada: a colocação em letras grandes e, embaixo, o aviso piscando para
 * apertar qualquer botão (que leva à tela de resultado).
 * Recebe: a corrida (o jogador já no pódio).
 * Devolve: nada.
 */
void desenharAvisoParaContinuar(Corrida *corrida) {
    float centro = larguraVirtual() / 2.0f;
    float pisca = 0.65f + 0.35f * sinf((float) GetTime() * 4.0f);
    const char *aviso = "APERTE QUALQUER BOTÃO PARA CONTINUAR";
    float largura = larguraDoTexto(FONTE_FORTE, aviso, 20.0f) + 56.0f;
    Rectangle caixa = { centro - largura / 2.0f, alturaVirtual() - 96.0f, largura, 38.0f };
    caixaArredondada(caixa, raioDoTamanho(caixa.width, caixa.height), Fade(COR_GRAFITE, 0.88f));
    escreverCentralizado(FONTE_FORTE, aviso, centro, caixa.y + 9.0f, 20.0f, Fade(COR_TEXTO_CLARO, pisca));
    (void) corrida;
}

/*
 * desenharPainelDoMotor
 * Só o painel do motor da corrida (o mostrador com a velocidade, o conta-giros, a marcha e o
 * nitro), no mesmo canto: o mundo aberto usa o mesmo.
 * Recebe: o carro (o da corrida que dirige o carro livre) e a velocidade a mostrar, em km/h.
 * Devolve: nada.
 */
void desenharPainelDoMotor(const Carro *carro, int kmh) {
    desenharVelocimetro(carro, kmh, 1, 0);   /* o mundo aberto é de um jogador: o pulo da marcha é o 0 */
    desenharNitro(carro);
}

/*
 * desenharDicaDoFimDeCorrida
 * O pé do cartão de chegada. Na tela dividida, quem chega primeiro vê o cartão enquanto o outro
 * ainda corre — e aí a dica do ENTER não vale, porque a tela de resultado só abre quando os dois
 * terminam.
 * Recebe: a corrida, o centro do cartão e a altura onde a dica começa.
 * Devolve: nada.
 */
static void desenharDicaDoFimDeCorrida(Corrida *corrida, float centro, float cursor) {
    if (!todosOsHumanosChegaram(corrida)) {
        escreverCentralizado(FONTE_FORTE, "ESPERANDO O OUTRO JOGADOR...", centro,
                             noMeio(FONTE_FORTE, 14.0f, meioDaTecla(cursor - 2.0f)), 14.0f, COR_TEXTO_FRACO);
        return;
    }
    const char *tecla = nomeNoControle("ENTER");   /* "A" com o controle ligado */
    float larguraDica = larguraDaTecla(tecla) + 8.0f + larguraDoTexto(FONTE_FORTE, "VER O RESULTADO", 14.0f);
    float xDica = roundf((centro - larguraDica / 2.0f) / 4.0f) * 4.0f;
    xDica += desenharTecla(xDica, cursor - 2.0f, tecla) + 8.0f;
    escrever(FONTE_FORTE, "VER O RESULTADO", xDica, noMeio(FONTE_FORTE, 14.0f, meioDaTecla(cursor - 2.0f)), 14.0f, COR_TEXTO_FRACO);
}

/*
 * desenharFimDeCorrida
 * Painel de chegada: posição, tempo total, tempo de cada volta e a
 * melhor volta. Na tela dividida vai só a melhor volta, para o cartão caber na faixa.
 * A tabela com todos os carros fica na tela de resultado (menu.c).
 * Recebe: a corrida (já terminada) e qual humano: na tela dividida, cada faixa mostra o cartão
 *         do jogador dela, e quem chega primeiro já o vê enquanto o outro ainda corre.
 * Devolve: nada.
 */
void desenharFimDeCorridaDoJogador(Corrida *corrida, int humano) {
    /* Na grade de 4: o cartão com 24 de folga e cantos de 24; o título dentro dele, a 16 da borda,
     * com cantos de 24 - 16 = 8.
     *
     * Na tela dividida o cartão é COMPACTO: a faixa tem ~290 de altura virtual e a lista de voltas
     * (32 por volta) fazia o cartão ir de 328 a 488, estourando a faixa no difícil. No lugar dela
     * entra uma linha só com a melhor volta — a lista inteira continua na tela de resultado. */
    const float largura = 384.0f, folga = 24.0f, linhaDaVolta = 32.0f, borda = 16.0f;
    int compacto = naFaixaEstreita();   /* é a altura que manda: o cartão tem de caber onde está */
    const float alturaDoTitulo = compacto ? 48.0f : 64.0f;
    const float corpoDoTitulo = compacto ? 32.0f : 40.0f;
    Carro *jogador = carroDoHumano(corrida, humano);
    int desclassificado = corrida->desclassificadoDosHumanos[humano];
    int posicaoFinal = corrida->posicaoFinalDosHumanos[humano];
    char tempo[32];
    int temObservacao = desclassificado || jogador->ultimaTroca > 0.0f;

    float alturaDosTempos = compacto ? linhaDaVolta                        /* uma linha: a melhor volta */
                                     : 16.0f + corrida->totalVoltas * linhaDaVolta;
    float altura = borda + alturaDoTitulo + (compacto ? 20.0f : 24.0f)     /* título */
                 + 40.0f + (temObservacao ? 20.0f : 0.0f)                  /* tempo total */
                 + alturaDosTempos
                 + (compacto ? 22.0f : 24.0f) + 24.0f + folga;             /* dica */
    float x = roundf((larguraVirtual() - largura) / 8.0f) * 4.0f;
    /* Na tela cheia o cartão sobe 24 para sobrar espaço embaixo; na faixa não há esse espaço */
    float y = roundf((alturaVirtual() - altura) / 8.0f) * 4.0f - (compacto ? 0.0f : 24.0f);
    float centro = x + largura / 2.0f;
    float raio = raioDoTamanho(largura, altura);

    Rectangle cartao = { x, y, largura, altura };
    caixaArredondada(cartao, raio, Fade(COR_GRAFITE, 0.92f));
    contornoArredondado(cartao, raio, 1.0f, Fade(COR_CROMO, 0.16f));

    /* O título, numa faixa vermelha dentro do cartão, com o quadriculado fino embaixo */
    Rectangle faixa = { x + borda, y + borda, largura - 2.0f * borda, alturaDoTitulo };
    caixaArredondada(faixa, raioDeDentro(raio, borda), COR_VERMELHO);
    desenharQuadriculado(faixa.x + 8.0f, faixa.y + alturaDoTitulo - 8.0f, faixa.width - 16.0f, 4.0f, 4.0f);
    const char *titulo = (posicaoFinal == 1) ? "KATCHAU!" : TextFormat("%dº LUGAR", posicaoFinal);
    if (desclassificado) {
        titulo = "DESCLASSIFICADO";
    }
    escreverCentralizado(FONTE_TITULO, titulo, centro, noMeio(FONTE_TITULO, corpoDoTitulo, faixa.y + (alturaDoTitulo - 8.0f) / 2.0f), corpoDoTitulo, COR_TEXTO_CLARO);   /* no meio, acima do quadriculado */
    float cursor = faixa.y + alturaDoTitulo + (compacto ? 20.0f : 24.0f);

    /* Tempo total */
    formatarTempo(jogador->tempoDeChegada, tempo, sizeof(tempo));
    escrever(FONTE_FORTE, "TEMPO TOTAL", x + folga, noMeio(FONTE_FORTE, 14.0f, cursor + 16.0f), 14.0f, COR_TEXTO_FRACO);
    escreverADireita(FONTE_TITULO, tempo, x + largura - folga, noMeio(FONTE_TITULO, 32.0f, cursor + 16.0f), 32.0f, COR_AMARELO);
    cursor += 40.0f;
    if (desclassificado) {
        escrever(FONTE_FORTE, "VOCÊ NÃO FEZ A PARADA OBRIGATÓRIA", x + folga, cursor, 12.0f, COR_AMARELO);
        cursor += 20.0f;
    } else if (jogador->ultimaTroca > 0.0f) {
        escrever(FONTE_FORTE, TextFormat("TROCA DE PNEUS DO GUIDO  %.2f s", jogador->ultimaTroca), x + folga, cursor, 12.0f, COR_TEXTO_FRACO);
        cursor += 20.0f;
    }
    /* Qual foi a melhor volta dele */
    int melhorVolta = 0;
    for (int i = 0; i < corrida->totalVoltas; i++) {
        if (jogador->temposDeVolta[i] < jogador->temposDeVolta[melhorVolta]) {
            melhorVolta = i;
        }
    }

    if (compacto) {
        /* Uma linha só: a melhor volta. A lista inteira não cabe na faixa e está na tela de resultado */
        float meioDaLinha = cursor + linhaDaVolta / 2.0f;
        DrawRectangleRec((Rectangle){ x + folga, cursor, largura - 2.0f * folga, 1.0f }, Fade(COR_TEXTO_CLARO, 0.10f));
        escrever(FONTE_FORTE, TextFormat("MELHOR VOLTA (%d)", melhorVolta + 1), x + folga,
                 noMeio(FONTE_FORTE, 16.0f, meioDaLinha), 16.0f, COR_TEXTO_FRACO);
        formatarTempo(jogador->temposDeVolta[melhorVolta], tempo, sizeof(tempo));
        escreverADireita(FONTE_FORTE, tempo, x + largura - folga, noMeio(FONTE_FORTE, 16.0f, meioDaLinha), 16.0f, COR_ROXO_HUD);
        cursor += linhaDaVolta;
        DrawRectangleRec((Rectangle){ x + folga, cursor, largura - 2.0f * folga, 1.0f }, Fade(COR_TEXTO_CLARO, 0.10f));
        cursor += 22.0f;
        desenharDicaDoFimDeCorrida(corrida, centro, cursor);
        return;
    }

    cursor += 16.0f;

    /* As voltas: fios finos separando; a melhor em roxo, com o selo em pílula */
    for (int i = 0; i < corrida->totalVoltas; i++) {
        int ehMelhor = (i == melhorVolta);
        DrawRectangleRec((Rectangle){ x + folga, cursor, largura - 2.0f * folga, 1.0f }, Fade(COR_TEXTO_CLARO, 0.10f));
        float meioDaLinha = cursor + linhaDaVolta / 2.0f;
        escrever(FONTE_FORTE, TextFormat("VOLTA %d", i + 1), x + folga, noMeio(FONTE_FORTE, 16.0f, meioDaLinha), 16.0f, COR_TEXTO_FRACO);
        if (ehMelhor) {
            caixaArredondada((Rectangle){ x + folga + 80.0f, meioDaLinha - 8.0f, 64.0f, 16.0f }, RAIO_PEQUENO, COR_ROXO_HUD);
            escreverCentralizado(FONTE_FORTE, "MELHOR", x + folga + 112.0f, noMeio(FONTE_FORTE, 12.0f, meioDaLinha), 12.0f, COR_TEXTO_CLARO);
        }
        formatarTempo(jogador->temposDeVolta[i], tempo, sizeof(tempo));
        escreverADireita(FONTE_FORTE, tempo, x + largura - folga, noMeio(FONTE_FORTE, 16.0f, meioDaLinha), 16.0f, ehMelhor ? COR_ROXO_HUD : COR_TEXTO_CLARO);
        cursor += linhaDaVolta;
    }
    DrawRectangleRec((Rectangle){ x + folga, cursor, largura - 2.0f * folga, 1.0f }, Fade(COR_TEXTO_CLARO, 0.10f));
    cursor += 24.0f;
    desenharDicaDoFimDeCorrida(corrida, centro, cursor);
}

