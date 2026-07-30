// main003.c — minimal example: OpenCV Vision
//
// Unlike main000/001/002.c, this one needs a C++ link step: OpenCV has no
// C API, so the actual pipeline lives in opencv_bridge.cpp (compiled with
// a C++ compiler) while this file stays plain C, exactly like main.c does
// it for the real app (see the Makefile's separate `%.o: %.c` / `%.o: %.cpp`
// rules). Build it as two objects linked together with a C++ driver:
//
//   gcc  -c main003.c        -o main003.o        -I.
//   g++  -c opencv_bridge.cpp -o opencv_bridge.o  -I. -std=c++17 $(pkg-config --cflags opencv4)
//   g++  main003.o opencv_bridge.o -o opencv_demo -L. -lraylib -lm -lpthread -ldl -lrt -lX11 $(pkg-config --libs opencv4)
//
// (On Windows: swap the last three -l flags for -lgdi32 -lwinmm, as in
// main000.c.) OpencvEffect_SetParams isn't called here — it's only declared
// under __EMSCRIPTEN__ (see opencv_effect.h), same as every other effect's
// *_SetParams in these minimal examples; Update/Draw/Unload run fine
// without it, just using the pipeline's compiled-in defaults (Canny edges).

#include "raylib.h"
#include "opencv_effect.h"

int main(void)
{
    const int screenW = 800;
    const int screenH = 600;

    InitWindow(screenW, screenH, "OpenCV Vision — minimal example");
    SetTargetFPS(60);

    RenderTexture2D scene = LoadRenderTexture(screenW, screenH);

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();

        /*/ Start scene placeholder. /*/
        BeginTextureMode(scene);
            ClearBackground(DARKGRAY);
            DrawCircle(screenW / 2, screenH / 2, 120, RED);
        EndTextureMode();
	/*/ End scene placeholder. /*/

        OpencvEffect_Update(dt);

        BeginDrawing();
            ClearBackground(BLACK);
            OpencvEffect_Draw(scene, screenW, screenH);
        EndDrawing();
    }

    OpencvEffect_Unload();
    UnloadRenderTexture(scene);
    CloseWindow();
    return 0;
}
