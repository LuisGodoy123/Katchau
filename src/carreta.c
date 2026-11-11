/*
 * carreta.c
 * ------------------------------------------------------------------
 * O lado de dentro da carreta do Mack. Veja carreta.h.
 * ------------------------------------------------------------------
 */

#include "raylib.h"
#include "rlgl.h"
#include "carreta.h"

#define ARQUIVO_DO_INTERIOR   "recursos/modelos/mack_interior.glb"

static Model modelo;
static int carregado = 0;

void carregarInteriorDaCarreta(void) {
    if (carregado || !FileExists(ARQUIVO_DO_INTERIOR)) {
        return;
    }
    modelo = LoadModel(ARQUIVO_DO_INTERIOR);
    if (modelo.meshCount == 0) {
        return;
    }
    /* as texturas (a chapa do piso e os pôsteres), nítidas de longe e de lado */
    for (int i = 0; i < modelo.materialCount; i++) {
        Texture2D *t = &modelo.materials[i].maps[MATERIAL_MAP_ALBEDO].texture;
        if (t->id != 0 && t->id != rlGetTextureIdDefault()) {
            GenTextureMipmaps(t);
            SetTextureFilter(*t, TEXTURE_FILTER_TRILINEAR);
            SetTextureFilter(*t, TEXTURE_FILTER_ANISOTROPIC_8X);
        }
    }
    carregado = 1;
}

void descarregarInteriorDaCarreta(void) {
    if (carregado) {
        UnloadModel(modelo);
        carregado = 0;
    }
}

void desenharInteriorDaCarreta(Matrix deDentro) {
    if (!carregado) {
        return;
    }
    rlDrawRenderBatchActive();
    rlEnableBackfaceCulling();   /* as faces são de um lado só, viradas para dentro do baú */
    for (int i = 0; i < modelo.meshCount; i++) {
        DrawMesh(modelo.meshes[i], modelo.materials[modelo.meshMaterial[i]], deDentro);
    }
    rlDisableBackfaceCulling();   /* como o passeio desenha o resto */
}
