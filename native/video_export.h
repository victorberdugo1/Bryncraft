#ifndef VIDEO_EXPORT_H
#define VIDEO_EXPORT_H

#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// ESTRUCTURA DE ESTADO
// ============================================================================

typedef struct {
    bool recording;
    int frameCount;
} VideoExportState;

static VideoExportState g_videoExport = {0};

// ============================================================================
// JAVASCRIPT BINDINGS (solo en WASM)
// ============================================================================

#ifdef __EMSCRIPTEN__
    #include <emscripten.h>

    // Llama a window.VideoExportJS.startEncoder(width, height, fps)
    EM_JS(void, js_start_encoder, (int width, int height, int fps), {
        if (window.VideoExportJS && window.VideoExportJS.startEncoder) {
            window.VideoExportJS.startEncoder(width, height, fps);
        } else {
            console.error('[C] VideoExportJS no está disponible');
        }
    });

    // Llama a window.VideoExportJS.finishEncoder(filename)
    EM_JS(void, js_finish_encoder, (const char *filename), {
        if (window.VideoExportJS && window.VideoExportJS.finishEncoder) {
            const filenameStr = UTF8ToString(filename);
            window.VideoExportJS.finishEncoder(filenameStr);
        } else {
            console.error('[C] VideoExportJS no está disponible');
        }
    });
#endif

// ============================================================================
// API PÚBLICA
// ============================================================================

/**
 * Inicializa el sistema de video export.
 * Llamar una sola vez al inicio del programa.
 */
static inline void VideoExportInit(void) {
    g_videoExport.recording = false;
    g_videoExport.frameCount = 0;
}

/**
 * Inicia la grabación de video.
 * @param width Ancho en píxeles
 * @param height Altura en píxeles
 */
static inline void VideoExportStart(int width, int height) {
    #ifdef __EMSCRIPTEN__
    if (!g_videoExport.recording) {
        g_videoExport.recording = true;
        g_videoExport.frameCount = 0;
        printf("[VideoExport] Grabación iniciada %dx%d\n", width, height);
    }
    #else
    printf("[VideoExport] No compilado para WASM, grabación deshabilitada\n");
    #endif
}

/**
 * Frame capture is driven exclusively from JS (see wasmBridge.captureFrame /
 * ExportPanel.tsx) — one explicit call per logical export frame. This used
 * to have a VideoExportCaptureFrame() companion called automatically from
 * the native render loop on every tick while recording, which double-fired
 * captures alongside JS's own explicit calls, corrupting exports. Removed;
 * frameCount below is now just informational bookkeeping.
 */

/**
 * Detiene la grabación y descarga el video.
 */
static inline void VideoExportStop(void) {
    #ifdef __EMSCRIPTEN__
    if (g_videoExport.recording) {
        g_videoExport.recording = false;
        printf("[VideoExport] Deteniendo grabación (%d frames)\n", g_videoExport.frameCount);
    }
    #else
    printf("[VideoExport] No compilado para WASM, nada que detener\n");
    #endif
}

/**
 * Retorna true si está grabando actualmente.
 */
static inline bool VideoExportIsRecording(void) {
    return g_videoExport.recording;
}

/**
 * Retorna la cantidad de frames grabados.
 */
static inline int VideoExportGetFrameCount(void) {
    return g_videoExport.frameCount;
}

/**
 * Limpia recursos (llamar en shutdown).
 */
static inline void VideoExportCleanup(void) {
    if (g_videoExport.recording) {
        VideoExportStop();
    }
}

#ifdef __cplusplus
}
#endif

#endif // VIDEO_EXPORT_H
