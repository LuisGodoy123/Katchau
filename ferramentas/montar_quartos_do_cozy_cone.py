"""
montar_quartos_do_cozy_cone.py
------------------------------------------------------------------
Monta o lado de dentro dos oito cones-quarto do Cozy Cone Motel, o motel da
Sally (do filme Carros, 2006), no lugar de cada cone no mapa do mundo aberto
(recursos/modelos/radiator_springs.glb), já nas coordenadas do mundo do jogo
(x, y para cima, z):

    Blender -b --factory-startup -P ferramentas/montar_quartos_do_cozy_cone.py -- \\
        recursos/modelos/radiator_springs.glb recursos/modelos/cozy_cone_quartos \\
        [--previas pasta] [--amostras 512] [--quadros atlas.png] [--noite]

Grava, com o nome dado no segundo argumento:
  cozy_cone_quartos.glb          os oito quartos, com a luz assada nas cores dos
                                 vértices (COLOR_0): o jogo os desenha com o shader
                                 padrão da raylib (textura × cor do vértice), sem luz;
  cozy_cone_quartos_colisao.glb  a colisão: o piso e, em faces em pé, as paredes (até
                                 o teto, para a câmera) e os móveis (sem tampos, que
                                 viram chão para o carro); a porta de garagem aberta;
                                 um disco no teto (6 m acima do piso: a câmera sabe
                                 que está coberta) e, no cone 4, a beira da base que
                                 a arena tinha cortado;
  cozy_cone_quartos.json         as portas, os lugares de estacionar (um por quarto, o
                                 "mcqueen" no cone do McQueen) e as aberturas que se
                                 tiram da casca dos cones no mapa (o painel pintado
                                 de cada porta e os vidros das janelas redondas).

O mapa (primeiro argumento) serve para remendar o cone 4 e para as prévias. No
mapa montado, a etapa da arena tirou a metade de trás da casca e da base do cone
4 (abaixo de y 7,56): a ferramenta acha as faces que faltam comparando com o cone
1, que está inteiro, e as põe de volta (a "pele" do cone 4, com a cor da textura
da casca nas cores dos vértices). Sem o mapa, o cone 4 fica sem a pele. Com
--previas, a ferramenta grava na pasta as vistas de dentro e de fora (a casca dos
cones em volta, sem as portas e os vidros, e a praça vista pela porta), mostrando
o que o jogo vai mostrar: as cores assadas × as texturas, com o Relâmpago McQueen
estacionado no cone dele. --quadros guarda (ou, se o arquivo já existe,
reaproveita) o atlas dos quadros, que é o passo mais demorado. --amostras é o
número de amostras do Cycles por ponto na luz assada.

O quarto, como no filme: o próprio cone por dentro (a parede acompanha a
inclinação da casca, uns 28 cm para dentro dela), com a parede da porta reta.
  1. Desenha as texturas (em PNG, porque a raylib do jogo não lê JPEG): o papel
     de parede cor de pêssego com conezinhos claros bem apagados, o carpete
     bege-acinzentado e a fibra de coco dos dois tapetes compridos.
  2. Pinta os quadros: paisagens do deserto em cenas pequenas renderizadas no
     Blender (as mesas de rocha no pôr do sol, a Rota 66, a serra das rochas em
     forma de rabo de peixe, o arco de rocha vermelha, o Cozy Cone à noite, a cachoeira,
     o cacto e a lua, e a Sally na estrada, com o modelo do jogo pintado "de
     quadro"), com cara de tinta em tela; mais o pôster do Relâmpago McQueen 95
     (só no cone dele) e o mostrador do relógio. Tudo num atlas de 2048 x 1024.
  3. Monta cada quarto: a parede de cone (papel de parede em cima e, embaixo,
     a barra mais escura com o friso de dentes de serra, a cimalha e o rodapé),
     a parede da porta com a moldura clara e a caixa da porta de garagem
     enrolada no alto (e o forro creme dos batentes e da verga da casca, até a
     boca do nicho), o teto com a sanca, o carpete e os dois tapetes de fibra
     de coco onde o carro para; as janelas redondas (o vão chanfrado, o
     caixilho em cruz, o bandô e as cortinas claras amarradas); o
     ar-condicionado de janela creme com a grade redonda de lâminas diagonais e
     os botões (ele tapa o da casca, que atravessa a parede), com o relógio em
     forma de cone em cima; a luminária de pé com cúpulas de cone; a mesinha
     anos 50 de pés abertos com o vaso de rosas vermelhas e o conezinho; os
     quadros em molduras de cone (um "A" estreito, com a tábua do pé larga); a luminária
     de cone pequena e o interruptor de chão perto da porta. No cone do McQueen,
     o pôster dele e um troféu pequeno da Copa Pistão na mesinha. Os cones de 5
     a 8 são espelhados no mapa: o quarto deles também (o ar-condicionado fica
     do outro lado), mas cada objeto é montado sem espelhar (as letras e os
     quadros saem certos).
  4. Assa a luz no Cycles (difusa direta e indireta, sem a cor) nas cores dos
     vértices, com as mesmas luzes em cada quarto: o sol da tarde entrando pela
     janela do ar-condicionado, o dia claro pela porta e pela outra janela, e o
     abajur (uma luz dentro de cada cúpula, que faz a poça quente na parede e no
     chão). As superfícies grandes são cortadas numa grade de uns 0,3 m em
     níveis globais (sem junções em T). Os pontos das paredes e do piso que
     ficam escondidos atrás dos quadros, do ar-condicionado, das molduras e dos
     tapetes ganham a luz dos vizinhos à vista (senão a sombra deles vaza em
     volta), e a luz é alisada entre os vizinhos da mesma superfície (a parede de
     cone inteira, não coluna por coluna). A luz cai um pouco no alto da parede,
     é normalizada (a mediana das paredes fica em 0,5 na tela) e passada para a
     escala de tela; a cor sem textura vai junto. O troféu e o pé do abajur
     ganham um brilho falso (assado). O forro da porta e a pele do cone 4 não
     são assados: o forro leva a cor que a moldura clara da porta ficou, e a
     pele, a da textura da casca.
  5. Exporta: poucos materiais (um sem textura e um por textura), cada malha
     com menos de 65 mil vértices (a raylib usa índices de 16 bits), e confere
     no .glb que todas as primitivas têm COLOR_0.

Com --noite, a ferramenta faz a luz de NOITE (o jogo mistura a de dia e a de
noite vértice a vértice, conforme a hora) e grava só:
  cozy_cone_quartos_noite.glb    o .glb de dia (que já tem de existir) copiado byte
                                 a byte, com outro COLOR_0: as mesmas malhas,
                                 primitivas, vértices, posições, coordenadas de
                                 textura, índices e texturas. A montagem é a mesma; a
                                 luz assada é sem o sol da tarde, com um luar fraco
                                 azul-arroxeado pela porta e pelas janelas e as luzes
                                 de dentro (o abajur de pé, a cúpula, a luminária de
                                 cone) mais presentes, com mais contraste (a mediana
                                 das paredes em 0,30). A pele do cone 4 leva a luz
                                 que o shader do mapa dá à meia-noite (os
                                 multiplicadores da chave das 0h de src/hora.c e a
                                 lua), sem luz de dentro; o forro da porta, na boca do
                                 nicho, a casca em volta à luz da porta. O assado de
                                 noite é exportado à parte e as cores dele passam para
                                 os vértices do .glb de dia (casando os triângulos), e
                                 a ferramenta confere que só o COLOR_0 mudou;
  cozy_cone_quartos_noite.json   as luzes pontuais que os quartos jogam para fora de
                                 noite, uma por cone, na porta (eixos do jogo; lugar,
                                 cor, raio e força).
O .glb de dia, a colisão e o .json ficam como estão. Sem --quadros, o atlas dos
quadros vem do .glb de dia. Com --previas, as vistas saem com o dia e a noite lado
a lado (os dois .glb como o jogo os carrega; de noite, a casca do mapa com a luz
da meia-noite do jogo e as luzes do _noite.json).

Eixos: no Blender, x = x do jogo, y = -z do jogo e z = y do jogo; o .glb sai com
+Y para cima, ou seja, nas coordenadas do jogo. Dentro de cada cone, as medidas
vão em (u, v): u do eixo do cone para fora pela porta e v para a direita de
quem olha a porta de fora (nos cones espelhados, o v do desenho é o contrário).
------------------------------------------------------------------
"""

import bpy
import bmesh
import json
import math
import os
import shutil
import struct
import sys
import tempfile
import time
import numpy as np
from mathutils import Matrix, Vector

INICIO = time.time()
bpy.ops.wm.read_factory_settings(use_empty=True)     # sem o cubo, a câmera e a luz da cena inicial
AQUI = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(AQUI)
PASTA_DAS_FONTES = os.path.join(REPO, 'recursos', 'fontes')
PASTA_DOS_CARROS = os.path.join(REPO, 'recursos', 'modelos', 'carros')

# ------------------------------------------------------------------
# Argumentos
# ------------------------------------------------------------------
argv = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
OPCOES_COM_VALOR = ('--previas', '--amostras', '--quadros')
opcoes, posicionais = {}, []
NOITE = '--noite' in argv          # a luz de noite: grava <saida>_noite.glb e <saida>_noite.json (veja 9.)
argv = [a for a in argv if a != '--noite']
i = 0
while i < len(argv):
    if argv[i] in OPCOES_COM_VALOR and i + 1 < len(argv):
        opcoes[argv[i]] = argv[i + 1]
        i += 2
    else:
        posicionais.append(argv[i])
        i += 1
ARQUIVO_DO_MAPA = posicionais[0] if posicionais else os.path.join(REPO, 'recursos', 'modelos', 'radiator_springs.glb')
SAIDA = posicionais[1] if len(posicionais) > 1 else os.path.join(REPO, 'recursos', 'modelos', 'cozy_cone_quartos')
if SAIDA.endswith('.glb'):
    SAIDA = SAIDA[:-4]
PREVIAS = opcoes.get('--previas')
AMOSTRAS = int(opcoes.get('--amostras', 512))
ATLAS_GUARDADO = opcoes.get('--quadros')
if NOITE and not os.path.exists(SAIDA + '.glb'):
    raise SystemExit('ERRO: --noite precisa do .glb de dia (%s.glb): a noite usa a geometria dele' % SAIDA)

# ------------------------------------------------------------------
# Os cones no mapa (medidos em radiator_springs.glb: a casca, o painel da porta e
# os vidros nos objetos ror_Object_61, ror_Object_60 e ror_Object_63; eixos do jogo)
# ------------------------------------------------------------------
# número, eixo (x, z: o meio do anel da casca em y 3,24), rumo do jogo para fora da porta e se o
# cone é espelhado (o ar-condicionado fica à direita de quem olha a porta de fora). O cone 1 é o do
# McQueen: o mais perto de vaga_mcqueen (641,5; 152,0).
CONES = [
    (1, (628.360, 155.531), 1.5623, False),
    (2, (629.057, 198.835), 1.5623, False),
    (3, (638.745, 248.780), 1.1921, False),
    (4, (680.422, 282.698), 0.4159, False),
    (5, (729.430, 277.370), -0.4005, True),
    (6, (762.661, 249.155), -1.1922, True),
    (7, (774.427, 204.663), -1.5623, True),
    (8, (775.327, 157.914), -1.5623, True),
]
CONE_DO_McQUEEN = 1

# As peças da casca que se tiram do mapa (caixas que contêm cada uma inteira; eixos do jogo; conferidas
# com testar_aberturas.py no modelo original do Race-O-Rama, x 4,5): o painel pintado da porta (6 faces
# do Object_60) e, em cada janela redonda, o vidro (21 faces do Object_60) e a lasca atrás dele (1 face
# do Object_61). (cone, nome, caixa mínima, caixa máxima, faces)
ABERTURAS_DA_CASCA = [
    (1, 'porta', (632.065, 2.85, 153.302), (632.218, 6.339, 157.607), 6),
    (1, 'direita', (629.63, 4.62, 150.254), (631.157, 6.43, 150.961), 22),
    (1, 'esquerda', (629.529, 4.613, 159.973), (631.225, 6.457, 160.718), 22),
    (2, 'porta', (632.762, 2.85, 196.607), (632.915, 6.339, 200.912), 6),
    (2, 'direita', (630.327, 4.62, 193.559), (631.854, 6.43, 194.265), 22),
    (2, 'esquerda', (630.226, 4.613, 203.277), (631.922, 6.457, 204.022), 22),
    (3, 'porta', (641.404, 2.85, 245.316), (643.113, 6.339, 249.348), 6),
    (3, 'direita', (638.02, 4.62, 243.397), (639.721, 6.43, 243.588), 22),
    (3, 'esquerda', (641.716, 4.613, 251.868), (643.014, 6.457, 253.156), 22),
    (4, 'porta', (679.943, 2.85, 278.275), (683.916, 6.339, 280.109), 6),
    (4, 'direita', (676.144, 4.62, 278.281), (677.433, 6.43, 279.35), 22),
    (4, 'esquerda', (685.601, 4.613, 281.893), (685.715, 6.457, 283.754), 22),
    (5, 'porta', (725.982, 2.849, 272.972), (729.996, 6.339, 274.751), 6),
    (5, 'direita', (724.119, 4.613, 276.509), (724.233, 6.456, 278.365), 22),
    (5, 'esquerda', (732.517, 4.62, 273.008), (733.787, 6.43, 274.091), 22),
    (6, 'porta', (758.31, 2.849, 245.688), (760.024, 6.339, 249.735), 6),
    (6, 'direita', (758.404, 4.613, 252.262), (759.694, 6.456, 253.55), 22),
    (6, 'esquerda', (761.707, 4.62, 243.756), (763.403, 6.43, 243.95), 22),
    (7, 'porta', (770.568, 2.849, 202.436), (770.721, 6.339, 206.74), 6),
    (7, 'direita', (771.561, 4.613, 209.105), (773.257, 6.456, 209.85), 22),
    (7, 'esquerda', (771.629, 4.62, 199.389), (773.157, 6.43, 200.095), 22),
    (8, 'porta', (771.468, 2.849, 155.687), (771.62, 6.339, 159.991), 6),
    (8, 'direita', (772.461, 4.613, 162.356), (774.157, 6.456, 163.101), 22),
    (8, 'esquerda', (772.529, 4.62, 152.639), (774.057, 6.43, 153.346), 22),
]

# As medidas de dentro, iguais nos oito cones (até 2 cm), em (u, v) a partir do eixo, nos cones de 1
# a 4 (nos espelhados, v troca de sinal)
PISO_Y = 2.83            # o chão de dentro: o alto da base quadrada do cone (a soleira da casca está em 2,81)
ALTURA = 6.0             # do piso ao teto (a casca, oca, sobe até y 23,5)
TETO_Y = PISO_Y + ALTURA
# A parede de cone: raio = R_PISO - INCLINACAO x (altura acima do piso), com o meio no eixo. Fica uns 28 cm
# para dentro do lado de dentro da casca (menos onde a casca chega mais perto, nas costas), para que o tubo
# da moldura das janelas da casca, que entra 8 cm além do vidro, fique atrás dela
R_PISO = 5.556
INCLINACAO = 0.2156
U_FRENTE = 3.60          # a parede da porta: reta, logo atrás dos batentes da casca (que vão de u 3,63 a 4,03)
U_SOLEIRA = 3.80         # o piso de dentro vai até aqui no vão (a rampa da casca começa em u 3,78, em y 2,81)
PORTA_V = (-2.10, 2.25)  # o vão livre entre os batentes da casca
PORTA_ALTURA = 3.54      # a verga da casca está em y 6,37
# As janelas redondas: o meio do vidro da casca (u, y, v), a normal dele (para fora) e o raio do vidro.
# O tubo da moldura da casca vai do vidro até 8 cm para dentro, com o raio de dentro a partir de 0,74 m.
JANELA_DO_AR = ((2.052, 5.536, -4.831), (0.4389, 0.0848, -0.8945))
JANELA_DA_MESA = ((2.073, 5.526, 4.906), (0.4401, 0.0848, 0.8939))
RAIO_DO_VAO = 0.82       # o furo na nossa parede (no plano do vidro)
RAIO_DA_JANELA = 0.73    # o vão livre, na ponta de fora do chanfro
FUNDO_DA_JANELA = -0.095 # a ponta de fora do chanfro, ao longo da normal, a partir do vidro (antes do tubo da casca)
# O ar-condicionado da casca atravessa a parede e entra no quarto até o raio 4,73: a caixa dele (a face de
# dentro, de ponta a ponta, em y 3,31) e a direção dele para fora
AR_CASCA_A = np.array([1.179, -4.728])
AR_CASCA_B = np.array([2.945, -3.788])
AR_CASCA_FORA = np.array([0.431, -0.902])
# O lado de fora da casca (um polígono de 16 lados, de uma face só): o raio nos cantos em função de y (os
# anéis do desenho; entre eles, reto). No meio de cada lado o raio é cos(pi/16) disto.
PERFIL_DA_CASCA = [(2.83, 6.44), (3.24, 6.00), (7.56, 5.14), (23.49, 1.33)]
# Os batentes e a verga da porta (Object_63 do mapa; os oito cones iguais até 2 cm): o lado de dentro dos
# batentes vai de u 3,63 a 4,04, em v -2,11 a -2,10 e 2,24 a 2,28; a frente deles fica em u 4,01 a 4,04,
# até v -2,56 e 2,73; a verga, embaixo em y 6,37 e na frente até y 6,57. Por cima vai um forro creme, 1,5 a
# 4 cm à frente de cada face (eles têm uma textura de cromado esverdeado, esticada)
FORRO_U = (3.55, 4.06)           # do quarto até a frente dos batentes
FORRO_V = (-2.085, 2.22)         # o lado de dentro dos batentes
FORRO_FRENTE_V = (-2.565, 2.735) # até onde vai a frente dos batentes (encostando no nicho da casca)
FORRO_VERGA = 3.52               # o forro embaixo da verga (acima do piso)
FORRO_ALTO = 3.72                # o alto do forro na frente (o teto do nicho está em y 6,54)

# ------------------------------------------------------------------
# O arranjo de cada quarto (em u, v, nos cones de 1 a 4)
# ------------------------------------------------------------------
PASSO = 0.42                 # a grade das superfícies médias
PASSO_DA_PAREDE_V = 0.30     # a parede da porta, em v
PASSO_DO_PISO = 0.25
PASSO_DO_TETO = 0.50
COLUNAS_DA_PAREDE = 120      # a parede de cone, em fatias de 3 graus (0,29 m embaixo)
NIVEIS = [0.0, 0.27, 0.54, 0.80, 1.10, 1.40, 1.70, 2.00, 2.30, 2.60, 2.90, 3.20, PORTA_ALTURA, 3.80, 4.10,
          4.40, 4.70, 5.00, 5.30, 5.60, ALTURA]
ALTO_DA_BARRA = 0.80         # a barra mais escura (com o friso de dentes) vai do piso até aqui
LUGAR_U = 0.20               # o meio do carro estacionado (e dos tapetes), de nariz para a porta
TAPETES_U = (-2.35, 2.75)
TAPETES_V = 0.95             # o meio de cada tapete (um embaixo de cada lado do carro)
LARGURA_DO_TAPETE = 0.60
ALTO_DO_TAPETE = 0.025       # a fibra de coco é grossa: uma caixa de 2,5 cm, com o chanfro em volta
CHANFRO_DO_TAPETE = 0.012
TRAMA_DO_TAPETE = 0.04       # a beira trançada, em volta
MESA = (118.0, 4.80)         # a mesinha: ângulo (graus, a partir da porta, para +v) e distância do eixo
LUMINARIA = (-108.0, 4.40)
# Perto da porta, no canto entre a parede da porta e o ar-condicionado (a uns 0,8 m da beira do vão, fora
# do caminho do carro): a luminária de cone pequena e o interruptor de chão (u, v)
CONEZINHO_DA_PORTA = (3.22, -3.45)
INTERRUPTOR = (2.80, -3.10)
# Os quadros de cone na parede: (ângulo, altura do meio acima do piso, altura do quadro)
QUADROS_NA_PAREDE = [(95.0, 2.40, 1.80), (125.0, 2.55, 2.05), (154.0, 2.40, 1.75), (180.0, 2.50, 1.95),
                     (-153.0, 2.40, 1.80), (-127.0, 2.55, 2.00), (-93.0, 2.35, 1.65)]
QUADRO_DA_FRENTE = (3.00, 2.25)      # v e altura do quadro (ou do pôster do McQueen) na parede da porta
# Os quadros de cada quarto (o índice da pintura no atlas, na ordem de QUADROS_NA_PAREDE e depois o da frente)
PINTURAS_DOS_CONES = {
    1: [0, 3, 6, 1, 5, 2, 7, None],
    2: [1, 5, 2, 0, 3, 7, 4, 6],
    3: [2, 7, 0, 4, 6, 1, 3, 5],
    4: [3, 1, 5, 6, 0, 4, 2, 7],
    5: [4, 6, 1, 2, 7, 0, 5, 3],
    6: [5, 0, 3, 7, 1, 6, 4, 2],
    7: [6, 2, 4, 5, 3, 0, 7, 1],
    8: [7, 4, 6, 3, 2, 5, 1, 0],
}

# ------------------------------------------------------------------
# Cores (na tela, 0 a 1)
# ------------------------------------------------------------------
PAPEL_DE_PAREDE = (0.97, 0.71, 0.52)    # pêssego (a textura já tem a cor; o vértice vai branco)
BARRA = (0.80, 0.50, 0.39)              # a barra de baixo, mais escura
DENTE = (0.60, 0.31, 0.25)              # os triângulos do friso
FRISO = (0.90, 0.64, 0.50)              # a cimalha em cima da barra
RODAPE = (0.40, 0.20, 0.15)
TETO = (0.96, 0.82, 0.70)
SANCA = (0.95, 0.84, 0.72)
CREME = (0.93, 0.88, 0.78)              # a moldura da porta, as janelas
CREME_ESCURO = (0.78, 0.72, 0.62)
VAO_DA_JANELA = (0.90, 0.66, 0.50)       # o chanfro da janela redonda, na cor da parede
AR_COR = (0.88, 0.86, 0.80)             # o ar-condicionado
AR_ESCURO = (0.52, 0.50, 0.47)
CORTINA = (0.95, 0.91, 0.84)
BANDO = (0.92, 0.86, 0.76)
LACO = (0.88, 0.48, 0.36)               # a fita que amarra a cortina
LARANJA = (0.95, 0.47, 0.15)            # os cones (relógio, cúpulas, enfeites)
LARANJA_ESCURO = (0.70, 0.30, 0.10)
MADEIRA = (0.45, 0.25, 0.14)            # a mesinha
MADEIRA_CLARA = (0.62, 0.38, 0.22)
MOLDURA = (0.50, 0.28, 0.15)            # as molduras dos quadros
MOLDURA_CLARA = (0.78, 0.52, 0.30)
LATAO = (0.78, 0.62, 0.32)
OURO = (0.92, 0.72, 0.30)
PRETO = (0.07, 0.065, 0.06)
METAL = (0.62, 0.62, 0.63)
TERRACOTA = (0.80, 0.42, 0.26)
ROSA_VERMELHA = (0.74, 0.05, 0.09)
ROSA_ESCURA = (0.40, 0.02, 0.05)
FOLHA = (0.20, 0.42, 0.18)
CAULE = (0.25, 0.45, 0.20)
BRANCO = (1.0, 1.0, 1.0)
LUZ_DA_CUPULA = (1.0, 0.86, 0.58)       # a cor do vértice do lado de dentro das cúpulas acesas
LUZ_DA_LAMPADA = (1.0, 0.96, 0.86)
LUZ_DO_CONEZINHO = (1.0, 0.62, 0.26)


def linear(c):
    return tuple(((x + 0.055) / 1.055) ** 2.4 if x > 0.04045 else x / 12.92 for x in c[:3])


def tela(c):
    c = np.clip(np.asarray(c, float), 0.0, None)
    return np.where(c > 0.0031308, 1.055 * np.power(c, 1 / 2.4) - 0.055, 12.92 * c)


def unit(v):
    v = np.asarray(v, float)
    return v / np.linalg.norm(v)


def misturar(a, b, t):
    return tuple(float(x) * (1 - t) + float(y) * t for x, y in zip(a, b))


def raio_da_parede(h):
    """O raio (horizontal) da parede de cone, h metros acima do piso"""
    return R_PISO - INCLINACAO * h


def raio_da_casca(h):
    """O raio do lado de fora da casca no meio de um lado do polígono, h metros acima do piso"""
    ys, rs = zip(*PERFIL_DA_CASCA)
    return float(np.interp(PISO_Y + h, ys, rs)) * math.cos(math.pi / 16)


def meia_largura_da_frente(h, folga=0.0):
    """Até onde (em |v|) vai a parede da porta, h acima do piso (onde ela encontra o cone)"""
    r = raio_da_parede(h) + folga
    return math.sqrt(max(r * r - U_FRENTE * U_FRENTE, 0.0))


def rumo_de(d):
    """O rumo do jogo de uma direção (x, z) (0 olhando para -z, crescendo para a direita)"""
    return math.atan2(d[0], -d[1])


# ------------------------------------------------------------------
# Imagens
# ------------------------------------------------------------------
PASTA_TEMP = tempfile.mkdtemp(prefix='quartos_do_cozy_cone_')


def imagem(nome, arr):
    """arr: altura x largura x 3 ou 4 (0 a 1, na tela), a linha 0 em cima. Vira PNG embutido."""
    arr = np.clip(np.asarray(arr, np.float32), 0, 1)
    h, w = arr.shape[:2]
    if arr.shape[2] == 3:
        arr = np.concatenate([arr, np.ones((h, w, 1), np.float32)], axis=2)
    img = bpy.data.images.new(nome, w, h, alpha=False)
    img.pixels.foreach_set(np.flipud(arr).ravel())
    caminho = os.path.join(PASTA_TEMP, nome + '.png')
    img.filepath_raw = caminho
    img.file_format = 'PNG'
    img.save()
    bpy.data.images.remove(img)
    img = bpy.data.images.load(caminho)
    img.name = nome
    img.pack()
    return img


def salvar_png(arr, caminho):
    arr = np.clip(np.asarray(arr, np.float32), 0, 1)
    h, w = arr.shape[:2]
    if arr.shape[2] == 3:
        arr = np.concatenate([arr, np.ones((h, w, 1), np.float32)], axis=2)
    img = bpy.data.images.new('salvar', w, h, alpha=False)
    img.pixels.foreach_set(np.flipud(arr).ravel())
    img.filepath_raw = caminho
    img.file_format = 'PNG'
    img.save()
    bpy.data.images.remove(img)


def ler_png(caminho):
    img = bpy.data.images.load(caminho)
    w, h = img.size
    a = np.empty(w * h * 4, np.float32)
    img.pixels.foreach_get(a)
    bpy.data.images.remove(img)
    return np.flipud(a.reshape(h, w, 4))


def ruido(h, w, sx, sy, rng):
    """Ruído que se repete nas bordas (para ladrilhar), borrado sx, sy pixels; média 0, desvio 1"""
    r = rng.standard_normal((h, w))
    fy = np.fft.fftfreq(h)[:, None]
    fx = np.fft.fftfreq(w)[None, :]
    f = np.exp(-2 * np.pi ** 2 * ((fx * sx) ** 2 + (fy * sy) ** 2))
    r = np.real(np.fft.ifft2(np.fft.fft2(r) * f))
    return (r - r.mean()) / (r.std() + 1e-9)


def matriz_de_reamostrar(n_de, n_para):
    """Pesos (n_para x n_de) para levar n_de amostras a n_para: a média das áreas (com as frações das
    bordas) quando reduz e a interpolação linear quando amplia (nenhuma linha fica sem pixel)"""
    M = np.zeros((n_para, n_de))
    if n_para <= n_de:
        s = n_de / n_para
        j = np.arange(n_de)
        for i in range(n_para):
            a, b = i * s, (i + 1) * s
            M[i] = np.clip(np.minimum(b, j + 1) - np.maximum(a, j), 0, None) / s
    else:
        x = (np.arange(n_para) + 0.5) * n_de / n_para - 0.5
        x = np.clip(x, 0, n_de - 1)
        x0 = np.floor(x).astype(int)
        x1 = np.minimum(x0 + 1, n_de - 1)
        f = x - x0
        M[np.arange(n_para), x0] += 1 - f
        M[np.arange(n_para), x1] += f
    return M


def redimensionar(img, w, h):
    """Leva a imagem (altura x largura x canais) a w x h (sem PIL), um eixo de cada vez"""
    My = matriz_de_reamostrar(img.shape[0], h)
    Mx = matriz_de_reamostrar(img.shape[1], w)
    tmp = np.tensordot(My, np.asarray(img, np.float64), axes=(1, 0))      # h x W x c
    return np.transpose(np.tensordot(tmp, Mx, axes=(1, 1)), (0, 2, 1)).astype(np.float32)


def suave(a, b, x):
    t = np.clip((x - a) / (b - a), 0, 1)
    return t * t * (3 - 2 * t)


# ------------------------------------------------------------------
# 1. Texturas
# ------------------------------------------------------------------
METROS_DO_PAPEL = 2.4           # o papel de parede: um ladrilho de 2,4 x 2,4 m (1024 px)
METROS_DO_CARPETE = 2.0
METROS_DO_CAPACHO = 0.6         # a fibra de coco: um ladrilho de 0,6 m ao comprido, a largura inteira do tapete
R_DO_PAPEL = 13 * METROS_DO_PAPEL / (2 * math.pi)   # o papel dá 13 voltas no cone (sem emenda nas costas)


def textura_do_papel():
    """Papel de parede pêssego, com conezinhos (o triângulo e o pé) um pouco mais claros, em fileiras
    desencontradas, bem apagados, e a fibra do papel"""
    rng = np.random.default_rng(31)
    n = 1024
    px = n / METROS_DO_PAPEL                    # pixels por metro
    yy, xx = np.mgrid[0:n, 0:n].astype(np.float32)
    cor = np.broadcast_to(np.array(PAPEL_DE_PAREDE, np.float32), (n, n, 3)).copy()
    motivo = np.zeros((n, n), np.float32)
    passo = 0.6 * px                            # 4 por fileira, 4 fileiras (repete na borda)
    for linha in range(4):
        y0 = (linha + 0.5) * passo
        desl = 0.5 * passo if linha % 2 else 0.0
        for col in range(5):
            x0 = (col * passo + desl) % n
            for dx in (-n, 0, n):
                cx = x0 + dx
                # um cone de 0,26 m de altura: o corpo (trapézio) e o pé (retângulo)
                alto, larg = 0.26 * px, 0.15 * px
                t = (yy - (y0 - alto / 2)) / alto      # 0 em cima, 1 no pé
                meia = (0.12 + 0.88 * t) * larg / 2
                corpo = (t > 0) & (t < 0.86) & (np.abs(xx - cx) < meia)
                pe = (t >= 0.86) & (t < 1.0) & (np.abs(xx - cx) < larg * 0.62)
                motivo = np.maximum(motivo, (corpo | pe).astype(np.float32))
    # bordas macias
    m = np.fft.ifft2(np.fft.fft2(motivo) * np.exp(-2 * np.pi ** 2 * ((np.fft.fftfreq(n)[None, :] * 1.6) ** 2 +
                                                                      (np.fft.fftfreq(n)[:, None] * 1.6) ** 2))).real
    cor = cor * (1 + 0.035 * m[..., None]) + np.array([0.0, 0.008, 0.012]) * m[..., None]
    # a fibra do papel: um ruído fino e fraco, igual nas duas direções (um ruído "escovado", em pé, fazia
    # listras junto com as colunas da parede de cone)
    fibra = ruido(n, n, 1.2, 1.2, rng)
    mancha = ruido(n, n, 60, 60, rng)
    cor *= (1 + 0.008 * fibra + 0.018 * mancha)[..., None]
    return np.clip(cor, 0, 1)


def textura_do_carpete():
    """Carpete bege-acinzentado: o pelo (ruído fino), tufos e manchas largas bem suaves"""
    rng = np.random.default_rng(7)
    n = 512
    fino = ruido(n, n, 0.6, 0.6, rng)
    tufo = ruido(n, n, 2.2, 2.2, rng)
    largo = ruido(n, n, 40, 40, rng)
    v = 1 + 0.06 * fino + 0.05 * tufo + 0.025 * largo
    v = v - np.where(fino < -2.1, 0.08, 0)
    cor = np.array([0.76, 0.72, 0.67])[None, None, :] * v[..., None]
    return np.clip(cor, 0, 1)


COR_DO_CAPACHO = np.array([0.66, 0.49, 0.29])     # castanho-dourado (a média, na tela)


