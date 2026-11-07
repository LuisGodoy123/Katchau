/*
 * motorgranular.c
 * ------------------------------------------------------------------
 * O motor do jogador, montado em pedacinhos da gravação. Veja motorgranular.h.
 * ------------------------------------------------------------------
 */

#include <math.h>
#include <string.h>
#include "motorgranular.h"
#include "motor.h"

#define TOM_NA_LINHA_VERMELHA  322.0f   /* Hz da gravação quando o motor dela bate no limite, antes de trocar de marcha */
#define MENOR_TAXA             0.55f
#define MAIOR_TAXA             1.80f
#define FOLGA_DO_CASAMENTO     480      /* amostras para cada lado em que se procura onde as ondas casam */
#define DECIMACAO              4        /* a procura olha uma amostra a cada tantas (e depois refina) */
#define NIVEL_ALVO             0.22f    /* o volume (RMS) que cada pedacinho recebe, para o som não oscilar */
#define SOBE_O_VOLUME          0.20f    /* o quanto o volume pode subir ou cair por passo (anda até o pedido) */

/* Um ponto da gravação e o tom dele, medidos nela. Cada marcha tem uma lista em ordem de tom. */
typedef struct {
    float segundos;
    float tom;
} PontoDaGravacao;

/* 1ª marcha: o motor parado, a arrancada (0,8 s a 1,3 s) e a primeira subida (3,2 s a 4,45 s da gravação) */
static const PontoDaGravacao PRIMEIRA[] = {
    { 0.40f, 40.0f }, { 0.80f, 89.0f }, { 0.90f, 106.0f }, { 1.00f, 143.0f }, { 1.10f, 164.0f }, { 1.20f, 192.0f },
    { 1.30f, 210.0f }, { 3.10f, 210.5f },   /* entre os dois a gravação pula (o tom cai e sobe de novo): é uma emenda, não uma subida */
    { 3.20f, 228.0f }, { 3.30f, 233.0f }, { 3.40f, 240.0f }, { 3.50f, 247.0f }, { 3.60f, 254.0f }, { 3.70f, 262.0f },
    { 3.80f, 271.0f }, { 3.90f, 281.0f }, { 4.00f, 287.0f }, { 4.10f, 295.0f }, { 4.20f, 304.0f }, { 4.30f, 313.0f },
    { 4.45f, 322.0f },
};
/* 2ª marcha: a segunda subida (6 s a 9,1 s) */
static const PontoDaGravacao SEGUNDA[] = {
    { 6.00f, 244.0f }, { 6.25f, 249.0f }, { 6.50f, 257.0f }, { 6.75f, 265.0f }, { 7.00f, 273.0f }, { 7.25f, 280.0f },
    { 7.50f, 288.0f }, { 7.75f, 295.0f }, { 8.00f, 301.0f }, { 8.25f, 307.0f }, { 8.50f, 313.0f }, { 8.75f, 318.0f },
    { 9.10f, 323.0f },
};
/* 3ª marcha em diante: a terceira subida, a mais longa (10,9 s a 16,5 s) */
static const PontoDaGravacao TERCEIRA[] = {
    { 10.90f, 255.0f }, { 11.30f, 260.0f }, { 11.70f, 264.0f }, { 12.10f, 269.0f }, { 12.50f, 274.0f }, { 12.90f, 277.0f },
    { 13.30f, 282.0f }, { 13.70f, 286.0f }, { 14.10f, 290.0f }, { 14.50f, 294.0f }, { 14.90f, 298.0f }, { 15.30f, 301.0f },
    { 15.70f, 306.0f }, { 16.10f, 309.0f }, { 16.50f, 314.0f },
};

static float limitar(float valor, float menor, float maior) {
    return valor < menor ? menor : (valor > maior ? maior : valor);
}

/*
 * tomDoGiro
 * Recebe: a rotação do jogo (0 a 1, em que 1 = RPM_MAXIMO).
 * Devolve: o tom (Hz) que o motor da gravação tinha nessa rotação: a linha vermelha do jogo (onde o
 *          câmbio troca) cai no tom em que o motor da gravação trocava de marcha.
 */
float tomDoGiro(float giro) {
    return TOM_NA_LINHA_VERMELHA * giro / (RPM_DA_TROCA / RPM_MAXIMO);
}

/*
 * pontoDaGravacao
 * Acha o ponto da gravação da marcha que tem o tom pedido (na lista da marcha, entre dois pontos).
 * Recebe: a marcha do jogo, o tom pedido e onde guardar o ponto (segundos) e o tom que ele tem.
 * Devolve: 1 se o tom está dentro do que a marcha da gravação cobre; 0 se ficou fora (o ponto devolvido
 *          é então o da ponta mais perto, e a diferença de tom é feita tocando mais rápido ou mais lento).
 */
