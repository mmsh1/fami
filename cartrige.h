#ifndef NES_CARTRIGE_H
#define NES_CARTRIGE_H

#include <stdint.h>

typedef enum {
	HORIZONTAL_MIRRORING,
	VERTICAL_MIRRORING,
	SINGLE_SCREEN_A,
	SINGLE_SCREEN_B,
	FOUR_SCREEN,
	INVALID_MIRRORING
} mirroring_type;

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
uint8_t cartrige_read(cartrige *, uint16_t);

#endif /* NES_CARTRIGE_H */
