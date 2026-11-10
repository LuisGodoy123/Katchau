"""
preparar_carros.py
------------------------------------------------------------------
Prepara para o jogo os modelos dos carros da corrida que vêm do Sketchfab
e precisam de conserto (as fichas estão em src/corrida.c, FICHAS, e os
arquivos em src/carros3d.c, MODELOS):

    blender -b --factory-startup -P ferramentas/preparar_carros.py -- \\
        ~/Downloads recursos/modelos/carros

Lê luigi.glb, jackson_storm.glb, boost.glb e o DJ (com o nome que o
Sketchfab dá, 13f61e6a113e41deaeb4b7a33adf5d72.glb) da pasta de origem e grava
luigi.glb, jackson_storm.glb, boost.glb e dj.glb na de destino. O wingo.glb
é lido da pasta de destino mesmo (o que já está no jogo) e consertado lá. O
que não estiver na pasta é pulado. O jogo (carros3d.c) mede cada um, acha as
rodas e deixa no tamanho da ficha.

Grava também os olhos de cada um no jeito dos outros carros: as três camadas
em recursos/modelos/carros/olhos/ (<nome>_fundo.png, _iris.png e
_mascara.png) e a linha dele em olhos.txt. A separação das camadas é a dos
figurantes (preparar_figurantes.py, separar_olhos): a íris é achada com raios
saindo do meio dela até o branco, o fundo é a textura sem as íris e a máscara
é o branco do olho.

O que muda em cada um:
  - Todos: as texturas em JPEG viram PNG (a raylib do jogo não lê JPEG) e as
    transformações dos nós vão para os vértices. O jogo não desenha o lado de
    trás das faces, então a frente delas e as normais têm de apontar para fora.
  - Luigi: as peças dele vêm espelhadas num nó. No glTF, isso inverte o lado
    da frente das faces, mas a raylib não segue essa regra: o Luigi antigo
    tinha as normais para dentro (por isso saía acinzentado, com a luz do lado
    errado). Ao passar o espelho para os vértices, o Blender embaralha as
    normais: elas são guardadas antes, no mundo, e postas de novo depois, e as
    peças espelhadas viram as faces e as normais para fora. Os olhos dele são
    os pintados no para-brisa (o Luigi antigo tinha a pálpebra quase fechada);
    saem duas pecinhas de olho escondidas dentro do carro, que não aparecem.
  - Wingo: a lataria vem do avesso, com as faces e as normais para dentro. No
    jogo ele aparecia oco: as peças de perto sumiam e se via o lado de dentro
    das de longe (o lado de baixo, os para-lamas por dentro). Essas peças
    viram para fora; a linha dele em olhos.txt só ganha o número novo do
    material dos olhos (as camadas são as mesmas).
  - DJ: vem deitado, com a frente em +x: gira para a frente ficar em +z,
    como nos outros.
------------------------------------------------------------------
"""
import math
import os
import sys

import bpy
import mathutils

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.dont_write_bytecode = True      # sem a pasta __pycache__ ao lado das ferramentas
import preparar_figurantes as fig   # noqa: E402  (lê as mesmas pastas: -- origem destino)

# Os carros com os olhos pintados: o arquivo de origem, o nome no jogo, o material dos olhos, onde
# fica mais ou menos o meio de cada íris (u, v da textura dos olhos, v de cima para baixo), o lado da
# pálpebra na textura (+1 embaixo: a textura está de cabeça para baixo no para-brisa; -1 em cima) e
# os ajustes da separação (preparar_figurantes.separar_olhos). A primeira íris do Boost passa da borda
# da textura e a do Storm entra embaixo da pálpebra: as duas saíam grandes demais e pegavam um pedaço
# da pálpebra (mesmo_tamanho e sem_a_palpebra), e as duas viram cópia da segunda, que está inteira
# (copiar_iris); a íris cinza-azulada do Storm tem pouca cor
CARROS = [
    ("luigi.glb", "luigi", "Material_1", ((0.472, 0.357), (0.769, 0.363)), 1, {}),
    ("jackson_storm.glb", "jackson_storm", "Material.013", ((0.491, 0.686), (0.791, 0.680)), -1,
     {"mesmo_tamanho": True, "cor_da_iris": 0.12, "sem_a_palpebra": True, "copiar_iris": 1}),
    ("boost.glb", "boost", "Material.021", ((0.546, 0.248), (0.810, 0.284)), 1,
     {"mesmo_tamanho": True, "sem_a_palpebra": True, "copiar_iris": 1}),
    ("13f61e6a113e41deaeb4b7a33adf5d72.glb", "dj", "Material.028", ((0.650, 0.225), (0.872, 0.234)), 1, {}),
]
SEM_AS_PECAS = {"luigi": ("lui_eye.001", "g_eye_shadow.001")}   # escondidas dentro do carro
DEITADOS = {"dj": -math.pi / 2.0}                                 # giro em volta do eixo de cima
# As peças do avesso (faces e normais para dentro), pelo material: viram se a maior delas ainda está
# do avesso (preparar_figurantes.esta_do_avesso), então rodar de novo no Wingo já consertado não o
# desconserta. Medido: nessas, só 0 a 23% das faces apontam para fora do meio do carro (nas boas de
# todos os modelos, 50% ou mais), e nenhum raio sai do carro pela frente delas
DO_AVESSO = {"wingo": ("material_0", "mat_0.005", "mat_3.005", "mat_1.003", "mat_0.001")}
MATERIAL_DOS_OLHOS_DO_WINGO = "mat_0.005"


