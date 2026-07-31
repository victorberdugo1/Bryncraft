import type { EffectId, EffectParams, ViewportOverlayStats } from "@/types/effects";

interface Particle {
  x: number;
  y: number;
  vx: number;
  vy: number;
  age: number;
  lifetime: number;
}

interface MatrixStream {
  col: number;
  slot: 0 | 1;
  head: number;
  dir: 1 | -1;
  speed: number;
  active: boolean;
}

// El stack de fuentes usado por el canvas para el efecto ASCII/Matrix.
const ASCII_FONT_STACK = `"JetBrains Mono", monospace`;

function hexToRgb(hex: string): [number, number, number] {
  const clean = hex.replace("#", "");
  const r = parseInt(clean.slice(0, 2), 16);
  const g = parseInt(clean.slice(2, 4), 16);
  const b = parseInt(clean.slice(4, 6), 16);
  return [Number.isNaN(r) ? 0 : r, Number.isNaN(g) ? 0 : g, Number.isNaN(b) ? 0 : b];
}

/**
 * Reference preview renderer. This is NOT the final visual output — the compiled
 * Raylib/WASM build (native/) owns rendering — but it mirrors the same param
 * contract so the UI is fully interactive during development, and it doubles as
 * a spec for what native/effects/*.c should reproduce.
 */
export class MockRenderer {
  private ctx: CanvasRenderingContext2D;
  private canvas: HTMLCanvasElement;
  private effect: EffectId = "ascii";
  private params: EffectParams = {};
  private particles: Particle[] = [];
  private matrixStreams: MatrixStream[] = [];
  private matrixGlyphs: string[] = [];
  private matrixBaseGlyphs: string[] = [];
  private matrixCols = 0;
  private matrixRows = 0;
  private matrixColLumaPrev: number[] = [];
  // Canvas offscreen de readback para renderAscii/renderAsciiMatrix — se
  // recreaba con document.createElement("canvas") en CADA frame antes de
  // este cambio, aunque cols/rows casi nunca varían entre frames (solo
  // cambian con el tamaño del viewport/fuente). Se cachea y solo se
  // recrea cuando cols/rows realmente cambian, igual que ya se hacía para
  // this.matrixStreams más abajo.
  private asciiOffCanvas: HTMLCanvasElement | null = null;
  private asciiOffCtx: CanvasRenderingContext2D | null = null;
  private asciiOffW = 0;
  private asciiOffH = 0;
  private frame = 0;
  // requestAnimationFrame-based scheduling — the browser's smoothest option
  // while the tab is visible — but it is fully suspended by every major
  // browser while the document is hidden, which froze both the live preview
  // and any in-progress export the moment the tab lost focus. setTimeout
  // keeps firing (throttled, but never fully stopped) in background tabs,
  // so the render loop uses it instead; the ~16ms interval targets the same
  // ~60fps rAF gave us while the tab is in the foreground.
  private timerHandle: number | null = null;
  private static readonly TICK_INTERVAL_MS = 16;
  private lastTime = performance.now();
  private fps = 0;
  private onStats?: (s: ViewportOverlayStats) => void;
  private t0 = performance.now();
  private sourceFrames: ImageBitmap[] | null = null;
  private sourceFrameIndex = 0;
  // Live camera feed (getUserMedia, via ViewportCanvas + cameraCapture.ts).
  // Mutually exclusive with sourceFrames at the app level (see
  // useAppStore's setCameraActive/loadVideo), so currentSourceFrame just
  // prefers this when set.
  private cameraVideoEl: HTMLVideoElement | null = null;

  constructor(canvas: HTMLCanvasElement) {
    this.canvas = canvas;
    const ctx = canvas.getContext("2d");
    if (!ctx) throw new Error("2D context unavailable");
    this.ctx = ctx;

    // El canvas 2D no espera automáticamente a que un webfont termine de
    // cargar: si fillText() se llama antes de que "JetBrains Mono" esté
    // lista, usa el fallback del sistema para ese frame y nunca vuelve a
    // redibujar solo porque la fuente llegó después. Se fuerza la carga acá.
    void document.fonts.load(`16px "JetBrains Mono"`);
  }

  setStatsListener(fn: (s: ViewportOverlayStats) => void) {
    this.onStats = fn;
  }