int pontoDaGravacao(int marcha, float tom, float *segundos, float *tomDaGravacao) {
    const PontoDaGravacao *lista;
    int total;
    if (marcha <= 0) {
        lista = PRIMEIRA;
        total = (int) (sizeof(PRIMEIRA) / sizeof(PRIMEIRA[0]));
    } else if (marcha == 1) {
        lista = SEGUNDA;
        total = (int) (sizeof(SEGUNDA) / sizeof(SEGUNDA[0]));
    } else {
        lista = TERCEIRA;
        total = (int) (sizeof(TERCEIRA) / sizeof(TERCEIRA[0]));
    }
    if (tom <= lista[0].tom) {
        *segundos = lista[0].segundos;
        *tomDaGravacao = lista[0].tom;
        return 0;
    }
    if (tom >= lista[total - 1].tom) {
        *segundos = lista[total - 1].segundos;
        *tomDaGravacao = lista[total - 1].tom;
        return 0;
    }
    for (int i = 1; i < total; i++) {
        if (tom <= lista[i].tom) {
            float t = (tom - lista[i - 1].tom) / (lista[i].tom - lista[i - 1].tom);
            *segundos = lista[i - 1].segundos + t * (lista[i].segundos - lista[i - 1].segundos);
            *tomDaGravacao = tom;
            return 1;
        }
    }
    return 0;
}

/*
 * iniciarMotorGranular
 * Recebe: o motor, a gravação (mono, 44100 Hz, que precisa continuar existindo) e quantas amostras tem.
 * Devolve: 1 se a gravação serve (é longa o bastante para as tabelas), 0 se não.
 */
int iniciarMotorGranular(MotorGranular *motor, const float *fonte, int total) {
    memset(motor, 0, sizeof(*motor));
    if (fonte == NULL || total < (int) (17.0f * TAXA_DO_MOTOR_GRANULAR)) {
        return 0;
    }
    motor->fonte = fonte;
    motor->total = total;
    motor->giro = 0.12f;
    motor->volume = 0.0f;
    motor->volumeAtual = 0.0f;
    return 1;
}

/*
 * definirMotorGranular
 * Diz ao motor como o carro está: a marcha, a rotação e o volume (0 a 1).
 */
void definirMotorGranular(MotorGranular *motor, int marcha, float giro, float volume) {
    motor->marcha = marcha;
    motor->giro = limitar(giro, 0.0f, 1.3f);   /* com o Doppler pode passar do limitador */
    motor->volume = limitar(volume, 0.0f, 1.0f);
}

/* A janela de Hann periódica: duas deslocadas de meia janela somam 1 */
static float janela(int k) {
    static float tabela[GRAO_DO_MOTOR];
    static int pronta = 0;
    if (!pronta) {
        for (int i = 0; i < GRAO_DO_MOTOR; i++) {
            tabela[i] = 0.5f - 0.5f * cosf(2.0f * 3.14159265f * (float) i / (float) GRAO_DO_MOTOR);
        }
        pronta = 1;
    }
    return tabela[k];
}

/* A gravação lida entre as amostras (interpolação linear); fora dela, silêncio */
static float amostraEm(const MotorGranular *motor, float posicao) {
    int i = (int) floorf(posicao);
    if (i < 0 || i + 1 >= motor->total) {
        return 0.0f;
    }
    float f = posicao - (float) i;
    return motor->fonte[i] * (1.0f - f) + motor->fonte[i + 1] * f;
}

/*
 * lerReferencia
 * Lê, de passo em passo, as amostras da continuação natural do último pedacinho (o que viria depois
 * dele na gravação), para comparar com cada candidato a próximo pedacinho.
 */
static void lerReferencia(const MotorGranular *motor, float referencia, float taxaDaReferencia, int passo, float *destino, int total) {
    for (int n = 0; n < total; n++) {
        destino[n] = amostraEm(motor, referencia + (float) (n * passo) * taxaDaReferencia);
    }
}

/*
 * casamento
 * Quão parecidas são a continuação natural e um candidato a próximo pedacinho: a correlação
 * normalizada (de -1 a 1) entre as amostras da referência e as do candidato.
 */
static float casamento(const MotorGranular *motor, const float *referencia, int total, float candidato, float taxa, int passo) {
    float soma = 0.0f, energiaA = 0.0f, energiaB = 0.0f;
    for (int n = 0; n < total; n++) {
        float a = referencia[n];
        float b = amostraEm(motor, candidato + (float) (n * passo) * taxa);
        soma += a * b;
        energiaA += a * a;
        energiaB += b * b;
    }
    return soma / sqrtf(energiaA * energiaB + 1e-9f);
}

