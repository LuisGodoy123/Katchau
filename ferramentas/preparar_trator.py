"""
preparar_trator.py
------------------------------------------------------------------
Prepara o modelo do trator-vaca (os tratores que pastam em volta de
Radiator Springs e que o jogador pode tombar) e escolhe onde ficam os
rebanhos:

    blender -b --factory-startup -P ferramentas/preparar_trator.py -- \\
        ~/Downloads/cars_tractor.glb recursos/modelos/radiator_springs.glb recursos/modelos

Lê o trator (Sketchfab "cars tractor", de zairiqzairiq, CC-BY-4.0) e o mapa
(o desenho, radiator_springs.glb, e a colisão ao lado dele, radiator_springs.col).
Grava, na pasta de destino:
  trator/corpo.glb         tudo menos os olhos e as rodas (a lataria, os vidros
                           dos lados e de trás, as pálpebras paradas e a sombra
                           em cima dos olhos, o eixo e as alças escuras)
  trator/olhos.glb         só os olhos: os dois vidros da frente da cabine (o
                           branco do olho e a íris vêm pintados neles)
  trator/rodas_tras.glb    as duas rodas grandes de trás (pneu, disco e calota)
  trator/rodas_frente.glb  as duas rodinhas da frente
  trator/trator.json       as medidas, os eixos das rodas, a boca do escapamento
                           e a faixa dos olhos (veja src/tratores.c)
  tratores.json            onde fica cada trator: x, z, y (o chão ali) e o rumo

Os quatro .glb estão no mesmo referencial (o jogo desenha todos com a mesma
matriz e gira as rodas em volta do eixo delas): em metros, a frente para +z,
o alto para +y, x = 0 no meio da largura, z = 0 no meio entre o eixo de trás
e o da frente e y = 0 no chão, embaixo das rodas.

O que ele faz com o trator:
  1. Tira o esqueleto: o modelo vem com ossos (o corpo, as pálpebras, a boca,
     as rodas) e pesos em cada vértice. Os vértices ficam na pose de repouso
     (é a mesma em que o modelo foi salvo) e saem a armadura e os pesos (o
     shader do jogo não faz skinning). Sai também a esfera que o importador
     cria para desenhar os ossos.
  2. Separa as peças pelos pesos: cada peça solta do modelo (ligada por arestas)
     vai para as rodas de trás ou da frente se quase todo o peso dela é dos ossos
     daquela roda (o pneu, o disco e a calota); o eixo de trás e o da frente,
     que pesam metade na roda e metade no corpo, ficam no corpo (são cilindros:
     girar ou não dá no mesmo). Os olhos são os dois vidros da frente da cabine
     (materiais tra_eye_l e tra_eye_r): o olho inteiro, branco e íris, é a
     textura deles.
  3. Conserta a textura dos olhos: é uma íris de 64 x 64 sobre branco, e as
     coordenadas vão de -0,6 a 1,6 na largura e de -1,6 a 2,6 na altura. O jogo
     de onde o modelo veio prendia a textura na borda (o resto do vidro saía
     branco); aqui ela se repete (o sampler do arquivo é REPEAT, e a raylib
     repete sempre), e cada olho virava uma grade de íris. A textura nova é a
     mesma íris, pixel por pixel, no meio de uma folha branca grande o bastante
     para as coordenadas caberem entre 0 e 1 (é o que a borda presa fazia).
  4. Põe em metros (COMPRIMENTO, da grade da frente ao pneu de trás) e no
     referencial acima. Os eixos das rodas vêm dos ossos das rodas (o meio
     de cada uma), e o raio, do ponto do pneu mais longe do eixo.
  5. Mede o que o jogo precisa (trator.json):
       - a boca do escapamento: x e z do emissor de fumaça do jogo original (o
         osso TRAC_RightTopExhaustParticle) e y no alto do cano, ali;
       - a faixa dos olhos: a parte do vidro que aparece, vista da frente, de
         cima, de baixo e dos lados (raios até pontos do vidro). Em cima, a pálpebra
         parada do modelo e a faixa de sombra cobrem o vidro; embaixo, a
         pálpebra de baixo. y_cima é o ponto mais alto do olho que aparece, e
         y_baixo, o mais baixo: a pálpebra do jogo desce de um até o outro e
         cobre o olho todo;
       - a cor da pálpebra: a média da textura da lataria nas pálpebras do
         modelo (rosa), em sRGB.

E com o mapa (tratores.json): os rebanhos estão em REBANHOS, escolhidos numa
grade da colisão vista de cima (areia plana, longe de ruas, prédios, rochas,
cercas, das entradas e vagas dos moradores, da rodovia e da pista de treino) e
conferidos em vistas de cima. Para cada trator, a ferramenta acha o chão na
colisão e confere, num círculo de RAIO_LIVRE metros em volta: o chão existe, é
areia ou terra, quase plano (INCLINACAO_MINIMA, DESNIVEL_MAXIMO) e os raios na
horizontal a 0,5, 1,0 e 1,8 m do chão não batem em nada. Confere também que o
chão desenhado (radiator_springs.glb) está na mesma altura, que os vizinhos do
rebanho ficam a 4-8 m, longe dos nós das ruas dos moradores
(src/moradores_mapa.h), e que dá para ir de carro até a rua (o caminho de cada
rebanho, de metro em metro: sem degrau, sem rampa forte e sem parede). Se algo
falhar, ele avisa e não grava tratores.json.
------------------------------------------------------------------
"""
import json
import math
import os
import re
import struct
import sys
import bmesh
import bpy
import mathutils
import numpy as np
from mathutils.bvhtree import BVHTree

