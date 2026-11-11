"""
montar_luzes_da_cidade.py
------------------------------------------------------------------
Monta as luzes de noite do resto de Radiator Springs (fora dos letreiros do
Ramone, do Flo, do Luigi, do Curios, do Sarge, do Fillmore e do Cozy Cone, que
têm as ferramentas deles): o que acende quando a cidade escurece, desenhado
por cima do mapa do mundo aberto (recursos/modelos/radiator_springs.glb), já
nas coordenadas do mundo do jogo (x, y para cima, z):

    Blender -b --factory-startup -P ferramentas/luzes/montar_luzes_da_cidade.py -- \\
        recursos/modelos/radiator_springs.glb recursos/modelos/luzes

Grava, na pasta dada no segundo argumento:
  cidade.glb   só o que brilha: os tubos de neon, as janelas e as placas acesas,
               as lâmpadas. Sem textura: a cor acesa de cada vértice vai em
               COLOR_0 (RGB, na tela, 0 a 1) e o A diz a ordem de acender
               (0 primeiro, 1 por último);
  cidade.json  o nome, quando a onda começa (acender_em, em segundos) e as luzes
               pontuais que caem no chão e nas paredes (x, y, z do jogo, cor,
               raio em metros e força).

O que o mapa tem (e o que não tem):
  - Postes de rua e semáforo: o mapa do Race-O-Rama não tem. Nenhum poste ao
    longo da rua e nada pendurado sobre o cruzamento (x ~505, onde a estrada de
    terra cruza a rua principal): a ferramenta não inventa a estrutura e não
    grava semaforo.glb. A luz da rua vem das vitrines, dos neons e das
    luminárias que existem (a do quartel, a da clínica, a da placa do Mate, as
    duas que iluminam a placa do quartel-general da Rust-eze).
  - Letreiros de neon das lojas da rua oeste (x 321 a 446): a camada de tubos
    do mapa (ror_Object_86, textura ror_Image_56, por cima das placas
    ror_Object_84/85) vira malha de verdade: cada texel do tubo (alfa > 0,45)
    vira um pedacinho de placa, juntados em retângulos da mesma cor, 5 cm à
    frente da placa. A cor sai da textura, puxada para a cor de neon mais
    perto (paleta abaixo).
  - Vitrines das lojas da rua oeste (ror_Object_70, o vidro cinza liso): uma
    placa quente 3 cm à frente de cada vidro, um pouco mais clara embaixo
    (a luz de dentro), umas apagadas.
  - A clínica do Doc: o letreiro ORNAMENT VALLEY MECHANICAL CLINIC e o DOCTOR
    HUDSON (as letras pintadas, que são escuras, acendem em branco frio; os
    emblemas em âmbar), os vidros verdes da frente, as escotilhas da porta e a
    luminária da porta do lado.
  - O quartel do Ruivo: os vidros das janelas de cima (os caixilhos brancos
    ficam apagados), a janela redonda, a faixa de vidros das três portas e a
    luminária pendurada embaixo da marquise da porta do meio.
  - A estátua do Stanley: as letras douradas da placa e a moldura.
  - Tow Mater Towing & Salvage: as letras TOW, MATER e TOWING & SALVAGE e a
    lâmpada embaixo da cúpula em cima da placa.
  - Mais para leste: as letras da placa do museu (Racing MUSEUM e ENTER), as
    janelas de caixilho amarelo das três casas, a vitrine, os tijolos de vidro,
    o LIGHTNING 95 McQUEEN, a placa redonda da Rust-eze e os dois refletores
    dela no quartel-general, e o cinema drive-in (o nome e o DRIVE-IN THEATER
    em creme, o painel de letras branco, mais fraco, com as letras vermelhas
    do filme apagadas por cima).
  Tribunal e prefeitura: o único prédio cívico do mapa é o do quartel (com a
  estátua do Stanley na frente), já aceso acima; o xerife não tem prédio (o
  lugar dele é atrás do outdoor da entrada leste).

Tudo sai num material branco só (baseColorFactor 1, 1, 1, 1), sem textura e sem
normais; cada primitiva com até 60.000 vértices (a raylib usa índices de 16
bits).

A ordem de acender (o A de COLOR_0): uma onda que sai do cruzamento (x ~505,
o meio da cidade) e corre para os dois lados. As janelas acendem antes e cada
uma no seu tempo (um sorteio por janela); os neons e as placas, um pouco
depois e mais em ordem; as luminárias, primeiro.

Eixos: no Blender, x = x do jogo, y = -z do jogo e z = y do jogo; o .glb sai
com +Y para cima, ou seja, nas coordenadas do jogo.
------------------------------------------------------------------
"""

import bpy
import sys
import os
import json
import math
import struct
import numpy as np

argv = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
if len(argv) < 2:
    raise SystemExit('uso: Blender -b --factory-startup -P montar_luzes_da_cidade.py -- mapa.glb pasta_de_saida')
ARQ_MAPA, PASTA = argv[0], argv[1]
os.makedirs(PASTA, exist_ok=True)

# ------------------------------------------------------------------
# Constantes
# ------------------------------------------------------------------
CENTRO_DA_ONDA = 505.0       # x do cruzamento: a onda de acender sai daqui
ALCANCE_DA_ONDA = 740.0      # metros até onde a onda chega ao fim (A = 1)
ACENDER_EM = 2.0             # segundos: quando a onda começa (o jogo espalha o A a partir daí)
MAX_VERTICES = 60000         # por primitiva (a raylib usa índices de 16 bits)
RETANGULOS_POR_OBJETO = MAX_VERTICES // 4
MAX_LUZES = 30               # luzes pontuais no JSON
SEMENTE = 1906               # o sorteio das janelas apagadas e das cores (sempre o mesmo resultado)
BRILHO_DA_JANELA = 0.66      # as janelas brilham menos que os neons (a cor acesa delas vai vezes isso)

