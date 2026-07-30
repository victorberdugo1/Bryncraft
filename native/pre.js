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

/*
 ============================================================================
 Silences "WebGL: INVALID_ENUM: getFramebufferAttachmentParameter: invalid
 parameter name" console spam.

 raylib's rlgl (compiled here for GRAPHICS_API_OPENGL_ES2, i.e. a WebGL1
 context — this Makefile never sets MIN_WEBGL_VERSION/USE_WEBGL2) queries
 framebuffer-attachment params every frame while managing g_sceneTarget's
 RenderTexture. Some of those pnames only exist in WebGL2 (e.g.
 FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE, ..._COLOR_ENCODING); asking a WebGL1
 context about them is a spec-valid INVALID_ENUM, harmless to the render
 (rlgl already tolerates the failed query), but Chrome logs every single
 occurrence, which floods the console until the browser gives up entirely
 ("too many errors, no more errors will be reported").

 This can't be caught with try/catch — the browser logs to console the
 moment it detects the bad enum, before the call even returns — so the only
 way to stop the message is to never let the call reach the real
 implementation at all. rlgl already tolerates this query failing (that's
 exactly why the render has been fine all along, even with the browser
 logging an error every time) — the browser's own fallback return value for
 a failed query is null, so returning null unconditionally, without ever
 calling through, is behavior-identical from rlgl's point of view. No need
 to guess whether it's target/attachment/pname that's invalid.
 ============================================================================
*/
(function () {
    'use strict';
    function patch(proto) {
        if (!proto || !proto.getFramebufferAttachmentParameter) return;
        proto.getFramebufferAttachmentParameter = function () {
            return null;
        };
    }

    patch(window.WebGLRenderingContext && window.WebGLRenderingContext.prototype);
    patch(window.WebGL2RenderingContext && window.WebGL2RenderingContext.prototype);
})();
