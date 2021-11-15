CFLAGS = -Wall -Wextra -std=c99 -pedantic -g3
#-Werror

all: options fami

options:
	@echo fami build options:
	@echo "CFLAGS	= $(CFLAGS)"
	@echo "CC	= $(CC)"

%.o: %.c
	$(CC) -c $(CFLAGS) $<

fami: bus.o cpu.o mem.o nes.o ppu.o
	$(CC) -o $@ $^

clean:
	rm -f fami
	rm -f *.o

.PHONY: all options clean
