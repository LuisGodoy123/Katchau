/*
 * render.c
 * ------------------------------------------------------------------
 * A visão da corrida, em 3D de verdade, e os carros. Veja render.h.
 *
 *   - O cenário fixo (estádio, asfalto, terreno, objetos, céu) vem de mundo.c.
 *   - A câmera anda pela lista circular da pista: fica atrás do carro,
 *     no ponto da pista CAMERA_ATRAS antes dele, e acima dele.
 *   - Todos os carros são modelos 3D (carros3d.c), colocados no mundo
 *     pela posição deles na lista.
 *   - A fumaça e a poeira são nuvens no mundo (fumaca.c).
 * ------------------------------------------------------------------
 */

#include <math.h>
#include <stddef.h>
#include "raylib.h"
#include "rlgl.h"
#include "render.h"
#include "pitstop.h"
#include "mundo.h"
#include "carros3d.h"
#include "fumaca.h"
#include "noite.h"
#include "raymath.h"

/* ---------- Câmera ---------- */
#define GIRO_DAS_RODAS_DO_GUIDO 18.0f   /* radianos por segundo */
#define ADIANTE_DA_ONDA        0.6f     /* radianos: a onda começa um pouco à frente do jogador */
#define CAMERA_NA_FUGA         0.75f    /* quanto do ângulo de fuga do carro a câmera acompanha */
#define NUMERO_DO_GUIDO        7        /* na tabela FICHAS */
#define ALTURA_NA_TROCA        0.10f    /* metros: o macaco levanta o carro */
#define CAMERA_DA_TROCA_LADO   4.6f     /* a câmera da troca: à direita do carro, na frente e no alto */
#define CAMERA_DA_TROCA_FRENTE 5.2f
#define CAMERA_DA_TROCA_ALTURA 2.8f
#define ALTURA_DA_SOMBRA_NO_CHAO 0.04f  /* metros acima do chão */
#define SOMBRA_PERTO_DA_CAMERA   0.40f  /* metros: quanto ela é puxada na direção da câmera */
#define GRAUS_DE_VISAO        65.0f    /* na vertical */
#define VELOCIDADE_DA_TROCA_DE_CAMERA 7.0f   /* 1/s: a câmera leva ~0,3 s para ir de uma distância à outra */
#define OLHAR_PARA_BAIXO      5.0f     /* graus abaixo do horizonte */
#define FOLGA_DA_CAMERA_NO_MURO 1.0f   /* metros: a câmera livre não encosta no muro */
#define VELOCIDADE_CAMERA_MENU 3000.0f /* a câmera do fundo do menu anda a 75 km/h */
#define SEGUNDOS_POR_PISTA    14.0f    /* o fundo dos menus troca de pista depois disso */
#define DURACAO_DA_TROCA_DE_PISTA 0.7f /* segundos escurecendo e clareando */

/* ---------- Distâncias do que é desenhado (metros) ---------- */
#define DISTANCIA_DOS_RIVAIS  1500.0f
#define MENOR_DISTANCIA_SPRITE 1.5f    /* mais perto que isso da câmera, também não */
#define DISTANCIA_DA_FUMACA   200.0f

/* ---------- Carros no mundo ---------- */
#define ENTRE_EIXOS           0.62f    /* distância entre os eixos, em comprimentos do carro */
#define BITOLA                0.82f    /* distância entre as rodas de um eixo, em larguras do carro */
#define GRAUS_DE_ROLAGEM      2.0f     /* e inclina para fora ao virar */
#define MERGULHO_NA_FREADA    0.025f   /* e abaixa o nariz ao frear */
#define TREPIDACAO_NA_GRAMA   0.06f    /* metros */

/* ---------- Fumaça ---------- */
#define INICIO_DA_FUMACA      0.55f    /* derrapagem abaixo disso não solta fumaça */
#define ANGULO_DA_FUMACA_DO_DRIFT 4.0f   /* graus */
#define ANGULO_DA_FUMACA_CHEIA 30.0f
#define MAIS_NUVENS_NO_DRIFT 1.3f
#define NUVENS_POR_SEGUNDO    55.0f    /* por roda, na derrapagem cheia */
#define VELOCIDADE_DA_ARRANCADA 1600.0f /* abaixo disso (40 km/h), acelerar patina um pouco */
#define ALTURA_DO_ESCAPAMENTO 0.42f    /* metros acima do chão */
#define LUGAR_DO_ESCAPAMENTO  0.22f    /* do meio do carro até cada escapamento, em larguras */
#define ALTURA_DA_NUVEM       0.25f    /* metros acima do chão */

