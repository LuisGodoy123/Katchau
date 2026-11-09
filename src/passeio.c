/*
 * passeio.c
 * ------------------------------------------------------------------
 * A tela do passeio livre. Veja passeio.h.
 *
 * O mapa (radiator_springs.glb) e a colisão (radiator_springs.col) são
 * carregados só na primeira vez que o jogador entra aqui. Os dois são
 * gerados por ferramentas/montar_mundo_aberto.py. Na mesma hora, o mapa
 * é fotografado de cima uma vez: a foto vira o minimapa e o mapa grande.
 * ------------------------------------------------------------------
 */

#include <math.h>
#include <string.h>
#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"
#include "passeio.h"
#include "livre.h"
#include "colisao.h"
#include "corrida.h"
#include "mundo.h"
#include "carros3d.h"
#include "fumaca.h"
#include "marcas.h"
#include "posprocesso.h"
#include "noite.h"
#include "configuracoes.h"
#include "controle.h"
#include "olhar.h"
#include "hud.h"
#include "visual.h"
#include "audio.h"
#include "efeitos.h"
#include "config.h"

#define ARQUIVO_DA_COLISAO    "recursos/modelos/radiator_springs.col"

/* Se o .col for antigo (sem os pontos): a rua principal e a pista de treino da versão atual */
#define LARGADA_X             675.0f
#define LARGADA_Z             135.9f
#define LARGADA_RUMO          (PI / 2.0f)
#define TREINO_X              998.4f
#define TREINO_Z              552.9f

#define GRAUS_DE_VISAO        65.0f
#define ATRASO_DA_CAMERA      0.22f     /* segundos: a câmera vem atrás do bico com este atraso */
#define ATRASO_DA_ALTURA      0.10f     /* segundos: e sobe e desce com o carro com este */
#define VELOCIDADE_DA_DISTANCIA 7.0f    /* 1/s: a troca de distância da câmera */
#define FOLGA_DA_CAMERA       1.2f      /* metros: a câmera nunca entra no chão */
#define OLHAR_PARA_BAIXO      5.0f      /* graus */
#define ALTURA_DA_SOMBRA      0.04f
#define SOMBRA_PERTO_DA_CAMERA 0.40f
#define ALCANCE_DA_SOMBRA     5.5f      /* metros abaixo do carro: mais longe que isso, sem sombra */
#define DEGRAU_DA_SOMBRA      1.0f      /* metros entre dois pontos vizinhos da sombra: mais que isso é beirada */
#define ENTRE_EIXOS           0.62f     /* em comprimentos do carro (como em render.c) */
#define BITOLA                0.82f     /* em larguras do carro */
#define PANCADA_QUE_SOA       0.12f
#define CAMERA_SEGUE_A_PAREDE 7.0f     /* por segundo: na parede, a câmera deita com o carro sem tranco */
#define CIMA_SEGUE            12.0f     /* por segundo: o "para cima" do desenho segue o da física, sem tranco */

/* Fumaça, poeira e marcas */
#define POEIRA_POR_SEGUNDO    44.0f     /* nuvens por roda, a toda na areia */
#define FUMACA_POR_SEGUNDO    40.0f     /* nuvens por roda, no drift cheio */
#define ESCORREGA_QUE_MARCA   0.22f
#define ESCORREGA_QUE_FUMA    0.30f
#define VELOCIDADE_QUE_LEVANTA 4.0f     /* m/s: abaixo disso a areia não levanta */
#define DENSIDADE_DA_POEIRA   2.2f      /* a poeira é mais densa que a fumaça do pneu */
#define CLAREIA_A_POEIRA      0.45f     /* a poeira é a cor do chão, bem mais clara (contra a própria areia) */
#define ALTURA_DO_ESCAPAMENTO 0.42f
#define LUGAR_DO_ESCAPAMENTO  0.22f

/* O minimapa e o mapa grande */
#define LADO_DA_FOTO          2048      /* pixels da foto do mapa, tirada de cima */
#define LADO_DO_MINIMAPA      256       /* pixels da imagem do minimapa */
#define RAIO_DO_MINIMAPA      420.0f    /* metros do carro até a borda do minimapa */
#define TAMANHO_DO_MINIMAPA   170.0f    /* na tela virtual */
#define COR_DO_FUNDO_DO_MAPA  (Color){ 196, 128, 84, 255 }

typedef struct {
    float atras, altura;
} Distancia;

static const Distancia DISTANCIAS[3] = { { 5.2f, 2.1f }, { 8.5f, 3.4f }, { 13.0f, 5.0f } };
static const char *NOMES_DOS_CHAOS[TOTAL_DE_CHAOS] = { "ASFALTO", "AREIA", "TERRA", "ROCHA", "" };

static MalhaDeColisao *malha = NULL;
static int tentouCarregar = 0;
static int pronto = 0;
static CarroLivre carro;
static int numero = 0;
static int pausado = 0, mapaAberto = 0, zoomDoMapa = 1;
static const float ZOOMS_DO_MAPA[3] = { 1.0f, 2.5f, 6.0f };
static Olhar olhar;
static float rumoDaCamera = 0.0f, alturaDaCamera = 0.0f, noWallride = 0.0f;
static Vector3 cimaDaCamera = { 0.0f, 1.0f, 0.0f }, frenteDaCamera = { 0.0f, 0.0f, -1.0f };
static int distanciaEscolhida = 1;
static Distancia distanciaAtual = { 8.5f, 3.4f };
static Vector3 cimaDoDesenho = { 0.0f, 1.0f, 0.0f };
static float largada[4] = { LARGADA_X, 0.0f, LARGADA_Z, LARGADA_RUMO };
static float treino[3] = { TREINO_X, 0.0f, TREINO_Z };

/* A foto do mapa, de cima: o canto de cima à esquerda é (mapaX0, mapaZ0); cada pixel tem metrosPorPixel */
static Texture2D fotoDoMapa = { 0 };
static RenderTexture2D minimapa = { 0 };
static float mapaX0 = 0.0f, mapaZ0 = 0.0f, metrosPorPixel = 1.0f;

/* ------------------------------------------------------------------
 * Carregar
 * ------------------------------------------------------------------ */

