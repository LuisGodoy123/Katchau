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
 *   - o nitro empurra forte e passa da velocidade máxima, enquanto durar. Ele
 *     recarrega aos poucos o tempo todo, e bem mais rápido freando;
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
#define FORCA_DA_ARRANCADA         0.55f /* a aceleração parado é 55% da "aceleracao" da ficha (era 75%: rápido demais)... */
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
#define NITRO_INICIAL              1.0f  /* larga com o tanque cheio */
#define GASTO_DO_NITRO             0.22f /* por segundo: o tanque cheio dura ~4,5 s */
#define RECARGA_DO_NITRO           0.02f /* por segundo: enche em 50 s andando normal... */
#define RECARGA_FREANDO            0.14f /* ...e em ~7 s freando */
#define MINIMO_PARA_LIGAR          0.10f /* abaixo disso não liga, para não ficar piscando no fim do tanque */
#define GANHO_DO_NITRO             0.18f /* a máxima sobe 18% (300 -> 354 km/h) */
#define FORCA_DO_NITRO             0.90f /* soma isso vezes a "aceleracao" da ficha */
#define COMPRIMENTO_PADRAO         4.4f  /* metros: quem cria o carro à mão recebe este tamanho */
#define LARGURA_PADRAO             2.0f
#define RAIO_MINIMO_DA_DIRECAO    12.0f    /* metros: a curva mais fechada que o volante todo faz, devagar, sem a direção guiada */
#define ACELERACAO_DE_CURVA_BASE  34.0f    /* m/s²: sem a guiada, o que o pneu segura na curva é isto... */
#define ACELERACAO_DE_CURVA_NO_PNEU 18.0f  /* ...mais isto vezes a aderência */
#define AGARRE_NO_DRIFT           0.6f     /* de lado, o bico gira mais depressa */
#define PERDA_NO_LIMITE_DO_PNEU   600.0f   /* o que a velocidade cai por segundo, no limite do pneu */
/* A suspensão: uma mola com amortecedor em cada eixo do corpo, só no desenho */
#define ROLAGEM_POR_G             7.0f     /* graus de rolagem por g, nas curvas leves (depois satura) */
#define ARFAGEM_POR_G             4.0f     /* graus de arfagem por g, nas acelerações leves */
#define MAIOR_ROLAGEM             6.0f     /* um carro de rua passa de 4 graus; aqui um pouco mais, arcade */
#define MAIOR_ARFAGEM             3.5f
#define ROLAGEM_DO_VOLANTE        1.2f     /* graus: o corpo já começa a deitar quando você vira o volante */
#define VELOCIDADE_DO_VOLANTE     30.0f    /* m/s: a partir daqui o volante pesa inteiro */
#define MOLA_DA_SUSPENSAO         150.0f   /* por segundo ao quadrado: responde rápido */
#define AMORTECEDOR               13.0f    /* por segundo: bem abaixo do crítico, balança uma vez e assenta */
#define GRAVIDADE                 9.8f
/* A ré: parado, segurando o freio, engata; o freio vira acelerador para trás */
#define SEGUNDOS_PARA_ENGATAR_A_RE 0.35f
#define MAXIMA_DE_RE              1800.0f  /* unidades por segundo: 45 km/h */
#define ACELERACAO_DE_RE          1500.0f
#define FREIO_DA_RE               3000.0f  /* acelerando, a ré para logo */
#define ATRITO_DA_RE              500.0f
#define RAIO_DA_RE                5.5f     /* metros: o raio da curva com o volante todo, de ré */
#define MAIOR_RUMO_DE_RE          80.0f    /* graus: dá para virar o carro quase de lado */
#define ALINHANDO_DEPOIS_DA_RE    0.5f     /* segundos: andando para a frente, o bico volta a seguir a pista */
#define PASSOS_PROCURANDO_NA_LINHA 200     /* segmentos: procurando onde o carro está */
#define FOLGA_DA_CONTA_NO_LIMITE  0.01f    /* metros */
#define VOLANTE_PARA_SAIR_DO_LIMITE 0.1f
#define GIRO_SAINDO_DO_LIMITE     120.0f   /* graus por segundo: manobrando para sair do limite */
#define RAPIDEZ_DA_FUGA           7.0f     /* por segundo: o quanto o ângulo de fuga acompanha */
#define MAIOR_ANGULO_DE_FUGA      28.0f    /* graus */
#define METROS_POR_SEGUNDO_MINIMOS 10.0f   /* devagar, o ângulo não dispara */
#define FREIO_DE_MAO              800.0f  /* o que a velocidade cai por segundo com o freio de mão: menos que o freio */
#define VELOCIDADE_PARA_O_DRIFT   1400.0f  /* abaixo de 35 km/h a traseira não solta */
#define VOLANTE_PARA_O_DRIFT      0.15f    /* com o volante reto, o carro só perde velocidade */
#define SOLTANDO_A_TRASEIRA       7.0f     /* por segundo: o freio de mão quebra a traseira */
#define MANTENDO_O_DRIFT          0.16f    /* por segundo: acelerando de lado, a traseira quase não volta */
#define VOLTANDO_DO_DRIFT         2.6f     /* por segundo: sem acelerar, ou quase reto, ela volta logo */
#define ANGULO_DE_MANTER          7.0f     /* graus: menos que isso, o drift acabou */
#define MAIOR_ANGULO_DO_DRIFT     55.0f    /* graus */
#define TORQUE_DO_VOLANTE         110.0f   /* graus/s²: o volante, ou o contraesterço, gira o carro */
#define FORCA_DO_CONTRAESTERCO    3.5f
#define FOLGA_DA_TRASEIRA         1.6f     /* a traseira só começa a voltar quando a força já caiu bastante: o drift sustenta e solta no fim */
#define TORQUE_DO_ACELERADOR      38.0f    /* acelerando, a roda de trás empurra a traseira para fora */
#define TORQUE_DO_FREIO_DE_MAO    280.0f   /* a pancada que quebra a traseira */
#define VOLTA_DO_ANGULO           5.0f     /* por segundo: o pneu tenta alinhar o carro... */
#define VOLTA_SEM_ADERENCIA       0.30f    /* ...com menos força se a traseira está solta (fração) */
#define AMORTECE_O_GIRO           3.0f     /* por segundo */
#define FRENTE_NO_DRIFT           1.00f    /* de lado, o volante vira menos o caminho: fração */
#define CAMINHO_PELO_ANGULO       0.70f    /* o caminho vai para onde o bico aponta... */
#define ANGULO_DO_CAMINHO         28.0f    /* ...até este ângulo: mais de lado que isso é só estilo, e não joga o carro para fora da pista */
#define EMPURRAO_NO_DRIFT         0.10f    /* e o quanto ele escorrega mais para fora */
#define PERDA_NO_DRIFT            300.0f   /* o carro de lado gasta velocidade, mesmo acelerando */
#define ARRASTO_DE_LADO           1500.0f  /* e mais, quanto mais de lado: multiplica o seno² do ângulo */
#define ROTACAO_NO_DRIFT          0.10f    /* a roda gira mais que o chão: o giro sobe, como fração da velocidade */
#define SEGURA_A_MARCHA           0.25f    /* de lado, a marcha fica */
#define METROS_DE_UM_JOGO_DE_PNEUS 38000.0f /* numa corrida de verdade, com as derrapagens, dura cerca de 5 voltas */
#define GASTO_DERRAPANDO          4.0f     /* derrapando ou de lado, gasta tantas vezes mais */
#define ADERENCIA_PERDIDA         0.45f    /* com o pneu acabado */
#define VELOCIDADE_PERDIDA        0.035f   /* com o pneu acabado, a máxima cai esse tanto */
#define RAIO_DA_RODA               0.35f /* metros: quanto a roda gira por metro andado */
#define GIRO_MAXIMO_DAS_RODAS     30.0f  /* radianos por segundo: mais que isso, na tela, a roda parece girar ao contrário */
#define GIRO_PATINANDO            22.0f  /* acelerando na arrancada, a roda gira mais que o carro anda */
#define VELOCIDADE_SEM_PATINAR  1600.0f  /* a partir de 40 km/h a roda não patina mais */
#define HABILIDADE_PADRAO          1.0f
#define QUEDA_DO_TRANCO            4.0f  /* 1/s: o tranco lateral de uma batida some em ~1/4 s */
#define MENOR_GIRO_PARA_RODAR    300.0f  /* graus por segundo: abaixo disso a batida só balança */
#define MAIOR_GIRO               900.0f
#define QUEDA_DO_GIRO              1.7f  /* 1/s */
#define GIRO_FINAL               220.0f  /* graus por segundo: girando menos que isso, o carro se endireita */
#define PERDA_RODANDO           5200.0f  /* velocidade perdida por segundo com o carro todo de lado */
#define RESTO_DA_RODADA            0.40f /* o carro nunca para de vez: sobra isso da velocidade que tinha */
#define TEMPO_ENTRE_RODADAS        3.0f  /* segundos sem poder rodar de novo */
#define BATIDA_NO_MURO_MINIMA      0.15f /* encostar no muro abaixo dessa fração da velocidade máxima não conta como batida */
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
    carro.anguloDasRodas = 0.0f;
    carro.direcaoLivre = 0;
    carro.desvioDoRumo = 0.0f;
    carro.temRumoNoMundo = 0;
    carro.rolagem = 0.0f;
    carro.rapidezDaRolagem = 0.0f;
    carro.arfagem = 0.0f;
    carro.rapidezDaArfagem = 0.0f;
    carro.temSuspensao = 0;
    carro.re = 0.0f;
    carro.rumoDeRe = 0.0f;
    carro.paradoFreando = 0.0f;
    carro.controladoPeloJogador = 0;
    carro.rumoNoMundo = 0.0f;
    carro.estadoDoPit = PIT_FORA;
    carro.voltaDoPit = 0;
    carro.querParar = 0;
    carro.pitFeito = 0;
    carro.vemAlguemAtras = 0;
    carro.tempoDaTroca = 0.0f;
    carro.duracaoDaTroca = 0.0f;
    carro.ultimaTroca = 0.0f;
    carro.nitro = NITRO_INICIAL;
    carro.nitroLigado = 0;
    carro.comprimento = COMPRIMENTO_PADRAO;
    carro.largura = LARGURA_PADRAO;
    carro.empurraoPendente = 0.0f;
    carro.velocidadeLateral = 0.0f;
    carro.velocidadeDeLado = 0.0f;
    carro.rodando = 0;
    carro.giro = 0.0f;
    carro.velocidadeDoGiro = 0.0f;
    carro.tempoSemRodar = 0.0f;
    carro.velocidadeNaRodada = 0.0f;
    carro.habilidade = HABILIDADE_PADRAO;
    carro.distanciaAFrente = -1.0f;
    carro.velocidadeAFrente = 0.0f;
    carro.lateralAFrente = 0.0f;
    carro.ocupadoEsquerda = 0;
    carro.ocupadoDireita = 0;
    carro.voltas = 0;
    carro.voltasDaProva = MAXIMO_DE_VOLTAS;
    carro.anguloDeFuga = 0.0f;
    carro.forcaDoDrift = 0.0f;
    carro.pancada = 0.0f;
    carro.batidaNova = 0.0f;
    carro.raspandoNoMuro = 0;
    carro.anguloDeDrift = 0.0f;
    carro.giroDoDrift = 0.0f;
    carro.gastaPneu = 0;
    carro.desgaste = 0.0f;
    for (int i = 0; i < MAXIMO_DE_VOLTAS; i++) {
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
    Comandos nenhumComando = { 0, 0, 0, 0, 0.0f, 0, 0 };
    carro.ultimosComandos = nenhumComando;
    carro.linhaAlvo = deslocamentoLateral * larguraDoLado(largada, deslocamentoLateral);

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
    if (carro->estadoDoPit != PIT_FORA) {
        return 0;   /* o pit lane é asfalto, mesmo longe da pista */
    }
    return carro->deslocamentoLateral < -carro->posicao.segmento->asfaltoEsquerda ||
           carro->deslocamentoLateral > LIMITE_PISTA;
}

