#ifndef NES_MAPPER0_H
#define NES_MAPPER0_H

#include <stdint.h>

typedef struct {
	uint8_t *prg;
	uint8_t *chr;
	uint8_t *prg_ram;
	uint8_t prg_size;
	uint8_t chr_size;
} mapper_nrom;

void mapper0_init(mapper_nrom *, uint8_t *, uint8_t *, uint8_t *, uint8_t, uint8_t);
uint8_t mapper0_read(const mapper_nrom *, uint16_t);
void mapper0_write(mapper_nrom *, uint16_t, uint8_t);

#endif /* NES_MAPPER0_H */
