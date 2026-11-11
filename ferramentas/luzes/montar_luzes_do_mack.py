"""
montar_luzes_do_mack.py
------------------------------------------------------------------
Monta as luzes de fora do Mack (o caminhão que leva o carro do jogador para
as corridas): as lâmpadas que acendem quando a cidade escurece, como as do
Mack de noite no filme Carros (2006). Fileiras de lâmpadas âmbar pequenas
ao longo da beira de cima da carreta (dos dois lados, na frente e atrás) e
do trilho de baixo dela, as luzes do teto da cabine, as dos degraus do
tanque e as dos para-lamas; os faróis e as lanternas da frente acesos; as
lanternas vermelhas atrás:

    Blender -b --factory-startup -P ferramentas/luzes/montar_luzes_do_mack.py -- \\
        recursos/modelos/mack.glb recursos/modelos [--previas pasta]

Grava, na pasta dada no segundo argumento:
  mack_luzes.glb   só as lâmpadas acesas, no formato das luzes da cidade
                   (luzes.c): um material branco, sem textura e sem normais; em
                   COLOR_0, o RGB é a cor acesa (na tela, 0 a 1) e o A é a ordem
                   de acender (0 primeiro, 1 por último);
  mack_luzes.json  o nome, quando o Mack acende (acender_em, em segundos) e as
                   luzes pontuais que as fileiras de baixo e os faróis jogam no
                   chão (x, y, z, cor, raio em metros e força).

Tudo no referencial do mack.glb, como a raylib o carrega: x para a esquerda do
caminhão, y para cima (as rodas em y = 0) e a frente para +z, com o meio do
caminhão na origem. O jogo desenha as lâmpadas com a mesma matriz do corpo do
Mack (matrizDoMack, em passeio.c), e elas andam com ele. A porta de trás
(mack_porta.glb, que desce e vira a rampa) fica sem lâmpada nenhuma, e as
lâmpadas ficam fora do caminho dela quando ela abre.

Com --previas, grava na pasta as vistas de conferência: de noite (o Mack
escuro na luz azulada da lua, como a noite do jogo, as lâmpadas acesas com o
brilho em volta e as luzes pontuais do .json no chão) e de dia (as lâmpadas
apagadas, como o jogo as mostra: a cor vezes 0,32), de lado e de 3/4.

O que ela faz:
  1. Lê o mack.glb e monta com ele uma árvore de raios (BVH). Cada lâmpada é
     pedida como um raio lançado de fora contra a lataria, no lugar e na
     direção em que ela vai: onde o raio bate é o pé da lâmpada, e a normal da
     face diz para onde ela olha. As fileiras:
       - a beira de cima da carreta: dos dois lados, 14 cm abaixo da quina do
         teto, em cima da faixa cinza pintada, da frente da carreta até a ponta
         de trás; na frente da carreta, a mesma altura;
       - atrás: a beira do teto, que passa da porta como uma marquise (uma
         faixa de 8,5 cm), com as lâmpadas vermelhas; nas pontas das abas de
         trás (as paredes que passam da porta), duas lanternas vermelhas em
         cápsula embaixo e uma lâmpada vermelha em cima;
       - o trilho de baixo da carreta: segue a beira de baixo da parede (o
         pescoço da carreta, a rampa dele, o trilho e a volta das caixas de
         rodas), sempre 15 cm para dentro dela: a beira é medida com raios de
         2 em 2 cm, e o caminho das lâmpadas é o ponto mais alto da beira numa
         janela de 15 cm para cada lado, mais 15 cm;
       - a cabine: as cinco caixinhas de luz que o modelo tem no teto ganham
         uma lente acesa na frente; as beiras de cima, dos dois lados, uma
         fileira;
       - os degraus do tanque (de cima e de baixo): uma lâmpada em cada lâmpada
         pintada na textura (de 45 em 45 cm);
       - os para-lamas da frente: uma fileira seguindo a curva, 12 cm abaixo
         da beira de cima, e uma lente redonda âmbar na frente da caixinha que
         fica em cima de cada um;
       - os faróis: duas lentes de branco quente em cada caixa de faróis, do
         tamanho dos faróis pintados; e as lanternas âmbar da frente da cabine,
         ao lado do capô.
  2. Assenta cada lâmpada e confere: a base dela fica a 1,5 cm da lataria
     (AFASTA), e de cada ponto da beira da base um raio para dentro tem de
     achar a lataria logo ali, entre 0,9 e 3,5 cm (a lâmpada não sobra para
     fora de uma quina nem entra na lataria); do meio da base até cada vértice
     da lâmpada, nada da lataria no caminho (nada encosta nela); fora das
     rodas (cilindros de mack_rodas.glb, que giram) e fora do caminho da porta
     de trás (o quarto de volta e pouco que ela dá em volta da dobradiça, como
     em passeio.c). A que não passa é empurrada de 2 em 2 cm pela fileira e
     para os lados (até 10 cm) até passar; a que não passa nunca fica de fora
     (e a ferramenta diz quais).
  3. As formas: as lâmpadas de contorno são meias-esferas de 4 a 5 cm de raio
     deitadas na lataria (na beira do teto atrás, que só tem 8,5 cm, meias-
     cápsulas deitadas de 3,8 cm; as lanternas de trás, meias-cápsulas em pé);
     os faróis, as lanternas da frente e as cinco do teto da cabine, lentes (um
     retângulo de cantos redondos ou um disco, 2 cm de espessura e a frente
     abaulada). Todas fechadas, com as faces viradas para fora (o jogo descarta
     as faces de trás).
  4. A ordem de acender (o A): os faróis primeiro (0), depois as fileiras
     correndo da frente da cabine até a traseira (o A vai com o z do modelo):
     as de cima e as de baixo, dos dois lados, juntas.
  5. Exporta só as lâmpadas, num material branco, com COLOR_0 em todas as
     primitivas (cada uma com menos de 60.000 vértices: a raylib usa índices
     de 16 bits), e confere no .glb gravado: as primitivas, os triângulos, o
     COLOR_0 com o A, e o referencial: os vértices lidos do .glb batem com os
     do Blender (x, z, -y), a caixa das lâmpadas cabe na do mack.glb, e o pé de
     cada lâmpada, medido contra a malha do mack.glb lida direto do arquivo
     (não pelo importador do Blender), está a 1,5 cm da lataria.

Eixos: ao importar o .glb, o Blender põe x = x do modelo, y = -z do modelo e
z = y do modelo; tudo aqui é medido assim, e o .glb sai com +Y para cima, de
volta às coordenadas do modelo.
------------------------------------------------------------------
"""

import bpy
import bmesh
import json
import math
import os
import struct
import sys
import numpy as np
from mathutils import Vector
from mathutils.bvhtree import BVHTree
from mathutils.kdtree import KDTree

