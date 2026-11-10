/*
 * carreta.c
 * ------------------------------------------------------------------
 * O lado de dentro da carreta do Mack. Veja carreta.h.
 *
 * As medidas são em metros, no "lado de dentro": x de uma parede à outra
 * (0 no meio), y do piso para cima e z da porta para o fundo. Sem luz
 * de verdade: as cores já vêm com o clima (o roxo do neon, o quente dos
 * spots e o escuro crescendo para o fundo).
 * ------------------------------------------------------------------
 */

#include <math.h>
#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"
#include "carreta.h"
#include "visual.h"

#define NEON                  (Color){ 230, 72, 255, 255 }
#define NEON_FRACO            (Color){ 140, 48, 180, 255 }
#define CIANO                 (Color){ 80, 220, 255, 255 }
#define SPOT                  (Color){ 255, 186, 104, 255 }
#define OURO                  (Color){ 236, 184, 52, 255 }
#define OURO_ESCURO           (Color){ 168, 118, 30, 255 }
#define MADEIRA               (Color){ 92, 54, 32, 255 }
#define ALTURA_DA_PRATELEIRA  2.9f      /* acima do teto de qualquer carro */
#define FUNDO_DA_PRATELEIRA   0.45f
#define CHANFRO               0.55f     /* o teto desce em chanfro nos últimos 55 cm de cada lado */
#define LARGURA_DO_NEON       0.035f
#define TOTAL_DE_QUADROS      6

static int carregado = 0;
static Texture2D piso, chapa, teto, telao, caixaDeSom;
static Texture2D quadros[TOTAL_DE_QUADROS];
static unsigned int sorteio = 12345u;

/* Um sorteio só daqui (não mexe nos sorteios do jogo) */
static int sortear(int ate) {
    sorteio = sorteio * 1664525u + 1013904223u;
    return (int) ((sorteio >> 8) % (unsigned int) ate);
}

static Texture2D virarTextura(Image imagem, int repetir) {
    Texture2D textura = LoadTextureFromImage(imagem);
    UnloadImage(imagem);
    GenTextureMipmaps(&textura);
    SetTextureFilter(textura, TEXTURE_FILTER_TRILINEAR);
    SetTextureWrap(textura, repetir ? TEXTURE_WRAP_REPEAT : TEXTURE_WRAP_CLAMP);
    return textura;
}

/* Um triângulo cheio na imagem, valha a ordem dos cantos que valer */
static void triangulo(Image *imagem, Vector2 a, Vector2 b, Vector2 c, Color cor) {
    ImageDrawTriangle(imagem, a, b, c, cor);
    ImageDrawTriangle(imagem, a, c, b, cor);
}

/* Um texto centralizado na imagem, com sombra */
static void textoNaImagem(Image *imagem, TipoFonte fonte, const char *texto, float centroX, float y, float tamanho, Color cor, Color sombra) {
    Font f = fonteDoJogo(fonte);
    Vector2 medida = MeasureTextEx(f, texto, tamanho, 1.0f);
    Vector2 lugar = { centroX - medida.x / 2.0f, y };
    ImageDrawTextEx(imagem, f, texto, (Vector2){ lugar.x + 3.0f, lugar.y + 3.0f }, tamanho, 1.0f, sombra);
    ImageDrawTextEx(imagem, f, texto, lugar, tamanho, 1.0f, cor);
}

/* O raio estilizado (dois pedaços inclinados) */
static void raioNaImagem(Image *imagem, float x, float y, float escala, Color cor) {
    Vector2 p[7] = { { 50, 0 }, { 100, 0 }, { 62, 92 }, { 96, 92 }, { 20, 220 }, { 38, 112 }, { 4, 112 } };
    for (int i = 0; i < 7; i++) {
        p[i] = (Vector2){ x + p[i].x * escala, y + p[i].y * escala };
    }
    triangulo(imagem, p[0], p[1], p[2], cor);
    triangulo(imagem, p[0], p[2], p[5], cor);
    triangulo(imagem, p[0], p[5], p[6], cor);
    triangulo(imagem, p[2], p[3], p[4], cor);
    triangulo(imagem, p[2], p[4], p[5], cor);
}

