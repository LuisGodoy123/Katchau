/*
 * hora.c
 * ------------------------------------------------------------------
 * O relógio do passeio e a luz de cada hora (veja hora.h).
 *
 * O dia de 48 minutos não anda igual: o dia (7h30 às 18h) leva 28 minutos,
 * o pôr do sol (18h às 20h30) leva 6, a noite (20h30 às 5h30) leva 10 e o
 * amanhecer (5h30 às 7h30) leva 4. A luz sai de uma tabela de horas-chave
 * (noite, antes do sol, nascer, manhã, dia, tarde, pôr do sol, sol que se
 * foi, crepúsculo), misturando as duas mais perto da hora de agora.
 *
 * O sol nasce no leste (+x, a ponta da rodovia) e se põe no oeste (-x),
 * no fim da rua principal. Às 11h29 ele está exatamente onde sempre esteve
 * (a luz das corridas), e ao meio-dia passa a 61° de altura.
 * ------------------------------------------------------------------
 */

#include "hora.h"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "raymath.h"
#include "rlgl.h"

#define HORA_DE_COMECO        10.0f
#define NASCE_AS              6.5f      /* horas: o sol no horizonte, nascendo... */
#define POE_AS                19.5f     /* ...e se pondo */
#define MAIS_SHADERS          24

/* As partes do dia: de que hora a que hora, e em quantos minutos de verdade */
typedef struct {
    float de, ate;          /* horas (ate pode passar de 24: a noite atravessa a meia-noite) */
    float minutos;
} ParteDoDia;

static const ParteDoDia PARTES[] = {
    { 5.5f, 7.5f, 4.0f },     /* amanhecer */
    { 7.5f, 18.0f, 28.0f },   /* dia */
    { 18.0f, 20.5f, 6.0f },   /* pôr do sol */
    { 20.5f, 29.5f, 10.0f },  /* noite (até as 5h30 do dia seguinte) */
};
#define TOTAL_DE_PARTES (int) (sizeof(PARTES) / sizeof(PARTES[0]))

/* Uma hora-chave da luz. Cores em 0 a 1; os multiplicadores valem 1 ao meio-dia */
typedef struct {
    float hora;
    float base[3], ceu[3], sol[3];
    float sombra;
    float ceuDeBaixo[3], ceuDeCima[3];
    float horizonte[3], alto[3], nuvens[3], brilho[3], corDoSol[3];
    float misturaDoDia, estrelas, lua;
    float interior[3];
    float luzes;
} HoraChave;