bpy.ops.wm.read_factory_settings(use_empty=True)     # sem o cubo, a câmera e a luz da cena inicial
AQUI = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(AQUI))

# ------------------------------------------------------------------
# Argumentos
# ------------------------------------------------------------------
argv = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
opcoes, posicionais = {}, []
i = 0
while i < len(argv):
    if argv[i] == '--previas' and i + 1 < len(argv):
        opcoes['--previas'] = argv[i + 1]
        i += 2
    else:
        posicionais.append(argv[i])
        i += 1
ARQUIVO_DO_MACK = posicionais[0] if posicionais else os.path.join(REPO, 'recursos', 'modelos', 'mack.glb')
PASTA = posicionais[1] if len(posicionais) > 1 else os.path.dirname(ARQUIVO_DO_MACK)
PASTA_DO_MACK = os.path.dirname(os.path.abspath(ARQUIVO_DO_MACK))
ARQUIVO_DAS_RODAS = os.path.join(PASTA_DO_MACK, 'mack_rodas.glb')
ARQUIVO_DA_PORTA = os.path.join(PASTA_DO_MACK, 'mack_porta.glb')
PREVIAS = opcoes.get('--previas')
os.makedirs(PASTA, exist_ok=True)

# ------------------------------------------------------------------
# Medidas e cores
# ------------------------------------------------------------------
AFASTA = 0.015               # metros entre a lataria e a base da lâmpada
APOIO_ATE = 0.035            # a lataria tem de estar até aqui embaixo de cada ponto da beira da base...
APOIO_DESDE = 0.009          # ...e não mais perto que isso (senão a base entra nela)
RAIO = 0.05                  # as lâmpadas de contorno
RAIO_MENOR = 0.04            # onde a faixa é estreita (a beira do teto atrás, as abas)
ESPACO = 0.50                # metros entre as lâmpadas das fileiras compridas
RECUO_DA_BEIRA = 0.15        # o trilho de baixo: o caminho das lâmpadas fica isso para dentro da beira
EMPURRA = (0.02, 0.04, 0.06, 0.08, 0.10)   # quanto uma lâmpada que não assenta é empurrada (metros)
MAX_VERTICES = 60000         # por primitiva (a raylib usa índices de 16 bits)
ACENDER_EM = 1.0             # segundos depois da cidade

# A cor acesa (na tela, 0 a 1), como as das luzes da cidade
AMBAR = (1.00, 0.72, 0.20)           # as de contorno
AMBAR_DA_FRENTE = (1.00, 0.58, 0.10)  # as lanternas da frente (mais fundas, de pisca)
BRANCO_QUENTE = (1.00, 0.93, 0.78)    # os faróis
VERMELHO = (1.00, 0.11, 0.06)         # as lanternas de trás

# A ordem de acender vai da frente da cabine (z do modelo = +15,3) até a traseira (-15,3)
FRENTE_DO_MACK = -15.3       # y do Blender
TRAS_DO_MACK = 15.3


def ordem_pelo_lugar(y):
    """O A de uma lâmpada no y do Blender: 0,03 na frente da cabine, 0,98 na traseira"""
    t = (y - FRENTE_DO_MACK) / (TRAS_DO_MACK - FRENTE_DO_MACK)
    return 0.03 + 0.95 * min(max(t, 0.0), 1.0)


def J(p):
    """Do Blender para o modelo (o referencial do mack.glb na raylib)"""
    return (float(p[0]), float(p[2]), float(-p[1]))


# ------------------------------------------------------------------
# O Mack: a lataria (para os raios), as rodas e a porta
# ------------------------------------------------------------------
def importar(caminho):
    antes = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=caminho)
    return [o for o in bpy.data.objects if o not in antes and o.type == 'MESH']


def vertices_do_mundo(o):
    co = np.empty(len(o.data.vertices) * 3)
    o.data.vertices.foreach_get('co', co)
    co = co.reshape(-1, 3)
    M = np.array(o.matrix_world)
    return co @ M[:3, :3].T + M[:3, 3]


def arvore_de(objs):
    verts, faces = [], []
    for o in objs:
        base = len(verts)
        verts.extend(Vector(v) for v in vertices_do_mundo(o))
        faces.extend([base + k for k in p.vertices] for p in o.data.polygons)
    return BVHTree.FromPolygons(verts, faces, all_triangles=False), len(faces)


print('Lendo %s' % ARQUIVO_DO_MACK)
CORPO = importar(ARQUIVO_DO_MACK)
LATARIA, FACES_DA_LATARIA = arvore_de(CORPO)
caixa = np.concatenate([vertices_do_mundo(o) for o in CORPO])
CAIXA_DO_CORPO = (caixa.min(0), caixa.max(0))
print('  %d malhas, %d faces; caixa (Blender) %s a %s' % (len(CORPO), FACES_DA_LATARIA,
                                                          np.round(CAIXA_DO_CORPO[0], 3), np.round(CAIXA_DO_CORPO[1], 3)))

# As rodas giram em volta do eixo delas (x): cada uma ocupa um cilindro
RODAS = []
if os.path.exists(ARQUIVO_DAS_RODAS):
    for o in importar(ARQUIVO_DAS_RODAS):
        v = vertices_do_mundo(o)
        mn, mx = v.min(0), v.max(0)
        meio = (mn + mx) / 2
        raio = float(np.max(np.hypot(v[:, 1] - meio[1], v[:, 2] - meio[2])))
        RODAS.append((mn[0], mx[0], meio[1], meio[2], raio))
        o.hide_render = True
print('  %d rodas' % len(RODAS))

# A porta de trás gira em volta da dobradiça (o pé dela, no eixo x) até virar a rampa (passeio.c)
PORTA = None
if os.path.exists(ARQUIVO_DA_PORTA):
    objs_da_porta = importar(ARQUIVO_DA_PORTA)
    v = np.concatenate([vertices_do_mundo(o) for o in objs_da_porta])
    mn, mx = v.min(0), v.max(0)
    dobradica_y, dobradica_z = (mn[1] + mx[1]) / 2, mn[2]
    altura = mx[2] - mn[2]
    alcance = float(np.max(np.hypot(v[:, 1] - dobradica_y, v[:, 2] - dobradica_z)))
    giro_maximo = math.pi / 2 + math.asin(min(max(dobradica_z / max(altura, 0.1), 0.0), 0.9))
    PORTA = dict(xmin=mn[0], xmax=mx[0], y=dobradica_y, z=dobradica_z, alcance=alcance, giro=giro_maximo,
                 caixa=(mn, mx))
    for o in objs_da_porta:
        o.hide_render = True
    print('  porta: x %.3f a %.3f, dobradiça em y %.3f z %.3f, alcance %.3f m, gira até %.1f°' % (
        mn[0], mx[0], dobradica_y, dobradica_z, alcance, math.degrees(giro_maximo)))


