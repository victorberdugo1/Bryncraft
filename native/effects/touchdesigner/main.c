#include "raylib.h"
#include "ascii_effect.h"
#include "crt_effect.h"
#include "opencv_effect.h"
#include "touchdesigner_effect.h"

int main(void) {
    const int screenW = 1280, screenH = 720;
    InitWindow(screenW, screenH, "TouchDesigner Hand Portal — mouse demo (L=pinch, R=fist)");
    SetTargetFPS(60);

    RenderTexture2D scene = LoadRenderTexture(screenW, screenH);
    TouchdesignerEffect_Init();
    TD_g_params.autoDetectHands = false;

    while (!WindowShouldClose()) {
        BeginTextureMode(scene);
            ClearBackground((Color){ 10, 12, 16, 255 });
            float t = (float)GetTime();
            for (int i = 0; i < 6; i++) {
                float x = screenW * (0.15f + 0.14f * i);
                float y = screenH * 0.5f + sinf(t * 1.3f + i) * screenH * 0.2f;
                DrawCircle((int)x, (int)y, 40 + 10 * sinf(t + i), (Color){ 40 + i * 30, 80, 160, 255 });
            }
            DrawText("mueve el mouse -- 1-5: estilo de manos -- 6-0: estilo de fondo", 40, 40, 20, RAYWHITE);
        EndTextureMode();

        if (IsKeyPressed(KEY_ONE)) TD_g_params.handStyle = TD_STYLE_NONE;
        if (IsKeyPressed(KEY_TWO)) TD_g_params.handStyle = TD_STYLE_ASCII;
        if (IsKeyPressed(KEY_THREE)) TD_g_params.handStyle = TD_STYLE_MATRIX;
        if (IsKeyPressed(KEY_FOUR)) TD_g_params.handStyle = TD_STYLE_CRT;
        if (IsKeyPressed(KEY_FIVE)) TD_g_params.handStyle = TD_STYLE_EDGES;

        if (IsKeyPressed(KEY_SIX)) TD_g_params.bgStyle = TD_STYLE_NONE;
        if (IsKeyPressed(KEY_SEVEN)) TD_g_params.bgStyle = TD_STYLE_ASCII;
        if (IsKeyPressed(KEY_EIGHT)) TD_g_params.bgStyle = TD_STYLE_MATRIX;
        if (IsKeyPressed(KEY_NINE)) TD_g_params.bgStyle = TD_STYLE_CRT;
        if (IsKeyPressed(KEY_ZERO)) TD_g_params.bgStyle = TD_STYLE_EDGES;

        Vector2 mouse = GetMousePosition();
        float mx = screenW > 0 ? mouse.x / (float)screenW : 0.5f;
        float my = screenH > 0 ? mouse.y / (float)screenH : 0.5f;
        float pinch = IsMouseButtonDown(MOUSE_BUTTON_LEFT) ? 1.0f : 0.0f;
        float fist = IsMouseButtonDown(MOUSE_BUTTON_RIGHT) ? 1.0f : 0.0f;

        TouchdesignerEffect_SetHandData(
            mx, my, pinch, fist, 1,
            0.5f, 0.5f, 0.0f, 0.0f, 0);

        TouchdesignerEffect_Update(GetFrameTime());
        BeginDrawing();
            ClearBackground(BLACK);
            TouchdesignerEffect_Draw(scene, screenW, screenH);
        EndDrawing();
    }

    TouchdesignerEffect_Unload();
    UnloadRenderTexture(scene);
    CloseWindow();
    return 0;
}
