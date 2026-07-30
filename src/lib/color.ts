// Utilidades mínimas de color hex <-> HSL. Se usan para derivar un color
// "similar" (mismo tono, más atenuado) a partir de otro — por ejemplo, el
// color de la estela del efecto Matrix a partir del color de la cabeza.

function hexToRgb(hex: string): { r: number; g: number; b: number; a: number } {
  const clean = hex.replace("#", "");
  const r = parseInt(clean.slice(0, 2), 16) || 0;
  const g = parseInt(clean.slice(2, 4), 16) || 0;
  const b = parseInt(clean.slice(4, 6), 16) || 0;
  const a = clean.length >= 8 ? parseInt(clean.slice(6, 8), 16) : 255;
  return { r, g, b, a };
}

function rgbToHex(r: number, g: number, b: number, a?: number): string {
  const toHex = (v: number) =>
    Math.round(Math.min(255, Math.max(0, v)))
      .toString(16)
      .padStart(2, "0");
  const base = `#${toHex(r)}${toHex(g)}${toHex(b)}`;
  return a !== undefined && a < 255 ? `${base}${toHex(a)}` : base;
}

function rgbToHsl(r: number, g: number, b: number) {
  r /= 255;
  g /= 255;
  b /= 255;
  const max = Math.max(r, g, b);
  const min = Math.min(r, g, b);
  let h = 0;
  let s = 0;
  const l = (max + min) / 2;
  const d = max - min;
  if (d !== 0) {
    s = d / (1 - Math.abs(2 * l - 1));
    switch (max) {
      case r:
        h = ((g - b) / d) % 6;
        break;
      case g:
        h = (b - r) / d + 2;
        break;
      default:
        h = (r - g) / d + 4;
    }
    h *= 60;
    if (h < 0) h += 360;
  }
  return { h, s, l };
}

function hslToRgb(h: number, s: number, l: number) {
  const c = (1 - Math.abs(2 * l - 1)) * s;
  const x = c * (1 - Math.abs(((h / 60) % 2) - 1));
  const m = l - c / 2;
  let r = 0;
  let g = 0;
  let b = 0;
  if (h < 60) [r, g, b] = [c, x, 0];
  else if (h < 120) [r, g, b] = [x, c, 0];
  else if (h < 180) [r, g, b] = [0, c, x];
  else if (h < 240) [r, g, b] = [0, x, c];
  else if (h < 300) [r, g, b] = [x, 0, c];
  else [r, g, b] = [c, 0, x];
  return { r: (r + m) * 255, g: (g + m) * 255, b: (b + m) * 255 };
}

/** Deriva un color "compañero" para el hex dado: mismo tono, algo más
 * oscuro y ligeramente menos saturado — pensado para que el color de la
 * estela (foreground) haga juego con el color de la cabeza (head) sin
 * quedar idéntico (la cabeza debe seguir leyéndose como el punto brillante). */
export function deriveTrailColor(headHex: string): string {
  const { r, g, b, a } = hexToRgb(headHex);
  const { h, s, l } = rgbToHsl(r, g, b);
  const trailS = Math.max(0, s * 0.85);
  const trailL = Math.max(0.15, l * 0.55);
  const rgb = hslToRgb(h, trailS, trailL);
  return rgbToHex(rgb.r, rgb.g, rgb.b, a < 255 ? a : undefined);
}
