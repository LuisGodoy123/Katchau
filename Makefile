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
CFLAGS = -std=c99 -O2 -Wall -Wextra -g -Isrc

# Módulos de lógica: não usam a raylib (regra 2 do roadmap)
LOGICA = src/pista.c src/obstaculos.c src/carro.c src/ia.c \
         src/ordenacao.c src/corrida.c src/motor.c src/motorgranular.c src/recordes.c src/olhar.c src/pitstop.c src/vibracao.c src/configuracoes.c src/mjpeg.c src/colisao.c src/livre.c src/moradores.c

# Módulos que desenham na tela
DESENHO = src/render.c src/corte.c src/mundo.c src/construcao.c src/carros3d.c src/fumaca.c src/faiscas.c src/fogos.c src/marcas.c src/torcida3d.c src/telao.c src/posprocesso.c src/noite.c src/hud.c src/menu.c src/visual.c src/audio.c src/efeitos.c src/exibicao.c src/apresentacao.c src/intro.c src/videofundo.c src/controle.c src/passeio.c src/carreta.c

FONTES = src/main.c $(LOGICA) $(DESENHO)
OBJETOS = $(FONTES:src/%.c=build/%.o)
FRAMEWORKS_DA_VIBRACAO = -framework Foundation -framework GameController -framework CoreHaptics

# ---- Bibliotecas por sistema operacional ----
ifeq ($(OS),Windows_NT)
    # O gcc que vem primeiro no PATH costuma ser o MinGW 32-bit de C:/MinGW/bin, que não tem
    # pthread.h e não linka com a raylib de 64 bits. Como o MinGW não traz make.exe (só
    # mingw32-make.exe), o make sai do w64devkit e o gcc sai do MinGW: o build começa e morre
    # no meio, em src/mjpeg.c. Fixar o compilador aqui faz o make funcionar de qualquer
    # terminal, sem mexer no PATH.
    #
    # O -B é parte da mesma história: sem ele o gcc acha pelo PATH o assembler e o linker do
    # MinGW (binutils 2.28, que não conhece o --gdwarf-5 que este gcc manda) em vez dos dele
    # (binutils 2.46). O -B diz ao gcc onde estão as ferramentas da própria instalação.
    #
    # Quem não tem o w64devkit nesse caminho segue com o gcc do PATH; para forçar o do PATH,
    # use "make W64DEVKIT=".
    W64DEVKIT ?= C:/raylib/w64devkit/bin
    ifneq ($(wildcard $(W64DEVKIT)/gcc.exe),)
        CC := $(W64DEVKIT)/gcc.exe -B$(W64DEVKIT)
    endif
    # Windows com MinGW (raylib em C:/raylib/raylib/src, padrão do instalador)
    RAYLIB_DIR ?= C:/raylib/raylib/src
    CFLAGS  += -I$(RAYLIB_DIR)
    # A libraylib.a do w64devkit já traz a GLFW dentro (o membro rglfw.o); outras instalações a
    # têm separada, em libglfw3.a. O -print-file-name devolve o caminho quando acha a biblioteca
    # e só o nome quando não acha, então o "/" diz se ela existe nesta máquina.
    GLFW_SEPARADA := $(findstring /,$(shell $(CC) -print-file-name=libglfw3.a))
    LIBS     = -L$(RAYLIB_DIR) -lraylib $(if $(GLFW_SEPARADA),-lglfw3) -lopengl32 -lgdi32 -lwinmm -lpthread
    EXE      = $(NOME).exe
    # A libraylib.a TEM de vir da mesma árvore que os headers de RAYLIB_DIR. Se ela não estiver
    # lá, o -lraylib cai numa cópia qualquer que o toolchain tenha (o w64devkit traz uma), e o
    # jogo passa a compilar contra um raylib.h e a rodar contra outro. O erro que isso dá não
    # parece nada com a causa: structs com campos em posições diferentes, ponteiros lidos
    # errados, e um SIGSEGV dentro do driver de vídeo ao subir uma malha montada pelo jogo
    # (UploadMesh em separarRodas). Conferido no link, para falhar com uma mensagem útil.
    CONFERIR_RAYLIB = test -f "$(RAYLIB_DIR)/libraylib.a" || {         echo "";         echo "ERRO: nao achei $(RAYLIB_DIR)/libraylib.a";         echo "      Os headers vem de $(RAYLIB_DIR), e a biblioteca precisa vir dali tambem.";         echo "      Compile a raylib uma vez:";         echo "";         echo "          cd $(RAYLIB_DIR) && make PLATFORM=PLATFORM_DESKTOP";         echo "";         echo "      (ou aponte para outra arvore com make RAYLIB_DIR=caminho/para/raylib/src)";         echo "";         exit 1; }
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
    CONFERIR_RAYLIB = true   # fora do Windows a raylib vem instalada no sistema
endif

# Fora do macOS o jogo não vibra (src/vibrar_outros.c não faz nada)
ifneq ($(SISTEMA),Darwin)
    OBJETOS += build/vibrar_outros.o
endif

# ---- Alvos ----
all: $(EXE)

$(EXE): $(OBJETOS)
	@$(CONFERIR_RAYLIB)
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
testes: build/teste_pista build/teste_livre
	./build/teste_pista
	./build/teste_livre

build/teste_livre: testes/teste_livre.c $(LOGICA) $(wildcard src/*.h) | build
	$(CC) $(CFLAGS) testes/teste_livre.c $(LOGICA) -o $@ -lm -lpthread

build/teste_pista: testes/teste_pista.c $(LOGICA) $(wildcard src/*.h) | build
	$(CC) $(CFLAGS) testes/teste_pista.c $(LOGICA) -o $@ -lm -lpthread

# Procura buracos, degraus e lugares onde o carro do passeio livre enrosca
sondar-colisao: build/sondar_colisao

build/sondar_colisao: ferramentas/sondar_colisao.c $(LOGICA) $(wildcard src/*.h) | build
	$(CC) $(CFLAGS) ferramentas/sondar_colisao.c $(LOGICA) -o $@ -lm -lpthread

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

# Mede Interlagos no modelo 3D e reescreve src/interlagos.h (só quando o modelo muda)
medir-interlagos: build/medir_interlagos
	./build/medir_interlagos recursos/modelos/interlagos.glb src/interlagos.h

build/medir_interlagos: ferramentas/medir_interlagos.c src/interlagos_materiais.h | build
	$(CC) $(CFLAGS) -O2 ferramentas/medir_interlagos.c -o $@ $(LIBS)

memoria: build/teste_pista
	valgrind --leak-check=full --error-exitcode=1 ./build/teste_pista

zip: clean
	zip -r $(NOME).zip Makefile README.md src testes recursos ferramentas

clean:
	rm -rf build $(NOME) $(NOME).exe $(NOME).zip

.PHONY: all run testes testar-vibracao medir-interlagos memoria zip clean
