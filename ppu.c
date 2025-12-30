#include "ines.h"
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

static inline uint8_t
get_color_idx_in_palette(uint8_t lo, uint8_t hi)
{
	return (lo & 0x1) << 1 | (hi & 0x1); /* from 0 to 3 */
}

static inline int
get_increment_mode(const r2C02 *ppu)
{
	return ppu->ppu_ctrl & PPUCTRL_INCREMENT_MODE;
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

static uint8_t
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

static void
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

static uint8_t
render_bg_pixel(r2C02 *ppu)
{
	uint8_t data;

	if (!(ppu->ppu_mask & PPUMASK_BACKGROUND_ENABLE))  { /* TODO: rewrite */
		return 0;
	}

	return data & 0x0F;
}

static uint8_t
render_fg_pixel(r2C02 *ppu)
{
	/* TODO: */
	return 0x1;
}


static void
vblank_start(r2C02 *ppu)
{
	ppu->ppu_status |= PPUSTATUS_VBLANK_ENABLED;
	ppu->frame_ready_flag = 1;
	bus_cpu_trigger_nmi(ppu->bus);
}

static void
vblank_end(r2C02 *ppu)
{
	ppu->ppu_status &= ~PPUSTATUS_VBLANK_ENABLED;
}

static void
vram_addr_increment(r2C02 *ppu)
{
	uint16_t inc_val = get_increment_mode(ppu) ? 32 : 1;
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

	return 0x0; /* TODO: assert? */
}

static void
vram_data_write(r2C02 *ppu, uint16_t addr, uint8_t val)
{
	if (addr < 0x2000) {
		bus_cartrige_write(ppu->bus, addr, val);
	}

	if (addr < 0x3F00) {
		nametable_write(ppu, addr, val);
	}

	if (addr < 0x4000) {
		palette_write(addr, val);
	}
}

static inline void
vram_reg_write(r2C02 *ppu, uint8_t val)
{
	if (ppu->vram_reg.write_flag == 0) {
		ppu->vram_reg.tmp_addr.part.hi = val;
		ppu->vram_reg.write_flag = 1;
	} else {
		ppu->vram_reg.tmp_addr.part.lo = val;
		ppu->vram_reg.curr_addr.whole = ppu->vram_reg.tmp_addr.whole;
		ppu->vram_reg.write_flag = 0;
	}
}

static inline void
set_pixel(r2C02 *ppu, int x, int y, uint32_t color)
{
	ppu->frame_buf[x + y * 256] = color;
}

/*
static void
debug_draw_tile(r2C02 *ppu, int x, int y, int tile_idx)
{
	uint8_t tile_hi, tile_lo;
	uint8_t color_idx;
	uint32_t bg_color, fg_color;
	int i, j;
	int tile_addr = tile_idx * 0x10; // 0->0, 1->16, 2->32, 3->48, 4->64 etc

	for (i = 0; i <= 8; i++) {
		tile_hi = vram_data_read(ppu, (uint16_t)(tile_addr + i));
		tile_lo = vram_data_read(ppu, (uint16_t)(tile_addr + i + 8));

		for (j = 7; j >= 0; j--) {
			color_idx = get_color_idx_in_palette(tile_lo, tile_hi);
			
			tile_hi /= 2;
			tile_lo /= 2;

			bg_color = ppu_colors[vram_data_read(ppu, 0x3F00)];
			fg_color = ppu_colors[vram_data_read(ppu, 0x3F00 | (color_idx + 6))];

			if (color_idx == 0) {
				fg_color = bg_color;
			}

			set_pixel(ppu, x + j, y + i, fg_color);
		}
	}
}
*/

static inline uint32_t
nes_palette_to_rgb(uint8_t color_idx)
{
	return ppu_colors[color_idx & 0x3F];
}

static void
render_pixel(r2C02 *ppu)
{
	int bg_rendering_enabled = ppu->ppu_mask & PPUMASK_BACKGROUND_ENABLE;
	int fg_rendering_enabled = ppu->ppu_mask & PPUMASK_SPRITE_ENABLE;

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
	rgb_color = nes_palette_to_rgb(final_color);
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
	/* TODO: do we need these lines? */
	ppu->bus = bus;
	ppu->frame = 0;
	ppu->scanline = 0;
	ppu->cycle = 0;
	ppu->frame_ready_flag = 0;
}

void
ppu_tick(r2C02 *ppu)
{
	int visible_scanline, visible_pixel;
	int enter_vblank, exit_vblank;
	/* int prerender_scanline, postrender_scanline; */

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
	visible_scanline = ppu->scanline >= 0 && ppu->scanline <= 239;
	visible_pixel = ppu->cycle >= 1 && ppu->cycle <= 256;

	/* prerender_scanline = ppu->scanline == -1; */
	/* postrender_scanline = ppu->scanline == 240; */

	enter_vblank = ppu->scanline == 241 && ppu->cycle == 1;
	exit_vblank = ppu->scanline == 261 && ppu->cycle == 1;

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

	if (visible_scanline && visible_pixel) {
		render_pixel(ppu);
	}

	if (enter_vblank) {
		vblank_start(ppu);
	}

	if (exit_vblank) {
		vblank_end(ppu);
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
			/* TODO: create NMI */
			ppu->ppu_ctrl = val;
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
			/* TODO: */
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
