/*
 * mjpeg.c
 * ------------------------------------------------------------------
 * O leitor de MJPEG e a thread que abre os quadros adiantado. Veja mjpeg.h.
 * ------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "mjpeg.h"

/* O stb_image (domínio público, o mesmo que vem com a raylib) só para abrir JPEG. Ele fica
 * "static" aqui dentro, para não brigar com a cópia que a própria raylib tem. */
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#if defined(__GNUC__)   /* vale para o arquivo todo: o GCC avisa das funções que não usamos só no fim dele */
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif
#include "externo/stb_image.h"

#define PEDACO_DA_LEITURA  (1 << 20)
#define QUADROS_PRONTOS    3   /* quantos quadros a thread deixa adiantados */

/* Um quadro já aberto, esperando a vez de ir para a tela */
typedef struct {
    int numero;                /* o número que só cresce, não o quadro dentro do arquivo */
    unsigned char *pixels;     /* RGB, do stb_image */
    int largura, altura;
} QuadroPronto;

struct Mjpeg {
    /* Só a thread mexe nestes (é quem lê o arquivo) */
    FILE *arquivo;
    unsigned char *buffer;
    long capacidadeDoBuffer;

    /* Lidos pelas duas, mas escritos uma vez só, antes de a thread começar */
    long *inicioDoQuadro;      /* onde cada JPEG começa no arquivo */
    int totalDeQuadros;
    long tamanhoDoArquivo;
    int emLoop;

    /* Combinados entre as duas, sempre com a trava fechada */
    pthread_mutex_t trava;
    pthread_cond_t temEspaco;     /* a thread espera aqui quando a fila está cheia */
    pthread_cond_t temQuadro;     /* quem chama espera aqui pelo primeiro quadro */
    QuadroPronto fila[QUADROS_PRONTOS];
    int quantosProntos;
    int proximoAAbrir;            /* o número do próximo quadro que a thread vai abrir */
    int ultimoPedido;             /* até onde o relógio de quem chama já chegou */
    int acabou;                   /* vídeo sem loop que chegou ao fim */
    int encerrando;               /* fecharMjpeg pediu para a thread sair */

    pthread_t thread;
    int threadViva;
};

/* ==================================================================
 * Ler o arquivo de quadros
 * ================================================================== */

/*
 * indexarQuadros
 * Percorre o arquivo uma vez e anota onde começa cada JPEG (a marca FF D8 FF). Dentro dos dados
 * de um JPEG o byte FF nunca aparece seguido de D8 e FF, então a marca só aparece no começo de
 * cada um.
 * Recebe: o vídeo, com o arquivo aberto.
 * Devolve: 1 se achou algum quadro.
 */
static int indexarQuadros(Mjpeg *video) {
    unsigned char *pedaco = malloc(PEDACO_DA_LEITURA + 2);
    if (pedaco == NULL) {
        return 0;
    }
    int capacidade = 1024;
    video->inicioDoQuadro = malloc(sizeof(long) * capacidade);
    if (video->inicioDoQuadro == NULL) {
        free(pedaco);
        return 0;
    }

    long base = 0;
    size_t sobra = 0;   /* bytes do fim do pedaço anterior: a marca pode ter ficado dividida */
    for (;;) {
        size_t lidos = fread(pedaco + sobra, 1, PEDACO_DA_LEITURA, video->arquivo);
        size_t total = sobra + lidos;
        if (total < 3) {
            break;
        }
        for (size_t i = 0; i + 2 < total; i++) {
            if (pedaco[i] == 0xFF && pedaco[i + 1] == 0xD8 && pedaco[i + 2] == 0xFF) {
                if (video->totalDeQuadros == capacidade) {
                    capacidade *= 2;
                    long *maior = realloc(video->inicioDoQuadro, sizeof(long) * capacidade);
                    if (maior == NULL) {
                        free(pedaco);
                        return video->totalDeQuadros > 0;
                    }
                    video->inicioDoQuadro = maior;
                }
                video->inicioDoQuadro[video->totalDeQuadros++] = base - (long) sobra + (long) i;
            }
        }
        if (lidos == 0) {
            break;
        }
        memmove(pedaco, pedaco + total - 2, 2);   /* os dois últimos bytes voltam para o começo */
        base += (long) lidos;
        sobra = 2;
    }
    free(pedaco);
    video->tamanhoDoArquivo = base;
    return video->totalDeQuadros > 0;
}

