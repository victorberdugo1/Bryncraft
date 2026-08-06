# ASCII Renderer (single-header, raylib)

Turns whatever you've rendered into a `RenderTexture2D` into a grid of
ASCII characters. Two modes included:

- **normal**: grayscale with a configurable character ramp (default
  ` .:-=+*#%@`).
- **matrix**: Matrix-style falling character rain, with a trail and an
  independent head color.

## Quick start (copy & paste)

1. Copy `ascii_effect.h` into your project.
2. (Optional, only needed for kana in Matrix mode) also copy
   `NotoSansJP-Kana.ttf` into the same folder you run your game from.
   Without it, Matrix mode just falls back to raylib's default font.
3. In your code:

```c
#include "raylib.h"
#include "ascii_effect.h"

int main(void) {
    InitWindow(800, 600, "my game");
    SetTargetFPS(60);

    RenderTexture2D scene = LoadRenderTexture(800, 600);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // 1. Render your game as usual, but into "scene"
        BeginTextureMode(scene);
            ClearBackground(DARKGRAY);
            // ... your regular drawing goes here ...
        EndTextureMode();

        // 2. Apply the effect
        AsciiEffect_Update(dt);
        BeginDrawing();
            ClearBackground(BLACK);
            AsciiEffect_Draw(scene, 800, 600);
        EndDrawing();
    }

    AsciiEffect_Unload();
    UnloadRenderTexture(scene);
    CloseWindow();
    return 0;
}
```

That's it. No JSON or extra libraries needed for the basics.

## Changing parameters (colors, size, Matrix mode...)

Everything is controlled by editing the global `ASCII_g_params` variable
directly (declared inside `ascii_effect.h`). For example, before your
loop:

```c
ASCII_g_params.fontSize = 8;
ASCII_g_params.foreground = (Color){ 0, 255, 120, 255 };
ASCII_g_params.mode = ASCII_MODE_MATRIX;      // or ASCII_MODE_NORMAL
ASCII_g_params.matrixSpeed = 20.0f;
```

You can see every available field at the top of the `.h` file, in the
`ASCII_Params` struct.

## Building

```bash
# Windows (MinGW)
gcc main.c -o game.exe -I. -L. -lraylib -lgdi32 -lwinmm

# Linux
gcc main.c -o game -I. -L. -lraylib -lm -lpthread -ldl -lrt -lX11
```

(`raylib.h` / `libraylib.a` need to be reachable through those
`-I`/`-L` flags; adjust the paths to wherever you have raylib
installed)

## Included demo

`main000.c` is a minimal standalone example (a red circle with the
effect applied). Build it the same way as above.

## Feeding the effect with video or a camera

The effect doesn't know or care where the image comes from: it just
needs a `RenderTexture2D` with something drawn into it. That means you
can use any image source as the "scene", not just raylib drawing calls:

- **Video**: if you decode a video frame by frame (e.g. with an
  external video library), just draw each decoded frame inside
  `BeginTextureMode(scene)/EndTextureMode()` before calling
  `AsciiEffect_Draw`, exactly like you would with any other drawing.
- **Camera**: raylib has no built-in webcam capture, so you'll need a
  separate library to read frames from the camera (OpenCV, for
  example). Once you have each frame as an `Image`/`Texture2D`, load it
  into `scene` with `UpdateTexture`, or draw it with `DrawTexturePro`
  inside `BeginTextureMode(scene)`.

Bottom line: the pattern is always the same, only where `scene`'s
content comes from changes.

## Exporting the current frame (TXT, PNG, or JPG)

### As text (.txt)

The header already includes a function that dumps the current
character grid to a string:

```c
const char *txt = js_get_ascii_grid_text();
FILE *f = fopen("frame.txt", "wb");
if (f) { fputs(txt, f); fclose(f); }
```

Works the same whether you build native or for the web; it doesn't
depend on Emscripten.

### As an image (.png or .jpg)

`AsciiEffect_Draw` draws directly onto whatever's currently active. To
save it as an image, draw it into your own `RenderTexture2D` and export
that with raylib's own functions:

```c
RenderTexture2D frame = LoadRenderTexture(800, 600);

BeginTextureMode(frame);
    ClearBackground(BLANK);
    AsciiEffect_Draw(scene, 800, 600);
EndTextureMode();

Image img = LoadImageFromTexture(frame.texture);
ImageFlipVertical(img);          // OpenGL textures come out upside down
ExportImage(img, "frame.png");   // or "frame.jpg"
UnloadImage(img);

UnloadRenderTexture(frame);
```

## Optional JSON usage (only if you embed this in an Emscripten/JS
runtime, e.g. inside a website)

If you build with `__EMSCRIPTEN__` defined and add your own minimal
JSON parser (not included in this folder), `AsciiEffect_SetParams`
becomes available, accepting this contract:

```json
{ "characters": " .:-=+*#%@", "fontSize": 12,
  "brightness": 1.0, "contrast": 1.0, "gamma": 1.0,
  "foreground": "#RRGGBB", "background": "#RRGGBBAA",
  "invert": false,
  "mode": "normal | matrix",
  "matrixChars": "...", "matrixDirection": "down | up | both",
  "matrixSpeed": 1.0, "matrixDensity": 1.0, "matrixTrailLength": 10,
  "matrixHeadColor": "#RRGGBB",
  "matrixReactive": false, "matrixReactiveStrength": 1.0,
  "matrixImageStrength": 1.0
}
```

If you build natively without Emscripten, ignore this section
entirely: everything is configured by touching `ASCII_g_params` as
shown above.