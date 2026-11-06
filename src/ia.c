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
 *
 * E o jeito de correr, que muda com o nível (corrida.c preenche):
 *   - o ritmo varia um pouco de volta em volta, como o de gente;
 *   - quem vem atrás pega o vácuo de propósito: entra na linha do carro
 *     da frente, e na hora de passar sai e usa o nitro;
 *   - quem vem colado atrás numa reta, mais rápido, encontra a porta
 *     fechada: a IA vai para a linha dele uma vez e segura;
 *   - no revide (foi ultrapassada agora), freia mais tarde e força mais.
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
#define FOLGA_NA_FRENAGEM_REVIDE  0.88f    /* no revide, conta mais da distância: freia mais tarde */
#define VACUO_DE                  8.0f     /* metros: mais perto que isso, já é hora de passar, não de seguir */
#define VACUO_ATE                 55.0f    /* e mais longe que isso, não vale a pena */
#define NITRO_PARA_PASSAR         45.0f    /* metros: com alguém mais perto que isso à frente, o nitro ajuda a passar */
#define CURVA_DE_RETA             0.15f    /* curvatura: menos que isso é reta (onde se defende e se pega vácuo) */
#define DEFESA_PERTO              18.0f    /* metros: quem vem mais perto que isso atrás pode encontrar a porta fechada */
#define DEFESA_MAIS_RAPIDO        1.01f    /* e só se vier mais rápido */
#define VAO_PARA_DEFENDER         3.0f     /* metros: com ele mais perto que isso, já não dá para fechar */
#define ANTES_DO_PIT              300.0f   /* metros: daqui até o fim da zona de entrada, vácuo e defesa ficam */
                                           /* longe do lado do pit (onde a pista emenda no pit lane, encostar */
                                           /* lá é entrar nele) */

/*
 * velocidadeDesejada
 * Olha os próximos SEGMENTOS_OLHADOS nós da pista e, para cada um,
 * calcula a maior velocidade de agora que ainda dá para frear até a
 * velocidade segura dele, na distância que falta até lá. A menor de
 * todas é a que a IA quer: assim ela freia na hora certa antes de cada
 * curva, nem cedo nem tarde, seja qual for o freio do carro.
 * Recebe: o carro e quanto da distância ele conta (menos de 1: começa a frear antes).
 * Devolve: a velocidade que a IA quer ter agora.
 */