def textura_do_capacho():
    """Fibra de coco, como o capacho do filme: tufos grossos e eriçados de 8 a 15 mm, com as pontas claras e
    os vãos escuros bem marcados, e a beira trançada (TRAMA_DO_TAPETE) dos dois lados compridos. u ao
    comprido do tapete (repete a cada METROS_DO_CAPACHO), v de um lado ao outro (0 a 1); as pontas do
    tapete usam a mesma faixa trançada (girada)"""
    rng = np.random.default_rng(13)
    n = 512
    px = n / METROS_DO_CAPACHO                       # pixels por metro (853: 1 cm = 8,5 px)
    yy, xx = np.mgrid[0:n, 0:n].astype(np.float32)
    # os tufos: um centro sorteado em cada célula de uma grade que repete na borda (Worley: F1, F2)
    nc = 52                                          # 11,5 mm por célula
    cel = n / nc
    cx = (np.arange(nc)[None, :] + 0.5 + rng.uniform(-0.36, 0.36, (nc, nc))) * cel
    cy = (np.arange(nc)[:, None] + 0.5 + rng.uniform(-0.36, 0.36, (nc, nc))) * cel
    raio = rng.uniform(0.004, 0.0075, (nc, nc)) * px          # 8 a 15 mm de diâmetro
    tom = rng.uniform(0.82, 1.18, (nc, nc))
    giro = rng.uniform(0, 2 * np.pi, (nc, nc))
    ci, cj = (yy // cel).astype(int), (xx // cel).astype(int)
    F1 = np.full((n, n), 9.0, np.float32)
    F2 = np.full((n, n), 9.0, np.float32)
    T1 = np.ones((n, n), np.float32)
    A1 = np.zeros((n, n), np.float32)
    for di in (-1, 0, 1):
        for dj in (-1, 0, 1):
            ia, ja = ci + di, cj + dj
            i, j = ia % nc, ja % nc
            ccx = cx[i, j] + n * np.floor_divide(ja, nc)
            ccy = cy[i, j] + n * np.floor_divide(ia, nc)
            d = np.hypot(xx - ccx, yy - ccy) / raio[i, j]
            perto = d < F1
            F2 = np.where(perto, F1, np.minimum(F2, d))
            F1 = np.where(perto, d, F1)
            T1 = np.where(perto, tom[i, j], T1)
            A1 = np.where(perto, np.arctan2(yy - ccy, xx - ccx) + giro[i, j], A1)
    alto = np.clip(1 - F1 ** 1.4, 0, 1)                       # o tufo: alto no meio, baixo na beira
    cerdas = 0.5 + 0.5 * np.sin(A1 * 13.0)                   # as cerdas abrindo do meio do tufo
    fino = ruido(n, n, 0.6, 0.6, rng)
    vao = suave(0.0, 0.35, F2 - F1)                          # escuro onde dois tufos se encontram
    # fios soltos por cima, em direções que mudam devagar (o capacho é eriçado, não liso)
    mistura = np.clip(0.5 + 0.5 * ruido(n, n, 18, 18, rng), 0, 1)
    fios = np.maximum(ruido(n, n, 0.45, 2.6, rng) * mistura, ruido(n, n, 2.6, 0.45, rng) * (1 - mistura))
    fios = suave(0.9, 2.2, fios)
    v = (0.22 + 0.78 * alto * (0.66 + 0.34 * cerdas)) * T1 * (0.58 + 0.42 * vao) + 0.05 * fino
    v = v * (1 - 0.55 * fios) + 0.95 * 0.55 * fios
    # a beira trançada (dos dois lados compridos): espinha de peixe, as fibras mais apertadas
    b = TRAMA_DO_TAPETE / LARGURA_DO_TAPETE * n
    dist = np.minimum(yy, n - 1 - yy)
    lado = np.where(dist < b / 2, 1.0, -1.0) * np.where(yy < n / 2, 1.0, -1.0)
    P = n / 36.0
    trama = 0.5 + 0.5 * np.sin(2 * np.pi * (xx + lado * yy) / P)
    fio = 0.5 + 0.5 * np.sin(2 * np.pi * (xx * 0.15 - lado * yy * 1.0) / 3.1)
    v_trama = (0.30 + 0.55 * trama ** 0.7 * (0.8 + 0.2 * fio)) + 0.05 * fino
    v_trama *= 1 - 0.35 * suave(3.0, 0.0, dist)                # a última fiada, um pouco mais escura
    v_trama *= 1 - 0.25 * (np.abs(dist - b / 2) < 1.0)        # a costura no meio da faixa
    na_trama = suave(b + 1.5, b - 1.5, dist)
    v = v * (1 - na_trama) + v_trama * 0.92 * na_trama
    # de valor para cor: escuro nos vãos, castanho-dourado no meio, palha nas pontas
    escuro = np.array([0.24, 0.15, 0.08])
    claro = np.array([0.90, 0.75, 0.50])
    v = v[..., None]
    meio = COR_DO_CAPACHO
    cor = np.where(v < 0.55, escuro + (meio - escuro) * (v / 0.55), meio + (claro - meio) * np.clip((v - 0.55) / 0.45, 0, 1.3))
    cor *= (1 + 0.05 * ruido(n, n, 40, 40, rng))[..., None]   # manchas largas, bem leves
    cor *= (COR_DO_CAPACHO / cor.reshape(-1, 3).mean(0))[None, None, :]   # a média fica na cor do capacho
    return np.clip(cor, 0, 1)


print('Desenhando as texturas...')
TEXTURAS = {'parede': textura_do_papel(), 'carpete': textura_do_carpete(), 'capacho': textura_do_capacho()}
IMG_PAPEL = imagem('cozy_papel_de_parede', TEXTURAS['parede'])
IMG_CARPETE = imagem('cozy_carpete', TEXTURAS['carpete'])
IMG_CAPACHO = imagem('cozy_fibra_de_coco', TEXTURAS['capacho'])
print('  %.0f s' % (time.time() - INICIO))


# ------------------------------------------------------------------
# 2. Os quadros (paisagens pintadas), o pôster do McQueen e o relógio, num atlas
# ------------------------------------------------------------------
FONTE_TITULO = os.path.join(PASTA_DAS_FONTES, 'racing_sans_one.ttf')
FONTE_TEXTO = os.path.join(PASTA_DAS_FONTES, 'saira_condensed_bold.ttf')

# O atlas (2048 x 1024): 12 células de 320 x 480 (6 por linha), mais o mostrador do relógio (128 x 128)
# na faixa da direita. Cada célula tem uma borda de 4 pixels repetidos (as mipmaps não puxam a vizinha).
TAM_DO_ATLAS = (2048, 1024)
BORDA_DA_CELULA = 4
CELULAS = [(k % 6 * 320, k // 6 * 480, 320, 480) for k in range(12)] + [(1920, 0, 128, 128)]
CELULA_DO_POSTER = 8
CELULA_DO_RELOGIO = 12
ASPECTO_DO_QUADRO = 320 / 480


def uv_da_celula(k):
    """(u0, v0, u1, v1) da parte útil da célula k, nas coordenadas do Blender (v para cima)"""
    x, y, w, h = CELULAS[k]
    b = BORDA_DA_CELULA + 1
    W, H = TAM_DO_ATLAS
    return ((x + b) / W, 1 - (y + h - b) / H, (x + w - b) / W, 1 - (y + b) / H)


def novo_material(nome):
    m = bpy.data.materials.new(nome)
    try:
        m.use_nodes = True
    except Exception:
        pass
    return m


def novo_mundo(nome):
    w = bpy.data.worlds.new(nome)
    try:
        w.use_nodes = True
    except Exception:
        pass
    return w


def material_emissao(nome, fonte='objeto'):
    """Material que só brilha: com a cor do objeto (texto) ou com a cor dos vértices (fundos)"""
    m = novo_material(nome)
    nt = m.node_tree
    nt.nodes.clear()
    saida = nt.nodes.new('ShaderNodeOutputMaterial')
    em = nt.nodes.new('ShaderNodeEmission')
    if fonte == 'objeto':
        info = nt.nodes.new('ShaderNodeObjectInfo')
        nt.links.new(info.outputs['Color'], em.inputs['Color'])
    else:
        at = nt.nodes.new('ShaderNodeAttribute')
        at.attribute_name = 'Col'
        nt.links.new(at.outputs['Color'], em.inputs['Color'])
    nt.links.new(em.outputs[0], saida.inputs['Surface'])
    return m


def importar_carro(nome_do_arquivo, comprimento=4.5):
    """Importa um carro do jogo, com 'comprimento' metros, de pé no chão (z = 0) e com a frente para -y"""
    antes = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=os.path.join(PASTA_DOS_CARROS, nome_do_arquivo + '.glb'))
    novos = [o for o in bpy.data.objects if o not in antes]
    bpy.context.view_layer.update()
    mn = Vector((1e9,) * 3)
    mx = Vector((-1e9,) * 3)
    for o in novos:
        if o.type == 'MESH':
            for c in o.bound_box:
                w = o.matrix_world @ Vector(c)
                mn = Vector(map(min, mn, w))
                mx = Vector(map(max, mx, w))
    esc = comprimento / max(mx.x - mn.x, mx.y - mn.y)
    raiz = bpy.data.objects.new('carro_' + nome_do_arquivo, None)
    bpy.context.scene.collection.objects.link(raiz)
    meio = Vector(((mn.x + mx.x) / 2, (mn.y + mx.y) / 2, mn.z))
    for o in novos:
        if o.parent is None:
            o.parent = raiz
            o.location = (o.location - meio) * esc
            o.scale = o.scale * esc
    bpy.context.view_layer.update()
    # a frente: o lado comprido mais estreito... os carros do jogo vêm com a frente para -y ou +x
    return raiz, [o for o in novos if o.type == 'MESH'], esc, (mx.x - mn.x) > (mx.y - mn.y)


def cor_da_palpebra(o, img):
    """Se img é a textura dos olhos (o branco do olho com a íris em cima e a pálpebra, da cor do carro,
    embaixo), devolve a cor da pálpebra (linear); senão, None"""
    if img is None or img.size[0] < 16 or img.size[1] < 16:
        return None
    w, h = img.size
    a = np.empty(w * h * 4, np.float32)
    img.pixels.foreach_get(a)
    a = a.reshape(h, w, 4)[..., :3]
    baixo = np.median(a[: max(1, h // 16)].reshape(-1, 3), axis=0)
    cima = np.median(a[-max(1, h // 16):].reshape(-1, 3), axis=0)
    satura = float(baixo.max() - baixo.min())
    olho = 'eye' in o.name.lower() or (cima.min() > 0.8 and satura > 0.12)
    if not olho:
        return None
    return linear(tuple(baixo))


def pintar_de_quadro(objetos, contorno):
    """Troca os materiais do carro por um sombreado em poucos tons (de pintura) e põe um contorno escuro
    (a casca invertida, com a face de trás escondida)"""
    feitos = {}
    for o in objetos:
        for slot in o.material_slots:
            m0 = slot.material
            if m0 is None:
                continue
            if m0.name in feitos:
                slot.material = feitos[m0.name]
                continue
            imagem_base, cor_base = None, (0.8, 0.8, 0.8, 1)
            for n in m0.node_tree.nodes:
                if n.type == 'BSDF_PRINCIPLED':
                    entrada = n.inputs['Base Color']
                    cor_base = tuple(entrada.default_value)
                    pilha = [l.from_node for l in entrada.links]
                    while pilha:
                        x = pilha.pop()
                        if x.type == 'TEX_IMAGE':
                            imagem_base = x.image
                            break
                        for e in x.inputs:
                            pilha += [l.from_node for l in e.links]
            m = novo_material(m0.name + '_quadro')
            nt = m.node_tree
            nt.nodes.clear()
            saida = nt.nodes.new('ShaderNodeOutputMaterial')
            dif = nt.nodes.new('ShaderNodeBsdfDiffuse')
            s2r = nt.nodes.new('ShaderNodeShaderToRGB')
            rampa = nt.nodes.new('ShaderNodeValToRGB')
            rampa.color_ramp.interpolation = 'CONSTANT'
            els = rampa.color_ramp.elements
            els[0].position = 0.0
            els[0].color = (0.42, 0.34, 0.40, 1)
            els[1].position = 0.12
            els[1].color = (0.70, 0.64, 0.62, 1)
            e = els.new(0.42)
            e.color = (1.0, 0.97, 0.92, 1)
            e = els.new(0.85)
            e.color = (1.22, 1.18, 1.10, 1)
            mult = nt.nodes.new('ShaderNodeMix')
            mult.data_type = 'RGBA'
            mult.blend_type = 'MULTIPLY'
            mult.inputs['Factor'].default_value = 1.0
            em = nt.nodes.new('ShaderNodeEmission')
            nt.links.new(dif.outputs[0], s2r.inputs[0])
            nt.links.new(s2r.outputs['Color'], rampa.inputs['Fac'])
            if imagem_base is not None:
                tex = nt.nodes.new('ShaderNodeTexImage')
                tex.image = imagem_base
                nt.links.new(tex.outputs['Color'], mult.inputs['A'])
            else:
                mult.inputs['A'].default_value = cor_base
            nt.links.new(rampa.outputs['Color'], mult.inputs['B'])
            nt.links.new(mult.outputs['Result'], em.inputs['Color'])
            nt.links.new(em.outputs[0], saida.inputs['Surface'])
            feitos[m0.name] = m
            slot.material = m
        if not o.data.materials or o.data.materials[-1] != contorno:
            o.data.materials.append(contorno)
        mod = o.modifiers.new('contorno', 'SOLIDIFY')
        mod.thickness = 0.03 / max(o.matrix_world.to_scale())
        mod.offset = 1.0
        mod.use_flip_normals = True
        mod.material_offset = len(o.data.materials) - 1
        mod.use_rim = False


def malha_plana(nome, pontos, cores, material, colecao):
    """Um polígono plano, com uma cor por ponto"""
    if len(pontos) < 3:
        return None
    me = bpy.data.meshes.new(nome)
    me.from_pydata([tuple(p) for p in pontos], [], [list(range(len(pontos)))])
    at = me.color_attributes.new('Col', 'FLOAT_COLOR', 'CORNER')
    for k, l in enumerate(me.loops):
        at.data[k].color = (*linear(cores[l.vertex_index]), 1)
    me.materials.append(material)
    o = bpy.data.objects.new(nome, me)
    colecao.objects.link(o)
    return o


class Tela:
    """Uma pintura: polígonos chapados (com degradês nas cores dos pontos) em planos a distâncias d da
    câmera (o menor fica na frente). Coordenadas (x, y): x de 0 a A (a largura sobre a altura), y de 0
    (embaixo) a 1 (em cima)."""

    def __init__(self, camada, mh, A, material, fontes, material_texto):
        self.camada, self.mh, self.A = camada, mh, A
        self.material, self.fontes, self.material_texto = material, fontes, material_texto

    def ponto(self, x, y, d):
        return ((x - self.A / 2) * 2 * self.mh * d, d, (y - 0.5) * 2 * self.mh * d)

    def poli(self, pts, cor, d):
        cores = cor if isinstance(cor, list) else [cor] * len(pts)
        malha_plana('p', [self.ponto(x, y, d) for x, y in pts], cores, self.material, self.camada)

    def retangulo(self, x0, y0, x1, y1, cor_baixo, cor_cima, d):
        self.poli([(x0, y0), (x1, y0), (x1, y1), (x0, y1)], [cor_baixo, cor_baixo, cor_cima, cor_cima], d)

    def ceu(self, paradas, d, x0=-0.1, x1=None):
        """Degradê vertical em faixas: paradas = [(y, cor), ...] de baixo para cima"""
        x1 = self.A + 0.1 if x1 is None else x1
        paradas = [(-0.1, paradas[0][1])] + list(paradas) + [(1.1, paradas[-1][1])]
        for (ya, ca), (yb, cb) in zip(paradas[:-1], paradas[1:]):
            self.retangulo(x0, ya, x1, yb, ca, cb, d)

    def elipse(self, cx, cy, rx, ry, cor, d, n=40, cor_borda=None):
        pts = [(cx + rx * math.cos(a), cy + ry * math.sin(a)) for a in np.linspace(0, 2 * math.pi, n, endpoint=False)]
        if cor_borda is None:
            self.poli(pts, cor, d)
        else:
            for k in range(n):
                self.poli([(cx, cy), pts[k], pts[(k + 1) % n]], [cor, cor_borda, cor_borda], d)

    def disco(self, cx, cy, r, cor, d, n=40, cor_borda=None):
        self.elipse(cx, cy, r, r, cor, d, n, cor_borda)

    def texto(self, txt, fonte, cor, x, y, larg, alt, d, sombra=None):
        if fonte not in self.fontes:
            self.fontes[fonte] = bpy.data.fonts.load(fonte, check_existing=True)
        for k, (dx, dy, c) in enumerate(([(0.006, -0.006, sombra)] if sombra else []) + [(0, 0, cor)]):
            cu = bpy.data.curves.new('txt', 'FONT')
            cu.body = txt
            cu.font = self.fontes[fonte]
            cu.align_x = 'CENTER'
            cu.align_y = 'CENTER'
            cu.size = 1.0
            o = bpy.data.objects.new('txt', cu)
            self.camada.objects.link(o)
            o.data.materials.append(self.material_texto)
            o.color = (*linear(c), 1)
            bpy.context.view_layer.update()
            dw, dh = max(o.dimensions.x, 1e-3), max(o.dimensions.y, 1e-3)
            dd = d + (0.05 if k == 0 and sombra else 0.0)
            esc = min(larg * 2 * self.mh * dd / dw, alt * 2 * self.mh * dd / dh)
            o.scale = (esc, esc, esc)
            o.rotation_euler = (math.pi / 2, 0, 0)
            o.location = self.ponto(x + dx, y + dy, dd)


def escurecer(c, k):
    return tuple(float(x) * k for x in c)


# --- as pinturas: cada uma recebe a tela e um gerador de números ---
def pintura_mesas(t, rng):
    """As mesas de rocha no pôr do sol, com a estrada sumindo no horizonte"""
    t.ceu([(0.30, (1.0, 0.82, 0.45)), (0.46, (0.99, 0.60, 0.36)), (0.68, (0.88, 0.42, 0.42)), (0.92, (0.48, 0.26, 0.44))], 80)
    t.disco(0.34, 0.47, 0.14, (1.0, 0.78, 0.48), 79.5, cor_borda=(0.99, 0.62, 0.38))
    t.disco(0.34, 0.47, 0.075, (1.0, 0.95, 0.70), 79)
    for k in range(4):                         # nuvens finas na frente do sol
        y = 0.55 + k * 0.06
        x0 = rng.uniform(-0.1, 0.3)
        t.poli([(x0, y), (x0 + 0.5, y + 0.01), (x0 + 0.48, y + 0.022), (x0 + 0.02, y + 0.016)], (0.98, 0.66, 0.50), 78.8)
    longe = [(-0.1, 0.33), (-0.1, 0.40), (0.02, 0.41), (0.04, 0.45), (0.17, 0.45), (0.19, 0.40), (0.38, 0.39),
             (0.40, 0.43), (0.50, 0.43), (0.52, 0.39), (0.62, 0.385), (0.64, 0.42), (0.70, 0.42), (0.80, 0.37), (0.80, 0.33)]
    t.poli(longe, (0.70, 0.38, 0.42), 78)
    t.retangulo(-0.1, -0.1, 0.8, 0.345, (0.50, 0.25, 0.14), (0.86, 0.52, 0.32), 77)
    butte = [(-0.1, 0.30), (-0.1, 0.67), (0.04, 0.67), (0.07, 0.64), (0.16, 0.64), (0.19, 0.57), (0.23, 0.40), (0.27, 0.30)]
    t.poli(butte, (0.52, 0.21, 0.15), 76)
    t.poli([(0.16, 0.64), (0.19, 0.57), (0.23, 0.40), (0.27, 0.30), (0.235, 0.30), (0.20, 0.40), (0.17, 0.56), (0.145, 0.63)], (0.90, 0.48, 0.26), 75.5)
    t.poli([(0.04, 0.67), (0.16, 0.67), (0.16, 0.64), (0.04, 0.64)], (0.62, 0.27, 0.18), 75.6)
    mesa = [(0.80, 0.30), (0.80, 0.55), (0.66, 0.55), (0.63, 0.52), (0.57, 0.52), (0.54, 0.42), (0.50, 0.30)]
    t.poli(mesa, (0.56, 0.24, 0.17), 76)
    t.poli([(0.57, 0.52), (0.54, 0.42), (0.50, 0.30), (0.535, 0.30), (0.57, 0.42), (0.595, 0.51)], (0.92, 0.52, 0.30), 75.5)
    estrada = [(0.10, -0.1), (0.56, -0.1), (0.44, 0.10), (0.385, 0.22), (0.358, 0.335), (0.350, 0.335), (0.33, 0.22), (0.25, 0.10)]
    t.poli(estrada, [(0.28, 0.22, 0.24), (0.28, 0.22, 0.24), (0.36, 0.27, 0.28), (0.44, 0.31, 0.32), (0.56, 0.38, 0.38),
                     (0.56, 0.38, 0.38), (0.44, 0.31, 0.32), (0.36, 0.27, 0.28)], 74)
    for k in range(7):
        a = k / 7.0
        y0, y1 = -0.08 + a * 0.40, -0.08 + (a + 0.07) * 0.40
        def meio(y):
            return 0.335 + 0.02 * math.sin(y * 9) + 0.0 * y
        w0, w1 = 0.010 * (1 - a), 0.010 * (1 - a - 0.07)
        t.poli([(meio(y0) - w0, y0), (meio(y0) + w0, y0), (meio(y1) + w1, y1), (meio(y1) - w1, y1)], (0.98, 0.84, 0.38), 73)
    for x, y, s in ((0.58, 0.12, 0.09), (0.06, 0.08, 0.07), (0.66, 0.24, 0.05)):      # cactos
        cor = (0.24, 0.20, 0.18)
        t.poli([(x - 0.01 * s / 0.07, y), (x + 0.01 * s / 0.07, y), (x + 0.01 * s / 0.07, y + s), (x - 0.01 * s / 0.07, y + s)], cor, 72)
        t.poli([(x + 0.01, y + s * 0.4), (x + 0.035, y + s * 0.4), (x + 0.035, y + s * 0.8), (x + 0.022, y + s * 0.8),
                (x + 0.022, y + s * 0.52), (x + 0.01, y + s * 0.52)], cor, 72)


def pintura_rota_66(t, rng):
    """A Rota 66 reta até o horizonte, com o escudo do 66, um cone na beira e as mesas lá longe"""
    t.ceu([(0.42, (0.90, 0.90, 0.86)), (0.58, (0.62, 0.78, 0.92)), (0.95, (0.26, 0.50, 0.82))], 80)
    for cx, cy, rx in ((0.15, 0.82, 0.12), (0.30, 0.86, 0.09), (0.52, 0.78, 0.14), (0.40, 0.70, 0.08)):
        t.elipse(cx, cy, rx, rx * 0.35, (0.97, 0.97, 0.95), 79.5)
        t.elipse(cx + 0.01, cy - 0.012, rx * 0.9, rx * 0.22, (0.84, 0.87, 0.93), 79.6)
    t.poli([(-0.1, 0.40), (0.05, 0.40), (0.07, 0.46), (0.18, 0.46), (0.20, 0.41), (0.46, 0.41), (0.48, 0.47),
            (0.55, 0.47), (0.57, 0.43), (0.80, 0.42), (0.80, 0.38), (-0.1, 0.38)], (0.74, 0.52, 0.46), 78.5)
    t.retangulo(-0.1, -0.1, 0.8, 0.42, (0.80, 0.58, 0.36), (0.94, 0.78, 0.56), 78)
    vp = (0.333, 0.42)
    t.poli([(0.0, -0.1), (0.667, -0.1), (vp[0] + 0.004, vp[1]), (vp[0] - 0.004, vp[1])],
           [(0.30, 0.29, 0.31), (0.30, 0.29, 0.31), (0.58, 0.56, 0.55), (0.58, 0.56, 0.55)], 77)
    for lado in (-1, 1):
        x0 = 0.333 + lado * 0.30
        t.poli([(x0 - 0.008, -0.1), (x0 + 0.008, -0.1), (vp[0] + lado * 0.0035, vp[1]), (vp[0] + lado * 0.0025, vp[1])], (0.93, 0.92, 0.88), 76)
    for k in range(9):
        a0, a1 = (k / 9) ** 1.6, ((k + 0.5) / 9) ** 1.6
        y0, y1 = -0.1 + a0 * 0.52, -0.1 + a1 * 0.52
        w0, w1 = 0.012 * (1 - a0), 0.012 * (1 - a1)
        t.poli([(0.333 - w0, y0), (0.333 + w0, y0), (0.333 + w1, y1), (0.333 - w1, y1)], (0.98, 0.82, 0.30), 75.5)
    # o escudo do 66, num poste
    t.retangulo(0.545, 0.10, 0.555, 0.36, (0.40, 0.40, 0.42), (0.55, 0.55, 0.56), 74)
    esc = [(0.50, 0.36), (0.60, 0.36), (0.605, 0.43), (0.59, 0.455), (0.55, 0.46), (0.51, 0.455), (0.495, 0.43)]
    t.poli([(0.55 + (x - 0.55) * 1.12, 0.405 + (y - 0.405) * 1.12) for x, y in esc], (0.10, 0.10, 0.10), 73.5)
    t.poli(esc, (0.97, 0.96, 0.92), 73)
    t.texto('66', FONTE_TITULO, (0.10, 0.10, 0.10), 0.55, 0.395, 0.07, 0.045, 72)
    # o cone na beira da estrada
    cx, cy = 0.10, 0.06
    t.poli([(cx - 0.07, cy), (cx + 0.07, cy), (cx + 0.07, cy + 0.015), (cx - 0.07, cy + 0.015)], (0.80, 0.34, 0.10), 71)
    t.poli([(cx - 0.045, cy + 0.015), (cx + 0.045, cy + 0.015), (cx + 0.010, cy + 0.20), (cx - 0.010, cy + 0.20)],
           [(0.80, 0.34, 0.10), (0.98, 0.50, 0.18), (0.98, 0.52, 0.20), (0.80, 0.36, 0.12)], 70.5)
    t.poli([(cx - 0.033, cy + 0.07), (cx + 0.033, cy + 0.07), (cx + 0.025, cy + 0.10), (cx - 0.025, cy + 0.10)], (0.96, 0.95, 0.92), 70)


def pintura_serra(t, rng):
    """A serra das rochas em forma de rabo de peixe, inclinadas, no fim da tarde"""
    t.ceu([(0.38, (0.99, 0.88, 0.68)), (0.62, (0.66, 0.84, 0.86)), (0.95, (0.30, 0.60, 0.78))], 80)
    t.poli([(-0.1, 0.36), (0.10, 0.44), (0.22, 0.42), (0.35, 0.47), (0.50, 0.43), (0.62, 0.46), (0.80, 0.42), (0.80, 0.33), (-0.1, 0.33)],
           (0.80, 0.62, 0.58), 79)
    t.retangulo(-0.1, -0.1, 0.8, 0.38, (0.70, 0.44, 0.26), (0.90, 0.66, 0.44), 78)
    for k, (x, alto, larg) in enumerate([(0.04, 0.60, 0.07), (0.15, 0.74, 0.08), (0.27, 0.82, 0.09),
                                         (0.40, 0.70, 0.085), (0.52, 0.62, 0.075), (0.62, 0.54, 0.06)]):
        incl = 0.10
        base = 0.34 - 0.012 * k
        topo = [(x + incl + larg * 0.55 + larg * 0.5 * math.cos(a), alto + larg * 0.45 * math.sin(a)) for a in np.linspace(0, math.pi, 8)]
        corpo = [(x, base), (x + larg * 1.2, base)] + topo
        d = 77 - k * 0.2
        t.poli(corpo, (0.84, 0.38, 0.20), d)
        sombra = [(x, base), (x + larg * 0.35, base), (x + incl + larg * 0.25, alto - 0.01), (x + incl + larg * 0.05, alto + 0.02)]
        t.poli(sombra, (0.56, 0.22, 0.14), d - 0.05)
        luz = [(x + larg * 1.05, base), (x + larg * 1.2, base), (x + incl + larg * 1.05, alto), (x + incl + larg * 0.92, alto + 0.01)]
        t.poli(luz, (0.97, 0.58, 0.32), d - 0.06)
    for k in range(18):                           # arbustos
        x, y = rng.uniform(-0.05, 0.72), rng.uniform(0.0, 0.30)
        s = 0.012 + 0.03 * (0.30 - y)
        t.elipse(x, y, s * 1.6, s, (0.36, 0.42, 0.26), 74 - y)
        t.elipse(x - s * 0.3, y + s * 0.3, s * 0.9, s * 0.6, (0.50, 0.56, 0.34), 73.9 - y)


def pintura_arco(t, rng):
    """O arco de rocha vermelha no fim da tarde: o sol baixo aparece pelo vão do arco, as mesas roxas lá longe
    e o chão de areia com moitas (no meio da tela: o quadro é um "A" estreito)"""
    t.ceu([(0.30, (1.0, 0.80, 0.50)), (0.42, (0.99, 0.62, 0.40)), (0.62, (0.82, 0.46, 0.50)), (0.90, (0.40, 0.28, 0.50))], 80)
    t.disco(0.33, 0.37, 0.07, (1.0, 0.93, 0.70), 79.5, cor_borda=(1.0, 0.72, 0.42))
    for k in range(3):                              # nuvens finas, acesas por baixo
        y = 0.58 + k * 0.07
        x0 = rng.uniform(0.0, 0.25)
        t.poli([(x0, y), (x0 + 0.42, y + 0.008), (x0 + 0.40, y + 0.022), (x0 + 0.03, y + 0.015)], (0.98, 0.64, 0.48), 79)
    t.poli([(-0.1, 0.30), (-0.1, 0.38), (0.05, 0.38), (0.07, 0.41), (0.16, 0.41), (0.18, 0.37), (0.48, 0.36), (0.50, 0.40),
            (0.60, 0.40), (0.62, 0.35), (0.80, 0.34), (0.80, 0.30)], (0.56, 0.30, 0.42), 78.5)
    t.retangulo(-0.1, -0.1, 0.8, 0.33, (0.62, 0.32, 0.18), (0.92, 0.60, 0.38), 78)
    # o arco: dois pés de rocha e o vão em cima, com o lado do sol aceso e o outro na sombra
    luz, meio, sombra = (0.93, 0.47, 0.25), (0.76, 0.32, 0.18), (0.48, 0.18, 0.13)
    fora = [(0.11, 0.18), (0.13, 0.36), (0.15, 0.48), (0.19, 0.57), (0.26, 0.625), (0.33, 0.64), (0.41, 0.625), (0.48, 0.57),
            (0.52, 0.48), (0.54, 0.36), (0.56, 0.18)]
    vao = [(0.47, 0.18), (0.46, 0.33), (0.43, 0.44), (0.38, 0.51), (0.33, 0.53), (0.28, 0.51), (0.23, 0.44), (0.20, 0.33), (0.19, 0.18)]
    d = 76.0
    for k in range(len(fora) - 1):                 # a rocha entre o contorno de fora e o vão, em fatias
        a0, a1 = fora[k], fora[k + 1]
        j0 = min(int(round(k * (len(vao) - 1) / (len(fora) - 1))), len(vao) - 1)
        j1 = min(int(round((k + 1) * (len(vao) - 1) / (len(fora) - 1))), len(vao) - 1)
        b0, b1 = vao[len(vao) - 1 - j0], vao[len(vao) - 1 - j1]
        cor = sombra if k < 3 else (meio if k < 6 else luz)
        t.poli([a0, a1, b1, b0], cor, d)
    for x0, x1 in ((0.11, 0.19), (0.47, 0.56)):    # os pés, até o chão
        t.poli([(x0 - 0.01, 0.10), (x1 + 0.01, 0.10), (x1, 0.19), (x0, 0.19)], meio, d - 0.1)
    t.poli([(0.53, 0.12), (0.565, 0.12), (0.555, 0.40), (0.53, 0.46)], luz, d - 0.2)          # a quina acesa
    # as camadas da rocha nos dois pés, de uma beira à outra de cada um
    esq_fora = [(y, x) for x, y in fora[:5]]
    esq_vao = sorted((y, x) for x, y in vao[5:])
    dir_fora = sorted((y, x) for x, y in fora[6:])
    dir_vao = [(y, x) for x, y in vao[:4]]

    def em(lado, y):
        ys, xs = zip(*lado)
        return float(np.interp(y, ys, xs))
    for y in (0.22, 0.30, 0.38):
        for a_, b_, cor in ((esq_fora, esq_vao, sombra), (dir_vao, dir_fora, meio)):
            x0, x1 = em(a_, y) + 0.004, em(b_, y) - 0.004
            t.poli([(x0, y), (x1, y + 0.006), (x1, y + 0.014), (x0, y + 0.008)], cor, d - 0.3)
    # a sombra comprida do arco na areia e as moitas
    t.poli([(0.10, 0.10), (0.57, 0.10), (0.70, 0.02), (-0.05, 0.02)], (0.50, 0.24, 0.16), 77)
    for k in range(16):
        x, y = rng.uniform(-0.05, 0.72), rng.uniform(-0.05, 0.27)
        r = 0.010 + 0.03 * (0.28 - y)
        t.elipse(x, y, r * 1.5, r, (0.34, 0.36, 0.20), 75 - y)
        t.elipse(x - r * 0.3, y + r * 0.3, r * 0.9, r * 0.55, (0.56, 0.52, 0.28), 74.9 - y)


def pintura_cozy_cone(t, rng):
    """O Cozy Cone à noite: os cones com as janelas acesas, as estrelas e a lua"""
    t.ceu([(0.30, (0.30, 0.18, 0.38)), (0.55, (0.12, 0.12, 0.32)), (0.95, (0.03, 0.05, 0.18))], 80)
    for k in range(50):
        x, y = rng.uniform(-0.05, 0.72), rng.uniform(0.52, 1.02)
        r = rng.uniform(0.002, 0.005)
        t.poli([(x - r, y), (x, y - r * 1.4), (x + r, y), (x, y + r * 1.4)], (0.98, 0.96, 0.82), 79)
    t.disco(0.50, 0.84, 0.05, (0.96, 0.93, 0.78), 79)
    t.disco(0.52, 0.85, 0.045, (0.08, 0.09, 0.26), 78.9)
    t.retangulo(-0.1, -0.1, 0.8, 0.26, (0.10, 0.07, 0.14), (0.22, 0.14, 0.24), 78)

    def cone(cx, base, larg, alto, d):
        meio_luz = (0.90, 0.45, 0.20)
        meio_sombra = (0.52, 0.22, 0.14)
        t.poli([(cx - larg / 2, base), (cx, base), (cx, base + alto), (cx - larg * 0.08, base + alto)], meio_sombra, d)
        t.poli([(cx, base), (cx + larg / 2, base), (cx + larg * 0.08, base + alto), (cx, base + alto)], meio_luz, d)
        t.poli([(cx - larg * 0.6, base - 0.012), (cx + larg * 0.6, base - 0.012), (cx + larg * 0.6, base + 0.01), (cx - larg * 0.6, base + 0.01)],
               (0.40, 0.18, 0.12), d - 0.1)
        # a porta acesa, o toldo listrado e as janelas
        t.poli([(cx - larg * 0.17, base), (cx + larg * 0.17, base), (cx + larg * 0.17, base + alto * 0.25), (cx - larg * 0.17, base + alto * 0.25)],
               [(1.0, 0.70, 0.36), (1.0, 0.70, 0.36), (1.0, 0.86, 0.55), (1.0, 0.86, 0.55)], d - 0.2)
        for k in range(5):
            x0 = cx - larg * 0.22 + k * larg * 0.088
            t.poli([(x0, base + alto * 0.27), (x0 + larg * 0.088, base + alto * 0.27), (x0 + larg * 0.088, base + alto * 0.32), (x0, base + alto * 0.32)],
                   (0.95, 0.90, 0.80) if k % 2 else (0.70, 0.30, 0.20), d - 0.25)
        for lado in (-1, 1):
            t.disco(cx + lado * larg * 0.27, base + alto * 0.30, larg * 0.065, (1.0, 0.86, 0.50), d - 0.2)
    cone(0.08, 0.20, 0.20, 0.36, 77)
    cone(0.60, 0.21, 0.18, 0.32, 77)
    cone(0.33, 0.12, 0.30, 0.52, 76)
    for x in (0.02, 0.64):                        # as lampadinhas de cone no chão
        t.poli([(x - 0.01, 0.10), (x + 0.01, 0.10), (x + 0.003, 0.13), (x - 0.003, 0.13)], (1.0, 0.80, 0.40), 75)
    t.retangulo(0.18, 0.69, 0.48, 0.755, (0.10, 0.06, 0.16), (0.10, 0.06, 0.16), 75)
    t.texto('COZY CONE', FONTE_TITULO, (1.0, 0.50, 0.78), 0.33, 0.722, 0.28, 0.05, 74.5)
    t.texto('MOTEL', FONTE_TEXTO, (0.45, 0.95, 0.90), 0.33, 0.665, 0.10, 0.025, 74.5)


def pintura_cachoeira(t, rng):
    """A cachoeira do Radiador: o cânion vermelho, a queda d'água e o rio"""
    t.ceu([(0.45, (0.94, 0.92, 0.84)), (0.75, (0.62, 0.78, 0.90)), (0.98, (0.40, 0.62, 0.86))], 80)
    t.poli([(-0.1, 0.30), (-0.1, 0.92), (0.06, 0.90), (0.12, 0.80), (0.20, 0.78), (0.26, 0.70), (0.27, 0.30)], (0.78, 0.36, 0.20), 77)
    t.poli([(0.80, 0.30), (0.80, 0.88), (0.62, 0.86), (0.55, 0.78), (0.46, 0.76), (0.41, 0.70), (0.40, 0.30)], (0.72, 0.32, 0.18), 77)
    for k in range(6):                            # as camadas da rocha
        y = 0.38 + k * 0.08
        t.poli([(-0.1, y), (0.27, y + 0.004), (0.27, y + 0.016), (-0.1, y + 0.012)], (0.92, 0.52, 0.30), 76.8)
        t.poli([(0.40, y + 0.02), (0.80, y + 0.01), (0.80, y + 0.022), (0.40, y + 0.034)], (0.56, 0.24, 0.14), 76.8)
    t.poli([(0.26, 0.70), (0.41, 0.70), (0.405, 0.30), (0.265, 0.30)], [(0.80, 0.90, 0.96)] * 2 + [(0.92, 0.97, 1.0)] * 2, 76.5)
    for k in range(12):
        x = 0.275 + k * 0.011
        t.poli([(x, 0.70), (x + 0.004, 0.70), (x + 0.005 + 0.002 * math.sin(k), 0.30), (x + 0.001, 0.30)], (0.62, 0.80, 0.92), 76.4)
    t.retangulo(-0.1, -0.1, 0.8, 0.31, (0.20, 0.42, 0.52), (0.40, 0.66, 0.78), 76)
    t.elipse(0.335, 0.30, 0.14, 0.03, (0.96, 0.98, 1.0), 75.5)
    for k in range(16):                           # as árvores e as moitas
        lado = -1 if k % 2 else 1
        x = 0.335 + lado * rng.uniform(0.18, 0.42)
        y = rng.uniform(0.02, 0.36)
        s = rng.uniform(0.03, 0.06)
        t.elipse(x, y, s, s * 1.3, (0.16, 0.34, 0.18), 74 - y)
        t.elipse(x - s * 0.3, y + s * 0.4, s * 0.6, s * 0.7, (0.30, 0.52, 0.26), 73.9 - y)
    t.poli([(0.22, -0.1), (0.46, -0.1), (0.37, 0.30), (0.30, 0.30)], [(0.28, 0.52, 0.62)] * 2 + [(0.50, 0.74, 0.84)] * 2, 75)


def pintura_cacto(t, rng):
    """O cacto e a lua no fim do dia, com as mesas roxas no horizonte"""
    t.ceu([(0.28, (0.99, 0.66, 0.42)), (0.45, (0.90, 0.46, 0.48)), (0.72, (0.48, 0.30, 0.52)), (0.95, (0.20, 0.16, 0.40))], 80)
    t.disco(0.40, 0.62, 0.115, (0.99, 0.95, 0.82), 79.5)
    for cx, cy, r in ((0.37, 0.65, 0.02), (0.43, 0.58, 0.015), (0.44, 0.67, 0.01)):
        t.disco(cx, cy, r, (0.90, 0.86, 0.74), 79.4)
    t.poli([(-0.1, 0.30), (-0.1, 0.40), (0.06, 0.40), (0.08, 0.44), (0.20, 0.44), (0.22, 0.38), (0.48, 0.37), (0.50, 0.42),
            (0.60, 0.42), (0.62, 0.36), (0.80, 0.35), (0.80, 0.30)], (0.44, 0.22, 0.38), 79)
    t.retangulo(-0.1, -0.1, 0.8, 0.33, (0.20, 0.10, 0.16), (0.40, 0.20, 0.28), 78)
    verde = (0.08, 0.14, 0.11)

    def cacto(x, y, s, d):
        r = 0.022 * s
        corpo = [(x - r, y), (x + r, y), (x + r, y + 0.42 * s)] + [(x + r * math.cos(a), y + 0.42 * s + r * math.sin(a)) for a in np.linspace(0, math.pi, 7)[1:-1]] + [(x - r, y + 0.42 * s)]
        t.poli(corpo, verde, d)
        for lado, y0, y1 in ((-1, 0.16, 0.32), (1, 0.22, 0.36)):
            xa = x + lado * r
            xb = x + lado * (r + 0.05 * s)
            braco = [(xa, y + y0 * s), (xa, y + (y0 + 0.04) * s), (xb - lado * 0.018 * s, y + (y0 + 0.04) * s),
                     (xb - lado * 0.018 * s, y + y1 * s), (xb + lado * 0.018 * s, y + y1 * s), (xb + lado * 0.018 * s, y + y0 * s)]
            t.poli(braco, verde, d)
    cacto(0.30, 0.08, 1.15, 76)
    cacto(0.60, 0.24, 0.45, 77)
    cacto(0.06, 0.20, 0.55, 77)
    for k in range(10):
        x, y = rng.uniform(-0.05, 0.72), rng.uniform(0.0, 0.22)
        t.elipse(x, y, 0.03, 0.012, (0.14, 0.08, 0.12), 75)


PINTURAS = [pintura_mesas, pintura_rota_66, pintura_serra, pintura_arco, pintura_cozy_cone, pintura_cachoeira,
            pintura_cacto, None]      # a última é a Sally na estrada (com o carro do jogo), feita à parte


def textura_do_relogio(n=128):
    """O mostrador: creme, com os 12 traços, os ponteiros em 10 e 10 e a borda escura"""
    yy, xx = np.mgrid[0:n, 0:n].astype(np.float32)
    c = (n - 1) / 2
    x, y = (xx - c) / c, (c - yy) / c
    r = np.hypot(x, y)
    a = np.arctan2(x, y)            # 0 em cima, horário
    img = np.ones((n, n, 3), np.float32) * np.array([0.96, 0.92, 0.80])
    img[r > 0.86] = (0.25, 0.18, 0.12)
    for k in range(12):
        ak = k * math.pi / 6
        d = np.abs(np.angle(np.exp(1j * (a - ak))))
        tick = (d < 0.05) & (r > 0.66) & (r < 0.80)
        img[tick] = (0.20, 0.15, 0.12)

    def ponteiro(ang, comp, larg):
        px, py = math.sin(ang), math.cos(ang)
        t = x * px + y * py
        dist = np.abs(x * py - y * px)
        return (t > -0.08) & (t < comp) & (dist < larg)
    img[ponteiro(math.radians(305), 0.45, 0.05)] = (0.15, 0.10, 0.08)
    img[ponteiro(math.radians(60), 0.62, 0.035)] = (0.15, 0.10, 0.08)
    img[r < 0.07] = (0.80, 0.30, 0.10)
    return img


def renderizar_quadros():
    """Pinta os quadros, o pôster e o relógio e devolve o atlas (1024 x 2048 x 3, linha 0 em cima)"""
    sc = bpy.context.scene
    sc.render.engine = 'BLENDER_EEVEE'
    sc.view_settings.view_transform = 'Standard'
    sc.view_settings.look = 'None'
    sc.eevee.taa_render_samples = 24
    sc.render.film_transparent = False
    w = novo_mundo('ceu_do_quadro')
    sc.world = w
    fundo_mundo = w.node_tree.nodes['Background']
    fundo_mundo.inputs[1].default_value = 0.7
    cam = bpy.data.cameras.new('cam_quadro')
    cam.lens = 50
    cam.sensor_fit = 'VERTICAL'
    cam.sensor_height = 24
    cam.clip_start = 0.5
    cam.clip_end = 500
    co = bpy.data.objects.new('cam_quadro', cam)
    sc.collection.objects.link(co)
    sc.camera = co
    co.location = (0, 0, 0)
    co.rotation_euler = (math.pi / 2, 0, 0)
    sol1 = bpy.data.objects.new('sol1', bpy.data.lights.new('sol1', 'SUN'))
    sol1.data.energy = 3.2
    sol1.data.color = (1.0, 0.92, 0.80)
    sol1.rotation_euler = (math.radians(55), math.radians(-25), math.radians(-35))
    sol2 = bpy.data.objects.new('sol2', bpy.data.lights.new('sol2', 'SUN'))
    sol2.data.energy = 1.2
    sol2.data.color = (0.85, 0.9, 1.0)
    sol2.rotation_euler = (math.radians(-60), math.radians(20), math.radians(150))
    for s in (sol1, sol2):
        sc.collection.objects.link(s)
    contorno = novo_material('contorno')
    contorno.use_backface_culling = True
    nt = contorno.node_tree
    nt.nodes.clear()
    em = nt.nodes.new('ShaderNodeEmission')
    em.inputs['Color'].default_value = (0.05, 0.035, 0.03, 1)
    nt.links.new(em.outputs[0], nt.nodes.new('ShaderNodeOutputMaterial').inputs['Surface'])
    mat_fundo = material_emissao('fundo_quadro', 'vertices')
    mat_texto = material_emissao('texto_quadro', 'objeto')
    fontes = {}
    camada = bpy.data.collections.new('camada_do_quadro')
    sc.collection.children.link(camada)
    W, H = TAM_DO_ATLAS
    atlas = np.zeros((H, W, 3), np.float32)

    def limpar_camada():
        dados = {o.data for o in camada.objects if o.data is not None}
        for o in list(camada.objects):
            bpy.data.objects.remove(o, do_unlink=True)
        for d in dados:
            if d.users == 0:
                if isinstance(d, bpy.types.Mesh):
                    bpy.data.meshes.remove(d)
                elif isinstance(d, bpy.types.Curve):
                    bpy.data.curves.remove(d)

    def render(w_px, h_px):
        sc.render.resolution_x = w_px
        sc.render.resolution_y = h_px
        sc.render.resolution_percentage = 100
        caminho = os.path.join(PASTA_TEMP, 'quadro_render.png')
        sc.render.filepath = caminho
        sc.render.image_settings.file_format = 'PNG'
        bpy.ops.render.render(write_still=True)
        return ler_png(caminho)[..., :3]

    def tinta_em_tela(img, rng):
        """Cara de pintura: pinceladas (ruído alongado em direções que mudam devagar), a trama da tela,
        um pouco menos de tons e o verniz amarelado nas bordas"""
        h, w = img.shape[:2]
        p1 = ruido(h, w, 0.8, 3.5, rng)
        p2 = ruido(h, w, 3.5, 0.8, rng)
        mistura = 0.5 + 0.5 * ruido(h, w, 30, 30, rng)
        mistura = np.clip(mistura, 0, 1)
        pincel = p1 * mistura + p2 * (1 - mistura)
        yy, xx = np.mgrid[0:h, 0:w]
        trama = 0.5 * np.sin(xx * 2.1) * np.sin(yy * 2.1)
        img = img * (1 + 0.035 * pincel[..., None] + 0.012 * trama[..., None])
        img = np.round(img * 24) / 24 * 0.3 + img * 0.7
        d = np.maximum(np.abs(xx / w - 0.5), np.abs(yy / h - 0.5)) * 2
        img = img * (1 - 0.10 * np.clip((d - 0.7) / 0.3, 0, 1) ** 2)[..., None]
        img = img * 0.94 + np.array([0.96, 0.90, 0.76]) * 0.06
        return np.clip(img, 0, 1)

    def por_na_celula(k, img):
        x, y, w, h = CELULAS[k]
        b = BORDA_DA_CELULA
        img = redimensionar(img, w - 2 * b, h - 2 * b)
        atlas[y:y + h, x:x + w] = np.pad(img, ((b, b), (b, b), (0, 0)), mode='edge')

    w_px, h_px = int((320 - 8) * 2.6), int((480 - 8) * 2.6)
    mh = 12 / cam.lens
    A = w_px / h_px
    tela_ = Tela(camada, mh, A, mat_fundo, fontes, mat_texto)
    # os carros (escondidos até a vez deles)
    carros = {}
    for chave in ('sally', 'mcqueen'):
        raiz, objs, esc, _ = importar_carro(chave)
        pintar_de_quadro(objs, contorno)
        carros[chave] = (raiz, objs)
        raiz.hide_render = True
        for o in objs:
            o.hide_render = True

    def mostrar_carro(chave, giro, alto, rola, x, y, d):
        for k, (raiz, objs) in carros.items():
            for o in [raiz] + objs:
                o.hide_render = k != chave
        raiz = carros[chave][0]
        p = tela_.ponto(x, y, d)
        raiz.location = (p[0], p[1], p[2] - 0.6)
        raiz.rotation_euler = (math.radians(alto), math.radians(rola), math.radians(giro))

    def esconder_carros():
        for raiz, objs in carros.values():
            for o in [raiz] + objs:
                o.hide_render = True

    for k, f in enumerate(PINTURAS):
        limpar_camada()
        esconder_carros()
        rng = np.random.default_rng(40 + k)
        if f is not None:
            f(tela_, rng)
        else:
            # a Sally na estrada do deserto, à tarde
            tela_.ceu([(0.40, (0.99, 0.86, 0.62)), (0.70, (0.70, 0.82, 0.90)), (0.95, (0.45, 0.66, 0.86))], 80)
            tela_.poli([(-0.1, 0.42), (0.08, 0.42), (0.10, 0.52), (0.26, 0.52), (0.28, 0.43), (0.60, 0.43), (0.62, 0.49),
                        (0.72, 0.49), (0.80, 0.44), (0.80, 0.38), (-0.1, 0.38)], (0.84, 0.46, 0.30), 79)
            tela_.poli([(0.10, 0.52), (0.26, 0.52), (0.26, 0.505), (0.10, 0.505)], (0.98, 0.62, 0.40), 78.9)
            tela_.retangulo(-0.1, -0.1, 0.8, 0.43, (0.82, 0.56, 0.34), (0.96, 0.78, 0.54), 78)
            tela_.poli([(-0.1, -0.1), (0.40, -0.1), (0.36, 0.43), (0.345, 0.43)], [(0.36, 0.34, 0.36)] * 2 + [(0.60, 0.56, 0.54)] * 2, 77)
            for j in range(6):
                a0, a1 = (j / 6) ** 1.5, ((j + 0.45) / 6) ** 1.5
                y0, y1 = -0.1 + a0 * 0.53, -0.1 + a1 * 0.53
                def mx(y):
                    return 0.15 + (0.352 - 0.15) * (y + 0.1) / 0.53
                t0, t1 = 0.008 * (1 - a0), 0.008 * (1 - a1)
                tela_.poli([(mx(y0) - t0, y0), (mx(y0) + t0, y0), (mx(y1) + t1, y1), (mx(y1) - t1, y1)], (0.98, 0.84, 0.36), 76)
            mostrar_carro('sally', -36, 9, 4, 0.36, 0.20, 19.0)
        fundo_mundo.inputs[0].default_value = (0.8, 0.7, 0.6, 1)
        img = render(w_px, h_px)
        por_na_celula(k, tinta_em_tela(img, np.random.default_rng(100 + k)))
        print('  quadro %d' % (k + 1))
    # o pôster do Relâmpago McQueen 95 (do cone dele)
    limpar_camada()
    esconder_carros()
    t = tela_
    t.retangulo(-0.1, -0.1, 0.8, 1.1, (0.62, 0.05, 0.05), (0.84, 0.10, 0.08), 80)
    meio = (0.333, 0.40)
    for j in range(24):
        if j % 2:
            continue
        a0, a1 = 2 * math.pi * j / 24, 2 * math.pi * (j + 1) / 24
        t.poli([meio, (meio[0] + 2 * math.cos(a0), meio[1] + 2 * math.sin(a0)), (meio[0] + 2 * math.cos(a1), meio[1] + 2 * math.sin(a1))],
               (0.92, 0.22, 0.10), 79)
    raio = [(0.10, 0.98), (0.27, 0.98), (0.21, 0.86), (0.32, 0.86), (0.07, 0.69), (0.14, 0.81), (0.04, 0.81)]
    t.poli([(x + 0.012, y - 0.012) for x, y in raio], (0.35, 0.03, 0.03), 78.5)
    t.poli(raio, (0.99, 0.80, 0.18), 78)
    t.texto('95', FONTE_TITULO, (0.99, 0.85, 0.20), 0.49, 0.865, 0.28, 0.15, 77, sombra=(0.30, 0.02, 0.02))
    t.texto('LIGHTNING', FONTE_TITULO, (0.98, 0.96, 0.90), 0.333, 0.665, 0.56, 0.07, 77, sombra=(0.30, 0.02, 0.02))
    t.texto('McQUEEN', FONTE_TITULO, (0.99, 0.82, 0.20), 0.333, 0.585, 0.56, 0.085, 77, sombra=(0.30, 0.02, 0.02))
    t.retangulo(-0.1, -0.1, 0.8, 0.13, (0.10, 0.08, 0.08), (0.16, 0.12, 0.12), 76)
    t.texto('COPA PISTÃO', FONTE_TEXTO, (0.99, 0.85, 0.20), 0.333, 0.065, 0.50, 0.06, 75)
    mostrar_carro('mcqueen', 32, 12, -4, 0.333, 0.30, 16.0)
    fundo_mundo.inputs[0].default_value = (0.8, 0.3, 0.2, 1)
    img = render(w_px, h_px)
    rng = np.random.default_rng(200)
    h, w = img.shape[:2]
    img = img * (1 + 0.02 * ruido(h, w, 1.5, 1.5, rng)[..., None])
    img = img * 0.95 + np.array([0.96, 0.92, 0.80]) * 0.05
    por_na_celula(CELULA_DO_POSTER, np.clip(img, 0, 1))
    print('  pôster do McQueen')
    limpar_camada()
    esconder_carros()
    x, y, w_, h_ = CELULAS[CELULA_DO_RELOGIO]
    atlas[y:y + h_, x:x + w_] = textura_do_relogio(h_)
    return np.round(atlas * 63) / 63        # 64 tons por canal: o PNG encolhe e não se vê diferença


def limpar_cena(guardar=()):
    for o in list(bpy.data.objects):
        bpy.data.objects.remove(o, do_unlink=True)
    for colecao in (bpy.data.meshes, bpy.data.curves, bpy.data.cameras, bpy.data.lights):
        for d in list(colecao):
            colecao.remove(d)
    for m in list(bpy.data.materials):
        bpy.data.materials.remove(m)
    for c in list(bpy.data.collections):
        bpy.data.collections.remove(c)
    for im in list(bpy.data.images):
        if im.name not in guardar:
            bpy.data.images.remove(im)


def imagem_do_glb(caminho, nome):
    """A imagem 'nome' embutida num .glb (o PNG gravado de volta num arquivo da pasta temporária), ou None"""
    d = open(caminho, 'rb').read()
    n = struct.unpack('<I', d[12:16])[0]
    j = json.loads(d[20:20 + n])
    for im in j.get('images', []):
        if im.get('name') == nome and 'bufferView' in im:
            bv = j['bufferViews'][im['bufferView']]
            ini = 20 + n + 8 + bv.get('byteOffset', 0)
            arquivo = os.path.join(PASTA_TEMP, 'do_glb_' + nome + '.png')
            with open(arquivo, 'wb') as f:
                f.write(d[ini:ini + bv['byteLength']])
            return arquivo
    return None


print('Pintando os quadros...')
if NOITE and not ATLAS_GUARDADO:
    # de noite, o atlas é o do .glb de dia (a mesma textura; e poupa o passo mais demorado)
    ATLAS_GUARDADO = imagem_do_glb(SAIDA + '.glb', 'cozy_quadros')
if ATLAS_GUARDADO and os.path.exists(ATLAS_GUARDADO):
    ATLAS = ler_png(ATLAS_GUARDADO)[..., :3]
    print('  reaproveitado de', ATLAS_GUARDADO)
else:
    ATLAS = renderizar_quadros()
    if ATLAS_GUARDADO:
        salvar_png(ATLAS, ATLAS_GUARDADO)
IMG_QUADROS = imagem('cozy_quadros', ATLAS)
limpar_cena(guardar={IMG_PAPEL.name, IMG_CARPETE.name, IMG_CAPACHO.name, IMG_QUADROS.name})
print('  %.0f s' % (time.time() - INICIO))

# ------------------------------------------------------------------
# 3. Os quartos
# ------------------------------------------------------------------
TAM_MAX_DE_CANTOS = 56000      # cantos por malha (a raylib usa índices de 16 bits: menos de 65536 vértices)


# As superfícies grandes e lisas (a luz delas é alisada entre os vizinhos da mesma superfície) e as marcas de
# algumas faces (as que levam uma cor depois do assado)
SUP_CONE, SUP_FRENTE, SUP_PISO, SUP_TETO, SUP_TAPETE = 1, 2, 3, 4, 5
MARCA_MOLDURA, MARCA_FORRO, MARCA_PELE = 1, 2, 3
MARCA_ATUAL = [0]


class Malha:
    """As faces de um material (em bmesh), com as coordenadas de textura, a cor sem luz (tinta, linear),
    o brilho (as lâmpadas), a superfície grande e lisa de que a face faz parte (0 se nenhuma), se a face é
    de metal (ganha o brilho falso), o quarto e a marca (MARCA_ATUAL). Quando uma enche, começa outra."""

    def __init__(self, nome, imagem=None):
        self.nome, self.imagem = nome, imagem
        self.bms = []
        self.novo()

    def novo(self):
        bm = bmesh.new()
        self.l_uv = bm.loops.layers.uv.new('UVMap')
        self.l_tinta = bm.loops.layers.float_color.new('tinta')
        self.l_brilho = bm.loops.layers.float_color.new('brilho')
        self.l_sup = bm.faces.layers.int.new('superficie')
        self.l_metal = bm.faces.layers.int.new('metal')
        self.l_quarto = bm.faces.layers.int.new('quarto')
        self.l_marca = bm.faces.layers.int.new('marca')
        self.bm = bm
        self.cantos = 0
        self.bms.append(bm)

    def face(self, pts, cor, n=None, uvs=None, brilho=None, sup=0, metal=False):
        pts = [tuple(map(float, p)) for p in pts]
        limpos, idx = [], []
        for k, p in enumerate(pts):
            if limpos and np.linalg.norm(np.subtract(p, limpos[-1])) < 1e-5:
                continue
            limpos.append(p)
            idx.append(k)
        if len(limpos) > 2 and np.linalg.norm(np.subtract(limpos[0], limpos[-1])) < 1e-5:
            limpos.pop()
            idx.pop()
        if len(limpos) < 3:
            return
        nn = normal_de(limpos)
        if np.linalg.norm(nn) < 1e-9:
            return
        if callable(cor):
            cores = [cor(p) for p in limpos]
        elif isinstance(cor, list):
            cores = [cor[k] for k in idx]
        else:
            cores = [cor] * len(limpos)
        uvl = [uvs[k] for k in idx] if uvs is not None else [(0.0, 0.0)] * len(limpos)
        if n is not None and np.dot(nn, n) < 0:
            limpos, cores, uvl = limpos[::-1], cores[::-1], uvl[::-1]
        if self.cantos + len(limpos) > TAM_MAX_DE_CANTOS:
            self.novo()
        vs = [self.bm.verts.new((p[0], -p[2], p[1])) for p in limpos]
        try:
            f = self.bm.faces.new(vs)
        except ValueError:
            return
        self.cantos += len(limpos)
        f[self.l_sup] = sup
        f[self.l_metal] = 1 if metal else 0
        f[self.l_quarto] = QUARTO_ATUAL[0]
        f[self.l_marca] = MARCA_ATUAL[0]
        b = (*linear(brilho), 1.0) if brilho is not None else (0.0, 0.0, 0.0, 1.0)
        for k, l in enumerate(f.loops):
            l[self.l_tinta] = (*linear(cores[k]), 1.0)
            l[self.l_uv].uv = uvl[k]
            l[self.l_brilho] = b


def normal_de(pts):
    n = np.zeros(3)
    m = len(pts)
    for k in range(m):
        a, b = np.array(pts[k]), np.array(pts[(k + 1) % m])
        n += np.cross(a, b)
    return n


QUARTO_ATUAL = [0]
MALHAS = {
    'cores': Malha('cozy_cores'),
    'parede': Malha('cozy_parede', IMG_PAPEL),
    'carpete': Malha('cozy_carpete', IMG_CARPETE),
    'capacho': Malha('cozy_capacho', IMG_CAPACHO),
    'quadros': Malha('cozy_quadros', IMG_QUADROS),
    'forro': Malha('cozy_cores'),    # sem luz assada (o forro da porta): o mesmo material das cores
}
NAO_ASSADAS = ('forro', 'pele')     # a pele do cone 4 (com a textura da casca) entra em MALHAS depois de ler o mapa
CORES = MALHAS['cores']


def quad(m, a, b, c, d, cor, n=None, uv=None, uvf=None, passo=PASSO, brilho=None, div=None, sup=0, metal=False):
    """Um quadrilátero (a, b, c, d em volta), subdividido numa grade de no máximo 'passo' metros.
    uv: as coordenadas de textura dos quatro cantos; uvf: uma função do ponto"""
    a, b, c, d = (np.asarray(x, float) for x in (a, b, c, d))
    if div:
        nu, nv = div
    else:
        nu = max(1, math.ceil(max(np.linalg.norm(b - a), np.linalg.norm(c - d)) / passo - 1e-6))
        nv = max(1, math.ceil(max(np.linalg.norm(d - a), np.linalg.norm(c - b)) / passo - 1e-6))

    def P(s, t):
        return (1 - t) * ((1 - s) * a + s * b) + t * ((1 - s) * d + s * c)

    def UV(s, t):
        u = [np.asarray(x, float) for x in uv]
        return tuple((1 - t) * ((1 - s) * u[0] + s * u[1]) + t * ((1 - s) * u[3] + s * u[2]))
    for i in range(nu):
        for j in range(nv):
            st = [(i / nu, j / nv), ((i + 1) / nu, j / nv), ((i + 1) / nu, (j + 1) / nv), (i / nu, (j + 1) / nv)]
            pts = [P(s, t) for s, t in st]
            if uv is not None:
                uvs = [UV(s, t) for s, t in st]
            elif uvf is not None:
                uvs = [uvf(p) for p in pts]
            else:
                uvs = None
            m.face(pts, cor, n=n, uvs=uvs, brilho=brilho, sup=sup, metal=metal)


class Ref:
    """Um referencial no mundo: a origem o e os eixos ex (à direita de quem olha), ey (para cima) e ez
    (para quem olha); p(a, h, b) = o + a·ex + h·ey + b·ez"""

    def __init__(self, o, ex, ey, ez):
        self.o = np.asarray(o, float)
        self.ex, self.ey, self.ez = (np.asarray(x, float) for x in (ex, ey, ez))

    def p(self, a, h, b):
        return self.o + a * self.ex + h * self.ey + b * self.ez

    def d(self, da, dh, db):
        return da * self.ex + dh * self.ey + db * self.ez


def ref_de(o, ez, ey=(0.0, 1.0, 0.0)):
    """O referencial com a frente ez, o alto ey e a direita de quem olha a frente (ex = ey × ez, no mundo:
    nada sai espelhado, nem nos cones espelhados)"""
    ez = unit(ez)
    ey = np.asarray(ey, float)
    ey = unit(ey - ez * np.dot(ey, ez))
    return Ref(o, np.cross(ey, ez), ey, ez)


def caixa(m, R, a0, a1, h0, h1, b0, b1, cor, sem=(), passo=PASSO, uvf=None, cores=None, brilho=None, metal=False):
    """Uma caixa no referencial R. 'sem' tira faces ('frente', 'tras', 'esq', 'dir', 'cima', 'baixo');
    'cores' dá uma cor por face"""
    P = R.p
    faces = {
        'frente': ([P(a0, h0, b1), P(a1, h0, b1), P(a1, h1, b1), P(a0, h1, b1)], R.d(0, 0, 1)),
        'tras': ([P(a0, h0, b0), P(a1, h0, b0), P(a1, h1, b0), P(a0, h1, b0)], R.d(0, 0, -1)),
        'esq': ([P(a0, h0, b0), P(a0, h0, b1), P(a0, h1, b1), P(a0, h1, b0)], R.d(-1, 0, 0)),
        'dir': ([P(a1, h0, b0), P(a1, h0, b1), P(a1, h1, b1), P(a1, h1, b0)], R.d(1, 0, 0)),
        'cima': ([P(a0, h1, b0), P(a1, h1, b0), P(a1, h1, b1), P(a0, h1, b1)], R.d(0, 1, 0)),
        'baixo': ([P(a0, h0, b0), P(a1, h0, b0), P(a1, h0, b1), P(a0, h0, b1)], R.d(0, -1, 0)),
    }
    for nome, (pts, n) in faces.items():
        if nome in sem:
            continue
        c = cores.get(nome, cor) if cores else cor
        quad(m, *pts, c, n=n, passo=passo, uvf=uvf, brilho=brilho, metal=metal)


def barra(m, p, q, largura, prof, lado, cor, metal=False):
    """Uma barra de p a q (pontos do mundo), com 'largura' na direção 'lado' e 'prof' na outra"""
    p, q = np.asarray(p, float), np.asarray(q, float)
    eixo = unit(q - p)
    lado = np.asarray(lado, float)
    lado = unit(lado - eixo * np.dot(lado, eixo))
    frente = np.cross(eixo, lado)
    hw, hp = largura / 2, prof / 2
    cantos = [(-1, -1), (1, -1), (1, 1), (-1, 1)]
    for k in range(4):
        s0, t0 = cantos[k]
        s1, t1 = cantos[(k + 1) % 4]
        o0 = lado * s0 * hw + frente * t0 * hp
        o1 = lado * s1 * hw + frente * t1 * hp
        quad(m, p + o0, p + o1, q + o1, q + o0, cor, n=unit(o0 + o1), div=(1, 1), metal=metal)


def torno(m, centro, eixo, e1, perfil, n_seg, cor, repete=4, ang=(0.0, 2 * math.pi), brilho=None, fechar=False,
          metal=False, tipos=None):
    """Gira o perfil [(raio, altura ao longo do eixo)] em volta do eixo. A face fica para o lado em que o
    perfil, andando, tem a sua direita (perfil anti-horário em (r, h) => para fora)."""
    centro = np.asarray(centro, float)
    eixo = unit(eixo)
    e1 = unit(np.asarray(e1, float) - eixo * np.dot(e1, eixo))
    e2 = np.cross(eixo, e1)
    angs = np.linspace(ang[0], ang[1], n_seg + 1)
    m_pts = len(perfil)
    segs = range(m_pts if fechar else m_pts - 1)
    for k in segs:
        r0, h0 = perfil[k]
        r1, h1 = perfil[(k + 1) % m_pts]
        dr, dh = r1 - r0, h1 - h0
        nr, nh = dh, -dr
        v0, v1 = tipos[k] if tipos else (0.0, 0.0)
        for j in range(n_seg):
            a0, a1 = angs[j], angs[j + 1]
            rad0 = math.cos(a0) * e1 + math.sin(a0) * e2
            rad1 = math.cos(a1) * e1 + math.sin(a1) * e2
            radm = unit(rad0 + rad1)
            pts = [centro + eixo * h0 + rad0 * r0, centro + eixo * h0 + rad1 * r0,
                   centro + eixo * h1 + rad1 * r1, centro + eixo * h1 + rad0 * r1]
            n = radm * nr + eixo * nh
            c = cor[k] if isinstance(cor, list) else cor
            u0, u1 = a0 / (2 * math.pi) * repete, a1 / (2 * math.pi) * repete
            m.face(pts, c, n=n, uvs=[(u0, v0), (u1, v0), (u1, v1), (u0, v1)], brilho=brilho, metal=metal)


def disco(m, centro, eixo, e1, raio, n_seg, cor, brilho=None, raio_dentro=0.0, metal=False):
    """Um disco (ou anel) virado para 'eixo'"""
    torno(m, centro, eixo, e1, [(raio, 0.0), (raio_dentro, 0.0)], n_seg, cor, brilho=brilho, metal=metal)


# --- textos (objetos de texto do Blender, convertidos em malha) ---
_FONTES = {}
_TEXTOS = {}


def malha_de_texto(txt, fonte, resolucao=1):
    chave = (txt, fonte, resolucao)
    if chave in _TEXTOS:
        return _TEXTOS[chave]
    if fonte not in _FONTES:
        _FONTES[fonte] = bpy.data.fonts.load(fonte, check_existing=True)
    cu = bpy.data.curves.new('texto', 'FONT')
    cu.body = txt
    cu.font = _FONTES[fonte]
    cu.size = 1.0
    cu.resolution_u = resolucao
    cu.fill_mode = 'BOTH'
    ob = bpy.data.objects.new('texto', cu)
    bpy.context.scene.collection.objects.link(ob)
    dg = bpy.context.evaluated_depsgraph_get()
    me = bpy.data.meshes.new_from_object(ob.evaluated_get(dg))
    pts = np.array([v.co[:2] for v in me.vertices])
    faces = [list(p.vertices) for p in me.polygons]
    bpy.data.objects.remove(ob, do_unlink=True)
    bpy.data.curves.remove(cu)
    bpy.data.meshes.remove(me)
    _TEXTOS[chave] = (pts, faces)
    return pts, faces


def texto(m, txt, fonte, centro, direita, cima, largura, altura, cor, brilho=None, resolucao=1):
    """Escreve txt centrado em 'centro', cabendo em largura x altura (metros), no plano (direita, cima);
    a face fica virada para direita x cima"""
    pts, faces = malha_de_texto(txt, fonte, resolucao)
    if len(pts) == 0:
        return
    mn, mx = pts.min(0), pts.max(0)
    w, h = mx - mn
    k = min(largura / max(w, 1e-6), altura / max(h, 1e-6))
    meio = (mn + mx) / 2
    centro = np.asarray(centro, float)
    direita, cima = unit(direita), unit(cima)
    n = np.cross(direita, cima)
    mundo = [centro + (p[0] - meio[0]) * k * direita + (p[1] - meio[1]) * k * cima for p in pts]
    for f in faces:
        m.face([mundo[i] for i in f], cor, n=n, brilho=brilho)


# --- polígonos ---
def area2(poly):
    a = 0.0
    for k in range(len(poly)):
        x1, z1 = poly[k][0], poly[k][1]
        x2, z2 = poly[(k + 1) % len(poly)][0], poly[(k + 1) % len(poly)][1]
        a += x1 * z2 - x2 * z1
    return a / 2


def cortar_meio_plano(poly, a, n):
    """O que fica do polígono (em 2D ou 3D) do lado (p - a)·n >= 0"""
    out = []
    m = len(poly)
    a, n = np.asarray(a, float), np.asarray(n, float)
    for k in range(m):
        p, q = np.asarray(poly[k], float), np.asarray(poly[(k + 1) % m], float)
        dp, dq = np.dot(p - a, n), np.dot(q - a, n)
        if dp >= 0:
            out.append(p)
        if (dp >= 0) != (dq >= 0):
            t = dp / (dp - dq)
            out.append(p + t * (q - p))
    return out


def cortar_convexo(poly, conv):
    s = 1 if area2(conv) > 0 else -1
    for k in range(len(conv)):
        a, b = np.asarray(conv[k]), np.asarray(conv[(k + 1) % len(conv)])
        e = b - a
        n = np.array([-e[1], e[0]]) * s
        poly = cortar_meio_plano(poly, a, n)
        if len(poly) < 3:
            return []
    return poly


def grade_no_convexo(conv, passo):
    xs = [p[0] for p in conv]
    zs = [p[1] for p in conv]
    x0 = math.floor(min(xs) / passo) * passo
    z0 = math.floor(min(zs) / passo) * passo
    pecas = []
    x = x0
    while x < max(xs):
        z = z0
        while z < max(zs):
            cel = [np.array([x, z]), np.array([x + passo, z]), np.array([x + passo, z + passo]), np.array([x, z + passo])]
            c = cortar_convexo(cel, conv)
            if len(c) >= 3 and abs(area2(c)) > 1e-5:
                pecas.append(c)
            z += passo
        x += passo
    return pecas


def disco_cortado(raio, u_max, n=96):
    """O círculo (em u, v) de raio 'raio' cortado em u <= u_max (convexo)"""
    pts = [np.array([raio * math.cos(a), raio * math.sin(a)]) for a in np.linspace(0, 2 * math.pi, n, endpoint=False)]
    return cortar_meio_plano(pts, (u_max, 0.0), (-1.0, 0.0))


# ------------------------------------------------------------------
# 3a. O quarto num cone: (u, h, v) do desenho -> mundo
# ------------------------------------------------------------------
class Quarto:
    def __init__(self, numero, eixo, rumo, espelho):
        self.numero = numero
        self.eixo = np.array(eixo, float)
        self.rumo = rumo
        self.f = np.array([math.sin(rumo), -math.cos(rumo)])          # para fora, pela porta
        self.d = np.array([-math.cos(rumo), -math.sin(rumo)])         # à direita de quem olha a porta de fora
        self.s = -1.0 if espelho else 1.0
        self.espelho = espelho
        self.mcqueen = numero == CONE_DO_McQUEEN
        self.caixas_de_colisao = []        # polígonos (u, v) dos obstáculos
        self.luzes = []
        # o que esconde a parede ou o piso atrás (a luz assada ali é trocada pela dos vizinhos à vista):
        # (referencial, contornos convexos em (a, y) dele, b mínimo, b máximo)
        self.escondidos = []

    def xz(self, u, v):
        return self.eixo + u * self.f + self.s * v * self.d

    def p(self, u, h, v):
        q = self.xz(u, v)
        return np.array([q[0], PISO_Y + h, q[1]])

    def pc(self, c):
        return self.p(c[0], c[1], c[2])

    def vet(self, du, dh, dv):
        q = du * self.f + self.s * dv * self.d
        return np.array([q[0], dh, q[1]])

    def vc(self, c):
        return self.vet(c[0], c[1], c[2])


def canon_da_parede(th, h, b=0.0):
    """O ponto (u, h, v) da parede de cone no ângulo th (a partir da porta, para +v), h acima do piso,
    b para dentro (na horizontal)"""
    r = raio_da_parede(h) - b
    return np.array([r * math.cos(th), h, r * math.sin(th)])


def normal_canon(th):
    """A normal da parede de cone (para dentro do quarto; ela olha um pouco para baixo)"""
    return unit(np.array([-math.cos(th), -INCLINACAO, -math.sin(th)]))


def alto_canon(th):
    return unit(np.array([-INCLINACAO * math.cos(th), 1.0, -INCLINACAO * math.sin(th)]))


def ref_na_parede(Q, th, h, b=0.0):
    """O referencial de um objeto pendurado na parede de cone (inclinado com ela)"""
    return ref_de(Q.pc(canon_da_parede(th, h, b)), Q.vc(normal_canon(th)), Q.vc(alto_canon(th)))


def ref_no_chao(Q, u, v, para_u, para_v):
    """O referencial de um objeto no chão em (u, v), com a frente na direção (para_u, para_v)"""
    return ref_de(Q.p(u, 0.0, v), Q.vet(para_u, 0.0, para_v))


def theta_da_coluna(j):
    return -math.pi + 2 * math.pi * j / COLUNAS_DA_PAREDE


def uv_do_papel(th, h):
    return (th * R_DO_PAPEL / METROS_DO_PAPEL, h / METROS_DO_PAPEL)


U_LIMITE = U_FRENTE + 0.03      # a parede de cone passa um pouco por trás da parede da porta (sem frestas)


class Janela:
    """Uma janela redonda (em u, h, v): o meio do vidro da casca, a normal para fora e a base do plano"""

    def __init__(self, dados):
        C, n = dados
        self.C = np.array([C[0], C[1] - PISO_Y, C[2]])
        self.n = unit(np.array(n, float))
        e1 = unit(np.cross(np.array([0.0, 1.0, 0.0]), self.n))
        e2 = unit(np.cross(self.n, e1))
        if e2[1] < 0:
            e2 = -e2
        self.e1, self.e2 = e1, e2
        self.th = math.atan2(self.C[2], self.C[0])
        r = raio_da_parede(self.C[1])
        meia = (RAIO_DO_VAO + 0.30) / r
        self.j0 = int(math.floor((self.th - meia + math.pi) / (2 * math.pi / COLUNAS_DA_PAREDE)))
        self.j1 = int(math.ceil((self.th + meia + math.pi) / (2 * math.pi / COLUNAS_DA_PAREDE)))
        self.i0 = max(k for k, h in enumerate(NIVEIS) if h <= self.C[1] - RAIO_DO_VAO - 0.25)
        self.i1 = min(k for k, h in enumerate(NIVEIS) if h >= self.C[1] + RAIO_DO_VAO + 0.25)

    def no_plano(self, rho, phi, a):
        return self.C + rho * (math.cos(phi) * self.e1 + math.sin(phi) * self.e2) + a * self.n

    def na_parede(self, rho, phi, b=0.0):
        """O ponto da parede (b para dentro) na reta paralela à normal, a rho do meio, no ângulo phi"""
        def f(a):
            p = self.no_plano(rho, phi, a)
            return math.hypot(p[0], p[2]) - (raio_da_parede(p[1]) - b)
        lo, hi = -1.5, 1.0
        for _ in range(50):
            mid = (lo + hi) / 2
            if f(mid) > 0:
                hi = mid
            else:
                lo = mid
        return self.no_plano(rho, phi, (lo + hi) / 2)

    def fase(self, p):
        q = p - self.C
        return math.atan2(float(q @ self.e2), float(q @ self.e1))

    def tem(self, j, i):
        return self.j0 <= j < self.j1 and self.i0 <= i < self.i1


def para_param(p):
    """(th, h) de um ponto (u, h, v) da parede"""
    return math.atan2(p[2], p[0]), p[1]


def face_da_parede(Q, canon, ths):
    """Uma face da parede de cone (pontos em u, h, v e o ângulo de cada um): a barra embaixo, o papel em cima"""
    hs = [c[1] for c in canon]
    thm = float(np.mean(ths))
    n = Q.vc(normal_canon(thm))
    pts = [Q.pc(c) for c in canon]
    if max(hs) <= ALTO_DA_BARRA + 1e-6:
        CORES.face(pts, BARRA, n=n, sup=SUP_CONE)
    else:
        MALHAS['parede'].face(pts, BRANCO, n=n, uvs=[uv_do_papel(t, h) for t, h in zip(ths, hs)], sup=SUP_CONE)


def montar_parede_de_cone(Q, janelas):
    for j in range(COLUNAS_DA_PAREDE):
        t0, t1 = theta_da_coluna(j), theta_da_coluna(j + 1)
        for i in range(len(NIVEIS) - 1):
            if any(w.tem(j, i) for w in janelas):
                continue
            h0, h1 = NIVEIS[i], NIVEIS[i + 1]
            canon = [canon_da_parede(t0, h0), canon_da_parede(t1, h0), canon_da_parede(t1, h1), canon_da_parede(t0, h1)]
            us = [c[0] for c in canon]
            if min(us) >= U_LIMITE:
                continue
            if max(us) > U_LIMITE:
                canon = cortar_meio_plano(canon, (U_LIMITE, 0.0, 0.0), (-1.0, 0.0, 0.0))
                if len(canon) < 3:
                    continue
            ths = []
            for c in canon:
                t = math.atan2(c[2], c[0])
                t = t0 + (t - t0 + math.pi) % (2 * math.pi) - math.pi
                ths.append(t)
            face_da_parede(Q, canon, ths)


def montar_janela(Q, w, lado_do_ar):
    """O remendo da parede em volta do furo (uma grade em anéis, sem junções em T com a parede), o vão
    chanfrado, a moldura na parede, o caixilho em cruz, o bandô e as cortinas"""
    # o contorno do remendo, na grade da parede (anti-horário em (th, h))
    borda = []
    for j in range(w.j0, w.j1 + 1):
        borda.append((theta_da_coluna(j), NIVEIS[w.i0]))
    for i in range(w.i0 + 1, w.i1 + 1):
        borda.append((theta_da_coluna(w.j1), NIVEIS[i]))
    for j in range(w.j1 - 1, w.j0 - 1, -1):
        borda.append((theta_da_coluna(j), NIVEIS[w.i1]))
    for i in range(w.i1 - 1, w.i0, -1):
        borda.append((theta_da_coluna(w.j0), NIVEIS[i]))
    furo = []
    for th, h in borda:
        phi = w.fase(canon_da_parede(th, h))
        furo.append((phi, w.na_parede(RAIO_DO_VAO, phi)))
    aneis = 4
    grade = []
    for k in range(aneis + 1):
        linha = []
        for (th, h), (phi, H) in zip(borda, furo):
            tH, hH = para_param(H)
            tH = th + (tH - th + math.pi) % (2 * math.pi) - math.pi
            s = k / aneis
            linha.append((th + (tH - th) * s, h + (hH - h) * s))
        grade.append(linha)
    nb = len(borda)
    for k in range(aneis):
        for i in range(nb):
            ps = [grade[k][i], grade[k][(i + 1) % nb], grade[k + 1][(i + 1) % nb], grade[k + 1][i]]
            canon = [canon_da_parede(t, h) for t, h in ps]
            face_da_parede(Q, canon, [t for t, h in ps])
    # o vão chanfrado (creme): da parede até a ponta de fora, onde fica o caixilho
    for i in range(nb):
        (pa, Ha), (pb, Hb) = furo[i], furo[(i + 1) % nb]
        Ia = w.no_plano(RAIO_DA_JANELA, pa, FUNDO_DA_JANELA)
        Ib = w.no_plano(RAIO_DA_JANELA, pb, FUNDO_DA_JANELA)
        pm = pa + ((pb - pa + math.pi) % (2 * math.pi) - math.pi) / 2
        n = -(math.cos(pm) * w.e1 + math.sin(pm) * w.e2)
        quad(CORES, Q.pc(Ha), Q.pc(Hb), Q.pc(Ib), Q.pc(Ia), VAO_DA_JANELA, n=Q.vc(n), div=(1, 2))
    # a moldura na parede, em volta do furo
    fases = np.linspace(0, 2 * math.pi, 49)
    dentro = [w.na_parede(RAIO_DO_VAO - 0.03, p, b=0.022) for p in fases]
    fora = [w.na_parede(RAIO_DO_VAO + 0.07, p, b=0.022) for p in fases]
    rente = [w.na_parede(RAIO_DO_VAO + 0.07, p, b=-0.01) for p in fases]
    for k in range(48):
        th = para_param(dentro[k])[0]
        nw = Q.vc(normal_canon(th))
        quad(CORES, Q.pc(dentro[k]), Q.pc(dentro[k + 1]), Q.pc(fora[k + 1]), Q.pc(fora[k]), CREME, n=nw, div=(1, 1))
        rad = Q.vc(unit(fora[k] - w.C - ((fora[k] - w.C) @ w.n) * w.n))
        quad(CORES, Q.pc(fora[k]), Q.pc(fora[k + 1]), Q.pc(rente[k + 1]), Q.pc(rente[k]), CREME_ESCURO, n=rad, div=(1, 1))
    # o caixilho: o aro e a cruz, na ponta de fora do chanfro
    R = Ref(Q.pc(w.C), Q.vc(w.e1), Q.vc(w.e2), Q.vc(-w.n))
    b0 = -FUNDO_DA_JANELA - 0.005
    b1 = b0 + 0.045
    eixo = Q.vc(-w.n)
    torno(CORES, R.p(0, 0, b0), eixo, Q.vc(w.e1), [(RAIO_DA_JANELA + 0.01, 0.0), (RAIO_DA_JANELA + 0.01, 0.045),
                                                   (RAIO_DA_JANELA - 0.065, 0.045), (RAIO_DA_JANELA - 0.065, 0.0)], 40, CREME)
    caixa(CORES, R, -0.035, 0.035, -RAIO_DA_JANELA + 0.03, RAIO_DA_JANELA - 0.03, b0 + 0.005, b1 - 0.008, CREME, passo=0.5)
    caixa(CORES, R, -RAIO_DA_JANELA + 0.03, RAIO_DA_JANELA - 0.03, -0.035, 0.035, b0 + 0.005, b1 - 0.008, CREME, passo=0.5)
    # o bandô: uma caixa em arco por cima da janela, acompanhando a parede
    r_janela = raio_da_parede(w.C[1])
    meia = 1.48 / r_janela
    alto = w.C[1] + RAIO_DO_VAO + 0.42
    baixo = alto - 0.46
    arco_caixa(Q, w.th - meia, w.th + meia, baixo, alto, 0.0, 0.30, BANDO, 18, faixa=CREME_ESCURO)
    # as cortinas, dos dois lados, amarradas
    for lado in (-1, 1):
        cortina(Q, w, lado, baixo + 0.02, w.C[1] - 1.17, lado_do_ar)


def arco_caixa(Q, th0, th1, h0, h1, b0, b1, cor, n_seg, faixa=None):
    """Uma caixa em arco encostada na parede de cone: da parede (b0) até b1, de th0 a th1 e de h0 a h1"""
    ths = np.linspace(th0, th1, n_seg + 1)
    for k in range(n_seg):
        ta, tb = ths[k], ths[k + 1]
        tm = (ta + tb) / 2
        nin = Q.vc(normal_canon(tm))
        c = lambda t, h, b: Q.pc(canon_da_parede(t, h, b))
        quad(CORES, c(ta, h0, b1), c(tb, h0, b1), c(tb, h1, b1), c(ta, h1, b1), escurecer(cor, 0.93 if k % 2 else 1.0), n=nin, div=(1, 2))
        quad(CORES, c(ta, h0, b0), c(tb, h0, b0), c(tb, h0, b1), c(ta, h0, b1), escurecer(cor, 0.9), n=(0, -1, 0), div=(1, 1))
        quad(CORES, c(ta, h1, b0), c(tb, h1, b0), c(tb, h1, b1), c(ta, h1, b1), cor, n=(0, 1, 0), div=(1, 1))
        if faixa:
            quad(CORES, c(ta, h0 + 0.03, b1 + 0.004), c(tb, h0 + 0.03, b1 + 0.004), c(tb, h0 + 0.065, b1 + 0.004),
                 c(ta, h0 + 0.065, b1 + 0.004), faixa, n=nin, div=(1, 1))
    for t, s in ((th0, -1), (th1, 1)):
        c = lambda h, b: Q.pc(canon_da_parede(t, h, b))
        lado = Q.vc(np.array([-math.sin(t), 0.0, math.cos(t)]) * s)
        quad(CORES, c(h0, b0), c(h0, b1), c(h1, b1), c(h1, b0), cor, n=lado, div=(1, 1))


def cortina(Q, w, lado, h_cima, h_baixo, lado_do_ar):
    """Uma cortina clara, franzida, amarrada um pouco abaixo do meio da janela e aberta embaixo"""
    ns, nt = 12, 16
    b_meio = 0.19
    h_laco = w.C[1] - 0.30
    t_laco = (h_cima - h_laco) / (h_cima - h_baixo)
    r_ref = raio_da_parede(w.C[1]) - b_meio

    def forma(t):
        """(meio, largura, amplitude das dobras) da cortina na fração t (0 em cima, 1 embaixo)"""
        if t <= t_laco:
            s = suave(0.0, 1.0, t / t_laco)
            return 0.98 + (1.24 - 0.98) * s, 0.74 + (0.15 - 0.74) * s, 0.045 + (0.02 - 0.045) * s
        s = (t - t_laco) / (1 - t_laco)
        s = s ** 0.8
        return 1.24 + (1.20 - 1.24) * s, 0.15 + (0.40 - 0.15) * s, 0.02 + (0.05 - 0.02) * s

    def ponto(s, t):
        meio, larg, amp = forma(t)
        x = lado * (meio + (s - 0.5) * larg)
        h = h_cima + (h_baixo - h_cima) * t
        b = b_meio + amp * math.sin(2 * math.pi * 4.5 * s + 0.6)
        th = w.th + x / r_ref
        return canon_da_parede(th, h, b), th
    grade = [[ponto(s / ns, t / nt) for s in range(ns + 1)] for t in range(nt + 1)]
    for i in range(nt):
        for j in range(ns):
            ps = [grade[i][j], grade[i][j + 1], grade[i + 1][j + 1], grade[i + 1][j]]
            th = ps[0][1]
            sombra = 0.92 + 0.08 * math.cos(2 * math.pi * 4.5 * (j + 0.5) / ns + 0.6)
            CORES.face([Q.pc(p[0]) for p in ps], escurecer(CORTINA, sombra), n=Q.vc(normal_canon(th)))
    # o laço
    meio, larg, amp = forma(t_laco)
    ths = [w.th + lado * (meio + (s - 0.5) * (larg + 0.04)) / r_ref for s in np.linspace(0, 1, 7)]
    for k in range(6):
        ta, tb = ths[k], ths[k + 1]
        c = lambda t, h: Q.pc(canon_da_parede(t, h, b_meio + amp + 0.02))
        CORES.face([c(ta, h_laco - 0.035), c(tb, h_laco - 0.035), c(tb, h_laco + 0.035), c(ta, h_laco + 0.035)], LACO,
                   n=Q.vc(normal_canon(ta)))


# ------------------------------------------------------------------
# 3b. A parede da porta, o piso, o teto e as molduras em volta
# ------------------------------------------------------------------
def montar_parede_da_frente(Q):
    v_max = meia_largura_da_frente(0.0, 0.03) + 0.05
    cortes = [-v_max, PORTA_V[0], PORTA_V[1], v_max]
    vs = []
    for a, b in zip(cortes[:-1], cortes[1:]):
        n = max(1, math.ceil((b - a) / PASSO_DA_PAREDE_V - 1e-6))
        vs += [a + (b - a) * k / n for k in range(n)]
    vs.append(v_max)
    n = Q.vet(-1.0, 0.0, 0.0)
    for i in range(len(NIVEIS) - 1):
        h0, h1 = NIVEIS[i], NIVEIS[i + 1]
        c0, c1 = meia_largura_da_frente(h0, 0.03), meia_largura_da_frente(h1, 0.03)
        for k in range(len(vs) - 1):
            va, vb = vs[k], vs[k + 1]
            if PORTA_V[0] - 1e-6 <= (va + vb) / 2 <= PORTA_V[1] + 1e-6 and h1 <= PORTA_ALTURA + 1e-6:
                continue
            pc = [np.array([va, h0]), np.array([vb, h0]), np.array([vb, h1]), np.array([va, h1])]
            pc = cortar_meio_plano(pc, (c0, h0), (-(h1 - h0), c1 - c0))
            if len(pc) >= 3:
                pc = cortar_meio_plano(pc, (-c0, h0), (h1 - h0, c1 - c0))
            if len(pc) < 3 or abs(area2(pc)) < 1e-6:
                continue
            pts = [Q.p(U_FRENTE, p[1], p[0]) for p in pc]
            if h1 <= ALTO_DA_BARRA + 1e-6:
                CORES.face(pts, BARRA, n=n, sup=SUP_FRENTE)
            else:
                MALHAS['parede'].face(pts, BRANCO, n=n, uvs=[(p[0] / METROS_DO_PAPEL, p[1] / METROS_DO_PAPEL) for p in pc],
                                      sup=SUP_FRENTE)


def montar_piso_e_teto(Q):
    up = Q.vet(0, 1, 0)
    pecas = grade_no_convexo(disco_cortado(raio_da_parede(0.0) + 0.03, U_FRENTE + 0.01), PASSO_DO_PISO)
    soleira = [np.array(p) for p in ((U_FRENTE, PORTA_V[0]), (U_SOLEIRA, PORTA_V[0]), (U_SOLEIRA, PORTA_V[1]), (U_FRENTE, PORTA_V[1]))]
    pecas += grade_no_convexo(soleira, PASSO_DO_PISO)
    for pc in pecas:
        MALHAS['carpete'].face([Q.p(p[0], 0.0, p[1]) for p in pc], BRANCO, n=up,
                               uvs=[(p[0] / METROS_DO_CARPETE, p[1] / METROS_DO_CARPETE) for p in pc], sup=SUP_PISO)
    # o perfil de alumínio na soleira
    caixa_c(Q, CORES, (U_FRENTE - 0.05, U_SOLEIRA - 0.02), (0.0, 0.006), PORTA_V, METAL, sem=('-h',), passo=1.0)
    down = Q.vet(0, -1, 0)
    for pc in grade_no_convexo(disco_cortado(raio_da_parede(ALTURA) + 0.03, U_LIMITE), PASSO_DO_TETO):
        CORES.face([Q.p(p[0], ALTURA, p[1]) for p in pc], TETO, n=down, sup=SUP_TETO)


def caixa_c(Q, m, us, hs, vs, cor, sem=(), passo=PASSO, cores=None, brilho=None, uvf=None):
    """Uma caixa alinhada com os eixos do quarto (u, h, v). 'sem' tira faces: '+u', '-u', '+h', '-h', '+v', '-v'"""
    (u0, u1), (h0, h1), (v0, v1) = us, hs, vs
    P = lambda u, h, v: Q.p(u, h, v)
    faces = {
        '+u': ([P(u1, h0, v0), P(u1, h0, v1), P(u1, h1, v1), P(u1, h1, v0)], (1, 0, 0)),
        '-u': ([P(u0, h0, v0), P(u0, h0, v1), P(u0, h1, v1), P(u0, h1, v0)], (-1, 0, 0)),
        '+h': ([P(u0, h1, v0), P(u1, h1, v0), P(u1, h1, v1), P(u0, h1, v1)], (0, 1, 0)),
        '-h': ([P(u0, h0, v0), P(u1, h0, v0), P(u1, h0, v1), P(u0, h0, v1)], (0, -1, 0)),
        '+v': ([P(u0, h0, v1), P(u1, h0, v1), P(u1, h1, v1), P(u0, h1, v1)], (0, 0, 1)),
        '-v': ([P(u0, h0, v0), P(u1, h0, v0), P(u1, h1, v0), P(u0, h1, v0)], (0, 0, -1)),
    }
    for nome, (pts, n) in faces.items():
        if nome in sem:
            continue
        c = cores.get(nome, cor) if cores else cor
        quad(m, *pts, c, n=Q.vc(n), passo=passo, brilho=brilho, uvf=uvf)


VAO_DA_MOLDURA = 0.28          # a largura da moldura clara da porta (as molduras de baixo param nela)


def contorno(h, b, colunas, porta=True, partes=(3, 3)):
    """O contorno do quarto (em u, v) na altura h, b para dentro das paredes. Com 'porta', começa e
    termina na moldura da porta (v > 0 primeiro); sem ela, dá a volta inteira (o último ponto é o
    primeiro). 'colunas' são os ângulos do arco (os mesmos para todos os pontos de um perfil);
    'partes', em quantos pedaços vai a parede da porta de cada lado. Devolve os pontos e, para cada
    trecho, a direção (u, v) para dentro do quarto."""
    rr = raio_da_parede(h) - b
    uc = U_FRENTE - b
    c = math.sqrt(max(rr * rr - uc * uc, 0.0))
    tc = math.atan2(c, uc)
    frente = np.array([-1.0, 0.0])
    pts, dirs = [], []
    if porta:
        va = PORTA_V[1] + VAO_DA_MOLDURA
        for k in range(partes[0]):
            pts.append(np.array([uc, va + (c - va) * k / partes[0]]))
            dirs.append(frente)
    arco = [tc] + list(colunas) + [2 * math.pi - tc]
    for k, t in enumerate(arco):
        pts.append(np.array([rr * math.cos(t), rr * math.sin(t)]))
        if k < len(arco) - 1:
            tm = (t + arco[k + 1]) / 2
            dirs.append(-np.array([math.cos(tm), math.sin(tm)]))
    if porta:
        vb = PORTA_V[0] - VAO_DA_MOLDURA
        for k in range(1, partes[1] + 1):
            dirs.append(frente)
            pts.append(np.array([uc, -c + (vb + c) * k / partes[1]]))
    else:
        n = partes[0] + partes[1] + 2
        for k in range(1, n + 1):
            dirs.append(frente)
            pts.append(np.array([uc, -c + 2 * c * k / n]))
    return pts, dirs


def colunas_do_arco(perfil):
    """Os ângulos das colunas da parede que ficam no arco em todas as alturas do perfil"""
    tmax = 0.0
    for b, h in perfil:
        rr = raio_da_parede(h) - b
        uc = U_FRENTE - b
        tmax = max(tmax, math.atan2(math.sqrt(max(rr * rr - uc * uc, 0.0)), uc))
    cols = []
    for j in range(COLUNAS_DA_PAREDE):
        t = theta_da_coluna(j) % (2 * math.pi)
        if tmax + 0.01 < t < 2 * math.pi - tmax - 0.01:
            cols.append(t)
    return sorted(cols)


def perfil_em_volta(Q, perfil, cor, porta=True):
    """Corre o perfil [(b, h)], de baixo para cima, em volta do quarto"""
    cols = colunas_do_arco(perfil)
    linhas = [contorno(h, b, cols, porta) for b, h in perfil]
    for k in range(len(perfil) - 1):
        (b0, h0), (b1, h1) = perfil[k], perfil[k + 1]
        (L0, D0), (L1, _) = linhas[k], linhas[k + 1]
        db, dh = b1 - b0, h1 - h0
        for i in range(len(L0) - 1):
            d = D0[i]
            n = Q.vet(d[0] * dh, -db, d[1] * dh)
            if np.linalg.norm(n) < 1e-9:
                continue
            pts = [Q.p(L0[i][0], h0, L0[i][1]), Q.p(L0[i + 1][0], h0, L0[i + 1][1]),
                   Q.p(L1[i + 1][0], h1, L1[i + 1][1]), Q.p(L1[i][0], h1, L1[i][1])]
            CORES.face(pts, cor, n=n)
    if porta:
        # as pontas, na moldura da porta
        for v, s in ((PORTA_V[1] + VAO_DA_MOLDURA, 1), (PORTA_V[0] - VAO_DA_MOLDURA, -1)):
            pts = [Q.p(U_FRENTE, perfil[0][1], v)] + [Q.p(U_FRENTE - b, h, v) for b, h in perfil] + [Q.p(U_FRENTE, perfil[-1][1], v)]
            CORES.face(pts, cor, n=Q.vet(0, 0, s))


PERFIL_DO_RODAPE = [(0.035, 0.0), (0.035, 0.13), (0.018, 0.15), (0.0, 0.16)]
PERFIL_DO_FRISO = [(0.0, 0.76), (0.03, 0.765), (0.042, 0.785), (0.042, 0.825), (0.026, 0.845), (0.0, 0.852)]
PERFIL_DA_SANCA = [(0.0, ALTURA - 0.24), (0.035, ALTURA - 0.23), (0.11, ALTURA - 0.14), (0.19, ALTURA - 0.055), (0.22, ALTURA)]


def montar_friso_de_dentes(Q):
    """Os triângulos escuros (dentes de serra) na barra, logo acima do rodapé"""
    b = 0.009
    h0, h1 = 0.16, 0.60
    larg = 0.40

    def dente(pa, pb, pt, n):
        m_ab, m_bt, m_ta = (pa + pb) / 2, (pb + pt) / 2, (pt + pa) / 2
        for tri in ((pa, m_ab, m_ta), (m_ab, pb, m_bt), (m_ta, m_bt, pt), (m_ab, m_bt, m_ta)):
            CORES.face([Q.pc(p) for p in tri], DENTE, n=n)
    # no arco
    rr = raio_da_parede(h0) - b
    tc = math.atan2(math.sqrt(rr * rr - (U_FRENTE - b) ** 2), U_FRENTE - b) + 0.004
    L = rr * (2 * math.pi - 2 * tc)
    nd = max(1, round(L / larg))
    for k in range(nd):
        ta = tc + (2 * math.pi - 2 * tc) * k / nd
        tb = tc + (2 * math.pi - 2 * tc) * (k + 1) / nd
        tm = (ta + tb) / 2
        dente(canon_da_parede(ta, h0, b), canon_da_parede(tb, h0, b), canon_da_parede(tm, h1, b), Q.vc(normal_canon(tm)))
    # na parede da porta, dos dois lados da moldura
    for va, vb in ((PORTA_V[1] + VAO_DA_MOLDURA + 0.02, meia_largura_da_frente(h1, -b - 0.004)),
                   (-meia_largura_da_frente(h1, -b - 0.004), PORTA_V[0] - VAO_DA_MOLDURA - 0.02)):
        nd = max(1, round((vb - va) / larg))
        u = U_FRENTE - b
        for k in range(nd):
            a, c = va + (vb - va) * k / nd, va + (vb - va) * (k + 1) / nd
            dente(np.array([u, h0, a]), np.array([u, h0, c]), np.array([u, h1, (a + c) / 2]), Q.vet(-1, 0, 0))


def montar_porta(Q):
    """A moldura clara da porta, o forro do vão e a caixa da porta de garagem enrolada no alto"""
    v0, v1 = PORTA_V
    m = VAO_DA_MOLDURA
    uf = U_FRENTE
    MARCA_ATUAL[0] = MARCA_MOLDURA
    caixa_c(Q, CORES, (uf - 0.05, uf), (0.0, PORTA_ALTURA + m), (v0 - m, v0), CREME, sem=('+u', '-h'))
    caixa_c(Q, CORES, (uf - 0.05, uf), (0.0, PORTA_ALTURA + m), (v1, v1 + m), CREME, sem=('+u', '-h'))
    caixa_c(Q, CORES, (uf - 0.05, uf), (PORTA_ALTURA, PORTA_ALTURA + m), (v0, v1), CREME, sem=('+u', '-v', '+v'))
    MARCA_ATUAL[0] = 0
    # o filete de dentro da moldura (um degrau mais escuro)
    for vs in ((v0 - 0.05, v0), (v1, v1 + 0.05)):
        caixa_c(Q, CORES, (uf - 0.065, uf - 0.05), (0.0, PORTA_ALTURA + 0.05), vs, CREME_ESCURO, sem=('+u', '-h'))
    caixa_c(Q, CORES, (uf - 0.065, uf - 0.05), (PORTA_ALTURA, PORTA_ALTURA + 0.05), (v0, v1), CREME_ESCURO, sem=('+u', '-v', '+v'))
    # a parede da porta atrás da moldura fica escondida
    R = Ref(Q.p(uf, 0.0, 0.0), Q.vet(0, 0, 1), Q.vet(0, 1, 0), Q.vet(-1, 0, 0))
    k = 0.01
    Q.escondidos.append((R, [[(v0 - m + k, -1.0), (v0 - k, -1.0), (v0 - k, PORTA_ALTURA + m - k), (v0 - m + k, PORTA_ALTURA + m - k)],
                             [(v1 + k, -1.0), (v1 + m - k, -1.0), (v1 + m - k, PORTA_ALTURA + m - k), (v1 + k, PORTA_ALTURA + m - k)],
                             [(v0 - m + k, PORTA_ALTURA + k), (v1 + m - k, PORTA_ALTURA + k), (v1 + m - k, PORTA_ALTURA + m - k),
                              (v0 - m + k, PORTA_ALTURA + m - k)]], -0.02, 0.07))
    montar_forro_da_porta(Q)
    # a porta de garagem levantada: o painel deitado sob o teto do vão, entrando no quarto, nos trilhos
    h0 = PORTA_ALTURA + m + 0.02
    fundo = uf - 2.1
    painel = (0.66, 0.46, 0.36)
    caixa_c(Q, CORES, (fundo, uf - 0.06), (h0, h0 + 0.07), (v0 - 0.12, v1 + 0.12), painel, passo=0.5,
            cores={'+h': escurecer(painel, 0.8), '-u': CREME})
    for k in range(1, 4):                               # as juntas das seções da porta
        uu = uf - 0.06 - (uf - 0.06 - fundo) * k / 4
        caixa_c(Q, CORES, (uu - 0.025, uu + 0.025), (h0 - 0.012, h0), (v0 - 0.10, v1 + 0.10), escurecer(painel, 0.72), passo=1.0)
    caixa_c(Q, CORES, (fundo - 0.03, fundo + 0.06), (h0 - 0.05, h0 + 0.08), (v0 - 0.13, v1 + 0.13), CREME, passo=1.0)
    caixa_c(Q, CORES, (fundo + 0.08, fundo + 0.16), (h0 - 0.09, h0 - 0.05), (-0.30, 0.45), METAL, passo=1.0)
    for vv in (v0 - 0.20, v1 + 0.12):                   # os trilhos, presos na parede e no teto
        caixa_c(Q, CORES, (fundo - 0.10, uf), (h0 - 0.02, h0 + 0.12), (vv, vv + 0.08), (0.38, 0.38, 0.40), passo=1.0)
        caixa_c(Q, CORES, (fundo + 0.11, fundo + 0.15), (h0 + 0.12, ALTURA), (vv + 0.02, vv + 0.06), (0.66, 0.64, 0.62), passo=1.0)


def montar_forro_da_porta(Q):
    """O forro creme por cima dos batentes e da verga da casca (o lado de dentro e a frente deles, até a boca
    do nicho), que no mapa têm uma textura de cromado esverdeado. Não é assado: leva a cor que a moldura
    clara da porta ficou (finalizar_cores)"""
    pele = MALHAS['forro']
    MARCA_ATUAL[0] = MARCA_FORRO
    u0, u1 = FORRO_U
    va, vb = FORRO_V
    hv, ha = FORRO_VERGA, FORRO_ALTO
    P = Q.p
    # o lado de dentro dos batentes e o de baixo da verga
    quad(pele, P(u0, 0.0, va), P(u1, 0.0, va), P(u1, hv, va), P(u0, hv, va), CREME, n=Q.vet(0, 0, 1), div=(2, 4))
    quad(pele, P(u0, 0.0, vb), P(u1, 0.0, vb), P(u1, hv, vb), P(u0, hv, vb), CREME, n=Q.vet(0, 0, -1), div=(2, 4))
    quad(pele, P(u0, hv, va), P(u1, hv, va), P(u1, hv, vb), P(u0, hv, vb), CREME, n=Q.vet(0, -1, 0), div=(2, 4))
    # a frente dos batentes e da verga (para fora, na boca do nicho)
    fora = Q.vet(1, 0, 0)
    quad(pele, P(u1, 0.0, FORRO_FRENTE_V[0]), P(u1, 0.0, va), P(u1, ha, va), P(u1, ha, FORRO_FRENTE_V[0]), CREME, n=fora, div=(1, 4))
    quad(pele, P(u1, 0.0, vb), P(u1, 0.0, FORRO_FRENTE_V[1]), P(u1, ha, FORRO_FRENTE_V[1]), P(u1, ha, vb), CREME, n=fora, div=(1, 4))
    quad(pele, P(u1, hv, va), P(u1, hv, vb), P(u1, ha, vb), P(u1, ha, va), CREME, n=fora, div=(4, 1))
    MARCA_ATUAL[0] = 0


# ------------------------------------------------------------------
# 3c. Os móveis e os enfeites
# ------------------------------------------------------------------
def montar_ar_condicionado(Q):
    """O ar-condicionado de janela, creme, por cima do que a casca enfia no quarto: a grade de lâminas, a
    roda de lâminas diagonais e o painel com os dois botões; com o relógio de cone em cima"""
    meio = (AR_CASCA_A + AR_CASCA_B) / 2 - 0.06 * AR_CASCA_FORA
    R = ref_de(Q.p(meio[0], 0.0, meio[1]), Q.vet(-AR_CASCA_FORA[0], 0.0, -AR_CASCA_FORA[1]))
    L = float(np.linalg.norm(AR_CASCA_B - AR_CASCA_A)) / 2 + 0.08
    h0, h1 = 0.42, 1.52
    # a caixa, aberta atrás: o fundo acompanha a inclinação da casca (atrás da nossa parede e dentro da
    # casca, sem aparecer por fora)
    f0, f1 = fundo_do_ar(h0, meio, L), fundo_do_ar(h1, meio, L)
    P = R.p
    quad(CORES, P(-L, h0, 0.0), P(L, h0, 0.0), P(L, h1, 0.0), P(-L, h1, 0.0), AR_COR, n=R.ez, passo=0.3)
    for a, sa in ((-L, -1), (L, 1)):
        quad(CORES, P(a, h0, -f0), P(a, h0, 0.0), P(a, h1, 0.0), P(a, h1, -f1), AR_COR, n=R.d(sa, 0, 0), passo=0.3)
    quad(CORES, P(-L, h1, -f1), P(L, h1, -f1), P(L, h1, 0.0), P(-L, h1, 0.0), AR_COR, n=R.ey, passo=0.3)
    quad(CORES, P(-L, h0, -f0), P(L, h0, -f0), P(L, h0, 0.0), P(-L, h0, 0.0), escurecer(AR_COR, 0.8), n=R.d(0, -1, 0), passo=0.3)
    k = 0.02
    Q.escondidos.append((R, [[(-L + k, h0 + k), (L - k, h0 + k), (L - k, h1 - k), (-L + k, h1 - k)]], -2.0, 0.0))
    e = 0.025
    for a0, a1, hh0, hh1 in ((-L, L, h0, h0 + 0.07), (-L, L, h1 - 0.07, h1), (-L, -L + 0.07, h0, h1), (L - 0.07, L, h0, h1),
                             (0.36, 0.42, h0, h1)):
        caixa(CORES, R, a0, a1, hh0, hh1, 0.0, e, escurecer(AR_COR, 1.03), sem=('tras',), passo=0.5)
    # as lâminas da grade (riscos escuros)
    a0, a1 = -L + 0.09, 0.34
    for k in range(13):
        hh = h0 + 0.12 + k * 0.067
        quad(CORES, R.p(a0, hh, 0.004), R.p(a1, hh, 0.004), R.p(a1, hh + 0.016, 0.004), R.p(a0, hh + 0.016, 0.004),
             AR_ESCURO, n=R.d(0, 0, 1), div=(2, 1))
    # a roda: o aro, o fundo e as lâminas diagonais
    cm = (-0.33, (h0 + h1) / 2)
    rr = 0.36
    disco(CORES, R.p(cm[0], cm[1], 0.006), R.d(0, 0, 1), R.d(1, 0, 0), rr - 0.04, 32, escurecer(AR_COR, 0.66))
    torno(CORES, R.p(cm[0], cm[1], 0.0), R.d(0, 0, 1), R.d(1, 0, 0),
          [(rr, 0.0), (rr + 0.005, 0.03), (rr - 0.02, 0.045), (rr - 0.05, 0.03), (rr - 0.055, 0.0)], 32, escurecer(AR_COR, 1.04))
    d = unit(np.array([1.0, 1.0]))
    for k in range(-5, 6):
        o = k * 0.055
        meia = math.sqrt(max((rr - 0.05) ** 2 - o * o, 0.0))
        if meia < 0.03:
            continue
        c = np.array(cm) + o * np.array([d[1], -d[0]])
        p = R.p(c[0] - d[0] * meia, c[1] - d[1] * meia, 0.02)
        q = R.p(c[0] + d[0] * meia, c[1] + d[1] * meia, 0.02)
        barra(CORES, p, q, 0.03, 0.026, R.ez, escurecer(AR_COR, 0.97))
    # o painel dos botões
    quad(CORES, R.p(0.46, h0 + 0.12, 0.004), R.p(L - 0.12, h0 + 0.12, 0.004), R.p(L - 0.12, h1 - 0.12, 0.004),
         R.p(0.46, h1 - 0.12, 0.004), (0.80, 0.77, 0.70), n=R.d(0, 0, 1), div=(1, 2))
    am = (0.46 + L - 0.12) / 2
    for hh in (h0 + 0.70, h0 + 0.44):
        torno(CORES, R.p(am, hh, 0.004), R.d(0, 0, 1), R.d(1, 0, 0), [(0.085, 0.0), (0.085, 0.045), (0.075, 0.06), (0.0, 0.065)], 20,
              (0.86, 0.62, 0.38))
        quad(CORES, R.p(am - 0.008, hh, 0.07), R.p(am + 0.008, hh, 0.07), R.p(am + 0.008, hh + 0.07, 0.07), R.p(am - 0.008, hh + 0.07, 0.07),
             (0.30, 0.18, 0.12), n=R.d(0, 0, 1), div=(1, 1))
    caixa(CORES, R, am - 0.03, am + 0.03, h0 + 0.17, h0 + 0.27, 0.0, 0.035, (0.62, 0.16, 0.12), sem=('tras',), passo=1.0)
    # o relógio de cone em cima (o mostrador vem do atlas)
    ac, bc = -0.26, -0.30
    caixa(CORES, R, ac - 0.22, ac + 0.22, h1, h1 + 0.045, bc - 0.22, bc + 0.22, LARANJA_ESCURO, sem=('baixo',), passo=1.0)
    base = R.p(ac, h1 + 0.045, bc)
    alto = 0.74
    perfil = [(0.19, 0.0), (0.05, alto), (0.0, alto + 0.01)]
    torno(CORES, base, R.d(0, 1, 0), R.d(1, 0, 0), perfil, 24, LARANJA)

    def raio_cone(hh):
        return 0.19 + (0.05 - 0.19) * hh / alto
    # o mostrador
    hm = 0.56
    rm = raio_cone(hm) + 0.004
    u0, v0, u1, v1 = uv_da_celula(CELULA_DO_RELOGIO)
    cen = R.p(ac, h1 + 0.045 + hm, bc + rm)
    pts = [(math.cos(a), math.sin(a)) for a in np.linspace(0, 2 * math.pi, 21)]
    for k in range(20):
        (xa, ya), (xb, yb) = pts[k], pts[k + 1]
        tri = [cen, cen + 0.072 * (xa * R.ex + ya * R.ey), cen + 0.072 * (xb * R.ex + yb * R.ey)]
        uvs = [((u0 + u1) / 2, (v0 + v1) / 2), (u0 + (u1 - u0) * (0.5 + 0.5 * xa), v0 + (v1 - v0) * (0.5 + 0.5 * ya)),
               (u0 + (u1 - u0) * (0.5 + 0.5 * xb), v0 + (v1 - v0) * (0.5 + 0.5 * yb))]
        MALHAS['quadros'].face(tri, BRANCO, n=R.ez, uvs=uvs)
    # a plaquinha e o painel creme na frente do cone (acompanhando a curva dele)
    for hha, hhb, larg, cor in ((0.40, 0.46, 0.10, CREME), (0.08, 0.34, 0.15, CREME)):
        for i in range(3):
            fa, fb = -larg / 2 + larg * i / 3, -larg / 2 + larg * (i + 1) / 3
            def pt(f, hh):
                r = raio_cone(hh) + 0.006
                ang = f / r
                return R.p(ac + r * math.sin(ang), h1 + 0.045 + hh, bc + r * math.cos(ang))
            CORES.face([pt(fa, hha), pt(fb, hha), pt(fb, hhb), pt(fa, hhb)], cor, n=R.ez)
    # o que o carro não atravessa
    Q.caixas_de_colisao.append(([np.array(c) for c in (
        meio + (-L) * unit(AR_CASCA_B - AR_CASCA_A), meio + L * unit(AR_CASCA_B - AR_CASCA_A),
        meio + L * unit(AR_CASCA_B - AR_CASCA_A) + 1.2 * AR_CASCA_FORA, meio - L * unit(AR_CASCA_B - AR_CASCA_A) + 1.2 * AR_CASCA_FORA)], 1.7))


def fundo_do_ar(h, meio, L):
    """Até onde (metros para fora, a partir da frente) vai a caixa do ar-condicionado, h acima do piso: o
    meio do fundo atrás da nossa parede (para não se ver o oco da caixa) e as pontas dentro da casca (para
    ela não aparecer do lado de fora). Fica no meio do que serve"""
    fora = unit(AR_CASCA_FORA)
    t = np.array([-fora[1], fora[0]])
    serve = [b for b in np.linspace(0.3, 1.6, 261)
             if np.linalg.norm(meio + b * fora) >= raio_da_parede(h) + 0.03 and
             np.linalg.norm(meio + b * fora + L * t) <= raio_da_casca(h) - 0.04 and
             np.linalg.norm(meio + b * fora - L * t) <= raio_da_casca(h) - 0.04]
    if not serve:
        raise SystemExit('ERRO: a caixa do ar-condicionado não cabe entre a parede e a casca em h %.2f' % h)
    return (min(serve) + max(serve)) / 2


def cupula(centro, eixo, e1):
    """Uma cúpula de cone laranja (com a faixa creme), acesa por dentro, e a lâmpada"""
    comp = 0.46
    torno(CORES, centro, eixo, e1, [(0.045, 0.0), (0.14, comp * 0.62)], 20, LARANJA)
    torno(CORES, centro, eixo, e1, [(0.14, comp * 0.62), (0.165, comp * 0.78)], 20, CREME)
    torno(CORES, centro, eixo, e1, [(0.165, comp * 0.78), (0.20, comp)], 20, LARANJA)
    torno(CORES, centro, eixo, e1, [(0.20, comp), (0.21, comp + 0.012), (0.19, comp + 0.012)], 20, LARANJA_ESCURO)
    torno(CORES, centro, eixo, e1, [(0.19, comp), (0.04, 0.02)], 20, LUZ_DA_CUPULA, brilho=LUZ_DA_CUPULA)
    eixo = unit(eixo)
    lamp = np.asarray(centro) + eixo * 0.16
    torno(CORES, lamp, eixo, e1, [(0.0, -0.06), (0.045, -0.035), (0.055, 0.0), (0.045, 0.035), (0.0, 0.06)], 12,
          LUZ_DA_LAMPADA, brilho=LUZ_DA_LAMPADA)
    return lamp


def montar_luminaria(Q):
    """A luminária de pé: a base, a haste de latão e duas cúpulas de cone nos braços"""
    th, r = math.radians(LUMINARIA[0]), LUMINARIA[1]
    u, v = r * math.cos(th), r * math.sin(th)
    R = ref_no_chao(Q, u, v, -math.cos(th), -math.sin(th))
    torno(CORES, R.p(0, 0, 0), R.ey, R.ex, [(0.22, 0.0), (0.22, 0.025), (0.18, 0.05), (0.04, 0.065), (0.0, 0.065)], 24,
          (0.30, 0.22, 0.16), metal=True)
    alto = 3.30
    torno(CORES, R.p(0, 0, 0), R.ey, R.ex, [(0.022, 0.06), (0.022, alto), (0.03, alto + 0.03), (0.0, alto + 0.06)], 12, LATAO, metal=True)
    for lado, hh in ((1, 3.05), (-1, 2.30)):
        p0 = R.p(0, hh, 0)
        p1 = R.p(lado * 0.26, hh + 0.10, 0.06)
        barra(CORES, p0, p1, 0.022, 0.022, R.ey, LATAO, metal=True)
        eixo = unit(R.d(lado * 0.62, -0.72, 0.30))
        Q.luzes.append(('abajur', cupula(p1, eixo, R.ey), eixo))
        # a cúpula acesa clareia a parede atrás dela (uma luz grande, entre a cúpula e a parede)
        Q.luzes.append(('cupula', p1 + eixo * 0.23 - R.ez * 0.24 + R.ey * 0.04, None))
    Q.caixas_de_colisao.append(([np.array([u, v]) + 0.32 * np.array(c) for c in ((-1, -1), (1, -1), (1, 1), (-1, 1))], 1.6))
    Q.escondidos.append((Ref(R.o, R.ex, R.ez, -R.ey), [[(-0.19, -0.19), (0.19, -0.19), (0.19, 0.19), (-0.19, 0.19)]], -0.02, 0.03))


def rosa(centro, cima, e1, tam):
    k = tam
    torno(CORES, centro, cima, e1, [(0.012 * k, -0.01 * k), (0.05 * k, 0.0), (0.062 * k, 0.03 * k), (0.055 * k, 0.055 * k), (0.035 * k, 0.06 * k)],
          9, [ROSA_ESCURA, ROSA_VERMELHA, ROSA_VERMELHA, misturar(ROSA_VERMELHA, (1, 0.5, 0.5), 0.25)])
    torno(CORES, np.asarray(centro) + 0.01 * k * unit(cima), cima, e1, [(0.034 * k, 0.03 * k), (0.038 * k, 0.05 * k), (0.022 * k, 0.058 * k), (0.0, 0.052 * k)], 7,
          [ROSA_VERMELHA, ROSA_ESCURA, ROSA_ESCURA])


def montar_mesinha(Q):
    """A mesinha anos 50 de pés abertos, com o vaso de rosas, o conezinho, o carrinho e, no cone do
    McQueen, o troféu da Copa Pistão"""
    th, r = math.radians(MESA[0]), MESA[1]
    u, v = r * math.cos(th), r * math.sin(th)
    R = ref_no_chao(Q, u, v, -math.cos(th), -math.sin(th))
    a, bb, h = 0.86, 0.33, 0.70
    caixa(CORES, R, -a, a, h, h + 0.05, -bb, bb, MADEIRA, cores={'cima': MADEIRA_CLARA}, passo=0.3)
    for sa in (-1, 1):
        for sb in (-1, 1):
            p = R.p(sa * (a - 0.16), h, sb * (bb - 0.08))
            q = R.p(sa * (a - 0.02), 0.0, sb * (bb + 0.02))
            barra(CORES, p, q, 0.045, 0.045, R.ez, escurecer(MADEIRA, 0.8))
            barra(CORES, q + (p - q) * 0.12, q, 0.05, 0.05, R.ez, LATAO, metal=True)
    topo = h + 0.05
    # o vaso (cone de barro) e as rosas
    va = 0.50
    torno(CORES, R.p(va, topo, 0), R.ey, R.ex, [(0.10, 0.0), (0.12, 0.03), (0.17, 0.40), (0.19, 0.42), (0.185, 0.44), (0.16, 0.44)],
          20, TERRACOTA)
    disco(CORES, R.p(va, topo + 0.39, 0), R.ey, R.ex, 0.165, 16, (0.22, 0.13, 0.08))
    rng = np.random.default_rng(5)
    for k in range(8):
        ang = k * 2.4
        rr = 0.04 + 0.09 * math.sqrt(k / 8)
        pa, pb = rr * math.cos(ang), rr * math.sin(ang)
        hh = topo + 0.66 + 0.12 * rng.uniform() - 0.06 * rr / 0.13
        flor = R.p(va + pa * 1.6, hh, pb * 1.6)
        barra(CORES, R.p(va + pa * 0.5, topo + 0.38, pb * 0.5), flor, 0.015, 0.015, R.ez, CAULE)
        cima = unit(R.d(pa * 3, 1.0, pb * 3))
        rosa(flor, cima, R.ex, 1.45)
        for s in (-1, 1):                               # as folhas
            meio = R.p(va + pa * 1.1, hh - 0.18, pb * 1.1)
            dirf = unit(R.d(s * math.cos(ang + 0.9), 0.35, s * math.sin(ang + 0.9)))
            lado = unit(np.cross(dirf, R.ey))
            CORES.face([meio, meio + dirf * 0.07 + lado * 0.035, meio + dirf * 0.16, meio + dirf * 0.07 - lado * 0.035], FOLHA, n=R.ey)
    # o conezinho e o carrinho verde
    ca = -0.12
    caixa(CORES, R, ca - 0.08, ca + 0.08, topo, topo + 0.02, -0.08, 0.08, LARANJA_ESCURO, sem=('baixo',), passo=1.0)
    torno(CORES, R.p(ca, topo + 0.02, 0), R.ey, R.ex, [(0.065, 0.0), (0.045, 0.09)], 14, LARANJA)
    torno(CORES, R.p(ca, topo + 0.02, 0), R.ey, R.ex, [(0.045, 0.09), (0.035, 0.13)], 14, CREME)
    torno(CORES, R.p(ca, topo + 0.02, 0), R.ey, R.ex, [(0.035, 0.13), (0.012, 0.23), (0.0, 0.235)], 14, LARANJA)
    cc = -0.52
    verde = (0.36, 0.60, 0.30)
    caixa(CORES, R, cc - 0.11, cc + 0.11, topo + 0.025, topo + 0.07, -0.05, 0.05, verde, passo=1.0)
    caixa(CORES, R, cc - 0.05, cc + 0.04, topo + 0.07, topo + 0.105, -0.045, 0.045, verde, passo=1.0,
          cores={'frente': (0.55, 0.75, 0.85), 'esq': (0.55, 0.75, 0.85)})
    for sa in (-1, 1):
        for sb in (-1, 1):
            disco(CORES, R.p(cc + sa * 0.07, topo + 0.025, sb * 0.052), R.d(0, 0, sb), R.ex, 0.025, 10, PRETO)
    if Q.mcqueen:
        montar_trofeu(R, 0.15, topo)
    Q.caixas_de_colisao.append(([np.array([u, v]) + np.array([c[0], c[1]]) for c in (
        Q_local(R, Q, -a - 0.05, -bb - 0.06), Q_local(R, Q, a + 0.05, -bb - 0.06), Q_local(R, Q, a + 0.05, bb + 0.06),
        Q_local(R, Q, -a - 0.05, bb + 0.06))], 1.3))


def Q_local(R, Q, a, b):
    """O deslocamento (du, dv) do quarto de um ponto (a, b) do referencial R (no chão)"""
    d = R.d(a, 0, b)
    du = float(np.dot([d[0], d[2]], Q.f))
    dv = float(np.dot([d[0], d[2]], Q.d)) * Q.s
    return (du, dv)


def montar_trofeu(R, a, topo):
    """Um troféu pequeno da Copa Pistão: a base preta, a taça dourada de alças grandes"""
    caixa(CORES, R, a - 0.08, a + 0.08, topo, topo + 0.06, -0.08, 0.08, PRETO, sem=('baixo',), passo=1.0)
    caixa(CORES, R, a - 0.065, a + 0.065, topo + 0.06, topo + 0.09, -0.065, 0.065, (0.28, 0.16, 0.10), sem=('baixo',), passo=1.0)
    base = R.p(a, topo + 0.09, 0)
    perfil = [(0.05, 0.0), (0.05, 0.015), (0.022, 0.03), (0.016, 0.10), (0.03, 0.12), (0.06, 0.15), (0.095, 0.22),
              (0.105, 0.29), (0.10, 0.30), (0.0, 0.30)]
    torno(CORES, base, R.ey, R.ex, perfil, 20, OURO, metal=True)
    for s in (-1, 1):
        pts = [R.p(a + s * (0.095 + 0.07 * math.sin(t)), topo + 0.09 + 0.21 + 0.06 * math.cos(t), 0) for t in np.linspace(0, math.pi, 7)]
        for p, q in zip(pts[:-1], pts[1:]):
            barra(CORES, p, q, 0.016, 0.016, R.ez, OURO, metal=True)


def montar_tapetes(Q):
    """Os dois tapetes compridos de fibra de coco, um embaixo de cada lado do carro: uma caixa de 2,5 cm com o
    chanfro em volta e os lados mais escuros, a beira trançada nas quatro beiradas (nas pontas, a faixa da
    textura vai atravessada). Sem colisão: o carro passa por cima"""
    u0, u1 = TAPETES_U
    e, c, t = ALTO_DO_TAPETE, CHANFRO_DO_TAPETE, TRAMA_DO_TAPETE
    cap = MALHAS['capacho']
    cima = Q.vet(0, 1, 0)
    faixa = TRAMA_DO_TAPETE / LARGURA_DO_TAPETE       # a faixa trançada, na textura (v de 0 a isto)
    cor_chanfro, cor_lado = (0.52, 0.37, 0.21), (0.33, 0.22, 0.12)
    for s in (-1, 1):
        va, vb = s * TAPETES_V - LARGURA_DO_TAPETE / 2, s * TAPETES_V + LARGURA_DO_TAPETE / 2
        ua, ub, wa, wb = u0 + c, u1 - c, va + c, vb - c
        largura = wb - wa
        nv = 2
        ws = [wa + largura * j / nv for j in range(nv + 1)]
        # o meio do tampo: os tufos, com as beiras compridas trançadas (já na textura)
        n = max(1, math.ceil((ub - ua - 2 * t) / 0.25 - 1e-6))
        us = [ua + t + (ub - ua - 2 * t) * i / n for i in range(n + 1)]
        for i in range(n):
            for j in range(nv):
                pts = [(us[i], ws[j]), (us[i + 1], ws[j]), (us[i + 1], ws[j + 1]), (us[i], ws[j + 1])]
                cap.face([Q.p(a, e, b) for a, b in pts], BRANCO, n=cima, sup=SUP_TAPETE,
                         uvs=[((a - u0) / METROS_DO_CAPACHO, (b - wa) / largura) for a, b in pts])
        # as pontas: a faixa trançada atravessada (a beira de fora é o v 0 da textura)
        for ponta, dentro in ((ua, ua + t), (ub, ub - t)):
            for j in range(nv):
                pts = [(ponta, ws[j]), (dentro, ws[j]), (dentro, ws[j + 1]), (ponta, ws[j + 1])]
                cap.face([Q.p(a, e, b) for a, b in pts], BRANCO, n=cima, sup=SUP_TAPETE,
                         uvs=[((b - wa) / METROS_DO_CAPACHO, abs(a - ponta) / t * faixa) for a, b in pts])
        # o chanfro e os lados
        cantos_cima = [(ua, wa), (ub, wa), (ub, wb), (ua, wb)]
        cantos_fora = [(u0, va), (u1, va), (u1, vb), (u0, vb)]
        normais = [(0, 0, -1), (1, 0, 0), (0, 0, 1), (-1, 0, 0)]
        for k in range(4):
            a0, a1 = cantos_cima[k], cantos_cima[(k + 1) % 4]
            b0, b1 = cantos_fora[k], cantos_fora[(k + 1) % 4]
            nn = np.array(normais[k], float)
            quad(CORES, Q.p(b0[0], e - c, b0[1]), Q.p(b1[0], e - c, b1[1]), Q.p(a1[0], e, a1[1]), Q.p(a0[0], e, a0[1]),
                 cor_chanfro, n=Q.vc(nn + np.array([0, 1.0, 0])), passo=0.5)
            quad(CORES, Q.p(b0[0], 0.0, b0[1]), Q.p(b1[0], 0.0, b1[1]), Q.p(b1[0], e - c, b1[1]), Q.p(b0[0], e - c, b0[1]),
                 cor_lado, n=Q.vc(nn), passo=0.5)
        # o carpete embaixo fica escondido
        k = 0.02
        Q.escondidos.append((Ref(Q.p(0, 0, 0), Q.vet(1, 0, 0), Q.vet(0, 0, 1), Q.vet(0, -1, 0)),
                             [[(u0 + k, va + k), (u1 - k, va + k), (u1 - k, vb - k), (u0 + k, vb - k)]], -0.02, 0.03))


def montar_perto_da_porta(Q):
    """A luminária de cone pequena (acesa) e o interruptor de chão, no canto entre a parede da porta e o
    ar-condicionado (fora do caminho do carro, que não tem colisão para eles)"""
    R = ref_no_chao(Q, CONEZINHO_DA_PORTA[0], CONEZINHO_DA_PORTA[1], -1.0, 0.3)
    caixa(CORES, R, -0.12, 0.12, 0.0, 0.025, -0.12, 0.12, (0.30, 0.16, 0.10), sem=('baixo',), passo=1.0)
    torno(CORES, R.p(0, 0.025, 0), R.ey, R.ex, [(0.10, 0.0), (0.075, 0.12), (0.06, 0.16), (0.03, 0.30), (0.0, 0.31)], 16,
          LUZ_DO_CONEZINHO, brilho=LUZ_DO_CONEZINHO)
    Q.luzes.append(('conezinho', R.p(0, 0.45, 0.0), None))
    Q.escondidos.append((Ref(R.o, R.ex, R.ez, -R.ey), [[(-0.11, -0.11), (0.11, -0.11), (0.11, 0.11), (-0.11, 0.11)]], -0.02, 0.03))
    R = ref_no_chao(Q, INTERRUPTOR[0], INTERRUPTOR[1], -1.0, 0.25)
    caixa(CORES, R, -0.20, 0.20, 0.0, 0.025, -0.12, 0.12, CREME, sem=('baixo',), passo=1.0)
    caixa(CORES, R, -0.14, 0.14, 0.025, 0.09, -0.08, 0.06, (0.80, 0.74, 0.62), sem=('baixo',), passo=1.0)
    quad(CORES, R.p(-0.14, 0.09, -0.08), R.p(0.14, 0.09, -0.08), R.p(0.14, 0.13, 0.03), R.p(-0.14, 0.13, 0.03),
         (0.74, 0.20, 0.14), n=R.d(0, 1, 0.5), div=(1, 1))
    for s in (-1, 1):
        CORES.face([R.p(s * 0.14, 0.09, -0.08), R.p(s * 0.14, 0.13, 0.03), R.p(s * 0.14, 0.09, 0.06)], (0.74, 0.20, 0.14), n=R.d(s, 0, 0))
    caixa(CORES, R, -0.05, 0.05, 0.10, 0.13, 0.04, 0.07, METAL, sem=('baixo',), passo=1.0)
    Q.escondidos.append((Ref(R.o, R.ex, R.ez, -R.ey), [[(-0.19, -0.11), (0.19, -0.11), (0.19, 0.11), (-0.19, 0.11)]], -0.02, 0.03))


PE_DO_QUADRO = 0.10        # a tábua do pé (a base do cone): a altura...
SOBRA_DO_PE = 0.21         # ...e quanto ela passa da pintura de cada lado
TOPO_DO_QUADRO = 0.10      # a largura do alto da pintura (fração da de baixo): um "A" estreito, como no filme
LARGURA_DA_MOLDURA = 0.075


def largura_do_quadro(alto):
    """A largura da tábua do pé de um quadro de cone com 'alto' metros"""
    return (alto - PE_DO_QUADRO - 0.08) * ASPECTO_DO_QUADRO + 2 * SOBRA_DO_PE


def quadro_de_cone(R, alto, celula):
    """Um quadro em moldura de cone: o "A" estreito (o corpo do cone) com a pintura, a moldura de madeira
    grossa e, embaixo, a tábua do pé, mais larga. O meio do quadro fica em (0, 0) de R."""
    pe = PE_DO_QUADRO
    hp = alto - pe - 0.08
    wp = hp * ASPECTO_DO_QUADRO
    y0 = -alto / 2
    yb = y0 + pe
    yt = yb + hp
    topo = TOPO_DO_QUADRO
    m = LARGURA_DA_MOLDURA
    # o pé
    caixa(CORES, R, -wp / 2 - SOBRA_DO_PE, wp / 2 + SOBRA_DO_PE, y0, yb, 0.0, 0.055, MOLDURA, sem=('tras',), passo=1.0,
          cores={'cima': MOLDURA_CLARA, 'baixo': escurecer(MOLDURA, 0.7)})
    # a pintura
    u0, v0, u1, v1 = uv_da_celula(celula)
    ns, nt = 4, 8
    for i in range(nt):
        for j in range(ns):
            pts, uvs = [], []
            for s, t in ((j / ns, i / nt), ((j + 1) / ns, i / nt), ((j + 1) / ns, (i + 1) / nt), (j / ns, (i + 1) / nt)):
                meia = wp / 2 * (1 - (1 - topo) * t)
                x = (s - 0.5) * 2 * meia
                pts.append(R.p(x, yb + t * hp, 0.012))
                uvs.append((u0 + (u1 - u0) * (x / wp + 0.5), v0 + (v1 - v0) * t))
            MALHAS['quadros'].face(pts, BRANCO, n=R.ez, uvs=uvs)
    # a moldura: os dois lados inclinados e o alto
    dentro = [(-wp / 2, yb), (-wp / 2 * topo, yt), (wp / 2 * topo, yt), (wp / 2, yb)]

    def desloca(p, q, d):
        e = unit(np.subtract(q, p))
        n = np.array([-e[1], e[0]])        # para fora (o contorno vai no sentido horário em x, y)
        return np.array(p) + n * d, np.array(q) + n * d

    linhas = [desloca(dentro[k], dentro[k + 1], m) for k in range(3)]

    def cruza(l1, l2):
        (p1, q1), (p2, q2) = [(np.asarray(x, float), np.asarray(y, float)) for x, y in (l1, l2)]
        d1, d2 = q1 - p1, q2 - p2
        A = np.array([[d1[0], -d2[0]], [d1[1], -d2[1]]])
        t = np.linalg.solve(A, p2 - p1)
        return p1 + d1 * t[0]
    fora = [cruza(linhas[0], ((-9, yb), (9, yb))), cruza(linhas[0], linhas[1]), cruza(linhas[1], linhas[2]),
            cruza(linhas[2], ((-9, yb), (9, yb)))]
    e = 0.05
    for k in range(3):
        a, b, c, d = dentro[k], dentro[k + 1], fora[k + 1], fora[k]
        quad(CORES, R.p(a[0], a[1], e), R.p(b[0], b[1], e), R.p(c[0], c[1], e), R.p(d[0], d[1], e), MOLDURA, n=R.ez, div=(1, 1))
        quad(CORES, R.p(a[0], a[1], 0.012), R.p(b[0], b[1], 0.012), R.p(b[0], b[1], e), R.p(a[0], a[1], e), MOLDURA_CLARA,
             n=R.d(-(b[1] - a[1]), b[0] - a[0], 0) * -1, div=(1, 1))
        quad(CORES, R.p(d[0], d[1], 0.0), R.p(c[0], c[1], 0.0), R.p(c[0], c[1], e), R.p(d[0], d[1], e), escurecer(MOLDURA, 0.8),
             n=R.d(c[1] - d[1], -(c[0] - d[0]), 0) * -1, div=(1, 1))
    # o que ele esconde da parede atrás (o pé e o "A", 2 cm para dentro da beira), em (a, y) de R
    k = 0.02
    pe_ = [(-wp / 2 - SOBRA_DO_PE + k, y0 + k), (wp / 2 + SOBRA_DO_PE - k, y0 + k), (wp / 2 + SOBRA_DO_PE - k, yb), (-wp / 2 - SOBRA_DO_PE + k, yb)]
    meio_x = 0.0
    a_ = [tuple(np.array(p) + k * unit(np.array([meio_x, (yb + yt) / 2]) - np.array(p))) for p in (fora[0], fora[3], fora[2], fora[1])]
    return wp + 2 * SOBRA_DO_PE, [pe_, a_]


def montar_quadros(Q, pinturas):
    for (graus, hm, alto), celula in zip(QUADROS_NA_PAREDE, pinturas):
        th = math.radians(graus)
        r = raio_da_parede(hm)
        larg = largura_do_quadro(alto)
        afasta = (larg / 2) ** 2 / (2 * r) + 0.006
        R = ref_na_parede(Q, th, hm, afasta)
        _, contornos = quadro_de_cone(R, alto, celula)
        Q.escondidos.append((R, contornos, -afasta - 0.06, 0.06))
    # na parede da porta, à esquerda de quem olha de dentro: o pôster do McQueen ou mais um quadro
    v, hm = QUADRO_DA_FRENTE
    R = ref_de(Q.p(U_FRENTE - 0.003, hm, v), Q.vet(-1.0, 0.0, 0.0))
    if Q.mcqueen:
        w, h = 0.66, 0.99
        caixa(CORES, R, -w / 2 - 0.03, w / 2 + 0.03, -h / 2 - 0.03, h / 2 + 0.03, 0.0, 0.03, (0.12, 0.10, 0.10), sem=('tras',), passo=1.0)
        u0, v0, u1, v1 = uv_da_celula(CELULA_DO_POSTER)
        quad(MALHAS['quadros'], R.p(-w / 2, -h / 2, 0.034), R.p(w / 2, -h / 2, 0.034), R.p(w / 2, h / 2, 0.034), R.p(-w / 2, h / 2, 0.034),
             BRANCO, n=R.ez, uv=[(u0, v0), (u1, v0), (u1, v1), (u0, v1)], div=(2, 3))
        k = w / 2 + 0.01
        Q.escondidos.append((R, [[(-k, -h / 2 - 0.01), (k, -h / 2 - 0.01), (k, h / 2 + 0.01), (-k, h / 2 + 0.01)]], -0.05, 0.06))
    else:
        _, contornos = quadro_de_cone(R, 1.15, pinturas[-1])
        Q.escondidos.append((R, contornos, -0.05, 0.06))


# ------------------------------------------------------------------
# 3d. Um quarto inteiro
# ------------------------------------------------------------------
def montar_quarto(Q):
    janelas = [Janela(JANELA_DO_AR), Janela(JANELA_DA_MESA)]
    montar_parede_de_cone(Q, janelas)
    montar_janela(Q, janelas[0], True)
    montar_janela(Q, janelas[1], False)
    montar_parede_da_frente(Q)
    montar_piso_e_teto(Q)
    perfil_em_volta(Q, PERFIL_DO_RODAPE, RODAPE)
    perfil_em_volta(Q, PERFIL_DO_FRISO, FRISO)
    perfil_em_volta(Q, PERFIL_DA_SANCA, SANCA, porta=False)
    montar_friso_de_dentes(Q)
    montar_porta(Q)
    montar_ar_condicionado(Q)
    montar_luminaria(Q)
    montar_mesinha(Q)
    montar_tapetes(Q)
    montar_perto_da_porta(Q)
    montar_quadros(Q, PINTURAS_DOS_CONES[Q.numero])
    Q.janelas = janelas


print('Montando os quartos...')
QUARTOS = []
for numero, eixo, rumo, espelho in CONES:
    QUARTO_ATUAL[0] = numero
    Q = Quarto(numero, eixo, rumo, espelho)
    montar_quarto(Q)
    QUARTOS.append(Q)
    print('  cone %d%s' % (numero, ' (espelhado)' if espelho else ''))
print('  %.0f s' % (time.time() - INICIO))


# ------------------------------------------------------------------
# 3e. A pele do cone 4: a casca e a base que a etapa da arena cortou no mapa montado
# ------------------------------------------------------------------
CONE_DA_PELE = 4
CONE_INTEIRO = 1             # o molde: um cone inteiro no mapa, virado do mesmo jeito (não espelhado)
ALTO_DA_BEIRA_DA_BASE = 1.30 # m: a colisão da beira da base devolvida (paredes em pé, do pé dela para cima)
PAREDES_DA_BASE = []         # os triângulos dessa colisão (eixos do jogo)
PELE = {}                    # o que se mediu (para o json)
# O jogo desenha o mapa com o shader dos mapas de dia (src/mundo.c): a textura vezes uma luz que depende da
# direção da face (luz = 0,58 + 0,30 (n.y/2 + 1/2) + 0,38 max(n·sol, 0), vezes 0,92). Os quartos vão com o
# shader padrão, sem luz: a pele leva essa luz na cor do vértice, face por face, para ficar igual às vizinhas
SOL_DO_MAPA = (0.35, 0.8, 0.45)
# De noite (--noite), a mesma conta com os multiplicadores da meia-noite do jogo (src/hora.c, a chave das 0h) e
# a luz vindo da lua: luz = (0,58 base + 0,30 (n.y/2 + 1/2) céu + 0,38 max(n·lua, 0) luar) × 0,92, por canal
LUA_DO_MAPA = (0.25, 0.75, -0.6)
BASE_DA_NOITE_DO_MAPA = (0.34, 0.35, 0.53)
CEU_DA_NOITE_DO_MAPA = (0.23, 0.25, 0.45)
LUAR_DO_MAPA = (0.24, 0.28, 0.46)


def luz_do_mapa(n):
    """A luz que o shader dos mapas de dia dá a uma face de normal n (eixos do jogo)"""
    n = unit(n)
    return (0.58 + 0.30 * (n[1] * 0.5 + 0.5) + 0.38 * max(float(np.dot(n, unit(SOL_DO_MAPA))), 0.0)) * 0.92


def luz_do_mapa_de_noite(n):
    """A luz (r, g, b) que o shader dos mapas dá à meia-noite a uma face de normal n (eixos do jogo)"""
    n = unit(n)
    lua = max(float(np.dot(n, unit(LUA_DO_MAPA))), 0.0)
    return (0.58 * np.array(BASE_DA_NOITE_DO_MAPA) + 0.30 * (n[1] * 0.5 + 0.5) * np.array(CEU_DA_NOITE_DO_MAPA)
            + 0.38 * lua * np.array(LUAR_DO_MAPA)) * 0.92


def ler_casca_do_mapa():
    """Do mapa montado, a casca dos cones (ror_Object_61): os pontos (eixos do jogo), as faces, as coordenadas
    de textura de cada canto e a textura (na tela, a linha 0 em cima). Tira do Blender tudo o que importou.
    Devolve None sem o mapa."""
    if not os.path.exists(ARQUIVO_DO_MAPA):
        return None
    colecoes = (bpy.data.objects, bpy.data.meshes, bpy.data.materials, bpy.data.images, bpy.data.cameras,
                bpy.data.lights, bpy.data.collections, bpy.data.node_groups)
    antes = [set(c) for c in colecoes]
    bpy.ops.import_scene.gltf(filepath=ARQUIVO_DO_MAPA)
    o = next((x for x in bpy.data.objects if x.name.startswith('ror_Object_61') and x.type == 'MESH'), None)
    casca = None
    if o is not None:
        me = o.data
        mw = np.array(o.matrix_world)
        co = np.empty(len(me.vertices) * 3)
        me.vertices.foreach_get('co', co)
        co = co.reshape(-1, 3) @ mw[:3, :3].T + mw[:3, 3]
        G = np.stack([co[:, 0], co[:, 2], -co[:, 1]], 1)
        uv = np.empty(len(me.loops) * 2)
        me.uv_layers.active.data.foreach_get('uv', uv)
        uv = uv.reshape(-1, 2)
        faces = [(list(p.vertices), [uv[k] for k in p.loop_indices]) for p in me.polygons]
        img = next(n.image for n in me.materials[0].node_tree.nodes if n.type == 'TEX_IMAGE')
        w, h = img.size
        px = np.empty(w * h * 4, np.float32)
        img.pixels.foreach_get(px)
        casca = dict(G=G, faces=faces, textura=np.flipud(px.reshape(h, w, 4)[..., :3]).copy())
    for c, a in zip(colecoes, antes):
        for d in [d for d in c if d not in a]:
            c.remove(d)
    return casca


def montar_pele_do_cone_4(casca):
    """Põe de volta as faces da casca (e da base) do cone 4 que faltam no mapa: as do cone inteiro, no mesmo
    lugar em relação ao eixo e à porta, que não têm par no cone 4 (sem os vidros e as lascas das janelas,
    que saem pelas aberturas). Os cantos que caem a menos de 6 cm de um do cone 4 vão para ele (sem frestas).
    Elas levam a textura da casca (uma cópia, 64 x 64) com as mesmas coordenadas e a cor do vértice branca:
    ficam iguais às vizinhas (não são assadas). A beira da base devolvida entra na colisão (como nos outros
    cones)"""
    alvo = next(Q for Q in QUARTOS if Q.numero == CONE_DA_PELE)
    molde = next(Q for Q in QUARTOS if Q.numero == CONE_INTEIRO)
    if alvo.espelho != molde.espelho:
        raise SystemExit('ERRO: o molde da pele tem de estar virado como o cone %d' % CONE_DA_PELE)
    G = casca['G']

    def local(Q, P):
        q = P[..., [0, 2]] - Q.eixo
        return np.stack([q @ Q.f, P[..., 1], q @ Q.d], -1)

    def mundo(Q, L):
        xz = Q.eixo + L[..., [0]] * Q.f + L[..., [2]] * Q.d
        return np.stack([xz[..., 0], L[..., 1], xz[..., 1]], -1)
    centros = np.array([G[f].mean(0) for f, _ in casca['faces']])
    Lm, La = local(molde, centros), local(alvo, centros)
    do_molde = np.flatnonzero(np.hypot(Lm[:, 0], Lm[:, 2]) < 12.0)
    do_alvo = np.flatnonzero(np.hypot(La[:, 0], La[:, 2]) < 12.0)
    pontos_do_alvo = G[sorted({v for i in do_alvo for v in casca['faces'][i][0]})]
    caixas = [(np.minimum(mn, mx) - 0.05, np.maximum(mn, mx) + 0.05) for cone, _, mn, mx, _ in ABERTURAS_DA_CASCA
              if cone == CONE_DA_PELE for mn, mx in [(np.array(mn), np.array(mx))]]
    faltam = []
    for i in do_molde:
        if np.min(np.linalg.norm(La[do_alvo] - Lm[i], axis=1)) < 0.06:
            continue                                   # o cone 4 tem esta face
        P = mundo(alvo, local(molde, G[casca['faces'][i][0]]))
        if any(np.all((P >= a) & (P <= b)) for a, b in caixas):
            continue                                   # o vidro e a lasca da janela: saem pelas aberturas
        faltam.append((i, P))
    MALHAS['pele'] = Malha('cozy_casca', imagem('cozy_casca', casca['textura']))
    QUARTO_ATUAL[0] = CONE_DA_PELE
    MARCA_ATUAL[0] = MARCA_PELE
    soldados = 0
    tri = 0
    for i, P in faltam:
        # os cantos perto de um do cone 4 vão para ele
        for k in range(len(P)):
            d = np.linalg.norm(pontos_do_alvo - P[k], axis=1)
            j = int(np.argmin(d))
            if d[j] < 0.06:
                P[k] = pontos_do_alvo[j]
                soldados += 1
        # a face como no mapa: a textura da casca com as mesmas coordenadas e, na cor do vértice, a luz que o
        # shader do mapa daria a ela (a normal para fora do cone, ou para cima: o lado que se vê)
        nrm = np.cross(P[1] - P[0], P[2] - P[0])
        nrm /= np.linalg.norm(nrm) + 1e-12
        fora = P.mean(0)[[0, 2]] - alvo.eixo
        if abs(nrm[1]) > 0.9:
            vista = nrm * np.sign(nrm[1])
        else:
            vista = nrm * (1.0 if np.dot(nrm[[0, 2]], fora) >= 0 else -1.0)
        if NOITE:
            c = tuple(np.minimum(luz_do_mapa_de_noite(vista), 1.0))     # sem luz de dentro: só a do mapa à noite
        else:
            c = (min(luz_do_mapa(vista), 1.0),) * 3
        MALHAS['pele'].face(P, c, uvs=[tuple(uv) for uv in casca['faces'][i][1]])
        tri += len(P) - 2
        # a beira da base (as faces em pé, de y 1,79 a 2,6): uma parede de colisão em pé, do pé dela para cima
        if abs(nrm[1]) < 0.3 and P[:, 1].max() < 2.7:
            baixos = [p for p in P if p[1] < PISO_Y - 0.9]
            if len(baixos) == 2:
                a, b = baixos
                y0 = min(a[1], b[1])
                PAREDES_DA_BASE.append((tuple(a), tuple(b), (b[0], y0 + ALTO_DA_BEIRA_DA_BASE, b[2])))
                PAREDES_DA_BASE.append((tuple(a), (b[0], y0 + ALTO_DA_BEIRA_DA_BASE, b[2]), (a[0], y0 + ALTO_DA_BEIRA_DA_BASE, a[2])))
    MARCA_ATUAL[0] = 0
    PELE.update(faces_devolvidas=len(faltam), triangulos=tri, cantos_soldados=soldados,
                paredes_de_colisao=len(PAREDES_DA_BASE))
    print('  pele do cone %d: %d faces da casca e da base devolvidas (do cone %d), %d triângulos, %d cantos soldados; '
          '%d triângulos de colisão na beira da base' % (CONE_DA_PELE, len(faltam), CONE_INTEIRO, tri, soldados, len(PAREDES_DA_BASE)))


print('Lendo a casca dos cones no mapa (%s)...' % ARQUIVO_DO_MAPA)
CASCA_DO_MAPA = ler_casca_do_mapa()
if CASCA_DO_MAPA is None:
    print('  sem o mapa: o cone %d fica sem a pele' % CONE_DA_PELE)
else:
    montar_pele_do_cone_4(CASCA_DO_MAPA)
    CASCA_DO_MAPA = None
print('  %.0f s' % (time.time() - INICIO))

# ------------------------------------------------------------------
# 4. Objetos, materiais e a luz assada
# ------------------------------------------------------------------
def criar_objetos():
    objs = []
    for chave, m in MALHAS.items():
        for k, bm in enumerate(m.bms):
            if len(bm.faces) == 0:
                bm.free()
                continue
            me = bpy.data.meshes.new('%s_%d' % (m.nome, k + 1))
            bm.to_mesh(me)
            bm.free()
            o = bpy.data.objects.new(me.name, me)
            bpy.context.scene.collection.objects.link(o)
            o['malha'] = chave
            objs.append(o)
    total = 0
    por_quarto = {}
    for chave in MALHAS:
        t = 0
        for o in objs:
            if o['malha'] == chave:
                me = o.data
                me.calc_loop_triangles()
                t += len(me.loop_triangles)
                q = atributo_de_face(me, 'quarto')
                pol = np.zeros(len(me.loop_triangles), np.int64)
                me.loop_triangles.foreach_get('polygon_index', pol)
                for num, cnt in zip(*np.unique(q[pol], return_counts=True)):
                    por_quarto[int(num)] = por_quarto.get(int(num), 0) + int(cnt)
        total += t
        print('  %-10s %7d triângulos' % (chave, t))
    print('  total      %7d triângulos; por quarto: %s' % (total, ', '.join('%d: %d' % kv for kv in sorted(por_quarto.items()))))
    TRIANGULOS_POR_QUARTO.update(por_quarto)
    return objs


TRIANGULOS_POR_QUARTO = {}


def material_de_assar(m):
    mat = novo_material('assar_' + m.nome)
    nt = mat.node_tree
    nt.nodes.clear()
    saida = nt.nodes.new('ShaderNodeOutputMaterial')
    tinta = nt.nodes.new('ShaderNodeAttribute')
    tinta.attribute_name = 'tinta'
    brilho = nt.nodes.new('ShaderNodeAttribute')
    brilho.attribute_name = 'brilho'
    cor = tinta.outputs['Color']
    if m.imagem is not None:
        tex = nt.nodes.new('ShaderNodeTexImage')
        tex.image = m.imagem
        mult = nt.nodes.new('ShaderNodeMix')
        mult.data_type = 'RGBA'
        mult.blend_type = 'MULTIPLY'
        mult.inputs['Factor'].default_value = 1.0
        nt.links.new(tinta.outputs['Color'], mult.inputs['A'])
        nt.links.new(tex.outputs['Color'], mult.inputs['B'])
        cor = mult.outputs['Result']
    bsdf = nt.nodes.new('ShaderNodeBsdfDiffuse')
    nt.links.new(cor, bsdf.inputs['Color'])
    em = nt.nodes.new('ShaderNodeEmission')
    nt.links.new(brilho.outputs['Color'], em.inputs['Color'])
    # as lâmpadas só brilham para a frente: sem o halo nas costas
    geo = nt.nodes.new('ShaderNodeNewGeometry')
    frente = nt.nodes.new('ShaderNodeMath')
    frente.operation = 'MULTIPLY_ADD'
    frente.inputs[1].default_value = -6.0
    frente.inputs[2].default_value = 6.0
    nt.links.new(geo.outputs['Backfacing'], frente.inputs[0])
    nt.links.new(frente.outputs[0], em.inputs['Strength'])
    soma = nt.nodes.new('ShaderNodeAddShader')
    nt.links.new(bsdf.outputs[0], soma.inputs[0])
    nt.links.new(em.outputs[0], soma.inputs[1])
    nt.links.new(soma.outputs[0], saida.inputs['Surface'])
    return mat


def b_de(p):
    return Vector((p[0], -p[2], p[1]))


CEU_DO_ASSADO = 0.12         # o céu que entra pelas aberturas (fraco: as luzes de área fazem o dia)
SOL_DO_ASSADO = 110000.0     # W: o holofote estreito que faz o sol da tarde, 16 m fora da janela do ar
ELEVACAO_DO_SOL = 50.0       # graus acima do horizonte (o feixe pega o alto do ar-condicionado e o relógio, como no filme)
GIRO_DO_SOL = -5.0           # graus: o sol vem um pouco de lado (a mancha cai no chão, ao lado do carro)
RAIO_DO_SOL = 0.70           # m: o tamanho da luz do sol (a 16 m): a beira da mancha fica macia, sem degraus da grade
COR_DO_SOL = (1.0, 0.72, 0.42)
LUZ_DA_PORTA = 50.0          # W: o dia claro da praça, pela porta (luz de área no vão)
COR_DA_PORTA = (1.0, 0.93, 0.84)
LUZ_DA_JANELA = 20.0         # W: o céu pelas janelas
COR_DA_JANELA = (0.82, 0.88, 1.0)
POTENCIA_DO_ABAJUR = 320.0   # W: a lâmpada de cada cúpula (uma luz pontual na boca da cúpula)
RAIO_DO_ABAJUR = 0.06
COR_DO_ABAJUR = (1.0, 0.70, 0.42)
POTENCIA_DA_CUPULA = 220.0   # W: a cúpula laranja acesa, vista de fora (uma luz grande atrás dela, para a parede)
RAIO_DA_CUPULA = 0.22
COR_DA_CUPULA = (1.0, 0.56, 0.26)
POTENCIA_DO_CONEZINHO = 30.0 # W: a luz da luminária de cone pequena (por cima dela)


def nova_luz(nome, tipo, pos, direcao=None, cor=(1, 1, 1), energia=100.0, **kw):
    L = bpy.data.lights.new(nome, tipo)
    L.energy = energia
    L.color = cor
    for k, v in kw.items():
        setattr(L, k, v)
    o = bpy.data.objects.new(nome, L)
    o.location = b_de(pos)
    if direcao is not None:
        o.rotation_euler = b_de(unit(direcao)).to_track_quat('-Z', 'Y').to_euler()
    bpy.context.scene.collection.objects.link(o)
    return o


def direcao_do_sol(w):
    """A direção (u, h, v) em que a luz do sol anda, entrando pela janela do ar"""
    nh = unit(np.array([w.n[0], 0.0, w.n[2]]))
    g = math.radians(GIRO_DO_SOL)
    c, s = math.cos(g), math.sin(g)
    nh = np.array([nh[0] * c - nh[2] * s, 0.0, nh[0] * s + nh[2] * c])
    e = math.radians(ELEVACAO_DO_SOL)
    return unit(-nh * math.cos(e) + np.array([0.0, -1.0, 0.0]) * math.sin(e))


def montar_luzes_do_cycles():
    sc = bpy.context.scene
    w = novo_mundo('ceu_dos_quartos')
    sc.world = w
    bg = w.node_tree.nodes['Background']
    bg.inputs[0].default_value = (0.55, 0.65, 0.85, 1)
    bg.inputs[1].default_value = CEU_DO_ASSADO
    for Q in QUARTOS:
        k = Q.numero
        jar, jmesa = Q.janelas
        d = direcao_do_sol(jar)
        nova_luz('sol_%d' % k, 'SPOT', Q.pc(jar.C - d * 16.0), Q.vc(d), COR_DO_SOL, SOL_DO_ASSADO,
                 spot_size=math.radians(11.0), spot_blend=0.3, shadow_soft_size=RAIO_DO_SOL)
        meio_v = (PORTA_V[0] + PORTA_V[1]) / 2
        nova_luz('porta_%d' % k, 'AREA', Q.p(U_FRENTE + 0.30, PORTA_ALTURA / 2, meio_v), Q.vet(-1, 0, 0), COR_DA_PORTA, LUZ_DA_PORTA,
                 shape='RECTANGLE', size=PORTA_V[1] - PORTA_V[0], size_y=PORTA_ALTURA)
        for j, w_ in enumerate((jar, jmesa)):
            nova_luz('janela_%d_%d' % (k, j), 'AREA', Q.pc(w_.C + 0.05 * w_.n), Q.vc(-w_.n), COR_DA_JANELA,
                     LUZ_DA_JANELA * (0.6 if j == 0 else 1.0), shape='DISK', size=2 * RAIO_DA_JANELA)
        for j, (nome, p, e) in enumerate(Q.luzes):
            if nome == 'abajur':
                # a lâmpada fica dentro de um bulbo fechado: a luz vai um pouco à frente dele, na cúpula
                nova_luz('abajur_%d_%d' % (k, j), 'POINT', np.asarray(p) + 0.10 * np.asarray(e), None, COR_DO_ABAJUR,
                         POTENCIA_DO_ABAJUR, shadow_soft_size=RAIO_DO_ABAJUR)
            elif nome == 'cupula':
                nova_luz('cupula_%d_%d' % (k, j), 'POINT', p, None, COR_DA_CUPULA, POTENCIA_DA_CUPULA, shadow_soft_size=RAIO_DA_CUPULA)
            else:
                nova_luz('conezinho_%d' % k, 'POINT', p, None, (1.0, 0.55, 0.22), POTENCIA_DO_CONEZINHO, shadow_soft_size=0.08)


# A luz de noite (--noite): sem o sol da tarde; o dia claro da porta e das janelas vira um luar fraco,
# azul-arroxeado; as luzes de dentro (o abajur de pé, a cúpula laranja, a luminária de cone pequena) acesas e
# mais presentes
CEU_DA_NOITE = (0.30, 0.28, 0.62)
FORCA_DO_CEU_DA_NOITE = 0.03
COR_DO_LUAR = (0.55, 0.56, 1.0)
NOITE_DAS_LUZES = (('porta_', 0.07), ('janela_', 0.07), ('abajur_', 1.35), ('cupula_', 1.35), ('conezinho_', 1.8))


def montar_luzes_da_noite():
    montar_luzes_do_cycles()
    sc = bpy.context.scene
    bg = sc.world.node_tree.nodes['Background']
    bg.inputs[0].default_value = (*CEU_DA_NOITE, 1)
    bg.inputs[1].default_value = FORCA_DO_CEU_DA_NOITE
    for o in list(sc.objects):
        if o.type != 'LIGHT':
            continue
        if o.name.startswith('sol_'):
            bpy.data.objects.remove(o, do_unlink=True)
            continue
        for prefixo, f in NOITE_DAS_LUZES:
            if o.name.startswith(prefixo):
                o.data.energy *= f
                if prefixo in ('porta_', 'janela_'):
                    o.data.color = COR_DO_LUAR
                break


def ligar_gpu():
    try:
        pr = bpy.context.preferences.addons['cycles'].preferences
        pr.compute_device_type = 'METAL'
        pr.get_devices()
        for d in pr.devices:
            d.use = True
        bpy.context.scene.cycles.device = 'GPU'
        return True
    except Exception as e:
        print('  sem GPU (%s): Cycles na CPU' % e)
        return False


def assar(todos):
    """Assa a luz dos quartos (a pele e o forro ficam fora da cena: não são assados nem fazem sombra)"""
    sc = bpy.context.scene
    sc.render.engine = 'CYCLES'
    ligar_gpu()
    sc.cycles.samples = AMOSTRAS
    sc.cycles.use_denoising = False
    sc.cycles.max_bounces = 6
    sc.cycles.diffuse_bounces = 3
    sc.cycles.glossy_bounces = 2
    objs = [o for o in todos if o['malha'] not in NAO_ASSADAS]
    for o in todos:
        o.hide_render = o not in objs
    for o in objs:
        m = MALHAS[o['malha']]
        o.data.materials.clear()
        o.data.materials.append(material_de_assar(m))
        at = o.data.color_attributes.new('luz', 'FLOAT_COLOR', 'CORNER')
        o.data.color_attributes.active_color = at
    bpy.ops.object.select_all(action='DESELECT')
    for o in objs:
        o.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]
    t = time.time()
    bpy.ops.object.bake(type='DIFFUSE', pass_filter={'DIRECT', 'INDIRECT'}, target='VERTEX_COLORS', use_clear=True)
    print('  luz difusa assada em %.0f s' % (time.time() - t))
    for o in todos:
        o.hide_render = False


def ler_atributo(me, nome):
    at = me.color_attributes.get(nome)
    a = np.zeros(len(me.loops) * 4, np.float32)
    if at is not None:
        at.data.foreach_get('color', a)
    return a.reshape(-1, 4)[:, :3]


def suavizar(me, valores):
    """Média dos cantos no mesmo ponto com a face virada mais ou menos para o mesmo lado (tira o ruído
    de cada amostra sem borrar as quinas)"""
    n = len(me.loops)
    vi = np.zeros(n, np.int64)
    me.loops.foreach_get('vertex_index', vi)
    co = np.zeros(len(me.vertices) * 3)
    me.vertices.foreach_get('co', co)
    co = co.reshape(-1, 3)[vi]
    fn = np.zeros(len(me.polygons) * 3)
    me.polygons.foreach_get('normal', fn)
    fn = fn.reshape(-1, 3)
    lt = np.zeros(len(me.polygons), np.int64)
    me.polygons.foreach_get('loop_total', lt)
    nl = np.repeat(fn, lt, axis=0)
    chave = np.round(co * 500).astype(np.int64)
    _, grupo = np.unique(chave, axis=0, return_inverse=True)
    grupo = grupo.ravel()
    ordem = np.argsort(grupo, kind='stable')
    g_ord = grupo[ordem]
    cortes = np.flatnonzero(np.diff(g_ord)) + 1
    saida = valores.copy()
    for idx in np.split(ordem, cortes):
        if len(idx) < 2:
            continue
        nn = nl[idx]
        perto = (nn @ nn.T) > 0.70
        v = valores[idx]
        saida[idx] = (perto.astype(np.float32) @ v) / perto.sum(1, keepdims=True)
    return saida


def atributo_de_face(me, nome):
    a = np.zeros(len(me.polygons), np.int32)
    at = me.attributes.get(nome)
    if at is not None:
        at.data.foreach_get('value', a)
    return a


def dados_dos_cantos(me):
    """Por canto: o índice da face, a posição (no Blender), a normal da face, a superfície grande de que a face
    faz parte (0 se nenhuma), se ela é de metal, o quarto e a marca"""
    nf = len(me.polygons)
    lt = np.zeros(nf, np.int64)
    ls = np.zeros(nf, np.int64)
    me.polygons.foreach_get('loop_total', lt)
    me.polygons.foreach_get('loop_start', ls)
    vi = np.zeros(len(me.loops), np.int64)
    me.loops.foreach_get('vertex_index', vi)
    co = np.zeros(len(me.vertices) * 3)
    me.vertices.foreach_get('co', co)
    fn = np.zeros(nf * 3)
    me.polygons.foreach_get('normal', fn)
    face = np.repeat(np.arange(nf), lt)
    return dict(face=face, ls=ls, lt=lt, co=co.reshape(-1, 3)[vi], n=fn.reshape(-1, 3)[face],
                sup=atributo_de_face(me, 'superficie')[face], metal=atributo_de_face(me, 'metal').astype(bool)[face],
                quarto=atributo_de_face(me, 'quarto')[face], marca=atributo_de_face(me, 'marca')[face])


def do_jogo(co):
    """Pontos do Blender nos eixos do jogo"""
    return np.stack([co[:, 0], co[:, 2], -co[:, 1]], 1)


def no_quarto(Q, P):
    """(u, h, v) do desenho de pontos P (eixos do jogo) do quarto Q"""
    q = P[:, [0, 2]] - Q.eixo
    return np.stack([q @ Q.f, P[:, 1] - PISO_Y, Q.s * (q @ Q.d)], 1)


def escondidos_no_quarto(Q, P, sup):
    """Quais pontos P (eixos do jogo; sup: a superfície de cada um) ficam escondidos: atrás de alguma coisa
    do quarto (Q.escondidos), embaixo do rodapé, da cimalha e da sanca, ou em cima, sob a sanca"""
    esc = np.zeros(len(P), bool)
    for R, contornos, b0, b1 in Q.escondidos:
        q = P - R.o
        a, y, b = q @ R.ex, q @ R.ey, q @ R.ez
        perto = (b >= b0) & (b <= b1)
        if not perto.any():
            continue
        for poly in contornos:
            poly = np.asarray(poly, float)
            S = np.array([(poly[(k + 1) % len(poly), 0] - poly[k, 0]) * (y - poly[k, 1]) -
                          (poly[(k + 1) % len(poly), 1] - poly[k, 1]) * (a - poly[k, 0]) for k in range(len(poly))])
            esc |= perto & (np.all(S >= 0, axis=0) | np.all(S <= 0, axis=0))
    L = no_quarto(Q, P)
    h = L[:, 1]
    parede = (sup == SUP_CONE) | (sup == SUP_FRENTE)
    esc |= parede & ((h < PERFIL_DO_RODAPE[1][1]) | ((h > PERFIL_DO_FRISO[1][1]) & (h < PERFIL_DO_FRISO[-2][1])) |
                     (h > PERFIL_DA_SANCA[1][1]))
    esc |= (sup == SUP_TETO) & (np.hypot(L[:, 0], L[:, 2]) > raio_da_parede(ALTURA) - PERFIL_DA_SANCA[-1][0] + 0.03)
    return esc


def alisar_superficies(itens, passadas=2):
    """itens: [(cantos, valores)] de cada objeto. Junta os cantos das superfícies grandes de todos os objetos
    em nós (o mesmo ponto da mesma superfície do mesmo quarto: a parede de cone inteira, não coluna por
    coluna), dá aos nós escondidos (atrás dos quadros, do ar-condicionado, das molduras, dos tapetes) a luz
    dos vizinhos à vista, de fora para dentro (senão a sombra deles vaza em volta pela interpolação), e alisa
    (laplaciano) entre os nós ligados por uma aresta. Devolve os valores novos de cada objeto e, por canto,
    se ele estava escondido."""
    chaves, vals, arestas_a, arestas_b, onde = [], [], [], [], []
    base = 0
    for k, (d, v) in enumerate(itens):
        idx = np.flatnonzero(d['sup'] > 0)
        onde.append(idx)
        if len(idx) == 0:
            continue
        chaves.append(np.concatenate([np.round(d['co'][idx] * 500), d['sup'][idx, None], d['quarto'][idx, None]], 1).astype(np.int64))
        vals.append(v[idx])
        pos = np.full(len(d['face']), -1, np.int64)
        pos[idx] = base + np.arange(len(idx))
        f = d['face'][idx]
        s_, t_ = d['ls'][f], d['lt'][f]
        prox = s_ + (idx - s_ + 1) % t_
        arestas_a.append(pos[idx])
        arestas_b.append(pos[prox])
        base += len(idx)
    saida = [v.copy() for _, v in itens]
    esc_dos_cantos = [np.zeros(len(d['face']), bool) for d, _ in itens]
    if base == 0:
        return saida, esc_dos_cantos
    chave = np.concatenate(chaves)
    _, no = np.unique(chave, axis=0, return_inverse=True)
    no = no.ravel()
    nn = int(no.max()) + 1
    a = no[np.concatenate(arestas_a)]
    b = np.concatenate(arestas_b)
    ok = b >= 0
    a, b = a[ok], no[b[ok]]
    V = np.concatenate(vals)
    val = np.zeros((nn, V.shape[1]))
    cnt = np.zeros(nn)
    np.add.at(val, no, V)
    np.add.at(cnt, no, 1)
    val /= np.maximum(cnt, 1)[:, None]
    # os nós escondidos, quarto por quarto
    pos_no = np.zeros((nn, 3))
    pos_no[no] = do_jogo(np.concatenate([d['co'][i] for (d, _), i in zip(itens, onde)]))
    sup_no = np.zeros(nn, np.int64)
    sup_no[no] = chave[:, 3]
    qua_no = np.zeros(nn, np.int64)
    qua_no[no] = chave[:, 4]
    esc = np.zeros(nn, bool)
    for Q in QUARTOS:
        sel = np.flatnonzero(qua_no == Q.numero)
        esc[sel] = escondidos_no_quarto(Q, pos_no[sel], sup_no[sel])
    original = val.copy()
    sabido = ~esc
    for _ in range(60):
        if sabido.all():
            break
        soma = np.zeros_like(val)
        conta = np.zeros(nn)
        m = sabido[b] & ~sabido[a]
        np.add.at(soma, a[m], val[b[m]])
        np.add.at(conta, a[m], 1)
        m = sabido[a] & ~sabido[b]
        np.add.at(soma, b[m], val[a[m]])
        np.add.at(conta, b[m], 1)
        novos = (conta > 0) & ~sabido
        if not novos.any():
            break
        val[novos] = soma[novos] / conta[novos, None]
        sabido |= novos
    val[~sabido] = original[~sabido]
    for _ in range(passadas):
        soma = np.zeros_like(val)
        grau = np.zeros(nn)
        np.add.at(soma, a, val[b])
        np.add.at(soma, b, val[a])
        np.add.at(grau, a, 1)
        np.add.at(grau, b, 1)
        tem = grau > 0
        val[tem] = 0.5 * val[tem] + 0.5 * soma[tem] / grau[tem, None]
    ini = 0
    for k, i in enumerate(onde):
        if len(i) == 0:
            continue
        nos = no[ini:ini + len(i)]
        saida[k][i] = val[nos]
        esc_dos_cantos[k][i] = esc[nos]
        ini += len(i)
    print('  luz: %d pontos das superfícies grandes, %d escondidos (com a luz dos vizinhos)' % (nn, esc.sum()))
    return saida, esc_dos_cantos


def joelho(x):
    # até 0,82 reta; acima, encosta devagar em 1 (os 8 bits cortam acima de 1)
    return np.where(x < 0.82, x, 0.82 + 0.18 * (1 - np.exp(-(x - 0.82) / 0.18)))


LUMA = np.array([0.2126, 0.7152, 0.0722])
AMBIENTE = 0.008         # quase nada de luz por igual: os cantos e o teto ficam mais escuros que as poças de luz
ALVO_DA_PAREDE = 0.50    # a mediana da luz das paredes (de 1 a 4 m de altura), na tela: daí sai a escala da luz
QUEDA_DO_ALTO = (2.6, ALTURA, 0.42)   # a luz cai no alto do quarto: de 1 (nesta altura) até isto (nesta outra)
SATURACAO_DA_LUZ = 0.78  # quanto da cor da luz (quente) fica na cor do vértice
OLHO_DO_BRILHO = (-4.4, 2.4, 0.0)     # o olho médio do brilho falso: a câmera atrás do carro estacionado
CREME_DE_FORA = (0.88, 0.82, 0.70)    # o forro da porta na boca do nicho, no dia claro de fora (a casca no mapa não tem sombra)
CONTRASTE = 1.0          # a luz na tela (sem a cor) vai a esta potência (de noite: os cantos mais escuros)
BRILHO_DO_SOL = 1.2      # no brilho falso do metal: o reflexo do sol...
BRILHO_DO_CEU = 0.25     # ...e o do céu claro em cima
if NOITE:
    # as mesmas contas, com a noite: a parede mais escura (a mediana), as poças quentes do abajur no alto da escala
    AMBIENTE = 0.003
    ALVO_DA_PAREDE = 0.30
    CONTRASTE = 1.35
    SATURACAO_DA_LUZ = 0.85
    BRILHO_DO_SOL = 0.0
    BRILHO_DO_CEU = 0.05
    CREME_DE_FORA = (0.56, 0.43, 0.34)  # na boca do nicho, de noite: a casca em volta à luz da porta (o _noite.json)


def brilho_falso(Q, co, n):
    """O brilho do metal (latão, ouro) visto do olho médio: os reflexos das cúpulas e do sol e um céu
    claro em cima (pontos e normais no Blender)"""
    olho = np.array(b_de(Q.p(*OLHO_DO_BRILHO)))
    v = co - olho
    v /= np.linalg.norm(v, axis=1, keepdims=True)
    r = v - 2 * (v * n).sum(1, keepdims=True) * n
    total = np.zeros(len(co))
    for nome, p, _ in Q.luzes:
        if nome != 'abajur':
            continue
        l = np.array(b_de(p)) - co
        l /= np.linalg.norm(l, axis=1, keepdims=True)
        total += 0.9 * np.clip((r * l).sum(1), 0, 1) ** 30
    sol = -np.array(b_de(Q.vc(direcao_do_sol(Q.janelas[0]))))
    total += BRILHO_DO_SOL * np.clip(r @ sol, 0, 1) ** 60
    total += BRILHO_DO_CEU * np.clip(r[:, 2], 0, 1) ** 2
    return total


def gravar_col(me, cor):
    if me.color_attributes.get('Col'):
        me.color_attributes.remove(me.color_attributes['Col'])
    at = me.color_attributes.new('Col', 'FLOAT_COLOR', 'CORNER')
    a = np.concatenate([cor, np.ones((len(cor), 1))], axis=1).astype(np.float32).ravel()
    at.data.foreach_set('color', a)
    me.color_attributes.active_color = me.color_attributes['Col']
    me.color_attributes.render_color_index = me.color_attributes.active_color_index


def finalizar_cores(objs):
    """A luz assada (linear) vira a cor do vértice: alisada, sem as sombras escondidas, com a queda no alto,
    normalizada pela mediana das paredes, na escala de tela, vezes a tinta. O forro da porta e a pele do
    cone 4 (não assados) recebem a cor deles aqui."""
    assados = [o for o in objs if o['malha'] not in NAO_ASSADAS]
    itens = []
    for o in assados:
        me = o.data
        cantos = dados_dos_cantos(me)
        itens.append((cantos, suavizar(me, ler_atributo(me, 'luz'))))
    luzes, escondidos = alisar_superficies(itens)
    dados = []
    lums = []
    por_numero = {Q.numero: Q for Q in QUARTOS}
    for o, (cantos, _), luz, esc in zip(assados, itens, luzes, escondidos):
        me = o.data
        tinta = ler_atributo(me, 'tinta')
        brilho = ler_atributo(me, 'brilho')
        acesa = brilho.sum(1) > 0
        # a queda no alto do quarto
        h = cantos['co'][:, 2] - PISO_Y
        h0, h1, fator = QUEDA_DO_ALTO
        luz = luz * (1 - (1 - fator) * suave(h0, h1, h))[:, None]
        dados.append((o, luz, tinta, brilho, cantos, esc))
        parede = ((cantos['sup'] == SUP_CONE) | (cantos['sup'] == SUP_FRENTE)) & ~esc & ~acesa & (h > 1.0) & (h < 4.0)
        lums.append(luz[parede] @ LUMA)
    lum = np.concatenate(lums)
    mediana = float(np.median(lum))
    alvo = ALVO_DA_PAREDE if CONTRASTE == 1.0 else ALVO_DA_PAREDE ** (1.0 / CONTRASTE)   # (a mediana depois do contraste)
    escala = (float(np.asarray(linear((alvo,) * 3))[0]) - AMBIENTE) / max(mediana, 1e-6)
    print('  luz: mediana das paredes %.4f, escala %.3f' % (mediana, escala))
    medidas = {'piso': [], 'tapetes': [], 'quadros': [], 'teto': [], 'pé das paredes': [], 'paredes (1 a 4 m)': [],
               'alto das paredes': []}
    cor_da_moldura = {}
    for o, luz, tinta, brilho, cantos, esc in dados:
        me = o.data
        tela_luz = joelho(tela(luz * escala + AMBIENTE))
        if CONTRASTE != 1.0:
            tela_luz = np.power(tela_luz, CONTRASTE)
        cinza = (tela_luz @ LUMA)[:, None]
        tela_luz = cinza + (tela_luz - cinza) * SATURACAO_DA_LUZ
        cor = tela_luz * tela(tinta)
        # o brilho falso do metal
        metal = cantos['metal']
        if metal.any():
            for num in np.unique(cantos['quarto'][metal]):
                sel = metal & (cantos['quarto'] == num)
                b = brilho_falso(por_numero[int(num)], cantos['co'][sel], cantos['n'][sel])
                cor[sel] = cor[sel] * (1 + 0.6 * b[:, None]) + b[:, None] * np.array([1.0, 0.94, 0.80]) * 0.55
        acesa = brilho.sum(1) > 0
        cor[acesa] = np.clip(tela(brilho[acesa]), 0, 1)
        cor = np.clip(cor, 0, 1)
        lt = tela_luz @ LUMA
        sup = cantos['sup']
        h = cantos['co'][:, 2] - PISO_Y
        vis = ~esc & ~acesa
        parede = ((sup == SUP_CONE) | (sup == SUP_FRENTE)) & vis
        medidas['piso'].append(lt[(sup == SUP_PISO) & vis])
        medidas['tapetes'].append(lt[(sup == SUP_TAPETE) & vis])
        medidas['teto'].append(lt[(sup == SUP_TETO) & vis])
        medidas['pé das paredes'].append(lt[parede & (h < 0.5)])
        medidas['paredes (1 a 4 m)'].append(lt[parede & (h > 1.0) & (h < 4.0)])
        medidas['alto das paredes'].append(lt[parede & (h > 5.0)])
        if o['malha'] == 'quadros':
            medidas['quadros'].append(lt)
        for num in np.unique(cantos['quarto'][cantos['marca'] == MARCA_MOLDURA]):
            sel = (cantos['marca'] == MARCA_MOLDURA) & (cantos['quarto'] == num)
            cor_da_moldura.setdefault(int(num), []).append(cor[sel])
        gravar_col(me, cor)
    for nome, v in medidas.items():
        v = np.concatenate(v) if v else np.zeros(1)
        if len(v) == 0:
            v = np.zeros(1)
        print('  luz na tela, %-18s p10 %.2f  mediana %.2f  p90 %.2f  máx %.2f' % (nome, *np.percentile(v, [10, 50, 90]), v.max()))
        MEDIDAS_DA_LUZ[nome] = [round(float(x), 3) for x in np.percentile(v, [10, 50, 90])]
    # o forro da porta: junto do quarto, a cor da moldura clara do mesmo quarto; na boca do nicho, o creme no
    # dia claro de fora. A pele do cone 4: branca (a textura da casca já tem a cor)
    moldura = {k: np.median(np.concatenate(v), axis=0) for k, v in cor_da_moldura.items()}
    for o in objs:
        if o['malha'] not in NAO_ASSADAS:
            continue
        me = o.data
        cantos = dados_dos_cantos(me)
        cor = np.clip(tela(ler_atributo(me, 'tinta')), 0, 1)
        forro = cantos['marca'] == MARCA_FORRO
        for num in np.unique(cantos['quarto'][forro]):
            Q = por_numero[int(num)]
            sel = forro & (cantos['quarto'] == num)
            u = no_quarto(Q, do_jogo(cantos['co'][sel]))[:, 0]
            k = suave(FORRO_U[0], FORRO_U[1], u)[:, None]
            cor[sel] = np.clip(moldura[int(num)][None, :] * (1 - k) + np.array(CREME_DE_FORA)[None, :] * k, 0, 1)
        gravar_col(me, cor)
    print('  cor da moldura da porta (forro): %s' % ', '.join('%d: %s' % (k, np.round(v, 2)) for k, v in sorted(moldura.items())))
    return escala


MEDIDAS_DA_LUZ = {}


def material_de_previa(m, so_luz=False):
    """O que o jogo mostra: textura × cor do vértice, sem luz (na tela: emissão de (t·c)^2,2). so_luz: sem a
    textura (só a cor do vértice: para ver a luz assada)"""
    mat = novo_material(('luz_' if so_luz else 'previa_') + m.nome)
    nt = mat.node_tree
    nt.nodes.clear()
    saida = nt.nodes.new('ShaderNodeOutputMaterial')
    at = nt.nodes.new('ShaderNodeAttribute')
    at.attribute_name = 'Col'
    gama = nt.nodes.new('ShaderNodeGamma')
    gama.inputs['Gamma'].default_value = 2.2
    nt.links.new(at.outputs['Color'], gama.inputs['Color'])
    cor = gama.outputs['Color']
    if m.imagem is not None and not so_luz:
        tex = nt.nodes.new('ShaderNodeTexImage')
        tex.image = m.imagem
        tex.interpolation = 'Linear'
        mult = nt.nodes.new('ShaderNodeMix')
        mult.data_type = 'RGBA'
        mult.blend_type = 'MULTIPLY'
        mult.inputs['Factor'].default_value = 1.0
        nt.links.new(cor, mult.inputs['A'])
        nt.links.new(tex.outputs['Color'], mult.inputs['B'])
        cor = mult.outputs['Result']
    em = nt.nodes.new('ShaderNodeEmission')
    nt.links.new(cor, em.inputs['Color'])
    nt.links.new(em.outputs[0], saida.inputs['Surface'])
    return mat


def material_de_exportar(m):
    mat = novo_material(m.nome)
    nt = mat.node_tree
    nt.nodes.clear()
    saida = nt.nodes.new('ShaderNodeOutputMaterial')
    bsdf = nt.nodes.new('ShaderNodeBsdfPrincipled')
    bsdf.inputs['Base Color'].default_value = (1, 1, 1, 1)
    bsdf.inputs['Roughness'].default_value = 0.8
    if m.imagem is not None:
        tex = nt.nodes.new('ShaderNodeTexImage')
        tex.image = m.imagem
        nt.links.new(tex.outputs['Color'], bsdf.inputs['Base Color'])
    nt.links.new(bsdf.outputs[0], saida.inputs['Surface'])
    return mat


def trocar_materiais(objs, fabrica):
    feitos = {}
    for o in objs:
        m = MALHAS[o['malha']]
        if m.nome not in feitos:
            feitos[m.nome] = fabrica(m)
        o.data.materials.clear()
        o.data.materials.append(feitos[m.nome])


print('Assando a luz (Cycles, %d amostras)...' % AMOSTRAS)
OBJS = criar_objetos()
if NOITE:
    montar_luzes_da_noite()
else:
    montar_luzes_do_cycles()
assar(OBJS)
finalizar_cores(OBJS)
print('  %.0f s' % (time.time() - INICIO))


# ------------------------------------------------------------------
# 5. Exportar o desenho e conferir
# ------------------------------------------------------------------
def ler_glb(caminho):
    d = open(caminho, 'rb').read()
    n = struct.unpack('<I', d[12:16])[0]
    return json.loads(d[20:20 + n]), d


def soldar(o):
    """Junta os vértices no mesmo lugar (cada face nasceu com os seus): as cores ficam nos cantos, então o
    exportador só separa onde a cor ou a textura mudam"""
    bm = bmesh.new()
    bm.from_mesh(o.data)
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=1e-5)
    bm.to_mesh(o.data)
    bm.free()


def media_de_col(objs):
    """A média das cores dos vértices pesada como o exportador grava: um vértice por ponto (soldado) com a
    mesma cor e as mesmas coordenadas de textura, não um por canto (para comparar com a do .glb)"""
    soma, n = np.zeros(3), 0
    for o in objs:
        me = o.data
        c = ler_atributo(me, 'Col')
        vi = np.zeros(len(me.loops), np.int64)
        me.loops.foreach_get('vertex_index', vi)
        partes = [vi[:, None].astype(np.float64), np.round(c * 65535)]
        if me.uv_layers:
            uv = np.zeros(len(me.loops) * 2)
            me.uv_layers[0].data.foreach_get('uv', uv)
            partes.append(np.round(uv.reshape(-1, 2) * 1e5))
        _, idx = np.unique(np.concatenate(partes, 1), axis=0, return_index=True)
        soma += c[idx].sum(0)
        n += len(idx)
    return soma / max(n, 1)


def cores_no_glb(j, d):
    """A média das COLOR_0 gravadas no .glb (lidas do buffer), ponderada pelos vértices"""
    n_json = struct.unpack('<I', d[12:16])[0]
    base = 20 + n_json + 8
    soma, n = np.zeros(3), 0
    tipos = {5121: (np.uint8, 255.0), 5123: (np.uint16, 65535.0), 5126: (np.float32, 1.0)}
    for me in j['meshes']:
        for p in me['primitives']:
            acc = j['accessors'][p['attributes']['COLOR_0']]
            bv = j['bufferViews'][acc['bufferView']]
            dt, esc = tipos[acc['componentType']]
            comps = {'VEC3': 3, 'VEC4': 4}[acc['type']]
            ini = base + bv.get('byteOffset', 0) + acc.get('byteOffset', 0)
            a = np.frombuffer(d, dt, count=acc['count'] * comps, offset=ini).reshape(-1, comps)[:, :3] / esc
            soma += a.sum(0)
            n += len(a)
    return soma / max(n, 1)


def exportar_visual(objs, caminho=None):
    trocar_materiais(objs, material_de_exportar)
    for o in objs:
        me = o.data
        for nome in [a.name for a in me.color_attributes if a.name != 'Col']:
            me.color_attributes.remove(me.color_attributes[nome])
        for nome in ('superficie', 'metal', 'quarto', 'marca'):
            if me.attributes.get(nome) is not None:
                me.attributes.remove(me.attributes[nome])
        me.color_attributes.active_color = me.color_attributes['Col']
        if MALHAS[o['malha']].imagem is None:
            while me.uv_layers:
                me.uv_layers.remove(me.uv_layers[0])
        soldar(o)
    esperado = media_de_col(objs)
    bpy.ops.object.select_all(action='DESELECT')
    for o in objs:
        o.select_set(True)
    caminho = caminho or SAIDA + '.glb'
    bpy.ops.export_scene.gltf(filepath=caminho, export_format='GLB', use_selection=True, export_yup=True,
                              export_apply=True, export_animations=False, export_normals=False,
                              export_vertex_color='ACTIVE', export_all_vertex_colors=False,
                              export_active_vertex_color_when_no_material=True, export_image_format='AUTO',
                              export_materials='EXPORT')
    j, d = ler_glb(caminho)
    tris = 0
    sem_cor = 0
    maior = 0
    for me in j['meshes']:
        for p in me['primitives']:
            if 'COLOR_0' not in p['attributes']:
                sem_cor += 1
            acc = j['accessors'][p['indices']]
            tris += acc['count'] // 3
            maior = max(maior, j['accessors'][p['attributes']['POSITION']]['count'])
    tipos = sorted({(j['accessors'][p['attributes']['COLOR_0']]['type'], j['accessors'][p['attributes']['COLOR_0']]['componentType'])
                    for me in j['meshes'] for p in me['primitives'] if 'COLOR_0' in p['attributes']})
    imgs = [(i.get('name'), i.get('mimeType')) for i in j.get('images', [])]
    fatores = [m.get('pbrMetallicRoughness', {}).get('baseColorFactor', [1, 1, 1, 1]) for m in j['materials']]
    print('Exportado %s' % caminho)
    print('  primitivas: %d (sem COLOR_0: %d; COLOR_0 %s); maior: %d vértices' % (
        sum(len(m['primitives']) for m in j['meshes']), sem_cor, tipos, maior))
    print('  triângulos: %d; materiais: %d; texturas: %s' % (tris, len(j['materials']), imgs))
    print('  baseColorFactor: %s' % fatores)
    print('  tamanho: %.2f MB' % (os.path.getsize(caminho) / 1e6))
    lido = cores_no_glb(j, d)
    print('  COLOR_0 lido do .glb: média %s; no Blender (por vértice soldado): %s' % (np.round(lido, 3), np.round(esperado, 3)))
    if np.abs(lido - esperado).max() > 0.01:
        raise SystemExit('ERRO: as cores dos vértices no .glb não batem com as do Blender')
    if sem_cor:
        raise SystemExit('ERRO: há primitivas sem COLOR_0')
    if maior >= 65536:
        raise SystemExit('ERRO: uma malha passou de 65535 vértices')
    return dict(triangulos=tris, materiais=len(j['materials']), texturas=len(imgs), bytes=os.path.getsize(caminho),
                primitivas=sum(len(m['primitives']) for m in j['meshes']), maior=maior)


if NOITE:
    # de noite, o desenho assado vai para a pasta temporária; o _noite.glb é o .glb de dia com estas cores (9.)
    CONTAGENS = exportar_visual(OBJS, os.path.join(PASTA_TEMP, 'noite_assada.glb'))
else:
    CONTAGENS = exportar_visual(OBJS)


# ------------------------------------------------------------------
# 6. A colisão
# ------------------------------------------------------------------
COLISAO = []      # triângulos (eixos do jogo)


def tri(a, b, c):
    COLISAO.append((tuple(map(float, a)), tuple(map(float, b)), tuple(map(float, c))))


def quad_col(a, b, c, d):
    tri(a, b, c)
    tri(a, c, d)


def colisao_do_quarto(Q):
    """O piso (com a soleira por cima do começo da rampa da casca), a parede de cone (inclinada como a de
    verdade, até o teto, para a câmera), a parede da porta com o vão aberto e os móveis em pé"""
    # o piso
    poli = disco_cortado(raio_da_parede(0.0) - 0.01, U_FRENTE, 64)
    p0 = poli[0]
    for k in range(1, len(poli) - 1):
        tri(Q.p(p0[0], 0.0, p0[1]), Q.p(poli[k][0], 0.0, poli[k][1]), Q.p(poli[k + 1][0], 0.0, poli[k + 1][1]))
    quad_col(Q.p(U_FRENTE, 0.0, PORTA_V[0] - 0.1), Q.p(3.95, 0.0, PORTA_V[0] - 0.1), Q.p(3.95, 0.0, PORTA_V[1] + 0.1),
             Q.p(U_FRENTE, 0.0, PORTA_V[1] + 0.1))
    # a parede de cone, em 40 fatias, nos níveis da altura
    niveis = [0.0, 1.5, 3.0, 4.5, ALTURA]
    fatias = 40
    aneis = []
    for h in niveis:
        rr = raio_da_parede(h) - 0.02
        tc = math.atan2(math.sqrt(max(rr * rr - (U_FRENTE - 0.02) ** 2, 0)), U_FRENTE - 0.02)
        aneis.append([Q.p(rr * math.cos(t), h, rr * math.sin(t)) for t in np.linspace(tc, 2 * math.pi - tc, fatias + 1)])
    for i in range(len(niveis) - 1):
        for k in range(fatias):
            quad_col(aneis[i][k], aneis[i][k + 1], aneis[i + 1][k + 1], aneis[i + 1][k])
    # a parede da porta: dos cantos até o vão, e por cima do vão
    uf = U_FRENTE - 0.02
    for i in range(len(niveis) - 1):
        h0, h1 = niveis[i], niveis[i + 1]
        c0 = meia_largura_da_frente(h0, -0.02)
        c1 = meia_largura_da_frente(h1, -0.02)
        quad_col(Q.p(uf, h0, PORTA_V[1]), Q.p(uf, h0, c0), Q.p(uf, h1, c1), Q.p(uf, h1, PORTA_V[1]))
        quad_col(Q.p(uf, h0, -c0), Q.p(uf, h0, PORTA_V[0]), Q.p(uf, h1, PORTA_V[0]), Q.p(uf, h1, -c1))
    quad_col(Q.p(uf, PORTA_ALTURA, PORTA_V[0]), Q.p(uf, PORTA_ALTURA, PORTA_V[1]), Q.p(uf, ALTURA, PORTA_V[1]), Q.p(uf, ALTURA, PORTA_V[0]))
    # o forro do vão, até os batentes da casca
    for v in PORTA_V:
        quad_col(Q.p(uf, 0.0, v), Q.p(3.66, 0.0, v), Q.p(3.66, PORTA_ALTURA, v), Q.p(uf, PORTA_ALTURA, v))
    # os móveis: caixas de paredes em pé, sem tampa
    for poli, alto in Q.caixas_de_colisao:
        for k in range(len(poli)):
            a, b = poli[k], poli[(k + 1) % len(poli)]
            quad_col(Q.p(a[0], 0.0, a[1]), Q.p(b[0], 0.0, b[1]), Q.p(b[0], alto, b[1]), Q.p(a[0], alto, a[1]))
    # o teto: um disco 6 m acima do piso (alto demais para virar chão do carro, que sobe até 2 m, e dos
    # moradores, que só olham até y 4,5), para a câmera saber que está coberta e chegar mais perto
    poli = disco_cortado(raio_da_parede(ALTURA) - 0.02, U_FRENTE - 0.02, 48)
    p0 = poli[0]
    for k in range(1, len(poli) - 1):
        tri(Q.p(p0[0], ALTURA, p0[1]), Q.p(poli[k + 1][0], ALTURA, poli[k + 1][1]), Q.p(poli[k][0], ALTURA, poli[k][1]))


def montar_colisao():
    for Q in QUARTOS:
        colisao_do_quarto(Q)
    for t in PAREDES_DA_BASE:      # a beira da base devolvida ao cone 4 (como nos outros cones)
        tri(*t)
    me = bpy.data.meshes.new('cozy_cone_quartos_colisao')
    verts = []
    faces = []
    for t in COLISAO:
        base = len(verts)
        verts += [b_de(p) for p in t]
        faces.append((base, base + 1, base + 2))
    me.from_pydata(verts, [], faces)
    o = bpy.data.objects.new('cozy_cone_quartos_colisao', me)
    bpy.context.scene.collection.objects.link(o)
    bpy.ops.object.select_all(action='DESELECT')
    o.select_set(True)
    bpy.context.view_layer.objects.active = o
    caminho = SAIDA + '_colisao.glb'
    bpy.ops.export_scene.gltf(filepath=caminho, export_format='GLB', use_selection=True, export_yup=True,
                              export_apply=True, export_animations=False, export_normals=False, export_texcoords=False,
                              export_vertex_color='NONE', export_materials='NONE')
    # confere as regras da física: deitados, só o piso e o teto (6 m acima dele); as paredes são todas em pé
    tri_arr = np.array(COLISAO)
    n = np.cross(tri_arr[:, 1] - tri_arr[:, 0], tri_arr[:, 2] - tri_arr[:, 0])
    n /= np.linalg.norm(n, axis=1, keepdims=True)
    deitado = np.abs(n[:, 1]) > 0.6
    ys = tri_arr[:, :, 1]
    piso = deitado & (np.abs(ys - PISO_Y).max(1) < 1e-3)
    teto = deitado & (np.abs(ys - TETO_Y).max(1) < 1e-3)
    if (deitado & ~piso & ~teto).any():
        raise SystemExit('ERRO: há faces deitadas na colisão fora do piso e do teto')
    print('Exportado %s: %d triângulos (%d de piso em y %.2f, %d de teto em y %.2f, %d de parede com |normal.y| até %.2f, '
          '%d deles na beira da base do cone %d)' % (caminho, len(COLISAO), piso.sum(), PISO_Y, teto.sum(), TETO_Y,
                                                      (~deitado).sum(), np.abs(n[~deitado, 1]).max(), len(PAREDES_DA_BASE), CONE_DA_PELE))
    o.hide_render = True
    CONTAGEM_DA_COLISAO.update(piso=int(piso.sum()), teto=int(teto.sum()), paredes=int((~deitado).sum()),
                               beira_da_base=len(PAREDES_DA_BASE))
    return len(COLISAO)


CONTAGEM_DA_COLISAO = {}


if not NOITE:     # de noite, a colisão e o json de dia ficam como estão
    TRIANGULOS_DA_COLISAO = montar_colisao()


# ------------------------------------------------------------------
# 7. O JSON para a integração
# ------------------------------------------------------------------
def r3(v):
    return [round(float(x), 3) for x in v]


def montar_json():
    portas, lugares = [], {}
    for Q in QUARTOS:
        meio_v = (PORTA_V[0] + PORTA_V[1]) / 2
        para_fora = Q.f
        portas.append(dict(
            cone=Q.numero,
            centro=r3(Q.p(3.825, 0.0, meio_v)),
            centro_por_dentro=r3(Q.p(U_FRENTE, 0.0, meio_v)),
            centro_na_boca_do_nicho=r3(Q.p(5.36, -0.02, meio_v)),
            rumo_para_fora=round(rumo_de(para_fora), 4),
            rumo_para_dentro=round(rumo_de(-para_fora), 4),
            direcao_para_fora=[round(float(para_fora[0]), 4), 0.0, round(float(para_fora[1]), 4)],
            largura=round(PORTA_V[1] - PORTA_V[0], 3),
            altura=round(PORTA_ALTURA, 3),
            pontas=[r3(Q.p(3.825, 0.0, PORTA_V[0])), r3(Q.p(3.825, 0.0, PORTA_V[1]))],
        ))
        nome = 'mcqueen' if Q.mcqueen else 'cone_%d' % Q.numero
        x, z = Q.xz(LUGAR_U, 0.0)
        lugares[nome] = dict(cone=Q.numero, x=round(float(x), 3), y=PISO_Y, z=round(float(z), 3), rumo=round(rumo_de(Q.f), 4),
                             comprimento=5.3, largura=2.7, teto_da_busca_do_chao=round(PISO_Y + 2.0, 3),
                             nota='entre os dois tapetes, com a frente para a porta (entrou de ré, como no filme); cabe até 5,3 x '
                                  '2,7 m. Para pôr um carro aqui, procure o chão só até teto_da_busca_do_chao (com o teto de '
                                  '5000 do levarCarroPara, o chão achado é o teto do quarto ou o topo do cone)')
    aberturas = []
    rumos = {Q.numero: Q.rumo for Q in QUARTOS}
    for cone, peca, mn, mx, faces in ABERTURAS_DA_CASCA:
        if peca == 'porta':
            a = dict(nome='porta_cone_%d' % cone, descricao='o painel pintado da porta de garagem do cone %d (peça solta)' % cone,
                     sai_da_colisao=True, caixa_min=list(mn), caixa_max=list(mx), faces=faces, objetos=['Object_60'],
                     rumo_para_fora=rumos[cone])
        else:
            a = dict(nome='vidro_janela_cone_%d_%s' % (cone, peca),
                     descricao='o vidro redondo pintado da janela da %s da porta do cone %d (de quem olha a porta de fora), '
                               'com a lasca atrás dele' % (peca, cone),
                     sai_da_colisao=False, caixa_min=list(mn), caixa_max=list(mx), faces=faces, objetos=['Object_60', 'Object_61'])
            if cone == 4 and peca == 'esquerda':
                a['nota'] = ('no mapa montado, o passo 5 (a arena) já tirou a lasca desta janela: saem 21 faces, não 22 '
                             '(o vidro sai do mesmo jeito)')
        aberturas.append(a)
    dados = dict(
        sobre='Os quartos dos oito cones do Cozy Cone Motel por dentro (ferramentas/montar_quartos_do_cozy_cone.py). Eixos do '
              'jogo: x, y para cima, z. Rumo do jogo: 0 olhando para -z, crescendo para a direita (a frente é (sin r, -cos r)).',
        piso_y=PISO_Y,
        teto_y=round(TETO_Y, 3),
        cones=[dict(cone=Q.numero, eixo=r3(Q.eixo), rumo_da_porta=Q.rumo, espelhado=Q.espelho, mcqueen=Q.mcqueen,
                    raio_da_parede_no_piso=round(raio_da_parede(0.0), 3), raio_da_parede_no_teto=round(raio_da_parede(ALTURA), 3))
               for Q in QUARTOS],
        portas=portas,
        lugares=lugares,
        aberturas_para_tirar_da_casca=aberturas,
        nota_das_aberturas='Peças inteiras (faces ligadas por vértices) dentro de cada caixa, no modelo do Race-O-Rama x 4,5 '
                           '(objetos Object_60 e Object_61; ror_ no mapa montado). As caixas pegam só a peça: conferidas com '
                           'testar_aberturas.py, sem precisar de filtro por objeto. As portas saem do desenho e da colisão (o '
                           'carro entra dirigindo; os batentes ficam); os vidros saem só do desenho (a colisão do quarto já '
                           'fecha a parede por dentro).',
        colisao=dict(arquivo=os.path.basename(SAIDA) + '_colisao.glb', triangulos=TRIANGULOS_DA_COLISAO, **CONTAGEM_DA_COLISAO,
                     nota='por quarto: o piso em y = piso_y (com a soleira até 0,13 m por cima do começo da rampa da casca), a '
                          'parede de cone e a parede da porta até o teto (o vão da porta aberto), caixas em pé (sem tampa) em '
                          'volta do ar-condicionado, da mesinha e da luminária e um disco de teto em teto_y (6 m acima do '
                          'piso: não vira chão para o carro nem para os moradores; com ele a câmera sabe que está coberta). '
                          'No cone 4, paredes em pé de %.1f m na beira da base que a arena tinha cortado (a pele).' % ALTO_DA_BEIRA_DA_BASE),
        pele_do_cone_4=dict(PELE, nota='no mapa montado, a etapa da arena (passo 5 do montar_mundo_aberto.py) tira a metade de '
                                       'trás da casca e da base do cone 4, abaixo de y 7,56. O .glb põe de volta essas faces (as do '
                                       'cone 1 no mesmo lugar, com uma cópia da textura da casca, material cozy_casca, e na cor do '
                                       'vértice a luz que o shader dos mapas de dia daria a cada face) e a colisão, a beira da '
                                       'base. Se o mapa voltar a ter o cone 4 inteiro, rode esta ferramenta de novo (a pele some '
                                       'sozinha).'),
        luz_na_tela=MEDIDAS_DA_LUZ,
    )
    with open(SAIDA + '.json', 'w') as f:
        json.dump(dados, f, indent=2, ensure_ascii=False)
    print('Gravado %s' % (SAIDA + '.json'))
    return dados


if not NOITE:
    DADOS = montar_json()


# ------------------------------------------------------------------
# 8. Prévias
# ------------------------------------------------------------------
def tirar_pecas_da_casca(objs, mn, mx):
    """Tira das malhas do mapa as peças inteiras (faces ligadas por vértices) dentro da caixa (como o
    montar_mundo_aberto.py faz)"""
    folga = 0.05
    a, b = b_de(mn), b_de(mx)
    lo = np.minimum(np.array(a), np.array(b)) - folga
    hi = np.maximum(np.array(a), np.array(b)) + folga
    tiradas = 0
    for o in objs:
        mw = o.matrix_world
        bm = bmesh.new()
        bm.from_mesh(o.data)
        vistas, apagar = set(), []
        for f in bm.faces:
            if f in vistas:
                continue
            peca, pilha = [], [f]
            vistas.add(f)
            while pilha:
                g = pilha.pop()
                peca.append(g)
                for v in g.verts:
                    for h in v.link_faces:
                        if h not in vistas:
                            vistas.add(h)
                            pilha.append(h)
            if all(np.all((np.array(mw @ v.co) >= lo) & (np.array(mw @ v.co) <= hi)) for g in peca for v in g.verts):
                apagar.extend(peca)
        if apagar:
            bmesh.ops.delete(bm, geom=apagar, context='FACES')
            bm.to_mesh(o.data)
            tiradas += len(apagar)
        bm.free()
    return tiradas


def importar_casca():
    """Importa o mapa, guarda o que está perto do motel e tira as portas e os vidros dos cones"""
    if not os.path.exists(ARQUIVO_DO_MAPA):
        print('  sem o mapa (%s): prévias sem a casca' % ARQUIVO_DO_MAPA)
        return []
    antes = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=ARQUIVO_DO_MAPA)
    novos = [o for o in bpy.data.objects if o not in antes]
    bpy.context.view_layer.update()
    guardar = []
    meio = Vector((700.0, -215.0, 0))
    for o in novos:
        if o.type != 'MESH':
            continue
        bb = [o.matrix_world @ Vector(c) for c in o.bound_box]
        mn = Vector((min(v.x for v in bb), min(v.y for v in bb), min(v.z for v in bb)))
        mx = Vector((max(v.x for v in bb), max(v.y for v in bb), max(v.z for v in bb)))
        if mx.x > meio.x - 260 and mn.x < meio.x + 260 and mx.y > meio.y - 260 and mn.y < meio.y + 260:
            guardar.append(o)
    for o in novos:
        if o not in guardar:
            bpy.data.objects.remove(o, do_unlink=True)
    cones = [o for o in guardar if o.name.startswith('ror_Object_60') or o.name.startswith('ror_Object_61')]
    total = 0
    for cone, peca, mn, mx, faces in ABERTURAS_DA_CASCA:
        total += tirar_pecas_da_casca(cones, mn, mx)
    print('  casca: %d faces tiradas (portas e vidros, só na prévia)' % total)
    # como o jogo desenha o mapa (o shader dos mapas de dia, src/mundo.c): a textura vezes a luz da direção da
    # face, vezes 0,92 (sem as sombras; a conta do jogo é na escala de tela)
    sol = b_de(unit(SOL_DO_MAPA))
    for o in guardar:
        for slot in o.material_slots:
            m = slot.material
            if m is None or m.get('previa'):
                continue
            nt = m.node_tree
            bsdf = next((n for n in nt.nodes if n.type == 'BSDF_PRINCIPLED'), None)
            out = next((n for n in nt.nodes if n.type == 'OUTPUT_MATERIAL'), None)
            if not bsdf or not out:
                continue
            geo = nt.nodes.new('ShaderNodeNewGeometry')
            ponto = nt.nodes.new('ShaderNodeVectorMath')
            ponto.operation = 'DOT_PRODUCT'
            ponto.inputs[1].default_value = tuple(sol)
            nt.links.new(geo.outputs['True Normal'], ponto.inputs[0])
            sol_max = nt.nodes.new('ShaderNodeMath')
            sol_max.operation = 'MAXIMUM'
            sol_max.inputs[1].default_value = 0.0
            nt.links.new(ponto.outputs['Value'], sol_max.inputs[0])
            separa = nt.nodes.new('ShaderNodeSeparateXYZ')
            nt.links.new(geo.outputs['True Normal'], separa.inputs[0])
            cima = nt.nodes.new('ShaderNodeMath')
            cima.operation = 'MULTIPLY_ADD'                 # 0,30 (n.y/2 + 1/2) + 0,58 = 0,15 n.y + 0,73
            cima.inputs[1].default_value = 0.15
            cima.inputs[2].default_value = 0.73
            nt.links.new(separa.outputs['Z'], cima.inputs[0])
            luz = nt.nodes.new('ShaderNodeMath')
            luz.operation = 'MULTIPLY_ADD'
            luz.inputs[1].default_value = 0.38
            nt.links.new(sol_max.outputs[0], luz.inputs[0])
            nt.links.new(cima.outputs[0], luz.inputs[2])
            fator = nt.nodes.new('ShaderNodeMath')
            fator.operation = 'MULTIPLY'
            fator.inputs[1].default_value = 0.92
            nt.links.new(luz.outputs[0], fator.inputs[0])
            gama = nt.nodes.new('ShaderNodeMath')            # o jogo multiplica na tela; aqui a conta é linear
            gama.operation = 'POWER'
            gama.inputs[1].default_value = 2.2
            nt.links.new(fator.outputs[0], gama.inputs[0])
            fator = gama
            mult = nt.nodes.new('ShaderNodeMix')
            mult.data_type = 'RGBA'
            mult.blend_type = 'MULTIPLY'
            mult.inputs['Factor'].default_value = 1.0
            if bsdf.inputs['Base Color'].links:
                nt.links.new(bsdf.inputs['Base Color'].links[0].from_socket, mult.inputs['A'])
            else:
                mult.inputs['A'].default_value = bsdf.inputs['Base Color'].default_value
            nt.links.new(fator.outputs[0], mult.inputs['B'])
            em = nt.nodes.new('ShaderNodeEmission')
            nt.links.new(mult.outputs['Result'], em.inputs['Color'])
            nt.links.new(em.outputs[0], out.inputs['Surface'])
            m['previa'] = 1
    return guardar


def estacionar_mcqueen(Q):
    """O Relâmpago McQueen do jogo no lugar dele, de nariz para a porta (só na prévia, para a escala)"""
    raiz, objs, esc, _ = importar_carro('mcqueen', 4.4)
    for o in objs:
        for slot in o.material_slots:
            m = slot.material
            if m is None or m.get('previa'):
                continue
            nt = m.node_tree
            bsdf = next((n for n in nt.nodes if n.type == 'BSDF_PRINCIPLED'), None)
            out = next((n for n in nt.nodes if n.type == 'OUTPUT_MATERIAL'), None)
            if not bsdf or not out:
                continue
            em = nt.nodes.new('ShaderNodeEmission')
            em.inputs['Strength'].default_value = 0.85
            if bsdf.inputs['Base Color'].links:
                nt.links.new(bsdf.inputs['Base Color'].links[0].from_socket, em.inputs['Color'])
            else:
                em.inputs['Color'].default_value = bsdf.inputs['Base Color'].default_value
            nt.links.new(em.outputs[0], out.inputs['Surface'])
            m['previa'] = 1
    x, z = Q.xz(LUGAR_U, 0.0)
    raiz.location = b_de((x, PISO_Y + 0.012, z))
    raiz.rotation_euler = (0, 0, math.atan2(Q.f[0], Q.f[1]) + GIRO_DO_CARRO_NA_PREVIA)
    return raiz


GIRO_DO_CARRO_NA_PREVIA = 0.0


def camera_em(pos_jogo, alvo_jogo, lente=22, nome='cam'):
    sc = bpy.context.scene
    cam = bpy.data.cameras.new(nome)
    cam.lens = lente
    cam.clip_start = 0.1
    cam.clip_end = 2000
    co = bpy.data.objects.new(nome, cam)
    sc.collection.objects.link(co)
    p, a = b_de(pos_jogo), b_de(alvo_jogo)
    co.location = p
    co.rotation_euler = (a - p).to_track_quat('-Z', 'Y').to_euler()
    sc.camera = co
    return co


def renderizar_eevee(caminho, w, h):
    sc = bpy.context.scene
    sc.render.engine = 'BLENDER_EEVEE'
    sc.view_settings.view_transform = 'Standard'
    sc.view_settings.look = 'None'
    sc.eevee.taa_render_samples = 32
    sc.render.resolution_x, sc.render.resolution_y = w, h
    sc.render.resolution_percentage = 100
    sc.render.film_transparent = False
    sc.render.image_settings.file_format = 'PNG'
    sc.render.filepath = caminho
    bpy.ops.render.render(write_still=True)


def fazer_previas(objs):
    os.makedirs(PREVIAS, exist_ok=True)
    trocar_materiais(objs, material_de_previa)
    sc = bpy.context.scene
    for o in list(sc.objects):
        if o.type == 'LIGHT':
            o.hide_render = True
    sc.world.node_tree.nodes['Background'].inputs[0].default_value = (0.55, 0.72, 0.95, 1)
    sc.world.node_tree.nodes['Background'].inputs[1].default_value = 1.0
    casca = importar_casca()
    q1 = next(Q for Q in QUARTOS if Q.mcqueen)
    q4 = next(Q for Q in QUARTOS if Q.numero == CONE_DA_PELE)
    q7 = next(Q for Q in QUARTOS if Q.numero == 7)
    carro = estacionar_mcqueen(q1)
    jm = q1.janelas[1]
    vistas = [
        (q1, 'porta_de_dentro', (-4.7, 2.3, 0.4), (3.6, 1.6, 0.0), 17),
        (q1, 'janela_do_ar', (2.6, 1.0, 2.8), (-0.4, 1.7, -4.9), 19),
        (q1, 'mesinha', (1.8, 1.3, -1.4), (-2.3, 1.2, 4.2), 19),
        (q1, 'fundo', (3.3, 1.9, -0.6), (-5.0, 2.2, 0.2), 16),
        (q1, 'abajur', (1.0, 1.6, 0.6), (-1.4, 2.5, -4.2), 21),
        (q1, 'ar_de_perto', (0.6, 1.6, -2.2), (2.2, 1.3, -4.4), 24),
        (q1, 'como_no_filme', (3.0, 0.9, 1.6), (-1.2, 1.4, -3.6), 18),
        (q1, 'fora_porta', (16.0, 1.8, -2.0), (3.6, 1.7, 0.0), 24),
        (q1, 'fora_janela', tuple(jm.C + 4.5 * jm.n + np.array([0.0, -1.2, 0.0])), tuple(jm.C - 3.0 * jm.n), 28),
        (q1, 'tapete_de_perto', (1.6, 1.3, -2.9), (0.4, 0.0, -0.95), 30),
        (q1, 'quadros_de_perto', (0.5, 2.6, 0.0), (-4.6, 3.3, -1.2), 30),
        (q1, 'porta_de_fora_perto', (9.5, 2.2, 1.0), (0.0, 1.6, 0.0), 24),
        (q1, 'porta_por_baixo', (0.5, 1.4, 0.0), (3.6, 4.2, 0.0), 18),
        (q7, 'cone7_porta_de_dentro', (-4.7, 2.3, 0.4), (3.6, 1.6, 0.0), 17),
        (q7, 'cone7_janela_do_ar', (2.6, 1.0, 2.8), (-0.4, 1.7, -4.9), 19),
        (q4, 'cone4_de_tras', (-16.0, 3.2, 0.0), (0.0, 1.8, 0.0), 24),
        (q4, 'cone4_de_lado', (-10.0, 2.4, -10.0), (0.0, 1.8, 0.0), 24),
        (q4, 'cone4_dentro_para_porta', (-4.7, 2.3, 0.4), (3.6, 1.6, 0.0), 17),
    ]
    for Q, nome, pos, alvo, lente in vistas:
        carro.hide_render = Q is not q1
        for o in carro.children_recursive:
            o.hide_render = Q is not q1
        co = camera_em(Q.p(*pos), Q.p(*alvo), lente=lente, nome='cam_' + nome)
        renderizar_eevee(os.path.join(PREVIAS, nome + '.png'), 1280, 720)
        bpy.data.objects.remove(co, do_unlink=True)
        print('  prévia %s' % nome)
    # o motel visto da praça, com as portas abertas
    co = camera_em((664.0, 4.6, 182.0), (631.0, 4.2, 172.0), lente=26, nome='cam_motel')
    renderizar_eevee(os.path.join(PREVIAS, 'motel_da_praca.png'), 1280, 720)
    bpy.data.objects.remove(co, do_unlink=True)
    # de cima, a planta do cone do McQueen (cortada abaixo do teto)
    for o in casca:
        o.hide_render = True
    cam = bpy.data.cameras.new('cima')
    cam.type = 'ORTHO'
    cam.ortho_scale = 13.5
    cam.clip_start = 3.6           # só o que fica abaixo de 2,35 m (a parede se inclina por cima do resto)
    cam.clip_end = 200
    co = bpy.data.objects.new('cima', cam)
    sc.collection.objects.link(co)
    co.location = b_de(q1.p(0.0, ALTURA - 0.05, 0.0))
    co.rotation_euler = (0, 0, math.atan2(q1.f[0], q1.f[1]) - math.pi / 2)
    sc.camera = co
    renderizar_eevee(os.path.join(PREVIAS, 'planta.png'), 1000, 1000)
    print('  prévia planta')
    bpy.data.objects.remove(co, do_unlink=True)
    # só a luz assada (sem as texturas): as poças de luz, os cantos e se sobrou alguma listra
    trocar_materiais(objs, lambda m: material_de_previa(m, so_luz=True))
    carro.hide_render = True
    for o in carro.children_recursive:
        o.hide_render = True
    for nome, pos, alvo, lente in (('luz_parede_dos_quadros', (0.5, 2.6, 0.0), (-4.6, 3.3, -1.2), 22),
                                   ('luz_para_a_porta', (-4.7, 2.3, 0.4), (3.6, 1.6, 0.0), 17),
                                   ('luz_para_o_fundo', (3.2, 2.5, 0.3), (-5.0, 1.6, 0.0), 17)):
        co = camera_em(q1.p(*pos), q1.p(*alvo), lente=lente, nome='cam_' + nome)
        renderizar_eevee(os.path.join(PREVIAS, nome + '.png'), 1280, 720)
        bpy.data.objects.remove(co, do_unlink=True)
        print('  prévia %s' % nome)
    salvar_png(ATLAS, os.path.join(PREVIAS, 'atlas_dos_quadros.png'))


if PREVIAS and not NOITE:
    print('Prévias em %s...' % PREVIAS)
    fazer_previas(OBJS)

# ------------------------------------------------------------------
# 9. A noite (--noite): <saida>_noite.glb e <saida>_noite.json
# ------------------------------------------------------------------
# O jogo mistura a luz de dia e a de noite vértice a vértice (a primitiva i e o vértice j de um casam com os do
# outro). Por isso o _noite.glb é uma cópia byte a byte do .glb de dia (as mesmas malhas e primitivas, na mesma
# ordem, os mesmos vértices, posições, coordenadas de textura, índices e texturas) em que só o COLOR_0 muda. As
# cores vêm do desenho assado de noite (a mesma montagem, com as luzes da noite), exportado à parte: o
# exportador separa os vértices onde a cor muda, e as de noite mudam em outros lugares, então cada vértice do
# dia leva a média das cores de noite dos cantos dos triângulos que caem nele (os triângulos saem na mesma
# ordem nos dois; se não, o par de cada um é achado pelas posições dos três cantos).
TIPOS_DO_GLB = {5121: np.uint8, 5123: np.uint16, 5125: np.uint32, 5126: np.float32}
COMPONENTES_DO_GLB = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3, 'VEC4': 4}


def onde_no_glb(j, d, k):
    """Onde ficam no arquivo os dados do accessor k: (início, tipo, componentes, quantos)"""
    n_json = struct.unpack('<I', d[12:16])[0]
    acc = j['accessors'][k]
    bv = j['bufferViews'][acc['bufferView']]
    dt = np.dtype(TIPOS_DO_GLB[acc['componentType']]).newbyteorder('<')
    c = COMPONENTES_DO_GLB[acc['type']]
    if bv.get('byteStride', c * dt.itemsize) != c * dt.itemsize:
        raise SystemExit('ERRO: o accessor %d é intercalado (byteStride)' % k)
    return 20 + n_json + 8 + bv.get('byteOffset', 0) + acc.get('byteOffset', 0), dt, c, acc['count']


def ler_acessor(j, d, k):
    ini, dt, c, n = onde_no_glb(j, d, k)
    return np.frombuffer(d, dt, count=n * c, offset=ini).reshape(n, c)


def bytes_do_acessor(j, d, k):
    ini, dt, c, n = onde_no_glb(j, d, k)
    return d[ini:ini + n * c * dt.itemsize]


def cor_do_acessor(j, d, k):
    a = ler_acessor(j, d, k).astype(np.float64)
    return a / {5121: 255.0, 5123: 65535.0}.get(j['accessors'][k]['componentType'], 1.0)


def pares_dos_cantos(Pd, Id, Pn, In):
    """Para cada canto dos triângulos do dia (Id), o vértice de noite do canto que é o par dele (-1 se nenhum).
    Os mesmos triângulos na mesma ordem: canto a canto. Se não, o par de cada triângulo pelas posições dos três
    cantos (em milímetros, a partir do menor, na mesma volta)"""
    if len(Id) == len(In) and np.abs(Pd[Id] - Pn[In]).max() < 1e-4:
        return In.copy()

    def chaves(P, I):
        q = np.round(P[I] * 1000).astype(np.int64).reshape(-1, 3, 3)
        ks, giros = [], []
        for t in q:
            g = min(range(3), key=lambda r: tuple(t[r]))
            ks.append(np.roll(t, -g, axis=0).tobytes())
            giros.append(g)
        return ks, giros
    kd, gd = chaves(Pd, Id)
    kn, gn = chaves(Pn, In)
    livres = {}
    for t, k in enumerate(kn):
        livres.setdefault(k, []).append(t)
    par = np.full(len(Id), -1, np.int64)
    for t, k in enumerate(kd):
        lista = livres.get(k)
        if not lista:
            continue
        u = lista.pop(0) if len(lista) > 1 else lista[0]
        for r in range(3):
            par[3 * t + (gd[t] + r) % 3] = In[3 * u + (gn[u] + r) % 3]
    return par


def transplantar_cores(dia, noite_assada, saida):
    """Grava em 'saida' o .glb de dia com o COLOR_0 de cada primitiva trocado pelas cores do assado de noite"""
    from mathutils.kdtree import KDTree
    jd, dd = ler_glb(dia)
    jn, dn = ler_glb(noite_assada)
    novo = bytearray(dd)
    noite_por_nome = {m['name']: m for m in jn['meshes']}
    print('A noite: as cores do assado de noite na geometria de %s' % dia)
    for md in jd['meshes']:
        mn = noite_por_nome.get(md['name'])
        if mn is None or len(mn['primitives']) != len(md['primitives']):
            raise SystemExit('ERRO: a malha %s do dia não tem par no assado de noite' % md['name'])
        for k, (pd, pn) in enumerate(zip(md['primitives'], mn['primitives'])):
            Pd = ler_acessor(jd, dd, pd['attributes']['POSITION']).astype(np.float64)
            Id = ler_acessor(jd, dd, pd['indices']).ravel().astype(np.int64)
            Pn = ler_acessor(jn, dn, pn['attributes']['POSITION']).astype(np.float64)
            In = ler_acessor(jn, dn, pn['indices']).ravel().astype(np.int64)
            Cn = cor_do_acessor(jn, dn, pn['attributes']['COLOR_0'])[:, :3]
            par = pares_dos_cantos(Pd, Id, Pn, In)
            em_ordem = len(Id) == len(In) and bool(np.array_equal(par, In))
            ok = par >= 0
            soma = np.zeros((len(Pd), 3))
            conta = np.zeros(len(Pd))
            np.add.at(soma, Id[ok], Cn[par[ok]])
            np.add.at(conta, Id[ok], 1)
            sem = np.flatnonzero(conta == 0)
            if len(sem):
                arv = KDTree(len(Pn))
                for i, p in enumerate(Pn):
                    arv.insert(tuple(p), i)
                arv.balance()
                for v in sem:
                    _, i, _ = arv.find(tuple(Pd[v]))
                    soma[v] = Cn[i]
                    conta[v] = 1
            cor = np.clip(soma / conta[:, None], 0, 1)
            kc = pd['attributes']['COLOR_0']
            ini, dt, c, n = onde_no_glb(jd, dd, kc)
            C = ler_acessor(jd, dd, kc).copy()
            if dt.kind == 'u':
                C[:, :3] = np.round(cor * np.iinfo(dt).max).astype(dt)
            else:
                C[:, :3] = cor
            novo[ini:ini + n * c * dt.itemsize] = C.astype(dt).tobytes()
            print('  %-24s primitiva %d: %6d vértices (%6d no assado de noite), %6d triângulos; %s; %d vértices sem '
                  'par (cor do vizinho mais perto)' % (md['name'], k, len(Pd), len(Pn), len(Id) // 3,
                                                       'os mesmos triângulos na mesma ordem' if em_ordem else
                                                       'pares achados pelas posições (%d cantos de %d)' % (ok.sum(), len(Id)),
                                                       len(sem)))
    with open(saida, 'wb') as f:
        f.write(novo)
    print('Gravado %s' % saida)


def conferir_noite(dia, noite):
    """Lê os dois .glb e confere que só o COLOR_0 mudou: o mesmo JSON (malhas, primitivas, accessors, materiais,
    texturas) e, por primitiva, os mesmos bytes de POSITION, TEXCOORD_0 e índices; fora dos COLOR_0, o arquivo
    inteiro igual byte a byte (as texturas embutidas também)"""
    jd, dd = ler_glb(dia)
    jn, dn = ler_glb(noite)
    n_json = struct.unpack('<I', dd[12:16])[0]
    if jd != jn or len(dd) != len(dn) or dd[:20 + n_json] != dn[:20 + n_json]:
        raise SystemExit('ERRO: o JSON do _noite.glb não é o do .glb de dia')
    fora = np.ones(len(dd), bool)
    print('Conferindo %s com %s:' % (os.path.basename(noite), os.path.basename(dia)))
    for i, (md, mn) in enumerate(zip(jd['meshes'], jn['meshes'])):
        for k, (pd, pn) in enumerate(zip(md['primitives'], mn['primitives'])):
            iguais = []
            for nome in ('POSITION', 'TEXCOORD_0', 'indices'):
                a = pd['indices'] if nome == 'indices' else pd['attributes'].get(nome)
                b = pn['indices'] if nome == 'indices' else pn['attributes'].get(nome)
                if a is None and b is None:
                    continue
                if a is None or b is None or bytes_do_acessor(jd, dd, a) != bytes_do_acessor(jn, dn, b):
                    raise SystemExit('ERRO: %s da primitiva %d da malha %s mudou' % (nome, k, md['name']))
                iguais.append(nome)
            kc = pd['attributes']['COLOR_0']
            ini, dt, c, n = onde_no_glb(jd, dd, kc)
            fora[ini:ini + n * c * dt.itemsize] = False
            cd = cor_do_acessor(jd, dd, kc)[:, :3]
            cn = cor_do_acessor(jn, dn, kc)[:, :3]
            mudou = 100.0 * np.mean(np.any(cd != cn, axis=1))
            print('  malha %2d %-24s p%d: %6d vértices, %6d triângulos; iguais: %s; COLOR_0 mudou em %3.0f%% dos vértices '
                  '(média dia %s, noite %s)' % (i, md['name'], k, n, jd['accessors'][pd['indices']]['count'] // 3,
                                               ', '.join(iguais), mudou, np.round(cd.mean(0), 3), np.round(cn.mean(0), 3)))
    a = np.frombuffer(dd, np.uint8)
    b = np.frombuffer(dn, np.uint8)
    diferentes = int(np.count_nonzero(a[fora] != b[fora]))
    print('  fora dos COLOR_0: %d bytes conferidos, %d diferentes' % (int(fora.sum()), diferentes))
    if diferentes:
        raise SystemExit('ERRO: o _noite.glb difere do .glb de dia fora dos COLOR_0')


def gravar_luzes_de_fora(luzes):
    """<saida>_noite.json: as luzes pontuais (eixos do jogo) que a luz de dentro joga para fora pelas portas e
    janelas, no chão e nas paredes em volta do prédio. Cada uma: lugar, cor (0 a 1), raio (metros: até onde ela
    chega) e força, como as Lampada de src/hora.h (o jogo usa as mais perto da câmera)"""
    dados = {'luzes': [dict(x=round(float(p[0]), 3), y=round(float(p[1]), 3), z=round(float(p[2]), 3),
                            r=round(float(c[0]), 3), g=round(float(c[1]), 3), b=round(float(c[2]), 3),
                            raio=round(float(raio), 2), forca=round(float(forca), 2))
                       for p, c, raio, forca in luzes]}
    caminho = SAIDA + '_noite.json'
    with open(caminho, 'w') as f:
        json.dump(dados, f, indent=2, ensure_ascii=False)
    print('Gravado %s: %d luzes' % (caminho, len(luzes)))
    for l in dados['luzes']:
        print('  (%.2f, %.2f, %.2f) cor (%.2f, %.2f, %.2f) raio %.1f força %.2f' % tuple(l.values()))


# As prévias de noite: o .glb de dia e o _noite.glb como o jogo os carrega (textura × cor do vértice), lado a
# lado, com a casca do mapa: de dia como as prévias de dia; de noite, com a luz que o shader dos mapas
# (src/mundo.c) dá à meia-noite (a chave das 0h de src/hora.c, com a lua) mais as lâmpadas do _noite.json
# (a conta de luzDasLampadas, src/hora.h)
LUA_DO_JOGO = (0.25, 0.75, -0.6)
BASE_DA_NOITE_DO_JOGO = (0.34, 0.35, 0.53)
CEU_DA_NOITE_DO_JOGO = (0.23, 0.25, 0.45)
LUAR_DO_JOGO = (0.24, 0.28, 0.46)
FUNDO_DA_NOITE = (0.012, 0.010, 0.040)     # o céu da noite atrás (linear)


def material_do_jogo(nome, img, atributo):
    """Textura × cor do vértice (o atributo do .glb importado), sem luz: na tela, emissão de (t·c)^2,2"""
    mat = novo_material(nome)
    nt = mat.node_tree
    nt.nodes.clear()
    saida = nt.nodes.new('ShaderNodeOutputMaterial')
    at = nt.nodes.new('ShaderNodeAttribute')
    at.attribute_name = atributo
    gama = nt.nodes.new('ShaderNodeGamma')
    gama.inputs['Gamma'].default_value = 2.2
    nt.links.new(at.outputs['Color'], gama.inputs['Color'])
    cor = gama.outputs['Color']
    if img is not None:
        tex = nt.nodes.new('ShaderNodeTexImage')
        tex.image = img
        tex.interpolation = 'Linear'
        mult = nt.nodes.new('ShaderNodeMix')
        mult.data_type = 'RGBA'
        mult.blend_type = 'MULTIPLY'
        mult.inputs['Factor'].default_value = 1.0
        nt.links.new(cor, mult.inputs['A'])
        nt.links.new(tex.outputs['Color'], mult.inputs['B'])
        cor = mult.outputs['Result']
    em = nt.nodes.new('ShaderNodeEmission')
    nt.links.new(cor, em.inputs['Color'])
    nt.links.new(em.outputs[0], saida.inputs['Surface'])
    return mat


def importar_para_previa(caminho):
    """Importa um .glb do interior (o que o jogo carrega) com o material do jogo. Devolve os objetos"""
    antes = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=caminho)
    novos = [o for o in bpy.data.objects if o not in antes]
    malhas = [o for o in novos if o.type == 'MESH']
    for o in novos:
        if o.type != 'MESH':
            bpy.data.objects.remove(o, do_unlink=True)
    for o in malhas:
        me = o.data
        m = o.material_slots[0].material if o.material_slots else None
        img = next((n.image for n in m.node_tree.nodes if n.type == 'TEX_IMAGE'), None) if m and m.node_tree else None
        me.materials.clear()
        me.materials.append(material_do_jogo('jogo_' + o.name, img, me.color_attributes[0].name))
    return malhas


def grupo_da_luz_da_noite(luzes):
    """Um grupo de nós: a luz que o jogo dá ao mapa à meia-noite num ponto (com as lâmpadas), na tela, elevada a
    2,2 (a prévia faz a conta em linear)"""
    g = bpy.data.node_groups.new('luz_da_noite', 'ShaderNodeTree')
    g.interface.new_socket('Luz', in_out='OUTPUT', socket_type='NodeSocketColor')
    N, L = g.nodes, g.links
    geo = N.new('ShaderNodeNewGeometry')
    n = geo.outputs['True Normal']

    def vet(op, a, b=None, escala=None):
        no = N.new('ShaderNodeVectorMath')
        no.operation = op
        for k, x in enumerate((a, b)):
            if x is None:
                continue
            if isinstance(x, bpy.types.NodeSocket):
                L.new(x, no.inputs[k])
            else:
                no.inputs[k].default_value = tuple(map(float, x))
        if escala is not None:
            if isinstance(escala, bpy.types.NodeSocket):
                L.new(escala, no.inputs['Scale'])
            else:
                no.inputs['Scale'].default_value = float(escala)
        return no.outputs['Value'] if op in ('DOT_PRODUCT', 'LENGTH', 'DISTANCE') else no.outputs['Vector']

    def mat_(op, a, b=None, c=None):
        no = N.new('ShaderNodeMath')
        no.operation = op
        for k, x in enumerate((a, b, c)):
            if x is None:
                continue
            if isinstance(x, bpy.types.NodeSocket):
                L.new(x, no.inputs[k])
            else:
                no.inputs[k].default_value = float(x)
        return no.outputs[0]
    sep = N.new('ShaderNodeSeparateXYZ')
    L.new(n, sep.inputs[0])
    cima = mat_('MULTIPLY_ADD', sep.outputs['Z'], 0.5, 0.5)
    total = vet('ADD', [0.58 * x for x in BASE_DA_NOITE_DO_JOGO], vet('SCALE', [0.30 * x for x in CEU_DA_NOITE_DO_JOGO], escala=cima))
    lua = mat_('MAXIMUM', vet('DOT_PRODUCT', n, tuple(b_de(unit(LUA_DO_JOGO)))), 0.0)
    total = vet('ADD', total, vet('SCALE', [0.38 * x for x in LUAR_DO_JOGO], escala=lua))
    for p, cor, raio, forca in luzes:
        d = vet('SUBTRACT', tuple(b_de(p)), geo.outputs['Position'])
        q = mat_('MULTIPLY', vet('DOT_PRODUCT', d, d), 1.0 / (raio * raio))
        a = mat_('MAXIMUM', mat_('SUBTRACT', 1.0, q), 0.0)
        a2 = mat_('MULTIPLY', a, a)
        virado = mat_('MULTIPLY_ADD', mat_('MAXIMUM', vet('DOT_PRODUCT', n, vet('NORMALIZE', d)), 0.0), 0.75, 0.25)
        total = vet('ADD', total, vet('SCALE', [forca * x for x in cor], escala=mat_('MULTIPLY', a2, virado)))
    total = vet('SCALE', total, escala=0.92)
    gama = N.new('ShaderNodeGamma')
    gama.inputs['Gamma'].default_value = 2.2
    L.new(total, gama.inputs['Color'])
    out = N.new('NodeGroupOutput')
    L.new(gama.outputs['Color'], out.inputs[0])
    return g


def casca_de_noite(casca, luzes):
    """Põe nos materiais da casca (já com a emissão das prévias de dia) a emissão de noite: a cor da textura
    vezes a luz da noite do jogo. Devolve as trocas, para hora_da_casca"""
    grupo = grupo_da_luz_da_noite(luzes)
    trocas = []
    feitos = set()
    for o in casca:
        for slot in o.material_slots:
            m = slot.material
            if m is None or m.name in feitos or m.node_tree is None:
                continue
            feitos.add(m.name)
            nt = m.node_tree
            out = next((x for x in nt.nodes if x.type == 'OUTPUT_MATERIAL' and x.inputs['Surface'].links), None)
            bsdf = next((x for x in nt.nodes if x.type == 'BSDF_PRINCIPLED'), None)
            if out is None or bsdf is None:
                continue
            dia = out.inputs['Surface'].links[0].from_socket
            mult = nt.nodes.new('ShaderNodeMix')
            mult.data_type = 'RGBA'
            mult.blend_type = 'MULTIPLY'
            mult.inputs['Factor'].default_value = 1.0
            if bsdf.inputs['Base Color'].links:
                nt.links.new(bsdf.inputs['Base Color'].links[0].from_socket, mult.inputs['A'])
            else:
                mult.inputs['A'].default_value = bsdf.inputs['Base Color'].default_value
            gn = nt.nodes.new('ShaderNodeGroup')
            gn.node_tree = grupo
            nt.links.new(gn.outputs[0], mult.inputs['B'])
            em = nt.nodes.new('ShaderNodeEmission')
            nt.links.new(mult.outputs['Result'], em.inputs['Color'])
            trocas.append((nt, out, dia, em.outputs[0]))
    return trocas


def hora_da_casca(trocas, noite):
    for nt, out, dia, noite_ in trocas:
        nt.links.new(noite_ if noite else dia, out.inputs['Surface'])


def previas_dia_e_noite(vistas, casca, luzes, esconder, w=960, h=540):
    """Cada vista (nome, posição, alvo, lente; eixos do jogo) renderizada com o .glb de dia e com o _noite.glb,
    lado a lado (dia à esquerda), em PREVIAS/<nome>.png"""
    os.makedirs(PREVIAS, exist_ok=True)
    sc = bpy.context.scene
    for o in list(sc.objects):
        if o.type == 'LIGHT' or o in esconder:
            o.hide_render = True
    dia = importar_para_previa(SAIDA + '.glb')
    noite = importar_para_previa(SAIDA + '_noite.glb')
    trocas = casca_de_noite(casca, luzes)
    if sc.world is None:
        sc.world = novo_mundo('ceu_das_previas')
    ceu = sc.world.node_tree.nodes['Background']
    for nome, pos, alvo, lente in vistas:
        co = camera_em(pos, alvo, lente=lente, nome='cam_' + nome)
        imgs = []
        for de_noite in (False, True):
            for o in dia:
                o.hide_render = de_noite
            for o in noite:
                o.hide_render = not de_noite
            hora_da_casca(trocas, de_noite)
            ceu.inputs[0].default_value = (*FUNDO_DA_NOITE, 1) if de_noite else (0.55, 0.72, 0.95, 1)
            ceu.inputs[1].default_value = 1.0
            arq = os.path.join(PASTA_TEMP, 'previa_%s_%d.png' % (nome, de_noite))
            renderizar_eevee(arq, w, h)
            imgs.append(ler_png(arq)[..., :3])
        salvar_png(np.concatenate([imgs[0], np.ones((h, 6, 3), np.float32), imgs[1]], axis=1),
                   os.path.join(PREVIAS, nome + '.png'))
        bpy.data.objects.remove(co, do_unlink=True)
        print('  prévia %s (dia | noite)' % nome)


def luzes_de_fora():
    """Uma por cone: a luz do quarto saindo pela porta, um pouco fora da boca do nicho, sobre a rampa"""
    meio_v = (PORTA_V[0] + PORTA_V[1]) / 2
    return [(tuple(Q.p(6.2, 1.1, meio_v)), (1.0, 0.70, 0.42), 7.0, 1.0) for Q in QUARTOS]


def pele_de_noite_no_glb(caminho, dia):
    """A pele do cone 4 no _noite.glb: a luz que o shader dos mapas dá à meia-noite, face a face, calculada na
    própria geometria do .glb (com a mesma regra de montar_pele_do_cone_4 para o lado que se vê). Fica exata
    mesmo que o mapa de agora devolva outras faces que o mapa do dia (sem luz de dentro: não é assada). Confere
    que a mesma conta com a luz de dia dá o COLOR_0 do dia"""
    j, d = ler_glb(caminho)
    jd, dd = ler_glb(dia)            # o mesmo JSON (os mesmos accessors): só para conferir o COLOR_0 do dia
    novo = bytearray(d)
    alvo = next(Q for Q in QUARTOS if Q.numero == CONE_DA_PELE)
    for me in j['meshes']:
        if not me['name'].startswith('cozy_casca'):
            continue
        for p in me['primitives']:
            P = ler_acessor(j, d, p['attributes']['POSITION']).astype(np.float64)
            I = ler_acessor(j, d, p['indices']).ravel().astype(np.int64).reshape(-1, 3)
            kc = p['attributes']['COLOR_0']
            C = ler_acessor(j, d, kc).copy()
            noite, dia, conta = np.zeros((len(P), 3)), np.zeros(len(P)), np.zeros(len(P))
            for t in I:
                A = P[t]
                nrm = np.cross(A[1] - A[0], A[2] - A[0])
                nrm /= np.linalg.norm(nrm) + 1e-12
                fora = A.mean(0)[[0, 2]] - alvo.eixo
                if abs(nrm[1]) > 0.9:
                    vista = nrm * np.sign(nrm[1])
                else:
                    vista = nrm * (1.0 if np.dot(nrm[[0, 2]], fora) >= 0 else -1.0)
                noite[t] += np.minimum(luz_do_mapa_de_noite(vista), 1.0)
                dia[t] += min(luz_do_mapa(vista), 1.0)
                conta[t] += 1
            usados = conta > 0
            noite[usados] /= conta[usados, None]
            dia[usados] /= conta[usados]
            ini, dt, c, n = onde_no_glb(j, d, kc)
            erro_do_dia = np.abs(cor_do_acessor(jd, dd, kc)[usados, :3] - dia[usados, None]).max()
            C[usados, :3] = np.round(noite[usados] * np.iinfo(dt).max).astype(dt)
            novo[ini:ini + n * c * dt.itemsize] = C.astype(dt).tobytes()
            print('  pele do cone %d (%s): %d vértices, %d triângulos com a luz do mapa à meia-noite (média %s); a mesma '
                  'conta com a luz de dia difere do COLOR_0 do dia em até %.4f' % (
                      CONE_DA_PELE, me['name'], int(usados.sum()), len(I), np.round(noite[usados].mean(0), 3), erro_do_dia))
    with open(caminho, 'wb') as f:
        f.write(novo)


if NOITE:
    transplantar_cores(SAIDA + '.glb', os.path.join(PASTA_TEMP, 'noite_assada.glb'), SAIDA + '_noite.glb')
    pele_de_noite_no_glb(SAIDA + '_noite.glb', SAIDA + '.glb')
    conferir_noite(SAIDA + '.glb', SAIDA + '_noite.glb')
    LUZES_DE_FORA = luzes_de_fora()
    gravar_luzes_de_fora(LUZES_DE_FORA)
    if PREVIAS:
        print('Prévias (dia | noite) em %s...' % PREVIAS)
        casca = importar_casca()
        q1 = next(Q for Q in QUARTOS if Q.mcqueen)
        q4 = next(Q for Q in QUARTOS if Q.numero == CONE_DA_PELE)
        q7 = next(Q for Q in QUARTOS if Q.numero == 7)
        jm = q1.janelas[1]
        vistas = [
            (q1, 'porta_de_dentro', (-4.7, 2.3, 0.4), (3.6, 1.6, 0.0), 17),
            (q1, 'janela_do_ar', (2.6, 1.0, 2.8), (-0.4, 1.7, -4.9), 19),
            (q1, 'mesinha', (1.8, 1.3, -1.4), (-2.3, 1.2, 4.2), 19),
            (q1, 'fundo', (3.3, 1.9, -0.6), (-5.0, 2.2, 0.2), 16),
            (q1, 'abajur', (1.0, 1.6, 0.6), (-1.4, 2.5, -4.2), 21),
            (q1, 'como_no_filme', (3.0, 0.9, 1.6), (-1.2, 1.4, -3.6), 18),
            (q1, 'fora_porta', (16.0, 1.8, -2.0), (3.6, 1.7, 0.0), 24),
            (q1, 'porta_de_fora_perto', (9.5, 2.2, 1.0), (0.0, 1.6, 0.0), 24),
            (q1, 'fora_janela', tuple(jm.C + 4.5 * jm.n + np.array([0.0, -1.2, 0.0])), tuple(jm.C - 3.0 * jm.n), 28),
            (q7, 'cone7_porta_de_dentro', (-4.7, 2.3, 0.4), (3.6, 1.6, 0.0), 17),
            (q4, 'cone4_de_tras', (-16.0, 3.2, 0.0), (0.0, 1.8, 0.0), 24),
            (q4, 'cone4_de_lado', (-10.0, 2.4, -10.0), (0.0, 1.8, 0.0), 24),
        ]
        previas_dia_e_noite([(nome, tuple(Q.p(*pos)), tuple(Q.p(*alvo)), lente) for Q, nome, pos, alvo, lente in vistas] +
                            [('motel_da_praca', (664.0, 4.6, 182.0), (631.0, 4.2, 172.0), 26)],
                            casca, LUZES_DE_FORA, OBJS)

shutil.rmtree(PASTA_TEMP, ignore_errors=True)
print('Pronto em %.0f s.' % (time.time() - INICIO))
