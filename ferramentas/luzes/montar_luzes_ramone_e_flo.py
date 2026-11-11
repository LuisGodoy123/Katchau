"""
montar_luzes_ramone_e_flo.py
------------------------------------------------------------------
Monta os neons e as luzes de noite de dois lugares de Radiator Springs (do
filme Carros, 2006), a Ramone's House of Body Art e o Flo's V8 Café, por cima
do mapa do mundo aberto (recursos/modelos/radiator_springs.glb), já nas
coordenadas do mundo do jogo (x, y para cima, z):

    Blender -b --factory-startup -P ferramentas/luzes/montar_luzes_ramone_e_flo.py -- \\
        recursos/modelos/radiator_springs.glb recursos/modelos/luzes

Grava, na pasta dada no segundo argumento:
  ramone.glb, flo.glb    só o que brilha de noite (os tubos de neon, as
                         lâmpadas fluorescentes e o vidro aceso das
                         vitrines), sem textura, com um material
                         branco; a cor de cada vértice (COLOR_0) é a cor ACESA
                         (sRGB) e o alfa é a ordem de acender (0 acende
                         primeiro, 1 por último): as letras acendem uma depois
                         da outra e as partes do letreiro em etapas. Cada malha
                         tem no máximo 60 mil vértices (a raylib usa índices de
                         16 bits);
  ramone.json, flo.json  o nome, quando o lugar começa a acender (segundos
                         depois de a cidade começar a acender) e as luzes
                         pontuais que os letreiros jogam no chão e nas paredes
                         em volta (eixos do jogo; lugar, cor, raio e força).

O mapa (do Race-O-Rama de PS3) só tem os letreiros pintados em texturas
pequenas; a ferramenta mede neles onde fica cada letreiro (pelos triângulos que
usam a parte da textura com o desenho dele: a posição, a orientação, o tamanho
e a curva das fachadas) e põe os tubos por cima, afastados uns centímetros da
superfície.

Como se desenha um neon de letra:
  1. o texto sai do Blender (fonte → malha cheia), cada prefixo do texto à
     parte, para saber a que letra pertence cada pedaço (as letras de mão
     emendam umas nas outras);
  2. a forma é rasterizada numa grade fina (1 cm ou menos), e a distância com
     sinal até a borda dá o contorno da letra (nível 0) e uma segunda linha por
     dentro (a "dupla linha" dos neons do filme), que some sozinha onde o traço
     é fino demais para ela;
  3. os contornos são simplificados (Douglas-Peucker), os cantos viram curvas
     (um tubo de vidro não dobra em quina) e o 2D passa para o 3D pela
     superfície do letreiro: um plano, ou o UV do próprio letreiro pintado no
     mapa (o ponto cai no triângulo que tem aquele pedaço da textura, e assim
     o tubo acompanha a curva das fachadas);
  4. cada linha vira um tubo redondo de 6 a 8 lados, de 2,5 a 5 cm de raio.

O que acende:
  Ramone   a torre: "RAMONES" nas quatro faces (contorno lilás e linha de
           dentro azulada), os cantos e as bordas da torre, a coluna de cima e
           a bola em verde-água; os anéis verde-água da cúpula e do tambor de
           cima; "House of Body Art" na fachada curva, em letra de mão,
           branco-lilás com a linha de dentro azul, onde o mapa pintou cada
           palavra.
  Flo      os anéis ciano em volta do prédio redondo (dois no disco de cima,
           dois no do meio e um em cima das vitrines); "Flo's" e "Cafe" no
           telhado em ciano com a linha de dentro rosa; o V rosa com a linha do
           meio ciano e o 8 rosa com o ciano por dentro e o emblema "(V)";
           LUGNUTS, GAS, OIL, GREASE e COOLANT em rosa, cada palavra no painel
           da faixa de cima das vitrines em que o mapa a pintou; as vitrines
           quentes (a porta da frente e as janelas); nas marquises das bombas, as bordas
           ciano, as calhas de lâmpadas fluorescentes embaixo, os aros rosa das
           saias das colunas, e em cima as molas de neon (vermelho, laranja,
           amarelo) com as estrelas amarelas; e o letreiro do poste na beira da
           estrada (o oval, as letras, o V e o 8), dos dois lados.

Ordem de acender (o alfa): Ramone: os anéis da cúpula de baixo para cima
(0-0,09), os cantos e a coluna de cima (0,12-0,19), R-A-M-O-N-E-S de cima para
baixo (0,22-0,59), "House" (0,64-0,68), "of" (0,71-0,72) e "Body Art" letra a
letra (0,76-0,98). Flo: as vitrines (0), as fluorescentes piscando a esmo
(0,02-0,10), os anéis do prédio de baixo para cima (0,12-0,24), as bordas das
marquises (0,27), as saias (0,30), as palavras da faixa (0,34-0,46), F-l-o-'-s
(0,50-0,61), o V (0,64), o 8 e o emblema (0,68-0,70), C-a-f-e (0,74-0,84), e as
molas com as estrelas, da rua para o prédio (0,84-0,99); o letreiro do poste vai
junto com os do prédio (o oval em 0,50).

Eixos: no Blender, x = x do jogo, y = -z do jogo e z = y do jogo; o .glb sai
com +Y para cima, ou seja, nas coordenadas do jogo.
------------------------------------------------------------------
"""

import bpy
import json
import math
import os
import struct
import sys
import time
from collections import defaultdict
import numpy as np

INICIO = time.time()
bpy.ops.wm.read_factory_settings(use_empty=True)     # sem o cubo, a câmera e a luz da cena inicial
AQUI = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(AQUI))
PASTA_DAS_FONTES = os.path.join(REPO, 'recursos', 'fontes')

# ------------------------------------------------------------------
# Argumentos
# ------------------------------------------------------------------
argv = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
ARQUIVO_DO_MAPA = argv[0] if len(argv) > 0 else os.path.join(REPO, 'recursos', 'modelos', 'radiator_springs.glb')
PASTA_DE_SAIDA = argv[1] if len(argv) > 1 else os.path.join(REPO, 'recursos', 'modelos', 'luzes')
os.makedirs(PASTA_DE_SAIDA, exist_ok=True)

# ------------------------------------------------------------------
# As cores acesas (sRGB, saturadas como o neon aceso no filme)
# ------------------------------------------------------------------
LILAS_BRANCO = (0.90, 0.72, 1.00)     # o contorno de "House of Body Art"
LILAS = (0.80, 0.52, 1.00)            # o contorno de RAMONES
AZUL_LILAS = (0.54, 0.60, 1.00)       # a linha de dentro das letras da Ramone
VERDE_AGUA = (0.22, 1.00, 0.78)       # a torre e a cúpula da Ramone
CIANO = (0.18, 0.95, 1.00)            # o Flo's
ROSA = (1.00, 0.16, 0.72)             # o V8 e as linhas de dentro do Flo's
ROSA_FAIXA = (1.00, 0.22, 0.62)       # LUGNUTS GAS OIL / COOLANT GREASE
BRANCO_FRIO = (0.86, 1.00, 1.00)      # as lâmpadas fluorescentes das marquises
AMARELO = (1.00, 0.90, 0.22)
LARANJA = (1.00, 0.52, 0.10)
VERMELHO = (1.00, 0.20, 0.12)
QUENTE = (1.00, 0.72, 0.38)           # as vitrines acesas (em cima; embaixo, mais âmbar e mais escuras)
QUENTE_BAIXO = (0.62, 0.34, 0.12)

FONTE_DE_MAO = 'lobster.ttf'                    # Lobster (OFL): "Flo's", "Cafe", "House of Body Art"
FONTE_DE_CAIXA = 'saira_condensed_bold.ttf'     # Saira Condensed (OFL): as letras de forma


# ------------------------------------------------------------------
# 1. O mapa: os triângulos de cada objeto, nos eixos do jogo, com o UV em pixels da textura
# ------------------------------------------------------------------
def b_de(p):
    """Do jogo (x, y, z) para o Blender (x, -z, y), em lote"""
    p = np.asarray(p, float)
    return np.stack([p[..., 0], -p[..., 2], p[..., 1]], -1)


class Tris:
    """Triângulos do mapa: G (n, 3, 3) nos eixos do jogo e U (n, 3, 2) em pixels da textura
    (u para a direita, v de cima para baixo, como na imagem)"""

    def __init__(self, G, U):
        self.G = np.asarray(G, float).reshape(-1, 3, 3)
        self.U = np.asarray(U, float).reshape(-1, 3, 2)
        self.C = self.G.mean(1)
        self.UC = self.U.mean(1)
        n = np.cross(self.G[:, 1] - self.G[:, 0], self.G[:, 2] - self.G[:, 0])
        self.area = np.linalg.norm(n, axis=1) / 2
        self.N = n / np.maximum(2 * self.area, 1e-12)[:, None]

    def __len__(self):
        return len(self.G)

    def __add__(self, outro):
        return Tris(np.concatenate([self.G, outro.G]), np.concatenate([self.U, outro.U]))

    def sub(self, sel):
        return Tris(self.G[sel], self.U[sel])

    def filtro(self, uv=None, uv_todos=False, caixa=None, normal=None, minimo=0.8, area_min=1e-4):
        """uv = (u0, u1, v0, v1) em pixels (o centro do triângulo, ou os três cantos com uv_todos);
        caixa = (x0, x1, y0, y1, z0, z1) para o centro; normal = direção (a normal precisa ter
        produto escalar >= minimo com ela)"""
        sel = self.area > area_min
        if uv is not None:
            u0, u1, v0, v1 = uv
            if uv_todos:
                U = self.U
                sel &= ((U[..., 0] >= u0) & (U[..., 0] <= u1) & (U[..., 1] >= v0) & (U[..., 1] <= v1)).all(1)
            else:
                c = self.UC
                sel &= (c[:, 0] >= u0) & (c[:, 0] <= u1) & (c[:, 1] >= v0) & (c[:, 1] <= v1)
        if caixa is not None:
            x0, x1, y0, y1, z0, z1 = caixa
            c = self.C
            sel &= (c[:, 0] >= x0) & (c[:, 0] <= x1) & (c[:, 1] >= y0) & (c[:, 1] <= y1) & (c[:, 2] >= z0) & (c[:, 2] <= z1)
        if normal is not None:
            d = np.asarray(normal, float)
            d = d / np.linalg.norm(d)
            sel &= self.N @ d >= minimo
        return self.sub(sel)

    def vertices(self):
        return np.unique(np.round(self.G.reshape(-1, 3), 4), axis=0)


def triangulos_do_mapa(nome):
    ob = bpy.data.objects.get(nome)
    if ob is None or ob.type != 'MESH':
        raise SystemExit('ERRO: o mapa não tem o objeto %s' % nome)
    me = ob.data
    me.calc_loop_triangles()
    n = len(me.loop_triangles)
    mw = np.array(ob.matrix_world)
    li = np.empty(n * 3, np.int32)
    me.loop_triangles.foreach_get('loops', li)
    vi = np.empty(n * 3, np.int32)
    me.loop_triangles.foreach_get('vertices', vi)
    co = np.empty(len(me.vertices) * 3)
    me.vertices.foreach_get('co', co)
    co = co.reshape(-1, 3) @ mw[:3, :3].T + mw[:3, 3]
    P = co[vi].reshape(n, 3, 3)
    G = np.stack([P[..., 0], P[..., 2], -P[..., 1]], -1)
    uv = np.empty(len(me.loops) * 2)
    me.uv_layers.active.data.foreach_get('uv', uv)
    uv = uv.reshape(-1, 2)[li].reshape(n, 3, 2)
    imagem = None
    for m in me.materials:
        if m and m.node_tree:
            for nd in m.node_tree.nodes:
                if nd.type == 'TEX_IMAGE' and nd.image:
                    imagem = nd.image
    W, H = imagem.size if imagem else (1, 1)
    U = np.stack([uv[..., 0] * W, (1 - uv[..., 1]) * H], -1)
    return Tris(G, U)


