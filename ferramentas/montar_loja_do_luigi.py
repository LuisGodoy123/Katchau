"""
montar_loja_do_luigi.py
------------------------------------------------------------------
Monta o lado de dentro da Casa Della Tires, a loja de pneus do Luigi e do
Guido (do filme Carros, 2006), no lugar do prédio dela no mapa do mundo
aberto (recursos/modelos/radiator_springs.glb), já nas coordenadas do mundo
do jogo (x, y para cima, z):

    Blender -b --factory-startup -P ferramentas/montar_loja_do_luigi.py -- \\
        recursos/modelos/radiator_springs.glb recursos/modelos/loja_do_luigi \\
        [--previas pasta] [--amostras 512] [--posteres atlas.png] [--noite]

Grava, com o nome dado no segundo argumento:
  loja_do_luigi.glb          o desenho, com a luz assada nas cores dos vértices
                             (COLOR_0): o jogo o desenha com o shader padrão da
                             raylib (textura × cor do vértice), sem luz nenhuma;
  loja_do_luigi_colisao.glb  a colisão: o piso e, em faces em pé, as paredes e os
                             móveis (sem teto, tampos nem prateleiras, que viram
                             chão para o carro); a porta de garagem aberta;
  loja_do_luigi.json         a porta (centro e direção), os lugares do Luigi e do
                             Guido e as aberturas que se tiram da casca do prédio
                             no mapa (o painel pintado da porta e os vidros).

O mapa (primeiro argumento) serve para conferir o chão na porta, para o reflexo
do espelho e para as prévias de fora; sem ele, valem as medidas guardadas
abaixo (e o espelho mostra céu no lugar da rua). Com --previas, a
ferramenta grava na pasta as vistas de dentro e uma de fora, pela porta aberta,
com a casca do prédio (sem o painel da porta e sem os vidros), mostrando o que o
jogo vai mostrar: as cores assadas × as texturas. --posteres guarda (ou, se o
arquivo já existe, reaproveita) o atlas dos pôsteres, que é o passo mais
demorado. --amostras é o número de amostras do Cycles por ponto na luz assada.

O que ela faz:
  1. Desenha as texturas (em PNG, porque a raylib do jogo não lê JPEG): o piso
     de tábuas longas e estreitas vermelho-castanhas, o asfalto da baia de
     serviço, a raiz de madeira dos painéis do móvel (redemoinhos suaves e nós)
     e a borracha dos pneus (flanco, banda de rodagem, faixa branca).
  2. Pinta os pôsteres de corridas italianas antigas: cada um é uma cena
     pequena renderizada no Blender com um dos carros do jogo
     (recursos/modelos/carros), pintado "de cartaz" (sombras em degraus e
     contorno), na frente de raios de sol, montanhas, arcadas ou faixas de
     velocidade, com o título em italiano nas fontes do jogo (nos carros
     repintados de vermelho, a íris dos olhos fica com a cor de verdade). Mais
     umas fotos em preto e branco. Tudo num atlas de 2048 x 2048: 32 retratos,
     5 paisagens e 8 fotos.
  3. Monta a loja: paredes rosadas uns centímetros para dentro da casca, com
     as aberturas no lugar das da casca (a porta de garagem diagonal, as duas
     janelas da frente e os dois arcos, que descem até o chão e viram
     vitrines com plataforma), o rebaixo entre as paredes e a casca fechado,
     caixilhos, rodapé, sanca e vigas cor de vinho, colunas redondas com base
     e capitel, o teto com luzes embutidas, o piso de tábuas e a baia de
     asfalto com as faixas; o móvel do fundo com as prateleiras de pneus, as
     plaquinhas e o nicho de cortinas vermelhas com a placa "Fettuccini
     Alfredo" e as três pilhas de pneus; o espelho, o elevador, a máquina de
     balancear, os pneus-banco, o pneu-troféu e as paredes forradas de pôsteres
     (colunas de quadros quase encostados, do rodapé alto até a sanca).
  4. Assa a luz no Cycles (difusa direta e indireta, sem a cor) nas cores
     dos vértices: o sol e o céu entram pelas aberturas, os focos quentes do
     teto (os perto das paredes desenham leques de luz), luzinhas escondidas
     nos compartimentos das prateleiras, a sanca âmbar e o brilho quente do
     fundo do nicho. As paredes (cortadas em níveis globais, sem junções em T)
     ficam numa grade de 0,3 m, o piso de 0,25 m; a luz é alisada entre
     vértices vizinhos de mesma normal, normalizada (o percentil 98 fica em
     0,95) e passada para a escala de tela; a cor sem textura vai junto na
     cor do vértice. O piso encerado ganha um reflexo falso: para cada vértice,
     o raio refletido de um olho médio é lançado na sala e a cor final do
     ponto atingido (pôsteres, janelas, móvel, nicho, luzes) se soma à luz.
     O espelho recebe um reflexo pré-renderizado da sala com a casca do mapa
     (pela porta e pelo arco se veem o pátio e a rua).
  5. Exporta: poucos materiais (um sem textura e um por textura), cada malha
     com menos de 65 mil vértices (a raylib usa índices de 16 bits), e
     confere no .glb que todas as primitivas têm COLOR_0.

Com --noite, a ferramenta faz a luz de NOITE (o jogo mistura a de dia e a de
noite vértice a vértice, conforme a hora) e grava só:
  loja_do_luigi_noite.glb    o .glb de dia (que já tem de existir) copiado byte a
                             byte, com outro COLOR_0: as mesmas malhas, primitivas,
                             vértices, posições, coordenadas de textura, índices e
                             texturas. A montagem é a mesma; a luz assada é sem sol,
                             com um luar fraco azul-arroxeado pelas aberturas e as
                             luzes de dentro todas acesas e mais fortes (os focos, as
                             prateleiras, a sanca, o nicho), com mais contraste; o
                             reflexo do piso mostra a noite lá fora; o espelho fica
                             com a textura do dia, mais escuro e quente. O assado de
                             noite é exportado à parte e as cores dele passam para os
                             vértices do .glb de dia (casando os triângulos), e a
                             ferramenta confere que só o COLOR_0 mudou;
  loja_do_luigi_noite.json   as luzes pontuais que a loja joga para fora de noite
                             (eixos do jogo; lugar, cor, raio e força).
O .glb de dia, a colisão e o .json ficam como estão. Sem --posteres, o atlas dos
pôsteres vem do .glb de dia. Com --previas, as vistas saem com o dia e a noite
lado a lado (os dois .glb como o jogo os carrega; de noite, a casca do mapa com a
luz da meia-noite do jogo e as luzes do _noite.json).

Eixos: no Blender, x = x do jogo, y = -z do jogo e z = y do jogo; o .glb sai
com +Y para cima, ou seja, nas coordenadas do jogo.
------------------------------------------------------------------
"""

import bpy
import bmesh
import json
import math
import os
import shutil
import struct
import sys
import tempfile
import time
import numpy as np
from mathutils import Matrix, Vector

INICIO = time.time()
bpy.ops.wm.read_factory_settings(use_empty=True)     # sem o cubo, a câmera e a luz da cena inicial
AQUI = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(AQUI)
PASTA_DAS_FONTES = os.path.join(REPO, 'recursos', 'fontes')
PASTA_DOS_CARROS = os.path.join(REPO, 'recursos', 'modelos', 'carros')

# ------------------------------------------------------------------
# Argumentos
# ------------------------------------------------------------------
argv = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
OPCOES_COM_VALOR = ('--previas', '--amostras', '--posteres')
opcoes, posicionais = {}, []
NOITE = '--noite' in argv          # a luz de noite: grava <saida>_noite.glb e <saida>_noite.json (veja 9.)
argv = [a for a in argv if a != '--noite']
i = 0
while i < len(argv):
    if argv[i] in OPCOES_COM_VALOR and i + 1 < len(argv):
        opcoes[argv[i]] = argv[i + 1]
        i += 2
    else:
        posicionais.append(argv[i])
        i += 1
ARQUIVO_DO_MAPA = posicionais[0] if posicionais else os.path.join(REPO, 'recursos', 'modelos', 'radiator_springs.glb')
SAIDA = posicionais[1] if len(posicionais) > 1 else os.path.join(REPO, 'recursos', 'modelos', 'loja_do_luigi')
if SAIDA.endswith('.glb'):
    SAIDA = SAIDA[:-4]
PREVIAS = opcoes.get('--previas')
AMOSTRAS = int(opcoes.get('--amostras', 512))
ATLAS_GUARDADO = opcoes.get('--posteres')
if NOITE and not os.path.exists(SAIDA + '.glb'):
    raise SystemExit('ERRO: --noite precisa do .glb de dia (%s.glb): a noite usa a geometria dele' % SAIDA)

# ------------------------------------------------------------------
# O prédio no mapa (medido em radiator_springs.glb, objeto ror_Object_46; eixos do jogo)
# ------------------------------------------------------------------
PISO_Y = 1.92            # o chão de dentro (a laje do lote, no mapa, está em 1,91: o pátio emenda sem degrau)
ALTURA = 6.5             # do piso ao teto (o telhado da casca está em 9,21)
TETO_Y = PISO_Y + ALTURA
SANCA = 0.55             # a sanca (e as vigas) descem isso do teto
# As faces de dentro das nossas paredes: uns centímetros para dentro da casca, passando por dentro
# dos peitoris fundos das janelas (1 m) e dos pilares da casca, que entram até 0,8 m no prédio
X_OESTE, Z_FRENTE, X_ARCO, Z_ARCO, X_LESTE, Z_FUNDO = 455.80, 147.65, 469.70, 168.25, 486.85, 177.05
# As faces de fora da casca, onde terminam os rebaixos das aberturas
Z_FRENTE_FORA, X_ARCO_FORA, Z_ARCO_FORA = 146.45, 470.70, 167.18
# A fachada diagonal (a da porta de garagem), na face de fora
DIAG_A = np.array([470.66, 157.79])
DIAG_B = np.array([478.39, 167.25])
RECUO_DA_DIAGONAL = 0.80          # a nossa parede diagonal, para dentro da face de fora
PORTA_T = (1.85, 10.37)           # o vão da porta, em metros ao longo da fachada a partir de DIAG_A
PORTA_TOPO = 6.93                 # o alto do vão (o painel pintado, 0,6 m para dentro, sobe em arco até 7,58)
FOLGA_DO_VAO = 0.03               # o nosso rebaixo fica isso para dentro do vão da casca (cobre o dela)
# Os arcos e as janelas da casca (as pontas do contorno de cada uma, em x ou z e y)
ARCO_DAS_JANELAS = [(0.00, 6.23), (0.25, 6.54), (0.67, 6.86), (1.37, 7.03), (2.07, 7.07),
                    (2.75, 7.03), (3.43, 6.86), (3.85, 6.54), (4.09, 6.23)]   # x a partir do começo
JANELAS_DA_FRENTE = (456.78, 464.64)     # o começo (x) de cada janela; o peitoril da casca fica em y 2,80
PEITORIL_DAS_JANELAS = 2.80
ARCO_OESTE = [(148.95, 6.28), (149.26, 6.64), (149.75, 7.01), (150.52, 7.32), (151.30, 7.48),
              (152.85, 7.48), (153.62, 7.32), (154.40, 7.01), (154.80, 6.64), (155.02, 6.28)]   # z, y
ARCO_SUL = [(480.54, 6.30), (480.80, 6.67), (481.20, 7.03), (481.84, 7.35), (482.48, 7.51),
            (483.77, 7.51), (484.42, 7.35), (485.06, 7.03), (485.39, 6.67), (485.58, 6.30)]     # x, y
# Os planos dos vidros pintados da casca (os caixilhos novos vão no mesmo lugar)
VIDRO_DA_FRENTE_Z = 146.80
VIDRO_OESTE_X = 469.99
VIDRO_SUL_Z = 167.63
# As peças da casca que se tiram do mapa (caixas que contêm cada uma inteira; eixos do jogo)
# (nome, descrição, caixa mínima, caixa máxima, faces da peça, o vão na face de fora: (x, z) das pontas,
#  y de baixo e y do alto)
ABERTURAS_DA_CASCA = [
    ('painel_da_porta', 'o painel de porta de garagem pintado, 0,6 m para dentro da fachada diagonal',
     (471.37, 1.80, 159.60), (476.96, 7.65, 166.42), 10, ((471.83, 159.21), (477.22, 165.79)), 1.87, 6.93),
    ('vidro_janela_frente_1', 'o vidro pintado da janela da frente de oeste (parede z = 146,45)',
     (456.72, 2.73, 146.75), (460.94, 7.14, 146.87), 12, ((456.78, 146.45), (460.87, 146.45)), 2.80, 7.07),
    ('vidro_janela_frente_2', 'o vidro pintado da janela da frente de leste (parede z = 146,45)',
     (464.57, 2.73, 146.74), (468.79, 7.14, 146.84), 12, ((464.64, 146.45), (468.72, 146.45)), 2.80, 7.07),
    ('vidro_arco_oeste', 'o vidro pintado do arco da parede x = 470,7 (vira vitrine)',
     (469.92, 1.70, 148.88), (470.06, 7.54, 155.11), 10, ((470.70, 148.95), (470.70, 155.02)), 1.85, 7.48),
    ('vidro_arco_sul', 'o vidro pintado do arco da parede z = 167,18 (vira vitrine)',
     (480.54, 1.75, 167.58), (485.75, 7.58, 167.68), 10, ((480.54, 167.18), (485.58, 167.18)), 1.85, 7.51),
]

# ------------------------------------------------------------------
# O arranjo (eixos do jogo)
# ------------------------------------------------------------------
PASSO = 0.42                 # a grade das superfícies grandes, para a luz assada ter detalhe
PASSO_DA_PAREDE = 0.30       # nas paredes (os "leques" das luzes do teto pedem uma grade mais fina)
PASSO_DO_PISO = 0.25         # no piso (o reflexo falso do encerado é calculado em cada vértice)
PASSO_DO_TETO = 0.60         # no teto (a luz nele é suave: o rebatido e a sanca)
LUGAR_DO_LUIGI = (467.90, 173.40, 3.0, 1.7)    # x, z, comprimento, largura
LUGAR_DO_GUIDO = (459.10, 173.60, 2.4, 1.2)
MOVEL_X = (457.00, 470.00)   # o móvel do fundo (nicho no meio), encostado na parede z = Z_FUNDO
ESPELHO_X = (471.00, 477.40)
BAIA_FUNDO = 8.8             # a baia de asfalto entra isso a partir da face de fora da fachada
ELEVADOR_TS = (3.55, 5.30)   # o elevador, em (ao longo da fachada, para dentro) a partir de DIAG_A
MAQUINA_U = 8.80             # a máquina de balancear, na parede do arco de oeste, ao lado da coluna da porta
CENTRO_DO_SALAO = (461.70, 152.40)   # o pódio de pneus no meio do salão da frente
# Pneus-banco (x, z)
BANCOS = [(466.10, 151.70), (467.30, 153.90), (465.70, 155.70), (481.10, 172.70), (482.80, 174.50)]

# ------------------------------------------------------------------
# Cores (na tela, 0 a 1)
# ------------------------------------------------------------------
PAREDE = (0.93, 0.82, 0.82)
TETO = (0.90, 0.83, 0.78)
VINHO = (0.40, 0.15, 0.16)             # bordô amarronzado, de madeira envernizada (no filme: #421d20 a #220407)
VINHO_ESCURO = (0.26, 0.085, 0.095)
VINHO_FUNDO = (0.14, 0.040, 0.050)
VINHO_BRILHO = (0.72, 0.38, 0.37)      # o realce do verniz nas colunas
CREME = (0.95, 0.89, 0.74)
VERMELHO = (0.74, 0.09, 0.09)
VERMELHO_CORTINA = (0.64, 0.05, 0.07)
DOURADO = (0.80, 0.62, 0.27)
METAL = (0.55, 0.56, 0.58)
METAL_ESCURO = (0.20, 0.21, 0.23)
CAIXILHO = (0.16, 0.24, 0.18)
BEGE = (0.85, 0.77, 0.60)
BRANCO = (1.0, 1.0, 1.0)
FAIXA_AMARELA = (0.93, 0.72, 0.12)
FAIXA_BRANCA = (0.93, 0.93, 0.90)
LUZ_DA_LAMPADA = (1.0, 0.97, 0.90)     # a cor do vértice das lâmpadas (brilham sozinhas)


def linear(c):
    return tuple(((x + 0.055) / 1.055) ** 2.4 if x > 0.04045 else x / 12.92 for x in c[:3])


def tela(c):
    c = np.clip(np.asarray(c, float), 0.0, None)
    return np.where(c > 0.0031308, 1.055 * np.power(c, 1 / 2.4) - 0.055, 12.92 * c)


def unit(v):
    v = np.asarray(v, float)
    return v / np.linalg.norm(v)


DIAG_D = unit(DIAG_B - DIAG_A)                 # ao longo da fachada
DIAG_N = np.array([-DIAG_D[1], DIAG_D[0]])     # para dentro da loja
DIAG_FORA = -DIAG_N                            # a normal da fachada (rumo 0,885)


def diag(t, s):
    """Ponto (x, z) a t metros ao longo da fachada diagonal e s metros para dentro da face de fora"""
    return DIAG_A + t * DIAG_D + s * DIAG_N


def em_ts(x, z):
    r = np.array([x, z]) - DIAG_A
    return float(r @ DIAG_D), float(r @ DIAG_N)


# As pontas da nossa parede diagonal (onde ela encontra x = X_ARCO e z = Z_ARCO)
T_CANTO_A = (X_ARCO - DIAG_A[0] - RECUO_DA_DIAGONAL * DIAG_N[0]) / DIAG_D[0]
T_CANTO_B = (Z_ARCO - DIAG_A[1] - RECUO_DA_DIAGONAL * DIAG_N[1]) / DIAG_D[1]
CANTO_A = diag(T_CANTO_A, RECUO_DA_DIAGONAL)
CANTO_B = diag(T_CANTO_B, RECUO_DA_DIAGONAL)
CONTORNO = [(X_OESTE, Z_FRENTE), (X_ARCO, Z_FRENTE), tuple(CANTO_A), tuple(CANTO_B),
            (X_LESTE, Z_ARCO), (X_LESTE, Z_FUNDO), (X_OESTE, Z_FUNDO)]


def dentro_do_contorno(x, z, contorno=CONTORNO):
    c = False
    n = len(contorno)
    for i in range(n):
        x1, z1 = contorno[i]
        x2, z2 = contorno[(i + 1) % n]
        if (z1 > z) != (z2 > z) and x < x1 + (z - z1) * (x2 - x1) / (z2 - z1):
            c = not c
    return c


def rumo_para(de, para):
    """O rumo do jogo (0 olhando para -z, crescendo para a direita: a frente é (sin r, -cos r))"""
    return math.atan2(para[0] - de[0], -(para[1] - de[1]))


print('Canto A (%.3f, %.3f), canto B (%.3f, %.3f)' % (*CANTO_A, *CANTO_B))

# ------------------------------------------------------------------
# Imagens
# ------------------------------------------------------------------
PASTA_TEMP = tempfile.mkdtemp(prefix='loja_do_luigi_')


def imagem(nome, arr):
    """arr: altura x largura x 3 ou 4 (0 a 1, na tela), a linha 0 em cima. Vira PNG embutido."""
    arr = np.clip(np.asarray(arr, np.float32), 0, 1)
    h, w = arr.shape[:2]
    if arr.shape[2] == 3:
        arr = np.concatenate([arr, np.ones((h, w, 1), np.float32)], axis=2)
    img = bpy.data.images.new(nome, w, h, alpha=False)
    img.pixels.foreach_set(np.flipud(arr).ravel())
    caminho = os.path.join(PASTA_TEMP, nome + '.png')
    img.filepath_raw = caminho
    img.file_format = 'PNG'
    img.save()
    bpy.data.images.remove(img)
    img = bpy.data.images.load(caminho)
    img.name = nome
    img.pack()
    return img


def salvar_png(arr, caminho):
    arr = np.clip(np.asarray(arr, np.float32), 0, 1)
    h, w = arr.shape[:2]
    if arr.shape[2] == 3:
        arr = np.concatenate([arr, np.ones((h, w, 1), np.float32)], axis=2)
    img = bpy.data.images.new('salvar', w, h, alpha=False)
    img.pixels.foreach_set(np.flipud(arr).ravel())
    img.filepath_raw = caminho
    img.file_format = 'PNG'
    img.save()
    bpy.data.images.remove(img)


def ler_png(caminho):
    img = bpy.data.images.load(caminho)
    w, h = img.size
    a = np.empty(w * h * 4, np.float32)
    img.pixels.foreach_get(a)
    bpy.data.images.remove(img)
    return np.flipud(a.reshape(h, w, 4))


def ruido(h, w, sx, sy, rng):
    """Ruído que se repete nas bordas (para ladrilhar), borrado sx, sy pixels; média 0, desvio 1"""
    r = rng.standard_normal((h, w))
    fy = np.fft.fftfreq(h)[:, None]
    fx = np.fft.fftfreq(w)[None, :]
    f = np.exp(-2 * np.pi ** 2 * ((fx * sx) ** 2 + (fy * sy) ** 2))
    r = np.real(np.fft.ifft2(np.fft.fft2(r) * f))
    return (r - r.mean()) / (r.std() + 1e-9)


def amostrar(img, x, y):
    """Lê img (h x w) em posições fracionárias, dando a volta nas bordas"""
    h, w = img.shape
    x0 = np.floor(x).astype(int)
    y0 = np.floor(y).astype(int)
    fx, fy = x - x0, y - y0
    x0 %= w
    y0 %= h
    x1, y1 = (x0 + 1) % w, (y0 + 1) % h
    return ((img[y0, x0] * (1 - fx) + img[y0, x1] * fx) * (1 - fy)
            + (img[y1, x0] * (1 - fx) + img[y1, x1] * fx) * fy)


# ------------------------------------------------------------------
# 1. Texturas
# ------------------------------------------------------------------
TAM_DO_PISO = 1024
METROS_DO_PISO = 4.0           # a textura do piso cobre 4 x 4 m: 30 tábuas de 13 cm, de 1,7 a 2,3 m
METROS_DO_ASFALTO = 2.5
METROS_DA_RAIZ = 2.8           # um painel de raiz do móvel (3,9 m) mostra pouco mais de uma repetição
COR_DO_PISO = (0.55, 0.18, 0.145)   # vermelho-castanho (carmim escuro), na tela


