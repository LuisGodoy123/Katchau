/*
 * vibrar_mac.m
 * ------------------------------------------------------------------
 * A vibração no macOS: é o único arquivo em Objective-C do projeto, porque é a
 * língua do sistema de controles da Apple (GameController e CoreHaptics).
 *
 * Cada mão do controle tem o seu "motor" (um CHHapticEngine), tocando uma vibração
 * contínua. A cada quadro só mudamos a força dela. Usamos o tocador simples, reiniciado
 * antes de o evento acabar: no macOS 27 o tocador "avançado", o que repete sozinho,
 * derruba a conexão com o serviço de vibração (erro 4097).
 * O tremor no gatilho do acelerador existe só no DualSense.
 * ------------------------------------------------------------------
 */

#import <Foundation/Foundation.h>
#import <GameController/GameController.h>
#import <CoreHaptics/CoreHaptics.h>

#include "vibrar.h"

#define SEGUNDOS_ENTRE_ENVIOS   0.025f  /* no máximo 40 vezes por segundo */
#define MUDANCA_MINIMA          0.015f  /* uma diferença menor nem é enviada */
#define NITIDEZ_DO_GRAVE        0.15f   /* de 0 (surdo) a 1 (seco) */
#define NITIDEZ_DO_AGUDO        0.85f
#define DURACAO_DO_EVENTO       30.0f   /* segundos: o máximo de um evento contínuo */
#define FOLGA_PARA_REINICIAR    2.0f    /* tantos segundos antes do fim, começa outro */

#define INICIO_DO_TREMOR        0.05f
#define FREQUENCIA_DA_PATINADA  0.35f

/* Um motor: uma das mãos do controle */
@interface MotorDoControle : NSObject
@property (nonatomic, strong) CHHapticEngine *motor;
@property (nonatomic, strong) id<CHHapticPatternPlayer> tocador;
@property (nonatomic, assign) CFAbsoluteTime inicioDoEvento;
@property (nonatomic, assign) float nitidez;
@property (nonatomic, assign) float ultimaForca;
@end

@implementation MotorDoControle
@end

static GCController *controleAtual = nil;
static MotorDoControle *maoEsquerda = nil, *maoDireita = nil;
static id observadorLigou = nil, observadorDesligou = nil;
static CFTimeInterval ultimoEnvio = 0.0;
static int gatilhoDoAcelerador = -1;   /* em que modo o gatilho está (-1 = não sabemos) */

/*
 * enviarForca
 * Recebe: a mão e a força.
 * Devolve: nada.
 */
static void enviarForca(MotorDoControle *mao, float forca) {
    CHHapticDynamicParameter *parametro = [[CHHapticDynamicParameter alloc] initWithParameterID:CHHapticDynamicParameterIDHapticIntensityControl value:forca relativeTime:0.0];
    NSError *erro = nil;
    if (![mao.tocador sendParameters:@[ parametro ] atTime:CHHapticTimeImmediate error:&erro]) {
        NSLog(@"VIBRACAO: forca nao enviada: %@", erro);
    }
}

/*
 * tocarEvento
 * Começa uma vibração contínua nova, com a força que a mão já estava usando.
 * Recebe: a mão.
 * Devolve: YES se deu certo.
 */
static BOOL tocarEvento(MotorDoControle *mao) {
    NSError *erro = nil;
    CHHapticEventParameter *forca = [[CHHapticEventParameter alloc] initWithParameterID:CHHapticEventParameterIDHapticIntensity value:1.0f];
    CHHapticEventParameter *nitidez = [[CHHapticEventParameter alloc] initWithParameterID:CHHapticEventParameterIDHapticSharpness value:mao.nitidez];
    CHHapticEvent *evento = [[CHHapticEvent alloc] initWithEventType:CHHapticEventTypeHapticContinuous
                                                          parameters:@[ forca, nitidez ]
                                                        relativeTime:0.0
                                                            duration:DURACAO_DO_EVENTO];
    CHHapticPattern *padrao = [[CHHapticPattern alloc] initWithEvents:@[ evento ] parameters:@[] error:&erro];
    if (padrao == nil) {
        NSLog(@"VIBRACAO: padrao invalido: %@", erro);
        return NO;
    }
    id<CHHapticPatternPlayer> novo = [mao.motor createPlayerWithPattern:padrao error:&erro];
    if (novo == nil) {
        NSLog(@"VIBRACAO: tocador invalido: %@", erro);
        return NO;
    }
    id<CHHapticPatternPlayer> antigo = mao.tocador;
    mao.tocador = novo;
    if (![novo startAtTime:CHHapticTimeImmediate error:&erro]) {
        NSLog(@"VIBRACAO: nao tocou: %@", erro);
        return NO;
    }
    enviarForca(mao, mao.ultimaForca);
    [antigo stopAtTime:CHHapticTimeImmediate error:nil];
    mao.inicioDoEvento = CFAbsoluteTimeGetCurrent();
    return YES;
}

