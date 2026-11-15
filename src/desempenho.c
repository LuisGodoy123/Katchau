/*
 * desempenho.c
 * ------------------------------------------------------------------
 * A medição do quadro e o gráfico dela (veja desempenho.h).
 *
 * Na CPU, cada etapa é o tempo entre a marca dela e a seguinte (GetTime).
 * Na placa, cada marca fecha a consulta de tempo da etapa anterior e abre a
 * da nova (GL_TIME_ELAPSED, do núcleo do OpenGL 3.3; o carimbo de tempo,
 * GL_TIMESTAMP, volta zerado no Mac): o resultado é o tempo que a placa
 * levou entre as duas marcas. As consultas ficam prontas uns quadros depois;
 * elas são lidas quando já chegaram (QUADROS_NA_PLACA quadros de folga),
 * para nunca parar a CPU esperando a placa. Quando a CPU é que segura o
 * quadro, a placa fica à toa entre as marcas e esse tempo também entra na
 * etapa: os números da placa por etapa valem de verdade quando é ela que
 * limita o quadro.
 * ------------------------------------------------------------------
 */

#include "desempenho.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "raylib.h"
#include "rlgl.h"
#include "hud.h"
#include "visual.h"

/* ---------- As funções do OpenGL dos carimbos de tempo ----------
 * A raylib não as expõe; cada sistema as entrega do seu jeito */
#define TEMPO_GL            0x88BF   /* GL_TIME_ELAPSED */
#define RESULTADO_GL        0x8866   /* GL_QUERY_RESULT */
#define RESULTADO_PRONTO_GL 0x8867   /* GL_QUERY_RESULT_AVAILABLE */
#if defined(_WIN32)
#define CHAMADA_GL __stdcall
#else
#define CHAMADA_GL
#endif
typedef void (CHAMADA_GL *GerarConsultas)(int quantas, unsigned int *ids);
typedef void (CHAMADA_GL *AbrirConsulta)(unsigned int alvo, unsigned int id);
typedef void (CHAMADA_GL *FecharConsulta)(unsigned int alvo);
typedef void (CHAMADA_GL *LerConsulta)(unsigned int id, unsigned int qual, int *valor);
typedef void (CHAMADA_GL *LerConsulta64)(unsigned int id, unsigned int qual, unsigned long long *valor);

#if defined(__APPLE__)
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
static void *funcaoDoGl(const char *nome) {
    if (strcmp(nome, "glGenQueries") == 0) return (void *) glGenQueries;
    if (strcmp(nome, "glBeginQuery") == 0) return (void *) glBeginQuery;
    if (strcmp(nome, "glEndQuery") == 0) return (void *) glEndQuery;
    if (strcmp(nome, "glGetQueryObjectiv") == 0) return (void *) glGetQueryObjectiv;
    if (strcmp(nome, "glGetQueryObjectui64v") == 0) return (void *) glGetQueryObjectui64v;
    return NULL;
}
#elif defined(_WIN32)
__declspec(dllimport) void *__stdcall wglGetProcAddress(const char *nome);
static void *funcaoDoGl(const char *nome) {
    return wglGetProcAddress(nome);
}
#else
typedef void (*FuncaoSolta)(void);
extern FuncaoSolta glXGetProcAddressARB(const unsigned char *nome);
static void *funcaoDoGl(const char *nome) {
    return (void *) glXGetProcAddressARB((const unsigned char *) nome);
}
#endif

static GerarConsultas glGerarConsultas;
static AbrirConsulta glAbrirConsulta;
static FecharConsulta glFecharConsulta;
static int consultaAberta = 0;
static LerConsulta glLerConsulta;
static LerConsulta64 glLerConsulta64;
static int temPlaca = 0;

/* ---------- O que cada quadro guarda ---------- */
#define QUADROS_NA_MEMORIA  600      /* o gráfico mostra os últimos GRAFICO_QUADROS deles */
#define QUADROS_NA_PLACA    5        /* quadros de folga até ler os carimbos */
#define MARCAS_POR_QUADRO   48
#define FIM_DO_QUADRO       TOTAL_ETAPAS   /* a marca depois da troca */

