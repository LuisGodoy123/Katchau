/*
 * ia.c
 * ------------------------------------------------------------------
 * Regras dos pilotos do computador. São só três perguntas por quadro:
 *
 *   1) Vem curva fechada aí? -> olha os próximos segmentos da lista,
 *      calcula a velocidade segura de cada um e se já está na hora de
 *      frear para chegar nela; acelera ou freia conforme a resposta.
 *      No meio da curva, perto do limite, só alivia: acelerar ali faz
 *      o carro sair de frente (veja carro.c).
 *   2) Tem carro logo à frente, na mesma linha? -> muda de linha para
 *      ultrapassar, se o lado estiver livre; senão, freia atrás dele.
 *   3) Estou fora da minha linha? -> vira o volante na direção dela.
 * ------------------------------------------------------------------
 */

#include <stddef.h>
#include <math.h>
#include "ia.h"

#define METROS_PARA_IR_AO_PIT 200.0f  /* quanto antes do pit lane ela já vai para o lado */
#define FOLGA_DO_LIMITE_NA_IA 2.2f    /* metros */
#define LADO_PARA_IR_AO_PIT   0.75f   /* em larguras da pista */
#define SEGMENTOS_OLHADOS         80       /* quantos segmentos à frente a IA enxerga (16000 unidades: */
                                           /* mais que a frenagem de 313 km/h até parar) */
#define FOLGA_NA_FRENAGEM         0.8f     /* a IA conta só 80% da distância: começa a frear um pouco antes */
#define MARGEM_DE_FREIO           1.05f    /* só freia se passar 5% da velocidade desejada */
#define DISTANCIA_PARA_ULTRAPASSAR 35.0f   /* metros: carro da frente mais perto que isso, na mesma linha: tentar passar */
#define DESVIO_ULTRAPASSAGEM      5.2f     /* metros de centro a centro ao passar: mais que a folga do volante e a do "lado livre" */
#define VELOCIDADE_SEM_PERIGO     1200.0f  /* abaixo de 30 km/h encostar não tem problema: não para a fila inteira */
#define FOLGA_PARA_ULTRAPASSAR    1.01f    /* querendo andar 1% mais que o da frente, já vale tentar passar */
#define MARGEM_ATRAS_DO_OUTRO     0.97f    /* freia um pouco antes do limite */
#define ERRO_PARA_VOLANTE_TODO    4.0f     /* metros: longe assim da linha, volante todo; mais perto, menos */
#define LIMITE_LINHA              0.75f    /* a IA nunca escolhe andar mais perto da borda que isso */
#define EMBALO_PARA_O_NITRO       0.70f    /* só usa o nitro acima de 70% da máxima */
#define TANQUE_PARA_O_NITRO       0.60f    /* e só começa com pelo menos isso no tanque */
#define TOLERANCIA_LINHA          0.8f     /* metros: perto assim da linha, não mexe no volante */

/*
 * velocidadeDesejada
 * Olha os próximos SEGMENTOS_OLHADOS nós da pista e, para cada um,
 * calcula a maior velocidade de agora que ainda dá para frear até a
 * velocidade segura dele, na distância que falta até lá. A menor de
 * todas é a que a IA quer: assim ela freia na hora certa antes de cada
 * curva, nem cedo nem tarde, seja qual for o freio do carro.
 * Recebe: o carro.
 * Devolve: a velocidade que a IA quer ter agora.
 */
static float velocidadeDesejada(Carro *carro) {
    float desejada = carro->velocidadeMaxima;
    Segmento *olhando = carro->posicao.segmento;
    float distancia = -carro->posicao.distanciaNoSegmento;   /* até o começo de cada segmento olhado */

    for (int i = 0; i < SEGMENTOS_OLHADOS; i++) {
        float segura = velocidadeSeguraNaCurva(carro, olhando->curvatura);
        float agora = velocidadeParaFrearAte(segura, distancia * FOLGA_NA_FRENAGEM);
        if (agora < desejada) {
            desejada = agora;
        }
        distancia += COMPRIMENTO_SEGMENTO;
        olhando = proximoSegmento(olhando);
    }
    return desejada;
}

/*
 * limitarLinha
 * Impede a IA de escolher uma linha muito perto da grama ou do muro.
 * Recebe: o segmento e a linha desejada, em metros.
 * Devolve: a linha dentro do limite, em metros.
 */
static float limitarLinha(Segmento *segmento, float metros) {
    float limite = larguraDoLado(segmento, metros) * LIMITE_LINHA;
    if (segmento->limiteDireita > 0.0f) {
        /* Onde a pista mediu o que para o carro, a IA guarda distância disso */
        float ateOLimite = ((metros < 0.0f) ? segmento->limiteEsquerda : segmento->limiteDireita) - FOLGA_DO_LIMITE_NA_IA;
        limite = (ateOLimite < limite) ? ateOLimite : limite;
    }
    if (metros < -limite) {
        return -limite;
    }
    if (metros > limite) {
        return limite;
    }
    return metros;
}

/*
 * escolherLinha
 * Se tem carro logo à frente, na mesma linha e mais lento do que este quer andar,
 * vai para o lado com espaço. Lado com alguém encostado não serve.
 * Recebe: o carro e a velocidade que ele quer ter.
 * Devolve: nada (altera carro->linhaAlvo).
 */
