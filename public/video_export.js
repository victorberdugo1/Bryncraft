// ============================================================================
// VideoExportJS - export a canvas to a downloadable media file
// - MP4/WebM: browser MediaRecorder stream capture. FPS/duration metadata is
//   fixed up afterwards by re-muxing through ffmpeg.wasm — which runs inside
//   ffmpeg-worker.js, off the main thread, so this never freezes the page.
// - PNG sequence: frame-by-frame PNG export with alpha preserved. All frames
//   go into one folder inside one ZIP, named uniquely per export so repeated
//   exports never collide or land as loose files in Downloads.
// - MOV alpha: frame-by-frame PNG -> QuickTime MOV (qtrle, alpha-preserving)
//   via ffmpeg.wasm. Every frame is written straight to the worker's virtual
//   FS as it's captured (never held as blobs in page memory), then ONE
//   ffmpeg exec() encodes the whole sequence into a single .mov at the end.
//   Earlier this batched frames into several "segment" .mov files and
//   concatenated them, which meant several exec() calls per export — on
//   ffmpeg.wasm's single, never-reset heap that reliably ran out of memory
//   and corrupted output on longer exports. One exec() call per export is
//   both simpler and far more memory-stable, and it's what actually
//   produces a single working file instead of a pile of numbered parts.
// ============================================================================

