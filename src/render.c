/*
 * render.c
 * ------------------------------------------------------------------
 * Estrada em pseudo-3D e carros. Veja render.h para a ideia geral.
 *
 * Como funciona a projeção (a "mágica" do pseudo-3D):
 *
 *   A câmera fica a ALTURA_CAMERA acima do chão. Um ponto do chão que
 *   está a uma distância z à frente aparece na tela multiplicado por
 *
 *        escala = PROFUNDIDADE_CAMERA / z
 *
 *   Longe (z grande) -> escala pequena -> ponto perto do horizonte
 *   e estrada estreita. Perto (z pequeno) -> escala grande -> ponto
 *   embaixo na tela e estrada larga. É só isso: uma divisão por z.
 *
 * Como funcionam as curvas:
 *
 *   A estrada nunca vira de verdade. Cada segmento empurra o centro
 *   da estrada um pouco para o lado, e esse empurrão vai crescendo
 *   (dx += curvatura). Somando os empurrões, os segmentos distantes
 *   ficam bem deslocados e a estrada parece curvar.
 * ------------------------------------------------------------------
 */

#include "raylib.h"
#include "render.h"

/* ---------- Câmera ---------- */
#define DISTANCIA_VISIVEL    100       /* quantos segmentos à frente são desenhados */
#define ALTURA_CAMERA        1000.0f   /* altura da câmera acima do chão */
#define PROFUNDIDADE_CAMERA  0.84f     /* "zoom" da lente (campo de visão de ~100 graus) */
#define DISTANCIA_MINIMA     1.0f      /* nada é projetado mais perto que isso (evita dividir por zero) */
#define DISTANCIA_CAMERA     (ALTURA_CAMERA * PROFUNDIDADE_CAMERA)   /* câmera atrás do carro: 840 unidades */
#define DISTANCIA_MINIMA_RIVAL 200.0f  /* rival mais perto da câmera que isso não é desenhado */

/* ---------- Carro (em pixels, no tamanho do carro do jogador) ---------- */
#define LARGURA_CARRO        170
#define ALTURA_CARROCERIA    58
#define MARGEM_CARRO         18        /* distância da base do carro até o pé da tela */
#define INCLINACAO_CABINE    8.0f      /* quanto a cabine desliza ao virar */
#define TREPIDACAO_MAXIMA    4         /* pulo máximo na grama */

/* ---------- Pista ---------- */
#define LARGURA_PISTA        2000.0f   /* metade da largura da estrada, em unidades de mundo */
#define FATOR_CURVA          4.0f      /* converte a curvatura (graus) em empurrão lateral na tela */
#define SEGMENTOS_POR_FAIXA  3         /* zebras e faixas trocam de cor a cada 3 segmentos */
#define PROPORCAO_ZEBRA      1.15f     /* a zebra é 15% mais larga que a estrada */
#define PROPORCAO_FAIXA      0.03f     /* largura da faixa branca em relação à estrada */
#define POSICAO_FAIXA        0.33f     /* faixas a 1/3 do centro, dividindo 3 pistas */

/* ---------- Cores ---------- */
#define COR_CEU_ALTO         (Color){  40, 110, 210, 255 }
#define COR_CEU_HORIZONTE    (Color){ 160, 210, 255, 255 }
#define COR_GRAMA_CLARA      (Color){  16, 170,  16, 255 }
#define COR_GRAMA_ESCURA     (Color){   0, 154,   0, 255 }
#define COR_ASFALTO_CLARO    (Color){ 107, 107, 107, 255 }
#define COR_ASFALTO_ESCURO   (Color){  99,  99,  99, 255 }
#define COR_ZEBRA_CLARA      (Color){ 245, 245, 245, 255 }
#define COR_ZEBRA_ESCURA     (Color){ 200,  20,  20, 255 }
#define COR_FAIXA            (Color){ 230, 230, 230, 255 }
#define COR_LARGADA          (Color){ 240, 240, 240, 255 }
#define COR_VIDRO            (Color){  70, 110, 150, 255 }
#define COR_PNEU             (Color){  25,  25,  25, 255 }
#define COR_PARACHOQUE       (Color){  60,  60,  60, 255 }
#define COR_LANTERNA         (Color){ 150,  20,  20, 255 }
#define COR_LANTERNA_FREIO   (Color){ 255,  90,  60, 255 }

