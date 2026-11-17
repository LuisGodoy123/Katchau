/*
 * olhos.h
 * ------------------------------------------------------------------
 * O painel dos olhos dos carros: tudo o que decide a cara deles fica aqui,
 * num lugar só, para ajustar e para criar reações.
 *
 * Os olhos são uma imagem no para-brisa (carros3d.c): a íris desliza para
 * os lados e para cima e para baixo, e cada pálpebra tem a sua forma:
 *   - fechar:    0 normal, 1 fechada, negativo arregalada;
 *   - inclinar:  > 0 o canto de dentro desce (bravo, decidido); < 0 sobe
 *                (preocupado, amigo);
 *   - arco:      > 0 a pálpebra faz um arco (sorrindo, surpreso);
 *   - íris:      o tamanho dela (+ maior, - menor).
 *
 * Como usar (as funções ficam em carros3d.h; valem para o próximo desenho
 * do carro, então quem quer uma cara chama a cada quadro):
 *   - para onde olha: olharParaNoProximoDesenho(carro, matriz, ponto) olha
 *     para um ponto do mundo (a câmera: contato visual; outro carro; o
 *     jogador), pela pose do carro. Sem pedir, os olhos vão para onde o
 *     volante vira, e de vez em quando dão uma olhadela;
 *   - a expressão: mostrarExpressao(carro, EXPRESSAO_FELIZ, 0.8f). Dá para
 *     somar várias (feliz e convencida); a tabela FORMAS_DAS_EXPRESSOES
 *     (olhos.c) diz a forma de cada uma;
 *   - as reações: piscarAgora, piscarUmOlho (a piscadela), focarOlhar (sem
 *     olhadelas: o olhar fica onde foi pedido, como no contato visual).
 *
 * Não usa a raylib (regra 2 do roadmap).
 * ------------------------------------------------------------------
 */

#ifndef OLHOS_H
#define OLHOS_H

#include "gestos.h"

/* As expressões. Para criar uma nova: um nome aqui e a forma dela em FORMAS_DAS_EXPRESSOES (olhos.c) */
typedef enum {
    EXPRESSAO_FELIZ,         /* aberto, a pálpebra em arco e o canto de dentro erguido (o sorriso é da boca) */
    EXPRESSAO_BRAVA,         /* a pálpebra desce sobre o canto de dentro */
    EXPRESSAO_TRISTE,        /* o canto de dentro sobe e o de fora cai */
    EXPRESSAO_ESPANTADA,     /* curioso, perguntando: arregalado, em arco */
    EXPRESSAO_SONOLENTA,     /* as pálpebras caem */
    EXPRESSAO_CONCENTRADA,   /* o "modo caça" em alta velocidade: apertado e decidido */
    EXPRESSAO_ASSUSTADA,     /* rodando, batendo: arregalado, a íris pequena, olhando para cima */
    EXPRESSAO_CONVENCIDA,    /* um pouco fechado, o canto de dentro descendo e um olho mais que o outro: quem se acha */
    EXPRESSAO_DESCONFIADA,   /* apertado e torto: "hm..." */
    EXPRESSAO_ENTEDIADA,     /* não impressionado: a pálpebra pesada e reta, o olhar um pouco baixo ("e daí?") */
    TOTAL_EXPRESSOES
} Expressao;

typedef struct {
    float fechar, inclinar, arco;
    float iris;          /* somado ao tamanho normal (1) */
    float sobe;          /* o olhar sobe (+) ou desce (-), de -1 a 1 */
    float assimetria;    /* > 0: o olho direito do carro mais fechado que o esquerdo (o olhar de canto) */
} FormaDosOlhos;

extern const FormaDosOlhos FORMAS_DAS_EXPRESSOES[TOTAL_EXPRESSOES];

/* A forma dos olhos com estes pesos (0 a 1 cada; somados, sem passar dos limites da pálpebra) */
FormaDosOlhos misturarExpressoes(const float pesos[TOTAL_EXPRESSOES]);
/* Soma aos pesos a emoção do corpo (gestos.c: alegria, raiva, tristeza, espanto e sono) */
void somarEmocao(const Emocao *emocao, float pesos[TOTAL_EXPRESSOES]);
const char *nomeDaExpressao(Expressao expressao);

