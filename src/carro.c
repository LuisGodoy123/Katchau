/*
 * carro.c
 * ------------------------------------------------------------------
 * Movimento do carro. A física é de fliperama, não realista:
 *
 *   - acelerar soma velocidade, como num F1: muita força na arrancada
 *     e cada vez menos perto da máxima (o ar segura o carro). O câmbio
 *     é automático (motor.c) e corta a potência por um instante a cada
 *     troca de marcha;
 *   - frear tira forte (~5 g); soltar o pedal tira pouco: só o ar e o
 *     atrito, mais em alta que em baixa. Quem quer perder velocidade
 *     de verdade antes da curva tem que usar o freio;
 *   - na grama a velocidade cai até um limite baixo;
 *   - virar muda a posição lateral; em baixa velocidade a direção já
 *     responde bem, e parado o carro não vira;
 *   - na curva o carro é empurrado para fora, e o empurrão cresce
 *     com o quadrado da velocidade: em alta, ou se freia antes da
 *     curva, ou se vai parar na grama. Perto desse limite o carro
 *     "derrapa" (carro->derrapagem), e o desenho solta fumaça;
 *   - o pé também conta na curva: acelerando, o carro sai de frente e
 *     é mais empurrado; aliviando ou freando, ele segura melhor;
 *   - passando do limite, o pneu arrasta e o carro perde velocidade,
 *     o que acaba devolvendo o controle (caro, mas não é grama na hora);
 *   - colado atrás de outro carro (vácuo), o ar segura menos e a
 *     velocidade máxima sobe um pouco.
 * ------------------------------------------------------------------
 */

#include <string.h>
#include <math.h>
#include "carro.h"

#define FORCA_FREIO             6000.0f  /* quanto a velocidade cai por segundo freando (~5 g, como um F1) */
#define ATRITO_DE_ROLAMENTO      300.0f  /* sem acelerar, cai isso por segundo parado... */
#define ARRASTO_DO_AR            900.0f  /* ...mais isso vezes (velocidade / máxima)^2: 1200 na máxima */
#define CORTE_NA_TROCA             0.2f  /* na troca de marcha sobra 20% da força do motor */
#define ATRITO_GRAMA            4500.0f  /* quanto cai por segundo na grama */
#define VELOCIDADE_MAXIMA_GRAMA 3000.0f  /* na grama o carro não passa disso */
#define VELOCIDADE_DE_VIRADA       2.0f  /* larguras de pista por segundo, na velocidade máxima */
#define GIRO_DO_VOLANTE            6.0f  /* com a seta apertada, o volante vai de 0 a 1 em ~0,17 s */
#define RETORNO_DO_VOLANTE         8.0f  /* sem seta, o volante volta ao centro em ~0,12 s */
#define FORCA_CENTRIFUGA           6.0f  /* intensidade do empurrão para fora nas curvas */
#define FORCA_DA_ARRANCADA         0.75f /* a aceleração parado é 75% da "aceleracao" da ficha... */
#define PERDA_NA_MAXIMA            0.8f  /* ...e na velocidade máxima cai 80% (sobram 20%) */
#define INICIO_DA_DERRAPAGEM       0.7f  /* a curva usando 70% do volante: começa a derrapar */
#define LIMITE_LATERAL             2.5f  /* até onde o carro pode ir pela grama */
#define EMPURRAO_SEM_PEDAL         0.9f  /* aliviando na curva, o empurrão para fora é 10% menor... */
#define EMPURRAO_FREANDO           0.8f  /* ...e freando, 20% (acelerando: o empurrão inteiro) */
#define PERDA_NA_DERRAPAGEM    30000.0f  /* passando 10% do limite, perde 3000 por segundo (1/4 do freio)... */
#define EXCESSO_MAXIMO             0.5f  /* ...e a perda para de crescer em 50% além do limite */
#define GANHO_NO_VACUO             0.06f /* no vácuo cheio, a máxima sobe 6% (300 -> 318 km/h) */
#define FOLGA_DO_MURO              1.4f  /* o centro do carro para a esta distância do muro (metros: meia largura do carro) */
#define PERDA_NO_MURO           5000.0f  /* raspando no muro, perde isso de velocidade por segundo */
#define KMH_POR_UNIDADE            0.025f /* 12000 unidades/s viram 300 km/h no velocímetro */