/* Cores de cada carro, pelo número: 0 é o jogador, 1 a 7 os rivais */
static const Color CORES_CARROCERIA[TOTAL_CARROS] = {
    { 220,  30,  40, 255 },   /* 0 Você: vermelho */
    { 240, 200,  20, 255 },   /* 1 Barão: amarelo */
    { 200,  60, 220, 255 },   /* 2 Neon: roxo */
    {  30,  90, 220, 255 },   /* 3 Trovão: azul */
    { 240, 130,  20, 255 },   /* 4 Zé Pistão: laranja */
    {  60, 190, 170, 255 },   /* 5 Fuscão: verde-água */
    { 250, 120, 180, 255 },   /* 6 Bolinha: rosa */
    { 150,  80,  40, 255 },   /* 7 Ferrugem: marrom */
};
static const Color CORES_CABINE[TOTAL_CARROS] = {
    { 170,  20,  30, 255 },
    { 190, 150,  10, 255 },
    { 150,  40, 170, 255 },
    {  20,  60, 170, 255 },
    { 190, 100,  10, 255 },
    {  40, 150, 130, 255 },
    { 210,  80, 140, 255 },
    { 110,  60,  30, 255 },
};

/* Onde um ponto da pista aparece na tela. */
typedef struct {
    float x;         /* centro da estrada na tela (pixels) */
    float y;         /* altura na tela (pixels) */
    float largura;   /* metade da largura da estrada na tela (pixels) */
} PontoNaTela;

/* Tudo o que a câmera enxerga neste quadro, calculado uma vez
 * e usado tanto para desenhar a estrada quanto os carros. */
typedef struct {
    PosicaoNaPista camera;
    float xCamera;
    Segmento *segmentos[DISTANCIA_VISIVEL];   /* os próximos nós da lista */
    float xCurva[DISTANCIA_VISIVEL];          /* empurrão da curva no começo de cada segmento */
    float dxCurva[DISTANCIA_VISIVEL];         /* quanto o empurrão cresce dentro do segmento */
    PontoNaTela perto[DISTANCIA_VISIVEL];     /* começo do segmento na tela */
    PontoNaTela longe[DISTANCIA_VISIVEL];     /* fim do segmento na tela */
} Visao;

/*
 * projetar
 * Converte um ponto do chão (coordenadas de mundo) em coordenadas de tela.
 * Recebe: x do centro da estrada, distância z à frente da câmera e x da câmera.
 * Devolve: a posição e a largura da estrada naquele ponto da tela.
 */
static PontoNaTela projetar(float xMundo, float z, float xCamera) {
    float metadeLargura = GetScreenWidth() / 2.0f;
    float metadeAltura = GetScreenHeight() / 2.0f;
    float escala = PROFUNDIDADE_CAMERA / z;

    PontoNaTela ponto;
    ponto.x = metadeLargura + escala * (xMundo - xCamera) * metadeLargura;
    ponto.y = metadeAltura + escala * ALTURA_CAMERA * metadeAltura;
    ponto.largura = escala * LARGURA_PISTA * metadeLargura;
    return ponto;
}

/*
 * calcularVisao
 * Anda pela lista a partir do segmento da câmera, calculando onde cada
 * um dos próximos DISTANCIA_VISIVEL segmentos aparece na tela e somando
 * o empurrão das curvas.
 * Recebe: a visão a preencher, a posição da câmera e o deslocamento
 *         lateral (0 = centro, -1 = borda esquerda, +1 = borda direita).
 * Devolve: nada.
 */
