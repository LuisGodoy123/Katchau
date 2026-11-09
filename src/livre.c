/*
 * livre.c
 * ------------------------------------------------------------------
 * A física do carro do passeio livre. Veja livre.h.
 *
 * O carro guarda o "para cima" dele (a normal do chão embaixo das rodas) e
 * a frente. Tudo o que ele faz no chão (acelerar, frear, virar, derrapar)
 * acontece no plano desse chão, seja ele plano, rampa ou parede. A gravidade
 * entra pela parte dela que cai nesse plano: numa ladeira ela freia; numa
 * parede ela puxa o carro para baixo, e só rápido ele continua grudado.
 *
 * Em cada passo, com as rodas em alguma superfície:
 *   1) acelera, freia, vira e derrapa, no plano dela;
 *   2) olha à frente: uma parede entrando de raspão e rápido vira o chão novo
 *      (o wallride); de frente, ou devagar, é batida;
 *   3) anda, e as paredes em volta o empurram para fora;
 *   4) procura a superfície embaixo das rodas (ao longo do "para baixo" dele):
 *      gruda nela e acompanha a inclinação; se ela fugir rápido demais (o fim
 *      de uma rampa), o carro sai voando.
 * No ar: a gravidade puxa, o carro se endireita aos poucos e, ao bater em
 * alguma coisa, aterrissa, gruda numa parede ou quica.
 * ------------------------------------------------------------------
 */

#include <math.h>
#include <stddef.h>
#include "livre.h"

#define PASSO_MAXIMO         (1.0f / 120.0f)   /* segundos: o quadro é dividido em passos deste tamanho */
#define GRAVIDADE_LIVRE      9.8f
#define MAIS_PASSOS          64                /* teto de passos por quadro */

/* A aceleração, o freio, a ré, o nitro, o volante, a curva e o drift são os da corrida (dirigirSemPista,
 * em carro.c): o carro responde igual nos dois modos. Daqui para baixo, só o que a corrida não tem */
#define QUEDA_DO_TRANCO      4.0f      /* 1/s: o tranco lateral de uma batida some em ~1/4 s (como na corrida) */
#define GIRO_NO_AR           1.2f      /* rad/s: no ar, o volante ainda gira um pouco o carro */

/* Chão, rampas e paredes */
#define DEGRAU               2.0f      /* metros: o raio que procura o chão sai daqui de cima (sobe um degrau assim) */
#define GRUDA_NO_CHAO        0.6f      /* metros: descendo, se o chão está até aqui embaixo, o carro acompanha */
#define VIRA_COM_O_CHAO      14.0f     /* por segundo: o "para cima" segue o do chão novo */
#define MAIOR_DOBRA          62.0f     /* graus: uma quina mais fechada que isso não é chão, é beirada */
#define MAIOR_DOBRA_NA_PAREDE 78.0f    /* graus: a rocha é cheia de quinas; na parede, o carro passa por elas */
#define GRAVIDADE_NA_PAREDE  0.35f     /* na parede, a gravidade pesa só isso (arcade) */
#define ADESAO               2.5f      /* m/s²: além da gravidade, o pneu "segura" isso numa lombada */
#define ALTURA_DA_ESFERA     1.15f     /* metros: o centro dos pedaços do carro que batem nas paredes */
#define ALTURA_DO_OLHAR      0.7f      /* metros: de onde o carro olha à frente procurando parede */
#define REBATE               0.25f     /* da velocidade contra a parede, quanto volta */
#define RASPANDO             0.6f      /* por segundo: o quanto a parede segura raspando de lado */
#define PANCADA_CHEIA        25.0f     /* m/s contra a parede: a batida mais forte */
#define CAIU_DO_MUNDO        200.0f    /* metros abaixo do mundo: volta ao começo */

