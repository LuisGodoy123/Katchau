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

static Segmento *comZebra;   /* um segmento com zebra, e de que lado ela está (+1 direita, -1 esquerda) */
static float ladoDaZebra;
static Corrida *daCorrida;

static void asfaltoRapido(Carro *c, float t) { (void) t; c->velocidade = 11500.0f; }
static void pneuNoLimite(Carro *c, float t) { (void) t; c->velocidade = 7000.0f; c->derrapagem = 0.9f; }
static void drift(Carro *c, float t) {
    c->velocidade = 6000.0f;
    c->forcaDoDrift = (t > 0.3f && t < 2.2f) ? 1.0f : 0.0f;
    c->anguloDeDrift = c->forcaDoDrift * 30.0f;
    c->derrapagem = c->forcaDoDrift;
}
static void ligarNitro(Carro *c, float t) { c->velocidade = 7000.0f; c->nitroLigado = (t > 0.4f && t < 2.2f); c->nitro = 1.0f; }
static void nitroAcabando(Carro *c, float t) { c->velocidade = 9000.0f; c->nitroLigado = (t < 2.0f); c->nitro = 0.05f; }
static void arrancada(Carro *c, float t) { (void) t; c->velocidade = 200.0f; c->ultimosComandos.acelerar = 1; }
static void zebra(Carro *c, float t) {
    (void) t;
    c->velocidade = 6000.0f;
    c->posicao.segmento = comZebra;
    float largura = (ladoDaZebra > 0.0f) ? comZebra->larguraDireita : comZebra->larguraEsquerda;
    float zebra = (ladoDaZebra > 0.0f) ? comZebra->zebraDireita : comZebra->zebraEsquerda;
    c->deslocamentoLateral = ladoDaZebra * (largura + zebra) / 2.0f / largura;
}
static void gramaADireita(Carro *c, float t) { (void) t; c->velocidade = 4000.0f; c->deslocamentoLateral = 1.6f; }
static void muroAEsquerda(Carro *c, float t) { (void) t; c->velocidade = 5000.0f; c->deslocamentoLateral = -0.9f; c->raspandoNoMuro = 1; }
static void batidas(Carro *c, float t) {
    c->velocidade = 7000.0f;
    c->contato = 0.0f;
    if (t > 0.5f && t < 0.52f) {
        c->pancada = 0.25f;   /* uma batida leve: se sente */
    } else if (t > 1.6f && t < 1.62f) {
        c->pancada = 1.0f;    /* uma pancada do lado direito */
        PontoNoMundo aqui = pontoNoMundo(c->posicao, c->deslocamentoLateral);
        float r = aqui.rumo * 0.0174533f;
        c->contato = 1.0f;
        c->contatoX = aqui.x + cosf(r) * 1.2f;
        c->contatoZ = aqui.z + sinf(r) * 1.2f;
    }
}
static void encostadoNoutroCarro(Carro *c, float t) {
    c->velocidade = 7000.0f;
    int encostado = (t > 0.5f && t < 2.0f);
    c->pancada = encostado ? 0.3f : 0.0f;
    c->contatoComCarro = encostado;
    c->contatoDeslize = 4.0f;
    c->contato = 0.0f;   /* sem o ponto do contato: no meio */
}
static void batendoNoMuro(Carro *c, float t) { c->velocidade = 8000.0f; c->deslocamentoLateral = 0.9f; c->raspandoNoMuro = (t > 0.6f && t < 1.8f); }
static void pneuEstourando(Carro *c, float t) { c->velocidade = 6000.0f; c->pneuEstourado = (t > 0.4f) ? 0x2 : 0; }
static void entrandoNoPit(Carro *c, float t) { c->velocidade = 2500.0f; c->estadoDoPit = (t > 0.5f) ? PIT_CHEGANDO : PIT_FORA; }
static void pistolaDoGuido(Carro *c, float t) { (void) t; c->velocidade = 0.0f; c->estadoDoPit = PIT_TROCANDO; c->tempoDaTroca = SEGUNDOS_PARA_A_RODA + 0.1f; c->duracaoDaTroca = DURACAO_DA_TROCA; }
static void vitoria(Carro *c, float t) {
    c->velocidade = 7000.0f;
    if (t > 0.5f) {
        daCorrida->terminou = 1;
        daCorrida->posicaoFinal = 1;
    }
}

typedef struct {
    const char *nome;
    Situacao situacao;
} Amostra;

static const Amostra AMOSTRAS[] = {
    { "Asfalto reto a 290 km/h: o controle fica quieto (de proposito)", asfaltoRapido },
    { "Pneu no limite: um tremor leve e irregular na mao direita", pneuNoLimite },
    { "Drift: o tranco ao soltar, o pneu arrastando do comeco ao fim, e o toque ao agarrar", drift },
    { "Nitro: o empurrao, um ronco baixo que assenta, e a cauda", ligarNitro },
    { "Nitro com o tanque acabando: engasga", nitroAcabando },
    { "Arrancada: roda patinando, na mao e no gatilho do acelerador", arrancada },
    { "Zebra: os relevos, mais na mao do lado dela", zebra },
    { "Grama a direita: forte no comeco, depois assenta", gramaADireita },
    { "Raspando no muro da esquerda", muroAEsquerda },
    { "Uma batida leve e depois uma forte do lado direito", batidas },
    { "Batendo noutro carro e seguindo encostado: o tranco e a lataria raspando", encostadoNoutroCarro },
    { "Batendo no muro da direita e raspando nele", batendoNoMuro },
    { "Pneu dianteiro direito estourando, e o aro raspando", pneuEstourando },
    { "Entrando no pit lane: o piloto automatico assume", entrandoNoPit },
    { "Pistola do Guido", pistolaDoGuido },
    { "Vitoria: a bandeirada e os pulsos de comemoracao", vitoria },
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
        daCorrida = &corrida;
        comZebra = pista->largada;
        while (!(comZebra->zebraDireita > comZebra->larguraDireita + 0.3f || comZebra->zebraEsquerda > comZebra->larguraEsquerda + 0.3f)) {
            comZebra = comZebra->proximo;
        }
        ladoDaZebra = (comZebra->zebraDireita > comZebra->larguraDireita + 0.3f) ? 1.0f : -1.0f;
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