LUMA = np.array([0.299, 0.587, 0.114])

# Cores acesas (na tela, 0 a 1)
AMBAR = (1.00, 0.72, 0.36)
CREME = (1.00, 0.86, 0.60)
LARANJA_QUENTE = (1.00, 0.64, 0.30)
BRANCO_QUENTE = (1.00, 0.90, 0.70)
QUENTES = [AMBAR, CREME, LARANJA_QUENTE, BRANCO_QUENTE, AMBAR, CREME]
BRANCO_FRIO = (0.82, 0.94, 1.00)
VERDE_CLINICA = (0.55, 0.95, 0.84)
OURO = (1.00, 0.80, 0.38)
LAMPADA = (1.00, 0.88, 0.62)
AMARELO_RUSTEZE = (1.00, 0.80, 0.26)
BRANCO_LETREIRO = (0.96, 0.97, 1.00)
LARANJA_95 = (1.00, 0.58, 0.16)

# As cores de neon dos tubos (cada texel vai para a mais perto, pela cromaticidade)
NEON = [
    (0.30, 0.95, 1.00),   # ciano
    (0.25, 0.55, 1.00),   # azul
    (0.40, 1.00, 0.35),   # verde
    (0.80, 1.00, 0.30),   # verde-amarelo
    (1.00, 0.92, 0.35),   # amarelo
    (1.00, 0.55, 0.15),   # laranja
    (1.00, 0.22, 0.18),   # vermelho
    (1.00, 0.38, 0.78),   # rosa
    (0.72, 0.42, 1.00),   # lilás
    (1.00, 0.97, 0.92),   # o miolo branco do tubo
]


def B(x, y, z):
    """Do jogo para o Blender"""
    return np.array([x, -z, y], dtype=np.float64)


def J(p):
    """Do Blender para o jogo"""
    return (float(p[0]), float(p[2]), float(-p[1]))


# ------------------------------------------------------------------
# As máscaras: que texels acendem (rgba em tela, N x 4)
# ------------------------------------------------------------------
def luma(c):
    return c[:, :3] @ LUMA


def m_alfa(c):
    return (c[:, 3] > 0.45) & (luma(c) > 0.12)


def m_escuro(limite):
    return lambda c: luma(c) < limite


def m_claro(limite):
    return lambda c: luma(c) > limite


def m_vidro_verde(c):
    return ((c[:, 1] - c[:, 0]) > 0.035) & (luma(c) < 0.86)


def m_colorido(c):
    """O emblema da clínica (laranja e marrom no meio do cinza)"""
    return ((c[:, 0] - c[:, 2]) > 0.16) & (luma(c) > 0.18)


def m_vidro_do_quartel(c):
    """Os vidros cinza-azulados; os caixilhos são quase brancos"""
    return luma(c) < 0.70


def m_vidro_da_porta(c):
    return ((c[:, 2] - c[:, 0]) > 0.015) & (luma(c) < 0.78)


def m_dourado(c):
    return ((c[:, 0] - c[:, 2]) > 0.12) & (luma(c) > 0.40)


def m_amarelo(c):
    return (c[:, 0] > 0.55) & (c[:, 1] > 0.40) & ((c[:, 0] - c[:, 2]) > 0.25)


def m_letras_do_qg(c):
    """LIGHTNING McQUEEN (branco) e o 95 (amarelo-laranja) na faixa vermelha"""
    branco = (luma(c) > 0.72) & ((c[:, 0] - c[:, 2]) < 0.15)
    laranja = (c[:, 0] > 0.75) & (c[:, 1] > 0.35) & (c[:, 2] < 0.35)
    return branco | laranja


def m_painel_do_cinema(c):
    return luma(c) > 0.66


def m_letras_do_museu(c):
    """As letras rosa-claras em cima do vermelho (o vermelho tem pouco verde e pouco azul)"""
    return (c[:, 1] > 0.42) & (c[:, 2] > 0.36)


def m_tijolo_de_vidro(c):
    return luma(c) > 0.30


