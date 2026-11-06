/*
 * simular_dificuldade.c
 * ------------------------------------------------------------------
 * Mede, sem gráficos, o quanto cada dificuldade é competitiva: corre
 * muitas provas com um "jogador simulado" no lugar do jogador e conta
 * as vitórias, as posições, o tempo disputando posição e as trocas.
 *
 * O jogador simulado é a própria IA do jogo no carro do jogador, com
 * um perfil de habilidade (de iniciante a perfeito) e jeito de gente:
 * o ritmo varia de volta em volta e, de vez em quando, ele erra
 * (tira o pé ou freia sem precisar).
 *
 * As provas cobrem todas as pistas e vários carros do jogador. Cada
 * prova tem a sua semente, então o resultado é sempre o mesmo.
 *
 * Como rodar (na raiz do projeto):
 *     make simulacao
 *     ./build/simular_dificuldade -r     -> rápido (menos provas)
 *     ./build/simular_dificuldade -n 1 -h 0.93 0.86
 *         -> só o nível 1 (médio), com outra habilidade para o rival da pole
 *            e para o último (para calibrar sem recompilar)
 * ------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "pista.h"
#include "carro.h"
#include "corrida.h"
#include "ia.h"

#define QUADRO              (1.0f / 60.0f)
#define MINUTOS_MAXIMOS     20.0f        /* uma prova que passar disso conta como abandono */
#define PERTO_PARA_DISPUTA  1.0f         /* segundos: um rival mais perto que isso é disputa */

/* Os perfis de jogador */
typedef struct {
    const char *nome;
    float habilidade;      /* quanto do limite do carro ele usa, na média */
    float variacao;        /* o quanto isso muda de volta em volta */
    float errosPorMinuto;  /* tirar o pé ou frear sem precisar */
} Perfil;

static const Perfil PERFIS[] = {
    { "Iniciante", 0.84f, 0.025f, 3.0f },
    { "Casual",    0.89f, 0.020f, 2.0f },
    { "Bom",       0.93f, 0.015f, 1.2f },
    { "Muito bom", 0.96f, 0.010f, 0.6f },
    { "Perfeito",  0.99f, 0.004f, 0.1f },
};
#define TOTAL_PERFIS ((int) (sizeof(PERFIS) / sizeof(PERFIS[0])))
#define PERFIL_DE_REFERENCIA 2   /* o "Bom": é por ele que sai a nota */

/* O resultado somado de várias provas */
typedef struct {
    int provas, vitorias, podios, abandonos;
    double somaPosicao;
    double tempoEmDisputa, tempoTotal;
    double trocas, voltas;
    double somaDistanciaAoVencedor;   /* segundos: quanto chegou atrás do vencedor (0 se venceu) */
    double somaVantagem;              /* segundos: quanto chegou na frente do 2º, quando venceu */
} Resultado;

static unsigned int sorteio;
static float habilidadeDaPole = -1.0f, habilidadeDoUltimo = -1.0f;   /* -1: a do nível */

static float sortear(void) {
    sorteio = sorteio * 1664525u + 1013904223u;
    return (float) (sorteio >> 8) / 16777216.0f;
}

/*
 * correr
 * Uma prova inteira, do grid até o jogador cruzar a chegada.
 * Recebe: a pista, o nível, o carro do jogador, o perfil, a semente e onde somar o resultado.
 * Devolve: nada.
 */
