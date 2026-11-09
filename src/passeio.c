/*
 * passeio.c
 * ------------------------------------------------------------------
 * O mundo aberto, onde o jogo acontece. Veja passeio.h.
 *
 * O mapa (radiator_springs.glb) e a colisão (radiator_springs.col) são
 * carregados só na primeira vez que o jogador entra aqui. Os dois são
 * gerados por ferramentas/montar_mundo_aberto.py. Na mesma hora, o mapa
 * é fotografado de cima uma vez: a foto vira o minimapa e o mapa grande.
 * O Mack (mack.glb, de ferramentas/preparar_mack.py) fica estacionado
 * na rua principal, logo à frente da largada.
 * ------------------------------------------------------------------
 */

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"
#include "passeio.h"
#include "livre.h"
#include "carreta.h"
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
#define RAIO_DO_MINIMAPA      320.0f    /* metros do carro até a borda do minimapa */
#define RAIO_DO_MINIMAPA_NA_TELA 58.0f  /* o minimapa é redondo: o raio dele na tela virtual */
#define COR_DO_FUNDO_DO_MAPA  (Color){ 196, 128, 84, 255 }
#define COR_DO_MACK           (Color){ 255, 120, 40, 255 }

typedef struct {
    float atras, altura;
} Distancia;

static const Distancia DISTANCIAS[3] = { { 5.2f, 2.1f }, { 8.5f, 3.4f }, { 13.0f, 5.0f } };

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

/* O Mack, estacionado à direita na rua principal, um pouco à frente da largada. Na vaga atrás da
 * carreta, o jogador aperta para entrar: a porta de trás desce e vira rampa, o carro sobe e entra, a
 * porta sobe, e aí vem a escolha da corrida (TELA_PISTAS). Escolhida, o Mack parte pela rua (e depois
 * vem a apresentação da pista); desistindo, o carro desce de ré pela rampa. Na volta de uma
 * corrida, ele desce do mesmo jeito */
#define ARQUIVO_DO_MACK       "recursos/modelos/mack.glb"   /* feito por ferramentas/preparar_mack.py */
#define ARQUIVO_DAS_RODAS_DO_MACK "recursos/modelos/mack_rodas.glb"   /* uma malha por roda */
#define ARQUIVO_DA_PORTA_DO_MACK  "recursos/modelos/mack_porta.glb"   /* a porta de trás, que vira rampa */
#define MAIS_RODAS            16
#define MACK_A_FRENTE         60.0f     /* metros da largada até o meio do Mack, no rumo dela */
#define MACK_AO_LADO          3.6f      /* metros para a direita: encostado no meio-fio (a rua tem uns 6 m de cada lado) */
#define VAGA_ATE              20.0f     /* metros atrás da traseira da carreta */
#define VAGA_DE_LADO          3.5f      /* metros para cada lado do meio dela */
/* A entrada (segundos desde o aperto) */
#define ALINHADO              1.3f      /* o carro chega ao pé da rampa, alinhado com a carreta */
#define PORTA_DESCE_DE        1.2f
#define PORTA_MEXE            1.2f      /* segundos para a porta descer (ou subir) */
#define SOBE_DE               2.5f      /* o carro sobe a rampa... */
#define DENTRO                4.1f      /* ...e está todo dentro da carreta */
#define PORTA_SOBE_DE         4.2f
#define SEGUNDOS_ENTRANDO     5.8f
/* A saída (desistindo da escolha, ou na volta da corrida) */
#define PORTA_DESCE_NA_SAIDA  0.3f
#define DESCE_DE              1.6f
#define FORA                  3.4f
#define PORTA_SOBE_NA_SAIDA   3.5f
#define SEGUNDOS_SAINDO       4.8f
#define SEGUNDOS_PARTINDO     5.0f      /* o Mack sai pela rua principal */
#define SEGUNDOS_DO_RECADO    6.0f      /* o resultado da corrida, na volta à cidade */
#define ESCURECE              0.45f     /* segundos de escurecer e de clarear, nas cenas do Mack */
#define ACELERACAO_DO_MACK    3.0f      /* m/s², partindo */
#define LONGE_DA_RAMPA        3.0f      /* metros entre o pé da rampa e o carro, depois de descer */
#define FUNDO_DA_CARRETA      1.5f      /* metros entre a porta e a traseira do carro, lá dentro */
#define FUNDO_DO_ESCURO       12.0f     /* metros de carreta escura atrás da porta aberta */
#define FAIXA_DE_CINEMA       0.11f     /* da altura da tela: as faixas pretas das cenas */
/* Os olhos do Mack (os dos carros, carros3d.c): acompanham o carro do jogador */
#define OLHA_ATE              70.0f     /* metros: mais longe que isso, ele olha à frente (e em volta) */
#define CANTO_DO_OLHO         55.0f     /* graus para o lado em que a íris chega ao canto */
#define SEGUNDOS_DE_SUSTO     0.7f      /* bateram nele */

typedef enum { CENA_DIRIGINDO, CENA_ENTRANDO, CENA_NA_CARRETA, CENA_PARTINDO, CENA_SAINDO } CenaDoMack;

static Model mack, rodasDoMack, modeloDaPorta;
static int temMack = 0, temRodasDoMack = 0, temPortaDoMack = 0, temOlhosDoMack = 0;
static Vector3 eixoDaRoda[MAIS_RODAS];   /* o meio de cada roda, no modelo (o eixo é o x dele) */
static float raioDaRodaDoMack[MAIS_RODAS];
static Matrix matrizesDoMack[MAIS_RODAS], matrizesDasRodas[MAIS_RODAS], matrizesDaPorta[MAIS_RODAS];   /* lidas no desenho */
static float mackX = 0.0f, mackY = 0.0f, mackZ = 0.0f, mackRumo = 0.0f, mackInclinacao = 0.0f;
static float meioDoMack = 15.3f, meiaLarguraDoMack = 2.25f, alturaDoMack = 7.6f;   /* medidos do modelo */
static Vector3 dobradica = { 0.0f, 1.15f, -14.22f };   /* o pé da porta, no modelo (medido do mack_porta.glb) */
static float alturaDaPorta = 5.8f, larguraDaPorta = 3.9f;
static float aberturaDaPorta = 0.0f;   /* 0 fechada, 1 encostada no chão (a rampa) */
static float mackAndou = 0.0f;         /* metros, partindo */
static float sustoDoMack = 0.0f;       /* segundos que ainda faltam do susto */
static CenaDoMack cena = CENA_DIRIGINDO;
static float tempoDaCena = 0.0f;
static float embarqueX = 0.0f, embarqueZ = 0.0f, embarqueRumo = 0.0f;   /* onde o carro estava quando apertou */
static int saindoDaCorrida = 0;      /* a saída é a volta de uma corrida (o letreiro e o recado) */
static char destino[64] = "";        /* o nome da pista, para a cena da partida */
static char recado[96] = "";         /* o resultado da corrida, na volta */
static float tempoDoRecado = 0.0f;
static int correrDireto = 0;         /* sem o mapa: a escolha leva direto à corrida */

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

