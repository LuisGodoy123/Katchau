"""
preparar_montreal.py
------------------------------------------------------------------
Prepara o modelo do Circuito Gilles-Villeneuve (Montreal) para o jogo e grava
recursos/modelos/montreal.glb:

    /Applications/Blender.app/Contents/Resources/5.2/python/bin/python3.13 ferramentas/preparar_montreal.py \
        ~/Downloads/circuit-gilles-villeneuve-montreal-2019-layout/source/montreal.glb recursos/modelos/montreal.glb

(qualquer Python com o numpy serve; o do Blender já tem. As imagens são reduzidas com o
"sips" do macOS.)

O modelo baixado tem uma malha só, com 25 partes (uma por material), e algumas passam
de 65 mil vértices (o alambrado tem 230 mil, os prédios 388 mil). A raylib aponta os
vértices de uma malha com 16 bits: o que passa disso vira lixo. Aqui:

  1. a rotação do nó (o modelo vem deitado, com z para cima) entra nos vértices, então
     o arquivo novo já está em y para cima, nos eixos do jogo, em metros;
  2. cada material grande é cortado em ladrilhos de LADO_DO_LADRILHO metros, vistos de
     cima (os que se espalham por quilômetros, em ladrilhos maiores), e cada ladrilho em
     pedaços de até MAXIMO_DE_VERTICES vértices. Os ladrilhos também deixam o corte pelo
     campo de visão (corte.c) descartar o que está longe;
  3. as imagens maiores que o limite de cada uma são reduzidas (as árvores e os prédios
     vêm em 4096 pixels);
  4. a ordem dos materiais é a mesma do modelo original: a raylib põe o material padrão
     na frente, então o material i do modelo é o i + 1 do jogo (montreal_materiais.h).
------------------------------------------------------------------
"""

import json
import os
import struct
import subprocess
import sys
import tempfile

import numpy as np

LADO_DO_LADRILHO = 300.0       # metros
LADO_DO_LADRILHO_DE_LONGE = 1500.0   # o que se espalha por quilômetros (a água, a grama, a cidade ao longe):
EXTENSAO_DE_LONGE = 2500.0           # ladrilhos de 300 m dariam dezenas de malhas, quase todas com poucos triângulos
MAXIMO_DE_VERTICES = 60000
TRIANGULOS_PARA_CORTAR = 3000  # materiais menores que isso (e pequenos no mapa) ficam inteiros
EXTENSAO_PARA_CORTAR = 600.0   # metros: ...a não ser que se espalhem por mais que isso
MAIOR_IMAGEM = 2048
MAIOR_IMAGEM_DE = {'groove_01': 1024}   # a borracha do traçado é um borrão: não precisa de detalhe


def ler_glb(caminho):
    with open(caminho, 'rb') as f:
        dados = f.read()
    tamanho_json = struct.unpack('<I', dados[12:16])[0]
    gltf = json.loads(dados[20:20 + tamanho_json])
    inicio = 20 + tamanho_json
    tamanho_bin = struct.unpack('<I', dados[inicio:inicio + 4])[0]
    return gltf, dados[inicio + 8:inicio + 8 + tamanho_bin]


def acessor(gltf, binario, indice):
    a = gltf['accessors'][indice]
    vista = gltf['bufferViews'][a['bufferView']]
    tipo = {5126: np.float32, 5125: np.uint32, 5123: np.uint16, 5121: np.uint8}[a['componentType']]
    componentes = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3, 'VEC4': 4}[a['type']]
    comeco = vista.get('byteOffset', 0) + a.get('byteOffset', 0)
    arr = np.frombuffer(binario, dtype=tipo, count=a['count'] * componentes, offset=comeco)
    return arr.reshape(-1, componentes) if componentes > 1 else arr


def matriz_do_no(no):
    """Só rotação, translação e escala (o modelo baixado só tem a rotação)"""
    if 'matrix' in no:
        return np.array(no['matrix'], dtype=np.float64).reshape(4, 4).T
    x, y, z, w = no.get('rotation', [0.0, 0.0, 0.0, 1.0])
    r = np.array([
        [1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)],
        [2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)],
        [2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)],
    ])
    m = np.eye(4)
    m[:3, :3] = r * np.array(no.get('scale', [1.0, 1.0, 1.0]))
    m[:3, 3] = no.get('translation', [0.0, 0.0, 0.0])
    return m


