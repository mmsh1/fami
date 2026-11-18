#ifndef NES_MAPPER1_H
#define NES_MAPPER1_H

#include <stdint.h>

typedef struct {
} mapper_mmc1;

void mapper1_init(mapper_mmc1 *);
uint8_t mapper1_read(const mapper_mmc1 *, uint16_t);
void mapper1_write(mapper_mmc1 *, uint16_t, uint8_t);

#endif /* NES_MAPPER1_H */
