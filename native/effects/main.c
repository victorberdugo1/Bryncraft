/*
 * gcc -o Bryncraft-Atelier_demo.exe main.c -I. -L. -lraylib -lgdi32 -lwinmm
 *
 * Teclas:
 *   1 -> ASCII
 *   2 -> Particulas
 *   3 -> CRT
 *   0 -> Sin efecto (escena limpia)
 */

#include "raylib.h"
#include "ascii_effect.h"
#include "crt_effect.h"
#include "particles_effect.h"
#include <math.h>

int main(void)
{
    const int screenWidth = 800;
    const int screenHeight = 600;

    InitWindow(screenWidth, screenHeight, "Bryncraft – Atelier Demo");

    // La cámara 3D
    Camera3D camera = { 0 };
    camera.position = (Vector3){ 5.0f, 5.0f, 5.0f };
    camera.target   = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up       = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy     = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // Textura donde se dibuja la escena 3D (se pasa a los efectos)
    RenderTexture2D sceneRT = LoadRenderTexture(screenWidth, screenHeight);

    // Inicializar el efecto CRT (compila su shader)
    CrtEffect_Init();

    // Efecto activo (0 = ninguno)
    int activeEffect = 0;
    float time = 0.0f;

    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();
        time += dt;

        // --- Cambio de efecto con teclas numéricas ---
        if (IsKeyPressed(KEY_ZERO))  activeEffect = 0;
        if (IsKeyPressed(KEY_ONE))   activeEffect = 1;
        if (IsKeyPressed(KEY_TWO))   activeEffect = 2;
        if (IsKeyPressed(KEY_THREE)) activeEffect = 3;

        // --- Actualizar efectos (todos se actualizan, aunque no se dibujen) ---
        AsciiEffect_Update(dt);
        CrtEffect_Update(dt);
        ParticlesEffect_Update(dt);

        // --- Movimiento orbital de la cámara ---
        float radius = 6.0f;
        camera.position.x = radius * cosf(time * 0.3f);
        camera.position.z = radius * sinf(time * 0.3f);
        camera.position.y = 3.0f + sinf(time * 0.5f);
        camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };

        // --- Dibujar la escena 3D en la textura ---
        BeginTextureMode(sceneRT);
            ClearBackground(DARKGRAY);
            BeginMode3D(camera);
                DrawGrid(10, 1.0f);
                DrawCube((Vector3){ 0.0f, 0.5f, 0.0f }, 1.0f, 1.0f, 1.0f, RED);
                DrawCubeWires((Vector3){ 0.0f, 0.5f, 0.0f }, 1.0f, 1.0f, 1.0f, MAROON);
                DrawSphere((Vector3){ 2.0f, 0.5f, 0.0f }, 0.5f, BLUE);
                DrawSphere((Vector3){ -2.0f, 0.5f, 0.0f }, 0.5f, GREEN);
            EndMode3D();
        EndTextureMode();

        // --- Dibujar en pantalla aplicando el efecto elegido ---
        BeginDrawing();
            ClearBackground(BLACK);

            switch (activeEffect)
            {
                case 0: // Sin efecto: dibujar la escena directamente
                    DrawTextureRec(
                        sceneRT.texture,
                        (Rectangle){ 0, 0, (float)sceneRT.texture.width, -(float)sceneRT.texture.height },
                        (Vector2){ 0, 0 },
                        WHITE
                    );
                    break;

                case 1: // ASCII
                    AsciiEffect_Draw(sceneRT, screenWidth, screenHeight);
                    break;

                case 2: // CRT
                    ParticlesEffect_Draw(sceneRT, screenWidth, screenHeight);
                    break;

                case 3: // Partículas
                    CrtEffect_Draw(sceneRT, screenWidth, screenHeight);
                    break;
            }

            // HUD con el efecto actual
            const char *effectName = (activeEffect == 0) ? "Ninguno" :
                                     (activeEffect == 1) ? "ASCII" :
                                     (activeEffect == 2) ? "Particulas" : "CRT";
            DrawText(TextFormat("Efecto: %s (teclas 0-3)", effectName), 10, 10, 20, WHITE);
            DrawText("ESC para salir", 10, 40, 15, LIGHTGRAY);
        EndDrawing();
    }

    // Limpiar
    AsciiEffect_Unload();
    CrtEffect_Unload();
    UnloadRenderTexture(sceneRT);
    CloseWindow();

    return 0;
}