static const HoraChave CHAVES[] = {
    /* noite: azulada, com a lua; dá para ver a rua */
    { 0.0f, { 0.34f, 0.35f, 0.53f }, { 0.23f, 0.25f, 0.45f }, { 0.24f, 0.28f, 0.46f }, 0.0f,
      { 0.04f, 0.04f, 0.09f }, { 0.16f, 0.15f, 0.32f },
      { 0.16f, 0.13f, 0.32f }, { 0.06f, 0.05f, 0.16f }, { 0.10f, 0.09f, 0.18f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f },
      0.0f, 1.0f, 1.0f, { 0.62f, 0.56f, 0.50f }, 1.0f },
    { 4.8f, { 0.34f, 0.35f, 0.53f }, { 0.23f, 0.25f, 0.45f }, { 0.24f, 0.28f, 0.46f }, 0.0f,
      { 0.04f, 0.04f, 0.09f }, { 0.16f, 0.15f, 0.32f },
      { 0.16f, 0.13f, 0.32f }, { 0.06f, 0.05f, 0.16f }, { 0.10f, 0.09f, 0.18f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f },
      0.0f, 1.0f, 1.0f, { 0.62f, 0.56f, 0.50f }, 1.0f },
    /* antes do sol: o horizonte começa a clarear, rosado */
    { 5.6f, { 0.42f, 0.42f, 0.58f }, { 0.36f, 0.38f, 0.58f }, { 0.18f, 0.18f, 0.28f }, 0.0f,
      { 0.20f, 0.16f, 0.20f }, { 0.30f, 0.34f, 0.55f },
      { 0.62f, 0.42f, 0.48f }, { 0.16f, 0.18f, 0.40f }, { 0.45f, 0.35f, 0.45f }, { 0.25f, 0.12f, 0.10f }, { 0.0f, 0.0f, 0.0f },
      0.0f, 0.35f, 0.3f, { 0.75f, 0.70f, 0.65f }, 0.8f },
    /* o sol nascendo */
    { 6.4f, { 0.62f, 0.58f, 0.62f }, { 0.58f, 0.55f, 0.66f }, { 0.95f, 0.55f, 0.32f }, 0.3f,
      { 0.50f, 0.40f, 0.36f }, { 0.55f, 0.62f, 0.85f },
      { 1.0f, 0.62f, 0.42f }, { 0.36f, 0.48f, 0.75f }, { 1.0f, 0.70f, 0.58f }, { 0.55f, 0.28f, 0.12f }, { 1.0f, 0.55f, 0.25f },
      0.1f, 0.0f, 0.0f, { 0.88f, 0.82f, 0.76f }, 0.2f },
    /* a manhã cedo, ainda dourada */
    { 7.4f, { 0.86f, 0.84f, 0.84f }, { 0.85f, 0.86f, 0.92f }, { 1.05f, 0.85f, 0.66f }, 0.85f,
      { 0.50f, 0.50f, 0.50f }, { 0.62f, 0.78f, 0.98f },
      { 0.85f, 0.78f, 0.72f }, { 0.30f, 0.50f, 0.82f }, { 1.0f, 0.92f, 0.85f }, { 0.15f, 0.07f, 0.02f }, { 1.0f, 0.85f, 0.6f },
      0.75f, 0.0f, 0.0f, { 0.97f, 0.97f, 0.97f }, 0.0f },
    /* o dia: a luz de sempre */
    { 8.4f, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, 1.0f,
      { 0.50f, 0.52f, 0.52f }, { 0.62f, 0.80f, 1.00f },
      { 0.70f, 0.82f, 0.95f }, { 0.20f, 0.44f, 0.77f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.95f, 0.85f },
      1.0f, 0.0f, 0.0f, { 1.0f, 1.0f, 1.0f }, 0.0f },
    { 17.0f, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, 1.0f,
      { 0.50f, 0.52f, 0.52f }, { 0.62f, 0.80f, 1.00f },
      { 0.70f, 0.82f, 0.95f }, { 0.20f, 0.44f, 0.77f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.95f, 0.85f },
      1.0f, 0.0f, 0.0f, { 1.0f, 1.0f, 1.0f }, 0.0f },
    /* a tarde dourada */
    { 18.2f, { 0.92f, 0.86f, 0.82f }, { 0.90f, 0.84f, 0.84f }, { 1.15f, 0.88f, 0.62f }, 0.95f,
      { 0.60f, 0.50f, 0.42f }, { 0.62f, 0.72f, 0.92f },
      { 1.0f, 0.78f, 0.55f }, { 0.28f, 0.42f, 0.75f }, { 1.0f, 0.85f, 0.70f }, { 0.30f, 0.14f, 0.04f }, { 1.0f, 0.75f, 0.45f },
      0.55f, 0.0f, 0.0f, { 0.95f, 0.95f, 0.95f }, 0.0f },
    /* o pôr do sol: laranja, as luzes da cidade acendendo */
    { 19.0f, { 0.66f, 0.52f, 0.54f }, { 0.62f, 0.50f, 0.62f }, { 1.30f, 0.62f, 0.28f }, 0.55f,
      { 0.65f, 0.35f, 0.22f }, { 0.40f, 0.40f, 0.68f },
      { 1.0f, 0.50f, 0.26f }, { 0.26f, 0.26f, 0.55f }, { 1.0f, 0.55f, 0.45f }, { 0.70f, 0.30f, 0.10f }, { 1.0f, 0.45f, 0.15f },
      0.0f, 0.0f, 0.0f, { 0.85f, 0.78f, 0.70f }, 0.5f },
    /* o sol acabou de ir: o horizonte ainda vermelho, o alto já roxo */
    { 19.7f, { 0.48f, 0.40f, 0.52f }, { 0.44f, 0.36f, 0.56f }, { 0.10f, 0.06f, 0.08f }, 0.0f,
      { 0.30f, 0.18f, 0.20f }, { 0.25f, 0.22f, 0.45f },
      { 0.80f, 0.38f, 0.32f }, { 0.16f, 0.14f, 0.38f }, { 0.62f, 0.34f, 0.40f }, { 0.45f, 0.15f, 0.08f }, { 0.9f, 0.3f, 0.1f },
      0.0f, 0.15f, 0.2f, { 0.72f, 0.66f, 0.60f }, 1.0f },
    /* o crepúsculo, roxo-azulado, como no filme */
    { 20.4f, { 0.36f, 0.34f, 0.52f }, { 0.28f, 0.26f, 0.48f }, { 0.20f, 0.22f, 0.38f }, 0.0f,
      { 0.08f, 0.07f, 0.14f }, { 0.18f, 0.16f, 0.36f },
      { 0.30f, 0.20f, 0.40f }, { 0.09f, 0.08f, 0.24f }, { 0.20f, 0.15f, 0.28f }, { 0.10f, 0.04f, 0.06f }, { 0.0f, 0.0f, 0.0f },
      0.0f, 0.7f, 0.8f, { 0.64f, 0.58f, 0.52f }, 1.0f },
    { 21.0f, { 0.34f, 0.35f, 0.53f }, { 0.23f, 0.25f, 0.45f }, { 0.24f, 0.28f, 0.46f }, 0.0f,
      { 0.04f, 0.04f, 0.09f }, { 0.16f, 0.15f, 0.32f },
      { 0.16f, 0.13f, 0.32f }, { 0.06f, 0.05f, 0.16f }, { 0.10f, 0.09f, 0.18f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f },
      0.0f, 1.0f, 1.0f, { 0.62f, 0.56f, 0.50f }, 1.0f },
};
#define TOTAL_DE_CHAVES (int) (sizeof(CHAVES) / sizeof(CHAVES[0]))

