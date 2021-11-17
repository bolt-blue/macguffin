CFLAGS ?= -Wall -g -O0

all: macguffin

macguffin: linux_macguffin.c
	$(CC) $(CFLAGS) $^ -o $@