#define LARGURA_CARRO         170      /* tamanho de referência das fotos nos menus */

/*
 * aFrenteDaCamera
 * Recebe: a câmera e um ponto.
 * Devolve: a distância do ponto à frente da câmera (negativa se estiver atrás).
 */
static float aFrenteDaCamera(Camera3D camera, Vector3 ponto) {
    Vector3 frente = { camera.target.x - camera.position.x, camera.target.y - camera.position.y,
                       camera.target.z - camera.position.z };
    float tamanho = sqrtf(frente.x * frente.x + frente.y * frente.y + frente.z * frente.z);
    return ((ponto.x - camera.position.x) * frente.x + (ponto.y - camera.position.y) * frente.y
            + (ponto.z - camera.position.z) * frente.z) / tamanho;
}

static int cenaPausada = 0;
static Olhar olharDoJogador = { 0.0f, 0.0f, 0.0f, 0.0f };

/* As três distâncias da câmera do jogador: perto, médio (a de sempre) e longe */
typedef struct {
    float atras;    /* metros atrás do centro do carro */
    float altura;   /* metros acima do chão */
} DistanciaDaCamera;

#define DISTANCIA_MEDIA 1
static const DistanciaDaCamera DISTANCIAS_DA_CAMERA[TOTAL_DISTANCIAS_DA_CAMERA] = {
    { 5.2f, 2.1f },
    { 8.5f, 3.4f },
    { 13.0f, 5.0f },
};
static int distanciaEscolhida = DISTANCIA_MEDIA;
static DistanciaDaCamera distanciaAtual = { 8.5f, 3.4f };   /* anda até a escolhida, sem pulo */

/*
 * alturaEmVoltaDoCarro
 * A altura do chão num ponto em volta do carro.
 * Recebe: a posição do carro, a lateral dele em metros e quantos metros andar para a
 *         frente e para a direita.
 * Devolve: a altura, em metros.
 */
static float alturaEmVoltaDoCarro(PosicaoNaPista carro, float lateral, float frente, float direita) {
    PosicaoNaPista lugar = carro;
    moverNaPista(&lugar, frente * UNIDADES_POR_METRO);   /* as voltas devolvidas não importam aqui */
    return alturaDoChao(lugar, lateral + direita);
}

/*
 * rumoDesenhado
 * Recebe: o carro e o rumo da pista onde ele está.
 * Devolve: para onde o desenho do carro aponta, em graus, sem contar o volante e o drift.
 */
static float rumoDesenhado(const Carro *carro, float rumoDaPista) {
    if (carro->direcaoLivre && carro->temRumoNoMundo && carro->estadoDoPit == PIT_FORA) {
        return carro->rumoNoMundo;
    }
    return rumoDaPista + carro->anguloDeFuga;
}

/*
 * montarCamera
 * Põe a câmera em volta de um ponto da pista: normalmente atrás dele, seguindo
 * a curva, ou para onde o jogador está olhando. De lado ela fica presa ao carro, que
 * por isso aparece no centro da tela, e não atravessa os muros do oval. Nas curvas
 * inclinadas ela inclina junto com o chão.
 * Recebe: a posição do carro, o deslocamento lateral dele, o olhar, a distância da câmera e,
 *         sem a direção guiada, o rumo real do carro (senão, NULL).
 * Devolve: a câmera.
 */
