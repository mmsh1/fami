#ifndef NES_GFX_H
#define NES_GFX_H

#include <stdint.h>

void gfx_destroy();
void gfx_draw_frame(const uint32_t *);
void gfx_init();
int gfx_should_exit();
void gfx_poll_controller(uint8_t *);

#endif /* NES_GFX_H */
