import { useEffect, useState } from "react";

/**
 * Tracks the *actual* visible viewport height using the VisualViewport API.
 *
 * Why not just `100dvh`: dvh (and vh) are usually enough, but on Chrome for
 * Android specifically, pages that set `overflow: hidden` on html/body (as
 * this app does, to stay full-screen) never trigger the browser's normal
 * scroll-driven toolbar collapse. Chrome ends up leaving its address bar
 * "expanded" indefinitely and doesn't reliably recompute the CSS viewport
 * units or `fixed`-position layout against the space that's actually still
 * visible under it — so a dialog centered with dvh/vh math can end up with
 * its bottom edge hidden behind Chrome's own UI even though the numbers on
 * paper looked fine. `window.visualViewport`, by contrast, always reports
 * the real visible height regardless of whether the page itself scrolls,
 * which is exactly the case this needs to cover.
 *
 * Falls back to `window.innerHeight` on browsers without the API (older
 * Safari/Firefox), which is fine there since those don't have this bug.
 */
export function useVisualViewportHeight(): number {
  const [height, setHeight] = useState(
    () => window.visualViewport?.height ?? window.innerHeight
  );

  useEffect(() => {
    const vv = window.visualViewport;
    if (!vv) return;

    const update = () => setHeight(vv.height);
    update();

    // 'resize' fires when the toolbar/keyboard shows or hides; 'scroll'
    // covers the pinch-zoom/pan case where the visual viewport moves
    // without technically resizing.
    vv.addEventListener("resize", update);
    vv.addEventListener("scroll", update);
    return () => {
      vv.removeEventListener("resize", update);
      vv.removeEventListener("scroll", update);
    };
  }, []);

  return height;
}