argv = sys.argv[sys.argv.index("--") + 1:]
ARQ_TRATOR, ARQ_MAPA, DESTINO = argv[0], argv[1], argv[2]
PASTA = os.path.join(DESTINO, "trator")
ARQ_COLISAO = os.path.splitext(ARQ_MAPA)[0] + ".col"
ARQ_MORADORES = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "src", "moradores_mapa.h")

COMPRIMENTO = 3.05        # metros, da grade da frente ao pneu de trás (fica com ~2,55 m de altura e ~2,1 de largura;
                          # o McQueen tem 4,4 m de comprimento)
PESO_DA_RODA = 0.9        # fração do peso de uma peça nos ossos de uma roda para ela ir com as rodas
OSSOS_TRAS = "wheelrear"  # pedaços do nome dos ossos das rodas
OSSOS_FRENTE = "wheelfront"
OSSO_DA_FUMACA = "TRAC_RightTopExhaustParticle"
OSSO_DA_RODA_TRAS = "Trac_wheelrearleft_059"
OSSO_DA_RODA_FRENTE = "Trac_wheelfrontleft_055"
OSSOS_DAS_PALPEBRAS = ("Trac_Lid_", "Trac_LidLow_")
MATERIAIS_DOS_OLHOS = ("tra_eye_l", "tra_eye_r")
CANO = 0.10               # metros: em volta do emissor, o que é o cano do escapamento (para achar o alto dele)
PASSO_NO_OLHO = 0.004     # metros entre os pontos do vidro testados com raios
VISTAS_DO_OLHO = [(e, a) for e in (-30.0, -15.0, 0.0, 15.0, 30.0, 45.0, 60.0) for a in (-40.0, -20.0, 0.0, 20.0, 40.0)]
                          # (elevação, azimute) em graus de quem olha o trator de frente

# ---- Os rebanhos ----
# Cada um: o nome, o meio (x, z do jogo), o caminho de carro até a rua principal e os tratores, em volta do meio:
# (dx, dz, rumo em graus). Rumo 0 olhando para -z, crescendo para a direita (horário, visto de cima): a frente é
# (sen rumo, -cos rumo). Os rumos variam como os de vacas pastando.
REBANHOS = [
    # Ao norte da rua, atrás da quadra oeste (dos prédios da Casa Della Tires e do Ramone), entre a estrada de
    # cascalho que sobe do fim oeste (x ~ 275) e a que faz a curva ao norte (z ~ -110 a -250): 235 m da rua.
    # Reto para o sul, o carro passa rente a um prédio da quadra: o caminho vai pela estrada de cascalho
    ("noroeste", (400.0, -100.0), [(400.0, -100.0), (280.0, -100.0), (280.0, 120.0), (300.0, 131.0)],
     [(0.0, 0.0, 20.0), (5.6, 1.8, 75.0), (-3.2, 5.0, 310.0), (2.8, 6.8, 160.0), (-5.4, -1.2, 230.0)]),
    # Ao norte, a leste da estrada de cascalho que sai da rua em x ~ 850, perto da curva dela: 275 m da rua
    ("nordeste", (1100.0, -140.0), [(1100.0, -140.0), (1000.0, -60.0), (1000.0, 131.0)],
     [(0.0, 0.0, 250.0), (5.2, -2.0, 200.0), (1.4, 5.4, 95.0), (6.4, 4.0, 330.0)]),
    # Ao sul, a sudoeste da cidade, entre a estrada de cascalho que desce do fim oeste e a que sai em x ~ 505:
    # 165 m da rua
    ("sudoeste", (330.0, 300.0), [(330.0, 300.0), (290.0, 200.0), (290.0, 135.0)],
     [(0.0, 0.0, 140.0), (-5.0, 2.6, 40.0), (4.2, 3.6, 215.0), (-0.6, 6.4, 290.0)]),
    # Ao sul, a leste do caminho de areia até a pista de treino (x ~ 1000) e ao norte do oval: 90 m da rua
    ("sudeste", (1070.0, 225.0), [(1070.0, 225.0), (1070.0, 140.0)],
     [(0.0, 0.0, 300.0), (5.4, 1.6, 10.0), (1.8, 5.6, 175.0)]),
]
RAIO_LIVRE = 6.0          # metros em volta de cada trator: chão plano e nada de pé
INCLINACAO_MINIMA = 0.97  # y da normal do chão (uns 14 graus)
DESNIVEL_MAXIMO = 0.5     # metros entre o ponto mais alto e o mais baixo do chão no círculo
ALTURAS_DOS_RAIOS = (0.5, 1.0, 1.8)
VIZINHO = (4.0, 8.0)      # metros até o trator mais perto do mesmo rebanho
LONGE_DOS_NOS = 50.0      # metros até o nó mais perto das ruas dos moradores
CHAO_DESENHADO = 0.15     # metros de diferença aceita entre o chão da colisão e o desenhado
DEGRAU = 0.35             # metros por metro de caminho (mais 0,1): mais que isso é degrau
RAMPA = 0.6               # y da normal do chão no caminho: menos que isso é rampa forte demais
MEIO_CARRO = 1.1          # metros para cada lado do caminho que também precisam estar livres


