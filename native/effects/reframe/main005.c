// main005.c — minimal example: Reframe (16:9 -> 9:16 auto vertical crop,
// face tracking + active-speaker panning). Needs a C++ link step, see
// README.md.
//
// Part of Bryncraft (https://bryncraft.online/) — created by Victor Berdugo

#include "raylib.h"
#include "reframe_effect.h"
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
    // Landscape source, portrait output — this is the whole point of the
    // effect: the window itself stays 9:16 so you can see exactly what
    // gets exported.
    const int sourceW = 1280;
    const int sourceH = 720;
    const int screenW = 405;
    const int screenH = 720;

    InitWindow(screenW, screenH, "Reframe — 16:9 to 9:16, minimal example");
    SetTargetFPS(60);

    LoadFaceCascadeFromDisk("haarcascade_frontalface_default.xml");
    ReframeEffect_Init();

    RenderTexture2D scene = LoadRenderTexture(sourceW, sourceH);
    bool haveCamera = RfCamera_Open(0);

    while (!WindowShouldClose())
    {
        if (haveCamera) {
            RfCamera_CaptureInto(scene);
        } else {
            BeginTextureMode(scene);
                ClearBackground(DARKGRAY);
                DrawText("No camera found", 20, 20, 20, RAYWHITE);
                DrawText("Point a webcam at this window to see the auto pan", 20, 50, 16, GRAY);
            EndTextureMode();
        }

        ReframeEffect_Update(GetFrameTime());

        BeginDrawing();
            ClearBackground(BLACK);
            ReframeEffect_Draw(scene, screenW, screenH);
        EndDrawing();
    }

    if (haveCamera) RfCamera_Close();
    ReframeEffect_Unload();
    UnloadRenderTexture(scene);
    CloseWindow();
    return 0;
}