  /**
   * Provee (o quita, pasando null) los frames decodificados de un video para
   * usarlos como fuente de imagen en lugar de la escena sintética. El audio
   * nunca pasa por acá — solo frames ya extraídos como ImageBitmap.
   */
  setSourceFrames(frames: ImageBitmap[] | null) {
    this.sourceFrames = frames;
    this.sourceFrameIndex = 0;
    console.log(`[MockRenderer] setSourceFrames: ${frames?.length ?? 0} frames loaded`);
  }

  get hasSourceFrames() {
    return !!this.sourceFrames?.length;
  }

  setSourceFrameIndex(index: number) {
    if (!this.sourceFrames?.length) return;
    const n = this.sourceFrames.length;
    this.sourceFrameIndex = ((Math.floor(index) % n) + n) % n;
  }

  private get currentSourceFrame(): ImageBitmap | HTMLVideoElement | null {
    if (this.cameraVideoEl) return this.cameraVideoEl;
    if (!this.sourceFrames?.length) return null;
    return this.sourceFrames[this.sourceFrameIndex] ?? null;
  }

  /** ImageBitmap exposes .width/.height directly; HTMLVideoElement's
   * .width/.height instead reflect its (here, never-set) HTML layout
   * attributes — the actual frame size is .videoWidth/.videoHeight. Every
   * call site that used to read source.width/source.height directly needs
   * this now that currentSourceFrame can return either type. */
  private sourceDims(source: ImageBitmap | HTMLVideoElement): { width: number; height: number } {
    if (source instanceof HTMLVideoElement) {
      return { width: source.videoWidth, height: source.videoHeight };
    }
    return { width: source.width, height: source.height };
  }

  /** Provides (or clears, passing null) a live camera <video> element as
   * the image source, same role as setSourceFrames but for a live feed
   * instead of pre-decoded frames. */
  setCameraSource(videoEl: HTMLVideoElement | null) {
    this.cameraVideoEl = videoEl;
  }

  get hasCameraSource() {
    return !!this.cameraVideoEl;
  }

  /** Called once per rAF tick from ViewportCanvas's camera loop, mirroring
   * wasmBridge.pushCameraFrame's call site so that loop can drive whichever
   * backend is active without branching. There's nothing to actually push
   * here: the <video> element plays live and drawImage() always reads its
   * current frame directly (see currentSourceFrame) — unlike video-file
   * ImageBitmaps, a live video element needs no separate per-frame decode
   * step. This method exists purely so the call site doesn't need to know
   * that difference. */
  pushCameraFrame() {}

  private drawCover(ctx: CanvasRenderingContext2D, img: CanvasImageSource, sw: number, sh: number, w: number, h: number) {
    const scale = Math.max(w / sw, h / sh);
    const dw = sw * scale;
    const dh = sh * scale;
    ctx.drawImage(img, (w - dw) / 2, (h - dh) / 2, dw, dh);
  }

  setEffect(effect: EffectId, params: EffectParams) {
    this.effect = effect;
    this.params = params;
    if (effect === "particles") this.particles = [];
    if (effect === "ascii") {
      this.matrixStreams = [];
      this.matrixGlyphs = [];
      this.matrixBaseGlyphs = [];
      this.matrixCols = 0;
      this.matrixRows = 0;
      this.matrixColLumaPrev = [];
    }
  }

  setParams(params: EffectParams) {
    this.params = params;
  }

  start() {
    const loop = () => {
      this.tick();
      this.timerHandle = window.setTimeout(loop, MockRenderer.TICK_INTERVAL_MS);
    };
    this.timerHandle = window.setTimeout(loop, MockRenderer.TICK_INTERVAL_MS);
  }

  stop() {
    if (this.timerHandle !== null) {
      window.clearTimeout(this.timerHandle);
      this.timerHandle = null;
    }
  }

  private tick() {
    const now = performance.now();
    const dt = Math.min(0.05, (now - this.lastTime) / 1000);
    const gpuStart = performance.now();
    this.fps = 1 / Math.max(dt, 1 / 240);
    this.lastTime = now;
    this.frame++;

    const w = this.canvas.width;
    const h = this.canvas.height;

    switch (this.effect) {
      case "ascii":
        this.renderAscii(w, h, dt);
        break;
      case "particles":
        this.renderParticles(w, h, dt);
        break;
      case "crt":
        this.renderCrt(w, h, now);
        break;
      case "opencv":
        this.renderOpencv(w, h, now);
        break;
    }

    const gpuFrameTimeMs = performance.now() - gpuStart;
    this.onStats?.({
      fps: Math.round(this.fps),
      resolutionW: w,
      resolutionH: h,
      frame: this.frame,
      effect: this.effect,
      gpuFrameTimeMs: Math.round(gpuFrameTimeMs * 100) / 100,
    });
  }

