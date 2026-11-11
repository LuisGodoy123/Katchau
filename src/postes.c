/*
 * postes.c
 * ------------------------------------------------------------------
 * Os postes da rua (veja postes.h).
 * ------------------------------------------------------------------
 */

#include "postes.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "raymath.h"
#include "rlgl.h"
#include "mundo.h"
#include "leitor_json.h"

#define ARQUIVO_DOS_POSTES    "recursos/modelos/postes.glb"
#define ARQUIVO_DOS_TRONCOS   "recursos/modelos/postes.json"
#define MAIS_POSTES           128

static Model modelo;
static int temModelo = 0;
static Matrix *matrizes = NULL;
static Poste postes[MAIS_POSTES];
static int total = 0;


/* Os troncos: { "postes": [ { "x", "z", "y_base", "altura", "raio" } ... ] } */
static void lerTroncos(void) {
    total = 0;
    if (!FileExists(ARQUIVO_DOS_TRONCOS)) {
        return;
    }
    char *texto = LoadFileText(ARQUIVO_DOS_TRONCOS);
    if (texto == NULL) {
        return;
    }
    const char *fim = texto + strlen(texto);
    const char *p = strstr(texto, "\"postes\"");
    p = (p != NULL) ? strchr(p, '[') : NULL;
    while (p != NULL && total < MAIS_POSTES) {
        const char *abre = strchr(p, '{');
        const char *fecha = (abre != NULL) ? strchr(abre, '}') : NULL;
        const char *fimDaLista = strchr(p, ']');
        if (abre == NULL || fecha == NULL || fecha > fim || (fimDaLista != NULL && abre > fimDaLista)) {
            break;
        }
        Poste *t = &postes[total++];
        t->x = numeroDaChave(abre, fecha, "x", 0.0f);
        t->z = numeroDaChave(abre, fecha, "z", 0.0f);
        t->base = numeroDaChave(abre, fecha, "y_base", 0.0f);
        t->topo = t->base + numeroDaChave(abre, fecha, "altura", 9.0f);
        t->raio = numeroDaChave(abre, fecha, "raio", 0.18f);
        p = fecha + 1;
    }
    UnloadFileText(texto);
}

void carregarPostes(void) {
    if (temModelo || !FileExists(ARQUIVO_DOS_POSTES)) {
        return;
    }
    modelo = LoadModel(ARQUIVO_DOS_POSTES);
    if (modelo.meshCount <= 0) {
        return;
    }
    prepararModeloDoMundoAberto(&modelo);   /* o shader do mapa: a luz da hora, a sombra e as lâmpadas */
    matrizes = (Matrix *) malloc((size_t) modelo.meshCount * sizeof(Matrix));
    for (int i = 0; i < modelo.meshCount; i++) {
        matrizes[i] = MatrixIdentity();
    }
    temModelo = 1;
    lerTroncos();
    TraceLog(LOG_INFO, "POSTES: %d malhas, %d troncos", modelo.meshCount, total);
}

void descarregarPostes(void) {
    if (!temModelo) {
        return;
    }
    for (int i = 0; i < modelo.materialCount; i++) {
        modelo.materials[i].shader.id = rlGetShaderIdDefault();   /* o shader é do mapa */
    }
    UnloadModel(modelo);
    free(matrizes);
    matrizes = NULL;
    temModelo = 0;
    total = 0;
}

const Model *modeloDosPostes(void) {
    return temModelo ? &modelo : NULL;
}

const Matrix *matrizesDosPostes(void) {
    return matrizes;
}

int totalDePostes(void) {
    return total;
}

const Poste *poste(int i) {
    return &postes[i];
}
