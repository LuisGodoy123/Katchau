/*
 * protocolo.c
 * ------------------------------------------------------------------
 * Os pacotes do modo online. Veja protocolo.h.
 * ------------------------------------------------------------------
 */

#include <string.h>
#include <math.h>
#include "protocolo.h"

/* ==================================================================
 * Números no fio
 *
 * Tudo em little-endian (o byte menos significativo primeiro), feito na mão.
 * Não se usa htons/ntohl: eles trocariam para big-endian e espalhariam
 * conversões pelo código. Escrevendo byte a byte, o pacote sai idêntico em
 * qualquer máquina e o código fica num lugar só.
 *
 * O "onde" é um cursor: cada função avança-o pelo que gravou, então escrever um
 * pacote é uma sequência de chamadas sem ninguém somando posições na mão.
 * ================================================================== */

void escreverU8(unsigned char *fio, int *onde, unsigned int valor) {
    fio[(*onde)++] = (unsigned char) (valor & 0xFFu);
}

void escreverU16(unsigned char *fio, int *onde, unsigned int valor) {
    fio[(*onde)++] = (unsigned char) (valor & 0xFFu);
    fio[(*onde)++] = (unsigned char) ((valor >> 8) & 0xFFu);
}

void escreverU32(unsigned char *fio, int *onde, unsigned int valor) {
    fio[(*onde)++] = (unsigned char) (valor & 0xFFu);
    fio[(*onde)++] = (unsigned char) ((valor >> 8) & 0xFFu);
    fio[(*onde)++] = (unsigned char) ((valor >> 16) & 0xFFu);
    fio[(*onde)++] = (unsigned char) ((valor >> 24) & 0xFFu);
}

/*
 * escreverF32
 * Um float no fio pelos seus bits. A união é o jeito certo de olhar os bits de
 * um float em C (um cast de ponteiro quebraria a regra de alinhamento estrito e
 * o -O2 teria licença para reordenar a leitura). Todas as máquinas que rodam o
 * jogo usam IEEE-754 de 32 bits, então os bits significam a mesma coisa nas duas
 * pontas.
 * Recebe: o fio, o cursor (é avançado) e o valor.
 * Devolve: nada.
 */
void escreverF32(unsigned char *fio, int *onde, float valor) {
    union { float f; unsigned int u; } ponte;
    ponte.f = valor;
    escreverU32(fio, onde, ponte.u);
}

/*
 * escreverTexto
 * Um nome de tamanho fixo no fio, sempre com os "quantos" bytes ocupados (o
 * resto em zeros) para o pacote nunca mudar de tamanho.
 * Recebe: o fio, o cursor, o texto e quantos bytes ele ocupa no fio.
 * Devolve: nada.
 */
void escreverTexto(unsigned char *fio, int *onde, const char *texto, int quantos) {
    int acabou = (texto == NULL);
    for (int i = 0; i < quantos; i++) {
        unsigned char letra = 0;
        if (!acabou) {
            if (texto[i] == '\0') {
                acabou = 1;   /* daqui para a frente, só zeros */
            } else {
                /* Só ASCII imprimível vai para a rede: este texto vai ser desenhado
                 * em OUTRA máquina, e um byte estragado viraria lixo na tela dela */
                letra = (texto[i] >= 32 && texto[i] <= 126) ? (unsigned char) texto[i] : (unsigned char) '?';
            }
        }
        fio[(*onde)++] = letra;
    }
}

unsigned int lerU8(const unsigned char *fio, int *onde) {
    return fio[(*onde)++];
}

unsigned int lerU16(const unsigned char *fio, int *onde) {
    unsigned int baixo = fio[(*onde)++];
    unsigned int alto = fio[(*onde)++];
    return baixo | (alto << 8);
}

unsigned int lerU32(const unsigned char *fio, int *onde) {
    unsigned int a = fio[(*onde)++], b = fio[(*onde)++], c = fio[(*onde)++], d = fio[(*onde)++];
    return a | (b << 8) | (c << 16) | (d << 24);
}

float lerF32(const unsigned char *fio, int *onde) {
    union { float f; unsigned int u; } ponte;
    ponte.u = lerU32(fio, onde);
    /* Um pacote estragado pode trazer bits que não são um número (NaN ou
     * infinito). Deixar isso entrar na física contamina tudo o que o carro
     * calcula depois, e o carro desaparece da tela sem nenhum erro aparecer. */
    if (!(ponte.f == ponte.f) || ponte.f > 1e18f || ponte.f < -1e18f) {
        return 0.0f;
    }
    return ponte.f;
}

