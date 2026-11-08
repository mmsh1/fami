#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "cartrige.h"
#include "ines.h"

enum {
	PRG_ROM_BANK_SIZE = 0x4000,
	CHR_ROM_BANK_SIZE = 0x2000,
	CHR_RAM_BANK_SIZE = 0x2000,
};

static inline mirroring_type
get_mirroring_type(uint8_t ctl)
{
	mirroring_type mt = ctl & 0x1;

	switch (mt) {
		case 0x0:
			return HORIZONTAL_MIRRORING;
		case 0x1:
			return VERTICAL_MIRRORING;
		case 0x4:
			return 0;
		/* TODO: */
		default:
			return INVALID_MIRRORING;
	}
}

static inline uint16_t
get_addr_offset(uint8_t prg_size)
{
	if (prg_size > 1) {
		return 0x7FFF;
	}
	return 0x3FFF;
}

static uint8_t*
load_block(size_t size)
{
	return calloc(1, size);
}

static uint8_t*
load_block_from_file(FILE *rom, size_t size)
{
	uint8_t *block = load_block(size);
	if (block == NULL) {
		return NULL;
	}

	if (fread(block, 1, size, rom) != size) {
		free(block);
		return NULL;
	}

	return block;
}

static int
cartrige_load_payload(FILE *rom, cartrige *c, const struct ines_header *h)
{
	size_t prg_bytes = (size_t)h->prg_rom_size * PRG_ROM_BANK_SIZE;
	size_t chr_bytes = h->chr_rom_size ? (size_t)h->chr_rom_size * CHR_ROM_BANK_SIZE : CHR_RAM_BANK_SIZE;

	c->prg = NULL;
	c->prg_ram = NULL;
	c->chr = NULL;

	c->prg = load_block_from_file(rom, prg_bytes);
	if (c->prg == NULL) {
		fprintf(stderr, "ERROR: PRG ALLOCATION FAILED!\n");
		goto read_err;
	}

	c->prg_ram = load_block(0x2000);
	if (c->prg_ram == NULL) {
		fprintf(stderr, "ERROR: PRG_RAM ALLOCATION FAILED!\n");
		goto read_err;
	}

	if (h->chr_rom_size != 0) {
		c->chr = load_block_from_file(rom, chr_bytes);
	} else {
		c->chr = load_block(chr_bytes);
	}

	if (c->chr == NULL) {
		fprintf(stderr, "ERROR: CHR ALLOCATION FAILED!\n");
		goto read_err;
	}

	return 0;

read_err:
	free(c->chr);
	free(c->prg_ram);
	free(c->prg);

	c->prg = NULL;
	c->prg_ram = NULL;
	c->chr = NULL;

	return -1;
}

cartrige
cartrige_create(const char *path)
{
	cartrige c = { .invalid = 1 };
	struct ines_header header;
	FILE *rom = fopen(path, "rb");

	if (rom == NULL) {
		fprintf(stderr, "ERROR: ROM NOT OPENED!\n");
		return c;
	}

	if (fread(&header, 1, sizeof(struct ines_header), rom) != sizeof(struct ines_header) || !is_valid_ines_tag(header.magic)) {
		goto done;
	}

	c.mirroring = get_mirroring_type(header.flags6);
	if (c.mirroring == INVALID_MIRRORING) {
		goto done;
	}

	if (cartrige_load_payload(rom, &c, &header) != 0) {
		c.invalid = 1;
		goto done;
	}

	c.prg_size = header.prg_rom_size;
	c.chr_size = header.chr_rom_size;
	c.mapper = (header.flags6 >> 4) | (header.flags7 & 0xF0);
	c.invalid = 0;

done:
	fclose(rom);
	return c;
}

mirroring_type
cartrige_get_mirroring(const cartrige *c)
{
	return c->mirroring;
}

uint8_t
cartrige_read(const cartrige *c, uint16_t addr)
{
	/* TODO: this is implementation only for mapper 0.
	* Other mappers will have different implementation
	* */
	if (addr <= 0x1FFF) {
		return c->chr[addr];
	}

	if (addr >= 0x6000 && addr < 0x8000) {
		if (c->prg_ram != NULL) {
			return c->prg_ram[addr - 0x6000];
		}
		return 0;
	}

	if (addr >= 0x8000) {
		addr &= get_addr_offset(c->prg_size);
		return c->prg[addr];
	}

	return 0;
}

void
cartrige_write(cartrige *c, uint16_t addr, uint8_t val)
{
	/* TODO: this is implementation only for mapper 0.
	* Other mappers will have different implementation
	* */
	if (addr >= 0x6000 && addr < 0x8000) {
		if (c->prg_ram != NULL) {
			c->prg_ram[addr - 0x6000] = val;
		}
	}
	if (addr >= 0x8000) {
		return;
	}
}

void
cartrige_free(cartrige *c)
{
	free(c->prg);
	free(c->chr);
	free(c->prg_ram);
}
