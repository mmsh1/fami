#ifndef NES_CARTRIGE_H
#define NES_CARTRIGE_H

#include <stdint.h>

#include "ines.h"

typedef struct {
	uint8_t *prg; /* code section */
	uint8_t *chr; /* graphics section */
	uint8_t prg_size;
	uint8_t chr_size;
	mirroring_type mirroring;
	int mapper;
	int invalid;
} cartrige;

cartrige cartrige_create(const char *);
void cartrige_free(cartrige *);
uint8_t cartrige_get_mirroring(const cartrige *);
uint8_t cartrige_read(const cartrige *, uint16_t);

#endif /* NES_CARTRIGE_H */