typedef struct {
    long numero;                    /* o número do quadro (0: vago) */
    float total;                    /* ms: do começo deste quadro ao do seguinte (0: ainda não se sabe) */
    float cpu[TOTAL_ETAPAS];        /* ms */
    float placa[TOTAL_ETAPAS];      /* ms */
    float placaTotal;               /* ms: do primeiro ao último carimbo */
    int temPlaca;
    int contas[TOTAL_CONTAS];
    int fase;                       /* na gravação: a fase (-1: fora dela) */
} QuadroMedido;

typedef struct {
    unsigned int ids[MARCAS_POR_QUADRO];
    unsigned char etapas[MARCAS_POR_QUADRO];   /* a etapa de cada consulta */
    int marcas;
    long numero;
    int esperando;
} QuadroNaPlaca;

static QuadroMedido historia[QUADROS_NA_MEMORIA];
static QuadroNaPlaca naPlaca[QUADROS_NA_PLACA];
static long numeroDoQuadro = 0;
static long primeiroDaSequencia = 1;    /* o gráfico só mostra quadros medidos em sequência */
static int medindo = 0;
static double inicioDoQuadro = 0.0, inicioDaEtapa = 0.0;
static EtapaDoQuadro etapaAtual = ETAPA_LOGICA;
static int graficoNaTela = 0;           /* o gráfico apareceu no último quadro */
static int trocadoNoF3 = 0;

/* ---------- A gravação ---------- */
#define MAIS_FASES 16
static int gravando = 0;
static char arquivoDaGravacao[256];
static char fases[MAIS_FASES][40];
static int totalDeFases = 0, faseAtual = -1;
static QuadroMedido *gravados = NULL;
static int totalGravados = 0, cabemGravados = 0;

static const char *NOMES_DAS_ETAPAS[TOTAL_ETAPAS] = {
    "Lógica", "Som", "Preparo", "Sombras", "Mundo", "Luzes", "Telões", "Pós e HUD", "Espera"
};
static const char *CHAVES_DAS_ETAPAS[TOTAL_ETAPAS] = {
    "logica", "som", "preparo", "sombras", "mundo", "luzes", "teloes", "tela", "espera"
};
static const char *CHAVES_DAS_CONTAS[TOTAL_CONTAS] = {
    "lampadas_acesas", "lampadas_nos_shaders", "envios_de_lampadas", "grades_montadas", "envios_da_luz", "sombras_refeitas"
};

/* As cores das etapas: a paleta categórica, na ordem fixa, nos tons para fundo escuro. A espera é cinza
 * (não é trabalho: é a CPU parada esperando a placa ou o VSync) */
static const Color CORES_DAS_ETAPAS[TOTAL_ETAPAS] = {
    { 0x39, 0x87, 0xe5, 255 },   /* azul */
    { 0xd9, 0x59, 0x26, 255 },   /* laranja */
    { 0x19, 0x9e, 0x70, 255 },   /* água */
    { 0xc9, 0x85, 0x00, 255 },   /* amarelo */
    { 0xd5, 0x51, 0x81, 255 },   /* magenta */
    { 0x00, 0x83, 0x00, 255 },   /* verde */
    { 0x90, 0x85, 0xe9, 255 },   /* violeta */
    { 0xe6, 0x67, 0x67, 255 },   /* vermelho */
    { 0x5f, 0x5e, 0x5a, 255 },   /* a espera */
};
#define COR_DO_FUNDO      (Color){ 0x1a, 0x1a, 0x19, 225 }
#define COR_DO_TEXTO      (Color){ 0xf0, 0xef, 0xec, 255 }
#define COR_DO_TEXTO_2    (Color){ 0xc3, 0xc2, 0xb7, 255 }
#define COR_DA_GRADE      (Color){ 0xf0, 0xef, 0xec, 46 }

/* ------------------------------------------------------------------
 * A medição
 * ------------------------------------------------------------------ */

