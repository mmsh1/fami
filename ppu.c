#include "ines.h"
#include "ppu.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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
	PALETTE_START = 0x3F00,
	NAMETABLE_SIZE = 0x1000
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
	PPUMASK_BGR = 0xE0,                        /* 1110 0000 */
	PPUMASK_SPRITE_ENABLE = 0x10,              /* 0001 0000 -> (1 << 4) */
	PPUMASK_BACKGROUND_ENABLE = 0x08,          /* 0000 1000 -> (1 << 3) */
	PPUMASK_SPRITE_LEFT_COL_ENABLE = 0x04,     /* 0000 0100 -> (1 << 2) */
	PPUMASK_BACKGROUND_LEFT_COL_ENABLE = 0x02, /* 0000 0010 -> (1 << 1) */
	PPUMASK_GREYSCALE = 0x01                   /* 0000 0001 -> (1 << 0) */
};

enum {
	PPUSTATUS_VBLANK_ENABLED = 0x80,  /* 1000 0000 -> (1 << 7) */
	PPUSTATUS_SPRITE_ZERO_HIT = 0x40, /* 0100 0000 -> (1 << 6) */
	PPUSTATUS_SPRITE_OVERFLOW = 0x20  /* 0010 0000 -> (1 << 5) */
};

enum {
	FINE_Y_SCROLL = 0x7000,   /* 0111 0000 0000 0000 */
	NAMETABLE_Y = 0x0800,     /* 0000 1000 0000 0000 */
	NAMETABLE_X = 0x0400,     /* 0000 0100 0000 0000 */
	COARSE_Y_SCROLL = 0x03E0, /* 0000 0011 1110 0000 */
	COARSE_X_SCROLL = 0x001F  /* 0000 0000 0001 1111 */
};

enum {
	SPRITE_ATTR_VERTICAL_FLIP = 0x80,   /* 1000 0000 -> (1 << 7)*/
	SPRITE_ATTR_HORIZONTAL_FLIP = 0x40, /* 0100 0000 -> (1 << 6)*/
	SPRITE_ATTR_PRIORITY = 0x20,        /* 0010 0000 -> (1 << 5)*/
	SPRITE_ATTR_PALETTE = 0x03          /* 0000 0011 */
};

static uint8_t
ppu_palette[0x20] = {0};

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

static inline uint8_t get_color_idx_in_palette(uint8_t lo, uint8_t hi) { return (lo & 0x1) << 1 | (hi & 0x1); }  /* from 0 to 3 */

static inline int in_range(int num, int lo, int hi)     { return (num >= lo) && (num < hi); }
static inline int in_range_inc(int num, int lo, int hi) { return (num >= lo) && (num <= hi); }

static inline int get_bit(uint8_t reg, uint8_t mask)     { return reg & mask; }
static inline void set_bit(uint8_t *reg, uint8_t mask)   { *reg |= mask; }
static inline void unset_bit(uint8_t *reg, uint8_t mask) { *reg &= ~mask; }

static inline int is_bg_tile_select_mode_enabled(uint8_t ctrl) { return get_bit(ctrl, PPUCTRL_BACKGROUND_TILE_SELECT); }
static inline int is_increment_mode_enabled(uint8_t ctrl)      { return get_bit(ctrl, PPUCTRL_INCREMENT_MODE); }
static inline int is_nmi_enabled(uint8_t ctrl)                 { return get_bit(ctrl, PPUCTRL_NMI_ENABLE); }
static inline int is_tall_sprites_enabled(uint8_t ctrl)        { return get_bit(ctrl, PPUCTRL_SPRITE_HEIGHT); }
static inline int is_sprite_tile_select_enabled(uint8_t ctrl)  { return get_bit(ctrl, PPUCTRL_SPRITE_TILE_SELECT); }
static inline int is_bg_left_col_enabled(uint8_t mask)         { return get_bit(mask, PPUMASK_BACKGROUND_LEFT_COL_ENABLE); }
static inline int is_fg_left_col_enabled(uint8_t mask)         { return get_bit(mask, PPUMASK_SPRITE_LEFT_COL_ENABLE); }
static inline int is_bg_rendering_enabled(uint8_t mask)        { return get_bit(mask, PPUMASK_BACKGROUND_ENABLE); }
static inline int is_fg_rendering_enabled(uint8_t mask)        { return get_bit(mask, PPUMASK_SPRITE_ENABLE); }
static inline int is_vblank_enabled(uint8_t status)            { return get_bit(status, PPUSTATUS_VBLANK_ENABLED); }
static inline int is_sprite_zero_hit_enabled(uint8_t status)   { return get_bit(status, PPUSTATUS_SPRITE_ZERO_HIT); }
static inline int is_rendering_enabled(uint8_t mask)           { return is_fg_rendering_enabled(mask) || is_bg_rendering_enabled(mask); }
static inline int get_sprites_height(uint8_t ctrl)             { return is_tall_sprites_enabled(ctrl) ? 16 : 8; }

