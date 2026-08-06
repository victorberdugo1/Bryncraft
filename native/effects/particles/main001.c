// main001.c — minimal standalone example: Particle System 
//
// gcc main001.c -o particles_demo.exe -I. -L. -lraylib -lgdi32 -lwinmm   (Windows)
// gcc main001.c -o particles_demo -I. -L. -lraylib -lm -lpthread -ldl -lrt -lX11  (Linux)
//
// Part of Bryncraft (https://bryncraft.online/) — created by Victor Berdugo

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

        BeginTextureMode(scene);
            ClearBackground(DARKGRAY);
            /* PLACEHOLDER: draw your scene content here */
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
