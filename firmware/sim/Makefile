# Host simulator of a Keybit network and the Keyword's side bus, built from
# the real firmware sources. Apple arm64 or Linux x86_64. `make test` runs
# the checks.
CC ?= cc
CFLAGS ?= -O2 -g -Wall -Wno-unused-function
KEYBIT := ../keybit
KEYWORD := ../keyword/Core

ifeq ($(shell uname -s),Darwin)
CTX := ctx_arm64.S
PIC :=
BUNDLE := -bundle -undefined dynamic_lookup
SIM_LD :=
SED_I := sed -i ''
else
CTX := ctx_x86_64.S
PIC := -fPIC
# Each Keybit copy keeps its own globals: bind its symbols to itself.
BUNDLE := -shared -fPIC -Wl,-Bsymbolic
SIM_LD := -rdynamic -ldl -lm
SED_I := sed -i
endif

all : sim node.bundle

# Keyword timing experiments: make sim KW_DEFS="-DKEYS_MS=10"
.PHONY : sim

# The Keybit firmware some Keybits still run, rebuilt from git, to check a
# Keyword change against them: make node_v10.bundle
V10_REV := 6039864
node_v10.bundle : node.c ch32fun.h sim.h
	rm -rf build_v10 && mkdir -p build_v10
	for f in main.c link.c link.h soft_i2c.c soft_i2c.h; do git show $(V10_REV):firmware/keybit/$$f > build_v10/$$f; done
	$(SED_I) 's/(\*f->in \& f->ck)/(READ_PORT(f->in) \& f->ck)/; s/(\*d->in \& d->dt)/(READ_PORT(d->in) \& d->dt)/' build_v10/link.c
	$(SED_I) 's/^#define CONFIG_PAGE .*/#ifndef CONFIG_PAGE\n#define CONFIG_PAGE 0x08003FC0u\n#endif/' build_v10/main.c
	$(CC) $(CFLAGS) -I. -Ibuild_v10 $(PIC) -Dlink_crc8=link_crc8_real -o build_v10/link.o -c build_v10/link.c
	$(CC) $(CFLAGS) -I. -Ibuild_v10 $(BUNDLE) -o $@ node.c build_v10/link.o build_v10/soft_i2c.c

# The Keybit firmware on the Keybits re-docked on 2026-10-29 (v12), whose
# status stream sends its CRC a bit late: make node_v12.bundle
V12_REV := 7737b2e
node_v12.bundle : node.c ch32fun.h sim.h
	rm -rf build_v12 && mkdir -p build_v12
	for f in main.c link.c link.h soft_i2c.c soft_i2c.h; do git show $(V12_REV):firmware/keybit/$$f > build_v12/$$f; done
	$(CC) $(CFLAGS) -I. -Ibuild_v12 $(PIC) -Dlink_crc8=link_crc8_real -o build_v12/link.o -c build_v12/link.c
	$(CC) $(CFLAGS) -I. -Ibuild_v12 $(BUNDLE) -o $@ node.c build_v12/link.o build_v12/soft_i2c.c

node.bundle : node.c $(KEYBIT)/main.c $(KEYBIT)/link.c $(KEYBIT)/soft_i2c.c $(KEYBIT)/link.h $(KEYBIT)/soft_i2c.h ch32fun.h sim.h
	$(CC) $(CFLAGS) -I. -I$(KEYBIT) $(PIC) -Dlink_crc8=link_crc8_real -o link.o -c $(KEYBIT)/link.c
	$(CC) $(CFLAGS) -I. -I$(KEYBIT) $(BUNDLE) -o $@ node.c link.o $(KEYBIT)/soft_i2c.c
	rm -f link.o

sim : sim.c kw.c $(CTX) sim.h ch32fun.h stm32wbxx_hal.h $(KEYWORD)/Src/keybit_bus.c $(KEYWORD)/Inc/keybit_bus.h
	$(CC) $(CFLAGS) $(KW_DEFS) -I. -I$(KEYWORD)/Inc -o $@ sim.c kw.c $(CTX) $(SIM_LD)

test : all
	./sim node.bundle all

clean :
	rm -rf sim *.bundle *.dSYM build_v10 build_v12

.PHONY : all test clean