print('Lendo o mapa %s' % ARQUIVO_DO_MAPA)
bpy.ops.import_scene.gltf(filepath=ARQUIVO_DO_MAPA)
T45 = triangulos_do_mapa('ror_Object_45')      # a Ramone (e os vizinhos de bloco)
T71 = triangulos_do_mapa('ror_Object_71')      # o Flo's: discos, marquises, o letreiro do poste
T72 = triangulos_do_mapa('ror_Object_72')      # o Flo's: o resto (V8, placas, vitrines, estrelas, saias)
TF = T72 + T71
# O mapa só serve para medir: sai da cena antes de montar as luzes
for ob in list(bpy.data.objects):
    bpy.data.objects.remove(ob, do_unlink=True)


# ------------------------------------------------------------------
# 2. Geometria 2D: rasterizar, distância com sinal, contornos
# ------------------------------------------------------------------
def rasterizar(tris, x0, y0, px, W, H):
    """Pinta os triângulos (n, 3, 2), em metros, numa grade H x W (a linha i é y0 + (i + 0,5) px)"""
    m = np.zeros((H, W), bool)
    for t in np.asarray(tris, float):
        a, b, c = (t - (x0, y0)) / px - 0.5
        xs = (a[0], b[0], c[0])
        ys = (a[1], b[1], c[1])
        j0, j1 = max(int(math.floor(min(xs))), 0), min(int(math.ceil(max(xs))), W - 1)
        i0, i1 = max(int(math.floor(min(ys))), 0), min(int(math.ceil(max(ys))), H - 1)
        if j0 > j1 or i0 > i1:
            continue
        d = (b[1] - c[1]) * (a[0] - c[0]) + (c[0] - b[0]) * (a[1] - c[1])
        if abs(d) < 1e-12:
            continue
        X, Y = np.meshgrid(np.arange(j0, j1 + 1), np.arange(i0, i1 + 1))
        l1 = ((b[1] - c[1]) * (X - c[0]) + (c[0] - b[0]) * (Y - c[1])) / d
        l2 = ((c[1] - a[1]) * (X - c[0]) + (a[0] - c[0]) * (Y - c[1])) / d
        e = -1e-7
        m[i0:i1 + 1, j0:j1 + 1] |= (l1 >= e) & (l2 >= e) & (1 - l1 - l2 >= e)
    return m


def distancia_ate(alvo):
    """A distância euclidiana (em pixels) de cada pixel até o pixel True mais perto (exata: uma passada
    pelas linhas e a mínima das parábolas pelas colunas)"""
    H, W = alvo.shape
    if not alvo.any():
        return np.full((H, W), 1e6)
    if H > W:
        return distancia_ate(alvo.T).T
    idx = np.arange(W, dtype=float)
    esq = np.maximum.accumulate(np.where(alvo, idx, -1e7), axis=1)
    dir_ = np.minimum.accumulate(np.where(alvo, idx, 1e7)[:, ::-1], axis=1)[:, ::-1]
    g = np.minimum(np.minimum(idx - esq, dir_ - idx), 1e5) ** 2
    ys = np.arange(H, dtype=float)
    D = np.empty((H, W))
    for i in range(H):
        D[i] = (g + ((ys - i) ** 2)[:, None]).min(0)
    return np.sqrt(D)


def distancia_com_sinal(m):
    """Negativa dentro, positiva fora, zero na borda dos pixels (em pixels), um pouco alisada"""
    F = np.where(m, -(distancia_ate(~m) - 0.5), distancia_ate(m) - 0.5)
    for _ in range(2):
        P = np.pad(F, 1, mode='edge')
        F = 0.25 * P[:-2, 1:-1] + 0.5 * P[1:-1, 1:-1] + 0.25 * P[2:, 1:-1]
        P = np.pad(F, 1, mode='edge')
        F = 0.25 * P[1:-1, :-2] + 0.5 * P[1:-1, 1:-1] + 0.25 * P[1:-1, 2:]
    return F


# Marching squares: os cantos da célula (i, j) são tl = (i, j), tr = (i, j+1), br = (i+1, j+1), bl = (i+1, j);
# as arestas: T = h(i, j), R = v(i, j+1), B = h(i+1, j), L = v(i, j)
_CASOS = {1: [('L', 'B')], 2: [('B', 'R')], 3: [('L', 'R')], 4: [('T', 'R')], 6: [('T', 'B')], 7: [('L', 'T')],
          8: [('L', 'T')], 9: [('T', 'B')], 11: [('T', 'R')], 12: [('L', 'R')], 13: [('B', 'R')], 14: [('L', 'B')]}


def isolinhas(F, nivel):
    """Os laços fechados onde F = nivel (F com borda toda do mesmo lado), em coordenadas da grade (coluna, linha)"""
    A = F - nivel
    A = np.where(np.abs(A) < 1e-9, 1e-9, A)
    H, W = A.shape
    p = A < 0
    caso = p[:-1, :-1] * 8 + p[:-1, 1:] * 4 + p[1:, 1:] * 2 + p[1:, :-1] * 1
    centro = (A[:-1, :-1] + A[:-1, 1:] + A[1:, 1:] + A[1:, :-1]) < 0

    def aresta(nome, I, J):
        if nome == 'T':
            return I * W + J
        if nome == 'B':
            return (I + 1) * W + J
        if nome == 'L':
            return H * W + I * W + J
        return H * W + I * W + J + 1

    pares = []
    for k, segs in _CASOS.items():
        I, J = np.nonzero(caso == k)
        for a, b in segs:
            pares.append(np.stack([aresta(a, I, J), aresta(b, I, J)], 1))
    for k in (5, 10):
        I, J = np.nonzero(caso == k)
        c = centro[I, J]
        if k == 5:
            segs_c, segs_s = [('L', 'T'), ('B', 'R')], [('L', 'B'), ('T', 'R')]
        else:
            segs_c, segs_s = [('T', 'R'), ('L', 'B')], [('L', 'T'), ('B', 'R')]
        for segs, sel in ((segs_c, c), (segs_s, ~c)):
            for a, b in segs:
                pares.append(np.stack([aresta(a, I[sel], J[sel]), aresta(b, I[sel], J[sel])], 1))
    if not pares:
        return []
    pares = np.concatenate(pares)
    if len(pares) == 0:
        return []
    # onde cada aresta corta o nível
    ids = np.unique(pares)
    pos = np.empty((len(ids), 2))
    h = ids < H * W
    ih, jh = np.divmod(ids[h], W)
    t = A[ih, jh] / (A[ih, jh] - A[ih, np.minimum(jh + 1, W - 1)])
    pos[h] = np.stack([jh + t, ih], 1)
    iv, jv = np.divmod(ids[~h] - H * W, W)
    t = A[iv, jv] / (A[iv, jv] - A[np.minimum(iv + 1, H - 1), jv])
    pos[~h] = np.stack([jv, iv + t], 1)
    onde = {int(e): k for k, e in enumerate(ids)}
    viz = defaultdict(list)
    for s, (a, b) in enumerate(pares):
        viz[int(a)].append(s)
        viz[int(b)].append(s)
    usado = np.zeros(len(pares), bool)
    lacos = []
    for s0 in range(len(pares)):
        if usado[s0]:
            continue
        usado[s0] = True
        a, b = int(pares[s0][0]), int(pares[s0][1])
        cadeia = [a, b]
        atual = b
        while True:
            prox = [s for s in viz[atual] if not usado[s]]
            if not prox:
                break
            s = prox[0]
            usado[s] = True
            x, y = int(pares[s][0]), int(pares[s][1])
            atual = y if x == atual else x
            cadeia.append(atual)
        if len(cadeia) > 3 and cadeia[-1] == cadeia[0]:
            cadeia = cadeia[:-1]
        lacos.append(pos[[onde[e] for e in cadeia]])
    return lacos


def douglas_peucker(P, tol):
    n = len(P)
    if n < 3:
        return P
    manter = np.zeros(n, bool)
    manter[0] = manter[-1] = True
    pilha = [(0, n - 1)]
    while pilha:
        i, j = pilha.pop()
        if j <= i + 1:
            continue
        a, b = P[i], P[j]
        ab = b - a
        L = np.linalg.norm(ab)
        Q = P[i + 1:j] - a
        if L < 1e-12:
            d = np.linalg.norm(Q, axis=1)
        else:
            d = np.abs(ab[0] * Q[:, 1] - ab[1] * Q[:, 0]) / L
        k = int(np.argmax(d))
        if d[k] > tol:
            m = i + 1 + k
            manter[m] = True
            pilha += [(i, m), (m, j)]
    return P[manter]


def simplificar(P, tol, fechado):
    if not fechado:
        return douglas_peucker(P, tol)
    k = int(np.argmax(np.linalg.norm(P - P[0], axis=1)))
    A = douglas_peucker(P[:k + 1], tol)
    B = douglas_peucker(np.vstack([P[k:], P[:1]]), tol)
    return np.vstack([A[:-1], B[:-1]])


def arredondar(P, raio, fechado, passo=math.radians(25), angulo_min=math.radians(24)):
    """Troca as quinas por curvas (Bézier quadrática que aproxima um arco de raio dado); serve em 2D e 3D"""
    P = np.asarray(P, float)
    n = len(P)
    saida = []
    for i in range(n):
        if not fechado and (i == 0 or i == n - 1):
            saida.append(P[i])
            continue
        a, b, c = P[i - 1], P[i], P[(i + 1) % n]
        u, v = b - a, c - b
        lu, lv = np.linalg.norm(u), np.linalg.norm(v)
        if lu < 1e-9 or lv < 1e-9:
            continue
        u, v = u / lu, v / lv
        ang = math.acos(float(np.clip(u @ v, -1, 1)))
        if ang < angulo_min:
            saida.append(b)
            continue
        t = min(raio * math.tan(min(ang, math.radians(170)) / 2), 0.45 * lu, 0.45 * lv)
        p0, p1 = b - u * t, b + v * t
        k = max(2, int(math.ceil(ang / passo)))
        for s in np.linspace(0, 1, k + 1):
            saida.append((1 - s) ** 2 * p0 + 2 * (1 - s) * s * b + s ** 2 * p1)
    return np.array(saida)


def subdividir(P, passo, fechado):
    """Põe pontos nos segmentos mais longos que o passo (para seguirem a curva de uma superfície)"""
    P = np.asarray(P, float)
    n = len(P)
    saida = []
    for i in range(n if fechado else n - 1):
        a, b = P[i], P[(i + 1) % n]
        k = max(1, int(math.ceil(np.linalg.norm(b - a) / passo)))
        for s in range(k):
            saida.append(a + (b - a) * s / k)
    if not fechado:
        saida.append(P[-1])
    return np.array(saida)


def area_e_perimetro(P):
    x, y = P[:, 0], P[:, 1]
    area = 0.5 * abs(np.dot(x, np.roll(y, -1)) - np.dot(y, np.roll(x, -1)))
    return area, np.linalg.norm(P - np.roll(P, -1, 0), axis=1).sum()


