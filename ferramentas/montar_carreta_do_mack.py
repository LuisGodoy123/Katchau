"""
montar_carreta_do_mack.py
------------------------------------------------------------------
Monta o lado de dentro do baú da carreta do Mack, o mais parecido possível com
o do filme Carros: o "camarim" do Relâmpago McQueen à noite, escuro, de
paredes de chapa perfurada e chapa xadrez grafite, riscado de raios de neon
amarelo, com o céu de fibra ótica no teto. Aparece quando a porta de trás desce
e vira rampa.

    Blender -b --factory-startup -P ferramentas/montar_carreta_do_mack.py -- \\
        recursos/modelos/mack_interior.glb [--luzes saida_das_luzes.glb] [--previa pasta]
        [--amostras 1024] [--atlas atlas.png] [--referencias pasta_com_59.png_e_60.png]

Grava dois modelos:
  mack_interior.glb        o lado de dentro, com a luz assada (o jogo o desenha com
                           o shader padrão da raylib, sem luz);
  mack_interior_luzes.glb  (ou o --luzes) só o que brilha: o tubo de neon, as
                           estrelas do teto, o vidro dos spots, os painéis de luz
                           embaixo dos armários e a tela do console, no formato
                           das luzes da cidade (luzes.c): material branco, sem
                           textura, e em COLOR_0 o RGB é a cor acesa (na tela, 0
                           a 1) e o A é a ordem de acender (0 primeiro, 1 por
                           último: uma onda que corre da porta para o fundo, as
                           estrelas espalhadas e a tela por último). Cada peça
                           fica 2 a 2,5 mm à frente da mesma peça no interior
                           (os tubos, 2,5 mm mais grossos), para o jogo desenhá-
                           la por cima, sempre acesa, e no brilho em volta.

--previa grava na pasta as prévias (como no jogo: o interior só com textura x cor
do vértice, a camada das luzes por cima e o brilho em volta), o atlas e as texturas;
--amostras é o número de amostras do Cycles no bake da luz; --atlas usa um atlas já
pintado (o atlas.png de uma prévia), sem renderizar os pôsteres de novo.

O referencial é o do "lado de dentro" que o jogo já usa (deDentro, em passeio.c):
x de uma parede à outra, com 0 no meio (de -1,92 a 1,92 m); y do piso (0) para
cima (até 5,76 m); z da porta (0) para o fundo (até 12 m). O jogo leva esse lado de
dentro até o Mack: ele começa em (dobradiça.x, dobradiça.y + 0,005, dobradiça.z +
0,04) do modelo do Mack, onde a dobradiça é o pé da porta (o meio em x e z e o y
mínimo do mack_porta.glb), com z crescendo para a frente do caminhão.

Não há luz no jogo para esse modelo: ele vai com o shader padrão da raylib
(textura x cor do vértice x cor do material). Então a luz vem assada nas cores dos
vértices (COLOR_0), já no espaço da tela (gama 2,2): nas superfícies com textura,
a cor do vértice é só a luz; nas outras, é a cor do material vezes a luz; nas que
brilham (neon, estrelas, spots, painéis, tela), é a cor delas.

O que tem lá dentro (o corte de cada parede, de baixo para cima):
  - o piso de chapa xadrez preta polida, com o reflexo (falso) do neon, dos
    painéis e da tela, visto de trás da rampa, e a soleira cromada;
  - a parede de baixo, de chapa perfurada grafite com o rodapé preto e o neon do
    rodapé (que contorna as caixas das rodas), e o RAIO: um painel de chapa xadrez
    em forma de raio (o do McQueen, deitado, com a ponta para o fundo) contornado
    de neon. Perto da porta, à esquerda de quem entra, o painel do teclado
    numérico (na cunha, virado para a porta);
  - o armário dos troféus: o fundo inclinado com os painéis de luz amarela (perto
    da porta ele é maior, com os adesivos na ponta virada para fora), a frente
    com o neon e, em cima, a prateleira com os troféus, os carrinhos de
    brinquedo do McQueen, os pôsteres em molduras e as placas redondas;
  - a sanca, com a fileira de spots redondos quentes que iluminam a prateleira;
  - a viga inclinada dos caixotões, cada um emoldurado de neon, e no meio do teto,
    mais alto, o céu preto com as estrelas de fibra ótica (azuis e roxas);
  - na porta, o portal: as pontas dos armários e da viga, com o neon contornando
    o corte (e passando pelo teto);
  - no fundo: o chanfro do teto com um caixotão, o console com a tela azul e as
    duas caixas de som redondas emolduradas em neon, e embaixo o painel cinza com
    o "95" em relevo e as duas bandeiras quadriculadas; nos cantos, raios de neon.
  As caixas das rodas continuam onde estão (elas existem de verdade no Mack): por
  dentro, caixas grafite de chapa perfurada com o tampo de chapa xadrez; por fora
  (pelo arco das rodas), poços escuros. O piso fica livre onde o carro para (da
  porta até uns 7 m, 2,7 m de largura): só as caixas das rodas estão ali.

O que ele faz:
  1. Mede o Mack: a porta (mack_porta.glb) dá a dobradiça, e as rodas da carreta
     (mack_rodas.glb) que sobem acima do piso do baú dão as caixas de roda, como
     desenharCaixasDasRodas em passeio.c: em cada lado, do pneu de dentro (menos 3
     cm) até a parede, 20 cm além das rodas na frente e atrás e 40 cm acima do topo
     do pneu, com a quina de cima do lado de dentro chanfrada.
  2. Pinta o atlas (2048 x 2048, PNG): 8 pôsteres de corrida (Rust-eze,
     Velocidade, Copa Pistão, 95, Katchau, Dinoco 400, Estreante do Ano, Los
     Angeles), com os modelos dos carros do jogo renderizados no EEVEE e os títulos
     em objetos de texto com as fontes do jogo; a tela do console (o rádio da
     estrada, azul); os adesivos (o 95, o raio, a Rust-eze, o McQueen, a Copa
     Pistão, a Dinoco) e a cara do teclado.
  3. Pinta as texturas que se repetem: a chapa xadrez preta (o piso, os raios e o
     tampo das caixas de roda) e a chapa perfurada (as paredes).
  4. Monta a geometria (uma malha para o interior e outra para as luzes).
  5. Assa a luz no Cycles (bake DIFFUSE, direta e indireta, sem a cor) nas cores
     dos vértices: quem ilumina é o próprio neon (os tubos emitem), os painéis, a
     tela, as estrelas e os spots (mais as luzes de spot de cada um), uma luz
     rasante escondida embaixo do console (o relevo do 95) e, fraca, a luz da rua
     entrando pela porta (o teclado e os adesivos da porta). As
     superfícies grandes saem numa grade de uns 12 cm, para o brilho do neon na
     parede ter detalhe. Tira o ruído juntando os cantos de cada vértice e
     normaliza a luz. O ouro e o cromo ganham um brilho falso (visto da câmera de
     trás da rampa), e o piso, o reflexo: de cada vértice dele saem raios na
     direção do espelho (visto de trás da rampa), e a cor do que eles acertam
     (já assada) vira o reflexo.
  6. Junta o interior em quatro materiais (cores, chapa xadrez, chapa perfurada e
     atlas), corta as malhas em pedaços de no máximo MAXIMO_DE_CANTOS cantos (a
     raylib guarda os índices em 16 bits: cada primitiva tem de ter menos de 65.536
     vértices), exporta os dois .glb com +Y para cima e confere, lendo o JSON do
     glTF, que cada primitiva tem COLOR_0 (VEC4) e menos de 65.536 vértices.
  7. Com --previa, abre os .glb gravados junto com o Mack, a porta (aberta, virando
     rampa) e as rodas, de noite, e renderiza como no jogo (o interior só com
     textura x cor do vértice; a camada das luzes por cima, acesa; e o brilho em
     volta, como no posprocesso.c: as luzes reduzidas a 1/4, borradas e somadas).
     As duas vistas do filme (da rampa olhando para dentro, e de dentro olhando
     para cima) saem também lado a lado com as imagens de referência, se
     REFERENCIAS existirem.

No Blender o modelo é montado com x = x do jogo, y do Blender = -z do jogo e z do
Blender = y do jogo; exportado com +Y para cima, ele sai nos eixos do jogo.
------------------------------------------------------------------
"""

import bpy, bmesh, math, os, sys, json, struct, time, tempfile, shutil
import numpy as np
from mathutils import Vector, Matrix
from mathutils.bvhtree import BVHTree

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MODELOS = os.path.join(RAIZ, 'recursos', 'modelos')
CARROS = os.path.join(MODELOS, 'carros')
FONTE_TITULO = os.path.join(RAIZ, 'recursos', 'fontes', 'racing_sans_one.ttf')
FONTE_FORTE = os.path.join(RAIZ, 'recursos', 'fontes', 'saira_condensed_bold.ttf')
FONTE_MEIA = os.path.join(RAIZ, 'recursos', 'fontes', 'saira_condensed_semibold.ttf')
FONTE_CURSIVA = os.path.join(RAIZ, 'recursos', 'fontes', 'lobster.ttf')
if not os.path.exists(FONTE_CURSIVA):
    FONTE_CURSIVA = FONTE_TITULO

argv = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []


def opcao(nome, padrao=None):
    return argv[argv.index(nome) + 1] if nome in argv else padrao


_com_valor = {'--previa', '--amostras', '--atlas', '--luzes', '--referencias'}
_soltos = [a for i, a in enumerate(argv) if not a.startswith('--') and (i == 0 or argv[i - 1] not in _com_valor)]
SAIDA = os.path.abspath(_soltos[0]) if _soltos else os.path.join(MODELOS, 'mack_interior.glb')
SAIDA_DAS_LUZES = os.path.abspath(opcao('--luzes', SAIDA[:-4] + '_luzes.glb' if SAIDA.endswith('.glb') else SAIDA + '_luzes.glb'))
PREVIA = opcao('--previa')
AMOSTRAS = int(opcao('--amostras', 1024))
ATLAS_PRONTO = opcao('--atlas')
# As imagens do filme para comparar com as prévias (a da rampa e a de dentro); --referencias pasta (59.png e 60.png)
REFERENCIAS = opcao('--referencias')
TEMP = tempfile.mkdtemp(prefix='carreta_do_mack_')

# ---- O lado de dentro (metros) ----
MEIA_LARGURA = 1.92        # x de -1,92 a 1,92
ALTURA = 5.76
FUNDO = 12.0
FOLGA_DA_PORTA = (0.005, 0.04)   # o lado de dentro começa 5 mm acima e 4 cm para dentro da dobradiça (passeio.c)
CASCA = 2.06               # x até onde as peças escondidas vão (a lateral da carroceria do Mack fica a 2,075-2,11 m do meio)
CASCA_NAS_RODAS = 2.03     # nas caixas de roda, abaixo de 0,95 m, nada passa daqui (o pneu de fora chega a 2,046)
CASCA_DO_PISO = 2.02       # o lado de baixo do piso
BAIXO_DA_CASCA = -0.43     # y até onde os poços das rodas descem (a saia da carroceria chega a -0,446 m)
BAIXO_DO_BATENTE = -0.28   # e o batente da porta (logo atrás da porta, o fundo da carroceria fica em -0,29 m)
ALTO_DA_CASCA = 5.88       # y até onde elas sobem (o teto da carroceria fica a 5,92 m)
# A quina de cima da carroceria, por dentro, é chanfrada (medida no mack.glb, em |x| e y; a carroceria não fica bem
# no meio do vão da porta, por isso os dois lados diferem). As peças escondidas param a 2,5 cm dela.
QUINA_DA_CARROCERIA = {+1: ((2.112, 5.747), (1.914, 5.917)), -1: ((2.071, 5.747), (1.872, 5.917))}
FOLGA_DA_QUINA = 0.025
ZB = 0.02                  # o plano do portal (o batente e as pontas dos armários e da viga, viradas para a porta)
PASSO = 0.12               # a grade das superfícies grandes, para a luz assada (o brilho do neon) ter detalhe
PASSO_DO_PISO = 0.08       # e a do piso, onde vai o reflexo

MAXIMO_DE_CANTOS = 60000   # cantos de face por pedaço: o exportador não cria mais vértices que isso (a raylib usa
                           # índices de 16 bits: menos de 65.536 vértices por primitiva)

# As rodas e o carro
ACIMA_DO_PNEU = 0.40       # a caixa de roda sobe 40 cm acima do topo do pneu
ALEM_DAS_RODAS = 0.20      # e passa 20 cm das rodas, na frente e atrás
DENTRO_DO_PNEU = 0.03      # e começa 3 cm para dentro do pneu
CHANFRO_DA_CAIXA = 0.10    # a quina de cima, do lado de dentro, é chanfrada (10 cm em x e em y)
CARRO_DE = 1.5             # a traseira do carro do jogador, lá dentro, a 1,5 m da porta
CARRO_ATE = 7.2            # até onde ele vai (o mais comprido tem 5,4 m)

# ---- O corte de cada parede, em (|x|, y): de baixo para cima ----
RODAPE = 0.10
ARMARIO = ((1.92, 2.55), (1.42, 2.82), (1.42, 3.00), (1.92, 3.00))   # o fundo inclinado, a frente e a prateleira
CAIXA_DA_PORTA = 0.95                                                # perto da porta (até aqui), o armário é maior:
ARMARIO_DA_PORTA = ((1.92, 1.80), (1.42, 2.50), (1.42, 3.00), (1.92, 3.00))
PRATELEIRA = 3.00
FRENTE = 1.42              # |x| da frente do armário e da sanca
SANCA = ((1.92, 4.48), (1.42, 4.48), (1.42, 4.80))                   # o fundo da sanca (os spots) e a frente dela
VIGA = ((1.42, 4.80), (0.82, 5.32))                                  # a viga inclinada dos caixotões
MIOLO = 0.82               # |x| da beira do céu das estrelas (o degrau sobe de 5,32 a 5,76)
FIM_DO_CEU = 11.25         # o céu vai até aqui; depois, o chanfro do fundo (de y 5,32 em z 11,25 a y 4,80 em z 12)
CAIXOTOES = 8              # por viga
CAIXOTAO = (1.05, 0.28, 0.40)   # o comprimento, a nervura entre eles e onde o primeiro começa (z)
CAIXOTAO_NA_VIGA = (0.15, 0.86)  # de onde a onde ele vai, na largura da viga (frações, de baixo para cima)
FUNDO_DO_CAIXOTAO = 0.07
SPOTS = (1.67, 0.55, 0.40)  # |x| da fileira de spots, o primeiro (z) e o passo
PAINEIS = (1.12, 0.80, 0.28, 0.22, 0.78)   # os painéis de luz embaixo do armário: o primeiro (z), o comprimento, o vão
                                           # e de onde a onde (frações da largura do fundo inclinado)
RAIO_DO_NEON = 0.009
GROSSURA_DA_LUZ = 0.0025   # o quanto a camada das luzes fica à frente (ou mais grossa)

# O raio (o do McQueen, deitado) nas paredes: o raio clássico de 7 pontas (bx de 0 a 1 na largura, by de 0, a ponta,
# a 1, a cabeça) vai com a cabeça na porta: z = Z0 + (1 - by) * COMPRIMENTO, y = Y0 + bx * ALTURA; fica acima das
# caixas de roda e abaixo do armário
RAIO_CLASSICO = ((0.30, 1.00), (0.75, 1.00), (0.55, 0.58), (0.80, 0.58), (0.25, 0.00), (0.42, 0.44), (0.15, 0.44))
RAIO_NA_PAREDE = (0.95, 10.60, 0.62, 2.28)    # Z0, comprimento, Y0, altura
RAIO_NO_FUNDO = (1.40, 0.46, 0.22, 2.05)      # nos cantos do fundo (em pé, a ponta para baixo): |x| de dentro, largura, y0, altura

# Os pôsteres e o que fica na prateleira (z), iguais nos dois lados
POSTERES_EM_Z = (1.55, 3.75, 5.95, 8.15, 10.35)
POSTER = (0.62, 0.93, 3.14)   # largura, altura e o pé
POSTERES_DA_PAREDE = {+1: (0, 3, 2, 6, 1), -1: (4, 5, 7, 1, 0)}
PLACAS_REDONDAS = {+1: (4.85, 'copa'), -1: (7.05, 'dinoco')}

# O fundo
PAINEL_95 = (1.30, 0.15, 2.62, 0.06)          # meia largura, de y, até y, quanto sai da parede
CONSOLE = (1.32, 2.80, 4.22, 0.14)            # meia largura, de y, até y, profundidade
TELA = (0.63, 3.07, 3.95)                     # meia largura, de y, até y
CAIXAS_DE_SOM = (0.99, 3.51, 0.27)            # |x|, y do meio, raio

# ---- As cores que brilham (no espaço da tela) ----
NEON = (1.0, 0.85, 0.12)
ESTRELAS = ((0.24, 0.50, 1.00), (0.55, 0.38, 1.00), (0.66, 0.80, 1.00))
PESO_DAS_ESTRELAS = (0.62, 0.24, 0.14)
COR_DO_SPOT = (1.0, 0.66, 0.28)          # o anel aceso do spot
COR_DO_VIDRO = (0.62, 0.38, 0.15)        # e o vidro no meio dele, mais fraco
COR_DO_PAINEL = (0.70, 0.62, 0.29)

# As cores (no espaço da tela, 0 a 1) e o jeito de cada acabamento:
#   'cor'    a cor x a luz assada
#   'luz'    só a luz (a textura dá a cor)
#   'metal'  a cor x a luz x o brilho falso
#   'fixo'   a cor, sem luz (o que brilha, e o que fica no escuro de fora)
# O terceiro campo é a textura ('chapa', 'perfurada' ou 'atlas'); o quarto, a força do brilho no bake (0 = não brilha).
ACABAMENTOS = {
    'perfurada':       ('luz', (0.330, 0.330, 0.335), 'perfurada', 0),
    'chapa':           ('luz', (0.360, 0.360, 0.365), 'chapa', 0),
    'grafite':         ('cor', (0.150, 0.150, 0.160), None, 0),
    'preto':           ('cor', (0.055, 0.055, 0.060), None, 0),
    'ceu':             ('cor', (0.030, 0.032, 0.060), None, 0),
    'cinza':           ('cor', (0.480, 0.480, 0.500), None, 0),
    'cinza_escuro':    ('cor', (0.300, 0.300, 0.320), None, 0),
    'bege':            ('cor', (0.620, 0.560, 0.430), None, 0),
    'laca_preta':      ('cor', (0.070, 0.068, 0.072), None, 0),
    'laca_vermelha':   ('cor', (0.820, 0.080, 0.060), None, 0),
    'madeira':         ('cor', (0.340, 0.180, 0.095), None, 0),
    'borracha':        ('cor', (0.090, 0.090, 0.095), None, 0),
    'vidro_escuro':    ('cor', (0.120, 0.160, 0.200), None, 0),
    'amarelo':         ('cor', (1.000, 0.780, 0.080), None, 0),
    'branco':          ('cor', (0.900, 0.890, 0.860), None, 0),
    'ouro':            ('metal', (1.000, 0.780, 0.330), None, 0),
    'ouro_vivo':       ('metal', (1.000, 0.860, 0.450), None, 0),
    'cromo':           ('metal', (0.820, 0.840, 0.880), None, 0),
    'poster':          ('luz', (0.600, 0.500, 0.400), 'atlas', 0),
    'tela':            ('fixo', (1.000, 1.000, 1.000), 'atlas', 9.0),
    'neon':            ('fixo', NEON, None, 90.0),
    'painel':          ('fixo', COR_DO_PAINEL, None, 1.2),
    'spot':            ('fixo', COR_DO_SPOT, None, 30.0),
    'spot_vidro':      ('fixo', COR_DO_VIDRO, None, 10.0),
    'estrela_0':       ('fixo', ESTRELAS[0], None, 25.0),
    'estrela_1':       ('fixo', ESTRELAS[1], None, 25.0),
    'estrela_2':       ('fixo', ESTRELAS[2], None, 25.0),
    'escuro':          ('fixo', (0.040, 0.040, 0.045), None, 0),
}
MATERIAL_DE_SAIDA = {None: 'carreta_cores', 'chapa': 'carreta_chapa', 'perfurada': 'carreta_perfurada', 'atlas': 'carreta_atlas'}

# A luz assada: a luminância no percentil abaixo vira EXPOSICAO (o resto chega a 1 devagar, pelo ombro)
EXPOSICAO = 0.60
PERCENTIL_DA_EXPOSICAO = 97
LUZ_DO_MUNDO = ((0.10, 0.13, 0.22), 0.3)
LUZ_DA_RUA = ((0.86, 0.84, 0.78), 140.0)     # a luz de fora, entrando pela porta (cor e potência)    # a noite lá fora, entrando pela porta (cor e força)

# O reflexo falso do metal (como na versão anterior): pela inclinação da normal (-1 para baixo, 1 para cima), o que
# ele reflete: o chão escuro, as paredes, as luzes do teto e o teto. Isso, mais o brilho das luzes visto da câmera de
# trás e um fio claro na borda, dá um número de 0 a 1 que vira cor pela rampa do metal (no espaço da tela).
REFLEXO_DO_METAL = ((-1.0, -0.50, -0.25, -0.12, 0.05, 0.30, 0.52, 0.75, 1.0),
                    (0.10, 0.12, 0.62, 0.30, 0.22, 0.40, 0.95, 0.55, 0.42))
RAMPA_DO_METAL = {
    'ouro':  ((0.00, 0.20, 0.45, 0.68, 0.86, 1.00),
              ((0.07, 0.035, 0.008), (0.26, 0.150, 0.035), (0.62, 0.400, 0.090), (0.86, 0.630, 0.180), (1.00, 0.850, 0.400),
               (1.00, 0.960, 0.760))),
    'cromo': ((0.00, 0.25, 0.55, 0.80, 1.00),
              ((0.04, 0.045, 0.055), (0.22, 0.24, 0.27), (0.56, 0.59, 0.64), (0.85, 0.87, 0.90), (1.00, 1.00, 1.00))),
}
RAMPA_DO_METAL['ouro_vivo'] = RAMPA_DO_METAL['ouro']
REALCE_DO_METAL = {'ouro': 0.0, 'cromo': 0.0, 'ouro_vivo': 0.25}
# O lustro das superfícies que não são de metal (um brilho largo e fraco, branco, somado à cor): (força, expoente)
LUSTRO = {'laca_preta': (0.035, 40), 'laca_vermelha': (0.050, 40), 'preto': (0.020, 20), 'grafite': (0.020, 20),
          'borracha': (0.020, 10), 'cinza': (0.020, 8)}

# O reflexo do piso: de cada vértice, RAIOS_DO_REFLEXO raios num cone de ASPEREZA (radianos) em volta da direção do
# espelho, vista de CAMERA_DO_REFLEXO; a cor do que eles acertam, vezes o Fresnel (F0 + (1 - F0)(1 - cos)^5) e a
# FORCA_DO_REFLEXO, soma-se à luz do piso (antes baixada para a mediana dela ir a BRILHO_DO_PISO, linear)
CAMERA_DO_REFLEXO = Vector((0.2, 2.0, -7.0))
RAIOS_DO_REFLEXO = 12
ASPEREZA = 0.07
F0_DO_PISO = 0.06
FORCA_DO_REFLEXO = 2.2
BRILHO_DO_PISO = 0.03
MEDIA_DA_TEXTURA = {'chapa': 0.36, 'perfurada': 0.30, 'atlas': 0.45, None: 1.0}

# A câmera do jogo atrás da rampa (cameraDaCena, CENA_SAINDO), no lado de dentro: o brilho falso do metal
CAMERA_DE_TRAS = Vector((0.59, 2.04, -17.04))
ALVO_DE_TRAS = Vector((0.0, 1.60, 4.96))
CAMERA_DO_LADO = Vector((9.24, 3.44, -17.04))   # CENA_ENTRANDO
ALVO_DO_LADO = Vector((0.0, 1.04, -4.54))

# O atlas (pixels, a partir do canto de cima à esquerda)
LADO_DO_ATLAS = 2048
POSTER_L, POSTER_A = 512, 768
CASA_DA_TELA = (0, 1536, 768, 496)               # x, y, largura e altura
ADESIVOS = {'95': (768, 1536, 256, 256), 'raio': (1024, 1536, 256, 256), 'rusteze': (1280, 1536, 512, 256),
            'mcqueen': (1280, 1792, 512, 256), 'copa': (768, 1792, 256, 256), 'dinoco': (1024, 1792, 256, 256),
            'teclado': (1792, 1536, 256, 512)}


def B(x, y=None, z=None):
    """Lado de dentro (x, y para cima, z para o fundo) -> Blender (x, -z, y)"""
    if y is None:
        x, y, z = x
    return Vector((x, -z, y))


def V(x, y, z):
    return Vector((x, y, z))


def linear(c):
    return tuple(max(0.0, v) ** 2.2 for v in c[:3])


def misturar(a, b, t):
    t = np.asarray(t, dtype=np.float32)
    return np.asarray(a, dtype=np.float32) * (1.0 - t[..., None]) + np.asarray(b, dtype=np.float32) * t[..., None]