/*
 * lerTexto
 * Um nome vindo do fio, sempre terminado e sempre imprimível.
 * Recebe: o fio, o cursor, onde guardar e quantos bytes ele ocupa no fio.
 * Devolve: nada.
 */
void lerTexto(const unsigned char *fio, int *onde, char *destino, int quantos) {
    for (int i = 0; i < quantos; i++) {
        unsigned char letra = fio[(*onde)++];
        if (destino != NULL && i < quantos - 1) {
            destino[i] = (letra >= 32 && letra <= 126) ? (char) letra : '\0';
        }
    }
    if (destino != NULL) {
        destino[quantos - 1] = '\0';
    }
}

/* ==================================================================
 * O cabeçalho
 * ================================================================== */

/*
 * escreverCabecalho
 * Põe os 8 bytes de abertura no pacote.
 * Recebe: o fio, o tipo do pacote e a sequência deste lado.
 * Devolve: quantos bytes foram escritos (o cursor para o corpo).
 */
int escreverCabecalho(unsigned char *fio, TipoDePacote tipo, unsigned short sequencia) {
    int onde = 0;
    escreverU32(fio, &onde, MARCA_DO_KATCHAU);
    escreverU8(fio, &onde, VERSAO_DO_PROTOCOLO);
    escreverU8(fio, &onde, (unsigned int) tipo);
    escreverU16(fio, &onde, sequencia);
    return onde;
}

/*
 * lerCabecalho
 * Confere e lê os 8 bytes de abertura.
 *
 * A marca errada é lixo da rede (ou outro programa na mesma porta) e some sem
 * ruído. A VERSÃO errada é outra história: o cabeçalho é congelado justamente
 * para que este caso seja reconhecido, e quem chama mostra "OUTRA VERSÃO" na
 * tela em vez de deixar o jogador olhando uma lista vazia.
 * Recebe: o fio, quantos bytes chegaram e onde guardar o cabeçalho.
 * Devolve: 1 se é um pacote do KATCHAU (de qualquer versão), 0 se é lixo.
 */
int lerCabecalho(const unsigned char *fio, int tamanho, CabecalhoDeRede *cabecalho) {
    if (fio == NULL || cabecalho == NULL || tamanho < BYTES_DO_CABECALHO) {
        return 0;
    }
    int onde = 0;
    cabecalho->marca = lerU32(fio, &onde);
    if (cabecalho->marca != MARCA_DO_KATCHAU) {
        return 0;
    }
    cabecalho->versao = (unsigned char) lerU8(fio, &onde);
    cabecalho->tipo = (unsigned char) lerU8(fio, &onde);
    cabecalho->sequencia = (unsigned short) lerU16(fio, &onde);
    return 1;
}

/*
 * sequenciaEMaisNova
 * O UDP não promete ordem: um pacote pode chegar depois de um mais novo, e
 * aplicá-lo faria o carro andar para trás. A conta é feita com a subtração com
 * sinal de 16 bits, que trata sozinha a volta do contador em 65535: depois de
 * 65535 vem 0, e 0 continua sendo "mais novo" que 65535.
 * Recebe: a sequência que chegou e a última aceita.
 * Devolve: 1 se vale aplicar, 0 se é velha ou repetida.
 */
int sequenciaEMaisNova(unsigned short nova, unsigned short ultima) {
    return (short) (nova - ultima) > 0;
}

/* ==================================================================
 * Os comandos
 * ================================================================== */

unsigned char bitsDosComandos(const Comandos *comandos) {
    unsigned char bits = 0;
    if (comandos == NULL) {
        return 0;
    }
    bits |= comandos->acelerar ? BIT_ACELERAR : 0;
    bits |= comandos->frear ? BIT_FREAR : 0;
    bits |= comandos->esquerda ? BIT_ESQUERDA : 0;
    bits |= comandos->direita ? BIT_DIREITA : 0;
    bits |= comandos->nitro ? BIT_NITRO : 0;
    bits |= comandos->freioDeMao ? BIT_FREIO_DE_MAO : 0;
    return bits;
}

/*
 * direcaoNoFio
 * O analógico do volante, de -1 a +1, num byte com sinal (-100 a +100). Um
 * centésimo de curso é bem mais fino que a mão de qualquer jogador, e economiza
 * 3 bytes por quadro de comando.
 * Recebe: a direção.
 * Devolve: o byte.
 */