def pedacos(indices, maximo):
    """Corta uma lista de triângulos em pedaços de até "maximo" vértices diferentes, na ordem"""
    saida = []
    comeco = 0
    total = len(indices)
    while comeco < total:
        vistos = set()
        fim = comeco
        while fim < total:
            novos = set(indices[fim].tolist()) - vistos
            if len(vistos) + len(novos) > maximo:
                break
            vistos |= novos
            fim += 1
        saida.append(indices[comeco:fim])
        comeco = fim
    return saida


def reduzir_imagem(png, nome, pasta):
    limite = MAIOR_IMAGEM_DE.get(nome, MAIOR_IMAGEM)
    largura, altura = struct.unpack('>II', png[16:24])
    if max(largura, altura) <= limite:
        return png
    entrada = os.path.join(pasta, nome + '_original.png')
    saida = os.path.join(pasta, nome + '.png')
    with open(entrada, 'wb') as f:
        f.write(png)
    subprocess.run(['sips', '-Z', str(limite), entrada, '--out', saida], check=True, stdout=subprocess.DEVNULL)
    with open(saida, 'rb') as f:
        reduzida = f.read()
    print('  imagem %-20s %4dx%-4d -> %d' % (nome, largura, altura, limite))
    return reduzida


class Escritor:
    """Junta os dados do arquivo novo num buffer só, alinhado a 4 bytes"""

    def __init__(self):
        self.binario = bytearray()
        self.vistas = []
        self.acessores = []

    def vista(self, dados, alvo=None):
        while len(self.binario) % 4:
            self.binario.append(0)
        v = {'buffer': 0, 'byteOffset': len(self.binario), 'byteLength': len(dados)}
        if alvo is not None:
            v['target'] = alvo
        self.binario += dados
        self.vistas.append(v)
        return len(self.vistas) - 1

    def acessor(self, arr, tipo, componente, alvo, com_limites=False):
        a = {'bufferView': self.vista(arr.tobytes(), alvo), 'componentType': componente,
             'count': int(arr.shape[0]), 'type': tipo}
        if com_limites:
            a['min'] = arr.min(axis=0).astype(float).tolist()
            a['max'] = arr.max(axis=0).astype(float).tolist()
        self.acessores.append(a)
        return len(self.acessores) - 1


