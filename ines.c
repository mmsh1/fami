#include "ines.h"

static const uint8_t
ines_tag[4] = { 0x4E, 0x45, 0x53, 0x1A }; /* N + E + S + 0x1A */

int
is_valid_ines_tag(const uint8_t *tag)
{
	int i;

	for (i = 0; i < 4; i++) {
		if (tag[i] != ines_tag[i]) {
			return 0;
		}
	}
	return 1;
}