/*
 * criarCarro
 * Monta um carro parado na largada.
 * Recebe: nome, segmento de largada, posição lateral no grid e a ficha técnica.
 * Devolve: o carro pronto (a struct inteira, por valor).
 */
Carro criarCarro(const char *nome, int numero, Segmento *largada, float deslocamentoLateral,
                 float velocidadeMaxima, float aceleracao, float aderencia) {
    Carro carro;

    strncpy(carro.nome, nome, TAMANHO_NOME - 1);
    carro.nome[TAMANHO_NOME - 1] = '\0';
    carro.numero = numero;

    carro.posicao.segmento = largada;
    carro.posicao.distanciaNoSegmento = 0.0f;
    carro.distanciaPercorrida = 0.0f;
    carro.deslocamentoLateral = deslocamentoLateral;

    carro.velocidade = 0.0f;
    carro.volante = 0.0f;
    carro.derrapagem = 0.0f;
    carro.vacuo = 0.0f;
    iniciarMotor(&carro.motor);
    carro.voltas = 0;
    for (int i = 0; i < TOTAL_VOLTAS; i++) {
        carro.temposDeVolta[i] = 0.0f;
    }
    carro.inicioDaVolta = 0.0f;
    carro.melhorVolta = 0.0f;
    carro.setorAtual = 0;
    carro.inicioDoSetor = 0.0f;
    for (int s = 0; s < TOTAL_SETORES; s++) {
        carro.setoresDaVolta[s] = 0.0f;
        carro.melhoresSetores[s] = 0.0f;
        carro.setoresDaMelhorVolta[s] = 0.0f;
    }
    carro.diferenca = 0.0f;
    carro.temDiferenca = 0;
    carro.tempoDeChegada = 0.0f;
    Comandos nenhumComando = { 0, 0, 0, 0, 0.0f };
    carro.ultimosComandos = nenhumComando;
    carro.linhaAlvo = deslocamentoLateral;

    carro.velocidadeMaxima = velocidadeMaxima;
    carro.aceleracao = aceleracao;
    carro.aderencia = aderencia;
    return carro;
}

/*
 * limitar
 * Prende um valor entre um mínimo e um máximo.
 * Recebe: o valor e os limites.
 * Devolve: o valor dentro dos limites.
 */
static float limitar(float valor, float minimo, float maximo) {
    if (valor < minimo) {
        return minimo;
    }
    if (valor > maximo) {
        return maximo;
    }
    return valor;
}

/*
 * estaForaDaPista
 * Recebe: o carro.
 * Devolve: 1 se o carro está na grama, 0 se está no asfalto.
 */
int estaForaDaPista(Carro *carro) {
    return carro->deslocamentoLateral < -LIMITE_PISTA ||
           carro->deslocamentoLateral > LIMITE_PISTA;
}

/*
 * forcaDoMotor
 * Quanto a velocidade sobe por segundo com o pé no fundo. Curva de F1:
 * a força cai com o quadrado da velocidade (o ar segura o carro). Com a
 * ficha do Katchau: 0-100 km/h em ~2,4 s e 0-200 em ~5 s. Durante a troca
 * de marcha a potência é cortada (sobra CORTE_NA_TROCA).
 * Recebe: a velocidade, a velocidade máxima, a aceleração da ficha e o motor.
 * Devolve: a aceleração, em unidades por segundo ao quadrado.
 */
static float forcaDoMotor(float velocidade, float maxima, float aceleracao, const Motor *motor) {
    float proporcao = velocidade / maxima;
    float forca = aceleracao * FORCA_DA_ARRANCADA * (1.0f - PERDA_NA_MAXIMA * proporcao * proporcao);
    return estaTrocandoDeMarcha(motor) ? forca * CORTE_NA_TROCA : forca;
}