def contornos(formas, niveis, px=0.008, tol=0.004, raio_do_tubo=0.03, caixa=None):
    """formas: lista de (rótulo, triângulos (n, 3, 2) em metros) ou (rótulo, função(X, Y) -> bool); cada
    pixel fica com o rótulo da primeira forma que o cobre (o desenho é a união de todas). niveis: as
    distâncias (em metros) das linhas: 0 é a borda, negativo é para dentro. Devolve, para cada nível, a
    lista de (pontos (m, 2), rótulos (m,)) dos laços, já simplificados e com as quinas arredondadas"""
    if caixa is None:
        tudo = np.concatenate([np.asarray(f).reshape(-1, 2) for _, f in formas if not callable(f)])
        caixa = (tudo[:, 0].min(), tudo[:, 1].min(), tudo[:, 0].max(), tudo[:, 1].max())
    margem = 6 * px + max(0.0, max(niveis))
    x0, y0 = caixa[0] - margem, caixa[1] - margem
    W = int(math.ceil((caixa[2] - caixa[0] + 2 * margem) / px))
    H = int(math.ceil((caixa[3] - caixa[1] + 2 * margem) / px))
    rot = np.zeros((H, W), np.int32)
    X, Y = np.meshgrid(x0 + (np.arange(W) + 0.5) * px, y0 + (np.arange(H) + 0.5) * px)
    for r, f in formas:
        m = f(X, Y) if callable(f) else rasterizar(f, x0, y0, px, W, H)
        rot[(rot == 0) & m] = r + 1
    m = rot > 0
    # o rótulo se espalha uns pixels para fora (para os pontos que caem na borda)
    for _ in range(4):
        for di, dj in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            viz = np.roll(np.roll(rot, di, 0), dj, 1)
            rot = np.where(rot == 0, viz, rot)
    F = distancia_com_sinal(m) * px
    res = {}
    for nivel in niveis:
        lacos = []
        for L in isolinhas(F, nivel):
            P = np.stack([x0 + (L[:, 0] + 0.5) * px, y0 + (L[:, 1] + 0.5) * px], 1)
            if len(P) < 4:
                continue
            area, perim = area_e_perimetro(P)
            if perim < 8 * raio_do_tubo or 2 * area / perim < 1.2 * raio_do_tubo:
                continue     # pequeno ou fino demais: as duas voltas do tubo se encostariam
            P = simplificar(P, tol, True)
            if len(P) < 3:
                continue
            P = arredondar(P, 1.4 * raio_do_tubo, True)
            j = np.clip(((P[:, 0] - x0) / px).astype(int), 0, W - 1)
            i = np.clip(((P[:, 1] - y0) / px).astype(int), 0, H - 1)
            lacos.append((P, np.maximum(rot[i, j] - 1, 0)))
        res[nivel] = lacos
    return res


# ------------------------------------------------------------------
# 3. Texto: a malha cheia de cada prefixo, para os rótulos das letras
# ------------------------------------------------------------------
_FONTES = {}


def fonte(arquivo):
    if arquivo not in _FONTES:
        _FONTES[arquivo] = bpy.data.fonts.load(os.path.join(PASTA_DAS_FONTES, arquivo), check_existing=True)
    return _FONTES[arquivo]


def _tris_do_texto(texto, arquivo):
    cu = bpy.data.curves.new('texto', 'FONT')
    cu.body = texto
    cu.font = fonte(arquivo)
    cu.size = 1.0
    cu.resolution_u = 10
    ob = bpy.data.objects.new('texto', cu)
    bpy.context.scene.collection.objects.link(ob)
    dg = bpy.context.evaluated_depsgraph_get()
    me = bpy.data.meshes.new_from_object(ob.evaluated_get(dg))
    me.calc_loop_triangles()
    n = len(me.loop_triangles)
    vi = np.empty(n * 3, np.int32)
    me.loop_triangles.foreach_get('vertices', vi)
    co = np.empty(len(me.vertices) * 3)
    me.vertices.foreach_get('co', co)
    T = co.reshape(-1, 3)[vi].reshape(n, 3, 3)[:, :, :2].copy()
    bpy.data.objects.remove(ob, do_unlink=True)
    bpy.data.curves.remove(cu)
    bpy.data.meshes.remove(me)
    return T


def texto(texto_, arquivo):
    """[(índice da letra, triângulos do prefixo até ela)] em unidades da fonte (tamanho 1), sem os espaços"""
    return [(i, _tris_do_texto(texto_[:i + 1], arquivo)) for i, ch in enumerate(texto_) if ch != ' ']


def caixa_de(formas):
    tudo = np.concatenate([f.reshape(-1, 2) for _, f in formas])
    return tudo.min(0), tudo.max(0)


def transformar(formas, escala=1.0, escala_x=1.0, inclinar=0.0, girar=0.0, ancora=(0, 0), destino=(0, 0)):
    """Aplica ao desenho: inclinação (x += y * inclinar), escala (com escala_x a mais em x), giro em graus,
    levando o ponto 'ancora' (já escalado) ao 'destino'"""
    c, s = math.cos(math.radians(girar)), math.sin(math.radians(girar))
    saida = []
    for r, T in formas:
        P = T.copy()
        P[..., 0] = P[..., 0] + P[..., 1] * inclinar
        P[..., 0] *= escala * escala_x
        P[..., 1] *= escala
        P = P - np.asarray(ancora)
        P = np.stack([c * P[..., 0] - s * P[..., 1], s * P[..., 0] + c * P[..., 1]], -1) + np.asarray(destino)
        saida.append((r, P))
    return saida


def encaixar(formas, x0, y0, x1, y1, escala_x=1.0, inclinar=0.0, alinhar='centro'):
    """Escala o texto para caber na caixa (sem deformar, além da escala_x) e o centraliza nela"""
    f = transformar(formas, 1.0, escala_x, inclinar)
    a, b = caixa_de(f)
    esc = min((x1 - x0) / (b[0] - a[0]), (y1 - y0) / (b[1] - a[1]))
    f = transformar(formas, esc, escala_x, inclinar)
    a, b = caixa_de(f)
    dx = (x0 + x1) / 2 - (a[0] + b[0]) / 2 if alinhar == 'centro' else x0 - a[0]
    dy = (y0 + y1) / 2 - (a[1] + b[1]) / 2
    return [(r, T + (dx, dy)) for r, T in f], esc


# ------------------------------------------------------------------
# 4. As superfícies: do 2D do letreiro (metros) para o mundo
# ------------------------------------------------------------------
CIMA = np.array([0.0, 1.0, 0.0])


def normalizar(v):
    v = np.asarray(v, float)
    return v / np.maximum(np.linalg.norm(v, axis=-1, keepdims=True), 1e-12)


class Plano:
    """origem + x * ex + y * ey + afastamento * n"""

    def __init__(self, origem, ex, ey, n=None):
        self.o = np.asarray(origem, float)
        self.ex = normalizar(ex)
        self.ey = normalizar(ey)
        self.n = normalizar(np.cross(self.ex, self.ey) if n is None else n)

    def para_3d(self, P2, afast):
        P2 = np.asarray(P2, float)
        af = np.broadcast_to(np.asarray(afast, float), (len(P2),))
        P = self.o + P2[:, :1] * self.ex + P2[:, 1:2] * self.ey + af[:, None] * self.n
        return P, np.tile(self.n, (len(P2), 1))


class SuperficieUV:
    """Um letreiro pintado no mapa: o 2D (s para a direita de quem olha de fora, h para cima, em metros, a
    partir do canto de baixo à esquerda da região de UV) vai para o UV e daí para o triângulo que o contém"""

    def __init__(self, tris, nome):
        if len(tris) == 0:
            raise SystemExit('ERRO: não achei no mapa o letreiro %s' % nome)
        self.tris = tris
        G, U = tris.G, tris.U
        e1, e2 = G[:, 1] - G[:, 0], G[:, 2] - G[:, 0]
        f1, f2 = U[:, 1] - U[:, 0], U[:, 2] - U[:, 0]
        det = f1[:, 0] * f2[:, 1] - f1[:, 1] * f2[:, 0]
        ok = np.abs(det) > 1e-9
        dPdu = (e1 * f2[:, 1:2] - e2 * f1[:, 1:2]) / np.where(ok, det, 1)[:, None]
        dPdv = (e2 * f1[:, 0:1] - e1 * f2[:, 0:1]) / np.where(ok, det, 1)[:, None]
        pes = tris.area * ok
        n = normalizar((tris.N * pes[:, None]).sum(0))
        direita = normalizar(np.cross(CIMA, n)) if abs(n[1]) < 0.9 else normalizar(dPdu.mean(0))
        cima = normalizar(np.cross(n, direita))
        du = (dPdu * pes[:, None]).sum(0) / pes.sum()
        dv = (dPdv * pes[:, None]).sum(0) / pes.sum()
        # u ou v podem estar trocados (o desenho pode estar de lado na textura)
        if abs(du @ direita) < abs(dv @ direita):
            self.troca = True
            du, dv = dv, du
        else:
            self.troca = False
        self.su = np.linalg.norm(du)
        self.sv = np.linalg.norm(dv)
        self.sinal_u = 1.0 if du @ direita > 0 else -1.0
        self.sinal_v = 1.0 if dv @ cima > 0 else -1.0
        Ux = U[..., 1] if self.troca else U[..., 0]
        Vx = U[..., 0] if self.troca else U[..., 1]
        self.u0 = Ux.min() if self.sinal_u > 0 else Ux.max()
        self.v0 = Vx.min() if self.sinal_v > 0 else Vx.max()
        self.largura = (Ux.max() - Ux.min()) * self.su
        self.altura = (Vx.max() - Vx.min()) * self.sv
        self.n = n
        print('  %-28s %5.2f x %4.2f m, %d triângulos, normal %s' % (nome, self.largura, self.altura, len(tris), np.round(n, 2)))

    def uv_de(self, P2):
        u = self.u0 + self.sinal_u * P2[:, 0] / self.su
        v = self.v0 + self.sinal_v * P2[:, 1] / self.sv
        return np.stack([v, u], 1) if self.troca else np.stack([u, v], 1)

    def de_uv(self, u, v):
        """O 2D (s, h) de um ponto dado em pixels da textura"""
        if self.troca:
            u, v = v, u
        return np.array([(u - self.u0) * self.sinal_u * self.su, (v - self.v0) * self.sinal_v * self.sv])

    def para_3d(self, P2, afast):
        P2 = np.asarray(P2, float)
        q = self.uv_de(P2)
        U, G = self.tris.U, self.tris.G
        a, b, c = U[:, 0], U[:, 1], U[:, 2]
        d = (b[:, 1] - c[:, 1]) * (a[:, 0] - c[:, 0]) + (c[:, 0] - b[:, 0]) * (a[:, 1] - c[:, 1])
        d = np.where(np.abs(d) < 1e-12, 1e-12, d)
        qx, qy = q[:, 0:1], q[:, 1:2]
        l1 = ((b[:, 1] - c[:, 1]) * (qx - c[:, 0]) + (c[:, 0] - b[:, 0]) * (qy - c[:, 1])) / d
        l2 = ((c[:, 1] - a[:, 1]) * (qx - c[:, 0]) + (a[:, 0] - c[:, 0]) * (qy - c[:, 1])) / d
        l3 = 1 - l1 - l2
        pior = np.minimum(np.minimum(l1, l2), l3)
        k = np.argmax(pior, axis=1)
        r = np.arange(len(P2))
        L = np.stack([l1[r, k], l2[r, k], l3[r, k]], 1)
        P = (L[:, :, None] * G[k]).sum(1)
        N = self.tris.N[k]
        af = np.broadcast_to(np.asarray(afast, float), (len(P2),))
        return P + af[:, None] * N, N


