/*
 * sondar_colisao.c
 * ------------------------------------------------------------------
 * Procura defeitos na malha de colisão do mundo aberto numa região:
 *   - buracos (pontos sem chão nenhum);
 *   - degraus (o chão muda muito de um ponto para o vizinho sem ser parede);
 *   - carros que enroscam, caem do mundo ou dão saltos impossíveis,
 *     soltando dezenas deles acelerando em direções diferentes.
 *
 * Não usa a raylib: só colisao.c e livre.c.
 *
 * Uso:
 *   make sondar-colisao
 *   ./build/sondar_colisao recursos/modelos/radiator_springs.col x0 z0 x1 z1
 * ------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "colisao.h"
#include "livre.h"
#include "corrida.h"

#define PASSO_DA_GRADE   4.0f    /* metros entre os pontos sondados */
#define DEGRAU_SUSPEITO  2.5f    /* metros de um ponto ao vizinho */
#define CARROS_POR_LADO  6
#define DIRECOES         8
#define SEGUNDOS         8.0f

int main(int argc, char *argv[]) {
    if (argc < 6) {
        printf("uso: %s arquivo.col x0 z0 x1 z1\n", argv[0]);
        return 1;
    }
    MalhaDeColisao *malha = carregarMalhaDeColisao(argv[1]);
    if (malha == NULL) {
        printf("nao deu para ler %s\n", argv[1]);
        return 1;
    }
    float x0 = (float) atof(argv[2]), z0 = (float) atof(argv[3]), x1 = (float) atof(argv[4]), z1 = (float) atof(argv[5]);
    printf("%d triangulos; regiao x %.0f..%.0f, z %.0f..%.0f\n", triangulosDaMalha(malha), x0, x1, z0, z1);

    /* 1) Buracos e degraus */
    int buracos = 0, degraus = 0, pontos = 0;
    for (float z = z0; z <= z1; z += PASSO_DA_GRADE) {
        float anterior = SEM_CHAO;
        for (float x = x0; x <= x1; x += PASSO_DA_GRADE) {
            float h = alturaDoChaoNaMalha(malha, x, z, 2000.0f, NULL);
            pontos++;
            if (h <= SEM_CHAO) {
                if (buracos++ < 10) {
                    printf("  buraco em (%.0f, %.0f)\n", x, z);
                }
            } else if (anterior > SEM_CHAO && fabsf(h - anterior) > DEGRAU_SUSPEITO) {
                degraus++;
            }
            anterior = h;
        }
    }
    printf("chao: %d pontos, %d buracos, %d degraus maiores que %.1f m (de cima)\n", pontos, buracos, degraus, DEGRAU_SUSPEITO);

    /* 2) Carros */
    int enroscados = 0, cairam = 0, saltos = 0, presos = 0, porBaixo = 0, total = 0;
    float maiorAltura = -1e9f;
    for (int i = 0; i < CARROS_POR_LADO; i++) {
        for (int j = 0; j < CARROS_POR_LADO; j++) {
            for (int d = 0; d < DIRECOES; d++) {
                float x = x0 + (x1 - x0) * (i + 0.5f) / CARROS_POR_LADO;
                float z = z0 + (z1 - z0) * (j + 0.5f) / CARROS_POR_LADO;
                float h = alturaDoChaoNaMalha(malha, x, z, 2000.0f, NULL);
                if (h <= SEM_CHAO) {
                    continue;
                }
                CarroLivre carro;
                iniciarCarroLivre(&carro, fichaDoCarro(0), x, h, z, d * 2.0f * 3.14159265f / DIRECOES);
                Comandos c = { 1, 0, 0, 0, 0.0f, 0, 0 };
                int parado = 0, problema = 0;
                float y = carro.y;
                for (float t = 0.0f; t < SEGUNDOS; t += 1.0f / 60.0f) {
                    float xAntes = carro.x, zAntes = carro.z;
                    atualizarCarroLivre(&carro, c, malha, 1.0f / 60.0f);
                    if (fabsf(carro.y - y) > 4.0f && carro.noChao) {
                        problema |= 1;   /* subiu ou desceu 4 m num quadro, no chão */
                    }
                    if (carro.quedas > 0) {
                        problema |= 2;   /* caiu do mundo e voltou ao começo */
                    }
                    float andou = hypotf(carro.x - xAntes, carro.z - zAntes);
                    parado = (andou < 0.02f && t > 1.0f && !carro.raspando) ? parado + 1 : 0;   /* parado sem parede na frente */
                    if (parado > 90) {
                        problema |= 4;
                    }
                    /* Por baixo do chão: há chão bem acima do carro (ele entrou embaixo da casca de um chão) */
                    if (carro.noChao && !carro.naParede && alturaDoChaoNaMalha(malha, carro.x, carro.z, carro.y + 60.0f, NULL) > carro.y + 2.5f) {
                        if (!(problema & 16) && getenv("SONDA_TUDO")) {
                            printf("    por baixo em (%.0f, %.1f, %.0f)\n", carro.x, carro.y, carro.z);
                        }
                        problema |= 16;
                    }
                    y = carro.y;
                    maiorAltura = fmaxf(maiorAltura, carro.y);
                }
                /* Preso de qualquer jeito (até encostado numa parede): em 8 s acelerando, não saiu de perto */
                if (hypotf(carro.x - x, carro.z - z) < 40.0f && !(problema & 2)) {
                    problema |= 8;
                    presos++;
                }
                total++;
                if (problema & 16) { porBaixo++; }
                if (problema & 1) { saltos++; }
                if (problema & 2) { cairam++; }
                if (problema & 4) { enroscados++; }
                if (problema && (saltos + cairam + enroscados + presos) <= (getenv("SONDA_TUDO") ? 100000 : 12)) {
                    printf("  carro de (%.0f, %.0f) rumo %d: %s%s%s%s%s -> parou em (%.0f, %.1f, %.0f)\n", x, z, d * 45,
                           (problema & 1) ? "salto " : "", (problema & 2) ? "caiu " : "", (problema & 4) ? "enroscou " : "", (problema & 8) ? "preso " : "", (problema & 16) ? "por-baixo " : "",
                           carro.x, carro.y, carro.z);
                }
            }
        }
    }
    printf("carros: %d soltos, %d enroscaram, %d presos (andaram menos de 40 m), %d passaram por baixo de um chao, %d cairam, %d saltos; o mais alto chegou a y = %.0f\n",
           total, enroscados, presos, porBaixo, cairam, saltos, maiorAltura);
    liberarMalhaDeColisao(malha);
    return 0;
}
