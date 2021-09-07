CFLAGS = -Wall -std=c99 -pedantic -g3
#-Wextra
#-Werror

all: options cnes

options:
	@echo cnes build options:
	@echo "CFLAGS	= $(CFLAGS)"
	@echo "CC	= $(CC)"

%.o: %.c
	$(CC) -c $(CFLAGS) $<

cnes: bus.o cpu.o mem.o nes.o ppu.o
	$(CC) -o $@ $^

clean:
	rm -f cnes
	rm -f *.o

.PHONY: all options clean