signed char direcaoNoFio(float direcao) {
    float limitada = direcao < -1.0f ? -1.0f : (direcao > 1.0f ? 1.0f : direcao);
    return (signed char) lroundf(limitada * 100.0f);
}

Comandos comandosDosBits(unsigned char bits, signed char direcao) {
    Comandos comandos;
    comandos.acelerar = (bits & BIT_ACELERAR) ? 1 : 0;
    comandos.frear = (bits & BIT_FREAR) ? 1 : 0;
    comandos.esquerda = (bits & BIT_ESQUERDA) ? 1 : 0;
    comandos.direita = (bits & BIT_DIREITA) ? 1 : 0;
    comandos.nitro = (bits & BIT_NITRO) ? 1 : 0;
    comandos.freioDeMao = (bits & BIT_FREIO_DE_MAO) ? 1 : 0;
    comandos.direcao = direcao / 100.0f;
    return comandos;
}

/* ==================================================================
 * O carro
 * ================================================================== */

/*
 * limitarU16 / limitarU8
 * Prendem um número quantizado na faixa que o campo do fio aguenta. Sem isto,
 * um valor fora da conta daria a volta (um carro a 20000 un/s viraria um carro
 * parado) e ninguém entenderia por quê.
 */
static unsigned int limitarU16(float valor) {
    if (valor < 0.0f) {
        return 0;
    }
    if (valor > 65535.0f) {
        return 65535;
    }
    return (unsigned int) lroundf(valor);
}

static unsigned int limitarU8(float valor) {
    if (valor < 0.0f) {
        return 0;
    }
    if (valor > 255.0f) {
        return 255;
    }
    return (unsigned int) lroundf(valor);
}

static int limitarI16(float valor) {
    if (valor < -32768.0f) {
        return -32768;
    }
    if (valor > 32767.0f) {
        return 32767;
    }
    return (int) lroundf(valor);
}

/*
 * guardarCarroNoFio
 * Resume um Carro de ~130 campos nos 36 bytes que o outro lado precisa.
 *
 * O que NÃO entra, de propósito, porque cada máquina recalcula igual (ou não
 * muda): o motor e a marcha (atualizarMotor, a partir da velocidade), as rodas
 * girando (rapidezDasRodas), a suspensão (atualizarSuspensao), o rumo da viagem
 * e o desvio do rumo (saem do rumo mais o segmento), a ficha técnica, o nome, o
 * número, os tempos de volta e de setor, e TODO o bloco do que a IA enxerga —
 * no convidado não roda IA nenhuma.
 * Recebe: o carro e onde guardar.
 * Devolve: nada.
 */
void guardarCarroNoFio(const Carro *carro, CarroNoFio *fio) {
    if (carro == NULL || fio == NULL) {
        return;
    }
    fio->indiceDoSegmento = (unsigned short) (carro->posicao.segmento != NULL ? carro->posicao.segmento->indice : 0);
    fio->distanciaNoSegmento = (unsigned short) limitarU16(carro->posicao.distanciaNoSegmento
                                                           / COMPRIMENTO_SEGMENTO * 65535.0f);
    fio->deslocamentoLateral = (short) limitarI16(carro->deslocamentoLateral * 8000.0f);
    fio->velocidade = (unsigned short) limitarU16(carro->velocidade * 4.0f);

    /* O rumo é um ângulo: normalizado em 0..360 e espalhado nos 65536 valores do
     * campo, dá uma precisão de 0,005 grau e nunca "estoura" a faixa */
    float rumo = fmodf(carro->rumoNoMundo, 360.0f);
    if (rumo < 0.0f) {
        rumo += 360.0f;
    }
    fio->rumoNoMundo = (unsigned short) (limitarU16(rumo * (65536.0f / 360.0f)) & 0xFFFFu);
    fio->anguloDeDrift = (short) limitarI16(carro->anguloDeDrift * 100.0f);
    fio->giro = (short) limitarI16(carro->giro * 100.0f);
    fio->volante = (signed char) direcaoNoFio(carro->volante);
    fio->nitro = (unsigned char) limitarU8(carro->nitro * 255.0f);
    fio->derrapagem = (unsigned char) limitarU8(carro->derrapagem * 255.0f);
    fio->forcaDoDrift = (unsigned char) limitarU8(carro->forcaDoDrift * 255.0f);

    unsigned char estado = 0;
    estado |= carro->nitroLigado ? FIO_NITRO_LIGADO : 0;
    estado |= carro->rodando ? FIO_RODANDO : 0;
    estado |= carro->raspandoNoMuro ? FIO_RASPANDO_NO_MURO : 0;
    estado |= carro->ultimosComandos.acelerar ? FIO_ACELERANDO : 0;
    estado |= carro->ultimosComandos.frear ? FIO_FREANDO : 0;
    estado |= carro->pitFeito ? FIO_PIT_FEITO : 0;
    estado |= carro->contatoComCarro ? FIO_CONTATO_DE_CARRO : 0;
    estado |= carro->direcaoLivre ? FIO_DIRECAO_LIVRE : 0;
    fio->estado = estado;

    fio->pit = (unsigned char) (((unsigned int) carro->estadoDoPit & 0x3u)
                                | (((unsigned int) carro->pneuEstourado & 0xFu) << 2)
                                | ((carro->ladoDoPrimeiroEstouro ? 1u : 0u) << 6));
    fio->voltas = (signed char) (carro->voltas < -1 ? -1 : (carro->voltas > 127 ? 127 : carro->voltas));
    fio->desgaste = (unsigned char) limitarU8(carro->desgaste * 255.0f);
    fio->contato = (unsigned char) limitarU8(carro->contato * 255.0f);
    fio->vacuo = (unsigned char) limitarU8(carro->vacuo * 255.0f);
    fio->distanciaPercorrida = carro->distanciaPercorrida;
    fio->tempoDeChegada = carro->tempoDeChegada;
    fio->melhorVolta = carro->melhorVolta;
}