typedef struct {
    Shader shader;
    int locSol, locBase, locCeu, locLuzDoSol, locSombra, locCeuDeBaixo, locCeuDeCima;
    int locTotalDeLampadas, locLampadas, locCoresDasLampadas, locVistaDasLampadas;
    int locGradeMedidas, locGrade, locLista;
} ShaderDoDia;

static float hora = HORA_DE_COMECO;
static int relogioIniciado = 0;
static LuzDoDia luzAgora, meioDia, aplicada;
static int meioDiaPronto = 0;
static float horaDaLuz = -1.0f;
static ShaderDoDia shaders[MAIS_SHADERS];
static int totalDeShaders = 0;
static int lampadasMandadas = -1;   /* quantas lâmpadas os shaders têm agora (-1: não se sabe) */

/* A direção de sempre do sol (a das corridas): é onde ele está às 11h29 */
static Vector3 solDeSempre(void) {
    return Vector3Normalize((Vector3){ 0.35f, 0.8f, 0.45f });
}

/*
 * solNaHora
 * O sol anda num círculo que nasce no +x e se põe no -x, passando pelo sol de sempre. Abaixo
 * do horizonte, o círculo continua (de noite ele está embaixo da terra).
 * Recebe: a hora.
 * Devolve: a direção do sol, normalizada.
 */
static Vector3 solNaHora(float h) {
    Vector3 sempre = solDeSempre();
    Vector3 leste = { 1.0f, 0.0f, 0.0f };
    /* sempre = leste * cos(a) + alto * sin(a): o "alto" do círculo, perpendicular ao leste */
    float cosA = Vector3DotProduct(sempre, leste);
    Vector3 alto = Vector3Normalize(Vector3Subtract(sempre, Vector3Scale(leste, cosA)));
    float angulo = PI * (h - NASCE_AS) / (POE_AS - NASCE_AS);
    return Vector3Add(Vector3Scale(leste, cosf(angulo)), Vector3Scale(alto, sinf(angulo)));
}

static Vector3 luaNoCeu(void) {
    return Vector3Normalize((Vector3){ 0.25f, 0.75f, -0.6f });
}

static Vector3 cor3(const float c[3]) {
    return (Vector3){ c[0], c[1], c[2] };
}

static Vector3 misturar3(const float a[3], const float b[3], float t) {
    return (Vector3){ a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t, a[2] + (b[2] - a[2]) * t };
}

static float suave(float t) {
    return t * t * (3.0f - 2.0f * t);
}

static float degrau(float de, float ate, float x) {
    float t = Clamp((x - de) / (ate - de), 0.0f, 1.0f);
    return suave(t);
}

/*
 * montarLuz
 * A luz de uma hora: as duas horas-chave em volta dela, misturadas, e o sol (ou a lua).
 * Recebe: a hora e onde guardar.
 * Devolve: nada.
 */