void iniciarDesempenho(void) {
    glGerarConsultas = (GerarConsultas) funcaoDoGl("glGenQueries");
    glAbrirConsulta = (AbrirConsulta) funcaoDoGl("glBeginQuery");
    glFecharConsulta = (FecharConsulta) funcaoDoGl("glEndQuery");
    glLerConsulta = (LerConsulta) funcaoDoGl("glGetQueryObjectiv");
    glLerConsulta64 = (LerConsulta64) funcaoDoGl("glGetQueryObjectui64v");
    temPlaca = glGerarConsultas != NULL && glAbrirConsulta != NULL && glFecharConsulta != NULL && glLerConsulta != NULL
               && glLerConsulta64 != NULL;
    if (temPlaca) {
        for (int i = 0; i < QUADROS_NA_PLACA; i++) {
            glGerarConsultas(MARCAS_POR_QUADRO, naPlaca[i].ids);
        }
    }
    TraceLog(LOG_INFO, "DESEMPENHO: tempo da placa de video %s", temPlaca ? "medido (consultas de tempo do OpenGL)" : "indisponivel");
}

static QuadroMedido *quadroDeNumero(long numero) {
    QuadroMedido *q = &historia[numero % QUADROS_NA_MEMORIA];
    return (q->numero == numero) ? q : NULL;
}

static void guardarNaGravacao(const QuadroMedido *q) {
    if (!gravando || q->fase < 0) {
        return;
    }
    if (totalGravados == cabemGravados) {
        int novo = cabemGravados ? cabemGravados * 2 : 8192;
        QuadroMedido *maior = realloc(gravados, sizeof(QuadroMedido) * (size_t) novo);
        if (maior == NULL) {
            return;
        }
        gravados = maior;
        cabemGravados = novo;
    }
    gravados[totalGravados++] = *q;
}

/* carimbar: fecha a consulta da etapa que termina e abre a da que começa (FIM_DO_QUADRO: só fecha) */
static void carimbar(int etapa) {
    QuadroNaPlaca *p = &naPlaca[numeroDoQuadro % QUADROS_NA_PLACA];
    rlDrawRenderBatchActive();   /* o que a raylib ainda segura é da etapa que termina */
    if (consultaAberta) {
        glFecharConsulta(TEMPO_GL);
        consultaAberta = 0;
    }
    if (etapa == FIM_DO_QUADRO || p->marcas >= MARCAS_POR_QUADRO) {
        return;
    }
    glAbrirConsulta(TEMPO_GL, p->ids[p->marcas]);
    consultaAberta = 1;
    p->etapas[p->marcas] = (unsigned char) etapa;
    p->marcas++;
}

/* lerDaPlaca: as consultas de um quadro, se já chegaram (ou esperando por elas) */
static void lerDaPlaca(QuadroNaPlaca *p, int esperar) {
    if (!p->esperando || p->marcas < 1) {
        p->esperando = 0;
        return;
    }
    int pronto = 0;
    glLerConsulta(p->ids[p->marcas - 1], RESULTADO_PRONTO_GL, &pronto);
    if (!pronto && !esperar) {
        return;
    }
    p->esperando = 0;
    QuadroMedido *q = quadroDeNumero(p->numero);
    if (q == NULL) {
        return;
    }
    double soma = 0.0;
    for (int i = 0; i < p->marcas; i++) {
        unsigned long long nanos = 0;
        glLerConsulta64(p->ids[i], RESULTADO_GL, &nanos);
        float ms = (float) ((double) nanos / 1.0e6);
        q->placa[p->etapas[i]] += ms;
        soma += ms;
    }
    q->placaTotal = (float) soma;
    q->temPlaca = 1;
    if (q->total > 0.0f) {
        guardarNaGravacao(q);
    }
}

void comecarQuadroDeDesempenho(void) {
    if (IsKeyPressed(KEY_F3)) {
        trocadoNoF3 = !trocadoNoF3;
    }
    double agora = GetTime();
    if (medindo) {
        /* O quadro anterior termina aqui */
        QuadroMedido *q = quadroDeNumero(numeroDoQuadro);
        if (q != NULL) {
            q->total = (float) ((agora - inicioDoQuadro) * 1000.0);
            if (!temPlaca) {
                guardarNaGravacao(q);
            }
        }
    }
    if (temPlaca) {
        for (int i = 0; i < QUADROS_NA_PLACA; i++) {
            lerDaPlaca(&naPlaca[i], 0);
        }
    }
    int medir = graficoNaTela || gravando;
    if (!medir) {
        medindo = 0;
        return;
    }
    if (!medindo) {
        primeiroDaSequencia = numeroDoQuadro + 1;
    }
    medindo = 1;
    numeroDoQuadro++;
    QuadroMedido *q = &historia[numeroDoQuadro % QUADROS_NA_MEMORIA];
    memset(q, 0, sizeof(*q));
    q->numero = numeroDoQuadro;
    q->fase = gravando ? faseAtual : -1;
    inicioDoQuadro = inicioDaEtapa = agora;
    etapaAtual = ETAPA_LOGICA;
    if (temPlaca) {
        QuadroNaPlaca *p = &naPlaca[numeroDoQuadro % QUADROS_NA_PLACA];
        if (p->esperando) {
            lerDaPlaca(p, 1);   /* QUADROS_NA_PLACA quadros atrás: já chegou (senão, espera) */
        }
        p->numero = numeroDoQuadro;
        p->marcas = 0;
        carimbar(ETAPA_LOGICA);
    }
}