/*
 * fotografarMapa
 * O mapa inteiro visto de cima, numa imagem quadrada com o norte (-z) para cima.
 */
static void fotografarMapa(void) {
    float minimo[3], maximo[3];
    limitesDaMalha(malha, minimo, maximo);
    float lado = fmaxf(maximo[0] - minimo[0], maximo[2] - minimo[2]);
    float meioX = (minimo[0] + maximo[0]) / 2.0f, meioZ = (minimo[2] + maximo[2]) / 2.0f;
    mapaX0 = meioX - lado / 2.0f;
    mapaZ0 = meioZ - lado / 2.0f;
    metrosPorPixel = lado / LADO_DA_FOTO;

    Camera3D deCima = { 0 };
    deCima.position = (Vector3){ meioX, maximo[1] + 100.0f, meioZ };
    deCima.target = (Vector3){ meioX, minimo[1], meioZ };
    deCima.up = (Vector3){ 0.0f, 0.0f, -1.0f };
    deCima.fovy = lado;
    deCima.projection = CAMERA_ORTHOGRAPHIC;
    RenderTexture2D foto = LoadRenderTexture(LADO_DA_FOTO, LADO_DA_FOTO);
    prepararSombras(CENARIO_DO_MUNDO_ABERTO, deCima, 0);   /* sem sombra na foto */
    BeginTextureMode(foto);
    ClearBackground(COR_DO_FUNDO_DO_MAPA);
    prepararPlanosDaCamera();
    BeginMode3D(deCima);
    rlDisableBackfaceCulling();
    desenharMapaDoMundoAberto();
    rlEnableBackfaceCulling();
    EndMode3D();
    EndTextureMode();
    /* A imagem da RenderTexture sai de cabeça para baixo: vira, e guarda como textura comum */
    Image imagem = LoadImageFromTexture(foto.texture);
    ImageFlipVertical(&imagem);
    fotoDoMapa = LoadTextureFromImage(imagem);
    GenTextureMipmaps(&fotoDoMapa);
    SetTextureFilter(fotoDoMapa, TEXTURE_FILTER_TRILINEAR);
    UnloadImage(imagem);
    UnloadRenderTexture(foto);
    minimapa = LoadRenderTexture(LADO_DO_MINIMAPA, LADO_DO_MINIMAPA);
    SetTextureFilter(minimapa.texture, TEXTURE_FILTER_BILINEAR);
}

static void reiniciarCamera(void);

/*
 * prepararMapa
 * Na primeira vez: o desenho (mundo.c), a colisão e a foto de cima.
 */
static void prepararMapa(void) {
    if (tentouCarregar) {
        return;
    }
    tentouCarregar = 1;
    /* São alguns segundos: um quadro avisando, antes de travar carregando */
    BeginDrawing();
    ClearBackground(BLACK);
    comecarInterface(1);
    escreverCentralizado(FONTE_TITULO, "CARREGANDO RADIATOR SPRINGS...", LARGURA_TELA / 2.0f, 280.0f, 30.0f, WHITE);
    terminarInterface();
    EndDrawing();
    malha = carregarMalhaDeColisao(ARQUIVO_DA_COLISAO);
    if (malha == NULL) {
        TraceLog(LOG_WARNING, "PASSEIO: falta %s (gere com ferramentas/montar_mundo_aberto.py)", ARQUIVO_DA_COLISAO);
    } else {
        TraceLog(LOG_INFO, "PASSEIO: %d triângulos de colisão", triangulosDaMalha(malha));
        pontosDaMalha(malha, largada, treino);
    }
    pronto = carregarMundoAberto() && malha != NULL;
    if (pronto) {
        fotografarMapa();
    }
}

/*
 * iniciarPasseio
 * Recebe: o carro escolhido na garagem.
 * Devolve: nada.
 */
void iniciarPasseio(int numeroDoCarro) {
    prepararMapa();
    numero = numeroDoCarro;
    iniciarCarroLivre(&carro, fichaDoCarro(numero), largada[0], largada[1], largada[2], largada[3]);
    voltarAoComeco(&carro, malha);
    pausado = 0;
    mapaAberto = 0;
    olhar = olharParaAFrente();
    reiniciarCamera();
    distanciaAtual = DISTANCIAS[distanciaEscolhida];
    limparFumaca();
    iniciarEfeitosDaCorrida();   /* para os sons que tenham sobrado */
}

int passeioPausado(void) {
    return pausado;
}

void descarregarPasseio(void) {
    liberarMalhaDeColisao(malha);
    malha = NULL;
    if (fotoDoMapa.id != 0) {
        UnloadTexture(fotoDoMapa);
        UnloadRenderTexture(minimapa);
        fotoDoMapa.id = 0;
    }
}

/* ------------------------------------------------------------------
 * Atualizar
 * ------------------------------------------------------------------ */

static Tela sair(void) {
    pausado = 0;
    mapaAberto = 0;
    iniciarEfeitosDaCorrida();
    limparMarcas();   /* as marcas e a fumaça do passeio não vão para a próxima corrida */
    limparFumaca();
    return TELA_MENU;
}

/* A câmera e o desenho recomeçam atrás do carro, de pé (teleporte, entrada, queda do mundo) */
static void reiniciarCamera(void) {
    rumoDaCamera = carro.rumo;
    alturaDaCamera = carro.y;
    cimaDoDesenho = (Vector3){ 0.0f, 1.0f, 0.0f };
    noWallride = 0.0f;
    cimaDaCamera = (Vector3){ 0.0f, 1.0f, 0.0f };
    frenteDaCamera = (Vector3){ carro.frente[0], carro.frente[1], carro.frente[2] };
    limparMarcas();
}

static void teleportar(float x, float z, float rumo) {
    levarCarroPara(&carro, malha, x, z, rumo);
    reiniciarCamera();
    tocarSom(SOM_CONFIRMAR);
}

/*
 * soltarPoeiraEFumaca
 * Na areia e na terra, as rodas de trás levantam poeira da cor do chão (mais rápido, mais
 * poeira; derrapando, muito mais). No asfalto, o drift solta fumaça branca, e a arrancada
 * também. Marcas de pneu: no drift e na freada forte.
 */
