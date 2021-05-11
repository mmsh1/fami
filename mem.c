#include <stdio.h>	/* TODO remove */
#include <string.h>	/* memset */

#include "mem.h"

/*uint8_t RAM[RAM_SIZE];*/

void
mem_reset(uint8_t *mem)
{
	memset(mem, 0, RAM_SIZE);
}
