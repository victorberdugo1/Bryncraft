const MAX_SOURCE_FPS = 60;
const MIN_SAMPLE_FPS = 8;

const MAX_FILE_SIZE_BYTES = 2 * 1024 * 1024 * 1024; // 2GB

// Budget for decoded frames held in memory as ImageBitmaps. Mobile browsers
// (especially iOS Safari) tend to crash tabs well before desktop limits, so
// they get a conservative budget. Desktop can comfortably take much more —
// callers pick the budget explicitly (see DESKTOP_MEMORY_BUDGET_BYTES /
// MOBILE_MEMORY_BUDGET_BYTES) rather than us guessing from the UA string.
export const MOBILE_MEMORY_BUDGET_BYTES = 900 * 1024 * 1024; // 900MB (400MB + 500MB)
export const DESKTOP_MEMORY_BUDGET_BYTES = 1536 * 1024 * 1024; // 1.5GB

// "Alta calidad" wants to get as close to the source video's own
// resolution/fps as it safely can — but a truly uncapped import (decode
// every frame at native res/fps, no matter how long/heavy the file) can
// easily demand tens of gigabytes for a long or high-res video, which
// doesn't just fail gracefully — it's the kind of allocation that hangs or
// kills the whole tab (and sometimes takes other tabs down with it) rather
// than throwing a catchable error. So "high quality" always still means
// "the best quality that fits inside a safety ceiling", never "unlimited";
// what changes here is how generous that ceiling is.
//
// navigator.deviceMemory (Chrome/Edge/Android; unavailable in
// Firefox/Safari) reports the device's RAM *class* in GB, heavily rounded
// (0.25/0.5/1/2/4/8/16/32...) — never exact, but a real signal for "does
// this machine have room to spare" that a fixed constant can't give us.
// Half of that class is a conservative slice for a single tab (leaving room
// for the browser itself, the OS, and every other open tab), floored at the
// old fixed desktop budget (so devices/browsers without the API are never
// worse off than before) and capped at a hard ceiling no device gets to
// exceed, since ImageBitmap/canvas overhead on top of the raw pixel bytes
// means even a device that reports plenty of RAM can't safely be handed a
// truly unbounded request.
const HIGH_QUALITY_HARD_CEILING_BYTES = 4 * 1024 * 1024 * 1024; // 4GB

export function getHighQualityMemoryBudgetBytes(): number {
  const deviceMemoryGb = (navigator as { deviceMemory?: number }).deviceMemory;
  if (!deviceMemoryGb || deviceMemoryGb <= 0) return DESKTOP_MEMORY_BUDGET_BYTES;
  const adaptiveBytes = deviceMemoryGb * 0.5 * 1024 * 1024 * 1024;
  return Math.min(HIGH_QUALITY_HARD_CEILING_BYTES, Math.max(DESKTOP_MEMORY_BUDGET_BYTES, adaptiveBytes));
}

export interface VideoFrameExtractionResult {
  frames: ImageBitmap[];
  fps: number;
  width: number;
  height: number;
  duration: number;
  notice: string | null;
}

function formatMB(bytes: number): string {
  return `${(bytes / (1024 * 1024)).toFixed(0)}MB`;
}

// Human-readable size for the quality-choice dialog — MB below 1GB (matches
// formatMB above, used elsewhere for error messages), GB above it, since a
// "1536MB" readout is harder to parse at a glance than "1.5GB".
export function formatBytes(bytes: number): string {
  if (bytes < 1024 * 1024 * 1024) return formatMB(bytes);
  return `${(bytes / (1024 * 1024 * 1024)).toFixed(1)}GB`;
}

function roundToEven(n: number): number {
  const r = Math.round(n);
  return r % 2 === 0 ? r : r + 1;
}