/* Os quadros da parede: artes do jogo, em retrato, com moldura */
static Image desenharQuadro(int qual) {
    const int L = 192, A = 256;
    Image imagem = GenImageColor(L, A, BLACK);
    switch (qual) {
        case 0:   /* o 95, sobre um raio */
            ImageDrawRectangle(&imagem, 0, 0, L, A, (Color){ 196, 22, 26, 255 });
            raioNaImagem(&imagem, 44, 22, 0.95f, (Color){ 120, 10, 10, 255 });
            raioNaImagem(&imagem, 40, 18, 0.95f, (Color){ 255, 206, 32, 255 });
            textoNaImagem(&imagem, FONTE_TITULO, "95", L / 2.0f, 128.0f, 104.0f, (Color){ 255, 236, 120, 255 }, (Color){ 60, 0, 0, 255 });
            break;
        case 1:   /* KATCHAU! */
            ImageDrawRectangle(&imagem, 0, 0, L, A, (Color){ 250, 198, 40, 255 });
            for (int i = -A; i < L; i += 36) {
                triangulo(&imagem, (Vector2){ (float) i, (float) A }, (Vector2){ (float) i + 14, (float) A }, (Vector2){ (float) i + A + 14, 0 }, (Color){ 244, 170, 30, 255 });
                triangulo(&imagem, (Vector2){ (float) i, (float) A }, (Vector2){ (float) i + A + 14, 0 }, (Vector2){ (float) i + A, 0 }, (Color){ 244, 170, 30, 255 });
            }
            textoNaImagem(&imagem, FONTE_TITULO, "KATCHAU!", L / 2.0f, 30.0f, 44.0f, (Color){ 214, 24, 22, 255 }, (Color){ 120, 40, 0, 255 });
            raioNaImagem(&imagem, 62, 92, 0.62f, (Color){ 214, 24, 22, 255 });
            textoNaImagem(&imagem, FONTE_TITULO, "95", L / 2.0f, 196.0f, 44.0f, (Color){ 214, 24, 22, 255 }, (Color){ 120, 40, 0, 255 });
            break;
        case 2: { /* a Copa Pistão: o troféu sobre a bandeira quadriculada */
            ImageDrawRectangle(&imagem, 0, 0, L, A, (Color){ 18, 28, 78, 255 });
            for (int x = 0; x < L; x += 16) {
                for (int y = 196; y < A; y += 16) {
                    ImageDrawRectangle(&imagem, x, y, 16, 16, ((x + y) / 16) % 2 ? WHITE : BLACK);
                }
            }
            ImageDrawCircle(&imagem, L / 2, 88, 40, OURO);
            ImageDrawRectangle(&imagem, L / 2 - 40, 48, 80, 40, (Color){ 18, 28, 78, 255 });
            ImageDrawRectangle(&imagem, L / 2 - 46, 44, 92, 8, OURO);
            ImageDrawRectangle(&imagem, L / 2 - 8, 124, 16, 34, OURO_ESCURO);
            ImageDrawRectangle(&imagem, L / 2 - 30, 156, 60, 14, OURO);
            textoNaImagem(&imagem, FONTE_TITULO, "COPA PISTÃO", L / 2.0f, 8.0f, 30.0f, WHITE, BLACK);
            break;
        }
        case 3:   /* Radiator Springs, no pôr do sol */
            for (int y = 0; y < A; y++) {
                float t = (float) y / A;
                ImageDrawRectangle(&imagem, 0, y, L, 1, ColorLerp((Color){ 255, 160, 70, 255 }, (Color){ 110, 40, 120, 255 }, t));
            }
            ImageDrawCircle(&imagem, L / 2, 120, 34, (Color){ 255, 226, 130, 255 });
            ImageDrawRectangle(&imagem, 10, 140, 60, 116, (Color){ 70, 24, 40, 255 });
            ImageDrawRectangle(&imagem, 0, 168, 30, 88, (Color){ 70, 24, 40, 255 });
            ImageDrawRectangle(&imagem, 120, 150, 72, 106, (Color){ 70, 24, 40, 255 });
            triangulo(&imagem, (Vector2){ 70, 140 }, (Vector2){ 92, 256 }, (Vector2){ 70, 256 }, (Color){ 70, 24, 40, 255 });
            ImageDrawRectangle(&imagem, 0, 226, L, 30, (Color){ 52, 18, 34, 255 });
            textoNaImagem(&imagem, FONTE_TITULO, "RADIATOR", L / 2.0f, 14.0f, 34.0f, WHITE, (Color){ 90, 30, 40, 255 });
            textoNaImagem(&imagem, FONTE_TITULO, "SPRINGS", L / 2.0f, 50.0f, 34.0f, WHITE, (Color){ 90, 30, 40, 255 });
            break;
        case 4:   /* Nº 1 */
            ImageDrawRectangle(&imagem, 0, 0, L, A, (Color){ 238, 234, 226, 255 });
            ImageDrawCircle(&imagem, L / 2, 104, 70, (Color){ 206, 22, 26, 255 });
            ImageDrawCircle(&imagem, L / 2, 104, 58, (Color){ 236, 40, 40, 255 });
            textoNaImagem(&imagem, FONTE_TITULO, "Nº 1", L / 2.0f, 70.0f, 60.0f, WHITE, (Color){ 120, 0, 0, 255 });
            textoNaImagem(&imagem, FONTE_TITULO, "CAMPEÃO", L / 2.0f, 190.0f, 36.0f, (Color){ 206, 22, 26, 255 }, (Color){ 200, 190, 180, 255 });
            break;
        default: { /* o carro 95 na pista */
            for (int y = 0; y < A; y++) {
                float t = (float) y / A;
                ImageDrawRectangle(&imagem, 0, y, L, 1, ColorLerp((Color){ 90, 170, 240, 255 }, (Color){ 30, 70, 150, 255 }, t));
            }
            ImageDrawRectangle(&imagem, 0, 176, L, 80, (Color){ 50, 50, 56, 255 });
            for (int x = 0; x < L; x += 40) {
                ImageDrawRectangle(&imagem, x, 214, 22, 5, (Color){ 240, 240, 240, 255 });
            }
            ImageDrawRectangle(&imagem, 26, 140, 140, 36, (Color){ 214, 24, 22, 255 });
            ImageDrawCircle(&imagem, 96, 140, 40, (Color){ 214, 24, 22, 255 });
            ImageDrawRectangle(&imagem, 56, 104, 80, 36, (Color){ 214, 24, 22, 255 });
            ImageDrawRectangle(&imagem, 70, 112, 52, 20, (Color){ 230, 230, 240, 255 });
            ImageDrawCircle(&imagem, 56, 178, 16, BLACK);
            ImageDrawCircle(&imagem, 136, 178, 16, BLACK);
            ImageDrawCircle(&imagem, 56, 178, 7, (Color){ 214, 24, 22, 255 });
            ImageDrawCircle(&imagem, 136, 178, 7, (Color){ 214, 24, 22, 255 });
            textoNaImagem(&imagem, FONTE_TITULO, "#95", L / 2.0f, 22.0f, 54.0f, WHITE, (Color){ 20, 40, 90, 255 });
            break;
        }
    }
    /* A moldura: dourada nuns, escura noutros */
    Color moldura = (qual % 2 == 0) ? OURO_ESCURO : (Color){ 30, 24, 28, 255 };
    ImageDrawRectangleLines(&imagem, (Rectangle){ 0, 0, (float) L, (float) A }, 8, moldura);
    ImageDrawRectangleLines(&imagem, (Rectangle){ 8, 8, (float) L - 16, (float) A - 16 }, 2, Fade(BLACK, 0.6f));
    return imagem;
}