/*
 * ligarTocador
 * Liga o motor e começa a tocar a vibração contínua, com força zero.
 * Recebe: o motor.
 * Devolve: YES se deu certo.
 */
static BOOL ligarTocador(MotorDoControle *mao) {
    NSError *erro = nil;
    if (![mao.motor startAndReturnError:&erro]) {
        NSLog(@"VIBRACAO: o motor nao ligou: %@", erro);
        return NO;
    }
    mao.ultimaForca = 0.0f;
    mao.tocador = nil;
    return tocarEvento(mao);
}

/*
 * criarMao
 * Recebe: o controle, de que lado e a nitidez dessa mão.
 * Devolve: a mão pronta, ou nil se o controle não vibra desse lado.
 */
static MotorDoControle *criarMao(GCController *controle, GCHapticsLocality lado, float nitidez) {
    CHHapticEngine *motor = [controle.haptics createEngineWithLocality:lado];
    if (motor == nil) {
        return nil;
    }
    MotorDoControle *mao = [[MotorDoControle alloc] init];
    mao.motor = motor;
    mao.nitidez = nitidez;
    __weak MotorDoControle *fraca = mao;
    motor.resetHandler = ^{      /* o sistema pode reiniciar o motor: toca de novo */
        MotorDoControle *forte = fraca;
        if (forte != nil) {
            ligarTocador(forte);
        }
    };
    motor.stoppedHandler = ^(CHHapticEngineStoppedReason motivo) {
        NSLog(@"VIBRACAO: o motor parou (motivo %ld)", (long) motivo);
    };
    return ligarTocador(mao) ? mao : nil;
}

/*
 * largarControle
 * Solta o controle atual: para os motores e relaxa os gatilhos.
 * Recebe: nada.
 * Devolve: nada.
 */
static void largarControle(void) {
    if ([controleAtual.extendedGamepad isKindOfClass:[GCDualSenseGamepad class]]) {
        GCDualSenseGamepad *dualSense = (GCDualSenseGamepad *) controleAtual.extendedGamepad;
        [dualSense.leftTrigger setModeOff];
        [dualSense.rightTrigger setModeOff];
    }
    [maoEsquerda.motor stopWithCompletionHandler:nil];
    [maoDireita.motor stopWithCompletionHandler:nil];
    maoEsquerda = nil;
    maoDireita = nil;
    controleAtual = nil;
    gatilhoDoAcelerador = -1;
}

/*
 * pegarControle
 * Escolhe um controle que vibre, de preferência o DualSense, e liga as duas mãos.
 * Recebe: nada.
 * Devolve: nada.
 */
static void pegarControle(void) {
    GCController *escolhido = nil;
    for (GCController *controle in [GCController controllers]) {
        if (controle.haptics == nil) {
            continue;
        }
        if (escolhido == nil || [controle.extendedGamepad isKindOfClass:[GCDualSenseGamepad class]]) {
            escolhido = controle;
        }
    }
    if (escolhido == controleAtual) {
        return;
    }
    largarControle();
    if (escolhido == nil) {
        return;
    }
    controleAtual = escolhido;
    if ([escolhido.extendedGamepad isKindOfClass:[GCDualSenseGamepad class]]) {
        /* O gatilho do freio fica sempre livre, mesmo que outro jogo o tenha deixado duro */
        [((GCDualSenseGamepad *) escolhido.extendedGamepad).leftTrigger setModeOff];
    }
    NSSet *lugares = escolhido.haptics.supportedLocalities;
    if ([lugares containsObject:GCHapticsLocalityLeftHandle] && [lugares containsObject:GCHapticsLocalityRightHandle]) {
        maoEsquerda = criarMao(escolhido, GCHapticsLocalityLeftHandle, NITIDEZ_DO_GRAVE);
        maoDireita = criarMao(escolhido, GCHapticsLocalityRightHandle, NITIDEZ_DO_AGUDO);
    } else {
        maoEsquerda = criarMao(escolhido, GCHapticsLocalityDefault, NITIDEZ_DO_GRAVE);   /* um motor só: faz o grave e o agudo */
    }
    NSLog(@"VIBRACAO: %@ (%s)", escolhido.vendorName, (maoEsquerda == nil) ? "SEM VIBRACAO" : (maoDireita != nil) ? "duas maos" : "um motor");
}

