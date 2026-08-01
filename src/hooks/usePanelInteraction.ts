import { useCallback, useRef, useState } from "react";

/**
 * Tracks whether the user is actively touching/dragging a control (slider,
 * switch, etc.) inside a panel. While `active` is true the panel should turn
 * near-transparent so the canvas underneath is visible through it — the
 * moment the user lets go, `active` stays true for a short "settle" window
 * (mirrors how a phone's brightness/volume OSD lingers for a beat before
 * fading back) and only then flips back to false, so the panel eases back to
 * its normal, readable state instead of snapping.
 */
export function usePanelInteraction(settleMs = 450) {
  const [active, setActive] = useState(false);
  const settleTimer = useRef<number | null>(null);

  const clearSettleTimer = () => {
    if (settleTimer.current !== null) {
      window.clearTimeout(settleTimer.current);
      settleTimer.current = null;
    }
  };

  const onPointerDown = useCallback(() => {
    clearSettleTimer();
    setActive(true);
  }, []);

  const onPointerUp = useCallback(() => {
    clearSettleTimer();
    settleTimer.current = window.setTimeout(() => setActive(false), settleMs);
  }, [settleMs]);

  return {
    active,
    handlers: {
      onPointerDown,
      onPointerUp,
      onPointerCancel: onPointerUp,
    },
  };
}