def raio_na_lataria(origem, direcao, alcance=8.0):
    """O primeiro ponto da lataria no raio: (lugar, normal virada para o raio, distância) ou None"""
    d = Vector(direcao).normalized()
    lugar, normal, _, dist = LATARIA.ray_cast(Vector(origem), d, alcance)
    if lugar is None:
        return None
    if normal.dot(d) > 0:
        normal = -normal
    return lugar, normal.normalized(), dist


# ------------------------------------------------------------------
# As formas
# ------------------------------------------------------------------
def cupula(base, N, E, raio, comprimento=0.0, gomos=6, fatias=8):
    """
    Meia-esfera deitada (ou meia-cápsula, com comprimento ao longo de E): a base no plano que passa
    por 'base', a cúpula para o lado de N. Os anéis são meios círculos em volta do eixo E (de um polo,
    em +E, ao outro, em -E); numa cápsula, o anel do meio se repete, um em cada ponta do cilindro.
    Devolve: os vértices, as faces, a beira da base (índices) e, para cada vértice, o ponto do eixo de
    onde ele é visto (para conferir que nada da lataria entra na lâmpada).
    """
    S = N.cross(E).normalized()
    meio = comprimento / 2.0
    verts, origens, aneis = [], [], []
    for k in range(fatias + 1):
        th = math.pi * k / fatias
        ce, se = math.cos(th), math.sin(th)
        if abs(ce) > 1e-6:
            desvios = [meio if ce > 0 else -meio]
        else:
            desvios = [meio, -meio] if meio > 0 else [0.0]
        for d in desvios:
            centro = base + E * d
            if se < 1e-6:
                aneis.append([len(verts)])
                verts.append(centro + E * (ce * raio))
                origens.append(centro)
                continue
            anel = []
            for j in range(gomos + 1):
                ph = math.pi * j / gomos
                anel.append(len(verts))
                verts.append(centro + E * (ce * raio) + (S * math.cos(ph) + N * math.sin(ph)) * (se * raio))
                origens.append(centro)
            aneis.append(anel)
    faces = []
    for a, b in zip(aneis[:-1], aneis[1:]):
        if len(a) == 1:
            faces += [(a[0], b[j], b[j + 1]) for j in range(gomos)]
        elif len(b) == 1:
            faces += [(a[j], a[j + 1], b[0]) for j in range(gomos)]
        else:
            faces += [(a[j], a[j + 1], b[j + 1], b[j]) for j in range(gomos)]
    # a base: a beira (o lado de S, de um polo ao outro, e o lado de -S de volta) e o meio
    beira = [aneis[0][0]] + [a[0] for a in aneis[1:-1]] + [aneis[-1][0]] + [a[-1] for a in reversed(aneis[1:-1])]
    c = len(verts)
    verts.append(base.copy())
    origens.append(base.copy())
    faces += [(c, beira[(j + 1) % len(beira)], beira[j]) for j in range(len(beira))]
    return verts, faces, beira, origens


def lente(base, N, U, largura, altura, espessura=0.02, abaulado=0.012, expoente=4.0, pontos=28):
    """
    Uma lente: o contorno é uma superelipse (expoente 2: um disco ou uma elipse; 4: um retângulo de
    cantos redondos) de largura x altura no plano de 'base' (U para cima), com 'espessura' e a frente
    abaulada. Devolve como cupula().
    """
    V = U.cross(N).normalized()
    U = N.cross(V).normalized()

    def contorno(escala, h):
        pts = []
        for k in range(pontos):
            a = 2 * math.pi * k / pontos
            c, s = math.cos(a), math.sin(a)
            x = math.copysign(abs(c) ** (2.0 / expoente), c) * largura / 2 * escala
            y = math.copysign(abs(s) ** (2.0 / expoente), s) * altura / 2 * escala
            pts.append(base + V * x + U * y + N * h)
        return pts

    verts, faces = [], []
    tras = contorno(1.0, 0.0)
    verts += tras
    centro_de_tras = len(verts)
    verts.append(base.copy())
    aneis = [list(range(0, pontos))]
    for escala in (1.0, 0.75, 0.45):
        h = espessura + abaulado * (1.0 - escala * escala)
        inicio = len(verts)
        verts += contorno(escala, h)
        aneis.append(list(range(inicio, inicio + pontos)))
    frente = len(verts)
    verts.append(base + N * (espessura + abaulado))
    faces += [(centro_de_tras, (k + 1) % pontos, k) for k in range(pontos)]
    for a, b in zip(aneis[:-1], aneis[1:]):
        faces += [(a[k], a[(k + 1) % pontos], b[(k + 1) % pontos], b[k]) for k in range(pontos)]
    ultimo = aneis[-1]
    faces += [(ultimo[k], ultimo[(k + 1) % pontos], frente) for k in range(pontos)]
    origens = [base.copy() for _ in verts]
    return verts, faces, list(range(0, pontos)), origens


# ------------------------------------------------------------------
# Assentar uma lâmpada na lataria
# ------------------------------------------------------------------
def problema(verts, beira, origens, base, N):
    """O que impede a lâmpada de ficar aí (None se nada)"""
    # 1) a base apoiada: debaixo de cada ponto da beira, e do meio, a lataria logo ali
    for p in [verts[k] for k in beira] + [base]:
        r = raio_na_lataria(p + N * 0.05, -N, 0.05 + APOIO_ATE + 0.01)
        if r is None or r[2] - 0.05 > APOIO_ATE:
            return 'sobra da quina'
        if r[2] - 0.05 < APOIO_DESDE:
            return 'entra na lataria'
    # 2) nada da lataria dentro dela: do eixo até cada vértice
    for v, o in zip(verts, origens):
        d = v - o
        L = d.length
        if L < 1e-6:
            continue
        if LATARIA.ray_cast(o, d / L, L + 0.004)[0] is not None:
            return 'encosta na lataria'
    # 3) fora das rodas (o cilindro de cada uma, que gira)
    for x0, x1, cy, cz, rr in RODAS:
        for v in verts:
            if x0 - 0.01 <= v.x <= x1 + 0.01 and math.hypot(v.y - cy, v.z - cz) < rr + 0.01:
                return 'encosta numa roda'
    # 4) fora da porta e do caminho dela até virar rampa
    if PORTA is not None:
        for v in verts:
            if not (PORTA['xmin'] - 0.01 <= v.x <= PORTA['xmax'] + 0.01):
                continue
            dy, dz = v.y - PORTA['y'], v.z - PORTA['z']
            giro = math.atan2(dy, dz)            # 0: a porta fechada (em pé); cresce abrindo, para trás
            if -0.03 <= giro <= PORTA['giro'] + 0.03 and math.hypot(dy, dz) < PORTA['alcance'] + 0.003:
                return 'no caminho da porta'
    return None


