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
 *
 * O dia passa (hora.c): 48 minutos do nascer do sol ao nascer do sol, com
 * o pôr do sol e a noite. No quarto do cone do Cozy Cone mais perto do Mack
 * (o cone 8), parado, o jogador dorme oito horas. O jogo começa nesse quarto, numa hora sorteada,
 * e a volta de uma corrida também cai numa hora sorteada.
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
#include "interiores.h"
#include "hora.h"
#include "luzes.h"
#include "postes.h"
#include "tratores.h"
#include "grilos.h"
#include "moradores.h"
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
#define ATRASO_DA_CAMERA      0.22f     /* segundos: a câmera vem atrás do caminho do carro com este atraso */
#define CAMINHO_DA_CAMERA_DE  2.0f      /* m/s: abaixo disso, a câmera vai atrás do rumo; acima de... */
#define CAMINHO_DA_CAMERA_ATE 8.0f      /* ...isto, de para onde o carro está indo (como na corrida, carro.c) */
#define ATRASO_DA_ALTURA      0.10f     /* segundos: e sobe e desce com o carro com este */
#define VELOCIDADE_DA_DISTANCIA 7.0f    /* 1/s: a troca de distância da câmera */
#define FOLGA_DA_CAMERA       1.2f      /* metros: a câmera nunca entra no chão */
/* O que há em volta da câmera (cameraLivre): ela confere, na malha de colisão, o caminho do carro até ela */
#define ALTURA_DO_PIVO        1.2f      /* metros acima do carro: o ponto que a câmera tem de enxergar */
#define FOLGA_DA_PAREDE       0.85f     /* metros: o mínimo entre a câmera e uma parede ou um teto (o plano de perto
                                         * da imagem fica a 0,5 m dela, e os cantos dele a ~0,8 m: mais perto, ela
                                         * vê através da parede) */
#define MENOR_BRACO           0.6f      /* metros do pivô: o mais perto do carro que uma parede traz a câmera */
#define SOBE_O_MORRO_ATE      6.0f      /* metros: um morro atrás do carro mais alto que isso, ela vem para a frente dele */
#define COBERTO_ATE           12.0f     /* metros acima do carro: um teto mais baixo que isso, e o carro está coberto */
#define ATRASO_DA_COBERTURA   0.35f     /* segundos: entrando ou saindo de baixo de um teto, a câmera vem para perto
                                         * (e volta) com este atraso */
#define VOLTA_DA_CAMERA       3.0f      /* 1/s: livre do obstáculo, a câmera volta para o lugar dela aos poucos */
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
/* Coberto (dentro de um prédio, como a loja do Luigi, debaixo de uma marquise ou de uma ponte), a câmera vem
 * para perto, mesmo com a distância de longe escolhida */
static const Distancia DISTANCIA_COBERTA = { 5.0f, 2.0f };

static MalhaDeColisao *malha = NULL;
static int tentouCarregar = 0;
static int pronto = 0;
static CarroLivre carro;
static int numero = 0;
static int pausado = 0, mapaAberto = 0, zoomDoMapa = 1;
static const float ZOOMS_DO_MAPA[3] = { 1.0f, 2.5f, 6.0f };
static Olhar olhar;
static float rumoDaCamera = 0.0f, alturaDaCamera = 0.0f, noWallride = 0.0f;
static float cobertura = 0.0f;                  /* 0 ao ar livre, 1 coberto (com o atraso da cobertura) */
static Vector3 desvioDaCamera = { 0.0f, 0.0f, 0.0f };   /* o quanto o que há em volta tirou a câmera do lugar dela */
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
#define DESENHA_O_BAU_ATE     150.0f    /* metros: mais longe que isso, o lado de dentro da carreta não aparece */
#define DESENHA_MORADOR_ATE   320.0f    /* metros: mais longe que isso, os moradores não são desenhados */
#define SOMBRA_DE_MORADOR_ATE 70.0f     /* e a sombra deles, só de perto */
#define BATIDA_QUE_ASSUSTA    3.0f      /* m/s: batendo mais forte que isso num morador, ele se assusta */
#define OUVE_ATE              26.0f     /* metros: a fala de um morador mais perto que isso aparece na tela */
#define FAIXA_DE_CINEMA       0.11f     /* da altura da tela: as faixas pretas das cenas */
/* Os olhos do Mack (os dos carros, carros3d.c): acompanham o carro do jogador */
#define OLHA_ATE              70.0f     /* metros: mais longe que isso, ele olha à frente (e em volta) */
#define CANTO_DO_OLHO         55.0f     /* graus para o lado em que a íris chega ao canto */
#define SEGUNDOS_DE_SUSTO     0.7f      /* bateram nele */

typedef enum { CENA_DIRIGINDO, CENA_ENTRANDO, CENA_NA_CARRETA, CENA_PARTINDO, CENA_SAINDO } CenaDoMack;

/* Dormir no Cozy Cone: no quarto do cone 8 (o mais perto do Mack), parado, ENTER (ou Y) dorme; a tela
 * escurece, o relógio pula oito horas e a tela clareia. É também ali que o jogo começa */
#define CAMA_X                775.127f  /* o lugar do carro no cone 8 (recursos/modelos/cozy_cone_quartos.json) */
#define CAMA_Z                157.912f
#define CAMA_RUMO             -1.5623f  /* de frente para a porta (entrou de ré, como no filme) */
#define TETO_DO_QUARTO        4.83f     /* o chão do quarto se procura abaixo disto (acima está o teto e o topo do cone) */
#define NA_CAMA_ATE           3.5f      /* metros do lugar */
#define PISO_DO_QUARTO        2.4f      /* acima disso o carro está no piso do quarto (a praça fica em 1,9) */
#define PARADO_PARA_DORMIR    1.5f      /* m/s */
#define ADORMECE_EM           1.2f      /* segundos escurecendo... */
#define DORME_POR             1.8f      /* ...dormindo (a tela preta)... */
#define ACORDA_EM             1.4f      /* ...e clareando */
#define COR_DA_NOITE          (Color){ 72, 86, 196, 255 }
#define LUGAR_DOS_POSTES      3         /* o lugar dos postes entre os modelos colocados no mundo (mundo.h) */