# ------------------------------------------------------------------
# As fontes: o que acende, onde e de que cor
#   modo 'mascara': os texels da textura (dentro dos retângulos, se houver) que passam na máscara
#   modo 'face':    a face inteira (o vidro liso)
#   modo 'baixo':   as faces viradas para baixo dentro de uma caixa (a lâmpada embaixo de uma cúpula)
#   modo 'frente':  as faces viradas para um ponto dentro de uma caixa (os refletores)
# retângulos em texels, contados de cima para baixo: (x0, y0, x1, y1)
# caixa: (x0, x1, z0, z1) do jogo, onde fica o centro dos triângulos; caixa3d: (min, max) do jogo
# ------------------------------------------------------------------
FONTES = [
    dict(nome='neons da rua oeste', tipo='neon', obj='ror_Object_86', modo='mascara', mascara=m_alfa,
         paleta=NEON, caixa=(300, 460, 100, 165), afastar=0.05, area_min=0.8, uv_min=8, sobre=1, luz=True),
    dict(nome='vitrines da rua oeste', tipo='janela', obj='ror_Object_70', modo='face', caixa=(320, 460, 95, 160),
         cores=QUENTES, apagadas=0.22, afastar=0.03, area_min=4.0, luz=True),
    dict(nome='letreiro da clínica', tipo='placa', obj='ror_Object_3', modo='mascara', mascara=m_escuro(0.46),
         retangulos=[(99, 179, 255, 193), (24, 155, 97, 176)], cor=BRANCO_FRIO, caixa=(605, 645, 75, 120),
         afastar=0.04, sobre=4, luz=True),
    dict(nome='emblemas da clínica', tipo='placa', obj='ror_Object_3', modo='mascara', mascara=m_colorido,
         retangulos=[(164, 174, 184, 198), (6, 152, 22, 178)], cor=AMBAR, caixa=(605, 645, 75, 120),
         afastar=0.045, sobre=4),
    dict(nome='vidros da clínica', tipo='janela', obj='ror_Object_3', modo='mascara', mascara=m_vidro_verde,
         retangulos=[(82, 200, 152, 256), (218, 200, 251, 256), (6, 196, 36, 224)], cores=[VERDE_CLINICA],
         apagadas=0.0, brilho=0.5, caixa=(605, 645, 75, 120), afastar=0.03, sobre=2, luz=True),
    dict(nome='luminária da clínica', tipo='lampada', obj='ror_Object_3', modo='baixo',
         caixa3d=((636.5, 8.3, 88.8), (638.3, 8.95, 90.5)), cor=LAMPADA, afastar=0.04, luz=True),
    dict(nome='janelas do quartel', tipo='janela', obj='ror_Object_5', modo='mascara', mascara=m_vidro_do_quartel,
         retangulos=[(0, 0, 256, 128)], cores=[AMBAR, CREME, LARANJA_QUENTE], apagadas=0.18,
         caixa=(220, 270, 112, 155), afastar=0.03, sobre=1, so_em_pe=True, luz=True),
    dict(nome='vidros das portas do quartel', tipo='janela', obj='ror_Object_7', modo='mascara',
         mascara=m_vidro_da_porta, retangulos=[(5, 66, 41, 91), (46, 66, 82, 91), (86, 66, 122, 91)],
         cores=[BRANCO_QUENTE], apagadas=0.0, caixa=(255, 270, 112, 155), afastar=0.03, sobre=2),
    dict(nome='luminária do quartel', tipo='lampada', obj='ror_Object_6', modo='baixo',
         caixa3d=((262.4, 6.6, 132.6), (264.5, 7.8, 134.3)), cor=LAMPADA, afastar=0.04, luz=True),
    dict(nome='placa do Stanley', tipo='placa', obj='ror_Object_2', modo='mascara', mascara=m_dourado,
         retangulos=[(46, 23, 94, 80)], cor=OURO, caixa=(275, 290, 125, 140), afastar=0.03, sobre=4, luz=True,
         peso_minimo=12.0, luz_afastar=3.0, luz_altura=4.5),
    dict(nome='letreiro do Mate', tipo='placa', obj='ror_Object_73', modo='mascara', mascara=m_claro(0.70),
         retangulos=[(333, 253, 548, 322), (343, 320, 461, 372), (206, 410, 461, 443)], cor=BRANCO_QUENTE,
         caixa3d=((933.0, 4.0, 148.0), (949.0, 14.0, 151.0)), afastar=0.04, sobre=2, luz=True),
    dict(nome='lâmpada do Mate', tipo='lampada', obj='ror_Object_73', modo='baixo',
         caixa3d=((940.2, 11.95, 148.2), (942.0, 12.7, 149.75)), cor=LAMPADA, afastar=0.04),
    dict(nome='placa do museu', tipo='placa', obj='ror_Object_27', modo='mascara', mascara=m_letras_do_museu,
         retangulos=[(800, 804, 942, 846), (802, 855, 976, 895), (786, 964, 956, 996)], cor=BRANCO_QUENTE,
         caixa=(905, 925, 115, 135), afastar=0.04, sobre=3, luz=True),
    dict(nome='janelas das casas', tipo='janela', obj='ror_Object_77', modo='mascara', mascara=m_escuro(0.36),
         retangulos=[(803, 113, 868, 232), (911, 15, 1008, 113)], cores=[AMBAR, LARANJA_QUENTE, CREME],
         apagadas=0.25, caixa=(805, 1045, 95, 130), afastar=0.03, sobre=1, luz=True),
    dict(nome='vitrine do quartel-general', tipo='janela', obj='ror_Object_14', modo='face',
         caixa=(1110, 1150, 95, 110), cores=[BRANCO_QUENTE], apagadas=0.0, brilho=0.5, afastar=0.03, area_min=1.0,
         uma_peca=True, luz=True),
    dict(nome='tijolos de vidro do quartel-general', tipo='janela', obj='ror_Object_67', modo='mascara',
         mascara=m_tijolo_de_vidro, retangulos=[(630, 143, 709, 244)], cores=[CREME, BRANCO_QUENTE],
         apagadas=0.0, caixa=(1060, 1200, 40, 130), afastar=0.03, sobre=1),
    dict(nome='letreiro do quartel-general', tipo='placa', obj='ror_Object_67', modo='mascara',
         mascara=m_letras_do_qg, retangulos=[(528, 312, 852, 344)], paleta=[BRANCO_LETREIRO, LARANJA_95],
         caixa=(1060, 1200, 40, 130), afastar=0.04, sobre=4),
    dict(nome='placa da Rust-eze', tipo='placa', obj='ror_Object_10', modo='mascara', mascara=m_amarelo,
         retangulos=None, cor=AMARELO_RUSTEZE, caixa=(1115, 1145, 100, 115), afastar=0.05, sobre=2,
         normal_z=1, luz=True),
    dict(nome='refletores da placa da Rust-eze', tipo='lampada', obj='ror_Object_67', modo='frente',
         caixa3d=((1119.5, 12.6, 109.0), (1138.6, 13.9, 110.6)), alvo=(1129.0, 15.5, 107.3), cor=LAMPADA,
         afastar=0.03),
    dict(nome='nome do cinema', tipo='placa', obj='ror_Object_27', modo='mascara', mascara=m_claro(0.62),
         retangulos=[(778, 555, 1014, 638), (776, 738, 980, 759)], cor=CREME, caixa=(1230, 1250, 100, 130),
         afastar=0.05, sobre=2, luz=True),
    dict(nome='painel do cinema', tipo='placa', obj='ror_Object_27', modo='mascara', mascara=m_painel_do_cinema,
         retangulos=[(768, 640, 976, 727)], cor=BRANCO_LETREIRO, brilho=0.6, caixa=(1230, 1250, 100, 130),
         afastar=0.04, sobre=1),
]


