/**
 * palmModelLoader.ts — Load the MP-PalmDet ONNX model and pass it to WASM
 *
 * Mismo patrón que cascadeLoader.ts: se trae por fetch() desde
 * public/assets/cv/... (archivo estático servido tal cual por Vite/nginx),
 * NO desde el FS virtual de Emscripten — evita depender de que 'FS' esté en
 * EXPORTED_RUNTIME_METHODS del build de native/Makefile.
 *
 * Solo Module._js_set_palm_model_data (+ _malloc/_free/HEAPU8, ya presentes)
 * necesita estar exportado del lado wasm.
 *
 * Si falla (corrupto/truncado/404), hand_tracking simplemente no detecta
 * manos (no crashea) — mismo graceful-degradation que la cara.
 */

declare const Module: any; // Emscripten Module

/**
 * Fetch el modelo ONNX del detector de palma desde public/assets y lo pasa
 * a WASM's js_set_palm_model_data()
 *
 * Si el modelo está corrupto/truncado/inaccesible, no throws — simplemente
 * devuelve y hand_tracking no detecta manos hasta que se resuelva.
 */
export async function initPalmModelData(): Promise<void> {
  const modelPath = '/assets/cv/palm_detection_mediapipe_2023feb.onnx';

  try {
    const response = await fetch(modelPath);
    if (!response.ok) {
      console.warn(
        `[Bryncraft] Palm model fetch failed: HTTP ${response.status} for ${modelPath}. ` +
        `hand_tracking desactivado (sin detección).`
      );
      return;
    }
    const modelData = new Uint8Array(await response.arrayBuffer());

    // ⚠️ Si está truncado, silenciosamente falla sin crashear. La versión
    // fp32 (sin cuantizar) real pesa ~3.7MB — el umbral se deja bajo (500KB)
    // sólo para detectar un archivo claramente cortado/vacío, no para
    // validar el tamaño exacto.
    if (modelData.length < 500000) {
      console.warn(
        `[Bryncraft] Palm model truncado: ${modelData.length}B < 500KB. ` +
        `hand_tracking desactivado (sin detección).`
      );
      return;
    }

    if (!Module._js_set_palm_model_data) {
      console.warn('[Bryncraft] js_set_palm_model_data not found. Hand tracking disabled.');
      return;
    }

    const ptr = Module._malloc(modelData.length);
    if (!ptr) {
      console.warn('[Bryncraft] malloc failed. Hand tracking disabled.');
      return;
    }

    Module.HEAPU8.set(modelData, ptr);
    Module._js_set_palm_model_data(modelData.length, ptr);
    Module._free(ptr);
  } catch (error) {
    console.warn('[Bryncraft] Palm model load failed:', error instanceof Error ? error.message : String(error));
    console.log('[Bryncraft] Hand tracking disabled (no detection)');
    // No throws — la app sigue funcionando normalmente
  }
}
