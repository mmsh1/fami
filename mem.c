#include <stdio.h>	/* TODO remove */
#include <string.h>	/* memset */

#include "mem.h"

void
mem_reset(uint8_t *mem)
{
	memset(mem, 0, RAM_SIZE);
}

int
main(void)
{
	/* mock for building */
	fprintf(stdout, "INFO:building done\n");
	return 0;
}