# ------------------------------------------------------------------
# 5. Os tubos
# ------------------------------------------------------------------
class Luzes:
    """Junta as peças (vértices, triângulos, cor RGBA por vértice) de um lugar"""

    def __init__(self, nome):
        self.nome = nome
        self.pecas = []
        self.contagem = defaultdict(int)

    def _juntar(self, V, F, cor, alfa, rotulo):
        n = len(V)
        cor = np.asarray(cor, float)
        cor = np.broadcast_to(cor, (n, 3)) if cor.ndim == 1 else cor
        alfa = np.broadcast_to(np.asarray(alfa, float), (n,))
        C = np.concatenate([cor, np.clip(alfa, 0, 1)[:, None]], 1)
        self.pecas.append((np.asarray(V, float), np.asarray(F, np.int64), C))
        self.contagem[rotulo] += len(F)

    def tubo(self, P, N, raio, cor, alfa, fechado, lados=6, rotulo='tubos'):
        """Um tubo pela polilinha P (n, 3); N (n, 3) dá o lado de 'fora' (a normal do letreiro), para o anel não
        girar à toa; cor (3,) ou (n, 3) e alfa escalar ou (n,) por ponto"""
        P = np.asarray(P, float)
        N = np.broadcast_to(np.asarray(N, float), P.shape)
        cor = np.asarray(cor, float)
        alfa = np.broadcast_to(np.asarray(alfa, float), (len(P),))
        # tira os pontos repetidos
        d = np.linalg.norm(np.diff(P, axis=0), axis=1)
        ok = np.concatenate([[True], d > 1e-5])
        if fechado and len(P) > 2 and np.linalg.norm(P[-1] - P[0]) < 1e-5:
            ok[-1] = False
        P, N, alfa = P[ok], N[ok], alfa[ok]
        if cor.ndim == 2:
            cor = cor[ok]
        n = len(P)
        if n < 2:
            return
        if fechado:
            T = np.roll(P, -1, 0) - np.roll(P, 1, 0)
        else:
            T = np.empty_like(P)
            T[1:-1] = P[2:] - P[:-2]
            T[0] = P[1] - P[0]
            T[-1] = P[-1] - P[-2]
        T = normalizar(T)
        B = np.cross(T, N)
        ruim = np.linalg.norm(B, axis=1) < 1e-4
        if ruim.any():
            B[ruim] = np.cross(T[ruim], np.where(np.abs(T[ruim, 1:2]) < 0.9, CIMA, [1.0, 0, 0]))
        B = normalizar(B)
        Nn = np.cross(B, T)
        ang = 2 * np.pi * np.arange(lados) / lados
        V = (P[:, None, :] + raio * (np.cos(ang)[None, :, None] * Nn[:, None, :] +
                                     np.sin(ang)[None, :, None] * B[:, None, :])).reshape(-1, 3)
        segs = n if fechado else n - 1
        i = np.arange(segs)[:, None]
        k = np.arange(lados)[None, :]
        j = (i + 1) % n
        k2 = (k + 1) % lados
        a, b, c, dd = i * lados + k, i * lados + k2, j * lados + k2, j * lados + k
        F = np.concatenate([np.stack([a, b, c], -1).reshape(-1, 3), np.stack([a, c, dd], -1).reshape(-1, 3)])
        cores = np.repeat(cor, lados, 0) if cor.ndim == 2 else cor
        alfas = np.repeat(alfa, lados)
        if not fechado:
            # as tampas das pontas (o eletrodo)
            c0, c1 = len(V), len(V) + 1
            V = np.vstack([V, P[0], P[-1]])
            kk = np.arange(lados)
            F = np.concatenate([F, np.stack([np.full(lados, c0), (kk + 1) % lados, kk], 1),
                                np.stack([np.full(lados, c1), (n - 1) * lados + kk, (n - 1) * lados + (kk + 1) % lados], 1)])
            if cor.ndim == 2:
                cores = np.vstack([cores, cor[0], cor[-1]])
            alfas = np.concatenate([alfas, [alfa[0], alfa[-1]]])
        self._juntar(V, F, cores, alfas, rotulo)

    def neon(self, superficie, lacos, afast, raio, cor, alfa_de, lados=6, passo=0.2, rotulo='letras'):
        """Os laços 2D de contornos() viram tubos na superfície; alfa_de(rótulos, pontos2d) dá a ordem"""
        for P2, rot in lacos:
            P2s = subdividir(P2, passo, True)
            # os rótulos dos pontos novos: o do ponto de origem mais perto
            if len(P2s) != len(P2):
                d = np.linalg.norm(P2s[:, None, :] - P2[None, :, :], axis=2)
                rot = rot[np.argmin(d, axis=1)]
            P3, N3 = superficie.para_3d(P2s, afast)
            self.tubo(P3, N3, raio, cor, alfa_de(rot, P2s), True, lados, rotulo)


def ordem_por_letra(alfas):
    """alfa_de para neon(): cada rótulo (índice da letra) tem o seu alfa"""
    alfas = np.asarray(alfas, float)
    return lambda rot, P2: alfas[np.clip(rot, 0, len(alfas) - 1)]


def ordem_fixa(a):
    return lambda rot, P2: np.full(len(P2), a)


def anel_em_volta(poligono_xz, y, afast, fechado=True, raio_curva=0.6):
    """Um anel a uma altura y em volta de um polígono convexo (x, z), afastado dele (sai pelos cantos em
    bissetriz), com as quinas arredondadas"""
    Q = np.asarray(poligono_xz, float)
    n = len(Q)
    nor = []
    for i in range(n):
        a, b, c = Q[i - 1], Q[i], Q[(i + 1) % n]
        e1 = normalizar(b - a)
        e2 = normalizar(c - b)
        n1 = np.array([e1[1], -e1[0]])
        n2 = np.array([e2[1], -e2[0]])
        m = normalizar(n1 + n2)
        nor.append(m / max(m @ n1, 0.3))
    nor = np.array(nor)
    # o sentido: para fora do centro
    cen = Q.mean(0)
    if ((Q - cen) * nor).sum() < 0:
        nor = -nor
    R = Q + afast * nor
    P = np.stack([R[:, 0], np.full(n, y), R[:, 1]], 1)
    P = arredondar(P, raio_curva, fechado, passo=math.radians(8))
    N = np.stack([P[:, 0] - cen[0], np.zeros(len(P)), P[:, 2] - cen[1]], 1)
    return P, normalizar(N)


def poligono_por_angulo(pontos_xz, centro=None, bins=None):
    """Os pontos (x, z) ordenados pelo ângulo em volta do centro; com bins, fica o mais afastado de cada fatia"""
    Q = np.asarray(pontos_xz, float)
    c = Q.mean(0) if centro is None else np.asarray(centro, float)
    ang = np.arctan2(Q[:, 1] - c[1], Q[:, 0] - c[0])
    r = np.hypot(Q[:, 0] - c[0], Q[:, 1] - c[1])
    if bins:
        k = ((ang + np.pi) / (2 * np.pi) * bins).astype(int) % bins
        sel = []
        for b in np.unique(k):
            ii = np.nonzero(k == b)[0]
            sel.append(ii[np.argmax(r[ii])])
        Q, ang = Q[sel], ang[sel]
    o = np.argsort(ang)
    Q = Q[o]
    # junta os pontos quase iguais
    manter = np.concatenate([[True], np.linalg.norm(np.diff(Q, axis=0), axis=1) > 0.02])
    return Q[manter]


def casco_convexo(Q):
    Q = np.unique(np.round(np.asarray(Q, float), 4), axis=0)
    Q = Q[np.lexsort((Q[:, 1], Q[:, 0]))]

    def cruz(o, a, b):
        return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])
    baixo, cima = [], []
    for p in Q:
        while len(baixo) >= 2 and cruz(baixo[-2], baixo[-1], p) <= 0:
            baixo.pop()
        baixo.append(p)
    for p in Q[::-1]:
        while len(cima) >= 2 and cruz(cima[-2], cima[-1], p) <= 0:
            cima.pop()
        cima.append(p)
    return np.array(baixo[:-1] + cima[:-1])


def estrela_2d(r_fora, r_dentro, pontas=8, aberta_embaixo=0.0):
    """Os vértices de uma estrela com uma ponta para cima; com aberta_embaixo > 0, a ponta de baixo vira duas
    pernas que descem até essa distância do centro (o poste entra por ali), e a linha fica aberta"""
    ang = np.pi / 2 + np.pi * np.arange(2 * pontas) / pontas
    r = np.where(np.arange(2 * pontas) % 2 == 0, r_fora, r_dentro)
    P = np.stack([r * np.cos(ang), r * np.sin(ang)], 1)
    if not aberta_embaixo:
        return P, True
    k = pontas      # a ponta de baixo
    perna_e = P[k - 1] * [1, 0] + [0, -aberta_embaixo]
    perna_d = P[k + 1] * [1, 0] + [0, -aberta_embaixo]
    ordem = [P[(k + 1 + i) % (2 * pontas)] for i in range(2 * pontas - 1)]
    return np.array([perna_d] + ordem + [perna_e]), False


def oito_2d(x0, y0, x1, y1):
    """O 8 do V8 (como o do mapa: a volta de baixo maior), como função dentro/fora na caixa dada"""
    w, h = x1 - x0, y1 - y0
    cx = (x0 + x1) / 2

    def el(X, Y, c, r):
        return ((X - c[0]) / r[0]) ** 2 + ((Y - c[1]) / r[1]) ** 2 <= 1

    def f(X, Y):
        baixo = el(X, Y, (cx, y0 + 0.249 * h), (0.500 * w, 0.249 * h))
        cima = el(X, Y, (cx - 0.004 * w, y0 + 0.749 * h), (0.434 * w, 0.249 * h))
        furo_b = el(X, Y, (cx, y0 + 0.245 * h), (0.311 * w, 0.137 * h))
        furo_c = el(X, Y, (cx - 0.004 * w, y0 + 0.756 * h), (0.275 * w, 0.142 * h))
        return (baixo | cima) & ~furo_b & ~furo_c
    furo_c = (cx - 0.004 * w, y0 + 0.756 * h, 0.275 * w, 0.142 * h)
    volta_c = (cx - 0.004 * w, y0 + 0.749 * h, 0.434 * w, 0.249 * h)
    return f, furo_c, volta_c


def linha_v(base, topo_e, topo_d, meio_y):
    """O V8 em prisma (as pernas com a crista no meio): devolve o contorno (fechado) e as duas linhas do
    meio das pernas, em 2D (x, y no plano do letreiro)"""
    ponta = np.asarray(base, float)
    forq = np.array([ponta[0], meio_y])
    contorno = np.array([ponta, topo_e, forq, topo_d])
    crista0 = np.array([ponta[0], (ponta[1] + meio_y) / 2])
    return contorno, [np.array([crista0, topo_e]), np.array([crista0, topo_d])]


def medir_coluna(tris, y0, y1, rmin, rmax, centro):
    """O polígono (x, z) dos vértices entre as alturas dadas e os raios dados em volta do centro"""
    V = tris.vertices()
    r = np.hypot(V[:, 0] - centro[0], V[:, 2] - centro[1])
    sel = (V[:, 1] >= y0) & (V[:, 1] <= y1) & (r >= rmin) & (r <= rmax)
    if sel.sum() < 3:
        raise SystemExit('ERRO: não achei a coluna (%.2f..%.2f, r %.2f..%.2f)' % (y0, y1, rmin, rmax))
    return casco_convexo(V[sel][:, [0, 2]])


