/*
 * configuracoes.h
 * ------------------------------------------------------------------
 * O que o jogador escolhe na tela de configurações: as teclas e os botões de
 * cada comando, as opções do controle e as de exibição (resolução, modo de tela,
 * sincronia vertical, limite de quadros, contador de FPS e gráficos).
 * Fica salvo em um arquivo de texto.
 *
 * Este módulo NÃO usa a raylib: os códigos das teclas e dos botões são
 * apenas números (os mesmos da raylib), e quem os lê é o controle.c.
 * ------------------------------------------------------------------
 */

#ifndef CONFIGURACOES_H
#define CONFIGURACOES_H

#define ARQUIVO_DE_CONFIGURACOES "configuracoes.txt"

/* Os comandos da corrida que dá para trocar. Pausar e os menus são fixos. */
typedef enum {
    ACAO_ACELERAR,
    ACAO_FREAR,
    ACAO_ESQUERDA,
    ACAO_DIREITA,
    ACAO_NITRO,
    ACAO_FREIO_DE_MAO,
    TOTAL_ACOES
} AcaoDeCorrida;

/* Cada comando tem duas teclas e dois botões: as 4 colunas da tela */
typedef enum {
    COLUNA_TECLA_1,
    COLUNA_TECLA_2,
    COLUNA_BOTAO_1,
    COLUNA_BOTAO_2,
    TOTAL_COLUNAS
} ColunaDeComando;

#define SEM_COMANDO  0    /* KEY_NULL e GAMEPAD_BUTTON_UNKNOWN, na raylib */

/* O jeito do volante no analógico */
typedef enum {
    VOLANTE_SUAVE,        /* bem fino perto do centro */
    VOLANTE_NORMAL,
    VOLANTE_DIRETO,       /* quanto inclina, tanto vira */
    TOTAL_VOLANTES
} RespostaDoVolante;

/* O modo de tela: ocupar o monitor todo ou abrir numa janela */
typedef enum {
    MODO_TELA_CHEIA,
    MODO_JANELA,
    TOTAL_MODOS_DE_TELA
} ModoDeTela;

/* As resoluções da tabela (índice 0 = automática). A maior é 1920x1080. */
#define TOTAL_RESOLUCOES          7
#define MAIOR_LARGURA             1920
#define MAIOR_ALTURA              1080
/* Os limites de quadros por segundo da tabela (o último é sem limite) */
#define TOTAL_LIMITES_DE_QUADROS  6
#define LIMITE_PADRAO_DE_QUADROS  1     /* índice do 60 */

/* Os pacotes da opção "gráficos" e a tabela de escalas de renderização */
typedef enum { PRESET_BAIXO, PRESET_MEDIO, PRESET_ALTO, PRESET_PERSONALIZADO, TOTAL_PRESETS } PresetGrafico;
#define TOTAL_ESCALAS_DA_IMAGEM   6
#define ESCALA_PADRAO_DA_IMAGEM   5     /* índice do 100% */
#define TOTAL_QUALIDADES_DE_SOMBRA 5
#define TOTAL_FILTROS_DE_TEXTURA  5
#define MAIOR_VOLUME              10

#define MENOR_FORCA_DA_VIBRACAO   10
#define PASSO_DA_FORCA            10
#define MENOR_ZONA_MORTA          2
#define MAIOR_ZONA_MORTA          30
#define PASSO_DA_ZONA_MORTA       2