static void soltarPoeiraEFumaca(Matrix noMundo, float tempoQuadro) {
    float rapidez = velocidadeDoCarroLivre(&carro);
    float meiaBitola = larguraDoCarro3D(numero) * BITOLA / 2.0f;
    float meioEntreEixos = comprimentoDoCarro3D(numero) * ENTRE_EIXOS / 2.0f;
    Vector3 frente = { noMundo.m8, noMundo.m9, noMundo.m10 };
    Vector3 velocidade = { carro.vx, carro.vy, carro.vz };

    /* Marcas */
    float marca = 0.0f;
    int todas = 0;
    if (carro.noChao && cimaDoDesenho.y > 0.6f && rapidez > 2.0f) {
        if (carro.escorregando > ESCORREGA_QUE_MARCA) {
            marca = fminf((carro.escorregando - ESCORREGA_QUE_MARCA) / 0.5f, 1.0f);
        } else if (carro.freando && velocidadeParaFrente(&carro) > 25.0f) {
            marca = 0.35f;
            todas = 1;
        }
        if (carro.chao == CHAO_AREIA || carro.chao == CHAO_TERRA) {
            marca *= 0.6f;   /* na terra, é o rastro do pneu */
        }
    }
    for (int roda = 0; roda < 4; roda++) {
        int deTras = (roda >= 2);
        if (marca <= 0.02f || (!deTras && !todas)) {
            soltarRoda(numero, roda);
            continue;
        }
        float lado = (roda % 2 == 0) ? 1.0f : -1.0f;
        Vector3 ponto = Vector3Transform((Vector3){ lado * meiaBitola, 0.0f, deTras ? -meioEntreEixos : meioEntreEixos }, noMundo);
        deixarMarca(numero, roda, ponto, frente, marca);
    }

    if (!carro.noChao || tempoQuadro <= 0.0f) {
        return;
    }
    /* Poeira ou fumaça? */
    float forca = 0.0f, porSegundo = 0.0f;
    TipoDeNuvem tipo = NUVEM_DE_PNEU;
    Color cor = WHITE;
    if ((carro.chao == CHAO_AREIA || carro.chao == CHAO_TERRA) && rapidez > VELOCIDADE_QUE_LEVANTA) {
        float solta = (carro.chao == CHAO_AREIA) ? 1.0f : 0.6f;
        forca = fminf(solta * (0.25f + 0.55f * fminf(rapidez / 40.0f, 1.0f) + 0.6f * carro.escorregando), 1.0f) * DENSIDADE_DA_POEIRA
                * fminf(rapidez / 15.0f, 1.0f);   /* devagar, pouca poeira (senão a nuvem cobre a tela) */
        porSegundo = POEIRA_POR_SEGUNDO * (0.35f + 0.65f * forca);
        /* Rápido ou de lado, a nuvem é a grande e demorada (a do drift), na cor da areia */
        tipo = (rapidez > 18.0f || carro.escorregando > 0.3f) ? NUVEM_DE_DRIFT : NUVEM_DE_POEIRA;
        unsigned char rgb[3];
        corDoChao(malha, carro.superficie, rgb);
        cor = ColorLerp((Color){ rgb[0], rgb[1], rgb[2], 255 }, WHITE, CLAREIA_A_POEIRA);
    } else if (carro.escorregando > ESCORREGA_QUE_FUMA && rapidez > 6.0f) {
        forca = fminf(0.55f + 0.45f * (carro.escorregando - ESCORREGA_QUE_FUMA) / 0.5f, 1.0f);
        porSegundo = FUMACA_POR_SEGUNDO * forca;
        tipo = NUVEM_DE_DRIFT;
    } else if (carro.acelerando && rapidez < 8.0f && carro.chao == CHAO_ASFALTO) {
        forca = 0.5f * (1.0f - rapidez / 8.0f);
        porSegundo = FUMACA_POR_SEGUNDO * 0.5f;
    }
    if (forca <= 0.0f) {
        return;
    }
    float quantas = porSegundo * tempoQuadro;
    int inteiras = (int) quantas;
    if (GetRandomValue(0, 1000) / 1000.0f < quantas - inteiras) {
        inteiras++;
    }
    for (int i = 0; i < inteiras; i++) {
        for (int lado = -1; lado <= 1; lado += 2) {
            float recuo = -rapidez * tempoQuadro * GetRandomValue(0, 1000) / 1000.0f;
            Vector3 roda = Vector3Transform((Vector3){ lado * meiaBitola, 0.25f, -meioEntreEixos + recuo }, noMundo);
            soltarNuvemColorida(roda, velocidade, forca, tipo, cor);
        }
    }
}

/*
 * atualizarPasseio
 * Um quadro do passeio. ESC (ou START) pausa; M (ou LB) abre o mapa; R (ou Y) põe o carro de
 * volta na rua principal; T leva à pista de treino; C (ou o touchpad) troca a câmera.
 * Recebe: o estado do menu (para a pausa) e o tempo do quadro.
 * Devolve: a próxima tela.
 */
