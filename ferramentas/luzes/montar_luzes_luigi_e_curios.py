"""
montar_luzes_luigi_e_curios.py
------------------------------------------------------------------
Monta os neons e as luzes de noite de dois lugares de Radiator Springs (do
filme Carros, 2006), desenhados por cima do mapa do mundo aberto
(recursos/modelos/radiator_springs.glb), já nas coordenadas do mundo do jogo
(x, y para cima, z):

  Luigi's Casa Della Tires   a loja de pneus do Luigi e do Guido;
  Radiator Springs Curios    a loja de lembranças da Lizzie.

    Blender -b --factory-startup -P ferramentas/luzes/montar_luzes_luigi_e_curios.py -- \\
        recursos/modelos/radiator_springs.glb recursos/modelos/luzes [--previas pasta]

Grava, na pasta dada no segundo argumento:
  luigi.glb, curios.glb    só o que brilha à noite (o jogo desenha na origem).
                           Sem normais; material branco; em todas as primitivas,
                           COLOR_0: o RGB é a cor acesa (sRGB, 0 a 1; nas placas
                           com textura ele multiplica a textura) e o A é a ordem
                           de acender (0 primeiro, 1 por último). Cada primitiva
                           com menos de 65.536 vértices (a raylib usa índices de
                           16 bits). Os fios dos varais e os braços e cúpulas das
                           luminárias são escuros e têm A = 0;
  luigi.json, curios.json  o nome, quando o lugar acende (acender_em, segundos)
                           e as luzes pontuais que ele joga no chão e nas paredes
                           (x, y, z do jogo, cor, raio em metros e força).

Com --previas, grava na pasta as vistas de conferência (Eevee), da rua, à
altura de quem dirige: de noite (o mapa escuro azulado, as luzes emissivas, as
luzes do JSON e o halo, feito depois sobre a imagem) e de dia (as luzes como
vidro apagado: a cor × 0,3). As vistas usam os .glb já gravados.

Cada parte (o letreiro, cada telhado, cada aro, cada varal, cada placa...) é
uma malha à parte no .glb: o jogo confere, malha a malha, se ela está à vista
antes de desenhar o halo dela.

O mapa só tem os letreiros pintados em texturas pequenas; a ferramenta mede o
prédio de cada lugar no mapa (as peças soltas da malha, os UVs das placas e
raios contra as paredes, para ficar sempre 3 a 8 cm à frente delas) e
constrói as luzes em cima.

Luigi's Casa Della Tires (ror_Object_46, textura ror_Image_7):
  - o letreiro em arco em cima da porta: "Luigi's" em letra cursiva de neon
    rosa-avermelhado (Lobster), "CASA DELLA TIRES" em verde, branco e vermelho
    (Saira Condensed), as três faixas verde-branca-vermelha de cada lado, as
    duas linhas vermelhas embaixo e o neon vermelho no contorno do arco; as
    letras saem do texto do Blender rasterizado (a união das letras, sem os
    cruzamentos da cursiva), contornado, simplificado e virado tubo, na caixa
    das letras pintadas da placa;
  - o neon vermelho no alto dos telhados de telha das duas alas e na parede logo
    abaixo do beiral (o perfil do telhado medido com raios, interrompido nos
    pilares);
  - os aros de neon vermelho (dois círculos, como um pneu: um em volta da banda
    e um no lugar do aro) nos discos em cima dos pilares e do letreiro;
  - os varais de lampadinhas amarelas em catenária sobre o pátio, saindo do
    disco do letreiro e da torre de pneus para os pilares;
  - as arandelas (a lâmpada embaixo de cada cúpula da parede);
  - a torre de pneus: um disco de luz quente dentro de cada pneu em pé.
  A ordem: as arandelas; a torre, de baixo para cima; o neon dos telhados
  correndo do letreiro para as pontas; os aros; "Luigi's" se escrevendo da
  esquerda para a direita; CASA, DELLA e TIRES letra a letra; as faixas e as
  linhas; por fim as lampadinhas, do letreiro e da torre para os pilares.

Radiator Springs Curios (ror_Object_11, textura ror_Image_33):
  - as placas acesas: cópias das faces pintadas das placas (a grande RADIATOR
    SPRINGS CURIOS, SOUVENIRS BRIC-A-BRAC TRINKETS, WESTERN JUNK FOSSILS, as
    plaquinhas SALE TODAY ONLY, U WANT IT, WE GOT IT e RARE HUBCAPS e a placa
    pintada na parede do lado), 4 cm à frente, com a textura do mapa (em PNG);
  - os globos "PUMP" das bombas de gasolina, copiados 3 cm para fora;
  - as letras da placa grande feitas de lâmpadas: as três linhas medidas na
    pintura (a caixa das letras vermelhas de cada uma), as letras desenhadas
    de novo com a fonte do jogo (Saira Condensed) e esticadas na caixa,
    afinadas até virar um traço, com uma lâmpada a cada 30 cm ao longo dele;
  - o contorno de lâmpadas das duas placas do lado (correndo em volta);
  - luminárias pequenas (braço, cúpula e lâmpada) iluminando de frente as
    plaquinhas da varanda e a placa da parede.
  A ordem: os globos das bombas; as plaquinhas, uma a uma; a placa da parede;
  as placas do lado com o contorno correndo; por fim a placa grande, letra a
  letra, na ordem de leitura.

Eixos: no Blender, x = x do jogo, y = -z do jogo e z = y do jogo; o .glb sai
com +Y para cima, ou seja, nas coordenadas do jogo.
------------------------------------------------------------------
"""

import bpy
import bmesh
import json
import math
import os
import struct
import sys
from collections import defaultdict

import numpy as np
from mathutils.bvhtree import BVHTree


# ------------------------------------------------------------------
# 0. Argumentos e caminhos
# ------------------------------------------------------------------
argv = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
if len(argv) < 2:
    raise SystemExit('uso: Blender -b --factory-startup -P montar_luzes_luigi_e_curios.py -- mapa.glb pasta_de_saida [--previas pasta]')
MAPA = os.path.abspath(argv[0])
SAIDA = os.path.abspath(argv[1])
PREVIAS = os.path.abspath(argv[argv.index('--previas') + 1]) if '--previas' in argv else None
RAIZ = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
FONTES = os.path.join(RAIZ, 'recursos', 'fontes')
os.makedirs(SAIDA, exist_ok=True)

# ------------------------------------------------------------------
# 1. Medidas e cores
# ------------------------------------------------------------------
FOLGA = 0.035              # metros entre a superfície do tubo (ou da lâmpada) e a parede do mapa
MAX_VERTICES = 60000       # por primitiva: a raylib converte os índices para 16 bits
CIMA = np.array([0.0, 1.0, 0.0])

# Cores acesas (sRGB)
VERMELHO_NEON = (1.00, 0.17, 0.13)
ROSA_NEON = (1.00, 0.30, 0.44)
VERDE_NEON = (0.20, 1.00, 0.50)
BRANCO_NEON = (0.74, 0.80, 0.80)       # um branco frio e mais fraco: o branco cheio vira um borrão no halo
AMARELO_LAMPADA = (1.00, 0.80, 0.40)
LUZ_DA_TORRE = (1.00, 0.68, 0.34)
LUZ_DA_ARANDELA = (1.00, 0.84, 0.58)
LAMPADA_DE_LETREIRO = (1.00, 0.88, 0.58)
LAMPADA_DE_LUMINARIA = (1.00, 0.93, 0.78)
FIO = (0.07, 0.06, 0.05)             # os fios dos varais (apagados, A = 0)
METAL_ESCURO = (0.13, 0.12, 0.11)    # braços e cúpulas das luminárias (A = 0)
# O tom que multiplica a textura das placas copiadas: embaixo e em cima (a luz vem das lâmpadas e das
# luminárias de cima)
PLACA_ACESA = ((0.62, 0.52, 0.36), (0.72, 0.61, 0.43))       # as placas com lâmpadas: âmbar, as lâmpadas aparecem
PLAQUINHA_ACESA = ((0.62, 0.54, 0.40), (0.90, 0.80, 0.62))   # as plaquinhas iluminadas de frente
PAREDE_ACESA = ((0.30, 0.27, 0.22), (0.80, 0.74, 0.60))      # a placa pintada na parede, só com as luminárias
GLOBO_ACESO = (1.00, 0.95, 0.84)                             # os globos das bombas

# Raios dos tubos (metros)
TUBO_DAS_LETRAS = 0.025
TUBO_DA_CURSIVA = 0.030
TUBO_DO_CONTORNO = 0.035
TUBO_DOS_AROS = 0.040
LADOS_DO_TUBO = 8
RAIO_DO_FIO = 0.008
RAIO_DA_LAMPADINHA = 0.075   # varais do Luigi
RAIO_DA_LAMPADA = 0.075      # letras da placa grande do Curios
RAIO_DO_CONTORNO = 0.065     # contorno das placas do lado do Curios

# ------------------------------------------------------------------
# 2. O mapa
# ------------------------------------------------------------------
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=MAPA)


def unit(v):
    v = np.asarray(v, float)
    n = np.linalg.norm(v, axis=-1, keepdims=True)
    return v / np.maximum(n, 1e-12)


def para_blender(p):
    p = np.asarray(p, float)
    return np.stack([p[..., 0], -p[..., 2], p[..., 1]], -1)


def imagem_do_material(mat):
    if mat is None or mat.node_tree is None:
        return None
    for n in mat.node_tree.nodes:
        if n.type == 'TEX_IMAGE' and n.image:
            return n.image
    return None


class Pedaco:
    """Os triângulos de objetos do mapa com o centro dentro de uma caixa do chão (x0, x1, z0, z1), nos eixos do
    jogo, com os UVs (os do Blender: v de baixo para cima), o nome da imagem e a peça solta de cada um (as
    faces ligadas por vértices; com soldar, pelos vértices no mesmo lugar)"""

    def __init__(self, nomes, caixa, soldar=False):
        tris, uvs, imgs, chaves = [], [], [], []
        base = 0
        for nome in nomes:
            ob = bpy.data.objects[nome]
            me = ob.data
            mw = np.array(ob.matrix_world)
            co = np.empty(len(me.vertices) * 3)
            me.vertices.foreach_get('co', co)
            co = co.reshape(-1, 3) @ mw[:3, :3].T + mw[:3, 3]
            g = np.stack([co[:, 0], co[:, 2], -co[:, 1]], 1)
            me.calc_loop_triangles()
            nt = len(me.loop_triangles)
            vi = np.empty(nt * 3, np.int64)
            me.loop_triangles.foreach_get('vertices', vi)
            vi = vi.reshape(-1, 3)
            li = np.empty(nt * 3, np.int64)
            me.loop_triangles.foreach_get('loops', li)
            li = li.reshape(-1, 3)
            mi = np.empty(nt, np.int64)
            me.loop_triangles.foreach_get('material_index', mi)
            if me.uv_layers.active:
                uvd = np.empty(len(me.loops) * 2)
                me.uv_layers.active.data.foreach_get('uv', uvd)
                uvd = uvd.reshape(-1, 2)
            else:
                uvd = np.zeros((len(me.loops), 2))
            T = g[vi]
            c = T.mean(1)
            dentro = (c[:, 0] >= caixa[0]) & (c[:, 0] <= caixa[1]) & (c[:, 2] >= caixa[2]) & (c[:, 2] <= caixa[3])
            nomes_img = [(imagem_do_material(m).name if imagem_do_material(m) else '') for m in me.materials] or ['']
            tris.append(T[dentro])
            uvs.append(uvd[li[dentro]])
            imgs += [nomes_img[min(k, len(nomes_img) - 1)] for k in mi[dentro]]
            if soldar:
                chaves.append(np.round(T[dentro].reshape(-1, 3), 3))
            else:
                chaves.append((vi[dentro] + base).reshape(-1, 1).astype(float))
            base += len(g)
        self.tri = np.concatenate(tris)
        self.uv = np.concatenate(uvs)
        self.img = np.array(imgs)
        n = np.cross(self.tri[:, 1] - self.tri[:, 0], self.tri[:, 2] - self.tri[:, 0])
        self.area = np.linalg.norm(n, axis=1) / 2
        self.normal = unit(n)
        # peças soltas: rótulos que se espalham pelos vértices em comum
        _, ids = np.unique(np.concatenate(chaves), axis=0, return_inverse=True)
        ids = ids.reshape(-1, 3)
        rot = np.arange(ids.max() + 1)
        while True:
            m = rot[ids].min(1)
            novo = rot.copy()
            for k in range(3):
                np.minimum.at(novo, ids[:, k], m)
            novo = novo[novo]
            if np.array_equal(novo, rot):
                break
            rot = novo
        self.peca = rot[ids[:, 0]]
        self._bvh = None

    def bvh(self):
        if self._bvh is None:
            V = self.tri.reshape(-1, 3)
            self._bvh = BVHTree.FromPolygons([tuple(p) for p in V], [(3 * k, 3 * k + 1, 3 * k + 2) for k in range(len(self.tri))])
        return self._bvh

    def raio(self, origem, direcao, alcance=3.0):
        """O primeiro ponto atingido (eixos do jogo) e a distância, ou (None, None)"""
        hit = self.bvh().ray_cast(tuple(map(float, origem)), tuple(map(float, unit(direcao))), alcance)
        if hit[0] is None:
            return None, None
        return np.array(hit[0]), hit[3]

    def da_peca(self, p):
        return np.where(self.peca == p)[0]

    def pecas(self):
        ids, n = np.unique(self.peca, return_counts=True)
        return [(i, c) for i, c in zip(ids, n)]


