"""
preparar_mack.py
------------------------------------------------------------------
Prepara o modelo do Mack (o caminhão que leva o carro do jogador da
cidade até as corridas) a partir do modelo original:

    blender -b --factory-startup -P ferramentas/preparar_mack.py -- \
        ~/Downloads/mack_only-cars-gcn-ver.glb recursos/modelos/mack.glb

Ele grava três arquivos: mack.glb (o caminhão sem as rodas e sem a porta
da carreta), mack_rodas.glb (as 10 rodas, cada uma numa malha, para o jogo
girar cada uma em volta do próprio eixo quando o Mack anda) e mack_porta.glb
(a porta de trás da carreta, que desce e vira a rampa do carro). E os olhos,
no jeito dos carros: recursos/modelos/carros/olhos/mack_fundo.png, _iris.png
e _mascara.png, e a linha "mack" de olhos.txt (veja carros3d.c).

O que ele faz:
  1. Refaz os olhos, como os do Mack do filme e vivos como os dos carros:
     o branco grande, a pálpebra vermelha cobrindo o alto, o traço escuro
     dos cílios e a íris grande, verde-avelã, puxada para o lado do nariz.
     No original (do GameCube), a textura do olho é uma íris pequena
     sobre fundo branco, e as coordenadas de textura vão de -1 a 2: o
     jogo de lá prendia a textura na borda; aqui ela se repetia, e cada
     olho virava uma grade de 3 x 3 íris. A textura nova (os dois olhos,
     cada um numa metade) é desenhada aqui mesmo, e as coordenadas saem
     da posição de cada ponto no olho. Ela sai também separada nas três
     camadas que o shader dos olhos dos carros usa (o fundo sem as íris,
     só as íris e a máscara do branco): no jogo, a íris do Mack anda, a
     pálpebra pisca e ele olha para o carro do jogador. A faixa preta de
     pálpebra do original sai: os cílios agora vêm na textura.
  2. Amplia 4 vezes as texturas do corpo e dos pneus (as do original são
     de 512 x 256 e 256 x 64), com o filtro xBR do ffmpeg, que mantém
     os contornos nítidos em vez de borrar. Sem o ffmpeg, elas ficam
     como estão.
  3. Separa as rodas: no original elas são peças soltas de uma malha só
     (pneu, disco e cubo de cada roda); as peças de cada roda viram uma
     malha, e as rodas vão para o outro arquivo.
  4. Separa a porta de trás da carreta (a arte do McQueen), que no jogo
     desce até o chão e vira a rampa.
  5. Deixa o modelo em metros (4,5 m de largura, uns 30 m de comprimento),
     com o meio do caminhão na origem, as rodas em y = 0 e a frente
     para +z (como os carros em carros3d.c).
------------------------------------------------------------------
"""
import json
import os
import shutil
import struct
import subprocess
import sys
import tempfile
import bmesh
import bpy
import mathutils
import numpy as np

LARGURA_EM_METROS = 4.5   # a cidade foi ampliada 4,5 vezes: menor que isso, o Mack some perto dela
LARGURA_DA_TEXTURA_DOS_OLHOS = 512   # os dois olhos, cada um numa metade
ALTURA_DA_TEXTURA_DOS_OLHOS = 256
ANDA_A_IRIS = 0.12                   # quanto a íris anda de lado olhando de canto (fração da textura)

# O olho, em frações dele: x de 0 (lado do nariz) a 1 (lado de fora); y de 0 (em cima) a 1 (embaixo)
PALPEBRA_NO_NARIZ = 0.28     # onde a pálpebra termina, no lado do nariz e no de fora (quase reta, como no filme)
PALPEBRA_FORA = 0.29
CILIOS = 0.035               # a grossura do traço escuro embaixo da pálpebra
IRIS_X, IRIS_Y = 0.37, 0.63  # o meio da íris
RAIO_DA_IRIS = 0.47          # nas unidades do modelo original (o olho tem uns 2,4 x 2,6)
PUPILA = 0.46                # da íris
COR_DA_IRIS_FORA = (0.15, 0.19, 0.11)
COR_DA_IRIS_MEIO = (0.30, 0.38, 0.21)
COR_DA_IRIS_DENTRO = (0.47, 0.46, 0.26)
COR_DA_PUPILA = (0.05, 0.05, 0.05)
COR_DOS_CILIOS = (0.08, 0.05, 0.05)