static inline int is_sprite_horizontal_flip_enabled(uint8_t attr) { return get_bit(attr, SPRITE_ATTR_HORIZONTAL_FLIP); }
static inline int is_sprite_vertical_flip_enabled(uint8_t attr)   { return get_bit(attr, SPRITE_ATTR_VERTICAL_FLIP); }
static inline int get_sprite_palette(uint8_t attr)                { return attr & SPRITE_ATTR_PALETTE; }

static inline void set_status_sprite_overflow(uint8_t *status) { set_bit(status, PPUSTATUS_SPRITE_OVERFLOW); }
static inline void set_status_vblank_enabled(uint8_t *status)  { set_bit(status, PPUSTATUS_VBLANK_ENABLED); }

static inline void unset_status_vblank_enabled(uint8_t *status) { unset_bit(status, PPUSTATUS_VBLANK_ENABLED); }

static inline uint16_t loopy_get(uint16_t reg, uint16_t mask, uint8_t shift) { return (reg & mask) >> shift; }
static inline uint16_t loopy_get_coarse_x(address reg)                       { return loopy_get(reg.whole, COARSE_X_SCROLL, 0); }
static inline uint16_t loopy_get_coarse_y(address reg)                       { return loopy_get(reg.whole, COARSE_Y_SCROLL, 5); }
static inline uint16_t loopy_get_fine_y(address reg)                         { return loopy_get(reg.whole, FINE_Y_SCROLL, 12); }
static inline uint16_t loopy_get_nametable_x(address reg)                    { return loopy_get(reg.whole, NAMETABLE_X, 10); }
static inline uint16_t loopy_get_nametable_y(address reg)                    { return loopy_get(reg.whole, NAMETABLE_Y, 11); }

static inline void loopy_set(uint16_t *reg, uint16_t mask, uint16_t val, uint8_t shift) { *reg = (*reg & ~mask) | ((val << shift) & mask); }
static inline void loopy_set_coarse_x(address *reg, uint16_t val)                       { loopy_set(&reg->whole, COARSE_X_SCROLL, val, 0); }
static inline void loopy_set_coarse_y(address *reg, uint16_t val)                       { loopy_set(&reg->whole, COARSE_Y_SCROLL, val, 5); }
static inline void loopy_set_fine_y(address *reg, uint16_t val)                         { loopy_set(&reg->whole, FINE_Y_SCROLL, val, 12); }
static inline void loopy_set_nametable_x(address *reg, uint16_t val)                    { loopy_set(&reg->whole, NAMETABLE_X, val, 10); }
static inline void loopy_set_nametable_y(address *reg, uint16_t val)                    { loopy_set(&reg->whole, NAMETABLE_Y, val, 11); }

static inline void loopy_upd_from_tmp_coarse_x(loopy_reg *reg)    { loopy_set_coarse_x(&reg->curr_addr, loopy_get_coarse_x(reg->tmp_addr)); }
static inline void loopy_upd_from_tmp_coarse_y(loopy_reg *reg)    { loopy_set_coarse_y(&reg->curr_addr, loopy_get_coarse_y(reg->tmp_addr)); }
static inline void loopy_upd_from_tmp_fine_y(loopy_reg *reg)      { loopy_set_fine_y(&reg->curr_addr, loopy_get_fine_y(reg->tmp_addr)); }
static inline void loopy_upd_from_tmp_nametable_x(loopy_reg *reg) { loopy_set_nametable_x(&reg->curr_addr, loopy_get_nametable_x(reg->tmp_addr)); }
static inline void loopy_upd_from_tmp_nametable_y(loopy_reg *reg) { loopy_set_nametable_y(&reg->curr_addr, loopy_get_nametable_y(reg->tmp_addr)); }