def uv_do_px(px, py, W, H):
    """Pixel do PNG (linha 0 em cima) para o UV do Blender (v de baixo para cima)"""
    return np.stack([np.asarray(px, float) / W, 1.0 - np.asarray(py, float) / H], -1)


def no_uv(tri, uv, pts):
    """Para cada ponto de UV, o ponto 3D (eixos do jogo) do triângulo que o contém (ou do mais perto) e a
    normal desse triângulo"""
    pts = np.atleast_2d(np.asarray(pts, float))
    a, b, c = uv[:, 0], uv[:, 1], uv[:, 2]
    v0, v1 = b - a, c - a
    d00 = (v0 * v0).sum(1)
    d01 = (v0 * v1).sum(1)
    d11 = (v1 * v1).sum(1)
    den = d00 * d11 - d01 * d01
    ok = np.abs(den) > 1e-14
    den = np.where(ok, den, 1.0)
    v2 = pts[:, None, :] - a[None]
    d20 = (v2 * v0[None]).sum(2)
    d21 = (v2 * v1[None]).sum(2)
    l1 = (d11[None] * d20 - d01[None] * d21) / den[None]
    l2 = (d00[None] * d21 - d01[None] * d20) / den[None]
    l0 = 1 - l1 - l2
    nota = np.minimum(np.minimum(l0, l1), l2)
    nota[:, ~ok] = -1e9
    k = nota.argmax(1)
    r = np.arange(len(pts))
    P = l0[r, k, None] * tri[k, 0] + l1[r, k, None] * tri[k, 1] + l2[r, k, None] * tri[k, 2]
    n = unit(np.cross(tri[k, 1] - tri[k, 0], tri[k, 2] - tri[k, 0]))
    return P, n


def pixels_da_imagem(nome):
    """Os pixels de uma imagem do mapa (linhas de cima para baixo, como no PNG; os valores do PNG, sRGB)"""
    im = bpy.data.images[nome]
    W, H = im.size
    px = np.empty(W * H * 4, np.float32)
    im.pixels.foreach_get(px)
    return px.reshape(H, W, 4)[::-1].copy()


# ------------------------------------------------------------------
# 3. O que brilha: tubos, lâmpadas, discos e cópias com textura
# ------------------------------------------------------------------
class Brilho:
    """As malhas que brilham num lugar: vértices (eixos do jogo), faces, cor por vértice (RGB a cor acesa,
    A a ordem de acender) e, nas faces com textura, os UVs. Uma lista de pedaços por parte (o letreiro, cada
    telhado, cada aro, cada varal...) e material: cada parte vira uma malha no .glb, e o jogo confere, malha
    a malha, se ela está à vista da câmera antes de desenhar o halo dela."""

    def __init__(self, nome):
        self.nome = nome
        self.parte = 'geral'
        self.pedacos = defaultdict(list)

    def juntar(self, material, V, F, cor, ordem, UV=None):
        V = np.asarray(V, float)
        n = len(V)
        cor = np.asarray(cor, float)
        if cor.ndim == 1:
            cor = np.tile(cor[:3], (n, 1))
        ordem = np.asarray(ordem, float)
        if ordem.ndim == 0:
            ordem = np.full(n, float(ordem))
        C = np.concatenate([np.clip(cor[:, :3], 0, 1), np.clip(ordem, 0, 1)[:, None]], 1)
        self.pedacos[(self.parte, material)].append((V, [tuple(f) for f in F], C, None if UV is None else np.asarray(UV, float)))

    def contar(self):
        v = sum(len(p[0]) for ps in self.pedacos.values() for p in ps)
        t = sum(len(f) - 2 for ps in self.pedacos.values() for p in ps for f in p[1])
        return v, t


def limpar(P, fechado=False, tol=1e-4):
    P = np.asarray(P, float)
    if len(P) < 2:
        return P, np.arange(len(P))
    manter = [0]
    for i in range(1, len(P)):
        if np.linalg.norm(P[i] - P[manter[-1]]) > tol:
            manter.append(i)
    if fechado and len(manter) > 2 and np.linalg.norm(P[manter[-1]] - P[manter[0]]) <= tol:
        manter.pop()
    return P[manter], np.array(manter)


def tubo(br, pontos, raio, cor, ordem, fechado=False, lados=LADOS_DO_TUBO, tampas=True, material='luz'):
    """Um tubo redondo em volta da linha de pontos (eixos do jogo), com os anéis girados por transporte
    paralelo (num laço fechado, a torção que sobra é repartida pelo caminho). cor e ordem: um valor ou um
    por ponto. As faces apontam para fora."""
    P, idx = limpar(pontos, fechado)
    n = len(P)
    if n < 2:
        return
    cor = np.asarray(cor, float)
    cor = cor[idx] if cor.ndim == 2 else np.tile(cor[:3], (n, 1))
    ordem = np.asarray(ordem, float)
    ordem = ordem[idx] if ordem.ndim == 1 else np.full(n, float(ordem))
    if fechado:
        T = P[np.r_[1:n, 0]] - P[np.r_[n - 1, 0:n - 1]]
    else:
        T = np.empty_like(P)
        T[1:-1] = P[2:] - P[:-2]
        T[0] = P[1] - P[0]
        T[-1] = P[-1] - P[-2]
    T = unit(T)
    N = np.empty_like(P)
    a = np.cross(T[0], CIMA) if abs(T[0][1]) < 0.9 else np.cross(T[0], [1.0, 0.0, 0.0])
    N[0] = unit(a)
    for i in range(1, n):
        v = N[i - 1] - np.dot(N[i - 1], T[i]) * T[i]
        if np.linalg.norm(v) < 1e-6:
            v = np.cross(T[i], CIMA) if abs(T[i][1]) < 0.9 else np.cross(T[i], [1.0, 0.0, 0.0])
        N[i] = unit(v)
    if fechado:
        v = unit(N[-1] - np.dot(N[-1], T[0]) * T[0])
        fi = math.atan2(np.dot(np.cross(T[0], v), N[0]), np.dot(v, N[0]))
        for i in range(n):
            ang = fi * i / n
            N[i] = N[i] * math.cos(ang) + np.cross(T[i], N[i]) * math.sin(ang)
    B = np.cross(T, N)
    th = 2 * np.pi * np.arange(lados) / lados
    anel = P[:, None, :] + raio * (np.cos(th)[None, :, None] * N[:, None, :] + np.sin(th)[None, :, None] * B[:, None, :])
    V = list(anel.reshape(-1, 3))
    Cs = list(np.repeat(cor, lados, 0))
    Os = list(np.repeat(ordem, lados))
    F = []
    for i in range(n if fechado else n - 1):
        i2 = (i + 1) % n
        for k in range(lados):
            k2 = (k + 1) % lados
            F.append((i * lados + k, i * lados + k2, i2 * lados + k2, i2 * lados + k))
    if not fechado and tampas:
        for ponta, i in ((0, 0), (1, n - 1)):
            c = len(V)
            V.append(P[i])
            Cs.append(cor[i])
            Os.append(ordem[i])
            for k in range(lados):
                k2 = (k + 1) % lados
                F.append((c, i * lados + k2, i * lados + k) if ponta == 0 else (c, i * lados + k, i * lados + k2))
    br.juntar(material, V, F, np.array(Cs), np.array(Os))


def _icosfera():
    """Uma icosfera de raio 1 com uma subdivisão (42 vértices, 80 triângulos)"""
    for sub in (1, 2):
        bm = bmesh.new()
        bmesh.ops.create_icosphere(bm, subdivisions=sub, radius=1.0)
        if len(bm.verts) == 42:
            V = np.array([v.co[:] for v in bm.verts])
            F = [tuple(v.index for v in f.verts) for f in bm.faces]
            bm.free()
            # Blender (x, y, z) -> jogo (x, z, -y): uma rotação, as faces continuam para fora
            return np.stack([V[:, 0], V[:, 2], -V[:, 1]], 1), F
        bm.free()
    raise SystemExit('icosfera: não achei a de 42 vértices')


ICOSFERA_V, ICOSFERA_F = _icosfera()


def lampada(br, centro, raio, cor, ordem, material='luz'):
    br.juntar(material, np.asarray(centro, float) + raio * ICOSFERA_V, ICOSFERA_F, cor, ordem)


def base_do_plano(n):
    n = unit(n)
    a = np.cross(n, CIMA) if abs(n[1]) < 0.9 else np.cross(n, [1.0, 0.0, 0.0])
    e1 = unit(a)
    e2 = np.cross(n, e1)
    return e1, e2


def disco(br, centro, normal, raio, cor, ordem, lados=16, duplo=True, material='luz'):
    """Um disco virado para a normal (com duplo, também para o outro lado)"""
    e1, e2 = base_do_plano(normal)
    th = 2 * np.pi * np.arange(lados) / lados
    anel = np.asarray(centro, float) + raio * (np.cos(th)[:, None] * e1 + np.sin(th)[:, None] * e2)
    V = np.concatenate([[centro], anel])
    F = [(0, 1 + k, 1 + (k + 1) % lados) for k in range(lados)]   # e1 x e2 = normal: para a frente
    br.juntar(material, V, F, cor, ordem)
    if duplo:
        br.juntar(material, V, [(f[0], f[2], f[1]) for f in F], cor, ordem)


def catenaria(a, b, flecha, n):
    t = np.linspace(0, 1, n)
    P = a[None] + (b - a)[None] * t[:, None]
    P[:, 1] -= 4 * flecha * t * (1 - t)
    return P


