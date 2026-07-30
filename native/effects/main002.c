// main002.c — minimal standalone example: CRT
// Only native/effects/crt_effect.h is needed (plus raylib.h).
//
// gcc -o crt_demo.exe main002.c -I. -I./native/effects -L. -lraylib -lgdi32 -lwinmm

#include "raylib.h"
#include "crt_effect.h"

int main(void)
{
    const int screenW = 800;
    const int screenH = 600;

    InitWindow(screenW, screenH, "CRT — minimal example");
    SetTargetFPS(60);

    RenderTexture2D scene = LoadRenderTexture(screenW, screenH);
    CrtEffect_Init(); // compiles the embedded shader — call once, after InitWindow

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();

        BeginTextureMode(scene);
            ClearBackground(DARKGRAY);
            DrawCircle(screenW / 2, screenH / 2, 120, RED);
        EndTextureMode();

        CrtEffect_Update(dt);

        BeginDrawing();
            ClearBackground(BLACK);
            CrtEffect_Draw(scene, screenW, screenH);
        EndDrawing();
    }

    CrtEffect_Unload();
    UnloadRenderTexture(scene);
    CloseWindow();
    return 0;
}