(function () {
  'use strict';

  let g_canvas = null;
  let g_stream = null;
  let g_mediaRecorder = null;
  let g_recordedChunks = [];
  let g_isRecording = false;
  let g_exportFormat = 'webm';
  let g_frameRate = 24;
  let g_exportStamp = '';

  // mov-alpha streaming (ffmpeg ready during capture): frames are written
  // straight to /frames in the worker's FS as they're captured; this is
  // just the running count, used for the frame_%05d.png filenames and to
  // know how many frames the final single-pass encode covers.
  let g_streamToFFmpeg = false;
  let g_frameCount = 0;

  // fallback paths (ffmpeg not ready during capture, or png-sequence export)
  let g_frameBlobs = [];
  // Total de segundos que representa el exec() de ffmpeg actualmente en
  // curso — se fija justo antes de cada llamada y es lo que el handler de
  // progreso usa para convertir progress.time en un porcentaje. Para el
  // encode de MOV es g_frameCount / g_frameRate (se conoce el nº de
  // frames); para el pase de fps-fix de MP4/WebM (que parte de un blob de
  // MediaRecorder, sin conteo de frames) se rellena con la duración
  // sondeada del blob — ver probeBlobDuration.
  let g_progressTotalSeconds = 0;

  // ----------------------------------------------------------------------
  // Status listener — purely a notification hook for the UI. It never
  // alters what gets executed (exec args, encode order, etc.); it only
  // reports which phase is currently running so the panel can show
  // something other than a frozen bar between "capture done" and
  // "file actually downloaded" — the gap that's largest for MOV, since
  // qtrle output is uncompressed-ish and the exec()/readFile()/zip steps
  // after the last captured frame can take a while with no other signal.
  // ----------------------------------------------------------------------
  let g_statusListener = null;
  let g_progressListener = null;
  function emitStatus(phase, message) {
    if (typeof g_statusListener === 'function') {
      try {
        g_statusListener(phase, message);
      } catch (error) {
        console.warn('[VideoExport] status listener threw', error);
      }
    }
  }

  function emitProgress(percent, timeElapsed, timeEstimated) {
    if (typeof g_progressListener === 'function') {
      try {
        g_progressListener({ percent, timeElapsed, timeEstimated });
      } catch (error) {
        console.warn('[VideoExport] progress listener threw', error);
      }
    }
  }

  // ----------------------------------------------------------------------
  // Monotonic progress gate + time-based fallback ramp.
  // Real progress (ffmpeg's "frame=" lines, JSZip's onUpdate) is always
  // preferred when it arrives, but neither source is guaranteed to fire
  // reliably in every browser/build, and a frozen bar for a 30s+ operation
  // reads as broken. reportProgress() only ever moves forward — whichever
  // source (real or heartbeat) currently has the higher number wins — so a
  // slow/absent real signal never leaves the bar stuck at 0%, and a real
  // signal that does arrive immediately overrides the estimate.
  // ----------------------------------------------------------------------
  let g_lastReportedPercent = -1;
  function resetProgressTracking() {
    g_lastReportedPercent = -1;
  }
  function reportProgress(percent, timeElapsed, timeEstimated) {
    const p = Math.max(0, Math.min(100, Math.round(percent)));
    if (p > g_lastReportedPercent) {
      g_lastReportedPercent = p;
      emitProgress(p, timeElapsed, timeEstimated);
    }
  }
  // Starts a timer that ramps reportProgress() from 0 up toward maxPercent
  // over estimateSeconds, easing out so it doesn't visibly stall pinned at
  // maxPercent for too long if the job runs longer than estimated. Purely a
  // fallback — a real reportProgress() call for a higher value always wins.
  function startHeartbeat(estimateSeconds, maxPercent) {
    const start = performance.now();
    const safeEstimate = estimateSeconds > 0 ? estimateSeconds : 5;
    return setInterval(() => {
      const elapsed = (performance.now() - start) / 1000;
      const ratio = Math.min(1, elapsed / safeEstimate);
      const eased = 1 - Math.pow(1 - ratio, 2);
      reportProgress(eased * maxPercent, elapsed, safeEstimate);
    }, 250);
  }
  function stopHeartbeat(timer) {
    clearInterval(timer);
  }
  // Best-effort probe of a captured blob's duration, used only to size the
  // fallback ramp for the MP4/WebM metadata-fix pass (see
  // _fixFrameRateMetadata) — never blocks the export if it fails.
  function probeBlobDuration(blob) {
    return new Promise((resolve) => {
      try {
        const url = URL.createObjectURL(blob);
        const video = document.createElement('video');
        video.preload = 'metadata';
        video.onloadedmetadata = () => {
          const duration = Number.isFinite(video.duration) ? video.duration : 0;
          URL.revokeObjectURL(url);
          resolve(duration);
        };
        video.onerror = () => {
          URL.revokeObjectURL(url);
          resolve(0);
        };
        video.src = url;
      } catch (error) {
        resolve(0);
      }
    });
  }

  // Scratch canvas used to verify a just-captured frame isn't blank (see
  // isBlobEmpty below) — reused across frames instead of recreated each time.
  let g_emptyCheckCanvas = null;
  let g_emptyCheckCtx = null;

  // ----------------------------------------------------------------------
  // ffmpeg worker client — every ffmpeg operation goes through this. None
  // of it runs on the main thread, so a slow/large encode never freezes the
  // page, and cancelRecording() can hard-abort it via worker.terminate().
  // ----------------------------------------------------------------------
  let g_worker = null;
  let g_ffmpegReady = false;
  let g_ffmpegInitPromise = null;
  let g_msgId = 0;
  const g_pending = new Map();

  function getWorker() {
    if (g_worker) return g_worker;
    g_worker = new Worker('/ffmpeg-worker.js');
    g_worker.onmessage = (event) => {
      const { id, ok, result, error, progress } = event.data;
      
      // Mensajes de progreso (sin ok/error). ffmpeg-worker.js ahora los
      // saca del canal nativo Module.setProgress(), no de parsear stderr
      // (ver el comentario ahí sobre por qué el parseo nunca disparaba).
      if (progress && g_progressTotalSeconds > 0) {
        // Se calcula aquí a partir de progress.time (tiempo ya codificado,
        // en segundos) en vez de usar progress.ratio directamente: ratio
        // es la propia estimación de ffmpeg, que solo es fiable cuando
        // conoce la duración total de antemano — no siempre cierto para
        // una entrada de secuencia de imágenes.
        // Capped at 90: the exec() call is only the encoding step. The
        // remaining 10 points are reserved for reading the file back out of
        // ffmpeg's virtual FS and handing it to the browser.
        const percent = Math.min(90, Math.round((progress.time / g_progressTotalSeconds) * 90));
        reportProgress(percent, progress.time, g_progressTotalSeconds);
        return;
      }
      
      const pending = g_pending.get(id);
      if (!pending) return;
      g_pending.delete(id);
      if (ok) pending.resolve(result);
      else pending.reject(new Error(error));
    };
    g_worker.onerror = (event) => {
      console.error('[VideoExport] ffmpeg worker error', event.message || event);
    };
    return g_worker;
  }

  function workerCall(type, payload, transfer) {
    return new Promise((resolve, reject) => {
      const id = ++g_msgId;
      g_pending.set(id, { resolve, reject });
      getWorker().postMessage({ id, type, payload }, transfer || []);
    });
  }

  // Fire-and-forget probe: spins up the worker and confirms ffmpeg.wasm
  // actually loaded there. Safe to call repeatedly — memoized.
  function ensureFFmpeg() {
    if (g_ffmpegInitPromise) return g_ffmpegInitPromise;
    g_ffmpegInitPromise = workerCall('ping', {})
      .then(() => {
        g_ffmpegReady = true;
        return true;
      })
      .catch((error) => {
        console.warn('[VideoExport] ffmpeg worker unavailable, falling back to raw capture / PNG ZIP', error);
        g_ffmpegReady = false;
        return false;
      });
    return g_ffmpegInitPromise;
  }

  // Hard-abort: kills the worker outright (even mid-exec, which a message
  // could never interrupt) and lets the next export spin up a fresh one.
  function terminateWorker() {
    if (g_worker) {
      g_worker.terminate();
      g_worker = null;
    }
    g_ffmpegReady = false;
    g_ffmpegInitPromise = null;
    g_pending.forEach((p) => p.reject(new Error('ffmpeg worker terminated')));
    g_pending.clear();
  }

  // ----------------------------------------------------------------------
  // zip worker client — compression runs entirely off the main thread (see
  // zip-worker.js for why), so progress updates actually get painted while
  // it runs instead of arriving all at once when the blocking work is done.
  // ----------------------------------------------------------------------
  let g_zipWorker = null;
  let g_zipMsgId = 0;
  const g_zipPending = new Map();

  function getZipWorker() {
    if (g_zipWorker) return g_zipWorker;
    g_zipWorker = new Worker('/zip-worker.js');
    g_zipWorker.onmessage = (event) => {
      const { id, ok, result, error, progress } = event.data;
      if (progress) {
        const pending = g_zipPending.get(id);
        if (pending && typeof pending.onProgress === 'function') {
          pending.onProgress(progress.percent);
        }
        return;
      }
      const pending = g_zipPending.get(id);
      if (!pending) return;
      g_zipPending.delete(id);
      if (ok) pending.resolve(result);
      else pending.reject(new Error(error));
    };
    g_zipWorker.onerror = (event) => {
      console.error('[VideoExport] zip worker error', event.message || event);
    };
    return g_zipWorker;
  }

  function zipWorkerCall(type, payload, transfer) {
    return new Promise((resolve, reject) => {
      const id = ++g_zipMsgId;
      g_zipPending.set(id, { resolve, reject });
      getZipWorker().postMessage({ id, type, payload }, transfer || []);
    });
  }

  function terminateZipWorker() {
    if (g_zipWorker) {
      g_zipWorker.terminate();
      g_zipWorker = null;
    }
    g_zipPending.forEach((p) => p.reject(new Error('zip worker terminated')));
    g_zipPending.clear();
  }

  // Compresión streaming: cada frame se envía al worker (transferido, cero
  // copias) en cuanto está disponible, en vez de acumular TODOS los frames
  // en un array en el hilo principal antes de mandarlos todos juntos como
  // antes -- eso duplicaba en el hilo principal toda la memoria que el zip
  // worker de por sí ya necesita, siendo la causa directa de los "Array
  // buffer allocation failed" en grabaciones largas a 1920x1080. Con esto
  // el hilo principal nunca retiene más de un frame a la vez.
  //
  // getFrame(i) debe devolver { name, data: Uint8Array } para el frame i-ésimo
  // (o null si ese frame no se pudo recuperar; se salta y sigue).
  async function streamZipFromFrames(frameCount, getFrame, folderName, percentFloor, percentCeiling, onReadProgress, onCompressStart) {
    await zipWorkerCall('zip-start', { folderName });

    let added = 0;
    for (let i = 0; i < frameCount; i += 1) {
      const frame = await getFrame(i);
      if (!frame) continue;
      await zipWorkerCall('zip-add-file', { name: frame.name, data: frame.data }, [frame.data.buffer]);
      added += 1;
      if (onReadProgress) onReadProgress(added, i);
    }

    if (added === 0) return null;

    if (onCompressStart) onCompressStart(added);

    const start = performance.now();
    const zipData = await new Promise((resolve, reject) => {
      const id = ++g_zipMsgId;
      g_zipPending.set(id, {
        resolve,
        reject,
        onProgress: (workerPercent) => {
          const elapsed = (performance.now() - start) / 1000;
          const percent = percentFloor + (workerPercent / 100) * (percentCeiling - percentFloor);
          const estimatedTotal = workerPercent > 0 ? elapsed / (workerPercent / 100) : elapsed;
          reportProgress(percent, elapsed, estimatedTotal);
        },
      });
      getZipWorker().postMessage({ id, type: 'zip-finish', payload: {} });
    });

    return new Blob([zipData], { type: 'application/zip' });
  }

  function getCanvas() {
    return document.getElementById('canvas') || window.Module?.canvas || null;
  }

  function pickMimeType(format) {
    const candidates = format === 'mp4'
      ? ['video/mp4', 'video/webm;codecs=vp9,opus', 'video/webm']
      : ['video/webm;codecs=vp9,opus', 'video/webm', 'video/mp4'];

    for (const mimeType of candidates) {
      if (typeof MediaRecorder !== 'undefined' && MediaRecorder.isTypeSupported?.(mimeType)) {
        return mimeType;
      }
    }

    return format === 'mp4' ? 'video/mp4' : 'video/webm';
  }

  function resolveDownloadName(filename, mimeType) {
    const safeName = (filename || 'export').replace(/\s+/g, '-');
    if (mimeType && mimeType.includes('mp4') && !safeName.toLowerCase().endsWith('.mp4')) {
      return `${safeName.replace(/\.[^.]+$/, '')}.mp4`;
    }
    if (mimeType && mimeType.includes('webm') && !safeName.toLowerCase().endsWith('.webm')) {
      return `${safeName.replace(/\.[^.]+$/, '')}.webm`;
    }
    return safeName;
  }

  function baseNameOf(filename) {
    return (filename || 'export').replace(/\.[^.]+$/, '');
  }

  // Unique per-export stamp (down to the second) so repeated exports of the
  // same format never overwrite/collide — each gets its own folder/file name.
  function makeExportStamp() {
    const d = new Date();
    const pad = (n) => String(n).padStart(2, '0');
    return `${d.getFullYear()}${pad(d.getMonth() + 1)}${pad(d.getDate())}-${pad(d.getHours())}${pad(d.getMinutes())}${pad(d.getSeconds())}`;
  }

  // True if every byte is identical — catches both "fully transparent"
  // (0,0,0,0 repeating) and "fully one solid color", the two shapes a
  // cleared/blanked WebGL buffer comes back as. Checks every byte (no
  // stride/sampling): the check canvas below (CHECK_SIZE) is small enough
  // that scanning it fully is still cheap, and skipping bytes is exactly
  // what let sparse real content (a handful of particle pixels on an
  // otherwise transparent frame) slip past undetected before.
  function isFlatImageData(data) {
    const first = data[0];
    for (let i = 1; i < data.length; i += 1) {
      if (data[i] !== first) return false;
    }
    return true;
  }

  // Size of the scratch canvas used to check for blank frames. Large enough
  // that a handful of sparse foreground pixels (e.g. a few particles on an
  // otherwise transparent frame) still survive the downscale below with a
  // visible contribution instead of being averaged away to nothing — a 16x16
  // target diluted sparse content so much it came back byte-identical to the
  // background, flagging genuinely non-blank frames as blank.
  const CHECK_SIZE = 128;

  // Decodes a just-captured PNG blob and confirms it actually has content.
  // Runs on every frame — the whole point is to catch a blank frame BEFORE
  // it's accepted into the export, not to trust that capture succeeded.
  async function isBlobEmpty(blob) {
    try {
      const bitmap = await createImageBitmap(blob);
      if (!g_emptyCheckCanvas) {
        g_emptyCheckCanvas = document.createElement('canvas');
        g_emptyCheckCanvas.width = CHECK_SIZE;
        g_emptyCheckCanvas.height = CHECK_SIZE;
        g_emptyCheckCtx = g_emptyCheckCanvas.getContext('2d', { willReadFrequently: true });
      }
      g_emptyCheckCtx.clearRect(0, 0, CHECK_SIZE, CHECK_SIZE);
      g_emptyCheckCtx.drawImage(bitmap, 0, 0, CHECK_SIZE, CHECK_SIZE);
      const { data } = g_emptyCheckCtx.getImageData(0, 0, CHECK_SIZE, CHECK_SIZE);
      if (typeof bitmap.close === 'function') bitmap.close();
      return isFlatImageData(data);
    } catch (error) {
      console.warn('[VideoExport] could not verify captured frame, accepting it as-is', error);
      return false; // Can't verify -> don't block the export over it.
    }
  }

  const VideoExportJS = {
    // Registers a callback fired as (phase, message) at each stage of the
    // post-capture finishing process. Never touches capture/encode
    // behavior — safe to attach/detach at any time, including mid-export.
    setStatusListener: function (fn) {
      g_statusListener = typeof fn === 'function' ? fn : null;
    },

    setProgressListener: function (fn) {
      g_progressListener = typeof fn === 'function' ? fn : null;
    },

    startEncoder: async function (width, height, fps, format) {
      // g_lastReportedPercent (see resetProgressTracking/reportProgress
      // above) is a monotonic gate that only lets progress move forward —
      // that's what stops a real signal from ever going backwards, but it
      // means it MUST be reset at the start of every new export. Without
      // this, once one export reaches 100%, every later export's
      // post-capture progress (0-99%) is silently swallowed forever
      // because it's never "greater than" the previous export's 100 —
      // the bar looks permanently stuck at 0% even though real progress
      // events are arriving underneath it.
      resetProgressTracking();
      g_exportFormat = format || 'webm';
      g_frameRate = fps || 24;
      g_recordedChunks = [];
      g_frameBlobs = [];
      g_canvas = getCanvas();
      g_exportStamp = makeExportStamp();

      if (!g_canvas) {
        console.error('[VideoExport] No canvas available for recording');
        return false;
      }

      if (g_exportFormat === 'png-sequence' || g_exportFormat === 'mov-alpha') {
        g_isRecording = true;
        g_frameCount = 0;

        g_streamToFFmpeg = false;
        if (g_exportFormat === 'mov-alpha') {
          const ready = await ensureFFmpeg();
          g_streamToFFmpeg = ready;
          if (ready) {
            await workerCall('mkdir', { path: '/frames' });
            await workerCall('cleardir', { path: '/frames' });
          }
        }

        console.log(`[VideoExport] frame export started (${width}x${height}, ${g_frameRate}fps, ${g_exportFormat}${g_streamToFFmpeg ? ', streaming-to-ffmpeg' : ''})`);
        return true;
      }

      if (typeof MediaRecorder === 'undefined' || typeof g_canvas.captureStream !== 'function') {
        console.error('[VideoExport] MediaRecorder/captureStream is not available in this browser');
        return false;
      }

      if (g_mediaRecorder && g_mediaRecorder.state !== 'inactive') {
        try {
          g_mediaRecorder.stop();
        } catch (error) {
          console.warn('[VideoExport] previous recorder could not be stopped', error);
        }
      }

      // Kick off the ffmpeg worker in the background so it's (hopefully)
      // ready by the time finishEncoder wants to fix fps/duration metadata.
      ensureFFmpeg();

      g_stream = g_canvas.captureStream(Math.max(1, g_frameRate));
      const mimeType = pickMimeType(g_exportFormat);
      const recorderOptions = mimeType ? { mimeType } : undefined;

      try {
        g_mediaRecorder = new MediaRecorder(g_stream, recorderOptions);
      } catch (error) {
        console.warn('[VideoExport] falling back to default MediaRecorder options', error);
        g_mediaRecorder = new MediaRecorder(g_stream);
      }

      g_mediaRecorder.ondataavailable = (event) => {
        if (event.data && event.data.size > 0) {
          g_recordedChunks.push(event.data);
        }
      };

      g_mediaRecorder.onerror = (event) => {
        console.error('[VideoExport] recorder error', event.error || event);
      };

      g_mediaRecorder.start(100);
      g_isRecording = true;
      console.log(`[VideoExport] recording started (${width}x${height}, ${g_frameRate}fps, ${g_exportFormat}) using ${g_mediaRecorder.mimeType || 'default'}`);
      return true;
    },

    // Captures the current canvas as one PNG frame, confirms it actually has
    // content (not blank), and only then commits it to the export — writing
    // it straight to the ffmpeg worker's FS for mov-alpha streaming, or
    // queuing the blob for png-sequence. Retries on a blank result instead of
    // silently accepting it. Returns true once the frame is genuinely
    // confirmed and committed, false if it never could be (caller decides
    // whether to retry further or abort the export).
    captureFrame: async function (frameIndex) {
      if (!g_isRecording) return true;
      if (g_exportFormat !== 'png-sequence' && g_exportFormat !== 'mov-alpha') return true;

      const canvas = getCanvas();
      if (!canvas) {
        console.error('[VideoExport] No canvas available for frame capture');
        return false;
      }

      // Algunos efectos (p.ej. el optical flow de opencv_effect.h) no
      // tienen un frame anterior con el que comparar en el primerísimo
      // frame, y devuelven a propósito una imagen totalmente vacía --  no
      // es un fallo de captura, y reintentar no cambia nada (siempre daría
      // el mismo resultado vacío). Así que en el frame 0 un resultado en
      // blanco se acepta directamente en vez de reintentar y, si sigue en
      // blanco tras los reintentos, abortar la exportación entera.
      const allowBlank = frameIndex === 0;

      const MAX_ATTEMPTS = 5;
      for (let attempt = 1; attempt <= MAX_ATTEMPTS; attempt += 1) {
        let blob = null;

        // LOCK raylib - main.c's UpdateDrawFrame skips drawing (never
        // blocks) for as long as this is set.
        if (window.Module?.ccall) {
          try {
            window.Module.ccall('js_lock_frame_capture', 'void', [], []);
          } catch (error) {
            console.warn('[VideoExport] could not lock frame capture', error);
          }
        }

        try {
          // No await between lock and read: JS is single-threaded, so
          // nothing (including the wasm main loop) can run between these
          // two lines — the canvas cannot change out from under this read.
          // A previous version awaited a short setTimeout in between, which
          // opened exactly the window where a stray frame could land.
          const dataUrl = canvas.toDataURL('image/png');
          const res = await fetch(dataUrl);
          blob = await res.blob();
        } catch (error) {
          console.error(`[VideoExport] frame capture failed (attempt ${attempt}/${MAX_ATTEMPTS})`, error);
        } finally {
          // UNLOCK raylib - allow rendering to resume
          if (window.Module?.ccall) {
            try {
              window.Module.ccall('js_unlock_frame_capture', 'void', [], []);
            } catch (error) {
              console.warn('[VideoExport] could not unlock frame capture', error);
            }
          }
        }

        const blank = blob ? await isBlobEmpty(blob) : true;
        const accept = blob && blob.size > 0 && (!blank || allowBlank);
        if (accept) {
          // Confirmed: this frame has real content (or is frame 0's known-
          // legitimate blank output). Commit it before reporting success —
          // the caller advances to the next frame only once this resolves.
          if (g_exportFormat === 'mov-alpha' && g_streamToFFmpeg) {
            try {
              const data = new Uint8Array(await blob.arrayBuffer());
              const framePath = `/frames/frame_${String(g_frameCount).padStart(5, '0')}.png`;
              await workerCall('writeFile', { path: framePath, data }, [data.buffer]);
              g_frameCount += 1;
            } catch (error) {
              console.error('[VideoExport] frame write to ffmpeg worker failed', error);
              return false;
            }
          } else {
            // png-sequence, or mov-alpha fallback when ffmpeg wasn't ready at start
            g_frameBlobs.push(blob);
          }
          return true;
        }

        console.warn(`[VideoExport] frame ${g_frameCount} came back ${blank ? 'blank' : 'invalid'}, retrying (attempt ${attempt}/${MAX_ATTEMPTS})`);
        // Give the renderer a full tick to paint something before trying
        // again — retrying instantly would very likely just re-read the
        // same not-yet-updated canvas.
        await new Promise((resolve) => window.setTimeout(resolve, 32));
      }

      console.error(`[VideoExport] frame ${g_frameCount} still blank after ${MAX_ATTEMPTS} attempts — giving up on this frame`);
      return false;
    },

    cancelRecording: function () {
      if (g_mediaRecorder && g_mediaRecorder.state !== 'inactive') {
        try {
          g_mediaRecorder.stop();
        } catch (error) {
          console.warn('[VideoExport] cancelRecording failed', error);
        }
      }

      // Hard-kill both workers: this is what makes Cancel actually work
      // even while an ffmpeg encode/exec or a zip compression is
      // mid-flight — a plain message could never interrupt either, but
      // terminating the worker thread can.
      terminateWorker();
      terminateZipWorker();

      this._cleanup();
      console.log('[VideoExport] recording cancelled');
    },

    finishEncoder: async function (filename) {
      // From here on capture is over; everything left (encode, read-back,
      // zip, download) has no per-frame signal of its own, hence the
      // explicit phase notifications below.
      emitStatus('finalizing', 'Frame capture complete, starting finishing steps…');

      if (g_exportFormat === 'png-sequence') {
        await this._finishPngZip(filename);
        this._cleanup();
        emitStatus('done', 'Export finished.');
        return;
      }

      if (g_exportFormat === 'mov-alpha') {
        if (g_streamToFFmpeg) {
          if (g_frameCount === 0) {
            console.error('[VideoExport] no frames were captured for MOV alpha');
            this._cleanup();
            emitStatus('error', 'No frames were captured.');
            return;
          }

          await this._encodeFramesToMov(filename);
          this._cleanup();
          emitStatus('done', 'Export finished.');
          return;
        }

        if (!g_frameBlobs.length) {
          console.error('[VideoExport] no frames were captured for MOV alpha');
          this._cleanup();
          emitStatus('error', 'No frames were captured.');
          return;
        }

        const ready = await ensureFFmpeg();
        if (ready) {
          try {
            await this._convertBlobsToMov(filename);
          } catch (error) {
            console.error('[VideoExport] MOV alpha export failed, falling back to PNG ZIP', error);
            await this._finishPngZip(filename);
          }
        } else {
          await this._finishPngZip(filename);
        }
        this._cleanup();
        emitStatus('done', 'Export finished.');
        return;
      }

      if (!g_isRecording || !g_mediaRecorder) {
        this._cleanup();
        emitStatus('done', 'Export finished.');
        return;
      }

      const recorder = g_mediaRecorder;
      g_mediaRecorder = null;
      g_isRecording = false;

      await new Promise((resolve, reject) => {
        recorder.onstop = () => resolve();
        recorder.onerror = (event) => reject(event.error || event);
        try {
          recorder.stop();
        } catch (error) {
          reject(error);
        }
      });

      const mimeType = recorder.mimeType || 'video/webm';
      const rawBlob = new Blob(g_recordedChunks, { type: mimeType });
      g_recordedChunks = [];

      if (!rawBlob.size) {
        console.error('[VideoExport] no media chunks were captured');
        this._cleanup();
        return;
      }

      // MediaRecorder captures at a variable frame rate based on when frames
      // actually arrive, so the container it produces doesn't reliably carry
      // the intended fps/duration metadata. Apps that re-transcode the file
      // (WhatsApp, Instagram, etc.) then guess a default fps instead of the
      // real one. Re-mux through ffmpeg forcing constant frame rate at the
      // source's actual fps fixes both — and since this runs in the worker,
      // it can take as long as it needs without freezing the page.
      let finalBlob = rawBlob;
      let finalMimeType = mimeType;

      const ready = await ensureFFmpeg();
      if (ready) {
        try {
          emitStatus('encoding', 'Fixing fps/duration metadata…');
          resetProgressTracking();
          finalBlob = await this._fixFrameRateMetadata(rawBlob, mimeType, filename);
          finalMimeType = finalBlob.type;
        } catch (error) {
          console.error('[VideoExport] could not fix fps/duration metadata, downloading raw capture instead', error);
          finalBlob = rawBlob;
          finalMimeType = mimeType;
        }
      } else {
        console.warn('[VideoExport] ffmpeg not ready, downloading raw capture (fps/duration metadata may be inaccurate)');
      }

      emitStatus('downloading', 'Handing the file to the browser…');
      const downloadName = resolveDownloadName(filename, finalMimeType);
      this._downloadBlob(finalBlob, downloadName);
      this._cleanup();
      emitStatus('done', 'Export finished.');
    },

    // Re-encodes the raw MediaRecorder output at a constant frame rate equal
    // to g_frameRate (the fps of the source video that was loaded), so the
    // exported file's fps and duration metadata are correct regardless of
    // how MediaRecorder timed the original frames. Runs entirely in the
    // ffmpeg worker — never touches the main thread.
    _fixFrameRateMetadata: async function (blob, sourceMimeType, filename) {
      const inputExt = sourceMimeType && sourceMimeType.includes('mp4') ? 'mp4' : 'webm';
      const wantsMp4 = g_exportFormat === 'mp4';
      const outputExt = wantsMp4 ? 'mp4' : 'webm';
      const inputPath = `/fpsfix_input.${inputExt}`;
      const outputPath = `/fpsfix_output.${outputExt}`;

      try {
        const data = new Uint8Array(await blob.arrayBuffer());
        await workerCall('writeFile', { path: inputPath, data }, [data.buffer]);

        // MediaRecorder output has no frame count to divide by (unlike the
        // MOV path), so probe the actual blob duration to know how far
        // along a given progress.time is. If probing fails for any reason
        // (some browsers/mimeTypes don't expose duration reliably), the
        // heartbeat below still ramps the bar so it never just sits frozen.
        const probedDuration = await probeBlobDuration(blob);
        g_progressTotalSeconds = probedDuration > 0 ? probedDuration : 0;
        const heartbeatEstimate = probedDuration > 0 ? probedDuration : 8;
        const heartbeat = startHeartbeat(heartbeatEstimate, 90);

        const args = wantsMp4
          ? [
              '-i', inputPath,
              '-r', String(g_frameRate),
              '-vsync', 'cfr',
              '-c:v', 'libx264',
              '-preset', 'veryfast',
              '-crf', '18',
              '-pix_fmt', 'yuv420p',
              '-c:a', 'aac',
              '-movflags', '+faststart',
              '-y', outputPath
            ]
          : [
              '-i', inputPath,
              '-r', String(g_frameRate),
              '-vsync', 'cfr',
              '-c:v', 'libvpx-vp9',
              '-crf', '30',
              '-b:v', '0',
              '-pix_fmt', 'yuv420p',
              '-c:a', 'libopus',
              '-y', outputPath
            ];

        let ret;
        try {
          ret = await workerCall('exec', { args });
        } finally {
          stopHeartbeat(heartbeat);
        }
        if (ret !== 0) {
          throw new Error(`ffmpeg fps fix returned ${ret}`);
        }

        emitStatus('reading', 'Reading the encoded file back from ffmpeg…');
        const outData = await workerCall('readFile', { path: outputPath });
        const outMimeType = wantsMp4 ? 'video/mp4' : 'video/webm';
        return new Blob([outData], { type: outMimeType });
      } finally {
        workerCall('unlink', { path: inputPath }).catch(() => {});
        workerCall('unlink', { path: outputPath }).catch(() => {});
      }
    },

    _downloadBlob: function (blob, filename) {
      try {
        const url = URL.createObjectURL(blob);
        const link = document.createElement('a');
        link.href = url;
        link.download = filename;
        link.style.display = 'none';
        document.body.appendChild(link);
        link.click();
        setTimeout(() => {
          if (link.parentNode) {
            link.parentNode.removeChild(link);
          }
          URL.revokeObjectURL(url);
        }, 2000);
      } catch (error) {
        console.error('[VideoExport] download failed', error);
      }
    },

    // Zips every captured PNG frame into ONE folder inside ONE zip file, so
    // Downloads only ever gets a single, organized file per export — never a
    // pile of loose PNGs or numbered zip parts. The folder/zip name carries
    // a timestamp, so a repeat export never collides with a previous one.
    _finishPngZip: async function (filename) {
      if (!g_frameBlobs.length) {
        console.error('[VideoExport] no frames were captured for PNG sequence');
        return;
      }

      const framesToZip = g_frameBlobs;
      g_frameBlobs = [];

      try {
        resetProgressTracking();
        emitStatus('reading', `Reading ${framesToZip.length} PNG frames…`);
        reportProgress(0, 0, 0);
        const baseName = baseNameOf(filename);
        const folderName = `${baseName}_${g_exportStamp}`;

        // Cada blob se lee y se manda al zip worker uno a uno (ver
        // streamZipFromFrames más arriba) -- nunca se tienen todos los
        // frames como Uint8Array en memoria a la vez, solo el que está en
        // tránsito en cada momento. Lectura y compresión van en tramos
        // separados de la barra (0-70% / 70-100%) y con su propio
        // emitStatus, para que quede claro cuál de las dos fases está
        // corriendo.
        const start = performance.now();
        const zipBlob = await streamZipFromFrames(
          framesToZip.length,
          async (i) => {
            const data = new Uint8Array(await framesToZip[i].arrayBuffer());
            return { name: `${baseName}_${String(i).padStart(5, '0')}.png`, data };
          },
          folderName,
          70,
          100,
          (added, i) => {
            if (i % 5 === 0 || i === framesToZip.length - 1) {
              const elapsed = (performance.now() - start) / 1000;
              const percent = (added / framesToZip.length) * 70;
              const estimatedTotal = percent > 0 ? elapsed / (percent / 70) : elapsed;
              reportProgress(percent, elapsed, estimatedTotal);
            }
          },
          () => {
            emitStatus('zipping', `Compressing ${framesToZip.length} PNG frames into a ZIP…`);
          },
        );

        if (!zipBlob) {
          console.error('[VideoExport] no frames could be zipped');
          return;
        }

        emitStatus('downloading', 'Handing the file to the browser…');
        reportProgress(100, 0, 0);
        this._downloadBlob(zipBlob, `${folderName}.zip`);
      } catch (error) {
        console.error('[VideoExport] ZIP export failed, those frames were lost', error);
      }
    },

    // Encodes every PNG frame currently sitting in /frames (in the worker's
    // FS) into ONE final .mov in a single ffmpeg exec() call — no segments,
    // no concat step. If the encode fails for any reason, the frames are
    // still sitting safely in the worker's FS, so they're read back and
    // zipped instead of being lost.
    _encodeFramesToMov: async function (filename) {
      const outputName = filename && filename.endsWith('.mov') ? filename : `${baseNameOf(filename)}.mov`;
      let encodeFailed = false;

      try {
        const args = [
          '-framerate', String(g_frameRate),
          '-i', '/frames/frame_%05d.png',
          '-c:v', 'qtrle',
          '-pix_fmt', 'argb',
          '-f', 'mov',
          '-y', '/output.mov'
        ];

        emitStatus('encoding', `Encoding ${g_frameCount} frames to MOV (qtrle)…`);
        resetProgressTracking();
        g_progressTotalSeconds = g_frameCount / g_frameRate;
        emitProgress(0, 0, g_progressTotalSeconds);

        // No fake timer-based progress here: ffmpeg-worker.js already parses
        // "frame=" out of ffmpeg's own stderr and posts it back as a real
        // { frame, time } progress message (see getWorker().onmessage below),
        // which is what actually drives the bar during this exec() call.
        const ret = await workerCall('exec', { args });

        if (ret !== 0) {
          throw new Error(`ffmpeg mov encode returned ${ret}`);
        }

        emitProgress(92, 0, 0);
        emitStatus('reading', 'Reading the finished MOV out of ffmpeg\u2019s virtual filesystem…');
        const data = await workerCall('readFile', { path: '/output.mov' });

        emitProgress(98, 0, 0);
        emitStatus('downloading', 'Handing the file to the browser…');
        emitProgress(100, 0, 0);
        this._downloadBlob(new Blob([data], { type: 'video/quicktime' }), outputName);
      } catch (error) {
        encodeFailed = true;
        console.error('[VideoExport] MOV encode failed, downloading captured frames as a PNG ZIP instead', error);
        emitStatus('encoding', 'MOV encode failed, falling back to a PNG ZIP of the captured frames…');
        // The frames are recovered from THIS SAME worker's FS below, so it
        // must stay alive for that -- the worker only gets torn down and
        // replaced with a fresh one *after* the recovery attempt, in the
        // finally block below.
        await this._zipFramesFromWorkerFS(filename);
      } finally {
        if (encodeFailed) {
          // A failed exec()/readFile() here is virtually always the ffmpeg
          // WASM module's own linear memory having grown right up against
          // its ceiling (it can only grow, never shrink, and this worker
          // instance is reused across every export in the page session) --
          // so even a tiny, few-second clip can start failing once enough
          // memory has piled up from earlier attempts. Once that's
          // happened once, every future exec() on this SAME worker keeps
          // failing too, however small. Terminating it here (recovery
          // frames are already read, see above) forces the NEXT export to
          // spin up a brand-new worker with a fresh heap instead of
          // inheriting this one's exhausted memory.
          terminateWorker();
        } else {
          await workerCall('cleardir', { path: '/frames' }).catch(() => {});
          await workerCall('unlink', { path: '/output.mov' }).catch(() => {});
        }
      }
    },

    // Fallback for when the single-pass MOV encode itself fails: the frames
    // are still sitting in the worker's FS (they were never held in page
    // memory), so read them back one by one and zip them — same "never lose
    // the user's frames" guarantee _finishPngZip gives the png-sequence path.
    _zipFramesFromWorkerFS: async function (filename) {
      if (g_frameCount === 0) return;

      try {
        resetProgressTracking();
        emitStatus('reading', `Recovering ${g_frameCount} frames from ffmpeg\u2019s virtual filesystem…`);
        reportProgress(0, 0, 0);
        const baseName = baseNameOf(filename);
        const folderName = `${baseName}_${g_exportStamp}`;

        // Cada frame se lee del FS del worker de ffmpeg y se manda
        // directamente al zip worker (ver streamZipFromFrames más arriba)
        // -- nunca se acumulan todos los frames en un array en el hilo
        // principal antes de zippear, que es justo lo que provocaba los
        // "Array buffer allocation failed" en grabaciones largas a 1080p.
        // Lectura y compresión se reparten en tramos separados de la barra
        // (0-70% / 70-100%) y con su propio emitStatus, para que quede
        // claro cuál de las dos fases está corriendo -- antes ambas
        // compartían el mismo 0-99%, así que un "99%" no dejaba claro si
        // ya casi terminaba de leer o llevaba un rato comprimiendo.
        let skipped = 0;
        const zipBlob = await streamZipFromFrames(
          g_frameCount,
          async (i) => {
            const framePath = `/frames/frame_${String(i).padStart(5, '0')}.png`;
            try {
              const data = await workerCall('readFile', { path: framePath });
              return { name: `${baseName}_${String(i).padStart(5, '0')}.png`, data };
            } catch (error) {
              skipped += 1;
              console.error(`[VideoExport] frame ${i} could not be read back (skipped)`, error);
              return null;
            }
          },
          folderName,
          70,
          100,
          (added) => {
            reportProgress(Math.round((added / g_frameCount) * 70), 0, 0);
          },
          () => {
            emitStatus('zipping', `Compressing ${g_frameCount - skipped} recovered frames into a ZIP…`);
          },
        );

        if (!zipBlob) {
          console.error('[VideoExport] no frames could be read back, nothing to zip');
          return;
        }
        if (skipped > 0) {
          console.warn(`[VideoExport] ${skipped} frame(s) could not be recovered and were skipped`);
        }

        emitStatus('downloading', 'Handing the file to the browser…');
        reportProgress(100, 0, 0);
        this._downloadBlob(zipBlob, `${folderName}.zip`);
      } catch (error) {
        console.error('[VideoExport] ZIP fallback also failed, those frames were lost', error);
      }
    },

    // Used only when ffmpeg wasn't ready during capture (so all frames ended
    // up as blobs in g_frameBlobs) but became ready by the time finishEncoder
    // runs. Writes those blobs into the worker's FS, then runs the same
    // single-pass encode used for the streaming path.
    _convertBlobsToMov: async function (filename) {
      emitStatus('preparing', `Handing ${g_frameBlobs.length} captured frames to ffmpeg…`);
      emitProgress(0, 0, 0);
      await workerCall('mkdir', { path: '/frames' });
      await workerCall('cleardir', { path: '/frames' });

      g_frameCount = 0;
      let usable = 0;
      const total = g_frameBlobs.length;

      for (let i = 0; i < g_frameBlobs.length; i += 1) {
        try {
          const data = new Uint8Array(await g_frameBlobs[i].arrayBuffer());
          const framePath = `/frames/frame_${String(g_frameCount).padStart(5, '0')}.png`;
          await workerCall('writeFile', { path: framePath, data }, [data.buffer]);
          g_frameCount += 1;
          usable += 1;
          emitProgress(Math.round((usable / total) * 10), 0, 0);
        } catch (error) {
          console.error(`[VideoExport] frame ${i} could not be read (skipped)`, error);
        }
      }
      g_frameBlobs = [];

      if (usable === 0) {
        throw new Error('No frames were usable');
      }

      await this._encodeFramesToMov(filename);
    },

    _cleanup: function () {
      if (g_stream) {
        g_stream.getTracks().forEach((track) => track.stop());
      }
      g_stream = null;
      g_canvas = null;
      g_mediaRecorder = null;
      g_recordedChunks = [];
      g_frameBlobs = [];
      g_isRecording = false;
      g_streamToFFmpeg = false;
      g_frameCount = 0;
    }
  };

  window.VideoExportJS = VideoExportJS;
})();