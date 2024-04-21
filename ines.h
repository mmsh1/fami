#ifndef NES_INES_H
#define NES_INES_H

#include <stdint.h>

/* see to https://www.nesdev.org/wiki/INES */

// enum mirroring {
// 	HORIZONTAL,
// 	VERTICAL,
// 	SINGLE_SCREEN_A,
// 	SINGLE_SCREEN_B,
// 	FOUR_SCREEN
// };

enum flags6_masks {
	MIRRORING_MASK = 0x01,
	PRG_RAM_MASK = 0x02,
	TRAINER_MASK = 0x04,
	ALT_LAYOUT_MASK = 0x08,
};

struct ines_header {
	uint8_t magic[4];
	uint8_t prg_rom_size; /* number of 16KB ROM banks (PRG ROM) */
	uint8_t chr_rom_size; /* number of 8KB VROM banks (CHR ROM) */
	uint8_t flags6;       /* ROM control byte 1 */
	uint8_t flags7;       /* ROM control byte 2 */
	uint8_t prg_ram_size; /* number of 8KB banks (PRG RAM) */
	uint8_t flags9;
	uint8_t flags10;
	uint8_t padding[5]; /* unused */
};

struct ines {
	struct ines_header header;
	uint8_t trainer[512];
};

int is_valid_ines_tag(const uint8_t *);

#endif /* NES_INES_H */
