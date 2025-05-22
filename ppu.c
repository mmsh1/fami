#include "ppu.h"

enum {
	PPUCTRL = 0x2000,
	PPUMASK = 0x2001,
	PPUSTATUS = 0x2002,
	OAMADDR = 0x2003,
	OAMDATA = 0x2004,
	PPUSCROLL = 0x2005,
	PPUADDR = 0x2006,
	PPUDATA = 0x2007,
	OAMDMA = 0x4014
};

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

static uint8_t
ppu_palette[0x20] = {
	0x09, 0x01, 0x00, 0x01, 0x00, 0x02, 0x02, 0x0D,
	0x08, 0x10, 0x08, 0x24, 0x00, 0x00, 0x04, 0x2C,
	0x09, 0x01, 0x34, 0x03, 0x00, 0x04, 0x00, 0x14,
	0x08, 0x3A, 0x00, 0x02, 0x00, 0x20, 0x2C, 0x08
};

static uint32_t
ppu_colors[0x40] = {
	0x666666FF, 0x002A88FF, 0x1412A7FF, 0x3B00A4FF,
	0x5C007EFF, 0x6E0040FF, 0x6C0600FF, 0x561D00FF,
	0x333500FF, 0x0B4800FF, 0x005200FF, 0x004F08FF,
	0x00404DFF, 0x000000FF, 0x000000FF, 0x000000FF,

	0xADADADFF, 0x155FD9FF, 0x4240FFFF, 0x7527FEFF,
	0xA01ACCFF, 0xB71E7BFF, 0xB53120FF, 0x994E00FF,
	0x6B6D00FF, 0x388700FF, 0x0C9300FF, 0x008F32FF,
	0x007C8DFF, 0x000000FF, 0x000000FF, 0x000000FF,

	0xFFFEFFFF, 0x64B0FFFF, 0x9290FFFF, 0xC676FFFF,
	0xF36AFFFF, 0xFE6ECCFF, 0xFE8170FF, 0xEA9E22FF,
	0xBCBE00FF, 0x88D800FF, 0x5CE430FF, 0x45E082FF,
	0x48CDDEFF, 0x4F4F4FFF, 0x000000FF, 0x000000FF,

	0xFFFEFFFF, 0xC0DFFFFF, 0xD3D2FFFF, 0xE8C8FFFF,
	0xFBC2FFFF, 0xFEC4EAFF, 0xFECCC5FF, 0xF7D8A5FF,
	0xE4E594FF, 0xCFEF96FF, 0xBDF4ABFF, 0xB3F3CCFF,
	0xB5EBF2FF, 0xB8B8B8FF, 0x000000FF, 0x000000FF
};

void
ppu_reset(r2C02 *ppu, struct bus *bus, cartrige *rom)
{
	ppu->bus = bus;
	ppu->scanline = 240; /* TODO: use defined const */
	ppu->cycle = 340;    /* TODO: use defined const */
	ppu->frame = 0;
	ppu->rom = rom;
}

static uint8_t
ppu_palette_read(uint16_t addr)
{
	switch (addr) {
		case 0x3F10:
		case 0x3F14:
		case 0x3F18:
		case 0x3F1C:
			addr -= 0x10;
	}

	addr -= 0x3F00;
	addr %= 0x20;

	return ppu_palette[addr];
}

static uint8_t
ppu_internal_read(r2C02 *ppu, uint16_t addr)
{
	if (addr < 0x2000) {
		return cartrige_read(ppu->rom, addr);
	}

	/* TODO: < 0x3000, < 0x3F00 */

	if (addr < 0x4000) {
		return ppu_palette_read(addr);
	}

	return 0x0;
}

static void
debug_draw_tile(r2C02 *ppu, int x, int y, int tile_idx)
{
	uint8_t tile_hi, tile_lo;
	uint8_t color_idx;
	uint32_t bg_color, fg_color;
	int i, j;

	tile_idx *= 0x10;

	for (i = 0; i <= 8; i++) {
		tile_hi = ppu_internal_read(ppu, (uint16_t)(tile_idx + i));
		tile_lo = ppu_internal_read(ppu, (uint16_t)(tile_idx + i + 8));

		for (j = 7; j >= 0; j--) {
			color_idx = (tile_lo & 0x1) << 1 | (tile_hi & 0x1); /* from 0 to 3 */
			
			bg_color = ppu_colors[ppu_internal_read(ppu, 0x3F00)];
			fg_color = ppu_colors[ppu_internal_read(ppu, 0x3F00 | (color_idx + 6))];

			if (color_idx == 0) {
				fg_color = bg_color;
			}

			ppu->frame_buf[x + j + ((y + i) * 256)] = fg_color;
			tile_hi /= 2;
			tile_lo /= 2;
		}
	}
}

void
ppu_tick(r2C02 *ppu)
{
	/* NOTE: graphics debug
	 * development only */

	int tile_idx = 0;
	int x, y;

	/* upper left quadrant */
	for (y = 0; y < 128; y += 8) {
		for (x = 0; x < 128; x += 8) {
			debug_draw_tile(ppu, x, y, tile_idx);
			tile_idx++;
		}
	}

	/* upper right quadrant */
	for (y = 0; y < 128; y += 8) {
		for (x = 128; x < 256; x += 8) {
			debug_draw_tile(ppu, x, y, tile_idx);
		}
	}
}

uint8_t
ppu_read(r2C02 *ppu, uint16_t addr)
{
	switch (addr) {
		case PPUSTATUS:
			return ppu->ppu_status;
		case OAMDATA:
		case PPUDATA:
			break;
	}

	return 0; /* TODO: handle addr >= VRAM_SIZE ? */
}

void
ppu_write(r2C02 *ppu, uint16_t addr, uint8_t val)
{
	switch (addr) {
		case PPUCTRL:
			// TODO: create NMI
			ppu->ppu_ctrl = val;
			break;
		case PPUADDR:
			if (ppu->reg.write_flag == 0) {
				ppu->reg.tmp_addr.part.hi = val;
				ppu->reg.write_flag = 1;
			} else {
				ppu->reg.tmp_addr.part.lo = val;
				ppu->reg.curr_addr.whole = ppu->reg.tmp_addr.whole;
				ppu->reg.write_flag = 0;
			}
			break;
	}

	/* TODO: handle addr >= VRAM_SIZE ? */
}