static void calcularVisao(Visao *visao, PosicaoNaPista camera, float deslocamentoLateral) {
    visao->camera = camera;
    visao->xCamera = deslocamentoLateral * LARGURA_PISTA;

    /* Empurrão lateral da curva. Começa "descontando" o pedaço do
     * segmento atual que a câmera já andou, para a curva não dar trancos. */
    float fracaoAndada = camera.distanciaNoSegmento / COMPRIMENTO_SEGMENTO;
    float xCurva = 0.0f;
    float dxCurva = -camera.segmento->curvatura * FATOR_CURVA * fracaoAndada;

    Segmento *atual = camera.segmento;
    for (int i = 0; i < DISTANCIA_VISIVEL; i++) {
        float zPerto = i * COMPRIMENTO_SEGMENTO - camera.distanciaNoSegmento;
        float zLonge = (i + 1) * COMPRIMENTO_SEGMENTO - camera.distanciaNoSegmento;
        if (zPerto < DISTANCIA_MINIMA) {
            zPerto = DISTANCIA_MINIMA;   /* o começo do segmento atual está atrás da câmera */
        }

        visao->segmentos[i] = atual;
        visao->xCurva[i] = xCurva;
        visao->dxCurva[i] = dxCurva;
        visao->perto[i] = projetar(xCurva, zPerto, visao->xCamera);
        visao->longe[i] = projetar(xCurva + dxCurva, zLonge, visao->xCamera);

        xCurva += dxCurva;
        dxCurva += atual->curvatura * FATOR_CURVA;

        atual = proximoSegmento(atual);   /* lista circular: depois do último vem a largada */
    }
}

/*
 * desenharTrapezio
 * Desenha um trapézio "deitado": a base de baixo é a parte mais perto,
 * a de cima é a mais longe. Cada pedaço de estrada, zebra e faixa é um.
 * A raylib só desenha triângulos, então são dois triângulos.
 * Recebe: centro e meia-largura embaixo e em cima, e a cor.
 * Devolve: nada.
 */
static void desenharTrapezio(float xPerto, float yPerto, float metadePerto,
                             float xLonge, float yLonge, float metadeLonge, Color cor) {
    Vector2 baixoEsquerda = { xPerto - metadePerto, yPerto };
    Vector2 baixoDireita  = { xPerto + metadePerto, yPerto };
    Vector2 cimaEsquerda  = { xLonge - metadeLonge, yLonge };
    Vector2 cimaDireita   = { xLonge + metadeLonge, yLonge };

    /* A raylib exige os vértices em sentido anti-horário na tela */
    DrawTriangle(cimaEsquerda, baixoEsquerda, baixoDireita, cor);
    DrawTriangle(cimaEsquerda, baixoDireita, cimaDireita, cor);
}

/*
 * ehSegmentoClaro
 * Diz se o segmento usa as cores claras ou escuras. As cores alternam
 * a cada SEGMENTOS_POR_FAIXA segmentos, o que dá a sensação de velocidade.
 * Recebe: o segmento.
 * Devolve: 1 se for claro, 0 se for escuro.
 */
static int ehSegmentoClaro(Segmento *segmento) {
    return (segmento->indice / SEGMENTOS_POR_FAIXA) % 2 == 0;
}

/*
 * desenharGrama
 * Pinta a faixa de grama de um segmento: um retângulo da largura da
 * tela, entre a altura do começo e a do fim do segmento.
 * Recebe: o segmento e os pontos de tela do começo (perto) e do fim (longe).
 * Devolve: nada.
 */
static void desenharGrama(Segmento *segmento, PontoNaTela perto, PontoNaTela longe) {
    Color corGrama = ehSegmentoClaro(segmento) ? COR_GRAMA_CLARA : COR_GRAMA_ESCURA;
    DrawRectangle(0, (int) longe.y, GetScreenWidth(), (int) (perto.y - longe.y) + 1, corGrama);
}

