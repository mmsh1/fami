#include "ines.h"
#include "ppu.h"

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
	COARSE_X_SCROLL = 0x001F, /* 0000 0000 0001 1111 */
	COARSE_Y_SCROLL = 0x03E0, /* 0000 0011 1110 0000 */
	NAMETABLE_X = 0x0400,     /* 0000 0100 0000 0000 */
	NAMETABLE_Y = 0x0800,     /* 0000 1000 0000 0000 */
	/* NAMETABLE = 0x0C00,       0000 1100 0000 0000 */
	FINE_Y_SCROLL = 0x7000    /* 0111 0000 0000 0000 */
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

static inline int in_range(int num, int lo, int hi) { return (num >= lo) && (num <= hi); };

static inline int is_bg_tile_select_mode_enabled(uint8_t ctrl) { return ctrl & PPUCTRL_BACKGROUND_TILE_SELECT; }
static inline int is_bg_rendering_enabled(uint8_t mask)        { return mask & PPUMASK_BACKGROUND_ENABLE; }
static inline int is_fg_rendering_enabled(uint8_t mask)        { return mask & PPUMASK_SPRITE_ENABLE; }
static inline int is_increment_mode_enabled(uint8_t ctrl)      { return ctrl & PPUCTRL_INCREMENT_MODE; }
static inline int is_nmi_enabled(uint8_t ctrl)                 { return ctrl & PPUCTRL_NMI_ENABLE; }
static inline int is_vblank_enabled(uint8_t status)            { return status & PPUSTATUS_VBLANK_ENABLED; }
static inline int is_rendering_enabled(uint8_t mask)           { return is_fg_rendering_enabled(mask) || is_bg_rendering_enabled(mask); }

static inline uint16_t loopy_get(uint16_t reg, uint16_t mask, uint8_t shift) { return (reg & mask) >> shift; }
static inline uint16_t loopy_get_coarse_x(uint16_t reg)                      { return loopy_get(reg, COARSE_X_SCROLL, 0); }
static inline uint16_t loopy_get_coarse_y(uint16_t reg)                      { return loopy_get(reg, COARSE_Y_SCROLL, 5); }
static inline uint16_t loopy_get_fine_y(uint16_t reg)                        { return loopy_get(reg, FINE_Y_SCROLL, 12); }
static inline uint16_t loopy_get_nametable_x(uint16_t reg)                   { return loopy_get(reg, NAMETABLE_X, 10); }
static inline uint16_t loopy_get_nametable_y(uint16_t reg)                   { return loopy_get(reg, NAMETABLE_Y, 11); }

static inline void loopy_set(uint16_t *reg, uint16_t mask, uint16_t val, uint8_t shift) { *reg = (*reg & ~mask) | ((val << shift) & mask); }
static inline void loopy_set_coarse_x(uint16_t *reg, uint16_t val)                      { loopy_set(reg, COARSE_X_SCROLL, val, 0); }
static inline void loopy_set_coarse_y(uint16_t *reg, uint16_t val)                      { loopy_set(reg, COARSE_Y_SCROLL, val, 5); }
static inline void loopy_set_fine_y(uint16_t *reg, uint16_t val)                        { loopy_set(reg, FINE_Y_SCROLL, val, 12); }
static inline void loopy_set_nametable_x(uint16_t *reg, uint16_t val)                   { loopy_set(reg, NAMETABLE_X, val, 10); }
static inline void loopy_set_nametable_y(uint16_t *reg, uint16_t val)                   { loopy_set(reg, NAMETABLE_Y, val, 11); }

static inline void loopy_toggle_nametable_x(uint16_t *reg) { *reg ^= NAMETABLE_X; }
static inline void loopy_toggle_nametable_y(uint16_t *reg) { *reg ^= NAMETABLE_Y; }

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
multiplex_pixels(uint8_t bg_pixel, uint8_t fg_pixel)
{
	return (fg_pixel == 0) ? bg_pixel : fg_pixel;
	/* TODO: add sprite priority */
}

static uint8_t
nametable_read(r2C02 *ppu, uint16_t addr)
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

	return ppu->vram[addr];
}

static void
nametable_write(r2C02 *ppu, uint16_t addr, uint8_t val)
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

	ppu->vram[addr] = val;
}

static inline uint8_t
palette_read(uint16_t addr)
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

