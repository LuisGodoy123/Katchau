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
| ↑ ou W | R2 | Acelerar |
| ↓ ou S | L2 | Frear |
| SHIFT | Cruz | Freio de mão (com o volante virado, drift) |
| ESPAÇO | Bola | Nitro (segure): recarrega aos poucos, e bem mais rápido freando |
| ← → ou A D | analógico esquerdo ou direcional | Virar (o volante gira aos poucos e volta sozinho ao centro) |
| | analógico direito | Câmera livre: gira em volta do carro; todo para cima olha para o alto e todo para baixo olha para trás num giro rápido (soltando, volta) |
| ENTER | Cruz | Ver o resultado (depois da chegada) |
| ESC   | Options | Pausar: a corrida e os sons congelam; escolha CONTINUAR ou MENU PRINCIPAL (ESC de novo continua) |

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
  oval.h, splat.h  traçado e lugares da torcida dos outros dois modelos 3D
  tracado.h        o formato dessas tabelas
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
  audio.c/.h       música, efeitos dos menus, largada, falas do McQueen
  efeitos.c/.h     sons do carro e da corrida: motores NASCAR (jogador e rivais), vento, pneu,
                   grama, muro, nitro, câmbio, voltas e pit stop
  controle.c/.h    controle (gamepad): comandos do carro e botões dos menus
testes/
  teste_pista.c    testa pista, carro, corrida, IA e ordenação no terminal
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

