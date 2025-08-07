#include <string.h>	/* memset */

#include "mem.h"

/* TODO: do we really need it? */
void
mem_reset(uint8_t *mem)
{
	memset(mem, 0, RAM_SIZE);
}