static void montarLuz(float h, LuzDoDia *luz) {
    int a = TOTAL_DE_CHAVES - 1, b = 0;
    for (int i = 0; i < TOTAL_DE_CHAVES; i++) {
        if (CHAVES[i].hora <= h) {
            a = i;
        }
    }
    b = (a + 1) % TOTAL_DE_CHAVES;
    float de = CHAVES[a].hora, ate = CHAVES[b].hora;
    if (ate <= de) {
        ate += 24.0f;
    }
    float agora = (h < de) ? h + 24.0f : h;
    float t = suave(Clamp((agora - de) / (ate - de), 0.0f, 1.0f));
    const HoraChave *ka = &CHAVES[a], *kb = &CHAVES[b];

    luz->base = misturar3(ka->base, kb->base, t);
    luz->ceu = misturar3(ka->ceu, kb->ceu, t);
    luz->luzDoSol = misturar3(ka->sol, kb->sol, t);
    luz->ceuDeBaixo = misturar3(ka->ceuDeBaixo, kb->ceuDeBaixo, t);
    luz->ceuDeCima = misturar3(ka->ceuDeCima, kb->ceuDeCima, t);
    luz->horizonte = misturar3(ka->horizonte, kb->horizonte, t);
    luz->alto = misturar3(ka->alto, kb->alto, t);
    luz->nuvens = misturar3(ka->nuvens, kb->nuvens, t);
    luz->brilho = misturar3(ka->brilho, kb->brilho, t);
    luz->corDoSol = misturar3(ka->corDoSol, kb->corDoSol, t);
    luz->interior = misturar3(ka->interior, kb->interior, t);
    luz->misturaDoDia = ka->misturaDoDia + (kb->misturaDoDia - ka->misturaDoDia) * t;
    luz->estrelas = ka->estrelas + (kb->estrelas - ka->estrelas) * t;
    luz->lua = ka->lua + (kb->lua - ka->lua) * t;
    luz->luzesDaCidade = ka->luzes + (kb->luzes - ka->luzes) * t;

    /* De dia a luz vem do sol (nunca de baixo do horizonte); de noite, da lua. A troca é perto do
     * horizonte, quando a luz direta quase não existe */
    Vector3 sol = solNaHora(h);
    Vector3 lua = luaNoCeu();
    Vector3 solAcima = Vector3Normalize((Vector3){ sol.x, fmaxf(sol.y, 0.06f), sol.z });
    float doSol = degrau(-0.04f, 0.06f, sol.y);
    luz->sol = Vector3Normalize(Vector3Lerp(lua, solAcima, doSol));
    luz->sombra = (ka->sombra + (kb->sombra - ka->sombra) * t) * degrau(0.03f, 0.12f, sol.y);
    luz->solNoCeu = sol;
    luz->luaNoCeu = lua;
}

static void prepararMeioDia(void) {
    if (meioDiaPronto) {
        return;
    }
    meioDiaPronto = 1;
    const HoraChave *k = &CHAVES[5];   /* o dia */
    meioDia.sol = solDeSempre();
    meioDia.base = cor3(k->base);
    meioDia.ceu = cor3(k->ceu);
    meioDia.luzDoSol = cor3(k->sol);
    meioDia.sombra = 1.0f;
    meioDia.ceuDeBaixo = cor3(k->ceuDeBaixo);
    meioDia.ceuDeCima = cor3(k->ceuDeCima);
    meioDia.solNoCeu = solDeSempre();
    meioDia.luaNoCeu = luaNoCeu();
    meioDia.horizonte = cor3(k->horizonte);
    meioDia.alto = cor3(k->alto);
    meioDia.nuvens = cor3(k->nuvens);
    meioDia.brilho = cor3(k->brilho);
    meioDia.corDoSol = cor3(k->corDoSol);
    meioDia.misturaDoDia = 1.0f;
    meioDia.estrelas = 0.0f;
    meioDia.lua = 0.0f;
    meioDia.interior = cor3(k->interior);
    meioDia.luzesDaCidade = 0.0f;
    aplicada = meioDia;
}

/* ------------------------------------------------------------------
 * O relógio
 * ------------------------------------------------------------------ */

void iniciarRelogio(void) {
    if (!relogioIniciado) {
        relogioIniciado = 1;
        hora = HORA_DE_COMECO;
    }
}

