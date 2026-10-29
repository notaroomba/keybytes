# Host simulator of a Keybit network and the Keyword's side bus, built from
# the real firmware sources. Apple arm64 only. `make test` runs the checks.
CC ?= cc
CFLAGS ?= -O2 -g -Wall -Wno-unused-function
KEYBIT := ../keybit
KEYWORD := ../keyword/Core

all : sim node.bundle

node.bundle : node.c $(KEYBIT)/main.c $(KEYBIT)/link.c $(KEYBIT)/soft_i2c.c $(KEYBIT)/link.h $(KEYBIT)/soft_i2c.h ch32fun.h sim.h
	$(CC) $(CFLAGS) -I. -I$(KEYBIT) -bundle -undefined dynamic_lookup -o $@ node.c $(KEYBIT)/link.c $(KEYBIT)/soft_i2c.c

sim : sim.c kw.c ctx_arm64.S sim.h ch32fun.h stm32wbxx_hal.h $(KEYWORD)/Src/keybit_bus.c $(KEYWORD)/Inc/keybit_bus.h
	$(CC) $(CFLAGS) -I. -I$(KEYWORD)/Inc -o $@ sim.c kw.c ctx_arm64.S

test : all
	./sim node.bundle all

clean :
	rm -rf sim node.bundle *.dSYM

.PHONY : all test clean
