/*
 * gestos.h
 * ------------------------------------------------------------------
 * A linguagem do corpo dos moradores (e do Mack): como cada um se mexe
 * falando, ouvindo, contente, bravo, assustado, tímido ou com sono, do
 * jeito dele.
 *
 * A regra é a da Pixar em Carros, a "verdade do material": o carro é de
 * metal e pesa mais de uma tonelada, então não estica, não se dobra e não
 * fica em pé nas rodas de trás para gesticular com as da frente. O que ele
 * faz com o corpo é o que uma suspensão deixa: o corpo sobe e desce sobre
 * as rodas, deita de lado e abaixa o bico girando em volta do eixo de trás
 * (o eixo de trás é o quadril; o carro inteiro é a cabeça). As rodas da
 * frente são as mãos. O resto vai nos olhos (a pálpebra faz de
 * sobrancelha) e na boca.
 *
 * Nada aqui é uma animação pronta. Há duas camadas, ao mesmo tempo:
 *   - o involuntário: a suspensão de cada um (o mesmo gesto no Mate, mole,
 *     balança e demora a parar; no Sargento sai seco), a respiração do
 *     motor em marcha lenta, o corpo que nunca fica parado de todo (um
 *     ruído suave, nunca igual), o reflexo do susto, o tremor do medo, o
 *     ar de um carro passando rente, a arrancada, a freada e a rua;
 *   - o voluntário: o que ele quer dizer com o corpo, que nasce do ânimo
 *     dele (Animo: alegria, agitação, conforto, interesse e sono). O ânimo
 *     sobe e desce com o que acontece (o jogador chegando devagar ou
 *     correndo, uma batida, a fala dele, a fala de quem conversa com ele,
 *     a noite) e volta devagar ao temperamento de cada um. A postura, os
 *     olhos e o tamanho e o jeito de cada gesto saem daí, na medida do
 *     que aconteceu. Falando, as ênfases caem nas palavras da frase e a
 *     boca abre nas vogais; ouvindo, o "uhum" responde às ênfases de quem
 *     fala; os tiques são vontades que se acumulam (o Ramone contente
 *     bombeia a hidráulica; o Sargento, desleixando, se apruma; a Lizzie
 *     sozinha cochila e acorda num susto).
 *
 * Os gestos são dados pelo quanto o corpo vai (radianos, metros), e a
 * suspensão de cada um diz só como ele vai: assim um carro de mola dura
 * gesticula do mesmo tamanho que um de mola mole, só que mais seco. Cada
 * gesto é um pulso suave no alvo da mola, nunca um tranco, e dura pelo
 * menos meia volta da suspensão: quem é pesado e mole (o Ruivo, o Mack)
 * gesticula mais devagar. Os limites da suspensão são macios.
 *
 * Não usa a raylib (regra 2 do roadmap): devolve só os números da pose.
 * ------------------------------------------------------------------
 */

#ifndef GESTOS_H
#define GESTOS_H

/* O que uma fala quer dizer (a tabela de moradores_mapa.h, ou a pontuação). Não é a cara que ele faz: é o
 * empurrão que a fala dá no ânimo, que se soma ao que ele já sentia */
typedef enum {
    HUMOR_NEUTRO,
    HUMOR_FELIZ,
    HUMOR_EMPOLGADO,
    HUMOR_BRAVO,
    HUMOR_ASSUSTADO,
    HUMOR_TIMIDO,
    HUMOR_CURIOSO,
    HUMOR_PENSATIVO,
    HUMOR_CALMO,
    HUMOR_TRISTE,
    HUMOR_SERIO,
    TOTAL_DE_HUMORES
} Humor;