def montar_forma(pedido, lugar, N):
    T = Vector(pedido['ao_longo'])
    T = (T - N * T.dot(N))
    if T.length < 1e-6:
        T = N.orthogonal()
    T.normalize()
    base = lugar + N * AFASTA
    if pedido['forma'] == 'cupula':
        return base, cupula(base, N, T, pedido.get('raio', RAIO))
    if pedido['forma'] == 'capsula':
        return base, cupula(base, N, T, pedido.get('raio', RAIO), pedido['comprimento'])
    U = Vector(pedido.get('cima', (0, 0, 1)))
    U = U - N * U.dot(N)
    return base, lente(base, N, U.normalized(), pedido['largura'], pedido['altura'],
                       expoente=pedido.get('expoente', 4.0))


def assentar(pedido, recusas):
    """
    Lança o raio do pedido; se a lâmpada não assenta, empurra a origem pela fileira (ao_longo) e para
    o lado (de 2 em 2 cm, até 10 cm) e tenta de novo. Devolve a lâmpada (dicionário) ou None.
    """
    origem, direcao = Vector(pedido['origem']), Vector(pedido['direcao']).normalized()
    T = Vector(pedido['ao_longo'])
    T = (T - direcao * T.dot(direcao)).normalized()
    B = direcao.cross(T).normalized()
    tentativas = [(0.0, 0.0)]
    for e in EMPURRA:
        tentativas += [(e, 0.0), (-e, 0.0), (0.0, e), (0.0, -e)]
    motivo = 'o raio não bate'
    for dt, db in tentativas:
        o = origem + T * dt + B * db
        r = raio_na_lataria(o, direcao, pedido.get('alcance', 8.0))
        if r is None:
            continue
        lugar, N, _ = r
        if not pedido['aceita'](lugar):
            motivo = 'bate em outra peça (%.2f, %.2f, %.2f)' % tuple(lugar)
            continue
        base, (verts, faces, beira, origens) = montar_forma(pedido, lugar, N)
        m = problema(verts, beira, origens, base, N)
        if m is None:
            return dict(grupo=pedido['grupo'], cor=pedido['cor'], ordem=pedido.get('ordem', ordem_pelo_lugar(base.y)),
                        base=base, normal=N, verts=verts, faces=faces, empurrada=(dt, db))
        motivo = m
    recusas.append((pedido['grupo'], tuple(round(c, 2) for c in origem), motivo))
    return None


# ------------------------------------------------------------------
# As fileiras (cada lâmpada é um pedido: de onde vem o raio, para onde vai e o que ele tem de achar)
# ------------------------------------------------------------------
def entre(a, b, n):
    return [a + (b - a) * k / (n - 1) for k in range(n)] if n > 1 else [(a + b) / 2]


def quantas(comprimento, espaco=ESPACO):
    return max(2, int(round(comprimento / espaco)) + 1)


def parede_da_carreta(lado, y, z):
    """Há parede de fora da carreta (|x| > 2) em (y, z), do lado dado?"""
    r = raio_na_lataria((lado * 3.0, y, z), (-lado, 0, 0), 1.2)
    return r is not None and abs(r[0].x) > 2.0


def caminho_pela_beira(ys, beira, recuo, de_baixo, espaco=ESPACO):
    """
    O caminho das lâmpadas ao longo de uma beira (z da beira para cada y): o ponto mais alto (de_baixo)
    ou mais baixo da beira numa janela de 'recuo' para cada lado, mais (ou menos) o recuo. Assim cada
    lâmpada fica a pelo menos 'recuo' da beira, na horizontal ou na vertical. Devolve os pontos (y, z)
    espaçados de 'espaco' (o mais perto disso que dê um número inteiro), com a direção do caminho em cada
    um, e o comprimento do caminho.
    """
    passo = ys[1] - ys[0]
    w = int(math.ceil(recuo / passo))
    z = np.empty_like(beira)
    for k in range(len(ys)):
        janela = beira[max(0, k - w):k + w + 1]
        z[k] = (np.nanmax(janela) + recuo) if de_baixo else (np.nanmin(janela) - recuo)
    ok = ~np.isnan(z)
    ys, z = ys[ok], z[ok]
    seg = np.hypot(np.diff(ys), np.diff(z))
    s = np.concatenate([[0.0], np.cumsum(seg)])
    n = quantas(s[-1], espaco)
    alvos = np.linspace(0.0, s[-1], n)
    pontos = []
    for a in alvos:
        y = float(np.interp(a, s, ys))
        zz = float(np.interp(a, s, z))
        y0, z0 = float(np.interp(max(a - 0.05, 0.0), s, ys)), float(np.interp(max(a - 0.05, 0.0), s, z))
        y1, z1 = float(np.interp(min(a + 0.05, s[-1]), s, ys)), float(np.interp(min(a + 0.05, s[-1]), s, z))
        pontos.append((y, zz, (0.0, y1 - y0, z1 - z0)))
    return pontos, s[-1]


def beira_de_baixo_da_carreta(lado, y0, y1):
    """A altura da beira de baixo da parede da carreta, de 2 em 2 cm (nan onde não há parede)"""
    ys = np.arange(y0, y1 + 1e-9, 0.02)
    beira = np.full(len(ys), np.nan)
    for k, y in enumerate(ys):
        achou = None
        for z in np.arange(0.40, 3.60, 0.04):
            if parede_da_carreta(lado, y, z):
                achou = z
                break
        if achou is None:
            continue
        baixo, alto = achou - 0.04, achou
        for _ in range(6):
            meio = (baixo + alto) / 2
            if parede_da_carreta(lado, y, meio):
                alto = meio
            else:
                baixo = meio
        beira[k] = alto
    return ys, beira


def beira_de_cima_do_para_lama(lado, y0, y1):
    """A altura da beira de cima do para-lama (de cima para baixo, fora da caixinha da lanterna)"""
    ys = np.arange(y0, y1 + 1e-9, 0.02)
    beira = np.full(len(ys), np.nan)
    for k, y in enumerate(ys):
        r = raio_na_lataria((lado * 1.85, y, 3.6), (0, 0, -1), 2.0)
        if r is not None and 1.7 < r[0].z < 2.95:
            beira[k] = r[0].z
    return ys, beira