/*
 * carregarInteriorDaCarreta
 * Desenha as imagens uma vez: o piso de chapa xadrez, a chapa furada das paredes, o teto
 * estrelado, o telão, a caixa de som e os quadros.
 */
void carregarInteriorDaCarreta(void) {
    if (carregado) {
        return;
    }
    /* O piso: chapa xadrez, os relevos inclinados para um lado e para o outro */
    Image imagem = GenImageColor(128, 128, (Color){ 30, 29, 36, 255 });
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            float cx = i * 16 + 8.0f, cy = j * 16 + 8.0f;
            float angulo = ((i + j) % 2) ? 0.785f : -0.785f;
            Vector2 d1 = { cosf(angulo) * 6.0f, sinf(angulo) * 6.0f }, d2 = { -sinf(angulo) * 1.6f, cosf(angulo) * 1.6f };
            Vector2 a = { cx + d1.x, cy + d1.y }, b = { cx + d2.x, cy + d2.y }, c = { cx - d1.x, cy - d1.y }, d = { cx - d2.x, cy - d2.y };
            triangulo(&imagem, a, b, c, (Color){ 66, 64, 76, 255 });
            triangulo(&imagem, a, c, d, (Color){ 66, 64, 76, 255 });
        }
    }
    piso = virarTextura(imagem, 1);

    /* A chapa das paredes: furinhos em grade e uma emenda */
    imagem = GenImageColor(128, 128, (Color){ 40, 32, 52, 255 });
    for (int x = 4; x < 128; x += 8) {
        for (int y = 4; y < 128; y += 8) {
            ImageDrawCircle(&imagem, x + ((y / 8) % 2) * 4, y, 2, (Color){ 22, 18, 30, 255 });
        }
    }
    ImageDrawRectangle(&imagem, 0, 126, 128, 2, (Color){ 58, 50, 74, 255 });
    chapa = virarTextura(imagem, 1);

    /* O teto: um céu de estrelinhas ciano e brancas */
    imagem = GenImageColor(512, 256, (Color){ 8, 12, 28, 255 });
    for (int i = 0; i < 700; i++) {
        int x = sortear(512), y = sortear(256), grande = sortear(5) == 0;
        Color cor = (sortear(3) == 0) ? (Color){ 230, 240, 255, 255 } : (Color){ 90, 214, 255, 255 };
        ImageDrawRectangle(&imagem, x, y, grande ? 2 : 1, grande ? 2 : 1, cor);
    }
    teto = virarTextura(imagem, 1);

    /* O telão: o som do carro, com as barras do equalizador */
    imagem = GenImageColor(256, 128, (Color){ 16, 40, 140, 255 });
    ImageDrawRectangle(&imagem, 0, 0, 256, 22, (Color){ 40, 92, 224, 255 });
    textoNaImagem(&imagem, FONTE_FORTE, "RADIO 95 FM", 70.0f, 2.0f, 18.0f, WHITE, (Color){ 10, 20, 80, 255 });
    for (int i = 0; i < 18; i++) {
        int altura = 10 + sortear(48);
        ImageDrawRectangle(&imagem, 14 + i * 9, 100 - altura, 6, altura, CIANO);
    }
    triangulo(&imagem, (Vector2){ 190, 46 }, (Vector2){ 190, 74 }, (Vector2){ 214, 60 }, WHITE);
    ImageDrawRectangle(&imagem, 14, 108, 228, 6, (Color){ 40, 92, 224, 255 });
    ImageDrawRectangle(&imagem, 14, 108, 140, 6, CIANO);
    ImageDrawRectangleLines(&imagem, (Rectangle){ 0, 0, 256, 128 }, 5, (Color){ 12, 12, 20, 255 });
    telao = virarTextura(imagem, 0);

    /* A caixa de som */
    imagem = GenImageColor(64, 64, (Color){ 24, 20, 32, 255 });
    ImageDrawCircle(&imagem, 32, 32, 28, (Color){ 150, 60, 200, 255 });
    ImageDrawCircle(&imagem, 32, 32, 25, (Color){ 30, 26, 40, 255 });
    ImageDrawCircle(&imagem, 32, 32, 12, (Color){ 64, 58, 76, 255 });
    ImageDrawCircle(&imagem, 32, 32, 5, (Color){ 20, 18, 26, 255 });
    caixaDeSom = virarTextura(imagem, 0);

    for (int i = 0; i < TOTAL_DE_QUADROS; i++) {
        quadros[i] = virarTextura(desenharQuadro(i), 0);
    }
    carregado = 1;
}