static void correr(Pista *pista, NivelDaCorrida nivel, int carro, const Perfil *perfil, unsigned int semente,
                   Resultado *resultado) {
    static Corrida corrida;   /* grande demais para a pilha */
    sorteio = semente;
    iniciarCorrida(&corrida, pista, carro, nivel);
    Carro *jogador = &corrida.jogador;
    jogador->habilidade = perfil->habilidade;
    if (habilidadeDaPole > 0.0f) {
        for (int i = 0; i < TOTAL_RIVAIS; i++) {
            corrida.rivais[i].habilidade = habilidadeDaPole - (habilidadeDaPole - habilidadeDoUltimo) * i / (TOTAL_RIVAIS - 1);
        }
    }

    int quadros = 0, quadrosMaximos = (int) (MINUTOS_MAXIMOS * 60.0f / QUADRO);
    int voltaVista = -1, posicaoAntes = TOTAL_CARROS;
    float errando = 0.0f;
    int erroFreia = 0;
    while (!corrida.terminou && quadros < quadrosMaximos) {
        /* O ritmo dele muda um pouco a cada volta */
        if (jogador->voltas != voltaVista) {
            voltaVista = jogador->voltas;
            float desvio = (sortear() + sortear() + sortear() - 1.5f) * 2.0f * perfil->variacao;
            jogador->habilidade = fminf(1.0f, perfil->habilidade + desvio);
        }
        Comandos comandos = atualizarIA(jogador, pista);
        /* E de vez em quando ele erra: tira o pé, ou freia sem precisar */
        if (errando <= 0.0f && corrida.contagemRegressiva <= 0.0f
            && sortear() < perfil->errosPorMinuto * QUADRO / 60.0f) {
            errando = 0.3f + 0.6f * sortear();
            erroFreia = sortear() < 0.35f;
        }
        if (errando > 0.0f) {
            errando -= QUADRO;
            comandos.acelerar = 0;
            comandos.nitro = 0;
            comandos.frear = erroFreia;
        }
        atualizarCorrida(&corrida, comandos, QUADRO);
        quadros++;
        if (corrida.contagemRegressiva > 0.0f) {
            continue;
        }

        /* Disputa: alguém a menos de 1 s, na frente ou atrás */
        int posicao = posicaoDoCarro(&corrida, jogador);
        int emDisputa = 0;
        if (posicao > 1) {
            emDisputa |= fabsf(intervaloEmSegundos(jogador, corrida.classificacao[posicao - 2])) < PERTO_PARA_DISPUTA;
        }
        if (posicao < TOTAL_CARROS) {
            emDisputa |= fabsf(intervaloEmSegundos(corrida.classificacao[posicao], jogador)) < PERTO_PARA_DISPUTA;
        }
        resultado->tempoEmDisputa += emDisputa ? QUADRO : 0.0f;
        resultado->tempoTotal += QUADRO;
        if (posicao != posicaoAntes) {
            resultado->trocas += 1.0;
            posicaoAntes = posicao;
        }
    }
    resultado->trocas -= 1.0;   /* a primeira "troca" é só a posição de largada */

    resultado->provas++;
    if (!corrida.terminou) {
        resultado->abandonos++;
        resultado->somaPosicao += TOTAL_CARROS;
        return;
    }
    int posicao = corrida.posicaoFinal;
    resultado->somaPosicao += posicao;
    resultado->vitorias += (posicao == 1);
    resultado->podios += (posicao <= 3);
    resultado->voltas += corrida.totalVoltas;
    /* A distância para o vencedor (ou do 2º, se venceu): termina a prova com a IA para ver os tempos */
    float chegadaJogador = jogador->tempoDeChegada;
    for (int extra = 0; extra < (int) (90.0f / QUADRO) && !todosChegaram(&corrida); extra++) {
        atualizarCorrida(&corrida, (Comandos){ 0 }, QUADRO);
    }
    float melhorRival = 1e9f;
    for (int i = 0; i < TOTAL_RIVAIS; i++) {
        if (corrida.rivais[i].tempoDeChegada > 0.0f) {
            melhorRival = fminf(melhorRival, corrida.rivais[i].tempoDeChegada);
        }
    }
    if (posicao == 1) {
        resultado->somaVantagem += (melhorRival < 1e8f) ? melhorRival - chegadaJogador : 0.0f;
    } else {
        resultado->somaDistanciaAoVencedor += (melhorRival < 1e8f) ? chegadaJogador - melhorRival : 0.0f;
    }
}

/*
 * nota
 * A nota de competitividade (0 a 10) de um nível, pelo jogador de referência:
 *   - pressão (40%): a fração do tempo com um rival a menos de 1 s;
 *   - desafio (35%): o quanto ele NÃO ganha fácil (100% de vitória = 0; 0% = 1);
 *   - equilíbrio (25%): o quanto as chegadas são apertadas (até 10 s do vencedor, ou de
 *     vantagem quando ganha: perto de 0 é 1, 10 s ou mais é 0).
 * Recebe: o resultado.
 * Devolve: a nota.
 */
static float nota(const Resultado *r) {
    float pressao = (float) (r->tempoEmDisputa / fmax(r->tempoTotal, 1e-9));
    float vitoria = (float) r->vitorias / (float) r->provas;
    float perdeu = (float) (r->provas - r->vitorias);
    float folga = (float) ((r->somaDistanciaAoVencedor + r->somaVantagem) / r->provas);
    (void) perdeu;
    float equilibrio = fmaxf(0.0f, 1.0f - folga / 10.0f);
    return 10.0f * (0.40f * fminf(1.0f, pressao / 0.6f) + 0.35f * (1.0f - vitoria) + 0.25f * equilibrio);
}