def main():
    if len(sys.argv) < 3:
        print('uso: preparar_montreal.py original.glb saida.glb')
        sys.exit(1)
    gltf, binario = ler_glb(sys.argv[1])
    escritor = Escritor()
    malhas, nos = [], []

    for indice_no, no in enumerate(gltf['nodes']):
        if 'mesh' not in no:
            continue
        m = matriz_do_no(no)
        rotacao = m[:3, :3]
        for primitiva in gltf['meshes'][no['mesh']]['primitives']:
            material = primitiva['material']
            nome = gltf['materials'][material]['name']
            pos = acessor(gltf, binario, primitiva['attributes']['POSITION']).astype(np.float64)
            pos = pos @ m[:3, :3].T + m[:3, 3]
            nor = acessor(gltf, binario, primitiva['attributes']['NORMAL']).astype(np.float64) @ rotacao.T
            nor /= np.maximum(np.linalg.norm(nor, axis=1, keepdims=True), 1e-12)
            uv = acessor(gltf, binario, primitiva['attributes']['TEXCOORD_0']).astype(np.float32)
            tri = acessor(gltf, binario, primitiva['indices']).astype(np.int64).reshape(-1, 3)

            centro = pos[tri].mean(axis=1)
            extensao = max(np.ptp(pos[:, 0]), np.ptp(pos[:, 2]))
            if len(tri) > TRIANGULOS_PARA_CORTAR or extensao > EXTENSAO_PARA_CORTAR:
                lado = LADO_DO_LADRILHO_DE_LONGE if extensao > EXTENSAO_DE_LONGE else LADO_DO_LADRILHO
                lx = np.floor(centro[:, 0] / lado).astype(np.int64)
                lz = np.floor(centro[:, 2] / lado).astype(np.int64)
                chave = lx * 100000 + lz
            else:
                chave = np.zeros(len(tri), np.int64)
            partes = 0
            for k in np.unique(chave):
                do_ladrilho = tri[chave == k]
                for pedaco in pedacos(do_ladrilho, MAXIMO_DE_VERTICES):
                    usados, novos = np.unique(pedaco.reshape(-1), return_inverse=True)
                    p = pos[usados].astype(np.float32)
                    atributos = {
                        'POSITION': escritor.acessor(p, 'VEC3', 5126, 34962, True),
                        'NORMAL': escritor.acessor(nor[usados].astype(np.float32), 'VEC3', 5126, 34962),
                        'TEXCOORD_0': escritor.acessor(uv[usados], 'VEC2', 5126, 34962),
                    }
                    ind = escritor.acessor(novos.astype(np.uint16), 'SCALAR', 5123, 34963)
                    malhas.append({'name': '%s_%d' % (nome, partes),
                                   'primitives': [{'attributes': atributos, 'indices': ind, 'material': material}]})
                    nos.append({'name': '%s_%d' % (nome, partes), 'mesh': len(malhas) - 1})
                    partes += 1
            print('%-18s %6d triangulos -> %3d malha(s)' % (nome, len(tri), partes))

    # Materiais: só a imagem de cor (a raylib não usa o resto), na mesma ordem
    imagens, texturas, materiais = [], [], []
    textura_da_imagem = {}
    pasta = tempfile.mkdtemp(prefix='montreal_')
    for mat in gltf['materials']:
        novo = {'name': mat['name'], 'doubleSided': True,
                'pbrMetallicRoughness': {'metallicFactor': 0.0, 'roughnessFactor': 1.0}}
        cor = mat.get('pbrMetallicRoughness', {}).get('baseColorTexture')
        if cor is not None:
            fonte = gltf['textures'][cor['index']]['source']
            if fonte not in textura_da_imagem:
                img = gltf['images'][fonte]
                vista = gltf['bufferViews'][img['bufferView']]
                comeco = vista.get('byteOffset', 0)
                png = reduzir_imagem(binario[comeco:comeco + vista['byteLength']], img['name'], pasta)
                imagens.append({'name': img['name'], 'mimeType': 'image/png', 'bufferView': escritor.vista(png)})
                texturas.append({'sampler': 0, 'source': len(imagens) - 1})
                textura_da_imagem[fonte] = len(texturas) - 1
            novo['pbrMetallicRoughness']['baseColorTexture'] = {'index': textura_da_imagem[fonte]}
        materiais.append(novo)

    saida = {
        'asset': {'version': '2.0', 'generator': 'KATCHAU ferramentas/preparar_montreal.py'},
        'scene': 0,
        'scenes': [{'name': 'montreal', 'nodes': list(range(len(nos)))}],
        'nodes': nos,
        'meshes': malhas,
        'materials': materiais,
        'textures': texturas,
        'images': imagens,
        'samplers': [{'magFilter': 9729, 'minFilter': 9987, 'wrapS': 10497, 'wrapT': 10497}],
        'accessors': escritor.acessores,
        'bufferViews': escritor.vistas,
        'buffers': [{'byteLength': len(escritor.binario)}],
    }
    texto = json.dumps(saida, separators=(',', ':')).encode()
    texto += b' ' * ((4 - len(texto) % 4) % 4)
    while len(escritor.binario) % 4:
        escritor.binario.append(0)
    with open(sys.argv[2], 'wb') as f:
        f.write(struct.pack('<III', 0x46546C67, 2, 12 + 8 + len(texto) + 8 + len(escritor.binario)))
        f.write(struct.pack('<II', len(texto), 0x4E4F534A) + texto)
        f.write(struct.pack('<II', len(escritor.binario), 0x004E4942) + bytes(escritor.binario))
    print('%d malhas, %d materiais, %.1f MB: %s' % (len(malhas), len(materiais), os.path.getsize(sys.argv[2]) / 1e6, sys.argv[2]))


if __name__ == '__main__':
    main()
