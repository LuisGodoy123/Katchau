# KATCHAU — Makefile
#   make          compila o jogo (gera ./katchau)
#   make run      compila e roda o jogo
#   make testes   compila e roda os testes de terminal (sem raylib)
#   make simulacao  corre muitas provas sem gráficos e mede a competitividade de cada nível
#   make testar-vibracao  toca cada vibração no controle, para sentir (macOS)
#   make memoria  roda os testes no valgrind (Linux/WSL)
#   make zip      gera katchau.zip para a entrega (sem executável)
#   make clean    apaga os arquivos gerados

NOME   = katchau
CC     = gcc
CFLAGS = -std=c99 -Wall -Wextra -g -Isrc

# Módulos de lógica: não usam a raylib (regra 2 do roadmap)
LOGICA = src/pista.c src/obstaculos.c src/carro.c src/ia.c \
         src/ordenacao.c src/corrida.c src/motor.c src/recordes.c src/olhar.c src/pitstop.c src/vibracao.c src/configuracoes.c

# Módulos que desenham na tela
DESENHO = src/render.c src/mundo.c src/construcao.c src/paisagem.c src/infraestrutura.c src/carros3d.c src/fumaca.c src/faiscas.c src/fogos.c src/marcas.c src/torcida3d.c src/telao.c src/posprocesso.c src/noite.c src/hud.c src/menu.c src/visual.c src/audio.c src/efeitos.c src/exibicao.c src/apresentacao.c src/intro.c src/videofundo.c src/controle.c

FONTES = src/main.c $(LOGICA) $(DESENHO)
OBJETOS = $(FONTES:src/%.c=build/%.o)
FRAMEWORKS_DA_VIBRACAO = -framework Foundation -framework GameController -framework CoreHaptics

# ---- Bibliotecas por sistema operacional ----
ifeq ($(OS),Windows_NT)
    # Windows com MinGW (raylib em C:/raylib/raylib/src, padrão do instalador)
    RAYLIB_DIR ?= C:/raylib/raylib/src
    CFLAGS  += -I$(RAYLIB_DIR)
    LIBS     = -L$(RAYLIB_DIR) -lraylib -lopengl32 -lgdi32 -lwinmm
    EXE      = $(NOME).exe
else
    SISTEMA := $(shell uname -s)
    EXE      = $(NOME)
    ifeq ($(SISTEMA),Darwin)
        # macOS com Homebrew (brew install raylib)
        BREW_PREFIX := $(shell brew --prefix 2>/dev/null)
        CFLAGS += -I$(BREW_PREFIX)/include
        LIBS    = -L$(BREW_PREFIX)/lib -lraylib \
                  -framework OpenGL -framework Cocoa -framework IOKit \
                  -framework CoreVideo $(FRAMEWORKS_DA_VIBRACAO)
        # A vibração do DualSense fala com o sistema de controles da Apple, em Objective-C
        OBJETOS += build/vibrar_mac.o
    else
        # Linux (raylib instalada em /usr/local)
        LIBS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
    endif
endif

# Fora do macOS o jogo não vibra (src/vibrar_outros.c não faz nada)
ifneq ($(SISTEMA),Darwin)
    OBJETOS += build/vibrar_outros.o
endif

# ---- Alvos ----
all: $(EXE)

$(EXE): $(OBJETOS)
	$(CC) $(OBJETOS) -o $@ $(LIBS)

# -MMD -MP: o gcc anota de quais .h cada .o depende (arquivos build/*.d).
# Assim, mudar só um .h (ex.: a struct Carro) recompila quem o inclui.
build/%.o: src/%.c | build
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

# O arquivo em Objective-C: sem -std=c99, e com a memória automática (ARC)
build/vibrar_mac.o: src/vibrar_mac.m src/vibrar.h src/vibracao.h | build
	$(CC) -Wall -Wextra -g -Isrc -fobjc-arc -c $< -o $@

-include $(OBJETOS:.o=.d)

build:
	mkdir -p build

run: $(EXE)
	./$(EXE)

# Os testes usam só a lógica, então compilam sem a raylib
testes: build/teste_pista
	./build/teste_pista

build/teste_pista: testes/teste_pista.c $(LOGICA) $(wildcard src/*.h) | build
	$(CC) $(CFLAGS) testes/teste_pista.c $(LOGICA) -o $@ -lm

# Corre muitas provas sem gráficos e mede o quanto cada dificuldade é competitiva
simulacao: build/simular_dificuldade
	./build/simular_dificuldade

build/simular_dificuldade: testes/simular_dificuldade.c $(LOGICA) $(wildcard src/*.h) | build
	$(CC) $(CFLAGS) -O2 testes/simular_dificuldade.c $(LOGICA) -o $@ -lm

# Para sentir cada vibração no controle (macOS), sem abrir o jogo
testar-vibracao: build/teste_vibracao
	./build/teste_vibracao

build/teste_vibracao: testes/teste_vibracao.m src/vibrar_mac.m $(LOGICA) $(wildcard src/*.h) | build
	$(CC) -Wall -Wextra -g -Isrc -fobjc-arc testes/teste_vibracao.m src/vibrar_mac.m $(LOGICA) -o $@ $(FRAMEWORKS_DA_VIBRACAO) -lm

memoria: build/teste_pista
	valgrind --leak-check=full --error-exitcode=1 ./build/teste_pista

zip: clean
	zip -r $(NOME).zip Makefile README.md src testes recursos

clean:
	rm -rf build $(NOME) $(NOME).exe $(NOME).zip

.PHONY: all run testes testar-vibracao memoria zip clean
