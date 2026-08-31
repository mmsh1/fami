#ifndef NES_CONTROLLER_H
#define NES_CONTROLLER_H

#include <stdint.h>

enum {
	BUTTON_A = 0x01,
	BUTTON_B = 0x02,
	BUTTON_SELECT = 0x04,
	BUTTON_START = 0x08,
	BUTTON_UP = 0x10,
	BUTTON_DOWN = 0x20,
	BUTTON_LEFT = 0x40,
	BUTTON_RIGHT = 0x80
};

typedef struct {
	uint8_t buttons;
	uint8_t shift;
	uint8_t latch;
} controller;

uint8_t controller_read(controller *);
void controller_write(controller *, uint8_t);

#endif /* NES_CONTROLLER_H */