def textura_do_piso():
    """Tábuas longas e estreitas de madeira vermelho-castanha, com juntas finas e veio longo e sutil.
    Cada tábua varia só ±8% no valor (sem mudar o matiz); a textura se repete nas bordas."""
    rng = np.random.default_rng(11)
    n = TAM_DO_PISO
    tabuas = 30
    largura = n / tabuas
    yy, xx = np.mgrid[0:n, 0:n].astype(np.float32)
    coluna = np.minimum((xx // largura).astype(int), tabuas - 1)
    tom = np.ones((n, n), np.float32)
    junta = np.zeros((n, n), bool)
    desloc = np.zeros((n, n), np.float32)
    for c in range(tabuas):
        # duas juntas de topo por coluna (tábuas de 1,7 a 2,3 m numa volta de 4 m)
        y0 = rng.uniform(0, n)
        juntas = np.sort(np.array([y0 % n, (y0 + rng.uniform(0.42, 0.58) * n) % n]))
        tons = rng.uniform(0.92, 1.08, len(juntas))
        desl = rng.uniform(0, n, len(juntas))
        m = coluna == c
        ys = yy[m]
        k = (np.searchsorted(juntas, ys, side='right') - 1) % len(juntas)
        tom[m] = tons[k]
        desloc[m] = desl[k]
        d = np.min(np.abs(ys[:, None] - juntas[None, :]), axis=1)
        d = np.minimum(d, n - d)
        junta[m] = d < 0.9
    fresta = (xx % largura) < 1.0
    veio1 = ruido(n, n, 0.7, 60, rng)      # veio fino e longo, ao comprido da tábua
    veio2 = ruido(n, n, 4, 160, rng)
    mancha = ruido(n, n, 40, 40, rng)
    v1 = amostrar(veio1, xx + desloc, yy)
    v2 = amostrar(veio2, xx + desloc * 0.5, yy)
    cor = np.array(COR_DO_PISO, np.float32)[None, None, :]
    cor = cor * (tom * (1 + 0.035 * v1 + 0.03 * v2 + 0.015 * mancha))[..., None]
    cor = np.broadcast_to(cor, (n, n, 3)).copy()
    cor[fresta | junta] *= 0.62
    return np.clip(cor, 0, 1)


def textura_do_asfalto():
    rng = np.random.default_rng(5)
    n = 512
    largo = ruido(n, n, 40, 40, rng)
    medio = ruido(n, n, 6, 6, rng)
    fino = ruido(n, n, 0.7, 0.7, rng)
    v = 0.25 + 0.025 * largo + 0.015 * medio + 0.03 * fino
    v = v + np.where(fino > 1.7, 0.09, 0) - np.where(fino < -1.9, 0.06, 0)
    cor = np.stack([v * 1.0, v * 0.985, v * 0.96], axis=2)
    return np.clip(cor, 0, 1)


def textura_da_raiz():
    """Raiz de madeira castanho-avermelhada escura: redemoinhos grandes e suaves (ruído deformado por
    outro ruído), um veio fino por cima e uns nós escuros; pouco contraste (~30%)"""
    rng = np.random.default_rng(3)
    n = 512
    yy, xx = np.mgrid[0:n, 0:n].astype(np.float32)
    w1 = ruido(n, n, 45, 45, rng)
    w2 = ruido(n, n, 45, 45, rng)
    base = ruido(n, n, 30, 30, rng)
    v = amostrar(base, xx + 70 * w1, yy + 70 * w2)
    w3 = ruido(n, n, 12, 12, rng)
    v = v + 0.25 * amostrar(ruido(n, n, 9, 9, rng), xx + 25 * w3, yy - 25 * w3)
    figura = 0.5 + 0.5 * np.sin(v * 4.2)
    fino = ruido(n, n, 0.8, 4.0, rng)
    t = 0.22 + 0.52 * figura + 0.06 * fino
    # os nós: manchas escuras com um anel um pouco mais claro em volta (dando a volta nas bordas)
    for _ in range(8):
        cx, cy = rng.uniform(0, n, 2)
        r = rng.uniform(5, 11)
        dx = (xx - cx + n / 2) % n - n / 2
        dy = (yy - cy + n / 2) % n - n / 2
        d = np.sqrt(dx ** 2 + (dy * rng.uniform(0.8, 1.25)) ** 2) / r
        t = t * (1 - 0.55 * np.exp(-d ** 2 * 1.5)) + 0.10 * np.exp(-(d - 1.6) ** 2 * 2.0)
    escuro = np.array([0.30, 0.09, 0.07], np.float32)
    claro = np.array([0.55, 0.22, 0.14], np.float32)
    t = np.clip(t, 0, 1)[..., None]
    cor = escuro * (1 - t) + claro * t
    return np.clip(cor, 0, 1)


# A textura dos pneus: quatro faixas de 128 linhas (v); u dá a volta no pneu
FAIXAS_DO_PNEU = {'flanco': 0, 'banda': 1, 'branco': 2, 'banda_clara': 3}


def textura_dos_pneus():
    rng = np.random.default_rng(9)
    w, h = 512, 512
    cor = np.zeros((h, w, 3), np.float32)
    fino = ruido(h, w, 0.8, 0.8, rng)
    # flanco preto (um preto de borracha, não o preto puro: a luz assada ainda desenha o pneu)
    cor[0:128] = (0.105 + 0.008 * fino[0:128])[..., None]
    # banda de rodagem: três sulcos em zigue-zague ao longo da volta e cortes de lado
    xx, yy = np.mgrid[0:128, 0:w][1].astype(np.float32), np.mgrid[0:128, 0:w][0].astype(np.float32)
    periodo = 32.0
    zz = np.abs(((xx / periodo) % 1.0) * 2 - 1)            # triângulo de 0 a 1
    sulco = np.zeros((128, w), bool)
    for frac in (0.27, 0.50, 0.73):
        meio = 8 + frac * 112 + (zz - 0.5) * 9
        sulco |= np.abs(yy - meio) < 2.6
    corte = (np.abs(((xx + yy * 0.55) % 16.0) - 8) < 1.1) & (yy > 14) & (yy < 114)
    ombro = (yy < 12) | (yy > 116)
    banda = np.full((128, w), 0.15, np.float32)
    banda[corte] = 0.08
    banda[sulco] = 0.04
    banda[ombro] = 0.12
    banda += 0.01 * fino[128:256]
    cor[128:256] = banda[..., None]
    # faixa branca
    cor[256:384] = (np.array([0.93, 0.92, 0.88]) + 0.012 * fino[256:384, :, None])
    # banda clara (para os pneus bege: a cor do vértice tinge)
    clara = np.full((128, w), 0.90, np.float32)
    clara[corte] = 0.74
    clara[sulco] = 0.52
    clara[ombro] = 0.84
    clara += 0.015 * fino[384:512]
    cor[384:512] = clara[..., None]
    return np.clip(cor, 0, 1)


def v_da_faixa(nome, frac=0.5):
    """A coordenada v (do Blender, para cima) de um ponto da faixa (frac 0 = alto da faixa)"""
    k = FAIXAS_DO_PNEU[nome]
    linha = k * 128 + 8 + frac * 112
    return 1.0 - linha / 512.0


print('Desenhando as texturas...')
TEXTURAS = {'piso': textura_do_piso(), 'asfalto': textura_do_asfalto(), 'raiz': textura_da_raiz(), 'pneus': textura_dos_pneus()}
IMG_PISO = imagem('loja_piso', TEXTURAS['piso'])
IMG_ASFALTO = imagem('loja_asfalto', TEXTURAS['asfalto'])
IMG_RAIZ = imagem('loja_raiz', TEXTURAS['raiz'])
IMG_PNEU = imagem('loja_pneus', TEXTURAS['pneus'])
print('  %.0f s' % (time.time() - INICIO))

# ------------------------------------------------------------------
# 2. Pôsteres
# ------------------------------------------------------------------
FONTE_TITULO = os.path.join(PASTA_DAS_FONTES, 'racing_sans_one.ttf')
FONTE_TEXTO = os.path.join(PASTA_DAS_FONTES, 'saira_condensed_bold.ttf')
FONTE_FINA = os.path.join(PASTA_DAS_FONTES, 'saira_condensed_semibold.ttf')

PAPEL = (0.94, 0.89, 0.76)
P_VERMELHO = (0.80, 0.13, 0.10)
P_AZUL = (0.12, 0.20, 0.38)
P_CEU = (0.42, 0.62, 0.78)
P_AMARELO = (0.93, 0.70, 0.20)
P_VERDE = (0.16, 0.44, 0.33)
P_PRETO = (0.08, 0.07, 0.07)
P_LARANJA = (0.91, 0.47, 0.17)
P_CINZA = (0.55, 0.55, 0.52)

# Os carros nos pôsteres: (arquivo, giro de matiz para pintar de vermelho, saturação, brilho).
# O Doc só aparece de frente: no lado dele há o nome de uma marca de verdade.
CARROS_DOS_POSTERES = {
    'mcqueen': ('mcqueen', 0.0, 1.0, 1.0),
    'francesco': ('francesco', 0.0, 1.0, 1.0),
    'luigi': ('luigi', 0.0, 1.0, 1.0),
    'doc': ('doc_hudson', 0.35, 1.3, 2.6),
    'sally': ('sally', 0.375, 2.4, 0.95),
    'king': ('the_king', 0.425, 1.3, 0.95),
    'guido': ('guido', 0.0, 1.0, 1.0),
    'ruivo': ('ruivo', 0.0, 1.0, 1.0),
    'boost': ('boost', 0.13, 2.2, 1.3),
}

# Cada pôster: o carro, a vista (giro, inclinação da câmera, rolagem), onde fica (s, t do pôster, de
# 0 a 1, e a distância), o fundo e os textos (texto, fonte, cor, s, t, largura e altura máximas)
POSTERES = [
    dict(carro='mcqueen', giro=35, alto=8, rola=-6, pos=(0.50, 0.40, 13), fundo='raios', cores=(PAPEL, P_AMARELO, P_VERMELHO),
         textos=[('GRAN PREMIO', FONTE_TITULO, P_VERMELHO, 0.5, 0.88, 0.86, 0.11), ('DI PORTO CORSA', FONTE_TEXTO, P_PRETO, 0.5, 0.78, 0.7, 0.06),
                 ('1957', FONTE_TITULO, P_PRETO, 0.5, 0.09, 0.3, 0.07)]),
    dict(carro='francesco', giro=-30, alto=10, rola=5, pos=(0.5, 0.42, 14), fundo='noite', cores=(P_AZUL, P_AMARELO, P_PRETO),
         textos=[('24 ORE', FONTE_TITULO, P_AMARELO, 0.5, 0.86, 0.6, 0.13), ('DI SANTA RUOTINA', FONTE_TEXTO, PAPEL, 0.5, 0.75, 0.75, 0.055),
                 ('CORSA DI DURATA', FONTE_FINA, P_AMARELO, 0.5, 0.08, 0.6, 0.04)]),
    dict(carro='king', giro=55, alto=14, rola=-4, pos=(0.52, 0.36, 15), fundo='montanhas', cores=(P_CEU, P_VERDE, PAPEL),
         textos=[('CORSA IN SALITA', FONTE_TITULO, P_VERMELHO, 0.5, 0.90, 0.88, 0.09), ('MONTE ROTELLA', FONTE_TEXTO, P_AZUL, 0.5, 0.81, 0.6, 0.05),
                 ('22 GIUGNO 1958', FONTE_FINA, PAPEL, 0.5, 0.07, 0.6, 0.04)]),
    dict(carro='luigi', giro=25, alto=6, rola=0, pos=(0.5, 0.36, 16), fundo='arcadas', cores=(P_AMARELO, P_LARANJA, PAPEL),
         textos=[('CASA DELLA TIRES', FONTE_TITULO, P_VERMELHO, 0.5, 0.89, 0.88, 0.09), ('LUIGI', FONTE_TITULO, P_VERDE, 0.5, 0.79, 0.5, 0.08),
                 ('RADIATOR SPRINGS', FONTE_TEXTO, P_PRETO, 0.5, 0.07, 0.75, 0.05)]),
    dict(carro='sally', giro=-50, alto=9, rola=6, pos=(0.5, 0.38, 14), fundo='deserto', cores=(P_LARANJA, P_AMARELO, P_VERMELHO),
         textos=[('TARGA DEL DESERTO', FONTE_TITULO, PAPEL, 0.5, 0.90, 0.9, 0.08), ('ORNAMENT VALLEY', FONTE_TEXTO, P_PRETO, 0.5, 0.81, 0.6, 0.05),
                 ('1956', FONTE_TITULO, PAPEL, 0.5, 0.08, 0.3, 0.07)]),
    dict(carro='king', giro=40, alto=5, rola=-8, pos=(0.48, 0.42, 16), fundo='velocidade', cores=(PAPEL, P_VERMELHO, P_AZUL),
         textos=[('GRAN PREMIO', FONTE_TITULO, P_AZUL, 0.5, 0.89, 0.86, 0.1), ("D'ITALIA", FONTE_TITULO, P_VERMELHO, 0.5, 0.79, 0.6, 0.08),
                 ('AUTODROMO DI CARSOLI', FONTE_FINA, P_PRETO, 0.5, 0.07, 0.7, 0.04)]),
    dict(carro='mcqueen', giro=-90, alto=4, rola=0, pos=(0.5, 0.45, 15), fundo='velocidade', cores=(P_AMARELO, P_VERMELHO, P_PRETO),
         textos=[('PISTON CUP', FONTE_TITULO, P_VERMELHO, 0.5, 0.88, 0.86, 0.11), ('LIGHTNING McQUEEN', FONTE_TEXTO, P_PRETO, 0.5, 0.77, 0.75, 0.055),
                 ('95', FONTE_TITULO, P_VERMELHO, 0.5, 0.11, 0.25, 0.1)]),
    dict(carro='francesco', giro=150, alto=12, rola=-5, pos=(0.5, 0.42, 13), fundo='mar', cores=(P_CEU, P_AZUL, PAPEL),
         textos=[('CORSA DELLA RIVIERA', FONTE_TITULO, P_VERMELHO, 0.5, 0.90, 0.9, 0.08), ('PORTO CORSA', FONTE_TEXTO, P_AZUL, 0.5, 0.81, 0.5, 0.05),
                 ('1959', FONTE_TITULO, PAPEL, 0.5, 0.08, 0.3, 0.07)]),
    dict(carro='doc', giro=6, alto=18, rola=0, pos=(0.5, 0.40, 13), fundo='raios', cores=(P_VERMELHO, P_LARANJA, P_AMARELO),
         textos=[('COPPA D\'ORO', FONTE_TITULO, PAPEL, 0.5, 0.88, 0.8, 0.11), ('DOC HUDSON', FONTE_TEXTO, P_PRETO, 0.5, 0.78, 0.55, 0.055),
                 ('PISTON CUP 1951', FONTE_FINA, PAPEL, 0.5, 0.07, 0.6, 0.045)]),
    dict(carro='luigi', giro=-35, alto=10, rola=4, pos=(0.5, 0.36, 15), fundo='pneus', cores=(P_VERDE, PAPEL, P_VERMELHO),
         textos=[('PNEUMATICI', FONTE_TITULO, PAPEL, 0.5, 0.89, 0.86, 0.1), ('FETTUCCINI', FONTE_TITULO, P_VERMELHO, 0.5, 0.79, 0.8, 0.09),
                 ('IL PNEU DEI CAMPIONI', FONTE_TEXTO, PAPEL, 0.5, 0.07, 0.75, 0.045)]),
    dict(carro='sally', giro=60, alto=7, rola=-6, pos=(0.5, 0.40, 14), fundo='montanhas', cores=(P_AMARELO, P_LARANJA, P_VERMELHO),
         textos=[('MILLE CURVE', FONTE_TITULO, P_AZUL, 0.5, 0.89, 0.86, 0.1), ('PASSO DEL CARBURATORE', FONTE_TEXTO, P_PRETO, 0.5, 0.80, 0.75, 0.045),
                 ('1960', FONTE_TITULO, P_AZUL, 0.5, 0.08, 0.3, 0.07)]),
    dict(carro='king', giro=-25, alto=15, rola=5, pos=(0.5, 0.40, 15), fundo='xadrez', cores=(PAPEL, P_PRETO, P_VERMELHO),
         textos=[('TROFEO TOPOLINO', FONTE_TITULO, P_VERMELHO, 0.5, 0.89, 0.88, 0.09), ('SANTA RUOTINA', FONTE_TEXTO, P_PRETO, 0.5, 0.80, 0.6, 0.05),
                 ('1955', FONTE_TITULO, P_PRETO, 0.5, 0.08, 0.3, 0.07)]),
    dict(carro='mcqueen', giro=10, alto=20, rola=0, pos=(0.5, 0.37, 12), fundo='sol', cores=(P_CEU, P_AMARELO, P_LARANJA),
         textos=[('RUST-EZE', FONTE_TITULO, P_VERMELHO, 0.5, 0.89, 0.8, 0.11), ('VELOCITA E STILE', FONTE_TEXTO, P_AZUL, 0.5, 0.79, 0.7, 0.05),
                 ('KA-CHOW!', FONTE_TITULO, PAPEL, 0.5, 0.08, 0.45, 0.07)]),
    dict(carro='boost', giro=45, alto=6, rola=-7, pos=(0.5, 0.40, 13), fundo='velocidade', cores=(P_PRETO, P_VERMELHO, P_AMARELO),
         textos=[('GARA NOTTURNA', FONTE_TITULO, P_AMARELO, 0.5, 0.89, 0.88, 0.09), ('CIRCUITO DI CARBURETOR COUNTY', FONTE_TEXTO, PAPEL, 0.5, 0.80, 0.85, 0.04),
                 ('ORE 21', FONTE_TITULO, P_VERMELHO, 0.5, 0.08, 0.3, 0.06)]),
    dict(carro='francesco', giro=30, alto=20, rola=0, pos=(0.5, 0.42, 14), fundo='arcadas', cores=(P_VERMELHO, P_AMARELO, PAPEL),
         textos=[('FORZA ROSSA', FONTE_TITULO, PAPEL, 0.5, 0.89, 0.86, 0.1), ('FRANCESCO BERNOULLI', FONTE_TEXTO, P_PRETO, 0.5, 0.79, 0.75, 0.05),
                 ('1961', FONTE_TITULO, PAPEL, 0.5, 0.08, 0.3, 0.07)]),
    dict(carro='doc', giro=-6, alto=6, rola=6, pos=(0.5, 0.40, 15), fundo='xadrez', cores=(P_AMARELO, P_PRETO, P_AZUL),
         textos=[('GRAN PREMIO', FONTE_TITULO, P_AZUL, 0.5, 0.89, 0.86, 0.1), ('DI RADIATOR SPRINGS', FONTE_TEXTO, P_PRETO, 0.5, 0.79, 0.75, 0.05),
                 ('ROUTE 66', FONTE_TITULO, P_VERMELHO, 0.5, 0.08, 0.4, 0.06)]),
    dict(carro='ruivo', giro=35, alto=8, rola=0, pos=(0.5, 0.40, 13), fundo='raios', cores=(P_CEU, PAPEL, P_VERMELHO),
         textos=[('FESTA DEL PNEU', FONTE_TITULO, P_VERMELHO, 0.5, 0.89, 0.86, 0.1), ('RADIATOR SPRINGS', FONTE_TEXTO, P_AZUL, 0.5, 0.79, 0.6, 0.05),
                 ('INGRESSO LIBERO', FONTE_FINA, P_AZUL, 0.5, 0.07, 0.6, 0.045)]),
    dict(carro='sally', giro=25, alto=12, rola=-4, pos=(0.5, 0.40, 13), fundo='sol', cores=(P_LARANJA, P_AMARELO, P_VERDE),
         textos=[('COPPA DELLE ALPI', FONTE_TITULO, P_VERDE, 0.5, 0.89, 0.88, 0.09), ('CORSA IN MONTAGNA', FONTE_TEXTO, P_PRETO, 0.5, 0.80, 0.6, 0.05),
                 ('1958', FONTE_TITULO, P_VERDE, 0.5, 0.08, 0.3, 0.07)]),
    dict(carro='francesco', giro=-60, alto=5, rola=4, pos=(0.5, 0.42, 15), fundo='velocidade', cores=(P_VERMELHO, PAPEL, P_PRETO),
         textos=[('COPPA PORTO CORSA', FONTE_TITULO, PAPEL, 0.5, 0.89, 0.9, 0.09), ('CIRCUITO CITTADINO', FONTE_TEXTO, P_PRETO, 0.5, 0.80, 0.6, 0.05),
                 ('1962', FONTE_TITULO, PAPEL, 0.5, 0.08, 0.3, 0.07)]),
    dict(carro='mcqueen', giro=-25, alto=9, rola=5, pos=(0.5, 0.40, 13), fundo='noite', cores=(P_AZUL, P_AMARELO, P_PRETO),
         textos=[('NOTTE DI NEON', FONTE_TITULO, P_AMARELO, 0.5, 0.88, 0.86, 0.1), ('RADIATOR SPRINGS', FONTE_TEXTO, PAPEL, 0.5, 0.79, 0.6, 0.05),
                 ('ROUTE 66', FONTE_TITULO, P_AMARELO, 0.5, 0.08, 0.4, 0.06)]),
    dict(carro='luigi', giro=40, alto=12, rola=-4, pos=(0.5, 0.38, 14), fundo='raios', cores=(P_AMARELO, PAPEL, P_VERDE),
         textos=[('VIVA LUIGI!', FONTE_TITULO, P_VERMELHO, 0.5, 0.88, 0.8, 0.11), ('CASA DELLA TIRES', FONTE_TEXTO, P_VERDE, 0.5, 0.78, 0.6, 0.05),
                 ('DAL 1956', FONTE_TITULO, PAPEL, 0.5, 0.08, 0.4, 0.06)]),
    dict(carro='guido', giro=30, alto=8, rola=-5, pos=(0.5, 0.31, 15), fundo='velocidade', cores=(PAPEL, P_AZUL, P_VERMELHO),
         textos=[('PIT STOP', FONTE_TITULO, P_VERMELHO, 0.5, 0.88, 0.8, 0.12), ('GUIDO IL VELOCE', FONTE_TEXTO, P_AZUL, 0.5, 0.77, 0.6, 0.05),
                 ('QUATTRO SECONDI', FONTE_FINA, P_PRETO, 0.5, 0.07, 0.6, 0.045)]),
    dict(carro='king', giro=-45, alto=10, rola=4, pos=(0.5, 0.40, 15), fundo='sol', cores=(P_CEU, P_AMARELO, P_AZUL),
         textos=[('TROFEO DINOCO', FONTE_TITULO, P_AZUL, 0.5, 0.89, 0.86, 0.1), ('STRIP WEATHERS', FONTE_TEXTO, P_PRETO, 0.5, 0.79, 0.6, 0.05),
                 ('1970', FONTE_TITULO, PAPEL, 0.5, 0.08, 0.3, 0.07)]),
    dict(carro='doc', giro=0, alto=10, rola=-3, pos=(0.5, 0.40, 14), fundo='velocidade', cores=(P_PRETO, P_AMARELO, P_VERMELHO),
         textos=[('LA LEGGENDA', FONTE_TITULO, P_AMARELO, 0.5, 0.88, 0.86, 0.1), ('TRE VOLTE PISTON CUP', FONTE_TEXTO, PAPEL, 0.5, 0.79, 0.7, 0.05),
                 ('1951 1952 1953', FONTE_FINA, P_AMARELO, 0.5, 0.07, 0.6, 0.045)]),
    dict(carro='sally', giro=-35, alto=8, rola=3, pos=(0.5, 0.38, 14), fundo='mar', cores=(P_LARANJA, P_AZUL, PAPEL),
         textos=[('GIRO DEL TRAMONTO', FONTE_TITULO, P_AZUL, 0.5, 0.89, 0.9, 0.08), ('COZY CONE', FONTE_TEXTO, PAPEL, 0.5, 0.80, 0.5, 0.05),
                 ('1957', FONTE_TITULO, P_AZUL, 0.5, 0.08, 0.3, 0.07)]),
    dict(carro='ruivo', giro=-40, alto=10, rola=4, pos=(0.5, 0.40, 13), fundo='arcadas', cores=(P_AMARELO, P_VERMELHO, PAPEL),
         textos=[('FESTA DEI POMPIERI', FONTE_TITULO, P_VERMELHO, 0.5, 0.89, 0.9, 0.085), ('RADIATOR SPRINGS', FONTE_TEXTO, P_PRETO, 0.5, 0.80, 0.6, 0.05),
                 ('DOMENICA', FONTE_TITULO, P_VERMELHO, 0.5, 0.08, 0.4, 0.06)]),
    dict(carro='boost', giro=-30, alto=7, rola=6, pos=(0.5, 0.40, 13), fundo='noite', cores=(P_PRETO, P_LARANJA, P_VERMELHO),
         textos=[('CORSA DI MEZZANOTTE', FONTE_TITULO, P_LARANJA, 0.5, 0.89, 0.9, 0.08), ('INTERSTATE 40', FONTE_TEXTO, PAPEL, 0.5, 0.80, 0.5, 0.05),
                 ('1964', FONTE_TITULO, P_LARANJA, 0.5, 0.08, 0.3, 0.07)]),
    dict(carro='mcqueen', giro=55, alto=12, rola=-5, pos=(0.5, 0.38, 14), fundo='montanhas', cores=(P_AMARELO, P_VERDE, P_VERMELHO),
         textos=[("WILLY'S BUTTE", FONTE_TITULO, P_VERMELHO, 0.5, 0.89, 0.86, 0.1), ('CORSA SU STERRATO', FONTE_TEXTO, P_PRETO, 0.5, 0.80, 0.6, 0.05),
                 ('GIRA A SINISTRA', FONTE_FINA, PAPEL, 0.5, 0.07, 0.6, 0.045)]),
    dict(carro='francesco', giro=10, alto=16, rola=0, pos=(0.5, 0.40, 13), fundo='sol', cores=(P_LARANJA, P_AMARELO, P_VERMELHO),
         textos=[('GRAN PREMIO', FONTE_TITULO, P_VERMELHO, 0.5, 0.89, 0.86, 0.1), ('DEL TRAMONTO', FONTE_TEXTO, P_PRETO, 0.5, 0.79, 0.6, 0.055),
                 ('1963', FONTE_TITULO, PAPEL, 0.5, 0.08, 0.3, 0.07)]),
    dict(carro='luigi', giro=-20, alto=6, rola=0, pos=(0.5, 0.40, 15), fundo='xadrez', cores=(P_VERDE, PAPEL, P_VERMELHO),
         textos=[('TROFEO LUIGI', FONTE_TITULO, PAPEL, 0.5, 0.89, 0.86, 0.1), ('PNEUMATICI DI CLASSE', FONTE_TEXTO, P_AMARELO, 0.5, 0.79, 0.7, 0.05),
                 ('1959', FONTE_TITULO, PAPEL, 0.5, 0.08, 0.3, 0.07)]),
    dict(carro='king', giro=30, alto=8, rola=-4, pos=(0.5, 0.36, 15), fundo='pneus', cores=(P_VERMELHO, PAPEL, P_PRETO),
         textos=[('PNEUMATICI', FONTE_TITULO, PAPEL, 0.5, 0.89, 0.86, 0.1), ('LA SCELTA DEI CAMPIONI', FONTE_TEXTO, P_AMARELO, 0.5, 0.80, 0.75, 0.05),
                 ('FETTUCCINI', FONTE_TITULO, PAPEL, 0.5, 0.08, 0.5, 0.07)]),
    dict(carro='boost', giro=60, alto=10, rola=-3, pos=(0.52, 0.40, 14), fundo='xadrez', cores=(P_AMARELO, P_PRETO, P_AZUL),
         textos=[('CIRCUITO DEL RADIATORE', FONTE_TITULO, P_AZUL, 0.5, 0.89, 0.92, 0.075), ('CARBURETOR COUNTY', FONTE_TEXTO, P_PRETO, 0.5, 0.80, 0.6, 0.05),
                 ('1960', FONTE_TITULO, PAPEL, 0.5, 0.08, 0.3, 0.07)]),
    # paisagens
    dict(formato='paisagem', carro='mcqueen', giro=-80, alto=6, rola=0, pos=(0.62, 0.38, 15), fundo='velocidade', cores=(PAPEL, P_VERMELHO, P_AMARELO),
         textos=[('PNEUMATICI FETTUCCINI', FONTE_TITULO, P_VERMELHO, 0.5, 0.86, 0.9, 0.14), ('DA CORSA', FONTE_TEXTO, P_PRETO, 0.2, 0.18, 0.3, 0.09)]),
    dict(formato='paisagem', carro='luigi', giro=-60, alto=8, rola=0, pos=(0.25, 0.36, 15), fundo='liso', cores=(P_AMARELO, PAPEL, P_VERMELHO),
         textos=[('CASA DELLA TIRES', FONTE_TITULO, P_VERMELHO, 0.71, 0.72, 0.52, 0.13), ('SOLO PNEUMATICI', FONTE_TEXTO, P_PRETO, 0.71, 0.53, 0.44, 0.09),
                 ('PER LE MACCHINE CHE AMANO', FONTE_FINA, P_PRETO, 0.71, 0.39, 0.48, 0.065)]),
    dict(formato='paisagem', carro='doc', giro=-8, alto=5, rola=-3, pos=(0.55, 0.36, 15), fundo='deserto', cores=(P_AMARELO, P_LARANJA, P_VERMELHO),
         textos=[('IL FAVOLOSO', FONTE_TEXTO, P_AZUL, 0.5, 0.90, 0.4, 0.08), ('DOC HUDSON', FONTE_TITULO, P_AZUL, 0.5, 0.77, 0.6, 0.15)]),
    dict(formato='paisagem', carro='francesco', giro=70, alto=8, rola=4, pos=(0.45, 0.40, 15), fundo='mar', cores=(P_CEU, P_AZUL, PAPEL),
         textos=[('GRAN PREMIO DI PORTO CORSA', FONTE_TITULO, P_VERMELHO, 0.5, 0.88, 0.92, 0.12), ('RIVIERA ITALIANA', FONTE_TEXTO, PAPEL, 0.5, 0.10, 0.5, 0.08)]),
    dict(formato='paisagem', carro='king', giro=-75, alto=7, rola=0, pos=(0.40, 0.38, 15), fundo='velocidade', cores=(P_AZUL, PAPEL, P_VERMELHO),
         textos=[('PISTON CUP', FONTE_TITULO, P_VERMELHO, 0.5, 0.86, 0.6, 0.15), ('IL RE DELLA PISTA', FONTE_TEXTO, PAPEL, 0.78, 0.14, 0.4, 0.08)]),
]
FOTOS = [('mcqueen', 30, 8), ('doc', -6, 10), ('francesco', 20, 14), ('luigi', -25, 9),
         ('guido', 35, 10, 12.0), ('sally', -30, 7), ('king', 45, 12), ('ruivo', -35, 10)]
N_RETRATOS = sum(1 for p in POSTERES if p.get('formato') != 'paisagem')
N_PAISAGENS = len(POSTERES) - N_RETRATOS
assert N_RETRATOS <= 32 and N_PAISAGENS <= 5, 'o atlas tem lugar para 32 retratos e 5 paisagens'

# O atlas (2048 x 2048): 32 retratos (256 x 384) em 4 linhas de 8, 5 paisagens (384 x 256) e 8 fotos
# (256 x 172). Cada célula tem uma borda de 4 pixels repetidos (as mipmaps não puxam a vizinha).
TAM_DO_ATLAS = 2048
BORDA_DA_CELULA = 4


def celulas_do_atlas():
    cel = []
    for k in range(32):
        cel.append(('retrato', (k % 8) * 256, (k // 8) * 384, 256, 384))
    for k in range(5):
        cel.append(('paisagem', k * 384, 1536, 384, 256))
    for k in range(8):
        cel.append(('foto', k * 256, 1792, 256, 172))
    return cel


CELULAS = celulas_do_atlas()


def uv_da_celula(k):
    """(u0, v0, u1, v1) da parte útil da célula k, nas coordenadas do Blender (v para cima)"""
    _, x, y, w, h = CELULAS[k]
    b = BORDA_DA_CELULA + 1
    return ((x + b) / TAM_DO_ATLAS, 1 - (y + h - b) / TAM_DO_ATLAS,
            (x + w - b) / TAM_DO_ATLAS, 1 - (y + b) / TAM_DO_ATLAS)


def limpar_cena():
    for o in list(bpy.data.objects):
        bpy.data.objects.remove(o, do_unlink=True)
    for colecao in (bpy.data.meshes, bpy.data.curves, bpy.data.cameras, bpy.data.lights):
        for d in list(colecao):
            colecao.remove(d)
    for m in list(bpy.data.materials):
        bpy.data.materials.remove(m)
    guardar = {IMG_PISO.name, IMG_ASFALTO.name, IMG_RAIZ.name, IMG_PNEU.name, 'loja_posteres', 'loja_espelho'}
    for im in list(bpy.data.images):
        if im.name not in guardar:
            bpy.data.images.remove(im)


def novo_material(nome):
    m = bpy.data.materials.new(nome)
    try:
        m.use_nodes = True
    except Exception:
        pass
    return m


def novo_mundo(nome):
    w = bpy.data.worlds.new(nome)
    try:
        w.use_nodes = True
    except Exception:
        pass
    return w


def material_emissao(nome, fonte='objeto'):
    """Material que só brilha: com a cor do objeto (texto) ou com a cor dos vértices (fundos)"""
    m = novo_material(nome)
    nt = m.node_tree
    nt.nodes.clear()
    saida = nt.nodes.new('ShaderNodeOutputMaterial')
    em = nt.nodes.new('ShaderNodeEmission')
    if fonte == 'objeto':
        info = nt.nodes.new('ShaderNodeObjectInfo')
        nt.links.new(info.outputs['Color'], em.inputs['Color'])
    else:
        at = nt.nodes.new('ShaderNodeAttribute')
        at.attribute_name = 'Col'
        nt.links.new(at.outputs['Color'], em.inputs['Color'])
    nt.links.new(em.outputs[0], saida.inputs['Surface'])
    return m


def importar_carro(nome_do_arquivo):
    """Importa um carro do jogo, com uns 4,5 m de comprimento, de pé no chão (z = 0) e com a frente para -y"""
    antes = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=os.path.join(PASTA_DOS_CARROS, nome_do_arquivo + '.glb'))
    novos = [o for o in bpy.data.objects if o not in antes]
    bpy.context.view_layer.update()
    mn = Vector((1e9,) * 3)
    mx = Vector((-1e9,) * 3)
    for o in novos:
        if o.type == 'MESH':
            for c in o.bound_box:
                w = o.matrix_world @ Vector(c)
                mn = Vector(map(min, mn, w))
                mx = Vector(map(max, mx, w))
    esc = 4.5 / max(mx.x - mn.x, mx.y - mn.y, (mx.z - mn.z) * 1.3)   # o Guido é mais alto que comprido
    raiz = bpy.data.objects.new('carro_' + nome_do_arquivo, None)
    bpy.context.scene.collection.objects.link(raiz)
    meio = Vector(((mn.x + mx.x) / 2, (mn.y + mx.y) / 2, mn.z))
    for o in novos:
        if o.parent is None:
            o.parent = raiz
            o.location = (o.location - meio) * esc
            o.scale = o.scale * esc
    bpy.context.view_layer.update()
    return raiz, [o for o in novos if o.type == 'MESH'], esc


def cor_da_palpebra(o, img):
    """Se img é a textura dos olhos (o branco do olho com a íris em cima e a pálpebra, da cor do carro,
    embaixo), devolve a cor da pálpebra (linear); senão, None. O giro de matiz que pinta o carro de
    vermelho só pega o que tem essa cor: a íris fica com a cor de verdade."""
    if img is None or img.size[0] < 16 or img.size[1] < 16:
        return None
    w, h = img.size
    a = np.empty(w * h * 4, np.float32)
    img.pixels.foreach_get(a)
    a = a.reshape(h, w, 4)[..., :3]          # a linha 0 é a de baixo
    baixo = np.median(a[: max(1, h // 16)].reshape(-1, 3), axis=0)
    cima = np.median(a[-max(1, h // 16):].reshape(-1, 3), axis=0)
    satura = float(baixo.max() - baixo.min())
    olho = 'eye' in o.name.lower() or (cima.min() > 0.8 and satura > 0.12)
    if not olho:
        return None
    return linear(tuple(baixo))


def pintar_de_cartaz(objetos, contorno, esc):
    """Troca os materiais do carro por um sombreado em degraus (de cartaz), com um giro de matiz,
    e põe um contorno preto (a casca invertida, com a face de trás escondida)"""
    hues = []
    feitos = {}
    for o in objetos:
        for k, slot in enumerate(o.material_slots):
            m0 = slot.material
            if m0 is None:
                continue
            if m0.name in feitos:
                slot.material = feitos[m0.name]
                continue
            imagem_base, cor_base = None, (0.8, 0.8, 0.8, 1)
            for n in m0.node_tree.nodes:
                if n.type == 'BSDF_PRINCIPLED':
                    entrada = n.inputs['Base Color']
                    cor_base = tuple(entrada.default_value)
                    pilha = [l.from_node for l in entrada.links]
                    while pilha:
                        x = pilha.pop()
                        if x.type == 'TEX_IMAGE':
                            imagem_base = x.image
                            break
                        for e in x.inputs:
                            pilha += [l.from_node for l in e.links]
            m = novo_material(m0.name + '_cartaz')
            nt = m.node_tree
            nt.nodes.clear()
            saida = nt.nodes.new('ShaderNodeOutputMaterial')
            dif = nt.nodes.new('ShaderNodeBsdfDiffuse')
            s2r = nt.nodes.new('ShaderNodeShaderToRGB')
            rampa = nt.nodes.new('ShaderNodeValToRGB')
            rampa.color_ramp.interpolation = 'CONSTANT'
            els = rampa.color_ramp.elements
            els[0].position = 0.0
            els[0].color = (0.30, 0.27, 0.33, 1)
            els[1].position = 0.12
            els[1].color = (0.62, 0.60, 0.62, 1)
            e = els.new(0.42)
            e.color = (1.0, 0.98, 0.94, 1)
            e = els.new(0.85)
            e.color = (1.25, 1.22, 1.15, 1)
            hs = nt.nodes.new('ShaderNodeHueSaturation')
            hues.append(hs)
            mult = nt.nodes.new('ShaderNodeMix')
            mult.data_type = 'RGBA'
            mult.blend_type = 'MULTIPLY'
            mult.inputs['Factor'].default_value = 1.0
            em = nt.nodes.new('ShaderNodeEmission')
            nt.links.new(dif.outputs[0], s2r.inputs[0])
            nt.links.new(s2r.outputs['Color'], rampa.inputs['Fac'])
            pintado = hs.outputs['Color']
            if imagem_base is not None:
                tex = nt.nodes.new('ShaderNodeTexImage')
                tex.image = imagem_base
                nt.links.new(tex.outputs['Color'], hs.inputs['Color'])
                palpebra = cor_da_palpebra(o, imagem_base)
                if palpebra is not None:
                    # nos olhos, só a pálpebra (da cor do carro) gira de matiz; a íris fica como é
                    dist = nt.nodes.new('ShaderNodeVectorMath')
                    dist.operation = 'DISTANCE'
                    dist.inputs[1].default_value = palpebra
                    nt.links.new(tex.outputs['Color'], dist.inputs[0])
                    faixa = nt.nodes.new('ShaderNodeMapRange')
                    faixa.inputs['From Min'].default_value = 0.05
                    faixa.inputs['From Max'].default_value = 0.20
                    faixa.inputs['To Min'].default_value = 1.0
                    faixa.inputs['To Max'].default_value = 0.0
                    faixa.clamp = True
                    nt.links.new(dist.outputs['Value'], faixa.inputs['Value'])
                    escolha = nt.nodes.new('ShaderNodeMix')
                    escolha.data_type = 'RGBA'
                    nt.links.new(faixa.outputs['Result'], escolha.inputs['Factor'])
                    nt.links.new(tex.outputs['Color'], escolha.inputs['A'])
                    nt.links.new(hs.outputs['Color'], escolha.inputs['B'])
                    pintado = escolha.outputs['Result']
            else:
                hs.inputs['Color'].default_value = cor_base
            nt.links.new(pintado, mult.inputs['A'])
            nt.links.new(rampa.outputs['Color'], mult.inputs['B'])
            nt.links.new(mult.outputs['Result'], em.inputs['Color'])
            nt.links.new(em.outputs[0], saida.inputs['Surface'])
            feitos[m0.name] = m
            slot.material = m
        if not o.data.materials or o.data.materials[-1] != contorno:
            o.data.materials.append(contorno)
        mod = o.modifiers.new('contorno', 'SOLIDIFY')
        mod.thickness = 0.035 / max(o.matrix_world.to_scale())
        mod.offset = 1.0
        mod.use_flip_normals = True
        mod.material_offset = len(o.data.materials) - 1
        mod.use_rim = False
    return hues


def malha_plana(nome, pontos, cores, material, colecao):
    """Um polígono plano (para os fundos dos pôsteres), com uma cor por ponto"""
    me = bpy.data.meshes.new(nome)
    me.from_pydata([tuple(p) for p in pontos], [], [list(range(len(pontos)))])
    at = me.color_attributes.new('Col', 'FLOAT_COLOR', 'CORNER')
    for k, l in enumerate(me.loops):
        at.data[k].color = (*linear(cores[l.vertex_index]), 1)
    me.materials.append(material)
    o = bpy.data.objects.new(nome, me)
    colecao.objects.link(o)
    return o


def renderizar_posteres():
    """Pinta os pôsteres e as fotos e devolve o atlas (2048 x 2048 x 3, linha 0 em cima)"""
    sc = bpy.context.scene
    sc.render.engine = 'BLENDER_EEVEE'
    sc.view_settings.view_transform = 'Standard'
    sc.view_settings.look = 'None'
    sc.eevee.taa_render_samples = 24
    sc.render.film_transparent = False
    w = novo_mundo('ceu_do_poster')
    sc.world = w
    fundo_mundo = w.node_tree.nodes['Background']
    fundo_mundo.inputs[1].default_value = 0.7
    cam = bpy.data.cameras.new('cam_poster')
    cam.lens = 50
    cam.sensor_fit = 'VERTICAL'
    cam.sensor_height = 24
    cam.clip_start = 0.5
    cam.clip_end = 500
    co = bpy.data.objects.new('cam_poster', cam)
    sc.collection.objects.link(co)
    sc.camera = co
    co.location = (0, 0, 0)
    co.rotation_euler = (math.pi / 2, 0, 0)
    sol1 = bpy.data.objects.new('sol1', bpy.data.lights.new('sol1', 'SUN'))
    sol1.data.energy = 3.2
    sol1.data.color = (1.0, 0.95, 0.85)
    sol1.rotation_euler = (math.radians(55), math.radians(-25), math.radians(-35))
    sol2 = bpy.data.objects.new('sol2', bpy.data.lights.new('sol2', 'SUN'))
    sol2.data.energy = 1.4
    sol2.data.color = (0.85, 0.9, 1.0)
    sol2.rotation_euler = (math.radians(-60), math.radians(20), math.radians(150))
    for s in (sol1, sol2):
        sc.collection.objects.link(s)
    contorno = novo_material('contorno')
    contorno.use_backface_culling = True
    nt = contorno.node_tree
    nt.nodes.clear()
    em = nt.nodes.new('ShaderNodeEmission')
    em.inputs['Color'].default_value = (0.03, 0.025, 0.025, 1)
    nt.links.new(em.outputs[0], nt.nodes.new('ShaderNodeOutputMaterial').inputs['Surface'])
    mat_fundo = material_emissao('fundo_poster', 'vertices')
    mat_texto = material_emissao('texto_poster', 'objeto')
    fontes = {}

    carros = {}
    for chave, (arquivo, giro_de_matiz, sat, val) in CARROS_DOS_POSTERES.items():
        raiz, objs, esc = importar_carro(arquivo)
        hues = pintar_de_cartaz(objs, contorno, esc)
        for h in hues:
            h.inputs['Hue'].default_value = (0.5 + giro_de_matiz) % 1.0
            h.inputs['Saturation'].default_value = sat
            h.inputs['Value'].default_value = val
        carros[chave] = (raiz, objs)
        raiz.hide_render = True
        for o in objs:
            o.hide_render = True

    camada = bpy.data.collections.new('camada_do_poster')
    sc.collection.children.link(camada)
    rng = np.random.default_rng(21)
    atlas = np.zeros((TAM_DO_ATLAS, TAM_DO_ATLAS, 3), np.float32)
    papel_grande = None

    def limpar_camada():
        dados = {o.data for o in camada.objects if o.data is not None}
        for o in list(camada.objects):
            bpy.data.objects.remove(o, do_unlink=True)
        for d in dados:
            if d.users == 0:
                if isinstance(d, bpy.types.Mesh):
                    bpy.data.meshes.remove(d)
                elif isinstance(d, bpy.types.Curve):
                    bpy.data.curves.remove(d)

    def mostrar_carro(chave, giro, alto, rola, pos, metade_w, metade_h):
        for k, (raiz, objs) in carros.items():
            vis = k == chave
            raiz.hide_render = not vis
            for o in objs:
                o.hide_render = not vis
        raiz = carros[chave][0]
        s, t, d = pos
        # a câmera fica no alto do carro (inclinação "alto") e o carro gira "giro" graus
        x = (s - 0.5) * 2 * metade_w * d
        z = (t - 0.5) * 2 * metade_h * d
        raiz.location = (x, d, z - 0.6)
        raiz.rotation_euler = (math.radians(alto), math.radians(rola), math.radians(giro))

    def ponto(s, t, d, metade_w, metade_h):
        return ((s - 0.5) * 2 * metade_w * d, d, (t - 0.5) * 2 * metade_h * d)

    def poli(nome, pts_st, cores, d, mw, mh):
        malha_plana(nome, [ponto(s, t, d, mw, mh) for s, t in pts_st], cores, mat_fundo, camada)

    def retangulo(nome, s0, t0, s1, t1, cor_baixo, cor_cima, d, mw, mh):
        poli(nome, [(s0, t0), (s1, t0), (s1, t1), (s0, t1)], [cor_baixo, cor_baixo, cor_cima, cor_cima], d, mw, mh)

    def texto(txt, fonte, cor, s, t, larg, alt, d, mw, mh):
        if fonte not in fontes:
            fontes[fonte] = bpy.data.fonts.load(fonte, check_existing=True)
        cu = bpy.data.curves.new('txt', 'FONT')
        cu.body = txt
        cu.font = fontes[fonte]
        cu.align_x = 'CENTER'
        cu.align_y = 'CENTER'
        cu.size = 1.0
        o = bpy.data.objects.new('txt', cu)
        camada.objects.link(o)
        o.data.materials.append(mat_texto)
        o.color = (*linear(cor), 1)
        bpy.context.view_layer.update()
        dw, dh = max(o.dimensions.x, 1e-3), max(o.dimensions.y, 1e-3)
        k = min(larg * 2 * mw * d / dw, alt * 2 * mh * d / dh)
        o.scale = (k, k, k)
        o.rotation_euler = (math.pi / 2, 0, 0)
        o.location = ponto(s, t, d, mw, mh)
        # sombra deslocada (de cartaz)
        o2 = o.copy()
        camada.objects.link(o2)
        o2.color = (*linear(tuple(0.25 * c for c in cor) if sum(cor) > 1.2 else (0.95, 0.9, 0.8)), 1)
        o2.location = ponto(s + 0.006, t - 0.006, d + 0.05, mw, mh)

    def fundo(tipo, cores, mw, mh, paisagem):
        c0, c1, c2 = cores
        D = 80
        retangulo('papel', -0.05, -0.05, 1.05, 1.05, c0, c0, D, mw, mh)
        if tipo == 'liso':
            retangulo('faixa', -0.05, 0.16, 1.05, 0.20, c2, c2, D - 1, mw, mh)
            retangulo('chao', -0.05, -0.05, 1.05, 0.16, c1, c1, D - 2, mw, mh)
        elif tipo == 'raios':
            meio = (0.5, 0.45)
            n = 18
            for k in range(n):
                if k % 2:
                    continue
                a0 = 2 * math.pi * k / n
                a1 = 2 * math.pi * (k + 1) / n
                pts = [meio, (meio[0] + 2 * math.cos(a0), meio[1] + 2 * math.sin(a0) * (0.66 if not paisagem else 1.5)),
                       (meio[0] + 2 * math.cos(a1), meio[1] + 2 * math.sin(a1) * (0.66 if not paisagem else 1.5))]
                poli('raio', pts, [c1, c1, c1], D - 1, mw, mh)
            circ = [(meio[0] + 0.24 * math.cos(a), meio[1] + 0.24 * math.sin(a) * (0.66 if not paisagem else 1.5))
                    for a in np.linspace(0, 2 * math.pi, 40, endpoint=False)]
            poli('sol', circ, [c2] * 40, D - 2, mw, mh)
            retangulo('chao', -0.05, -0.05, 1.05, 0.2, c2, c2, D - 3, mw, mh)
        elif tipo == 'sol':
            retangulo('ceu', -0.05, 0.35, 1.05, 1.05, c1, c0, D - 1, mw, mh)
            circ = [(0.5 + 0.3 * math.cos(a), 0.5 + 0.3 * math.sin(a) * (0.66 if not paisagem else 1.5))
                    for a in np.linspace(0, 2 * math.pi, 48, endpoint=False)]
            poli('sol', circ, [c2] * 48, D - 2, mw, mh)
            poli('estrada', [(-0.1, -0.05), (1.1, -0.05), (0.56, 0.36), (0.44, 0.36)], [P_CINZA, P_CINZA, P_PRETO, P_PRETO], D - 3, mw, mh)
            retangulo('terra_e', -0.05, -0.05, 0.0, 0.36, c2, c2, D - 4, mw, mh)
            poli('terra_d', [(-0.05, -0.05), (-0.1, -0.05), (0.44, 0.36), (-0.05, 0.36)], [c2] * 4, D - 2.5, mw, mh)
            poli('terra_d2', [(1.1, -0.05), (1.05, -0.05), (1.05, 0.36), (0.56, 0.36)], [c2] * 4, D - 2.5, mw, mh)
        elif tipo == 'montanhas':
            retangulo('ceu', -0.05, 0.3, 1.05, 1.05, c0, PAPEL, D - 1, mw, mh)
            for camada_k, (alt, cor) in enumerate([(0.62, tuple(0.8 * x + 0.2 for x in c1)), (0.5, c1), (0.36, tuple(0.6 * x for x in c1))]):
                xs = np.linspace(-0.05, 1.05, 9)
                ys = alt + rng.uniform(-0.08, 0.1, 9) * (1 if camada_k < 2 else 0.4)
                ys[::2] -= 0.08
                pts = [(-0.05, -0.05)] + list(zip(xs, ys)) + [(1.05, -0.05)]
                poli('serra', pts, [cor] * len(pts), D - 1 - camada_k, mw, mh)
            poli('estrada', [(0.2, -0.05), (0.9, -0.05), (0.62, 0.38), (0.55, 0.38)], [P_CINZA, P_CINZA, P_CINZA, P_CINZA], D - 5, mw, mh)
        elif tipo == 'deserto':
            retangulo('ceu', -0.05, 0.3, 1.05, 1.05, c0, c1, D - 1, mw, mh)
            circ = [(0.25 + 0.12 * math.cos(a), 0.66 + 0.12 * math.sin(a) * (0.66 if not paisagem else 1.5))
                    for a in np.linspace(0, 2 * math.pi, 40, endpoint=False)]
            poli('sol', circ, [PAPEL] * 40, D - 2, mw, mh)
            for k, (x0, larg, alt) in enumerate([(0.05, 0.22, 0.55), (0.62, 0.3, 0.5), (0.35, 0.15, 0.44)]):
                pts = [(x0 - 0.04, 0.3), (x0 + larg + 0.04, 0.3), (x0 + larg, alt), (x0, alt)]
                poli('mesa', pts, [c2] * 4, D - 3 - k, mw, mh)
            retangulo('chao', -0.05, -0.05, 1.05, 0.32, tuple(0.8 * x for x in c2), c1, D - 6, mw, mh)
        elif tipo == 'velocidade':
            for k in range(7):
                y0 = 0.12 + k * 0.1 + rng.uniform(-0.02, 0.02)
                larg = rng.uniform(0.02, 0.05)
                cor = c1 if k % 2 == 0 else c2
                x0 = rng.uniform(-0.1, 0.3)
                poli('faixa', [(x0, y0), (1.1, y0 + 0.08), (1.1, y0 + 0.08 + larg), (x0, y0 + larg)], [cor] * 4, D - 1 - k * 0.1, mw, mh)
            retangulo('chao', -0.05, -0.05, 1.05, 0.18, c2, c2, D - 2, mw, mh)
        elif tipo == 'noite':
            retangulo('ceu', -0.05, -0.05, 1.05, 1.05, tuple(0.4 * x for x in c0), c0, D - 1, mw, mh)
            circ = [(0.72 + 0.1 * math.cos(a), 0.66 + 0.1 * math.sin(a) * (0.66 if not paisagem else 1.5))
                    for a in np.linspace(0, 2 * math.pi, 40, endpoint=False)]
            poli('lua', circ, [c1] * 40, D - 2, mw, mh)
            for k in range(26):
                x, y = rng.uniform(0, 1), rng.uniform(0.5, 1)
                r = 0.004
                poli('estrela', [(x - r, y), (x, y - r * 1.5), (x + r, y), (x, y + r * 1.5)], [PAPEL] * 4, D - 3, mw, mh)
            poli('farol', [(0.5, 0.40), (-0.2, 0.0), (0.2, -0.05)], [c1, tuple(0.3 * x for x in c0), tuple(0.3 * x for x in c0)], D - 4, mw, mh)
            retangulo('chao', -0.05, -0.05, 1.05, 0.25, c2, tuple(0.6 * x for x in c0), D - 5, mw, mh)
        elif tipo == 'mar':
            retangulo('ceu', -0.05, 0.4, 1.05, 1.05, c0, PAPEL, D - 1, mw, mh)
            retangulo('mar', -0.05, 0.25, 1.05, 0.42, c1, c1, D - 2, mw, mh)
            for k in range(5):
                x0 = 0.05 + k * 0.2
                poli('casa', [(x0, 0.4), (x0 + 0.12, 0.4), (x0 + 0.12, 0.5 + 0.03 * (k % 2)), (x0, 0.5 + 0.03 * (k % 2))],
                     [(P_AMARELO, P_LARANJA, PAPEL, P_VERMELHO, P_AMARELO)[k]] * 4, D - 3, mw, mh)
            retangulo('estrada', -0.05, -0.05, 1.05, 0.26, c2, c2, D - 4, mw, mh)
        elif tipo == 'arcadas':
            retangulo('muro', -0.05, 0.22, 1.05, 1.05, c1, c0, D - 1, mw, mh)
            n = 5 if not paisagem else 8
            for k in range(n):
                x0 = -0.05 + k * 1.1 / n + 0.03
                larg = 1.1 / n - 0.06
                topo = 0.62
                pts = [(x0, 0.22), (x0 + larg, 0.22), (x0 + larg, topo)]
                for a in np.linspace(0, math.pi, 10)[1:-1]:
                    pts.append((x0 + larg / 2 + larg / 2 * math.cos(a), topo + larg / 2 * math.sin(a) * (0.66 if not paisagem else 1.5)))
                pts.append((x0, topo))
                poli('arco', pts, [tuple(0.55 * x for x in c1)] * len(pts), D - 2, mw, mh)
            retangulo('chao', -0.05, -0.05, 1.05, 0.24, c2, c2, D - 3, mw, mh)
        elif tipo == 'xadrez':
            retangulo('faixa', -0.05, 0.62, 1.05, 0.72, c1, c1, D - 1, mw, mh)
            m = 14
            for k in range(m):
                for j in range(2):
                    if (k + j) % 2 == 0:
                        x0 = -0.05 + k * 1.1 / m
                        y0 = 0.62 + j * 0.05
                        retangulo('casa', x0, y0, x0 + 1.1 / m, y0 + 0.05, c0, c0, D - 2, mw, mh)
            retangulo('chao', -0.05, -0.05, 1.05, 0.22, c2, c2, D - 3, mw, mh)
        elif tipo == 'pneus':
            for k in range(7):
                cx, cy = rng.uniform(0, 1), rng.uniform(0.12, 0.62)
                r = rng.uniform(0.09, 0.16)
                ext = [(cx + r * math.cos(a), cy + r * math.sin(a) * (0.66 if not paisagem else 1.5)) for a in np.linspace(0, 2 * math.pi, 32, endpoint=False)]
                poli('pneu', ext, [P_PRETO] * 32, D - 1 - k * 0.2, mw, mh)
                r2 = r * 0.72
                bra = [(cx + r2 * math.cos(a), cy + r2 * math.sin(a) * (0.66 if not paisagem else 1.5)) for a in np.linspace(0, 2 * math.pi, 32, endpoint=False)]
                poli('branca', bra, [c1] * 32, D - 1.05 - k * 0.2, mw, mh)
                r3 = r * 0.5
                fur = [(cx + r3 * math.cos(a), cy + r3 * math.sin(a) * (0.66 if not paisagem else 1.5)) for a in np.linspace(0, 2 * math.pi, 32, endpoint=False)]
                poli('furo', fur, [c0] * 32, D - 1.1 - k * 0.2, mw, mh)
            retangulo('chao', -0.05, -0.05, 1.05, 0.2, c2, c2, D - 3, mw, mh)

    def render(w_px, h_px):
        sc.render.resolution_x = w_px
        sc.render.resolution_y = h_px
        sc.render.resolution_percentage = 100
        caminho = os.path.join(PASTA_TEMP, 'poster_render.png')
        sc.render.filepath = caminho
        sc.render.image_settings.file_format = 'PNG'
        bpy.ops.render.render(write_still=True)
        return ler_png(caminho)[..., :3]

    def papel(img, rng_local, envelhecer=0.10):
        h, w = img.shape[:2]
        grao = ruido(h, w, 1.6, 1.6, rng_local)       # grão suave (o PNG fica bem menor que com ruído fino)
        fibra = ruido(h, w, 1.0, 12, rng_local)
        img = img * (1 + 0.022 * grao[..., None] + 0.014 * fibra[..., None])
        img = img * (1 - envelhecer) + np.array(PAPEL) * envelhecer
        yy, xx = np.mgrid[0:h, 0:w]
        d = np.maximum(np.abs(xx / w - 0.5), np.abs(yy / h - 0.5)) * 2
        img = img * (1 - 0.12 * np.clip((d - 0.75) / 0.25, 0, 1) ** 2)[..., None]
        img = np.round(img * 14) / 14 * 0.25 + img * 0.75     # um pouco de "impressão" em poucas tintas
        return np.clip(img, 0, 1)

    def por_na_celula(k, img):
        _, x, y, w, h = CELULAS[k]
        b = BORDA_DA_CELULA
        alvo_w, alvo_h = w - 2 * b, h - 2 * b
        img = redimensionar(img, alvo_w, alvo_h)
        cel = np.pad(img, ((b, b), (b, b), (0, 0)), mode='edge')
        atlas[y:y + h, x:x + w] = cel

    def tamanho_do_render(k, fator):
        _, _, _, w, h = CELULAS[k]
        return int(round((w - 2 * BORDA_DA_CELULA) * fator)), int(round((h - 2 * BORDA_DA_CELULA) * fator))

    n_retrato, n_paisagem = 0, 0
    for k, p in enumerate(POSTERES):
        limpar_camada()
        paisagem = p.get('formato') == 'paisagem'
        if paisagem:
            celula = N_RETRATOS + n_paisagem
            n_paisagem += 1
        else:
            celula = n_retrato
            n_retrato += 1
        w_px, h_px = tamanho_do_render(celula, 2.7)
        cam.sensor_fit = 'VERTICAL'
        mh = 12 / cam.lens
        mw = mh * w_px / h_px
        fundo(p['fundo'], p['cores'], mw, mh, paisagem)
        for (txt, fonte, cor, s, t, larg, alt) in p['textos']:
            texto(txt, fonte, cor, s, t, larg, alt, 6, mw, mh)
        mostrar_carro(p['carro'], p['giro'], p['alto'], p['rola'], p['pos'], mw, mh)
        fundo_mundo.inputs[0].default_value = (*linear(p['cores'][0]), 1)
        img = render(w_px, h_px)
        img = papel(img, np.random.default_rng(100 + k))
        por_na_celula(celula, img)
        print('  pôster %d: %s' % (k + 1, p['textos'][0][0]))
    # As fotos em preto e branco
    for k, (chave, giro, alto, *dist) in enumerate(FOTOS):
        limpar_camada()
        w_px, h_px = tamanho_do_render(N_RETRATOS + N_PAISAGENS + k, 2.0)
        mh = 12 / cam.lens
        mw = mh * w_px / h_px
        retangulo('fundo', -0.05, -0.05, 1.05, 1.05, (0.35, 0.35, 0.35), (0.75, 0.75, 0.75), 80, mw, mh)
        retangulo('chao', -0.05, -0.05, 1.05, 0.3, (0.25, 0.25, 0.25), (0.45, 0.45, 0.45), 79, mw, mh)
        mostrar_carro(chave, giro, alto, 0, (0.5, 0.36 if dist else 0.40, dist[0] if dist else 8.0), mw, mh)
        fundo_mundo.inputs[0].default_value = (0.5, 0.5, 0.5, 1)
        img = render(w_px, h_px)
        cinza = img @ np.array([0.3, 0.59, 0.11])
        cinza = np.clip(cinza * 1.35 + 0.06, 0, 1)
        img = cinza[..., None] * np.array([1.0, 0.94, 0.84])
        img = papel(img, np.random.default_rng(300 + k), 0.06)
        img[:8, :] = img[-8:, :] = 0.93
        img[:, :8] = img[:, -8:] = 0.93
        por_na_celula(N_RETRATOS + N_PAISAGENS + k, img)
    limpar_camada()
    return np.round(atlas * 63) / 63        # 64 tons por canal: o PNG encolhe e não se vê diferença


def matriz_de_reamostrar(n_de, n_para):
    """Pesos (n_para x n_de) para levar n_de amostras a n_para: a média das áreas (com as frações das
    bordas) quando reduz e a interpolação linear quando amplia (nenhuma linha fica sem pixel)"""
    M = np.zeros((n_para, n_de))
    if n_para <= n_de:
        s = n_de / n_para
        j = np.arange(n_de)
        for i in range(n_para):
            a, b = i * s, (i + 1) * s
            M[i] = np.clip(np.minimum(b, j + 1) - np.maximum(a, j), 0, None) / s
    else:
        x = (np.arange(n_para) + 0.5) * n_de / n_para - 0.5
        x = np.clip(x, 0, n_de - 1)
        x0 = np.floor(x).astype(int)
        x1 = np.minimum(x0 + 1, n_de - 1)
        f = x - x0
        M[np.arange(n_para), x0] += 1 - f
        M[np.arange(n_para), x1] += f
    return M


def redimensionar(img, w, h):
    """Leva a imagem (altura x largura x canais) a w x h (sem PIL), um eixo de cada vez"""
    My = matriz_de_reamostrar(img.shape[0], h)
    Mx = matriz_de_reamostrar(img.shape[1], w)
    tmp = np.tensordot(My, np.asarray(img, np.float64), axes=(1, 0))      # h x W x c
    return np.transpose(np.tensordot(tmp, Mx, axes=(1, 1)), (0, 2, 1)).astype(np.float32)


def imagem_do_glb(caminho, nome):
    """A imagem 'nome' embutida num .glb (o PNG gravado de volta num arquivo da pasta temporária), ou None"""
    d = open(caminho, 'rb').read()
    n = struct.unpack('<I', d[12:16])[0]
    j = json.loads(d[20:20 + n])
    for im in j.get('images', []):
        if im.get('name') == nome and 'bufferView' in im:
            bv = j['bufferViews'][im['bufferView']]
            ini = 20 + n + 8 + bv.get('byteOffset', 0)
            arquivo = os.path.join(PASTA_TEMP, 'do_glb_' + nome + '.png')
            with open(arquivo, 'wb') as f:
                f.write(d[ini:ini + bv['byteLength']])
            return arquivo
    return None


print('Pintando os pôsteres...')
if NOITE and not ATLAS_GUARDADO:
    # de noite, o atlas é o do .glb de dia (a mesma textura; e poupa o passo mais demorado)
    ATLAS_GUARDADO = imagem_do_glb(SAIDA + '.glb', 'loja_posteres')
if ATLAS_GUARDADO and os.path.exists(ATLAS_GUARDADO):
    ATLAS = ler_png(ATLAS_GUARDADO)[..., :3]
    print('  reaproveitado de', ATLAS_GUARDADO)
else:
    ATLAS = renderizar_posteres()
    if ATLAS_GUARDADO:
        salvar_png(ATLAS, ATLAS_GUARDADO)
IMG_POSTERES = imagem('loja_posteres', ATLAS)
limpar_cena()
print('  %.0f s' % (time.time() - INICIO))

# ------------------------------------------------------------------
# 3. A loja
# ------------------------------------------------------------------
TAM_MAX_DE_CANTOS = 56000      # cantos por malha (a raylib usa índices de 16 bits: menos de 65536 vértices)


class Malha:
    """As faces de um material (em bmesh), com as coordenadas de textura, a cor sem luz (tinta, linear)
    e o brilho (as lâmpadas). Quando uma enche, começa outra."""

    def __init__(self, nome, imagem=None, brilhante=False):
        self.nome, self.imagem, self.brilhante = nome, imagem, brilhante
        self.bms = []
        self.novo()

    def novo(self):
        bm = bmesh.new()
        self.l_uv = bm.loops.layers.uv.new('UVMap')
        self.l_tinta = bm.loops.layers.float_color.new('tinta')
        self.l_brilho = bm.loops.layers.float_color.new('brilho')
        self.l_suave = bm.faces.layers.int.new('suave')
        self.bm = bm
        self.cantos = 0
        self.bms.append(bm)

    def face(self, pts, cor, n=None, uvs=None, brilho=None, suave=False):
        """suave: a face é de uma superfície grande e lisa (parede, teto, piso), em que a luz assada é
        alisada depois entre os vértices vizinhos de mesma normal"""
        pts = [tuple(map(float, p)) for p in pts]
        # tira pontos repetidos
        limpos, uvl, idx = [], [], []
        for k, p in enumerate(pts):
            if limpos and np.linalg.norm(np.subtract(p, limpos[-1])) < 1e-5:
                continue
            limpos.append(p)
            idx.append(k)
        if len(limpos) > 2 and np.linalg.norm(np.subtract(limpos[0], limpos[-1])) < 1e-5:
            limpos.pop()
            idx.pop()
        if len(limpos) < 3:
            return
        nn = normal_de(limpos)
        if np.linalg.norm(nn) < 1e-9:
            return
        if callable(cor):
            cores = [cor(p) for p in limpos]
        elif isinstance(cor, list):
            cores = [cor[k] for k in idx]
        else:
            cores = [cor] * len(limpos)
        uvl = [uvs[k] for k in idx] if uvs is not None else [(0.0, 0.0)] * len(limpos)
        if n is not None and np.dot(nn, n) < 0:
            limpos, cores, uvl = limpos[::-1], cores[::-1], uvl[::-1]
        if self.cantos + len(limpos) > TAM_MAX_DE_CANTOS:
            self.novo()
        vs = [self.bm.verts.new((p[0], -p[2], p[1])) for p in limpos]
        try:
            f = self.bm.faces.new(vs)
        except ValueError:
            return
        self.cantos += len(limpos)
        f[self.l_suave] = 1 if suave else 0
        b = (*linear(brilho), 1.0) if brilho is not None else (0.0, 0.0, 0.0, 1.0)
        for k, l in enumerate(f.loops):
            l[self.l_tinta] = (*linear(cores[k]), 1.0)
            l[self.l_uv].uv = uvl[k]
            l[self.l_brilho] = b


def normal_de(pts):
    n = np.zeros(3)
    m = len(pts)
    for k in range(m):
        a, b = np.array(pts[k]), np.array(pts[(k + 1) % m])
        n += np.cross(a, b)
    return n


MALHAS = {
    'cores': Malha('loja_cores'),
    'piso': Malha('loja_piso', IMG_PISO),
    'asfalto': Malha('loja_asfalto', IMG_ASFALTO),
    'posteres': Malha('loja_posteres', IMG_POSTERES),
    'raiz': Malha('loja_raiz', IMG_RAIZ),
    'pneus': Malha('loja_pneus', IMG_PNEU),
    'espelho': Malha('loja_espelho', None),
}
CORES = MALHAS['cores']
COLISAO = []      # triângulos da colisão (eixos do jogo)


def quad(m, a, b, c, d, cor, n=None, uv=None, uvf=None, passo=PASSO, brilho=None, div=None, suave=False):
    """Um quadrilátero (a, b, c, d em volta), subdividido numa grade de no máximo 'passo' metros.
    uv: as coordenadas de textura dos quatro cantos; uvf: uma função do ponto"""
    a, b, c, d = (np.asarray(x, float) for x in (a, b, c, d))
    if div:
        nu, nv = div
    else:
        nu = max(1, math.ceil(max(np.linalg.norm(b - a), np.linalg.norm(c - d)) / passo - 1e-6))
        nv = max(1, math.ceil(max(np.linalg.norm(d - a), np.linalg.norm(c - b)) / passo - 1e-6))

    def P(s, t):
        return (1 - t) * ((1 - s) * a + s * b) + t * ((1 - s) * d + s * c)

    def UV(s, t):
        if uv is None:
            return None
        u = [np.asarray(x, float) for x in uv]
        return tuple((1 - t) * ((1 - s) * u[0] + s * u[1]) + t * ((1 - s) * u[3] + s * u[2]))
    for i in range(nu):
        for j in range(nv):
            st = [(i / nu, j / nv), ((i + 1) / nu, j / nv), ((i + 1) / nu, (j + 1) / nv), (i / nu, (j + 1) / nv)]
            pts = [P(s, t) for s, t in st]
            if uv is not None:
                uvs = [UV(s, t) for s, t in st]
            elif uvf is not None:
                uvs = [uvf(p) for p in pts]
            else:
                uvs = None
            m.face(pts, cor, n=n, uvs=uvs, brilho=brilho, suave=suave)


class Ref:
    """Um referencial no chão: o (x, z), u ao longo, n para a frente (para dentro da sala); h a partir de y0"""

    def __init__(self, o, u, n, y0=PISO_Y):
        self.o = np.asarray(o, float)
        self.u = unit(u)
        self.n = unit(n)
        self.y0 = y0

    def p(self, a, h, b):
        xz = self.o + a * self.u + b * self.n
        return np.array([xz[0], self.y0 + h, xz[1]])

    def d(self, du, dh, dn):
        v = du * self.u + dn * self.n
        return np.array([v[0], dh, v[1]])


def caixa(m, R, a0, a1, h0, h1, b0, b1, cor, sem=(), passo=PASSO, uvf=None, cores=None, brilho=None):
    """Uma caixa no referencial R. 'sem' tira faces ('frente', 'tras', 'esq', 'dir', 'cima', 'baixo');
    'cores' dá uma cor por face"""
    P = lambda a, h, b: R.p(a, h, b)
    faces = {
        'frente': ([P(a0, h0, b1), P(a1, h0, b1), P(a1, h1, b1), P(a0, h1, b1)], R.d(0, 0, 1)),
        'tras': ([P(a0, h0, b0), P(a1, h0, b0), P(a1, h1, b0), P(a0, h1, b0)], R.d(0, 0, -1)),
        'esq': ([P(a0, h0, b0), P(a0, h0, b1), P(a0, h1, b1), P(a0, h1, b0)], R.d(-1, 0, 0)),
        'dir': ([P(a1, h0, b0), P(a1, h0, b1), P(a1, h1, b1), P(a1, h1, b0)], R.d(1, 0, 0)),
        'cima': ([P(a0, h1, b0), P(a1, h1, b0), P(a1, h1, b1), P(a0, h1, b1)], R.d(0, 1, 0)),
        'baixo': ([P(a0, h0, b0), P(a1, h0, b0), P(a1, h0, b1), P(a0, h0, b1)], R.d(0, -1, 0)),
    }
    for nome, (pts, n) in faces.items():
        if nome in sem:
            continue
        c = cores.get(nome, cor) if cores else cor
        quad(m, *pts, c, n=n, passo=passo, uvf=uvf, brilho=brilho)


def barra(m, p, q, largura, prof, lado, cor):
    """Uma barra de p a q (pontos do jogo), com 'largura' na direção 'lado' e 'prof' na outra"""
    p, q = np.asarray(p, float), np.asarray(q, float)
    eixo = unit(q - p)
    lado = unit(np.asarray(lado, float) - eixo * np.dot(lado, eixo))
    frente = np.cross(eixo, lado)
    hw, hp = largura / 2, prof / 2
    cantos = [(-1, -1), (1, -1), (1, 1), (-1, 1)]
    for k in range(4):
        s0, t0 = cantos[k]
        s1, t1 = cantos[(k + 1) % 4]
        o0 = lado * s0 * hw + frente * t0 * hp
        o1 = lado * s1 * hw + frente * t1 * hp
        n = unit(o0 + o1)
        quad(m, p + o0, p + o1, q + o1, q + o0, cor, n=n, div=(1, 1))


def torno(m, centro, eixo, e1, perfil, n_seg, cor, tipos=None, repete=4, ang=(0.0, 2 * math.pi), brilho=None, fechar=False):
    """Gira o perfil [(raio, altura ao longo do eixo)] em volta do eixo. tipos[k] = (v0, v1) dá as
    coordenadas v de textura do trecho k (u dá a volta, 'repete' vezes). A face fica para o lado em
    que o perfil, andando, tem a sua direita (perfil anti-horário em (r, h) => para fora)."""
    centro = np.asarray(centro, float)
    eixo = unit(eixo)
    e1 = unit(np.asarray(e1, float) - eixo * np.dot(e1, eixo))
    e2 = np.cross(eixo, e1)
    angs = np.linspace(ang[0], ang[1], n_seg + 1)
    m_pts = len(perfil)
    segs = range(m_pts if fechar else m_pts - 1)
    for k in segs:
        r0, h0 = perfil[k]
        r1, h1 = perfil[(k + 1) % m_pts]
        dr, dh = r1 - r0, h1 - h0
        nr, nh = dh, -dr          # a normal do trecho, no plano (r, h)
        v0, v1 = tipos[k] if tipos else (0.0, 0.0)
        for j in range(n_seg):
            a0, a1 = angs[j], angs[j + 1]
            rad0 = math.cos(a0) * e1 + math.sin(a0) * e2
            rad1 = math.cos(a1) * e1 + math.sin(a1) * e2
            radm = unit(rad0 + rad1)
            pts = [centro + eixo * h0 + rad0 * r0, centro + eixo * h0 + rad1 * r0,
                   centro + eixo * h1 + rad1 * r1, centro + eixo * h1 + rad0 * r1]
            n = radm * nr + eixo * nh
            u0 = a0 / (2 * math.pi) * repete
            u1 = a1 / (2 * math.pi) * repete
            uvs = [(u0, v0), (u1, v0), (u1, v1), (u0, v1)]
            m.face(pts, cor, n=n, uvs=uvs, brilho=brilho)


def disco(m, centro, eixo, e1, raio, n_seg, cor, brilho=None, raio_dentro=0.0):
    """Um disco (ou anel) virado para 'eixo'"""
    torno(m, centro, eixo, e1, [(raio, 0.0), (raio_dentro, 0.0)], n_seg, cor, brilho=brilho)


# --- textos (objetos de texto do Blender, convertidos em malha) ---
_FONTES = {}
_TEXTOS = {}


def malha_de_texto(txt, fonte, resolucao=1):
    chave = (txt, fonte, resolucao)
    if chave in _TEXTOS:
        return _TEXTOS[chave]
    if fonte not in _FONTES:
        _FONTES[fonte] = bpy.data.fonts.load(fonte, check_existing=True)
    cu = bpy.data.curves.new('texto', 'FONT')
    cu.body = txt
    cu.font = _FONTES[fonte]
    cu.size = 1.0
    cu.resolution_u = resolucao       # 1 nas plaquinhas (pequenas), 2 nas placas grandes
    cu.fill_mode = 'BOTH'
    ob = bpy.data.objects.new('texto', cu)
    bpy.context.scene.collection.objects.link(ob)
    dg = bpy.context.evaluated_depsgraph_get()
    me = bpy.data.meshes.new_from_object(ob.evaluated_get(dg))
    pts = np.array([v.co[:2] for v in me.vertices])
    faces = [list(p.vertices) for p in me.polygons]
    bpy.data.objects.remove(ob, do_unlink=True)
    bpy.data.curves.remove(cu)
    bpy.data.meshes.remove(me)
    _TEXTOS[chave] = (pts, faces)
    return pts, faces


def texto(m, txt, fonte, centro, direita, cima, largura, altura, cor, brilho=None, resolucao=1):
    """Escreve txt centrado em 'centro', cabendo em largura x altura (metros), no plano (direita, cima);
    a face fica virada para direita x cima"""
    pts, faces = malha_de_texto(txt, fonte, resolucao)
    if len(pts) == 0:
        return
    mn, mx = pts.min(0), pts.max(0)
    w, h = mx - mn
    k = min(largura / max(w, 1e-6), altura / max(h, 1e-6))
    meio = (mn + mx) / 2
    centro = np.asarray(centro, float)
    direita, cima = unit(direita), unit(cima)
    n = np.cross(direita, cima)
    mundo = [centro + (p[0] - meio[0]) * k * direita + (p[1] - meio[1]) * k * cima for p in pts]
    for f in faces:
        m.face([mundo[i] for i in f], cor, n=n, brilho=brilho)


# --- polígonos no plano do chão (x, z) ---
def area2(poly):
    a = 0.0
    for k in range(len(poly)):
        x1, z1 = poly[k]
        x2, z2 = poly[(k + 1) % len(poly)]
        a += x1 * z2 - x2 * z1
    return a / 2


def cortar_meio_plano(poly, a, n):
    """O que fica do polígono do lado (p - a)·n >= 0"""
    out = []
    m = len(poly)
    for k in range(m):
        p, q = np.asarray(poly[k]), np.asarray(poly[(k + 1) % m])
        dp, dq = np.dot(p - a, n), np.dot(q - a, n)
        if dp >= 0:
            out.append(p)
        if (dp >= 0) != (dq >= 0):
            t = dp / (dp - dq)
            out.append(p + t * (q - p))
    return out


def cortar_convexo(poly, conv):
    s = 1 if area2(conv) > 0 else -1
    for k in range(len(conv)):
        a, b = np.asarray(conv[k]), np.asarray(conv[(k + 1) % len(conv)])
        e = b - a
        n = np.array([-e[1], e[0]]) * s
        poly = cortar_meio_plano(poly, a, n)
        if len(poly) < 3:
            return []
    return poly


def grade_no_convexo(conv, passo):
    xs = [p[0] for p in conv]
    zs = [p[1] for p in conv]
    x0 = math.floor(min(xs) / passo) * passo
    z0 = math.floor(min(zs) / passo) * passo
    pecas = []
    x = x0
    while x < max(xs):
        z = z0
        while z < max(zs):
            cel = [np.array([x, z]), np.array([x + passo, z]), np.array([x + passo, z + passo]), np.array([x, z + passo])]
            c = cortar_convexo(cel, conv)
            if len(c) >= 3 and abs(area2(c)) > 1e-5:
                pecas.append(c)
            z += passo
        x += passo
    return pecas


def separar_por_convexo(pecas, conv):
    """Separa cada peça em (dentro de conv, fora de conv)"""
    s = 1 if area2(conv) > 0 else -1
    dentro, fora = [], []
    xs = [p[0] for p in conv]
    zs = [p[1] for p in conv]
    for pc in pecas:
        px = [p[0] for p in pc]
        pz = [p[1] for p in pc]
        if max(px) < min(xs) or min(px) > max(xs) or max(pz) < min(zs) or min(pz) > max(zs):
            fora.append(pc)
            continue
        resto = pc
        for k in range(len(conv)):
            a, b = np.asarray(conv[k]), np.asarray(conv[(k + 1) % len(conv)])
            e = b - a
            n = np.array([-e[1], e[0]]) * s
            f = cortar_meio_plano(resto, a, -n)
            if len(f) >= 3 and abs(area2(f)) > 1e-6:
                fora.append(f)
            resto = cortar_meio_plano(resto, a, n)
            if len(resto) < 3:
                break
        if len(resto) >= 3 and abs(area2(resto)) > 1e-6:
            dentro.append(resto)
    return dentro, fora


# ------------------------------------------------------------------
# 3a. Paredes, aberturas, piso e teto
# ------------------------------------------------------------------
class Parede:
    def __init__(self, nome, p0, p1, n, fora):
        self.nome = nome
        self.p0, self.p1 = np.asarray(p0, float), np.asarray(p1, float)
        self.R = Ref(self.p0, self.p1 - self.p0, n)
        self.L = float(np.linalg.norm(self.p1 - self.p0))
        self.fora = fora              # do plano da parede até a face de fora da casca
        self.vaos = []

    def u_de(self, x, z):
        return float((np.array([x, z]) - self.p0) @ self.R.u)


class Vao:
    """Uma abertura na parede: de u0 a u1, da base (y absoluto) até o contorno de cima [(u, y)]"""

    def __init__(self, tipo, contorno, base, vidro=None):
        self.tipo = tipo
        self.contorno = contorno
        self.u0, self.u1 = contorno[0][0], contorno[-1][0]
        self.base = base
        self.vidro = vidro             # distância do plano da parede até o plano do vidro da casca

    def topo(self, u):
        c = self.contorno
        if u <= c[0][0]:
            return c[0][1]
        for k in range(len(c) - 1):
            if c[k][0] <= u <= c[k + 1][0]:
                t = (u - c[k][0]) / max(c[k + 1][0] - c[k][0], 1e-9)
                return c[k][1] + t * (c[k + 1][1] - c[k][1])
        return c[-1][1]


def contorno_encolhido(pontos, folga, base_reta=None):
    """Encolhe o contorno do vão da casca 'folga' metros (nos lados e em cima)"""
    u0, u1 = pontos[0][0] + folga, pontos[-1][0] - folga
    out = []
    for u, y in pontos:
        out.append((min(max(u, u0), u1), y - folga))
    out[0] = (u0, out[0][1])
    out[-1] = (u1, out[-1][1])
    # tira pontos repetidos
    limpo = [out[0]]
    for p in out[1:]:
        if abs(p[0] - limpo[-1][0]) > 1e-4:
            limpo.append(p)
    return limpo


PAREDES = [
    Parede('frente', (X_OESTE, Z_FRENTE), (X_ARCO, Z_FRENTE), (0, 1), Z_FRENTE - Z_FRENTE_FORA),
    Parede('arco_oeste', (X_ARCO, Z_FRENTE), CANTO_A, (-1, 0), X_ARCO_FORA - X_ARCO),
    Parede('porta', CANTO_A, CANTO_B, DIAG_N, RECUO_DA_DIAGONAL),
    Parede('arco_sul', CANTO_B, (X_LESTE, Z_ARCO), (0, 1), Z_ARCO - Z_ARCO_FORA),
    Parede('leste', (X_LESTE, Z_ARCO), (X_LESTE, Z_FUNDO), (-1, 0), 1.0),
    Parede('fundo', (X_LESTE, Z_FUNDO), (X_OESTE, Z_FUNDO), (0, -1), 1.0),
    Parede('oeste', (X_OESTE, Z_FUNDO), (X_OESTE, Z_FRENTE), (1, 0), 1.0),
]
P_FRENTE, P_ARCO_OESTE, P_PORTA, P_ARCO_SUL, P_LESTE, P_FUNDO, P_OESTE = PAREDES

for x0 in JANELAS_DA_FRENTE:
    pts = [(x0 + du - X_OESTE, y) for du, y in ARCO_DAS_JANELAS]
    c = contorno_encolhido(pts, FOLGA_DO_VAO)
    P_FRENTE.vaos.append(Vao('janela', c, PEITORIL_DAS_JANELAS + FOLGA_DO_VAO, vidro=Z_FRENTE - VIDRO_DA_FRENTE_Z))
P_ARCO_OESTE.vaos.append(Vao('vitrine', contorno_encolhido([(z - Z_FRENTE, y) for z, y in ARCO_OESTE], FOLGA_DO_VAO),
                             PISO_Y, vidro=VIDRO_OESTE_X - X_ARCO))
P_ARCO_SUL.vaos.append(Vao('vitrine', contorno_encolhido([(x - CANTO_B[0], y) for x, y in ARCO_SUL], FOLGA_DO_VAO),
                           PISO_Y, vidro=Z_ARCO - VIDRO_SUL_Z))
U_PORTA = (PORTA_T[0] - T_CANTO_A + FOLGA_DO_VAO, PORTA_T[1] - T_CANTO_A - FOLGA_DO_VAO)
P_PORTA.vaos.append(Vao('porta', [(U_PORTA[0], PORTA_TOPO - FOLGA_DO_VAO), (U_PORTA[1], PORTA_TOPO - FOLGA_DO_VAO)], PISO_Y))


def niveis_da_parede(W):
    """As alturas (y absoluto) em que toda a parede é cortada: uma grade global de PASSO_DA_PAREDE a
    partir do piso, o teto e a base das janelas. Todas as colunas da parede cortam nos mesmos níveis."""
    ys = [PISO_Y + k * PASSO_DA_PAREDE for k in range(int(ALTURA / PASSO_DA_PAREDE) + 1)]
    ys = [y for y in ys if y < TETO_Y - 0.10] + [TETO_Y]
    for v in W.vaos:
        if v.base > PISO_Y + 1e-4:
            ys = [y for y in ys if abs(y - v.base) > 0.08] + [v.base]
    return sorted(ys)


def limpar_poligono(pc, tol=1e-6):
    out = []
    for p in pc:
        p = (float(p[0]), float(p[1]))
        if not out or abs(p[0] - out[-1][0]) > tol or abs(p[1] - out[-1][1]) > tol:
            out.append(p)
    if len(out) > 1 and abs(out[0][0] - out[-1][0]) <= tol and abs(out[0][1] - out[-1][1]) <= tol:
        out.pop()
    return out


def sem_juncoes_em_t(polis, tol=1e-5):
    """Põe em cada aresta os vértices dos vizinhos que caem no meio dela (junções em T viram vértices
    de verdade: sem frestas na rasterização e sem listras na luz interpolada)"""
    todos = np.unique(np.round(np.array([p for pc in polis for p in pc]), 6), axis=0)
    saida = []
    for pc in polis:
        novo = []
        for k in range(len(pc)):
            a, b = np.array(pc[k]), np.array(pc[(k + 1) % len(pc)])
            novo.append(pc[k])
            e = b - a
            L2 = float(e @ e)
            if L2 < 1e-12:
                continue
            t = (todos - a) @ e / L2
            d = np.abs((todos[:, 0] - a[0]) * e[1] - (todos[:, 1] - a[1]) * e[0]) / math.sqrt(L2)
            meio = (t > 1e-6) & (t < 1 - 1e-6) & (d < tol)
            for j in np.argsort(t[meio]):
                q = todos[meio][j]
                novo.append((float(q[0]), float(q[1])))
        saida.append(limpar_poligono(novo))
    return saida


def montar_parede(W, cor):
    """A parede, em colunas (cortadas nas pontas dos vãos e a cada PASSO_DA_PAREDE) e em faixas
    horizontais nos níveis globais; acima de um vão, as faixas são recortadas pelo contorno do arco"""
    m = CORES
    R = W.R
    us = {0.0, W.L}
    for v in W.vaos:
        us.update(u for u, _ in v.contorno)
    us = sorted(us)
    cortes = []
    for k in range(len(us) - 1):
        a, b = us[k], us[k + 1]
        n = max(1, math.ceil((b - a) / PASSO_DA_PAREDE - 1e-6))
        cortes += [a + (b - a) * j / n for j in range(n)]
    cortes.append(W.L)
    niveis = niveis_da_parede(W)
    polis = []
    for k in range(len(cortes) - 1):
        ua, ub = cortes[k], cortes[k + 1]
        if ub - ua < 1e-5:
            continue
        um = (ua + ub) / 2
        vao = next((v for v in W.vaos if v.u0 - 1e-6 <= um <= v.u1 + 1e-6), None)
        regioes = []
        if vao:
            if vao.base > PISO_Y + 1e-4:
                regioes.append([(ua, PISO_Y), (ub, PISO_Y), (ub, vao.base), (ua, vao.base)])
            regioes.append([(ua, vao.topo(ua)), (ub, vao.topo(ub)), (ub, TETO_Y), (ua, TETO_Y)])
        else:
            regioes.append([(ua, PISO_Y), (ub, PISO_Y), (ub, TETO_Y), (ua, TETO_Y)])
        for reg in regioes:
            reg = [np.array(p, float) for p in reg]
            for j in range(len(niveis) - 1):
                pc = cortar_meio_plano(reg, np.array([0.0, niveis[j]]), np.array([0.0, 1.0]))
                if len(pc) < 3:
                    continue
                pc = cortar_meio_plano(pc, np.array([0.0, niveis[j + 1]]), np.array([0.0, -1.0]))
                pc = limpar_poligono(pc)
                if len(pc) >= 3 and abs(area2(pc)) > 1e-6:
                    polis.append(pc)
    for pc in sem_juncoes_em_t(polis):
        m.face([R.p(u, y - PISO_Y, 0) for u, y in pc], cor, n=R.d(0, 0, 1), suave=True)


def montar_rebaixo(W, v, cor_rebaixo, cor_moldura, largura_moldura=0.16, saliencia=0.035):
    """Fecha o rebaixo entre a nossa parede e a face de fora da casca, e põe a moldura por dentro"""
    m = CORES
    R = W.R
    fora = -(W.fora - 0.02)
    hb = v.base - PISO_Y
    # ombreiras
    for u, s in ((v.u0, 1), (v.u1, -1)):
        topo = v.topo(u) - PISO_Y
        quad(m, R.p(u, hb, saliencia), R.p(u, hb, fora), R.p(u, topo, fora), R.p(u, topo, saliencia), cor_rebaixo, n=R.d(s, 0, 0))
    # o alto, seguindo o arco
    c = v.contorno
    for k in range(len(c) - 1):
        (ua, ya), (ub, yb) = c[k], c[k + 1]
        if ub - ua < 1e-5:
            continue
        quad(m, R.p(ua, ya - PISO_Y, saliencia), R.p(ub, yb - PISO_Y, saliencia), R.p(ub, yb - PISO_Y, fora),
             R.p(ua, ya - PISO_Y, fora), cor_rebaixo, n=R.d(0, -1, 0))
    # o peitoril (janelas)
    if v.tipo == 'janela':
        quad(m, R.p(v.u0, hb, saliencia), R.p(v.u1, hb, saliencia), R.p(v.u1, hb, fora), R.p(v.u0, hb, fora), cor_rebaixo, n=R.d(0, 1, 0))
        # o peitoril de dentro, de madeira
        caixa(m, R, v.u0 - 0.12, v.u1 + 0.12, hb - 0.06, hb + 0.0, 0, 0.16, cor_moldura, sem=('tras',))
    # a moldura por dentro (frente e a borda de fora)
    lm = largura_moldura
    pts_int = [(v.u0, v.base)] + list(c) + [(v.u1, v.base)]
    # deslocar a linha para fora (normal no plano da parede)
    ext = []
    for k, (u, y) in enumerate(pts_int):
        if k == 0:
            ext.append((u - lm, y))
        elif k == len(pts_int) - 1:
            ext.append((u + lm, y))
        else:
            up, yp = pts_int[k - 1]
            un, yn = pts_int[k + 1]
            t1 = unit((u - up, y - yp)) if (u, y) != (up, yp) else np.array([0, 1.0])
            t2 = unit((un - u, yn - y)) if (u, y) != (un, yn) else np.array([1.0, 0])
            nm = unit(np.array([-t1[1], t1[0]]) + np.array([-t2[1], t2[0]]))
            cosang = max(np.dot(nm, np.array([-t1[1], t1[0]])), 0.5)
            ext.append((u + nm[0] * lm / cosang, y + nm[1] * lm / cosang))
    for k in range(len(pts_int) - 1):
        (ua, ya), (ub, yb) = pts_int[k], pts_int[k + 1]
        (ea, fa), (eb, fb) = ext[k], ext[k + 1]
        pts = [R.p(ua, ya - PISO_Y, saliencia), R.p(ub, yb - PISO_Y, saliencia), R.p(eb, fb - PISO_Y, saliencia), R.p(ea, fa - PISO_Y, saliencia)]
        m.face(pts, cor_moldura, n=R.d(0, 0, 1))
        pts = [R.p(ea, fa - PISO_Y, 0), R.p(eb, fb - PISO_Y, 0), R.p(eb, fb - PISO_Y, saliencia), R.p(ea, fa - PISO_Y, saliencia)]
        m.face(pts, cor_moldura, n=R.d(ea + eb - ua - ub, fa + fb - ya - yb, 0))


def montar_caixilho(W, v, desde=None):
    """O caixilho (no plano do vidro da casca): a moldura em volta e as barras"""
    m = CORES
    R = W.R
    b = -v.vidro
    larg, prof = 0.08, 0.07
    base = desde if desde is not None else v.base
    lado = R.d(0, 0, 1)
    # em volta
    c = [(v.u0 + 0.04, base)] + [(min(max(u, v.u0 + 0.04), v.u1 - 0.04), y - 0.04) for u, y in v.contorno] + [(v.u1 - 0.04, base)]
    for k in range(len(c) - 1):
        (ua, ya), (ub, yb) = c[k], c[k + 1]
        if abs(ua - ub) < 1e-4 and abs(ya - yb) < 1e-4:
            continue
        barra(m, R.p(ua, ya - PISO_Y, b), R.p(ub, yb - PISO_Y, b), larg, prof, lado, CAIXILHO)
    barra(m, R.p(c[-1][0], base - PISO_Y, b), R.p(c[0][0], base - PISO_Y, b), larg, prof, lado, CAIXILHO)
    # barras em pé
    largura = v.u1 - v.u0
    colunas = 3 if largura < 4.5 else 4
    for k in range(1, colunas):
        u = v.u0 + largura * k / colunas
        barra(m, R.p(u, base - PISO_Y, b), R.p(u, v.topo(u) - 0.04 - PISO_Y, b), 0.05, 0.06, lado, CAIXILHO)
    # barras deitadas, até o começo do arco
    mola = min(v.contorno[0][1], v.contorno[-1][1]) - 0.04
    linhas = max(2, round((mola - base) / 1.05))
    for k in range(1, linhas + 1):
        y = base + (mola - base) * k / linhas
        if k == linhas:
            y = mola
        barra(m, R.p(v.u0 + 0.04, y - PISO_Y, b), R.p(v.u1 - 0.04, y - PISO_Y, b), 0.05, 0.06, lado, CAIXILHO)


print('Montando a loja...')
for W in PAREDES:
    montar_parede(W, PAREDE)
    for v in W.vaos:
        if v.tipo == 'porta':
            montar_rebaixo(W, v, PAREDE, VINHO, largura_moldura=0.32, saliencia=0.06)
        else:
            montar_rebaixo(W, v, PAREDE, VINHO)
            if v.tipo == 'janela':
                montar_caixilho(W, v)


# Piso: o L, o triângulo da porta e o rebaixo da porta (até a face de fora)
PECAS_DO_PISO = [
    [(X_OESTE, Z_FRENTE), (X_ARCO, Z_FRENTE), (X_ARCO, Z_FUNDO), (X_OESTE, Z_FUNDO)],
    [(X_ARCO, Z_ARCO), (X_LESTE, Z_ARCO), (X_LESTE, Z_FUNDO), (X_ARCO, Z_FUNDO)],
    [tuple(CANTO_A), tuple(CANTO_B), (X_ARCO, Z_ARCO)],
]
SOLEIRA = [tuple(diag(PORTA_T[0] + FOLGA_DO_VAO, -0.02)), tuple(diag(PORTA_T[1] - FOLGA_DO_VAO, -0.02)),
           tuple(diag(PORTA_T[1] - FOLGA_DO_VAO, RECUO_DA_DIAGONAL)), tuple(diag(PORTA_T[0] + FOLGA_DO_VAO, RECUO_DA_DIAGONAL))]
BAIA = [tuple(diag(PORTA_T[0] + FOLGA_DO_VAO, -0.05)), tuple(diag(PORTA_T[1] - FOLGA_DO_VAO, -0.05)),
        tuple(diag(PORTA_T[1] - FOLGA_DO_VAO, BAIA_FUNDO)), tuple(diag(PORTA_T[0] + FOLGA_DO_VAO, BAIA_FUNDO))]


def montar_piso():
    pecas = []
    for pc in PECAS_DO_PISO + [SOLEIRA]:
        pecas += grade_no_convexo([np.asarray(p) for p in pc], PASSO_DO_PISO)
    asfalto, madeira = separar_por_convexo(pecas, [np.asarray(p) for p in BAIA])
    up = np.array([0, 1.0, 0])
    for pc in madeira:
        pts = [(p[0], PISO_Y, p[1]) for p in pc]
        MALHAS['piso'].face(pts, BRANCO, n=up, uvs=[(p[0] / METROS_DO_PISO, -p[1] / METROS_DO_PISO) for p in pc], suave=True)
    for pc in asfalto:
        pts = [(p[0], PISO_Y, p[1]) for p in pc]
        MALHAS['asfalto'].face(pts, BRANCO, n=up, uvs=[(p[0] / METROS_DO_ASFALTO, -p[1] / METROS_DO_ASFALTO) for p in pc], suave=True)
    # as faixas da baia: brancas contínuas nos lados e amarela tracejada no meio
    h = PISO_Y + 0.012
    t0, t1 = PORTA_T[0] + FOLGA_DO_VAO, PORTA_T[1] - FOLGA_DO_VAO

    def faixa(ta, tb, sa, sb, cor):
        pts = [diag(ta, sa), diag(tb, sa), diag(tb, sb), diag(ta, sb)]
        CORES.face([(p[0], h, p[1]) for p in pts], cor, n=up)
    for t in (t0 + 0.25, t1 - 0.25 - 0.13):
        s = 0.05
        while s < BAIA_FUNDO - 0.3:
            faixa(t, t + 0.13, s, min(s + PASSO, BAIA_FUNDO - 0.3), FAIXA_BRANCA)
            s += PASSO
    tm = (t0 + t1) / 2
    s = 0.4
    while s < BAIA_FUNDO - 0.6:
        faixa(tm - 0.08, tm + 0.08, s, s + 0.9, FAIXA_AMARELA)
        s += 1.6
    faixa(t0 + 0.25, t1 - 0.25, BAIA_FUNDO - 0.43, BAIA_FUNDO - 0.3, FAIXA_BRANCA)


def montar_teto():
    pecas = []
    for pc in PECAS_DO_PISO:
        pecas += grade_no_convexo([np.asarray(p) for p in pc], PASSO_DO_TETO)
    down = np.array([0, -1.0, 0])
    for pc in pecas:
        CORES.face([(p[0], TETO_Y, p[1]) for p in pc], TETO, n=down, suave=True)


montar_piso()
montar_teto()


# ------------------------------------------------------------------
# 3b. Sanca, rodapé, colunas e vigas
# ------------------------------------------------------------------
def canto_deslocado(Wa, Wb, b):
    """O ponto (x, z) onde as linhas das paredes Wa e Wb, deslocadas b para dentro, se cruzam"""
    pa = Wa.p0 + b * Wa.R.n
    pb = Wb.p0 + b * Wb.R.n
    A = np.array([[Wa.R.u[0], -Wb.R.u[0]], [Wa.R.u[1], -Wb.R.u[1]]])
    rhs = pb - pa
    if abs(np.linalg.det(A)) < 1e-9:
        return pa + Wa.L * Wa.R.u
    t = np.linalg.solve(A, rhs)
    return pa + t[0] * Wa.R.u


def perfil_em_volta(perfil, cor, paredes=PAREDES):
    """Corre o perfil [(b, h)] em volta da sala inteira, com os cantos em meia-esquadria"""
    n = len(paredes)
    for k in range(len(perfil) - 1):
        (b0, h0), (b1, h1) = perfil[k], perfil[k + 1]
        for w in range(n):
            Wp, W, Wn = paredes[w - 1], paredes[w], paredes[(w + 1) % n]
            a0 = canto_deslocado(Wp, W, b0)
            a1 = canto_deslocado(Wp, W, b1)
            c0 = canto_deslocado(W, Wn, b0)
            c1 = canto_deslocado(W, Wn, b1)
            pts = [(a0[0], PISO_Y + h0, a0[1]), (c0[0], PISO_Y + h0, c0[1]), (c1[0], PISO_Y + h1, c1[1]), (a1[0], PISO_Y + h1, a1[1])]
            # a normal: do perfil (no plano b, h), para dentro da sala
            db, dh = b1 - b0, h1 - h0
            nrm = np.array([W.R.n[0] * dh, -db, W.R.n[1] * dh])
            if np.linalg.norm(nrm) < 1e-9:
                continue
            quad(CORES, *pts, cor, n=nrm, div=(max(1, math.ceil(W.L / (2 * PASSO))), 1))


def montar_sanca():
    """A sanca: uma calha vinho em volta da sala, aberta em cima, com a luz escondida que lava o teto"""
    H = ALTURA
    perfil = [(0.0, H - SANCA), (0.40, H - SANCA), (0.40, H - SANCA + 0.08), (0.36, H - SANCA + 0.12),
              (0.36, H - SANCA + 0.26), (0.30, H - SANCA + 0.26), (0.30, H - SANCA + 0.10), (0.0, H - SANCA + 0.10)]
    perfil_em_volta(perfil, VINHO)


def montar_rodape():
    perfil = [(0.035, 0.0), (0.035, 0.17), (0.0, 0.22)]
    for W in PAREDES:
        livres = [(0.0, W.L)]
        for v in W.vaos:
            if v.base <= PISO_Y + 0.01:
                novos = []
                for a, b in livres:
                    if v.u1 <= a or v.u0 >= b:
                        novos.append((a, b))
                    else:
                        if v.u0 - (0.32 if v.tipo == 'porta' else 0.16) > a:
                            novos.append((a, v.u0 - (0.32 if v.tipo == 'porta' else 0.16)))
                        if v.u1 + (0.32 if v.tipo == 'porta' else 0.16) < b:
                            novos.append((v.u1 + (0.32 if v.tipo == 'porta' else 0.16), b))
                livres = novos
        k = PAREDES.index(W)
        Wp, Wn = PAREDES[k - 1], PAREDES[(k + 1) % len(PAREDES)]
        for a, b in livres:
            for j in range(len(perfil) - 1):
                (b0, h0), (b1, h1) = perfil[j], perfil[j + 1]

                def ponta(u, bb, canto_de):
                    if canto_de is not None:
                        return canto_deslocado(*canto_de, bb)
                    return W.p0 + u * W.R.u + bb * W.R.n
                ca = (Wp, W) if a < 1e-6 else None
                cb = (W, Wn) if b > W.L - 1e-6 else None
                pa0, pa1 = ponta(a, b0, ca), ponta(a, b1, ca)
                pb0, pb1 = ponta(b, b0, cb), ponta(b, b1, cb)
                pts = [(pa0[0], PISO_Y + h0, pa0[1]), (pb0[0], PISO_Y + h0, pb0[1]), (pb1[0], PISO_Y + h1, pb1[1]), (pa1[0], PISO_Y + h1, pa1[1])]
                db, dh = b1 - b0, h1 - h0
                nrm = np.array([W.R.n[0] * dh, -db, W.R.n[1] * dh])
                quad(CORES, *pts, VINHO, n=nrm, div=(max(1, math.ceil((b - a) / PASSO)), 1))
            # as pontas (nos vãos)
            for u, s, canto in ((a, -1, a < 1e-6), (b, 1, b > W.L - 1e-6)):
                if canto:
                    continue
                pts = [W.R.p(u, 0, 0), W.R.p(u, 0, 0.035), W.R.p(u, 0.17, 0.035), W.R.p(u, 0.22, 0)]
                CORES.face(pts, VINHO, n=W.R.d(s, 0, 0))


CENTRO_DA_SALA = (465.5, 165.0)


def verniz(centro_xz, y0, altura, base=VINHO, realce=VINHO_BRILHO, direcao=None, largura=0.30):
    """A cor (por vértice) de uma peça redonda de madeira envernizada: um degradê de baixo (mais escuro)
    para cima e uma faixa de realce em pé, um pouco de lado de quem olha do meio da sala (o reflexo do
    verniz, que o shader do jogo não faz)"""
    c = np.asarray(centro_xz, float)
    d = unit(direcao if direcao is not None else np.array(CENTRO_DA_SALA) - c)
    base, realce = np.array(base), np.array(realce)

    def cor(p):
        r = np.array([p[0], p[2]]) - c
        t = float(np.clip((p[1] - y0) / altura, 0, 1))
        k = 0.80 + 0.28 * t
        if np.linalg.norm(r) < 1e-6:
            return tuple(base * k)
        ang = math.atan2(r[0] * d[1] - r[1] * d[0], float(r @ d))
        b = math.exp(-((ang - 0.5) / largura) ** 2) * 0.85 + math.exp(-((ang + 1.1) / (largura * 0.7)) ** 2) * 0.35
        return tuple(np.clip(base * k * (1 - b) + realce * b * (0.75 + 0.25 * t), 0, 1))
    return cor


def degrade_vertical(base, y0, altura, de=0.78, ate=1.08):
    """A cor de uma peça plana de madeira: mais escura embaixo, mais clara em cima"""
    base = np.array(base)
    return lambda p: tuple(np.clip(base * (de + (ate - de) * float(np.clip((p[1] - y0) / altura, 0, 1))), 0, 1))


def coluna(x, z, h_total, r=0.36, base_quadrada=True, cor=VINHO, cor_escura=VINHO_ESCURO, y0=PISO_Y):
    """Coluna redonda com base (plinto e anel) e capitel (equino e ábaco), envernizada"""
    R = Ref((x, z), (1, 0), (0, 1), y0=y0)
    lp = r * 1.32
    if base_quadrada:
        caixa(CORES, R, -lp, lp, 0, 0.26, -lp, lp, cor_escura, sem=('baixo',))
        caixa(CORES, R, -lp, lp, h_total - 0.18, h_total, -lp, lp, cor_escura, sem=())
    perfil = [(r * 1.18, 0.26), (r * 1.22, 0.32), (r * 1.18, 0.40), (r * 1.05, 0.44), (r, 0.50),
              (r * 0.94, h_total - 0.62), (r * 1.0, h_total - 0.55), (r * 1.06, h_total - 0.50),
              (r * 1.18, h_total - 0.36), (r * 1.25, h_total - 0.24), (r * 1.25, h_total - 0.18)]
    torno(CORES, (x, y0, z), (0, 1, 0), (1, 0, 0), perfil, 28, verniz((x, z), y0, h_total, base=cor))


def montar_colunas_e_vigas():
    h_col = ALTURA - SANCA
    # nos dois cantos da fachada (que entram na sala) e, na frente deles, nas paredes de oeste e do fundo
    coluna(CANTO_A[0], CANTO_A[1], h_col)
    coluna(CANTO_B[0], CANTO_B[1], h_col)
    coluna(X_OESTE + 0.12, CANTO_A[1], h_col)
    coluna(CANTO_B[0], Z_FUNDO - 0.12, h_col)
    for (a, b) in ((CANTO_A, (X_OESTE, CANTO_A[1])), (CANTO_B, (CANTO_B[0], Z_FUNDO))):
        a, b = np.asarray(a), np.asarray(b)
        R = Ref(a, b - a, (-(b - a)[1], (b - a)[0]))
        L = np.linalg.norm(b - a)
        caixa(CORES, R, 0, L, ALTURA - SANCA, ALTURA, -0.30, 0.30, VINHO, sem=('cima',))
    for x, z in (CANTO_A, CANTO_B, (X_OESTE + 0.12, CANTO_A[1]), (CANTO_B[0], Z_FUNDO - 0.12)):
        COLISAO_CAIXAS.append(('coluna', x, z, 0.5))
        COLUNAS_GRANDES.append((x, z))


COLISAO_CAIXAS = []      # obstáculos: (nome, x, z, raio) ou ('caixa', R, a0, a1, b0, b1)
COLUNAS_GRANDES = []     # (x, z) das colunas redondas do salão (os pôsteres desviam delas)
ESTANTES = []            # (parede, a0, a1) das estantes encostadas (idem)
montar_sanca()
montar_rodape()
montar_colunas_e_vigas()


# ------------------------------------------------------------------
# 3c. Pneus
# ------------------------------------------------------------------
def pneu(centro, eixo, e1, R_fora, largura, R_dentro, tipo='preto', tinta=BRANCO, n_seg=20, assento=False):
    """Um pneu de verdade (sem aro): flancos, ombros e banda de rodagem.
    tipo: 'preto', 'faixa_branca' (faixa branca nos flancos), 'branco' (flanco todo branco, banda preta),
    'claro' (pneu bege: flanco e banda claros, tingidos por 'tinta')"""
    W = largura
    Rf, Rd = R_fora, R_dentro
    alt = Rf - Rd
    # perfil anti-horário no plano (r, h): começa no talão de baixo, sobe pelo flanco de fora...
    perfil = [
        (Rd + 0.02 * alt, -0.40 * W),
        (Rd + 0.30 * alt, -0.49 * W),
        (Rd + 0.65 * alt, -0.495 * W),
        (Rf - 0.08 * alt, -0.44 * W),
        (Rf - 0.015 * alt, -0.36 * W),
        (Rf, -0.18 * W),
        (Rf + 0.004, 0.0),
        (Rf, 0.18 * W),
        (Rf - 0.015 * alt, 0.36 * W),
        (Rf - 0.08 * alt, 0.44 * W),
        (Rd + 0.65 * alt, 0.495 * W),
        (Rd + 0.30 * alt, 0.49 * W),
        (Rd + 0.02 * alt, 0.40 * W),
        (Rd - 0.01, 0.15 * W),
        (Rd - 0.01, -0.15 * W),
    ]
    n = len(perfil)
    fl = v_da_faixa('flanco')
    br = v_da_faixa('branco')
    if tipo == 'claro':
        fl = br
    banda = 'banda_clara' if tipo == 'claro' else 'banda'
    tipos = []
    for k in range(n):
        # trecho k vai do ponto k ao k+1 (o perfil é anti-horário: as faces ficam para fora)
        a, b = perfil[k], perfil[(k + 1) % n]
        ra, rb = a[0], b[0]
        meio_r = (ra + rb) / 2
        frac = (meio_r - Rd) / alt
        if min(ra, rb) >= Rf - 0.021 * alt and abs(a[1]) <= 0.37 * W and abs(b[1]) <= 0.37 * W:
            # banda de rodagem: v corre de um ombro ao outro
            fa = (a[1] / W + 0.36) / 0.72
            fb = (b[1] / W + 0.36) / 0.72
            tipos.append((v_da_faixa(banda, np.clip(fa, 0, 1)), v_da_faixa(banda, np.clip(fb, 0, 1))))
        elif frac < 0.0:
            tipos.append((fl, fl))
        else:
            v = fl
            if tipo == 'faixa_branca' and 0.30 < frac < 0.62:
                v = br
            if tipo == 'branco' and frac < 0.85:
                v = br
            tipos.append((v, v))
    torno(MALHAS['pneus'], centro, eixo, e1, perfil, n_seg, tinta, tipos=tipos, repete=6, fechar=True)
    if assento:
        # o assento vermelho, no furo de cima (pneu-banco)
        c = np.asarray(centro, float) + unit(eixo) * (0.44 * W)
        disco(CORES, c, eixo, e1, Rd + 0.01, n_seg, VERMELHO_CORTINA)


def plaquinha(centro, direita, frente, larg, alt, txt_cima, txt_baixo, inclina=14):
    """Plaquinha creme com faixa vermelha em cima, inclinada para trás (como as do filme)"""
    centro = np.asarray(centro, float)
    direita = unit(direita)
    frente = unit(frente)
    up = np.array([0, 1.0, 0])
    a = math.radians(inclina)
    cima = unit(up * math.cos(a) - frente * math.sin(a))
    nrm = np.cross(direita, cima)
    if np.dot(nrm, frente) < 0:
        direita = -direita
        nrm = -nrm
    esp = 0.018
    base = centro - cima * alt / 2

    def P(s, t, e=0.0):
        return base + direita * (s * larg / 2) + cima * (t * alt) - nrm * e
    # frente: creme embaixo, vermelha em cima
    quad(CORES, P(-1, 0), P(1, 0), P(1, 0.64), P(-1, 0.64), CREME, n=nrm, div=(1, 1))
    quad(CORES, P(-1, 0.64), P(1, 0.64), P(1, 1), P(-1, 1), VERMELHO, n=nrm, div=(1, 1))
    # costas e bordas
    quad(CORES, P(-1, 0, esp), P(1, 0, esp), P(1, 1, esp), P(-1, 1, esp), CREME, n=-nrm, div=(1, 1))
    quad(CORES, P(-1, 1), P(1, 1), P(1, 1, esp), P(-1, 1, esp), CREME, n=cima, div=(1, 1))
    quad(CORES, P(-1, 0), P(-1, 1), P(-1, 1, esp), P(-1, 0, esp), CREME, n=-direita, div=(1, 1))
    quad(CORES, P(1, 0), P(1, 1), P(1, 1, esp), P(1, 0, esp), CREME, n=direita, div=(1, 1))
    off = nrm * 0.003
    texto(CORES, txt_cima, FONTE_TITULO, base + cima * alt * 0.82 + off, direita, cima, larg * 0.80, alt * 0.27, CREME)
    texto(CORES, txt_baixo, FONTE_TITULO, base + cima * alt * 0.32 + off, direita, cima, larg * 0.90, alt * 0.50, VERMELHO)


def berco(R, a, h, b, larg, prof, cor=METAL_ESCURO):
    """O suporte de um pneu em pé"""
    caixa(CORES, R, a - larg / 2, a - larg / 2 + 0.06, h, h + 0.09, b - prof / 2, b + prof / 2, cor)
    caixa(CORES, R, a + larg / 2 - 0.06, a + larg / 2, h, h + 0.09, b - prof / 2, b + prof / 2, cor)
    caixa(CORES, R, a - larg / 2, a + larg / 2, h, h + 0.03, b - prof / 2, b + prof / 2, cor)


def pneu_em_pe(R, a, h, b, Rf=0.40, W=0.24, Rd=0.22, tipo='preto', tinta=BRANCO, n_seg=20):
    """Um pneu em pé num berço, olhando para a frente do referencial (eixo = n)"""
    berco(R, a, h, b, Rf * 1.0, W + 0.04)
    c = R.p(a, h + 0.05 + Rf, b)
    pneu(c, R.d(0, 0, 1), R.d(1, 0, 0), Rf, W, Rd, tipo=tipo, tinta=tinta, n_seg=n_seg)


def pilha_de_pneus(R, a, h, b, quantos, Rf=0.42, W=0.21, Rd=0.24, tipo='claro', tinta=BEGE, n_seg=20):
    for k in range(quantos):
        c = R.p(a, h + W / 2 + k * W * 0.98, b)
        e1 = R.d(math.cos(k * 0.7), 0, math.sin(k * 0.7))
        pneu(c, (0, 1, 0), e1, Rf, W, Rd, tipo=tipo, tinta=tinta, n_seg=n_seg)
    return h + quantos * W * 0.98


# ------------------------------------------------------------------
# 3d. O móvel do fundo, com o nicho
# ------------------------------------------------------------------
def painel_de_raiz(R, a0, a1, h0, h1, b, moldura=0.06):
    """Um painel de raiz de madeira com moldura, na frente (b) do referencial"""
    caixa(CORES, R, a0, a1, h0, h1, b - 0.01, b + 0.015, VINHO, sem=('tras',))
    uvf = lambda p: (float(np.dot(np.array([p[0], p[2]]) - R.o, R.u)) / METROS_DA_RAIZ, (p[1] - R.y0) / METROS_DA_RAIZ)
    quad(MALHAS['raiz'], R.p(a0 + moldura, h0 + moldura, b + 0.02), R.p(a1 - moldura, h0 + moldura, b + 0.02),
         R.p(a1 - moldura, h1 - moldura, b + 0.02), R.p(a0 + moldura, h1 - moldura, b + 0.02), BRANCO, n=R.d(0, 0, 1),
         uvf=uvf)


BALCAO = 1.12        # o alto da base do móvel (onde ficam a primeira fileira de pneus e o nicho)
PRATELEIRA = 2.86    # o alto da prateleira do meio
TOPO_DO_MOVEL = 4.55  # onde começa a cornija


def base_do_movel(R, a0, a1, prof, paineis):
    caixa(CORES, R, a0 + 0.04, a1 - 0.04, 0, 0.14, 0, prof - 0.08, VINHO_FUNDO, sem=('baixo', 'tras'))
    caixa(CORES, R, a0, a1, 0.14, BALCAO - 0.08, 0, prof, VINHO_ESCURO, sem=('baixo', 'tras'))
    caixa(CORES, R, a0 - 0.03, a1 + 0.03, BALCAO - 0.08, BALCAO, 0, prof + 0.07, VINHO, sem=('tras',))
    for p0, p1 in paineis:
        painel_de_raiz(R, p0 + 0.1, p1 - 0.1, 0.25, BALCAO - 0.18, prof)


FUNDO_CLARO = (0.44, 0.17, 0.16)     # o fundo dos compartimentos, perto da luz escondida
FUNDO_ESCURO = (0.16, 0.05, 0.055)
LUZES_DAS_PRATELEIRAS = []           # (centro, ao longo, largura, para onde emite): só no assado


def secao_de_prateleiras(R, a0, a1, prof, rotulos, tipos_de_pneu, topo=TOPO_DO_MOVEL):
    """Fundo, laterais, prateleira do meio e duas fileiras de três pneus em pé com plaquinhas. O fundo de
    cada compartimento clareia para cima (onde fica uma luz escondida, que só existe no assado), para os
    pneus pretos se destacarem dele."""
    caixa(CORES, R, a0, a1, BALCAO, topo, 0.0, 0.05, VINHO_FUNDO, sem=('tras', 'frente'))
    for h0, h1 in ((BALCAO, PRATELEIRA - 0.08), (PRATELEIRA, topo)):
        def fundo(p, h0=h0, h1=h1):
            t = float(np.clip((p[1] - R.y0 - h0) / (h1 - h0), 0, 1)) ** 1.4
            return tuple(np.array(FUNDO_ESCURO) * (1 - t) + np.array(FUNDO_CLARO) * t)
        quad(CORES, R.p(a0, h0, 0.05), R.p(a1, h0, 0.05), R.p(a1, h1, 0.05), R.p(a0, h1, 0.05), fundo, n=R.d(0, 0, 1), passo=0.3)
        LUZES_DAS_PRATELEIRAS.append((R.p((a0 + a1) / 2, h1 - 0.10, prof - 0.10), R.d(1, 0, 0), a1 - a0 - 0.3,
                                      unit(R.d(0, -1.0, -0.35))))
    for a in (a0, a1 - 0.08):
        caixa(CORES, R, a, a + 0.08, BALCAO, topo, 0.05, prof, VINHO_ESCURO, sem=('tras',))
    caixa(CORES, R, a0 + 0.08, a1 - 0.08, PRATELEIRA - 0.08, PRATELEIRA, 0.05, prof - 0.02, VINHO_ESCURO, sem=('tras',))
    caixa(CORES, R, a0 + 0.08, a1 - 0.08, PRATELEIRA, PRATELEIRA + 0.05, prof - 0.08, prof, VINHO, sem=('tras',))
    w = (a1 - a0) / 3
    k = 0
    for h in (BALCAO, PRATELEIRA):
        for j in range(3):
            a = a0 + w * (j + 0.5)
            # pneus grandes: ~70% da altura do compartimento
            pneu_em_pe(R, a, h, 0.40, Rf=0.54, W=0.30, Rd=0.29, tipo=tipos_de_pneu[k % len(tipos_de_pneu)], n_seg=20)
            cima, baixo = rotulos[k % len(rotulos)]
            plaquinha(R.p(a, h + 0.17, prof - 0.12), R.d(1, 0, 0), R.d(0, 0, 1), 0.42, 0.27, cima, baixo)
            k += 1


def cortina(R, a0, a1, h0, h1, b, dobras=4, amp=0.05):
    """Cortina de veludo com dobras"""
    n = dobras * 8
    for j in range(n):
        s0, s1 = j / n, (j + 1) / n
        ua, ub = a0 + (a1 - a0) * s0, a0 + (a1 - a0) * s1
        ba = b + amp * math.sin(2 * math.pi * dobras * s0)
        bb = b + amp * math.sin(2 * math.pi * dobras * s1)
        nv = max(1, math.ceil((h1 - h0) / PASSO))
        for k in range(nv):
            t0, t1 = k / nv, (k + 1) / nv
            ha, hb = h0 + (h1 - h0) * t0, h0 + (h1 - h0) * t1
            # a ponta de baixo abre um pouco
            CORES.face([R.p(ua, ha, ba), R.p(ub, ha, bb), R.p(ub, hb, bb), R.p(ua, hb, ba)], VERMELHO_CORTINA, n=R.d(0, 0, 1))


def placa_fettuccini(R, a, h, b, larg=1.9, alt=0.62):
    """A placa "Fettuccini / Alfredo": vermelha em cima, faixa amarela e verde embaixo"""
    esp = 0.05
    caixa(CORES, R, a - larg / 2 - 0.04, a + larg / 2 + 0.04, h - 0.04, h + alt + 0.04, b - esp, b, DOURADO)
    corte = h + alt * 0.40
    quad(CORES, R.p(a - larg / 2, corte, b + 0.004), R.p(a + larg / 2, corte, b + 0.004), R.p(a + larg / 2, h + alt, b + 0.004),
         R.p(a - larg / 2, h + alt, b + 0.004), VERMELHO, n=R.d(0, 0, 1), div=(1, 1))

    def degrade(p):
        s = abs(float(np.dot(np.array([p[0], p[2]]) - R.o, R.u)) - a) / (larg / 2)
        t = (p[1] - R.y0 - h) / (alt * 0.4)
        amarelo = np.array([0.98, 0.84, 0.30])
        verde = np.array([0.40, 0.62, 0.30])
        c = amarelo * (1 - s ** 1.5) + verde * s ** 1.5
        return tuple(c * (0.85 + 0.15 * t))
    quad(CORES, R.p(a - larg / 2, h, b + 0.004), R.p(a + larg / 2, h, b + 0.004), R.p(a + larg / 2, corte, b + 0.004),
         R.p(a - larg / 2, corte, b + 0.004), degrade, n=R.d(0, 0, 1), div=(8, 2))
    quad(CORES, R.p(a - larg / 2, corte - 0.012, b + 0.006), R.p(a + larg / 2, corte - 0.012, b + 0.006),
         R.p(a + larg / 2, corte + 0.012, b + 0.006), R.p(a - larg / 2, corte + 0.012, b + 0.006), CREME, n=R.d(0, 0, 1), div=(1, 1))
    off = R.d(0, 0, 0.008)
    texto(CORES, 'Fettuccini', FONTE_TITULO, R.p(a, h + alt * 0.70, b) + off, R.d(1, 0, 0), (0, 1, 0), larg * 0.86, alt * 0.44, CREME, resolucao=2)
    texto(CORES, 'Alfredo', FONTE_TITULO, R.p(a, h + alt * 0.20, b) + off, R.d(1, 0, 0), (0, 1, 0), larg * 0.45, alt * 0.27, VINHO_ESCURO, resolucao=2)


def coluninha(R, a, b, h0, h1, r=0.2):
    c = R.p(a, h0, b)
    lp = r * 1.4
    caixa(CORES, R, a - lp, a + lp, h0, h0 + 0.12, b - lp, b + lp, VINHO_ESCURO, sem=('baixo',))
    caixa(CORES, R, a - lp, a + lp, h1 - 0.12, h1, b - lp, b + lp, VINHO_ESCURO, sem=())
    H = h1 - h0
    perfil = [(r * 1.25, 0.12), (r * 1.2, 0.20), (r, 0.26), (r * 0.95, H - 0.3), (r * 1.1, H - 0.22), (r * 1.25, H - 0.12)]
    torno(CORES, c, (0, 1, 0), R.d(1, 0, 0), perfil, 24, verniz((c[0], c[2]), c[1], H, direcao=R.n))


def cornija(R, a0, a1, prof, h=TOPO_DO_MOVEL):
    caixa(CORES, R, a0 - 0.03, a1 + 0.03, h, h + 0.12, 0, prof + 0.05, VINHO, sem=('tras',))
    caixa(CORES, R, a0, a1, h + 0.12, h + 0.42, 0, prof, VINHO_ESCURO, sem=('tras',))
    n = max(1, round((a1 - a0) / 1.4))
    w = (a1 - a0) / n
    for k in range(n):
        p0 = a0 + k * w
        caixa(CORES, R, p0 + 0.06, p0 + w - 0.06, h + 0.16, h + 0.38, prof, prof + 0.012, VINHO, sem=('tras',))
        uvf = lambda p: (float(np.dot(np.array([p[0], p[2]]) - R.o, R.u)) / METROS_DA_RAIZ, (p[1] - R.y0) / METROS_DA_RAIZ)
        quad(MALHAS['raiz'], R.p(p0 + 0.10, h + 0.20, prof + 0.014), R.p(p0 + w - 0.10, h + 0.20, prof + 0.014),
             R.p(p0 + w - 0.10, h + 0.34, prof + 0.014), R.p(p0 + 0.10, h + 0.34, prof + 0.014), BRANCO, n=R.d(0, 0, 1), uvf=uvf)
    caixa(CORES, R, a0 - 0.08, a1 + 0.08, h + 0.42, h + 0.55, 0, prof + 0.14, VINHO, sem=('tras',))


ROTULOS_A = [('Fettuccini', 'Fettuccini'), ('Fettuccini', 'Alfredo'), ('Fettuccini', 'Fettuccini'),
             ('Fettuccini', 'Radials'), ('Fettuccini', 'Tread Star'), ('Fettuccini', 'Radials')]
ROTULOS_B = [('Fettuccini', 'Corsa'), ('Fettuccini', 'Fettuccini'), ('Fettuccini', 'Strada'),
             ('Fettuccini', 'Tread Star'), ('Fettuccini', 'Radials'), ('Fettuccini', 'Sport')]
LUZES_DO_NICHO = []


def montar_movel():
    # referencial: a = 0 na ponta esquerda (vista de dentro: x = MOVEL_X[1]), b para dentro da sala
    x0, x1 = MOVEL_X
    R = Ref((x1, Z_FUNDO), (-1, 0), (0, -1))
    L = x1 - x0
    prof = 1.0
    lado = 3.9
    col = 0.7
    nicho = L - 2 * lado - 2 * col
    a_n0, a_n1 = lado + col, lado + col + nicho
    T = TOPO_DO_MOVEL
    base_do_movel(R, 0, L, prof, [(0, lado), (lado + col, lado + col + nicho), (L - lado, L)])
    secao_de_prateleiras(R, 0, lado, prof, ROTULOS_A, ['preto', 'preto', 'faixa_branca'])
    secao_de_prateleiras(R, L - lado, L, prof, ROTULOS_B, ['faixa_branca', 'preto', 'preto'])
    # entre as seções e o nicho: pilastras com as colunas na frente
    for a in (lado, a_n1):
        caixa(CORES, R, a, a + col, BALCAO, T, 0.0, 0.34, degrade_vertical(VINHO_ESCURO, PISO_Y + BALCAO, T - BALCAO), sem=('tras',))
        coluninha(R, a + col / 2, 0.62, BALCAO, T, r=0.21)
    # o nicho: o fundo (que a luz quente escondida faz brilhar), o teto, o chão, as cortinas
    meio = (a_n0 + a_n1) / 2
    quad(CORES, R.p(a_n0, BALCAO, 0.03), R.p(a_n1, BALCAO, 0.03), R.p(a_n1, T, 0.03), R.p(a_n0, T, 0.03),
         (0.62, 0.22, 0.10), n=R.d(0, 0, 1), passo=0.16)
    caixa(CORES, R, a_n0, a_n1, T - 0.06, T, 0.03, prof, VINHO_FUNDO, sem=('tras', 'cima'))
    quad(CORES, R.p(a_n0, BALCAO, 0.03), R.p(a_n1, BALCAO, 0.03), R.p(a_n1, BALCAO, prof), R.p(a_n0, BALCAO, prof),
         VINHO_FUNDO, n=R.d(0, 1, 0), passo=0.2)
    cortina(R, a_n0 + 0.02, a_n0 + 0.40, BALCAO + 0.01, T - 0.2, 0.93, dobras=3)
    cortina(R, a_n1 - 0.40, a_n1 - 0.02, BALCAO + 0.01, T - 0.2, 0.93, dobras=3)
    caixa(CORES, R, a_n0, a_n1, T - 0.30, T - 0.06, 0.82, 0.98, VERMELHO_CORTINA, sem=('tras',))
    # as três pilhas de pneus bege com um pneu de faixa branca em pé em cima e as plaquinhas
    for desl, quantos, rot, tinta, Rf in ((-1.15, 4, 'Latte', (0.86, 0.76, 0.60), 0.40), (0.0, 5, 'Bianco Massimo', (0.92, 0.86, 0.72), 0.47),
                                          (1.15, 4, 'Crema', (0.90, 0.82, 0.64), 0.40)):
        topo = pilha_de_pneus(R, meio + desl, BALCAO, 0.47, quantos, Rf=0.44, W=0.23, Rd=0.25, tinta=tinta, n_seg=18)
        pneu_em_pe(R, meio + desl, topo, 0.45, Rf=Rf, W=0.24, Rd=Rf * 0.5, tipo='branco', n_seg=24)
        plaquinha(R.p(meio + desl, topo + 0.12, 0.86), R.d(1, 0, 0), R.d(0, 0, 1), 0.38, 0.22, 'Fettuccini', rot)
    placa_fettuccini(R, meio, T - 1.02, 0.30, larg=2.1, alt=0.68)
    LUZES_DO_NICHO.append(R.p(meio, 2.55, 0.18))
    LUZES_DO_NICHO.append(R.p(meio - 1.15, 2.35, 0.16))
    LUZES_DO_NICHO.append(R.p(meio + 1.15, 2.35, 0.16))
    cornija(R, 0, L, prof)
    COLISAO_CAIXAS.append(('caixa', R, -0.1, L + 0.1, -0.2, prof + 0.12))


def estante(W, a_meio, largura=3.9, rotulos=ROTULOS_B, tipos=('preto', 'faixa_branca', 'preto')):
    """Uma estante de pneus solta, encostada na parede W (como as seções do móvel)"""
    R = W.R
    a0, a1 = a_meio - largura / 2, a_meio + largura / 2
    prof = 0.95
    base_do_movel(R, a0, a1, prof, [(a0, a1)])
    secao_de_prateleiras(R, a0, a1, prof, rotulos, list(tipos))
    cornija(R, a0, a1, prof)
    COLISAO_CAIXAS.append(('caixa', R, a0 - 0.1, a1 + 0.1, -0.2, prof + 0.12))
    ESTANTES.append((W, a0, a1))


montar_movel()
estante(P_LESTE, P_LESTE.u_de(X_LESTE, 172.65))
estante(P_OESTE, P_OESTE.u_de(X_OESTE, 151.10), rotulos=ROTULOS_A, tipos=('faixa_branca', 'preto', 'preto'))


# ------------------------------------------------------------------
# 3e. As vitrines (os arcos que descem até o chão), o pneu-troféu
# ------------------------------------------------------------------
ALTURA_DA_VITRINE = 1.05


def montar_vitrine(W, v, conteudo):
    R = W.R
    fora = -(W.fora - 0.12)
    dentro = 1.30
    h = ALTURA_DA_VITRINE
    caixa(CORES, R, v.u0, v.u1, 0, h - 0.08, fora, dentro, VINHO_ESCURO, sem=('baixo',))
    caixa(CORES, R, v.u0 - 0.04, v.u1 + 0.04, h - 0.08, h, fora - 0.04, dentro + 0.05, VINHO, sem=('baixo',))
    painel_de_raiz(R, v.u0 + 0.15, v.u1 - 0.15, 0.14, h - 0.16, dentro)
    quad(CORES, R.p(v.u0 + 0.05, h + 0.002, fora + 0.05), R.p(v.u1 - 0.05, h + 0.002, fora + 0.05),
         R.p(v.u1 - 0.05, h + 0.002, dentro - 0.02), R.p(v.u0 + 0.05, h + 0.002, dentro - 0.02), VERMELHO_CORTINA, n=(0, 1, 0))
    montar_caixilho(W, v, desde=PISO_Y + h)
    COLISAO_CAIXAS.append(('caixa', R, v.u0 - 0.05, v.u1 + 0.05, -0.5, dentro + 0.08))
    conteudo(R, v, h, (fora + dentro) / 2)


def vitrine_do_trofeu(R, v, h, bm):
    meio = (v.u0 + v.u1) / 2
    # o pedestal
    caixa(CORES, R, meio - 0.30, meio + 0.30, h, h + 0.10, bm - 0.30, bm + 0.30, VINHO_ESCURO)
    caixa(CORES, R, meio - 0.24, meio + 0.24, h + 0.10, h + 0.86, bm - 0.24, bm + 0.24, VINHO, sem=('baixo',))
    caixa(CORES, R, meio - 0.30, meio + 0.30, h + 0.86, h + 0.96, bm - 0.30, bm + 0.30, DOURADO, sem=('baixo',))
    caixa(CORES, R, meio - 0.14, meio + 0.14, h + 0.40, h + 0.58, bm + 0.24, bm + 0.25, DOURADO, sem=('tras',))
    texto(CORES, 'PISTON CUP', FONTE_TEXTO, R.p(meio, h + 0.49, bm + 0.253), R.d(1, 0, 0), (0, 1, 0), 0.24, 0.07, VINHO_ESCURO)
    pneu_em_pe(R, meio, h + 0.96, bm, Rf=0.46, W=0.26, Rd=0.24, tipo='faixa_branca', n_seg=28)
    # pilhas dos lados
    for a in (v.u0 + 1.0, v.u1 - 1.0):
        topo = pilha_de_pneus(R, a, h, bm, 3, Rf=0.40, W=0.22, Rd=0.22, tipo='faixa_branca', tinta=BRANCO, n_seg=16)
        plaquinha(R.p(a, topo + 0.12, bm + 0.25), R.d(1, 0, 0), R.d(0, 0, 1), 0.34, 0.2, 'Fettuccini', 'Corsa')


def vitrine_da_piramide(R, v, h, bm):
    meio = (v.u0 + v.u1) / 2
    for desl in (-1.4, 0.0, 1.4):
        topo = pilha_de_pneus(R, meio + desl, h, bm, 2 if desl else 3, Rf=0.40, W=0.22, Rd=0.22,
                              tipo='preto' if desl else 'claro', tinta=BRANCO if desl else (0.90, 0.82, 0.64), n_seg=16)
        pneu_em_pe(R, meio + desl, topo, bm, Rf=0.40, W=0.24, Rd=0.21, tipo='faixa_branca' if desl else 'branco', n_seg=20)
        plaquinha(R.p(meio + desl, topo + 0.12, bm + 0.32), R.d(1, 0, 0), R.d(0, 0, 1), 0.34, 0.2, 'Fettuccini',
                  ('Radials', 'Bianco', 'Tread Star')[int(desl / 1.4) + 1])


montar_vitrine(P_ARCO_OESTE, P_ARCO_OESTE.vaos[0], vitrine_do_trofeu)
montar_vitrine(P_ARCO_SUL, P_ARCO_SUL.vaos[0], vitrine_da_piramide)


# ------------------------------------------------------------------
# 3f. A porta de garagem (enrolada no alto), o espelho, o elevador, a máquina, os bancos
# ------------------------------------------------------------------
def montar_porta_enrolada():
    W = P_PORTA
    v = W.vaos[0]
    R = W.R
    topo = v.topo(v.u0) - PISO_Y
    # o rolo da porta: as ripas enroladas (faixas ao comprido do eixo, em dois tons de vinho)
    centro_a = R.p(v.u0 - 0.2, topo + 0.42, 0.42)
    centro_b = R.p(v.u1 + 0.2, topo + 0.42, 0.42)
    eixo = unit(centro_b - centro_a)
    L = np.linalg.norm(centro_b - centro_a)
    e1 = np.array([0, 1.0, 0])
    e2 = np.cross(eixo, e1)
    n, raio = 40, 0.34
    for j in range(n):
        a0, a1 = 2 * math.pi * j / n, 2 * math.pi * (j + 1) / n
        r0 = math.cos(a0) * e1 + math.sin(a0) * e2
        r1 = math.cos(a1) * e1 + math.sin(a1) * e2
        cor = (0.46, 0.16, 0.14) if j % 2 else (0.30, 0.085, 0.08)
        quad(CORES, centro_a + r0 * raio, centro_b + r0 * raio, centro_b + r1 * raio, centro_a + r1 * raio, cor,
             n=unit(r0 + r1), div=(max(1, math.ceil(L / 1.2)), 1))
    for c, s in ((centro_a, -1), (centro_b, 1)):
        disco(CORES, c, eixo * s, e1, raio, n, METAL_ESCURO)
    # a lâmina de baixo da porta, para fora do rolo
    caixa(CORES, R, v.u0 - 0.1, v.u1 + 0.1, topo + 0.04, topo + 0.12, 0.08, 0.16, METAL_ESCURO)
    # a caixa (capô) por cima: vinho, mais escura por baixo, com o nome da loja na frente
    baixo = {'baixo': VINHO_FUNDO}
    caixa(CORES, R, v.u0 - 0.3, v.u1 + 0.3, topo + 0.80, topo + 0.92, 0.0, 0.90, VINHO, cores=baixo)
    caixa(CORES, R, v.u0 - 0.3, v.u1 + 0.3, topo + 0.26, topo + 0.80, 0.80, 0.86, VINHO, sem=('tras',), cores=baixo)
    caixa(CORES, R, v.u0 - 0.34, v.u1 + 0.34, topo + 0.22, topo + 0.28, 0.80, 0.90, VINHO_ESCURO, cores=baixo)
    caixa(CORES, R, v.u0 - 0.34, v.u1 + 0.34, topo + 0.76, topo + 0.82, 0.80, 0.90, VINHO_ESCURO, cores=baixo)
    for a in (v.u0 - 0.3, v.u1 + 0.24):
        caixa(CORES, R, a, a + 0.06, topo + 0.10, topo + 0.80, 0.0, 0.86, VINHO, cores=baixo)
    meio = (v.u0 + v.u1) / 2
    texto(CORES, 'Casa Della Tires', FONTE_TITULO, R.p(meio, topo + 0.52, 0.865), R.d(1, 0, 0), (0, 1, 0), 4.2, 0.34, CREME, resolucao=2)
    for a in (meio - 2.75, meio + 2.75):
        caixa(CORES, R, a - 0.30, a + 0.30, topo + 0.47, topo + 0.57, 0.86, 0.875, DOURADO, sem=('tras',))
    # os trilhos dos lados
    for a in (v.u0 - 0.14, v.u1 + 0.04):
        caixa(CORES, R, a, a + 0.10, 0.0, topo + 0.12, 0.06, 0.18, METAL)


def montar_espelho():
    W = P_FUNDO
    R = W.R
    a0, a1 = W.u_de(ESPELHO_X[1], Z_FUNDO), W.u_de(ESPELHO_X[0], Z_FUNDO)
    h0, h1 = 0.22, 4.75
    f = 0.22
    caixa(CORES, R, a0, a1, h0, h1, 0, 0.09, VINHO, sem=('tras',))
    caixa(CORES, R, a0 - 0.08, a1 + 0.08, h1, h1 + 0.16, 0, 0.13, VINHO_ESCURO, sem=('tras',))
    caixa(CORES, R, a0 - 0.06, a1 + 0.06, h0 - 0.12, h0, 0, 0.12, VINHO_ESCURO, sem=('tras',))
    # o vidro do espelho (o reflexo vem pintado na textura, se houver)
    ESPELHO['R'] = R
    ESPELHO['a'] = (a0 + f, a1 - f)
    ESPELHO['h'] = (h0 + f, h1 - f)
    ESPELHO['b'] = 0.095
    # a textura é quadrada (512 x 512), mas só as linhas de cima têm o reflexo, no aspecto do vidro
    larg, alt = (a1 - a0) - 2 * f, (h1 - h0) - 2 * f
    ESPELHO['linhas'] = int(round(TAM_DO_ESPELHO * alt / larg))
    v0 = 1.0 - ESPELHO['linhas'] / TAM_DO_ESPELHO
    pts = [R.p(a0 + f, h0 + f, 0.095), R.p(a1 - f, h0 + f, 0.095), R.p(a1 - f, h1 - f, 0.095), R.p(a0 + f, h1 - f, 0.095)]
    quad(MALHAS['espelho'], *pts, BRANCO, n=R.d(0, 0, 1), uv=[(0, v0), (1, v0), (1, 1), (0, 1)])


ESPELHO = {}
TAM_DO_ESPELHO = 512


PRETO_DE_BORRACHA = (0.07, 0.07, 0.07)
CROMO = (0.62, 0.64, 0.67)
CROMO_BRILHO = (0.97, 0.97, 0.98)


def montar_elevador():
    """O elevador de uma coluna: base com faixa zebrada, coluna cromada com colar vermelho, a cabeça e
    dois braços longos com pontas pretas e amarelas e sapatas de borracha (a colisão é uma caixa)"""
    t, s = ELEVADOR_TS
    c = diag(t, s)
    R = Ref(c, DIAG_D, DIAG_N)
    up = (0, 1, 0)
    caixa(CORES, R, -0.62, 0.62, 0, 0.04, -0.62, 0.62, METAL_ESCURO, sem=('baixo',))
    # a faixa zebrada em volta da base (blocos amarelos e pretos; cada lado fica com um canto)
    for lado in range(4):
        ang = lado * math.pi / 2
        ca, sa = math.cos(ang), math.sin(ang)
        for j in range(7):
            s0, s1 = -0.62 + j * 0.16, -0.62 + (j + 1) * 0.16
            cor = FAIXA_AMARELA if j % 2 == 0 else PRETO_DE_BORRACHA
            q = [(s0, 0.50), (s1, 0.50), (s1, 0.62), (s0, 0.62)]
            CORES.face([R.p(a * ca - b * sa, 0.042, a * sa + b * ca) for a, b in q], cor, n=up)
    torno(CORES, R.p(0, 0.04, 0), up, R.d(1, 0, 0), [(0.27, 0.0), (0.27, 0.04), (0.20, 0.08), (0.18, 0.12), (0.18, 1.08),
                                                        (0.215, 1.12), (0.215, 1.20)], 24,
          verniz((c[0], c[1]), PISO_Y, 1.2, base=CROMO, realce=CROMO_BRILHO, largura=0.35))
    torno(CORES, R.p(0, 0.78, 0), up, R.d(1, 0, 0), [(0.20, 0.0), (0.205, 0.02), (0.205, 0.12), (0.20, 0.14)], 24, VERMELHO)
    caixa(CORES, R, -0.30, 0.30, 1.20, 1.32, -0.30, 0.30, METAL_ESCURO)
    for a in (-0.40, 0.40):
        caixa(CORES, R, a - 0.07, a + 0.07, 1.22, 1.32, -0.80, 0.80, CROMO)
        for ponta in (-1, 1):
            for j in range(3):
                b0 = ponta * (0.80 + j * 0.10)
                b1 = ponta * (0.80 + (j + 1) * 0.10)
                caixa(CORES, R, a - 0.07, a + 0.07, 1.22, 1.32, min(b0, b1), max(b0, b1),
                      FAIXA_AMARELA if j % 2 == 0 else PRETO_DE_BORRACHA)
            torno(CORES, R.p(a, 1.32, ponta * 1.0), up, R.d(1, 0, 0), [(0.09, 0.0), (0.09, 0.06), (0.0, 0.06)], 12, PRETO_DE_BORRACHA)
    COLISAO_CAIXAS.append(('caixa', R, -0.6, 0.6, -1.3, 1.3))


def montar_maquina():
    W = P_ARCO_OESTE
    c = W.p0 + MAQUINA_U * W.R.u + 0.62 * W.R.n
    n = W.R.n
    R = Ref(c, (-n[1], n[0]), n)
    caixa(CORES, R, -0.45, 0.45, 0, 0.08, -0.35, 0.35, METAL_ESCURO, sem=('baixo',))
    caixa(CORES, R, -0.42, 0.42, 0.08, 0.86, -0.32, 0.32, BEGE)
    # o painel inclinado em cima
    P = R.p
    quad(CORES, P(-0.42, 0.86, 0.32), P(0.42, 0.86, 0.32), P(0.42, 1.05, -0.32), P(-0.42, 1.05, -0.32), (0.80, 0.72, 0.55), n=R.d(0, 1, 0.5))
    quad(CORES, P(-0.42, 0.86, -0.32), P(0.42, 0.86, -0.32), P(0.42, 1.05, -0.32), P(-0.42, 1.05, -0.32), BEGE, n=R.d(0, 0, -1))
    for a in (-0.42, 0.42):
        CORES.face([P(a, 0.86, 0.32), P(a, 0.86, -0.32), P(a, 1.05, -0.32)], BEGE, n=R.d(1 if a > 0 else -1, 0, 0))
    quad(CORES, P(-0.25, 0.92, 0.17), P(0.25, 0.92, 0.17), P(0.25, 1.0, -0.08), P(-0.25, 1.0, -0.08), METAL_ESCURO, n=R.d(0, 1, 0.5))
    # o eixo e a capa do lado
    torno(CORES, R.p(0.42, 0.62, 0), R.d(1, 0, 0), (0, 1, 0), [(0.05, 0.0), (0.05, 0.38), (0.0, 0.38)], 14, METAL)
    torno(CORES, R.p(0.42, 0.62, 0), R.d(1, 0, 0), (0, 1, 0), [(0.32, 0.12), (0.34, 0.16), (0.34, 0.30), (0.30, 0.34)], 20, METAL_ESCURO,
          ang=(0.0, math.pi))
    COLISAO_CAIXAS.append(('caixa', R, -0.6, 0.85, -0.45, 0.45))


def montar_bancos():
    for k, (x, z) in enumerate(BANCOS):
        R = Ref((x, z), (1, 0), (0, 1))
        e1 = (math.cos(k * 1.3), 0, math.sin(k * 1.3))
        pneu(R.p(0, 0.20, 0), (0, 1, 0), e1, 0.47, 0.40, 0.21, tipo='branco', n_seg=24, assento=True)
        COLISAO_CAIXAS.append(('coluna', x, z, 0.5))


def montar_podio():
    """O pódio redondo do salão da frente: uma pilha de pneus de faixa branca e três pneus em pé em volta"""
    x, z = CENTRO_DO_SALAO
    porta = diag((PORTA_T[0] + PORTA_T[1]) / 2, RECUO_DA_DIAGONAL)
    n = unit(porta - np.array([x, z]))
    R = Ref((x, z), (-n[1], n[0]), n)
    torno(CORES, R.p(0, 0, 0), (0, 1, 0), (1, 0, 0), [(1.25, 0.0), (1.25, 0.24), (1.20, 0.30)], 40, VINHO_ESCURO)
    disco(CORES, R.p(0, 0.30, 0), (0, 1, 0), (1, 0, 0), 1.20, 40, VERMELHO_CORTINA)
    topo = pilha_de_pneus(R, 0, 0.30, 0, 3, Rf=0.42, W=0.22, Rd=0.23, tipo='faixa_branca', tinta=BRANCO, n_seg=20)
    pneu_em_pe(R, 0, topo, 0, Rf=0.42, W=0.24, Rd=0.22, tipo='branco', n_seg=22)
    plaquinha(R.p(0, topo + 0.12, 0.30), R.d(1, 0, 0), R.d(0, 0, 1), 0.34, 0.2, 'Fettuccini', 'Sport')
    for k in range(3):
        ang = math.radians(90 + 120 * k)
        d = np.cos(ang) * R.n + np.sin(ang) * R.u
        Rk = Ref(np.array([x, z]) + d * 0.80, (-d[1], d[0]), d)
        pneu_em_pe(Rk, 0, 0.30, 0, Rf=0.38, W=0.22, Rd=0.20, tipo=('preto', 'faixa_branca', 'preto')[k], n_seg=18)
    COLISAO_CAIXAS.append(('coluna', x, z, 1.32))


montar_porta_enrolada()
montar_podio()
montar_espelho()
montar_elevador()
montar_maquina()
montar_bancos()


# ------------------------------------------------------------------
# 3g. Os pôsteres nas paredes
# ------------------------------------------------------------------
MOLDURAS = [(0.07, 0.06, 0.06), (0.24, 0.12, 0.06), DOURADO, VINHO_ESCURO, (0.70, 0.70, 0.68), (0.07, 0.06, 0.06)]
ASPECTO = {t: (CELULAS[[c[0] for c in CELULAS].index(t)][4] - 2 * BORDA_DA_CELULA - 2) /
              (CELULAS[[c[0] for c in CELULAS].index(t)][3] - 2 * BORDA_DA_CELULA - 2) for t in ('retrato', 'paisagem', 'foto')}
ALTO_DOS_POSTERES = ALTURA - SANCA - 0.08     # o alto dos quadros (acima do piso), logo abaixo da sanca
BAIXO_DOS_POSTERES = 1.05
VAO_ENTRE_POSTERES = 0.18                    # a folga entre um quadro e outro (no máximo 0,25 m)
ALTURA_MINIMA = 1.2                          # pôster de verdade (as fotos só tapam buracos)


def moldura_de(tipo):
    return 0.05 if tipo != 'foto' else 0.04


def altura_do_quadro(tipo, largura, passe):
    """A altura por fora da moldura de um quadro de 'largura' (por fora)"""
    m = moldura_de(tipo)
    return (largura - 2 * (m + passe)) * ASPECTO[tipo] + 2 * (m + passe)


def largura_do_quadro(tipo, altura, passe):
    m = moldura_de(tipo)
    return (altura - 2 * (m + passe)) / ASPECTO[tipo] + 2 * (m + passe)


def poster(W, a_meio, h_meio, largura, celula, rng, passe=0.0, b0=0.0, espelhar=False):
    """Um pôster emoldurado na parede W (largura da moldura por fora; a altura vem da célula).
    espelhar: a imagem invertida na horizontal (só nas fotos, que não têm letras)"""
    tipo = CELULAS[celula][0]
    moldura = moldura_de(tipo)
    altura = altura_do_quadro(tipo, largura, passe)
    R = W.R
    a0, a1 = a_meio - largura / 2, a_meio + largura / 2
    h0, h1 = h_meio - altura / 2, h_meio + altura / 2
    cor = MOLDURAS[int(rng.integers(len(MOLDURAS)))]
    e = 0.045
    b = b0 + 0.002
    # moldura: quatro caixas
    caixa(CORES, R, a0, a1, h0, h0 + moldura, b, b + e, cor, sem=('tras',), passo=1.2)
    caixa(CORES, R, a0, a1, h1 - moldura, h1, b, b + e, cor, sem=('tras',), passo=1.2)
    caixa(CORES, R, a0, a0 + moldura, h0 + moldura, h1 - moldura, b, b + e, cor, sem=('tras', 'cima', 'baixo'), passo=1.2)
    caixa(CORES, R, a1 - moldura, a1, h0 + moldura, h1 - moldura, b, b + e, cor, sem=('tras', 'cima', 'baixo'), passo=1.2)
    if passe > 0:
        quad(CORES, R.p(a0 + moldura, h0 + moldura, b + 0.01), R.p(a1 - moldura, h0 + moldura, b + 0.01),
             R.p(a1 - moldura, h1 - moldura, b + 0.01), R.p(a0 + moldura, h1 - moldura, b + 0.01), CREME, n=R.d(0, 0, 1), div=(1, 1))
    u0, v0, u1, v1 = uv_da_celula(celula)
    if espelhar:
        u0, u1 = u1, u0
    c0, c1 = a0 + moldura + passe, a1 - moldura - passe
    d0, d1 = h0 + moldura + passe, h1 - moldura - passe
    quad(MALHAS['posteres'], R.p(c0, d0, b + 0.015), R.p(c1, d0, b + 0.015), R.p(c1, d1, b + 0.015), R.p(c0, d1, b + 0.015),
         BRANCO, n=R.d(0, 0, 1), uv=[(u0, v0), (u1, v0), (u1, v1), (u0, v1)],
         div=(max(1, round((c1 - c0) / 0.25)), max(1, round((d1 - d0) / 0.25))))
    return altura


def passe_do_poster(tipo, rng):
    return 0.05 if tipo == 'foto' or rng.uniform() < 0.3 else 0.0


class Sorteio:
    """Dá as células dos pôsteres sem repetir as últimas (as fotos repetidas saem espelhadas)"""

    def __init__(self, rng):
        self.rng = rng
        self.ultimos = []
        self.usos = {}

    def pegar(self, tipo):
        todas = [k for k, c in enumerate(CELULAS) if c[0] == tipo]
        menos = min(self.usos.get(k, 0) for k in todas)
        opcoes = [k for k in todas if k not in self.ultimos and self.usos.get(k, 0) <= menos + 1]
        if not opcoes:
            opcoes = [k for k in todas if k not in self.ultimos] or todas
        k = int(self.rng.choice(opcoes))
        self.ultimos = (self.ultimos + [k])[-14:]
        self.usos[k] = self.usos.get(k, 0) + 1
        espelhar = tipo == 'foto' and self.usos[k] % 2 == 0
        return k, espelhar


def montar_coluna(larg_max, H, rng):
    """Escolhe os quadros de uma coluna (de cima para baixo) que cabem em larg_max x H e a enchem.
    Devolve [(tipo, largura, passe, altura)], a largura da coluna e a folga vertical entre eles."""
    combos = []
    if larg_max >= 1.75:
        combos += [('retrato', 'paisagem'), ('paisagem', 'retrato'), ('paisagem', 'paisagem', 'paisagem'),
                   ('retrato', 'paisagem', 'foto'), ('paisagem', 'retrato', 'foto'), ('paisagem', 'paisagem', 'foto')]
    combos += [('retrato', 'retrato'), ('retrato', 'retrato', 'foto'), ('retrato', 'retrato', 'retrato'), ('foto', 'retrato', 'retrato')]
    boas = []
    for combo in combos:
        itens = []
        for t in combo:
            ps = passe_do_poster(t, rng)
            if t == 'retrato':
                w = min(larg_max, rng.uniform(1.30, 1.60))
            elif t == 'paisagem':
                w = larg_max * rng.uniform(0.92, 1.0)
            else:
                w = min(larg_max * 0.75, rng.uniform(0.75, 0.95))
            itens.append([t, w, ps, altura_do_quadro(t, w, ps)])
        vao = VAO_ENTRE_POSTERES
        livre = H - vao * (len(itens) - 1) - 0.06
        total = sum(i[3] for i in itens)
        esc = min(1.0, livre / total)
        ok = True
        for i in itens:
            i[1] = largura_do_quadro(i[0], i[3] * esc, i[2])
            i[3] = i[3] * esc
            if (i[0] != 'foto' and i[3] < ALTURA_MINIMA) or (i[0] == 'foto' and i[3] < 0.42):
                ok = False
        cheio = (sum(i[3] for i in itens) + vao * (len(itens) - 1)) / H
        if ok and cheio > 0.84:
            boas.append((cheio, itens))
    if not boas:
        # uma pilha de retratos (ou de fotos, se a coluna é estreita) até encher
        tipo = 'retrato' if altura_do_quadro('retrato', larg_max, 0) >= ALTURA_MINIMA else 'foto'
        w = larg_max
        h = altura_do_quadro(tipo, w, 0.0)
        n = max(1, int((H + VAO_ENTRE_POSTERES) // (h + VAO_ENTRE_POSTERES)))
        if n == 1 and h > H:
            h = H - 0.06
            w = largura_do_quadro(tipo, h, 0.0)
        boas = [(1.0, [[tipo, w, 0.0, h] for _ in range(n)])]
    pesos = np.array([c for c, _ in boas]) ** 4
    _, itens = boas[int(rng.choice(len(boas), p=pesos / pesos.sum()))]
    return itens, max(i[1] for i in itens)


def pendurar(W, ua, ub, ha, hb, rng, sorteio):
    """Enche o trecho da parede (de ua a ub, de ha a hb acima do piso) de colunas de quadros, quase
    encostados, como no filme"""
    H = hb - ha
    if ub - ua < 0.55 or H < 0.4:
        return 0
    if H < ALTURA_MINIMA + 0.1:
        return fileira(W, ua, ub, ha, hb, rng, sorteio)
    colunas = []
    u = ua
    g = VAO_ENTRE_POSTERES
    while ub - u >= 0.80:
        resta = ub - u
        if resta <= 2.2:
            alvo = resta
        elif resta < 2.2 + g + 0.80:
            alvo = (resta - g) / 2
        else:
            alvo = rng.uniform(1.25, 2.1)
        itens, larg = montar_coluna(alvo, H, rng)
        colunas.append((itens, larg))
        u += larg + g
    if not colunas and ub - ua >= 0.55:
        # um vão estreito: uma coluna de fotos
        w = ub - ua - 0.04
        h = altura_do_quadro('foto', w, 0.05)
        n = max(1, int((H + g) // (h + g)))
        colunas.append(([['foto', w, 0.05, h] for _ in range(n)], w))
    if not colunas:
        return 0
    usado = sum(l for _, l in colunas) + g * (len(colunas) - 1)
    sobra = (ub - ua) - usado
    extra = min(sobra / (len(colunas) + 1), 0.10)
    u = ua + (sobra - extra * (len(colunas) - 1)) / 2
    n = 0
    for itens, larg in colunas:
        total = sum(i[3] for i in itens)
        vao = min(0.25, (H - total) / max(len(itens) - 1, 1)) if len(itens) > 1 else 0
        vao = max(vao, 0.10)
        bloco = total + vao * (len(itens) - 1)
        h = hb - (H - bloco) / 2 + rng.uniform(-0.5, 0.5) * max(H - bloco, 0) * 0.5
        h = min(h, hb)
        for t, w, ps, a in itens:
            cel, esp = sorteio.pegar(t)
            desl = rng.uniform(-0.5, 0.5) * (larg - w)
            poster(W, u + larg / 2 + desl, h - a / 2, w, cel, rng, ps, espelhar=esp)
            h -= a + vao
            n += 1
        u += larg + g + extra
    return n


def fileira(W, ua, ub, ha, hb, rng, sorteio):
    """Uma fileira baixa de paisagens e fotos (acima dos móveis, do espelho e das janelas)"""
    H = hb - ha
    h = H - 0.06
    itens = []
    u = 0.0
    while True:
        t = 'paisagem' if rng.uniform() < 0.55 else 'foto'
        ps = passe_do_poster(t, rng) if h > 0.6 else 0.0
        w = largura_do_quadro(t, h, ps)
        if u + w > ub - ua:
            t, ps = 'foto', 0.0
            w = largura_do_quadro(t, h, ps)
            if u + w > ub - ua:
                break
        itens.append((t, w, ps))
        u += w + 0.16
    if not itens:
        return 0
    usado = sum(w for _, w, _ in itens) + 0.16 * (len(itens) - 1)
    vao = 0.16 + min(((ub - ua) - usado) / max(len(itens) - 1, 1), 0.10) if len(itens) > 1 else 0
    usado = sum(w for _, w, _ in itens) + vao * (len(itens) - 1)
    u = ua + ((ub - ua) - usado) / 2
    for t, w, ps in itens:
        cel, esp = sorteio.pegar(t)
        poster(W, u + w / 2, ha + H / 2, w, cel, rng, ps, espelhar=esp)
        u += w + vao
    return len(itens)


def trechos_livres(W, bloqueios, margem=0.15):
    """Os trechos [u0, u1] da parede que sobram fora dos bloqueios"""
    livres = [(margem, W.L - margem)]
    for a, b in bloqueios:
        novos = []
        for c, d in livres:
            if b <= c or a >= d:
                novos.append((c, d))
                continue
            if a > c:
                novos.append((c, a))
            if b < d:
                novos.append((b, d))
        livres = novos
    return [(a, b) for a, b in livres if b - a > 0.5]


def bloqueio_de_coluna(W, x, z, raio=0.62):
    """O trecho da parede W tapado por uma coluna em (x, z), se ela encosta na parede"""
    d = float((np.array([x, z]) - W.p0) @ W.R.n)
    if abs(d) > 0.8:
        return None
    u = W.u_de(x, z)
    return (u - raio, u + raio)


def montar_posteres():
    rng = np.random.default_rng(42)
    s = Sorteio(rng)
    BAIXO, ALTO = BAIXO_DOS_POSTERES, ALTO_DOS_POSTERES
    topo_dos_moveis = TOPO_DO_MOVEL + 0.55 + 0.07
    bloq = {W.nome: [] for W in PAREDES}
    em_cima = []           # (W, u0, u1, ha): fileiras acima de móveis e vãos
    for W in PAREDES:
        for x, z in COLUNAS_GRANDES:
            b = bloqueio_de_coluna(W, x, z)
            if b:
                bloq[W.nome].append(b)
        for v in W.vaos:
            folga = 0.45 if v.tipo == 'porta' else 0.26
            bloq[W.nome].append((v.u0 - folga, v.u1 + folga))
            if v.tipo == 'janela':
                alto_do_vao = max(y for _, y in v.contorno) - PISO_Y + 0.16 + 0.07
                em_cima.append((W, v.u0 - 0.2, v.u1 + 0.2, alto_do_vao))
    # o móvel do fundo, o espelho e as estantes
    m0, m1 = sorted((P_FUNDO.u_de(MOVEL_X[1], Z_FUNDO), P_FUNDO.u_de(MOVEL_X[0], Z_FUNDO)))
    bloq['fundo'].append((m0 - 0.15, m1 + 0.15))
    em_cima.append((P_FUNDO, m0, m1, topo_dos_moveis))
    e0, e1 = sorted((P_FUNDO.u_de(ESPELHO_X[1], Z_FUNDO), P_FUNDO.u_de(ESPELHO_X[0], Z_FUNDO)))
    bloq['fundo'].append((e0 - 0.14, e1 + 0.14))
    em_cima.append((P_FUNDO, e0 - 0.05, e1 + 0.05, 4.75 + 0.16 + 0.07))
    for W, a0, a1 in ESTANTES:
        bloq[W.nome].append((a0 - 0.18, a1 + 0.18))
        em_cima.append((W, a0, a1, topo_dos_moveis))
    n = 0
    for W in PAREDES:
        for ua, ub in trechos_livres(W, bloq[W.nome]):
            n += pendurar(W, ua, ub, BAIXO, ALTO, rng, s)
    for W, ua, ub, ha in em_cima:
        if ALTO - ha >= 0.42:
            n += fileira(W, ua, ub, ha, ALTO, rng, s)
    print('  quadros nas paredes: %d' % n)


montar_posteres()


# ------------------------------------------------------------------
# 3h. As luzes do teto (o que se vê; a luz de verdade é a do Cycles)
# ------------------------------------------------------------------
def posicoes_das_luzes():
    """As luzes embutidas do teto: (x, z, tipo, parede). As 'parede' ficam perto das paredes (o facho
    delas desenha os leques de luz nos pôsteres); as 'centro' fazem as poças de luz no piso."""
    pts = []
    for W in PAREDES:
        n = max(1, round((W.L - 1.6) / 2.6))
        for k in range(n):
            u = 0.8 + (W.L - 1.6) * (k + 0.5) / n
            if any(v.tipo == 'porta' and v.u0 - 0.4 < u < v.u1 + 0.4 for v in W.vaos):
                continue
            perto_de_movel = any(Wm is W and a0 - 0.3 < u < a1 + 0.3 for Wm, a0, a1 in ESTANTES)
            if W is P_FUNDO:
                m0, m1 = sorted((W.u_de(MOVEL_X[1], Z_FUNDO), W.u_de(MOVEL_X[0], Z_FUNDO)))
                perto_de_movel = perto_de_movel or m0 - 0.3 < u < m1 + 0.3
            xz = W.p0 + u * W.R.u + (1.55 if perto_de_movel else 0.65) * W.R.n
            pts.append((xz[0], xz[1], 'parede', W))
    for x in (460.40, 464.90):
        for z in (150.80, 155.30, 159.80, 164.30, 168.80, 172.60):
            pts.append((x, z, 'centro', None))
    for x in (473.50, 478.50, 483.50):
        pts.append((x, 172.65, 'centro', None))
    for t in (4.2, 8.0):
        xz = diag(t, 4.6)
        pts.append((xz[0], xz[1], 'centro', None))
    bons = []
    for x, z, tipo, W in pts:
        if dentro_do_contorno(x, z) and all(dentro_do_contorno(x + dx, z + dz) for dx, dz in ((0.5, 0), (-0.5, 0), (0, 0.5), (0, -0.5))):
            bons.append((x, z, tipo, W))
    return bons


LUZES = posicoes_das_luzes()
for x, z, _, _ in LUZES:
    c = (x, TETO_Y - 0.012, z)
    disco(CORES, c, (0, -1, 0), (1, 0, 0), 0.18, 16, BRANCO, brilho=LUZ_DA_LAMPADA)
    torno(CORES, (x, TETO_Y, z), (0, -1, 0), (1, 0, 0), [(0.25, 0.0), (0.25, 0.018), (0.18, 0.018)], 16, (0.62, 0.56, 0.48))
print('  luzes do teto: %d' % len(LUZES))
print('  %.0f s' % (time.time() - INICIO))


# ------------------------------------------------------------------
# 4. Objetos, materiais e a luz assada
# ------------------------------------------------------------------
def criar_objetos():
    objs = []
    for chave, m in MALHAS.items():
        for k, bm in enumerate(m.bms):
            if len(bm.faces) == 0:
                continue
            me = bpy.data.meshes.new('%s_%d' % (m.nome, k + 1))
            bm.to_mesh(me)
            bm.free()
            o = bpy.data.objects.new(me.name, me)
            bpy.context.scene.collection.objects.link(o)
            o['malha'] = chave
            objs.append(o)
    for chave in MALHAS:
        t = sum(len(o.data.loop_triangles) if o.data.calc_loop_triangles() is None else 0 for o in objs if o['malha'] == chave)
        print('  %-10s %7d triângulos' % (chave, t))
    return objs


def material_de_assar(m):
    mat = novo_material('assar_' + m.nome)
    nt = mat.node_tree
    nt.nodes.clear()
    saida = nt.nodes.new('ShaderNodeOutputMaterial')
    tinta = nt.nodes.new('ShaderNodeAttribute')
    tinta.attribute_name = 'tinta'
    brilho = nt.nodes.new('ShaderNodeAttribute')
    brilho.attribute_name = 'brilho'
    cor = tinta.outputs['Color']
    if m.imagem is not None:
        tex = nt.nodes.new('ShaderNodeTexImage')
        tex.image = m.imagem
        mult = nt.nodes.new('ShaderNodeMix')
        mult.data_type = 'RGBA'
        mult.blend_type = 'MULTIPLY'
        mult.inputs['Factor'].default_value = 1.0
        nt.links.new(tinta.outputs['Color'], mult.inputs['A'])
        nt.links.new(tex.outputs['Color'], mult.inputs['B'])
        cor = mult.outputs['Result']
    if m.nome == 'loja_piso':
        # o piso é encerado (o reflexo se assa à parte); a luz que ele rebate vai menos tingida de
        # vermelho (senão o teto e as paredes ficam cor de tijolo)
        hs = nt.nodes.new('ShaderNodeHueSaturation')
        hs.inputs['Saturation'].default_value = 0.4
        nt.links.new(cor, hs.inputs['Color'])
        bsdf = nt.nodes.new('ShaderNodeBsdfPrincipled')
        bsdf.inputs['Roughness'].default_value = 0.22
        nt.links.new(hs.outputs['Color'], bsdf.inputs['Base Color'])
    else:
        bsdf = nt.nodes.new('ShaderNodeBsdfDiffuse')
        nt.links.new(cor, bsdf.inputs['Color'])
    em = nt.nodes.new('ShaderNodeEmission')
    nt.links.new(brilho.outputs['Color'], em.inputs['Color'])
    # as lâmpadas só brilham para a frente (para baixo): sem o halo nas costas, no teto
    geo = nt.nodes.new('ShaderNodeNewGeometry')
    frente = nt.nodes.new('ShaderNodeMath')
    frente.operation = 'MULTIPLY_ADD'
    frente.inputs[1].default_value = -6.0
    frente.inputs[2].default_value = 6.0
    nt.links.new(geo.outputs['Backfacing'], frente.inputs[0])
    nt.links.new(frente.outputs[0], em.inputs['Strength'])
    soma = nt.nodes.new('ShaderNodeAddShader')
    nt.links.new(bsdf.outputs[0], soma.inputs[0])
    nt.links.new(em.outputs[0], soma.inputs[1])
    nt.links.new(soma.outputs[0], saida.inputs['Surface'])
    return mat


def b_de(p):
    return Vector((p[0], -p[2], p[1]))


CEU_DO_ASSADO = 0.8          # o céu (azul) que entra pelas aberturas
SOL_DO_ASSADO = 2.4          # o sol (W/m²)
FOCO_DA_PAREDE = 900.0       # os focos do teto perto das paredes (W)
FOCO_DO_CENTRO = 420.0       # os do meio da sala
COR_DAS_LUZES = (1.0, 0.70, 0.46)    # ~3000 K
LUZ_DA_PRATELEIRA = 9.0      # W por metro, nas luzinhas dos compartimentos
LUZ_DA_SANCA = 2.5           # W por metro
LUZ_DO_REBATIDO = 0.3        # W/m², a luz fraca virada para o teto (o rebatido claro do piso)


def montar_luzes_do_cycles():
    sc = bpy.context.scene
    w = novo_mundo('ceu_da_loja')
    sc.world = w
    bg = w.node_tree.nodes['Background']
    bg.inputs[0].default_value = (0.50, 0.66, 0.95, 1)
    bg.inputs[1].default_value = CEU_DO_ASSADO
    # o sol, entrando pelas aberturas (vem do lado do pátio e da rua)
    sol = bpy.data.lights.new('sol', 'SUN')
    sol.energy = SOL_DO_ASSADO
    sol.color = (1.0, 0.93, 0.82)
    sol.angle = math.radians(2.0)
    so = bpy.data.objects.new('sol', sol)
    sc.collection.objects.link(so)
    direcao = b_de(unit((-0.62, -0.55, 0.56)))          # para onde a luz vai (eixos do jogo -> Blender)
    so.rotation_euler = direcao.to_track_quat('-Z', 'Y').to_euler()
    # as luzes do teto: focos quentes (~3000 K) de facho fechado; as das paredes, um pouco viradas para
    # elas, desenham os leques de luz
    for k, (x, z, tipo, W) in enumerate(LUZES):
        L = bpy.data.lights.new('teto_%d' % k, 'SPOT')
        L.energy = FOCO_DA_PAREDE if tipo == 'parede' else FOCO_DO_CENTRO
        L.spot_size = math.radians(56 if tipo == 'parede' else 100)
        L.spot_blend = 0.30 if tipo == 'parede' else 0.6
        L.shadow_soft_size = 0.10
        L.color = COR_DAS_LUZES
        o = bpy.data.objects.new(L.name, L)
        o.location = b_de((x, TETO_Y - 0.05, z))
        d = np.array([0, -1.0, 0])
        if W is not None:
            d = unit(d - math.tan(math.radians(7)) * np.array([W.R.n[0], 0, W.R.n[1]]))
        o.rotation_euler = b_de(d).to_track_quat('-Z', 'Y').to_euler()
        sc.collection.objects.link(o)
    # as luzinhas escondidas nos compartimentos das prateleiras (só no assado)
    for k, (c, ao_longo, larg, emite) in enumerate(LUZES_DAS_PRATELEIRAS):
        L = bpy.data.lights.new('prateleira_%d' % k, 'AREA')
        L.shape = 'RECTANGLE'
        L.size = larg
        L.size_y = 0.05
        L.energy = LUZ_DA_PRATELEIRA * larg
        L.color = (1.0, 0.72, 0.45)
        o = bpy.data.objects.new(L.name, L)
        o.location = b_de(c)
        x = b_de(np.asarray(ao_longo, float))
        zz = -b_de(np.asarray(emite, float))
        y = zz.cross(x)
        o.rotation_euler = Matrix((x, y, zz)).transposed().to_euler()
        sc.collection.objects.link(o)
    # a luz escondida na sanca, lavando o teto (âmbar e fraca: é um brilho, não uma fita de LED)
    for k, W in enumerate(PAREDES):
        comp = W.L - 0.8
        if comp < 0.5:
            continue
        L = bpy.data.lights.new('sanca_%d' % k, 'AREA')
        L.shape = 'RECTANGLE'
        L.size = comp
        L.size_y = 0.06
        L.energy = LUZ_DA_SANCA * comp
        L.color = (1.0, 0.62, 0.32)
        o = bpy.data.objects.new(L.name, L)
        o.location = b_de(W.R.p(W.L / 2, ALTURA - SANCA + 0.16, 0.16))
        x = b_de(np.array([W.R.u[0], 0, W.R.u[1]]))
        emite = b_de(unit(np.array([0, 1.0, 0]) + 0.55 * np.array([W.R.n[0], 0, W.R.n[1]])))
        z = -emite
        y = z.cross(x)
        o.rotation_euler = Matrix((x, y, z)).transposed().to_euler()
        sc.collection.objects.link(o)
    # o rebatido do chão no teto (o piso de verdade rebate a luz bem tingida de vermelho; esta luz fraca,
    # virada para cima, faz as vezes de um rebatido mais claro): o teto fica creme, não cor de tijolo
    for k, pc in enumerate(PECAS_DO_PISO):
        xs = [p[0] for p in pc]
        zs = [p[1] for p in pc]
        w, d = max(xs) - min(xs), max(zs) - min(zs)
        L = bpy.data.lights.new('rebatido_%d' % k, 'AREA')
        L.shape = 'RECTANGLE'
        L.size = w
        L.size_y = d
        L.energy = LUZ_DO_REBATIDO * w * d
        L.color = (1.0, 0.94, 0.86)
        o = bpy.data.objects.new(L.name, L)
        o.location = b_de(((min(xs) + max(xs)) / 2, PISO_Y + ALTURA - 0.9, (min(zs) + max(zs)) / 2))
        o.rotation_euler = (math.pi, 0, 0)
        sc.collection.objects.link(o)
    for k, p in enumerate(LUZES_DO_NICHO):
        L = bpy.data.lights.new('nicho_%d' % k, 'POINT')
        L.energy = 120 if k == 0 else 45
        L.color = (1.0, 0.42, 0.12)
        L.shadow_soft_size = 0.25
        o = bpy.data.objects.new(L.name, L)
        o.location = b_de(p)
        sc.collection.objects.link(o)


# A luz de noite (--noite): sem o sol; o céu vira um luar fraco, azul-arroxeado, entrando pelas aberturas; as
# luzes de dentro, todas acesas e mais presentes (os focos do teto, as luzinhas das prateleiras, a sanca e o
# nicho); o rebatido claro do piso no teto fica mais fraco (o teto e os cantos escurecem)
CEU_DA_NOITE = (0.30, 0.28, 0.62)
FORCA_DO_CEU_DA_NOITE = 0.04
NOITE_DOS_FOCOS = 1.0        # multiplica a potência das luzes de dia, por tipo
NOITE_DOS_FOCOS_DO_CENTRO = 1.4    # os do meio da sala, mais fechados: poças de luz no piso
ABERTURA_DO_CENTRO_DE_NOITE = math.radians(74)
NOITE_DA_PRATELEIRA = 2.2
NOITE_DA_SANCA = 2.0
NOITE_DO_NICHO = 2.0
NOITE_DO_REBATIDO = 0.15


def montar_luzes_da_noite():
    montar_luzes_do_cycles()
    sc = bpy.context.scene
    bg = sc.world.node_tree.nodes['Background']
    bg.inputs[0].default_value = (*CEU_DA_NOITE, 1)
    bg.inputs[1].default_value = FORCA_DO_CEU_DA_NOITE
    fatores = (('teto_', NOITE_DOS_FOCOS), ('prateleira_', NOITE_DA_PRATELEIRA), ('sanca_', NOITE_DA_SANCA),
               ('nicho_', NOITE_DO_NICHO), ('rebatido_', NOITE_DO_REBATIDO))
    for o in list(sc.objects):
        if o.type != 'LIGHT':
            continue
        if o.data.type == 'SUN':
            bpy.data.objects.remove(o, do_unlink=True)
            continue
        for prefixo, f in fatores:
            if o.name.startswith(prefixo):
                o.data.energy *= f
    for k, (x, z, tipo, W) in enumerate(LUZES):
        o = sc.objects.get('teto_%d' % k)
        if tipo == 'centro' and o is not None and o.type == 'LIGHT':
            o.data.energy *= NOITE_DOS_FOCOS_DO_CENTRO
            o.data.spot_size = ABERTURA_DO_CENTRO_DE_NOITE
            o.data.spot_blend = 0.45


def ligar_gpu():
    try:
        pr = bpy.context.preferences.addons['cycles'].preferences
        pr.compute_device_type = 'METAL'
        pr.get_devices()
        for d in pr.devices:
            d.use = True
        bpy.context.scene.cycles.device = 'GPU'
        return True
    except Exception as e:
        print('  sem GPU (%s): Cycles na CPU' % e)
        return False


def assar(objs):
    sc = bpy.context.scene
    sc.render.engine = 'CYCLES'
    ligar_gpu()
    sc.cycles.samples = AMOSTRAS
    sc.cycles.use_denoising = False
    sc.cycles.max_bounces = 6
    sc.cycles.diffuse_bounces = 4
    sc.cycles.glossy_bounces = 2
    for o in objs:
        m = MALHAS[o['malha']]
        o.data.materials.clear()
        o.data.materials.append(material_de_assar(m))
        at = o.data.color_attributes.new('luz', 'FLOAT_COLOR', 'CORNER')
        o.data.color_attributes.active_color = at
    bpy.ops.object.select_all(action='DESELECT')
    for o in objs:
        o.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]
    t = time.time()
    bpy.ops.object.bake(type='DIFFUSE', pass_filter={'DIRECT', 'INDIRECT'}, target='VERTEX_COLORS', use_clear=True)
    print('  luz difusa assada em %.0f s' % (time.time() - t))


def ler_atributo(me, nome):
    at = me.color_attributes.get(nome)
    a = np.zeros(len(me.loops) * 4, np.float32)
    if at is not None:
        at.data.foreach_get('color', a)
    return a.reshape(-1, 4)[:, :3]


def suavizar(me, valores):
    """Média dos cantos no mesmo ponto com a face virada mais ou menos para o mesmo lado (tira o ruído
    de cada amostra sem borrar as quinas)"""
    n = len(me.loops)
    vi = np.zeros(n, np.int64)
    me.loops.foreach_get('vertex_index', vi)
    co = np.zeros(len(me.vertices) * 3)
    me.vertices.foreach_get('co', co)
    co = co.reshape(-1, 3)[vi]
    fn = np.zeros(len(me.polygons) * 3)
    me.polygons.foreach_get('normal', fn)
    fn = fn.reshape(-1, 3)
    li = np.zeros(len(me.polygons), np.int64)
    lt = np.zeros(len(me.polygons), np.int64)
    me.polygons.foreach_get('loop_start', li)
    me.polygons.foreach_get('loop_total', lt)
    nl = np.repeat(fn, lt, axis=0)
    chave = np.round(co * 500).astype(np.int64)
    _, grupo = np.unique(chave, axis=0, return_inverse=True)
    grupo = grupo.ravel()
    ordem = np.argsort(grupo, kind='stable')
    g_ord = grupo[ordem]
    cortes = np.flatnonzero(np.diff(g_ord)) + 1
    saida = valores.copy()
    for idx in np.split(ordem, cortes):
        if len(idx) < 2:
            continue
        nn = nl[idx]
        perto = (nn @ nn.T) > 0.70
        v = valores[idx]
        saida[idx] = (perto.astype(np.float32) @ v) / perto.sum(1, keepdims=True)
    return saida


AMBIENTE = 0.008         # quase nada de luz por igual: os cantos e o teto ficam mais escuros que as poças de luz
PERCENTIL_DA_LUZ = 98    # a luz é normalizada para este percentil (das superfícies sem brilho próprio)...
ALVO_DA_LUZ = 0.95       # ...ficar com este valor na tela
BASE_DO_PISO = 0.45      # a mediana da luz do piso na tela (o reflexo falso vem por cima)
SATURACAO_DA_LUZ = 0.55  # quanto da cor da luz (quente, e o rebatido vermelho do piso) fica na cor do vértice
SATURACAO_DO_PISO = 0.35 # no piso, menos ainda: a cor vem da madeira
COR_DO_ESPELHO = (0.93, 0.93, 0.93)   # a cor do vértice do espelho (a textura já mostra a sala iluminada)
CONTRASTE = 1.0          # a luz na tela (sem a cor) vai a esta potência (de noite: os cantos mais escuros)
if NOITE:
    # as mesmas contas, com a noite: as poças quentes dos focos no alto da escala, o resto mais escuro
    AMBIENTE = 0.002
    BASE_DO_PISO = 0.26
    SATURACAO_DA_LUZ = 0.80
    SATURACAO_DO_PISO = 0.50
    CONTRASTE = 1.50
    COR_DO_ESPELHO = (0.52, 0.44, 0.37)   # o reflexo (de dia, na textura) da sala à luz quente das lâmpadas


def dados_dos_cantos(me):
    """Por canto: o índice da face, a posição, a normal da face e se a face é 'suave'"""
    nf = len(me.polygons)
    lt = np.zeros(nf, np.int64)
    ls = np.zeros(nf, np.int64)
    me.polygons.foreach_get('loop_total', lt)
    me.polygons.foreach_get('loop_start', ls)
    vi = np.zeros(len(me.loops), np.int64)
    me.loops.foreach_get('vertex_index', vi)
    co = np.zeros(len(me.vertices) * 3)
    me.vertices.foreach_get('co', co)
    fn = np.zeros(nf * 3)
    me.polygons.foreach_get('normal', fn)
    suave = np.zeros(nf, np.int32)
    at = me.attributes.get('suave')
    if at is not None:
        at.data.foreach_get('value', suave)
    face = np.repeat(np.arange(nf), lt)
    return dict(face=face, ls=ls, lt=lt, co=co.reshape(-1, 3)[vi], n=fn.reshape(-1, 3)[face],
                suave=suave.astype(bool)[face])


def alisar(me, valores, passadas=2, cantos=None):
    """Alisa (laplaciano) os valores dos cantos das faces 'suave', só entre vértices vizinhos (ligados
    por uma aresta) de mesma normal: tira as listras da luz interpolada nas paredes, no teto e no piso"""
    d = cantos or dados_dos_cantos(me)
    idx = np.flatnonzero(d['suave'])
    if len(idx) == 0:
        return valores
    chave = np.concatenate([np.round(d['co'][idx] * 500), np.round(d['n'][idx] * 20)], axis=1).astype(np.int64)
    _, no = np.unique(chave, axis=0, return_inverse=True)
    no = no.ravel()
    nn = int(no.max()) + 1
    no_do_canto = np.full(len(d['face']), -1, np.int64)
    no_do_canto[idx] = no
    f = d['face'][idx]
    s, t = d['ls'][f], d['lt'][f]
    prox = s + (idx - s + 1) % t
    a, b = no_do_canto[idx], no_do_canto[prox]
    val = np.zeros((nn, valores.shape[1]))
    cnt = np.zeros(nn)
    np.add.at(val, no, valores[idx])
    np.add.at(cnt, no, 1)
    val /= np.maximum(cnt, 1)[:, None]
    for _ in range(passadas):
        soma = np.zeros_like(val)
        grau = np.zeros(nn)
        np.add.at(soma, a, val[b])
        np.add.at(soma, b, val[a])
        np.add.at(grau, a, 1)
        np.add.at(grau, b, 1)
        tem = grau > 0
        val[tem] = 0.5 * val[tem] + 0.5 * soma[tem] / grau[tem, None]
    saida = valores.copy()
    saida[idx] = val[no]
    return saida


def joelho(x):
    # até 0,82 reta; acima, encosta devagar em 1 (os 8 bits cortam acima de 1)
    return np.where(x < 0.82, x, 0.82 + 0.18 * (1 - np.exp(-(x - 0.82) / 0.18)))


LUMA = np.array([0.2126, 0.7152, 0.0722])


def finalizar_cores(objs):
    """A luz assada (linear) vira a cor do vértice: alisada, normalizada, na escala de tela, vezes a tinta"""
    dados = []
    lums = []
    for o in objs:
        me = o.data
        cantos = dados_dos_cantos(me)
        luz = suavizar(me, ler_atributo(me, 'luz'))
        luz = alisar(me, luz, cantos=cantos)
        tinta = ler_atributo(me, 'tinta')
        brilho = ler_atributo(me, 'brilho')
        dados.append((o, luz, tinta, brilho, cantos))
        if o['malha'] in ('cores', 'piso', 'asfalto'):
            ok = brilho.sum(1) == 0
            lums.append(luz[ok] @ LUMA)
    lum = np.concatenate(lums)
    ref = np.percentile(lum, PERCENTIL_DA_LUZ)
    escala = ALVO_DA_LUZ / max(ref, 1e-6)
    piso = np.concatenate([luz @ LUMA for o, luz, *_ in dados if o['malha'] == 'piso'])
    na_tela = lambda x: joelho(tela(x * escala + AMBIENTE)) if CONTRASTE == 1.0 else \
        np.power(joelho(tela(x * escala + AMBIENTE)), CONTRASTE)
    fator_do_piso = BASE_DO_PISO / max(float(np.median(na_tela(piso))), 1e-6)
    print('  luz: percentil %d = %.3f, escala %.3f; o piso x %.2f' % (PERCENTIL_DA_LUZ, ref, escala, fator_do_piso))
    medidas = {'piso': [], 'pôsteres': [], 'teto': [], 'pé das paredes': []}
    for o, luz, tinta, brilho, cantos in dados:
        me = o.data
        tela_luz = na_tela(luz)
        cinza = (tela_luz @ LUMA)[:, None]
        sat = SATURACAO_DO_PISO if o['malha'] == 'piso' else SATURACAO_DA_LUZ
        tela_luz = cinza + (tela_luz - cinza) * sat
        if o['malha'] == 'piso':
            tela_luz = np.clip(tela_luz * fator_do_piso, 0, 1)
        cor = tela_luz * tela(tinta)
        acesa = brilho.sum(1) > 0
        cor[acesa] = np.clip(tela(brilho[acesa]), 0, 1)
        if o['malha'] == 'espelho':
            cor[:] = COR_DO_ESPELHO    # o espelho mostra a sala já iluminada (na textura): não escurece com a luz de lá
        cor = np.clip(cor, 0, 1)
        # as medidas (a luz na tela, sem a tinta)
        lt = tela_luz @ LUMA
        if o['malha'] == 'piso':
            medidas['piso'].append(lt)
        elif o['malha'] == 'posteres':
            medidas['pôsteres'].append(lt)
        elif o['malha'] == 'cores':
            s = cantos['suave'] & ~acesa
            nz = cantos['n'][:, 2]
            alt = cantos['co'][:, 2] - PISO_Y
            medidas['pé das paredes'].append(lt[s & (np.abs(nz) < 0.1) & (alt < 0.35)])
            medidas['teto'].append(lt[s & (nz < -0.9)])
        if me.color_attributes.get('Col'):
            me.color_attributes.remove(me.color_attributes['Col'])
        at = me.color_attributes.new('Col', 'FLOAT_COLOR', 'CORNER')
        a = np.concatenate([cor, np.ones((len(cor), 1))], axis=1).astype(np.float32).ravel()
        at.data.foreach_set('color', a)
        me.color_attributes.active_color = me.color_attributes['Col']
        me.color_attributes.render_color_index = me.color_attributes.active_color_index
    for nome, v in medidas.items():
        v = np.concatenate(v) if v else np.zeros(1)
        print('  luz na tela, %-14s p10 %.2f  mediana %.2f  p90 %.2f' % (nome, *np.percentile(v, [10, 50, 90])))
    return escala


def material_de_previa(m):
    """O que o jogo mostra: textura × cor do vértice, sem luz (na tela: emissão de (t·c)^2,2)"""
    mat = novo_material('previa_' + m.nome)
    nt = mat.node_tree
    nt.nodes.clear()
    saida = nt.nodes.new('ShaderNodeOutputMaterial')
    at = nt.nodes.new('ShaderNodeAttribute')
    at.attribute_name = 'Col'
    gama = nt.nodes.new('ShaderNodeGamma')
    gama.inputs['Gamma'].default_value = 2.2
    nt.links.new(at.outputs['Color'], gama.inputs['Color'])
    cor = gama.outputs['Color']
    if m.imagem is not None:
        tex = nt.nodes.new('ShaderNodeTexImage')
        tex.image = m.imagem
        tex.interpolation = 'Linear'
        mult = nt.nodes.new('ShaderNodeMix')
        mult.data_type = 'RGBA'
        mult.blend_type = 'MULTIPLY'
        mult.inputs['Factor'].default_value = 1.0
        nt.links.new(cor, mult.inputs['A'])
        nt.links.new(tex.outputs['Color'], mult.inputs['B'])
        cor = mult.outputs['Result']
    em = nt.nodes.new('ShaderNodeEmission')
    nt.links.new(cor, em.inputs['Color'])
    nt.links.new(em.outputs[0], saida.inputs['Surface'])
    return mat


def material_de_exportar(m):
    mat = novo_material(m.nome)
    nt = mat.node_tree
    nt.nodes.clear()
    saida = nt.nodes.new('ShaderNodeOutputMaterial')
    bsdf = nt.nodes.new('ShaderNodeBsdfPrincipled')
    bsdf.inputs['Base Color'].default_value = (1, 1, 1, 1)
    bsdf.inputs['Roughness'].default_value = 0.8
    if m.imagem is not None:
        tex = nt.nodes.new('ShaderNodeTexImage')
        tex.image = m.imagem
        nt.links.new(tex.outputs['Color'], bsdf.inputs['Base Color'])
    nt.links.new(bsdf.outputs[0], saida.inputs['Surface'])
    return mat


def trocar_materiais(objs, fabrica):
    feitos = {}
    for o in objs:
        m = MALHAS[o['malha']]
        if m.nome not in feitos:
            feitos[m.nome] = fabrica(m)
        o.data.materials.clear()
        o.data.materials.append(feitos[m.nome])


print('Assando a luz (Cycles, %d amostras)...' % AMOSTRAS)
OBJS = criar_objetos()
if NOITE:
    montar_luzes_da_noite()
else:
    montar_luzes_do_cycles()
assar(OBJS)
finalizar_cores(OBJS)


# ------------------------------------------------------------------
# 4a. O brilho do piso encerado: um reflexo falso, somado nas cores dos vértices do piso
# ------------------------------------------------------------------
PESO_DO_REFLEXO = 0.68       # quanto da cor refletida (o que o jogo mostra no ponto atingido) se soma
PESO_DOS_FOCOS = 0.20        # o brilho embaixo de cada foco do meio da sala
OLHO_LONGE = 7.0             # o olho médio fica até 7 m do ponto, na direção do meio da sala...
OLHO_ALTO = 2.4              # ...e 2,4 m acima do piso (como a câmera atrás do carro)
RAIOS_DO_REFLEXO = 8         # raios por vértice (um cone estreito: o reflexo sai borrado)
CEU_REFLETIDO = (0.88, 0.89, 0.90)     # o que se vê pela porta e pelas janelas
ESPELHO_REFLETIDO = 0.45               # o espelho, visto no piso
if NOITE:
    CEU_REFLETIDO = (0.07, 0.07, 0.15)     # a noite lá fora
    ESPELHO_REFLETIDO = 0.22
    PESO_DOS_FOCOS = 0.24


def refletir_no_piso(objs):
    """Para cada vértice do piso, reflete o raio que vem de um olho médio, procura o que ele atinge na
    sala (pôsteres, janelas, móvel, nicho, luzes) e soma a cor final desse ponto (cor do vértice ×
    textura), borrada pelos vizinhos. O piso da textura é escuro: estes realces leem como o brilho
    do encerado."""
    from mathutils.bvhtree import BVHTree
    texturas = {'posteres': ATLAS, 'raiz': TEXTURAS['raiz'], 'pneus': TEXTURAS['pneus'], 'asfalto': TEXTURAS['asfalto']}
    V, T, C, U, X, lista = [], [], [], [], [], []
    base = 0
    for o in objs:
        chave = o['malha']
        if chave == 'piso':
            continue
        me = o.data
        me.calc_loop_triangles()
        n = len(me.loop_triangles)
        if n == 0:
            continue
        lp = np.zeros(n * 3, np.int64)
        vt = np.zeros(n * 3, np.int64)
        me.loop_triangles.foreach_get('loops', lp)
        me.loop_triangles.foreach_get('vertices', vt)
        co = np.zeros(len(me.vertices) * 3)
        me.vertices.foreach_get('co', co)
        col = ler_atributo(me, 'Col')
        if chave == 'espelho':
            col = np.full_like(col, ESPELHO_REFLETIDO)
        uv = np.zeros((len(me.loops), 2))
        if me.uv_layers:
            a = np.zeros(len(me.loops) * 2)
            me.uv_layers[0].data.foreach_get('uv', a)
            uv = a.reshape(-1, 2)
        V.append(co.reshape(-1, 3))
        T.append(vt.reshape(-1, 3) + base)
        base += len(me.vertices)
        C.append(col[lp].reshape(-1, 3, 3))
        U.append(uv[lp].reshape(-1, 3, 2))
        if chave in texturas:
            lista.append(texturas[chave])
            X.append(np.full(n, len(lista) - 1))
        else:
            X.append(np.full(n, -1))
    V, T, C, U, X = (np.concatenate(a) for a in (V, T, C, U, X))
    bvh = BVHTree.FromPolygons(V.tolist(), T.tolist(), all_triangles=True)
    rng = np.random.default_rng(7)
    jitter = rng.normal(size=(RAIOS_DO_REFLEXO, 3)) * 0.022
    centro = np.array([CENTRO_DA_SALA[0], -CENTRO_DA_SALA[1]])
    focos = [(x, -z) for x, z, tipo, _ in LUZES if tipo == 'centro']
    for o in objs:
        if o['malha'] != 'piso':
            continue
        me = o.data
        cantos = dados_dos_cantos(me)
        chave = np.round(cantos['co'] * 1000).astype(np.int64)
        uniq, inv = np.unique(chave, axis=0, return_inverse=True)
        inv = inv.ravel()
        pts = uniq / 1000.0
        alvos, onde, quem = [], [], []
        for k, p in enumerate(pts):
            d = centro - p[:2]
            dist = float(np.linalg.norm(d))
            olho = np.array([*(p[:2] + d * min(1.0, OLHO_LONGE / max(dist, 1e-6))), PISO_Y + OLHO_ALTO])
            i = p - olho
            r = unit(np.array([i[0], i[1], -i[2]]))
            origem = Vector((p[0], p[1], p[2] + 0.01))
            for j in jitter:
                rr = unit(r + j)
                loc, _, idx, _ = bvh.ray_cast(origem, Vector(rr), 80.0)
                if idx is not None:
                    alvos.append(idx)
                    onde.append(loc[:])
                    quem.append(k)
        refl = np.tile(np.array(CEU_REFLETIDO), (len(pts), 1)) * RAIOS_DO_REFLEXO
        if alvos:
            idx = np.array(alvos)
            loc = np.array(onde)
            quem = np.array(quem)
            a, b, c = V[T[idx, 0]], V[T[idx, 1]], V[T[idx, 2]]
            v0, v1, v2 = b - a, c - a, loc - a
            d00, d01, d11 = (v0 * v0).sum(1), (v0 * v1).sum(1), (v1 * v1).sum(1)
            d20, d21 = (v2 * v0).sum(1), (v2 * v1).sum(1)
            den = np.maximum(d00 * d11 - d01 * d01, 1e-12)
            w1 = (d11 * d20 - d01 * d21) / den
            w2 = (d00 * d21 - d01 * d20) / den
            w0 = 1 - w1 - w2
            pesos = np.stack([w0, w1, w2], 1)[..., None]
            cor = (C[idx] * pesos).sum(1)
            uv = (U[idx] * pesos).sum(1)
            for t, img in enumerate(lista):
                m = X[idx] == t
                if not m.any():
                    continue
                h, w = img.shape[:2]
                px = np.clip(((uv[m, 0] % 1.0) * w).astype(int), 0, w - 1)
                py = np.clip(((1.0 - uv[m, 1] % 1.0) * h).astype(int), 0, h - 1)
                cor[m] = cor[m] * img[py, px, :3]
            # os raios que acertaram tiram a parte do céu que foi posta de antemão
            np.add.at(refl, quem, cor - np.array(CEU_REFLETIDO))
        refl /= RAIOS_DO_REFLEXO
        refl = alisar(me, refl[inv], passadas=1, cantos=cantos)
        refl = np.clip(refl, 0, 1) ** 1.5          # mais contraste: o escuro reflete pouco, o claro brilha
        # o brilho embaixo dos focos do meio da sala
        brilho = np.zeros(len(inv))
        for fx, fy in focos:
            d2 = (cantos['co'][:, 0] - fx) ** 2 + (cantos['co'][:, 1] - fy) ** 2
            brilho += np.exp(-d2 / (2 * 0.55 ** 2))
        col = ler_atributo(me, 'Col')
        col = col + PESO_DO_REFLEXO * refl + PESO_DOS_FOCOS * brilho[:, None] * np.array([1.0, 0.86, 0.70])
        col = np.clip(col, 0, 1)
        at = me.color_attributes['Col']
        at.data.foreach_set('color', np.concatenate([col, np.ones((len(col), 1))], axis=1).astype(np.float32).ravel())
        print('  reflexo no piso: %d vértices, %d raios acertaram; luz do piso na tela: mediana %.2f, p90 %.2f' % (
            len(pts), len(alvos), np.median(col @ LUMA), np.percentile(col @ LUMA, 90)))


refletir_no_piso(OBJS)
print('  %.0f s' % (time.time() - INICIO))


# ------------------------------------------------------------------
# 4b. A casca do prédio (do mapa), para o reflexo do espelho e as prévias
# ------------------------------------------------------------------
def importar_casca():
    """Importa o mapa, guarda só o que está perto da loja e tira o painel da porta e os vidros"""
    if not os.path.exists(ARQUIVO_DO_MAPA):
        print('  sem o mapa (%s): prévias sem a casca' % ARQUIVO_DO_MAPA)
        return []
    antes = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=ARQUIVO_DO_MAPA)
    novos = [o for o in bpy.data.objects if o not in antes]
    bpy.context.view_layer.update()
    guardar = []
    meio = Vector((471.0, -162.5, 0))
    for o in novos:
        if o.type != 'MESH':
            continue
        bb = [o.matrix_world @ Vector(c) for c in o.bound_box]
        mn = Vector((min(v.x for v in bb), min(v.y for v in bb), min(v.z for v in bb)))
        mx = Vector((max(v.x for v in bb), max(v.y for v in bb), max(v.z for v in bb)))
        perto = mx.x > meio.x - 140 and mn.x < meio.x + 140 and mx.y > meio.y - 140 and mn.y < meio.y + 140
        if perto:
            guardar.append(o)
    for o in novos:
        if o not in guardar:
            bpy.data.objects.remove(o, do_unlink=True)
    # tira as peças das aberturas (as que ficam inteiras dentro de cada caixa)
    casca = bpy.data.objects.get('ror_Object_46')
    if casca is not None:
        me = casca.data
        bm = bmesh.new()
        bm.from_mesh(me)
        mw = casca.matrix_world
        tirar = set()
        bm.verts.ensure_lookup_table()
        for nome, _, mn, mx, *_ in ABERTURAS_DA_CASCA:
            dentro = 0
            for f in bm.faces:
                pts = [mw @ v.co for v in f.verts]
                ok = all(mn[0] <= p.x <= mx[0] and mn[1] <= p.z <= mx[1] and mn[2] <= -p.y <= mx[2] for p in pts)
                if ok:
                    tirar.add(f)
                    dentro += 1
            print('  casca: %s -> %d faces tiradas (só na prévia)' % (nome, dentro))
        bmesh.ops.delete(bm, geom=list(tirar), context='FACES')
        bm.to_mesh(me)
        bm.free()
    # o chão na porta
    dg = bpy.context.evaluated_depsgraph_get()
    for nome, (x, z) in (('fora da porta', tuple(diag(6.1, -2.0))), ('pátio', (480.0, 157.0))):
        ok, loc, *_ = bpy.context.scene.ray_cast(dg, b_de((x, PISO_Y + 2.5, z)), Vector((0, 0, -1)))
        print('  chão do mapa em %s (%.2f, %.2f): y = %s' % (nome, x, z, '%.3f' % loc.z if ok else 'nenhum'))
    # materiais do mapa: só a cor da textura (sem luz)
    for o in guardar:
        for slot in o.material_slots:
            m = slot.material
            if m is None or m.get('previa'):
                continue
            nt = m.node_tree
            bsdf = next((n for n in nt.nodes if n.type == 'BSDF_PRINCIPLED'), None)
            out = next((n for n in nt.nodes if n.type == 'OUTPUT_MATERIAL'), None)
            if not bsdf or not out:
                continue
            em = nt.nodes.new('ShaderNodeEmission')
            if bsdf.inputs['Base Color'].links:
                mult = nt.nodes.new('ShaderNodeMix')
                mult.data_type = 'RGBA'
                mult.blend_type = 'MULTIPLY'
                mult.inputs['Factor'].default_value = 1.0
                mult.inputs['B'].default_value = (0.92, 0.92, 0.92, 1)
                nt.links.new(bsdf.inputs['Base Color'].links[0].from_socket, mult.inputs['A'])
                nt.links.new(mult.outputs['Result'], em.inputs['Color'])
            else:
                em.inputs['Color'].default_value = bsdf.inputs['Base Color'].default_value
            nt.links.new(em.outputs[0], out.inputs['Surface'])
            m['previa'] = 1
    return guardar


CASCA = importar_casca()


# ------------------------------------------------------------------
# 4c. O reflexo do espelho: a sala vista do outro lado do vidro, pintada na textura dele
# ------------------------------------------------------------------
def camera_em(pos_jogo, alvo_jogo, lente=22, nome='cam'):
    sc = bpy.context.scene
    cam = bpy.data.cameras.new(nome)
    cam.lens = lente
    cam.clip_start = 0.1
    cam.clip_end = 2000
    co = bpy.data.objects.new(nome, cam)
    sc.collection.objects.link(co)
    p, a = b_de(pos_jogo), b_de(alvo_jogo)
    co.location = p
    co.rotation_euler = (a - p).to_track_quat('-Z', 'Y').to_euler()
    sc.camera = co
    return co


def renderizar_eevee(caminho, w, h):
    sc = bpy.context.scene
    sc.render.engine = 'BLENDER_EEVEE'
    sc.view_settings.view_transform = 'Standard'
    sc.view_settings.look = 'None'
    sc.eevee.taa_render_samples = 32
    sc.render.resolution_x, sc.render.resolution_y = w, h
    sc.render.resolution_percentage = 100
    sc.render.film_transparent = False
    sc.render.image_settings.file_format = 'PNG'
    sc.render.filepath = caminho
    bpy.ops.render.render(write_still=True)


def pintar_espelho(objs):
    """Renderiza a sala do ponto que o espelho reflete (de quem está no meio da baia, à altura de um
    carro) e põe na textura do espelho, invertida como num espelho"""
    R = ESPELHO['R']
    a0, a1 = ESPELHO['a']
    h0, h1 = ESPELHO['h']
    b = ESPELHO['b']
    # o observador: no meio da baia, a 1,5 m do chão
    obs = diag((PORTA_T[0] + PORTA_T[1]) / 2 - 1.0, 6.5)
    obs_b = float((obs - R.o) @ R.n)
    obs_a = float((obs - R.o) @ R.u)
    # a câmera do outro lado do vidro, olhando reto para dentro da sala (perpendicular ao espelho)
    cam_a, cam_b = obs_a, b - (obs_b - b)
    pos = R.p(cam_a, 1.5, cam_b)
    alvo = R.p(cam_a, 1.5, cam_b + 10)
    co = camera_em(pos, alvo, nome='cam_espelho')
    cam = co.data
    d = b - cam_b                                    # da câmera ao plano do espelho
    cam.clip_start = d + 0.05                        # a parede e o próprio vidro ficam para trás
    larg, alt = a1 - a0, h1 - h0
    cam.sensor_fit = 'HORIZONTAL'
    cam.sensor_width = 36
    cam.lens = 36 * d / larg                         # a imagem cobre o vidro inteiro
    # o meio do vidro fora do eixo da câmera: desloca a imagem (em frações da largura)
    direita = np.cross(R.d(0, 0, 1), np.array([0, 1.0, 0]))
    desvio = R.p((a0 + a1) / 2, (h0 + h1) / 2, b) - pos
    cam.shift_x = float(desvio @ direita) / larg
    cam.shift_y = float(desvio[1]) / larg
    w_px, h_px = 2 * TAM_DO_ESPELHO, 2 * ESPELHO['linhas']       # o dobro, e depois reduz (sem serrilhado)
    caminho = os.path.join(PASTA_TEMP, 'espelho.png')
    trocar_materiais(objs, material_de_previa)
    sc = bpy.context.scene
    sc.world.node_tree.nodes['Background'].inputs[0].default_value = (0.55, 0.72, 0.95, 1)
    sc.world.node_tree.nodes['Background'].inputs[1].default_value = 1.0
    esconder = [o for o in objs if o['malha'] == 'espelho'] + [o for o in sc.objects if o.type == 'LIGHT']
    for o in esconder:
        o.hide_render = True
    renderizar_eevee(caminho, w_px, h_px)
    for o in esconder:
        if o.type != 'LIGHT':
            o.hide_render = False
    img = ler_png(caminho)[..., :3]
    # a câmera está atrás do espelho olhando para a sala: a direita dela é a esquerda de quem olha o espelho
    img = img[:, ::-1]
    img = redimensionar(img, TAM_DO_ESPELHO, ESPELHO['linhas'])
    # um espelho velho: um pouco mais escuro e menos saturado, com faixas diagonais suaves de brilho
    cinza = (img @ LUMA)[..., None]
    img = (img * 0.85 + cinza * 0.15) * 0.85
    h, w = img.shape[:2]
    yy, xx = np.mgrid[0:h, 0:w].astype(np.float32)
    s = (xx / w) * 0.8 + (yy / h) * 0.6
    faixas = 0.11 * np.exp(-((s - 0.32) / 0.05) ** 2) + 0.06 * np.exp(-((s - 0.42) / 0.025) ** 2) + 0.08 * np.exp(-((s - 0.95) / 0.07) ** 2)
    img = np.clip(img + faixas[..., None] * (1 - img), 0, 1)
    tex = np.concatenate([img, np.repeat(img[-1:], TAM_DO_ESPELHO - h, axis=0)], axis=0)
    MALHAS['espelho'].imagem = imagem('loja_espelho', tex)
    TEXTURAS['espelho'] = tex
    bpy.data.objects.remove(co, do_unlink=True)


if not NOITE:
    pintar_espelho(OBJS)    # de noite, a textura do espelho é a do dia (a do .glb de dia; só a cor do vértice muda)


# ------------------------------------------------------------------
# 5. Exportar o desenho e conferir
# ------------------------------------------------------------------
def ler_glb(caminho):
    d = open(caminho, 'rb').read()
    n = struct.unpack('<I', d[12:16])[0]
    return json.loads(d[20:20 + n]), d


def soldar(o):
    """Junta os vértices no mesmo lugar (cada face nasceu com os seus): as cores ficam nos cantos, então o
    exportador só separa onde a cor ou a textura mudam"""
    bm = bmesh.new()
    bm.from_mesh(o.data)
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=1e-5)
    bm.to_mesh(o.data)
    bm.free()


def media_de_col(objs):
    soma, n = np.zeros(3), 0
    for o in objs:
        c = ler_atributo(o.data, 'Col')
        soma += c.sum(0)
        n += len(c)
    return soma / max(n, 1)


def cores_no_glb(j, d):
    """A média das COLOR_0 gravadas no .glb (lidas do buffer), ponderada pelos vértices"""
    n_json = struct.unpack('<I', d[12:16])[0]
    base = 20 + n_json + 8
    soma, n = np.zeros(3), 0
    tipos = {5121: (np.uint8, 255.0), 5123: (np.uint16, 65535.0), 5126: (np.float32, 1.0)}
    for me in j['meshes']:
        for p in me['primitives']:
            acc = j['accessors'][p['attributes']['COLOR_0']]
            bv = j['bufferViews'][acc['bufferView']]
            dt, esc = tipos[acc['componentType']]
            comps = {'VEC3': 3, 'VEC4': 4}[acc['type']]
            ini = base + bv.get('byteOffset', 0) + acc.get('byteOffset', 0)
            a = np.frombuffer(d, dt, count=acc['count'] * comps, offset=ini).reshape(-1, comps)[:, :3] / esc
            soma += a.sum(0)
            n += len(a)
    return soma / max(n, 1)


def exportar_visual(objs, caminho=None):
    trocar_materiais(objs, material_de_exportar)
    for o in objs:
        me = o.data
        for nome in [a.name for a in me.color_attributes if a.name != 'Col']:
            me.color_attributes.remove(me.color_attributes[nome])
        if me.attributes.get('suave') is not None:
            me.attributes.remove(me.attributes['suave'])
        me.color_attributes.active_color = me.color_attributes['Col']
        if MALHAS[o['malha']].imagem is None and o['malha'] != 'espelho':
            while me.uv_layers:
                me.uv_layers.remove(me.uv_layers[0])
        soldar(o)
    esperado = media_de_col(objs)
    bpy.ops.object.select_all(action='DESELECT')
    for o in objs:
        o.select_set(True)
    caminho = caminho or SAIDA + '.glb'
    bpy.ops.export_scene.gltf(filepath=caminho, export_format='GLB', use_selection=True, export_yup=True,
                              export_apply=True, export_animations=False, export_normals=False,
                              export_vertex_color='ACTIVE', export_all_vertex_colors=False,
                              export_active_vertex_color_when_no_material=True, export_image_format='AUTO',
                              export_materials='EXPORT')
    j, d = ler_glb(caminho)
    tris = 0
    sem_cor = 0
    maior = 0
    for me in j['meshes']:
        for p in me['primitives']:
            if 'COLOR_0' not in p['attributes']:
                sem_cor += 1
            acc = j['accessors'][p['indices']]
            tris += acc['count'] // 3
            maior = max(maior, j['accessors'][p['attributes']['POSITION']]['count'])
    tipos = sorted({(j['accessors'][p['attributes']['COLOR_0']]['type'], j['accessors'][p['attributes']['COLOR_0']]['componentType'])
                    for me in j['meshes'] for p in me['primitives'] if 'COLOR_0' in p['attributes']})
    imgs = [(i.get('name'), i.get('mimeType')) for i in j.get('images', [])]
    fatores = [m.get('pbrMetallicRoughness', {}).get('baseColorFactor', [1, 1, 1, 1]) for m in j['materials']]
    print('Exportado %s' % caminho)
    print('  primitivas: %d (sem COLOR_0: %d; COLOR_0 %s); maior: %d vértices' % (
        sum(len(m['primitives']) for m in j['meshes']), sem_cor, tipos, maior))
    print('  triângulos: %d; materiais: %d; texturas: %s' % (tris, len(j['materials']), imgs))
    print('  baseColorFactor: %s' % fatores)
    print('  tamanho: %.2f MB' % (os.path.getsize(caminho) / 1e6))
    lido = cores_no_glb(j, d)
    print('  COLOR_0 lido do .glb: média %s; nos cantos do Blender: %s' % (np.round(lido, 3), np.round(esperado, 3)))
    if sem_cor:
        raise SystemExit('ERRO: há primitivas sem COLOR_0')
    if maior >= 65536:
        raise SystemExit('ERRO: uma malha passou de 65535 vértices')
    return dict(triangulos=tris, materiais=len(j['materials']), texturas=len(imgs), bytes=os.path.getsize(caminho))


if NOITE:
    # de noite, o desenho assado vai para a pasta temporária; o _noite.glb é o .glb de dia com estas cores (9.)
    CONTAGENS = exportar_visual(OBJS, os.path.join(PASTA_TEMP, 'noite_assada.glb'))
else:
    CONTAGENS = exportar_visual(OBJS)


# ------------------------------------------------------------------
# 6. A colisão
# ------------------------------------------------------------------
def tri(a, b, c):
    COLISAO.append((tuple(map(float, a)), tuple(map(float, b)), tuple(map(float, c))))


def quad_col(a, b, c, d):
    tri(a, b, c)
    tri(a, c, d)


def parede_col(p, q, h0, h1):
    """Uma parede em pé de p a q (x, z), de h0 a h1 acima do piso"""
    a = (p[0], PISO_Y + h0, p[1])
    b = (q[0], PISO_Y + h0, q[1])
    c = (q[0], PISO_Y + h1, q[1])
    d = (p[0], PISO_Y + h1, p[1])
    quad_col(a, b, c, d)


def montar_colisao():
    ALTURA_DAS_PAREDES = ALTURA
    ALTURA_DOS_MOVEIS = 2.0
    # o piso (com uma aba de 1,5 m para fora da porta, por cima do pátio)
    aba = [tuple(diag(PORTA_T[0] - 0.4, -1.5)), tuple(diag(PORTA_T[1] + 0.4, -1.5)),
           tuple(diag(PORTA_T[1] + 0.4, 0.0)), tuple(diag(PORTA_T[0] - 0.4, 0.0))]
    soleira = [tuple(diag(PORTA_T[0] + FOLGA_DO_VAO, 0.0)), tuple(diag(PORTA_T[1] - FOLGA_DO_VAO, 0.0)),
               tuple(diag(PORTA_T[1] - FOLGA_DO_VAO, RECUO_DA_DIAGONAL)), tuple(diag(PORTA_T[0] + FOLGA_DO_VAO, RECUO_DA_DIAGONAL))]
    for pc in PECAS_DO_PISO + [soleira, aba]:
        p0 = pc[0]
        for k in range(1, len(pc) - 1):
            tri((p0[0], PISO_Y, p0[1]), (pc[k][0], PISO_Y, pc[k][1]), (pc[k + 1][0], PISO_Y, pc[k + 1][1]))
    # as paredes (as janelas e vitrines fechadas; a porta aberta)
    for W in PAREDES:
        if W is P_PORTA:
            v = W.vaos[0]
            parede_col(W.p0, W.p0 + v.u0 * W.R.u, 0, ALTURA_DAS_PAREDES)
            parede_col(W.p0 + v.u1 * W.R.u, W.p1, 0, ALTURA_DAS_PAREDES)
            # as ombreiras da porta, da nossa parede até a face de fora
            for u in (v.u0, v.u1):
                p = W.p0 + u * W.R.u
                parede_col(p, p - W.fora * W.R.n, 0, PORTA_TOPO - PISO_Y)
        else:
            parede_col(W.p0, W.p1, 0, ALTURA_DAS_PAREDES)
    # os obstáculos: caixas de paredes em pé, sem tampa
    for ob in COLISAO_CAIXAS:
        if ob[0] == 'coluna':
            _, x, z, r = ob
            pts = [(x + r * math.cos(a), z + r * math.sin(a)) for a in np.linspace(0, 2 * math.pi, 9)[:-1]]
            for k in range(8):
                parede_col(pts[k], pts[(k + 1) % 8], 0, ALTURA_DOS_MOVEIS)
        else:
            _, R, a0, a1, b0, b1 = ob
            c = [R.o + a0 * R.u + b0 * R.n, R.o + a1 * R.u + b0 * R.n, R.o + a1 * R.u + b1 * R.n, R.o + a0 * R.u + b1 * R.n]
            for k in range(4):
                parede_col(c[k], c[(k + 1) % 4], 0, ALTURA_DOS_MOVEIS)
    me = bpy.data.meshes.new('loja_do_luigi_colisao')
    verts = []
    faces = []
    for t in COLISAO:
        base = len(verts)
        verts += [b_de(p) for p in t]
        faces.append((base, base + 1, base + 2))
    me.from_pydata(verts, [], faces)
    o = bpy.data.objects.new('loja_do_luigi_colisao', me)
    bpy.context.scene.collection.objects.link(o)
    bpy.ops.object.select_all(action='DESELECT')
    o.select_set(True)
    bpy.context.view_layer.objects.active = o
    caminho = SAIDA + '_colisao.glb'
    bpy.ops.export_scene.gltf(filepath=caminho, export_format='GLB', use_selection=True, export_yup=True,
                              export_apply=True, export_animations=False, export_normals=False, export_texcoords=False,
                              export_vertex_color='NONE', export_materials='NONE')
    print('Exportado %s: %d triângulos' % (caminho, len(COLISAO)))
    o.hide_render = True
    return len(COLISAO)


if not NOITE:     # de noite, a colisão e o json de dia ficam como estão
    TRIANGULOS_DA_COLISAO = montar_colisao()


# ------------------------------------------------------------------
# 7. O JSON para a integração
# ------------------------------------------------------------------
def montar_json():
    centro_fora = diag((PORTA_T[0] + PORTA_T[1]) / 2, 0.0)
    centro_dentro = diag((PORTA_T[0] + PORTA_T[1]) / 2, RECUO_DA_DIAGONAL)
    rumo_dentro = math.atan2(DIAG_N[0], -DIAG_N[1])
    lugares = {}
    for nome, (x, z, comp, larg) in (('luigi', LUGAR_DO_LUIGI), ('guido', LUGAR_DO_GUIDO)):
        r = rumo_para((x, z), centro_dentro)
        lugares[nome] = dict(x=round(x, 3), y=PISO_Y, z=round(z, 3), rumo=round(r, 4), comprimento=comp, largura=larg,
                             nota='olhando para a porta')
    aberturas = []
    for nome, desc, mn, mx, faces, (pa, pb), y0, y1 in ABERTURAS_DA_CASCA:
        aberturas.append(dict(nome=nome, descricao=desc, caixa_min=list(mn), caixa_max=list(mx), faces=faces,
                              vao_na_face_de_fora=[[pa[0], y0, pa[1]], [pb[0], y0, pb[1]], [pb[0], y1, pb[1]], [pa[0], y1, pa[1]]]))
    dados = dict(
        sobre='Casa Della Tires por dentro (ferramentas/montar_loja_do_luigi.py). Eixos do jogo: x, y para cima, z. '
              'Rumo do jogo: 0 olhando para -z, crescendo para a direita (a frente é (sin r, -cos r)).',
        piso_y=PISO_Y,
        teto_y=round(TETO_Y, 3),
        porta=dict(
            centro_na_fachada=[round(centro_fora[0], 3), PISO_Y, round(centro_fora[1], 3)],
            centro_por_dentro=[round(centro_dentro[0], 3), PISO_Y, round(centro_dentro[1], 3)],
            direcao_para_dentro=[round(DIAG_N[0], 4), 0.0, round(DIAG_N[1], 4)],
            rumo_para_dentro=round(rumo_dentro, 4),
            rumo_para_fora=round(math.atan2(DIAG_FORA[0], -DIAG_FORA[1]), 4),
            ao_longo=[round(DIAG_D[0], 4), 0.0, round(DIAG_D[1], 4)],
            largura=round(PORTA_T[1] - PORTA_T[0] - 2 * FOLGA_DO_VAO, 3),
            altura=round(PORTA_TOPO - FOLGA_DO_VAO - PISO_Y, 3),
            pontas_na_fachada=[[round(v, 3) for v in diag(PORTA_T[0], 0)], [round(v, 3) for v in diag(PORTA_T[1], 0)]],
            profundidade_do_rebaixo=RECUO_DA_DIAGONAL,
        ),
        lugares=lugares,
        baia_de_servico=[[round(p[0], 3), round(p[1], 3)] for p in BAIA],
        contorno_por_dentro=[[round(p[0], 3), round(p[1], 3)] for p in CONTORNO],
        aberturas_para_tirar_da_casca=aberturas,
        nota_das_aberturas='Peças de ror_Object_46 inteiras dentro de cada caixa (eixos do jogo; "faces" = quantas faces '
                           'a peça tem). vao_na_face_de_fora: os quatro cantos do vão, na face de fora da casca (a janela '
                           'e os arcos têm o alto em arco até o y do alto). Tirar do desenho; '
                           'na colisão do mapa, a do painel da porta sai (a porta fica aberta) e as dos vidros podem '
                           'ficar (a colisão da loja já fecha as janelas e as vitrines por dentro).',
        colisao=dict(arquivo=os.path.basename(SAIDA) + '_colisao.glb', triangulos=TRIANGULOS_DA_COLISAO,
                     nota='piso em y = piso_y (com uma aba de 1,5 m para fora da porta) e paredes em pé; '
                          'sem teto nem tampos'),
    )
    with open(SAIDA + '.json', 'w') as f:
        json.dump(dados, f, indent=2, ensure_ascii=False)
    print('Gravado %s' % (SAIDA + '.json'))
    return dados


if not NOITE:
    DADOS = montar_json()


# ------------------------------------------------------------------
# 8. Prévias
# ------------------------------------------------------------------
def fazer_previas(objs):
    os.makedirs(PREVIAS, exist_ok=True)
    trocar_materiais(objs, material_de_previa)
    sc = bpy.context.scene
    for o in list(sc.objects):
        if o.type == 'LIGHT':
            o.hide_render = True
    sc.world.node_tree.nodes['Background'].inputs[0].default_value = (0.55, 0.72, 0.95, 1)
    sc.world.node_tree.nodes['Background'].inputs[1].default_value = 1.0
    porta_dentro = diag((PORTA_T[0] + PORTA_T[1]) / 2, RECUO_DA_DIAGONAL)
    vistas = [
        ('dentro_movel', (473.0, PISO_Y + 1.9, 165.0), (463.5, PISO_Y + 2.2, 177.0), 18),
        ('dentro_porta', (462.5, PISO_Y + 2.2, 173.0), (porta_dentro[0], PISO_Y + 2.6, porta_dentro[1]), 16),
        ('posteres_janela', (458.5, PISO_Y + 2.0, 163.0), (469.7, PISO_Y + 2.6, 151.5), 16),
        ('posteres_oeste', (467.5, PISO_Y + 2.0, 160.5), (455.8, PISO_Y + 3.0, 166.0), 16),
        ('ala_sul', (468.5, PISO_Y + 2.0, 166.5), (484.0, PISO_Y + 2.0, 174.0), 16),
        ('nicho', (463.5, PISO_Y + 1.7, 171.0), (463.5, PISO_Y + 2.4, 177.0), 24),
        ('fora_porta', tuple(np.array([diag(6.1, -16.0)[0], PISO_Y + 3.2, diag(6.1, -16.0)[1]])),
         (porta_dentro[0], PISO_Y + 2.5, porta_dentro[1]), 24),
        ('fora_vitrine', (494.0, PISO_Y + 2.5, 150.0), (476.0, PISO_Y + 2.5, 162.0), 22),
        ('entrando', (476.3, PISO_Y + 2.6, 161.0), (466.0, PISO_Y + 1.6, 169.5), 16),
        ('espelho', (466.5, PISO_Y + 2.0, 166.0), (474.2, PISO_Y + 2.3, 177.0), 18),
        ('piso_brilho', (466.0, PISO_Y + 1.2, 156.8), (462.0, PISO_Y + 0.5, 176.0), 18),
    ]
    for nome, pos, alvo, lente in vistas:
        co = camera_em(pos, alvo, lente=lente, nome='cam_' + nome)
        renderizar_eevee(os.path.join(PREVIAS, nome + '.png'), 1280, 720)
        bpy.data.objects.remove(co, do_unlink=True)
        print('  prévia %s' % nome)
    # de cima, a planta (com os lugares do Luigi e do Guido marcados)
    for nome, (x, z, comp, larg) in (('luigi', LUGAR_DO_LUIGI), ('guido', LUGAR_DO_GUIDO)):
        r = rumo_para((x, z), porta_dentro)
        f = np.array([math.sin(r), -math.cos(r)])
        d = np.array([math.cos(r), math.sin(r)])
        c = np.array([x, z])
        pts = [c + f * comp / 2 + d * larg / 2, c + f * comp / 2 - d * larg / 2, c - f * comp / 2 - d * larg / 2, c - f * comp / 2 + d * larg / 2]
        me = bpy.data.meshes.new('lugar_' + nome)
        me.from_pydata([b_de((p[0], PISO_Y + 0.05, p[1])) for p in pts], [], [(0, 1, 2, 3)])
        o = bpy.data.objects.new(me.name, me)
        sc.collection.objects.link(o)
        mat = novo_material('lugar')
        nt = mat.node_tree
        nt.nodes.clear()
        em = nt.nodes.new('ShaderNodeEmission')
        em.inputs['Color'].default_value = (0.1, 0.9, 0.2, 1) if nome == 'luigi' else (0.1, 0.4, 1.0, 1)
        nt.links.new(em.outputs[0], nt.nodes.new('ShaderNodeOutputMaterial').inputs['Surface'])
        me.materials.append(mat)
    cam = bpy.data.cameras.new('cima')
    cam.type = 'ORTHO'
    cam.ortho_scale = 36
    cam.clip_start = 1
    cam.clip_end = 200
    co = bpy.data.objects.new('cima', cam)
    sc.collection.objects.link(co)
    co.location = b_de((471.3, TETO_Y - 0.6, 162.3))
    co.rotation_euler = (0, 0, 0)
    sc.camera = co
    renderizar_eevee(os.path.join(PREVIAS, 'planta.png'), 1100, 1100)
    print('  prévia planta (cortada abaixo do teto)')
    cam.clip_start = 0.62
    renderizar_eevee(os.path.join(PREVIAS, 'planta.png'), 1100, 1100)
    salvar_png(ATLAS, os.path.join(PREVIAS, 'atlas_dos_posteres.png'))


if PREVIAS and not NOITE:
    print('Prévias em %s...' % PREVIAS)
    fazer_previas(OBJS)

# ------------------------------------------------------------------
# 9. A noite (--noite): <saida>_noite.glb e <saida>_noite.json
# ------------------------------------------------------------------
# O jogo mistura a luz de dia e a de noite vértice a vértice (a primitiva i e o vértice j de um casam com os do
# outro). Por isso o _noite.glb é uma cópia byte a byte do .glb de dia (as mesmas malhas e primitivas, na mesma
# ordem, os mesmos vértices, posições, coordenadas de textura, índices e texturas) em que só o COLOR_0 muda. As
# cores vêm do desenho assado de noite (a mesma montagem, com as luzes da noite), exportado à parte: o
# exportador separa os vértices onde a cor muda, e as de noite mudam em outros lugares, então cada vértice do
# dia leva a média das cores de noite dos cantos dos triângulos que caem nele (os triângulos saem na mesma
# ordem nos dois; se não, o par de cada um é achado pelas posições dos três cantos).
TIPOS_DO_GLB = {5121: np.uint8, 5123: np.uint16, 5125: np.uint32, 5126: np.float32}
COMPONENTES_DO_GLB = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3, 'VEC4': 4}


def onde_no_glb(j, d, k):
    """Onde ficam no arquivo os dados do accessor k: (início, tipo, componentes, quantos)"""
    n_json = struct.unpack('<I', d[12:16])[0]
    acc = j['accessors'][k]
    bv = j['bufferViews'][acc['bufferView']]
    dt = np.dtype(TIPOS_DO_GLB[acc['componentType']]).newbyteorder('<')
    c = COMPONENTES_DO_GLB[acc['type']]
    if bv.get('byteStride', c * dt.itemsize) != c * dt.itemsize:
        raise SystemExit('ERRO: o accessor %d é intercalado (byteStride)' % k)
    return 20 + n_json + 8 + bv.get('byteOffset', 0) + acc.get('byteOffset', 0), dt, c, acc['count']


def ler_acessor(j, d, k):
    ini, dt, c, n = onde_no_glb(j, d, k)
    return np.frombuffer(d, dt, count=n * c, offset=ini).reshape(n, c)


def bytes_do_acessor(j, d, k):
    ini, dt, c, n = onde_no_glb(j, d, k)
    return d[ini:ini + n * c * dt.itemsize]


def cor_do_acessor(j, d, k):
    a = ler_acessor(j, d, k).astype(np.float64)
    return a / {5121: 255.0, 5123: 65535.0}.get(j['accessors'][k]['componentType'], 1.0)


def pares_dos_cantos(Pd, Id, Pn, In):
    """Para cada canto dos triângulos do dia (Id), o vértice de noite do canto que é o par dele (-1 se nenhum).
    Os mesmos triângulos na mesma ordem: canto a canto. Se não, o par de cada triângulo pelas posições dos três
    cantos (em milímetros, a partir do menor, na mesma volta)"""
    if len(Id) == len(In) and np.abs(Pd[Id] - Pn[In]).max() < 1e-4:
        return In.copy()

    def chaves(P, I):
        q = np.round(P[I] * 1000).astype(np.int64).reshape(-1, 3, 3)
        ks, giros = [], []
        for t in q:
            g = min(range(3), key=lambda r: tuple(t[r]))
            ks.append(np.roll(t, -g, axis=0).tobytes())
            giros.append(g)
        return ks, giros
    kd, gd = chaves(Pd, Id)
    kn, gn = chaves(Pn, In)
    livres = {}
    for t, k in enumerate(kn):
        livres.setdefault(k, []).append(t)
    par = np.full(len(Id), -1, np.int64)
    for t, k in enumerate(kd):
        lista = livres.get(k)
        if not lista:
            continue
        u = lista.pop(0) if len(lista) > 1 else lista[0]
        for r in range(3):
            par[3 * t + (gd[t] + r) % 3] = In[3 * u + (gn[u] + r) % 3]
    return par


def transplantar_cores(dia, noite_assada, saida):
    """Grava em 'saida' o .glb de dia com o COLOR_0 de cada primitiva trocado pelas cores do assado de noite"""
    from mathutils.kdtree import KDTree
    jd, dd = ler_glb(dia)
    jn, dn = ler_glb(noite_assada)
    novo = bytearray(dd)
    noite_por_nome = {m['name']: m for m in jn['meshes']}
    print('A noite: as cores do assado de noite na geometria de %s' % dia)
    for md in jd['meshes']:
        mn = noite_por_nome.get(md['name'])
        if mn is None or len(mn['primitives']) != len(md['primitives']):
            raise SystemExit('ERRO: a malha %s do dia não tem par no assado de noite' % md['name'])
        for k, (pd, pn) in enumerate(zip(md['primitives'], mn['primitives'])):
            Pd = ler_acessor(jd, dd, pd['attributes']['POSITION']).astype(np.float64)
            Id = ler_acessor(jd, dd, pd['indices']).ravel().astype(np.int64)
            Pn = ler_acessor(jn, dn, pn['attributes']['POSITION']).astype(np.float64)
            In = ler_acessor(jn, dn, pn['indices']).ravel().astype(np.int64)
            Cn = cor_do_acessor(jn, dn, pn['attributes']['COLOR_0'])[:, :3]
            par = pares_dos_cantos(Pd, Id, Pn, In)
            em_ordem = len(Id) == len(In) and bool(np.array_equal(par, In))
            ok = par >= 0
            soma = np.zeros((len(Pd), 3))
            conta = np.zeros(len(Pd))
            np.add.at(soma, Id[ok], Cn[par[ok]])
            np.add.at(conta, Id[ok], 1)
            sem = np.flatnonzero(conta == 0)
            if len(sem):
                arv = KDTree(len(Pn))
                for i, p in enumerate(Pn):
                    arv.insert(tuple(p), i)
                arv.balance()
                for v in sem:
                    _, i, _ = arv.find(tuple(Pd[v]))
                    soma[v] = Cn[i]
                    conta[v] = 1
            cor = np.clip(soma / conta[:, None], 0, 1)
            kc = pd['attributes']['COLOR_0']
            ini, dt, c, n = onde_no_glb(jd, dd, kc)
            C = ler_acessor(jd, dd, kc).copy()
            if dt.kind == 'u':
                C[:, :3] = np.round(cor * np.iinfo(dt).max).astype(dt)
            else:
                C[:, :3] = cor
            novo[ini:ini + n * c * dt.itemsize] = C.astype(dt).tobytes()
            print('  %-24s primitiva %d: %6d vértices (%6d no assado de noite), %6d triângulos; %s; %d vértices sem '
                  'par (cor do vizinho mais perto)' % (md['name'], k, len(Pd), len(Pn), len(Id) // 3,
                                                       'os mesmos triângulos na mesma ordem' if em_ordem else
                                                       'pares achados pelas posições (%d cantos de %d)' % (ok.sum(), len(Id)),
                                                       len(sem)))
    with open(saida, 'wb') as f:
        f.write(novo)
    print('Gravado %s' % saida)


def conferir_noite(dia, noite):
    """Lê os dois .glb e confere que só o COLOR_0 mudou: o mesmo JSON (malhas, primitivas, accessors, materiais,
    texturas) e, por primitiva, os mesmos bytes de POSITION, TEXCOORD_0 e índices; fora dos COLOR_0, o arquivo
    inteiro igual byte a byte (as texturas embutidas também)"""
    jd, dd = ler_glb(dia)
    jn, dn = ler_glb(noite)
    n_json = struct.unpack('<I', dd[12:16])[0]
    if jd != jn or len(dd) != len(dn) or dd[:20 + n_json] != dn[:20 + n_json]:
        raise SystemExit('ERRO: o JSON do _noite.glb não é o do .glb de dia')
    fora = np.ones(len(dd), bool)
    print('Conferindo %s com %s:' % (os.path.basename(noite), os.path.basename(dia)))
    for i, (md, mn) in enumerate(zip(jd['meshes'], jn['meshes'])):
        for k, (pd, pn) in enumerate(zip(md['primitives'], mn['primitives'])):
            iguais = []
            for nome in ('POSITION', 'TEXCOORD_0', 'indices'):
                a = pd['indices'] if nome == 'indices' else pd['attributes'].get(nome)
                b = pn['indices'] if nome == 'indices' else pn['attributes'].get(nome)
                if a is None and b is None:
                    continue
                if a is None or b is None or bytes_do_acessor(jd, dd, a) != bytes_do_acessor(jn, dn, b):
                    raise SystemExit('ERRO: %s da primitiva %d da malha %s mudou' % (nome, k, md['name']))
                iguais.append(nome)
            kc = pd['attributes']['COLOR_0']
            ini, dt, c, n = onde_no_glb(jd, dd, kc)
            fora[ini:ini + n * c * dt.itemsize] = False
            cd = cor_do_acessor(jd, dd, kc)[:, :3]
            cn = cor_do_acessor(jn, dn, kc)[:, :3]
            mudou = 100.0 * np.mean(np.any(cd != cn, axis=1))
            print('  malha %2d %-24s p%d: %6d vértices, %6d triângulos; iguais: %s; COLOR_0 mudou em %3.0f%% dos vértices '
                  '(média dia %s, noite %s)' % (i, md['name'], k, n, jd['accessors'][pd['indices']]['count'] // 3,
                                               ', '.join(iguais), mudou, np.round(cd.mean(0), 3), np.round(cn.mean(0), 3)))
    a = np.frombuffer(dd, np.uint8)
    b = np.frombuffer(dn, np.uint8)
    diferentes = int(np.count_nonzero(a[fora] != b[fora]))
    print('  fora dos COLOR_0: %d bytes conferidos, %d diferentes' % (int(fora.sum()), diferentes))
    if diferentes:
        raise SystemExit('ERRO: o _noite.glb difere do .glb de dia fora dos COLOR_0')


def gravar_luzes_de_fora(luzes):
    """<saida>_noite.json: as luzes pontuais (eixos do jogo) que a luz de dentro joga para fora pelas portas e
    janelas, no chão e nas paredes em volta do prédio. Cada uma: lugar, cor (0 a 1), raio (metros: até onde ela
    chega) e força, como as Lampada de src/hora.h (o jogo usa as mais perto da câmera)"""
    dados = {'luzes': [dict(x=round(float(p[0]), 3), y=round(float(p[1]), 3), z=round(float(p[2]), 3),
                            r=round(float(c[0]), 3), g=round(float(c[1]), 3), b=round(float(c[2]), 3),
                            raio=round(float(raio), 2), forca=round(float(forca), 2))
                       for p, c, raio, forca in luzes]}
    caminho = SAIDA + '_noite.json'
    with open(caminho, 'w') as f:
        json.dump(dados, f, indent=2, ensure_ascii=False)
    print('Gravado %s: %d luzes' % (caminho, len(luzes)))
    for l in dados['luzes']:
        print('  (%.2f, %.2f, %.2f) cor (%.2f, %.2f, %.2f) raio %.1f força %.2f' % tuple(l.values()))


# As prévias de noite: o .glb de dia e o _noite.glb como o jogo os carrega (textura × cor do vértice), lado a
# lado, com a casca do mapa: de dia como as prévias de dia; de noite, com a luz que o shader dos mapas
# (src/mundo.c) dá à meia-noite (a chave das 0h de src/hora.c, com a lua) mais as lâmpadas do _noite.json
# (a conta de luzDasLampadas, src/hora.h)
LUA_DO_JOGO = (0.25, 0.75, -0.6)
BASE_DA_NOITE_DO_JOGO = (0.30, 0.32, 0.50)
CEU_DA_NOITE_DO_JOGO = (0.20, 0.22, 0.42)
LUAR_DO_JOGO = (0.22, 0.26, 0.44)
FUNDO_DA_NOITE = (0.012, 0.010, 0.040)     # o céu da noite atrás (linear)


def material_do_jogo(nome, img, atributo):
    """Textura × cor do vértice (o atributo do .glb importado), sem luz: na tela, emissão de (t·c)^2,2"""
    mat = novo_material(nome)
    nt = mat.node_tree
    nt.nodes.clear()
    saida = nt.nodes.new('ShaderNodeOutputMaterial')
    at = nt.nodes.new('ShaderNodeAttribute')
    at.attribute_name = atributo
    gama = nt.nodes.new('ShaderNodeGamma')
    gama.inputs['Gamma'].default_value = 2.2
    nt.links.new(at.outputs['Color'], gama.inputs['Color'])
    cor = gama.outputs['Color']
    if img is not None:
        tex = nt.nodes.new('ShaderNodeTexImage')
        tex.image = img
        tex.interpolation = 'Linear'
        mult = nt.nodes.new('ShaderNodeMix')
        mult.data_type = 'RGBA'
        mult.blend_type = 'MULTIPLY'
        mult.inputs['Factor'].default_value = 1.0
        nt.links.new(cor, mult.inputs['A'])
        nt.links.new(tex.outputs['Color'], mult.inputs['B'])
        cor = mult.outputs['Result']
    em = nt.nodes.new('ShaderNodeEmission')
    nt.links.new(cor, em.inputs['Color'])
    nt.links.new(em.outputs[0], saida.inputs['Surface'])
    return mat


def importar_para_previa(caminho):
    """Importa um .glb do interior (o que o jogo carrega) com o material do jogo. Devolve os objetos"""
    antes = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=caminho)
    novos = [o for o in bpy.data.objects if o not in antes]
    malhas = [o for o in novos if o.type == 'MESH']
    for o in novos:
        if o.type != 'MESH':
            bpy.data.objects.remove(o, do_unlink=True)
    for o in malhas:
        me = o.data
        m = o.material_slots[0].material if o.material_slots else None
        img = next((n.image for n in m.node_tree.nodes if n.type == 'TEX_IMAGE'), None) if m and m.node_tree else None
        me.materials.clear()
        me.materials.append(material_do_jogo('jogo_' + o.name, img, me.color_attributes[0].name))
    return malhas


def grupo_da_luz_da_noite(luzes):
    """Um grupo de nós: a luz que o jogo dá ao mapa à meia-noite num ponto (com as lâmpadas), na tela, elevada a
    2,2 (a prévia faz a conta em linear)"""
    g = bpy.data.node_groups.new('luz_da_noite', 'ShaderNodeTree')
    g.interface.new_socket('Luz', in_out='OUTPUT', socket_type='NodeSocketColor')
    N, L = g.nodes, g.links
    geo = N.new('ShaderNodeNewGeometry')
    n = geo.outputs['True Normal']

    def vet(op, a, b=None, escala=None):
        no = N.new('ShaderNodeVectorMath')
        no.operation = op
        for k, x in enumerate((a, b)):
            if x is None:
                continue
            if isinstance(x, bpy.types.NodeSocket):
                L.new(x, no.inputs[k])
            else:
                no.inputs[k].default_value = tuple(map(float, x))
        if escala is not None:
            if isinstance(escala, bpy.types.NodeSocket):
                L.new(escala, no.inputs['Scale'])
            else:
                no.inputs['Scale'].default_value = float(escala)
        return no.outputs['Value'] if op in ('DOT_PRODUCT', 'LENGTH', 'DISTANCE') else no.outputs['Vector']

    def mat_(op, a, b=None, c=None):
        no = N.new('ShaderNodeMath')
        no.operation = op
        for k, x in enumerate((a, b, c)):
            if x is None:
                continue
            if isinstance(x, bpy.types.NodeSocket):
                L.new(x, no.inputs[k])
            else:
                no.inputs[k].default_value = float(x)
        return no.outputs[0]
    sep = N.new('ShaderNodeSeparateXYZ')
    L.new(n, sep.inputs[0])
    cima = mat_('MULTIPLY_ADD', sep.outputs['Z'], 0.5, 0.5)
    total = vet('ADD', [0.58 * x for x in BASE_DA_NOITE_DO_JOGO], vet('SCALE', [0.30 * x for x in CEU_DA_NOITE_DO_JOGO], escala=cima))
    lua = mat_('MAXIMUM', vet('DOT_PRODUCT', n, tuple(b_de(unit(LUA_DO_JOGO)))), 0.0)
    total = vet('ADD', total, vet('SCALE', [0.38 * x for x in LUAR_DO_JOGO], escala=lua))
    for p, cor, raio, forca in luzes:
        d = vet('SUBTRACT', tuple(b_de(p)), geo.outputs['Position'])
        q = mat_('MULTIPLY', vet('DOT_PRODUCT', d, d), 1.0 / (raio * raio))
        a = mat_('MAXIMUM', mat_('SUBTRACT', 1.0, q), 0.0)
        a2 = mat_('MULTIPLY', a, a)
        virado = mat_('MULTIPLY_ADD', mat_('MAXIMUM', vet('DOT_PRODUCT', n, vet('NORMALIZE', d)), 0.0), 0.75, 0.25)
        total = vet('ADD', total, vet('SCALE', [forca * x for x in cor], escala=mat_('MULTIPLY', a2, virado)))
    total = vet('SCALE', total, escala=0.92)
    gama = N.new('ShaderNodeGamma')
    gama.inputs['Gamma'].default_value = 2.2
    L.new(total, gama.inputs['Color'])
    out = N.new('NodeGroupOutput')
    L.new(gama.outputs['Color'], out.inputs[0])
    return g


def casca_de_noite(casca, luzes):
    """Põe nos materiais da casca (já com a emissão das prévias de dia) a emissão de noite: a cor da textura
    vezes a luz da noite do jogo. Devolve as trocas, para hora_da_casca"""
    grupo = grupo_da_luz_da_noite(luzes)
    trocas = []
    feitos = set()
    for o in casca:
        for slot in o.material_slots:
            m = slot.material
            if m is None or m.name in feitos or m.node_tree is None:
                continue
            feitos.add(m.name)
            nt = m.node_tree
            out = next((x for x in nt.nodes if x.type == 'OUTPUT_MATERIAL' and x.inputs['Surface'].links), None)
            bsdf = next((x for x in nt.nodes if x.type == 'BSDF_PRINCIPLED'), None)
            if out is None or bsdf is None:
                continue
            dia = out.inputs['Surface'].links[0].from_socket
            mult = nt.nodes.new('ShaderNodeMix')
            mult.data_type = 'RGBA'
            mult.blend_type = 'MULTIPLY'
            mult.inputs['Factor'].default_value = 1.0
            if bsdf.inputs['Base Color'].links:
                nt.links.new(bsdf.inputs['Base Color'].links[0].from_socket, mult.inputs['A'])
            else:
                mult.inputs['A'].default_value = bsdf.inputs['Base Color'].default_value
            gn = nt.nodes.new('ShaderNodeGroup')
            gn.node_tree = grupo
            nt.links.new(gn.outputs[0], mult.inputs['B'])
            em = nt.nodes.new('ShaderNodeEmission')
            nt.links.new(mult.outputs['Result'], em.inputs['Color'])
            trocas.append((nt, out, dia, em.outputs[0]))
    return trocas


def hora_da_casca(trocas, noite):
    for nt, out, dia, noite_ in trocas:
        nt.links.new(noite_ if noite else dia, out.inputs['Surface'])


def previas_dia_e_noite(vistas, casca, luzes, esconder, w=960, h=540):
    """Cada vista (nome, posição, alvo, lente; eixos do jogo) renderizada com o .glb de dia e com o _noite.glb,
    lado a lado (dia à esquerda), em PREVIAS/<nome>.png"""
    os.makedirs(PREVIAS, exist_ok=True)
    sc = bpy.context.scene
    for o in list(sc.objects):
        if o.type == 'LIGHT' or o in esconder:
            o.hide_render = True
    dia = importar_para_previa(SAIDA + '.glb')
    noite = importar_para_previa(SAIDA + '_noite.glb')
    trocas = casca_de_noite(casca, luzes)
    if sc.world is None:
        sc.world = novo_mundo('ceu_das_previas')
    ceu = sc.world.node_tree.nodes['Background']
    for nome, pos, alvo, lente in vistas:
        co = camera_em(pos, alvo, lente=lente, nome='cam_' + nome)
        imgs = []
        for de_noite in (False, True):
            for o in dia:
                o.hide_render = de_noite
            for o in noite:
                o.hide_render = not de_noite
            hora_da_casca(trocas, de_noite)
            ceu.inputs[0].default_value = (*FUNDO_DA_NOITE, 1) if de_noite else (0.55, 0.72, 0.95, 1)
            ceu.inputs[1].default_value = 1.0
            arq = os.path.join(PASTA_TEMP, 'previa_%s_%d.png' % (nome, de_noite))
            renderizar_eevee(arq, w, h)
            imgs.append(ler_png(arq)[..., :3])
        salvar_png(np.concatenate([imgs[0], np.ones((h, 6, 3), np.float32), imgs[1]], axis=1),
                   os.path.join(PREVIAS, nome + '.png'))
        bpy.data.objects.remove(co, do_unlink=True)
        print('  prévia %s (dia | noite)' % nome)


def luzes_de_fora():
    """As luzes que a loja joga para fora de noite: a porta de garagem, as duas janelas da frente e as duas
    vitrines dos arcos (cada uma um pouco para fora do vão, a meia altura dele)"""
    quente = (1.0, 0.72, 0.45)
    porta = diag((PORTA_T[0] + PORTA_T[1]) / 2, -1.8)
    luzes = [((porta[0], PISO_Y + 1.8, porta[1]), quente, 11.0, 1.5)]
    largura_da_janela = ARCO_DAS_JANELAS[-1][0]
    for x0 in JANELAS_DA_FRENTE:
        luzes.append(((x0 + largura_da_janela / 2, PISO_Y + 1.9, Z_FRENTE_FORA - 1.5), quente, 7.0, 0.9))
    luzes.append(((X_ARCO_FORA + 1.5, PISO_Y + 1.6, (ARCO_OESTE[0][0] + ARCO_OESTE[-1][0]) / 2), quente, 7.5, 1.0))
    luzes.append((((ARCO_SUL[0][0] + ARCO_SUL[-1][0]) / 2, PISO_Y + 1.6, Z_ARCO_FORA - 1.5), quente, 7.5, 1.0))
    return luzes


if NOITE:
    transplantar_cores(SAIDA + '.glb', os.path.join(PASTA_TEMP, 'noite_assada.glb'), SAIDA + '_noite.glb')
    conferir_noite(SAIDA + '.glb', SAIDA + '_noite.glb')
    LUZES_DE_FORA = luzes_de_fora()
    gravar_luzes_de_fora(LUZES_DE_FORA)
    if PREVIAS:
        print('Prévias (dia | noite) em %s...' % PREVIAS)
        porta_dentro = diag((PORTA_T[0] + PORTA_T[1]) / 2, RECUO_DA_DIAGONAL)
        de_fora = diag(6.1, -16.0)
        previas_dia_e_noite([
            ('dentro_movel', (473.0, PISO_Y + 1.9, 165.0), (463.5, PISO_Y + 2.2, 177.0), 18),
            ('dentro_porta', (462.5, PISO_Y + 2.2, 173.0), (porta_dentro[0], PISO_Y + 2.6, porta_dentro[1]), 16),
            ('entrando', (476.3, PISO_Y + 2.6, 161.0), (466.0, PISO_Y + 1.6, 169.5), 16),
            ('posteres_janela', (458.5, PISO_Y + 2.0, 163.0), (469.7, PISO_Y + 2.6, 151.5), 16),
            ('ala_sul', (468.5, PISO_Y + 2.0, 166.5), (484.0, PISO_Y + 2.0, 174.0), 16),
            ('nicho', (463.5, PISO_Y + 1.7, 171.0), (463.5, PISO_Y + 2.4, 177.0), 24),
            ('espelho', (466.5, PISO_Y + 2.0, 166.0), (474.2, PISO_Y + 2.3, 177.0), 18),
            ('fora_porta', (de_fora[0], PISO_Y + 3.2, de_fora[1]), (porta_dentro[0], PISO_Y + 2.5, porta_dentro[1]), 24),
            ('fora_vitrine', (494.0, PISO_Y + 2.5, 150.0), (476.0, PISO_Y + 2.5, 162.0), 22),
            ('fora_janelas', (462.7, PISO_Y + 2.6, 129.0), (462.7, PISO_Y + 3.2, 147.0), 24),
            ('fora_longe', (503.0, PISO_Y + 6.0, 134.0), (472.0, PISO_Y + 2.0, 160.0), 20),
        ], CASCA, LUZES_DE_FORA, OBJS)

shutil.rmtree(PASTA_TEMP, ignore_errors=True)
print('Pronto em %.0f s.' % (time.time() - INICIO))