Tela atualizarPasseio(EstadoMenu *menu, float tempoQuadro) {
    if (!pronto) {
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER) || controleApertou(BOTAO_VOLTAR) || controleApertou(BOTAO_CONFIRMAR)) {
            tocarSom(SOM_CONFIRMAR);
            return sair();
        }
        return TELA_PASSEIO;
    }
    if (pausado) {
        EscolhaDaPausa escolha = atualizarPausa(menu);
        if (escolha == PAUSA_CONTINUAR) {
            pausado = 0;
            pausarEfeitosDaCorrida(0);
        } else if (escolha == PAUSA_SAIR) {
            return sair();
        }
        return TELA_PASSEIO;
    }
    if (mapaAberto) {
        /* No mapa o carro espera; Z (RB) aproxima; ENTER (A) leva à pista de treino, R (Y) à cidade */
        if (IsKeyPressed(KEY_Z) || controleApertou(BOTAO_ABA_SEGUINTE)) {
            zoomDoMapa = (zoomDoMapa + 1) % 3;
            tocarSom(SOM_CURSOR);
        }
        if (IsKeyPressed(KEY_M) || IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_TAB) || controleApertou(BOTAO_ABA_ANTERIOR) || controleApertou(BOTAO_VOLTAR)) {
            mapaAberto = 0;
            pausarEfeitosDaCorrida(0);
            tocarSom(SOM_CURSOR);
        } else if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_T) || controleApertou(BOTAO_CONFIRMAR)) {
            mapaAberto = 0;
            pausarEfeitosDaCorrida(0);
            teleportar(treino[0] - 120.0f, treino[2], PI / 2.0f);
        } else if (IsKeyPressed(KEY_R) || controleApertou(BOTAO_LIMPAR)) {
            mapaAberto = 0;
            pausarEfeitosDaCorrida(0);
            teleportar(largada[0], largada[2], largada[3]);
        }
        return TELA_PASSEIO;
    }
    if (IsKeyPressed(KEY_ESCAPE) || controleApertou(BOTAO_START)) {
        tocarSom(SOM_CONFIRMAR);
        pausado = 1;
        abrirPausa(menu);
        pausarEfeitosDaCorrida(1);
        return TELA_PASSEIO;
    }
    if (IsKeyPressed(KEY_M) || IsKeyPressed(KEY_TAB) || controleApertou(BOTAO_ABA_ANTERIOR)) {
        mapaAberto = 1;
        pausarEfeitosDaCorrida(1);
        tocarSom(SOM_CURSOR);
        return TELA_PASSEIO;
    }
    if (IsKeyPressed(KEY_C) || controleApertou(BOTAO_CAMERA)) {
        distanciaEscolhida = (distanciaEscolhida + 1) % 3;
        tocarSom(SOM_CURSOR);
    }
    if (IsKeyPressed(KEY_R) || controleApertou(BOTAO_LIMPAR)) {
        teleportar(largada[0], largada[2], largada[3]);
    }
    if (IsKeyPressed(KEY_T)) {
        teleportar(treino[0] - 120.0f, treino[2], PI / 2.0f);
    }

    Comandos comandos = { 0, 0, 0, 0, 0.0f, 0, 0 };
    lerComandosDoTeclado(&comandos);
    somarComandosDoControle(&comandos);
    int quedasAntes = carro.quedas;
    atualizarCarroLivre(&carro, comandos, malha, tempoQuadro);
    if (carro.quedas != quedasAntes) {
        reiniciarCamera();   /* caiu do mundo e voltou ao começo: a câmera não fica lá embaixo */
    }

    float eixoX = 0.0f, eixoY = 0.0f;
    lerAnalogicoDaCamera(&eixoX, &eixoY);
    atualizarOlhar(&olhar, eixoX, eixoY, tempoQuadro);

    /* Os sons da corrida, lendo o carro da corrida que dirige o carro livre (motor, velocidade e nitro) */
    float naTerra = !carro.noChao ? 0.0f : (carro.chao == CHAO_AREIA) ? 1.0f : (carro.chao == CHAO_TERRA) ? 0.6f : 0.0f;
    float escorrega = (carro.noChao && carro.chao != CHAO_AREIA) ? carro.escorregando : carro.escorregando * 0.3f;
    atualizarEfeitosDoPasseio(&carro.corrida, escorrega, carro.raspando, naTerra, 1, tempoQuadro);
    if (carro.pancada > PANCADA_QUE_SOA) {
        tocarSomDaBatida(carro.pancada, 0);
    }
    carro.pancada = 0.0f;
    return TELA_PASSEIO;
}

/* ------------------------------------------------------------------
 * Desenho
 * ------------------------------------------------------------------ */

/*
 * montarCamera
 * Atrás do carro, para onde o bico aponta visto de cima (com um pequeno atraso), girando com
 * o analógico direito; nunca dentro do chão. Numa parede, ela continua de pé, atrás e um pouco
 * para fora dela.
 */
