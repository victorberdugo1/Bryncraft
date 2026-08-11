import { useEffect, useRef } from "react";
import type { EffectId } from "@/types/effects";
import { cn } from "@/lib/utils";
import { THUMBNAILS } from "@/effects";

interface EffectThumbnailProps {
  effect: EffectId;
  active: boolean;
}

// Lightweight, self-contained animated preview — independent from the main
// viewport renderer so switching effects in the sidebar never touches the
// live preview state. El dibujo de cada efecto vive en su propio archivo
// bajo ./thumbnails/ (ver ./thumbnails/index.ts) — este componente solo
// arma el <canvas>, el loop de requestAnimationFrame, y llama a la función
// registrada para el efecto activo.
export function EffectThumbnail({ effect, active }: EffectThumbnailProps) {
  const canvasRef = useRef<HTMLCanvasElement>(null);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext("2d");
    if (!ctx) return;
    let raf = 0;
    let t = 0;

    const drawEffect = THUMBNAILS[effect];

    const draw = () => {
      t += 0.02;
      const w = canvas.width;
      const h = canvas.height;
      ctx.fillStyle = "#0b0b0e";
      ctx.fillRect(0, 0, w, h);

      drawEffect(ctx, w, h, t);

      raf = requestAnimationFrame(draw);
    };
    raf = requestAnimationFrame(draw);
    return () => cancelAnimationFrame(raf);
  }, [effect]);

  return (
    <canvas
      ref={canvasRef}
      width={140}
      height={78}
      className={cn(
        "w-full rounded-md border transition-colors",
        active ? "border-accent shadow-[0_0_0_1px_#44D4FF]" : "border-border"
      )}
    />
  );
}
