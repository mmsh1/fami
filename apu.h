#ifndef NES_APU_H
#define NES_APU_H

#include <stdint.h>

enum {
	APU_AUDIO_BUFFER_SIZE = 2048,
	APU_SAMPLE_RATE = 48000
};

struct bus;

typedef struct {
	uint8_t enabled;
	uint8_t halt;
	uint8_t value;
	uint8_t reload_inhibit;
} apu_length_counter;

typedef struct {
	uint8_t volume;
	uint8_t constant_volume;

	uint8_t divider;
	uint8_t decay;
	uint8_t start;
} apu_envelope;

typedef struct {
	uint8_t enabled;
	uint8_t period;
	uint8_t negate;
	uint8_t shift;

	uint8_t divider;
	uint8_t reload;
} apu_sweep;

typedef struct {
	apu_length_counter length;
	apu_envelope envelope;
	apu_sweep sweep;
	
	uint16_t period;
	uint16_t timer;
	
	uint8_t duty;
	uint8_t step;
} apu_pulse;

typedef struct {
	apu_length_counter length;
	
	uint16_t period;
	uint16_t timer;
	uint8_t step;
	
	uint8_t linear_value;
	uint8_t linear_reload_value;
	uint8_t linear_reload;
} apu_triangle;

typedef struct {
	apu_length_counter length;
	apu_envelope envelope;
	
	uint16_t period;
	uint16_t timer;
	uint16_t lfsr;
	
	uint8_t mode;
} apu_noise;

typedef struct {
	apu_pulse pulse[2];
	apu_triangle triangle;
	apu_noise noise;

	uint64_t cycle;

	uint32_t frame_cycle;
	uint8_t five_step_mode;
	uint8_t frame_irq_inhibit;
	uint8_t frame_irq;

	uint8_t frame_write_pending;
	uint8_t frame_write_delay;
	uint8_t frame_write_value;

	uint32_t sample_fraction;

	int16_t audio_buffer[APU_AUDIO_BUFFER_SIZE];
	uint64_t audio_buffer_length;

	struct bus *bus;
} apu;

void apu_reset(apu *, struct bus *);
void apu_tick(apu *);
uint8_t apu_read(apu *, uint16_t);
void apu_write(apu *, uint16_t, uint8_t);

#endif /* NES_APU_H */
