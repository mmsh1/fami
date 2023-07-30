#include <string.h>	/* memset */

/* #include "bus.h" */
#include "mem.h"

void
mem_reset(uint8_t *mem)
{
	memset(mem, 0, RAM_SIZE);
}
