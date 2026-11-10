#ifndef NES_AUDIO_H
#define NES_AUDIO_H

#include <stdint.h>

void audio_destroy(void);
void audio_init(uint32_t sample_rate);
void audio_write(const int16_t *, uint64_t);

#endif /* NES_AUDIO_H */