static inline void loopy_toggle_nametable_x(address *reg) { reg->whole ^= NAMETABLE_X; }
static inline void loopy_toggle_nametable_y(address *reg) { reg->whole ^= NAMETABLE_Y; }

static inline int is_palette_addr(uint16_t addr) { return (addr % 0x4000) >= PALETTE_START; }

static inline sprite
create_empty_sprite()
{
	return (sprite){
		.pos_y = 0xFF,
		.tile_idx = 0xFF,
		.attributes = 0xFF,
		.pos_x = 0xFF
	};
}

static inline void
set_pixel(r2C02 *ppu, int x, int y, uint32_t color)
{
	uint8_t r = (color >> 24) & 0xFF;
	uint8_t g = (color >> 16) & 0xFF;
	uint8_t b = (color >> 8) & 0xFF;
	uint8_t a = color & 0xFF;
	uint32_t rgba = (a << 24) | (b << 16) | (g << 8) | r;

	ppu->frame_buf[x + y * 256] = rgba;
}

static inline uint8_t
select_priority(uint8_t bg_pixel, uint8_t fg_pixel)
{
	return (fg_pixel == 0) ? bg_pixel : fg_pixel;
	/* TODO: add sprite priority */
}

static inline uint8_t
reverse_bits(uint8_t x) {
	x = (x >> 4) | (x << 4);
	x = ((x >> 2) & 0x33) | ((x & 0x33) << 2);
	x = ((x >> 1) & 0x55) | ((x & 0x55) << 1);
	return x;
}

static uint16_t
mirror_nametable_addr(r2C02 *ppu, uint16_t addr)
{
	mirroring_type mt = bus_cartrige_get_mirroring(ppu->bus);
	switch (mt) {
		case HORIZONTAL_MIRRORING:
			addr = ((addr / 2) & 0x400) + (addr % 0x400);
			break;
		case VERTICAL_MIRRORING:
			addr %= 0x800;
			break;
		case SINGLE_SCREEN_A:
		case SINGLE_SCREEN_B:
		case FOUR_SCREEN:
		case INVALID_MIRRORING: // TODO:
		default:
			addr -= 0x2000;
	}

	return addr;
}

static uint8_t
nametable_read(r2C02 *ppu, uint16_t addr)
{
	addr = mirror_nametable_addr(ppu, addr);
	return ppu->vram[addr];
}

static void
nametable_write(r2C02 *ppu, uint16_t addr, uint8_t val)
{
	addr = mirror_nametable_addr(ppu, addr);
	ppu->vram[addr] = val;
}

static uint16_t
palette_index(uint16_t addr)
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

	return addr;
}

static inline uint8_t
palette_read(uint16_t addr)
{
	addr = palette_index(addr);
	return ppu_palette[addr];
}

static inline void
palette_write(uint16_t addr, uint8_t val)
{
	addr = palette_index(addr);
	ppu_palette[addr] = val;
}

static inline void
update_shift(r2C02 *ppu)
{
	ppu->shift.tile_lo <<= 1;
	ppu->shift.tile_hi <<= 1;
	ppu->shift.attr_lo <<= 1;
	ppu->shift.attr_hi <<= 1;
}

static inline void
load_next_tile(r2C02 *ppu)
{
	ppu->shift.tile_lo |= ppu->next_tile.tile_lo;
	ppu->shift.tile_hi |= ppu->next_tile.tile_hi;
	ppu->shift.attr_lo |= (ppu->next_tile.attr & 0x1) ? 0xFF : 0x00;
	ppu->shift.attr_hi |= (ppu->next_tile.attr & 0x2) ? 0xFF : 0x00;
}

static uint8_t
get_bg_pixel_addr(r2C02 *ppu)
{
	uint8_t bit_hi, bit_lo, color;
	uint8_t pal_hi, pal_lo;
	uint8_t x_scroll = ppu->vram_reg.fine_x_scroll;
	uint16_t mask = 0x8000 >> x_scroll;
	uint16_t shift = 15 - x_scroll;
	uint16_t palette;
	
	if (!is_bg_left_col_enabled(ppu->ppu_mask) && ppu->cycle < 9) {
		return 0;
	}

	bit_hi = ((ppu->shift.tile_hi & mask) >> shift) & 0x01;
	bit_lo = ((ppu->shift.tile_lo & mask) >> shift) & 0x01;
	color = (bit_hi << 1) | bit_lo;

	if (color == 0) {
		return 0;
	}

	pal_hi = ((ppu->shift.attr_hi & mask) >> shift) & 0x01;
	pal_lo = ((ppu->shift.attr_lo & mask) >> shift) & 0x01;
	palette = (pal_hi << 1) | pal_lo;

	return palette * 4 + color;
}

