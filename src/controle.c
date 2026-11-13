/*
 * controle.c
 * ------------------------------------------------------------------
 * Leitura do controle (gamepad). Veja controle.h.
 * ------------------------------------------------------------------
 */

#include <string.h>
#include <math.h>
#include "raylib.h"
#include "controle.h"
#include "configuracoes.h"

#define LIMITE_NO_MENU        0.60f   /* analógico no menu: passou disso, conta um toque... */
#define SOLTURA_NO_MENU       0.30f   /* ...e só conta outro depois de voltar para menos disso */

/* As 4 direções do analógico, na ordem de BOTAO_CIMA a BOTAO_DIREITA */
#define TOTAL_DIRECOES 4

/* O touchpad do PlayStation não é um botão "padrão" da raylib (que só conhece os 15 do
 * controle de Xbox). No macOS ele vem do sistema de controles da Apple (vibrar_mac.m),
 * porque a raylib do Homebrew não deixa ler a GLFW. Nos outros sistemas lemos o botão
 * cru da GLFW, que a raylib traz embutida: no DualShock 4 e no DualSense (USB ou
 * Bluetooth) o touchpad é o botão 13. A exceção é o Windows: a raylib do instalador
 * não deixa a GLFW visível (o link falha com "undefined reference to
 * glfwGetJoystickButtons"), então lá o touchpad fica sem leitura e a câmera continua
 * no botão CRIAR/BACK. */
#ifdef __APPLE__
#include "vibrar.h"
#elif !defined(_WIN32)
extern const unsigned char *glfwGetJoystickButtons(int jid, int *count);
#define BOTAO_CRU_DO_TOUCHPAD 13
#endif

static int ehDualSense(void);

static int controle = -1;                     /* qual controle está em uso (-1 = nenhum) */
static int controles[MAXIMO_DE_CONTROLES];    /* todos os ligados, em ordem (a tela dividida reparte) */
static int totalDeControles;
static int touchpadApertado;                  /* o touchpad está apertado */
static int acabouDeApertarOTouchpad;          /* ...e começou a estar neste quadro */
static int inclinado[TOTAL_DIRECOES];         /* o analógico está inclinado para esse lado */
static int acabouDeInclinar[TOTAL_DIRECOES];  /* ...e começou a estar neste quadro */

/*
 * atualizarControle
 * Chamar uma vez por quadro, antes de ler o controle. Lista os controles
 * ligados (a tela dividida reparte um para cada jogador), elege o primeiro
 * deles como o principal e transforma o analógico esquerdo em "toques" para os
 * menus: inclinar conta como apertar uma seta uma vez. Para contar de
 * novo é preciso soltar (voltar para perto do centro), senão o cursor
 * dispararia pela lista com o analógico parado inclinado.
 * Recebe: nada.
 * Devolve: nada.
 */
void atualizarControle(void) {
    totalDeControles = 0;
    for (int i = 0; i < MAXIMO_DE_CONTROLES; i++) {
        if (IsGamepadAvailable(i)) {
            controles[totalDeControles++] = i;
        }
    }
    controle = (totalDeControles > 0) ? controles[0] : -1;   /* o principal: o primeiro ligado */

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

    int touchpad = 0;
    if (ehDualSense()) {   /* nos outros controles o botão 13 é outra coisa */
#ifdef __APPLE__
        touchpad = touchpadDoControleApertado();
#elif !defined(_WIN32)
        int total = 0;
        const unsigned char *botoes = glfwGetJoystickButtons(controle, &total);
        touchpad = botoes != NULL && total > BOTAO_CRU_DO_TOUCHPAD && botoes[BOTAO_CRU_DO_TOUCHPAD];
#endif
    }
    acabouDeApertarOTouchpad = touchpad && !touchpadApertado;
    touchpadApertado = touchpad;
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
        case BOTAO_LIMPAR:
            return IsGamepadButtonPressed(controle, GAMEPAD_BUTTON_RIGHT_FACE_UP);
        case BOTAO_ABA_ANTERIOR:
            return IsGamepadButtonPressed(controle, GAMEPAD_BUTTON_LEFT_TRIGGER_1);
        case BOTAO_ABA_SEGUINTE:
            return IsGamepadButtonPressed(controle, GAMEPAD_BUTTON_RIGHT_TRIGGER_1);
        case BOTAO_CAMERA:
            return acabouDeApertarOTouchpad || IsGamepadButtonPressed(controle, GAMEPAD_BUTTON_MIDDLE_LEFT);
        case BOTAO_CONVERSAR:
            return IsGamepadButtonPressed(controle, GAMEPAD_BUTTON_RIGHT_FACE_LEFT);
        default:
            return 0;
    }
}

