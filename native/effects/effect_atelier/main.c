#include "raylib.h"
#include "effect_atelier.h"

int main(void) {
    const int screenW = 800, screenH = 600;
    InitWindow(screenW, screenH, "Element Burst — click to trigger");
    SetTargetFPS(60);

    RenderTexture2D scene = LoadRenderTexture(screenW, screenH);
    EffectAtelierEffect_Init();

    while (!WindowShouldClose()) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_SPACE))
            EffectAtelierEffect_Trigger();

        EffectAtelierEffect_Update(GetFrameTime());

        BeginDrawing();
            ClearBackground(BLACK);
            EffectAtelierEffect_Draw(scene, screenW, screenH);
            DrawText("Click or press SPACE to trigger the effect", 16, 16, 20, RAYWHITE);
        EndDrawing();
    }

    EffectAtelierEffect_Unload();
    UnloadRenderTexture(scene);
    CloseWindow();
    return 0;
}
