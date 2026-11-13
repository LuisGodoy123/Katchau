"""
medir_bocas.py
------------------------------------------------------------------
Mede a mandíbula de cada morador, para a boca mexer quando ele fala
(carros3d.c), e grava recursos/modelos/carros/bocas.txt:

    /Applications/Blender.app/Contents/Resources/5.2/python/bin/python3.13 ferramentas/medir_bocas.py

(qualquer Python com o numpy serve; o do Blender já tem.)

Todos os modelos dos moradores têm a boca modelada, não pintada: os lábios,
o "saco" da boca atrás deles (com teto, chão e fundo), os dentes de cima, os
de baixo e a língua (os dentes e a língua são ilhas soltas). Então dá para
abrir a boca girando a mandíbula (o lábio de baixo, o queixo, os dentes de
baixo e a língua) em volta de uma dobradiça na horizontal, de lado a lado do
carro: abrindo, aparece o lado de dentro de verdade.

Não dá para escolher a mandíbula por uma caixa: em quase todos os modelos os
lábios de cima e de baixo são superfícies separadas que se sobrepõem um pouco
na boca fechada (no McQueen, o de baixo sobe até y = 1,522 e o de cima desce
até 1,474), e nenhum plano separa os dois. A escolha é pela malha:
  1. cada vértice (os do mesmo lugar contam como um) é da zona dos lábios
     (z > zl e |x - xc| < xk), senão "de cima" (y > ys) ou "de baixo";
  2. a região R é |x - xc| <= xr, z >= zb, y >= yc;
  3. a mandíbula é o pedaço ligado pelas arestas dos triângulos, dentro de R,
     que contém o lábio de baixo (a semente), sem atravessar de "cima" para
     fora de "cima" (a fenda da boca), mais as ilhas inteiras listadas (os
     dentes de baixo, a língua e, em alguns, o para-choque que faz de lábio);
  4. o peso de cada ponto cai suave para os cantos da boca e para o queixo:
     w = (1 - suave(x_in, x_out, |x - xc|)) * suave(y_bot0, y_bot1, y),
     e é 1 nas ilhas "forçadas".
Os números de cada modelo foram medidos olhando as renderizações da boca
aberta em 12, 20, 30 e 40 graus. Até 20 graus todos ficam bem; os que têm o
para-choque ou um friso de cromo como lábio de baixo (o Doc, o Xerife, o
Fillmore, o Sargento e o Luigi) entortam em V acima disso, e ficam em 15 (o
Fillmore em 12: mais que isso, abre uma fresta entre o lábio de cima e a língua).

O Mack também (recursos/modelos/mack.glb): a boca dele fica no para-choque, e
a mandíbula é o queixo inteiro.

Os modelos são lidos como a raylib os carrega (a matriz de cada nó aplicada;
uma malha por primitiva, na ordem dos nós), e os pontos saem nesse
referencial, que é o que o vertexPosition do shader recebe. As ilhas são
numeradas como em mandibula: a posição da raiz de cada uma na ordem das
raízes, depois de soldar os vértices do mesmo lugar.

bocas.txt: uma linha "boca <modelo> <dobradiça y> <dobradiça z> <graus> <pontos>"
por modelo e, depois dela, um ponto por linha: "x y z peso" (só os de peso > 0;
os pontos repetidos saem uma vez). O jogo acha cada vértice pela posição.
------------------------------------------------------------------
"""
import json
import os
import struct
import sys
from collections import deque

import numpy as np

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PASTA = os.path.join(REPO, "recursos", "modelos", "carros")
SAIDA = os.path.join(PASTA, "bocas.txt")