/*
 * forcaDoMotor
 * Quanto a velocidade sobe por segundo com o pé no fundo. Curva de F1:
 * a força cai com o quadrado da velocidade (o ar segura o carro). Com a
 * ficha do Katchau: 0-100 km/h em ~3,5 s e 0-200 em ~7,6 s. Durante a troca
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
 * atualizarNitro
 * Liga e desliga o nitro, gasta enquanto queima e recarrega quando não está
 * queimando: devagar sempre, e bem mais rápido freando com o carro andando.
 * Frear desliga o nitro.
 * Recebe: o carro, os comandos e o tempo do quadro.
 * Devolve: nada (altera nitro e nitroLigado).
 */
static void atualizarNitro(Carro *carro, Comandos comandos, float tempoQuadro) {
    int quer = comandos.nitro && !comandos.frear && !comandos.freioDeMao;
    if (carro->nitroLigado && (!quer || carro->nitro <= 0.0f)) {
        carro->nitroLigado = 0;
    } else if (!carro->nitroLigado && quer && carro->nitro >= MINIMO_PARA_LIGAR) {
        carro->nitroLigado = 1;
    }

    if (carro->nitroLigado) {
        carro->nitro -= GASTO_DO_NITRO * tempoQuadro;
    } else if (comandos.frear && carro->velocidade > 0.0f) {
        carro->nitro += RECARGA_FREANDO * tempoQuadro;
    } else {
        carro->nitro += RECARGA_DO_NITRO * tempoQuadro;
    }
    carro->nitro = limitar(carro->nitro, 0.0f, 1.0f);
}

/*
 * atualizarVelocidade
 * Aplica acelerador, freio, ar e grama à velocidade.
 * Recebe: o carro, os comandos e o tempo do quadro em segundos.
 * Devolve: nada (altera carro->velocidade).
 */
