"""
preparar_figurantes.py
------------------------------------------------------------------
Prepara os modelos dos moradores de Radiator Springs que não correm
(os "figurantes" de src/moradores.h) para o jogo:

    blender -b --factory-startup -P ferramentas/preparar_figurantes.py -- \\
        ~/Downloads recursos/modelos/carros

Lê flo.glb, cars_red.glb, sherrif.glb e lizzie.glb da pasta de origem e
grava flo.glb, ruivo.glb, xerife.glb e lizzie.glb na de destino. O jogo
(carros3d.c) mede cada um, acha as rodas e deixa no tamanho da ficha.
Grava também os olhos de cada um no jeito dos carros da corrida: as três
camadas em recursos/modelos/carros/olhos/ (<nome>_fundo.png, _iris.png e
_mascara.png) e a linha dele em olhos.txt. No jogo, as íris andam para o
lado da curva, as pálpebras piscam e eles se assustam.

O que muda em cada um:
  - Todos: as texturas em JPEG viram PNG (a raylib do jogo não lê JPEG:
    o corpo da Flo e do Xerife e os olhos da Lizzie ficavam brancos), e
    nenhuma passa de 1024 pixels (a do Xerife tinha 2048).
  - Flo, Xerife e Lizzie: os olhos vêm pintados numa textura só (os dois
    olhos, a pálpebra e os cílios). Cada íris é achada com raios saindo
    do meio dela até o branco (uma elipse ajustada nas bordas); a camada
    das íris é a textura dentro das elipses (o pedaço escondido pelos
    cílios é refeito espelhando a íris), o fundo é a textura sem elas
    (cada coluna refeita a partir das colunas dos lados, acompanhando a
    curva da pálpebra) e a máscara é o branco do olho.
  - Lizzie: o modelo vem do avesso, com as faces e as normais para dentro.
    O jogo não desenha o lado de trás das faces: ela aparecia oca, com o
    banco e o lado de baixo à mostra e sem os olhos de frente. Todas as
    peças viram para fora.
  - Xerife: tira uns triângulos soltos e finos que saem para os lados
    perto das rodas (alargavam a caixa do carro, e o jogo tira a largura
    dela).
  - Ruivo (cars_red.glb, que vem de um jogo):
      * tira o esqueleto e fica na pose de repouso (o shader dos carros
        não faz skinning) e a esfera que o importador cria;
      * o modelo vem espelhado (os letreiros saem ao contrário): espelha
        em x e vira as faces;
      * os olhos têm o defeito do Mack: a textura é só uma íris pequena e
        estreita, sobre branco, e as coordenadas vão de -1,4 a 2,4 (o jogo
        de lá prendia a textura na borda; aqui ela se repetia, e cada olho
        virava uma grade de íris). Os olhos são redesenhados como os do
        Ruivo do filme (o branco grande, a íris redonda cor de âmbar, a
        pupila grande com brilho e a pálpebra vermelha no alto), os dois
        num material só, cada um numa metade da textura, com coordenadas
        tiradas da posição de cada ponto no olho;
      * tira a segunda camada de coordenadas (com valores absurdos).
------------------------------------------------------------------
"""
import json
import math
import os
import struct
import sys
import bmesh
import bpy
import mathutils
import numpy as np
from mathutils.bvhtree import BVHTree

argv = sys.argv[sys.argv.index("--") + 1:]
origem, destino = argv[0], argv[1]
PASTA_DOS_OLHOS = os.path.join(destino, "olhos")
MAIOR_TEXTURA = 1024
LADO_DO_OLHO_DO_RUIVO = 256          # cada olho do Ruivo, na textura nova (os dois lado a lado)
# Os olhos do Ruivo, desenhados como os do filme. Em frações do olho: x de 0 (lado do nariz) a 1 (de fora),
# y de 0 (em cima) a 1 (embaixo)
RUIVO_PALPEBRA = 0.13                # a lataria cobre o alto do olho até aqui (a pintada é a que pisca)
RUIVO_CILIOS = 0.03                  # o traço escuro embaixo dela
RUIVO_IRIS_X, RUIVO_IRIS_Y = 0.46, 0.56
RUIVO_RAIO = 0.29                    # o raio da íris, em alturas do olho
RUIVO_PUPILA = 0.50                  # da íris
RUIVO_IRIS_FORA = (0.36, 0.17, 0.06)
RUIVO_IRIS_MEIO = (0.66, 0.37, 0.15)
RUIVO_IRIS_DENTRO = (0.82, 0.55, 0.27)
ANDA_POR_RAIO = 1.2                  # quanto a íris anda de lado, em raios dela (como nos carros da corrida)