# Por modelo: a mandíbula ("flood": uma por malha onde ela fica), a dobradiça (y, z), o peso e o limite
BOCAS = {
    "mcqueen": dict(flood=[dict(mesh=0, xc=0.0, ys=1.44, zl=6.79, xk=1.25, xr=1.9, zb=5.8, yc=0.6, Ht=0.9005, thw=1.3659,
                                islands=[0, 4])],
                    hinge=(1.44, 5.86), w=dict(x_in=1.0, x_out=1.9, y_bot0=0.6, y_bot1=0.85), graus=20),
    "mater": dict(flood=[dict(mesh=5, xc=0.0065, ys=0.825, zl=2.0, xk=0.0, xr=0.6, zb=1.65, yc=0.62, Ht=0.2645, thw=0.4287,
                              islands=[1, 14], seeds=[[0.0, 0.777, 2.193]])],
                  hinge=(0.825, 1.72), w=dict(x_in=0.25, x_out=0.6, y_bot0=0.62, y_bot1=0.7), graus=20),
    "sally": dict(flood=[dict(mesh=1, xc=0.0, ys=0.375, zl=1.919, xk=0.34, xr=0.67, zb=1.42, yc=0.19, Ht=0.2034, thw=0.4778,
                              islands=[26, 28, 29])],
                  hinge=(0.375, 1.5), w=dict(x_in=0.25, x_out=0.67, y_bot0=0.19, y_bot1=0.25), graus=20),
    "doc_hudson": dict(flood=[dict(mesh=3, xc=0.0, ys=0.43, zl=2.0, xk=0.0, xr=0.95, zb=1.25, yc=0.26, Ht=0.1822, thw=0.4405,
                                   islands=[25, 14, 26], seeds=[[0.0, 0.35, 1.6]])],
                       hinge=(0.43, 1.3), w=dict(x_in=0.55, x_out=0.95, y_bot0=0.2, y_bot1=0.265), graus=15),
    "guido": dict(flood=[dict(mesh=3, xc=0.0, ys=0.39, zl=0.648, xk=0.13, xr=0.25, zb=0.4, yc=0.29, Ht=0.1006, thw=0.1774,
                              islands=[1, 0])],
                  hinge=(0.39, 0.48), w=dict(x_in=0.09, x_out=0.25, y_bot0=0.29, y_bot1=0.33), graus=20),
    "ramone": dict(flood=[dict(mesh=4, xc=0.0038, ys=0.318, zl=2.389, xk=0.45, xr=0.614, zb=1.872, yc=0.127, Ht=0.2124,
                               thw=0.4386, islands=[29, 38])],
                   hinge=(0.318, 1.98), w=dict(x_in=0.33, x_out=0.61, y_bot0=0.13, y_bot1=0.18), graus=20),
    "flo": dict(flood=[dict(mesh=0, xc=0.0, ys=1.42, zl=6.387, xk=1.15, xr=1.635, zb=5.389, yc=0.527, Ht=0.918, thw=1.168,
                            islands=[11, 16])],
                hinge=(1.42, 5.58), w=dict(x_in=0.85, x_out=1.63, y_bot0=0.53, y_bot1=0.7), graus=20),
    "xerife": dict(flood=[dict(mesh=3, xc=0.0028, ys=0.56, zl=1.9425, xk=0.47, xr=0.7356, zb=1.393, yc=0.356, Ht=0.2121,
                               thw=0.5255, islands=[9, 25]),
                          dict(mesh=0, xc=0.0028, ys=0.6, zl=2.1, xk=0.49, xr=1.2, zb=1.8, yc=0.35, Ht=0.2121, thw=0.5255,
                               seeds=[[0.0, 0.4634, 2.2493]])],
                   hinge=(0.56, 1.485), w=dict(x_in=0.35, x_out=0.8, y_bot0=0.36, y_bot1=0.42), graus=15),
    "sarge": dict(flood=[dict(mesh=2, xc=0.0, ys=0.6, zl=1.29, xk=0.205, xr=0.35, zb=1.05, yc=0.5, Ht=0.1301, thw=0.2173,
                              islands=[67, 132], seeds=[[0.0, 0.5367, 1.2987]])],
                  hinge=(0.6, 1.08), w=dict(x_in=0.15, x_out=0.35, y_bot0=0.5, y_bot1=0.55), graus=15),
    "fillmore": dict(flood=[dict(mesh=4, xc=0.0, ys=0.592, zl=2.2, xk=0.0, xr=0.6, zb=1.65, yc=0.4, Ht=0.1773, thw=0.4259,
                                 islands=[52, 30, 51, 28], seeds=[[0.05, 0.495, 2.0]])],
                     hinge=(0.59, 1.7), w=dict(x_in=0.35, x_out=0.6, y_bot0=0.4, y_bot1=0.42,
                                               force=[[4, 52, 1.0], [4, 28, 1.0], [4, 30, 1.0], [4, 51, 1.0]]), graus=12),
    "lizzie": dict(flood=[dict(mesh=0, xc=0.0317, ys=1.86, zl=3.4348, xk=0.7, xr=0.905, zb=2.807, yc=1.428, Ht=0.4844,
                               thw=0.6465, islands=[41, 53])],
                   hinge=(1.86, 3.0), w=dict(x_in=0.5, x_out=0.9, y_bot0=1.43, y_bot1=1.57), graus=20),
    "ruivo": dict(flood=[dict(mesh=0, xc=0.0, ys=0.0334, zl=0.11, xk=0.0175, xr=0.05, zb=0.09, yc=0.018, Ht=0.0133, thw=0.0207,
                              islands=[1, 5, 0], seeds=[[0.0, 0.031, 0.1175]])],
                  hinge=(0.0334, 0.092), w=dict(x_in=0.03, x_out=0.05, y_bot0=0.018, y_bot1=0.019), graus=20),
    # O Mack (recursos/modelos/mack.glb): a boca fica no para-choque, embaixo da grade. Os lábios se separam por
    # um plano (uma fresta de 5 mm), então não há zona dos lábios; o queixo (o para-choque inteiro) desce junto
    "mack": dict(arquivo=os.path.join(REPO, "recursos", "modelos", "mack.glb"),
                 flood=[dict(mesh=0, xc=0.0, ys=1.351, zl=15.0, xk=0.0, xr=1.3, zb=13.86, yc=0.6, Ht=0.6237, thw=0.902,
                             islands=[9, 1], seeds=[[0.0, 1.2508, 15.2671]])],
                 hinge=(1.351, 13.86), w=dict(x_in=0.7, x_out=1.3, y_bot0=0.5, y_bot1=0.6), graus=20),
    "luigi": dict(flood=[dict(mesh=3, xc=0.0, ys=0.133, zl=0.3, xk=0.0, xr=0.12, zb=0.3, yc=0.075, Ht=0.0592, thw=0.0822,
                              islands=[6], seeds=[[0.0, 0.1217, 0.3576]]),
                         dict(mesh=2, xc=0.0, ys=0.13305, zl=0.3, xk=0.0, xr=0.12, zb=0.235, yc=0.09, Ht=0.0592, thw=0.0822,
                              islands=[9, 18], seeds=[[0.0, 0.104, 0.3]])],
                  hinge=(0.133, 0.241), w=dict(x_in=0.06, x_out=0.12, y_bot0=0.075, y_bot1=0.1), graus=15),
}