/*
 * direcaoDoAnalogico
 * Transforma a inclinação do analógico em quanto girar o volante:
 *   - abaixo da zona morta é 0 (o analógico nunca para exatamente no centro);
 *   - dali até o fim, a inclinação é esticada de 0 a 1, sem pulo na borda
 *     da zona morta;
 *   - e passa por uma curva: perto do centro o volante gira menos que a
 *     inclinação (correções finas na reta), e no fim chega a 1.
 * Recebe: a inclinação do eixo X (-1 a +1).
 * Devolve: a direção, de -1 a +1.
 */
static float direcaoDoAnalogico(float x) {
    static const float PRECISAO_NO_CENTRO[TOTAL_VOLANTES] = { 0.7f, 0.35f, 0.1f };   /* 0 = resposta reta; perto de 1 = bem mais fino no centro */
    const Configuracoes *config = configuracoesAtuais();
    float zonaMorta = config->zonaMorta / 100.0f;
    float precisao = PRECISAO_NO_CENTRO[config->respostaDoVolante];
    float tamanho = fabsf(x);
    if (tamanho < zonaMorta) {
        return 0.0f;
    }
    float esticado = (tamanho - zonaMorta) / (1.0f - zonaMorta);
    if (esticado > 1.0f) {
        esticado = 1.0f;
    }
    float curva = esticado * ((1.0f - precisao) + precisao * esticado);
    return (x < 0.0f) ? -curva : curva;
}

/*
 * qualControle
 * Resolve o controle de uma entrada: QUALQUER_CONTROLE é o principal (o primeiro ligado),
 * que é o que o jogo sempre usou com um jogador só.
 * Recebe: a entrada.
 * Devolve: o índice do controle, ou -1 se este jogador não tem nenhum.
 */
static int qualControle(const EntradaDoJogador *entrada) {
    if (entrada->controle == QUALQUER_CONTROLE) {
        return controle;
    }
    if (entrada->controle < 0 || entrada->controle >= MAXIMO_DE_CONTROLES) {
        return -1;
    }
    return IsGamepadAvailable(entrada->controle) ? entrada->controle : -1;
}

/*
 * controleDoJogador
 * Recebe: a entrada de um jogador.
 * Devolve: o índice do controle dele, ou -1 se ele joga de teclado. A placa de controles da
 *          largada (hud.c) pergunta isto para dizer teclas ou botões.
 */
int controleDoJogador(const EntradaDoJogador *entrada) {
    return qualControle(entrada);
}

/*
 * teclaDaAcao
 * Recebe: as configurações, o comando e a coluna de teclas (TODAS_AS_TECLAS lê as duas,
 *         que é o jogador único; SEM_TECLADO não lê nenhuma).
 * Devolve: 1 se uma das teclas desse comando está apertada.
 */
static int teclaDaAcao(const Configuracoes *config, AcaoDeCorrida acao, int coluna) {
    if (coluna == SEM_TECLADO) {
        return 0;
    }
    int primeira = (coluna == TODAS_AS_TECLAS) ? COLUNA_TECLA_1 : coluna;
    int quantas = (coluna == TODAS_AS_TECLAS) ? 2 : 1;
    for (int c = primeira; c < primeira + quantas; c++) {
        int codigo = config->comandos[acao][c];
        if (codigo != SEM_COMANDO && IsKeyDown(codigo)) {
            return 1;
        }
    }
    return 0;
}

/*
 * botaoDaAcao
 * Recebe: as configurações, o comando e qual controle olhar (-1 = nenhum).
 * Devolve: 1 se um dos dois botões desse comando está apertado.
 */
static int botaoDaAcao(const Configuracoes *config, AcaoDeCorrida acao, int oControle) {
    if (oControle < 0) {
        return 0;
    }
    for (int c = COLUNA_BOTAO_1; c < COLUNA_BOTAO_1 + 2; c++) {
        int codigo = config->comandos[acao][c];
        if (codigo != SEM_COMANDO && IsGamepadButtonDown(oControle, codigo)) {
            return 1;
        }
    }
    return 0;
}

