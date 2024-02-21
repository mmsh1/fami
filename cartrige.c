#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "cartrige.h"
#include "ines.h"

static long int
get_romsize(FILE *rom)
{
	int64_t size;

	fseek(rom, 0, SEEK_END);
	size = ftell(rom);
	fseek(rom, 0, SEEK_SET);
	return size;
}

cartrige
cartrige_create(const char *path)
{
	FILE *rom = NULL;
	int64_t size;

	rom = fopen(path, "r");
	if (rom == NULL) {
		fprintf(stderr, "ROM NOT OPENED!\n");
		exit(1); /* TODO don't exit here! create an error field in cartrige struct */
	}

	/* NOTE now we believe that rom size will fit in our buffer */
	/* TODO rewrite it! */
	size = get_romsize(rom);
	fprintf(stderr, "romsize: %lu\n", size);
	fclose(rom);

	/* TODO */
	/* extract contents of rom according to ines layout */
	return (cartrige){};
}