  /**
   * Full-bleed placeholder scene: covers the entire canvas resolution edge-to-edge
   * with translucent animated wave bands, instead of a shape confined to the center.
   */
  private paintWaveScene(
    ctx: CanvasRenderingContext2D,
    w: number,
    h: number,
    t: number,
    bandColor: string,
  ) {
    const bands = 5;
    const steps = 32;
    ctx.save();
    ctx.fillStyle = bandColor;
    for (let i = 0; i < bands; i++) {
      const phase = t * 0.0002 + i * 1.3;
      const baseY = h * ((i + 0.5) / bands);
      const amp = h * (0.06 + i * 0.01);
      ctx.globalAlpha = 0.22 - i * 0.03;
      ctx.beginPath();
      ctx.moveTo(0, h);
      ctx.lineTo(0, baseY + Math.sin(phase) * amp);
      for (let s = 1; s <= steps; s++) {
        const x = (w * s) / steps;
        const y = baseY + Math.sin(x * 0.008 + phase) * amp;
        ctx.lineTo(x, y);
      }
      ctx.lineTo(w, h);
      ctx.closePath();
      ctx.fill();
    }
    ctx.restore();
    ctx.globalAlpha = 1;
  }

  private baseScene(w: number, h: number, t: number) {
    const ctx = this.ctx;
    const source = this.currentSourceFrame;
    if (source) {
      ctx.fillStyle = "#000000";
      ctx.fillRect(0, 0, w, h);
      const { width, height } = this.sourceDims(source);
      this.drawCover(ctx, source, width, height, w, h);
      return;
    }
    const grad = ctx.createLinearGradient(0, 0, w, h);
    grad.addColorStop(0, "#1b3a44");
    grad.addColorStop(1, "#0b0b0e");
    ctx.fillStyle = grad;
    ctx.fillRect(0, 0, w, h);
    this.paintWaveScene(ctx, w, h, t, "#44D4FF");
  }

  private renderAscii(w: number, h: number, dt: number) {
    const ctx = this.ctx;
    const mode = String(this.params.mode ?? "normal");
    const fontSize = Number(this.params.fontSize ?? 10);
    const bg = String(this.params.background ?? "#0B0B0E00");

    const cols = Math.max(1, Math.floor(w / fontSize));
    const rows = Math.max(1, Math.floor(h / fontSize));

    if (!this.asciiOffCanvas || this.asciiOffW !== cols || this.asciiOffH !== rows) {
      this.asciiOffCanvas = document.createElement("canvas");
      this.asciiOffCanvas.width = cols;
      this.asciiOffCanvas.height = rows;
      this.asciiOffCtx = this.asciiOffCanvas.getContext("2d")!;
      this.asciiOffW = cols;
      this.asciiOffH = rows;
    }
    const octx = this.asciiOffCtx!;
    const source = this.currentSourceFrame;
    
    if (source) {
      octx.fillStyle = "#000000";
      octx.fillRect(0, 0, cols, rows);
      const { width, height } = this.sourceDims(source);
      this.drawCover(octx, source, width, height, cols, rows);
    } else {
      const t = performance.now();
      const grad = octx.createLinearGradient(0, 0, cols, rows);
      grad.addColorStop(0, "#3a3a3a");
      grad.addColorStop(1, "#000000");
      octx.fillStyle = grad;
      octx.fillRect(0, 0, cols, rows);
      this.paintWaveScene(octx, cols, rows, t, "#ffffff");
    }

    const img = octx.getImageData(0, 0, cols, rows).data;

    ctx.clearRect(0, 0, w, h);
    
    // En modo matrix el video NUNCA se dibuja en crudo: como en la película,
    // el "video" se reconstruye enteramente con la brillantez/densidad de
    // los glifos (ver renderAsciiMatrix). Dibujar el frame crudo detrás del
    // texto es lo que hacía que, sin importar density/color/velocidad, se
    // viera como un video con rayitas encima en vez de un pasillo hecho de
    // caracteres.
    if (mode === "matrix") {
      ctx.fillStyle = "#000000";
      ctx.fillRect(0, 0, w, h);
    } else {
      ctx.fillStyle = bg;
      ctx.fillRect(0, 0, w, h);
    }
    
    ctx.font = `${fontSize}px ${ASCII_FONT_STACK}`;
    ctx.textBaseline = "top";

    if (mode === "matrix") {
      this.renderAsciiMatrix(dt, cols, rows, fontSize, img);
      return;
    }

    const ramp = String(this.params.characters ?? " .:-=+*#%@");
    const brightness = Number(this.params.brightness ?? 0.8);
    const contrast = Number(this.params.contrast ?? 1.2);
    const gamma = Number(this.params.gamma ?? 1.1);
    const fg = String(this.params.foreground ?? "#44D4FF");
    const invert = Boolean(this.params.invert ?? false);
    ctx.fillStyle = fg;

    for (let y = 0; y < rows; y++) {
      let line = "";
      for (let x = 0; x < cols; x++) {
        const idx = (y * cols + x) * 4;
        let lum = (img[idx] + img[idx + 1] + img[idx + 2]) / (3 * 255);
        lum = Math.pow(lum, 1 / Math.max(0.01, gamma));
        lum = (lum - 0.5) * contrast + 0.5 + (brightness - 1);
        lum = Math.min(1, Math.max(0, lum));
        if (invert) lum = 1 - lum;
        const charIdx = Math.floor(lum * (ramp.length - 1));
        line += ramp[charIdx] ?? " ";
      }
      ctx.fillText(line, 0, y * fontSize);
    }
  }