function computeExtractionPlan(
  duration: number,
  sourceWidth: number,
  sourceHeight: number,
  sourceFps: number,
  memoryBudgetBytes: number,
): { fps: number; width: number; height: number; notice: string | null } {
  const targetFps = Math.min(sourceFps, MAX_SOURCE_FPS);
  const bytesPerFrameFullRes = sourceWidth * sourceHeight * 4;
  const fpsThatFitsFullRes = memoryBudgetBytes / (duration * bytesPerFrameFullRes);

  if (fpsThatFitsFullRes >= targetFps) {
    return { fps: targetFps, width: sourceWidth, height: sourceHeight, notice: null };
  }
  if (fpsThatFitsFullRes >= MIN_SAMPLE_FPS) {
    const fps = fpsThatFitsFullRes;
    return {
      fps,
      width: sourceWidth,
      height: sourceHeight,
      notice: `Video imported at ${fps.toFixed(1)}fps (instead of ${targetFps}fps) to fit in memory — resolution was kept at ${sourceWidth}×${sourceHeight}, same as the original.`,
    };
  }

  const scale = Math.sqrt(memoryBudgetBytes / (duration * MIN_SAMPLE_FPS * bytesPerFrameFullRes));
  const clampedScale = Math.min(1, scale);
  const width = Math.max(2, roundToEven(sourceWidth * clampedScale));
  const height = Math.max(2, roundToEven(sourceHeight * clampedScale));
  return {
    fps: MIN_SAMPLE_FPS,
    width,
    height,
    notice: `Video imported at ${width}×${height} @ ${MIN_SAMPLE_FPS}fps (original: ${sourceWidth}×${sourceHeight} @ ${targetFps}fps) — resolution and fps were reduced because the video's length/weight didn't fit in memory.`,
  };
}

export interface VideoMetadata {
  duration: number;
  width: number;
  height: number;
  fps: number;
}

// Detects the source video's native fps by seeking to consecutive positions
// and measuring the smallest mediaTime step that produces a new decoded frame.
// Uses requestVideoFrameCallback (available in Chrome/Edge/Safari 15.4+) to
// read the exact presentation timestamp of each decoded frame — no playback
// needed, so autoplay restrictions and throttling cannot affect the result.
const FALLBACK_FPS = 30;

function snapFpsToStandard(measured: number): number {
  const STANDARD_FPS = [24000 / 1001, 24, 25, 30000 / 1001, 30, 48, 50, 60000 / 1001, 60, 120];
  let best = measured;
  let bestDist = Infinity;
  for (const std of STANDARD_FPS) {
    const dist = Math.abs(measured - std);
    if (dist < bestDist) { bestDist = dist; best = std; }
  }
  return bestDist / best < 0.02 ? Math.round(best * 1000) / 1000 : Math.round(measured * 100) / 100;
}