# ------------------------------------------------------------------
# O mapa
# ------------------------------------------------------------------
def carregar_mapa():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=ARQ_MAPA)


def imagem_do_material(mat):
    if mat is None or mat.node_tree is None:
        return None
    for n in mat.node_tree.nodes:
        if n.type == 'TEX_IMAGE' and n.image is not None:
            return n.image
    return None


CACHE_DE_IMAGENS = {}


def pixels(img):
    """Os pixels da imagem (H x W x 4, de baixo para cima, como o UV), em tela"""
    if img.name not in CACHE_DE_IMAGENS:
        w, h = img.size
        a = np.array(img.pixels[:], dtype=np.float32).reshape(h, w, 4)
        CACHE_DE_IMAGENS[img.name] = a
    return CACHE_DE_IMAGENS[img.name]


def amostrar(px, x, y):
    """Bilinear, com a textura repetindo (x, y em texels, de baixo para cima)"""
    h, w = px.shape[:2]
    x = x - 0.5
    y = y - 0.5
    x0 = np.floor(x).astype(np.int64)
    y0 = np.floor(y).astype(np.int64)
    fx = (x - x0)[:, None]
    fy = (y - y0)[:, None]
    x0m, x1m = x0 % w, (x0 + 1) % w
    y0m, y1m = y0 % h, (y0 + 1) % h
    a = px[y0m, x0m] * (1 - fx) + px[y0m, x1m] * fx
    b = px[y1m, x0m] * (1 - fx) + px[y1m, x1m] * fx
    return a * (1 - fy) + b * fy


def triangulos(ob):
    """Os triângulos do objeto no mundo: posições (N x 3 x 3), UVs (N x 3 x 2), material, área, normal"""
    me = ob.data
    me.calc_loop_triangles()
    mw = np.array(ob.matrix_world)
    nv = len(me.vertices)
    co = np.empty(nv * 3)
    me.vertices.foreach_get('co', co)
    co = co.reshape(-1, 3) @ mw[:3, :3].T + mw[:3, 3]
    nt = len(me.loop_triangles)
    vi = np.empty(nt * 3, dtype=np.int64)
    me.loop_triangles.foreach_get('vertices', vi)
    li = np.empty(nt * 3, dtype=np.int64)
    me.loop_triangles.foreach_get('loops', li)
    mi = np.empty(nt, dtype=np.int64)
    me.loop_triangles.foreach_get('material_index', mi)
    uvl = me.uv_layers.active
    uv = np.empty(len(me.loops) * 2)
    uvl.data.foreach_get('uv', uv)
    uv = uv.reshape(-1, 2)
    P = co[vi].reshape(-1, 3, 3)
    T = uv[li].reshape(-1, 3, 2)
    cr = np.cross(P[:, 1] - P[:, 0], P[:, 2] - P[:, 0])
    area = np.linalg.norm(cr, axis=1) / 2
    n = cr / np.maximum(np.linalg.norm(cr, axis=1), 1e-12)[:, None]
    return P, T, mi, area, n


def na_caixa(P, caixa):
    c = P.mean(1)
    gx, gz = c[:, 0], -c[:, 1]
    x0, x1, z0, z1 = caixa
    return (gx >= x0) & (gx <= x1) & (gz >= z0) & (gz <= z1)


def na_caixa3d(P, caixa3d):
    c = P.mean(1)
    g = np.stack([c[:, 0], c[:, 2], -c[:, 1]], 1)
    mn, mx = np.array(caixa3d[0]), np.array(caixa3d[1])
    return np.all((g >= mn) & (g <= mx), axis=1)


