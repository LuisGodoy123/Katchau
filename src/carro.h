/*
 * carro.h
 * ------------------------------------------------------------------
 * O carro e o seu movimento pela pista.
 *
 * O carro não lê o teclado. Quem decide o que fazer preenche uma
 * struct Comandos (acelerar, frear, virar) e o carro só obedece.
 * Assim o mesmo código serve para o jogador (comandos vêm do teclado,
 * em main.c) e para os rivais (comandos vêm da IA, na Fase 4).
 *
 * Este módulo NÃO usa a raylib.
 * ------------------------------------------------------------------
 */

#ifndef CARRO_H
#define CARRO_H

#include "config.h"
#include "pista.h"
#include "motor.h"

#define TAMANHO_NOME        20
#define LIMITE_PISTA        1.0f   /* além disso (para qualquer lado) é grama */
#define GRAUS_DO_VOLANTE    7.0f   /* o carro aponta para onde o volante manda */
#define TOTAL_SETORES       3      /* a volta é dividida em 3 setores, como no F1 */

/* Ficha técnica de um modelo de carro. A garagem mostra e ordena
 * as fichas; a corrida cria os carros a partir delas. */
typedef struct {
    const char *nome;
    const char *descricao;         /* o jeito do carro, mostrado na garagem */
    float velocidadeMaxima;        /* unidades por segundo */
    float aceleracao;              /* quanto a velocidade sobe por segundo */
    float aderencia;               /* de 0 a 1 */
    float comprimento;             /* metros: o tamanho do modelo 3D e da caixa de colisão */
    float largura;
} FichaTecnica;

/* Por onde o carro anda no pit lane (pitstop.c) */
typedef enum {
    PIT_FORA,       /* na pista */
    PIT_CHEGANDO,   /* do começo do pit lane até a vaga */
    PIT_TROCANDO,   /* parado na vaga */
    PIT_SAINDO      /* da vaga até voltar à pista */
} EstadoDoPit;

/* O que o piloto quer fazer neste quadro: 1 = sim, 0 = não.
 * As setas (esquerda/direita) giram o volante todo; o analógico do
 * controle diz quanto girar, em "direcao". */
typedef struct {
    int acelerar;
    int frear;
    int esquerda;
    int direita;
    float direcao;                 /* analógico: -1 todo à esquerda a +1 todo à direita (0 = só as setas) */
    int nitro;                     /* 1 = quer usar o nitro */
    int freioDeMao;                /* 1 = puxou o freio de mão */
} Comandos;

