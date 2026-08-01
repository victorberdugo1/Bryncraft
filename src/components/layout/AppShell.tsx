import { useState } from "react";
import { TopBar } from "@/components/layout/TopBar";
import { LeftSidebar } from "@/components/layout/LeftSidebar";
import { RightInspector } from "@/components/layout/RightInspector";
import { CenterViewport } from "@/components/layout/CenterViewport";
import { BottomTimeline } from "@/components/layout/BottomTimeline";
import { CodePanel } from "@/components/layout/CodePanel";
import { ResizeHandle } from "@/components/layout/ResizeHandle";
import { MobileDockShell } from "@/components/layout/MobileDockShell";
import { useAppStore } from "@/store/useAppStore";
import { useMediaQuery } from "@/hooks/useMediaQuery";
import { clamp } from "@/lib/utils";

export function AppShell() {
  // Below this width we switch to MobileDockShell (phones and tablets): a
  // full-screen canvas with panels tucked behind edge tabs. At/above it we
  // keep the classic resizable multi-pane desktop layout. Deciding this in
  // JS — instead of rendering both layouts and CSS-hiding one — means
  // CenterViewport (and therefore ViewportCanvas, its <canvas>, and its
  // WASM/mock renderer attachment) only ever mounts once.
  const isDesktop = useMediaQuery("(min-width: 1024px)");

  const [leftWidth, setLeftWidth] = useState(224);
  const [rightWidth, setRightWidth] = useState(288);
  const [codeHeight, setCodeHeight] = useState(220);

  const leftSidebarOpen = useAppStore((s) => s.leftSidebarOpen);
  const rightPanelOpen = useAppStore((s) => s.rightPanelOpen);

  return (
    <div className="flex h-screen w-screen flex-col overflow-hidden font-ui text-[13px]">
      {/* Hidden on mobile/tablet on purpose: the whole screen is the canvas
          there, panels live behind the edge tabs in MobileDockShell. */}
      {isDesktop && <TopBar />}

      {/* Desktop / large tablet docking layout */}
      {isDesktop && (
        <div className="flex min-h-0 flex-1">
          {leftSidebarOpen && (
            <>
              <div style={{ width: leftWidth }} className="shrink-0">
                <LeftSidebar />
              </div>
              <ResizeHandle orientation="vertical" onResize={(d) => setLeftWidth((w) => clamp(w + d, 180, 420))} />
            </>
          )}

          <div className="flex min-h-0 min-w-0 flex-1 flex-col">
            <div className="min-h-0 flex-1">
              <CenterViewport />
            </div>
            <ResizeHandle
              orientation="horizontal"
              onResize={(d) => setCodeHeight((h) => clamp(h - d, 120, 480))}
            />
            <div style={{ height: codeHeight }} className="shrink-0 border-t border-border">
              <CodePanel />
            </div>
            <BottomTimeline />
          </div>

          {rightPanelOpen && (
            <>
              <ResizeHandle orientation="vertical" onResize={(d) => setRightWidth((w) => clamp(w - d, 220, 480))} />
              <div style={{ width: rightWidth }} className="shrink-0">
                <RightInspector />
              </div>
            </>
          )}
        </div>
      )}

      {/* Mobile / tablet: full-screen canvas with edge-docked panels */}
      {!isDesktop && <MobileDockShell />}
    </div>
  );
}