void etapaDoQuadro(EtapaDoQuadro etapa) {
    if (!medindo || etapa == etapaAtual) {
        return;
    }
    double agora = GetTime();
    historia[numeroDoQuadro % QUADROS_NA_MEMORIA].cpu[etapaAtual] += (float) ((agora - inicioDaEtapa) * 1000.0);
    inicioDaEtapa = agora;
    etapaAtual = etapa;
    if (temPlaca) {
        carimbar(etapa);
    }
}

void contarNoQuadro(ContaDoQuadro conta, int quanto) {
    if (medindo) {
        historia[numeroDoQuadro % QUADROS_NA_MEMORIA].contas[conta] += quanto;
    }
}

void terminarQuadroDeDesempenho(void) {
    if (!medindo) {
        return;
    }
    double agora = GetTime();
    historia[numeroDoQuadro % QUADROS_NA_MEMORIA].cpu[etapaAtual] += (float) ((agora - inicioDaEtapa) * 1000.0);
    inicioDaEtapa = agora;
    if (temPlaca) {
        carimbar(FIM_DO_QUADRO);
        naPlaca[numeroDoQuadro % QUADROS_NA_PLACA].esperando = 1;
    }
}

/* ------------------------------------------------------------------
 * As contas de um trecho de quadros
 * ------------------------------------------------------------------ */

typedef struct {
    int quadros;
    float media, mediana, p95, p99, maior, desvio;   /* ms */
    int trancos;                                       /* quadros com mais que o dobro da mediana */
    float cpu[TOTAL_ETAPAS], placa[TOTAL_ETAPAS], placaTotal;
    float contas[TOTAL_CONTAS];
    int comPlaca;
} Resumo;

static int compararFloat(const void *a, const void *b) {
    float x = *(const float *) a, y = *(const float *) b;
    return (x > y) - (x < y);
}

static Resumo resumir(const QuadroMedido *const *quadros, int total) {
    Resumo r;
    memset(&r, 0, sizeof(r));
    if (total <= 0) {
        return r;
    }
    float *ordenados = malloc(sizeof(float) * (size_t) total);
    if (ordenados == NULL) {
        return r;
    }
    double soma = 0.0, somaDosQuadrados = 0.0;
    for (int i = 0; i < total; i++) {
        const QuadroMedido *q = quadros[i];
        ordenados[i] = q->total;
        soma += q->total;
        somaDosQuadrados += (double) q->total * q->total;
        for (int e = 0; e < TOTAL_ETAPAS; e++) {
            r.cpu[e] += q->cpu[e];
            r.placa[e] += q->placa[e];
        }
        for (int c = 0; c < TOTAL_CONTAS; c++) {
            r.contas[c] += (float) q->contas[c];
        }
        if (q->temPlaca) {
            r.placaTotal += q->placaTotal;
            r.comPlaca++;
        }
    }
    qsort(ordenados, (size_t) total, sizeof(float), compararFloat);
    r.quadros = total;
    r.media = (float) (soma / total);
    r.desvio = (float) sqrt(fmax(somaDosQuadrados / total - (soma / total) * (soma / total), 0.0));
    r.mediana = ordenados[total / 2];
    r.p95 = ordenados[(int) (total * 0.95f)];
    r.p99 = ordenados[(int) (total * 0.99f)];
    r.maior = ordenados[total - 1];
    for (int i = 0; i < total; i++) {
        r.trancos += (ordenados[i] > 2.0f * r.mediana);
    }
    for (int e = 0; e < TOTAL_ETAPAS; e++) {
        r.cpu[e] /= (float) total;
        r.placa[e] /= (float) (r.comPlaca > 0 ? r.comPlaca : total);
    }
    r.placaTotal /= (float) (r.comPlaca > 0 ? r.comPlaca : 1);
    for (int c = 0; c < TOTAL_CONTAS; c++) {
        r.contas[c] /= (float) total;
    }
    free(ordenados);
    return r;
}

