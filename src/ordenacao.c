/*
 * ordenacao.c
 * ------------------------------------------------------------------
 * Implementação dos algoritmos de ordenação. Veja ordenacao.h.
 * ------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include "ordenacao.h"

/* ==================================================================
 * INSERTION SORT: classificação da corrida
 * ================================================================== */

/*
 * estaNaFrente
 * A regra de quem está na frente na corrida:
 *   1) quem já completou a corrida fica na frente de quem ainda corre;
 *   2) entre dois que já chegaram, ganha quem chegou antes (menor tempo);
 *   3) entre dois que ainda correm, ganha quem andou mais.
 * Sem a regra 1, um carro que chegou em 3º poderia "subir" no resultado
 * ultrapassando alguém depois da bandeirada.
 * Recebe: dois carros.
 * Devolve: 1 se "a" está na frente de "b", 0 se não.
 */
static int estaNaFrente(Carro *a, Carro *b) {
    int aChegou = a->tempoDeChegada > 0.0f;
    int bChegou = b->tempoDeChegada > 0.0f;

    if (aChegou && bChegou) {
        return a->tempoDeChegada < b->tempoDeChegada;
    }
    if (aChegou != bChegou) {
        return aChegou;
    }
    return a->distanciaPercorrida > b->distanciaPercorrida;
}

/*
 * ordenarCarrosPorDistancia  (INSERTION SORT)
 * Coloca os carros em ordem de corrida, do 1º lugar ao último, pela
 * regra de estaNaFrente() (na prática: quem andou mais vai na frente).
 *
 * Como funciona: como quem organiza cartas na mão. A parte da esquerda
 * do vetor já está ordenada. Pegamos o próximo carro ("atual") e vamos
 * empurrando para a direita os carros que estão ATRÁS dele, até achar o
 * lugar dele. Repetimos até o fim do vetor.
 *
 * Por que Insertion Sort aqui: a ordem é refeita a cada quadro (60 vezes
 * por segundo) e, de um quadro para o outro, quase nada muda: no máximo
 * um carro passa outro. Com o vetor quase ordenado o laço de dentro
 * quase nunca roda, e o custo fica perto de O(n). O pior caso, vetor
 * ao contrário, é O(n²), mas isso não acontece numa corrida.
 *
 * Recebe: o vetor de ponteiros e quantos carros ele tem.
 * Devolve: quantos deslocamentos foram feitos (0 se já estava ordenado).
 *          Serve para ver na prática que vetor quase ordenado custa pouco.
 */
int ordenarCarrosPorDistancia(Carro *carros[], int quantidade) {
    int deslocamentos = 0;

    for (int i = 1; i < quantidade; i++) {
        Carro *atual = carros[i];
        int j = i - 1;

        /* Empurra para a direita quem está atrás do atual */
        while (j >= 0 && estaNaFrente(atual, carros[j])) {
            carros[j + 1] = carros[j];
            j--;
            deslocamentos++;
        }

        carros[j + 1] = atual;   /* encaixa o atual no buraco que sobrou */
    }

    return deslocamentos;
}

/* ==================================================================
 * MERGE SORT: fichas da garagem
 * ================================================================== */

/*
 * valorDoAtributo
 * Recebe: uma ficha e o atributo desejado.
 * Devolve: o valor daquele atributo na ficha.
 */
float valorDoAtributo(const FichaTecnica *ficha, Atributo atributo) {
    switch (atributo) {
        case ATRIBUTO_VELOCIDADE:
            return ficha->velocidadeMaxima;
        case ATRIBUTO_ACELERACAO:
            return ficha->aceleracao;
        case ATRIBUTO_ADERENCIA:
            return ficha->aderencia;
        default:
            return 0.0f;
    }
}

/*
 * nomeDoAtributo
 * Recebe: o atributo.
 * Devolve: o nome dele para mostrar na tela.
 */
const char *nomeDoAtributo(Atributo atributo) {
    switch (atributo) {
        case ATRIBUTO_VELOCIDADE:
            return "VELOCIDADE";
        case ATRIBUTO_ACELERACAO:
            return "ACELERACAO";
        case ATRIBUTO_ADERENCIA:
            return "ADERENCIA";
        default:
            return "?";
    }
}

