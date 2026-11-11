"""
montar_luzes_sarge_fillmore_cozy.py
------------------------------------------------------------------
Monta os neons e as luzes de noite de três lugares de Radiator Springs (do
filme Carros, 2006): o Sarge's Surplus Hut, o domo do Fillmore e o Cozy Cone
Motel (a placa grande e as luminárias da praça). Tudo desenhado por cima do
mapa do mundo aberto (recursos/modelos/radiator_springs.glb), já nas
coordenadas do mundo do jogo (x, y para cima, z):

    Blender -b --factory-startup -P ferramentas/luzes/montar_luzes_sarge_fillmore_cozy.py -- \\
        recursos/modelos/radiator_springs.glb recursos/modelos/luzes [--previas pasta] [--vistas a,b]

Grava, na pasta dada no segundo argumento, para cada lugar (sarge, fillmore,
cozy_cone):
  <lugar>.glb   só o que brilha à noite. A cor acesa de cada vértice vai em
                COLOR_0 (RGB na tela, 0 a 1; na face com textura, multiplica a
                textura) e o A diz a ordem de acender (0 primeiro, 1 por
                último). Material branco; só o domo do Fillmore (e o símbolo da
                paz) tem textura, em PNG. Nenhuma primitiva passa de 65.535
                vértices (a raylib usa índices de 16 bits);
  <lugar>.json  o nome, quando o lugar acende (acender_em, em segundos) e as
                luzes pontuais que ele joga em volta (x, y, z do jogo, cor,
                raio em metros e força).
Com --previas, grava na pasta as conferências: de noite (o mapa escuro
azulado, as luzes emissivas com halo e as luzes do JSON acesas) e de dia (os
tubos apagados, como vidro: a cor × 0,3), do ponto de vista de quem dirige;
--vistas escolhe só algumas delas (os nomes estão em VISTAS, lá embaixo).

O mapa (Race-O-Rama, PS3) só tem os letreiros pintados em texturas pequenas.
A ferramenta acha as placas pelas UVs (a região da textura onde cada desenho
está pintado), mede o plano de cada face (frente e verso) e põe os tubos de
neon 5 a 6 cm à frente dela. As letras saem do contorno do texto do Blender
(texto → curva → polilinha simplificada → tubo redondo de 6 ou 8 lados); o
contorno de dentro (o neon duplo do filme) é o contorno de fora recuado. As
fontes são as de recursos/fontes: Saira Condensed (SARGE'S, SURPLUS HUT,
MOTEL), Racing Sans One (COZY CONE) e Lobster (VACANCY).

  1. Sarge's Surplus Hut (ror_Object_51): a placa que sai da frente do galpão,
     "SARGE'S" (contorno duplo) e "SURPLUS HUT" em verde; a divisa de
     sargento ao lado (três divisas, o círculo com a estrela e três arcos) em
     amarelo-esverdeado e dourado; as lâmpadas do arco em cima e o contorno da
     ponta da seta; a arandela em cima da placa e a luminária da porta.
  2. Fillmore (ror_Object_56): as faces do domo copiadas 2,5 cm para fora, com
     a pintura do mapa passada para "tinta fluorescente sob luz negra" (cores
     saturadas: o sol laranja, as nuvens violeta, os arcos-íris, as estradas
     azuis) e as arestas da estrutura geodésica escuras; a cortina de contas
     coloridas na entrada; o símbolo da paz da beira da estrada e a lâmpada
     redonda do poste (violeta, a luz negra).
  3. Cozy Cone (ror_Object_8 e a praça): "COZY CONE" amarelo (duplo) dentro da
     moldura laranja; o cone laranja (duplo) com o carrinho dormindo amarelo e
     os "Z" ciano; "MOTEL" branco duplo com a moldura branca; "VACANCY"
     laranja na plaquinha de baixo; os cones de luz dos cantos das bases dos
     oito quartos acesos; a lâmpada em cima de cada porta; as lâmpadas nas
     cúpulas dos quatro postes dos canteiros; e cones-luminária pequenos
     (40 cm) em volta dos três canteiros, acendendo em onda.

A ordem de acender: as lâmpadas primeiro, depois as letras uma a uma (como no
filme), as molduras se desenhando ao longo do tubo, e por último o VACANCY e a
ponta da seta do Sarge. As luminárias da praça acendem em onda a partir da
placa.

Eixos: no Blender, x = x do jogo, y = -z do jogo e z = y do jogo; o .glb sai
com +Y para cima, ou seja, nas coordenadas do jogo.
------------------------------------------------------------------
"""

import bpy
import json
import math
import os
import shutil
import struct
import sys
import tempfile
import time
import numpy as np
from mathutils import Vector

INICIO = time.time()
bpy.ops.wm.read_factory_settings(use_empty=True)     # sem o cubo, a câmera e a luz da cena inicial
AQUI = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(AQUI))
PASTA_DAS_FONTES = os.path.join(REPO, 'recursos', 'fontes')

# ------------------------------------------------------------------
# Argumentos
# ------------------------------------------------------------------
argv = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
OPCOES_COM_VALOR = ('--previas', '--vistas')
opcoes, posicionais = {}, []
i = 0
while i < len(argv):
    if argv[i] in OPCOES_COM_VALOR and i + 1 < len(argv):
        opcoes[argv[i]] = argv[i + 1]
        i += 2
    else:
        posicionais.append(argv[i])
        i += 1
ARQUIVO_DO_MAPA = os.path.abspath(posicionais[0] if posicionais else
                                  os.path.join(REPO, 'recursos', 'modelos', 'radiator_springs.glb'))
PASTA_DE_SAIDA = os.path.abspath(posicionais[1] if len(posicionais) > 1 else
                                 os.path.join(REPO, 'recursos', 'modelos', 'luzes'))
PREVIAS = os.path.abspath(opcoes['--previas']) if '--previas' in opcoes else None
SO_AS_VISTAS = opcoes['--vistas'].split(',') if '--vistas' in opcoes else None   # só estas prévias
os.makedirs(PASTA_DE_SAIDA, exist_ok=True)
PASTA_TEMP = tempfile.mkdtemp(prefix='luzes_sfc_')


def parte(nome):
    print('[%6.1f s] %s' % (time.time() - INICIO, nome))


# ------------------------------------------------------------------
# Cores (na tela, sRGB 0 a 1) e a espessura dos tubos
# ------------------------------------------------------------------
VERDE_SARGE = (0.32, 1.00, 0.34)          # o neon verde de SARGE'S SURPLUS HUT
DIVISA = (0.80, 1.00, 0.16)               # as divisas e os arcos: amarelo-esverdeado
DOURADO = (1.00, 0.84, 0.20)              # o círculo e a estrela da divisa
LAMPADA = (1.00, 0.90, 0.66)              # lâmpadas incandescentes (arco, arandela, portas)
AMBAR = (1.00, 0.66, 0.16)                # o contorno da ponta da seta
AMARELO_COZY = (1.00, 0.86, 0.16)         # COZY CONE e o carrinho
LARANJA_COZY = (1.00, 0.40, 0.07)         # a moldura e o cone
CIANO_Z = (0.22, 1.00, 0.86)              # os "Z" do carrinho dormindo
VERDE_Z = (0.45, 1.00, 0.45)
BRANCO_MOTEL = (1.00, 0.97, 0.90)         # MOTEL e a moldura dele
LARANJA_VACANCY = (1.00, 0.62, 0.14)
VIOLETA = (0.74, 0.56, 1.00)              # a lâmpada de luz negra do Fillmore

AFASTAR = 0.06                            # o eixo dos tubos fica 6 cm à frente da placa

# ------------------------------------------------------------------
# 1. O mapa: os triângulos (eixos do jogo) com as UVs em pixels da textura
# ------------------------------------------------------------------
parte('Importando o mapa %s' % ARQUIVO_DO_MAPA)
bpy.ops.import_scene.gltf(filepath=ARQUIVO_DO_MAPA)
OBJETOS_DO_MAPA = [o for o in bpy.data.objects if o.type == 'MESH']


def imagem_do_material(mat):
    if mat is None or mat.node_tree is None:
        return None
    for nd in mat.node_tree.nodes:
        if nd.type == 'TEX_IMAGE' and nd.image is not None:
            return nd.image
    return None


def normalizar(v):
    v = np.asarray(v, float)
    n = np.linalg.norm(v, axis=-1, keepdims=True)
    return v / np.maximum(n, 1e-12)


class Tris:
    """Os triângulos de um objeto do mapa: p (n,3,3) em eixos do jogo, t (n,3,2) em pixels da textura
    (linha 0 em cima, como no PNG), uv (n,3,2) como no Blender, n (n,3) normais e área. Os triângulos
    repetidos (o mapa tem uns em dobro, na mesma posição e com a mesma face) saem."""

    def __init__(self, nome):
        ob = bpy.data.objects[nome]
        me = ob.data
        me.calc_loop_triangles()
        mw = np.array(ob.matrix_world)
        nv = len(me.vertices)
        co = np.empty(nv * 3)
        me.vertices.foreach_get('co', co)
        co = co.reshape(-1, 3) @ mw[:3, :3].T + mw[:3, 3]
        g = np.stack([co[:, 0], co[:, 2], -co[:, 1]], 1)
        nt = len(me.loop_triangles)
        tv = np.empty(nt * 3, np.int32)
        me.loop_triangles.foreach_get('vertices', tv)
        tl = np.empty(nt * 3, np.int32)
        me.loop_triangles.foreach_get('loops', tl)
        uv = np.empty(len(me.loops) * 2)
        me.uv_layers[0].data.foreach_get('uv', uv)
        uv = uv.reshape(-1, 2)
        p = g[tv.reshape(-1, 3)]
        uvt = uv[tl.reshape(-1, 3)]
        nn = np.cross(p[:, 1] - p[:, 0], p[:, 2] - p[:, 0])
        chaves, manter = set(), []
        for k in range(nt):
            ch = tuple(sorted(tuple(np.round(q, 3)) for q in p[k])) + tuple(np.sign(np.round(nn[k], 3)))
            if ch not in chaves:
                chaves.add(ch)
                manter.append(k)
        manter = np.array(manter)
        self.nome = nome
        self.imagem = imagem_do_material(me.materials[0] if me.materials else None)
        self.W, self.H = self.imagem.size if self.imagem else (1, 1)
        self.p = p[manter]
        self.uv = uvt[manter]
        self.t = np.stack([self.uv[..., 0] * self.W, (1 - self.uv[..., 1]) * self.H], -1)
        nn = nn[manter]
        self.area = np.linalg.norm(nn, axis=1) / 2
        self.n = normalizar(nn)

    def escolher(self, tex=None, normal=None, cos_min=0.85, caixa=None):
        """Os triângulos com o centro (na textura) no retângulo tex=(x0, x1, y0, y1), a normal perto de
        'normal' e o centro (no mundo) na caixa=(x0, x1, y0, y1, z0, z1)"""
        s = self.area > 1e-6
        if tex is not None:
            c = self.t.mean(1)
            s &= (c[:, 0] > tex[0]) & (c[:, 0] < tex[1]) & (c[:, 1] > tex[2]) & (c[:, 1] < tex[3])
        if normal is not None:
            s &= self.n @ normalizar(normal) > cos_min
        if caixa is not None:
            c = self.p.mean(1)
            s &= ((c[:, 0] > caixa[0]) & (c[:, 0] < caixa[1]) & (c[:, 1] > caixa[2]) & (c[:, 1] < caixa[3]) &
                  (c[:, 2] > caixa[4]) & (c[:, 2] < caixa[5]))
        return np.nonzero(s)[0]


