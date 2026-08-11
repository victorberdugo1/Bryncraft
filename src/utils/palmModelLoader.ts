declare const Module: any;

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
  }
}
