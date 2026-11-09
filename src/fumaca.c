/*
 * fumaca.c
 * ------------------------------------------------------------------
 * As nuvens de fumaça e de poeira. Veja fumaca.h.
 *
 * As nuvens ficam num vetor de tamanho fixo. Quando ele enche, a nuvem nova
 * ocupa o lugar da mais antiga. A imagem de cada nuvem é gerada ao abrir
 * o jogo: um borrão redondo, de bordas suaves, recortado com ruído para
 * não parecer uma bola perfeita.
 *
 * As três variações ficam numa textura só, lado a lado (o atlas): a placa de vídeo
 * desenha tudo o que usa a mesma textura numa chamada, e trocar de textura no meio
 * do laço custa uma chamada de desenho por troca. Como a variação de cada nuvem é
 * sorteada, desenhar na ordem do vetor com três texturas separadas custava quase mil
 * chamadas por quadro; com o atlas, custa uma.
 * ------------------------------------------------------------------
 */

#include <math.h>
#include "fumaca.h"
#include "rlgl.h"
#include "raymath.h"

#define MAXIMO_DE_NUVENS     1400
#define LADO_DA_IMAGEM       96
#define VARIACOES_DE_IMAGEM  3
#define LADO_DO_ATLAS        (LADO_DA_IMAGEM * 2)   /* as variações num quadrado 2x2 (a quarta célula fica vazia) */

#define VIDA_DO_PNEU         1.5f      /* segundos */
#define VIDA_DA_POEIRA       1.1f
#define VIDA_DO_DRIFT 1.8f
#define TAMANHO_INICIAL_DRIFT 0.8f
#define TAMANHO_FINAL_DRIFT 5.5f
#define OPACIDADE_DRIFT 0.40f
#define ARRASTO_DO_DRIFT 0.45f     /* acompanha o carro por mais tempo: a nuvem fica à vista, ao lado dele */
#define SUBIDA_DO_DRIFT 1.4f
#define FREIO_DO_AR_NO_DRIFT 1.8f
#define TAMANHO_INICIAL      0.45f     /* metros */
#define TAMANHO_FINAL_PNEU   4.2f
#define TAMANHO_FINAL_POEIRA 3.2f
#define OPACIDADE_PNEU       0.46f
#define OPACIDADE_POEIRA     0.40f
#define SUBIDA               0.9f      /* metros por segundo */
#define ARRASTO_DO_CARRO     0.20f     /* a nuvem nasce com esta fração da velocidade do carro... */
#define FREIO_DO_AR          2.2f      /* ...e o ar a segura (1/s) */
#define ESPALHAR             0.9f      /* metros por segundo, para qualquer lado */
#define COR_DO_PNEU          (Color){ 232, 232, 236, 255 }
#define COMPRIMENTO_DA_CHAMA  1.5f      /* metros */
#define LARGURA_DA_CHAMA      0.70f
#define PEDACOS_DA_CHAMA      9
#define COR_DA_CHAMA_PERTO    (Color){ 130, 205, 255, 255 }
#define COR_DA_CHAMA_LONGE    (Color){  20,  70, 255, 255 }
#define COR_DA_POEIRA        (Color){ 150, 122,  78, 255 }

typedef struct {
    Vector3 posicao;
    Vector3 velocidade;
    float idade;         /* segundos desde que nasceu */
    float vida;          /* segundos até sumir (0 = lugar livre) */
    float forca;         /* de 0 a 1: quanto ela é densa */
    float giro;          /* graus, para as nuvens não ficarem iguais */
    float giroPorSegundo;
    int imagem;
    TipoDeNuvem tipo;
} Nuvem;

static Nuvem nuvens[MAXIMO_DE_NUVENS];
static int proximaNuvem = 0;
static Texture2D atlas;

/* Onde cada variação está no atlas, já em coordenadas de textura (de 0 a 1) */
typedef struct {
    float u0, v0, u1, v1;
} CelulaDoAtlas;
static CelulaDoAtlas celulas[VARIACOES_DE_IMAGEM];

