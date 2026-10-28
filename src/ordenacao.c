/*
 * ordenacao.c
 * ------------------------------------------------------------------
 * Implementação dos algoritmos de ordenação. Veja ordenacao.h.
 * ------------------------------------------------------------------
 */

#include "ordenacao.h"

/*
 * ordenarCarrosPorDistancia  (INSERTION SORT)
 * Coloca os carros em ordem de quem andou mais (1º lugar) para quem
 * andou menos (último).
 *
 * Como funciona: como quem organiza cartas na mão. A parte da esquerda
 * do vetor já está ordenada. Pegamos o próximo carro ("atual") e vamos
 * empurrando para a direita os carros que andaram MENOS que ele, até
 * achar o lugar dele. Repetimos até o fim do vetor.
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

        /* Empurra para a direita quem andou menos que o atual */
        while (j >= 0 && carros[j]->distanciaPercorrida < atual->distanciaPercorrida) {
            carros[j + 1] = carros[j];
            j--;
            deslocamentos++;
        }

        carros[j + 1] = atual;   /* encaixa o atual no buraco que sobrou */
    }

    return deslocamentos;
}