/*
 * perdaSemAcelerar
 * Quanto a velocidade cai por segundo com o pé fora do acelerador (e do
 * freio): um pouco de atrito sempre e o ar, que cresce com a velocidade.
 * Recebe: o carro.
 * Devolve: a perda, em unidades por segundo ao quadrado.
 */
static float perdaSemAcelerar(Carro *carro) {
    float proporcao = carro->velocidade / carro->velocidadeMaxima;
    return ATRITO_DE_ROLAMENTO + ARRASTO_DO_AR * proporcao * proporcao;
}

/*
 * atualizarVelocidade
 * Aplica acelerador, freio, ar e grama à velocidade.
 * Recebe: o carro, os comandos e o tempo do quadro em segundos.
 * Devolve: nada (altera carro->velocidade).
 */
static void atualizarVelocidade(Carro *carro, Comandos comandos, float tempoQuadro) {
    float anterior = carro->velocidade;
    float maxima = velocidadeMaximaNoVacuo(carro);

    if (comandos.acelerar) {
        carro->velocidade += forcaDoMotor(carro->velocidade, maxima, carro->aceleracao, &carro->motor) * tempoQuadro;
    } else if (comandos.frear) {
        carro->velocidade -= FORCA_FREIO * tempoQuadro;
    } else {
        carro->velocidade -= perdaSemAcelerar(carro) * tempoQuadro;
    }

    if (estaForaDaPista(carro) && carro->velocidade > VELOCIDADE_MAXIMA_GRAMA) {
        carro->velocidade -= ATRITO_GRAMA * tempoQuadro;
    }

    /* Acima da máxima (acabou de sair do vácuo): o ar tira o excesso aos
     * poucos, em vez de cortar de uma vez */
    if (carro->velocidade > maxima) {
        float freadoPeloAr = anterior - perdaSemAcelerar(carro) * tempoQuadro;
        carro->velocidade = (freadoPeloAr > maxima) ? freadoPeloAr : maxima;
    }
    if (carro->velocidade < 0.0f) {
        carro->velocidade = 0.0f;
    }
}

/*
 * aproximar
 * Leva um valor na direção de um alvo, no máximo "passo" por chamada,
 * sem passar do alvo.
 * Recebe: o valor atual, o alvo e o passo máximo.
 * Devolve: o novo valor.
 */
static float aproximar(float valor, float alvo, float passo) {
    if (valor < alvo) {
        return (valor + passo > alvo) ? alvo : valor + passo;
    }
    return (valor - passo < alvo) ? alvo : valor - passo;
}

/*
 * alvoDoVolante
 * Para onde o piloto quer o volante: as setas pedem tudo (-1 ou +1), o
 * analógico pede só o quanto está inclinado. Com os dois ao mesmo tempo,
 * vale o que pede mais.
 * Recebe: os comandos.
 * Devolve: de -1 (todo à esquerda) a +1 (todo à direita).
 */
static float alvoDoVolante(Comandos comandos) {
    float setas = (float) (comandos.direita - comandos.esquerda);   /* -1, 0 ou +1 */
    float analogico = limitar(comandos.direcao, -1.0f, 1.0f);
    return (fabsf(analogico) > fabsf(setas)) ? analogico : setas;
}

/*
 * atualizarVolante
 * O volante não vai direto para o fim: ele gira aos poucos até o alvo
 * (o fim, com as setas, ou o meio do caminho, com o analógico meio
 * inclinado). Voltando para o centro ele anda mais rápido, como um
 * volante de verdade que volta sozinho.
 * Isso deixa a curva suave e a pose do carro na tela sem trancos.
 * Recebe: o carro, os comandos e o tempo do quadro em segundos.
 * Devolve: nada (altera carro->volante).
 */