/*
 * intercalar
 * O coração do Merge Sort. Recebe duas metades vizinhas que já estão
 * ordenadas, fichas[inicio..meio] e fichas[meio+1..fim], e junta as duas
 * numa só, também ordenada (do maior valor para o menor).
 *
 * Como funciona: olha a primeira ficha de cada metade, copia a maior para
 * o vetor auxiliar e avança naquela metade. Quando uma metade acaba, copia
 * o que sobrou da outra. No fim, copia o auxiliar de volta.
 *
 * Em caso de empate, a ficha da metade esquerda vai primeiro (o ">=").
 * Isso deixa o Merge Sort ESTÁVEL: carros empatados mantêm a ordem
 * em que estavam antes.
 *
 * Recebe: o vetor, o vetor auxiliar, os limites das metades e o atributo.
 * Devolve: nada.
 */
static void intercalar(const FichaTecnica *fichas[], const FichaTecnica *auxiliar[],
                       int inicio, int meio, int fim, Atributo atributo) {
    int esquerda = inicio;    /* próxima ficha da metade esquerda */
    int direita = meio + 1;   /* próxima ficha da metade direita */
    int destino = inicio;     /* próxima posição livre no auxiliar */

    while (esquerda <= meio && direita <= fim) {
        float valorEsquerda = valorDoAtributo(fichas[esquerda], atributo);
        float valorDireita = valorDoAtributo(fichas[direita], atributo);

        if (valorEsquerda >= valorDireita) {
            auxiliar[destino] = fichas[esquerda];
            esquerda++;
        } else {
            auxiliar[destino] = fichas[direita];
            direita++;
        }
        destino++;
    }

    /* Uma das metades acabou: o resto da outra já está em ordem */
    while (esquerda <= meio) {
        auxiliar[destino] = fichas[esquerda];
        esquerda++;
        destino++;
    }
    while (direita <= fim) {
        auxiliar[destino] = fichas[direita];
        direita++;
        destino++;
    }

    for (int i = inicio; i <= fim; i++) {
        fichas[i] = auxiliar[i];
    }
}

/*
 * mergeSortRecursivo
 * Divide o trecho fichas[inicio..fim] ao meio, ordena cada metade
 * chamando a si mesma e depois intercala as duas.
 * Um trecho com 1 ficha (ou nenhuma) já está ordenado: é onde a
 * recursão para.
 * Recebe: o vetor, o auxiliar, os limites do trecho e o atributo.
 * Devolve: nada.
 */
static void mergeSortRecursivo(const FichaTecnica *fichas[], const FichaTecnica *auxiliar[],
                               int inicio, int fim, Atributo atributo) {
    if (inicio >= fim) {
        return;
    }

    int meio = (inicio + fim) / 2;
    mergeSortRecursivo(fichas, auxiliar, inicio, meio, atributo);
    mergeSortRecursivo(fichas, auxiliar, meio + 1, fim, atributo);
    intercalar(fichas, auxiliar, inicio, meio, fim, atributo);
}

/*
 * ordenarFichasPorAtributo  (MERGE SORT)
 * Ordena as fichas da garagem do maior para o menor valor do atributo
 * escolhido (velocidade, aceleração ou aderência).
 *
 * Por que Merge Sort aqui: na garagem o jogador troca o critério e a
 * lista inteira muda de ordem, então não dá para contar com ela "quase
 * ordenada" como na corrida. O Merge Sort custa O(n log n) em qualquer
 * caso e é estável, então quem empata no atributo novo continua na ordem
 * do critério anterior.
 *
 * Recebe: o vetor de ponteiros para fichas, o tamanho e o atributo.
 * Devolve: nada (o vetor é ordenado no lugar).
 */
void ordenarFichasPorAtributo(const FichaTecnica *fichas[], int quantidade, Atributo atributo) {
    if (quantidade < 2) {
        return;
    }

    /* Vetor auxiliar para o intercalar(), do mesmo tamanho do original */
    const FichaTecnica **auxiliar = malloc(quantidade * sizeof(const FichaTecnica *));
    if (auxiliar == NULL) {
        printf("Erro: memoria insuficiente para ordenar a garagem.\n");
        return;
    }

    mergeSortRecursivo(fichas, auxiliar, 0, quantidade - 1, atributo);
    free(auxiliar);
}