static uint8_t
get_fg_pixel_addr(r2C02 *ppu)
{
	uint8_t bit_hi, bit_lo, color;
	uint8_t sprite_x;
	uint8_t pixel_in_sprite;
	uint16_t palette;
	int x = ppu->cycle - 1;
	int i;

	if (!is_fg_left_col_enabled(ppu->ppu_mask) && ppu->cycle < 9) {
		return 0;
	}

	for (i = 7; i >= 0; i--) {
		sprite_x = ppu->fetched_sprites[i].x;

		if (sprite_x == 0xFF) {
			continue;
		}

		if (x < sprite_x || x >= sprite_x + 8) {
			continue;
		}

		pixel_in_sprite = x - sprite_x;

		bit_hi = (ppu->fetched_sprites[i].tile_hi >> pixel_in_sprite) & 0x01;
		bit_lo = (ppu->fetched_sprites[i].tile_lo >> pixel_in_sprite) & 0x01;
		color = (bit_hi << 1) | bit_lo;

		if (color == 0) {
			continue;
		}

		palette = 4 + get_sprite_palette(ppu->fetched_sprites[i].attributes);
		return palette * 4 + color;
	}

	return 0;
}

static void
oam_dma_write(r2C02 *ppu, uint8_t idx)
{
	/* TODO: should we set stall cycles or add them? investigate */
	uint64_t total = bus_cpu_get_total_cycles(ppu->bus);
	uint64_t stall = total % 2 == 1 ? 513 : 514;
	uint8_t addr;
	int i;

	bus_cpu_set_stall_cycles(ppu->bus, stall);

	for (i = 0; i < OAM_SIZE_BYTES; i++) {
		addr = ppu->oam_addr + (uint8_t)i;
		ppu->oam.bytes[addr] = bus_read(ppu->bus, idx * OAM_SIZE_BYTES + i);
	}
}

static void
scroll_reg_write(r2C02 *ppu, uint8_t val)
{
	/* TODO: unreadable! rewrite */
	if (ppu->vram_reg.write_flag == 0) {
		/*
			NOTE:
			t: ....... ...ABCDE <- d: ABCDE...
			x:              FGH <- d: .....FGH
			w:                  <- 1
		*/
		loopy_set_coarse_x(&ppu->vram_reg.tmp_addr, val >> 3);
		ppu->vram_reg.fine_x_scroll = val & 0x7;
		ppu->vram_reg.write_flag = 1;
	} else {
		/*
			NOTE:
			t: FGH..AB CDE..... <- d: ABCDEFGH
			w:                  <- 0
		*/
		loopy_set_fine_y(&ppu->vram_reg.tmp_addr, val & 0x7);
		loopy_set_coarse_y(&ppu->vram_reg.tmp_addr, val >> 3);
		ppu->vram_reg.write_flag = 0;
	}
}

static void
vblank_end(r2C02 *ppu)
{
	unset_status_vblank_enabled(&ppu->ppu_status);
}

static void
vblank_start(r2C02 *ppu)
{
	set_status_vblank_enabled(&ppu->ppu_status);
	ppu->frame_ready_flag = 1;

	if (is_nmi_enabled(ppu->ppu_ctrl)) {
		bus_cpu_trigger_nmi(ppu->bus);
	}
}

static void
vram_addr_increment(r2C02 *ppu)
{
	uint16_t inc_val = is_increment_mode_enabled(ppu->ppu_ctrl) ? 32 : 1;
	ppu->vram_reg.curr_addr.whole += inc_val;
}

static uint16_t
vram_addr_read(r2C02 *ppu)
{
	uint16_t addr = ppu->vram_reg.curr_addr.whole;
	vram_addr_increment(ppu);
	return addr;
}

