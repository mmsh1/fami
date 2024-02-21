#include <stdint.h>

/* see to https://www.nesdev.org/wiki/INES */

enum mirroring {
	HORIZONTAL,
	VERTICAL
};

enum flags6_masks {
	MIRRORING_MASK = 0x01,
	PRG_RAM_MASK = 0x02,
	TRAINER_MASK = 0x04,
	ALT_LAYOUT_MASK = 0x08,
};

const uint8_t ines_label[4] = { 0x4E, 0x45, 0x53, 0x1A }; /* NES + 0x1A */

struct ines_header {
	uint8_t label[4];
	uint8_t prg_rom_size; /* in 16KB units */
	uint8_t chr_rom_size; /* in 8KB units */
	uint8_t flags6;
	uint8_t flags7;
	uint8_t prg_ram_size;
	uint8_t flags9;
	uint8_t flags10;
	uint8_t padding[5]; /* unused */
};

struct ines {
	struct ines_header header;
	uint8_t trainer[512];
};
