#ifndef NES_CARTRIDGE_H
#define NES_CARTRIDGE_H

#include <stdint.h>

#include "ines.h"
#include "mapper0.h"
#include "mapper1.h"

typedef struct {
	uint8_t *prg; /* code section */
	uint8_t *chr; /* graphics section */
	uint8_t *prg_ram;
	uint8_t prg_size;
	uint8_t chr_size;
	mirroring_type mirroring;
	int mapper_id;
	int invalid;
	
	union {
		mapper_nrom m0;
		mapper_mmc1 m1;
		//mapper_uxrom m2;
		//mapper_cnrom m3;
		//mapper_mmc3 m4;
		//mapper_axrom m5;
		//mapper_mmc2 m6;
	} mapper_state;
} cartridge;

cartridge cartridge_create(const char *);
void cartridge_free(cartridge *);
mirroring_type cartridge_get_mirroring(const cartridge *);
uint8_t cartridge_read(const cartridge *, uint16_t);
void cartridge_write(cartridge *, uint16_t, uint8_t);

#endif /* NES_CARTRIDGE_H */