# Onde fica, mais ou menos, o meio de cada íris (u, v da textura dos olhos, v de cima para baixo):
# o ponto de partida dos raios. E o lado da pálpebra que pisca (+1 embaixo na textura: nos três, a
# textura está de cabeça para baixo no para-brisa, e o alto do carro é o pé dela)
OLHOS_PINTADOS = {
    "flo":    ("Material__44", ((0.527, 0.273), (0.805, 0.320)), 1, True),
    "xerife": ("Material.012", ((0.387, 0.254), (0.777, 0.254)), 1, True),
    "lizzie": ("Material.016", ((0.488, 0.244), (0.814, 0.244)), 1, False),
}


def importar(arquivo):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=os.path.join(origem, arquivo))
    return bpy.context.scene


def malhas():
    return [o for o in bpy.context.scene.objects if o.type == 'MESH']


def aplicar_transformacoes():
    """Cada malha com a transformação dela dentro dos vértices, sem pai"""
    for o in malhas():
        o.data = o.data.copy()
        o.data.transform(o.matrix_world)
        o.parent = None
        o.matrix_world = mathutils.Matrix.Identity(4)
    for o in [o for o in bpy.context.scene.objects if o.type != 'MESH']:
        bpy.data.objects.remove(o, do_unlink=True)


def no_de_imagem(material):
    if material is None or not material.use_nodes:
        return None
    return next((n for n in material.node_tree.nodes if n.type == 'TEX_IMAGE' and n.image), None)


def virar_para_fora(o):
    """Vira a frente das faces e as normais da malha (um modelo que veio do avesso)"""
    me = o.data
    cantos = me.corner_normals
    guardadas = {}
    for p in me.polygons:
        for canto in p.loop_indices:
            guardadas[(p.index, me.loops[canto].vertex_index)] = -cantos[canto].vector
    me.flip_normals()   # só a ordem dos cantos: as normais são postas logo abaixo
    novas = [None] * len(me.loops)
    for p in me.polygons:
        for canto in p.loop_indices:
            novas[canto] = guardadas[(p.index, me.loops[canto].vertex_index)]
    me.normals_split_custom_set(novas)


def esta_do_avesso(o):
    """Um raio sai do meio de cada face da malha para a frente dela e outro para trás. Do lado
    certo, muitos saem do carro pela frente; do avesso, os da frente batem no outro lado do carro
    e quem sai são os de trás. Devolve 1 se a malha está do avesso"""
    pontos, faces = [], []
    for m in malhas():
        base = len(pontos)
        pontos += [m.matrix_world @ v.co for v in m.data.vertices]
        faces += [[base + i for i in p.vertices] for p in m.data.polygons]
    arvore = BVHTree.FromPolygons(pontos, faces)
    frente = tras = 0
    for p in o.data.polygons:
        centro, normal = o.matrix_world @ p.center, (o.matrix_world.to_3x3() @ p.normal).normalized()
        folga = 1e-4 * (1.0 + centro.length)
        frente += arvore.ray_cast(centro + normal * folga, normal)[0] is None
        tras += arvore.ray_cast(centro - normal * folga, -normal)[0] is None
    return tras > frente + 0.2 * len(o.data.polygons)


def trocar_imagem(velha, nova):
    for m in bpy.data.materials:
        if m.use_nodes:
            for n in m.node_tree.nodes:
                if n.type == 'TEX_IMAGE' and n.image == velha:
                    n.image = nova


# ---- Imagens: de e para numpy, com a linha 0 em cima (como as coordenadas do glTF) ----
def ler(imagem):
    largura, altura = imagem.size
    p = np.empty(largura * altura * 4, dtype=np.float32)
    imagem.pixels.foreach_get(p)
    return np.flipud(p.reshape(altura, largura, 4)).copy()


def nova_imagem(nome, pixels, alfa=False):
    altura, largura = pixels.shape[:2]
    imagem = bpy.data.images.new(nome, largura, altura, alpha=alfa)
    p = np.ones((altura, largura, 4), dtype=np.float32)
    p[..., :pixels.shape[2]] = np.clip(pixels, 0.0, 1.0)
    if not alfa:
        p[..., 3] = 1.0
    imagem.pixels.foreach_set(np.flipud(p).ravel())
    imagem.file_format = 'PNG'
    return imagem


def gravar(nome, pixels, alfa=False):
    imagem = nova_imagem(nome, pixels, alfa)
    imagem.filepath_raw = os.path.join(PASTA_DOS_OLHOS, nome + ".png")
    imagem.save()
    bpy.data.images.remove(imagem)