def contornos_dos_triangulos(P):
    """As bordas (arestas usadas por um só triângulo) de um punhado de triângulos P (n,3,3), encadeadas
    em laços fechados; o mais comprido primeiro"""
    chave = {}
    pos = []

    def idx(q):
        k = tuple(np.round(q, 3))
        if k not in chave:
            chave[k] = len(pos)
            pos.append(q)
        return chave[k]
    conta = {}
    for tri in P:
        a, b, c = idx(tri[0]), idx(tri[1]), idx(tri[2])
        for e in ((a, b), (b, c), (c, a)):
            k = (min(e), max(e))
            conta[k] = conta.get(k, 0) + 1
    bordas = [k for k, n in conta.items() if n == 1]
    viz = {}
    for a, b in bordas:
        viz.setdefault(a, []).append(b)
        viz.setdefault(b, []).append(a)
    pos = np.array(pos)
    lacos, usado = [], set()
    for ini in viz:
        if ini in usado:
            continue
        laco = [ini]
        usado.add(ini)
        ant, atual = None, ini
        while True:
            prox = [v for v in viz[atual] if v != ant and v not in usado]
            if not prox:
                break
            ant, atual = atual, prox[0]
            usado.add(atual)
            laco.append(atual)
        if len(laco) >= 3:
            lacos.append(pos[laco])
    lacos.sort(key=lambda L: -np.linalg.norm(np.diff(np.vstack([L, L[:1]]), axis=0), axis=1).sum())
    return lacos


class Placa:
    """Uma face plana de placa do mapa: a origem no plano mais à frente, u para a direita de quem lê de
    frente, v para cima, n para fora; e o mapa da textura (pixels) para o mundo, pelo maior triângulo"""

    def __init__(self, T, idx):
        P = T.p[idx]
        A = T.area[idx]
        n = normalizar((T.n[idx] * A[:, None]).sum(0))
        cima = np.array([0.0, 1.0, 0.0])
        eu = normalizar(np.cross(cima, n))
        ev = np.cross(n, eu)
        V = P.reshape(-1, 3)
        o = V.mean(0)
        o = o + n * ((V - o) @ n).max()
        self.o, self.n, self.eu, self.ev = o, n, eu, ev
        k = idx[int(np.argmax(A))]
        p0, p1, p2 = T.p[k]
        t0, t1, t2 = T.t[k]
        self.afim = np.stack([p1 - p0, p2 - p0], 1) @ np.linalg.inv(np.stack([t1 - t0, t2 - t0], 1))
        self.p0, self.t0 = p0, t0
        lacos = contornos_dos_triangulos(P)
        self.contorno = self.local(lacos[0]) if lacos else None

    def local(self, X):
        d = np.asarray(X, float) - self.o
        return np.stack([d @ self.eu, d @ self.ev], -1)

    def tex(self, tx, ty):
        """Um ponto da textura (pixels) nas coordenadas da placa"""
        X = self.p0 + self.afim @ (np.array([tx, ty], float) - self.t0)
        return self.local(X)

    def caixa_tex(self, x0, x1, y0, y1):
        q = np.array([self.tex(x, y) for x in (x0, x1) for y in (y0, y1)])
        return q[:, 0].min(), q[:, 0].max(), q[:, 1].min(), q[:, 1].max()

    def mundo(self, Q, afastar=AFASTAR):
        Q = np.asarray(Q, float)
        return self.o + Q[..., 0:1] * self.eu + Q[..., 1:2] * self.ev + afastar * self.n


# ------------------------------------------------------------------
# 2. Polígonos no plano (contornos de letras e de placas)
# ------------------------------------------------------------------
def area_assinada(P):
    x, y = P[:, 0], P[:, 1]
    return 0.5 * float((x * np.roll(y, -1) - np.roll(x, -1) * y).sum())


def ponto_no_poligono(q, P):
    x, y = q
    xa, ya = P[:, 0], P[:, 1]
    xb, yb = np.roll(xa, -1), np.roll(ya, -1)
    cruza = (ya > y) != (yb > y)
    with np.errstate(divide='ignore', invalid='ignore'):
        xc = xa + (y - ya) * (xb - xa) / (yb - ya)
    return bool(np.count_nonzero(cruza & (x < xc)) % 2)


def dentro_par_impar(q, lacos):
    return sum(ponto_no_poligono(q, L) for L in lacos) % 2 == 1


def distancia_aos_lados(Q, lacos):
    """A menor distância de cada ponto de Q aos lados de todos os laços"""
    A = np.vstack([L for L in lacos])
    B = np.vstack([np.roll(L, -1, 0) for L in lacos])
    d = B - A
    dd = np.maximum((d * d).sum(1), 1e-12)
    t = np.clip(((Q[:, None, :] - A[None]) * d[None]).sum(-1) / dd[None], 0, 1)
    proj = A[None] + t[..., None] * d[None]
    return np.sqrt(((Q[:, None, :] - proj) ** 2).sum(-1)).min(1)


def simplificar(P, tol, fechado=True):
    """Douglas-Peucker; num laço fechado, corta nos dois pontos mais distantes"""
    P = np.asarray(P, float)
    if len(P) < 4:
        return P

    def dp(Q):
        if len(Q) < 3:
            return [0, len(Q) - 1]
        a, b = Q[0], Q[-1]
        ab = b - a
        L = np.linalg.norm(ab)
        if L < 1e-12:
            d = np.linalg.norm(Q - a, axis=1)
        else:
            d = np.abs(ab[0] * (Q[:, 1] - a[1]) - ab[1] * (Q[:, 0] - a[0])) / L
        k = int(np.argmax(d[1:-1])) + 1
        if d[k] <= tol:
            return [0, len(Q) - 1]
        e = dp(Q[:k + 1])
        f = dp(Q[k:])
        return e[:-1] + [k + j for j in f]
    if not fechado:
        return P[dp(P)]
    i0 = 0
    i1 = int(np.argmax(np.linalg.norm(P - P[0], axis=1)))
    if i1 == 0:
        return P
    A = P[i0:i1 + 1]
    B = np.vstack([P[i1:], P[:1]])
    ka = dp(A)
    kb = dp(B)
    return np.vstack([A[ka[:-1]], B[kb[:-1]]])


def recuar(P, d, para_a_esquerda=None):
    """O laço P deslocado d para dentro (para a esquerda de quem anda, se para_a_esquerda; senão, para
    dentro do polígono pelo sentido dele). Em esquadria, limitada a 2,5 d nos cantos agudos."""
    if para_a_esquerda is None:
        para_a_esquerda = area_assinada(P) > 0
    seg = np.roll(P, -1, 0) - P
    t = normalizar(seg)
    nesq = np.stack([-t[:, 1], t[:, 0]], 1)
    nn = nesq if para_a_esquerda else -nesq
    nprev = np.roll(nn, 1, 0)
    m = normalizar(nn + nprev)
    cosm = np.clip((m * nn).sum(1), 0.4, 1.0)
    return P + m * (d / cosm)[:, None]


def limpar_recuo(Q, originais, d, perda_max=0.35):
    """Tira do laço recuado os pontos que ficaram perto demais da borda original (as pontas que viram
    espinhos nos cantos agudos e os laços que se cruzam nos traços finos). Se perde muito, some."""
    ok = distancia_aos_lados(Q, originais) > d * 0.72
    if ok.mean() < 1 - perda_max or ok.sum() < 4:
        return None
    R = Q[ok]
    if abs(area_assinada(R)) < (d * 2) ** 2:
        return None
    return R


def arredondar(P, raio, fechado=True, passos=3):
    """Troca cada canto por um arquinho (como o vidro dobrado do neon)"""
    P = np.asarray(P, float)
    n = len(P)
    out = []
    for i in range(n):
        if not fechado and (i == 0 or i == n - 1):
            out.append(P[i])
            continue
        a, b, c = P[i - 1], P[i], P[(i + 1) % n]
        la, lc = np.linalg.norm(b - a), np.linalg.norm(c - b)
        r = min(raio, la * 0.45, lc * 0.45)
        ta, tc = normalizar(b - a), normalizar(c - b)
        if r < 1e-4 or np.dot(ta, tc) > 0.985:
            out.append(b)
            continue
        p0, p2 = b - ta * r, b + tc * r
        for s in np.linspace(0, 1, passos + 2):
            out.append((1 - s) ** 2 * p0 + 2 * s * (1 - s) * b + s * s * p2)
    return np.array(out)


def cortes_horizontais(P, v):
    """Os u onde a reta horizontal v corta o polígono P"""
    xa, ya = P[:, 0], P[:, 1]
    xb, yb = np.roll(xa, -1), np.roll(ya, -1)
    s = (ya - v) * (yb - v) < 0
    with np.errstate(divide='ignore', invalid='ignore'):
        u = xa + (v - ya) * (xb - xa) / (yb - ya)
    return np.sort(u[s])


# ------------------------------------------------------------------
# 3. Texto do Blender → contornos (laços), letra por letra
# ------------------------------------------------------------------
FONTES = {}


def fonte(arq):
    if arq not in FONTES:
        FONTES[arq] = bpy.data.fonts.load(os.path.join(PASTA_DAS_FONTES, arq))
    return FONTES[arq]


def encadear_arestas(V, E):
    viz = {}
    for a, b in E:
        viz.setdefault(a, []).append(b)
        viz.setdefault(b, []).append(a)
    usado, lacos = set(), []
    for ini in viz:
        if ini in usado:
            continue
        laco, ant, atual = [ini], None, ini
        usado.add(ini)
        while True:
            prox = [v for v in viz[atual] if v != ant and v not in usado]
            if not prox:
                break
            ant, atual = atual, prox[0]
            usado.add(atual)
            laco.append(atual)
        L = V[laco]
        L = L[np.r_[True, np.linalg.norm(np.diff(L, axis=0), axis=1) > 1e-6]]
        if len(L) >= 3:
            lacos.append(L)
    return lacos


def letras_do_texto(texto, arq_fonte, resolucao=3, espaco=1.0):
    """O contorno de cada letra (lista de laços, os furos junto), da esquerda para a direita. O texto do
    Blender vira curva (sem preenchimento) e a curva vira as polilinhas do contorno."""
    cu = bpy.data.curves.new('texto', 'FONT')
    cu.body = texto
    cu.font = fonte(arq_fonte)
    cu.size = 1.0
    cu.fill_mode = 'NONE'
    cu.resolution_u = resolucao
    cu.space_character = espaco
    ob = bpy.data.objects.new('texto', cu)
    bpy.context.scene.collection.objects.link(ob)
    dg = bpy.context.evaluated_depsgraph_get()
    oe = ob.evaluated_get(dg)
    me = oe.to_mesh()
    V = np.array([v.co[:2] for v in me.vertices], float)
    E = [tuple(e.vertices) for e in me.edges]
    oe.to_mesh_clear()
    bpy.data.objects.remove(ob)
    bpy.data.curves.remove(cu)
    lacos = encadear_arestas(V, E)
    n = len(lacos)
    contem = [[j for j in range(n) if j != k and ponto_no_poligono(lacos[k][0], lacos[j])] for k in range(n)]
    de_fora = [k for k in range(n) if len(contem[k]) % 2 == 0]
    grupos = {k: [k] for k in de_fora}
    for k in range(n):
        if k in grupos:
            continue
        cands = [j for j in contem[k] if j in grupos]
        if cands:
            grupos[min(cands, key=lambda j: abs(area_assinada(lacos[j])))].append(k)
    # pedaços soltos que caem na faixa de outra letra (o pingo, o acento) ficam com ela
    faixa = {k: (lacos[k][:, 0].min(), lacos[k][:, 0].max()) for k in grupos}
    for k in sorted(grupos, key=lambda k: faixa[k][1] - faixa[k][0]):
        cx = lacos[k][:, 0].mean()
        for j in grupos:
            if j != k and grupos.get(j) and faixa[j][0] < cx < faixa[j][1] and \
                    faixa[j][1] - faixa[j][0] > 1.5 * (faixa[k][1] - faixa[k][0]):
                grupos[j] += grupos[k]
                grupos[k] = []
                break
    letras = [[lacos[i] for i in g] for k, g in grupos.items() if g]
    letras.sort(key=lambda L: min(q[:, 0].min() for q in L))
    return letras