static uint8_t
vram_data_read(r2C02 *ppu, uint16_t addr)
{
	if (addr < 0x2000) {
		return bus_cartrige_read(ppu->bus, addr);
	}

	if (addr < 0x3F00) {
		return nametable_read(ppu, addr);
	}

	if (addr < 0x4000) {
		return palette_read(addr);
	}

	fprintf(stderr, "invalid vram_data_read\n");
	exit(1);
	//return 0x0; /* TODO: assert? */
}

static void
vram_data_write(r2C02 *ppu, uint16_t addr, uint8_t val)
{
	if (addr < 0x2000) {
		bus_cartrige_write(ppu->bus, addr, val);
	} else if (addr < 0x3F00) {
		nametable_write(ppu, addr, val);
	} else if (addr < 0x4000) {
		palette_write(addr, val);
	}
}

static uint8_t
ppustatus_read(r2C02 *ppu)
{
	uint8_t res = ppu->ppu_status;
	unset_status_vblank_enabled(&ppu->ppu_status);
	ppu->vram_reg.write_flag = 0;
	return res;
}

static uint8_t
ppudata_read(r2C02 *ppu)
{
	uint16_t addr = ppu->vram_reg.curr_addr.whole;
	uint8_t val = ppu->read_buffer;

	vram_addr_increment(ppu);

	if (!is_palette_addr(addr)) {
		ppu->read_buffer = vram_data_read(ppu, addr);
		return val;
	}

	ppu->read_buffer = vram_data_read(ppu, addr - NAMETABLE_SIZE);
	return vram_data_read(ppu, addr);
}

static inline void
vram_reg_write(r2C02 *ppu, uint8_t val)
{
	if (ppu->vram_reg.write_flag == 0) {
		//ppu->vram_reg.tmp_addr.part.hi = val;
		ppu->vram_reg.tmp_addr.part.hi = val & 0x3F;
		//ppu->vram_reg.tmp_addr.part.lo = 0;
		ppu->vram_reg.write_flag = 1;
	} else {
		ppu->vram_reg.tmp_addr.part.lo = val;
		ppu->vram_reg.curr_addr = ppu->vram_reg.tmp_addr;
		ppu->vram_reg.tmp_addr.whole = 0;
		ppu->vram_reg.write_flag = 0;
	}
}

static inline uint32_t
nes_palette_to_rgb(uint16_t color_idx)
{
	return ppu_colors[color_idx & 0x3F];
}

static void
render_pixel(r2C02 *ppu)
{
	int x = ppu->cycle - 1;
	int y = ppu->scanline;

	uint8_t bg_pixel_addr = is_bg_rendering_enabled(ppu->ppu_mask) ? get_bg_pixel_addr(ppu) : 0;
	uint8_t fg_pixel_addr = is_fg_rendering_enabled(ppu->ppu_mask) ? get_fg_pixel_addr(ppu) : 0;

	uint16_t pixel_addr = 0x3F00 + select_priority(bg_pixel_addr, fg_pixel_addr);
	uint16_t color_idx = vram_data_read(ppu, pixel_addr);
	uint32_t color = nes_palette_to_rgb(color_idx);

	set_pixel(ppu, x, y, color);
}

static void
clear_sprites(r2C02 *ppu)
{
	int i;

	for (i = 0; i < OAM2_SIZE_BYTES; i++) {
		ppu->oam2.bytes[i] = 0xFF;
	}
}

static void
evaluate_sprites(r2C02 *ppu)
{
	uint8_t m = 0, n = 0;
	uint8_t y = 0;

	int sprite_top = 0;
	int sprite_bottom = 0;

	ppu->active_sprites = 0;

	for (; ppu->active_sprites < 8 && n < OAM_SIZE_SPRITES; n++) {
		y = ppu->oam.sprites[n].pos_y;

		sprite_top = y;
		sprite_bottom = y + get_sprites_height(ppu->ppu_ctrl);

		if (in_range(ppu->scanline, sprite_top, sprite_bottom)) {
			ppu->oam2.sprites[ppu->active_sprites] = ppu->oam.sprites[n];
			ppu->active_sprites++;
		}
	}

	/* NOTE: here we have to implement NES sprite overflow bug.
	 * See: https://www.nesdev.org/wiki/PPU_sprite_evaluation#Sprite_overflow_bug
	 */
	/* TODO: temporary solution, not accurate */
	for (; n < OAM_SIZE_SPRITES; n++) {
		y = ppu->oam.sprites[n].pos_y;
		
		if (in_range(ppu->scanline, sprite_top, sprite_bottom)) {
			set_status_sprite_overflow(&ppu->ppu_status);
			break;
		}
	}
}

