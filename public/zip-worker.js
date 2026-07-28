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
    if (type === 'zip') {
      const JSZipCtor = await loadJSZip();
      const zip = new JSZipCtor();
      const folder = payload.folderName ? zip.folder(payload.folderName) : zip;
      const files = payload.files; // [{ name, data: Uint8Array }]

      // Adding files to the in-memory zip structure is cheap (no
      // compression yet), so this portion is a small slice of the total:
      // 0-10%. Real compression work (10-100%) happens in generateAsync.
      g_lastProgressPost = 0;
      for (let i = 0; i < files.length; i += 1) {
        folder.file(files[i].name, files[i].data);
        maybePostProgress(id, Math.round(((i + 1) / files.length) * 10), i === files.length - 1);
      }

      g_lastProgressPost = 0;
      const zipData = await zip.generateAsync({ type: 'uint8array', compression: 'DEFLATE' }, (metadata) => {
        maybePostProgress(id, 10 + Math.round((metadata.percent / 100) * 90));
      });

      self.postMessage({ id, ok: true, result: zipData }, [zipData.buffer]);
      return;
    }
    throw new Error(`unknown zip-worker message: ${type}`);
  } catch (error) {
    self.postMessage({ id, ok: false, error: String((error && error.message) || error) });
  }
};