# ------------------------------------------------------------------
# Rasterizar a máscara de um triângulo em retângulos
# ------------------------------------------------------------------
def retangulos_do_triangulo(P, T, px, mascara, cor_de, retangulos, sobre):
    """Os retângulos (em células de 1/sobre texel) de um triângulo onde a máscara acende.
    Devolve uma lista de (4 cantos no mundo, índice da cor)"""
    h, w = px.shape[:2]
    Tt = T * np.array([w, h]) * sobre                      # em células
    lo = np.floor(Tt.min(0)).astype(np.int64)
    hi = np.ceil(Tt.max(0)).astype(np.int64)
    caixas = []
    if retangulos is None:
        caixas.append((lo[0], lo[1], hi[0], hi[1]))
    else:
        # cada retângulo, repetido nas cópias da textura que o triângulo cobre
        for (x0, y0, x1, y1) in retangulos:
            by0, by1 = h - y1, h - y0                        # de baixo para cima
            for kx in range(math.floor(lo[0] / (w * sobre)) - 1, math.floor(hi[0] / (w * sobre)) + 1):
                for ky in range(math.floor(lo[1] / (h * sobre)) - 1, math.floor(hi[1] / (h * sobre)) + 1):
                    a0 = max(lo[0], (kx * w + x0) * sobre)
                    a1 = min(hi[0], (kx * w + x1) * sobre)
                    b0 = max(lo[1], (ky * h + by0) * sobre)
                    b1 = min(hi[1], (ky * h + by1) * sobre)
                    if a1 > a0 and b1 > b0:
                        caixas.append((a0, b0, a1, b1))
    if not caixas:
        return []
    # o mapa afim de (células) para o mundo
    e1, e2 = Tt[1] - Tt[0], Tt[2] - Tt[0]
    det = e1[0] * e2[1] - e1[1] * e2[0]
    if abs(det) < 1e-9:
        return []
    inv = np.array([[e2[1], -e2[0]], [-e1[1], e1[0]]]) / det
    E = np.stack([P[1] - P[0], P[2] - P[0]], 1)            # 3 x 2

    def mundo(q):
        ab = (q - Tt[0]) @ inv.T
        return P[0] + ab @ E.T

    saida = []
    for (a0, b0, a1, b1) in caixas:
        if (a1 - a0) * (b1 - b0) > 6_000_000:
            print('  aviso: triângulo grande demais na textura, pulado')
            continue
        ii, jj = np.meshgrid(np.arange(a0, a1), np.arange(b0, b1))
        ii = ii.ravel()
        jj = jj.ravel()
        q = np.stack([ii + 0.5, jj + 0.5], 1)
        ab = (q - Tt[0]) @ inv.T
        dentro = (ab[:, 0] >= -1e-6) & (ab[:, 1] >= -1e-6) & (ab.sum(1) <= 1 + 1e-6)
        if not dentro.any():
            continue
        ii, jj, q = ii[dentro], jj[dentro], q[dentro]
        c = amostrar(px, q[:, 0] / sobre, q[:, 1] / sobre)
        acende = mascara(c)
        if not acende.any():
            continue
        ii, jj, c = ii[acende], jj[acende], c[acende]
        ci = cor_de(c)
        # tiras na horizontal: células seguidas na mesma linha e com a mesma cor
        ordem = np.lexsort((ii, ci, jj))
        ii, jj, ci = ii[ordem], jj[ordem], ci[ordem]
        quebra = np.ones(len(ii), bool)
        quebra[1:] = (jj[1:] != jj[:-1]) | (ci[1:] != ci[:-1]) | (ii[1:] != ii[:-1] + 1)
        ini = np.flatnonzero(quebra)
        fim = np.append(ini[1:], len(ii)) - 1
        tiras = np.stack([ii[ini], ii[fim] + 1, ci[ini], jj[ini]], 1)
        # e as tiras iguais em linhas seguidas viram um retângulo só
        ordem = np.lexsort((tiras[:, 3], tiras[:, 2], tiras[:, 1], tiras[:, 0]))
        tiras = tiras[ordem]
        quebra = np.ones(len(tiras), bool)
        quebra[1:] = np.any(tiras[1:, :3] != tiras[:-1, :3], axis=1) | (tiras[1:, 3] != tiras[:-1, 3] + 1)
        ini = np.flatnonzero(quebra)
        fim = np.append(ini[1:], len(tiras)) - 1
        for s, f in zip(ini, fim):
            x0, x1, cor, y0 = tiras[s]
            y1 = tiras[f, 3] + 1
            cantos = np.array([[x0, y0], [x1, y0], [x1, y1], [x0, y1]], dtype=np.float64)
            saida.append((mundo(cantos), int(cor)))
    return saida


def cor_pela_paleta(paleta):
    pal = np.array(paleta, dtype=np.float64)
    paln = pal / pal.max(1, keepdims=True)

    def cor_de(c):
        rgb = c[:, :3].astype(np.float64)
        rgbn = rgb / np.maximum(rgb.max(1, keepdims=True), 1e-4)
        d = ((rgbn[:, None, :] - paln[None, :, :]) ** 2).sum(2)
        return d.argmin(1)
    return cor_de


def cor_unica(c):
    return np.zeros(len(c), dtype=np.int64)


# ------------------------------------------------------------------
# Juntar as peças (os grupos de retângulos perto uns dos outros: uma placa, uma janela)
# ------------------------------------------------------------------
def agrupar(poligonos, passo=1.2):
    """União dos polígonos que encostam (um canto no mesmo lugar) ou que ficam a menos de ~passo um do outro
    (os centros numa grade de células)"""
    centros = np.array([p.mean(0) for p in poligonos])
    n = len(centros)
    pai = np.arange(n)

    def raiz(i):
        while pai[i] != i:
            pai[i] = pai[pai[i]]
            i = pai[i]
        return i

    cantos = {}
    for i, p in enumerate(poligonos):
        for q in np.round(p * 1000).astype(np.int64):
            k = tuple(q)
            if k in cantos:
                a, b = raiz(i), raiz(cantos[k])
                if a != b:
                    pai[a] = b
            else:
                cantos[k] = i
    cel = np.floor(centros / passo).astype(np.int64)
    tabela = {}
    for i, k in enumerate(map(tuple, cel)):
        tabela.setdefault(k, []).append(i)
    for k, lista in tabela.items():
        for i in lista[1:]:
            a, b = raiz(i), raiz(lista[0])
            if a != b:
                pai[a] = b
        for dx in (-1, 0, 1):
            for dy in (-1, 0, 1):
                for dz in (-1, 0, 1):
                    if (dx, dy, dz) <= (0, 0, 0):
                        continue
                    viz = tabela.get((k[0] + dx, k[1] + dy, k[2] + dz))
                    if viz:
                        a, b = raiz(lista[0]), raiz(viz[0])
                        if a != b:
                            pai[a] = b
    return np.array([raiz(i) for i in range(n)])


def ordem_de_acender(x_jogo, tipo, sorteio):
    d = min(abs(x_jogo - CENTRO_DA_ONDA) / ALCANCE_DA_ONDA, 1.0)
    if tipo == 'janela':
        a = 0.03 + 0.55 * d + 0.32 * sorteio
    elif tipo == 'lampada':
        a = 0.00 + 0.55 * d + 0.06 * sorteio
    else:
        a = 0.10 + 0.80 * d + 0.08 * sorteio
    return float(min(max(a, 0.0), 1.0))