static Camera3D montarCamera(void) {
    float dt = (pausado || mapaAberto) ? 0.0f : GetFrameTime();
    float passo = fminf(VELOCIDADE_DA_DISTANCIA * dt, 1.0f);
    Distancia alvo = DISTANCIAS[distanciaEscolhida];
    distanciaAtual.atras += (alvo.atras - distanciaAtual.atras) * passo;
    distanciaAtual.altura += (alvo.altura - distanciaAtual.altura) * passo;

    if (dt > 0.0f) {
        float diferenca = carro.rumo - rumoDaCamera;
        diferenca -= 2.0f * PI * roundf(diferenca / (2.0f * PI));
        rumoDaCamera += diferenca * (1.0f - expf(-dt / ATRASO_DA_CAMERA));
        alturaDaCamera += (carro.y - alturaDaCamera) * (1.0f - expf(-dt / ATRASO_DA_ALTURA));
    }
    float giro = rumoDaCamera + olhar.giro * DEG2RAD;
    float dx = sinf(giro), dz = -cosf(giro);

    Camera3D camera = { 0 };
    camera.position = (Vector3){ carro.x - dx * distanciaAtual.atras, alturaDaCamera + distanciaAtual.altura,
                                 carro.z - dz * distanciaAtual.atras };
    float chao = alturaDoChaoNaMalha(malha, camera.position.x, camera.position.z, camera.position.y + 30.0f, NULL);
    if (chao > SEM_CHAO && camera.position.y < chao + FOLGA_DA_CAMERA) {
        camera.position.y = chao + FOLGA_DA_CAMERA;
    }
    float ladeira = (carro.y + 1.0f - camera.position.y) / distanciaAtual.atras + distanciaAtual.altura / distanciaAtual.atras * 0.55f;
    camera.target = (Vector3){ camera.position.x + dx,
                               camera.position.y + ladeira + tanf((olhar.elevacao - OLHAR_PARA_BAIXO) * DEG2RAD),
                               camera.position.z + dz };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };

    /* Na parede, a câmera deita junto com o carro: fica atrás e "acima" dele em relação à parede, com o
     * "para cima" dela no do carro. Assim, na tela, a parede vira chão e esquerda/direita no volante é
     * esquerda/direita na tela (como nos jogos de corrida com wallride) */
    Vector3 cimaDoCarro = { carro.cima[0], carro.cima[1], carro.cima[2] };
    Vector3 frenteDoCarro = { carro.frente[0], carro.frente[1], carro.frente[2] };
    if (dt > 0.0f) {
        float segue = fminf(CAMERA_SEGUE_A_PAREDE * dt, 1.0f);
        cimaDaCamera = Vector3Normalize(Vector3Lerp(cimaDaCamera, cimaDoCarro, segue));
        frenteDaCamera = Vector3Lerp(frenteDaCamera, frenteDoCarro, segue);
        float alvoDaParede = carro.naParede ? 1.0f : 0.0f;
        noWallride += (alvoDaParede - noWallride) * fminf(dt * 4.0f, 1.0f);
    }
    frenteDaCamera = Vector3Subtract(frenteDaCamera, Vector3Scale(cimaDaCamera, Vector3DotProduct(frenteDaCamera, cimaDaCamera)));
    if (Vector3Length(frenteDaCamera) < 1e-3f) {
        frenteDaCamera = frenteDoCarro;
    }
    frenteDaCamera = Vector3Normalize(frenteDaCamera);
    if (noWallride > 0.001f) {
        Vector3 frente = Vector3RotateByAxisAngle(frenteDaCamera, cimaDaCamera, -olhar.giro * DEG2RAD);   /* o analógico direito ainda gira */
        Vector3 carroNoMundo = { carro.x, carro.y, carro.z };
        Vector3 deParede = Vector3Add(Vector3Subtract(carroNoMundo, Vector3Scale(frente, distanciaAtual.atras)),
                                      Vector3Scale(cimaDaCamera, distanciaAtual.altura));
        Vector3 alvoDeParede = Vector3Add(Vector3Add(carroNoMundo, Vector3Scale(frente, 5.0f)), Vector3Scale(cimaDaCamera, 1.0f));
        Vector3 olhaNormal = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
        Vector3 olhaParede = Vector3Normalize(Vector3Subtract(alvoDeParede, deParede));
        camera.position = Vector3Lerp(camera.position, deParede, noWallride);
        camera.target = Vector3Add(camera.position, Vector3Normalize(Vector3Lerp(olhaNormal, olhaParede, noWallride)));
        camera.up = Vector3Normalize(Vector3Lerp(camera.up, cimaDaCamera, noWallride));
    }
    camera.fovy = GRAUS_DE_VISAO;
    camera.projection = CAMERA_PERSPECTIVE;
    return camera;
}

/*
 * matrizDoCarro
 * O carro de pé no chão dele (o "para cima" vem da física, suavizado para o desenho não
 * tremer nas emendas dos triângulos), com o bico para a frente.
 */
static Matrix matrizDoCarro(void) {
    float dt = (pausado || mapaAberto) ? 0.0f : GetFrameTime();
    Vector3 cimaFisica = { carro.cima[0], carro.cima[1], carro.cima[2] };
    cimaDoDesenho = Vector3Normalize(Vector3Lerp(cimaDoDesenho, cimaFisica, fminf(CIMA_SEGUE * dt, 1.0f)));
    if (Vector3DotProduct(cimaDoDesenho, cimaFisica) < 0.3f) {
        cimaDoDesenho = cimaFisica;   /* mudou de uma vez: sem girar no ar */
    }
    Vector3 frente = { carro.frente[0], carro.frente[1], carro.frente[2] };
    frente = Vector3Normalize(Vector3Subtract(frente, Vector3Scale(cimaDoDesenho, Vector3DotProduct(frente, cimaDoDesenho))));
    /* Como na corrida: o bico aponta para o caminho mais o ângulo do drift e um pouco do volante */
    float bico = carro.corrida.anguloDeDrift + carro.volante * GRAUS_DO_VOLANTE;
    frente = Vector3RotateByAxisAngle(frente, cimaDoDesenho, -bico * DEG2RAD);
    Vector3 lado = Vector3Normalize(Vector3CrossProduct(cimaDoDesenho, frente));   /* a esquerda do carro é +x no modelo */
    Vector3 cima = Vector3CrossProduct(frente, lado);
    Matrix m = { 0 };
    m.m0 = lado.x;   m.m1 = lado.y;   m.m2 = lado.z;
    m.m4 = cima.x;   m.m5 = cima.y;   m.m6 = cima.z;
    m.m8 = frente.x; m.m9 = frente.y; m.m10 = frente.z;
    m.m12 = carro.x; m.m13 = carro.y; m.m14 = carro.z;
    m.m15 = 1.0f;
    return m;
}

/*
 * desenharSombraDoCarro
 * Uma grade de pontos no chão em volta do carro (como na corrida), procurados ao longo do
 * "para baixo" dele (numa parede, a sombra fica na parede), e puxados um pouco para a câmera.
 */
static void desenharSombraDoCarro(Camera3D camera, Matrix noMundo) {
    Vector3 grade[PONTOS_DA_SOMBRA * PONTOS_DA_SOMBRA];
    float alturas[PONTOS_DA_SOMBRA * PONTOS_DA_SOMBRA];
    float meio = ladoDaSombra(numero) / 2.0f;
    Vector3 cima = { noMundo.m4, noMundo.m5, noMundo.m6 };
    float baixo[3] = { -cima.x, -cima.y, -cima.z };
    for (int i = 0; i < PONTOS_DA_SOMBRA; i++) {
        for (int j = 0; j < PONTOS_DA_SOMBRA; j++) {
            float paraEsquerda = meio - 2.0f * meio * i / (PONTOS_DA_SOMBRA - 1);
            float paraFrente = meio - 2.0f * meio * j / (PONTOS_DA_SOMBRA - 1);
            Vector3 ponto = Vector3Transform((Vector3){ paraEsquerda, 1.5f, paraFrente }, noMundo);
            float origem[3] = { ponto.x, ponto.y, ponto.z };
            Acerto acerto;
            if (!raioNaMalha(malha, origem, baixo, ALCANCE_DA_SOMBRA, &acerto)) {
                return;   /* longe do chão (no ar): sem sombra, em vez de uma sombra grudada no carro */
            }
            ponto = (Vector3){ acerto.ponto[0], acerto.ponto[1], acerto.ponto[2] };
            alturas[i * PONTOS_DA_SOMBRA + j] = acerto.distancia;
            ponto = Vector3Add(ponto, Vector3Scale(cima, ALTURA_DA_SOMBRA));
            Vector3 paraCamera = Vector3Normalize(Vector3Subtract(camera.position, ponto));
            grade[i * PONTOS_DA_SOMBRA + j] = Vector3Add(ponto, Vector3Scale(paraCamera, SOMBRA_PERTO_DA_CAMERA));
        }
    }
    /* Na beira de um degrau, os pontos de um lado caem no chão de baixo e a sombra vira uma cortina: sem sombra */
    for (int i = 0; i < PONTOS_DA_SOMBRA; i++) {
        for (int j = 0; j < PONTOS_DA_SOMBRA; j++) {
            float a = alturas[i * PONTOS_DA_SOMBRA + j];
            if ((i + 1 < PONTOS_DA_SOMBRA && fabsf(a - alturas[(i + 1) * PONTOS_DA_SOMBRA + j]) > DEGRAU_DA_SOMBRA) ||
                (j + 1 < PONTOS_DA_SOMBRA && fabsf(a - alturas[i * PONTOS_DA_SOMBRA + j + 1]) > DEGRAU_DA_SOMBRA)) {
                return;
            }
        }
    }
    desenharSombraNoChao(numero, grade);
}