/* Os quadros já completos (com o total), os mais novos por último, até "quantos" */
static int ultimosQuadros(const QuadroMedido **saida, int quantos) {
    int total = 0;
    for (long n = numeroDoQuadro - 1; n >= primeiroDaSequencia && total < quantos; n--) {
        const QuadroMedido *q = quadroDeNumero(n);
        if (q == NULL || q->total <= 0.0f) {
            break;
        }
        total++;
    }
    for (int i = 0; i < total; i++) {
        saida[i] = quadroDeNumero(numeroDoQuadro - total + i);
    }
    return total;
}

/* ------------------------------------------------------------------
 * O gráfico
 * ------------------------------------------------------------------ */

#define GRAFICO_QUADROS     240
#define LARGURA_DO_PAINEL   344.0f
#define ALTURA_DO_GRAFICO   74.0f
#define QUADROS_DA_LEGENDA  60     /* a legenda é a média do último segundo (mais ou menos) */

/* Com vírgula, como se escreve aqui */
static const char *comVirgula(const char *texto) {
    static char saida[8][96];
    static int qual = 0;
    char *s = saida[qual];
    qual = (qual + 1) % 8;
    snprintf(s, 96, "%s", texto);
    for (char *c = s; *c; c++) {
        if (*c == '.') {
            *c = ',';
        }
    }
    return s;
}

int graficoDeDesempenhoLigado(int modo) {
    return (modo == 2) != trocadoNoF3;
}

