"""
montar_mundo_aberto.py
------------------------------------------------------------------
Junta os dois modelos de Radiator Springs num mapa só, para o passeio livre:

  - Race-O-Rama (PS3): a cidade, o Willy's Butte e os cânions. É a base.
  - Cars 2 (PS3), "Radiator Springs Arena": o oval de terra, a mesa com a
    estrada em espiral e os paredões vermelhos. Entra como região nova, no
    lugar livre mais perto da cidade, ligada a ela por uma estrada.

O que o script faz:
  1. põe os dois em metros (Race-O-Rama x4,5, Cars 2 x37,5: as estradas dos
     dois ficam do mesmo tamanho e o carro do jogo fica pequeno na cidade);
  2. tira os painéis de horizonte do Cars 2 (planos azulados em pé);
  3. procura, em volta da cidade, o lugar mais perto onde a arena cabe sem
     encostar em cânion, mesa, prédio ou estrada (girando de 90 em 90 graus);
  4. assenta no terreno as "películas" do Cars 2 (manchas, terra do butte,
     estradas), que no original flutuam uns centímetros: em escala, viravam
     beiradas de um metro onde o carro enroscava;
  5. corta o chão do Race-O-Rama embaixo da arena (sem dois chões um em cima
     do outro);
  6. puxa as cores do Race-O-Rama para as do Cars 2 (areia laranja, rochas
     vermelhas). Só os pixels "cor de areia" mudam: asfalto, faixas, prédios
     e placas ficam como estão;
  7. cria o chão de base (o deserto entre tudo) e a estrada de ligação;
  8. exporta recursos/modelos/radiator_springs.glb (o desenho) e
     recursos/modelos/radiator_springs.col (a colisão, nos eixos do jogo: x,
     y para cima, z = -norte, com o tipo de chão e a cor de cada triângulo,
     a largada e o centro da arena).

Uso (Blender 4.x ou 5.x, sem abrir a janela):
  Blender -b --factory-startup -P ferramentas/montar_mundo_aberto.py -- \\
      caminho/ps3-cars_race_o_rama_radiator_springs.glb \\
      caminho/ps3_cars_2_the_video_game_radiator_springs_arena.glb \\
      recursos/modelos/radiator_springs [--previa pasta]
------------------------------------------------------------------
"""

import bpy, bmesh, sys, os, math, struct, tempfile
import numpy as np
from mathutils import Vector, Matrix
from mathutils.bvhtree import BVHTree

argv = sys.argv[sys.argv.index('--') + 1:]
ARQ_ROR, ARQ_C2, SAIDA = argv[0], argv[1], argv[2]
PREVIA = argv[argv.index('--previa') + 1] if '--previa' in argv else None
TEMP = tempfile.mkdtemp(prefix='mundo_aberto_')

ESCALA_ROR = 4.5           # 1 unidade do Race-O-Rama = 4,5 m
ESCALA_C2 = 37.5           # 1 unidade do Cars 2 = 37,5 m (a mesma proporção entre os dois)
LARGADA_ROR = (150.0, -30.2)    # a rua principal, em frente ao Flo's (unidades do Race-O-Rama)
CENTRO_DA_CIDADE_ROR = (150.0, -30.0)
OVAL_C2 = Vector((0.0, -4.0, 0.0))   # o meio do oval de terra, nas unidades do Cars 2

RAIO_DO_TREINO = 330.0     # metros em volta do meio do oval: o que fica do Cars 2 (a pista de treino)
RAIO_DAS_ROCHAS = 420.0    # metros: os paredões vão um pouco além (para o wallride)
FOLGA_DA_ARENA = 40.0      # metros livres em volta da arena
PASSO_DA_BUSCA = 50.0      # metros entre os lugares testados
PASSO_DA_AMOSTRA = 45.0    # metros entre os pontos que conferem se a arena cabe
PLANA = 0.85               # z da normal: uma face assim é chão, mesmo com textura de rocha
ALTURA_DURA = 10.0         # metros acima do chão: daí para cima (mesa, cânion), a arena desvia
ALTURA_DO_PREDIO = 1.0     # metros: um prédio ou objeto pequeno bloqueia já daqui
OBJETO_PEQUENO = 150.0     # metros: menor que isso (visto de cima), é prédio ou objeto
DISTANCIA_MINIMA = 350.0   # metros do meio da cidade ao meio da arena: ela não cai em cima da cidade
PELICULA = 4.0             # metros: o que está até isso acima (ou abaixo) do terreno do Cars 2 é película
ACIMA_DO_TERRENO = 0.06    # metros: onde a película fica

ESTRADA_ACIMA = 0.03       # metros: a estrada de ligação é pintura no chão, não uma laje
LARGURA_DA_LIGACAO = 20.0  # metros, como a estrada do Race-O-Rama na escala nova (com o acostamento)
MARGEM = 400.0             # metros de deserto além de tudo
CELULA_DA_BASE = 30.0      # metros: a grade do chão de base
BASE_ABAIXO = 0.3          # metros: o chão de base fica abaixo do chão dos modelos (pouco: na borda, é um degrau)
CELULA_DA_EMENDA = 5.0     # metros: a malha fina que emenda a cidade e a pista de treino
MARGEM_DA_EMENDA = 260.0   # metros além da pista de treino que a emenda cobre (chega até a cidade)
EMENDA_ABAIXO = 0.12       # metros: embaixo do chão que existe, a emenda fica escondida
PEDACO_DA_SAIA = 2.0       # metros: cada borda vira pedaços deste tamanho, cada um com a sua queda
SAIA_A_PARTIR_DE = 0.6     # metros: uma borda de chão mais alta que isso sobre o que há embaixo ganha saia (parede)
MONTINHO_DO_PLATO = 1.2    # metros: o platô da pista de treino não passa disso acima do nível da cidade
FAIXA_DE_ENCAIXE = 80.0    # metros antes do recorte do terreno da pista de treino em que ele vai encaixando no chão da cidade
BASE_EMBAIXO_DA_EMENDA = 0.4   # metros: na região da emenda, o chão de base fica pelo menos isso embaixo dela
CHAO_INCLINADO = 0.5       # z da normal: daqui para cima é chão (para a emenda e o platô)

# Os matinhos e cactos do Cars 2, carimbados na areia da cidade
JANELA_DO_MATO = 60.0      # metros: o lado de cada molde (um pedaço da vegetação da arena)
MOLDES_DO_MATO = 24
DENSIDADE_DO_MATO = ((700.0, 0.25), (2000.0, 0.06), (1e9, 0.02))   # até tantos metros do centro da cidade: a fração da densidade da arena
LONGE_DA_RUA = 4.0         # metros: um matinho não nasce mais perto que isso de rua, prédio ou rocha

# ---- Texturas (nomes que o importador do Blender dá, Image_N na ordem do .glb) ----
AREIA_ROR = {'Image_1', 'Image_2', 'Image_3', 'Image_4', 'Image_45'}
ESTRADA_ROR = {'Image_30'}       # o asfalto com acostamento de areia
ROCHA_ROR = {'Image_10'}
ESTRATO_ROR = {'Image_0'}
HORIZONTE_ROR = {'Image_60'}     # os cartões de montanha ao longe (vinham roxos)
AREIA_C2, ROCHA_C2, ESTRATO_C2 = 'Image_3', 'Image_4', 'Image_2'
TERRENO_C2 = 'Image_3'           # o chão da arena (RAD_BOSS_Terrain)
HORIZONTE_C2 = {'Image_0', 'Image_1'}          # os painéis em pé do Cars 2
VEGETACAO_C2 = {'RAD_Vegetation_1_MAT', 'RAD_Cactus_1_MAT'}  # sem colisão
# Objetos pequenos do Race-O-Rama que flutuam (placas, letreiros sobre a rua):
# sem colisão, para o carro passar por baixo.
FLUTUA_ACIMA = 2.0         # unidades do Race-O-Rama
FLUTUANTE_MAXIMO = 40.0    # unidades: maior que isso é morro, não placa

