# Texturas do KATCHAU

Imagens PNG usadas pelo `render.c`. Elas são carregadas uma vez em
`carregarTexturas()` (chamada no `main.c`). Se uma imagem faltar, aquela
parte volta a ser desenhada com as cores sólidas (`COR_*` no `render.c`).

Já usadas: fundo (céu, cidade, morros), asfalto, grama e as três poses
do carro (os rivais são o mesmo sprite com o vermelho trocado pela cor
de cada um) e os `sprites/` (objetos na beira da pista, ver
`obstaculos.c`). A zebra continua com cores sólidas (só nas curvas), e a
faixa não é mais usada.

Os caminhos são relativos à raiz do projeto, que é de onde o jogo roda
(`./katchau` ou `make run`). Exemplo na raylib:

```c
Texture2D carro = LoadTexture("recursos/texturas/carro/carro_reto.png");
/* ... */
UnloadTexture(carro);   /* antes do CloseWindow() */
```

## Pastas

| Pasta      | Arquivo                | Tamanho  | Uso                                         |
|------------|------------------------|----------|---------------------------------------------|
| `estrada/` | `asfalto_claro.png`    | 256x256  | asfalto dos segmentos claros                |
|            | `asfalto_escuro.png`   | 256x256  | asfalto dos segmentos escuros               |
|            | `grama_clara.png`      | 256x256  | grama dos segmentos claros                  |
|            | `grama_escura.png`     | 256x256  | grama dos segmentos escuros                 |
|            | `zebra.png`            | 256x64   | zebra vermelha e branca da borda            |
|            | `faixa.png`            | 8x64     | faixa branca do meio da pista               |
| `fundo/`   | `ceu.png`              | 2560x960 | céu com nuvens (camada mais distante)       |
|            | `morros.png`           | 2560x480 | morros verdes em 3 camadas (a mais próxima) |
|            | `cidade.png`           | 2560x480 | prédios de São Paulo (camada do meio)       |
| `carro/`   | `carro_reto.png`       | 120x64   | carro (fórmula) de trás, reto               |
|            | `carro_esquerda.png`   | 120x64   | carro virando para a esquerda               |
|            | `carro_direita.png`    | 120x64   | carro virando para a direita                |
| `sprites/` | `arvore.png`           | 288x416  | árvore na beira da pista                    |
|            | `portico.png`          | 960x440  | pórtico da largada/chegada                  |
|            | `pneus.png`            | 896x288  | barreira: pilhas de pneus deitados          |
|            | `placa_50.png`         | 128x224  | placa de frenagem 50 m                      |
|            | `placa_100.png`        | 128x224  | placa de frenagem 100 m                     |
|            | `placa_150.png`        | 128x224  | placa de frenagem 150 m                     |
| `logo/`    | `katchau.png`          | 1774x887 | logo do menu principal (`visual.c`)         |

## Como gerar de novo

Os **morros** (`fundo/morros.png`) são desenhados por código em
`ferramentas/gerar_morros.py`: três camadas de cristas lisas, no estilo da
cidade, que emendam sem costura nas bordas. Para mudar a altura, as cores ou
o formato, mude as `CAMADAS` no script e rode (precisa de Python 3 com numpy;
o Python do Blender já tem):

A **barreira de pneus** (`sprites/pneus.png`) sai de `ferramentas/gerar_pneus.py`:
6 pilhas de 5 pneus deitados, vistas de lado e um pouco de cima, com o pneu do topo
pintado (vermelho e branco, alternando). O número de pilhas, de pneus, as cores e o
achatamento ficam nas constantes do começo do script.

```sh
PY=/Applications/Blender.app/Contents/Resources/5.2/python/bin/python3.13
$PY ferramentas/gerar_morros.py
$PY ferramentas/gerar_pneus.py
```

As outras imagens não têm script no repositório: para trocá-las, substitua o PNG
mantendo o nome.

O `render.c` não depende do tamanho das imagens: a grama usa a largura da
textura e o fundo converte para a largura de referência `LARGURA_FUNDO`
(640). As texturas repetidas usam mipmaps, para não tremerem ao longe.

## Regras para adicionar texturas

- Só PNG, nome em minúsculas, sem acento e com `_` no lugar de espaço.
- Cada imagem vai na pasta do seu tipo; se não couber em nenhuma, crie
  uma pasta nova e anote aqui.
- Fundo transparente nos sprites e no carro (canal alfa).
- Texturas que repetem (asfalto, grama) devem emendar sem costura nas bordas.