/* Os tiques: o jeito próprio de cada um. Cada um nasce de uma vontade que o ânimo vai enchendo */
typedef enum {
    TIQUE_NENHUM,
    TIQUE_HIDRAULICA,   /* o Ramone: contente, a suspensão a ar bombeia ou ergue um lado */
    TIQUE_MOTOR_VELHO,  /* a Lizzie: o motor treme sempre, mais quando ela se agita */
    TIQUE_GINGADO,      /* o Fillmore: à vontade e contente, balança devagar */
    TIQUE_CHACOALHO,    /* o Mate: a lataria solta chacoalha quando o corpo mexe forte */
    TIQUE_ACELERA,      /* o McQueen: agitado e parado, acelera o motor (o corpo torce) */
    TIQUE_SENTIDO,      /* o Sargento: o corpo desleixa aos poucos, e ele se apruma */
    TIQUE_INQUIETO      /* o Guido: agitado, não para quieto (as rodas, os garfos, pulinhos) */
} Tique;

/* O ânimo: o que ele sente agora. Contínuo; volta devagar ao temperamento */
typedef struct {
    float alegria;     /* -1 (chateado, bravo) a +1 (contente) */
    float agitacao;    /* 0 (calmo, sonolento) a 1 (elétrico) */
    float conforto;    /* -1 (ameaçado, sem graça) a +1 (à vontade) */
    float interesse;   /* 0 a 1: o quanto presta atenção em quem olha */
    float sono;        /* 0 a 1 */
} Animo;

/* O jeito de uma pessoa */
typedef struct {
    /* A suspensão: como o corpo vai (não o quanto) */
    float frequencia;      /* Hz: alta é seca (o Guido), baixa é mole (o Ruivo, o Fillmore) */
    float amortecimento;   /* 0 a 1: pouco, o corpo balança depois de cada gesto (o Mate) */
    /* O temperamento: o ânimo de sempre e o quanto as coisas o mexem */
    float alegria, agitacao, conforto;
    float empatia;         /* 0 a 1: o quanto pega o ânimo de quem fala com ele */
    float timidez;         /* 0 a 1: gente perto o deixa sem graça (o Ruivo) */
    float sonolencia;      /* 0 a 1: sozinho e parado, o sono chega (a Lizzie, o Mack) */
    float inquietude;      /* 0 a 1: o corpo mexe sozinho (o Guido muito, o Doc quase nada) */
    /* O jeito de se expressar */
    float energia;         /* o tamanho dos gestos (1: o normal) */
    float ritmo;           /* ênfases por segundo, no máximo, falando */
    float postura;         /* radianos, o bico parado: + abaixado (a Lizzie), - erguido (o Sargento) */
    float altura;          /* metros, o corpo parado sobre as rodas: - rebaixado (o Ramone) */
    float maos;            /* 0 a 1: o quanto gesticula com as rodas da frente */
    float requebro;        /* 0 a 1: o quanto as ênfases deitam o corpo de lado (a Flo) */
    float boca;            /* o quanto a boca abre falando (1: o normal) */
    Humor exclamando;      /* o que um "!" quer dizer nele (o Sargento: bravo) */
    Tique tique;
} Jeito;

/* Os olhos: o quanto de cada emoção (0 a 1) e para onde olham na altura (-1 embaixo a +1 em cima) */
typedef struct {
    float alegria, raiva, tristeza, espanto, sono;
    float olharParaCima;
} Emocao;

/* O corpo agora, para o desenho */
typedef struct {
    float rolagem;    /* radianos: + o teto vai para a esquerda do carro */
    float aceno;      /* radianos: + o bico desce, girando em volta do eixo de trás */
    float subida;     /* metros: o corpo sobe (+) ou baixa (-) sobre as rodas */
    float maos;       /* -1 a +1: as rodas da frente, somadas ao volante (as mãos gesticulando) */
    float abreAsMaos; /* 0 a 1: as rodas da frente abertas para fora (os braços abertos: a pergunta, o "!") */
    float garfos;     /* 0 a 1: os garfos do Guido erguidos */
    float boca;       /* 0 (fechada) a 1 (toda aberta) */
    Emocao olhos;
} Pose;

