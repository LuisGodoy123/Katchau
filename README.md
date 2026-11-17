# KATCHAU

Jogo de corrida 3D em C com [raylib](https://www.raylib.com/), inspirado no filme **Carros** . A pista é uma **lista circular duplamente encadeada** de segmentos, e a
classificação da corrida é refeita a cada quadro com **Insertion Sort**.

---

## 1. Como executar

O jogo precisa da biblioteca **raylib** instalada. Depois dela, são dois comandos.

### Windows

1. Instale o pacote oficial da raylib (já vem com o compilador MinGW/w64devkit):
   https://raysan5.itch.io/raylib — ele instala em `C:\raylib`.
2. Compile a raylib uma vez, para a biblioteca ficar na mesma pasta dos headers:

   ```sh
   cd C:/raylib/raylib/src && make PLATFORM=PLATFORM_DESKTOP
   ```

3. Na pasta do jogo:

   ```sh
   make run
   ```

Dá para compilar de qualquer terminal (cmd, PowerShell ou `C:\raylib\w64devkit\w64devkit.exe`):
o Makefile encontra o gcc do w64devkit sozinho, sem mexer no PATH.

Se a raylib estiver em outro lugar: `make RAYLIB_DIR=caminho/para/raylib/src`.

### Linux (Ubuntu/Debian) e WSL

```sh
sudo apt install build-essential libasound2-dev libx11-dev libxrandr-dev \
     libxi-dev libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev libxinerama-dev
git clone --depth 1 https://github.com/raysan5/raylib.git
cd raylib/src && make PLATFORM=PLATFORM_DESKTOP && sudo make install
```

Depois, na pasta do jogo: `make run`.

### macOS

```sh
xcode-select --install
brew install raylib
```

Depois, na pasta do jogo: `make run`.

### Comandos do Makefile

```sh
make          # compila (gera ./katchau ou katchau.exe)
make run      # compila e abre o jogo
make testes   # roda os testes de terminal (não precisam da raylib)
make clean    # apaga os arquivos gerados
```

**Importante:** rode o jogo **a partir da pasta do projeto** (onde estão o `Makefile` e a pasta
`recursos/`). Os modelos, sons e fontes são carregados por caminho relativo, como
`recursos/sons/real_gone.mp3` (`src/audio.c:15`).

O jogo abre em tela cheia, na resolução do monitor. Para abrir numa janela 800×600:
`./katchau --janela`.

---

## 2. Controles

### Na corrida

| Tecla | Controle | Ação |
|-------|----------|------|
| ↑ ou W | R2 | Acelerar |
| ↓ ou S | L2 | Frear |
| ← → ou A D | Analógico esquerdo / direcional | Virar |
| SHIFT | Cruz | Freio de mão (com o volante virado, drift) |
| ESPAÇO | Bola | Nitro (segure); recarrega aos poucos, mais rápido freando |
| — | Analógico direito | Câmera livre em volta do carro |
| ENTER | Cruz | Ver o resultado, depois da chegada |
| ESC | Options | Pausar |

### Nos menus

| Tecla | Ação |
|-------|------|
| ↑ ↓ ou W S | Escolher opção |
| ← → ou A D | Na garagem: escolher o carro no carrossel |
| ↑ ↓ | Na garagem: trocar o critério de ordenação (velocidade, aceleração, aderência) |
| ENTER | Confirmar |
| ESC | Voltar |

Controle de Xbox ou PlayStation (USB ou Bluetooth) funciona junto com o teclado, sem configurar.

---

## 3. Estrutura dos arquivos

```
Makefile          Compilação (bibliotecas por sistema operacional)
README.md         Este arquivo
src/              Código-fonte (.c e .h)
testes/           Testes de terminal da lógica, sem raylib
recursos/         Modelos 3D, sons, texturas e fontes
```

O núcleo avaliado pela disciplina está em seis arquivos:

| Arquivo | O que tem |
|---------|-----------|
| `src/pista.c` / `pista.h` | A **lista circular duplamente encadeada** de segmentos e suas operações |
| `src/obstaculos.c` / `.h` | Lista simplesmente encadeada de objetos de cada segmento |
| `src/ordenacao.c` / `.h` | **Insertion Sort** e **Merge Sort** |
| `src/corrida.c` / `.h` | Regras da prova: voltas, classificação, colisão, chegada |
| `src/carro.c` / `.h` | Física e movimento de cada carro pela lista |
| `src/ia.c` / `.h` | Pilotos automáticos |

Os outros módulos (`render.c`, `mundo.c`, `hud.c`, `menu.c`, `efeitos.c` e companhia) são desenho
e som, construídos sobre essa lógica.

---

## 4. Como o código atende cada requisito

Os números de linha são desta versão do código.

### Requisito 2 — Menu de opções interativas

| Tela | Onde |
|------|------|
| Menu principal (JOGAR, GARAGEM, COMO JOGAR, CONFIGURAÇÕES, SAIR) | `src/menu.c:518-545`, textos em `src/menu.c:101` |
| Garagem (escolha do carro, com o critério de ordenação) | `src/menu.c:772-864` |
| Escolha da corrida (pista, voltas, dificuldade) | `src/menu.c:1308-1441` |
| Configurações | `src/menu.c:2252-2355` |
| Pausa | `src/menu.c:2810-2850` |
| Resultado da corrida | `src/menu.c:2902-2912` |

### Requisito 3 — Estrutura de dados na lógica central do jogo

A estrutura é uma **lista circular duplamente encadeada**: a pista. Cada nó é um pedaço curto de
asfalto, com a sua curvatura, largura, altura e superfície.

| O que | Onde |
|-------|------|
| O nó (`Segmento`, com `proximo` e `anterior`; o último aponta para o primeiro) | `src/pista.h:113-153` |
| A porta de entrada da lista (`Pista`, que guarda o nó da largada) | `src/pista.h:210-221` |
| A posição de qualquer coisa na pista: **em qual nó** e quanto andou nele | `src/pista.h:226-228` |
| O campo de posição dentro do carro | `src/carro.h:65` |

**A lista não é um depósito: ela é a pista.** A posição de cada carro *é* um nó da lista, e as
decisões abaixo exigem percorrer a lista a cada quadro:

| Decisão do jogo | Lógica | Onde |
|-----------------|--------|------|
| Andar na pista e **contar as voltas** | Soma a distância, pula para `proximo` (ou `anterior`, de ré) e conta +1 volta ao entrar no nó com `ehLinhaDeChegada` | `src/pista.c:1110-1133` |
| O quadro de cada um dos 17 carros | Chama o movimento acima e devolve se cruzou a linha | `src/carro.c:1431-1506` |
| **Força da curva** sobre o carro | Lê a `curvatura` do nó atual e joga o carro para fora | `src/carro.c:818-888` (leitura em `843`) |
| Asfalto, zebra, área de escape ou grama sob as rodas | Lê as medidas do nó e a posição lateral | `src/pista.c:1043-1060` |
| **A IA freia antes da curva** | Percorre os 80 próximos nós, lê a curvatura de cada um e calcula a velocidade que ainda permite frear a tempo | `src/ia.c:68-83` |
| Carro do jogador sem direção guiada | Caminha por `anterior`/`proximo` até achar o nó na perpendicular do carro, e percorre a volta inteira quando ele corta pela grama | `src/carro.c:1012-1034` e `1043-1072`, usados em `1177-1283` |
| Montar o grid de largada | Anda **para trás** a partir da largada, um nó por vez | `src/corrida.c:139-148` |
| Posicionar a câmera atrás do carro | Anda para trás na lista a partir do nó do carro | `src/render.c:220-291` |
| Desenhar o mini-mapa da pista | Percorre a volta acumulando as curvaturas | `src/hud.c:272-318` |

Sem percorrer a lista não existe volta, nem chegada, nem curva.

**Segunda estrutura:** cada segmento guarda os objetos pintados na sua beira (os colchetes do
grid) numa **lista simplesmente encadeada** — nó em `src/obstaculos.h:29-33`, inserção em
`src/obstaculos.c:24-35`, contagem em `:43-49`, liberação em `:57-65`, e o uso em `:78-87`. Essa
lista é armazenamento; a estrutura central do jogo é a lista circular da pista.

### Requisito 4 — Algoritmo de ordenação, e não apenas para ranking

Dois algoritmos, cada um escolhido pelo formato dos dados que ele recebe.

| Algoritmo | Implementação | Quem chama |
|-----------|---------------|------------|
| **Insertion Sort** — ordem de corrida dos 17 carros | `src/ordenacao.c:60-78`, com o comparador `estaNaFrente` em `:27-38` | `src/corrida.c:336` (montagem do grid) e `src/corrida.c:1255`, **a cada quadro**, dentro de `atualizarCorrida` (`:1176-1277`) |
| **Merge Sort** — fichas da garagem por atributo | entrada `src/ordenacao.c:209-223`, recursão `:183-193`, intercalação `:137-172` | `reordenarGaragem`, `src/menu.c:685-695`, acionado em `src/menu.c:389` e `:814` |

**Por que não é só um placar.** O jogo consome a **ordem inteira, 60 vezes por segundo** — não o
primeiro lugar. Para saber apenas quem venceu bastaria um mínimo, O(n); o que o jogo precisa é da
sequência completa e da **vizinhança** dentro dela:

| Quem usa a ordem | O que a ordem decide | Onde |
|------------------|----------------------|------|
| Painel de posições | A posição de cada um dos 17 carros e o **intervalo entre carros consecutivos**, calculado contra `classificacao[i - 1]` | `src/hud.c:512-577` (intervalo em `561`) |
| Câmera de TV | **Qual** carro filmar: o líder ou o vizinho imediatamente à frente do jogador no vetor ordenado | `src/render.c:1852-1865`, usada em `:1781-1843` |
| Onda da torcida | Dispara quando `classificacao[0]` chega à metade da prova | `src/render.c:988-1001` |
| Fim da prova | Posição de um carro, colocação final e vitória/pódio | `src/corrida.c:1316-1323` → `:1273` → `:1382-1384` |
| Garagem | **Qual carro cada posição do cursor seleciona**: trocar o critério reordena o carrossel, e o carro escolhido é `menu->garagem[selecionado]`, que é o carro que vai para a corrida | `src/menu.c:814` |

Na garagem a ordenação é parte da escolha, não da exibição: com outro critério, a mesma tecla
seleciona outro carro.

**Por que estes dois algoritmos.** O vetor da corrida chega **quase ordenado** a cada quadro — de
um quadro para o outro, no máximo uma ultrapassagem muda de lugar. O Insertion Sort é adaptativo:
nesse caso o laço interno quase não roda e o custo fica perto de O(n), enquanto obter a ordem
completa por mínimos sucessivos seria O(n²) por quadro. Para mostrar isso medido,
`ordenarCarrosPorDistancia` devolve quantos deslocamentos fez (`src/ordenacao.c:60-78`). Na
garagem é o contrário: trocar o critério muda a lista inteira de ordem, então não há "quase
ordenado" com que contar — daí o Merge Sort, O(n log n) em qualquer caso e **estável**, o que
mantém os carros empatados no critério novo na ordem do critério anterior
(`src/ordenacao.c:137-172`, no `>=` da intercalação).

### Requisito 5 — Pelo menos 6 funções que envolvam a estrutura de dados

São 23 (18 na lista circular, 3 na lista de obstáculos e 2 em `carro.c`).

**Lista circular duplamente encadeada (`src/pista.c`)**

| Função | Linhas | O que faz |
|--------|--------|-----------|
| `criarSegmento` | `50-92` | `malloc` de um nó solto, apontando para si mesmo nos dois sentidos |
| `renumerarSegmentos` | `102-117` | Percorre uma volta e reescreve o índice de cada nó |
| `moverALargada` | `125-135` | Anda a cabeça da lista N nós para a frente |
| `criarPistaVazia` | `147-165` | Cria a pista sem nenhum nó |
| `adicionarTrecho` | `181-191` | Insere N nós no fim, com curvatura e altura |
| `inserirSegmento` | `883-910` | Insere depois de um nó dado (ou no fim); trata a pista vazia |
| `removerSegmento` | `922-952` | Religa `anterior` ↔ `proximo`, trata nó único e remoção da largada, libera |
| `buscarSegmento` | `960-974` | Busca sequencial pelo índice, dando uma volta |
| `proximoSegmento` | `983-985` | O nó seguinte (depois do último vem a largada) |
| `segmentoAnterior` | `993-995` | O nó anterior |
| `contarSegmentos` | `1003-1016` | Conta os nós dando uma volta |
| `temZebra` | `1024-1033` | Lê os dados do nó |
| `superficieNaLateral` | `1043-1060` | Asfalto, zebra, escape ou grama naquele ponto do nó |
| `lateralDoLugar` | `1082-1093` | Anda para trás até o lugar do grid |
| `moverNaPista` | `1110-1133` | Anda uma distância pulando nós; devolve as voltas ganhas ou perdidas |
| `alturaNaPosicao` | `1142-1147` | Interpola a altura do nó com a do `proximo` |
| `liberarPista` | `1158-1179` | Corta o círculo e libera nó por nó |
| `pontoNoMundo` | `1320-1349` | Posição 3D interpolando o nó e o seguinte |

**Lista simplesmente encadeada (`src/obstaculos.c`)**

| Função | Linhas | O que faz |
|--------|--------|-----------|
| `adicionarObstaculo` | `24-35` | Insere no início da lista do nó, O(1) |
| `contarObstaculos` | `43-49` | Conta até o `NULL` |
| `liberarObstaculos` | `57-65` | Libera a lista de um nó |

**Caminhada pela lista em `src/carro.c`:** `acharNaLinha` (`1012-1034`) e `trechoMaisPerto`
(`1043-1072`).

### Requisito 6 — Principais funções especificadas

Além das tabelas acima, **toda** função do projeto tem um cabeçalho de comentário dizendo o que
ela faz, o que recebe e o que devolve — por exemplo `src/pista.c:1095-1109` (antes de
`moverNaPista`) e `src/ordenacao.c:40-59` (antes do Insertion Sort). Cada `.h` abre com um
comentário explicando o papel do módulo (`src/ordenacao.h:1-15`, `src/obstaculos.h:4-18`).

---