# =====================================================================
# O trator
# =====================================================================
def importar_trator():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=ARQ_TRATOR)
    armadura = next(o for o in bpy.context.scene.objects if o.type == 'ARMATURE')
    pecas = [o for o in bpy.context.scene.objects if o.type == 'MESH' and o.parent == armadura]
    return armadura, pecas


def posicao_do_osso(armadura, nome):
    """Onde está o osso (a cabeça dele, na pose do arquivo), no mundo"""
    osso = next(b for b in armadura.pose.bones if b.name.startswith(nome))
    return armadura.matrix_world @ osso.head


def assar(armadura, pecas):
    """Cada malha na pose de repouso, no mundo, sem pai, sem armadura e sem pesos. Devolve, para cada
    malha nova, o peso de cada vértice nas rodas de trás, nas da frente, nas pálpebras e no todo"""
    armadura.data.pose_position = 'REST'
    bpy.context.view_layer.update()
    grafo = bpy.context.evaluated_depsgraph_get()
    novas = []
    for o in pecas:
        me = bpy.data.meshes.new_from_object(o.evaluated_get(grafo))
        n = len(me.vertices)
        pesos = np.zeros((n, 4))   # trás, frente, pálpebras, todos
        for v in o.data.vertices:
            for g in v.groups:
                nome = o.vertex_groups[g.group].name
                pesos[v.index, 3] += g.weight
                if OSSOS_TRAS in nome:
                    pesos[v.index, 0] += g.weight
                elif OSSOS_FRENTE in nome:
                    pesos[v.index, 1] += g.weight
                elif nome.startswith(OSSOS_DAS_PALPEBRAS):
                    pesos[v.index, 2] += g.weight
        nova = bpy.data.objects.new(o.name, me)
        bpy.context.scene.collection.objects.link(nova)
        nova.matrix_world = o.matrix_world
        nova.vertex_groups.clear()
        novas.append((nova, pesos))
    # a transformação de cada uma para dentro dos vértices (o operador cuida das normais)
    bpy.ops.object.select_all(action='DESELECT')
    for nova, _ in novas:
        nova.select_set(True)
    bpy.context.view_layer.objects.active = novas[0][0]
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    for o in list(bpy.context.scene.objects):
        if o not in [nova for nova, _ in novas]:
            bpy.data.objects.remove(o, do_unlink=True)
    return novas


def pecas_soltas(me):
    """O número da peça solta de cada vértice: ligados por arestas ou no mesmo lugar (as costuras de textura
    separam os vértices, mas a peça é uma só)"""
    pai = list(range(len(me.vertices)))

    def raiz(i):
        while pai[i] != i:
            pai[i] = pai[pai[i]]
            i = pai[i]
        return i
    for e in me.edges:
        a, b = raiz(e.vertices[0]), raiz(e.vertices[1])
        pai[a] = b
    lugar = {}
    for v in me.vertices:
        chave = tuple(round(c, 5) for c in v.co)
        if chave in lugar:
            pai[raiz(v.index)] = raiz(lugar[chave])
        else:
            lugar[chave] = v.index
    return np.array([raiz(i) for i in range(len(me.vertices))])


def separar(novas):
    """Corta cada malha nas partes do jogo. Devolve {parte: [objetos]} e os pixels da textura embaixo das
    pálpebras paradas do modelo (para a cor da pálpebra do jogo)"""
    partes = {"corpo": [], "olhos": [], "rodas_tras": [], "rodas_frente": []}
    palpebras = []
    for o, pesos in novas:
        me = o.data
        material = me.materials[0].name if me.materials else ""
        if material.split(".")[0] in MATERIAIS_DOS_OLHOS:
            partes["olhos"].append(o)
            continue
        peca = pecas_soltas(me)
        rotulo_da_peca, palpebra_da_peca = {}, {}
        for p in np.unique(peca):
            soma = pesos[peca == p].sum(axis=0)
            total = max(soma[3], 1e-9)
            if soma[0] / total >= PESO_DA_RODA:
                rotulo_da_peca[p] = "rodas_tras"
            elif soma[1] / total >= PESO_DA_RODA:
                rotulo_da_peca[p] = "rodas_frente"
            else:
                rotulo_da_peca[p] = "corpo"
            palpebra_da_peca[p] = soma[2] / total >= 0.5
        rotulo_da_face = [rotulo_da_peca[peca[f.vertices[0]]] for f in me.polygons]
        faces_das_palpebras = [f.index for f in me.polygons if palpebra_da_peca[peca[f.vertices[0]]]]
        if faces_das_palpebras:
            palpebras.append(pixels_embaixo(o, faces_das_palpebras))
        for rotulo in sorted(set(rotulo_da_face)):
            parte = o.copy()
            parte.data = me.copy()
            parte.name = f"{rotulo}_{o.name}"
            bpy.context.scene.collection.objects.link(parte)
            bm = bmesh.new()
            bm.from_mesh(parte.data)
            bm.faces.ensure_lookup_table()
            fora = [bm.faces[i] for i, r in enumerate(rotulo_da_face) if r != rotulo]
            bmesh.ops.delete(bm, geom=fora, context='FACES')
            soltos = [v for v in bm.verts if not v.link_faces]
            bmesh.ops.delete(bm, geom=soltos, context='VERTS')
            bm.to_mesh(parte.data)
            bm.free()
            partes[rotulo].append(parte)
        bpy.data.objects.remove(o, do_unlink=True)
    return partes, np.concatenate(palpebras)


