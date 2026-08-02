// ============================================================================
// zip-worker.js — runs JSZip's DEFLATE compression inside a dedicated Worker.
//
// Why this exists: JSZip's API is Promise-based ("generateAsync"), but the
// actual DEFLATE compression is CPU-bound and doesn't hand control back to
// the browser's render loop often enough for a progress bar to actually
// repaint while it's running. Doing this on the main thread meant the
// "Compressing…" bar looked frozen at 0% for the whole operation even
// though the underlying compression was progressing fine — the DOM update
// was queued, it just never got a chance to paint.
//
// Moving it into its own Worker (same pattern as ffmpeg-worker.js) fixes
// that directly: the main thread stays completely free, so every progress
// postMessage from here gets painted as it arrives instead of piling up
// behind one giant blocking task.
// ============================================================================

let g_jsZipPromise = null;
// Estado del zip "en construcción" cuando se usa el protocolo streaming
// (zip-start / zip-add-file / zip-finish) en vez del mensaje 'zip' de una
// sola pasada. Un solo zip en construcción a la vez -- suficiente, ya que
// cada export tiene su propio ciclo start→add*→finish antes del siguiente.
let g_streamZip = null;
let g_streamFolder = null;
let g_streamFileCount = 0;

function loadJSZip() {
  if (g_jsZipPromise) return g_jsZipPromise;
  g_jsZipPromise = new Promise((resolve, reject) => {
    if (typeof JSZip !== 'undefined') {
      resolve(JSZip);
      return;
    }
    try {
      importScripts('https://cdn.jsdelivr.net/npm/jszip@3.10.1/dist/jszip.min.js');
      resolve(JSZip);
    } catch (error) {
      reject(new Error('JSZip could not be loaded in the worker: ' + error.message));
    }
  });
  return g_jsZipPromise;
}

// Throttled the same way ffmpeg-worker.js throttles its progress messages,
// so a fast compress on a small export doesn't flood postMessage either.
let g_lastProgressPost = 0;
function maybePostProgress(id, percent, force) {
  const now = Date.now();
  if (!force && now - g_lastProgressPost < 100) return;
  g_lastProgressPost = now;
  self.postMessage({ id, progress: { percent } });
}

self.onmessage = async (event) => {
  const { id, type, payload } = event.data;
  try {
    // --- Protocolo streaming: úsalo cuando los frames llegan uno a uno
    // (p.ej. desde el FS del worker de ffmpeg, o desde blobs capturados)
    // en vez de todos juntos en un solo mensaje -- así el hilo principal
    // nunca necesita tener todos los frames en memoria a la vez, solo el
    // que esté en tránsito. El worker sigue necesitando guardar todos los
    // frames (JSZip los retiene hasta generateAsync), pero se evita la
    // copia adicional que antes existía en el hilo principal a la vez.
    if (type === 'zip-start') {
      const JSZipCtor = await loadJSZip();
      g_streamZip = new JSZipCtor();
      g_streamFolder = payload.folderName ? g_streamZip.folder(payload.folderName) : g_streamZip;
      g_streamFileCount = 0;
      self.postMessage({ id, ok: true, result: null });
      return;
    }
    if (type === 'zip-add-file') {
      if (!g_streamZip) throw new Error('zip-add-file called before zip-start');
      g_streamFolder.file(payload.name, payload.data);
      g_streamFileCount += 1;
      self.postMessage({ id, ok: true, result: null });
      return;
    }
    if (type === 'zip-finish') {
      if (!g_streamZip) throw new Error('zip-finish called before zip-start');
      const zip = g_streamZip;
      g_streamZip = null;
      g_streamFolder = null;
      g_streamFileCount = 0;
      g_lastProgressPost = 0;
      // STORE, no DEFLATE: el contenido son PNGs, que ya vienen comprimidos.
      // Aplicarles DEFLATE encima apenas reduce el tamaño (unos pocos %) y
      // en cambio cuesta muchísima CPU de un solo hilo para cientos de
      // frames a 1920x1080 -- es lo que hacía que esto se quedara "colgado"
      // cerca del 100% durante mucho tiempo (JSZip seguía comprimiendo de
      // verdad, pero su estimación de porcentaje no es lineal cerca del
      // final). STORE simplemente copia los bytes: casi instantáneo.
      const zipData = await zip.generateAsync({ type: 'uint8array', compression: 'STORE' }, (metadata) => {
        maybePostProgress(id, Math.round(metadata.percent));
      });
      self.postMessage({ id, ok: true, result: zipData }, [zipData.buffer]);
      return;
    }

    throw new Error(`unknown zip-worker message: ${type}`);
  } catch (error) {
    self.postMessage({ id, ok: false, error: String((error && error.message) || error) });
  }
};