def suave(a, b, x):
    t = np.clip((x - a) / (b - a), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def borrar(img, raio, passadas=3):
    """Borrão de caixa (separável) repetido: quase gaussiano"""
    out = img.astype(np.float32)
    for _ in range(passadas):
        for eixo in (0, 1):
            c = np.cumsum(np.pad(out, [(raio + 1, raio) if e == eixo else (0, 0) for e in range(out.ndim)], mode='edge'), axis=eixo)
            n = out.shape[eixo]
            out = (np.take(c, np.arange(2 * raio + 1, 2 * raio + 1 + n), axis=eixo) - np.take(c, np.arange(0, n), axis=eixo)) / (2 * raio + 1)
    return out


def imagem_do_blender(nome, rgb, alfa=None):
    """Uma imagem empacotada em PNG a partir de um array (linha 0 em cima), no espaço da tela"""
    a, l = rgb.shape[:2]
    pixels = np.ones((a, l, 4), dtype=np.float32)
    pixels[..., :3] = np.clip(rgb[..., :3], 0.0, 1.0)
    if alfa is not None:
        pixels[..., 3] = np.clip(alfa, 0.0, 1.0)
    img = bpy.data.images.new(nome, l, a, alpha=alfa is not None)
    img.pixels.foreach_set(np.flipud(pixels).ravel())
    img.file_format = 'PNG'
    img.pack()
    return img


def ler_png(caminho):
    img = bpy.data.images.load(caminho)
    l, a = img.size
    p = np.empty(l * a * 4, dtype=np.float32)
    img.pixels.foreach_get(p)
    bpy.data.images.remove(img)
    return np.flipud(p.reshape(a, l, 4)).copy()


def gravar_png(caminho, rgb):
    img = imagem_do_blender(os.path.basename(caminho), rgb)
    img.filepath_raw = caminho
    img.save()
    bpy.data.images.remove(img)


def importar(caminho):
    antes = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=caminho)
    return [o for o in bpy.data.objects if o not in antes]


def no_jogo(p):
    """Blender (importado de um .glb) -> eixos do jogo"""
    return Vector((p.x, p.z, -p.y))


def limites(objetos):
    pts = [o.matrix_world @ v.co for o in objetos if o.type == 'MESH' for v in o.data.vertices]
    mn = Vector([min(p[i] for p in pts) for i in range(3)])
    mx = Vector([max(p[i] for p in pts) for i in range(3)])
    return mn, mx


def ordem_por_z(z):
    """A ordem de acender (o A da camada das luzes): uma onda que corre da porta para o fundo"""
    return float(np.clip(0.04 + 0.78 * z / FUNDO, 0.0, 1.0))


# ======================================================================
# 1) O Mack: a dobradiça e as caixas de roda
# ======================================================================
def medir_o_mack():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    porta = [o for o in importar(os.path.join(MODELOS, 'mack_porta.glb')) if o.type == 'MESH']
    pts = [no_jogo(o.matrix_world @ v.co) for o in porta for v in o.data.vertices]
    mn = Vector([min(p[i] for p in pts) for i in range(3)])
    mx = Vector([max(p[i] for p in pts) for i in range(3)])
    dobradica = Vector(((mn.x + mx.x) / 2, mn.y, (mn.z + mx.z) / 2))
    origem = dobradica + Vector((0.0, FOLGA_DA_PORTA[0], FOLGA_DA_PORTA[1]))
    porta_info = dict(largura=mx.x - mn.x, altura=mx.y - mn.y)
    rodas = [o for o in importar(os.path.join(MODELOS, 'mack_rodas.glb')) if o.type == 'MESH']
    caixas, cilindros = [], []
    for lado in (-1, 1):
        z0, z1, dentro, topo = 1e9, -1e9, 1e9, -1e9
        for r in rodas:
            p = [no_jogo(r.matrix_world @ v.co) - origem for v in r.data.vertices]   # no lado de dentro
            rmn = Vector([min(q[i] for q in p) for i in range(3)])
            rmx = Vector([max(q[i] for q in p) for i in range(3)])
            meio = (rmn + rmx) / 2
            if meio.x * lado <= 0 or meio.z < 0 or meio.z > FUNDO or rmx.y <= 0:
                continue   # do outro lado, fora do baú ou embaixo do piso
            z0, z1 = min(z0, rmn.z), max(z1, rmx.z)
            dentro = min(dentro, min(abs(rmn.x), abs(rmx.x)))
            topo = max(topo, rmx.y)
            # o cilindro da roda (eixo em x), para a conferência
            cilindros.append(dict(x0=rmn.x, x1=rmx.x, y=meio.y, z=meio.z, raio=max(rmx.y - rmn.y, rmx.z - rmn.z) / 2))
        if z1 > z0:
            caixas.append(dict(lado=lado, x=dentro - DENTRO_DO_PNEU, z0=z0 - ALEM_DAS_RODAS, z1=z1 + ALEM_DAS_RODAS,
                               topo=topo + ACIMA_DO_PNEU, pneu=topo))
    # a carroceria (no lado de dentro, nos eixos do Blender), para conferir que nada fura a casca
    mack = [o for o in importar(os.path.join(MODELOS, 'mack.glb')) if o.type == 'MESH']
    desloca = Matrix.Translation(-B(origem))
    verts, polis = [], []
    for o in mack:
        m = desloca @ o.matrix_world
        base = len(verts)
        verts += [m @ v.co for v in o.data.vertices]
        polis += [tuple(base + i for i in p.vertices) for p in o.data.polygons]
    print(f"CARRETA: dobradiça {tuple(round(v, 4) for v in dobradica)}; o lado de dentro começa em {tuple(round(v, 4) for v in origem)}"
          f" (porta de {porta_info['largura']:.3f} x {porta_info['altura']:.3f} m)")
    for c in caixas:
        print(f"CARRETA: caixa de roda do lado {c['lado']:+d}: |x| de {c['x']:.3f} até a parede, z de {c['z0']:.3f} a {c['z1']:.3f},"
              f" y até {c['topo']:.3f} (o pneu chega a {c['pneu']:.3f})")
    return origem, dobradica, porta_info, caixas, dict(cilindros=cilindros, verts=verts, polis=polis)


# ======================================================================
# 2) O atlas: os pôsteres, a tela, os adesivos e o teclado (os carros e as letras renderizados no EEVEE)
# ======================================================================
def preparar_eevee(cena, largura, altura, transparente=True):
    cena.render.engine = 'BLENDER_EEVEE'
    cena.render.resolution_x, cena.render.resolution_y = largura, altura
    cena.render.resolution_percentage = 100
    cena.render.film_transparent = transparente
    cena.render.image_settings.file_format = 'PNG'
    cena.render.image_settings.color_mode = 'RGBA'
    cena.view_settings.view_transform = 'Standard'
    cena.view_settings.look = 'None'
    try:
        cena.eevee.taa_render_samples = 32
    except AttributeError:
        pass


def renderizar(cena, nome):
    caminho = os.path.join(TEMP, nome + '.png')
    cena.render.filepath = caminho
    bpy.ops.render.render(write_still=True)
    return ler_png(caminho)


def foto_do_carro(arquivo, largura, altura, azimute, elevacao, distancia, lente, alvo=(0.0, 0.0, 0.45), rolagem=0.0,
                  desloca=(0.0, 0.0), luz=(35.0, 50.0), forca=4.0, ambiente=0.55, contraluz=(1.0, 0.85, 0.6)):
    """O carro (o comprimento vira 4 unidades, o meio na origem, as rodas em z = 0) visto de uma câmera em volta
    dele: azimute 0 = de frente, positivo para a esquerda do carro; elevação em graus; distância em unidades.
    Devolve RGBA (linha 0 em cima), sem fundo."""
    bpy.ops.wm.read_factory_settings(use_empty=True)
    cena = bpy.context.scene
    objetos = importar(os.path.join(CARROS, arquivo))
    mn, mx = limites(objetos)
    escala = 4.0 / (mx.y - mn.y)
    raiz = bpy.data.objects.new('raiz', None)
    cena.collection.objects.link(raiz)
    for o in objetos:
        if o.parent is None:
            o.parent = raiz
    raiz.matrix_world = Matrix.Scale(escala, 4) @ Matrix.Translation(-Vector(((mn.x + mx.x) / 2, (mn.y + mx.y) / 2, mn.z)))
    alvo = Vector(alvo)
    a, e = math.radians(azimute), math.radians(elevacao)
    cam = bpy.data.objects.new('camera', bpy.data.cameras.new('camera'))
    cena.collection.objects.link(cam)
    cena.camera = cam
    cam.data.lens = lente
    cam.data.shift_x, cam.data.shift_y = desloca
    cam.location = alvo + distancia * Vector((-math.sin(a) * math.cos(e), -math.cos(a) * math.cos(e), math.sin(e)))
    olhar = (alvo - cam.location).to_track_quat('-Z', 'Y')
    cam.rotation_euler = (olhar @ Matrix.Rotation(math.radians(rolagem), 4, 'Z').to_quaternion()).to_euler()
    sol = bpy.data.objects.new('sol', bpy.data.lights.new('sol', 'SUN'))
    cena.collection.objects.link(sol)
    sol.data.energy = forca
    sol.data.angle = math.radians(8)
    sol.rotation_euler = (math.radians(luz[1]), 0.0, math.radians(luz[0]) + math.pi)
    contra = bpy.data.objects.new('contraluz', bpy.data.lights.new('contraluz', 'SUN'))
    cena.collection.objects.link(contra)
    contra.data.energy = forca * 0.8
    contra.data.color = contraluz
    contra.rotation_euler = (math.radians(70), 0.0, a + math.radians(160))
    mundo = bpy.data.worlds.new('mundo')
    cena.world = mundo
    mundo.use_nodes = True
    mundo.node_tree.nodes['Background'].inputs[0].default_value = (0.75, 0.80, 0.90, 1.0)
    mundo.node_tree.nodes['Background'].inputs[1].default_value = ambiente
    preparar_eevee(cena, largura, altura)
    return renderizar(cena, 'carro_' + arquivo)


def textos(largura, altura, linhas):
    """As letras de uma imagem, sem fundo. Cada linha: (texto, fonte, tamanho, cor, x, y, opções), com o tamanho e
    a posição em frações da largura (y de cima para baixo). Opções: contorno=(cor, espessura), sombra=(cor, dx, dy),
    inclina (itálico falso), espaco (entre letras), max (largura máxima), alinha ('CENTER', 'LEFT', 'RIGHT'), gira (graus)."""
    bpy.ops.wm.read_factory_settings(use_empty=True)
    cena = bpy.context.scene
    proporcao = altura / largura
    cam = bpy.data.objects.new('camera', bpy.data.cameras.new('camera'))
    cena.collection.objects.link(cam)
    cena.camera = cam
    cam.data.type = 'ORTHO'
    cam.data.sensor_fit = 'VERTICAL'
    cam.data.ortho_scale = proporcao
    cam.location = (0.5, proporcao / 2, 10.0)
    fontes = {}
    materiais = {}

    def material(cor):
        chave = tuple(round(c, 3) for c in cor)
        if chave not in materiais:
            m = bpy.data.materials.new('letra')
            m.use_nodes = True
            nt = m.node_tree
            nt.nodes.clear()
            em = nt.nodes.new('ShaderNodeEmission')
            em.inputs[0].default_value = (*linear(cor), 1.0)
            out = nt.nodes.new('ShaderNodeOutputMaterial')
            nt.links.new(em.outputs[0], out.inputs[0])
            materiais[chave] = m
        return materiais[chave]

    def letra(texto, fonte, tamanho, cor, x, y, z, op, grossura=0.0, dx=0.0, dy=0.0):
        if fonte not in fontes:
            fontes[fonte] = bpy.data.fonts.load(fonte)
        curva = bpy.data.curves.new('texto', 'FONT')
        curva.body = texto
        curva.font = fontes[fonte]
        curva.size = tamanho
        curva.align_x = op.get('alinha', 'CENTER')
        curva.align_y = 'CENTER'
        curva.shear = op.get('inclina', 0.0)
        curva.space_character = op.get('espaco', 1.0)
        curva.offset = grossura
        curva.resolution_u = 6
        o = bpy.data.objects.new('texto', curva)
        cena.collection.objects.link(o)
        o.data.materials.append(material(cor))
        o.location = (x + dx, proporcao - y + dy, z)
        o.rotation_euler = (0.0, 0.0, math.radians(op.get('gira', 0.0)))
        bpy.context.view_layer.update()
        if 'max' in op and o.dimensions.x > op['max']:
            o.scale = (op['max'] / o.dimensions.x,) * 3
        return o

    for (texto, fonte, tamanho, cor, x, y, op) in linhas:
        letra(texto, fonte, tamanho, cor, x, y, 0.0, op)
        if 'contorno' in op:
            letra(texto, fonte, tamanho, op['contorno'][0], x, y, -0.01, op, grossura=op['contorno'][1])
        if 'sombra' in op:
            c, dx, dy = op['sombra']
            letra(texto, fonte, tamanho, c, x, y, -0.02, op, grossura=op.get('contorno', (None, 0.0))[1], dx=dx, dy=-dy)
    preparar_eevee(cena, largura, altura)
    return renderizar(cena, 'textos')


# ---- Os fundos (espaço da tela, linha 0 em cima) ----
def grade_de_pixels(largura, altura):
    y, x = np.mgrid[0:altura, 0:largura].astype(np.float32)
    return (x + 0.5) / largura, (y + 0.5) / largura   # em frações da largura


def fundo_de_raios(largura, altura, cor1, cor2, centro, raios=28, giro=0.0):
    x, y = grade_de_pixels(largura, altura)
    ang = np.arctan2(y - centro[1], x - centro[0]) + giro
    t = (np.floor((ang + np.pi) / (2 * np.pi) * raios) % 2).astype(np.float32)
    return misturar(cor1, cor2, t)


def degrade(largura, altura, cor_de_cima, cor_de_baixo, de=0.0, ate=1.0):
    _, y = grade_de_pixels(largura, altura)
    t = suave(de, ate, y / (altura / largura))
    return misturar(cor_de_cima, cor_de_baixo, t)


def disco(img, largura, altura, centro, raio, cor, borda=0.004):
    x, y = grade_de_pixels(largura, altura)
    d = np.hypot(x - centro[0], y - centro[1])
    return misturar(img, cor, 1.0 - suave(raio - borda, raio + borda, d))


def elipse(img, largura, altura, centro, semi, cor, borda=0.004):
    x, y = grade_de_pixels(largura, altura)
    d = np.hypot((x - centro[0]) / semi[0], (y - centro[1]) / semi[1])
    return misturar(img, cor, 1.0 - suave(1.0 - borda / semi[1], 1.0 + borda / semi[1], d))


def retangulo(img, largura, altura, x0, y0, x1, y1, cor, raio=0.0):
    """Um retângulo (de cantos redondos, com raio) em frações da largura"""
    x, y = grade_de_pixels(largura, altura)
    cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
    hx, hy = (x1 - x0) / 2 - raio, (y1 - y0) / 2 - raio
    d = np.hypot(np.maximum(np.abs(x - cx) - hx, 0.0), np.maximum(np.abs(y - cy) - hy, 0.0)) - raio
    return misturar(img, cor, 1.0 - suave(-0.0015, 0.0015, d))


def poligono_na_imagem(img, largura, altura, pontos, cor, borda=0.0):
    """Um polígono (em frações da largura, y de cima para baixo), com a borda suavizada; borda > 0 engorda ele"""
    x, y = grade_de_pixels(largura, altura)
    dentro = np.zeros(x.shape, dtype=bool)
    dist = np.full(x.shape, 1e9, dtype=np.float32)
    n = len(pontos)
    for i in range(n):
        (ax, ay), (bx, by) = pontos[i], pontos[(i + 1) % n]
        cruza = ((ay > y) != (by > y)) & (x < (bx - ax) * (y - ay) / (by - ay + 1e-12) + ax)
        dentro ^= cruza
        ex, ey = bx - ax, by - ay
        t = np.clip(((x - ax) * ex + (y - ay) * ey) / (ex * ex + ey * ey), 0.0, 1.0)
        dist = np.minimum(dist, np.hypot(x - ax - t * ex, y - ay - t * ey))
    d = np.where(dentro, -dist, dist) - borda
    return misturar(img, cor, 1.0 - suave(-0.0015, 0.0015, d))


def faixas(img, largura, altura, cores, angulo, de, largura_da_faixa):
    """Faixas paralelas inclinadas (a primeira começa em 'de', medido na direção perpendicular a elas)"""
    x, y = grade_de_pixels(largura, altura)
    a = math.radians(angulo)
    d = x * math.sin(a) + y * math.cos(a) - de
    for i, cor in enumerate(cores):
        dentro = suave(-0.002, 0.002, d - i * largura_da_faixa) * (1.0 - suave(-0.002, 0.002, d - (i + 1) * largura_da_faixa))
        img = misturar(img, cor, dentro)
    return img


def xadrez(img, largura, altura, de, ate, tamanho, cor1=(0.08, 0.07, 0.07), cor2=(0.95, 0.92, 0.84)):
    x, y = grade_de_pixels(largura, altura)
    casa = ((np.floor(x / tamanho) + np.floor(y / tamanho)) % 2).astype(np.float32)
    dentro = (y >= de) & (y < ate)
    return np.where(dentro[..., None], misturar(cor1, cor2, casa), img)


def riscos_de_velocidade(img, largura, altura, cor, de, ate, quantos, semente, forca=0.6):
    """Riscos horizontais (o carro passando rápido)"""
    rng = np.random.default_rng(semente)
    x, y = grade_de_pixels(largura, altura)
    for _ in range(quantos):
        yy = rng.uniform(de, ate)
        esp = rng.uniform(0.002, 0.008)
        x0 = rng.uniform(-0.2, 0.7)
        comp = rng.uniform(0.3, 0.9)
        t = (1.0 - suave(esp * 0.5, esp, np.abs(y - yy))) * suave(x0, x0 + 0.1, x) * (1.0 - suave(x0 + comp - 0.2, x0 + comp, x))
        img = misturar(img, cor, t * rng.uniform(0.3, 1.0) * forca)
    return img


def por_cima(fundo, rgba, sombra=None):
    """O RGBA por cima do fundo; com sombra (cor, dx, dy, raio, força), uma sombra borrada antes"""
    if sombra is not None:
        cor, dx, dy, raio, forca = sombra
        alfa = np.roll(np.roll(rgba[..., 3], dy, axis=0), dx, axis=1)
        alfa = borrar(alfa, raio)
        fundo = misturar(fundo, cor, alfa * forca)
    return fundo * (1.0 - rgba[..., 3:4]) + rgba[..., :3] * rgba[..., 3:4]