/*
 * abrirQuadroDoArquivo
 * Lê um JPEG do arquivo e o abre. Só a thread chama (é quem mexe no arquivo e no buffer).
 * Recebe: o vídeo, o quadro dentro do arquivo e onde guardar a largura e a altura.
 * Devolve: os pixels em RGB (liberar com stbi_image_free), ou NULL.
 */
static unsigned char *abrirQuadroDoArquivo(Mjpeg *video, int quadro, int *largura, int *altura) {
    long inicio = video->inicioDoQuadro[quadro];
    long fim = (quadro + 1 < video->totalDeQuadros) ? video->inicioDoQuadro[quadro + 1]
                                                    : video->tamanhoDoArquivo;
    long tamanho = fim - inicio;
    if (tamanho <= 0) {
        return NULL;
    }
    if (tamanho > video->capacidadeDoBuffer) {
        unsigned char *maior = realloc(video->buffer, tamanho);
        if (maior == NULL) {
            return NULL;
        }
        video->buffer = maior;
        video->capacidadeDoBuffer = tamanho;
    }
    if (fseek(video->arquivo, inicio, SEEK_SET) != 0 ||
        fread(video->buffer, 1, tamanho, video->arquivo) != (size_t) tamanho) {
        return NULL;
    }
    int canais;
    return stbi_load_from_memory(video->buffer, (int) tamanho, largura, altura, &canais, 3);
}

/* ==================================================================
 * A thread que abre os quadros adiantado
 * ================================================================== */

/*
 * trabalhoDaThread
 * Abre um quadro por vez e o põe na fila, até ela encher; aí espera alguém tirar. Depois de cada
 * quadro, o próximo é o maior entre "o seguinte" e "o que o relógio está pedindo": assim, quando
 * a máquina não dá conta, ela pula para o compasso em vez de ficar cada vez mais atrasada, e nunca
 * joga fora um quadro que já custou caro.
 * Recebe: o vídeo.
 * Devolve: NULL (exigência do pthread).
 */
static void *trabalhoDaThread(void *dado) {
    Mjpeg *video = (Mjpeg *) dado;
    for (;;) {
        pthread_mutex_lock(&video->trava);
        while (!video->encerrando && !video->acabou && video->quantosProntos == QUADROS_PRONTOS) {
            pthread_cond_wait(&video->temEspaco, &video->trava);
        }
        if (video->encerrando || video->acabou) {
            pthread_mutex_unlock(&video->trava);
            break;
        }
        int numero = video->proximoAAbrir;
        pthread_mutex_unlock(&video->trava);

        /* A parte cara fica fora da trava: quem chama nunca espera por ela */
        int largura = 0, altura = 0;
        int quadroNoArquivo = numero % video->totalDeQuadros;
        unsigned char *pixels = abrirQuadroDoArquivo(video, quadroNoArquivo, &largura, &altura);

        pthread_mutex_lock(&video->trava);
        if (video->encerrando) {
            pthread_mutex_unlock(&video->trava);
            if (pixels != NULL) {
                stbi_image_free(pixels);
            }
            break;
        }
        if (pixels == NULL) {   /* quadro estragado: segue para o seguinte */
            video->proximoAAbrir = numero + 1;
        } else {
            video->fila[video->quantosProntos].numero = numero;
            video->fila[video->quantosProntos].pixels = pixels;
            video->fila[video->quantosProntos].largura = largura;
            video->fila[video->quantosProntos].altura = altura;
            video->quantosProntos++;
            /* Pula para o compasso, se ficou atrás dele */
            int seguinte = numero + 1;
            video->proximoAAbrir = (video->ultimoPedido > seguinte) ? video->ultimoPedido : seguinte;
            pthread_cond_signal(&video->temQuadro);
        }
        if (!video->emLoop && video->proximoAAbrir >= video->totalDeQuadros) {
            video->acabou = 1;
            pthread_cond_signal(&video->temQuadro);
        }
        pthread_mutex_unlock(&video->trava);
    }
    return NULL;
}

/* ==================================================================
 * Abrir, pegar quadros e fechar
 * ================================================================== */

/*
 * tirarDaFila
 * Tira o primeiro quadro da fila e empurra os outros para trás. A trava já tem de estar fechada.
 * Recebe: o vídeo.
 * Devolve: o quadro que saiu.
 */
