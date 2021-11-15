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

static int
nes_loadrom(nes *n, const char *path)
{
	FILE *rom = NULL;
	uint64_t readed;
	int64_t size;

	rom = fopen(path, "r");
	if (rom == NULL) {
		fprintf(stderr, "ROM NOT OPENED!\n");
		return -1; /* TODO rewrite */
	}

	/* NOTE now we believe that rom size will fit in our buffer */
	/* TODO rewrite it! */
	fseek(rom, 0, SEEK_END);
	size = ftell(rom);
	fseek(rom, 0, SEEK_SET);

	readed = fread(&n->ram[0x8000], size, 1, rom);

	fclose(rom);

	return 0;
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
	/*(void)frame_length;*/

	nes_tick(n);
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

	/*if (argc != 2) {
		fprintf(stderr, "ERROR: WRONG ARGUMENTS!\n");
		exit(EXIT_FAILURE);
	}

	filename = argv[1];*/

	nes_init(&n);
	/* nes_loadrom(&n, filename); */

	n.ram[0xFFFC] = 0xA9;
	fprintf(stderr, "%X PLACED IN %X\n", n.ram[0xFFFC], 0xFFFC);
	n.ram[0xFFFD] = 0x88;
	fprintf(stderr, "%X PLACED IN %X\n", n.ram[0xFFFD], 0xFFFD);
	n.ram[0xFFFE] = 0xE0;
	fprintf(stderr, "%X PLACED IN %X\n", n.ram[0xFFFE], 0xFFFE);
	n.ram[0xFFFF] = 0xF8;
	fprintf(stderr, "%X PLACED IN %X\n", n.ram[0xFFFF], 0xFFFF);

	nes_run(&n);
	nes_run(&n);
	nes_run(&n);
	nes_run(&n);

	return 0;
}