void iniciarVibracao(void) {
    NSNotificationCenter *central = [NSNotificationCenter defaultCenter];
    observadorLigou = [central addObserverForName:GCControllerDidConnectNotification object:nil queue:[NSOperationQueue mainQueue]
                                       usingBlock:^(NSNotification *aviso) { (void) aviso; pegarControle(); }];
    observadorDesligou = [central addObserverForName:GCControllerDidDisconnectNotification object:nil queue:[NSOperationQueue mainQueue]
                                          usingBlock:^(NSNotification *aviso) { (void) aviso; pegarControle(); }];
    pegarControle();
}

/*
 * mudarForca
 * Recebe: a mão e a força nova.
 * Devolve: nada.
 */
static void mudarForca(MotorDoControle *mao, float forca) {
    if (mao == nil || mao.tocador == nil) {
        return;
    }
    if (CFAbsoluteTimeGetCurrent() - mao.inicioDoEvento > DURACAO_DO_EVENTO - FOLGA_PARA_REINICIAR) {
        tocarEvento(mao);
    }
    if (fabsf(forca - mao.ultimaForca) < MUDANCA_MINIMA && !(forca == 0.0f && mao.ultimaForca != 0.0f)) {
        return;
    }
    mao.ultimaForca = forca;
    enviarForca(mao, forca);
}

/*
 * ajustarGatilhos
 * O acelerador treme com a roda patinando. Só manda para o controle o que mudou.
 * Recebe: a vibração.
 * Devolve: nada.
 */
static void ajustarGatilhos(const Vibracao *vibracao) {
    if (![controleAtual.extendedGamepad isKindOfClass:[GCDualSenseGamepad class]]) {
        return;
    }
    GCDualSenseGamepad *dualSense = (GCDualSenseGamepad *) controleAtual.extendedGamepad;
    int modoDoAcelerador = (vibracao->noAcelerador > 0.05f) ? 1 : 0;
    if (modoDoAcelerador == 1) {
        [dualSense.rightTrigger setModeVibrationWithStartPosition:INICIO_DO_TREMOR amplitude:vibracao->noAcelerador frequency:FREQUENCIA_DA_PATINADA];
    } else if (modoDoAcelerador != gatilhoDoAcelerador) {
        [dualSense.rightTrigger setModeOff];
    }
    gatilhoDoAcelerador = modoDoAcelerador;
}

void aplicarVibracao(const Vibracao *vibracao) {
    if (controleAtual == nil) {
        return;
    }
    CFTimeInterval agora = CFAbsoluteTimeGetCurrent();
    if (agora - ultimoEnvio < SEGUNDOS_ENTRE_ENVIOS) {
        return;
    }
    ultimoEnvio = agora;
    if (maoDireita != nil) {
        mudarForca(maoEsquerda, vibracao->grave);
        mudarForca(maoDireita, vibracao->agudo);
    } else {
        mudarForca(maoEsquerda, fmaxf(vibracao->grave, vibracao->agudo));
    }
    ajustarGatilhos(vibracao);
}

void pararVibracao(void) {
    if (controleAtual == nil) {
        return;
    }
    mudarForca(maoEsquerda, 0.0f);
    mudarForca(maoDireita, 0.0f);
    Vibracao nada = { 0.0f, 0.0f, 0.0f };
    ajustarGatilhos(&nada);
}

void encerrarVibracao(void) {
    pararVibracao();
    largarControle();
    NSNotificationCenter *central = [NSNotificationCenter defaultCenter];
    if (observadorLigou != nil) {
        [central removeObserver:observadorLigou];
        [central removeObserver:observadorDesligou];
        observadorLigou = observadorDesligou = nil;
    }
}
