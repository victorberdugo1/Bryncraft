// main000.c — minimal standalone example: ASCII Renderer
// Only native/effects/ascii_effect.h is needed (plus raylib.h).
//
// gcc -o ascii_demo.exe main000.c -I. -I./native/effects -L. -lraylib -lgdi32 -lwinmm

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

        // Anything can be drawn into 'scene' — this is just a placeholder shape.
        BeginTextureMode(scene);
            ClearBackground(DARKGRAY);
            DrawCircle(screenW / 2, screenH / 2, 120, RED);
        EndTextureMode();

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