void descarregarInteriorDaCarreta(void) {
    if (!carregado) {
        return;
    }
    UnloadTexture(piso);
    UnloadTexture(chapa);
    UnloadTexture(teto);
    UnloadTexture(telao);
    UnloadTexture(caixaDeSom);
    for (int i = 0; i < TOTAL_DE_QUADROS; i++) {
        UnloadTexture(quadros[i]);
    }
    carregado = 0;
}

/* ------------------------------------------------------------------
 * As peças
 * ------------------------------------------------------------------ */

/* Um retângulo (a, b, c, d em volta), com uma cor em cada canto e a imagem de (u0, v0) a (u1, v1) */
static void retangulo(unsigned int textura, Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color ca, Color cb, Color cc, Color cd,
                      float u0, float v0, float u1, float v1) {
    rlSetTexture(textura);
    rlBegin(RL_QUADS);
    rlColor4ub(ca.r, ca.g, ca.b, ca.a); rlTexCoord2f(u0, v0); rlVertex3f(a.x, a.y, a.z);
    rlColor4ub(cb.r, cb.g, cb.b, cb.a); rlTexCoord2f(u1, v0); rlVertex3f(b.x, b.y, b.z);
    rlColor4ub(cc.r, cc.g, cc.b, cc.a); rlTexCoord2f(u1, v1); rlVertex3f(c.x, c.y, c.z);
    rlColor4ub(cd.r, cd.g, cd.b, cd.a); rlTexCoord2f(u0, v1); rlVertex3f(d.x, d.y, d.z);
    rlEnd();
    rlSetTexture(0);
}

static void retanguloDeUmaCor(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color cor) {
    retangulo(rlGetTextureIdDefault(), a, b, c, d, cor, cor, cor, cor, 0, 0, 1, 1);
}

/* Mais escuro para o fundo: a luz entra pela porta */
static Color comLuz(Color cor, float z, float fundo) {
    float k = 1.0f - 0.62f * Clamp(z / fundo, 0.0f, 1.0f);
    return (Color){ (unsigned char) (cor.r * k), (unsigned char) (cor.g * k), (unsigned char) (cor.b * k), cor.a };
}