def sem_jpeg():
    """As texturas em JPEG (e as maiores que MAIOR_TEXTURA) viram PNG empacotado no modelo"""
    for imagem in list(bpy.data.images):
        if imagem.packed_file is None or imagem.size[0] == 0:
            continue
        jpeg = imagem.packed_file.data[:3] == b'\xff\xd8\xff' or imagem.file_format == 'JPEG'
        if not jpeg and max(imagem.size) <= MAIOR_TEXTURA:
            continue
        largura, altura = imagem.size
        p = np.empty(largura * altura * 4, dtype=np.float32)
        imagem.pixels.foreach_get(p)
        nova = bpy.data.images.new(imagem.name + "_png", largura, altura, alpha=not jpeg)
        nova.pixels.foreach_set(p)
        if max(largura, altura) > MAIOR_TEXTURA:
            k = MAIOR_TEXTURA / max(largura, altura)
            nova.scale(max(1, round(largura * k)), max(1, round(altura * k)))
        nova.file_format = 'PNG'
        nova.pack()
        trocar_imagem(imagem, nova)
        print(f"FIGURANTE: {imagem.name} ({'JPEG' if jpeg else 'PNG'} {largura}x{altura}) -> PNG {nova.size[0]}x{nova.size[1]}")


def exportar(arquivo):
    bpy.ops.object.select_all(action='SELECT')
    caminho = os.path.join(destino, arquivo)
    bpy.ops.export_scene.gltf(filepath=caminho, export_format='GLB', export_yup=True,
                              use_selection=True, export_skins=False, export_animations=False)
    print("FIGURANTE: gravado", arquivo)
    return caminho


def materiais_do_glb(caminho):
    with open(caminho, "rb") as arquivo:
        arquivo.read(12)
        tamanho, _ = struct.unpack("<II", arquivo.read(8))
        gltf = json.loads(arquivo.read(tamanho))
    return [m.get("name", "") for m in gltf.get("materials", [])]


linhas_dos_olhos = {}


def linha_dos_olhos(nome, caminho_do_glb, material, elipses, largura, altura, lado):
    """A linha do carro em olhos.txt (veja carros3d.c): o material, quanto a íris anda, o lado da
    pálpebra, a altura do meio do olho, o raio da íris na altura e o meio de cada íris"""
    numero = materiais_do_glb(caminho_do_glb).index(material)
    (cx1, cy1, a1, b1), (cx2, cy2, a2, b2) = elipses
    anda = ANDA_POR_RAIO * (a1 + a2) / 2.0 / largura
    linhas_dos_olhos[nome] = (f"{nome} {numero} {anda:.4f} {lado} {(cy1 + cy2) / 2.0 / altura:.4f} "
                              f"{(b1 + b2) / 2.0 / altura:.4f} {cx1 / largura:.4f} {cy1 / altura:.4f} "
                              f"{cx2 / largura:.4f} {cy2 / altura:.4f}")
    print("FIGURANTE: olhos.txt ->", linhas_dos_olhos[nome])


# ---- Os olhos pintados: achar as íris e separar as camadas ----
def brilho_e_saturacao(rgb):
    maior, menor = rgb.max(axis=-1), rgb.min(axis=-1)
    return maior, menor, (maior - menor) / np.maximum(maior, 1e-3)


def achar_elipse(parar, semente, seguidos, longe, nome, raios=None):
    """Uma elipse de eixos retos (meio x, meio y, raio x, raio y, em pixels) em volta da semente:
    raios saem dela até o primeiro trecho de "parar"; os que não acham (os cílios, por cima) ficam
    de fora; a elipse é ajustada nas bordas, tirando as que destoam. Com "raios" (rx, ry), só o meio
    é ajustado: a íris que passa da borda da textura tem bordas de um lado só, e com elas sozinhas a
    elipse sai grande demais"""
    altura, largura = parar.shape
    sx, sy = semente
    bordas = []
    for k in range(144):
        ang = 2.0 * math.pi * k / 144
        dx, dy = math.cos(ang), math.sin(ang)
        corrida = 0
        for passo in range(1, longe):
            x, y = int(round(sx + dx * passo)), int(round(sy + dy * passo))
            if not (0 <= x < largura and 0 <= y < altura):
                break
            corrida = corrida + 1 if parar[y, x] else 0
            if corrida == seguidos:
                bordas.append((sx + dx * (passo - seguidos + 1), sy + dy * (passo - seguidos + 1)))
                break
    pontos = np.array(bordas, dtype=np.float64)
    if raios is not None:
        rx, ry = raios
        cx, cy = sx, sy
    for _ in range(4):
        x, y = pontos[:, 0], pontos[:, 1]
        if raios is None:
            A = np.stack([x * x, y * y, x, y], axis=1)
            a, b, c, d = np.linalg.lstsq(A, np.ones(len(x)), rcond=None)[0]
            cx, cy = -c / (2 * a), -d / (2 * b)
            k = 1 + c * c / (4 * a) + d * d / (4 * b)
            rx, ry = math.sqrt(k / a), math.sqrt(k / b)
        else:
            # o meio do círculo de raio 1 que melhor passa pelas bordas, na escala da elipse (Gauss-Newton)
            for _ in range(20):
                px, py = (x - cx) / rx, (y - cy) / ry
                d = np.maximum(np.hypot(px, py), 1e-6)
                J = np.stack([-px / d / rx, -py / d / ry], axis=1)
                passo = np.linalg.lstsq(J, -(d - 1.0), rcond=None)[0]
                cx, cy = cx + passo[0], cy + passo[1]
        erro = np.abs(np.hypot((x - cx) / rx, (y - cy) / ry) - 1.0)
        bons = erro < max(0.06, np.percentile(erro, 75))
        if bons.all() or bons.sum() < 12:
            break
        pontos = pontos[bons]
    print(f"FIGURANTE:   {nome} em ({cx:.1f}, {cy:.1f}), raios {rx:.1f} x {ry:.1f} ({len(pontos)} de {len(bordas)} bordas)")
    return cx, cy, rx, ry