# ---------------- O .glb como a raylib o carrega ----------------
TIPOS = {5120: np.int8, 5121: np.uint8, 5122: np.int16, 5123: np.uint16, 5125: np.uint32, 5126: np.float32}
COMPONENTES = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4, "MAT4": 16}


def ler_glb(caminho):
    b = open(caminho, "rb").read()
    _, _, tamanho = struct.unpack_from("<III", b, 0)
    pos, js, binario = 12, None, None
    while pos < tamanho:
        n, tipo = struct.unpack_from("<II", b, pos)
        pedaco = b[pos + 8: pos + 8 + n]
        if tipo == 0x4E4F534A:
            js = json.loads(pedaco)
        elif tipo == 0x004E4942:
            binario = pedaco
        pos += 8 + n
    return js, binario


def acessor(js, binario, i):
    a = js["accessors"][i]
    n, tipo, quantos = COMPONENTES[a["type"]], np.dtype(TIPOS[a["componentType"]]), a["count"]
    vista = js["bufferViews"][a["bufferView"]]
    comeco = vista.get("byteOffset", 0) + a.get("byteOffset", 0)
    passo = vista.get("byteStride", 0)
    if passo and passo != n * tipo.itemsize:
        return np.array([np.frombuffer(binario, tipo, n, comeco + k * passo) for k in range(quantos)])
    return np.frombuffer(binario, tipo, quantos * n, comeco).reshape(quantos, n).copy()