int main(int argc, char **argv) {
    int rapido = 0, soONivel = -1;
    for (int a = 1; a < argc; a++) {
        if (strcmp(argv[a], "-r") == 0) {
            rapido = 1;
        } else if (strcmp(argv[a], "-n") == 0 && a + 1 < argc) {
            soONivel = atoi(argv[++a]);
        } else if (strcmp(argv[a], "-h") == 0 && a + 2 < argc) {
            habilidadeDaPole = (float) atof(argv[++a]);
            habilidadeDoUltimo = (float) atof(argv[++a]);
        }
    }
    /* Os carros do jogador: todos, ou um de cada "tipo" no modo rápido */
    int carros[TOTAL_CARROS], totalDeCarros = 0;
    for (int c = 0; c < TOTAL_CARROS; c++) {
        if (!rapido || c % 3 == 0) {
            carros[totalDeCarros++] = c;
        }
    }
    Pista *pistas[TOTAL_PISTAS];
    for (int p = 0; p < TOTAL_PISTAS; p++) {
        pistas[p] = criarPistaPorId((IdPista) p);
    }
    static Resultado resultados[TOTAL_NIVEIS][TOTAL_PERFIS];
    memset(resultados, 0, sizeof(resultados));

    for (int n = 0; n < TOTAL_NIVEIS; n++) {
        if (soONivel >= 0 && n != soONivel) {
            continue;
        }
        for (int f = 0; f < TOTAL_PERFIS; f++) {
            for (int p = 0; p < TOTAL_PISTAS; p++) {
                for (int c = 0; c < totalDeCarros; c++) {
                    unsigned int semente = 12345u + (unsigned int) (((n * 31 + f) * 31 + p) * 31 + c);
                    correr(pistas[p], (NivelDaCorrida) n, carros[c], &PERFIS[f], semente, &resultados[n][f]);
                }
            }
            Resultado *r = &resultados[n][f];
            fprintf(stderr, "  %s / %s: %d provas\n", fichaDoNivel((NivelDaCorrida) n)->nome, PERFIS[f].nome, r->provas);
        }
    }

    printf("\nChance de vitória (e posição média) por nível e por jogador\n");
    printf("%-10s", "");
    for (int f = 0; f < TOTAL_PERFIS; f++) {
        printf(" | %-15s", PERFIS[f].nome);
    }
    printf("\n");
    for (int n = 0; n < TOTAL_NIVEIS; n++) {
        if (soONivel >= 0 && n != soONivel) {
            continue;
        }
        printf("%-10s", fichaDoNivel((NivelDaCorrida) n)->nome);
        for (int f = 0; f < TOTAL_PERFIS; f++) {
            Resultado *r = &resultados[n][f];
            printf(" | %5.1f%% (%4.1fº) ", 100.0 * r->vitorias / r->provas, r->somaPosicao / r->provas);
        }
        printf("\n");
    }

    printf("\nDetalhes com o jogador de referência (%s)\n", PERFIS[PERFIL_DE_REFERENCIA].nome);
    printf("%-10s | vitória | pódio | posição | em disputa | trocas/volta | atrás do vencedor | vantagem | nota\n", "");
    for (int n = 0; n < TOTAL_NIVEIS; n++) {
        if (soONivel >= 0 && n != soONivel) {
            continue;
        }
        Resultado *r = &resultados[n][PERFIL_DE_REFERENCIA];
        int perdeu = r->provas - r->vitorias;
        printf("%-10s | %6.1f%% | %4.1f%% | %6.1fº | %9.1f%% | %12.2f | %15.1f s | %6.1f s | %4.1f\n",
               fichaDoNivel((NivelDaCorrida) n)->nome, 100.0 * r->vitorias / r->provas, 100.0 * r->podios / r->provas,
               r->somaPosicao / r->provas, 100.0 * r->tempoEmDisputa / r->tempoTotal, r->trocas / fmax(r->voltas, 1.0),
               perdeu ? r->somaDistanciaAoVencedor / perdeu : 0.0, r->vitorias ? r->somaVantagem / r->vitorias : 0.0,
               nota(r));
    }

    printf("\nNota de competitividade (0 a 10) por nível e por jogador\n");
    for (int n = 0; n < TOTAL_NIVEIS; n++) {
        if (soONivel >= 0 && n != soONivel) {
            continue;
        }
        printf("%-10s", fichaDoNivel((NivelDaCorrida) n)->nome);
        for (int f = 0; f < TOTAL_PERFIS; f++) {
            printf(" | %-9s %4.1f", PERFIS[f].nome, nota(&resultados[n][f]));
        }
        printf("\n");
    }
    int abandonos = 0;
    for (int n = 0; n < TOTAL_NIVEIS; n++) {
        if (soONivel >= 0 && n != soONivel) {
            continue;
        }
        for (int f = 0; f < TOTAL_PERFIS; f++) {
            abandonos += resultados[n][f].abandonos;
        }
    }
    printf("\nProvas que passaram de %.0f minutos (abandono): %d\n", MINUTOS_MAXIMOS, abandonos);

    for (int p = 0; p < TOTAL_PISTAS; p++) {
        liberarPista(pistas[p]);
    }
    return 0;
}
