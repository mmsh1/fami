#include <stddef.h>
#include <stdint.h>

#include "mapper0.h"

static inline uint16_t
get_addr_offset(uint8_t prg_size)
{
	if (prg_size > 1) {
		return 0x7FFF;
	}
	return 0x3FFF;
}

void
mapper0_init(mapper_nrom *mapper, uint8_t *prg, uint8_t *chr, uint8_t *prg_ram,
	uint8_t prg_size, uint8_t chr_size)
{
	mapper->prg = prg;
	mapper->chr = chr;
	mapper->prg_ram = prg_ram;
	mapper->prg_size = prg_size;
	mapper->chr_size = chr_size;
}

uint8_t
mapper0_read(const mapper_nrom *mapper, uint16_t addr)
{
	if (addr <= 0x1FFF) {
		return mapper->chr[addr];
	}
	if (addr >= 0x6000 && addr < 0x8000) {
		if (mapper->prg_ram != NULL) {
			return mapper->prg_ram[addr - 0x6000];
		}
		return 0;
	}
	if (addr >= 0x8000) {
		addr &= get_addr_offset(mapper->prg_size);
		return mapper->prg[addr];
	}
	return 0;
}

void
mapper0_write(mapper_nrom *mapper, uint16_t addr, uint8_t val)
{
	if (addr >= 0x6000 && addr < 0x8000) {
		if (mapper->prg_ram != NULL) {
			mapper->prg_ram[addr - 0x6000] = val;
		}
	}
	if (addr >= 0x8000) {
		return;
	}
}
