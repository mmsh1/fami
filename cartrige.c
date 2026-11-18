#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "cartrige.h"
#include "ines.h"
#include "mapper0.h"
#include "mapper1.h"

enum {
	PRG_ROM_BANK_SIZE = 0x4000,
	CHR_ROM_BANK_SIZE = 0x2000,
	CHR_RAM_BANK_SIZE = 0x2000,
};

enum {
	MAPPER_NROM = 0,
	MAPPER_MMC1 = 1,
	MAPPER_UXROM = 2,
	MAPPER_CNROM = 3,
	MAPPER_MMC3 = 4,
	MAPPER_AXROM = 7,
	MAPPER_MMC2 = 9
};

static inline mirroring_type
get_mirroring_type(uint8_t flags6)
{
	if (flags6 & ALT_LAYOUT_MASK) {
		return FOUR_SCREEN;
	}
	if (flags6 & MIRRORING_MASK) {
		return VERTICAL_MIRRORING;
	}
	return HORIZONTAL_MIRRORING;
}

static int
apply_header(cartrige *c, struct ines_header *header)
{
	c->mirroring = get_mirroring_type(header->flags6);
	if (c->mirroring == INVALID_MIRRORING) {
		return -1;
	}
	c->prg_size = header->prg_rom_size;
	c->chr_size = header->chr_rom_size;
	c->mapper_id = (header->flags6 >> 4) | (header->flags7 & 0xF0);
	return 0;
}

static int
init_mapper(cartrige *c)
{
	switch (c->mapper_id) {
	case MAPPER_NROM:
		mapper0_init(&c->mapper_state.m0, c->prg, c->chr, c->prg_ram, c->prg_size, c->chr_size);
		return 0;
	default:
		fprintf(stderr, "ERROR: UNSUPPORTED MAPPER: %d\n", c->mapper_id);
		return -1;
	}
}

static uint8_t*
load_block(uint64_t size)
{
	return calloc(1, size);
}

static uint8_t*
load_block_from_file(FILE *rom, uint64_t size)
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
load_payload(FILE *rom, cartrige *c, const struct ines_header *h)
{
	uint64_t prg_bytes = (uint64_t)h->prg_rom_size * PRG_ROM_BANK_SIZE;
	uint64_t chr_bytes = h->chr_rom_size ? (uint64_t)h->chr_rom_size * CHR_ROM_BANK_SIZE : CHR_RAM_BANK_SIZE;

	c->prg = load_block_from_file(rom, prg_bytes);
	if (c->prg == NULL) {
		fprintf(stderr, "ERROR: PRG ALLOCATION FAILED!\n");
		return -1;
	}

	c->prg_ram = load_block(0x2000);
	if (c->prg_ram == NULL) {
		fprintf(stderr, "ERROR: PRG_RAM ALLOCATION FAILED!\n");
		return -1;
	}

	if (h->chr_rom_size != 0) {
		c->chr = load_block_from_file(rom, chr_bytes);
	} else {
		c->chr = load_block(chr_bytes);
	}

	if (c->chr == NULL) {
		fprintf(stderr, "ERROR: CHR ALLOCATION FAILED!\n");
		return -1;
	}

	return 0;
}

static int
read_ines_header(FILE *rom, struct ines_header *header)
{
	if (fread(header, 1, sizeof(*header), rom) != sizeof(*header)) {
		return -1;
	}
	if (!is_valid_ines_tag(header->magic)) {
		return -1;
	}
	return 0;
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

	if (read_ines_header(rom, &header) != 0) {
		fprintf(stderr, "ERROR: INVALID INES HEADER!\n");
		goto fail;
	}

	if (apply_header(&c, &header) != 0) {
		goto fail;
	}

	if (load_payload(rom, &c, &header) != 0) {
		goto fail;
	}

	if (init_mapper(&c) != 0) {
		goto fail;
	}

	c.invalid = 0;
	fprintf(stderr, "MAPPER: %d\n", c.mapper_id);
	goto done;

fail:
	cartrige_free(&c);

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
	switch (c->mapper_id) {
	case MAPPER_NROM:
		return mapper0_read(&c->mapper_state.m0, addr);
	case MAPPER_MMC1:
	case MAPPER_UXROM:
	case MAPPER_CNROM:
	case MAPPER_MMC3:
	case MAPPER_AXROM:
	case MAPPER_MMC2:
	default:
		fprintf(stderr, "UNIMPLEMENTED MAPPER READ!\n"); /* TODO: remove */
		return 0;
	}
}

void
cartrige_write(cartrige *c, uint16_t addr, uint8_t val)
{
	switch (c->mapper_id) {
	case MAPPER_NROM:
		mapper0_write(&c->mapper_state.m0, addr, val);
		break;
	case MAPPER_MMC1:
	case MAPPER_UXROM:
	case MAPPER_CNROM:
	case MAPPER_MMC3:
	case MAPPER_AXROM:
	case MAPPER_MMC2:
	default:
		fprintf(stderr, "UNIMPLEMENTED MAPPER WRITE!\n"); /* TODO: remove */
		break;
	}
}

void
cartrige_free(cartrige *c)
{
	free(c->prg);
	free(c->chr);
	free(c->prg_ram);

	c->prg = NULL;
	c->chr = NULL;
	c->prg_ram = NULL;
}
