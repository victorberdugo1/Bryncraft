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

const LEFT_WIDTH_MIN = 180;
const RIGHT_WIDTH_MIN = 220;
const CODE_HEIGHT_MIN = 120;

export function AppShell() {
  const isDesktop = useMediaQuery("(min-width: 1024px)");

  const [leftWidth, setLeftWidth] = useState(LEFT_WIDTH_MIN);
  const [rightWidth, setRightWidth] = useState(RIGHT_WIDTH_MIN);
  const [codeHeight, setCodeHeight] = useState(CODE_HEIGHT_MIN);

  const leftSidebarOpen = useAppStore((s) => s.leftSidebarOpen);
  const rightPanelOpen = useAppStore((s) => s.rightPanelOpen);

  return (
    <div className="flex h-screen w-screen flex-col overflow-hidden font-ui text-[13px]">
      {isDesktop && <TopBar />}

      {isDesktop && (
        <div className="flex min-h-0 flex-1">
          {leftSidebarOpen && (
            <>
              <div style={{ width: leftWidth }} className="shrink-0">
                <LeftSidebar />
              </div>
              <ResizeHandle
                orientation="vertical"
                onResize={(d) => setLeftWidth((w) => clamp(w + d, LEFT_WIDTH_MIN, 420))}
              />
            </>
          )}

          <div className="flex min-h-0 min-w-0 flex-1 flex-col">
            <div className="min-h-0 flex-1">
              <CenterViewport />
            </div>
            <ResizeHandle
              orientation="horizontal"
              onResize={(d) => setCodeHeight((h) => clamp(h - d, CODE_HEIGHT_MIN, 480))}
            />
            <div style={{ height: codeHeight }} className="shrink-0 border-t border-border">
              <CodePanel />
            </div>
            <BottomTimeline />
          </div>

          {rightPanelOpen && (
            <>
              <ResizeHandle
                orientation="vertical"
                onResize={(d) => setRightWidth((w) => clamp(w - d, RIGHT_WIDTH_MIN, 480))}
              />
              <div style={{ width: rightWidth }} className="shrink-0">
                <RightInspector />
              </div>
            </>
          )}
        </div>
      )}

      {!isDesktop && <MobileDockShell />}
    </div>
  );
}