  private renderAsciiMatrix(
    dt: number,
    cols: number,
    rows: number,
    fontSize: number,
    img: Uint8ClampedArray,
  ) {
    const ctx = this.ctx;
    
    const charsRaw = String(
      this.params.matrixChars ?? "0123456789ABCDEF",
    );
    const glyphPool = Array.from(charsRaw).filter((c) => c !== " ");
    const direction = String(this.params.matrixDirection ?? "down");
    const speed = Number(this.params.matrixSpeed ?? 10);
    const density = Number(this.params.matrixDensity ?? 0.85);
    const trailLength = Math.max(2, Math.round(Number(this.params.matrixTrailLength ?? 16)));
    const headColor = String(this.params.matrixHeadColor ?? "#CFFFE0");
    const fg = String(this.params.foreground ?? "#44D4FF");
    const reactive = Boolean(this.params.matrixReactive ?? true);
    const reactivityMode = String(this.params.matrixReactivityMode ?? "speed");
    const reactiveStrength = Number(this.params.matrixReactiveStrength ?? 0.8);
    const imageStrength = Math.max(0, Number(this.params.matrixImageStrength ?? 1.3));
    const wantBoth = direction === "both";

    if (this.matrixCols !== cols || this.matrixRows !== rows) {
      this.matrixCols = cols;
      this.matrixRows = rows;
      this.matrixGlyphs = new Array(cols * rows).fill(" ");
      this.matrixBaseGlyphs = new Array(cols * rows)
        .fill(" ")
        .map(() => (glyphPool.length ? glyphPool[Math.floor(Math.random() * glyphPool.length)] : " "));
      this.matrixStreams = [];
      for (let c = 0; c < cols; c++) {
        this.matrixStreams.push({ col: c, slot: 0, head: 0, dir: 1, speed: 0, active: false });
        this.matrixStreams.push({ col: c, slot: 1, head: 0, dir: -1, speed: 0, active: false });
      }
      this.matrixColLumaPrev = new Array(cols).fill(0);
    }

    const colLuma: number[] = new Array(cols).fill(0);
    for (let x = 0; x < cols; x++) {
      let sum = 0;
      for (let y = 0; y < rows; y++) {
        const idx = (y * cols + x) * 4;
        sum += (img[idx] + img[idx + 1] + img[idx + 2]) / (3 * 255);
      }
      colLuma[x] = sum / rows;
    }

    const colMotion: number[] = new Array(cols).fill(0);
    if (this.matrixColLumaPrev.length === cols) {
      for (let x = 0; x < cols; x++) {
        colMotion[x] = Math.min(1, Math.abs(colLuma[x] - this.matrixColLumaPrev[x]) * 6);
      }
    }
    this.matrixColLumaPrev = colLuma;

    // --- Capa base: reconstruye la imagen fuente con la propia densidad/
    // brillo de los glifos, en vez de mostrar el video en crudo. Esto es lo
    // que en la referencia hace que se note claramente el pasillo, la gente
    // y los cables: cada celda dibuja SIEMPRE un carácter, más brillante
    // donde el frame es más claro y casi negro donde es oscuro. Por encima
    // van las corrientes de código animadas (loop de abajo), más brillantes,
    // dando el efecto de "rain" clásico sobre la imagen reconocible.
    const [fgR, fgG, fgB] = hexToRgb(fg);
    // Invariante respecto a x/y: antes se reasignaba (con un template string
    // nuevo) en cada una de las cols*rows iteraciones de abajo, forzando al
    // motor a re-parsear el color como CSS miles de veces por frame para
    // nada — el valor nunca cambia dentro de este bucle. Se calcula una
    // sola vez aquí.
    const baseGlyphStyle = `rgb(${Math.round(fgR * 0.7)},${Math.round(fgG * 0.7)},${Math.round(fgB * 0.7)})`;
    ctx.fillStyle = baseGlyphStyle;
    for (let y = 0; y < rows; y++) {
      for (let x = 0; x < cols; x++) {
        const idx = y * cols + x;
        const pIdx = idx * 4;
        const luma = (img[pIdx] + img[pIdx + 1] + img[pIdx + 2]) / (3 * 255);

        // shimmer sutil: una fracción chica de celdas muta su glifo cada
        // frame, como el parpadeo de fondo real del código de la película.
        if (glyphPool.length && Math.random() < 0.06 * dt * (speed / 10)) {
          this.matrixBaseGlyphs[idx] = glyphPool[Math.floor(Math.random() * glyphPool.length)];
        }

        const alpha = Math.min(1, Math.max(0.04, luma * imageStrength));
        ctx.globalAlpha = alpha;
        ctx.fillText(this.matrixBaseGlyphs[idx] ?? " ", x * fontSize, y * fontSize);
      }
    }
    ctx.globalAlpha = 1;

    for (const st of this.matrixStreams) {
      if (st.slot === 1 && !wantBoth) {
        st.active = false;
        continue;
      }
      const dir: 1 | -1 = wantBoth ? (st.slot === 0 ? 1 : -1) : direction === "up" ? -1 : 1;
      st.dir = dir;

      const luma = (reactive && reactivityMode !== "off") ? colLuma[st.col] : 0;
      const motion = (reactive && reactivityMode !== "off") ? colMotion[st.col] : 0;
      
      // Calcular boost solo si mode permite (speed, all)
      const useSpeedReactivity = reactivityMode === "speed" || reactivityMode === "all";
      const boost = useSpeedReactivity ? (1 + reactiveStrength * (luma * 0.5 + motion * 1.5)) : 1;

      if (!st.active) {
        // Para density o all mode: más spawn en zonas claras
        let spawnDensity = density;
        if ((reactivityMode === "density" || reactivityMode === "all") && reactive) {
          spawnDensity = density * (0.3 + luma * 1.4); // Hasta 1.7x más denso en claros
        }
        
        if (Math.random() < spawnDensity * dt * 0.6) {
          st.head = dir > 0 ? -Math.random() * trailLength : rows - 1 + Math.random() * trailLength;
          st.speed = speed * (0.6 + Math.random() * 0.8);
          st.active = true;
        } else {
          continue;
        }
      } else {
        st.head += dir * st.speed * boost * dt;
        const outOfBounds = dir > 0 ? st.head - trailLength > rows : st.head + trailLength < -1;
        if (outOfBounds) {
          st.active = false;
          continue;
        }
      }

      for (let k = 0; k < trailLength; k++) {
        const row = Math.round(st.head - dir * k);
        if (row < 0 || row >= rows) continue;
        const idx = row * cols + st.col;
        if (k === 0 || Math.random() < 0.12) {
          this.matrixGlyphs[idx] = glyphPool.length
            ? glyphPool[Math.floor(Math.random() * glyphPool.length)]
            : " ";
        }
        const fadeLin = 1 - k / trailLength;
        const fade = fadeLin * fadeLin;
        ctx.globalAlpha = k === 0 ? 1 : Math.max(0, fade);
        
        // Para colors o all mode: variar color según luminancia, siempre
        // partiendo de los colores que el usuario eligió (antes esto usaba
        // #CFFFE0/#44D4FF fijos sin importar qué se pusiera en Head Color /
        // Foreground Color, por eso cambiar el color no se notaba).
        let charColor = k === 0 ? headColor : fg;
        if ((reactivityMode === "colors" || reactivityMode === "all") && reactive) {
          const brightness = k === 0 ? Math.max(0.5, luma) : luma * 0.8;
          const headRGB = hexToRgb(headColor);
          const trailRGB = hexToRgb(fg);

          if (k === 0) {
            // Head color: más brillante en claros
            const r = Math.round(headRGB[0] * brightness);
            const g = Math.round(headRGB[1] * brightness);
            const b = Math.round(headRGB[2] * brightness);
            charColor = `rgb(${r},${g},${b})`;
          } else {
            // Trail: más saturation en claros
            const r = Math.round(trailRGB[0] * brightness);
            const g = Math.round(trailRGB[1] * brightness);
            const b = Math.round(trailRGB[2] * brightness);
            charColor = `rgb(${r},${g},${b})`;
          }
        }
        
        ctx.fillStyle = charColor;
        ctx.fillText(this.matrixGlyphs[idx], st.col * fontSize, row * fontSize);
      }
    }
    ctx.globalAlpha = 1;
  }