/*
 * avancarRelogio
 * Anda o relógio pelos segundos de verdade, cada parte do dia no passo dela.
 * Recebe: os segundos.
 * Devolve: nada.
 */
void avancarRelogio(float segundos) {
    for (int volta = 0; segundos > 0.0f && volta < 8; volta++) {
        float h = hora;
        const ParteDoDia *parte = &PARTES[TOTAL_DE_PARTES - 1];
        for (int i = 0; i < TOTAL_DE_PARTES; i++) {
            float de = PARTES[i].de, ate = PARTES[i].ate;
            float hh = (h < de) ? h + 24.0f : h;
            if (hh >= de && hh < ate) {
                parte = &PARTES[i];
                h = hh;
                break;
            }
        }
        float horasPorSegundo = (parte->ate - parte->de) / (parte->minutos * 60.0f);
        float faltam = parte->ate - h;   /* horas até o fim desta parte */
        float anda = segundos * horasPorSegundo;
        if (anda < faltam) {
            h += anda;
            segundos = 0.0f;
        } else {
            h = parte->ate;
            segundos -= faltam / horasPorSegundo;
        }
        hora = fmodf(h, 24.0f);
    }
}

float horaDoDia(void) {
    return hora;
}

void definirHoraDoDia(float novaHora) {
    hora = fmodf(fmodf(novaHora, 24.0f) + 24.0f, 24.0f);
    relogioIniciado = 1;
}

/*
 * sortearHoraDoDia
 * Uma hora qualquer, sorteada nos 48 minutos do dia: cada parte sai na proporção dos minutos dela (o dia, que
 * dura mais, sai mais; a noite, o pôr e o nascer do sol também aparecem).
 * Recebe: nada.
 * Devolve: nada.
 */
void sortearHoraDoDia(void) {
    float minutosDoDia = 0.0f;
    for (int i = 0; i < TOTAL_DE_PARTES; i++) {
        minutosDoDia += PARTES[i].minutos;
    }
    hora = PARTES[0].de;
    relogioIniciado = 1;
    avancarRelogio((float) GetRandomValue(0, (int) (minutosDoDia * 60.0f) - 1) + (float) GetRandomValue(0, 99) / 100.0f);
}

void escreverHora(char *texto, int tamanho, float h) {
    int minutos = (int) floorf(fmodf(h, 24.0f) * 60.0f);
    snprintf(texto, (size_t) tamanho, "%02d:%02d", (minutos / 60) % 24, minutos % 60);
}

/* ------------------------------------------------------------------
 * A luz
 * ------------------------------------------------------------------ */

const LuzDoDia *luzDaHora(void) {
    prepararMeioDia();
    if (hora != horaDaLuz) {
        horaDaLuz = hora;
        montarLuz(hora, &luzAgora);
    }
    return &luzAgora;
}

const LuzDoDia *luzDoMeioDia(void) {
    prepararMeioDia();
    return &meioDia;
}

const LuzDoDia *luzAplicada(void) {
    prepararMeioDia();
    return &aplicada;
}

static void mandarAoShader(const ShaderDoDia *s, const LuzDoDia *luz) {
    if (s->locSol >= 0) SetShaderValue(s->shader, s->locSol, &luz->sol, SHADER_UNIFORM_VEC3);
    if (s->locBase >= 0) SetShaderValue(s->shader, s->locBase, &luz->base, SHADER_UNIFORM_VEC3);
    if (s->locCeu >= 0) SetShaderValue(s->shader, s->locCeu, &luz->ceu, SHADER_UNIFORM_VEC3);
    if (s->locLuzDoSol >= 0) SetShaderValue(s->shader, s->locLuzDoSol, &luz->luzDoSol, SHADER_UNIFORM_VEC3);
    if (s->locSombra >= 0) SetShaderValue(s->shader, s->locSombra, &luz->sombra, SHADER_UNIFORM_FLOAT);
    if (s->locCeuDeBaixo >= 0) SetShaderValue(s->shader, s->locCeuDeBaixo, &luz->ceuDeBaixo, SHADER_UNIFORM_VEC3);
    if (s->locCeuDeCima >= 0) SetShaderValue(s->shader, s->locCeuDeCima, &luz->ceuDeCima, SHADER_UNIFORM_VEC3);
}