#define HORAS_DE_SONO         8.0f      /* dormindo, o relógio pula oito horas */
typedef enum { ACORDADO, ADORMECENDO, DORMINDO, ACORDANDO } Sono;
static Sono sono = ACORDADO;
static float tempoDoSono = 0.0f;
static float horaDeAcordar = 0.0f;

static Model mack, rodasDoMack, modeloDaPorta;
static int luzesDoMack = -1, luzesDoBau = -1;   /* as lâmpadas de fora do Mack e o que brilha no baú (luzes.c) */
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
 * empurrarDaCaixa
 * Uma caixa sólida (o Mack, um morador), vista de cima: três círculos ao longo do carro (do bico à
 * traseira) não entram nela. Quem entra sai pelo lado mais perto, e a velocidade contra ela some.
 * Recebe: o meio da caixa, o rumo dela, as meias medidas (comprimento e largura) e o alto dela.
 * Devolve: a velocidade (m/s) com que o carro bateu nela (0 se não bateu).
 */
static float empurrarDaCaixa(float meioX, float meioZ, float rumo, float meioComprimento, float meiaLargura, float alto) {
    if (carro.y > alto) {
        return 0.0f;
    }
    float fx = sinf(rumo), fz = -cosf(rumo), dx = cosf(rumo), dz = sinf(rumo);
    float bicoX = carro.frente[0], bicoZ = carro.frente[2];
    float tamanho = hypotf(bicoX, bicoZ);
    if (tamanho < 1e-3f) {
        return 0.0f;
    }
    bicoX /= tamanho;
    bicoZ /= tamanho;
    float raio = carro.largura / 2.0f;
    float ate = fmaxf(carro.comprimento / 2.0f - raio, 0.0f);
    float bateu = 0.0f;
    for (int i = -1; i <= 1; i++) {
        float px = carro.x + bicoX * ate * i, pz = carro.z + bicoZ * ate * i;
        float aoLongo = (px - meioX) * fx + (pz - meioZ) * fz;
        float deLado = (px - meioX) * dx + (pz - meioZ) * dz;
        float sobraLongo = aoLongo - Clamp(aoLongo, -meioComprimento, meioComprimento);
        float sobraLado = deLado - Clamp(deLado, -meiaLargura, meiaLargura);
        float distancia = hypotf(sobraLongo, sobraLado);
        float paraLongo, paraLado, entrou;
        if (distancia > 1e-4f) {
            if (distancia >= raio) {
                continue;
            }
            paraLongo = sobraLongo / distancia;
            paraLado = sobraLado / distancia;
            entrou = raio - distancia;
        } else if (meioComprimento - fabsf(aoLongo) < meiaLargura - fabsf(deLado)) {
            paraLongo = (aoLongo >= 0.0f) ? 1.0f : -1.0f;   /* dentro: sai pela ponta mais perto */
            paraLado = 0.0f;
            entrou = meioComprimento - fabsf(aoLongo) + raio;
        } else {
            paraLongo = 0.0f;
            paraLado = (deLado >= 0.0f) ? 1.0f : -1.0f;   /* ou pelo lado */
            entrou = meiaLargura - fabsf(deLado) + raio;
        }
        float nx = paraLongo * fx + paraLado * dx, nz = paraLongo * fz + paraLado * dz;
        carro.x += nx * entrou;
        carro.z += nz * entrou;
        float contra = carro.vx * nx + carro.vz * nz;
        if (contra < 0.0f) {
            carro.vx -= contra * nx;
            carro.vz -= contra * nz;
            carro.pancada = fmaxf(carro.pancada, fminf(-contra / 25.0f, 1.0f));
            bateu = fmaxf(bateu, -contra);
        }
    }
    return bateu;
}

/* O Mack parado é sólido */
static float empurrarDoMack(void) {
    return temMack ? empurrarDaCaixa(mackX, mackZ, mackRumo, meioDoMack, meiaLarguraDoMack, mackY + alturaDoMack) : 0.0f;
}

