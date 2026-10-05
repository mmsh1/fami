#include <stdio.h>  /* TODO: remove */
#include <stdlib.h> /* exit */

#include "bus.h"
#include "controller.h"

void
bus_init(bus *bus, r2A03 *cpu, r2C02 *ppu, uint8_t *ram, controller *controller, cartrige rom)
{
	bus->cpu = cpu;
	bus->ppu = ppu;
	bus->ram = ram;
	bus->controller = controller;
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
bus_cartrige_write(bus *b, uint16_t addr, uint8_t val)
{
	cartrige_write(&b->rom, addr, val);
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

uint64_t
bus_cpu_get_total_cycles(bus *b)
{
	return cpu_get_total_cycles(b->cpu);
}

void
bus_cpu_set_stall_cycles(bus *b, uint64_t cycles)
{
	cpu_set_stall_cycles(b->cpu, cycles);
}

uint8_t
bus_ppu_get_frame_ready_flag(bus *b)
{
	return ppu_get_frame_ready_flag(b->ppu);
}

void
bus_ppu_unset_frame_ready_flag(bus *b)
{
	ppu_unset_frame_ready_flag(b->ppu);
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
		return 0;
	}

	if (addr == 0x4015) {
		return 0;
	}
	
	if (addr == 0x4016) {
		return controller_read(b->controller);;
	}

	if (addr == 0x4017) {
		/* TODO: controller 2 read */
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
	if (addr < 0x2000) {
		b->ram[addr % 0x800] = val; /* TODO: add check */
		return;
	}

	if (addr < 0x4000) {
		addr = 0x2000 + addr % 8; // TODO: create func for composing addr?
		ppu_write(b->ppu, addr, val);
	}

	if (addr == 0x4014) {
		ppu_write(b->ppu, addr, val);
	}

	if (addr == 0x4016) {
		controller_write(b->controller, val);
	}

	if (addr == 0x4017) {
		/* TODO: write controller 2 */
	}

	
	if (addr >= 0x8000) {
		fprintf(stderr, "trying to write to cartrige rom space\n"); /* TODO: remove */
		exit(1); /* TODO: replace with assert? */
	}
}
