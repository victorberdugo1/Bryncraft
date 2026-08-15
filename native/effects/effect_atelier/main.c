#include "raylib.h"
#include "effect_atelier.h"

int main(void) {
    const int screenW = 800, screenH = 600;
    InitWindow(screenW, screenH, "Element Burst — minimal example");
    SetTargetFPS(60);

    RenderTexture2D scene = LoadRenderTexture(screenW, screenH);
    EffectAtelierEffect_Init();

    while (!WindowShouldClose()) {
        EffectAtelierEffect_Update(GetFrameTime());
        BeginDrawing();
            ClearBackground(BLACK);
            EffectAtelierEffect_Draw(scene, screenW, screenH);
        EndDrawing();
    }

    EffectAtelierEffect_Unload();
    UnloadRenderTexture(scene);
    CloseWindow();
    return 0;
}
