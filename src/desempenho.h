/*
 * desempenho.h
 * ------------------------------------------------------------------
 * A medição do quadro: quanto cada etapa leva na CPU e na placa de vídeo,
 * o que mudou de um quadro para o outro (as contas: lâmpadas mandadas,
 * sombras refeitas...) e o gráfico disso na tela (F3, ou a opção
 * "Desempenho na tela" das configurações). O benchmark do passeio grava
 * tudo num arquivo (gravarDesempenho).
 *
 * Cada etapa do quadro é marcada quando começa (etapaDoQuadro): o tempo
 * até a próxima marca é dela. Na placa, cada etapa tem uma consulta de
 * tempo (GL_TIME_ELAPSED) que só é lida uns quadros depois, sem esperar a
 * placa. Com o gráfico desligado e sem gravação, nada é medido.
 *
 * Este módulo usa a raylib (desenha o gráfico).
 * ------------------------------------------------------------------
 */

#ifndef DESEMPENHO_H
#define DESEMPENHO_H

/* As etapas do quadro, na ordem em que acontecem (e a ordem das cores no gráfico) */
typedef enum {
    ETAPA_LOGICA,       /* o jogo andando: o carro, a IA, os moradores, a câmera */
    ETAPA_SOM,          /* a música e os efeitos (a música é decodificada aqui) */
    ETAPA_PREPARO,      /* a luz da hora e as lâmpadas escolhidas, mandadas aos shaders */
    ETAPA_SOMBRAS,      /* o mapa de sombra do sol */
    ETAPA_MUNDO,        /* o cenário, os interiores, os carros, a fumaça */
    ETAPA_LUZES,        /* o halo das luzes (as luzes de novo, sozinhas) */
    ETAPA_TELOES,       /* a transmissão dos telões (o mundo desenhado mais uma vez) */
    ETAPA_TELA,         /* o pós-processamento e o HUD */
    ETAPA_TROCA,        /* a troca de quadro: a CPU espera a placa (e o VSync) */
    TOTAL_ETAPAS
} EtapaDoQuadro;

/* O que se conta em cada quadro */
typedef enum {
    CONTA_LAMPADAS_ACESAS,      /* as luzes pontuais acesas perto */
    CONTA_LAMPADAS_NOS_SHADERS, /* as escolhidas para os shaders */
    CONTA_ENVIOS_DE_LAMPADAS,   /* vezes que as lâmpadas foram mandadas aos shaders */
    CONTA_GRADES_MONTADAS,      /* vezes que a grade das lâmpadas foi refeita */
    CONTA_ENVIOS_DA_LUZ,        /* vezes que a luz da hora foi mandada aos shaders */
    CONTA_SOMBRAS_REFEITAS,     /* quadrados do mapa de sombra desenhados de novo */
    TOTAL_CONTAS
} ContaDoQuadro;

void iniciarDesempenho(void);                /* depois de abrir a janela */
void comecarQuadroDeDesempenho(void);        /* no começo de cada quadro */
void etapaDoQuadro(EtapaDoQuadro etapa);     /* daqui em diante, o tempo é desta etapa */
void contarNoQuadro(ContaDoQuadro conta, int quanto);
void terminarQuadroDeDesempenho(void);       /* logo depois de EndDrawing */

/* O gráfico: 0 nada, 1 só o FPS, 2 o gráfico (a opção das configurações); F3 alterna o gráfico */
void desenharDesempenhoNaTela(int modo);     /* antes de EndDrawing */
int graficoDeDesempenhoLigado(int modo);

/* A gravação (o benchmark): cada quadro vira uma linha do arquivo, com a fase em que estava */
void gravarDesempenho(const char *arquivo);
void faseDaGravacao(const char *nome);
void terminarGravacao(void);                 /* escreve o resumo (no log e em <arquivo>.txt) */

#endif
