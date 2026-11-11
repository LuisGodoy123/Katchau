/*
 * interiores.c
 * ------------------------------------------------------------------
 * O lado de dentro dos prédios do mundo aberto. Veja interiores.h.
 *
 * A luz de cada um vem assada nas cores dos vértices. Quando há também a
 * versão de noite (<interior>_noite.glb, a mesma geometria com a luz de
 * noite: os abajures e as luminárias acesos, sem o sol), as cores dela vão
 * para a mesma malha, como um segundo jogo de cores (no lugar das tangentes,
 * que estes modelos não usam), e um shader mistura as duas pela hora. Sem a
 * versão de noite, a luz de dia é só escurecida.
 * ------------------------------------------------------------------
 */

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "interiores.h"

static const char *ARQUIVOS[] = {
    "recursos/modelos/loja_do_luigi.glb",
    "recursos/modelos/cozy_cone_quartos.glb",
    "recursos/modelos/cozy_cone_recepcao.glb",
};
#define TOTAL_DE_INTERIORES   ((int) (sizeof(ARQUIVOS) / sizeof(ARQUIVOS[0])))
#define DESENHA_ATE           520.0f    /* metros da câmera até a caixa do modelo: mais longe, ele não aparece (de noite,
                                         * as janelas acesas aparecem de longe) */
#define MAIS_MALHAS           64

typedef struct {
    Model modelo;
    BoundingBox caixa;
    int carregado;
    int temNoite;                         /* as cores de noite estão na malha */
    unsigned int coresDaNoite[MAIS_MALHAS];   /* os buffers delas (devolvidos no fim) */
} Interior;

static Interior interiores[TOTAL_DE_INTERIORES];
static Shader shaderDosInteriores;
static int locNoite = -1;
static int shaderCarregado = 0;

/* A cor de dia e a de noite (na tangente), misturadas pela "noite" */
static const char *VERTICES_DOS_INTERIORES =
    "#version 330\n"
    "in vec3 vertexPosition; in vec2 vertexTexCoord; in vec4 vertexColor; in vec4 vertexTangent;\n"
    "uniform mat4 mvp; uniform float noite;\n"
    "out vec2 fragTexCoord; out vec4 fragColor;\n"
    "void main() { fragTexCoord = vertexTexCoord;\n"
    "              fragColor = mix(vertexColor, vertexTangent, noite);\n"
    "              gl_Position = mvp * vec4(vertexPosition, 1.0); }\n";
static const char *PIXELS_DOS_INTERIORES =
    "#version 330\n"
    "in vec2 fragTexCoord; in vec4 fragColor;\n"
    "uniform sampler2D texture0; uniform vec4 colDiffuse;\n"
    "out vec4 finalColor;\n"
    "void main() { finalColor = texture(texture0, fragTexCoord) * colDiffuse * fragColor; }\n";

/* O nome da versão de noite: loja_do_luigi.glb -> loja_do_luigi_noite.glb */
static void nomeDaNoite(const char *arquivo, char *saida, int tamanho) {
    int n = (int) strlen(arquivo) - 4;   /* sem o ".glb" */
    if (n < 0 || n + 11 >= tamanho) {
        saida[0] = '\0';
        return;
    }
    memcpy(saida, arquivo, (size_t) n);
    strcpy(saida + n, "_noite.glb");
}

/*
 * juntarANoite
 * Põe as cores da versão de noite em cada malha do modelo de dia (no atributo da tangente). Só se as
 * duas versões tiverem as mesmas malhas, com o mesmo número de vértices.
 * Recebe: o interior e o caminho da versão de noite.
 * Devolve: nada (marca temNoite).
 */
