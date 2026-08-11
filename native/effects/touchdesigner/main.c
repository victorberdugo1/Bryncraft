// main.c — minimal example: hand-tracked liquid-glass portal (ascii/matrix/crt)
//
// La detección de manos real es OpenCV en C++ (ver la sección
// TOUCHDESIGNER_EFFECT_IMPLEMENTATION al final de touchdesigner_effect.h) y
// corre automáticamente en TouchdesignerEffect_Draw() cuando
// TD_g_params.autoDetectHands es true (el default). Este demo standalone lo
// desactiva y simula UNA mano con el mouse (botón izquierdo = pinch, botón
// derecho = fist) para poder iterar el shader/look sin cámara ni levantar
// el proyecto React completo.
//
// Compilar (Linux, ver native/README.md para flags de raylib):
//   g++ main.c -I.. -I../.. -I<opencv_include> `pkg-config --cflags --libs opencv4` \
//       -DTOUCHDESIGNER_EFFECT_IMPLEMENTATION -lraylib -lm -lpthread -ldl -o touchdesigner_demo

#include "raylib.h"
#include "touchdesigner_effect.h"

int main(void) {
    const int screenW = 1280, screenH = 720;
    InitWindow(screenW, screenH, "TouchDesigner Hand Portal — mouse demo (L=pinch, R=fist)");
    SetTargetFPS(60);

    RenderTexture2D scene = LoadRenderTexture(screenW, screenH);
    TouchdesignerEffect_Init();
    TD_g_params.autoDetectHands = false; // este demo maneja las manos a mano (mouse), no con cámara

    while (!WindowShouldClose()) {
        // Escena base: algo con textura/movimiento para que se note el
        // ascii/matrix/crt de adentro del portal (una cámara real la
        // reemplaza en el proyecto de verdad, vía OcvCamera_* u OpenCV).
        BeginTextureMode(scene);
            ClearBackground((Color){ 10, 12, 16, 255 });
            float t = (float)GetTime();
            for (int i = 0; i < 6; i++) {
                float x = screenW * (0.15f + 0.14f * i);
                float y = screenH * 0.5f + sinf(t * 1.3f + i) * screenH * 0.2f;
                DrawCircle((int)x, (int)y, 40 + 10 * sinf(t + i), (Color){ 40 + i * 30, 80, 160, 255 });
            }
            DrawText("mueve el mouse", 40, 40, 24, RAYWHITE);
        EndTextureMode();

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