async function readFpsFromContainer(file: File): Promise<number | null> {
  try {
    const header = await file.slice(0, 64 * 1024).arrayBuffer();
    const view = new DataView(header);
    const bytes = new Uint8Array(header);

    const readU32 = (off: number) => view.getUint32(off, false);
    const readU16 = (off: number) => view.getUint16(off, false);
    const str4 = (off: number) => String.fromCharCode(bytes[off], bytes[off+1], bytes[off+2], bytes[off+3]);

    const isMp4 = (() => {
      const ftyp = str4(4);
      if (ftyp === "ftyp") return true;
      for (let i = 0; i < Math.min(bytes.length - 8, 512); i += 4) {
        if (str4(i + 4) === "ftyp") return true;
      }
      return false;
    })();

    if (isMp4) {
      const findBox = (buf: Uint8Array, dv: DataView, start: number, end: number, name: string): number => {
        let off = start;
        while (off + 8 <= end && off + 8 <= buf.length) {
          const size = dv.getUint32(off, false);
          if (size < 8) break;
          if (str4(off + 4) === name) return off;
          off += size;
        }
        return -1;
      };

      let off = 0;
      const end = bytes.length;

      const moovOff = findBox(bytes, view, off, end, "moov");
      if (moovOff < 0) return null;
      const moovSize = readU32(moovOff);
      const moovEnd = moovOff + moovSize;

      let trakOff = moovOff + 8;
      while (trakOff < moovEnd) {
        const found = findBox(bytes, view, trakOff, moovEnd, "trak");
        if (found < 0) break;
        const trakSize = readU32(found);
        const trakEnd = found + trakSize;

        const mdiaOff = findBox(bytes, view, found + 8, trakEnd, "mdia");
        if (mdiaOff < 0) { trakOff = found + trakSize; continue; }
        const mdiaSize = readU32(mdiaOff);
        const mdiaEnd = mdiaOff + mdiaSize;

        const hdlrOff = findBox(bytes, view, mdiaOff + 8, mdiaEnd, "hdlr");
        if (hdlrOff >= 0 && hdlrOff + 16 < bytes.length) {
          const handlerType = str4(hdlrOff + 16);
          if (handlerType !== "vide") { trakOff = found + trakSize; continue; }
        }

        const mdhdOff = findBox(bytes, view, mdiaOff + 8, mdiaEnd, "mdhd");
        if (mdhdOff < 0) { trakOff = found + trakSize; continue; }

        const version = bytes[mdhdOff + 8];
        let timeScale: number;
        let sampleCount: number;
        let duration: number;

        if (version === 1) {
          if (mdhdOff + 36 > bytes.length) return null;
          timeScale = readU32(mdhdOff + 20);
          duration = Number(view.getBigUint64(mdhdOff + 24, false));
        } else {
          if (mdhdOff + 28 > bytes.length) return null;
          timeScale = readU32(mdhdOff + 20);
          duration = readU32(mdhdOff + 24);
        }

        const stblOff = (() => {
          const minfOff = findBox(bytes, view, mdiaOff + 8, mdiaEnd, "minf");
          if (minfOff < 0) return -1;
          const minfSize = readU32(minfOff);
          return findBox(bytes, view, minfOff + 8, minfOff + minfSize, "stbl");
        })();

        if (stblOff >= 0) {
          const stblSize = readU32(stblOff);
          const sttsOff = findBox(bytes, view, stblOff + 8, stblOff + stblSize, "stts");
          if (sttsOff >= 0 && sttsOff + 16 < bytes.length) {
            const entryCount = readU32(sttsOff + 12);
            if (entryCount > 0 && sttsOff + 16 + 8 <= bytes.length) {
              const sampleDelta = readU32(sttsOff + 20);
              if (sampleDelta > 0) {
                return snapFpsToStandard(timeScale / sampleDelta);
              }
            }
          }
        }

        if (duration > 0 && timeScale > 0) {
          const stcoOff = (() => {
            if (stblOff < 0) return -1;
            const stblSize = readU32(stblOff);
            const stszOff = findBox(bytes, view, stblOff + 8, stblOff + stblSize, "stsz");
            if (stszOff < 0) return -1;
            return stszOff;
          })();
          if (stcoOff >= 0 && stcoOff + 20 <= bytes.length) {
            sampleCount = readU32(stcoOff + 16);
            if (sampleCount > 0) {
              return snapFpsToStandard((sampleCount * timeScale) / duration);
            }
          }
        }

        trakOff = found + trakSize;
      }
      return null;
    }

    const isWebm = bytes[0] === 0x1a && bytes[1] === 0x45 && bytes[2] === 0xdf && bytes[3] === 0xa3;
    if (isWebm) {
      for (let i = 0; i < bytes.length - 8; i++) {
        if (bytes[i] === 0x23 && bytes[i+1] === 0xe3 && bytes[i+2] === 0x83 && bytes[i+3] === 0xa4) {
          let len = 0;
          let j = i + 4;
          const vint = bytes[j];
          if (vint & 0x80) { len = vint & 0x7f; j += 1; }
          else if (vint & 0x40) { len = ((vint & 0x3f) << 8) | bytes[j+1]; j += 2; }
          else { j += 1; }
          if (len >= 4 && j + 4 <= bytes.length) {
            const ns = view.getUint32(j, false);
            if (ns > 0) return snapFpsToStandard(1e9 / ns);
          }
        }
      }
    }

    return null;
  } catch {
    return null;
  }
}