static void juntarANoite(Interior *in, const char *caminho) {
    if (!FileExists(caminho)) {
        return;
    }
    Model noite = LoadModel(caminho);
    int igual = noite.meshCount == in->modelo.meshCount && noite.meshCount <= MAIS_MALHAS;
    for (int i = 0; igual && i < noite.meshCount; i++) {
        igual = noite.meshes[i].vertexCount == in->modelo.meshes[i].vertexCount && noite.meshes[i].colors != NULL
                && in->modelo.meshes[i].vaoId != 0;
    }
    if (!igual) {
        TraceLog(LOG_WARNING, "INTERIORES: %s não tem as mesmas malhas da versão de dia: fica sem a noite", caminho);
        UnloadModel(noite);
        return;
    }
    for (int i = 0; i < noite.meshCount; i++) {
        const Mesh *m = &noite.meshes[i];
        float *cores = (float *) malloc((size_t) m->vertexCount * 4 * sizeof(float));
        for (int v = 0; v < m->vertexCount * 4; v++) {
            cores[v] = m->colors[v] / 255.0f;
        }
        rlEnableVertexArray(in->modelo.meshes[i].vaoId);
        in->coresDaNoite[i] = rlLoadVertexBuffer(cores, m->vertexCount * 4 * (int) sizeof(float), false);
        rlSetVertexAttribute(RL_DEFAULT_SHADER_ATTRIB_LOCATION_TANGENT, 4, RL_FLOAT, false, 0, 0);
        rlEnableVertexAttribute(RL_DEFAULT_SHADER_ATTRIB_LOCATION_TANGENT);
        rlDisableVertexArray();
        free(cores);
    }
    UnloadModel(noite);
    for (int i = 0; i < in->modelo.materialCount; i++) {
        in->modelo.materials[i].shader = shaderDosInteriores;
    }
    in->temNoite = 1;
    TraceLog(LOG_INFO, "INTERIORES: %s, a luz de noite junto da de dia", caminho);
}

void carregarInteriores(void) {
    if (!shaderCarregado) {
        shaderDosInteriores = LoadShaderFromMemory(VERTICES_DOS_INTERIORES, PIXELS_DOS_INTERIORES);
        locNoite = GetShaderLocation(shaderDosInteriores, "noite");
        shaderCarregado = 1;
    }
    for (int k = 0; k < TOTAL_DE_INTERIORES; k++) {
        Interior *in = &interiores[k];
        if (in->carregado || !FileExists(ARQUIVOS[k])) {
            continue;
        }
        in->modelo = LoadModel(ARQUIVOS[k]);
        if (in->modelo.meshCount == 0) {
            continue;
        }
        /* as texturas, nítidas de longe e de lado (o piso, os quadros) */
        for (int i = 0; i < in->modelo.materialCount; i++) {
            Texture2D *t = &in->modelo.materials[i].maps[MATERIAL_MAP_ALBEDO].texture;
            if (t->id != 0 && t->id != rlGetTextureIdDefault()) {
                GenTextureMipmaps(t);
                SetTextureFilter(*t, TEXTURE_FILTER_TRILINEAR);
                SetTextureFilter(*t, TEXTURE_FILTER_ANISOTROPIC_8X);
            }
        }
        in->caixa = GetModelBoundingBox(in->modelo);
        in->carregado = 1;
        char noite[512];
        nomeDaNoite(ARQUIVOS[k], noite, (int) sizeof(noite));
        juntarANoite(in, noite);
    }
}

void descarregarInteriores(void) {
    for (int k = 0; k < TOTAL_DE_INTERIORES; k++) {
        Interior *in = &interiores[k];
        if (!in->carregado) {
            continue;
        }
        if (in->temNoite) {
            for (int i = 0; i < in->modelo.meshCount; i++) {
                rlUnloadVertexBuffer(in->coresDaNoite[i]);
            }
            for (int i = 0; i < in->modelo.materialCount; i++) {
                in->modelo.materials[i].shader.id = rlGetShaderIdDefault();   /* o shader é de todos */
            }
        }
        UnloadModel(in->modelo);
        in->carregado = 0;
        in->temNoite = 0;
    }
    if (shaderCarregado) {
        UnloadShader(shaderDosInteriores);
        shaderCarregado = 0;
    }
}

void desenharInteriores(Vector3 camera, Color luz, float noite) {
    noite = Clamp(noite, 0.0f, 1.0f);
    if (shaderCarregado) {
        SetShaderValue(shaderDosInteriores, locNoite, &noite, SHADER_UNIFORM_FLOAT);
    }
    for (int k = 0; k < TOTAL_DE_INTERIORES; k++) {
        const Interior *in = &interiores[k];
        if (!in->carregado) {
            continue;
        }
        Vector3 perto = Vector3Clamp(camera, in->caixa.min, in->caixa.max);   /* o ponto da caixa mais perto da câmera */
        if (Vector3Distance(camera, perto) > DESENHA_ATE) {
            continue;
        }
        rlDrawRenderBatchActive();
        DrawModel(in->modelo, (Vector3){ 0.0f, 0.0f, 0.0f }, 1.0f, in->temNoite ? WHITE : luz);
    }
}
