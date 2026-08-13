// main004.c — minimal example: TouchDesigner Hand Tracker (needs a C++ link step, see README.md)
//
// Part of Bryncraft (https://bryncraft.online/) — created by Victor Berdugo

#include "raylib.h"

#ifdef TD_ENABLE_ASCII
#include "ascii_effect.h"
#endif
#ifdef TD_ENABLE_CRT
#include "crt_effect.h"
#endif
#ifdef TD_ENABLE_OPENCV_EDGES
#include "opencv_effect.h"
#endif

#include "touchdesigner_effect.h"

int main(void) {
    const int screenW = 1280, screenH = 720;
    InitWindow(screenW, screenH, "TouchDesigner Hand Portal — camera demo");
    SetTargetFPS(60);

    RenderTexture2D scene = LoadRenderTexture(screenW, screenH);
    TouchdesignerEffect_Init();

#ifdef TD_ENABLE_ASCII
    AsciiEffect_Init();
#endif
#ifdef TD_ENABLE_CRT
    CrtEffect_Init();
#endif
#ifdef TD_ENABLE_OPENCV_EDGES
    OpencvEffect_Init();
#endif

    bool haveCamera = TD_HandCamera_Open(0);

    while (!WindowShouldClose()) {
        if (haveCamera) {
            TD_HandCamera_CaptureInto(scene);
        } else {
            BeginTextureMode(scene);
                ClearBackground((Color){ 10, 12, 16, 255 });
                DrawText("no camera found", 40, 40, 20, RAYWHITE);
            EndTextureMode();
        }

        TouchdesignerEffect_Update(GetFrameTime());
        BeginDrawing();
            ClearBackground(BLACK);
            TouchdesignerEffect_Draw(scene, screenW, screenH);
        EndDrawing();
    }

    if (haveCamera) TD_HandCamera_Close();
#ifdef TD_ENABLE_ASCII
    AsciiEffect_Unload();
#endif
#ifdef TD_ENABLE_CRT
    CrtEffect_Unload();
#endif
#ifdef TD_ENABLE_OPENCV_EDGES
    OpencvEffect_Unload();
#endif
    TouchdesignerEffect_Unload();
    UnloadRenderTexture(scene);
    CloseWindow();
    return 0;
}