# ------------------------------------------------------------------
# Montar cada fonte
# ------------------------------------------------------------------
class Peca:
    """Um grupo de retângulos (ou triângulos) que acende junto: os cantos no mundo (Blender), a cor de cada
    canto e o A (a ordem)"""
    def __init__(self, fonte):
        self.fonte = fonte
        self.poligonos = []      # listas de cantos (3 ou 4) no mundo
        self.cores = []          # uma cor (r, g, b) por canto, na mesma ordem


def montar_fonte(f):
    ob = bpy.data.objects.get(f['obj'])
    if ob is None:
        print('  aviso: %s não está no mapa' % f['obj'])
        return []
    P, T, mi, area, n = triangulos(ob)
    sel = np.ones(len(P), bool)
    if 'caixa' in f:
        sel &= na_caixa(P, f['caixa'])
    if 'caixa3d' in f:
        sel &= na_caixa3d(P, f['caixa3d'])
    ny = n[:, 2]                                             # o y do jogo
    if f.get('so_em_pe'):
        sel &= np.abs(ny) < 0.3
    if 'normal_z' in f:                                      # só as faces viradas para esse lado (z do jogo)
        sel &= (-n[:, 1]) * f['normal_z'] > 0.5
    poligonos, cores_idx, normais = [], [], []

    if f['modo'] == 'mascara':
        if 'paleta' in f:
            paleta, cor_de = f['paleta'], cor_pela_paleta(f['paleta'])
        else:
            paleta, cor_de = [f.get('cor', (1, 1, 1))], cor_unica
        for t in np.flatnonzero(sel):
            img = imagem_do_material(ob.data.materials[mi[t]]) if mi[t] < len(ob.data.materials) else None
            if img is None:
                continue
            px = pixels(img)
            h, w = px.shape[:2]
            if area[t] < f.get('area_min', 0.0):
                continue
            ext = (T[t].max(0) - T[t].min(0)) * np.array([w, h])
            if f.get('uv_min') and min(ext) < f['uv_min']:
                continue
            for cantos, ci in retangulos_do_triangulo(P[t], T[t], px, f['mascara'], cor_de,
                                                      f.get('retangulos'), f.get('sobre', 1)):
                poligonos.append(cantos + n[t] * f['afastar'])
                cores_idx.append(ci)
                normais.append(n[t])
    elif f['modo'] == 'face':
        sel &= np.abs(ny) < 0.3
        sel &= area >= f.get('area_min', 0.0) / 2
        for t in np.flatnonzero(sel):
            poligonos.append(P[t] + n[t] * f['afastar'])
            cores_idx.append(0)
            normais.append(n[t])
        paleta = [(1, 1, 1)]
    elif f['modo'] == 'baixo':
        sel &= ny < -0.35
        for t in np.flatnonzero(sel):
            poligonos.append(P[t] + n[t] * f['afastar'])
            cores_idx.append(0)
            normais.append(n[t])
        paleta = [f['cor']]
    elif f['modo'] == 'frente':
        alvo = B(*f['alvo'])
        for t in np.flatnonzero(sel):
            d = alvo - P[t].mean(0)
            d /= np.linalg.norm(d)
            if n[t] @ d > 0.25:
                poligonos.append(P[t] + n[t] * f['afastar'])
                cores_idx.append(0)
                normais.append(n[t])
        paleta = [f['cor']]

    if not poligonos:
        print('  aviso: %s não acendeu nada' % f['nome'])
        return []

    # as peças: o que fica junto acende junto
    centros = np.array([p.mean(0) for p in poligonos])
    if f.get('uma_peca'):
        grupo = np.zeros(len(poligonos), dtype=np.int64)          # tudo acende junto, da mesma cor
    else:
        grupo = agrupar(poligonos, 1.2 if f['tipo'] != 'janela' else 0.6)
    pecas = []
    for g in np.unique(grupo):
        idx = np.flatnonzero(grupo == g)
        cg = centros[idx].mean(0)
        # o sorteio de cada peça sai da posição dela (o mesmo a cada vez que a ferramenta roda)
        r = np.random.default_rng(abs(int(cg[0] * 7919 + cg[1] * 104729 + cg[2] * 1299709)) + SEMENTE)
        if f['tipo'] == 'janela' and r.random() < f.get('apagadas', 0.0):
            continue
        pc = Peca(f)
        pc.normal = np.mean([normais[i] for i in idx], axis=0)
        pc.ordem = ordem_de_acender(cg[0], f['tipo'], r.random())
        if f['tipo'] == 'janela':
            # uma luz de dentro, mais fraca que um neon: umas mais acesas, outras de abajur
            base = np.array(f['cores'][r.integers(len(f['cores']))], dtype=np.float64)
            base = base * f.get('brilho', BRILHO_DA_JANELA) * (0.72 + 0.28 * r.random())
            # a luz de dentro: um pouco mais clara embaixo
            zs = np.concatenate([poligonos[i][:, 2] for i in idx])
            z0, z1 = zs.min(), zs.max()
            for i in idx:
                pc.poligonos.append(poligonos[i])
                k = 1.0 - 0.24 * (poligonos[i][:, 2] - z0) / max(z1 - z0, 1e-6)
                pc.cores.append(base[None, :] * k[:, None])
        else:
            for i in idx:
                pc.poligonos.append(poligonos[i])
                cor = np.array(paleta[cores_idx[i]], dtype=np.float64) * f.get('brilho', 1.0)
                pc.cores.append(np.repeat(cor[None, :], len(poligonos[i]), 0))
        pecas.append(pc)
    return pecas


