/*
 * noite.h
 * ------------------------------------------------------------------
 * A noite de corrida da Copa Pistão: a luz da arena e dos postes, a torcida
 * nos assentos, os flashes das câmeras, os refletores e o céu estrelado.
 *
 * A luz é calculada nos shaders. Os postes são muitos para todos entrarem
 * neles, então a cada quadro vão só os que estão perto da câmera.
 * ------------------------------------------------------------------
 */

#ifndef NOITE_H
#define NOITE_H

#include "raylib.h"

#define MAXIMO_DE_LUZES_NO_SHADER 32

/* O pedaço de GLSL da luz, usado também pelo shader dos carros (carros3d.c) */
#define GLSL_DA_LUZ_DA_NOITE \
    "uniform float noite; uniform int totalLuzes; uniform vec3 luzes[32];\n" \
    "vec3 luzDaNoite(vec3 p, vec3 n) {\n" \
    "    vec3 luz = vec3(0.54, 0.60, 0.72) * (0.62 + 0.38 * (n.y * 0.5 + 0.5));\n" \
    "    for (int i = 0; i < totalLuzes; i++) {\n" \
    "        vec3 d = luzes[i] - p;\n" \
    "        float cai = 1.0 / (1.0 + dot(d.xz, d.xz) / 260.0);\n" \
    "        luz += vec3(1.0, 0.95, 0.84) * 0.95 * cai * cai;\n" \
    "    }\n" \
    "    return luz;\n" \
    "}\n"

void carregarNoite(Model estadio, int materialDosAssentos, float escala);
void descarregarNoite(void);
void registrarShaderIluminado(Shader shader);
void adicionarLuzDePoste(Vector3 lampada);
Shader shaderDaNoite(void);
Shader shaderDaTorcida(void);
Texture2D texturaDoCeuNoturno(void);
void prepararLuzes(Camera3D camera, int ehNoite);
void atualizarNoite(float tempoQuadro);
void desenharBrilhos(Camera3D camera);

#endif
