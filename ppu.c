#include "ppu.h"

enum {
	PPUCTRL_NMI_ENABLE = 0x80,
	PPUCTRL_MASTER_SLAVE = 0x40,
	PPUCTRL_SPRITE_HEIGHT = 0x20,
	PPUCTRL_BACKGROUND_TILE_SELECT = 0x10,
	PPUCTRL_SPRITE_TILE_SELECT = 0x08,
	PPUCTRL_INCREMENT_MODE = 0x04,
	PPUCTRL_NAMETABLE_SELECT = 0x03
};

enum {
	PPUMASK_BGR = 0xE0,
	PPUMASK_SPRITE_ENABLE = 0x10,
	PPUMASK_BACKGROUND_ENABLE = 0x08,
	PPUMASK_SPRITE_LEFT_COL_ENABLE = 0x04,
	PPUMASK_BACKGROUND_LEFT_COL_ENABLE = 0x02,
	PPUMASK_GREYSCALE = 0x01
};

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