def matriz_do_no(no):
    if "matrix" in no:
        return np.array(no["matrix"], float).reshape(4, 4).T
    x, y, z, w = no.get("rotation", [0, 0, 0, 1])
    r = np.array([[1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)],
                  [2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)],
                  [2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)]])
    m = np.eye(4)
    m[:3, :3] = r @ np.diag(no.get("scale", [1, 1, 1]))
    m[:3, 3] = no.get("translation", [0, 0, 0])
    return m


def malhas_do_glb(caminho):
    """As malhas na ordem da raylib: uma por primitiva de triângulos, com a matriz do nó (e dos pais) aplicada"""
    js, binario = ler_glb(caminho)
    nos = js.get("nodes", [])
    pai = {f: i for i, no in enumerate(nos) for f in no.get("children", [])}

    def no_no_mundo(i):
        m = matriz_do_no(nos[i])
        return no_no_mundo(pai[i]) @ m if i in pai else m

    malhas = []
    for i, no in enumerate(nos):
        if "mesh" not in no:
            continue
        m = no_no_mundo(i)
        for p in js["meshes"][no["mesh"]]["primitives"]:
            if p.get("mode", 4) != 4:
                continue
            pos = acessor(js, binario, p["attributes"]["POSITION"]).astype(float)
            pos = (m[:3, :3] @ pos.T).T + m[:3, 3]
            idx = acessor(js, binario, p["indices"]).reshape(-1).astype(np.int64) if "indices" in p else np.arange(len(pos))
            malhas.append((pos, idx.reshape(-1, 3)))
    return malhas


def ilhas(pos, tris):
    """O número da ilha de cada vértice: soldando os do mesmo lugar e ligando os de cada triângulo"""
    pai = np.arange(len(pos))

    def raiz(a):
        r = a
        while pai[r] != r:
            r = pai[r]
        while pai[a] != r:
            pai[a], a = r, pai[a]
        return r

    ordem = np.lexsort((pos[:, 2], pos[:, 1], pos[:, 0]))
    iguais = np.all(pos[ordem][1:] == pos[ordem][:-1], axis=1)
    for k in np.nonzero(iguais)[0]:
        a, b = raiz(ordem[k]), raiz(ordem[k + 1])
        if a != b:
            pai[b] = a
    for t in tris:
        a, b, c = raiz(t[0]), raiz(t[1]), raiz(t[2])
        if b != a:
            pai[b] = a
        if c != a:
            pai[c] = a
    raizes = np.array([raiz(i) for i in range(len(pos))])
    return np.unique(raizes, return_inverse=True)[1].reshape(-1)


