#include "controller.h"

static inline uint8_t
get_button_state(uint8_t reg, uint8_t mask)
{
	return (reg & mask) ? 1 : 0;
}

static uint8_t
controller_output(const controller *c)
{
	if (c->latch) {
		return 0x40 | get_button_state(c->buttons, BUTTON_A);
	}

	switch (c->shift) {
		case 0: return 0x40 | get_button_state(c->buttons, BUTTON_A);
		case 1: return 0x40 | get_button_state(c->buttons, BUTTON_B);
		case 2: return 0x40 | get_button_state(c->buttons, BUTTON_SELECT);
		case 3: return 0x40 | get_button_state(c->buttons, BUTTON_START);
		case 4: return 0x40 | get_button_state(c->buttons, BUTTON_UP);
		case 5: return 0x40 | get_button_state(c->buttons, BUTTON_DOWN);
		case 6: return 0x40 | get_button_state(c->buttons, BUTTON_LEFT);
		case 7: return 0x40 | get_button_state(c->buttons, BUTTON_RIGHT);
	}

	return 0x41;
}

uint8_t
controller_read(controller *c, uint64_t cycle)
{
	uint8_t result = controller_output(c);
	int consecutive = 0;

	if (c->last_read_valid) {
		consecutive = cycle == c->last_read_cycle || cycle == c->last_read_cycle + 1;
	}

	c->last_read_cycle = cycle;
	c->last_read_valid = 1;

	if (c->latch || consecutive) {
		return result;
	}

	if (c->shift < 8) {
		c->shift++;
	}

	return result;
}

void
controller_write(controller *c, uint8_t val)
{
	c->latch = val & 1;

	if (c->latch) {
		c->shift = 0;
	}
}

