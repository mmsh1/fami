#include <stdio.h>
#include <stdlib.h>

#include "cpu.h"
#include "mem.h"



int
main(int argc, char **argv)
{
	mos6502 cpu;
	uint8_t mem[RAM_SIZE];

	if (argc != 2) {
		/* TODO print usage */
		fprintf(stderr, "ERROR: WRONG ARGUMENTS!\n");
		exit(EXIT_FAILURE);
	}

	cpu_reset(&cpu);
	mem_reset(mem);

	return 0;
}
