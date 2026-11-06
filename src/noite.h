/*
 * noite.h
 * ------------------------------------------------------------------
 * A noite de corrida da Copa Pistão: os refletores no alto do estádio, que
 * iluminam tudo, a torcida nos assentos, os flashes das câmeras e o céu
 * estrelado. A luz é calculada nos shaders.
 * ------------------------------------------------------------------
 */

#ifndef NOITE_H
#define NOITE_H

#include "raylib.h"

#define ALTURA_DO_REFLETOR 22.0f   /* metros, do pé do refletor até o centro das lâmpadas */

/* O pedaço de GLSL da luz, usado também pelo shader dos carros (carros3d.c): uma
 * luz fraca do céu mais os refletores, cada um pesando conforme a distância e
 * o quanto a superfície está virada para ele. */
#define GLSL_DA_LUZ_DA_NOITE \
    "uniform float noite; uniform int totalLuzes; uniform vec3 luzes[32];\n" \
    "vec3 luzDaNoite(vec3 p, vec3 n) {\n" \
    "    vec3 luz = vec3(0.13, 0.16, 0.23);\n" \
    "    for (int i = 0; i < totalLuzes; i++) {\n" \
    "        vec3 d = luzes[i] - p;\n" \
    "        float quadrado = dot(d, d);\n" \
    "        float virado = max(dot(n, d / sqrt(quadrado)), 0.0) * 0.82 + 0.18;\n" \
    "        luz += vec3(0.80, 0.88, 1.0) * 0.25 * virado / (1.0 + quadrado / 490000.0);\n" \
    "    }\n" \
    "    return luz;\n" \
    "}\n"

void carregarNoite(Model estadio, int materialDosAssentos, float escala);
void descarregarNoite(void);
void registrarShaderIluminado(Shader shader);
int refletoresDaNoite(const Vector3 **lista);
Shader shaderDaNoite(void);
Shader shaderDaTorcida(void);
Texture2D texturaDoCeuNoturno(void);
void prepararLuzes(int ehNoite);
void animarTorcida(float tempoQuadro);
void comecarOndaDaTorcida(float angulo);
float anguloNoEstadio(float x, float z);
void atualizarNoite(float tempoQuadro);
void desenharBrilhos(Camera3D camera);

#endif