static void desenharGrafico(void) {
    static const QuadroMedido *quadros[GRAFICO_QUADROS];
    int total = ultimosQuadros(quadros, GRAFICO_QUADROS);
    comecarInterface(0);
    float x = ((float) larguraVirtual() - LARGURA_DO_PAINEL) / 2.0f, y = 6.0f;
    float altura = 206.0f;
    DrawRectangleRounded((Rectangle){ x, y, LARGURA_DO_PAINEL, altura }, 0.06f, 6, COR_DO_FUNDO);
    float margem = 10.0f;
    if (total < 2) {
        escrever(FONTE_FORTE, "DESEMPENHO", x + margem, y + 8.0f, 12.0f, COR_DO_TEXTO_2);
        escrever(FONTE_TEXTO, "medindo...", x + margem, y + 26.0f, 12.0f, COR_DO_TEXTO_2);
        terminarInterface();
        return;
    }
    Resumo tudo = resumir(quadros, total);
    int naLegenda = total < QUADROS_DA_LEGENDA ? total : QUADROS_DA_LEGENDA;
    Resumo agora = resumir(quadros + (total - naLegenda), naLegenda);

    /* O cabeçalho: o FPS de agora e, dos últimos quadros, o 1% pior, a variação e o maior */
    escrever(FONTE_FORTE, "DESEMPENHO", x + margem, y + 7.0f, 11.0f, COR_DO_TEXTO_2);
    escreverADireita(FONTE_FORTE, comVirgula(TextFormat("%.0f FPS  %.1f ms", 1000.0f / fmaxf(agora.media, 0.01f), agora.media)),
                     x + LARGURA_DO_PAINEL - margem, y + 4.0f, 15.0f, COR_DO_TEXTO);
    escrever(FONTE_TEXTO, comVirgula(TextFormat("1%% pior %.0f FPS (%.1f ms)   desvio %.1f ms   maior %.1f ms   trancos %d",
                                                1000.0f / fmaxf(tudo.p99, 0.01f), tudo.p99, tudo.desvio, tudo.maior, tudo.trancos)),
             x + margem, y + 22.0f, 10.0f, COR_DO_TEXTO_2);

    /* A escala: a menor que cabe o 1% pior com folga */
    static const float ESCALAS[] = { 10.0f, 20.0f, 33.4f, 50.0f, 100.0f, 250.0f };
    float teto = ESCALAS[5];
    for (int i = 0; i < 6; i++) {
        if (tudo.p99 * 1.15f <= ESCALAS[i]) {
            teto = ESCALAS[i];
            break;
        }
    }
    float gx = x + margem, gy = y + 38.0f, gl = LARGURA_DO_PAINEL - 2.0f * margem - 26.0f;
    float porMs = ALTURA_DO_GRAFICO / teto;
    float coluna = gl / GRAFICO_QUADROS;
    /* As linhas de referência: 60, 120 e 200 FPS */
    static const float REFERENCIAS[] = { 1000.0f / 60.0f, 1000.0f / 120.0f, 1000.0f / 200.0f };
    static const char *ROTULOS[] = { "60", "120", "200" };
    float rotuloAntes = -100.0f;
    for (int i = 0; i < 3; i++) {
        if (REFERENCIAS[i] >= teto) {
            continue;
        }
        float ly = gy + ALTURA_DO_GRAFICO - REFERENCIAS[i] * porMs;
        DrawLineEx((Vector2){ gx, ly }, (Vector2){ gx + gl, ly }, 1.0f, COR_DA_GRADE);
        if (ly - rotuloAntes >= 10.0f) {   /* numa escala grande, as linhas de 120 e 200 ficam coladas: um rótulo só */
            escrever(FONTE_TEXTO, ROTULOS[i], gx + gl + 4.0f, ly - 6.0f, 9.0f, COR_DO_TEXTO_2);
            rotuloAntes = ly;
        }
    }
    DrawLineEx((Vector2){ gx, gy + ALTURA_DO_GRAFICO }, (Vector2){ gx + gl, gy + ALTURA_DO_GRAFICO }, 1.0f, COR_DA_GRADE);
    escrever(FONTE_TEXTO, comVirgula(TextFormat("%.0f ms", teto)), gx + gl + 4.0f, gy - 4.0f, 9.0f, COR_DO_TEXTO_2);

    /* Cada quadro é uma coluna: as etapas da CPU empilhadas, a espera em cinza até o total; a linha clara é
     * o tempo da placa de vídeo */
    float inicio = gx + gl - coluna * (float) total;
    for (int i = 0; i < total; i++) {
        const QuadroMedido *q = quadros[i];
        float cx = inicio + coluna * (float) i, base = gy + ALTURA_DO_GRAFICO;
        float empilhado = 0.0f;
        for (int e = 0; e < TOTAL_ETAPAS; e++) {
            float ms = (e == ETAPA_TROCA) ? fmaxf(q->total - empilhado, 0.0f) : q->cpu[e];
            float h = fminf(ms, teto - empilhado) * porMs;
            if (h <= 0.0f) {
                continue;
            }
            DrawRectangleRec((Rectangle){ cx, base - empilhado * porMs - h, coluna + 0.35f, h }, CORES_DAS_ETAPAS[e]);
            empilhado = fminf(empilhado + ms, teto);
        }
    }
    for (int i = 1; i < total; i++) {
        const QuadroMedido *a = quadros[i - 1], *b = quadros[i];
        if (!a->temPlaca || !b->temPlaca) {
            continue;
        }
        float ax = inicio + coluna * ((float) i - 0.5f), bx = ax + coluna;
        float ay = gy + ALTURA_DO_GRAFICO - fminf(a->placaTotal, teto) * porMs;
        float by = gy + ALTURA_DO_GRAFICO - fminf(b->placaTotal, teto) * porMs;
        DrawLineEx((Vector2){ ax, ay }, (Vector2){ bx, by }, 1.5f, COR_DO_TEXTO);
    }

    /* A legenda: cada etapa, com a média do último segundo na CPU e na placa */
    float ly = gy + ALTURA_DO_GRAFICO + 7.0f;
    escreverADireita(FONTE_TEXTO, temPlaca ? "CPU  /  placa (ms)" : "CPU (ms)", x + LARGURA_DO_PAINEL - margem, ly - 1.0f, 9.0f, COR_DO_TEXTO_2);
    for (int e = 0; e < TOTAL_ETAPAS; e++) {
        int linha = e % 5, col = e / 5;
        float ex = x + margem + col * 168.0f, ey = ly + 10.0f + linha * 11.0f;
        DrawRectangleRounded((Rectangle){ ex, ey + 2.0f, 8.0f, 8.0f }, 0.3f, 4, CORES_DAS_ETAPAS[e]);
        escrever(FONTE_TEXTO, NOMES_DAS_ETAPAS[e], ex + 12.0f, ey, 10.0f, COR_DO_TEXTO);
        const char *valor = (temPlaca && agora.comPlaca > 0)
                            ? TextFormat("%.2f / %.2f", agora.cpu[e], agora.placa[e]) : TextFormat("%.2f", agora.cpu[e]);
        escreverADireita(FONTE_TEXTO, comVirgula(valor), ex + 158.0f, ey, 10.0f, COR_DO_TEXTO_2);
    }
    /* A linha da placa, na legenda */
    float px = x + margem + 168.0f, py = ly + 10.0f + 4 * 11.0f;
    DrawLineEx((Vector2){ px, py + 6.0f }, (Vector2){ px + 8.0f, py + 6.0f }, 1.5f, COR_DO_TEXTO);
    escrever(FONTE_TEXTO, "Placa (total)", px + 12.0f, py, 10.0f, COR_DO_TEXTO);
    if (temPlaca && agora.comPlaca > 0) {
        escreverADireita(FONTE_TEXTO, comVirgula(TextFormat("%.2f", agora.placaTotal)), px + 158.0f, py, 10.0f, COR_DO_TEXTO_2);
    }

    /* As contas do quadro (média do último segundo) */
    escrever(FONTE_TEXTO,
             comVirgula(TextFormat("lâmpadas %.0f acesas, %.0f nos shaders   envios: lâmpadas %.1f, grade %.1f, luz %.1f   sombras refeitas %.2f",
                                   agora.contas[CONTA_LAMPADAS_ACESAS], agora.contas[CONTA_LAMPADAS_NOS_SHADERS],
                                   agora.contas[CONTA_ENVIOS_DE_LAMPADAS], agora.contas[CONTA_GRADES_MONTADAS],
                                   agora.contas[CONTA_ENVIOS_DA_LUZ], agora.contas[CONTA_SOMBRAS_REFEITAS])),
             x + margem, y + altura - 15.0f, 9.0f, COR_DO_TEXTO_2);
    terminarInterface();
}

