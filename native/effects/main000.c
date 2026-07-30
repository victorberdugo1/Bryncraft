// main000.c — minimal example: ASCII Renderer
// Only libraylib.a , raylib.h & ascii_effect.h is needed.
//
// gcc main000.c -o ascii_demo.exe -L. -lraylib -lgdi32 -lwinmm
// gcc main000.c -o ascii_demo -L. -lraylib -lm -lpthread -ldl -lrt -lX11

#include "raylib.h"
#include "ascii_effect.h"

int main(void)
{
    const int screenW = 800;
    const int screenH = 600;

    InitWindow(screenW, screenH, "ASCII Renderer — minimal example");
    SetTargetFPS(60);

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