typedef struct {
    char nome[TAMANHO_NOME];
    int numero;                    /* qual carro é (define o modelo 3D) */

    /* ---- Onde está ---- */
    PosicaoNaPista posicao;        /* segmento atual (nó da lista) e quanto andou nele */
    float distanciaPercorrida;     /* total desde a largada; ordena os carros na Fase 4 */
    float deslocamentoLateral;     /* 0 = centro, -1 = borda esquerda, +1 = borda direita */

    /* ---- Como está andando ---- */
    float velocidade;              /* unidades de mundo por segundo */
    float volante;                 /* -1 todo à esquerda, 0 reto, +1 todo à direita; anda aos poucos */
    float derrapagem;              /* 0 com folga; sobe até 1 quando a curva está vencendo o volante */
    float vacuo;                   /* 0 no ar limpo, até 1 colado atrás de outro carro (corrida.c preenche) */
    Motor motor;                   /* rotação e marcha (o câmbio corta a potência na troca) */
    float anguloDasRodas;          /* radianos: quanto as rodas já giraram (só para o desenho) */
    float nitro;                   /* quanto nitro tem: de 0 a 1 */
    int nitroLigado;               /* 1 enquanto está queimando */

    /* ---- Colisão ---- */
    float comprimento, largura;    /* metros: o tamanho da caixa de colisão */
    float empurraoPendente;        /* unidades: o quanto uma batida o empurrou para a frente (ou para trás) */
    float velocidadeLateral;       /* metros por segundo para a direita: o tranco de uma batida de lado */
    float velocidadeDeLado;        /* metros por segundo para a direita: o quanto ele está indo para o lado */

    float pancada;                 /* de 0 a 1: a força da última batida (a vibração do controle consome) */
    float batidaNova;              /* de 0 a 1: uma batida que o som ainda não tocou (batidaDoMcQueen consome) */
    int batidaNovaDeCarro;         /* 1 se essa batida foi contra outro carro (0 = foi no muro) */
    int rodadaNova;                /* 1 quando ESTE carro começou a rodar e o som ainda não tocou (rodadaDoMcQueen consome) */
    int raspandoNoMuro;            /* 1 enquanto o carro está encostado no muro */
    float contato;                 /* de 0 a 1: a força do contato NESTE quadro (com carro ou muro); 0 = sem contato.
                                    * Para as faíscas e o som de lata: vale para todos os carros */
    int contatoComCarro;           /* 1: o contato deste quadro foi com outro carro (0 = no muro) */
    float contatoX, contatoY, contatoZ;   /* metros: onde, no mundo, as duas superfícies se tocam */
    float contatoDeslize;          /* metros por segundo: quanto uma superfície escorrega na outra */

    /* ---- Rodada ---- */
    int rodando;                   /* 1 enquanto está rodando: o piloto não manda no carro */
    float giro;                    /* graus: quanto o carro está virado em relação à pista */
    float velocidadeDoGiro;        /* graus por segundo */
    float tempoSemRodar;           /* segundos até poder rodar de novo */
    float velocidadeNaRodada;      /* a velocidade quando começou a rodar */

    /* ---- Freio de mão e drift ---- */
    int direcaoLivre;              /* 1: sem direção guiada, o carro anda no mundo e quem faz a curva é o volante */
    float desvioDoRumo;            /* graus: para onde o bico aponta em relação à linha da pista, sem a direção guiada */
    float ritmoDoPelotao;              /* o rubber banding dos rivais: 1 = normal (veja corrida.c) */
    float re;                          /* unidades por segundo andando de ré (0 = não está de ré) */
    float rumoDeRe;                    /* graus que o bico girou dando ré (direção guiada) */
    float paradoFreando;               /* segundos parado com o freio apertado: passou do tempo, engata a ré */
    int controladoPeloJogador;         /* o jogador: dá ré; a IA, não */
    float rolagem, rapidezDaRolagem;   /* suspensão: graus que o corpo inclina para fora da curva */
    float arfagem, rapidezDaArfagem;   /* graus que o bico sobe (freando, desce) */
    float rumoAnterior, velocidadeAnterior;
    int temSuspensao;
    int temRumoNoMundo;            /* sem a direção guiada: 1 quando o campo abaixo já vale */
    float rumoNoMundo;             /* graus: para onde o carro aponta no mundo; é o volante que muda */
    float rumoDaViagem;            /* graus: para onde ele está indo de fato (o rumo mais o pedaço de lado do
                                    * drift e das batidas): a câmera vai atrás dele, como nos jogos de corrida */
    float anguloDeFuga;            /* graus: sem volante na curva, o carro segue reto, e o bico não acompanha a pista */
    float forcaDoDrift;            /* de 0 a 1: o quanto a traseira está solta */
    float anguloDeDrift;           /* graus: o quanto o carro está de lado por causa do drift */
    float giroDoDrift;             /* graus por segundo: a traseira tem inércia */

    /* ---- Pneus (só no nível difícil) ---- */
    int gastaPneu;                 /* 1: os pneus gastam nesta corrida */
    float metrosDeUmJogoDePneus;   /* quanto um jogo de pneus anda (mais se a pista for maior: veja corrida.c) */
    float desgaste;                /* de 0 (novos) a 1 (acabados); o Guido zera na troca */
    int pneuEstourado;             /* os pneus estourados, um bit para cada roda (0 = nenhum): bit 0 dianteiro esquerdo,
                                    * 1 dianteiro direito, 2 traseiro esquerdo, 3 traseiro direito. Os quatro estouram
                                    * aos poucos, de 5% de vida até 0%, os de trás primeiro (veja estourarPneu) */
    int ladoDoPrimeiroEstouro;     /* 0 = o primeiro pneu a estourar foi o esquerdo, 1 = o direito (os outros seguem dele) */
    float tempoDoEstouro;          /* segundos desde o último estouro */
    float fasePulso;               /* segundos andando no topo da velocidade: é a fase das oscilações da velocidade (veja atualizarVelocidade) */

    /* ---- Pitstop ---- */
    EstadoDoPit estadoDoPit;
    int voltaDoPit;                /* só para a IA: em que volta ela vai parar (0 = nunca) */
    int querParar;                 /* 1: a IA vai entrar no próximo pit lane */
    int pitFeito;                  /* 1 depois de completar uma troca */
    float tempoDaTroca;            /* segundos parado na vaga */
    float duracaoDaTroca;          /* quanto a troca dele leva */
    int vemAlguemAtras;            /* 1: no pit lane, vem um carro aí atrás (quem está saindo da vaga espera) */
    float ultimaTroca;             /* quanto levou a última (0 = nenhuma) */

    /* ---- O que o piloto da IA enxerga (corrida.c preenche a cada quadro) ---- */
    float habilidade;              /* de 0 a 1: quanto do limite do carro a IA usa */
    float distanciaAFrente;        /* metros até a traseira de quem está na mesma linha (negativo = ninguém) */
    float velocidadeAFrente;       /* a velocidade dele */
    float lateralAFrente;          /* metros: onde ele está na largura */
    int ocupadoEsquerda, ocupadoDireita;   /* 1 se há um carro encostado nesse lado */
    int voltas;                    /* voltas completadas */
    int voltasDaProva;             /* quantas voltas tem esta corrida */
    float temposDeVolta[MAXIMO_DE_VOLTAS];
    float inicioDaVolta;           /* momento da corrida em que a volta atual começou */
    float tempoDeChegada;          /* momento em que completou a última volta (0 = ainda correndo) */
    float melhorVolta;             /* menor tempo de volta até agora (0 = nenhuma volta completa) */
    Comandos ultimosComandos;      /* o que o piloto fez no último quadro (lanternas de freio) */
    float linhaAlvo;               /* só para a IA: em que ponto da largura ela quer andar, em metros */

    /* ---- O jeito de correr da IA (corrida.c e ia.c) ---- */
    float esforco;                 /* o quanto se esforça agora além da habilidade (atrás do pelotão, no revide) */
    float revide;                  /* segundos que faltam do revide: foi ultrapassado e quer a posição de volta */
    float vontadeDeRevide;         /* de 0 a 1, do nível: a força do revide */
    float defesa;                  /* de 0 a 1, do nível: a chance de fechar a porta para quem vem atrás */
    float defendendo;              /* > 0: segundos segurando a linha da defesa; < 0: espera para defender de novo */
    float linhaDaDefesa;           /* metros: a linha que está segurando */
    float variacao;                /* do nível: o quanto o ritmo muda de uma volta para a outra */
    float ritmoDaVolta;            /* o desta volta, somado à habilidade */
    int voltaDoRitmo;              /* a volta em que o ritmo foi sorteado */
    unsigned int sorteio;          /* os sorteios deste piloto (não mexe no rand() do resto do jogo) */
    int aFrenteDoJogador;          /* 1 se estava à frente do jogador no quadro anterior (para o revide) */
    float distanciaAtras;          /* metros até quem vem logo atrás, em qualquer linha (negativo = ninguém) */
    float velocidadeAtras;         /* a velocidade dele */
    float lateralAtras;            /* metros: onde ele está na largura */
    float distanciaDoDaFrente;     /* metros até o carro logo à frente, em qualquer linha (negativo = ninguém) */
    float velocidadeDoDaFrente;    /* a velocidade dele */
    float lateralDoDaFrente;       /* metros: onde ele está na largura */

    /* ---- Setores: a volta em 3 partes, para o painel da volta no HUD ---- */
    int setorAtual;                              /* 0, 1 ou 2 */
    float inicioDoSetor;                         /* momento da corrida em que o setor atual começou */
    float setoresDaVolta[TOTAL_SETORES];         /* setores já feitos nesta volta (0 = ainda não) */
    float melhoresSetores[TOTAL_SETORES];        /* melhor tempo do carro em cada setor (0 = nenhum) */
    float setoresDaMelhorVolta[TOTAL_SETORES];   /* os setores da melhor volta, para comparar */
    float diferenca;                             /* ganho (< 0) ou perda (> 0) contra a melhor volta, em s */
    int temDiferenca;                            /* 0 até existir uma melhor volta para comparar */

    /* ---- Ficha técnica (cada carro tem a sua; usada na garagem da Fase 6) ---- */
    float velocidadeMaxima;        /* unidades por segundo */
    float aceleracao;              /* força da arrancada (a aceleração cai perto da máxima) */
    float aderencia;               /* de 0 a 1: quanto resiste a ser jogado para fora na curva */
} Carro;

