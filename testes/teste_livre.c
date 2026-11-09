/*
 * teste_livre.c
 * ------------------------------------------------------------------
 * Testa no terminal, sem gráficos, a malha de colisão (colisao.c) e o
 * carro do passeio livre (livre.c): primeiro num mundo de mentira
 * (um chão, uma parede, uma marquise e uma rampa), depois, se o arquivo
 * existir, no mapa de verdade (recursos/modelos/radiator_springs.col).
 *
 * Como rodar (na raiz do projeto):
 *     make testes
 *
 * Cada verificação imprime [OK] ou [FALHOU]. No fim aparece o total.
 * ------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "colisao.h"
#include "livre.h"
#include "corrida.h"

static int total = 0, falhas = 0;

static void verificar(int condicao, const char *texto) {
    total++;
    if (!condicao) {
        falhas++;
    }
    printf("[%s] %s\n", condicao ? "OK" : "FALHOU", texto);
}

/* Um quadrilátero (dois triângulos) a, b, c, d */
static int quadrado(float *v, int i, const float *a, const float *b, const float *c, const float *d) {
    const float *pontos[6] = { a, b, c, a, c, d };
    for (int p = 0; p < 6; p++) {
        for (int e = 0; e < 3; e++) {
            v[i * 9 + p * 3 + e] = pontos[p][e];
        }
    }
    return i + 2;
}

/*
 * O mundo de mentira, em metros:
 *   - chão em y = 0, de -200 a 200 em x e z;
 *   - parede em pé em x = 60, de z = -100 a 100, com 40 m de altura;
 *   - marquise em y = 3, de x = -40 a -20 (o carro passa por baixo);
 *   - rampa de x = -100 a -80 em z de 40 a 60, subindo 4 m, terminando no vazio sobre o chão.
 */
static MalhaDeColisao *mundoDeMentira(void) {
    float v[9 * 8];
    int n = 0;
    float c0[3] = { -200, 0, -200 }, c1[3] = { 200, 0, -200 }, c2[3] = { 200, 0, 200 }, c3[3] = { -200, 0, 200 };
    n = quadrado(v, n, c0, c1, c2, c3);
    float p0[3] = { 60, 0, -100 }, p1[3] = { 60, 0, 100 }, p2[3] = { 60, 40, 100 }, p3[3] = { 60, 40, -100 };
    n = quadrado(v, n, p0, p1, p2, p3);
    float m0[3] = { -40, 3, -10 }, m1[3] = { -20, 3, -10 }, m2[3] = { -20, 3, 10 }, m3[3] = { -40, 3, 10 };
    n = quadrado(v, n, m0, m1, m2, m3);
    float r0[3] = { -100, 0, 40 }, r1[3] = { -80, 4, 40 }, r2[3] = { -80, 4, 60 }, r3[3] = { -100, 0, 60 };
    n = quadrado(v, n, r0, r1, r2, r3);
    return criarMalhaDeColisao(v, n);
}

static Comandos acelerando(void) {
    Comandos c = { 1, 0, 0, 0, 0.0f, 0, 0 };
    return c;
}

static void andar(CarroLivre *carro, Comandos comandos, MalhaDeColisao *malha, float segundos) {
    for (float t = 0.0f; t < segundos; t += 1.0f / 60.0f) {
        atualizarCarroLivre(carro, comandos, malha, 1.0f / 60.0f);
    }
}

