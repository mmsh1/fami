#include <stdio.h>
#include <stdlib.h>

#include "bus.h"

typedef struct {
	/* r2A03 apu */
	bus bus;
	r2A03 cpu;
	r2C02 ppu;
	uint8_t ram[RAM_SIZE];
} nes;

static double fps_ntsc = 60.0988;

static void
nes_loadrom(const char *path)
{
	/* fread */
}

static void
nes_tick(nes *n)
{
	/* bus_apu_tick(&n->bus); */
	bus_cpu_tick(&n->bus);
	bus_ppu_tick(&n->bus);
	bus_ppu_tick(&n->bus);
	bus_ppu_tick(&n->bus);
}

static void
nes_run(nes *n)
{
	double frame_length = 1000 / fps_ntsc;


	cpu_exec(&n->cpu, 15);
}

static void
nes_init(nes *n)
{
	bus_cpu_reset(&n->bus, &n->cpu);
	bus_ram_reset(&n->bus, n->ram);
	bus_ppu_reset(&n->bus, &n->ppu);
}

int
main(int argc, char **argv)
{
	nes n;
	char *filename = NULL;
	/* FILE *rom = NULL; */

	if (argc != 2) {
		fprintf(stderr, "ERROR: WRONG ARGUMENTS!\n"); /* TODO print usage */
		exit(EXIT_FAILURE);
	}

	filename = argv[1];

	nes_init(&n);
	/*nes_loadrom(filename);*/

	/*
	rom = fopen(filename, "r");
	if (rom == NULL) {
		fprintf(stderr, "ERROR: ROM %s NOT OPENED!\n", filename);
		exit(EXIT_FAILURE);
	}
	*/
	/* for (int i = 0; i < 0xFFFF; i++) ram[i] = 0xA9; */

	/*
	ram[0xFFFC] = 0xA9;
	fprintf(stderr, "PLACED IN %X\n", 0xFFFC);
	ram[0xFFFD] = 0x88;
	fprintf(stderr, "PLACED IN %X\n", 0xFFFD);
	ram[0xFFFE] = 0xE0;
	fprintf(stderr, "PLACED IN %X\n", 0xFFFE);
	ram[0xFFFF] = 0xF8;
	fprintf(stderr, "PLACED IN %X\n", 0xFFFF);
	*/

	/* cpu_exec(&cpu, 15); */

	return 0;
}
