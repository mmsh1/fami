#ifndef NES_MEM_H
#define NES_MEM_H

#include <stdint.h>

#define RAM_SIZE 0x10000 /* TODO do we need it here? */

void mem_reset(uint8_t *);

#endif /* NES_MEM_H */
