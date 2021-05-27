#include <stdio.h>
#include <stdlib.h>

#include "bus.h"
#include "cpu.h"
#include "mem.h"

int
main(int argc, char **argv)
{
	mos6502 cpu;
	uint8_t ram[RAM_SIZE];
	char *filename = NULL;

	if (argc != 2) {
		/* TODO print usage */
		fprintf(stderr, "ERROR: WRONG ARGUMENTS!\n");
		exit(EXIT_FAILURE);
	}

	filename = argv[1];

	cpu_reset(&cpu);
	mem_reset(ram);
	bus_reset(ram);

	return 0;
}