def pedidos():
    P = []
    for lado in (1, -1):
        L = 'esquerda' if lado > 0 else 'direita'   # o +x do modelo é a esquerda do caminhão

        # A beira de cima da carreta, de lado: da frente da carreta até a ponta de trás
        for y in entre(-6.90, 15.10, quantas(15.10 + 6.90)):
            P.append(dict(grupo='carreta: beira de cima (%s)' % L, forma='cupula', cor=AMBAR,
                          origem=(lado * 3.5, y, 6.82), direcao=(-lado, 0, 0), ao_longo=(0, 1, 0),
                          aceita=lambda h: 2.05 < abs(h.x) < 2.20))

        # O trilho de baixo da carreta: o pescoço, a rampa dele, o trilho e a volta das caixas de rodas
        ys, beira = beira_de_baixo_da_carreta(lado, -6.95, 14.48)
        caminho, comprimento = caminho_pela_beira(ys, beira, RECUO_DA_BEIRA, de_baixo=True)
        print('  trilho de baixo (%s): %.1f m de caminho, %d lâmpadas' % (L, comprimento, len(caminho)))
        for y, z, t in caminho:
            P.append(dict(grupo='carreta: trilho de baixo (%s)' % L, forma='cupula', cor=AMBAR,
                          origem=(lado * 3.5, y, z), direcao=(-lado, 0, 0), ao_longo=t,
                          aceita=lambda h: 2.0 < abs(h.x) < 2.20))

        # As abas de trás (as paredes que passam da porta): as lanternas embaixo e uma lâmpada em cima
        for z in (1.02, 1.30):
            P.append(dict(grupo='traseira: lanternas', forma='capsula', cor=VERMELHO, raio=0.042, comprimento=0.14,
                          origem=(lado * 2.04, 17.0, z), direcao=(0, -1, 0), ao_longo=(0, 0, 1),
                          aceita=lambda h: 14.55 < h.y < 14.70))
        P.append(dict(grupo='traseira: pontas das abas', forma='cupula', cor=VERMELHO, raio=RAIO_MENOR,
                      origem=(lado * 2.04, 17.0, 6.35), direcao=(0, -1, 0), ao_longo=(0, 0, 1),
                      aceita=lambda h: 14.9 < h.y < 15.3))

        # A cabine: a beira de cima, de lado
        for y in entre(-11.35, -8.85, 6):
            P.append(dict(grupo='cabine: beira do teto (%s)' % L, forma='cupula', cor=AMBAR,
                          origem=(lado * 2.6, y, 6.80), direcao=(-lado, 0, 0), ao_longo=(0, 1, 0),
                          aceita=lambda h: 1.1 < abs(h.x) < 1.35))

        # Os degraus do tanque: em cima de cada lâmpada pintada na textura
        for y in (-11.56, -11.11, -10.66, -10.21, -9.76, -9.30):
            P.append(dict(grupo='tanque: degrau de cima (%s)' % L, forma='cupula', cor=AMBAR,
                          origem=(lado * 3.0, y, 2.25), direcao=(-lado, 0, 0), ao_longo=(0, 1, 0),
                          aceita=lambda h: 2.08 < abs(h.x) < 2.15))
            P.append(dict(grupo='tanque: degrau de baixo (%s)' % L, forma='cupula', cor=AMBAR,
                          origem=(lado * 3.0, y, 1.33), direcao=(-lado, 0, 0), ao_longo=(0, 1, 0),
                          aceita=lambda h: 2.2 < abs(h.x) < 2.3))

        # O para-lama da frente: uma fileira seguindo a curva, abaixo da beira de cima
        ys, beira = beira_de_cima_do_para_lama(lado, -15.05, -12.35)
        caminho, _ = caminho_pela_beira(ys, beira, 0.12, de_baixo=False, espaco=0.55)
        for y, z, t in caminho:
            P.append(dict(grupo='para-lama (%s)' % L, forma='cupula', cor=AMBAR,
                          origem=(lado * 3.0, y, z), direcao=(-lado, 0, 0), ao_longo=t,
                          aceita=lambda h: 1.8 < abs(h.x) < 1.97))
        # e a lanterna redonda na frente da caixinha em cima dele
        P.append(dict(grupo='frente: lanternas dos para-lamas', forma='lente', cor=AMBAR_DA_FRENTE,
                      largura=0.28, altura=0.28, expoente=2.0,
                      origem=(lado * 1.53, -16.0, 3.062), direcao=(0, 1, 0), ao_longo=(1, 0, 0),
                      aceita=lambda h: -14.42 < h.y < -14.30))
        # as lanternas da frente da cabine, ao lado do capô
        P.append(dict(grupo='frente: lanternas da cabine', forma='lente', cor=AMBAR_DA_FRENTE,
                      largura=0.15, altura=0.46, expoente=4.0,
                      origem=(lado * 1.442, -12.6, 3.515), direcao=(0, 1, 0), ao_longo=(1, 0, 0),
                      aceita=lambda h: -10.85 < h.y < -10.70))
        # os faróis: dois em cada caixa
        for x in (1.33, 1.63):
            P.append(dict(grupo='frente: faróis', forma='lente', cor=BRANCO_QUENTE, ordem=0.0,
                          largura=0.235, altura=0.17, expoente=5.0,
                          origem=(lado * x, -16.0, 2.332), direcao=(0, 1, 0), ao_longo=(1, 0, 0),
                          aceita=lambda h: -15.2 < h.y < -15.1))

    # Na frente da carreta, a mesma beira de cima (atrás da cabine; as do meio ficam na frente da caixa
    # que sai 16 cm da frente da carreta, entre x = -0,55 e 0,55)
    for x in entre(-1.84, 1.84, 9):
        P.append(dict(grupo='carreta: beira de cima (frente)', forma='cupula', cor=AMBAR,
                      origem=(x, -7.6, 6.82), direcao=(0, 1, 0), ao_longo=(1, 0, 0),
                      aceita=lambda h: -7.35 < h.y < -7.0))

    # Atrás: a beira do teto, que passa da porta como uma marquise
    for x in entre(-1.62, 1.62, 7):
        P.append(dict(grupo='traseira: beira do teto', forma='capsula', cor=VERMELHO, raio=0.038, comprimento=0.06,
                      origem=(x, 17.0, 6.9025), direcao=(0, -1, 0), ao_longo=(1, 0, 0),
                      aceita=lambda h: h.y > 15.2))

    # As cinco caixinhas de luz no teto da cabine: uma lente na frente de cada uma (a frente delas é um
    # trapézio de 9 a 19 cm; a lente fica dentro dele)
    for x, z, largura, altura in ((-0.86, 7.228, 0.065, 0.06), (-0.352, 7.29, 0.065, 0.065), (0.007, 7.28, 0.08, 0.08),
                                  (0.357, 7.29, 0.065, 0.065), (0.86, 7.228, 0.065, 0.06)):
        P.append(dict(grupo='cabine: luzes do teto', forma='lente', cor=AMBAR,
                      largura=largura, altura=altura, expoente=4.0,
                      origem=(x, -12.6, z), direcao=(0, 1, 0), ao_longo=(1, 0, 0),
                      aceita=lambda h: -11.3 < h.y < -10.9 and h.z > 7.15))
    return P