def reamostrar(P, passo, fechado=False):
    """Pontos a cada passo (pelo comprimento) ao longo da linha"""
    P = np.asarray(P, float)
    Q = np.concatenate([P, P[:1]]) if fechado else P
    d = np.r_[0, np.cumsum(np.linalg.norm(np.diff(Q, axis=0), axis=1))]
    total = d[-1]
    k = max(1, int(round(total / passo)))
    s = np.linspace(0, total, k + (0 if fechado else 1), endpoint=not fechado)
    return np.stack([np.interp(s, d, Q[:, j]) for j in range(Q.shape[1])], 1), s / max(total, 1e-9)


# ------------------------------------------------------------------
# 4. Contornos: letras, grades e traços
# ------------------------------------------------------------------
_fontes = {}


def fonte(arquivo):
    if arquivo not in _fontes:
        _fontes[arquivo] = bpy.data.fonts.load(os.path.join(FONTES, arquivo))
    return _fontes[arquivo]


def triangulos_do_texto(texto, arquivo):
    """Os triângulos (2D, unidades da fonte) do texto preenchido, pelo texto do Blender"""
    cu = bpy.data.curves.new('texto', 'FONT')
    cu.font = fonte(arquivo)
    cu.body = texto
    cu.size = 1.0
    cu.resolution_u = 16
    cu.fill_mode = 'BOTH'
    ob = bpy.data.objects.new('texto', cu)
    bpy.context.scene.collection.objects.link(ob)
    dg = bpy.context.evaluated_depsgraph_get()
    ev = ob.evaluated_get(dg)
    me = ev.to_mesh()
    me.calc_loop_triangles()
    V = np.array([v.co[:2] for v in me.vertices])
    T = np.array([t.vertices[:] for t in me.loop_triangles])
    ev.to_mesh_clear()
    bpy.data.objects.remove(ob)
    bpy.data.curves.remove(cu)
    return V[T]


def rasterizar(tris, x0, y0, passo, W, H):
    """A grade (H linhas de baixo para cima, W colunas) com as células cujo centro cai num dos triângulos"""
    M = np.zeros((H, W), bool)
    for t in tris:
        (ax, ay), (bx, by), (cx, cy) = t
        i0 = max(0, int((min(ay, by, cy) - y0) / passo) - 1)
        i1 = min(H, int((max(ay, by, cy) - y0) / passo) + 2)
        j0 = max(0, int((min(ax, bx, cx) - x0) / passo) - 1)
        j1 = min(W, int((max(ax, bx, cx) - x0) / passo) + 2)
        if i1 <= i0 or j1 <= j0:
            continue
        ys = y0 + (np.arange(i0, i1) + 0.5) * passo
        xs = x0 + (np.arange(j0, j1) + 0.5) * passo
        X, Y = np.meshgrid(xs, ys)
        e0 = (bx - ax) * (Y - ay) - (by - ay) * (X - ax)
        e1 = (cx - bx) * (Y - by) - (cy - by) * (X - bx)
        e2 = (ax - cx) * (Y - cy) - (ay - cy) * (X - cx)
        dentro = ((e0 >= 0) & (e1 >= 0) & (e2 >= 0)) | ((e0 <= 0) & (e1 <= 0) & (e2 <= 0))
        M[i0:i1, j0:j1] |= dentro
    return M


def lacos_da_grade(M):
    """As bordas das regiões cheias da grade (linha 0 embaixo), como laços de cantos de célula: os de fora no
    sentido anti-horário, os buracos no horário. Onde duas regiões só se tocam na diagonal, ficam separadas."""
    P = np.pad(M, 1)
    ii, jj = np.nonzero(P)
    ar = []
    s = ~P[ii - 1, jj]
    ar.append(np.stack([jj[s], ii[s], jj[s] + 1, ii[s]], 1))
    s = ~P[ii, jj + 1]
    ar.append(np.stack([jj[s] + 1, ii[s], jj[s] + 1, ii[s] + 1], 1))
    s = ~P[ii + 1, jj]
    ar.append(np.stack([jj[s] + 1, ii[s] + 1, jj[s], ii[s] + 1], 1))
    s = ~P[ii, jj - 1]
    ar.append(np.stack([jj[s], ii[s] + 1, jj[s], ii[s]], 1))
    E = np.concatenate(ar).tolist()
    saindo = defaultdict(list)
    for k, (x0, y0, _, _) in enumerate(E):
        saindo[(x0, y0)].append(k)
    usado = [False] * len(E)
    lacos = []
    for k0 in range(len(E)):
        if usado[k0]:
            continue
        laco = []
        k = k0
        while not usado[k]:
            usado[k] = True
            x0, y0, x1, y1 = E[k]
            laco.append((x0, y0))
            cands = [c for c in saindo[(x1, y1)] if not usado[c]]
            if not cands:
                break
            if len(cands) > 1:
                dx, dy = x1 - x0, y1 - y0
                cands.sort(key=lambda c: dx * (E[c][3] - E[c][1]) - dy * (E[c][2] - E[c][0]))   # a mais à direita
            k = cands[0]
        if len(laco) >= 4:
            lacos.append(np.array(laco, float) - 1.0)
    return lacos


def area_assinada(P):
    x, y = P[:, 0], P[:, 1]
    return 0.5 * float(np.sum(x * np.roll(y, -1) - np.roll(x, -1) * y))


def _dp(P, tol):
    """Douglas-Peucker numa linha aberta"""
    manter = np.zeros(len(P), bool)
    manter[0] = manter[-1] = True
    pilha = [(0, len(P) - 1)]
    while pilha:
        a, b = pilha.pop()
        if b <= a + 1:
            continue
        d = P[b] - P[a]
        L = np.linalg.norm(d)
        Q = P[a + 1:b] - P[a]
        dist = np.abs(Q[:, 0] * d[1] - Q[:, 1] * d[0]) / L if L > 1e-12 else np.linalg.norm(Q, axis=1)
        k = int(dist.argmax())
        if dist[k] > tol:
            m = a + 1 + k
            manter[m] = True
            pilha += [(a, m), (m, b)]
    return P[manter]


def simplificar(P, tol, fechado=True):
    if not fechado:
        return _dp(P, tol)
    k = int(np.linalg.norm(P - P[0], axis=1).argmax())
    A = _dp(np.concatenate([P[:k + 1]]), tol)
    B = _dp(np.concatenate([P[k:], P[:1]]), tol)
    return np.concatenate([A[:-1], B[:-1]])


def chaikin(P, vezes, fechado=True):
    for _ in range(vezes):
        if fechado:
            Q = np.roll(P, -1, 0)
            A, B = 0.75 * P + 0.25 * Q, 0.25 * P + 0.75 * Q
            P = np.stack([A, B], 1).reshape(-1, P.shape[1])
        else:
            A, B = 0.75 * P[:-1] + 0.25 * P[1:], 0.25 * P[:-1] + 0.75 * P[1:]
            P = np.concatenate([P[:1], np.stack([A, B], 1).reshape(-1, P.shape[1]), P[-1:]])
    return P


def dentro_do_poligono(p, P):
    x, y = p
    xs, ys = P[:, 0], P[:, 1]
    x2, y2 = np.roll(xs, -1), np.roll(ys, -1)
    cruza = ((ys > y) != (y2 > y)) & (x < (x2 - xs) * (y - ys) / np.where(y2 != ys, y2 - ys, 1e-12) + xs)
    return bool(cruza.sum() % 2)


def contornos_de_texto(texto, arquivo, linhas=360, suavizar=2):
    """Os contornos do texto (a união das letras, sem os cruzamentos das letras ligadas), em unidades da fonte:
    uma lista de letras, cada uma uma lista de laços (o de fora primeiro, depois os buracos), da esquerda para
    a direita, e a caixa (x0, x1, y0, y1) do texto"""
    T = triangulos_do_texto(texto, arquivo)
    pts = T.reshape(-1, 2)
    x0, y0 = pts.min(0)
    x1, y1 = pts.max(0)
    passo = (y1 - y0) / linhas
    m = 4
    W = int((x1 - x0) / passo) + 2 * m
    H = int((y1 - y0) / passo) + 2 * m
    M = rasterizar(T, x0 - m * passo, y0 - m * passo, passo, W, H)
    lacos = []
    for L in lacos_da_grade(M):
        a = area_assinada(L)
        if abs(a) < 6:   # sujeira de poucos pixels
            continue
        L = simplificar(L, 0.7, True)
        if len(L) < 3:
            continue
        L = chaikin(L, suavizar, True)
        L = L * passo + [x0 - m * passo, y0 - m * passo]
        lacos.append((a, L))
    fora = [L for a, L in lacos if a > 0]
    buracos = [L for a, L in lacos if a < 0]
    letras = [[L] for L in fora]
    for B in buracos:
        donos = [k for k, L in enumerate(fora) if dentro_do_poligono(B[0], L)]
        if donos:
            k = min(donos, key=lambda k: abs(area_assinada(fora[k])))
            letras[k].append(B)
    letras.sort(key=lambda ls: ls[0][:, 0].min())
    return letras, (x0, x1, y0, y1)


def afinar(M):
    """O esqueleto da máscara (Zhang-Suen): um traço de um pixel no meio de cada risco"""
    img = M.astype(np.uint8).copy()
    while True:
        mudou = False
        for passo in (0, 1):
            P = np.pad(img, 1)
            p2, p3, p4 = P[:-2, 1:-1], P[:-2, 2:], P[1:-1, 2:]
            p5, p6, p7 = P[2:, 2:], P[2:, 1:-1], P[2:, :-2]
            p8, p9 = P[1:-1, :-2], P[:-2, :-2]
            seq = [p2, p3, p4, p5, p6, p7, p8, p9, p2]
            B = p2 + p3 + p4 + p5 + p6 + p7 + p8 + p9
            A = sum(((seq[k] == 0) & (seq[k + 1] == 1)).astype(np.uint8) for k in range(8))
            if passo == 0:
                c = (p2 * p4 * p6 == 0) & (p4 * p6 * p8 == 0)
            else:
                c = (p2 * p4 * p8 == 0) & (p2 * p6 * p8 == 0)
            tirar = (img == 1) & (B >= 2) & (B <= 6) & (A == 1) & c
            if tirar.any():
                img[tirar] = 0
                mudou = True
        if not mudou:
            break
    return img.astype(bool)


def rotular(M, oito=True):
    """Os componentes ligados da máscara (rótulo 0 é o fundo)"""
    rot = np.zeros(M.shape, np.int32)
    H, W = M.shape
    viz = [(-1, 0), (1, 0), (0, -1), (0, 1)] + ([(-1, -1), (-1, 1), (1, -1), (1, 1)] if oito else [])
    n = 0
    for i0, j0 in zip(*np.nonzero(M)):
        if rot[i0, j0]:
            continue
        n += 1
        fila = [(i0, j0)]
        rot[i0, j0] = n
        while fila:
            i, j = fila.pop()
            for di, dj in viz:
                a, b = i + di, j + dj
                if 0 <= a < H and 0 <= b < W and M[a, b] and not rot[a, b]:
                    rot[a, b] = n
                    fila.append((a, b))
    return rot, n


def faixas(m, folga=3):
    """Os trechos (início, fim) em que a lista de verdadeiros/falsos é verdadeira, juntando os separados por
    até `folga` falsos"""
    idx = np.where(m)[0]
    if len(idx) == 0:
        return []
    out = [[idx[0], idx[0]]]
    for i in idx[1:]:
        if i - out[-1][1] <= folga + 1:
            out[-1][1] = i
        else:
            out.append([i, i])
    return [tuple(f) for f in out]