static uint16_t
get_sprite_addr(r2C02 *ppu, sprite sprite)
{
	int sprite_height = get_sprites_height(ppu->ppu_ctrl);
	uint8_t y_pos = ppu->scanline - sprite.pos_y;
	uint16_t tile_idx = sprite.tile_idx;
	uint16_t pattern_table;

	if (is_sprite_vertical_flip_enabled(sprite.attributes)) {
		y_pos = sprite_height - y_pos - 1;
	}

	if (sprite_height == 8) {
		pattern_table = is_sprite_tile_select_enabled(ppu->ppu_ctrl) ? 0x1000 : 0;
	} else {
		pattern_table = (tile_idx & 1) ? 0x1000 : 0;
		tile_idx &= 0xFE; /* 1111 1110 */

		if (y_pos >= 8) {
			tile_idx += 1;
			y_pos -= 8;
		}
	}

	return pattern_table + tile_idx * 16 + y_pos;
}

static void
fetch_sprites(r2C02 *ppu)
{
	uint8_t tile_lo, tile_hi;
	uint16_t tile_idx;
	int i;
	sprite current_sprite;

	for (i = 0; i < 8; i++) {
		if (i < ppu->active_sprites) {
			current_sprite = ppu->oam2.sprites[i];
		} else {
			current_sprite = create_empty_sprite();

			/* NOTE: For the first empty sprite slot, this will consist of sprite #63's
			 * Y-coordinate followed by 3 $FF bytes; for subsequent empty sprite slots,
			 * this will be four $FF bytes.
			 * See: https://www.nesdev.org/wiki/PPU_sprite_evaluation.
			 */
			if (i == ppu->active_sprites) {
				current_sprite.pos_y = ppu->oam.sprites[63].pos_y;
			}
		}

		tile_idx = get_sprite_addr(ppu, current_sprite);
		tile_lo = vram_data_read(ppu, tile_idx);
		tile_hi = vram_data_read(ppu, tile_idx + 8);

		if (!is_sprite_horizontal_flip_enabled(current_sprite.attributes)) {
			tile_lo = reverse_bits(tile_lo);
			tile_hi = reverse_bits(tile_hi);
		}

		ppu->fetched_sprites[i].tile_lo = tile_lo;
		ppu->fetched_sprites[i].tile_hi = tile_hi;
		ppu->fetched_sprites[i].attributes = current_sprite.attributes;
		ppu->fetched_sprites[i].x = current_sprite.pos_x;
	}
}

static void
fetch_attr_table(r2C02 *ppu)
{
	address addr = ppu->vram_reg.curr_addr;
	/* TODO: rewrite! */
	uint16_t attr_byte_addr = 0x23C0 | (addr.whole & 0x0C00) | ((addr.whole >> 4) & 0x38) | ((addr.whole >> 2) & 0x07);
	uint8_t attr_byte = vram_data_read(ppu, attr_byte_addr);

	uint16_t coarse_x = loopy_get_coarse_x(addr);
	uint16_t coarse_y = loopy_get_coarse_y(addr);
	uint8_t shift = (coarse_y & 0x02) << 1 | (coarse_x & 0x02);

	ppu->next_tile.attr = (attr_byte >> shift) & 0x03;
}

static void
update_x_scroll(r2C02 *ppu)
{
	uint16_t coarse_x = loopy_get_coarse_x(ppu->vram_reg.curr_addr);

	if (coarse_x == 31) {
		loopy_set_coarse_x(&ppu->vram_reg.curr_addr, 0);
		loopy_toggle_nametable_x(&ppu->vram_reg.curr_addr);
	} else {
		loopy_set_coarse_x(&ppu->vram_reg.curr_addr, coarse_x + 1);
	}
}