void escreverCarroNoFio(unsigned char *fio, int *onde, const CarroNoFio *carro) {
    escreverU16(fio, onde, carro->indiceDoSegmento);
    escreverU16(fio, onde, carro->distanciaNoSegmento);
    escreverU16(fio, onde, (unsigned int) (unsigned short) carro->deslocamentoLateral);
    escreverU16(fio, onde, carro->velocidade);
    escreverU16(fio, onde, carro->rumoNoMundo);
    escreverU16(fio, onde, (unsigned int) (unsigned short) carro->anguloDeDrift);
    escreverU16(fio, onde, (unsigned int) (unsigned short) carro->giro);
    escreverU8(fio, onde, (unsigned int) (unsigned char) carro->volante);
    escreverU8(fio, onde, carro->nitro);
    escreverU8(fio, onde, carro->derrapagem);
    escreverU8(fio, onde, carro->forcaDoDrift);
    escreverU8(fio, onde, carro->estado);
    escreverU8(fio, onde, carro->pit);
    escreverU8(fio, onde, (unsigned int) (unsigned char) carro->voltas);
    escreverU8(fio, onde, carro->desgaste);
    escreverU8(fio, onde, carro->contato);
    escreverU8(fio, onde, carro->vacuo);
    escreverF32(fio, onde, carro->distanciaPercorrida);
    escreverF32(fio, onde, carro->tempoDeChegada);
    escreverF32(fio, onde, carro->melhorVolta);
}

void lerCarroDoFio(const unsigned char *fio, int *onde, CarroNoFio *carro) {
    carro->indiceDoSegmento = (unsigned short) lerU16(fio, onde);
    carro->distanciaNoSegmento = (unsigned short) lerU16(fio, onde);
    carro->deslocamentoLateral = (short) lerU16(fio, onde);
    carro->velocidade = (unsigned short) lerU16(fio, onde);
    carro->rumoNoMundo = (unsigned short) lerU16(fio, onde);
    carro->anguloDeDrift = (short) lerU16(fio, onde);
    carro->giro = (short) lerU16(fio, onde);
    carro->volante = (signed char) lerU8(fio, onde);
    carro->nitro = (unsigned char) lerU8(fio, onde);
    carro->derrapagem = (unsigned char) lerU8(fio, onde);
    carro->forcaDoDrift = (unsigned char) lerU8(fio, onde);
    carro->estado = (unsigned char) lerU8(fio, onde);
    carro->pit = (unsigned char) lerU8(fio, onde);
    carro->voltas = (signed char) lerU8(fio, onde);
    carro->desgaste = (unsigned char) lerU8(fio, onde);
    carro->contato = (unsigned char) lerU8(fio, onde);
    carro->vacuo = (unsigned char) lerU8(fio, onde);
    carro->distanciaPercorrida = lerF32(fio, onde);
    carro->tempoDeChegada = lerF32(fio, onde);
    carro->melhorVolta = lerF32(fio, onde);
}
