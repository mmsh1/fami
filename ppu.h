#ifndef NES_PPU_H
#define NES_PPU_H

#include <stdint.h>

enum {
	VRAM_SIZE = 2048,
};

enum {
	OAM_SIZE_BYTES = 256,
	OAM2_SIZE_BYTES = 32,
	OAM_SIZE_SPRITES = OAM_SIZE_BYTES / 4,
	OAM2_SIZE_SPRITES = OAM2_SIZE_BYTES / 4
};

enum {
	SCREEN_WIDTH = 256,
	SCREEN_HEIGHT = 240
};

struct bus; /* NOTE: to avoid circular include */

typedef struct {
	uint8_t pos_y;
	uint8_t tile_idx;
	uint8_t attributes;
	uint8_t pos_x;
} sprite;

typedef struct {
	uint8_t x;
	uint8_t attributes;
	uint8_t tile_lo;
	uint8_t tile_hi;
	uint8_t shift_counter;
} sprite_cache;

typedef union {
	struct {
		uint8_t lo;
		uint8_t hi;
	} part;
	uint16_t whole;
} address;

typedef struct {
	address curr_addr;
	address tmp_addr;
	uint8_t fine_x_scroll;
	uint8_t write_flag;
} loopy_reg;

typedef struct {
	uint8_t ppu_ctrl;
	uint8_t ppu_mask;
	uint8_t ppu_status;
	uint8_t oam_addr;

	uint8_t frame_ready_flag;
	uint8_t read_buffer;
	uint8_t vram[VRAM_SIZE];
	uint8_t io_bus;

	union {
		sprite sprites[OAM_SIZE_SPRITES];
		uint8_t bytes[OAM_SIZE_BYTES];
	} oam;

	union {
		sprite sprites[OAM2_SIZE_SPRITES];
		uint8_t bytes[OAM2_SIZE_BYTES];
	} oam2;

	sprite_cache fetched_sprites[8];

	int scanline; /* [0..261] */
	int cycle;    /* [0..340] */
	int frame;
	int active_sprites;
	int sprite_zero_index;

	uint8_t palette[0x20];
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

	loopy_reg vram_reg;

	struct bus *bus;

	uint64_t total_cycles; /* TODO: remove! */
} ppu;

uint8_t ppu_get_frame_ready_flag(ppu *);
void ppu_unset_frame_ready_flag(ppu *);
void ppu_reset(ppu *, struct bus *);
void ppu_tick(ppu *);
uint8_t ppu_read(ppu *, uint16_t);
void ppu_write(ppu *, uint16_t, uint8_t);

#endif /* NES_PPU_H */