# ------------------------------------------------------------------
# As luzes pontuais (para o JSON)
# ------------------------------------------------------------------
def luzes_das_pecas(pecas):
    """Uma luz por peça que pede luz, na frente dela e mais para baixo (onde ela ilumina a calçada e a parede);
    as que caem perto se juntam (a média ponderada pelo quanto cada uma emite) e ficam as MAX_LUZES mais fortes"""
    candidatas = []
    for pc in pecas:
        tipo = pc.fonte['tipo']
        if not pc.fonte.get('luz'):
            continue
        cantos = np.concatenate(pc.poligonos)
        cores = np.concatenate(pc.cores)
        area = 0.0
        for p in pc.poligonos:
            area += np.linalg.norm(np.cross(p[1] - p[0], p[2] - p[0])) * (1.0 if len(p) == 4 else 0.5)
        c = cantos.mean(0)
        cor = cores.mean(0)
        brilho = float(cor.max())
        cor = cor / max(brilho, 1e-6)
        # o quanto a peça emite: a área acesa vezes o brilho (os tubos de neon são finos mas fortes;
        # uma lâmpada pequena ilumina bem em volta dela)
        peso = area * brilho * (3.0 if tipo == 'neon' else 1.0)
        peso = max(peso, {'lampada': 6.0, 'placa': 3.0}.get(tipo, 0.0), pc.fonte.get('peso_minimo', 0.0))
        nrm = pc.normal.copy()
        nrm[2] = 0.0                                          # só a parte deitada (para a frente)
        if np.linalg.norm(nrm) > 0.3:
            nrm /= np.linalg.norm(nrm)
        else:
            nrm[:] = 0.0                                      # placa dos dois lados: a luz fica embaixo dela
        if tipo == 'lampada':
            x, y, z = J(c + nrm * pc.fonte.get('luz_afastar', 1.0))
            y -= 0.8
        elif tipo == 'janela':
            x, y, z = J(c + nrm * pc.fonte.get('luz_afastar', 1.8))
            y = min(max(y - 1.0, 2.2), 3.5)
        else:
            x, y, z = J(c + nrm * pc.fonte.get('luz_afastar', 2.5))
            y = min(max(y - 3.0, 2.5), 7.0)
        if 'luz_altura' in pc.fonte:
            y = pc.fonte['luz_altura']
        candidatas.append(dict(x=x, y=y, z=z, cor=cor, peso=peso))
    # juntar as que caem perto (até 7 m)
    luzes = []
    for c in sorted(candidatas, key=lambda c: -c['peso']):
        for l in luzes:
            if math.dist((l['x'], l['y'], l['z']), (c['x'], c['y'], c['z'])) < 7.0:
                p = l['peso'] + c['peso']
                for k in ('x', 'y', 'z'):
                    l[k] = (l[k] * l['peso'] + c[k] * c['peso']) / p
                l['cor'] = (l['cor'] * l['peso'] + c['cor'] * c['peso']) / p
                l['peso'] = p
                break
        else:
            luzes.append(dict(c))
    # escolher até MAX_LUZES espalhadas pela cidade: a cada escolha, as que ficam a menos de 30 m das já
    # escolhidas valem menos (senão a rua oeste, cheia de neon e vitrine, levava todas)
    escolhidas = []
    resto = list(luzes)
    while resto and len(escolhidas) < MAX_LUZES:
        def valor(l):
            perto = sum(1 for e in escolhidas if abs(e['x'] - l['x']) < 30.0 and abs(e['z'] - l['z']) < 30.0)
            return l['peso'] / (1.0 + 0.8 * perto)
        melhor = max(resto, key=valor)
        escolhidas.append(melhor)
        resto.remove(melhor)
    saida = []
    for l in sorted(escolhidas, key=lambda l: l['x']):
        cor = l['cor'] / max(l['cor'].max(), 1e-6)
        r = math.sqrt(l['peso'])
        raio = float(np.clip(5.0 + 1.4 * r, 6.0, 16.0))
        forca = float(np.clip(0.40 + 0.12 * r, 0.5, 1.4))
        saida.append({'x': round(l['x'], 2), 'y': round(l['y'], 2), 'z': round(l['z'], 2),
                      'r': round(float(cor[0]), 3), 'g': round(float(cor[1]), 3), 'b': round(float(cor[2]), 3),
                      'raio': round(raio, 1), 'forca': round(forca, 2)})
    return saida