  private renderParticles(w: number, h: number, dt: number) {
    const ctx = this.ctx;
    const count = Number(this.params.count ?? 2000);
    const spawnRate = Number(this.params.spawnRate ?? 120);
    const gravity = Number(this.params.gravity ?? 9.8);
    const lifetime = Number(this.params.lifetime ?? 2.5);
    const size = Number(this.params.size ?? 4);
    const sizeFalloff = Number(this.params.sizeFalloff ?? 0.6);
    const color = String(this.params.color ?? "#44D4FF");
    const spread = (Number(this.params.spread ?? 45) * Math.PI) / 180;
    const spawnX = Number(this.params.spawnX ?? 0.5);
    const spawnY = Number(this.params.spawnY ?? 0.8);

    const toSpawn = Math.min(count - this.particles.length, Math.round(spawnRate * dt));
    for (let i = 0; i < toSpawn; i++) {
      const angle = -Math.PI / 2 + (Math.random() - 0.5) * spread;
      const speed = 60 + Math.random() * 120;
      this.particles.push({
        x: w * spawnX,
        y: h * spawnY,
        vx: Math.cos(angle) * speed,
        vy: Math.sin(angle) * speed,
        age: 0,
        lifetime: lifetime * (0.6 + Math.random() * 0.4),
      });
    }

    const source = this.currentSourceFrame;
    if (source) {
      ctx.fillStyle = "#000000";
      ctx.fillRect(0, 0, w, h);
      const { width, height } = this.sourceDims(source);
      this.drawCover(ctx, source, width, height, w, h);
      ctx.fillStyle = "rgba(11,11,14,0.25)";
      ctx.fillRect(0, 0, w, h);
    } else {
      ctx.fillStyle = "#0b0b0e";
      ctx.fillRect(0, 0, w, h);
    }

    this.particles = this.particles.filter((p) => p.age < p.lifetime);
    ctx.fillStyle = color;
    for (const p of this.particles) {
      p.age += dt;
      p.vy += gravity * dt * 20;
      p.x += p.vx * dt;
      p.y += p.vy * dt;
      const lifeRatio = 1 - p.age / p.lifetime;
      const r = Math.max(0.2, size * (1 - sizeFalloff * (1 - lifeRatio)));
      ctx.globalAlpha = Math.max(0, lifeRatio);
      ctx.beginPath();
      ctx.arc(p.x, p.y, r, 0, Math.PI * 2);
      ctx.fill();
    }
    ctx.globalAlpha = 1;
    if (this.particles.length > count) this.particles.length = count;
  }

