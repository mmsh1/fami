#ifndef NES_PPU_H
#define NES_PPU_H

#include <stdint.h>

enum {
	OAM_SIZE = 256,
	VRAM_SIZE = 2048
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

struct bus;

typedef struct {
	uint8_t ppu_ctrl;   /* PPUCTRL   $2000 */
	uint8_t ppu_mask;   /* PPUMASK   $2001 */
	uint8_t ppu_status; /* PPUSTATUS $2002 */
	uint8_t oam_addr;   /* OAMADDR   $2003 */
	uint8_t oam_data;   /* OAMDATA   $2004 */
	uint8_t ppu_scroll; /* PPUSCROLL $2005 */
	uint8_t ppu_addr;   /* PPUADDR   $2006 */
	uint8_t ppu_data;   /* PPUDATA   $2007 */
	uint8_t oam_dma;    /* OAMDMA    $4014 */

	uint8_t vram[VRAM_SIZE];
	uint8_t oam[OAM_SIZE];

	int scanline;
	int cycle;

	struct bus *bus;
} r2C02;

void ppu_reset(r2C02 *, struct bus *);
void ppu_tick(r2C02 *);
uint8_t ppu_read(r2C02 *, uint16_t);
void ppu_write(r2C02 *, uint16_t, uint8_t);

#endif /* NES_PPU_H */
