// main001.c — minimal standalone example: Particle System
// Self-contained folder: particles_effect.h + main001.c. raylib.h /
// libraylib.a are shared one level up, in native/effects/.
//
// gcc -o particles_demo.exe main001.c -I.. -L.. -lraylib -lgdi32 -lwinmm

#include "raylib.h"
#include "particles_effect.h"

int main(void)
{
    const int screenW = 800;
    const int screenH = 600;

    InitWindow(screenW, screenH, "Particle System — minimal example");
    SetTargetFPS(60);

    RenderTexture2D scene = LoadRenderTexture(screenW, screenH);

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();

        // Anything can be drawn into 'scene' — the particles react to it
        // when PART_g_params.reactive is on (see the header above).
        BeginTextureMode(scene);
            ClearBackground(DARKGRAY);
            DrawCircle(screenW / 2, screenH / 2, 120, RED);
        EndTextureMode();

        ParticlesEffect_Update(dt);

        BeginDrawing();
            ClearBackground(BLACK);
            ParticlesEffect_Draw(scene, screenW, screenH);
        EndDrawing();
    }

    UnloadRenderTexture(scene);
    CloseWindow();
    return 0;
}
