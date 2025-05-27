#include "raylib.h"

#include "gfx.h"

RenderTexture2D viewport;

void
gfx_destroy()
{
	CloseWindow();
}

void
gfx_draw_frame(const uint32_t *frame_buf)
{
	BeginTextureMode(viewport);
		ClearBackground(BLACK);
		UpdateTexture(viewport.texture, frame_buf);
	EndTextureMode();

	BeginDrawing();
		DrawTexture(viewport.texture, 0, 0, WHITE);
	EndDrawing();
}

void
gfx_init()
{
	InitWindow(256, 240, "");
	viewport = LoadRenderTexture(256, 240);
	SetTextureFilter(viewport.texture, TEXTURE_FILTER_POINT);
}

int
gfx_should_exit()
{
	return WindowShouldClose();
}
