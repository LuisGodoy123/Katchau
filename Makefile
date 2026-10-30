# KATCHAU — Makefile
#   make          compila o jogo (gera ./katchau)
#   make run      compila e roda o jogo
#   make testes   compila e roda os testes de terminal (sem raylib)
#   make memoria  roda os testes no valgrind (Linux/WSL)
#   make zip      gera katchau.zip para a entrega (sem executável)
#   make clean    apaga os arquivos gerados

NOME   = katchau
CC     = gcc
CFLAGS = -std=c99 -Wall -Wextra -g -Isrc

# Módulos de lógica: não usam a raylib (regra 2 do roadmap)
LOGICA = src/pista.c src/obstaculos.c src/carro.c src/ia.c \
         src/ordenacao.c src/corrida.c

# Módulos que desenham na tela
DESENHO = src/render.c src/hud.c src/menu.c src/visual.c

FONTES = src/main.c $(LOGICA) $(DESENHO)
OBJETOS = $(FONTES:src/%.c=build/%.o)

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
                  -framework CoreVideo
    else
        # Linux (raylib instalada em /usr/local)
        LIBS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
    endif
endif

# ---- Alvos ----
all: $(EXE)

$(EXE): $(OBJETOS)
	$(CC) $(OBJETOS) -o $@ $(LIBS)

# -MMD -MP: o gcc anota de quais .h cada .o depende (arquivos build/*.d).
# Assim, mudar só um .h (ex.: a struct Carro) recompila quem o inclui.
build/%.o: src/%.c | build
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

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

memoria: build/teste_pista
	valgrind --leak-check=full --error-exitcode=1 ./build/teste_pista

zip: clean
	zip -r $(NOME).zip Makefile README.md src testes recursos

clean:
	rm -rf build $(NOME) $(NOME).exe $(NOME).zip

.PHONY: all run testes memoria zip clean
