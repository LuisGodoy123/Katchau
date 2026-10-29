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
 *
 * Como funcionam as subidas e descidas:
 *
 *   Cada segmento tem uma altura. Na projeção, em vez da altura fixa
 *   da câmera, usamos a diferença entre a altura da câmera e a do
 *   ponto: um ponto mais alto que o chão sob a câmera sobe na tela,
 *   um mais baixo desce. Atrás de uma lombada, a estrada fica escondida
 *   pela parte mais perto, que é desenhada por cima.
 *
 * Texturas:
 *
 *   As imagens de recursos/texturas são carregadas uma vez, no começo
 *   (carregarTexturas). Se alguma não for encontrada, aquela parte do
 *   desenho volta a usar as cores sólidas, então o jogo nunca quebra
 *   por falta de imagem.
 * ------------------------------------------------------------------
 */

#include <stddef.h>
#include "raylib.h"
#include "rlgl.h"
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

/* ---------- Texturas ---------- */
#define PASTA_TEXTURAS       "recursos/texturas/"
#define LARGURA_FUNDO        640.0f    /* largura das imagens de fundo, em pixels */
#define REPETICOES_ASFALTO   4.0f      /* quantas vezes a textura do asfalto se repete na largura */
#define TAMANHO_TEXTURA_CHAO 64.0f     /* lado das texturas de asfalto e grama, em pixels */
#define VELOCIDADE_CIDADE    0.5f      /* a cidade, mais longe, desliza na metade da velocidade dos morros */
#define VELOCIDADE_CEU       0.2f
#define VELOCIDADE_CAMERA_MENU 3000.0f /* a câmera do fundo do menu anda a 75 km/h */
#define LIMITE_VERMELHO      60        /* quanto o vermelho tem que passar o verde e o azul para ser "pintura" */
#define BRILHO_VERMELHO      200.0f    /* vermelho do sprite que vira exatamente a cor do rival */

/* As três poses do sprite do carro */
#define POSE_RETO            0
#define POSE_ESQUERDA        1
#define POSE_DIREITA         2
#define TOTAL_POSES          3

/* ---------- Cores ---------- */
#define COR_CEU_ALTO         (Color){  40, 110, 210, 255 }
#define COR_CEU_HORIZONTE    (Color){ 160, 210, 255, 255 }
#define COR_GRAMA_CLARA      (Color){  16, 170,  16, 255 }
#define COR_GRAMA_ESCURA     (Color){   0, 154,   0, 255 }
#define COR_CHAO_DISTANTE    (Color){  54, 125,  48, 255 }   /* média da textura de grama */
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

/* Texturas carregadas por carregarTexturas(). id == 0 quer dizer "não carregou". */
static Texture2D texturaCeu;
static Texture2D texturaCidade;
static Texture2D texturaMorros;
static Texture2D texturaAsfalto[2];   /* [0] escuro, [1] claro */
static Texture2D texturaGrama[2];     /* [0] escura, [1] clara */
static Texture2D texturaCarro[TOTAL_CARROS][TOTAL_POSES];

/* Quanto o fundo já deslizou para o lado (em pixels da imagem) por causa das curvas */
static float deslocamentoFundo = 0.0f;

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
    float alturaCamera;                       /* altura da câmera no mundo (chão + ALTURA_CAMERA) */
    Segmento *segmentos[DISTANCIA_VISIVEL];   /* os próximos nós da lista */
    float xCurva[DISTANCIA_VISIVEL];          /* empurrão da curva no começo de cada segmento */
    float dxCurva[DISTANCIA_VISIVEL];         /* quanto o empurrão cresce dentro do segmento */
    PontoNaTela perto[DISTANCIA_VISIVEL];     /* começo do segmento na tela */
    PontoNaTela longe[DISTANCIA_VISIVEL];     /* fim do segmento na tela */
    float limiteTela[DISTANCIA_VISIVEL];      /* abaixo desta linha da tela, a estrada mais perto cobre o segmento */
} Visao;

/*
 * carregarTexturaRepetida
 * Carrega uma imagem que pode se repetir lado a lado (asfalto, grama, fundo).
 * Recebe: o nome do arquivo dentro de PASTA_TEXTURAS.
 * Devolve: a textura (id 0 se o arquivo não existir).
 */
