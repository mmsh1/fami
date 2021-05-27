#include "bus.h"

uint8_t *ram;

void
bus_reset(uint8_t *mem)
{
	ram = mem;
}