void registrarShaderDoDia(Shader shader) {
    prepararMeioDia();
    if (totalDeShaders == MAIS_SHADERS || shader.id == 0) {
        return;
    }
    ShaderDoDia *s = &shaders[totalDeShaders++];
    s->shader = shader;
    s->locSol = GetShaderLocation(shader, "solDoDia");
    s->locBase = GetShaderLocation(shader, "luzDeBase");
    s->locCeu = GetShaderLocation(shader, "luzDoCeu");
    s->locLuzDoSol = GetShaderLocation(shader, "luzDoSol");
    s->locSombra = GetShaderLocation(shader, "forcaDaSombra");
    s->locCeuDeBaixo = GetShaderLocation(shader, "ceuDeBaixo");
    s->locCeuDeCima = GetShaderLocation(shader, "ceuDeCima");
    s->locTotalDeLampadas = GetShaderLocation(shader, "totalDeLampadas");
    s->locLampadas = GetShaderLocation(shader, "lampadas");
    s->locCoresDasLampadas = GetShaderLocation(shader, "coresDasLampadas");
    s->locVistaDasLampadas = GetShaderLocation(shader, "vistaDasLampadas");
    s->locGradeMedidas = GetShaderLocation(shader, "gradeMedidas");
    s->locGrade = GetShaderLocation(shader, "gradeDasLampadas");
    s->locLista = GetShaderLocation(shader, "listaDasLampadas");
    mandarAoShader(s, &aplicada);
    int slotDaGrade = SLOT_DA_GRADE_DAS_LAMPADAS, slotDaLista = SLOT_DA_LISTA_DAS_LAMPADAS;
    if (s->locGrade >= 0) SetShaderValue(shader, s->locGrade, &slotDaGrade, SHADER_UNIFORM_INT);
    if (s->locLista >= 0) SetShaderValue(shader, s->locLista, &slotDaLista, SHADER_UNIFORM_INT);
    lampadasMandadas = -1;   /* o shader novo ainda não recebeu as lâmpadas: a próxima chamada manda a todos */
    int nenhuma = 0, slot = SLOT_DA_VISTA_DAS_LAMPADAS;
    if (s->locTotalDeLampadas >= 0) SetShaderValue(shader, s->locTotalDeLampadas, &nenhuma, SHADER_UNIFORM_INT);
    if (s->locVistaDasLampadas >= 0) SetShaderValue(shader, s->locVistaDasLampadas, &slot, SHADER_UNIFORM_INT);
}

/* ---------- A grade das lâmpadas ----------
 * O laço do shader percorria as 64 lâmpadas em cada pixel (de noite, a tela inteira), quase todas longe
 * demais para contar. A grade vê o mapa de cima: cada célula guarda só as lâmpadas que a alcançam. */
#define MAIS_ENTRADAS_NA_LISTA (LARGURA_DA_LISTA * ALTURA_DA_LISTA)
#define LADO_MINIMO_DA_CELULA  4.0f   /* metros */
static Texture2D imagemDaGrade, imagemDaLista;
static int temGrade = 0, tentouCriarGrade = 0;
static float lugaresMandados[MAIS_LAMPADAS * 4], coresMandadas[MAIS_LAMPADAS * 4];

static void criarImagensDaGrade(void) {
    tentouCriarGrade = 1;
    if (getenv("KATCHAU_SEM_GRADE") != NULL) {
        return;   /* para comparar: o laço de sempre */
    }
    static float vazia[LADO_DA_GRADE * LADO_DA_GRADE];
    static float vaziaDaLista[MAIS_ENTRADAS_NA_LISTA];
    Image g = { vazia, LADO_DA_GRADE, LADO_DA_GRADE, 1, PIXELFORMAT_UNCOMPRESSED_R32 };
    Image l = { vaziaDaLista, LARGURA_DA_LISTA, ALTURA_DA_LISTA, 1, PIXELFORMAT_UNCOMPRESSED_R32 };
    imagemDaGrade = LoadTextureFromImage(g);
    imagemDaLista = LoadTextureFromImage(l);
    if (imagemDaGrade.id == 0 || imagemDaLista.id == 0) {
        TraceLog(LOG_WARNING, "HORA: sem a grade das lâmpadas: o shader percorre todas");
        return;
    }
    SetTextureFilter(imagemDaGrade, TEXTURE_FILTER_POINT);
    SetTextureFilter(imagemDaLista, TEXTURE_FILTER_POINT);
    SetTextureWrap(imagemDaGrade, TEXTURE_WRAP_CLAMP);
    SetTextureWrap(imagemDaLista, TEXTURE_WRAP_CLAMP);
    temGrade = 1;
}