# ------------------------------------------------------------------
# 6. Ramone's House of Body Art
# ------------------------------------------------------------------
def montar_ramone():
    print('\nRamone\'s House of Body Art')
    L = Luzes('ramone')
    # A torre: as quatro faces com o RAMONES pintado (o mesmo pedaço da textura em cada uma)
    faces = T45.filtro(uv=(105, 140, 80, 256), uv_todos=True, caixa=(470, 490, 12, 24, 100, 115))
    faces = faces.sub(np.abs(faces.N[:, 1]) < 0.2)
    centro = faces.C.mean(0)
    cx, cz = centro[0], centro[2]
    y_base, y_topo = faces.G[..., 1].min(), faces.G[..., 1].max()
    print('  torre: centro (%.2f, %.2f), RAMONES de y %.2f a %.2f' % (cx, cz, y_base, y_topo))
    sups = []
    for d in ([1, 0, 0], [-1, 0, 0], [0, 0, 1], [0, 0, -1]):
        f = faces.filtro(normal=d, minimo=0.95)
        sups.append(SuperficieUV(f, 'face %s da torre' % d))
    # As letras: uma por vez de cima para baixo, na fonte de forma, larga
    letras = 'RAMONES'
    altura_da_letra = 1.04
    for sup in sups:
        L_ = sup.largura
        # as letras ocupam a altura da faixa pintada (de 6 % a 95,5 % da face), de cima para baixo
        h0, h1 = 0.06 * sup.altura, 0.955 * sup.altura
        passo_y = (h1 - h0) / len(letras)
        for k, ch in enumerate(letras):
            hc = h1 - (k + 0.5) * passo_y
            formas = texto(ch, FONTE_DE_CAIXA)
            formas, _ = encaixar(formas, 0.5 * L_ - 0.50, hc - altura_da_letra / 2, 0.5 * L_ + 0.50,
                                 hc + altura_da_letra / 2, escala_x=1.45)
            cont = contornos(formas, [0.0, -0.085], px=0.006, tol=0.005, raio_do_tubo=0.03)
            a = 0.22 + 0.06 * k
            L.neon(sup, cont[0.0], 0.06, 0.03, LILAS, ordem_fixa(a), rotulo='RAMONES')
            L.neon(sup, cont[-0.085], 0.06, 0.026, AZUL_LILAS, ordem_fixa(a + 0.01), rotulo='RAMONES')
    # Os cantos da torre e os anéis de cima e de baixo, em verde-água
    oct_ = medir_coluna(T45, y_base - 0.05, y_base + 0.05, 1.0, 1.3, (cx, cz))
    raio_vd = 0.04
    for y in (y_base + 0.42, y_topo - 0.12):
        P, N = anel_em_volta(oct_, y, raio_vd + 0.035, raio_curva=0.12)
        L.tubo(P, N, raio_vd, VERDE_AGUA, 0.12, True, 8, rotulo='torre verde-água')
    for i in range(len(oct_)):
        a, b = oct_[i], oct_[(i + 1) % len(oct_)]
        if np.linalg.norm(b - a) > 0.4:
            continue     # só os chanfros dos cantos
        m = (a + b) / 2
        dirc = normalizar(np.array([m[0] - cx, m[1] - cz]))
        p = m + dirc * (raio_vd + 0.035)
        P = np.array([[p[0], y, p[1]] for y in np.linspace(y_base + 0.42, y_topo - 0.12, 30)])
        L.tubo(P, [dirc[0], 0, dirc[1]], raio_vd, VERDE_AGUA, 0.14, False, 8, rotulo='torre verde-água')
    # A coluna de cima (y 23,07 a 24,88) e a bola
    V = T45.vertices()
    r = np.hypot(V[:, 0] - cx, V[:, 2] - cz)
    sel = (V[:, 1] > y_topo + 0.2) & (V[:, 1] < y_topo + 2.3) & (r < 0.8) & (r > 0.5)
    col = casco_convexo(V[sel][:, [0, 2]])
    ys_col = V[sel][:, 1]
    for y in (ys_col.min() + 0.08, ys_col.max() - 0.08):
        P, N = anel_em_volta(col, y, 0.065, raio_curva=0.1)
        L.tubo(P, N, 0.035, VERDE_AGUA, 0.16, True, 8, rotulo='torre verde-água')
    for ang in (45, 135, 225, 315):
        d = np.array([math.cos(math.radians(ang)), math.sin(math.radians(ang))])
        k = int(np.argmax(col @ d - col.mean(0) @ d))
        p = col[k] + d * 0.065
        P = np.array([[p[0], y, p[1]] for y in np.linspace(ys_col.min() + 0.08, ys_col.max() - 0.08, 12)])
        L.tubo(P, [d[0], 0, d[1]], 0.035, VERDE_AGUA, 0.17, False, 8, rotulo='torre verde-água')
    bola = V[(V[:, 1] > ys_col.max() + 0.05) & (r < 0.9)]
    by0, by1 = bola[:, 1].min(), bola[:, 1].max()
    rb = np.hypot(bola[:, 0] - cx, bola[:, 2] - cz).max()
    yeq = bola[np.argmax(np.hypot(bola[:, 0] - cx, bola[:, 2] - cz)), 1]
    cb = np.array([cx, (by0 + by1) / 2, cz])
    t = np.linspace(0, 2 * np.pi, 41)[:-1]
    re = rb + 0.07
    P = np.stack([cx + re * np.cos(t), np.full(len(t), yeq), cz + re * np.sin(t)], 1)
    L.tubo(P, normalizar(P - [cx, yeq, cz]), 0.03, VERDE_AGUA, 0.18, True, 8, rotulo='torre verde-água')
    rv = (by1 - by0) / 2 + 0.07
    for eixo in ([1, 0, 0], [0, 0, 1]):
        e = np.array(eixo, float)
        P = cb + re * np.cos(t)[:, None] * e + rv * np.sin(t)[:, None] * CIMA
        L.tubo(P, normalizar(P - cb), 0.03, VERDE_AGUA, 0.19, True, 8, rotulo='torre verde-água')
    print('  coluna de cima y %.2f..%.2f, bola y %.2f..%.2f (raio %.2f)' % (ys_col.min(), ys_col.max(), by0, by1, rb))
    # Os tambores em volta da base da torre: a cúpula (o tambor largo, que desce até o telhado) e o de cima
    tambor = medir_coluna(T45, 10.4, 10.6, 3.3, 4.1, (cx, cz))
    ys = [8.62, 10.40]
    for k, y in enumerate(ys):
        P, N = anel_em_volta(tambor, y, 0.09, raio_curva=0.5)
        L.tubo(P, N, 0.05, VERDE_AGUA, 0.00 + 0.03 * k, True, 8, rotulo='cúpula verde-água')
    tambor2 = medir_coluna(T45, 11.5, 13.1, 2.1, 2.6, (cx, cz))
    for k, y in enumerate((11.72, 12.96)):
        P, N = anel_em_volta(tambor2, y, 0.08, raio_curva=0.35)
        L.tubo(P, N, 0.045, VERDE_AGUA, 0.06 + 0.03 * k, True, 8, rotulo='cúpula verde-água')

    # "House of Body Art" na fachada curva (o pedaço da textura de u 136 a 255, v 224 a 255)
    fach = T45.filtro(uv=(135, 256, 224, 256), uv_todos=True)
    fach = fach.sub(np.abs(fach.N[:, 1]) < 0.2)
    sup = SuperficieUV(fach, 'fachada House of Body Art')
    # o lugar das palavras na textura (pixels): House (de lado), of, Body Art
    s_b0, h_b0 = sup.de_uv(173.5, 229.5)
    s_b1, h_b1 = sup.de_uv(246.0, 251.5)
    s_h0, h_h0 = sup.de_uv(146.0, 236.0)
    s_h1, h_h1 = sup.de_uv(172.0, 253.0)
    s_o0, h_o0 = sup.de_uv(158.5, 228.5)
    s_o1, h_o1 = sup.de_uv(170.0, 238.0)
    body, _ = encaixar(texto('Body Art', FONTE_DE_MAO), s_b0, h_b0, s_b1, h_b1)
    house = texto('House', FONTE_DE_MAO)
    house = transformar(house, girar=26)
    a, b = caixa_de(house)
    esc = min((s_h1 - s_h0) / (b[0] - a[0]), (h_h1 - h_h0) / (b[1] - a[1]))
    house = transformar(texto('House', FONTE_DE_MAO), esc, girar=26)
    a, b = caixa_de(house)
    house = [(r, T + ((s_h0 + s_h1) / 2 - (a[0] + b[0]) / 2, (h_h0 + h_h1) / 2 - (a[1] + b[1]) / 2)) for r, T in house]
    of_, _ = encaixar(texto('of', FONTE_DE_MAO), s_o0, h_o0, s_o1, h_o1)
    afast = 0.07
    for formas, alfas, nome in ((house, [0.64, 0.65, 0.66, 0.67, 0.68], 'House'), (of_, [0.71, 0.72], 'of'),
                                (body, [0.76, 0.795, 0.83, 0.865, 0, 0.90, 0.935, 0.97], 'Body Art')):
        a, b = caixa_de(formas)
        grosso = (b[1] - a[1]) > 0.7
        dentro = -0.075 if grosso else -0.055
        raio = 0.03 if grosso else 0.026
        cont = contornos(formas, [0.0, dentro], px=0.005, tol=0.004, raio_do_tubo=raio)
        L.neon(sup, cont[0.0], afast, raio, LILAS_BRANCO, ordem_por_letra(alfas), rotulo=nome)
        L.neon(sup, cont[dentro], afast, max(0.025, raio * 0.85), AZUL_LILAS, ordem_por_letra(np.array(alfas) + 0.01),
               rotulo=nome)

    # As luzes que os neons jogam em volta (eixos do jogo)
    def ponto(s, h, af):
        P, _ = sup.para_3d(np.array([[s, h]]), af)
        return P[0]
    luzes = []
    for s, raio_l, forca in ((sup.largura * 0.66, 13.0, 1.6), (sup.largura * 0.2, 9.0, 1.0)):
        p = ponto(s, 0.6, 2.2)
        luzes.append((p, LILAS, raio_l, forca))
    meia = 0.5 * sups[0].largura
    for d, forca in (([0, 0, 1], 1.1), ([1, 0, 0], 1.0), ([-1, 0, 0], 0.7), ([0, 0, -1], 0.7)):
        p = np.array([cx, (y_base + y_topo) / 2, cz]) + np.array(d) * (meia + 2.2)
        luzes.append((p, LILAS, 12.0 if d[2] > 0 else 10.0, forca))
    for d in ([0.6, 0, 0.8], [-0.6, 0, 0.8]):
        p = np.array([cx, 10.2, cz]) + normalizar(d) * 5.6
        luzes.append((p, VERDE_AGUA, 9.0, 0.9))
    return L, luzes


# ------------------------------------------------------------------
# 7. Flo's V8 Café
# ------------------------------------------------------------------
UV_DO_CINZA = (612, 950, 225, 400)     # o cinza em volta das letras da faixa: os discos e as marquises
UV_DO_TETO = (512, 690, 0, 185)        # o forro das marquises, com as duas calhas pintadas
UV_DA_ESTRELA = (588, 692, 405, 510)
UV_DO_OVAL = (715, 1025, 400, 512)
UV_DA_SAIA = (890, 1024, 100, 512)     # as saias (o tecido enrolado) em volta do alto das colunas das marquises


def fascia(tris, y0, y1, rmin, centro):
    """O polígono (x, z) da borda de fora de um disco do prédio redondo (as faces em pé entre y0 e y1)"""
    t = tris.sub((np.abs(tris.N[:, 1]) < 0.3) & (tris.G[..., 1].min(1) >= y0 - 0.02) & (tris.G[..., 1].max(1) <= y1 + 0.02))
    V = t.G.reshape(-1, 3)
    r = np.hypot(V[:, 0] - centro[0], V[:, 2] - centro[1])
    V = V[r > rmin]
    return poligono_por_angulo(V[:, [0, 2]], centro, bins=64)


def corte_em_z(tris, z0):
    """Os segmentos (x0, y0, x1, y1) onde o plano z = z0 corta os triângulos"""
    G = tris.G
    s = G[..., 2] - z0
    segs = []
    for g, sg in zip(G, s):
        if (sg > 0).all() or (sg < 0).all():
            continue
        pts = []
        for i in range(3):
            a, b, sa, sb = g[i], g[(i + 1) % 3], sg[i], sg[(i + 1) % 3]
            if (sa > 0) != (sb > 0):
                t = sa / (sa - sb)
                pts.append(a + t * (b - a))
        if len(pts) == 2:
            segs.append((pts[0][0], pts[0][1], pts[1][0], pts[1][1]))
    return np.array(segs).reshape(-1, 4)