static Camera3D montarCamera(PosicaoNaPista carro, float lateral, Olhar olhar, DistanciaDaCamera distancia,
                             const float *rumoDoCarro) {
    PosicaoNaPista atras = carro;
    moverNaPista(&atras, -distancia.atras * UNIDADES_POR_METRO);
    PontoNoMundo deCamera = pontoNoMundo(atras, lateral);
    PontoNoMundo doCarro = pontoNoMundo(carro, lateral);

    /* A frente da pista, vista de cima, e a direita dela */
    float dx = doCarro.x - deCamera.x, dz = doCarro.z - deCamera.z;
    float comprimento = hypotf(dx, dz);
    dx /= comprimento;
    dz /= comprimento;
    if (rumoDoCarro != NULL) {
        /* Sem a direção guiada a câmera vem atrás do carro, para onde ele vai: a curva da pista não a gira */
        dx = sinf(*rumoDoCarro * DEG2RAD);
        dz = -cosf(*rumoDoCarro * DEG2RAD);
    }
    float direitaX = -dz, direitaZ = dx;

    /* Para onde a câmera olha, girando em volta do carro */
    float seno = sinf(olhar.giro * DEG2RAD), cosseno = cosf(olhar.giro * DEG2RAD);
    float olhaX = dx * cosseno + direitaX * seno;
    float olhaZ = dz * cosseno + direitaZ * seno;

    /* A câmera fica do outro lado do carro, sem passar do muro */
    float atrasNaPista = comprimento * cosseno;
    float aoLado = -comprimento * seno;
    Segmento *aqui = carro.segmento;
    float emMetros = lateral * larguraDoLado(aqui, lateral);
    if (aqui->temMuro && fabsf(lateral) <= 1.0f) {   /* no pit lane, o muro fica do outro lado */
        float limiteDireita = aqui->larguraDireita - FOLGA_DA_CAMERA_NO_MURO - emMetros;
        float limiteEsquerda = -aqui->larguraEsquerda + FOLGA_DA_CAMERA_NO_MURO - emMetros;
        aoLado = fminf(fmaxf(aoLado, limiteEsquerda), limiteDireita);
    }
    float chao = alturaEmVoltaDoCarro(carro, emMetros, -atrasNaPista, aoLado);
    float ladeira = (doCarro.y - chao) / comprimento;

    Camera3D camera = { 0 };
    camera.position = (Vector3){ doCarro.x - dx * atrasNaPista + direitaX * aoLado, chao + distancia.altura,
                                 doCarro.z - dz * atrasNaPista + direitaZ * aoLado };
    camera.target = (Vector3){ camera.position.x + olhaX,
                               camera.position.y + ladeira + tanf((olhar.elevacao - OLHAR_PARA_BAIXO) * DEG2RAD),
                               camera.position.z + olhaZ };

    /* O "para cima" da câmera é perpendicular ao chão inclinado */
    float rumo = doCarro.rumo * DEG2RAD;
    float inclinacao = atanf(doCarro.inclinacao);
    camera.up = (Vector3){ -cosf(rumo) * sinf(inclinacao), cosf(inclinacao), -sinf(rumo) * sinf(inclinacao) };
    camera.fovy = GRAUS_DE_VISAO;
    camera.projection = CAMERA_PERSPECTIVE;
    return camera;
}

/*
 * matrizDoCarro
 * Assenta o carro no chão: mede a altura do asfalto embaixo das quatro
 * rodas e, com elas, acha a altura, a ladeira e a inclinação do carro. Em
 * cima disso, ele aponta para onde o volante manda, inclina um pouco para
 * fora ao virar e abaixa o nariz ao frear.
 * Recebe: o carro.
 * Devolve: a matriz que leva o carro normalizado até lá.
 */
