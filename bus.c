#include <stdio.h>  /* TODO remove */
#include <stdlib.h> /* TODO remove */

#include "bus.h"

void
bus_init(bus *bus, r2A03 *cpu, uint8_t *ram, cartrige rom)
{
	bus->cpu = cpu;
	bus->ram = ram;
	bus->rom = rom;
}

void
bus_cpu_reset(bus *b)
{
	cpu_reset(b->cpu, b);
}

void
bus_cpu_tick(bus *b)
{
	cpu_tick(b->cpu);
}

void
bus_ppu_reset(bus *b)
{
	ppu_reset(b->ppu, b);
}

void
bus_ppu_tick(bus *b)
{
	ppu_tick(b->ppu);
}

void
bus_ram_reset(bus *b)
{
	mem_reset(b->ram);
}

uint8_t
bus_read(bus *b, uint16_t addr)
{
	if (addr >= 0x0000 && addr <= 0x1FFF) {
		return b->ram[addr];
	}

	if (addr >= 0x2000 && addr <= 0x3FFF) {
		return ppu_read(b->ppu, addr);
	}
	
	if (addr >= 0x8000 && addr <= 0xFFFF) {
		return cartrige_read(&b->rom, addr);
	}

	return 0; /* TODO: create error value */
}

void
bus_write(bus *b, uint16_t addr, uint8_t val)
{
	if (addr >= 0x0000 && addr < 0x8000) {
		b->ram[addr] = val; /* TODO: add check */
		return;
	}

	if (addr >= 0x8000 && addr <= 0xFFFF) {
		fprintf(stderr, "trying to write to cartrige rom space\n"); /* TODO remove */
		exit(1); /* TODO: replace with assert? */
	}
}