Carro criarCarro(const char *nome, int numero, Segmento *largada, float deslocamentoLateral,
                 float velocidadeMaxima, float aceleracao, float aderencia);
int atualizarCarro(Carro *carro, Comandos comandos, float tempoQuadro);
int estaForaDaPista(Carro *carro);
Superficie superficieDoCarro(Carro *carro);
float aceleracaoDeCurva(const Carro *carro);
void atualizarSuspensao(Carro *carro, float tempoQuadro);
int velocidadeEmKmh(Carro *carro);
float dirigirSemPista(Carro *carro, Comandos comandos, float tempoQuadro, float aderenciaDoChao, float *deLado, int *deRe);
void girarVolante(Carro *carro, Comandos comandos, float tempoQuadro);
void estourarPneu(Carro *carro);
int quantosPneusEstourados(const Carro *carro);
int converterParaKmh(float velocidade);
float velocidadeSeguraNaCurva(Carro *carro, float curvatura);
float velocidadeSeguraAcelerando(Carro *carro, float curvatura);
float velocidadeMaximaAgora(Carro *carro);
float velocidadeParaFrearAte(float alvo, float distancia);
float rapidezDasRodas(float velocidade, int acelerando);
float aderenciaAtual(const Carro *carro);
float volanteDaFolgaDaCurva(const Carro *carro);
void fazerRodar(Carro *carro, float impulso);
void meiasMedidasDoCarro(const Carro *carro, float *meioComprimento, float *meiaLargura);
float tempoAteVelocidade(const FichaTecnica *ficha, float velocidade);

#endif
