#include <stdint.h>
#include <stdio.h>  /* TODO remove */
#include <stdlib.h> /* TODO remove */

#include "bus.h"

void
bus_init(bus *bus, r2A03 *cpu, r2C02 *ppu, uint8_t *ram, cartrige rom)
{
	bus->cpu = cpu;
	bus->ppu = ppu;
	bus->ram = ram;
	bus->rom = rom;
}

uint8_t
bus_cartrige_get_mirroring(bus *b)
{
	return cartrige_get_mirroring(&b->rom);
}

uint8_t
bus_cartrige_read(bus *b, uint16_t addr)
{
	return cartrige_read(&b->rom, addr);
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
bus_cpu_trigger_nmi(bus *b)
{
	cpu_trigger_nmi(b->cpu);
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
	if (addr < 0x2000) {
		return b->ram[addr % 0x800];
	}

	if (addr < 0x4000) {
		addr = 0x2000 + addr % 8; // TODO: create func for composing addr?
		return ppu_read(b->ppu, addr);
	}

	if (addr == 0x4014) {
		fprintf(stderr, "bus_read 0x4014 -> PPU OAM\n");
		return 0;
	}

	if (addr == 0x4015) {
		fprintf(stderr, "bus_read 0x4015 -> APU Register\n");
		return 0;
	}
	
	if (addr == 0x4016) {
		fprintf(stderr, "bus_read 0x4016 -> controller_1\n");
		return 0;
	}

	if (addr == 0x4017) {
		fprintf(stderr, "bus_read 0x4017 -> controller_2\n");
		return 0;
	}

	if (addr >= 0x6000) {
		return bus_cartrige_read(b, addr);
	}

	return 0; /* TODO: create error value */
}

void
bus_write(bus *b, uint16_t addr, uint8_t val)
{
	// TODO: define addresses!
	if (addr < 0x1FFF) {
		b->ram[addr] = val; /* TODO: add check */
		return;
	}

	if (addr >= 0x2000 && addr <= 0x3FFF) {
		ppu_write(b->ppu, addr, val);
	}
	
	if (addr >= 0x8000) {
		fprintf(stderr, "trying to write to cartrige rom space\n"); /* TODO remove */
		exit(1); /* TODO: replace with assert? */
	}
}