/*
 * novoPedaco
 * Monta o próximo meio-pedacinho (PASSO_DO_MOTOR amostras): escolhe o ponto da gravação pelo giro e pela
 * marcha, acha onde as ondas casam com o que já saiu, e soma a janela do pedacinho novo à cauda do anterior.
 */
static void novoPedaco(MotorGranular *motor) {
    float tom = limitar(tomDoGiro(motor->giro), 30.0f, 400.0f);
    float segundos, tomDaGravacao;
    pontoDaGravacao(motor->marcha, tom, &segundos, &tomDaGravacao);
    float taxa = limitar(tom / tomDaGravacao, MENOR_TAXA, MAIOR_TAXA);
    motor->tomPedido = tom;

    float comprimento = (float) GRAO_DO_MOTOR * taxa;
    float inicio = segundos * (float) TAXA_DO_MOTOR_GRANULAR - comprimento * 0.5f;

    /* Onde as ondas casam: procura em volta do ponto, comparando com a continuação do pedacinho anterior */
    if (motor->temAnterior) {
        float referencia = motor->inicioAnterior + (float) PASSO_DO_MOTOR * motor->taxaAnterior;
        float grossa[PASSO_DO_MOTOR / 4], fina[PASSO_DO_MOTOR / 2];
        lerReferencia(motor, referencia, motor->taxaAnterior, 4, grossa, PASSO_DO_MOTOR / 4);
        lerReferencia(motor, referencia, motor->taxaAnterior, 2, fina, PASSO_DO_MOTOR / 2);
        float melhor = -2.0f, melhorDesvio = 0.0f;
        for (int d = -FOLGA_DO_CASAMENTO; d <= FOLGA_DO_CASAMENTO; d += DECIMACAO) {
            float c = casamento(motor, grossa, PASSO_DO_MOTOR / 4, inicio + (float) d, taxa, 4);
            if (c > melhor) {
                melhor = c;
                melhorDesvio = (float) d;
            }
        }
        float desvio = melhorDesvio;
        melhor = -2.0f;
        for (int d = -DECIMACAO; d <= DECIMACAO; d++) {   /* refina, amostra por amostra */
            float c = casamento(motor, fina, PASSO_DO_MOTOR / 2, inicio + melhorDesvio + (float) d, taxa, 2);
            if (c > melhor) {
                melhor = c;
                desvio = melhorDesvio + (float) d;
            }
        }
        inicio += desvio;
    }
    /* O pedacinho tem de caber na gravação */
    inicio = limitar(inicio, 0.0f, (float) motor->total - comprimento - 2.0f);

    /* O nível: todo pedacinho sai com o mesmo volume (RMS), para as variações da gravação não virarem trancos */
    float energia = 0.0f;
    for (int k = 0; k < GRAO_DO_MOTOR; k += 8) {
        float a = amostraEm(motor, inicio + (float) k * taxa);
        energia += a * a;
    }
    float rms = sqrtf(energia / (float) (GRAO_DO_MOTOR / 8));
    float ganho = limitar(NIVEL_ALVO / (rms + 1e-5f), 0.4f, 3.0f);

    /* O pedacinho com a janela: a primeira metade soma à cauda do anterior e sai; a segunda vira a cauda nova */
    for (int k = 0; k < PASSO_DO_MOTOR; k++) {
        float a = amostraEm(motor, inicio + (float) k * taxa) * janela(k) * ganho;
        float b = amostraEm(motor, inicio + (float) (k + PASSO_DO_MOTOR) * taxa) * janela(k + PASSO_DO_MOTOR) * ganho;
        motor->pronto[k] = motor->cauda[k] + a;
        motor->cauda[k] = b;
    }
    motor->restam = PASSO_DO_MOTOR;
    motor->inicioAnterior = inicio;
    motor->taxaAnterior = taxa;
    motor->temAnterior = 1;
}

/*
 * gerarMotorGranular
 * Entrega o som do motor, mono, no volume pedido (o volume sobe e desce aos poucos, sem trancos).
 * Recebe: o motor, onde pôr as amostras e quantas pôr.
 * Devolve: nada.
 */
void gerarMotorGranular(MotorGranular *motor, float *saida, int quadros) {
    for (int i = 0; i < quadros; i++) {
        if (motor->restam <= 0) {
            novoPedaco(motor);
        }
        float v = motor->volumeAtual;
        float alvo = motor->volume;
        float passo = SOBE_O_VOLUME / (float) PASSO_DO_MOTOR;
        v = (alvo > v) ? fminf(v + passo, alvo) : fmaxf(v - passo, alvo);
        motor->volumeAtual = v;
        saida[i] = motor->pronto[PASSO_DO_MOTOR - motor->restam] * v;
        motor->restam--;
    }
}