/*
 * desenharEstrada
 * Desenha zebras, asfalto e faixas de um segmento.
 * Recebe: o segmento e os pontos de tela do começo (perto) e do fim (longe).
 * Devolve: nada.
 */
static void desenharEstrada(Segmento *segmento, PontoNaTela perto, PontoNaTela longe) {
    int claro = ehSegmentoClaro(segmento);

    /* Zebra: um trapézio um pouco mais largo, que sobra dos lados do asfalto */
    Color corZebra = claro ? COR_ZEBRA_CLARA : COR_ZEBRA_ESCURA;
    desenharTrapezio(perto.x, perto.y, perto.largura * PROPORCAO_ZEBRA,
                     longe.x, longe.y, longe.largura * PROPORCAO_ZEBRA, corZebra);

    /* Asfalto (branco na linha de largada/chegada) */
    Color corAsfalto = claro ? COR_ASFALTO_CLARO : COR_ASFALTO_ESCURO;
    if (segmento->ehLinhaDeChegada) {
        corAsfalto = COR_LARGADA;
    }
    desenharTrapezio(perto.x, perto.y, perto.largura,
                     longe.x, longe.y, longe.largura, corAsfalto);

    /* Faixas brancas tracejadas: só nos segmentos claros */
    if (claro && !segmento->ehLinhaDeChegada) {
        float desvioPerto = perto.largura * POSICAO_FAIXA;
        float desvioLonge = longe.largura * POSICAO_FAIXA;
        float metadeFaixaPerto = perto.largura * PROPORCAO_FAIXA;
        float metadeFaixaLonge = longe.largura * PROPORCAO_FAIXA;

        desenharTrapezio(perto.x - desvioPerto, perto.y, metadeFaixaPerto,
                         longe.x - desvioLonge, longe.y, metadeFaixaLonge, COR_FAIXA);
        desenharTrapezio(perto.x + desvioPerto, perto.y, metadeFaixaPerto,
                         longe.x + desvioLonge, longe.y, metadeFaixaLonge, COR_FAIXA);
    }
}

/*
 * desenharFundo
 * Pinta o céu em degradê até o horizonte (meio da tela) e o chão
 * inteiro de grama abaixo dele. O chão cobre a faixa entre o último
 * segmento visível e o horizonte, que nenhum segmento alcança.
 * Recebe: nada.
 * Devolve: nada.
 */
static void desenharFundo(void) {
    int largura = GetScreenWidth();
    int horizonte = GetScreenHeight() / 2;
    DrawRectangleGradientV(0, 0, largura, horizonte, COR_CEU_ALTO, COR_CEU_HORIZONTE);
    DrawRectangle(0, horizonte, largura, GetScreenHeight() - horizonte, COR_GRAMA_ESCURA);
}

/*
 * desenharPista
 * Desenha a estrada já projetada, do mais longe para o mais perto, para
 * que o que está perto cubra o que está longe, como um pintor que pinta
 * o fundo primeiro.
 * Recebe: a visão calculada neste quadro.
 * Devolve: nada.
 */
static void desenharPista(Visao *visao) {
    desenharFundo();

    /* Toda a grama primeiro: se ela fosse junto com a estrada, a grama de
     * um segmento pintaria uma linha por cima do asfalto do anterior. */
    for (int i = DISTANCIA_VISIVEL - 1; i >= 0; i--) {
        desenharGrama(visao->segmentos[i], visao->perto[i], visao->longe[i]);
    }
    for (int i = DISTANCIA_VISIVEL - 1; i >= 0; i--) {
        desenharEstrada(visao->segmentos[i], visao->perto[i], visao->longe[i]);
    }
}