def montar_flo():
    print('\nFlo\'s V8 Café')
    L = Luzes('flo')
    # O prédio redondo: o centro pela borda do disco de cima
    disco = T71.filtro(uv=UV_DO_CINZA, caixa=(530, 570, 12.2, 13.2, 50, 82))
    Vd = disco.G.reshape(-1, 3)
    centro = np.array([(Vd[:, 0].min() + Vd[:, 0].max()) / 2, (Vd[:, 2].min() + Vd[:, 2].max()) / 2])
    print('  prédio redondo: centro (%.2f, %.2f)' % tuple(centro))
    cinza = T71.filtro(uv=UV_DO_CINZA, caixa=(530, 570, 5, 14, 50, 82))
    # Os anéis ciano: dois no disco de cima, dois no do meio, um no beiral de cima das vitrines
    aneis = [(12.30, 13.06, (12.43, 12.93), 0.21), (7.32, 8.08, (7.45, 7.95), 0.15), (5.56, 5.84, (5.70,), 0.12)]
    for y0, y1, alturas, a in aneis:
        pol = fascia(cinza if y0 > 6 else T71.filtro(caixa=(530, 570, 5.4, 6.0, 50, 82)), y0, y1, 8.0, centro)
        for k, y in enumerate(alturas):
            P, N = anel_em_volta(pol, y, 0.085, raio_curva=1.2)
            L.tubo(P, N, 0.042, CIANO, a + 0.03 * k, True, 8, rotulo='anéis ciano')
    # A faixa de cima das vitrines: cada palavra no painel em que o mapa a pintou (o mapa tem GREASE antes de
    # COOLANT; no filme é o contrário)
    banda = T72.filtro(uv=UV_DO_CINZA, caixa=(530, 570, 5.6, 6.9, 70, 82))
    banda = banda.sub(np.abs(banda.N[:, 1]) < 0.3)
    for palavra, uvp, alfa in (('LUGNUTS', (660, 812, 340, 402), 0.34), ('GAS', (810, 878, 340, 402), 0.37),
                               ('OIL', (878, 952, 340, 402), 0.40), ('GREASE', (612, 790, 222, 281), 0.43),
                               ('COOLANT', (662, 803, 282, 341), 0.46)):
        painel = banda.filtro(uv=uvp, uv_todos=True)
        rad = normalizar(np.stack([painel.C[:, 0] - centro[0], np.zeros(len(painel)), painel.C[:, 2] - centro[1]], 1))
        painel = painel.sub((painel.N * rad).sum(1) > 0.5)     # só as faces de fora
        sup = SuperficieUV(painel, 'painel %s' % palavra)
        formas, _ = encaixar(texto(palavra, FONTE_DE_CAIXA), 0.05 * sup.largura, 0.17 * sup.altura,
                             0.95 * sup.largura, 0.80 * sup.altura)
        cont = contornos(formas, [0.0], px=0.004, tol=0.004, raio_do_tubo=0.025)
        L.neon(sup, cont[0.0], 0.06, 0.025, ROSA_FAIXA, ordem_fixa(alfa), passo=0.3, rotulo='LUGNUTS')
    # O V8 da frente (as pernas do V, a crista do meio e a caixa do 8), medido nos vértices
    v8 = TF.filtro(caixa=(546, 552.5, 6, 17, 79.9, 81.5))
    Vv = v8.vertices()
    ponta = Vv[np.argmin(Vv[:, 1])]
    topo = Vv[np.abs(Vv[:, 1] - Vv[:, 1].max()) < 0.01]
    topo_e, topo_d = topo[np.argmin(topo[:, 0])], topo[np.argmax(topo[:, 0])]
    na_linha = Vv[(np.abs(Vv[:, 0] - ponta[0]) < 0.02) & (Vv[:, 1] < 10.5)]
    forquilha_y = na_linha[:, 1].max()
    crista_z0 = Vv[(np.abs(Vv[:, 0] - ponta[0]) < 0.02)][:, 2].max()
    print('  V: ponta (%.2f, %.2f), forquilha y %.2f, topos (%.2f, %.2f) e (%.2f, %.2f), crista z %.2f..%.2f' % (
        ponta[0], ponta[1], forquilha_y, topo_e[0], topo_e[1], topo_d[0], topo_d[1], crista_z0, topo_e[2]))

    # O V (rosa no contorno, ciano na crista) e o 8 (rosa, ciano por dentro) com o emblema (V)
    def z_borda(y):
        t = np.clip((y - ponta[1]) / (topo_e[1] - ponta[1]), 0, 1)
        return ponta[2] * (1 - t) + topo_e[2] * t

    def z_crista(y):
        t = np.clip((y - (ponta[1] + forquilha_y) / 2) / (topo_e[1] - (ponta[1] + forquilha_y) / 2), 0, 1)
        return crista_z0 * (1 - t) + topo_e[2] * t
    contorno, cristas = linha_v(ponta[:2], topo_e[:2], topo_d[:2], forquilha_y)
    P2 = subdividir(np.array(contorno), 0.25, True)
    P2 = arredondar(P2, 0.05, True)
    P = np.stack([P2[:, 0], P2[:, 1], z_borda(P2[:, 1]) + 0.09], 1)
    L.tubo(P, [0, 0, 1], 0.035, ROSA, 0.64, True, 8, rotulo='V8')
    for c in cristas:
        P2 = subdividir(c, 0.25, False)
        P = np.stack([P2[:, 0], P2[:, 1], z_crista(P2[:, 1]) + 0.07], 1)
        L.tubo(P, [0, 0, 1], 0.03, CIANO, 0.645, False, 8, rotulo='V8')
    # o 8 e a caixa dele (entre as pernas, de y 11,0 a 15,4): os vértices da face da frente
    oito = Vv[(Vv[:, 1] > 11.2) & (Vv[:, 1] < 15.45) & (np.abs(Vv[:, 0] - ponta[0]) < 1.4)]
    oito = oito[oito[:, 2] > oito[:, 2].max() - 0.05]
    ox0, ox1, oy0, oy1 = oito[:, 0].min(), oito[:, 0].max(), oito[:, 1].min(), oito[:, 1].max()
    z_oito = max(crista_z0, oito[:, 2].max()) + 0.10
    print('  8: x %.2f..%.2f, y %.2f..%.2f; neon em z %.2f' % (ox0, ox1, oy0, oy1, z_oito))
    plano8 = Plano([0, 0, z_oito], [1, 0, 0], [0, 1, 0], [0, 0, 1])
    fazer_oito(L, plano8, ox0, oy0, ox1, oy1, 0.68)

    # "Flo's" e "Cafe" no telhado (as placas da frente)
    placa_flo = T72.filtro(uv=(725, 910, 60, 145), caixa=(540, 548, 12.5, 16, 77, 82), normal=[-0.45, 0, 0.89], minimo=0.9)
    placa_cafe = TF.filtro(uv=(330, 662, 285, 382), caixa=(551, 558, 12, 16, 77, 82))
    placa_cafe = placa_cafe.sub(placa_cafe.N[:, 2] > 0.5)
    placa_cafe = so_a_da_frente(placa_cafe)
    for tris, txt, alfas, nome in ((placa_flo, "Flo's", [0.50, 0.53, 0.56, 0.58, 0.60], "Flo's"),
                                   (placa_cafe, 'Cafe', [0.74, 0.77, 0.80, 0.83], 'Cafe')):
        sup = SuperficieUV(tris, 'placa %s do telhado' % nome)
        letras_neon(L, sup, txt, alfas, 0.06 * sup.largura, 0.10 * sup.altura, 0.94 * sup.largura,
                    0.90 * sup.altura, 0.075, 0.034, nome)

    # As vitrines quentes (a frente aberta e as janelas dos lados e de trás)
    base_v = T72.filtro(caixa=(530, 570, 2.5, 5.7, 50, 82))
    base_v = base_v.sub((np.abs(base_v.N[:, 1]) < 0.2) & (base_v.G[..., 1].min(1) > 2.9) & (base_v.G[..., 1].max(1) < 5.65))
    rad = normalizar(np.stack([base_v.C[:, 0] - centro[0], np.zeros(len(base_v)), base_v.C[:, 2] - centro[1]], 1))
    base_v = base_v.sub((base_v.N * rad).sum(1) > 0.9)
    angs = np.degrees(np.arctan2(base_v.C[:, 0] - centro[0], base_v.C[:, 2] - centro[1]))
    vidro = (np.abs(angs) < 19) | ((np.abs(angs) > 23) & (np.abs(angs) < 64)) | ((np.abs(angs) > 115) & (np.abs(angs) < 134))
    vidros = base_v.sub(vidro)
    n_v = 0
    for g, n in zip(vidros.G, vidros.N):
        # cada triângulo do vidro vira um triângulo de luz, 3 cm para fora, sem a moldura de baixo e de cima
        gg = g.copy()
        gg[:, 1] = np.clip(gg[:, 1], 3.25, 5.42)
        t = ((gg[:, 1] - 3.25) / (5.42 - 3.25))[:, None]
        cor = np.array(QUENTE_BAIXO) * (1 - t) + np.array(QUENTE) * t
        L._juntar(gg + n * 0.03, np.array([[0, 1, 2]]), cor, 0.0, 'vitrines')
        n_v += 1
    print('  vitrines: %d triângulos' % n_v)

    # As marquises das bombas
    estrelas = T72.filtro(uv=UV_DA_ESTRELA)
    estrelas_c = []
    for g in estrelas.G:
        c = g.mean(0)
        if not any(np.linalg.norm(c - e) < 1.2 for e in estrelas_c):
            estrelas_c.append(c)
    est = []
    for c in estrelas_c:
        t = estrelas.sub(np.linalg.norm(estrelas.C - c, axis=1) < 1.2)
        est.append(t.G.reshape(-1, 3).mean(0))
    est = np.array(est)
    print('  %d estrelas' % len(est))
    luzes_marquise = []
    for lado in (-1, 1):
        sel = (est[:, 0] < centro[0]) if lado < 0 else (est[:, 0] > centro[0])
        minhas = est[sel]
        xs = minhas[:, 0].mean()
        teto = T72.filtro(uv=UV_DO_TETO, caixa=(xs - 6, xs + 6, 4, 9, 84, 124), normal=[0, -1, 0], minimo=0.6)
        Vt = teto.G.reshape(-1, 3)
        z0, z1 = Vt[:, 2].min(), Vt[:, 2].max()
        zm = z0 + 0.37 * (z1 - z0)        # longe das emendas dos pedaços do forro
        segs = corte_em_z(TF.filtro(caixa=(xs - 6, xs + 6, 4.5, 9, z0 - 1, z1 + 1)), zm)
        xa = np.concatenate([segs[:, 0], segs[:, 2]])
        ya = np.concatenate([segs[:, 1], segs[:, 3]])
        x_fora = xa.min() if lado < 0 else xa.max()
        x_dentro = xa.max() if lado < 0 else xa.min()
        y_fora = ya[np.abs(xa - x_fora) < 0.12]
        y_dentro = ya[np.abs(xa - x_dentro) < 0.12]
        # o forro: y = a + b x (pelos vértices dele), e as calhas pintadas pelo u da textura
        A = np.stack([np.ones(len(Vt)), Vt[:, 0]], 1)
        coef = np.linalg.lstsq(A, Vt[:, 1], rcond=None)[0]
        Ut = teto.U.reshape(-1, 2)
        cu = np.linalg.lstsq(A, Ut[:, 0], rcond=None)[0]       # u = c0 + c1 x
        print('  marquise %s: z %.2f..%.2f, borda de fora x %.2f (y %.2f..%.2f), de dentro x %.2f (y %.2f..%.2f), forro y = %.3f + %.4f x' % (
            'oeste' if lado < 0 else 'leste', z0, z1, x_fora, y_fora.min(), y_fora.max(), x_dentro, y_dentro.min(),
            y_dentro.max(), coef[0], coef[1]))
        # as bordas ciano (de fora e de dentro), ao longo de z
        zs = np.linspace(z0 + 0.06, z1 - 0.06, 40)
        for x, ys_, a in ((x_fora, y_fora, 0.27), (x_dentro, y_dentro, 0.275)):
            fora = 1.0 if (x == x_fora) == (lado > 0) else -1.0     # o lado de fora desta borda, em x
            P = np.stack([np.full(len(zs), x + fora * 0.075), np.full(len(zs), (ys_.min() + ys_.max()) / 2), zs], 1)
            L.tubo(P, [fora, 0, 0], 0.045, CIANO, a, False, 8, rotulo='marquises ciano')
        # as calhas fluorescentes: duas lâmpadas por calha, em pedaços de 2,4 m
        rng = np.random.default_rng(7 if lado < 0 else 11)
        nrm = normalizar(np.array([coef[1], -1.0, 0.0]))     # para baixo, perpendicular ao forro
        for u in (533.0, 661.0):
            xc = (u - cu[0]) / cu[1]
            for dx in (-0.07, 0.07):
                x = xc + dx
                y = coef[0] + coef[1] * x
                p0 = np.array([x, y, 0]) + nrm * 0.045
                z = z0 + 0.35
                while z + 1.0 < z1 - 0.3:
                    z2 = min(z + 2.4, z1 - 0.35)
                    P = np.array([[p0[0], p0[1], z], [p0[0], p0[1], z2]])
                    P = subdividir(P, 0.6, False)
                    L.tubo(P, -nrm, 0.025, BRANCO_FRIO, rng.uniform(0.02, 0.10), False, 6, rotulo='fluorescentes')
                    z = z2 + 0.15
        luzes_marquise.append(((x_fora + x_dentro) / 2, coef[0] + coef[1] * (x_fora + x_dentro) / 2 - 2.6, (z0 + z1) / 2))
        # as saias das colunas: o aro de baixo, rosa
        nrm_cima = -nrm
        for e in minhas:
            saia = T72.filtro(uv=UV_DA_SAIA, caixa=(e[0] - 2.0, e[0] + 2.0, 4.9, 7.1, e[2] - 2.0, e[2] + 2.0))
            Vs = saia.vertices()
            hs = Vs @ nrm_cima
            sel_s = hs < hs.min() + 0.04
            aro = Vs[sel_s]
            if len(aro) < 6:
                continue
            ca = aro.mean(0)
            pol = poligono_por_angulo(aro[:, [0, 2]], ca[[0, 2]], bins=24)
            # de volta ao 3D: y pelo plano do aro
            Pa = []
            for q in pol:
                k = int(np.argmin(np.hypot(aro[:, 0] - q[0], aro[:, 2] - q[1])))
                Pa.append(aro[k])
            Pa = np.array(Pa)
            rad_a = normalizar(np.stack([Pa[:, 0] - ca[0], np.zeros(len(Pa)), Pa[:, 2] - ca[2]], 1))
            Pa = Pa + rad_a * 0.035 + nrm * 0.03
            Pa = arredondar(subdividir(Pa, 0.2, True), 0.3, True, passo=math.radians(10))
            L.tubo(Pa, normalizar(np.stack([Pa[:, 0] - ca[0], np.zeros(len(Pa)), Pa[:, 2] - ca[2]], 1)), 0.03,
                   ROSA, 0.30, True, 6, rotulo='saias rosa')
        # as molas e as estrelas (da rua para o prédio)
        for k, e in enumerate(sorted(minhas, key=lambda e: -e[2])):
            a0 = 0.84 + 0.04 * k
            poste = TF.filtro(caixa=(e[0] - 1.0, e[0] + 1.0, 7.5, 9.69, e[2] - 0.5, e[2] + 0.5))
            poste = poste.sub(~(((poste.UC[:, 0] >= UV_DO_CINZA[0]) & (poste.UC[:, 0] <= UV_DO_CINZA[1]) &
                                 (poste.UC[:, 1] >= UV_DO_CINZA[2]) & (poste.UC[:, 1] <= UV_DO_CINZA[3]))))
            Vq = poste.vertices()
            Vq = Vq[(np.abs(Vq[:, 2] - e[2]) < 0.45)]
            yq0 = Vq[:, 1].min()
            baixo = Vq[Vq[:, 1] < yq0 + 0.12]
            cima = Vq[Vq[:, 1] > 9.45]
            b0 = np.array([(baixo[:, 0].min() + baixo[:, 0].max()) / 2, baixo[:, 1].mean(), e[2]])
            b1 = np.array([(cima[:, 0].min() + cima[:, 0].max()) / 2, cima[:, 1].mean(), e[2]])
            eixo = normalizar(b1 - b0)
            rp = max(0.25, (baixo[:, 2].max() - baixo[:, 2].min()) / 2)
            e1 = normalizar(np.cross(eixo, [0, 0, 1.0]))
            e2 = np.cross(eixo, e1)
            comp = np.linalg.norm(b1 - b0)
            voltas = 3.25
            t = np.linspace(0, 1, int(voltas * 22) + 1)
            phi = 2 * np.pi * voltas * t
            R = rp + 0.13
            s = 0.18 + (comp - 0.30) * t
            P = b0 + s[:, None] * eixo + R * (np.cos(phi)[:, None] * e1 + np.sin(phi)[:, None] * e2)
            cor = np.array([np.interp(t, [0, 0.3, 0.6, 1.0], [c[i] for c in (VERMELHO, LARANJA, LARANJA, AMARELO)]) for i in range(3)]).T
            L.tubo(P, np.cos(phi)[:, None] * e1 + np.sin(phi)[:, None] * e2, 0.035, cor, a0 + 0.025 * t, False, 6, rotulo='molas')
            # a estrela: uma linha só, no plano da placa (a placa tem a estrela pintada nas duas faces; o tubo a
            # atravessa e aparece igual dos dois lados)
            st, fechada = estrela_2d(0.68, 0.36, 8, aberta_embaixo=0.62)
            st = arredondar(st, 0.04, fechada)
            P3, N3 = Plano(e, [1, 0, 0], [0, 1, 0], [0, 0, 1]).para_3d(st, 0.0)
            L.tubo(P3, N3, 0.032, AMARELO, a0 + 0.03, fechada, 8, rotulo='estrelas')
        print('  marquise %s: %d estrelas com mola' % ('oeste' if lado < 0 else 'leste', len(minhas)))

    # O letreiro do poste na beira da estrada (o oval, as letras, o V e o 8), dos dois lados
    oval = T71.filtro(uv=UV_DO_OVAL, caixa=(526, 532, 10, 16, 112, 127))
    for lado in (-1, 1):
        f = oval.sub(oval.N[:, 0] * lado > 0.9)
        Vo = f.G.reshape(-1, 3)
        xo = Vo[:, 0].mean()
        pol = casco_convexo(Vo[:, [2, 1]])
        c = pol.mean(0)
        # o oval por dentro da borda; o plano um pouco à frente das plaquinhas das letras
        letras = TF.filtro(caixa=(526, 532, 11, 15, 112, 127), normal=[lado, 0, 0], minimo=0.95)
        letras = letras.sub((np.abs(letras.G[:, :, 0].mean(1) - xo) > 0.03))     # as plaquinhas, à frente do oval
        x_letras = letras.G[..., 0].min() if lado < 0 else letras.G[..., 0].max()
        x_neon = x_letras + lado * 0.055
        ov = poligono_por_angulo(pol, c)
        ov = c + (ov - c) * (1 - 0.10 / np.linalg.norm(ov - c, axis=1))[:, None]
        ov = arredondar(subdividir(ov, 0.3, True), 0.6, True, passo=math.radians(8))
        P = np.stack([np.full(len(ov), x_neon), ov[:, 1], ov[:, 0]], 1)
        L.tubo(P, [lado, 0, 0], 0.035, CIANO, 0.50, True, 8, rotulo='poste')
        # as letras: as plaquinhas da frente deste lado (Flo's à esquerda, Cafe à direita de quem olha)
        for uvr, txt, alfas in (((725, 910, 60, 145), "Flo's", [0.52, 0.54, 0.56, 0.57, 0.58]),
                                ((495, 662, 285, 382), 'Cafe', [0.74, 0.77, 0.80, 0.83])):
            t = letras.filtro(uv=uvr)
            if len(t) == 0:
                continue
            sup = SuperficieUV(t, 'poste %s (%s)' % (txt, 'oeste' if lado < 0 else 'leste'))
            letras_neon(L, sup, txt, alfas, 0.04 * sup.largura, 0.06 * sup.altura, 0.96 * sup.largura,
                        0.94 * sup.altura, 0.05, 0.027, 'poste', afast=0.055)
    # o V e o 8 do poste
    pernas = TF.filtro(caixa=(527.5, 530, 10.5, 19, 116.5, 122.5), uv=(950, 1024, 0, 430))   # as pernas do V
    Vp_ = pernas.vertices()
    ponta_p = Vp_[np.argmin(Vp_[:, 1])]
    topo_p = Vp_[np.abs(Vp_[:, 1] - Vp_[:, 1].max()) < 0.01]
    topo_pe, topo_pd = topo_p[np.argmin(topo_p[:, 2])], topo_p[np.argmax(topo_p[:, 2])]
    forquilha_p = pernas.G[..., 1][pernas.N[:, 1] > 0.1].min() if (pernas.N[:, 1] > 0.1).any() else ponta_p[1] + 1.2
    xr0, xr1 = Vp_[:, 0].min(), Vp_[:, 0].max()
    # as bordas das pernas (as faces dos lados do prisma), de cada lado
    lados_v = TF.filtro(caixa=(527.5, 530, 10.5, 19, 116.5, 122.5), uv=(150, 180, 230, 390))
    xb = lados_v.G[..., 0]
    x_borda = {-1: xb.min(), 1: xb.max()}
    caixa8 = TF.filtro(uv=(390, 515, 0, 188), caixa=(527.5, 530, 14, 19.5, 117, 122))
    print('  V do poste: ponta y %.2f, forquilha y %.2f, topos z %.2f / %.2f em y %.2f; cristas x %.2f / %.2f, bordas x %.2f / %.2f' % (
        ponta_p[1], forquilha_p, topo_pe[2], topo_pd[2], topo_pe[1], xr0, xr1, x_borda[-1], x_borda[1]))
    for lado in (-1, 1):
        # o V no plano (z, y) do mundo, sobre as bordas das pernas (rosa) e as cristas (ciano)
        contorno, cristas = linha_v([ponta_p[2], ponta_p[1]], [topo_pe[2], topo_pe[1]], [topo_pd[2], topo_pd[1]], forquilha_p)
        x_crista = xr0 if lado < 0 else xr1
        P2 = arredondar(subdividir(np.array(contorno), 0.25, True), 0.05, True)
        P = np.stack([np.full(len(P2), x_borda[lado] + lado * 0.085), P2[:, 1], P2[:, 0]], 1)
        L.tubo(P, [lado, 0, 0], 0.03, ROSA, 0.64, True, 8, rotulo='poste')
        for cc in cristas:
            P2 = subdividir(cc, 0.25, False)
            P = np.stack([np.full(len(P2), x_crista + lado * 0.06), P2[:, 1], P2[:, 0]], 1)
            L.tubo(P, [lado, 0, 0], 0.028, CIANO, 0.645, False, 8, rotulo='poste')
        f = caixa8.sub(caixa8.N[:, 0] * lado > 0.9)
        V8 = f.G.reshape(-1, 3)
        x8 = (V8[:, 0].min() if lado < 0 else V8[:, 0].max()) + lado * 0.06
        z0, z1 = V8[:, 2].min(), V8[:, 2].max()
        y0, y1 = V8[:, 1].min(), V8[:, 1].max()
        mz, my = 0.06 * (z1 - z0), 0.04 * (y1 - y0)
        # o plano do 8: x para a direita de quem olha deste lado
        if lado < 0:
            pl = Plano([x8, 0, 0], [0, 0, 1], [0, 1, 0], [-1, 0, 0])
            fazer_oito(L, pl, z0 + mz, y0 + my, z1 - mz, y1 - my, 0.68, rotulo='poste')
        else:
            pl = Plano([x8, 0, 0], [0, 0, -1], [0, 1, 0], [1, 0, 0])
            fazer_oito(L, pl, -z1 + mz, y0 + my, -z0 - mz, y1 - my, 0.68, rotulo='poste')

    # As luzes que o Flo's joga em volta (eixos do jogo)
    cx, cz = centro
    luzes = [
        (np.array([cx, 9.6, cz + 15.0]), CIANO, 18.0, 1.6),
        (np.array([cx, 9.6, cz - 15.0]), CIANO, 14.0, 0.9),
        (np.array([ponta[0], 12.0, ponta[2] + 2.5]), ROSA, 12.0, 1.2),
        (np.array([cx, 3.8, cz + 10.4]), QUENTE, 9.0, 1.0),
    ]
    for (x, y, z) in luzes_marquise:
        luzes.append((np.array([x, y, z]), BRANCO_FRIO, 20.0, 1.3))
    for lado in (-1, 1):
        sel = (est[:, 0] < cx) if lado < 0 else (est[:, 0] > cx)
        m = est[sel].mean(0)
        luzes.append((np.array([m[0], 9.4, m[2]]), LARANJA, 11.0, 0.8))
    luzes.append((np.array([0.5 * (x_borda[-1] + x_borda[1]), ponta_p[1] - 0.4, ponta_p[2]]), CIANO, 15.0, 1.1))
    return L, luzes