# ---- O tipo de chão de cada textura (o jogo usa para a aderência, a poeira e o som) ----
CHAO_ASFALTO, CHAO_AREIA, CHAO_TERRA, CHAO_ROCHA, CHAO_OUTRO = 0, 1, 2, 3, 4
TIPOS = {}
for n in ('Image_1', 'Image_2', 'Image_3', 'Image_4', 'Image_45'):
    TIPOS['ror_' + n] = CHAO_AREIA
for n in ('Image_30', 'Image_61', 'Image_17', 'Image_16', 'Image_43'):
    TIPOS['ror_' + n] = CHAO_ASFALTO
for n in ('Image_41',):
    TIPOS['ror_' + n] = CHAO_TERRA
for n in ('Image_10', 'Image_0', 'Image_60'):
    TIPOS['ror_' + n] = CHAO_ROCHA
for n in ('Image_3', 'Image_8'):
    TIPOS['c2_' + n] = CHAO_AREIA
for n in ('Image_6',):
    TIPOS['c2_' + n] = CHAO_TERRA
for n in ('Image_4', 'Image_5', 'Image_7', 'Image_2'):
    TIPOS['c2_' + n] = CHAO_ROCHA


def importar(caminho, marca):
    antes = set(bpy.data.objects)
    imagens_antes = set(bpy.data.images)
    bpy.ops.import_scene.gltf(filepath=caminho)
    novos = [o for o in bpy.data.objects if o not in antes]
    for im in [im for im in bpy.data.images if im not in imagens_antes]:  # cópia: renomear reordena
        im.name = marca + '_' + im.name
    for o in novos:
        o.name = marca + '_' + o.name
    return [o for o in novos if o.type == 'MESH']


def aplicar_transformacao(objs, matriz):
    for o in objs:
        mundo = o.matrix_world.copy()
        o.parent = None
        o.matrix_world = matriz @ mundo
        o.data = o.data.copy()  # malhas compartilhadas não aplicam duas vezes
        o.data.transform(o.matrix_world)
        o.matrix_world = Matrix.Identity(4)


def imagens_do(o):
    r = set()
    for m in o.data.materials:
        if m and m.use_nodes:
            for n in m.node_tree.nodes:
                if n.type == 'TEX_IMAGE' and n.image:
                    r.add(n.image.name)
    return r


def imagem_do_material(m):
    if m and m.use_nodes:
        for n in m.node_tree.nodes:
            if n.type == 'TEX_IMAGE' and n.image:
                return n.image
    return None


def limites(objs):
    mn = Vector((1e18,) * 3); mx = Vector((-1e18,) * 3)
    for o in objs:
        for v in o.data.vertices:
            mn = Vector(map(min, mn, v.co)); mx = Vector(map(max, mx, v.co))
    return mn, mx


def arvore(objs):
    bm = bmesh.new()
    for o in objs:
        bm.from_mesh(o.data)
    bmesh.ops.triangulate(bm, faces=bm.faces)
    arv = BVHTree.FromBMesh(bm)
    bm.free()
    return arv


def altura(arv, x, y):
    hit = arv.ray_cast(Vector((x, y, 9000.0)), Vector((0, 0, -1)))
    return hit[0].z if hit[0] else None


# ------------------------------------------------------------------
# Cores: sRGB <-> Lab, e a transferência de cor (média e desvio no Lab)
# ------------------------------------------------------------------
def srgb_para_lab(rgb):
    c = np.where(rgb <= 0.04045, rgb / 12.92, ((rgb + 0.055) / 1.055) ** 2.4)
    m = np.array([[0.4124, 0.3576, 0.1805], [0.2126, 0.7152, 0.0722], [0.0193, 0.1192, 0.9505]])
    xyz = c @ m.T / np.array([0.95047, 1.0, 1.08883])
    f = np.where(xyz > 0.008856, np.cbrt(xyz), 7.787 * xyz + 16 / 116)
    return np.stack([116 * f[:, 1] - 16, 500 * (f[:, 0] - f[:, 1]), 200 * (f[:, 1] - f[:, 2])], 1)


def lab_para_srgb(lab):
    fy = (lab[:, 0] + 16) / 116; fx = fy + lab[:, 1] / 500; fz = fy - lab[:, 2] / 200
    f = np.stack([fx, fy, fz], 1)
    xyz = np.where(f > 0.206893, f ** 3, (f - 16 / 116) / 7.787) * np.array([0.95047, 1.0, 1.08883])
    m = np.array([[3.2406, -1.5372, -0.4986], [-0.9689, 1.8758, 0.0415], [0.0557, -0.2040, 1.0570]])
    c = np.clip(xyz @ m.T, 0, 1)
    return np.where(c <= 0.0031308, 12.92 * c, 1.055 * c ** (1 / 2.4) - 0.055)


def pixels(im):
    a = np.empty(im.size[0] * im.size[1] * 4, dtype=np.float32)
    im.pixels.foreach_get(a)
    return a.reshape(-1, 4)


def peso_de_areia(lab):
    """1 nos pixels cor de areia/rocha; 0 no cinza (asfalto) e no muito saturado (faixa amarela)."""
    croma = np.hypot(lab[:, 1], lab[:, 2])
    def degrau(a, b, x):
        t = np.clip((x - a) / (b - a), 0, 1); return t * t * (3 - 2 * t)
    return degrau(7.0, 13.0, croma) * (1 - degrau(38.0, 55.0, croma))


def estatisticas(nomes, inteira=False):
    labs = []
    for n in nomes:
        p = pixels(bpy.data.images[n])
        labs.append(srgb_para_lab(p[:, :3].astype(np.float64)))
    lab = np.concatenate(labs)
    w = np.ones((len(lab), 1)) if inteira else peso_de_areia(lab)[:, None]
    media = (lab * w).sum(0) / w.sum()
    desvio = np.sqrt(((lab - media) ** 2 * w).sum(0) / w.sum())
    return media, np.maximum(desvio, 1e-3)


def recolorir(nomes, alvo, inteira=False):
    """inteira: muda todos os pixels (numa imagem sem asfalto nem faixa para proteger)."""
    origem_m, origem_d = estatisticas(nomes, inteira)
    alvo_m, alvo_d = estatisticas([alvo])
    razao = np.clip(alvo_d / origem_d, 0.5, 2.0)
    for n in nomes:
        im = bpy.data.images[n]
        p = pixels(im)
        lab = srgb_para_lab(p[:, :3].astype(np.float64))
        novo = alvo_m + (lab - origem_m) * razao
        w = np.ones((len(lab), 1)) if inteira else peso_de_areia(lab)[:, None]
        p[:, :3] = lab_para_srgb(lab * (1 - w) + novo * w)
        # A imagem veio empacotada no .glb: um reload() voltaria ao original.
        # Salva numa imagem nova e troca a referência nos materiais.
        nova = bpy.data.images.new(n + '_rec', im.size[0], im.size[1], alpha=True)
        nova.pixels.foreach_set(p.ravel())
        caminho = os.path.join(TEMP, n + '.png')
        nova.filepath_raw = caminho; nova.file_format = 'PNG'; nova.save()
        bpy.data.images.remove(nova)
        nova = bpy.data.images.load(caminho)
        for m in bpy.data.materials:
            if m.use_nodes:
                for no in m.node_tree.nodes:
                    if no.type == 'TEX_IMAGE' and no.image == im:
                        no.image = nova
        bpy.data.images.remove(im)
        nova.name = n
        print('  recolorida', n, '->', alvo)


# ==================================================================
bpy.ops.wm.read_factory_settings(use_empty=True)
print('Importando...')
ror = importar(ARQ_ROR, 'ror')
c2 = importar(ARQ_C2, 'c2')