static void testarMalha(MalhaDeColisao *malha) {
    printf("\n--- Malha de colisão (mundo de mentira) ---\n");
    verificar(malha != NULL && triangulosDaMalha(malha) == 8, "a malha tem os 8 triângulos");
    float normal[3];
    verificar(fabsf(alturaDoChaoNaMalha(malha, 0, 0, 5, normal)) < 1e-4f && normal[1] > 0.99f, "o chão em (0, 0) está em y = 0, virado para cima");
    verificar(fabsf(alturaDoChaoNaMalha(malha, -30, 0, 0.9f, NULL)) < 1e-4f, "embaixo da marquise, o chão é o de baixo");
    verificar(fabsf(alturaDoChaoNaMalha(malha, -30, 0, 10, NULL) - 3.0f) < 1e-4f, "de cima, o chão é a marquise");
    verificar(fabsf(alturaDoChaoNaMalha(malha, -90, 50, 10, NULL) - 2.0f) < 1e-3f, "no meio da rampa, o chão está a 2 m");
    verificar(alturaDoChaoNaMalha(malha, 500, 0, 10, NULL) <= SEM_CHAO, "fora do mundo não há chão");
    float centro[3] = { 59.5f, 1.0f, 0.0f }, empurrao[3], ceu[3] = { 0, 1, 0 };
    verificar(empurrarDasParedes(malha, centro, 1.0f, ceu, empurrao) && empurrao[0] < -0.4f, "encostado na parede, ela empurra para -x");
    float longe[3] = { 50.0f, 1.0f, 0.0f };
    verificar(!empurrarDasParedes(malha, longe, 1.0f, ceu, empurrao), "longe da parede, nada empurra");
    float deLado[3] = { 1.0f, 0.0f, 0.0f };
    float naParede[3] = { 59.5f, 1.5f, 0.0f };   /* longe do chão (1,5 m), encostado na parede */
    verificar(!empurrarDasParedes(malha, naParede, 1.0f, deLado, empurrao), "para um carro na parede, a própria parede é chão, não parede");
    float pertoDoChao[3] = { 59.5f, 0.6f, 0.0f };
    verificar(empurrarDasParedes(malha, pertoDoChao, 1.0f, deLado, empurrao) && empurrao[1] > 0.3f,
              "e, para ele, o chão é parede: perto dele, ela o empurra para cima");
    Acerto acerto;
    float origem[3] = { 40.0f, 1.0f, 0.0f }, direcao[3] = { 1.0f, 0.0f, 0.0f };
    verificar(raioNaMalha(malha, origem, direcao, 50.0f, &acerto) && fabsf(acerto.ponto[0] - 60.0f) < 1e-3f && acerto.normal[0] < -0.99f,
              "um raio para +x bate na parede em x = 60, com a normal voltada para ele");
    float paraBaixo[3] = { 0.0f, -1.0f, 0.0f }, alto[3] = { -30.0f, 10.0f, 0.0f };
    verificar(raioNaMalha(malha, alto, paraBaixo, 20.0f, &acerto) && fabsf(acerto.ponto[1] - 3.0f) < 1e-3f, "um raio para baixo para na marquise");
}

static void testarCarro(MalhaDeColisao *malha) {
    printf("\n--- Carro livre (mundo de mentira) ---\n");
    CarroLivre carro;
    iniciarCarroLivre(&carro, fichaDoCarro(0), 0, 0, 0, 3.14159265f / 2.0f);   /* olhando para +x */
    andar(&carro, acelerando(), malha, 2.0f);
    verificar(velocidadeParaFrente(&carro) > 12.0f && carro.x > 10.0f, "acelerando 2 s para o leste, passa de 43 km/h");
    verificar(fabsf(carro.y) < 1e-3f && carro.noChao, "no chão plano, continua no chão");
    andar(&carro, acelerando(), malha, 6.0f);
    verificar(carro.x < 60.0f - carro.largura * 0.5f + 0.05f, "não atravessa a parede");
    verificar(fabsf(velocidadeParaFrente(&carro)) < 8.0f, "na parede, perde a velocidade");

    Comandos freando = { 0, 1, 0, 0, 0.0f, 0, 0 };
    iniciarCarroLivre(&carro, fichaDoCarro(0), 0, 0, 0, 3.14159265f / 2.0f);
    andar(&carro, acelerando(), malha, 2.0f);
    andar(&carro, freando, malha, 1.5f);
    verificar(velocidadeParaFrente(&carro) <= 0.0f, "freando, para (e começa a dar ré)");

    Comandos virando = { 1, 0, 0, 1, 0.0f, 0, 0 };
    iniciarCarroLivre(&carro, fichaDoCarro(0), 0, 0, -150, 0.0f);
    float rumo = carro.rumo;
    andar(&carro, virando, malha, 1.0f);
    verificar(carro.rumo > rumo + 0.3f, "com a seta da direita, o rumo cresce (vira à direita)");

    iniciarCarroLivre(&carro, fichaDoCarro(0), -30, 0, -60, 3.14159265f);   /* olhando para +z, rumo à marquise */
    andar(&carro, acelerando(), malha, 6.0f);
    verificar(carro.z > 20.0f && carro.y < 0.01f, "passa por baixo da marquise sem subir nela");

    iniciarCarroLivre(&carro, fichaDoCarro(0), -130, 0, 50, 3.14159265f / 2.0f);
    int voou = 0;
    for (float t = 0.0f; t < 6.0f; t += 1.0f / 60.0f) {
        atualizarCarroLivre(&carro, acelerando(), malha, 1.0f / 60.0f);
        if (!carro.noChao && carro.y > 3.0f) {
            voou = 1;
        }
    }
    verificar(voou, "a rampa joga o carro para o alto");
    verificar(carro.noChao && fabsf(carro.y) < 1e-3f, "e ele cai de volta no chão");
}