/* A base dos borrões deste quadro (veja prepararBase). O eixo horizontal é a direita da
 * câmera; o vertical é sempre o do mundo. Nas curvas inclinadas e nas subidas a câmera roda
 * junto com o chão (montarCamera, em render.c), e aí os dois eixos não são perpendiculares:
 * "inclinacao" é o quanto eles se aproximam, e entra na conta do giro. */
static Vector3 direitaDaCamera = { 1.0f, 0.0f, 0.0f };
static float inclinacaoDaBase = 0.0f;    /* direitaDaCamera . (0,1,0) */
static float aberturaDaBase = 1.0f;      /* sqrt(1 - inclinacao^2) */

/*
 * sortear
 * Recebe: os limites.
 * Devolve: um número entre eles.
 */
static float sortear(float minimo, float maximo) {
    return minimo + (maximo - minimo) * GetRandomValue(0, 1000) / 1000.0f;
}

/*
 * desenharBorrao
 * Um borrão branco numa célula do atlas: a opacidade cai do centro para a borda, e o
 * ruído desfaz o contorno. A borda da célula fica sempre transparente (o borrão é
 * redondo, inscrito no quadrado), então as células não vazam uma na outra ao reduzir.
 * Recebe: a folha do atlas, o canto da célula e a semente do ruído (muda o desenho).
 * Devolve: nada.
 */
static void desenharBorrao(Image *folha, int cantoX, int cantoY, int semente) {
    Image ruido = GenImagePerlinNoise(LADO_DA_IMAGEM, LADO_DA_IMAGEM, semente * 97, semente * 53, 3.0f);
    Color *cores = LoadImageColors(ruido);
    float meio = (LADO_DA_IMAGEM - 1) / 2.0f;
    for (int y = 0; y < LADO_DA_IMAGEM; y++) {
        for (int x = 0; x < LADO_DA_IMAGEM; x++) {
            float dx = (x - meio) / meio, dy = (y - meio) / meio;
            float distancia = sqrtf(dx * dx + dy * dy);
            float redondo = fmaxf(1.0f - distancia, 0.0f);
            redondo = redondo * redondo * (3.0f - 2.0f * redondo);   /* suaviza a queda */
            float textura = 0.45f + 0.75f * cores[y * LADO_DA_IMAGEM + x].r / 255.0f;
            float alfa = fminf(redondo * textura * 1.25f, 1.0f);
            ImageDrawPixel(folha, cantoX + x, cantoY + y, (Color){ 255, 255, 255, (unsigned char) (alfa * 255.0f) });
        }
    }
    UnloadImageColors(cores);
    UnloadImage(ruido);
}

/*
 * carregarFumaca
 * Gera o atlas com as variações das nuvens. Chamar depois de criar a janela.
 * Recebe: nada.
 * Devolve: nada.
 */
void carregarFumaca(void) {
    /* Branco transparente, e não BLANK: todo pixel do atlas fica com a cor 255, e só o alfa
     * varia. Assim as versões reduzidas (os mipmaps) só misturam alfa, como aconteceria com
     * uma imagem sozinha — nenhuma célula escurece a vizinha nem o canto que ficou vazio. */
    Image folha = GenImageColor(LADO_DO_ATLAS, LADO_DO_ATLAS, (Color){ 255, 255, 255, 0 });
    for (int i = 0; i < VARIACOES_DE_IMAGEM; i++) {
        int cantoX = (i % 2) * LADO_DA_IMAGEM, cantoY = (i / 2) * LADO_DA_IMAGEM;
        desenharBorrao(&folha, cantoX, cantoY, i + 1);
        celulas[i].u0 = (float) cantoX / LADO_DO_ATLAS;
        celulas[i].v0 = (float) cantoY / LADO_DO_ATLAS;
        celulas[i].u1 = (float) (cantoX + LADO_DA_IMAGEM) / LADO_DO_ATLAS;
        celulas[i].v1 = (float) (cantoY + LADO_DA_IMAGEM) / LADO_DO_ATLAS;
    }
    atlas = LoadTextureFromImage(folha);
    GenTextureMipmaps(&atlas);
    SetTextureFilter(atlas, TEXTURE_FILTER_TRILINEAR);
    UnloadImage(folha);
    limparFumaca();
}

