#include "controller.h"

static inline uint8_t get_button_state(uint8_t reg, uint8_t mask) { return (reg & mask) ? 1 : 0; }
static inline uint8_t get_button_state_A(uint8_t reg)             { return get_button_state(reg, BUTTON_A); }
static inline uint8_t get_button_state_B(uint8_t reg)             { return get_button_state(reg, BUTTON_B); }
static inline uint8_t get_button_state_SELECT(uint8_t reg)        { return get_button_state(reg, BUTTON_SELECT); }
static inline uint8_t get_button_state_START(uint8_t reg)         { return get_button_state(reg, BUTTON_START); }
static inline uint8_t get_button_state_UP(uint8_t reg)            { return get_button_state(reg, BUTTON_UP); }
static inline uint8_t get_button_state_DOWN(uint8_t reg)          { return get_button_state(reg, BUTTON_DOWN); }
static inline uint8_t get_button_state_LEFT(uint8_t reg)          { return get_button_state(reg, BUTTON_LEFT); }
static inline uint8_t get_button_state_RIGHT(uint8_t reg)         { return get_button_state(reg, BUTTON_RIGHT); }

uint8_t
controller_read(controller *c)
{
	if (c->latch) {
		return 0x40 | get_button_state_A(c->buttons);
	}

	switch (c->shift++) {
		case 0: return 0x40 | get_button_state_A(c->buttons);
		case 1: return 0x40 | get_button_state_B(c->buttons);
		case 2: return 0x40 | get_button_state_SELECT(c->buttons);
		case 3: return 0x40 | get_button_state_START(c->buttons);
		case 4: return 0x40 | get_button_state_UP(c->buttons);
		case 5: return 0x40 | get_button_state_DOWN(c->buttons);
		case 6: return 0x40 | get_button_state_LEFT(c->buttons);
		case 7: return 0x40 | get_button_state_RIGHT(c->buttons);
	}

	return 0x41;
}

void
controller_write(controller *c, uint8_t val)
{
	c->latch = val;

	if (c->latch) {
		c->shift = 0;
	}
}