async function detectVideoFps(video: HTMLVideoElement, file?: File): Promise<number> {
  if (file) {
    const fps = await readFpsFromContainer(file);
    if (fps !== null && fps > 1 && fps <= 120) return fps;
  }

  if (typeof video.requestVideoFrameCallback !== "function") return FALLBACK_FPS;
  const duration = video.duration;
  if (!isFinite(duration) || duration <= 1) return FALLBACK_FPS;

  const JUMP = 1 / 24;
  const STANDARD_FPS = [24000 / 1001, 24, 25, 30000 / 1001, 30, 48, 50, 60000 / 1001, 60];

  const getMediaTimeAt = (t: number): Promise<number> =>
    new Promise((resolve, reject) => {
      const timeout = setTimeout(() => reject(new Error("rVFC timeout")), 1500);
      video.requestVideoFrameCallback((_now, meta) => {
        clearTimeout(timeout);
        resolve((meta as { mediaTime: number }).mediaTime);
      });
      video.currentTime = t;
    });

  const snapToStandard = (delta: number): number => {
    let best = FALLBACK_FPS;
    let bestDist = Infinity;
    for (const std of STANDARD_FPS) {
      const n = Math.max(1, Math.round(delta * std));
      const dist = Math.abs(1 / (delta / n) - std);
      if (dist < bestDist) { bestDist = dist; best = std; }
    }
    return best;
  };

  const anchors = [0.15, 0.35, 0.55, 0.75]
    .map((f) => Math.min(f * duration, duration - JUMP - 0.1))
    .filter((a) => a > 0);

  const votes: Record<number, number> = {};
  for (const anchor of anchors) {
    try {
      const t0 = await getMediaTimeAt(anchor);
      const t1 = await getMediaTimeAt(anchor + JUMP);
      const delta = t1 - t0;
      if (delta > 0.005) {
        const std = snapToStandard(delta);
        votes[std] = (votes[std] ?? 0) + 1;
      }
    } catch {
      continue;
    }
  }

  if (Object.keys(votes).length === 0) return FALLBACK_FPS;

  let winner = FALLBACK_FPS;
  let maxVotes = 0;
  for (const [fps, count] of Object.entries(votes)) {
    if (count > maxVotes) { maxVotes = count; winner = Number(fps); }
  }

  return Math.round(winner * 1000) / 1000;
}

export async function getVideoMetadata(file: File): Promise<VideoMetadata> {
  const url = URL.createObjectURL(file);
  const video = document.createElement("video");
  video.muted = true;
  video.playsInline = true;
  video.preload = "auto";
  video.style.position = "fixed";
  video.style.left = "-9999px";
  video.style.width = "1px";
  video.style.height = "1px";
  video.src = url;
  document.body.appendChild(video);
  try {
    await new Promise<void>((resolve, reject) => {
      const onLoaded = () => {
        video.removeEventListener("loadedmetadata", onLoaded);
        video.removeEventListener("error", onError);
        resolve();
      };
      const onError = () => {
        video.removeEventListener("loadedmetadata", onLoaded);
        video.removeEventListener("error", onError);
        reject(new Error("No se pudo leer el archivo de video"));
      };
      video.addEventListener("loadedmetadata", onLoaded);
      video.addEventListener("error", onError);
    });

    const duration = video.duration;
    const width = video.videoWidth;
    const height = video.videoHeight;
    if (!isFinite(duration) || duration <= 0 || !width || !height) {
      throw new Error("El video no tiene metadata válida");
    }

    const fps = await detectVideoFps(video, file);
    return { duration, width, height, fps };
  } finally {
    video.pause();
    video.removeAttribute("src");
    video.load();
    document.body.removeChild(video);
    URL.revokeObjectURL(url);
  }
}

// True if importing at `memoryBudgetBytes` would already need to drop fps
// and/or resolution below the source video's own values — i.e. whether a
// bigger budget (like the desktop one) would actually change the outcome.
// When this is false, "calidad recomendada" and "alta calidad" produce the
// exact same result, so there's nothing to ask the user about.
export function needsQualityChoice(
  metadata: VideoMetadata,
  memoryBudgetBytes: number = MOBILE_MEMORY_BUDGET_BYTES,
): boolean {
  const targetFps = Math.min(metadata.fps, MAX_SOURCE_FPS);
  const bytesPerFrameFullRes = metadata.width * metadata.height * 4;
  const fpsThatFitsFullRes = memoryBudgetBytes / (metadata.duration * bytesPerFrameFullRes);
  return fpsThatFitsFullRes < targetFps;
}