static QuadroPronto tirarDaFila(Mjpeg *video) {
    QuadroPronto saiu = video->fila[0];
    for (int i = 1; i < video->quantosProntos; i++) {
        video->fila[i - 1] = video->fila[i];
    }
    video->quantosProntos--;
    return saiu;
}

Mjpeg *abrirMjpeg(const char *arquivo, int emLoop) {
    Mjpeg *video = calloc(1, sizeof(Mjpeg));
    if (video == NULL) {
        return NULL;
    }
    video->emLoop = emLoop;
    video->arquivo = fopen(arquivo, "rb");
    if (video->arquivo == NULL || !indexarQuadros(video)) {
        fecharMjpeg(video);
        return NULL;
    }
    if (pthread_mutex_init(&video->trava, NULL) != 0) {
        fecharMjpeg(video);
        return NULL;
    }
    if (pthread_cond_init(&video->temEspaco, NULL) != 0 ||
        pthread_cond_init(&video->temQuadro, NULL) != 0) {
        pthread_mutex_destroy(&video->trava);
        fecharMjpeg(video);
        return NULL;
    }
    if (pthread_create(&video->thread, NULL, trabalhoDaThread, video) != 0) {
        pthread_cond_destroy(&video->temEspaco);
        pthread_cond_destroy(&video->temQuadro);
        pthread_mutex_destroy(&video->trava);
        fecharMjpeg(video);
        return NULL;
    }
    video->threadViva = 1;
    return video;
}

int totalDeQuadrosMjpeg(const Mjpeg *video) {
    return (video != NULL) ? video->totalDeQuadros : 0;
}

int esperarQuadroMjpeg(Mjpeg *video) {
    if (video == NULL || !video->threadViva) {
        return 0;
    }
    pthread_mutex_lock(&video->trava);
    while (video->quantosProntos == 0 && !video->acabou && !video->encerrando) {
        pthread_cond_wait(&video->temQuadro, &video->trava);
    }
    int tem = (video->quantosProntos > 0);
    pthread_mutex_unlock(&video->trava);
    return tem;
}

unsigned char *pegarQuadroMjpeg(Mjpeg *video, int desejado, int *largura, int *altura) {
    if (video == NULL || !video->threadViva) {
        return NULL;
    }
    pthread_mutex_lock(&video->trava);
    if (desejado > video->ultimoPedido) {
        video->ultimoPedido = desejado;   /* avisa a thread até onde o relógio chegou */
    }

    /* Joga fora os que o relógio já passou, menos o último deles: esse ainda serve de imagem */
    while (video->quantosProntos > 1 && video->fila[1].numero <= desejado) {
        QuadroPronto velho = tirarDaFila(video);
        stbi_image_free(velho.pixels);
        pthread_cond_signal(&video->temEspaco);
    }

    unsigned char *pixels = NULL;
    if (video->quantosProntos > 0 && video->fila[0].numero <= desejado) {
        QuadroPronto escolhido = tirarDaFila(video);
        pixels = escolhido.pixels;
        *largura = escolhido.largura;
        *altura = escolhido.altura;
        pthread_cond_signal(&video->temEspaco);
    }
    pthread_mutex_unlock(&video->trava);
    return pixels;
}

void devolverQuadroMjpeg(Mjpeg *video, unsigned char *pixels) {
    (void) video;
    if (pixels != NULL) {
        stbi_image_free(pixels);
    }
}

void fecharMjpeg(Mjpeg *video) {
    if (video == NULL) {
        return;
    }
    if (video->threadViva) {
        pthread_mutex_lock(&video->trava);
        video->encerrando = 1;
        pthread_cond_broadcast(&video->temEspaco);
        pthread_cond_broadcast(&video->temQuadro);
        pthread_mutex_unlock(&video->trava);
        pthread_join(video->thread, NULL);
        video->threadViva = 0;
        for (int i = 0; i < video->quantosProntos; i++) {
            stbi_image_free(video->fila[i].pixels);
        }
        video->quantosProntos = 0;
        pthread_cond_destroy(&video->temEspaco);
        pthread_cond_destroy(&video->temQuadro);
        pthread_mutex_destroy(&video->trava);
    }
    if (video->arquivo != NULL) {
        fclose(video->arquivo);
    }
    free(video->inicioDoQuadro);
    free(video->buffer);
    free(video);
}