def no_de_imagem(material):
    if material is None or material.node_tree is None:
        return None
    return next((n for n in material.node_tree.nodes if n.type == 'TEX_IMAGE' and n.image), None)


def consertar_olhos(olhos):
    """A íris no meio de uma folha branca do tamanho das coordenadas dos olhos (como a textura presa na borda),
    um material só para os dois olhos"""
    us, vs = [], []
    for o in olhos:
        uv = o.data.uv_layers[0].data
        us += [c.uv[0] for c in uv]
        vs += [c.uv[1] for c in uv]
    u0, u1 = math.floor(min(us)), math.ceil(max(us))
    v0, v1 = math.floor(min(vs)), math.ceil(max(vs))
    material = olhos[0].data.materials[0]
    iris = no_de_imagem(material).image
    w, h = iris.size
    p = np.empty(w * h * 4, dtype=np.float32)
    iris.pixels.foreach_get(p)
    p = p.reshape(h, w, 4)
    borda = np.concatenate([p[0], p[-1], p[:, 0], p[:, -1]])[:, :3]
    folha = np.ones(((v1 - v0) * h, (u1 - u0) * w, 4), dtype=np.float32)
    folha[..., :3] = np.median(borda, axis=0)   # a cor da borda (branco): é o que a borda presa repetia
    folha[(-v0) * h:(1 - v0) * h, (-u0) * w:(1 - u0) * w] = p
    nova = bpy.data.images.new("olhos", folha.shape[1], folha.shape[0], alpha=False)
    nova.pixels.foreach_set(folha.ravel())
    nova.file_format = 'PNG'
    nova.pack()
    no = no_de_imagem(material)
    no.image = nova
    no.extension = 'EXTEND'
    material.name = "olhos"
    for o in olhos:
        o.data.materials.clear()
        o.data.materials.append(material)
        for c in o.data.uv_layers[0].data:
            c.uv = ((c.uv[0] - u0) / (u1 - u0), (c.uv[1] - v0) / (v1 - v0))
    print(f"TRATOR: olhos: a íris {w}x{h} no meio de uma folha {nova.size[0]}x{nova.size[1]} "
          f"(u de {min(us):.2f} a {max(us):.2f}, v de {min(vs):.2f} a {max(vs):.2f})")


def juntar(objetos, nome):
    bpy.ops.object.select_all(action='DESELECT')
    for o in objetos:
        o.select_set(True)
    bpy.context.view_layer.objects.active = objetos[0]
    if len(objetos) > 1:
        bpy.ops.object.join()
    o = bpy.context.view_layer.objects.active
    o.name = nome
    o.data.name = nome
    return o


def vertices(o):
    p = np.empty(len(o.data.vertices) * 3)
    o.data.vertices.foreach_get("co", p)
    return p.reshape(-1, 3)


def para_o_jogo(p):
    """Blender (x, y, z) -> jogo (x, y, z): o jogo é (x, z, -y) do Blender"""
    p = np.asarray(p, dtype=float)
    return np.stack([p[..., 0], p[..., 2], -p[..., 1]], axis=-1)


def pixels_embaixo(o, faces):
    """Os pixels da textura (sRGB) embaixo das faces dadas: pontos sorteados em cada triângulo, uns três por
    pixel coberto"""
    amostras = [np.zeros((0, 3), dtype=np.float32)]
    uv = o.data.uv_layers[0].data
    for i in faces:
        f = o.data.polygons[i]
        no = no_de_imagem(o.data.materials[f.material_index])
        if no is None:
            continue   # a faixa de sombra em cima dos olhos não tem textura: a cor é a das pálpebras
        imagem = no.image
        w, h = imagem.size
        px = np.empty(w * h * 4, dtype=np.float32)
        imagem.pixels.foreach_get(px)   # as texturas de 8 bits vêm como estão no arquivo: sRGB
        px = px.reshape(h, w, 4)
        cantos = [np.array(uv[l].uv) * (w, h) for l in f.loop_indices]
        for k in range(1, len(cantos) - 1):
            a, b, c = cantos[0], cantos[k], cantos[k + 1]
            d1, d2 = b - a, c - a
            n = max(3, int(1.5 * abs(d1[0] * d2[1] - d1[1] * d2[0])) + 3)
            s = np.random.default_rng(i).random((n, 2))
            s[s.sum(1) > 1] = 1 - s[s.sum(1) > 1]
            pts = a + s[:, :1] * d1 + s[:, 1:] * d2
            amostras.append(px[np.floor(pts[:, 1]).astype(int) % h, np.floor(pts[:, 0]).astype(int) % w, :3])
    return np.concatenate(amostras)