/*
 * montarGrade
 * Põe cada lâmpada nas células que o quadrado dela (x e z, o raio para cada lado) toca, na ordem das lâmpadas,
 * e manda as duas imagens para a placa.
 * Recebe: os lugares (x, y, z, raio), quantas lâmpadas e onde guardar as medidas (canto x, canto z, 1 / lado, 1).
 * Devolve: 1 se montou; 0 se a lista não coube (aí o shader percorre todas as lâmpadas).
 */
static int montarGrade(const float *lugares, int total, Vector4 *medidas) {
    static float tabela[LADO_DA_GRADE * LADO_DA_GRADE], lista[MAIS_ENTRADAS_NA_LISTA];
    static int quantas[LADO_DA_GRADE * LADO_DA_GRADE], comeco[LADO_DA_GRADE * LADO_DA_GRADE];
    float menorX = 1e30f, menorZ = 1e30f, maiorX = -1e30f, maiorZ = -1e30f;
    for (int i = 0; i < total; i++) {
        menorX = fminf(menorX, lugares[i * 4] - lugares[i * 4 + 3]);
        maiorX = fmaxf(maiorX, lugares[i * 4] + lugares[i * 4 + 3]);
        menorZ = fminf(menorZ, lugares[i * 4 + 2] - lugares[i * 4 + 3]);
        maiorZ = fmaxf(maiorZ, lugares[i * 4 + 2] + lugares[i * 4 + 3]);
    }
    float lado = fmaxf(fmaxf(maiorX - menorX, maiorZ - menorZ) / (float) LADO_DA_GRADE * 1.001f, LADO_MINIMO_DA_CELULA);
tentar:
    for (int c = 0; c < LADO_DA_GRADE * LADO_DA_GRADE; c++) {
        quantas[c] = 0;
    }
    /* Duas passadas: contar quantas lâmpadas caem em cada célula, e depois guardá-las uma depois da outra */
    for (int passada = 0; passada < 2; passada++) {
        if (passada == 1) {
            int soma = 0;
            for (int c = 0; c < LADO_DA_GRADE * LADO_DA_GRADE; c++) {
                comeco[c] = soma;
                soma += quantas[c];
                quantas[c] = 0;
            }
            if (soma > MAIS_ENTRADAS_NA_LISTA) {
                /* A lista não cabe (lâmpadas de raio grande tocam muitas células pequenas): células maiores */
                if (lado > 4000.0f) {
                    return 0;
                }
                lado *= 1.4f;
                goto tentar;
            }
        }
        for (int i = 0; i < total; i++) {
            float x = lugares[i * 4], z = lugares[i * 4 + 2], r = lugares[i * 4 + 3];
            int x0 = (int) floorf((x - r - menorX) / lado), x1 = (int) floorf((x + r - menorX) / lado);
            int z0 = (int) floorf((z - r - menorZ) / lado), z1 = (int) floorf((z + r - menorZ) / lado);
            x0 = (x0 < 0) ? 0 : x0;
            z0 = (z0 < 0) ? 0 : z0;
            x1 = (x1 >= LADO_DA_GRADE) ? LADO_DA_GRADE - 1 : x1;
            z1 = (z1 >= LADO_DA_GRADE) ? LADO_DA_GRADE - 1 : z1;
            for (int cz = z0; cz <= z1; cz++) {
                for (int cx = x0; cx <= x1; cx++) {
                    int c = cz * LADO_DA_GRADE + cx;
                    if (passada == 1) {
                        lista[comeco[c] + quantas[c]] = (float) i;
                    }
                    quantas[c]++;
                }
            }
        }
    }
    for (int c = 0; c < LADO_DA_GRADE * LADO_DA_GRADE; c++) {
        tabela[c] = (float) (comeco[c] * 64 + quantas[c]);
    }
    UpdateTexture(imagemDaGrade, tabela);
    UpdateTexture(imagemDaLista, lista);
    *medidas = (Vector4){ menorX, menorZ, 1.0f / lado, 1.0f };
    return 1;
}

/*
 * ligarGradeDasLampadas
 * Liga as duas imagens da grade nas unidades delas (como a vista das lâmpadas). Chamar a cada quadro.
 */