def achar_iris(rgb, semente, raios=None):
    """A íris: da semente (fração da textura) até o branco do olho (o brilho dela, branco também,
    é pequeno: só um trecho maior conta)"""
    altura, largura = rgb.shape[:2]
    _, menor, saturacao = brilho_e_saturacao(rgb)
    branco = (menor > 0.80) & (saturacao < 0.12)
    return achar_elipse(branco, (semente[0] * largura, semente[1] * altura), max(3, largura // 100),
                        int(0.22 * largura), "íris", raios)


def achar_pupila(rgb, iris):
    """A pupila: do meio da íris até o que não é preto"""
    brilho, _, _ = brilho_e_saturacao(rgb)
    return achar_elipse(brilho > 0.22, iris[:2], 3, int(min(iris[2], iris[3])), "pupila")


def dilatar(m, raio):
    saida = m.copy()
    altura, largura = m.shape
    grande = np.pad(m, raio)
    for dy in range(-raio, raio + 1):
        for dx in range(-raio, raio + 1):
            if dx * dx + dy * dy <= raio * raio:
                saida |= grande[raio + dy:raio + dy + altura, raio + dx:raio + dx + largura]
    return saida


def suavizar(m, vezes=2):
    m = m.astype(np.float32)
    for _ in range(vezes):
        g = np.pad(m, 1, mode='edge')
        m = (g[:-2, :-2] + g[:-2, 1:-1] + g[:-2, 2:] + g[1:-1, :-2] + g[1:-1, 1:-1] + g[1:-1, 2:]
             + g[2:, :-2] + g[2:, 1:-1] + g[2:, 2:]) / 9.0
    return m


def maiores_trechos(m):
    """Em cada coluna, o maior trecho de m: onde começa e onde termina (linhas)"""
    altura, largura = m.shape
    inicio = np.zeros(largura, dtype=int)
    fim = np.zeros(largura, dtype=int)
    for x in range(largura):
        coluna = np.concatenate([[False], m[:, x], [False]]).astype(np.int8)
        trocas = np.flatnonzero(np.diff(coluna))
        if len(trocas) >= 2:
            de, ate = trocas[0::2], trocas[1::2]
            k = np.argmax(ate - de)
            inicio[x], fim[x] = de[k], ate[k]
    return inicio, fim


def separar_olhos(nome, imagem, sementes, iris_de_cor, mesmo_tamanho=False, cor_da_iris=0.3, sem_a_palpebra=False,
                  copiar_iris=None):
    """As três camadas a partir da textura pintada. Devolve as elipses das íris.
    iris_de_cor: as íris são coloridas (a Flo e o Xerife); senão (a Lizzie), cinza, dentro do branco.
    Para os carros da corrida (preparar_carros.py): mesmo_tamanho (as duas íris ficam com os raios da
    menor, e cada uma só acha o meio: uma íris cortada pela borda da textura saía grande demais),
    cor_da_iris (a saturação a partir da qual um pixel é da íris), sem_a_palpebra (o que tem a cor da
    pálpebra não é íris, mesmo dentro da elipse) e copiar_iris (o número da íris inteira: as duas são o
    mesmo desenho, e a outra, cortada pela borda ou pela pálpebra, vira cópia dela)"""
    rgb = ler(imagem)[..., :3]
    altura, largura = rgb.shape[:2]
    brilho, menor, saturacao = brilho_e_saturacao(rgb)
    elipses = [achar_iris(rgb, s) for s in sementes]
    if mesmo_tamanho:
        raios = (min(e[2] for e in elipses), min(e[3] for e in elipses))
        elipses = [achar_iris(rgb, s, raios) for s in sementes]
    pupilas = [achar_pupila(rgb, e) for e in elipses]
    Y, X = np.mgrid[0:altura, 0:largura].astype(np.float64) + 0.5
    olho = (brilho > 0.5) & (saturacao < 0.16)    # o branco e a sombra dele (onde a íris pode aparecer)

    def raio(e, folga=1.0, mais=0.0):
        cx, cy, rx, ry = e
        return np.hypot((X - cx) / (rx * folga + mais), (Y - cy) / (ry * folga + mais))

    # As bordas do branco em cada coluna (o maior trecho): de cima (T) e de baixo (E). Nas colunas das
    # íris, uma curva (Hermite) das colunas logo antes até as logo depois, com a inclinação que a
    # borda tem de cada lado: a linha dos cílios desce e sobe por baixo da íris como nos lados
    T, E = (v.astype(np.float64) for v in maiores_trechos(olho))
    faixas = []
    for cx, cy, rx, ry in elipses:
        xl = int(max(16, math.floor(cx - rx * 1.08 - 4)))
        xr = int(min(largura - 17, math.ceil(cx + rx * 1.08 + 4)))
        faixas.append((xl, xr))
        x = np.arange(xl + 1, xr)
        t = (x - xl) / (xr - xl)
        h00, h10, h01, h11 = 2 * t**3 - 3 * t**2 + 1, t**3 - 2 * t**2 + t, -2 * t**3 + 3 * t**2, t**3 - t**2
        for borda, mais_para_fora in ((T, np.minimum), (E, np.maximum)):
            antes = np.polyfit(np.arange(xl - 15, xl + 1), borda[xl - 15:xl + 1], 1)[0]
            depois = np.polyfit(np.arange(xr, xr + 16), borda[xr:xr + 16], 1)[0]
            antes, depois = np.clip([antes, depois], -2.5, 2.5) * (xr - xl)
            curva = h00 * borda[xl] + h10 * antes + h01 * borda[xr] + h11 * depois
            reta = borda[xl] + (borda[xr] - borda[xl]) * t
            # a curva só vale onde ela abre mais o branco que a reta (a cova embaixo da íris da Flo);
            # onde ela passaria do ponto para dentro do branco, fica a reta
            borda[xl + 1:xr] = mais_para_fora(curva, reta)

    # O que é íris de verdade, dentro de cada elipse: a pupila (preta como os cílios, mas tanto faz) e
    # a cor da íris (ou, na íris cinza, o que está dentro do branco)
    longe_da_palpebra = np.ones((altura, largura), bool)
    if iris_de_cor:
        de_iris = (saturacao > cor_da_iris) & (brilho > 0.1)
        if sem_a_palpebra:
            # a cor da pálpebra: a do que não é branco nem íris (a maior parte é a lataria)
            fora = ~olho & np.all([raio(e) > 1.15 for e in elipses], axis=0) & (brilho > 0.1)
            palpebra = np.median(rgb[fora], axis=0)
            longe_da_palpebra = np.linalg.norm(rgb - palpebra, axis=-1) > 0.12
            de_iris &= longe_da_palpebra
            print(f"FIGURANTE:   a pálpebra tem a cor {np.round(palpebra, 2)}: não é íris")
    else:
        de_iris = (Y >= T[None, :]) & (Y < E[None, :]) & (brilho < 0.9)
    iris = np.zeros((altura, largura, 4), dtype=np.float32)
    trocar = np.zeros((altura, largura), bool)    # os pixels do fundo que eram íris
    for e, pu in zip(elipses, pupilas):
        cx, cy, rx, ry = e
        r = raio(e)
        boa = (r <= 1.0) & (de_iris | (raio(pu, 1.15) <= 1.0)) & longe_da_palpebra
        # o que os cílios (ou o branco, na borda) tampam vem do espelho, de cima para baixo; se o espelho
        # também não é íris, do mesmo raio na horizontal
        ys, xs = np.nonzero((r <= 1.04) & ~boa)
        my = np.clip(np.round(2 * cy - (ys + 0.5) - 0.5).astype(int), 0, altura - 1)
        espelho = boa[my, xs]
        cor = rgb.copy()
        cor[ys[espelho], xs[espelho]] = rgb[my[espelho], xs[espelho]]
        resto_y, resto_x = ys[~espelho], xs[~espelho]
        rr = r[resto_y, resto_x]
        hx = np.clip(np.round(cx + np.sign(resto_x + 0.5 - cx) * rr * rx * 0.98 - 0.5).astype(int), 0, largura - 1)
        cor[resto_y, resto_x] = rgb[np.full(len(hx), int(cy)), hx]
        alfa = np.clip(0.5 - (r - 1.0) * min(rx, ry) / 1.5, 0.0, 1.0)
        perto = alfa > 0.0
        iris[perto, :3] = cor[perto]
        iris[..., 3] = np.maximum(iris[..., 3], alfa)
        # no fundo, sai o que era íris e a borda dela (o contorno escuro, fora da elipse ajustada)
        trocar |= boa | ((raio(e, 1.0, 3.0) <= 1.0) & (Y < E[None, :]))
    if copiar_iris is not None:
        # as outras íris viram cópia da inteira, cada uma no lugar dela
        inteira = iris.copy()
        cx0, cy0 = elipses[copiar_iris][:2]
        for k, e in enumerate(elipses):
            if k != copiar_iris:
                ys, xs = np.nonzero(raio(e, 1.0, 3.0) <= 1.0)
                de_y = np.clip(np.round(ys + cy0 - e[1]).astype(int), 0, altura - 1)
                de_x = np.clip(np.round(xs + cx0 - e[0]).astype(int), 0, largura - 1)
                iris[ys, xs] = inteira[de_y, de_x]
    trocar = dilatar(trocar, 3)
    peso = suavizar(trocar)

    # O fundo: sem as íris. Cada ponto trocado vem das colunas dos lados da íris, no mesmo lugar
    # relativo ao branco (acompanhando a curva da pálpebra), misturadas pela distância
    fundo = rgb.copy()
    for (cx, cy, rx, ry), (xl, xr) in zip(elipses, faixas):
        ys, xs = np.nonzero((peso > 0.0) & (X > xl) & (X < xr + 1))
        y = ys + 0.5

        def mesmo_lugar(xref):
            t0, e0, t1, e1 = T[xs], E[xs], T[xref], E[xref]
            s = (y - t0) / np.maximum(e0 - t0, 1.0)
            v = np.where(y < t0, t1 - (t0 - y), np.where(y >= e0, e1 + (y - e0), t1 + s * (e1 - t1)))
            return rgb[np.clip(v.astype(int), 0, altura - 1), xref]

        t = ((xs - xl) / max(xr - xl, 1))[:, None]
        refeito = mesmo_lugar(np.full(len(xs), xl)) * (1 - t) + mesmo_lugar(np.full(len(xs), xr)) * t
        k = peso[ys, xs][:, None]
        fundo[ys, xs] = fundo[ys, xs] * (1 - k) + refeito * k

    # A máscara: o branco do fundo já sem as íris, o maior trecho de cada coluna, com as bordas
    # alisadas de coluna a coluna (uma mediana tira os fiapos da sombra; senão a pálpebra fecha em degraus)
    f_brilho, _, f_saturacao = brilho_e_saturacao(fundo)
    de, ate = maiores_trechos((f_brilho > 0.42) & (f_saturacao < 0.16))
    tem = (ate - de) >= 3

    def alisar(v):
        g = np.pad(v.astype(np.float64), 7, mode='edge')
        mediana = np.median(np.lib.stride_tricks.sliding_window_view(g, 15), axis=1)
        g = np.pad(mediana, 2, mode='edge')
        return np.convolve(g, np.ones(5) / 5.0, mode='valid')

    de, ate = alisar(de), alisar(ate)
    mascara = np.clip(np.minimum(Y - de[None, :], ate[None, :] - Y) + 0.5, 0.0, 1.0) * tem[None, :]
    mascara = mascara.astype(np.float32)

    gravar(nome + "_fundo", fundo)
    gravar(nome + "_iris", iris, alfa=True)
    gravar(nome + "_mascara", np.repeat(mascara[..., None], 3, axis=-1))
    return elipses, largura, altura


def olhos_pintados(nome, caminho_do_glb):
    material_dos_olhos, sementes, lado, iris_de_cor = OLHOS_PINTADOS[nome]
    imagem = no_de_imagem(bpy.data.materials[material_dos_olhos]).image
    return material_dos_olhos, separar_olhos(nome, imagem, sementes, iris_de_cor), lado


def gravar_tabela_dos_olhos():
    """A tabela dos olhos (olhos.txt): as linhas dos carros preparados agora, no lugar das antigas"""
    tabela = os.path.join(PASTA_DOS_OLHOS, "olhos.txt")
    linhas = open(tabela).read().splitlines() if os.path.exists(tabela) else []
    linhas = [l for l in linhas if l.split(" ")[0] not in linhas_dos_olhos] + list(linhas_dos_olhos.values())
    with open(tabela, "w") as arquivo:
        arquivo.write("\n".join(linhas) + "\n")


def preparar():
    os.makedirs(PASTA_DOS_OLHOS, exist_ok=True)

    # ---- Flo e Lizzie ----
    for de, para in (("flo.glb", "flo"), ("lizzie.glb", "lizzie")):
        importar(de)
        aplicar_transformacoes()
        if esta_do_avesso(max(malhas(), key=lambda o: len(o.data.polygons))):
            for o in malhas():
                virar_para_fora(o)
            print(f"FIGURANTE: {para} estava do avesso, virada para fora")
        sem_jpeg()
        material, (elipses, largura, altura), lado = olhos_pintados(para, None)
        caminho = exportar(para + ".glb")
        linha_dos_olhos(para, caminho, material, elipses, largura, altura, lado)

    # ---- Xerife: sem os fiapos dos lados ----
    importar("sherrif.glb")
    aplicar_transformacoes()
    tirados = 0
    for o in malhas():
        bm = bmesh.new()
        bm.from_mesh(o.data)
        fiapos = [f for f in bm.faces if abs(f.calc_center_median().x) > 1.17 and f.calc_center_median().z < 0.35]
        tirados += len(fiapos)
        bmesh.ops.delete(bm, geom=fiapos, context='FACES')
        bm.to_mesh(o.data)
        bm.free()
    print("FIGURANTE: xerife sem", tirados, "triângulos soltos")
    sem_jpeg()
    material, (elipses, largura, altura), lado = olhos_pintados("xerife", None)
    caminho = exportar("xerife.glb")
    linha_dos_olhos("xerife", caminho, material, elipses, largura, altura, lado)

    # ---- Ruivo ----
    importar("cars_red.glb")
    for o in malhas():
        for mod in list(o.modifiers):
            if mod.type == 'ARMATURE':
                o.modifiers.remove(mod)   # a pose de repouso: os vértices como estão
    for o in [o for o in malhas() if o.name.lower().startswith("icosphere")]:
        bpy.data.objects.remove(o, do_unlink=True)
    aplicar_transformacoes()
    for o in malhas():
        o.vertex_groups.clear()
        while len(o.data.uv_layers) > 1:
            o.data.uv_layers.remove(o.data.uv_layers[-1])
        # espelha em x e vira as faces (para a frente das faces continuar para fora)
        o.data.transform(mathutils.Matrix.Scale(-1.0, 4, (1.0, 0.0, 0.0)))
        bm = bmesh.new()
        bm.from_mesh(o.data)
        bmesh.ops.reverse_faces(bm, faces=bm.faces)
        bm.to_mesh(o.data)
        bm.free()
    sem_jpeg()

    # A cor da lataria, para a pálpebra: a mediana dos pixels bem vermelhos da textura do corpo
    corpo = ler(no_de_imagem(bpy.data.materials["red_body"]).image)[..., :3].reshape(-1, 3)
    cor_da_lataria = np.median(corpo[(corpo[:, 0] > 0.45) & (corpo[:, 1] < 0.2) & (corpo[:, 2] < 0.2)], axis=0)

    # Os olhos, como os do Ruivo do filme: cada olho numa metade de uma textura só (o do lado de x < 0 na de
    # u menor), com as coordenadas tiradas da posição de cada ponto no olho (u de um lado ao outro, v de cima
    # para baixo), para a íris sair redonda no modelo. O branco grande, a pálpebra (a lataria) no alto com o
    # traço escuro, e a íris redonda, cor de âmbar, com a pupila grande e o brilho
    olhos_do_ruivo = sorted((o for o in malhas() if o.data.materials and o.data.materials[0].name in ("red_eye_r", "red_eye_l")),
                            key=lambda o: sum(v.co.x for v in o.data.vertices))
    largura_do_olho = altura_do_olho = 1.0
    for metade, o in enumerate(olhos_do_ruivo):
        pontos = [v.co.copy() for v in o.data.vertices]
        x0, x1 = min(p.x for p in pontos), max(p.x for p in pontos)
        baixo, cima = min(p.z for p in pontos), max(p.z for p in pontos)
        largura_do_olho, altura_do_olho = x1 - x0, cima - baixo
        camada = o.data.uv_layers.active.data
        for laco in o.data.loops:
            p = pontos[laco.vertex_index]
            u = 0.5 * metade + 0.5 * (p.x - x0) / (x1 - x0)
            v = (cima - p.z) / (cima - baixo)
            camada[laco.index].uv = (u, 1.0 - v)
    print(f"FIGURANTE: olho do Ruivo de {largura_do_olho:.3f} x {altura_do_olho:.3f}")

    L, A = 2 * LADO_DO_OLHO_DO_RUIVO, LADO_DO_OLHO_DO_RUIVO
    coluna = np.arange(L)
    metade = coluna >= L // 2
    u_no_olho = ((coluna % (L // 2)) + 0.5) / (L // 2)
    x_do_olho = np.where(metade, u_no_olho, 1.0 - u_no_olho)   # 0 no lado do nariz, 1 no de fora
    X = np.broadcast_to(x_do_olho[None, :], (A, L))
    Y = np.broadcast_to(((np.arange(A) + 0.5) / A)[:, None], (A, L))
    pixel = altura_do_olho / A


    def suave(borda, d):
        """0 dentro, 1 fora, com um pixel e meio de transição"""
        return np.clip((d - borda) / (1.5 * pixel) + 0.5, 0.0, 1.0)


    def misturar(a, b, quanto):
        return a * (1.0 - quanto[..., None]) + np.asarray(b) * quanto[..., None]


    abaixo = (Y - RUIVO_PALPEBRA) * altura_do_olho          # > 0 no branco
    branco = 0.97 - 0.15 * np.exp(-np.clip(abaixo, 0, None) / (0.18 * altura_do_olho)) - 0.06 * ((X - 0.5) * 2) ** 4
    fundo = np.stack([branco * 0.99, branco * 0.98, branco * 0.96], axis=-1)
    fundo = misturar(fundo, (0.10, 0.03, 0.03), 1.0 - suave(0.0, abaixo - RUIVO_CILIOS * altura_do_olho))
    fundo = misturar(fundo, cor_da_lataria, 1.0 - suave(0.0, abaixo))
    mascara = suave(0.0, abaixo - RUIVO_CILIOS * altura_do_olho)

    raio = RUIVO_RAIO * altura_do_olho
    dx, dy = (X - RUIVO_IRIS_X) * largura_do_olho, (Y - RUIVO_IRIS_Y) * altura_do_olho
    r = np.hypot(dx, dy) / raio
    angulo = np.arctan2(dy, dx)
    t = np.clip((r - RUIVO_PUPILA) / (1.0 - RUIVO_PUPILA), 0.0, 1.0)
    forma = (A, L, 3)
    cor_da_iris = np.where(t[..., None] < 0.5,
                           misturar(np.broadcast_to(RUIVO_IRIS_DENTRO, forma).copy(), RUIVO_IRIS_MEIO, t / 0.5),
                           misturar(np.broadcast_to(RUIVO_IRIS_MEIO, forma).copy(), RUIVO_IRIS_FORA, (t - 0.5) / 0.5))
    cor_da_iris *= (1.0 + 0.09 * np.sin(angulo * 27.0) * (t > 0.05))[..., None]
    cor_da_iris = misturar(cor_da_iris, (0.16, 0.07, 0.02), 1.0 - suave(0.0, (0.93 - r) * raio))   # o aro escuro
    cor_da_iris = misturar(cor_da_iris, (0.04, 0.03, 0.03), 1.0 - suave(RUIVO_PUPILA * raio, r * raio))
    brilho = np.hypot(dx + 0.30 * raio * np.where(metade, 1.0, -1.0)[None, :], dy + 0.30 * raio)
    cor_da_iris = misturar(cor_da_iris, (1, 1, 1), 1.0 - suave(0.13 * raio, brilho))
    alfa = 1.0 - suave(raio, r * raio)
    iris = np.concatenate([cor_da_iris, alfa[..., None]], axis=-1)
    gravar("ruivo_fundo", fundo)
    gravar("ruivo_iris", iris, alfa=True)
    gravar("ruivo_mascara", np.repeat(mascara[..., None], 3, axis=-1))

    # O modelo: os dois olhos numa malha, com um material só e a textura junta (o fundo com as íris)
    a = (alfa * mascara)[..., None]
    textura = nova_imagem("ruivo_olhos", fundo * (1 - a) + cor_da_iris * a)
    textura.pack()
    material = olhos_do_ruivo[0].data.materials[0]
    material.name = "red_eyes"
    no = no_de_imagem(material)
    no.image = textura
    no.extension = 'EXTEND'
    olhos_do_ruivo[1].data.materials[0] = material
    bpy.ops.object.select_all(action='DESELECT')
    for o in olhos_do_ruivo:
        o.select_set(True)
    bpy.context.view_layer.objects.active = olhos_do_ruivo[0]
    bpy.ops.object.join()
    caminho = exportar("ruivo.glb")
    # na tabela: a íris anda de lado ANDA_POR_RAIO raios; os meios, um em cada metade
    raio_u, raio_v = raio / largura_do_olho * 0.5, RUIVO_RAIO
    cx = (0.5 * (1.0 - RUIVO_IRIS_X) * L, (0.5 + 0.5 * RUIVO_IRIS_X) * L)
    linha_dos_olhos("ruivo", caminho, "red_eyes", [(cx[0], RUIVO_IRIS_Y * A, raio_u * L, raio_v * A),
                                                   (cx[1], RUIVO_IRIS_Y * A, raio_u * L, raio_v * A)], L, A, -1)

    gravar_tabela_dos_olhos()


# Rodando direto (blender -P), prepara os figurantes; importado (preparar_carros.py), só empresta as funções
if __name__ == "__main__":
    preparar()