/*
 * desenharCarro
 * Desenha um carro visto de trás com formas simples da raylib.
 * O mesmo desenho serve para o jogador (escala 1) e para os rivais
 * (menores quanto mais longe estão).
 *   - virando, a cabine desliza um pouco para o lado da curva;
 *   - freando, as lanternas acendem mais forte.
 * Recebe: o carro, o centro da base na tela (x, y) e a escala.
 * Devolve: nada.
 */
static void desenharCarro(Carro *carro, float centroX, float baseY, float escala) {
    Comandos comandos = carro->ultimosComandos;
    Color corCarroceria = CORES_CARROCERIA[carro->numero];
    Color corCabine = CORES_CABINE[carro->numero];

    float inclinacao = 0.0f;
    if (comandos.esquerda) {
        inclinacao = -INCLINACAO_CABINE * escala;
    }
    if (comandos.direita) {
        inclinacao = INCLINACAO_CABINE * escala;
    }

    float largura = LARGURA_CARRO * escala;
    float metade = largura / 2.0f;
    float alturaCarroceria = ALTURA_CARROCERIA * escala;
    float topoCarroceria = baseY - alturaCarroceria;

    /* Sombra */
    DrawEllipse((int) centroX, (int) baseY, metade + 10 * escala, 10 * escala, Fade(BLACK, 0.35f));

    /* Pneus traseiros */
    float larguraPneu = 34 * escala;
    Rectangle pneuEsquerdo = { centroX - metade, baseY - 30 * escala, larguraPneu, 32 * escala };
    Rectangle pneuDireito = { centroX + metade - larguraPneu, baseY - 30 * escala, larguraPneu, 32 * escala };
    DrawRectangleRounded(pneuEsquerdo, 0.3f, 4, COR_PNEU);
    DrawRectangleRounded(pneuDireito, 0.3f, 4, COR_PNEU);

    /* Cabine e vidro traseiro */
    Rectangle cabine = { centroX - metade * 0.62f + inclinacao, topoCarroceria - 34 * escala,
                         largura * 0.62f, 44 * escala };
    Rectangle vidro = { cabine.x + 10 * escala, cabine.y + 7 * escala,
                        cabine.width - 20 * escala, 22 * escala };
    DrawRectangleRounded(cabine, 0.5f, 6, corCabine);
    DrawRectangleRounded(vidro, 0.4f, 6, COR_VIDRO);

    /* Carroceria */
    Rectangle carroceria = { centroX - metade + 6 * escala, topoCarroceria,
                             largura - 12 * escala, alturaCarroceria - 8 * escala };
    DrawRectangleRounded(carroceria, 0.4f, 6, corCarroceria);

    /* Lanternas */
    Color corLanterna = comandos.frear ? COR_LANTERNA_FREIO : COR_LANTERNA;
    float larguraLanterna = 34 * escala;
    float alturaLanterna = 12 * escala;
    DrawRectangleRounded((Rectangle){ carroceria.x + 10 * escala, topoCarroceria + 12 * escala,
                                      larguraLanterna, alturaLanterna }, 0.5f, 4, corLanterna);
    DrawRectangleRounded((Rectangle){ carroceria.x + carroceria.width - 10 * escala - larguraLanterna,
                                      topoCarroceria + 12 * escala, larguraLanterna, alturaLanterna },
                         0.5f, 4, corLanterna);

    /* Para-choque */
    DrawRectangleRounded((Rectangle){ carroceria.x + 4 * escala, baseY - 18 * escala,
                                      carroceria.width - 8 * escala, 10 * escala }, 0.5f, 4, COR_PARACHOQUE);
}

/*
 * desenharJogador
 * O carro do jogador fica sempre no mesmo lugar da tela (embaixo, no
 * centro): quem se mexe é a estrada. Na grama ele trepida.
 * Recebe: o carro do jogador.
 * Devolve: nada.
 */