# ------------------------------------------------------------------
# Exportar
# ------------------------------------------------------------------
def material_da_luz():
    m = bpy.data.materials.new('luz_da_cidade')
    m.use_nodes = True
    bsdf = next(n for n in m.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    bsdf.inputs['Base Color'].default_value = (1, 1, 1, 1)
    return m


def objetos_das_pecas(pecas, nome, mat):
    """Malhas com no máximo RETANGULOS_POR_OBJETO polígonos cada; a cor acesa e a ordem nos cantos"""
    objs = []
    lote, n_lote = [], 0
    lotes = []
    for pc in pecas:
        for p, c in zip(pc.poligonos, pc.cores):
            lote.append((p, c, pc.ordem))
            n_lote += 1
            if n_lote >= RETANGULOS_POR_OBJETO:
                lotes.append(lote)
                lote, n_lote = [], 0
    if lote:
        lotes.append(lote)
    for k, lote in enumerate(lotes):
        verts, faces, cores = [], [], []
        for p, c, a in lote:
            i0 = len(verts)
            verts.extend(map(tuple, p))
            faces.append(tuple(range(i0, i0 + len(p))))
            for cc in c:
                cores.append((min(cc[0], 1.0), min(cc[1], 1.0), min(cc[2], 1.0), a))
        me = bpy.data.meshes.new('%s_%d' % (nome, k))
        me.from_pydata(verts, [], faces)
        me.update()
        at = me.color_attributes.new('Col', 'FLOAT_COLOR', 'CORNER')
        # os cantos de cada face estão na ordem em que foram criados
        li = np.empty(len(me.loops), dtype=np.int64)
        me.loops.foreach_get('vertex_index', li)
        cores = np.array(cores, dtype=np.float32)
        at.data.foreach_set('color', cores[li].ravel())
        me.color_attributes.active_color = at
        me.color_attributes.render_color_index = me.color_attributes.active_color_index
        me.materials.append(mat)
        o = bpy.data.objects.new('%s_%d' % (nome, k), me)
        bpy.context.scene.collection.objects.link(o)
        objs.append(o)
    return objs


def ler_glb(caminho):
    d = open(caminho, 'rb').read()
    n = struct.unpack('<I', d[12:16])[0]
    return json.loads(d[20:20 + n]), d


def conferir_glb(caminho, esperado):
    j, d = ler_glb(caminho)
    n_json = struct.unpack('<I', d[12:16])[0]
    base = 20 + n_json + 8
    tris, prims, maior, sem_cor, tipos = 0, 0, 0, 0, set()
    soma, n = np.zeros(4), 0
    for me in j['meshes']:
        for p in me['primitives']:
            prims += 1
            tris += j['accessors'][p['indices']]['count'] // 3
            maior = max(maior, j['accessors'][p['attributes']['POSITION']]['count'])
            if 'COLOR_0' not in p['attributes']:
                sem_cor += 1
                continue
            acc = j['accessors'][p['attributes']['COLOR_0']]
            tipos.add((acc['type'], acc['componentType']))
            bv = j['bufferViews'][acc['bufferView']]
            dt, esc = {5121: (np.uint8, 255.0), 5123: (np.uint16, 65535.0), 5126: (np.float32, 1.0)}[acc['componentType']]
            comps = {'VEC3': 3, 'VEC4': 4}[acc['type']]
            ini = base + bv.get('byteOffset', 0) + acc.get('byteOffset', 0)
            a = np.frombuffer(d, dt, count=acc['count'] * comps, offset=ini).reshape(-1, comps) / esc
            if comps == 3:
                a = np.concatenate([a, np.ones((len(a), 1))], 1)
            soma += a.sum(0)
            n += len(a)
    fatores = [m.get('pbrMetallicRoughness', {}).get('baseColorFactor', [1, 1, 1, 1]) for m in j.get('materials', [])]
    print('Exportado %s' % caminho)
    print('  %d triângulos, %d primitivas (sem COLOR_0: %d; COLOR_0 %s), maior: %d vértices' % (
        tris, prims, sem_cor, sorted(tipos), maior))
    print('  baseColorFactor %s, imagens: %d' % (fatores, len(j.get('images', []))))
    print('  COLOR_0 lido do .glb: média %s; nos cantos do Blender: %s' % (
        np.round(soma / max(n, 1), 3), np.round(esperado, 3)))
    if sem_cor:
        raise SystemExit('ERRO: há primitivas sem COLOR_0')
    if tipos != {('VEC4', 5126)} and not all(t[0] == 'VEC4' for t in tipos):
        raise SystemExit('ERRO: COLOR_0 sem o A (a ordem de acender)')
    if maior > 65535:
        raise SystemExit('ERRO: primitiva com mais de 65535 vértices')
    return tris, prims, maior


def exportar(objs, caminho):
    bpy.ops.object.select_all(action='DESELECT')
    for o in objs:
        o.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]
    bpy.ops.export_scene.gltf(filepath=caminho, export_format='GLB', use_selection=True, export_yup=True,
                              export_apply=True, export_animations=False, export_normals=False,
                              export_texcoords=False, export_vertex_color='ACTIVE', export_all_vertex_colors=False,
                              export_active_vertex_color_when_no_material=True, export_materials='EXPORT')
    soma, n = np.zeros(4), 0
    for o in objs:
        at = o.data.color_attributes['Col']
        c = np.empty(len(at.data) * 4, dtype=np.float32)
        at.data.foreach_get('color', c)
        c = c.reshape(-1, 4)
        soma += c.sum(0)
        n += len(c)
    return conferir_glb(caminho, soma / max(n, 1))


# ------------------------------------------------------------------
# Tudo
# ------------------------------------------------------------------
def main():
    carregar_mapa()
    pecas = []
    for f in FONTES:
        novas = montar_fonte(f)
        n_pol = sum(len(p.poligonos) for p in novas)
        print('%-36s %4d peças, %6d polígonos' % (f['nome'], len(novas), n_pol))
        pecas.extend(novas)
    if not pecas:
        raise SystemExit('ERRO: nada acendeu')
    mat = material_da_luz()
    objs = objetos_das_pecas(pecas, 'luzes_da_cidade', mat)
    caminho = os.path.join(PASTA, 'cidade.glb')
    tris, prims, maior = exportar(objs, caminho)
    luzes = luzes_das_pecas(pecas)
    dados = {'nome': 'Radiator Springs: a rua, a clínica, o quartel, o Mate e o leste da cidade',
             'acender_em': ACENDER_EM, 'luzes': luzes}
    with open(os.path.join(PASTA, 'cidade.json'), 'w') as fj:
        json.dump(dados, fj, indent=1, ensure_ascii=False)
    print('Gravado %s: %d luzes' % (os.path.join(PASTA, 'cidade.json'), len(luzes)))
    ordens = np.array([pc.ordem for pc in pecas])
    print('Ordem de acender: %d peças, A de %.2f a %.2f' % (len(pecas), ordens.min(), ordens.max()))


main()
