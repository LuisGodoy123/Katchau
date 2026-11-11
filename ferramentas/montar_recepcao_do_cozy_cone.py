"""
montar_recepcao_do_cozy_cone.py
------------------------------------------------------------------
Monta o lado de dentro da recepção (o escritório) do Cozy Cone Motel, onde a
Sally fica (do filme Carros, 2006), no lugar do prédio dela no mapa do mundo
aberto (recursos/modelos/radiator_springs.glb), já nas coordenadas do mundo
do jogo (x, y para cima, z):

    Blender -b --factory-startup -P ferramentas/montar_recepcao_do_cozy_cone.py -- \\
        recursos/modelos/radiator_springs.glb recursos/modelos/cozy_cone_recepcao \\
        [--previas pasta] [--amostras 384] [--quadros atlas.png] [--noite]

Grava, com o nome dado no segundo argumento:
  cozy_cone_recepcao.glb          o desenho, com a luz assada nas cores dos vértices
                                  (COLOR_0): o jogo o desenha com o shader padrão da
                                  raylib (textura × cor do vértice), sem luz nenhuma;
  cozy_cone_recepcao_colisao.glb  a colisão: o piso e, em faces em pé, as paredes (até o
                                  teto) e os móveis (caixas sem tampa, até o alto dos que
                                  são mais altos que a câmera); a entrada aberta;
  cozy_cone_recepcao.json         a entrada (centro e direção), o lugar da Sally (atrás do
                                  balcão, de frente para a entrada) com o caminho livre até
                                  ele, e o que se tira da casca do prédio no mapa: peças
                                  inteiras por caixa (os vidros dos lados, o piso velho, o
                                  balcão e os armários do Race-O-Rama) e vãos por recorte (os
                                  vidros da frente e dos fundos, cuja caixa também conteria a
                                  moldura da casca que fica na frente deles).

O mapa (primeiro argumento) entra no assado da luz (o telhado, a marquise e os
pilares fazem sombra; a praça rebate o sol) e nas prévias; sem ele, a luz é
assada só com as paredes daqui. A casca recebe as mesmas aberturas que o
integrador (montar_mundo_aberto.py) vai fazer, com as mesmas regras, e a
ferramenta confere o resultado (as faces de cada caixa; a área de cada recorte,
só do vidro) e que o forro da casca fica acima do teto novo. Com --previas, a
ferramenta grava na pasta as vistas de dentro e de fora, pela entrada e pelas
vitrines, com a casca do prédio (sem os vidros) e a Sally no lugar dela,
mostrando o que o jogo vai mostrar: as cores assadas × as texturas. --quadros
guarda (ou, se o arquivo já existe, reaproveita) o atlas dos quadros, que é o
passo mais demorado. --amostras é o número de amostras do Cycles por ponto na
luz assada.

O que ela faz:
  1. Desenha as texturas (em PNG, porque a raylib do jogo não lê JPEG): o piso
     de ladrilhos em xadrez creme e rosado e as placas do teto.
  2. Pinta os quadros: cenas pequenas renderizadas no Blender (as mesas de
     rocha ao pôr do sol, a Rota 66, o cone de noite, os cactos, a Sally, o
     McQueen e o Mater dos modelos do jogo, pintados "de cartaz", e uma foto antiga
     em preto e branco), o cartaz do
     Cozy Cone Motel, a plaquinha "Honk for Service", o calendário, o capacho e
     o tapete; tudo num atlas de 2048 x 2048.
  3. Monta a sala por dentro da casca: paredes cor de pêssego uns centímetros
     para dentro dela, com as aberturas no lugar das da casca (as vitrines e as
     bandeiras da frente, que é inclinada, as janelas dos lados e dos fundos, a
     entrada do meio), os rebaixos de madeira até o plano dos vidros, os
     caixilhos e os peitoris, o friso de triângulos laranja e amarelos no pé
     das paredes, as pilastras dos cantos, o teto de placas inclinado como o
     telhado, com vigas, e o piso de ladrilhos com o capacho e o tapete. Os
     móveis do filme: o balcão de rolos pretos acolchoados com a prateleira
     redonda de cones e miniaturas na ponta, a plaquinha, o ventilador de
     fitinhas e o abajur-cone em cima; a estante branca embutida dos fundos
     (quase até o teto, em cinco seções), cheia de cones grandes, pirâmides de
     cones, quadros e o relógio sol-raiado de setas; as luminárias pendentes de
     cone; os cactos com costelas e espinhos (palma, mandacaru, saguaro, bola) em
     suportes de ferro batido; as heras em vasos-cone pendurados; as pilhas de
     cones dos dois cantos dos fundos; as prateleiras baixas de cones embaixo das
     janelas; o cartaz, os quadros e as flâmulas das paredes dos lados; o
     letreiro OFFICE de neon (tubos grossos com o halo) na vitrine de oeste (a
     da direita de quem olha da praça, como no filme), virado para fora, com
     cones num estrado atrás dele; os cactos na vitrine de leste; a cortina
     franzida no vão do meio dos fundos (atrás da estante, vista de fora).
  4. Assa a luz no Cycles (difusa direta e indireta, sem a cor) e a oclusão de
     ambiente nas cores dos vértices: pouco céu e o sol do jogo entram pelas
     janelas (com a casca do mapa fazendo sombra); as luminárias de cone fazem
     poças de luz quente (as de perto das paredes iluminam a parede), dois focos
     iluminam a estante e um o balcão, o abajur, o neon tinge de laranja a
     vitrine dele. As superfícies grandes ficam numa grade de 0,3 m cortada em
     níveis globais (sem junções em T); a luz é alisada entre vizinhos de mesma
     normal, levada à tela numa exposição fixa (as poças do piso), com contraste,
     vezes a oclusão (os cantos, o pé dos móveis e as junções escurecem) e vezes
     a cor sem textura. O piso encerado ganha um reflexo falso (o raio refletido
     de um olho médio, lançado na sala).
  5. Exporta: poucos materiais (um sem textura e um por textura), cada malha
     com menos de 65 mil vértices (a raylib usa índices de 16 bits), e confere
     no .glb que todas as primitivas têm COLOR_0.

Com --noite, a ferramenta faz a luz de NOITE (o jogo mistura a de dia e a de
noite vértice a vértice, conforme a hora) e grava só:
  cozy_cone_recepcao_noite.glb    o .glb de dia (que já tem de existir) copiado
                                  byte a byte, com outro COLOR_0: as mesmas malhas,
                                  primitivas, vértices, posições, coordenadas de
                                  textura, índices e texturas. A montagem é a mesma;
                                  a luz assada é sem sol, com um luar fraco
                                  azul-arroxeado pelas janelas e as luzes de dentro
                                  todas acesas e mais presentes (as cúpulas, os
                                  abajures e o letreiro OFFICE, que tinge de laranja a
                                  vitrine e a sala em volta), com mais contraste; o
                                  reflexo do piso mostra a noite lá fora. O assado de
                                  noite é exportado à parte e as cores dele passam
                                  para os vértices do .glb de dia (casando os
                                  triângulos), e a ferramenta confere que só o
                                  COLOR_0 mudou;
  cozy_cone_recepcao_noite.json   as luzes pontuais que a recepção joga para fora de
                                  noite (eixos do jogo; lugar, cor, raio e força).
O .glb de dia, a colisão e o .json ficam como estão. Sem --quadros, o atlas dos
quadros vem do .glb de dia. O mapa já montado (com as aberturas da recepção
tiradas pelo integrador) também serve: de noite, a abertura que não acha nada na
casca é dada como já aberta. Com --previas, as vistas saem com o dia e a noite
lado a lado (os dois .glb como o jogo os carrega; de noite, a casca do mapa com a
luz da meia-noite do jogo e as luzes do _noite.json).

Eixos: no Blender, x = x do jogo, y = -z do jogo e z = y do jogo; o .glb sai
com +Y para cima, ou seja, nas coordenadas do jogo.
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
SAIDA = posicionais[1] if len(posicionais) > 1 else os.path.join(REPO, 'recursos', 'modelos', 'cozy_cone_recepcao')
if SAIDA.endswith('.glb'):
    SAIDA = SAIDA[:-4]
PREVIAS = opcoes.get('--previas')
AMOSTRAS = int(opcoes.get('--amostras', 384))
ATLAS_GUARDADO = opcoes.get('--quadros')
if NOITE and not os.path.exists(SAIDA + '.glb'):
    raise SystemExit('ERRO: --noite precisa do .glb de dia (%s.glb): a noite usa a geometria dele' % SAIDA)

# ------------------------------------------------------------------
# O prédio no mapa (medido em radiator_springs.glb: ror_Object_9 é o prédio, ror_Object_16 a caixa de
# vidro por dentro das molduras; eixos do jogo)
# ------------------------------------------------------------------
PISO_Y = 1.905           # o chão de dentro: 3 cm acima da praça (1,876), que passa por baixo do prédio
Y_DA_PRACA = 1.876
# A fachada da frente é inclinada (o alto vai para fora): o plano dos vidros dela é
# z = 180,16 - 0,299 (y - 1,97). A nossa parede da frente fica RECUO_DA_FRENTE para dentro dele (os
# peitoris, os pilares e os trilhos das bandeiras da casca entram até 0,37 m além do vidro)
INCLINACAO = 0.299
Z_VIDRO_FRENTE_NO_PISO = 180.16 + (1.97 - PISO_Y) * INCLINACAO
RECUO_DA_FRENTE = 0.38
# As faces de dentro das nossas paredes (os caixilhos da casca entram até x 687,12 e 709,54 e z 203,65)
X_OESTE, X_LESTE, Z_FUNDO = 687.18, 709.40, 203.50
# Os planos dos vidros da casca (até onde vão os rebaixos)
X_VIDRO_OESTE, X_VIDRO_LESTE, Z_VIDRO_FUNDO = 686.64, 709.90, 204.10
# O teto: na frente, 5 cm abaixo do tampo da caixa de vidro (7,72 em z 178,44); a inclinação é um pouco maior
# que a do tampo (6,27 em z 204,17) porque, nos fundos, o forro do telhado da casca (um triângulo grande do
# Object_9, de y 5,93 a 7,08) desce mais que o tampo: assim a casca fica pelo menos 5 cm acima do teto novo
# em toda a sala (conferido ao importar o mapa)
TETO_A, TETO_Z0, TETO_K = 7.67, 178.44, 0.0600
FOLGA_MINIMA_DA_CASCA = 0.04
# A entrada do meio (já aberta na casca): x 694,40-702,03, verga em 5,96 (embaixo dela, z 178,1-178,7)
ENTRADA_X = (694.43, 702.00)
ENTRADA_TOPO = 5.93
Z_DA_VERGA = 178.70
Z_DA_SOLEIRA = 179.05      # o piso de dentro sai pela entrada até aqui (a casca começa em z ~179,5-180,0)
# A vaga do meio dos fundos (x 694,84-702,50) é um vão da moldura fechado só pelo vidro: fica atrás da estante
VAO_DOS_FUNDOS_X = (694.84, 702.50)


def y_teto(z):
    return TETO_A - TETO_K * (z - TETO_Z0)


def z_frente(y, recuo=RECUO_DA_FRENTE):
    """O z da nossa parede da frente (inclinada) na altura y"""
    return Z_VIDRO_FRENTE_NO_PISO + recuo - INCLINACAO * (y - PISO_Y)


def topo_da_frente():
    y = 7.6
    for _ in range(30):
        y = y_teto(z_frente(y))
    return y


Y_TOPO_FRENTE = topo_da_frente()            # onde o teto encontra a parede da frente
Z_TOPO_FRENTE = z_frente(Y_TOPO_FRENTE)
Z_FRENTE_NO_PISO = z_frente(PISO_Y)

# As peças da casca que se tiram do mapa (caixas que contêm cada uma inteira; eixos do jogo; conferidas com
# testar_aberturas.py no Race-O-Rama original x4,5). "objetos": de onde a peça sai (no mapa, com o prefixo
# ror_; é só informação: nenhuma destas caixas contém peça inteira de outro objeto). A primeira que sai da
# colisão é do Object_9 (o integrador dá à colisão de dentro o tipo de chão da primeira casca mexida). Os
# vidros dos lados vêm antes dos recortes (abaixo), que cortariam as peças deles.
# (nome, descrição, caixa mínima, caixa máxima, faces, objetos, sai da colisão)
ABERTURAS_DA_CASCA = [
    ('recepcao_piso_de_dentro', 'o piso velho de dentro (y 2,11; o novo fica em piso_y, na altura da praça)',
     (686.753, 2.113, 179.916), (709.548, 2.113, 204.028), 2, ['Object_9'], True),
    ('recepcao_balcao', 'o balcão do Race-O-Rama no meio da sala',
     (697.181, 2.051, 186.443), (699.365, 2.889, 193.783), 10, ['Object_9'], True),
    ('recepcao_armario_oeste', 'o armário encostado na parede oeste',
     (686.64, 2.051, 187.496), (688.798, 5.834, 195.669), 8, ['Object_9'], True),
    ('recepcao_armario_leste', 'o armário encostado na parede leste',
     (707.827, 2.051, 187.127), (709.985, 5.834, 195.299), 8, ['Object_9'], True),
    ('recepcao_hastes_da_vitrine_oeste', 'as duas hastes em pé atrás do vidro da vitrine de oeste (x 689,5 e 692,5, '
     'y 4,22-7,90; cada uma são 4 tiras soltas de 2 faces): a parede inclinada de dentro as cruzaria e elas '
     'furariam o teto novo',
     (689.42, 4.22, 179.55), (692.60, 7.90, 179.70), 16, ['Object_9'], True),
    ('recepcao_fundo_da_caixa', 'o fundo da caixa de vidro (y 1,90-1,97, virado para baixo; furaria o piso novo)',
     (686.603, 1.936, 180.203), (709.88, 1.936, 204.182), 6, ['Object_16'], True),
    ('recepcao_vidro_oeste', 'o vidro da parede oeste (as janelas e as bandeiras)',
     (686.603, 1.939, 178.481), (686.642, 7.677, 204.061), 2, ['Object_16'], False),
    ('recepcao_vidro_leste', 'o vidro da parede leste (as janelas e as bandeiras)',
     (709.92, 1.939, 178.481), (709.92, 7.677, 204.182), 2, ['Object_16'], False),
]

# Os vidros da frente (dois quadriláteros inclinados de y 1,97 a 7,72, um de cada lado da entrada) e o dos
# fundos (uma peça só, de 2,0 a 6,27) não saem por caixa: a caixa de cada um também contém, inteiras, as
# peças da moldura do Object_9 que ficam na frente dele (o painel verde, o rodapé, o peitoril, os trilhos;
# 14, 10 e 2 peças), e o tirar_pecas() do integrador as apagaria junto. Saem por RECORTE, só no vão que se
# vê de cada vitrine, bandeira e janela (entre os batentes, o peitoril e a verga da casca): o integrador
# corta as faces da casca pelos 4 lados do vão e apaga as que têm o centro dentro dele, numa faixa de
# ±(profundidade + 0,3) m em volta do plano dos cantos. Os cantos ficam num plano VERTICAL (z fixo) no meio
# do vidro: assim os lados de cima e de baixo do vão são planos horizontais (o peitoril e a verga ficam de
# fora) e a faixa (mais larga que a inclinação do vidro) só contém o vidro; ela é larga também porque o
# integrador só corta as faces que têm um vértice a menos de (meia diagonal + profundidade + 0,3) m do
# meio do vão, e os cantos do vidro estão longe. O que sobra do vidro fica atrás das molduras da casca (de
# fora) e das nossas paredes (de dentro). A ordem importa: as vitrines antes das bandeiras (o corte delas
# deixa vértices perto das bandeiras). Conferido com as funções do integrador: só o Object_16 perde área,
# e a área tirada é a do vão.
# (nome, descrição, (x0, x1), (y0, y1), z dos cantos, profundidade)
RECORTES_DA_CASCA = [
    ('recepcao_vidro_vitrine_oeste', 'o vidro da vitrine de oeste (a do letreiro OFFICE), entre o peitoril e a verga',
     (686.900, 693.875), (3.108, 5.896), 179.405, 0.85),
    ('recepcao_vidro_bandeira_oeste', 'o vidro da bandeira em cima da vitrine de oeste, entre os trilhos',
     (686.950, 693.850), (6.152, 7.466), 178.713, 0.25),
    ('recepcao_vidro_vitrine_leste', 'o vidro da vitrine de leste (a dos cactos), entre o peitoril e a verga',
     (702.575, 709.320), (3.108, 5.895), 179.405, 0.88),
    ('recepcao_vidro_bandeira_leste', 'o vidro da bandeira em cima da vitrine de leste, entre os trilhos',
     (702.570, 709.300), (6.152, 7.415), 178.721, 0.25),
    ('recepcao_vidro_fundos_janela_oeste', 'o vidro da janela de oeste dos fundos',
     (687.340, 694.310), (3.108, 5.868), 204.090, 0.35),
    ('recepcao_vidro_fundos_vao_do_meio', 'o vidro do vão do meio dos fundos (atrás da estante), acima das sapatas',
     (694.860, 702.480), (3.210, 5.905), 204.130, 0.25),
    ('recepcao_vidro_fundos_vao_do_meio_pe', 'o vidro do vão do meio dos fundos, entre as sapatas dos pilares',
     (695.470, 701.870), (1.950, 3.210), 204.130, 0.30),
    # atrás das sapatas (pirâmides): a faixa fica para dentro do vidro, sem pegar as faces delas nem as do rodapé
    ('recepcao_vidro_fundos_atras_da_sapata_oeste', 'o vidro do vão do meio dos fundos atrás da sapata do pilar de oeste',
     (694.860, 695.470), (2.100, 3.210), 203.900, 0.0),
    ('recepcao_vidro_fundos_atras_da_sapata_leste', 'o vidro do vão do meio dos fundos atrás da sapata do pilar de leste',
     (701.870, 702.480), (2.100, 3.210), 203.900, 0.0),
    ('recepcao_vidro_fundos_janela_leste', 'o vidro da janela de leste dos fundos',
     (703.010, 709.600), (3.108, 5.870), 204.170, 0.50),
]
FORA_DO_RECORTE = 0.3          # (o do integrador) a faixa do recorte vai isso além da profundidade


def cantos_do_recorte(r):
    _, _, (x0, x1), (y0, y1), zc, _ = r
    return [[x0, y0, zc], [x1, y0, zc], [x1, y1, zc], [x0, y1, zc]]

# ------------------------------------------------------------------
# O arranjo (eixos do jogo)
# ------------------------------------------------------------------
PASSO = 0.42                 # a grade das superfícies dos móveis
PASSO_DA_PAREDE = 0.30       # nas paredes
PASSO_DO_PISO = 0.30         # no piso (o reflexo falso do encerado é calculado em cada vértice)
PASSO_DO_TETO = 0.55         # no teto
FRISO = 0.34                 # a altura do friso de triângulos no pé das paredes
CANTO = 0.36                 # as aberturas param isso antes de um canto da sala (as pilastras)
PILASTRA = 0.22
MOLDURA = 0.11               # a largura dos caixilhos de madeira em volta das aberturas
# A Sally: atrás do balcão (na ponta redonda dele), de frente para a entrada, na frente da estante
LUGAR_DA_SALLY = (697.90, 199.60, 4.8, 2.4)          # x, z, comprimento e largura da vaga
COMPRIMENTO_DA_SALLY = 4.4
LARGURA_DO_CAMINHO = 4.0
CAMINHO_X = (695.80, 699.80)                         # o corredor livre da entrada até a Sally
# O balcão: a frente (os rolos) olha para a entrada; a ponta redonda de oeste fica ao lado do nariz da Sally
BALCAO_X = (700.55, 706.85)    # do centro da ponta redonda até a ponta de leste
BALCAO_Z = (196.00, 197.15)    # da frente dos rolos às costas
ESTANTE_X = (694.42, 702.90)   # a estante embutida dos fundos: entre as janelas, cobrindo o vão do meio da casca
ESTANTE_FUNDO = 0.75
ESTANTE_ALTURA = 3.85          # até y 5,76, uns 0,4 m abaixo do teto (a fileira de cones de cima quase encosta nele)

# ------------------------------------------------------------------
# Cores (na tela, 0 a 1)
# ------------------------------------------------------------------
PAREDE = (0.93, 0.76, 0.62)            # pêssego
PAREDE_FUNDO_ESTANTE = (0.90, 0.78, 0.76)   # o fundo lilás-rosado dos nichos da estante
CREME = (0.95, 0.90, 0.80)
ESTANTE = (0.94, 0.90, 0.82)
LARANJA = (0.93, 0.43, 0.11)
AMARELO_FRISO = (0.99, 0.82, 0.45)
MADEIRA = (0.60, 0.32, 0.15)            # os caixilhos e os rebaixos
MADEIRA_ESCURA = (0.33, 0.17, 0.08)
MADEIRA_TAMPO = (0.50, 0.26, 0.12)
TETO = (0.54, 0.43, 0.34)
VIGA = (0.34, 0.20, 0.10)
CONE = (0.96, 0.40, 0.08)
CONE_ESCURO = (0.74, 0.22, 0.08)
CONE_BRANCO = (0.96, 0.94, 0.88)
CONE_PRETO = (0.10, 0.09, 0.09)
ROLO = (0.15, 0.15, 0.18)
ROLO_REALCE = (0.50, 0.50, 0.56)
FERRO = (0.07, 0.065, 0.06)
TERRACOTA = (0.74, 0.40, 0.24)
TERRA = (0.25, 0.16, 0.10)
VERDE = (0.36, 0.53, 0.30)
VERDE_ESCURO = (0.18, 0.32, 0.16)
VERDE_PALMA = (0.50, 0.60, 0.40)
VERDE_HERA = (0.27, 0.43, 0.25)
COBRE = (0.72, 0.43, 0.20)
LATAO = (0.86, 0.68, 0.30)
CREME_VENTILADOR = (0.90, 0.84, 0.70)
CROMO = (0.70, 0.71, 0.73)
BRANCO = (1.0, 1.0, 1.0)
PAPEL_BRANCO = (0.96, 0.95, 0.92)
LUZ_DA_LAMPADA = (1.0, 0.86, 0.58)     # a boca acesa das luminárias (brilha sozinha)
CUPULA_ACESA = (0.90, 0.38, 0.08)      # a cúpula de cone, acesa por dentro
NEON = (1.0, 0.45, 0.15)               # o miolo dos tubos do letreiro OFFICE (cor fixa, saturada)
NEON_HALO = (0.55, 0.18, 0.05)         # o halo: uma cópia mais grossa logo atrás dos tubos


def linear(c):
    return tuple(((x + 0.055) / 1.055) ** 2.4 if x > 0.04045 else x / 12.92 for x in c[:3])


def tela(c):
    c = np.clip(np.asarray(c, float), 0.0, None)
    return np.where(c > 0.0031308, 1.055 * np.power(c, 1 / 2.4) - 0.055, 12.92 * c)


def unit(v):
    v = np.asarray(v, float)
    return v / np.linalg.norm(v)


def mistura(a, b, t):
    return tuple(float(x * (1 - t) + y * t) for x, y in zip(a, b))


def escurecer(c, k):
    return tuple(float(x * k) for x in c)


def rumo_para(de, para):
    """O rumo do jogo (0 olhando para -z, crescendo para a direita: a frente é (sin r, -cos r))"""
    return math.atan2(para[0] - de[0], -(para[1] - de[1]))


print('Parede da frente: z %.3f no piso, encontra o teto em y %.3f (z %.3f)' % (Z_FRENTE_NO_PISO, Y_TOPO_FRENTE, Z_TOPO_FRENTE))

# ------------------------------------------------------------------
# Imagens
# ------------------------------------------------------------------
PASTA_TEMP = tempfile.mkdtemp(prefix='recepcao_cozy_')


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
    """Pesos (n_para x n_de) para levar n_de amostras a n_para: a média das áreas quando reduz e a
    interpolação linear quando amplia"""
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
    tmp = np.tensordot(My, np.asarray(img, np.float64), axes=(1, 0))
    return np.transpose(np.tensordot(tmp, Mx, axes=(1, 1)), (0, 2, 1)).astype(np.float32)


# ------------------------------------------------------------------
# 1. Texturas
# ------------------------------------------------------------------
LADRILHO = 1.0                 # metros de cada ladrilho do piso
METROS_DO_PISO = 4.0           # a textura do piso cobre 4 x 4 ladrilhos
METROS_DO_TETO = 1.2           # a do teto, 2 x 2 placas de 0,6 m
PISO_CLARO = (0.93, 0.86, 0.74)       # creme
PISO_ESCURO = (0.80, 0.60, 0.47)      # rosado claro (o xadrez do filme tem pouco contraste)


def textura_do_piso():
    """Ladrilhos bege em xadrez (claro e areia), com rejunte fino, um mosqueado suave de linóleo e uma
    leve variação de tom entre um ladrilho e outro; a textura se repete nas bordas"""
    rng = np.random.default_rng(17)
    n = 1024
    k = 4
    lado = n // k
    yy, xx = np.mgrid[0:n, 0:n].astype(np.float32)
    ci = (xx // lado).astype(int)
    cj = (yy // lado).astype(int)
    xadrez = (ci + cj) % 2
    tons = rng.uniform(0.96, 1.04, (k, k))
    tom = tons[cj, ci]
    claro = np.array(PISO_CLARO, np.float32)
    escuro = np.array(PISO_ESCURO, np.float32)
    base = np.where(xadrez[..., None] == 0, claro, escuro)
    manchas = ruido(n, n, 18, 18, rng)
    grao = ruido(n, n, 1.2, 1.2, rng)
    veio = ruido(n, n, 6, 40, rng)
    cor = base * (tom * (1 + 0.035 * manchas + 0.018 * grao + 0.015 * veio))[..., None]
    # o rejunte: 3 px, um pouco mais escuro, com a borda do ladrilho levemente chanfrada
    dx = np.minimum(xx % lado, lado - 1 - (xx % lado))
    dy = np.minimum(yy % lado, lado - 1 - (yy % lado))
    d = np.minimum(dx, dy)
    rejunte = d < 1.5
    chanfro = np.clip((d - 1.5) / 4.0, 0, 1)
    cor = cor * (0.90 + 0.10 * chanfro)[..., None]
    cor[rejunte] = np.array((0.60, 0.53, 0.43))
    return np.clip(cor, 0, 1)


def textura_do_teto():
    """Placas de forro (0,6 m) bege, com furinhos e fissuras miúdas, e a junta entre elas"""
    rng = np.random.default_rng(23)
    n = 512
    lado = n // 2
    yy, xx = np.mgrid[0:n, 0:n].astype(np.float32)
    grao = ruido(n, n, 0.8, 0.8, rng)
    manchas = ruido(n, n, 30, 30, rng)
    fissura = ruido(n, n, 1.5, 6, rng)
    v = 1.0 + 0.05 * grao + 0.03 * manchas
    v = v - np.where(fissura > 2.0, 0.10, 0.0) - np.where(grao < -2.1, 0.12, 0.0)
    d = np.minimum(np.minimum(xx % lado, lado - 1 - (xx % lado)), np.minimum(yy % lado, lado - 1 - (yy % lado)))
    v = v * np.where(d < 2.0, 0.74, np.where(d < 4.0, 0.90, 1.0))
    cor = np.stack([v, v * 0.97, v * 0.93], axis=2)
    return np.clip(cor, 0, 1)


TEXTURAS = {'piso': textura_do_piso(), 'teto': textura_do_teto()}
IMG_PISO = imagem('recepcao_piso', TEXTURAS['piso'])
IMG_TETO = imagem('recepcao_teto', TEXTURAS['teto'])


# ------------------------------------------------------------------
# 2. Os quadros (o atlas)
# ------------------------------------------------------------------
FONTE_TITULO = os.path.join(PASTA_DAS_FONTES, 'racing_sans_one.ttf')
FONTE_TEXTO = os.path.join(PASTA_DAS_FONTES, 'saira_condensed_bold.ttf')
FONTE_FINA = os.path.join(PASTA_DAS_FONTES, 'saira_condensed_semibold.ttf')
TAM_DO_ATLAS = 2048
BORDA_DA_CELULA = 4
# As células: (nome, x, y, largura, altura) em pixels (a linha 0 em cima)
CELULAS = []
for _k, _n in enumerate(['mesas', 'rota66', 'cone_noite', 'sally', 'mcqueen']):
    CELULAS.append((_n, _k * 384, 0, 384, 288))
for _k, _n in enumerate(['cactos', 'cones', 'mater', 'doc', 'motel']):
    CELULAS.append((_n, _k * 384, 288, 384, 288))
for _k, _n in enumerate(['retrato_cone', 'calendario', 'cartaz66', 'retrato_mesa']):
    CELULAS.append((_n, _k * 288, 576, 288, 384))
CELULAS.append(('honk', 1152, 576, 384, 288))
CELULAS.append(('cartaz', 1536, 576, 512, 768))
CELULAS.append(('capacho', 0, 960, 768, 384))
CELULAS.append(('folhetos', 768, 960, 384, 288))
CELULAS.append(('chaves', 1152, 864, 384, 192))
CELULAS.append(('tapete', 0, 1344, 1024, 640))
CELULAS.append(('lua', 1024, 1344, 384, 288))
CELULAS.append(('placa', 1408, 1344, 384, 288))
NOME_DA_CELULA = {c[0]: k for k, c in enumerate(CELULAS)}


def uv_da_celula(nome):
    """(u0, v0, u1, v1) da parte útil da célula, nas coordenadas do Blender (v para cima)"""
    _, x, y, w, h = CELULAS[NOME_DA_CELULA[nome]]
    b = BORDA_DA_CELULA + 1
    return ((x + b) / TAM_DO_ATLAS, 1 - (y + h - b) / TAM_DO_ATLAS,
            (x + w - b) / TAM_DO_ATLAS, 1 - (y + b) / TAM_DO_ATLAS)


def aspecto(nome):
    _, _, _, w, h = CELULAS[NOME_DA_CELULA[nome]]
    return (w - 2 * BORDA_DA_CELULA - 2) / (h - 2 * BORDA_DA_CELULA - 2)


def limpar_cena(guardar_imagens=()):
    for o in list(bpy.data.objects):
        bpy.data.objects.remove(o, do_unlink=True)
    for colecao in (bpy.data.meshes, bpy.data.curves, bpy.data.cameras, bpy.data.lights):
        for d in list(colecao):
            colecao.remove(d)
    for m in list(bpy.data.materials):
        bpy.data.materials.remove(m)
    guardar = {IMG_PISO.name, IMG_TETO.name, 'recepcao_quadros'} | set(guardar_imagens)
    for im in list(bpy.data.images):
        if im.name not in guardar:
            bpy.data.images.remove(im)


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


def importar_carro(nome_do_arquivo):
    """Importa um carro do jogo, com uns 4,5 m de comprimento, de pé no chão (z = 0) e com a frente para -y"""
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
    esc = 4.5 / max(mx.x - mn.x, mx.y - mn.y, (mx.z - mn.z) * 1.3)
    raiz = bpy.data.objects.new('carro_' + nome_do_arquivo, None)
    bpy.context.scene.collection.objects.link(raiz)
    meio = Vector(((mn.x + mx.x) / 2, (mn.y + mx.y) / 2, mn.z))
    for o in novos:
        if o.parent is None:
            o.parent = raiz
            o.location = (o.location - meio) * esc
            o.scale = o.scale * esc
    bpy.context.view_layer.update()
    return raiz, [o for o in novos if o.type == 'MESH'], esc


def cor_da_palpebra(o, img):
    """Se img é a textura dos olhos, devolve a cor da pálpebra (linear); senão, None (o giro de matiz que
    repinta o carro só pega o que tem essa cor: a íris fica com a cor de verdade)"""
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


def pintar_de_cartaz(objetos, contorno):
    """Troca os materiais do carro por um sombreado em degraus (de cartaz) e põe um contorno preto"""
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
            m = novo_material(m0.name + '_cartaz')
            nt = m.node_tree
            nt.nodes.clear()
            saida = nt.nodes.new('ShaderNodeOutputMaterial')
            dif = nt.nodes.new('ShaderNodeBsdfDiffuse')
            s2r = nt.nodes.new('ShaderNodeShaderToRGB')
            rampa = nt.nodes.new('ShaderNodeValToRGB')
            rampa.color_ramp.interpolation = 'CONSTANT'
            els = rampa.color_ramp.elements
            els[0].position = 0.0
            els[0].color = (0.32, 0.29, 0.35, 1)
            els[1].position = 0.12
            els[1].color = (0.64, 0.62, 0.64, 1)
            e = els.new(0.42)
            e.color = (1.0, 0.98, 0.94, 1)
            e = els.new(0.85)
            e.color = (1.22, 1.20, 1.14, 1)
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
        mod.thickness = 0.035 / max(o.matrix_world.to_scale())
        mod.offset = 1.0
        mod.use_flip_normals = True
        mod.material_offset = len(o.data.materials) - 1
        mod.use_rim = False


def malha_plana(nome, pontos, cores, material, colecao):
    me = bpy.data.meshes.new(nome)
    me.from_pydata([tuple(p) for p in pontos], [], [list(range(len(pontos)))])
    at = me.color_attributes.new('Col', 'FLOAT_COLOR', 'CORNER')
    for k, l in enumerate(me.loops):
        at.data[k].color = (*linear(cores[l.vertex_index]), 1)
    me.materials.append(material)
    o = bpy.data.objects.new(nome, me)
    colecao.objects.link(o)
    return o


# Cores dos quadros
P_PAPEL = (0.95, 0.90, 0.78)
P_CEU = (0.48, 0.70, 0.86)
P_CEU_CLARO = (0.86, 0.91, 0.90)
P_LARANJA = (0.95, 0.46, 0.14)
P_PESSEGO = (0.98, 0.72, 0.48)
P_ROSA = (0.88, 0.48, 0.48)
P_ROXO = (0.38, 0.22, 0.40)
P_NOITE = (0.07, 0.08, 0.22)
P_ROCHA = (0.66, 0.30, 0.17)
P_ROCHA_CLARA = (0.86, 0.50, 0.28)
P_ROCHA_ESCURA = (0.42, 0.18, 0.12)
P_AREIA = (0.90, 0.72, 0.46)
P_ESTRADA = (0.30, 0.29, 0.29)
P_AMARELO = (0.98, 0.82, 0.30)
P_VERDE = (0.24, 0.38, 0.22)
P_PRETO = (0.08, 0.07, 0.07)
P_BRANCO = (0.97, 0.96, 0.92)
P_MARROM = (0.36, 0.20, 0.10)


def renderizar_quadros():
    """Pinta os quadros, o cartaz, a plaquinha, o calendário e o capacho; devolve o atlas (linha 0 em cima)"""
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
    sol1.data.color = (1.0, 0.95, 0.85)
    sol1.rotation_euler = (math.radians(55), math.radians(-25), math.radians(-35))
    sol2 = bpy.data.objects.new('sol2', bpy.data.lights.new('sol2', 'SUN'))
    sol2.data.energy = 1.4
    sol2.data.color = (0.85, 0.9, 1.0)
    sol2.rotation_euler = (math.radians(-60), math.radians(20), math.radians(150))
    for s in (sol1, sol2):
        sc.collection.objects.link(s)
    contorno = novo_material('contorno')
    contorno.use_backface_culling = True
    nt = contorno.node_tree
    nt.nodes.clear()
    em = nt.nodes.new('ShaderNodeEmission')
    em.inputs['Color'].default_value = (0.03, 0.025, 0.025, 1)
    nt.links.new(em.outputs[0], nt.nodes.new('ShaderNodeOutputMaterial').inputs['Surface'])
    mat_fundo = material_emissao('fundo_quadro', 'vertices')
    mat_texto = material_emissao('texto_quadro', 'objeto')
    fontes = {}
    carros = {}
    for chave, arquivo in (('sally', 'sally'), ('mcqueen', 'mcqueen'), ('mater', 'mater')):
        raiz, objs, _ = importar_carro(arquivo)
        pintar_de_cartaz(objs, contorno)
        carros[chave] = (raiz, objs)
        raiz.hide_render = True
        for o in objs:
            o.hide_render = True
    camada = bpy.data.collections.new('camada_do_quadro')
    sc.collection.children.link(camada)
    atlas = np.zeros((TAM_DO_ATLAS, TAM_DO_ATLAS, 3), np.float32)
    estado = {}

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
        for k, (raiz, objs) in carros.items():
            raiz.hide_render = True
            for o in objs:
                o.hide_render = True

    def ponto(s, t, d):
        mw, mh = estado['mw'], estado['mh']
        return ((s - 0.5) * 2 * mw * d, d, (t - 0.5) * 2 * mh * d)

    def poli(pts_st, cores, d=80):
        if not isinstance(cores, list):
            cores = [cores] * len(pts_st)
        malha_plana('p', [ponto(s, t, d) for s, t in pts_st], cores, mat_fundo, camada)

    def ret(s0, t0, s1, t1, c0, c1=None, d=80):
        c1 = c0 if c1 is None else c1
        poli([(s0, t0), (s1, t0), (s1, t1), (s0, t1)], [c0, c0, c1, c1], d)

    def circ(cs, ct, r, cor, d=80, n=40, ry=None):
        asp = estado['mw'] / estado['mh']
        ry = r * asp if ry is None else ry
        poli([(cs + r * math.cos(a), ct + ry * math.sin(a)) for a in np.linspace(0, 2 * math.pi, n, endpoint=False)], cor, d)

    def texto(txt, fonte, cor, s, t, larg, alt, d=60, sombra=None, rot=0.0):
        if fonte not in fontes:
            fontes[fonte] = bpy.data.fonts.load(fonte, check_existing=True)
        cu = bpy.data.curves.new('txt', 'FONT')
        cu.body = txt
        cu.font = fontes[fonte]
        cu.align_x = 'CENTER'
        cu.align_y = 'CENTER'
        cu.size = 1.0
        o = bpy.data.objects.new('txt', cu)
        camada.objects.link(o)
        o.data.materials.append(mat_texto)
        o.color = (*linear(cor), 1)
        bpy.context.view_layer.update()
        dw, dh = max(o.dimensions.x, 1e-3), max(o.dimensions.y, 1e-3)
        mw, mh = estado['mw'], estado['mh']
        k = min(larg * 2 * mw * d / dw, alt * 2 * mh * d / dh)
        o.scale = (k, k, k)
        o.rotation_euler = (math.pi / 2, math.radians(rot), 0)
        o.location = ponto(s, t, d)
        if sombra is not None:
            o2 = o.copy()
            camada.objects.link(o2)
            o2.color = (*linear(sombra), 1)
            o2.location = ponto(s + 0.006, t - 0.008, d + 0.05)

    def carro(chave, giro, alto, pos, rola=0.0):
        raiz, objs = carros[chave]
        raiz.hide_render = False
        for o in objs:
            o.hide_render = False
        s, t, d = pos
        mw, mh = estado['mw'], estado['mh']
        raiz.location = ((s - 0.5) * 2 * mw * d, d, (t - 0.5) * 2 * mh * d - 0.6)
        raiz.rotation_euler = (math.radians(alto), math.radians(rola), math.radians(giro))

    def render(w_px, h_px):
        sc.render.resolution_x = w_px
        sc.render.resolution_y = h_px
        sc.render.resolution_percentage = 100
        caminho = os.path.join(PASTA_TEMP, 'quadro_render.png')
        sc.render.filepath = caminho
        sc.render.image_settings.file_format = 'PNG'
        bpy.ops.render.render(write_still=True)
        return ler_png(caminho)[..., :3]

    def pintura(img, rng, envelhecer=0.08):
        """Um pouco de papel e de tinta: grão, fibra, cantos mais escuros e poucas tintas"""
        h, w = img.shape[:2]
        grao = ruido(h, w, 1.6, 1.6, rng)
        fibra = ruido(h, w, 1.0, 10, rng)
        img = img * (1 + 0.025 * grao[..., None] + 0.015 * fibra[..., None])
        img = img * (1 - envelhecer) + np.array(P_PAPEL) * envelhecer
        yy, xx = np.mgrid[0:h, 0:w]
        d = np.maximum(np.abs(xx / w - 0.5), np.abs(yy / h - 0.5)) * 2
        img = img * (1 - 0.14 * np.clip((d - 0.7) / 0.3, 0, 1) ** 2)[..., None]
        img = np.round(img * 16) / 16 * 0.3 + img * 0.7
        return np.clip(img, 0, 1)

    def por_na_celula(nome, img):
        _, x, y, w, h = CELULAS[NOME_DA_CELULA[nome]]
        b = BORDA_DA_CELULA
        img = redimensionar(img, w - 2 * b, h - 2 * b)
        atlas[y:y + h, x:x + w] = np.pad(img, ((b, b), (b, b), (0, 0)), mode='edge')

    def preparar(nome, fator=2.2):
        limpar_camada()
        _, _, _, w, h = CELULAS[NOME_DA_CELULA[nome]]
        w_px = int(round((w - 2 * BORDA_DA_CELULA) * fator))
        h_px = int(round((h - 2 * BORDA_DA_CELULA) * fator))
        estado['mh'] = 12 / cam.lens
        estado['mw'] = estado['mh'] * w_px / h_px
        estado['px'] = (w_px, h_px)
        fundo_mundo.inputs[0].default_value = (0.5, 0.5, 0.5, 1)

    def mesas_ao_longe(t0, cores, rng, n=3, alturas=(0.16, 0.26)):
        """Mesas de rocha (topo plano, paredes íngremes, um talude embaixo)"""
        xs = np.sort(rng.uniform(0.0, 1.0, n))
        for k, x0 in enumerate(xs):
            larg = rng.uniform(0.14, 0.30)
            alt = rng.uniform(*alturas)
            c = cores[k % len(cores)]
            poli([(x0 - larg / 2 - 0.08, t0), (x0 + larg / 2 + 0.08, t0), (x0 + larg / 2 + 0.01, t0 + alt * 0.35),
                  (x0 + larg / 2 - 0.01, t0 + alt), (x0 - larg / 2 + 0.02, t0 + alt), (x0 - larg / 2, t0 + alt * 0.35)],
                 [c, c, c, mistura(c, P_ROCHA_CLARA, 0.35), mistura(c, P_ROCHA_CLARA, 0.35), c], 78 - k * 0.1)

    def cone_desenhado(cs, ct, alt, d, cor=P_LARANJA, porta=False, janelas=False):
        asp = estado['mw'] / estado['mh']
        larg = alt * 0.62 / asp
        poli([(cs - larg / 2, ct), (cs + larg / 2, ct), (cs + larg * 0.07, ct + alt), (cs - larg * 0.07, ct + alt)],
             [escurecer(cor, 0.85), cor, cor, escurecer(cor, 0.9)], d)
        poli([(cs - larg * 0.62, ct - 0.015), (cs + larg * 0.62, ct - 0.015), (cs + larg * 0.62, ct + 0.02),
              (cs - larg * 0.62, ct + 0.02)], escurecer(cor, 0.75), d - 0.1)
        if porta:
            poli([(cs - larg * 0.16, ct + 0.02), (cs + larg * 0.16, ct + 0.02), (cs + larg * 0.16, ct + alt * 0.32),
                  (cs - larg * 0.16, ct + alt * 0.32)], (0.30, 0.16, 0.10), d - 0.2)
        if janelas:
            for sx in (-0.27, 0.27):
                circ(cs + larg * sx, ct + alt * 0.40, larg * 0.07, (1.0, 0.86, 0.45), d - 0.2, n=20)
        else:
            poli([(cs - larg * 0.27, ct + alt * 0.52), (cs + larg * 0.27, ct + alt * 0.52), (cs + larg * 0.20, ct + alt * 0.66),
                  (cs - larg * 0.20, ct + alt * 0.66)], P_BRANCO, d - 0.2)

    rng = np.random.default_rng(31)

    # --- mesas ao pôr do sol
    preparar('mesas')
    ret(-0.05, 0.30, 1.05, 1.05, (1.0, 0.82, 0.50), (0.86, 0.42, 0.40))
    circ(0.70, 0.46, 0.09, (1.0, 0.93, 0.70), d=79)
    mesas_ao_longe(0.30, [P_ROCHA, P_ROCHA_ESCURA, (0.58, 0.26, 0.20)], rng, n=4, alturas=(0.14, 0.30))
    ret(-0.05, -0.05, 1.05, 0.31, (0.78, 0.52, 0.30), P_AREIA, d=77)
    for x in (0.12, 0.86):
        ret(x, 0.10, x + 0.018, 0.30, P_VERDE, d=76)
        ret(x - 0.03, 0.18, x, 0.20, P_VERDE, d=76)
        ret(x - 0.03, 0.18, x - 0.015, 0.25, P_VERDE, d=76)
    img = render(*estado['px'])
    por_na_celula('mesas', pintura(img, np.random.default_rng(1)))

    # --- a Rota 66
    preparar('rota66')
    ret(-0.05, 0.35, 1.05, 1.05, P_CEU_CLARO, P_CEU)
    mesas_ao_longe(0.35, [(0.72, 0.46, 0.38), (0.66, 0.40, 0.34)], rng, n=3, alturas=(0.08, 0.16))
    ret(-0.05, -0.05, 1.05, 0.36, (0.80, 0.60, 0.38), P_AREIA, d=77)
    poli([(0.08, -0.05), (0.92, -0.05), (0.53, 0.355), (0.47, 0.355)], [P_ESTRADA, P_ESTRADA, (0.5, 0.48, 0.46), (0.5, 0.48, 0.46)], d=76)
    for k in range(7):
        t0 = 0.33 * (1 - (k / 7) ** 0.5)
        t1 = t0 - 0.018 * (1 + k)
        ret(0.497, max(t1, -0.05), 0.503 + 0.0015 * k, t0, P_AMARELO, d=75)
    ret(0.80, 0.10, 0.81, 0.44, (0.45, 0.42, 0.40), d=74)
    poli([(0.74, 0.42), (0.87, 0.42), (0.87, 0.58), (0.805, 0.62), (0.74, 0.58)], P_BRANCO, d=73)
    poli([(0.75, 0.43), (0.86, 0.43), (0.86, 0.575), (0.805, 0.61), (0.75, 0.575)], P_PRETO, d=72.9)
    poli([(0.758, 0.44), (0.852, 0.44), (0.852, 0.565), (0.805, 0.598), (0.758, 0.565)], P_BRANCO, d=72.8)
    texto('66', FONTE_TITULO, P_PRETO, 0.805, 0.49, 0.08, 0.08, d=60)
    texto('ROTA', FONTE_TEXTO, P_PRETO, 0.805, 0.56, 0.06, 0.03, d=60)
    img = render(*estado['px'])
    por_na_celula('rota66', pintura(img, np.random.default_rng(2)))

    # --- um cone-quarto de noite
    preparar('cone_noite')
    ret(-0.05, 0.2, 1.05, 1.05, (0.30, 0.20, 0.38), P_NOITE)
    for k in range(40):
        x, y = rng.uniform(0, 1), rng.uniform(0.45, 1)
        r = rng.uniform(0.002, 0.005)
        poli([(x - r, y), (x, y - r * 1.4), (x + r, y), (x, y + r * 1.4)], (1.0, 0.97, 0.85), d=79)
    circ(0.82, 0.80, 0.06, (0.98, 0.94, 0.78), d=79)
    ret(-0.05, -0.05, 1.05, 0.22, (0.20, 0.13, 0.13), (0.32, 0.20, 0.18), d=77)
    cone_desenhado(0.30, 0.20, 0.42, 70, cor=(0.80, 0.36, 0.12), janelas=True)
    cone_desenhado(0.58, 0.18, 0.62, 68, porta=True, janelas=True)
    circ(0.58, 0.18 + 0.62 * 0.32 / 2, 0.04, (1.0, 0.80, 0.40), d=67.5, n=16)
    img = render(*estado['px'])
    por_na_celula('cone_noite', pintura(img, np.random.default_rng(3)))

    # --- a Sally (retrato)
    preparar('sally')
    ret(-0.05, 0.25, 1.05, 1.05, (0.86, 0.90, 0.86), (0.52, 0.74, 0.80))
    mesas_ao_longe(0.25, [(0.86, 0.62, 0.52)], rng, n=3, alturas=(0.10, 0.18))
    ret(-0.05, -0.05, 1.05, 0.26, (0.86, 0.76, 0.62), d=77)
    fundo_mundo.inputs[0].default_value = (*linear((0.52, 0.74, 0.80)), 1)
    carro('sally', 32, 8, (0.5, 0.40, 8.5))
    img = render(*estado['px'])
    por_na_celula('sally', pintura(img, np.random.default_rng(4)))

    # --- o McQueen na estrada do deserto
    preparar('mcqueen')
    ret(-0.05, 0.30, 1.05, 1.05, (1.0, 0.84, 0.52), (0.94, 0.52, 0.36))
    circ(0.22, 0.62, 0.10, (1.0, 0.94, 0.72), d=79)
    mesas_ao_longe(0.30, [P_ROCHA, P_ROCHA_ESCURA], rng, n=3, alturas=(0.12, 0.24))
    ret(-0.05, -0.05, 1.05, 0.31, (0.70, 0.46, 0.28), P_AREIA, d=77)
    poli([(-0.05, 0.05), (1.05, 0.05), (1.05, 0.20), (-0.05, 0.20)], P_ESTRADA, d=76)
    for k in range(5):
        ret(0.05 + k * 0.22, 0.12, 0.15 + k * 0.22, 0.13, P_AMARELO, d=75.5)
    for k in range(4):
        ret(-0.05, 0.42 + k * 0.035, 0.30 - k * 0.05, 0.43 + k * 0.035, (1.0, 0.95, 0.80), d=74)
    carro('mcqueen', -68, 6, (0.56, 0.36, 9.0))
    img = render(*estado['px'])
    por_na_celula('mcqueen', pintura(img, np.random.default_rng(5)))

    # --- cactos no pôr do sol
    preparar('cactos')
    ret(-0.05, 0.25, 1.05, 1.05, (0.99, 0.70, 0.40), (0.52, 0.30, 0.50))
    circ(0.50, 0.33, 0.14, (1.0, 0.86, 0.52), d=79)
    ret(-0.05, -0.05, 1.05, 0.27, (0.30, 0.16, 0.18), (0.48, 0.24, 0.24), d=77)
    for (x, alt, larg) in ((0.20, 0.58, 0.05), (0.74, 0.46, 0.04), (0.88, 0.30, 0.03)):
        c = (0.16, 0.12, 0.16)
        ret(x - larg / 2, 0.22, x + larg / 2, 0.22 + alt, c, d=70)
        circ(x, 0.22 + alt, larg / 2, c, d=70, n=16)
        for lado, h0, h1 in ((-1, 0.40, 0.70), (1, 0.30, 0.62)):
            ax = x + lado * larg * 1.4
            ret(min(x, ax), 0.22 + alt * h0, max(x, ax), 0.22 + alt * h0 + larg * 0.8, c, d=70)
            ret(ax - larg * 0.4, 0.22 + alt * h0, ax + larg * 0.4, 0.22 + alt * h1, c, d=70)
            circ(ax, 0.22 + alt * h1, larg * 0.4, c, d=70, n=12)
    img = render(*estado['px'])
    por_na_celula('cactos', pintura(img, np.random.default_rng(6)))

    # --- cones (natureza-morta)
    preparar('cones')
    ret(-0.05, -0.05, 1.05, 1.05, (0.80, 0.86, 0.84), (0.95, 0.92, 0.84))
    ret(-0.05, -0.05, 1.05, 0.22, (0.62, 0.42, 0.28), (0.70, 0.50, 0.34), d=79)
    cone_desenhado(0.30, 0.22, 0.55, 70)
    cone_desenhado(0.55, 0.22, 0.38, 69, cor=(0.90, 0.32, 0.10))
    cone_desenhado(0.74, 0.22, 0.46, 68)
    img = render(*estado['px'])
    por_na_celula('cones', pintura(img, np.random.default_rng(7)))

    # --- o Mater
    preparar('mater')
    ret(-0.05, 0.25, 1.05, 1.05, (0.92, 0.90, 0.80), (0.56, 0.72, 0.84))
    mesas_ao_longe(0.25, [(0.74, 0.50, 0.40)], rng, n=2, alturas=(0.10, 0.16))
    ret(-0.05, -0.05, 1.05, 0.26, (0.72, 0.58, 0.40), d=77)
    fundo_mundo.inputs[0].default_value = (*linear((0.56, 0.72, 0.84)), 1)
    carro('mater', 28, 6, (0.5, 0.36, 8.8))
    img = render(*estado['px'])
    por_na_celula('mater', pintura(img, np.random.default_rng(8)))

    # --- o McQueen numa foto antiga em preto e branco (de frente)
    preparar('doc')
    ret(-0.05, -0.05, 1.05, 1.05, (0.35, 0.35, 0.35), (0.78, 0.78, 0.78))
    ret(-0.05, -0.05, 1.05, 0.30, (0.25, 0.25, 0.25), (0.45, 0.45, 0.45), d=79)
    fundo_mundo.inputs[0].default_value = (0.5, 0.5, 0.5, 1)
    carro('mcqueen', -28, 8, (0.5, 0.36, 9.5))
    img = render(*estado['px'])
    cinza = np.clip((img @ np.array([0.3, 0.59, 0.11])) * 1.3 + 0.05, 0, 1)
    img = cinza[..., None] * np.array([1.0, 0.94, 0.84])
    img = pintura(img, np.random.default_rng(9), 0.05)
    img[:10, :] = img[-10:, :] = 0.93
    img[:, :10] = img[:, -10:] = 0.93
    por_na_celula('doc', img)

    # --- o motel de dia
    preparar('motel')
    ret(-0.05, 0.25, 1.05, 1.05, P_CEU_CLARO, P_CEU)
    mesas_ao_longe(0.25, [(0.76, 0.48, 0.36)], rng, n=3, alturas=(0.12, 0.22))
    ret(-0.05, -0.05, 1.05, 0.26, (0.80, 0.66, 0.50), (0.86, 0.74, 0.58), d=77)
    for k, (x, alt) in enumerate(((0.12, 0.30), (0.32, 0.34), (0.68, 0.34), (0.88, 0.30))):
        cone_desenhado(x, 0.22, alt, 70 - k * 0.3, porta=True, janelas=True)
    ret(0.495, 0.22, 0.505, 0.62, (0.40, 0.38, 0.36), d=69)
    poli([(0.43, 0.56), (0.57, 0.56), (0.57, 0.70), (0.43, 0.70)], P_LARANJA, d=68)
    texto('COZY CONE', FONTE_TITULO, P_BRANCO, 0.50, 0.63, 0.12, 0.08, d=60)
    img = render(*estado['px'])
    por_na_celula('motel', pintura(img, np.random.default_rng(10)))

    # --- retrato de um cone (moldura oval)
    preparar('retrato_cone')
    ret(-0.05, -0.05, 1.05, 1.05, (0.30, 0.52, 0.56), (0.62, 0.78, 0.76))
    ret(-0.05, -0.05, 1.05, 0.20, (0.42, 0.30, 0.22), d=79)
    cone_desenhado(0.50, 0.20, 0.62, 70)
    img = render(*estado['px'])
    por_na_celula('retrato_cone', pintura(img, np.random.default_rng(11)))

    # --- o calendário
    preparar('calendario')
    ret(-0.05, -0.05, 1.05, 1.05, P_BRANCO)
    ret(0.06, 0.52, 0.94, 0.94, (0.94, 0.60, 0.40), (0.86, 0.46, 0.42), d=79)
    mesas_ao_longe(0.52, [P_ROCHA, P_ROCHA_ESCURA], rng, n=3, alturas=(0.10, 0.20))
    ret(0.06, 0.52, 0.94, 0.58, P_AREIA, d=77)
    texto('RADIATOR SPRINGS', FONTE_TEXTO, (0.62, 0.12, 0.08), 0.5, 0.47, 0.84, 0.05, d=60)
    texto('JUNHO', FONTE_TITULO, P_PRETO, 0.5, 0.41, 0.4, 0.05, d=60)
    for k in range(30):
        c, r = k % 7, k // 7
        texto(str(k + 1), FONTE_FINA, (0.70, 0.14, 0.10) if c == 0 else P_PRETO, 0.12 + c * 0.127, 0.34 - r * 0.07, 0.07, 0.04, d=60)
    img = render(*estado['px'])
    por_na_celula('calendario', pintura(img, np.random.default_rng(12), 0.04))

    # --- cartaz da Rota 66 (retrato)
    preparar('cartaz66')
    ret(-0.05, 0.3, 1.05, 1.05, (0.99, 0.84, 0.52), (0.94, 0.56, 0.32))
    mesas_ao_longe(0.30, [P_ROCHA, P_ROCHA_ESCURA], rng, n=3, alturas=(0.10, 0.20))
    ret(-0.05, -0.05, 1.05, 0.31, (0.78, 0.54, 0.32), P_AREIA, d=77)
    poli([(0.05, -0.05), (0.95, -0.05), (0.53, 0.305), (0.47, 0.305)], P_ESTRADA, d=76)
    ret(0.495, -0.05, 0.505, 0.30, P_AMARELO, d=75)
    poli([(0.30, 0.56), (0.70, 0.56), (0.70, 0.78), (0.50, 0.83), (0.30, 0.78)], P_PRETO, d=73)
    poli([(0.32, 0.575), (0.68, 0.575), (0.68, 0.77), (0.50, 0.815), (0.32, 0.77)], P_BRANCO, d=72.9)
    texto('66', FONTE_TITULO, P_PRETO, 0.50, 0.65, 0.26, 0.11, d=60)
    texto('ROTA', FONTE_TEXTO, P_PRETO, 0.50, 0.755, 0.16, 0.035, d=60)
    texto('RADIATOR SPRINGS', FONTE_TEXTO, P_BRANCO, 0.50, 0.93, 0.86, 0.06, d=60, sombra=P_MARROM)
    img = render(*estado['px'])
    por_na_celula('cartaz66', pintura(img, np.random.default_rng(13)))

    # --- retrato de uma mesa (moldura dourada redonda)
    preparar('retrato_mesa')
    ret(-0.05, 0.30, 1.05, 1.05, (0.96, 0.86, 0.66), (0.62, 0.74, 0.86))
    mesas_ao_longe(0.30, [P_ROCHA], rng, n=1, alturas=(0.30, 0.36))
    ret(-0.05, -0.05, 1.05, 0.31, P_AREIA, d=77)
    img = render(*estado['px'])
    por_na_celula('retrato_mesa', pintura(img, np.random.default_rng(14)))

    # --- a plaquinha "Honk for Service" (o cartão creme com a letra cursiva e os enfeites dos cantos)
    preparar('honk')
    ret(-0.05, -0.05, 1.05, 1.05, (0.96, 0.92, 0.80), (0.98, 0.95, 0.86))
    for (s, t, sx, sy) in ((0.08, 0.10, 1, 1), (0.92, 0.10, -1, 1), (0.08, 0.90, 1, -1), (0.92, 0.90, -1, -1)):
        ret(s, t, s + sx * 0.07, t + sy * 0.008, (0.55, 0.42, 0.30), d=79)
        ret(s, t, s + sx * 0.008, t + sy * 0.09, (0.55, 0.42, 0.30), d=79)
    texto('Honk', FONTE_TITULO, (0.30, 0.22, 0.18), 0.40, 0.66, 0.52, 0.30, d=60, rot=-6)
    texto('for', FONTE_FINA, (0.30, 0.22, 0.18), 0.72, 0.505, 0.16, 0.13, d=60)
    texto('Service', FONTE_TITULO, (0.30, 0.22, 0.18), 0.52, 0.32, 0.72, 0.28, d=60, rot=-4)
    img = render(*estado['px'])
    por_na_celula('honk', pintura(img, np.random.default_rng(15), 0.10))

    # --- o cartaz do Cozy Cone Motel
    preparar('cartaz')
    ret(-0.05, 0.25, 1.05, 1.05, (0.34, 0.22, 0.42), (0.06, 0.08, 0.22))
    for k in range(50):
        x, y = rng.uniform(0, 1), rng.uniform(0.45, 1)
        r = rng.uniform(0.002, 0.006)
        poli([(x - r, y), (x, y - r * 1.1), (x + r, y), (x, y + r * 1.1)], (1.0, 0.95, 0.80), d=79)
    ret(-0.05, -0.05, 1.05, 0.26, (0.28, 0.16, 0.14), (0.42, 0.24, 0.20), d=77)
    cone_desenhado(0.22, 0.24, 0.24, 71, cor=(0.80, 0.36, 0.12), janelas=True)
    cone_desenhado(0.80, 0.24, 0.26, 71, cor=(0.80, 0.36, 0.12), janelas=True)
    cone_desenhado(0.50, 0.20, 0.42, 70, porta=True, janelas=True)
    texto('COZY', FONTE_TITULO, (1.0, 0.62, 0.20), 0.50, 0.86, 0.80, 0.10, d=60, sombra=(0.40, 0.10, 0.06))
    texto('CONE', FONTE_TITULO, (1.0, 0.62, 0.20), 0.50, 0.75, 0.80, 0.10, d=60, sombra=(0.40, 0.10, 0.06))
    texto('MOTEL', FONTE_TEXTO, P_BRANCO, 0.50, 0.665, 0.40, 0.05, d=60)
    texto('Durma num cone!', FONTE_FINA, (1.0, 0.88, 0.60), 0.50, 0.10, 0.70, 0.04, d=60)
    texto('RADIATOR SPRINGS  ·  ROTA 66', FONTE_TEXTO, (0.96, 0.80, 0.56), 0.50, 0.05, 0.86, 0.025, d=60)
    img = render(*estado['px'])
    por_na_celula('cartaz', pintura(img, np.random.default_rng(16), 0.05))

    # --- o capacho (fibra de coco com a borda escura e o nome)
    preparar('capacho')
    ret(-0.05, -0.05, 1.05, 1.05, (0.72, 0.56, 0.30))
    ret(0.035, 0.07, 0.965, 0.93, (0.36, 0.22, 0.10), d=79)
    ret(0.06, 0.12, 0.94, 0.88, (0.76, 0.60, 0.33), d=78)
    texto('COZY CONE', FONTE_TITULO, (0.30, 0.17, 0.08), 0.50, 0.53, 0.78, 0.46, d=60)
    img = render(*estado['px'])
    h, w = img.shape[:2]
    fibra = ruido(h, w, 0.7, 0.7, np.random.default_rng(17))
    tufo = ruido(h, w, 3.0, 3.0, np.random.default_rng(18))
    img = img * (1 + 0.16 * fibra[..., None] + 0.07 * tufo[..., None])
    por_na_celula('capacho', np.clip(img, 0, 1))

    # --- os folhetos da estante de folhetos (uma fileira de capas)
    preparar('folhetos')
    ret(-0.05, -0.05, 1.05, 1.05, (0.30, 0.18, 0.10))
    capas = [(P_LARANJA, 'COZY CONE'), ((0.20, 0.50, 0.70), 'ROTA 66'), ((0.80, 0.22, 0.16), "FLO'S V8"),
             ((0.94, 0.80, 0.30), 'PNEUS'), ((0.26, 0.56, 0.36), 'ORNAMENT VALLEY'), ((0.62, 0.30, 0.62), 'RAMONE')]
    for k, (cor, txt) in enumerate(capas):
        s0 = 0.02 + (k % 3) * 0.33
        t0 = 0.52 if k < 3 else 0.03
        ret(s0, t0, s0 + 0.30, t0 + 0.45, cor, mistura(cor, P_BRANCO, 0.25), d=79)
        texto(txt, FONTE_TEXTO, P_BRANCO, s0 + 0.15, t0 + 0.36, 0.26, 0.05, d=60)
        ret(s0 + 0.04, t0 + 0.05, s0 + 0.26, t0 + 0.28, mistura(cor, P_PRETO, 0.3), d=78)
    img = render(*estado['px'])
    por_na_celula('folhetos', pintura(img, np.random.default_rng(19), 0.05))

    # --- o título do quadro de chaves
    preparar('chaves')
    ret(-0.05, -0.05, 1.05, 1.05, (0.96, 0.90, 0.76))
    texto('Cozy Cone', FONTE_TITULO, (0.80, 0.30, 0.08), 0.50, 0.62, 0.86, 0.48, d=60)
    texto('CHAVES DOS QUARTOS', FONTE_TEXTO, (0.30, 0.18, 0.10), 0.50, 0.22, 0.80, 0.18, d=60)
    img = render(*estado['px'])
    por_na_celula('chaves', pintura(img, np.random.default_rng(20), 0.05))

    # --- uma noite com lua sobre as mesas
    preparar('lua')
    ret(-0.05, 0.25, 1.05, 1.05, (0.36, 0.30, 0.52), (0.08, 0.10, 0.26))
    circ(0.30, 0.70, 0.08, (0.98, 0.96, 0.84), d=79)
    mesas_ao_longe(0.25, [(0.20, 0.14, 0.24), (0.26, 0.16, 0.26)], rng, n=3, alturas=(0.14, 0.26))
    ret(-0.05, -0.05, 1.05, 0.26, (0.14, 0.10, 0.16), d=77)
    img = render(*estado['px'])
    por_na_celula('lua', pintura(img, np.random.default_rng(21)))

    # --- a placa "Bem-vindo a Radiator Springs"
    preparar('placa')
    ret(-0.05, 0.25, 1.05, 1.05, P_CEU_CLARO, P_CEU)
    ret(-0.05, -0.05, 1.05, 0.26, P_AREIA, d=77)
    ret(0.20, 0.10, 0.23, 0.55, P_MARROM, d=75)
    ret(0.77, 0.10, 0.80, 0.55, P_MARROM, d=75)
    poli([(0.12, 0.45), (0.88, 0.45), (0.90, 0.82), (0.10, 0.82)], (0.95, 0.88, 0.70), d=74)
    texto('Bem-vindo a', FONTE_FINA, P_MARROM, 0.50, 0.73, 0.40, 0.06, d=60)
    texto('RADIATOR SPRINGS', FONTE_TITULO, (0.70, 0.18, 0.10), 0.50, 0.60, 0.72, 0.10, d=60)
    texto('a joia da Rota 66', FONTE_FINA, P_MARROM, 0.50, 0.50, 0.46, 0.05, d=60)
    img = render(*estado['px'])
    por_na_celula('placa', pintura(img, np.random.default_rng(22)))

    limpar_camada()
    atlas = np.round(atlas * 63) / 63
    # --- o tapete (só numpy): mostarda com bordas escuras, uma faixa de losangos e cones no meio
    _, x, y, w, h = CELULAS[NOME_DA_CELULA['tapete']]
    yy, xx = np.mgrid[0:h, 0:w].astype(np.float32)
    u = xx / (w - 1)
    v = yy / (h - 1)
    d = np.minimum(np.minimum(u, 1 - u) * (w / h), np.minimum(v, 1 - v))
    cor = np.zeros((h, w, 3), np.float32) + np.array((0.78, 0.62, 0.26))
    cor[d < 0.11] = (0.46, 0.28, 0.12)
    cor[(d >= 0.11) & (d < 0.14)] = (0.90, 0.78, 0.48)
    cor[(d >= 0.14) & (d < 0.20)] = (0.60, 0.30, 0.12)
    meio = (d >= 0.20)
    # losangos no miolo
    lu = (u * w / h * 6.0) % 1.0
    lv = (v * 6.0) % 1.0
    los = (np.abs(lu - 0.5) + np.abs(lv - 0.5)) < 0.28
    cor[meio & los] = cor[meio & los] * 0.70 + np.array((0.60, 0.26, 0.10)) * 0.30
    fibra = ruido(h, w, 0.8, 0.8, np.random.default_rng(23))
    pelo = ruido(h, w, 3.5, 3.5, np.random.default_rng(24))
    cor = cor * (1 + 0.08 * fibra[..., None] + 0.05 * pelo[..., None])
    franja = ((xx < 6) | (xx > w - 7)) & ((yy.astype(int) // 5) % 2 == 0)
    cor[franja] = (0.92, 0.86, 0.70)
    atlas[y:y + h, x:x + w] = np.clip(np.round(cor * 63) / 63, 0, 1)
    return atlas


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
    ATLAS_GUARDADO = imagem_do_glb(SAIDA + '.glb', 'recepcao_quadros')
if ATLAS_GUARDADO and os.path.exists(ATLAS_GUARDADO):
    ATLAS = ler_png(ATLAS_GUARDADO)[..., :3]
    print('  reaproveitado de', ATLAS_GUARDADO)
else:
    ATLAS = renderizar_quadros()
    if ATLAS_GUARDADO:
        salvar_png(ATLAS, ATLAS_GUARDADO)
TEXTURAS['quadros'] = ATLAS
IMG_QUADROS = imagem('recepcao_quadros', ATLAS)
limpar_cena()
print('  %.0f s' % (time.time() - INICIO))

# ------------------------------------------------------------------
# 3. A sala
# ------------------------------------------------------------------
TAM_MAX_DE_CANTOS = 56000      # cantos por malha (a raylib usa índices de 16 bits: menos de 65536 vértices)


class Malha:
    """As faces de um material (em bmesh), com as coordenadas de textura, a cor sem luz (tinta, linear)
    e o brilho (as lâmpadas). Quando uma enche, começa outra."""

    def __init__(self, nome, imagem=None):
        self.nome, self.imagem = nome, imagem
        self.bms = []
        self.novo()

    def novo(self):
        bm = bmesh.new()
        self.l_uv = bm.loops.layers.uv.new('UVMap')
        self.l_tinta = bm.loops.layers.float_color.new('tinta')
        self.l_brilho = bm.loops.layers.float_color.new('brilho')
        self.l_suave = bm.faces.layers.int.new('suave')
        self.bm = bm
        self.cantos = 0
        self.bms.append(bm)

    def face(self, pts, cor, n=None, uvs=None, brilho=None, suave=False):
        """suave: a face é de uma superfície grande e lisa (parede, teto, piso), em que a luz assada é
        alisada depois entre os vértices vizinhos de mesma normal"""
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
        if callable(brilho):
            brilhos = [brilho(p) for p in limpos]
        elif isinstance(brilho, list):
            brilhos = [brilho[k] for k in idx]
        else:
            brilhos = [brilho] * len(limpos)
        uvl = [uvs[k] for k in idx] if uvs is not None else [(0.0, 0.0)] * len(limpos)
        if n is not None and np.dot(nn, n) < 0:
            limpos, cores, uvl, brilhos = limpos[::-1], cores[::-1], uvl[::-1], brilhos[::-1]
        if self.cantos + len(limpos) > TAM_MAX_DE_CANTOS:
            self.novo()
        vs = [self.bm.verts.new((p[0], -p[2], p[1])) for p in limpos]
        try:
            f = self.bm.faces.new(vs)
        except ValueError:
            return
        self.cantos += len(limpos)
        TRIANGULOS_POR_PARTE[PARTE[0]] = TRIANGULOS_POR_PARTE.get(PARTE[0], 0) + len(limpos) - 2
        f[self.l_suave] = 1 if suave else 0
        for k, l in enumerate(f.loops):
            l[self.l_tinta] = (*linear(cores[k]), 1.0)
            l[self.l_uv].uv = uvl[k]
            b = brilhos[k]
            l[self.l_brilho] = (*linear(b), 1.0) if b is not None else (0.0, 0.0, 0.0, 1.0)


TRIANGULOS_POR_PARTE = {}     # para conferir onde vão os triângulos (o limite é ~100 mil)
PARTE = ['sala']


def parte(nome):
    PARTE[0] = nome


def normal_de(pts):
    n = np.zeros(3)
    m = len(pts)
    for k in range(m):
        a, b = np.array(pts[k]), np.array(pts[(k + 1) % m])
        n += np.cross(a, b)
    return n


MALHAS = {
    'cores': Malha('recepcao_cores'),
    'piso': Malha('recepcao_piso', IMG_PISO),
    'teto': Malha('recepcao_teto', IMG_TETO),
    'quadros': Malha('recepcao_quadros', IMG_QUADROS),
}
CORES = MALHAS['cores']
QUADROS = MALHAS['quadros']
COLISAO = []           # triângulos da colisão (eixos do jogo)
OBSTACULOS = []        # (nome, [(x, z), ...], altura) contornos dos móveis na colisão (paredes em pé, sem tampa)
ALTURA_DOS_MOVEIS = 1.6   # a altura mínima das caixas da colisão (o carro só bate no que passa de ~0,65 m)
# A câmera do jogo, aqui dentro (coberto), anda 5 m atrás e 2 m acima do carro (DISTANCIA_COBERTA de
# src/passeio.c): em y ~3,9. Ela não atravessa a colisão: o que for mais alto que ALTURA_DOS_MOVEIS (os cactos,
# os abajures de pé, a estante, as pilhas de cones) tem a caixa até a altura dele


def obstaculo_caixa(nome, x0, x1, z0, z1, altura=ALTURA_DOS_MOVEIS):
    OBSTACULOS.append((nome, [(x0, z0), (x1, z0), (x1, z1), (x0, z1)], max(altura, ALTURA_DOS_MOVEIS)))


def obstaculo_redondo(nome, x, z, r, n=8, altura=ALTURA_DOS_MOVEIS):
    OBSTACULOS.append((nome, [(x + r * math.cos(a), z + r * math.sin(a)) for a in np.linspace(0, 2 * math.pi, n, endpoint=False)],
                       max(altura, ALTURA_DOS_MOVEIS)))


def quad(m, a, b, c, d, cor, n=None, uv=None, uvf=None, passo=PASSO, brilho=None, div=None, suave=False):
    """Um quadrilátero (a, b, c, d em volta), subdividido numa grade de no máximo 'passo' metros"""
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
            m.face(pts, cor, n=n, uvs=uvs, brilho=brilho, suave=suave)


class Ref:
    """Um referencial no chão: o (x, z), u ao longo, n para a frente; h a partir de y0"""

    def __init__(self, o, u, n, y0=PISO_Y):
        self.o = np.asarray(o, float)
        self.u = unit(u)
        self.n = unit(n)
        self.y0 = y0

    def p(self, a, h, b):
        xz = self.o + a * self.u + b * self.n
        return np.array([xz[0], self.y0 + h, xz[1]])

    def d(self, du, dh, dn):
        v = du * self.u + dn * self.n
        return np.array([v[0], dh, v[1]])


R_MUNDO = Ref((0.0, 0.0), (1, 0), (0, 1))     # a = x, b = z, h acima do piso


def caixa(m, R, a0, a1, h0, h1, b0, b1, cor, sem=(), passo=PASSO, cores=None, brilho=None):
    """Uma caixa no referencial R. 'sem' tira faces ('frente', 'tras', 'esq', 'dir', 'cima', 'baixo')"""
    P = lambda a, h, b: R.p(a, h, b)
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
        quad(m, *pts, c, n=n, passo=passo, brilho=brilho)


def caixa_xz(m, x0, x1, y0, y1, z0, z1, cor, sem=(), passo=PASSO, cores=None, brilho=None):
    """Uma caixa alinhada aos eixos do jogo (y absoluto)"""
    R = Ref((0.0, 0.0), (1, 0), (0, 1), y0=0.0)
    caixa(m, R, x0, x1, y0, y1, z0, z1, cor, sem=sem, passo=passo, cores=cores, brilho=brilho)


def torno(m, centro, eixo, e1, perfil, n_seg, cor, ang=(0.0, 2 * math.pi), brilho=None, uvs=None, suave=False):
    """Gira o perfil [(raio, altura ao longo do eixo)] em volta do eixo. A face fica para o lado em
    que o perfil, andando, tem a sua direita (perfil subindo por fora => normais para fora)."""
    centro = np.asarray(centro, float)
    eixo = unit(eixo)
    e1 = unit(np.asarray(e1, float) - eixo * np.dot(e1, eixo))
    e2 = np.cross(eixo, e1)
    angs = np.linspace(ang[0], ang[1], n_seg + 1)
    for k in range(len(perfil) - 1):
        r0, h0 = perfil[k]
        r1, h1 = perfil[k + 1]
        dr, dh = r1 - r0, h1 - h0
        nr, nh = dh, -dr
        for j in range(n_seg):
            a0, a1 = angs[j], angs[j + 1]
            rad0 = math.cos(a0) * e1 + math.sin(a0) * e2
            rad1 = math.cos(a1) * e1 + math.sin(a1) * e2
            radm = unit(rad0 + rad1)
            pts = [centro + eixo * h0 + rad0 * r0, centro + eixo * h0 + rad1 * r0,
                   centro + eixo * h1 + rad1 * r1, centro + eixo * h1 + rad0 * r1]
            n = radm * nr + eixo * nh
            uv = None
            if uvs is not None:
                uv = [uvs(a0, h0), uvs(a1, h0), uvs(a1, h1), uvs(a0, h1)]
            m.face(pts, cor, n=n, uvs=uv, brilho=brilho, suave=suave)


def disco(m, centro, eixo, e1, raio, n_seg, cor, brilho=None, raio_dentro=0.0):
    """Um disco (ou anel) virado para 'eixo'"""
    torno(m, centro, eixo, e1, [(raio, 0.0), (raio_dentro, 0.0)], n_seg, cor, brilho=brilho)


def cilindro(m, centro, eixo, e1, raio, altura, n_seg, cor, tampa=True, base=False, brilho=None):
    torno(m, centro, eixo, e1, [(raio, 0.0), (raio, altura)], n_seg, cor, brilho=brilho)
    if tampa:
        disco(m, np.asarray(centro, float) + unit(eixo) * altura, eixo, e1, raio, n_seg, cor, brilho=brilho)
    if base:
        disco(m, centro, -unit(eixo), e1, raio, n_seg, cor, brilho=brilho)


def tubo(m, pts, raio, cor, lados=4, secao=None, fechar=False, brilho=None, raios=None, giro=0.0):
    """Um tubo (ou barra) ao longo da polilinha pts (eixos do jogo), com o referencial transportado ao
    longo dela. secao: [(ângulo, fator do raio)] (as costelas dos cactos); raios: um raio por ponto"""
    limpos, rs = [], []
    for k, p in enumerate(pts):
        p = np.asarray(p, float)
        if limpos and np.linalg.norm(p - limpos[-1]) < 1e-6:
            continue
        limpos.append(p)
        rs.append(raios[k] if raios is not None else raio)
    pts, raios = limpos, rs
    n = len(pts)
    if n < 2:
        return
    tang = []
    for k in range(n):
        if k == 0:
            t = pts[1] - pts[0]
        elif k == n - 1:
            t = pts[-1] - pts[-2]
        else:
            t = unit(pts[k + 1] - pts[k]) + unit(pts[k] - pts[k - 1])
        tang.append(unit(t))
    ref = np.array([0, 1.0, 0]) if abs(tang[0][1]) < 0.9 else np.array([1.0, 0, 0])
    nrm = unit(np.cross(tang[0], ref))
    if secao is None:
        secao = [(giro + 2 * math.pi * j / lados + math.pi / lados, 1.0) for j in range(lados)]
    aneis = []
    for k in range(n):
        if k > 0:
            nrm = unit(nrm - tang[k] * np.dot(nrm, tang[k]))
        bi = np.cross(tang[k], nrm)
        r = raios[k]
        aneis.append([pts[k] + r * f * (math.cos(a) * nrm + math.sin(a) * bi) for a, f in secao])
    ns = len(secao)
    for k in range(n - 1):
        for j in range(ns):
            j2 = (j + 1) % ns
            a, b, c, d = aneis[k][j], aneis[k][j2], aneis[k + 1][j2], aneis[k + 1][j]
            meio = (pts[k] + pts[k + 1]) / 2
            nn = (a + b + c + d) / 4 - meio
            if callable(cor):
                cc = [cor(k, j, a), cor(k, j2, b), cor(k + 1, j2, c), cor(k + 1, j, d)]
                m.face([a, b, c, d], cc, n=nn, brilho=brilho)
            else:
                m.face([a, b, c, d], cor, n=nn, brilho=brilho)
    if fechar:
        for k, s in ((0, -1), (n - 1, 1)):
            c0 = cor(k, 0, pts[k]) if callable(cor) else cor
            m.face(aneis[k], c0, n=tang[k] * s, brilho=brilho)
    return aneis


def elipsoide(m, centro, ex, ey, ez, cor, nu=10, nv=7, brilho=None):
    """Um elipsoide de semieixos ex, ey, ez (vetores do jogo)"""
    centro = np.asarray(centro, float)
    ex, ey, ez = (np.asarray(v, float) for v in (ex, ey, ez))

    def P(i, j):
        u = 2 * math.pi * i / nu
        v = math.pi * j / nv - math.pi / 2
        return centro + math.cos(v) * (math.cos(u) * ex + math.sin(u) * ez) + math.sin(v) * ey
    for i in range(nu):
        for j in range(nv):
            pts = [P(i, j), P(i + 1, j), P(i + 1, j + 1), P(i, j + 1)]
            nn = sum(pts) / 4 - centro
            m.face(pts, cor, n=nn, brilho=brilho)


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


# --- polígonos num plano (2D) ---
def area2(poly):
    a = 0.0
    for k in range(len(poly)):
        x1, z1 = poly[k]
        x2, z2 = poly[(k + 1) % len(poly)]
        a += x1 * z2 - x2 * z1
    return a / 2


def cortar_meio_plano(poly, a, n):
    """O que fica do polígono do lado (p - a)·n >= 0"""
    out = []
    m = len(poly)
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
        a, b = np.asarray(conv[k], float), np.asarray(conv[(k + 1) % len(conv)], float)
        e = b - a
        n = np.array([-e[1], e[0]]) * s
        poly = cortar_meio_plano(poly, a, n)
        if len(poly) < 3:
            return []
    return poly


def grade_no_convexo(conv, passo, origem=(0.0, 0.0)):
    xs = [p[0] for p in conv]
    zs = [p[1] for p in conv]
    x0 = origem[0] + math.floor((min(xs) - origem[0]) / passo) * passo
    z0 = origem[1] + math.floor((min(zs) - origem[1]) / passo) * passo
    pecas = []
    x = x0
    while x < max(xs) - 1e-9:
        z = z0
        while z < max(zs) - 1e-9:
            cel = [np.array([x, z]), np.array([x + passo, z]), np.array([x + passo, z + passo]), np.array([x, z + passo])]
            c = cortar_convexo(cel, conv)
            if len(c) >= 3 and abs(area2(c)) > 1e-6:
                pecas.append(c)
            z += passo
        x += passo
    return pecas


def separar_por_convexo(pecas, conv):
    """Separa cada peça em (dentro de conv, fora de conv)"""
    s = 1 if area2(conv) > 0 else -1
    dentro, fora = [], []
    xs = [p[0] for p in conv]
    zs = [p[1] for p in conv]
    for pc in pecas:
        px = [p[0] for p in pc]
        pz = [p[1] for p in pc]
        if max(px) < min(xs) + 1e-9 or min(px) > max(xs) - 1e-9 or max(pz) < min(zs) + 1e-9 or min(pz) > max(zs) - 1e-9:
            fora.append(pc)
            continue
        resto = pc
        for k in range(len(conv)):
            a, b = np.asarray(conv[k], float), np.asarray(conv[(k + 1) % len(conv)], float)
            e = b - a
            n = np.array([-e[1], e[0]]) * s
            f = cortar_meio_plano(resto, a, -n)
            if len(f) >= 3 and abs(area2(f)) > 1e-6:
                fora.append(f)
            resto = cortar_meio_plano(resto, a, n)
            if len(resto) < 3:
                break
        if len(resto) >= 3 and abs(area2(resto)) > 1e-6:
            dentro.append(resto)
    return dentro, fora


def limpar_poligono(pc, tol=1e-6):
    out = []
    for p in pc:
        p = (float(p[0]), float(p[1]))
        if not out or abs(p[0] - out[-1][0]) > tol or abs(p[1] - out[-1][1]) > tol:
            out.append(p)
    if len(out) > 1 and abs(out[0][0] - out[-1][0]) <= tol and abs(out[0][1] - out[-1][1]) <= tol:
        out.pop()
    return out


def sem_juncoes_em_t(polis, tol=1e-5):
    """Põe em cada aresta os vértices dos vizinhos que caem no meio dela (junções em T viram vértices
    de verdade: sem frestas na rasterização e sem listras na luz interpolada)"""
    if not polis:
        return []
    todos = np.unique(np.round(np.array([p for pc in polis for p in pc]), 6), axis=0)
    saida = []
    for pc in polis:
        novo = []
        for k in range(len(pc)):
            a, b = np.array(pc[k]), np.array(pc[(k + 1) % len(pc)])
            novo.append(pc[k])
            e = b - a
            L2 = float(e @ e)
            if L2 < 1e-12:
                continue
            t = (todos - a) @ e / L2
            d = np.abs((todos[:, 0] - a[0]) * e[1] - (todos[:, 1] - a[1]) * e[0]) / math.sqrt(L2)
            meio = (t > 1e-6) & (t < 1 - 1e-6) & (d < tol)
            for j in np.argsort(t[meio]):
                q = todos[meio][j]
                novo.append((float(q[0]), float(q[1])))
        saida.append(limpar_poligono(novo))
    return saida


def deslocar_poligono(P, d):
    """O polígono convexo P (anti-horário) deslocado d para fora (cantos em meia-esquadria)"""
    P = [np.asarray(p, float) for p in P]
    n = len(P)
    if area2(P) < 0:
        P = P[::-1]
    linhas = []
    for k in range(n):
        a, b = P[k], P[(k + 1) % n]
        e = unit(b - a)
        fora = np.array([e[1], -e[0]])
        linhas.append((a + fora * d, e))
    Q = []
    for k in range(n):
        (p1, e1), (p2, e2) = linhas[k - 1], linhas[k]
        A = np.array([[e1[0], -e2[0]], [e1[1], -e2[1]]])
        if abs(np.linalg.det(A)) < 1e-9:
            Q.append(p2)
        else:
            t = np.linalg.solve(A, p2 - p1)
            Q.append(p1 + t[0] * e1)
    return P, Q


# ------------------------------------------------------------------
# 3a. As paredes, com as aberturas no lugar das da casca
# ------------------------------------------------------------------
class Vao:
    """Uma abertura: um polígono convexo em (u, y) da parede; 'tipo' é janela, bandeira ou porta"""

    def __init__(self, nome, tipo, poligono, fundo=None):
        self.nome, self.tipo = nome, tipo
        P = [np.asarray(p, float) for p in poligono]
        if area2(P) < 0:
            P = P[::-1]
        self.P = P
        self.fundo = fundo
        self.u0 = min(p[0] for p in P)
        self.u1 = max(p[0] for p in P)
        self.y0 = min(p[1] for p in P)
        self.y1 = max(p[1] for p in P)


class Parede:
    def __init__(self, nome, fp, normal, contorno, fundo):
        self.nome = nome
        self.fp = fp
        self.n = unit(normal)
        P = [np.asarray(p, float) for p in contorno]
        if area2(P) < 0:
            P = P[::-1]
        self.contorno = P
        self.fundo = fundo           # da face da parede até o plano dos vidros da casca
        self.vaos = []

    def p(self, u, y, b=0.0):
        return self.fp(u, y, b)

    def direcao(self, du, dy):
        """O vetor do jogo que corresponde a (du, dy) no plano da parede"""
        return self.p(du, PISO_Y + dy, 0) - self.p(0.0, PISO_Y, 0)


def frente_pt(u, y, b):
    return np.array([u, y, z_frente(y) + b])


def oeste_pt(u, y, b):
    return np.array([X_OESTE + b, y, u])


def leste_pt(u, y, b):
    return np.array([X_LESTE - b, y, u])


def fundo_pt(u, y, b):
    return np.array([u, y, Z_FUNDO - b])


Y_TETO_FUNDO = y_teto(Z_FUNDO)
P_FRENTE = Parede('frente', frente_pt, (0, INCLINACAO, 1),
                  [(X_OESTE, PISO_Y), (X_LESTE, PISO_Y), (X_LESTE, Y_TOPO_FRENTE), (X_OESTE, Y_TOPO_FRENTE)], RECUO_DA_FRENTE)
_lado = [(Z_FRENTE_NO_PISO, PISO_Y), (Z_FUNDO, PISO_Y), (Z_FUNDO, Y_TETO_FUNDO), (Z_TOPO_FRENTE, Y_TOPO_FRENTE)]
P_OESTE = Parede('oeste', oeste_pt, (1, 0, 0), _lado, X_OESTE - X_VIDRO_OESTE)
P_LESTE = Parede('leste', leste_pt, (-1, 0, 0), _lado, X_VIDRO_LESTE - X_LESTE)
P_FUNDO = Parede('fundo', fundo_pt, (0, 0, -1),
                 [(X_OESTE, PISO_Y), (X_LESTE, PISO_Y), (X_LESTE, Y_TETO_FUNDO), (X_OESTE, Y_TETO_FUNDO)], Z_VIDRO_FUNDO - Z_FUNDO)
PAREDES = [P_FRENTE, P_LESTE, P_FUNDO, P_OESTE]


def u_da_quina_da_frente(y):
    """Nas paredes dos lados (u = z): a quina com a parede da frente, na altura y"""
    return z_frente(y)


def canto_inclinado(y0, topo_fn, desloc=CANTO):
    """O canto de cima, junto à frente, de uma abertura dos lados: onde a reta u = quina(y) + desloc
    encontra o topo y = topo_fn(u)"""
    y = y0
    for _ in range(40):
        u = u_da_quina_da_frente(y) + desloc
        y = topo_fn(u)
    return (u_da_quina_da_frente(y) + desloc, y)


def vao_dos_lados(nome, tipo, u1, y0, topo, da_frente=False, u0=None):
    """Uma abertura dos lados de u0 (ou da quina da frente) a u1, da base y0 ao topo (número ou função de u)"""
    tf = topo if callable(topo) else (lambda u, t=topo: t)
    if da_frente:
        a = (u_da_quina_da_frente(y0) + CANTO, y0)
        d = canto_inclinado(y0, tf)
    else:
        a = (u0, y0)
        d = (u0, tf(u0))
    if tf(u1) < y0 + 0.12:
        # o topo inclinado desce até a base antes de u1: corta a ponta fina (fica com 12 cm de altura)
        u1 = (TETO_A - 0.15 - y0 - 0.12) / TETO_K + TETO_Z0
    b = (u1, y0)
    c = (u1, tf(u1))
    return Vao(nome, tipo, [a, b, c, d])


def topo_da_bandeira(u):
    return y_teto(u) - 0.15


# A frente (u = x): as vitrines, as bandeiras de cima e a entrada
P_FRENTE.vaos += [
    Vao('vitrine_oeste', 'janela', [(X_OESTE + CANTO, 3.13), (693.85, 3.13), (693.85, 5.87), (X_OESTE + CANTO, 5.87)]),
    Vao('bandeira_oeste', 'bandeira', [(X_OESTE + CANTO, 6.18), (693.82, 6.18), (693.82, 7.44), (X_OESTE + CANTO, 7.44)]),
    Vao('entrada', 'porta', [(ENTRADA_X[0], PISO_Y), (ENTRADA_X[1], PISO_Y), (ENTRADA_X[1], ENTRADA_TOPO), (ENTRADA_X[0], ENTRADA_TOPO)]),
    Vao('vitrine_leste', 'janela', [(702.60, 3.13), (X_LESTE - CANTO, 3.13), (X_LESTE - CANTO, 5.87), (702.60, 5.87)]),
    Vao('bandeira_leste', 'bandeira', [(702.60, 6.18), (X_LESTE - CANTO, 6.18), (X_LESTE - CANTO, 7.38), (702.60, 7.38)]),
]
# (as aberturas daqui ficam dentro dos vãos que os recortes abrem no vidro da casca)
# Oeste (u = z): duas janelas com bandeira e a bandeira do meio (a parede cheia da casca fica embaixo dela)
P_OESTE.vaos += [
    vao_dos_lados('janela_1', 'janela', 186.95, 3.13, 5.89, da_frente=True),
    vao_dos_lados('bandeira_1', 'bandeira', 186.95, 6.19, topo_da_bandeira, da_frente=True),
    vao_dos_lados('bandeira_2', 'bandeira', 195.70, 6.25, topo_da_bandeira, u0=187.30),
    vao_dos_lados('janela_2', 'janela', Z_FUNDO - CANTO, 3.13, 5.86, u0=196.10),
    vao_dos_lados('bandeira_3', 'bandeira', Z_FUNDO - CANTO, 6.19, topo_da_bandeira, u0=196.10),
]
P_LESTE.vaos += [
    vao_dos_lados('janela_1', 'janela', 186.55, 3.13, 5.86, da_frente=True),
    vao_dos_lados('bandeira_1', 'bandeira', 186.55, 6.19, topo_da_bandeira, da_frente=True),
    vao_dos_lados('bandeira_2', 'bandeira', 195.45, 6.24, topo_da_bandeira, u0=186.95),
    vao_dos_lados('janela_2', 'janela', Z_FUNDO - CANTO, 3.13, 5.86, u0=195.75),
    vao_dos_lados('bandeira_3', 'bandeira', Z_FUNDO - CANTO, 6.19, topo_da_bandeira, u0=195.75),
]
P_FUNDO.vaos += [
    Vao('janela_1', 'janela', [(X_OESTE + CANTO, 3.13), (694.28, 3.13), (694.28, 5.85), (X_OESTE + CANTO, 5.85)]),
    Vao('janela_2', 'janela', [(703.03, 3.13), (X_LESTE - CANTO, 3.13), (X_LESTE - CANTO, 5.85), (703.03, 5.85)]),
]


def niveis_da_parede(W, base):
    ys = [base + k * PASSO_DA_PAREDE for k in range(int((max(p[1] for p in W.contorno) - base) / PASSO_DA_PAREDE) + 2)]
    for v in W.vaos:
        for p in v.P:
            ys = [y for y in ys if abs(y - p[1]) > 0.06] + [float(p[1])]
    ys.append(max(p[1] for p in W.contorno))
    return sorted(set(round(y, 6) for y in ys))


def montar_parede(W, cor=PAREDE):
    """A parede em células (colunas cortadas nas pontas das aberturas e a cada PASSO_DA_PAREDE, faixas
    nos níveis globais), recortadas pelo contorno dela e pelas aberturas; sem junções em T"""
    base = PISO_Y + FRISO
    # o contorno sem o friso de baixo
    cont = cortar_meio_plano(W.contorno, np.array([0.0, base]), np.array([0.0, 1.0]))
    us = {float(p[0]) for p in cont}
    for v in W.vaos:
        us.update(float(p[0]) for p in v.P)
    us = sorted(us)
    cortes = []
    for k in range(len(us) - 1):
        a, b = us[k], us[k + 1]
        n = max(1, math.ceil((b - a) / PASSO_DA_PAREDE - 1e-6))
        cortes += [a + (b - a) * j / n for j in range(n)]
    cortes.append(us[-1])
    niveis = niveis_da_parede(W, base)
    pecas = []
    for k in range(len(cortes) - 1):
        ua, ub = cortes[k], cortes[k + 1]
        if ub - ua < 1e-5:
            continue
        for j in range(len(niveis) - 1):
            ya, yb = niveis[j], niveis[j + 1]
            cel = [np.array([ua, ya]), np.array([ub, ya]), np.array([ub, yb]), np.array([ua, yb])]
            c = cortar_convexo(cel, cont)
            if len(c) >= 3 and abs(area2(c)) > 1e-6:
                pecas.append(c)
    for v in W.vaos:
        _, pecas = separar_por_convexo(pecas, v.P)
    polis = [limpar_poligono(pc) for pc in pecas]
    polis = [pc for pc in polis if len(pc) >= 3 and abs(area2(pc)) > 1e-6]
    for pc in sem_juncoes_em_t(polis):
        CORES.face([W.p(u, y) for u, y in pc], cor, n=W.n, suave=True)


def montar_friso(W, ua, ub):
    """O friso do pé da parede: triângulos laranja (para baixo) e amarelos (para cima), e o filete laranja"""
    L = 0.30
    H = FRISO
    k0 = math.floor(ua / L) - 1
    k1 = math.ceil(ub / L) + 1
    tris = []
    for k in range(k0, k1):
        u = k * L
        tris.append(([(u, H), (u + L, H), (u + L / 2, 0.0)], LARANJA))
        tris.append(([(u + L / 2, 0.0), (u + L, H), (u + 1.5 * L, 0.0)], AMARELO_FRISO))
    polis, cores = [], []
    for pts, cor in tris:
        pc = [np.array(p, float) for p in pts]
        pc = cortar_meio_plano(pc, np.array([ua, 0.0]), np.array([1.0, 0.0]))
        if len(pc) < 3:
            continue
        pc = cortar_meio_plano(pc, np.array([ub, 0.0]), np.array([-1.0, 0.0]))
        pc = limpar_poligono(pc)
        if len(pc) >= 3 and abs(area2(pc)) > 1e-7:
            polis.append(pc)
            cores.append(cor)
    for pc, cor in zip(sem_juncoes_em_t(polis), cores):
        CORES.face([W.p(u, PISO_Y + h) for u, h in pc], cor, n=W.n, suave=True)
    # o filete
    for (h0, h1, b0, b1) in ((H, H + 0.045, 0.0, 0.022),):
        quad(CORES, W.p(ua, PISO_Y + h0, b1), W.p(ub, PISO_Y + h0, b1), W.p(ub, PISO_Y + h1, b1), W.p(ua, PISO_Y + h1, b1),
             LARANJA, n=W.n, passo=0.6)
        quad(CORES, W.p(ua, PISO_Y + h1, b0), W.p(ub, PISO_Y + h1, b0), W.p(ub, PISO_Y + h1, b1), W.p(ua, PISO_Y + h1, b1),
             LARANJA, n=(0, 1, 0), passo=0.6)
        quad(CORES, W.p(ua, PISO_Y + h0, b0), W.p(ub, PISO_Y + h0, b0), W.p(ub, PISO_Y + h0, b1), W.p(ua, PISO_Y + h0, b1),
             LARANJA, n=(0, -1, 0), passo=0.6)


def montar_rebaixo(W, v, cor=MADEIRA, ressalto=0.03):
    """Fecha o rebaixo entre a nossa parede e o plano dos vidros da casca, em volta da abertura"""
    P = v.P
    n = len(P)
    centro = sum(P) / n
    for k in range(n):
        a, b = P[k], P[(k + 1) % n]
        if v.tipo == 'porta' and abs(a[1] - PISO_Y) < 1e-6 and abs(b[1] - PISO_Y) < 1e-6:
            continue
        fundo = W.fundo
        if v.tipo == 'porta' and abs(a[1] - ENTRADA_TOPO) < 1e-6 and abs(b[1] - ENTRADA_TOPO) < 1e-6:
            fundo = z_frente(ENTRADA_TOPO) - Z_DA_VERGA      # a verga da casca fica mais para fora
        e = b - a
        para_dentro = np.array([-e[1], e[0]])
        if np.dot(para_dentro, centro - a) < 0:
            para_dentro = -para_dentro
        nn = W.direcao(*(unit(para_dentro) * 0.01))
        quad(CORES, W.p(a[0], a[1], ressalto), W.p(b[0], b[1], ressalto), W.p(b[0], b[1], -fundo), W.p(a[0], a[1], -fundo),
             cor, n=nn, passo=0.5)
    # o peitoril de dentro, de madeira, embaixo das janelas (o de baixo dele fica 2 cm abaixo do peitoril da casca,
    # que entra uns centímetros na sala em y 3,063: sem faces quase coplanares)
    if v.tipo == 'janela':
        y = v.y0
        u0 = min(p[0] for p in P if abs(p[1] - y) < 1e-6) - 0.12
        u1 = max(p[0] for p in P if abs(p[1] - y) < 1e-6) + 0.12
        pts = lambda h, b: [W.p(u0, y + h, b), W.p(u1, y + h, b)]
        for (h0, h1, b0, b1) in ((-0.09, 0.0, 0.0, 0.17),):
            quad(CORES, *pts(h1, b0), *pts(h1, b1)[::-1], MADEIRA, n=(0, 1, 0), passo=0.5)
            quad(CORES, *pts(h0, b1), *pts(h1, b1)[::-1], MADEIRA, n=W.n, passo=0.5)
            quad(CORES, *pts(h0, b0), *pts(h0, b1)[::-1], MADEIRA_ESCURA, n=(0, -1, 0), passo=0.5)
            for uu, s in ((u0, -1), (u1, 1)):
                quad(CORES, W.p(uu, y + h0, b0), W.p(uu, y + h0, b1), W.p(uu, y + h1, b1), W.p(uu, y + h1, b0), MADEIRA,
                     n=W.direcao(s * 0.01, 0), div=(1, 1))


def montar_caixilho(W, v, larg=MOLDURA, ressalto=0.03):
    """A moldura de madeira em volta da abertura, na face da parede"""
    if v.tipo == 'porta':
        u0, u1 = ENTRADA_X
        y1 = ENTRADA_TOPO
        for (a0, a1, h0, h1) in ((u0 - 0.20, u0, PISO_Y, y1 + 0.20), (u1, u1 + 0.20, PISO_Y, y1 + 0.20), (u0, u1, y1, y1 + 0.20)):
            pts = [W.p(a0, h0, ressalto + 0.02), W.p(a1, h0, ressalto + 0.02), W.p(a1, h1, ressalto + 0.02), W.p(a0, h1, ressalto + 0.02)]
            quad(CORES, *pts, MADEIRA, n=W.n, passo=0.5)
        for (uu, h0, h1, s) in ((u0 - 0.20, PISO_Y, y1 + 0.20, -1), (u1 + 0.20, PISO_Y, y1 + 0.20, 1)):
            quad(CORES, W.p(uu, h0, 0), W.p(uu, h1, 0), W.p(uu, h1, ressalto + 0.02), W.p(uu, h0, ressalto + 0.02), MADEIRA,
                 n=W.direcao(s * 0.01, 0), passo=0.5)
        quad(CORES, W.p(u0 - 0.20, y1 + 0.20, 0), W.p(u1 + 0.20, y1 + 0.20, 0), W.p(u1 + 0.20, y1 + 0.20, ressalto + 0.02),
             W.p(u0 - 0.20, y1 + 0.20, ressalto + 0.02), MADEIRA, n=(0, 1, 0), passo=0.5)
        # a parte de dentro do batente, até a parede
        for uu, s in ((u0, 1), (u1, -1)):
            quad(CORES, W.p(uu, PISO_Y, ressalto), W.p(uu, y1, ressalto), W.p(uu, y1, ressalto + 0.02), W.p(uu, PISO_Y, ressalto + 0.02),
                 MADEIRA, n=W.direcao(s * 0.01, 0), passo=0.5)
        return
    P, Q = deslocar_poligono(v.P, larg)
    n = len(P)
    for k in range(n):
        a, b = P[k], P[(k + 1) % n]
        qa, qb = Q[k], Q[(k + 1) % n]
        if v.tipo == 'janela' and abs(a[1] - v.y0) < 1e-6 and abs(b[1] - v.y0) < 1e-6:
            continue          # embaixo fica o peitoril
        CORES.face([W.p(*a, ressalto), W.p(*b, ressalto), W.p(*qb, ressalto), W.p(*qa, ressalto)], MADEIRA, n=W.n)
        e = qb - qa
        fora = np.array([e[1], -e[0]])
        nn = W.direcao(*(unit(fora) * 0.01))
        CORES.face([W.p(*qa, 0), W.p(*qb, 0), W.p(*qb, ressalto), W.p(*qa, ressalto)], MADEIRA, n=nn)


def montar_pilastras():
    """As pilastras de madeira dos quatro cantos (as da frente seguem a inclinação da parede)"""
    L = PILASTRA
    for W, u0, u1 in ((P_FRENTE, X_OESTE, X_OESTE + L), (P_FRENTE, X_LESTE - L, X_LESTE)):
        # na frente: u ao longo de x, b para dentro (+z)
        topo = Y_TOPO_FRENTE
        sinal = 1 if u0 == X_OESTE else -1
        uin = u1 if sinal == 1 else u0
        pts_frente = [W.p(u0, PISO_Y, L), W.p(u1, PISO_Y, L), W.p(u1, topo, L), W.p(u0, topo, L)]
        quad(CORES, *pts_frente, MADEIRA, n=(0, INCLINACAO, 1), passo=0.5)
        quad(CORES, W.p(uin, PISO_Y, 0), W.p(uin, PISO_Y, L), W.p(uin, topo, L), W.p(uin, topo, 0), MADEIRA,
             n=(sinal, 0, 0), passo=0.5)
    for (x0, x1, sx) in ((X_OESTE, X_OESTE + L, 1), (X_LESTE - L, X_LESTE, -1)):
        z0, z1 = Z_FUNDO - L, Z_FUNDO
        y1 = y_teto(z0)
        xin = x1 if sx == 1 else x0
        quad(CORES, (x0, PISO_Y, z0), (x1, PISO_Y, z0), (x1, y1, z0), (x0, y1, z0), MADEIRA, n=(0, 0, -1), passo=0.5)
        quad(CORES, (xin, PISO_Y, z0), (xin, PISO_Y, z1), (xin, y1, z1), (xin, y1, z0), MADEIRA, n=(sx, 0, 0), passo=0.5)


print('Montando a sala...')
parte('paredes')
for W in PAREDES:
    montar_parede(W)
    for v in W.vaos:
        montar_rebaixo(W, v)
        montar_caixilho(W, v)
# o friso: de pilastra a pilastra, menos na entrada
for W in PAREDES:
    us = [p[0] for p in W.contorno]
    ua, ub = min(us) + PILASTRA, max(us) - PILASTRA
    if W is P_FRENTE:
        montar_friso(W, ua, ENTRADA_X[0] - 0.20)
        montar_friso(W, ENTRADA_X[1] + 0.20, ub)
    else:
        if W in (P_OESTE, P_LESTE):
            ua = Z_FRENTE_NO_PISO + PILASTRA
        montar_friso(W, ua, ub)
montar_pilastras()


def montar_painel_dos_fundos():
    """Atrás da estante, virada para fora: a cortina que o vão do meio da casca nos fundos (sem o vidro) mostra.
    Pregas de largura e fundura variáveis, mais franzidas nas pontas; o bandô com o varão em cima e a barra
    embaixo; acesa por dentro (a cor é fixa, com as dobras sombreadas)"""
    x0, x1 = VAO_DOS_FUNDOS_X[0] - 0.10, VAO_DOS_FUNDOS_X[1] + 0.10
    z0 = Z_VIDRO_FUNDO - 0.17                 # o meio das pregas (entre a nossa parede e o vidro da casca)
    y0, y1 = PISO_Y + 0.06, 5.78
    rng = np.random.default_rng(9)
    # as pregas: a fase anda mais depressa perto das pontas (o franzido) e varia devagar no meio; as pregas das
    # pontas são mais fundas
    L = x1 - x0
    xs, fase, fundura = [x0], [0.0], []
    while True:
        t = (xs[-1] - x0) / L
        ponta = max(0.0, 1 - min(t, 1 - t) / 0.18)
        f = (1.0 + 1.3 * ponta) * (1 + 0.30 * math.sin(2 * math.pi * (t * 3.1 + 0.2)) * math.sin(2 * math.pi * (t * 1.3 + 0.7)))
        fundura.append((0.030 + 0.035 * (0.5 + 0.5 * math.sin(xs[-1] * 1.3 + 0.4) * math.sin(xs[-1] * 0.7))) * (1 + 0.5 * ponta))
        if xs[-1] >= x1 - 1e-6:
            break
        dx = min(x1 - xs[-1], 0.05 / f)
        xs.append(xs[-1] + dx)
        fase.append(fase[-1] + dx * f * 2 * math.pi / 0.34)
    fase = np.array(fase)
    fundura = np.array(fundura)
    zs = z0 + fundura * np.sin(fase)
    dz = np.gradient(zs, np.array(xs))
    nivs = np.linspace(y0, y1, 7)
    cor = (0.86, 0.50, 0.26)

    def tom(k, y):
        nx = -dz[k] / math.sqrt(1 + dz[k] ** 2)            # a normal da prega (virada para fora, +z)
        luz = 0.58 + 0.42 * max(0.0, 0.25 * nx + 0.97 / math.sqrt(1 + dz[k] ** 2))
        vale = 0.80 + 0.20 * (math.sin(fase[k]) * 0.5 + 0.5)  # o fundo das pregas fica mais escuro
        alto = 0.92 + 0.08 * (y - y0) / (y1 - y0)
        return tuple(float(c * luz * vale * alto) for c in cor)
    for k in range(len(xs) - 1):
        for j in range(len(nivs) - 1):
            ya, yb = nivs[j], nivs[j + 1]
            pts = [(xs[k], ya, zs[k]), (xs[k + 1], ya, zs[k + 1]), (xs[k + 1], yb, zs[k + 1]), (xs[k], yb, zs[k])]
            brilhos = [tom(k, ya), tom(k + 1, ya), tom(k + 1, yb), tom(k, yb)]
            CORES.face(pts, BRANCO, n=(0, 0, 1), brilho=brilhos)
    # a barra embaixo: uma faixa um pouco mais escura, sem pregas fundas
    barra = escurecer(cor, 0.62)
    quad(CORES, (x0, PISO_Y + 0.02, z0 + 0.07), (x1, PISO_Y + 0.02, z0 + 0.07), (x1, y0 + 0.10, z0 + 0.07), (x0, y0 + 0.10, z0 + 0.07),
         BRANCO, n=(0, 0, 1), passo=0.8, brilho=barra)
    # o bandô (madeira) e o varão, em cima
    caixa_xz(CORES, x0 - 0.12, x1 + 0.12, y1 - 0.02, y1 + 0.17, z0 - 0.09, z0 + 0.10, BRANCO, sem=('tras',), passo=0.6,
             brilho=escurecer(MADEIRA, 0.85))
    tubo(CORES, [(x0 - 0.08, y1 - 0.04, z0), (x1 + 0.08, y1 - 0.04, z0)], 0.018, BRANCO, lados=6, brilho=LATAO)


parte('cortina')
montar_painel_dos_fundos()


def plano_do_vidro(x, y, nome):
    """O ponto (x, y) no plano do vidro da casca de uma vitrine/janela (frente: inclinado; fundos: quase em pé)
    e a normal para fora"""
    if 'fundos' in nome:
        z = 204.095 + 0.00529 * (x - 689.21) - 0.0034 * (y - 3.355)
        return np.array([x, y, z]), unit((-0.00529, 0.0034, 1.0))
    z = 180.163 - 0.29979 * (y - 1.973)
    return np.array([x, y, z]), unit((0.0, -0.29979, -1.0))


# quanto a moldura passa do vão de cada lado (oeste, leste, embaixo, em cima); na janela de leste dos fundos, o
# pilar da casca fica logo a oeste do vão, com a face no plano da moldura: ali ela não passa dele
PASSA_DO_VAO = {'recepcao_vidro_fundos_janela_leste': (0.015, 0.12, 0.12, 0.12)}


def baguetes_dos_vidros(largura=0.12, por_dentro=0.015, afastada=0.03):
    """O vidro da casca sobra em volta de cada vão recortado, entre o corte e o batente, o peitoril e a verga
    dela (que vão se afastando do vidro): visto de fora, de lado, aparece uma faixa cinza. Uma moldura fina
    de madeira, 3 cm para fora do plano do vidro, cobre essa faixa (de dentro, ela fica atrás da nossa parede)"""
    cor = (0.40, 0.22, 0.10)
    for nome, _, (x0, x1), (y0, y1), _, _ in RECORTES_DA_CASCA:
        if 'vao_do_meio' in nome or 'sapata' in nome:
            continue                     # (o vão do meio dos fundos fica atrás da cortina e dos pilares)
        lo, ll, lb, lc = PASSA_DO_VAO.get(nome, (largura,) * 4)
        fora = [(x0 - lo, y0 - lb), (x1 + ll, y0 - lb), (x1 + ll, y1 + lc), (x0 - lo, y1 + lc)]
        dentro = [(x0 + por_dentro, y0 + por_dentro), (x1 - por_dentro, y0 + por_dentro), (x1 - por_dentro, y1 - por_dentro),
                  (x0 + por_dentro, y1 - por_dentro)]
        for k in range(4):
            pts, n = [], None
            for (x, y) in (fora[k], fora[(k + 1) % 4], dentro[(k + 1) % 4], dentro[k]):
                p, n = plano_do_vidro(x, y, nome)
                pts.append(p + n * afastada)
            CORES.face(pts, BRANCO, n=n, brilho=cor)


parte('baguetes')
baguetes_dos_vidros()


# ------------------------------------------------------------------
# 3b. O piso, o capacho, o tapete, o teto e as vigas
# ------------------------------------------------------------------
PECAS_DO_PISO = [
    [(X_OESTE, Z_FRENTE_NO_PISO), (X_LESTE, Z_FRENTE_NO_PISO), (X_LESTE, Z_FUNDO), (X_OESTE, Z_FUNDO)],
    [(ENTRADA_X[0], Z_DA_SOLEIRA), (ENTRADA_X[1], Z_DA_SOLEIRA), (ENTRADA_X[1], Z_FRENTE_NO_PISO), (ENTRADA_X[0], Z_FRENTE_NO_PISO)],
]
ORIGEM_DOS_LADRILHOS = ((ENTRADA_X[0] + ENTRADA_X[1]) / 2 - 0.5 * LADRILHO, Z_FRENTE_NO_PISO)


def uv_do_piso(x, z):
    return ((x - ORIGEM_DOS_LADRILHOS[0]) / METROS_DO_PISO, -(z - ORIGEM_DOS_LADRILHOS[1]) / METROS_DO_PISO)


def montar_piso():
    up = np.array([0, 1.0, 0])
    for pc in PECAS_DO_PISO:
        for c in grade_no_convexo([np.asarray(p) for p in pc], PASSO_DO_PISO, origem=ORIGEM_DOS_LADRILHOS):
            MALHAS['piso'].face([(p[0], PISO_Y, p[1]) for p in c], BRANCO, n=up, uvs=[uv_do_piso(p[0], p[1]) for p in c], suave=True)
    # a borda da soleira desce até a praça (3 cm)
    x0, x1 = ENTRADA_X
    z0 = Z_DA_SOLEIRA
    quad(MALHAS['piso'], (x0, PISO_Y, z0), (x1, PISO_Y, z0), (x1, Y_DA_PRACA + 0.004, z0 - 0.10), (x0, Y_DA_PRACA + 0.004, z0 - 0.10),
         escurecer(BRANCO, 0.85), n=(0, 1, -0.3), uv=[uv_do_piso(x0, z0), uv_do_piso(x1, z0), uv_do_piso(x1, z0 - 0.1), uv_do_piso(x0, z0 - 0.1)],
         div=(12, 1))


def tapete(nome, x0, x1, z0, z1, altura=0.012, girar=False):
    """Um tapete (célula do atlas), com uma borda fina em volta"""
    u0, v0, u1, v1 = uv_da_celula(nome)
    y = PISO_Y + altura
    if girar:
        uv = [(u0, v1), (u0, v0), (u1, v0), (u1, v1)]
    else:
        uv = [(u0, v0), (u1, v0), (u1, v1), (u0, v1)]
    # o x cresce para a direita de quem entra (olhando +z, a direita é -x): a frente do tapete fica na entrada
    quad(QUADROS, (x1, y, z0), (x0, y, z0), (x0, y, z1), (x1, y, z1), BRANCO, n=(0, 1, 0), uv=uv, passo=0.35)
    cx, cz = (x0 + x1) / 2, (z0 + z1) / 2
    for (a, b) in (((x0, z0), (x1, z0)), ((x1, z0), (x1, z1)), ((x1, z1), (x0, z1)), ((x0, z1), (x0, z0))):
        meio = ((a[0] + b[0]) / 2 - cx, 0.0, (a[1] + b[1]) / 2 - cz)
        quad(CORES, (a[0], PISO_Y, a[1]), (b[0], PISO_Y, b[1]), (b[0], y, b[1]), (a[0], y, a[1]), (0.30, 0.20, 0.10),
             n=meio, div=(4, 1))


def montar_teto():
    pc = [(X_OESTE, Z_TOPO_FRENTE), (X_LESTE, Z_TOPO_FRENTE), (X_LESTE, Z_FUNDO), (X_OESTE, Z_FUNDO)]
    down = np.array([0, -1.0, 0])
    for c in grade_no_convexo([np.asarray(p) for p in pc], PASSO_DO_TETO, origem=(X_OESTE, Z_FUNDO)):
        MALHAS['teto'].face([(p[0], y_teto(p[1]), p[1]) for p in c], TETO, n=down,
                            uvs=[(p[0] / METROS_DO_TETO, -p[1] / METROS_DO_TETO) for p in c], suave=True)


def viga_sob_o_teto(x0, x1, z0, z1, altura, cor=VIGA):
    """Uma viga encostada no teto inclinado (o alto dela segue o teto)"""
    def P(x, z, h):
        return (x, y_teto(z) - h, z)
    quad(CORES, P(x0, z0, altura), P(x1, z0, altura), P(x1, z1, altura), P(x0, z1, altura), cor, n=(0, -1, 0), passo=0.6)
    quad(CORES, P(x0, z0, 0), P(x0, z1, 0), P(x0, z1, altura), P(x0, z0, altura), cor, n=(-1, 0, 0), passo=0.6)
    quad(CORES, P(x1, z0, 0), P(x1, z1, 0), P(x1, z1, altura), P(x1, z0, altura), cor, n=(1, 0, 0), passo=0.6)
    quad(CORES, P(x0, z0, 0), P(x1, z0, 0), P(x1, z0, altura), P(x0, z0, altura), cor, n=(0, 0, -1), passo=0.6)
    quad(CORES, P(x0, z1, 0), P(x1, z1, 0), P(x1, z1, altura), P(x0, z1, altura), cor, n=(0, 0, 1), passo=0.6)


def montar_vigas():
    # duas vigas mestras de frente a fundo, na linha dos pilares da entrada
    for x in (ENTRADA_X[0] - 0.05, ENTRADA_X[1] + 0.05):
        viga_sob_o_teto(x - 0.14, x + 0.14, Z_TOPO_FRENTE + 0.05, Z_FUNDO - 0.02, 0.34)
    # as travessas
    for z in (183.6, 188.2, 192.8, 197.4, 201.6):
        viga_sob_o_teto(X_OESTE + 0.02, X_LESTE - 0.02, z - 0.11, z + 0.11, 0.24)


parte('piso')
montar_piso()
tapete('capacho', 696.25, 700.15, 179.55, 181.45)
tapete('tapete', 700.95, 706.15, 192.15, 195.40)
parte('teto e vigas')
montar_teto()
montar_vigas()
print('  paredes, piso e teto: %.0f s' % (time.time() - INICIO))


# ------------------------------------------------------------------
# 3c. Cones de trânsito
# ------------------------------------------------------------------
def cone(m, x, z, y0, H, faixa='branca', giro=0.0, cor=CONE, lados=None, base_cor=None, so_base=False, ate=None):
    """Um cone de trânsito: a base quadrada e o corpo, com faixa branca, duas faixas ou faixa preta.
    so_base: só a base (os de baixo de uma pilha); ate: o corpo só até esta altura (o resto fica escondido)"""
    lados = lados or (14 if H > 0.5 else 10 if H > 0.22 else 7)
    S = 0.60 * H
    t = max(0.045 * H, 0.012)
    R = Ref((x, z), (math.cos(giro), math.sin(giro)), (-math.sin(giro), math.cos(giro)), y0=y0)
    bc = base_cor or escurecer(cor, 0.80)
    caixa(m, R, -S / 2, S / 2, 0, t, -S / 2, S / 2, bc, sem=('baixo',), passo=0.6)
    if so_base:
        return
    r0, r1 = 0.25 * H, 0.032 * H
    raio = lambda h: r0 + (r1 - r0) * (h - t) / (H - t)
    cortes = [t, H]
    if faixa == 'branca':
        faixas = [(0.52 * H, 0.68 * H, CONE_BRANCO)]
    elif faixa == 'duas':
        faixas = [(0.40 * H, 0.50 * H, CONE_BRANCO), (0.62 * H, 0.72 * H, CONE_BRANCO)]
    elif faixa == 'preta':
        faixas = [(0.50 * H, 0.64 * H, CONE_PRETO)]
    else:
        faixas = []
    for f0, f1, _ in faixas:
        cortes += [f0, f1]
    cortes = sorted(cortes)
    topo = H if ate is None else min(H, ate)
    for k in range(len(cortes) - 1):
        h0, h1 = cortes[k], min(cortes[k + 1], topo)
        if h1 <= h0 + 1e-6:
            continue
        c = cor
        for f0, f1, fc in faixas:
            if f0 - 1e-6 <= h0 and h1 <= f1 + 1e-6:
                c = fc
        # o laranja fica um pouco mais escuro embaixo (o cone é de borracha, fosca)
        if c is cor:
            cc = lambda p, h0=h0: mistura(escurecer(cor, 0.88), cor, float(np.clip((p[1] - y0) / H, 0, 1)))
        else:
            cc = c
        torno(m, (x, y0, z), (0, 1, 0), (math.cos(giro), 0, math.sin(giro)), [(raio(h0), h0), (raio(h1), h1)], lados, cc)
    if ate is None:
        disco(m, (x, y0 + H, z), (0, 1, 0), (1, 0, 0), r1, lados, cor)


def pilha_de_cones(m, x, z, y0, H, quantos, faixa='branca', giro=0.0, cor=CONE):
    """Cones empilhados (um dentro do outro): só as bases e o pedaço do corpo entre elas aparecem"""
    passo = 0.11 * H
    for k in range(quantos):
        yk = y0 + k * passo
        ultimo = k == quantos - 1
        cone(m, x, z, yk, H, faixa=faixa if ultimo else None, giro=giro + k * 0.03, cor=cor,
             ate=None if ultimo else passo + 0.045 * H)
    return y0 + (quantos - 1) * passo + H


def piramide_de_cones(m, x, z, y0, H, base=4, giro=0.0, cor=CONE, faixa=None):
    """Cones pequenos em pirâmide (de frente): base, base-1, ... (cada um apoiado nos dois de baixo)"""
    S = 0.62 * H
    d = np.array([math.cos(giro), math.sin(giro)])
    for fila in range(base):
        n = base - fila
        for k in range(n):
            a = (k - (n - 1) / 2) * S
            y = y0 + fila * H * 0.62
            cone(m, x + a * d[0], z + a * d[1], y, H, faixa=faixa, giro=giro, cor=cor)


# ------------------------------------------------------------------
# 3d. O balcão: rolos pretos acolchoados, o tampo, a prateleira redonda; e o que fica em cima dele
# ------------------------------------------------------------------
R_BALCAO = Ref((BALCAO_X[0], BALCAO_Z[0]), (1, 0), (0, 1))    # a ao longo (+x), b para trás (+z)
L_BALCAO = BALCAO_X[1] - BALCAO_X[0]
P_BALCAO = BALCAO_Z[1] - BALCAO_Z[0]
RAIO_DA_PONTA = 0.60
H_PLINTO = 0.13
R_ROLO = 0.215
H_TAMPO = H_PLINTO + 4 * R_ROLO + 0.005
E_TAMPO = 0.08
TOPO_DO_BALCAO = PISO_Y + H_TAMPO + E_TAMPO


def rolo(a0, a1, h, b, raio, lados=16, passo=0.16, tufos=0.75):
    """Um rolo acolchoado deitado (ao longo de a): vincos a cada 'tufos' metros e a ponta de leste redonda"""
    n_a = max(2, math.ceil((a1 - a0) / passo))
    an = np.linspace(a0, a1, n_a + 1)
    eixo_pts = []
    raios = []
    for a in an:
        fase = (a - a0) / tufos
        vinco = math.exp(-((fase - round(fase)) * tufos / 0.05) ** 2)
        eixo_pts.append(R_BALCAO.p(a, h, b))
        raios.append(raio * (1 - 0.07 * vinco))

    luz = unit(np.array([0, 0.75, -0.66]))     # de onde vem o realce: de cima e da frente (de quem chega)

    def cor(k, j, p):
        a = an[min(k, len(an) - 1)]
        fase = (a - a0) / tufos
        vinco = math.exp(-((fase - round(fase)) * tufos / 0.05) ** 2)
        nrm = unit(np.asarray(p) - R_BALCAO.p(a, h, b))
        r = float(np.clip((np.dot(nrm, luz) - 0.55) / 0.40, 0, 1)) ** 1.5
        return escurecer(mistura(ROLO, ROLO_REALCE, r * (1 - vinco)), 1.0 - 0.45 * vinco)
    tubo(CORES, eixo_pts, raio, cor, raios=raios, lados=lados)
    # a ponta de leste: meia esfera
    c = R_BALCAO.p(a1, h, b)
    perfil = [(raio * math.cos(t), raio * math.sin(t)) for t in np.linspace(0, math.pi / 2, 5)]
    torno(CORES, c, (1, 0, 0), (0, 1, 0), perfil, lados, ROLO)


def montar_balcao():
    R = R_BALCAO
    L, P = L_BALCAO, P_BALCAO
    # o plinto (madeira escura, um pouco recuado) e o corpo atrás dos rolos
    caixa(CORES, R, 0.0, L + 0.02, 0.0, H_PLINTO, 0.06, P - 0.02, MADEIRA_ESCURA, sem=('baixo',))
    caixa(CORES, R, 0.0, L - 0.05, H_PLINTO, H_TAMPO, 2 * R_ROLO - 0.06, P - 0.03, MADEIRA_ESCURA, sem=('baixo', 'cima'))
    # os rolos (dois, empilhados), da prateleira redonda até a ponta de leste
    for k in range(2):
        rolo(0.02, L - R_ROLO, H_PLINTO + R_ROLO * (1 + 2 * k), R_ROLO, R_ROLO)
    # o tampo: retângulo e meio disco na ponta de oeste (sobre a prateleira redonda), com a borda arredondada
    b0, b1 = -0.07, P + 0.05
    cb = (b0 + b1) / 2
    rp = (b1 - b0) / 2
    h0, h1 = H_TAMPO, H_TAMPO + E_TAMPO
    # em cima: grade no retângulo e leque no meio disco
    for c in grade_no_convexo([np.array(p) for p in [(0.0, b0), (L + 0.06, b0), (L + 0.06, b1), (0.0, b1)]], 0.35, origem=(0.0, b0)):
        CORES.face([R.p(p[0], h1, p[1]) for p in c], MADEIRA_TAMPO, n=(0, 1, 0))
    arco = [(-rp * math.sin(t), cb + rp * math.cos(t)) for t in np.linspace(math.pi, 0, 13)]
    for k in range(len(arco) - 1):
        CORES.face([R.p(0.0, h1, cb), R.p(arco[k][0], h1, arco[k][1]), R.p(arco[k + 1][0], h1, arco[k + 1][1])],
                   MADEIRA_TAMPO, n=(0, 1, 0))
        CORES.face([R.p(0.0, h0, cb), R.p(arco[k][0], h0, arco[k][1]), R.p(arco[k + 1][0], h0, arco[k + 1][1])],
                   MADEIRA_ESCURA, n=(0, -1, 0))
    caixa(CORES, R, 0.0, L + 0.06, h0, h0 + 0.001, b0, b1, MADEIRA_ESCURA, sem=('cima', 'frente', 'tras', 'esq', 'dir'))
    # a borda do tampo (frente arredondada: três faixas)
    borda = [(0.0, h0 + 0.015), (0.03, h0 + 0.035), (0.03, h1 - 0.025), (0.0, h1)]
    pts_arco = [(-rp * math.sin(t), cb + rp * math.cos(t)) for t in np.linspace(0, math.pi, 13)]
    contorno_todo = [(0.0, b0), (L + 0.06, b0), (L + 0.06, b1), (0.0, b1)] + pts_arco[1:-1]
    m = len(contorno_todo)
    for k in range(m):
        a, b = np.array(contorno_todo[k]), np.array(contorno_todo[(k + 1) % m])
        e = b - a
        fora = unit(np.array([e[1], -e[0]]))
        if area2(contorno_todo) < 0:
            fora = -fora
        for j in range(len(borda) - 1):
            (d0, y0), (d1, y1) = borda[j], borda[j + 1]
            pa0 = a + fora * d0
            pb0 = b + fora * d0
            pa1 = a + fora * d1
            pb1 = b + fora * d1
            nn = R.d(fora[0], 0.6 * (j - 1), fora[1])
            CORES.face([R.p(pa0[0], y0, pa0[1]), R.p(pb0[0], y0, pb0[1]), R.p(pb1[0], y1, pb1[1]), R.p(pa1[0], y1, pa1[1])],
                       MADEIRA_TAMPO if j > 0 else MADEIRA_ESCURA, n=nn)
    # a prateleira redonda na ponta de oeste: meio cilindro aberto, com 3 tábuas e 5 colunas
    rr = RAIO_DA_PONTA
    cbx = 0.0
    centro = R.p(cbx, 0.0, cb)
    angs = np.linspace(math.pi / 2, 3 * math.pi / 2, 9)          # do lado de oeste (a < 0)
    for hh, esp, cor in ((0.0, H_PLINTO, MADEIRA_ESCURA), (0.46, 0.035, MADEIRA), (0.80, 0.035, MADEIRA)):
        for k in range(len(angs) - 1):
            a0, a1 = angs[k], angs[k + 1]
            p0 = (cbx + rr * math.cos(a0), cb + rr * math.sin(a0))
            p1 = (cbx + rr * math.cos(a1), cb + rr * math.sin(a1))
            CORES.face([R.p(cbx, hh + esp, cb), R.p(p0[0], hh + esp, p0[1]), R.p(p1[0], hh + esp, p1[1])], cor, n=(0, 1, 0))
            if hh > 0:
                CORES.face([R.p(cbx, hh, cb), R.p(p0[0], hh, p0[1]), R.p(p1[0], hh, p1[1])], escurecer(cor, 0.8), n=(0, -1, 0))
            nn = R.d(math.cos((a0 + a1) / 2), 0, math.sin((a0 + a1) / 2))
            CORES.face([R.p(p0[0], hh, p0[1]), R.p(p1[0], hh, p1[1]), R.p(p1[0], hh + esp, p1[1]), R.p(p0[0], hh + esp, p0[1])],
                       cor, n=nn)
    for t in np.linspace(math.pi / 2, 3 * math.pi / 2, 5):
        c = R.p(cbx + (rr - 0.03) * math.cos(t), 0.0, cb + (rr - 0.03) * math.sin(t))
        cilindro(CORES, c, (0, 1, 0), (1, 0, 0), 0.028, H_TAMPO, 6, MADEIRA_ESCURA, tampa=False)
    # o fundo da prateleira (a ponta dos rolos)
    quad(CORES, R.p(0.015, H_PLINTO, b0 + 0.05), R.p(0.015, H_PLINTO, b1 - 0.05), R.p(0.015, H_TAMPO, b1 - 0.05),
         R.p(0.015, H_TAMPO, b0 + 0.05), MADEIRA_ESCURA, n=(-1, 0, 0), passo=0.3)
    # os cones pequenos e as miniaturas nas três tábuas
    rng = np.random.default_rng(41)
    for hh in (H_PLINTO, 0.495, 0.835):
        for t in np.linspace(math.pi / 2 + 0.35, 3 * math.pi / 2 - 0.35, 6):
            r = rr * rng.uniform(0.55, 0.78)
            x, z = R.p(cbx + r * math.cos(t), 0, cb + r * math.sin(t))[[0, 2]]
            y = PISO_Y + hh
            if rng.uniform() < 0.62:
                H = rng.uniform(0.12, 0.24) if hh < 0.8 else rng.uniform(0.10, 0.15)
                cone(CORES, x, z, y, H, faixa='branca' if rng.uniform() < 0.5 else None, giro=rng.uniform(0, 1))
            else:
                miniatura(x, z, y, rng.uniform(0, 2 * math.pi), rng)
    # por trás, o lado de quem atende: duas gavetas
    for a0 in (1.2, 3.6):
        caixa(CORES, R, a0, a0 + 1.6, 0.45, 0.85, P - 0.03, P - 0.01, MADEIRA, sem=('tras',))
        caixa(CORES, R, a0 + 0.7, a0 + 0.9, 0.62, 0.68, P - 0.01, P + 0.02, LATAO)
    # a colisão: o contorno do balcão com a ponta redonda
    pts = [R.p(L + 0.06, 0, b0)[[0, 2]], R.p(L + 0.06, 0, b1)[[0, 2]]]
    pts += [R.p(cbx + rr * math.cos(t), 0, cb + rr * math.sin(t))[[0, 2]] for t in np.linspace(math.pi / 2, 3 * math.pi / 2, 7)]
    OBSTACULOS.append(('balcao', [tuple(p) for p in pts], ALTURA_DOS_MOVEIS))


def miniatura(x, z, y, giro, rng):
    """Um carrinho em miniatura (corpo, cabine, quatro rodas)"""
    cor = [(0.20, 0.42, 0.78), (0.80, 0.16, 0.12), (0.30, 0.60, 0.40), (0.92, 0.80, 0.30)][rng.integers(0, 4)]
    R = Ref((x, z), (math.cos(giro), math.sin(giro)), (-math.sin(giro), math.cos(giro)), y0=y)
    caixa(CORES, R, -0.07, 0.07, 0.018, 0.05, -0.035, 0.035, cor, sem=('baixo',), passo=1)
    caixa(CORES, R, -0.03, 0.035, 0.05, 0.075, -0.03, 0.03, escurecer(cor, 0.85), sem=('baixo',), passo=1)
    for a in (-0.045, 0.045):
        for b in (-0.036, 0.036):
            cilindro(CORES, R.p(a, 0.018, b), R.d(0, 0, 1), (0, 1, 0), 0.018, 0.012 * (1 if b > 0 else -1), 6, (0.05, 0.05, 0.05))


def plaquinha_honk(x, z, y):
    """A plaquinha "Honk for Service": o cartão na moldura de madeira, inclinada para trás, com um pé"""
    larg, alt = 0.58, 0.58 / aspecto('honk')
    frente = np.array([0, 0, -1.0])
    direita = np.array([1.0, 0, 0]) * -1         # quem olha da entrada (para +z) tem a direita em -x
    a = math.radians(12)
    cima = unit(np.array([0, 1.0, 0]) * math.cos(a) + np.array([0, 0, 1.0]) * math.sin(a))
    base = np.array([x, y, z])
    nrm = np.cross(direita, cima)
    mold = 0.055
    u0, v0, u1, v1 = uv_da_celula('honk')

    def P(s, t, e=0.0):
        return base + direita * s + cima * t - nrm * e
    quad(QUADROS, P(-larg / 2, mold), P(larg / 2, mold), P(larg / 2, mold + alt), P(-larg / 2, mold + alt), BRANCO, n=nrm,
         uv=[(u0, v0), (u1, v0), (u1, v1), (u0, v1)], div=(2, 2))
    W, H = larg + 2 * mold, alt + 2 * mold
    for (s0, s1, t0, t1) in ((-W / 2, W / 2, 0, mold), (-W / 2, W / 2, H - mold, H), (-W / 2, -larg / 2, mold, H - mold),
                             (larg / 2, W / 2, mold, H - mold)):
        quad(CORES, P(s0, t0, -0.012), P(s1, t0, -0.012), P(s1, t1, -0.012), P(s0, t1, -0.012), (0.66, 0.36, 0.14), n=nrm, div=(1, 1))
    for (s0, s1, t0, t1, nn) in ((-W / 2, W / 2, H, H, cima), (-W / 2, W / 2, 0, 0, -cima), (-W / 2, -W / 2, 0, H, -direita),
                                 (W / 2, W / 2, 0, H, direita)):
        quad(CORES, P(s0, t0, -0.012), P(s1, t1, -0.012), P(s1, t1, 0.03), P(s0, t0, 0.03), (0.52, 0.27, 0.10), n=nn, div=(1, 1))
    quad(CORES, P(-W / 2, 0, 0.03), P(W / 2, 0, 0.03), P(W / 2, H, 0.03), P(-W / 2, H, 0.03), (0.40, 0.22, 0.10), n=-nrm, div=(1, 1))


def ventilador(x, z, y, giro):
    """O ventilador antigo: pé de sino creme, gaiola de arame, pás e fitinhas laranja ao vento"""
    d = np.array([math.cos(giro), 0, math.sin(giro)])           # para onde ele sopra
    lado = np.array([-d[2], 0, d[0]])
    up = np.array([0, 1.0, 0])
    base = np.array([x, y, z])
    torno(CORES, base, up, lado, [(0.17, 0.0), (0.16, 0.025), (0.10, 0.06), (0.05, 0.11), (0.04, 0.13)], 14, CREME_VENTILADOR)
    disco(CORES, base + up * 0.13, up, lado, 0.04, 10, CREME_VENTILADOR)
    pescoco = [base + up * 0.12, base + up * 0.24 - d * 0.03, base + up * 0.32 - d * 0.06]
    tubo(CORES, pescoco, 0.03, CREME_VENTILADOR, lados=8)
    motor = base + up * 0.38 - d * 0.08
    elipsoide(CORES, motor, d * 0.10, up * 0.085, lado * 0.085, CREME_VENTILADOR, nu=10, nv=6)
    centro = motor + d * 0.11
    rg = 0.21
    for k, off in enumerate((0.0, 0.11)):
        c = centro + d * off
        pts = [c + rg * (math.cos(a) * up + math.sin(a) * lado) for a in np.linspace(0, 2 * math.pi, 25)]
        tubo(CORES, pts, 0.006, LATAO, lados=3)
    for a in np.linspace(0, 2 * math.pi, 13)[:-1]:
        rad = math.cos(a) * up + math.sin(a) * lado
        pts = [centro + d * 0.11 + rad * 0.03, centro + d * 0.15 + rad * 0.12, centro + d * 0.11 + rad * rg]
        tubo(CORES, pts, 0.005, LATAO, lados=3)
        pts = [centro - d * 0.02 + rad * 0.04, centro - d * 0.01 + rad * 0.14, centro + rad * rg]
        tubo(CORES, pts, 0.005, LATAO, lados=3)
    disco(CORES, centro + d * 0.16, d, up, 0.035, 10, LATAO)
    for k in range(4):
        a = k * math.pi / 2 + 0.4
        rad = math.cos(a) * up + math.sin(a) * lado
        tang = math.cos(a + math.pi / 2) * up + math.sin(a + math.pi / 2) * lado
        c = centro + d * 0.05 + rad * 0.10
        CORES.face([c - rad * 0.07 - tang * 0.035, c + rad * 0.08 - tang * 0.05 + d * 0.02, c + rad * 0.08 + tang * 0.05 - d * 0.02,
                    c - rad * 0.07 + tang * 0.035], LATAO, n=d)
    # as fitinhas, presas na gaiola da frente, voando para a frente
    for k, a in enumerate((0.35, 1.05, -0.45, -1.2)):
        rad = math.cos(a) * up + math.sin(a) * lado
        p0 = centro + d * 0.12 + rad * (rg * 0.85)
        comp = 0.40 + 0.06 * k
        pts = []
        for t in np.linspace(0, 1, 9):
            p = p0 + d * comp * t + rad * 0.04 * t + up * (0.05 * math.sin(t * 7 + k) * t - 0.03 * t)
            pts.append(p)
        largura = 0.032
        for j in range(len(pts) - 1):
            a0, a1 = pts[j], pts[j + 1]
            w0 = lado * largura * (1 - 0.3 * j / 8)
            tor = math.sin(j * 1.3 + k) * 0.6
            w0 = unit(w0 * math.cos(tor) + up * math.sin(tor) * np.linalg.norm(w0)) * largura
            CORES.face([a0 - w0 / 2, a0 + w0 / 2, a1 + w0 / 2, a1 - w0 / 2], (0.96, 0.42, 0.16), n=up)


def cupula_de_cone(m, boca, eixo, comprimento, raio_boca, raio_topo, lados=16, aba=0.10, por_dentro=LUZ_DA_LAMPADA,
                   por_fora=CUPULA_ACESA):
    """A cúpula em forma de cone de trânsito (aberta embaixo, com a aba da base em volta da boca), acesa por
    dentro: por fora, laranja com um degradê (mais clara perto da boca); por dentro e na boca, a luz"""
    boca = np.asarray(boca, float)
    eixo = unit(eixo)                                   # da boca para a ponta
    e1 = unit(np.cross(eixo, np.array([0.3, 0.2, 1.0])))

    def fora(p):
        # a faixa clara junto da boca (a luz atravessa a borda), laranja vivo no meio, mais escura na ponta
        t = float(np.clip(np.dot(np.asarray(p) - boca, eixo) / comprimento, 0, 1))
        if t < 0.45:
            return mistura((1.0, 0.82, 0.50), (0.97, 0.48, 0.13), (t / 0.45) ** 0.7)
        return mistura((0.97, 0.48, 0.13), escurecer(por_fora, 0.70), (t - 0.45) / 0.55)
    perfil = [(raio_boca + (raio_topo - raio_boca) * t, comprimento * t) for t in (0.0, 0.15, 0.45, 0.75, 1.0)]
    torno(m, boca, eixo, e1, perfil, lados, BRANCO, brilho=fora)
    torno(m, boca + eixo * 0.012, eixo, e1, [(raio_topo * 0.9, comprimento - 0.012), (raio_boca - 0.012, 0.0)], lados, BRANCO,
          brilho=por_dentro)      # o lado de dentro, um pouco para dentro do de fora
    disco(m, boca + eixo * comprimento, eixo, e1, raio_topo, lados, BRANCO, brilho=por_fora)
    # a aba (como a base do cone, mas redonda) em volta da boca
    torno(m, boca, eixo, e1, [(raio_boca + aba, 0.0), (raio_boca, 0.0)], lados, BRANCO, brilho=(1.0, 0.66, 0.30))
    torno(m, boca - eixo * 0.025, eixo, e1, [(raio_boca, 0.0), (raio_boca + aba, 0.0)], lados, BRANCO, brilho=(0.96, 0.60, 0.26))
    torno(m, boca - eixo * 0.025, eixo, e1, [(raio_boca + aba, 0.0), (raio_boca + aba, 0.025)], lados, BRANCO,
          brilho=(1.0, 0.74, 0.40))
    # a lâmpada, um pouco para dentro da boca
    disco(m, boca + eixo * 0.08, -eixo, e1, raio_boca * 0.85, lados, BRANCO, brilho=(1.0, 0.95, 0.80))


def abajur(x, z, y):
    """O abajur de mesa: a cúpula de cone inclinada, a haste cromada e o pé redondo"""
    base = np.array([x, y, z])
    up = np.array([0, 1.0, 0])
    disco(CORES, base + up * 0.025, up, (1, 0, 0), 0.15, 16, CROMO)
    torno(CORES, base, up, (1, 0, 0), [(0.15, 0.0), (0.15, 0.025)], 16, escurecer(CROMO, 0.8))
    topo = base + up * 0.52
    tubo(CORES, [base + up * 0.02, topo], 0.012, CROMO, lados=6)
    eixo = unit(np.array([0.15, 1.0, -0.35]))           # da boca para a ponta: a boca olha para baixo e para a frente
    boca = topo - eixo * 0.06
    cupula_de_cone(CORES, boca, eixo, 0.46, 0.20, 0.03, lados=14, aba=0.07)
    return boca


def papeis(x, z, y):
    for k, (dx, dz, g) in enumerate(((0.0, 0.0, 0.05), (0.06, 0.03, -0.12))):
        R = Ref((x + dx, z + dz), (math.cos(g), math.sin(g)), (-math.sin(g), math.cos(g)), y0=y + 0.002 * (k + 1))
        quad(CORES, R.p(-0.16, 0, -0.11), R.p(0.16, 0, -0.11), R.p(0.16, 0, 0.11), R.p(-0.16, 0, 0.11), PAPEL_BRANCO,
             n=(0, 1, 0), div=(1, 1))
    tubo(CORES, [(x - 0.12, y + 0.008, z + 0.05), (x + 0.10, y + 0.008, z + 0.07)], 0.006, (0.15, 0.20, 0.55), lados=4)


parte('balcão')
montar_balcao()
Y_TAMPO = TOPO_DO_BALCAO
plaquinha_honk(BALCAO_X[1] - 0.80, BALCAO_Z[0] + 0.32, Y_TAMPO)
ventilador(BALCAO_X[1] - 2.05, BALCAO_Z[0] + 0.55, Y_TAMPO, math.radians(195))
BOCA_DO_ABAJUR = abajur(BALCAO_X[1] - 3.35, BALCAO_Z[0] + 0.62, Y_TAMPO)
papeis(BALCAO_X[0] + 1.55, BALCAO_Z[0] + 0.40, Y_TAMPO)
for k, (dx, dz) in enumerate(((0.15, 0.20), (0.25, 0.16), (0.35, 0.22))):
    cone(CORES, BALCAO_X[0] + dx, BALCAO_Z[0] + dz, Y_TAMPO, 0.11, faixa=None, giro=k)


# ------------------------------------------------------------------
# 3e. A estante embutida dos fundos: cones, quadros e o relógio sol-raiado de setas
# ------------------------------------------------------------------
R_ESTANTE = Ref((ESTANTE_X[0], Z_FUNDO), (1, 0), (0, -1))     # a ao longo (+x), b para dentro da sala (-z)
L_ESTANTE = ESTANTE_X[1] - ESTANTE_X[0]
# Cinco seções (a: ao longo, de oeste para leste): as de fora mais estreitas, com quatro nichos; as do meio com
# três; a do centro (atrás da Sally) com um nicho embaixo e o relógio no nicho alto. As alturas são as das
# faces de cima das tábuas (a primeira é o fundo da estante); os nichos têm de 0,8 a 1,2 m e os cones, de 70 a
# 85 % disso
_LARGURAS = [1.45, 1.70, 2.18, 1.70, 1.45]
SECOES = [(sum(_LARGURAS[:k]), sum(_LARGURAS[:k + 1])) for k in range(5)]
PRATELEIRAS = {0: [0.12, 1.00, 1.86, 2.80], 1: [0.12, 1.30, 2.45], 2: [0.12, 1.25], 3: [0.12, 1.30, 2.45],
               4: [0.12, 1.00, 1.86, 2.80]}
MOLDURAS_DOS_QUADROS = [(0.40, 0.22, 0.10), (0.62, 0.36, 0.16), (0.78, 0.60, 0.26), (0.20, 0.12, 0.08), (0.90, 0.88, 0.82)]


def quadro(R, a, h, b, largura, nome, cor_moldura, moldura=0.045, profundidade=0.03, inclina=0.0, oval=False, frente=None):
    """Um quadro (célula do atlas) com moldura, no referencial R (a ao longo, b para a frente: a face olha para +n).
    h é a altura do pé dele; inclina: graus para trás (de pé numa prateleira)"""
    alt = largura / aspecto(nome)
    W, H = largura + 2 * moldura, alt + 2 * moldura
    ang = math.radians(inclina)
    cima = unit(np.array([0, 1.0, 0]) * math.cos(ang) - np.array([R.n[0], 0, R.n[1]]) * math.sin(ang))
    nrm = np.array([R.n[0], 0, R.n[1]]) * math.cos(ang) + np.array([0, 1.0, 0]) * math.sin(ang)
    direita = np.cross(-np.array([R.n[0], 0, R.n[1]]), np.array([0, 1.0, 0]))    # a direita de quem olha o quadro
    base = R.p(a, h, b)
    u0, v0, u1, v1 = uv_da_celula(nome)

    def P(s, t, e=0.0):
        return base + direita * s + cima * t + nrm * e
    if oval:
        cx, cy = 0.0, H / 2
        n_seg = 24
        rx, ry = largura / 2, alt / 2
        pts, uvs = [], []
        for k in range(n_seg):
            t = 2 * math.pi * k / n_seg
            pts.append(P(cx + rx * math.cos(t), cy + ry * math.sin(t), profundidade))
            uvs.append(((u0 + u1) / 2 + (u1 - u0) / 2 * math.cos(t), (v0 + v1) / 2 + (v1 - v0) / 2 * math.sin(t)))
        QUADROS.face(pts, BRANCO, n=nrm, uvs=uvs)
        for k in range(n_seg):
            t0, t1 = 2 * math.pi * k / n_seg, 2 * math.pi * (k + 1) / n_seg
            a0 = P(cx + rx * math.cos(t0), cy + ry * math.sin(t0), profundidade)
            a1 = P(cx + rx * math.cos(t1), cy + ry * math.sin(t1), profundidade)
            b0 = P(cx + (rx + moldura * 1.6) * math.cos(t0), cy + (ry + moldura * 1.6) * math.sin(t0), profundidade + 0.012)
            b1 = P(cx + (rx + moldura * 1.6) * math.cos(t1), cy + (ry + moldura * 1.6) * math.sin(t1), profundidade + 0.012)
            CORES.face([a0, a1, b1, b0], cor_moldura, n=nrm)
            c0 = P(cx + (rx + moldura * 1.6) * math.cos(t0), cy + (ry + moldura * 1.6) * math.sin(t0), 0)
            c1 = P(cx + (rx + moldura * 1.6) * math.cos(t1), cy + (ry + moldura * 1.6) * math.sin(t1), 0)
            CORES.face([b0, b1, c1, c0], escurecer(cor_moldura, 0.8), n=(b0 + b1) / 2 - P(cx, cy))
        return
    quad(QUADROS, P(-largura / 2, moldura, profundidade), P(largura / 2, moldura, profundidade),
         P(largura / 2, moldura + alt, profundidade), P(-largura / 2, moldura + alt, profundidade), BRANCO, n=nrm,
         uv=[(u0, v0), (u1, v0), (u1, v1), (u0, v1)], div=(2, 2))
    for (s0, s1, t0, t1) in ((-W / 2, W / 2, 0, moldura), (-W / 2, W / 2, H - moldura, H), (-W / 2, -largura / 2, moldura, H - moldura),
                             (largura / 2, W / 2, moldura, H - moldura)):
        quad(CORES, P(s0, t0, profundidade + 0.008), P(s1, t0, profundidade + 0.008), P(s1, t1, profundidade + 0.008),
             P(s0, t1, profundidade + 0.008), cor_moldura, n=nrm, div=(1, 1))
    for (s0, s1, t0, t1, nn) in ((-W / 2, W / 2, H, H, cima), (-W / 2, W / 2, 0, 0, -cima), (-W / 2, -W / 2, 0, H, -direita),
                                 (W / 2, W / 2, 0, H, direita)):
        quad(CORES, P(s0, t0, 0), P(s1, t1, 0), P(s1, t1, profundidade + 0.008), P(s0, t0, profundidade + 0.008),
             escurecer(cor_moldura, 0.8), n=nn, div=(1, 1))
    # o filete de dentro da moldura (o passe-partout fino)
    for (s0, s1, t0, t1, nn) in ((-largura / 2, largura / 2, moldura, moldura, cima), (-largura / 2, largura / 2, H - moldura, H - moldura, -cima)):
        quad(CORES, P(s0, t0, profundidade), P(s1, t1, profundidade), P(s1, t1, profundidade + 0.008), P(s0, t0, profundidade + 0.008),
             escurecer(cor_moldura, 0.6), n=nn, div=(1, 1))


def relogio(centro, frente, direita):
    """O relógio sol-raiado: o miolo de madeira, doze varetas de cobre com pontas de seta e os ponteiros"""
    c = np.asarray(centro, float)
    f, d = unit(frente), unit(direita)
    up = np.cross(f, d) * -1 if np.cross(f, d)[1] < 0 else np.cross(f, d)
    up = unit(np.cross(d, f)) if np.dot(np.cross(d, f), [0, 1, 0]) > 0 else unit(np.cross(f, d))
    disco(CORES, c + f * 0.05, f, d, 0.18, 24, (0.45, 0.26, 0.13))
    torno(CORES, c, f, d, [(0.18, 0.0), (0.18, 0.05)], 24, (0.35, 0.20, 0.10))
    disco(CORES, c + f * 0.052, f, d, 0.13, 24, (0.58, 0.36, 0.18))
    for k in range(12):
        a = 2 * math.pi * k / 12
        rad = math.cos(a) * d + math.sin(a) * up
        tg = -math.sin(a) * d + math.cos(a) * up
        comp = 0.78 if k % 2 == 0 else 0.60
        p0 = c + rad * 0.18 + f * 0.03
        p1 = c + rad * comp + f * 0.03
        tubo(CORES, [p0, p1], 0.014, COBRE, lados=4)
        # a ponta de seta (losango achatado) e as penas perto do miolo
        ponta = p1 + rad * 0.12
        CORES.face([p1 - rad * 0.01 + tg * 0.065, ponta, p1 - rad * 0.01 - tg * 0.065, p1 + rad * 0.02], COBRE, n=f)
        CORES.face([p1 - rad * 0.01 + tg * 0.065 - f * 0.012, ponta - f * 0.012, p1 - rad * 0.01 - tg * 0.065 - f * 0.012,
                    p1 + rad * 0.02 - f * 0.012], escurecer(COBRE, 0.7), n=-f)
        for s in (1, -1):
            q = c + rad * 0.25 + f * 0.03
            CORES.face([q, q + rad * 0.09, q + rad * 0.12 + tg * s * 0.045, q + rad * 0.03 + tg * s * 0.045], (0.80, 0.52, 0.26), n=f)
    for (ang, comp, larg) in ((math.radians(62), 0.44, 0.036), (math.radians(150), 0.60, 0.026)):
        rad = math.cos(ang) * d + math.sin(ang) * up
        tg = -math.sin(ang) * d + math.cos(ang) * up
        p = c + f * 0.07
        CORES.face([p - tg * larg / 2 - rad * 0.06, p + rad * comp, p + tg * larg / 2 - rad * 0.06], (0.10, 0.07, 0.05), n=f)


def montar_estante():
    R = R_ESTANTE
    L, Pf, Ht = L_ESTANTE, ESTANTE_FUNDO, ESTANTE_ALTURA
    # o plinto, o tampo e a cornija (que passa só um pouco dos lados: os caixilhos das janelas estão ali)
    caixa(CORES, R, 0.0, L, 0.0, 0.12, 0.04, Pf - 0.04, escurecer(ESTANTE, 0.86), sem=('baixo', 'tras'))
    caixa(CORES, R, -0.03, L + 0.03, Ht - 0.12, Ht, 0.0, Pf + 0.05, ESTANTE, sem=('tras',))
    caixa(CORES, R, -0.03, L + 0.03, Ht - 0.02, Ht + 0.03, 0.0, Pf + 0.08, escurecer(ESTANTE, 0.95), sem=('tras',))
    # os fundos lilases dos nichos
    for (a0, a1) in SECOES:
        quad(CORES, R.p(a0, 0.12, 0.02), R.p(a1, 0.12, 0.02), R.p(a1, Ht - 0.12, 0.02), R.p(a0, Ht - 0.12, 0.02), PAREDE_FUNDO_ESTANTE,
             n=R.d(0, 0, 1), passo=PASSO_DA_PAREDE, suave=True)
    # as colunas
    for a in [SECOES[0][0]] + [s1 for _, s1 in SECOES]:
        caixa(CORES, R, a - 0.07, a + 0.07, 0.0, Ht - 0.12, 0.02, Pf, ESTANTE, sem=('tras',), passo=0.5)
    # as prateleiras (com a borda da frente um pouco mais alta)
    for k, (a0, a1) in enumerate(SECOES):
        for h in PRATELEIRAS[k][1:]:
            caixa(CORES, R, a0 + 0.07, a1 - 0.07, h - 0.07, h, 0.02, Pf - 0.01, ESTANTE, sem=('tras',), passo=0.5)
            caixa(CORES, R, a0 + 0.07, a1 - 0.07, h - 0.09, h + 0.01, Pf - 0.04, Pf + 0.01, escurecer(ESTANTE, 0.97), sem=('tras',), passo=0.5)
    # o rodapé da estante, um pouco recuado e mais escuro
    caixa(CORES, R, 0.0, L, 0.0, 0.10, Pf - 0.06, Pf - 0.04, escurecer(ESTANTE, 0.72), sem=('tras', 'baixo'), passo=0.5)
    # o que tem nela (a: ao longo; h: a altura da tábua; b: para a frente)
    rng = np.random.default_rng(5)
    x = lambda a: R.p(a, 0, 0)[0]
    z = lambda b: R.p(0, 0, b)[2]
    y = lambda h: PISO_Y + h

    def c(a, h, H, faixa, giro=0.3, b=0.38, cor=CONE):
        cone(CORES, x(a), z(b), y(h), H, faixa, giro=giro, cor=cor)
    s0, s1, s2, s3, s4 = (a for a, _ in SECOES)
    # seção 0 (a de oeste, à esquerda de quem olha a estante)
    c(s0 + 0.45, 0.12, 0.66, 'branca', 0.3)
    quadro(R, s0 + 1.05, 0.12, 0.10, 0.44, 'mesas', MOLDURAS_DOS_QUADROS[0], inclina=10)
    quadro(R, s0 + 0.42, 1.00, 0.06, 0.48, 'cone_noite', MOLDURAS_DOS_QUADROS[1], inclina=8)
    c(s0 + 1.06, 1.00, 0.62, 'preta', 0.6)
    c(s0 + 0.45, 1.86, 0.70, 'duas', 0.9)
    piramide_de_cones(CORES, x(s0 + 1.06), z(0.40), y(1.86), 0.20, base=3, giro=0.0)
    c(s0 + 0.62, 2.80, 0.76, 'branca', 0.2)
    c(s0 + 1.15, 2.80, 0.32, None, 0.7)
    # seção 1
    c(s1 + 0.45, 0.12, 0.92, 'preta', 0.1)
    quadro(R, s1 + 1.20, 0.12, 0.12, 0.46, 'retrato_mesa', (0.82, 0.64, 0.28), inclina=9, oval=True)
    quadro(R, s1 + 0.50, 1.30, 0.08, 0.54, 'lua', MOLDURAS_DOS_QUADROS[2], inclina=9)
    c(s1 + 1.25, 1.30, 0.86, 'branca', 0.5)
    c(s1 + 0.55, 2.45, 0.96, 'branca', 0.2)
    piramide_de_cones(CORES, x(s1 + 1.25), z(0.40), y(2.45), 0.24, base=4, giro=0.0)
    # seção 2 (a do centro, atrás da Sally): cones embaixo; em cima, o relógio no fundo e um quadro pendurado
    c(s2 + 0.42, 0.12, 0.95, 'branca', 0.5)
    quadro(R, s2 + 1.09, 0.12, 0.08, 0.56, 'retrato_cone', MOLDURAS_DOS_QUADROS[0], inclina=8)
    c(s2 + 1.76, 0.12, 0.92, 'duas', 0.2)
    piramide_de_cones(CORES, x(s2 + 0.42), z(0.40), y(1.25), 0.17, base=3, giro=0.0)
    quadro(R, s2 + 1.12, 1.25, 0.06, 0.34, 'cartaz66', MOLDURAS_DOS_QUADROS[3], inclina=8)
    c(s2 + 1.80, 1.25, 0.56, 'preta', 0.4)
    relogio(R.p((s2 + SECOES[2][1]) / 2, 2.80, 0.03), R.d(0, 0, 1), R.d(1, 0, 0))
    # seção 3
    quadro(R, s3 + 0.48, 0.12, 0.08, 0.50, 'placa', MOLDURAS_DOS_QUADROS[1], inclina=8)
    c(s3 + 1.22, 0.12, 0.90, 'branca', 0.6)
    piramide_de_cones(CORES, x(s3 + 0.52), z(0.40), y(1.30), 0.26, base=4, giro=0.0)
    c(s3 + 1.28, 1.30, 0.88, 'preta', 0.3)
    quadro(R, s3 + 0.52, 2.45, 0.06, 0.50, 'mater', MOLDURAS_DOS_QUADROS[3], inclina=8)
    c(s3 + 1.25, 2.45, 0.94, 'duas', 0.8)
    # seção 4 (a de leste)
    c(s4 + 0.42, 0.12, 0.64, 'branca', 0.7)
    c(s4 + 1.02, 0.12, 0.46, None, 0.1)
    quadro(R, s4 + 0.48, 1.00, 0.06, 0.46, 'sally', MOLDURAS_DOS_QUADROS[1], inclina=8)
    c(s4 + 1.08, 1.00, 0.58, 'branca', 0.4)
    c(s4 + 0.72, 1.86, 0.68, 'branca', 0.1)
    quadro(R, s4 + 0.45, 2.80, 0.06, 0.44, 'cactos', MOLDURAS_DOS_QUADROS[3], inclina=8)
    c(s4 + 1.08, 2.80, 0.74, 'preta', 0.9)
    # em cima: a fileira de cones escuros quase no teto (a cornija fica uns 0,4 m abaixo dele)
    topo = Ht + 0.03
    for k, a in enumerate(np.linspace(0.30, L - 0.30, 14)):
        folga = y_teto(z(0.36)) - (PISO_Y + topo) - 0.03
        H = min(folga, 0.30 + 0.08 * rng.uniform())
        cone(CORES, x(a), z(0.36), y(topo), H, None if k % 3 else 'branca', giro=rng.uniform(0, 1), cor=CONE_ESCURO, lados=10)
    obstaculo_caixa('estante', ESTANTE_X[0] - 0.05, ESTANTE_X[1] + 0.05, Z_FUNDO - ESTANTE_FUNDO - 0.08, Z_FUNDO,
                    altura=ESTANTE_ALTURA)


parte('estante')
montar_estante()


# ------------------------------------------------------------------
# 3f. Cactos, vasos e suportes de ferro batido; heras penduradas em vasos-cone
# ------------------------------------------------------------------
def secao_de_costelas(costelas, fundo=0.85):
    """A seção em estrela de um cacto: dois pontos por costela (a quina, raio 1, e o vale, raio 'fundo')"""
    out = []
    for j in range(costelas * 2):
        a = 2 * math.pi * j / (costelas * 2)
        out.append((a, 1.0 if j % 2 == 0 else fundo))
    return out


ESPINHO = (0.93, 0.90, 0.70)       # os espinhos (pontinhos claros nas quinas)


def espinhos(aneis, centros, costelas, rng, de=0, ate=None, tam=0.016):
    """Pontinhos claros (losangos pequenos virados para fora) nas quinas das costelas, entre os anéis, em
    quinas alternadas de um anel para o outro"""
    ate = len(aneis) - 1 if ate is None else ate
    for k in range(de, ate):
        for j in range(0, costelas * 2, 2):
            if (j // 2 + k) % 2:
                continue
            for t in (0.3, 0.75):
                p = (1 - t) * aneis[k][j] + t * aneis[k + 1][j]
                c = (1 - t) * centros[k] + t * centros[k + 1]
                fora = unit(p - c)
                ao_longo = unit(centros[k + 1] - centros[k])
                lado = np.cross(ao_longo, fora)
                q = p + fora * 0.006
                s = tam * rng.uniform(0.8, 1.2)
                CORES.face([q + ao_longo * s, q + lado * s * 0.7, q - ao_longo * s, q - lado * s * 0.7], ESPINHO, n=fora)


def coluna_de_cacto(base, altura, raio, costelas=10, cor=VERDE, escuro=VERDE_ESCURO, caminho=None, rng=None):
    """Um tronco (ou braço) de cacto: a seção em estrela (costelas), a ponta arredondada em três anéis que
    fecham num ponto (sem tampa chata) e os espinhos nas quinas. caminho: a linha do eixo até onde começa a
    ponta (senão, reto para cima até altura - raio); os anéis do trecho reto ficam a cada ~0,4 m"""
    rng = rng or np.random.default_rng(3)
    base = np.asarray(base, float)
    if caminho is None:
        reto = max(altura - raio, 0.05)
        n = max(2, int(math.ceil(reto / 0.40)))
        caminho = [base + np.array([0, reto * t, 0]) for t in np.linspace(0, 1, n + 1)]
    caminho = [np.asarray(p, float) for p in caminho]
    pts = list(caminho)
    raios = [raio] * len(pts)
    raios[0] = raio * 0.92                     # um pouco mais fino no pé
    t_fim = unit(caminho[-1] - caminho[-2])
    for h in (0.50, 0.85, 1.0):                # a ponta: meia esfera (um pouco achatada) até um ponto
        pts.append(caminho[-1] + t_fim * raio * h * 0.9)
        raios.append(raio * math.sqrt(max(0.0, 1 - h * h)))
    n_reto = len(caminho)

    def cor_f(k, j, p):
        claro = 0.86 + 0.18 * min(1.0, k / max(1, n_reto - 1))      # mais escuro embaixo
        if j % 2 == 0:
            return escurecer(mistura(cor, (0.62, 0.74, 0.48), 0.18), claro)    # a quina, mais clara
        return escurecer(escuro, claro)
    aneis = tubo(CORES, pts, raio, cor_f, secao=secao_de_costelas(costelas), raios=raios)
    if aneis:
        espinhos(aneis, pts, costelas, rng, de=0, ate=n_reto, tam=0.012 + 0.03 * raio)


def braco(base_no_tronco, direcao, sai, sobe, raio, costelas=6, rng=None):
    """Um braço de saguaro: sai do tronco na horizontal, faz a curva para cima (um quarto de círculo) e sobe"""
    d = unit(np.array([direcao[0], 0, direcao[1]]))
    up = np.array([0, 1.0, 0])
    b = np.asarray(base_no_tronco, float)
    re = sai * 0.55                                   # o raio da curva do cotovelo
    pts = [b - d * raio * 0.5, b + d * (sai - re) * 0.5, b + d * (sai - re)]
    centro = b + d * (sai - re) + up * re
    for t in np.linspace(0, math.pi / 2, 4)[1:]:
        pts.append(centro + d * re * math.sin(t) - up * re * math.cos(t))
    topo = pts[-1]
    pts.append(topo + up * sobe * 0.5)
    pts.append(topo + up * sobe)
    coluna_de_cacto(b, 0, raio, costelas, caminho=pts, rng=rng)


def saguaro(base, altura, raio=0.15, bracos=((0.0, 0.40, 0.30, 0.40), (math.pi, 0.55, 0.26, 0.28)), giro=0.0, rng=None):
    base = np.asarray(base, float)
    coluna_de_cacto(base, altura, raio, 10, rng=rng)
    for ang, h, sai, sobe in bracos:
        d = (math.cos(ang + giro), math.sin(ang + giro))
        braco(base + np.array([0, h * altura, 0]), d, sai, sobe * altura, raio * 0.66, rng=rng)


def mandacaru(base, alturas, raio=0.10, rng=None):
    """Várias colunas que saem juntas do vaso, as de fora um pouco abertas"""
    base = np.asarray(base, float)
    for k, h in enumerate(alturas):
        a = 2 * math.pi * k / len(alturas) + 0.3
        rad = np.array([math.cos(a), 0, math.sin(a)])
        off = rad * (0.0 if k == 0 else raio * 1.45)
        r = raio * (1.0 if k == 0 else 0.86)
        reto = max(h - r, 0.1)
        n = max(2, int(math.ceil(reto / 0.40)))
        pts = []
        for t in np.linspace(0, 1, n + 1):
            abre = 0.0 if k == 0 else 0.10 * math.sin(t * math.pi / 2)     # as de fora se abrem e depois sobem
            pts.append(base + off + rad * abre * reto + np.array([0, reto * t, 0]))
        coluna_de_cacto(base + off, h, r, 7, caminho=pts, rng=rng)


def palma(base, rng, n_niveis=2, escala=1.0):
    """Palma (opúntia): duas ou três raquetes ovais achatadas saindo do vaso, abertas em leque, e outras nascendo
    na borda de cima delas, cada vez mais inclinadas para os lados; as aréolas são pontinhos claros"""
    base = np.asarray(base, float)
    Y = np.array([0, 1.0, 0])

    def raquete(c, giro, de_lado, para_frente, w, h, nivel):
        d = np.array([math.cos(giro), 0, math.sin(giro)])          # a largura da raquete
        frente = np.cross(d, Y)                                    # a espessura
        up = unit(Y * math.cos(de_lado) + d * math.sin(de_lado) + frente * math.sin(para_frente))
        espessura = unit(np.cross(d, up)) * 0.045 * escala
        d = unit(np.cross(up, espessura))
        cen = c + up * h
        cor = mistura(VERDE_PALMA, (0.42, 0.52, 0.32), rng.uniform(0, 0.5))
        elipsoide(CORES, cen, d * w, up * h, espessura, cor, nu=8, nv=5)
        for _ in range(5):
            s, t = rng.uniform(-0.6, 0.6), rng.uniform(-0.6, 0.6)
            p = cen + d * w * s + up * h * t
            for e in (espessura, -espessura):
                q = p + e * math.sqrt(max(0.05, 1 - s * s - t * t)) * 1.02
                CORES.face([q + d * 0.009, q + up * 0.009, q - d * 0.009, q - up * 0.009], ESPINHO, n=e)
        if nivel < n_niveis:
            for k in range(rng.integers(1, 3)):
                lado = (1 if (k + nivel) % 2 == 0 else -1) * rng.uniform(0.45, 0.9)
                topo = cen + up * h * 0.72 + d * w * lado * 0.70
                raquete(topo - up * 0.02, giro + rng.uniform(-0.6, 0.6), de_lado * 0.6 + lado * rng.uniform(0.45, 0.80),
                        rng.uniform(-0.3, 0.3), w * rng.uniform(0.80, 0.92), h * rng.uniform(0.80, 0.92), nivel + 1)
    n0 = 3 if escala >= 1.05 else 2
    g0 = rng.uniform(0, math.pi)
    for k in range(n0):
        de_lado = (k - (n0 - 1) / 2) * 0.55 + rng.uniform(-0.1, 0.1)
        raquete(base - Y * 0.04, g0 + k * 1.1 + rng.uniform(-0.25, 0.25), de_lado, rng.uniform(-0.25, 0.25),
                0.19 * escala, 0.25 * escala, 0 if k == n0 // 2 else 1)


def bola(base, raio=0.22, rng=None):
    """O cacto-bola: uma bola achatada de costelas, com a ponta fechada"""
    c = np.asarray(base, float)
    perfil = [(0.55, 0.0), (0.88, 0.12), (1.0, 0.32), (0.97, 0.52), (0.82, 0.72), (0.52, 0.88), (0.0, 0.96)]
    pts = [c + np.array([0, raio * 1.25 * h, 0]) for _, h in perfil]
    raios = [raio * r for r, _ in perfil]
    aneis = tubo(CORES, pts, raio, lambda k, j, p: escurecer(VERDE if j % 2 == 0 else VERDE_ESCURO, 0.88 + 0.03 * k),
                 secao=secao_de_costelas(12, 0.86), raios=raios)
    if aneis:
        espinhos(aneis, pts, 12, rng or np.random.default_rng(4), de=1, ate=5, tam=0.014)


def vaso(base, raio_topo, altura, cor=TERRACOTA, borda=True, lados=12):
    base = np.asarray(base, float)
    up = (0, 1, 0)
    r0 = raio_topo * 0.68
    perfil = [(r0, 0.0), (raio_topo * 0.96, altura * 0.86), (raio_topo, altura * 0.86)]
    if borda:
        perfil += [(raio_topo * 1.06, altura * 0.88), (raio_topo * 1.06, altura), (raio_topo * 0.94, altura),
                   (raio_topo * 0.94, altura * 0.93)]
    torno(CORES, base, up, (1, 0, 0), perfil, lados, lambda p: mistura(escurecer(cor, 0.82), cor, float(np.clip((p[1] - base[1]) / altura, 0, 1))))
    disco(CORES, base + np.array([0, altura * 0.93, 0]), up, (1, 0, 0), raio_topo * 0.94, lados, TERRA)
    disco(CORES, base, (0, -1, 0), (1, 0, 0), r0, lados, escurecer(cor, 0.7))
    return base + np.array([0, altura * 0.93, 0])


def voluta(centro, plano_u, plano_v, r0, voltas=1.15, n=16, r1=None):
    """Os pontos de uma espiral (a voluta do ferro batido) no plano (plano_u, plano_v)"""
    r1 = r0 * 0.25 if r1 is None else r1
    pts = []
    for t in np.linspace(0, 1, n):
        a = 2 * math.pi * voltas * t
        r = r0 + (r1 - r0) * t
        pts.append(np.asarray(centro) + r * (math.cos(a) * np.asarray(plano_u) + math.sin(a) * np.asarray(plano_v)))
    return pts


def suporte_de_ferro(x, z, altura, raio_anel, pernas=3, giro=0.0, altura_da_planta=0.0):
    """O suporte de ferro batido: o anel que segura o vaso, um anel no meio, as pernas abertas com as volutas
    no pé e as orelhas em cima (barras de seção triangular, finas)"""
    up = np.array([0, 1.0, 0])
    c = np.array([x, PISO_Y, z])
    barra = 0.016
    anel = [c + up * altura + raio_anel * np.array([math.cos(a), 0, math.sin(a)]) for a in np.linspace(0, 2 * math.pi, 13)]
    tubo(CORES, anel, barra, FERRO, lados=3)
    anel2 = [c + up * altura * 0.42 + raio_anel * 0.85 * np.array([math.cos(a), 0, math.sin(a)]) for a in np.linspace(0, 2 * math.pi, 11)]
    tubo(CORES, anel2, barra * 0.8, FERRO, lados=3)
    for k in range(pernas):
        a = giro + 2 * math.pi * k / pernas
        rad = np.array([math.cos(a), 0, math.sin(a)])
        topo = c + up * altura + rad * raio_anel
        meio = c + up * altura * 0.42 + rad * raio_anel * 0.85
        pe = c + up * 0.14 + rad * (raio_anel * 1.25)
        tubo(CORES, [topo + up * 0.05, topo, meio, pe], barra, FERRO, lados=3)
        # a voluta do pé (para fora) e a orelha de cima (para fora e para cima)
        tubo(CORES, voluta(pe + rad * 0.07, -rad, -up, 0.07, voltas=0.9, n=7, r1=0.02)[::-1], barra, FERRO, lados=3)
        o = topo + up * 0.05
        tubo(CORES, [o] + voluta(o + rad * 0.06 + up * 0.03, -rad, up * -1, 0.065, voltas=1.0, n=7, r1=0.02)[1:], barra * 0.85, FERRO, lados=3)
    # a colisão vai até o alto da planta (a câmera não a atravessa)
    obstaculo_redondo('suporte', x, z, raio_anel * 1.25 + 0.12, altura=altura + altura_da_planta)


ALTURA_DA_PLANTA = {'saguaro': lambda kw: kw.get('altura', 1.6), 'mandacaru': lambda kw: max(kw.get('alturas', (1.25,))),
                    'palma': lambda kw: 0.9 * kw.get('escala', 1.0), 'bola': lambda kw: 0.3}


def planta_no_suporte(x, z, tipo, rng, altura_suporte=0.62, raio_vaso=0.27, alt_vaso=0.42, **kw):
    suporte_de_ferro(x, z, altura_suporte, raio_vaso * 0.78, pernas=3, giro=rng.uniform(0, 2),
                     altura_da_planta=alt_vaso * 0.6 + ALTURA_DA_PLANTA[tipo](kw))
    topo = vaso((x, PISO_Y + altura_suporte - alt_vaso * 0.35, z), raio_vaso, alt_vaso)
    plantar(topo, tipo, rng, **kw)


def plantar(topo, tipo, rng, **kw):
    if tipo == 'saguaro':
        saguaro(topo, kw.get('altura', 1.6), raio=kw.get('raio', 0.13), giro=rng.uniform(0, math.pi), rng=rng)
    elif tipo == 'mandacaru':
        mandacaru(topo, kw.get('alturas', (1.25, 0.85, 0.65, 0.5)), raio=kw.get('raio', 0.085), rng=rng)
    elif tipo == 'palma':
        palma(topo, rng, escala=kw.get('escala', 1.0), n_niveis=kw.get('niveis', 2))
    elif tipo == 'bola':
        bola(np.asarray(topo) - np.array([0, 0.03, 0]), raio=kw.get('raio', 0.22), rng=rng)


# A câmera do jogo anda em y ~3,9 aqui dentro: perto das paredes, o que pende do teto não desce abaixo disto
Y_MINIMO_DO_QUE_PENDE = 4.95


def hera_pendurada(x, z, rng, comprimento=1.7, n_ramos=8):
    """Um vaso-cone (de cabeça para baixo) pendurado do teto por três fios, com a hera caindo pelas bordas; a
    ponta mais baixa dos ramos fica em Y_MINIMO_DO_QUE_PENDE ou acima"""
    up = np.array([0, 1.0, 0])
    teto = y_teto(z)
    y_gancho = teto - 0.42
    boca = np.array([x, y_gancho, z])
    altura = 0.48
    r = 0.30
    comprimento = max(0.35, min(comprimento, y_gancho + 0.07 - Y_MINIMO_DO_QUE_PENDE))
    perfil = [(0.04, -altura), (r, 0.0)]
    torno(CORES, boca, up, (1, 0, 0), perfil, 12, lambda p: mistura(escurecer(CONE, 0.75), CONE, float(np.clip((p[1] - y_gancho + altura) / altura, 0, 1))))
    torno(CORES, boca, up, (1, 0, 0), [(r, 0.0), (r * 1.08, 0.04), (r * 1.08, 0.08), (r * 0.98, 0.08)], 12, CONE_BRANCO)
    disco(CORES, boca + up * 0.06, up, (1, 0, 0), r * 0.98, 12, TERRA)
    for k in range(3):
        a = 2 * math.pi * k / 3
        p = boca + up * 0.08 + r * np.array([math.cos(a), 0, math.sin(a)])
        tubo(CORES, [p, np.array([x, teto - 0.02, z])], 0.004, (0.25, 0.22, 0.20), lados=3)
    for k in range(n_ramos):
        a = 2 * math.pi * k / n_ramos + rng.uniform(-0.2, 0.2)
        rad = np.array([math.cos(a), 0, math.sin(a)])
        p = boca + up * 0.07 + rad * r * 0.95
        L = comprimento * rng.uniform(0.45, 1.0)
        pts = [p]
        n = max(3, int(L / 0.11))
        for j in range(1, n + 1):
            t = j / n
            pts.append(p + rad * (0.10 * math.sin(t * 2.2) + 0.05 * t) + up * (0.05 - L * t)
                       + np.array([math.sin(t * 5 + k), 0, math.cos(t * 4 + k)]) * 0.04 * t)
        tubo(CORES, pts, 0.005, (0.22, 0.30, 0.16), lados=3)
        for j in range(1, len(pts)):
            q = pts[j]
            for lado in (1, -1):
                folha(q + np.cross(rad, up) * lado * 0.03 + up * 0.02 * lado, rad, rng, tam=0.10 * rng.uniform(0.75, 1.2))


def folha(p, fora, rng, tam=0.08):
    """Uma folha de hera (três pontas), virada mais ou menos para fora"""
    up = np.array([0, 1.0, 0])
    n = unit(np.asarray(fora) + rng.normal(size=3) * 0.5)
    a = unit(np.cross(n, up) + rng.normal(size=3) * 0.2)
    b = unit(np.cross(a, n))
    pts = []
    for ang, r in ((90, 1.0), (150, 0.55), (195, 0.85), (270, 0.3), (345, 0.85), (30, 0.55)):
        t = math.radians(ang)
        pts.append(np.asarray(p) + tam * r * (math.cos(t) * a + math.sin(t) * b) - b * tam * 0.3)
    cor = mistura(VERDE_HERA, (0.40, 0.55, 0.36), rng.uniform(0, 0.6))
    CORES.face(pts, cor, n=n)


def canteiro(x, z, comprimento, largura, plantas, rng):
    """Um canteiro baixo de madeira (ao longo de z) com cactos e pedras"""
    R = Ref((x, z), (0, 1), (1, 0))
    h = 0.48
    caixa(CORES, R, -comprimento / 2, comprimento / 2, 0.0, h, -largura / 2, largura / 2, MADEIRA, sem=('baixo', 'cima'), passo=0.5)
    for (a0, a1, b0, b1) in ((-comprimento / 2, comprimento / 2, -largura / 2, -largura / 2 + 0.08),
                             (-comprimento / 2, comprimento / 2, largura / 2 - 0.08, largura / 2),
                             (-comprimento / 2, -comprimento / 2 + 0.08, -largura / 2, largura / 2),
                             (comprimento / 2 - 0.08, comprimento / 2, -largura / 2, largura / 2)):
        caixa(CORES, R, a0, a1, h, h + 0.04, b0, b1, MADEIRA_ESCURA, sem=('baixo',), passo=0.5)
    quad(CORES, R.p(-comprimento / 2 + 0.08, h - 0.04, -largura / 2 + 0.08), R.p(comprimento / 2 - 0.08, h - 0.04, -largura / 2 + 0.08),
         R.p(comprimento / 2 - 0.08, h - 0.04, largura / 2 - 0.08), R.p(-comprimento / 2 + 0.08, h - 0.04, largura / 2 - 0.08),
         (0.74, 0.58, 0.40), n=(0, 1, 0), passo=0.3)
    for _ in range(9):
        a, b = rng.uniform(-comprimento / 2 + 0.2, comprimento / 2 - 0.2), rng.uniform(-largura / 2 + 0.2, largura / 2 - 0.2)
        r = rng.uniform(0.06, 0.13)
        c = R.p(a, h - 0.04, b)
        elipsoide(CORES, c, (r, 0, 0), (0, r * 0.6, 0), (0, 0, r * 0.8), mistura((0.62, 0.42, 0.32), (0.80, 0.66, 0.52), rng.uniform()),
                  nu=6, nv=3)
    alto = 0.0
    for (a, b, tipo, kw) in plantas:
        plantar(R.p(a, h - 0.04, b), tipo, rng, **kw)
        alto = max(alto, ALTURA_DA_PLANTA[tipo](kw))
    pts = [R.p(sa * comprimento / 2, 0, sb * largura / 2)[[0, 2]] for sa, sb in ((-1, -1), (1, -1), (1, 1), (-1, 1))]
    OBSTACULOS.append(('canteiro', [tuple(p) for p in pts], max(ALTURA_DOS_MOVEIS, h + alto)))


RNG = np.random.default_rng(12)
parte('plantas')
# A vitrine de leste (a da esquerda de quem olha da praça): quatro cactos em suportes, o saguaro alto no meio
planta_no_suporte(703.30, 181.65, 'mandacaru', RNG, altura_suporte=0.62, alturas=(1.30, 0.95, 0.72, 0.52))
planta_no_suporte(704.95, 181.50, 'saguaro', RNG, altura_suporte=0.52, altura=2.00, raio=0.15)
planta_no_suporte(706.60, 181.70, 'palma', RNG, altura_suporte=0.72, escala=1.15, niveis=3)
planta_no_suporte(708.25, 181.55, 'saguaro', RNG, altura_suporte=0.62, altura=1.40, raio=0.12)
# a parede de oeste: perto do cartaz e embaixo da janela dos fundos
planta_no_suporte(688.05, 188.55, 'palma', RNG, altura_suporte=0.72, escala=1.05)
planta_no_suporte(688.05, 194.55, 'saguaro', RNG, altura_suporte=0.50, altura=1.75)
planta_no_suporte(688.25, 197.35, 'bola', RNG, altura_suporte=0.80, raio_vaso=0.24, alt_vaso=0.32)
# a parede de leste, na frente das prateleiras baixas das janelas
planta_no_suporte(708.15, 183.75, 'mandacaru', RNG, altura_suporte=0.66, alturas=(1.15, 0.85, 0.6))
planta_no_suporte(708.15, 185.75, 'saguaro', RNG, altura_suporte=0.55, altura=1.50, raio=0.12)
planta_no_suporte(708.20, 196.95, 'bola', RNG, altura_suporte=0.80, raio_vaso=0.22, alt_vaso=0.32)
planta_no_suporte(708.20, 198.75, 'palma', RNG, altura_suporte=0.70, escala=0.95)
# os fundos, na frente das prateleiras baixas das janelas
planta_no_suporte(704.05, 202.45, 'palma', RNG, altura_suporte=0.70, escala=1.1, niveis=3)
planta_no_suporte(692.70, 202.55, 'saguaro', RNG, altura_suporte=0.55, altura=1.55, raio=0.13)
# vasos grandes no chão
for (x, z, tipo, kw) in ((689.20, 185.25, 'mandacaru', dict(alturas=(1.9, 1.4, 1.1, 0.8), raio=0.12)),
                         (707.85, 195.05, 'saguaro', dict(altura=2.3, raio=0.17))):
    topo = vaso((x, PISO_Y, z), 0.42, 0.62, lados=14)
    plantar(topo, tipo, RNG, **kw)
    obstaculo_redondo('vaso', x, z, 0.55, altura=0.6 + ALTURA_DA_PLANTA[tipo](kw))
# dois canteiros baixos de madeira com cactos e pedras, um de cada lado do corredor
canteiro(692.10, 191.20, 3.3, 1.15, [(-1.05, 0.0, 'saguaro', dict(altura=1.55, raio=0.13)), (0.0, 0.1, 'bola', {}),
                                    (1.00, -0.05, 'mandacaru', dict(alturas=(1.05, 0.75, 0.55), raio=0.08))], RNG)
canteiro(703.60, 187.60, 3.0, 1.10, [(-0.95, 0.05, 'mandacaru', dict(alturas=(1.2, 0.9, 0.6), raio=0.085)),
                                    (0.10, -0.05, 'palma', dict(escala=0.85)), (1.00, 0.05, 'bola', {})], RNG)
# heras penduradas (perto das paredes, curtas o bastante para a câmera passar embaixo delas)
parte('heras')
for (x, z, comp) in ((689.70, 186.40, 1.25), (707.70, 186.20, 1.30), (690.60, 200.60, 1.0),
                     (706.70, 200.90, 1.05), (693.10, 182.30, 1.2), (703.60, 182.40, 1.1)):
    hera_pendurada(x, z, RNG, comprimento=comp, n_ramos=9)
# a hera que cai por cima da estante, na ponta de leste
for k in range(7):
    p = np.array([ESTANTE_X[1] - 0.25 - k * 0.22, PISO_Y + ESTANTE_ALTURA + 0.04, Z_FUNDO - 0.55])
    pts = [p]
    for j in range(1, 7):
        t = j / 6
        pts.append(p + np.array([0.05 * math.sin(t * 3 + k), -0.9 * t * (0.6 + 0.4 * (k % 3) / 2), -0.12 * t]))
    tubo(CORES, pts, 0.005, (0.22, 0.30, 0.16), lados=3)
    for q in pts[1:]:
        folha(q, (0, 0, -1), RNG, tam=0.07)


# ------------------------------------------------------------------
# 3g. Pilhas de cones, os cones da vitrine de leste, o letreiro OFFICE
# ------------------------------------------------------------------
def ref_da_parede(W, u0):
    """O referencial de uma parede em pé (oeste, leste ou fundos): a ao longo dela (u - u0), b para dentro da sala"""
    if W is P_OESTE:
        return Ref((X_OESTE, u0), (0, 1), (1, 0))
    if W is P_LESTE:
        return Ref((X_LESTE, u0), (0, 1), (-1, 0))
    return Ref((u0, Z_FUNDO), (1, 0), (0, -1))


def prateleira_baixa(W, u0, u1, rng, altura=0.92, fundo=0.46):
    """Uma prateleira baixa (creme, como a estante) encostada na parede W embaixo de uma janela, de u0 a u1: o
    tampo, uma tábua no meio, os lados e o rodapé; cones e pirâmides de cones em cima e dentro"""
    R = ref_da_parede(W, u0)
    L = u1 - u0
    caixa(CORES, R, 0.0, L, altura - 0.05, altura, 0.0, fundo, ESTANTE, sem=('tras',), passo=0.5)
    caixa(CORES, R, 0.04, L - 0.04, 0.44, 0.48, 0.02, fundo - 0.03, ESTANTE, sem=('tras',), passo=0.5)
    for a0 in (0.0, L - 0.05):
        caixa(CORES, R, a0, a0 + 0.05, 0.0, altura - 0.05, 0.0, fundo, ESTANTE, sem=('tras', 'baixo'), passo=0.5)
    caixa(CORES, R, 0.05, L - 0.05, 0.0, 0.10, fundo - 0.06, fundo - 0.03, escurecer(ESTANTE, 0.72), sem=('tras', 'baixo'), passo=0.5)
    quad(CORES, R.p(0.05, 0.10, 0.02), R.p(L - 0.05, 0.10, 0.02), R.p(L - 0.05, altura - 0.05, 0.02), R.p(0.05, altura - 0.05, 0.02),
         PAREDE_FUNDO_ESTANTE, n=R.d(0, 0, 1), passo=PASSO_DA_PAREDE, suave=True)
    quad(CORES, R.p(0.05, 0.10, 0.02), R.p(L - 0.05, 0.10, 0.02), R.p(L - 0.05, 0.10, fundo - 0.06), R.p(0.05, 0.10, fundo - 0.06),
         escurecer(ESTANTE, 0.9), n=(0, 1, 0), passo=0.5)
    # em cima: cones de tamanhos variados e uma pirâmide; dentro: cones pequenos nas duas tábuas
    a = 0.22
    while a < L - 0.2:
        if rng.uniform() < 0.22 and a < L - 0.55:
            p = R.p(a + 0.20, altura, fundo * 0.5)
            piramide_de_cones(CORES, p[0], p[2], p[1], 0.17, base=3, giro=math.atan2(R.u[1], R.u[0]))
            a += 0.62
            continue
        H = rng.uniform(0.28, 0.58)
        p = R.p(a, altura, fundo * 0.5 + rng.uniform(-0.05, 0.05))
        cone(CORES, p[0], p[2], p[1], H, ['branca', 'preta', None, 'duas'][rng.integers(0, 4)], giro=rng.uniform(0, 1))
        a += 0.6 * H * 0.6 + rng.uniform(0.10, 0.22)
    for h0, h1 in ((0.48, altura - 0.05),):
        a = 0.30
        while a < L - 0.25:
            H = (h1 - h0) * rng.uniform(0.62, 0.82)
            p = R.p(a, h0, fundo * 0.45)
            cone(CORES, p[0], p[2], p[1], H, 'branca' if rng.uniform() < 0.5 else None, giro=rng.uniform(0, 1), lados=7)
            a += rng.uniform(0.55, 0.85)
    q0, q1 = R.p(0, 0, 0), R.p(L, 0, fundo + 0.05)
    obstaculo_caixa('prateleira_baixa', min(q0[0], q1[0]), max(q0[0], q1[0]), min(q0[2], q1[2]), max(q0[2], q1[2]))


def canto_de_cones(espelhar, rng):
    """O canto dos fundos (o de oeste, ou o de leste espelhado): pilhas altas de cones, um cone grande e um
    estrado de madeira com uma pirâmide de cones"""
    X = (lambda x: x) if not espelhar else (lambda x: X_OESTE + X_LESTE - x)
    pilha_de_cones(CORES, X(688.15), 202.75, PISO_Y, 1.55, 5, 'branca', giro=0.2)
    pilha_de_cones(CORES, X(689.45), 202.90, PISO_Y, 1.35, 3, 'duas' if not espelhar else 'preta', giro=0.5)
    cone(CORES, X(688.30), 201.15, PISO_Y, 1.75, 'branca' if not espelhar else 'preta', giro=0.1)
    pilha_de_cones(CORES, X(690.55), 202.85, PISO_Y, 1.15, 4, 'preta' if not espelhar else 'branca', giro=0.8)
    R = Ref((X(688.6), 199.4 if not espelhar else 199.95), (1, 0), (0, 1))
    caixa(CORES, R, -0.75, 0.75, 0.0, 0.34, -0.55, 0.55, MADEIRA, sem=('baixo',), passo=0.5)
    for a in (-0.55, -0.15, 0.25, 0.62):
        caixa(CORES, R, a - 0.04, a + 0.04, 0.34, 0.345, -0.55, 0.55, MADEIRA_ESCURA, sem=('baixo',), passo=1)
    piramide_de_cones(CORES, R.o[0], R.o[1], PISO_Y + 0.345, 0.46, base=3, giro=0.0)
    if not espelhar:
        cone(CORES, X(689.95), 199.9, PISO_Y, 0.95, 'branca', giro=0.6)
    x0, x1 = sorted((X(X_OESTE), X(691.15)))
    obstaculo_caixa('cones_do_canto', x0, x1, 198.70 if not espelhar else 199.30, Z_FUNDO, altura=2.25)


def montar_cones():
    rng = np.random.default_rng(77)
    canto_de_cones(False, rng)
    canto_de_cones(True, rng)
    # os cones da vitrine de oeste (atrás do letreiro, como no filme), num estrado de madeira
    Re = Ref((690.70, 181.75), (1, 0), (0, 1))
    he = 0.52
    caixa(CORES, Re, -2.85, 2.85, 0.0, he, -0.95, 0.95, MADEIRA, sem=('baixo',), passo=0.5,
          cores={'cima': (0.70, 0.40, 0.20)})
    caixa(CORES, Re, -2.90, 2.90, he - 0.06, he, -1.00, 1.00, MADEIRA_ESCURA, sem=('baixo', 'cima'), passo=0.5)
    y = PISO_Y + he
    for (x, z, H, faixa, g) in ((688.30, 181.55, 1.05, 'branca', 0.2), (689.45, 181.30, 1.70, 'branca', 0.5),
                                (690.85, 181.75, 1.25, 'preta', 0.1), (693.10, 181.45, 1.45, 'branca', 0.7),
                                (689.95, 182.35, 0.60, None, 0.3), (692.20, 182.40, 0.68, 'branca', 0.9)):
        cone(CORES, x, z, y, H, faixa, giro=g)
    pilha_de_cones(CORES, 691.95, 181.35, y, 1.10, 3, 'branca', giro=0.4)
    obstaculo_caixa('cones_da_vitrine', 687.75, 693.65, Z_FRENTE_NO_PISO, 182.85, altura=he + 1.75)
    # embaixo do quadro de chaves
    cone(CORES, 708.70, 189.40, PISO_Y, 1.10, 'branca', giro=0.5)
    cone(CORES, 708.85, 190.30, PISO_Y, 0.62, None, giro=0.1)
    obstaculo_caixa('cones_do_quadro', 708.0, X_LESTE, 188.80, 190.80)
    # as prateleiras baixas de cones embaixo das janelas dos lados e dos fundos
    prateleira_baixa(P_OESTE, 183.05, 186.75, rng)
    prateleira_baixa(P_OESTE, 196.25, 198.55, rng)
    prateleira_baixa(P_LESTE, 182.95, 186.35, rng)
    prateleira_baixa(P_LESTE, 195.95, 199.15, rng)
    prateleira_baixa(P_FUNDO, 691.35, 694.18, rng)
    prateleira_baixa(P_FUNDO, 703.10, 705.55, rng)
    # cones pequenos nos peitoris fundos das janelas
    for W in (P_OESTE, P_LESTE, P_FUNDO):
        for v in W.vaos:
            if v.tipo != 'janela':
                continue
            for k in range(2):
                u = v.u0 + (v.u1 - v.u0) * (0.18 + 0.64 * k) + rng.uniform(-0.3, 0.3)
                b = -W.fundo * 0.45
                p = W.p(u, v.y0, b)
                cone(CORES, p[0], p[2], v.y0, rng.uniform(0.22, 0.34), 'branca' if k else None, giro=rng.uniform(0, 1))


parte('cones')
montar_cones()

# O letreiro OFFICE de neon, pendurado atrás da vitrine de oeste, virado para fora (para quem olha da praça): letras
# de 1 m de altura, tubos de 7 cm e, logo atrás deles, uma cópia bem mais grossa e mais escura (o halo). A parede
# da frente é inclinada (o alto vai para fora): o letreiro fica em pé, uns centímetros para dentro dela embaixo
NEON_ALTURA = 1.00
NEON_CENTRO = np.array([690.70, 4.98, z_frente(4.98 - NEON_ALTURA / 2) + 0.07])
RAIO_DO_NEON = 0.035
RAIO_DO_HALO = 0.085


def tracos_de_letra(letra, w, h):
    """Os traços (polilinhas em (s, t), s para a direita de quem lê) de cada letra do letreiro"""
    if letra == 'O':
        return [[(w / 2 + w / 2 * math.cos(a), h / 2 + h / 2 * math.sin(a)) for a in np.linspace(0, 2 * math.pi, 21)]]
    if letra == 'F':
        return [[(0, 0), (0, h), (w, h)], [(0, h * 0.52), (w * 0.78, h * 0.52)]]
    if letra == 'I':
        return [[(w / 2, 0), (w / 2, h)]]
    if letra == 'C':
        return [[(w / 2 + w / 2 * math.cos(a), h / 2 + h / 2 * math.sin(a)) for a in np.linspace(math.radians(40), math.radians(320), 16)]]
    if letra == 'E':
        return [[(w, h), (0, h), (0, 0), (w, 0)], [(0, h * 0.52), (w * 0.8, h * 0.52)]]
    return []


def montar_neon():
    h = NEON_ALTURA
    k = h / 0.62
    largs = {'O': 0.52 * k, 'F': 0.36 * k, 'I': 0.06 * k, 'C': 0.46 * k, 'E': 0.36 * k}
    espaco = 0.12 * k
    total = sum(largs[c] for c in 'OFFICE') + espaco * 5
    direita = np.array([-1.0, 0, 0])          # quem olha da praça (para +z) tem a direita em -x
    up = np.array([0, 1.0, 0])
    atras = np.array([0, 0, 1.0])
    s = -total / 2
    origem = NEON_CENTRO - up * h / 2
    for c in 'OFFICE':
        w = largs[c]
        for traco in tracos_de_letra(c, w, h):
            pts = [origem + direita * (s + a) + up * b for a, b in traco]
            tubo(CORES, pts, RAIO_DO_NEON, BRANCO, lados=6, brilho=NEON, fechar=True)
            tubo(CORES, [p + atras * 0.06 for p in pts], RAIO_DO_HALO, BRANCO, lados=6, brilho=NEON_HALO, fechar=True)
        s += w + espaco
    # as correntinhas que o seguram, até o teto
    for dx in (-total / 2 + 0.3, total / 2 - 0.3):
        p = NEON_CENTRO + direita * dx + up * (h / 2 + 0.03) + atras * 0.06
        topo = np.array([p[0], y_teto(p[2]) - 0.01, p[2]])
        tubo(CORES, [p, topo], 0.006, (0.30, 0.30, 0.32), lados=3)
    # a barra de trás, escura
    tubo(CORES, [NEON_CENTRO + direita * (-total / 2) + atras * 0.16 + up * 0.1, NEON_CENTRO + direita * (total / 2) + atras * 0.16 + up * 0.1],
         0.014, (0.18, 0.18, 0.20), lados=4)


parte('neon')
montar_neon()


# ------------------------------------------------------------------
# 3h. As paredes cheias dos lados: o cartaz, os quadros, a estante de folhetos, o quadro de chaves
# ------------------------------------------------------------------
def flamula(R, a, h, comprimento, altura, cor, txt, cor_do_texto=PAPEL_BRANCO, caida=-7.0):
    """Uma flâmula de feltro pregada na parede (no referencial R, a face para +n): o triângulo comprido com a
    ponta para a direita de quem olha, um pouco caída, a faixa da base e o nome escrito em letras de feltro"""
    direita = np.cross(-np.array([R.n[0], 0, R.n[1]]), np.array([0, 1.0, 0]))
    up = np.array([0, 1.0, 0])
    nrm = np.array([R.n[0], 0, R.n[1]])
    ang = math.radians(caida)
    ao_longo = direita * math.cos(ang) + up * math.sin(ang)
    de_pe = -direita * math.sin(ang) + up * math.cos(ang)
    base = R.p(a, h, 0.012)
    faixa = 0.07 * comprimento
    a0, a1 = base + de_pe * altura / 2, base - de_pe * altura / 2
    ponta = base + ao_longo * comprimento
    CORES.face([a1, a1 + ao_longo * faixa + de_pe * (altura / 2 * faixa / comprimento),
                a0 + ao_longo * faixa - de_pe * (altura / 2 * faixa / comprimento), a0], escurecer(cor, 0.55), n=nrm)
    CORES.face([a1 + ao_longo * faixa + de_pe * (altura / 2 * faixa / comprimento), ponta,
                a0 + ao_longo * faixa - de_pe * (altura / 2 * faixa / comprimento)], cor, n=nrm)
    # as fitinhas que pendem da base
    for s in (-0.3, 0.3):
        p = base + de_pe * altura * s
        CORES.face([p - ao_longo * 0.012, p + ao_longo * 0.012, p + ao_longo * 0.02 - up * 0.16, p - ao_longo * 0.004 - up * 0.16],
                   escurecer(cor, 0.55), n=nrm)
    centro = base + ao_longo * comprimento * 0.42 + nrm * 0.004
    texto(CORES, txt, FONTE_TITULO, centro, ao_longo, de_pe, comprimento * 0.56, altura * 0.30, cor_do_texto)


def montar_paredes_cheias():
    # oeste: o cartaz grande do Cozy Cone, quadros em volta e as flâmulas em cima
    R = Ref((X_OESTE, 191.45), (0, -1), (1, 0))     # a ao longo (para -z: a direita de quem olha a parede), b para a sala
    quadro(R, 0.0, 1.25, 0.0, 1.55, 'cartaz', (0.30, 0.16, 0.08), moldura=0.07, profundidade=0.035)
    quadro(R, 2.55, 1.85, 0.0, 0.70, 'rota66', MOLDURAS_DOS_QUADROS[1])
    quadro(R, -2.55, 1.85, 0.0, 0.70, 'motel', MOLDURAS_DOS_QUADROS[0])
    quadro(R, 2.55, 2.70, 0.0, 0.46, 'cartaz66', MOLDURAS_DOS_QUADROS[3])
    quadro(R, -2.55, 2.75, 0.0, 0.52, 'doc', (0.12, 0.10, 0.09))
    quadro(R, 3.70, 1.70, 0.0, 0.44, 'retrato_mesa', MOLDURAS_DOS_QUADROS[2], oval=True)
    quadro(R, -3.65, 1.75, 0.0, 0.50, 'lua', MOLDURAS_DOS_QUADROS[3])
    quadro(R, 1.30, 3.00, 0.0, 0.48, 'cone_noite', MOLDURAS_DOS_QUADROS[1])
    quadro(R, -1.30, 3.00, 0.0, 0.48, 'mesas', MOLDURAS_DOS_QUADROS[0])
    # as flâmulas, embaixo das bandeiras (que começam 4,3 m acima do piso)
    for k, (a, cor, txt) in enumerate(((-3.9, (0.78, 0.14, 0.12), 'RADIATOR SPRINGS'), (-1.2, (0.16, 0.34, 0.62), 'ROTA 66'),
                                       (1.6, (0.92, 0.46, 0.10), 'COZY CONE'))):
        flamula(R, a, 3.95 + 0.10 * (k % 2), 1.45, 0.42, cor, txt)
    # a estante de folhetos embaixo do cartaz
    Rf = Ref((X_OESTE + 0.02, 191.45), (0, -1), (1, 0))
    caixa(CORES, Rf, -0.75, 0.75, 0.0, 1.05, 0.0, 0.36, MADEIRA, sem=('tras', 'baixo'), passo=0.5)
    u0, v0, u1, v1 = uv_da_celula('folhetos')
    for k, h in enumerate((0.40, 0.72, 1.04)):
        # a tábua inclinada com os folhetos (meia célula em cada fileira)
        vm = (v0 + v1) / 2
        uv = [(u0, v0), (u1, v0), (u1, vm), (u0, vm)] if k % 2 == 0 else [(u0, vm), (u1, vm), (u1, v1), (u0, v1)]
        a = Rf.p(-0.70, h - 0.26, 0.38)
        b = Rf.p(0.70, h - 0.26, 0.38)
        c = Rf.p(0.70, h, 0.26)
        d = Rf.p(-0.70, h, 0.26)
        quad(QUADROS, a, b, c, d, BRANCO, n=Rf.d(0, 0.5, 1), uv=uv, div=(2, 1))
        quad(CORES, Rf.p(-0.72, h - 0.28, 0.40), Rf.p(0.72, h - 0.28, 0.40), Rf.p(0.72, h - 0.20, 0.40), Rf.p(-0.72, h - 0.20, 0.40),
             MADEIRA_ESCURA, n=Rf.d(0, 0, 1), div=(2, 1))
    obstaculo_caixa('folhetos', X_OESTE, X_OESTE + 0.55, 190.55, 192.35)
    # leste: o quadro de chaves (dos oito cones), o calendário, quadros e as flâmulas em cima
    R = Ref((X_LESTE, 191.10), (0, 1), (-1, 0))
    caixa(CORES, R, -0.85, 0.85, 1.35, 2.45, 0.0, 0.05, MADEIRA, sem=('tras',), passo=0.5)
    u0, v0, u1, v1 = uv_da_celula('chaves')
    quad(QUADROS, R.p(-0.55, 2.12, 0.052), R.p(0.55, 2.12, 0.052), R.p(0.55, 2.40, 0.052), R.p(-0.55, 2.40, 0.052), BRANCO,
         n=R.d(0, 0, 1), uv=[(u0, v0), (u1, v0), (u1, v1), (u0, v1)], div=(2, 1))
    for k in range(8):
        a = -0.70 + k * 0.20
        tubo(CORES, [R.p(a, 1.92, 0.05), R.p(a, 1.92, 0.10)], 0.008, LATAO, lados=4)
        p = R.p(a, 1.68, 0.09)
        cone(CORES, p[0], p[2], p[1], 0.17, 'branca', giro=0.0, lados=8)
        tubo(CORES, [R.p(a, 1.85, 0.09), R.p(a, 1.92, 0.10)], 0.003, (0.5, 0.5, 0.5), lados=3)
        texto(CORES, str(k + 1), FONTE_TITULO, R.p(a, 1.55, 0.055), R.d(1, 0, 0), (0, 1, 0), 0.08, 0.09, (0.95, 0.88, 0.70))
    quadro(R, -2.40, 1.70, 0.0, 0.62, 'calendario', (0.85, 0.85, 0.82), moldura=0.02)
    quadro(R, 2.40, 1.90, 0.0, 0.80, 'mcqueen', MOLDURAS_DOS_QUADROS[3])
    quadro(R, 2.40, 1.05, 0.0, 0.62, 'sally', MOLDURAS_DOS_QUADROS[1])
    quadro(R, 0.0, 2.85, 0.0, 0.70, 'placa', MOLDURAS_DOS_QUADROS[0])
    quadro(R, -3.75, 1.75, 0.0, 0.52, 'cactos', MOLDURAS_DOS_QUADROS[1])
    quadro(R, 1.35, 2.95, 0.0, 0.44, 'retrato_cone', MOLDURAS_DOS_QUADROS[2], oval=True)
    quadro(R, -2.40, 2.75, 0.0, 0.46, 'cones', MOLDURAS_DOS_QUADROS[4])
    for k, (a, cor, txt) in enumerate(((-3.9, (0.20, 0.44, 0.30), 'ORNAMENT VALLEY'), (-1.2, (0.78, 0.14, 0.12), 'COPA PISTÃO'),
                                       (1.6, (0.16, 0.34, 0.62), 'RADIATOR SPRINGS'))):
        flamula(R, a, 3.95 + 0.10 * (k % 2), 1.45, 0.42, cor, txt)


parte('paredes cheias')
montar_paredes_cheias()


# ------------------------------------------------------------------
# 3i. As luminárias pendentes de cone
# ------------------------------------------------------------------
# (x, z, para onde a boca se inclina: None ao acaso; 0 para oeste, pi para leste...) as de perto das paredes
# iluminam a parede
LUMINARIAS = [(698.20, 186.60, None), (691.30, 188.70, 0.0), (705.30, 188.60, math.pi), (691.40, 196.80, None),
              (703.10, 196.55, None), (705.90, 196.55, None), (697.90, 196.20, None), (690.10, 183.50, 0.6),
              (706.50, 183.40, math.pi - 0.6), (707.40, 192.10, math.pi)]
BOCAS = []


def luminaria(x, z, rng, queda=1.15, giro=None):
    teto = y_teto(z)
    up = np.array([0, 1.0, 0])
    disco(CORES, (x, teto - 0.01, z), (0, -1, 0), (1, 0, 0), 0.09, 10, (0.30, 0.20, 0.12))
    pivo = np.array([x, teto - queda, z])
    tubo(CORES, [np.array([x, teto - 0.01, z]), pivo], 0.022, (0.32, 0.20, 0.10), lados=6)
    # a barrinha em T no alto da haste (como no filme)
    tubo(CORES, [pivo + np.array([-0.10, 0.05, 0]), pivo + np.array([0.10, 0.05, 0])], 0.02, (0.32, 0.20, 0.10), lados=6)
    giro = rng.uniform(0, 2 * math.pi) if giro is None else giro
    inclina = math.radians(28)
    eixo = unit(np.array([math.sin(inclina) * math.cos(giro), math.cos(inclina), math.sin(inclina) * math.sin(giro)]))
    comprimento = 0.68
    boca = pivo - eixo * comprimento
    cupula_de_cone(CORES, boca, eixo, comprimento, 0.30, 0.055, lados=18, aba=0.10)
    BOCAS.append((boca, -eixo))


parte('luminárias')
for k, (x, z, giro) in enumerate(LUMINARIAS):
    luminaria(x, z, np.random.default_rng(100 + k), queda=0.32 if z > 194 else 0.55, giro=giro)

# Os abajures de pé (a mesma cúpula de cone, numa haste alta), nos cantos da sala
LUMINARIAS_DE_PE = [(706.80, 186.90), (689.70, 189.10)]
BOCAS_DE_PE = []


def luminaria_de_pe(x, z, giro):
    base = np.array([x, PISO_Y, z])
    up = np.array([0, 1.0, 0])
    torno(CORES, base, up, (1, 0, 0), [(0.25, 0.0), (0.25, 0.03), (0.20, 0.06), (0.05, 0.09)], 16, (0.30, 0.22, 0.14))
    topo = base + up * 2.15
    tubo(CORES, [base + up * 0.08, topo], 0.018, LATAO, lados=8)
    for h in (0.10, 1.10, 2.05):
        torno(CORES, base + up * h, up, (1, 0, 0), [(0.03, 0.0), (0.03, 0.04)], 8, (0.62, 0.46, 0.20))
    eixo = unit(np.array([0.32 * math.cos(giro), 1.0, 0.32 * math.sin(giro)]))
    boca = topo - eixo * 0.08
    cupula_de_cone(CORES, boca, eixo, 0.56, 0.26, 0.045, lados=16, aba=0.09)
    BOCAS_DE_PE.append((boca, -eixo))
    obstaculo_redondo('abajur_de_pe', x, z, 0.40, altura=2.75)


for k, (x, z) in enumerate(LUMINARIAS_DE_PE):
    luminaria_de_pe(x, z, 0.6 + 2.1 * k)
print('  móveis: %.0f s' % (time.time() - INICIO))
for _nome, _t in sorted(TRIANGULOS_POR_PARTE.items(), key=lambda kv: -kv[1]):
    print('    %-16s %7d triângulos' % (_nome, _t))
print('    %-16s %7d triângulos' % ('total', sum(TRIANGULOS_POR_PARTE.values())))


# ------------------------------------------------------------------
# 4. Objetos, materiais e a luz assada
# ------------------------------------------------------------------
def criar_objetos():
    objs = []
    for chave, m in MALHAS.items():
        for k, bm in enumerate(m.bms):
            if len(bm.faces) == 0:
                continue
            me = bpy.data.meshes.new('%s_%d' % (m.nome, k + 1))
            bm.to_mesh(me)
            bm.free()
            o = bpy.data.objects.new(me.name, me)
            bpy.context.scene.collection.objects.link(o)
            o['malha'] = chave
            objs.append(o)
    for chave in MALHAS:
        t = 0
        for o in objs:
            if o['malha'] == chave:
                o.data.calc_loop_triangles()
                t += len(o.data.loop_triangles)
        print('  %-10s %7d triângulos' % (chave, t))
    return objs


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
    geo = nt.nodes.new('ShaderNodeNewGeometry')
    frente = nt.nodes.new('ShaderNodeMath')
    frente.operation = 'MULTIPLY_ADD'
    frente.inputs[1].default_value = -5.0
    frente.inputs[2].default_value = 5.0
    nt.links.new(geo.outputs['Backfacing'], frente.inputs[0])
    nt.links.new(frente.outputs[0], em.inputs['Strength'])
    soma = nt.nodes.new('ShaderNodeAddShader')
    nt.links.new(bsdf.outputs[0], soma.inputs[0])
    nt.links.new(em.outputs[0], soma.inputs[1])
    nt.links.new(soma.outputs[0], saida.inputs['Surface'])
    return mat


def b_de(p):
    return Vector((p[0], -p[2], p[1]))


CEU_DO_ASSADO = 0.06         # o céu (azul) que entra pelas janelas: pouco (no filme é de noite)
SOL_DO_ASSADO = 0.35         # o sol (W/m²), na direção do sol do jogo (src/mundo.c): só entra pelos fundos e por leste
LUZ_DA_LUMINARIA = 1400.0    # W, o facho de cada luminária pendente...
ABERTURA_DA_LUMINARIA = math.radians(78)   # ...estreito, para fazer poças de luz no chão
MISTURA_DA_LUMINARIA = 0.32
LUZ_NA_CUPULA = 90.0         # W, a lâmpada nua dentro da cúpula (clareia o teto e o alto das paredes em volta)
LUZ_DO_ABAJUR = 160.0
LUZ_DE_PE = 380.0
LUZ_DO_NEON = 600.0          # tinge de laranja a vitrine do letreiro
LUZ_DA_ESTANTE = 1500.0       # W, cada um dos dois focos do teto virados para a estante
LUZ_DO_TAMPO = 260.0         # W, o foco em cima do balcão
COR_DAS_LUZES = (1.0, 0.64, 0.34)    # bem quente, como no filme
LUZ_DO_REBATIDO = 0.0       # W/m², a luz fraca virada para o teto (o rebatido do piso claro)
DIRECAO_DO_SOL = (-0.35, -0.8, -0.45)    # para onde a luz do sol vai (o sol do jogo fica em (0,35; 0,8; 0,45))
DISTANCIA_DA_OCLUSAO = 0.8   # m: a oclusão de ambiente escurece os cantos, o pé do balcão e as junções


def foco(nome, de, para, energia, abertura, mistura_, cor=COR_DAS_LUZES, raio=0.15):
    sc = bpy.context.scene
    L = bpy.data.lights.new(nome, 'SPOT')
    L.energy = energia
    L.spot_size = abertura
    L.spot_blend = mistura_
    L.shadow_soft_size = raio
    L.color = cor
    o = bpy.data.objects.new(nome, L)
    o.location = b_de(de)
    o.rotation_euler = b_de(unit(np.asarray(para, float) - np.asarray(de, float))).to_track_quat('-Z', 'Y').to_euler()
    sc.collection.objects.link(o)
    return o


def montar_luzes_do_cycles():
    sc = bpy.context.scene
    w = novo_mundo('ceu_da_recepcao')
    sc.world = w
    bg = w.node_tree.nodes['Background']
    bg.inputs[0].default_value = (0.55, 0.68, 0.95, 1)
    bg.inputs[1].default_value = CEU_DO_ASSADO
    w.light_settings.distance = DISTANCIA_DA_OCLUSAO
    sol = bpy.data.lights.new('sol', 'SUN')
    sol.energy = SOL_DO_ASSADO
    sol.color = (1.0, 0.93, 0.82)
    sol.angle = math.radians(2.0)
    so = bpy.data.objects.new('sol', sol)
    sc.collection.objects.link(so)
    so.rotation_euler = b_de(unit(DIRECAO_DO_SOL)).to_track_quat('-Z', 'Y').to_euler()
    for k, (boca, para) in enumerate(BOCAS):
        d = unit(np.asarray(para) * 0.55 + np.array([0, -1.0, 0]) * 0.45)
        foco('luminaria_%d' % k, boca + para * 0.02, boca + para * 0.02 + d, LUZ_DA_LUMINARIA, ABERTURA_DA_LUMINARIA,
             MISTURA_DA_LUMINARIA, raio=0.18)
        P = bpy.data.lights.new('cupula_%d' % k, 'POINT')
        P.energy = LUZ_NA_CUPULA
        P.color = (1.0, 0.70, 0.40)
        P.shadow_soft_size = 0.1
        o = bpy.data.objects.new(P.name, P)
        o.location = b_de(boca - para * 0.30)
        sc.collection.objects.link(o)
    for k, (boca, para) in enumerate(BOCAS_DE_PE):
        d = unit(np.asarray(para) * 0.5 + np.array([0, -1.0, 0]) * 0.5)
        foco('de_pe_%d' % k, boca + para * 0.02, boca + para * 0.02 + d, LUZ_DE_PE, math.radians(95), 0.35, raio=0.12)
        P = bpy.data.lights.new('de_pe_cupula_%d' % k, 'POINT')
        P.energy = LUZ_NA_CUPULA * 0.7
        P.color = (1.0, 0.70, 0.40)
        o = bpy.data.objects.new(P.name, P)
        o.location = b_de(boca - para * 0.25)
        sc.collection.objects.link(o)
    foco('abajur', BOCA_DO_ABAJUR + np.array([0, -0.03, 0]), BOCA_DO_ABAJUR + np.array([-0.1, -1.03, 0.25]), LUZ_DO_ABAJUR,
         math.radians(110), 0.5)
    # os focos do teto: dois na estante (a luz própria dela, como no filme) e um no tampo do balcão
    ze = Z_FUNDO - ESTANTE_FUNDO
    for k, x in enumerate((ESTANTE_X[0] + 2.2, ESTANTE_X[1] - 2.2)):
        de = np.array([x, y_teto(ze - 2.6) - 0.15, ze - 2.6])
        foco('estante_%d' % k, de, (x, PISO_Y + 2.1, ze + 0.3), LUZ_DA_ESTANTE, math.radians(58), 0.45)
    de = np.array([(BALCAO_X[0] + BALCAO_X[1]) / 2 + 0.6, y_teto(BALCAO_Z[0] - 0.4) - 0.15, BALCAO_Z[0] - 0.4])
    foco('tampo', de, ((BALCAO_X[0] + BALCAO_X[1]) / 2 + 0.6, Y_TAMPO, (BALCAO_Z[0] + BALCAO_Z[1]) / 2), LUZ_DO_TAMPO,
         math.radians(75), 0.5)
    P = bpy.data.lights.new('neon', 'POINT')
    P.energy = LUZ_DO_NEON
    P.color = (1.0, 0.42, 0.14)
    P.shadow_soft_size = 0.8
    o = bpy.data.objects.new('neon', P)
    o.location = b_de(NEON_CENTRO + np.array([0, 0, 0.25]))
    sc.collection.objects.link(o)
    if LUZ_DO_REBATIDO <= 0:
        return
    L = bpy.data.lights.new('rebatido', 'AREA')
    L.shape = 'RECTANGLE'
    L.size = X_LESTE - X_OESTE
    L.size_y = Z_FUNDO - Z_FRENTE_NO_PISO
    L.energy = LUZ_DO_REBATIDO * L.size * L.size_y
    L.color = (1.0, 0.90, 0.78)
    o = bpy.data.objects.new('rebatido', L)
    o.location = b_de(((X_OESTE + X_LESTE) / 2, PISO_Y + 2.6, (Z_FRENTE_NO_PISO + Z_FUNDO) / 2))
    o.rotation_euler = (math.pi, 0, 0)
    sc.collection.objects.link(o)


# A luz de noite (--noite): sem o sol; o céu vira um luar fraco, azul-arroxeado, entrando pelas janelas; as
# luzes de dentro, todas acesas e mais presentes: as cúpulas de cone mais acesas, os abajures, e o letreiro
# OFFICE, que tinge de laranja a vitrine dele e a sala em volta
CEU_DA_NOITE = (0.30, 0.28, 0.62)
FORCA_DO_CEU_DA_NOITE = 0.025
NOITE_DAS_LUZES = (('luminaria_', 1.0), ('cupula_', 2.4), ('de_pe_', 1.4), ('abajur', 1.6), ('estante_', 0.9),
                   ('tampo', 1.2), ('neon', 4.0), ('rebatido', 0.5))


def montar_luzes_da_noite():
    montar_luzes_do_cycles()
    sc = bpy.context.scene
    bg = sc.world.node_tree.nodes['Background']
    bg.inputs[0].default_value = (*CEU_DA_NOITE, 1)
    bg.inputs[1].default_value = FORCA_DO_CEU_DA_NOITE
    for o in list(sc.objects):
        if o.type != 'LIGHT':
            continue
        if o.data.type == 'SUN':
            bpy.data.objects.remove(o, do_unlink=True)
            continue
        for prefixo, f in NOITE_DAS_LUZES:
            if o.name.startswith(prefixo):
                o.data.energy *= f
                break
    neon = next((o for o in sc.objects if o.type == 'LIGHT' and o.name.startswith('neon')), None)
    if neon is not None:
        neon.data.shadow_soft_size = 1.2       # o brilho do letreiro, largo


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


def assar(objs):
    """Assa nas cores dos vértices a luz difusa (direta e indireta, sem a cor) em 'luz' e a oclusão de
    ambiente (até DISTANCIA_DA_OCLUSAO) em 'ao'"""
    sc = bpy.context.scene
    sc.render.engine = 'CYCLES'
    ligar_gpu()
    sc.cycles.samples = AMOSTRAS
    sc.cycles.use_denoising = False
    sc.cycles.max_bounces = 6
    sc.cycles.diffuse_bounces = 4
    sc.cycles.glossy_bounces = 1
    sc.cycles.transparent_max_bounces = 4
    for o in objs:
        m = MALHAS[o['malha']]
        o.data.materials.clear()
        o.data.materials.append(material_de_assar(m))
    bpy.ops.object.select_all(action='DESELECT')
    for o in objs:
        o.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]
    for nome, tipo, filtro, amostras in (('luz', 'DIFFUSE', {'DIRECT', 'INDIRECT'}, AMOSTRAS), ('ao', 'AO', None, min(AMOSTRAS, 128))):
        for o in objs:
            at = o.data.color_attributes.new(nome, 'FLOAT_COLOR', 'CORNER')
            o.data.color_attributes.active_color = at
        sc.cycles.samples = amostras
        t = time.time()
        if filtro:
            bpy.ops.object.bake(type=tipo, pass_filter=filtro, target='VERTEX_COLORS', use_clear=True)
        else:
            bpy.ops.object.bake(type=tipo, target='VERTEX_COLORS', use_clear=True)
        print('  %s assada em %.0f s' % (nome, time.time() - t))


def ler_atributo(me, nome):
    at = me.color_attributes.get(nome)
    a = np.zeros(len(me.loops) * 4, np.float32)
    if at is not None:
        at.data.foreach_get('color', a)
    return a.reshape(-1, 4)[:, :3]


def suavizar(me, valores):
    """Média dos cantos no mesmo ponto com a face virada mais ou menos para o mesmo lado"""
    vi = np.zeros(len(me.loops), np.int64)
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


AMBIENTE = 0.008         # quase nada de luz por igual: os cantos ficam mais escuros que as poças de luz
ALVO_DAS_POCAS = 0.85    # a exposição é fixa: o p95 do piso embaixo das luminárias fica com este valor na tela
RAIO_DAS_POCAS = 1.3     # m, em volta do ponto onde o facho de cada luminária pendente bate no piso
CONTRASTE = 1.15         # a luz na tela (sem a cor) vai a esta potência: escurece o que é pouco iluminado
FORCA_DA_OCLUSAO = 0.75  # quanto da oclusão de ambiente escurece a luz (1: toda)
SATURACAO_DA_LUZ = 0.80  # quanto da cor da luz (quente) fica na cor do vértice
SATURACAO_DO_PISO = 0.60
if NOITE:
    # as mesmas contas, com a noite: as poças quentes das luminárias no alto da escala, o resto mais escuro
    AMBIENTE = 0.003
    ALVO_DAS_POCAS = 0.90
    CONTRASTE = 1.45
    SATURACAO_DA_LUZ = 0.86
    SATURACAO_DO_PISO = 0.66


def dados_dos_cantos(me):
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
    suave = np.zeros(nf, np.int32)
    at = me.attributes.get('suave')
    if at is not None:
        at.data.foreach_get('value', suave)
    face = np.repeat(np.arange(nf), lt)
    return dict(face=face, ls=ls, lt=lt, co=co.reshape(-1, 3)[vi], n=fn.reshape(-1, 3)[face], suave=suave.astype(bool)[face])


def alisar(me, valores, passadas=2, cantos=None):
    """Alisa (laplaciano) os valores dos cantos das faces 'suave', só entre vértices vizinhos (ligados
    por uma aresta) de mesma normal"""
    d = cantos or dados_dos_cantos(me)
    idx = np.flatnonzero(d['suave'])
    if len(idx) == 0:
        return valores
    chave = np.concatenate([np.round(d['co'][idx] * 500), np.round(d['n'][idx] * 20)], axis=1).astype(np.int64)
    _, no = np.unique(chave, axis=0, return_inverse=True)
    no = no.ravel()
    nn = int(no.max()) + 1
    no_do_canto = np.full(len(d['face']), -1, np.int64)
    no_do_canto[idx] = no
    f = d['face'][idx]
    s, t = d['ls'][f], d['lt'][f]
    prox = s + (idx - s + 1) % t
    a, b = no_do_canto[idx], no_do_canto[prox]
    ok = b >= 0
    a, b = a[ok], b[ok]
    val = np.zeros((nn, valores.shape[1]))
    cnt = np.zeros(nn)
    np.add.at(val, no, valores[idx])
    np.add.at(cnt, no, 1)
    val /= np.maximum(cnt, 1)[:, None]
    for _ in range(passadas):
        soma = np.zeros_like(val)
        grau = np.zeros(nn)
        np.add.at(soma, a, val[b])
        np.add.at(soma, b, val[a])
        np.add.at(grau, a, 1)
        np.add.at(grau, b, 1)
        tem = grau > 0
        val[tem] = 0.5 * val[tem] + 0.5 * soma[tem] / grau[tem, None]
    saida = valores.copy()
    saida[idx] = val[no]
    return saida


def joelho(x):
    return np.where(x < 0.82, x, 0.82 + 0.18 * (1 - np.exp(-(x - 0.82) / 0.18)))


LUMA = np.array([0.2126, 0.7152, 0.0722])


def na_tela(luz, escala):
    """A luz assada (linear) na escala de tela, com o joelho e o contraste"""
    return np.power(joelho(tela(luz * escala + AMBIENTE)), CONTRASTE)


def centros_das_pocas():
    """Onde o facho de cada luminária pendente bate no piso (x, z do jogo)"""
    out = []
    for boca, para in BOCAS:
        d = unit(np.asarray(para) * 0.55 + np.array([0, -1.0, 0]) * 0.45)
        t = (boca[1] - PISO_Y) / max(-d[1], 1e-3)
        out.append((boca[0] + d[0] * t, boca[2] + d[2] * t))
    return np.array(out)


def finalizar_cores(objs):
    """A luz assada (linear) vira a cor do vértice: alisada, numa exposição fixa (as poças de luz do piso em
    ALVO_DAS_POCAS), com o contraste, vezes a oclusão de ambiente e vezes a tinta"""
    dados = []
    for o in objs:
        me = o.data
        cantos = dados_dos_cantos(me)
        luz = alisar(me, suavizar(me, ler_atributo(me, 'luz')), cantos=cantos)
        ao = alisar(me, suavizar(me, ler_atributo(me, 'ao')), cantos=cantos)[:, :1]
        tinta = ler_atributo(me, 'tinta')
        brilho = ler_atributo(me, 'brilho')
        dados.append((o, luz, ao, tinta, brilho, cantos))
    # a exposição: o p95 do piso nas poças de luz das luminárias
    pocas = centros_das_pocas()
    lum = []
    for o, luz, ao, tinta, brilho, cantos in dados:
        if o['malha'] != 'piso':
            continue
        xz = np.stack([cantos['co'][:, 0], -cantos['co'][:, 1]], 1)        # (x, z do jogo)
        d = np.min(np.linalg.norm(xz[:, None, :] - pocas[None, :, :], axis=2), axis=1)
        lum.append((luz @ LUMA)[d < RAIO_DAS_POCAS])
    lum = np.concatenate(lum)
    lo, hi = 1e-4, 100.0
    for _ in range(60):
        meio = math.sqrt(lo * hi)
        if np.percentile(na_tela(lum, meio), 95) > ALVO_DAS_POCAS:
            hi = meio
        else:
            lo = meio
    escala = math.sqrt(lo * hi)
    print('  luz: exposição %.3f (o p95 das poças de luz do piso na tela = %.2f)' % (escala, np.percentile(na_tela(lum, escala), 95)))
    medidas = {'piso': [], 'quadros': [], 'teto': [], 'paredes': [], 'oclusão': []}
    for o, luz, ao, tinta, brilho, cantos in dados:
        me = o.data
        tela_luz = na_tela(luz, escala) * (1.0 - FORCA_DA_OCLUSAO * (1.0 - np.clip(ao, 0, 1)))
        cinza = (tela_luz @ LUMA)[:, None]
        sat = SATURACAO_DO_PISO if o['malha'] == 'piso' else SATURACAO_DA_LUZ
        tela_luz = cinza + (tela_luz - cinza) * sat
        cor = tela_luz * tela(tinta)
        acesa = brilho.sum(1) > 0
        cor[acesa] = np.clip(tela(brilho[acesa]), 0, 1)
        cor = np.clip(cor, 0, 1)
        lt = tela_luz @ LUMA
        if o['malha'] == 'piso':
            medidas['piso'].append(lt)
        elif o['malha'] == 'quadros':
            medidas['quadros'].append(lt)
        elif o['malha'] == 'teto':
            medidas['teto'].append(lt)
        elif o['malha'] == 'cores':
            s = cantos['suave'] & ~acesa
            medidas['paredes'].append(lt[s & (np.abs(cantos['n'][:, 2]) < 0.3)])
        medidas['oclusão'].append(ao[~acesa, 0])
        if me.color_attributes.get('Col'):
            me.color_attributes.remove(me.color_attributes['Col'])
        at = me.color_attributes.new('Col', 'FLOAT_COLOR', 'CORNER')
        a = np.concatenate([cor, np.ones((len(cor), 1))], axis=1).astype(np.float32).ravel()
        at.data.foreach_set('color', a)
        me.color_attributes.active_color = me.color_attributes['Col']
        me.color_attributes.render_color_index = me.color_attributes.active_color_index
    for nome, v in medidas.items():
        v = np.concatenate(v) if v else np.zeros(1)
        print('  luz na tela, %-10s p10 %.2f  mediana %.2f  p90 %.2f' % (nome, *np.percentile(v, [10, 50, 90])))
    return escala


def material_de_previa(m):
    """O que o jogo mostra: textura × cor do vértice, sem luz"""
    mat = novo_material('previa_' + m.nome)
    nt = mat.node_tree
    nt.nodes.clear()
    saida = nt.nodes.new('ShaderNodeOutputMaterial')
    at = nt.nodes.new('ShaderNodeAttribute')
    at.attribute_name = 'Col'
    gama = nt.nodes.new('ShaderNodeGamma')
    gama.inputs['Gamma'].default_value = 2.2
    nt.links.new(at.outputs['Color'], gama.inputs['Color'])
    cor = gama.outputs['Color']
    if m.imagem is not None:
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


# ------------------------------------------------------------------
# 4a. A casca do prédio (do mapa): entra no assado (sombra e rebatido) e nas prévias
# ------------------------------------------------------------------
def tirar_da_casca(objetos, nome, mn, mx):
    """Apaga, dos objetos do mapa (já nas coordenadas do mundo), as peças inteiras dentro da caixa (com a
    folga do integrador, 0,05 m), de qualquer objeto, como o tirar_pecas() de montar_mundo_aberto.py"""
    folga = 0.05
    tiradas = 0
    dentro = lambda v: (mn[0] - folga <= v.x <= mx[0] + folga and mn[1] - folga <= v.z <= mx[1] + folga
                        and mn[2] - folga <= -v.y <= mx[2] + folga)
    for o in objetos:
        me = o.data
        bm = bmesh.new()
        bm.from_mesh(me)
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
            if all(dentro(v.co) for g in peca for v in g.verts):
                apagar.extend(peca)
        if apagar:
            bmesh.ops.delete(bm, geom=apagar, context='FACES')
            bm.to_mesh(me)
            tiradas += len(apagar)
        bm.free()
    return tiradas


def recortar_da_casca(objetos, cantos, profundidade):
    """Abre um vão na casca como o recortar() de montar_mundo_aberto.py: corta as faces (as que têm um vértice
    perto do vão) pelos 4 planos dos lados dele e apaga as que têm o centro dentro do vão, numa faixa de
    ±(profundidade + FORA_DO_RECORTE) em volta do plano dos cantos. Devolve as faces tiradas"""
    cantos = [b_de(c) for c in cantos]
    meio = sum(cantos, Vector()) / 4
    normal = (cantos[1] - cantos[0]).cross(cantos[3] - cantos[0]).normalized()
    lados = []
    for i in range(4):
        aresta = cantos[(i + 1) % 4] - cantos[i]
        para_dentro = normal.cross(aresta).normalized()
        if para_dentro.dot(meio - cantos[i]) < 0:
            para_dentro = -para_dentro
        lados.append((cantos[i], para_dentro))

    def no_vao(p, folga=1e-4):
        if any((p - c).dot(n) < -folga for c, n in lados):
            return False
        return abs((p - meio).dot(normal)) <= profundidade + FORA_DO_RECORTE
    raio = max((c - meio).length for c in cantos) + profundidade + FORA_DO_RECORTE
    tiradas = 0
    for o in objetos:
        bm = bmesh.new()
        bm.from_mesh(o.data)
        perto = lambda f: any((v.co - meio).length < raio for v in f.verts)
        if not any(perto(f) for f in bm.faces):
            bm.free()
            continue
        for c, n in lados:
            faces = [f for f in bm.faces if perto(f)]
            geom = faces + list({e for f in faces for e in f.edges}) + list({v for f in faces for v in f.verts})
            bmesh.ops.bisect_plane(bm, geom=geom, plane_co=c, plane_no=n)
        apagar = [f for f in bm.faces if perto(f) and no_vao(f.calc_center_median())]
        if apagar:
            bmesh.ops.delete(bm, geom=apagar, context='FACES')
            bm.to_mesh(o.data)
            tiradas += len(apagar)
        bm.free()
    return tiradas


def area_por_objeto(objetos):
    return {o.name: sum(p.area for p in o.data.polygons) for o in objetos}


def importar_casca():
    """Importa o mapa, guarda só o que está perto da recepção (nas coordenadas do mundo, como o integrador)
    e tira dele as peças e os vãos do json, na mesma ordem do integrador (primeiro o que sai da colisão);
    confere que os recortes só tiram área do Object_16 (o vidro), e a da conta"""
    if not os.path.exists(ARQUIVO_DO_MAPA):
        print('  sem o mapa (%s): assado e prévias sem a casca' % ARQUIVO_DO_MAPA)
        return []
    antes = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=ARQUIVO_DO_MAPA)
    novos = [o for o in bpy.data.objects if o not in antes]
    bpy.context.view_layer.update()
    guardar = []
    meio = Vector((698.2, -192.0, 0))
    for o in novos:
        if o.type != 'MESH':
            continue
        bb = [o.matrix_world @ Vector(c) for c in o.bound_box]
        mn = Vector((min(v.x for v in bb), min(v.y for v in bb), min(v.z for v in bb)))
        mx = Vector((max(v.x for v in bb), max(v.y for v in bb), max(v.z for v in bb)))
        if mx.x > meio.x - 130 and mn.x < meio.x + 130 and mx.y > meio.y - 130 and mn.y < meio.y + 130:
            guardar.append(o)
    for o in novos:
        if o not in guardar:
            bpy.data.objects.remove(o, do_unlink=True)
    for o in guardar:
        mw = o.matrix_world.copy()
        o.parent = None
        o.data = o.data.copy()
        o.data.transform(mw)
        o.matrix_world = Matrix.Identity(4)
    ror = [o for o in guardar if o.name.startswith('ror_')]
    for sai in (True, False):
        for nome, _, mn, mx, faces, _, sai_da_colisao in ABERTURAS_DA_CASCA:
            if sai_da_colisao == sai:
                n = tirar_da_casca(ror, nome, mn, mx)
                print('  casca: %s -> %d faces tiradas (%d esperadas)' % (nome, n, faces))
                if n == 0 and NOITE:
                    print('    (já aberta no mapa: o integrador tirou esta peça; a casca fica igual)')
                elif n != faces:
                    raise SystemExit('ERRO: a abertura %s tirou %d faces da casca (esperadas %d)' % (nome, n, faces))
    for r in RECORTES_DA_CASCA:
        nome, _, (x0, x1), (y0, y1), zc, prof = r
        antes = area_por_objeto(ror)
        n = recortar_da_casca(ror, cantos_do_recorte(r), prof)
        depois = area_por_objeto(ror)
        perdas = {k: antes[k] - depois[k] for k in antes if antes[k] - depois[k] > 1e-3}
        z = lambda y: 180.163 - 0.29979 * (y - 1.973)      # o plano dos vidros inclinados da frente
        esperada = (x1 - x0) * ((y1 - y0) if zc > 200 else math.hypot(y1 - y0, z(y1) - z(y0)))
        print('  casca: %s -> recorte, %d faces, %.2f m2 tirados (%.2f esperados) de %s' % (
            nome, n, sum(perdas.values()), esperada, ', '.join(sorted(k.split('.')[0] for k in perdas))))
        if NOITE and not perdas:
            print('    (já recortado no mapa: o integrador abriu este vão; a casca fica igual)')
        elif any(k.split('.')[0] != 'ror_Object_16' for k in perdas) or abs(sum(perdas.values()) - esperada) > 0.05:
            raise SystemExit('ERRO: o recorte %s tirou da casca o que não devia' % nome)
    # o tampo da caixa de vidro fica no mapa (em cima do nosso teto); para o assado e as prévias, sai também
    # (depois de conferir que nenhum pedaço da casca desce a menos de FOLGA_MINIMA_DA_CASCA do teto novo)
    conferir_teto(guardar)
    vidro = [o for o in guardar if o.name.split('.')[0] == 'ror_Object_16']
    for o in vidro:
        guardar.remove(o)
        bpy.data.objects.remove(o, do_unlink=True)
    dg = bpy.context.evaluated_depsgraph_get()
    for nome, (x, z) in (('na entrada', (698.2, 178.6)), ('dentro', (698.2, 190.0))):
        ok, loc, *_ = bpy.context.scene.ray_cast(dg, b_de((x, 3.0, z)), Vector((0, 0, -1)))
        print('  chão do mapa %s (%.1f, %.1f): y = %s' % (nome, x, z, '%.3f' % loc.z if ok else 'nenhum'))
    return guardar


def conferir_teto(casca):
    """Lança raios para cima a partir do teto novo, numa grade da sala: a casca do mapa (o forro do telhado, o
    tampo da caixa de vidro) tem de ficar pelo menos FOLGA_MINIMA_DA_CASCA acima dele"""
    from mathutils.bvhtree import BVHTree
    dg = bpy.context.evaluated_depsgraph_get()
    arvores = [BVHTree.FromObject(o, dg) for o in casca if o.type == 'MESH' and len(o.data.polygons)]
    pior = (9.0, None)
    for x in np.linspace(X_OESTE + 0.02, X_LESTE - 0.02, 45):
        for z in np.linspace(Z_TOPO_FRENTE + 0.05, Z_FUNDO - 0.02, 50):
            y = y_teto(z)
            origem = b_de((x, y - 0.30, z))
            for arv in arvores:
                loc, _, _, dist = arv.ray_cast(origem, Vector((0, 0, 1)), 2.0)
                if loc is not None and dist - 0.30 < pior[0]:
                    pior = (dist - 0.30, (x, z))
    print('  a casca mais baixa fica %.3f m acima do teto novo (em x %.2f, z %.2f)' % (pior[0], *(pior[1] or (0, 0))))
    if pior[0] < FOLGA_MINIMA_DA_CASCA:
        raise SystemExit('ERRO: a casca do mapa desce até o teto novo (folga %.3f m)' % pior[0])


def casca_para_previa(casca):
    """Materiais do mapa só com a cor da textura (sem luz), como o jogo mostra"""
    for o in casca:
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
            if bsdf.inputs['Base Color'].links:
                mult = nt.nodes.new('ShaderNodeMix')
                mult.data_type = 'RGBA'
                mult.blend_type = 'MULTIPLY'
                mult.inputs['Factor'].default_value = 1.0
                mult.inputs['B'].default_value = (0.92, 0.92, 0.92, 1)
                nt.links.new(bsdf.inputs['Base Color'].links[0].from_socket, mult.inputs['A'])
                nt.links.new(mult.outputs['Result'], em.inputs['Color'])
            else:
                em.inputs['Color'].default_value = bsdf.inputs['Base Color'].default_value
            nt.links.new(em.outputs[0], out.inputs['Surface'])
            try:
                m.blend_method = 'OPAQUE'
            except Exception:
                pass
            m['previa'] = 1


print('Assando a luz (Cycles, %d amostras)...' % AMOSTRAS)
OBJS = criar_objetos()
CASCA = importar_casca()
if NOITE:
    montar_luzes_da_noite()
else:
    montar_luzes_do_cycles()
assar(OBJS)
finalizar_cores(OBJS)


# ------------------------------------------------------------------
# 4b. O brilho do piso encerado: um reflexo falso, somado nas cores dos vértices do piso
# ------------------------------------------------------------------
PESO_DO_REFLEXO = 0.35
PESO_DAS_LUZES = 0.22         # o brilho embaixo de cada luminária
OLHO_LONGE = 7.0
OLHO_ALTO = 2.4
RAIOS_DO_REFLEXO = 8
CEU_REFLETIDO = (0.58, 0.58, 0.56)     # o que se vê pelas janelas (a praça e o céu, de dia)
if NOITE:
    CEU_REFLETIDO = (0.07, 0.07, 0.15)     # a noite lá fora


def refletir_no_piso(objs):
    from mathutils.bvhtree import BVHTree
    texturas = {'quadros': ATLAS, 'teto': TEXTURAS['teto']}
    V, T, C, U, X, lista = [], [], [], [], [], []
    base = 0
    for o in objs:
        chave = o['malha']
        if chave == 'piso':
            continue
        me = o.data
        me.calc_loop_triangles()
        n = len(me.loop_triangles)
        if n == 0:
            continue
        lp = np.zeros(n * 3, np.int64)
        vt = np.zeros(n * 3, np.int64)
        me.loop_triangles.foreach_get('loops', lp)
        me.loop_triangles.foreach_get('vertices', vt)
        co = np.zeros(len(me.vertices) * 3)
        me.vertices.foreach_get('co', co)
        col = ler_atributo(me, 'Col')
        uv = np.zeros((len(me.loops), 2))
        if me.uv_layers:
            a = np.zeros(len(me.loops) * 2)
            me.uv_layers[0].data.foreach_get('uv', a)
            uv = a.reshape(-1, 2)
        V.append(co.reshape(-1, 3))
        T.append(vt.reshape(-1, 3) + base)
        base += len(me.vertices)
        C.append(col[lp].reshape(-1, 3, 3))
        U.append(uv[lp].reshape(-1, 3, 2))
        if chave in texturas:
            lista.append(texturas[chave])
            X.append(np.full(n, len(lista) - 1))
        else:
            X.append(np.full(n, -1))
    V, T, C, U, X = (np.concatenate(a) for a in (V, T, C, U, X))
    bvh = BVHTree.FromPolygons(V.tolist(), T.tolist(), all_triangles=True)
    rng = np.random.default_rng(7)
    jitter = rng.normal(size=(RAIOS_DO_REFLEXO, 3)) * 0.025
    centro = np.array([698.2, -191.0])
    luzes = [(b[0], -b[2]) for b, _ in BOCAS + BOCAS_DE_PE]
    for o in objs:
        if o['malha'] != 'piso':
            continue
        me = o.data
        cantos = dados_dos_cantos(me)
        chave = np.round(cantos['co'] * 1000).astype(np.int64)
        uniq, inv = np.unique(chave, axis=0, return_inverse=True)
        inv = inv.ravel()
        pts = uniq / 1000.0
        alvos, onde, quem = [], [], []
        for k, p in enumerate(pts):
            d = centro - p[:2]
            dist = float(np.linalg.norm(d))
            olho = np.array([*(p[:2] + d * min(1.0, OLHO_LONGE / max(dist, 1e-6))), PISO_Y + OLHO_ALTO])
            i = p - olho
            r = unit(np.array([i[0], i[1], -i[2]]))
            origem = Vector((p[0], p[1], p[2] + 0.01))
            for j in jitter:
                rr = unit(r + j)
                loc, _, idx, _ = bvh.ray_cast(origem, Vector(rr), 60.0)
                if idx is not None:
                    alvos.append(idx)
                    onde.append(loc[:])
                    quem.append(k)
        refl = np.tile(np.array(CEU_REFLETIDO), (len(pts), 1)) * RAIOS_DO_REFLEXO
        if alvos:
            idx = np.array(alvos)
            loc = np.array(onde)
            quem = np.array(quem)
            a, b, c = V[T[idx, 0]], V[T[idx, 1]], V[T[idx, 2]]
            v0, v1, v2 = b - a, c - a, loc - a
            d00, d01, d11 = (v0 * v0).sum(1), (v0 * v1).sum(1), (v1 * v1).sum(1)
            d20, d21 = (v2 * v0).sum(1), (v2 * v1).sum(1)
            den = np.maximum(d00 * d11 - d01 * d01, 1e-12)
            w1 = (d11 * d20 - d01 * d21) / den
            w2 = (d00 * d21 - d01 * d20) / den
            w0 = 1 - w1 - w2
            pesos = np.stack([w0, w1, w2], 1)[..., None]
            cor = (C[idx] * pesos).sum(1)
            uv = (U[idx] * pesos).sum(1)
            for t, img in enumerate(lista):
                m = X[idx] == t
                if not m.any():
                    continue
                h, w = img.shape[:2]
                px = np.clip(((uv[m, 0] % 1.0) * w).astype(int), 0, w - 1)
                py = np.clip(((1.0 - uv[m, 1] % 1.0) * h).astype(int), 0, h - 1)
                cor[m] = cor[m] * img[py, px, :3]
            np.add.at(refl, quem, cor - np.array(CEU_REFLETIDO))
        refl /= RAIOS_DO_REFLEXO
        refl = alisar(me, refl[inv], passadas=1, cantos=cantos)
        refl = np.clip(refl, 0, 1) ** 1.6
        brilho = np.zeros(len(inv))
        for fx, fy in luzes:
            d2 = (cantos['co'][:, 0] - fx) ** 2 + (cantos['co'][:, 1] - fy) ** 2
            brilho += np.exp(-d2 / (2 * 0.6 ** 2))
        col = ler_atributo(me, 'Col')
        col = col + PESO_DO_REFLEXO * refl + PESO_DAS_LUZES * brilho[:, None] * np.array([1.0, 0.82, 0.60])
        col = np.clip(col, 0, 1)
        at = me.color_attributes['Col']
        at.data.foreach_set('color', np.concatenate([col, np.ones((len(col), 1))], axis=1).astype(np.float32).ravel())
        print('  reflexo no piso: %d vértices, %d raios acertaram; luz do piso: mediana %.2f, p90 %.2f' % (
            len(pts), len(alvos), np.median(col @ LUMA), np.percentile(col @ LUMA, 90)))


refletir_no_piso(OBJS)
print('  %.0f s' % (time.time() - INICIO))


# ------------------------------------------------------------------
# 5. Exportar o desenho e conferir
# ------------------------------------------------------------------
def ler_glb(caminho):
    d = open(caminho, 'rb').read()
    n = struct.unpack('<I', d[12:16])[0]
    return json.loads(d[20:20 + n]), d


def soldar(o):
    bm = bmesh.new()
    bm.from_mesh(o.data)
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=1e-5)
    bm.to_mesh(o.data)
    bm.free()


def media_de_col(objs):
    soma, n = np.zeros(3), 0
    for o in objs:
        c = ler_atributo(o.data, 'Col')
        soma += c.sum(0)
        n += len(c)
    return soma / max(n, 1)


def cores_no_glb(j, d):
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
        if me.attributes.get('suave') is not None:
            me.attributes.remove(me.attributes['suave'])
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
    print('  COLOR_0 lido do .glb: média %s; nos cantos do Blender: %s' % (np.round(lido, 3), np.round(esperado, 3)))
    if sem_cor:
        raise SystemExit('ERRO: há primitivas sem COLOR_0')
    if maior >= 65536:
        raise SystemExit('ERRO: uma malha passou de 65535 vértices')
    if any(m.get('mimeType') != 'image/png' for m in j.get('images', [])):
        raise SystemExit('ERRO: há textura que não é PNG')
    return dict(triangulos=tris, materiais=len(j['materials']), texturas=len(imgs), bytes=os.path.getsize(caminho),
                primitivas=sum(len(m['primitives']) for m in j['meshes']), maior_primitiva=maior)


if NOITE:
    # de noite, o desenho assado vai para a pasta temporária; o _noite.glb é o .glb de dia com estas cores (9.)
    CONTAGENS = exportar_visual(OBJS, os.path.join(PASTA_TEMP, 'noite_assada.glb'))
else:
    CONTAGENS = exportar_visual(OBJS)


# ------------------------------------------------------------------
# 6. A colisão
# ------------------------------------------------------------------
def tri(a, b, c):
    COLISAO.append((tuple(map(float, a)), tuple(map(float, b)), tuple(map(float, c))))


def quad_col(a, b, c, d):
    tri(a, b, c)
    tri(a, c, d)


def parede_col(p, q, y0, y1):
    """Uma parede em pé de p a q (x, z), de y0 a y1 (absolutos; y1 pode ser uma função de (x, z))"""
    f = y1 if callable(y1) else (lambda x, z: y1)
    quad_col((p[0], y0, p[1]), (q[0], y0, q[1]), (q[0], f(*q), q[1]), (p[0], f(*p), p[1]))


def montar_colisao():
    # o piso: a sala, a soleira e uma aba que desce à praça
    for pc in PECAS_DO_PISO:
        p0 = pc[0]
        for k in range(1, len(pc) - 1):
            tri((p0[0], PISO_Y, p0[1]), (pc[k][0], PISO_Y, pc[k][1]), (pc[k + 1][0], PISO_Y, pc[k + 1][1]))
    x0, x1 = ENTRADA_X[0] - 0.3, ENTRADA_X[1] + 0.3
    quad_col((x0, Y_DA_PRACA - 0.01, Z_DA_SOLEIRA - 1.2), (x1, Y_DA_PRACA - 0.01, Z_DA_SOLEIRA - 1.2), (x1, PISO_Y, Z_DA_SOLEIRA),
             (x0, PISO_Y, Z_DA_SOLEIRA))
    # as paredes, do piso até o teto (a da frente é inclinada como a de dentro), com a entrada aberta
    y0 = PISO_Y - 0.05
    topo = lambda x, z: y_teto(z)

    def frente(x0_, x1_, ya, yb):
        quad_col((x0_, ya, z_frente(ya)), (x1_, ya, z_frente(ya)), (x1_, yb, z_frente(yb)), (x0_, yb, z_frente(yb)))
    frente(X_OESTE, ENTRADA_X[0], y0, Y_TOPO_FRENTE)
    frente(ENTRADA_X[1], X_LESTE, y0, Y_TOPO_FRENTE)
    frente(ENTRADA_X[0], ENTRADA_X[1], ENTRADA_TOPO, Y_TOPO_FRENTE)
    # os batentes da entrada, da nossa parede até a casca
    for x in ENTRADA_X:
        zf = z_frente(PISO_Y)
        quad_col((x, y0, zf), (x, y0, Z_DA_SOLEIRA - 0.3), (x, ENTRADA_TOPO, Z_DA_SOLEIRA - 0.3), (x, ENTRADA_TOPO, zf))
    for (x, nome) in ((X_OESTE, 'oeste'), (X_LESTE, 'leste')):
        # do pé inclinado da frente aos fundos
        za = z_frente(PISO_Y)
        quad_col((x, y0, za), (x, y0, Z_FUNDO), (x, y_teto(Z_FUNDO), Z_FUNDO), (x, Y_TOPO_FRENTE, Z_TOPO_FRENTE))
    parede_col((X_OESTE, Z_FUNDO), (X_LESTE, Z_FUNDO), y0, topo)
    # os móveis: caixas de paredes em pé, sem tampa
    for nome, pts, altura in OBSTACULOS:
        n = len(pts)
        for k in range(n):
            parede_col(pts[k], pts[(k + 1) % n], y0, PISO_Y + altura)
    me = bpy.data.meshes.new('cozy_cone_recepcao_colisao')
    verts = []
    faces = []
    for t in COLISAO:
        base = len(verts)
        verts += [b_de(p) for p in t]
        faces.append((base, base + 1, base + 2))
    me.from_pydata(verts, [], faces)
    o = bpy.data.objects.new('cozy_cone_recepcao_colisao', me)
    bpy.context.scene.collection.objects.link(o)
    bpy.ops.object.select_all(action='DESELECT')
    o.select_set(True)
    bpy.context.view_layer.objects.active = o
    caminho = SAIDA + '_colisao.glb'
    bpy.ops.export_scene.gltf(filepath=caminho, export_format='GLB', use_selection=True, export_yup=True,
                              export_apply=True, export_animations=False, export_normals=False, export_texcoords=False,
                              export_vertex_color='NONE', export_materials='NONE')
    # confere: só o piso deitado; o resto em pé (|normal.y| <= 0,6)
    deitados = 0
    for a, b, c in COLISAO:
        n = np.cross(np.subtract(b, a), np.subtract(c, a))
        if np.linalg.norm(n) > 0 and abs(n[1]) / np.linalg.norm(n) > 0.6:
            deitados += 1
            if max(a[1], b[1], c[1]) > PISO_Y + 0.01:
                raise SystemExit('ERRO: há uma face deitada na colisão acima do piso')
    print('Exportado %s: %d triângulos (%d do piso)' % (caminho, len(COLISAO), deitados))
    o.hide_render = True
    return len(COLISAO)


def conferir_caminho():
    """O corredor da entrada até a Sally não pode cruzar nenhum obstáculo"""
    x0, x1 = CAMINHO_X
    z0, z1 = Z_DA_SOLEIRA, LUGAR_DA_SALLY[1] + COMPRIMENTO_DA_SALLY / 2
    for nome, pts, _ in OBSTACULOS:
        xs = [p[0] for p in pts]
        zs = [p[1] for p in pts]
        if max(xs) > x0 and min(xs) < x1 and max(zs) > z0 and min(zs) < z1:
            raise SystemExit('ERRO: o obstáculo %s fica no caminho da Sally' % nome)
    print('  o caminho da entrada até a Sally (x %.2f-%.2f, %.1f m) está livre' % (x0, x1, x1 - x0))


if not NOITE:     # de noite, a colisão e o json de dia ficam como estão
    TRIANGULOS_DA_COLISAO = montar_colisao()
    conferir_caminho()


# ------------------------------------------------------------------
# 7. O JSON para a integração
# ------------------------------------------------------------------
def montar_json():
    xc = (ENTRADA_X[0] + ENTRADA_X[1]) / 2
    x, z, comp, larg = LUGAR_DA_SALLY
    aberturas = []
    for nome, desc, mn, mx, faces, objs, sai in ABERTURAS_DA_CASCA:
        aberturas.append(dict(nome=nome, descricao=desc, caixa_min=list(mn), caixa_max=list(mx), faces=faces, objetos=objs,
                              sai_da_colisao=sai))
    for r in RECORTES_DA_CASCA:
        nome, desc, _, _, _, prof = r
        aberturas.append(dict(nome=nome, descricao=desc, recorte=dict(cantos=cantos_do_recorte(r), profundidade=prof),
                              objetos=['Object_16'], sai_da_colisao=False))
    dados = dict(
        sobre='Recepção (escritório) do Cozy Cone Motel por dentro (ferramentas/montar_recepcao_do_cozy_cone.py). '
              'Eixos do jogo: x, y para cima, z. Rumo do jogo: 0 olhando para -z, crescendo para a direita '
              '(a frente é (sin r, -cos r)).',
        piso_y=PISO_Y,
        teto_y=round(Y_TOPO_FRENTE, 3),
        teto_y_fundo=round(Y_TETO_FUNDO, 3),
        nota_do_teto='o teto é inclinado como o telhado: y = %.3f - %.4f (z - %.2f), de y %.2f na frente a %.2f nos fundos' % (
            TETO_A, TETO_K, TETO_Z0, Y_TOPO_FRENTE, Y_TETO_FUNDO),
        porta=dict(
            centro_na_fachada=[round(xc, 3), PISO_Y, Z_DA_SOLEIRA],
            centro_por_dentro=[round(xc, 3), PISO_Y, round(Z_FRENTE_NO_PISO, 3)],
            direcao_para_dentro=[0.0, 0.0, 1.0],
            rumo_para_dentro=round(math.pi, 4),
            rumo_para_fora=0.0,
            ao_longo=[1.0, 0.0, 0.0],
            largura=round(ENTRADA_X[1] - ENTRADA_X[0], 3),
            largura_livre_no_chao=6.42,
            altura=round(ENTRADA_TOPO - PISO_Y, 3),
            pontas_na_fachada=[[ENTRADA_X[0], Z_DA_SOLEIRA], [ENTRADA_X[1], Z_DA_SOLEIRA]],
            profundidade_do_rebaixo=round(Z_FRENTE_NO_PISO - Z_DA_SOLEIRA, 3),
            nota='a entrada do meio já é aberta na casca (x 694,40-702,03, verga em y 5,96); as sapatas dos pilares '
                 '(y 2,0-2,1) deixam 6,42 m livres junto ao chão (x 695,02-701,44); o piso de dentro emenda com a praça '
                 '(3 cm acima dela, com a borda chanfrada)',
        ),
        lugares=dict(
            sally=dict(x=x, y=PISO_Y, z=z, rumo=0.0, comprimento=comp, largura=larg,
                       nota='atrás do balcão (ao lado da ponta redonda dele), na frente da estante, de frente para a entrada'),
        ),
        caminho_da_entrada=dict(
            pontos=[[round(xc, 3), PISO_Y, round(Z_DA_SOLEIRA - 1.0, 3)], [round(xc, 3), PISO_Y, round(Z_FRENTE_NO_PISO + 2.0, 3)],
                    [x, PISO_Y, round(z - comp / 2 - 1.5, 3)], [x, PISO_Y, z]],
            largura=LARGURA_DO_CAMINHO,
            faixa_livre_x=list(CAMINHO_X),
            nota='o corredor x %.2f-%.2f fica livre de móveis da soleira até o fundo da vaga (z %.2f); o balcão começa '
                 'em x %.2f' % (CAMINHO_X[0], CAMINHO_X[1], z + COMPRIMENTO_DA_SALLY / 2, BALCAO_X[0] - RAIO_DA_PONTA),
        ),
        contorno_por_dentro=[[X_OESTE, round(Z_FRENTE_NO_PISO, 3)], [X_LESTE, round(Z_FRENTE_NO_PISO, 3)], [X_LESTE, Z_FUNDO],
                             [X_OESTE, Z_FUNDO]],
        aberturas_para_tirar_da_casca=aberturas,
        nota_das_aberturas='As com caixa: peças inteiras da casca dentro dela (eixos do jogo; "faces" = quantas faces saem; '
                           'conferidas com testar_aberturas.py no Race-O-Rama original x4,5); nenhuma caixa contém peça '
                           'inteira de outro objeto além do de "objetos" (que é só informação). As que saem da colisão: o '
                           'piso velho (y 2,11, acima do novo), o balcão, os armários e as hastes do Race-O-Rama (dentro da '
                           'sala nova) e o fundo da caixa de vidro. Os vidros saem só do desenho (a colisão deles fica, por '
                           'fora da colisão daqui): os dos lados por caixa; os da frente (inclinados) e o dos fundos por '
                           'RECORTE, só no vão de cada vitrine, bandeira e janela, porque a caixa deles também conteria as '
                           'peças da moldura do Object_9 que ficam na frente do vidro (o painel verde, o rodapé, o peitoril, '
                           'os trilhos). Os cantos de cada recorte ficam num plano vertical no meio do vidro; a faixa de '
                           '±(profundidade + 0,3) m em volta dele só contém o vidro (conferido com as funções recortar() e '
                           'tirar_pecas() do integrador no mapa: só o Object_16 perde área, e a do vão). A ordem importa '
                           '(as vitrines antes das bandeiras; os vidros dos lados antes dos recortes). O tampo da caixa de '
                           'vidro (Object_16, y 7,72-6,27) fica no mapa: ele fica 5 cm ou mais acima do teto novo e a '
                           'câmera para nele.',
        colisao=dict(arquivo=os.path.basename(SAIDA) + '_colisao.glb', triangulos=TRIANGULOS_DA_COLISAO,
                     nota='piso em y = piso_y (com uma aba que desce à praça na entrada) e paredes em pé até o teto '
                          '(a da frente, inclinada, com a entrada aberta); os móveis em caixas sem tampa de %.1f m ou '
                          'até o alto deles (os cactos, os abajures de pé, a estante, as pilhas de cones: a câmera do '
                          'jogo, aqui dentro, anda em y ~3,9, 2 m acima do carro, e não atravessa a colisão); sem teto '
                          '(o tampo da caixa de vidro do mapa faz o teto da câmera)' % ALTURA_DOS_MOVEIS),
        contagens=CONTAGENS,
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


def sally_na_vaga():
    """A Sally (o modelo do jogo, só a cor da textura) no lugar dela, para as prévias"""
    antes = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=os.path.join(PASTA_DOS_CARROS, 'sally.glb'))
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
    esc = COMPRIMENTO_DA_SALLY / (mx.y - mn.y)
    raiz = bpy.data.objects.new('sally', None)
    bpy.context.scene.collection.objects.link(raiz)
    meio = Vector(((mn.x + mx.x) / 2, (mn.y + mx.y) / 2, mn.z))
    for o in novos:
        if o.parent is None:
            o.parent = raiz
            o.location = (o.location - meio) * esc
            o.scale = o.scale * esc
    x, z = LUGAR_DA_SALLY[:2]
    raiz.location = b_de((x, PISO_Y, z))
    # o modelo tem a frente para -y do Blender (= +z do jogo): gira meia volta para olhar para a entrada (-z)
    raiz.rotation_euler = (0, 0, math.pi)
    casca_para_previa([o for o in novos if o.type == 'MESH'])
    return raiz


def fazer_previas(objs):
    os.makedirs(PREVIAS, exist_ok=True)
    trocar_materiais(objs, material_de_previa)
    casca_para_previa(CASCA)
    sc = bpy.context.scene
    for o in list(sc.objects):
        if o.type == 'LIGHT':
            o.hide_render = True
    sc.world.node_tree.nodes['Background'].inputs[0].default_value = (0.55, 0.72, 0.95, 1)
    sc.world.node_tree.nodes['Background'].inputs[1].default_value = 1.0
    try:
        sally_na_vaga()
    except Exception as e:
        print('  sem a Sally nas prévias (%s)' % e)
    P = PISO_Y
    vistas = [
        ('jogo_chegando', (698.2, P + 3.4, 175.0), (698.2, P + 1.0, 192.0), 16),
        ('jogo_dentro', (697.6, P + 3.4, 184.5), (698.6, P + 1.0, 201.0), 16),
        ('jogo_saindo', (699.0, P + 3.2, 201.5), (697.8, P + 1.0, 184.0), 16),
        ('jogo_balcao', (692.5, P + 3.4, 187.0), (704.0, P + 1.0, 196.0), 16),
        ('filme', (704.6, P + 1.55, 188.4), (698.4, P + 1.75, 201.0), 20),
        ('balcao_perto', (702.4, P + 1.75, 194.2), (704.6, P + 1.15, 196.6), 24),
        ('entrando', (698.2, P + 2.4, 181.8), (698.6, P + 1.5, 198.0), 16),
        ('balcao', (702.6, P + 1.5, 191.6), (703.6, P + 0.9, 197.0), 22),
        ('estante', (698.6, P + 2.1, 192.5), (698.6, P + 2.2, 203.5), 18),
        ('oeste', (701.5, P + 2.3, 190.5), (687.2, P + 2.6, 191.5), 16),
        ('leste', (694.5, P + 2.3, 191.0), (709.4, P + 2.6, 190.5), 16),
        ('de_dentro_para_fora', (698.0, P + 2.4, 199.0), (698.0, P + 2.5, 180.0), 16),
        ('canto_dos_cones', (695.5, P + 2.0, 194.0), (689.0, P + 1.2, 202.5), 18),
        ('fora_entrada', (698.2, P + 2.6, 170.5), (698.2, P + 1.9, 192.0), 22),
        ('fora_longe', (698.2, P + 4.2, 160.0), (698.2, P + 2.6, 186.0), 24),
        ('fora_vitrine_leste', (705.6, P + 2.4, 170.5), (705.8, P + 2.9, 181.0), 24),
        ('fora_vitrine_oeste', (690.2, P + 2.4, 171.0), (690.6, P + 2.4, 182.0), 24),
        ('fora_lado_oeste', (676.0, P + 2.8, 192.0), (690.0, P + 2.4, 192.0), 24),
        ('fora_fundos', (698.6, P + 2.4, 216.0), (698.6, P + 2.4, 203.0), 24),
        ('fora_office', (690.8, P + 2.6, 172.5), (690.8, P + 3.0, 180.0), 30),
        ('parede_leste', (700.5, P + 2.0, 188.5), (709.4, P + 2.3, 191.2), 18),
        ('parede_oeste', (696.0, P + 2.0, 194.5), (687.2, P + 2.4, 191.0), 18),
    ]
    for nome, pos, alvo, lente in vistas:
        co = camera_em(pos, alvo, lente=lente, nome='cam_' + nome)
        renderizar_eevee(os.path.join(PREVIAS, nome + '.png'), 1280, 720)
        bpy.data.objects.remove(co, do_unlink=True)
        print('  prévia %s' % nome)
    # de cima, a planta (cortada abaixo do teto), com a vaga da Sally e o caminho marcados
    x, z, comp, larg = LUGAR_DA_SALLY
    for nome, pts, cor in (('vaga', [(x - larg / 2, z - comp / 2), (x + larg / 2, z - comp / 2), (x + larg / 2, z + comp / 2),
                                     (x - larg / 2, z + comp / 2)], (0.1, 0.9, 0.2, 1)),
                           ('caminho', [(CAMINHO_X[0], Z_DA_SOLEIRA), (CAMINHO_X[1], Z_DA_SOLEIRA), (CAMINHO_X[1], z - comp / 2),
                                        (CAMINHO_X[0], z - comp / 2)], (0.1, 0.4, 1.0, 1))):
        me = bpy.data.meshes.new('marca_' + nome)
        me.from_pydata([b_de((p[0], PISO_Y + 0.03, p[1])) for p in pts], [], [(0, 1, 2, 3)])
        o = bpy.data.objects.new(me.name, me)
        sc.collection.objects.link(o)
        mat = novo_material('marca_' + nome)
        nt = mat.node_tree
        nt.nodes.clear()
        em = nt.nodes.new('ShaderNodeEmission')
        em.inputs['Color'].default_value = cor
        nt.links.new(em.outputs[0], nt.nodes.new('ShaderNodeOutputMaterial').inputs['Surface'])
        me.materials.append(mat)
        o.hide_render = True
        o['marca'] = 1
    cam = bpy.data.cameras.new('cima')
    cam.type = 'ORTHO'
    cam.ortho_scale = 27
    cam.clip_start = 1
    cam.clip_end = 200
    co = bpy.data.objects.new('cima', cam)
    sc.collection.objects.link(co)
    co.location = b_de(((X_OESTE + X_LESTE) / 2, PISO_Y + 4.0, (Z_FRENTE_NO_PISO + Z_FUNDO) / 2 - 0.5))
    co.rotation_euler = (0, 0, 0)
    sc.camera = co
    renderizar_eevee(os.path.join(PREVIAS, 'planta.png'), 1100, 1100)
    for o in sc.objects:
        if o.get('marca'):
            o.hide_render = False
    renderizar_eevee(os.path.join(PREVIAS, 'planta_caminho.png'), 1100, 1100)
    salvar_png(ATLAS, os.path.join(PREVIAS, 'atlas_dos_quadros.png'))
    print('  prévia planta')


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
BASE_DA_NOITE_DO_JOGO = (0.30, 0.32, 0.50)
CEU_DA_NOITE_DO_JOGO = (0.20, 0.22, 0.42)
LUAR_DO_JOGO = (0.22, 0.26, 0.44)
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
    """As luzes que a recepção joga para fora de noite: a entrada, as duas vitrines da frente (a do letreiro
    OFFICE, laranja), a janela da frente de cada lado e os fundos (cada uma um pouco para fora dos vãos)"""
    quente = (1.0, 0.70, 0.42)
    xc = (ENTRADA_X[0] + ENTRADA_X[1]) / 2
    z_fora = z_frente(PISO_Y) - 2.2          # a frente é inclinada: 2,2 m à frente do pé da nossa parede
    return [
        ((xc, PISO_Y + 1.8, z_fora), quente, 11.0, 1.4),
        ((NEON_CENTRO[0], PISO_Y + 2.2, z_fora), (1.0, 0.48, 0.18), 9.0, 1.6),
        (((702.60 + X_LESTE) / 2, PISO_Y + 2.2, z_fora), quente, 8.0, 1.0),
        ((X_OESTE - 2.4, PISO_Y + 1.8, 184.0), quente, 8.5, 0.9),       # a janela da frente de cada lado
        ((X_LESTE + 2.4, PISO_Y + 1.8, 184.0), quente, 8.5, 0.9),
        (((X_OESTE + X_LESTE) / 2, PISO_Y + 2.2, Z_FUNDO + 1.9), quente, 11.0, 0.8),
    ]


if NOITE:
    transplantar_cores(SAIDA + '.glb', os.path.join(PASTA_TEMP, 'noite_assada.glb'), SAIDA + '_noite.glb')
    conferir_noite(SAIDA + '.glb', SAIDA + '_noite.glb')
    LUZES_DE_FORA = luzes_de_fora()
    gravar_luzes_de_fora(LUZES_DE_FORA)
    if PREVIAS:
        print('Prévias (dia | noite) em %s...' % PREVIAS)
        casca_para_previa(CASCA)
        P = PISO_Y
        previas_dia_e_noite([
            ('jogo_chegando', (698.2, P + 3.4, 175.0), (698.2, P + 1.0, 192.0), 16),
            ('jogo_dentro', (697.6, P + 3.4, 184.5), (698.6, P + 1.0, 201.0), 16),
            ('filme', (704.6, P + 1.55, 188.4), (698.4, P + 1.75, 201.0), 20),
            ('estante', (698.6, P + 2.1, 192.5), (698.6, P + 2.2, 203.5), 18),
            ('oeste', (701.5, P + 2.3, 190.5), (687.2, P + 2.6, 191.5), 16),
            ('leste', (694.5, P + 2.3, 191.0), (709.4, P + 2.6, 190.5), 16),
            ('de_dentro_para_fora', (698.0, P + 2.4, 199.0), (698.0, P + 2.5, 180.0), 16),
            ('fora_entrada', (698.2, P + 2.6, 170.5), (698.2, P + 1.9, 192.0), 22),
            ('fora_longe', (698.2, P + 4.2, 160.0), (698.2, P + 2.6, 186.0), 24),
            ('fora_office', (690.8, P + 2.6, 172.5), (690.8, P + 3.0, 180.0), 30),
            ('fora_lado_oeste', (676.0, P + 2.8, 192.0), (690.0, P + 2.4, 192.0), 24),
            ('fora_fundos', (698.6, P + 2.4, 216.0), (698.6, P + 2.4, 203.0), 24),
        ], CASCA, LUZES_DE_FORA, OBJS)

shutil.rmtree(PASTA_TEMP, ignore_errors=True)
print('Pronto em %.0f s.' % (time.time() - INICIO))