def mandibula(pos, tris, j, ilha):
    """Os vértices da mandíbula numa malha (veja o passo 3 lá em cima)"""
    xc, ys, zl, xk, xr, zb, yc = j["xc"], j["ys"], j["zl"], j["xk"], j["xr"], j["zb"], j["yc"]
    unicos, no = np.unique(pos, axis=0, return_inverse=True)
    no = no.reshape(-1)
    x, y, z = unicos[:, 0], unicos[:, 1], unicos[:, 2]
    em_r = (np.abs(x - xc) <= xr) & (z >= zb) & (y >= yc)
    labios = (z > zl) & (np.abs(x - xc) < xk)
    de_cima = (~labios) & (y > ys)
    arestas = np.concatenate([tris[:, [0, 1]], tris[:, [1, 2]], tris[:, [2, 0]]])
    a, b = no[arestas[:, 0]], no[arestas[:, 1]]
    pode = ~(de_cima[a] ^ de_cima[b]) & em_r[a] & em_r[b]
    vizinhos = [[] for _ in range(len(unicos))]
    for i, k in zip(a[pode].tolist(), b[pode].tolist()):
        vizinhos[i].append(k)
        vizinhos[k].append(i)
    if j.get("seeds"):
        fora = np.zeros(len(unicos), bool)
        if j.get("islands"):
            fora[no[np.isin(ilha, j["islands"])]] = True   # a semente não cai num dente nem na língua
        sementes = []
        for ponto in j["seeds"]:
            d = np.linalg.norm(unicos - np.array(ponto), axis=1)
            d[~em_r | fora] = 1e18
            sementes.append(int(np.argmin(d)))
    else:   # o ponto mais à frente, perto do meio, logo abaixo da fenda
        ht, largura = j.get("Ht", 0.1), j.get("thw", max(xk, 1e-6))
        candidatos = np.nonzero(em_r & (np.abs(x - xc) < 0.2 * largura) & (y < ys - 0.2 * ht) & (y > ys - 0.6 * ht) & (z > zl))[0]
        sementes = [int(candidatos[np.argmax(z[candidatos])])]
    visto = np.zeros(len(unicos), bool)
    visto[sementes] = True
    fila = deque(sementes)
    while fila:
        i = fila.popleft()
        for k in vizinhos[i]:
            if not visto[k]:
                visto[k] = True
                fila.append(k)
    dela = visto[no]
    if j.get("islands"):
        dela |= np.isin(ilha, j["islands"])
    return dela


def suave(a, b, x):
    t = np.clip((x - a) / (b - a), 0.0, 1.0)
    return t * t * (3 - 2 * t)


def main():
    linhas = ["# A mandíbula de cada morador (ferramentas/medir_bocas.py): a dobradiça (y, z), o quanto ela abre (graus)",
              "# e os pontos dela, no referencial do .glb como a raylib o carrega: x y z peso. Veja carros3d.c."]
    for nome, boca in BOCAS.items():
        malhas = malhas_do_glb(boca.get("arquivo", os.path.join(PASTA, nome + ".glb")))
        pesos = {}
        for j in boca["flood"]:
            pos, tris = malhas[j["mesh"]]
            ilha = ilhas(pos, tris)
            dela = mandibula(pos, tris, j, ilha)
            w = boca["w"]
            meio = boca["flood"][0]["xc"]   # o meio da boca (há modelo um pouco fora do x = 0)
            peso = (1 - suave(w["x_in"], w["x_out"], np.abs(pos[:, 0] - meio))) * suave(w["y_bot0"], w["y_bot1"], pos[:, 1])
            for malha, numero, forca in w.get("force", []):
                if malha == j["mesh"]:
                    peso[ilha == numero] = forca
            peso *= dela
            for p, q in zip(pos[peso > 1e-3], peso[peso > 1e-3]):
                chave = (round(p[0], 7), round(p[1], 7), round(p[2], 7))
                pesos[chave] = max(pesos.get(chave, 0.0), float(q))
        linhas.append(f"boca {nome} {boca['hinge'][0]:.5f} {boca['hinge'][1]:.5f} {boca['graus']} {len(pesos)}")
        linhas += [f"{x:.7f} {y:.7f} {z:.7f} {q:.3f}" for (x, y, z), q in sorted(pesos.items())]
        print(f"BOCA: {nome}: {len(pesos)} pontos na mandíbula, abre até {boca['graus']} graus")
    with open(SAIDA, "w") as arquivo:
        arquivo.write("\n".join(linhas) + "\n")
    print("BOCA: gravado em", SAIDA)


if __name__ == "__main__":
    main()