def faixa_dos_olhos(olhos, todos):
    """A faixa de altura do olho que aparece: raios de várias direções (de frente, de cima e dos lados) até
    pontos do vidro; o ponto aparece se o raio bate nele antes de qualquer outra coisa"""
    pontos, faces, dono = [], [], []
    for o in todos:
        base = len(pontos)
        pontos += [v.co.copy() for v in o.data.vertices]
        for f in o.data.polygons:
            faces.append([base + i for i in f.vertices])
            dono.append(o in olhos)
    arvore = BVHTree.FromPolygons(pontos, faces)
    alvos = []
    for o in olhos:
        for f in o.data.polygons:
            cantos = [o.data.vertices[i].co for i in f.vertices]
            for k in range(1, len(cantos) - 1):
                a, b, c = cantos[0], cantos[k], cantos[k + 1]
                n = max(2, int(max((b - a).length, (c - a).length) / PASSO_NO_OLHO))
                for i in range(n + 1):
                    for j in range(n + 1 - i):
                        alvos.append(a + (b - a) * (i / n) + (c - a) * (j / n))
    vistos = []
    for elevacao, azimute in VISTAS_DO_OLHO:
        e, a = math.radians(elevacao), math.radians(azimute)
        d = mathutils.Vector((math.sin(a) * math.cos(e), math.cos(a) * math.cos(e), -math.sin(e)))  # do olho para dentro
        for alvo in alvos:
            origem = alvo - d * 10.0
            onde, _, indice, dist = arvore.ray_cast(origem, d)
            if onde is not None and dono[indice] and (onde - alvo).length < 0.002:
                vistos.append(onde.z)
    return max(vistos), min(vistos), min(p.z for p in alvos), max(p.z for p in alvos)


def exportar(objeto, arquivo):
    bpy.ops.object.select_all(action='DESELECT')
    objeto.select_set(True)
    bpy.context.view_layer.objects.active = objeto
    caminho = os.path.join(PASTA, arquivo)
    bpy.ops.export_scene.gltf(filepath=caminho, export_format='GLB', export_yup=True, use_selection=True,
                              export_skins=False, export_animations=False, export_morph=False,
                              export_vertex_color='NONE', export_normals=True, export_tangents=False,
                              export_image_format='AUTO', export_materials='EXPORT', export_apply=False)
    return caminho


def conferir_glb(caminho):
    """Lê o .glb gravado: sem skin, sem JOINTS/WEIGHTS, com NORMAL, texturas PNG, até 65.535 vértices por
    primitiva. Devolve (triângulos, primitivas, caixa no jogo)"""
    with open(caminho, "rb") as arquivo:
        dados = arquivo.read()
    tamanho = struct.unpack("<I", dados[12:16])[0]
    gltf = json.loads(dados[20:20 + tamanho])
    assert "skins" not in gltf and "animations" not in gltf, caminho
    triangulos, primitivas, mn, mx = 0, 0, np.full(3, 1e9), np.full(3, -1e9)
    for malha in gltf["meshes"]:
        for p in malha["primitives"]:
            a = p["attributes"]
            assert "NORMAL" in a and "JOINTS_0" not in a and "WEIGHTS_0" not in a and "COLOR_0" not in a, caminho
            pos = gltf["accessors"][a["POSITION"]]
            assert pos["count"] <= 65535, caminho
            mn, mx = np.minimum(mn, pos["min"]), np.maximum(mx, pos["max"])
            triangulos += gltf["accessors"][p["indices"]]["count"] // 3
            primitivas += 1
    for imagem in gltf.get("images", []):
        assert imagem.get("mimeType") == "image/png", caminho
    for no in gltf["nodes"]:
        assert not any(k in no for k in ("matrix", "rotation", "scale", "translation")) or \
            no.get("translation", [0, 0, 0]) == [0, 0, 0], caminho
    return triangulos, primitivas, mn, mx