  private renderCrt(w: number, h: number, t: number) {
    const ctx = this.ctx;
    this.baseScene(w, h, t);

    const scanlineIntensity = Number(this.params.scanlineIntensity ?? 0.35);
    const scanlineCount = Number(this.params.scanlineCount ?? 480);
    const scanlineSpeed = Number(this.params.scanlineSpeed ?? 0);
    const noise = Number(this.params.noise ?? 0.05);
    const vignette = Number(this.params.vignette ?? 0.3);
    const flicker = Number(this.params.flicker ?? 0.1);
    const aberration = Number(this.params.chromaticAberration ?? 0.4);

    // crude chromatic aberration via offset composite copies
    if (aberration > 0.01) {
      const snapshot = ctx.getImageData(0, 0, w, h);
      ctx.globalCompositeOperation = "screen";
      ctx.globalAlpha = 0.5;
      ctx.putImageData(snapshot, 0, 0);
      ctx.drawImage(this.canvas, -aberration, 0);
      ctx.drawImage(this.canvas, aberration, 0);
      ctx.globalCompositeOperation = "source-over";
      ctx.globalAlpha = 1;
    }

    // scanlines — scroll offset mirrors the shader's `uv.y - uTime * speed * 0.2`
    // (fraction of screen height per second), just expressed in pixels here.
    const step = Math.max(1, h / scanlineCount);
    const scrollPx = (((t / 1000) * scanlineSpeed * 0.2 * h) % (step * 2) + step * 2) % (step * 2);
    ctx.fillStyle = `rgba(0,0,0,${scanlineIntensity})`;
    for (let y = -step * 2; y < h + step * 2; y += step * 2) {
      ctx.fillRect(0, y + scrollPx, w, step);
    }

    // noise
    if (noise > 0.001) {
      const n = Math.floor(noise * 400);
      ctx.fillStyle = "rgba(255,255,255,0.5)";
      for (let i = 0; i < n; i++) {
        ctx.globalAlpha = Math.random() * noise;
        ctx.fillRect(Math.random() * w, Math.random() * h, 1, 1);
      }
      ctx.globalAlpha = 1;
    }

    // vignette — darkens toward each edge independently (rectangular falloff)
    // instead of a centered radial halo, so the effect reads as covering the
    // full canvas resolution rather than a glowing circle in the middle.
    if (vignette > 0.001) {
      ctx.save();
      const edges: Array<[number, number, number, number]> = [
        [0, 0, w * 0.35, 0], // left
        [w, 0, w * 0.65, 0], // right
        [0, 0, 0, h * 0.35], // top
        [0, h, 0, h * 0.65], // bottom
      ];
      for (const [x0, y0, x1, y1] of edges) {
        const edgeGrad = ctx.createLinearGradient(x0, y0, x1, y1);
        edgeGrad.addColorStop(0, `rgba(0,0,0,${vignette})`);
        edgeGrad.addColorStop(1, "rgba(0,0,0,0)");
        ctx.fillStyle = edgeGrad;
        ctx.fillRect(0, 0, w, h);
      }
      ctx.restore();
    }

    // flicker
    if (flicker > 0.001) {
      ctx.fillStyle = `rgba(255,255,255,${Math.random() * flicker * 0.15})`;
      ctx.fillRect(0, 0, w, h);
    }
  }