static void
update_y_scroll(r2C02 *ppu)
{
	uint16_t fine_y = loopy_get_fine_y(ppu->vram_reg.curr_addr);
	uint16_t coarse_y;

	if (fine_y < 7) {
		loopy_set_fine_y(&ppu->vram_reg.curr_addr, fine_y + 1);
	} else {
		coarse_y = loopy_get_coarse_y(ppu->vram_reg.curr_addr);
		loopy_set_fine_y(&ppu->vram_reg.curr_addr, 0);

		switch (coarse_y) {
			case 29:
				loopy_set_coarse_y(&ppu->vram_reg.curr_addr, 0);
				loopy_toggle_nametable_y(&ppu->vram_reg.curr_addr);
				break;
			case 31:
				loopy_set_coarse_y(&ppu->vram_reg.curr_addr, 0);
				break;
			default:
				loopy_set_coarse_y(&ppu->vram_reg.curr_addr, coarse_y + 1);
				break;
		}
	}
}

static void
fetch_tile_id(r2C02 *ppu)
{
	uint16_t addr = 0x2000 | (ppu->vram_reg.curr_addr.whole & 0x0FFF);
	ppu->next_tile.tile_id = vram_data_read(ppu, addr);
}

static uint16_t
bg_tile_addr(r2C02 *ppu)
{
	uint16_t pattern_table = is_bg_tile_select_mode_enabled(ppu->ppu_ctrl) ? 0x1000 : 0;
	uint16_t addr = pattern_table + ppu->next_tile.tile_id * 0x10;
	return addr + loopy_get_fine_y(ppu->vram_reg.curr_addr);
}

static void
fetch_lo_tile(r2C02 *ppu)
{
	uint16_t addr = bg_tile_addr(ppu);
	ppu->next_tile.tile_lo = vram_data_read(ppu, addr);
}

static void
fetch_hi_tile(r2C02 *ppu)
{
	uint16_t addr = bg_tile_addr(ppu);
	ppu->next_tile.tile_hi = vram_data_read(ppu, addr + 8);
}

uint8_t
ppu_get_frame_ready_flag(r2C02 *ppu)
{
	return ppu->frame_ready_flag;
}

void
ppu_unset_frame_ready_flag(r2C02 *ppu)
{
	ppu->frame_ready_flag = 0;
}

void
ppu_reset(r2C02 *ppu, struct bus *bus)
{
	ppu->bus = bus;
}

/* TODO: only for debug */
static void
disasm(r2C02 *ppu)
{
	fprintf(stderr, "total: %zu. x: %d. y: %d. ", ppu->total_cycles, ppu->cycle, ppu->scanline);
	fprintf(stderr, "ctrl: %02x. mask: %02x, status: %02x, v: %04x, t: %04x, fx: %d\n",
		ppu->ppu_ctrl,
		ppu->ppu_mask,
		ppu->ppu_status,
		ppu->vram_reg.curr_addr.whole,
		ppu->vram_reg.tmp_addr.whole,
		ppu->vram_reg.fine_x_scroll
	);
}

static void
update_counters(r2C02 *ppu)
{
	ppu->total_cycles++; /* TODO: debug only */
	ppu->cycle++;

	/* TODO: check odd frame? */

	if (ppu->cycle == 341) {
		ppu->cycle = 0;
		ppu->scanline++;

		if (ppu->scanline == 261) {
			ppu->scanline = -1;
			ppu->frame++;
		}
	}
}

static void
sprite_pipeline(r2C02 *ppu)
{
	/* NOTE: sprite evaluation
	 * See: https://www.nesdev.org/wiki/PPU_sprite_evaluation
	 * cycle 1-64:      clear sprites                   (use cycle == 1)
	 * cycle 65-256:    evaluate sprites                (use cycle == 65)
	 * cycle 257-320:   fetch sprites                   (use cycle == 257)
	 * cycle 321-340+0: background render pipeline init (use cycle == 321)
	 */
	
	switch (ppu->cycle) {
		case 1:
			clear_sprites(ppu);
			break;
		case 65:
			evaluate_sprites(ppu);
			break;
		case 257:
			fetch_sprites(ppu);
			break;
	}
}

