# Texturas do KATCHAU

Imagens PNG usadas pelo `render.c`. Elas são carregadas uma vez em
`carregarTexturas()` (chamada no `main.c`). Se uma imagem faltar, aquela
parte volta a ser desenhada com as cores sólidas (`COR_*` no `render.c`).

Já usadas: fundo (céu, cidade, morros), asfalto, grama e as três poses
do carro (os rivais são o mesmo sprite com o vermelho trocado pela cor
de cada um). A zebra e a faixa continuam com cores sólidas, e os
`sprites/` ficam para a Fase 5 (objetos na beira da pista).

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
| `estrada/` | `asfalto_claro.png`    | 64x64    | asfalto dos segmentos claros                |
|            | `asfalto_escuro.png`   | 64x64    | asfalto dos segmentos escuros               |
|            | `grama_clara.png`      | 64x64    | grama dos segmentos claros                  |
|            | `grama_escura.png`     | 64x64    | grama dos segmentos escuros                 |
|            | `zebra.png`            | 64x16    | zebra vermelha e branca da borda            |
|            | `faixa.png`            | 8x64     | faixa branca do meio da pista               |
| `fundo/`   | `ceu.png`              | 640x240  | céu com nuvens (camada mais distante)       |
|            | `morros.png`           | 640x120  | morros verdes (camada mais próxima)         |
|            | `cidade.png`           | 640x120  | prédios de São Paulo (camada do meio)       |
| `carro/`   | `carro_reto.png`       | 120x64   | carro (fórmula) de trás, reto               |
|            | `carro_esquerda.png`   | 120x64   | carro virando para a esquerda               |
|            | `carro_direita.png`    | 120x64   | carro virando para a direita                |
| `sprites/` | `arvore.png`           | 72x104   | árvore na beira da pista                    |
|            | `arquibancada.png`     | 200x104  | arquibancadas (A, B e M)                    |
|            | `boxes.png`            | 200x76   | prédio dos boxes na reta principal          |
|            | `portico.png`          | 240x110  | pórtico da largada/chegada                  |
|            | `pneus.png`            | 112x44   | barreira de pneus                           |
|            | `placa_50.png`         | 32x56    | placa de frenagem 50 m                      |
|            | `placa_100.png`        | 32x56    | placa de frenagem 100 m                     |
|            | `placa_150.png`        | 32x56    | placa de frenagem 150 m                     |

## Regras para adicionar texturas

- Só PNG, nome em minúsculas, sem acento e com `_` no lugar de espaço.
- Cada imagem vai na pasta do seu tipo; se não couber em nenhuma, crie
  uma pasta nova e anote aqui.
- Fundo transparente nos sprites e no carro (canal alfa).
- Texturas que repetem (asfalto, grama) devem emendar sem costura nas bordas.