  /**
   * The actual OpenCV pipelines (edges/contours/optical flow/background
   * subtraction/face detection) only exist natively in
   * native/effects/opencv_effect.h's implementation section — reimplementing
   * five CV algorithms in JS canvas2D just for this dev-only fallback isn't
   * worth the maintenance burden of a second implementation that could
   * drift from the real one.
   * This shows the raw source (video file or camera) plus a label, so
   * switching to this effect in mock mode is clearly a "preview the wasm
   * build to see this effect" state rather than silently doing nothing.
   */
  private renderOpencv(w: number, h: number, t: number) {
    const ctx = this.ctx;
    this.baseScene(w, h, t);

    const label = "OpenCV Vision — build/run the WASM renderer to see this effect";
    ctx.save();
    ctx.font = `${Math.max(12, Math.round(w * 0.016))}px ${ASCII_FONT_STACK}`;
    ctx.textBaseline = "bottom";
    const paddingX = 12;
    const paddingY = 10;
    const metrics = ctx.measureText(label);
    ctx.fillStyle = "rgba(11,11,14,0.65)";
    ctx.fillRect(0, h - metrics.actualBoundingBoxAscent - paddingY * 2, metrics.width + paddingX * 2, metrics.actualBoundingBoxAscent + paddingY * 2);
    ctx.fillStyle = "#44D4FF";
    ctx.fillText(label, paddingX, h - paddingY);
    ctx.restore();
  }
}
