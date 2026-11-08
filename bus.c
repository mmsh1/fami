#include <stdio.h>  /* TODO: remove */

#include "bus.h"

void
bus_init(bus *bus, apu *apu, cpu *cpu, ppu *ppu, uint8_t *ram, controller *controller, cartrige rom)
{
	bus->apu = apu;
	bus->cpu = cpu;
	bus->ppu = ppu;
	bus->ram = ram;
	bus->controller = controller;
	bus->rom = rom;
}

void
bus_apu_reset(bus *b)
{
	apu_reset(b->apu, b);
}

void
bus_apu_tick(bus *b)
{
	apu_tick(b->apu);
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
	if (addr < 0x2000) { return b->ram[addr % 0x800]; }
	if (addr < 0x4000) { return ppu_read(b->ppu, 0x2000 + addr % 8); } /* TODO: create func for composing addr */
	if (addr == 0x4014) { return 0; }
	if (addr == 0x4015) { return apu_read(b->apu, addr)  | (b->cpu->open_bus & 0x20); }
	if (addr == 0x4016) { return controller_read(b->controller) & 0x1F | (b->cpu->open_bus & 0xE0); }
	if (addr == 0x4017) { return b->cpu->open_bus; }
	if (addr >= 0x6000) { return bus_cartrige_read(b, addr); }
	return b->cpu->open_bus;
}

void
bus_write(bus *b, uint16_t addr, uint8_t val)
{
	/* TODO: define addresses? */
	/* TODO: reogranize */
	if (addr < 0x2000) { b->ram[addr % 0x800] = val; }
	else if (addr < 0x4000) { ppu_write(b->ppu, 0x2000 + addr % 8, val); } /* TODO: create func for composing addr */
	else if (addr < 0x4014) { apu_write(b->apu, addr, val); }
	else if (addr == 0x4014) { ppu_write(b->ppu, addr, val); }
	else if (addr == 0x4015) { apu_write(b->apu, addr, val); }
	else if (addr == 0x4016) { controller_write(b->controller, val); }
	else if (addr == 0x4017) { apu_write(b->apu, addr, val); }
	else if (addr >= 0x6000) { cartrige_write(&b->rom, addr, val); }
}