/* ------------------------------------------------------------------
 * O Mack
 * ------------------------------------------------------------------ */

/* A frente e a direita do Mack, vistas de cima */
static void eixosDoMack(float *frenteX, float *frenteZ, float *direitaX, float *direitaZ) {
    *frenteX = sinf(mackRumo);
    *frenteZ = -cosf(mackRumo);
    *direitaX = cosf(mackRumo);
    *direitaZ = sinf(mackRumo);
}

/* A altura do chão sob o Mack e a inclinação dele, com o meio em (x, z) */
static void chaoDoMack(float x, float z, float *y, float *inclinacao) {
    float fx, fz, dx, dz;
    eixosDoMack(&fx, &fz, &dx, &dz);
    float teto = largada[1] + 30.0f;
    float eixo = meioDoMack * 0.7f;
    float naFrente = alturaDoChaoNaMalha(malha, x + fx * eixo, z + fz * eixo, teto, NULL);
    float atras = alturaDoChaoNaMalha(malha, x - fx * eixo, z - fz * eixo, teto, NULL);
    if (naFrente <= SEM_CHAO || atras <= SEM_CHAO) {
        return;   /* fica a de antes */
    }
    *y = (naFrente + atras) / 2.0f;
    *inclinacao = atan2f(naFrente - atras, 2.0f * eixo);
}

/*
 * carregarMack
 * O modelo (com a luz e a sombra do mapa), as medidas dele e a vaga na rua principal.
 * Sem o arquivo, a vaga continua lá: as corridas não somem por falta do desenho.
 */
static void carregarMack(void) {
    if (FileExists(ARQUIVO_DO_MACK)) {
        mack = LoadModel(ARQUIVO_DO_MACK);
        temMack = mack.meshCount > 0;
    } else {
        TraceLog(LOG_WARNING, "PASSEIO: falta %s (gere com ferramentas/preparar_mack.py)", ARQUIVO_DO_MACK);
    }
    if (temMack) {
        prepararModeloDoMundoAberto(&mack);
        BoundingBox caixa = GetModelBoundingBox(mack);
        meioDoMack = (caixa.max.z - caixa.min.z) / 2.0f;
        meiaLarguraDoMack = (caixa.max.x - caixa.min.x) / 2.0f;
        alturaDoMack = caixa.max.y;   /* as rodas, à parte, ficam em y = 0 */
        temMack = mack.meshCount <= MAIS_RODAS;
        temOlhosDoMack = temMack && darOlhosAoModelo(&mack, "mack");   /* os olhos vivos dos carros */
    }
    /* A porta de trás: gira em volta do pé dela (a dobradiça), no eixo x do modelo */
    if (temMack && FileExists(ARQUIVO_DA_PORTA_DO_MACK)) {
        modeloDaPorta = LoadModel(ARQUIVO_DA_PORTA_DO_MACK);
        temPortaDoMack = modeloDaPorta.meshCount > 0 && modeloDaPorta.meshCount <= MAIS_RODAS;
        if (temPortaDoMack) {
            prepararModeloDoMundoAberto(&modeloDaPorta);
            BoundingBox porta = GetModelBoundingBox(modeloDaPorta);
            dobradica = (Vector3){ (porta.min.x + porta.max.x) / 2.0f, porta.min.y, (porta.min.z + porta.max.z) / 2.0f };
            alturaDaPorta = porta.max.y - porta.min.y;
            larguraDaPorta = porta.max.x - porta.min.x;
        }
    }
    /* As rodas: cada malha gira em volta do meio dela, no eixo x do modelo (pneu, disco e cubo são simétricos) */
    if (temMack && FileExists(ARQUIVO_DAS_RODAS_DO_MACK)) {
        rodasDoMack = LoadModel(ARQUIVO_DAS_RODAS_DO_MACK);
        temRodasDoMack = rodasDoMack.meshCount > 0 && rodasDoMack.meshCount <= MAIS_RODAS;
        if (temRodasDoMack) {
            prepararModeloDoMundoAberto(&rodasDoMack);
            for (int i = 0; i < rodasDoMack.meshCount; i++) {
                BoundingBox roda = GetMeshBoundingBox(rodasDoMack.meshes[i]);
                eixoDaRoda[i] = Vector3Scale(Vector3Add(roda.min, roda.max), 0.5f);
                raioDaRodaDoMack[i] = fmaxf((roda.max.y - roda.min.y) / 2.0f, 0.1f);
            }
        }
    }
    if (temPortaDoMack) {
        carregarInteriorDaCarreta();   /* o que aparece quando a porta abre */
    }
    mackRumo = largada[3];
    float fx, fz, dx, dz;
    eixosDoMack(&fx, &fz, &dx, &dz);
    mackX = largada[0] + fx * MACK_A_FRENTE + dx * MACK_AO_LADO;
    mackZ = largada[2] + fz * MACK_A_FRENTE + dz * MACK_AO_LADO;
    mackY = largada[1];
    chaoDoMack(mackX, mackZ, &mackY, &mackInclinacao);
}

/* O Mack no mundo: parado, ou andando pela rua (partindo) */
static Matrix matrizDoMack(void) {
    float fx, fz, dx, dz;
    eixosDoMack(&fx, &fz, &dx, &dz);
    float x = mackX + fx * mackAndou, z = mackZ + fz * mackAndou;
    float y = mackY, inclinacao = mackInclinacao;
    if (mackAndou > 0.0f) {
        chaoDoMack(x, z, &y, &inclinacao);
    }
    Vector3 frente = { fx * cosf(inclinacao), sinf(inclinacao), fz * cosf(inclinacao) };
    Vector3 esquerda = Vector3Normalize((Vector3){ frente.z, 0.0f, -frente.x });   /* +x do modelo */
    Vector3 cima = Vector3CrossProduct(frente, esquerda);
    Matrix m = { 0 };
    m.m0 = esquerda.x; m.m1 = esquerda.y; m.m2 = esquerda.z;
    m.m4 = cima.x;     m.m5 = cima.y;     m.m6 = cima.z;
    m.m8 = frente.x;   m.m9 = frente.y;   m.m10 = frente.z;
    m.m12 = x;         m.m13 = y;         m.m14 = z;
    m.m15 = 1.0f;
    return m;
}

