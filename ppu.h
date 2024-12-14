#ifndef NES_PPU_H
#define NES_PPU_H

#include <stdint.h>

enum {
	OAM_SIZE = 256,
	VRAM_SIZE = 2048
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