static Matrix matrizDoCarro(Carro *carro) {
    PontoNoMundo aqui = pontoNoMundo(carro->posicao, carro->deslocamentoLateral);
    float meiaDistancia = comprimentoDoCarro3D(carro->numero) * ENTRE_EIXOS / 2.0f;
    float meiaBitola = larguraDoCarro3D(carro->numero) * BITOLA / 2.0f;
    float lateral = carro->deslocamentoLateral * larguraDoLado(carro->posicao.segmento, carro->deslocamentoLateral);

    PosicaoNaPista eixoDaFrente = carro->posicao, eixoDeTras = carro->posicao;
    moverNaPista(&eixoDaFrente, meiaDistancia * UNIDADES_POR_METRO);
    moverNaPista(&eixoDeTras, -meiaDistancia * UNIDADES_POR_METRO);
    float frenteEsquerda = alturaDoChao(eixoDaFrente, lateral - meiaBitola);
    float frenteDireita = alturaDoChao(eixoDaFrente, lateral + meiaBitola);
    float trasEsquerda = alturaDoChao(eixoDeTras, lateral - meiaBitola);
    float trasDireita = alturaDoChao(eixoDeTras, lateral + meiaBitola);

    float ladeira = ((frenteEsquerda + frenteDireita) - (trasEsquerda + trasDireita)) / (4.0f * meiaDistancia);
    float inclinacao = ((frenteDireita + trasDireita) - (frenteEsquerda + trasEsquerda)) / (4.0f * meiaBitola);
    float y = (frenteEsquerda + frenteDireita + trasEsquerda + trasDireita) / 4.0f;

    if (carro->ultimosComandos.frear && carro->velocidade > 0.0f) {
        ladeira -= MERGULHO_NA_FREADA;
    }
    if (estaForaDaPista(carro) && carro->velocidade > 0.0f) {
        y += TREPIDACAO_NA_GRAMA * sinf(carro->distanciaPercorrida * 0.05f);
    }
    if (carro->estadoDoPit == PIT_TROCANDO) {
        float tempo = fminf(carro->tempoDaTroca, carro->duracaoDaTroca - carro->tempoDaTroca);
        y += ALTURA_NA_TROCA * fminf(tempo / SEGUNDOS_PARA_A_RODA, 1.0f);
    }

    /* Sem a direção guiada, o bico aponta para onde o carro está indo de verdade */
    float rumoBase = rumoDesenhado(carro, aqui.rumo);
    float rumo = (rumoBase + carro->giro + carro->anguloDeDrift + carro->volante * GRAUS_DO_VOLANTE) * DEG2RAD;
    Vector3 frente = Vector3Normalize((Vector3){ sinf(rumo), ladeira, -cosf(rumo) });

    float rolagem = atanf(inclinacao) + carro->volante * GRAUS_DE_ROLAGEM * DEG2RAD;
    Vector3 cima = { -cosf(rumo) * sinf(rolagem), cosf(rolagem), -sinf(rumo) * sinf(rolagem) };
    Vector3 lado = Vector3Normalize(Vector3CrossProduct(cima, frente));   /* a esquerda do carro é +x no modelo */
    cima = Vector3CrossProduct(frente, lado);

    Matrix m = { 0 };
    m.m0 = lado.x;   m.m1 = lado.y;   m.m2 = lado.z;
    m.m4 = cima.x;   m.m5 = cima.y;   m.m6 = cima.z;
    m.m8 = frente.x; m.m9 = frente.y; m.m10 = frente.z;
    m.m12 = aqui.x;  m.m13 = y;       m.m14 = aqui.z;
    m.m15 = 1.0f;
    return m;
}

/*
 * desenharCarroNoMundo
 * Recebe: a câmera e o carro.
 * Devolve: nada (carro atrás da câmera ou longe demais não é desenhado).
 */
/*
 * desenharSombraDoCarro
 * Põe cada ponto da sombra em cima do chão daquele lugar, sem contar a inclinação do
 * carro (freada, volante, trepidação). Depois puxa o ponto para perto da câmera: na
 * tela ele fica no mesmo lugar, mas não some por pouco atrás do asfalto ou da pintura.
 * Recebe: a câmera e o carro.
 * Devolve: nada.
 */
static void desenharSombraDoCarro(Camera3D camera, Carro *carro) {
    Vector3 grade[PONTOS_DA_SOMBRA * PONTOS_DA_SOMBRA];
    float meio = ladoDaSombra(carro->numero) / 2.0f;
    float lateral = carro->deslocamentoLateral * larguraDoLado(carro->posicao.segmento, carro->deslocamentoLateral);
    float rumoDaPista = pontoNoMundo(carro->posicao, carro->deslocamentoLateral).rumo;
    float giro = (rumoDesenhado(carro, rumoDaPista) - rumoDaPista + carro->giro + carro->anguloDeDrift + carro->volante * GRAUS_DO_VOLANTE) * DEG2RAD;
    for (int i = 0; i < PONTOS_DA_SOMBRA; i++) {
        for (int j = 0; j < PONTOS_DA_SOMBRA; j++) {
            float paraDireita = -meio + 2.0f * meio * i / (PONTOS_DA_SOMBRA - 1);
            float paraFrente = meio - 2.0f * meio * j / (PONTOS_DA_SOMBRA - 1);
            float naPista = paraFrente * cosf(giro) - paraDireita * sinf(giro);
            float aoLado = lateral + paraDireita * cosf(giro) + paraFrente * sinf(giro);

            PosicaoNaPista lugar = carro->posicao;
            moverNaPista(&lugar, naPista * UNIDADES_POR_METRO);
            PontoNoMundo chao = pontoNoMundo(lugar, aoLado / larguraDoLado(lugar.segmento, aoLado));
            Vector3 ponto = { chao.x, chao.y + ALTURA_DA_SOMBRA_NO_CHAO, chao.z };
            Vector3 paraCamera = Vector3Normalize(Vector3Subtract(camera.position, ponto));
            grade[i * PONTOS_DA_SOMBRA + j] = Vector3Add(ponto, Vector3Scale(paraCamera, SOMBRA_PERTO_DA_CAMERA));
        }
    }
    desenharSombraNoChao(carro->numero, grade);
}

