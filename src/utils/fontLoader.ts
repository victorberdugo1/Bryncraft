/**
 * fontLoader.ts — Load the matrix-mode TTF font and pass it to WASM
 *
 * Mismo patrón que cascadeLoader.ts: se trae por fetch() desde
 * public/assets/... (archivo estático servido tal cual por Vite/nginx) — NO
 * desde el FS virtual de Emscripten, para evitar la fragilidad de
 * --preload-file.
 *
 * Sin esto, el modo "matrix" del efecto ascii solo tenía la fuente bitmap
 * por defecto de raylib (GetFontDefault), que únicamente trae glifos ASCII:
 * cualquier caracter fuera de ese rango (katakana, hiragana, etc.) en
 * matrixChars se dibujaba como '?'. Esta fuente es un subset de Noto Sans
 * JP (solo ASCII + hiragana + katakana + katakana medio ancho) para que el
 * archivo sea pequeño.
 *
 * Solo Module._js_set_matrix_font_data (+ _malloc/_free/HEAPU8, ya
 * presentes) necesita estar exportado del lado wasm.
 *
 * Si falla (corrupt/truncado/404), el modo matrix simplemente sigue
 * usando GetFontDefault() como fallback (solo ASCII) -- no crashea.
 */

declare const Module: any; // Emscripten Module

const MATRIX_FONT_PATH = "/assets/fonts/NotoSansJP-Kana.ttf";

/**
 * Fetch la fuente TTF desde public/assets y pásasela a WASM's
 * js_set_matrix_font_data()
 *
 * Si la fuente está corrupta/truncada/inaccesible, no throws -- simplemente
 * devuelve y el modo matrix sigue funcionando con GetFontDefault() (solo
 * ASCII, como antes de este fix).
 */
export async function initMatrixFontData(): Promise<void> {
  try {
    const response = await fetch(MATRIX_FONT_PATH);
    if (!response.ok) {
      console.warn(
        `[Bryncraft] Matrix font fetch failed: HTTP ${response.status} for ${MATRIX_FONT_PATH}. ` +
        `Modo matrix usará GetFontDefault() (solo ASCII).`
      );
      return;
    }
    const fontData = new Uint8Array(await response.arrayBuffer());

    // ⚠️ Si está truncado (< 1KB), silenciosamente falla sin crashear.
    if (fontData.length < 1000) {
      console.warn(
        `[Bryncraft] Matrix font truncada: ${fontData.length}B < 1KB. ` +
        `Modo matrix usará GetFontDefault() (solo ASCII).`
      );
      return;
    }

    if (!Module._js_set_matrix_font_data) {
      console.warn("[Bryncraft] js_set_matrix_font_data not found. Modo matrix seguirá con GetFontDefault().");
      return;
    }

    const ptr = Module._malloc(fontData.length);
    if (!ptr) {
      console.warn("[Bryncraft] malloc failed. Modo matrix seguirá con GetFontDefault().");
      return;
    }

    Module.HEAPU8.set(fontData, ptr);
    Module._js_set_matrix_font_data(fontData.length, ptr);
    Module._free(ptr);
  } catch (error) {
    console.warn("[Bryncraft] Matrix font load failed:", error instanceof Error ? error.message : String(error));
    console.log("[Bryncraft] Modo matrix seguirá con GetFontDefault() (solo ASCII)");

    // No throws — la app sigue funcionando normalmente
  }
}
