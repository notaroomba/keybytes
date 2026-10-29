# Host simulator of a Keybit network and the Keyword's side bus, built from
# the real firmware sources. Apple arm64 only. `make test` runs the checks.
CC ?= cc
CFLAGS ?= -O2 -g -Wall -Wno-unused-function
KEYBIT := ../keybit
KEYWORD := ../keyword/Core

all : sim node.bundle

# Keyword timing experiments: make sim KW_DEFS="-DKEYS_MS=10"
.PHONY : sim

# The Keybit firmware some Keybits still run, rebuilt from git, to check a
# Keyword change against them: make node_v10.bundle
V10_REV := 6039864
node_v10.bundle : node.c ch32fun.h sim.h
	rm -rf build_v10 && mkdir -p build_v10
	for f in main.c link.c link.h soft_i2c.c soft_i2c.h; do git show $(V10_REV):firmware/keybit/$$f > build_v10/$$f; done
	sed -i '' 's/(\*f->in \& f->ck)/(READ_PORT(f->in) \& f->ck)/; s/(\*d->in \& d->dt)/(READ_PORT(d->in) \& d->dt)/' build_v10/link.c
	sed -i '' 's/^#define CONFIG_PAGE .*/#ifndef CONFIG_PAGE\n#define CONFIG_PAGE 0x08003FC0u\n#endif/' build_v10/main.c
	$(CC) $(CFLAGS) -I. -Ibuild_v10 -bundle -undefined dynamic_lookup -o $@ node.c build_v10/link.c build_v10/soft_i2c.c

node.bundle : node.c $(KEYBIT)/main.c $(KEYBIT)/link.c $(KEYBIT)/soft_i2c.c $(KEYBIT)/link.h $(KEYBIT)/soft_i2c.h ch32fun.h sim.h
	$(CC) $(CFLAGS) -I. -I$(KEYBIT) -bundle -undefined dynamic_lookup -o $@ node.c $(KEYBIT)/link.c $(KEYBIT)/soft_i2c.c

sim : sim.c kw.c ctx_arm64.S sim.h ch32fun.h stm32wbxx_hal.h $(KEYWORD)/Src/keybit_bus.c $(KEYWORD)/Inc/keybit_bus.h
	$(CC) $(CFLAGS) $(KW_DEFS) -I. -I$(KEYWORD)/Inc -o $@ sim.c kw.c ctx_arm64.S

test : all
	./sim node.bundle all

clean :
	rm -rf sim *.bundle *.dSYM build_v10

.PHONY : all test clean