def caminhar_no_esqueleto(S):
    """Os pixels do esqueleto numa ordem que segue os traços: começando nas pontas, em profundidade"""
    pts = set(zip(*np.nonzero(S)))
    viz = [(-1, 0), (1, 0), (0, -1), (0, 1), (-1, -1), (-1, 1), (1, -1), (1, 1)]

    def vizinhos(p):
        return [(p[0] + a, p[1] + b) for a, b in viz if (p[0] + a, p[1] + b) in pts]

    ordem = []
    visto = set()
    pontas = sorted(p for p in pts if len(vizinhos(p)) == 1)
    for inicio in pontas + sorted(pts):
        if inicio in visto:
            continue
        pilha = [inicio]
        while pilha:
            p = pilha.pop()
            if p in visto:
                continue
            visto.add(p)
            ordem.append(p)
            vs = [v for v in vizinhos(p) if v not in visto]
            # os vizinhos de lado primeiro na pilha, os da diagonal por último (seguem o traço reto)
            vs.sort(key=lambda v: abs(v[0] - p[0]) + abs(v[1] - p[1]), reverse=True)
            pilha += vs
    return ordem


def espalhar(pontos, distancia):
    """Escolhe, na ordem dada, os pontos a pelo menos `distancia` de todos os já escolhidos"""
    escolhidos = []
    grade = defaultdict(list)
    for k, p in enumerate(pontos):
        c = tuple(np.floor(p / distancia).astype(int))
        perto = False
        for dx in (-1, 0, 1):
            for dy in (-1, 0, 1):
                for dz in (-1, 0, 1):
                    for q in grade[(c[0] + dx, c[1] + dy, c[2] + dz)]:
                        if np.linalg.norm(pontos[q] - p) < distancia:
                            perto = True
                            break
                    if perto:
                        break
                if perto:
                    break
            if perto:
                break
        if not perto:
            escolhidos.append(k)
            grade[c].append(k)
    return escolhidos


# ------------------------------------------------------------------
# 5. Luigi's Casa Della Tires
# ------------------------------------------------------------------
LUIGI_CAIXA = (436.0, 492.0, 140.0, 190.0)        # x0, x1, z0, z1 do jogo
TORRE_CAIXA = (476.0, 494.0, 138.0, 158.0)
TEXTURA_DO_LUIGI = 'ror_Image_7'
# A placa em arco, pintada na textura do prédio (pixels do PNG, linha 0 em cima; a pintura está de cabeça
# para baixo): as caixas das letras pintadas, onde entram as de neon
CASA_PX = (25.0, 110.0, 160.5, 168.0)              # "CASA DELLA TIRES"
LUIGI_PX = (40.0, 90.0, 172.0, 199.0)              # "Luigis", com a laçada do L
LARGURA_DA_CURSIVA = 5.0                           # metros: "Luigi's" de neon (a pintada tem 4,6 m)
ALTURA_DO_CASA = 0.78                              # metros: as letras de "CASA DELLA TIRES"
# As faces com telhado de telha (vistas da rua e do pátio): duas pontas no chão do jogo (x, z) e a direção
# de fora. Os pilares, que passam na frente, cortam os tubos sozinhos (pelos raios).
TELHADOS = [
    ('frente da ala da rua principal', (455.2, 146.45), (470.9, 146.45), (0.0, -1.0)),
    ('lado da ala da rua principal, para o pátio', (470.7, 146.6), (470.7, 157.6), (1.0, 0.0)),
    ('frente da ala da travessa, para o pátio', (478.6, 167.2), (487.9, 167.2), (0.0, -1.0)),
    ('lado da ala da travessa, para a travessa', (487.8, 167.1), (487.8, 178.5), (1.0, 0.0)),
]
# Os varais: do disco em cima do letreiro ('centro') e da torre ('torre') para os pilares (o disco mais perto
# do ponto dado no chão)
VARAIS = [
    ('centro', 'torre'), ('centro', (470.4, 146.8)), ('centro', (487.6, 167.4)),
    ('torre', (470.4, 157.3)), ('torre', (478.8, 167.5)), ('torre', (470.4, 146.8)), ('torre', (487.6, 167.4)),
]
FLECHA_DO_VARAL = 0.075        # da distância
ENTRE_LAMPADINHAS = 0.60       # metros


def medir_luigi():
    predio = Pedaco(['ror_Object_46'], LUIGI_CAIXA)
    torre = Pedaco(['ror_torre_de_pneus_a', 'ror_torre_de_pneus_b'], TORRE_CAIXA, soldar=True)
    W, H = bpy.data.images[TEXTURA_DO_LUIGI].size
    # a placa: a peça cujo UV tem o meio de "CASA DELLA TIRES", virada de lado (não o teto)
    uv_meio = uv_do_px((CASA_PX[0] + CASA_PX[1]) / 2, (CASA_PX[2] + CASA_PX[3]) / 2, W, H)
    cand = []
    for k in np.where(predio.img == TEXTURA_DO_LUIGI)[0]:
        a, b, c = predio.uv[k]
        v0, v1, v2 = b - a, c - a, uv_meio - a
        den = v0[0] * v1[1] - v0[1] * v1[0]
        if abs(den) < 1e-12:
            continue
        l1 = (v2[0] * v1[1] - v2[1] * v1[0]) / den
        l2 = (v0[0] * v2[1] - v0[1] * v2[0]) / den
        if l1 >= -1e-6 and l2 >= -1e-6 and l1 + l2 <= 1 + 1e-6 and abs(predio.normal[k][1]) < 0.3:
            cand.append(k)
    if not cand:
        raise SystemExit('Luigi: não achei a placa em arco na textura do prédio')
    tris_placa = predio.da_peca(predio.peca[cand[0]])
    N = (predio.normal[tris_placa] * predio.area[tris_placa, None]).sum(0)
    N[1] = 0
    N = unit(N)
    R = np.cross(CIMA, N)                       # a direita de quem olha a placa
    V = predio.tri[tris_placa].reshape(-1, 3)
    centro = V.mean(0)
    base_y = V[:, 1].min()
    O = np.array([centro[0], base_y, centro[2]])

    def sy(p):
        p = np.atleast_2d(p) - O
        return np.stack([p @ R, p[:, 1] + base_y], 1)

    # o contorno de cima do arco: os vértices acima da base, da esquerda para a direita
    s_y = np.unique(np.round(sy(V), 3), axis=0)
    topo = s_y[s_y[:, 1] > base_y + 0.4]
    topo = topo[np.argsort(topo[:, 0])]
    lados = s_y[s_y[:, 1] < base_y + 0.4]
    placa = dict(N=N, R=R, O=O, base_y=base_y, topo=topo, s0=lados[:, 0].min(), s1=lados[:, 0].max(),
                 tris=tris_placa)

    def caixa_px(cx):
        cantos = uv_do_px([cx[0], cx[1], cx[0], cx[1]], [cx[2], cx[2], cx[3], cx[3]], W, H)
        P, _ = no_uv(predio.tri[tris_placa], predio.uv[tris_placa], cantos)
        q = sy(P)
        return q[:, 0].min(), q[:, 0].max(), q[:, 1].min(), q[:, 1].max()

    placa['casa'] = caixa_px(CASA_PX)
    placa['luigi'] = caixa_px(LUIGI_PX)

    # os discos em cima dos pilares: peças em anel (umas 40 faces, uns 2 m de altura, no alto)
    aneis = []
    for p, n in predio.pecas():
        if not (30 <= n <= 60):
            continue
        ks = predio.da_peca(p)
        Vp = np.unique(np.round(predio.tri[ks].reshape(-1, 3), 4), axis=0)
        if Vp[:, 1].min() < 9.5 or not (1.6 < np.ptp(Vp[:, 1]) < 2.8):
            continue
        c = Vp.mean(0)
        X = Vp - c
        w, vec = np.linalg.eigh(X.T @ X)
        eixo = vec[:, 0]
        if abs(eixo[1]) > 0.3:
            continue
        rr = np.linalg.norm(X - np.outer(X @ eixo, eixo), axis=1)
        aneis.append(dict(c=c, eixo=unit(eixo), R=float(rr.max()), meia=float(np.abs(X @ eixo).max())))
    if len(aneis) < 3:
        raise SystemExit('Luigi: achei só %d discos nos pilares' % len(aneis))

    # as cúpulas das arandelas: o fundo chato de cada uma (umas 10 faces, 0,8 a 1,3 m de largura)
    arandelas = []
    for p, n in predio.pecas():
        if not (6 <= n <= 16):
            continue
        Vp = predio.tri[predio.da_peca(p)].reshape(-1, 3)
        larg = max(np.ptp(Vp[:, 0]), np.ptp(Vp[:, 2]))
        if np.ptp(Vp[:, 1]) < 0.05 and 0.7 < larg < 1.4 and 6.5 < Vp[:, 1].mean() < 8.5:
            c = Vp.mean(0)
            arandelas.append(dict(c=c, r=float(np.linalg.norm(Vp[:, [0, 2]] - c[[0, 2]], axis=1).max())))

    # os pneus em pé da torre: as peças em anel com o eixo deitado
    pneus = []
    for p, n in torre.pecas():
        if n < 400:
            continue
        Vp = np.unique(np.round(torre.tri[torre.da_peca(p)].reshape(-1, 3), 3), axis=0)
        c = Vp.mean(0)
        X = Vp - c
        w, vec = np.linalg.eigh(X.T @ X)
        eixo = unit(vec[:, 0])
        rr = np.linalg.norm(X - np.outer(X @ eixo, eixo), axis=1)
        if abs(eixo[1]) < 0.5 and rr.min() > 0.2:
            pneus.append(dict(c=c, eixo=eixo, aro=float(rr.min()), fora=float(rr.max())))
    VT = torre.tri.reshape(-1, 3)
    return predio, torre, placa, aneis, arandelas, pneus, VT


