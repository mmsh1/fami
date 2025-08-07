#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "bus.h"
#include "cartrige.h"
#include "gfx.h"
#include "raylib.h"


typedef struct {
	/* r2A03 apu */
	bus bus;
	r2A03 cpu;
	r2C02 ppu;
	cartrige rom;
	uint8_t ram[RAM_SIZE];
} nes;

// static double fps_ntsc = 60.0988;

static void
nes_cleanup(nes *n)
{
	cartrige_free(&n->rom);
}

static void
nes_loadrom(nes *n, const char *path)
{
	n->rom = cartrige_create(path);
	// TODO: handle invalid result
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
nes_runloop(nes *n)
{
	while (!gfx_should_exit()) {
		nes_tick(n);
		gfx_draw_frame(n->ppu.frame_buf);
	}
}

static void
nes_init(nes *n)
{
	bus_init(&n->bus, &n->cpu, &n->ppu, n->ram, n->rom);
	bus_ram_reset(&n->bus);
	bus_cpu_reset(&n->bus);
	bus_ppu_reset(&n->bus);
	gfx_init(); // TODO: create layer for holding array
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
	nes_runloop(&n);
	nes_cleanup(&n);

	return 0;
}
