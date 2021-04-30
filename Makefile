CFLAGS = -Wall -Wextra -std=c89 -pedantic
#-Werror

#test
all: options cnes

options:
	@echo cnes build options:
	@echo "CFLAGS	= $(CFLAGS)"
	@echo "CC	= $(CC)"

%.o: %.c
	$(CC) -c $(CFLAGS) $<

cnes: cpu.o mem.o
	$(CC) -o $@ $^

clean:
	rm -f cnes
	rm -f *.o

.PHONY: all clean