/* O ângulo da rampa: a porta toda aberta passa da horizontal até encostar no chão */
static float anguloDaRampa(void) {
    return asinf(Clamp(dobradica.y / fmaxf(alturaDaPorta, 0.1f), 0.0f, 0.9f));
}

/* Quantos metros a rampa ocupa no chão, atrás da dobradiça */
static float comprimentoDaRampa(void) {
    return alturaDaPorta * cosf(anguloDaRampa());
}

/* A porta no mundo: girada em volta da dobradiça (para trás e para baixo, até virar a rampa) */
static Matrix matrizDaPortaDoMack(void) {
    float giro = aberturaDaPorta * (PI / 2.0f + anguloDaRampa());
    Matrix emVolta = MatrixMultiply(MatrixMultiply(MatrixTranslate(-dobradica.x, -dobradica.y, -dobradica.z), MatrixRotateX(-giro)),
                                    MatrixTranslate(dobradica.x, dobradica.y, dobradica.z));
    return MatrixMultiply(emVolta, matrizDoMack());
}

/*
 * colocarMack
 * As matrizes do corpo e de cada roda neste quadro (as rodas giram o quanto o Mack andou, cada
 * uma pelo raio dela), para o desenho e a sombra.
 */
static void colocarMack(void) {
    if (!temMack) {
        return;
    }
    Matrix noMundo = matrizDoMack();
    for (int i = 0; i < mack.meshCount; i++) {
        matrizesDoMack[i] = noMundo;
    }
    colocarNoMundoAberto(0, &mack, matrizesDoMack);
    if (temRodasDoMack) {
        for (int i = 0; i < rodasDoMack.meshCount; i++) {
            Vector3 eixo = eixoDaRoda[i];
            Matrix giro = MatrixMultiply(MatrixMultiply(MatrixTranslate(-eixo.x, -eixo.y, -eixo.z),
                                                        MatrixRotateX(mackAndou / raioDaRodaDoMack[i])),
                                         MatrixTranslate(eixo.x, eixo.y, eixo.z));
            matrizesDasRodas[i] = MatrixMultiply(giro, noMundo);
        }
        colocarNoMundoAberto(1, &rodasDoMack, matrizesDasRodas);
    }
    if (temPortaDoMack) {
        Matrix porta = matrizDaPortaDoMack();
        for (int i = 0; i < modeloDaPorta.meshCount; i++) {
            matrizesDaPorta[i] = porta;
        }
        colocarNoMundoAberto(2, &modeloDaPorta, matrizesDaPorta);
    }
}

/* A porta da carreta (o fundo do Mack, no chão) */
static Vector3 portaDoMack(void) {
    float fx, fz, dx, dz;
    eixosDoMack(&fx, &fz, &dx, &dz);
    return (Vector3){ mackX - fx * meioDoMack, mackY, mackZ - fz * meioDoMack };
}

/*
 * empurrarDoMack
 * O Mack parado é sólido: três círculos ao longo do carro (do bico à traseira) não entram na caixa
 * dele, vista de cima. Quem entra sai pelo lado mais perto, e a velocidade contra ele some.
 */
static void empurrarDoMack(void) {
    if (!temMack || carro.y > mackY + alturaDoMack) {
        return;
    }
    float fx, fz, dx, dz;
    eixosDoMack(&fx, &fz, &dx, &dz);
    float bicoX = carro.frente[0], bicoZ = carro.frente[2];
    float tamanho = hypotf(bicoX, bicoZ);
    if (tamanho < 1e-3f) {
        return;
    }
    bicoX /= tamanho;
    bicoZ /= tamanho;
    float raio = carro.largura / 2.0f;
    float ate = fmaxf(carro.comprimento / 2.0f - raio, 0.0f);
    for (int i = -1; i <= 1; i++) {
        float px = carro.x + bicoX * ate * i, pz = carro.z + bicoZ * ate * i;
        float aoLongo = (px - mackX) * fx + (pz - mackZ) * fz;
        float deLado = (px - mackX) * dx + (pz - mackZ) * dz;
        float sobraLongo = aoLongo - Clamp(aoLongo, -meioDoMack, meioDoMack);
        float sobraLado = deLado - Clamp(deLado, -meiaLarguraDoMack, meiaLarguraDoMack);
        float distancia = hypotf(sobraLongo, sobraLado);
        float paraLongo, paraLado, entrou;
        if (distancia > 1e-4f) {
            if (distancia >= raio) {
                continue;
            }
            paraLongo = sobraLongo / distancia;
            paraLado = sobraLado / distancia;
            entrou = raio - distancia;
        } else if (meioDoMack - fabsf(aoLongo) < meiaLarguraDoMack - fabsf(deLado)) {
            paraLongo = (aoLongo >= 0.0f) ? 1.0f : -1.0f;   /* dentro: sai pela ponta mais perto */
            paraLado = 0.0f;
            entrou = meioDoMack - fabsf(aoLongo) + raio;
        } else {
            paraLongo = 0.0f;
            paraLado = (deLado >= 0.0f) ? 1.0f : -1.0f;   /* ou pelo lado */
            entrou = meiaLarguraDoMack - fabsf(deLado) + raio;
        }
        float nx = paraLongo * fx + paraLado * dx, nz = paraLongo * fz + paraLado * dz;
        carro.x += nx * entrou;
        carro.z += nz * entrou;
        float contra = carro.vx * nx + carro.vz * nz;
        if (contra < 0.0f) {
            carro.vx -= contra * nx;
            carro.vz -= contra * nz;
            carro.pancada = fmaxf(carro.pancada, fminf(-contra / 25.0f, 1.0f));
        }
    }
}

/* O carro está na vaga atrás da carreta? */
static int naVagaDoMack(void) {
    float fx, fz, dx, dz;
    eixosDoMack(&fx, &fz, &dx, &dz);
    float rx = carro.x - mackX, rz = carro.z - mackZ;
    float atrasDaPorta = -(rx * fx + rz * fz) - meioDoMack;
    float deLado = rx * dx + rz * dz;
    return atrasDaPorta > -0.5f && atrasDaPorta < VAGA_ATE && fabsf(deLado) < VAGA_DE_LADO && fabsf(carro.y - mackY) < 3.0f;
}