def encaixar(letras, caixa, esticar=(0.8, 1.6), alinhar=0.5, escala=None):
    """Põe as letras na caixa (u0, u1, v0, v1) da placa, em metros; pode esticar na horizontal (de
    esticar[0] a esticar[1] vezes a altura). Devolve as letras e a escala usada (sx, sy)."""
    todos = np.vstack([q for L in letras for q in L])
    x0, x1 = todos[:, 0].min(), todos[:, 0].max()
    y0, y1 = todos[:, 1].min(), todos[:, 1].max()
    u0, u1, v0, v1 = caixa
    if escala is None:
        sx, sy = (u1 - u0) / (x1 - x0), (v1 - v0) / (y1 - y0)
        r = sx / sy
        if r > esticar[1]:
            sx = sy * esticar[1]
        elif r < esticar[0]:
            sy = sx / esticar[0]
    else:
        sx, sy = escala
    largura = (x1 - x0) * sx
    uc = u0 + (u1 - u0 - largura) * alinhar
    vc = (v0 + v1) / 2
    saida = [[np.stack([uc + (q[:, 0] - x0) * sx, vc + (q[:, 1] - (y0 + y1) / 2) * sy], 1) for q in L] for L in letras]
    return saida, (sx, sy)


def contornos_duplos(letra, recuo, tol):
    """O contorno de fora e, se recuo > 0, o de dentro (recuado para o lado cheio da letra)"""
    fora = [simplificar(q, tol) for q in letra]
    dentro = []
    if recuo > 0:
        for q in letra:
            seg = np.roll(q, -1, 0) - q
            k = int(np.argmax(np.linalg.norm(seg, axis=1)))
            meio = q[k] + seg[k] / 2
            t = normalizar(seg[k])
            esq = np.array([-t[1], t[0]])
            cheio_esq = dentro_par_impar(meio + esq * 1e-3, letra)
            r = limpar_recuo(recuar(q, recuo, cheio_esq), letra, recuo)
            if r is not None:
                dentro.append(simplificar(r, tol))
    return fora, dentro


# ------------------------------------------------------------------
# 4. As malhas: tubos, bulbos, cones; tudo com a cor acesa e a ordem por vértice
# ------------------------------------------------------------------
class Malha:
    """Um punhado de pedaços (vértices em eixos do jogo, triângulos, cor RGBA por vértice e, com
    textura, UV por vértice) que vira um ou mais objetos na hora de exportar"""

    def __init__(self, nome, imagem=None):
        self.nome = nome
        self.imagem = imagem
        self.pedacos = []

    def juntar(self, V, F, C, UV=None):
        V = np.asarray(V, float)
        F = np.asarray(F, np.int64).reshape(-1, 3)
        C = np.asarray(C, float)
        if C.ndim == 1:
            C = np.tile(C, (len(V), 1))
        self.pedacos.append((V, F, C, None if UV is None else np.asarray(UV, float)))

    def contagem(self):
        return sum(len(p[0]) for p in self.pedacos), sum(len(p[1]) for p in self.pedacos)


def cor_com_ordem(rgb, alfa, n):
    C = np.empty((n, 4))
    C[:, :3] = rgb
    C[:, 3] = alfa
    return C


def tubo(M, P, raio, rgb, alfa, normal, fechado=True, lados=6):
    """Um tubo redondo ao longo da polilinha P (n,3) que fica num plano de normal 'normal' (o plano da
    placa); 'alfa' é um número ou um por ponto (a ordem de acender ao longo do tubo). O anel de cada
    ponto se alonga no plano nos cantos (meia-esquadria) para o tubo não afinar; as faces ficam no
    sentido anti-horário visto de fora, e as pontas de um tubo aberto ganham tampa."""
    P = np.asarray(P, float)
    alfa = np.broadcast_to(np.asarray(alfa, float), (len(P),)).copy()
    if fechado and len(P) > 2 and np.linalg.norm(P[0] - P[-1]) < 1e-6:
        P, alfa = P[:-1], alfa[:-1]
    ok = np.r_[True, np.linalg.norm(np.diff(P, axis=0), axis=1) > 1e-5]
    P, alfa = P[ok], alfa[ok]
    N = len(P)
    if N < 2:
        return
    nrm = normalizar(normal)
    if fechado:
        T = normalizar(np.roll(P, -1, 0) - P)
        tin, tout = np.roll(T, 1, 0), T
    else:
        T = normalizar(np.diff(P, axis=0))
        tin, tout = np.vstack([T[:1], T]), np.vstack([T, T[-1:]])
    tan = tin + tout
    ruim = np.linalg.norm(tan, axis=1) < 1e-6
    tan[ruim] = tout[ruim]
    tan = normalizar(tan)
    lado = normalizar(np.cross(tan, nrm))
    lado_seg = normalizar(np.cross(tout, nrm))
    esq = 1.0 / np.clip(np.abs((lado * lado_seg).sum(1)), 0.45, 1.0)
    ang = np.arange(lados) * 2 * math.pi / lados
    V = (P[:, None, :] + raio * (np.cos(ang)[None, :, None] * (lado * esq[:, None])[:, None, :] +
                                 np.sin(ang)[None, :, None] * nrm[None, None, :])).reshape(-1, 3)
    C = np.repeat(cor_com_ordem(rgb, 0, N), lados, 0)
    C[:, 3] = np.repeat(alfa, lados)
    F = []
    nseg = N if fechado else N - 1
    for i in range(nseg):
        j = (i + 1) % N
        for k in range(lados):
            a, b = i * lados + k, i * lados + (k + 1) % lados
            c, d = j * lados + (k + 1) % lados, j * lados + k
            F += [(a, c, b), (a, d, c)]
    if not fechado:
        V = np.vstack([V, P[0], P[-1]])
        C = np.vstack([C, C[0], C[(N - 1) * lados]])
        c0, c1 = len(V) - 2, len(V) - 1
        for k in range(lados):
            F.append((c0, k, (k + 1) % lados))
            F.append((c1, (N - 1) * lados + (k + 1) % lados, (N - 1) * lados + k))
    M.juntar(V, F, C)


def esfera(M, c, raios, rgb, alfa, eixos=None, seg=8, aneis=5):
    """Um bulbo (elipsoide) de centro c e semi-eixos raios, nos eixos dados (3x3, linhas) ou nos do mundo"""
    c = np.asarray(c, float)
    E = np.eye(3) if eixos is None else np.asarray(eixos, float)
    rx, ry, rz = raios
    V = [c + E[1] * ry]
    for a in range(1, aneis):
        th = math.pi * a / aneis
        for s in range(seg):
            ph = 2 * math.pi * s / seg
            V.append(c + E[0] * rx * math.sin(th) * math.cos(ph) + E[1] * ry * math.cos(th) +
                     E[2] * rz * math.sin(th) * math.sin(ph))
    V.append(c - E[1] * ry)
    F = []
    for s in range(seg):
        F.append((0, 1 + (s + 1) % seg, 1 + s))
    for a in range(aneis - 2):
        b0, b1 = 1 + a * seg, 1 + (a + 1) * seg
        for s in range(seg):
            s1 = (s + 1) % seg
            F += [(b0 + s, b0 + s1, b1 + s1), (b0 + s, b1 + s1, b1 + s)]
    ult = len(V) - 1
    b0 = 1 + (aneis - 2) * seg
    for s in range(seg):
        F.append((ult, b0 + s, b0 + (s + 1) % seg))
    V = np.array(V)
    M.juntar(V, orientar_para_fora(V, F, c), cor_com_ordem(rgb, alfa, len(V)))


def orientar_para_fora(V, F, centro):
    """Vira os triângulos que olham para o centro (a raylib desenha só a frente, no sentido anti-horário);
    'centro' é um ponto só ou um por triângulo"""
    V = np.asarray(V)
    F = np.asarray(F, np.int64).reshape(-1, 3)
    n = np.cross(V[F[:, 1]] - V[F[:, 0]], V[F[:, 2]] - V[F[:, 0]])
    fora = V[F].mean(1) - np.asarray(centro, float)
    virar = (n * fora).sum(1) < 0
    F[virar] = F[virar][:, ::-1]
    return F


# ------------------------------------------------------------------
# 5. A exportação e a conferência do .glb
# ------------------------------------------------------------------
LIMITE_DE_VERTICES = 60000     # por objeto (a raylib usa índices de 16 bits: até 65.535 por primitiva)


def material_branco(nome, imagem=None):
    m = bpy.data.materials.new(nome)
    try:
        m.use_nodes = True
    except Exception:
        pass
    nt = m.node_tree
    nt.nodes.clear()
    saida = nt.nodes.new('ShaderNodeOutputMaterial')
    bsdf = nt.nodes.new('ShaderNodeBsdfPrincipled')
    bsdf.inputs['Base Color'].default_value = (1, 1, 1, 1)
    bsdf.inputs['Roughness'].default_value = 0.5
    if imagem is not None:
        tex = nt.nodes.new('ShaderNodeTexImage')
        tex.image = imagem
        nt.links.new(tex.outputs['Color'], bsdf.inputs['Base Color'])
    nt.links.new(bsdf.outputs[0], saida.inputs['Surface'])
    return m


def criar_objetos(malhas, prefixo):
    """Os pedaços viram objetos do Blender (cada um com menos de LIMITE_DE_VERTICES), nos eixos do Blender"""
    objs = []
    for M in malhas:
        if not M.pedacos:
            continue
        mat = material_branco(prefixo + '_' + M.nome, M.imagem)
        grupos, atual, conta = [], [], 0
        for p in M.pedacos:
            if atual and conta + len(p[0]) > LIMITE_DE_VERTICES:
                grupos.append(atual)
                atual, conta = [], 0
            atual.append(p)
            conta += len(p[0])
        if atual:
            grupos.append(atual)
        for gi, g in enumerate(grupos):
            Vs, Fs, Cs, UVs, base = [], [], [], [], 0
            for V, F, C, UV in g:
                Vs.append(V)
                Fs.append(F + base)
                Cs.append(C)
                UVs.append(UV if UV is not None else np.zeros((len(V), 2)))
                base += len(V)
            V = np.vstack(Vs)
            F = np.vstack(Fs)
            C = np.vstack(Cs)
            UV = np.vstack(UVs)
            me = bpy.data.meshes.new('%s_%s_%d' % (prefixo, M.nome, gi))
            Vb = np.stack([V[:, 0], -V[:, 2], V[:, 1]], 1)
            me.vertices.add(len(Vb))
            me.vertices.foreach_set('co', Vb.ravel())
            me.loops.add(len(F) * 3)
            me.loops.foreach_set('vertex_index', F.ravel())
            me.polygons.add(len(F))
            me.polygons.foreach_set('loop_start', np.arange(0, len(F) * 3, 3))
            me.polygons.foreach_set('loop_total', np.full(len(F), 3))
            me.update(calc_edges=True)
            at = me.color_attributes.new('Col', 'FLOAT_COLOR', 'CORNER')
            at.data.foreach_set('color', C[F.ravel()].ravel())
            me.color_attributes.active_color = at
            if M.imagem is not None:
                uvl = me.uv_layers.new(name='UVMap')
                uvl.data.foreach_set('uv', UV[F.ravel()].ravel())
            me.materials.append(mat)
            o = bpy.data.objects.new(me.name, me)
            bpy.context.scene.collection.objects.link(o)
            objs.append(o)
    return objs