static void atualizarVolante(Carro *carro, Comandos comandos, float tempoQuadro) {
    float alvo = alvoDoVolante(comandos);

    /* Voltando: o alvo está mais perto do centro ou do outro lado */
    int voltando = fabsf(alvo) < fabsf(carro->volante) || alvo * carro->volante < 0.0f;
    float rapidez = voltando ? RETORNO_DO_VOLANTE : GIRO_DO_VOLANTE;
    carro->volante = aproximar(carro->volante, alvo, rapidez * tempoQuadro);
}

/*
 * respostaDaDirecao
 * Quanto o volante vira o carro, conforme a velocidade: p * (2 - p),
 * onde p vai de 0 (parado) a 1 (velocidade máxima). Parado dá 0, na
 * máxima dá 1, e no meio dá mais que p: a 100 km/h a direção já tem 56%
 * da força (com uma reta, p, seriam só 33%, e o carro ficava "duro").
 * Recebe: a proporção da velocidade máxima (0 a 1).
 * Devolve: de 0 a 1.
 */
static float respostaDaDirecao(float proporcaoVelocidade) {
    return proporcaoVelocidade * (2.0f - proporcaoVelocidade);
}

/*
 * atualizarLateral
 * Vira o carro conforme o volante e aplica o empurrão da curva.
 * Também mede a derrapagem: quanto do volante a curva está "gastando".
 * Passando do limite (a curva gasta mais que o volante todo), o pneu
 * arrasta e tira velocidade, proporcional a quanto passou.
 * O deslocamento lateral é medido em larguras da pista, e no oval cada lado
 * tem uma largura: a conta corrige isso, para o carro andar de lado os mesmos
 * metros por segundo em qualquer ponto (em Interlagos o fator é 1).
 * Recebe: o carro, os comandos (o pé muda o empurrão) e o tempo do quadro.
 * Devolve: nada (altera deslocamentoLateral, derrapagem e velocidade).
 */
static void atualizarLateral(Carro *carro, Comandos comandos, float tempoQuadro) {
    /* 0 parado, 1 na velocidade máxima */
    float proporcaoVelocidade = carro->velocidade / carro->velocidadeMaxima;
    float direcao = VELOCIDADE_DE_VIRADA * respostaDaDirecao(proporcaoVelocidade);

    /* Volante: o quanto vira depende de quanto o volante está girado */
    Segmento *aqui = carro->posicao.segmento;
    float paraMetros = MEIA_LARGURA_PADRAO / larguraDoLado(aqui, carro->deslocamentoLateral);
    carro->deslocamentoLateral += direcao * carro->volante * paraMetros * tempoQuadro;

    /* Força centrífuga: curva para a direita (curvatura > 0) empurra
     * para a esquerda, e vice-versa. Mais aderência, menos empurrão.
     * Acelerando o carro sai de frente; freando ele crava a dianteira. */
    float pe = EMPURRAO_SEM_PEDAL;
    if (comandos.acelerar) {
        pe = 1.0f;
    } else if (comandos.frear) {
        pe = EMPURRAO_FREANDO;
    }
    float curvatura = carro->posicao.segmento->curvatura;
    float empurrao = curvatura * FORCA_CENTRIFUGA * proporcaoVelocidade * proporcaoVelocidade
                     * (1.0f - carro->aderencia) * pe;
    carro->deslocamentoLateral -= empurrao * paraMetros * tempoQuadro;

    /* Derrapagem: 0 enquanto o empurrão gasta menos de 70% da direção,
     * 1 quando gasta tudo (dali em diante o carro vai para fora) */
    float uso = 0.0f;
    if (direcao > 0.0f) {
        uso = (empurrao < 0 ? -empurrao : empurrao) / direcao;
    }
    carro->derrapagem = limitar((uso - INICIO_DA_DERRAPAGEM) / (1.0f - INICIO_DA_DERRAPAGEM), 0.0f, 1.0f);

    /* Passou do limite: o pneu arrasta. Com menos velocidade o empurrão
     * cai (com o quadrado), e o carro volta a obedecer o volante */
    float excesso = limitar(uso - 1.0f, 0.0f, EXCESSO_MAXIMO);
    carro->velocidade -= PERDA_NA_DERRAPAGEM * excesso * tempoQuadro;
    if (carro->velocidade < 0.0f) {
        carro->velocidade = 0.0f;
    }

    carro->deslocamentoLateral = limitar(carro->deslocamentoLateral, -LIMITE_LATERAL, LIMITE_LATERAL);

    /* Muro (só no oval): o carro não passa dele, e raspar custa velocidade.
     * A largura de cada lado muda ao longo da pista, então a conta é em metros */
    if (aqui->temMuro) {
        float limiteDireita = LIMITE_PISTA - FOLGA_DO_MURO / aqui->larguraDireita;
        float limiteEsquerda = LIMITE_PISTA - FOLGA_DO_MURO / aqui->larguraEsquerda;
        if (carro->deslocamentoLateral > limiteDireita || carro->deslocamentoLateral < -limiteEsquerda) {
            carro->deslocamentoLateral = limitar(carro->deslocamentoLateral, -limiteEsquerda, limiteDireita);
            carro->velocidade = fmaxf(carro->velocidade - PERDA_NO_MURO * tempoQuadro, 0.0f);
        }
    }
}