/*
 * lerComandosDoJogador
 * Os comandos de um jogador neste quadro, do teclado e do controle dele: o que estiver
 * apertado em qualquer um dos dois vale. O analógico esquerdo vira o volante, só o quanto
 * está inclinado (comandos->direcao) — e só se este jogador tiver controle, senão o volante
 * de quem joga de teclado seria zerado a cada quadro.
 * Recebe: de onde vêm os comandos dele e onde escrever.
 * Devolve: nada.
 */
void lerComandosDoJogador(const EntradaDoJogador *entrada, Comandos *comandos) {
    const Configuracoes *config = configuracoesAtuais();
    int oControle = qualControle(entrada);
    int coluna = entrada->colunaDeTeclas;
    *comandos = (Comandos) { 0, 0, 0, 0, 0.0f, 0, 0 };
    comandos->acelerar = teclaDaAcao(config, ACAO_ACELERAR, coluna) || botaoDaAcao(config, ACAO_ACELERAR, oControle);
    comandos->frear = teclaDaAcao(config, ACAO_FREAR, coluna) || botaoDaAcao(config, ACAO_FREAR, oControle);
    comandos->esquerda = teclaDaAcao(config, ACAO_ESQUERDA, coluna) || botaoDaAcao(config, ACAO_ESQUERDA, oControle);
    comandos->direita = teclaDaAcao(config, ACAO_DIREITA, coluna) || botaoDaAcao(config, ACAO_DIREITA, oControle);
    comandos->nitro = teclaDaAcao(config, ACAO_NITRO, coluna) || botaoDaAcao(config, ACAO_NITRO, oControle);
    comandos->freioDeMao = teclaDaAcao(config, ACAO_FREIO_DE_MAO, coluna) || botaoDaAcao(config, ACAO_FREIO_DE_MAO, oControle);
    if (oControle >= 0) {
        comandos->direcao = direcaoDoAnalogico(GetGamepadAxisMovement(oControle, GAMEPAD_AXIS_LEFT_X));
    }
}

/*
 * entradaDeTudo
 * Recebe: nada.
 * Devolve: a entrada do jogador único: as duas colunas de teclas (as setas E o WASD) e
 *          qualquer controle ligado, tudo no mesmo carro, como o jogo sempre fez.
 */
EntradaDoJogador entradaDeTudo(void) {
    EntradaDoJogador tudo = { QUALQUER_CONTROLE, TODAS_AS_TECLAS };
    return tudo;
}

/*
 * repartirEntradas
 * Quem dirige com o quê. Com um jogador, ele fica com tudo, como sempre. Com dois (a tela
 * dividida), cada um fica com UMA coluna de teclas e com UM controle:
 *   jogador 1: as setas e o 1º controle ligado;
 *   jogador 2: o WASD e o 2º controle ligado.
 * Quem não recebeu controle joga de teclado, e quem recebeu continua com as teclas dele
 * também: não custa nada e não atrapalha ninguém, porque as colunas são separadas.
 * Recebe: onde escrever as entradas e quantos jogadores.
 * Devolve: nada.
 */
void repartirEntradas(EntradaDoJogador entradas[], int totalDeJogadores) {
    if (totalDeJogadores < 2) {
        entradas[0] = entradaDeTudo();
        return;
    }
    static const int COLUNA_DO_JOGADOR[MAXIMO_DE_JOGADORES_LOCAIS] = { COLUNA_TECLA_1, COLUNA_TECLA_2 };
    for (int j = 0; j < totalDeJogadores && j < MAXIMO_DE_JOGADORES_LOCAIS; j++) {
        entradas[j].colunaDeTeclas = COLUNA_DO_JOGADOR[j];
        entradas[j].controle = (j < totalDeControles) ? controles[j] : SEM_CONTROLE;
    }
}

/*
 * quantosControles
 * Recebe: nada.
 * Devolve: quantos controles estão ligados (a tela do Mack diz quem joga com o quê).
 */
int quantosControles(void) {
    return totalDeControles;
}

/*
 * controleDoJogadorApertou
 * Como controleApertou, mas no controle de um jogador só: na corrida de tela dividida, a
 * pausa e a troca de câmera de cada um saem do controle dele. Sem os "toques" do analógico
 * nem o touchpad, que são do controle principal (os menus são de um jogador).
 * Recebe: a entrada do jogador e o botão.
 * Devolve: 1 se ele foi apertado neste quadro, 0 se não ou se o jogador não tem controle.
 */