/*
 * O wallride: uma parede comprida (x = 60) e o carro correndo rente a ela, virando aos poucos
 * para dentro. Rápido, ele sobe e anda nela; devagar, só bate.
 */
static void testarWallride(MalhaDeColisao *malha) {
    printf("\n--- Wallride ---\n");
    CarroLivre carro;
    iniciarCarroLivre(&carro, fichaDoCarro(0), 52.0f, 0.0f, 90.0f, 0.0f);   /* olhando para -z, a parede à direita */
    carro.vz = -35.0f;   /* 126 km/h */
    Comandos encostando = { 1, 0, 0, 1, 0.25f, 0, 0 };
    int subiu = 0;
    float maisAlto = 0.0f;
    for (float t = 0.0f; t < 3.0f; t += 1.0f / 60.0f) {
        atualizarCarroLivre(&carro, encostando, malha, 1.0f / 60.0f);
        if (carro.naParede) {
            subiu = 1;
            maisAlto = fmaxf(maisAlto, carro.y);
        }
    }
    verificar(subiu, "rápido e de raspão, o carro sobe na parede");
    verificar(maisAlto > 0.5f && carro.x <= 60.5f, "e anda nela, sem atravessar");

    iniciarCarroLivre(&carro, fichaDoCarro(0), 52.0f, 0.0f, 90.0f, 0.0f);
    carro.vz = -8.0f;   /* 29 km/h */
    Comandos devagar = { 0, 0, 0, 1, 0.6f, 0, 0 };
    subiu = 0;
    for (float t = 0.0f; t < 3.0f; t += 1.0f / 60.0f) {
        atualizarCarroLivre(&carro, devagar, malha, 1.0f / 60.0f);
        subiu |= carro.naParede;
    }
    verificar(!subiu && carro.x < 60.0f, "devagar, a parede segura o carro do lado de cá");

    /* Na parede e sem velocidade, ele cai */
    iniciarCarroLivre(&carro, fichaDoCarro(0), 52.0f, 0.0f, 90.0f, 0.0f);
    carro.vz = -35.0f;
    for (float t = 0.0f; t < 1.5f && !carro.naParede; t += 1.0f / 60.0f) {
        atualizarCarroLivre(&carro, encostando, malha, 1.0f / 60.0f);
    }
    Comandos freando = { 0, 1, 0, 0, 0.0f, 0, 0 };
    for (float t = 0.0f; t < 4.0f; t += 1.0f / 60.0f) {
        atualizarCarroLivre(&carro, freando, malha, 1.0f / 60.0f);
    }
    verificar(!carro.naParede && carro.noChao && carro.y < 0.1f, "freando na parede, ele solta e volta ao chão");

    /* Na parede, o volante manda (arcade): virando para cima o carro sobe, para baixo ele desce */
    iniciarCarroLivre(&carro, fichaDoCarro(0), 52.0f, 0.0f, 95.0f, 0.0f);
    carro.vz = -40.0f;
    for (float t = 0.0f; t < 3.0f && !(carro.naParede && carro.segundosNaParede > 0.2f); t += 1.0f / 60.0f) {
        atualizarCarroLivre(&carro, encostando, malha, 1.0f / 60.0f);
    }
    float antes = carro.y;
    float frente0[3] = { carro.frente[0], carro.frente[1], carro.frente[2] };
    Comandos sobe = { 1, 0, 0, 0, 1.0f, 0, 0 };   /* a parede está à direita: a direita do carro, nela, é para cima */
    for (int i = 0; i < 30; i++) {
        atualizarCarroLivre(&carro, sobe, malha, 1.0f / 60.0f);
    }
    float giro = acosf(fminf(1.0f, frente0[0] * carro.frente[0] + frente0[1] * carro.frente[1] + frente0[2] * carro.frente[2])) * 57.3f;
    printf("     meio segundo virando na parede: girou %.0f graus, subiu %.1f m\n", giro, carro.y - antes);
    verificar(carro.naParede && giro > 30.0f && carro.y > antes + 4.0f, "na parede, virando para cima, o carro gira rápido e sobe");
    Comandos desce = { 1, 0, 0, 0, -1.0f, 0, 0 };
    for (int i = 0; i < 60; i++) {
        atualizarCarroLivre(&carro, desce, malha, 1.0f / 60.0f);
    }
    printf("     1 segundo virando para baixo: o bico ficou com %.2f para cima\n", carro.frente[1]);
    verificar(carro.naParede && carro.frente[1] < -0.1f, "virando para baixo, em um segundo o bico já aponta para baixo, ainda na parede");
    for (int i = 0; i < 240 && carro.naParede; i++) {
        atualizarCarroLivre(&carro, desce, malha, 1.0f / 60.0f);
    }
    Comandos reto = { 1, 0, 0, 0, 0.0f, 0, 0 };   /* no chão, solta o volante */
    for (int i = 0; i < 120; i++) {
        atualizarCarroLivre(&carro, reto, malha, 1.0f / 60.0f);
    }
    verificar(carro.noChao && !carro.naParede && carro.y < 0.5f, "descendo a parede, ele volta para o chão de pé");
}