/* O wallride */
#define VELOCIDADE_DA_PAREDE 13.0f     /* m/s (47 km/h): para subir na parede, pelo menos isso */
#define VELOCIDADE_PARA_FICAR 10.0f    /* m/s (36 km/h): já na parede, ela segura o carro até aqui */
#define RASPAO_DA_PAREDE     0.87f     /* seno do ângulo de entrada: mais de frente que ~60 graus é batida */
#define PAREDE_ACIMA_DE      45.0f     /* graus de diferença: daqui para cima a superfície nova é "parede" (precisa de velocidade) */
#define VOLANTE_PARA_A_PAREDE 0.1f     /* o volante virado para a parede, pelo menos isso */
#define GIRO_NA_PAREDE       1.7f      /* rad/s com o volante todo: na parede o carro vira solto (arcade) */
#define VOLANTE_A_MAIS_NA_PAREDE 9.0f /* por segundo: na parede, o volante gira mais isso além do da corrida */
#define DESLIZE_NA_PAREDE    7.0f      /* m/s: na parede, o volante também empurra o carro direto para o lado */
#define PEGA_NA_PAREDE       6.0f      /* por segundo: a rapidez com que o deslize obedece ao volante */
#define VOLTA_AO_CHAO        0.2f      /* indo para o chão pelo menos isso (seno): da parede, o carro volta para ele */
#define PERDA_AO_SUBIR       0.93f     /* ao passar para a parede, sobra isso da velocidade */
#define EMPINADO             0.55f     /* y do "para cima": abaixo disso o carro está numa parede */
#define DESCOLA              2.5f      /* m/s: soltando da parede, o carro sai com isso para fora */
#define LADEIRA_SUBINDO      0.6f      /* subindo um barranco (não uma parede), a gravidade pesa só isso */
#define ATERRISSA            0.45f     /* y da normal: daqui para cima, no ar, é chão para aterrissar */
#define ENDIREITA_NO_AR      2.5f      /* por segundo: no ar, o carro volta a ficar de pé */

/* O atrito de cada chão (m/s² a mais, rolando). A curva é a mesma em qualquer chão, como na corrida
 * (lá a grama só tira velocidade): a areia só segura um pouco o carro */
static const float ATRITO_DO_CHAO[TOTAL_DE_CHAOS] = { 0.0f, 2.6f, 1.0f, 0.3f, 0.0f };

/* ------------------------------------------------------------------ */

static float limitar(float v, float a, float b) {
    return v < a ? a : (v > b ? b : v);
}

static float aproximar(float atual, float alvo, float passo) {
    if (atual < alvo) {
        return fminf(atual + passo, alvo);
    }
    return fmaxf(atual - passo, alvo);
}