# 1. Escala (a posição da arena vem depois)
aplicar_transformacao(ror, Matrix.Scale(ESCALA_ROR, 4))
aplicar_transformacao(c2, Matrix.Scale(ESCALA_C2, 4))

# 2. Fora os painéis de horizonte do Cars 2
for o in list(c2):
    if imagens_do(o) & {'c2_' + n for n in HORIZONTE_C2}:
        c2.remove(o); bpy.data.objects.remove(o)

areia_ror = {'ror_' + n for n in AREIA_ROR}
rocha_ror = {'ror_' + n for n in ROCHA_ROR | ESTRATO_ROR | HORIZONTE_ROR}
chao_ror = [o for o in ror if imagens_do(o) & areia_ror]


# A arena desvia do que é alto (cânions, mesas, prédios). O que é baixo (chão, estradas,
# cercas, muretas) é cortado embaixo dela
obstaculos_ror = [o for o in ror if not (imagens_do(o) & areia_ror)]


def tamanho_xy(o):
    mn, mx = limites([o])
    return max(mx.x - mn.x, mx.y - mn.y)


pequenos_ror = [o for o in obstaculos_ror if tamanho_xy(o) < OBJETO_PEQUENO]
grandes_ror = [o for o in obstaculos_ror if o not in pequenos_ror]
asfalto_ror = {n for n, t in TIPOS.items() if t in (CHAO_ASFALTO, CHAO_TERRA) and n.startswith('ror_')}
ruas_ror = [o for o in ror if imagens_do(o) & asfalto_ror]   # ruas, calçadas e o circuito de terra: a arena nunca passa por cima
macios_ror = [o for o in ror if o not in ruas_ror and o not in pequenos_ror]   # o que pode ser cortado embaixo da arena
vegetacao_c2 = [o for o in c2 if any(m and m.name in VEGETACAO_C2 for m in o.data.materials)]
terreno_c2 = [o for o in c2 if 'c2_' + TERRENO_C2 in imagens_do(o) and o not in vegetacao_c2]

# 2a. Os matinhos e cactos da arena viram moldes, antes do recorte: pedaços de JANELA x JANELA da
# vegetação, com cada parte (um cartão ou um cacto) inteira e na altura certa em relação ao terreno
def partes_soltas(o):
    bm = bmesh.new(); bm.from_mesh(o.data)
    bm.faces.ensure_lookup_table()
    camada = bm.loops.layers.uv.active
    vistos, partes = set(), []
    for f in bm.faces:
        if f.index in vistos:
            continue
        pilha, faces = [f], []
        vistos.add(f.index)
        while pilha:
            g = pilha.pop(); faces.append(g)
            for e in g.edges:
                for h in e.link_faces:
                    if h.index not in vistos:
                        vistos.add(h.index); pilha.append(h)
        dados = [([l.vert.co.copy() for l in g.loops], [tuple(l[camada].uv) if camada else (0.0, 0.0) for l in g.loops]) for g in faces]
        pontos = [c for fc, _ in dados for c in fc]
        partes.append({'centro': sum(pontos, Vector()) / len(pontos), 'base': min(c.z for c in pontos), 'faces': dados,
                       'material': o.data.materials[0] if o.data.materials else None})
    bm.free()
    return partes


arv_terreno_inteiro = arvore(terreno_c2)
fontes_do_mato, vistas = [], []
for o in sorted(vegetacao_c2, key=lambda o: -len(o.data.polygons)):
    mn_o, mx_o = limites([o])
    if any((mn_o - a).length < 1.0 and (mx_o - b).length < 1.0 and abs(len(o.data.polygons) - n) < 0.02 * n for a, b, n in vistas):
        continue   # a mesma vegetação duas vezes (a arena traz uma cópia)
    vistas.append((mn_o, mx_o, len(o.data.polygons)))
    fontes_do_mato.append(o)
partes_do_mato = [pt for o in fontes_do_mato for pt in partes_soltas(o)]
for pt in partes_do_mato:
    t = altura(arv_terreno_inteiro, pt['centro'].x, pt['centro'].y)
    pt['acima'] = max(-0.3, min(0.05, pt['base'] - t)) if t is not None else 0.0   # na cidade, rente à areia
mn_m = Vector(map(min, *[pt['centro'] for pt in partes_do_mato])); mx_m = Vector(map(max, *[pt['centro'] for pt in partes_do_mato]))
janelas = []
x = mn_m.x
while x + JANELA_DO_MATO <= mx_m.x:
    y = mn_m.y
    while y + JANELA_DO_MATO <= mx_m.y:
        dentro = [pt for pt in partes_do_mato if x <= pt['centro'].x < x + JANELA_DO_MATO and y <= pt['centro'].y < y + JANELA_DO_MATO]
        if dentro:
            janelas.append((Vector((x + JANELA_DO_MATO / 2, y + JANELA_DO_MATO / 2, 0)), dentro))
        y += JANELA_DO_MATO
    x += JANELA_DO_MATO