/* Uma faixa de neon numa parede (x fixo), de (y0, z0) até (y1, z1) */
static void neonNaParede(float x, float y0, float z0, float y1, float z1, Color cor) {
    float dy = y1 - y0, dz = z1 - z0, tamanho = sqrtf(dy * dy + dz * dz);
    if (tamanho < 1e-3f) {
        return;
    }
    float py = -dz / tamanho * LARGURA_DO_NEON, pz = dy / tamanho * LARGURA_DO_NEON;
    retanguloDeUmaCor((Vector3){ x, y0 - py, z0 - pz }, (Vector3){ x, y1 - py, z1 - pz }, (Vector3){ x, y1 + py, z1 + pz },
                      (Vector3){ x, y0 + py, z0 + pz }, cor);
}

/* Um troféu dourado (a base de madeira, a haste e a taça), em pé em (x, y, z) */
static void trofeu(float x, float y, float z, float tamanho) {
    DrawCube((Vector3){ x, y + 0.06f * tamanho, z }, 0.18f * tamanho, 0.12f * tamanho, 0.18f * tamanho, MADEIRA);
    DrawCylinderEx((Vector3){ x, y + 0.12f * tamanho, z }, (Vector3){ x, y + 0.30f * tamanho, z }, 0.03f * tamanho, 0.03f * tamanho, 8, OURO_ESCURO);
    DrawCylinderEx((Vector3){ x, y + 0.30f * tamanho, z }, (Vector3){ x, y + 0.54f * tamanho, z }, 0.05f * tamanho, 0.13f * tamanho, 14, OURO);
    DrawCylinderEx((Vector3){ x, y + 0.53f * tamanho, z }, (Vector3){ x, y + 0.55f * tamanho, z }, 0.135f * tamanho, 0.135f * tamanho, 14, OURO_ESCURO);
}

/* Uma miniatura de carro de corrida (vermelha, com o bico para o fundo) */
static void miniatura(float x, float y, float z) {
    DrawCube((Vector3){ x, y + 0.075f, z }, 0.15f, 0.07f, 0.30f, (Color){ 210, 24, 22, 255 });
    DrawCube((Vector3){ x, y + 0.13f, z - 0.02f }, 0.12f, 0.05f, 0.13f, (Color){ 150, 12, 12, 255 });
    DrawCube((Vector3){ x, y + 0.13f, z + 0.035f }, 0.11f, 0.035f, 0.02f, (Color){ 200, 220, 240, 255 });
    for (int i = 0; i < 4; i++) {
        float lado = (i % 2) ? 0.08f : -0.08f, frente = (i < 2) ? 0.09f : -0.09f;
        DrawCylinderEx((Vector3){ x + lado - 0.012f, y + 0.035f, z + frente }, (Vector3){ x + lado + 0.012f, y + 0.035f, z + frente },
                       0.035f, 0.035f, 10, (Color){ 20, 20, 22, 255 });
    }
}

/* Uma placa encostada na parede (do lado dado: -1 ou +1) */
static void placa(float x, float y, float z, float lado) {
    DrawCube((Vector3){ x, y + 0.17f, z }, 0.03f, 0.34f, 0.26f, MADEIRA);
    DrawCube((Vector3){ x - lado * 0.02f, y + 0.19f, z }, 0.012f, 0.18f, 0.16f, OURO);
}

/*
 * desenharInteriorDaCarreta
 * O piso, as paredes de chapa com o neon, as prateleiras (com troféus, miniaturas e placas), os
 * quadros, o teto chanfrado com spots e o miolo estrelado, e o fundo com o telão e as caixas de som.
 */
