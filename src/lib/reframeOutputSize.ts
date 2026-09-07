export function computeReframeOutputSize(
  sourceWidth: number,
  sourceHeight: number,
  aspectPreset: unknown,
): [number, number] {
  const preset = String(aspectPreset ?? "9:16");
  const parts = preset.split(":").map(Number);
  const [aw, ah] = parts.length === 2 && parts[0] > 0 && parts[1] > 0 ? [parts[0], parts[1]] : [9, 16];

  const tryW = Math.round(sourceHeight * (aw / ah));
  if (tryW <= sourceWidth) {
    const w = tryW % 2 === 0 ? tryW : tryW - 1;
    const h = sourceHeight % 2 === 0 ? sourceHeight : sourceHeight - 1;
    return [w, h];
  }
  const tryH = Math.round(sourceWidth * (ah / aw));
  const h = tryH % 2 === 0 ? tryH : tryH - 1;
  const w = Math.round(h * (aw / ah));
  return [w % 2 === 0 ? w : w - 1, h];
}