void desenharDesempenhoNaTela(int modo) {
    graficoNaTela = graficoDeDesempenhoLigado(modo);
    if (graficoNaTela) {
        desenharGrafico();
    } else if (modo == 1) {
        comecarInterface(0);
        escrever(FONTE_FORTE, TextFormat("%d FPS", GetFPS()), larguraVirtual() - 78.0f, 8.0f, 16.0f, COR_AMARELO);
        terminarInterface();
    }
}

/* ------------------------------------------------------------------
 * A gravação
 * ------------------------------------------------------------------ */

void gravarDesempenho(const char *arquivo) {
    snprintf(arquivoDaGravacao, sizeof(arquivoDaGravacao), "%s", arquivo);
    totalGravados = 0;
    totalDeFases = 0;
    faseAtual = -1;
    gravando = 1;
}

void faseDaGravacao(const char *nome) {
    if (!gravando) {
        return;
    }
    faseAtual = -1;   /* sem nome: um trecho que não conta (o carro foi levado, a hora pulou) */
    if (nome != NULL) {
        for (int i = 0; i < totalDeFases && faseAtual < 0; i++) {
            if (strcmp(fases[i], nome) == 0) {
                faseAtual = i;
            }
        }
        if (faseAtual < 0 && totalDeFases < MAIS_FASES) {
            snprintf(fases[totalDeFases], sizeof(fases[0]), "%s", nome);
            faseAtual = totalDeFases++;
        }
    }
    /* Vale já para o quadro de agora: o quadro em que o carro é levado é desenhado no lugar novo */
    if (medindo) {
        historia[numeroDoQuadro % QUADROS_NA_MEMORIA].fase = faseAtual;
    }
}