static void escolherLinha(Carro *carro, float desejada) {
    if (carro->distanciaAFrente < 0.0f || carro->distanciaAFrente > DISTANCIA_PARA_ULTRAPASSAR
        || desejada <= carro->velocidadeAFrente * FOLGA_PARA_ULTRAPASSAR) {
        return;
    }
    float aqui = carro->deslocamentoLateral * larguraDoLado(carro->posicao.segmento, carro->deslocamentoLateral);
    int vaiPelaDireita = (carro->lateralAFrente <= aqui);
    if (vaiPelaDireita ? carro->ocupadoDireita : carro->ocupadoEsquerda) {
        vaiPelaDireita = !vaiPelaDireita;
    }
    if (vaiPelaDireita ? carro->ocupadoDireita : carro->ocupadoEsquerda) {
        return;   /* os dois lados ocupados: fica atrás (veja atualizarIA) */
    }
    carro->linhaAlvo = carro->lateralAFrente + (vaiPelaDireita ? DESVIO_ULTRAPASSAGEM : -DESVIO_ULTRAPASSAGEM);
}

/*
 * vaiBater
 * Recebe: o carro.
 * Devolve: 1 se, freando agora, ele mal consegue não bater em quem está à frente.
 */
static int vaiBater(Carro *carro) {
    if (carro->distanciaAFrente < 0.0f || carro->velocidade <= carro->velocidadeAFrente
        || carro->velocidade < VELOCIDADE_SEM_PERIGO) {
        return 0;
    }
    float folga = velocidadeParaFrearAte(carro->velocidadeAFrente, carro->distanciaAFrente * UNIDADES_POR_METRO * FOLGA_NA_FRENAGEM);
    return carro->velocidade > folga * MARGEM_ATRAS_DO_OUTRO;
}

/*
 * atualizarIA
 * Decide os comandos do piloto do computador neste quadro.
 * Recebe: o carro da IA (corrida.c já preencheu o que ele enxerga).
 * Devolve: os comandos (acelerar, frear, esquerda, direita).
 */
Comandos atualizarIA(Carro *carro, const Pista *pista) {
    /* A IA sempre dirige com a direção guiada, inclusive com o carro do jogador depois da chegada */
    carro->direcaoLivre = 0;
    carro->desvioDoRumo = 0.0f;
    carro->temRumoNoMundo = 0;
    Comandos comandos = { 0, 0, 0, 0, 0.0f, 0, 0 };

    /* 1) Velocidade: acelera abaixo da desejada (e, dentro da curva,
     *    abaixo do limite de quem acelera), freia bem acima dela. A habilidade
     *    diz quanto do limite do carro este piloto usa. */
    float teto = carro->velocidadeMaxima * carro->habilidade * carro->ritmoDoPelotao;
    /* O ritmo do pelotão mexe no motor (reta); nas curvas, para baixo, mas nunca acima do seguro */
    float desejada = fminf(velocidadeDesejada(carro) * carro->habilidade * fminf(carro->ritmoDoPelotao, 1.0f), teto);
    float podeAcelerar = velocidadeSeguraAcelerando(carro, carro->posicao.segmento->curvatura);
    int retaLivre = (desejada >= teto * 0.999f);
    if (retaLivre) {
        desejada = teto;
    }
    if (carro->velocidade < desejada && carro->velocidade < podeAcelerar) {
        comandos.acelerar = 1;
    } else if (!retaLivre && carro->velocidade > desejada * MARGEM_DE_FREIO) {
        comandos.frear = 1;   /* numa reta livre não freia: acima do teto é vácuo ou nitro */
    }

    /* Nitro: só com a pista livre à frente, depois de embalar, e com um bom tanque */
    if (retaLivre && carro->velocidade > carro->velocidadeMaxima * EMBALO_PARA_O_NITRO
        && carro->distanciaAFrente < 0.0f && (carro->nitroLigado || carro->nitro >= TANQUE_PARA_O_NITRO)) {
        comandos.nitro = 1;
    }

    /* 2) Linha: tenta ultrapassar quem está na frente; chegando na volta de
     *    parar, vai para a esquerda, para entrar no pit lane */
    escolherLinha(carro, desejada);
    carro->querParar = (carro->voltaDoPit > 0 && carro->voltas + 1 == carro->voltaDoPit && !carro->pitFeito);
    float metros = (carro->posicao.segmento->indice * COMPRIMENTO_SEGMENTO + carro->posicao.distanciaNoSegmento) / UNIDADES_POR_METRO;
    if (carro->querParar && metrosDesdeAEntrada(pista, metros + METROS_PARA_IR_AO_PIT) < METROS_PARA_IR_AO_PIT) {
        carro->linhaAlvo = pista->pit.lado * LADO_PARA_IR_AO_PIT
                           * ((pista->pit.lado < 0) ? carro->posicao.segmento->larguraEsquerda : carro->posicao.segmento->larguraDireita);
    }

    /* Sem espaço para desviar e chegando rápido: freia atrás dele */
    if (vaiBater(carro)) {
        comandos.acelerar = 0;
        comandos.nitro = 0;
        comandos.frear = 1;
    }

    /* 3) Volante: gira na direção da linha escolhida, mais perto dela com mais calma */
    Segmento *aqui = carro->posicao.segmento;
    float erro = limitarLinha(aqui, carro->linhaAlvo) - carro->deslocamentoLateral * larguraDoLado(aqui, carro->deslocamentoLateral);
    if (erro > TOLERANCIA_LINHA && !carro->ocupadoDireita) {
        comandos.direita = (erro > ERRO_PARA_VOLANTE_TODO);
        comandos.direcao = erro / ERRO_PARA_VOLANTE_TODO;
    } else if (erro < -TOLERANCIA_LINHA && !carro->ocupadoEsquerda) {
        comandos.esquerda = (erro < -ERRO_PARA_VOLANTE_TODO);
        comandos.direcao = erro / ERRO_PARA_VOLANTE_TODO;
    }
    comandos.direcao += volanteDaFolgaDaCurva(carro);

    return comandos;
}
