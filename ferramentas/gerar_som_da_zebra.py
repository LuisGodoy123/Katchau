"""
gerar_som_da_zebra.py
------------------------------------------------------------------
Gera recursos/sons/efeitos/zebra.wav: o ronco do pneu passando pela
zebra. São batidas graves e curtas, uma por faixa, com um chiado de
borracha por cima. O arquivo tem um número inteiro de batidas, então
toca em loop sem emenda; o jogo muda a velocidade dele conforme a do
carro (efeitos.c).

Uso: python3 ferramentas/gerar_som_da_zebra.py recursos/sons/efeitos/zebra.wav
------------------------------------------------------------------
"""

import math
import random
import struct
import sys
import wave

TAXA = 44100
SEGUNDOS = 1.0
BATIDAS_POR_SEGUNDO = 36      # na velocidade de referência (o jogo acelera ou freia isso)
TOM_DA_BATIDA = 85.0          # Hz: o corpo grave de cada batida
QUEDA_DA_BATIDA = 38.0        # quanto a batida morre por segundo
CHIADO = 0.18                 # a borracha raspando nos degraus


def gerar(arquivo):
    random.seed(2026)
    total = int(TAXA * SEGUNDOS)
    periodo = TAXA / BATIDAS_POR_SEGUNDO
    amostras = []
    for i in range(total):
        dentro = (i % periodo) / TAXA   # segundos desde a última batida
        batida = math.sin(2 * math.pi * TOM_DA_BATIDA * dentro) * math.exp(-QUEDA_DA_BATIDA * dentro)
        chiado = (random.random() * 2 - 1) * CHIADO * math.exp(-60.0 * dentro)
        amostras.append(0.8 * batida + chiado)
    maior = max(abs(a) for a in amostras)
    with wave.open(arquivo, "wb") as saida:
        saida.setnchannels(1)
        saida.setsampwidth(2)
        saida.setframerate(TAXA)
        saida.writeframes(b"".join(struct.pack("<h", int(a / maior * 0.9 * 32767)) for a in amostras))


if __name__ == "__main__":
    gerar(sys.argv[1] if len(sys.argv) > 1 else "recursos/sons/efeitos/zebra.wav")