def preparar_trator():
    armadura, pecas = importar_trator()
    fumaca = posicao_do_osso(armadura, OSSO_DA_FUMACA)
    eixo_tras = posicao_do_osso(armadura, OSSO_DA_RODA_TRAS)
    eixo_frente = posicao_do_osso(armadura, OSSO_DA_RODA_FRENTE)
    novas = assar(armadura, pecas)
    partes, palpebras = separar(novas)
    consertar_olhos(partes["olhos"])
    for nome, objetos in partes.items():
        print(f"TRATOR: {nome}: {len(objetos)} pedaços")

    # ---- Medidas, ainda nas unidades do arquivo ----
    todos = [o for objetos in partes.values() for o in objetos]
    tudo = np.concatenate([vertices(o) for o in todos])
    rodas = {r: np.concatenate([vertices(o) for o in partes[r]]) for r in ("rodas_tras", "rodas_frente")}
    chao = min(rodas["rodas_tras"][:, 2].min(), rodas["rodas_frente"][:, 2].min())
    escala = COMPRIMENTO / (tudo[:, 1].max() - tudo[:, 1].min())
    meio = mathutils.Vector(((tudo[:, 0].min() + tudo[:, 0].max()) / 2, (eixo_tras.y + eixo_frente.y) / 2, chao))
    mover = mathutils.Matrix.Scale(escala, 4) @ mathutils.Matrix.Translation(-meio)
    for o in todos:
        o.data.transform(mover)   # escala igual nos três eixos e translação: as normais não mudam
        o.data.update()
    raios = {}
    for r, eixo in (("rodas_tras", eixo_tras), ("rodas_frente", eixo_frente)):
        e = mover @ eixo
        p = np.concatenate([vertices(o) for o in partes[r]])
        raios[r] = (e, float(np.hypot(p[:, 1] - e.y, p[:, 2] - e.z).max()))
    fumaca = mover @ fumaca
    cano = np.concatenate([vertices(o) for o in partes["corpo"]])
    cano = cano[np.hypot(cano[:, 0] - fumaca.x, cano[:, 1] - fumaca.y) < CANO]
    boca = (fumaca.x, fumaca.y, float(cano[:, 2].max()))
    cor = palpebras.mean(axis=0)

    # ---- Um objeto por arquivo ----
    corpo = juntar(partes["corpo"], "trator_corpo")
    olhos = juntar(partes["olhos"], "trator_olhos")
    tras = juntar(partes["rodas_tras"], "trator_rodas_tras")
    frente = juntar(partes["rodas_frente"], "trator_rodas_frente")
    y_cima, y_baixo, vidro_baixo, vidro_cima = faixa_dos_olhos([olhos], [corpo, olhos, tras, frente])
    print(f"TRATOR: olhos: o vidro vai de y {vidro_baixo:.3f} a {vidro_cima:.3f}; aparece de {y_baixo:.3f} a {y_cima:.3f}")

    os.makedirs(PASTA, exist_ok=True)
    caixa_min, caixa_max = np.full(3, 1e9), np.full(3, -1e9)
    for objeto, arquivo in ((corpo, "corpo.glb"), (olhos, "olhos.glb"), (tras, "rodas_tras.glb"),
                            (frente, "rodas_frente.glb")):
        caminho = exportar(objeto, arquivo)
        triangulos, primitivas, mn, mx = conferir_glb(caminho)
        caixa_min, caixa_max = np.minimum(caixa_min, mn), np.maximum(caixa_max, mx)
        print(f"TRATOR: {arquivo}: {triangulos} triângulos, {primitivas} primitivas, "
              f"caixa {np.round(mn, 3).tolist()} a {np.round(mx, 3).tolist()}")

    def eixo(r):
        e, raio = raios[r]
        g = para_o_jogo(e)
        return {"y": round(float(g[1]), 4), "z": round(float(g[2]), 4), "raio": round(raio, 4)}
    b = para_o_jogo(boca)
    ficha = {
        "comprimento": round(float(caixa_max[2] - caixa_min[2]), 4),
        "largura": round(float(caixa_max[0] - caixa_min[0]), 4),
        "altura": round(float(caixa_max[1] - caixa_min[1]), 4),
        "eixo_tras": eixo("rodas_tras"),
        "eixo_frente": eixo("rodas_frente"),
        "traseira_z": round(float(caixa_min[2]), 4),
        "frente_z": round(float(caixa_max[2]), 4),
        "escapamento": {"x": round(float(b[0]), 4), "y": round(float(b[1]), 4), "z": round(float(b[2]), 4)},
        "olhos": {"y_cima": round(y_cima, 4), "y_baixo": round(y_baixo, 4),
                  "cor_da_palpebra": [round(float(c), 3) for c in cor]},
    }
    with open(os.path.join(PASTA, "trator.json"), "w") as arquivo:
        json.dump(ficha, arquivo, indent=2, ensure_ascii=False)
        arquivo.write("\n")
    print("TRATOR: trator.json ->", json.dumps(ficha, ensure_ascii=False))
    return ficha


# =====================================================================
# Os rebanhos no mapa
# =====================================================================
def ler_colisao(caminho):
    """O .col (KCOL versão 2, veja src/colisao.c): os triângulos nos eixos do jogo e o tipo de chão de cada um"""
    with open(caminho, "rb") as arquivo:
        dados = arquivo.read()
    assert dados[:4] == b"KCOL"
    versao, total = struct.unpack("<II", dados[4:12])
    assert versao == 2
    pos = 12 + 24 + 16 + 12
    paleta_n = struct.unpack("<I", dados[pos:pos + 4])[0]
    pos += 4
    paleta = np.frombuffer(dados, dtype=np.uint8, count=4 * paleta_n, offset=pos).reshape(-1, 4)
    pos += 4 * paleta_n
    tri = np.frombuffer(dados, dtype=np.float32, count=total * 9, offset=pos).reshape(-1, 3, 3).astype(np.float64)
    pos += total * 36
    superficie = np.frombuffer(dados, dtype=np.uint8, count=total, offset=pos)
    return tri, paleta[superficie, 0]


