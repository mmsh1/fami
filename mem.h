#ifndef NES_RAM_H
#define NES_RAM_H

#include <stdint.h>

#define RAM_SIZE 0x10000

uint8_t RAM[RAM_SIZE];

void mem_reset(uint8_t *);

#endif /* NES_RAM_H */
