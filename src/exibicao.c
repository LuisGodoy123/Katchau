/*
 * exibicao.c
 * ------------------------------------------------------------------
 * O modo de tela, a resolução, a sincronia vertical e o limite de quadros. Veja exibicao.h.
 * ------------------------------------------------------------------
 */

#include <stddef.h>
#include "raylib.h"
#include "exibicao.h"

#ifdef _WIN32
/* Para a tela cheia numa resolução menor que a do monitor, a raylib não tem função: o GLFW que ela
 * traz por dentro tem (glfwSetWindowMonitor). Só estas duas, declaradas aqui para não depender do
 * cabeçalho dele. */
typedef struct GLFWmonitor GLFWmonitor;
typedef struct GLFWwindow GLFWwindow;
extern GLFWmonitor **glfwGetMonitors(int *total);
extern GLFWwindow *glfwGetCurrentContext(void);   /* a janela do GLFW (GetWindowHandle da raylib devolve o HWND do Windows, outra coisa) */
extern void glfwSetWindowMonitor(GLFWwindow *janela, GLFWmonitor *monitor, int x, int y, int largura, int altura, int taxa);
#define GLFW_DONT_CARE_DA_TAXA (-1)
#endif

/* O que já está na tela (para não refazer a janela à toa): -1 = nada ainda */
static int modoAplicado = -1;
static int larguraAplicada = 0;
static int alturaAplicada = 0;

/*
 * tamanhoDoMonitor
 * O tamanho do monitor em pixels, o de verdade: lido uma vez antes de qualquer mudança de modo de
 * vídeo (com a tela cheia numa resolução menor, o monitor passa a dizer a resolução nova).
 * Recebe: onde guardar a largura e a altura.
 * Devolve: nada.
 */
void tamanhoDoMonitor(int *largura, int *altura) {
    static int lido = 0, l = 1920, a = 1080;
    if (!lido && IsWindowReady()) {
        int monitor = GetCurrentMonitor();
        l = GetMonitorWidth(monitor);
        a = GetMonitorHeight(monitor);
        if (l <= 0 || a <= 0) {
            l = 1920;
            a = 1080;
        }
        lido = 1;
    }
    *largura = l;
    *altura = a;
}

/*
 * resolucaoCabeNoMonitor
 * Recebe: o índice da resolução na tabela.
 * Devolve: 1 se ela cabe no monitor.
 */
int resolucaoCabeNoMonitor(int indice) {
    int l, a;
    tamanhoDoMonitor(&l, &a);
    return resolucaoCabe(indice, l, a);
}

/*
 * aplicarDesempenho
 * A sincronia vertical e o limite de quadros: valem na hora, sem mexer na janela.
 * Recebe: as configurações.
 * Devolve: nada.
 */
void aplicarDesempenho(const Configuracoes *config) {
    if (config->sincroniaVertical) {
        SetWindowState(FLAG_VSYNC_HINT);
    } else {
        ClearWindowState(FLAG_VSYNC_HINT);
    }
    SetTargetFPS(quadrosPorSegundoDoLimite(config->limiteDeQuadros));
}

/*
 * sairDosModosDeTela
 * Volta a janela comum (com bordas), para poder mudar de tamanho ou de modo.
 */
static void sairDosModosDeTela(void) {
    if (IsWindowFullscreen()) {
        ToggleFullscreen();
    }
    if (IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE)) {
        ToggleBorderlessWindowed();
    }
}

/*
 * aplicarTela
 * Muda a janela para o modo e a resolução das configurações (se já está assim, não faz nada).
 * Recebe: as configurações.
 * Devolve: nada.
 */
void aplicarTela(const Configuracoes *config) {
    int monitorL, monitorA, l, a;
    tamanhoDoMonitor(&monitorL, &monitorA);
    resolucaoEfetiva(config, monitorL, monitorA, &l, &a);
    if (modoAplicado == config->modoDeTela && larguraAplicada == l && alturaAplicada == a) {
        return;
    }
    modoAplicado = config->modoDeTela;
    larguraAplicada = l;
    alturaAplicada = a;

    sairDosModosDeTela();
    if (config->modoDeTela == MODO_JANELA) {
        SetWindowSize(l, a);
        Vector2 canto = GetMonitorPosition(GetCurrentMonitor());
        SetWindowPosition((int) canto.x + (monitorL - l) / 2, (int) canto.y + (monitorA - a) / 2);
        return;
    }
    if (l == monitorL && a == monitorA) {
        ToggleBorderlessWindowed();   /* a resolução do monitor: janela sem bordas, o Alt+Tab não trava */
        return;
    }
#ifdef _WIN32
    /* Resolução menor: a tela cheia de verdade, que muda o modo de vídeo do monitor */
    ToggleFullscreen();   /* a raylib passa a tratar a janela como tela cheia... */
    int total = 0;
    GLFWmonitor **monitores = glfwGetMonitors(&total);
    int monitor = GetCurrentMonitor();
    if (monitores != NULL && monitor >= 0 && monitor < total) {
        glfwSetWindowMonitor(glfwGetCurrentContext(), monitores[monitor], 0, 0, l, a, GLFW_DONT_CARE_DA_TAXA);   /* ...no tamanho pedido */
    }
#else
    ToggleBorderlessWindowed();   /* fora do Windows, a tela cheia fica sempre no tamanho do monitor */
#endif
}

/*
 * filtroDeTexturaDaConfig
 * O filtro das texturas do jogo, o que o jogador escolheu: bilinear, trilinear ou anisotrópico 4x, 8x ou 16x. Quem carrega
 * uma textura chama isto em vez de fixar um filtro; vale para as texturas carregadas daí em diante (ou seja, ao abrir o jogo).
 * Recebe: nada.
 * Devolve: o TEXTURE_FILTER_* da raylib.
 */
int filtroDeTexturaDaConfig(void) {
    switch (configuracoesAtuais()->filtroDeTexturas) {
        case 0: return TEXTURE_FILTER_BILINEAR;
        case 1: return TEXTURE_FILTER_TRILINEAR;
        case 2: return TEXTURE_FILTER_ANISOTROPIC_4X;
        case 4: return TEXTURE_FILTER_ANISOTROPIC_16X;
        default: return TEXTURE_FILTER_ANISOTROPIC_8X;
    }
}
