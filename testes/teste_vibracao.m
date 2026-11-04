/*
 * teste_vibracao.m
 * ------------------------------------------------------------------
 * Para SENTIR a vibração sem abrir o jogo: toca cada situação no controle, uma de
 * cada vez, usando as mesmas contas do jogo (vibracao.c), e escreve qual é.
 * Rode com: make testar-vibracao
 * ------------------------------------------------------------------
 */

#import <Foundation/Foundation.h>
#import <GameController/GameController.h>

#include <stdio.h>
#include <math.h>

#include "vibrar.h"
#include "pista.h"
#include "pitstop.h"

static Corrida corrida;
static EstadoDaVibracao estado;

/* A situação de cada amostra: muda o carro a cada quadro */
typedef void (*Situacao)(Carro *carro, float tempo);

static void asfaltoDevagar(Carro *c, float t) { (void) t; c->velocidade = 3000.0f; }
static void asfaltoMedio(Carro *c, float t) { (void) t; c->velocidade = 7000.0f; }
static void asfaltoRapido(Carro *c, float t) { (void) t; c->velocidade = 11500.0f; }
static void pneuNoLimite(Carro *c, float t) { (void) t; c->velocidade = 7000.0f; c->derrapagem = 0.9f; }
static void asfaltoEDepoisDerrapa(Carro *c, float t) { c->velocidade = 7000.0f; c->derrapagem = (t > 1.2f) ? 1.0f : 0.0f; }
static void drift(Carro *c, float t) {
    c->velocidade = 6000.0f;
    c->forcaDoDrift = (t > 0.3f && t < 2.2f) ? 1.0f : 0.0f;
    c->anguloDeDrift = c->forcaDoDrift * 30.0f;
    c->derrapagem = c->forcaDoDrift;
}
static void ligarNitro(Carro *c, float t) { c->velocidade = 7000.0f; c->nitroLigado = (t > 0.4f && t < 2.2f); c->nitro = 1.0f; }
static void nitroAcabando(Carro *c, float t) { c->velocidade = 9000.0f; c->nitroLigado = (t < 2.0f); c->nitro = 0.05f; }
static void trocaDeMarcha(Carro *c, float t) { c->velocidade = 7000.0f; c->motor.marcha = 1 + (int) (t / 0.6f); }
static void freadaForte(Carro *c, float t) { (void) t; c->velocidade = 9000.0f; c->ultimosComandos.frear = 1; }
static void arrancada(Carro *c, float t) { (void) t; c->velocidade = 200.0f; c->ultimosComandos.acelerar = 1; }
static void grama(Carro *c, float t) { (void) t; c->velocidade = 3000.0f; c->deslocamentoLateral = 1.6f; }
static void batida(Carro *c, float t) { c->velocidade = 7000.0f; c->pancada = (t > 0.5f && t < 0.52f) ? 1.0f : 0.0f; }
static void pistolaDoGuido(Carro *c, float t) { (void) t; c->velocidade = 0.0f; c->estadoDoPit = PIT_TROCANDO; c->tempoDaTroca = SEGUNDOS_PARA_A_RODA + 0.1f; c->duracaoDaTroca = DURACAO_DA_TROCA; }

typedef struct {
    const char *nome;
    Situacao situacao;
} Amostra;

static const Amostra AMOSTRAS[] = {
    { "Asfalto a 75 km/h", asfaltoDevagar },
    { "Asfalto a 175 km/h", asfaltoMedio },
    { "Asfalto a 290 km/h", asfaltoRapido },
    { "Asfalto, e aos 1,2 s o pneu escorrega: o chao some e entra o tremor", asfaltoEDepoisDerrapa },
    { "Pneu no limite", pneuNoLimite },
    { "Drift: a traseira solta, fica de lado, e o pneu agarra no fim", drift },
    { "Nitro: liga, queima e apaga", ligarNitro },
    { "Nitro com o tanque acabando: engasga", nitroAcabando },
    { "Trocas de marcha", trocaDeMarcha },
    { "Freada forte, com ABS", freadaForte },
    { "Arrancada: roda patinando, na mao e no gatilho do acelerador", arrancada },
    { "Grama", grama },
    { "Batida", batida },
    { "Pistola do Guido", pistolaDoGuido },
};

int main(void) {
    @autoreleasepool {
        iniciarVibracao();
        printf("Procurando o controle: ligue o DualSense agora (15 s)...\n");
        for (int i = 0; i < 150 && [[GCController controllers] count] == 0; i++) {
            [[NSRunLoop mainRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.1]];
        }
        NSArray *controles = [GCController controllers];
        if (controles.count == 0) {
            printf("Nenhum controle encontrado. Confira se ele esta conectado em Ajustes > Bluetooth.\n");
            encerrarVibracao();
            return 1;
        }
        for (GCController *controle in controles) {
            printf("Achei: %s | vibra: %s | gatilhos adaptativos: %s\n", controle.vendorName.UTF8String,
                   controle.haptics != nil ? "sim" : "NAO",
                   [controle.extendedGamepad isKindOfClass:[GCDualSenseGamepad class]] ? "sim" : "NAO");
        }
        [[NSRunLoop mainRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.5]];

        Pista *pista = criarPista();
        for (size_t a = 0; a < sizeof(AMOSTRAS) / sizeof(AMOSTRAS[0]); a++) {
            printf("  %zu) %s\n", a + 1, AMOSTRAS[a].nome);
            fflush(stdout);
            iniciarCorrida(&corrida, pista, 0, NIVEL_DIFICIL);
            corrida.contagemRegressiva = 0.0f;
            iniciarEstadoDaVibracao(&estado);
            estado.numeroDaContagemAntes = 0;
            corrida.jogador.posicao.segmento = buscarSegmento(pista, 550);   /* uma reta */
            corrida.jogador.posicao.distanciaNoSegmento = 0.0f;
            corrida.jogador.deslocamentoLateral = 0.0f;
            estado.marchaAntes = corrida.jogador.motor.marcha;
            NSDate *inicio = [NSDate date];
            float anterior = 0.0f;
            while (true) {
                float tempo = [[NSDate date] timeIntervalSinceDate:inicio];
                if (tempo >= 3.0f) {
                    break;
                }
                AMOSTRAS[a].situacao(&corrida.jogador, tempo);
                Vibracao v = calcularVibracao(&estado, &corrida, tempo - anterior);
                anterior = tempo;
                aplicarVibracao(&v);
                [[NSRunLoop mainRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.008]];
            }
            pararVibracao();
            [[NSRunLoop mainRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.8]];
        }
        liberarPista(pista);
        encerrarVibracao();
        [[NSRunLoop mainRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.2]];
        printf("Fim do teste.\n");
    }
    return 0;
}