static void testarMapaDeVerdade(void) {
    MalhaDeColisao *malha = carregarMalhaDeColisao("recursos/modelos/radiator_springs.col");
    printf("\n--- Mapa de verdade ---\n");
    if (malha == NULL) {
        printf("(sem recursos/modelos/radiator_springs.col: pulado)\n");
        return;
    }
    verificar(triangulosDaMalha(malha) > 100000, "o mapa tem mais de 100 mil triângulos");
    float largada[4] = { 450.0f, 1.0f, 90.6f, 3.14159265f / 2.0f }, treino[3] = { 0 };
    verificar(pontosDaMalha(malha, largada, treino), "o arquivo traz a largada e a pista de treino");
    float chao = alturaDoChaoNaMalha(malha, largada[0], largada[2], 51.0f, NULL);
    verificar(chao > -5.0f && chao < 8.0f, "há chão na rua principal, perto de y = 0");
    float doTreino = hypotf(treino[0] - largada[0], treino[2] - largada[2]);
    printf("     a pista de treino fica a %.0f m da largada\n", doTreino);
    verificar(doTreino < 800.0f, "a pista de treino fica perto da cidade (menos de 800 m)");
    /* O anel do oval é terra batida: procura em volta do meio dele */
    int deTerra = 0;
    for (int i = -30; i <= 30; i++) {
        for (int j = -30; j <= 30; j++) {
            Acerto acerto;
            float alto[3] = { treino[0] + i * 8.0f, treino[1] + 80.0f, treino[2] + j * 8.0f }, baixo[3] = { 0, -1, 0 };
            if (raioNaMalha(malha, alto, baixo, 200.0f, &acerto) && tipoDoChao(malha, acerto.superficie) == CHAO_TERRA) {
                deTerra++;
            }
        }
    }
    printf("     %d pontos de terra batida em volta do treino\n", deTerra);
    verificar(deTerra > 40, "o oval de treino é de terra batida");
    CarroLivre carro;
    iniciarCarroLivre(&carro, fichaDoCarro(0), largada[0], largada[1], largada[2], largada[3]);
    voltarAoComeco(&carro, malha);
    float maior = 0.0f;
    for (float t = 0.0f; t < 8.0f; t += 1.0f / 60.0f) {
        atualizarCarroLivre(&carro, acelerando(), malha, 1.0f / 60.0f);
        maior = fmaxf(maior, velocidadeParaFrente(&carro));
    }
    printf("     andou até x = %.0f, z = %.0f, y = %.1f; máxima %.0f km/h\n", carro.x, carro.z, carro.y, maior * 3.6f);
    verificar(maior > 25.0f && carro.x > largada[0] + 100.0f, "anda pela rua principal para o leste sem enroscar");
    liberarMalhaDeColisao(malha);
}

int main(void) {
    MalhaDeColisao *malha = mundoDeMentira();
    testarMalha(malha);
    testarCarro(malha);
    testarWallride(malha);
    liberarMalhaDeColisao(malha);
    testarMapaDeVerdade();
    printf("\n%d de %d verificações passaram.\n", total - falhas, total);
    return falhas == 0 ? 0 : 1;
}