# ------------------------------------------------------------------
# As malhas e a exportação
# ------------------------------------------------------------------
def material_da_luz():
    m = bpy.data.materials.new('luz_do_mack')
    m.use_nodes = True
    bsdf = next(n for n in m.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    bsdf.inputs['Base Color'].default_value = (1, 1, 1, 1)
    return m


def objetos_das_lampadas(lampadas, mat):
    """Malhas de até MAX_VERTICES vértices; a cor acesa e a ordem em cada vértice; as faces para fora"""
    lotes, lote, n = [], [], 0
    for lp in lampadas:
        if n + len(lp['verts']) > MAX_VERTICES:
            lotes.append(lote)
            lote, n = [], 0
        lote.append(lp)
        n += len(lp['verts'])
    if lote:
        lotes.append(lote)
    objs = []
    for k, lote in enumerate(lotes):
        verts, faces, cores = [], [], []
        for lp in lote:
            b = len(verts)
            verts += [tuple(v) for v in lp['verts']]
            faces += [tuple(b + i for i in f) for f in lp['faces']]
            cores += [(lp['cor'][0], lp['cor'][1], lp['cor'][2], lp['ordem'])] * len(lp['verts'])
        me = bpy.data.meshes.new('mack_luzes_%d' % k)
        me.from_pydata(verts, [], faces)
        me.update()
        # cada lâmpada é fechada: as normais (a ordem dos cantos) viram para fora, peça por peça
        bm = bmesh.new()
        bm.from_mesh(me)
        bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
        bm.to_mesh(me)
        bm.free()
        at = me.color_attributes.new('Col', 'FLOAT_COLOR', 'POINT')
        at.data.foreach_set('color', np.array(cores, dtype=np.float32).ravel())
        me.color_attributes.active_color = at
        me.color_attributes.render_color_index = me.color_attributes.active_color_index
        me.materials.append(mat)
        o = bpy.data.objects.new('mack_luzes_%d' % k, me)
        bpy.context.scene.collection.objects.link(o)
        objs.append(o)
    return objs


def ler_glb(caminho):
    d = open(caminho, 'rb').read()
    n = struct.unpack('<I', d[12:16])[0]
    return json.loads(d[20:20 + n]), d, 20 + n + 8


def acessor(j, d, base, indice):
    acc = j['accessors'][indice]
    bv = j['bufferViews'][acc['bufferView']]
    dt = {5121: np.uint8, 5123: np.uint16, 5125: np.uint32, 5126: np.float32}[acc['componentType']]
    comps = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3, 'VEC4': 4}[acc['type']]
    tam = np.dtype(dt).itemsize
    ini = base + bv.get('byteOffset', 0) + acc.get('byteOffset', 0)
    passo = bv.get('byteStride', comps * tam)
    if passo == comps * tam:
        a = np.frombuffer(d, dt, count=acc['count'] * comps, offset=ini).reshape(-1, comps)
    else:
        a = np.array([np.frombuffer(d, dt, count=comps, offset=ini + k * passo) for k in range(acc['count'])])
    if acc.get('normalized'):
        a = a / float(np.iinfo(dt).max)
    return a.astype(np.float64)


def malha_do_glb(caminho):
    """Os triângulos de um .glb, lidos direto do arquivo (com a matriz de cada nó, se houver)"""
    j, d, base = ler_glb(caminho)
    verts, tris = [], []
    for no in j['nodes']:
        if 'mesh' not in no:
            continue
        M = np.eye(4)
        if 'matrix' in no:
            M = np.array(no['matrix']).reshape(4, 4).T
        elif any(k in no for k in ('translation', 'rotation', 'scale')):
            raise SystemExit('ERRO: %s tem nós com transformação (a conferência não sabe ler)' % caminho)
        for p in j['meshes'][no['mesh']]['primitives']:
            pos = acessor(j, d, base, p['attributes']['POSITION'])
            pos = pos @ M[:3, :3].T + M[:3, 3]
            ind = acessor(j, d, base, p['indices']).astype(np.int64).reshape(-1, 3)
            tris.append(ind + len(verts))
            verts.extend(pos)
    return np.array(verts), np.concatenate(tris)


def conferir_glb(caminho, objs, lampadas):
    j, d, base = ler_glb(caminho)
    tris, prims, maior, sem_cor, tipos = 0, 0, 0, 0, set()
    pos_glb, cor_glb = [], []
    for me in j['meshes']:
        for p in me['primitives']:
            prims += 1
            tris += j['accessors'][p['indices']]['count'] // 3
            maior = max(maior, j['accessors'][p['attributes']['POSITION']]['count'])
            pos_glb.append(acessor(j, d, base, p['attributes']['POSITION']))
            if 'COLOR_0' not in p['attributes']:
                sem_cor += 1
                continue
            acc = j['accessors'][p['attributes']['COLOR_0']]
            tipos.add((acc['type'], acc['componentType']))
            cor_glb.append(acessor(j, d, base, p['attributes']['COLOR_0']))
    pos_glb = np.concatenate(pos_glb)
    fatores = [m.get('pbrMetallicRoughness', {}).get('baseColorFactor', [1, 1, 1, 1]) for m in j.get('materials', [])]
    print('Exportado %s' % caminho)
    print('  %d triângulos, %d primitivas (sem COLOR_0: %d; COLOR_0 %s), maior: %d vértices' % (
        tris, prims, sem_cor, sorted(tipos), maior))
    print('  materiais: %s, imagens: %d, normais: %s' % (
        fatores, len(j.get('images', [])), any('NORMAL' in p['attributes'] for m in j['meshes'] for p in m['primitives'])))
    if sem_cor:
        raise SystemExit('ERRO: há primitivas sem COLOR_0')
    if not all(t[0] == 'VEC4' for t in tipos):
        raise SystemExit('ERRO: COLOR_0 sem o A (a ordem de acender)')
    if maior > 65535:
        raise SystemExit('ERRO: primitiva com mais de 65535 vértices')
    cor_glb = np.concatenate(cor_glb)
    print('  COLOR_0 lido: A de %.3f a %.3f; %d cores acesas diferentes' % (
        cor_glb[:, 3].min(), cor_glb[:, 3].max(), len(np.unique(np.round(cor_glb[:, :3], 3), axis=0))))

    # O referencial: os vértices do .glb são os do Blender passados para o modelo (x, z, -y)
    arvore = KDTree(len(pos_glb))
    for k, p in enumerate(pos_glb):
        arvore.insert(p, k)
    arvore.balance()
    pior = 0.0
    for o in objs:
        for v in vertices_do_mundo(o):
            pior = max(pior, arvore.find(J(v))[2])
    print('  referencial: cada vértice do Blender (x, z, -y) achado no .glb a no máximo %.2e m' % pior)
    if pior > 1e-4:
        raise SystemExit('ERRO: o .glb não está no referencial do modelo')

    # A caixa das lâmpadas e a do mack.glb, as duas lidas dos arquivos
    verts_do_mack, tris_do_mack = malha_do_glb(ARQUIVO_DO_MACK)
    cm, cM = verts_do_mack.min(0), verts_do_mack.max(0)
    lm, lM = pos_glb.min(0), pos_glb.max(0)
    print('  caixa do mack.glb:      %s a %s' % (np.round(cm, 3), np.round(cM, 3)))
    print('  caixa de mack_luzes.glb: %s a %s' % (np.round(lm, 3), np.round(lM, 3)))
    if np.any(lm < cm - 0.12) or np.any(lM > cM + 0.12):
        raise SystemExit('ERRO: as lâmpadas saem da caixa do Mack')

    # O pé de cada lâmpada contra a malha do mack.glb lida do arquivo (no referencial do modelo)
    arv = BVHTree.FromPolygons([Vector(v) for v in verts_do_mack], tris_do_mack.tolist())
    dist = []
    for lp in lampadas:
        dist.append(arv.find_nearest(Vector(J(lp['base'])))[3])
    dist = np.array(dist)
    print('  base de cada lâmpada até a lataria do mack.glb: %.4f a %.4f m (mediana %.4f)' % (
        dist.min(), dist.max(), np.median(dist)))
    if dist.min() < APOIO_DESDE or dist.max() > AFASTA + 0.01:
        raise SystemExit('ERRO: alguma lâmpada fora da lataria do mack.glb')
    return tris, prims, maior


