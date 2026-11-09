# Aquafresh PS2
#   make            -> aquafresh.elf (precisa do PS2SDK + gsKit; veja README)
#   make packed     -> aquafresh-packed.elf (comprimido com ps2-packer)
#   make iso        -> aquafresh.iso (para PCSX2)
#   make test       -> testes das regras no PC
#   make shots      -> capturas de tela geradas no PC (pasta screenshots/)

EE_BIN = aquafresh.elf
EE_OBJS = src/main_ps2.o src/ui.o src/draw.o src/game.o src/font.o
EE_INCS = -I$(GSKIT)/include -Isrc
EE_LDFLAGS = -L$(GSKIT)/lib
EE_LIBS = -lgskit -ldmakit -lpad -lm
EE_CFLAGS = -std=gnu99

HOST_CC ?= gcc
HOST_SRCS = src/ui.c src/draw.c src/game.c src/font.c

.PHONY: all clean packed iso test shots docker font

all: $(EE_BIN)

packed: aquafresh-packed.elf

aquafresh-packed.elf: $(EE_BIN)
	ps2-packer $< $@

iso: aquafresh.iso

aquafresh.iso: aquafresh-packed.elf iso/SYSTEM.CNF
	rm -rf build/iso && mkdir -p build/iso
	cp iso/SYSTEM.CNF build/iso/SYSTEM.CNF
	cp aquafresh-packed.elf build/iso/AQUA_000.01
	xorriso -as mkisofs -quiet -V AQUAFRESH -iso-level 1 -o $@ build/iso

# Regenera a fonte a partir da arte ASCII (src/font.c ja vem pronto).
font:
	python3 tools/gen_font.py src/font.c

test:
	mkdir -p build
	$(HOST_CC) -O2 -Wall -Wextra -o build/test_game tests/test_game.c src/game.c
	./build/test_game

shots:
	mkdir -p build screenshots
	$(HOST_CC) -O2 -Wall -Wextra -Isrc -o build/host_shots host/host_main.c $(HOST_SRCS) -lm
	./build/host_shots screenshots
	for f in screenshots/*.ppm; do python3 -c "from PIL import Image; import sys; Image.open(sys.argv[1]).save(sys.argv[1][:-4] + '.png')" $$f && rm $$f; done

# Compila dentro do container oficial do ps2dev (so precisa de Docker).
docker:
	docker run --rm -v "$(CURDIR)":/src -w /src ps2dev/ps2dev:latest \
		sh -c "apk add --no-cache make xorriso >/dev/null 2>&1; make all packed iso"

clean:
	rm -rf $(EE_BIN) aquafresh-packed.elf aquafresh.iso $(EE_OBJS) build

PS2SDK_MK ?= $(PS2SDK)
ifdef PS2SDK
include $(PS2SDK_MK)/samples/Makefile.pref
include $(PS2SDK_MK)/samples/Makefile.eeglobal
endif
