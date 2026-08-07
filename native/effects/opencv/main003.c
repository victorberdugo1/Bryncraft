// main003.c — minimal example: OpenCV Vision (needs a C++ link step, see README.md)

#include "raylib.h"
#include "opencv_effect.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <direct.h>
#endif

extern void js_set_cascade_data(size_t bufSize, unsigned char *buf);

static void LoadFaceCascadeFromDisk(const char *path)
{
#ifdef _WIN32
    _mkdir("/tmp");
#endif

    FILE *f = fopen(path, "rb");
    if (!f) { TraceLog(LOG_WARNING, "Could not open %s", path); return; }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    unsigned char *buf = (unsigned char *)malloc((size_t)size);
    if (buf) {
        fread(buf, 1, (size_t)size, f);
        js_set_cascade_data((size_t)size, buf);
        free(buf);
    }
    fclose(f);
}

int main(void)
{
    const int screenW = 800;
    const int screenH = 600;

    InitWindow(screenW, screenH, "OpenCV Vision — minimal example");
    SetTargetFPS(60);

    LoadFaceCascadeFromDisk("haarcascade_frontalface_default.xml");

    RenderTexture2D scene = LoadRenderTexture(screenW, screenH);
    bool haveCamera = OcvCamera_Open(0);

    while (!WindowShouldClose())
    {
        if (haveCamera) {
            OcvCamera_CaptureInto(scene);
        } else {
            BeginTextureMode(scene);
                ClearBackground(DARKGRAY);
                DrawCircle(screenW / 2, screenH / 2, 120, RED);
            EndTextureMode();
        }

        OpencvEffect_Update(GetFrameTime());

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