export interface ExtractionPlanPreview {
  fps: number;
  width: number;
  height: number;
  estimatedBytes: number;
  isFullQuality: boolean;
}

// Lets the UI show what a given budget will actually produce for a specific
// file *before* the user commits to importing it — e.g. the quality-choice
// dialog uses this to show real resolution/fps/size numbers for both
// options instead of vaguely promising "better quality", so a choice that's
// still going to reduce fps/resolution (because even the high-quality
// budget can't fit this particular file) says so upfront rather than
// surprising the user with the post-import notice.
export function previewExtractionPlan(
  metadata: VideoMetadata,
  memoryBudgetBytes: number,
): ExtractionPlanPreview {
  const targetFps = Math.min(metadata.fps, MAX_SOURCE_FPS);
  const { fps, width, height } = computeExtractionPlan(
    metadata.duration,
    metadata.width,
    metadata.height,
    metadata.fps,
    memoryBudgetBytes,
  );
  const estimatedBytes = Math.round(width * height * 4 * fps * metadata.duration);
  const isFullQuality = width === metadata.width && height === metadata.height && fps >= targetFps;
  return { fps: Math.round(fps * 100) / 100, width, height, estimatedBytes, isFullQuality };
}

async function extractFramesViaPlayback(
  video: HTMLVideoElement,
  canvas: HTMLCanvasElement,
  ctx: CanvasRenderingContext2D,
  duration: number,
  fps: number,
  onProgress?: (done: number, total: number) => void,
): Promise<ImageBitmap[]> {
  const frameDuration = 1 / fps;
  const pendingCaptures: Promise<ImageBitmap | null>[] = [];
  let nextSlot = 0;
  let lastCapturedMediaTime = -Infinity;
  let videoEnded = false;
  let rvcfPending = false;
  const totalExpected = Math.round(duration * fps);
  let settled = false;

  let settleResolve!: (frames: ImageBitmap[]) => void;
  let settleReject!: (err: Error) => void;
  const result = new Promise<ImageBitmap[]>((resolve, reject) => {
    settleResolve = resolve;
    settleReject = reject;
  });

  const drain = (reject?: Error) => {
    if (settled) return;
    settled = true;
    Promise.all(pendingCaptures).then((results) => {
      const filled: ImageBitmap[] = [];
      for (const r of results) {
        if (r !== null) {
          filled.push(r);
        } else if (filled.length > 0) {
          filled.push(filled[filled.length - 1]);
        }
      }
      if (filled.length === 0) {
        if (reject) settleReject(reject);
        else settleReject(new Error("No se pudo extraer ningún frame del video"));
        return;
      }
      while (filled.length < totalExpected) {
        filled.push(filled[filled.length - 1]);
      }
      settleResolve(filled);
    });
  };

  const trySettle = () => {
    if (videoEnded && !rvcfPending) drain();
  };

  const timeout = setTimeout(() => {
    video.pause();
    drain();
  }, duration * 1000 + 30000);

  const cleanup = () => {
    clearTimeout(timeout);
    video.removeEventListener("ended", onEnded);
    video.removeEventListener("error", onError);
    video.removeEventListener("abort", onAbort);
    video.removeEventListener("pause", onPause);
    video.removeEventListener("stalled", onStalled);
  };

  const onEnded = () => {
    cleanup();
    videoEnded = true;
    trySettle();
  };

  const onError = () => {
    cleanup();
    drain(new Error("Error durante la reproducción del video"));
  };

  const onAbort = () => {
    cleanup();
    drain();
  };

  let resuming = false;
  const resumePlayback = () => {
    if (settled || videoEnded || resuming) return;
    resuming = true;
    video.play().then(() => { resuming = false; }).catch(() => drain());
  };

  const onPause = () => resumePlayback();
  const onStalled = () => resumePlayback();

  video.addEventListener("ended", onEnded);
  video.addEventListener("error", onError);
  video.addEventListener("abort", onAbort);
  video.addEventListener("pause", onPause);
  video.addEventListener("stalled", onStalled);

  let reportedCount = 0;

  const scheduleNext = () => {
    if (settled) return;
    rvcfPending = true;
    video.requestVideoFrameCallback((_now, meta) => {
      rvcfPending = false;
      if (settled) {
        trySettle();
        return;
      }
      const mediaTime = (meta as { mediaTime: number }).mediaTime;
      const targetTime = nextSlot * frameDuration;

      if (
        mediaTime >= targetTime - frameDuration * 0.5 &&
        mediaTime - lastCapturedMediaTime >= frameDuration * 0.5
      ) {
        let capturePromise: Promise<ImageBitmap | null>;
        try {
          ctx.drawImage(video, 0, 0, canvas.width, canvas.height);
          capturePromise = createImageBitmap(canvas).then((bmp) => {
            if (!settled) {
              reportedCount++;
              onProgress?.(reportedCount, totalExpected);
            }
            return bmp;
          }).catch(() => null);
        } catch {
          capturePromise = Promise.resolve(null);
        }
        pendingCaptures.push(capturePromise);
        lastCapturedMediaTime = mediaTime;
        nextSlot++;
      }

      if (videoEnded) {
        trySettle();
      } else {
        scheduleNext();
      }
    });
  };

  video.currentTime = 0;
  video.playbackRate = 1;
  video.play()
    .then(scheduleNext)
    .catch((err) => drain(err instanceof Error ? err : new Error(String(err))));

  return result;
}