int controleDoJogadorApertou(const EntradaDoJogador *entrada, BotaoDoControle botao) {
    int oControle = qualControle(entrada);
    if (oControle < 0) {
        return 0;
    }
    if (entrada->controle == QUALQUER_CONTROLE) {
        return controleApertou(botao);   /* o principal: com os toques do analógico e o touchpad */
    }
    switch (botao) {
        case BOTAO_CONFIRMAR: return IsGamepadButtonPressed(oControle, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
        case BOTAO_VOLTAR:    return IsGamepadButtonPressed(oControle, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);
        case BOTAO_START:     return IsGamepadButtonPressed(oControle, GAMEPAD_BUTTON_MIDDLE_RIGHT);
        case BOTAO_LIMPAR:    return IsGamepadButtonPressed(oControle, GAMEPAD_BUTTON_RIGHT_FACE_UP);
        case BOTAO_CAMERA:    return IsGamepadButtonPressed(oControle, GAMEPAD_BUTTON_MIDDLE_LEFT);
        case BOTAO_CONVERSAR: return IsGamepadButtonPressed(oControle, GAMEPAD_BUTTON_RIGHT_FACE_LEFT);
        default:              return 0;
    }
}

/*
 * lerComandosDoTeclado
 * Recebe: os comandos do quadro, a preencher com as teclas escolhidas pelo jogador (as duas
 *         colunas: as setas e o WASD).
 * Devolve: nada.
 */
void lerComandosDoTeclado(Comandos *comandos) {
    const Configuracoes *config = configuracoesAtuais();
    comandos->acelerar = teclaDaAcao(config, ACAO_ACELERAR, TODAS_AS_TECLAS);
    comandos->frear = teclaDaAcao(config, ACAO_FREAR, TODAS_AS_TECLAS);
    comandos->esquerda = teclaDaAcao(config, ACAO_ESQUERDA, TODAS_AS_TECLAS);
    comandos->direita = teclaDaAcao(config, ACAO_DIREITA, TODAS_AS_TECLAS);
    comandos->nitro = teclaDaAcao(config, ACAO_NITRO, TODAS_AS_TECLAS);
    comandos->freioDeMao = teclaDaAcao(config, ACAO_FREIO_DE_MAO, TODAS_AS_TECLAS);
}

/*
 * somarComandosDoControle
 * Junta o controle principal aos comandos que já vieram do teclado: o que estiver apertado em
 * qualquer um dos dois vale. Os botões são os escolhidos pelo jogador. O analógico esquerdo
 * sempre vira o volante, só o quanto está inclinado (comandos->direcao).
 * Recebe: os comandos do quadro (já com o teclado).
 * Devolve: nada (altera os comandos).
 */
void somarComandosDoControle(Comandos *comandos) {
    if (controle < 0) {
        return;
    }
    const Configuracoes *config = configuracoesAtuais();
    comandos->acelerar |= botaoDaAcao(config, ACAO_ACELERAR, controle);
    comandos->frear |= botaoDaAcao(config, ACAO_FREAR, controle);
    comandos->nitro |= botaoDaAcao(config, ACAO_NITRO, controle);
    comandos->freioDeMao |= botaoDaAcao(config, ACAO_FREIO_DE_MAO, controle);
    comandos->esquerda |= botaoDaAcao(config, ACAO_ESQUERDA, controle);
    comandos->direita |= botaoDaAcao(config, ACAO_DIREITA, controle);
    comandos->direcao = direcaoDoAnalogico(GetGamepadAxisMovement(controle, GAMEPAD_AXIS_LEFT_X));
}

/*
 * teclaApertadaAgora
 * Recebe: nada.
 * Devolve: a tecla apertada neste quadro (0 = nenhuma). Serve para escolher uma tecla.
 */
int teclaApertadaAgora(void) {
    return GetKeyPressed();
}

/*
 * botaoApertadoAgora
 * Procura o botão que acabou de ser apertado. O START não conta: ele é o da pausa.
 * Recebe: nada.
 * Devolve: o botão (0 = nenhum).
 */
int botaoApertadoAgora(void) {
    if (controle < 0) {
        return 0;
    }
    for (int botao = GAMEPAD_BUTTON_LEFT_FACE_UP; botao <= GAMEPAD_BUTTON_RIGHT_THUMB; botao++) {
        if (botao != GAMEPAD_BUTTON_MIDDLE_RIGHT && IsGamepadButtonPressed(controle, botao)) {
            return botao;
        }
    }
    return 0;
}

/*
 * ehDualSense
 * Recebe: nada.
 * Devolve: 1 se o controle ligado é de PlayStation (os botões têm outros nomes).
 */
static int ehDualSense(void) {
    if (controle < 0) {
        return 0;
    }
    const char *nome = GetGamepadName(controle);
    return nome != NULL && (strstr(nome, "DualSense") != NULL || strstr(nome, "DualShock") != NULL || strstr(nome, "PS5") != NULL || strstr(nome, "PS4") != NULL);
}

/*
 * nomeDaTecla
 * Recebe: o código da tecla.
 * Devolve: o nome para as telas. As setas usam os nomes que desenharTecla mostra como seta.
 */
const char *nomeDaTecla(int tecla) {
    if (tecla >= KEY_A && tecla <= KEY_Z) {
        return TextFormat("%c", 'A' + (tecla - KEY_A));
    }
    if (tecla >= KEY_ZERO && tecla <= KEY_NINE) {
        return TextFormat("%d", tecla - KEY_ZERO);
    }
    if (tecla >= KEY_F1 && tecla <= KEY_F12) {
        return TextFormat("F%d", tecla - KEY_F1 + 1);
    }
    if (tecla >= KEY_KP_0 && tecla <= KEY_KP_9) {
        return TextFormat("NUM %d", tecla - KEY_KP_0);
    }
    switch (tecla) {
        case SEM_COMANDO:      return "---";
        case KEY_UP:           return "CIMA";
        case KEY_DOWN:         return "BAIXO";
        case KEY_LEFT:         return "ESQ";
        case KEY_RIGHT:        return "DIR";
        case KEY_SPACE:        return "ESPACO";
        case KEY_LEFT_SHIFT:   return "SHIFT E";
        case KEY_RIGHT_SHIFT:  return "SHIFT D";
        case KEY_LEFT_CONTROL: return "CTRL E";
        case KEY_RIGHT_CONTROL: return "CTRL D";
        case KEY_LEFT_ALT:     return "ALT E";
        case KEY_RIGHT_ALT:    return "ALT D";
        case KEY_LEFT_SUPER:   return "CMD E";
        case KEY_RIGHT_SUPER:  return "CMD D";
        case KEY_ENTER:        return "ENTER";
        case KEY_KP_ENTER:     return "NUM ENTER";
        case KEY_TAB:          return "TAB";
        case KEY_BACKSPACE:    return "APAGAR";
        case KEY_DELETE:       return "DEL";
        case KEY_CAPS_LOCK:    return "CAPS";
        case KEY_COMMA:        return ",";
        case KEY_PERIOD:       return ".";
        /* O teclado brasileiro tem um ponto no bloco numérico também, e ele fica do lado das setas:
         * é uma escolha natural para o nitro do jogador 1, e a placa da largada tem de saber o nome */
        case KEY_KP_DECIMAL:   return "NUM .";
        case KEY_SLASH:        return "/";
        case KEY_SEMICOLON:    return ";";
        case KEY_APOSTROPHE:   return "'";
        case KEY_MINUS:        return "-";
        case KEY_EQUAL:        return "=";
        case KEY_LEFT_BRACKET: return "[";
        case KEY_RIGHT_BRACKET: return "]";
        case KEY_BACKSLASH:    return "\\";
        case KEY_GRAVE:        return "`";
        default:               return TextFormat("TECLA %d", tecla);
    }
}

/*
 * nomeDoBotao
 * Recebe: o código do botão.
 * Devolve: o nome para as telas: os do DualSense, se for ele, senão os do Xbox. O @ na
 *          frente diz a desenharTecla que é um ícone do controle, não uma tecla ("@A" é o
 *          botão A, e "A" é a tecla A).
 */
const char *nomeDoBotao(int botao) {
    int ps = ehDualSense();
    switch (botao) {
        case SEM_COMANDO:                     return "---";
        case GAMEPAD_BUTTON_LEFT_FACE_UP:     return "@DPAD CIMA";
        case GAMEPAD_BUTTON_LEFT_FACE_DOWN:   return "@DPAD BAIXO";
        case GAMEPAD_BUTTON_LEFT_FACE_LEFT:   return "@DPAD ESQ";
        case GAMEPAD_BUTTON_LEFT_FACE_RIGHT:  return "@DPAD DIR";
        case GAMEPAD_BUTTON_RIGHT_FACE_UP:    return ps ? "@TRIANGULO" : "@Y";
        case GAMEPAD_BUTTON_RIGHT_FACE_RIGHT: return ps ? "@BOLA" : "@B";
        case GAMEPAD_BUTTON_RIGHT_FACE_DOWN:  return ps ? "@CRUZ" : "@A";
        case GAMEPAD_BUTTON_RIGHT_FACE_LEFT:  return ps ? "@QUADRADO" : "@X";
        case GAMEPAD_BUTTON_LEFT_TRIGGER_1:   return ps ? "@L1" : "@LB";
        case GAMEPAD_BUTTON_LEFT_TRIGGER_2:   return ps ? "@L2" : "@LT";
        case GAMEPAD_BUTTON_RIGHT_TRIGGER_1:  return ps ? "@R1" : "@RB";
        case GAMEPAD_BUTTON_RIGHT_TRIGGER_2:  return ps ? "@R2" : "@RT";
        case GAMEPAD_BUTTON_MIDDLE_RIGHT:     return ps ? "@OPTIONS" : "@START";
        case GAMEPAD_BUTTON_MIDDLE_LEFT:      return ps ? "@CRIAR" : "@BACK";
        case GAMEPAD_BUTTON_MIDDLE:           return ps ? "@PS" : "@GUIA";
        case GAMEPAD_BUTTON_LEFT_THUMB:       return ps ? "@L3" : "@LS";
        case GAMEPAD_BUTTON_RIGHT_THUMB:      return ps ? "@R3" : "@RS";
        default:                              return TextFormat("BOTAO %d", botao);
    }
}

/*
 * lerAnalogicoDaCamera
 * Recebe: onde guardar a inclinação do analógico direito (y > 0 é para baixo).
 * Devolve: nada (0 e 0 se não há controle).
 */
void lerAnalogicoDaCamera(float *eixoX, float *eixoY) {
    EntradaDoJogador tudo = entradaDeTudo();
    lerAnalogicoDaCameraDoJogador(&tudo, eixoX, eixoY);
}

/*
 * lerAnalogicoDaCameraDoJogador
 * O mesmo, no controle de um jogador só: na tela dividida cada um olha em volta na metade
 * dele sem mexer na câmera do outro.
 * Recebe: a entrada do jogador e onde guardar a inclinação do analógico direito.
 * Devolve: nada (0 e 0 se esse jogador não tem controle).
 */
void lerAnalogicoDaCameraDoJogador(const EntradaDoJogador *entrada, float *eixoX, float *eixoY) {
    const Configuracoes *config = configuracoesAtuais();
    int oControle = qualControle(entrada);
    *eixoX = (oControle >= 0) ? GetGamepadAxisMovement(oControle, GAMEPAD_AXIS_RIGHT_X) : 0.0f;
    *eixoY = (oControle >= 0) ? GetGamepadAxisMovement(oControle, GAMEPAD_AXIS_RIGHT_Y) : 0.0f;
    if (config->inverterCameraX) {
        *eixoX = -*eixoX;
    }
    if (config->inverterCameraY) {
        *eixoY = -*eixoY;
    }
}

/*
 * nomeNoControle
 * Para as dicas da tela: com um controle ligado, ENTER e ESC viram os botões dele
 * (A e B, ou Cruz e Bola), e as setas viram o direcional.
 * Recebe: o nome da tecla, como as dicas usam.
 * Devolve: o nome que deve aparecer.
 */
const char *nomeNoControle(const char *tecla) {
    if (controle < 0) {
        return tecla;
    }
    if (strcmp(tecla, "ENTER") == 0) {
        return nomeDoBotao(GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
    }
    if (strcmp(tecla, "ESC") == 0) {
        return nomeDoBotao(GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);
    }
    if (strcmp(tecla, "CIMA") == 0) {
        return nomeDoBotao(GAMEPAD_BUTTON_LEFT_FACE_UP);
    }
    if (strcmp(tecla, "BAIXO") == 0) {
        return nomeDoBotao(GAMEPAD_BUTTON_LEFT_FACE_DOWN);
    }
    if (strcmp(tecla, "ESQ") == 0) {
        return nomeDoBotao(GAMEPAD_BUTTON_LEFT_FACE_LEFT);
    }
    if (strcmp(tecla, "DIR") == 0) {
        return nomeDoBotao(GAMEPAD_BUTTON_LEFT_FACE_RIGHT);
    }
    if (strcmp(tecla, "ANALOG E") == 0) {
        return "@ANALOG E";
    }
    if (strcmp(tecla, "ANALOG D") == 0) {
        return "@ANALOG D";
    }
    return tecla;
}