static float velocidadeDesejada(Carro *carro, float folgaNaFrenagem) {
    float desejada = carro->velocidadeMaxima;
    Segmento *olhando = carro->posicao.segmento;
    float distancia = -carro->posicao.distanciaNoSegmento;   /* até o começo de cada segmento olhado */

    for (int i = 0; i < SEGMENTOS_OLHADOS; i++) {
        float segura = velocidadeSeguraNaCurva(carro, olhando->curvatura);
        float agora = velocidadeParaFrearAte(segura, distancia * folgaNaFrenagem);
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
 * sortearDoPiloto
 * Recebe: o carro (que guarda os sorteios dele).
 * Devolve: um número de 0 a 1.
 */
static float sortearDoPiloto(Carro *carro) {
    carro->sorteio = carro->sorteio * 1664525u + 1013904223u;
    return (float) (carro->sorteio >> 8) / 16777216.0f;
}

/*
 * habilidadeAgora
 * A habilidade deste quadro: a do piloto, mais o ritmo desta volta (sorteado a cada volta, como o
 * de gente) e o esforço (atrás do pelotão ou no revide). Nunca passa do limite do carro.
 * Recebe: o carro.
 * Devolve: de 0 a 1.
 */
static float habilidadeAgora(Carro *carro) {
    if (carro->voltas != carro->voltaDoRitmo) {
        carro->voltaDoRitmo = carro->voltas;
        float soma = sortearDoPiloto(carro) + sortearDoPiloto(carro) + sortearDoPiloto(carro);
        carro->ritmoDaVolta = (soma - 1.5f) * 2.0f * carro->variacao;
    }
    return fmaxf(0.5f, fminf(1.0f, carro->habilidade + carro->ritmoDaVolta + carro->esforco));
}

/*
 * pertoDoPit
 * Recebe: o carro e a pista.
 * Devolve: 1 se está chegando na entrada do pit lane, ou na zona dela.
 */
static int pertoDoPit(Carro *carro, const Pista *pista) {
    if (!pista->temPit) {
        return 0;
    }
    float metros = (carro->posicao.segmento->indice * COMPRIMENTO_SEGMENTO + carro->posicao.distanciaNoSegmento) / UNIDADES_POR_METRO;
    return metrosDesdeAEntrada(pista, metros + ANTES_DO_PIT) < ANTES_DO_PIT + metrosDesdeAEntrada(pista, pista->pit.fimDaZona);
}

/*
 * pegarVacuo
 * Numa reta, com alguém mais lento um pouco à frente em outra linha: entra atrás dele para pegar o
 * vácuo. Mais perto, escolherLinha manda sair para passar.
 * Recebe: o carro e a velocidade que ele quer ter.
 * Devolve: nada (altera carro->linhaAlvo).
 */
static void pegarVacuo(Carro *carro, float desejada) {
    if (carro->distanciaDoDaFrente < VACUO_DE || carro->distanciaDoDaFrente > VACUO_ATE
        || fabsf(carro->posicao.segmento->curvatura) > CURVA_DE_RETA
        || desejada <= carro->velocidadeDoDaFrente * FOLGA_PARA_ULTRAPASSAR) {
        return;
    }
    carro->linhaAlvo = carro->lateralDoDaFrente;
}

/*
 * defender
 * Fecha a porta: alguém mais rápido vem colado atrás, numa reta, e ainda não está do lado. Uma
 * vez por encontro (a chance é do nível), vai para a linha dele e segura um pouco. Depois espera
 * antes de defender de novo: mudar de linha toda hora seria jogo sujo. O tempo da defesa e o da
 * espera andam em corrida.c.
 * Recebe: o carro.
 * Devolve: nada (altera carro->linhaAlvo e a defesa).
 */
static void defender(Carro *carro) {
    if (carro->defendendo > 0.0f) {
        carro->linhaAlvo = carro->linhaDaDefesa;
        return;
    }
    if (carro->defendendo < 0.0f) {
        return;
    }
    if (carro->defesa <= 0.0f || carro->distanciaAtras < VAO_PARA_DEFENDER || carro->distanciaAtras > DEFESA_PERTO
        || carro->velocidadeAtras <= carro->velocidade * DEFESA_MAIS_RAPIDO
        || fabsf(carro->posicao.segmento->curvatura) > CURVA_DE_RETA || carro->ocupadoEsquerda || carro->ocupadoDireita) {
        return;
    }
    if (sortearDoPiloto(carro) < carro->defesa) {
        carro->defendendo = SEGUNDOS_DE_DEFESA;
        carro->linhaDaDefesa = carro->lateralAtras;
        carro->linhaAlvo = carro->linhaDaDefesa;
    } else {
        carro->defendendo = -ESPERA_PARA_DEFENDER;   /* desta vez deixou: não sorteia de novo a cada quadro */
    }
}

/*
 * atualizarIA
 * Decide os comandos do piloto do computador neste quadro.
 * Recebe: o carro da IA (corrida.c já preencheu o que ele enxerga) e a pista.
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
    float habilidade = habilidadeAgora(carro);
    float teto = carro->velocidadeMaxima * habilidade * carro->ritmoDoPelotao;
    /* O ritmo do pelotão mexe no motor (reta); nas curvas, para baixo, mas nunca acima do seguro */
    float folga = (carro->revide > 0.0f) ? FOLGA_NA_FRENAGEM_REVIDE : FOLGA_NA_FRENAGEM;
    float desejada = fminf(velocidadeDesejada(carro, folga) * habilidade * fminf(carro->ritmoDoPelotao, 1.0f), teto);
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

    /* Nitro: depois de embalar, com um bom tanque, numa reta livre na sua linha. Com alguém logo à
     * frente em outra linha, o nitro serve para passar; no revide, para retomar a posição */
    int livreNaLinha = carro->distanciaAFrente < 0.0f;
    int paraPassar = carro->distanciaDoDaFrente >= 0.0f && carro->distanciaDoDaFrente < NITRO_PARA_PASSAR;
    if (retaLivre && carro->velocidade > carro->velocidadeMaxima * EMBALO_PARA_O_NITRO && livreNaLinha
        && (carro->nitroLigado || carro->nitro >= TANQUE_PARA_O_NITRO || (paraPassar && carro->revide > 0.0f))) {
        comandos.nitro = 1;
    }

    /* 2) Linha: pega o vácuo de quem está um pouco à frente, tenta ultrapassar quem está logo à
     *    frente, defende de quem vem atrás; chegando na volta de parar, vai para o lado do pit lane */
    int pitPerto = pertoDoPit(carro, pista);
    if (!pitPerto) {
        pegarVacuo(carro, desejada);
    }
    escolherLinha(carro, desejada);
    if (!carro->controladoPeloJogador && !pitPerto) {
        defender(carro);
    } else if (carro->defendendo > 0.0f) {
        carro->defendendo = 0.0f;   /* chegando no pit, larga a defesa */
    }
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