/* O descanso da íris de um personagem (a tabela fica em olhos.c): em raios dela, o quanto ela fica abaixo do lugar
 * em que o modelo a desenhou (0 para quem não está na tabela). O nome é o do modelo, sem o ".glb" */
float irisDeDescanso(const char *nome);

/* Para onde os olhos vão, olhando para um ponto: o ponto em metros, no carro, a partir dos olhos dele (x para a
 * esquerda do carro, y para cima, z para a frente). Devolve o olhar de lado (-1 a +1, + para a direita do carro) e
 * para cima (-1 a +1), já no alcance dos olhos */
void olharParaOPonto(float x, float y, float z, float *deLado, float *paraCima);

/* ---------- Os ajustes globais ---------- */
/* O alcance: a quantos graus do bico a íris chega ao fim do caminho dela. Medido na garagem: a câmera fica a 31° de
 * lado e 4° acima dos olhos (Luigi, McQueen, Mate e Sally), e o olhar que encara nas fotos é 0,7 de lado e 0,3 para
 * cima. Mais alcance: os olhos andam menos para o mesmo ponto */
#define ALCANCE_DO_OLHAR_DE_LADO     44.0f
#define ALCANCE_DO_OLHAR_PARA_CIMA   14.0f
/* Onde ficam os olhos no carro (frações da altura e do comprimento, a partir do meio, no chão) */
#define OLHOS_NA_ALTURA       0.78f
#define OLHOS_NA_FRENTE       0.12f
/* Seguindo o volante: um pouco de volante já vira bem os olhos */
#define CURVA_DO_OLHAR        0.55f
#define SOBE_O_OLHAR          0.45f     /* quanto a íris sobe ou desce, em raios dela */
/* Os limites da pálpebra, somando tudo */
#define MAIS_FECHADO          0.95f
#define MAIS_ABERTO          -0.45f
/* O susto: a íris treme de lado (frações do quanto ela anda) e passa em tanto tempo */
#define SUSTO_TREME           0.10f
#define SUSTO_PASSA           0.80f
/* As molas: a frequência (Hz) e o amortecimento de cada parte da cara */
#define MOLA_DO_OLHAR         3.2f, 0.55f
#define MOLA_DA_PALPEBRA      2.2f, 0.75f
#define MOLA_DA_INCLINACAO    2.0f, 0.80f
#define MOLA_DO_ARCO          2.0f, 0.60f
#define MOLA_DA_IRIS          4.0f, 0.50f
#define MOLA_DA_ASSIMETRIA    2.0f, 0.75f
/* As piscadas: de tempos em tempos, sorteadas */
#define PISCA_DE_TEMPOS       2.2f      /* segundos, no mínimo... */
#define PISCA_ATE             5.5f      /* ...e no máximo */
#define DURACAO_DA_PISCADA    0.17f
#define PISCADA_DUPLA         0.20f     /* a chance de piscar duas vezes */
#define DURACAO_DA_PISCADELA  0.45f     /* um olho só: fecha, segura um pouco e abre */
/* Os reflexos de um olho de verdade: a pálpebra de cima acompanha o olhar (olhando para cima, ela sobe junto;
 * para baixo, desce: sem isso, o olho para cima parece arregalado), e num salto grande do olhar (do volante para
 * alguém, de um lado para o outro), às vezes ele pisca no caminho */
#define PALPEBRA_SEGUE_O_OLHAR 0.22f    /* o quanto a pálpebra fecha por olhar todo para baixo */
#define SALTO_DO_OLHAR        0.5f      /* o caminho (no alcance dos olhos) que conta como salto */
#define PISCA_NO_SALTO        0.6f      /* a chance de piscar num salto */
#define PISCA_NO_SALTO_A_CADA 0.8f      /* segundos desde a última piscada, no mínimo */
/* As olhadelas: com o olhar solto, ele muda um pouco de vez em quando */
#define OLHADELA_DE_TEMPOS    0.5f
#define OLHADELA_ATE          1.8f
#define OLHADELA_DE_LADO      0.22f
#define OLHADELA_DE_ALTURA    0.15f

#endif