/*
 * descarregarFumaca
 * Recebe: nada.
 * Devolve: nada.
 */
void descarregarFumaca(void) {
    UnloadTexture(atlas);
}

/*
 * limparFumaca
 * Tira todas as nuvens (corrida nova).
 * Recebe: nada.
 * Devolve: nada.
 */
void limparFumaca(void) {
    for (int i = 0; i < MAXIMO_DE_NUVENS; i++) {
        nuvens[i].vida = 0.0f;
    }
}

/*
 * soltarNuvem
 * Solta uma nuvem nova na roda de um carro.
 * Recebe: onde está a roda, a velocidade do carro, a densidade (0 a 1) e o tipo.
 * Devolve: nada.
 */
void soltarNuvem(Vector3 roda, Vector3 velocidadeDoCarro, float forca, TipoDeNuvem tipo) {
    Nuvem *nuvem = &nuvens[proximaNuvem];
    proximaNuvem = (proximaNuvem + 1) % MAXIMO_DE_NUVENS;

    nuvem->posicao = roda;
    nuvem->velocidade = (Vector3){ velocidadeDoCarro.x * ARRASTO_DO_CARRO + sortear(-ESPALHAR, ESPALHAR),
                                   SUBIDA * sortear(0.6f, 1.4f),
                                   velocidadeDoCarro.z * ARRASTO_DO_CARRO + sortear(-ESPALHAR, ESPALHAR) };
    if (tipo == NUVEM_DE_DRIFT) {
        nuvem->velocidade.x += velocidadeDoCarro.x * (ARRASTO_DO_DRIFT - ARRASTO_DO_CARRO);
        nuvem->velocidade.z += velocidadeDoCarro.z * (ARRASTO_DO_DRIFT - ARRASTO_DO_CARRO);
        nuvem->velocidade.y *= SUBIDA_DO_DRIFT / SUBIDA;
    }
    nuvem->idade = 0.0f;
    nuvem->vida = ((tipo == NUVEM_DE_DRIFT) ? VIDA_DO_DRIFT : (tipo == NUVEM_DE_PNEU) ? VIDA_DO_PNEU : VIDA_DA_POEIRA) * sortear(0.8f, 1.2f);
    nuvem->forca = forca;
    nuvem->giro = sortear(0.0f, 360.0f);
    nuvem->giroPorSegundo = sortear(-40.0f, 40.0f);
    nuvem->imagem = GetRandomValue(0, VARIACOES_DE_IMAGEM - 1);
    nuvem->tipo = tipo;
}

/*
 * atualizarFumaca
 * Envelhece e move as nuvens. Com tempo 0 (pausa), elas ficam paradas no ar.
 * Recebe: o tempo que passou na corrida.
 * Devolve: nada.
 */
void atualizarFumaca(float tempoQuadro) {
    float freio = expf(-FREIO_DO_AR * tempoQuadro);
    float freioNoDrift = expf(-FREIO_DO_AR_NO_DRIFT * tempoQuadro);
    for (int i = 0; i < MAXIMO_DE_NUVENS; i++) {
        Nuvem *nuvem = &nuvens[i];
        if (nuvem->vida <= 0.0f) {
            continue;
        }
        nuvem->idade += tempoQuadro;
        if (nuvem->idade >= nuvem->vida) {
            nuvem->vida = 0.0f;
            continue;
        }
        nuvem->posicao.x += nuvem->velocidade.x * tempoQuadro;
        nuvem->posicao.y += nuvem->velocidade.y * tempoQuadro;
        nuvem->posicao.z += nuvem->velocidade.z * tempoQuadro;
        float freioDesta = (nuvem->tipo == NUVEM_DE_DRIFT) ? freioNoDrift : freio;
        nuvem->velocidade.x *= freioDesta;
        nuvem->velocidade.z *= freioDesta;
        nuvem->giro += nuvem->giroPorSegundo * tempoQuadro;
    }
}