def impresso(img, semente, margem=0.022, cor_da_margem=(0.94, 0.93, 0.90)):
    """A cara de pôster impresso (brilhante, novo): um pouco de vinheta, manchinhas suaves e a margem branca"""
    a, l = img.shape[:2]
    rng = np.random.default_rng(semente)
    img = np.clip(img, 0.0, 1.0)
    mancha = borrar(rng.normal(0.0, 1.0, ((a + 7) // 8, (l + 7) // 8)).astype(np.float32), 3)
    mancha = np.kron(mancha, np.ones((8, 8), dtype=np.float32))[:a, :l]
    mancha = (mancha - mancha.mean()) / (mancha.std() + 1e-6)
    img = img * (1.0 + 0.02 * mancha[..., None])
    x, y = grade_de_pixels(l, a)
    d = np.hypot((x - 0.5) / 0.5, (y - a / l / 2) / (a / l / 2))
    img = img * (1.0 - 0.14 * suave(0.70, 1.45, d))[..., None]
    m = int(round(margem * l))
    if m > 0:
        borda = np.ones((a, l), dtype=bool)
        borda[m:a - m, m:l - m] = False
        img = np.where(borda[..., None], np.asarray(cor_da_margem, dtype=np.float32), img)
    return np.clip(img, 0.0, 1.0)


# As cores das imagens
VERMELHO_P = (0.82, 0.08, 0.06)
VERMELHO_ESCURO_P = (0.42, 0.04, 0.04)
AMARELO_P = (0.99, 0.80, 0.16)
LARANJA_P = (0.96, 0.48, 0.10)
CREME_P = (0.97, 0.92, 0.78)
PRETO_P = (0.06, 0.05, 0.05)
AZUL_P = (0.10, 0.36, 0.80)
AZUL_ESCURO_P = (0.04, 0.12, 0.36)
BRANCO_P = (0.98, 0.97, 0.94)
OURO_P = (1.00, 0.80, 0.28)
SOMBRA_P = ((0.0, 0.0, 0.0), 8, 12, 6, 0.55)


def poster_rusteze(L, A):
    img = degrade(L, A, (0.99, 0.86, 0.50), (0.93, 0.60, 0.22))
    img = fundo_de_raios(L, A, (0.99, 0.82, 0.42), (1.0, 0.90, 0.56), (0.5, 0.92), raios=34) * 0.5 + img * 0.5
    img = disco(img, L, A, (0.5, 0.80), 0.40, (0.62, 0.22, 0.07))
    img = disco(img, L, A, (0.5, 0.80), 0.375, (0.86, 0.40, 0.12))
    carro = foto_do_carro('mcqueen.glb', L, A, azimute=-62, elevacao=7, distancia=8.4, lente=40, desloca=(0.0, 0.0), luz=(60, 40))
    img = por_cima(img, carro, ((0.30, 0.10, 0.03), 6, 12, 5, 0.5))
    letras = textos(L, A, [
        ('Rust-eze', FONTE_CURSIVA, 0.26, (0.82, 0.14, 0.05), 0.5, 0.17,
         dict(contorno=(CREME_P, 0.014), sombra=((0.40, 0.12, 0.04), 0.012, 0.012), max=0.90)),
        ('POMADA MEDICINAL', FONTE_FORTE, 0.085, (0.42, 0.14, 0.05), 0.5, 0.34, dict(espaco=1.1, max=0.86)),
        ('PARA PARA-CHOQUES', FONTE_FORTE, 0.085, (0.42, 0.14, 0.05), 0.5, 0.43, dict(espaco=1.1, max=0.86)),
        ('"FERRUGEM? NUNCA MAIS!"', FONTE_MEIA, 0.062, (0.55, 0.18, 0.05), 0.5, 1.33, dict(max=0.86)),
        ('O PATROCINADOR DO 95', FONTE_MEIA, 0.050, (0.42, 0.14, 0.05), 0.5, 1.41, dict(espaco=1.2, max=0.8)),
    ])
    return impresso(por_cima(img, letras), 1)


def poster_velocidade(L, A):
    img = degrade(L, A, (0.05, 0.04, 0.07), (0.34, 0.03, 0.04))
    img = riscos_de_velocidade(img, L, A, (1.0, 0.72, 0.15), 0.50, 1.20, 46, 21, forca=0.55)
    img = faixas(img, L, A, [AMARELO_P, LARANJA_P, VERMELHO_P], -28, 1.16, 0.05)
    carro = foto_do_carro('mcqueen.glb', L, A, azimute=-24, elevacao=3, distancia=5.8, lente=26, rolagem=-10, desloca=(0.0, -0.02),
                          contraluz=(1.0, 0.5, 0.2))
    img = por_cima(img, carro, SOMBRA_P)
    letras = textos(L, A, [
        ('VELOCIDADE.', FONTE_TITULO, 0.17, AMARELO_P, 0.5, 0.15, dict(contorno=(VERMELHO_P, 0.012), sombra=(PRETO_P, 0.01, 0.01),
                                                                         max=0.92, inclina=0.12)),
        ('EU SOU', FONTE_FORTE, 0.075, BRANCO_P, 0.5, 0.285, dict(espaco=1.3, max=0.5)),
        ('VELOCIDADE.', FONTE_TITULO, 0.12, BRANCO_P, 0.5, 0.385, dict(contorno=(VERMELHO_ESCURO_P, 0.01), max=0.8, inclina=0.12)),
        ('RELÂMPAGO McQUEEN  ·  95', FONTE_FORTE, 0.062, AMARELO_P, 0.5, 1.10, dict(espaco=1.1, max=0.86, contorno=(PRETO_P, 0.006))),
    ])
    return impresso(por_cima(img, letras), 2)


def poster_copa(L, A):
    img = degrade(L, A, (0.08, 0.12, 0.32), (0.02, 0.03, 0.10))
    img = fundo_de_raios(L, A, (0.08, 0.12, 0.32), (0.12, 0.18, 0.42), (0.5, -0.15), raios=22) * 0.6 + img * 0.4
    img = xadrez(img, L, A, 1.20, 1.50, 0.0625)
    rei = foto_do_carro('the_king.glb', L, A, azimute=-78, elevacao=6, distancia=10.5, lente=40, desloca=(-0.16, 0.07),
                        contraluz=(0.6, 0.8, 1.0))
    img = por_cima(img, rei, SOMBRA_P)
    carro = foto_do_carro('mcqueen.glb', L, A, azimute=-78, elevacao=6, distancia=8.0, lente=40, desloca=(0.08, -0.05),
                          contraluz=(1.0, 0.8, 0.5))
    img = por_cima(img, carro, SOMBRA_P)
    letras = textos(L, A, [
        ('COPA', FONTE_TITULO, 0.22, OURO_P, 0.5, 0.15, dict(contorno=((0.45, 0.24, 0.04), 0.014), sombra=(PRETO_P, 0.012, 0.012),
                                                             max=0.8, inclina=0.1)),
        ('PISTÃO', FONTE_TITULO, 0.22, OURO_P, 0.5, 0.33, dict(contorno=((0.45, 0.24, 0.04), 0.014), sombra=(PRETO_P, 0.012, 0.012),
                                                               max=0.86, inclina=0.1)),
        ('A GRANDE DECISÃO', FONTE_FORTE, 0.07, BRANCO_P, 0.5, 1.11, dict(espaco=1.2, max=0.86, contorno=(PRETO_P, 0.006))),
    ])
    return impresso(por_cima(img, letras), 3)


def poster_95(L, A):
    img = fundo_de_raios(L, A, (0.80, 0.08, 0.06), (0.90, 0.16, 0.08), (0.5, 0.60), raios=30)
    img = poligono_na_imagem(img, L, A, [(0.62, 0.05), (0.30, 0.62), (0.52, 0.62), (0.36, 1.10), (0.78, 0.48), (0.55, 0.48),
                                         (0.72, 0.05)], (0.98, 0.70, 0.12))
    noventa = textos(L, A, [('95', FONTE_TITULO, 0.60, AMARELO_P, 0.5, 0.52, dict(contorno=(VERMELHO_ESCURO_P, 0.022),
                                                                                  sombra=(PRETO_P, 0.02, 0.02), max=0.95, inclina=0.14))])
    img = por_cima(img, noventa)
    carro = foto_do_carro('mcqueen.glb', L, A, azimute=-90, elevacao=4, distancia=8.4, lente=40, desloca=(0.0, 0.12))
    img = por_cima(img, carro, SOMBRA_P)
    letras = textos(L, A, [
        ('RELÂMPAGO McQUEEN', FONTE_FORTE, 0.088, BRANCO_P, 0.5, 1.36, dict(espaco=1.05, max=0.88, contorno=(VERMELHO_ESCURO_P, 0.008))),
    ])
    return impresso(por_cima(img, letras), 4)


def poster_katchau(L, A):
    img = degrade(L, A, (0.10, 0.06, 0.08), (0.30, 0.04, 0.05))
    img = faixas(img, L, A, [AMARELO_P, LARANJA_P, VERMELHO_P], -35, 0.82, 0.075)
    carro = foto_do_carro('mcqueen.glb', L, A, azimute=-16, elevacao=5, distancia=5.4, lente=28, rolagem=-14, desloca=(0.0, -0.06),
                          contraluz=(1.0, 0.6, 0.3))
    img = por_cima(img, carro, ((0.0, 0.0, 0.0), 6, 10, 6, 0.6))
    letras = textos(L, A, [
        ('KATCHAU!', FONTE_TITULO, 0.25, AMARELO_P, 0.5, 0.17, dict(contorno=(VERMELHO_P, 0.016), sombra=(PRETO_P, 0.014, 0.014),
                                                                     max=0.92, inclina=0.12, gira=6)),
        ('RELÂMPAGO McQUEEN', FONTE_FORTE, 0.10, BRANCO_P, 0.5, 0.36, dict(espaco=1.1, max=0.86)),
        ('O MAIS RÁPIDO DA PISTA', FONTE_MEIA, 0.058, AMARELO_P, 0.5, 0.45, dict(espaco=1.1, max=0.84)),
    ])
    return impresso(por_cima(img, letras), 5)


def poster_dinoco(L, A):
    img = degrade(L, A, (0.32, 0.64, 0.96), (0.05, 0.24, 0.62))
    img = faixas(img, L, A, [BRANCO_P, (0.99, 0.80, 0.18), BRANCO_P], 22, 0.95, 0.04)
    rei = foto_do_carro('the_king.glb', L, A, azimute=32, elevacao=10, distancia=8.4, lente=36, rolagem=8, desloca=(0.0, 0.04))
    img = por_cima(img, rei, ((0.02, 0.06, 0.16), -10, 14, 6, 0.55))
    letras = textos(L, A, [
        ('DINOCO', FONTE_TITULO, 0.24, BRANCO_P, 0.5, 0.16, dict(contorno=(AZUL_ESCURO_P, 0.014), sombra=(PRETO_P, 0.012, 0.012),
                                                                 max=0.9, inclina=0.1)),
        ('400', FONTE_TITULO, 0.21, (0.99, 0.82, 0.20), 0.5, 0.34, dict(contorno=(AZUL_ESCURO_P, 0.014), max=0.6, inclina=0.1)),
        ('O REI  ·  43', FONTE_FORTE, 0.072, BRANCO_P, 0.5, 1.33, dict(espaco=1.2, max=0.86, contorno=(AZUL_ESCURO_P, 0.006))),
        ('COPA PISTÃO', FONTE_MEIA, 0.056, (0.99, 0.86, 0.32), 0.5, 1.41, dict(espaco=1.2, max=0.8)),
    ])
    return impresso(por_cima(img, letras), 6)


def poster_estreante(L, A):
    img = degrade(L, A, (0.14, 0.11, 0.08), (0.04, 0.03, 0.03))
    img = fundo_de_raios(L, A, (0.14, 0.11, 0.08), (0.20, 0.15, 0.09), (0.5, 0.80), raios=36) * 0.6 + img * 0.4
    img = disco(img, L, A, (0.5, 0.80), 0.43, (0.86, 0.64, 0.20))
    img = disco(img, L, A, (0.5, 0.80), 0.41, (0.16, 0.12, 0.08))
    carro = foto_do_carro('mcqueen.glb', L, A, azimute=-8, elevacao=9, distancia=7.6, lente=40, desloca=(0.0, 0.0),
                          contraluz=(1.0, 0.8, 0.45))
    img = por_cima(img, carro, SOMBRA_P)
    letras = textos(L, A, [
        ('ESTREANTE', FONTE_TITULO, 0.17, OURO_P, 0.5, 0.14, dict(contorno=((0.40, 0.20, 0.03), 0.012), max=0.92, inclina=0.1)),
        ('DO ANO', FONTE_TITULO, 0.16, BRANCO_P, 0.5, 0.29, dict(contorno=((0.40, 0.20, 0.03), 0.012), max=0.7, inclina=0.1)),
        ('RELÂMPAGO McQUEEN', FONTE_FORTE, 0.075, OURO_P, 0.5, 1.30, dict(espaco=1.1, max=0.86)),
        ('COPA PISTÃO  ·  95', FONTE_MEIA, 0.052, BRANCO_P, 0.5, 1.39, dict(espaco=1.2, max=0.8)),
    ])
    return impresso(por_cima(img, letras), 7)


def poster_los_angeles(L, A):
    img = degrade(L, A, (0.99, 0.66, 0.24), (0.42, 0.10, 0.36), 0.0, 1.05)
    img = disco(img, L, A, (0.5, 0.86), 0.30, (1.0, 0.88, 0.50))
    for i, y in enumerate((0.80, 0.86, 0.92, 0.98)):
        img = faixas(img, L, A, [(0.42 + 0.06 * i, 0.10, 0.36)], 0, y, 0.012 + 0.004 * i)
    img = faixas(img, L, A, [(0.18, 0.05, 0.18)], 0, 1.04, 0.50)
    rei = foto_do_carro('the_king.glb', L, A, azimute=-48, elevacao=7, distancia=11.0, lente=38, desloca=(-0.14, 0.06))
    img = por_cima(img, rei, SOMBRA_P)
    carro = foto_do_carro('mcqueen.glb', L, A, azimute=-48, elevacao=7, distancia=8.2, lente=38, desloca=(0.06, -0.04))
    img = por_cima(img, carro, SOMBRA_P)
    letras = textos(L, A, [
        ('LOS ANGELES', FONTE_TITULO, 0.15, BRANCO_P, 0.5, 0.14, dict(contorno=((0.45, 0.08, 0.30), 0.012), max=0.92, inclina=0.1)),
        ('A GRANDE FINAL', FONTE_FORTE, 0.085, (1.0, 0.92, 0.55), 0.5, 0.28, dict(espaco=1.15, max=0.86)),
        ('O DESEMPATE DA COPA PISTÃO', FONTE_FORTE, 0.056, BRANCO_P, 0.5, 1.36, dict(max=0.88)),
    ])
    return impresso(por_cima(img, letras), 8)


POSTERES = [poster_rusteze, poster_velocidade, poster_copa, poster_95, poster_katchau, poster_dinoco, poster_estreante,
            poster_los_angeles]


def casa_do_poster(i):
    """Onde fica o pôster i no atlas: x, y, largura e altura (3 x 2 casas, mais 2 na coluna da direita)"""
    if i < 6:
        linha, coluna = divmod(i, 3)
        return coluna * POSTER_L, linha * POSTER_A, POSTER_L, POSTER_A
    return 1536, (i - 6) * POSTER_A, POSTER_L, POSTER_A


def imagem_da_tela(L, A):
    """A tela do console: o rádio da estrada (azul, de cristal líquido), com a faixa tocando, o equalizador e o 95"""
    x, y = grade_de_pixels(L, A)
    a = A / L
    img = degrade(L, A, (0.04, 0.16, 0.66), (0.02, 0.08, 0.40))
    img = retangulo(img, L, A, 0.02, 0.02, 0.98, 0.085, (0.10, 0.34, 0.92), raio=0.012)
    img = retangulo(img, L, A, 0.025, 0.11, 0.66, 0.30, (0.03, 0.12, 0.52), raio=0.015)
    img = retangulo(img, L, A, 0.69, 0.11, 0.975, 0.50, (0.03, 0.12, 0.52), raio=0.015)
    img = retangulo(img, L, A, 0.70, 0.12, 0.965, 0.49, (0.06, 0.24, 0.80), raio=0.012)
    # o equalizador
    rng = np.random.default_rng(12)
    barras = 22
    for i in range(barras):
        x0 = 0.04 + i * 0.6 / barras
        alto = rng.uniform(0.25, 1.0) * (0.6 + 0.4 * math.sin(i * 0.45))
        for k in range(int(alto * 9) + 1):
            yb = 0.53 - k * 0.022
            cor = (0.55, 0.95, 1.0) if k < 6 else (1.0, 0.90, 0.30)
            img = retangulo(img, L, A, x0, yb - 0.016, x0 + 0.6 / barras * 0.72, yb, cor)
    # a barra do tempo
    img = retangulo(img, L, A, 0.04, 0.575, 0.96, 0.590, (0.08, 0.28, 0.75), raio=0.006)
    img = retangulo(img, L, A, 0.04, 0.575, 0.66, 0.590, (0.55, 0.92, 1.0), raio=0.006)
    img = disco(img, L, A, (0.66, 0.5825), 0.013, BRANCO_P)
    letras = textos(L, A, [
        ('RELÂMPAGO McQUEEN', FONTE_FORTE, 0.040, (0.88, 0.96, 1.0), 0.045, 0.053, dict(alinha='LEFT', espaco=1.1)),
        ('RÁDIO DA ESTRADA', FONTE_FORTE, 0.036, (0.60, 0.85, 1.0), 0.955, 0.053, dict(alinha='RIGHT', espaco=1.1)),
        ('VIDA NA ESTRADA', FONTE_FORTE, 0.085, (0.96, 0.99, 1.0), 0.05, 0.175, dict(alinha='LEFT', max=0.58)),
        ('FAIXA 07  ·  TRILHA DA COPA PISTÃO', FONTE_MEIA, 0.036, (0.60, 0.85, 1.0), 0.05, 0.255, dict(alinha='LEFT', max=0.58)),
        ('95', FONTE_TITULO, 0.20, (1.0, 0.86, 0.16), 0.832, 0.29, dict(contorno=((0.85, 0.10, 0.06), 0.010), inclina=0.12)),
        ('KATCHAU!', FONTE_FORTE, 0.040, (0.88, 0.96, 1.0), 0.832, 0.445, dict(espaco=1.1)),
        ('2:47', FONTE_FORTE, 0.034, (0.70, 0.90, 1.0), 0.04, 0.623, dict(alinha='LEFT')),
        ('4:12', FONTE_FORTE, 0.034, (0.70, 0.90, 1.0), 0.96, 0.623, dict(alinha='RIGHT')),
    ])
    img = por_cima(img, letras)
    linhas = (np.floor(y * L) % 3 == 0).astype(np.float32)
    return np.clip(img * (1.0 - 0.06 * linhas[..., None]), 0.0, 1.0)


def adesivo_95(L, A):
    img = np.ones((A, L, 3), dtype=np.float32) * np.asarray(VERMELHO_P, dtype=np.float32)
    img = disco(img, L, A, (0.5, 0.5), 0.47, BRANCO_P)
    img = disco(img, L, A, (0.5, 0.5), 0.42, VERMELHO_P)
    letras = textos(L, A, [('95', FONTE_TITULO, 0.52, AMARELO_P, 0.5, 0.5, dict(contorno=(VERMELHO_ESCURO_P, 0.025), inclina=0.14,
                                                                                 max=0.78))])
    return np.clip(por_cima(img, letras), 0.0, 1.0)


# O raio dos adesivos (em frações da casa, y de cima para baixo): o raio clássico, com a cabeça em cima
RAIO_DO_ADESIVO = tuple((0.08 + bx * 0.84, 0.04 + (1.0 - by) * 0.92) for bx, by in RAIO_CLASSICO)


def adesivo_raio(L, A):
    img = np.ones((A, L, 3), dtype=np.float32) * np.asarray(VERMELHO_P, dtype=np.float32)
    img = poligono_na_imagem(img, L, A, RAIO_DO_ADESIVO, (1.0, 0.82, 0.10), borda=-0.035)
    return img


def adesivo_rusteze(L, A):
    a = A / L
    img = np.ones((A, L, 3), dtype=np.float32) * np.asarray((0.60, 0.20, 0.06), dtype=np.float32)
    img = elipse(img, L, A, (0.5, a / 2), (0.47, a * 0.45), CREME_P)
    img = elipse(img, L, A, (0.5, a / 2), (0.44, a * 0.40), (0.84, 0.36, 0.10))
    letras = textos(L, A, [('Rust-eze', FONTE_CURSIVA, 0.20, CREME_P, 0.5, a * 0.48, dict(contorno=((0.45, 0.12, 0.04), 0.012),
                                                                                         max=0.72))])
    return np.clip(por_cima(img, letras), 0.0, 1.0)


def adesivo_mcqueen(L, A):
    a = A / L
    img = degrade(L, A, (0.14, 0.02, 0.03), (0.30, 0.03, 0.04))
    img = poligono_na_imagem(img, L, A, [(0.70, 0.05), (0.56, 0.27), (0.66, 0.27), (0.52, 0.47), (0.84, 0.20), (0.73, 0.20),
                                         (0.82, 0.05)], (1.0, 0.80, 0.12))
    letras = textos(L, A, [
        ('RELÂMPAGO', FONTE_FORTE, 0.07, AMARELO_P, 0.40, a * 0.24, dict(espaco=1.25, max=0.6)),
        ('McQUEEN', FONTE_TITULO, 0.20, VERMELHO_P, 0.46, a * 0.62, dict(contorno=(AMARELO_P, 0.012), inclina=0.14, max=0.86)),
    ])
    return np.clip(por_cima(img, letras), 0.0, 1.0)


def adesivo_copa(L, A):
    img = np.ones((A, L, 3), dtype=np.float32) * np.asarray(PRETO_P, dtype=np.float32)
    img = disco(img, L, A, (0.5, 0.5), 0.48, OURO_P)
    img = disco(img, L, A, (0.5, 0.5), 0.43, (0.08, 0.10, 0.22))
    x, y = grade_de_pixels(L, A)
    casa = ((np.floor(x / 0.05) + np.floor(y / 0.05)) % 2).astype(np.float32)
    faixa = (y > 0.62) & (y < 0.72) & (np.hypot(x - 0.5, y - 0.5) < 0.42)
    img = np.where(faixa[..., None], misturar(PRETO_P, BRANCO_P, casa), img)
    letras = textos(L, A, [
        ('COPA', FONTE_TITULO, 0.20, OURO_P, 0.5, 0.33, dict(max=0.6, contorno=((0.35, 0.18, 0.03), 0.008))),
        ('PISTÃO', FONTE_TITULO, 0.16, OURO_P, 0.5, 0.50, dict(max=0.66, contorno=((0.35, 0.18, 0.03), 0.008))),
    ])
    return np.clip(por_cima(img, letras), 0.0, 1.0)


def adesivo_dinoco(L, A):
    img = np.ones((A, L, 3), dtype=np.float32) * np.asarray(AZUL_P, dtype=np.float32)
    img = disco(img, L, A, (0.5, 0.5), 0.47, BRANCO_P)
    img = disco(img, L, A, (0.5, 0.5), 0.43, AZUL_P)
    letras = textos(L, A, [
        ('DINOCO', FONTE_TITULO, 0.20, BRANCO_P, 0.5, 0.47, dict(max=0.72, inclina=0.08)),
        ('400', FONTE_FORTE, 0.10, (0.99, 0.82, 0.20), 0.5, 0.66, dict()),
    ])
    return np.clip(por_cima(img, letras), 0.0, 1.0)


def imagem_do_teclado(L, A):
    """A cara do teclado numérico perto da porta: bege, com a grade do alto-falante, o visor e as 12 teclas"""
    a = A / L
    img = degrade(L, A, (0.80, 0.74, 0.58), (0.66, 0.60, 0.46))
    x, y = grade_de_pixels(L, A)
    # a grade do alto-falante (furinhos num círculo)
    furos = ((np.hypot((x * 26) % 1.0 - 0.5, (y * 26) % 1.0 - 0.5) < 0.28) & (np.hypot(x - 0.32, y - 0.20) < 0.14))
    img = np.where(furos[..., None], np.asarray((0.25, 0.22, 0.17), dtype=np.float32), img)
    img = retangulo(img, L, A, 0.56, 0.10, 0.92, 0.32, (0.30, 0.27, 0.21), raio=0.02)
    img = retangulo(img, L, A, 0.58, 0.12, 0.90, 0.30, (0.04, 0.14, 0.14), raio=0.015)
    linhas = []
    teclas = ('1', '2', '3', '4', '5', '6', '7', '8', '9', '*', '0', '#')
    for i, t in enumerate(teclas):
        lin, col = divmod(i, 3)
        x0, y0 = 0.10 + col * 0.28, 0.48 + lin * 0.34
        img = retangulo(img, L, A, x0 + 0.01, y0 + 0.012, x0 + 0.23, y0 + 0.29, (0.40, 0.36, 0.28), raio=0.03)   # a sombra
        img = retangulo(img, L, A, x0, y0, x0 + 0.22, y0 + 0.27, (0.86, 0.81, 0.66), raio=0.03)
        img = retangulo(img, L, A, x0 + 0.02, y0 + 0.02, x0 + 0.20, y0 + 0.12, (0.92, 0.88, 0.74), raio=0.02)
        linhas.append((t, FONTE_FORTE, 0.15, (0.20, 0.18, 0.14), x0 + 0.11, y0 + 0.14, dict()))
    linhas.append(('95', FONTE_FORTE, 0.10, (0.35, 0.95, 0.88), 0.74, 0.21, dict()))
    return np.clip(por_cima(img, textos(L, A, linhas)), 0.0, 1.0)


IMAGENS_DOS_ADESIVOS = {'95': adesivo_95, 'raio': adesivo_raio, 'rusteze': adesivo_rusteze, 'mcqueen': adesivo_mcqueen,
                        'copa': adesivo_copa, 'dinoco': adesivo_dinoco, 'teclado': imagem_do_teclado}


def pintar_o_atlas():
    """Os 8 pôsteres, a tela, os adesivos e o teclado, num atlas de 2048 x 2048"""
    atlas = np.zeros((LADO_DO_ATLAS, LADO_DO_ATLAS, 3), dtype=np.float32)
    for i, pintar in enumerate(POSTERES):
        t = time.time()
        x, y, l, a = casa_do_poster(i)
        atlas[y:y + a, x:x + l] = pintar(l, a)
        print(f"CARRETA: pôster {pintar.__name__} ({time.time() - t:.1f} s)")
    x, y, l, a = CASA_DA_TELA
    atlas[y:y + a, x:x + l] = imagem_da_tela(l, a)
    for nome, (x, y, l, a) in ADESIVOS.items():
        atlas[y:y + a, x:x + l] = IMAGENS_DOS_ADESIVOS[nome](l, a)
    return atlas


def no_atlas(x, y, l, a):
    """O retângulo (em pixels do atlas, de cima à esquerda) nas coordenadas de textura do Blender (u0, v0, u1, v1)"""
    n = float(LADO_DO_ATLAS)
    return (x / n, 1.0 - (y + a) / n, (x + l) / n, 1.0 - y / n)


def lugar_do_poster(i):
    x, y, l, a = casa_do_poster(i)
    return no_atlas(x + 2, y + 2, l - 4, a - 4)


LUGAR_DA_TELA = no_atlas(CASA_DA_TELA[0] + 2, CASA_DA_TELA[1] + 2, CASA_DA_TELA[2] - 4, CASA_DA_TELA[3] - 4)


def uv_na_casa(casa, fx, fy):
    """Um ponto de uma casa do atlas (fx, fy em frações da largura e da altura dela, y de cima para baixo)"""
    x, y, l, a = casa
    n = float(LADO_DO_ATLAS)
    return ((x + 1 + fx * (l - 2)) / n, 1.0 - (y + 1 + fy * (a - 2)) / n)


# ======================================================================
# 3) As texturas que se repetem: a chapa xadrez preta e a chapa perfurada
# ======================================================================
LADO_DA_CHAPA = 512
METROS_DA_CHAPA = 0.36     # um ladrilho da chapa xadrez cobre 36 x 36 cm (os relevos com uns 4 cm)
METROS_DA_PERFURADA = 0.32  # e um da chapa perfurada, 32 x 32 cm (32 x 32 furos)


def pintar_a_chapa():
    """A chapa xadrez (os relevos em losango, alternados), escura e polida: a base quase preta e os relevos com o
    brilho na aresta que pega a luz"""
    n = LADO_DA_CHAPA
    casas = 8
    y, x = np.mgrid[0:n, 0:n].astype(np.float32)
    c = n / casas
    i, j = np.floor(x / c), np.floor(y / c)
    u, v = (x - (i + 0.5) * c) / c, (y - (j + 0.5) * c) / c        # de -0,5 a 0,5 dentro da casa
    sinal = np.where((i + j) % 2 == 0, 1.0, -1.0)
    s, t = (u + sinal * v) / math.sqrt(2), (u - sinal * v) / math.sqrt(2)   # ao longo do relevo e de lado
    comp, larg = 0.42, 0.085
    d = np.sqrt((np.maximum(np.abs(s) - (comp - larg), 0.0) / larg) ** 2 + (t / larg) ** 2)
    altura = np.clip(1.0 - d, 0.0, 1.0) ** 0.5
    gy, gx = np.gradient(altura)
    luz = np.clip(0.5 + 2.2 * (-gx * 0.6 - gy * 0.8), 0.0, 1.0)     # a luz vem de cima, à esquerda
    rng = np.random.default_rng(3)
    escovado = borrar(rng.normal(0.0, 1.0, (n, n)).astype(np.float32), 1, 1)
    escovado = np.repeat(escovado.mean(axis=1, keepdims=True), n, axis=1) * 0.5 + escovado * 0.5
    base = 0.24 + 0.02 * escovado
    relevo = altura > 0.02
    cor = np.where(relevo, 0.34 + 0.42 * luz + 0.06 * altura, base)
    brilho = np.clip((luz - 0.60) * 3.0, 0.0, 1.0) * relevo
    cor = cor + 0.25 * brilho
    rgb = np.stack([cor, cor, cor * 1.02], axis=-1)
    return np.clip(rgb, 0.0, 1.0)


def pintar_a_perfurada():
    """A chapa perfurada: grafite escovado com 32 x 32 furos redondos em fileiras desencontradas (a borda de cima de
    cada furo pega a luz, a de baixo faz sombra)"""
    n = LADO_DA_CHAPA
    passo = n / 32
    y, x = np.mgrid[0:n, 0:n].astype(np.float32) + 0.5
    linha = np.floor(y / passo)
    xx = (x + (linha % 2) * passo / 2) % passo - passo / 2
    yy = y % passo - passo / 2
    d = np.hypot(xx, yy)
    r = passo * 0.30
    furo = 1.0 - suave(r - 0.8, r + 0.8, d)
    rng = np.random.default_rng(5)
    escovado = borrar(rng.normal(0.0, 1.0, (n, n)).astype(np.float32), 1, 1)
    escovado = np.repeat(escovado.mean(axis=0, keepdims=True), n, axis=0) * 0.6 + escovado * 0.4
    metal = 0.40 + 0.03 * escovado
    borda = suave(r - 0.5, r + 1.8, d) * (1.0 - suave(r + 1.8, r + 3.0, d))   # o anel em volta do furo
    metal = metal + 0.10 * borda * np.clip(-yy / r, 0.0, 1.0) - 0.08 * borda * np.clip(yy / r, 0.0, 1.0)
    cor = metal * (1.0 - furo) + 0.05 * furo
    rgb = np.stack([cor, cor * 1.0, cor * 1.04], axis=-1)
    return np.clip(rgb, 0.0, 1.0)


# ======================================================================
# 4) A geometria
# ======================================================================
class Camada:
    """A camada das luzes: uma malha só, com a cor acesa e a ordem de acender em cada canto"""

    def __init__(self):
        self.bm = bmesh.new()
        self.cor = self.bm.loops.layers.float_color.new('Col')

    def vertice(self, p):
        return self.bm.verts.new(B(p))

    def face(self, verts, cor, ordem, cores=None):
        try:
            f = self.bm.faces.new(verts)
        except ValueError:
            return None
        for i, laco in enumerate(f.loops):
            c = cores[i] if cores is not None else cor
            laco[self.cor] = (c[0], c[1], c[2], ordem)
        return f

    def poligono(self, pontos, normal, cor, ordem):
        pontos = [Vector(p) for p in pontos]
        if newell(pontos).dot(Vector(normal)) < 0:
            pontos = pontos[::-1]
        return self.face([self.vertice(p) for p in pontos], cor, ordem)


def newell(pontos):
    n = Vector((0.0, 0.0, 0.0))
    for i in range(len(pontos)):
        a, b = pontos[i], pontos[(i + 1) % len(pontos)]
        n += Vector(((a.y - b.y) * (a.z + b.z), (a.z - b.z) * (a.x + b.x), (a.x - b.x) * (a.y + b.y)))
    return n


def base_perpendicular(eixo):
    eixo = Vector(eixo).normalized()
    ref = V(0, 1, 0) if abs(eixo.y) < 0.9 else V(1, 0, 0)
    a = ref.cross(eixo).normalized()
    return a, eixo.cross(a).normalized()


class Construtor:
    """Uma malha só (bmesh), com um material por acabamento, coordenadas de textura e as faces no lado de dentro; e a
    camada das luzes"""

    def __init__(self):
        self.bm = bmesh.new()
        self.uv = self.bm.loops.layers.uv.new('UV')
        self.ordem = []
        self.luzes = Camada()

    def indice(self, acab):
        if acab not in ACABAMENTOS:
            raise KeyError(acab)
        if acab not in self.ordem:
            self.ordem.append(acab)
        return self.ordem.index(acab)

    def vertice(self, p):
        return self.bm.verts.new(B(p))

    def face(self, verts, acab, uvs=None, liso=False):
        try:
            f = self.bm.faces.new(verts)
        except ValueError:
            return None
        f.material_index = self.indice(acab)
        f.smooth = liso
        if uvs is not None:
            for laco, uv in zip(f.loops, uvs):
                laco[self.uv].uv = uv
        return f

    def poligono(self, pontos, acab, uvs=None, liso=False):
        return self.face([self.vertice(p) for p in pontos], acab, uvs, liso)

    def poligono_virado(self, pontos, normal, acab, uvs=None):
        """Um polígono plano, com os vértices na ordem que deixa a face virada para 'normal'"""
        pontos = [Vector(p) for p in pontos]
        if newell(pontos).dot(Vector(normal)) < 0:
            pontos = pontos[::-1]
            uvs = uvs[::-1] if uvs else uvs
        return self.poligono(pontos, acab, uvs)

    # ---- Grades (as superfícies grandes) ----
    def grade(self, origem, du, dv, acab, passo=PASSO, quebras_u=(), quebras_v=(), uv=None, pular=None, liso=False):
        """Um retângulo em grade de origem até origem + du + dv, virado para du x dv. As quebras (em metros ao longo
        de du e de dv) viram linhas da grade. uv: None, ('mundo', escala) ou ('atlas', (u0, v0, u1, v1)).
        pular(p) -> True para não fazer a casa com o meio em p."""
        origem, du, dv = Vector(origem), Vector(du), Vector(dv)
        lu, lv = du.length, dv.length
        if lu < 1e-6 or lv < 1e-6:
            return
        nu, nv = max(1, math.ceil(lu / passo - 1e-6)), max(1, math.ceil(lv / passo - 1e-6))
        us = sorted(set([round(i / nu * lu, 5) for i in range(nu + 1)] + [round(q, 5) for q in quebras_u if 0 < q < lu]))
        vs = sorted(set([round(j / nv * lv, 5) for j in range(nv + 1)] + [round(q, 5) for q in quebras_v if 0 < q < lv]))
        us = [a for k, a in enumerate(us) if k == 0 or a - us[k - 1] > 0.004]
        vs = [a for k, a in enumerate(vs) if k == 0 or a - vs[k - 1] > 0.004]
        eu, ev = du / lu, dv / lv
        normal = eu.cross(ev).normalized()
        vert = [[None] * len(vs) for _ in us]
        for i, a in enumerate(us):
            for j, b in enumerate(vs):
                vert[i][j] = self.vertice(origem + eu * a + ev * b)
        for i in range(len(us) - 1):
            for j in range(len(vs) - 1):
                if pular is not None and pular(origem + eu * (us[i] + us[i + 1]) / 2 + ev * (vs[j] + vs[j + 1]) / 2):
                    continue
                cantos = [(i, j), (i + 1, j), (i + 1, j + 1), (i, j + 1)]
                uvs = None
                if uv is not None:
                    uvs = [self._uv(uv, origem + eu * us[a] + ev * vs[b], us[a] / lu, vs[b] / lv, normal) for a, b in cantos]
                self.face([vert[a][b] for a, b in cantos], acab, uvs, liso)

    def grade4(self, a, b, c, d, acab, passo=PASSO, uv=None):
        """Um quadrilátero qualquer (a, b, c, d em volta, virado para (b - a) x (d - a)) em grade bilinear"""
        a, b, c, d = Vector(a), Vector(b), Vector(c), Vector(d)
        nu = max(1, math.ceil(max((b - a).length, (c - d).length) / passo))
        nv = max(1, math.ceil(max((d - a).length, (c - b).length) / passo))
        pts = [[a.lerp(b, i / nu).lerp(d.lerp(c, i / nu), j / nv) for j in range(nv + 1)] for i in range(nu + 1)]
        vert = [[self.vertice(p) for p in linha] for linha in pts]
        normal = (b - a).cross(d - a).normalized()
        for i in range(nu):
            for j in range(nv):
                cantos = [(i, j), (i + 1, j), (i + 1, j + 1), (i, j + 1)]
                uvs = [self._uv(uv, pts[p][q], 0, 0, normal) for p, q in cantos] if uv is not None else None
                self.face([vert[p][q] for p, q in cantos], acab, uvs)

    def _uv(self, uv, p, fu, fv, normal):
        tipo, valor = uv
        if tipo == 'atlas':
            u0, v0, u1, v1 = valor
            return (u0 + fu * (u1 - u0), v0 + fv * (v1 - v0))
        n = [abs(c) for c in normal]
        if n[1] >= n[0] and n[1] >= n[2]:
            return (p.x / valor, p.z / valor)
        if n[0] >= n[2]:
            return (p.z / valor, p.y / valor)
        return (p.x / valor, p.y / valor)

    def caixa(self, x0, x1, y0, y1, z0, z1, acab, passo=PASSO, sem=(), uv=None, por_dentro=False, acabs=None):
        """Uma caixa de faces viradas para fora (ou para dentro). sem: as faces que não entram ('+x', '-x', '+y', ...).
        acabs: um acabamento diferente para algumas faces ({'+y': 'chapa'})."""
        x0, x1 = min(x0, x1), max(x0, x1)
        y0, y1 = min(y0, y1), max(y0, y1)
        z0, z1 = min(z0, z1), max(z0, z1)
        X, Y, Z = V(x1 - x0, 0, 0), V(0, y1 - y0, 0), V(0, 0, z1 - z0)
        faces = {
            '+x': (V(x1, y0, z0), Y, Z), '-x': (V(x0, y0, z0), Z, Y),
            '+y': (V(x0, y1, z0), Z, X), '-y': (V(x0, y0, z0), X, Z),
            '+z': (V(x0, y0, z1), X, Y), '-z': (V(x0, y0, z0), Y, X),
        }
        for nome, (o, du, dv) in faces.items():
            if nome in sem:
                continue
            if por_dentro:
                du, dv = dv, du
            a = (acabs or {}).get(nome, acab)
            textura = ACABAMENTOS[a][2]
            u = (uv.get(textura) if isinstance(uv, dict) else uv) if textura is not None else None
            self.grade(o, du, dv, a, passo, uv=u)

    def faixa(self, s, p0, p1, z0, z1, acab, passo=PASSO, uv=None, quebras_p=(), quebras_z=(), pular=None):
        """Uma faixa do corte da parede do lado s: de p0 a p1 (em (|x|, y)) e de z0 a z1, virada para o lado aberto
        (a esquerda de quem anda de p0 a p1 no plano (|x|, y))"""
        a = V(s * p0[0], p0[1], z0)
        b = V(s * p1[0], p1[1], z0)
        du, dv = b - a, V(0, 0, z1 - z0)
        dx, dy = p1[0] - p0[0], p1[1] - p0[1]
        normal = V(-s * dy, dx, 0)
        if du.cross(dv).dot(normal) >= 0:
            self.grade(a, du, dv, acab, passo, quebras_u=quebras_p, quebras_v=[q - z0 for q in quebras_z], uv=uv, pular=pular)
        else:
            self.grade(a, dv, du, acab, passo, quebras_u=[q - z0 for q in quebras_z], quebras_v=quebras_p, uv=uv, pular=pular)

    # ---- Peças redondas ----
    def torno(self, perfil, centro, acab, lados=24, eixo=V(0, 1, 0), referencia=V(1, 0, 0), liso=True, de=0.0, ate=2 * math.pi):
        """Um sólido de revolução. perfil: [(r, h, acabamento do trecho que começa aqui ou None)], em volta do sólido no
        sentido anti-horário (r para a direita, h para cima): as faces ficam viradas para fora."""
        eixo = Vector(eixo).normalized()
        ref = Vector(referencia)
        ref = (ref - eixo * ref.dot(eixo)).normalized()
        terceiro = ref.cross(eixo)
        centro = Vector(centro)
        fechado = abs((ate - de) - 2 * math.pi) < 1e-6
        n = lados if fechado else lados + 1
        aneis = []
        for item in perfil:
            r, h = item[0], item[1]
            anel = []
            for k in range(n):
                a = de + (ate - de) * k / lados
                anel.append(self.vertice(centro + eixo * h + (ref * math.cos(a) + terceiro * math.sin(a)) * r))
            aneis.append(anel)
        trecho = acab
        for i in range(len(perfil) - 1):
            if perfil[i][2] is not None:
                trecho = perfil[i][2]
            if abs(perfil[i][0]) < 1e-6 and abs(perfil[i + 1][0]) < 1e-6:
                continue
            for k in range(lados):
                k1 = (k + 1) % n
                a, b, c, d = aneis[i][k], aneis[i + 1][k], aneis[i + 1][k1], aneis[i][k1]
                if abs(perfil[i][0]) < 1e-6:
                    self.face([a, b, c], trecho, liso=liso)
                elif abs(perfil[i + 1][0]) < 1e-6:
                    self.face([a, b, d], trecho, liso=liso)
                else:
                    self.face([a, b, c, d], trecho, liso=liso)

    def tubo(self, pontos, raio, acab, lados=8, tampas=True, liso=True, fechado=False):
        """Um tubo ao longo de uma linha (alças, suportes, anéis): os anéis perpendiculares à média das direções"""
        pontos = [Vector(p) for p in pontos]
        n = len(pontos)
        tang = []
        for i in range(n):
            if fechado:
                a, b = pontos[(i - 1) % n], pontos[(i + 1) % n]
            else:
                a, b = pontos[max(i - 1, 0)], pontos[min(i + 1, n - 1)]
            tang.append((b - a).normalized())
        n0 = tang[0].cross(V(0, 1, 0))
        if n0.length < 0.1:
            n0 = tang[0].cross(V(1, 0, 0))
        n0.normalize()
        normais = [n0]
        for i in range(1, n):
            q = normais[-1] - tang[i] * normais[-1].dot(tang[i])
            normais.append(q.normalized())
        aneis = []
        for p, t, q in zip(pontos, tang, normais):
            b = t.cross(q)
            aneis.append([self.vertice(p + raio * (q * math.cos(2 * math.pi * k / lados) + b * math.sin(2 * math.pi * k / lados)))
                          for k in range(lados)])
        trechos = n if fechado else n - 1
        for i in range(trechos):
            j = (i + 1) % n
            for k in range(lados):
                k1 = (k + 1) % lados
                self.face([aneis[i][k], aneis[i][k1], aneis[j][k1], aneis[j][k]], acab, liso=liso)
        if tampas and not fechado:
            self.face(list(reversed(aneis[0])), acab)
            self.face(aneis[-1], acab)

    def cilindro(self, base, eixo, raio, comprimento, acab, lados=12, tampa_de_cima=True, tampa_de_baixo=True, acab_da_tampa=None):
        eixo = Vector(eixo).normalized()
        perfil = []
        if tampa_de_baixo:
            perfil.append((0.0, 0.0, None))
        perfil += [(raio, 0.0, None), (raio, comprimento, acab_da_tampa)]
        if tampa_de_cima:
            perfil.append((0.0, comprimento, None))
        ref = V(1, 0, 0) if abs(eixo.x) < 0.9 else V(0, 1, 0)
        self.torno(perfil, base, acab, lados, eixo, ref, liso=False)

    # ---- O que brilha (no interior e na camada das luzes) ----
    def _tubo_reto(self, malha, a, b, raio, lados, cor_ou_acab, ordem=None):
        """Um trecho reto de tubo com as tampas (no interior, malha = self; na camada, malha = self.luzes)"""
        e1, e2 = base_perpendicular(b - a)
        aneis = []
        for p in (a, b):
            aneis.append([malha.vertice(p + raio * (e1 * math.cos(2 * math.pi * k / lados) + e2 * math.sin(2 * math.pi * k / lados)))
                          for k in range(lados)])
        faces = [[aneis[0][k], aneis[0][(k + 1) % lados], aneis[1][(k + 1) % lados], aneis[1][k]] for k in range(lados)]
        faces += [list(reversed(aneis[0])), aneis[1]]
        for f in faces:
            if malha is self:
                self.face(f, cor_ou_acab)
            else:
                malha.face(f, cor_ou_acab, ordem)

    def _bolinha(self, malha, centro, raio, cor_ou_acab, ordem=None, lados=6, meridianos=3):
        """Uma esfera pequena (as juntas do neon)"""
        polos = (malha.vertice(centro + V(0, raio, 0)), malha.vertice(centro - V(0, raio, 0)))
        aneis = []
        for m in range(1, meridianos):
            fi = math.pi * m / meridianos
            aneis.append([malha.vertice(centro + V(raio * math.sin(fi) * math.cos(2 * math.pi * k / lados), raio * math.cos(fi),
                                                   raio * math.sin(fi) * math.sin(2 * math.pi * k / lados))) for k in range(lados)])
        faces = []
        for k in range(lados):
            k1 = (k + 1) % lados
            faces.append([polos[0], aneis[0][k1], aneis[0][k]])
            faces.append([polos[1], aneis[-1][k], aneis[-1][k1]])
            for m in range(len(aneis) - 1):
                faces.append([aneis[m][k], aneis[m][k1], aneis[m + 1][k1], aneis[m + 1][k]])
        for f in faces:
            if malha is self:
                self.face(f, cor_ou_acab)
            else:
                malha.face(f, cor_ou_acab, ordem)

    def neon(self, pontos, fechado=False, liso=False, ordem=None, raio=RAIO_DO_NEON, cor=NEON):
        """Um tubo de neon (no interior, 'neon'; na camada das luzes, 2,5 mm mais grosso, aceso). Em linhas quebradas
        (liso=False), cada trecho é um tubo reto e as juntas são bolinhas; nas curvas (liso=True), um tubo só."""
        pontos = [Vector(p) for p in pontos]
        if liso:
            o = ordem if ordem is not None else ordem_por_z(sum((p.z for p in pontos), 0.0) / len(pontos))
            self.tubo(pontos, raio, 'neon', lados=6, liso=False, fechado=fechado)
            camada = self.luzes
            n = len(pontos)
            # (o mesmo tubo, mais grosso, na camada)
            tang = []
            for i in range(n):
                a, b = (pontos[(i - 1) % n], pontos[(i + 1) % n]) if fechado else (pontos[max(i - 1, 0)], pontos[min(i + 1, n - 1)])
                tang.append((b - a).normalized())
            q0 = tang[0].cross(V(0, 1, 0))
            if q0.length < 0.1:
                q0 = tang[0].cross(V(1, 0, 0))
            normais = [q0.normalized()]
            for i in range(1, n):
                q = normais[-1] - tang[i] * normais[-1].dot(tang[i])
                normais.append(q.normalized())
            r = raio + GROSSURA_DA_LUZ
            aneis = [[camada.vertice(p + r * (q * math.cos(2 * math.pi * k / 6) + t.cross(q) * math.sin(2 * math.pi * k / 6)))
                      for k in range(6)] for p, t, q in zip(pontos, tang, normais)]
            for i in range(n if fechado else n - 1):
                j = (i + 1) % n
                for k in range(6):
                    camada.face([aneis[i][k], aneis[i][(k + 1) % 6], aneis[j][(k + 1) % 6], aneis[j][k]], cor, o)
            return
        if fechado:
            pontos = pontos + [pontos[0]]
        for i in range(len(pontos) - 1):
            a, b = pontos[i], pontos[i + 1]
            if (b - a).length < 1e-4:
                continue
            o = ordem if ordem is not None else ordem_por_z((a.z + b.z) / 2)
            self._tubo_reto(self, a, b, raio, 6, 'neon')
            self._tubo_reto(self.luzes, a, b, raio + GROSSURA_DA_LUZ, 6, cor, o)
        juntas = pontos[1:-1] if not fechado else pontos[:-1]
        for p in juntas:
            o = ordem if ordem is not None else ordem_por_z(p.z)
            self._bolinha(self, p, raio * 1.02, 'neon')
            self._bolinha(self.luzes, p, raio * 1.02 + GROSSURA_DA_LUZ, cor, o)

    def chapa_acesa(self, pontos, normal, acab, cor, ordem, uvs=None, afasta=0.0015):
        """Um polígono que brilha (encostado numa superfície): no interior, 'afasta' à frente dela; na camada das luzes,
        mais GROSSURA_DA_LUZ à frente"""
        n = Vector(normal).normalized()
        self.poligono_virado([Vector(p) + n * afasta for p in pontos], n, acab, uvs)
        self.luzes.poligono([Vector(p) + n * (afasta + GROSSURA_DA_LUZ) for p in pontos], n, cor, ordem)

    # ---- Placas, letras e relevos ----
    def prisma(self, pontos2d, profundidade, origem, direita, cima, acab, acab_lado=None, fundo=False, uv=None):
        """Um polígono (em metros, no plano direita x cima) com espessura, saindo da parede para a frente (com fundo,
        fechado também atrás). uv: ('mundo', escala) para a frente, se ela tiver textura"""
        direita, cima = Vector(direita).normalized(), Vector(cima).normalized()
        frente = direita.cross(cima)
        origem = Vector(origem)
        p_tras = [origem + direita * x + cima * y for x, y in pontos2d]
        p_diante = [p + frente * profundidade for p in p_tras]
        area = sum(pontos2d[i][0] * pontos2d[(i + 1) % len(pontos2d)][1] - pontos2d[(i + 1) % len(pontos2d)][0] * pontos2d[i][1]
                   for i in range(len(pontos2d)))
        if area < 0:
            p_tras, p_diante = p_tras[::-1], p_diante[::-1]
        tras = [self.vertice(p) for p in p_tras]
        diante = [self.vertice(p) for p in p_diante]
        uvs = [self._uv(uv, p, 0, 0, frente) for p in p_diante] if uv is not None else None
        self.face(diante, acab, uvs)
        if fundo:
            self.face(tras[::-1], acab)
        n = len(diante)
        for i in range(n):
            j = (i + 1) % n
            self.face([tras[i], tras[j], diante[j], diante[i]], acab_lado or acab)

    def texto(self, texto, fonte, tamanho, origem, direita, cima, acab, profundidade=0.02, alinha='CENTER', espaco=1.0,
              inclina=0.0, grossura=0.0, largura_max=None, escala=None, acab_lado=None, resolucao=3):
        """Um letreiro: objeto de texto do Blender (com a fonte do jogo) virado malha, encostado em 'origem' (o meio
        da linha), lido da frente (direita x cima aponta para quem lê). Com profundidade 0, as letras são chapadas.
        Devolve a largura e a escala usada."""
        curva = bpy.data.curves.new('letreiro', 'FONT')
        curva.body = texto
        curva.font = carregar_fonte(fonte)
        curva.size = tamanho
        curva.align_x = alinha
        curva.align_y = 'CENTER'
        curva.space_character = espaco
        curva.shear = inclina
        curva.offset = grossura
        curva.extrude = profundidade / 2
        curva.resolution_u = resolucao
        obj = bpy.data.objects.new('letreiro', curva)
        bpy.context.scene.collection.objects.link(obj)
        grafo = bpy.context.evaluated_depsgraph_get()
        malha = bpy.data.meshes.new_from_object(obj.evaluated_get(grafo))
        xs = [v.co.x for v in malha.vertices]
        largura = (max(xs) - min(xs)) if xs else 0.0
        if escala is None:
            escala = 1.0
            if largura_max is not None and largura > largura_max:
                escala = largura_max / largura
        direita, cima = Vector(direita).normalized(), Vector(cima).normalized()
        frente = direita.cross(cima)
        origem = Vector(origem)
        pontos = [origem + direita * (v.co.x * escala) + cima * (v.co.y * escala) + frente * (v.co.z + profundidade / 2)
                  for v in malha.vertices]
        if profundidade <= 0.0:
            for p in malha.polygons:
                self.poligono_virado([pontos[i] for i in p.vertices], frente, acab)
        else:
            verts = [self.vertice(q) for q in pontos]
            for p in malha.polygons:
                frontal = abs(p.normal.z) > 0.5
                if frontal and p.normal.z < 0:
                    continue          # (a face de trás, encostada na parede)
                self.face([verts[i] for i in p.vertices], acab if frontal or acab_lado is None else acab_lado)
        bpy.data.objects.remove(obj)
        bpy.data.curves.remove(curva)
        bpy.data.meshes.remove(malha)
        return largura * escala, escala

    def adesivo(self, nome, centro, direita, cima, largura, forma='retangulo', altura=None):
        """Um adesivo do atlas: centro já 2 mm à frente da superfície; forma 'disco', 'oval', 'retangulo' ou 'raio'"""
        casa = ADESIVOS[nome]
        direita, cima = Vector(direita).normalized(), Vector(cima).normalized()
        altura = altura if altura is not None else largura * casa[3] / casa[2]
        centro = Vector(centro)
        if forma in ('disco', 'oval'):
            pts = [(0.5 + 0.47 * math.cos(2 * math.pi * k / 24), 0.5 + 0.47 * math.sin(2 * math.pi * k / 24)) for k in range(24)]
        elif forma == 'raio':
            pts = list(RAIO_DO_ADESIVO)
        else:
            pts = [(0.0, 0.0), (1.0, 0.0), (1.0, 1.0), (0.0, 1.0)]
        pontos = [centro + direita * ((fx - 0.5) * largura) + cima * ((0.5 - fy) * altura) for fx, fy in pts]
        uvs = [uv_na_casa(casa, fx, fy) for fx, fy in pts]
        self.poligono_virado(pontos, direita.cross(cima), 'poster', uvs)

    def estrela(self, centro, frente, cima, raio, espessura, acab, pontas=5):
        """Uma estrela maciça (as pontas num plano, com um bico na frente e outro atrás)"""
        frente, cima = Vector(frente).normalized(), Vector(cima).normalized()
        direita = cima.cross(frente)
        centro = Vector(centro)
        anel = []
        for i in range(2 * pontas):
            a = math.pi / 2 + i * math.pi / pontas
            r = raio if i % 2 == 0 else raio * 0.45
            anel.append(self.vertice(centro + (direita * math.cos(a) + cima * math.sin(a)) * r))
        bicos = (self.vertice(centro + frente * espessura), self.vertice(centro - frente * espessura))
        n = len(anel)
        for i in range(n):
            self.face([anel[i], anel[(i + 1) % n], bicos[0]], acab)
            self.face([anel[(i + 1) % n], anel[i], bicos[1]], acab)


_fontes = {}


def carregar_fonte(caminho):
    if caminho not in _fontes:
        _fontes[caminho] = bpy.data.fonts.load(caminho)
    return _fontes[caminho]


def deslocar(pontos, d, fechado=False, limite=3.0):
    """Desloca uma linha (2D) d para a esquerda de quem anda nela (com d < 0, para a direita), com as juntas em
    esquadria (limitadas a 'limite' vezes d: depois, chanfradas)"""
    p = [np.array(q, dtype=np.float64) for q in pontos]
    n = len(p)
    saida = []

    def normal(a, b):
        t = (b - a) / np.linalg.norm(b - a)
        return np.array((-t[1], t[0]))

    for i in range(n):
        if not fechado and i == 0:
            saida.append(p[0] + d * normal(p[0], p[1]))
            continue
        if not fechado and i == n - 1:
            saida.append(p[-1] + d * normal(p[-2], p[-1]))
            continue
        a, b, c = p[(i - 1) % n], p[i], p[(i + 1) % n]
        n1, n2 = normal(a, b), normal(b, c)
        m = n1 + n2
        cos = max(np.dot(m / np.linalg.norm(m), n1), 1e-6) if np.linalg.norm(m) > 1e-9 else 1e-6
        if 1.0 / cos > limite:
            saida.append(b + d * n1)
            saida.append(b + d * n2)
        else:
            saida.append(b + d * m / np.linalg.norm(m) / cos)
    return [tuple(q) for q in saida]


def raio_mapeado(z0, comp, y0, alt):
    """O raio clássico deitado (a cabeça na porta, a ponta no fundo), em (z, y)"""
    return [(z0 + (1.0 - by) * comp, y0 + bx * alt) for bx, by in RAIO_CLASSICO]


def area2d(p):
    return 0.5 * sum(p[i][0] * p[(i + 1) % len(p)][1] - p[(i + 1) % len(p)][0] * p[i][1] for i in range(len(p)))


def retangulo_redondo(meia_l, meia_a, raio, por_canto=4):
    """Os pontos de um retângulo de cantos redondos (no sentido anti-horário), em volta de (0, 0)"""
    pts = []
    for cx, cy, a0 in ((meia_l - raio, meia_a - raio, 0.0), (-meia_l + raio, meia_a - raio, 90.0),
                       (-meia_l + raio, -meia_a + raio, 180.0), (meia_l - raio, -meia_a + raio, 270.0)):
        for k in range(por_canto + 1):
            a = math.radians(a0 + 90.0 * k / por_canto)
            pts.append((cx + raio * math.cos(a), cy + raio * math.sin(a)))
    return pts


# ---- As peças ----
def spline(pontos, por_trecho=4):
    """Uma curva lisa (Catmull-Rom) passando pelos pontos"""
    p = [Vector(q) for q in pontos]
    p = [p[0] * 2 - p[1]] + p + [p[-1] * 2 - p[-2]]
    saida = []
    for i in range(1, len(p) - 2):
        p0, p1, p2, p3 = p[i - 1], p[i], p[i + 1], p[i + 2]
        for k in range(por_trecho):
            t = k / por_trecho
            saida.append(0.5 * (2 * p1 + (p2 - p0) * t + (2 * p0 - 5 * p1 + 4 * p2 - p3) * t * t + (3 * p1 - p0 - 3 * p2 + p3) * t ** 3))
    saida.append(p[-2])
    return saida


def alcas(c, centro, direcao, k, curva, raio, lados=6, por_trecho=2, acab='ouro'):
    """As duas alças de uma taça: a curva (r, h) em volta do eixo, dos dois lados, na 'direcao' (no plano xz)"""
    e = Vector(direcao).normalized()
    for s in (-1, 1):
        pts = spline([centro + e * (s * r * k) + V(0, h * k, 0) for r, h in curva], por_trecho)
        c.tubo(pts, raio * k, acab, lados=lados)


def trofeu_copa(c, x, y, z, altura, base='laca_preta', direcao_das_alcas=V(0, 0, 1), lados=14):
    """A taça de duas alças, num pedestal. direcao_das_alcas: None (sem alças) ou a direção delas (no plano xz)"""
    k = altura
    pe = 0.10 * k
    c.caixa(x - 0.17 * k, x + 0.17 * k, y, y + pe, z - 0.17 * k, z + 0.17 * k, base, passo=1.0, sem=('-y',))
    c.caixa(x - 0.18 * k, x + 0.18 * k, y + pe - 0.014 * k, y + pe, z - 0.18 * k, z + 0.18 * k, 'ouro', passo=1.0, sem=('-y',))
    y = y + pe
    perfil = [(0.0, 0.0, None), (0.15, 0.0, None), (0.15, 0.025, None), (0.115, 0.05, None), (0.09, 0.085, None),
              (0.04, 0.12, None), (0.03, 0.27, None), (0.055, 0.29, None), (0.06, 0.31, None), (0.035, 0.33, None),
              (0.045, 0.37, None), (0.11, 0.43, None), (0.18, 0.53, None), (0.215, 0.65, None), (0.23, 0.75, None),
              (0.243, 0.795, 'ouro_vivo'), (0.252, 0.815, None), (0.237, 0.823, 'ouro'), (0.215, 0.78, None), (0.20, 0.68, None),
              (0.0, 0.62, None)]
    c.torno([(r * k, h * k, a) for r, h, a in perfil], V(x, y, z), 'ouro', lados=lados)
    if direcao_das_alcas is not None:
        alcas(c, V(x, y, z), direcao_das_alcas, k, ((0.205, 0.72), (0.29, 0.745), (0.345, 0.69), (0.335, 0.59), (0.27, 0.50),
                                                    (0.17, 0.455)), 0.022)


def trofeu_coluna(c, x, y, z, altura, base='laca_preta', frente=V(0, 0, -1)):
    """O troféu de coluna: base preta de três degraus com a plaquinha dourada, a coluna dourada com anéis e uma
    estrela maciça em cima"""
    k = altura
    frente = Vector(frente).normalized()
    c.caixa(x - 0.11 * k, x + 0.11 * k, y, y + 0.07 * k, z - 0.11 * k, z + 0.11 * k, base, passo=1.0, sem=('-y',))
    c.caixa(x - 0.085 * k, x + 0.085 * k, y + 0.07 * k, y + 0.15 * k, z - 0.085 * k, z + 0.085 * k, base, passo=1.0, sem=('-y',))
    c.caixa(x - 0.06 * k, x + 0.06 * k, y + 0.15 * k, y + 0.19 * k, z - 0.06 * k, z + 0.06 * k, base, passo=1.0, sem=('-y',))
    p = V(x, y + 0.11 * k, z) + frente * (0.085 * k + 0.002)
    direita = V(0, 1, 0).cross(frente)
    c.poligono_virado([p + direita * (s * 0.06 * k) + V(0, t * 0.025 * k, 0) for s, t in ((-1, -1), (1, -1), (1, 1), (-1, 1))], frente,
                      'ouro_vivo')
    y = y + 0.19 * k
    c.torno([(0.0, 0.0, None), (0.05, 0.0, None), (0.05, 0.02, None), (0.032, 0.04, None), (0.028, 0.16, None), (0.042, 0.18, None),
             (0.042, 0.20, None), (0.026, 0.22, None), (0.024, 0.40, None), (0.040, 0.42, None), (0.040, 0.44, None),
             (0.030, 0.46, None), (0.046, 0.49, None), (0.040, 0.52, None), (0.0, 0.53, None)], V(x, y, z), 'ouro', lados=10)
    c.estrela(V(x, y + 0.53 * k + 0.115 * k, z), frente, V(0, 1, 0), 0.13 * k, 0.035 * k, 'ouro_vivo')
    c.estrela(V(x, y + 0.53 * k + 0.115 * k, z), frente.cross(V(0, 1, 0)), V(0, 1, 0), 0.10 * k, 0.03 * k, 'ouro')


def trofeu_tigela(c, x, y, z, altura, base='madeira', lados=14):
    """A taça baixa, sem alças, numa base redonda"""
    k = altura
    c.torno([(0.0, 0.0, None), (0.14, 0.0, None), (0.14, 0.12, None), (0.12, 0.14, 'ouro'), (0.07, 0.17, None),
             (0.03, 0.22, None), (0.025, 0.40, None), (0.08, 0.46, None), (0.17, 0.56, None), (0.20, 0.66, None),
             (0.21, 0.70, 'ouro_vivo'), (0.19, 0.70, 'ouro'), (0.175, 0.62, None), (0.0, 0.57, None)], V(x, y, z), base, lados=lados)


def copa_pistao(c, x, y, z, k=1.0):
    """A Copa Pistão: um pedestal preto de degraus e a taça grande, de boca larga, haste em degraus e alças grandes
    em curva (viradas para z, ao longo da prateleira)"""
    for (m, y0, y1, acab) in ((0.25, 0.00, 0.06, 'laca_preta'), (0.255, 0.06, 0.072, 'ouro'), (0.21, 0.072, 0.15, 'laca_preta'),
                              (0.215, 0.15, 0.16, 'ouro'), (0.17, 0.16, 0.20, 'laca_preta')):
        c.caixa(x - m * k, x + m * k, y + y0 * k, y + y1 * k, z - m * k, z + m * k, acab, passo=1.0, sem=('-y',))
    y = y + 0.20 * k
    perfil = [(0.0, 0.0, None), (0.20, 0.0, None), (0.20, 0.03, None), (0.17, 0.035, None), (0.17, 0.06, None), (0.13, 0.065, None),
              (0.13, 0.09, None), (0.09, 0.10, None), (0.05, 0.14, None), (0.036, 0.20, None), (0.034, 0.25, None),
              (0.075, 0.275, None), (0.08, 0.30, None), (0.045, 0.325, None), (0.032, 0.36, None), (0.05, 0.40, None),
              (0.12, 0.45, None), (0.20, 0.52, None), (0.26, 0.60, None), (0.30, 0.69, None), (0.33, 0.78, None),
              (0.35, 0.86, 'ouro_vivo'), (0.385, 0.905, None), (0.40, 0.925, None), (0.39, 0.935, 'ouro'), (0.355, 0.90, None),
              (0.31, 0.84, None), (0.0, 0.74, None)]
    c.torno([(r * k, h * k, a) for r, h, a in perfil], V(x, y, z), 'ouro', lados=24)
    alcas(c, V(x, y, z), V(0, 0, 1), k, ((0.17, 0.47), (0.30, 0.46), (0.45, 0.52), (0.53, 0.65), (0.53, 0.79), (0.47, 0.89),
                                          (0.385, 0.905)), 0.026, lados=8, por_trecho=3)


PLACAS = (('COPA PISTÃO', '2005'), ('ESTREANTE', 'DO ANO'), ('DINOCO 400', '1º LUGAR'), ('COPA PISTÃO', '2006'))


def placa(c, x, y, z, s, linhas=PLACAS[0]):
    """A placa de madeira com a plaquinha dourada gravada, encostada na parede (s: o lado da parede)"""
    direita = V(0, 0, s)
    cima = V(s * 0.10, 1, 0).normalized()
    frente = direita.cross(cima)
    o = V(x + s * 0.015, y, z)
    c.prisma([(-0.13, 0.0), (0.13, 0.0), (0.13, 0.34), (-0.13, 0.34)], 0.025, o, direita, cima, 'madeira')
    c.prisma([(-0.09, 0.08), (0.09, 0.08), (0.09, 0.22), (-0.09, 0.22)], 0.006, o + frente * 0.025, direita, cima, 'ouro')
    na_placa = o + frente * 0.0325
    c.texto(linhas[0], FONTE_FORTE, 0.032, na_placa + cima * 0.183, direita, cima, 'laca_preta', profundidade=0.0, espaco=1.1,
            largura_max=0.15, resolucao=1)
    c.texto(linhas[1], FONTE_FORTE, 0.052, na_placa + cima * 0.128, direita, cima, 'laca_preta', profundidade=0.0, largura_max=0.13,
            resolucao=1)


def carrinho(c, x, y, z, giro, comprimento=0.36):
    """Um carrinho de brinquedo do McQueen: a carroceria vermelha (o perfil de lado), as janelas escuras, as quatro
    rodas pretas e o aerofólio. giro: o ângulo (graus) do bico, a partir de +z, em volta de y"""
    L = comprimento
    a = math.radians(giro)
    frente = V(math.sin(a), 0, math.cos(a))
    lado = V(math.cos(a), 0, -math.sin(a))          # (frente x cima = -lado: o prisma sai de +lado para -lado)
    base = V(x, y, z)
    meia = 0.21 * L
    perfil = [(-0.50, 0.07), (0.50, 0.07), (0.50, 0.17), (0.40, 0.21), (0.16, 0.25), (0.02, 0.36), (-0.24, 0.36),
              (-0.38, 0.26), (-0.50, 0.24)]
    c.prisma([(p * L, h * L) for p, h in perfil], 2 * meia, base + lado * meia, frente, V(0, 1, 0), 'laca_vermelha', fundo=True)
    for e in (-1, 1):
        o = base + lado * (e * (meia + 0.001))
        q = [(0.12, 0.255), (0.01, 0.345), (-0.20, 0.345), (-0.30, 0.265)]
        c.poligono_virado([o + frente * (p * L) + V(0, h * L, 0) for p, h in q], lado * e, 'vidro_escuro')
        for pz in (-0.32, 0.30):
            c.cilindro(base + frente * (pz * L) + V(0, 0.085 * L, 0) + lado * (e * (meia - 0.004)), lado * e, 0.085 * L, 0.022,
                       'borracha', lados=10)
        n = lado * e
        c.adesivo('95', base + frente * (-0.05 * L) + V(0, 0.17 * L, 0) + n * (meia + 0.0015), V(0, 1, 0).cross(n), V(0, 1, 0),
                  0.075 * L / 0.36, 'disco')
    q = base + frente * (-0.47 * L) + V(0, 0.29 * L, 0)
    c.prisma([(-0.06 * L, 0.0), (0.04 * L, 0.0), (0.04 * L, 0.025 * L), (-0.06 * L, 0.025 * L)], 2 * meia, q + lado * meia, frente,
             V(0, 1, 0), 'laca_preta', fundo=True)


def luz(tipo, pos, direcao, cor, potencia, tamanho, eixo=None, angulo=80.0):
    return dict(tipo=tipo, pos=Vector(pos), dir=Vector(direcao).normalized(), cor=cor, potencia=potencia, tamanho=tamanho, eixo=eixo,
                angulo=angulo)


def montar(c, caixas_de_roda, imagem_da_tela):
    W, H, F = MEIA_LARGURA, ALTURA, FUNDO
    luzes = []

    # ---- O piso, a soleira e o lado de baixo do piso (os dois furados nas caixas de roda, que passam pelas rodas) ----
    def na_roda(p, folga=0.0):
        """p (no chão ou embaixo dele) está numa caixa de roda: entre a frente da caixa e a carroceria"""
        return any(p.x * cx['lado'] > cx['x'] + folga and cx['z0'] < p.z < cx['z1'] for cx in caixas_de_roda)

    quebras_x = [W - abs(cx['x']) for cx in caixas_de_roda] + [W + abs(cx['x']) for cx in caixas_de_roda]
    quebras_z = [q for cx in caixas_de_roda for q in (cx['z0'], cx['z1'])] + [0.08]
    c.grade(V(-W, 0, 0), V(0, 0, F), V(2 * W, 0, 0), 'chapa', passo=PASSO_DO_PISO, quebras_u=quebras_z, quebras_v=quebras_x,
            uv=('mundo', METROS_DA_CHAPA), pular=na_roda)
    c.caixa(-W, W, 0.0, 0.010, 0.0, 0.08, 'cromo', passo=0.5, sem=('-y', '+z'))
    quebras_x = [CASCA_DO_PISO - abs(cx['x']) - 0.015 for cx in caixas_de_roda] + [CASCA_DO_PISO + abs(cx['x']) + 0.015 for cx in caixas_de_roda]
    c.grade(V(-CASCA_DO_PISO, -0.02, 0.02), V(2 * CASCA_DO_PISO, 0, 0), V(0, 0, F - 0.02), 'escuro', passo=4.0, quebras_u=quebras_x,
            quebras_v=[q - 0.02 for q in quebras_z], pular=lambda p: na_roda(p, 0.015))

    # ---- O batente: um portal (virado para fora, logo atrás do vão da porta do Mack) que fecha o que fica entre o vão e
    # o lado de dentro (as frestas das paredes, a de cima e a de baixo do piso); a quina de cima é chanfrada como a da
    # carroceria. As pontas dos armários, da sanca e da viga ficam no mesmo plano (ZB). ----
    para_fora = V(0, 0, -1)
    Y_DO_LADO = 5.0
    for s in (-1, 1):
        (qx0, qy0), (qx1, qy1) = QUINA_DA_CARROCERIA[s]
        nq = Vector((qy1 - qy0, qx0 - qx1)).normalized()
        cq = nq.x * qx0 + nq.y * qy0 - FOLGA_DA_QUINA
        y_da_quina = (cq - nq.x * CASCA) / nq.y
        x_da_quina = (cq - nq.y * ALTO_DA_CASCA) / nq.x
        x0, x1 = sorted((s * W, s * CASCA))
        c.grade(V(x0, BAIXO_DO_BATENTE, ZB), V(0, Y_DO_LADO - BAIXO_DO_BATENTE, 0), V(x1 - x0, 0, 0), 'preto', passo=0.5)
        c.poligono_virado([V(s * W, Y_DO_LADO, ZB), V(s * CASCA, Y_DO_LADO, ZB), V(s * CASCA, y_da_quina, ZB),
                           V(s * x_da_quina, ALTO_DA_CASCA, ZB), V(0, ALTO_DA_CASCA, ZB), V(0, H, ZB), V(s * W, H, ZB)], para_fora, 'preto')
    c.grade(V(-CASCA, BAIXO_DO_BATENTE, ZB), V(0, -BAIXO_DO_BATENTE, 0), V(2 * CASCA, 0, 0), 'preto', passo=0.5)

    # ---- O céu das estrelas, o degrau do fundo dele, o chanfro do fundo com o caixotão ----
    c.grade(V(-MIOLO, H, ZB), V(2 * MIOLO, 0, 0), V(0, 0, FIM_DO_CEU - ZB), 'ceu', passo=0.25)
    c.grade(V(-MIOLO, VIGA[1][1], FIM_DO_CEU), V(0, H - VIGA[1][1], 0), V(2 * MIOLO, 0, 0), 'preto', passo=0.25)
    rng = np.random.default_rng(95)
    estrelas = []
    tentativas = 0
    while len(estrelas) < 1000 and tentativas < 30000:
        tentativas += 1
        p = (rng.uniform(-MIOLO + 0.05, MIOLO - 0.05), rng.uniform(ZB + 0.08, FIM_DO_CEU - 0.06))
        if all((p[0] - q[0]) ** 2 + (p[1] - q[1]) ** 2 > 0.055 ** 2 for q in estrelas[-300:]):
            estrelas.append(p)
    for (ex, ez) in estrelas:
        k = int(rng.choice(3, p=PESO_DAS_ESTRELAS))
        r = float(np.exp(rng.uniform(math.log(0.010), math.log(0.022))))
        if rng.uniform() < 0.06:
            r *= 1.5
        pts = [V(ex + r * math.cos(2 * math.pi * i / 6 + 0.3), H, ez + r * math.sin(2 * math.pi * i / 6 + 0.3)) for i in range(6)]
        c.chapa_acesa(pts, V(0, -1, 0), f'estrela_{k}', ESTRELAS[k], float(rng.uniform(0.25, 0.95)), afasta=0.0015)

    # o chanfro do fundo: um trapézio com um caixotão no meio (furado no trapézio)
    (xb, yb), (xt, yt) = VIGA
    def no_chanfro(u, t):
        """u: -1 a 1 (de um lado ao outro, na largura do trapézio naquela altura); t: 0 (embaixo, na parede do fundo) a 1"""
        meia = xb + (xt - xb) * t
        return V(u * meia, yb + (yt - yb) * t, F - (F - FIM_DO_CEU) * t)
    normal_do_chanfro = V(0, -(F - FIM_DO_CEU), -(yt - yb)).normalized()
    t0, t1, ux = 0.22, 0.80, 0.62           # o caixotão: de t0 a t1, e |x| até ux
    # as quatro partes em volta do furo
    c.grade4(no_chanfro(1, 0), no_chanfro(-1, 0), no_chanfro(-1, t0), no_chanfro(1, t0), 'grafite', passo=PASSO)
    c.grade4(no_chanfro(1, t1), no_chanfro(-1, t1), no_chanfro(-1, 1), no_chanfro(1, 1), 'grafite', passo=PASSO)
    for s in (-1, 1):
        def x_em(t, sx=s):
            return V(sx * ux, yb + (yt - yb) * t, F - (F - FIM_DO_CEU) * t)
        a, b2 = no_chanfro(s, t0), no_chanfro(s, t1)
        q = [a, x_em(t0), x_em(t1), b2]
        c.poligono_virado(q, normal_do_chanfro, 'grafite')
    furo = [V(ux, *no_chanfro(0, t0).yz), V(-ux, *no_chanfro(0, t0).yz), V(-ux, *no_chanfro(0, t1).yz), V(ux, *no_chanfro(0, t1).yz)]
    caixotao(c, furo, normal_do_chanfro, ordem=ordem_por_z(F))

    # ---- As paredes ----
    for s in (-1, 1):
        xp = s * W
        de_dentro = '+x' if s > 0 else '-x'      # a face de uma peça encostada na parede, que não aparece
        para_o_meio = '-x' if s > 0 else '+x'    # a face virada para o meio do baú
        roda = next((cx for cx in caixas_de_roda if cx['lado'] == s), None)
        direita_da_parede = V(0, 0, 1) if s > 0 else V(0, 0, -1)    # quem olha a parede tem o +z à direita no lado +x

        def parede(y0, y1, acab, recuo=0.0, z0=0.0, z1=F, passo=PASSO, uv=None, quebras_z=(), quebras_y=()):
            x = xp - s * recuo
            if s > 0:
                c.grade(V(x, y0, z0), V(0, 0, z1 - z0), V(0, y1 - y0, 0), acab, passo, quebras_u=[q - z0 for q in quebras_z],
                        quebras_v=[q - y0 for q in quebras_y], uv=uv)
            else:
                c.grade(V(x, y0, z0), V(0, y1 - y0, 0), V(0, 0, z1 - z0), acab, passo, quebras_v=[q - z0 for q in quebras_z],
                        quebras_u=[q - y0 for q in quebras_y], uv=uv)

        uvp = ('mundo', METROS_DA_PERFURADA)
        uvc = ('mundo', METROS_DA_CHAPA)
        y_armario = ARMARIO[0][1]
        y_armario_porta = ARMARIO_DA_PORTA[0][1]
        # a parede de baixo (chapa perfurada), em volta da caixa de roda e embaixo dos armários; o rodapé preto
        trechos = [(0.0, CAIXA_DA_PORTA, 0.0, y_armario_porta)]
        if roda:
            trechos += [(CAIXA_DA_PORTA, roda['z0'], 0.0, y_armario), (roda['z0'], roda['z1'], roda['topo'], y_armario),
                        (roda['z1'], F, 0.0, y_armario)]
        else:
            trechos += [(CAIXA_DA_PORTA, F, 0.0, y_armario)]
        for (za, zc, ya, yc) in trechos:
            parede(max(ya, RODAPE if ya == 0.0 else ya), yc, 'perfurada', z0=za, z1=zc, uv=uvp)
            if ya == 0.0:
                c.caixa(xp - s * 0.015, xp, 0.0, RODAPE, za, zc, 'preto', sem=(de_dentro, '-y', '-z', '+z'))
        # a parede dos troféus (atrás da prateleira, até a sanca)
        parede(PRATELEIRA, SANCA[0][1], 'perfurada', z0=0.0, z1=F, uv=uvp)

        # o armário: o fundo inclinado, a frente e a prateleira (perto da porta, maior)
        (u0, y0), (u1, y1), (u2, y2), (u3, y3) = ARMARIO
        c.faixa(s, (u0, y0), (u1, y1), CAIXA_DA_PORTA, F, 'grafite')
        c.faixa(s, (u1, y1), (u2, y2), CAIXA_DA_PORTA, F, 'grafite')
        (v0, w0), (v1, w1), (v2, w2), (v3, w3) = ARMARIO_DA_PORTA
        c.faixa(s, (v0, w0), (v1, w1), ZB, CAIXA_DA_PORTA, 'grafite')
        c.faixa(s, (v1, w1), (v2, w2), ZB, CAIXA_DA_PORTA, 'grafite')
        c.faixa(s, (u2, y2), (u3, y3), ZB, F, 'laca_preta', passo=0.25)
        c.caixa(s * (u2 - 0.012), s * u2, y2 - 0.004, y2 + 0.012, ZB, F, 'cromo', passo=1.0, sem=('-z', '+z', de_dentro, '-y'))
        # o degrau entre a caixa da porta e o armário (virado para o fundo) e a ponta da caixa da porta (para a porta)
        c.poligono_virado([V(s * v0, w0, CAIXA_DA_PORTA), V(s * v1, w1, CAIXA_DA_PORTA), V(s * u1, y1, CAIXA_DA_PORTA),
                           V(s * u0, y0, CAIXA_DA_PORTA)], V(0, 0, 1), 'grafite')
        c.poligono_virado([V(s * u, y, ZB) for u, y in ARMARIO_DA_PORTA], para_fora, 'grafite')
        # o neon da frente do armário (embaixo da beira) e o do portal da caixa da porta
        nf = [V(s * (u1 - RAIO_DO_NEON - 0.002), w1 + 0.016, ZB + 0.03), V(s * (u1 - RAIO_DO_NEON - 0.002), w1 + 0.016, CAIXA_DA_PORTA),
              V(s * (u1 - RAIO_DO_NEON - 0.002), y1 + 0.016, CAIXA_DA_PORTA), V(s * (u1 - RAIO_DO_NEON - 0.002), y1 + 0.016, F - 0.03)]
        c.neon(nf)
        perfil = deslocar([(v0, w0), (v1, w1), (v2, w2)], RAIO_DO_NEON + 0.004)
        c.neon([V(s * u, y, ZB + 0.035) for u, y in perfil])
        # os painéis de luz no fundo inclinado (e um grande no da caixa da porta)
        def painel(p0, p1, z0, z1, f0, f1):
            d = Vector((p1[0] - p0[0], p1[1] - p0[1]))
            ls = d.length
            n2 = Vector((-d.y, d.x)) / ls             # o lado aberto, em (|x|, y)
            normal = V(s * n2.x, n2.y, 0)
            meio_t, meio_z = (f0 + f1) / 2 * ls, (z0 + z1) / 2
            pts = []
            for (pt, pz) in retangulo_redondo((f1 - f0) / 2 * ls, (z1 - z0) / 2, 0.05, por_canto=3):
                t = (meio_t + pt) / ls
                pts.append(V(s * (p0[0] + d.x * t), p0[1] + d.y * t, meio_z + pz))
            c.chapa_acesa(pts, normal, 'painel', COR_DO_PAINEL, ordem_por_z(meio_z))
        z = PAINEIS[0]
        while z + PAINEIS[1] < F - 0.05:
            painel(ARMARIO[0], ARMARIO[1], z, z + PAINEIS[1], PAINEIS[3], PAINEIS[4])
            z += PAINEIS[1] + PAINEIS[2]
        painel(ARMARIO_DA_PORTA[0], ARMARIO_DA_PORTA[1], ZB + 0.10, CAIXA_DA_PORTA - 0.10, 0.12, 0.88)

        # a sanca, com os spots
        (a0, b0), (a1, b1), (a2, b2) = SANCA
        c.faixa(s, (a0, b0), (a1, b1), ZB, F, 'grafite')
        c.faixa(s, (a1, b1), (a2, b2), ZB, F, 'preto', passo=0.25)
        xs, z, passo_s = SPOTS
        while z < F - 0.15:
            centro = V(s * xs, b0, z)
            c.cilindro(centro + V(0, -0.016, 0), V(0, 1, 0), 0.056, 0.016, 'cromo', lados=12, tampa_de_cima=False)
            spot(c, centro + V(0, -0.016, 0), ordem_por_z(z))
            alvo = V(s * (W - 0.05), PRATELEIRA + 0.2, z)
            foco = centro + V(0, -0.04, 0)
            luzes.append(luz('spot', foco, alvo - foco, (1.0, 0.76, 0.45), 26.0, 0.03, angulo=95.0))
            z += passo_s

        # a viga dos caixotões (o retângulo até o fim do céu, furado nos caixotões, e o triângulo da esquadria) e o degrau
        (vb_u, vb_y), (vt_u, vt_y) = VIGA
        ls = math.hypot(vb_u - vt_u, vt_y - vb_y)
        f0, f1 = CAIXOTAO_NA_VIGA
        comp, nerv, z_cx = CAIXOTAO
        zs = [z_cx + i * (comp + nerv) for i in range(CAIXOTOES)]

        def no_caixotao(p):
            t = (p.y - vb_y) / (vt_y - vb_y)
            return f0 < t < f1 and any(zz < p.z < zz + comp for zz in zs)
        c.faixa(s, VIGA[0], VIGA[1], ZB, FIM_DO_CEU, 'grafite', quebras_p=[f0 * ls, f1 * ls],
                quebras_z=[q for zz in zs for q in (zz, zz + comp)], pular=no_caixotao)
        normal_da_viga = V(-s * (vt_y - vb_y), -(vb_u - vt_u), 0).normalized()
        c.poligono_virado([V(s * vb_u, vb_y, FIM_DO_CEU), V(s * vb_u, vb_y, F), V(s * vt_u, vt_y, FIM_DO_CEU)], normal_da_viga, 'grafite')
        for zz in zs:
            def na_viga(t, zq):
                return V(s * (vb_u + (vt_u - vb_u) * t), vb_y + (vt_y - vb_y) * t, zq)
            caixotao(c, [na_viga(f0, zz), na_viga(f1, zz), na_viga(f1, zz + comp), na_viga(f0, zz + comp)], normal_da_viga)
        c.faixa(s, VIGA[1], (MIOLO, H), ZB, FIM_DO_CEU, 'preto', passo=0.25)

        # a ponta de cima, virada para a porta (o portal), com o neon contornando o corte (e passando pelo teto)
        corte = [SANCA[0], SANCA[1], SANCA[2], VIGA[1], (MIOLO, H), (W, H)]
        c.poligono_virado([V(s * u, y, ZB) for u, y in corte], para_fora, 'grafite')

        # o neon do rodapé: pela parede, contornando a caixa da roda
        yn = RODAPE + RAIO_DO_NEON + 0.004
        un = W - 0.015 - RAIO_DO_NEON - 0.003
        caminho = [V(s * un, yn, 0.10)]
        if roda:
            ui = abs(roda['x']) - RAIO_DO_NEON - 0.003
            caminho += [V(s * un, yn, roda['z0'] - 0.03), V(s * ui, yn, roda['z0'] - 0.03), V(s * ui, yn, roda['z1'] + 0.03),
                        V(s * un, yn, roda['z1'] + 0.03)]
        caminho.append(V(s * un, yn, F - 0.03))
        c.neon(caminho)

        # o raio: o painel de chapa xadrez (2,5 cm) e o neon em volta dele, encostado na parede
        z0r, compr, y0r, altr = RAIO_NA_PAREDE
        contorno = raio_mapeado(z0r, compr, y0r, altr)        # (z, y)
        if s > 0:
            pontos2d = [(z, y) for z, y in contorno]          # direita = +z, cima = +y
            origem = V(xp, 0, 0)
        else:
            pontos2d = [(-z, y) for z, y in contorno]         # direita = -z
            origem = V(xp, 0, 0)
        c.prisma(pontos2d, 0.025, origem, direita_da_parede, V(0, 1, 0), 'chapa', acab_lado='grafite', uv=uvc)
        sentido = 1.0 if area2d(contorno) > 0 else -1.0
        fora = deslocar(contorno, -sentido * (RAIO_DO_NEON + 0.004), fechado=True)
        c.neon([V(xp - s * (RAIO_DO_NEON + 0.003), y, z) for z, y in fora], fechado=True)

        # o painel de adesivos embaixo da ponta do raio, perto do fundo
        zp0, zp1, yp0, yp1 = 8.55, 10.75, 0.30, 0.92
        c.caixa(xp - s * 0.03, xp, yp0, yp1, zp0, zp1, 'grafite', passo=0.3, sem=(de_dentro,))
        frente_p = V(-s, 0, 0)
        xa = xp - s * 0.033
        itens = (('95', 'disco', 0.42, 8.95), ('rusteze', 'oval', 0.62, 9.68), ('raio', 'raio', 0.36, 10.38))
        for nome, forma, larg, zz in itens:
            alt = larg * (0.48 if forma == 'oval' else 1.0)
            c.adesivo(nome, V(xa, (yp0 + yp1) / 2, zz), direita_da_parede, V(0, 1, 0), larg if forma != 'oval' else larg, forma,
                      altura=alt if forma != 'raio' else 0.50)

        # os adesivos na ponta da caixa da porta (virada para fora)
        frente_c = V(0, 0, -1)
        dir_c = V(-1, 0, 0)
        zc_ad = ZB - 0.002
        if s > 0:
            c.adesivo('95', V(s * 1.67, 2.57, zc_ad), dir_c, V(0, 1, 0), 0.22, 'disco')
            c.adesivo('rusteze', V(s * 1.76, 2.16, zc_ad), dir_c, V(0, 1, 0), 0.26, 'oval', altura=0.125)
        else:
            c.adesivo('dinoco', V(s * 1.67, 2.57, zc_ad), dir_c, V(0, 1, 0), 0.22, 'disco')
            c.adesivo('raio', V(s * 1.78, 2.17, zc_ad), dir_c, V(0, 1, 0), 0.16, 'raio', altura=0.22)

        # a caixa da roda (grafite, de chapa perfurada, com o tampo de chapa xadrez) e o poço escuro (visto de fora)
        if roda is not None:
            xi, z0, z1, yt = s * roda['x'], roda['z0'], roda['z1'], roda['topo']
            yc = yt - CHANFRO_DA_CAIXA
            xch = xi + s * CHANFRO_DA_CAIXA
            c.caixa(xi, xp, 0.0, yc, z0, z1, 'perfurada', passo=PASSO, sem=(de_dentro, '-y', '+y'), uv=uvp)
            c.caixa(xch, xp, yc, yt, z0, z1, 'perfurada', passo=PASSO, sem=(de_dentro, '-y', para_o_meio), acabs={'+y': 'chapa'},
                    uv={'perfurada': uvp, 'chapa': uvc})
            ao_longo, de_lado = V(0, 0, z1 - z0), V(s * CHANFRO_DA_CAIXA, CHANFRO_DA_CAIXA, 0)
            if s > 0:
                c.grade(V(xi, yc, z0), ao_longo, de_lado, 'chapa', passo=PASSO, uv=uvc)
            else:
                c.grade(V(xi, yc, z0), de_lado, ao_longo, 'chapa', passo=PASSO, uv=uvc)
            for (zz, n) in ((z0, V(0, 0, -1)), (z1, V(0, 0, 1))):
                c.poligono_virado([V(xi, yc, zz), V(xch, yc, zz), V(xch, yt, zz)], n, 'perfurada',
                                  uvs=[(p.z / METROS_DA_PERFURADA, p.y / METROS_DA_PERFURADA) for p in
                                       (V(xi, yc, zz), V(xch, yc, zz), V(xch, yt, zz))])
            c.caixa(xi - s * 0.010, xi + s * 0.012, yc - 0.012, yc + 0.010, z0 - 0.010, z1 + 0.010, 'grafite', passo=0.6, sem=(de_dentro,))
            c.caixa(xch - s * 0.012, xch + s * 0.010, yt - 0.010, yt + 0.008, z0 - 0.010, z1 + 0.010, 'grafite', passo=0.6, sem=(de_dentro,))
            # o adesivo da Rust-eze (ou o do McQueen) na ponta virada para a porta
            largura_ad = min(abs(xp - xi) - 0.16, 0.56)
            centro = V((xi + xp) / 2, yc / 2 + 0.03, z0 - 0.003)
            if s > 0:
                c.adesivo('rusteze', centro, V(-1, 0, 0), V(0, 1, 0), largura_ad, 'oval', altura=largura_ad * 0.48)
            else:
                c.adesivo('mcqueen', centro, V(-1, 0, 0), V(0, 1, 0), largura_ad, 'retangulo')
            xc = s * CASCA_NAS_RODAS
            topo_do_poco = yc - 0.01
            c.caixa(xi + s * 0.015, xc, BAIXO_DA_CASCA, topo_do_poco, z0 + 0.03, z1 - 0.03, 'escuro', passo=2.0, por_dentro=True,
                    sem=('-y', de_dentro))
            tira = V(0, yt + 0.17 - topo_do_poco, 0)
            if s > 0:
                c.grade(V(xc - 0.001, topo_do_poco, z0 + 0.03), tira, V(0, 0, z1 - z0 - 0.06), 'escuro', passo=2.0)
            else:
                c.grade(V(xc + 0.001, topo_do_poco, z0 + 0.03), V(0, 0, z1 - z0 - 0.06), tira, 'escuro', passo=2.0)

        # ---- A prateleira: os pôsteres (em molduras douradas), as placas redondas, os troféus e os carrinhos ----
        xq = xp - s * 0.022
        pl, pa, py = POSTER
        for k, zq in enumerate(POSTERES_EM_Z):
            numero = POSTERES_DA_PAREDE[s][k]
            c.grade(V(xq, py, zq - s * pl / 2), direita_da_parede * pl, V(0, pa, 0), 'poster', passo=0.32,
                    uv=('atlas', lugar_do_poster(numero)))
            m = 0.032
            for (ya, yb_, za, zb_) in ((py - m, py, zq - pl / 2 - m, zq + pl / 2 + m), (py + pa, py + pa + m, zq - pl / 2 - m, zq + pl / 2 + m),
                                       (py, py + pa, zq - pl / 2 - m, zq - pl / 2), (py, py + pa, zq + pl / 2, zq + pl / 2 + m)):
                c.caixa(xp - s * 0.038, xp, ya, yb_, za, zb_, 'ouro', passo=0.5, sem=(de_dentro,))
            # os carrinhos na frente do pôster
            for dz, giro in ((-0.20, 160 if s > 0 else 200), (0.20, 25 if s > 0 else -25)):
                carrinho(c, s * 1.60, PRATELEIRA, zq + dz, giro + (s * 15))
        meios = [0.48] + [(a + b) / 2 for a, b in zip(POSTERES_EM_Z[:-1], POSTERES_EM_Z[1:])] + [11.48]
        z_placa, qual = PLACAS_REDONDAS[s]
        xt_ = s * 1.68
        for k, zm in enumerate(meios):
            tipo = (k + (0 if s > 0 else 2)) % 4
            if k == len(meios) - 1 and s > 0:
                copa_pistao(c, xt_, PRATELEIRA, zm, k=0.55)
            elif k == 0:
                trofeu_tigela(c, xt_, PRATELEIRA, zm, 0.34, base='laca_preta')
            elif tipo == 0:
                trofeu_copa(c, xt_, PRATELEIRA, zm, 0.56)
            elif tipo == 1:
                trofeu_coluna(c, xt_, PRATELEIRA, zm, 0.52, frente=V(-s, 0, 0))
            elif tipo == 2:
                trofeu_tigela(c, xt_, PRATELEIRA, zm, 0.46)
            else:
                trofeu_copa(c, xt_, PRATELEIRA, zm, 0.44, base='madeira', direcao_das_alcas=None)
            for dz in (-0.42, 0.42):
                zz = zm + dz
                if zz < ZB + 0.15 or zz > F - 0.15:
                    continue
                tipo2 = (k * 2 + (1 if dz > 0 else 0) + (1 if s > 0 else 0)) % 3
                if tipo2 == 0:
                    placa(c, xp - s * 0.065, PRATELEIRA, zz, s, PLACAS[(k + (s > 0)) % len(PLACAS)])
                elif tipo2 == 1:
                    trofeu_tigela(c, s * 1.70, PRATELEIRA, zz, 0.30, base='laca_preta')
                else:
                    trofeu_coluna(c, s * 1.70, PRATELEIRA, zz, 0.34, frente=V(-s, 0, 0))
            if abs(zm - z_placa) < 0.01:
                centro = V(xp - s * 0.004, PRATELEIRA + 0.86, zm)
                c.cilindro(centro, V(-s, 0, 0), 0.21, 0.03, 'cromo', lados=24)
                c.adesivo(qual, centro + V(-s * 0.033, 0, 0), direita_da_parede, V(0, 1, 0), 0.38, 'disco')

    # ---- O teclado (na parede do +x, perto da porta, numa cunha virada para a porta) ----
    s = 1
    ang = math.radians(35)
    frente_t = V(-math.cos(ang), 0, -math.sin(ang))
    direita_t = V(-math.sin(ang), 0, math.cos(ang))
    lt, at, y0t = 0.30, 0.60, 0.92
    canto = V(W, y0t, 0.30)                    # a quina da cunha encostada na parede (do lado da porta)
    a_ = canto
    b_ = canto + direita_t * lt
    c_ = V(W, y0t, b_.z)
    for (yy, n) in ((y0t, V(0, -1, 0)), (y0t + at, V(0, 1, 0))):
        c.poligono_virado([V(p.x, yy, p.z) for p in (a_, b_, c_)], n, 'bege')
    c.poligono_virado([V(b_.x, y0t, b_.z), V(c_.x, y0t, c_.z), V(c_.x, y0t + at, c_.z), V(b_.x, y0t + at, b_.z)], V(0, 0, 1), 'bege')
    cantos = [a_, b_, b_ + V(0, at, 0), a_ + V(0, at, 0)]
    c.poligono_virado(cantos, frente_t, 'bege')
    casa = ADESIVOS['teclado']
    m = 0.015
    pts = [a_ + direita_t * m + V(0, m, 0), b_ - direita_t * m + V(0, m, 0), b_ - direita_t * m + V(0, at - m, 0),
           a_ + direita_t * m + V(0, at - m, 0)]
    pts = [p + frente_t * 0.002 for p in pts]
    c.poligono_virado(pts, frente_t, 'poster', [uv_na_casa(casa, 0, 1), uv_na_casa(casa, 1, 1), uv_na_casa(casa, 1, 0),
                                                uv_na_casa(casa, 0, 0)])

    # ---- O portal: o neon do corte de cima, de uma parede à outra, passando pelo teto ----
    corte = [SANCA[0], SANCA[1], SANCA[2], VIGA[1], (MIOLO, H)]
    lado_dir = deslocar(corte + [(-MIOLO, H)], RAIO_DO_NEON + 0.004)[:-1]
    arco = [V(u, y, ZB + 0.035) for u, y in lado_dir] + [V(-u, y, ZB + 0.035) for u, y in reversed(lado_dir)]
    c.neon(arco)

    # ---- O fundo ----
    zf = F
    # a parede do fundo (chapa perfurada) até o chanfro
    c.grade(V(W, 0.0, zf), V(-2 * W, 0, 0), V(0, VIGA[0][1], 0), 'perfurada', passo=PASSO, uv=('mundo', METROS_DA_PERFURADA))
    c.grade(V(W, 0.0, zf - 0.015), V(-2 * W, 0, 0), V(0, RODAPE, 0), 'preto', passo=0.5)
    # os raios dos cantos (em pé, a ponta para baixo), em painéis de chapa xadrez com o neon em volta
    ui, lr, y0b, ab = RAIO_NO_FUNDO
    for s in (-1, 1):
        contorno = [(s * (ui + bx * lr), y0b + by * ab) for bx, by in RAIO_CLASSICO]      # (x, y)
        # no plano da parede do fundo: direita (de quem olha para o fundo) = -x, cima = +y
        pontos2d = [(-x, y) for x, y in contorno]
        c.prisma(pontos2d, 0.025, V(0, 0, zf), V(-1, 0, 0), V(0, 1, 0), 'chapa', acab_lado='grafite', uv=('mundo', METROS_DA_CHAPA))
        sentido = 1.0 if area2d(contorno) > 0 else -1.0
        fora = deslocar(contorno, -sentido * (RAIO_DO_NEON + 0.004), fechado=True)
        c.neon([V(x, y, zf - RAIO_DO_NEON - 0.003) for x, y in fora], fechado=True)
    # o painel do 95: cinza, saindo da parede, com o 95 em relevo e as bandeiras quadriculadas
    pl95, py0, py1, pp = PAINEL_95
    c.caixa(-pl95, pl95, py0, py1, zf - pp, zf, 'cinza', passo=0.2, sem=('+z',))
    zf95 = zf - pp
    direita, cima = V(-1, 0, 0), V(0, 1, 0)
    c.texto('95', FONTE_TITULO, 1.05, V(0.04, 1.72, zf95), direita, cima, 'cinza', profundidade=0.06, inclina=0.14,
            acab_lado='cinza_escuro', largura_max=1.25, resolucao=3)
    for s in (-1, 1):
        # a bandeira: um pano em ondas de 8 x 5 casas, as casas pretas em relevo, no mastro inclinado
        base = V(s * 0.08, 0.42, zf95)
        topo_m = V(s * 0.50, 1.30, zf95)
        c.tubo([base + V(0, 0, -0.02), topo_m + V(0, 0, -0.02)], 0.018, 'cinza_escuro', lados=6)
        eixo_u = V(s * 1.0, 0.12, 0).normalized()
        eixo_v = (topo_m - base).normalized()
        nu, nv, tam = 7, 5, 0.10
        for i in range(nu):
            for j in range(nv):
                onda = 0.03 * math.sin(i * 0.9 + j * 0.2)
                o = topo_m + eixo_u * (i * tam) - eixo_v * (j * tam) + V(0, onda, 0)
                alto = 0.012 if (i + j) % 2 == 0 else 0.003
                pts = [o, o + eixo_u * tam, o + eixo_u * tam - eixo_v * tam, o - eixo_v * tam]
                pts = [p + V(0, 0, -alto) for p in pts]
                c.poligono_virado(pts, V(0, 0, -1), 'cinza' if (i + j) % 2 == 0 else 'cinza_escuro')
                if (i + j) % 2 == 0:
                    for a, b in ((0, 1), (1, 2), (2, 3), (3, 0)):
                        q = [pts[a], pts[b], pts[b] + V(0, 0, alto - 0.003), pts[a] + V(0, 0, alto - 0.003)]
                        meio = (pts[a] + pts[b]) / 2 - (pts[0] + pts[2]) / 2
                        c.poligono_virado(q, V(meio.x, meio.y, 0), 'cinza_escuro')
    # a luz rasante de cima no painel do 95 (escondida embaixo do console)
    cl, cy0, cy1, cp = CONSOLE
    luzes.append(luz('area', V(0, cy0 - 0.05, zf - cp - 0.10), V(0, -1, 0.55), (0.80, 0.86, 1.0), 30.0, (2.2, 0.06), eixo=V(1, 0, 0)))

    # o console: a caixa de cantos redondos, a frente preta, o neon em volta, a tela e as caixas de som
    zc0 = zf - cp
    meio_c = V(0, (cy0 + cy1) / 2, zc0)
    contorno_c = retangulo_redondo(cl, (cy1 - cy0) / 2, 0.16, por_canto=5)
    frente_c = [V(-x, meio_c.y + y, zc0) for x, y in contorno_c]
    tras_c = [V(-x, meio_c.y + y, zf) for x, y in contorno_c]
    nc = len(frente_c)
    for i in range(nc):
        j = (i + 1) % nc
        meio_aresta = (frente_c[i] + frente_c[j]) / 2 - meio_c
        c.poligono_virado([frente_c[i], frente_c[j], tras_c[j], tras_c[i]], V(meio_aresta.x, meio_aresta.y, 0), 'grafite')
    c.poligono_virado(frente_c, V(0, 0, -1), 'preto')
    borda = [V(-x, meio_c.y + y, zc0 - RAIO_DO_NEON - 0.002) for x, y in retangulo_redondo(cl - 0.035, (cy1 - cy0) / 2 - 0.035, 0.13, 5)]
    c.neon(borda, fechado=True, liso=True, ordem=0.86)
    # a tela (um pouco para dentro, numa moldura)
    tl, ty0, ty1 = TELA
    zt = zc0 - 0.016                      # a frente da moldura da tela
    c.caixa(-tl - 0.025, tl + 0.025, ty0 - 0.025, ty1 + 0.025, zt, zc0, 'grafite', passo=0.5, sem=('+z',))
    zs = zt - 0.002                       # a tela, 2 mm à frente da moldura
    c.grade(V(tl, ty0, zs), V(-2 * tl, 0, 0), V(0, ty1 - ty0, 0), 'tela', passo=0.25, uv=('atlas', LUGAR_DA_TELA))
    # na camada das luzes, a tela em grade, com as cores da imagem nos vértices (a camada não tem textura)
    nx, ny = 128, 80
    alto_img, largo_img = imagem_da_tela.shape[:2]
    vert = []
    for j in range(ny + 1):
        linha = []
        for i in range(nx + 1):
            fx, fy = i / nx, j / ny
            p = V(tl - 2 * tl * fx, ty1 - (ty1 - ty0) * fy, zs - GROSSURA_DA_LUZ)
            px = min(largo_img - 1, max(0, int(fx * (largo_img - 1) + 0.5)))
            py_ = min(alto_img - 1, max(0, int(fy * (alto_img - 1) + 0.5)))
            j0, j1 = max(0, py_ - 2), min(alto_img, py_ + 3)
            i0, i1 = max(0, px - 2), min(largo_img, px + 3)
            cor = imagem_da_tela[j0:j1, i0:i1].reshape(-1, 3).mean(0)
            linha.append((c.luzes.vertice(p), tuple(float(v) for v in cor)))
        vert.append(linha)
    for j in range(ny):
        for i in range(nx):
            q = [vert[j][i], vert[j + 1][i], vert[j + 1][i + 1], vert[j][i + 1]]     # (virada para a porta)
            c.luzes.face([v for v, _ in q], None, 0.92, cores=[cor for _, cor in q])
    # as caixas de som: o aro de neon, o cone e a tampinha cromada
    xsom, ysom, rsom = CAIXAS_DE_SOM
    for s in (-1, 1):
        centro = V(s * xsom, ysom, zc0)
        c.torno([(0.0, 0.028, 'cromo'), (rsom * 0.16, 0.035, None), (rsom * 0.28, 0.056, 'laca_preta'), (rsom * 0.32, 0.055, 'borracha'),
                 (rsom * 0.80, 0.03, None), (rsom * 0.95, 0.0, None)], centro - V(0, 0, 0.001), 'borracha', lados=28, eixo=V(0, 0, 1))
        anel = [centro + V(rsom * math.cos(2 * math.pi * k / 32), rsom * math.sin(2 * math.pi * k / 32), -RAIO_DO_NEON - 0.002)
                for k in range(32)]
        c.neon(anel, fechado=True, liso=True, ordem=0.84)
    luzes.append(luz('area', V(0, (ty0 + ty1) / 2, zs - 0.08), V(0, -0.15, -1), (0.25, 0.45, 1.0), 50.0, (2 * tl, ty1 - ty0),
                     eixo=V(1, 0, 0)))
    # a parede de cima do console, até o chanfro (já na parede do fundo) e o friso do chanfro
    c.caixa(-xb, xb, VIGA[0][1] - 0.03, VIGA[0][1], zf - 0.03, zf, 'preto', passo=0.5, sem=('+z',))
    return luzes


def spot(c, centro, ordem, r_vidro=0.030, r_anel=0.044, lados=16):
    """O spot visto de baixo: o anel aceso (âmbar) e o vidro no meio, mais fraco (no interior e na camada)"""
    def ponto(r, k):
        a = 2 * math.pi * k / lados
        return centro + V(r * math.cos(a), 0, r * math.sin(a))
    c.chapa_acesa([ponto(r_vidro, k) for k in range(lados)], V(0, -1, 0), 'spot_vidro', COR_DO_VIDRO, ordem, afasta=0.001)
    for k in range(lados):
        q = [ponto(r_vidro, k), ponto(r_anel, k), ponto(r_anel, k + 1), ponto(r_vidro, k + 1)]
        c.chapa_acesa(q, V(0, -1, 0), 'spot', COR_DO_SPOT, ordem, afasta=0.001)


def caixotao(c, furo, normal, ordem=None):
    """Um caixotão: o furo (4 cantos num plano, virado para 'normal') afundado FUNDO_DO_CAIXOTAO, com as paredes de
    dentro, o fundo de chapa perfurada e o neon em volta da boca, um pouco para dentro"""
    n = Vector(normal).normalized()
    fundo = [p - n * FUNDO_DO_CAIXOTAO for p in furo]
    meio = sum(furo, Vector((0, 0, 0))) / 4
    for i in range(4):
        j = (i + 1) % 4
        q = [furo[i], furo[j], fundo[j], fundo[i]]
        para_dentro = meio - (furo[i] + furo[j]) / 2
        c.poligono_virado(q, para_dentro, 'preto')
    uvs = []
    for p in fundo:
        a = [abs(v) for v in n]
        if a[1] >= a[0] and a[1] >= a[2]:
            uvs.append((p.x / METROS_DA_PERFURADA, p.z / METROS_DA_PERFURADA))
        elif a[0] >= a[2]:
            uvs.append((p.z / METROS_DA_PERFURADA, p.y / METROS_DA_PERFURADA))
        else:
            uvs.append((p.x / METROS_DA_PERFURADA, p.y / METROS_DA_PERFURADA))
    c.poligono_virado(fundo, n, 'perfurada', uvs)
    # o neon: em volta da boca, por fora do furo, encostado na superfície (a moldura acesa)
    d = RAIO_DO_NEON + 0.004
    anel = []
    for i in range(4):
        p = furo[i]
        a, b = furo[(i - 1) % 4], furo[(i + 1) % 4]
        e1 = (a - p).normalized()
        e2 = (b - p).normalized()
        anel.append(p - (e1 + e2) * d + n * (RAIO_DO_NEON + 0.003))
    c.neon(anel, fechado=True, ordem=ordem)


# ======================================================================
# 5) A luz assada
# ======================================================================
def preparar_cycles(cena):
    cena.render.engine = 'CYCLES'
    try:
        prefs = bpy.context.preferences.addons['cycles'].preferences
        prefs.compute_device_type = 'METAL'
        prefs.get_devices()
        usou = False
        for d in prefs.devices:
            d.use = d.type != 'CPU'
            usou = usou or d.use
        cena.cycles.device = 'GPU' if usou else 'CPU'
    except Exception:
        cena.cycles.device = 'CPU'
    cena.cycles.samples = AMOSTRAS
    cena.cycles.use_denoising = False
    cena.cycles.max_bounces = 6
    cena.cycles.diffuse_bounces = 4
    cena.cycles.glossy_bounces = 1
    cena.cycles.sample_clamp_indirect = 6.0
    print('CARRETA: Cycles no', cena.cycles.device)


def material_do_bake(nome, cor_da_tela):
    tipo, cor, textura, brilho = ACABAMENTOS[nome]
    m = bpy.data.materials.new('assar_' + nome)
    m.use_nodes = True
    nt = m.node_tree
    nt.nodes.clear()
    out = nt.nodes.new('ShaderNodeOutputMaterial')
    if brilho > 0:
        em = nt.nodes.new('ShaderNodeEmission')
        em.inputs[0].default_value = (*linear(cor_da_tela if nome == 'tela' else cor), 1.0)
        em.inputs[1].default_value = brilho
        nt.links.new(em.outputs[0], out.inputs[0])
    else:
        bsdf = nt.nodes.new('ShaderNodeBsdfDiffuse')
        bsdf.inputs[0].default_value = (*linear(cor), 1.0)
        nt.links.new(bsdf.outputs[0], out.inputs[0])
    return m


def acender(cena, luzes):
    for i, l in enumerate(luzes):
        tipo, tamanho = l['tipo'], l['tamanho']
        if tipo == 'spot':
            dado = bpy.data.lights.new(f'spot_{i}', 'SPOT')
            dado.spot_size = math.radians(l['angulo'])
            dado.spot_blend = 0.7
            dado.shadow_soft_size = tamanho
        else:
            dado = bpy.data.lights.new(f'area_{i}', 'AREA')
            dado.shape = 'RECTANGLE'
            dado.size, dado.size_y = tamanho
        dado.energy = l['potencia']
        dado.color = l['cor']
        luz_ = bpy.data.objects.new(dado.name, dado)
        cena.collection.objects.link(luz_)
        luz_.location = B(l['pos'])
        d = B(l['dir']).normalized()
        if tipo == 'area':
            eixo_x = B(l['eixo'] if l['eixo'] is not None else V(1, 0, 0))
            eixo_x = (eixo_x - d * eixo_x.dot(d)).normalized()
            eixo_y = (-d).cross(eixo_x)
            luz_.matrix_world = Matrix((
                (eixo_x.x, eixo_y.x, -d.x, luz_.location.x),
                (eixo_x.y, eixo_y.y, -d.y, luz_.location.y),
                (eixo_x.z, eixo_y.z, -d.z, luz_.location.z),
                (0, 0, 0, 1)))
        else:
            luz_.rotation_euler = d.to_track_quat('-Z', 'Y').to_euler()
    # a luz da rua (os postes e as vitrines lá fora), entrando fraca pela porta: o teclado e os adesivos da porta
    dado = bpy.data.lights.new('rua', 'AREA')
    dado.shape = 'RECTANGLE'
    dado.size, dado.size_y = 3.6, 3.0
    dado.energy = LUZ_DA_RUA[1]
    dado.color = LUZ_DA_RUA[0]
    rua = bpy.data.objects.new('rua', dado)
    cena.collection.objects.link(rua)
    rua.location = B(V(0, 2.8, -2.4))
    rua.rotation_euler = B(V(0, -0.12, 1)).normalized().to_track_quat('-Z', 'Y').to_euler()
    mundo = bpy.data.worlds.new('noite')
    cena.world = mundo
    mundo.use_nodes = True
    mundo.node_tree.nodes['Background'].inputs[0].default_value = (*LUZ_DO_MUNDO[0], 1.0)
    mundo.node_tree.nodes['Background'].inputs[1].default_value = LUZ_DO_MUNDO[1]


def assar(obj):
    """A luz (direta e indireta, sem a cor) em cada canto de face, num atributo de cor"""
    cena = bpy.context.scene
    preparar_cycles(cena)
    cena.render.bake.use_pass_direct = True
    cena.render.bake.use_pass_indirect = True
    cena.render.bake.use_pass_color = False
    cena.render.bake.target = 'VERTEX_COLORS'
    me = obj.data
    atributo = me.color_attributes.new('luz', 'FLOAT_COLOR', 'CORNER')
    me.color_attributes.active_color = atributo
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    t = time.time()
    bpy.ops.object.bake(type='DIFFUSE', pass_filter={'DIRECT', 'INDIRECT'}, target='VERTEX_COLORS')
    print(f'CARRETA: luz assada em {time.time() - t:.1f} s ({len(me.loops)} cantos, {AMOSTRAS} amostras)')
    luz_ = np.empty(len(me.loops) * 4, dtype=np.float32)
    me.color_attributes['luz'].data.foreach_get('color', luz_)
    return luz_.reshape(-1, 4)[:, :3].copy()


def juntar_cantos(me, valores):
    """Tira o ruído: os cantos de um mesmo vértice com a mesma normal ficam com a média deles"""
    nl = len(me.loops)
    vert = np.empty(nl, dtype=np.int64)
    me.loops.foreach_get('vertex_index', vert)
    normais = np.empty(nl * 3, dtype=np.float32)
    me.corner_normals.foreach_get('vector', normais)
    normais = np.round(normais.reshape(-1, 3) * 4.0).astype(np.int64)
    chave = vert * 1000 + (normais[:, 0] + 4) * 81 + (normais[:, 1] + 4) * 9 + (normais[:, 2] + 4)
    _, grupo = np.unique(chave, return_inverse=True)
    soma = np.zeros((grupo.max() + 1, valores.shape[1]), dtype=np.float64)
    np.add.at(soma, grupo, valores)
    conta = np.bincount(grupo).astype(np.float64)
    return (soma / conta[:, None])[grupo].astype(np.float32)


def alisar_grades(me, valores, vezes=1):
    """Um pouco de média com os vizinhos (pelas arestas), entre cantos de faces paralelas: menos ruído nas paredes"""
    nl = len(me.loops)
    vert = np.empty(nl, dtype=np.int64)
    me.loops.foreach_get('vertex_index', vert)
    normais = np.empty(nl * 3, dtype=np.float32)
    me.corner_normals.foreach_get('vector', normais)
    normais = normais.reshape(-1, 3)
    chave_n = np.round(normais * 4.0).astype(np.int64)
    chave = vert * 1000 + (chave_n[:, 0] + 4) * 81 + (chave_n[:, 1] + 4) * 9 + (chave_n[:, 2] + 4)
    uniq, grupo = np.unique(chave, return_inverse=True)
    ng = len(uniq)
    inicio = np.empty(len(me.polygons), dtype=np.int64)
    total = np.empty(len(me.polygons), dtype=np.int64)
    me.polygons.foreach_get('loop_start', inicio)
    me.polygons.foreach_get('loop_total', total)
    proximo = np.arange(nl)
    for s, t in zip(inicio, total):
        proximo[s:s + t] = np.roll(np.arange(s, s + t), -1)
    a, b = grupo, grupo[proximo]
    valor = np.zeros((ng, valores.shape[1]), dtype=np.float64)
    np.add.at(valor, grupo, valores)
    conta = np.bincount(grupo, minlength=ng).astype(np.float64)
    valor /= conta[:, None]
    normal_do_grupo = np.zeros((ng, 3), dtype=np.float64)
    np.add.at(normal_do_grupo, grupo, normais)
    normal_do_grupo /= np.maximum(np.linalg.norm(normal_do_grupo, axis=1, keepdims=True), 1e-9)
    junto = (normal_do_grupo[a] * normal_do_grupo[b]).sum(1) > 0.995
    a, b = a[junto], b[junto]
    for _ in range(vezes):
        soma = valor.copy()
        n = np.ones(ng)
        np.add.at(soma, a, valor[b] * 0.5)
        np.add.at(n, a, 0.5)
        np.add.at(soma, b, valor[a] * 0.5)
        np.add.at(n, b, 0.5)
        valor = soma / n[:, None]
    return valor[grupo].astype(np.float32)


def curva(x):
    """O ombro: até 0,8 fica igual; daí para cima chega devagar em 1"""
    return np.where(x < 0.8, x, 0.8 + 0.2 * (1.0 - np.exp(-(x - 0.8) / 0.2)))


def reflexo_do_piso(me, piso, cor_dos_poligonos, p_canto):
    """O reflexo do piso polido (linear): de cada canto do piso, raios em volta da direção do espelho (vista de
    CAMERA_DO_REFLEXO); cada raio pega a cor (já assada, linear) do polígono que acerta"""
    co = np.empty(len(me.vertices) * 3, dtype=np.float32)
    me.vertices.foreach_get('co', co)
    co = co.reshape(-1, 3)
    arvore = BVHTree.FromPolygons([Vector(v) for v in co], [tuple(p.vertices) for p in me.polygons])
    cam = np.array(B(CAMERA_DO_REFLEXO), dtype=np.float64)
    rng = np.random.default_rng(7)
    jitter = rng.normal(0.0, ASPEREZA, (RAIOS_DO_REFLEXO, 2))
    vert = np.empty(len(me.loops), dtype=np.int64)
    me.loops.foreach_get('vertex_index', vert)
    feitos = {}
    saida = np.zeros((int(piso.sum()), 3), dtype=np.float32)
    for k, canto in enumerate(np.nonzero(piso)[0]):
        vi = int(vert[canto])
        if vi in feitos:
            saida[k] = feitos[vi]
            continue
        p = co[vi].astype(np.float64)
        v = p - cam
        v /= np.linalg.norm(v)
        r = np.array((v[0], v[1], -v[2]))          # no Blender, o piso é z = 0 (o y do jogo)
        cosv = max(-v[2], 0.0)
        fresnel = F0_DO_PISO + (1.0 - F0_DO_PISO) * (1.0 - cosv) ** 5
        e1 = np.cross(r, (0.0, 0.0, 1.0))
        if np.linalg.norm(e1) < 1e-6:
            e1 = np.array((1.0, 0.0, 0.0))
        e1 /= np.linalg.norm(e1)
        e2 = np.cross(r, e1)
        soma = np.zeros(3)
        origem = Vector(p + np.array((0.0, 0.0, 0.002)))
        for a, b in jitter:
            d = r + a * e1 + b * e2
            if d[2] <= 0.01:
                d[2] = 0.01
            d /= np.linalg.norm(d)
            _, _, indice, _ = arvore.ray_cast(origem, Vector(d), 40.0)
            if indice is not None:
                soma += cor_dos_poligonos[indice]
        cor = (soma / len(jitter)) * fresnel * FORCA_DO_REFLEXO
        feitos[vi] = cor
        saida[k] = cor
    return saida


def cores_finais(obj, ordem, luz_, cor_da_tela):
    me = obj.data
    nl = len(me.loops)
    mat = np.empty(len(me.polygons), dtype=np.int64)
    me.polygons.foreach_get('material_index', mat)
    total = np.empty(len(me.polygons), dtype=np.int64)
    me.polygons.foreach_get('loop_total', total)
    inicio = np.empty(len(me.polygons), dtype=np.int64)
    me.polygons.foreach_get('loop_start', inicio)
    mat_canto = np.repeat(mat, total)
    poli_canto = np.repeat(np.arange(len(me.polygons)), total)
    tipos = np.array([ACABAMENTOS[n][0] for n in ordem])
    cor = np.array([ACABAMENTOS[n][1] for n in ordem], dtype=np.float32)
    tipo_canto = tipos[mat_canto]
    cor_canto = cor[mat_canto]
    assada = (tipo_canto == 'cor') | (tipo_canto == 'luz') | (tipo_canto == 'metal')

    luz_ = juntar_cantos(me, luz_)
    luz_ = alisar_grades(me, luz_, 1)
    posicoes = np.empty(len(me.vertices) * 3, dtype=np.float32)
    me.vertices.foreach_get('co', posicoes)
    posicoes = posicoes.reshape(-1, 3)
    vert = np.empty(nl, dtype=np.int64)
    me.loops.foreach_get('vertex_index', vert)
    p = posicoes[vert]
    dentro = np.stack([p[:, 0], p[:, 2], -p[:, 1]], axis=1)          # cada canto no lado de dentro
    lum = luz_ @ np.array([0.2126, 0.7152, 0.0722], dtype=np.float32)
    exposicao = EXPOSICAO / max(np.percentile(lum[assada], PERCENTIL_DA_EXPOSICAO), 1e-6)
    E = luz_ * exposicao
    print(f'CARRETA: exposição {exposicao:.4f}; luz (percentis 10, 50, 90, 99):',
          np.round(np.percentile(lum[assada] * exposicao, [10, 50, 90, 99]), 3))

    normais = np.empty(nl * 3, dtype=np.float32)
    me.corner_normals.foreach_get('vector', normais)
    n = normais.reshape(-1, 3)
    cam = np.array(B(CAMERA_DE_TRAS), dtype=np.float32)
    v = cam - p
    v /= np.linalg.norm(v, axis=1, keepdims=True)
    cima = np.array(B(V(0, 1, -0.25)).normalized(), dtype=np.float32)
    h1 = v + cima
    h1 /= np.linalg.norm(h1, axis=1, keepdims=True)
    lado = np.array(B(V(0.6, 0.5, -0.6)).normalized(), dtype=np.float32)
    h2 = v + lado
    h2 /= np.linalg.norm(h2, axis=1, keepdims=True)
    para_cima = n @ np.array(B(V(0, 1, 0)), dtype=np.float32)
    ambiente = np.interp(para_cima, REFLEXO_DO_METAL[0], REFLEXO_DO_METAL[1]).astype(np.float32)
    brilho = 1.0 * np.clip((n * h1).sum(1), 0.0, 1.0) ** 40 + 0.55 * np.clip((n * h2).sum(1), 0.0, 1.0) ** 16
    borda = (1.0 - np.abs((n * v).sum(1))) ** 4
    luz_media = np.clip(E.mean(1), 0.0, 1.5)

    final = np.zeros((nl, 3), dtype=np.float32)
    lin = cor_canto ** 2.2
    m = (tipo_canto == 'cor') | (tipo_canto == 'metal')
    final[m] = lin[m] * E[m]
    m = tipo_canto == 'luz'
    final[m] = E[m]
    m = tipo_canto == 'fixo'
    final[m] = lin[m]
    # o lustro (laca, grafite, borracha): um brilho largo, branco e fraco, que acompanha a luz do lugar
    for i, nome in enumerate(ordem):
        if nome in LUSTRO:
            forca, expoente = LUSTRO[nome]
            m = mat_canto == i
            reflexo = np.clip((n[m] * h1[m]).sum(1), 0.0, 1.0) ** expoente + 0.5 * np.clip((n[m] * h2[m]).sum(1), 0.0, 1.0) ** expoente
            final[m] += (forca * np.sqrt(np.minimum(luz_media[m], 1.0)) * reflexo)[:, None]

    # o reflexo do piso: a cor de cada polígono (linear, com a média da textura) e os raios do espelho
    texturas = [ACABAMENTOS[nome][2] for nome in ordem]
    fator = np.array([MEDIA_DA_TEXTURA[t] if ACABAMENTOS[nome][0] == 'luz' else 1.0 for nome, t in zip(ordem, texturas)],
                     dtype=np.float32)
    canto_fator = fator[mat_canto][:, None] * np.ones((1, 3), dtype=np.float32)
    if 'tela' in ordem:
        canto_fator[mat_canto == ordem.index('tela')] = np.asarray(linear(cor_da_tela), dtype=np.float32)
    cor_poli = np.zeros((len(me.polygons), 3), dtype=np.float64)
    np.add.at(cor_poli, poli_canto, np.clip(final, 0.0, 4.0) * canto_fator)
    cor_poli /= total[:, None]
    piso = (tipo_canto == 'luz') & (np.array([ACABAMENTOS[nome][2] == 'chapa' for nome in ordem])[mat_canto]) & \
        (dentro[:, 1] < 0.005) & (n[:, 2] > 0.9) & (dentro[:, 2] > 0.06)
    if piso.any():
        t = time.time()
        mediana = float(np.median(E[piso] @ np.array([0.2126, 0.7152, 0.0722], dtype=np.float32)))
        refl = reflexo_do_piso(me, piso, cor_poli, p)
        final[piso] = E[piso] * min(1.0, BRILHO_DO_PISO / max(mediana, 1e-6)) + refl / MEDIA_DA_TEXTURA['chapa'] * 0.6
        print(f'CARRETA: reflexo do piso em {time.time() - t:.1f} s ({int(piso.sum())} cantos)')

    # o ombro, mantendo o matiz, e a gama da tela (só no que é assado)
    maximo = final.max(1, keepdims=True)
    final = np.where(maximo > 1e-6, final * curva(maximo) / np.maximum(maximo, 1e-6), final)
    final = np.clip(final, 0.0, 1.0) ** (1.0 / 2.2)
    for i, nome in enumerate(ordem):
        if ACABAMENTOS[nome][0] != 'metal':
            continue
        m = mat_canto == i
        acesa = np.sqrt(np.minimum(luz_media[m] * 5.0, 1.0))       # (o metal brilha mesmo com pouca luz: os spots)
        r = ambiente[m] * (0.30 + 0.70 * acesa) + brilho[m] * (0.35 + 0.65 * acesa) + 0.30 * borda[m] * acesa + REALCE_DO_METAL[nome]
        pontos, cores = RAMPA_DO_METAL[nome]
        cores = np.asarray(cores, dtype=np.float32)
        final[m] = np.stack([np.interp(np.clip(r, 0.0, 1.0), pontos, cores[:, k]) for k in range(3)], axis=1)
    m = tipo_canto == 'fixo'
    final[m] = cor_canto[m]
    rgba = np.ones((nl, 4), dtype=np.float32)
    rgba[:, :3] = np.clip(final, 0.0, 1.0)
    atributo = me.color_attributes.new('Color', 'FLOAT_COLOR', 'CORNER')
    atributo.data.foreach_set('color', rgba.ravel())
    me.color_attributes.active_color = atributo
    try:
        me.color_attributes.render_color_index = me.color_attributes.active_color_index
    except Exception:
        pass
    me.color_attributes.remove(me.color_attributes['luz'])
    return rgba


# ======================================================================
# 6) Os materiais de saída, a exportação e a conferência
# ======================================================================
def material_de_saida(nome, imagem):
    m = bpy.data.materials.new(nome)
    m.use_backface_culling = True      # as faces são de um lado só: o glTF sai com doubleSided false
    m.use_nodes = True
    nt = m.node_tree
    nt.nodes.clear()
    bsdf = nt.nodes.new('ShaderNodeBsdfPrincipled')
    bsdf.inputs['Base Color'].default_value = (1.0, 1.0, 1.0, 1.0)
    bsdf.inputs['Metallic'].default_value = 0.0
    bsdf.inputs['Roughness'].default_value = 0.8
    out = nt.nodes.new('ShaderNodeOutputMaterial')
    nt.links.new(bsdf.outputs[0], out.inputs[0])
    if imagem is not None:
        tex = nt.nodes.new('ShaderNodeTexImage')
        tex.image = imagem
        tex.interpolation = 'Linear'
        nt.links.new(tex.outputs[0], bsdf.inputs['Base Color'])
    return m


def dividir(obj):
    """Corta o objeto ao meio (no eixo mais comprido, metade das faces de cada lado) até cada pedaço ter no máximo
    MAXIMO_DE_CANTOS cantos: cada primitiva do glTF tem de ter menos de 65.536 vértices (a raylib usa índices de 16
    bits). Os pedaços mantêm os materiais, as coordenadas de textura e as cores. Devolve os pedaços."""
    me = obj.data
    if len(me.loops) <= MAXIMO_DE_CANTOS:
        return [obj]
    centros = np.empty(len(me.polygons) * 3, dtype=np.float32)
    me.polygons.foreach_get('center', centros)
    centros = centros.reshape(-1, 3)
    eixo = int(np.argmax(centros.max(0) - centros.min(0)))
    # (pela ordem ao longo do eixo, não pelo valor: muitas faces no mesmo plano, como a tela, não travam o corte)
    primeira = np.zeros(len(me.polygons), dtype=bool)
    primeira[np.argsort(centros[:, eixo], kind='stable')[:len(me.polygons) // 2]] = True
    ativa = me.color_attributes.active_color.name
    metades = []
    for lado in (0, 1):
        nova = me.copy()
        bm = bmesh.new()
        bm.from_mesh(nova)
        bm.faces.ensure_lookup_table()
        fora = [f for f in bm.faces if primeira[f.index] == (lado == 1)]
        bmesh.ops.delete(bm, geom=fora, context='FACES')
        bm.to_mesh(nova)
        bm.free()
        nova.color_attributes.active_color = nova.color_attributes[ativa]
        try:
            nova.color_attributes.render_color_index = nova.color_attributes.active_color_index
        except Exception:
            pass
        nova.name = obj.name + ('_a' if lado == 0 else '_b')
        o = bpy.data.objects.new(nova.name, nova)
        bpy.context.scene.collection.objects.link(o)
        metades.append(o)
    bpy.data.objects.remove(obj)
    return [p for m in metades for p in dividir(m)]


def ler_gltf(caminho):
    with open(caminho, 'rb') as f:
        f.read(12)
        n, _ = struct.unpack('<II', f.read(8))
        gltf = json.loads(f.read(n))
        n2, _ = struct.unpack('<II', f.read(8))
        binario = f.read(n2)
    return gltf, binario


def ler_acessor(gltf, binario, indice):
    acc = gltf['accessors'][indice]
    bv = gltf['bufferViews'][acc['bufferView']]
    dt, esc = {5121: (np.uint8, 255.0), 5123: (np.uint16, 65535.0), 5126: (np.float32, 1.0)}[acc['componentType']]
    comps = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3, 'VEC4': 4}[acc['type']]
    ini = bv.get('byteOffset', 0) + acc.get('byteOffset', 0)
    return np.frombuffer(binario, dt, count=acc['count'] * comps, offset=ini).reshape(-1, comps) / esc


def conferir(caminho, nome='CARRETA', sem_textura=False):
    gltf, binario = ler_gltf(caminho)
    tri = 0
    sem_cor = []
    vertices = []
    alfas = []
    for m in gltf['meshes']:
        for p in m['primitives']:
            acc = gltf['accessors'][p['indices']] if 'indices' in p else gltf['accessors'][p['attributes']['POSITION']]
            tri += acc['count'] // 3
            nv = gltf['accessors'][p['attributes']['POSITION']]['count']
            vertices.append((m.get('name'), gltf['materials'][p['material']]['name'] if 'material' in p else None, nv, acc['count'] // 3))
            if nv > 65535:
                raise SystemExit(f"{nome}: a primitiva de {m.get('name')} tem {nv} vértices: a raylib guarda os índices em 16 bits "
                                 f"(no máximo 65.535); baixe MAXIMO_DE_CANTOS")
            if 'COLOR_0' not in p['attributes']:
                sem_cor.append(m.get('name'))
            else:
                c = gltf['accessors'][p['attributes']['COLOR_0']]
                if c['type'] != 'VEC4':
                    raise SystemExit(f"{nome}: COLOR_0 de {m.get('name')} é {c['type']} (a raylib 5 só lê VEC4)")
                alfas.append(ler_acessor(gltf, binario, p['attributes']['COLOR_0'])[:, 3])
    if sem_cor:
        raise SystemExit(f'{nome}: primitivas sem COLOR_0: {sem_cor}')
    imagens = [(im.get('name'), im.get('mimeType')) for im in gltf.get('images', [])]
    if any(t != 'image/png' for _, t in imagens):
        raise SystemExit(f'{nome}: há imagem que não é PNG: {imagens}')
    if sem_textura and imagens:
        raise SystemExit(f'{nome}: a camada das luzes não pode ter textura: {imagens}')
    for mt in gltf['materials']:
        f = mt.get('pbrMetallicRoughness', {}).get('baseColorFactor', [1, 1, 1, 1])
        if any(abs(v - 1.0) > 1e-3 for v in f):
            raise SystemExit(f"{nome}: o material {mt.get('name')} não tem baseColorFactor branco: {f}")
    duplas = [mt.get('name') for mt in gltf['materials'] if mt.get('doubleSided')]
    if duplas:
        raise SystemExit(f'{nome}: materiais com doubleSided: {duplas}')
    prims = sum(len(m['primitives']) for m in gltf['meshes'])
    tamanho = os.path.getsize(caminho)
    alfa = np.concatenate(alfas)
    for nm, material, nv, nt in vertices:
        print(f"{nome}:   {nm} / {material}: {nv} vértices, {nt} triângulos")
    print(f"{nome}: conferido: {prims} primitivas, todas com COLOR_0 (VEC4) e no máximo {max(v[2] for v in vertices)} vértices; "
          f"{tri} triângulos; {len(gltf['materials'])} materiais (doubleSided false); {len(imagens)} texturas PNG {imagens}; "
          f"alfa de {alfa.min():.2f} a {alfa.max():.2f}; {tamanho / 1e6:.2f} MB")
    return dict(triangulos=tri, materiais=len(gltf['materials']), texturas=len(imagens), bytes=tamanho, primitivas=prims,
                vertices=sum(v[2] for v in vertices), maior=max(v[2] for v in vertices))


def pontos_dos_triangulos(me, n=4):
    """Pontos espalhados em cada triângulo da malha (os cantos, as bordas e o miolo), nos eixos do lado de dentro"""
    me.calc_loop_triangles()
    co = np.empty(len(me.vertices) * 3, dtype=np.float32)
    me.vertices.foreach_get('co', co)
    co = co.reshape(-1, 3)
    tri = np.empty(len(me.loop_triangles) * 3, dtype=np.int64)
    me.loop_triangles.foreach_get('vertices', tri)
    tri = tri.reshape(-1, 3)
    pesos = np.array([(i / n, j / n, (n - i - j) / n) for i in range(n + 1) for j in range(n + 1 - i)], dtype=np.float32)
    p = np.einsum('pk,tkc->tpc', pesos, co[tri]).reshape(-1, 3)
    return np.stack([p[:, 0], p[:, 2], -p[:, 1]], axis=1)          # Blender -> lado de dentro


def conferir_a_casca(me, caixas, mack, nome='CARRETA'):
    """Confere, antes de assar, que o lado de dentro não fura nada do Mack: nenhum ponto dentro de uma roda, nada além
    de CASCA_NAS_RODAS abaixo de 0,95 m nas caixas de roda, nada abaixo da saia de trás, e nada que passe da carroceria
    (um raio do meio do baú até cada ponto de fora do vão útil não pode encontrar a carroceria no caminho)."""
    p = pontos_dos_triangulos(me)
    erros = []
    for r in mack['cilindros']:
        dentro = (p[:, 0] > r['x0'] - 0.003) & (p[:, 0] < r['x1'] + 0.003) & \
                 ((p[:, 1] - r['y']) ** 2 + (p[:, 2] - r['z']) ** 2 < (r['raio'] + 0.003) ** 2)
        if dentro.any():
            erros.append(f"{int(dentro.sum())} pontos dentro da roda em z {r['z']:.2f} (x {r['x0']:.2f} a {r['x1']:.2f}), "
                         f"por exemplo {np.round(p[dentro][0], 3)}")
    for cx in caixas:
        fora = (p[:, 0] * cx['lado'] > CASCA_NAS_RODAS + 1e-4) & (p[:, 1] < 0.95) & (p[:, 2] > cx['z0']) & (p[:, 2] < cx['z1'])
        if fora.any():
            erros.append(f"{int(fora.sum())} pontos além de |x| {CASCA_NAS_RODAS} na caixa de roda do lado {cx['lado']:+d}, "
                         f"por exemplo {np.round(p[fora][0], 3)}")
    baixo = p[:, 1] < BAIXO_DA_CASCA - 1e-4
    if baixo.any():
        erros.append(f"{int(baixo.sum())} pontos abaixo de y {BAIXO_DA_CASCA}, por exemplo {np.round(p[baixo][0], 3)}")
    arvore = BVHTree.FromPolygons(mack['verts'], mack['polis'])
    nas_rodas = np.zeros(len(p), dtype=bool)
    for cx in caixas:
        nas_rodas |= (p[:, 0] * cx['lado'] > 0) & (p[:, 1] < 0.95) & (p[:, 2] > cx['z0']) & (p[:, 2] < cx['z1'])
    longe = ((np.abs(p[:, 0]) > MEIA_LARGURA + 0.01) | (p[:, 1] > ALTURA + 0.01)) & ~nas_rodas
    furos = []
    for q in np.unique(np.round(p[longe], 3), axis=0):
        de = B(0.0, float(min(max(q[1], 0.3), 5.5)), float(q[2]))
        para = B(*map(float, q))
        d = para - de
        if d.length > 1e-4 and arvore.ray_cast(de, d.normalized(), d.length - 0.001)[0] is not None:
            furos.append(tuple(q))
    if furos:
        erros.append(f"{len(furos)} pontos passam da carroceria, por exemplo {furos[:6]}")
    print(f"{nome}: casca conferida: {len(p)} pontos, {int(longe.sum())} fora do vão útil; "
          + ('nada fura as rodas nem a carroceria' if not erros else 'PROBLEMAS'))
    if erros:
        for e in erros:
            print(f'{nome}:   ', e)
        raise SystemExit(f'{nome}: o lado de dentro fura o Mack (veja acima)')


def exportar(objetos, caminho, normais=True, texturas=True):
    bpy.ops.object.select_all(action='DESELECT')
    for o in objetos:
        o.select_set(True)
    bpy.context.view_layer.objects.active = objetos[0]
    os.makedirs(os.path.dirname(caminho), exist_ok=True)
    bpy.ops.export_scene.gltf(filepath=caminho, export_format='GLB', export_yup=True, use_selection=True,
                              export_vertex_color='ACTIVE', export_all_vertex_colors=False,
                              export_active_vertex_color_when_no_material=True, export_normals=normais,
                              export_texcoords=texturas, export_image_format='AUTO', export_materials='EXPORT',
                              export_animations=False, export_skins=False, export_morph=False)


def a_camada_das_luzes(c, cena):
    """A malha da camada das luzes, cortada em pedaços, com o material branco e as cores nos cantos"""
    bm = c.luzes.bm
    me = bpy.data.meshes.new('carreta_luzes')
    bm.to_mesh(me)
    bm.free()
    # a cor dos cantos vira o atributo de cor ativo (FLOAT_COLOR, CORNER)
    nome = me.color_attributes[0].name if len(me.color_attributes) else None
    if nome is None:
        raise SystemExit('CARRETA: a camada das luzes ficou sem cor')
    me.color_attributes.active_color = me.color_attributes[nome]
    me.color_attributes.render_color_index = me.color_attributes.active_color_index
    m = bpy.data.materials.new('carreta_luzes')
    m.use_backface_culling = True
    m.use_nodes = True
    bsdf = next(n for n in m.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    bsdf.inputs['Base Color'].default_value = (1, 1, 1, 1)
    me.materials.append(m)
    obj = bpy.data.objects.new('carreta_luzes', me)
    cena.collection.objects.link(obj)
    print(f'CARRETA: camada das luzes: {len(me.polygons)} faces, {sum(len(p.vertices) - 2 for p in me.polygons)} triângulos')
    return obj


def construir():
    origem, dobradica, porta, caixas, mack = medir_o_mack()
    atlas = ler_png(ATLAS_PRONTO)[..., :3] if ATLAS_PRONTO else pintar_o_atlas()
    chapa = pintar_a_chapa()
    perfurada = pintar_a_perfurada()
    _fontes.clear()
    if PREVIA:
        os.makedirs(PREVIA, exist_ok=True)

    bpy.ops.wm.read_factory_settings(use_empty=True)
    cena = bpy.context.scene
    if PREVIA:
        gravar_png(os.path.join(PREVIA, 'atlas.png'), atlas)
        gravar_png(os.path.join(PREVIA, 'chapa.png'), chapa)
        gravar_png(os.path.join(PREVIA, 'perfurada.png'), perfurada)
    imagens = {'atlas': imagem_do_blender('carreta_atlas', atlas), 'chapa': imagem_do_blender('carreta_chapa', chapa),
               'perfurada': imagem_do_blender('carreta_perfurada', perfurada)}
    x, y, l, a = CASA_DA_TELA
    imagem_da_tela = atlas[y + 2:y + a - 2, x + 2:x + l - 2, :3]
    cor_da_tela = tuple(float(v) for v in imagem_da_tela.reshape(-1, 3).mean(0))

    c = Construtor()
    luzes = montar(c, caixas, imagem_da_tela)
    bmesh.ops.remove_doubles(c.bm, verts=c.bm.verts, dist=1e-5)
    bmesh.ops.delete(c.bm, geom=[v for v in c.bm.verts if not v.link_faces], context='VERTS')
    me = bpy.data.meshes.new('carreta')
    c.bm.to_mesh(me)
    c.bm.free()
    conferir_a_casca(me, caixas, mack)
    obj = bpy.data.objects.new('carreta', me)
    cena.collection.objects.link(obj)
    for nome in c.ordem:
        me.materials.append(material_do_bake(nome, cor_da_tela))
    tri_por_acab = {}
    for p in me.polygons:
        nome = c.ordem[p.material_index]
        tri_por_acab[nome] = tri_por_acab.get(nome, 0) + len(p.vertices) - 2
    print(f'CARRETA: {len(me.polygons)} faces, {sum(tri_por_acab.values())} triângulos:',
          ', '.join(f'{n} {q}' for n, q in sorted(tri_por_acab.items(), key=lambda x: -x[1])))

    acender(cena, luzes)
    luz_ = assar(obj)
    cores_finais(obj, c.ordem, luz_, cor_da_tela)

    # os materiais de saída: um por textura
    saida = {textura: material_de_saida(nome, imagens.get(textura)) for textura, nome in MATERIAL_DE_SAIDA.items()}
    mat = np.empty(len(me.polygons), dtype=np.int64)
    me.polygons.foreach_get('material_index', mat)
    me.materials.clear()
    usados = []
    for textura in MATERIAL_DE_SAIDA:
        if any(ACABAMENTOS[n][2] == textura for n in c.ordem):
            usados.append(textura)
            me.materials.append(saida[textura])
    novo = [usados.index(ACABAMENTOS[n][2]) for n in c.ordem]
    me.polygons.foreach_set('material_index', np.array(novo, dtype=np.int64)[mat])
    for o in list(cena.objects):
        if o is not obj:
            bpy.data.objects.remove(o)
    pedacos = dividir(obj)
    print(f"CARRETA: {len(pedacos)} pedaços ({', '.join(str(len(o.data.loops)) for o in pedacos)} cantos)")
    exportar(pedacos, SAIDA)
    print('CARRETA: gravado', SAIDA)
    info = conferir(SAIDA)

    # a camada das luzes
    for o in list(cena.objects):
        bpy.data.objects.remove(o)
    camada = a_camada_das_luzes(c, cena)
    conferir_a_casca(camada.data, caixas, mack, nome='LUZES')
    pedacos = dividir(camada)
    exportar(pedacos, SAIDA_DAS_LUZES, normais=False, texturas=False)
    print('CARRETA: gravado', SAIDA_DAS_LUZES)
    info_luzes = conferir(SAIDA_DAS_LUZES, nome='LUZES', sem_textura=True)
    return origem, dobradica, caixas, info, info_luzes


# ======================================================================
# 7) As prévias: os .glb gravados, como no jogo, com o Mack, a rampa e o brilho em volta
# ======================================================================
def borrar_gauss(img, sigma):
    """Um borrão quase gaussiano (três caixas) com mais ou menos o sigma pedido"""
    return borrar(img, max(1, int(round(sigma))), 3)


def brilho_em_volta(cena_rgb, luzes_rgb):
    """Como o posprocesso.c: as luzes reduzidas a 1/4, borradas (duas vezes em cada direção) e somadas à cena"""
    a, l = luzes_rgb.shape[:2]
    a4, l4 = a // 4, l // 4
    pequena = luzes_rgb[:a4 * 4, :l4 * 4].reshape(a4, 4, l4, 4, 3).mean((1, 3))
    sigma = 3.6 * l / 1280.0
    halo = borrar_gauss(pequena, sigma)
    halo = np.kron(halo, np.ones((4, 4, 1), dtype=np.float32))
    saida = cena_rgb.copy()
    saida[:a4 * 4, :l4 * 4] += halo * 0.75
    return np.clip(saida, 0.0, 1.0)


def previa(origem, dobradica, porta_aberta=1.0):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    cena = bpy.context.scene
    interior = [o for o in importar(SAIDA) if o.type == 'MESH']
    camada = [o for o in importar(SAIDA_DAS_LUZES) if o.type == 'MESH']
    feitos = set()
    apagar = []                         # (os nós de emissão, para a passada só das luzes)

    def emissao(m, imagem, nome_da_cor, multiplica=1.0, gama=2.2):
        nt = m.node_tree
        nt.nodes.clear()
        out = nt.nodes.new('ShaderNodeOutputMaterial')
        em = nt.nodes.new('ShaderNodeEmission')
        cor = nt.nodes.new('ShaderNodeVertexColor')
        cor.layer_name = nome_da_cor
        mult = nt.nodes.new('ShaderNodeMix')
        mult.data_type = 'RGBA'
        mult.blend_type = 'MULTIPLY'
        mult.inputs[0].default_value = 1.0
        mult.inputs[7].default_value = (multiplica, multiplica, multiplica, 1.0)
        nt.links.new(cor.outputs[0], mult.inputs[6])
        g = nt.nodes.new('ShaderNodeGamma')
        g.inputs[1].default_value = gama
        nt.links.new(mult.outputs[2], g.inputs[0])
        if imagem is not None:
            t2 = nt.nodes.new('ShaderNodeTexImage')
            t2.image = imagem
            t2.interpolation = 'Linear'
            mix = nt.nodes.new('ShaderNodeMix')
            mix.data_type = 'RGBA'
            mix.blend_type = 'MULTIPLY'
            mix.inputs[0].default_value = 1.0
            nt.links.new(t2.outputs[0], mix.inputs[6])
            nt.links.new(g.outputs[0], mix.inputs[7])
            nt.links.new(mix.outputs[2], em.inputs[0])
        else:
            nt.links.new(g.outputs[0], em.inputs[0])
        nt.links.new(em.outputs[0], out.inputs[0])
        m.use_backface_culling = True
        return em

    for o in interior:
        for slot in o.material_slots:
            m = slot.material
            if m.name in feitos:
                continue
            feitos.add(m.name)
            tex = next((n for n in m.node_tree.nodes if n.type == 'TEX_IMAGE'), None)
            imagem = tex.image if tex is not None else None
            apagar.append(emissao(m, imagem, o.data.color_attributes[0].name))
    for o in camada:
        for slot in o.material_slots:
            m = slot.material
            if m.name in feitos:
                continue
            feitos.add(m.name)
            # (como o shader das luzes: (cor + 0,18 x o maior canal) x brilho; aqui, uma aproximação: x 1,15)
            emissao(m, None, o.data.color_attributes[0].name, multiplica=1.15)
    desloca = Matrix.Translation(-B(origem))
    mack = importar(os.path.join(MODELOS, 'mack.glb'))
    porta = importar(os.path.join(MODELOS, 'mack_porta.glb'))
    rodas = importar(os.path.join(MODELOS, 'mack_rodas.glb'))
    for o in mack + porta + rodas:
        if o.parent is None:
            o.matrix_world = desloca @ o.matrix_world
    bpy.context.view_layer.update()
    altura_da_porta = max((o.matrix_world @ v.co).z for o in porta if o.type == 'MESH' for v in o.data.vertices) - \
        min((o.matrix_world @ v.co).z for o in porta if o.type == 'MESH' for v in o.data.vertices)
    giro = porta_aberta * (math.pi / 2 + math.asin(min(dobradica.y / altura_da_porta, 0.9)))
    dob = B(dobradica - origem)
    for o in porta:
        if o.parent is None:
            o.matrix_world = Matrix.Translation(dob) @ Matrix.Rotation(-giro, 4, 'X') @ Matrix.Translation(-dob) @ o.matrix_world
    # a rampa (o lado de dentro da porta, que o jogo desenha de chapa cinza)
    me = bpy.data.meshes.new('rampa')
    comp = altura_da_porta
    fim = dob + Vector((0, math.sin(giro) * comp, math.cos(giro) * comp))
    me.from_pydata([dob + Vector((-1.92, 0, 0.01)), dob + Vector((1.92, 0, 0.01)), fim + Vector((1.92, 0, 0.01)), fim + Vector((-1.92, 0, 0.01))],
                   [], [(0, 1, 2, 3)])
    rampa = bpy.data.objects.new('rampa', me)
    cena.collection.objects.link(rampa)
    mr = bpy.data.materials.new('rampa')
    mr.use_nodes = True
    mr.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value = (0.12, 0.12, 0.13, 1.0)
    mr.node_tree.nodes['Principled BSDF'].inputs['Roughness'].default_value = 0.35
    me.materials.append(mr)
    for o in mack + porta + rodas:
        if o.type == 'MESH':
            for s in o.material_slots:
                if s.material:
                    s.material.use_backface_culling = False
    chao = bpy.data.meshes.new('chao')
    y0 = -origem.y
    chao.from_pydata([(-80, -80, y0), (80, -80, y0), (80, 80, y0), (-80, 80, y0)], [], [(0, 1, 2, 3)])
    oc = bpy.data.objects.new('chao', chao)
    cena.collection.objects.link(oc)
    mc = bpy.data.materials.new('asfalto')
    mc.use_nodes = True
    mc.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value = (0.10, 0.09, 0.09, 1.0)
    chao.materials.append(mc)
    # a noite: o céu escuro, a lua fraca e o que sai da porta aberta (para a carroceria em volta dela)
    mundo = bpy.data.worlds.new('ceu')
    cena.world = mundo
    mundo.use_nodes = True
    mundo.node_tree.nodes['Background'].inputs[0].default_value = (0.010, 0.014, 0.030, 1.0)
    mundo.node_tree.nodes['Background'].inputs[1].default_value = 1.0
    lua = bpy.data.objects.new('lua', bpy.data.lights.new('lua', 'SUN'))
    cena.collection.objects.link(lua)
    lua.data.energy = 0.25
    lua.data.color = (0.6, 0.7, 1.0)
    lua.rotation_euler = (math.radians(55), 0, math.radians(200))
    saida_de_luz = bpy.data.objects.new('porta_acesa', bpy.data.lights.new('porta_acesa', 'AREA'))
    cena.collection.objects.link(saida_de_luz)
    saida_de_luz.data.shape = 'RECTANGLE'
    saida_de_luz.data.size, saida_de_luz.data.size_y = 3.6, 5.4
    saida_de_luz.data.energy = 70.0
    saida_de_luz.data.color = (1.0, 0.86, 0.40)
    saida_de_luz.location = B(V(0, 2.8, -0.15))
    saida_de_luz.rotation_euler = B(V(0, 0, -1)).to_track_quat('-Z', 'Y').to_euler()
    preparar_eevee(cena, 1280, 720, transparente=False)
    cam = bpy.data.objects.new('camera', bpy.data.cameras.new('camera'))
    cena.collection.objects.link(cam)
    cena.camera = cam
    cam.data.clip_start = 0.05
    cam.data.clip_end = 500

    def foto(nome, pos, alvo, angulo, tamanho=(1280, 720), horizontal=False, so_luzes=False):
        cena.render.resolution_x, cena.render.resolution_y = tamanho
        cam.data.sensor_fit = 'HORIZONTAL' if horizontal else 'VERTICAL'
        cam.data.angle = math.radians(angulo)
        cam.location = B(pos)
        cam.rotation_euler = (B(alvo) - B(pos)).to_track_quat('-Z', 'Y').to_euler()
        caminho = os.path.join(TEMP, nome + '_cena.png')
        cena.render.filepath = caminho
        bpy.ops.render.render(write_still=True)
        a = ler_png(caminho)[..., :3]
        # a passada só das luzes: o resto preto (tapa as luzes que ficam atrás dele)
        guardados = []
        for em in apagar:
            guardados.append(em.inputs[1].default_value)
            em.inputs[1].default_value = 0.0
        visiveis = [o for o in cena.objects if o.type == 'MESH' and o not in interior and o not in camada]
        for o in visiveis:
            o.is_holdout = True               # (pretos: tapam as luzes que ficam atrás deles, como a profundidade no jogo)
        mundo.node_tree.nodes['Background'].inputs[1].default_value = 0.0
        caminho = os.path.join(TEMP, nome + '_luzes.png')
        cena.render.filepath = caminho
        bpy.ops.render.render(write_still=True)
        b = ler_png(caminho)[..., :3]
        for em, valor in zip(apagar, guardados):
            em.inputs[1].default_value = valor
        for o in visiveis:
            o.is_holdout = False
        mundo.node_tree.nodes['Background'].inputs[1].default_value = 1.0
        final = brilho_em_volta(a, b)
        if so_luzes:
            gravar_png(os.path.join(PREVIA, nome + '_so_luzes.png'), b)
        gravar_png(os.path.join(PREVIA, nome + '.png'), final)
        print('CARRETA: prévia', os.path.join(PREVIA, nome + '.png'))
        return final

    # as duas vistas do filme (no tamanho das imagens de referência)
    rampa_img = foto('vista_da_rampa', V(0.0, 1.15, -4.6), V(0.0, 1.95, 12.0), 74, (2000, 850), horizontal=True, so_luzes=True)
    vistas = [
        ('vista_do_jogo_saindo', CAMERA_DE_TRAS, ALVO_DE_TRAS, 55),
        ('vista_do_jogo_entrando', CAMERA_DO_LADO, ALVO_DO_LADO, 55),
        ('vista_do_fundo', V(0.0, 2.3, 6.2), V(0.0, 2.5, 12.0), 62),
        ('vista_da_parede', V(-0.9, 2.2, 3.0), V(1.92, 2.0, 6.5), 66),
        ('vista_da_prateleira', V(-0.4, 3.0, 2.6), V(1.80, 3.35, 5.8), 55),
        ('vista_do_teto', V(0.0, 2.2, 3.0), V(0.0, 5.76, 7.5), 70),
        ('vista_da_porta_de_dentro', V(0.0, 2.4, 10.5), V(0.0, 2.8, 0.0), 70),
        ('vista_do_arco', V(-7.0, 0.2, 3.3), V(2.0, 0.2, 3.3), 40),
        ('vista_da_quina_de_cima', V(-3.2, 6.6, -2.2), V(-2.0, 5.8, 0.2), 45),
    ]
    for nome, pos, alvo, ang in vistas:
        foto(nome, pos, alvo, ang)
    # com o carro do jogador estacionado lá dentro (traseira a 1,5 m da porta, bico para o fundo)
    carro = importar(os.path.join(CARROS, 'mcqueen.glb'))
    mn, mx = limites(carro)
    escala = 4.4 / (mx.y - mn.y)
    raiz = bpy.data.objects.new('raiz', None)
    cena.collection.objects.link(raiz)
    for o in carro:
        if o.parent is None:
            o.parent = raiz
    raiz.matrix_world = Matrix.Translation(B(V(0, 0, CARRO_DE + 2.2))) @ Matrix.Scale(escala, 4) @ \
        Matrix.Translation(-Vector(((mn.x + mx.x) / 2, (mn.y + mx.y) / 2, mn.z)))
    luz_do_carro = bpy.data.objects.new('luz_do_carro', bpy.data.lights.new('luz_do_carro', 'AREA'))
    cena.collection.objects.link(luz_do_carro)
    luz_do_carro.data.size = 3.0
    luz_do_carro.data.energy = 400.0
    luz_do_carro.data.color = (1.0, 0.85, 0.5)
    luz_do_carro.location = B(V(0, 4.5, 3.7))
    luz_do_carro.rotation_euler = B(V(0, -1, 0)).to_track_quat('-Z', 'Y').to_euler()
    dentro_img = foto('vista_de_dentro_para_cima', V(-0.85, 2.0, 0.8), V(0.55, 5.3, 8.5), 100, (2000, 844), horizontal=True)
    foto('vista_do_jogo_saindo_com_carro', CAMERA_DE_TRAS, ALVO_DE_TRAS, 55)
    # lado a lado com o filme
    if REFERENCIAS:
        for nome, img, ref in (('comparar_rampa', rampa_img, '59.png'), ('comparar_de_dentro', dentro_img, '60.png')):
            caminho = os.path.join(REFERENCIAS, ref)
            if os.path.exists(caminho):
                r = ler_png(caminho)[..., :3]
                if r.shape[1] == img.shape[1]:
                    gravar_png(os.path.join(PREVIA, nome + '.png'), np.concatenate([r, np.zeros((8, r.shape[1], 3), np.float32), img], 0))


inicio = time.time()
origem, dobradica, caixas, info, info_luzes = construir()
if PREVIA:
    previa(origem, dobradica)
shutil.rmtree(TEMP, ignore_errors=True)
print(f"CARRETA: pronto em {time.time() - inicio:.0f} s: interior com {info['triangulos']} triângulos ({info['primitivas']} primitivas, "
      f"maior com {info['maior']} vértices, {info['bytes'] / 1e6:.2f} MB); luzes com {info_luzes['triangulos']} triângulos "
      f"({info_luzes['primitivas']} primitivas, maior com {info_luzes['maior']} vértices, {info_luzes['bytes'] / 1e6:.2f} MB)")