def montar_luigi():
    predio, torre, placa, aneis, arandelas, pneus, VT = medir_luigi()
    br = Brilho('luigi')
    N, R, O = placa['N'], placa['R'], placa['O']
    luzes = []
    print('Luigi: placa com a normal (%.3f, %.3f), %d discos, %d arandelas, %d pneus em pé na torre'
          % (N[0], N[2], len(aneis), len(arandelas), len(pneus)))

    def na_placa(pts, raio):
        """Pontos (s, y) da placa para o mundo, à frente da superfície que os raios acham"""
        pts = np.atleast_2d(pts)
        out = []
        for s, y in pts:
            p = O + s * R + (y - O[1]) * CIMA
            hit, _ = predio.raio(p + N * 1.0, -N, 1.6)
            prof = np.dot(hit - O, N) if hit is not None else 0.075
            out.append(p + N * (prof - np.dot(p - O, N) + FOLGA + raio))
        return np.array(out)

    # --- o contorno do arco: 13 cm abaixo da borda de cima, de pilar a pilar
    br.parte = 'letreiro'
    linha = chaikin(placa['topo'].copy(), 3, fechado=False)
    linha[:, 1] -= 0.13
    # encolhe 25 cm nas pontas (os pilares)
    d = np.r_[0, np.cumsum(np.linalg.norm(np.diff(linha, axis=0), axis=1))]
    linha = linha[(d > 0.25) & (d < d[-1] - 0.25)]
    linha, _ = reamostrar(linha, 0.06)
    s_max = np.abs(linha[:, 0]).max()
    ordem_arco = 0.24 + 0.08 * np.abs(linha[:, 0]) / s_max
    tubo(br, na_placa(linha, TUBO_DO_CONTORNO), TUBO_DO_CONTORNO, VERMELHO_NEON, ordem_arco)

    # --- "Luigi's": cursiva, a união das letras, escrevendo-se da esquerda para a direita
    letras, (x0, x1, y0, y1) = contornos_de_texto("Luigi's", 'lobster.ttf', linhas=420, suavizar=2)
    cs0, cs1, cy0, cy1 = placa['luigi']
    esc = LARGURA_DA_CURSIVA / (x1 - x0)
    cx, cy = (cs0 + cs1) / 2, (cy0 + cy1) / 2
    for laco in [L for ls in letras for L in ls]:
        q = np.stack([cx + (laco[:, 0] - (x0 + x1) / 2) * esc, cy + (laco[:, 1] - (y0 + y1) / 2) * esc], 1)
        q, _ = reamostrar(q, 0.035, fechado=True)
        q = simplificar(q, 0.004, True)
        a = 0.55 + 0.11 * (q[:, 0] - (cx - LARGURA_DA_CURSIVA / 2)) / LARGURA_DA_CURSIVA
        tubo(br, na_placa(q, TUBO_DA_CURSIVA), TUBO_DA_CURSIVA, ROSA_NEON, a, fechado=True)
    print('Luigi: "Luigi\'s" com %.2f x %.2f m, %d contornos' % (LARGURA_DA_CURSIVA, (y1 - y0) * esc, sum(len(l) for l in letras)))

    # --- "CASA DELLA TIRES": letra a letra; as palavras pelos dois maiores vãos entre as letras
    letras, (x0, x1, y0, y1) = contornos_de_texto('CASA DELLA TIRES', 'saira_condensed_bold.ttf', linhas=300, suavizar=1)
    cs0, cs1, cy0, cy1 = placa['casa']
    esc = min(ALTURA_DO_CASA / (y1 - y0), (cs1 - cs0 + 0.3) / (x1 - x0))
    cx, cy = (cs0 + cs1) / 2, (cy0 + cy1) / 2
    ext = [(ls[0][:, 0].min(), ls[0][:, 0].max()) for ls in letras]
    vaos = [ext[k + 1][0] - ext[k][1] for k in range(len(ext) - 1)]
    quebras = sorted(np.argsort(vaos)[-2:])
    cores = (VERDE_NEON, BRANCO_NEON, VERMELHO_NEON)
    s_letras = []
    for k, ls in enumerate(letras):
        palavra = int(k > quebras[0]) + int(k > quebras[1])
        ordem = 0.70 + 0.10 * k / max(len(letras) - 1, 1) + 0.01 * palavra
        for laco in ls:
            q = np.stack([cx + (laco[:, 0] - (x0 + x1) / 2) * esc, cy + (laco[:, 1] - (y0 + y1) / 2) * esc], 1)
            q, _ = reamostrar(q, 0.03, fechado=True)
            q = simplificar(q, 0.003, True)
            tubo(br, na_placa(q, TUBO_DAS_LETRAS), TUBO_DAS_LETRAS, cores[palavra], ordem, fechado=True)
            s_letras.append(q[:, 0])
    s_letras = np.concatenate(s_letras)
    casa_s0, casa_s1 = s_letras.min(), s_letras.max()
    casa_y0 = cy - (y1 - y0) * esc / 2
    casa_y1 = cy + (y1 - y0) * esc / 2
    print('Luigi: "CASA DELLA TIRES" com %.2f x %.2f m (%d letras)' % (casa_s1 - casa_s0, casa_y1 - casa_y0, len(letras)))

    # --- as faixas verde, branca e vermelha dos dois lados e as duas linhas vermelhas embaixo
    margem = 0.30
    for lado, (a, b) in (('esquerda', (placa['s0'] + margem, casa_s0 - 0.22)), ('direita', (casa_s1 + 0.22, placa['s1'] - margem))):
        if b - a < 0.3:
            continue
        for f, cor in ((0.86, VERDE_NEON), (0.52, BRANCO_NEON), (0.18, VERMELHO_NEON)):
            y = casa_y0 + f * (casa_y1 - casa_y0)
            q = np.stack([np.linspace(a, b, 12), np.full(12, y)], 1)
            tubo(br, na_placa(q, TUBO_DAS_LETRAS), TUBO_DAS_LETRAS, cor, 0.82)
    for y, encolhe in ((casa_y0 - 0.13, 0.25), (placa['base_y'] - 0.10, 0.45)):
        a, b = placa['s0'] + encolhe, placa['s1'] - encolhe
        s = np.linspace(a, b, 60)
        q = np.stack([s, np.full(len(s), y)], 1)
        tubo(br, na_placa(q, TUBO_DAS_LETRAS), TUBO_DAS_LETRAS, VERMELHO_NEON, 0.84 + 0.04 * np.abs(s) / max(abs(a), abs(b)))

    # --- o neon dos telhados: no alto da telha e na parede logo abaixo do beiral
    centro_da_placa = O + CIMA * 2.0
    for i_telhado, (nome, pa, pb, fora) in enumerate(TELHADOS):
        br.parte = 'telhado_%d' % i_telhado
        pa, pb = np.array([pa[0], 0, pa[1]]), np.array([pb[0], 0, pb[1]])
        n = np.array([fora[0], 0.0, fora[1]])
        ao_longo = unit(pb - pa)
        comp = np.linalg.norm(pb - pa)
        # o perfil, com raios de fora para dentro a cada 2 cm de altura, em quatro lugares da face: a parede
        # (logo abaixo do telhado, onde não há janela nem arco), a telha que começa a sair dela (o beiral) e
        # sobe inclinada para trás até o cume (o último ponto atingido)
        ys = np.arange(7.0, 11.0, 0.02)
        beirais, cumes = [], []
        for f in (0.2, 0.37, 0.63, 0.8):
            base = pa + ao_longo * comp * f
            prof = []
            for y in ys:
                hit, dist = predio.raio(base + n * 2.0 + CIMA * y, -n, 4.0)
                prof.append(dist if hit is not None else np.inf)
            prof = np.array(prof)
            faixa = prof[(ys > 7.4) & (ys < 7.9) & np.isfinite(prof)]
            if len(faixa) < 5:
                continue
            parede = np.median(faixa)
            sai = np.where((ys > 7.4) & (prof < parede - 0.03))[0]
            if len(sai) == 0:
                continue
            beirais.append(ys[sai[0]])
            cumes.append(ys[np.isfinite(prof) & (prof < parede + 1.5)].max())
        if not beirais:
            print('Luigi: %s sem beiral, pulei' % nome)
            continue
        y_beiral, y_cume = float(np.median(beirais)), float(np.median(cumes))
        alturas = (('cume', y_cume - 0.07), ('parede', y_beiral - 0.10))
        for qual, y in alturas:
            # a linha, interrompida onde algo bem mais perto (um pilar) está na frente
            amostras = np.arange(0.05, comp - 0.05, 0.08)
            pts, dists = [], []
            for t in amostras:
                base = pa + ao_longo * t + CIMA * y
                hit, dist = predio.raio(base + n * 2.0, -n, 4.0)
                pts.append(base)
                dists.append(dist if hit is not None else np.inf)
            dists = np.array(dists)
            ref = np.median(dists[np.isfinite(dists)])
            bom = np.abs(dists - ref) < 0.10
            # fecha falhas de uma amostra e tira 12 cm de perto dos pilares
            grupos, atual = [], []
            for k, b in enumerate(bom):
                if b:
                    atual.append(k)
                elif atual:
                    grupos.append(atual)
                    atual = []
            if atual:
                grupos.append(atual)
            for g in grupos:
                if len(g) < 4:
                    continue
                t0, t1 = amostras[g[0]] + (0.12 if g[0] > 0 else 0.0), amostras[g[-1]] - (0.12 if g[-1] < len(amostras) - 1 else 0.0)
                if t1 - t0 < 0.3:
                    continue
                ts = np.linspace(t0, t1, max(2, int((t1 - t0) / 0.5) + 1))
                linha = np.array([pa + ao_longo * t + CIMA * y + n * (2.0 - ref + FOLGA + TUBO_DO_CONTORNO) for t in ts])
                # corre de perto do letreiro para longe
                dl = np.linalg.norm((linha - centro_da_placa)[:, [0, 2]], axis=1)
                ordem = 0.32 + 0.12 * (dl - 3.0) / 20.0
                tubo(br, linha, TUBO_DO_CONTORNO, VERMELHO_NEON, np.clip(ordem, 0.30, 0.46))
        print('Luigi: telhado %s: beiral a %.2f m, cume a %.2f m' % (nome, y_beiral, y_cume))

    # --- os aros de neon nos discos (um pneu de neon): um círculo em volta da banda, por fora, e outro no
    # meio, no lugar do aro; os dois no plano do meio do disco, para ficarem certos dos dois lados (o disco
    # do mapa é uma banda com uma face só)
    centro = max(aneis, key=lambda a: a['c'][1])
    for i_anel, an in enumerate(aneis):
        br.parte = 'aro_%d' % i_anel
        e1, e2 = base_do_plano(an['eixo'])
        dist = np.linalg.norm((an['c'] - centro['c'])[[0, 2]])
        ordem = 0.46 if an is centro else 0.48 + 0.05 * min(dist / 20.0, 1.0)
        for r in (an['R'] + FOLGA + TUBO_DOS_AROS, an['R'] * 0.55):
            th = np.linspace(0, 2 * np.pi, 40, endpoint=False)
            circ = an['c'] + r * (np.cos(th)[:, None] * e1 + np.sin(th)[:, None] * e2)
            tubo(br, circ, TUBO_DOS_AROS, VERMELHO_NEON, ordem, fechado=True)

    # --- a torre de pneus: luz quente dentro de cada pneu em pé, de baixo para cima
    if pneus:
        br.parte = 'torre'
        ys = np.array([p['c'][1] for p in pneus])
        for p in pneus:
            ordem = 0.04 + 0.16 * (p['c'][1] - ys.min()) / max(np.ptp(ys), 1e-6)
            disco(br, p['c'], p['eixo'], p['aro'] * 0.97, LUZ_DA_TORRE, ordem, lados=16)
        # do lado da torre virado para o pátio, a meia altura dos pneus acesos
        cen = np.array([p['c'] for p in pneus]).mean(0)
        sel = VT[np.abs(VT[:, 1] - 8.0) < 0.7]
        eixo = np.array([(sel[:, 0].min() + sel[:, 0].max()) / 2, 8.0, (sel[:, 2].min() + sel[:, 2].max()) / 2])
        d = unit((O - eixo) * [1, 0, 1])
        r = np.max((sel - eixo)[:, [0, 2]] @ d[[0, 2]])
        luzes.append(dict(p=eixo + d * (r + 0.6), cor=LUZ_DA_TORRE, raio=10.0, forca=0.9))

    # --- as arandelas
    for i_arandela, a in enumerate(arandelas):
        br.parte = 'arandela_%d' % i_arandela
        c = a['c']
        disco(br, c - CIMA * 0.015, -CIMA, a['r'] * 0.8, LUZ_DA_ARANDELA, 0.0, lados=16, duplo=False)
        lampada(br, c - CIMA * 0.13, 0.09, LUZ_DA_ARANDELA, 0.0)
        # a luz cai na parede e no chão: um pouco para fora da parede
        hit = None
        melhor = None
        for d in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            h, dist = predio.raio(c - CIMA * 0.3, np.array([d[0], 0.0, d[1]]), 1.5)
            if h is not None and (melhor is None or dist < melhor[1]):
                melhor = (np.array([d[0], 0.0, d[1]]), dist)
        fora = -melhor[0] if melhor else np.zeros(3)
        luzes.append(dict(p=c - CIMA * 0.4 + fora * 0.4, cor=LUZ_DA_ARANDELA, raio=7.0, forca=0.8))

    # --- os varais de lampadinhas
    def ancora(nome, outro_ponto):
        if nome == 'torre':
            h = 13.4
            sel = VT[np.abs(VT[:, 1] - h) < 0.7]
            c = np.array([(sel[:, 0].min() + sel[:, 0].max()) / 2, h, (sel[:, 2].min() + sel[:, 2].max()) / 2])
            d = outro_ponto - c
            d[1] = 0
            d = unit(d)
            r = np.max((sel - c)[:, [0, 2]] @ d[[0, 2]])
            return c + d * (r + 0.03)
        an = centro if nome == 'centro' else min(aneis, key=lambda a: np.linalg.norm(a['c'][[0, 2]] - np.array(nome)))
        return an['c'] - CIMA * (an['R'] + 0.06)

    def pos_aprox(nome):
        if nome == 'torre':
            return np.array([VT[:, 0].mean(), 13.0, VT[:, 2].mean()])
        if nome == 'centro':
            return centro['c']
        an = min(aneis, key=lambda a: np.linalg.norm(a['c'][[0, 2]] - np.array(nome)))
        return an['c']

    meios = []
    total_lampadinhas = 0
    for i_varal, (a_nome, b_nome) in enumerate(VARAIS):
        br.parte = 'varal_%d' % i_varal
        a = ancora(a_nome, pos_aprox(b_nome))
        b = ancora(b_nome, pos_aprox(a_nome))
        vao = np.linalg.norm(b - a)
        linha = catenaria(a, b, FLECHA_DO_VARAL * vao, max(8, int(vao / 0.4)))
        tubo(br, linha, RAIO_DO_FIO, FIO, 0.0, lados=6)
        pts, t = reamostrar(linha, ENTRE_LAMPADINHAS)
        for p, tt in zip(pts[1:-1], t[1:-1]):
            if tt * vao < 0.4 or (1 - tt) * vao < 0.4:
                continue
            # acende do letreiro e da torre para os pilares
            ordem = 0.88 + 0.12 * tt
            lampada(br, p - CIMA * (RAIO_DA_LAMPADINHA + 0.02), RAIO_DA_LAMPADINHA, AMARELO_LAMPADA, ordem)
            total_lampadinhas += 1
        meios.append(linha[len(linha) // 2])
    print('Luigi: %d varais, %d lampadinhas' % (len(VARAIS), total_lampadinhas))

    # --- as luzes pontuais (eixos do jogo)
    m = np.array(meios).mean(0)
    luzes.insert(0, dict(p=np.array([m[0], m[1] - 0.5, m[2]]), cor=AMARELO_LAMPADA, raio=18.0, forca=1.0))
    luzes.insert(1, dict(p=O + CIMA * 1.6 + N * 2.5, cor=(1.0, 0.36, 0.40), raio=10.0, forca=0.6))
    for nome, pa, pb, fora in (TELHADOS[0], TELHADOS[3]):
        meio = (np.array([pa[0], 0, pa[1]]) + np.array([pb[0], 0, pb[1]])) / 2
        luzes.append(dict(p=meio + CIMA * 7.9 + np.array([fora[0], 0, fora[1]]) * 1.0, cor=VERMELHO_NEON, raio=9.0, forca=0.6))
    return br, luzes


# ------------------------------------------------------------------
# 6. Radiator Springs Curios
# ------------------------------------------------------------------
CURIOS_CAIXA = (518.0, 556.0, 140.0, 176.0)
TEXTURA_DO_CURIOS = 'ror_Image_33'
# As placas, pelas caixas delas na textura (pixels do PNG, linha 0 em cima; pintadas de cabeça para baixo)
PLACAS = {
    'grande': (256, 510, 256, 510),         # RADIATOR SPRINGS CURIOS
    'lembrancas': (518, 640, 259, 509),     # SOUVENIRS BRIC-A-BRAC TRINKETS
    'faroeste': (649, 768, 260, 509),       # WESTERN JUNK FOSSILS
    'calotas': (766, 951, 438, 508),        # RARE HUBCAPS
    'voce_quer': (768, 951, 349, 437),      # U WANT IT
    'nos_temos': (767, 953, 258, 347),      # WE GOT IT
    'liquidacao': (958, 1018, 268, 500),    # SALE TODAY ONLY
    'parede': (691, 896, 51, 256),          # a placa pintada na parede do lado
}
PLAQUINHAS = ('calotas', 'voce_quer', 'nos_temos', 'liquidacao')
ENTRE_LAMPADAS_DAS_LETRAS = 0.24
LINHAS_DA_PLACA_GRANDE = ('RADIATOR', 'SPRINGS', 'CURIOS')   # de cima para baixo
FONTE_DA_PLACA_GRANDE = 'saira_condensed_bold.ttf'
ENTRE_LAMPADAS_DO_CONTORNO = 0.30


def medir_curios():
    loja = Pedaco(['ror_Object_11', 'ror_Object_53'], CURIOS_CAIXA)
    W, H = bpy.data.images[TEXTURA_DO_CURIOS].size
    placas = {}
    uvpx = np.stack([loja.uv[..., 0] * W, (1 - loja.uv[..., 1]) * H], -1).mean(1)
    for nome, (x0, x1, y0, y1) in PLACAS.items():
        sel = np.where((loja.img == TEXTURA_DO_CURIOS) & (uvpx[:, 0] > x0 - 2) & (uvpx[:, 0] < x1 + 2)
                       & (uvpx[:, 1] > y0 - 2) & (uvpx[:, 1] < y1 + 2) & (np.abs(loja.normal[:, 1]) < 0.5))[0]
        if len(sel) == 0:
            raise SystemExit('Curios: não achei a placa %s' % nome)
        n = unit((loja.normal[sel] * loja.area[sel, None]).sum(0))
        sel = sel[(loja.normal[sel] @ n) > 0.7]
        N = unit((loja.normal[sel] * loja.area[sel, None]).sum(0) * [1, 0, 1])
        R = np.cross(CIMA, N)
        V = loja.tri[sel].reshape(-1, 3)
        centro = (V.min(0) + V.max(0)) / 2      # o meio da caixa (a média puxaria para onde há mais triângulos)
        placas[nome] = dict(tris=sel, N=N, R=R, centro=centro, y0=V[:, 1].min(), y1=V[:, 1].max(),
                            s0=((V - centro) @ R).min(), s1=((V - centro) @ R).max(), caixa=(x0, x1, y0, y1))
        print('Curios: placa %-11s %2d triângulos, %.1f x %.1f m, virada para (%.2f, %.2f)'
              % (nome, len(sel), placas[nome]['s1'] - placas[nome]['s0'], placas[nome]['y1'] - placas[nome]['y0'], N[0], N[2]))
    return loja, placas, W, H


def luminaria(br, apoio, n, alvo, ordem, alto=0.28, avanco=0.6):
    """Uma luminária pequena: o braço sai do apoio, sobe e avança; a cúpula aponta para o alvo, com a lâmpada
    na boca"""
    p0 = apoio
    p1 = apoio + CIMA * alto
    p2 = apoio + CIMA * (alto + 0.06) + n * (avanco * 0.55)
    p3 = apoio + CIMA * alto + n * avanco
    tubo(br, chaikin(np.array([p0, p1, p2, p3]), 2, fechado=False), 0.018, METAL_ESCURO, 0.0, lados=6)
    d = unit(alvo - p3)
    tubo(br, [p3 - d * 0.05, p3 + d * 0.10], 0.085, METAL_ESCURO, 0.0, lados=10)
    lampada(br, p3 + d * 0.13, 0.055, LAMPADA_DE_LUMINARIA, ordem)


def montar_curios():
    loja, placas, W, H = medir_curios()
    br = Brilho('curios')
    luzes = []
    px = pixels_da_imagem(TEXTURA_DO_CURIOS)

    # --- as placas acesas: cópias 4 cm à frente, com a textura do mapa
    ordem_da_placa = {'calotas': 0.02, 'voce_quer': 0.07, 'nos_temos': 0.12, 'liquidacao': 0.17,
                      'parede': 0.20, 'lembrancas': 0.24, 'faroeste': 0.24, 'grande': 0.45}
    for nome, pl in placas.items():
        br.parte = 'placa_' + nome
        ks = pl['tris']
        T = loja.tri[ks] + loja.normal[ks][:, None, :] * 0.04
        UV = loja.uv[ks].copy()
        baixo, cima = PLAQUINHA_ACESA if nome in PLAQUINHAS else PAREDE_ACESA if nome == 'parede' else PLACA_ACESA
        V = T.reshape(-1, 3)
        f = np.clip((V[:, 1] - pl['y0']) / max(pl['y1'] - pl['y0'], 1e-6), 0, 1)[:, None]
        cor = np.array(baixo) * (1 - f) + np.array(cima) * f
        br.juntar('placas', V, [(3 * k, 3 * k + 1, 3 * k + 2) for k in range(len(ks))], cor,
                  ordem_da_placa[nome], UV=UV.reshape(-1, 2))

    # --- os globos das bombas de gasolina na frente da varanda: os discos pintados ("PUMP") e a borda deles,
    # copiados 3 cm para fora
    globos = 0
    br.parte = 'bombas'
    for p, n in loja.pecas():
        ks = loja.da_peca(p)
        if not (16 <= n <= 40) or not np.all(loja.img[ks] == TEXTURA_DO_CURIOS):
            continue
        Vp = loja.tri[ks].reshape(-1, 3)
        c = Vp.mean(0)
        if np.ptp(Vp, 0).max() < 1.0 and 5.0 < c[1] < 7.5 and c[2] < placas['calotas']['centro'][2] - 3.0:
            T = loja.tri[ks] + loja.normal[ks][:, None, :] * 0.03
            br.juntar('placas', T.reshape(-1, 3), [(3 * k, 3 * k + 1, 3 * k + 2) for k in range(len(ks))],
                      GLOBO_ACESO, 0.0, UV=loja.uv[ks].reshape(-1, 2))
            globos += 1
    print('Curios: %d pedaços dos globos das bombas' % globos)
    luzes_das_bombas = globos

    # --- as letras da placa grande, de lâmpadas. A pintura é pequena e as letras dela se tocam: dela saem só
    # as três linhas (as caixas das letras vermelhas); as letras são desenhadas de novo com a fonte do jogo,
    # esticadas na caixa de cada linha, e cada uma ganha uma lâmpada a cada 30 cm no traço do meio
    g = placas['grande']
    x0, x1, y0, y1 = g['caixa']
    reg = px[y0:y1, x0:x1, :3]
    verm = (reg[..., 0] > 0.45) & (reg[..., 0] - reg[..., 1] > 0.25) & (reg[..., 0] - reg[..., 2] > 0.2)
    bandas = sorted(faixas(verm.sum(1) > 0.03 * verm.shape[1]), key=lambda f: f[1] - f[0])[-len(LINHAS_DA_PLACA_GRANDE):]
    bandas.sort(key=lambda f: -f[0])            # a pintura está de cabeça para baixo: a de baixo no PNG é a de cima
    br.parte = 'placa_grande'
    O, R, N = g['centro'], g['R'], g['N']
    Tg, UVg = loja.tri[g['tris']], loja.uv[g['tris']]
    total = sum(len(t.replace(' ', '')) for t in LINHAS_DA_PLACA_GRANDE)
    k_letra = 0
    n_lampadas = 0
    for texto, (a, b) in zip(LINHAS_DA_PLACA_GRANDE, bandas):
        cols = np.where(verm[a:b + 1].sum(0) >= 2)[0]
        cantos = uv_do_px([x0 + cols.min(), x0 + cols.max() + 1] * 2, [y0 + a] * 2 + [y0 + b + 1] * 2, W, H)
        P, _ = no_uv(Tg, UVg, cantos)
        s0, s1 = ((P - O) @ R).min(), ((P - O) @ R).max()
        ly0, ly1 = P[:, 1].min(), P[:, 1].max()
        T = triangulos_do_texto(texto, FONTE_DA_PLACA_GRANDE)
        fx0, fy0 = T.reshape(-1, 2).min(0)
        fx1, fy1 = T.reshape(-1, 2).max(0)
        passo = (fy1 - fy0) / 120
        m = 3
        M = rasterizar(T, fx0 - m * passo, fy0 - m * passo, passo, int((fx1 - fx0) / passo) + 2 * m, int((fy1 - fy0) / passo) + 2 * m)
        rot, nl = rotular(M)
        comps = [c for c in range(1, nl + 1) if (rot == c).sum() > 40]
        comps.sort(key=lambda c: np.nonzero(rot == c)[1].min())
        for c in comps:
            esq = afinar(rot == c)
            caminho = caminhar_no_esqueleto(esq)
            ii = np.array([p[0] for p in caminho], float)
            jj = np.array([p[1] for p in caminho], float)
            fx = fx0 + (jj - m + 0.5) * passo
            fy = fy0 + (ii - m + 0.5) * passo
            s = s0 + (fx - fx0) / (fx1 - fx0) * (s1 - s0)
            y = ly0 + (fy - fy0) / (fy1 - fy0) * (ly1 - ly0)
            esc = espalhar(np.stack([s, y, np.zeros_like(s)], 1), ENTRE_LAMPADAS_DAS_LETRAS)
            ordem = 0.45 + 0.55 * k_letra / max(total - 1, 1)
            for e in esc:
                p = O + s[e] * R + (y[e] - O[1]) * CIMA
                hit, _ = loja.raio(p + N * 1.0, -N, 2.0)
                base = hit if hit is not None else p
                lampada(br, base + N * (0.04 + FOLGA + RAIO_DA_LAMPADA), RAIO_DA_LAMPADA, LAMPADA_DE_LETREIRO, ordem)
                n_lampadas += 1
            k_letra += 1
        print('Curios: linha %-8s com %.2f x %.2f m, %d letras' % (texto, s1 - s0, ly1 - ly0, len(comps)))
    print('Curios: placa grande com %d letras e %d lâmpadas nelas' % (k_letra, n_lampadas))

    # --- o contorno de lâmpadas das placas do lado, correndo em volta
    for nome in ('lembrancas', 'faroeste'):
        br.parte = 'placa_' + nome
        pl = placas[nome]
        x0, x1, y0, y1 = pl['caixa']
        ks = pl['tris']
        # quantos metros por pixel, para recuar 16 cm da borda
        a, _ = no_uv(loja.tri[ks], loja.uv[ks], uv_do_px([x0, x1, x0], [y0, y0, y1], W, H))
        mx = np.linalg.norm(a[1] - a[0]) / (x1 - x0)
        my = np.linalg.norm(a[2] - a[0]) / (y1 - y0)
        dx, dy = 0.16 / mx, 0.16 / my
        cantos = np.array([[x0 + dx, y0 + dy], [x1 - dx, y0 + dy], [x1 - dx, y1 - dy], [x0 + dx, y1 - dy]])
        borda = np.concatenate([np.linspace(cantos[i], cantos[(i + 1) % 4], 80, endpoint=False) for i in range(4)])
        P, n = no_uv(loja.tri[ks], loja.uv[ks], uv_do_px(borda[:, 0], borda[:, 1], W, H))
        Q, t = reamostrar(np.concatenate([P, n], 1), ENTRE_LAMPADAS_DO_CONTORNO, fechado=True)
        for q, tt in zip(Q, t):
            lampada(br, q[:3] + unit(q[3:]) * (0.04 + FOLGA + RAIO_DO_CONTORNO), RAIO_DO_CONTORNO, LAMPADA_DE_LETREIRO, 0.24 + 0.18 * tt)
        print('Curios: contorno da placa %s com %d lâmpadas' % (nome, len(Q)))
        luzes.append(dict(p=pl['centro'] + pl['N'] * 2.2, cor=LAMPADA_DE_LETREIRO, raio=9.0, forca=0.8))

    # --- as luminárias das plaquinhas da varanda e da placa da parede
    for nome in PLAQUINHAS + ('parede',):
        br.parte = 'placa_' + nome
        pl = placas[nome]
        quantas = 4 if nome == 'parede' else 2
        larg = pl['s1'] - pl['s0']
        for f in np.linspace(0.5 / quantas, 1 - 0.5 / quantas, quantas):
            s = pl['s0'] + larg * f
            topo = pl['centro'] + pl['R'] * s + CIMA * (pl['y1'] - pl['centro'][1])
            hit, _ = loja.raio(topo + pl['N'] * 1.0 - CIMA * 0.05, -pl['N'], 2.0)
            apoio = (hit if hit is not None else topo) + CIMA * (0.05 if nome != 'parede' else 0.15)
            alvo = pl['centro'] + pl['R'] * s - CIMA * (0.15 * (pl['y1'] - pl['y0']))
            luminaria(br, apoio, pl['N'], alvo, ordem_da_placa[nome], alto=0.22 if nome != 'parede' else 0.12,
                      avanco=0.55 if nome != 'parede' else 0.7)

    # --- as luzes pontuais
    luzes.insert(0, dict(p=g['centro'] + g['N'] * 1.5 + CIMA * ((g['y0'] + 0.4 * (g['y1'] - g['y0'])) - g['centro'][1]),
                         cor=(1.0, 0.85, 0.50), raio=16.0, forca=1.2))
    pq = np.array([placas[n]['centro'] for n in PLAQUINHAS])
    nq = placas['voce_quer']['N']
    luzes.insert(1, dict(p=pq[:3].mean(0) + nq * 2.5, cor=LAMPADA_DE_LUMINARIA, raio=12.0, forca=1.0))
    luzes.append(dict(p=placas['liquidacao']['centro'] + nq * 2.0, cor=LAMPADA_DE_LUMINARIA, raio=8.0, forca=0.7))
    pw = placas['parede']
    luzes.append(dict(p=pw['centro'] + pw['N'] * 2.5 + CIMA * 1.5, cor=LAMPADA_DE_LUMINARIA, raio=10.0, forca=0.9))
    if luzes_das_bombas:
        bombas = [loja.tri[loja.da_peca(p)].reshape(-1, 3).mean(0) for p, n in loja.pecas()
                  if 16 <= n <= 40 and np.all(loja.img[loja.da_peca(p)] == TEXTURA_DO_CURIOS)]
        bombas = np.array([b for b in bombas if 5.0 < b[1] < 7.5 and b[2] < placas['calotas']['centro'][2] - 3.0])
        luzes.append(dict(p=bombas.mean(0) + np.array([0.0, -0.5, -1.2]), cor=LAMPADA_DE_LUMINARIA, raio=7.0, forca=0.6))
    # a varanda: a luz das plaquinhas e da vitrine caindo no chão de tábuas
    luzes.append(dict(p=np.array([pq[:3, 0].mean(), 4.8, pq[:3, 2].mean() + 1.0]), cor=(1.0, 0.78, 0.50), raio=10.0, forca=0.7))
    return br, luzes


# ------------------------------------------------------------------
# 7. Exportar e conferir
# ------------------------------------------------------------------
def novo_material(nome, imagem=None):
    mat = bpy.data.materials.new(nome)
    try:
        mat.use_nodes = True
    except Exception:
        pass
    nt = mat.node_tree
    nt.nodes.clear()
    saida = nt.nodes.new('ShaderNodeOutputMaterial')
    bsdf = nt.nodes.new('ShaderNodeBsdfPrincipled')
    bsdf.inputs['Base Color'].default_value = (1, 1, 1, 1)
    if imagem is not None:
        tex = nt.nodes.new('ShaderNodeTexImage')
        tex.image = imagem
        nt.links.new(tex.outputs['Color'], bsdf.inputs['Base Color'])
    nt.links.new(bsdf.outputs[0], saida.inputs['Surface'])
    return mat


def textura_das_placas():
    """A textura das placas do Curios (a do mapa), em PNG"""
    px = pixels_da_imagem(TEXTURA_DO_CURIOS)
    H, W = px.shape[:2]
    im = bpy.data.images.new('curios_placas', W, H, alpha=False)
    im.pixels.foreach_set(np.ascontiguousarray(px[::-1]).ravel())
    im.file_format = 'PNG'
    im.pack()
    return im


def criar_objetos(br, imagens):
    objs = []
    materiais = {}
    for (parte, material), pedacos in br.pedacos.items():
        if material not in materiais:
            materiais[material] = novo_material('%s_%s' % (br.nome, material), imagens.get(material))
        mat = materiais[material]
        grupos, atual, nv = [], [], 0
        for p in pedacos:
            if atual and nv + len(p[0]) > MAX_VERTICES:
                grupos.append(atual)
                atual, nv = [], 0
            atual.append(p)
            nv += len(p[0])
        if atual:
            grupos.append(atual)
        for gi, grupo in enumerate(grupos):
            V, F, C, UV, base = [], [], [], [], 0
            for v, f, c, uv in grupo:
                V.append(v)
                F += [tuple(i + base for i in face) for face in f]
                C.append(c)
                UV.append(uv if uv is not None else np.zeros((len(v), 2)))
                base += len(v)
            V = para_blender(np.concatenate(V))
            C = np.concatenate(C)
            UV = np.concatenate(UV)
            me = bpy.data.meshes.new('%s_%s_%s%s' % (br.nome, parte, material, '_%d' % gi if gi else ''))
            me.from_pydata([tuple(p) for p in V], [], F)
            me.update()
            at = me.color_attributes.new('Col', 'FLOAT_COLOR', 'POINT')
            at.data.foreach_set('color', C.astype(np.float32).ravel())
            me.color_attributes.active_color = at
            if material in imagens:
                uvl = me.uv_layers.new(name='UVMap')
                vi = np.empty(len(me.loops), np.int64)
                me.loops.foreach_get('vertex_index', vi)
                uvl.data.foreach_set('uv', UV[vi].astype(np.float32).ravel())
            me.materials.append(mat)
            o = bpy.data.objects.new(me.name, me)
            bpy.context.scene.collection.objects.link(o)
            objs.append(o)
    return objs


def ler_glb(caminho):
    d = open(caminho, 'rb').read()
    n = struct.unpack('<I', d[12:16])[0]
    return json.loads(d[20:20 + n]), d


def exportar(br, imagens, luzes, nome_do_lugar, acender_em):
    objs = criar_objetos(br, imagens)
    bpy.ops.object.select_all(action='DESELECT')
    for o in objs:
        o.select_set(True)
    caminho = os.path.join(SAIDA, br.nome + '.glb')
    bpy.ops.export_scene.gltf(filepath=caminho, export_format='GLB', use_selection=True, export_yup=True,
                              export_apply=True, export_animations=False, export_normals=False,
                              export_vertex_color='ACTIVE', export_all_vertex_colors=False,
                              export_active_vertex_color_when_no_material=True, export_image_format='AUTO',
                              export_materials='EXPORT')
    j, d = ler_glb(caminho)
    tris = sem_cor = maior = 0
    tipos = set()
    for me in j['meshes']:
        for p in me['primitives']:
            if 'COLOR_0' not in p['attributes']:
                sem_cor += 1
            else:
                acc = j['accessors'][p['attributes']['COLOR_0']]
                tipos.add((acc['type'], acc['componentType']))
            tris += j['accessors'][p['indices']]['count'] // 3
            maior = max(maior, j['accessors'][p['attributes']['POSITION']]['count'])
    prims = sum(len(m['primitives']) for m in j['meshes'])
    imgs = [(i.get('name'), i.get('mimeType')) for i in j.get('images', [])]
    fatores = [m.get('pbrMetallicRoughness', {}).get('baseColorFactor', [1, 1, 1, 1]) for m in j['materials']]
    print('Exportado %s' % caminho)
    print('  primitivas: %d (sem COLOR_0: %d; COLOR_0 %s); maior: %d vértices' % (prims, sem_cor, sorted(tipos), maior))
    print('  triângulos: %d; texturas: %s; baseColorFactor: %s' % (tris, imgs, fatores))
    print('  tamanho: %.2f MB' % (os.path.getsize(caminho) / 1e6))
    if sem_cor or any(t[0] != 'VEC4' for t in tipos):
        raise SystemExit('ERRO: há primitivas sem COLOR_0 VEC4')
    if maior > 65535:
        raise SystemExit('ERRO: primitiva com mais de 65.535 vértices')
    if any(m != 'image/png' for _, m in imgs):
        raise SystemExit('ERRO: textura que não é PNG')
    if any(any(abs(c - 1) > 1e-6 for c in f) for f in fatores):
        raise SystemExit('ERRO: baseColorFactor diferente de branco')
    dados = dict(nome=nome_do_lugar, acender_em=acender_em, luzes=[
        dict(x=round(float(l['p'][0]), 2), y=round(float(l['p'][1]), 2), z=round(float(l['p'][2]), 2),
             r=round(float(l['cor'][0]), 3), g=round(float(l['cor'][1]), 3), b=round(float(l['cor'][2]), 3),
             raio=round(float(l['raio']), 1), forca=round(float(l['forca']), 2)) for l in luzes])
    with open(os.path.join(SAIDA, br.nome + '.json'), 'w') as f:
        json.dump(dados, f, indent=1, ensure_ascii=False)
        f.write('\n')
    print('  %d luzes pontuais em %s' % (len(luzes), br.nome + '.json'))
    for o in objs:
        bpy.data.objects.remove(o, do_unlink=True)
    return dict(primitivas=prims, triangulos=tris, maior=maior)


print('Montando as luzes do Luigi...')
BR_LUIGI, LUZES_LUIGI = montar_luigi()
print('  %d vértices, %d triângulos antes de exportar' % BR_LUIGI.contar())
RES_LUIGI = exportar(BR_LUIGI, {}, LUZES_LUIGI, "Luigi's Casa Della Tires", 3.0)

print('Montando as luzes do Curios...')
BR_CURIOS, LUZES_CURIOS = montar_curios()
print('  %d vértices, %d triângulos antes de exportar' % BR_CURIOS.contar())
RES_CURIOS = exportar(BR_CURIOS, {'placas': textura_das_placas()}, LUZES_CURIOS, 'Radiator Springs Curios', 4.0)


# ------------------------------------------------------------------
# 8. Prévias (--previas)
# ------------------------------------------------------------------
VISTAS = [
    # nome, lugar, câmera (jogo), alvo (jogo), lente (mm), dia?
    ('luigi_noite_rua', 'luigi', (480.0, 2.5, 132.0), (476.5, 9.0, 160.0), 24, False),
    ('luigi_noite_patio', 'luigi', (490.0, 2.2, 153.0), (474.5, 9.6, 162.5), 24, False),
    ('luigi_noite_travessa', 'luigi', (502.0, 2.5, 174.0), (476.0, 9.0, 160.0), 24, False),
    ('luigi_dia_rua', 'luigi', (480.0, 2.5, 132.0), (476.5, 9.0, 160.0), 24, True),
    ('curios_noite_rua', 'curios', (538.0, 2.5, 126.0), (538.0, 11.5, 158.0), 24, False),
    ('curios_noite_leste', 'curios', (563.0, 2.5, 141.0), (540.0, 9.5, 158.0), 24, False),
    ('curios_noite_perto', 'curios', (533.0, 2.3, 138.0), (538.0, 11.5, 158.0), 22, False),
    ('curios_dia_rua', 'curios', (538.0, 2.5, 126.0), (538.0, 11.5, 158.0), 24, True),
]


def previas(pasta):
    os.makedirs(pasta, exist_ok=True)
    sc = bpy.context.scene
    antes = set(bpy.data.objects)
    lugares = {}
    for nome in ('luigi', 'curios'):
        ja = set(bpy.data.objects)
        bpy.ops.import_scene.gltf(filepath=os.path.join(SAIDA, nome + '.glb'))
        lugares[nome] = [o for o in bpy.data.objects if o not in ja and o.type == 'MESH']
    luzes_json = {n: json.load(open(os.path.join(SAIDA, n + '.json'))) for n in ('luigi', 'curios')}

    def material_da_luz(o, noite):
        me = o.data
        attr = me.color_attributes[0].name if me.color_attributes else None
        img = None
        for m in me.materials:
            img = img or imagem_do_material(m)
        mat = bpy.data.materials.new('previa')
        try:
            mat.use_nodes = True
        except Exception:
            pass
        nt = mat.node_tree
        nt.nodes.clear()
        out = nt.nodes.new('ShaderNodeOutputMaterial')
        ca = nt.nodes.new('ShaderNodeVertexColor')
        ca.layer_name = attr or ''
        gam = nt.nodes.new('ShaderNodeGamma')      # o COLOR_0 está em sRGB
        gam.inputs[1].default_value = 2.2
        nt.links.new(ca.outputs['Color'], gam.inputs[0])
        cor = gam.outputs[0]
        if img is not None:
            tex = nt.nodes.new('ShaderNodeTexImage')
            tex.image = img
            mix = nt.nodes.new('ShaderNodeMix')
            mix.data_type = 'RGBA'
            mix.blend_type = 'MULTIPLY'
            mix.inputs['Factor'].default_value = 1.0
            nt.links.new(tex.outputs['Color'], mix.inputs['A'])
            nt.links.new(cor, mix.inputs['B'])
            cor = mix.outputs['Result']
        if noite:
            em = nt.nodes.new('ShaderNodeEmission')
            em.inputs['Strength'].default_value = 1.8 if img is not None else 4.5   # o jogo: a cor × 1,6 (+ 0,12)
            nt.links.new(cor, em.inputs['Color'])
            nt.links.new(em.outputs[0], out.inputs['Surface'])
        else:
            mul = nt.nodes.new('ShaderNodeMix')
            mul.data_type = 'RGBA'
            mul.blend_type = 'MULTIPLY'
            mul.inputs['Factor'].default_value = 1.0
            mul.inputs['B'].default_value = (0.3, 0.3, 0.3, 1)
            nt.links.new(cor, mul.inputs['A'])
            bs = nt.nodes.new('ShaderNodeBsdfPrincipled')
            bs.inputs['Roughness'].default_value = 0.25
            nt.links.new(mul.outputs['Result'], bs.inputs['Base Color'])
            nt.links.new(bs.outputs[0], out.inputs['Surface'])
        me.materials.clear()
        me.materials.append(mat)

    mundo = bpy.data.worlds.new('mundo')
    try:
        mundo.use_nodes = True
    except Exception:
        pass
    sc.world = mundo
    fundo = mundo.node_tree.nodes['Background']
    sol_d = bpy.data.lights.new('sol', 'SUN')
    sol = bpy.data.objects.new('sol', sol_d)
    sc.collection.objects.link(sol)
    sol.rotation_euler = (math.radians(50), math.radians(10), math.radians(140))
    cam_d = bpy.data.cameras.new('cam')
    cam_d.clip_start, cam_d.clip_end = 0.1, 3000
    cam = bpy.data.objects.new('cam', cam_d)
    sc.collection.objects.link(cam)
    sc.camera = cam
    sc.render.engine = 'BLENDER_EEVEE'
    sc.render.resolution_x, sc.render.resolution_y = 1400, 600
    try:
        sc.eevee.taa_render_samples = 32
    except Exception:
        pass
    pontos = []
    for nome, lugar, pc, pa, lente, dia in VISTAS:
        for o in pontos:
            bpy.data.objects.remove(o, do_unlink=True)
        pontos = []
        for n, objs in lugares.items():
            for o in objs:
                material_da_luz(o, not dia)
        if dia:
            fundo.inputs[0].default_value = (0.42, 0.58, 0.85, 1)
            fundo.inputs[1].default_value = 1.0
            sol_d.energy = 4.0
            sol_d.color = (1.0, 0.96, 0.9)
        else:
            fundo.inputs[0].default_value = (0.020, 0.024, 0.065, 1)
            fundo.inputs[1].default_value = 1.0
            sol_d.energy = 0.12
            sol_d.color = (0.55, 0.62, 1.0)
            for n, dados in luzes_json.items():
                for k, l in enumerate(dados['luzes']):
                    ld = bpy.data.lights.new('l', 'POINT')
                    ld.color = (l['r'], l['g'], l['b'])
                    ld.energy = l['forca'] * l['raio'] ** 2 * 7.0
                    ld.shadow_soft_size = 0.2
                    try:
                        ld.use_custom_distance = True
                        ld.cutoff_distance = l['raio']
                    except Exception:
                        pass
                    lo = bpy.data.objects.new('l', ld)
                    lo.location = (l['x'], -l['z'], l['y'])
                    sc.collection.objects.link(lo)
                    pontos.append(lo)
        p, a = para_blender(np.array(pc)), para_blender(np.array(pa))
        from mathutils import Vector
        cam.location = Vector(p)
        cam.rotation_euler = (Vector(a) - Vector(p)).to_track_quat('-Z', 'Y').to_euler()
        cam_d.lens = lente
        caminho = os.path.join(pasta, nome + '.png')
        if dia:
            sc.view_settings.view_transform = 'Standard'
            sc.render.image_settings.file_format = 'PNG'
            sc.render.filepath = caminho
            bpy.ops.render.render(write_still=True)
        else:
            sc.view_settings.view_transform = 'Standard'
            sc.render.image_settings.file_format = 'OPEN_EXR'
            exr = os.path.join(pasta, nome + '.exr')
            sc.render.filepath = exr
            bpy.ops.render.render(write_still=True)
            halo(exr, caminho)
            os.remove(exr)
        print('Prévia %s' % caminho)


def _borrar(img, r):
    """Três passadas de média em caixa (quase uma gaussiana), nas duas direções"""
    for _ in range(3):
        for eixo in (0, 1):
            c = np.cumsum(np.pad(img, [(r + 1, r) if e == eixo else (0, 0) for e in range(3)], mode='edge'), axis=eixo)
            if eixo == 0:
                img = (c[2 * r + 1:] - c[:-2 * r - 1]) / (2 * r + 1)
            else:
                img = (c[:, 2 * r + 1:] - c[:, :-2 * r - 1]) / (2 * r + 1)
    return img


def halo(exr, png):
    im = bpy.data.images.load(exr)
    W, H = im.size
    px = np.empty(W * H * 4, np.float32)
    im.pixels.foreach_get(px)
    px = px.reshape(H, W, 4)[..., :3].astype(np.float64)
    bpy.data.images.remove(im)
    claro = np.maximum(px - 0.9, 0.0)
    h = 0.55 * _borrar(claro, 3) + 0.35 * _borrar(claro, 10) + 0.25 * _borrar(claro, 28)
    img = px + h
    img = np.clip(img, 0, 1)
    img = np.where(img <= 0.0031308, img * 12.92, 1.055 * np.power(img, 1 / 2.4) - 0.055)
    out = bpy.data.images.new('halo', W, H, alpha=False)
    rgba = np.concatenate([img, np.ones((H, W, 1))], 2).astype(np.float32)
    out.pixels.foreach_set(rgba.ravel())
    out.filepath_raw = png
    out.file_format = 'PNG'
    out.save()                     # uma imagem de 8 bits grava os valores como estão (já em sRGB)
    bpy.data.images.remove(out)


if PREVIAS:
    previas(PREVIAS)

print('Pronto: luigi.glb (%(triangulos)d triângulos, %(primitivas)d primitivas, maior %(maior)d vértices)' % RES_LUIGI)
print('        curios.glb (%(triangulos)d triângulos, %(primitivas)d primitivas, maior %(maior)d vértices)' % RES_CURIOS)
