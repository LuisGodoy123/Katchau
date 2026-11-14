"""
montar_mundo_aberto.py
------------------------------------------------------------------
Junta os dois modelos de Radiator Springs num mapa só, para o passeio livre:

  - Race-O-Rama (PS3): a cidade, o Willy's Butte e os cânions. É a base.
  - Cars 2 (PS3), "Radiator Springs Arena": o oval de terra, a mesa com a
    estrada em espiral e os paredões vermelhos. Entra como região nova, no
    lugar livre mais perto da cidade, ligada a ela pela planície de areia.

O que o script faz:
  0. troca a torre de pneus do Luigi (cartões e discos achatados, no
     original) por pneus 3D, como a do filme: grupos de pneus deitados
     separados por coroas de pneus em pé, cada um meio torto, e a torre
     toda tombando como a de Pisa;
  1. põe os dois em metros (Race-O-Rama x4,5, Cars 2 x37,5: as estradas dos
     dois ficam do mesmo tamanho e o carro do jogo fica pequeno na cidade);
  2. tira os painéis de horizonte do Cars 2 (planos azulados em pé);
  3. procura, em volta da cidade, o lugar mais perto onde a arena cabe sem
     encostar em cânion, mesa, prédio ou estrada (girando de 90 em 90 graus);
  4. assenta no terreno as "películas" do Cars 2 (manchas, terra do butte,
     estradas), que no original flutuam uns centímetros: em escala, viravam
     beiradas de um metro onde o carro enroscava;
     Depois de posta a arena, as rochas dela (a mesa ao lado da pista de
     treino, o chão vermelho em volta, os paredões) ficam por peça: cada peça
     quase toda dentro do raio volta inteira (cortada face a face, a mesa
     virava um pente de triângulos soltos); a beirada baixa delas desce (ou
     sobe) aos poucos até o chão da cidade; e saem os cartões soltos em pé e
     os dentes que sobram nos cortes;
  5. corta o chão do Race-O-Rama embaixo da arena (sem dois chões um em cima
     do outro);
  6. puxa as cores do Race-O-Rama para as do Cars 2 (areia laranja, rochas
     vermelhas). Só os pixels "cor de areia" mudam: asfalto, faixas, prédios
     e placas ficam como estão;
  7. cria o chão de base (o deserto entre tudo) e, com COM_RUA_DO_TREINO
     (desligado: a pista de treino fica ligada à cidade pela areia), a rua
     da pista de treino (de asfalto, saindo em "T" da rua da cidade mais
     perto, reta até a curva de cima da pista de treino); e abre os prédios
     em que o carro entra (a Casa Della Tires, os cones e a recepção do Cozy
     Cone): tira da casca deles as portas pintadas (do desenho e da colisão:
     o carro entra dirigindo) e os vidros pintados (do desenho), e põe na
     colisão o lado de dentro de cada um (recursos/modelos/<interior>.json e
     <interior>_colisao.glb, das ferramentas montar_loja_do_luigi.py,
     montar_quartos_do_cozy_cone.py e montar_recepcao_do_cozy_cone.py; o
     desenho deles o jogo faz à parte, em src/interiores.c);
  7f. põe as calçadas da cidade no nível do asfalto: ampliadas, as guias do
     Race-O-Rama viravam degraus de 20 a 40 cm; a beira da calçada fica rente
     à rua e ela sobe devagar até a altura de antes só longe dela, perto dos
     prédios, e o que estava apoiado nela (paredes, cercas, postes) desce junto;
  8. passa as texturas JPEG para PNG (a raylib do jogo não lê JPEG: a pedra
     da mesa e as placas da cidade saíam cinza ou brancas) e exporta
     recursos/modelos/radiator_springs.glb (o desenho) e
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

import bpy, bmesh, sys, os, math, struct, tempfile, json
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
ALEM_DAS_ROCHAS = 90.0     # metros: uma peça de rocha quase toda dentro do raio fica inteira se não passar disso além dele
FAIXA_DAS_ROCHAS = 30.0    # metros: na beirada baixa de uma rocha, a faixa em que ela desce (ou sobe) até o chão da cidade
BEIRADA_BAIXA = 8.0        # metros acima do chão da cidade: uma beirada de rocha até essa altura encaixa no chão
ROCHA_BAIXA = 12.0         # metros acima do chão da cidade: até essa altura, a rocha acompanha o encaixe da beirada
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

# A rua da pista de treino: igual à rua principal (a mesma textura, a mesma largura e a mesma repetição)
COM_RUA_DO_TREINO = False            # a rua em "T" da cidade até a pista de treino (7c): tirada a pedido;
                                     # a pista fica ligada à cidade pela planície de areia
RUA_DA_CIDADE = 'ror_Image_61'       # o asfalto das ruas, com a faixa amarela
PISTA_DE_TREINO = 'c2_Image_6'       # a terra da pista de treino (RAD_BOSS_Road)
LARGURA_DA_RUA = 14.7                # metros, como a rua principal
METROS_POR_VOLTA_DA_RUA = 31.4       # a textura da rua principal se repete a cada tanto, no comprimento
BORDAS_DA_RUA = (0.065, 0.995)       # o u das duas beiras, como na rua principal
ACOSTAMENTO = 3.0                    # metros de areia descendo da beira da rua até o chão
RUA_ACIMA = 0.05                     # metros acima do ponto mais alto do chão embaixo de cada seção

# A torre de pneus do Luigi (no Race-O-Rama: x, y do pé dela, em unidades do original)
TORRE_DE_PNEUS = (107.8, -33.3)
RAIO_DA_TORRE = 1.0                  # unidades: as peças dela cabem neste raio
BASE_DA_TORRE = 0.5                  # unidades: abaixo disso é o pedestal (fica)
PNEUS_EM_PE = 4                      # pneus em pé em cada coroa (em cruz)
INCLINACAO_DA_TORRE = 0.16           # o topo sai do prumo esta fração da altura (como a de Pisa: uns 9 graus)
CURVA_DA_TORRE = 1.5                 # > 1: a torre vai cedendo, mais torta em cima
LADO_DA_INCLINACAO = (-0.5, 0.85)    # para onde ela tomba (no Blender): para a rua, um pouco para oeste,
                                     # de perfil para quem anda pela rua principal
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
AREIA_LISA_ROR = {'Image_2', 'Image_3'}   # o chão liso da cidade e dos morros (as outras areias são trilhas)
MAIOR_TEXTURA = 1024             # pixels: a maior textura que vai para o jogo (a chapa ondulada vinha com 2048)
METROS_DA_AREIA = 28.5           # a areia da arena repete a cada tantos metros (medido no terreno dela)
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


# 1b. A torre de pneus do Luigi, em 3D. No original ela é feita de discos achatados e cartões
# com a imagem de pneus em pé: de perto, chapada. As peças dela saem (o pedestal fica) e entra
# uma torre de pneus de verdade, como a do filme: com a altura e a largura da original, mas
# tombando como a de Pisa, em grupos de pneus deitados separados por coroas de pneus em pé
TONS_DE_PNEU = 4   # faixas da textura: preto, cinza, gasto e de faixa branca


def textura_de_pneu():
    """Quatro faixas na mesma imagem, uma por tom de pneu. Em cada uma, o v dá a volta na seção do
    pneu (0 = o meio da banda de rodagem, por fora; meia volta = por dentro, no aro) e o u dá a volta
    no pneu. A banda tem blocos grandes em duas fileiras desencontradas e um sulco no meio; o ombro,
    entalhes; o flanco, um anel em relevo e uma fila de "letras"; por dentro, o talão escuro"""
    L, A = 512, 512
    faixa = A // TONS_DE_PNEU
    img = np.zeros((A, L, 4), dtype=np.float32); img[..., 3] = 1.0
    u = (np.arange(L) + 0.5) / L
    fi = 2 * np.pi * ((np.arange(faixa) + 0.5) / faixa)
    F, U = np.meshgrid(fi, u, indexing='ij')
    c, s_ = np.cos(F), np.sin(F)
    rng = np.random.default_rng(95)
    ruido = rng.normal(0.0, 0.012, F.shape)
    # cinza-escuro, não preto: no sol do jogo, a borracha preta vira um borrão e os blocos somem
    tons = ((0.190, 0.182, 0.175), (0.255, 0.250, 0.245), (0.235, 0.212, 0.185), (0.170, 0.165, 0.160))
    for k in range(TONS_DE_PNEU):
        base = np.full(F.shape, 1.0)
        # a banda de rodagem: duas fileiras de blocos, desencontradas, com um sulco no meio
        banda = c > np.cos(0.70)
        metade = s_ > 0
        u_do_bloco = (U * 24 + np.where(metade, 0.0, 0.5)) % 1.0
        bloco = (u_do_bloco < 0.70) & (np.abs(s_) > 0.07)
        cor_banda = np.where(bloco, 1.45, 0.45)
        # o ombro: entalhes curtos
        ombro = (c <= np.cos(0.70)) & (c > np.cos(1.05))
        cor_ombro = np.where((U * 48) % 1.0 < 0.5, 1.15, 0.75)
        # o flanco: liso, com um anel em relevo e uma fila de "letras" (traços claros)
        flanco = (c <= np.cos(1.05)) & (c > np.cos(2.45))
        meio_do_flanco = np.abs(np.abs(F - np.pi) - (np.pi - 1.75))   # 0 no meio de cada flanco
        anel = np.abs(meio_do_flanco - 0.30) < 0.035
        letras = (np.abs(meio_do_flanco + 0.0) < 0.10) & ((U * 64) % 1.0 < 0.55) & ((U * 4) % 1.0 < 0.62)
        cor_flanco = 1.0 + 0.35 * anel + 0.45 * letras
        # por dentro: o talão, escuro
        dentro = c <= np.cos(2.45)
        cor = np.where(banda, cor_banda, np.where(ombro, cor_ombro, np.where(flanco, cor_flanco, 0.55)))
        rgb = np.stack([cor * tons[k][0], cor * tons[k][1], cor * tons[k][2]], -1) + ruido[..., None]
        if k == 3:   # a faixa branca, perto do aro, em cada flanco
            branca = flanco & (np.abs(meio_do_flanco - 0.45) < 0.16)
            rgb[branca] = (0.82, 0.81, 0.77)
        img[k * faixa:(k + 1) * faixa, :, :3] = np.clip(rgb, 0.0, 1.0)
    imagem = bpy.data.images.new('pneu_da_torre', L, A, alpha=False)
    imagem.pixels = img.ravel()
    imagem.file_format = 'PNG'
    imagem.pack()
    material = bpy.data.materials.new('pneu_da_torre')
    material.use_nodes = True
    no = material.node_tree.nodes.new('ShaderNodeTexImage'); no.image = imagem
    bsdf = next(n for n in material.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    material.node_tree.links.new(no.outputs['Color'], bsdf.inputs['Base Color'])
    return material


def pneu(bm, uv, centro, eixo, raio_externo, raio_do_aro, largura, tom, giro, voltas=40, lados=16):
    """Um pneu em volta do eixo dado: a seção é um retângulo de cantos bem redondos (a banda quase
    reta, os flancos estufados), do aro (por dentro) até o raio de fora, com a largura dada"""
    a = eixo.normalized(); u = a.orthogonal().normalized(); v = a.cross(u)
    meio = (raio_externo + raio_do_aro) / 2
    meia_espessura = (raio_externo - raio_do_aro) / 2
    vs = []
    for i in range(voltas):
        t = 2 * math.pi * i / voltas + giro
        para_fora = u * math.cos(t) + v * math.sin(t)
        for j in range(lados):
            f = 2 * math.pi * j / lados
            c, s = math.cos(f), math.sin(f)
            ce, se = math.copysign(abs(c) ** 0.45, c), math.copysign(abs(s) ** 0.6, s)
            estufa = 1.0 + 0.06 * (1.0 - abs(s)) * (1.0 if c > 0 else 0.0)   # o flanco estufa um pouco perto da banda
            vs.append(bm.verts.new(centro + para_fora * (meio + meia_espessura * ce * estufa) + a * (largura / 2 * se)))
    v0 = tom / TONS_DE_PNEU
    for i in range(voltas):
        for j in range(lados):
            i1, j1 = (i + 1) % voltas, (j + 1) % lados
            face = bm.faces.new((vs[i * lados + j], vs[i1 * lados + j], vs[i1 * lados + j1], vs[i * lados + j1]))
            for laco, (ii, jj) in zip(face.loops, ((i, j), (i + 1, j), (i + 1, j + 1), (i, j + 1))):
                laco[uv].uv = (2.0 * ii / voltas, v0 + (jj / lados) / TONS_DE_PNEU)


def trocar_torre_de_pneus():
    centro = Vector(TORRE_DE_PNEUS) * ESCALA_ROR
    raio = RAIO_DA_TORRE * ESCALA_ROR
    base = BASE_DA_TORRE * ESCALA_ROR
    tirados = []
    for o in ror:
        bm = bmesh.new(); bm.from_mesh(o.data); bm.faces.ensure_lookup_table()
        # as peças soltas inteiras dentro do cilindro, acima do pedestal (a busca começa só nas faces
        # de dentro, mas segue a peça inteira: um prédio que encoste no cilindro não é cortado)
        apagar = []
        vistas = set()
        for f in bm.faces:
            if f.index in vistas or (f.calc_center_median().xy - centro).length > raio:
                continue
            peca, fila = set(), [f]
            while fila:
                g = fila.pop()
                if g.index in peca:
                    continue
                peca.add(g.index)
                for e in g.edges:
                    for h in e.link_faces:
                        if h.index not in peca:
                            fila.append(h)
            vistas |= peca
            faces = [bm.faces[i] for i in peca]
            pontos = [vv.co for g in faces for vv in g.verts]
            if pontos and all((p.xy - centro).length < raio for p in pontos) and max(p.z for p in pontos) > base + 0.3:
                apagar += faces
        if apagar:
            tirados += [vv.co.copy() for g in apagar for vv in g.verts]
            bmesh.ops.delete(bm, geom=list(set(apagar)), context='FACES')
            bm.to_mesh(o.data)
        bm.free()
    if len(tirados) < 50:
        print('Torre de pneus: não achei a torre; fica a original')
        return None
    z0 = min(p.z for p in tirados)   # o topo do pedestal
    z1 = max(p.z for p in tirados)
    def meio_entre(za, zb):
        sel = [p for p in tirados if za <= p.z <= zb]
        return Vector(((min(p.x for p in sel) + max(p.x for p in sel)) / 2, (min(p.y for p in sel) + max(p.y for p in sel)) / 2))
    pe = meio_entre(z0, z0 + (z1 - z0) * 0.15)
    topo = meio_entre(z1 - (z1 - z0) * 0.15, z1)
    raio_externo = sorted((p.xy - pe).length for p in tirados if p.z < z0 + (z1 - z0) * 0.3)[-1] * 0.92
    bm = bmesh.new(); uv = bm.loops.layers.uv.new()
    total = z1 - z0
    sorteio = __import__('random').Random(95)   # a mesma torre a cada vez que o mapa é montado
    # As medidas, em raios da torre: o pneu deitado (o de baixo é o mais largo) e a coroa de pneus em pé
    altura_deitado = 0.62 * raio_externo
    raio_em_pe = 0.47 * raio_externo
    largura_em_pe = 0.50 * raio_externo
    # A sequência, de baixo para cima: grupos de 2 ou 3 deitados, e uma coroa entre um grupo e outro
    pecas = []
    h = 0.0
    while True:
        grupo = 3 if sorteio.random() < 0.5 else 2
        for _ in range(grupo):
            pecas.append(('deitado', h, altura_deitado)); h += altura_deitado
        if h + 2 * raio_em_pe + altura_deitado > total:
            break
        pecas.append(('coroa', h, 2 * raio_em_pe)); h += 2 * raio_em_pe
    k = total / h   # estica (ou encolhe) um pouco a sequência para fechar na altura da original
    # O eixo: tomba para LADO_DA_INCLINACAO, cada vez mais em cima (a torre cede)
    lado = Vector((LADO_DA_INCLINACAO[0], LADO_DA_INCLINACAO[1], 0.0)).normalized()
    tombo = INCLINACAO_DA_TORRE * total
    def no_eixo(altura):
        t = max(0.0, min(1.0, altura / total))
        return Vector((pe.x, pe.y, z0 + altura)) + lado * (tombo * t ** CURVA_DA_TORRE)
    def para_cima(altura):
        t = max(1e-3, min(1.0, altura / total))
        return (Vector((0.0, 0.0, 1.0)) + lado * (INCLINACAO_DA_TORRE * CURVA_DA_TORRE * t ** (CURVA_DA_TORRE - 1.0))).normalized()
    def torto(eixo, graus):
        """O eixo, inclinado um pouco para um lado qualquer (pneus empilhados à mão)"""
        lado_qualquer = eixo.orthogonal().normalized()
        lado_qualquer.rotate(Matrix.Rotation(sorteio.uniform(0, 2 * math.pi), 3, eixo))
        return (eixo + lado_qualquer * math.tan(math.radians(graus))).normalized()
    deitados = em_pe = 0
    for tipo, h0, altura in pecas:
        h0 *= k; altura *= k
        meio_h = h0 + altura / 2
        cima = para_cima(meio_h)
        if tipo == 'deitado':
            escala = sorteio.uniform(0.95, 1.03) * (1.04 if h0 == 0.0 else 1.0)
            desvio = cima.orthogonal().normalized() * sorteio.uniform(0.0, 0.05 * raio_externo)
            desvio.rotate(Matrix.Rotation(sorteio.uniform(0, 2 * math.pi), 3, cima))
            tom = 3 if sorteio.random() < 0.12 else sorteio.choice((0, 0, 1, 2))
            pneu(bm, uv, no_eixo(meio_h) + desvio, torto(cima, sorteio.uniform(0.0, 2.5)), raio_externo * escala,
                 raio_externo * 0.40 * escala, altura * 0.98, tom, sorteio.uniform(0, 2 * math.pi))
            deitados += 1
        else:
            meio = no_eixo(meio_h)
            frente = cima.orthogonal().normalized()
            frente.rotate(Matrix.Rotation(sorteio.uniform(0, math.pi / 2), 3, cima))
            for i in range(PNEUS_EM_PE):
                para_fora = frente.copy()
                para_fora.rotate(Matrix.Rotation(2 * math.pi * i / PNEUS_EM_PE, 3, cima))
                tom = 3 if sorteio.random() < 0.25 else sorteio.choice((0, 1, 2))
                r = raio_em_pe * k * sorteio.uniform(0.95, 1.02)
                pneu(bm, uv, meio + para_fora * (raio_externo - largura_em_pe / 2) - cima * (raio_em_pe * k - r),
                     torto(para_fora, sorteio.uniform(0.0, 4.0)), r, r * 0.45, largura_em_pe, tom,
                     sorteio.uniform(0, 2 * math.pi), voltas=32, lados=14)
                em_pe += 1
    me = bpy.data.meshes.new('torre_de_pneus'); bm.to_mesh(me); bm.free()
    me.materials.append(textura_de_pneu())
    torre = bpy.data.objects.new('ror_torre_de_pneus', me); bpy.context.scene.collection.objects.link(torre)
    print('Torre de pneus: %d pneus deitados e %d em pé, %.1f m de altura, %.1f m de largura, o topo %.1f m fora do prumo'
          % (deitados, em_pe, total, 2 * raio_externo, tombo))
    return torre


torre_de_pneus = trocar_torre_de_pneus()
if torre_de_pneus is not None:
    ror.append(torre_de_pneus)

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
# As rochas inteiras, antes do corte: a busca do lugar usa as rochas cortadas (como sempre, para a arena não
# mudar de lugar), e depois de posta a arena cada peça de rocha volta inteira ou sai inteira (3a)
rochas_inteiras = {o.name: o.data.copy() for o in c2 if o not in terreno_c2 and o not in vegetacao_c2}
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


# 3a. As rochas por peça, não por face: o corte face a face no raio das rochas retalhava o que ficava em
# cima da linha (a mesa ao lado da pista de treino virava um pente de triângulos soltos, e o chão vermelho
# e a estrada em volta dela, tiras penduradas). Cada peça (as faces ligadas por vértices) volta inteira se
# está quase toda dentro do raio e não passa muito dele; quase toda fora, sai inteira; grande demais, fica
# cortada como antes. As faces das peças inteiras ficam marcadas ("inteira"): o 4b não as poda
def pecas(bm):
    """As peças de uma malha: listas de faces ligadas por vértices na mesma posição (até 1 cm)"""
    pai = list(range(len(bm.faces)))
    def raiz(i):
        while pai[i] != i:
            pai[i] = pai[pai[i]]
            i = pai[i]
        return i
    dono = {}
    for f in bm.faces:
        for v in f.verts:
            chave = (round(v.co.x * 100), round(v.co.y * 100), round(v.co.z * 100))
            if chave in dono:
                a, b = raiz(dono[chave]), raiz(f.index)
                if a != b:
                    pai[a] = b
            else:
                dono[chave] = f.index
    grupos = {}
    for f in bm.faces:
        grupos.setdefault(raiz(f.index), []).append(f)
    return list(grupos.values())


posta = Matrix.Translation((0, 0, z_ror - z_c2)) @ Matrix.Translation(lugar) @ rot4 @ Matrix.Translation(-meio_c2)
voltaram = sairam = cortadas_ainda = 0
for o in [o for o in c2 if o not in terreno_c2 and o not in vegetacao_c2]:
    inteira = rochas_inteiras.get(o.name)
    if inteira is None:
        continue
    inteira.transform(posta)
    bm = bmesh.new(); bm.from_mesh(inteira)
    bm.faces.ensure_lookup_table()
    marca = bm.faces.layers.int.new('inteira')
    apagar = []
    for peca in pecas(bm):
        dentro = [f for f in peca if (f.calc_center_median().xy - oval.xy).length <= RAIO_DAS_ROCHAS]
        alcance = max((v.co.xy - oval.xy).length for f in peca for v in f.verts)
        if len(dentro) * 2 < len(peca):
            apagar += peca
            sairam += 1
        elif alcance <= RAIO_DAS_ROCHAS + ALEM_DAS_ROCHAS:
            for f in peca:
                f[marca] = 1
            voltaram += 1
        else:
            apagar += [f for f in peca if f not in dentro]
            cortadas_ainda += 1
    bmesh.ops.delete(bm, geom=apagar, context='FACES')
    bm.to_mesh(o.data); bm.free()
    bpy.data.meshes.remove(inteira)
print('Rochas da arena: %d peças inteiras, %d tiradas inteiras, %d grandes cortadas no raio' % (voltaram, sairam, cortadas_ainda))

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
    marca = bm.faces.layers.int.get('inteira')
    fora = [f for f in bm.faces if abs(f.normal.z) > 0.85 and f.calc_center_median().z < z_ror + 4.0
            and (f.calc_center_median().xy - oval.xy).length > RAIO_DO_TREINO
            and not (marca is not None and f[marca] == 1)]
    podados += len(fora)
    bmesh.ops.delete(bm, geom=fora, context='FACES')
    bm.to_mesh(o.data); bm.free()
print('Pisos soltos além do terreno: %d faces' % podados)

# 4c. Nas rochas: os cartões soltos em pé (um painel de pedra de dois triângulos, fundo de cena do jogo
# original, plantado no meio do deserto) e os dentes que ainda sobram nos cortes (faces presas por um lado só)
rochas_c2 = [o for o in c2 if o not in terreno_c2 and o not in vegetacao_c2]
cartoes = dentes = 0
for o in rochas_c2:
    bm = bmesh.new(); bm.from_mesh(o.data)
    bm.faces.ensure_lookup_table(); bm.normal_update()
    apagar = set()
    for peca in pecas(bm):
        if len(peca) <= 4 and all(abs(f.normal.z) < 0.3 for f in peca) and sum(f.calc_area() for f in peca) > 30.0:
            apagar.update(peca)
            cartoes += 1
    contagem = {}
    def chave(v):
        return (round(v.co.x * 100), round(v.co.y * 100), round(v.co.z * 100))
    for f in bm.faces:
        for e in f.edges:
            k = frozenset((chave(e.verts[0]), chave(e.verts[1])))
            contagem[k] = contagem.get(k, 0) + 1
    for f in bm.faces:
        livres = sum(1 for e in f.edges if contagem[frozenset((chave(e.verts[0]), chave(e.verts[1])))] == 1)
        if livres >= 2 and abs(f.normal.z) < 0.9 and f.calc_area() < 200.0 and f not in apagar:
            apagar.add(f)
            dentes += 1
    bmesh.ops.delete(bm, geom=list(apagar), context='FACES')
    bm.to_mesh(o.data); bm.free()
print('Rochas: %d cartões soltos e %d dentes tirados' % (cartoes, dentes))

# 4d. A beirada baixa das rochas encaixa no chão da cidade: perto de uma beirada que termina no ar (ou
# enterrada), a rocha baixa desce (ou sobe) aos poucos até a altura que o chão da cidade tinha ali. Sem isso,
# o chão vermelho em volta da mesa acabava num degrau, e do lado de lá, num buraco. A beirada que fica em
# cima de outra rocha (a estrada pintada sobre o chão vermelho) não conta; a mesma conta vale para tudo o
# que está no mesmo lugar, e a estrada continua colada no chão. A terra da pista de treino e o que sobrou do
# butte no meio do oval (a pedra, o chão e a estrada dele) também não são o chão da arena, mas não são beirada:
# ficam de fora. Embaixo do oval a cidade tinha um morro (até 26 m), e puxadas para ele as bordas da pista
# viravam paredes em pé nas retas
from mathutils.kdtree import KDTree
NAO_ENCAIXAM = ('BOSS_Road', 'Butte')   # nos nomes dos materiais
rochas_de_beirada = [o for o in rochas_c2
                     if not any(m and any(n in m.name for n in NAO_ENCAIXAM) for m in o.data.materials)]
arv_rochas_c2 = arvore(rochas_c2)
beiradas = []
for o in rochas_de_beirada:
    me = o.data
    def chave_v(i):
        c = me.vertices[i].co
        return (round(c.x * 100), round(c.y * 100), round(c.z * 100))
    contagem = {}
    for p in me.polygons:
        vs = list(p.vertices)
        for a, b in zip(vs, vs[1:] + vs[:1]):
            k = frozenset((chave_v(a), chave_v(b)))
            contagem[k] = contagem.get(k, 0) + 1
    vistos = set()
    for p in me.polygons:
        vs = list(p.vertices)
        for a, b in zip(vs, vs[1:] + vs[:1]):
            if contagem[frozenset((chave_v(a), chave_v(b)))] != 1:
                continue
            for i in (a, b):
                if i in vistos:
                    continue
                vistos.add(i)
                c = me.vertices[i].co
                chao = altura(arv_chao_busca, c.x, c.y)
                chao = chao if chao is not None else z_ror
                if c.z > chao + BEIRADA_BAIXA:
                    continue
                embaixo = arv_rochas_c2.ray_cast(Vector((c.x, c.y, c.z - 0.2)), Vector((0, 0, -1)), 1.5)
                if embaixo[0] is None:
                    beiradas.append(c.copy())
encaixados_r = 0
if beiradas:
    kd = KDTree(len(beiradas))
    for i, b in enumerate(beiradas):
        kd.insert(Vector((b.x, b.y, 0.0)), i)
    kd.balance()
    for o in rochas_de_beirada:
        for v in o.data.vertices:
            chao = altura(arv_chao_busca, v.co.x, v.co.y)
            chao = chao if chao is not None else z_ror
            if v.co.z > chao + ROCHA_BAIXA:
                continue
            _, _, d = kd.find(Vector((v.co.x, v.co.y, 0.0)))
            if d < FAIXA_DAS_ROCHAS:
                t = degrau_suave(0.0, FAIXA_DAS_ROCHAS, d)
                v.co.z = chao * (1.0 - t) + v.co.z * t
                encaixados_r += 1
print('Rochas: %d pontos de beirada, %d vértices encaixados no chão da cidade' % (len(beiradas), encaixados_r))

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

# 6b. O chão liso da cidade fica com a areia da arena (a do Race-O-Rama, mesmo recolorida, é lisa e
# borrada; a da arena tem pedrinhas e manchas). As coordenadas saem da posição, na escala da arena,
# projetando cada face no plano em que ela mais se deita (as encostas não esticam). As trilhas
# (as areias com marcas de pneu) ficam como estão
mat_areia_da_arena = next(m for m in bpy.data.materials if m.use_nodes and any(
    n.type == 'TEX_IMAGE' and n.image and n.image.name == 'c2_' + TERRENO_C2 for n in m.node_tree.nodes))
lisas = {'ror_' + n for n in AREIA_LISA_ROR}
trocadas = 0
for o in chao_ror:
    me = o.data
    de_areia_lisa = {i for i, m in enumerate(me.materials) if (imagem_do_material(m) is not None and imagem_do_material(m).name in lisas)}
    if not de_areia_lisa or me.uv_layers.active is None:
        continue
    if mat_areia_da_arena.name not in [m.name for m in me.materials if m]:
        me.materials.append(mat_areia_da_arena)
    nova = [m.name if m else None for m in me.materials].index(mat_areia_da_arena.name)
    camada = me.uv_layers.active.data
    for p in me.polygons:
        if p.material_index not in de_areia_lisa:
            continue
        p.material_index = nova
        nx_, ny_, nz_ = (abs(c) for c in p.normal)
        for laco in p.loop_indices:
            co = me.vertices[me.loops[laco].vertex_index].co
            if nz_ >= max(nx_, ny_) * 0.8:
                camada[laco].uv = (co.x / METROS_DA_AREIA, co.y / METROS_DA_AREIA)
            elif nx_ > ny_:
                camada[laco].uv = (co.y / METROS_DA_AREIA, co.z / METROS_DA_AREIA)
            else:
                camada[laco].uv = (co.x / METROS_DA_AREIA, co.z / METROS_DA_AREIA)
        trocadas += 1
print('Chão da cidade: %d faces com a areia da arena' % trocadas)

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
    uv.data[lp.index].uv = (v[0] / METROS_DA_AREIA, v[1] / METROS_DA_AREIA)  # como na arena
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
    uv.data[lp.index].uv = (v[0] / METROS_DA_AREIA, v[1] / METROS_DA_AREIA)
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

# 7c. A rua da pista de treino: sai em "T" da rua da cidade mais perto da pista de treino, reta e
# perpendicular a ela, até a beira da curva de cima da pista. É igual à rua principal (o mesmo asfalto
# com a faixa amarela, a mesma largura). O leito acompanha o chão (cada seção na altura do ponto mais
# alto embaixo dela, sem descer em nenhum ponto) e um acostamento de areia desce das beiras até o chão,
# sem beirada no ar
ruas_da_cidade = [o for o in ror if RUA_DA_CIDADE in imagens_do(o)]
pistas_do_treino = [o for o in c2 if PISTA_DE_TREINO in imagens_do(o)]
ligacao = None
if COM_RUA_DO_TREINO and ruas_da_cidade and pistas_do_treino:
    arv_rua = arvore(ruas_da_cidade)
    arv_pista = arvore(pistas_do_treino)
    melhor = None
    for o in pistas_do_treino:
        for v in o.data.vertices:
            perto = arv_rua.find_nearest(v.co)
            if perto[0] is not None:
                d = (perto[0].xy - v.co.xy).length
                if melhor is None or d < melhor[0]:
                    melhor = (d, perto[0].copy(), v.co.copy())
    _, inicio, fim = melhor
    inicio.z = fim.z = 0.0
    direcao = fim - inicio
    comprimento = direcao.length
    direcao.normalize()
    normal = Vector((direcao.y, -direcao.x, 0.0))
    meia = LARGURA_DA_RUA / 2
    n = max(2, int(comprimento / 2.0))
    centros = [inicio + direcao * (comprimento * k / n) for k in range(n + 1)]
    chao = [max(chao_em(c.x + normal.x * t, c.y + normal.y * t) for t in (-meia, -meia / 2, 0.0, meia / 2, meia)) + RUA_ACIMA
            for c in centros]
    # Nas pontas, a altura da rua da cidade e a da pista (sem degrau), misturando nos primeiros e últimos 10 m
    na_rua = altura(arv_rua, inicio.x, inicio.y)
    na_pista = altura(arv_pista, fim.x, fim.y)
    for k, c in enumerate(centros):
        s_ = comprimento * k / n
        if na_rua is not None and s_ < 10.0:
            chao[k] = na_rua + (chao[k] - na_rua) * (s_ / 10.0)
        if na_pista is not None and comprimento - s_ < 10.0:
            chao[k] = na_pista + (chao[k] - na_pista) * ((comprimento - s_) / 10.0)
    alturas = list(chao)
    for _ in range(30):   # suaviza só subindo: a rua não afunda no chão em ponto nenhum
        alturas = [alturas[0]] + [(alturas[k - 1] + 2 * alturas[k] + alturas[k + 1]) / 4 for k in range(1, n)] + [alturas[-1]]
        alturas = [max(a, b) for a, b in zip(alturas, chao)]
    vs, fs, uvs, mats = [], [], [], []
    for k, c in enumerate(centros):
        z = alturas[k]
        fora_e = c - normal * (meia + ACOSTAMENTO); fora_d = c + normal * (meia + ACOSTAMENTO)
        vs += [(fora_e.x, fora_e.y, chao_em(fora_e.x, fora_e.y) - 0.15),
               (c.x - normal.x * meia, c.y - normal.y * meia, z),
               (c.x + normal.x * meia, c.y + normal.y * meia, z),
               (fora_d.x, fora_d.y, chao_em(fora_d.x, fora_d.y) - 0.15)]
    for k in range(n):
        a, b = 4 * k, 4 * (k + 1)
        fs += [(a, a + 1, b + 1, b), (a + 1, a + 2, b + 2, b + 1), (a + 2, a + 3, b + 3, b + 2)]
        mats += [1, 0, 1]
    me = bpy.data.meshes.new('rua_do_treino'); me.from_pydata(vs, [], fs)
    uv = me.uv_layers.new()
    for p, m in zip(me.polygons, mats):
        p.material_index = m
        for laco in p.loop_indices:
            vi = me.loops[laco].vertex_index
            k, coluna = divmod(vi, 4)
            if m == 0:
                uv.data[laco].uv = (BORDAS_DA_RUA[0] if coluna == 1 else BORDAS_DA_RUA[1], -(comprimento * k / n) / METROS_POR_VOLTA_DA_RUA)
            else:
                x, y, _ = vs[vi]
                uv.data[laco].uv = (x / METROS_DA_AREIA, y / METROS_DA_AREIA)
    mat_rua = next(m for o in ruas_da_cidade for m in o.data.materials
                   if m and m.use_nodes and any(x.type == 'TEX_IMAGE' and x.image and x.image.name == RUA_DA_CIDADE for x in m.node_tree.nodes))
    me.materials.append(mat_rua)
    me.materials.append(emenda.data.materials[0])
    ligacao = bpy.data.objects.new('rua_do_treino', me); bpy.context.scene.collection.objects.link(ligacao)
    print('Rua da pista de treino: %.0f m, de (%.0f, %.0f) a (%.0f, %.0f) no jogo' % (comprimento, inicio.x, -inicio.y, fim.x, -fim.y))
    # Os matinhos e cactos da arena que caíram em cima da rua (no platô) saem, cada moita inteira
    faixa = meia + ACOSTAMENTO + 1.5
    def na_rua(p):
        d = p - inicio
        ao_longo = d.x * direcao.x + d.y * direcao.y
        return -2.0 < ao_longo < comprimento + 2.0 and abs(d.x * normal.x + d.y * normal.y) < faixa
    tiradas = 0
    for o in vegetacao_c2:
        bm = bmesh.new(); bm.from_mesh(o.data); bm.faces.ensure_lookup_table()
        apagar, vistas = [], set()
        for f in bm.faces:
            if f.index in vistas or not na_rua(f.calc_center_median()):
                continue
            moita, fila = set(), [f]
            while fila:
                g = fila.pop()
                if g.index in moita:
                    continue
                moita.add(g.index)
                for e in g.edges:
                    fila += [h for h in e.link_faces if h.index not in moita]
            vistas |= moita
            faces = [bm.faces[i] for i in moita]
            meio = sum((v.co for g in faces for v in g.verts), Vector()) / sum(len(g.verts) for g in faces)
            if na_rua(meio):
                apagar += faces
                tiradas += 1
        if apagar:
            bmesh.ops.delete(bm, geom=list(set(apagar)), context='FACES')
            bm.to_mesh(o.data)
        bm.free()
    print('Rua da pista de treino: %d moitas da arena tiradas de cima dela' % tiradas)
elif not COM_RUA_DO_TREINO:
    print('Rua da pista de treino: desligada (COM_RUA_DO_TREINO)')
else:
    print('Rua da pista de treino: faltou a rua da cidade ou a pista; sem rua')
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
# 7e. Os prédios em que o carro entra: a Casa Della Tires (a loja do Luigi), os quartos dos cones e a recepção
# do Cozy Cone. O lado de dentro de cada um é um modelo à parte (recursos/modelos/<interior>.glb, de
# ferramentas/montar_loja_do_luigi.py, montar_quartos_do_cozy_cone.py e montar_recepcao_do_cozy_cone.py), que o
# jogo desenha no lugar do prédio (src/interiores.c); no Race-O-Rama os prédios são cascas fechadas, com as
# portas e os vidros só pintados. Saem da casca as aberturas do <interior>.json: as portas, do desenho e da
# colisão (o carro entra dirigindo, pela porta de verdade), e os vidros, só do desenho (de dentro se vê a rua,
# e de fora, o lado de dentro; a colisão deles fica, além da que o interior já tem). Uma abertura é uma peça
# inteira da casca (as faces ligadas por vértices) dentro de uma caixa, ou um recorte na superfície da casca
# (um vão de 4 cantos atravessado até uma profundidade). A colisão do lado de dentro (o piso, as paredes e os
# móveis, <interior>_colisao.glb) entra no .col (8a)
# ------------------------------------------------------------------
INTERIORES = ('loja_do_luigi', 'cozy_cone_quartos', 'cozy_cone_recepcao')
MODELOS = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'recursos', 'modelos')
FOLGA_DA_ABERTURA = 0.05   # metros em volta da caixa de cada abertura
FORA_DO_RECORTE = 0.3      # metros: o recorte começa assim fora da face de fora da casca (ela é curva num cone)


def no_blender(p):
    """Um ponto nos eixos do jogo, nos do Blender (y = -z do jogo, z = y do jogo)"""
    return Vector((p[0], -p[2], p[1]))


def coordenadas(o):
    co = np.empty(len(o.data.vertices) * 3, dtype=np.float32)   # (o bound_box fica velho depois do transform)
    o.data.vertices.foreach_get('co', co)
    return co.reshape(-1, 3)


def tirar_pecas(a, mexidas):
    """Apaga, das malhas do Race-O-Rama, as peças inteiras dentro da caixa da abertura. Devolve quantas faces"""
    folga = Vector((FOLGA_DA_ABERTURA,) * 3)
    p0, p1 = no_blender(a['caixa_min']), no_blender(a['caixa_max'])
    mn = Vector(map(min, p0, p1)) - folga; mx = Vector(map(max, p0, p1)) + folga
    dentro = lambda p: all(mn[i] <= p[i] <= mx[i] for i in range(3))
    tiradas = 0
    for o in ror:
        if not o.data.vertices:
            continue
        co = coordenadas(o)
        if (co.max(0) < np.array(mn)).any() or (co.min(0) > np.array(mx)).any():
            continue
        bm = bmesh.new(); bm.from_mesh(o.data)
        vistas, apagar = set(), []
        for f in bm.faces:
            if f in vistas:
                continue
            peca, pilha = [], [f]; vistas.add(f)
            while pilha:
                g = pilha.pop(); peca.append(g)
                for v in g.verts:
                    for h in v.link_faces:
                        if h not in vistas:
                            vistas.add(h); pilha.append(h)
            if all(dentro(v.co) for g in peca for v in g.verts):
                apagar.extend(peca)
        if apagar:
            bmesh.ops.delete(bm, geom=apagar, context='FACES')
            bm.to_mesh(o.data)
            tiradas += len(apagar); mexidas.append(o)
        bm.free()
    return tiradas


def recortar(a, mexidas):
    """Abre um vão na superfície da casca: corta as faces pelos 4 lados do vão e apaga as de dentro dele, de
    FORA_DO_RECORTE fora da face de fora até a profundidade. Devolve quantas faces"""
    cantos = [no_blender(c) for c in a['recorte']['cantos']]
    profundidade = a['recorte']['profundidade']
    meio = sum(cantos, Vector()) / 4
    normal = (cantos[1] - cantos[0]).cross(cantos[3] - cantos[0]).normalized()
    lados = []   # os 4 planos dos lados do vão: (um ponto, a normal para dentro do vão)
    for i in range(4):
        aresta = cantos[(i + 1) % 4] - cantos[i]
        para_dentro = normal.cross(aresta).normalized()
        if para_dentro.dot(meio - cantos[i]) < 0:
            para_dentro = -para_dentro
        lados.append((cantos[i], para_dentro))
    # dos dois lados do plano dos cantos (para fora, a folga da curva; para dentro, a profundidade)
    def no_vao(p, folga=1e-4):
        if any((p - c).dot(n) < -folga for c, n in lados):
            return False
        return abs((p - meio).dot(normal)) <= profundidade + FORA_DO_RECORTE
    raio = max((c - meio).length for c in cantos) + profundidade + FORA_DO_RECORTE
    tiradas = 0
    for o in ror:
        if not o.data.vertices:
            continue
        co = coordenadas(o)
        if (co.max(0) < np.array(meio) - raio).any() or (co.min(0) > np.array(meio) + raio).any():
            continue
        bm = bmesh.new(); bm.from_mesh(o.data)
        perto = lambda f: any((v.co - meio).length < raio for v in f.verts)
        if not any(perto(f) for f in bm.faces):
            bm.free(); continue
        for c, n in lados:
            faces = [f for f in bm.faces if perto(f)]
            geom = faces + list({e for f in faces for e in f.edges}) + list({v for f in faces for v in f.verts})
            bmesh.ops.bisect_plane(bm, geom=geom, plane_co=c, plane_no=n)
        apagar = [f for f in bm.faces if perto(f) and no_vao(f.calc_center_median())]
        if apagar:
            bmesh.ops.delete(bm, geom=apagar, context='FACES')
            bm.to_mesh(o.data)
            tiradas += len(apagar); mexidas.append(o)
        bm.free()
    return tiradas


def abrir(aberturas):
    """Tira da casca cada abertura. Devolve as malhas de onde tirou"""
    mexidas = []
    for a in aberturas:
        if 'recorte' in a:
            tiradas = recortar(a, mexidas)
            print('Interiores: %s, recorte, %d faces tiradas' % (a['nome'], tiradas))
        else:
            tiradas = tirar_pecas(a, mexidas)
            print('Interiores: %s, %d faces tiradas (%d esperadas)' % (a['nome'], tiradas, a['faces']))
    return mexidas


def sai_da_colisao(a):
    return a.get('sai_da_colisao', a['nome'] == 'painel_da_porta')   # (o json da loja, de antes, não diz)


vidros_dos_interiores, colisoes_dos_interiores = [], []
for interior in INTERIORES:
    arquivo = os.path.join(MODELOS, interior)
    if not (os.path.exists(arquivo + '.json') and os.path.exists(arquivo + '_colisao.glb')):
        print('Interiores: sem %s.json/_colisao.glb, o prédio fica fechado' % arquivo)
        continue
    with open(arquivo + '.json') as f:
        aberturas = json.load(f)['aberturas_para_tirar_da_casca']
    casca = abrir([a for a in aberturas if sai_da_colisao(a)])
    vidros_dos_interiores += [a for a in aberturas if not sai_da_colisao(a)]
    antes = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=arquivo + '_colisao.glb')
    importados = [o for o in bpy.data.objects if o not in antes]
    colisao = [o for o in importados if o.type == 'MESH']
    aplicar_transformacao(colisao, Matrix.Identity(4))
    for o in importados:
        if o not in colisao:
            bpy.data.objects.remove(o)
    # o piso de dentro tem o tipo de chão e a cor do chão da casca do prédio
    for o in colisao:
        o.data.materials.clear()
        if casca and casca[0].data.materials:
            o.data.materials.append(casca[0].data.materials[0])
    colisoes_dos_interiores += colisao
    print('Interiores: %s, colisão do lado de dentro, %d faces (a casca é %s)' % (
        interior, sum(len(o.data.polygons) for o in colisao), casca[0].name if casca else '?'))

# ------------------------------------------------------------------
# 7f. As calçadas no nível do asfalto. Ampliadas 4,5 vezes, as do Race-O-Rama ficaram com uma guia de
# 20 a 40 cm (um degrau quase em pé e uma faixa de 60 cm em cima dele) e ainda subiam até meio metro
# acima da rua: grossas e altas. Agora a beira da calçada fica rente ao asfalto, e ela sobe devagar até a
# altura de antes só longe da rua, perto dos prédios.
#   1. Cada vértice da calçada (menos os da beira, que ela divide com a rua), até CALCADA_ATE acima da
#      rua do lado, desce pela mesma regra, só da distância até a rua: rente até RENTE_ATE, como era depois
#      de RAMPA_DA_CALCADA, e no meio uma subida suave. A malha da calçada tem poucos vértices (os triângulos
#      são compridos, ao longo da rua): uma regra só da distância deixa iguais as faixas paralelas à rua,
#      sem ondular.
#      Uma parede que divide vértices com a calçada estica para baixo junto. O pé da guia (o bloco da
#      calçada desce abaixo do asfalto) sobe até a rua, para não abrir uma fresta entre as duas.
#   0. Uma laje apoiada na calçada (o lote da loja do Luigi, com a porta de garagem) ganha um corte a
#      RAMPA_DA_CALCADA da rua: só a parte da frente desce com a calçada, em rampa, e o resto fica onde estava
#      (a porta continua no nível do piso de dentro, que é outro modelo).
#   2. O que estava apoiado na calçada, à parte (o pé de uma parede, de uma cerca, de um poste), desce o
#      tanto que o chão desceu embaixo dele: estica para baixo e nada fica flutuando.
# ------------------------------------------------------------------
CALCADA_ROR = 'ror_Image_17'      # o concreto cinza das calçadas
CIMENTO_ROR = 'ror_Image_16'      # o cimento bege: no nível da rua é pátio e entrada; acima, calçada (a do Luigi, a do Ramone)
ASFALTO_DA_RUA = {'ror_Image_61', 'ror_Image_30'}   # a rua principal e a rodovia
NIVEL_DA_RUA_ATE = 1.56    # metros: a cidade é plana (as ruas a 1,42-1,50 m); o cimento até aqui é rua
CALCADA_ATE = 0.8          # metros acima da rua: mais alto que isso não é calçada (varanda, telhado)
RENTE_ATE = 4.0            # metros da rua: até aqui, a calçada fica rente ao asfalto...
RAMPA_DA_CALCADA = 8.0     # ...e daqui em diante, como era (no meio, sobe devagar)
APOIADO_ATE = 0.15         # metros: um vértice tão perto do chão da calçada (de antes) está apoiado nela
PE_DA_GUIA = 1.0           # metros da rua: o pé da guia (o bloco da calçada desce abaixo do asfalto) que fica tão
FUNDO_DA_GUIA = 0.8        # perto e até tanto abaixo dela sobe até o asfalto: senão, baixado o topo, abria uma fresta
PISO_ACIMA = 0.03          # metros: uma laje (o piso de um lote) apoiada na calçada fica pelo menos isso acima dela
ACIMA_DO_ASFALTO = 0.01    # metros: a calçada rente fica um tantinho acima, sem brigar com a rua


def faces_por_imagem(o):
    """Para cada face, o nome da imagem do material dela"""
    nomes = [(imagem_do_material(m).name if imagem_do_material(m) else '') for m in o.data.materials] or ['']
    return [nomes[p.material_index] if p.material_index < len(nomes) else '' for p in o.data.polygons]


def classes_das_faces(o):
    """Para cada face: 'rua', 'calcada' ou None (o resto). Medido antes de mexer: a calçada baixada continua calçada"""
    classes = []
    for f, im in zip(o.data.polygons, faces_por_imagem(o)):
        if im in ASFALTO_DA_RUA:
            classes.append('rua')
        elif im == CIMENTO_ROR:
            alto = max(o.data.vertices[i].co.z for i in f.vertices)
            classes.append('rua' if alto <= NIVEL_DA_RUA_ATE else 'calcada')
        elif im == CALCADA_ROR:
            classes.append('calcada')
        else:
            classes.append(None)
    return classes


def arvore_das_faces(objs, classes, qual, na_cidade, achatar=False):
    """Uma árvore de raios com as faces da classe pedida, na cidade"""
    bm = bmesh.new()
    for o in objs:
        for f, c in zip(o.data.polygons, classes[o.name]):
            vs = [o.data.vertices[i].co for i in f.vertices]
            if c == qual and any(na_cidade(v) for v in vs):
                novos = [bm.verts.new((v.x, v.y, 0.0) if achatar else v.copy()) for v in vs]
                try:
                    bm.faces.new(novos)
                except ValueError:
                    pass
    arv = BVHTree.FromBMesh(bm)
    bm.free()
    return arv


def nivelar_calcadas():
    na_cidade = lambda p: 200.0 < p.x < 1400.0 and -260.0 < p.y < -20.0
    objs = [o for o in ror if o.type == 'MESH' and o.name in bpy.data.objects]
    classes = {o.name: classes_das_faces(o) for o in objs}
    arv_deitada = arvore_das_faces(objs, classes, 'rua', na_cidade, achatar=True)
    arv_da_rua = arvore_das_faces(objs, classes, 'rua', na_cidade)
    calcada_antes = arvore_das_faces(objs, classes, 'calcada', na_cidade)

    def rua_perto(v):
        """A distância de lado até a rua mais perto e a altura dela ali (None se não há rua perto)"""
        perto = arv_deitada.find_nearest(Vector((v.x, v.y, 0.0)), RAMPA_DA_CALCADA)
        if perto[0] is None:
            return None
        # a altura da rua ali: o ponto da rua mais perto (um raio em pé escapava pelas emendas, na beira dela)
        rua = arv_da_rua.find_nearest(Vector((perto[0].x, perto[0].y, v.z)), 3.0)
        return None if rua[0] is None else (perto[3], rua[0].z + ACIMA_DO_ASFALTO)

    def sobre_a_calcada(v):
        hit = calcada_antes.ray_cast(Vector((v.x, v.y, v.z + 0.01)), Vector((0, 0, -1)), APOIADO_ATE + 0.02)
        return hit[0] is not None and PISO_ACIMA <= v.z - hit[0].z <= APOIADO_ATE

    # 0. As lajes apoiadas na calçada: cortadas a RAMPA_DA_CALCADA da rua
    import bmesh
    cortes = 0
    for o in objs:
        vs = o.data.vertices
        planos = {}
        for f, c in zip(o.data.polygons, classes[o.name]):
            if c is not None or f.normal.z < 0.9 or not all(na_cidade(vs[k].co) for k in f.vertices):
                continue
            pts = [vs[k].co for k in f.vertices]
            if max(p.z for p in pts) - min(p.z for p in pts) > 0.05 or not any(sobre_a_calcada(p) for p in pts):
                continue
            ds = [(arv_deitada.find_nearest(Vector((p.x, p.y, 0.0)), 80.0), p) for p in pts]
            ds = [(hit, p) for hit, p in ds if hit[0] is not None]
            if not ds or min(h[3] for h, _ in ds) >= RAMPA_DA_CALCADA or max(h[3] for h, _ in ds) <= RAMPA_DA_CALCADA:
                continue
            hit, p = min(ds, key=lambda e: e[0][3])
            n = Vector((p.x - hit[0].x, p.y - hit[0].y, 0.0))
            if n.length < 1e-3:
                continue
            n.normalize()
            co = Vector((hit[0].x, hit[0].y, 0.0)) + n * RAMPA_DA_CALCADA
            chave = (round(co.x, 1), round(co.y, 1), round(n.x, 2), round(n.y, 2))
            planos.setdefault(chave, (co, n, []))[2].append(f.index)
        if not planos:
            continue
        bm = bmesh.new()
        bm.from_mesh(o.data)
        marca = bm.faces.layers.int.new('laje')
        bm.faces.ensure_lookup_table()
        for k, (_, _, faces) in enumerate(planos.values()):
            for i in faces:
                bm.faces[i][marca] = k + 1
        for k, (co, n, _) in enumerate(planos.values()):
            faces = [f for f in bm.faces if f[marca] == k + 1]
            geom = faces + list({e for f in faces for e in f.edges}) + list({v for f in faces for v in f.verts})
            bmesh.ops.bisect_plane(bm, geom=geom, plane_co=co, plane_no=n)
            cortes += 1
        bm.faces.layers.int.remove(marca)
        bm.to_mesh(o.data)
        bm.free()
        o.data.update()
        classes[o.name] = classes_das_faces(o)
    lajes = {}
    for o in objs:
        vs = o.data.vertices
        lajes[o.name] = {k for f, c in zip(o.data.polygons, classes[o.name]) if c is None and f.normal.z >= 0.9
                         for k in f.vertices if na_cidade(vs[k].co) and sobre_a_calcada(vs[k].co)}

    # 1. A calçada
    da_calcada, da_rua = {}, {}
    mexidos = 0
    for o in objs:
        de_calcada, de_rua = set(), set()
        for f, c in zip(o.data.polygons, classes[o.name]):
            if c == 'calcada':
                de_calcada.update(f.vertices)
            elif c == 'rua':
                de_rua.update(f.vertices)
        da_calcada[o.name], da_rua[o.name] = de_calcada - de_rua, de_rua
        for i in da_calcada[o.name]:
            v = o.data.vertices[i].co
            achou = rua_perto(v) if na_cidade(v) else None
            if achou is None:
                continue
            d, h = achou
            if h - FUNDO_DA_GUIA < v.z < h and d < PE_DA_GUIA:
                v.z = h   # o pé da guia, abaixo da rua: sobe até ela e fecha a fresta entre a rua e a calçada
                mexidos += 1
                continue
            if v.z <= h or v.z - h > CALCADA_ATE:
                continue
            v.z = h + (v.z - h) * degrau_suave(RENTE_ATE, RAMPA_DA_CALCADA, d)
            mexidos += 1
        o.data.update()
    calcada_depois = arvore_das_faces(objs, classes, 'calcada', na_cidade)

    # 2. O que estava apoiado nela desce junto (as paredes, as cercas, os postes, a base da torre de pneus)
    apoiados = 0
    for o in objs:
        for i, vert in enumerate(o.data.vertices):
            v = vert.co
            if i in da_calcada[o.name] or i in da_rua[o.name] or not na_cidade(v):
                continue
            antes = calcada_antes.ray_cast(v + Vector((0, 0, 0.3)), Vector((0, 0, -1)), 0.6)
            if antes[0] is None or abs(antes[0].z - v.z) > APOIADO_ATE:
                continue
            depois = calcada_depois.ray_cast(Vector((v.x, v.y, antes[0].z + 0.3)), Vector((0, 0, -1)), 2.0)
            if depois[0] is None or depois[0].z >= antes[0].z - 0.005:
                continue
            desce = depois[0].z - antes[0].z
            if i in lajes[o.name]:   # a laje: a frente desce, e para de descer no corte
                achou = rua_perto(v)
                desce *= 0.0 if achou is None else 1.0 - degrau_suave(RENTE_ATE, RAMPA_DA_CALCADA, achou[0])
            v.z += desce
            apoiados += 1
        o.data.update()
    print('Calçadas: %d vértices baixados até o nível do asfalto; %d do que estava apoiado nelas desceram junto; '
          '%d cortes em lajes' % (mexidos, apoiados, cortes))


nivelar_calcadas()

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


solidos = [o for o in ror + c2 if tem_colisao(o)] + [base, emenda] + ([ligacao] if ligacao else []) + ([saias] if saias else []) \
    + colisoes_dos_interiores
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

# A colisão dos interiores não vai para o desenho (o lado de dentro deles é outro modelo), e os vidros saem do desenho
for o in colisoes_dos_interiores:
    bpy.data.objects.remove(o)
abrir(vidros_dos_interiores)

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


# As texturas em JPEG viram PNG (a raylib do jogo não lê JPEG: a pedra da mesa da arena saía cinza, e as
# placas da cidade, o outdoor "Greetings from" e o "Leaving Radiator Springs", brancos). Nenhuma passa de
# MAIOR_TEXTURA pixels de lado
def sem_jpeg():
    trocadas = 0
    for imagem in list(bpy.data.images):
        if imagem.size[0] == 0:
            continue
        empacotada = imagem.packed_file is not None
        jpeg = imagem.file_format == 'JPEG' or (empacotada and imagem.packed_file.data[:3] == b'\xff\xd8\xff')
        if not jpeg:
            continue
        largura, altura = imagem.size
        p = np.empty(largura * altura * 4, dtype=np.float32)
        imagem.pixels.foreach_get(p)
        nova = bpy.data.images.new(imagem.name + '_png', largura, altura, alpha=False)
        nova.pixels.foreach_set(p)
        if max(largura, altura) > MAIOR_TEXTURA:
            k = MAIOR_TEXTURA / max(largura, altura)
            nova.scale(max(1, round(largura * k)), max(1, round(altura * k)))
        nova.file_format = 'PNG'
        nova.pack()
        for m in bpy.data.materials:
            if m.use_nodes:
                for no in m.node_tree.nodes:
                    if no.type == 'TEX_IMAGE' and no.image == imagem:
                        no.image = nova
        nome = imagem.name
        bpy.data.images.remove(imagem)
        nova.name = nome
        trocadas += 1
    print('Texturas JPEG passadas para PNG: %d' % trocadas)


sem_jpeg()
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
