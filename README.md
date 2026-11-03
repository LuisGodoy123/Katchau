# KATCHAU

Jogo de corrida 3D em C com [raylib](https://www.raylib.com/). A pista é uma
lista circular duplamente encadeada de segmentos.

## Estado atual

- **Fase 0:** estrutura do projeto, `Makefile` e janela da raylib.
- **Fase 1:** pista como lista circular duplamente encadeada (`src/pista.c`), testada no terminal.
- **Fase 2:** estrada em pseudo-3D (hoje em 3D de verdade, veja "O 3D").
- **Fase 3:** carro do jogador (`src/carro.c`), corrida de 3 voltas com tempo por volta (`src/corrida.c`) e HUD simples (`src/hud.c`).
- **Fase 4:** 13 rivais com IA (`src/ia.c`), classificação refeita a cada quadro com Insertion Sort (`src/ordenacao.c`), colisão e posição no HUD.
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

| Tecla | Controle | Ação |
|-------|----------|------|
| ↑ ou W | RT ou A | Acelerar |
| ↓ ou S | LT ou X | Frear |
| ESPAÇO | RB ou B | Nitro (segure): recarrega aos poucos, e bem mais rápido freando |
| ← → ou A D | analógico esquerdo ou direcional | Virar (o volante gira aos poucos e volta sozinho ao centro) |
| | analógico direito | Câmera livre: gira em volta do carro; todo para cima olha para o alto e todo para baixo olha para trás num giro rápido (soltando, volta) |
| ENTER | A | Ver o resultado (depois da chegada) |
| ESC   | START | Pausar: a corrida e os sons congelam; escolha CONTINUAR ou MENU PRINCIPAL (ESC de novo continua) |

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
  └──> PISTA ──(ESC)──> MENU
         └──(ENTER)──> CORRIDA ──(chegada + ENTER)──> RESULTADO ──(ENTER)──> CORRIDA
          └──(ESC)──> PAUSA                  └──(ESC)──> MENU
                        ├──(CONTINUAR ou ESC)──> CORRIDA, de onde parou
                        └──(MENU PRINCIPAL)───> MENU
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
  mundo.c/.h       cenário 3D: o modelo do estádio e as malhas de Interlagos
  carros3d.c/.h    modelos 3D dos carros, sombras e fotos para os menus
  fumaca.c/.h      fumaça dos pneus e poeira da grama
  noite.c/.h       noite da Copa Pistão: refletores, torcida e flashes
  copa_pistao.h    traçado da Copa Pistão, medido no modelo 3D
  carro.c/.h       struct Carro, Comandos e movimento                  (fase 3)
  corrida.c/.h     grid, voltas, colisão, classificação e fim          (fases 3 e 4)
  ia.c/.h          decisões dos rivais                                 (fase 4)
  ordenacao.c/.h   Insertion Sort e Merge Sort                         (fases 4 e 6)
  obstaculos.c/.h  objetos da beira: lista simples por segmento       (fase 5)
  hud.c/.h         posições, volta e setores, mini-mapa e velocímetro  (fases 3 a 6)
  menu.c/.h        menu, garagem, como jogar e tela de resultado       (fase 6)
  visual.c/.h      identidade visual: cores, fontes, logo e formas     (fase 6)
  olhar.c/.h       câmera livre: transforma o analógico direito em ângulos
  motor.c/.h       rotação, marcha e carga do motor (câmbio de cada carro; também comanda o som)
  audio.c/.h       música, efeitos dos menus, largada e o som do motor do carro
  controle.c/.h    controle (gamepad): comandos do carro e botões dos menus
testes/
  teste_pista.c    testa pista, carro, corrida, IA e ordenação no terminal
recursos/
  fontes/          Racing Sans One e Saira Condensed (licença OFL, arquivos *_licenca.txt)
  sons/            música dos menus e da vitória, efeitos dos menus e motor da largada (*.wav, *.mp3)
  texturas/        PNGs do jogo (ver recursos/texturas/README.md)
    logo/          katchau.png, a logo do menu principal
    estrada/       asfalto, grama, zebra e faixa
    fundo/         céu, cidade e morros (camadas do horizonte)
    carro/         carro reto, virando à esquerda e à direita
    sprites/       árvores, pórtico, pneus e placas
ferramentas/
  gerar_sprites.py script do Blender: modelo 3D (.glb) vira sprite PNG
```

Regra: só `render.c`, `mundo.c`, `carros3d.c`, `fumaca.c`, `noite.c`, `hud.c`, `menu.c`, `visual.c`, `audio.c` e `controle.c` incluem `raylib.h` (além do `main.c`, que abre
a janela). O resto é lógica pura e pode ser testado no terminal.

## Os carros

São 14 carros (`FICHAS` em `corrida.c`), todos modelos 3D em `recursos/modelos/carros/`. Na garagem o
jogador escolhe um; os outros 13 viram os rivais, e o jogador larga em último. A "volta limpa" é o tempo de
uma volta lançada, sozinho na pista e dirigido pela IA, usando o nitro.

| Carro | Jeito | Interlagos | Copa Pistão |
|---|---|---|---|
| McQueen | o carro da casa, equilibrado (o padrão) | 52,2 s | 39,5 s |
| The King | veterano, voa na reta | 51,8 s | 37,6 s |
| Doc Hudson | lenda, gruda nas curvas | 51,8 s | 40,7 s |
| Francesco | formula, arranca forte | 52,0 s | 39,6 s |
| Sally | certinha em tudo | 52,8 s | 40,3 s |
| Ramone | estiloso e estável | 53,2 s | 41,1 s |
| Luigi | pequeno e esperto na largada | 53,7 s | 41,8 s |
| Guido | minúsculo, mas não erra curva | 53,6 s | 43,4 s |
| Mater | guincho enferrujado | 54,6 s | 42,6 s |
| Sarge | jipe militar, firme no chão | 54,2 s | 42,2 s |
| Fillmore | sem pressa e sem sair da pista | 53,3 s | 43,0 s |
| Finn McMissile | espião, rápido em tudo | 51,9 s | 39,2 s |
| Wingo | tuner, nervoso na arrancada | 53,1 s | 40,0 s |
| Snot Rod | motorzão, difícil de segurar | 52,4 s | 38,8 s |

`carros3d.c` carrega os modelos e deixa todos iguais: mede cada um, escala para o comprimento do personagem
(`MODELOS`), centraliza, põe as rodas no chão e a frente para +z. O carro no mundo segue a pista, o
volante e o chão embaixo das rodas (`matrizDoCarro` em `render.c`). A sombra é a foto do próprio modelo visto de cima, desfocada,
e a fumaça e a poeira são nuvens soltas no mundo (`fumaca.c`). As rodas giram de verdade: `carros3d.c` acha, ao carregar, as peças redondas encostadas no chão, separa cada roda
do resto do modelo e a desenha girando em volta do próprio eixo; as da frente também viram com o volante. Na arrancada elas patinam, e o giro na tela
tem um limite, porque acima dele a roda pareceria girar ao contrário. A garagem e o menu usam uma foto de cada modelo, tirada ao carregar.

Os modelos são do Sketchfab, de DinseyPixarCarsModels e LukeMcQueen02, publicados como CC-BY-4.0 (o do Luigi veio
sem autor nem licença). Os personagens são de *Carros* (Disney/Pixar). A única mudança nos arquivos foi converter as
texturas JPEG para PNG, porque a raylib da Homebrew não lê JPEG.

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

Cada segmento guarda os seus objetos numa **lista simples** (`obstaculos.c`), e `decorarPista` escolhe o
que fica onde em Interlagos:

- o **pórtico** em cima da linha de chegada e os **colchetes do grid**, pintados no asfalto;
- **barreiras de pneus** por fora das curvas e **placas de 150, 100 e 50 m** antes delas;
- **arquibancadas e camarotes** nos lugares do mapa oficial do GP (setores A, B, M, D, H, N, R, G, S e V, Pit Stop Club e
  Orange Tree Club; tabela `ARQUIBANCADAS`). As posições foram lidas no mapa e alinhadas pelos trechos da pista;
- **árvores** sorteadas dos dois lados (sorteio com semente fixa: ficam sempre no mesmo lugar).

Num circuito sinuoso, o lugar de um objeto pode cair em cima de outro trecho da pista. Por isso `lugarLivre` confere
cada um contra a volta inteira: ele só entra se ficar além de onde o carro alcança em qualquer trecho.

As formas são modelos 3D em `recursos/modelos/objetos/` (3 árvores, pilhas de pneus, 3 placas, o pórtico, a arquibancada, o camarote e as placas dos setores, mais o refletor do estádio), feitos
no Blender e editáveis lá. `construirObjetos` (`mundo.c`) põe cada cópia na altura exata do terreno desenhado e junta
todas em poucas malhas, os lotes, para desenhar centenas de objetos de uma vez. O oval não tem objetos: o cenário dele
já está no modelo 3D.

As **zebras** só aparecem nas curvas, como na vida real: um segmento tem zebra quando vira
pelo menos `CURVATURA_MINIMA_ZEBRA` graus (`temZebra()` em `pista.c`).

## O 3D

A lógica é a mesma nas duas pistas: a lista circular, a corrida, o carro e a IA não usam a
raylib. Cada segmento sabe onde está no mundo, em metros (`xMundo`, `zMundo`, `altura`, `rumo`,
largura de cada lado e `perfil` do chão), e `pontoNoMundo()` (`pista.c`) converte qualquer posição na pista
em um ponto 3D. O desenho usa isso:

- `mundo.c` monta e desenha o cenário fixo: o modelo do estádio ou as malhas geradas a partir da lista;
- `render.c` põe a câmera atrás do carro, andando pela lista, desenha os objetos como sprites que encaram
  a câmera e coloca os carros (`carros3d.c`) no mundo.

**A física ainda é a antiga:** o carro não tem direção própria, só a posição lateral na pista.

## As pistas

Escolhidas numa tela depois de CORRER (`IdPista` em `pista.h`).

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

### Interlagos

As malhas são geradas da lista ao abrir o jogo (`construirInterlagos` em `mundo.c`): asfalto,
zebras e um terreno que acompanha a altura da pista, com a cidade e os morros em volta.

Interlagos tem 2587 segmentos (cerca de 4.309 m), corrida no sentido anti-horário.
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