static void desenharCarroNoMundo(Camera3D camera, Carro *carro) {
    PontoNoMundo ponto = pontoNoMundo(carro->posicao, carro->deslocamentoLateral);
    float distancia = aFrenteDaCamera(camera, (Vector3){ ponto.x, ponto.y, ponto.z });
    if (distancia < -6.0f || distancia > DISTANCIA_DOS_RIVAIS) {
        return;
    }
    desenharSombraDoCarro(camera, carro);
    if (carro->estadoDoPit == PIT_TROCANDO && carro->duracaoDaTroca == DURACAO_DA_TROCA) {
        int canto;
        float andamento;
        if (faseDaTroca(carro, &canto, &andamento)) {
            Vector3 centro;
            trocarRodaNoProximoDesenho(rodaDoCanto(carro->numero, canto, &centro), andamento);
        }
    }
    desenharCarro3D(carro->numero, matrizDoCarro(carro), carro->anguloDasRodas, carro->volante);
}

/*
 * desenharChamasDoCarro
 * O nitro: uma chama azul em cada escapamento, atrás do carro.
 * Recebe: a câmera e o carro.
 * Devolve: nada.
 */
/*
 * desenharGuido
 * O Guido fica na vaga do jogador esperando. Na troca ele dispara de roda em roda,
 * contornando o carro pela traseira e pela frente (o caminho está em pitstop.c).
 * Recebe: a câmera e o jogador.
 * Devolve: nada.
 */
static void desenharGuido(Camera3D camera, Pista *pista, Carro *jogador) {
    /* A vaga, como se o carro já estivesse parado nela */
    Carro naVaga = *jogador;
    lugarDaVaga(pista, jogador, &naVaga.posicao, &naVaga.deslocamentoLateral);
    naVaga.volante = 0.0f;
    naVaga.giro = 0.0f;
    naVaga.velocidade = 0.0f;
    naVaga.estadoDoPit = PIT_FORA;
    naVaga.ultimosComandos.frear = 0;
    PontoNoMundo ponto = pontoNoMundo(naVaga.posicao, naVaga.deslocamentoLateral);
    float distancia = aFrenteDaCamera(camera, (Vector3){ ponto.x, ponto.y, ponto.z });
    if (distancia < -6.0f || distancia > DISTANCIA_DOS_RIVAIS) {
        return;
    }
    Matrix vaga = matrizDoCarro(&naVaga);

    Vector3 frente = { 0 }, tras = { 0 };
    rodaDoCanto(jogador->numero, 0, &frente);
    rodaDoCanto(jogador->numero, 1, &tras);
    LugarDoGuido lugar = lugarDoGuido(jogador, larguraDoCarro3D(jogador->numero) / 2.0f, comprimentoDoCarro3D(jogador->numero) / 2.0f, frente.z, tras.z);
    float rodas = (jogador->estadoDoPit == PIT_TROCANDO) ? jogador->tempoDaTroca * GIRO_DAS_RODAS_DO_GUIDO : 0.0f;
    Matrix noCarro = MatrixMultiply(MatrixRotateY(lugar.olha), MatrixTranslate(lugar.x, 0.0f, lugar.z));
    Matrix noMundo = MatrixMultiply(noCarro, vaga);
    desenharSombra(NUMERO_DO_GUIDO, noMundo);
    desenharCarro3D(NUMERO_DO_GUIDO, noMundo, rodas, 0.0f);
}

static void desenharChamasDoCarro(Camera3D camera, Carro *carro) {
    if (!carro->nitroLigado) {
        return;
    }
    PontoNoMundo ponto = pontoNoMundo(carro->posicao, carro->deslocamentoLateral);
    float distancia = aFrenteDaCamera(camera, (Vector3){ ponto.x, ponto.y, ponto.z });
    if (distancia < -8.0f || distancia > DISTANCIA_DA_FUMACA) {
        return;
    }
    Matrix noMundo = matrizDoCarro(carro);
    Vector3 paraTras = { -noMundo.m8, -noMundo.m9, -noMundo.m10 };
    float atras = comprimentoDoCarro3D(carro->numero) / 2.0f;
    float meiaDistancia = larguraDoCarro3D(carro->numero) * LUGAR_DO_ESCAPAMENTO;
    for (int lado = -1; lado <= 1; lado += 2) {
        Vector3 escape = Vector3Transform((Vector3){ lado * meiaDistancia, ALTURA_DO_ESCAPAMENTO, -atras }, noMundo);
        desenharChamaDoNitro(camera, escape, paraTras);
    }
}