def faces_contra_as_normais(o):
    """A fração das faces cuja frente (a ordem dos cantos) aponta contra as normais dos cantos"""
    me = o.data
    normais = me.corner_normals
    contra = 0
    for p in me.polygons:
        soma = mathutils.Vector()
        for canto in p.loop_indices:
            soma += normais[canto].vector
        contra += p.normal.dot(soma) < 0.0
    return contra / max(len(me.polygons), 1)


def normais_no_mundo():
    """As normais de cada canto de cada malha, no mundo (a matriz das normais leva o espelho certo)"""
    guardadas = {}
    for o in fig.malhas():
        normais = o.matrix_world.to_3x3().inverted().transposed()
        guardadas[o.name] = [(normais @ c.vector).normalized() for c in o.data.corner_normals]
    return guardadas


def material_de(o):
    return o.data.materials[0].name if o.data.materials else ""


def acertar_faces(nome, espelhadas):
    listadas = [o for o in fig.malhas() if material_de(o) in DO_AVESSO.get(nome, ())]
    if listadas and not fig.esta_do_avesso(max(listadas, key=lambda o: len(o.data.polygons))):
        print(f"CARRO: {nome} já está do lado certo")
        listadas = []
    for o in fig.malhas():
        material = material_de(o)
        if o in listadas or o.name in espelhadas:
            fig.virar_para_fora(o)
            print(f"CARRO: {nome}, {material}: {'espelhada' if o.name in espelhadas else 'do avesso'}, virada para fora")
        contra = faces_contra_as_normais(o)
        if contra > 0.05:
            print(f"CARRO: {nome}, {material}: {100 * contra:.0f}% das faces contra as normais")


def preparar(arquivo, nome):
    fig.importar(arquivo)
    for o in [o for o in fig.malhas() if o.data.materials and o.data.materials[0].name in SEM_AS_PECAS.get(nome, ())]:
        print(f"CARRO: {nome} sem a peça {o.name} ({o.data.materials[0].name})")
        bpy.data.objects.remove(o, do_unlink=True)
    normais = normais_no_mundo()
    espelhadas = {o.name for o in fig.malhas() if o.matrix_world.determinant() < 0.0}
    fig.aplicar_transformacoes()
    for o in fig.malhas():
        o.data.normals_split_custom_set(normais[o.name])
    if nome in DEITADOS:
        giro = mathutils.Matrix.Rotation(DEITADOS[nome], 4, 'Z')
        for o in fig.malhas():
            o.data.transform(giro)
    acertar_faces(nome, espelhadas)
    fig.sem_jpeg()


os.makedirs(fig.PASTA_DOS_OLHOS, exist_ok=True)

for arquivo, nome, material, sementes, lado, ajustes in CARROS:
    if not os.path.exists(os.path.join(fig.origem, arquivo)):
        print(f"CARRO: {arquivo} não está em {fig.origem}, pulado")
        continue
    preparar(arquivo, nome)
    imagem = fig.no_de_imagem(bpy.data.materials[material]).image
    elipses, largura, altura = fig.separar_olhos(nome, imagem, sementes, True, **ajustes)
    caminho = fig.exportar(nome + ".glb")
    fig.linha_dos_olhos(nome, caminho, material, elipses, largura, altura, lado)

# O Wingo: só as faces (os olhos continuam os mesmos; muda só o número do material na linha dele)
wingo = os.path.join(fig.destino, "wingo.glb")
tabela = os.path.join(fig.PASTA_DOS_OLHOS, "olhos.txt")
linha_velha = next((l for l in open(tabela).read().splitlines() if l.split(" ")[0] == "wingo"), None)
if os.path.exists(wingo) and linha_velha:
    preparar(os.path.abspath(wingo), "wingo")
    caminho = fig.exportar("wingo.glb")
    campos = linha_velha.split(" ")
    campos[1] = str(fig.materiais_do_glb(caminho).index(MATERIAL_DOS_OLHOS_DO_WINGO))
    fig.linhas_dos_olhos["wingo"] = " ".join(campos)
    print("CARRO: olhos.txt ->", fig.linhas_dos_olhos["wingo"])

fig.gravar_tabela_dos_olhos()