def ler_glb(caminho):
    d = open(caminho, 'rb').read()
    n = struct.unpack('<I', d[12:16])[0]
    return json.loads(d[20:20 + n]), d


def cores_do_glb(j, d):
    n_json = struct.unpack('<I', d[12:16])[0]
    base = 20 + n_json + 8
    tipos = {5121: (np.uint8, 255.0), 5123: (np.uint16, 65535.0), 5126: (np.float32, 1.0)}
    todas = []
    for me in j['meshes']:
        for p in me['primitives']:
            acc = j['accessors'][p['attributes']['COLOR_0']]
            bv = j['bufferViews'][acc['bufferView']]
            dt, esc = tipos[acc['componentType']]
            comps = {'VEC3': 3, 'VEC4': 4}[acc['type']]
            ini = base + bv.get('byteOffset', 0) + acc.get('byteOffset', 0)
            todas.append(np.frombuffer(d, dt, count=acc['count'] * comps, offset=ini).reshape(-1, comps) / esc)
    return np.vstack(todas)


CONTAGENS = {}


def exportar(lugar, malhas):
    objs = criar_objetos(malhas, lugar)
    esperado = np.vstack([np.vstack([p[2] for p in M.pedacos]) for M in malhas if M.pedacos])
    bpy.ops.object.select_all(action='DESELECT')
    for o in objs:
        o.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]
    caminho = os.path.join(PASTA_DE_SAIDA, lugar + '.glb')
    bpy.ops.export_scene.gltf(filepath=caminho, export_format='GLB', use_selection=True, export_yup=True,
                              export_apply=True, export_animations=False, export_normals=False,
                              export_vertex_color='ACTIVE', export_all_vertex_colors=False,
                              export_active_vertex_color_when_no_material=True, export_image_format='AUTO',
                              export_materials='EXPORT')
    j, d = ler_glb(caminho)
    prims = [p for me in j['meshes'] for p in me['primitives']]
    sem_cor = sum(1 for p in prims if 'COLOR_0' not in p['attributes'])
    tipos = sorted({j['accessors'][p['attributes']['COLOR_0']]['type'] for p in prims if 'COLOR_0' in p['attributes']})
    tris = sum(j['accessors'][p['indices']]['count'] // 3 for p in prims)
    maior = max(j['accessors'][p['attributes']['POSITION']]['count'] for p in prims)
    imgs = [(im.get('name'), im.get('mimeType')) for im in j.get('images', [])]
    fatores = [m.get('pbrMetallicRoughness', {}).get('baseColorFactor', [1, 1, 1, 1]) for m in j['materials']]
    lido = cores_do_glb(j, d)
    print('Exportado %s' % caminho)
    print('  primitivas: %d (sem COLOR_0: %d; COLOR_0 %s); maior: %d vértices' % (len(prims), sem_cor, tipos, maior))
    print('  triângulos: %d; materiais: %d; texturas: %s; baseColorFactor: %s' % (tris, len(j['materials']), imgs, fatores))
    print('  COLOR_0 lido do .glb: média %s; no Blender: %s; A de %.2f a %.2f' % (
        np.round(lido.mean(0), 3), np.round(esperado.mean(0), 3), lido[:, 3].min(), lido[:, 3].max()))
    print('  tamanho: %.2f MB' % (os.path.getsize(caminho) / 1e6))
    if sem_cor or tipos != ['VEC4']:
        raise SystemExit('ERRO: há primitivas sem COLOR_0 VEC4')
    if maior > 65535:
        raise SystemExit('ERRO: primitiva com mais de 65.535 vértices')
    if any(mt != 'image/png' for _, mt in imgs):
        raise SystemExit('ERRO: textura que não é PNG')
    if np.abs(lido.mean(0) - esperado.mean(0)).max() > 0.01:
        raise SystemExit('ERRO: as cores do .glb não batem com as do Blender')
    CONTAGENS[lugar] = dict(triangulos=tris, primitivas=len(prims), maior_primitiva=maior,
                            texturas=imgs, megabytes=round(os.path.getsize(caminho) / 1e6, 2))
    for o in objs:
        o.hide_render = True
        o.hide_set(True)
    return caminho


def gravar_json(lugar, nome, acender_em, luzes):
    dados = {'nome': nome, 'acender_em': acender_em,
             'luzes': [{'x': round(float(x), 2), 'y': round(float(y), 2), 'z': round(float(z), 2),
                        'r': round(float(c[0]), 3), 'g': round(float(c[1]), 3), 'b': round(float(c[2]), 3),
                        'raio': round(float(raio), 1), 'forca': round(float(forca), 2)}
                       for (x, y, z), c, raio, forca in luzes]}
    caminho = os.path.join(PASTA_DE_SAIDA, lugar + '.json')
    with open(caminho, 'w') as f:
        json.dump(dados, f, indent=1, ensure_ascii=False)
    print('Gravado %s (%d luzes)' % (caminho, len(luzes)))
    return dados


def ilhas(T, idx, com_uv=False):
    """Os triângulos de idx agrupados em ilhas ligadas (vértices na mesma posição; com_uv, e com a mesma
    UV: separa peças que só se encostam)"""
    pai = list(range(len(idx)))

    def raiz(a):
        while pai[a] != a:
            pai[a] = pai[pai[a]]
            a = pai[a]
        return a
    dono = {}
    for k, t in enumerate(idx):
        for q, w in zip(T.p[t], T.uv[t]):
            ch = tuple(np.round(q, 3)) + (tuple(np.round(w, 4)) if com_uv else ())
            if ch in dono:
                a, b = raiz(k), raiz(dono[ch])
                if a != b:
                    pai[a] = b
            else:
                dono[ch] = k
    grupos = {}
    for k in range(len(idx)):
        grupos.setdefault(raiz(k), []).append(idx[k])
    return [np.array(g) for g in grupos.values()]


def ao_longo(P, a0, a1, fechado=True):
    """A ordem de acender correndo de a0 a a1 ao longo da polilinha (a moldura que se desenha)"""
    P = np.asarray(P, float)
    d = np.linalg.norm(np.diff(P, axis=0), axis=1)
    s = np.r_[0, np.cumsum(d)]
    total = s[-1] + (np.linalg.norm(P[0] - P[-1]) if fechado else 0)
    return a0 + (a1 - a0) * s / max(total, 1e-9)


def tubo_fechado_com_volta(M, P, raio, rgb, a0, a1, normal, lados=6):
    """Um laço que se desenha: o primeiro ponto se repete no fim, com a ordem do fim"""
    P = np.vstack([P, P[:1]])
    tubo(M, P, raio, rgb, ao_longo(P, a0, a1, fechado=False), normal, fechado=False, lados=lados)


# ------------------------------------------------------------------
# 6. Sarge's Surplus Hut
# ------------------------------------------------------------------
parte("Sarge's Surplus Hut")
SARGE = Tris('ror_Object_51')
M_SARGE = Malha('neon')
LUZES_SARGE = []
CAIXA_SARGE = (695.0, 709.0, 6.0, 13.5, 105.0, 122.0)
TOL = 0.006                     # simplificação dos contornos (m)


def divisa(pd):
    """A divisa de sargento-mor na placa pd (as divisas em cima, o círculo com a estrela, os arcos
    embaixo); devolve [(laço, cor, ordem)] em coordenadas da placa"""
    C = pd.contorno
    u0, u1 = C[:, 0].min(), C[:, 0].max()
    v1 = C[:, 1].max()
    uc, W = (u0 + u1) / 2, u1 - u0
    ombro = C[np.abs(C[:, 0] - uc) > 0.42 * W][:, 1].max()
    k = (v1 - ombro) / (W / 2)                                   # a inclinação do "telhado"
    m = 0.12
    e = W / 2 - m
    banda, vao = 0.20, 0.11
    pecas = []
    A = v1 - m * 1.3
    for i in range(3):
        a = A - i * (banda + vao)
        q = np.array([(-e, a - k * e), (0, a), (e, a - k * e), (e, a - banda - k * e), (0, a - banda), (-e, a - banda - k * e)])
        pecas.append((arredondar(q + [uc, 0], 0.05), DIVISA, 0.62 + 0.04 * i))
    a2 = A - 2 * (banda + vao) - banda
    rc = 0.30
    vc = a2 - k * rc - 0.08 - rc
    ang = np.linspace(0, 2 * math.pi, 25)[:-1]
    pecas.append((np.stack([uc + rc * np.cos(ang), vc + rc * np.sin(ang)], 1), DOURADO, 0.75))
    est = []
    for j in range(10):
        r = 0.22 if j % 2 == 0 else 0.088
        t = math.pi / 2 + j * math.pi / 5
        est.append((uc + r * math.cos(t), vc + r * math.sin(t)))
    pecas.append((np.array(est), DOURADO, 0.79))
    for i in range(3):
        ri = rc + 0.16 + i * 0.27
        ro = ri + 0.17
        fi = min(math.radians(66), math.asin(min(1.0, e / ro)))
        f = np.linspace(-fi, fi, 13)
        fora = np.stack([uc + ro * np.sin(f), vc - ro * np.cos(f)], 1)
        dentro = np.stack([uc + ri * np.sin(f[::-1]), vc - ri * np.cos(f[::-1])], 1)
        pecas.append((arredondar(np.vstack([fora, dentro]), 0.04), DIVISA, 0.84 + 0.04 * i))
    return pecas


def centro_do_arco(Q):
    """O centro (e o raio médio) do círculo que melhor passa pelos pontos Q (n,2)"""
    A = np.c_[2 * Q, np.ones(len(Q))]
    b = (Q ** 2).sum(1)
    x = np.linalg.lstsq(A, b, rcond=None)[0]
    c = x[:2]
    return c, math.sqrt(x[2] + c @ c)


for lado in (-1, +1):
    normal = (lado, 0, 0)
    # a placa: SARGE'S em cima, SURPLUS HUT embaixo, onde estão pintados
    pl = Placa(SARGE, SARGE.escolher(tex=(110, 240, 104, 156), normal=normal, caixa=CAIXA_SARGE))
    letras, _ = encaixar(letras_do_texto("SARGE'S", 'saira_condensed_bold.ttf'), pl.caixa_tex(121, 226, 128, 143.5),
                         esticar=(0.8, 1.8))
    for i, L in enumerate(letras):
        a = 0.08 + 0.28 * i / (len(letras) - 1)
        fora, dentro = contornos_duplos(L, 0.05, TOL)
        for q in fora:
            tubo(M_SARGE, pl.mundo(q, 0.05), 0.028, VERDE_SARGE, a, pl.n)
        for q in dentro:
            tubo(M_SARGE, pl.mundo(q, 0.05), 0.02, VERDE_SARGE, a, pl.n)
    letras, _ = encaixar(letras_do_texto('SURPLUS HUT', 'saira_condensed_bold.ttf'), pl.caixa_tex(124, 225, 117.5, 126.5),
                         esticar=(0.8, 1.8))
    for i, L in enumerate(letras):
        a = 0.42 + 0.16 * i / (len(letras) - 1)
        for q in contornos_duplos(L, 0, TOL)[0]:
            tubo(M_SARGE, pl.mundo(q, 0.05), 0.022, VERDE_SARGE, a, pl.n)
    meio = pl.mundo(np.array([[np.mean(pl.contorno[:, 0]), np.mean(pl.contorno[:, 1]) - 2.6]]), 3.6)[0]
    LUZES_SARGE.append((meio, VERDE_SARGE, 12.0, 1.0))

    # a divisa
    pd = Placa(SARGE, SARGE.escolher(tex=(30, 108, 78, 150), normal=normal, caixa=CAIXA_SARGE))
    for q, cor, a in divisa(pd):
        tubo(M_SARGE, pd.mundo(q, 0.05), 0.03, cor, a, pd.n)
    meio = pd.mundo(np.array([[np.mean(pd.contorno[:, 0]), np.mean(pd.contorno[:, 1]) - 2.8]]), 4.0)[0]
    LUZES_SARGE.append((meio, (0.85, 1.0, 0.25), 11.0, 0.7))

    # as lâmpadas do arco, do lado da divisa até a seta
    pa = Placa(SARGE, SARGE.escolher(tex=(95, 130, 178, 201), normal=normal, caixa=CAIXA_SARGE))
    Q = pa.contorno
    c, R = centro_do_arco(Q)
    rho = np.linalg.norm(Q - c, axis=1)
    th = np.arctan2(Q[:, 1] - c[1], Q[:, 0] - c[0])
    fo, de = rho > R, rho <= R
    t0 = max(th[fo].min(), th[de].min())
    t1 = min(th[fo].max(), th[de].max())
    ts = np.linspace(t0, t1, 8)[1:-1]
    so, sd = np.argsort(th[fo]), np.argsort(th[de])
    r_meio = (np.interp(ts, th[fo][so], rho[fo][so]) + np.interp(ts, th[de][sd], rho[de][sd])) / 2
    pts = np.stack([c[0] + r_meio * np.cos(ts), c[1] + r_meio * np.sin(ts)], 1)
    if pts[0, 1] > pts[-1, 1]:
        pts = pts[::-1]                    # começa embaixo (perto da divisa)
    for j, q in enumerate(pts):
        esfera(M_SARGE, pa.mundo(q, 0.07), (0.075, 0.075, 0.075), LAMPADA, 0.40 + 0.50 * j / (len(pts) - 1))

    # o contorno da ponta da seta
    ps = Placa(SARGE, SARGE.escolher(tex=(94, 129, 153, 177), normal=normal, caixa=CAIXA_SARGE))
    q = recuar(ps.contorno, 0.06)
    q = arredondar(simplificar(q, 0.01), 0.04)
    tubo(M_SARGE, ps.mundo(q, 0.05), 0.025, AMBAR, 1.0, ps.n)

# a lâmpada do arco (uma luz só, no meio do arco)
LUZES_SARGE.append(((701.9, 12.4, 117.4), LAMPADA, 7.0, 0.6))

# a arandela em cima da placa: a lâmpada dentro da cúpula, virada para a placa
idx = SARGE.escolher(caixa=(701.4, 702.4, 9.85, 10.45, 114.8, 115.45))
V = SARGE.p[idx].reshape(-1, 3)
abre = normalizar(np.array([0.0, -0.57, 0.82]))
centro = V.mean(0) + abre * 0.10
esfera(M_SARGE, centro, (0.12, 0.12, 0.12), LAMPADA, 0.0)
LUZES_SARGE.append((centro + abre * 0.6, LAMPADA, 8.0, 0.9))
# a luminária da porta (a cúpula larga embaixo da placa, aberta para baixo)
idx = SARGE.escolher(caixa=(701.2, 702.6, 6.35, 6.85, 113.9, 115.3))
V = SARGE.p[idx].reshape(-1, 3)
cx, cz = (V[:, 0].min() + V[:, 0].max()) / 2, (V[:, 2].min() + V[:, 2].max()) / 2
rr = min(V[:, 0].max() - V[:, 0].min(), V[:, 2].max() - V[:, 2].min()) / 2 * 0.82
esfera(M_SARGE, (cx, V[:, 1].min() + 0.05, cz), (rr, 0.07, rr), LAMPADA, 0.02)
LUZES_SARGE.append(((cx, V[:, 1].min() - 0.8, cz + 0.4), LAMPADA, 9.0, 1.1))
print('  Sarge: %d vértices, %d triângulos' % M_SARGE.contagem())


# ------------------------------------------------------------------
# 7. Fillmore: o domo em tinta fluorescente, a cortina de contas, a paz, a lâmpada
# ------------------------------------------------------------------
parte('Fillmore')
FILL = Tris('ror_Object_56')
LUZES_FILLMORE = []


def hsv_de_rgb(c):
    r, g, b = c[..., 0], c[..., 1], c[..., 2]
    v = c.max(-1)
    mn = c.min(-1)
    d = v - mn
    s = np.where(v > 1e-6, d / np.maximum(v, 1e-6), 0)
    h = np.zeros_like(v)
    dd = np.maximum(d, 1e-6)
    h = np.where(v == r, ((g - b) / dd) % 6, h)
    h = np.where(v == g, (b - r) / dd + 2, h)
    h = np.where(v == b, (r - g) / dd + 4, h)
    h = np.where(d < 1e-6, 0, h) / 6.0
    return h % 1.0, s, v


def rgb_de_hsv(h, s, v):
    i = np.floor(h * 6).astype(int) % 6
    f = h * 6 - np.floor(h * 6)
    p, q, t = v * (1 - s), v * (1 - f * s), v * (1 - (1 - f) * s)
    r = np.choose(i, [v, q, p, p, t, v])
    g = np.choose(i, [t, v, v, q, p, p])
    b = np.choose(i, [p, p, t, v, v, q])
    return np.stack([r, g, b], -1)


def suave(a, b, x):
    t = np.clip((x - a) / (b - a), 0, 1)
    return t * t * (3 - 2 * t)


def textura_fluorescente(im, arestas_uv):
    """A pintura do mapa (ror_Image_14) como tinta fluorescente sob luz negra, no dobro do tamanho: as
    cores saturadas (o amarelo puxado para o laranja), os brancos e cinzas claros violeta, os escuros
    mais escuros; e as arestas da estrutura do domo (as arestas das UVs) pintadas de quase preto"""
    W, H = im.size
    px = np.array(im.pixels[:], np.float32).reshape(H, W, 4)[::-1, :, :3]     # linha 0 em cima
    # o dobro, bilinear
    ys = (np.arange(2 * H) + 0.5) / 2 - 0.5
    xs = (np.arange(2 * W) + 0.5) / 2 - 0.5
    y0 = np.clip(np.floor(ys).astype(int), 0, H - 1)
    x0 = np.clip(np.floor(xs).astype(int), 0, W - 1)
    y1, x1 = np.clip(y0 + 1, 0, H - 1), np.clip(x0 + 1, 0, W - 1)
    fy = np.clip(ys - y0, 0, 1)[:, None, None]
    fx = np.clip(xs - x0, 0, 1)[None, :, None]
    c = (px[y0][:, x0] * (1 - fy) * (1 - fx) + px[y0][:, x1] * (1 - fy) * fx +
         px[y1][:, x0] * fy * (1 - fx) + px[y1][:, x1] * fy * fx)
    h, s, v = hsv_de_rgb(c)
    h2 = (h - 0.08 * np.exp(-((h - 0.15) / 0.06) ** 2) + 0.03 * np.exp(-((h - 0.80) / 0.08) ** 2)) % 1.0
    s2 = np.clip(0.70 + s * 1.5, 0, 1)
    v2 = np.clip(v ** 0.85 * 1.05, 0, 1)
    viva = rgb_de_hsv(h2, s2, v2)
    branco = np.array([0.50, 0.22, 0.95])[None, None] * (0.40 + 0.60 * v)[..., None]
    w_branco = (suave(0.24, 0.10, s) * suave(0.35, 0.6, v))[..., None]
    out = viva * (1 - w_branco) + branco * w_branco
    escuro = suave(0.30, 0.12, v)[..., None]
    out = out * (1 - escuro) + c * 0.55 * escuro
    # as arestas da estrutura geodésica: linhas escuras de ~3 px (16 cm) no dobro do tamanho
    ESTRUTURA = np.array([0.05, 0.02, 0.08])
    meia = 1.6
    for (a, b) in arestas_uv:
        a = np.array([a[0] * 2 * W, (1 - a[1]) * 2 * H])
        b = np.array([b[0] * 2 * W, (1 - b[1]) * 2 * H])
        xa, xb = int(max(0, min(a[0], b[0]) - 4)), int(min(2 * W, max(a[0], b[0]) + 5))
        ya, yb = int(max(0, min(a[1], b[1]) - 4)), int(min(2 * H, max(a[1], b[1]) + 5))
        if xb <= xa or yb <= ya:
            continue
        gy, gx = np.mgrid[ya:yb, xa:xb] + 0.5
        d = b - a
        t = np.clip(((gx - a[0]) * d[0] + (gy - a[1]) * d[1]) / max(d @ d, 1e-9), 0, 1)
        dist = np.hypot(gx - a[0] - t * d[0], gy - a[1] - t * d[1])
        w = np.clip(meia + 0.5 - dist, 0, 1)[..., None]
        out[ya:yb, xa:xb] = out[ya:yb, xa:xb] * (1 - w) + ESTRUTURA * w
    rgba = np.concatenate([np.clip(out, 0, 1), np.ones((2 * H, 2 * W, 1), np.float32)], 2)
    nova = bpy.data.images.new('fillmore_fluorescente', 2 * W, 2 * H, alpha=False)
    nova.pixels.foreach_set(rgba[::-1].astype(np.float32).ravel())
    caminho = os.path.join(PASTA_TEMP, 'fillmore_fluorescente.png')
    nova.filepath_raw = caminho
    nova.file_format = 'PNG'
    nova.save()
    bpy.data.images.remove(nova)
    nova = bpy.data.images.load(caminho)
    nova.name = 'fillmore_fluorescente'
    nova.pack()
    return nova


# o domo: as faces pintadas com o sol, os cactos e os arcos-íris
idx_domo = FILL.escolher(tex=(0, 240, 0, 216), caixa=(748.0, 770.0, 1.9, 12.0, 88.0, 110.0))
P = FILL.p[idx_domo]
UV = FILL.uv[idx_domo]
Vt = P.reshape(-1, 3)
centro_domo = np.array([(Vt[:, 0].min() + Vt[:, 0].max()) / 2, Vt[:, 1].min(), (Vt[:, 2].min() + Vt[:, 2].max()) / 2])
fn = np.cross(P[:, 1] - P[:, 0], P[:, 2] - P[:, 0])
fora = (fn * (P.mean(1) - centro_domo)).sum(1) < 0
P[fora] = P[fora][:, ::-1]                    # todas as faces viradas para fora
UV[fora] = UV[fora][:, ::-1]
fn = normalizar(np.cross(P[:, 1] - P[:, 0], P[:, 2] - P[:, 0]))
soma = {}
for t in range(len(P)):
    for q in P[t]:
        ch = tuple(np.round(q, 3))
        soma[ch] = soma.get(ch, 0) + fn[t]
desloc = np.array([[normalizar(soma[tuple(np.round(q, 3))]) for q in tri] for tri in P])
P2 = P + 0.025 * desloc
arestas = set()
for tri in UV:
    for a, b in ((0, 1), (1, 2), (2, 0)):
        arestas.add(tuple(sorted((tuple(np.round(tri[a], 5)), tuple(np.round(tri[b], 5))))))
IMAGEM_FLUOR = textura_fluorescente(FILL.imagem, arestas)
M_FILL_TEX = Malha('pintura', IMAGEM_FLUOR)
ymin, ymax = Vt[:, 1].min(), Vt[:, 1].max()
Vd = P2.reshape(-1, 3)
alfa = 0.08 + 0.37 * (Vd[:, 1] - ymin) / (ymax - ymin)
M_FILL_TEX.juntar(Vd, np.arange(len(Vd)).reshape(-1, 3), cor_com_ordem((1, 1, 1), alfa, len(Vd)), UV.reshape(-1, 2))
print('  domo: %d faces; centro (%.1f, %.1f, %.1f), de y %.2f a %.2f' % (len(P), *centro_domo, ymin, ymax))

# o símbolo da paz da beira da estrada (frente e verso)
idx = FILL.escolher(tex=(240, 273, 100, 146), caixa=(774.5, 778.0, 2.0, 4.6, 126.0, 129.0))
P = FILL.p[idx] + FILL.n[idx][:, None, :] * 0.02
Vp = P.reshape(-1, 3)
M_FILL_TEX.juntar(Vp, np.arange(len(Vp)).reshape(-1, 3), cor_com_ordem((1, 1, 1), 0.55, len(Vp)),
                  FILL.uv[idx].reshape(-1, 2))
paz = Vp.mean(0)
print('  símbolo da paz: %d faces em (%.1f, %.1f, %.1f)' % (len(idx), *paz))

M_FILL = Malha('neon')
# a lâmpada redonda do poste: a luz negra
idx = FILL.escolher(tex=(455, 482, 75, 100), caixa=(761.5, 764.0, 5.5, 7.2, 113.3, 114.8))
V = FILL.p[idx].reshape(-1, 3)
cl = V.mean(0)
nl = normalizar((FILL.n[idx] * FILL.area[idx][:, None]).sum(0))
e1 = normalizar(np.cross([0, 1, 0], nl))
e2 = np.cross(nl, e1)
rl = np.linalg.norm(V - cl, axis=1).max()
esfera(M_FILL, cl + nl * 0.06, (rl * 0.92, rl * 0.92, 0.12), VIOLETA, 0.0, eixos=np.array([e1, e2, nl]), seg=12, aneis=6)
LUZES_FILLMORE.append((cl + nl * 0.6, (0.62, 0.40, 1.0), 12.0, 0.9))
print('  lâmpada do poste em (%.1f, %.1f, %.1f), raio %.2f' % (*cl, rl))

# a cortina de contas na entrada (o trapézio listrado do fundo do túnel)
idx = FILL.escolher(tex=(220, 295, 50, 97), caixa=(750.0, 757.0, 2.0, 6.5, 97.0, 106.0))
V = np.unique(np.round(FILL.p[idx].reshape(-1, 3), 3), axis=0)
ncort = normalizar((FILL.n[idx] * FILL.area[idx][:, None]).sum(0))
if ncort @ (np.array([750.0, 4.0, 102.0]) - V.mean(0)) < 0:
    ncort = -ncort
lado_c = normalizar(np.cross([0, 1, 0], ncort))
baixo = V[np.argsort(V[:, 1])[:2]]
cima = V[np.argsort(V[:, 1])[-2:]]
baixo = baixo[np.argsort(baixo @ lado_c)]
cima = cima[np.argsort(cima @ lado_c)]
CONTAS = [(1.00, 0.45, 0.08), (0.20, 0.45, 1.00), (0.72, 0.22, 1.00), (1.00, 0.24, 0.60), (0.50, 1.00, 0.22)]
FIOS = 30
for f in range(FIOS):
    s = (f + 0.5) / FIOS
    topo = cima[0] + s * (cima[1] - cima[0]) + ncort * 0.12
    pe = baixo[0] + s * (baixo[1] - baixo[0]) + ncort * 0.12
    d = pe - topo
    comp = np.linalg.norm(d)
    d = d / comp
    n_contas = int((comp - 0.12) / 0.105)
    ex = normalizar(np.cross(d, ncort))
    ez = np.cross(ex, d)
    for b in range(n_contas):
        cc = topo + d * (0.07 + b * 0.105)
        cor = CONTAS[(f // 2 + b // 3) % len(CONTAS)]
        esfera(M_FILL, cc, (0.034, 0.046, 0.034), cor, 0.55 + 0.40 * s, eixos=np.array([ex, -d, ez]), seg=6, aneis=3)
meio_cortina = (cima.mean(0) + baixo.mean(0)) / 2
LUZES_FILLMORE.append((meio_cortina + ncort * 2.2, (0.85, 0.38, 0.85), 9.0, 1.0))
print('  cortina: %d fios na entrada em (%.1f, %.1f, %.1f)' % (FIOS, *meio_cortina))

# a luz negra em volta do domo: o roxo que pinta o chão e as árvores
for ang in (90, 180, 270, 0):
    a = math.radians(ang)
    LUZES_FILLMORE.append(((centro_domo[0] + 10.5 * math.sin(a), centro_domo[1] + 1.2, centro_domo[2] + 10.5 * math.cos(a)),
                           (0.55, 0.26, 1.0), 14.0, 1.1))
LUZES_FILLMORE.append((paz + np.array([0.0, 0.8, 0.9]), (0.70, 0.35, 1.0), 6.0, 0.6))
print('  Fillmore: neon %d vértices / %d triângulos; pintura %d / %d' % (M_FILL.contagem() + M_FILL_TEX.contagem()))


# ------------------------------------------------------------------
# 8. Cozy Cone: a placa grande e as luminárias da praça
# ------------------------------------------------------------------
parte('Cozy Cone')
COZY = Tris('ror_Object_8')
M_COZY = Malha('neon')
LUZES_COZY = []
PE_DA_PLACA = np.array([697.6, 2.3, 145.0])


def carrinho():
    """O carrinho dormindo (de frente, como no neon do filme), numa caixa de 2 m de largura por
    2,05 m de altura, com a base em v = 0: [(polilinha, fechada)]"""
    corpo = np.array([(-1.0, 0.42), (-1.0, 0.95), (-0.88, 1.12), (-0.80, 1.55), (-0.62, 1.86), (-0.30, 2.02),
                      (0.0, 2.05), (0.30, 2.02), (0.62, 1.86), (0.80, 1.55), (0.88, 1.12), (1.0, 0.95), (1.0, 0.42),
                      (0.9, 0.32), (-0.9, 0.32)])
    vidro = np.array([(-0.68, 1.16), (-0.60, 1.58), (-0.38, 1.78), (0.38, 1.78), (0.60, 1.58), (0.68, 1.16)])
    pecas = [(arredondar(corpo, 0.10), True), (arredondar(vidro, 0.08), True)]
    f = np.linspace(math.radians(200), math.radians(340), 7)
    for cx in (-0.30, 0.30):
        pecas.append((np.stack([cx + 0.19 * np.cos(f), 1.50 + 0.19 * np.sin(f)], 1), False))   # olhos fechados
    a = np.linspace(0, 2 * math.pi, 11)[:-1]
    for cx in (-0.62, 0.62):
        pecas.append((np.stack([cx + 0.12 * np.cos(a), 0.74 + 0.12 * np.sin(a)], 1), True))     # faróis
    f = np.linspace(math.radians(215), math.radians(325), 7)
    pecas.append((np.stack([0.32 * np.cos(f), 1.02 + 0.32 * np.sin(f)], 1), False))             # o sorriso
    for s in (-1, 1):
        pecas.append((np.array([(s * 0.80, 0.32), (s * 0.80, 0.0), (s * 0.52, 0.0), (s * 0.52, 0.32)]), False))
    return pecas


def letra_z(tam):
    """Um Z de um traço só (largura 0,8 da altura), centrado na origem"""
    w, h = tam * 0.4, tam / 2
    return np.array([(-w, h), (w, h), (-w, -h), (w, -h)])


for lado in (-1, +1):
    normal = (lado, 0, 0)
    # MOTEL: as letras onde estão pintadas, contorno duplo branco, e a moldura branca da placa
    pm = Placa(COZY, COZY.escolher(tex=(95, 252, 0, 46.5), normal=normal, caixa=(695, 700, 12.5, 16.0, 138, 151)))
    letras, _ = encaixar(letras_do_texto('MOTEL', 'saira_condensed_semibold.ttf', espaco=1.12), pm.caixa_tex(109, 226, 9.5, 36.5),
                         esticar=(0.8, 2.2))
    for i, L in enumerate(letras):
        a = 0.84 + 0.10 * i / (len(letras) - 1)
        fora, dentro = contornos_duplos(L, 0.13, TOL)
        for q in fora:
            tubo(M_COZY, pm.mundo(q), 0.036, BRANCO_MOTEL, a, pm.n, lados=8)
        for q in dentro:
            tubo(M_COZY, pm.mundo(q), 0.028, BRANCO_MOTEL, a, pm.n)
    q = arredondar(simplificar(recuar(pm.contorno, 0.17), 0.01), 0.12)
    tubo_fechado_com_volta(M_COZY, pm.mundo(q), 0.04, BRANCO_MOTEL, 0.78, 0.83, pm.n)

    # VACANCY na plaquinha de baixo
    pv = Placa(COZY, COZY.escolher(tex=(78, 176, 46, 71), normal=normal, caixa=(695, 700, 10.9, 12.6, 140, 148)))
    C = pv.contorno
    cx = (C[:, 0].min() + 0.45, C[:, 0].max() - 0.45, C[:, 1].min() + 0.24, C[:, 1].max() - 0.20)
    letras, _ = encaixar(letras_do_texto('VACANCY', 'lobster.ttf'), cx, esticar=(0.8, 1.5))
    for L in letras:
        for q in contornos_duplos(L, 0, TOL)[0]:
            tubo(M_COZY, pv.mundo(q), 0.026, LARANJA_VACANCY, 1.0, pv.n)

    # o cone (contorno duplo laranja), o carrinho dormindo e os Z
    pc = Placa(COZY, COZY.escolher(tex=(0, 92, 66, 176), normal=normal, caixa=(695, 700, 15.6, 23.4, 143, 151)))
    for recuo, raio, a0 in ((0.13, 0.045, 0.50), (0.33, 0.04, 0.52)):
        q = arredondar(simplificar(recuar(pc.contorno, recuo), 0.01), 0.10)
        tubo_fechado_com_volta(M_COZY, pc.mundo(q), raio, LARANJA_COZY, a0, a0 + 0.08, pc.n, lados=8)
    u0, u1, v0, v1 = pc.caixa_tex(30, 60, 85, 118)
    base_cone = pc.contorno[:, 1].min()
    topo_cone = pc.contorno[:, 1].max()
    # o eixo do cone: o meio da ponta de cima
    pontas = pc.contorno[pc.contorno[:, 1] > topo_cone - 0.05]
    eixo_u = pontas[:, 0].mean()
    # a base do cone (onde a placa de baixo acaba): a altura em que o cone fica estreito
    alturas = np.linspace(base_cone + 0.05, topo_cone - 0.05, 200)
    larguras = np.array([np.ptp(cortes_horizontais(pc.contorno, v)) if len(cortes_horizontais(pc.contorno, v)) >= 2 else 0
                         for v in alturas])
    v_base = alturas[np.argmax(larguras < larguras[0] - 0.4)]
    alt_carro = min(v1 - v0, 2.3)
    esc = alt_carro / 2.05
    base_carro = v_base + 0.42
    for q, fechado in carrinho():
        Q = np.stack([eixo_u + q[:, 0] * esc, base_carro + q[:, 1] * esc], 1)
        tubo(M_COZY, pc.mundo(Q), 0.034, AMARELO_COZY, 0.62, pc.n, fechado=fechado)
    zs = [(0.62, 0.10), (0.50, -0.08), (0.40, 0.05)]
    vz = base_carro + 2.05 * esc + 0.30
    for k, (tam, du) in enumerate(zs):
        vz += tam / 2
        Q = letra_z(tam) + [eixo_u + du, vz]
        tubo(M_COZY, pc.mundo(Q), 0.03, CIANO_Z if k != 1 else VERDE_Z, 0.68 + 0.04 * k, pc.n, fechado=False)
        vz += tam / 2 + 0.16

    # COZY CONE amarelo (duplo), dentro da moldura laranja que segue a borda da placa
    pz = Placa(COZY, COZY.escolher(tex=(158, 256, 40, 175), normal=normal, caixa=(695, 700, 15.6, 20.4, 138.5, 146.5)))
    q = arredondar(simplificar(recuar(pz.contorno, 0.16), 0.01), 0.16)
    tubo_fechado_com_volta(M_COZY, pz.mundo(q), 0.045, LARANJA_COZY, 0.0, 0.10, pz.n, lados=8)
    # o lugar livre para as letras: a placa menos o cone, que fica na frente dela (à direita de quem lê
    # pelo oeste, à esquerda pelo leste)
    cone_aqui = pz.local(pc.mundo(pc.contorno, 0.0))
    cone_a_direita = cone_aqui[:, 0].mean() > pz.contorno[:, 0].mean()
    vb, vt = pz.contorno[:, 1].min() + 0.36, pz.contorno[:, 1].max() - 0.34
    hl = (vt - vb - 0.22) / 2
    caixas = []
    for (a, b) in ((vt - hl, vt), (vb, vb + hl)):
        esq, dir_ = -1e9, 1e9
        for v in np.linspace(a, b, 12):
            pp = cortes_horizontais(pz.contorno, v)
            cc = cortes_horizontais(cone_aqui, v)
            e, d = pp[0], pp[-1]
            if len(cc):
                if cone_a_direita:
                    d = min(d, cc[0])
                else:
                    e = max(e, cc[-1])
            esq, dir_ = max(esq, e), min(dir_, d)
        caixas.append((esq + 0.22, dir_ - 0.22, a, b))
    linhas = [letras_do_texto('COZY', 'racing_sans_one.ttf'), letras_do_texto('CONE', 'racing_sans_one.ttf')]
    escalas = [encaixar(L, c, esticar=(0.8, 1.35))[1] for L, c in zip(linhas, caixas)]
    escala = (min(e[0] for e in escalas), min(e[1] for e in escalas))
    for li, (L, c) in enumerate(zip(linhas, caixas)):
        letras, _ = encaixar(L, c, escala=escala, alinhar=0.35 if cone_a_direita else 0.65)
        for i, letra in enumerate(letras):
            a = 0.14 + li * 0.18 + 0.14 * i / (len(letras) - 1)
            fora, dentro = contornos_duplos(letra, 0.065, TOL)
            for q in fora:
                tubo(M_COZY, pz.mundo(q), 0.04, AMARELO_COZY, a, pz.n, lados=8)
            for q in dentro:
                tubo(M_COZY, pz.mundo(q), 0.03, AMARELO_COZY, a, pz.n)

# a luz da placa: na frente dela, do lado da rua
LUZES_COZY.append(((692.0, 6.5, 137.5), (1.0, 0.72, 0.40), 22.0, 1.2))

# as luminárias da praça: os cones dos cantos das bases dos quartos, acesos
QUARTOS = Tris('ror_Object_61')
idx = QUARTOS.escolher(caixa=(615, 790, 1.5, 3.2, 140, 295))
onda_max = 150.0


def onda(x, z):
    """A ordem da onda que sai da placa e corre pela praça"""
    return float(np.clip(0.05 + 0.90 * math.hypot(x - PE_DA_PLACA[0], z - PE_DA_PLACA[2]) / onda_max, 0, 1))


n_cantos = 0
for g in ilhas(QUARTOS, idx):
    V = QUARTOS.p[g].reshape(-1, 3)
    ext = V.max(0) - V.min(0)
    if not (0.6 < ext[0] < 1.4 and 0.6 < ext[2] < 1.4 and 0.9 < ext[1] < 1.4):
        continue
    eixo = np.array([(V[:, 0].min() + V[:, 0].max()) / 2, 0, (V[:, 2].min() + V[:, 2].max()) / 2])
    P = QUARTOS.p[g].copy()
    rad = P - eixo
    rad[..., 1] = 0
    P = P + normalizar(rad) * 0.025 + np.array([0, 0.01, 0])
    y0, y1 = V[:, 1].min(), V[:, 1].max()
    Vc = P.reshape(-1, 3)
    h = np.clip((Vc[:, 1] - y0) / (y1 - y0), 0, 1) ** 0.8
    cor = np.array([1.0, 0.74, 0.34]) * (1 - h[:, None]) + np.array([0.98, 0.34, 0.05]) * h[:, None]
    C = np.c_[cor, np.full(len(Vc), onda(eixo[0], eixo[2]))]
    F = orientar_para_fora(Vc, np.arange(len(Vc)).reshape(-1, 3), np.c_[np.full(len(Vc) // 3, eixo[0]),
                                                                      P.mean(1)[:, 1] - 0.3, np.full(len(Vc) // 3, eixo[2])])
    M_COZY.juntar(Vc, F, C)
    n_cantos += 1
print('  cones de luz dos cantos das bases: %d' % n_cantos)

# a lâmpada em cima de cada porta (a cúpula pequena acima do toldo, aberta para baixo)
idx = QUARTOS.escolher(caixa=(615, 790, 6.85, 7.35, 140, 295))
n_portas = 0
centro_praca = np.array([698.6, 0.0, 215.0])
for g in ilhas(QUARTOS, idx):
    V = QUARTOS.p[g].reshape(-1, 3)
    ext = V.max(0) - V.min(0)
    if ext[0] > 0.7 or ext[2] > 0.7 or V[:, 1].min() < 6.85:
        continue
    c = (V.min(0) + V.max(0)) / 2
    r = max(ext[0], ext[2]) / 2
    esfera(M_COZY, (c[0], V[:, 1].min() - 0.01, c[2]), (r * 0.9, 0.06, r * 0.9), LAMPADA, onda(c[0], c[2]) * 0.6)
    para_fora = normalizar(np.array([centro_praca[0] - c[0], 0, centro_praca[2] - c[2]]))
    LUZES_COZY.append(((c[0] + para_fora[0] * 1.6, 5.0, c[2] + para_fora[2] * 1.6), (1.0, 0.74, 0.42), 10.0, 1.1))
    n_portas += 1
print('  lâmpadas das portas: %d' % n_portas)


def cone_luminaria(M, base, alfa, altura=0.40, raio=0.15, lados=10):
    """Um cone pequeno de verdade: o miolo aceso (claro embaixo, quase branco) e a ponta laranja"""
    b = np.asarray(base, float)
    ang = np.arange(lados) * 2 * math.pi / lados
    anel = np.stack([np.cos(ang), np.zeros(lados), np.sin(ang)], 1)
    V = np.vstack([b + anel * raio, b + anel * raio * 0.62 + [0, altura * 0.42, 0], b + [0, altura, 0], b])
    cores = [(1.0, 0.72, 0.32)] * lados + [(1.0, 0.54, 0.15)] * lados + [(0.96, 0.32, 0.05), (1.0, 0.72, 0.32)]
    F = []
    for k in range(lados):
        k1 = (k + 1) % lados
        F += [(k, k1, lados + k1), (k, lados + k1, lados + k), (lados + k, lados + k1, 2 * lados), (2 * lados + 1, k1, k)]
    F = orientar_para_fora(V, F, b + [0, altura * 0.3, 0])
    M.juntar(V, F, np.c_[np.array(cores), np.full(len(V), alfa)])


# e os cones-luminária pequenos em volta dos canteiros (a grama da praça e a ilha da placa)
GRAMA = Tris('ror_Object_64')
idx = GRAMA.escolher(normal=(0, 1, 0), cos_min=0.95, caixa=(615, 790, 1.5, 2.6, 135, 295))
n_lum = 0
ESPACO = 4.5
for g in ilhas(GRAMA, idx):
    if GRAMA.area[g].sum() < 300:
        continue
    lacos = contornos_dos_triangulos(GRAMA.p[g])
    if not lacos:
        continue
    L = lacos[0]
    y = L[:, 1].mean()
    q = recuar(L[:, [0, 2]], 0.6)
    q = np.vstack([q, q[:1]])
    d = np.linalg.norm(np.diff(q, axis=0), axis=1)
    s = np.r_[0, np.cumsum(d)]
    n = max(3, int(round(s[-1] / ESPACO)))
    for t in np.arange(n) * s[-1] / n:
        x = np.interp(t, s, q[:, 0])
        z = np.interp(t, s, q[:, 1])
        cone_luminaria(M_COZY, (x, y, z), onda(x, z))
        n_lum += 1
# os quatro postes dos canteiros: uma lâmpada em cada uma das duas cúpulas da cabeça (abertas para baixo)
POSTES = Tris('ror_Object_9')
idx = POSTES.escolher(caixa=(640, 760, 7.4, 8.7, 195, 255))
n_cupulas = 0
for g in ilhas(POSTES, idx, com_uv=True):
    V = POSTES.p[g].reshape(-1, 3)
    ext = V.max(0) - V.min(0)
    if len(g) < 30 or ext.max() > 2.0:
        continue
    nc = normalizar((POSTES.n[g] * POSTES.area[g][:, None]).sum(0))
    if nc[1] > -0.4:
        continue
    c = V.mean(0) + nc * 0.12
    e1 = normalizar(np.cross([0, 1, 0], nc)) if abs(nc[1]) < 0.99 else np.array([1.0, 0, 0])
    e2 = np.cross(nc, e1)
    esfera(M_COZY, c, (0.34, 0.34, 0.07), LAMPADA, onda(c[0], c[2]) * 0.8, eixos=np.array([e1, e2, nc]), seg=10, aneis=4)
    n_cupulas += 1
print('  lâmpadas nos postes dos canteiros: %d' % n_cupulas)
LUZES_COZY.append(((698.6, 6.0, 223.0), (1.0, 0.60, 0.26), 26.0, 0.8))
print('  cones-luminária nos canteiros: %d' % n_lum)
print('  Cozy Cone: %d vértices, %d triângulos' % M_COZY.contagem())


# ------------------------------------------------------------------
# 9. Exportar e gravar os JSON
# ------------------------------------------------------------------
parte('Exportando')
exportar('sarge', [M_SARGE])
gravar_json('sarge', "Sarge's Surplus Hut", 2.0, LUZES_SARGE)
exportar('fillmore', [M_FILL_TEX, M_FILL])
gravar_json('fillmore', 'Fillmore: o domo sob luz negra', 3.0, LUZES_FILLMORE)
exportar('cozy_cone', [M_COZY])
gravar_json('cozy_cone', 'Cozy Cone Motel: a placa e a praça', 1.0, LUZES_COZY)
for k, v in CONTAGENS.items():
    print('  %-10s %s' % (k, v))


# ------------------------------------------------------------------
# 10. As prévias (com --previas): de noite e de dia, de quem dirige
# ------------------------------------------------------------------
VISTAS = [
    # nome, câmera (x, y, z do jogo), alvo, lente (mm)
    ('sarge_rua_oeste', (689.0, 2.6, 134.0), (701.8, 9.0, 117.5), 24),
    ('sarge_perto', (693.0, 3.2, 124.0), (701.8, 9.4, 117.6), 26),
    ('sarge_rua_leste', (716.0, 2.6, 134.0), (702.0, 9.0, 117.5), 24),
    ('fillmore_rua', (757.0, 2.6, 130.0), (758.5, 5.0, 99.0), 24),
    ('fillmore_entrada', (737.0, 2.6, 118.0), (755.0, 4.2, 100.0), 24),
    ('cozy_placa', (679.0, 2.6, 127.0), (697.5, 15.0, 145.0), 22),
    ('cozy_placa_leste', (717.0, 2.6, 127.0), (697.5, 15.0, 145.0), 22),
    ('cozy_placa_perto', (684.0, 12.0, 138.0), (697.5, 17.5, 145.5), 30),
    ('cozy_placa_perto_leste', (711.0, 12.0, 138.0), (697.5, 17.5, 145.5), 30),
    ('cozy_praca', (700.0, 2.8, 262.0), (700.0, 3.0, 205.0), 22),
    ('cozy_quarto', (656.0, 2.6, 176.0), (633.0, 5.0, 156.0), 24),
]
NOITE_CEU = (0.010, 0.014, 0.045)
NOITE_EMISSAO = 1.8


def no(nt, tipo, **kw):
    n = nt.nodes.new(tipo)
    for k, v in kw.items():
        n.inputs[k].default_value = v
    return n


def preparar_materiais_do_mapa():
    """Guarda, de cada material do mapa, a textura e se usa o alfa dela"""
    feitos = {}
    for o in OBJETOS_DO_MAPA:
        for s in o.material_slots:
            m = s.material
            if m is None or m.name in feitos or m.node_tree is None:
                continue
            nt = m.node_tree
            tex = next((n for n in nt.nodes if n.type == 'TEX_IMAGE' and n.image), None)
            bsdf = next((n for n in nt.nodes if n.type == 'BSDF_PRINCIPLED'), None)
            alfa = bool(bsdf and bsdf.inputs['Alpha'].is_linked)
            base = tuple(bsdf.inputs['Base Color'].default_value) if bsdf else (0.6, 0.6, 0.6, 1)
            out = next((n for n in nt.nodes if n.type == 'OUTPUT_MATERIAL'), None)
            dif = nt.nodes.new('ShaderNodeBsdfDiffuse')
            em = nt.nodes.new('ShaderNodeEmission')
            if tex:
                nt.links.new(tex.outputs['Color'], dif.inputs['Color'])
                nt.links.new(tex.outputs['Color'], em.inputs['Color'])
            else:
                dif.inputs['Color'].default_value = base
                em.inputs['Color'].default_value = base
            sh_noite, sh_dia = dif.outputs[0], em.outputs[0]
            if alfa and tex:
                tr = nt.nodes.new('ShaderNodeBsdfTransparent')
                saidas = []
                for sh in (sh_noite, sh_dia):
                    mx = nt.nodes.new('ShaderNodeMixShader')
                    nt.links.new(tex.outputs['Alpha'], mx.inputs['Fac'])
                    nt.links.new(tr.outputs[0], mx.inputs[1])
                    nt.links.new(sh, mx.inputs[2])
                    saidas.append(mx.outputs[0])
                sh_noite, sh_dia = saidas
            feitos[m.name] = (m, out, sh_noite, sh_dia)
    return feitos


def material_das_luzes(o, modo):
    """Os tubos: de noite, emissivos (cor × textura); de dia, vidro apagado (a cor × 0,3)"""
    me = o.data
    attr = me.color_attributes[0].name if me.color_attributes else 'Col'
    for s in o.material_slots:
        m = s.material
        nt = m.node_tree
        tex = next((n for n in nt.nodes if n.type == 'TEX_IMAGE' and n.image), None)
        for n in [n for n in nt.nodes if n.name.startswith('previa_')]:
            nt.nodes.remove(n)
        out = next(n for n in nt.nodes if n.type == 'OUTPUT_MATERIAL')
        at = nt.nodes.new('ShaderNodeAttribute')
        at.name = 'previa_attr'
        at.attribute_name = attr
        cor = at.outputs['Color']
        if tex is not None:
            mul = nt.nodes.new('ShaderNodeMix')
            mul.name = 'previa_mul'
            mul.data_type = 'RGBA'
            mul.blend_type = 'MULTIPLY'
            mul.inputs['Factor'].default_value = 1.0
            nt.links.new(at.outputs['Color'], mul.inputs[6])
            nt.links.new(tex.outputs['Color'], mul.inputs[7])
            cor = mul.outputs[2]
        em = nt.nodes.new('ShaderNodeEmission')
        em.name = 'previa_em'
        nt.links.new(cor, em.inputs['Color'])
        em.inputs['Strength'].default_value = NOITE_EMISSAO if modo == 'noite' else 0.3
        nt.links.new(em.outputs[0], out.inputs['Surface'])


def desfocar(img, r):
    """Três passadas de caixa (quase gaussiana) de raio r, nas duas direções"""
    r = max(1, int(r))
    for eixo in (0, 1):
        for _ in range(3):
            pad = [(0, 0)] * img.ndim
            pad[eixo] = (r + 1, r)
            c = np.cumsum(np.pad(img, pad, mode='edge'), axis=eixo)
            n = img.shape[eixo]
            img = (np.take(c, np.arange(2 * r + 1, 2 * r + 1 + n), axis=eixo) -
                   np.take(c, np.arange(0, n), axis=eixo)) / (2 * r + 1)
    return img


def gravar_com_halo(exr, png, halo):
    im = bpy.data.images.load(exr)
    w, h = im.size
    px = np.array(im.pixels[:], np.float32).reshape(h, w, 4)[..., :3]
    bpy.data.images.remove(im)
    if halo:
        lum = px.max(-1)
        claro = px * np.clip((lum - 0.9) / 0.9, 0, 1)[..., None]
        esc = w / 1280.0
        b = 0.30 * desfocar(claro, 3 * esc) + 0.30 * desfocar(claro, 11 * esc) + 0.25 * desfocar(claro, 30 * esc)
        px = px + b
    px = np.clip(px, 0, None)
    px = px / (1 + 0.08 * px)
    srgb = np.where(px <= 0.0031308, 12.92 * px, 1.055 * np.power(np.maximum(px, 1e-9), 1 / 2.4) - 0.055)
    rgba = np.concatenate([np.clip(srgb, 0, 1), np.ones((h, w, 1), np.float32)], 2).astype(np.float32)
    nova = bpy.data.images.new('previa', w, h, alpha=False)
    nova.pixels.foreach_set(rgba.ravel())
    nova.filepath_raw = png
    nova.file_format = 'PNG'
    nova.save()
    bpy.data.images.remove(nova)


def previas():
    sc = bpy.context.scene
    os.makedirs(PREVIAS, exist_ok=True)
    mats = preparar_materiais_do_mapa()
    # as luzes, lidas de volta dos .glb gravados (o que o jogo vai ler)
    antes = set(bpy.data.objects)
    luzes_json = []
    for lugar in ('sarge', 'fillmore', 'cozy_cone'):
        bpy.ops.import_scene.gltf(filepath=os.path.join(PASTA_DE_SAIDA, lugar + '.glb'))
        luzes_json += json.load(open(os.path.join(PASTA_DE_SAIDA, lugar + '.json')))['luzes']
    nossos = [o for o in bpy.data.objects if o not in antes and o.type == 'MESH']
    pontos = []
    for k, lz in enumerate(luzes_json):
        ld = bpy.data.lights.new('luz_%d' % k, 'POINT')
        lin = [c ** 2.2 for c in (lz['r'], lz['g'], lz['b'])]
        ld.color = lin
        ld.energy = 2.5 * lz['forca'] * lz['raio'] ** 2
        ld.shadow_soft_size = 0.3
        try:
            ld.use_custom_distance = True
            ld.cutoff_distance = lz['raio'] * 1.5
        except Exception:
            pass
        lo = bpy.data.objects.new('luz_%d' % k, ld)
        lo.location = (lz['x'], -lz['z'], lz['y'])
        sc.collection.objects.link(lo)
        pontos.append(lo)
    w = bpy.data.worlds.new('ceu')
    try:
        w.use_nodes = True
    except Exception:
        pass
    sc.world = w
    fundo = w.node_tree.nodes['Background']
    cam = bpy.data.cameras.new('cam')
    cam.clip_start = 0.3
    cam.clip_end = 6000
    co = bpy.data.objects.new('cam', cam)
    sc.collection.objects.link(co)
    sc.camera = co
    sc.render.engine = 'BLENDER_EEVEE'
    sc.eevee.taa_render_samples = 24
    sc.view_settings.view_transform = 'Standard'
    sc.render.resolution_x, sc.render.resolution_y = 1280, 720
    sc.render.resolution_percentage = 100
    sc.render.image_settings.file_format = 'OPEN_EXR'
    for modo in ('noite', 'dia'):
        for m, out, sh_noite, sh_dia in mats.values():
            m.node_tree.links.new(sh_noite if modo == 'noite' else sh_dia, out.inputs['Surface'])
        for o in nossos:
            o.hide_render = False
            material_das_luzes(o, modo)
        for lo in pontos:
            lo.hide_render = modo != 'noite'
        if modo == 'noite':
            fundo.inputs[0].default_value = (*NOITE_CEU, 1)
            fundo.inputs[1].default_value = 1.0
        else:
            fundo.inputs[0].default_value = (0.45, 0.62, 0.9, 1)
            fundo.inputs[1].default_value = 1.0
        for nome, cpos, alvo, lente in VISTAS:
            if SO_AS_VISTAS and nome not in SO_AS_VISTAS:
                continue
            cam.lens = lente
            co.location = Vector((cpos[0], -cpos[2], cpos[1]))
            a = Vector((alvo[0], -alvo[2], alvo[1]))
            co.rotation_euler = (a - co.location).to_track_quat('-Z', 'Y').to_euler()
            exr = os.path.join(PASTA_TEMP, 'previa.exr')
            sc.render.filepath = exr
            bpy.ops.render.render(write_still=True)
            gravar_com_halo(exr, os.path.join(PREVIAS, '%s_%s.png' % (nome, modo)), modo == 'noite')
            print('  prévia %s_%s' % (nome, modo))


if PREVIAS:
    parte('Prévias em %s' % PREVIAS)
    previas()

shutil.rmtree(PASTA_TEMP, ignore_errors=True)
parte('Pronto')
