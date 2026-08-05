// main003.c — minimal example: OpenCV Vision
//
// Unlike main000/001/002.c, this one needs a C++ link step: OpenCV has no
// C API, so opencv_effect.h is single-header (stb_image.h-style) — plain-C
// declarations at the top (what this file includes), and the real OpenCV
// pipeline further down, compiled only when OPENCV_EFFECT_IMPLEMENTATION is
// defined first. There's no separate .cpp for that: opencv_effect.h is
// compiled directly as its own C++ translation unit, `-x c++` forcing the
// C++ frontend regardless of the .h extension. Camera capture needs
// OpenCV's videoio module, which isn't part of the trimmed core/imgproc/
// video/objdetect wasm build (see opencv_effect.h) — this is only ever
// reachable through the system OpenCV pkg-config picks up here, on a real
// desktop. raylib.h / libraylib.a live one level up, in native/effects/
// (shared by every effect's standalone demo) — hence -I.. / -L.. below.
// See opencv_build_and_run.sh / .bat in this same folder for the scripted
// version of these three commands:
//
//   gcc  -c main003.c -o main003.o -I..
//   g++  -DOPENCV_EFFECT_IMPLEMENTATION -x c++ -c opencv_effect.h -o opencv_effect.o -I.. -std=c++20 $(pkg-config --cflags opencv4)
//   g++  main003.o opencv_effect.o -o opencv_demo -L.. -lraylib -lm -lpthread -ldl -lrt -lX11 $(pkg-config --libs opencv4)
//
// (On Windows: swap the last three -l flags for -lgdi32 -lwinmm, as in
// main000.c.) OpencvEffect_SetParams isn't called here — it's only declared
// under __EMSCRIPTEN__ (see opencv_effect.h), same as every other effect's
// *_SetParams in these minimal examples; Update/Draw/Unload run fine
// without it, just using the pipeline's compiled-in defaults (Canny edges).
//
// Camera: tries to open system camera 0 at startup via OcvCamera_Open()
// (native-only API, declared under the #ifndef __EMSCRIPTEN__ block in
// opencv_effect.h). If that succeeds, every frame is grabbed straight into
// `scene` with OcvCamera_CaptureInto() instead of the placeholder circle —
// so face_detect/edges/optical_flow etc. all run on your actual webcam feed.
// No camera found (or none attached) just falls back to the placeholder,
// so this still runs fine on a headless/CI box.

#include "raylib.h"
#include "opencv_effect.h"

int main(void)
{
    const int screenW = 800;
    const int screenH = 600;

    InitWindow(screenW, screenH, "OpenCV Vision — minimal example");
    SetTargetFPS(60);

    RenderTexture2D scene = LoadRenderTexture(screenW, screenH);

    bool haveCamera = OcvCamera_Open(0);
    if (!haveCamera) {
        TraceLog(LOG_WARNING, "No camera found on device 0 — using the placeholder scene instead.");
    }

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();

        if (haveCamera) {
            OcvCamera_CaptureInto(scene);
        } else {
            /*/ Start scene placeholder. /*/
            BeginTextureMode(scene);
                ClearBackground(DARKGRAY);
                DrawCircle(screenW / 2, screenH / 2, 120, RED);
            EndTextureMode();
	    /*/ End scene placeholder. /*/
        }

        OpencvEffect_Update(dt);

        BeginDrawing();
            ClearBackground(BLACK);
            OpencvEffect_Draw(scene, screenW, screenH);
        EndDrawing();
    }

    if (haveCamera) OcvCamera_Close();
    OpencvEffect_Unload();
    UnloadRenderTexture(scene);
    CloseWindow();
    return 0;
}