/*
 * prepararBase
 * Guarda o eixo horizontal dos borrões deste quadro: a direita da câmera, que é a
 * primeira coluna da matriz de vista. O DrawBillboardPro da raylib refaz essa matriz
 * inteira em cada borrão, e são milhares por quadro — a conta é a mesma para todos.
 * Chamar uma vez por quadro, antes dos borrões.
 * Recebe: a câmera.
 * Devolve: nada.
 */
static void prepararBase(Camera3D camera) {
    Matrix vista = MatrixLookAt(camera.position, camera.target, camera.up);
    direitaDaCamera = (Vector3){ vista.m0, vista.m4, vista.m8 };
    inclinacaoDaBase = direitaDaCamera.y;
    float abertura = 1.0f - inclinacaoDaBase * inclinacaoDaBase;
    aberturaDaBase = (abertura > 1e-6f) ? sqrtf(abertura) : 1.0f;   /* a câmera deitada de lado não acontece */
}

/*
 * desenharBorrao3D
 * Um borrão virado para a câmera, no lugar de DrawBillboardPro. Faz a mesma conta, mas sem
 * refazer a matriz da câmera a cada borrão e sem as quatro rotações em volta de um eixo:
 * como o quadrado está no plano dos dois eixos da base, girá-lo é girar os quatro cantos
 * dentro desse plano, com um seno e um cosseno só.
 * Chamar entre rlBegin(RL_QUADS) e rlEnd(), com o atlas já escolhido.
 * Recebe: a variação, o centro, o tamanho em metros, o giro em graus e a cor.
 * Devolve: nada.
 */
static void desenharBorrao3D(int imagem, Vector3 centro, float tamanho, float giro, Color cor) {
    float angulo = giro * DEG2RAD;
    float c = cosf(angulo), s = sinf(angulo) / aberturaDaBase;
    float k = inclinacaoDaBase;
    /* Os quatro cantos do quadrado, em múltiplos do tamanho, na base (direita, para cima) */
    static const float CANTO_A[4] = { -0.5f,  0.5f, 0.5f, -0.5f };
    static const float CANTO_B[4] = { -0.5f, -0.5f, 0.5f,  0.5f };
    const CelulaDoAtlas *celula = &celulas[imagem];
    static const int CANTO_U[4] = { 0, 1, 1, 0 };   /* a ordem de DrawBillboardPro: v de baixo primeiro */
    static const int CANTO_V[4] = { 1, 1, 0, 0 };
    rlColor4ub(cor.r, cor.g, cor.b, cor.a);
    for (int i = 0; i < 4; i++) {
        float a0 = CANTO_A[i], b0 = CANTO_B[i];
        float a = a0 * c - (a0 * k + b0) * s;
        float b = b0 * c + (a0 + b0 * k) * s;
        rlTexCoord2f(CANTO_U[i] ? celula->u1 : celula->u0, CANTO_V[i] ? celula->v1 : celula->v0);
        rlVertex3f(centro.x + tamanho * a * direitaDaCamera.x,
                   centro.y + tamanho * (a * direitaDaCamera.y + b),
                   centro.z + tamanho * a * direitaDaCamera.z);
    }
}

/*
 * desenharFumaca3D
 * Desenha as nuvens, misturadas com o que está atrás. Elas respeitam o que
 * está na frente (um muro esconde a fumaça), mas não escrevem na
 * profundidade, para uma não recortar a outra. Como todas usam o atlas, as
 * mil e quatrocentas saem numa chamada de desenho. Chamar dentro de
 * BeginMode3D, depois de tudo o que é sólido.
 * Recebe: a câmera.
 * Devolve: nada.
 */
