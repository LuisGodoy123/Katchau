# KATCHAU

Jogo de corrida 3D em C com [raylib](https://www.raylib.com/). A pista é uma
lista circular duplamente encadeada de segmentos.

## Estado atual

- **Fase 0:** estrutura do projeto, `Makefile` e janela da raylib.
- **Fase 1:** pista como lista circular duplamente encadeada (`src/pista.c`), testada no terminal.
- **Fase 2:** estrada em pseudo-3D (hoje em 3D de verdade, veja "O 3D").
- **Fase 3:** carro do jogador (`src/carro.c`), corrida de 3 voltas com tempo por volta (`src/corrida.c`) e HUD simples (`src/hud.c`).
- **Fase 4:** 16 rivais com IA (`src/ia.c`), classificação refeita a cada quadro com Insertion Sort (`src/ordenacao.c`), colisão e posição no HUD.
- **Fase 5 (em parte):** Interlagos de verdade (o traçado de 2023, num modelo 3D medido para o jogo), mini-mapa, zebras e muros do modelo,
  linha de chegada quadriculada e objetos na beira da pista (`src/obstaculos.c`). Falta o turbo.
- **Fase 6:** menu principal, garagem com Merge Sort, como jogar, contagem regressiva e tela de resultado (`src/menu.c`).
- **O jogo no mundo aberto:** no menu, **JOGAR** leva a Radiator Springs, e é lá que tudo acontece. As
  corridas começam no **Mack**, estacionado na rua principal, logo à frente da largada. Na vaga atrás da
  carreta, o jogador aperta **ENTER** (ou Y) para entrar: a porta de trás desce e vira rampa, o carro sobe e
  entra, a porta sobe, e aí vem a escolha da corrida. Escolhida, o Mack parte pela rua (uma cena curta, com a
  música do menu seguindo até a apresentação da pista); desistindo, o carro desce de ré pela rampa. No fim
  da corrida (ou saindo dela pela pausa), o carro desce do mesmo jeito, de volta na cidade, com o resultado
  no alto da tela. Por dentro, a carreta é o "camarim" do carro (`src/carreta.c`): chapa escura com neon
  roxo, prateleiras com troféus e miniaturas, quadros, teto estrelado e um telão. Os olhos do Mack são vivos
  como os dos carros (o mesmo shader e a mesma cara de `src/carros3d.c`): ele pisca, olha em volta,
  acompanha o carro do jogador e se assusta se bater nele. O modelo (`recursos/modelos/mack.glb`, as 10
  rodas em `mack_rodas.glb`, que giram quando ele anda, e a porta em `mack_porta.glb`) e as camadas dos
  olhos saem de `ferramentas/preparar_mack.py`, que redesenha os olhos como os do filme, amplia 4 vezes as
  texturas (filtro xBR do ffmpeg), separa as rodas e a porta e põe o caminhão em metros (30 m).
- **Os moradores de Radiator Springs** (`src/moradores.c`, o mapa deles em `src/moradores_mapa.h`): o McQueen
  (quando o jogador está com outro carro), o Mate, a Sally, o Doc, o Luigi, o Guido, o Ramone, a Flo, o Xerife,
  o Sargento, o Fillmore, a Lizzie e o Ruivo, cada um no seu lugar (a loja do Luigi, o café da Flo, o Cozy
  Cone, o ferro-velho, o quartel, o esconderijo do Xerife atrás do outdoor...) e com uma rotina de dia: vai a
  outros lugares (quase todos passam no café da Flo), fica um tempo e volta. Andam pela faixa da direita,
  seguindo o grafo das ruas, freiam atrás de quem vai para o mesmo lado na frente deles (cansando de esperar
  outro morador, seguem; dois que se encostam se empurram de lado, e ninguém fica travado), esperam a vez de
  sair para a rua, entram de ré nas vagas e são sólidos para o carro do jogador (batendo forte, eles se
  assustam). Cada um sorteia os programas pelo gosto dele, com tempos que variam: o Ramone vive no café da
  Flo, o xerife faz a ronda pela rua principal, o Doc chama o McQueen para treinar no oval, a Sally chama o
  McQueen para passear pela rodovia, o Mate passa no Cozy Cone para buscar o McQueen, o Sargento chama o
  Fillmore para o café, e o Guido vai sempre atrás do Luigi. Quem vai buscar alguém para na casa dele, os
  dois conversam e saem juntos; acabado o programa, cada um volta para a sua casa. Estacionados perto, eles
  conversam: olham um para o outro e falam na vez de cada um (o Luigi e o Guido, o Sargento e o Fillmore
  discutindo, a Flo e o Ramone...), e o jogador lê a conversa embaixo da tela quando chega perto. Com o
  jogador: olham para ele quando ele chega devagar, cumprimentam quando ele para do lado (o Ruivo, tímido, se
  encolhe), se assustam e reclamam quando ele passa raspando rápido, e o xerife, no esconderijo atrás do
  outdoor, vai atrás de quem passa a mais de 90 km/h (no minimapa, um ponto vermelho): parando, leva uma
  bronca; fugindo para longe, ele desiste. A Flo, o Ruivo, o Xerife e a
  Lizzie não correm: os modelos deles (`recursos/modelos/carros/flo.glb`, `ruivo.glb`, `xerife.glb`,
  `lizzie.glb`) saem de `ferramentas/preparar_figurantes.py` (que passa as texturas JPEG para PNG, porque a
  raylib do jogo não lê JPEG, separa os olhos pintados de cada um nas três camadas dos olhos vivos, redesenha
  os do Ruivo como os do filme e desvira a Lizzie, que vem do avesso) e entram em `carros3d.c` depois dos carros da corrida.
- **Passeio livre:** um mundo aberto de Radiator Springs (a cidade, o Willy's Butte, os cânions e, colada na
  cidade, a pista de treino de terra que veio do Cars 2), sem pista: o carro anda em qualquer direção
  (`src/livre.c`, que dirige com a direção livre da corrida, `dirigirSemPista` em `src/carro.c`) e o "para
  cima" dele é o do chão embaixo das rodas, então ele sobe barrancos e, rápido,
  anda nas paredes (wallride). O chão e as paredes vêm de uma malha de colisão em grade (`src/colisao.c`), que
  também diz o tipo de chão (asfalto, areia, terra, rocha): a aderência, a poeira e o som saem daí. A tela,
  com o minimapa e o mapa grande, fica em `src/passeio.c`. A pista de treino fica a ~530 m da rua principal,
  ligada por uma planície de areia plana (o mapa costura os dois com uma "emenda"); a areia
  da cidade é a mesma da arena do Cars 2 (a textura, na mesma escala) e tem os mesmos matinhos e cactos. Do
  lado da cidade, a pista de treino tem a mesa de pedra da arena, inteira e com a lombada vermelha em volta
  descendo até o chão da cidade (`ferramentas/montar_mundo_aberto.py` guarda as rochas da arena por peça e
  encaixa as beiradas baixas delas no chão, mas não a terra do oval nem o butte no meio dele; as texturas
  JPEG viram PNG, que a raylib do jogo não lê JPEG).

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
Ele coloca a raylib em `C:\raylib`. Dá para compilar de qualquer terminal (cmd, PowerShell ou
o `C:\raylib\w64devkit\w64devkit.exe`): o Makefile acha o gcc do w64devkit sozinho, sem
precisar ajustar o PATH.

Isso importa porque o `gcc` do PATH costuma ser o **MinGW 32-bit** de `C:\MinGW\bin`, que não
tem `pthread.h` e não linka com a raylib de 64 bits — e como o MinGW não traz `make.exe` (só
`mingw32-make.exe`), o `make` sai do w64devkit e o `gcc` sai do MinGW. O build então morre no
meio, em `src/mjpeg.c`, parecendo problema desse arquivo. Se acontecer, rode `make clean`
antes de compilar de novo: os objetos que ficaram são de 32 bits, e o link falharia com
`undefined reference to sincos`.

**Compile a raylib uma vez**, para a biblioteca vir da mesma árvore que os headers:

```sh
cd C:/raylib/raylib/src && make PLATFORM=PLATFORM_DESKTOP
```

Isso é obrigatório, e o Makefile falha com uma mensagem clara se faltar. O pacote oficial traz
os headers em `C:/raylib/raylib/src` **sem** a `libraylib.a` ao lado; sem ela, o `-lraylib` cai
numa cópia que o w64devkit traz, compilada de outra versão. O jogo então compila contra um
`raylib.h` e roda contra outro: a struct `Mesh` tem os campos em posições diferentes dos dois
lados, e o jogo quebra com SIGSEGV dentro do driver de vídeo ao subir uma malha que ele mesmo
monta (`UploadMesh` em `separarRodas`, `carros3d.c`) — um erro que não parece em nada com a
causa.

Em outras pastas: `make RAYLIB_DIR=caminho/para/raylib/src` e, se o w64devkit não estiver em
`C:/raylib/w64devkit/bin`, `make W64DEVKIT=caminho/para/w64devkit/bin` (ou `make W64DEVKIT=`
para usar o gcc do PATH).

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
800×600 (útil para testar e depurar): `./katchau --janela`. Para abrir direto na cidade, sem o menu:
`./katchau --passeio` (dá para juntar as duas).

O mapa do passeio (`recursos/modelos/radiator_springs.glb` e `.col`) é gerado no Blender, a partir dos
dois modelos de Radiator Springs do PS3, por `ferramentas/montar_mundo_aberto.py` (o uso está no começo
do arquivo). Ele põe os dois na mesma escala, puxa as cores do Race-O-Rama para as do Cars 2 e exporta
também os triângulos da colisão.

**Na cidade**, os comandos são os da corrida, mais: **R** (ou Y) volta para a rua principal,
**T** vai para a pista de treino, **M** (ou LB) abre o mapa (lá, **Z**/RB aproxima e **ENTER**/A leva à pista
de treino), **C** troca a câmera e **ESC** pausa. O velocímetro é o mesmo da corrida; no canto de baixo à
esquerda fica o minimapa (redondo, com o Mack marcado em laranja). Na vaga atrás do Mack, **ENTER** (ou Y)
entra nele. Nas cenas do Mack, **ENTER** (ou A) pula. A pista de treino fica além da planície de areia, ao sul
da rua principal; na frente da loja do Luigi, a torre de pneus é de pneus 3D. A câmera vem atrás de para onde o
carro está indo (a velocidade), como nos jogos de corrida: no drift, ela olha o caminho e o carro aparece de lado
(devagar ou de ré, ela vem atrás do bico). Na corrida sem a direção guiada é igual (`rumoDaViagem` em `carro.c`). No chão, o carro dirige com a mesma conta da corrida (a direção livre, sem assistência): o volante, a
curva, o freio, a ré, o nitro e o drift são iguais, em qualquer piso (a areia só segura um pouco a velocidade).
**Wallride:** encoste numa parede rápido (mais de ~40 km/h), de raspão, e vire o volante para o lado dela: o carro
sobe e anda nela enquanto tiver velocidade (abaixo de ~30 km/h ele solta). Na parede o controle é arcade: a câmera
deita junto com o carro (a parede vira chão na tela), o volante gira o carro na hora e ainda o empurra para o lado,
e a gravidade pesa só um terço. Virando para o chão, ele volta para o chão sozinho.

Para conferir a colisão de uma região do mapa (buracos, degraus, lugares onde o carro enrosca):
`make sondar-colisao` e `./build/sondar_colisao recursos/modelos/radiator_springs.col x0 z0 x1 z1`.

A pista e os carros são desenhados na resolução real da tela; em monitores largos
(16:9), a pista não estica, só mostra mais dos lados. O HUD e os menus são
desenhados numa "tela virtual" de 800×600 que a câmera 2D da raylib amplia para o
tamanho real (`comecarInterface()` em `hud.c`).

## 3. Controles

**Na corrida**

| Tecla | Controle | Ação |
|-------|----------|------|
| ↑ ou W | R2 | Acelerar |
| ↓ ou S | L2 | Frear |
| SHIFT | Cruz | Freio de mão (com o volante virado, drift) |
| ESPAÇO | Bola | Nitro (segure): recarrega aos poucos, e bem mais rápido freando |
| ← → ou A D | analógico esquerdo ou direcional | Virar (o volante gira aos poucos e volta sozinho ao centro) |
| | analógico direito | Câmera livre: gira em volta do carro; todo para cima olha para o alto e todo para baixo olha para trás num giro rápido (soltando, volta) |
| ENTER | Cruz | Ver o resultado (depois da chegada) |
| ESC   | Options | Pausar: a corrida e os sons congelam; escolha CONTINUAR ou VOLTAR À CIDADE (ESC de novo continua) |

O controle (Xbox, PlayStation ou parecido, por USB ou Bluetooth) funciona junto com o
teclado, sem configurar nada. Os nomes da tabela são os do controle de Xbox; no de
PlayStation ficam nos mesmos lugares (R2/✕, L2/□, ○). Nos menus, direcional ou analógico
andam, **A** confirma e **B** volta; com o controle ligado, as dicas da tela mostram os botões.

O **analógico é proporcional**: meio inclinado, o volante gira só até a metade (as setas e o
direcional sempre giram tudo). Tem uma folga pequena no centro (12%), e perto dele a resposta é
mais fina que no fim do curso, para corrigir a linha na reta sem ziguezague
(`direcaoDoAnalogico` em `controle.c`; o campo `direcao` dos `Comandos` leva o valor ao carro).

**Nos menus**

| Tecla | Ação |
|-------|------|
| ↑ ↓ ou W S | Escolher opção / carro |
| ← → ou A D | Na garagem: trocar o critério de ordenação |
| ENTER | Confirmar |
| ESC   | Voltar |

Para fechar o jogo, use **SAIR** no menu (ou feche a janela).

O carro acelera como um F1: forte na saída (0-100 km/h em ~3,5 s) e cada vez menos
perto da máxima (0-300 km/h em ~15 s).

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
  └──(JOGAR)──> CIDADE (passeio.c) ──(pausa: MENU PRINCIPAL)──> MENU
                  │  └──(ENTER na vaga do Mack: o carro sobe a rampa)──> PISTA ──(VOLTAR: desce a rampa)──> CIDADE
                  │                         └──(IR COM O MACK)──> CIDADE: o Mack parte ──> CORRIDA
                  └──(o carro desce da carreta)<──┐
CORRIDA ──(chegada + ENTER)──> RESULTADO ──(ENTER)──┤
   │                              └──(R)──> CORRIDA │
   └──(ESC)──> PAUSA                                │
                 ├──(CONTINUAR ou ESC)──> CORRIDA, de onde parou
                 └──(VOLTAR À CIDADE)───────────────┘
```

Na tela de resultado a corrida continua ao fundo, e a tabela se completa conforme
os rivais terminam a prova.

## Estrutura

```
src/
  main.c           laço do jogo: ler teclado, atualizar, desenhar
  config.h         constantes compartilhadas (TOTAL_VOLTAS, tamanho da tela...)
  pista.c/.h       lista circular duplamente encadeada de segmentos   (fase 1)
  render.c/.h      câmera, carros e sprites na corrida em 3D
  corte.c/.h       o corte pelo campo de visão: não manda desenhar o que está fora da tela
  mundo.c/.h       cenário 3D: os modelos das pistas e o que o jogo põe em cima deles
  carros3d.c/.h    modelos 3D dos carros, sombras e fotos para os menus
  fumaca.c/.h      fumaça dos pneus e poeira da grama
  noite.c/.h       noite da Copa Pistão: refletores, torcida e flashes
  copa_pistao.h    traçado da Copa Pistão, medido no modelo 3D
  oval.h, splat.h  traçado e lugares da torcida dos outros dois modelos 3D
  interlagos.h     traçado, bordas, muros, pit lane e torcida de Interlagos (gerado: veja ferramentas/)
  interlagos_materiais.h  o que é cada material do modelo de Interlagos (chão e jeito de desenhar)
  tracado.h        o formato dessas tabelas
  carro.c/.h       struct Carro, Comandos e movimento                  (fase 3)
  corrida.c/.h     grid, voltas, colisão, classificação e fim          (fases 3 e 4)
  ia.c/.h          decisões dos rivais                                 (fase 4)
  ordenacao.c/.h   Insertion Sort e Merge Sort                         (fases 4 e 6)
  obstaculos.c/.h  objetos da beira: lista simples por segmento       (fase 5)
  hud.c/.h         posições, volta e setores, mini-mapa e velocímetro  (fases 3 a 6)
  menu.c/.h        menu, garagem, como jogar e tela de resultado       (fase 6)
  passeio.c/.h     a cidade (o mundo aberto), o Mack e as cenas da ida e da volta das corridas
  carreta.c/.h     o lado de dentro da carreta do Mack (neon, troféus, quadros, telão)
  moradores.c/.h   os moradores andando pela cidade (lógica: rotina, caminho, fila); o mapa em moradores_mapa.h
  visual.c/.h      identidade visual: cores, fontes, logo e formas     (fase 6)
  olhar.c/.h       câmera livre: transforma o analógico direito em ângulos
  motor.c/.h       rotação, marcha e carga do motor (câmbio de cada carro; também comanda o som)
  audio.c/.h       música, efeitos dos menus, largada, falas do McQueen
  efeitos.c/.h     sons do carro e da corrida: motores NASCAR (jogador e rivais), vento, pneu,
                   grama, muro, nitro, câmbio, voltas e pit stop
  controle.c/.h    controle (gamepad): comandos do carro e botões dos menus
testes/
  teste_pista.c    testa pista, carro, corrida, IA e ordenação no terminal
ferramentas/
  medir_interlagos.c     mede o modelo de Interlagos e escreve src/interlagos.h (make medir-interlagos)
  preparar_carros.py     prepara os modelos do Luigi, do Wingo, do Jackson Storm, do Boost e do DJ (Blender)
  preparar_figurantes.py prepara os da Flo, do Ruivo, do Xerife e da Lizzie (Blender)
  gerar_som_da_zebra.py  gera recursos/sons/efeitos/zebra.wav
recursos/
  fontes/          Racing Sans One e Saira Condensed (licença OFL, arquivos *_licenca.txt)
  sons/            música, efeitos dos menus e falas do McQueen (*.wav, *.mp3)
    nascar/        motores V8 estilo NASCAR em várias rotações (loops) e o motor da largada
    efeitos/       vento, pneu, grama, muro, nitro, câmbio, voltas e pit stop
  texturas/        PNGs do jogo (ver recursos/texturas/README.md)
    logo/          katchau.png, a logo do menu principal
    estrada/       asfalto, grama, zebra e faixa
    fundo/         céu, cidade e morros (camadas do horizonte)
    carro/         carro reto, virando à esquerda e à direita
    sprites/       árvores, pórtico, pneus e placas
```

Regra: só `render.c`, `corte.c`, `mundo.c`, `carros3d.c`, `fumaca.c`, `noite.c`, `hud.c`, `menu.c`, `visual.c`, `audio.c`, `efeitos.c` e `controle.c` incluem `raylib.h` (além do `main.c`, que abre
a janela). O resto é lógica pura e pode ser testado no terminal.

## Os carros

São 17 carros (`FICHAS` em `corrida.c`), todos modelos 3D em `recursos/modelos/carros/`. Na garagem o
jogador escolhe um; os outros 16 viram os rivais, e o jogador larga em último. A "volta limpa" é o tempo de
uma volta lançada, sozinho na pista e dirigido pela IA, usando o nitro.

| Carro | Jeito | Interlagos | Copa Pistão |
|---|---|---|---|
| McQueen | o carro da casa, equilibrado (o padrão) | 73,2 s | 39,5 s |
| The King | veterano, voa na reta | 73,1 s | 37,6 s |
| Doc Hudson | lenda, gruda nas curvas | 73,0 s | 40,7 s |
| Francesco | formula, arranca forte | 72,9 s | 39,6 s |
| Sally | certinha em tudo | 73,1 s | 40,3 s |
| Ramone | estiloso e estável | 73,1 s | 41,1 s |
| Luigi | pequeno e esperto na largada | 72,9 s | 41,8 s |
| Guido | minúsculo, mas não erra curva | 72,8 s | 43,4 s |
| Mater | guincho enferrujado | 73,1 s | 42,6 s |
| Sarge | jipe militar, firme no chão | 73,1 s | 42,2 s |
| Fillmore | sem pressa e sem sair da pista | 73,1 s | 43,0 s |
| Finn McMissile | espião, rápido em tudo | 72,7 s | 39,2 s |
| Wingo | tuner, nervoso na arrancada | 72,8 s | 40,0 s |
| Snot Rod | motorzão, difícil de segurar | 72,7 s | 38,8 s |
| Jackson Storm | nova geração, voa na reta | 73,4 s | 39,3 s |
| Boost | líder dos tuners, arrancada brava | 73,1 s | 39,6 s |
| DJ | tuner do som, liso nas curvas | 73,1 s | 39,8 s |

`carros3d.c` carrega os modelos e deixa todos iguais: mede cada um, escala para o comprimento do personagem
(`MODELOS`), centraliza, põe as rodas no chão e a frente para +z. O carro no mundo segue a pista, o
volante e o chão embaixo das rodas (`matrizDoCarro` em `render.c`). A sombra é a foto do próprio modelo visto de cima, desfocada, numa malha
que acompanha o chão ponto a ponto (`desenharSombraDoCarro` em `render.c`), sem inclinar com o carro: uma sombra plana afundava na pista inclinada da Copa Pistão;
e a fumaça e a poeira são nuvens soltas no mundo (`fumaca.c`). As rodas giram de verdade: `carros3d.c` acha, ao carregar, as peças redondas encostadas no chão, separa cada roda
do resto do modelo e a desenha girando em volta do próprio eixo; as da frente também viram com o volante. Na arrancada elas patinam, e o giro na tela
tem um limite, porque acima dele a roda pareceria girar ao contrário. A garagem e o menu usam uma foto de cada modelo, tirada ao carregar.

Os modelos são do Sketchfab, de DinseyPixarCarsModels e LukeMcQueen02 (o Luigi), publicados como CC-BY-4.0. Os
personagens são de *Carros* (Disney/Pixar). Nos arquivos, as texturas JPEG viraram PNG, porque a raylib da Homebrew não
lê JPEG. O Luigi, o Wingo, o Jackson Storm, o Boost e o DJ passam por `ferramentas/preparar_carros.py`, que também separa
os olhos pintados nas três camadas dos olhos vivos, desvira as peças do avesso (o jogo não desenha o lado de trás das
faces: o Wingo aparecia oco, e o Luigi antigo tinha a luz do lado errado) e põe o DJ, que vinha deitado, de frente para +z.

## Pilotagem

A física é de fliperama (`carro.c`), mas tem algumas coisas que recompensam quem pilota bem:

- **Freio de verdade.** O freio tira ~5 g, como um F1: de 300 a 100 km/h são uns 90 m, então
  as placas de 150, 100 e 50 m antes das curvas servem para achar o ponto de frenagem. Tirar o
  pé quase não freia (só o ar e o atrito: ~1 g a 300 km/h, bem menos em baixa), então quem
  quer perder velocidade usa o freio. A IA calcula, pela mesma física, de onde precisa começar
  a frear para cada curva (`velocidadeParaFrearAte`).
- **Câmbio.** Todo carro tem um câmbio automático de 7 marchas (`motor.c`, dentro do `Carro`),
  que sobe na linha vermelha (8.000 rpm). Em cada troca a potência é cortada por 0,16 s: dá
  para sentir o "tranco" de cada marcha.

- **O pé muda a curva.** Acelerando, o carro "sai de frente" e é empurrado para fora com
  força total; aliviando, o empurrão cai 10%, e freando, 20%. O certo é frear antes, aliviar no
  meio e acelerar na saída. A IA faz o mesmo: só acelera na curva abaixo do limite de quem acelera
  (`velocidadeSeguraAcelerando`).
- **Passar do limite custa velocidade.** Quando a curva empurra mais do que o volante segura,
  o pneu arrasta e o carro perde velocidade, proporcional a quanto passou. Com menos velocidade
  o empurrão cai e o carro volta a obedecer. Entrar quente é uma aposta, não grama na certa.
- **Nitro.** Segurar o botão empurra forte e leva a máxima até 354 km/h (300 + 18%), por até ~4,5 s, com uma chama azul nos
  escapamentos e um arco com um raio ao lado do velocímetro. O tanque enche em 50 s andando e em ~7 s freando, então cada freada também é recarga. Os
  rivais usam nas retas livres, com pelo menos 60% no tanque.
- **Vácuo.** Até ~30 m atrás de outro carro, na mesma linha, o ar segura menos: a velocidade
  máxima sobe até 6% (300 → 318 km/h) e aparece **VÁCUO** em cima do velocímetro. Para passar,
  é só sair de lado antes de bater na traseira. Saindo do vácuo, o excesso vai embora aos poucos.

O teste 17 confere o pé na curva, a derrapagem e o vácuo; o teste 18, o freio, o alívio e o câmbio.

**Colisão e rodada.** Cada carro é uma caixa com o tamanho real do modelo, que acompanha o carro virado (`meiasMedidasDoCarro`), e
vale em toda a volta, até com quem está uma volta atrás. Encostando, os dois se separam pelo lado em que entraram menos: por trás, quem
vem atrás passa velocidade para o da frente; de lado, os dois levam um tranco e perdem velocidade. Uma batida forte fora do centro (na quina da
traseira, ou de lado por trás do centro) faz o carro rodar: ele gira, ninguém o controla, perde velocidade sem parar de vez e se endireita sozinho, em
1 a 3 s, com 3 s sem poder rodar de novo. Só rodam batidas com o jogador envolvido: entre rivais elas fechariam a pista longe dos olhos dele.

**Equilíbrio.** Os carros têm jeitos diferentes, mas o mesmo ritmo: a aderência de cada um foi calculada por simulação para a volta limpa em
Interlagos dar o mesmo tempo, e a velocidade máxima varia só 2%, porque ela decide o oval. Quem decide a corrida é o piloto. Os rivais têm
uma `habilidade`, de 93% do limite do carro (a pole) a 86% (o último do grid, antes do jogador); para deixar o jogo mais fácil ou difícil, mude
`HABILIDADE_DA_POLE` e `HABILIDADE_DO_ULTIMO` em `corrida.c`.

## Pitstop

Em Interlagos, nos níveis médio e difícil, a parada nos boxes é obrigatória: quem cruza a última bandeirada sem parar é desclassificado. As garagens ficam logo depois da linha de chegada: quem entra no pit na última volta cruza a linha antes de parar, então pare antes dela.

- **Entrar:** no fim da Subida dos Boxes, onde acaba o guard-rail da esquerda e começam as faixas brancas que separam o pit lane, com as rodas da esquerda em cima da linha branca (`querEntrarNoPit`). Ali o piloto automático assume, no limite de 100 km/h perto das garagens. A linha de corrida da Subida passa pela esquerda, então só entra quem encosta de propósito na linha (`soEncostadoNaBorda`).
- **O caminho:** é o pit lane do modelo, medido (`PIT_INTERLAGOS` em `interlagos.h`): passa na frente das garagens, logo depois da linha de chegada, corre por dentro do S do Senna e da Curva do Sol e volta à pista na Reta Oposta. A tabela medida é alisada (média dos vizinhos) e, longe da vaga, o carro anda exatamente por ela, sem atraso; o passo na pista é corrigido para ele andar no mundo o que a velocidade manda (por dentro de uma curva, um metro da linha da pista vale menos), o bico aponta para onde ele andou de verdade (com um giro máximo) e a câmera vem atrás do bico. Nas curvas do caminho ele anda no máximo na velocidade que as faz com 25 m/s² de lado, freando com calma antes delas, e antes da manobra até a vaga já chega devagar: nada de balanço, câmera de lado ou trancos na saída.
- **A troca:** cada carro tem uma vaga (as 17 cabem na frente das garagens). Na do jogador, o Guido passa por todas as rodas em 3,2 s (`faseDaTroca` e `lugarDoGuido` em `pitstop.c`, desenho em `render.c`), e a imagem corta para uma câmera ao lado do carro.
- **Rivais:** cada um para numa volta (`voltaDoPit`) e leva mais tempo na troca que o Guido. No pit lane há fila: quem vem atrás reduz, quem sai da vaga espera passar quem já vem na faixa, e a colisão só separa os carros, sem tranco nem rodada; o carro parado na troca nunca é empurrado.

A Copa Pistão ainda não tem pit lane: lá não há parada.

## Configurações

No menu principal, **CONFIGURAÇÕES** troca as teclas e os botões de cada comando (duas teclas e dois botões para cada um) e as opções do controle: ligar a vibração, a força dela, o tremor no acelerador, a zona morta do analógico, a resposta do volante e inverter a câmera. Tudo fica em `configuracoes.txt`, salvo a cada mudança; apagar o arquivo volta ao padrão. A lógica está em `src/configuracoes.c`, e os menus e a pausa não mudam de tecla.

## Curvas na mão do piloto

A posição do carro é medida em relação à pista, então, sem cuidado, ele acompanharia a curva sozinho. Duas regras evitam isso (`atualizarLateral` em `carro.c`):

- **Curva leve pede volante:** quanto mais folga o pneu tem, mais o carro segue reto se ninguém virar (até o dobro do empurrão). No limite do pneu nada muda, e a velocidade segura de cada curva é a mesma.
- **O bico não vira sozinho:** o que falta de volante vira um ângulo para fora (`anguloDeFuga`), no carro e na câmera. Só no desenho: quem manda no caminho é a conta da física.

## Vibração no controle

A raylib não tem vibração no macOS, então o jogo fala direto com o sistema de controles da Apple (`src/vibrar_mac.m`). A lógica fica separada e testada em `src/vibracao.c`. Com `make testar-vibracao` dá para sentir cada efeito no controle, sem abrir o jogo. Fora do macOS o jogo não vibra.

| O que acontece | O que se sente |
|---|---|
| Asfalto | sempre presente quando o carro anda: textura que cresce com a velocidade, com as juntas do piso de vez em quando |
| Pneu escorregando | o asfalto some das mãos, o carro flutua, e entra um tremor irregular na mão direita |
| Drift | tranco quando a traseira solta, o peso na mão de fora da curva, e outro tranco quando o pneu agarra |
| Nitro | empurrão ao ligar, combustão pulsando enquanto queima, engasgos com o tanque no fim e uma cauda ao apagar |
| Freada | o peso indo para a frente e, nas freadas fortes, o ABS na mão direita |
| Arrancada | a roda patinando, na mão direita e no gatilho do acelerador |
| Zebra e grama | pulsos no ritmo do carro; tremor irregular |
| Batida, muro, rodada | tranco proporcional à diferença de velocidade; raspar; sacudir |
| Marchas | um toque a cada troca, maior na reduzida |
| Boxes | o macaco subindo e descendo, e a pistola do Guido em cada roda |
| Largada e chegada | um toque a cada número, e um tranco no GO e na bandeirada |

## Níveis, pneus e drift

Na escolha da corrida (no Mack) se escolhe a pista e o nível (`NIVEIS` em `corrida.c`). O vermelho marca o que está escolhido, e o contorno amarelo, onde o cursor está; confirmar passa para a linha seguinte, e só o IR COM O MACK larga:

| Nível | Voltas | Rivais (habilidade) | Pitstop | Pneus gastam |
|---|---|---|---|---|
| Fácil | 3 | 0,87 a 0,80 | não | não |
| Médio | 5 | 0,93 a 0,86 | sim (em Interlagos) | não |
| Difícil | 8 | 0,97 a 0,91 | sim (em Interlagos) | sim (em Interlagos) |

- **Pneus:** o desgaste cresce com o caminho (um jogo dura cerca de 5 voltas) e 5 vezes mais rápido derrapando. Pneu gasto segura menos nas curvas e tira um pouco da velocidade máxima; a troca do Guido deixa os pneus novos. Os rivais também gastam.
- **Freio de mão** (Shift, ou Cruz no controle): tira velocidade. Um toque com o volante virado quebra a traseira (`atualizarDrift` em `carro.c`), e o carro fica de lado até 55°. Depois quem manda é o pé: acelerando de lado o drift continua, sem acelerar ele acaba, e o volante controla o ângulo, que tem inércia; virar para o outro lado é o contraesterço. De lado o carro vai para onde o bico aponta (até um limite, para não sair da pista), solta fumaça, gasta pneu e perde velocidade, mais quanto mais de lado.
- **Câmbio** (`motor.c`): cada marcha é bem mais longa que a anterior, e a nova nasce entre 6.200 e 6.400 rpm, longe do corte de 8.000. A reduzida depende do pé: freando reduz cedo, com uma acelerada de punta-taco; aliviando, deixa a marcha longa; acelerando com o motor fraco, pula direto para a marcha que tem força.

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
- **embaixo à esquerda** o mini-mapa e **embaixo à direita** o painel do motor, minimalista, tudo
  dentro de um mostrador redondo: um conta-giros de 0 a 9 mil rpm com ponteiro e faixa vermelha em volta,
  a velocidade grande no meio (amarela no vácuo) e a marcha num círculo vermelho embaixo (N no grid;
  pisca em branco perto do limite e dá um pulo quando troca). O câmbio é automático, de 7 marchas (`motor.c`; cada carro tem o seu): sobe a marcha quando o
  ponteiro chega na linha vermelha, em **8.000 rpm** (`RPM_DA_TROCA` em `motor.h`, que também marca
  onde começa a faixa vermelha do mostrador). As marchas de cada carro cobrem frações da própria
  velocidade máxima. O motor do jogador também comanda o som.

## Vitória

Chegando em **1º lugar** na última volta, `segundosAteAVitoria()` (`corrida.c`) estima quanto
falta para a linha (distância que falta ÷ velocidade). Uns 6 s antes, `audio.c` começa
`life_is_a_highway.mp3` bem baixo, já no ponto `1:25 - segundos que faltam`, e vai corrigindo
se a estimativa escorregar; no quadro da bandeirada a música é cravada em **1:25** (o refrão)
e sobe ao volume máximo, no lugar da música de fundo. Na mesma hora sai o **confete**
(`visual.c`). Em qualquer posição, ao cruzar a linha, a IA assume o carro do jogador, que segue dando
voltas atrás do painel de chegada e da tela de resultado. Essas voltas extras não entram nos tempos.

## Objetos da beira da pista

Cada segmento guarda os seus objetos numa **lista simples** (`obstaculos.c`). Hoje eles são os colchetes do grid, pintados
no asfalto das pistas cujo modelo não traz o grid pintado (`decorarOval`). Em Interlagos o grid, a linha de chegada, as
zebras, os muros, os alambrados, as arquibancadas, os boxes, as árvores e os prédios em volta já estão no modelo 3D; o
jogo põe por cima:

- **treze telões**: cinco fixos (`TELOES_DE_INTERLAGOS` em `mundo.c`), quatro do lado dos boxes, virados para as
  arquibancadas da reta e da Subida (o pé vai no chão, ou no telhado, do que estiver ali), e um no fim da reta, para quem
  chega no S do Senna; e um de frente para cada arquibancada que o jogo levanta (dois na da Reta Oposta, que é comprida),
  do outro lado da pista, além do muro (`colocarTeloesDasArquibancadas`): o lugar foge de outros trechos da pista e da
  estrada do pit lane. Todos passam a transmissão da corrida;
- a **torcida** em cada assento das arquibancadas do modelo (`TORCIDA_INTERLAGOS`), com os carrinhos 3D perto da câmera;
- as **vagas pintadas** no pit lane, na frente das garagens;
- as **arquibancadas que o modelo não tem** (`ARQUIBANCADAS_QUE_FALTAM` em `mundo.c`): o modelo só traz as da reta e da
  Subida (setores A, B e M do mapa oficial do GP); as de fora do S do Senna, da Curva do Sol, da Reta Oposta e da Descida
  do Lago (D, H, N, R, G, S e V) são **cópias da arquibancada coberta da reta**: uma seção inteira dela (37,5 m, com o
  concreto, as fileiras, as escadas, o alambrado, os pilares e a cobertura de treliça, com os materiais e as imagens do
  próprio modelo) é recortada do modelo (os triângulos que passam da borda são cortados nela), fatiada a cada 2,5 m e
  repetida ao longo do trecho, cada fatia acompanhando a curva da pista. As cópias entram no modelo como malhas a mais,
  então são desenhadas, fazem e recebem sombra como o resto dele; os lugares da torcida da seção vão junto. A frente
  fica 6 m além do muro mais longe do trecho, o pé no chão do modelo, e as paredes descem 10 m para dentro do terreno,
  para nada flutuar onde ele desce.

As **zebras**: em Interlagos são as do modelo, medidas de cada lado (`zebraDireita`, `zebraEsquerda` no segmento). Em cima
delas o carro está na pista, mas sente: o carro pula de leve nos degraus, o controle vibra e toca o ronco da zebra
(`zebra.wav`). Nas outras pistas, um segmento tem zebra quando vira pelo menos `CURVATURA_MINIMA_ZEBRA` graus (`temZebra()`
em `pista.c`).

O **chão** de cada lado (`superficieNaLateral` em `pista.c`): asfalto, zebra, área de escape asfaltada e grama. Na grama o
carro não passa de 75 km/h e levanta poeira; na área de escape, de 150 km/h, sem poeira (as duas contam como fora da
pista). Os **muros** são os do modelo: cada segmento sabe a quantos metros fica o que para o carro de cada lado
(`limiteDireita`, `limiteEsquerda`); encostar raspa e, rápido, é batida. Onde não há barreira, o carro vai até um pouco
além da área de escape.

## O 3D

A lógica é a mesma nas duas pistas: a lista circular, a corrida, o carro e a IA não usam a
raylib. Cada segmento sabe onde está no mundo, em metros (`xMundo`, `zMundo`, `altura`, `rumo`,
largura de cada lado e `perfil` do chão), e `pontoNoMundo()` (`pista.c`) converte qualquer posição na pista
em um ponto 3D. O desenho usa isso:

- `mundo.c` carrega e desenha o cenário fixo: o modelo 3D de cada pista e o que o jogo põe em cima dele;
- `render.c` põe a câmera atrás do carro, andando pela lista, desenha os objetos como sprites que encaram
  a câmera e coloca os carros (`carros3d.c`) no mundo.

**A física ainda é a antiga:** o carro não tem direção própria, só a posição lateral na pista.

## As pistas

Escolhidas no Mack, na cidade (`IdPista` em `pista.h`).

### Copa Pistão

O oval do modelo 3D `recursos/modelos/copa_pistao.glb`, desenhado inteiro, como veio. O traçado
(`copa_pistao.h`) foi medido nele: a pista é a faixa entre os dois muros de concreto, e a altura real do asfalto
foi medida em 9 lugares da largura, ponto a ponto, porque o piso não é um plano (a faixa de dentro é reta e a de fora,
inclinada). Cada carro é assentado pelas quatro rodas (`matrizDoCarro` em `render.c`). A tabela guarda uma linha lisa a distância fixa do muro de fora e a largura
de cada lado. A inclinação das curvas (até ~8°) sai do perfil medido, e a câmera a acompanha. O carro para nos
muros, dos dois lados, e raspar custa velocidade. A pista do filme não tem medida oficial, então a volta tem o comprimento
real de Daytona, 2,5 milhas ou 4.023 m (`COMPRIMENTO_COPA_PISTAO`), e o modelo é ampliado até caber nisso
(~43 m por unidade). O modelo não tem proporções reais: com esse tamanho, as arquibancadas ficam bem
mais altas e a pista mais larga que as de verdade.

A corrida é **à noite** (`noite.c`), com céu estrelado. Quem ilumina tudo são os 32 **refletores** (`refletor.glb`) no alto das
arquibancadas, virados para o centro: a pista, os carros e a torcida recebem a luz deles no shader, conforme a distância e o
quanto cada superfície está virada para cada refletor. A **torcida** é desenhada no shader dos assentos do próprio modelo: cada
lugar sorteia uma cor pela posição, então são dezenas de milhares de torcedores sem geometria nova, e os **flashes**
das câmeras acendem em pontos sorteados dos assentos.

Modelo: [Motor Speedway of the South](https://sketchfab.com/3d-models/motor-speedway-of-the-south-b6406f62b7b94580844fb8508cc72148),
de DinseyPixarCarsModels, publicado como CC-BY-4.0. O estádio é do universo de *Carros* (Disney/Pixar) e
traz os logotipos do filme. A única mudança no arquivo foi converter uma textura JPEG para PNG, porque
a raylib da Homebrew não lê JPEG.

### Direção guiada

O carro anda pela lista de segmentos, então a pista faz a curva por ele; o que o tira da linha é o empurrão para
fora, que cresce com o quadrado da velocidade. Devagar, ele quase contorna a curva sozinho. Isso é a direção
guiada, e vem ligada. Desligada nas configurações, vale só para o jogador: o carro guarda para onde o bico aponta
(`desvioDoRumo`), a curva da pista aumenta a diferença e só o volante a tira, então sem volante ele segue reto
em qualquer velocidade. A IA sempre dirige com ela ligada, inclusive com o carro do jogador depois da chegada.
No Splat ela não existe (`semDirecaoGuiada`): lá o jogador sempre dirige sem ela, seja qual for a opção. E a
pista não tem muro invisível: cada lado para no que existe de verdade (`LIMITES_SPLAT`, medido no modelo): muro,
barranco, buraco, outro trecho da corrida, os pneus das ruas fechadas ou, no máximo, 22 m de área de escape na grama.

Sem a direção guiada, o carro anda no mundo (`andarNoMundo`): tem lugar e rumo próprios, e só o volante muda o rumo,
até o que o pneu segura naquela velocidade. Sem volante, o caminho é uma reta. A pista só é consultada depois, para
saber em que segmento ele está e a quantos metros da linha. O carro e a câmera usam esse rumo.

Para investigar como o carro se comporta, `KATCHAU_TELEMETRIA=1 ./katchau` grava cada quadro da corrida em `telemetria.csv`.

### Medir o desempenho

`KATCHAU_QUADROS=1 ./katchau` escreve no log, a cada 5 segundos e no fim, o tempo médio de quadro, o do 1% pior
(onde aparecem os trancos, que a média esconde) e o maior. Antes de medir, desligue a sincronia vertical e o limite
de quadros em Configurações: com eles ligados, o que se mede é o monitor, não o jogo.

Para saber **onde** está o tempo, o caminho confiável não é cronometrar blocos — num OpenGL emulado, parte do custo
de uma chamada de desenho acontece dentro dela, na mesma thread, e parte fica para o fim do quadro. O que funciona é
desligar um sistema de cada vez e comparar: `KATCHAU_SEM_CENARIO`, `KATCHAU_SEM_CARROS`, `KATCHAU_SEM_FUMACA`,
`KATCHAU_SEM_MARCAS` (`render.c`), `KATCHAU_SEM_TELAO` (`telao.c`) e `KATCHAU_SEM_TORCIDA3D` (`torcida3d.c`).

### Efeitos de tela

A cena 3D é desenhada numa imagem e passa por um último shader (`posprocesso.c`): brilho em volta das luzes (os faróis,
os refletores da Copa, o nitro e os telões), um tom de cor para cada pista (tarde quente em Interlagos, noite azulada
na Copa), vinheta e, perto da velocidade máxima e no nitro, desfoque nas bordas. A pintura dos carros reflete o céu,
mais forte nas bordas (`carros3d.c`), e as telas dos telões brilham sozinhas. Em Configurações, GRÁFICOS escolhe entre
ALTO (sombras e efeitos), MÉDIO (só efeitos) e BAIXO.

### Telões e placar

Na Copa Pistão, os telões (o cubo pendurado no meio e o telão do alto da arquibancada) passam a transmissão da
corrida, umas 20 vezes por segundo: a cada 6 s a câmera troca de carro (o jogador, o líder, quem está logo à frente
dele, alguém do pelotão) e de ângulo (perseguição, on-board no teto, lateral, helicóptero, frontal), com as legendas
de TV. O placar da torre e do telão do alto mostra a volta do líder e os quinze primeiros. As imagens do modelo são
trocadas por RenderTextures (`telao.c`); a câmera de TV fica em `render.c`.

Nas outras pistas, o telão é uma estrutura 3D (`objetos/telao.glb` e `telao_katchau.glb`, gerados por
`build/exp/gerar_telao3d.py`): duas torres de treliça com sapatas, o gabinete com a tela, a passarela de manutenção,
as caixas de som e o letreiro KATCHAU com lâmpadas. A tela traz uma imagem de espera só dela; ao carregar, o jogo troca
essa imagem pela transmissão (`ligarTeloesDoCenario` em `mundo.c`). A transmissão é desenhada sem as sombras do sol.

Desenhar a transmissão é desenhar o mundo inteiro outra vez (o cenário e os 17 carros), numa imagem de 512x312. Como
Interlagos tem 5 telões em 6 km de pista, isso só é feito quando alguma tela está no campo de visão do jogador: no
resto da volta, o que apareceria nela é invisível. `teloesAVista` (`mundo.c`) guarda a caixa de cada tela no
carregamento e `atualizarTelao` (`render.c`) pergunta antes de desenhar. `KATCHAU_SEM_TELAO=1 ./katchau` desliga a
transmissão de vez, para comparar o desempenho.

### Torcida 3D

Em todas as pistas, os lugares das arquibancadas perto da câmera ganham carrinhos de verdade (`torcida3d.c`): o
completo (76 triângulos, `objetos/carro_torcida.glb`) até 200 m e um simples (28 triângulos,
`carro_torcida_longe.glb`) de 200 a 400 m; os dois são gerados por `build/exp/gerar_torcida.py`. Cada um fica
virado para a pista, com cor sorteada e pulando, e cada grupo vai para a placa de vídeo numa chamada só
(os lugares ficam na placa de vídeo desde a carga; por quadro, o processador só escolhe os trechos da grade perto
da câmera, e o shader gira, posiciona e escala cada carrinho). Os lugares vêm dos degraus do estádio (Copa Pistão) e
dos lugares medidos nos modelos (Interlagos, Oval e Splat); uma grade de células acha os lugares perto sem
olhar todos. Mais longe continua a torcida pintada, que some onde os carrinhos aparecem. Na onda da Copa Pistão,
os faróis deles acendem. `KATCHAU_SEM_TORCIDA3D=1 ./katchau` desliga, para comparar o desempenho.

### Marcas de pneu e sombras

Quando o pneu escorrega (drift, derrapagem, arrancada patinando, rodada e freada forte em alta), cada roda deixa um
rastro escuro no asfalto, que some aos poucos (`marcas.c`). Na grama não marca.

De dia, o cenário faz sombra do sol (a face que a câmera vê, se estiver de costas para o sol, fica na própria
sombra: assim os trilhos do guard-rail e os muros finos não ficam serrilhados): a cada quadro, o que fica num quadrado de 280 m em volta da câmera é desenhado
visto do sol num mapa de profundidade, e o shader do cenário escurece o que estiver atrás de alguma coisa. À noite,
na Copa Pistão, não há sombra do sol. Dá para desligar em Configurações, se o computador ficar lento.

### A onda da torcida

Na Copa Pistão, quando o líder chega à metade da corrida, a torcida de carrinhos faz a onda: começando um pouco
à frente do jogador, os carros de cada setor acendem os faróis e disparam as câmeras, um setor depois do outro,
até dar a volta inteira no estádio (7 s). Os faróis são fileiras de pares de luzes amareladas sobre os assentos
(`acenderFarol` em `noite.c`); o clarão de leve do setor fica no shader da torcida.

### Oval e Splat

O Splat tem muito mais ruas do que a corrida usa. A rota escolhida (2,1 km) sai da reta dos boxes, passa por cima
da ponte, vira no cruzamento para o laço de cima, volta por baixo da ponte, desce pelos esses e fecha pela reta;
ficaram de fora o grampo do lado oeste e as esquinas mais fechadas. A curva mais fechada tem 30 m de raio.

Duas pistas de dia que vêm prontas em um modelo 3D (`recursos/modelos/oval.glb` e `splat.glb`). O traçado e
os lugares da torcida (`oval.h`, `splat.h`) foram medidos nos modelos, e a arquibancada recebe uma figura em
cada lugar, com o mesmo shader de torcida da Copa Pistão. O pit lane de cada uma é o que o modelo mostra: no
oval, entre o muro dos boxes e as garagens; no Splat, a estrada paralela à reta, na frente do prédio das
garagens. Na Copa Pistão não há garagens: a equipe arma as tendas no pátio por dentro da Reta Oposta.

As árvores e estruturas dos dois mapas (`cenario_mapas.h`) usam os modelos de `recursos/modelos/objetos/`, mais pinheiros
e um telão próprio. Cada lugar foi conferido contra a malha do modelo: arquibancadas e camarotes nas retas,
olhando para a pista; caminhões no paddock; carros no estacionamento; postos de fiscal e torres de TV em
volta do circuito; e a mata em uma cortina acompanhando a pista e em manchas mais longe.

Os modelos têm mais ruas do que a corrida usa. Onde a estrada continua além do limite da pista, uma fileira de
pneus fecha a passagem, deixando só as aberturas do pit lane. No Splat, os três pórticos de checkpoint saíram.

Modelos:
- [Cartoon Race Track - Oval](https://sketchfab.com/3d-models/cartoon-race-track-oval-f88b33a3a65c4965b03578dc7f4f6eb4),
  de [RCC Design](https://sketchfab.com/retrovalorem), publicado como CC-BY-4.0. Alterado para o jogo: árvores,
  pneus e arquibancadas com menos polígonos, imagens em 1024 pixels, malhas juntadas por material e sem os
  cones, as barreiras e o muro que ficavam em cima do asfalto do oval. As árvores do modelo foram trocadas
  pelas do jogo, nos mesmos lugares.
- [Splat Track](https://sketchfab.com/3d-models/splat-track-9878cad018a9440c8cfa2ab2c9d1eda7),
  de [Geedtopia](https://sketchfab.com/Geedtopia), publicado como **CC-BY-NC-4.0: só para uso não comercial**.
  Alterado para o jogo: sem os pórticos de checkpoint, malhas juntadas por material, e o magenta que marcava os pixels transparentes das
  imagens trocado pela cor média de cada uma, senão ele vazava para as folhas.

### Interlagos

O autódromo de verdade, com o traçado de 2023, num modelo 3D (`recursos/modelos/interlagos.glb`). O modelo está em metros,
e o jogo usa ele 40% maior (`ESCALA_INTERLAGOS` em `pista.h`): na medida real o asfalto tem de 10 a 16 m de largura e,
com 14 carros, quase não dá para ultrapassar (um piloto perfeito, com carro 5% melhor, chegava em 10º). Para correr na
medida real, é só pôr a escala em 1. Os pneus duram as mesmas ~5 voltas em qualquer escala (`escalaDoDesgaste` em
`corrida.c`).

O modelo tem 177 malhas, e o jogo só manda para a placa de vídeo as que podem aparecer na tela — em média umas 78
(`corte.c`). Não é economia de triângulos: é economia de chamadas de desenho, porque cada malha custa uma troca de
material, de shader e de buffers. As malhas são separadas por tipo de desenho no carregamento
(`prepararCaixasDoMapa` em `mundo.c`), com a caixa de cada uma já em coordenadas do mundo. O mesmo corte vale para o
mapa de sombra (que enxerga um quadrado de 330 m) e para a transmissão dos telões.

O traçado foi **medido no modelo** por `ferramentas/medir_interlagos.c` (`make medir-interlagos`), que escreve
`src/interlagos.h`. A ferramenta carrega o modelo com a raylib e:

- anda pela pista a partir da linha de chegada, de 2 em 2 m, achando as duas bordas do asfalto e corrigindo o ponto para o
  meio delas. A pista é o asfalto com a borracha do traçado por cima (o pit lane não tem borracha), mais a linha branca da
  borda; onde outro asfalto encosta (o pit lane, o traçado antigo do Bico de Pato), a borda que salta é ignorada;
- mede em cada ponto a largura, a altura do asfalto em 9 lugares da largura, a zebra, a área de escape, a altura do chão
  além da borda (de 1 a 90 m, em 13 distâncias) e a distância até o muro de cada lado (um raio de lado, a 35 cm e a 90 cm do chão);
- anda pelo pit lane, da entrada à saída, e guarda onde ele fica em relação à pista;
- acha os assentos das arquibancadas, para a torcida.

`interlagos_materiais.h` diz o que é cada um dos 177 materiais do modelo: o chão que o carro sente (pista, zebra, escape,
grama, pit, muro) e o jeito de desenhar. O modelo veio de outro jogo, onde o canal alfa das imagens tem vários usos: na
maioria é o brilho (ignorado), nas folhas é o recorte, nos alambrados é transparência (de longe eles viram um véu, não uma
parede cinza), nas linhas pintadas é a tinta sobre o asfalto e na borracha do traçado é o quanto escurece. O asfalto e a
grama usam as imagens do jogo, presas ao chão do mundo. O terreno em volta é uma foto de satélite, borrada de perto:
dentro da área do autódromo ele vira a grama do jogo, e a foto só aparece ao longe (`definirAreaDoAutodromo`). O muro de concreto também trazia as paredes invisíveis do jogo
original (triângulos de 20 a 45 m de altura): elas são tiradas ao carregar. As imagens gastas do modelo (o concreto
manchado e o enferrujado, o guard-rail enferrujado) davam um ar de abandonado: o concreto (inclusive o muro com musgo do
canal da Descida do Lago, que antes sumia e deixava ver o vazio atrás) vira um concreto limpo gerado pelo jogo
(`gerarConcretoLimpo`), e o guard-rail usa a imagem do novo. O terreno do modelo acaba a algumas centenas de metros
da pista: a **saia** (`construirSaiaDoTerreno`) estica a borda de fora dele até o horizonte, para não aparecer o vazio
além dela. O lago ganhou uma cor de água (no modelo ele é azul-claro, quase da cor do céu).

Interlagos tem 3625 segmentos (6.042 m na escala do jogo; 4.315 m no modelo, contra 4.309 m de verdade), corrida no
sentido anti-horário, com o asfalto de 13 a 27 m de largura. A soma das curvaturas de uma volta dá -360 graus, e o
desnível é de 59 m (43 m de verdade), do S do Senna à Descida do Lago.

| Segmentos | Trecho |
|---|---|
| 0–220 | Reta dos Boxes (largada no 0) |
| 221–399 | S do Senna |
| 400–631 | Curva do Sol |
| 632–1134 | Reta Oposta |
| 1135–1397 | Descida do Lago |
| 1660–1844 | Ferradura |
| 1909–1999 | Laranjinha |
| 2000–2150 | Pinheirinho |
| 2252–2355 | Bico de Pato |
| 2415–2580 | Mergulho |
| 2702–2780 | Junção |
| 2781–3425 | Subida dos Boxes |
| 3426–3624 | Reta dos Boxes |

O modelo é o "Interlagos circuit 2023 layout", baixado do Sketchfab (veio como "failed import", sem o processamento do
site). Ele traz a marca e texturas de um jogo comercial (Automobilista, da Reiza Studios): antes de publicar o jogo,
confira a licença.

A lista completa, segmento a segmento, sai com `./build/teste_pista -v`.
