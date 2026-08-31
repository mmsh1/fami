#include "raylib.h"

#include "controller.h"
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
		//DrawTexture(viewport.texture, 0, 0, WHITE);
		DrawTexturePro(viewport.texture,
			(Rectangle){ 0, 0, (float)viewport.texture.width, (float)viewport.texture.height },
			//(Rectangle){ 0, 0, 256 * 2, 240 * 2 },
			(Rectangle){ 0, 0, 256 * 3, 240 * 3 },
			(Vector2){ 0, 0 },
			0.0f,
			WHITE
		);
	EndDrawing();
}

void
gfx_init()
{
	SetTraceLogLevel(LOG_NONE);
	//InitWindow(256, 240, "");
	//viewport = LoadRenderTexture(256, 240);
	//InitWindow(256 * 2, 240 * 2, "");
	InitWindow(256 * 3, 240 * 3, "");
	viewport = LoadRenderTexture(256, 240);
	SetTextureFilter(viewport.texture, TEXTURE_FILTER_POINT);
}

int
gfx_should_exit()
{
	return WindowShouldClose();
}

void
gfx_poll_controller(uint8_t *reg)
{
	*reg = 0;

	if (IsKeyDown(KEY_Z))     *reg |= BUTTON_A;
	if (IsKeyDown(KEY_X))     *reg |= BUTTON_B;
	if (IsKeyDown(KEY_A))     *reg |= BUTTON_SELECT;
	if (IsKeyDown(KEY_S))     *reg |= BUTTON_START;
	if (IsKeyDown(KEY_UP))    *reg |= BUTTON_UP;
	if (IsKeyDown(KEY_DOWN))  *reg |= BUTTON_DOWN;
	if (IsKeyDown(KEY_LEFT))  *reg |= BUTTON_LEFT;
	if (IsKeyDown(KEY_RIGHT)) *reg |= BUTTON_RIGHT;
}
