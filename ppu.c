#include "ppu.h"

void
ppu_reset(r2C02 *ppu, struct bus *bus)
{
	ppu->bus = bus;
}

void
ppu_tick(r2C02 *ppu)
{
	(void)ppu;
}

uint8_t
ppu_read(r2C02 *ppu, uint16_t addr)
{
	if (addr < VRAM_SIZE) {
		return ppu->vram[addr];
	}
	return 0; /* TODO: handle addr >= VRAM_SIZE ? */
}

void
ppu_write(r2C02 *ppu, uint16_t addr, uint8_t val)
{
	if (addr < VRAM_SIZE) {
		ppu->vram[addr] = val;
	}
	/* TODO: handle addr >= VRAM_SIZE ? */
}
