# KATCHAU

Jogo de corrida pseudo-3D em C com [raylib](https://www.raylib.com/). A pista é uma
lista circular duplamente encadeada de segmentos.

## Estado atual

- **Fase 0:** estrutura do projeto, `Makefile` e janela da raylib.
- **Fase 1:** pista como lista circular duplamente encadeada (`src/pista.c`), testada no terminal.
- **Fase 2:** estrada em pseudo-3D (`src/render.c`).
- **Fase 3:** carro do jogador (`src/carro.c`), corrida de 3 voltas com tempo por volta (`src/corrida.c`) e HUD simples (`src/hud.c`).
- **Fase 4:** 7 rivais com IA (`src/ia.c`), classificação refeita a cada quadro com Insertion Sort (`src/ordenacao.c`), colisão e posição no HUD.

## 1. Instalar a raylib

**macOS**
```sh
xcode-select --install     # compilador, se ainda não tiver
brew install raylib
```

**Linux (Ubuntu/Debian) e WSL**
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
make memoria  # roda os testes no valgrind (Linux/WSL)
make clean    # apaga os arquivos gerados
```

Para ver cada segmento da pista no teste: `./build/teste_pista -v`.

## 3. Controles

| Tecla | Ação |
|-------|------|
| ↑ ou W | Acelerar |
| ↓ ou S | Frear |
| ← → ou A D | Virar |
| ENTER | Correr de novo (depois da chegada) |
| ESC   | Sair |

Curvas fechadas jogam o carro para fora em alta velocidade: freie antes delas.
Na grama o carro não passa de ~75 km/h.

_(completar nas fases 5 e 6: turbo, menu)_

## Estrutura

```
src/
  main.c           laço do jogo: ler teclado, atualizar, desenhar
  config.h         constantes compartilhadas (TOTAL_VOLTAS, tamanho da tela...)
  pista.c/.h       lista circular duplamente encadeada de segmentos   (fase 1)
  render.c/.h      estrada pseudo-3D e carros na tela                  (fases 2 a 4)
  carro.c/.h       struct Carro, Comandos e movimento                  (fase 3)
  corrida.c/.h     grid, voltas, colisão, classificação e fim          (fases 3 e 4)
  ia.c/.h          decisões dos rivais                                 (fase 4)
  ordenacao.c/.h   Insertion Sort e Merge Sort                         (fases 4 e 6)
  obstaculos.c/.h  lista simples de obstáculos por segmento            (fase 5)
  hud.c/.h         velocímetro, tempo, posição, volta e mini-mapa      (fases 3 a 5)
  menu.c/.h        menu principal, garagem e tela de resultado         (fase 6)
testes/
  teste_pista.c    testa pista, carro, corrida, IA e ordenação no terminal
recursos/
  texturas/        PNGs do jogo (ver recursos/texturas/README.md)
    estrada/       asfalto, grama, zebra e faixa
    fundo/         céu, cidade e morros (camadas do horizonte)
    carro/         carro reto, virando à esquerda e à direita
    sprites/       árvores, arquibancadas, boxes, pórtico, pneus e placas
```

Regra: só `render.c`, `hud.c` e `menu.c` incluem `raylib.h` (além do `main.c`, que abre
a janela). O resto é lógica pura e pode ser testado no terminal.

## Os rivais

O jogador larga em último. Cada rival tem uma ficha técnica própria; a "volta limpa" é o
tempo de uma volta sozinho na pista, dirigido pela IA (o carro do jogador faz 31,4 s).

| Carro | Cor | Jeito | Volta limpa |
|---|---|---|---|
| Barão | amarelo | esportivo arrogante, voa na reta | 31,0 s |
| Neon | roxo | gruda nas curvas | 31,0 s |
| Trovão | azul | arranca forte | 31,4 s |
| Zé Pistão | laranja | certinho em tudo | 31,7 s |
| Fuscão | verde-água | lento, mas não sai da pista | 32,0 s |
| Bolinha | rosa | esperto na largada | 32,5 s |
| Ferrugem | marrom | picape enferrujada | 32,7 s |

## Onde a ordenação entra na lógica

`corrida.classificacao` é um vetor de 8 ponteiros para `Carro`, reordenado a cada quadro
por `ordenarCarrosPorDistancia()` (Insertion Sort). Ele é usado para:

1. **Posição** do jogador no HUD e na chegada (`posicaoDoCarro`).
2. **Carro da frente** de cada rival (`carroDaFrente`): é só o vizinho anterior no vetor.
   A IA usa isso para decidir quando ultrapassar.
3. **Fim de corrida:** a posição final sai direto do vetor ordenado.

Insertion Sort foi escolhido porque, de um quadro para o outro, a ordem quase não muda:
com o vetor quase ordenado ele faz no máximo 1 ou 2 deslocamentos (perto de O(n)). O teste 7
mostra isso: 0 deslocamentos com o vetor já ordenado, 1 numa ultrapassagem e 28 no pior caso
(vetor ao contrário, O(n²)).

## A pista

O autódromo tem 1593 segmentos e é corrido no sentido horário. Cada segmento guarda sua
curvatura em graus (negativo para a esquerda, positivo para a direita), e a soma de uma
volta dá 360 graus.

| Segmentos | Trecho |
|---|---|
| 0–199 | Reta principal (largada no 0) |
| 200–259 | Curva 1, fechada à direita |
| 260–612 | Reta, chicane (396–475) e reta |
| 613–792 | Curva 2, aberta à direita |
| 793–992 | Reta oposta |
| 993–1052 | Curva 3, fechada à direita |
| 1053–1412 | Reta, esse suave (1113–1352) e reta |
| 1413–1592 | Curva 4, aberta à direita |