export async function extractVideoFrames(
  file: File,
  onProgress?: (done: number, total: number) => void,
  memoryBudgetBytes: number = MOBILE_MEMORY_BUDGET_BYTES,
): Promise<VideoFrameExtractionResult> {
  if (file.size > MAX_FILE_SIZE_BYTES) {
    throw new Error(
      `El archivo pesa ${formatMB(file.size)} y el máximo soportado es ${formatMB(MAX_FILE_SIZE_BYTES)}. Prueba con un video más corto o comprimido.`,
    );
  }

  const url = URL.createObjectURL(file);
  const video = document.createElement("video");
  video.muted = true;
  video.playsInline = true;
  video.preload = "auto";
  video.style.position = "fixed";
  video.style.left = "-9999px";
  video.style.width = "1px";
  video.style.height = "1px";
  video.src = url;
  document.body.appendChild(video);

  try {
    await new Promise<void>((resolve, reject) => {
      const onLoaded = () => {
        video.removeEventListener("loadedmetadata", onLoaded);
        video.removeEventListener("error", onError);
        resolve();
      };
      const onError = () => {
        video.removeEventListener("loadedmetadata", onLoaded);
        video.removeEventListener("error", onError);
        reject(new Error("No se pudo leer el archivo de video"));
      };
      video.addEventListener("loadedmetadata", onLoaded);
      video.addEventListener("error", onError);
    });

    const duration = video.duration;
    if (!isFinite(duration) || duration <= 0) {
      throw new Error("El video no tiene una duración válida");
    }

    const sourceWidth = video.videoWidth;
    const sourceHeight = video.videoHeight;
    if (!sourceWidth || !sourceHeight) {
      throw new Error("El video no tiene dimensiones válidas");
    }

    const sourceFps = await detectVideoFps(video, file);

    const { fps, width, height, notice } = computeExtractionPlan(
      duration,
      sourceWidth,
      sourceHeight,
      sourceFps,
      memoryBudgetBytes,
    );

    const canvas = document.createElement("canvas");
    canvas.width = width;
    canvas.height = height;
    const ctx = canvas.getContext("2d");
    if (!ctx) throw new Error("2D context unavailable");

    const frames = await extractFramesViaPlayback(video, canvas, ctx, duration, fps, onProgress);

    if (frames.length === 0) {
      throw new Error("No se pudo extraer ningún frame del video");
    }

    return { frames, fps, width, height, duration, notice };
  } finally {
    video.pause();
    video.removeAttribute("src");
    video.load();
    document.body.removeChild(video);
    URL.revokeObjectURL(url);
  }
}