static void atualizarVelocidade(Carro *carro, Comandos comandos, float tempoQuadro) {
    float anterior = carro->velocidade;
    float maxima = velocidadeMaximaAgora(carro);

    if (comandos.freioDeMao) {
        carro->velocidade -= (FREIO_DE_MAO + perdaSemAcelerar(carro)) * tempoQuadro;
    } else if (comandos.acelerar || carro->nitroLigado) {
        carro->velocidade += forcaDoMotor(carro->velocidade, maxima, carro->aceleracao, &carro->motor) * tempoQuadro;
        if (carro->nitroLigado) {
            carro->velocidade += carro->aceleracao * FORCA_DO_NITRO * tempoQuadro;
        }
    } else if (comandos.frear) {
        carro->velocidade -= FORCA_FREIO * tempoQuadro;
    } else {
        carro->velocidade -= perdaSemAcelerar(carro) * tempoQuadro;
    }

    float senoDoAngulo = sinf(carro->anguloDeDrift * 0.0174533f);
    carro->velocidade -= (PERDA_NO_DRIFT * carro->forcaDoDrift + ARRASTO_DE_LADO * senoDoAngulo * senoDoAngulo) * tempoQuadro;

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
 * aderenciaAtual
 * O pneu gasto segura menos: a perda cresce devagar no começo e rápido no fim.
 * Recebe: o carro.
 * Devolve: a aderência de agora, de 0 a 1.
 */
float aderenciaAtual(const Carro *carro) {
    return carro->aderencia * (1.0f - ADERENCIA_PERDIDA * carro->desgaste * carro->desgaste);
}

/*
 * atualizarDrift
 * O freio de mão, com o volante virado, quebra a traseira. Depois disso quem manda é o
 * pé: acelerando de lado o drift continua, sem acelerar ele acaba, e o volante controla
 * o ângulo (virar para o outro lado é o contraesterço).
 * Recebe: o carro, os comandos e o tempo do quadro.
 * Devolve: nada.
 */
static void atualizarDrift(Carro *carro, Comandos comandos, float tempoQuadro) {
    int rapidoOBastante = carro->velocidade > VELOCIDADE_PARA_O_DRIFT;
    int quebrouATraseira = comandos.freioDeMao && rapidoOBastante && fabsf(carro->volante) > VOLANTE_PARA_O_DRIFT;
    int aceleraDeLado = comandos.acelerar && !comandos.frear && rapidoOBastante
                        && fabsf(carro->anguloDeDrift) > ANGULO_DE_MANTER;

    if (quebrouATraseira) {
        carro->forcaDoDrift = fminf(carro->forcaDoDrift + SOLTANDO_A_TRASEIRA * tempoQuadro, 1.0f);
    } else {
        float volta = aceleraDeLado ? MANTENDO_O_DRIFT : VOLTANDO_DO_DRIFT;
        carro->forcaDoDrift = fmaxf(carro->forcaDoDrift - volta * tempoQuadro, 0.0f);
    }

    /* O ângulo tem inércia: o volante gira o carro (e o contraesterço segura), o acelerador
     * empurra a traseira, e o pneu tenta alinhar, com menos força quanto mais solta ela está */
    float forca = fminf(carro->forcaDoDrift * FOLGA_DA_TRASEIRA, 1.0f);
    float lado = (carro->anguloDeDrift > 0.0f) ? 1.0f : -1.0f;
    float torque = carro->volante * TORQUE_DO_VOLANTE * forca;
    if (carro->volante * carro->anguloDeDrift < 0.0f) {
        torque *= FORCA_DO_CONTRAESTERCO;   /* corrigir segura mais do que virar gira */
    }
    if (quebrouATraseira) {
        torque += ((carro->volante > 0.0f) ? 1.0f : -1.0f) * TORQUE_DO_FREIO_DE_MAO;
    }
    if (aceleraDeLado) {
        torque += lado * TORQUE_DO_ACELERADOR * forca;
    }
    float alinhar = VOLTA_DO_ANGULO * (1.0f - (1.0f - VOLTA_SEM_ADERENCIA) * forca);
    float aceleracaoDoGiro = torque - carro->anguloDeDrift * alinhar * alinhar - carro->giroDoDrift * AMORTECE_O_GIRO;
    carro->giroDoDrift += aceleracaoDoGiro * tempoQuadro;
    carro->anguloDeDrift += carro->giroDoDrift * tempoQuadro;
    if (fabsf(carro->anguloDeDrift) > MAIOR_ANGULO_DO_DRIFT) {
        carro->anguloDeDrift = limitar(carro->anguloDeDrift, -MAIOR_ANGULO_DO_DRIFT, MAIOR_ANGULO_DO_DRIFT);
        carro->giroDoDrift = 0.0f;
    }
}

/*
 * atualizarDesgaste
 * Os pneus gastam com o caminho, e bem mais derrapando ou de lado.
 * Recebe: o carro e quantas unidades ele andou.
 * Devolve: nada.
 */
static void atualizarDesgaste(Carro *carro, float distancia) {
    if (!carro->gastaPneu || distancia <= 0.0f) {
        return;
    }
    float metros = distancia / UNIDADES_POR_METRO * (1.0f + GASTO_DERRAPANDO * carro->derrapagem);
    carro->desgaste = fminf(carro->desgaste + metros / METROS_DE_UM_JOGO_DE_PNEUS, 1.0f);
}

/*
 * volanteDaFolgaDaCurva
 * Quanto de volante cancela o empurrão extra das curvas leves (veja atualizarLateral).
 * A IA soma isso ao volante dela: a regra existe para o piloto ter que virar, e não
 * pode tirar os rivais da linha.
 * Recebe: o carro.
 * Devolve: de -1 a +1.
 */
float volanteDaFolgaDaCurva(const Carro *carro) {
    float proporcao = carro->velocidade / carro->velocidadeMaxima;
    float direcao = VELOCIDADE_DE_VIRADA * respostaDaDirecao(proporcao);
    if (direcao <= 0.0f) {
        return 0.0f;
    }
    float pe = EMPURRAO_SEM_PEDAL;
    if (carro->ultimosComandos.acelerar || carro->nitroLigado) {
        pe = 1.0f;
    } else if (carro->ultimosComandos.frear) {
        pe = EMPURRAO_FREANDO;
    }
    float empurrao = carro->posicao.segmento->curvatura * FORCA_CENTRIFUGA * proporcao * proporcao
                     * (1.0f - aderenciaAtual(carro)) * pe;
    float uso = fminf(fabsf(empurrao) / direcao, 1.0f);
    return limitar(empurrao * (1.0f - uso) / direcao, -1.0f, 1.0f);
}

/*
 * encostarNoLimite
 * O carro encostou no muro ou no fim da área de escape: perde velocidade. O limite não
 * endireita o carro, senão ele sairia andando junto dele, como num trilho: com o bico
 * apontado para fora ele fica ali, e é o volante que tira o bico de lá.
 * Recebe: o carro, o lado do limite (+1 direita, -1 esquerda) e o tempo do quadro.
 * Devolve: nada.
 */
static void encostarNoLimite(Carro *carro, float lado, float tempoQuadro) {
    carro->velocidade = fmaxf(carro->velocidade - PERDA_NO_MURO * tempoQuadro, 0.0f);
    carro->raspandoNoMuro = 1;
    if (carro->direcaoLivre && carro->desvioDoRumo * lado > 0.0f && carro->volante * lado < -VOLANTE_PARA_SAIR_DO_LIMITE) {
        float gira = fminf(GIRO_SAINDO_DO_LIMITE * tempoQuadro, fabsf(carro->desvioDoRumo));
        carro->rumoNoMundo -= lado * gira;
        carro->desvioDoRumo -= lado * gira;
    }
}

/*
 * aplicarLimites
 * O que o carro não atravessa: o fim da grama, os muros e o que a pista mediu em cada lado.
 * Recebe: o carro e o tempo do quadro.
 * Devolve: nada.
 */
static void aplicarLimites(Carro *carro, float tempoQuadro) {
    Segmento *aqui = carro->posicao.segmento;
    carro->deslocamentoLateral = limitar(carro->deslocamentoLateral, -LIMITE_LATERAL, LIMITE_LATERAL);

    int jaRaspava = carro->raspandoNoMuro;
    carro->raspandoNoMuro = 0;

    /* Muro (só no oval): o carro não passa dele, e raspar custa velocidade.
     * A largura de cada lado muda ao longo da pista, então a conta é em metros */
    if (aqui->temMuro) {
        float limiteDireita = aqui->lugarDoMuro - FOLGA_DO_MURO / aqui->larguraDireita;
        float limiteEsquerda = aqui->lugarDoMuro - FOLGA_DO_MURO / aqui->larguraEsquerda;
        if (carro->deslocamentoLateral > limiteDireita || carro->deslocamentoLateral < -limiteEsquerda) {
            carro->deslocamentoLateral = limitar(carro->deslocamentoLateral, -limiteEsquerda, limiteDireita);
            encostarNoLimite(carro, (carro->deslocamentoLateral > 0.0f) ? 1.0f : -1.0f, tempoQuadro);
        }
    }

    /* Onde a pista mediu o que para o carro em cada lado, o limite é isso, em metros */
    if (aqui->limiteDireita > 0.0f) {
        float emMetros = carro->deslocamentoLateral * larguraDoLado(aqui, carro->deslocamentoLateral);
        float limitado = limitar(emMetros, -(aqui->limiteEsquerda - FOLGA_DO_MURO), aqui->limiteDireita - FOLGA_DO_MURO);
        /* A folga é para o arredondamento da conta não contar como batida, quadro após quadro */
        if (fabsf(limitado - emMetros) > FOLGA_DA_CONTA_NO_LIMITE) {
            carro->deslocamentoLateral = limitado / larguraDoLado(aqui, limitado);
            encostarNoLimite(carro, (limitado > 0.0f) ? 1.0f : -1.0f, tempoQuadro);
        }
    }

    /* Encostar no muro, vindo de fora dele e com velocidade, é uma batida (raspar depois não é) */
    if (carro->raspandoNoMuro && !jaRaspava) {
        float proporcao = carro->velocidade / carro->velocidadeMaxima;
        if (proporcao > BATIDA_NO_MURO_MINIMA) {
            carro->batidaNova = fmaxf(carro->batidaNova, fminf(0.3f + proporcao, 1.0f));
        }
    }
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
    float direcao = VELOCIDADE_DE_VIRADA * respostaDaDirecao(proporcaoVelocidade) * (1.0f - (1.0f - FRENTE_NO_DRIFT) * carro->forcaDoDrift);

    /* Volante: o quanto vira depende de quanto o volante está girado */
    Segmento *aqui = carro->posicao.segmento;
    float paraMetros = MEIA_LARGURA_PADRAO / larguraDoLado(aqui, carro->deslocamentoLateral);
    carro->deslocamentoLateral += direcao * carro->volante * paraMetros * tempoQuadro;

    /* De lado, o carro vai para onde o bico aponta: é o que faz o drift entrar na curva */
    float anguloDoCaminho = limitar(carro->anguloDeDrift, -ANGULO_DO_CAMINHO, ANGULO_DO_CAMINHO);
    float paraOndeAponta = sinf(anguloDoCaminho * 0.0174533f) * carro->velocidade / UNIDADES_POR_METRO
                           * CAMINHO_PELO_ANGULO * carro->forcaDoDrift;
    carro->deslocamentoLateral += paraOndeAponta / larguraDoLado(aqui, carro->deslocamentoLateral) * tempoQuadro;

    /* Força centrífuga: curva para a direita (curvatura > 0) empurra
     * para a esquerda, e vice-versa. Mais aderência, menos empurrão.
     * Acelerando o carro sai de frente; freando ele crava a dianteira. */
    float pe = EMPURRAO_SEM_PEDAL;
    if (comandos.acelerar || carro->nitroLigado) {
        pe = 1.0f;
    } else if (comandos.frear) {
        pe = EMPURRAO_FREANDO;
    }
    float curvatura = carro->posicao.segmento->curvatura;
    float empurrao = curvatura * FORCA_CENTRIFUGA * proporcaoVelocidade * proporcaoVelocidade
                     * (1.0f - aderenciaAtual(carro)) * pe * (1.0f + EMPURRAO_NO_DRIFT * carro->forcaDoDrift);

    /* Curva leve também pede volante: quanto mais folga o pneu tem, mais o carro segue reto
     * se ninguém virar (até o dobro). No limite não muda nada: a velocidade segura é a mesma. */
    float usoDoPneu = (direcao > 0.0f) ? fminf(fabsf(empurrao) / direcao, 1.0f) : 1.0f;
    float empurraoReal = empurrao * (2.0f - usoDoPneu);
    carro->deslocamentoLateral -= empurraoReal * paraMetros * tempoQuadro;

    /* O bico só acompanha a curva se o piloto estiver virando: o que falta de volante vira
     * um ângulo para fora (só no desenho e na câmera; quem manda no caminho é a conta acima) */
    float virandoParaDentro = (empurraoReal > 0.0f) ? direcao * carro->volante : -direcao * carro->volante;
    float semVolante = fmaxf(fabsf(empurraoReal) - fmaxf(virandoParaDentro, 0.0f), 0.0f);
    float fugaEmMetros = semVolante * MEIA_LARGURA_PADRAO;
    float rapidezEmMetros = fmaxf(carro->velocidade / UNIDADES_POR_METRO, METROS_POR_SEGUNDO_MINIMOS);
    float alvoDaFuga = atan2f(fugaEmMetros, rapidezEmMetros) * 57.2958f;
    alvoDaFuga = fminf(alvoDaFuga, MAIOR_ANGULO_DE_FUGA) * ((empurraoReal > 0.0f) ? -1.0f : 1.0f);
    if (carro->velocidade <= 0.0f) {
        alvoDaFuga = 0.0f;
    }
    carro->anguloDeFuga += (alvoDaFuga - carro->anguloDeFuga) * fminf(RAPIDEZ_DA_FUGA * tempoQuadro, 1.0f);

    /* O tranco de uma batida de lado, que some logo */
    carro->deslocamentoLateral += carro->velocidadeLateral / larguraDoLado(aqui, carro->deslocamentoLateral) * tempoQuadro;
    carro->velocidadeLateral *= expf(-QUEDA_DO_TRANCO * tempoQuadro);

    /* Derrapagem: 0 enquanto o empurrão gasta menos de 70% da direção,
     * 1 quando gasta tudo (dali em diante o carro vai para fora) */
    float uso = 0.0f;
    if (direcao > 0.0f) {
        uso = (empurrao < 0 ? -empurrao : empurrao) / direcao;
    }
    carro->derrapagem = limitar((uso - INICIO_DA_DERRAPAGEM) / (1.0f - INICIO_DA_DERRAPAGEM), 0.0f, 1.0f);
    carro->derrapagem = fmaxf(carro->derrapagem, carro->forcaDoDrift);   /* de lado, sai fumaça */

    /* Passou do limite: o pneu arrasta. Com menos velocidade o empurrão
     * cai (com o quadrado), e o carro volta a obedecer o volante */
    float excesso = limitar(uso - 1.0f, 0.0f, EXCESSO_MAXIMO) * (1.0f - carro->forcaDoDrift);   /* de lado, quem gasta é o drift */
    carro->velocidade -= PERDA_NA_DERRAPAGEM * excesso * tempoQuadro;
    if (carro->velocidade < 0.0f) {
        carro->velocidade = 0.0f;
    }

    aplicarLimites(carro, tempoQuadro);
}

/*
 * rapidezDasRodas
 * Com que rapidez a roda gira na tela. Em baixa velocidade é o giro de verdade;
 * depois ele vai chegando aos poucos ao máximo que a tela mostra sem parecer
 * que a roda anda para trás. Na arrancada, acelerando, ela patina.
 * Recebe: a velocidade e se o piloto está acelerando.
 * Devolve: radianos por segundo.
 */
float rapidezDasRodas(float velocidade, int acelerando) {
    float deVerdade = velocidade / UNIDADES_POR_METRO / RAIO_DA_RODA;
    float rapidez = GIRO_MAXIMO_DAS_RODAS * (1.0f - expf(-deVerdade / GIRO_MAXIMO_DAS_RODAS));
    if (acelerando && velocidade < VELOCIDADE_SEM_PATINAR) {
        rapidez = fmaxf(rapidez, GIRO_PATINANDO * (1.0f - velocidade / VELOCIDADE_SEM_PATINAR));
    }
    return rapidez;
}

/*
 * fazerRodar
 * Uma batida fora do centro faz o carro rodar. Batida fraca não roda, e quem
 * acabou de rodar fica um tempo sem poder rodar de novo.
 * Recebe: o carro e o impulso em graus por segundo (+ gira para a direita).
 * Devolve: nada.
 */
void fazerRodar(Carro *carro, float impulso) {
    if (carro->rodando || carro->tempoSemRodar > 0.0f || fabsf(impulso) < MENOR_GIRO_PARA_RODAR) {
        return;
    }
    carro->rodando = 1;
    carro->nitroLigado = 0;
    carro->velocidadeNaRodada = carro->velocidade;
    carro->velocidadeDoGiro = limitar(impulso, -MAIOR_GIRO, MAIOR_GIRO);
}

/*
 * atualizarRodada
 * Enquanto roda, o carro gira, o giro vai perdendo força e, de lado, ele perde
 * velocidade. No fim ele se endireita sozinho e o piloto volta a mandar.
 * Recebe: o carro e o tempo do quadro.
 * Devolve: nada.
 */
static void atualizarRodada(Carro *carro, float tempoQuadro) {
    carro->tempoSemRodar = fmaxf(carro->tempoSemRodar - tempoQuadro, 0.0f);
    if (!carro->rodando) {
        return;
    }
    if (fabsf(carro->velocidadeDoGiro) > GIRO_FINAL) {
        carro->giro += carro->velocidadeDoGiro * tempoQuadro;
        carro->velocidadeDoGiro *= expf(-QUEDA_DO_GIRO * tempoQuadro);
    } else {
        /* Termina a volta no sentido em que já estava girando */
        float sentido = (carro->velocidadeDoGiro < 0.0f) ? -1.0f : 1.0f;
        float alvo = sentido * ceilf(fabsf(carro->giro) / 360.0f) * 360.0f;
        carro->giro += sentido * GIRO_FINAL * tempoQuadro;
        if (sentido * (carro->giro - alvo) >= 0.0f) {
            carro->giro = 0.0f;
            carro->velocidadeDoGiro = 0.0f;
            carro->rodando = 0;
            carro->tempoSemRodar = TEMPO_ENTRE_RODADAS;
            return;
        }
    }
    float deLado = fabsf(sinf(carro->giro * 0.0174533f));
    float menor = carro->velocidadeNaRodada * RESTO_DA_RODADA;
    carro->velocidade = fmaxf(carro->velocidade - PERDA_RODANDO * deLado * tempoQuadro, fminf(menor, carro->velocidade));
}

/*
 * meiasMedidasDoCarro
 * A caixa de colisão do carro, alinhada com a pista: ela envolve o carro
 * inteiro, mesmo virado pelo volante ou rodando.
 * Recebe: o carro e onde guardar a metade do comprimento e da largura, em metros.
 * Devolve: nada.
 */
void meiasMedidasDoCarro(const Carro *carro, float *meioComprimento, float *meiaLargura) {
    float angulo = (carro->giro + carro->anguloDeDrift + carro->volante * GRAUS_DO_VOLANTE) * 0.0174533f;
    float seno = fabsf(sinf(angulo)), cosseno = fabsf(cosf(angulo));
    *meioComprimento = (carro->comprimento * cosseno + carro->largura * seno) / 2.0f;
    *meiaLargura = (carro->comprimento * seno + carro->largura * cosseno) / 2.0f;
}

/*
 * aceleracaoDeCurva
 * Recebe: o carro.
 * Devolve: o quanto o pneu segura na curva, sem a direção guiada, em metros por segundo ao quadrado.
 */
float aceleracaoDeCurva(const Carro *carro) {
    return ACELERACAO_DE_CURVA_BASE + ACELERACAO_DE_CURVA_NO_PNEU * aderenciaAtual(carro);
}

/*
 * erroAoLongo
 * Recebe: um segmento, o andamento dentro dele (0 a 1) e um ponto do mundo.
 * Devolve: quantos metros o ponto está à frente desse lugar da linha, medidos no rumo dela.
 */
static float erroAoLongo(const Segmento *a, float t, float x, float z) {
    const Segmento *b = a->proximo;
    float mudou = b->rumo - a->rumo;
    mudou -= 360.0f * roundf(mudou / 360.0f);
    float rumo = (a->rumo + mudou * t) * 0.0174533f;
    float cx = a->xMundo + (b->xMundo - a->xMundo) * t, cz = a->zMundo + (b->zMundo - a->zMundo) * t;
    return (x - cx) * sinf(rumo) - (z - cz) * cosf(rumo);
}

/*
 * andarNoMundo
 * Sem a direção guiada, a pista não leva o carro: ele tem lugar e rumo próprios no mundo, e o
 * volante é a única coisa que muda o rumo. Sem volante, o caminho é uma reta. Depois de andar,
 * a pista só responde onde ele está: em que segmento e a quantos metros da linha.
 * O volante vira cada vez menos com a velocidade: a aderência limita o quanto o carro curva.
 * Recebe: o carro e o tempo do quadro.
 * Devolve: quantas vezes ele cruzou a linha de chegada.
 */
static int andarNoMundo(Carro *carro, float tempoQuadro) {
    PontoNoMundo aqui = pontoNoMundo(carro->posicao, carro->deslocamentoLateral);
    if (!carro->temRumoNoMundo) {
        carro->rumoNoMundo = aqui.rumo;
        carro->temRumoNoMundo = 1;
    }

    /* O volante muda o rumo, até onde o pneu segura */
    float metrosPorSegundo = carro->velocidade / UNIDADES_POR_METRO;
    float agarra = aceleracaoDeCurva(carro) * (1.0f + AGARRE_NO_DRIFT * carro->forcaDoDrift);
    /* Volante todo é o máximo que dá nessa velocidade: devagar, o raio mínimo; rápido, o que o pneu segura */
    float maximo = fminf(metrosPorSegundo / RAIO_MINIMO_DA_DIRECAO, agarra / fmaxf(metrosPorSegundo, 1.0f));
    carro->rumoNoMundo += carro->volante * maximo * 57.2958f * tempoQuadro;

    /* Levando o pneu ao limite, ele canta e o carro perde um pouco de velocidade */
    float uso = fabsf(carro->volante) * maximo * metrosPorSegundo / agarra;
    carro->derrapagem = fmaxf(limitar((uso - INICIO_DA_DERRAPAGEM) / (1.0f - INICIO_DA_DERRAPAGEM), 0.0f, 1.0f), carro->forcaDoDrift);
    float gastaNaCurva = PERDA_NO_LIMITE_DO_PNEU * limitar((uso - INICIO_DA_DERRAPAGEM) / (1.0f - INICIO_DA_DERRAPAGEM), 0.0f, 1.0f);
    carro->velocidade = fmaxf(carro->velocidade - gastaNaCurva * (1.0f - carro->forcaDoDrift) * tempoQuadro, 0.0f);

    /* Anda para onde aponta. De lado, o drift e as batidas somam o seu pedaço */
    float rumo = carro->rumoNoMundo * 0.0174533f;
    float frenteX = sinf(rumo), frenteZ = -cosf(rumo), direitaX = cosf(rumo), direitaZ = sinf(rumo);
    float anguloDoCaminho = limitar(carro->anguloDeDrift, -ANGULO_DO_CAMINHO, ANGULO_DO_CAMINHO);
    float deLado = sinf(anguloDoCaminho * 0.0174533f) * metrosPorSegundo * CAMINHO_PELO_ANGULO * carro->forcaDoDrift + carro->velocidadeLateral;
    carro->velocidadeLateral *= expf(-QUEDA_DO_TRANCO * tempoQuadro);
    float adiante = carro->velocidade * tempoQuadro / UNIDADES_POR_METRO + carro->empurraoPendente / UNIDADES_POR_METRO;
    carro->empurraoPendente = 0.0f;
    float x = aqui.x + frenteX * adiante + direitaX * deLado * tempoQuadro;
    float z = aqui.z + frenteZ * adiante + direitaZ * deLado * tempoQuadro;

    /* Agora a pista diz onde ele está */
    Segmento *antes = carro->posicao.segmento;
    Segmento *novo = antes;
    for (int i = 0; i < PASSOS_PROCURANDO_NA_LINHA; i++) {
        if (erroAoLongo(novo, 0.0f, x, z) < 0.0f) {
            novo = novo->anterior;
        } else if (erroAoLongo(novo, 1.0f, x, z) >= 0.0f) {
            novo = novo->proximo;
        } else {
            break;
        }
    }
    float t0 = 0.0f, t1 = 1.0f;
    for (int i = 0; i < 10; i++) {
        float meio = (t0 + t1) / 2.0f;
        if (erroAoLongo(novo, meio, x, z) >= 0.0f) {
            t0 = meio;
        } else {
            t1 = meio;
        }
    }
    float t = (t0 + t1) / 2.0f;

    /* Da linha de chegada para trás ele não passa, para a volta não contar duas vezes */
    int total = antes->indice - novo->indice;
    int voltou = antes->indice < novo->indice && novo->indice - antes->indice > PASSOS_PROCURANDO_NA_LINHA * 2;
    if (voltou) {
        novo = antes;
        t = 0.0f;
    }
    int cruzou = total > PASSOS_PROCURANDO_NA_LINHA * 2;

    /* O quanto andou conta na linha, como para todo mundo */
    float antesNaLinha = antes->indice * COMPRIMENTO_SEGMENTO + carro->posicao.distanciaNoSegmento;
    float agoraNaLinha = novo->indice * COMPRIMENTO_SEGMENTO + t * COMPRIMENTO_SEGMENTO;
    float andou = agoraNaLinha - antesNaLinha;
    if (cruzou) {
        andou += (antes->indice + 1) * COMPRIMENTO_SEGMENTO;
    }
    carro->distanciaPercorrida += andou;
    atualizarDesgaste(carro, fabsf(andou));
    carro->posicao.segmento = novo;
    carro->posicao.distanciaNoSegmento = t * COMPRIMENTO_SEGMENTO;

    const Segmento *seguinte = novo->proximo;
    float mudou = seguinte->rumo - novo->rumo;
    mudou -= 360.0f * roundf(mudou / 360.0f);
    float rumoDaLinha = novo->rumo + mudou * t;
    float angulo = rumoDaLinha * 0.0174533f;
    float cx = novo->xMundo + (seguinte->xMundo - novo->xMundo) * t, cz = novo->zMundo + (seguinte->zMundo - novo->zMundo) * t;
    float lateral = (x - cx) * cosf(angulo) + (z - cz) * sinf(angulo);
    float largura = larguraDoLado(novo, lateral) + (larguraDoLado((Segmento *) seguinte, lateral) - larguraDoLado(novo, lateral)) * t;
    carro->deslocamentoLateral = lateral / largura;
    float diferenca = carro->rumoNoMundo - rumoDaLinha;
    carro->desvioDoRumo = diferenca - 360.0f * roundf(diferenca / 360.0f);
    carro->anguloDeFuga = 0.0f;

    aplicarLimites(carro, tempoQuadro);
    return cruzou ? 1 : 0;
}

/*
 * atualizarSuspensao
 * A suspensão, só para o desenho: a aceleração de lado (rapidez de giro vezes velocidade)
 * pede rolagem para fora da curva, e a de frente pede arfagem (freando, o bico desce).
 * Cada uma é uma mola com amortecedor: o corpo chega lá com um pequeno balanço.
 * Recebe: o carro e o tempo do quadro.
 * Devolve: nada.
 */
void atualizarSuspensao(Carro *carro, float tempoQuadro) {
    if (tempoQuadro <= 0.0f) {
        return;
    }
    PontoNoMundo aqui = pontoNoMundo(carro->posicao, carro->deslocamentoLateral);
    float rumo = (carro->direcaoLivre && carro->temRumoNoMundo) ? carro->rumoNoMundo
                                                                : aqui.rumo + carro->anguloDeFuga + carro->anguloDeDrift;
    if (!carro->temSuspensao) {
        carro->rumoAnterior = rumo;
        carro->velocidadeAnterior = carro->velocidade;
        carro->temSuspensao = 1;
        return;
    }
    float giro = rumo - carro->rumoAnterior;
    giro -= 360.0f * roundf(giro / 360.0f);
    float metrosPorSegundo = carro->velocidade / UNIDADES_POR_METRO;
    float deLado = metrosPorSegundo * giro * 0.0174533f / tempoQuadro;   /* para a direita, positivo */
    float deFrente = (carro->velocidade - carro->velocidadeAnterior) / UNIDADES_POR_METRO / tempoQuadro;
    carro->rumoAnterior = rumo;
    carro->velocidadeAnterior = carro->velocidade;

    /* Curvas leves já mexem bem o corpo, e as fortes saturam suave no máximo (tanh) */
    float alvoDaRolagem = MAIOR_ROLAGEM * tanhf(deLado / GRAVIDADE * ROLAGEM_POR_G / MAIOR_ROLAGEM);
    alvoDaRolagem += ROLAGEM_DO_VOLANTE * carro->volante * fminf(metrosPorSegundo / VELOCIDADE_DO_VOLANTE, 1.0f);
    alvoDaRolagem = limitar(alvoDaRolagem, -MAIOR_ROLAGEM, MAIOR_ROLAGEM);
    float alvoDaArfagem = MAIOR_ARFAGEM * tanhf(deFrente / GRAVIDADE * ARFAGEM_POR_G / MAIOR_ARFAGEM);
    float passo = fminf(tempoQuadro, 1.0f / 30.0f);
    carro->rapidezDaRolagem += (MOLA_DA_SUSPENSAO * (alvoDaRolagem - carro->rolagem) - AMORTECEDOR * carro->rapidezDaRolagem) * passo;
    carro->rolagem += carro->rapidezDaRolagem * passo;
    carro->rapidezDaArfagem += (MOLA_DA_SUSPENSAO * (alvoDaArfagem - carro->arfagem) - AMORTECEDOR * carro->rapidezDaArfagem) * passo;
    carro->arfagem += carro->rapidezDaArfagem * passo;
    carro->rolagem = limitar(carro->rolagem, -MAIOR_ROLAGEM, MAIOR_ROLAGEM);
    carro->arfagem = limitar(carro->arfagem, -MAIOR_ARFAGEM, MAIOR_ARFAGEM);
}

/*
 * controlarRe
 * A ré do jogador: parado, segurando o freio por um instante, engata; aí o freio acelera
 * para trás e o acelerador freia. Parou a ré e soltou o freio, volta a andar para a frente.
 * Recebe: o carro, os comandos e o tempo do quadro.
 * Devolve: 1 se o carro está de ré neste quadro.
 */
static int controlarRe(Carro *carro, Comandos comandos, float tempoQuadro) {
    if (!carro->controladoPeloJogador || carro->rodando) {
        carro->re = 0.0f;
        carro->paradoFreando = 0.0f;
        return 0;
    }
    if (carro->re <= 0.0f) {
        int parado = carro->velocidade <= 1.0f;
        carro->paradoFreando = (parado && comandos.frear && !comandos.acelerar) ? carro->paradoFreando + tempoQuadro : 0.0f;
        if (carro->paradoFreando < SEGUNDOS_PARA_ENGATAR_A_RE) {
            return 0;
        }
    }
    if (comandos.acelerar) {
        carro->re -= FREIO_DA_RE * tempoQuadro;
    } else if (comandos.frear) {
        carro->re += ACELERACAO_DE_RE * tempoQuadro;
    } else {
        carro->re -= ATRITO_DA_RE * tempoQuadro;
    }
    carro->re = limitar(carro->re, 0.0f, MAXIMA_DE_RE);
    carro->velocidade = 0.0f;
    if (carro->re <= 0.0f && !comandos.frear) {
        carro->paradoFreando = 0.0f;   /* desengatou */
        return 0;
    }
    return 1;
}

/*
 * andarDeRe
 * Anda para trás. Na direção livre, o bico gira para o lado contrário do volante (como de
 * ré num carro de verdade); na guiada, o carro vai de lado para onde o volante manda.
 * Recebe: o carro, os comandos e o tempo do quadro.
 * Devolve: quantas voltas ganhou (de ré, nunca atravessa a linha de chegada).
 */
static int andarDeRe(Carro *carro, Comandos comandos, float tempoQuadro) {
    carro->ultimosComandos = comandos;
    carro->nitroLigado = 0;
    carro->forcaDoDrift = 0.0f;
    carro->anguloDeDrift = 0.0f;
    carro->derrapagem = 0.0f;
    atualizarMotor(&carro->motor, carro->re / carro->velocidadeMaxima, comandos.frear, 0, 0, tempoQuadro);
    atualizarVolante(carro, comandos, tempoQuadro);
    float metrosPorSegundo = carro->re / UNIDADES_POR_METRO;
    float lateralAntes = carro->deslocamentoLateral * larguraDoLado(carro->posicao.segmento, carro->deslocamentoLateral);
    int voltas = 0;
    if (carro->direcaoLivre) {
        /* andarNoMundo com a velocidade negativa: vai para trás e o rumo gira ao contrário */
        carro->velocidade = -carro->re;
        if (carro->posicao.segmento->ehLinhaDeChegada && carro->posicao.distanciaNoSegmento < carro->re * tempoQuadro) {
            carro->velocidade = 0.0f;
        }
        voltas = andarNoMundo(carro, tempoQuadro);
        carro->velocidade = 0.0f;
    } else {
        /* O bico gira ao contrário do volante, e o carro anda para trás para onde a traseira aponta */
        Segmento *aqui = carro->posicao.segmento;
        carro->rumoDeRe -= carro->volante * metrosPorSegundo / RAIO_DA_RE * 57.2958f * tempoQuadro;
        carro->rumoDeRe = limitar(carro->rumoDeRe, -MAIOR_RUMO_DE_RE, MAIOR_RUMO_DE_RE);
        float rumo = carro->rumoDeRe * 0.0174533f;
        float deLado = -metrosPorSegundo * sinf(rumo);
        carro->deslocamentoLateral += deLado / larguraDoLado(aqui, carro->deslocamentoLateral) * tempoQuadro;
        float distancia = -carro->re * cosf(rumo) * tempoQuadro;
        if (aqui->ehLinhaDeChegada && carro->posicao.distanciaNoSegmento + distancia < 0.0f) {
            distancia = -carro->posicao.distanciaNoSegmento;
        }
        carro->distanciaPercorrida += distancia;
        Segmento *segmentoAntes = carro->posicao.segmento;
        voltas = moverNaPista(&carro->posicao, distancia);
        if (carro->posicao.segmento != segmentoAntes) {
            carro->deslocamentoLateral *= larguraDoLado(segmentoAntes, carro->deslocamentoLateral)
                                          / larguraDoLado(carro->posicao.segmento, carro->deslocamentoLateral);
        }
        aplicarLimites(carro, tempoQuadro);
    }
    carro->anguloDasRodas = fmodf(carro->anguloDasRodas - rapidezDasRodas(carro->re, 0) * tempoQuadro + 2.0f * 3.14159265f,
                                  2.0f * 3.14159265f);
    float lateralDepois = carro->deslocamentoLateral * larguraDoLado(carro->posicao.segmento, carro->deslocamentoLateral);
    carro->velocidadeDeLado = (tempoQuadro > 0.0f) ? (lateralDepois - lateralAntes) / tempoQuadro : 0.0f;
    carro->voltas += voltas;
    atualizarSuspensao(carro, tempoQuadro);
    return voltas;
}

/*
 * atualizarCarro
 * Avança o carro um quadro: motor (marcha), velocidade, volante, lateral e depois anda
 * pela lista da pista com moverNaPista().
 * Recebe: o carro, os comandos e o tempo do quadro em segundos.
 * Devolve: quantas voltas o carro completou neste quadro (0 ou 1).
 */
int atualizarCarro(Carro *carro, Comandos comandos, float tempoQuadro) {
    if (carro->rodando) {
        Comandos nenhumComando = { 0, 0, 0, 0, 0.0f, 0, 0 };
        comandos = nenhumComando;
    }
    atualizarRodada(carro, tempoQuadro);
    if (controlarRe(carro, comandos, tempoQuadro)) {
        return andarDeRe(carro, comandos, tempoQuadro);
    }
    carro->rumoDeRe *= expf(-tempoQuadro / ALINHANDO_DEPOIS_DA_RE);
    float lateralAntes = carro->deslocamentoLateral * larguraDoLado(carro->posicao.segmento, carro->deslocamentoLateral);

    carro->ultimosComandos = comandos;
    atualizarNitro(carro, comandos, tempoQuadro);
    atualizarMotor(&carro->motor, carro->velocidade / carro->velocidadeMaxima + ROTACAO_NO_DRIFT * carro->forcaDoDrift,
                   (comandos.acelerar || carro->nitroLigado) && !comandos.frear && !comandos.freioDeMao,
                   comandos.frear, carro->forcaDoDrift > SEGURA_A_MARCHA, tempoQuadro);
    atualizarVelocidade(carro, comandos, tempoQuadro);
    atualizarVolante(carro, comandos, tempoQuadro);
    atualizarDrift(carro, comandos, tempoQuadro);
    if (carro->direcaoLivre) {
        float lateralAntesLivre = carro->deslocamentoLateral * larguraDoLado(carro->posicao.segmento, carro->deslocamentoLateral);
        int cruzouLivre = andarNoMundo(carro, tempoQuadro);
        carro->anguloDasRodas = fmodf(carro->anguloDasRodas
                                      + rapidezDasRodas(carro->velocidade, comandos.acelerar || carro->nitroLigado) * tempoQuadro,
                                      2.0f * 3.14159265f);
        float lateralDepoisLivre = carro->deslocamentoLateral * larguraDoLado(carro->posicao.segmento, carro->deslocamentoLateral);
        carro->velocidadeDeLado = (tempoQuadro > 0.0f) ? (lateralDepoisLivre - lateralAntesLivre) / tempoQuadro : 0.0f;
        carro->voltas += cruzouLivre;
        atualizarSuspensao(carro, tempoQuadro);
        return cruzouLivre;
    }
    atualizarLateral(carro, comandos, tempoQuadro);
    carro->anguloDasRodas = fmodf(carro->anguloDasRodas
                                  + rapidezDasRodas(carro->velocidade, comandos.acelerar || carro->nitroLigado) * tempoQuadro,
                                  2.0f * 3.14159265f);

    float lateralDepois = carro->deslocamentoLateral * larguraDoLado(carro->posicao.segmento, carro->deslocamentoLateral);
    carro->velocidadeDeLado = (tempoQuadro > 0.0f) ? (lateralDepois - lateralAntes) / tempoQuadro : 0.0f;

    /* Anda, mais o que uma batida empurrou. Para trás ele não atravessa a linha
     * de chegada, para a volta não contar duas vezes */
    float distancia = carro->velocidade * tempoQuadro + carro->empurraoPendente;
    carro->empurraoPendente = 0.0f;
    if (carro->posicao.segmento->ehLinhaDeChegada && carro->posicao.distanciaNoSegmento + distancia < 0.0f) {
        distancia = -carro->posicao.distanciaNoSegmento;
    }
    carro->distanciaPercorrida += distancia;
    atualizarDesgaste(carro, distancia);

    /* Mudando de segmento, o carro continua no mesmo lugar em metros: se a pista
     * alarga ou estreita daquele lado, muda a fração da largura, não o caminho dele */
    Segmento *segmentoAntes = carro->posicao.segmento;
    int voltasGanhas = moverNaPista(&carro->posicao, distancia);
    if (carro->posicao.segmento != segmentoAntes) {
        carro->deslocamentoLateral *= larguraDoLado(segmentoAntes, carro->deslocamentoLateral)
                                      / larguraDoLado(carro->posicao.segmento, carro->deslocamentoLateral);
    }
    carro->voltas += voltasGanhas;
    atualizarSuspensao(carro, tempoQuadro);
    return voltasGanhas;
}

/*
 * velocidadeEmKmh
 * Converte a velocidade interna para o velocímetro.
 * Recebe: o carro.
 * Devolve: a velocidade em km/h, arredondada.
 */
int velocidadeEmKmh(Carro *carro) {
    return converterParaKmh(carro->re > 0.0f ? carro->re : carro->velocidade);
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

    float k = curvatura * FORCA_CENTRIFUGA * (1.0f - aderenciaAtual(carro)) * pe;
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
 * velocidadeMaximaAgora
 * A máxima de verdade neste momento: a da ficha, mais o vácuo e o nitro.
 * Recebe: o carro.
 * Devolve: a velocidade máxima, em unidades por segundo.
 */
float velocidadeMaximaAgora(Carro *carro) {
    float ganho = 1.0f + GANHO_NO_VACUO * carro->vacuo;
    if (carro->nitroLigado) {
        ganho += GANHO_DO_NITRO;
    }
    return carro->velocidadeMaxima * ganho;
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
        atualizarMotor(&motor, atual / ficha->velocidadeMaxima, 1, 0, 0, passo);
        atual += forcaDoMotor(atual, ficha->velocidadeMaxima, ficha->aceleracao, &motor) * passo;
        tempo += passo;
    }
    return tempo;
}