/*
 * soltarFumacaDoCarro
 * Solta as nuvens nas rodas de trás, de acordo com o que o carro está fazendo:
 * poeira na grama, fumaça na derrapagem e na arrancada forte. O número de
 * nuvens depende do tempo que passou, não de quantos quadros o jogo desenha.
 * Recebe: a câmera, o carro e o tempo que passou na corrida.
 * Devolve: nada.
 */
static void soltarFumacaDoCarro(Camera3D camera, Carro *carro, float tempoQuadro) {
    PontoNoMundo ponto = pontoNoMundo(carro->posicao, carro->deslocamentoLateral);
    float distancia = aFrenteDaCamera(camera, (Vector3){ ponto.x, ponto.y, ponto.z });
    if (distancia < -12.0f || distancia > DISTANCIA_DA_FUMACA || carro->velocidade <= 0.0f) {
        return;
    }

    float forca = 0.0f;
    TipoDeNuvem tipo = NUVEM_DE_PNEU;
    if (estaForaDaPista(carro)) {
        forca = 0.9f;
        tipo = NUVEM_DE_POEIRA;
    } else if (carro->rodando) {
        forca = 1.0f;
    } else if (carro->forcaDoDrift > 0.05f && fabsf(carro->anguloDeDrift) > ANGULO_DA_FUMACA_DO_DRIFT) {
        /* De lado, os pneus de trás queimam de verdade: quanto mais de lado, mais fumaça */
        forca = 0.65f + 0.35f * fminf(fabsf(carro->anguloDeDrift) / ANGULO_DA_FUMACA_CHEIA, 1.0f);
        tipo = NUVEM_DE_DRIFT;
    } else if (carro->derrapagem > INICIO_DA_FUMACA) {
        forca = (carro->derrapagem - INICIO_DA_FUMACA) / (1.0f - INICIO_DA_FUMACA);
    } else if (carro->ultimosComandos.acelerar && carro->velocidade < VELOCIDADE_DA_ARRANCADA) {
        forca = 0.55f * (1.0f - carro->velocidade / VELOCIDADE_DA_ARRANCADA);
    }
    if (forca <= 0.0f) {
        return;
    }

    float metrosPorSegundo = carro->velocidade / UNIDADES_POR_METRO;
    float rumo = ponto.rumo * DEG2RAD;
    Vector3 velocidade = { sinf(rumo) * metrosPorSegundo, 0.0f, -cosf(rumo) * metrosPorSegundo };

    /* Quantas nuvens neste quadro: a parte fracionária é sorteada */
    float quantas = NUVENS_POR_SEGUNDO * tempoQuadro * (0.4f + 0.6f * forca);
    if (tipo == NUVEM_DE_DRIFT) {
        quantas *= MAIS_NUVENS_NO_DRIFT;
    }
    int inteiras = (int) quantas;
    if (GetRandomValue(0, 1000) / 1000.0f < quantas - inteiras) {
        inteiras++;
    }

    Matrix noMundo = matrizDoCarro(carro);
    float meiaBitola = larguraDoCarro3D(carro->numero) * BITOLA / 2.0f;
    float eixoDeTras = -comprimentoDoCarro3D(carro->numero) * ENTRE_EIXOS / 2.0f;
    for (int i = 0; i < inteiras; i++) {
        for (int lado = -1; lado <= 1; lado += 2) {
            /* Sorteia onde, ao longo do caminho do quadro, ela nasce: senão as nuvens saem em fila */
            float recuo = -metrosPorSegundo * tempoQuadro * GetRandomValue(0, 1000) / 1000.0f;
            Vector3 roda = Vector3Transform((Vector3){ lado * meiaBitola, ALTURA_DA_NUVEM, eixoDeTras + recuo }, noMundo);
            soltarNuvem(roda, velocidade, forca, tipo);
        }
    }
}

/*
 * congelarCena
 * Na pausa, os flashes e a torcida param junto com a corrida.
 * Recebe: 1 se o jogo está pausado.
 * Devolve: nada.
 */
void congelarCena(int pausada) {
    cenaPausada = pausada;
}

/*
 * definirOlhar
 * Recebe: para onde o jogador está olhando (o main.c lê o analógico).
 * Devolve: nada.
 */
void definirOlhar(Olhar olhar) {
    olharDoJogador = olhar;
}

