#include <stdint.h>
#include <stdio.h>

#include "bus.h"
#include "cartrige.h"


typedef struct {
	/* r2A03 apu */
	bus bus;
	r2A03 cpu;
	r2C02 ppu;
	cartrige rom;
	uint8_t ram[RAM_SIZE];
} nes;

static double fps_ntsc = 60.0988;

static void
nes_loadrom(nes *n, const char *path)
{
	n->rom = cartrige_create(path);
}

static void
nes_tick(nes *n)
{
	/* bus_apu_tick(&n->bus); */
	bus_cpu_tick(&n->bus);

	bus_ppu_tick(&n->bus); /* use cycle? */
	bus_ppu_tick(&n->bus);
	bus_ppu_tick(&n->bus);
}

static void
nes_run(nes *n)
{
	double frame_length = 1000 / fps_ntsc;
	(void)frame_length;

	nes_tick(n);
}

static void
nes_init(nes *n)
{
	bus_init(&n->bus, &n->cpu, &n->ppu, n->ram, n->rom);
	bus_cpu_reset(&n->bus);
	/* bus_ram_reset(&n->bus); */
	bus_ppu_reset(&n->bus);
}

static void
nes_run_loop(nes *n)
{
	while (1) {
		nes_run(n);
	}
}

int
main(int argc, char **argv)
{
	nes n = {0};

	if (argc != 2) {
		fprintf(stderr, "usage: ./fami romfile\n");
		exit(EXIT_FAILURE);
	}

	nes_loadrom(&n, argv[1]);
	nes_init(&n);

	nes_run_loop(&n);
	return 0;
}