/*
 * atualizarCarro
 * Avança o carro um quadro: motor (marcha), velocidade, volante, lateral e depois anda
 * pela lista da pista com moverNaPista().
 * Recebe: o carro, os comandos e o tempo do quadro em segundos.
 * Devolve: quantas voltas o carro completou neste quadro (0 ou 1).
 */
int atualizarCarro(Carro *carro, Comandos comandos, float tempoQuadro) {
    carro->ultimosComandos = comandos;
    atualizarMotor(&carro->motor, carro->velocidade / carro->velocidadeMaxima,
                   comandos.acelerar && !comandos.frear, tempoQuadro);
    atualizarVelocidade(carro, comandos, tempoQuadro);
    atualizarVolante(carro, comandos, tempoQuadro);
    atualizarLateral(carro, comandos, tempoQuadro);

    float distancia = carro->velocidade * tempoQuadro;
    carro->distanciaPercorrida += distancia;

    int voltasGanhas = moverNaPista(&carro->posicao, distancia);
    carro->voltas += voltasGanhas;
    return voltasGanhas;
}

/*
 * velocidadeEmKmh
 * Converte a velocidade interna para o velocímetro.
 * Recebe: o carro.
 * Devolve: a velocidade em km/h, arredondada.
 */
int velocidadeEmKmh(Carro *carro) {
    return converterParaKmh(carro->velocidade);
}

/*
 * converterParaKmh
 * Converte qualquer velocidade interna (por exemplo, a velocidade
 * máxima de uma ficha na garagem) para km/h.
 * Recebe: a velocidade em unidades por segundo.
 * Devolve: a velocidade em km/h, arredondada.
 */
int converterParaKmh(float velocidade) {
    return (int) (velocidade * KMH_POR_UNIDADE + 0.5f);
}

/*
 * velocidadeSeguraComPe
 * A maior velocidade em que o volante ainda vence o empurrão da curva.
 * Vem de igualar as duas fórmulas de atualizarLateral(), chamando
 * V = VELOCIDADE_DE_VIRADA e K = curvatura * FORCA_CENTRIFUGA * (1 - aderencia) * pé:
 *
 *     volante  = V * p * (2 - p)
 *     empurrão = K * p * p
 *
 *   volante = empurrão  ->  V * (2 - p) = K * p  ->  p = 2V / (K + V)
 *
 * onde p é a proporção da velocidade máxima (0 a 1).
 * Recebe: o carro, a curvatura e o fator do pé (1 = acelerando).
 * Devolve: a velocidade segura, em unidades por segundo.
 */