def exportar(objs, caminho, lampadas):
    bpy.ops.object.select_all(action='DESELECT')
    for o in objs:
        o.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]
    bpy.ops.export_scene.gltf(filepath=caminho, export_format='GLB', use_selection=True, export_yup=True,
                              export_apply=True, export_animations=False, export_normals=False,
                              export_texcoords=False, export_vertex_color='ACTIVE', export_all_vertex_colors=False,
                              export_active_vertex_color_when_no_material=True, export_materials='EXPORT')
    return conferir_glb(caminho, objs, lampadas)


# ------------------------------------------------------------------
# As luzes pontuais (no referencial do modelo)
# ------------------------------------------------------------------
def luzes_pontuais():
    luzes = []

    def luz(p, cor, raio, forca):
        x, y, z = J(p)
        luzes.append({'x': round(x, 3), 'y': round(y, 3), 'z': round(z, 3), 'r': cor[0], 'g': cor[1], 'b': cor[2],
                      'raio': raio, 'forca': forca})

    for lado in (1, -1):
        # as fileiras de baixo jogam âmbar no chão e na parede, uns 80 cm para fora da lataria
        luz((lado * 3.0, -10.4, 1.5), AMBAR, 6.0, 0.8)    # os degraus do tanque
        luz((lado * 3.0, 1.5, 0.9), AMBAR, 8.0, 0.8)      # o trilho, no meio da carreta
        luz((lado * 3.0, 10.8, 1.3), AMBAR, 7.0, 0.8)     # as caixas de rodas da carreta
    # Os faróis acendem (as lentes, no .glb), mas não jogam luz na rua: no filme os carros não têm farol
    # iluminando o caminho (e a luz deles acendia a frente do próprio Mack)
    return luzes


# ------------------------------------------------------------------
# As prévias: de noite (como o jogo, com o brilho em volta) e de dia
# ------------------------------------------------------------------
VISTAS = {
    '34_frente': ((10.8, -24.5, 1.9), (0.3, -2.5, 3.3), 25),
    'lado_esquerdo': ((31.0, 0.3, 3.0), (0.0, 0.3, 3.5), 30),
    'lado_direito': ((-31.0, 0.3, 3.0), (0.0, 0.3, 3.5), 30),
    'tras': ((10.0, 31.0, 3.2), (0.2, 7.0, 3.4), 27),
    'cabine': ((8.0, -20.5, 5.2), (0.3, -11.0, 3.9), 30),
}
VISTAS_DE_DIA = ('34_frente', 'lado_esquerdo', 'tras', 'cabine')


def borrar(a, sigma):
    """Borrão gaussiano separável (pelas linhas e pelas colunas), com FFT"""
    k = int(3 * sigma)
    x = np.arange(-k, k + 1)
    g = np.exp(-x * x / (2.0 * sigma * sigma))
    g /= g.sum()
    for eixo in (0, 1):
        pad = [(0, 0)] * a.ndim
        pad[eixo] = (k, k)
        p = np.pad(a, pad, mode='edge')
        n = p.shape[eixo]
        F = np.fft.rfft(p, axis=eixo)
        G = np.fft.rfft(g, n=n)
        forma = [1] * a.ndim
        forma[eixo] = -1
        b = np.fft.irfft(F * G.reshape(forma), n=n, axis=eixo)
        a = np.take(b, np.arange(2 * k, 2 * k + a.shape[eixo]), axis=eixo)
    return a


def gravar_png(caminho_exr, caminho_png, brilho):
    img = bpy.data.images.load(caminho_exr)
    W, H = img.size
    px = np.array(img.pixels[:], dtype=np.float32).reshape(H, W, 4)[:, :, :3].astype(np.float64)
    bpy.data.images.remove(img)
    if brilho:
        claro = np.maximum(px - 1.0, 0.0)
        halo = 0.55 * borrar(claro, 2.5) + 0.35 * borrar(claro, 9.0) + 0.22 * borrar(claro, 26.0)
        px = px + halo
    px = px * (1.0 + px / 4.0) / (1.0 + px)                 # o claro vai encostando no branco sem estourar
    px = np.where(px <= 0.0031308, px * 12.92, 1.055 * np.power(np.clip(px, 0, None), 1 / 2.4) - 0.055)
    px = np.clip(px, 0.0, 1.0)
    saida = bpy.data.images.new('previa', W, H, alpha=True, float_buffer=False)
    rgba = np.concatenate([px, np.ones((H, W, 1))], axis=2).astype(np.float32)
    saida.pixels.foreach_set(rgba.ravel())
    saida.filepath_raw = caminho_png
    saida.file_format = 'PNG'
    saida.save()
    bpy.data.images.remove(saida)
    os.remove(caminho_exr)