/*
 * puxarOndaDaTorcida
 * Na metade da corrida, quando o líder chega lá, a torcida da Copa Pistão faz a onda com
 * os faróis. Ela começa um pouco à frente do jogador, para ele ver a onda vir e dar a volta.
 * Recebe: a corrida.
 * Devolve: nada.
 */
static void puxarOndaDaTorcida(const Corrida *corrida) {
    static float relogioAnterior = 0.0f;
    static int jaFez = 0;
    if (corrida->tempoDeCorrida < relogioAnterior) {
        jaFez = 0;   /* corrida nova */
    }
    relogioAnterior = corrida->tempoDeCorrida;
    int metade = (corrida->totalVoltas + 1) / 2;
    if (!jaFez && corrida->classificacao[0] != NULL && corrida->classificacao[0]->voltas >= metade) {
        jaFez = 1;
        PontoNoMundo jogador = pontoNoMundo(corrida->jogador.posicao, corrida->jogador.deslocamentoLateral);
        comecarOndaDaTorcida(anguloNoEstadio(jogador.x, jogador.z) + ADIANTE_DA_ONDA);
    }
}

/*
 * desenharMundo3D
 * O cenário e os carros, vistos pela câmera.
 * Recebe: a câmera, a pista e a corrida (NULL nos menus: sem carros).
 * Devolve: nada.
 */
static void desenharMundo3D(Camera3D camera, Pista *pista, Corrida *corrida) {
    int ehNoite = (pista->id == PISTA_COPA_PISTAO);
    if (ehNoite && corrida != NULL) {
        puxarOndaDaTorcida(corrida);
    }
    animarTorcida(cenaPausada ? 0.0f : GetFrameTime());
    if (ehNoite) {
        atualizarNoite(cenaPausada ? 0.0f : GetFrameTime());
    }
    prepararLuzes(ehNoite);

    prepararPlanosDaCamera();
    BeginMode3D(camera);
    rlDisableBackfaceCulling();
    desenharCenario(pista->id, camera.position);

    if (corrida != NULL) {
        for (int r = 0; r < TOTAL_RIVAIS; r++) {
            desenharCarroNoMundo(camera, &corrida->rivais[r]);
        }
        desenharCarroNoMundo(camera, &corrida->jogador);
        if (corrida->temParada) {
            desenharGuido(camera, pista, &corrida->jogador);
        }
        desenharFumaca3D(camera);
        for (int r = 0; r < TOTAL_RIVAIS; r++) {
            desenharChamasDoCarro(camera, &corrida->rivais[r]);
        }
        desenharChamasDoCarro(camera, &corrida->jogador);
    }
    if (ehNoite) {
        desenharBrilhos(camera);
    }

    rlEnableBackfaceCulling();
    EndMode3D();
}

/*
 * cameraDaTroca
 * Na troca de pneus, a imagem corta para a frente do carro, do lado da pista, para ver
 * o Guido passando por todas as rodas. No fim, corta de volta para trás do carro.
 * Recebe: a câmera normal e o jogador.
 * Devolve: a câmera.
 */
static Camera3D cameraDaTroca(Camera3D normal, Carro *carro) {
    Matrix m = matrizDoCarro(carro);
    Vector3 centro = { m.m12, m.m13, m.m14 };
    Vector3 lado = { m.m0, 0.0f, m.m2 }, frente = { m.m8, 0.0f, m.m10 };
    Camera3D camera = normal;
    camera.position = Vector3Add(Vector3Add(centro, Vector3Scale(lado, -CAMERA_DA_TROCA_LADO)), Vector3Scale(frente, CAMERA_DA_TROCA_FRENTE));
    camera.position.y += CAMERA_DA_TROCA_ALTURA;
    camera.target = (Vector3){ centro.x, centro.y + 0.4f, centro.z };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    return camera;
}

/*
 * alternarDistanciaDaCamera
 * Passa a câmera da corrida para a próxima distância: perto, médio, longe e de volta
 * ao perto. A escolha vale também nas próximas corridas.
 * Recebe: nada.
 * Devolve: a distância escolhida (0 = perto, 1 = médio, 2 = longe).
 */
int alternarDistanciaDaCamera(void) {
    distanciaEscolhida = (distanciaEscolhida + 1) % TOTAL_DISTANCIAS_DA_CAMERA;
    return distanciaEscolhida;
}

/*
 * desenharVisaoDoJogador
 * Desenha o que o jogador vê: o mundo pela câmera atrás do carro.
 * Recebe: a corrida.
 * Devolve: nada.
 */
