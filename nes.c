#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "bus.h"
#include "controller.h"
#include "gfx.h"


#define RAM_SIZE 0x10000

typedef struct {
	/* r2A03 apu */
	bus bus;
	r2A03 cpu;
	r2C02 ppu;
	cartrige rom;
	controller controller;
	uint8_t ram[RAM_SIZE];
} nes;

static void
nes_cleanup(nes *n)
{
	cartrige_free(&n->rom);
	gfx_destroy();
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

static int
nes_should_exit()
{
	return gfx_should_exit();
}

static void
nes_draw(nes *n)
{
	uint8_t flag = bus_ppu_get_frame_ready_flag(&n->bus);

	if (flag) {
		bus_ppu_unset_frame_ready_flag(&n->bus);
		gfx_draw_frame(n->ppu.frame_buf);
	}
}

static void
nes_runloop(nes *n)
{
	while (!nes_should_exit()) {
		gfx_poll_controller(&n->controller.buttons);

		nes_tick(n);
		nes_draw(n);
	}
}

static void
nes_init(nes *n)
{
	bus_init(&n->bus, &n->cpu, &n->ppu, n->ram, &n->controller, n->rom);
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