/* O carro parado num lugar, de pé no chão, com o bico para o rumo dado (nas cenas do Mack) */
static void porCarroEm(float x, float z, float rumo, float andou) {
    float chao = alturaDoChaoNaMalha(malha, x, z, mackY + 3.0f, NULL);
    carro.x = x;
    carro.z = z;
    if (chao > SEM_CHAO) {
        carro.y = chao;
    }
    carro.rumo = rumo;
    carro.frente[0] = sinf(rumo); carro.frente[1] = 0.0f; carro.frente[2] = -cosf(rumo);
    carro.cima[0] = 0.0f; carro.cima[1] = 1.0f; carro.cima[2] = 0.0f;
    carro.vx = carro.vy = carro.vz = 0.0f;
    carro.noChao = 1;
    carro.naParede = 0;
    carro.nitroLigado = 0;
    carro.escorregando = 0.0f;
    carro.volante = carro.corrida.volante = 0.0f;
    carro.corrida.anguloDeDrift = 0.0f;
    carro.corrida.rumoNoMundo = rumo * RAD2DEG;
    carro.anguloDasRodas = fmodf(carro.anguloDasRodas + andou / 0.33f + 2.0f * PI, 2.0f * PI);
}

/* A altura do chão do carro (no modelo do Mack) a "atras" metros da dobradiça: dentro da carreta, o
 * piso dela; na rampa, descendo até o chão; depois, o chão */
static float alturaNaRampa(float atras) {
    if (atras <= 0.0f) {
        return dobradica.y;
    }
    float comprimento = comprimentoDaRampa();
    return (atras >= comprimento) ? 0.0f : dobradica.y * (1.0f - atras / comprimento);
}

/* Onde fica o meio do carro, a tantos metros atrás da dobradiça, no eixo do Mack (no chão do mundo) */
static Vector2 noEixoDoMack(float atras) {
    float fx, fz, dx, dz;
    eixosDoMack(&fx, &fz, &dx, &dz);
    Matrix m = matrizDoMack();
    Vector3 p = Vector3Transform((Vector3){ dobradica.x, 0.0f, dobradica.z - atras }, m);
    return (Vector2){ p.x, p.z };
}

/*
 * porCarroNaRampa
 * O carro no eixo do Mack, com o bico para a carreta, a "atras" metros da dobradiça (o meio dele):
 * no chão, subindo a rampa (de bico para cima) ou dentro da carreta.
 */
static void porCarroNaRampa(float atras, float andou) {
    float eixo = carro.comprimento * 0.31f;   /* meia distância entre os eixos */
    float naFrente = alturaNaRampa(atras - eixo), naTraseira = alturaNaRampa(atras + eixo);
    float inclinacao = atan2f(naFrente - naTraseira, 2.0f * eixo);
    Vector2 lugar = noEixoDoMack(atras);
    porCarroEm(lugar.x, lugar.y, mackRumo, andou);
    carro.y = mackY + (naFrente + naTraseira) / 2.0f;
    float fx = sinf(mackRumo), fz = -cosf(mackRumo);
    carro.frente[0] = fx * cosf(inclinacao); carro.frente[1] = sinf(inclinacao); carro.frente[2] = fz * cosf(inclinacao);
    carro.cima[0] = -fx * sinf(inclinacao); carro.cima[1] = cosf(inclinacao); carro.cima[2] = -fz * sinf(inclinacao);
}

/* Onde o meio do carro fica no pé da rampa (alinhado, ainda no chão) e lá dentro da carreta */
static float atrasNoPeDaRampa(void) {
    return comprimentoDaRampa() + carro.comprimento / 2.0f + 0.5f;
}

static float atrasLaDentro(void) {
    return -(carro.comprimento / 2.0f + FUNDO_DA_CARRETA);
}

