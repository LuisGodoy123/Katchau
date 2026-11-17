/*
 * olhos.c
 * ------------------------------------------------------------------
 * O painel dos olhos: a forma de cada expressão, a mistura delas e a conta
 * de olhar para um ponto. Veja olhos.h.
 * ------------------------------------------------------------------
 */

#include <math.h>
#include <string.h>
#include "olhos.h"

#define GRAUS_POR_RADIANO 57.2957795f

/* A forma de cada expressão, com toda a força (o peso 1). As do jogo de sempre (feliz, brava, triste, espantada,
 * sonolenta, concentrada, assustada) são as que já existiam; convencida e desconfiada são as das firulas da
 * garagem. Para ajustar uma cara, é aqui. Cuidado com o "fechar": só a pálpebra de cima mexe, então olho apertado
 * por cima lê como sono. Por isso a feliz quase não fecha (nos filmes, o olho feliz fica aberto e o sorriso é da
 * boca) e a convencida fecha pouco e desce o canto de dentro (o olhar decidido de quem se acha) */
const FormaDosOlhos FORMAS_DAS_EXPRESSOES[TOTAL_EXPRESSOES] = {
    /*                         fechar  inclinar  arco    íris    sobe   assimetria */
    [EXPRESSAO_FELIZ]       = { 0.04f, -0.12f,  0.15f,  0.06f,  0.00f, 0.00f },
    [EXPRESSAO_BRAVA]       = { 0.28f,  0.60f, -0.10f, -0.10f,  0.00f, 0.00f },
    [EXPRESSAO_TRISTE]      = { 0.28f, -0.50f,  0.00f,  0.08f,  0.00f, 0.00f },
    [EXPRESSAO_ESPANTADA]   = {-0.28f,  0.00f,  0.30f, -0.06f,  0.00f, 0.00f },
    [EXPRESSAO_SONOLENTA]   = { 0.80f,  0.00f,  0.00f,  0.00f,  0.00f, 0.00f },
    [EXPRESSAO_CONCENTRADA] = { 0.35f,  0.35f,  0.00f,  0.04f,  0.00f, 0.00f },
    [EXPRESSAO_ASSUSTADA]   = {-0.40f, -0.30f,  0.35f, -0.22f,  0.35f, 0.00f },
    [EXPRESSAO_CONVENCIDA]  = { 0.20f,  0.40f, -0.05f,  0.00f,  0.00f, 0.20f },
    [EXPRESSAO_DESCONFIADA] = { 0.42f,  0.32f, -0.12f, -0.05f,  0.00f, 0.35f },
    [EXPRESSAO_ENTEDIADA]   = { 0.48f,  0.05f, -0.18f, -0.04f, -0.10f, 0.00f },
};

/* O descanso da íris de quem precisa: o modelo desenhou a íris num lugar, o filme mostra em outro. Em raios da íris,
 * o quanto o meio dela desce (+) ou sobe (-) do lugar do desenho, com o olhar parado. Quem não está aqui fica como
 * veio. O nome é o do modelo (recursos/modelos/carros/<nome>.glb) */
static const struct {
    const char *nome;
    float desce;
} IRIS_DE_DESCANSO[] = {
    { "mcqueen", 0.50f },   /* no modelo ela fica alta: muito branco embaixo e a pálpebra por cima, cara de sono */
};

float irisDeDescanso(const char *nome) {
    for (size_t i = 0; nome != NULL && i < sizeof(IRIS_DE_DESCANSO) / sizeof(IRIS_DE_DESCANSO[0]); i++) {
        if (strcmp(IRIS_DE_DESCANSO[i].nome, nome) == 0) {
            return IRIS_DE_DESCANSO[i].desce;
        }
    }
    return 0.0f;
}

static const char *NOMES[TOTAL_EXPRESSOES] = {
    "feliz", "brava", "triste", "espantada", "sonolenta", "concentrada", "assustada", "convencida", "desconfiada",
    "entediada"
};

const char *nomeDaExpressao(Expressao expressao) {
    return ((int) expressao >= 0 && expressao < TOTAL_EXPRESSOES) ? NOMES[expressao] : "?";
}

static float limitarOlho(float v, float a, float b) {
    return (v < a) ? a : (v > b) ? b : v;
}

/*
 * misturarExpressoes
 * Recebe: o peso de cada expressão (0 a 1).
 * Devolve: a forma dos olhos, a soma das formas pelos pesos, nos limites da pálpebra.
 */
FormaDosOlhos misturarExpressoes(const float pesos[TOTAL_EXPRESSOES]) {
    FormaDosOlhos f = { 0 };
    for (int i = 0; i < TOTAL_EXPRESSOES; i++) {
        float p = limitarOlho(pesos[i], 0.0f, 1.0f);
        const FormaDosOlhos *e = &FORMAS_DAS_EXPRESSOES[i];
        f.fechar += e->fechar * p;
        f.inclinar += e->inclinar * p;
        f.arco += e->arco * p;
        f.iris += e->iris * p;
        f.sobe += e->sobe * p;
        f.assimetria += e->assimetria * p;
    }
    f.fechar = limitarOlho(f.fechar, MAIS_ABERTO, MAIS_FECHADO);
    f.inclinar = limitarOlho(f.inclinar, -0.7f, 0.7f);
    f.arco = limitarOlho(f.arco, -0.3f, 0.5f);
    f.sobe = limitarOlho(f.sobe, -1.0f, 1.0f);
    f.assimetria = limitarOlho(f.assimetria, -0.5f, 0.5f);
    return f;
}

/*
 * somarEmocao
 * A emoção do corpo (gestos.c) vira pesos das expressões.
 * Recebe: a emoção e os pesos a somar.
 * Devolve: nada.
 */
void somarEmocao(const Emocao *emocao, float pesos[TOTAL_EXPRESSOES]) {
    pesos[EXPRESSAO_FELIZ] += emocao->alegria;
    pesos[EXPRESSAO_BRAVA] += emocao->raiva;
    pesos[EXPRESSAO_TRISTE] += emocao->tristeza;
    pesos[EXPRESSAO_ESPANTADA] += emocao->espanto;
    pesos[EXPRESSAO_SONOLENTA] += emocao->sono;
}

/*
 * olharParaOPonto
 * Recebe: o ponto, no carro, a partir dos olhos (x para a esquerda, y para cima, z para a frente), e onde guardar.
 * Devolve: nada (o olhar de lado, + para a direita do carro, e para cima, de -1 a +1).
 */
void olharParaOPonto(float x, float y, float z, float *deLado, float *paraCima) {
    float deLadoEmGraus = atan2f(-x, fmaxf(z, 0.05f)) * GRAUS_POR_RADIANO;
    float paraCimaEmGraus = atan2f(y, sqrtf(x * x + z * z)) * GRAUS_POR_RADIANO;
    *deLado = limitarOlho(deLadoEmGraus / ALCANCE_DO_OLHAR_DE_LADO, -1.0f, 1.0f);
    *paraCima = limitarOlho(paraCimaEmGraus / ALCANCE_DO_OLHAR_PARA_CIMA, -1.0f, 1.0f);
}
