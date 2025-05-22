CFLAGS = -Wall -Wextra -std=c99 -pedantic -g3 -Wconversion
LIBS = lib/libraylib.a -lm
#-Werror

all: options fami

options:
	@echo fami build options:
	@echo "CFLAGS	= $(CFLAGS)"
	@echo "CC	= $(CC)"

%.o: %.c
	$(CC) -c $(CFLAGS) $<

fami: bus.o cartrige.o cpu.o gfx.o ines.o mem.o nes.o ppu.o
	$(CC) -o $@ $^ $(LIBS) -fsanitize=address -fsanitize=undefined

test: cpu_test.o
	$(CC) -o $@ $^ -lcriterion -Wl,-rpath, /usr/lib/libgit2.so

clean:
	rm -f fami
	rm -f test
	rm -f *.o

.PHONY: all options clean
