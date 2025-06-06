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

static mirroring_type
get_mirroring_type(uint8_t ctl)
{
	uint8_t mirroring = ctl & 0x1;

	switch (mirroring) {
		case 0x0:
			return HORIZONTAL_MIRRORING;
		case 0x1:
			return VERTICAL_MIRRORING;
		case 0x4:
			return 0;
		/* TODO: */
	}

	return INVALID_MIRRORING;
}

static uint16_t
get_addr_offset(const cartrige *c)
{
	if (c->prg_size > 1) {
		return 0x7FFF;
	}
	return 0x3FFF;
}

cartrige
cartrige_create(const char *path)
{
	FILE *rom = NULL;
	struct ines_header header;
	mirroring_type mirroring;
	uint8_t *prg, *chr;

	rom = fopen(path, "rb");
	if (rom == NULL) {
		fprintf(stderr, "ERROR: ROM NOT OPENED!\n"); /* TODO wrap */
		return (cartrige){
			.invalid = 1
		};
	}

	/* TODO */
	/* extract contents of rom according to ines layout */

	/* check ines tag */
	fread(&header, sizeof(uint8_t), 16, rom);
	if (!is_valid_ines_tag(header.magic)) {
		fclose(rom);
		fprintf(stderr, "ROM is not an iNES image.\n");  /* TODO wrap */
		return (cartrige){
			.invalid = 1
		};
	}

	/* get mapper */
	/* check ines version (1.0 / 2.0) */

	/* get mirroring */
	mirroring = get_mirroring_type(header.flags6);
	if (mirroring == INVALID_MIRRORING) {
		fclose(rom);
		fprintf(stderr, "Invalid mirroring type.\n");  /* TODO wrap */
		return (cartrige){
			.invalid = 1
		};
	}

	/* allocate prg */
	prg = calloc(header.prg_rom_size * PRG_ROM_BANK_SIZE, sizeof(uint8_t));
	if (!prg) {
		exit(1);
	}

	/* allocate chr */
	chr = calloc(header.chr_rom_size * CHR_ROM_BANK_SIZE, sizeof(uint8_t));
	if (!chr) {
		free(prg);
		exit(1);
	}

	fread(prg, sizeof(uint8_t), header.prg_rom_size * PRG_ROM_BANK_SIZE, rom);
	fread(chr, sizeof(uint8_t), header.chr_rom_size * CHR_ROM_BANK_SIZE, rom);

	return (cartrige){
		.prg = prg,
		.chr = chr,
		.prg_size = header.prg_rom_size,
		.chr_size = header.chr_rom_size
	};
}

uint8_t
cartrige_get_mirroring(const cartrige *c)
{
	return c->mirroring;
}

uint8_t
cartrige_read(const cartrige *c, uint16_t addr)
{
	if (addr <= 0x1FFF) {
		return c->chr[addr];
	}

	if (addr >= 0x8000) {
		addr &= get_addr_offset(c);
		return c->prg[addr];
	}

	fprintf(stderr, "ERROR: ILLEGAL READ FROM %04X\n", addr);
	return 0; // TODO how can we handle this?
}

void
cartrige_free(cartrige *c)
{
	free(c->prg);
	free(c->chr);
}