static float velocidadeSeguraComPe(Carro *carro, float curvatura, float pe) {
    if (curvatura < 0) {
        curvatura = -curvatura;   /* para a esquerda ou para a direita, tanto faz */
    }
    if (curvatura == 0) {
        return carro->velocidadeMaxima;   /* reta: sem limite */
    }

    float k = curvatura * FORCA_CENTRIFUGA * (1.0f - carro->aderencia) * pe;
    float proporcao = 2.0f * VELOCIDADE_DE_VIRADA / (k + VELOCIDADE_DE_VIRADA);
    if (proporcao > 1.0f) {
        proporcao = 1.0f;   /* curva suave: dá para fazer no máximo */
    }
    return proporcao * carro->velocidadeMaxima;
}

/*
 * velocidadeSeguraNaCurva
 * O limite da curva com o pé fora do acelerador. A IA usa isso para
 * saber quanto frear antes de uma curva.
 * Recebe: o carro e a curvatura da curva.
 * Devolve: a velocidade segura, em unidades por segundo.
 */
float velocidadeSeguraNaCurva(Carro *carro, float curvatura) {
    return velocidadeSeguraComPe(carro, curvatura, EMPURRAO_SEM_PEDAL);
}

/*
 * velocidadeSeguraAcelerando
 * O limite da curva com o pé no fundo, que é mais baixo: acelerando, o
 * carro sai de frente. Abaixo disso a IA pode acelerar sem derrapar.
 * Recebe: o carro e a curvatura da curva.
 * Devolve: a velocidade segura, em unidades por segundo.
 */
float velocidadeSeguraAcelerando(Carro *carro, float curvatura) {
    return velocidadeSeguraComPe(carro, curvatura, 1.0f);
}

/*
 * velocidadeMaximaNoVacuo
 * A máxima de verdade neste momento: a da ficha, mais o ganho do vácuo.
 * Recebe: o carro.
 * Devolve: a velocidade máxima, em unidades por segundo.
 */
float velocidadeMaximaNoVacuo(Carro *carro) {
    return carro->velocidadeMaxima * (1.0f + GANHO_NO_VACUO * carro->vacuo);
}

/*
 * velocidadeParaFrearAte
 * A maior velocidade de agora que ainda dá para baixar até "alvo",
 * freando forte, em "distancia" unidades (v² = alvo² + 2 * freio * d).
 * A IA usa para saber quando começar a frear antes de uma curva.
 * Recebe: a velocidade alvo e a distância que falta.
 * Devolve: a velocidade, em unidades por segundo.
 */
float velocidadeParaFrearAte(float alvo, float distancia) {
    if (distancia <= 0.0f) {
        return alvo;
    }
    return sqrtf(alvo * alvo + 2.0f * FORCA_FREIO * distancia);
}

/*
 * tempoAteVelocidade
 * Quantos segundos o carro leva, saindo parado e acelerando sempre, para
 * chegar a uma velocidade. A garagem usa para mostrar o 0 a 100 km/h.
 * Como a aceleração muda com a velocidade (curva de F1) e o câmbio corta
 * a potência nas trocas, a conta anda em passinhos de 1/240 s com as
 * mesmas fórmulas da corrida.
 * Recebe: a ficha do carro e a velocidade alvo (unidades por segundo).
 * Devolve: o tempo em segundos (0 se o carro nunca chega nela).
 */
float tempoAteVelocidade(const FichaTecnica *ficha, float velocidade) {
    if (velocidade > ficha->velocidadeMaxima) {
        return 0.0f;
    }
    float passo = 1.0f / 240.0f;
    float atual = 0.0f;
    float tempo = 0.0f;
    Motor motor;
    iniciarMotor(&motor);
    while (atual < velocidade) {
        atualizarMotor(&motor, atual / ficha->velocidadeMaxima, 1, passo);
        atual += forcaDoMotor(atual, ficha->velocidadeMaxima, ficha->aceleracao, &motor) * passo;
        tempo += passo;
    }
    return tempo;
}
