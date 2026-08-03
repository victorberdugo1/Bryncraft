import * as React from "react";
import * as DialogPrimitive from "@radix-ui/react-dialog";
import { X } from "lucide-react";
import { cn } from "@/lib/utils";
import { useVisualViewportHeight } from "@/hooks/useVisualViewportHeight";

export const Dialog = DialogPrimitive.Root;
export const DialogTrigger = DialogPrimitive.Trigger;

export const DialogContent = React.forwardRef<
  React.ElementRef<typeof DialogPrimitive.Content>,
  React.ComponentPropsWithoutRef<typeof DialogPrimitive.Content>
>(({ className, children, ...props }, ref) => {
  // See useVisualViewportHeight's own comment for why: on Chrome for
  // Android, this app's full-screen `overflow: hidden` html/body means
  // Chrome's address bar never collapses via the normal scroll-driven
  // path, so vh/dvh and fixed-position math can both end up sized against
  // more height than is actually visible — cutting off the dialog's
  // bottom. Measuring window.visualViewport directly and using that as
  // this wrapper's real height sidesteps it: that API always reflects
  // what's actually on screen, independent of whether the page scrolls.
  const viewportHeight = useVisualViewportHeight();

  return (
    <DialogPrimitive.Portal>
      <DialogPrimitive.Overlay className="fixed inset-0 z-50 bg-black/60 animate-fade-in" />
      <div
        className="fixed left-0 right-0 top-0 z-50 flex items-center justify-center overflow-y-auto p-4"
        style={{ height: viewportHeight }}
      >
        <DialogPrimitive.Content
          ref={ref}
          className={cn(
            "relative my-auto w-full max-w-lg rounded-lg border border-border bg-panel p-5 shadow-floating animate-slide-up",
            className
          )}
          {...props}
        >
          {children}
          <DialogPrimitive.Close className="absolute right-3 top-3 rounded-sm text-muted-foreground hover:text-foreground">
            <X className="h-4 w-4" />
          </DialogPrimitive.Close>
        </DialogPrimitive.Content>
      </div>
    </DialogPrimitive.Portal>
  );
});
DialogContent.displayName = "DialogContent";

export const DialogTitle = DialogPrimitive.Title;
export const DialogDescription = DialogPrimitive.Description;
