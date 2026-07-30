/**
 * cascadeLoader.ts — Load Haar cascade XML and pass it to WASM
 *
 * Se trae por fetch() desde public/assets/cv/... (archivo estático servido
 * tal cual por Vite/nginx) — NO desde el FS virtual de Emscripten. Leerlo
 * vía Module.FS.readFile() requeriría que 'FS' esté en
 * EXPORTED_RUNTIME_METHODS del build de native/Makefile, un export fácil de
 * perder entre rebuilds; fetch() solo depende de que el archivo esté en
 * public/assets, que es un requisito mucho más simple de garantizar.
 *
 * Solo Module._js_set_cascade_data (+ _malloc/_free/HEAPU8, ya presentes)
 * necesita estar exportado del lado wasm.
 *
 * Si falla (corrupt/truncado/404), face_detect simplemente hace passthrough
 * (no crashea).
 */

declare const Module: any; // Emscripten Module

/**
 * Fetch cascade XML from public/assets and pass it to WASM's js_set_cascade_data()
 *
 * Si el cascade está corrupto/truncado/inaccesible, no throws — simplemente
 * devuelve y face_detect funciona como passthrough (copia frame sin procesar).
 */
export async function initCascadeData(): Promise<void> {
  const cascadePath = '/assets/cv/haarcascade_frontalface_default.xml';

  try {
    // Trae el archivo estático servido por Vite/nginx (no el FS virtual de wasm)
    const response = await fetch(cascadePath);
    if (!response.ok) {
      console.warn(
        `[Bryncraft] Cascade fetch failed: HTTP ${response.status} for ${cascadePath}. ` +
        `face_detect desactivado (passthrough).`
      );
      return;
    }
    const cascadeData = new Uint8Array(await response.arrayBuffer());

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
  } catch (error) {
    // Cualquier error → silencioso, graceful degradation
    console.warn('[Bryncraft] Cascade load failed:', error instanceof Error ? error.message : String(error));
    console.log('[Bryncraft] Face detection disabled (passthrough mode)');
    
    // No throws — la app sigue funcionando normalmente
  }
}