static inline void
palette_write(uint16_t addr, uint8_t val)
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
render_bg_pixel(r2C02 *ppu)
{
	uint8_t bit_hi, bit_lo, color;
	uint8_t pal_hi, pal_lo;
	uint8_t x_scroll = ppu->vram_reg.fine_x_scroll;
	uint16_t palette;
	uint16_t mask = 0x8000 >> ppu->vram_reg.fine_x_scroll;
	
	if (!is_bg_rendering_enabled(ppu->ppu_mask)) {
		return 0;
	}

	if (!(ppu->ppu_mask & PPUMASK_BACKGROUND_LEFT_COL_ENABLE) && ppu->cycle < 9) {
		return 0;
	}

	bit_hi = ((ppu->shift.tile_hi & mask) >> (15 - x_scroll)) & 0x01;
	bit_lo = ((ppu->shift.tile_lo & mask) >> (15 - x_scroll)) & 0x01;
	color = (bit_hi << 1) | bit_lo;

	if (color == 0) {
		return 0;
	}

	pal_hi = ((ppu->shift.attr_hi & mask) >> (15 - x_scroll)) & 0x01;
	pal_lo = ((ppu->shift.attr_lo & mask) >> (15 - x_scroll)) & 0x01;
	palette = (pal_hi << 1) | (pal_lo);

	return palette * 4 + color;
}

static uint8_t
render_fg_pixel(r2C02 *ppu)
{
	/* TODO: */
	return 0x0;
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
		//loopy_set_coarse_x(&ppu->vram_reg.tmp_addr.whole, (val & 0xF8) >> 3);
		loopy_set_coarse_x(&ppu->vram_reg.tmp_addr.whole, val >> 3);
		ppu->vram_reg.fine_x_scroll = val & 0x7;
		ppu->vram_reg.write_flag = 1;
	} else {
		/*
			NOTE:
			t: FGH..AB CDE..... <- d: ABCDEFGH
			w:                  <- 0
		*/
		loopy_set_fine_y(&ppu->vram_reg.tmp_addr.whole, val & 0x7);
		loopy_set_coarse_y(&ppu->vram_reg.tmp_addr.whole, val >> 3);
		ppu->vram_reg.write_flag = 0;
	}
}

static void
vblank_end(r2C02 *ppu)
{
	ppu->ppu_status &= ~PPUSTATUS_VBLANK_ENABLED;
}