def previas(pasta, objs, luzes):
    os.makedirs(pasta, exist_ok=True)
    sc = bpy.context.scene
    sc.render.engine = 'BLENDER_EEVEE'
    sc.eevee.taa_render_samples = 48
    sc.render.resolution_x, sc.render.resolution_y = 1600, 900
    sc.render.image_settings.file_format = 'OPEN_EXR'
    sc.render.image_settings.color_depth = '32'
    sc.view_settings.view_transform = 'Standard'
    for caminho in (ARQUIVO_DAS_RODAS, ARQUIVO_DA_PORTA):
        if os.path.exists(caminho):
            importar(caminho)                       # para aparecer (a cópia das medidas fica escondida)
    # o chão: asfalto
    me = bpy.data.meshes.new('chao')
    me.from_pydata([(-150, -150, 0), (150, -150, 0), (150, 150, 0), (-150, 150, 0)], [], [(0, 1, 2, 3)])
    chao = bpy.data.objects.new('chao', me)
    sc.collection.objects.link(chao)
    m_chao = bpy.data.materials.new('asfalto')
    m_chao.use_nodes = True
    bsdf_chao = next(n for n in m_chao.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    bsdf_chao.inputs['Roughness'].default_value = 0.85
    me.materials.append(m_chao)
    # as lâmpadas: emissão com a cor do vértice
    m_luz = bpy.data.materials.new('lampada_da_previa')
    m_luz.use_nodes = True
    nt = m_luz.node_tree
    nt.nodes.clear()
    atributo = nt.nodes.new('ShaderNodeAttribute')
    atributo.attribute_name = 'Col'
    emissao = nt.nodes.new('ShaderNodeEmission')
    saida = nt.nodes.new('ShaderNodeOutputMaterial')
    nt.links.new(atributo.outputs['Color'], emissao.inputs['Color'])
    nt.links.new(emissao.outputs['Emission'], saida.inputs['Surface'])
    for o in objs:
        o.data.materials.clear()
        o.data.materials.append(m_luz)
    # o céu e a luz do sol (ou da lua)
    mundo = bpy.data.worlds.new('ceu')
    sc.world = mundo
    mundo.use_nodes = True
    fundo = mundo.node_tree.nodes['Background']
    sol = bpy.data.objects.new('sol', bpy.data.lights.new('sol', 'SUN'))
    sc.collection.objects.link(sol)
    sol.rotation_euler = (math.radians(50), 0.0, math.radians(35))
    # as luzes pontuais do .json, de volta ao Blender
    pontuais = []
    for k, l in enumerate(luzes):
        dados = bpy.data.lights.new('pontual_%d' % k, 'POINT')
        dados.color = (l['r'], l['g'], l['b'])
        dados.shadow_soft_size = 0.5
        try:
            dados.use_custom_distance = True
            dados.cutoff_distance = l['raio']
        except AttributeError:
            pass
        o = bpy.data.objects.new('pontual_%d' % k, dados)
        o.location = (l['x'], -l['z'], l['y'])
        sc.collection.objects.link(o)
        pontuais.append((o, l))
    cam = bpy.data.objects.new('camera', bpy.data.cameras.new('camera'))
    sc.collection.objects.link(cam)
    sc.camera = cam
    for noite in (True, False):
        if noite:
            # a noite do jogo: azulada, com a lua; dá para ver a rua (hora.c)
            fundo.inputs[0].default_value = (0.020, 0.024, 0.050, 1)
            fundo.inputs[1].default_value = 1.0
            sol.data.color = (0.55, 0.62, 1.0)
            sol.data.energy = 0.35
            bsdf_chao.inputs['Base Color'].default_value = (0.05, 0.05, 0.055, 1)
            emissao.inputs['Strength'].default_value = 14.0
            for o, l in pontuais:
                o.data.energy = l['forca'] * l['raio'] * l['raio'] * 6.0
                o.hide_render = False
        else:
            fundo.inputs[0].default_value = (0.55, 0.68, 0.90, 1)
            fundo.inputs[1].default_value = 0.9
            sol.data.color = (1.0, 0.97, 0.9)
            sol.data.energy = 3.5
            bsdf_chao.inputs['Base Color'].default_value = (0.16, 0.155, 0.15, 1)
            emissao.inputs['Strength'].default_value = 0.32     # apagadas: a cor vezes 0,32, como no jogo
            for o, l in pontuais:
                o.hide_render = True
        for nome, (pos, alvo, lente_mm) in VISTAS.items():
            if not noite and nome not in VISTAS_DE_DIA:
                continue
            cam.location = Vector(pos)
            cam.rotation_euler = (Vector(alvo) - Vector(pos)).to_track_quat('-Z', 'Y').to_euler()
            cam.data.lens = lente_mm
            arquivo = os.path.join(pasta, '%s_%s' % ('noite' if noite else 'dia', nome))
            sc.render.filepath = arquivo + '.exr'
            bpy.ops.render.render(write_still=True)
            gravar_png(arquivo + '.exr', arquivo + '.png', brilho=noite)
            print('  prévia: %s.png' % arquivo)


# ------------------------------------------------------------------
# Tudo
# ------------------------------------------------------------------
def main():
    lampadas, recusas = [], []
    for p in pedidos():
        lp = assentar(p, recusas)
        if lp is not None:
            lampadas.append(lp)
    grupos = {}
    for lp in lampadas:
        grupos.setdefault(lp['grupo'], []).append(lp)
    print('Lâmpadas: %d' % len(lampadas))
    for g, lps in grupos.items():
        empurradas = sum(1 for lp in lps if lp['empurrada'] != (0.0, 0.0))
        print('  %-40s %3d%s' % (g, len(lps), ('  (%d empurradas)' % empurradas) if empurradas else ''))
    for g, o, m in recusas:
        print('  DE FORA: %s, raio de %s: %s' % (g, o, m))
    if not lampadas:
        raise SystemExit('ERRO: nenhuma lâmpada assentou')
    mat = material_da_luz()
    objs = objetos_das_lampadas(lampadas, mat)
    caminho = os.path.join(PASTA, 'mack_luzes.glb')
    exportar(objs, caminho, lampadas)
    luzes = luzes_pontuais()
    dados = {'nome': 'mack', 'acender_em': ACENDER_EM, 'luzes': luzes}
    with open(os.path.join(PASTA, 'mack_luzes.json'), 'w') as fj:
        json.dump(dados, fj, indent=1, ensure_ascii=False)
    print('Gravado %s: %d luzes pontuais' % (os.path.join(PASTA, 'mack_luzes.json'), len(luzes)))
    ordens = np.array([lp['ordem'] for lp in lampadas])
    print('Ordem de acender: A de %.2f a %.2f' % (ordens.min(), ordens.max()))
    if PREVIAS:
        previas(PREVIAS, objs, luzes)


main()