Regra: só `render.c`, `mundo.c`, `carros3d.c`, `fumaca.c`, `noite.c`, `hud.c`, `menu.c`, `visual.c`, `audio.c`, `efeitos.c` e `controle.c` incluem `raylib.h` (além do `main.c`, que abre
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
volante e o chão embaixo das rodas (`matrizDoCarro` em `render.c`). A sombra é a foto do próprio modelo visto de cima, desfocada, numa malha
que acompanha o chão ponto a ponto (`desenharSombraDoCarro` em `render.c`), sem inclinar com o carro: uma sombra plana afundava na pista inclinada da Copa Pistão;
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

## Pitstop

Em Interlagos, nos níveis médio e difícil, a parada nos boxes é obrigatória: quem cruza a última bandeirada sem parar é desclassificado.

- **Entrar:** na Subida dos Boxes, a faixa de desaceleração abre colada na pista: é só ir para a esquerda, em cima dela (`querEntrarNoPit`). Ali o piloto automático assume, no limite de 100 km/h perto das garagens.
- **A troca:** cada carro tem uma vaga. Na do jogador, o Guido passa por todas as rodas em 3,2 s (`faseDaTroca` e `lugarDoGuido` em `pitstop.c`, desenho em `render.c`), e a imagem corta para uma câmera ao lado do carro.
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

Na tela CORRIDA se escolhe a pista e o nível (`NIVEIS` em `corrida.c`). O vermelho marca o que está escolhido, e o contorno amarelo, onde o cursor está; confirmar passa para a linha seguinte, e só o CORRER larga:

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

Cada segmento guarda os seus objetos numa **lista simples** (`obstaculos.c`), e `decorarPista` escolhe o
que fica onde em Interlagos:

- o **pórtico** em cima da linha de chegada e os **colchetes do grid**, pintados no asfalto;
- **barreiras de pneus** por fora das curvas e **placas de 150, 100 e 50 m** antes delas;
- **arquibancadas contínuas** nos lugares do mapa oficial do GP (setores A, B, M, D, H, N, R, G, S e V; tabela `ARQUIBANCADAS`).
  Elas acompanham a curva da pista, com muro da frente, fileiras com a torcida, corredores e, em algumas, cobertura sobre colunas;
- a **reta dos boxes**: muro e alambrado dos dois lados, com um funil no fim da Subida dos Boxes, para quem vem pela grama, e o camarote
  Pit Stop Club depois da linha;
- os **boxes**: o prédio é feito de módulos de 6 garagens (`boxes.glb`), cada um na altura da reta, com pontas envidraçadas e a torre de controle,
  com um pátio de asfalto e ruas de serviço. Onde o terreno desce, tudo fica sobre uma laje com pilares e vigas (`construirEstruturaDosBoxes`). O **pit lane** abre colado na pista na Subida dos Boxes, como uma faixa de desaceleração, passa por trás do funil, corre atrás do muro e, por dentro do S do Senna e da Curva do Sol, volta a colar na pista na Reta Oposta, como uma faixa de aceleração. Nas duas ligações há uma ilha de asfalto pintado. É por ele que passa o pitstop (`pitstop.c`);
- **árvores** só nos bosques da tabela `BOSQUES` (sorteio com semente fixa: ficam sempre no mesmo lugar). Longe deles, o campo é aberto.
- em volta da pista (`infraestrutura.c`): linhas brancas nas bordas, a marca de borracha no traçado, brita por fora das curvas, guard-rail com alambrado e placas de patrocínio, postos de fiscal, torres de TV, duas passarelas, o telão, bandeiras no alto das arquibancadas, a rua de serviço, tendas, estacionamento, caminhões das equipes no pátio e bancadas na mureta dos boxes. Fica tudo além de onde o carro chega: a física da pista não muda;
- os prédios, casas, árvores, caminhões, tendas, torres e o telão são modelos `.glb` em `recursos/modelos/objetos/`; só o que acompanha a forma da pista e do terreno é montado em código (linhas, brita, guard-rail, placas, morros);
- além do muro do autódromo (`paisagem.c`), tudo em 3D de verdade: a avenida com o trânsito, os bairros, os prédios, as matas, o centro da cidade e os morros até o horizonte, com o ar clareando o que está longe. Só o céu é uma imagem. Dentro do circuito fica o lago da Descida do Lago.

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

### Marcas de pneu e sombras

Quando o pneu escorrega (drift, derrapagem, arrancada patinando, rodada e freada forte em alta), cada roda deixa um
rastro escuro no asfalto, que some aos poucos (`marcas.c`). Na grama não marca.

De dia, o cenário faz sombra do sol: a cada quadro, o que fica num quadrado de 280 m em volta da câmera é desenhado
visto do sol num mapa de profundidade, e o shader do cenário escurece o que estiver atrás de alguma coisa. À noite,
na Copa Pistão, não há sombra do sol. Dá para desligar em Configurações, se o computador ficar lento.

### A onda da torcida

Na Copa Pistão, quando o líder chega à metade da corrida, a torcida de carrinhos faz a onda: começando um pouco
à frente do jogador, os carros de cada setor acendem os faróis e disparam as câmeras, um setor depois do outro,
até dar a volta inteira no estádio (7 s). Tudo no shader da torcida (`noite.c`), pelo ângulo em volta do centro.

### Oval e Splat

O Splat tem muito mais ruas do que a corrida usa. A rota escolhida (2,1 km) sai da reta dos boxes, passa por cima
da ponte, vira no cruzamento para o laço de cima, volta por baixo da ponte, desce pelos esses e fecha pela reta;
ficaram de fora o grampo do lado oeste e as esquinas mais fechadas. A curva mais fechada tem 30 m de raio.

Duas pistas de dia que vêm prontas em um modelo 3D (`recursos/modelos/oval.glb` e `splat.glb`). O traçado e
os lugares da torcida (`oval.h`, `splat.h`) foram medidos nos modelos, e a arquibancada recebe uma figura em
cada lugar, com o mesmo shader de torcida da Copa Pistão. O pit lane de cada uma é o que o modelo mostra: no
oval, entre o muro dos boxes e as garagens; no Splat, a estrada paralela à reta, na frente do prédio das
garagens. Na Copa Pistão não há garagens: a equipe arma as tendas no pátio por dentro da Reta Oposta.

As árvores e estruturas dos dois mapas (`cenario_mapas.h`) usam os mesmos modelos de Interlagos, mais pinheiros
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
