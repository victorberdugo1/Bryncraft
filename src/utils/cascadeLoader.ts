/**
 * cascadeLoader.ts — Load Haar cascade XML from Emscripten FS and pass to WASM
 * 
 * El cascade está empaquetado en index.data por --preload-file assets
 * y es accesible en el FS virtual de WASM como /assets/cv/haarcascade_frontalface_default.xml
 * 
 * Si falla (corrupt/truncado), face_detect simplemente hace passthrough (no crashea).
 */

declare const Module: any; // Emscripten Module

/**
 * Read cascade XML from Emscripten FS and pass it to WASM's js_set_cascade_data()
 * 
 * Si el cascade está corrupto/truncado, no throws — simplemente devuelve false
 * y face_detect modo funciona como passthrough (copia frame sin procesar).
 */
export async function initCascadeData(): Promise<void> {
  const cascadePath = '/assets/cv/haarcascade_frontalface_default.xml';

  try {
    // Intenta cargar desde FS virtual
    const cascadeData = Module.FS.readFile(cascadePath);

    console.log(`[Bryncraft] Cascade read: ${cascadeData.length} bytes`);

    // ⚠️ Si está truncado (< 100KB), silenciosamente falla
    // sin crashear. face_detect será passthrough.
    if (cascadeData.length < 100000) {
      console.warn(
        `[Bryncraft] Cascade truncado: ${cascadeData.length}B < 100KB. ` +
        `face_detect desactivado (passthrough).`
      );
      return;  // ← No throws, graceful degradation
    }

    // Verificar que la función existe
    if (!Module._js_set_cascade_data) {
      console.warn('[Bryncraft] js_set_cascade_data not found. Face detection disabled.');
      return;
    }

    // Pasar a WASM
    const ptr = Module._malloc(cascadeData.length);
    if (!ptr) {
      console.warn('[Bryncraft] malloc failed. Face detection disabled.');
      return;
    }

    Module.HEAPU8.set(cascadeData, ptr);
    Module._js_set_cascade_data(cascadeData.length, ptr);
    Module._free(ptr);

    console.log('[Bryncraft] ✅ Cascade loaded successfully');
  } catch (error) {
    // Cualquier error → silencioso, graceful degradation
    console.warn('[Bryncraft] Cascade load failed:', error instanceof Error ? error.message : String(error));
    console.log('[Bryncraft] Face detection disabled (passthrough mode)');
    
    // No throws — la app sigue funcionando normalmente
  }
}