def so_a_da_frente(t):
    """De duas camadas paralelas da mesma placa, fica a mais da frente"""
    n = normalizar(t.N.mean(0))
    d = t.C @ n
    return t.sub(d > d.max() - 0.03)


def letras_neon(L, sup, txt, alfas, x0, y0, x1, y1, dentro, raio, rotulo, afast=0.06):
    """Um texto de mão com contorno ciano e linha rosa por dentro, encaixado na caixa (metros da superfície)"""
    formas, _ = encaixar(texto(txt, FONTE_DE_MAO), x0, y0, x1, y1)
    cont = contornos(formas, [0.0, -dentro], px=0.006, tol=0.0045, raio_do_tubo=raio)
    L.neon(sup, cont[0.0], afast, raio, CIANO, ordem_por_letra(alfas), rotulo=rotulo)
    L.neon(sup, cont[-dentro], afast, max(0.025, raio * 0.85), ROSA, ordem_por_letra(np.array(alfas) + 0.01), rotulo=rotulo)


def fazer_oito(L, plano, x0, y0, x1, y1, alfa, rotulo='V8'):
    """O 8 (rosa por fora, ciano por dentro) e o emblema "(V)": os parênteses dos lados da volta de cima e o
    V pequeno dentro dela"""
    f, furo_c, volta_c = oito_2d(x0, y0, x1, y1)
    larg = x1 - x0
    raio = 0.035 if larg > 1.5 else 0.03
    dentro = 0.14 * larg / 2.5
    cont = contornos([(0, f)], [0.0, -dentro], px=0.008, tol=0.004, raio_do_tubo=raio, caixa=(x0, y0, x1, y1))
    L.neon(plano, cont[0.0], 0.0, raio, ROSA, ordem_fixa(alfa), lados=8, rotulo=rotulo)
    L.neon(plano, cont[-dentro], 0.0, max(0.025, raio * 0.85), CIANO, ordem_fixa(alfa + 0.01), lados=8, rotulo=rotulo)
    cx, cy, rx, ry = furo_c
    v = np.array([[cx - 0.45 * rx, cy + 0.42 * ry], [cx, cy - 0.55 * ry], [cx + 0.45 * rx, cy + 0.42 * ry]])
    v = arredondar(subdividir(v, 0.1, False), 0.04, False)
    P, N = plano.para_3d(v, 0.0)
    L.tubo(P, N, 0.025, CIANO, alfa + 0.02, False, 8, rotulo=rotulo)
    vx, vy, vrx, vry = volta_c
    for lado in (-1, 1):
        t = np.radians(np.linspace(-38, 38, 14)) + (np.pi if lado < 0 else 0)
        arco = np.stack([vx + (vrx + 0.22 * larg / 2.5) * np.cos(t), vy + (vry + 0.05) * np.sin(t)], 1)
        P, N = plano.para_3d(arco, 0.0)
        L.tubo(P, N, 0.025, CIANO, alfa + 0.02, False, 8, rotulo=rotulo)