/*
 * desenharCarro
 * O carro do jogador, com o corpo deitando para fora nas curvas e a cara de quem está correndo.
 */
static void desenharCarro(Matrix noMundo) {
    /* O corpo deita e o bico abaixa com a suspensão da corrida */
    inclinarCorpoNoProximoDesenho(numero, carro.corrida.rolagem * DEG2RAD, -carro.corrida.arfagem * DEG2RAD);
    float rapidez = fabsf(velocidadeParaFrente(&carro)) / carro.velocidadeMaxima;
    float aperto = (rapidez - 0.7f) / 0.3f;
    if ((carro.nitroLigado && rapidez > 0.4f) || carro.naParede) {
        aperto = 1.0f;
    }
    expressaoNoProximoDesenho(numero, aperto, (carro.raspando || (!carro.noChao && carro.segundosNoAr > 0.6f)) ? 0.7f : 0.0f);
    desenharCarro3D(numero, noMundo, carro.anguloDasRodas, carro.volante);
}

/* A chama azul do nitro, nos dois escapamentos */
static void desenharChamas(Camera3D camera, Matrix noMundo) {
    if (!carro.nitroLigado) {
        return;
    }
    Vector3 paraTras = { -noMundo.m8, -noMundo.m9, -noMundo.m10 };
    float atras = comprimentoDoCarro3D(numero) / 2.0f;
    float meiaDistancia = larguraDoCarro3D(numero) * LUGAR_DO_ESCAPAMENTO;
    comecarChamas(camera);
    for (int lado = -1; lado <= 1; lado += 2) {
        Vector3 escape = Vector3Transform((Vector3){ lado * meiaDistancia, ALTURA_DO_ESCAPAMENTO, -atras }, noMundo);
        desenharChamaDoNitro(escape, paraTras);
    }
    terminarChamas();
}

/* Onde um ponto do mundo cai na foto do mapa (em pixels) */
static Vector2 naFoto(float x, float z) {
    return (Vector2){ (x - mapaX0) / metrosPorPixel, (z - mapaZ0) / metrosPorPixel };
}

/* Uma seta (o carro), com a ponta para o ângulo dado (0 = para cima, crescendo no sentido do relógio) */
static void desenharSeta(Vector2 centro, float angulo, float tamanho, Color cor) {
    Vector2 ponta = { centro.x + sinf(angulo) * tamanho, centro.y - cosf(angulo) * tamanho };
    Vector2 esquerda = { centro.x + sinf(angulo - 2.5f) * tamanho * 0.8f, centro.y - cosf(angulo - 2.5f) * tamanho * 0.8f };
    Vector2 direita = { centro.x + sinf(angulo + 2.5f) * tamanho * 0.8f, centro.y - cosf(angulo + 2.5f) * tamanho * 0.8f };
    Vector2 base = { centro.x - sinf(angulo) * tamanho * 0.35f, centro.y + cosf(angulo) * tamanho * 0.35f };
    /* Os dois sentidos: a raylib só desenha o triângulo que vem no sentido certo */
    DrawTriangle(ponta, esquerda, base, cor);
    DrawTriangle(ponta, base, esquerda, cor);
    DrawTriangle(ponta, base, direita, cor);
    DrawTriangle(ponta, direita, base, cor);
}

/*
 * atualizarMinimapa
 * O pedaço do mapa em volta do carro, girado para a câmera apontar para cima, com a pista de
 * treino e a cidade marcadas. Desenhado numa imagem própria (antes da cena), que o HUD põe
 * no canto.
 */
static void atualizarMinimapa(void) {
    float pixelsPorMetro = (LADO_DO_MINIMAPA / 2.0f) / RAIO_DO_MINIMAPA;
    float escala = pixelsPorMetro * metrosPorPixel;
    Vector2 carroNaFoto = naFoto(carro.x, carro.z);
    float meio = LADO_DO_MINIMAPA / 2.0f;
    BeginTextureMode(minimapa);
    ClearBackground(COR_DO_FUNDO_DO_MAPA);
    DrawTexturePro(fotoDoMapa, (Rectangle){ 0, 0, (float) fotoDoMapa.width, (float) fotoDoMapa.height },
                   (Rectangle){ meio, meio, fotoDoMapa.width * escala, fotoDoMapa.height * escala },
                   (Vector2){ carroNaFoto.x * escala, carroNaFoto.y * escala }, -rumoDaCamera * RAD2DEG, WHITE);
    /* A pista de treino e a cidade, presas à borda quando estão longe */
    float pontos[2][2] = { { treino[0], treino[2] }, { largada[0], largada[2] } };
    Color cores[2] = { COR_AMARELO, WHITE };
    for (int k = 0; k < 2; k++) {
        float dx = (pontos[k][0] - carro.x) * pixelsPorMetro, dz = (pontos[k][1] - carro.z) * pixelsPorMetro;
        float c = cosf(-rumoDaCamera), s = sinf(-rumoDaCamera);
        Vector2 p = { dx * c - dz * s, dx * s + dz * c };
        float distancia = Vector2Length(p);
        if (distancia > meio - 12.0f) {
            p = Vector2Scale(p, (meio - 12.0f) / distancia);
        }
        DrawCircleV((Vector2){ meio + p.x, meio + p.y }, 9.0f, BLACK);
        DrawCircleV((Vector2){ meio + p.x, meio + p.y }, 7.0f, cores[k]);
    }
    desenharSeta((Vector2){ meio, meio }, carro.rumo - rumoDaCamera, 13.0f, BLACK);
    desenharSeta((Vector2){ meio, meio }, carro.rumo - rumoDaCamera, 10.0f, (Color){ 230, 40, 40, 255 });
    EndTextureMode();
}

