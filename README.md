# KATCHAU

Jogo de corrida pseudo-3D em C com [raylib](https://www.raylib.com/). A pista é uma
lista circular duplamente encadeada de segmentos.

## Estado atual

- **Fase 0:** estrutura do projeto, `Makefile` e janela da raylib.
- **Fase 1:** pista como lista circular duplamente encadeada (`src/pista.c`), testada no terminal.
- **Fase 2:** estrada em pseudo-3D (`src/render.c`).
- **Fase 3:** carro do jogador (`src/carro.c`), corrida de 3 voltas com tempo por volta (`src/corrida.c`) e HUD simples (`src/hud.c`).
- **Fase 4:** 7 rivais com IA (`src/ia.c`), classificação refeita a cada quadro com Insertion Sort (`src/ordenacao.c`), colisão e posição no HUD.
- **Fase 5 (em parte):** Interlagos com subidas e descidas, texturas, mini-mapa, zebras só nas curvas,
  linha de chegada quadriculada e objetos na beira da pista (`src/obstaculos.c`). Falta o turbo.
- **Fase 6:** menu principal, garagem com Merge Sort, como jogar, contagem regressiva e tela de resultado (`src/menu.c`).

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

O jogo abre em **tela cheia, na resolução do monitor**. Para abrir numa janela
800×600 (útil para testar e depurar): `./katchau --janela`.

A pista e os carros são desenhados na resolução real da tela; em monitores largos
(16:9), a pista não estica, só mostra mais dos lados. O HUD e os menus são
desenhados numa "tela virtual" de 800×600 que a câmera 2D da raylib amplia para o
tamanho real (`comecarInterface()` em `hud.c`).

## 3. Controles

**Na corrida**

| Tecla | Ação |
|-------|------|
| ↑ ou W | Acelerar |
| ↓ ou S | Frear |
| ← → ou A D | Virar (o volante gira aos poucos e volta sozinho ao centro) |
| ENTER | Ver o resultado (depois da chegada) |
| ESC   | Abandonar a corrida e voltar ao menu |

**Nos menus**

| Tecla | Ação |
|-------|------|
| ↑ ↓ ou W S | Escolher opção / carro |
| ← → ou A D | Na garagem: trocar o critério de ordenação |
| ENTER | Confirmar |
| ESC   | Voltar |

Para fechar o jogo, use **SAIR** no menu (ou feche a janela).

O carro acelera como um F1: forte na saída (0-100 km/h em ~2,3 s) e cada vez menos
perto da máxima (0-300 km/h em ~11 s).

Curvas fechadas jogam o carro para fora em alta velocidade: freie antes delas. Quando
sai **fumaça dos pneus**, a curva está quase vencendo o volante: alivie ou ele escapa.
Na grama (sai poeira) o carro não passa de ~75 km/h.

_(completar na fase 5: turbo)_

## As telas

`main.c` guarda em qual tela o jogador está (`Tela`, em `menu.h`) e, a cada quadro,
chama a função `atualizar...()` dela, que devolve para qual tela ir, e depois a
`desenhar...()`:

```
MENU ──> GARAGEM ──> MENU
  │──> COMO JOGAR ──> MENU
  └──> CORRIDA ──(chegada + ENTER)──> RESULTADO ──(ENTER)──> CORRIDA
          └──(ESC)──> MENU                   └──(ESC)──> MENU
```

Na tela de resultado a corrida continua ao fundo, e a tabela se completa conforme
os rivais terminam a prova.

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
  obstaculos.c/.h  objetos da beira: lista simples por segmento       (fase 5)
  hud.c/.h         posições, volta e setores, mini-mapa e velocímetro  (fases 3 a 6)
  menu.c/.h        menu, garagem, como jogar e tela de resultado       (fase 6)
  visual.c/.h      identidade visual: cores, fontes, logo e formas     (fase 6)
  audio.c/.h       música dos menus (toca nos menus, some na corrida)
testes/
  teste_pista.c    testa pista, carro, corrida, IA e ordenação no terminal
recursos/
  fontes/          Racing Sans One e Saira Condensed (licença OFL, arquivos *_licenca.txt)
  sons/            menu_theme.mp3, a música dos menus
  texturas/        PNGs do jogo (ver recursos/texturas/README.md)
    logo/          katchau.png, a logo do menu principal
    estrada/       asfalto, grama, zebra e faixa
    fundo/         céu, cidade e morros (camadas do horizonte)
    carro/         carro reto, virando à esquerda e à direita
    sprites/       árvores, pórtico, pneus e placas
ferramentas/
  gerar_sprites.py script do Blender: modelo 3D (.glb) vira sprite PNG
```

Regra: só `render.c`, `hud.c`, `menu.c`, `visual.c` e `audio.c` incluem `raylib.h` (além do `main.c`, que abre
a janela). O resto é lógica pura e pode ser testado no terminal.

## Sprites dos carros

O `render.c` procura as imagens dos carros em `recursos/texturas/carro/`, nesta ordem:

1. **Folha própria do carro:** `<numero>_folha.png` (ex.: `3_folha.png` para o Trovão), usada como está.
2. **Folha compartilhada:** `carro_folha.png`, repintada com a cor de cada carro (troca o
   vermelho da pintura pela cor dele; o carro 0 usa a original).
3. **Três poses antigas:** `carro_reto.png`, `carro_esquerda.png` e `carro_direita.png`.
4. Se nada disso existir, o carro é desenhado com formas simples.

A **folha** é uma grade: cada linha é um ângulo de volante (de cima para baixo: todo à
esquerda, ..., reto, ..., todo à direita) e cada coluna é um quadro do giro das rodas.
O jogo espera **9 linhas × 4 colunas** (`ANGULOS_DA_FOLHA` e `QUADROS_DA_RODA` no
`render.c`). A linha sai do volante gradual do carro; a coluna, da distância percorrida
(as rodas giram mais rápido quanto mais rápido o carro anda).

A vista de frente (`carro_frente.png` ou `<numero>_frente.png`) aparece na garagem e no
menu. As folhas e as vistas de frente usam *mipmaps*, para os rivais distantes não
ficarem serrilhados.

As folhas podem ser geradas a partir de um modelo 3D `.glb` com o script do Blender
`ferramentas/gerar_sprites.py` (as instruções estão no começo do arquivo). O mesmo
script gera os sprites do cenário (`--modo objeto`) e as camadas do fundo (`--modo fundo`):

```
BLENDER=/Applications/Blender.app/Contents/MacOS/Blender
$BLENDER -b -P ferramentas/gerar_sprites.py -- --modo carro \
    --modelo carro.glb --saida recursos/texturas/carro/carro_folha.png
$BLENDER -b -P ferramentas/gerar_sprites.py -- --modo objeto \
    --modelo arvore.glb --saida recursos/texturas/sprites/arvore.png
```

## Os carros

São 8 carros (`FICHAS` em `corrida.c`). Na garagem o jogador escolhe um; os outros 7
viram os rivais, e o jogador larga em último. A "volta limpa" é o tempo de uma volta lançada
(a segunda, já em movimento) em Interlagos, sozinho na pista e dirigido pela IA.

| Carro | Cor | Jeito | Volta limpa |
|---|---|---|---|
| Katchau | vermelho | o carro da casa, equilibrado (o padrão) | 55,9 s |
| Barão | amarelo | esportivo arrogante, voa na reta | 55,7 s |
| Neon | roxo | gruda nas curvas | 55,2 s |
| Trovão | azul | arranca forte | 55,8 s |
| Zé Pistão | laranja | certinho em tudo | 56,6 s |
| Fuscão | verde-água | lento, mas não sai da pista | 56,7 s |
| Bolinha | rosa | esperto na largada | 57,5 s |
| Ferrugem | marrom | picape enferrujada | 58,3 s |

## Onde a ordenação entra na lógica

### Insertion Sort: a classificação da corrida

`corrida.classificacao` é um vetor de 8 ponteiros para `Carro`, reordenado a cada quadro
por `ordenarCarrosPorDistancia()`. A regra de quem está na frente (`estaNaFrente`):
quem já cruzou a chegada fica na frente de quem ainda corre (pelo tempo de chegada);
entre os que correm, quem andou mais. O vetor é usado para:

1. **Posição** do jogador no HUD e na chegada (`posicaoDoCarro`).
2. **Carro da frente** de cada rival (`carroDaFrente`): é só o vizinho anterior no vetor.
   A IA usa isso para decidir quando ultrapassar.
3. **Tela de resultado:** a tabela de chegada é o próprio vetor, na ordem.

Insertion Sort foi escolhido porque, de um quadro para o outro, a ordem quase não muda:
com o vetor quase ordenado ele faz no máximo 1 ou 2 deslocamentos (perto de O(n)). O teste 7
mostra isso: 0 deslocamentos com o vetor já ordenado, 1 numa ultrapassagem e 28 no pior caso
(vetor ao contrário, O(n²)).

### Merge Sort: a garagem

A garagem guarda um vetor de 8 ponteiros para `FichaTecnica`, ordenado por
`ordenarFichasPorAtributo()` do maior para o menor valor do critério escolhido
(velocidade, aceleração ou aderência). Trocar o critério com ← → reordena a lista, e
o carro marcado continua marcado, só muda de linha.

Merge Sort foi escolhido porque, ao trocar o critério, a lista muda inteira de ordem:
não dá para contar com ela "quase ordenada". Ele custa O(n log n) em qualquer caso e
é **estável**: carros empatados no critério novo mantêm a ordem do anterior (teste 9).

## Largada e HUD

O grid fica **antes da linha de chegada**, em zigue-zague, com um colchete pintado no
asfalto em cada lugar (as medidas estão em `pista.h`: `DISTANCIA_DA_POLE`,
`ESPACO_ENTRE_LUGARES`). Os carros começam com `voltas = -1`: o primeiro cruzamento da
linha, logo depois da largada, só abre a volta 1, que (como no F1) conta desde a largada.

A volta é dividida em **3 setores** (terços da pista). Em `corrida.c`, cada carro guarda o
tempo de cada setor, o melhor setor dele e os setores da melhor volta; a corrida guarda o
melhor setor e a volta mais rápida de todos. O HUD mostra:

- **em cima à esquerda**, a tabela de posições: bandeira (largada, verde ou chegada), tempo
  de prova e, para cada carro, o intervalo para você (+ à frente, - atrás) e a última volta;
- **em cima à direita**, a sua volta: a diferença para a sua melhor volta no último setor
  (verde ganhando, vermelho perdendo), os tempos atual, última, melhor e a mais rápida da
  corrida, e os setores: **roxo** = melhor da corrida, **verde** = seu melhor, **amarelo** = mais lento;
- **embaixo à esquerda** o mini-mapa e **embaixo à direita** o velocímetro.

## Objetos da beira da pista

Cada segmento tem uma **lista simplesmente encadeada** de objetos (`Obstaculo`, em
`obstaculos.h`): o segmento aponta para o primeiro, e cada um aponta para o próximo.
`decorarPista()`, chamada no fim de `criarPista()`, põe numa volta pela pista:

- o **pórtico** em cima da linha de chegada;
- **barreiras de pneus** por fora das curvas e **placas de 150, 100 e 50 m** antes delas;
- **árvores** sorteadas dos dois lados (sorteio com semente fixa: ficam sempre no mesmo lugar).

Tudo fica além de onde o carro consegue chegar (lateral 2,5), menos o pórtico, que passa
por cima da pista. O `render.c` desenha os objetos junto com os rivais, do mais longe para
o mais perto; a imagem e a largura de cada tipo estão em `ARQUIVOS_OBJETO` e
`LARGURA_OBJETO`. A lista de cada segmento é liberada junto com ele (`liberarObstaculos`).

As **zebras** só aparecem nas curvas, como na vida real: um segmento tem zebra quando vira
pelo menos `CURVATURA_MINIMA_ZEBRA` graus (`temZebra()` em `pista.c`).

## A pista

A pista é Interlagos: 2587 segmentos (cerca de 4.309 m), corrida no sentido anti-horário.
Cada segmento guarda sua curvatura em graus (negativo para a esquerda, positivo para a
direita), a altura do chão e o nome do trecho. A soma das curvaturas de uma volta dá
-360 graus, e o desnível é de 56 m, do S do Senna à Descida do Lago.

| Segmentos | Trecho |
|---|---|
| 0–196 | Reta dos Boxes (largada no 0) |
| 197–294 | S do Senna |
| 295–443 | Curva do Sol |
| 444–802 | Reta Oposta |
| 803–964 | Descida do Lago |
| 1036–1214 | Ferradura |
| 1282–1345 | Laranjinha |
| 1437–1496 | Pinheirinho |
| 1582–1623 | Bico de Pato |
| 1691–1789 | Mergulho |
| 1856–1925 | Junção |
| 1926–2419 | Subida dos Boxes |
| 2420–2586 | Reta dos Boxes |

A lista completa, segmento a segmento, sai com `./build/teste_pista -v`.