void ligarGradeDasLampadas(void) {
    if (!temGrade) {
        return;
    }
    rlActiveTextureSlot(SLOT_DA_LISTA_DAS_LAMPADAS);
    rlEnableTexture(imagemDaLista.id);
    rlActiveTextureSlot(SLOT_DA_GRADE_DAS_LAMPADAS);
    rlEnableTexture(imagemDaGrade.id);
    rlActiveTextureSlot(0);
}

/*
 * aplicarLampadas
 * Manda as lâmpadas aos shaders registrados que as usam. Se são as mesmas do quadro anterior, não manda nada.
 * Recebe: a lista (pode ser NULL com total 0) e quantas.
 * Devolve: nada.
 */
void aplicarLampadas(const Lampada *lista, int total) {
    if (total > MAIS_LAMPADAS) {
        total = MAIS_LAMPADAS;
    }
    if (total < 0) {
        total = 0;
    }
    if (total == 0 && lampadasMandadas == 0) {
        return;   /* de dia, nada muda de um quadro para o outro */
    }
    float lugares[MAIS_LAMPADAS * 4], cores[MAIS_LAMPADAS * 4];
    for (int i = 0; i < total; i++) {
        lugares[i * 4] = lista[i].lugar.x;
        lugares[i * 4 + 1] = lista[i].lugar.y;
        lugares[i * 4 + 2] = lista[i].lugar.z;
        lugares[i * 4 + 3] = fmaxf(lista[i].raio, 0.5f);
        cores[i * 4] = lista[i].cor.x;
        cores[i * 4 + 1] = lista[i].cor.y;
        cores[i * 4 + 2] = lista[i].cor.z;
        /* a força (até 7,9) e, por cima, o quadro da vista: forca + 8 × (quadro + 1) */
        cores[i * 4 + 3] = fminf(fmaxf(lista[i].forca, 0.0f), 7.9f) + 8.0f * (float) (lista[i].quadro + 1);
    }
    if (total == lampadasMandadas && memcmp(lugares, lugaresMandados, sizeof(float) * 4 * (size_t) total) == 0
        && memcmp(cores, coresMandadas, sizeof(float) * 4 * (size_t) total) == 0) {
        return;   /* os mesmos valores já estão nos shaders */
    }
    memcpy(lugaresMandados, lugares, sizeof(float) * 4 * (size_t) total);
    memcpy(coresMandadas, cores, sizeof(float) * 4 * (size_t) total);
    lampadasMandadas = total;

    if (total > 0 && !tentouCriarGrade) {
        criarImagensDaGrade();
    }
    Vector4 medidas = { 0.0f, 0.0f, 0.0f, 0.0f };   /* o último valor 0: o shader percorre todas as lâmpadas */
    if (total > 0 && temGrade) {
        if (!montarGrade(lugares, total, &medidas)) {
            medidas = (Vector4){ 0.0f, 0.0f, 0.0f, 0.0f };
        }
    }
    for (int i = 0; i < totalDeShaders; i++) {
        const ShaderDoDia *s = &shaders[i];
        if (s->locTotalDeLampadas < 0) {
            continue;
        }
        SetShaderValue(s->shader, s->locTotalDeLampadas, &total, SHADER_UNIFORM_INT);
        if (s->locGradeMedidas >= 0) SetShaderValue(s->shader, s->locGradeMedidas, &medidas, SHADER_UNIFORM_VEC4);
        if (total > 0 && s->locLampadas >= 0) SetShaderValueV(s->shader, s->locLampadas, lugares, SHADER_UNIFORM_VEC4, total);
        if (total > 0 && s->locCoresDasLampadas >= 0) {
            SetShaderValueV(s->shader, s->locCoresDasLampadas, cores, SHADER_UNIFORM_VEC4, total);
        }
    }
}

/*
 * aplicarLuzDoDia
 * Manda a luz a todos os shaders registrados (só quando ela muda).
 * Recebe: a luz (NULL: a do meio-dia).
 * Devolve: nada.
 */
void aplicarLuzDoDia(const LuzDoDia *luz) {
    prepararMeioDia();
    if (luz == NULL) {
        luz = &meioDia;
    }
    if (memcmp(luz, &aplicada, sizeof(LuzDoDia)) == 0) {
        return;
    }
    aplicada = *luz;
    for (int i = 0; i < totalDeShaders; i++) {
        mandarAoShader(&shaders[i], &aplicada);
    }
}