static float escalar(const float *a, const float *b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

static void vetorial(const float *a, const float *b, float *r) {
    float x = a[1] * b[2] - a[2] * b[1], y = a[2] * b[0] - a[0] * b[2], z = a[0] * b[1] - a[1] * b[0];
    r[0] = x; r[1] = y; r[2] = z;
}

static float normalizar(float *v) {
    float t = sqrtf(escalar(v, v));
    if (t > 1e-9f) {
        v[0] /= t; v[1] /= t; v[2] /= t;
    }
    return t;
}

/* Tira de v a parte na direção n (n de tamanho 1) */
static void tirarParte(float *v, const float *n) {
    float d = escalar(v, n);
    v[0] -= n[0] * d; v[1] -= n[1] * d; v[2] -= n[2] * d;
}

/* A frente perpendicular ao "para cima"; se ela sumir (o bico apontava para o chão), usa a reserva */
static void endireitarFrente(CarroLivre *carro, const float *reserva) {
    float f[3] = { carro->frente[0], carro->frente[1], carro->frente[2] };
    tirarParte(f, carro->cima);
    if (normalizar(f) < 1e-3f) {
        f[0] = reserva[0]; f[1] = reserva[1]; f[2] = reserva[2];
        tirarParte(f, carro->cima);
        if (normalizar(f) < 1e-3f) {
            float x[3] = { 1.0f, 0.0f, 0.0f };
            tirarParte(x, carro->cima);
            normalizar(x);
            f[0] = x[0]; f[1] = x[1]; f[2] = x[2];
        }
    }
    carro->frente[0] = f[0]; carro->frente[1] = f[1]; carro->frente[2] = f[2];
}

float velocidadeParaFrente(const CarroLivre *carro) {
    float v[3] = { carro->vx, carro->vy, carro->vz };
    return escalar(v, carro->frente);
}

float velocidadeDoCarroLivre(const CarroLivre *carro) {
    return sqrtf(carro->vx * carro->vx + carro->vy * carro->vy + carro->vz * carro->vz);
}

/*
 * iniciarCarroLivre
 * Recebe: o carro, a ficha técnica, onde ele começa (metros) e para onde aponta (radianos).
 * Devolve: nada.
 */
void iniciarCarroLivre(CarroLivre *carro, const FichaTecnica *ficha, float x, float y, float z, float rumo) {
    CarroLivre novo = { 0 };
    novo.x = novo.xInicial = x;
    novo.y = novo.yInicial = y;
    novo.z = novo.zInicial = z;
    novo.rumo = novo.rumoInicial = rumo;
    novo.cima[1] = 1.0f;
    novo.frente[0] = sinf(rumo);
    novo.frente[2] = -cosf(rumo);
    novo.noChao = 1;
    novo.chao = CHAO_ASFALTO;
    /* O carro da corrida, sem pista: é ele que acelera, freia, vira e derrapa (dirigirSemPista) */
    novo.corrida = criarCarro(ficha->nome, 0, NULL, 0.0f, ficha->velocidadeMaxima, ficha->aceleracao, ficha->aderencia);
    novo.corrida.direcaoLivre = 1;
    novo.corrida.temRumoNoMundo = 1;
    novo.corrida.controladoPeloJogador = 1;   /* dá ré */
    novo.corrida.rumoNoMundo = rumo * 57.29578f;
    novo.corrida.comprimento = ficha->comprimento;
    novo.corrida.largura = ficha->largura;
    novo.nitro = novo.corrida.nitro;
    novo.motor = novo.corrida.motor;
    novo.velocidadeMaxima = ficha->velocidadeMaxima / UNIDADES_POR_METRO;   /* em metros, como a corrida anda */
    novo.aceleracao = ficha->aceleracao / UNIDADES_POR_METRO;
    novo.aderencia = ficha->aderencia;
    novo.comprimento = ficha->comprimento;
    novo.largura = ficha->largura;
    *carro = novo;
}

/*
 * levarCarroPara
 * Põe o carro parado, de pé, em cima do chão de um ponto (para a largada e para o mapa).
 * Recebe: o carro, a malha, o ponto (x, z) e o rumo.
 * Devolve: nada.
 */
void levarCarroPara(CarroLivre *carro, MalhaDeColisao *malha, float x, float z, float rumo) {
    carro->x = x;
    carro->z = z;
    carro->rumo = rumo;
    carro->cima[0] = 0.0f; carro->cima[1] = 1.0f; carro->cima[2] = 0.0f;
    carro->frente[0] = sinf(rumo); carro->frente[1] = 0.0f; carro->frente[2] = -cosf(rumo);
    float chao = malha ? alturaDoChaoNaMalha(malha, x, z, 5000.0f, NULL) : SEM_CHAO;
    carro->y = (chao > SEM_CHAO) ? chao : carro->yInicial;
    carro->vx = carro->vy = carro->vz = 0.0f;
    carro->volante = 0.0f;
    carro->noChao = 1;
    carro->naParede = 0;
    carro->segundosNaParede = 0.0f;
    carro->segundosNoAr = 0.0f;
    carro->escorregando = 0.0f;
    carro->doDriftAntes = 0.0f;
    Carro *corrida = &carro->corrida;
    corrida->velocidade = corrida->re = corrida->paradoFreando = 0.0f;
    corrida->volante = 0.0f;
    corrida->forcaDoDrift = corrida->anguloDeDrift = corrida->giroDoDrift = corrida->derrapagem = 0.0f;
    corrida->nitroLigado = 0;
    corrida->temSuspensao = 0;
    corrida->rumoNoMundo = rumo * 57.29578f;
    iniciarMotor(&corrida->motor);
    carro->motor = corrida->motor;
}

void voltarAoComeco(CarroLivre *carro, MalhaDeColisao *malha) {
    levarCarroPara(carro, malha, carro->xInicial, carro->zInicial, carro->rumoInicial);
}

/* O rumo (visto de cima) segue a frente, se ela não estiver apontando para o céu ou o chão */
static void atualizarRumo(CarroLivre *carro) {
    if (hypotf(carro->frente[0], carro->frente[2]) > 0.2f) {
        carro->rumo = atan2f(carro->frente[0], -carro->frente[2]);
    }
}

/*
 * dirigir
 * Acelerador, freio, volante e pneu, no plano do chão embaixo das rodas.
 */
static void dirigir(CarroLivre *carro, const Comandos *comandos, float passo) {
    float *n = carro->cima, *f = carro->frente;
    float d[3];
    vetorial(f, n, d);   /* a direita */
    float v[3] = { carro->vx, carro->vy, carro->vz };
    float frente = escalar(v, f), lado = escalar(v, d);
    int emPe = n[1] <= EMPINADO;   /* numa parede */
    Carro *corrida = &carro->corrida;

    /* ---- A direção da corrida: nitro, motor, velocidade, volante, drift, ré e o giro do rumo ----
     * A velocidade de agora (as batidas e a ladeira mexem nela) entra na conta da corrida */
    if (corrida->re > 0.0f) {
        corrida->re = fmaxf(-frente, 0.0f) * UNIDADES_POR_METRO;
    } else {
        corrida->velocidade = fmaxf(frente, 0.0f) * UNIDADES_POR_METRO;
    }
    corrida->rumoNoMundo = carro->rumo * 57.29578f;
    float doDrift;
    int deRe;
    float giroDaCorrida = dirigirSemPista(corrida, *comandos, passo, 1.0f, &doDrift, &deRe);   /* a curva da corrida em qualquer chão */
    frente = deRe ? -corrida->re / UNIDADES_POR_METRO : corrida->velocidade / UNIDADES_POR_METRO;

    /* ---- O que a corrida não tem: a ladeira, a areia e a parede ---- */
    float g[3] = { 0.0f, -GRAVIDADE_LIVRE, 0.0f };
    tirarParte(g, n);
    float ladeira = escalar(g, f);
    if (!emPe && ladeira < 0.0f) {
        ladeira *= LADEIRA_SUBINDO;   /* arcade: o carro sobe barranco de areia sem empacar */
    } else if (emPe) {
        ladeira *= GRAVIDADE_NA_PAREDE;   /* arcade: rápido, o pneu "gruda" e a parede puxa menos */
        lado += escalar(g, d) * GRAVIDADE_NA_PAREDE * passo;
    }
    frente += ladeira * passo;
    float areia = ATRITO_DO_CHAO[carro->chao] * (comandos->acelerar ? 0.5f : 1.0f) * passo;   /* a areia segura um pouco */
    frente = (frente > 0.0f) ? fmaxf(frente - areia, 0.0f) : fminf(frente + areia, 0.0f);
    if (!deRe && frente < 0.0f) {
        frente = 0.0f;   /* a ladeira para o carro; a ré é só no freio, como na corrida */
    }

    float giro;
    if (emPe) {
        /* Na parede (arcade): o volante responde mais rápido, vira o carro solto e o empurra para o lado */
        float setas = (float) (comandos->direita - comandos->esquerda);
        float alvo = (fabsf(comandos->direcao) > fabsf(setas)) ? limitar(comandos->direcao, -1.0f, 1.0f) : setas;
        corrida->volante = aproximar(corrida->volante, alvo, VOLANTE_A_MAIS_NA_PAREDE * passo);
        giro = corrida->volante * GIRO_NA_PAREDE * passo;
        float alvoDoLado = corrida->volante * DESLIZE_NA_PAREDE;
        lado += (alvoDoLado - lado) * (1.0f - expf(-PEGA_NA_PAREDE * passo));
        carro->escorregando = limitar(fabsf(lado - alvoDoLado) / 8.0f, 0.0f, 1.0f);
        carro->doDriftAntes = 0.0f;
    } else {
        giro = giroDaCorrida * 0.0174533f;
        /* De lado, como na corrida: o tranco das batidas some aos poucos, e o drift soma o seu pedaço */
        float tranco = (lado - carro->doDriftAntes) * expf(-QUEDA_DO_TRANCO * passo);
        lado = tranco + doDrift;
        carro->doDriftAntes = doDrift;
        carro->escorregando = fmaxf(corrida->derrapagem, corrida->forcaDoDrift);
    }
    carro->volante = corrida->volante;

    float c = cosf(giro), s = sinf(giro);
    for (int e = 0; e < 3; e++) {
        float novo = f[e] * c + d[e] * s;
        d[e] = d[e] * c - f[e] * s;
        f[e] = novo;
    }
    carro->vx = f[0] * frente + d[0] * lado;
    carro->vy = f[1] * frente + d[1] * lado;
    carro->vz = f[2] * frente + d[2] * lado;
    atualizarRumo(carro);   /* já neste passo: a suspensão da corrida mede o giro entre um passo e outro */
}

/*
 * subirNaParede
 * O carro passa para uma superfície nova (uma parede, ou o fim de um salto numa rampa):
 * o "para cima" vira a normal dela e a velocidade passa a correr no plano dela.
 */
static void subirNaSuperficie(CarroLivre *carro, const Acerto *acerto, float perda) {
    float v[3] = { carro->vx, carro->vy, carro->vz };
    float rapidez = sqrtf(escalar(v, v));
    for (int e = 0; e < 3; e++) {
        carro->cima[e] = acerto->normal[e];
    }
    tirarParte(v, carro->cima);
    if (normalizar(v) > 1e-3f) {
        carro->vx = v[0] * rapidez * perda;
        carro->vy = v[1] * rapidez * perda;
        carro->vz = v[2] * rapidez * perda;
    }
    endireitarFrente(carro, v);
    carro->x = acerto->ponto[0] + acerto->normal[0] * 0.02f;
    carro->y = acerto->ponto[1] + acerto->normal[1] * 0.02f;
    carro->z = acerto->ponto[2] + acerto->normal[2] * 0.02f;
    carro->superficie = acerto->superficie;
    carro->noChao = 1;
    carro->segundosNoAr = 0.0f;   /* quem chama já usou o tempo no ar para a pancada */
}

/* Dá para andar nesta superfície vindo assim? Rampa: sempre. Parede: rápido, de raspão e com o
 * volante virado para o lado dela (só raspar num prédio não leva o carro para cima dele).
 * No ar (volante: 0) basta vir rápido e de raspão. */
static int podeSubir(const CarroLivre *carro, const float *normal, const float *direcao) {
    float v[3] = { carro->vx, carro->vy, carro->vz };
    float rapidez = sqrtf(escalar(v, v));
    float diferenca = acosf(limitar(escalar(normal, carro->cima), -1.0f, 1.0f)) * 57.29578f;
    if (diferenca < PAREDE_ACIMA_DE) {
        return 1;   /* uma rampa: qualquer carro sobe */
    }
    if (normal[1] > ATERRISSA && normal[1] > carro->cima[1] + 0.05f && escalar(direcao, normal) < -VOLTA_AO_CHAO) {
        return 1;   /* de uma parede (ou de um barranco em pé) de volta para o chão: indo para ele, sempre (arcade) */
    }
    float deFrente = -escalar(direcao, normal);   /* 1 = bem de frente */
    if (rapidez <= VELOCIDADE_DA_PAREDE || deFrente >= RASPAO_DA_PAREDE || normal[1] < -0.5f) {
        return 0;
    }
    if (!carro->noChao) {
        return 1;
    }
    float direita[3];
    vetorial(carro->frente, carro->cima, direita);
    float ladoDaParede = -escalar(normal, direita);   /* > 0: a parede está à direita */
    return carro->volante * ladoDaParede > VOLANTE_PARA_A_PAREDE;
}

/*
 * baterNasParedes
 * Dois pedaços do carro (a frente e a traseira) contra as paredes: o carro sai de dentro
 * delas e perde a velocidade que ia contra elas.
 */
static void baterNasParedes(CarroLivre *carro, MalhaDeColisao *malha, float passo) {
    float raio = carro->largura * 0.5f;
    float meio = fmaxf(carro->comprimento * 0.5f - raio, 0.0f);
    /* A esfera fica acima do degrau que o carro acompanha (GRUDA_NO_CHAO): senão, nos carros largos,
     * ela encostava no chão e a quina de qualquer degrau virava parede */
    float altura = fmaxf(ALTURA_DA_ESFERA, raio + GRUDA_NO_CHAO + 0.05f);
    carro->raspando = 0;
    for (int ponta = -1; ponta <= 1; ponta += 2) {
        float centro[3];
        centro[0] = carro->x + carro->cima[0] * altura + carro->frente[0] * meio * ponta;
        centro[1] = carro->y + carro->cima[1] * altura + carro->frente[1] * meio * ponta;
        centro[2] = carro->z + carro->cima[2] * altura + carro->frente[2] * meio * ponta;
        float empurrao[3];
        if (!empurrarDasParedes(malha, centro, raio, carro->cima, empurrao)) {
            continue;
        }
        float tamanho = sqrtf(escalar(empurrao, empurrao));
        if (tamanho < 1e-5f) {
            continue;
        }
        if (tamanho > raio) {   /* várias faces da mesma parede somam: não empurra mais que o carro entrou */
            for (int e = 0; e < 3; e++) {
                empurrao[e] *= raio / tamanho;
            }
            tamanho = raio;
        }
        float n[3] = { empurrao[0] / tamanho, empurrao[1] / tamanho, empurrao[2] / tamanho };
        float v[3] = { carro->vx, carro->vy, carro->vz };
        /* De raspão, rápido e virando para ela: em vez de bater, o carro sobe na parede */
        float direcao[3] = { v[0], v[1], v[2] };
        if (normalizar(direcao) > 1e-3f && podeSubir(carro, n, direcao)) {
            float paraDentro[3] = { -n[0], -n[1], -n[2] };
            Acerto acerto;
            if (raioNaMalha(malha, centro, paraDentro, raio + 0.5f, &acerto) &&
                acosf(limitar(escalar(acerto.normal, carro->cima), -1.0f, 1.0f)) * 57.29578f >= PAREDE_ACIMA_DE &&
                podeSubir(carro, acerto.normal, direcao)) {   /* a face que o raio acertou também tem de servir */
                /* O meio do carro vai para a parede, não a ponta da esfera: desfaz o deslocamento da esfera
                 * ao longo do carro, ficando no plano da face */
                float d[3];
                for (int e = 0; e < 3; e++) {
                    d[e] = -carro->frente[e] * meio * ponta;
                }
                tirarParte(d, acerto.normal);
                for (int e = 0; e < 3; e++) {
                    acerto.ponto[e] += d[e];
                }
                subirNaSuperficie(carro, &acerto, PERDA_AO_SUBIR);
                carro->chao = tipoDoChao(malha, acerto.superficie);
                return;
            }
        }
        carro->x += empurrao[0];
        carro->y += empurrao[1];
        carro->z += empurrao[2];
        float contra = escalar(v, n);
        if (contra < 0.0f) {
            carro->pancada = fmaxf(carro->pancada, limitar(-contra / PANCADA_CHEIA, 0.0f, 1.0f));
            carro->vx -= n[0] * contra * (1.0f + REBATE);
            carro->vy -= n[1] * contra * (1.0f + REBATE);
            carro->vz -= n[2] * contra * (1.0f + REBATE);
        }
        float segura = expf(-RASPANDO * passo);
        carro->vx *= segura;
        carro->vy *= segura;
        carro->vz *= segura;
        carro->raspando = 1;
    }
}

/*
 * olharAFrente
 * Antes de andar: se o caminho do passo bate numa superfície, ou o carro passa para ela
 * (rampa, ou parede de raspão e rápido), ou a batida fica para as esferas.
 * Devolve: 1 se o carro passou para a superfície nova (o passo já foi dado).
 */
static int olharAFrente(CarroLivre *carro, MalhaDeColisao *malha, float passo) {
    float v[3] = { carro->vx, carro->vy, carro->vz };
    float andar = sqrtf(escalar(v, v)) * passo;
    if (andar < 1e-4f) {
        return 0;
    }
    float direcao[3] = { v[0], v[1], v[2] };
    normalizar(direcao);
    float origem[3] = { carro->x + carro->cima[0] * ALTURA_DO_OLHAR, carro->y + carro->cima[1] * ALTURA_DO_OLHAR,
                        carro->z + carro->cima[2] * ALTURA_DO_OLHAR };
    Acerto acerto;
    if (!raioNaMalha(malha, origem, direcao, andar + carro->comprimento * 0.5f, &acerto)) {
        return 0;
    }
    float diferenca = acosf(limitar(escalar(acerto.normal, carro->cima), -1.0f, 1.0f)) * 57.29578f;
    if (diferenca < PAREDE_ACIMA_DE || !podeSubir(carro, acerto.normal, direcao)) {
        return 0;   /* rampa: o raio de baixo cuida; parede de frente: as esferas batem */
    }
    if (acerto.distancia > andar + carro->comprimento * 0.25f) {
        return 0;   /* ainda longe: chega no próximo passo */
    }
    subirNaSuperficie(carro, &acerto, PERDA_AO_SUBIR);
    return 1;
}

/*
 * acharOChao
 * Procura a superfície embaixo das rodas, ao longo do "para baixo" do carro. Gruda se ela
 * estiver perto e a dobra for suave; se ela fugir mais rápido do que o pneu segura (o fim de
 * uma rampa, uma lombada a toda), o carro sai voando.
 */
static void acharOChao(CarroLivre *carro, MalhaDeColisao *malha, float passo) {
    float *n = carro->cima;
    float origem[3] = { carro->x + n[0] * DEGRAU, carro->y + n[1] * DEGRAU, carro->z + n[2] * DEGRAU };
    float baixo[3] = { -n[0], -n[1], -n[2] };
    Acerto acerto;
    if (!raioNaMalha(malha, origem, baixo, DEGRAU + GRUDA_NO_CHAO, &acerto)) {
        carro->noChao = 0;
        carro->segundosNoAr = 0.0f;
        return;
    }
    float dobra = acosf(limitar(escalar(acerto.normal, n), -1.0f, 1.0f));
    if (dobra * 57.29578f > (n[1] <= EMPINADO ? MAIOR_DOBRA_NA_PAREDE : MAIOR_DOBRA)) {
        carro->noChao = 0;   /* uma beirada: o carro passa por cima dela, no ar */
        carro->segundosNoAr = 0.0f;
        return;
    }
    /* Lombada (o chão novo se inclina para a frente, para baixo): segura só até um certo ponto */
    float v[3] = { carro->vx, carro->vy, carro->vz };
    float rapidez = sqrtf(escalar(v, v));
    float desce = escalar(acerto.normal, carro->frente) - escalar(n, carro->frente);
    if (n[1] > EMPINADO && desce > 0.0f && rapidez * passo > 1e-4f && acerto.distancia > DEGRAU + 0.05f) {   /* na parede, quem segura é a velocidade */
        float curvatura = dobra / (rapidez * passo);
        float segura = GRAVIDADE_LIVRE * fmaxf(n[1], 0.0f) + ADESAO;
        if (rapidez * rapidez * curvatura > segura) {
            carro->noChao = 0;
            carro->segundosNoAr = 0.0f;
            return;
        }
    }
    carro->x = acerto.ponto[0];
    carro->y = acerto.ponto[1];
    carro->z = acerto.ponto[2];
    float segue = fminf(VIRA_COM_O_CHAO * passo, 1.0f);
    for (int e = 0; e < 3; e++) {
        n[e] += (acerto.normal[e] - n[e]) * segue;
    }
    normalizar(n);
    endireitarFrente(carro, v);
    tirarParte(v, n);
    carro->vx = v[0]; carro->vy = v[1]; carro->vz = v[2];
    carro->superficie = acerto.superficie;
    carro->chao = tipoDoChao(malha, acerto.superficie);
}

/*
 * descolarSePrecisar
 * Numa parede (ou de cabeça para baixo), só a velocidade segura o carro: devagar, ele solta.
 */
static void descolarSePrecisar(CarroLivre *carro) {
    carro->naParede = carro->cima[1] < EMPINADO;
    if (!carro->naParede) {
        return;
    }
    float precisa = VELOCIDADE_PARA_FICAR * (1.0f + fmaxf(-carro->cima[1], 0.0f));
    if (velocidadeDoCarroLivre(carro) < precisa) {
        carro->noChao = 0;
        carro->naParede = 0;
        carro->segundosNoAr = 0.0f;
        carro->vx += carro->cima[0] * DESCOLA;
        carro->vy += carro->cima[1] * DESCOLA;
        carro->vz += carro->cima[2] * DESCOLA;
    }
}

/*
 * voar
 * No ar: a gravidade puxa, o carro se endireita aos poucos e o volante gira um pouco. Ao
 * bater em alguma coisa: chão (aterrissa), parede de raspão e rápido (gruda) ou quica.
 */
static void voar(CarroLivre *carro, const Comandos *comandos, MalhaDeColisao *malha, float passo) {
    carro->vy -= GRAVIDADE_LIVRE * passo;
    carro->segundosNoAr += passo;
    float ceu[3] = { 0.0f, 1.0f, 0.0f };
    float endireita = fminf(ENDIREITA_NO_AR * passo, 1.0f);
    for (int e = 0; e < 3; e++) {
        carro->cima[e] += (ceu[e] - carro->cima[e]) * endireita;
    }
    normalizar(carro->cima);
    girarVolante(&carro->corrida, *comandos, passo);   /* o volante da corrida */
    carro->volante = carro->corrida.volante;
    float d[3];
    vetorial(carro->frente, carro->cima, d);
    float angulo = carro->volante * GIRO_NO_AR * passo, c = cosf(angulo), s = sinf(angulo);
    for (int e = 0; e < 3; e++) {
        carro->frente[e] = carro->frente[e] * c + d[e] * s;
    }
    float v[3] = { carro->vx, carro->vy, carro->vz };
    endireitarFrente(carro, v);

    float andar = sqrtf(escalar(v, v)) * passo;
    float direcao[3] = { v[0], v[1], v[2] };
    normalizar(direcao);
    float origem[3] = { carro->x + carro->cima[0] * ALTURA_DO_OLHAR, carro->y + carro->cima[1] * ALTURA_DO_OLHAR,
                        carro->z + carro->cima[2] * ALTURA_DO_OLHAR };
    Acerto acerto;
    if (andar > 1e-5f && raioNaMalha(malha, origem, direcao, andar + ALTURA_DO_OLHAR, &acerto)) {
        float contra = -escalar(v, acerto.normal);
        if (acerto.normal[1] > ATERRISSA || podeSubir(carro, acerto.normal, direcao)) {
            if (carro->segundosNoAr > 0.25f) {
                carro->pancada = fmaxf(carro->pancada, limitar(contra / PANCADA_CHEIA, 0.0f, 1.0f));
            }
            subirNaSuperficie(carro, &acerto, 1.0f);
            carro->chao = tipoDoChao(malha, acerto.superficie);
            return;
        }
        /* Quica */
        carro->pancada = fmaxf(carro->pancada, limitar(contra / PANCADA_CHEIA, 0.0f, 1.0f));
        carro->x = acerto.ponto[0] + acerto.normal[0] * 0.3f - carro->cima[0] * ALTURA_DO_OLHAR;
        carro->y = acerto.ponto[1] + acerto.normal[1] * 0.3f - carro->cima[1] * ALTURA_DO_OLHAR;
        carro->z = acerto.ponto[2] + acerto.normal[2] * 0.3f - carro->cima[2] * ALTURA_DO_OLHAR;
        carro->vx += acerto.normal[0] * contra * (1.0f + REBATE);
        carro->vy += acerto.normal[1] * contra * (1.0f + REBATE);
        carro->vz += acerto.normal[2] * contra * (1.0f + REBATE);
        return;
    }
    carro->x += v[0] * passo;
    carro->y += v[1] * passo;
    carro->z += v[2] * passo;

    /* Caindo devagar bem em cima do chão: o raio da velocidade é curto demais, olha para baixo */
    float queda = fmaxf(-v[1] * passo, 0.0f) + 0.05f;
    float alto[3] = { carro->x, carro->y + 0.5f, carro->z };
    float baixo[3] = { 0.0f, -1.0f, 0.0f };
    if (raioNaMalha(malha, alto, baixo, 0.5f + queda, &acerto) && acerto.normal[1] > ATERRISSA && v[1] <= 0.0f) {
        if (carro->segundosNoAr > 0.25f) {
            carro->pancada = fmaxf(carro->pancada, limitar(-v[1] / PANCADA_CHEIA, 0.0f, 1.0f));
        }
        subirNaSuperficie(carro, &acerto, 1.0f);
        carro->chao = tipoDoChao(malha, acerto.superficie);
    }
}

/*
 * atualizarCarroLivre
 * Um quadro do carro, em passos curtos (a física fica igual em qualquer FPS).
 * Recebe: o carro, os comandos do piloto, a malha de colisão e o tempo do quadro.
 * Devolve: nada.
 */
void atualizarCarroLivre(CarroLivre *carro, Comandos comandos, MalhaDeColisao *malha, float tempoQuadro) {
    if (tempoQuadro > 0.1f) {
        tempoQuadro = 0.1f;   /* um tranco (a janela travou um instante) não teleporta o carro */
    }
    carro->acelerando = comandos.acelerar;
    carro->freando = comandos.frear;
    carro->freioDeMao = comandos.freioDeMao;

    /* Passos curtos o bastante para a esfera não atravessar uma parede fina de uma vez */
    float rapidez = velocidadeDoCarroLivre(carro);
    int passos = (int) ceilf(fmaxf(tempoQuadro / PASSO_MAXIMO, rapidez * tempoQuadro / (0.5f * carro->largura * 0.5f)));
    if (passos < 1) {
        passos = 1;
    }
    if (passos > MAIS_PASSOS) {
        passos = MAIS_PASSOS;
    }
    float passo = tempoQuadro / passos;
    for (int i = 0; i < passos; i++) {
        if (malha == NULL) {
            if (carro->noChao) {
                dirigir(carro, &comandos, passo);
            }
            carro->x += carro->vx * passo;
            carro->y += carro->vy * passo;
            carro->z += carro->vz * passo;
            continue;
        }
        if (!carro->noChao) {
            voar(carro, &comandos, malha, passo);
            continue;
        }
        dirigir(carro, &comandos, passo);
        descolarSePrecisar(carro);
        if (!carro->noChao) {
            continue;
        }
        if (olharAFrente(carro, malha, passo)) {
            continue;
        }
        carro->x += carro->vx * passo;
        carro->y += carro->vy * passo;
        carro->z += carro->vz * passo;
        baterNasParedes(carro, malha, passo);
        acharOChao(carro, malha, passo);
    }
    carro->naParede = carro->noChao && carro->cima[1] < EMPINADO;
    carro->segundosNaParede = carro->naParede ? carro->segundosNaParede + tempoQuadro : 0.0f;
    atualizarRumo(carro);

    /* No chão, o motor, o nitro e as rodas andaram na conta da corrida; no ar, o motor gira solto */
    Carro *corrida = &carro->corrida;
    if (!carro->noChao) {
        corrida->nitroLigado = 0;
        atualizarMotor(&corrida->motor, velocidadeParaFrente(carro) * UNIDADES_POR_METRO / corrida->velocidadeMaxima,
                       comandos.acelerar, 0, 1, tempoQuadro);
    }
    carro->motor = corrida->motor;
    carro->nitro = corrida->nitro;
    carro->nitroLigado = corrida->nitroLigado;
    carro->anguloDasRodas = corrida->anguloDasRodas;
    carro->volante = corrida->volante;

    if (malha != NULL) {
        float minimo[3], maximo[3];
        limitesDaMalha(malha, minimo, maximo);
        if (carro->y < minimo[1] - CAIU_DO_MUNDO || carro->x < minimo[0] - 50.0f || carro->x > maximo[0] + 50.0f ||
            carro->z < minimo[2] - 50.0f || carro->z > maximo[2] + 50.0f) {
            voltarAoComeco(carro, malha);
            carro->quedas++;   /* caiu do mundo: quem desenha sabe que o carro foi levado ao começo */
        }
    }
}
