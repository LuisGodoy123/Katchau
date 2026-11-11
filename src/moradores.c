/*
 * moradores.c
 * ------------------------------------------------------------------
 * Os moradores de Radiator Springs vivendo na cidade. Veja moradores.h.
 *
 * O mapa (os lugares, o grafo das ruas, os percursos, os programas e as
 * falas) está em moradores_mapa.h, nos eixos do jogo. Cada lugar tem a
 * vaga do dono (um nó do grafo) e, ao lado dela, vagas para visitas, se a
 * malha de colisão disser que cabem.
 *
 * A vida de cada um:
 *   - em casa, espera um tempo sorteado e escolhe um dos programas dele
 *     (sorteado pelo peso de cada um);
 *   - um programa com convidado: se o convidado está em casa, passa lá
 *     primeiro, conversam um pouco e vão juntos (o convidado sai logo
 *     atrás); quando o programa acaba, cada um volta para a sua casa;
 *   - o Guido acompanha sempre o Luigi: sai atrás dele e volta com ele;
 *   - acabado um programa, às vezes emenda outro antes de voltar.
 * As manobras nas pontas de cada ida:
 *   - vaga de ré (o carro estacionado olha para a rua): chega de frente até
 *     a entrada, para e entra de ré, virando até o rumo da vaga; na saída,
 *     sai de frente;
 *   - vaga de frente (a Flo, olhando o balcão): entra de frente e sai de ré.
 * Andando, ele mira um ponto um pouco à frente no caminho, vira o volante
 * até lá (com o raio mínimo de um carro), anda devagar nas curvas, freia
 * atrás de quem vai na frente para o mesmo lado e, saindo para a rua,
 * espera quem vem chegando passar.
 *
 * Estacionados, os que estão perto um do outro conversam: olham um para o
 * outro e falam cada um na sua vez (o que vem por último o jogador lê, se
 * estiver perto). Com o jogador: quem está estacionado olha para ele
 * quando ele para por perto e cumprimenta; quem ele passa rápido rente se
 * assusta e reclama; o xerife, no esconderijo, vai atrás de quem passa
 * correndo e, se o pega, dá uma bronca.
 * ------------------------------------------------------------------
 */

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "moradores.h"
#include "corrida.h"

#define PI_M                  3.14159265f
#define VELOCIDADE_NA_CIDADE  9.0f      /* m/s (uns 32 km/h no velocímetro) */
#define VELOCIDADE_NO_PATIO   3.5f      /* nas entradas e nos pátios */
#define VELOCIDADE_DE_RE      2.2f
#define ACELERA               2.5f      /* m/s² */
#define FREIA                 6.0f
#define RAIO_MINIMO           6.0f      /* metros: o menor raio de curva */
#define OLHA_A_FRENTE         7.0f      /* metros: ele mira entre o ponto do caminho e o seguinte (rápido, mais longe) */
#define RAIO_DE_MANOBRA       2.5f      /* metros: chegando no fim do caminho, devagar, ele vira mais fechado */
#define MANOBRA_PERTO         10.0f     /* metros do fim do caminho em que ele manobra */
#define RODEANDO_NO_MAXIMO    6.0f      /* segundos rodeando o fim do caminho: estaciona dali mesmo */
#define VELOCIDADE_NO_TREINO  22.0f     /* m/s no oval da pista de treino (uns 80 km/h) */
#define ENTRADA_NAO_E_RETORNO 300.0f    /* metros a mais no caminho para quem usaria a entrada de alguém como retorno */
#define CHEGOU_NO_PONTO       3.5f      /* metros: passa para o próximo ponto */
#define CHEGOU_NA_VAGA        0.8f
#define DISTANCIA_NA_FILA     6.0f      /* metros de folga atrás de quem está na frente */
#define LARGURA_DA_FAIXA      2.4f      /* metros para cada lado: quem está nessa faixa está "na frente" */
#define BUZINA_DEPOIS         3.0f      /* segundos parado atrás do jogador */
#define CONTORNA_O_JOGADOR    10.0f     /* segundos parado atrás do jogador parado: vai em frente (e o empurra)... */
#define CONTORNA_POR          3.0f      /* ...por tantos segundos */
#define DESISTE_DE_ESPERAR    5.0f      /* segundos parado atrás de outro morador: passa assim mesmo... */
#define IGNORA_POR            4.0f      /* ...por tantos segundos (o outro sai da frente, empurrado) */
#define MESMO_SENTIDO         0.35f     /* cosseno: um morador só segura o de trás se andam para o mesmo lado */
#define ALTURA_LIVRE          0.6f      /* metros: acima do chão da vaga, mais que isso é prédio */
#define DE_RE                 1.75f     /* radianos (100°): mais que isso entre o bico e o caminho, é de ré */
#define MAIS_VAGAS            4
#define VAGA_LONGE_DE_OUTRA   3.4f      /* metros: uma vaga de visita mais perto que isso de outra vaga não entra */
#define RE_NA_SAIDA           4.0f      /* metros além do meio carro: quanto recua quem sai de ré */
#define GIRO_NA_SAIDA         1.0f      /* radianos: o quanto ele gira recuando */
#define MAIS_NOS              256
#define MAIS_PROGRAMAS        8
#define TETO_DA_CIDADE        4.5f      /* metros: o chão da cidade fica abaixo disso (até uns 3 m, o piso dos cones
                                         * do Cozy Cone) e as marquises e os tetos, acima: procurar daqui para baixo */
#define NINGUEM               (-1)
#define SEM_PERCURSO          (-1)

/* A vida na cidade */
#define CHANCE_DE_EMENDAR     0.3f      /* acabado um programa, a chance de ir a outro antes de voltar para casa */
#define NOITINHA              19.5f     /* horas: daí em diante, só o café da Flo (aberto de noite)... */
#define RECOLHER              22.0f     /* ...e daí até o amanhecer, todo mundo em casa */
#define AMANHECE              5.5f
#define BUSCAR_FICA_DE        8.0f      /* segundos de conversa quando um passa para buscar o outro */
#define BUSCAR_FICA_ATE       14.0f
#define SAI_ATRAS_DE          1.4f      /* segundos: quem vai junto sai tanto depois do outro */
#define JUNTO_ATE             30.0f     /* metros: mais longe que isso, quem acompanha ficou para trás */
#define TENTA_DE_NOVO         12.0f     /* segundos: sem vaga em lugar nenhum, espera e tenta de novo */
#define CONVERSA_ATE          16.0f     /* metros: dois estacionados mais perto que isso conversam */
#define TURNO_DE                2.8f    /* segundos de cada fala na conversa */
#define TURNO_ATE             4.6f
#define CHANCE_DE_PAUSA       0.45f     /* depois de cada fala, a chance de uma pausa na conversa... */
#define PAUSA_DE              2.0f      /* ...de tantos segundos */
#define PAUSA_ATE             7.0f
#define CHANCE_DA_FALA_PROPRIA 0.6f     /* uma fala solta: a chance de ser das da pessoa (senão, das de todo mundo) */
#define OLHA_QUEM_PASSA_ATE   14.0f     /* metros: estacionado, acompanha com os olhos quem passa */
#define OLHA_O_JOGADOR_ATE    13.0f     /* metros: estacionado, olha o jogador que chega devagar */
#define CUMPRIMENTA_ATE       11.0f     /* metros: o jogador parado mais perto que isso ganha um cumprimento */
#define CUMPRIMENTA_DEPOIS    0.8f      /* segundos parado perto */
#define CUMPRIMENTA_DE_NOVO   40.0f     /* segundos até cumprimentar o mesmo jogador de novo */
#define PARADO_ATE            1.5f      /* m/s: o jogador "parado" */
#define DEVAGAR_ATE           6.0f      /* m/s: o jogador "devagar" (ele olha) */
#define SUSTO_DE_PASSAGEM     17.0f     /* m/s: passando mais rápido que isso... */
#define SUSTO_ATE             7.5f      /* ...a menos que isso de um morador, ele se assusta */
#define RECLAMA_DE_NOVO       10.0f     /* segundos */
#define DURA_A_FALA           3.8f
#define CANTO_DO_OLHO         1.05f     /* radianos (60°): daí para o lado, os olhos estão no canto */
#define CEDE_ATE              24.0f     /* metros: saindo para a rua, espera quem vem a menos disso */
#define CEDE_PERTO            7.0f      /* metros do ponto em que entra na rua: daí ele para e olha */
#define CEDE_NO_MAXIMO        8.0f      /* segundos esperando: depois, entra assim mesmo */
/* O xerife */
#define LIMITE_DO_XERIFE      25.0f     /* m/s (90 km/h): mais rápido que isso perto do esconderijo, ele vai atrás */
#define XERIFE_VE_ATE         80.0f     /* metros */
#define VELOCIDADE_DA_PERSEGUICAO 30.0f
#define ACELERA_NA_PERSEGUICAO 7.0f
#define FICA_ATRAS            10.0f     /* metros: o xerife para tanto atrás do jogador */
#define PEGOU_ATE             14.0f     /* metros: o jogador parado a menos disso foi pego... */
#define PEGOU_DEPOIS          1.2f      /* ...depois de tantos segundos parado */
#define FUGIU_ALEM            180.0f    /* metros */
#define PERSEGUE_NO_MAXIMO    35.0f     /* segundos */
#define LONGE_DE_CASA         700.0f    /* metros: mais longe que isso do esconderijo, o xerife desiste */
#define BRONCA_DURA           4.5f
#define XERIFE_DESCANSA       60.0f     /* segundos depois de uma perseguição até a próxima */

/* As marcas de um ponto do caminho */
#define ENTRA_NA_RUA          1         /* chegando neste ponto, ele entra numa rua: espera a vez */
#define NO_TREINO             2         /* um ponto do oval da pista de treino: ali se acelera */

/* O que o morador está fazendo */
enum { PARADO, SAINDO_DE_RE, ANDANDO, ENTRANDO_DE_RE, PERSEGUINDO, DANDO_BRONCA };

/* As fases de um programa */
enum { SEM_PROGRAMA, INDO_BUSCAR, NA_CASA_DO_CONVIDADO, INDO_AO_PROGRAMA, NO_PROGRAMA };

/* ------------------------------------------------------------------
 * Os figurantes (os modelos que não correm)
 * ------------------------------------------------------------------ */
