/*
 ============================================================================
 Forces every WebGL context this page creates to keep its drawing buffer
 (preserveDrawingBuffer true) instead of letting the browser auto-clear it
 after compositing.

 This is the actual root cause of the empty PNG frame bug: by default
 (preserveDrawingBuffer false), the browser is allowed to clear the WebGL
 backbuffer any time after a frame is presented — timing that is entirely
 internal to the browser/GPU compositor and invisible to JS. video_export.js
 reads the canvas via toDataURL() or toBlob() from a separate JS tick than the
 one that drew the frame, so no amount of JS-side waiting, polling, or
 locking can guarantee the read lands before that auto-clear. Some frames
 come back blank no matter how carefully the JS side is timed.

 With preserveDrawingBuffer true, the buffer keeps whatever was last drawn
 until the next real draw call, so a read at any later point is guaranteed
 to see that last frame's actual pixels.

 Must run BEFORE GLFW/Emscripten creates the context (i.e. before
 InitWindow() executes in main.c) — which is exactly when --pre-js content
 runs, ahead of the rest of the generated glue code.
 ============================================================================
*/
(function () {
    'use strict';
    var originalGetContext = HTMLCanvasElement.prototype.getContext;
    HTMLCanvasElement.prototype.getContext = function (type, attributes) {
        if (type === 'webgl' || type === 'webgl2' || type === 'experimental-webgl') {
            attributes = attributes || {};
            attributes.preserveDrawingBuffer = true;
        }
        return originalGetContext.call(this, type, attributes);
    };
})();