CFLAGS = -Wall -Werror -Wextra -std=c99 -pedantic -g3

all: options cnes

options:
	@echo cnes build options:
	@echo "CFLAGS	= $(CFLAGS)"
	@echo "CC	= $(CC)"

%.o: %.c
	$(CC) -c $(CFLAGS) $<

cnes: bus.o cpu.o mem.o nes.o
	$(CC) -o $@ $^

clean:
	rm -f cnes
	rm -f *.o

.PHONY: all options clean