void desenharInteriorDaCarreta(Matrix deDentro, float largura, float altura, float fundo) {
    if (!carregado) {
        return;
    }
    float W = largura / 2.0f, H = altura, F = fundo;
    float topoDaParede = H - CHANFRO, Y = ALTURA_DA_PRATELEIRA;
    rlDrawRenderBatchActive();
    rlPushMatrix();
    rlMultMatrixf(MatrixToFloat(deDentro));

    /* O piso */
    Color pisoPerto = { 170, 164, 186, 255 };
    retangulo(piso.id, (Vector3){ -W, 0.0f, 0.0f }, (Vector3){ W, 0.0f, 0.0f }, (Vector3){ W, 0.0f, F }, (Vector3){ -W, 0.0f, F },
              pisoPerto, pisoPerto, comLuz(pisoPerto, F, F), comLuz(pisoPerto, F, F), 0.0f, 0.0f, 2.0f * W / 0.6f, F / 0.6f);

    for (int i = 0; i < 2; i++) {
        float lado = i ? 1.0f : -1.0f, x = lado * W;
        /* A parede, de chapa furada */
        Color parede = { 205, 186, 226, 255 };
        retangulo(chapa.id, (Vector3){ x, 0.0f, 0.0f }, (Vector3){ x, 0.0f, F }, (Vector3){ x, topoDaParede, F }, (Vector3){ x, topoDaParede, 0.0f },
                  parede, comLuz(parede, F, F), comLuz(parede, F, F), parede, 0.0f, 0.0f, F / 0.7f, topoDaParede / 0.7f);
        /* O chanfro do teto, com os painéis de borda acesa */
        Vector3 a = { x, topoDaParede, 0.0f }, b = { lado * (W - CHANFRO * 0.9f), H, 0.0f };
        Color chanfro = { 40, 34, 54, 255 };
        retanguloDeUmaCor(a, b, (Vector3){ b.x, b.y, F }, (Vector3){ a.x, a.y, F }, chanfro);
        for (float z = 0.6f; z < F - 0.8f; z += 1.6f) {
            float t0 = 0.22f, t1 = 0.78f;
            Vector3 p0 = Vector3Lerp(a, b, t0), p1 = Vector3Lerp(a, b, t1);
            p0.x -= lado * 0.01f; p1.x -= lado * 0.01f; p0.y -= 0.01f; p1.y -= 0.01f;
            Color painel = comLuz((Color){ 14, 12, 22, 255 }, z, F);
            retanguloDeUmaCor((Vector3){ p0.x, p0.y, z }, (Vector3){ p1.x, p1.y, z }, (Vector3){ p1.x, p1.y, z + 1.1f }, (Vector3){ p0.x, p0.y, z + 1.1f }, painel);
            Color borda = (i == 0) ? CIANO : NEON_FRACO;
            retanguloDeUmaCor((Vector3){ p0.x, p0.y - 0.005f, z }, (Vector3){ p1.x, p1.y - 0.005f, z }, (Vector3){ p1.x, p1.y - 0.005f, z + 0.03f },
                              (Vector3){ p0.x, p0.y - 0.005f, z + 0.03f }, borda);
            retanguloDeUmaCor((Vector3){ p0.x, p0.y - 0.005f, z + 1.07f }, (Vector3){ p1.x, p1.y - 0.005f, z + 1.07f },
                              (Vector3){ p1.x, p1.y - 0.005f, z + 1.1f }, (Vector3){ p0.x, p0.y - 0.005f, z + 1.1f }, borda);
        }
        /* Os spots quentes, numa fileira no alto da parede */
        float xs = x - lado * 0.02f;
        for (float z = 0.4f; z < F - 0.3f; z += 0.7f) {
            retanguloDeUmaCor((Vector3){ xs, topoDaParede - 0.12f, z - 0.05f }, (Vector3){ xs, topoDaParede - 0.12f, z + 0.05f },
                              (Vector3){ xs, topoDaParede - 0.04f, z + 0.05f }, (Vector3){ xs, topoDaParede - 0.04f, z - 0.05f }, SPOT);
        }

        /* O neon: uma faixa baixa, um zigue-zague no meio e uma faixa alta */
        float xn = x - lado * 0.02f;
        neonNaParede(xn, 0.85f, 0.2f, 0.85f, F - 0.3f, NEON);
        for (int k = 0; k < 7; k++) {
            float z0 = 0.4f + k * 1.6f, z1 = z0 + 1.6f;
            if (z1 > F - 0.4f) {
                break;
            }
            neonNaParede(xn, (k % 2) ? 2.3f : 1.3f, z0, (k % 2) ? 1.3f : 2.3f, z1, NEON);
        }
        neonNaParede(xn, topoDaParede - 0.2f, 0.2f, topoDaParede - 0.2f, F - 0.3f, NEON_FRACO);

        /* A prateleira, com o neon na beirada e as luzinhas embaixo */
        float xd = lado * (W - FUNDO_DA_PRATELEIRA), z0 = 0.25f, z1 = F - 0.6f;
        retanguloDeUmaCor((Vector3){ x, Y, z0 }, (Vector3){ xd, Y, z0 }, (Vector3){ xd, Y, z1 }, (Vector3){ x, Y, z1 }, (Color){ 64, 52, 78, 255 });
        retanguloDeUmaCor((Vector3){ xd, Y - 0.08f, z0 }, (Vector3){ xd, Y, z0 }, (Vector3){ xd, Y, z1 }, (Vector3){ xd, Y - 0.08f, z1 }, (Color){ 40, 34, 52, 255 });
        retanguloDeUmaCor((Vector3){ x, Y - 0.08f, z0 }, (Vector3){ xd, Y - 0.08f, z0 }, (Vector3){ xd, Y - 0.08f, z1 }, (Vector3){ x, Y - 0.08f, z1 }, (Color){ 28, 24, 38, 255 });
        retanguloDeUmaCor((Vector3){ xd - lado * 0.005f, Y - 0.07f, z0 }, (Vector3){ xd - lado * 0.005f, Y - 0.035f, z0 },
                          (Vector3){ xd - lado * 0.005f, Y - 0.035f, z1 }, (Vector3){ xd - lado * 0.005f, Y - 0.07f, z1 }, NEON);
        for (float z = z0 + 0.4f; z < z1; z += 0.9f) {
            float xl = (x + xd) / 2.0f;
            retanguloDeUmaCor((Vector3){ xl - 0.05f, Y - 0.085f, z - 0.05f }, (Vector3){ xl + 0.05f, Y - 0.085f, z - 0.05f },
                              (Vector3){ xl + 0.05f, Y - 0.085f, z + 0.05f }, (Vector3){ xl - 0.05f, Y - 0.085f, z + 0.05f }, SPOT);
        }

        /* Em cima da prateleira: troféus, miniaturas e placas, alternando */
        int k = i;
        for (float z = z0 + 0.45f; z < z1 - 0.2f; z += 0.85f, k++) {
            float xc = lado * (W - 0.24f);
            switch (k % 4) {
                case 0: trofeu(xc, Y, z, 1.0f); break;
                case 1: miniatura(xc, Y, z); break;
                case 2: trofeu(xc, Y, z, 1.35f); break;
                default: placa(lado * (W - 0.04f), Y, z, lado); break;
            }
        }

        /* Os quadros, acima da prateleira (a imagem cresce para a direita de quem olha a parede) */
        int q = i * 3;
        for (float z = 1.0f; z + 0.5f < z1; z += 1.45f, q++) {
            float xq = x - lado * 0.015f, yb = Y + 0.55f, yc = Y + 1.75f;
            float za = z - 0.45f, zb = z + 0.45f;
            Color cor = comLuz(WHITE, z, F * 1.4f);
            if (lado > 0.0f) {   /* na parede do +x, a direita de quem olha é o +z */
                retangulo(quadros[q % TOTAL_DE_QUADROS].id, (Vector3){ xq, yc, za }, (Vector3){ xq, yc, zb }, (Vector3){ xq, yb, zb },
                          (Vector3){ xq, yb, za }, cor, cor, cor, cor, 0.0f, 0.0f, 1.0f, 1.0f);
            } else {
                retangulo(quadros[q % TOTAL_DE_QUADROS].id, (Vector3){ xq, yc, zb }, (Vector3){ xq, yc, za }, (Vector3){ xq, yb, za },
                          (Vector3){ xq, yb, zb }, cor, cor, cor, cor, 0.0f, 0.0f, 1.0f, 1.0f);
            }
        }
    }

    /* O miolo do teto: o céu de estrelinhas */
    float xt = W - CHANFRO * 0.9f;
    retangulo(teto.id, (Vector3){ -xt, H, 0.0f }, (Vector3){ xt, H, 0.0f }, (Vector3){ xt, H, F }, (Vector3){ -xt, H, F },
              WHITE, WHITE, WHITE, WHITE, 0.0f, 0.0f, 1.0f, F / 6.0f);

    /* O fundo: a parede, o telão com moldura de neon e as caixas de som */
    Color fundoDaParede = { 26, 22, 34, 255 };
    retanguloDeUmaCor((Vector3){ -W, 0.0f, F }, (Vector3){ W, 0.0f, F }, (Vector3){ W, H, F }, (Vector3){ -W, H, F }, fundoDaParede);
    float yTelao = H * 0.52f, meiaTela = fminf(1.3f, W * 0.62f);
    retanguloDeUmaCor((Vector3){ -meiaTela - 0.08f, yTelao - 0.08f, F - 0.01f }, (Vector3){ meiaTela + 0.08f, yTelao - 0.08f, F - 0.01f },
                      (Vector3){ meiaTela + 0.08f, yTelao + 1.38f, F - 0.01f }, (Vector3){ -meiaTela - 0.08f, yTelao + 1.38f, F - 0.01f }, NEON);
    retangulo(telao.id, (Vector3){ meiaTela, yTelao + 1.3f, F - 0.02f }, (Vector3){ -meiaTela, yTelao + 1.3f, F - 0.02f },
              (Vector3){ -meiaTela, yTelao, F - 0.02f }, (Vector3){ meiaTela, yTelao, F - 0.02f }, WHITE, WHITE, WHITE, WHITE, 0.0f, 0.0f, 1.0f, 1.0f);
    for (int i = 0; i < 2; i++) {
        float xc = (i ? 1.0f : -1.0f) * (meiaTela + 0.12f + 0.32f);
        if (fabsf(xc) + 0.3f > W) {
            continue;
        }
        retangulo(caixaDeSom.id, (Vector3){ xc + 0.3f, yTelao + 0.95f, F - 0.02f }, (Vector3){ xc - 0.3f, yTelao + 0.95f, F - 0.02f },
                  (Vector3){ xc - 0.3f, yTelao + 0.35f, F - 0.02f }, (Vector3){ xc + 0.3f, yTelao + 0.35f, F - 0.02f },
                  WHITE, WHITE, WHITE, WHITE, 0.0f, 0.0f, 1.0f, 1.0f);
    }
    retanguloDeUmaCor((Vector3){ -W, 0.9f, F - 0.02f }, (Vector3){ W, 0.9f, F - 0.02f }, (Vector3){ W, 0.95f, F - 0.02f }, (Vector3){ -W, 0.95f, F - 0.02f }, CIANO);

    rlPopMatrix();
    rlDrawRenderBatchActive();
}