void desenharVisaoDoJogador(Corrida *corrida) {
    Carro *jogador = &corrida->jogador;
    /* A câmera acompanha o carro, não a pista: sem volante na curva, ela também não vira sozinha */
    Olhar olhar = olharDoJogador;
    int comRumoReal = jogador->direcaoLivre && jogador->temRumoNoMundo && jogador->estadoDoPit == PIT_FORA;
    if (!comRumoReal) {
        olhar.giro += jogador->anguloDeFuga * CAMERA_NA_FUGA;
    }
    /* A distância anda aos poucos até a escolhida (perto, médio ou longe) */
    const DistanciaDaCamera *alvo = &DISTANCIAS_DA_CAMERA[distanciaEscolhida];
    float passo = fminf(VELOCIDADE_DA_TROCA_DE_CAMERA * GetFrameTime(), 1.0f);
    distanciaAtual.atras += (alvo->atras - distanciaAtual.atras) * passo;
    distanciaAtual.altura += (alvo->altura - distanciaAtual.altura) * passo;
    Camera3D camera = montarCamera(jogador->posicao, jogador->deslocamentoLateral, olhar, distanciaAtual,
                                   comRumoReal ? &jogador->rumoNoMundo : NULL);
    if (jogador->estadoDoPit == PIT_TROCANDO) {
        camera = cameraDaTroca(camera, jogador);
    }

    /* A fumaça anda com o relógio da corrida: na pausa e na contagem, ele para.
     * Se ele voltou atrás, é uma corrida nova. */
    static float relogioAnterior = 0.0f;
    float passou = corrida->tempoDeCorrida - relogioAnterior;
    relogioAnterior = corrida->tempoDeCorrida;
    if (passou < 0.0f) {
        limparFumaca();
        passou = 0.0f;
    }
    if (passou > 0.0f) {
        for (int r = 0; r < TOTAL_RIVAIS; r++) {
            soltarFumacaDoCarro(camera, &corrida->rivais[r], passou);
        }
        soltarFumacaDoCarro(camera, jogador, passou);
    }
    atualizarFumaca(passou);

    desenharMundo3D(camera, corrida->pista, corrida);
}

/*
 * desenharPistaDeFundo
 * Fundo das telas de menu: a câmera passeia sozinha pela pista, devagar,
 * pela linha dela. Ela anda pela lista com moverNaPista(), então dá voltas
 * infinitas no circuito. Cada pista lembra onde o passeio parou.
 * Recebe: a pista.
 * Devolve: nada.
 */
void desenharPistaDeFundo(Pista *pista) {
    static PosicaoNaPista passeios[TOTAL_PISTAS];
    PosicaoNaPista *passeio = &passeios[pista->id];
    if (passeio->segmento == NULL) {
        passeio->segmento = pista->largada;
    }
    moverNaPista(passeio, VELOCIDADE_CAMERA_MENU * GetFrameTime());
    desenharMundo3D(montarCamera(*passeio, 0.0f, olharParaAFrente(), DISTANCIAS_DA_CAMERA[DISTANCIA_MEDIA], NULL),
                    pista, NULL);
}

/*
 * desenharFundoAlternado
 * O fundo dos menus: mostra cada pista por um tempo, trocando com um
 * escurecimento rápido para a troca não ser um salto.
 * Recebe: as pistas, na ordem de IdPista.
 * Devolve: nada.
 */
void desenharFundoAlternado(Pista *pistas[]) {
    static float relogio = 0.0f;
    relogio += GetFrameTime();
    int qual = (int) (relogio / SEGUNDOS_POR_PISTA) % TOTAL_PISTAS;
    float noTurno = fmodf(relogio, SEGUNDOS_POR_PISTA);

    desenharPistaDeFundo(pistas[qual]);

    float escuro = 1.0f - fminf(fminf(noTurno, SEGUNDOS_POR_PISTA - noTurno) / DURACAO_DA_TROCA_DE_PISTA, 1.0f);
    if (escuro > 0.0f) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, escuro));
    }
}

/*
 * desenharCarroParado
 * A miniatura de um carro, para a garagem e o menu.
 * Recebe: o número do carro, o centro da base na tela e a escala.
 * Devolve: nada.
 */
void desenharCarroParado(int numero, float centroX, float baseY, float escala) {
    desenharMiniatura(numero, centroX, baseY, LARGURA_CARRO * escala);
}
