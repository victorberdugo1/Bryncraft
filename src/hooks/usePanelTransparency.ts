import { createContext, useContext } from "react";

/**
 * Set by EdgeDock (mobile/tablet only) to true while the user is actively
 * dragging a control inside an open panel. The panels themselves
 * (LeftSidebar, RightInspector, CodePanel, BottomTimeline) each paint their
 * own solid `bg-panel` background — without this, that opaque background
 * would sit on top of EdgeDock's fade and hide the canvas underneath no
 * matter how transparent the drawer wrapper itself becomes. Reading this
 * context lets each panel swap to a transparent background at the exact
 * same moment. Defaults to false, so on desktop (no provider) every panel
 * renders exactly as it always has.
 */
export const PanelTransparencyContext = createContext(false);

export function usePanelTransparentBg() {
  return useContext(PanelTransparencyContext);
}