static void
vblank_start(r2C02 *ppu)
{
	ppu->ppu_status |= PPUSTATUS_VBLANK_ENABLED;
	ppu->frame_ready_flag = 1;

	if (ppu->ppu_ctrl & PPUCTRL_NMI_ENABLE) {
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
	/*return 0x0; TODO: assert? */
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

static inline void
vram_reg_write(r2C02 *ppu, uint8_t val)
{
	if (ppu->vram_reg.write_flag == 0) {
		ppu->vram_reg.tmp_addr.part.hi = val;
		ppu->vram_reg.tmp_addr.part.lo = 0; /*TODO:?*/
		ppu->vram_reg.write_flag = 1;
	} else {
		ppu->vram_reg.tmp_addr.part.lo = val;
		ppu->vram_reg.curr_addr = ppu->vram_reg.tmp_addr;
		ppu->vram_reg.tmp_addr.whole = 0;
		ppu->vram_reg.write_flag = 0; /*TODO:?*/
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
	int bg_rendering_enabled = is_bg_rendering_enabled(ppu->ppu_mask);
	int fg_rendering_enabled = is_fg_rendering_enabled(ppu->ppu_mask);

	int x = ppu->cycle - 1;
	int y = ppu->scanline;

	uint8_t bg_color = 0;
	uint8_t fg_color = 0;
	uint8_t final_color = 0;
	uint32_t rgb_color = 0;

	if (bg_rendering_enabled) {
		bg_color = render_bg_pixel(ppu);
	}

	if (fg_rendering_enabled) {
		fg_color = render_fg_pixel(ppu);
	}

	final_color = multiplex_pixels(bg_color, fg_color);
	uint16_t color_idx = 0x3F00 + final_color;
	color_idx = vram_data_read(ppu, color_idx);

	rgb_color = nes_palette_to_rgb(color_idx);
	set_pixel(ppu, x, y, rgb_color);
}

static void
clear_sprites(r2C02 *ppu)
{
	int i;
	for (i = 0; i < OAM2_SIZE; i++) {
		ppu->oam2[i] = 0;
	}
}

static void
evaluate_sprites(r2C02 *ppu)
{
	/* TODO: here we should search OAM in order to find
	 * sprites that will be on current scanline. We choose
	 * the first eight.
	 * If eight sprites were found, it checks (in a wrongly-implemented fashion)
	 * for further sprites on the scanline to see if the sprite overflow flag
	 * should be set. Then, using the details for the eight (or fewer) sprites
	 * chosen, it determines which pixels each has on the scanline and where
	 * to draw them.
	 */
}

static void
fetch_sprites(r2C02 *ppu)
{

}

static uint8_t
fetch_attr_table(r2C02 *ppu)
{
	uint16_t addr = ppu->vram_reg.curr_addr.whole;
	/* TODO: rewrite! */
	uint16_t attr_byte_addr = 0x23C0 | (addr & 0x0C00) | ((addr >> 4) & 0x38) | ((addr >> 2) & 0x07);
	uint8_t attr_byte = vram_data_read(ppu, attr_byte_addr);

	uint16_t coarse_x = loopy_get_coarse_x(addr);
	uint16_t coarse_y = loopy_get_coarse_y(addr);
	uint8_t shift = (coarse_y & 0x02) << 1 | coarse_x & 0x02;

	return (attr_byte >> shift) & 0x03;
}

static uint16_t
update_x_scroll(r2C02 *ppu)
{
	uint16_t loopy_reg = ppu->vram_reg.curr_addr.whole;
	uint16_t coarse_x = loopy_get_coarse_x(loopy_reg);

	if (coarse_x == 31) {
		loopy_set_coarse_x(&loopy_reg, 0);
		loopy_toggle_nametable_x(&loopy_reg);
	} else {
		loopy_set_coarse_x(&loopy_reg, coarse_x + 1);
	}

	return loopy_reg;
}

static uint16_t
update_y_scroll(r2C02 *ppu)
{
	uint16_t loopy_reg = ppu->vram_reg.curr_addr.whole;
	uint16_t fine_y = loopy_get_fine_y(loopy_reg);
	uint16_t coarse_y;

	if (fine_y < 7) {
		loopy_set_fine_y(&loopy_reg, fine_y + 1);
	} else {
		coarse_y = loopy_get_coarse_y(loopy_reg);
		loopy_set_fine_y(&loopy_reg, 0);

		switch (coarse_y) {
			case 29:
				loopy_set_coarse_y(&loopy_reg, 0);
				loopy_toggle_nametable_y(&loopy_reg);
				break;
			case 31:
				loopy_set_coarse_y(&loopy_reg, 0);
				break;
			default:
				loopy_set_coarse_y(&loopy_reg, coarse_y + 1);
				break;
		}
	}

	return loopy_reg;
}

static uint8_t
fetch_tile_id(r2C02 *ppu)
{
	uint16_t addr = 0x2000 | (ppu->vram_reg.curr_addr.whole & 0x0FFF);
	return vram_data_read(ppu, addr);
}

static uint8_t
fetch_lo_tile(r2C02 *ppu)
{
	uint16_t pattern_table = is_bg_tile_select_mode_enabled(ppu->ppu_ctrl) ? 0x1000 : 0;
	uint16_t addr = pattern_table + ppu->next_tile.tile_id * 0x10;
	addr += loopy_get_fine_y(ppu->vram_reg.curr_addr.whole);
	return vram_data_read(ppu, addr);
}

static uint8_t
fetch_hi_tile(r2C02 *ppu)
{

	uint16_t pattern_table = is_bg_tile_select_mode_enabled(ppu->ppu_ctrl) ? 0x1000 : 0;
	uint16_t addr = pattern_table + ppu->next_tile.tile_id * 0x10;
	addr += loopy_get_fine_y(ppu->vram_reg.curr_addr.whole);
	return vram_data_read(ppu, addr + 8);
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

	/* TODO: do we need these lines?
	ppu->frame = 0;
	ppu->scanline = 0;
	ppu->cycle = 0;
	ppu->frame_ready_flag = 0;
	*/
}

/* TODO: only for debug */
static void
disasm(r2C02 *ppu)
{
	fprintf(stderr, "x: %d. y: %d. ", ppu->cycle, ppu->scanline);
	fprintf(stderr, "ctrl: %02x. mask: %02x, status: %02x, v: %04x, t: %04x, fx: %d\n",
		ppu->ppu_ctrl,
		ppu->ppu_mask,
		ppu->ppu_status,
		ppu->vram_reg.curr_addr.whole,
		ppu->vram_reg.tmp_addr.whole,
		ppu->vram_reg.fine_x_scroll
	);
}

void
ppu_tick(r2C02 *ppu)
{
	int visible_scanline, visible_pixel;
	int enter_vblank, exit_vblank;
	int prerender_scanline, render_scanline, postrender_scanline;
	int rendering_enabled;

	//disasm(ppu);

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

	/* See: https://www.nesdev.org/wiki/PPU_rendering */
	visible_scanline = in_range(ppu->scanline, 0, 239);
	visible_pixel = in_range(ppu->cycle, 1, 256);

	prerender_scanline = ppu->scanline == -1;
	render_scanline = visible_scanline || prerender_scanline;
	/* postrender_scanline = ppu->scanline == 240; */

	enter_vblank = ppu->scanline == 241 && ppu->cycle == 1;
	exit_vblank = ppu->scanline == 261 && ppu->cycle == 1;

	rendering_enabled = is_rendering_enabled(ppu->ppu_mask);
	//new_pixel_group = ppu->cycle % 8 == 1;

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

	/* TODO: rewrite like fetch conveyor */
	if (rendering_enabled && render_scanline) {
		if (visible_pixel || in_range(ppu->cycle, 321, 336)) {
			switch (ppu->cycle % 8) {
				case 0:
					ppu->vram_reg.curr_addr.whole = update_x_scroll(ppu);
					break;
				case 1:
					ppu->next_tile.tile_id = fetch_tile_id(ppu);
					break;
				case 3:
					ppu->next_tile.attr = fetch_attr_table(ppu);
					break;
				case 5:
					ppu->next_tile.tile_lo = fetch_lo_tile(ppu);
					break;
				case 7:
					ppu->next_tile.tile_hi = fetch_hi_tile(ppu);
					break;
			}
		}

		if (in_range(ppu->cycle, 2, 257) || in_range(ppu->cycle, 322, 337)) {
			update_shift(ppu);
		
			if (ppu->cycle % 8 == 1) {
			  load_next_tile(ppu);
			}
		}

		if (ppu->cycle == 256) {
			ppu->vram_reg.curr_addr.whole = update_y_scroll(ppu);
		}

		if (ppu->cycle == 257) {
			/* TODO: implement update from tmp wrappers:
			loopy_upd_from_tmp_coarse_x(&ppu->vram_reg);
			loopy_upd_from_tmp_nametable_x(&ppu->vram_reg);
			*/

			/* Copy X: v: ....F.. ...EDCBA = t: ....F.. ...EDCBA */
			loopy_set_coarse_x(&ppu->vram_reg.curr_addr.whole, loopy_get_coarse_x(ppu->vram_reg.tmp_addr.whole));
			loopy_set_nametable_x(&ppu->vram_reg.curr_addr.whole, loopy_get_nametable_x(ppu->vram_reg.tmp_addr.whole));
		}
	}

	if (visible_scanline && visible_pixel) {
		render_pixel(ppu);
	}

	if (enter_vblank) {
		vblank_start(ppu);
	}

	if (ppu->scanline == -1) {
		if (ppu->cycle == 1) {
			vblank_end(ppu);
		}

		if (rendering_enabled && in_range(ppu->cycle, 280, 304)) {
			/* TODO: implement update from tmp wrappers:
			loopy_upd_from_tmp_coarse_y(&ppu->vram_reg);
			loopy_upd_from_tmp_fine_y(&ppu->vram_reg);
			loopy_upd_from_tmp_nametable_y(&ppu->vram_reg);
			*/

			loopy_set_coarse_y(&ppu->vram_reg.curr_addr.whole, loopy_get_coarse_y(ppu->vram_reg.tmp_addr.whole));
			loopy_set_fine_y(&ppu->vram_reg.curr_addr.whole, loopy_get_fine_y(ppu->vram_reg.tmp_addr.whole));
			loopy_set_nametable_y(&ppu->vram_reg.curr_addr.whole, loopy_get_nametable_y(ppu->vram_reg.tmp_addr.whole));
		}
	}

}

uint8_t
ppu_read(r2C02 *ppu, uint16_t addr)
{
	uint8_t res = 0;

	switch (addr) {
		case PPUSTATUS:
			res = ppu->ppu_status;
			ppu->ppu_status &= ~PPUSTATUS_VBLANK_ENABLED;
			ppu->vram_reg.write_flag = 0;
			return res;
		case OAMDATA:
			return ppu->oam[ppu->oam_addr];
		case PPUDATA:
			return vram_data_read(ppu, vram_addr_read(ppu)); /* TODO: move vram_addr_read into vram_data_read */
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
			loopy_set_nametable_x(&ppu->vram_reg.tmp_addr.whole, val & 0x1);
			loopy_set_nametable_y(&ppu->vram_reg.tmp_addr.whole, (val & 0x2) >> 1);
			break;
		case PPUMASK:
			ppu->ppu_mask = val;
			break;
		case OAMADDR:
			ppu->oam_addr = val;
			break;
		case OAMDATA:
			ppu->oam[ppu->oam_addr] = val;
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
			/* TODO: */
			break;
	}

	/* TODO: handle addr >= VRAM_SIZE ? */
}