/*
 * desenharMapaGrande
 * O mapa inteiro, com o carro, a cidade e a pista de treino.
 */
static void desenharMapaGrande(void) {
    float largura = (float) larguraVirtual(), altura = (float) alturaVirtual();
    DrawRectangle(-200, -200, (int) largura + 400, (int) altura + 400, Fade(BLACK, 0.7f));
    float lado = altura - 120.0f;
    Rectangle tela = { (largura - lado) / 2.0f, 60.0f, lado, lado };
    caixaArredondada((Rectangle){ tela.x - 6, tela.y - 6, tela.width + 12, tela.height + 12 }, 10.0f, COR_PAINEL);
    /* O pedaço da foto que aparece: tudo, ou (aproximando) um quadrado em volta do carro */
    float pedaco = fotoDoMapa.width / ZOOMS_DO_MAPA[zoomDoMapa];
    Vector2 p0 = naFoto(carro.x, carro.z);
    Rectangle origem = { Clamp(p0.x - pedaco / 2.0f, 0.0f, fotoDoMapa.width - pedaco), Clamp(p0.y - pedaco / 2.0f, 0.0f, fotoDoMapa.height - pedaco),
                         pedaco, pedaco };
    DrawTexturePro(fotoDoMapa, origem, tela, (Vector2){ 0 }, 0.0f, WHITE);
    float escala = lado / pedaco;
    Vector2 t = naFoto(treino[0], treino[2]), c = naFoto(largada[0], largada[2]), p = naFoto(carro.x, carro.z);
    Vector2 naTela[3] = { { tela.x + (t.x - origem.x) * escala, tela.y + (t.y - origem.y) * escala },
                          { tela.x + (c.x - origem.x) * escala, tela.y + (c.y - origem.y) * escala },
                          { tela.x + (p.x - origem.x) * escala, tela.y + (p.y - origem.y) * escala } };
    /* Com zoom, um marcador pode ficar fora do pedaço que aparece: ele não é desenhado */
    if (CheckCollisionPointRec(naTela[0], tela)) {
        DrawCircleV(naTela[0], 8.0f, BLACK);
        DrawCircleV(naTela[0], 6.0f, COR_AMARELO);
        escrever(FONTE_FORTE, "PISTA DE TREINO", naTela[0].x + 11.0f, naTela[0].y - 2.0f, 14.0f, COR_AMARELO);
    }
    if (CheckCollisionPointRec(naTela[1], tela)) {
        DrawCircleV(naTela[1], 8.0f, BLACK);
        DrawCircleV(naTela[1], 6.0f, WHITE);
        escrever(FONTE_FORTE, "RADIATOR SPRINGS", naTela[1].x + 11.0f, naTela[1].y - 16.0f, 14.0f, WHITE);
    }
    if (CheckCollisionPointRec(naTela[2], tela)) {
        desenharSeta(naTela[2], carro.rumo, 12.0f, BLACK);
        desenharSeta(naTela[2], carro.rumo, 9.0f, (Color){ 230, 40, 40, 255 });
    }

    escreverCentralizado(FONTE_TITULO, "MAPA", largura / 2.0f, 18.0f, 30.0f, WHITE);
    const char *dica = controleConectado() ? "RB: zoom     A: pista de treino     Y: cidade     LB: fecha"
                                           : "Z: zoom     ENTER: pista de treino     R: cidade     M: fecha";
    escreverCentralizado(FONTE_FORTE, dica, largura / 2.0f, altura - 46.0f, 16.0f, COR_TEXTO_CLARO);
}

/*
 * desenharHudDoPasseio
 * Velocidade, marcha e nitro embaixo à direita; o minimapa embaixo à esquerda; a seta para a
 * pista de treino no alto. (O wallride não tem aviso: a câmera deitando com o carro já diz tudo.)
 */