static void desenharJogador(Carro *jogador) {
    float centroX = GetScreenWidth() / 2.0f;
    float baseY = GetScreenHeight() - MARGEM_CARRO;

    if (estaForaDaPista(jogador) && jogador->velocidade > 0) {
        baseY -= GetRandomValue(0, TREPIDACAO_MAXIMA);
    }
    desenharCarro(jogador, centroX, baseY, 1.0f);
}

/*
 * desenharRival
 * Projeta um rival que está no i-ésimo segmento visível e o desenha
 * no tamanho certo para a distância.
 * Recebe: a visão, o índice do segmento na visão e o rival.
 * Devolve: nada.
 */
static void desenharRival(Visao *visao, int i, Carro *rival) {
    float z = i * COMPRIMENTO_SEGMENTO - visao->camera.distanciaNoSegmento
              + rival->posicao.distanciaNoSegmento;
    if (z < DISTANCIA_MINIMA_RIVAL) {
        return;   /* atrás da câmera ou colado nela */
    }

    /* Centro da estrada naquele ponto exato do segmento (com a curva) */
    float fracao = rival->posicao.distanciaNoSegmento / COMPRIMENTO_SEGMENTO;
    float xCurva = visao->xCurva[i] + visao->dxCurva[i] * fracao;
    PontoNaTela chao = projetar(xCurva, z, visao->xCamera);

    /* O rival tem o tamanho do jogador quando está na mesma distância que ele */
    PontoNaTela chaoDoJogador = projetar(0.0f, DISTANCIA_CAMERA, 0.0f);
    float escala = chao.largura / chaoDoJogador.largura;

    float centroX = chao.x + rival->deslocamentoLateral * chao.largura;
    float baseY = chao.y - MARGEM_CARRO * escala;
    desenharCarro(rival, centroX, baseY, escala);
}

/*
 * desenharCarros
 * Desenha rivais e jogador do mais longe para o mais perto. Para isso
 * percorre os segmentos visíveis de trás para a frente e desenha os
 * rivais que estão em cada um. O jogador entra na vez dele: depois dos
 * rivais que estão à frente dele e antes dos que estão ao lado ou atrás.
 * Recebe: a visão e a corrida.
 * Devolve: nada.
 */
static void desenharCarros(Visao *visao, Corrida *corrida) {
    int jogadorDesenhado = 0;

    for (int i = DISTANCIA_VISIVEL - 1; i >= 0; i--) {
        for (int r = 0; r < TOTAL_RIVAIS; r++) {
            Carro *rival = &corrida->rivais[r];
            if (rival->posicao.segmento != visao->segmentos[i]) {
                continue;
            }

            float z = i * COMPRIMENTO_SEGMENTO - visao->camera.distanciaNoSegmento
                      + rival->posicao.distanciaNoSegmento;
            if (!jogadorDesenhado && z < DISTANCIA_CAMERA) {
                desenharJogador(&corrida->jogador);
                jogadorDesenhado = 1;
            }
            desenharRival(visao, i, rival);
        }
    }

    if (!jogadorDesenhado) {
        desenharJogador(&corrida->jogador);
    }
}

/*
 * desenharVisaoDoJogador
 * Desenha o que o jogador vê: a câmera fica um pouco atrás e acima do
 * carro, olhando para a frente. A posição da câmera é a do carro
 * "andada para trás" DISTANCIA_CAMERA unidades na lista da pista.
 * Recebe: a corrida.
 * Devolve: nada.
 */
void desenharVisaoDoJogador(Corrida *corrida) {
    PosicaoNaPista camera = corrida->jogador.posicao;
    moverNaPista(&camera, -DISTANCIA_CAMERA);   /* as voltas devolvidas não importam aqui */

    /* static: a struct é grande (~6 KB), então fica fora da pilha */
    static Visao visao;
    calcularVisao(&visao, camera, corrida->jogador.deslocamentoLateral);

    desenharPista(&visao);
    desenharCarros(&visao, corrida);
}
