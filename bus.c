#include "bus.h"
#include "cpu.h"
#include "ppu.h"

void
bus_cpu_reset(bus *b, r2A03 *c)
{
	cpu_reset(c);
	b->cpu = c;
	c->bus = b;
}

void
bus_cpu_tick(bus *b)
{
	cpu_tick(b->cpu);
}

void
bus_ppu_reset(bus *b, r2C02 *p)
{
	/* ppu_reset(p); */
	b->ppu = p;
	/* p->bus = b */
}

void
bus_ppu_tick(bus *b)
{
	ppu_tick(b->ppu);
}

void
bus_ram_reset(bus *b, uint8_t *r)
{
	mem_reset(r);
	b->ram = r;
}

uint8_t
bus_ram_read(bus *b, uint16_t addr)
{
	if (addr >= 0x0000 && addr <= 0xFFFF) {
		return b->ram[addr]; /* TODO add check */
	}

	return 0; //TODO create error value
}

void
bus_ram_write(bus *b, uint16_t addr, uint8_t val)
{
	if (addr >= 0x0000 && addr <= 0xFFFF) {
		b->ram[addr] = val; /* TODO add check */
	}
}