static void escreverResumo(FILE *f, const char *nome, const Resumo *r) {
    fprintf(f, "%-14s %6d quadros | media %6.2f ms (%4.0f FPS) | mediana %6.2f | 1%% pior %6.2f ms (%4.0f FPS) | maior %7.2f | "
               "variacao +-%5.2f ms | trancos %d\n",
            nome, r->quadros, r->media, 1000.0f / fmaxf(r->media, 0.01f), r->mediana, r->p99, 1000.0f / fmaxf(r->p99, 0.01f),
            r->maior, r->desvio, r->trancos);
    fprintf(f, "%-14s   CPU:", "");
    for (int e = 0; e < TOTAL_ETAPAS; e++) {
        fprintf(f, " %s %.2f", CHAVES_DAS_ETAPAS[e], r->cpu[e]);
    }
    fprintf(f, "\n");
    if (r->comPlaca > 0) {
        fprintf(f, "%-14s placa:", "");
        for (int e = 0; e < TOTAL_ETAPAS; e++) {
            fprintf(f, " %s %.2f", CHAVES_DAS_ETAPAS[e], r->placa[e]);
        }
        fprintf(f, " | total %.2f\n", r->placaTotal);
    }
    fprintf(f, "%-14s contas:", "");
    for (int c = 0; c < TOTAL_CONTAS; c++) {
        fprintf(f, " %s %.2f", CHAVES_DAS_CONTAS[c], r->contas[c]);
    }
    fprintf(f, "\n");
}

void terminarGravacao(void) {
    if (!gravando) {
        return;
    }
    if (temPlaca) {
        for (int i = 0; i < QUADROS_NA_PLACA; i++) {
            lerDaPlaca(&naPlaca[i], 1);
        }
    }
    gravando = 0;
    FILE *f = fopen(arquivoDaGravacao, "w");
    if (f != NULL) {
        fprintf(f, "fase,quadro,total_ms");
        for (int e = 0; e < TOTAL_ETAPAS; e++) fprintf(f, ",cpu_%s", CHAVES_DAS_ETAPAS[e]);
        for (int e = 0; e < TOTAL_ETAPAS; e++) fprintf(f, ",placa_%s", CHAVES_DAS_ETAPAS[e]);
        fprintf(f, ",placa_total");
        for (int c = 0; c < TOTAL_CONTAS; c++) fprintf(f, ",%s", CHAVES_DAS_CONTAS[c]);
        fprintf(f, "\n");
        for (int i = 0; i < totalGravados; i++) {
            const QuadroMedido *q = &gravados[i];
            fprintf(f, "%s,%ld,%.3f", fases[q->fase], q->numero, q->total);
            for (int e = 0; e < TOTAL_ETAPAS; e++) fprintf(f, ",%.3f", q->cpu[e]);
            for (int e = 0; e < TOTAL_ETAPAS; e++) fprintf(f, ",%.3f", q->temPlaca ? q->placa[e] : -1.0f);
            fprintf(f, ",%.3f", q->temPlaca ? q->placaTotal : -1.0f);
            for (int c = 0; c < TOTAL_CONTAS; c++) fprintf(f, ",%d", q->contas[c]);
            fprintf(f, "\n");
        }
        fclose(f);
    }
    /* O resumo de cada fase, no log e num .txt ao lado */
    char caminhoDoResumo[300];
    snprintf(caminhoDoResumo, sizeof(caminhoDoResumo), "%s.txt", arquivoDaGravacao);
    FILE *r = fopen(caminhoDoResumo, "w");
    const QuadroMedido **daFase = malloc(sizeof(QuadroMedido *) * (size_t) (totalGravados > 0 ? totalGravados : 1));
    for (int fase = 0; fase < totalDeFases && daFase != NULL; fase++) {
        int n = 0;
        for (int i = 0; i < totalGravados; i++) {
            if (gravados[i].fase == fase && gravados[i].total > 0.0f) {
                daFase[n++] = &gravados[i];
            }
        }
        Resumo resumo = resumir(daFase, n);
        escreverResumo(stdout, fases[fase], &resumo);
        if (r != NULL) {
            escreverResumo(r, fases[fase], &resumo);
        }
    }
    fflush(stdout);
    free(daFase);
    if (r != NULL) {
        fclose(r);
    }
    TraceLog(LOG_INFO, "DESEMPENHO: %d quadros gravados em %s (resumo em %s)", totalGravados, arquivoDaGravacao, caminhoDoResumo);
    free(gravados);
    gravados = NULL;
    totalGravados = cabemGravados = 0;
}