static Texture2D carregarTexturaRepetida(const char *arquivo) {
    Texture2D textura = LoadTexture(TextFormat("%s%s", PASTA_TEXTURAS, arquivo));
    if (textura.id != 0) {
        SetTextureWrap(textura, TEXTURE_WRAP_REPEAT);
    }
    return textura;
}

/*
 * carregarCarroPintado
 * Carrega um sprite do carro trocando o vermelho da pintura pela cor
 * pedida. Cada pixel bem vermelho vira a cor nova, mantendo o quanto ele
 * era claro ou escuro (assim as sombras do desenho continuam lá).
 * Pneus, asa e detalhes amarelos não são vermelhos e ficam iguais.
 * Recebe: o nome do arquivo e a cor da carroceria.
 * Devolve: a textura (id 0 se o arquivo não existir).
 */
static Texture2D carregarCarroPintado(const char *arquivo, Color cor) {
    Image imagem = LoadImage(TextFormat("%s%s", PASTA_TEXTURAS, arquivo));
    if (imagem.data == NULL) {
        return (Texture2D){ 0 };
    }
    ImageFormat(&imagem, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

    Color *pixels = (Color *) imagem.data;
    for (int i = 0; i < imagem.width * imagem.height; i++) {
        Color p = pixels[i];
        if (p.r > p.g + LIMITE_VERMELHO && p.r > p.b + LIMITE_VERMELHO) {
            float brilho = p.r / BRILHO_VERMELHO;
            float r = cor.r * brilho, g = cor.g * brilho, b = cor.b * brilho;
            pixels[i].r = (unsigned char) (r > 255 ? 255 : r);
            pixels[i].g = (unsigned char) (g > 255 ? 255 : g);
            pixels[i].b = (unsigned char) (b > 255 ? 255 : b);
        }
    }

    Texture2D textura = LoadTextureFromImage(imagem);
    UnloadImage(imagem);
    return textura;
}

/*
 * carregarTexturas
 * Lê todas as imagens do disco para a placa de vídeo. Tem que ser
 * chamada depois de InitWindow (a raylib precisa da janela aberta).
 * Recebe: nada.
 * Devolve: nada.
 */
void carregarTexturas(void) {
    texturaCeu = carregarTexturaRepetida("fundo/ceu.png");
    texturaCidade = carregarTexturaRepetida("fundo/cidade.png");
    texturaMorros = carregarTexturaRepetida("fundo/morros.png");
    texturaAsfalto[0] = carregarTexturaRepetida("estrada/asfalto_escuro.png");
    texturaAsfalto[1] = carregarTexturaRepetida("estrada/asfalto_claro.png");
    texturaGrama[0] = carregarTexturaRepetida("estrada/grama_escura.png");
    texturaGrama[1] = carregarTexturaRepetida("estrada/grama_clara.png");

    const char *poses[TOTAL_POSES] = {
        "carro/carro_reto.png", "carro/carro_esquerda.png", "carro/carro_direita.png"
    };
    for (int pose = 0; pose < TOTAL_POSES; pose++) {
        /* O sprite já é vermelho: o jogador usa o original */
        texturaCarro[0][pose] = LoadTexture(TextFormat("%s%s", PASTA_TEXTURAS, poses[pose]));
        for (int numero = 1; numero < TOTAL_CARROS; numero++) {
            texturaCarro[numero][pose] = carregarCarroPintado(poses[pose], CORES_CARROCERIA[numero]);
        }
    }
}

/*
 * descarregarTexturas
 * Devolve a memória de vídeo das texturas. Chamar antes de CloseWindow.
 * (UnloadTexture ignora texturas com id 0, as que não carregaram.)
 * Recebe: nada.
 * Devolve: nada.
 */
void descarregarTexturas(void) {
    UnloadTexture(texturaCeu);
    UnloadTexture(texturaCidade);
    UnloadTexture(texturaMorros);
    for (int i = 0; i < 2; i++) {
        UnloadTexture(texturaAsfalto[i]);
        UnloadTexture(texturaGrama[i]);
    }
    for (int numero = 0; numero < TOTAL_CARROS; numero++) {
        for (int pose = 0; pose < TOTAL_POSES; pose++) {
            UnloadTexture(texturaCarro[numero][pose]);
        }
    }
}

/*
 * projetar
 * Converte um ponto do chão (coordenadas de mundo) em coordenadas de tela.
 * Recebe: x do centro da estrada, altura do chão nesse ponto, distância z
 *         à frente da câmera, x da câmera e altura da câmera.
 * Devolve: a posição e a largura da estrada naquele ponto da tela.
 */
static PontoNaTela projetar(float xMundo, float alturaMundo, float z, float xCamera, float alturaCamera) {
    float metadeLargura = GetScreenWidth() / 2.0f;
    float metadeAltura = GetScreenHeight() / 2.0f;
    float escala = PROFUNDIDADE_CAMERA / z;

    PontoNaTela ponto;
    ponto.x = metadeLargura + escala * (xMundo - xCamera) * metadeLargura;
    ponto.y = metadeAltura + escala * (alturaCamera - alturaMundo) * metadeAltura;
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
    visao->alturaCamera = alturaNaPosicao(camera) + ALTURA_CAMERA;
    float limite = GetScreenHeight();

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
        Segmento *seguinte = proximoSegmento(atual);
        visao->perto[i] = projetar(xCurva, atual->altura, zPerto, visao->xCamera, visao->alturaCamera);
        visao->longe[i] = projetar(xCurva + dxCurva, seguinte->altura, zLonge,
                                   visao->xCamera, visao->alturaCamera);

        /* O ponto mais alto da tela já ocupado pela estrada mais perto:
         * o que estiver abaixo dele, neste segmento, fica escondido. */
        visao->limiteTela[i] = limite;
        if (visao->longe[i].y < limite) {
            limite = visao->longe[i].y;
        }

        xCurva += dxCurva;
        dxCurva += atual->curvatura * FATOR_CURVA;

        atual = seguinte;   /* lista circular: depois do último vem a largada */
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
 * desenharTrapezioComTextura
 * Igual ao desenharTrapezio, mas preenchido com uma textura. A imagem
 * ocupa o segmento inteiro no comprimento e se repete "repeticoes"
 * vezes na largura. Usa a rlgl (a camada de baixo da raylib), porque
 * as funções comuns de textura só desenham retângulos.
 * Recebe: a textura, centro e meia-largura embaixo e em cima, e as repetições.
 * Devolve: nada.
 */
static void desenharTrapezioComTextura(Texture2D textura,
                                       float xPerto, float yPerto, float metadePerto,
                                       float xLonge, float yLonge, float metadeLonge,
                                       float repeticoes) {
    rlSetTexture(textura.id);
    rlBegin(RL_QUADS);
    rlColor4ub(255, 255, 255, 255);
    /* Mesma ordem anti-horária do desenharTrapezio */
    rlTexCoord2f(0.0f, 0.0f);       rlVertex2f(xLonge - metadeLonge, yLonge);
    rlTexCoord2f(0.0f, 1.0f);       rlVertex2f(xPerto - metadePerto, yPerto);
    rlTexCoord2f(repeticoes, 1.0f); rlVertex2f(xPerto + metadePerto, yPerto);
    rlTexCoord2f(repeticoes, 0.0f); rlVertex2f(xLonge + metadeLonge, yLonge);
    rlEnd();
    rlSetTexture(0);
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
    if (longe.y >= perto.y) {
        return;   /* descida vista de cima de uma lombada: o segmento está de costas e escondido */
    }
    int claro = ehSegmentoClaro(segmento);
    int largura = GetScreenWidth();
    float altura = perto.y - longe.y + 1;

    Texture2D textura = texturaGrama[claro];
    if (textura.id == 0) {
        Color corGrama = claro ? COR_GRAMA_CLARA : COR_GRAMA_ESCURA;
        DrawRectangle(0, (int) longe.y, largura, (int) altura, corGrama);
        return;
    }

    /* Cada "ladrilho" de grama tem metade da largura da estrada naquele
     * ponto, então ele encolhe com a distância. A conta do x prende os
     * ladrilhos no centro da estrada, para a grama andar junto nas curvas. */
    float ladrilho = perto.largura / 2.0f;
    if (ladrilho < 1.0f) {
        ladrilho = 1.0f;
    }
    Rectangle origem = { -perto.x / ladrilho * TAMANHO_TEXTURA_CHAO, 0.0f,
                         largura / ladrilho * TAMANHO_TEXTURA_CHAO, TAMANHO_TEXTURA_CHAO };
    Rectangle destino = { 0.0f, longe.y, (float) largura, altura };
    DrawTexturePro(textura, origem, destino, (Vector2){ 0, 0 }, 0.0f, WHITE);
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
    Texture2D textura = texturaAsfalto[claro];
    if (segmento->ehLinhaDeChegada || textura.id == 0) {
        Color corAsfalto = claro ? COR_ASFALTO_CLARO : COR_ASFALTO_ESCURO;
        if (segmento->ehLinhaDeChegada) {
            corAsfalto = COR_LARGADA;
        }
        desenharTrapezio(perto.x, perto.y, perto.largura,
                         longe.x, longe.y, longe.largura, corAsfalto);
    } else {
        desenharTrapezioComTextura(textura, perto.x, perto.y, perto.largura,
                                   longe.x, longe.y, longe.largura, REPETICOES_ASFALTO);
    }

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
 * desenharCamadaDoFundo
 * Desenha uma imagem de fundo esticada na largura da tela, com a base
 * encostada em "baseY". A imagem se repete para os lados, então pode
 * deslizar sem acabar.
 * Recebe: a textura, a base na tela, quanto já deslizou e a escala.
 * Devolve: nada.
 */
static void desenharCamadaDoFundo(Texture2D textura, float baseY, float deslizou, float escala) {
    float altura = textura.height * escala;
    Rectangle origem = { deslizou, 0.0f, LARGURA_FUNDO, (float) textura.height };
    Rectangle destino = { 0.0f, baseY - altura, GetScreenWidth(), altura };
    DrawTexturePro(textura, origem, destino, (Vector2){ 0, 0 }, 0.0f, WHITE);
}

/*
 * desenharFundo
 * Pinta o céu até o horizonte (meio da tela) e o chão inteiro abaixo
 * dele. O chão cobre a faixa entre o último segmento visível e o
 * horizonte, que nenhum segmento alcança.
 * Com as texturas, o céu, a cidade e os morros são três camadas que
 * deslizam nas curvas, cada uma numa velocidade (efeito parallax:
 * o que está mais longe parece se mexer menos).
 * Recebe: nada.
 * Devolve: nada.
 */
static void desenharFundo(void) {
    int largura = GetScreenWidth();
    int horizonte = GetScreenHeight() / 2;
    float escala = largura / LARGURA_FUNDO;

    if (texturaCeu.id != 0) {
        /* O céu sempre preenche do topo até o horizonte */
        Rectangle origem = { deslocamentoFundo * VELOCIDADE_CEU, 0.0f,
                             LARGURA_FUNDO, (float) texturaCeu.height };
        Rectangle destino = { 0.0f, 0.0f, (float) largura, (float) horizonte };
        DrawTexturePro(texturaCeu, origem, destino, (Vector2){ 0, 0 }, 0.0f, WHITE);
    } else {
        DrawRectangleGradientV(0, 0, largura, horizonte, COR_CEU_ALTO, COR_CEU_HORIZONTE);
    }
    if (texturaCidade.id != 0) {
        desenharCamadaDoFundo(texturaCidade, horizonte, deslocamentoFundo * VELOCIDADE_CIDADE, escala);
    }
    if (texturaMorros.id != 0) {
        desenharCamadaDoFundo(texturaMorros, horizonte, deslocamentoFundo, escala);
    }

    Color corChao = texturaGrama[0].id != 0 ? COR_CHAO_DISTANTE : COR_GRAMA_ESCURA;
    DrawRectangle(0, horizonte, largura, GetScreenHeight() - horizonte, corChao);
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
static void desenharCarroComFormas(Carro *carro, float centroX, float baseY, float escala) {
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
 * desenharCarro
 * Desenha um carro visto de trás usando o sprite da pose certa
 * (reto, virando à esquerda ou à direita). Se o sprite não carregou,
 * usa o desenho com formas simples.
 * Recebe: o carro, o centro da base na tela (x, y) e a escala.
 * Devolve: nada.
 */
static void desenharCarro(Carro *carro, float centroX, float baseY, float escala) {
    Comandos comandos = carro->ultimosComandos;
    int pose = POSE_RETO;
    if (comandos.esquerda && !comandos.direita) {
        pose = POSE_ESQUERDA;
    }
    if (comandos.direita && !comandos.esquerda) {
        pose = POSE_DIREITA;
    }

    Texture2D sprite = texturaCarro[carro->numero][pose];
    if (sprite.id == 0) {
        desenharCarroComFormas(carro, centroX, baseY, escala);
        return;
    }

    /* O sprite fica com a mesma largura do carro desenhado com formas,
     * mantendo a proporção da imagem (a sombra já vem no desenho) */
    float largura = LARGURA_CARRO * escala;
    float altura = largura * sprite.height / sprite.width;
    Rectangle origem = { 0.0f, 0.0f, (float) sprite.width, (float) sprite.height };
    Rectangle destino = { centroX - largura / 2.0f, baseY - altura, largura, altura };
    DrawTexturePro(sprite, origem, destino, (Vector2){ 0, 0 }, 0.0f, WHITE);
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
    PontoNaTela chao = projetar(xCurva, alturaNaPosicao(rival->posicao), z,
                                visao->xCamera, visao->alturaCamera);

    /* O rival tem o tamanho do jogador quando está na mesma distância que ele
     * (a largura na tela só depende da distância, não da altura) */
    PontoNaTela chaoDoJogador = projetar(0.0f, 0.0f, DISTANCIA_CAMERA, 0.0f, 0.0f);
    float escala = chao.largura / chaoDoJogador.largura;

    float centroX = chao.x + rival->deslocamentoLateral * chao.largura;
    float baseY = chao.y - MARGEM_CARRO * escala;

    /* Atrás de uma lombada, só aparece a parte do carro acima da estrada da frente */
    BeginScissorMode(0, 0, GetScreenWidth(), (int) visao->limiteTela[i]);
    desenharCarro(rival, centroX, baseY, escala);
    EndScissorMode();
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

    /* Nas curvas o fundo desliza: uma volta de 360 graus anda uma
     * largura inteira da imagem dos morros. Curva à direita (curvatura
     * positiva) faz o fundo andar para a esquerda. */
    float grausPorSegundo = camera.segmento->curvatura * corrida->jogador.velocidade
                            / COMPRIMENTO_SEGMENTO;
    deslocamentoFundo += grausPorSegundo * GetFrameTime() * LARGURA_FUNDO / 360.0f;

    /* static: a struct é grande (~6 KB), então fica fora da pilha */
    static Visao visao;
    calcularVisao(&visao, camera, corrida->jogador.deslocamentoLateral);

    desenharPista(&visao);
    desenharCarros(&visao, corrida);
}

/*
 * desenharPistaDeFundo
 * Fundo das telas de menu: a câmera passeia sozinha pela pista, devagar,
 * pelo centro da estrada. Ela anda pela lista com moverNaPista(), então
 * dá voltas infinitas no circuito.
 * Recebe: a pista.
 * Devolve: nada.
 */
void desenharPistaDeFundo(Pista *pista) {
    static PosicaoNaPista camera = { NULL, 0.0f };
    if (camera.segmento == NULL) {
        camera.segmento = pista->largada;
    }
    moverNaPista(&camera, VELOCIDADE_CAMERA_MENU * GetFrameTime());

    /* O fundo desliza nas curvas como na corrida */
    float grausPorSegundo = camera.segmento->curvatura * VELOCIDADE_CAMERA_MENU / COMPRIMENTO_SEGMENTO;
    deslocamentoFundo += grausPorSegundo * GetFrameTime() * LARGURA_FUNDO / 360.0f;

    static Visao visao;
    calcularVisao(&visao, camera, 0.0f);
    desenharPista(&visao);
}

/*
 * desenharCarroParado
 * Desenha um modelo de carro fora da corrida (por exemplo, na garagem),
 * visto de trás e andando reto.
 * Recebe: o número do carro (define a cor), o centro da base na tela e a escala.
 * Devolve: nada.
 */
void desenharCarroParado(int numero, float centroX, float baseY, float escala) {
    Carro carro = { 0 };   /* sem comandos: pose reta, lanternas apagadas */
    carro.numero = numero;
    desenharCarro(&carro, centroX, baseY, escala);
}