typedef struct {
    int comandos[TOTAL_ACOES][TOTAL_COLUNAS];
    int vibracaoLigada;
    int forcaDaVibracao;      /* porcentagem, de 10 a 100 */
    int gatilhosAdaptativos;  /* 1: o gatilho do acelerador treme com a roda patinando (DualSense) */
    int zonaMorta;            /* porcentagem do analógico que conta como "reto" */
    int respostaDoVolante;    /* RespostaDoVolante */
    int efeitos;              /* 1: brilho, cor e desfoque de velocidade (pós-processamento) */
    int sombras;              /* 1: o cenário faz sombra (pesa um pouco no desempenho) */
    int direcaoGuiada;        /* 1: o carro acompanha a curva; 0: ele mantém o rumo e quem vira é o volante */
    int inverterCameraX;
    int inverterCameraY;
    int resolucao;            /* índice na tabela de resoluções; 0 = automática */
    int modoDeTela;           /* ModoDeTela */
    int sincroniaVertical;    /* 1: cada quadro sai junto com a atualização do monitor (VSync) */
    int limiteDeQuadros;      /* índice na tabela de limites de quadros por segundo */
    int mostrarFps;           /* 1: mostra o contador de quadros por segundo no canto da tela */
    int tracadoDeCorrida;     /* 1: a linha de corrida no asfalto: verde acelera, amarelo alivia, vermelho freia */
    /* ---- Gráficos ---- */
    int presetGrafico;        /* PresetGrafico: o último pacote escolhido (PRESET_PERSONALIZADO se mexeu depois em alguma opção) */
    int escalaDaImagem;       /* índice na tabela de escalas de renderização: o mundo é desenhado menor e ampliado (mais FPS) */
    int bloom;                /* 1: o brilho em volta das luzes */
    int lensFlare;            /* 1: os raios de luz dos clarões das batidas e das faíscas */
    int granulado;            /* 1: granulado de filme sobre a imagem */
    int profundidadeDeCampo;  /* 1: o fundo distante fica levemente desfocado (simulado, sem custo de profundidade) */
    int distorcaoDaLente;     /* 0 = nenhuma, 1 = distorção, 2 = distorção + aberração cromática */
    int nitidez;              /* 0 = nenhuma, 1 = leve, 2 = forte (afiar a imagem, como o FidelityFX CAS) */
    int suavizacao;           /* 0 = nenhuma, 1 = FXAA (suaviza as serrilhas das bordas) */
    int oclusaoAmbiental;     /* 1: escurece os cantos e as frestas (simulado na imagem, sem profundidade) */
    int qualidadeDasSombras;  /* 0 = sem sombras, 1 a 4 = tamanho do mapa de sombra (1024, 2048, 3072, 4096); vale ao abrir o jogo */
    int filtroDeTexturas;     /* 0 = bilinear, 1 = trilinear, 2 = anisotrópico 4x, 3 = 8x, 4 = 16x; vale ao abrir o jogo */
    /* ---- Som ---- */
    int volumeVozes;          /* 0 a 10: as falas do McQueen */
    int volumeMusica;         /* 0 a 10: as músicas */
    int volumeEfeitos;        /* 0 a 10: motores, batidas, pneus, vento, fogos... */
    int volumeMenu;           /* 0 a 10: os sons dos menus */
    int alcanceDinamico;      /* 0 = amplo (do baixinho ao estrondo), 1 = reduzido (sobe o baixo e segura o alto) */
    int surroundVirtual;      /* 1: abre a imagem do som estéreo, para fones e caixas parecerem mais largos */
} Configuracoes;

Configuracoes *configuracoesAtuais(void);
void configuracoesPadrao(Configuracoes *config);
void definirComando(Configuracoes *config, AcaoDeCorrida acao, ColunaDeComando coluna, int codigo);
int limparComando(Configuracoes *config, AcaoDeCorrida acao, ColunaDeComando coluna);
int salvarConfiguracoes(const Configuracoes *config, const char *arquivo);
int carregarConfiguracoes(Configuracoes *config, const char *arquivo);
const char *nomeDaAcao(AcaoDeCorrida acao);
int resolucaoDaTabela(int indice, int *largura, int *altura);
int resolucaoCabe(int indice, int larguraDoMonitor, int alturaDoMonitor);
void resolucaoEfetiva(const Configuracoes *config, int larguraDoMonitor, int alturaDoMonitor, int *largura, int *altura);
int quadrosPorSegundoDoLimite(int indice);
int escalaDaImagemEmPorcento(int indice);
int ladoDoMapaDeSombra(int qualidade);
void aplicarPresetGrafico(Configuracoes *config, PresetGrafico preset);
float fatorDeVolume(int volume);

#endif