static const FichaDeFigurante FIGURANTES[TOTAL_DE_FIGURANTES] = {
    { "flo.glb",    "Flo",    5.6f, 2.25f, 0 },
    { "ruivo.glb",  "Ruivo",  7.5f, 3.10f, 0 },
    { "xerife.glb", "Xerife", 5.0f, 2.30f, 0 },
    { "lizzie.glb", "Lizzie", 3.4f, 1.95f, 0 },
};

const FichaDeFigurante *fichaDoFigurante(int qual) {
    return &FIGURANTES[qual];
}

const char *nomeDoModelo(int modelo) {
    return (modelo < TOTAL_CARROS) ? fichaDoCarro(modelo)->nome : FIGURANTES[modelo - TOTAL_CARROS].nome;
}

void medidasDoModelo(int modelo, float *comprimento, float *largura) {
    if (modelo < TOTAL_CARROS) {
        *comprimento = fichaDoCarro(modelo)->comprimento;
        *largura = fichaDoCarro(modelo)->largura;
    } else {
        *comprimento = FIGURANTES[modelo - TOTAL_CARROS].comprimento;
        *largura = FIGURANTES[modelo - TOTAL_CARROS].largura;
    }
}

/* ------------------------------------------------------------------
 * O mapa
 * ------------------------------------------------------------------ */
typedef struct {
    const char *nome;
    const char *vaga;        /* o nó do grafo da vaga do dono */
    float rumo;              /* para onde o carro aponta, estacionado */
    const char *visitas;     /* o nó das vagas de visita, quando não cabem ao lado da do dono (o McQueen fica
                              * dentro do cone; quem vem buscá-lo para na praça). NULL: ao lado da do dono */
    float rumoDasVisitas;    /* para onde apontam as de visita, em "visitas" */
    unsigned char lados;     /* quais das vagas de visita (LADOS, um bit cada) servem; 0: todas. Dentro de um
                              * prédio, só as que têm o caminho livre até elas (a ré não desvia de nada) */
} Lugar;

typedef struct {
    const char *id;
    float x, z;
    const char *vizinhos[6];  /* para onde se vai deste nó */
} NoDaRua;

typedef struct {
    int lugar;               /* para onde vai (e estaciona) */
    int percurso;            /* um passeio antes de chegar (SEM_PERCURSO: o caminho mais curto) */
    int convidado;           /* a pessoa que ele passa para buscar e vai junto (NINGUEM) */
    float peso;              /* a chance de sair para este programa, perto dos outros (0: fim da lista) */
    float de, ate;           /* segundos que fica lá */
} Programa;

typedef struct {
    const char *nome;
    int modelo;
    int casa;
    float emCasaDe, emCasaAte;     /* segundos em casa entre um programa e outro */
    float comecaEsperando;         /* segundos em casa antes da primeira saída (as rotinas não começam juntas) */
    float pressa;                  /* a velocidade na cidade é VELOCIDADE_NA_CIDADE vezes isto */
    int segue;                     /* a pessoa que ele acompanha sempre (NINGUEM) */
    Programa programas[MAIS_PROGRAMAS];
    const char *aoJogador[3];      /* o que diz ao jogador que para perto (NULL no fim) */
    const char *conversa[4];       /* o que diz conversando com qualquer um (NULL no fim) */
} Pessoa;

typedef struct {
    int a, b;                      /* as duas pessoas */
    const char *falas[8];          /* alternadas, a primeira de "a" (NULL no fim) */
} Conversa;

#include "moradores_mapa.h"

#define TOTAL_DE_LUGARES   (int) (sizeof(LUGARES) / sizeof(LUGARES[0]))
#define TOTAL_DE_NOS       (int) (sizeof(RUAS) / sizeof(RUAS[0]))
#define TOTAL_DE_PESSOAS   (int) (sizeof(PESSOAS) / sizeof(PESSOAS[0]))
#define TOTAL_DE_PERCURSOS (int) (sizeof(PERCURSOS) / sizeof(PERCURSOS[0]))
#define TOTAL_DE_CONVERSAS (int) (sizeof(CONVERSAS) / sizeof(CONVERSAS[0]))
#define TOTAL_DE_SUSTOS    (int) (sizeof(SUSTOS) / sizeof(SUSTOS[0]))
#define TOTAL_DE_FALAS_DE_SEMPRE (int) (sizeof(FALAS_DE_SEMPRE) / sizeof(FALAS_DE_SEMPRE[0]))

/* ------------------------------------------------------------------
 * O estado
 * ------------------------------------------------------------------ */
typedef struct {
    float x, z, rumo;
    int ocupada;             /* 1 + o número do morador que está nela ou indo para ela (0 = livre) */
    int no;                  /* o nó do grafo de onde se chega a ela: o da vaga do dono ou o das visitas */
} Vaga;

typedef struct {
    int lugarAtual, vagaAtual;     /* onde ele está (ou para onde vai) */
    int estado;
    float reDeX, reDeZ, reDeRumo;  /* a manobra de ré: de onde... */
    float reAteX, reAteZ, reAteRumo;   /* ...até onde */
    float reAndou, reTotal;
    int reNaChegada;               /* 1: o fim do caminho é entrar de ré */
    float ignora;                  /* segundos em que ele não espera outro morador (cansou de esperar) */
    float ignoraJogador;           /* segundos em que ele não espera o jogador parado na frente */
    unsigned char marcas[MAIS_PONTOS_NO_CAMINHO];   /* de cada ponto: ENTRA_NA_RUA, NO_TREINO */
    float cedendo;                 /* segundos esperando para entrar na rua */
    float rodeando;                /* segundos perto do fim do caminho sem chegar */
    unsigned int sorteio;

    /* O programa */
    int programa;                  /* o programa de agora (NINGUEM: nenhum) */
    int fase;
    int convidado;                 /* o morador que ele chamou (NINGUEM) */
    int lider;                     /* o morador que ele acompanha agora (NINGUEM) */
    int sempreJunto;               /* 1: acompanha sempre (o Guido com o Luigi) */
    int indoLugar, indoPercurso;   /* a ida marcada para quando a espera acabar (indoLugar < 0: nenhuma) */

    /* A conversa e o jogador */
    int parceiro;                  /* com quem conversa agora (NINGUEM) */
    float turno;                   /* segundos até a próxima fala da conversa */
    int linha;                     /* a próxima fala da conversa do par */
    int calado;                    /* 1: na conversa, agora é uma pausa */
    unsigned int falaPropria, falaDeSempre;   /* o rodízio das falas soltas */
    float jogadorPerto;            /* segundos com o jogador parado perto */
    float cumprimentou;            /* segundos até poder cumprimentar de novo */
    float reclamou;                /* segundos até poder reclamar de novo */
    float fase2;                   /* do aceno */

    /* O xerife */
    float perseguiu, pegando, descanso;
} Rotina;

static Morador moradores[MAIS_MORADORES];
static Rotina rotinas[MAIS_MORADORES];
static int total = 0;
static int moradorDaPessoa[TOTAL_DE_PESSOAS];
static Vaga vagas[TOTAL_DE_LUGARES][MAIS_VAGAS];
static float horaDaCidade = 12.0f;   /* a hora do dia (o passeio avisa): de noite, cada um fica em casa */
static int totalDeVagas[TOTAL_DE_LUGARES];
static int noDaVaga[TOTAL_DE_LUGARES];
static int vizinhos[MAIS_NOS][6];
static int totalDeVizinhos[MAIS_NOS];
static int percursos[TOTAL_DE_PERCURSOS][8];
static int totalNosPercursos[TOTAL_DE_PERCURSOS];

/* O jogador, neste passo */
static struct {
    float x, z, rumo, velocidade;
} jogador;

int totalDeMoradores(void) {
    return total;
}

Morador *morador(int i) {
    return &moradores[i];
}

static float diferencaDeRumo(float alvo, float atual) {
    float d = fmodf(alvo - atual + PI_M, 2.0f * PI_M);
    if (d < 0.0f) {
        d += 2.0f * PI_M;
    }
    return d - PI_M;
}

static float limitar(float v, float a, float b) {
    return (v < a) ? a : (v > b) ? b : v;
}

