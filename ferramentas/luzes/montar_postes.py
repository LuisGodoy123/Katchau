"""
montar_postes.py
------------------------------------------------------------------
Monta os postes de luz da rua principal de Radiator Springs e o semáforo
pendurado do cruzamento (no filme Carros, 2006: os postes de madeira da rede
elétrica, um pouco tortos, com o braço curvo de metal e a luminária "cabeça de
cobra" de vapor de sódio, que faz poças amarelo-alaranjadas na rua; os fios
entre eles; e o semáforo de três lentes pendurado num fio sobre a rua, que
pisca amarelo), por cima do mapa do mundo aberto
(recursos/modelos/radiator_springs.glb), já nas coordenadas do mundo do jogo
(x, y para cima, z):

    Blender -b --factory-startup -P ferramentas/luzes/montar_postes.py -- \\
        recursos/modelos/radiator_springs.glb recursos/modelos

Grava, na pasta dada no segundo argumento:
  postes.glb           o desenho dos postes, de dia e de noite: os troncos de
                       madeira (com a cruzeta, as mãos-francesas e os
                       isoladores de vidro), o braço de metal e a carcaça da
                       luminária, uns transformadores, os fios entre os postes
                       (caindo em catenária), o semáforo (a caixa mostarda, as
                       viseiras e as lentes vermelha e verde, apagadas) e os
                       fios que o seguram atravessando a rua. O jogo desenha
                       com o shader do mapa (textura × baseColorFactor, a luz
                       da hora e a sombra do sol): a cor está nos materiais
                       (sem cor de vértice), a madeira tem uma textura PNG
                       feita aqui, e as normais não vão (o shader as tira da
                       posição);
  postes.json          a colisão: um cilindro por tronco
                       ({"postes": [{"x", "z", "y_base", "altura", "raio"}]});
  luzes/postes.glb     o vidro de cada luminária, que acende à noite: material
                       branco; em COLOR_0, o RGB é a cor acesa (o amarelo-
                       alaranjado do sódio) e o A é a ordem de acender (uma
                       onda que corre a rua de oeste para leste);
  luzes/postes.json    uma luz pontual por luminária, logo abaixo da cabeça
                       (é ela que faz a poça amarela na rua e nas fachadas);
  luzes/semaforo.glb   as lentes do meio do semáforo (âmbar, nas quatro faces),
                       A = 0: o jogo pisca este arquivo inteiro, dia e noite
                       (é o nome "semaforo" que liga isso, luzes.c);
  luzes/semaforo.json  sem luzes pontuais (a luz pontual não piscaria junto).
Cada primitiva com no máximo 60 mil vértices (a raylib usa índices de 16 bits).

O mapa (do Race-O-Rama de PS3) não tem postes nem semáforo. A ferramenta mede
nele, com raios:
  - o meio-fio de cada lado, em cada x (onde acaba o asfalto, ror_Object_87);
    o tronco fica DO_MEIO_FIO metros para dentro da calçada (ou do
    acostamento de terra, no leste) e o braço avança sobre a beira da pista;
  - o chão sob o tronco (a calçada), para a base;
  - se cabe: o tronco, a cruzeta, o braço e a cabeça não podem encostar em
    nada do mapa (prédio, placa, marquise, letreiro, árvore). Se o x pedido
    não serve, procura o mais perto (até PROCURA_ATE metros para cada lado);
  - os fios: o vão entre dois postes do mesmo lado só ganha fio se nenhum dos
    fios (com uma folga em volta) passar por um letreiro ou por um prédio.
E evita, com os dados de src/moradores_mapa.h:
  - as entradas dos lugares: cada trecho que leva um morador da rua até a
    entrada_* de um lugar (ou de volta) cruza o meio-fio num x; nada de poste
    a menos de FOLGA_DA_ENTRADA metros desses cruzamentos;
  - a estrada de terra que cruza a rua (o cruzamento, x ~497 a 525);
  - o lado sul de x 700 a 800: o Mack estaciona na faixa de quem vai para
    leste, em (735; 139,5), com 7,6 m de altura, e parte por ela (o braço
    passaria por cima dele); o jogador larga em (675; 135,9), no meio da rua;
  - o retorno do oeste (x < 316), onde os moradores fazem a volta;
  - o canteiro da placa do Cozy Cone (lado sul, x 680 a 700): o único lugar
    livre ali põe o poste bem na frente do MOTEL, na vista da largada.

Os postes ficam alternados (um de cada lado a cada ~20 m; de cada lado, um a
cada ~40 m), mais ralos onde as entradas não deixam (os lugares do meio da
cidade). O semáforo pende do fio entre os dois postes da esquina leste do
cruzamento (um em cada lado da rua), com um segundo fio por baixo (o que segura
a caixa para não balançar), a uns 6,3 a 7,4 m da rua.

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
import tempfile
import shutil
import numpy as np
import mathutils
from mathutils.bvhtree import BVHTree

argv = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
if len(argv) < 2:
    raise SystemExit('uso: Blender -b --factory-startup -P montar_postes.py -- mapa.glb pasta_dos_modelos')
ARQ_MAPA, PASTA = argv[0], argv[1]
PASTA_DAS_LUZES = os.path.join(PASTA, 'luzes')
os.makedirs(PASTA_DAS_LUZES, exist_ok=True)

# ------------------------------------------------------------------
# Constantes
# ------------------------------------------------------------------
SEMENTE = 1955                # o sorteio da inclinação de cada poste (sempre o mesmo resultado)
MAX_VERTICES = 60000          # por primitiva (a raylib usa índices de 16 bits)

# A rua principal (medida no mapa)
ASFALTO = 'ror_Object_87'     # o asfalto da rua principal
MEIO_DA_RUA_Z = 135.3
FAIXAS_Z = (131.5, 138.8)     # as faixas dos moradores (oeste e leste)
DO_MEIO_FIO = 0.9             # metros do meio-fio até o meio do tronco
PROCURA_ATE = 6.0             # metros para cada lado do x pedido
FOLGA_DA_ENTRADA = 8.0        # metros para cada lado de onde os carros cruzam a calçada

# O poste (metros, a partir do chão)
ALTURA = 9.0                  # o alto do tronco
ENTERRADO = 0.4               # o pé entra no chão (a calçada não é plana)
RAIO_DO_PE, RAIO_DO_ALTO = 0.16, 0.12
RAIO_DA_COLISAO = 0.22
LADOS_DO_TRONCO = 12
CRUZETA_Y = 8.40              # a cruzeta (de lado no tronco, atravessando a rua)
CRUZETA_MEIA = 1.0            # meio comprimento
ISOLADORES_R = (-0.85, 0.85)  # na cruzeta (para longe da rua e para a rua); mais um no alto do tronco
BRACO_Y = 6.95                # onde o braço sai do tronco
CABECA_Y = 7.62               # a borda de baixo da carcaça (a luminária fica a ~7,6 m)
CABECA_DE, CABECA_ATE = 1.86, 2.62   # a carcaça, em metros do eixo do tronco para a rua
VIDRO_R = 2.27                # o meio do vidro
LUZ_ABAIXO = 0.38             # a luz pontual, abaixo da borda da carcaça
INCLINACAO_DA_CABECA = math.radians(4.0)

# A luz
COR_ACESA = (1.00, 0.70, 0.32)        # o vidro aceso (sódio)
COR_DA_LUZ = (1.00, 0.72, 0.38)       # a luz pontual
RAIO_DA_LUZ = 22.0
FORCA_DA_LUZ = 1.5
ACENDER_EM = 0.3                      # segundos: os postes acendem antes dos neons
COR_DO_SEMAFORO = (1.00, 0.72, 0.10)  # a lente âmbar do meio

# Os fios
RAIO_DO_FIO = 0.016
LADOS_DO_FIO = 4
VAO_MAXIMO = 70.0             # metros: mais que isso, sem fio entre os dois
CAIDA = 0.022                 # quanto o fio cai no meio do vão (fração do vão)
FOLGA_DO_FIO = 0.25           # metros em volta do fio que têm de estar livres

# O semáforo
SEMAFORO_ALTO = 7.70          # o fio, sobre a rua (metros acima do asfalto)
SEMAFORO_LADO = 0.42          # a caixa (quadrada, vista de cima)
SEMAFORO_ALTURA = 1.12
LENTE_RAIO = 0.10
LENTES_DY = 0.36              # entre as lentes
AMARRA_Y = 6.0                # o fio de baixo, no tronco (metros acima do chão)
SPAN_Y = 8.15                 # o fio de cima, no tronco

# Cores (na tela, 0 a 1: o shader do mapa multiplica direto)
CORES = {
    'madeira': (1.0, 1.0, 1.0),              # a cor vem da textura
    'metal': (0.40, 0.41, 0.42),
    'luminaria': (0.50, 0.51, 0.50),
    'fio': (0.06, 0.055, 0.05),
    'isolador': (0.48, 0.62, 0.56),          # vidro verde-água
    'transformador': (0.44, 0.46, 0.46),
    'semaforo': (0.64, 0.51, 0.12),          # amarelo-mostarda
    'semaforo_dentro': (0.05, 0.05, 0.045),
    'lente_vermelha': (0.34, 0.05, 0.04),
    'lente_verde': (0.05, 0.25, 0.16),
}

# ------------------------------------------------------------------
# Onde: os x pedidos (o lado N é o de z menor, o do Doc e do Flo; o S, o do Luigi e do Cozy Cone).
# "semaforo" marca os dois postes de onde pende o semáforo
# ------------------------------------------------------------------
PEDIDOS = [
    (322, 'N'), (336, 'S'), (364, 'N'), (378, 'S'), (400, 'N'), (418, 'S'), (438, 'N'), (452, 'S'),
    (519.5, 'S', 'semaforo'), (527.5, 'N', 'semaforo'),
    (577, 'N'), (583, 'S'), (607, 'N'), (622, 'S'), (656, 'N'), (684, 'N'),
    (722, 'N'), (788, 'N'), (806, 'S'),
]
# (no lado S, de 680 a 700 só cabe um poste no canteiro da placa do Cozy Cone, bem na frente do MOTEL e
# da vista da largada: fica sem; a luz ali vem do 684 N e da placa)
# Os fios que atravessam a rua (entre os postes desses x, um de cada lado)
TRAVESSIAS = [(400, 418), (577, 583), (607, 622)]
TRANSFORMADORES = [364, 607, 722]    # os postes com um transformador pendurado

# Os nós dos moradores (src/moradores_mapa.h): os trechos entre a rua e a entrada de cada lugar
L = lambda x: (x, 138.8)          # faixa de quem vai para leste
O = lambda x: (x, 131.5)          # faixa de quem vai para oeste
ENTRADAS = {
    'casa_della_tires': ((480.5, 145.8), [L(450), O(505)], [L(505), O(450)]),
    'ramone': ((483.0, 125.0), [L(450), O(505)], [L(505), O(450)]),
    'flo': ((547.0, 124.5), [L(530), O(580)], [L(580), O(530)]),
    'curios': ((561.0, 144.8), [L(530), O(580)], [L(580), O(530)]),
    'doc': ((634.5, 125.5), [L(605), O(655)], [L(655), O(605)]),
    'cozy': ((660.0, 146.5), [L(630), O(680), (649.0, 145.5)], [L(680), O(630)]),
    'sarge': ((701.0, 126.8), [L(680), (715.0, 130.6)], [(715.0, 134.4), O(680)]),
    'fillmore': ((755.5, 125.5), [(735.0, 134.4), O(770)], [L(770), (735.0, 130.6)]),
}
# As faixas proibidas de cada lado (além das entradas): (x de, x até, por quê)
PROIBIDOS = {
    'N': [(0, 316, 'o retorno do oeste'), (496, 526, 'a estrada de terra do cruzamento')],
    'S': [(0, 316, 'o retorno do oeste'), (494, 519, 'a estrada de terra do cruzamento'),
          (700, 800, 'o Mack (estacionado em 735 e partindo para leste) e a vaga atrás dele')],
}
MEIO_FIO_APROX = {'N': 128.3, 'S': 142.2}   # só para achar onde os trechos cruzam a calçada


def B(x, y, z):
    """Do jogo para o Blender"""
    return mathutils.Vector((x, -z, y))


def normalizar(v):
    v = np.asarray(v, dtype=np.float64)
    n = np.linalg.norm(v)
    return v / n if n > 1e-12 else v


# ------------------------------------------------------------------
# O mapa: os raios
# ------------------------------------------------------------------
class Mapa:
    def __init__(self):
        bpy.ops.wm.read_factory_settings(use_empty=True)
        bpy.ops.import_scene.gltf(filepath=ARQ_MAPA)
        verts, polys, nomes = [], [], []
        base = 0
        for ob in bpy.data.objects:
            if ob.type != 'MESH':
                continue
            me = ob.data
            mw = np.array(ob.matrix_world)
            n = len(me.vertices)
            co = np.empty(n * 3)
            me.vertices.foreach_get('co', co)
            co = co.reshape(-1, 3) @ mw[:3, :3].T + mw[:3, 3]
            me.calc_loop_triangles()
            nt = len(me.loop_triangles)
            tv = np.empty(nt * 3, dtype=np.int64)
            me.loop_triangles.foreach_get('vertices', tv)
            verts.extend(map(tuple, co))
            polys.extend(map(tuple, tv.reshape(-1, 3) + base))
            nomes.extend([ob.name] * nt)
            base += n
        self.bvh = BVHTree.FromPolygons(verts, polys)
        self.nomes = nomes
        print('Mapa: %d triângulos' % len(polys))

    def raio(self, a, b):
        """O primeiro acerto entre os pontos a e b (jogo): (distância, objeto) ou None"""
        pa, pb = B(*a), B(*b)
        d = pb - pa
        comprimento = d.length
        if comprimento < 1e-6:
            return None
        loc, nor, idx, dist = self.bvh.ray_cast(pa, d / comprimento, comprimento)
        return None if loc is None else (dist, self.nomes[idx])

    def chao(self, x, z, de):
        """A altura do primeiro acerto descendo de y = de: (y, objeto) ou None"""
        loc, nor, idx, dist = self.bvh.ray_cast(B(x, de, z), mathutils.Vector((0, 0, -1)), 30.0)
        return None if loc is None else (loc.z, self.nomes[idx])

    def caixa_livre(self, cantos, passo=0.2):
        """Raios em grade cruzando a caixa nas três direções. cantos: origem e três arestas (jogo).
        Devolve o objeto que está dentro dela, ou None"""
        o, a, b, c = (np.asarray(v, dtype=np.float64) for v in cantos)
        na = max(2, int(math.ceil(np.linalg.norm(a) / passo)) + 1)
        nb = max(2, int(math.ceil(np.linalg.norm(b) / passo)) + 1)
        nc = max(2, int(math.ceil(np.linalg.norm(c) / passo)) + 1)
        ta, tb, tc = np.linspace(0, 1, na), np.linspace(0, 1, nb), np.linspace(0, 1, nc)
        for (e1, t1), (e2, t2), (e3, _) in (((a, ta), (b, tb), (c, tc)), ((a, ta), (c, tc), (b, tb)),
                                           ((b, tb), (c, tc), (a, ta))):
            for s in t1:
                for t in t2:
                    p = o + s * e1 + t * e2
                    r = self.raio(p, p + e3)
                    if r:
                        return r[1]
        return None


# ------------------------------------------------------------------
# Onde pode: as entradas e as faixas proibidas
# ------------------------------------------------------------------
def cruzamentos_das_entradas():
    """Para cada lado, os intervalos de x onde os carros cruzam a calçada (com a folga)"""
    saida = {'N': [], 'S': []}
    for nome, (entrada, vem, vai) in ENTRADAS.items():
        lado = 'N' if entrada[1] < MEIO_DA_RUA_Z else 'S'
        zc = MEIO_FIO_APROX[lado]
        xs = []
        for a, b in [(p, entrada) for p in vem] + [(entrada, p) for p in vai]:
            if (a[1] - zc) * (b[1] - zc) < 0:
                t = (zc - a[1]) / (b[1] - a[1])
                xs.append(a[0] + t * (b[0] - a[0]))
        if xs:
            saida[lado].append((min(xs) - FOLGA_DA_ENTRADA, max(xs) + FOLGA_DA_ENTRADA, 'a entrada ' + nome))
    return saida


def proibido(x, lado, entradas):
    for x0, x1, porque in PROIBIDOS[lado] + entradas[lado]:
        if x0 <= x <= x1:
            return porque
    return None


# ------------------------------------------------------------------
# O poste: o sistema de cada um (s ao longo da rua, y para cima, r para a rua) e a inclinação
# ------------------------------------------------------------------
class Poste:
    def __init__(self, x, lado, z, chao, chao_baixo, inclinacao, eixo, giro):
        self.x, self.lado, self.z = x, lado, z
        self.chao = chao                  # a altura da calçada no meio do tronco
        self.chao_baixo = chao_baixo      # a mais baixa em volta (o pé começa abaixo dela)
        self.para_a_rua = np.array([0.0, 0.0, 1.0 if lado == 'N' else -1.0])
        self.ao_longo = np.array([1.0, 0.0, 0.0])
        self.base = np.array([x, chao, z])
        eixo = normalizar(eixo)
        self.giro = mathutils.Matrix.Rotation(inclinacao, 3, mathutils.Vector(eixo))
        self.giro = np.array(self.giro)
        self.giro_da_textura = giro
        self.semaforo = False
        self.transformador = False

    def mundo(self, s, y, r):
        """Do sistema do poste (s, y, r; arrays) para o mundo, com a inclinação em volta do pé"""
        s, y, r = np.broadcast_arrays(np.asarray(s, dtype=np.float64), np.asarray(y, dtype=np.float64),
                                      np.asarray(r, dtype=np.float64))
        local = (s[..., None] * self.ao_longo + y[..., None] * np.array([0.0, 1.0, 0.0]) +
                 r[..., None] * self.para_a_rua)
        return self.base + local @ self.giro.T

    def raio_do_tronco(self, y):
        return RAIO_DO_PE + (RAIO_DO_ALTO - RAIO_DO_PE) * np.clip(y / ALTURA, 0, 1)


def medir_poste(mapa, x, lado):
    """O meio-fio, o lugar do tronco e o chão naquele x. Devolve (z, chao, chao_baixo) ou None"""
    passo = -1.0 if lado == 'N' else 1.0
    z = MEIO_DA_RUA_Z
    y_rua = None
    while abs(z - MEIO_DA_RUA_Z) < 12.0:
        c = mapa.chao(x, z, 6.0)
        if c is None or not c[1].startswith(ASFALTO):
            break
        y_rua = c[0]
        z += passo * 0.05
    else:
        return None
    if y_rua is None:
        return None
    meio_fio = z - passo * 0.025
    zp = meio_fio + passo * DO_MEIO_FIO
    c = mapa.chao(x, zp, y_rua + 3.0)
    if c is None or not (y_rua - 0.2 <= c[0] <= y_rua + 0.95):
        return None
    alturas = [c[0]]
    for a in range(8):
        ang = a * math.pi / 4
        cc = mapa.chao(x + 0.3 * math.cos(ang), zp + 0.3 * math.sin(ang), y_rua + 3.0)
        if cc is not None and y_rua - 0.5 <= cc[0] <= y_rua + 1.0:
            alturas.append(cc[0])
    return zp, c[0], min(alturas), meio_fio, y_rua


def cabe_o_poste(mapa, x, lado, z, chao, transformador=False):
    """O tronco, a cruzeta, o braço e a cabeça não encostam em nada? Devolve o que atrapalha, ou None"""
    r = 1.0 if lado == 'N' else -1.0
    # o tronco (com folga), do chão até acima do alto (começa 0,45 m acima: a calçada sobe na rampa
    # do meio-fio, e um meio-fio ou um canteiro baixo pode ficar em volta do pé)
    meio = 0.65 if transformador else 0.45
    o = mapa.caixa_livre(((x - meio, chao + 0.45, z - meio), (2 * meio, 0, 0), (0, ALTURA + 0.6 - 0.45, 0),
                          (0, 0, 2 * meio)))
    if o:
        return 'o tronco (' + o + ')'
    # a cruzeta (atravessa a rua, de -1 a +1 m) com os isoladores e o alto
    o = mapa.caixa_livre(((x - 0.4, chao + CRUZETA_Y - 0.5, z - CRUZETA_MEIA - 0.25), (0.8, 0, 0), (0, 1.4, 0),
                          (0, 0, 2 * CRUZETA_MEIA + 0.5)))
    if o:
        return 'a cruzeta (' + o + ')'
    # o braço e a cabeça, até 0,2 m depois da frente da carcaça
    z0, z1 = sorted((z, z + r * (CABECA_ATE + 0.2)))
    o = mapa.caixa_livre(((x - 0.5, chao + BRACO_Y - 0.8, z0), (1.0, 0, 0), (0, 1.9, 0), (0, 0, z1 - z0)))
    if o:
        return 'o braço (' + o + ')'
    return None


def escolher_postes(mapa):
    entradas = cruzamentos_das_entradas()
    for lado in 'NS':
        for x0, x1, porque in entradas[lado]:
            print('  %s: sem poste de x %.1f a %.1f (%s)' % (lado, x0, x1, porque))
    sorteio = np.random.default_rng(SEMENTE)
    postes = []
    for pedido in PEDIDOS:
        x_pedido, lado = float(pedido[0]), pedido[1]
        e_semaforo = len(pedido) > 2 and pedido[2] == 'semaforo'
        transformador = int(round(x_pedido)) in TRANSFORMADORES
        achado = None
        tentativas = sorted(np.arange(-PROCURA_ATE, PROCURA_ATE + 1e-6, 0.5), key=lambda d: (abs(d), d))
        motivos = []
        for d in tentativas:
            x = x_pedido + d
            porque = proibido(x, lado, entradas)
            if porque:
                motivos.append('%.1f: %s' % (x, porque))
                continue
            m = medir_poste(mapa, x, lado)
            if m is None:
                motivos.append('%.1f: sem calçada' % x)
                continue
            zp, chao, chao_baixo, meio_fio, y_rua = m
            # o tronco não pode ficar na faixa dos carros (com 2,5 m de folga)
            if min(abs(zp - f) for f in FAIXAS_Z) < 2.5 + RAIO_DA_COLISAO:
                motivos.append('%.1f: na faixa' % x)
                continue
            atrapalha = cabe_o_poste(mapa, x, lado, zp, chao, transformador)
            if atrapalha:
                motivos.append('%.1f: %s' % (x, atrapalha))
                continue
            achado = (x, zp, chao, chao_baixo, meio_fio, y_rua)
            break
        if achado is None:
            print('  ERRO: o poste em x %.1f (%s) não coube: %s' % (x_pedido, lado, '; '.join(motivos[:6])))
            continue
        x, zp, chao, chao_baixo, meio_fio, y_rua = achado
        inclinacao = math.radians(sorteio.uniform(0.6, 1.7))     # um pouco tortos
        ang = sorteio.uniform(0, 2 * math.pi)
        p = Poste(x, lado, zp, chao, chao_baixo, inclinacao, (math.cos(ang), 0.0, math.sin(ang)),
                  sorteio.uniform(0, 1))
        p.semaforo = e_semaforo
        p.transformador = transformador
        p.meio_fio, p.y_rua = meio_fio, y_rua
        postes.append(p)
        print('  poste %s x %6.1f (pedido %6.1f)  z %6.2f  meio-fio %6.2f  chão %.2f (rua %.2f)%s%s' % (
            lado, x, x_pedido, zp, meio_fio, chao, y_rua, '  semáforo' if e_semaforo else '',
            '  transformador' if transformador else ''))
    postes.sort(key=lambda p: p.x)
    return postes


# ------------------------------------------------------------------
# As malhas (no mundo do jogo): vértices, faces, UV e material
# ------------------------------------------------------------------
class Malhas:
    """Junta as peças por material"""

    def __init__(self):
        self.por_material = {}

    def juntar(self, material, verts, faces, uvs=None, cor=None):
        verts = np.asarray(verts, dtype=np.float64).reshape(-1, 3)
        if uvs is None:
            uvs = np.zeros((len(verts), 2))
        lista = self.por_material.setdefault(material, [])
        lista.append((verts, [tuple(f) for f in faces], np.asarray(uvs, dtype=np.float64).reshape(-1, 2), cor))

    def triangulos(self):
        return sum(sum(len(f) - 2 for f in fs) for lista in self.por_material.values() for _, fs, _, _ in lista)


def orientar(verts, face, dentro):
    """A face com a frente para fora (para longe do ponto 'dentro')"""
    p = verts[list(face)]
    n = np.zeros(3)
    for i in range(len(p)):
        n += np.cross(p[i], p[(i + 1) % len(p)])
    if np.dot(n, p.mean(0) - dentro) < 0:
        return tuple(reversed(face))
    return tuple(face)


def tubo(pontos, raios, lados, tampas=True, v_por_metro=0.0, u_giro=0.0, normal0=None):
    """Um tubo pela polilinha (anéis com o referencial transportado, sem torcer).
    Devolve verts, faces, uvs. A costura do UV repete o primeiro vértice de cada anel"""
    P = np.asarray(pontos, dtype=np.float64)
    R = np.broadcast_to(np.asarray(raios, dtype=np.float64), (len(P),))
    n = len(P)
    T = np.zeros_like(P)
    for i in range(n):
        a, b = P[max(i - 1, 0)], P[min(i + 1, n - 1)]
        T[i] = normalizar(b - a)
    if normal0 is None:
        ref = np.array([0.0, 1.0, 0.0]) if abs(T[0][1]) < 0.9 else np.array([1.0, 0.0, 0.0])
    else:
        ref = np.asarray(normal0, dtype=np.float64)
    N0 = normalizar(ref - np.dot(ref, T[0]) * T[0])
    Ns, Bs = [], []
    nn = N0
    for i in range(n):
        nn = normalizar(nn - np.dot(nn, T[i]) * T[i])
        Ns.append(nn)
        Bs.append(np.cross(T[i], nn))
    comprimento = np.concatenate([[0], np.cumsum(np.linalg.norm(np.diff(P, axis=0), axis=1))])
    verts, uvs, faces = [], [], []
    for i in range(n):
        for j in range(lados + 1):
            th = 2 * math.pi * j / lados
            verts.append(P[i] + R[i] * (math.cos(th) * Ns[i] + math.sin(th) * Bs[i]))
            uvs.append((j / lados + u_giro, comprimento[i] * v_por_metro))
    k = lados + 1
    for i in range(n - 1):
        for j in range(lados):
            faces.append((i * k + j, i * k + j + 1, (i + 1) * k + j + 1, (i + 1) * k + j))
    if tampas:
        faces.append(tuple(reversed(range(0, lados))))
        faces.append(tuple(range((n - 1) * k, (n - 1) * k + lados)))
    return np.array(verts), faces, np.array(uvs)


def caixa(centro, eixos, meias, uv_escala=None):
    """Uma caixa (faces separadas, para o UV de cada uma seguir o comprimento: o eixo 0)"""
    c = np.asarray(centro, dtype=np.float64)
    E = [np.asarray(e, dtype=np.float64) * m for e, m in zip(eixos, meias)]
    verts, faces, uvs = [], [], []
    for eixo in range(3):
        for sinal in (-1, 1):
            a, b = [k for k in range(3) if k != eixo]
            cantos = []
            for sa, sb in ((-1, -1), (1, -1), (1, 1), (-1, 1)):
                cantos.append(c + sinal * E[eixo] + sa * E[a] + sb * E[b])
            i0 = len(verts)
            verts.extend(cantos)
            for p in cantos:
                if uv_escala:
                    # o v corre ao longo do eixo 0 (o veio da madeira); nas pontas, o que sobrar
                    d = p - c
                    if eixo == 0:
                        u, v = np.dot(d, normalizar(E[1])), np.dot(d, normalizar(E[2]))
                    else:
                        u, v = np.dot(d, normalizar(E[b if a == 0 else a])), np.dot(d, normalizar(E[0]))
                    uvs.append((u * uv_escala, v * uv_escala))
                else:
                    uvs.append((0.0, 0.0))
            faces.append((i0, i0 + 1, i0 + 2, i0 + 3))
    verts = np.array(verts)
    faces = [orientar(verts, f, c) for f in faces]
    return verts, faces, np.array(uvs)


def calota(centro, normal, raio, altura, lados=12, aneis=3):
    """Uma lente: uma calota de esfera virada para 'normal', com a base no centro"""
    n = normalizar(normal)
    a = normalizar(np.cross(n, [0.0, 1.0, 0.0]) if abs(n[1]) < 0.9 else np.cross(n, [1.0, 0.0, 0.0]))
    b = np.cross(n, a)
    R = (raio * raio + altura * altura) / (2 * altura)       # o raio da esfera
    verts = [np.asarray(centro) + n * altura]                # o bico
    for k in range(aneis, 0, -1):
        r = raio * k / aneis
        h = math.sqrt(max(R * R - r * r, 0)) - (R - altura)
        for j in range(lados):
            th = 2 * math.pi * j / lados
            verts.append(np.asarray(centro) + n * h + r * (math.cos(th) * a + math.sin(th) * b))
    verts = np.array(verts)
    # os anéis vão de fora (idx 0, o da base) para dentro (idx aneis - 1, o mais perto do bico)
    faces = []
    inicio = lambda idx: 1 + idx * lados
    for j in range(lados):
        faces.append((0, inicio(aneis - 1) + j, inicio(aneis - 1) + (j + 1) % lados))
    for idx in range(aneis - 1):
        for j in range(lados):
            faces.append((inicio(idx) + j, inicio(idx) + (j + 1) % lados, inicio(idx + 1) + (j + 1) % lados,
                          inicio(idx + 1) + j))
    faces.append(tuple(range(inicio(0), inicio(0) + lados)))    # o fundo
    dentro = np.asarray(centro) + n * (altura * 0.4)
    faces = [orientar(verts, f, dentro) for f in faces]
    return verts, faces


def anel_de_cortes(cortes):
    """Junta cortes (uma lista de polígonos fechados com o mesmo número de pontos) num sólido fechado"""
    m = len(cortes[0])
    verts = np.concatenate(cortes)
    faces = []
    for i in range(len(cortes) - 1):
        for j in range(m):
            faces.append((i * m + j, i * m + (j + 1) % m, (i + 1) * m + (j + 1) % m, (i + 1) * m + j))
    centros = [c.mean(0) for c in cortes]
    # os lados: para fora do eixo; as pontas: para longe do corte vizinho
    saida = [orientar(verts, f, (centros[f[0] // m] + centros[f[2] // m]) / 2) for f in faces]
    saida.append(orientar(verts, tuple(range(0, m)), centros[1]))
    saida.append(orientar(verts, tuple(range((len(cortes) - 1) * m, len(cortes) * m)), centros[-2]))
    return verts, saida


def bezier(p0, p1, p2, p3, n):
    t = np.linspace(0, 1, n)[:, None]
    return ((1 - t) ** 3) * p0 + 3 * ((1 - t) ** 2) * t * p1 + 3 * (1 - t) * t * t * p2 + (t ** 3) * p3


def catenaria(a, b, caida, n):
    """O fio de a até b, caindo 'caida' metros no meio (parábola)"""
    t = np.linspace(0, 1, n)[:, None]
    p = np.asarray(a) + t * (np.asarray(b) - np.asarray(a))
    p[:, 1] -= caida * 4 * t[:, 0] * (1 - t[:, 0])
    return p


# ------------------------------------------------------------------
# As peças do poste
# ------------------------------------------------------------------
def isolador(centro_da_base):
    """Um isolador de vidro (de sino), em pé; devolve verts, faces e o ponto onde o fio passa"""
    perfil = [(0.000, 0.022), (0.035, 0.050), (0.055, 0.054), (0.070, 0.040), (0.090, 0.046), (0.115, 0.036),
              (0.135, 0.020)]
    base = np.asarray(centro_da_base)
    pontos = [base + [0, y, 0] for y, _ in perfil]
    v, f, _ = tubo(pontos, [r for _, r in perfil], 7, normal0=(1, 0, 0))
    return v, f, base + [0, 0.105, 0]


def montar_poste(p, malhas):
    """Tudo de um poste. Devolve os pontos dos fios {nome: ponto}, a luz e o vidro"""
    pontos = {}
    # o tronco, um pouco mais grosso embaixo
    ys = np.array([p.chao_baixo - p.chao - ENTERRADO, 0.0, 1.5, 3.5, 5.5, 7.5, ALTURA])
    eixo = p.mundo(0, ys, 0)
    v, f, uv = tubo(eixo, p.raio_do_tronco(ys), LADOS_DO_TRONCO, v_por_metro=0.5, u_giro=p.giro_da_textura,
                    normal0=p.ao_longo)
    malhas.juntar('madeira', v, f, uv)
    # o alto: um pino com isolador
    v, f, fio = isolador(p.mundo(0, ALTURA, 0))
    malhas.juntar('isolador', v, f)
    pontos['alto'] = fio
    # a cruzeta: de lado no tronco (no +s), atravessando a rua
    s_cruzeta = RAIO_DO_ALTO + 0.05
    centro = p.mundo(s_cruzeta, CRUZETA_Y, 0)
    eixo_r = p.giro @ p.para_a_rua
    eixo_y = p.giro @ np.array([0.0, 1.0, 0.0])
    eixo_s = p.giro @ p.ao_longo
    v, f, uv = caixa(centro, (eixo_r, eixo_y, eixo_s), (CRUZETA_MEIA, 0.055, 0.045), uv_escala=0.5)
    malhas.juntar('madeira', v, f, uv)
    for r in ISOLADORES_R:
        v, f, fio = isolador(p.mundo(s_cruzeta, CRUZETA_Y + 0.055, r))
        malhas.juntar('isolador', v, f)
        pontos['cruzeta_%+.0f' % r] = fio
    # as mãos-francesas (do tronco à cruzeta)
    for r in (-0.55, 0.55):
        a = p.mundo(RAIO_DO_ALTO + 0.01, CRUZETA_Y - 0.55, 0)
        b = p.mundo(s_cruzeta, CRUZETA_Y - 0.055, r)
        v, f, _ = tubo([a, b], 0.014, 4)
        malhas.juntar('metal', v, f)
    # o braço: sai do tronco, sobe em curva e chega quase deitado na cabeça
    raio_no_braco = float(p.raio_do_tronco(BRACO_Y))
    rs_ys = bezier(np.array([raio_no_braco * 0.6, BRACO_Y]), np.array([0.95, BRACO_Y - 0.02]),
                   np.array([1.40, CABECA_Y + 0.06]), np.array([CABECA_DE + 0.04, CABECA_Y + 0.06]), 18)
    braco = p.mundo(0, rs_ys[:, 1], rs_ys[:, 0])
    v, f, _ = tubo(braco, 0.036, 8)
    malhas.juntar('metal', v, f)
    # a escora (do tronco até o meio do braço) e as braçadeiras
    a = p.mundo(0, BRACO_Y - 0.62, float(p.raio_do_tronco(BRACO_Y - 0.62)) * 0.7)
    k = 6
    v, f, _ = tubo([a, braco[k]], 0.018, 6)
    malhas.juntar('metal', v, f)
    for y in (BRACO_Y, BRACO_Y - 0.62):
        rr = float(p.raio_do_tronco(y)) + 0.012
        v, f, _ = tubo(p.mundo(0, np.array([y - 0.045, y + 0.045]), 0), rr, LADOS_DO_TRONCO)
        malhas.juntar('metal', v, f)
    # a cabeça (a carcaça), em cortes ao longo do comprimento; a borda de baixo é reta
    ss = np.array([0.00, 0.08, 0.18, 0.32, 0.50, 0.68, 0.82, 0.92, 0.97, 1.00])
    larg = np.interp(ss, [0, 0.12, 0.3, 0.5, 0.75, 0.9, 0.97, 1.0], [0.10, 0.20, 0.32, 0.34, 0.32, 0.27, 0.19, 0.08])
    alt = np.interp(ss, [0, 0.12, 0.3, 0.5, 0.75, 0.9, 0.97, 1.0], [0.09, 0.12, 0.16, 0.165, 0.15, 0.12, 0.08, 0.035])
    fundo = np.interp(ss, [0, 0.12, 0.3, 1.0], [0.05, 0.02, 0.0, 0.0])    # o pescoço é redondo embaixo
    cortes = []
    giro_cabeca = INCLINACAO_DA_CABECA
    r_neck, y_neck = CABECA_DE, CABECA_Y

    def na_cabeca(s_, y_, r_):
        """Do sistema da cabeça (r ao longo dela, a partir do pescoço) para o do poste, inclinada para cima"""
        rr = r_neck + r_ * math.cos(giro_cabeca) - (y_ - y_neck) * math.sin(giro_cabeca)
        yy = y_neck + r_ * math.sin(giro_cabeca) + (y_ - y_neck) * math.cos(giro_cabeca)
        return s_, yy, rr

    comprimento = CABECA_ATE - CABECA_DE
    for k in range(len(ss)):
        corte = []
        for j in range(12):
            th = math.pi * j / 11
            s_ = larg[k] / 2 * math.cos(th)
            y_ = CABECA_Y + alt[k] * (math.sin(th) ** 0.75)
            corte.append(na_cabeca(s_, y_, ss[k] * comprimento))
        corte.append(na_cabeca(-larg[k] / 2 * 0.8, CABECA_Y - fundo[k], ss[k] * comprimento))
        corte.append(na_cabeca(larg[k] / 2 * 0.8, CABECA_Y - fundo[k], ss[k] * comprimento))
        c = np.array(corte)
        cortes.append(p.mundo(c[:, 0], c[:, 1], c[:, 2]))
    v, f = anel_de_cortes(cortes)
    malhas.juntar('luminaria', v, f)
    # o vidro: meia elipse embaixo da carcaça (vai para luzes/postes.glb)
    vidro_r = VIDRO_R - CABECA_DE
    eixos = (0.25, 0.125, 0.085)          # ao longo, de lado, para baixo
    vv, ff = [], []
    lados, aneis = 14, 5
    for a in range(aneis + 1):
        fi = (math.pi / 2) * a / aneis   # 0 na borda, 90° no fundo
        for j in range(lados):
            th = 2 * math.pi * j / lados
            r_ = vidro_r + eixos[0] * math.cos(th) * math.cos(fi)
            s_ = eixos[1] * math.sin(th) * math.cos(fi)
            y_ = CABECA_Y - 0.004 - eixos[2] * math.sin(fi)
            vv.append(na_cabeca(s_, y_, r_))
    vv = np.array(vv)
    vv = p.mundo(vv[:, 0], vv[:, 1], vv[:, 2])
    for a in range(aneis):
        for j in range(lados):
            ff.append((a * lados + j, a * lados + (j + 1) % lados, (a + 1) * lados + (j + 1) % lados,
                       (a + 1) * lados + j))
    # (sem tampa em cima: a carcaça fecha o vidro)
    centro_do_vidro = np.array(p.mundo(*na_cabeca(0, CABECA_Y - 0.03, vidro_r)))
    # o fundo vira um ponto só (o anel 90° tem todos iguais): tira as faces degeneradas depois
    ff = [orientar(vv, f_, centro_do_vidro) for f_ in ff]
    vidro = (vv, ff)
    luz = np.array(p.mundo(*na_cabeca(0, CABECA_Y - LUZ_ABAIXO, vidro_r)))
    # o transformador (do lado das casas)
    if p.transformador:
        eixo_t = p.mundo(0, np.array([ALTURA - 2.75, ALTURA - 1.95]), -0.44)
        v, f, _ = tubo(eixo_t, 0.24, 14)
        malhas.juntar('transformador', v, f)
        v, f, _ = tubo(p.mundo(0, np.array([ALTURA - 1.95, ALTURA - 1.88]), -0.44), 0.255, 14)
        malhas.juntar('transformador', v, f)
        for y in (ALTURA - 2.55, ALTURA - 2.15):
            v, f, uv = caixa(p.mundo(0, y, -0.18), (eixo_r, eixo_y, eixo_s), (0.12, 0.03, 0.06))
            malhas.juntar('metal', v, f)
        # o fio que desce da cruzeta até ele
        v, f, _ = tubo([pontos['cruzeta_-1'], p.mundo(0.1, ALTURA - 1.86, -0.44)], RAIO_DO_FIO, LADOS_DO_FIO)
        malhas.juntar('fio', v, f)
    # os pontos do fio do semáforo (no lado da rua do tronco)
    pontos['span'] = np.array(p.mundo(0, SPAN_Y, float(p.raio_do_tronco(SPAN_Y)) + 0.02))
    pontos['amarra'] = np.array(p.mundo(0, AMARRA_Y, float(p.raio_do_tronco(AMARRA_Y)) + 0.02))
    if p.semaforo:
        for y in (SPAN_Y, AMARRA_Y):
            rr = float(p.raio_do_tronco(y)) + 0.012
            v, f, _ = tubo(p.mundo(0, np.array([y - 0.05, y + 0.05]), 0), rr, LADOS_DO_TRONCO)
            malhas.juntar('metal', v, f)
    return pontos, luz, vidro


def limpar_degeneradas(verts, faces):
    """Tira as faces com área zero (o fundo do vidro) e os vértices repetidos delas"""
    saida = []
    for f in faces:
        unicos = []
        for i in f:
            if not any(np.allclose(verts[i], verts[u], atol=1e-7) for u in unicos):
                unicos.append(i)
        if len(unicos) >= 3:
            saida.append(tuple(unicos))
    return saida


# ------------------------------------------------------------------
# Os fios
# ------------------------------------------------------------------
def fio_livre(mapa, pts):
    """O fio (polilinha) e uma folga em volta não passam por nada do mapa?"""
    desvios = [np.zeros(3), np.array([0, FOLGA_DO_FIO, 0]), np.array([0, -FOLGA_DO_FIO, 0]),
               np.array([0, 0, FOLGA_DO_FIO]), np.array([0, 0, -FOLGA_DO_FIO]),
               np.array([FOLGA_DO_FIO, 0, 0]), np.array([-FOLGA_DO_FIO, 0, 0])]
    for d in desvios:
        for a, b in zip(pts[:-1], pts[1:]):
            r = mapa.raio(a + d, b + d)
            if r:
                return r[1]
    return None


def montar_fios(mapa, postes, pontos, malhas):
    vaos, sem = [], []
    for lado in 'NS':
        doLado = [p for p in postes if p.lado == lado]
        for a, b in zip(doLado[:-1], doLado[1:]):
            vao = abs(b.x - a.x)
            if vao > VAO_MAXIMO:
                sem.append('%s %.0f-%.0f (%.0f m: longe demais)' % (lado, a.x, b.x, vao))
                continue
            linhas = []
            atrapalha = None
            for chave in ('alto', 'cruzeta_-1', 'cruzeta_+1'):
                pts = catenaria(pontos[id(a)][chave], pontos[id(b)][chave], CAIDA * vao, max(12, int(vao / 2)))
                o = fio_livre(mapa, pts[1:-1]) if len(pts) > 2 else None
                if o:
                    atrapalha = o
                    break
                linhas.append(pts)
            if atrapalha:
                sem.append('%s %.0f-%.0f (passaria por %s)' % (lado, a.x, b.x, atrapalha))
                continue
            for pts in linhas:
                v, f, _ = tubo(pts, RAIO_DO_FIO, LADOS_DO_FIO, tampas=False)
                malhas.juntar('fio', v, f)
            vaos.append('%s %.0f-%.0f' % (lado, a.x, b.x))
    # as travessias: do isolador da rua de um ao do outro
    for xa, xb in TRAVESSIAS:
        a = min((p for p in postes if p.lado == 'N'), key=lambda p: abs(p.x - xa))
        b = min((p for p in postes if p.lado == 'S'), key=lambda p: abs(p.x - xb))
        pts = catenaria(pontos[id(a)]['cruzeta_+1'], pontos[id(b)]['cruzeta_+1'],
                        CAIDA * np.linalg.norm(pontos[id(a)]['cruzeta_+1'] - pontos[id(b)]['cruzeta_+1']), 14)
        o = fio_livre(mapa, pts[1:-1])
        if o:
            sem.append('travessia %.0f-%.0f (passaria por %s)' % (a.x, b.x, o))
            continue
        v, f, _ = tubo(pts, RAIO_DO_FIO, LADOS_DO_FIO, tampas=False)
        malhas.juntar('fio', v, f)
        vaos.append('travessia %.0f-%.0f' % (a.x, b.x))
    print('Fios: %s' % ', '.join(vaos))
    if sem:
        print('Sem fio: %s' % ', '.join(sem))
    return vaos, sem


# ------------------------------------------------------------------
# O semáforo
# ------------------------------------------------------------------
def montar_semaforo(mapa, postes, pontos, malhas):
    """A caixa pendurada no meio do fio entre os dois postes da esquina. Devolve as lentes acesas e o lugar"""
    dois = [p for p in postes if p.semaforo]
    if len(dois) != 2:
        print('ERRO: o semáforo precisa de dois postes (achou %d)' % len(dois))
        return None, None
    n, s = sorted(dois, key=lambda p: p.lado)    # N e S
    an, as_ = pontos[id(n)]['span'], pontos[id(s)]['span']
    meio = (an + as_) / 2
    c = mapa.chao(meio[0], meio[2], 6.0)
    y_rua = c[0] if c else 1.48
    gancho = np.array([meio[0], y_rua + SEMAFORO_ALTO, meio[2]])
    topo = gancho[1] - 0.30
    centro = np.array([meio[0], topo - SEMAFORO_ALTURA / 2, meio[2]])
    fundo = topo - SEMAFORO_ALTURA
    # o fio de cima (do tronco até o gancho, de cada lado) e o de baixo (até o olhal embaixo da caixa)
    for a in (an, as_):
        pts = catenaria(a, gancho, 0.10, 16)
        v, f, _ = tubo(pts, RAIO_DO_FIO * 1.25, LADOS_DO_FIO, tampas=False)
        malhas.juntar('fio', v, f)
    olhal = np.array([meio[0], fundo - 0.06, meio[2]])
    for p in (n, s):
        pts = catenaria(pontos[id(p)]['amarra'], olhal, 0.06, 14)
        v, f, _ = tubo(pts, RAIO_DO_FIO, LADOS_DO_FIO, tampas=False)
        malhas.juntar('fio', v, f)
    # o gancho: a haste e a braçadeira no fio
    v, f, _ = tubo([gancho + [0, 0.04, 0], np.array([meio[0], topo + 0.05, meio[2]])], 0.025, 8)
    malhas.juntar('metal', v, f)
    v, f, _ = tubo([gancho + [-0.07, 0.0, 0], gancho + [0.07, 0.0, 0]], 0.04, 8)
    malhas.juntar('metal', v, f)
    v, f, _ = tubo([np.array([meio[0], fundo, meio[2]]), olhal], 0.018, 6)
    malhas.juntar('metal', v, f)
    # a caixa e a tampa de cima
    X, Y, Z = np.eye(3)
    m = SEMAFORO_LADO / 2
    v, f, _ = caixa(centro, (X, Y, Z), (m, SEMAFORO_ALTURA / 2, m))
    malhas.juntar('semaforo', v, f)
    v, f, _ = caixa(centro + [0, SEMAFORO_ALTURA / 2 + 0.03, 0], (X, Y, Z), (m + 0.03, 0.03, m + 0.03))
    malhas.juntar('semaforo', v, f)
    v, f, _ = caixa(centro - [0, SEMAFORO_ALTURA / 2 + 0.025, 0], (X, Y, Z), (m + 0.02, 0.025, m + 0.02))
    malhas.juntar('semaforo', v, f)
    acesas = []
    for normal in (X, -X, Z, -Z):
        lado = np.cross(Y, normal)
        face = centro + normal * m
        for k, (dy, cor) in enumerate(((LENTES_DY, 'lente_vermelha'), (0.0, None), (-LENTES_DY, 'lente_verde'))):
            meio_lente = face + Y * dy
            # o fundo preto em volta da lente
            v, f, _ = tubo([meio_lente, meio_lente + normal * 0.004], LENTE_RAIO + 0.03, 14, normal0=Y)
            malhas.juntar('semaforo_dentro', v, f)
            # a lente: as apagadas aqui, a do meio no semaforo.glb
            v, f = calota(meio_lente + normal * 0.006, normal, LENTE_RAIO, 0.035)
            if cor:
                malhas.juntar(cor, v, f)
            else:
                acesas.append((v, f))
            # a viseira: meio tubo (aberto embaixo), de fora mostarda e de dentro preta
            angs = np.linspace(math.radians(-25), math.radians(205), 11)
            r_fora, r_dentro, fundo_v = LENTE_RAIO + 0.035, LENTE_RAIO + 0.023, 0.24
            eixo_da_viseira = meio_lente + normal * (fundo_v / 2)
            for rr, mat, para_fora in ((r_fora, 'semaforo', True), (r_dentro, 'semaforo_dentro', False)):
                vv, ff = [], []
                for i in range(len(angs)):
                    d = math.cos(angs[i]) * lado + math.sin(angs[i]) * Y
                    vv.append(meio_lente + normal * 0.004 + d * rr)
                    vv.append(meio_lente + normal * fundo_v + d * rr)
                vv = np.array(vv)
                for i in range(len(angs) - 1):
                    f_ = orientar(vv, (2 * i, 2 * i + 2, 2 * i + 3, 2 * i + 1), eixo_da_viseira)
                    ff.append(f_ if para_fora else tuple(reversed(f_)))
                malhas.juntar(mat, vv, ff)
            # a borda da viseira (a frente e as pontas), fechando a espessura
            vv, ff = [], []
            for i in range(len(angs)):
                d = math.cos(angs[i]) * lado + math.sin(angs[i]) * Y
                vv.append(meio_lente + normal * fundo_v + d * r_fora)
                vv.append(meio_lente + normal * fundo_v + d * r_dentro)
            vv = np.array(vv)
            for i in range(len(angs) - 1):
                ff.append(orientar(vv, (2 * i, 2 * i + 2, 2 * i + 3, 2 * i + 1), meio_lente))
            malhas.juntar('semaforo', vv, ff)
    print('Semáforo: no meio do fio entre x %.1f (N) e x %.1f (S): (%.2f; %.2f), a caixa de %.2f a %.2f m '
          'acima da rua' % (n.x, s.x, meio[0], meio[2], fundo - y_rua, topo - y_rua))
    return acesas, (float(meio[0]), float(meio[2]), float(fundo - y_rua), float(topo - y_rua), float(y_rua))


# ------------------------------------------------------------------
# A textura da madeira (gasta, cinza-amarronzada, com veios ao longo do tronco)
# ------------------------------------------------------------------
def textura_da_madeira(pasta):
    W, H = 128, 512
    rng = np.random.default_rng(SEMENTE)

    def ruido(w, h, escala_x, escala_y):
        pequeno = rng.random((max(2, h // escala_y), max(2, w // escala_x)))
        ys = np.linspace(0, pequeno.shape[0] - 1, h)
        xs = np.linspace(0, pequeno.shape[1] - 1, w)
        y0 = np.floor(ys).astype(int)
        x0 = np.floor(xs).astype(int)
        y1 = np.minimum(y0 + 1, pequeno.shape[0] - 1)
        x1 = np.minimum(x0 + 1, pequeno.shape[1] - 1)
        fy = (ys - y0)[:, None]
        fx = (xs - x0)[None, :]
        a = pequeno[y0][:, x0] * (1 - fx) + pequeno[y0][:, x1] * fx
        b = pequeno[y1][:, x0] * (1 - fx) + pequeno[y1][:, x1] * fx
        return a * (1 - fy) + b * fy

    veio = 0.55 * ruido(W, H, 3, 64) + 0.30 * ruido(W, H, 1, 24) + 0.15 * ruido(W, H, 8, 8)
    mancha = ruido(W, H, 32, 96)
    base = np.array([0.43, 0.37, 0.30])
    cinza = np.array([0.47, 0.45, 0.42])
    escuro = np.array([0.24, 0.20, 0.16])
    cor = base[None, None, :] * (1 - 0.55 * mancha[..., None]) + cinza[None, None, :] * (0.55 * mancha[..., None])
    cor = cor * (0.80 + 0.32 * veio[..., None])
    # as rachaduras (umas linhas escuras ao longo do tronco)
    for _ in range(7):
        x = rng.integers(0, W)
        y0 = rng.integers(0, H)
        comprimento = rng.integers(H // 6, H // 2)
        for k in range(comprimento):
            y = (y0 + k) % H
            xx = (x + int(round(2 * math.sin(k / 37.0)))) % W
            cor[y, xx] = cor[y, xx] * 0.35 + escuro * 0.65
            cor[y, (xx + 1) % W] = cor[y, (xx + 1) % W] * 0.7 + escuro * 0.3
    cor = np.clip(cor, 0, 1)
    rgba = np.concatenate([cor, np.ones((H, W, 1))], 2).astype(np.float32)
    img = bpy.data.images.new('madeira_do_poste', W, H, alpha=True)
    img.pixels[:] = rgba.ravel()
    caminho = os.path.join(pasta, 'madeira_do_poste.png')
    img.filepath_raw = caminho
    img.file_format = 'PNG'
    img.save()
    return img


# ------------------------------------------------------------------
# Para o Blender e exportar
# ------------------------------------------------------------------
def material(nome, cor, imagem=None):
    m = bpy.data.materials.new(nome)
    m.use_nodes = True
    bsdf = next(n for n in m.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    bsdf.inputs['Base Color'].default_value = (cor[0], cor[1], cor[2], 1.0)
    bsdf.inputs['Roughness'].default_value = 0.8
    if imagem is not None:
        tex = m.node_tree.nodes.new('ShaderNodeTexImage')
        tex.image = imagem
        m.node_tree.links.new(tex.outputs['Color'], bsdf.inputs['Base Color'])
    return m


def criar_objeto(nome, verts, faces, uvs, mat, cores=None):
    """Um objeto do Blender a partir de vértices no jogo (convertidos), com UV e, se dadas, as cores nos cantos"""
    vb = np.stack([verts[:, 0], -verts[:, 2], verts[:, 1]], 1)
    me = bpy.data.meshes.new(nome)
    me.from_pydata([tuple(v) for v in vb], [], [tuple(f) for f in faces])
    me.update()
    li = np.empty(len(me.loops), dtype=np.int64)
    me.loops.foreach_get('vertex_index', li)
    if uvs is not None:
        uv = me.uv_layers.new(name='UVMap')
        uv.data.foreach_set('uv', uvs[li].astype(np.float32).ravel())
    if cores is not None:
        at = me.color_attributes.new('Col', 'FLOAT_COLOR', 'CORNER')
        at.data.foreach_set('color', cores[li].astype(np.float32).ravel())
        me.color_attributes.active_color = at
        me.color_attributes.render_color_index = me.color_attributes.active_color_index
    me.materials.append(mat)
    o = bpy.data.objects.new(nome, me)
    bpy.context.scene.collection.objects.link(o)
    return o


def objetos_das_malhas(malhas, mats, prefixo, com_uv=True):
    """Um objeto por material (partido em MAX_VERTICES)"""
    objs = []
    for nome, lista in malhas.por_material.items():
        lote_v, lote_f, lote_uv, lote_c = [], [], [], []
        n = 0
        k = 0

        def fechar():
            nonlocal lote_v, lote_f, lote_uv, lote_c, n, k
            if not lote_v:
                return
            cores = np.concatenate(lote_c) if lote_c and lote_c[0] is not None else None
            objs.append(criar_objeto('%s_%s_%d' % (prefixo, nome, k), np.concatenate(lote_v), lote_f,
                                     np.concatenate(lote_uv) if com_uv else None, mats[nome], cores))
            lote_v, lote_f, lote_uv, lote_c = [], [], [], []
            n = 0
            k += 1

        for verts, faces, uvs, cor in lista:
            if n + len(verts) > MAX_VERTICES:
                fechar()
            lote_f.extend(tuple(i + n for i in f) for f in faces)
            lote_v.append(verts)
            lote_uv.append(uvs)
            lote_c.append(None if cor is None else np.broadcast_to(np.asarray(cor, dtype=np.float64),
                                                                  (len(verts), 4)))
            n += len(verts)
        fechar()
    return objs


def ler_glb(caminho):
    d = open(caminho, 'rb').read()
    n = struct.unpack('<I', d[12:16])[0]
    return json.loads(d[20:20 + n]), d


def conferir_glb(caminho, com_cor):
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
    fatores = [[round(c, 3) for c in m.get('pbrMetallicRoughness', {}).get('baseColorFactor', [1, 1, 1, 1])]
               for m in j.get('materials', [])]
    print('Exportado %s' % caminho)
    print('  %d triângulos, %d primitivas (sem COLOR_0: %d; COLOR_0 %s), maior: %d vértices' % (
        tris, prims, sem_cor, sorted(tipos), maior))
    print('  materiais %s, imagens: %d %s' % (fatores, len(j.get('images', [])),
                                             [im.get('mimeType') for im in j.get('images', [])]))
    if com_cor:
        print('  COLOR_0 médio: %s' % np.round(soma / max(n, 1), 3))
        if sem_cor:
            raise SystemExit('ERRO: há primitivas sem COLOR_0')
        if not all(t[0] == 'VEC4' for t in tipos):
            raise SystemExit('ERRO: COLOR_0 sem o A (a ordem de acender)')
    elif prims - sem_cor:
        raise SystemExit('ERRO: o modelo dos postes não deveria ter COLOR_0')
    if maior > 65535:
        raise SystemExit('ERRO: primitiva com mais de 65535 vértices')
    for im in j.get('images', []):
        if im.get('mimeType') != 'image/png':
            raise SystemExit('ERRO: imagem que não é PNG')
    return tris, prims, maior


def exportar(objs, caminho, com_cor):
    bpy.ops.object.select_all(action='DESELECT')
    for o in objs:
        o.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]
    if com_cor:
        bpy.ops.export_scene.gltf(filepath=caminho, export_format='GLB', use_selection=True, export_yup=True,
                                  export_apply=True, export_animations=False, export_normals=False,
                                  export_texcoords=False, export_vertex_color='ACTIVE',
                                  export_all_vertex_colors=False, export_active_vertex_color_when_no_material=True,
                                  export_materials='EXPORT')
    else:
        bpy.ops.export_scene.gltf(filepath=caminho, export_format='GLB', use_selection=True, export_yup=True,
                                  export_apply=True, export_animations=False, export_normals=False,
                                  export_texcoords=True, export_vertex_color='NONE', export_materials='EXPORT',
                                  export_image_format='AUTO')
    return conferir_glb(caminho, com_cor)


# ------------------------------------------------------------------
# Tudo
# ------------------------------------------------------------------
def main():
    mapa = Mapa()
    print('Os postes:')
    postes = escolher_postes(mapa)
    if not postes:
        raise SystemExit('ERRO: nenhum poste')
    # some o mapa da cena (só os raios precisavam dele): o que se exporta é só o que se monta aqui
    for o in list(bpy.data.objects):
        bpy.data.objects.remove(o, do_unlink=True)

    malhas = Malhas()
    luzes_vidro = Malhas()
    pontos, luzes = {}, []
    xs = np.array([p.x for p in postes])
    x0, x1 = xs.min(), xs.max()
    for p in postes:
        pts, luz, (vv, ff) = montar_poste(p, malhas)
        pontos[id(p)] = pts
        ordem = float((p.x - x0) / max(x1 - x0, 1e-6))
        ff = limpar_degeneradas(vv, ff)
        luzes_vidro.juntar('luz', vv, ff, cor=(COR_ACESA[0], COR_ACESA[1], COR_ACESA[2], ordem))
        luzes.append({'x': round(float(luz[0]), 2), 'y': round(float(luz[1]), 2), 'z': round(float(luz[2]), 2),
                      'r': COR_DA_LUZ[0], 'g': COR_DA_LUZ[1], 'b': COR_DA_LUZ[2],
                      'raio': RAIO_DA_LUZ, 'forca': FORCA_DA_LUZ})
    vaos, sem = montar_fios(mapa, postes, pontos, malhas)
    lentes, onde_semaforo = montar_semaforo(mapa, postes, pontos, malhas)
    luzes_semaforo = Malhas()
    for v, f in (lentes or []):
        luzes_semaforo.juntar('luz', v, f, cor=(COR_DO_SEMAFORO[0], COR_DO_SEMAFORO[1], COR_DO_SEMAFORO[2], 0.0))

    # os postes (de dia e de noite)
    pasta_temp = tempfile.mkdtemp(prefix='postes_')
    madeira = textura_da_madeira(pasta_temp)
    mats = {nome: material('poste_' + nome, cor, madeira if nome == 'madeira' else None)
            for nome, cor in CORES.items()}
    objs = objetos_das_malhas(malhas, mats, 'postes')
    tris, prims, maior = exportar(objs, os.path.join(PASTA, 'postes.glb'), False)
    for o in objs:
        bpy.data.objects.remove(o, do_unlink=True)
    colisao = [{'x': round(p.x, 2), 'z': round(float(p.z), 2), 'y_base': round(float(p.chao), 3),
                'altura': ALTURA, 'raio': RAIO_DA_COLISAO} for p in postes]
    with open(os.path.join(PASTA, 'postes.json'), 'w') as fj:
        json.dump({'postes': colisao}, fj, indent=1, ensure_ascii=False)
    print('Gravado %s: %d troncos' % (os.path.join(PASTA, 'postes.json'), len(colisao)))

    # o que acende
    branco = material('luz_dos_postes', (1.0, 1.0, 1.0))
    objs = objetos_das_malhas(luzes_vidro, {'luz': branco}, 'luzes_dos_postes', com_uv=False)
    exportar(objs, os.path.join(PASTA_DAS_LUZES, 'postes.glb'), True)
    for o in objs:
        bpy.data.objects.remove(o, do_unlink=True)
    with open(os.path.join(PASTA_DAS_LUZES, 'postes.json'), 'w') as fj:
        json.dump({'nome': 'os postes da rua principal (as luminárias de vapor de sódio)', 'acender_em': ACENDER_EM,
                   'luzes': luzes}, fj, indent=1, ensure_ascii=False)
    print('Gravado %s: %d luzes' % (os.path.join(PASTA_DAS_LUZES, 'postes.json'), len(luzes)))
    if luzes_semaforo.por_material:
        objs = objetos_das_malhas(luzes_semaforo, {'luz': branco}, 'semaforo', com_uv=False)
        exportar(objs, os.path.join(PASTA_DAS_LUZES, 'semaforo.glb'), True)
        with open(os.path.join(PASTA_DAS_LUZES, 'semaforo.json'), 'w') as fj:
            json.dump({'nome': 'o semáforo do cruzamento (pisca amarelo)', 'acender_em': 0.0, 'luzes': []}, fj,
                      indent=1, ensure_ascii=False)
        print('Gravado %s' % os.path.join(PASTA_DAS_LUZES, 'semaforo.json'))
    shutil.rmtree(pasta_temp, ignore_errors=True)
    print('Resumo: %d postes, %d luzes; postes.glb com %d triângulos em %d primitivas (maior: %d vértices)' % (
        len(postes), len(luzes), tris, prims, maior))
    if onde_semaforo:
        print('Semáforo em (%.2f; %.2f), de %.2f a %.2f m acima da rua' % onde_semaforo[:4])


main()
