#ifndef NES_APU_H
#define NES_APU_H

#include <stdint.h>

struct bus;

typedef struct {
	uint8_t enabled;
	uint8_t halt;
	uint8_t value;
} apu_length_counter;

typedef struct {
	apu_length_counter pulse[2];
	apu_length_counter triangle;
	apu_length_counter noise;

	uint32_t frame_cycle;
	uint8_t five_step_mode;
	uint8_t frame_irq_inhibit;
	uint8_t frame_irq;

	struct bus *bus;
} apu;

void apu_reset(apu *, struct bus *);
void apu_tick(apu *);
uint8_t apu_read(apu *, uint16_t);
void apu_write(apu *, uint16_t, uint8_t);

#endif /* NES_APU_H */