static void
background_pipeline(r2C02 *ppu, int visible_pixel, int prerender_pixel)
{
	int in_fetch_range = visible_pixel || prerender_pixel;
	int in_shift_range = in_range_inc(ppu->cycle, 2, 257) || in_range_inc(ppu->cycle, 322, 337);
	int new_pixel_group = (ppu->cycle % 8 == 1);

	if (in_fetch_range) {
		switch (ppu->cycle % 8) {
			case 0:
				update_x_scroll(ppu);
				break;
			case 1:
				fetch_tile_id(ppu);
				break;
			case 3:
				fetch_attr_table(ppu);
				break;
			case 5:
				fetch_lo_tile(ppu);
				break;
			case 7:
				fetch_hi_tile(ppu);
				break;
		}
	}

	if (in_shift_range) {
		update_shift(ppu);
		
		if (new_pixel_group) {
		  load_next_tile(ppu);
		}
	}

	if (ppu->cycle == 257) {
		/* Copy X: v: ....F.. ...EDCBA = t: ....F.. ...EDCBA */
		loopy_upd_from_tmp_coarse_x(&ppu->vram_reg);
		loopy_upd_from_tmp_nametable_x(&ppu->vram_reg);
	}
}

void
ppu_tick(r2C02 *ppu)
{
	int visible_scanline, visible_pixel;
	int prerender_scanline, prerender_pixel;
	int enter_vblank, exit_vblank;
	int rendering_enabled;

	//disasm(ppu);
	update_counters(ppu);

	/* See: https://www.nesdev.org/wiki/PPU_rendering */
	visible_scanline = in_range_inc(ppu->scanline, 0, 239);
	visible_pixel = in_range_inc(ppu->cycle, 1, 256);

	prerender_pixel = in_range_inc(ppu->cycle, 321, 336);
	prerender_scanline = ppu->scanline == -1;
	rendering_enabled = is_rendering_enabled(ppu->ppu_mask);

	enter_vblank = ppu->scanline == 241 && ppu->cycle == 1;
	exit_vblank = prerender_scanline && ppu->cycle == 1;

	if (rendering_enabled) {
		sprite_pipeline(ppu);

		if (visible_scanline || prerender_scanline) {
			background_pipeline(ppu, visible_pixel, prerender_pixel);

			if (ppu->cycle == 256) {
				update_y_scroll(ppu);
			}
		}
	}

	if (visible_scanline && visible_pixel) {
		render_pixel(ppu);
	}

	if (enter_vblank) {
		vblank_start(ppu);
	}
	
	if (exit_vblank) {
		vblank_end(ppu);
	}

	if (prerender_scanline) {
		if (rendering_enabled && in_range_inc(ppu->cycle, 280, 304)) {
			loopy_upd_from_tmp_coarse_y(&ppu->vram_reg);
			loopy_upd_from_tmp_fine_y(&ppu->vram_reg);
			loopy_upd_from_tmp_nametable_y(&ppu->vram_reg);
		}
	}
}

uint8_t
ppu_read(r2C02 *ppu, uint16_t addr)
{
	switch (addr) {
		case PPUSTATUS:
			return ppustatus_read(ppu);
		case OAMDATA:
			return ppu->oam.bytes[ppu->oam_addr];
		case PPUDATA:
			return ppudata_read(ppu);
	}

	return 0; /* TODO: handle addr >= VRAM_SIZE ? */
}

void
ppu_write(r2C02 *ppu, uint16_t addr, uint8_t val)
{
	switch (addr) {
		case PPUCTRL:
			if (!is_nmi_enabled(ppu->ppu_ctrl) && is_vblank_enabled(ppu->ppu_status)) {
				bus_cpu_trigger_nmi(ppu->bus);
			}

			ppu->ppu_ctrl = val;
			loopy_set_nametable_x(&ppu->vram_reg.tmp_addr, val & 0x1);
			loopy_set_nametable_y(&ppu->vram_reg.tmp_addr, (val & 0x2) >> 1);
			break;
		case PPUMASK:
			ppu->ppu_mask = val;
			break;
		case OAMADDR:
			ppu->oam_addr = val;
			break;
		case OAMDATA:
			ppu->oam.bytes[ppu->oam_addr] = val;
			ppu->oam_addr++;
			break;
		case PPUSCROLL:
			scroll_reg_write(ppu, val);
			break;
		case PPUADDR:
			vram_reg_write(ppu, val);
			break;
		case PPUDATA:
			vram_data_write(ppu, vram_addr_read(ppu), val); /* TODO: move vram_addr_read into vram_data_write */
			break;
		case OAMDMA:
			oam_dma_write(ppu, val);
			break;
	}

	/* TODO: handle addr >= VRAM_SIZE ? */
}