argv = sys.argv[sys.argv.index("--") + 1:]
origem, destino = argv[0], argv[1]

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=origem)
cena = bpy.context.scene
malhas = [o for o in cena.objects if o.type == 'MESH']


def material_de(objeto):
    return objeto.data.materials[0] if objeto.data.materials else None


# ---- 1) Os olhos ----
# (só "Eyes": "Eyeshadow", a faixa preta de pálpebra, também começa com "Eyes")
olhos = [o for o in malhas if material_de(o) is not None and material_de(o).name.split(".")[0] == "Eyes"]
faixas = [o for o in malhas if material_de(o) is not None and material_de(o).name.split(".")[0] == "Eyeshadow"]
for o in faixas:
    bpy.data.objects.remove(o, do_unlink=True)
malhas = [o for o in malhas if o not in faixas]

# A cor da lataria, para a pálpebra: a mediana dos pixels bem vermelhos da textura do corpo
corpo = next(o for o in malhas if material_de(o).name.split(".")[0] == "Body")
imagem_do_corpo = next(n for n in material_de(corpo).node_tree.nodes if n.type == 'TEX_IMAGE').image
pixels_do_corpo = np.array(imagem_do_corpo.pixels[:], dtype=np.float32).reshape(-1, 4)
vermelhos = pixels_do_corpo[(pixels_do_corpo[:, 0] > 0.45) & (pixels_do_corpo[:, 1] < 0.2) & (pixels_do_corpo[:, 2] < 0.2)]
cor_da_lataria = np.median(vermelhos[:, :3], axis=0)

# As coordenadas: cada olho numa metade da textura (o da direita do Mack, x < 0, na de u menor), com u
# crescendo com o x e v de cima (0) para baixo (1), como no glTF. No Blender, o v é ao contrário
largura_do_olho = altura_do_olho = 1.0
for o in olhos:
    malha = o.data
    pontos = [o.matrix_world @ v.co for v in malha.vertices]
    limites = {}
    for lado in (1, -1):
        meus = [p for p in pontos if p.x * lado > 0]
        limites[lado] = (min(p.x for p in meus), max(p.x for p in meus), min(p.z for p in meus), max(p.z for p in meus))
    x0, x1, baixo, cima = limites[1]
    largura_do_olho, altura_do_olho = x1 - x0, cima - baixo
    coordenadas = malha.uv_layers.active.data
    for poligono in malha.polygons:
        for laco in poligono.loop_indices:
            p = pontos[malha.loops[laco].vertex_index]
            lado = 1 if p.x > 0 else -1
            x0, x1, baixo, cima = limites[lado]
            u = (0.5 if lado > 0 else 0.0) + 0.5 * (p.x - x0) / (x1 - x0)
            v = (cima - p.z) / (cima - baixo)
            coordenadas[laco].uv = (u, 1.0 - v)
print(f"MACK: olho de {largura_do_olho:.2f} x {altura_do_olho:.2f}; lataria {np.round(cor_da_lataria, 2)}")