/* O que está acontecendo com ele agora (moradores.c e passeio.c contam) */
typedef struct {
    /* Ele mesmo */
    int parado;               /* estacionado */
    float velocidade;         /* m/s */
    float volante;            /* -1 a +1 */
    const char *fala;         /* o que ele está dizendo (NULL: calado) */
    float duracaoDaFala;      /* segundos que a fala inteira fica na tela */
    Humor humorDaFala;        /* o que a fala quer dizer */
    /* A conversa */
    int ouvindo;              /* numa conversa, na vez do outro */
    float enfaseDoOutro;      /* a ênfase que quem fala com ele deu agora (0: nenhuma) */
    float alegriaDoOutro, agitacaoDoOutro;   /* o ânimo de quem fala com ele */
    /* Para onde olha */
    int atento;               /* olhando alguém */
    float lado;               /* -1 a +1: para que lado está quem ele olha */
    float perto;              /* 0 a 1: o quanto quem ele olha está perto */
    /* O jogador */
    float perigo;             /* 0 a 1: o jogador vindo rápido e rente (daí o susto) */
    float visita;             /* 0 a 1: o jogador parado ou devagar ali perto (companhia) */
    float ladoDoJogador;      /* -1 a +1 */
    float batida;             /* 0 a 1: bateram nele agora (só no quadro da batida) */
    float vento;              /* o ar de um carro passando rápido rente: + empurra para a esquerda */
    float bronca;             /* 0 a 1: o xerife atrás de quem corre */
    /* Em volta */
    int sozinho;              /* ninguém por perto */
    float noite;              /* 0 (dia) a 1 (tarde da noite) */
} Situacao;

/* Uma mola do corpo: a posição e a velocidade */
typedef struct {
    float x, v;
} MolaDoCorpo;

#define MAIS_BATIDAS  16
#define MAIS_TOQUES   32
#define MAIS_PULSOS   8

/* Um gesto em andamento: o alvo de uma mola sobe e volta numa curva suave (a mola vai atrás dele) */
typedef struct {
    float inicio, duracao, quanto;   /* segundos no relógio do corpo, segundos (0: livre) e o pico do alvo */
} Pulso;

/* O estado do corpo de um morador (quem o usa guarda um para cada) */
typedef struct {
    MolaDoCorpo rolagem, aceno, subida, maos, garfos, boca, pergunta, abreAsMaos;
    Pulso pulsos[3][MAIS_PULSOS];     /* os gestos em andamento na rolagem, no aceno e na subida */
    Animo animo;
    float relogio;                    /* segundos */
    float faseDoRespiro, faseDoGingado;
    float fases[4][3];                /* do ruído do corpo, sorteadas: cada um mexe de um jeito */
    /* A fala */
    const char *falaAntes;
    float tempoDaFala, tempoPorLetra;
    float batidas[MAIS_BATIDAS], forcas[MAIS_BATIDAS];
    int totalDeBatidas, proximaBatida;
    float toques[MAIS_TOQUES], forcasDosToques[MAIS_TOQUES];   /* as mãos: um gesto a cada palavra */
    int totalDeToques, proximoToque;
    float ladoDaMao;
    float tempoDaPergunta;            /* quando a frase vira pergunta (< 0: não é) */
    float pensando;                   /* 0 a 1: pensando no que diz (olha para cima) */
    float ladoDaEnfase;
    float alvoDasMaos, alvoDosGarfos, alvoDaAbertura;
    float enfase;                     /* a ênfase que ele deu neste passo (quem ouve responde) */
    /* Ouvindo */
    float uhum, forcaDoUhum;          /* o "uhum" que vem daqui a pouco */
    /* O susto */
    float refratario, espanto, perigoAntes;
    /* Os tiques */
    float vontade, desleixo, chacoalho, tique, forcaDoTique, interesseAntes;
    int qualTique, cochilando;
    /* Andando */
    float velocidadeAntes, arrancada, proximoSolavanco;
    unsigned int sorteio;
} Corpo;

void iniciarCorpo(Corpo *corpo, const Jeito *jeito, unsigned int semente);
/* Um passo do corpo: o jeito da pessoa, o que está acontecendo e o tempo; devolve a pose */
void moverCorpo(Corpo *corpo, const Jeito *jeito, const Situacao *agora, float tempo, Pose *pose);

#endif