static void desenharHudDoPasseio(Camera3D camera) {
    float largura = (float) larguraVirtual(), altura = (float) alturaVirtual();

    /* Velocidade e marcha */
    int kmh = converterParaKmh(velocidadeDoCarroLivre(&carro) * UNIDADES_POR_METRO);   /* o mesmo velocímetro da corrida */
    Rectangle painel = { largura - 196.0f, altura - 112.0f, 176.0f, 92.0f };
    caixaArredondada(painel, 10.0f, COR_PAINEL);
    escreverADireita(FONTE_TITULO, TextFormat("%d", kmh), painel.x + 118.0f, painel.y + 6.0f, 52.0f, WHITE);
    escrever(FONTE_FORTE, "KM/H", painel.x + 124.0f, painel.y + 30.0f, 15.0f, COR_TEXTO_FRACO);
    const char *marcha = (velocidadeParaFrente(&carro) < -0.5f) ? "R" : TextFormat("%d", carro.motor.marcha + 1);
    escrever(FONTE_TITULO, marcha, painel.x + 136.0f, painel.y + 46.0f, 24.0f, COR_AMARELO);
    desenharBarraDeBlocos(painel.x + 14.0f, painel.y + 70.0f, 110.0f, 10.0f, carro.nitro, (Color){ 60, 160, 255, 255 });
    escrever(FONTE_FORTE, "NITRO", painel.x + 130.0f, painel.y + 67.0f, 13.0f, COR_TEXTO_FRACO);
    if (carro.noChao && NOMES_DOS_CHAOS[carro.chao][0] != '\0') {
        escrever(FONTE_FORTE, NOMES_DOS_CHAOS[carro.chao], painel.x + 14.0f, painel.y + 50.0f, 13.0f, COR_TEXTO_FRACO);
    }

    /* O minimapa */
    Rectangle mini = { 20.0f, altura - TAMANHO_DO_MINIMAPA - 20.0f, TAMANHO_DO_MINIMAPA, TAMANHO_DO_MINIMAPA };
    caixaArredondada((Rectangle){ mini.x - 4, mini.y - 4, mini.width + 8, mini.height + 8 }, 10.0f, COR_PAINEL);
    DrawTexturePro(minimapa.texture, (Rectangle){ 0, 0, LADO_DO_MINIMAPA, -LADO_DO_MINIMAPA }, mini, (Vector2){ 0 }, 0.0f, WHITE);
    caixaArredondada((Rectangle){ mini.x + 4, mini.y + 4, 62.0f, 20.0f }, 6.0f, COR_PAINEL);
    escrever(FONTE_FORTE, controleConectado() ? "LB  MAPA" : "M  MAPA", mini.x + 10.0f, mini.y + 6.0f, 13.0f, WHITE);

    /* O nome do modo e as teclas, em cima à esquerda */
    caixaArredondada((Rectangle){ 20.0f, 20.0f, 300.0f, 58.0f }, 10.0f, COR_PAINEL);
    escrever(FONTE_TITULO, "PASSEIO LIVRE", 34.0f, 24.0f, 24.0f, WHITE);
    escrever(FONTE_TEXTO, controleConectado() ? "Y cidade   LB mapa   START pausa" : "R cidade   T treino   M mapa   C câmera   ESC pausa",
             34.0f, 52.0f, 15.0f, COR_TEXTO_FRACO);

    /* A seta para a pista de treino, girando com a câmera */
    float paraX = treino[0] - carro.x, paraZ = treino[2] - carro.z;
    float distancia = sqrtf(paraX * paraX + paraZ * paraZ);
    if (distancia > 220.0f) {
        Vector3 olha = Vector3Subtract(camera.target, camera.position);
        float angulo = atan2f(paraX, -paraZ) - atan2f(olha.x, -olha.z);
        float x = largura / 2.0f;
        caixaArredondada((Rectangle){ x - 104.0f, 20.0f, 208.0f, 48.0f }, 10.0f, COR_PAINEL);
        desenharSeta((Vector2){ x - 76.0f, 44.0f }, angulo, 14.0f, COR_AMARELO);
        escrever(FONTE_FORTE, "PISTA DE TREINO", x - 52.0f, 24.0f, 14.0f, COR_TEXTO_FRACO);
        escrever(FONTE_TITULO, distancia >= 1000.0f ? TextFormat("%.1f km", distancia / 1000.0f) : TextFormat("%d m", (int) distancia),
                 x - 52.0f, 38.0f, 22.0f, WHITE);
    }
}

/*
 * desenharPasseio
 * A cena (com o sol, as sombras e o tom de cor da corrida), os efeitos, o HUD e, se for o
 * caso, o mapa grande ou a pausa.
 * Recebe: o estado do menu (para a pausa).
 * Devolve: nada.
 */
void desenharPasseio(EstadoMenu *menu) {
    if (!pronto) {
        ClearBackground(BLACK);
        comecarInterface(1);
        escreverCentralizado(FONTE_TITULO, "MAPA DO PASSEIO NÃO ENCONTRADO", LARGURA_TELA / 2.0f, 250.0f, 30.0f, WHITE);
        escreverCentralizado(FONTE_TEXTO, "Gere recursos/modelos/radiator_springs.glb e .col com ferramentas/montar_mundo_aberto.py",
                             LARGURA_TELA / 2.0f, 296.0f, 16.0f, COR_TEXTO_FRACO);
        escreverCentralizado(FONTE_FORTE, "ENTER volta ao menu", LARGURA_TELA / 2.0f, 340.0f, 16.0f, COR_AMARELO);
        terminarInterface();
        return;
    }
    float dt = (pausado || mapaAberto) ? 0.0f : GetFrameTime();
    Camera3D camera = montarCamera();
    Matrix noMundo = matrizDoCarro();
    soltarPoeiraEFumaca(noMundo, dt);
    atualizarFumaca(dt);
    atualizarMarcas(dt);
    atualizarMinimapa();

    prepararLuzes(0);
    prepararSombras(CENARIO_DO_MUNDO_ABERTO, camera, configuracoesAtuais()->sombras);
    definirOlhoDosCarros(camera.position);
    comecarPosProcesso();
    prepararPlanosDaCamera();
    BeginMode3D(camera);
    rlDisableBackfaceCulling();
    desenharCenario(CENARIO_DO_MUNDO_ABERTO, camera.position);
    desenharMarcas();
    comecarSombrasNoChao();
    desenharSombraDoCarro(camera, noMundo);
    terminarSombrasNoChao();
    desenharCarro(noMundo);
    desenharFumaca3D(camera);
    desenharChamas(camera, noMundo);
    rlEnableBackfaceCulling();
    EndMode3D();
    int efeitos = configuracoesAtuais()->efeitos;
    if (efeitos) {
        /* As luzes de novo, sozinhas: o halo nasce da chama do nitro */
        comecarLuzesDoPosProcesso();
        prepararPlanosDaCamera();
        BeginMode3D(camera);
        desenharChamas(camera, noMundo);
        EndMode3D();
    }
    float rapidez = Clamp(velocidadeDoCarroLivre(&carro) / carro.velocidadeMaxima, 0.0f, 1.2f);
    float desfoque = efeitos ? 0.35f * rapidez * rapidez + (carro.nitroLigado ? 0.25f : 0.0f) : 0.0f;
    terminarPosProcesso(PISTA_OVAL, desfoque, efeitos);

    comecarInterface(0);
    if (mapaAberto) {
        desenharMapaGrande();
    } else {
        desenharHudDoPasseio(camera);
    }
    terminarInterface();
    if (pausado) {
        comecarInterface(1);
        desenharPausa(menu);
        terminarInterface();
    }
}
