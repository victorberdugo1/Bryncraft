const DEFAULT_FPS = 30;
const MIN_SAMPLE_FPS = 8;

const MAX_FILE_SIZE_BYTES = 2 * 1024 * 1024 * 1024; // 2GB
const MEMORY_BUDGET_BYTES = 400 * 1024 * 1024; // 400MB

export interface VideoFrameExtractionResult {
  frames: ImageBitmap[];
  fps: number;
  width: number;
  height: number;
  notice: string | null;
}

function formatMB(bytes: number): string {
  return `${(bytes / (1024 * 1024)).toFixed(0)}MB`;
}

function roundToEven(n: number): number {
  const r = Math.round(n);
  return r % 2 === 0 ? r : r + 1;
}

function computeExtractionPlan(
  duration: number,
  sourceWidth: number,
  sourceHeight: number,
): { fps: number; width: number; height: number; notice: string | null } {
  const bytesPerFrameFullRes = sourceWidth * sourceHeight * 4;
  const fpsThatFitsFullRes = MEMORY_BUDGET_BYTES / (duration * bytesPerFrameFullRes);

  if (fpsThatFitsFullRes >= DEFAULT_FPS) {
    return { fps: DEFAULT_FPS, width: sourceWidth, height: sourceHeight, notice: null };
  }
  if (fpsThatFitsFullRes >= MIN_SAMPLE_FPS) {
    const fps = fpsThatFitsFullRes;
    return {
      fps,
      width: sourceWidth,
      height: sourceHeight,
      notice: `Video imported at ${fps.toFixed(1)}fps (instead of ${DEFAULT_FPS}fps) to fit in memory — resolution was kept at ${sourceWidth}×${sourceHeight}, same as the original.`,
    };
  }

  const scale = Math.sqrt(MEMORY_BUDGET_BYTES / (duration * MIN_SAMPLE_FPS * bytesPerFrameFullRes));
  const clampedScale = Math.min(1, scale);
  const width = Math.max(2, roundToEven(sourceWidth * clampedScale));
  const height = Math.max(2, roundToEven(sourceHeight * clampedScale));
  return {
    fps: MIN_SAMPLE_FPS,
    width,
    height,
    notice: `Video imported at ${width}×${height} @ ${MIN_SAMPLE_FPS}fps (original: ${sourceWidth}×${sourceHeight} @ ${DEFAULT_FPS}fps) — resolution and fps were reduced because the video's length/weight didn't fit in memory.`,
  };
}

function seekToFrame(video: HTMLVideoElement, t: number): Promise<void> {
  return new Promise((resolve, reject) => {
    let settled = false;
    const clear = () => {
      video.removeEventListener("seeked", onSeeked);
      video.removeEventListener("error", onError);
      clearTimeout(timeoutHandle);
    };
    const onError = () => {
      if (settled) return;
      settled = true;
      clear();
      reject(new Error("Error al buscar frame del video"));
    };
    const finish = () => {
      if (settled) return;
      settled = true;
      clear();
      resolve();
    };
    const onSeeked = () => {
      requestAnimationFrame(() => requestAnimationFrame(finish));
    };

    // Si `t` coincide con el currentTime actual, el navegador no dispara
    // "seeked" ni requestVideoFrameCallback — sin este timeout la promesa
    // quedaría colgada para siempre.
    const timeoutHandle = setTimeout(finish, 500);

    video.addEventListener("error", onError);

    if (typeof video.requestVideoFrameCallback === "function") {
      video.requestVideoFrameCallback(() => finish());
      video.currentTime = t;
      video.addEventListener("seeked", onSeeked);
    } else {
      video.addEventListener("seeked", onSeeked);
      video.currentTime = t;
    }
  });
}

export async function extractVideoFrames(
  file: File,
  onProgress?: (done: number, total: number) => void,
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
  // Necesario en varios navegadores para que el decodificador entregue
  // frames reales al hacer seek fuera de reproducción.
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

    const { fps, width, height, notice } = computeExtractionPlan(duration, sourceWidth, sourceHeight);

    // Sin un play/pause real antes de hacer seek, algunos navegadores nunca
    // entregan frames decodificados y drawImage captura cuadros negros.
    try {
      await video.play();
      video.pause();
    } catch {
      // Autoplay bloqueado: el seek + rVFC de abajo suele bastar igual.
    }
    video.currentTime = 0;
    await seekToFrame(video, 0);

    const totalFrames = Math.max(1, Math.round(duration * fps));

    const canvas = document.createElement("canvas");
    canvas.width = width;
    canvas.height = height;
    const ctx = canvas.getContext("2d");
    if (!ctx) throw new Error("2D context unavailable");

    const frames: ImageBitmap[] = [];
    for (let i = 0; i < totalFrames; i++) {
      const t = Math.min(duration, i / fps);
      try {
        await seekToFrame(video, t);
      } catch (err) {
        try {
          await seekToFrame(video, t);
        } catch {
          console.warn(`[videoFrameExtractor] frame ${i} seek failed twice, reusing previous frame`, err);
          if (frames.length > 0) {
            frames.push(frames[frames.length - 1]);
            onProgress?.(i + 1, totalFrames);
            continue;
          }
          console.error(`[videoFrameExtractor] stopping import at frame ${i}: seek failed`, err);
          break;
        }
      }

      try {
        ctx.drawImage(video, 0, 0, width, height);
        const bitmap = await createImageBitmap(canvas);
        frames.push(bitmap);
        onProgress?.(i + 1, totalFrames);
      } catch (err) {
        console.error(`[videoFrameExtractor] stopping import at frame ${i}: could not create frame bitmap`, err);
        break;
      }
    }

    if (frames.length === 0) {
      throw new Error("No se pudo extraer ningún frame del video");
    }

    return { frames, fps, width, height, notice };
  } finally {
    video.pause();
    video.removeAttribute("src");
    video.load();
    document.body.removeChild(video);
    URL.revokeObjectURL(url);
  }
}