/* Os moradores também; quem leva uma batida forte se assusta (os olhos arregalam) */
static void empurrarDosMoradores(void) {
    for (int i = 0; i < totalDeMoradores(); i++) {
        Morador *m = morador(i);
        if (empurrarDaCaixa(m->x, m->z, m->rumo, m->comprimento / 2.0f, m->largura / 2.0f, m->y + 2.5f) > BATIDA_QUE_ASSUSTA) {
            m->susto = 0.9f;
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

/* Um multiplicador de luz (0 a 1 em cada canal) como cor */
static Color corDaLuz(Vector3 luz) {
    return (Color){ (unsigned char) (Clamp(luz.x, 0.0f, 1.0f) * 255.0f), (unsigned char) (Clamp(luz.y, 0.0f, 1.0f) * 255.0f),
                    (unsigned char) (Clamp(luz.z, 0.0f, 1.0f) * 255.0f), 255 };
}

/* Os postes da rua: o carro não atravessa o tronco (e bater nele soa como bater num muro) */
static void empurrarDosPostes(void) {
    for (int i = 0; i < totalDePostes(); i++) {
        const Poste *p = poste(i);
        if (fabsf(p->x - carro.x) > 8.0f || fabsf(p->z - carro.z) > 8.0f || carro.y < p->base - 2.0f) {
            continue;
        }
        empurrarDaCaixa(p->x, p->z, 0.0f, p->raio, p->raio, p->topo);
    }
}

/* Os tratores dos pastos são sólidos (em pé ou de costas) */
static void empurrarDosTratores(void) {
    for (int i = 0; i < totalDeTratores(); i++) {
        float x, z, rumo, meioComprimento, meiaLargura, alto;
        if (!caixaDoTrator(i, &x, &z, &rumo, &meioComprimento, &meiaLargura, &alto)
            || fabsf(x - carro.x) > 12.0f || fabsf(z - carro.z) > 12.0f) {
            continue;
        }
        empurrarDaCaixa(x, z, rumo, meioComprimento, meiaLargura, alto);
    }
}

/* O carro parado no quarto do cone 8, dirigindo e acordado: pode dormir */
static int naCamaDoCone(void) {
    return cena == CENA_DIRIGINDO && sono == ACORDADO && hypotf(carro.x - CAMA_X, carro.z - CAMA_Z) < NA_CAMA_ATE
           && carro.y > PISO_DO_QUARTO && fabsf(velocidadeDoCarroLivre(&carro)) < PARADO_PARA_DORMIR;
}

/*
 * deitar
 * Dorme: a tela escurece, o relógio pula oito horas e a tela clareia, com a cidade já como deve estar.
 */
static void deitar(void) {
    horaDeAcordar = fmodf(horaDoDia() + HORAS_DE_SONO, 24.0f);
    sono = ADORMECENDO;
    tempoDoSono = 0.0f;
    tocarSom(SOM_CONFIRMAR);
}

/*
 * atualizarSono
 * O escurecer, o pulo do relógio e o clarear.
 * Recebe: o tempo do quadro.
 * Devolve: nada.
 */
static void atualizarSono(float tempoQuadro) {
    tempoDoSono += tempoQuadro;
    if (sono == ADORMECENDO && tempoDoSono >= ADORMECE_EM) {
        sono = DORMINDO;
        tempoDoSono = 0.0f;
        definirHoraDoDia(horaDeAcordar);
        atualizarLuzesDaCidade(luzDaHora()->luzesDaCidade, 0.0f, 1);   /* acorda com a cidade já como deve estar */
        definirHoraDaCidade(horaDoDia());
    } else if (sono == DORMINDO && tempoDoSono >= DORME_POR) {
        sono = ACORDANDO;
        tempoDoSono = 0.0f;
    } else if (sono == ACORDANDO && tempoDoSono >= ACORDA_EM) {
        sono = ACORDADO;
    }
}

/* O quanto a tela está escura no sono (0 a 1) */
static float escuroDoSono(void) {
    switch (sono) {
        case ADORMECENDO: return Clamp(tempoDoSono / ADORMECE_EM, 0.0f, 1.0f);
        case DORMINDO: return 1.0f;
        case ACORDANDO: return 1.0f - Clamp(tempoDoSono / ACORDA_EM, 0.0f, 1.0f);
        default: return 0.0f;
    }
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
        carregarInteriores();   /* a loja do Luigi e o Cozy Cone por dentro, no lugar dos prédios */
        carregarLuzesDaCidade(malha);   /* os neons e as lâmpadas da noite */
        luzesDoMack = registrarLuzesMoveis("recursos/modelos/mack_luzes.glb", "recursos/modelos/mack_luzes.json", 0);
        luzesDoBau = registrarLuzesMoveis("recursos/modelos/mack_interior_luzes.glb", NULL, 1);
        carregarPostes();               /* os postes de luz da rua e o semáforo */
        carregarTratores();             /* os tratores-vaca dos pastos */
        carregarGrilos();
    }
}

/* O carro no quarto do cone 8, no lugar dele (o chão é o do quarto, não o teto nem o topo do cone) */
static void acordarNoCone(void) {
    levarCarroPara(&carro, malha, CAMA_X, CAMA_Z, CAMA_RUMO);
    float chao = (malha != NULL) ? alturaDoChaoNaMalha(malha, CAMA_X, CAMA_Z, TETO_DO_QUARTO, NULL) : SEM_CHAO;
    if (chao > SEM_CHAO) {
        carro.y = chao;
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
    acordarNoCone();   /* o jogo começa no quarto do cone 8, de frente para a porta */
    iniciarMoradores(malha, numero);   /* cada um em casa (menos quem tem o carro do jogador) */
    colocarNoMundoAberto(LUGAR_DOS_POSTES, modeloDosPostes(), matrizesDosPostes());   /* com sombra, como o Mack */
    sortearHoraDoDia();   /* cada vez que entra no jogo, uma hora do dia */
    definirHoraDaCidade(horaDoDia());
    atualizarLuzesDaCidade(luzDaHora()->luzesDaCidade, 0.0f, 1);   /* já acesas, se for de noite */
    sono = ACORDADO;
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
    /* acabando ou abandonando, a corrida levou um tempo: volta numa outra hora do dia (só desistir na
     * escolha da corrida, voltarDaEscolhaDaCorrida, deixa a hora como estava) */
    sortearHoraDoDia();
    definirHoraDaCidade(horaDoDia());
    atualizarLuzesDaCidade(luzDaHora()->luzesDaCidade, 0.0f, 1);
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
    descarregarInteriores();
    descarregarLuzesDaCidade();
    colocarNoMundoAberto(LUGAR_DOS_POSTES, NULL, NULL);
    descarregarPostes();
    descarregarTratores();
    descarregarGrilos();
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
    cobertura = 0.0f;
    desvioDaCamera = (Vector3){ 0.0f, 0.0f, 0.0f };
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
    if (!pausado && !mapaAberto) {
        /* a cidade segue, até nas cenas do Mack; e o dia passa (dormindo, o relógio pula de uma vez) */
        if (sono == ACORDADO) {
            avancarRelogio(tempoQuadro);
        }
        definirHoraDaCidade(horaDoDia());   /* de noite os moradores ficam em casa */
        atualizarMoradores(malha, carro.x, carro.z, carro.rumo, velocidadeDoCarroLivre(&carro), tempoQuadro);
        atualizarLuzesDaCidade(luzDaHora()->luzesDaCidade, tempoQuadro, 0);
        /* os tratores dormem de noite; o ronco do motor parado (freio de mão e acelerador) os assusta */
        atualizarTratores(luzDaHora()->luzesDaCidade, (Vector3){ carro.x, carro.y, carro.z },
                          (cena == CENA_DIRIGINDO && sono == ACORDADO) ? carro.roncando : 0.0f, tempoQuadro);
    }
    atualizarGrilos(luzDaHora()->luzesDaCidade, !pausado && !mapaAberto && cena == CENA_DIRIGINDO);
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
    if (sono != ACORDADO) {
        atualizarSono(tempoQuadro);   /* escolhendo a hora ou dormindo: o carro fica parado no quarto */
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
    /* No quarto do cone 8, ENTER (ou Y) dorme */
    if (naCamaDoCone() && (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || controleApertou(BOTAO_LIMPAR))) {
        deitar();
        return TELA_PASSEIO;
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
    if (empurrarDoMack() > BATIDA_QUE_ASSUSTA && carro.pancada > fmaxf(pancadaAntes, 0.15f)) {
        sustoDoMack = SEGUNDOS_DE_SUSTO;   /* bateram no Mack: ele se assusta */
    }
    empurrarDosMoradores();
    empurrarDosPostes();
    empurrarDosTratores();
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
 * rumoDaViagem
 * Para onde a câmera vai atrás: para onde o carro está indo (a velocidade, vista de cima), como nos
 * jogos de corrida. No drift, o bico aponta para dentro da curva e o caminho (o rumo da física) vira
 * para fora com o contraesterço; o carro vai entre os dois. Indo atrás do bico ou do rumo, a câmera
 * olhava para o lado errado e o carro escorregava pela tela; indo atrás da velocidade, ela olha o
 * caminho e o carro aparece de lado. Devagar (a velocidade dá pulos) ou de ré, ela vai atrás do rumo.
 */
static float rumoDaViagem(void) {
    float rapidez = hypotf(carro.vx, carro.vz);
    float peso = Clamp((rapidez - CAMINHO_DA_CAMERA_DE) / (CAMINHO_DA_CAMERA_ATE - CAMINHO_DA_CAMERA_DE), 0.0f, 1.0f);
    if (carro.corrida.re > 0.0f || peso <= 0.0f) {
        return carro.rumo;
    }
    float diferenca = atan2f(carro.vx, -carro.vz) - carro.rumo;
    diferenca -= 2.0f * PI * roundf(diferenca / (2.0f * PI));
    if (fabsf(diferenca) > PI * 0.6f) {
        return carro.rumo;   /* a velocidade para trás do rumo: escorregando de ré, rodando */
    }
    return carro.rumo + diferenca * peso;
}

/* Um raio na malha de "de" até "ate" (Devolve: 1 se bateu em alguma coisa no caminho) */
static int raioEntre(Vector3 de, Vector3 ate, Acerto *acerto) {
    Vector3 caminho = Vector3Subtract(ate, de);
    float distancia = Vector3Length(caminho);
    if (distancia < 1e-3f) {
        return 0;
    }
    float origem[3] = { de.x, de.y, de.z };
    float direcao[3] = { caminho.x / distancia, caminho.y / distancia, caminho.z / distancia };
    return raioNaMalha(malha, origem, direcao, distancia, acerto);
}

/*
 * cameraLivre
 * Onde a câmera pode ficar, o mais perto possível de onde ela queria ficar, com o carro à vista: o
 * "braço" do pivô (logo acima do carro) até ela, conferido na malha de colisão. O que estiver no
 * caminho decide, pela direção da face:
 *   - chão (um morro atrás do carro, a beira de uma rampa): ela sobe por cima dele;
 *   - teto (o forro de um prédio, uma marquise, uma ponte): ela desce para baixo dele;
 *   - parede (ou o morro alto demais): ela vem para a frente dela.
 * Só o chão embaixo dela conta como chão: o telhado de cima não (antes a câmera pulava para cima de
 * qualquer teto em que o carro entrasse embaixo). E ela fica a FOLGA_DA_PAREDE das paredes e do teto
 * em volta, senão o plano de perto da imagem os corta e se vê através deles.
 * Recebe: o pivô e onde a câmera queria ficar.
 */
static Vector3 cameraLivre(Vector3 pivo, Vector3 camera) {
    float chao = alturaDoChaoNaMalha(malha, camera.x, camera.z, camera.y, NULL);
    if (chao > SEM_CHAO && camera.y < chao + FOLGA_DA_CAMERA) {
        camera.y = chao + FOLGA_DA_CAMERA;
    }
    for (int tentativa = 0; tentativa < 4; tentativa++) {
        Acerto acerto;
        if (!raioEntre(pivo, camera, &acerto)) {
            break;
        }
        if (acerto.normal[1] > 0.6f) {   /* chão no caminho: por cima dele */
            float topo = alturaDoChaoNaMalha(malha, camera.x, camera.z, camera.y + SOBE_O_MORRO_ATE + FOLGA_DA_CAMERA, NULL);
            float sobe = fmaxf(topo, acerto.ponto[1]) + FOLGA_DA_CAMERA;
            if (sobe > camera.y + 0.01f && sobe < camera.y + SOBE_O_MORRO_ATE) {
                camera.y = sobe;
                continue;
            }
        } else if (acerto.normal[1] < -0.6f) {   /* teto no caminho: por baixo dele */
            float desce = fmaxf(acerto.ponto[1] - FOLGA_DA_PAREDE, pivo.y - 0.3f);
            if (desce < camera.y - 0.01f) {
                camera.y = desce;
                continue;
            }
        }
        /* parede: a câmera vem para a frente dela */
        Vector3 braco = Vector3Subtract(camera, pivo);
        float comprimento = Vector3Length(braco);
        float ate = fmaxf(acerto.distancia - FOLGA_DA_PAREDE, fminf(MENOR_BRACO, comprimento));
        camera = Vector3Add(pivo, Vector3Scale(braco, ate / comprimento));
        break;
    }

    /* Longe das paredes em volta (de lado, o raio do carro até ela não as vê) e do teto em cima */
    float centro[3] = { camera.x, camera.y, camera.z }, cima[3] = { 0.0f, 1.0f, 0.0f }, empurrao[3];
    if (empurrarDasParedes(malha, centro, FOLGA_DA_PAREDE, cima, empurrao)) {
        Vector3 empurrada = { camera.x + empurrao[0], camera.y, camera.z + empurrao[2] };
        Acerto acerto;
        if (!raioEntre(pivo, empurrada, &acerto)) {   /* sem passar para trás de outra parede */
            camera = empurrada;
        }
    }
    Acerto acerto;
    if (raioNaMalha(malha, centro, cima, FOLGA_DA_PAREDE, &acerto)) {
        camera.y = fmaxf(camera.y - (FOLGA_DA_PAREDE - acerto.distancia), pivo.y - 0.3f);
    }
    return camera;
}

/*
 * montarCamera
 * Atrás do carro, para onde ele está indo visto de cima (com um pequeno atraso), girando com
 * o analógico direito, e sempre com o carro à vista (cameraLivre): por cima de um morro, por baixo
 * de um teto, na frente de uma parede. Coberto, ela vem mais perto. Numa parede, ela continua de
 * pé, atrás e um pouco para fora dela.
 */
static Camera3D montarCamera(void) {
    float dt = (pausado || mapaAberto) ? 0.0f : GetFrameTime();
    float passo = fminf(VELOCIDADE_DA_DISTANCIA * dt, 1.0f);
    Vector3 pivo = { carro.x, carro.y + ALTURA_DO_PIVO, carro.z };
    if (dt > 0.0f) {
        float origem[3] = { pivo.x, pivo.y, pivo.z }, paraCima[3] = { 0.0f, 1.0f, 0.0f };
        Acerto acerto;
        float coberto = raioNaMalha(malha, origem, paraCima, COBERTO_ATE, &acerto) ? 1.0f : 0.0f;
        cobertura += (coberto - cobertura) * (1.0f - expf(-dt / ATRASO_DA_COBERTURA));
    }
    Distancia escolhida = DISTANCIAS[distanciaEscolhida];
    Distancia alvo = {
        escolhida.atras + (fminf(DISTANCIA_COBERTA.atras, escolhida.atras) - escolhida.atras) * cobertura,
        escolhida.altura + (fminf(DISTANCIA_COBERTA.altura, escolhida.altura) - escolhida.altura) * cobertura,
    };
    distanciaAtual.atras += (alvo.atras - distanciaAtual.atras) * passo;
    distanciaAtual.altura += (alvo.altura - distanciaAtual.altura) * passo;

    if (dt > 0.0f) {
        float diferenca = rumoDaViagem() - rumoDaCamera;
        diferenca -= 2.0f * PI * roundf(diferenca / (2.0f * PI));
        rumoDaCamera += diferenca * (1.0f - expf(-dt / ATRASO_DA_CAMERA));
        alturaDaCamera += (carro.y - alturaDaCamera) * (1.0f - expf(-dt / ATRASO_DA_ALTURA));
    }
    float giro = rumoDaCamera + olhar.giro * DEG2RAD;
    float dx = sinf(giro), dz = -cosf(giro);

    /* Onde ela queria ficar, e onde pode: chegar perto do carro (ou descer) é na hora, senão ela entra
     * na parede; voltar para o lugar é aos poucos, sem tranco, enquanto o carro continuar à vista */
    Vector3 desejada = { carro.x - dx * distanciaAtual.atras, alturaDaCamera + distanciaAtual.altura,
                         carro.z - dz * distanciaAtual.atras };
    Vector3 livre = cameraLivre(pivo, desejada);
    Vector3 desvio = Vector3Subtract(livre, desejada);
    if (dt > 0.0f) {
        Vector3 suave = Vector3Lerp(desvioDaCamera, desvio, 1.0f - expf(-dt * VOLTA_DA_CAMERA));
        Vector3 comSuave = Vector3Add(desejada, suave);
        Acerto acerto;
        if (Vector3Distance(pivo, comSuave) > Vector3Distance(pivo, livre) + 0.02f || raioEntre(pivo, comSuave, &acerto)) {
            suave = desvio;
        }
        desvioDaCamera = suave;
    }
    Camera3D camera = { 0 };
    camera.position = Vector3Add(desejada, desvioDaCamera);
    /* olhando para o carro de onde ela está (perto de uma parede, ela vem mais para perto e mais para cima dele) */
    float horizontal = fmaxf(hypotf(camera.position.x - carro.x, camera.position.z - carro.z), 1.0f);
    float ladeira = (carro.y + 1.0f - camera.position.y) / horizontal + distanciaAtual.altura / distanciaAtual.atras * 0.55f;
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
static void desenharSombraDoModelo(int modelo, Camera3D camera, Matrix noMundo) {
    Vector3 grade[PONTOS_DA_SOMBRA * PONTOS_DA_SOMBRA];
    float alturas[PONTOS_DA_SOMBRA * PONTOS_DA_SOMBRA];
    float meio = ladoDaSombra(modelo) / 2.0f;
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
    desenharSombraNoChao(modelo, grade);
}

static void desenharSombraDoCarro(Camera3D camera, Matrix noMundo) {
    desenharSombraDoModelo(numero, camera, noMundo);
}

/* Um morador no mundo: de pé no chão, com o bico inclinado na ladeira */
static Matrix matrizDoMorador(const Morador *m) {
    Vector3 frente = { sinf(m->rumo) * cosf(m->inclinacao), sinf(m->inclinacao), -cosf(m->rumo) * cosf(m->inclinacao) };
    Vector3 esquerda = Vector3Normalize((Vector3){ frente.z, 0.0f, -frente.x });   /* +x do modelo */
    Vector3 cima = Vector3CrossProduct(frente, esquerda);
    Matrix noMundo = { 0 };
    noMundo.m0 = esquerda.x; noMundo.m1 = esquerda.y; noMundo.m2 = esquerda.z;
    noMundo.m4 = cima.x;     noMundo.m5 = cima.y;     noMundo.m6 = cima.z;
    noMundo.m8 = frente.x;   noMundo.m9 = frente.y;   noMundo.m10 = frente.z;
    noMundo.m12 = m->x;      noMundo.m13 = m->y;      noMundo.m14 = m->z;
    noMundo.m15 = 1.0f;
    return noMundo;
}

/*
 * prepararLampadas
 * As luzes pontuais deste quadro (as da cidade e as que os interiores jogam para fora): as MAIS_LAMPADAS
 * que mais aparecem para a câmera vão para os shaders do mapa e dos carros.
 */
static void prepararLampadas(Camera3D camera) {
    static Lampada todas[256];
    int total = lampadasAcesas(todas, 256);
    static Lampada escolhidas[MAIS_LAMPADAS];
    int quantas = escolherLampadas(camera, todas, total, escolhidas, MAIS_LAMPADAS);
    aplicarLampadas(escolhidas, quantas);
    ligarVistaDasLampadas();
}

/*
 * desenharMoradores
 * Os moradores que estão perto (e não atrás da câmera): o carro, com as rodas girando, o corpo
 * deitando um pouco na curva e a cara viva (os olhos olham para onde o volante vira; batendo, o
 * susto), e a sombra no chão, se estiverem bem perto.
 */
static void desenharMoradores(Camera3D camera) {
    Vector3 olha = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    for (int i = 0; i < totalDeMoradores(); i++) {
        const Morador *m = morador(i);
        Vector3 onde = { m->x, m->y + 1.0f, m->z };
        Vector3 ate = Vector3Subtract(onde, camera.position);
        float distancia = Vector3Length(ate);
        if (distancia > DESENHA_MORADOR_ATE || (distancia > 20.0f && Vector3DotProduct(ate, olha) < -0.2f * distancia)) {
            continue;
        }
        Matrix noMundo = matrizDoMorador(m);
        if (distancia < SOMBRA_DE_MORADOR_ATE) {
            comecarSombrasNoChao();
            desenharSombraDoModelo(m->modelo, camera, noMundo);
            terminarSombrasNoChao();
        }
        float deita = -m->volante * fminf(fabsf(m->velocidade) / 9.0f, 1.0f) * 0.05f;
        inclinarCorpoNoProximoDesenho(m->modelo, deita, m->aceno);
        expressaoNoProximoDesenho(m->modelo, m->aperto, (m->susto > 0.0f) ? 1.0f : 0.0f);
        if (m->olhaAlgo) {
            olharNoProximoDesenho(m->modelo, m->olhar);   /* estacionado, olhando alguém */
        }
        desenharCarro3D(m->modelo, noMundo, m->anguloDasRodas, m->volante);
    }
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
    /* Os moradores: pontinhos brancos (o xerife atrás do jogador, vermelho e maior) */
    for (int i = 0; i < totalDeMoradores(); i++) {
        const Morador *m = morador(i);
        float dx = (m->x - carro.x) * pixelsPorMetro, dz = (m->z - carro.z) * pixelsPorMetro;
        float c = cosf(-rumoDaCamera), s = sinf(-rumoDaCamera);
        Vector2 p = { dx * c - dz * s, dx * s + dz * c };
        if (m->perseguindo && Vector2Length(p) >= meio - 12.0f) {
            p = Vector2Scale(p, (meio - 12.0f) / Vector2Length(p));   /* na borda, mostrando de onde ele vem */
        }
        if (Vector2Length(p) < meio - 8.0f) {
            float raio = m->perseguindo ? 6.0f : 3.5f;
            DrawCircleV((Vector2){ meio + p.x, meio + p.y }, raio + 1.5f, BLACK);
            DrawCircleV((Vector2){ meio + p.x, meio + p.y }, raio, m->perseguindo ? (Color){ 235, 40, 40, 255 } : (Color){ 235, 235, 235, 255 });
        }
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
/*
 * desenharFalasDosMoradores
 * A fala de um morador, como legenda embaixo: a de quem fala com o jogador de longe (o xerife atrás
 * dele) ou, senão, a de quem está mais perto, se estiver perto o bastante para ouvir. O nome em
 * amarelo, a fala em branco, numa faixa escura; aparece e some aos poucos.
 */
static void desenharFalasDosMoradores(float centroX, float altura) {
    const Morador *quem = NULL;
    float menor = OUVE_ATE;
    for (int i = 0; i < totalDeMoradores(); i++) {
        const Morador *m = morador(i);
        if (m->fala == NULL || m->falaRestante <= 0.0f) {
            continue;
        }
        float d = (m->falaLonge) ? 0.0f : hypotf(m->x - carro.x, m->z - carro.z);
        if (d < menor) {
            menor = d;
            quem = m;
        }
    }
    if (quem == NULL) {
        return;
    }
    char nome[64];
    snprintf(nome, sizeof(nome), "%s:", quem->nome);
    float alfa = Clamp(quem->falaRestante / 0.4f, 0.0f, 1.0f);
    float tamanho = 21.0f;
    float larguraDoNome = larguraDoTexto(FONTE_FORTE, nome, tamanho);
    float larguraTotal = larguraDoNome + 10.0f + larguraDoTexto(FONTE_TEXTO, quem->fala, tamanho) + 40.0f;
    Rectangle caixa = { centroX - larguraTotal / 2.0f, altura - 160.0f, larguraTotal, 40.0f };
    caixaArredondada(caixa, 10.0f, Fade(COR_PAINEL, 0.85f * alfa));
    escrever(FONTE_FORTE, nome, caixa.x + 20.0f, caixa.y + 8.0f, tamanho, Fade(COR_AMARELO, alfa));
    escrever(FONTE_TEXTO, quem->fala, caixa.x + 30.0f + larguraDoNome, caixa.y + 8.0f, tamanho, Fade(WHITE, alfa));
}

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

    /* A hora, em cima do minimapa: um sol ou uma lua e o relógio */
    char textoDaHora[8];
    escreverHora(textoDaHora, (int) sizeof(textoDaHora), horaDoDia());
    float larguraDaHora = larguraDoTexto(FONTE_TITULO, textoDaHora, 18.0f) + 40.0f;
    Rectangle caixaDaHora = { centro.x - larguraDaHora / 2.0f, centro.y - RAIO_DO_MINIMAPA_NA_TELA - 36.0f, larguraDaHora, 26.0f };
    caixaArredondada(caixaDaHora, 8.0f, COR_PAINEL);
    int deNoite = luzDaHora()->luzesDaCidade > 0.5f;
    Vector2 astro = { caixaDaHora.x + 14.0f, caixaDaHora.y + 13.0f };
    DrawCircleV(astro, 6.0f, deNoite ? (Color){ 226, 232, 255, 255 } : COR_AMARELO);
    if (deNoite) {
        DrawCircleV((Vector2){ astro.x + 3.0f, astro.y - 2.0f }, 5.0f, COR_PAINEL);   /* a lua minguante */
    }
    escrever(FONTE_TITULO, textoDaHora, caixaDaHora.x + 26.0f, caixaDaHora.y + 3.0f, 18.0f, WHITE);

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

    /* No quarto do cone 8: o botão de dormir */
    if (naCamaDoCone()) {
        const char *tecla = controleConectado() ? "Y" : "ENTER";
        const char *texto = "DORMIR";
        float larguraDaTecla = larguraDoTexto(FONTE_TITULO, tecla, 18.0f) + 18.0f;
        float total = larguraDaTecla + 12.0f + larguraDoTexto(FONTE_TITULO, texto, 20.0f) + 28.0f;
        Rectangle caixa = { x - total / 2.0f, altura - 92.0f, total, 44.0f };
        caixaArredondada(caixa, 10.0f, COR_PAINEL);
        caixaArredondada((Rectangle){ caixa.x + 14.0f, caixa.y + 9.0f, larguraDaTecla, 26.0f }, 6.0f, COR_DA_NOITE);
        escreverCentralizado(FONTE_TITULO, tecla, caixa.x + 14.0f + larguraDaTecla / 2.0f, caixa.y + 11.0f, 18.0f, WHITE);
        escrever(FONTE_TITULO, texto, caixa.x + 26.0f + larguraDaTecla, caixa.y + 10.0f, 20.0f, WHITE);
    }

    desenharFalasDosMoradores(x, altura);

    /* O resultado da corrida, na volta à cidade */
    if (tempoDoRecado > 0.0f && recado[0] != '\0') {
        float alfa = Clamp(tempoDoRecado / 0.8f, 0.0f, 1.0f);
        float larguraDoRecado = larguraDoTexto(FONTE_TITULO, recado, 24.0f) + 48.0f;
        caixaArredondada((Rectangle){ x - larguraDoRecado / 2.0f, 80.0f, larguraDoRecado, 42.0f }, 10.0f, Fade(COR_PAINEL, alfa));
        escreverCentralizado(FONTE_TITULO, recado, x, 87.0f, 24.0f, Fade(WHITE, alfa));
    }
}

/*
 * desenharSono
 * A tela escurecendo, o "Z z z" com a hora em que acorda (oito horas depois), e a tela clareando.
 */
static void desenharSono(void) {
    float escuro = escuroDoSono();
    if (escuro <= 0.0f) {
        return;
    }
    float largura = (float) larguraVirtual(), altura = (float) alturaVirtual();
    DrawRectangle(-200, -200, (int) largura + 400, (int) altura + 400, Fade(BLACK, escuro));
    float texto = escuro * escuro;
    if (sono == DORMINDO) {
        texto = Clamp(fminf(tempoDoSono / 0.3f, (DORME_POR - tempoDoSono) / 0.3f), 0.0f, 1.0f);
    }
    if (sono != DORMINDO || texto <= 0.0f) {
        return;
    }
    char hora[8], linha[64];
    escreverHora(hora, (int) sizeof(hora), horaDeAcordar);
    snprintf(linha, sizeof(linha), "OITO HORAS DEPOIS  -  %s", hora);
    escreverCentralizado(FONTE_TITULO, "Z  z  z", largura / 2.0f, altura / 2.0f - 46.0f, 46.0f, Fade((Color){ 170, 182, 255, 255 }, texto));
    escreverCentralizado(FONTE_FORTE, linha, largura / 2.0f, altura / 2.0f + 14.0f, 18.0f, Fade(WHITE, texto));
}

/* Um retângulo com uma cor em cada canto (dentro de rlBegin(RL_QUADS)) */
static void quadrado(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color ca, Color cb, Color cc, Color cd) {
    rlColor4ub(ca.r, ca.g, ca.b, ca.a); rlVertex3f(a.x, a.y, a.z);
    rlColor4ub(cb.r, cb.g, cb.b, cb.a); rlVertex3f(b.x, b.y, b.z);
    rlColor4ub(cc.r, cc.g, cc.b, cc.a); rlVertex3f(c.x, c.y, c.z);
    rlColor4ub(cd.r, cd.g, cd.b, cd.a); rlVertex3f(d.x, d.y, d.z);
}

/*
 * desenharBauDoMack
 * O lado de dentro da carreta (carreta.c), sempre que o Mack está perto: o modelo não tem nada dentro
 * do baú (ferramentas/preparar_mack.py tira o que havia lá), então é ele que fecha o baú, visto de
 * qualquer lado (pela porta aberta ou, pelos arcos das rodas, as caixas delas). Com a porta aberta,
 * também o lado de dentro da porta, de chapa de metal, que vira o piso da rampa.
 */
/* As lâmpadas de fora do Mack vão com ele; o que brilha no baú, junto com o lado de dentro (veja abaixo) */
static void moverLuzesDoMack(Vector3 camera) {
    Matrix doMack = matrizDoMack();
    moverLuzes(luzesDoMack, MatrixMultiply(mack.transform, doMack), temMack);
    Matrix deDentro = MatrixMultiply(MatrixTranslate(dobradica.x, dobradica.y + 0.005f, dobradica.z + 0.04f), doMack);
    moverLuzes(luzesDoBau, deDentro, temPortaDoMack && Vector3Distance(camera, (Vector3){ doMack.m12, doMack.m13, doMack.m14 }) < DESENHA_O_BAU_ATE);
}

static void desenharBauDoMack(Vector3 camera) {
    if (!temPortaDoMack) {
        return;
    }
    Matrix mack = matrizDoMack();
    if (Vector3Distance(camera, (Vector3){ mack.m12, mack.m13, mack.m14 }) < DESENHA_O_BAU_ATE) {
        Matrix deDentro = MatrixMultiply(MatrixTranslate(dobradica.x, dobradica.y + 0.005f, dobradica.z + 0.04f), mack);
        desenharInteriorDaCarreta(deDentro);
    }
    if (aberturaDaPorta <= 0.001f) {
        return;
    }
    float x0 = dobradica.x - larguraDaPorta / 2.0f + 0.03f, x1 = dobradica.x + larguraDaPorta / 2.0f - 0.03f;
    float y0 = dobradica.y + 0.01f, y1 = dobradica.y + alturaDaPorta - 0.03f;

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
    moverLuzesDoMack(camera.position);
    soltarPoeiraEFumaca(noMundo, (cena == CENA_DIRIGINDO) ? dt : 0.0f);
    atualizarFumaca(dt);
    atualizarMarcas(dt);
    atualizarMinimapa();

    prepararLuzes(0);
    const LuzDoDia *luzDoDia = luzDaHora();   /* a luz da hora (no fim do quadro volta a do meio-dia) */
    aplicarLuzDoDia(luzDoDia);
    luzDaFumaca(corDaLuz(luzDoDia->base));
    prepararLampadas(camera);
    prepararSombras(CENARIO_DO_MUNDO_ABERTO, camera, configuracoesAtuais()->sombras);
    definirOlhoDosCarros(camera.position);
    comecarPosProcesso();
    prepararPlanosDaCamera();
    BeginMode3D(camera);
    rlDisableBackfaceCulling();
    desenharCenario(CENARIO_DO_MUNDO_ABERTO, camera.position);
    desenharInteriores(camera.position, corDaLuz(luzDoDia->interior), luzDoDia->luzesDaCidade);   /* por dentro */
    desenharLuzesDaCidade(camera, luzDoDia->base);   /* os neons (apagados de dia) */
    if (temOlhosDoMack) {
        prepararOlhosDoModelo(olharDoMack(camera), (cena == CENA_PARTINDO) ? 0.3f : 0.0f, (sustoDoMack > 0.0f) ? 1.0f : 0.0f);
    }
    desenharModelosNoMundoAberto();   /* o Mack, as rodas e a porta */
    desenharBauDoMack(camera.position);
    desenharMarcas();
    desenharMoradores(camera);
    desenharTratores(camera);
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
        /* As luzes de novo, sozinhas: o halo nasce da chama do nitro e dos neons */
        comecarLuzesComProfundidade();   /* o que está na frente (prédios, carros, o Mack) tapa as luzes */
        prepararPlanosDaCamera();
        BeginMode3D(camera);
        rlDisableBackfaceCulling();
        desenharHalosDaCidade(camera);
        desenharChamas(camera, noMundo);
        rlEnableBackfaceCulling();
        EndMode3D();
    }
    float rapidez = Clamp(velocidadeDoCarroLivre(&carro) / carro.velocidadeMaxima, 0.0f, 1.2f);
    float desfoque = efeitos ? 0.35f * rapidez * rapidez + (carro.nitroLigado ? 0.25f : 0.0f) : 0.0f;
    terminarPosProcesso(PISTA_OVAL, desfoque, efeitos ? POSPROCESSO_COMPLETO : POSPROCESSO_SO_COPIA);
    aplicarLuzDoDia(NULL);   /* fora do passeio (as corridas, os menus), a luz de sempre */
    aplicarLampadas(NULL, 0);
    luzDaFumaca(WHITE);

    comecarInterface(0);
    if (cena != CENA_DIRIGINDO) {
        desenharCinema();
    } else if (mapaAberto) {
        desenharMapaGrande();
    } else {
        desenharHudDoPasseio();
        desenharSono();
    }
    terminarInterface();
    if (pausado) {
        comecarInterface(1);
        desenharPausa(menu);
        terminarInterface();
    }
}