/*
 * desenharCaixasDeRoda
 * Cada caixa: as seis faces de chapa (um pouco mais clara em cima) e um friso de neon na beira de cima
 * do lado do corredor. Fecham também o arco das rodas, visto de fora da carreta.
 */
void desenharCaixasDeRoda(Matrix deDentro, const Vector3 *de, const Vector3 *ate, int total) {
    if (!carregado || total <= 0) {
        return;
    }
    rlDrawRenderBatchActive();
    rlPushMatrix();
    rlMultMatrixf(MatrixToFloat(deDentro));
    Color lado = { 120, 104, 142, 255 }, topo = { 150, 132, 172, 255 }, sombra = { 20, 18, 24, 255 };   /* sombra: o que se vê de fora, pelo arco */
    for (int i = 0; i < total; i++) {
        Vector3 a = { fminf(de[i].x, ate[i].x), fminf(de[i].y, ate[i].y), fminf(de[i].z, ate[i].z) };
        Vector3 b = { fmaxf(de[i].x, ate[i].x), fmaxf(de[i].y, ate[i].y), fmaxf(de[i].z, ate[i].z) };
        float u = (b.z - a.z) / 0.7f, v = (b.y - a.y) / 0.7f, w = (b.x - a.x) / 0.7f;
        retangulo(chapa.id, (Vector3){ a.x, b.y, a.z }, (Vector3){ b.x, b.y, a.z }, (Vector3){ b.x, b.y, b.z }, (Vector3){ a.x, b.y, b.z },
                  topo, topo, topo, topo, 0, 0, w, u);
        Color ladoA = (fabsf(a.x) > fabsf(b.x)) ? sombra : lado, ladoB = (fabsf(b.x) > fabsf(a.x)) ? sombra : lado;   /* o da parede, escuro */
        retangulo(chapa.id, (Vector3){ a.x, a.y, a.z }, (Vector3){ b.x, a.y, a.z }, (Vector3){ b.x, a.y, b.z }, (Vector3){ a.x, a.y, b.z },
                  sombra, sombra, sombra, sombra, 0, 0, w, u);
        retangulo(chapa.id, (Vector3){ a.x, a.y, a.z }, (Vector3){ a.x, a.y, b.z }, (Vector3){ a.x, b.y, b.z }, (Vector3){ a.x, b.y, a.z },
                  ladoA, ladoA, ladoA, ladoA, 0, 0, u, v);
        retangulo(chapa.id, (Vector3){ b.x, a.y, a.z }, (Vector3){ b.x, a.y, b.z }, (Vector3){ b.x, b.y, b.z }, (Vector3){ b.x, b.y, a.z },
                  ladoB, ladoB, ladoB, ladoB, 0, 0, u, v);
        retangulo(chapa.id, (Vector3){ a.x, a.y, a.z }, (Vector3){ b.x, a.y, a.z }, (Vector3){ b.x, b.y, a.z }, (Vector3){ a.x, b.y, a.z },
                  lado, lado, lado, lado, 0, 0, w, v);
        retangulo(chapa.id, (Vector3){ a.x, a.y, b.z }, (Vector3){ b.x, a.y, b.z }, (Vector3){ b.x, b.y, b.z }, (Vector3){ a.x, b.y, b.z },
                  lado, lado, lado, lado, 0, 0, w, v);
        /* o friso, na beira de cima do lado do corredor (o lado mais perto do meio) */
        float x = (fabsf(a.x) < fabsf(b.x)) ? a.x : b.x;
        float dentro = (x < 0.0f) ? 0.005f : -0.005f;
        retanguloDeUmaCor((Vector3){ x - dentro, b.y - 0.06f, a.z }, (Vector3){ x - dentro, b.y - 0.06f, b.z },
                          (Vector3){ x - dentro, b.y - 0.025f, b.z }, (Vector3){ x - dentro, b.y - 0.025f, a.z }, NEON);
    }
    rlPopMatrix();
    rlDrawRenderBatchActive();
}
