#ifndef NES_PPU_H
#define NES_PPU_H

#include <stdint.h>

enum {
	VRAM_SIZE = 2048,
	OAM_SIZE = 256,
	OAM2_SIZE = 32
};

enum {
	SCREEN_WIDTH = 256,
	SCREEN_HEIGHT = 240
};

/* NOTE: to use these functions we have to import bus.h
 * But we can't do it due to circular include (they will include each other).
 * Therefore, we are using forward declaration. */
struct bus;
uint8_t bus_cartrige_read(struct bus *, uint16_t);
void bus_cartrige_write(struct bus *, uint16_t, uint8_t);
uint8_t bus_cartrige_get_mirroring(struct bus *);
void bus_cpu_trigger_nmi(struct bus *);


typedef struct {
	uint8_t pos_x;
	uint8_t pos_y;
	uint8_t tile_idx;
	uint8_t attributes;
} sprite;

typedef union {
	struct {
		uint8_t lo;
		uint8_t hi;
	} part;
	uint16_t whole;
} address;

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

	uint8_t read_buffer;
	uint8_t write_buffer;
	uint8_t frame_ready_flag;
	uint8_t suppress_nmi_flag;

	uint8_t vram[VRAM_SIZE];
	uint8_t oam[OAM_SIZE];
	uint8_t oam2[OAM2_SIZE];

	int scanline; /* [0..261] */
	int cycle;    /* [0..340] */
	int frame;

	sprite sprite_table[8];
	uint32_t frame_buf[SCREEN_WIDTH * SCREEN_HEIGHT];

	struct {
		uint16_t tile_lo;
		uint16_t tile_hi;
		uint16_t attr_lo;
		uint16_t attr_hi;
	} shift;

	struct {
		uint8_t tile_lo;
		uint8_t tile_hi;
		uint8_t attr;
		uint8_t tile_id;
	} next_tile;

	struct {
		address curr_addr;
		address tmp_addr;
		uint8_t fine_x_scroll;
		uint8_t write_flag;
	} vram_reg;

	struct bus *bus;
} r2C02;

uint8_t ppu_get_frame_ready_flag(r2C02 *);
void ppu_unset_frame_ready_flag(r2C02 *);
void ppu_reset(r2C02 *, struct bus *);
void ppu_tick(r2C02 *);
uint8_t ppu_read(r2C02 *, uint16_t);
void ppu_write(r2C02 *, uint16_t, uint8_t);

#endif /* NES_PPU_H */
