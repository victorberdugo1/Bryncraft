// ============================================================================
// ffmpeg-worker.js — runs ffmpeg.wasm inside a dedicated Worker.
// Everything here (loading the core, FS ops, exec) happens off the main
// thread, so no matter how long an encode takes, the page/UI never freezes
// and a terminate() from the main thread can hard-abort it instantly.
// ============================================================================

let g_module = null;
let g_stderr = '';
// Which exec() call (by message id) is currently running, if any — the
// permanent logger/progress handlers below only report progress while this
// is set, and are installed ONCE (not reassigned per-exec()) since some
// @ffmpeg/core builds only read these hooks once during their own init.
let g_activeExecId = null;
let g_lastProgressPost = 0;

function loadCore() {
  return new Promise((resolve, reject) => {
    if (typeof createFFmpegCore !== 'undefined') {
      resolve();
      return;
    }
    const candidates = [
      '/ffmpeg/ffmpeg-core.js',
      'https://cdn.jsdelivr.net/npm/@ffmpeg/core@0.12.10/dist/umd/ffmpeg-core.js',
    ];
    let i = 0;
    const tryNext = () => {
      if (i >= candidates.length) {
        reject(new Error('ffmpeg core script could not be loaded'));
        return;
      }
      const url = candidates[i];
      i += 1;
      try {
        importScripts(url);
        resolve();
      } catch (error) {
        tryNext();
      }
    };
    tryNext();
  });
}

// ffmpeg-core.js overwrites whatever locateFile() you pass it (it has its
// own internal one baked in for its normal loading flow) and, lacking a
// mainScriptUrlOrBlob, falls back to resolving "ffmpeg-core.wasm" relative
// to *this worker script's own* directory — i.e. "/ffmpeg-core.wasm" at the
// site root, not "/ffmpeg/ffmpeg-core.wasm" where the file actually lives.
// That 404s, the dev/preview server's SPA fallback serves index.html for
// it, and WebAssembly.instantiate then chokes on HTML instead of wasm
// ("expected magic word", since it got "<!do..." instead of the binary).
// Fetching the .wasm ourselves and handing it in as Module.wasmBinary
// sidesteps locateFile entirely — the core uses it directly and never
// tries to fetch anything on its own.
function fetchWasmBinary() {
  const candidates = [
    '/ffmpeg/ffmpeg-core.wasm',
    'https://cdn.jsdelivr.net/npm/@ffmpeg/core@0.12.10/dist/umd/ffmpeg-core.wasm',
  ];
  return (async () => {
    let lastError = null;
    for (const url of candidates) {
      try {
        const res = await fetch(url);
        if (!res.ok) {
          lastError = new Error(`HTTP ${res.status} fetching ${url}`);
          continue;
        }
        const buf = await res.arrayBuffer();
        const head = new Uint8Array(buf.slice(0, 4));
        const isWasm = head[0] === 0x00 && head[1] === 0x61 && head[2] === 0x73 && head[3] === 0x6d;
        if (!isWasm) {
          lastError = new Error(`${url} did not return a valid .wasm file (got something else, e.g. an HTML fallback page)`);
          continue;
        }
        return buf;
      } catch (error) {
        lastError = error;
      }
    }
    throw lastError || new Error('ffmpeg-core.wasm could not be loaded from any candidate URL');
  })();
}

const initPromise = (async () => {
  await loadCore();
  const wasmBinary = await fetchWasmBinary();
  g_module = await createFFmpegCore({
    wasmBinary,
    locateFile: (path) => (path.endsWith('.wasm') ? '/ffmpeg/ffmpeg-core.wasm' : path),
  });
  // This @ffmpeg/core build (0.12.x) does NOT call the printErr/print
  // options passed to createFFmpegCore() above — its own init immediately
  // overwrites Module.print/Module.printErr with wrappers that route
  // through Module.logger (see setLogger below), and separately exposes a
  // native progress channel (Module.setProgress) fed directly from C via a
  // send_progress() call on every internal progress tick. That channel is
  // far more reliable than regex-parsing ffmpeg's stderr text (which is
  // what a previous version of this file tried, and which silently never
  // fired because of the above — the bar sat frozen at 0% for the whole
  // encode). Both are installed ONCE here, never reassigned per-exec(),
  // and gated by g_activeExecId so they're a no-op outside of an exec()
  // call.
  g_module.setLogger(({ message }) => {
    g_stderr += `${message}\n`;
  });
  g_module.setProgress(({ progress, time }) => {
    if (g_activeExecId === null) return;
    const now = Date.now();
    if (now - g_lastProgressPost < 100) return;
    g_lastProgressPost = now;
    // `time` is the amount of output encoded so far, in microseconds
    // (ffmpeg.wasm convention); `progress` is ffmpeg's own 0..1 estimate,
    // which is only meaningful when it already knows the total duration
    // up front (not always true for an image2-sequence input) — so the
    // percent shown to the UI is computed from elapsed time vs. the
    // known total (g_frameCount / g_frameRate) on the main thread instead
    // of trusted blindly here; this just forwards the raw numbers.
    self.postMessage({ id: g_activeExecId, progress: { time: time / 1e6, ratio: progress } });
  });
})();

function clearDir(path) {
  let entries = [];
  try {
    entries = g_module.FS.readdir(path);
  } catch (error) {
    return; // directory might not exist yet
  }
  for (const entry of entries) {
    if (entry === '.' || entry === '..') continue;
    try {
      g_module.FS.unlink(`${path}/${entry}`);
    } catch (error) {
      // best effort
    }
  }
}

self.onmessage = async (event) => {
  const { id, type, payload } = event.data;
  try {
    await initPromise;
    switch (type) {
      case 'ping': {
        self.postMessage({ id, ok: true, result: null });
        break;
      }
      case 'mkdir': {
        try { g_module.FS.mkdir(payload.path); } catch (error) { /* already exists */ }
        self.postMessage({ id, ok: true, result: null });
        break;
      }
      case 'cleardir': {
        clearDir(payload.path);
        self.postMessage({ id, ok: true, result: null });
        break;
      }
      case 'unlink': {
        try { g_module.FS.unlink(payload.path); } catch (error) { /* ignore */ }
        self.postMessage({ id, ok: true, result: null });
        break;
      }
      case 'writeFile': {
        g_module.FS.writeFile(payload.path, payload.data);
        self.postMessage({ id, ok: true, result: null });
        break;
      }
      case 'readFile': {
        const data = g_module.FS.readFile(payload.path);
        self.postMessage({ id, ok: true, result: data }, [data.buffer]);
        break;
      }
      case 'exec': {
        g_stderr = '';
        g_lastProgressPost = 0;
        g_activeExecId = id;
        const ret = g_module.exec(...payload.args);
        g_activeExecId = null;
        self.postMessage({ id, ok: true, result: ret });
        break;
      }
      default:
        throw new Error(`unknown ffmpeg-worker message: ${type}`);
    }
  } catch (error) {
    self.postMessage({ id, ok: false, error: String((error && error.message) || error) });
  }
};