static float suave(float t) {
    t = limitar(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

static float rumoDe(float dx, float dz) {
    return atan2f(dx, -dz);
}

static float distanciaEntre(const Morador *a, float x, float z) {
    return hypotf(a->x - x, a->z - z);
}

/* Um número entre a e b, dos sorteios só deste morador (não mexe no rand() do jogo) */
static float sortear(Rotina *r, float a, float b) {
    r->sorteio = r->sorteio * 1664525u + 1013904223u;
    return a + (b - a) * (float) (r->sorteio >> 8) / 16777216.0f;
}

static const Pessoa *pessoaDe(int quem) {
    return &PESSOAS[moradores[quem].pessoa];
}

static int emCasa(int quem) {
    return rotinas[quem].lugarAtual == pessoaDe(quem)->casa;
}

/* Para onde ir com os olhos, de -1 (todo à esquerda) a +1 (todo à direita), olhando um ponto */
static float olharPara(const Morador *m, float x, float z) {
    float dx = x - m->x, dz = z - m->z;
    float frente = dx * sinf(m->rumo) - dz * cosf(m->rumo);
    float direita = dx * cosf(m->rumo) + dz * sinf(m->rumo);
    return limitar(atan2f(direita, frente) / CANTO_DO_OLHO, -1.0f, 1.0f);
}

static void dizer(Morador *m, const char *fala, float segundos, int longe) {
    if (fala == NULL) {
        return;
    }
    m->fala = fala;
    m->falaRestante = segundos;
    m->falaLonge = longe;
}

static int noPeloNome(const char *id) {
    for (int i = 0; i < TOTAL_DE_NOS; i++) {
        if (strcmp(RUAS[i].id, id) == 0) {
            return i;
        }
    }
    return -1;
}

/* Um nó das ruas de verdade (as faixas da rua principal e da rodovia), não de um pátio ou retorno */
static int eRua(int no) {
    const char *id = RUAS[no].id;
    return ((id[0] == 'L' || id[0] == 'O') && isdigit((unsigned char) id[1]))
           || (id[0] == 'R' && (id[1] == 'L' || id[1] == 'O'));
}

static int eEntrada(int no) {
    return strncmp(RUAS[no].id, "entrada_", 8) == 0;
}

static int eTreino(int no) {
    return strncmp(RUAS[no].id, "OV_", 3) == 0;
}

/* A vaga cabe ali? Nada alto (prédio, poste, muro) sobre o chão dela, que é plano */
static int vagaLivre(MalhaDeColisao *malha, float x, float z, float rumo, float comprimento, float largura) {
    float fx = sinf(rumo), fz = -cosf(rumo), dx = cosf(rumo), dz = sinf(rumo);
    float meio = alturaDoChaoNaMalha(malha, x, z, TETO_DA_CIDADE, NULL);
    if (meio <= SEM_CHAO) {
        return 0;
    }
    for (int i = -2; i <= 2; i++) {
        for (int j = -1; j <= 1; j++) {
            float px = x + fx * comprimento * 0.25f * i + dx * largura * 0.5f * j;
            float pz = z + fz * comprimento * 0.25f * i + dz * largura * 0.5f * j;
            float h = alturaDoChaoNaMalha(malha, px, pz, meio + 3.0f, NULL);
            if (h <= SEM_CHAO || fabsf(h - meio) > ALTURA_LIVRE) {
                return 0;
            }
        }
    }
    /* E nenhuma parede atravessando a vaga: dentro de um prédio (a Casa Della Tires) não há telhado nem
     * tampo que o chão acuse. Raios pelas bordas e pelas diagonais do retângulo, a 1 m do chão */
    float cantos[4][2];
    for (int k = 0; k < 4; k++) {
        float frente = (k == 0 || k == 3) ? 0.5f : -0.5f, lado = (k < 2) ? 0.5f : -0.5f;
        cantos[k][0] = x + fx * comprimento * frente + dx * largura * lado;
        cantos[k][1] = z + fz * comprimento * frente + dz * largura * lado;
    }
    static const int BORDAS[6][2] = { { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 }, { 0, 2 }, { 1, 3 } };
    for (int k = 0; k < 6; k++) {
        const float *a = cantos[BORDAS[k][0]], *b = cantos[BORDAS[k][1]];
        float comprimentoDaBorda = hypotf(b[0] - a[0], b[1] - a[1]);
        float origem[3] = { a[0], meio + 1.0f, a[1] };
        float direcao[3] = { (b[0] - a[0]) / comprimentoDaBorda, 0.0f, (b[1] - a[1]) / comprimentoDaBorda };
        Acerto acerto;
        if (raioNaMalha(malha, origem, direcao, comprimentoDaBorda, &acerto)) {
            return 0;
        }
    }
    return 1;
}

/* As vagas de cada lugar: a do dono e, ao lado dela (ou do nó das visitas do lugar), outras para visitas, se
 * couberem */
/* Uma vaga nova encosta em alguma das que já existem (de qualquer lugar)? (a da Casa Della Tires ficava no
 * caminho da ré do Guido, que estaciona do lado do Luigi) */
static int pertoDeOutraVaga(float x, float z) {
    for (int l = 0; l < TOTAL_DE_LUGARES; l++) {
        for (int v = 0; v < totalDeVagas[l]; v++) {
            if (hypotf(vagas[l][v].x - x, vagas[l][v].z - z) < VAGA_LONGE_DE_OUTRA) {
                return 1;
            }
        }
    }
    return 0;
}

static void montarVagas(MalhaDeColisao *malha) {
    /* Primeiro as vagas dos donos, de todos os lugares; depois as de visita, onde couberem */
    for (int l = 0; l < TOTAL_DE_LUGARES; l++) {
        int no = noPeloNome(LUGARES[l].vaga);
        noDaVaga[l] = no;
        totalDeVagas[l] = 0;
        if (no < 0) {
            fprintf(stderr, "MORADORES: o nó %s não existe\n", LUGARES[l].vaga);
            continue;
        }
        vagas[l][totalDeVagas[l]++] = (Vaga){ RUAS[no].x, RUAS[no].z, LUGARES[l].rumo, 0, no };
    }
    for (int l = 0; l < TOTAL_DE_LUGARES; l++) {
        if (noDaVaga[l] < 0) {
            continue;
        }
        int no = noDaVaga[l];
        float rumo = LUGARES[l].rumo;
        if (LUGARES[l].visitas != NULL) {
            no = noPeloNome(LUGARES[l].visitas);
            rumo = LUGARES[l].rumoDasVisitas;
            if (no < 0) {
                fprintf(stderr, "MORADORES: o nó %s não existe\n", LUGARES[l].visitas);
                continue;
            }
        }
        float x = RUAS[no].x, z = RUAS[no].z, dx = cosf(rumo), dz = sinf(rumo);
        static const float LADOS[] = { 3.6f, -3.6f, 7.2f, -7.2f };
        for (int k = 0; k < 4 && totalDeVagas[l] < MAIS_VAGAS; k++) {
            if (LUGARES[l].lados != 0 && !(LUGARES[l].lados & (1u << k))) {
                continue;
            }
            float vx = x + dx * LADOS[k], vz = z + dz * LADOS[k];
            if (!pertoDeOutraVaga(vx, vz) && vagaLivre(malha, vx, vz, rumo, 5.8f, 2.6f)) {
                vagas[l][totalDeVagas[l]++] = (Vaga){ vx, vz, rumo, 0, no };
            }
        }
    }
}

/* Reserva uma vaga livre do lugar para o morador (-1 se não há). A do dono, só para o dono */
static int reservarVaga(int lugar, int quem, int dono) {
    for (int v = dono ? 0 : 1; v < totalDeVagas[lugar]; v++) {
        if (vagas[lugar][v].ocupada == 0) {
            vagas[lugar][v].ocupada = quem + 1;
            return v;
        }
    }
    return -1;
}

/* O caminho mais curto no grafo (Dijkstra; são poucos nós). Devolve quantos nós pôs em "saida" */
static int caminhoNasRuas(int de, int ate, int *saida, int cabe) {
    static float distancia[MAIS_NOS];
    static int anterior[MAIS_NOS], fechado[MAIS_NOS];
    for (int i = 0; i < TOTAL_DE_NOS; i++) {
        distancia[i] = 1e18f;
        anterior[i] = -1;
        fechado[i] = 0;
    }
    distancia[de] = 0.0f;
    for (;;) {
        int u = -1;
        for (int i = 0; i < TOTAL_DE_NOS; i++) {
            if (!fechado[i] && distancia[i] < 1e17f && (u < 0 || distancia[i] < distancia[u])) {
                u = i;
            }
        }
        if (u < 0 || u == ate) {
            break;
        }
        fechado[u] = 1;
        for (int k = 0; k < totalDeVizinhos[u]; k++) {
            int v = vizinhos[u][k];
            float d = distancia[u] + hypotf(RUAS[v].x - RUAS[u].x, RUAS[v].z - RUAS[u].z);
            if (eEntrada(u) && anterior[u] >= 0 && eRua(anterior[u]) && eRua(v)) {
                d += ENTRADA_NAO_E_RETORNO;   /* da rua para a rua pela entrada de alguém: um retorno no quintal alheio */
            }
            if (d < distancia[v]) {
                distancia[v] = d;
                anterior[v] = u;
            }
        }
    }
    if (distancia[ate] > 1e17f) {
        return 0;
    }
    int aoContrario[MAIS_NOS], n = 0;
    for (int v = ate; v >= 0 && n < MAIS_NOS; v = anterior[v]) {
        aoContrario[n++] = v;
    }
    if (n > cabe) {
        return 0;
    }
    for (int i = 0; i < n; i++) {
        saida[i] = aoContrario[n - 1 - i];
    }
    return n;
}

/*
 * nosDaIda
 * Os nós de uma ida: de "de" até "ate", passando antes pelos nós do percurso, em ordem (cada trecho, o
 * caminho mais curto). Devolve quantos nós pôs em "saida" (0 se algum trecho não tem caminho ou não cabe).
 */
static int nosDaIda(int de, int percurso, int ate, int *saida, int cabe) {
    int pontos[10], n = 0;
    pontos[n++] = de;
    if (percurso >= 0) {
        for (int k = 0; k < totalNosPercursos[percurso]; k++) {
            pontos[n++] = percursos[percurso][k];
        }
    }
    pontos[n++] = ate;
    int juntos = 0;
    for (int k = 0; k + 1 < n; k++) {
        int trecho[MAIS_NOS];
        int t = caminhoNasRuas(pontos[k], pontos[k + 1], trecho, MAIS_NOS);
        if (t < 1 || juntos + t > cabe) {
            return 0;
        }
        for (int i = (juntos > 0) ? 1 : 0; i < t; i++) {   /* a emenda de um trecho com o outro, uma vez só */
            saida[juntos++] = trecho[i];
        }
    }
    return juntos;
}

static void porNoCaminho(Morador *m, Rotina *r, float x, float z, int marcas) {
    if (m->totalDoCaminho < MAIS_PONTOS_NO_CAMINHO) {
        m->caminho[m->totalDoCaminho][0] = x;
        m->caminho[m->totalDoCaminho][1] = z;
        r->marcas[m->totalDoCaminho] = (unsigned char) marcas;
        m->totalDoCaminho++;
    }
}

static void comecarRe(Rotina *r, const Morador *m, float ateX, float ateZ, float ateRumo) {
    r->reDeX = m->x; r->reDeZ = m->z; r->reDeRumo = m->rumo;
    r->reAteX = ateX; r->reAteZ = ateZ; r->reAteRumo = ateRumo;
    r->reAndou = 0.0f;
    r->reTotal = fmaxf(hypotf(ateX - m->x, ateZ - m->z), 1.0f);
}

/*
 * montarCaminho
 * Os pontos do caminho, a partir dos nós da ida (do primeiro, o nó da vaga de onde sai ou o nó mais perto,
 * até o nó da vaga de chegada: o da vaga do dono ou o das visitas do lugar), e a manobra de saída. Nas vagas
 * de visita (ao lado do nó), um ponto a mais na frente (ou atrás) da vaga, no fundo da entrada: ele entra e
 * sai em linha reta, sem passar pela frente do carro do dono.
 * Recebe: a vaga de onde sai (NULL: ele não está numa vaga, e o primeiro nó entra no caminho) e o número
 *         dela no lugar, e a vaga de chegada.
 */
static void montarCaminho(Morador *m, Rotina *r, const int *nos, int n, const Vaga *saida, int vagaDaSaida,
                          int lugar, int v) {
    const Vaga *vaga = &vagas[lugar][v];
    m->totalDoCaminho = 0;
    int primeiroDoMeio = (saida != NULL) ? 1 : 0;
    if (saida != NULL && vagaDaSaida > 0) {
        const NoDaRua *dono = &RUAS[saida->no], *primeiro = &RUAS[nos[1]];
        float fx = sinf(saida->rumo), fz = -cosf(saida->rumo);
        float fundo = (primeiro->x - dono->x) * fx + (primeiro->z - dono->z) * fz;
        if (fundo > 0.0f) {   /* estacionado de ré, olhando a entrada: sai de frente, reto */
            porNoCaminho(m, r, saida->x + fx * fundo, saida->z + fz * fundo, 0);
        }
    }
    for (int i = primeiroDoMeio; i < n - 1; i++) {
        porNoCaminho(m, r, RUAS[nos[i]].x, RUAS[nos[i]].z,
                     ((!eRua(nos[i]) && eRua(nos[i + 1])) ? ENTRA_NA_RUA : 0) | (eTreino(nos[i]) ? NO_TREINO : 0));
    }
    if (v > 0 && n >= 2) {
        const NoDaRua *dono = &RUAS[vaga->no], *ultimo = &RUAS[nos[n - 2]];
        float fx = sinf(vaga->rumo), fz = -cosf(vaga->rumo);
        float fundo = (ultimo->x - dono->x) * fx + (ultimo->z - dono->z) * fz;
        porNoCaminho(m, r, vaga->x + fx * fundo, vaga->z + fz * fundo, 0);
    }
    porNoCaminho(m, r, vaga->x, vaga->z, 0);
    m->proximo = 0;
    m->rumoDaChegada = vaga->rumo;
    m->andando = 1;
    m->bloqueado = 0.0f;
    r->ignora = 0.0f;
    r->cedendo = 0.0f;
    r->rodeando = 0.0f;
    /* Chegando: se a vaga olha ao contrário de quem chega pelo último trecho, ele entra de ré */
    if (m->totalDoCaminho >= 2) {
        float ax = m->caminho[m->totalDoCaminho - 2][0], az = m->caminho[m->totalDoCaminho - 2][1];
        r->reNaChegada = fabsf(diferencaDeRumo(vaga->rumo, rumoDe(vaga->x - ax, vaga->z - az))) > DE_RE;
    } else {
        r->reNaChegada = 0;
    }
    /* Saindo de uma vaga: se o bico aponta ao contrário do primeiro ponto, ele sai de ré: recua uns metros
     * em linha reta, girando um pouco para o lado do caminho, e daí segue de frente */
    float px = m->caminho[0][0], pz = m->caminho[0][1];
    if (saida != NULL && m->totalDoCaminho > 2
        && fabsf(diferencaDeRumo(rumoDe(px - m->x, pz - m->z), m->rumo)) > DE_RE) {
        float recua = fminf(m->comprimento / 2.0f + RE_NA_SAIDA, hypotf(px - m->x, pz - m->z));
        float atrasX = m->x - sinf(m->rumo) * recua, atrasZ = m->z + cosf(m->rumo) * recua;
        /* virando para o caminho: de ré, a traseira vai para o lado contrário de para onde o bico vai depois */
        float paraOCaminho = diferencaDeRumo(rumoDe(px - atrasX, pz - atrasZ), m->rumo);
        comecarRe(r, m, atrasX, atrasZ, m->rumo + limitar(paraOCaminho, -GIRO_NA_SAIDA, GIRO_NA_SAIDA));
        r->estado = SAINDO_DE_RE;
        m->proximo = 0;
    } else {
        r->estado = ANDANDO;
    }
}

/* Uma vaga para ele no lugar: a mesma, se volta para onde está (a ronda); senão, uma livre (-1: nenhuma) */
static int vagaPara(int quem, int lugar) {
    Rotina *r = &rotinas[quem];
    if (lugar == r->lugarAtual) {
        return r->vagaAtual;
    }
    return reservarVaga(lugar, quem, lugar == pessoaDe(quem)->casa);
}

/*
 * partir
 * Sai da vaga onde está para uma vaga do lugar, passando pelo percurso. Devolve 1 se conseguiu (sem
 * vaga ou sem caminho, fica onde está e devolve 0).
 */
static int partir(int quem, int lugar, int percurso) {
    Morador *m = &moradores[quem];
    Rotina *r = &rotinas[quem];
    if (lugar < 0 || noDaVaga[lugar] < 0 || noDaVaga[r->lugarAtual] < 0
        || (lugar == r->lugarAtual && percurso == SEM_PERCURSO)) {
        return 0;
    }
    int v = vagaPara(quem, lugar);
    if (v < 0) {
        return 0;
    }
    static int nos[MAIS_PONTOS_NO_CAMINHO];
    int de = vagas[r->lugarAtual][r->vagaAtual].no, ate = vagas[lugar][v].no;
    int n = nosDaIda(de, percurso, ate, nos, MAIS_PONTOS_NO_CAMINHO - 4);
    if (n < 3 && percurso != SEM_PERCURSO) {
        n = nosDaIda(de, SEM_PERCURSO, ate, nos, MAIS_PONTOS_NO_CAMINHO - 4);
    }
    if (n < 3) {
        if (lugar != r->lugarAtual) {
            vagas[lugar][v].ocupada = 0;
        }
        return 0;
    }
    int lugarDaSaida = r->lugarAtual, vagaDaSaida = r->vagaAtual;
    Vaga saida = vagas[lugarDaSaida][vagaDaSaida];
    if (lugar != lugarDaSaida) {
        vagas[lugarDaSaida][vagaDaSaida].ocupada = 0;   /* a vaga de onde ele sai fica livre */
    }
    r->lugarAtual = lugar;
    r->vagaAtual = v;
    montarCaminho(m, r, nos, n, &saida, vagaDaSaida, lugar, v);
    return 1;
}

/*
 * voltarDeOndeEsta
 * Fora de qualquer vaga (o xerife, depois de uma perseguição): do nó mais perto (de preferência na frente
 * dele e na mão certa) até a vaga dele no lugar.
 */
static int voltarDeOndeEsta(int quem, int lugar) {
    Morador *m = &moradores[quem];
    Rotina *r = &rotinas[quem];
    float fx = sinf(m->rumo), fz = -cosf(m->rumo);
    int melhor = -1;
    float menor = 1e18f;
    for (int i = 0; i < TOTAL_DE_NOS; i++) {
        if (strncmp(RUAS[i].id, "vaga_", 5) == 0) {
            continue;   /* sai por uma rua ou entrada, não pela vaga de alguém (nem pela dele) */
        }
        float dx = RUAS[i].x - m->x, dz = RUAS[i].z - m->z;
        float custo = hypotf(dx, dz);
        if (dx * fx + dz * fz < 0.0f) {
            custo += 15.0f;   /* atrás dele: teria de dar a volta */
        }
        if (totalDeVizinhos[i] > 0) {
            int s = vizinhos[i][0];
            if ((RUAS[s].x - RUAS[i].x) * fx + (RUAS[s].z - RUAS[i].z) * fz < 0.0f) {
                custo += 20.0f;   /* a mão da faixa é a contrária */
            }
        }
        if (custo < menor) {
            menor = custo;
            melhor = i;
        }
    }
    int v = vagaPara(quem, lugar);
    if (melhor < 0 || v < 0) {
        return 0;
    }
    static int nos[MAIS_PONTOS_NO_CAMINHO];
    int n = nosDaIda(melhor, SEM_PERCURSO, vagas[lugar][v].no, nos, MAIS_PONTOS_NO_CAMINHO - 4);
    if (n < 1) {
        return 0;
    }
    if (lugar != r->lugarAtual) {
        vagas[r->lugarAtual][r->vagaAtual].ocupada = 0;
    }
    r->lugarAtual = lugar;
    r->vagaAtual = v;
    montarCaminho(m, r, nos, n, NULL, 0, lugar, v);
    return 1;
}

/* Quem o acompanha (o Guido, o convidado) sai logo atrás dele: para onde ele vai ou, quando ele volta
 * para casa ou o programa acabou, para a casa de quem acompanha */
static void chamarQuemAcompanha(int lider, int lugar, int percurso) {
    Rotina *rl = &rotinas[lider];
    int casaDoLider = pessoaDe(lider)->casa;
    for (int k = 0; k < total; k++) {
        Rotina *r = &rotinas[k];
        if (k == lider || r->lider != lider) {
            continue;
        }
        if (r->estado != PARADO || distanciaEntre(&moradores[k], moradores[lider].x, moradores[lider].z) > JUNTO_ATE) {
            if (!r->sempreJunto) {
                r->lider = NINGUEM;   /* ficou para trás: segue a vida dele */
            }
            continue;
        }
        int casa = pessoaDe(k)->casa;
        if (!r->sempreJunto && lugar == casaDoLider && percurso == SEM_PERCURSO) {
            /* o programa acabou (o líder volta para casa): o convidado volta para a dele, sozinho */
            r->lider = NINGUEM;
            r->indoLugar = casa;
            r->indoPercurso = SEM_PERCURSO;
            if (rl->convidado == k) {
                rl->convidado = NINGUEM;
            }
        } else {
            r->indoLugar = (lugar == casaDoLider) ? casa : lugar;
            r->indoPercurso = percurso;
        }
        moradores[k].espera = SAI_ATRAS_DE;
    }
}

/* Sai para uma ida, levando quem o acompanha. Devolve 1 se saiu */
static int sairPara(int quem, int lugar, int percurso) {
    if (!partir(quem, lugar, percurso)) {
        return 0;
    }
    chamarQuemAcompanha(quem, lugar, percurso);
    return 1;
}

/* O convidado está livre para ir junto? (em casa, estacionado, sem programa nem companhia) */
static int livreParaIr(int quem) {
    if (quem == NINGUEM) {
        return 0;
    }
    Rotina *r = &rotinas[quem];
    return r->estado == PARADO && emCasa(quem) && r->lider == NINGUEM && r->convidado == NINGUEM
           && r->fase == SEM_PROGRAMA && r->indoLugar < 0 && moradores[quem].espera > 2.0f;
}

/* O programa combina com a hora? De noitinha só o café da Flo; tarde da noite, nenhum */
static int programaNaHora(const Programa *g) {
    if (horaDaCidade >= RECOLHER || horaDaCidade < AMANHECE) {
        return 0;
    }
    if (horaDaCidade >= NOITINHA) {
        return g->lugar == LUGAR_FLO && g->percurso == SEM_PERCURSO;
    }
    return 1;
}

void definirHoraDaCidade(float hora) {
    horaDaCidade = hora;
}

/*
 * escolherPrograma
 * Sorteia um programa (pelo peso), que não seja ficar onde está. Com convidado livre em outro lugar,
 * vai buscá-lo antes; no mesmo lugar, vai junto direto. Devolve 1 se saiu.
 */
static int escolherPrograma(int quem) {
    Rotina *r = &rotinas[quem];
    const Pessoa *p = pessoaDe(quem);
    float soma = 0.0f;
    int possiveis[MAIS_PROGRAMAS], n = 0;
    for (int i = 0; i < MAIS_PROGRAMAS && p->programas[i].peso > 0.0f; i++) {
        const Programa *g = &p->programas[i];
        if ((g->lugar == r->lugarAtual && g->percurso == SEM_PERCURSO) || noDaVaga[g->lugar] < 0) {
            continue;
        }
        if (g->lugar == r->lugarAtual && !emCasa(quem)) {
            continue;   /* a ronda e o treino saem de casa */
        }
        if (!programaNaHora(g)) {
            continue;   /* de noite, cada um em casa (ou no café da Flo, de noitinha) */
        }
        possiveis[n++] = i;
        soma += g->peso;
    }
    for (int tentativa = 0; tentativa < 3 && n > 0; tentativa++) {
        float s = sortear(r, 0.0f, soma);
        int escolhido = possiveis[n - 1];
        for (int k = 0; k < n; k++) {
            s -= p->programas[possiveis[k]].peso;
            if (s <= 0.0f) {
                escolhido = possiveis[k];
                break;
            }
        }
        const Programa *g = &p->programas[escolhido];
        int convidado = (g->convidado >= 0) ? moradorDaPessoa[g->convidado] : NINGUEM;
        if (!livreParaIr(convidado)) {
            convidado = NINGUEM;
        }
        r->programa = escolhido;
        if (convidado != NINGUEM && rotinas[convidado].lugarAtual != r->lugarAtual) {
            /* passa na casa do convidado primeiro */
            if (sairPara(quem, rotinas[convidado].lugarAtual, SEM_PERCURSO)) {
                r->fase = INDO_BUSCAR;
                r->convidado = convidado;
                rotinas[convidado].lider = quem;
                rotinas[convidado].sempreJunto = 0;
                moradores[convidado].espera = 1e9f;   /* espera ele chegar */
                return 1;
            }
            convidado = NINGUEM;
        }
        if (convidado != NINGUEM) {
            r->convidado = convidado;
            rotinas[convidado].lider = quem;
            rotinas[convidado].sempreJunto = 0;
            moradores[convidado].espera = 1e9f;
        }
        int faseAntes = r->fase;
        r->fase = INDO_AO_PROGRAMA;   /* antes de sair: quem acompanha vê para onde ele vai */
        if (sairPara(quem, g->lugar, g->percurso)) {
            return 1;
        }
        r->fase = faseAntes;
        if (convidado != NINGUEM) {
            rotinas[convidado].lider = NINGUEM;
            moradores[convidado].espera = sortear(&rotinas[convidado], 5.0f, 15.0f);
            r->convidado = NINGUEM;
        }
        r->programa = NINGUEM;
    }
    return 0;
}

/* Ir para casa (acabado o programa). Devolve 1 se saiu */
static int voltarParaCasa(int quem) {
    Rotina *r = &rotinas[quem];
    if (emCasa(quem)) {
        return 0;
    }
    r->fase = SEM_PROGRAMA;
    r->programa = NINGUEM;
    return sairPara(quem, pessoaDe(quem)->casa, SEM_PERCURSO);
}

/*
 * decidir
 * Acabou a espera, estacionado: o que fazer agora.
 */
static void decidir(int quem) {
    Morador *m = &moradores[quem];
    Rotina *r = &rotinas[quem];
    const Pessoa *p = pessoaDe(quem);

    /* Uma ida marcada (acompanhando alguém) */
    if (r->indoLugar >= 0) {
        int lugar = r->indoLugar, percurso = r->indoPercurso;
        r->indoLugar = -1;
        r->indoPercurso = SEM_PERCURSO;
        if (partir(quem, lugar, percurso)) {
            return;
        }
        if (!r->sempreJunto) {
            r->lider = NINGUEM;   /* não deu para ir junto (sem vaga): segue a vida dele */
        }
        if (voltarParaCasa(quem)) {
            return;
        }
        m->espera = TENTA_DE_NOVO;
        return;
    }

    /* Acompanhando alguém: sai só quando ele sai (chamarQuemAcompanha). Se ele sumiu de perto, volta */
    if (r->lider != NINGUEM) {
        const Morador *l = &moradores[r->lider];
        if (rotinas[r->lider].estado == PARADO && distanciaEntre(m, l->x, l->z) > JUNTO_ATE) {
            if (!r->sempreJunto) {
                r->lider = NINGUEM;
            }
            if (!voltarParaCasa(quem)) {
                m->espera = TENTA_DE_NOVO;
            }
            return;
        }
        m->espera = 3.0f;
        return;
    }

    switch (r->fase) {
        case NA_CASA_DO_CONVIDADO: {
            /* conversaram: agora o programa, com o convidado logo atrás */
            const Programa *g = &p->programas[r->programa];
            r->fase = INDO_AO_PROGRAMA;
            if (sairPara(quem, g->lugar, g->percurso)) {
                return;
            }
            if (r->convidado != NINGUEM) {
                /* não deu (sem vaga): o convidado, que está em casa, fica e segue a vida dele */
                rotinas[r->convidado].lider = NINGUEM;
                rotinas[r->convidado].indoLugar = -1;
                rotinas[r->convidado].indoPercurso = SEM_PERCURSO;
                moradores[r->convidado].espera = sortear(&rotinas[r->convidado], 10.0f, 30.0f);
                r->convidado = NINGUEM;
            }
            r->fase = NO_PROGRAMA;
            break;
        }
        case NO_PROGRAMA:
            /* acabou o programa: às vezes emenda outro (sozinho), senão volta para casa */
            if (r->convidado == NINGUEM && sortear(r, 0.0f, 1.0f) < CHANCE_DE_EMENDAR && escolherPrograma(quem)) {
                return;
            }
            if (voltarParaCasa(quem)) {
                r->convidado = NINGUEM;   /* o convidado foi chamado para casa em chamarQuemAcompanha */
                return;
            }
            break;
        default:
            if (!emCasa(quem) && voltarParaCasa(quem)) {
                return;
            }
            if (escolherPrograma(quem)) {
                return;
            }
            break;
    }
    m->espera = TENTA_DE_NOVO;
}

/*
 * chegou
 * Estacionou no fim de uma ida: quanto tempo fica, conforme o que veio fazer.
 */
static void chegou(int quem) {
    Morador *m = &moradores[quem];
    Rotina *r = &rotinas[quem];
    const Pessoa *p = pessoaDe(quem);
    r->estado = PARADO;
    m->andando = 0;
    m->velocidade = 0.0f;
    if (r->lider != NINGUEM) {
        if (!r->sempreJunto && emCasa(quem)) {
            r->lider = NINGUEM;   /* o convidado chegou de volta em casa: acabou o passeio junto */
        } else {
            m->espera = 3.0f;     /* espera o outro sair (decidir confere se ele ainda está perto) */
            return;
        }
    }
    switch (r->fase) {
        case INDO_BUSCAR:
            r->fase = NA_CASA_DO_CONVIDADO;
            m->espera = sortear(r, BUSCAR_FICA_DE, BUSCAR_FICA_ATE);
            return;
        case INDO_AO_PROGRAMA: {
            const Programa *g = &p->programas[r->programa];
            if (emCasa(quem)) {
                /* um passeio que volta para casa (a ronda, o treino): acabou o programa */
                r->fase = SEM_PROGRAMA;
                r->programa = NINGUEM;
                r->convidado = NINGUEM;
                break;
            }
            r->fase = NO_PROGRAMA;
            m->espera = sortear(r, g->de, g->ate);
            return;
        }
        default:
            r->fase = SEM_PROGRAMA;
            break;
    }
    m->espera = sortear(r, p->emCasaDe, p->emCasaAte);
}

/* A altura do chão sob o carro e o bico para cima numa ladeira (ou subindo um meio-fio) */
static void porNoChao(Morador *m, MalhaDeColisao *malha) {
    float teto = m->y + 1.5f;
    float meio = alturaDoChaoNaMalha(malha, m->x, m->z, teto, NULL);
    if (meio > SEM_CHAO) {
        m->y = meio;
    }
    float fx = sinf(m->rumo), fz = -cosf(m->rumo), eixo = m->comprimento * 0.32f;
    float frente = alturaDoChaoNaMalha(malha, m->x + fx * eixo, m->z + fz * eixo, teto, NULL);
    float tras = alturaDoChaoNaMalha(malha, m->x - fx * eixo, m->z - fz * eixo, teto, NULL);
    if (frente > SEM_CHAO && tras > SEM_CHAO) {
        m->inclinacao = atan2f(frente - tras, 2.0f * eixo);
    }
}

/*
 * iniciarMoradores
 * O grafo, os percursos, as vagas e cada morador em casa (menos o que tem o mesmo carro do jogador).
 */
void iniciarMoradores(MalhaDeColisao *malha, int carroDoJogador) {
    total = 0;
    for (int p = 0; p < TOTAL_DE_PESSOAS; p++) {
        moradorDaPessoa[p] = NINGUEM;
    }
    if (malha == NULL || TOTAL_DE_NOS > MAIS_NOS) {
        return;
    }
    for (int i = 0; i < TOTAL_DE_NOS; i++) {
        totalDeVizinhos[i] = 0;
        for (int k = 0; k < 6 && RUAS[i].vizinhos[k] != NULL; k++) {
            int v = noPeloNome(RUAS[i].vizinhos[k]);
            if (v >= 0) {
                vizinhos[i][totalDeVizinhos[i]++] = v;
            } else {
                fprintf(stderr, "MORADORES: %s aponta para %s, que não existe\n", RUAS[i].id, RUAS[i].vizinhos[k]);
            }
        }
    }
    for (int c = 0; c < TOTAL_DE_PERCURSOS; c++) {
        totalNosPercursos[c] = 0;
        for (int k = 0; k < 8 && PERCURSOS[c][k] != NULL; k++) {
            int no = noPeloNome(PERCURSOS[c][k]);
            if (no >= 0) {
                percursos[c][totalNosPercursos[c]++] = no;
            }
        }
    }
    montarVagas(malha);
    for (int p = 0; p < TOTAL_DE_PESSOAS && total < MAIS_MORADORES; p++) {
        const Pessoa *pessoa = &PESSOAS[p];
        int casa = pessoa->casa;
        if (pessoa->modelo == carroDoJogador || noDaVaga[casa] < 0) {
            continue;   /* o jogador é ele */
        }
        Morador *m = &moradores[total];
        Rotina *r = &rotinas[total];
        memset(m, 0, sizeof(*m));
        memset(r, 0, sizeof(*r));
        m->modelo = pessoa->modelo;
        m->nome = pessoa->nome;
        m->pessoa = p;
        medidasDoModelo(m->modelo, &m->comprimento, &m->largura);
        int v = reservarVaga(casa, total, 1);
        if (v < 0) {
            continue;
        }
        r->lugarAtual = casa;
        r->vagaAtual = v;
        r->estado = PARADO;
        r->programa = r->convidado = r->lider = r->parceiro = NINGUEM;
        r->indoLugar = -1;
        r->fase = SEM_PROGRAMA;
        r->sorteio = 2654435761u * (unsigned int) (p + 7);
        r->fase2 = sortear(r, 0.0f, 10.0f);
        r->falaPropria = (unsigned int) sortear(r, 0.0f, 4.0f);
        r->falaDeSempre = (unsigned int) sortear(r, 0.0f, 8.0f);
        m->x = vagas[casa][v].x;
        m->z = vagas[casa][v].z;
        m->rumo = m->rumoDaChegada = vagas[casa][v].rumo;
        float chao = alturaDoChaoNaMalha(malha, m->x, m->z, TETO_DA_CIDADE, NULL);
        m->y = (chao > SEM_CHAO) ? chao : 0.0f;
        porNoChao(m, malha);
        m->espera = pessoa->comecaEsperando;
        moradorDaPessoa[p] = total;
        total++;
    }
    /* Quem acompanha alguém sempre (o Guido, o Luigi), se os dois estão na cidade */
    for (int i = 0; i < total; i++) {
        int segue = pessoaDe(i)->segue;
        if (segue >= 0 && moradorDaPessoa[segue] != NINGUEM) {
            rotinas[i].lider = moradorDaPessoa[segue];
            rotinas[i].sempreJunto = 1;
            moradores[i].espera = 1e9f;
        }
    }
}

/* Quem está na frente deste morador, na faixa dele: a distância livre (ou um número grande) */
static float quemEstaNaFrente(const Morador *m, int quem, int *eJogador) {
    float fx = sinf(m->rumo), fz = -cosf(m->rumo);
    float menor = 1e9f;
    *eJogador = 0;
    for (int i = -1; i < total; i++) {
        /* Outro morador só segura este se está andando para o mesmo lado (a fila). Estacionado, ele está
         * fora da rua; vindo na contramão ou cruzando, cada um segue (e um empurra o outro de lado, se
         * encostarem): dois parados um de frente para o outro nunca mais sairiam do lugar */
        if ((i < 0 && rotinas[quem].ignoraJogador > 0.0f)
            || i == quem || (i >= 0 && (rotinas[quem].ignora > 0.0f || rotinas[i].estado == PARADO
                                     || rotinas[i].estado == DANDO_BRONCA
                                     || cosf(moradores[i].rumo - m->rumo) < MESMO_SENTIDO))) {
            continue;
        }
        float ox = (i < 0) ? jogador.x : moradores[i].x, oz = (i < 0) ? jogador.z : moradores[i].z;
        float dx = ox - m->x, dz = oz - m->z;
        float frente = dx * fx + dz * fz;
        float lado = fabsf(-dx * fz + dz * fx);
        float meioDoOutro = (i < 0) ? 2.2f : moradores[i].comprimento / 2.0f;
        float larguraDoOutro = (i < 0) ? 1.0f : moradores[i].largura / 2.0f;
        if (frente > 0.0f && lado < LARGURA_DA_FAIXA + larguraDoOutro * 0.5f) {
            float distancia = frente - m->comprimento / 2.0f - meioDoOutro;
            if (distancia < menor) {
                menor = distancia;
                *eJogador = (i < 0);
            }
        }
    }
    return menor;
}

/* Vem alguém pela rua, chegando perto do ponto onde ele vai entrar? (só quem está andando de verdade) */
static int vemAlguem(int quem, float x, float z) {
    if (jogador.velocidade > PARADO_ATE) {
        float d = hypotf(jogador.x - x, jogador.z - z);
        float chegando = (x - jogador.x) * sinf(jogador.rumo) - (z - jogador.z) * cosf(jogador.rumo);
        if (d < CEDE_ATE && chegando > 0.0f) {
            return 1;
        }
    }
    for (int i = 0; i < total; i++) {
        const Morador *o = &moradores[i];
        if (i == quem || (rotinas[i].estado != ANDANDO && rotinas[i].estado != PERSEGUINDO) || o->velocidade < PARADO_ATE) {
            continue;
        }
        float d = distanciaEntre(o, x, z);
        float chegando = (x - o->x) * sinf(o->rumo) - (z - o->z) * cosf(o->rumo);
        if (d < CEDE_ATE && chegando > 0.0f) {
            return 1;
        }
    }
    return 0;
}

/* No ponto de entrar na rua, ainda esperando a vez? (vem alguém, e ele não cansou de esperar) */
static int esperandoAVez(int quem) {
    const Morador *m = &moradores[quem];
    const Rotina *r = &rotinas[quem];
    return (r->marcas[m->proximo] & ENTRA_NA_RUA) && m->proximo + 1 < m->totalDoCaminho && r->cedendo < CEDE_NO_MAXIMO
           && vemAlguem(quem, m->caminho[m->proximo + 1][0], m->caminho[m->proximo + 1][1]);
}

/* A manobra de ré: anda para trás de um ponto a outro, girando aos poucos até o rumo de lá */
static int manobrarDeRe(Morador *m, Rotina *r, MalhaDeColisao *malha, float tempo) {
    float antes = r->reAndou;
    r->reAndou = fminf(r->reAndou + VELOCIDADE_DE_RE * tempo, r->reTotal);
    float t = suave(r->reAndou / r->reTotal);
    float giro = diferencaDeRumo(r->reAteRumo, r->reDeRumo);
    m->x = r->reDeX + (r->reAteX - r->reDeX) * t;
    m->z = r->reDeZ + (r->reAteZ - r->reDeZ) * t;
    m->rumo = r->reDeRumo + giro * t;
    m->velocidade = -VELOCIDADE_DE_RE;
    m->volante += (limitar(-giro, -0.8f, 0.8f) - m->volante) * fminf(tempo * 4.0f, 1.0f);   /* de ré, o volante vira ao contrário */
    m->anguloDasRodas = fmodf(m->anguloDasRodas - (r->reAndou - antes) / 0.36f + 2.0f * PI_M, 2.0f * PI_M);
    porNoChao(m, malha);
    return r->reAndou >= r->reTotal;
}

/* O ponto está dentro do círculo da curva mais fechada que ele faz para aquele lado? (não dá para chegar
 * nele sem dar ré ou uma volta) */
static int foraDoAlcance(const Morador *m, float px, float pz, float raio) {
    float direitaX = cosf(m->rumo), direitaZ = sinf(m->rumo);
    float lado = ((px - m->x) * direitaX + (pz - m->z) * direitaZ >= 0.0f) ? 1.0f : -1.0f;
    float cx = m->x + direitaX * raio * lado, cz = m->z + direitaZ * raio * lado;
    return hypotf(px - cx, pz - cz) < raio * 0.95f;
}

/* Anda com o volante para o rumo pedido (até o raio de curva daquela velocidade) */
static void andar(Morador *m, MalhaDeColisao *malha, float falta, float raio, float tempo) {
    float giroMaximo = fmaxf(m->velocidade, 0.0f) / raio;
    m->rumo += limitar(falta * 2.5f, -giroMaximo, giroMaximo) * tempo;
    m->volante += (limitar(falta * 2.0f, -1.0f, 1.0f) - m->volante) * fminf(tempo * 6.0f, 1.0f);
    m->x += sinf(m->rumo) * m->velocidade * tempo;
    m->z -= cosf(m->rumo) * m->velocidade * tempo;
    m->anguloDasRodas = fmodf(m->anguloDasRodas + m->velocidade * tempo / 0.36f, 2.0f * PI_M);
    porNoChao(m, malha);
}

static void dirigir(int quem, MalhaDeColisao *malha, float tempo) {
    Morador *m = &moradores[quem];
    Rotina *r = &rotinas[quem];
    int pontoDaVaga = m->totalDoCaminho - 1;
    int pontoDaEntrada = r->reNaChegada ? pontoDaVaga - 1 : pontoDaVaga;   /* onde ele para de andar de frente */
    if (pontoDaEntrada < 0) {
        chegou(quem);
        return;
    }
    /* O ponto que ele mira: o seguinte, quando chega perto deste ou quando este ficou fora do alcance (dentro
     * do círculo da curva mais fechada que ele faz: senão ele daria voltas em torno do ponto para sempre) */
    while (m->proximo < pontoDaEntrada) {
        float dx = m->caminho[m->proximo][0] - m->x, dz = m->caminho[m->proximo][1] - m->z;
        if (dx * dx + dz * dz > CHEGOU_NO_PONTO * CHEGOU_NO_PONTO
            && !foraDoAlcance(m, m->caminho[m->proximo][0], m->caminho[m->proximo][1], RAIO_MINIMO)) {
            break;
        }
        if (esperandoAVez(quem)) {
            break;   /* no ponto de entrar na rua, esperando quem vem: não passa dele */
        }
        m->proximo++;
    }
    int ultimo = (m->proximo >= pontoDaEntrada);
    float alvoX = m->caminho[m->proximo][0], alvoZ = m->caminho[m->proximo][1];
    float dx = alvoX - m->x, dz = alvoZ - m->z;
    float distancia = hypotf(dx, dz);
    float olhaAFrente = fmaxf(OLHA_A_FRENTE, m->velocidade * 0.6f);
    if (!ultimo && distancia < olhaAFrente) {
        float t = 1.0f - distancia / olhaAFrente;   /* a curva começa antes do ponto, sem quina */
        alvoX += (m->caminho[m->proximo + 1][0] - alvoX) * t;
        alvoZ += (m->caminho[m->proximo + 1][1] - alvoZ) * t;
        dx = alvoX - m->x;
        dz = alvoZ - m->z;
    }
    float falta = diferencaDeRumo(rumoDe(dx, dz), m->rumo);

    /* A velocidade: a dele na cidade, menos nas curvas, nos pátios, chegando e atrás de alguém */
    int noTreino = (r->marcas[m->proximo] & NO_TREINO) != 0;
    float alvo = (noTreino ? VELOCIDADE_NO_TREINO : VELOCIDADE_NA_CIDADE) * pessoaDe(quem)->pressa
                 * (1.0f - fminf(fabsf(falta), 1.3f) / 1.7f);
    if (m->proximo >= pontoDaEntrada - 1 || m->proximo == 0) {
        alvo = fminf(alvo, VELOCIDADE_NO_PATIO);
    }
    if (ultimo) {
        alvo = fminf(alvo, 0.7f + distancia * 0.6f);
    }
    /* Saindo para a rua: para antes e espera quem vem chegando passar */
    if (!ultimo && distancia < CEDE_PERTO && esperandoAVez(quem)) {
        alvo = fminf(alvo, fmaxf(0.0f, (distancia - 1.5f) * 0.8f));
        r->cedendo += tempo;
    } else if (!(r->marcas[m->proximo] & ENTRA_NA_RUA)) {
        r->cedendo = 0.0f;
    }
    /* Perto do fim do caminho, manobra devagar e mais fechado; rodeando demais, estaciona dali mesmo */
    int manobrando = ultimo && distancia < MANOBRA_PERTO;
    if (manobrando) {
        r->rodeando += tempo;
        if (foraDoAlcance(m, alvoX, alvoZ, RAIO_MINIMO)) {
            alvo = fminf(alvo, 1.5f);
        }
    }
    int eJogador = 0;
    float livre = quemEstaNaFrente(m, quem, &eJogador);
    float folga = DISTANCIA_NA_FILA + fmaxf(m->velocidade, 0.0f) * 0.8f;
    int segurado = livre < folga;
    if (segurado) {
        alvo = fminf(alvo, fmaxf(0.0f, (livre - DISTANCIA_NA_FILA * 0.5f) * 0.7f));
    }
    if (segurado && m->velocidade < 0.3f) {
        float antes = m->bloqueado;
        m->bloqueado += tempo;
        if (eJogador && antes < BUZINA_DEPOIS && m->bloqueado >= BUZINA_DEPOIS) {
            m->buzinou = 1;
        }
        if (!eJogador && m->bloqueado > DESISTE_DE_ESPERAR) {
            r->ignora = IGNORA_POR;   /* cansou de esperar o outro morador: vai assim mesmo */
            m->bloqueado = 0.0f;
        } else if (eJogador && m->bloqueado > CONTORNA_O_JOGADOR) {
            r->ignoraJogador = CONTORNA_POR;   /* buzinou e o jogador não saiu: vai em frente, encostando nele */
            m->bloqueado = 0.0f;
        }
    } else {
        m->bloqueado = 0.0f;
    }
    m->velocidade += limitar(alvo - m->velocidade, -FREIA * tempo, ACELERA * tempo);
    andar(m, malha, falta, manobrando ? RAIO_DE_MANOBRA : RAIO_MINIMO, tempo);

    /* Chegou: na vaga (de frente) ou na entrada, para entrar de ré */
    if (ultimo && (distancia < CHEGOU_NA_VAGA * (r->reNaChegada ? 2.0f : 1.0f) || r->rodeando > RODEANDO_NO_MAXIMO)) {
        m->velocidade = 0.0f;
        if (r->reNaChegada) {
            const Vaga *vaga = &vagas[r->lugarAtual][r->vagaAtual];
            comecarRe(r, m, vaga->x, vaga->z, vaga->rumo);
            r->estado = ENTRANDO_DE_RE;
        } else {
            chegou(quem);
        }
    }
}

/*
 * perseguir
 * O xerife atrás do jogador: direto até ele (sem o grafo), rápido, e para um pouco atrás. Com o jogador
 * parado perto, pegou: a bronca. Longe demais ou tempo demais, desiste.
 */
static void perseguir(int quem, MalhaDeColisao *malha, float tempo) {
    Morador *m = &moradores[quem];
    Rotina *r = &rotinas[quem];
    float dx = jogador.x - m->x, dz = jogador.z - m->z;
    float d = hypotf(dx, dz);
    r->perseguiu += tempo;
    m->perseguindo = 1;
    m->aperto = 1.0f;
    float alvo = fminf(VELOCIDADE_DA_PERSEGUICAO, fmaxf(0.0f, (d - FICA_ATRAS) * 1.5f) + jogador.velocidade * 0.9f);
    float muda = (alvo > m->velocidade) ? ACELERA_NA_PERSEGUICAO : FREIA * 1.5f;
    m->velocidade += limitar(alvo - m->velocidade, -muda * tempo, muda * tempo);
    andar(m, malha, diferencaDeRumo(rumoDe(dx, dz), m->rumo), fmaxf(RAIO_MINIMO, m->velocidade * 0.8f), tempo);
    if (d < PEGOU_ATE && jogador.velocidade < PARADO_ATE * 2.0f) {
        r->pegando += tempo;
    } else {
        r->pegando = 0.0f;
    }
    if (r->pegando > PEGOU_DEPOIS) {
        dizer(m, XERIFE_PEGOU, BRONCA_DURA + 0.5f, 1);
        r->estado = DANDO_BRONCA;
        m->espera = BRONCA_DURA;
        m->velocidade = 0.0f;
    } else if (d > FUGIU_ALEM || r->perseguiu > PERSEGUE_NO_MAXIMO
               || hypotf(m->x - RUAS[noDaVaga[r->lugarAtual]].x, m->z - RUAS[noDaVaga[r->lugarAtual]].z) > LONGE_DE_CASA) {
        dizer(m, XERIFE_DESISTE, DURA_A_FALA, 1);
        m->perseguindo = 0;
        r->descanso = XERIFE_DESCANSA;
        if (!voltarDeOndeEsta(quem, pessoaDe(quem)->casa)) {
            r->estado = DANDO_BRONCA;   /* sem caminho (longe das ruas): fica um pouco e tenta de novo */
            m->espera = 3.0f;
        }
    }
}

/* O xerife no esconderijo vê o jogador passando correndo? */
static int xerifeViuCorrendo(int quem) {
    const Morador *m = &moradores[quem];
    const Rotina *r = &rotinas[quem];
    return m->pessoa == P_XERIFE && r->estado == PARADO && emCasa(quem) && r->descanso <= 0.0f
           && r->fase == SEM_PROGRAMA && jogador.velocidade > LIMITE_DO_XERIFE
           && distanciaEntre(m, jogador.x, jogador.z) < XERIFE_VE_ATE;
}

/*
 * separarMoradores
 * Dois moradores que encostam se empurram de lado (cada carro são três círculos, do bico à traseira).
 * Só quem está andando é empurrado: o estacionado e o que manobra de ré ficam onde estão.
 */
static void separarMoradores(void) {
    for (int i = 0; i < total; i++) {
        for (int j = i + 1; j < total; j++) {
            Morador *a = &moradores[i], *b = &moradores[j];
            float perto = (a->comprimento + b->comprimento) / 2.0f;
            if (fabsf(a->x - b->x) > perto || fabsf(a->z - b->z) > perto) {
                continue;
            }
            float pesoA = (rotinas[i].estado == ANDANDO || rotinas[i].estado == PERSEGUINDO) ? 1.0f : 0.0f;
            float pesoB = (rotinas[j].estado == ANDANDO || rotinas[j].estado == PERSEGUINDO) ? 1.0f : 0.0f;
            if (pesoA + pesoB <= 0.0f) {
                continue;
            }
            pesoA /= pesoA + pesoB;
            pesoB = 1.0f - pesoA;
            float ra = a->largura * 0.45f, rb = b->largura * 0.45f;
            float ateA = fmaxf(a->comprimento / 2.0f - ra, 0.0f), ateB = fmaxf(b->comprimento / 2.0f - rb, 0.0f);
            float pushX = 0.0f, pushZ = 0.0f;
            for (int ka = -1; ka <= 1; ka++) {
                for (int kb = -1; kb <= 1; kb++) {
                    float ax = a->x + sinf(a->rumo) * ateA * ka, az = a->z - cosf(a->rumo) * ateA * ka;
                    float bx = b->x + sinf(b->rumo) * ateB * kb, bz = b->z - cosf(b->rumo) * ateB * kb;
                    float dx = ax - bx, dz = az - bz, d = hypotf(dx, dz);
                    if (d < ra + rb && d > 1e-3f) {
                        float entrou = (ra + rb - d) / 3.0f;   /* três pares por círculo: um terço cada */
                        pushX += dx / d * entrou;
                        pushZ += dz / d * entrou;
                    }
                }
            }
            a->x += pushX * pesoA;
            a->z += pushZ * pesoA;
            b->x -= pushX * pesoB;
            b->z -= pushZ * pesoB;
        }
    }
}

/* A conversa do par (a ordem não importa), ou NULL */
static const Conversa *conversaDoPar(int pa, int pb) {
    for (int c = 0; c < TOTAL_DE_CONVERSAS; c++) {
        if ((CONVERSAS[c].a == pa && CONVERSAS[c].b == pb) || (CONVERSAS[c].a == pb && CONVERSAS[c].b == pa)) {
            return &CONVERSAS[c];
        }
    }
    return NULL;
}

/* Uma fala solta: das que a pessoa diz para qualquer um ou das que todo mundo diz, em rodízio (sem
 * repetir a mesma em seguida) */
static const char *falaSolta(int quem) {
    const Pessoa *p = pessoaDe(quem);
    Rotina *r = &rotinas[quem];
    int minhas = 0;
    while (minhas < 4 && p->conversa[minhas] != NULL) {
        minhas++;
    }
    int soAsDele = (moradores[quem].pessoa == P_GUIDO || moradores[quem].pessoa == P_RUIVO);   /* o Guido só fala italiano */
    if (minhas > 0 && (soAsDele || sortear(r, 0.0f, 1.0f) < CHANCE_DA_FALA_PROPRIA)) {
        return p->conversa[(r->falaPropria++) % minhas];
    }
    return FALAS_DE_SEMPRE[(r->falaDeSempre++) % TOTAL_DE_FALAS_DE_SEMPRE];
}

static int estacionado(int i) {
    return rotinas[i].estado == PARADO || rotinas[i].estado == ENTRANDO_DE_RE;
}

/*
 * conversar
 * Os estacionados perto um do outro conversam aos pares (cada um com o mais perto dele, se for recíproco):
 * olham um para o outro e falam na vez de cada um; quem fala mexe o bico. A vez passa de tempos em
 * tempos, com a próxima fala da conversa do par (ou uma fala solta de quem fala).
 */
static void conversar(float tempo) {
    int maisPerto[MAIS_MORADORES];
    for (int i = 0; i < total; i++) {
        maisPerto[i] = NINGUEM;
        if (rotinas[i].estado != PARADO) {
            continue;
        }
        float menor = CONVERSA_ATE;
        for (int j = 0; j < total; j++) {
            float d = distanciaEntre(&moradores[i], moradores[j].x, moradores[j].z);
            if (j != i && estacionado(j) && d < menor) {
                menor = d;
                maisPerto[i] = j;
            }
        }
    }
    for (int i = 0; i < total; i++) {
        int j = maisPerto[i];
        Rotina *r = &rotinas[i];
        if (j == NINGUEM || maisPerto[j] != i) {
            r->parceiro = NINGUEM;
            continue;
        }
        Morador *m = &moradores[i];
        m->olhaAlgo = 1;
        m->olhar = olharPara(m, moradores[j].x, moradores[j].z);
        if (r->parceiro != j) {   /* começou a conversa agora */
            r->parceiro = j;
            r->turno = sortear(r, 0.5f, 2.0f);
            r->linha = 0;
        }
        if (i > j) {
            continue;   /* o par é tocado pelo de número menor */
        }
        r->turno -= tempo;
        if (r->turno > 0.0f) {
            continue;
        }
        /* depois de uma fala, às vezes uma pausa (ninguém fala o tempo todo) */
        if (!r->calado && sortear(r, 0.0f, 1.0f) < CHANCE_DE_PAUSA) {
            r->calado = 1;
            r->turno = sortear(r, PAUSA_DE, PAUSA_ATE);
            continue;
        }
        r->calado = 0;
        float turno = sortear(r, TURNO_DE, TURNO_ATE);
        r->turno = turno;
        const Conversa *c = conversaDoPar(m->pessoa, moradores[j].pessoa);
        int quantas = 0;
        while (c != NULL && quantas < 8 && c->falas[quantas] != NULL) {
            quantas++;
        }
        int k = r->linha;
        int fala = (quantas > 0) ? (((k % quantas) % 2 == 0) == (c->a == m->pessoa) ? i : j) : ((k % 2 == 0) ? i : j);
        if (moradores[fala].falaRestante > 0.5f) {
            r->turno = 1.0f;   /* ainda falando outra coisa (com o jogador): a conversa espera */
            continue;
        }
        r->linha++;
        dizer(&moradores[fala], (quantas > 0) ? c->falas[k % quantas] : falaSolta(fala), turno, 0);
    }
}

/*
 * reagirAoJogador
 * Estacionado, olha o jogador que chega devagar e cumprimenta quem para perto (o Ruivo, tímido, se
 * encolhe e olha para o outro lado). Passando rápido rente, ele se assusta e reclama.
 */
static void reagirAoJogador(int i, float tempo) {
    Morador *m = &moradores[i];
    Rotina *r = &rotinas[i];
    float d = distanciaEntre(m, jogador.x, jogador.z);
    r->cumprimentou = fmaxf(r->cumprimentou - tempo, 0.0f);
    r->reclamou = fmaxf(r->reclamou - tempo, 0.0f);
    if (jogador.velocidade > SUSTO_DE_PASSAGEM && d < SUSTO_ATE + m->comprimento / 2.0f && r->estado != PERSEGUINDO) {
        m->susto = fmaxf(m->susto, 1.0f);
        if (r->reclamou <= 0.0f && m->pessoa != P_RUIVO && m->pessoa != P_XERIFE) {
            dizer(m, (m->pessoa == P_LIZZIE) ? SUSTO_DA_LIZZIE : SUSTOS[(int) sortear(r, 0.0f, TOTAL_DE_SUSTOS - 0.001f)],
                  DURA_A_FALA, 0);
            r->reclamou = RECLAMA_DE_NOVO;
        }
    }
    if (r->estado != PARADO) {
        r->jogadorPerto = 0.0f;
        return;
    }
    if (d < OLHA_O_JOGADOR_ATE && jogador.velocidade < DEVAGAR_ATE) {
        m->olhaAlgo = 1;
        m->olhar = olharPara(m, jogador.x, jogador.z);
        if (m->pessoa == P_RUIVO && d < CUMPRIMENTA_ATE) {
            m->olhar = -m->olhar;   /* tímido: olha para o outro lado */
        }
    }
    if (d < CUMPRIMENTA_ATE && jogador.velocidade < PARADO_ATE) {
        r->jogadorPerto += tempo;
        if (r->jogadorPerto > CUMPRIMENTA_DEPOIS && r->cumprimentou <= 0.0f) {
            const Pessoa *p = pessoaDe(i);
            int n = 0;
            while (n < 3 && p->aoJogador[n] != NULL) {
                n++;
            }
            if (n > 0) {
                dizer(m, p->aoJogador[(int) sortear(r, 0.0f, (float) n - 0.001f)], DURA_A_FALA + 0.6f, 0);
            } else {
                m->susto = fmaxf(m->susto, 0.6f);   /* o Ruivo */
            }
            r->cumprimentou = CUMPRIMENTA_DE_NOVO;
        }
    } else {
        r->jogadorPerto = 0.0f;
    }
}

/* Estacionado sem conversa nem jogador: acompanha com os olhos quem passa perto */
static void olharQuemPassa(int i) {
    Morador *m = &moradores[i];
    if (m->olhaAlgo || rotinas[i].estado != PARADO) {
        return;
    }
    float menor = OLHA_QUEM_PASSA_ATE;
    for (int j = 0; j < total; j++) {
        float d = distanciaEntre(m, moradores[j].x, moradores[j].z);
        if (j != i && fabsf(moradores[j].velocidade) > 1.0f && d < menor) {
            menor = d;
            m->olhaAlgo = 1;
            m->olhar = olharPara(m, moradores[j].x, moradores[j].z);
        }
    }
}

/*
 * atualizarMoradores
 * Cada um: estacionado, espera e, na hora, decide o que fazer; manobrando, de ré; andando, pelo caminho;
 * o xerife, atrás do jogador ou dando a bronca. Depois, quem encosta se empurra, e a vida em volta: as
 * conversas, o jogador, os olhares e as falas que vão acabando.
 */
void atualizarMoradores(MalhaDeColisao *malha, float jogadorX, float jogadorZ, float jogadorRumo,
                        float jogadorVelocidade, float tempo) {
    if (tempo <= 0.0f) {
        return;
    }
    tempo = fminf(tempo, 0.1f);
    jogador.x = jogadorX;
    jogador.z = jogadorZ;
    jogador.rumo = jogadorRumo;
    jogador.velocidade = jogadorVelocidade;
    for (int i = 0; i < total; i++) {
        Morador *m = &moradores[i];
        Rotina *r = &rotinas[i];
        m->buzinou = 0;
        m->olhaAlgo = 0;
        m->aperto = 0.0f;
        m->perseguindo = 0;
        if (m->susto > 0.0f) {
            m->susto -= tempo;
        }
        r->ignora = fmaxf(r->ignora - tempo, 0.0f);
        r->ignoraJogador = fmaxf(r->ignoraJogador - tempo, 0.0f);
        r->descanso = fmaxf(r->descanso - tempo, 0.0f);
        switch (r->estado) {
            case SAINDO_DE_RE:
                if (manobrarDeRe(m, r, malha, tempo)) {
                    r->estado = ANDANDO;
                    m->velocidade = 0.0f;
                }
                break;
            case ANDANDO:
                dirigir(i, malha, tempo);
                break;
            case ENTRANDO_DE_RE:
                if (manobrarDeRe(m, r, malha, tempo)) {
                    chegou(i);
                }
                break;
            case PERSEGUINDO:
                perseguir(i, malha, tempo);
                break;
            case DANDO_BRONCA:
                m->velocidade = 0.0f;
                m->aperto = 0.6f;
                m->olhaAlgo = 1;
                m->olhar = olharPara(m, jogador.x, jogador.z);
                m->espera -= tempo;
                if (m->espera <= 0.0f) {
                    r->descanso = XERIFE_DESCANSA;
                    if (!voltarDeOndeEsta(i, pessoaDe(i)->casa)) {
                        m->espera = 3.0f;
                    }
                }
                break;
            default:
                m->velocidade = 0.0f;
                m->rumo += diferencaDeRumo(m->rumoDaChegada, m->rumo) * fminf(tempo * 2.0f, 1.0f);
                m->volante *= expf(-tempo * 3.0f);
                if (xerifeViuCorrendo(i)) {
                    r->estado = PERSEGUINDO;
                    r->perseguiu = r->pegando = 0.0f;
                    m->andando = 1;
                    dizer(m, XERIFE_VAI, DURA_A_FALA, 1);
                    break;
                }
                m->espera -= tempo;
                if (m->espera <= 0.0f) {
                    decidir(i);
                }
                break;
        }
    }
    separarMoradores();

    /* A vida em volta */
    conversar(tempo);
    for (int i = 0; i < total; i++) {
        Morador *m = &moradores[i];
        Rotina *r = &rotinas[i];
        reagirAoJogador(i, tempo);
        olharQuemPassa(i);
        if (m->falaRestante > 0.0f) {
            m->falaRestante -= tempo;
            if (m->falaRestante <= 0.0f) {
                m->fala = NULL;
                m->falaLonge = 0;
            }
        }
        /* Falando, o bico sobe e desce aos pouquinhos; ouvindo, quase parado */
        r->fase2 += tempo;
        float falando = (m->fala != NULL && r->estado == PARADO) ? 1.0f : 0.0f;
        m->aceno = falando * 0.022f * sinf(r->fase2 * 10.0f) * fmaxf(0.0f, sinf(r->fase2 * 2.1f))
                   + (r->parceiro != NINGUEM) * (1.0f - falando) * 0.006f * sinf(r->fase2 * 2.7f);
    }
}
