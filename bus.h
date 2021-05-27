#ifndef NES_BUS_H
#define NES_BUS_H

#include <stdint.h>

void bus_reset(uint8_t *);
uint8_t read_from_bus(uint16_t);
void write_to_bus(uint16_t, uint8_t);

#endif /* NES_BUS_H */