void desenharFumaca3D(Camera3D camera) {
    rlDrawRenderBatchActive();
    BeginBlendMode(BLEND_ALPHA);
    rlDisableDepthMask();
    prepararBase(camera);
    rlSetTexture(atlas.id);
    rlBegin(RL_QUADS);
    for (int i = 0; i < MAXIMO_DE_NUVENS; i++) {
        Nuvem *nuvem = &nuvens[i];
        if (nuvem->vida <= 0.0f) {
            continue;
        }
        float fase = nuvem->idade / nuvem->vida;
        int ehDrift = (nuvem->tipo == NUVEM_DE_DRIFT);
        int ehPneu = (nuvem->tipo == NUVEM_DE_PNEU) || ehDrift;
        float tamanhoFinal = ehDrift ? TAMANHO_FINAL_DRIFT : ehPneu ? TAMANHO_FINAL_PNEU : TAMANHO_FINAL_POEIRA;
        float tamanhoInicial = ehDrift ? TAMANHO_INICIAL_DRIFT : TAMANHO_INICIAL;
        float tamanho = tamanhoInicial + (tamanhoFinal - tamanhoInicial) * sqrtf(fase);   /* cresce rápido e depois devagar */
        float entrada = fminf(fase / 0.08f, 1.0f);                                          /* não aparece de repente */
        float opacidade = (ehDrift ? OPACIDADE_DRIFT : ehPneu ? OPACIDADE_PNEU : OPACIDADE_POEIRA) * nuvem->forca * entrada * (1.0f - fase) * (1.0f - fase);

        desenharBorrao3D(nuvem->imagem, nuvem->posicao, tamanho, nuvem->giro,
                         Fade(ehPneu ? COR_DO_PNEU : COR_DA_POEIRA, opacidade));
    }
    rlEnd();
    rlSetTexture(0);
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
    EndBlendMode();
}

/*
 * desenharChamaDoNitro
 * A chama azul: vários borrões somando luz, do escapamento para trás, cada vez
 * menores e mais azuis. O comprimento treme a cada quadro, como fogo.
 * Chamar dentro de BeginMode3D, depois de tudo o que é sólido.
 * Recebe: a câmera, a saída do escapamento e a direção para trás do carro (unitária).
 * Devolve: nada.
 */
void desenharChamaDoNitro(Vector3 escape, Vector3 paraTras) {
    float comprimento = COMPRIMENTO_DA_CHAMA * sortear(0.75f, 1.2f);
    for (int i = 0; i < PEDACOS_DA_CHAMA; i++) {
        float fase = (float) i / (PEDACOS_DA_CHAMA - 1);
        float distancia = comprimento * fase;
        Vector3 centro = { escape.x + paraTras.x * distancia, escape.y + paraTras.y * distancia + sortear(-0.03f, 0.03f),
                           escape.z + paraTras.z * distancia };
        float tamanho = LARGURA_DA_CHAMA * (1.0f - 0.72f * fase) * sortear(0.85f, 1.15f);
        Color cor = ColorLerp(COR_DA_CHAMA_PERTO, COR_DA_CHAMA_LONGE, fase);
        /* Uma variação só, girada a cada pedaço: varia sem mudar de célula do atlas */
        desenharBorrao3D(0, centro, tamanho, sortear(0.0f, 360.0f), Fade(cor, 1.0f - 0.55f * fase));
    }
    /* O miolo branco, bem na boca do escapamento */
    desenharBorrao3D(0, escape, LARGURA_DA_CHAMA * 0.55f, 0.0f, WHITE);
}

/*
 * comecarChamas / terminarChamas
 * As chamas de todos os carros vão num bloco só, somando luz, sem escrever na profundidade.
 * Como todas usam a mesma célula do atlas, o bloco inteiro sai numa chamada de desenho.
 * Recebe: a câmera (comecarChamas), para onde os borrões se viram.
 * Devolve: nada.
 */
void comecarChamas(Camera3D camera) {
    rlDrawRenderBatchActive();
    BeginBlendMode(BLEND_ADDITIVE);
    rlDisableDepthMask();
    prepararBase(camera);
    rlSetTexture(atlas.id);
    rlBegin(RL_QUADS);
}

void terminarChamas(void) {
    rlEnd();
    rlSetTexture(0);
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
    EndBlendMode();
}