# As imagens, na ordem do arquivo (a linha 0 em cima). Em cada pixel: x do olho de 0 (nariz) a 1 (fora) e y
L, A = LARGURA_DA_TEXTURA_DOS_OLHOS, ALTURA_DA_TEXTURA_DOS_OLHOS
coluna = np.arange(L)
metade = (coluna >= L // 2)
u_no_olho = ((coluna % (L // 2)) + 0.5) / (L // 2)
x_do_olho = np.where(metade, u_no_olho, 1.0 - u_no_olho)   # o da direita (metade 0) tem o nariz no u maior
X = np.broadcast_to(x_do_olho[None, :], (A, L))
Y = np.broadcast_to(((np.arange(A) + 0.5) / A)[:, None], (A, L))
pixel = largura_do_olho / (L // 2)   # unidades do modelo por pixel


def suave(borda, d):
    """0 dentro, 1 fora, com um pixel e meio de transição (sem serrilhado)"""
    return np.clip((d - borda) / (1.5 * pixel) + 0.5, 0.0, 1.0)


def misturar(a, b, quanto):
    return a * (1.0 - quanto[..., None]) + np.asarray(b) * quanto[..., None]


# O fundo: o branco (um pouco mais escuro perto da pálpebra e nos cantos), os cílios e a pálpebra
palpebra = PALPEBRA_NO_NARIZ + (PALPEBRA_FORA - PALPEBRA_NO_NARIZ) * X
abaixo = (Y - palpebra) * altura_do_olho
branco = 0.97 - 0.16 * np.exp(-np.clip(abaixo, 0, None) / 0.25) - 0.05 * ((X - 0.5) * 2) ** 4
fundo = np.stack([branco * 0.98, branco * 0.99, branco], axis=-1)
fundo = misturar(fundo, COR_DOS_CILIOS, 1.0 - suave(0.0, abaixo - CILIOS * altura_do_olho))
fundo = misturar(fundo, cor_da_lataria, 1.0 - suave(0.0, abaixo))

# A máscara: onde o branco aparece (abaixo dos cílios)
mascara = suave(0.0, abaixo - CILIOS * altura_do_olho)

# As íris: anel escuro por fora, verde-avelã, mais clara perto da pupila, com raios finos e o brilho
dx, dy = (X - IRIS_X) * largura_do_olho, (Y - IRIS_Y) * altura_do_olho
r = np.hypot(dx, dy) / RAIO_DA_IRIS
angulo = np.arctan2(dy, dx)
t = np.clip((r - PUPILA) / (1.0 - PUPILA), 0.0, 1.0)
forma = (A, L, 3)
cor_da_iris = np.where(t[..., None] < 0.55,
                       misturar(np.broadcast_to(COR_DA_IRIS_DENTRO, forma).copy(), COR_DA_IRIS_MEIO, t / 0.55),
                       misturar(np.broadcast_to(COR_DA_IRIS_MEIO, forma).copy(), COR_DA_IRIS_FORA, (t - 0.55) / 0.45))
cor_da_iris *= (1.0 + 0.10 * np.sin(angulo * 23.0) * (t > 0.05))[..., None]
cor_da_iris = misturar(cor_da_iris, COR_DA_PUPILA, 1.0 - suave(PUPILA * RAIO_DA_IRIS, r * RAIO_DA_IRIS))
brilho = np.hypot(dx - 0.30 * RAIO_DA_IRIS * np.where(metade, -1.0, 1.0)[None, :], dy + 0.38 * RAIO_DA_IRIS)
cor_da_iris = misturar(cor_da_iris, (1, 1, 1), 1.0 - suave(0.16 * RAIO_DA_IRIS, brilho))
alfa_da_iris = 1.0 - suave(RAIO_DA_IRIS, r * RAIO_DA_IRIS)

# Junto (a textura do modelo, para quem não usa as camadas): como o shader dos olhos junta
sombra = np.clip(fundo.max(axis=-1) / 0.97, 0.0, 1.0)
junto = misturar(fundo, np.zeros(3), np.zeros((A, L)))
junto = junto * (1.0 - (alfa_da_iris * mascara)[..., None]) + cor_da_iris * sombra[..., None] * (alfa_da_iris * mascara)[..., None]


def imagem(nome, rgb, alfa=None, arquivo=None):
    """Uma imagem do Blender (que guarda de baixo para cima), gravada em PNG se pedirem"""
    pixels = np.ones((A, L, 4), dtype=np.float32)
    pixels[..., :3] = np.clip(rgb, 0.0, 1.0)
    if alfa is not None:
        pixels[..., 3] = np.clip(alfa, 0.0, 1.0)
    nova = bpy.data.images.new(nome, L, A, alpha=alfa is not None)
    nova.pixels = np.flipud(pixels).ravel()
    nova.file_format = 'PNG'
    if arquivo is not None:
        nova.filepath_raw = arquivo
        nova.save()
    return nova


pasta_dos_olhos = os.path.join(os.path.dirname(os.path.abspath(destino)), "carros", "olhos")
os.makedirs(pasta_dos_olhos, exist_ok=True)
imagem("mack_fundo", fundo, arquivo=os.path.join(pasta_dos_olhos, "mack_fundo.png"))
imagem("mack_iris", cor_da_iris, alfa_da_iris, arquivo=os.path.join(pasta_dos_olhos, "mack_iris.png"))
imagem("mack_mascara", np.repeat(mascara[..., None], 3, axis=-1), arquivo=os.path.join(pasta_dos_olhos, "mack_mascara.png"))
imagem_nova = imagem("olho_do_mack", junto)
imagem_nova.pack()
no_da_imagem = next(n for n in material_de(olhos[0]).node_tree.nodes if n.type == 'TEX_IMAGE')
no_da_imagem.image = imagem_nova
no_da_imagem.interpolation = 'Linear'
no_da_imagem.extension = 'EXTEND'
# O meio de cada íris parada (u, v), para a tabela dos olhos
meios_das_iris = (0.5 * (1.0 - IRIS_X), IRIS_Y, 0.5 + 0.5 * IRIS_X, IRIS_Y)

# ---- 2) As texturas do corpo e dos pneus, 4 vezes maiores ----
def ampliar(imagem):
    if shutil.which("ffmpeg") is None:
        print("MACK: sem o ffmpeg, a textura", imagem.name, "fica como está")
        return imagem
    pasta = tempfile.mkdtemp()
    antes, depois = os.path.join(pasta, "antes.png"), os.path.join(pasta, "depois.png")
    imagem.filepath_raw = antes
    imagem.file_format = 'PNG'
    imagem.save()
    subprocess.run(["ffmpeg", "-hide_banner", "-loglevel", "error", "-y", "-i", antes, "-vf", "xbr=4", depois], check=True)
    nova = bpy.data.images.load(depois)
    nova.pack()
    nova.name = imagem.name + "_x4"
    print(f"MACK: {imagem.name} {imagem.size[0]}x{imagem.size[1]} -> {nova.size[0]}x{nova.size[1]}")
    shutil.rmtree(pasta, ignore_errors=True)
    return nova


for nome in ("Body", "Tires"):
    material = bpy.data.materials.get(nome)
    if material is not None:
        for no in material.node_tree.nodes:
            if no.type == 'TEX_IMAGE' and no.image is not None:
                no.image = ampliar(no.image)

# ---- 3) Cada roda numa malha (as peças soltas dos pneus, juntas pelo lado e pelo eixo) ----
pneus = next(o for o in malhas if material_de(o).name.split(".")[0] == "Tires")
bpy.ops.object.select_all(action='DESELECT')
bpy.context.view_layer.objects.active = pneus
pneus.select_set(True)
bpy.ops.object.mode_set(mode='EDIT')
bpy.ops.mesh.select_all(action='SELECT')
bpy.ops.mesh.separate(type='LOOSE')
bpy.ops.object.mode_set(mode='OBJECT')
pecas = [o for o in cena.objects if o.type == 'MESH' and material_de(o).name.split(".")[0] == "Tires"]
rodas_por_lugar = {}
for peca in pecas:
    cantos = [peca.matrix_world @ mathutils.Vector(c) for c in peca.bound_box]
    meio_x = sum(c.x for c in cantos) / 8
    meio_y = sum(c.y for c in cantos) / 8
    lugar = (1 if meio_x > 0 else -1, round(meio_y / 2.0))   # o lado e o eixo (os eixos ficam a mais de 5 unidades um do outro)
    rodas_por_lugar.setdefault(lugar, []).append(peca)
rodas = []
for lugar, grupo in sorted(rodas_por_lugar.items()):
    bpy.ops.object.select_all(action='DESELECT')
    for peca in grupo:
        peca.select_set(True)
    bpy.context.view_layer.objects.active = grupo[0]
    if len(grupo) > 1:
        bpy.ops.object.join()
    roda = bpy.context.view_layer.objects.active
    roda.name = f"roda_{len(rodas)}"
    rodas.append(roda)
print(f"MACK: {len(pecas)} peças de pneu viraram {len(rodas)} rodas")
malhas = [o for o in cena.objects if o.type == 'MESH']

# ---- 4) Em metros, no meio, rodas no chão ----
minimo = mathutils.Vector((1e9, 1e9, 1e9))
maximo = mathutils.Vector((-1e9, -1e9, -1e9))
for o in malhas:
    for canto in o.bound_box:
        p = o.matrix_world @ mathutils.Vector(canto)
        minimo = mathutils.Vector(map(min, minimo, p))
        maximo = mathutils.Vector(map(max, maximo, p))
escala = LARGURA_EM_METROS / (maximo.x - minimo.x)
meio = mathutils.Vector(((minimo.x + maximo.x) / 2, (minimo.y + maximo.y) / 2, minimo.z))
ajuste = mathutils.Matrix.Scale(escala, 4) @ mathutils.Matrix.Translation(-meio)
for o in malhas:
    o.data.transform(ajuste @ o.matrix_world)
    o.parent = None
    o.matrix_world = mathutils.Matrix.Identity(4)
for o in [o for o in cena.objects if o.type != 'MESH']:
    bpy.data.objects.remove(o, do_unlink=True)
tamanho = (maximo - minimo) * escala
print(f"MACK: {tamanho.x:.2f} m de largura, {tamanho.y:.2f} m de comprimento, {tamanho.z:.2f} m de altura")

# ---- 5) A porta de trás da carreta: as faces grandes viradas para trás, na ponta de trás ----
corpo = next(o for o in malhas if material_de(o).name.split(".")[0] == "Body")
fim_de_tras = max(v.co.y for v in corpo.data.vertices)


def eh_da_porta(face):
    return face.normal.y > 0.95 and face.calc_center_median().y > fim_de_tras - 1.6 and face.calc_area() > 1.0


porta = corpo.copy()
porta.data = corpo.data.copy()
porta.name = "porta"
cena.collection.objects.link(porta)
for objeto, apagar_a_porta in ((corpo, True), (porta, False)):
    bm = bmesh.new()
    bm.from_mesh(objeto.data)
    apagar = [f for f in bm.faces if eh_da_porta(f) == apagar_a_porta]
    bmesh.ops.delete(bm, geom=apagar, context='FACES')
    bm.to_mesh(objeto.data)
    bm.free()
cantos = [v.co for v in porta.data.vertices]
print(f"MACK: porta de {max(c.x for c in cantos) - min(c.x for c in cantos):.2f} x {max(c.z for c in cantos) - min(c.z for c in cantos):.2f} m,"
      f" com a dobradiça a {min(c.z for c in cantos):.2f} m do chão")
malhas = [o for o in cena.objects if o.type == 'MESH']

# A frente do caminhão é -y no Blender, que vira +z no glTF (y para cima)
base = destino[:-4] if destino.endswith(".glb") else destino
partes = ((destino, [o for o in malhas if o not in rodas and o is not porta]), (base + "_rodas.glb", rodas), (base + "_porta.glb", [porta]))
for arquivo, parte in partes:
    bpy.ops.object.select_all(action='DESELECT')
    for o in parte:
        o.select_set(True)
    bpy.ops.export_scene.gltf(filepath=arquivo, export_format='GLB', export_yup=True, use_selection=True)
    print("MACK: gravado em", arquivo, f"({len(parte)} malhas)")

# ---- 6) A linha do Mack na tabela dos olhos (o material dos olhos é o número dele no .glb) ----
with open(destino, "rb") as arquivo:
    arquivo.read(12)
    tamanho, _ = struct.unpack("<II", arquivo.read(8))
    gltf = json.loads(arquivo.read(tamanho))
nomes = [m.get("name", "") for m in gltf.get("materials", [])]
material_dos_olhos = next(i for i, nome in enumerate(nomes) if nome.split(".")[0] == "Eyes")
linha = (f"mack {material_dos_olhos} {ANDA_A_IRIS:.4f} -1 {IRIS_Y:.4f} {RAIO_DA_IRIS / altura_do_olho:.4f} "
         + " ".join(f"{m:.4f}" for m in meios_das_iris))
tabela = os.path.join(pasta_dos_olhos, "olhos.txt")
linhas = open(tabela).read().splitlines() if os.path.exists(tabela) else []
linhas = [l for l in linhas if not l.startswith("mack ")] + [linha]
with open(tabela, "w") as arquivo:
    arquivo.write("\n".join(linhas) + "\n")
print("MACK: olhos.txt ->", linha)