class Colisao:
    AREIA, TERRA = 1, 2   # os tipos de chão de src/colisao.h

    def __init__(self, caminho):
        self.tri, self.tipo = ler_colisao(caminho)
        self.mn, self.mx = self.tri.min(1), self.tri.max(1)
        n = np.cross(self.tri[:, 1] - self.tri[:, 0], self.tri[:, 2] - self.tri[:, 0])
        self.ny = n[:, 1] / np.maximum(np.linalg.norm(n, axis=1), 1e-12)

    def perto(self, x0, x1, z0, z1):
        return np.nonzero((self.mx[:, 0] >= x0) & (self.mn[:, 0] <= x1) & (self.mx[:, 2] >= z0) & (self.mn[:, 2] <= z1))[0]

    def chao(self, idx, x, z, teto=1e9):
        """A superfície deitada mais alta embaixo de (x, z), abaixo do teto: (y, y da normal, tipo) ou None"""
        a, b, c = self.tri[idx, 0], self.tri[idx, 1], self.tri[idx, 2]
        v0x, v0z, v1x, v1z = b[:, 0] - a[:, 0], b[:, 2] - a[:, 2], c[:, 0] - a[:, 0], c[:, 2] - a[:, 2]
        v2x, v2z = x - a[:, 0], z - a[:, 2]
        det = v0x * v1z - v1x * v0z
        ok = np.abs(det) > 1e-12
        det = np.where(ok, det, 1.0)
        u = (v2x * v1z - v1x * v2z) / det
        v = (v0x * v2z - v2x * v0z) / det
        y = a[:, 1] + u * (b[:, 1] - a[:, 1]) + v * (c[:, 1] - a[:, 1])
        s = ok & (u >= -1e-6) & (v >= -1e-6) & (u + v <= 1 + 1e-6) & (np.abs(self.ny[idx]) > 0.2) & (y <= teto)
        if not s.any():
            return None
        j = int(np.argmax(np.where(s, y, -1e18)))
        return float(y[j]), float(abs(self.ny[idx][j])), int(self.tipo[idx][j])

    def raio(self, idx, origem, direcao, alcance):
        """O primeiro triângulo que o raio acerta até o alcance: (distância, y da normal) ou None"""
        t3 = self.tri[idx]
        e1, e2 = t3[:, 1] - t3[:, 0], t3[:, 2] - t3[:, 0]
        p = np.cross(direcao, e2)
        det = (e1 * p).sum(1)
        ok = np.abs(det) > 1e-12
        inv = np.where(ok, 1.0 / np.where(ok, det, 1.0), 0.0)
        tv = origem - t3[:, 0]
        u = (tv * p).sum(1) * inv
        q = np.cross(tv, e1)
        v = (q * direcao).sum(1) * inv
        t = (e2 * q).sum(1) * inv
        s = ok & (u >= 0) & (v >= 0) & (u + v <= 1) & (t > 1e-4) & (t < alcance)
        if not s.any():
            return None
        j = int(np.argmin(np.where(s, t, 1e18)))
        return float(t[j]), float(self.ny[idx][j])

    def conferir_lugar(self, x, z):
        """O chão em (x, z) e o que há num círculo de RAIO_LIVRE em volta. Devolve (y, problemas)"""
        r = RAIO_LIVRE
        idx = self.perto(x - r - 2, x + r + 2, z - r - 2, z + r + 2)
        meio = self.chao(idx, x, z)
        if meio is None:
            return None, ["sem chão"]
        y0, problemas, alturas = meio[0], [], []
        for distancia in (0.0, 1.5, 3.0, 4.5, r):
            k = 1 if distancia == 0 else int(2 * math.pi * distancia / 1.5)
            for a in np.linspace(0, 2 * math.pi, k, endpoint=False):
                c = self.chao(idx, x + distancia * math.cos(a), z + distancia * math.sin(a), y0 + 3.0)
                if c is None:
                    problemas.append(f"sem chão a {distancia} m")
                    continue
                alturas.append(c[0])
                if c[1] < INCLINACAO_MINIMA:
                    problemas.append(f"inclinado a {distancia} m (normal {c[1]:.3f})")
                if c[2] not in (self.AREIA, self.TERRA):
                    problemas.append(f"chão do tipo {c[2]} a {distancia} m")
        if alturas and max(alturas) - min(alturas) > DESNIVEL_MAXIMO:
            problemas.append(f"desnível de {max(alturas) - min(alturas):.2f} m")
        for h in ALTURAS_DOS_RAIOS:
            for a in np.linspace(0, 2 * math.pi, 32, endpoint=False):
                d = np.array([math.cos(a), 0.0, math.sin(a)])
                bateu = self.raio(idx, np.array([x, y0 + h, z]), d, r + 1.0)
                if bateu is not None:
                    problemas.append(f"raio a {h} m do chão bate a {bateu[0]:.1f} m")
        return y0, sorted(set(problemas))

    def conferir_caminho(self, pontos, passo=1.0):
        """Anda pelo caminho de metro em metro: o chão existe, sem degrau, sem rampa forte e sem parede (raios
        curtos a 0,5 e 1,0 m do chão, de um ponto ao próximo, no meio e a MEIO_CARRO para cada lado: a largura
        de um carro passa)"""
        problemas, y_antes = [], None
        for (x1, z1), (x2, z2) in zip(pontos, pontos[1:]):
            comprimento = math.hypot(x2 - x1, z2 - z1)
            n = max(1, int(comprimento / passo))
            idx = self.perto(min(x1, x2) - 5, max(x1, x2) + 5, min(z1, z2) - 5, max(z1, z2) + 5)
            d = np.array([(x2 - x1) / comprimento, 0.0, (z2 - z1) / comprimento])
            lado = np.array([-d[2], 0.0, d[0]])
            for i in range(n + 1):
                x, z = x1 + (x2 - x1) * i / n, z1 + (z2 - z1) * i / n
                c = self.chao(idx, x, z, (y_antes + 2.0) if y_antes is not None else 1e9)
                if c is None:
                    problemas.append(f"sem chão em ({x:.0f}, {z:.0f})")
                    continue
                y, ny, _ = c
                if ny < RAMPA:
                    problemas.append(f"rampa forte em ({x:.0f}, {z:.0f})")
                if y_antes is not None and abs(y - y_antes) > DEGRAU * comprimento / n + 0.1:
                    problemas.append(f"degrau de {y - y_antes:.2f} m em ({x:.0f}, {z:.0f})")
                if i < n:
                    for desvio in (-MEIO_CARRO, 0.0, MEIO_CARRO):
                        for h in (0.5, 1.0):
                            origem = np.array([x, y + h, z]) + lado * desvio
                            bateu = self.raio(idx, origem, d, comprimento / n)
                            if bateu is not None and abs(bateu[1]) < 0.55:
                                problemas.append(f"parede em ({origem[0]:.0f}, {origem[2]:.0f}) a {h} m do chão")
                y_antes = y
        return problemas


