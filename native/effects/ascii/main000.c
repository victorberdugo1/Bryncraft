// main000.c — minimal ASCII Renderer example
//
// gcc main000.c -o ascii_demo.exe -I. -L. -lraylib -lgdi32 -lwinmm   (Windows)
// gcc main000.c -o ascii_demo -I. -L. -lraylib -lm -lpthread -ldl -lrt -lX11  (Linux)
//
// Part of Bryncraft (https://bryncraft.online/) — created by Victor Berdugo

#include "raylib.h"
#include "ascii_effect.h"
#include <stdio.h>

int main(void)
{
    const int screenW = 800;
    const int screenH = 600;

    InitWindow(screenW, screenH, "ASCII Renderer — example");
    SetTargetFPS(60);

    const char *fontPath = "NotoSansJP-Kana.ttf";
    FILE *f = fopen(fontPath, "rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        size_t size = ftell(f);
        fseek(f, 0, SEEK_SET);
        uint8_t *buffer = (uint8_t *)malloc(size);
        if (buffer) {
            size_t read = fread(buffer, 1, size, f);
            if (read == size) {
                js_set_matrix_font_data(size, buffer);
            }
			free(buffer);
        }
        fclose(f);
    }

    RenderTexture2D scene = LoadRenderTexture(screenW, screenH);

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();


        /*/ Start scene placeholder. /*/
        BeginTextureMode(scene);
            ClearBackground(DARKGRAY);
            DrawCircle(screenW / 2, screenH / 2, 120, RED);
        EndTextureMode();
		/*/ End scene placeholder. /*/

        AsciiEffect_Update(dt);

        BeginDrawing();
            ClearBackground(BLACK);
            AsciiEffect_Draw(scene, screenW, screenH);
        EndDrawing();
    }

    AsciiEffect_Unload();
    UnloadRenderTexture(scene);
    CloseWindow();
    return 0;
}