/* Uma passagem suave de 0 a 1, de "de" até "de + duracao" segundos */
static float passagem(float tempo, float de, float duracao) {
    float t = Clamp((tempo - de) / duracao, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

/* Do jeito que os letreiros escrevem: em maiúsculas */
static void copiarEmMaiusculas(char *destinoDoTexto, int tamanho, const char *texto) {
    int i = 0;
    for (; texto[i] != '\0' && i < tamanho - 1; i++) {
        destinoDoTexto[i] = (char) toupper((unsigned char) texto[i]);
    }
    destinoDoTexto[i] = '\0';
}

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
        carregarMack();
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
    cena = CENA_DIRIGINDO;
    mackAndou = 0.0f;
    aberturaDaPorta = 0.0f;
    tempoDoRecado = 0.0f;
    correrDireto = 0;
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

/* A saída da carreta: a porta desce e o carro desce de ré pela rampa (atualizarCena) */
static void sairDaCarreta(int daCorrida) {
    cena = CENA_SAINDO;
    tempoDaCena = 0.0f;
    saindoDaCorrida = daCorrida;
    mackAndou = 0.0f;
    aberturaDaPorta = 0.0f;
    pausado = 0;
    mapaAberto = 0;
    porCarroNaRampa(atrasLaDentro(), 0.0f);
    limparMarcas();
    limparFumaca();
    iniciarEfeitosDaCorrida();
}

/*
 * voltarDaEscolhaDaCorrida
 * Desistiu na escolha da corrida: o carro desce de ré da carreta, de volta à vaga.
 */
void voltarDaEscolhaDaCorrida(void) {
    correrDireto = 0;
    if (pronto) {
        sairDaCarreta(0);
    }
}

/*
 * partirComOMack
 * Escolheu a corrida: com o carro na carreta, o Mack parte pela rua principal. No fim da cena,
 * atualizarPasseio leva à corrida (que começa pela apresentação da pista).
 * Recebe: o nome da pista.
 * Devolve: nada.
 */
void partirComOMack(const char *nomeDaPista) {
    copiarEmMaiusculas(destino, sizeof(destino), nomeDaPista);
    if (!pronto) {
        correrDireto = 1;   /* sem o mapa, sem cena */
        return;
    }
    cena = CENA_PARTINDO;
    tempoDaCena = 0.0f;
    mackAndou = 0.0f;
    aberturaDaPorta = 0.0f;
    pausado = 0;
    mapaAberto = 0;
}

/*
 * voltarDaCorrida
 * O Mack trouxe o carro de volta: ele desce de ré pela rampa, na vaga, e o recado (o resultado
 * da corrida) aparece por alguns segundos.
 * Recebe: o carro do jogador e o recado.
 * Devolve: nada.
 */
void voltarDaCorrida(int numeroDoCarro, const char *texto) {
    prepararMapa();
    if (numeroDoCarro != numero || !pronto) {
        iniciarPasseio(numeroDoCarro);
    }
    snprintf(recado, sizeof(recado), "%s", texto);
    tempoDoRecado = 0.0f;
    correrDireto = 0;
    if (pronto) {
        sairDaCarreta(1);
    }
}

int cenaDoMackComMusica(void) {
    return cena == CENA_PARTINDO;   /* da escolha da corrida até a apresentação da pista */
}

void descarregarPasseio(void) {
    Model *modelos[3] = { &mack, &rodasDoMack, &modeloDaPorta };
    for (int k = 0; k < 3; k++) {
        colocarNoMundoAberto(k, NULL, NULL);
    }
    int tem[3] = { mack.meshCount > 0, rodasDoMack.meshCount > 0, modeloDaPorta.meshCount > 0 };
    for (int k = 0; k < 3; k++) {
        if (!tem[k]) {
            continue;
        }
        for (int i = 0; i < modelos[k]->materialCount; i++) {
            modelos[k]->materials[i].shader.id = rlGetShaderIdDefault();   /* o shader é do mundo (mundo.c) */
        }
        UnloadModel(*modelos[k]);
        modelos[k]->meshCount = 0;
    }
    temMack = temRodasDoMack = temPortaDoMack = temOlhosDoMack = 0;
    descarregarInteriorDaCarreta();
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

/* Apertou para entrar no Mack: o carro para, e a cena da entrada começa (atualizarCena) */
static void entrarNoMack(void) {
    tocarSom(SOM_CONFIRMAR);
    cena = CENA_ENTRANDO;
    tempoDaCena = 0.0f;
    embarqueX = carro.x;
    embarqueZ = carro.z;
    embarqueRumo = carro.rumo;
    aberturaDaPorta = 0.0f;
    carro.vx = carro.vy = carro.vz = 0.0f;
    carro.corrida.velocidade = carro.corrida.re = 0.0f;
    carro.corrida.forcaDoDrift = carro.corrida.anguloDeDrift = 0.0f;
    carro.nitroLigado = carro.corrida.nitroLigado = 0;
    limparFumaca();
}

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
 * atualizarCena
 * As cenas do Mack, com o jogador só olhando (ENTER ou A pula):
 *   entrando:    o carro desliza até o pé da rampa, alinhado com a carreta; a porta desce e vira
 *                rampa; o carro sobe e entra; a porta sobe e a tela escurece: a escolha da corrida;
 *   partindo:    o Mack sai pela rua principal e passa pela câmera, com o nome da pista;
 *   saindo:      a porta desce, o carro desce de ré pela rampa até a vaga, a porta sobe e o
 *                jogador volta a dirigir (desistindo da escolha, ou na volta de uma corrida).
 * Recebe: o tempo do quadro.
 * Devolve: TELA_PISTAS no fim da entrada, TELA_CORRIDA no fim da partida; senão, TELA_PASSEIO.
 */
static Tela atualizarCena(float tempoQuadro) {
    tempoDaCena += tempoQuadro;
    int pulou = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || controleApertou(BOTAO_CONFIRMAR);
    if (cena == CENA_NA_CARRETA) {
        sairDaCarreta(0);   /* voltou para a cidade sem passar pela escolha: o carro desce */
    }
    if (cena == CENA_ENTRANDO) {
        float pe = atrasNoPeDaRampa();
        static float andouAntes = 0.0f;
        if (tempoDaCena <= tempoQuadro) {
            andouAntes = 0.0f;
        }
        if (tempoDaCena < SOBE_DE) {
            /* Desliza até o pé da rampa, virando o bico para a carreta (de frente ou de ré, como precisar) */
            float k = passagem(tempoDaCena, 0.0f, ALINHADO);
            Vector2 de = { embarqueX, embarqueZ }, ate = noEixoDoMack(pe);
            Vector2 aqui = Vector2Lerp(de, ate, k);
            float giro = mackRumo - embarqueRumo;
            giro -= 2.0f * PI * roundf(giro / (2.0f * PI));
            float andou = Vector2Distance(de, ate) * k * ((Vector2DotProduct(Vector2Subtract(ate, de),
                                                          (Vector2){ sinf(mackRumo), -cosf(mackRumo) }) >= 0.0f) ? 1.0f : -1.0f);
            porCarroEm(aqui.x, aqui.y, embarqueRumo + giro * k, andou - andouAntes);
            andouAntes = andou;
        } else {
            float k = passagem(tempoDaCena, SOBE_DE, DENTRO - SOBE_DE);
            float andou = (pe - atrasLaDentro()) * k;
            porCarroNaRampa(pe - andou, andou - andouAntes);
            andouAntes = andou;
        }
        aberturaDaPorta = passagem(tempoDaCena, PORTA_DESCE_DE, PORTA_MEXE) * (1.0f - passagem(tempoDaCena, PORTA_SOBE_DE, PORTA_MEXE));
        atualizarEfeitosDoPasseio(&carro.corrida, 0.0f, 0, 0.0f, 1, tempoQuadro);
        if (tempoDaCena >= SEGUNDOS_ENTRANDO || pulou) {
            porCarroNaRampa(atrasLaDentro(), 0.0f);
            aberturaDaPorta = 0.0f;
            cena = CENA_NA_CARRETA;   /* o carro está na carreta: a escolha da corrida */
            iniciarEfeitosDaCorrida();
            return TELA_PISTAS;
        }
    } else if (cena == CENA_PARTINDO) {
        mackAndou = 0.5f * ACELERACAO_DO_MACK * tempoDaCena * tempoDaCena;
        if (tempoDaCena >= SEGUNDOS_PARTINDO || pulou) {
            cena = CENA_NA_CARRETA;
            mackAndou = 0.0f;
            limparMarcas();   /* as marcas e a fumaça da cidade não vão para a corrida */
            limparFumaca();
            iniciarEfeitosDaCorrida();
            return TELA_CORRIDA;
        }
    } else if (cena == CENA_SAINDO) {
        static float desceuAntes = 0.0f;
        if (tempoDaCena <= tempoQuadro) {
            desceuAntes = 0.0f;
        }
        float fora = atrasNoPeDaRampa() + LONGE_DA_RAMPA;
        float desceu = (fora - atrasLaDentro()) * passagem(tempoDaCena, DESCE_DE, FORA - DESCE_DE);
        porCarroNaRampa(atrasLaDentro() + desceu, desceuAntes - desceu);
        desceuAntes = desceu;
        aberturaDaPorta = passagem(tempoDaCena, PORTA_DESCE_NA_SAIDA, PORTA_MEXE)
                          * (1.0f - passagem(tempoDaCena, PORTA_SOBE_NA_SAIDA, PORTA_MEXE));
        atualizarEfeitosDoPasseio(&carro.corrida, 0.0f, 0, 0.0f, 1, tempoQuadro);
        if (tempoDaCena >= SEGUNDOS_SAINDO || pulou) {
            porCarroNaRampa(fora, 0.0f);
            aberturaDaPorta = 0.0f;
            cena = CENA_DIRIGINDO;
            reiniciarCamera();
            if (saindoDaCorrida) {
                tempoDoRecado = SEGUNDOS_DO_RECADO;
            }
        }
    }
    return TELA_PASSEIO;
}

/*
 * atualizarPasseio
 * Um quadro do mundo aberto. ESC (ou START) pausa; M (ou LB) abre o mapa; R (ou Y) põe o carro de
 * volta na rua principal; T leva à pista de treino; C (ou o touchpad) troca a câmera. Na vaga
 * atrás do Mack, ENTER (ou Y) entra nele.
 * Recebe: o estado do menu (para a pausa) e o tempo do quadro.
 * Devolve: a próxima tela.
 */
Tela atualizarPasseio(EstadoMenu *menu, float tempoQuadro) {
    if (!pronto) {
        if (correrDireto) {
            correrDireto = 0;
            return TELA_CORRIDA;
        }
        if (IsKeyPressed(KEY_ENTER) || controleApertou(BOTAO_CONFIRMAR)) {
            tocarSom(SOM_CONFIRMAR);
            return TELA_PISTAS;   /* sem o mapa, as corridas pelo menu de sempre */
        }
        if (IsKeyPressed(KEY_ESCAPE) || controleApertou(BOTAO_VOLTAR)) {
            tocarSom(SOM_VOLTAR);
            return sair();
        }
        return TELA_PASSEIO;
    }
    if (tempoDoRecado > 0.0f && !pausado && !mapaAberto) {
        tempoDoRecado -= tempoQuadro;
    }
    sustoDoMack = fmaxf(sustoDoMack - tempoQuadro, 0.0f);
    if (cena != CENA_DIRIGINDO) {
        return atualizarCena(tempoQuadro);
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
    /* Na vaga do Mack, ENTER (ou Y) entra nele; fora dela, Y volta à rua principal */
    int naVaga = naVagaDoMack();
    if (naVaga && (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || controleApertou(BOTAO_LIMPAR))) {
        entrarNoMack();
        return TELA_PASSEIO;
    }
    if (IsKeyPressed(KEY_R) || (!naVaga && controleApertou(BOTAO_LIMPAR))) {
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
    float pancadaAntes = carro.pancada;
    empurrarDoMack();
    if (carro.pancada > fmaxf(pancadaAntes, 0.15f)) {
        sustoDoMack = SEGUNDOS_DE_SUSTO;   /* bateram no Mack: ele se assusta */
    }
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
 * cameraDaCena
 * As câmeras paradas das cenas do Mack: atrás e ao lado da carreta (embarcando e chegando), e
 * na beira da rua, à frente dele, olhando o Mack passar (partindo).
 */
static Camera3D cameraDaCena(void) {
    float fx, fz, dx, dz;
    eixosDoMack(&fx, &fz, &dx, &dz);
    Vector3 porta = portaDoMack();
    Vector3 posicao, alvo;
    if (cena == CENA_PARTINDO) {
        float frente = meioDoMack + 22.0f, lado = -(meiaLarguraDoMack + 5.5f);
        posicao = (Vector3){ mackX + fx * frente + dx * lado, 0.0f, mackZ + fz * frente + dz * lado };
        float chao = alturaDoChaoNaMalha(malha, posicao.x, posicao.z, mackY + 20.0f, NULL);
        posicao.y = ((chao > SEM_CHAO) ? chao : mackY) + 1.6f;
        Matrix m = matrizDoMack();
        alvo = (Vector3){ m.m12, m.m13 + alturaDoMack * 0.45f, m.m14 };
    } else if (cena == CENA_SAINDO) {
        /* Atrás da rampa, olhando reto para dentro da carreta: o carro desce de ré na direção da câmera
         * (e a câmera de dirigir começa logo atrás dela, quando o jogador assume) */
        Matrix m = matrizDoMack();
        posicao = Vector3Transform((Vector3){ 0.6f, 3.2f, dobradica.z - 17.0f }, m);
        alvo = Vector3Transform((Vector3){ 0.0f, dobradica.y + 1.6f, dobradica.z + 5.0f }, m);
        (void) porta;
    } else {
        /* Atrás e à esquerda da carreta (do lado da rua), vendo a rampa de lado */
        Matrix m = matrizDoMack();
        posicao = Vector3Transform((Vector3){ meiaLarguraDoMack + 7.0f, 4.6f, dobradica.z - 17.0f }, m);
        alvo = Vector3Transform((Vector3){ 0.0f, 2.2f, dobradica.z - 4.5f }, m);
    }
    Camera3D camera = { 0 };
    camera.position = posicao;
    camera.target = alvo;
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = (cena == CENA_PARTINDO) ? 50.0f : 55.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    return camera;
}

/* O quanto a tela está escura nas cenas do Mack (0 a 1) */
static float escuroDaCena(void) {
    float entra = 1.0f - tempoDaCena / ESCURECE;   /* toda cena começa clareando */
    float sai = 0.0f;
    if (cena == CENA_ENTRANDO) {
        entra = 0.0f;   /* começa dirigindo: sem escurecer */
        sai = (tempoDaCena - (SEGUNDOS_ENTRANDO - ESCURECE)) / ESCURECE;
    } else if (cena == CENA_PARTINDO) {
        sai = (tempoDaCena - (SEGUNDOS_PARTINDO - ESCURECE * 1.4f)) / (ESCURECE * 1.4f);
    } else if (cena == CENA_SAINDO) {
        entra = 1.0f - tempoDaCena / ESCURECE;
    } else {
        return 0.0f;
    }
    return Clamp(fmaxf(entra, sai), 0.0f, 1.0f);
}

/*
 * desenharCinema
 * Nas cenas do Mack: as faixas pretas em cima e embaixo, o letreiro da cena e o escurecer.
 */
static void desenharCinema(void) {
    float largura = (float) larguraVirtual(), altura = (float) alturaVirtual();
    float faixa = altura * FAIXA_DE_CINEMA;
    DrawRectangle(-200, -200, (int) largura + 400, (int) faixa + 200, BLACK);
    DrawRectangle(-200, (int) (altura - faixa), (int) largura + 400, (int) faixa + 200, BLACK);
    const char *titulo = NULL, *subtitulo = NULL;
    float alfa = 0.0f;
    if (cena == CENA_PARTINDO) {
        titulo = destino;
        subtitulo = "O MACK ESTÁ LEVANDO VOCÊ PARA A CORRIDA";
        alfa = Clamp((tempoDaCena - 0.6f) / 0.5f, 0.0f, 1.0f);
    } else if (cena == CENA_SAINDO && saindoDaCorrida) {
        titulo = "RADIATOR SPRINGS";
        subtitulo = "DE VOLTA À CIDADE";
        alfa = Clamp((tempoDaCena - 0.4f) / 0.5f, 0.0f, 1.0f) * Clamp((SEGUNDOS_SAINDO - tempoDaCena) / 0.4f, 0.0f, 1.0f);
    }
    if (titulo != NULL && alfa > 0.0f) {
        float y = altura - faixa - 92.0f;
        escrever(FONTE_FORTE, subtitulo, 48.0f, y, 16.0f, Fade(COR_AMARELO, alfa));
        escrever(FONTE_TITULO, titulo, 46.0f, y + 18.0f, 46.0f, Fade(WHITE, alfa));
    }
    float escuro = escuroDaCena();
    if (escuro > 0.0f) {
        DrawRectangle(-200, -200, (int) largura + 400, (int) altura + 400, Fade(BLACK, escuro));
    }
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
    /* A pista de treino, a cidade e o Mack, presos à borda quando estão longe */
    float pontos[3][2] = { { treino[0], treino[2] }, { largada[0], largada[2] }, { mackX, mackZ } };
    Color cores[3] = { COR_AMARELO, WHITE, COR_DO_MACK };
    for (int k = 0; k < 3; k++) {
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
    Vector2 m = naFoto(mackX, mackZ);
    Vector2 mackNaTela = { tela.x + (m.x - origem.x) * escala, tela.y + (m.y - origem.y) * escala };
    if (CheckCollisionPointRec(mackNaTela, tela)) {
        DrawCircleV(mackNaTela, 8.0f, BLACK);
        DrawCircleV(mackNaTela, 6.0f, COR_DO_MACK);
        escrever(FONTE_FORTE, "MACK: CORRIDAS", mackNaTela.x + 11.0f, mackNaTela.y + 2.0f, 14.0f, COR_DO_MACK);
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
 * desenharMinimapaRedondo
 * A imagem do minimapa recortada num círculo (um leque de triângulos), com uma borda.
 */
static void desenharMinimapaRedondo(Vector2 centro, float raio) {
    DrawCircleV(centro, raio + 4.0f, COR_PAINEL);
    rlDrawRenderBatchActive();
    rlDisableBackfaceCulling();
    rlSetTexture(minimapa.texture.id);
    rlBegin(RL_TRIANGLES);
    rlColor4ub(255, 255, 255, 255);
    const int LADOS = 56;
    for (int i = 0; i < LADOS; i++) {
        float a0 = 2.0f * PI * i / LADOS, a1 = 2.0f * PI * (i + 1) / LADOS;
        /* a imagem da RenderTexture está de cabeça para baixo: o alto da tela é o v = 1 */
        rlTexCoord2f(0.5f, 0.5f);
        rlVertex2f(centro.x, centro.y);
        rlTexCoord2f(0.5f + 0.5f * cosf(a1), 0.5f - 0.5f * sinf(a1));
        rlVertex2f(centro.x + cosf(a1) * raio, centro.y + sinf(a1) * raio);
        rlTexCoord2f(0.5f + 0.5f * cosf(a0), 0.5f - 0.5f * sinf(a0));
        rlVertex2f(centro.x + cosf(a0) * raio, centro.y + sinf(a0) * raio);
    }
    rlEnd();
    rlSetTexture(0);
    rlDrawRenderBatchActive();
    rlEnableBackfaceCulling();
    DrawRing(centro, raio, raio + 2.0f, 0.0f, 360.0f, 64, Fade(WHITE, 0.55f));
}

/*
 * desenharHudDoPasseio
 * O painel do motor da corrida embaixo à direita e o minimapa (redondo, com o Mack marcado) embaixo
 * à esquerda. (O wallride não tem aviso: a câmera deitando com o carro já diz tudo.)
 */
static void desenharHudDoPasseio(void) {
    float largura = (float) larguraVirtual(), altura = (float) alturaVirtual();

    /* O velocímetro da corrida, com a velocidade de verdade do carro livre */
    int kmh = converterParaKmh(velocidadeDoCarroLivre(&carro) * UNIDADES_POR_METRO);
    desenharPainelDoMotor(&carro.corrida, kmh);

    /* O minimapa, redondo, com a tecla do mapa embaixo */
    Vector2 centro = { 24.0f + RAIO_DO_MINIMAPA_NA_TELA, altura - 34.0f - RAIO_DO_MINIMAPA_NA_TELA };
    desenharMinimapaRedondo(centro, RAIO_DO_MINIMAPA_NA_TELA);
    const char *tecla = controleConectado() ? "LB  MAPA" : "M  MAPA";
    float larguraDaTecla = larguraDoTexto(FONTE_FORTE, tecla, 12.0f) + 16.0f;
    caixaArredondada((Rectangle){ centro.x - larguraDaTecla / 2.0f, centro.y + RAIO_DO_MINIMAPA_NA_TELA + 8.0f, larguraDaTecla, 18.0f },
                     6.0f, COR_PAINEL);
    escreverCentralizado(FONTE_FORTE, tecla, centro.x, centro.y + RAIO_DO_MINIMAPA_NA_TELA + 10.0f, 12.0f, WHITE);

    float x = largura / 2.0f;

    /* Na vaga atrás da carreta: o botão de entrar no Mack */
    if (naVagaDoMack()) {
        const char *tecla = controleConectado() ? "Y" : "ENTER";
        const char *texto = "ENTRAR NO MACK";
        float larguraDaTecla = larguraDoTexto(FONTE_TITULO, tecla, 18.0f) + 18.0f;
        float total = larguraDaTecla + 12.0f + larguraDoTexto(FONTE_TITULO, texto, 20.0f) + 28.0f;
        Rectangle caixa = { x - total / 2.0f, altura - 92.0f, total, 44.0f };
        caixaArredondada(caixa, 10.0f, COR_PAINEL);
        caixaArredondada((Rectangle){ caixa.x + 14.0f, caixa.y + 9.0f, larguraDaTecla, 26.0f }, 6.0f, COR_DO_MACK);
        escreverCentralizado(FONTE_TITULO, tecla, caixa.x + 14.0f + larguraDaTecla / 2.0f, caixa.y + 11.0f, 18.0f, WHITE);
        escrever(FONTE_TITULO, texto, caixa.x + 26.0f + larguraDaTecla, caixa.y + 10.0f, 20.0f, WHITE);
    }

    /* O resultado da corrida, na volta à cidade */
    if (tempoDoRecado > 0.0f && recado[0] != '\0') {
        float alfa = Clamp(tempoDoRecado / 0.8f, 0.0f, 1.0f);
        float larguraDoRecado = larguraDoTexto(FONTE_TITULO, recado, 24.0f) + 48.0f;
        caixaArredondada((Rectangle){ x - larguraDoRecado / 2.0f, 80.0f, larguraDoRecado, 42.0f }, 10.0f, Fade(COR_PAINEL, alfa));
        escreverCentralizado(FONTE_TITULO, recado, x, 87.0f, 24.0f, Fade(WHITE, alfa));
    }
}

/* Um retângulo com uma cor em cada canto (dentro de rlBegin(RL_QUADS)) */
static void quadrado(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color ca, Color cb, Color cc, Color cd) {
    rlColor4ub(ca.r, ca.g, ca.b, ca.a); rlVertex3f(a.x, a.y, a.z);
    rlColor4ub(cb.r, cb.g, cb.b, cb.a); rlVertex3f(b.x, b.y, b.z);
    rlColor4ub(cc.r, cc.g, cc.b, cc.a); rlVertex3f(c.x, c.y, c.z);
    rlColor4ub(cd.r, cd.g, cd.b, cd.a); rlVertex3f(d.x, d.y, d.z);
}

/*
 * desenharPortaAberta
 * Com a porta aberta: o lado de dentro da carreta (carreta.c) e o lado de dentro da porta, de chapa
 * de metal, que vira o piso da rampa.
 */
static void desenharPortaAberta(void) {
    if (!temPortaDoMack || aberturaDaPorta <= 0.001f) {
        return;
    }
    float x0 = dobradica.x - larguraDaPorta / 2.0f + 0.03f, x1 = dobradica.x + larguraDaPorta / 2.0f - 0.03f;
    float y0 = dobradica.y + 0.01f, y1 = dobradica.y + alturaDaPorta - 0.03f;
    Matrix deDentro = MatrixMultiply(MatrixTranslate(dobradica.x, dobradica.y + 0.005f, dobradica.z + 0.04f), matrizDoMack());
    desenharInteriorDaCarreta(deDentro, larguraDaPorta - 0.06f, alturaDaPorta - 0.04f, FUNDO_DO_ESCURO);

    /* O lado de dentro da porta (o piso da rampa): chapa cinza, com as bordas e umas faixas mais escuras */
    float meioX = (x0 + x1) / 2.0f, borda = 0.18f;
    float z = dobradica.z + 0.03f;
    Color chapa = { 118, 120, 124, 255 }, chapaClara = { 140, 142, 146, 255 }, escura = { 62, 62, 66, 255 };
    rlPushMatrix();
    rlMultMatrixf(MatrixToFloat(matrizDaPortaDoMack()));
    rlBegin(RL_QUADS);
    quadrado((Vector3){ x0 + borda, y0, z }, (Vector3){ x1 - borda, y0, z }, (Vector3){ x1 - borda, y1, z }, (Vector3){ x0 + borda, y1, z },
             chapa, chapa, chapaClara, chapaClara);
    quadrado((Vector3){ x0, y0, z }, (Vector3){ x0 + borda, y0, z }, (Vector3){ x0 + borda, y1, z }, (Vector3){ x0, y1, z }, escura, escura, escura, escura);
    quadrado((Vector3){ x1 - borda, y0, z }, (Vector3){ x1, y0, z }, (Vector3){ x1, y1, z }, (Vector3){ x1 - borda, y1, z }, escura, escura, escura, escura);
    for (float y = y0 + 0.45f; y < y1 - 0.2f; y += 0.55f) {   /* as travas, onde o pneu pega na subida */
        quadrado((Vector3){ x0 + borda, y, z + 0.01f }, (Vector3){ x1 - borda, y, z + 0.01f },
                 (Vector3){ x1 - borda, y + 0.07f, z + 0.01f }, (Vector3){ x0 + borda, y + 0.07f, z + 0.01f }, escura, escura, escura, escura);
    }
    rlEnd();
    rlPopMatrix();
    rlDrawRenderBatchActive();
    (void) meioX;
}

/*
 * olharDoMack
 * Para onde os olhos do Mack vão (como o volante dos carros: -1 todo à esquerda, +1 todo à
 * direita): acompanham o carro do jogador; partindo, olham para a câmera, que ele passa.
 */
static float olharDoMack(Camera3D camera) {
    Matrix m = matrizDoMack();
    Vector3 olhos = Vector3Transform((Vector3){ 0.0f, alturaDoMack * 0.64f, meioDoMack - 2.5f }, m);
    Vector3 alvo = (cena == CENA_PARTINDO) ? camera.position : (Vector3){ carro.x, carro.y + 0.8f, carro.z };
    Vector3 para = Vector3Subtract(alvo, olhos);
    if (Vector3Length(para) > OLHA_ATE) {
        return 0.0f;
    }
    Vector3 frente = { m.m8, m.m9, m.m10 }, direita = { -m.m0, -m.m1, -m.m2 };
    float angulo = atan2f(Vector3DotProduct(para, direita), Vector3DotProduct(para, frente)) * RAD2DEG;
    return Clamp(angulo / CANTO_DO_OLHO, -1.0f, 1.0f);
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
    Camera3D camera = montarCamera();   /* sempre: ela segue o carro mesmo nas cenas, para não dar tranco depois */
    if (cena != CENA_DIRIGINDO) {
        camera = cameraDaCena();
    }
    int carroAparece = (cena != CENA_PARTINDO && cena != CENA_NA_CARRETA);   /* partindo, ele está na carreta */
    Matrix noMundo = matrizDoCarro();
    colocarMack();   /* antes das sombras: ele também faz sombra */
    soltarPoeiraEFumaca(noMundo, (cena == CENA_DIRIGINDO) ? dt : 0.0f);
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
    if (temOlhosDoMack) {
        prepararOlhosDoModelo(olharDoMack(camera), (cena == CENA_PARTINDO) ? 0.3f : 0.0f, (sustoDoMack > 0.0f) ? 1.0f : 0.0f);
    }
    desenharModelosNoMundoAberto();   /* o Mack, as rodas e a porta */
    desenharPortaAberta();
    desenharMarcas();
    if (carroAparece) {
        if (cena == CENA_DIRIGINDO) {   /* na rampa e dentro da carreta, a sombra cairia na rua */
            comecarSombrasNoChao();
            desenharSombraDoCarro(camera, noMundo);
            terminarSombrasNoChao();
        }
        desenharCarro(noMundo);
    }
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
    if (cena != CENA_DIRIGINDO) {
        desenharCinema();
    } else if (mapaAberto) {
        desenharMapaGrande();
    } else {
        desenharHudDoPasseio();
    }
    terminarInterface();
    if (pausado) {
        comecarInterface(1);
        desenharPausa(menu);
        terminarInterface();
    }
}