def nos_dos_moradores():
    """Os nós das ruas dos moradores (nome, x, z), lidos de src/moradores_mapa.h"""
    try:
        with open(ARQ_MORADORES, encoding="utf-8") as arquivo:
            texto = arquivo.read()
    except OSError:
        print("TRATOR: aviso: sem", ARQ_MORADORES, "(não confere a distância dos nós)")
        return []
    return [(n, float(x), float(z)) for n, x, z in
            re.findall(r'\{\s*"([A-Za-z_0-9]+)",\s*(-?[0-9.]+)f,\s*(-?[0-9.]+)f', texto)]


def chao_desenhado(pontos):
    """A altura do chão desenhado (radiator_springs.glb) em cada (x, z) do jogo: um raio de cima para baixo"""
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=ARQ_MAPA)
    grafo = bpy.context.evaluated_depsgraph_get()
    alturas = []
    for x, z, y in pontos:
        acertou, onde, *_ = bpy.context.scene.ray_cast(grafo, mathutils.Vector((x, -z, y + 2.5)),
                                                       mathutils.Vector((0, 0, -1)), distance=10.0)
        alturas.append(onde.z if acertou else None)
    return alturas


def escolher_lugares():
    colisao = Colisao(ARQ_COLISAO)
    nos = nos_dos_moradores()
    tratores, falhas = [], []
    for nome, (cx, cz), caminho, membros in REBANHOS:
        problemas = colisao.conferir_caminho(caminho)
        if problemas:
            falhas.append(f"{nome}: caminho até a rua: " + "; ".join(problemas[:5]))
        posicoes = [(cx + dx, cz + dz) for dx, dz, _ in membros]
        for i, ((dx, dz, rumo), (x, z)) in enumerate(zip(membros, posicoes)):
            y, problemas = colisao.conferir_lugar(x, z)
            if problemas:
                falhas.append(f"{nome} {i + 1} ({x:.1f}, {z:.1f}): " + "; ".join(problemas[:5]))
            vizinho = min(math.hypot(x - a, z - b) for j, (a, b) in enumerate(posicoes) if j != i)
            if not VIZINHO[0] <= vizinho <= VIZINHO[1]:
                falhas.append(f"{nome} {i + 1}: o vizinho mais perto está a {vizinho:.1f} m")
            if nos:
                no = min(nos, key=lambda n: math.hypot(n[1] - x, n[2] - z))
                longe = math.hypot(no[1] - x, no[2] - z)
                if longe < LONGE_DOS_NOS:
                    falhas.append(f"{nome} {i + 1}: a {longe:.0f} m do nó {no[0]}")
            tratores.append({"x": round(x, 2), "z": round(z, 2), "y": round(y if y is not None else 0.0, 3),
                             "rumo": round(math.radians(rumo) % (2 * math.pi), 4), "rebanho": nome})
            no_txt = f", nó mais perto {no[0]} a {longe:.0f} m" if nos else ""
            print(f"TRATOR: {nome} {i + 1}: ({x:.1f}, {z:.1f}) chão {tratores[-1]['y']:.3f}, rumo {rumo:.0f}°, "
                  f"vizinho a {vizinho:.1f} m{no_txt}")
    desenhado = chao_desenhado([(t["x"], t["z"], t["y"]) for t in tratores])
    for t, y in zip(tratores, desenhado):
        if y is None or abs(y - t["y"]) > CHAO_DESENHADO:
            falhas.append(f"{t['rebanho']} ({t['x']}, {t['z']}): o chão desenhado está em {y} e o da colisão em {t['y']}")
    if falhas:
        print("TRATOR: PROBLEMAS (tratores.json não foi gravado):")
        for f in falhas:
            print("   ", f)
        return False
    with open(os.path.join(DESTINO, "tratores.json"), "w") as arquivo:
        arquivo.write('{ "tratores": [\n')
        linhas = [f'    {{"x": {t["x"]}, "z": {t["z"]}, "y": {t["y"]}, "rumo": {t["rumo"]}}}' for t in tratores]
        arquivo.write(",\n".join(linhas))
        arquivo.write("\n] }\n")
    print(f"TRATOR: tratores.json: {len(tratores)} tratores em {len(REBANHOS)} rebanhos")
    return True


preparar_trator()
escolher_lugares()