# ------------------------------------------------------------------
# 8. As malhas, a exportação e a conferência
# ------------------------------------------------------------------
MATERIAL = bpy.data.materials.new('neon')
MATERIAL.diffuse_color = (1, 1, 1, 1)
if MATERIAL.node_tree is None:
    MATERIAL.use_nodes = True
for nd in MATERIAL.node_tree.nodes:
    if nd.type == 'BSDF_PRINCIPLED':
        nd.inputs['Base Color'].default_value = (1, 1, 1, 1)

MAX_VERTICES = 60000


def criar_objetos(L):
    """As peças em malhas de no máximo MAX_VERTICES vértices (a raylib usa índices de 16 bits)"""
    objs = []
    grupo, nv = [], 0

    def fechar():
        nonlocal grupo, nv
        if not grupo:
            return
        V, F, C, base = [], [], [], 0
        for v, f, c in grupo:
            V.append(v)
            F.append(f + base)
            C.append(c)
            base += len(v)
        V, F, C = np.concatenate(V), np.concatenate(F), np.concatenate(C)
        me = bpy.data.meshes.new('%s_%d' % (L.nome, len(objs)))
        Vb = b_de(V).astype(np.float32)
        me.vertices.add(len(Vb))
        me.vertices.foreach_set('co', Vb.ravel())
        me.loops.add(F.size)
        me.loops.foreach_set('vertex_index', F.astype(np.int32).ravel())
        me.polygons.add(len(F))
        me.polygons.foreach_set('loop_start', np.arange(0, F.size, 3, dtype=np.int32))
        me.update(calc_edges=True)
        at = me.color_attributes.new('Col', 'FLOAT_COLOR', 'POINT')
        at.data.foreach_set('color', C.astype(np.float32).ravel())
        me.color_attributes.active_color = at
        me.color_attributes.render_color_index = 0
        me.materials.append(MATERIAL)
        ob = bpy.data.objects.new(me.name, me)
        bpy.context.scene.collection.objects.link(ob)
        objs.append(ob)
        grupo, nv = [], 0

    for peca in L.pecas:
        if nv + len(peca[0]) > MAX_VERTICES:
            fechar()
        grupo.append(peca)
        nv += len(peca[0])
    fechar()
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
            passo = bv.get('byteStride', comps * np.dtype(dt).itemsize)
            bruto = np.frombuffer(d, np.uint8, count=acc['count'] * passo, offset=ini).reshape(acc['count'], passo)
            a = bruto[:, :comps * np.dtype(dt).itemsize].copy().view(dt).reshape(-1, comps) / esc
            todas.append(a)
    return np.concatenate(todas)


def exportar(L, luzes, nome_bonito, acender_em):
    objs = criar_objetos(L)
    esperado = np.concatenate([c for _, _, c in L.pecas])
    bpy.ops.object.select_all(action='DESELECT')
    for o in objs:
        o.select_set(True)
    caminho = os.path.join(PASTA_DE_SAIDA, L.nome + '.glb')
    bpy.ops.export_scene.gltf(filepath=caminho, export_format='GLB', use_selection=True, export_yup=True,
                              export_apply=True, export_animations=False, export_normals=False,
                              export_texcoords=False, export_vertex_color='ACTIVE', export_all_vertex_colors=False,
                              export_active_vertex_color_when_no_material=True, export_materials='EXPORT')
    j, d = ler_glb(caminho)
    tris = maior = sem_cor = 0
    tipos = set()
    nprim = 0
    for me in j['meshes']:
        for p in me['primitives']:
            nprim += 1
            if 'COLOR_0' not in p['attributes']:
                sem_cor += 1
                continue
            acc = j['accessors'][p['attributes']['COLOR_0']]
            tipos.add((acc['type'], acc['componentType']))
            tris += j['accessors'][p['indices']]['count'] // 3
            maior = max(maior, j['accessors'][p['attributes']['POSITION']]['count'])
    fatores = [m.get('pbrMetallicRoughness', {}).get('baseColorFactor', [1, 1, 1, 1]) for m in j.get('materials', [])]
    lido = cores_do_glb(j, d) if not sem_cor else np.zeros((1, 4))
    print('\nExportado %s' % caminho)
    print('  primitivas: %d (sem COLOR_0: %d; COLOR_0 %s); maior: %d vértices' % (nprim, sem_cor, sorted(tipos), maior))
    print('  triângulos: %d; materiais: %d; baseColorFactor %s; texturas: %d' % (
        tris, len(j.get('materials', [])), fatores, len(j.get('images', []))))
    print('  COLOR_0 lido do .glb: média %s; esperado %s' % (np.round(lido.mean(0), 4), np.round(esperado.mean(0), 4)))
    print('  alfa (ordem de acender): %.2f..%.2f' % (lido[:, 3].min(), lido[:, 3].max()))
    for k, v in sorted(L.contagem.items(), key=lambda kv: -kv[1]):
        print('    %-22s %7d triângulos' % (k, v))
    print('  tamanho: %.2f MB' % (os.path.getsize(caminho) / 1e6))
    if sem_cor:
        raise SystemExit('ERRO: há primitivas sem COLOR_0')
    if any(t != 'VEC4' for t, _ in tipos):
        raise SystemExit('ERRO: o COLOR_0 não saiu VEC4')
    if maior > 65535:
        raise SystemExit('ERRO: uma primitiva passou de 65535 vértices')
    if (np.abs(lido.mean(0) - esperado.mean(0)).max() > 0.01 or np.abs(lido.min(0) - esperado.min(0)).max() > 0.01 or
            np.abs(lido.max(0) - esperado.max(0)).max() > 0.01):
        raise SystemExit('ERRO: as cores lidas do .glb não batem com as do Blender (conversão de cor?)')
    dados = {'nome': nome_bonito, 'acender_em': acender_em, 'luzes': [
        {'x': round(float(p[0]), 2), 'y': round(float(p[1]), 2), 'z': round(float(p[2]), 2),
         'r': cor[0], 'g': cor[1], 'b': cor[2], 'raio': raio, 'forca': forca} for p, cor, raio, forca in luzes]}
    with open(os.path.join(PASTA_DE_SAIDA, L.nome + '.json'), 'w') as f:
        json.dump(dados, f, indent=2, ensure_ascii=False)
        f.write('\n')
    print('  %s.json: %d luzes, acende %.1f s depois da cidade' % (L.nome, len(luzes), acender_em))
    for o in objs:
        bpy.data.objects.remove(o, do_unlink=True)
    return dict(triangulos=tris, primitivas=nprim, maior=maior)


RAMONE, LUZES_RAMONE = montar_ramone()
FLO, LUZES_FLO = montar_flo()
exportar(RAMONE, LUZES_RAMONE, "Ramone's House of Body Art", 2.5)
exportar(FLO, LUZES_FLO, "Flo's V8 Café", 1.5)
print('\nPronto em %.1f s' % (time.time() - INICIO))
