/*
 * controle.c
 * ------------------------------------------------------------------
 * Leitura do controle (gamepad). Veja controle.h.
 * ------------------------------------------------------------------
 */

#include <string.h>
#include "raylib.h"
#include "controle.h"

#define MAXIMO_DE_CONTROLES   4
#define ZONA_MORTA_DIRECAO    0.35f   /* analógico na corrida: menos que isso é "reto" */
#define LIMITE_NO_MENU        0.60f   /* analógico no menu: passou disso, conta um toque... */
#define SOLTURA_NO_MENU       0.30f   /* ...e só conta outro depois de voltar para menos disso */

/* As 4 direções do analógico, na ordem de BOTAO_CIMA a BOTAO_DIREITA */
#define TOTAL_DIRECOES 4

static int controle = -1;                     /* qual controle está em uso (-1 = nenhum) */
static int inclinado[TOTAL_DIRECOES];         /* o analógico está inclinado para esse lado */
static int acabouDeInclinar[TOTAL_DIRECOES];  /* ...e começou a estar neste quadro */

/*
 * atualizarControle
 * Chamar uma vez por quadro, antes de ler o controle. Acha o primeiro
 * controle ligado e transforma o analógico esquerdo em "toques" para os
 * menus: inclinar conta como apertar uma seta uma vez. Para contar de
 * novo é preciso soltar (voltar para perto do centro), senão o cursor
 * dispararia pela lista com o analógico parado inclinado.
 * Recebe: nada.
 * Devolve: nada.
 */
void atualizarControle(void) {
    controle = -1;
    for (int i = 0; i < MAXIMO_DE_CONTROLES; i++) {
        if (IsGamepadAvailable(i)) {
            controle = i;
            break;
        }
    }

    float x = 0.0f, y = 0.0f;
    if (controle >= 0) {
        x = GetGamepadAxisMovement(controle, GAMEPAD_AXIS_LEFT_X);
        y = GetGamepadAxisMovement(controle, GAMEPAD_AXIS_LEFT_Y);
    }
    float inclinacao[TOTAL_DIRECOES] = { -y, y, -x, x };   /* cima, baixo, esquerda, direita */
    for (int d = 0; d < TOTAL_DIRECOES; d++) {
        int agora = inclinado[d] ? (inclinacao[d] > SOLTURA_NO_MENU) : (inclinacao[d] > LIMITE_NO_MENU);
        acabouDeInclinar[d] = agora && !inclinado[d];
        inclinado[d] = agora;
    }
}

/*
 * controleConectado
 * Recebe: nada.
 * Devolve: 1 se há um controle ligado (as dicas da tela mostram os botões dele).
 */
int controleConectado(void) {
    return controle >= 0;
}

/*
 * controleApertou
 * Recebe: o botão.
 * Devolve: 1 se ele foi apertado neste quadro (as direções valem pelo
 *          direcional e pelo analógico), 0 se não ou se não há controle.
 */
int controleApertou(BotaoDoControle botao) {
    static const GamepadButton DIRECIONAL[TOTAL_DIRECOES] = {
        GAMEPAD_BUTTON_LEFT_FACE_UP, GAMEPAD_BUTTON_LEFT_FACE_DOWN,
        GAMEPAD_BUTTON_LEFT_FACE_LEFT, GAMEPAD_BUTTON_LEFT_FACE_RIGHT
    };
    if (controle < 0) {
        return 0;
    }
    switch (botao) {
        case BOTAO_CIMA:
        case BOTAO_BAIXO:
        case BOTAO_ESQUERDA:
        case BOTAO_DIREITA:
            return IsGamepadButtonPressed(controle, DIRECIONAL[botao]) || acabouDeInclinar[botao];
        case BOTAO_CONFIRMAR:
            return IsGamepadButtonPressed(controle, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
        case BOTAO_VOLTAR:
            return IsGamepadButtonPressed(controle, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);
        case BOTAO_START:
            return IsGamepadButtonPressed(controle, GAMEPAD_BUTTON_MIDDLE_RIGHT);
        default:
            return 0;
    }
}

/*
 * somarComandosDoControle
 * Junta o controle aos comandos que já vieram do teclado: o que estiver
 * apertado em qualquer um dos dois vale.
 *   - acelerar: gatilho direito (RT) ou A;
 *   - frear: gatilho esquerdo (LT) ou X;
 *   - virar: analógico esquerdo (passando da zona morta) ou direcional.
 * Recebe: os comandos do quadro (já com o teclado).
 * Devolve: nada (altera os comandos).
 */
void somarComandosDoControle(Comandos *comandos) {
    if (controle < 0) {
        return;
    }
    float x = GetGamepadAxisMovement(controle, GAMEPAD_AXIS_LEFT_X);
    comandos->acelerar |= IsGamepadButtonDown(controle, GAMEPAD_BUTTON_RIGHT_TRIGGER_2) ||
                          IsGamepadButtonDown(controle, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
    comandos->frear |= IsGamepadButtonDown(controle, GAMEPAD_BUTTON_LEFT_TRIGGER_2) ||
                       IsGamepadButtonDown(controle, GAMEPAD_BUTTON_RIGHT_FACE_LEFT);
    comandos->esquerda |= x < -ZONA_MORTA_DIRECAO ||
                          IsGamepadButtonDown(controle, GAMEPAD_BUTTON_LEFT_FACE_LEFT);
    comandos->direita |= x > ZONA_MORTA_DIRECAO ||
                         IsGamepadButtonDown(controle, GAMEPAD_BUTTON_LEFT_FACE_RIGHT);
}

/*
 * nomeNoControle
 * Para as dicas da tela: com um controle ligado, ENTER vira "A" e ESC
 * vira "B". As setas continuam setas (valem para o direcional).
 * Recebe: o nome da tecla, como as dicas usam.
 * Devolve: o nome que deve aparecer.
 */
const char *nomeNoControle(const char *tecla) {
    if (controle < 0) {
        return tecla;
    }
    if (strcmp(tecla, "ENTER") == 0) {
        return "A";
    }
    if (strcmp(tecla, "ESC") == 0) {
        return "B";
    }
    return tecla;
}
