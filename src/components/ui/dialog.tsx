import * as React from "react";
import * as DialogPrimitive from "@radix-ui/react-dialog";
import { X } from "lucide-react";
import { cn } from "@/lib/utils";

export const Dialog = DialogPrimitive.Root;
export const DialogTrigger = DialogPrimitive.Trigger;

export const DialogContent = React.forwardRef<
  React.ElementRef<typeof DialogPrimitive.Content>,
  React.ComponentPropsWithoutRef<typeof DialogPrimitive.Content>
>(({ className, children, ...props }, ref) => (
  <DialogPrimitive.Portal>
    <DialogPrimitive.Overlay className="fixed inset-0 z-50 bg-black/60 animate-fade-in" />
    {/*
      Centers via flexbox on a full-viewport wrapper instead of the usual
      top-1/2 + -translate-y-1/2 trick. That trick computes its position
      against the *layout* viewport, and on Chrome for Android that
      calculation isn't reliably redone when the browser's own toolbar
      shows/hides — so the dialog could end up centered against a taller
      viewport than what's actually visible, cutting off its bottom edge
      behind Chrome's UI (this didn't show up on desktop/iOS Safari, which
      don't have the same recalculation gap). Flex-centering + the wrapper
      itself scrolling sidesteps that: it centers against whatever height
      the browser reports *right now*, no transform math involved.
    */}
    <div className="fixed inset-0 z-50 flex items-center justify-center overflow-y-auto p-4">
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
));
DialogContent.displayName = "DialogContent";

export const DialogTitle = DialogPrimitive.Title;
export const DialogDescription = DialogPrimitive.Description;
