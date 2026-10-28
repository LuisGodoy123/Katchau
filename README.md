# KATCHAU

Jogo de corrida pseudo-3D em C com [raylib](https://www.raylib.com/). A pista é uma
lista circular duplamente encadeada de segmentos.

## 1. Instalar a raylib

**macOS**
```sh
xcode-select --install     # compilador, se ainda não tiver
brew install raylib
```

**Linux (Ubuntu/Debian)**
```sh
sudo apt install build-essential git libasound2-dev libx11-dev libxrandr-dev \
     libxi-dev libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev libxinerama-dev
git clone --depth 1 https://github.com/raysan5/raylib.git
cd raylib/src && make PLATFORM=PLATFORM_DESKTOP && sudo make install
```

**Windows**
Instale o pacote oficial (raylib + MinGW) de https://raysan5.itch.io/raylib.
Ele coloca a raylib em `C:\raylib`. Compile pelo terminal do MinGW
(`C:\raylib\w64devkit\w64devkit.exe`). Se estiver em outra pasta:
`make RAYLIB_DIR=caminho/para/raylib/src`.

## 2. Compilar e rodar

```sh
make          # compila
make run      # compila e abre o jogo
make testes   # roda os testes de terminal (não precisam da raylib)
make clean    # apaga os arquivos gerados
```

## 3. Controles

| Tecla | Ação |
|-------|------|
| ESC   | Sair |

_(completar nas fases 3 a 6: acelerar, frear, virar, turbo, menu)_

## Estrutura

```
src/
  main.c           abre a janela e troca entre telas
  config.h         constantes compartilhadas (TOTAL_VOLTAS, tamanho da tela...)
  pista.c/.h       lista circular duplamente encadeada de segmentos   (fase 1)
  render.c/.h      estrada pseudo-3D e carros na tela                  (fase 2)
  carro.c/.h       struct Carro e movimento do jogador                 (fase 3)
  corrida.c/.h     voltas, colisão, vácuo, turbo e fim de corrida      (fases 3 a 5)
  ia.c/.h          decisões dos rivais                                 (fase 4)
  ordenacao.c/.h   Insertion Sort e Merge Sort                         (fases 4 e 6)
  obstaculos.c/.h  lista simples de obstáculos por segmento            (fase 5)
  hud.c/.h         velocímetro, tempo, posição, volta e mini-mapa      (fase 5)
  menu.c/.h        menu principal, garagem e tela de resultado         (fase 6)
testes/            programas de terminal das fases 1 e 4
recursos/          imagens, fontes e sons
```

Regra: só `render.c`, `hud.c` e `menu.c` incluem `raylib.h`. O resto é lógica pura
e pode ser testado no terminal.
