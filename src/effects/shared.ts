// Todo lo que un efecto necesita para existir, en un solo lugar. Un efecto
// nuevo es UN archivo en esta carpeta (ver ascii.ts como plantilla) + UNA
// línea de registro en index.ts. Nada más en el frontend.

// ============================================================================
// Definición de parámetros (alimenta el Inspector / panel de controles)
// ============================================================================

export type ParamType = "float" | "int" | "bool" | "color" | "string" | "select";

export interface ParamSchema {
  key: string;
  label: string;
  type: ParamType;
  default: number | boolean | string;
  min?: number;
  max?: number;
  step?: number;
  options?: string[];
  group?: string;
  /** Solo aplica a type: "color" — si es true, el valor es un hex de 8
   * dígitos #RRGGBBAA (con slider de alpha en la UI) en vez del de 6. */
  alpha?: boolean;
  /** Si está seteado, este control solo se muestra en el Inspector mientras
   * el param "mode" del propio efecto tenga uno de estos valores — es
   * puramente visual (el param/valor/comportamiento no cambia, solo si el
   * control se renderiza). Omitilo para controles que aplican siempre. */
  modes?: string[];
  /** Si está seteado, este control solo se muestra en el Inspector mientras
   * el param indicado por "key" tenga el valor "equals" — puramente visual,
   * igual que "modes" pero contra cualquier otro param del efecto. */
  showWhen?: { key: string; equals: EffectParamValue };
}

export interface EffectDefinition<Id extends string = string> {
  id: Id;
  name: string;
  description: string;
  params: ParamSchema[];
}

export type EffectParamValue = number | boolean | string;
export type EffectParams = Record<string, EffectParamValue>;

// ============================================================================
// Codegen (arma el .h/.c "listo para descargar" con los params actuales)
// ============================================================================

export function fmtInt(v: unknown): string {
  const n = typeof v === "number" ? v : Number(v);
  return String(Math.round(n));
}

// SIEMPRE con punto decimal, aunque el valor sea entero (120 -> "120.000f",
// nunca "120f", que es un sufijo inválido en C sin punto decimal).
export function fmtFloat(v: unknown): string {
  const n = typeof v === "number" ? v : Number(v);
  return `${n.toFixed(3)}f`;
}

export function escapeCString(v: unknown): string {
  return String(v).replace(/\\/g, "\\\\").replace(/"/g, '\\"');
}

export function hexToRgbComment(hex: string) {
  const h = hex.replace("#", "");
  const r = parseInt(h.slice(0, 2), 16);
  const g = parseInt(h.slice(2, 4), 16);
  const b = parseInt(h.slice(4, 6), 16);
  return `${r}, ${g}, ${b}`;
}

export function hexToRgbaComment(hex: string) {
  const h = hex.replace("#", "");
  const a = h.length >= 8 ? parseInt(h.slice(6, 8), 16) : 255;
  return `${hexToRgbComment(hex)}, ${a}`;
}

export function hexToColorLiteral(hex: string) {
  return `(Color){ ${hexToRgbComment(String(hex))}, 255 }`;
}

/** Extrae y desescapa un `static const char *NOMBRE = "..." "..." ...;` de
 * C tal como está en el .h real — así el shader/texto que se muestra en un
 * "Extra" es siempre exactamente el que se compila, nunca una copia aparte. */
export function extractCString(headerRaw: string, varName: string): string {
  const declRe = new RegExp(`static const char \\*${varName} =([\\s\\S]*?);\\r?\\n`);
  const m = headerRaw.match(declRe);
  if (!m) return "";
  const body = m[1];
  const strRe = /"(?:[^"\\]|\\.)*"/g;
  const parts = body.match(strRe) ?? [];
  return parts.map((p) => JSON.parse(p)).join("");
}

export type ExtraAsset =
  | { filename: string; label: string; kind: "text"; content: string }
  | { filename: string; label: string; kind: "binary-url"; url: string };

export interface EffectCodegenModule {
  headerRaw: string;
  mainRaw: string;
  mainFilename: string;
  readmeRaw: string;
  paramsRegex: RegExp;
  buildParamsBlock: (params: EffectParams) => string;
  /** Casi siempre una lista fija. Si un efecto necesita distintos archivos
   * según los params actuales (ej. touchdesigner: handStyle/bgStyle deciden
   * qué ascii_effect.h/crt_effect.h/opencv_effect.h hacen falta), puede ser
   * una función en vez de un array. */
  extras: ExtraAsset[] | ((params: EffectParams) => ExtraAsset[]);
}

// ============================================================================
// Thumbnail (miniatura animada en la barra lateral, Canvas 2D)
// ============================================================================

export type ThumbnailDrawFn = (ctx: CanvasRenderingContext2D, w: number, h: number, t: number) => void;

// ============================================================================
// El paquete completo que exporta cada archivo de efecto
// ============================================================================

export interface EffectModule<Id extends string = string> {
  definition: EffectDefinition<Id>;
  codegen: EffectCodegenModule;
  thumbnail: ThumbnailDrawFn;
}

// ============================================================================
// Tipos runtime que no son "definición de efecto" pero viven acá desde
// siempre — mensajes entre React y el WASM, stats del viewport, export.
// ============================================================================

export type ExportFormat = "mp4" | "webm" | "png-sequence" | "mov-alpha" | "jpg" | "png" | "ascii-txt";

export interface ExportJobState {
  running: boolean;
  format: ExportFormat;
  progress: number; // 0..1 (frame capture progress)
  currentFrame: number;
  totalFrames: number;
  etaSeconds: number;
  error?: string;
  phase?: "capturing" | "finalizing" | "encoding" | "reading" | "zipping" | "compressing" | "preparing" | "downloading" | "done" | "error";
  statusMessage?: string;
  postCaptureProgress?: number;
  postCaptureEtaSeconds?: number;
}