janelas.sort(key=lambda j: len(j[1]))
meio_das_janelas = janelas[len(janelas) // 5:]   # as mais vazias ficam de fora (bordas da arena)
passo = max(1, len(meio_das_janelas) // MOLDES_DO_MATO)
moldes = [meio_das_janelas[k] for k in range(0, len(meio_das_janelas), passo)][:MOLDES_DO_MATO]
print('Mato: %d partes em %d objetos; %d moldes (%d a %d partes cada)' % (
    len(partes_do_mato), len(fontes_do_mato), len(moldes), min(len(m[1]) for m in moldes), max(len(m[1]) for m in moldes)))

# 2b. Do Cars 2 fica só a pista de treino: o oval, o terreno em volta e os paredões perto dele
# (o butte dele sai: a cidade já tem o Willy's Butte)
centro_do_oval = (OVAL_C2 * ESCALA_C2).xy
for o in list(c2):
    raio = RAIO_DO_TREINO if (o in terreno_c2 or o in vegetacao_c2) else RAIO_DAS_ROCHAS
    bm = bmesh.new(); bm.from_mesh(o.data)
    fora = [f for f in bm.faces if (f.calc_center_median().xy - centro_do_oval).length > raio]
    bmesh.ops.delete(bm, geom=fora, context='FACES')
    bm.to_mesh(o.data); bm.free()
    if len(o.data.polygons) == 0:
        c2.remove(o)
        for lista in (vegetacao_c2, terreno_c2):
            if o in lista:
                lista.remove(o)
        bpy.data.objects.remove(o)
print('Pista de treino: %d objetos do Cars 2' % len(c2))

# 3. Onde a arena cabe, o mais perto possível da cidade
print('Procurando lugar para a arena...')
arv_obst = arvore(grandes_ror)
arv_pequenos = arvore(pequenos_ror)
arv_chao_busca = arvore(chao_ror)
arv_ruas = arvore(ruas_ror)
mn_c2, mx_c2 = limites([o for o in c2 if o not in vegetacao_c2])
meio_c2 = Vector(((mn_c2.x + mx_c2.x) / 2, (mn_c2.y + mx_c2.y) / 2, 0))
# Só o chão dirigível da arena (com uma folga) precisa estar livre: os paredões dela podem
# encostar nos cânions
arv_terreno0 = arvore(terreno_c2)
amostras = []
x = mn_c2.x - FOLGA_DA_ARENA
while x <= mx_c2.x + FOLGA_DA_ARENA:
    y = mn_c2.y - FOLGA_DA_ARENA
    while y <= mx_c2.y + FOLGA_DA_ARENA:
        perto = any(altura(arv_terreno0, x + dx, y + dy) is not None
                    for dx, dy in ((0, 0), (FOLGA_DA_ARENA, 0), (-FOLGA_DA_ARENA, 0), (0, FOLGA_DA_ARENA), (0, -FOLGA_DA_ARENA)))
        if perto:
            amostras.append(Vector((x, y, 0)) - meio_c2)
        y += PASSO_DA_AMOSTRA
    x += PASSO_DA_AMOSTRA
# E os paredões da pista de treino: podem encostar em cânion, mas não em rua nem em prédio
arv_rochas0 = arvore([o for o in c2 if o not in terreno_c2 and o not in vegetacao_c2])
amostras_rochas = []
x = mn_c2.x
while x <= mx_c2.x:
    y = mn_c2.y
    while y <= mx_c2.y:
        if altura(arv_rochas0, x, y) is not None and altura(arv_terreno0, x, y) is None:
            amostras_rochas.append(Vector((x, y, 0)) - meio_c2)
        y += PASSO_DA_AMOSTRA / 2
    x += PASSO_DA_AMOSTRA / 2
print('Amostras do chão da arena: %d; dos paredões: %d' % (len(amostras), len(amostras_rochas)))
cidade = Vector((CENTRO_DA_CIDADE_ROR[0] * ESCALA_ROR, CENTRO_DA_CIDADE_ROR[1] * ESCALA_ROR, 0))
mn_ror, mx_ror = limites(ror)
candidatos = []
cx = mn_ror.x
while cx <= mx_ror.x:
    cy = mn_ror.y
    while cy <= mx_ror.y:
        candidatos.append(Vector((cx, cy, 0)))
        cy += PASSO_DA_BUSCA
    cx += PASSO_DA_BUSCA
candidatos.sort(key=lambda c: (c - cidade).length)
lugar, giro, melhor = None, 0, 1e18
oval_local = OVAL_C2 * ESCALA_C2 - meio_c2
oval_local.z = 0
for c in candidatos:
    if (c - cidade).length - (mx_c2 - mn_c2).length / 2 > melhor:
        break   # daqui para a frente, nem o oval mais bem virado fica mais perto
    for g in range(4):
        rot = Matrix.Rotation(g * math.pi / 2, 3, 'Z')
        distancia = (c + rot @ oval_local - cidade).length
        if distancia >= melhor:
            continue

        def livre(p):
            if arv_ruas.ray_cast(p + Vector((0, 0, 9000)), Vector((0, 0, -1)))[0] is not None:
                return False
            chao = altura(arv_chao_busca, p.x, p.y)
            chao = chao if chao is not None else 0.0
            hit = arv_pequenos.ray_cast(p + Vector((0, 0, 9000)), Vector((0, 0, -1)))[0]
            if hit is not None and hit.z > chao + ALTURA_DO_PREDIO:
                return False
            hit = arv_obst.ray_cast(p + Vector((0, 0, 9000)), Vector((0, 0, -1)))[0]
            return hit is None or hit.z < chao + ALTURA_DURA
        def sem_cidade(p):
            if arv_ruas.ray_cast(p + Vector((0, 0, 9000)), Vector((0, 0, -1)))[0] is not None:
                return False
            hit = arv_pequenos.ray_cast(p + Vector((0, 0, 9000)), Vector((0, 0, -1)))[0]
            chao = altura(arv_chao_busca, p.x, p.y)
            return hit is None or hit.z <= (chao if chao is not None else 0.0) + ALTURA_DO_PREDIO
        if all(livre(c + rot @ a) for a in amostras) and all(sem_cidade(c + rot @ a) for a in amostras_rochas):
            lugar, giro, melhor = c, g, distancia
if lugar is None:
    raise SystemExit('a arena não cabe em lugar nenhum')
print('Arena em (%.0f, %.0f), girada %d graus; o oval fica a %.0f m da cidade' % (lugar.x, lugar.y, giro * 90, melhor))
rot4 = Matrix.Rotation(giro * math.pi / 2, 4, 'Z')
aplicar_transformacao(c2, Matrix.Translation(lugar) @ rot4 @ Matrix.Translation(-meio_c2))
oval = lugar + rot4.to_3x3() @ Vector((oval_local.x, oval_local.y, 0))

# Altura: a borda do terreno da arena fica na altura do chão do Race-O-Rama em volta
arv_chao_ror = arvore(chao_ror)
arv_terreno = arvore(terreno_c2)
bordas_ror, bordas_c2 = [], []
for a in amostras:
    p = lugar + rot4.to_3x3() @ a
    t = altura(arv_terreno, p.x, p.y)
    r = altura(arv_chao_ror, p.x, p.y)
    if t is not None:
        bordas_c2.append(t)
    elif r is not None:
        bordas_ror.append(r)
z_ror = float(np.median(bordas_ror)) if bordas_ror else 0.0
# O platô da pista de treino (o chão em volta do oval) fica na altura do chão da cidade
plato = [altura(arv_terreno, oval.x + dx, oval.y + dy) for dx in range(-150, 151, 25) for dy in range(-150, 151, 25)]
z_c2 = float(np.median([h for h in plato if h is not None])) if any(h is not None for h in plato) else 0.0
for o in c2:
    o.data.transform(Matrix.Translation((0, 0, z_ror - z_c2)))
print('Arena subiu %.1f m (chão em volta: %.1f m)' % (z_ror - z_c2, z_ror))

# 3b. Sem o fosso: a arena original era um platô no meio de um fosso de ~9 m, que caía bem no
# caminho da cidade. Tudo o que estava abaixo do platô sobe até ele, e as paredes do fosso somem
# E a borda do terreno (o recorte) encaixa no chão da cidade daquele lugar: numa faixa antes do recorte,
# o terreno vai se misturando com a altura que o chão da cidade tinha ali (alto num morro, baixo na
# planície). Sem isso sobrava beirada: a borda alta pendurada sobre a cidade, ou um poço entre um morro
# da cidade e a borda baixa
def degrau_suave(a, b, x):
    t = min(max((x - a) / (b - a), 0.0), 1.0)
    return t * t * (3 - 2 * t)


arv_rochas_treino = arvore([o for o in c2 if o not in terreno_c2 and o not in vegetacao_c2])
arv_terreno_antes = arvore(terreno_c2)   # para baixar os matinhos da arena junto com o terreno
aplainados, encaixados = 0, 0
for o in terreno_c2:
    subiram = set()
    for v in o.data.vertices:
        if v.co.z < z_ror:
            v.co.z = z_ror
            subiram.add(v.index)
            aplainados += 1
        elif v.co.z > z_ror + MONTINHO_DO_PLATO and altura(arv_rochas_treino, v.co.x, v.co.y) is None:
            v.co.z = z_ror + MONTINHO_DO_PLATO   # pista de treino plana: sem montinhos nem folhas sobrepostas em degrau
            aplainados += 1                       # (embaixo de uma rocha, não: ela ficaria flutuando)
        mistura = degrau_suave(RAIO_DO_TREINO - FAIXA_DE_ENCAIXE, RAIO_DO_TREINO - 5.0, (v.co.xy - oval.xy).length)
        if mistura > 0.0:
            h = altura(arv_chao_busca, v.co.x, v.co.y)   # o chão da cidade como era, antes do corte
            v.co.z = v.co.z * (1.0 - mistura) + (h if h is not None else z_ror) * mistura
            encaixados += 1
    bm = bmesh.new(); bm.from_mesh(o.data)
    bm.normal_update()
    em_pe = [f for f in bm.faces if abs(f.normal.z) < 0.35 and any(v.index in subiram for v in f.verts)]   # só as paredes do fosso
    bmesh.ops.delete(bm, geom=em_pe, context='FACES')
    bm.to_mesh(o.data); bm.free()
arv_terreno = arvore(terreno_c2)
# Os matinhos da arena acompanham o terreno (os que estavam num montinho aplainado desceriam flutuando)
for o in vegetacao_c2:
    for v in o.data.vertices:
        antes, depois = altura(arv_terreno_antes, v.co.x, v.co.y), altura(arv_terreno, v.co.x, v.co.y)
        if antes is not None and depois is not None:
            v.co.z += depois - antes
print('Platô: %d vértices aplainados (fosso e montinhos); %d da borda encaixados no chão da cidade' % (aplainados, encaixados))

# 4. As películas do Cars 2 assentam no terreno
assentados = 0
for k, o in enumerate(o for o in c2 if o not in terreno_c2 and o not in vegetacao_c2):
    for v in o.data.vertices:
        t = altura(arv_terreno, v.co.x, v.co.y)
        if t is not None and abs(v.co.z - t) < PELICULA:
            v.co.z = t + ACIMA_DO_TERRENO * (1 + k % 3)   # camadas diferentes não brigam na profundidade
            assentados += 1
print('Películas: %d vértices assentados no terreno' % assentados)

# 4b. Além do terreno da pista de treino só ficam os paredões: os pisos baixos e planos do Cars 2 que
# sobraram ali (o chão vermelho, recortado no raio dos paredões) ficavam soltos, na altura do fosso
podados = 0
for o in [o for o in c2 if o not in terreno_c2 and o not in vegetacao_c2]:
    bm = bmesh.new(); bm.from_mesh(o.data)
    bm.normal_update()
    fora = [f for f in bm.faces if abs(f.normal.z) > 0.85 and f.calc_center_median().z < z_ror + 4.0
            and (f.calc_center_median().xy - oval.xy).length > RAIO_DO_TREINO]
    podados += len(fora)
    bmesh.ops.delete(bm, geom=fora, context='FACES')
    bm.to_mesh(o.data); bm.free()
print('Pisos soltos além do terreno: %d faces' % podados)

# 5. O chão, as estradas e as cercas do Race-O-Rama somem embaixo da arena
cortadas = 0
arv_arena = arvore([o for o in c2 if o not in vegetacao_c2])
for o in macios_ror:
    bm = bmesh.new(); bm.from_mesh(o.data)
    def embaixo(f):
        c = f.calc_center_median()
        if altura(arv_arena, c.x, c.y) is None:
            return False
        ch = altura(arv_chao_busca, c.x, c.y)
        return c.z < (ch if ch is not None else 0.0) + ALTURA_DURA   # paredão não se corta
    fora = [f for f in bm.faces if embaixo(f)]
    cortadas += len(fora)
    bmesh.ops.delete(bm, geom=fora, context='FACES')
    bm.to_mesh(o.data); bm.free()
print('Race-O-Rama: %d faces cortadas embaixo da arena' % cortadas)


# 6. Cores
print('Recolorindo...')
recolorir(sorted('ror_' + n for n in AREIA_ROR | ESTRADA_ROR), 'c2_' + AREIA_C2)
recolorir(sorted('ror_' + n for n in ROCHA_ROR), 'c2_' + ROCHA_C2)
recolorir(sorted('ror_' + n for n in ESTRATO_ROR), 'c2_' + ESTRATO_C2)
recolorir(sorted('ror_' + n for n in HORIZONTE_ROR), 'c2_' + ROCHA_C2, inteira=True)

# 7a. Chão de base: a altura do chão dos modelos, preenchida para fora por difusão
arv_chao_ror = arvore(chao_ror)
todos = ror + c2
mn, mx = limites(todos)
x0, y0 = mn.x - MARGEM, mn.y - MARGEM
nx = int(math.ceil((mx.x + MARGEM - x0) / CELULA_DA_BASE)) + 1
ny = int(math.ceil((mx.y + MARGEM - y0) / CELULA_DA_BASE)) + 1
print('Chão de base: %d x %d pontos' % (nx, ny))
H = np.zeros((ny, nx)); conhecido = np.zeros((ny, nx), bool)
for j in range(ny):
    for i in range(nx):
        x, y = x0 + i * CELULA_DA_BASE, y0 + j * CELULA_DA_BASE
        # o mínimo em volta do ponto: a base nunca aparece por cima do chão do modelo
        zs = [z for z in (altura(a, x + dx, y + dy) for a in (arv_chao_ror, arv_terreno)
                          for dx in (-CELULA_DA_BASE, 0, CELULA_DA_BASE)
                          for dy in (-CELULA_DA_BASE, 0, CELULA_DA_BASE)) if z is not None]
        if zs:
            H[j, i] = min(zs) - BASE_ABAIXO; conhecido[j, i] = True
H[~conhecido] = H[conhecido].mean()
for _ in range(1500):  # difusão: o vazio vira um deserto suave que continua o chão
    viz = (np.roll(H, 1, 0) + np.roll(H, -1, 0) + np.roll(H, 1, 1) + np.roll(H, -1, 1)) / 4
    viz[0, :] = H[1, :]; viz[-1, :] = H[-2, :]; viz[:, 0] = H[:, 1]; viz[:, -1] = H[:, -2]
    H = np.where(conhecido, H, viz)

me = bpy.data.meshes.new('base')
verts = [(x0 + i * CELULA_DA_BASE, y0 + j * CELULA_DA_BASE, H[j, i]) for j in range(ny) for i in range(nx)]
faces = [(j * nx + i, j * nx + i + 1, (j + 1) * nx + i + 1, (j + 1) * nx + i) for j in range(ny - 1) for i in range(nx - 1)]
me.from_pydata(verts, [], faces)
uv = me.uv_layers.new()
for lp in me.loops:
    v = verts[lp.vertex_index]
    uv.data[lp.index].uv = (v[0] / 60.0, v[1] / 60.0)  # a areia do Cars 2 repete a cada 60 m
mat_areia = next(m for m in bpy.data.materials if m.use_nodes and any(
    n.type == 'TEX_IMAGE' and n.image and n.image.name == 'c2_' + AREIA_C2 for n in m.node_tree.nodes))
me.materials.append(mat_areia)
base = bpy.data.objects.new('base_deserto', me); bpy.context.scene.collection.objects.link(base)


def altura_base(x, y):
    fi = (x - x0) / CELULA_DA_BASE; fj = (y - y0) / CELULA_DA_BASE
    i = int(np.clip(fi, 0, nx - 2)); j = int(np.clip(fj, 0, ny - 2)); u = fi - i; v = fj - j
    return (H[j, i] * (1 - u) * (1 - v) + H[j, i + 1] * u * (1 - v) + H[j + 1, i] * (1 - u) * v + H[j + 1, i + 1] * u * v)


# 7b. A emenda: entre a cidade e a pista de treino, uma malha fina na altura do chão em volta.
# Onde há chão (o da pista de treino, o da cidade como era antes do corte, as ruas), ela fica logo
# embaixo, escondida; nos buracos (o chão da cidade cortado embaixo dos paredões, as frestas entre
# as duas), ela aparece, plana, continuando o chão dos lados. Na borda, encontra o chão de base
def chao_plano(arv, x, y):
    hit = arv.ray_cast(Vector((x, y, 9000.0)), Vector((0, 0, -1)))
    return hit[0].z if hit[0] is not None and hit[1].z > CHAO_INCLINADO else None


arv_ruas_e = arvore(ruas_ror)
# o piso da pista de treino: só o terreno (as películas estão assentadas nele; os paredões têm faces
# planas enterradas, bem abaixo do chão, que não podem ensinar altura à emenda)
def piso_do_treino(x, y):
    hit = arv_terreno.ray_cast(Vector((x, y, 9000.0)), Vector((0, 0, -1)))
    if hit[0] is None or hit[1].z < CHAO_INCLINADO:
        return None
    return hit[0].z
mn_e, mx_e = limites([o for o in c2 if o not in vegetacao_c2])
ex0, ey0 = mn_e.x - MARGEM_DA_EMENDA, mn_e.y - MARGEM_DA_EMENDA
enx = int(math.ceil((mx_e.x + MARGEM_DA_EMENDA - ex0) / CELULA_DA_EMENDA)) + 1
eny = int(math.ceil((mx_e.y + MARGEM_DA_EMENDA - ey0) / CELULA_DA_EMENDA)) + 1
E = np.zeros((eny, enx)); conhecidoE = np.zeros((eny, enx), bool)
for j in range(eny):
    for i in range(enx):
        x, y = ex0 + i * CELULA_DA_EMENDA, ey0 + j * CELULA_DA_EMENDA
        t = piso_do_treino(x, y)
        if t is None:
            # o chão da cidade DEPOIS do corte: onde ele foi cortado é buraco, e a emenda aparece
            zs = [z for z in (chao_plano(arv_chao_ror, x, y), chao_plano(arv_ruas_e, x, y)) if z is not None]
            t = max(zs) if zs else None
        if t is not None:
            E[j, i] = t - EMENDA_ABAIXO; conhecidoE[j, i] = True
        elif i in (0, enx - 1) or j in (0, eny - 1):
            E[j, i] = altura_base(x, y); conhecidoE[j, i] = True   # a borda encontra o chão de base
buracoE = ~conhecidoE
E[buracoE] = E[conhecidoE].mean()
for _ in range(4000):
    viz = (np.roll(E, 1, 0) + np.roll(E, -1, 0) + np.roll(E, 1, 1) + np.roll(E, -1, 1)) / 4
    E = np.where(conhecidoE, E, viz)
# Só sai malha onde ela aparece: nos buracos e numa faixa em volta
visivel = buracoE   # uma face sai se tocar um buraco (as que só têm cantos com chão ficam escondidas por ele)
indice = -np.ones((eny, enx), int)
vs, fs = [], []
for j in range(eny - 1):
    for i in range(enx - 1):
        if visivel[j, i] or visivel[j, i + 1] or visivel[j + 1, i] or visivel[j + 1, i + 1]:
            canto = []
            for jj, ii in ((j, i), (j, i + 1), (j + 1, i + 1), (j + 1, i)):
                if indice[jj, ii] < 0:
                    indice[jj, ii] = len(vs)
                    vs.append((ex0 + ii * CELULA_DA_EMENDA, ey0 + jj * CELULA_DA_EMENDA, E[jj, ii]))
                canto.append(indice[jj, ii])
            fs.append(tuple(canto))
me = bpy.data.meshes.new('emenda'); me.from_pydata(vs, [], fs)
uv = me.uv_layers.new()
for lp in me.loops:
    v = vs[lp.vertex_index]
    uv.data[lp.index].uv = (v[0] / 60.0, v[1] / 60.0)
me.materials.append(mat_areia)
emenda = bpy.data.objects.new('emenda_treino', me); bpy.context.scene.collection.objects.link(emenda)
print('Emenda: %d x %d pontos, %d buracos tapados com %d faces' % (enx, eny, int(buracoE.sum()), len(fs)))

# O chão de base, dentro da região da emenda, fica sempre embaixo dela (senão ele, que segue o mínimo em
# volta numa grade grossa, aparecia por cima da emenda e fazia degrau)
rebaixados = 0
for j in range(1, ny - 1):
    for i in range(1, nx - 1):
        x, y = x0 + i * CELULA_DA_BASE, y0 + j * CELULA_DA_BASE
        if ex0 < x < ex0 + (enx - 1) * CELULA_DA_EMENDA and ey0 < y < ey0 + (eny - 1) * CELULA_DA_EMENDA:
            fi = (x - ex0) / CELULA_DA_EMENDA; fj = (y - ey0) / CELULA_DA_EMENDA
            ii = int(min(fi, enx - 2)); jj = int(min(fj, eny - 2))
            menor = float(E[jj:jj + 2, ii:ii + 2].min())
            # o menor da emenda em volta (uma célula da base cobre várias da emenda)
            r = int(CELULA_DA_BASE / CELULA_DA_EMENDA) + 1
            menor = float(E[max(0, jj - r):jj + r + 1, max(0, ii - r):ii + r + 1].min())
            if H[j, i] > menor - BASE_EMBAIXO_DA_EMENDA:
                H[j, i] = menor - BASE_EMBAIXO_DA_EMENDA
                base.data.vertices[j * nx + i].co.z = H[j, i]
                rebaixados += 1
print('Chão de base: %d pontos rebaixados para baixo da emenda' % rebaixados)


def altura_emenda(x, y):
    fi = (x - ex0) / CELULA_DA_EMENDA; fj = (y - ey0) / CELULA_DA_EMENDA
    if fi < 0 or fj < 0 or fi > enx - 1 or fj > eny - 1:
        return None
    i = int(min(fi, enx - 2)); j = int(min(fj, eny - 2)); u = fi - i; v = fj - j
    return float(E[j, i] * (1 - u) * (1 - v) + E[j, i + 1] * u * (1 - v) + E[j + 1, i] * (1 - u) * v + E[j + 1, i + 1] * u * v)


def chao_em(x, y):
    zs = [z for z in (altura(arv_chao_ror, x, y), altura(arv_terreno, x, y)) if z is not None]
    e = altura_emenda(x, y)
    if e is not None:
        zs.append(e + EMENDA_ABAIXO)
    return max(zs + [altura_base(x, y) + BASE_ABAIXO])


# 7b2. Saias: o chão da cidade é uma casca aberta. Onde a borda dela fica bem acima do que há embaixo
# (o corte em volta da pista de treino, a beira de uma encosta), o carro, vindo de baixo, passaria por
# baixo do chão. A borda ganha uma parede de pedra até o chão de baixo: fecha o buraco na tela e é
# parede na colisão
arv_embaixo = arvore([base, emenda] + [o for o in c2 if o not in vegetacao_c2] + ror)   # tudo: uma encosta de pedra embaixo da borda já fecha
mat_pedra = next(m for m in bpy.data.materials if m.use_nodes and any(
    n.type == 'TEX_IMAGE' and n.image and n.image.name == 'ror_Image_0' for n in m.node_tree.nodes))
vs, fs, uvs = [], [], []
for o in chao_ror + terreno_c2:
    bm = bmesh.new(); bm.from_mesh(o.data)
    for e in bm.edges:
        if len(e.link_faces) != 1:
            continue
        a, b = e.verts[0].co.copy(), e.verts[1].co.copy()
        d = b - a; d.z = 0
        if d.length < 1e-3:
            continue
        # para fora: perpendicular à borda, vista de cima, do lado oposto ao da face
        fora = Vector((d.y, -d.x, 0)).normalized()
        if fora.dot((a + b) / 2 - e.link_faces[0].calc_center_median()) < 0:
            fora = -fora
        # a borda em pedaços de ~2 m: cada pedaço com a queda dele (uma borda inclinada pode ter uma
        # ponta apoiada e a outra no ar)
        if (((a + b) / 2).xy - oval.xy).length > RAIO_DAS_ROCHAS + 40.0:
            continue   # só as bordas que o recorte da pista de treino criou: o resto do mapa fica como era
        pedacos = max(1, int(math.ceil(d.length / PEDACO_DA_SAIA)))
        for k in range(pedacos):
            p0, p1 = a.lerp(b, k / pedacos), a.lerp(b, (k + 1) / pedacos)
            meio = (p0 + p1) / 2 + fora * 1.0
            hit = arv_embaixo.ray_cast(Vector((meio.x, meio.y, max(p0.z, p1.z) - 0.05)), Vector((0, 0, -1)))
            if hit[0] is None:
                continue
            if (p0.z + p1.z) / 2 - hit[0].z < SAIA_A_PARTIR_DE:
                continue
            baixo = hit[0].z - 0.2
            n = len(vs)
            vs += [(p0.x, p0.y, p0.z), (p1.x, p1.y, p1.z), (p1.x, p1.y, baixo), (p0.x, p0.y, baixo)]
            fs.append((n, n + 1, n + 2, n + 3))
            u0 = (p0 - a).length / 20; u1 = (p1 - a).length / 20
            uvs += [(u0, p0.z / 20), (u1, p1.z / 20), (u1, baixo / 20), (u0, baixo / 20)]
    bm.free()
saias = None
if fs:
    me = bpy.data.meshes.new('saias'); me.from_pydata(vs, [], fs)
    uv = me.uv_layers.new()
    for lp in me.loops:
        uv.data[lp.index].uv = uvs[lp.vertex_index]
    me.materials.append(mat_pedra)
    saias = bpy.data.objects.new('saias_do_chao', me); bpy.context.scene.collection.objects.link(saias)
print('Saias: %d bordas de chão fechadas até o chão de baixo' % len(fs))

# 7c. A estrada de ligação: da estrada do Race-O-Rama mais perto até a borda da arena, em linha
# reta, se o caminho não atravessar nada
estradas = [o for o in ror if imagens_do(o) & {'ror_' + n for n in ESTRADA_ROR}]
inicio = min((v.co.copy() for o in estradas for v in o.data.vertices), key=lambda p: (p.xy - oval.xy).length)
inicio.z = 0
# o fim: andando do oval para a cidade, o último ponto que ainda é terreno da arena
direcao = (inicio - oval); direcao.z = 0; direcao.normalize()
fim = oval.copy()
for k in range(1, 400):
    p = oval + direcao * (k * 5.0)
    if altura(arv_terreno, p.x, p.y) is None:
        fim = oval + direcao * max(0.0, (k - 1) * 5.0 - 10.0)   # termina 10 m dentro do platô, não na fresta
        break
arv_c2_solidos = arvore([o for o in c2 if o not in vegetacao_c2 and o not in terreno_c2])
livre = True
comprimento = (fim - inicio).length
for k in range(int(comprimento / 10.0)):
    p = inicio.lerp(fim, k * 10.0 / comprimento)
    if (fim - p).length < 80 or (p - inicio).length < 40:
        continue
    for a in (arv_obst, arv_c2_solidos):
        h = altura(a, p.x, p.y)
        if h is not None and h > chao_em(p.x, p.y) + 1.5:
            livre = False
ligacao = None
if livre and comprimento > 50:
    n = int(comprimento / 6.0)
    pts = [inicio.lerp(fim, k / n) for k in range(n + 1)]
    zs = [chao_em(p.x, p.y) + ESTRADA_ACIMA for p in pts]
    for _ in range(20):
        zs = [zs[0]] + [(zs[k - 1] + zs[k] * 2 + zs[k + 1]) / 4 for k in range(1, len(zs) - 1)] + [zs[-1]]
    t = (fim - inicio).normalized()
    lado = Vector((t.y, -t.x, 0)) * (LARGURA_DA_LIGACAO / 2)
    vs, fs, uvs = [], [], []
    for k, p in enumerate(pts):
        d = (p - inicio).length
        vs += [(p.x - lado.x, p.y - lado.y, zs[k]), (p.x + lado.x, p.y + lado.y, zs[k])]
        uvs += [(0.0, d / LARGURA_DA_LIGACAO), (1.0, d / LARGURA_DA_LIGACAO)]
        if k:
            fs.append((2 * k - 2, 2 * k - 1, 2 * k + 1, 2 * k))
    me = bpy.data.meshes.new('ligacao'); me.from_pydata(vs, [], fs)
    uv = me.uv_layers.new()
    for lp in me.loops:
        uv.data[lp.index].uv = uvs[lp.vertex_index]
    mat_estrada = next(m for m in bpy.data.materials if m.use_nodes and any(
        n.type == 'TEX_IMAGE' and n.image and n.image.name == 'ror_Image_30' for n in m.node_tree.nodes))
    me.materials.append(mat_estrada)
    ligacao = bpy.data.objects.new('estrada_ligacao', me); bpy.context.scene.collection.objects.link(ligacao)
    print('Estrada de ligação: %.0f m' % comprimento)
else:
    print('Estrada de ligação: o caminho em linha reta atravessa alguma coisa; sem estrada')
for m in bpy.data.materials:
    if m.use_nodes:
        for n in m.node_tree.nodes:
            if n.type == 'TEX_IMAGE':
                n.extension = 'REPEAT'

# 7d. Os matinhos e cactos na areia da cidade: os moldes, girados, carimbados numa grade, cada
# parte mantida com a probabilidade da densidade dali. Fora de rua, prédio, rocha e da arena
# (que já tem os dela)
arv_tudo = arvore(ror + [o for o in c2 if o not in vegetacao_c2] + [base, emenda] + ([ligacao] if ligacao else []) + ([saias] if saias else []))
arv_areia = arvore(chao_ror + [base, emenda])
arv_ruas_mato = arvore(ruas_ror + ([ligacao] if ligacao else []))


def solo_livre(x, y, exigir_areia):
    hit = arv_areia.ray_cast(Vector((x, y, 9000.0)), Vector((0, 0, -1)))
    if hit[0] is None or (exigir_areia and hit[1].z < 0.9):
        return None
    topo = arv_tudo.ray_cast(Vector((x, y, 9000.0)), Vector((0, 0, -1)))
    if topo[0] is not None and topo[0].z > hit[0].z + 0.15:
        return None   # tem alguma coisa em cima da areia: prédio, rocha
    rua = arv_ruas_mato.ray_cast(Vector((x, y, 9000.0)), Vector((0, 0, -1)))
    if rua[0] is not None and rua[0].z > hit[0].z - 0.5:
        return None   # rua, calçada ou a estrada de ligação (rentes à areia: a regra de cima não as pega)
    return hit[0].z


def densidade(d):
    for limite, fracao in DENSIDADE_DO_MATO:
        if d <= limite:
            return fracao
    return 0.0


sorteio = np.random.default_rng(7)
mn_r, mx_r = limites(chao_ror)
por_material = {}
carimbadas = 0
cy = mn_r.y
while cy <= mx_r.y:
    cx = mn_r.x
    while cx <= mx_r.x:
        centro = Vector((cx + sorteio.uniform(-15, 15), cy + sorteio.uniform(-15, 15), 0))
        fracao = densidade((centro - cidade).length)
        meio_do_molde, partes = moldes[int(sorteio.integers(len(moldes)))]
        rot = Matrix.Rotation(float(sorteio.uniform(0, 2 * math.pi)), 3, 'Z')
        for pt in partes:
            if sorteio.random() > fracao:
                continue
            c = centro + rot @ Vector((pt['centro'].x - meio_do_molde.x, pt['centro'].y - meio_do_molde.y, 0))
            if altura(arv_terreno, c.x, c.y) is not None:
                continue
            chao = solo_livre(c.x, c.y, True)
            if chao is None or any(solo_livre(c.x + dx, c.y + dy, False) is None for dx, dy in
                                   ((LONGE_DA_RUA, 0), (-LONGE_DA_RUA, 0), (0, LONGE_DA_RUA), (0, -LONGE_DA_RUA))):
                continue
            lista = por_material.setdefault(pt['material'], ([], [], []))
            for cantos, uvs in pt['faces']:
                ini = len(lista[0])
                for p in cantos:
                    q = rot @ Vector((p.x - pt['centro'].x, p.y - pt['centro'].y, 0))
                    lista[0].append((c.x + q.x, c.y + q.y, chao + pt['acima'] + (p.z - pt['base'])))
                lista[1].append(tuple(range(ini, ini + len(cantos))))
                lista[2].extend(uvs)
            carimbadas += 1
        cx += JANELA_DO_MATO
    cy += JANELA_DO_MATO
mato_da_cidade = []
for material, (vs, fs, uvs) in por_material.items():
    me = bpy.data.meshes.new('mato'); me.from_pydata(vs, [], fs)
    camada = me.uv_layers.new()
    camada.data.foreach_set('uv', [c for uv in uvs for c in uv])
    if material:
        me.materials.append(material)
    o = bpy.data.objects.new('mato_cidade', me); bpy.context.scene.collection.objects.link(o)
    mato_da_cidade.append(o)
print('Mato na cidade: %d partes, %d faces' % (carimbadas, sum(len(o.data.polygons) for o in mato_da_cidade)))

# ------------------------------------------------------------------
# 8a. Colisão: tudo menos vegetação e placas que flutuam, com o tipo de chão de cada triângulo
# ------------------------------------------------------------------
def tem_colisao(o):
    if o.name.startswith('c2_'):
        return o not in vegetacao_c2
    if o.name.startswith('ror_'):
        zs = [v.co.z for v in o.data.vertices]
        if not zs:
            return False
        xs = [v.co.x for v in o.data.vertices]; ys = [v.co.y for v in o.data.vertices]
        tamanho = max(max(xs) - min(xs), max(ys) - min(ys)) / ESCALA_ROR
        if min(zs) / ESCALA_ROR > FLUTUA_ACIMA and tamanho < FLUTUANTE_MAXIMO:
            return False
    return True


paleta, indice_da_imagem = [], {}


def entrada_da_paleta(im, plana=False):
    nome = im.name if im else ''
    tipo = TIPOS.get(nome, CHAO_OUTRO)
    if plana and tipo == CHAO_ROCHA:
        tipo, nome = CHAO_TERRA, nome + '#plana'   # o chão vermelho em volta dos paredões: terra, levanta poeira
    if nome not in indice_da_imagem:
        cor = (128, 128, 128)
        if im:
            p = pixels(im)
            p = p[p[:, 3] > 0.5] if (p[:, 3] > 0.5).any() else p
            cor = tuple(int(round(c * 255)) for c in p[:, :3].mean(0))
        indice_da_imagem[nome] = len(paleta)
        paleta.append((tipo,) + cor)
    return indice_da_imagem[nome]


solidos = [o for o in ror + c2 if tem_colisao(o)] + [base, emenda] + ([ligacao] if ligacao else []) + ([saias] if saias else [])
tris, chaos = [], []
for o in solidos:
    imagens = [imagem_do_material(m) for m in o.data.materials] or [None]
    bm = bmesh.new(); bm.from_mesh(o.data)
    bmesh.ops.triangulate(bm, faces=bm.faces)
    for f in bm.faces:
        a, b, c = (v.co for v in f.verts)
        if (b - a).cross(c - a).length < 1e-6:
            continue
        for v in (a, b, c):
            tris.extend((v.x, v.z, -v.y))  # eixos do jogo
        chaos.append(entrada_da_paleta(imagens[min(f.material_index, len(imagens) - 1)], f.normal.z > PLANA))
    bm.free()
arr = np.array(tris, dtype=np.float32).reshape(-1, 3)

largada_b = Vector((LARGADA_ROR[0] * ESCALA_ROR, LARGADA_ROR[1] * ESCALA_ROR, 0))
largada = (largada_b.x, chao_em(largada_b.x, largada_b.y), -largada_b.y, math.pi / 2)
arena = (oval.x, chao_em(oval.x, oval.y), -oval.y)
with open(SAIDA + '.col', 'wb') as f:
    f.write(b'KCOL'); f.write(struct.pack('<II', 2, len(arr) // 3))
    f.write(struct.pack('<6f', *arr.min(0), *arr.max(0)))
    f.write(struct.pack('<4f', *largada)); f.write(struct.pack('<3f', *arena))
    f.write(struct.pack('<I', len(paleta)))
    for e in paleta:
        f.write(struct.pack('<4B', *e))
    f.write(arr.tobytes())
    f.write(np.array(chaos, dtype=np.uint8).tobytes())
print('Colisão: %d triângulos, %d entradas na paleta (%s.col)' % (len(arr) // 3, len(paleta), SAIDA))
print('Largada', tuple(round(v, 1) for v in largada), '; arena', tuple(round(v, 1) for v in arena))

# ------------------------------------------------------------------
# 8b. O desenho
# ------------------------------------------------------------------
# A raylib guarda os índices de cada malha em 16 bits: nenhuma pode passar de 65535
# vértices. Quem passa é cortado ao meio (pelo eixo mais comprido) até caber.
MAXIMO_DE_CANTOS = 60000   # cantos de face: o exportador não cria mais vértices que isso


def dividir(o):
    if len(o.data.loops) <= MAXIMO_DE_CANTOS:
        return
    centros = [p.center.copy() for p in o.data.polygons]
    mn = Vector(map(min, *centros)); mx = Vector(map(max, *centros))
    eixo = max(range(3), key=lambda e: mx[e] - mn[e])
    corte = sorted(c[eixo] for c in centros)[len(centros) // 2]
    metades = []
    for lado in (0, 1):
        me = o.data.copy()
        bm = bmesh.new(); bm.from_mesh(me)
        fora = [f for f in bm.faces if (f.calc_center_median()[eixo] < corte) == (lado == 1)]
        bmesh.ops.delete(bm, geom=fora, context='FACES')
        bm.to_mesh(me); bm.free()
        nova = bpy.data.objects.new(o.name + ('_a' if lado == 0 else '_b'), me)
        bpy.context.scene.collection.objects.link(nova)
        metades.append(nova)
    bpy.data.objects.remove(o)
    for m in metades:
        dividir(m)


for o in list(bpy.data.objects):
    if o.type != 'MESH' or len(o.data.polygons) == 0:
        bpy.data.objects.remove(o)
for o in list(bpy.data.objects):
    dividir(o)
print('Malhas exportadas: %d' % len(bpy.data.objects))
bpy.ops.object.select_all(action='SELECT')
bpy.ops.export_scene.gltf(filepath=SAIDA + '.glb', export_format='GLB', use_selection=True,
                          export_yup=True, export_apply=True, export_animations=False,
                          export_normals=False)  # o jogo tira a direção da superfície das derivadas
print('Exportado', SAIDA + '.glb')

# Prévia: vista de cima, com a cor das texturas sem luz
if PREVIA:
    os.makedirs(PREVIA, exist_ok=True)
    for m in bpy.data.materials:
        if not m.use_nodes: continue
        nt = m.node_tree
        bsdf = next((n for n in nt.nodes if n.type == 'BSDF_PRINCIPLED'), None)
        saida = next((n for n in nt.nodes if n.type == 'OUTPUT_MATERIAL'), None)
        if not bsdf or not saida or not bsdf.inputs['Base Color'].links: continue
        em = nt.nodes.new('ShaderNodeEmission')
        nt.links.new(bsdf.inputs['Base Color'].links[0].from_socket, em.inputs['Color'])
        nt.links.new(em.outputs[0], saida.inputs[0])
    sc = bpy.context.scene
    sc.render.engine = 'BLENDER_EEVEE'; sc.view_settings.view_transform = 'Standard'
    w = bpy.data.worlds.new('ceu'); sc.world = w; w.use_nodes = True
    w.node_tree.nodes['Background'].inputs[0].default_value = (0.4, 0.55, 0.8, 1)
    cam = bpy.data.cameras.new('cam'); cam.clip_start = 0.5; cam.clip_end = 1e5
    co = bpy.data.objects.new('cam', cam); sc.collection.objects.link(co); sc.camera = co
    cam.type = 'ORTHO'; cam.ortho_scale = max(mx.x - mn.x, mx.y - mn.y) * 1.05
    co.location = ((mn.x + mx.x) / 2, (mn.y + mx.y) / 2, 5000); co.rotation_euler = (0, 0, 0)  # norte para cima
    sc.render.resolution_x, sc.render.resolution_y = 1600, 1200
    sc.render.filepath = os.path.join(PREVIA, 'cima.png'); bpy.ops.render.render(write_still=True)
