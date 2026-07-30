# Makefile - build WASM (emscripten + raylib) for the Bryncraft renderer.
# Adapted from the chess_viewer reference build; same emsdk/raylib/ffmpeg
# bootstrap, different source list and JS export surface.

EMSDK_DIR  ?= emsdk
EMSDK_ENV  := $(EMSDK_DIR)/emsdk_env.sh
EMSDK_EMCC := $(EMSDK_DIR)/upstream/emscripten/emcc

RAYLIB_SRC      ?= raylib/src
RAYLIB_REPO_DIR := $(patsubst %/src,%,$(RAYLIB_SRC))
RAYLIB_LIB_SRC  := $(RAYLIB_SRC)/libraylib.web.a
RAYLIB_LIB      := libraylib.a
RAYLIB_CONFIG   := $(RAYLIB_SRC)/config.h

# === OpenCV (wasm static libs, for the "opencv" effect) ===
# Built from source with the Emscripten toolchain via CMake — NOT the
# embind-wrapped opencv.js bundle (platforms/js/build_js.py); we want plain
# static libs to link directly into opencv_bridge.cpp. Trimmed to only the
# modules the effect actually uses (core/imgproc/video/objdetect) via
# BUILD_LIST, with codecs/GUI/hardware backends we never touch switched
# off — that's most of what keeps this from being a multi-hour build.
OPENCV_VERSION     := 4.9.0
OPENCV_SRC_DIR     := opencv
OPENCV_BUILD_DIR   := opencv_build
OPENCV_INSTALL_DIR := $(OPENCV_BUILD_DIR)/install
OPENCV_INCLUDE     := $(OPENCV_INSTALL_DIR)/include/opencv4
OPENCV_LIB_DIR     := $(OPENCV_INSTALL_DIR)/lib
OPENCV_3RDPARTY_DIR:= $(OPENCV_INSTALL_DIR)/lib/opencv4/3rdparty
OPENCV_MODULES     := core,imgproc,video,objdetect
# Marker file used as the "already built" check, same idea as
# RAYLIB_LIB_SRC above for raylib.
OPENCV_MARKER      := $(OPENCV_LIB_DIR)/libopencv_core.a

# === FFmpeg.wasm Configuration ===
FFMPEG_VERSION        := 0.12.10
FFMPEG_DIR            := ffmpeg
FFMPEG_WRAPPER_FILES  := ffmpeg.min.js ffmpeg.js
FFMPEG_CORE_FILES     := ffmpeg-core.js ffmpeg-core.wasm
FFMPEG_FILES          := $(FFMPEG_WRAPPER_FILES) $(FFMPEG_CORE_FILES)
FFMPEG_TARGETS        := $(addprefix $(FFMPEG_DIR)/,$(FFMPEG_FILES))
FFMPEG_WRAPPER_URL    := https://cdn.jsdelivr.net/npm/@ffmpeg/ffmpeg@$(FFMPEG_VERSION)/dist/umd
FFMPEG_CORE_URL       := https://cdn.jsdelivr.net/npm/@ffmpeg/core@$(FFMPEG_VERSION)/dist/umd

# === Compilation ===
EMCC := emcc
EMXX := em++

TARGET   := index
OUT_HTML := $(TARGET).html
OUT_JS   := $(TARGET).js
OUT_WASM := $(TARGET).wasm
OUT_DATA := $(TARGET).data
OUTPUTS  := $(OUT_HTML) $(OUT_JS) $(OUT_WASM) $(OUT_DATA)

SRC_C   := main.c json_mini.c
SRC_CXX := opencv_bridge.cpp
SRC     := $(SRC_C) $(SRC_CXX)
OBJ     := $(SRC_C:.c=.o) $(SRC_CXX:.cpp=.o)
ASSETS_DIR := assets
SHELL_FILE := shell.html
VIDEO_EXPORT := video_export_emscripten.js
# Todos los headers de effects/ (ascii_effect.h, crt_effect.h, etc.) y todo
# archivo bajo assets/, empaquetado por --preload-file. Sin esto, $(OUT_HTML)
# más abajo solo dependía de main.c/json_mini.c/effect_common.h: tocar
# ascii_effect.h no invalidaba el binario ya compilado, y un
# `make all` local (sin `make clean` de por medio) podía servir el .wasm
# viejo sin avisar que el cambio nunca se compiló.
EFFECT_HEADERS := $(wildcard effects/*.h)
ASSET_FILES := $(shell find $(ASSETS_DIR) -type f 2>/dev/null)

# Functions/runtime methods React's wasmBridge.ts calls via Module.ccall(...)
EXPORTED_FUNCS    := ['_js_set_effect_json','_js_get_stats_json','_js_start_export','_js_stop_export','_js_set_video_frame','_js_clear_video_frame','_js_set_canvas_size','_js_lock_frame_capture','_js_unlock_frame_capture','_js_get_presented_frame_count','_malloc','_free','_main']
EXPORTED_RUNTIME  := ['ccall','cwrap','HEAPU8']

DEST ?= ../public/wasm
PRE_JS := pre.js

CFLAGS   := -Os -Wall -DPLATFORM_WEB -I. -I$(RAYLIB_SRC) -I$(OPENCV_INCLUDE)
CXXFLAGS := $(CFLAGS) -std=c++17
LDFLAGS := ./$(RAYLIB_LIB) \
           -s USE_GLFW=3 \
           -s GL_ENABLE_GET_PROC_ADDRESS=1 \
           -s ASYNCIFY \
           -s TOTAL_MEMORY=134217728 \
           -s FORCE_FILESYSTEM=1 \
           -s EXPORTED_FUNCTIONS="$(EXPORTED_FUNCS)" \
           -s EXPORTED_RUNTIME_METHODS="$(EXPORTED_RUNTIME)" \
           --preload-file $(ASSETS_DIR) \
           --pre-js $(PRE_JS) \
           --shell-file $(SHELL_FILE)

# `=` (not `:=`): this must re-expand at link time, AFTER `make opencv` has
# run within the same `make all` invocation — with `:=` the $(shell find...)
# would run once when the Makefile is first parsed, before OPENCV_LIB_DIR
# has anything in it, and silently bake in an empty library list.
# --start-group/--end-group lets wasm-ld resolve the static libs' mutual
# dependencies regardless of link order (objdetect <-> video <-> imgproc
# <-> core all reference each other).
CV_LIBS = -Wl,--start-group $(shell find $(OPENCV_LIB_DIR) $(OPENCV_3RDPARTY_DIR) -name '*.a' 2>/dev/null) -Wl,--end-group

# === Utility Functions ===
define WITH_EMSDK
bash -c 'source "$(EMSDK_ENV)" >/dev/null 2>&1 && $(1)'
endef

# === Commands ===
CLONE_EMSDK   = test -d "$(EMSDK_DIR)" || git clone --depth 1 https://github.com/emscripten-core/emsdk.git "$(EMSDK_DIR)"
INSTALL_EMSDK = test -x "$(EMSDK_EMCC)" || (cd "$(EMSDK_DIR)" && ./emsdk install latest && ./emsdk activate latest)
CLONE_RAYLIB  = test -d "$(RAYLIB_SRC)" || git clone --depth 1 https://github.com/raysan5/raylib.git "$(RAYLIB_REPO_DIR)"
COPY_RAYLIB   = cp -f "$(RAYLIB_LIB_SRC)" "./$(RAYLIB_LIB)"
CLONE_OPENCV  = test -d "$(OPENCV_SRC_DIR)" || git clone --depth 1 --branch $(OPENCV_VERSION) https://github.com/opencv/opencv.git "$(OPENCV_SRC_DIR)"

RM_OUT        = rm -f $(OUTPUTS) $(OBJ)
RM_LIB        = rm -f "./$(RAYLIB_LIB)"
RM_RAYLIB_DIR = rm -rf "$(RAYLIB_REPO_DIR)"
RM_EMSDK_DIR  = rm -rf "$(EMSDK_DIR)"
RM_FFMPEG_DIR = rm -rf "$(FFMPEG_DIR)"
RM_OPENCV_DIR = rm -rf "$(OPENCV_SRC_DIR)" "$(OPENCV_BUILD_DIR)"

.PHONY: all emsdk raylib opencv ffmpeg run copy clean fclean emsdk-clean ffmpeg-clean opencv-clean help

help:
	@echo ""
	@echo "Procedural VFX Atelier - native renderer Makefile"
	@echo "=============================================="
	@echo ""
	@echo "  make all          - Build everything (emsdk, raylib, ffmpeg, renderer)"
	@echo "  make run          - Build and serve on http://localhost:8000"
	@echo "  make copy         - Copy build output into ../public/wasm (or DEST)"
	@echo ""
	@echo "  make emsdk        - Install emscripten"
	@echo "  make raylib       - Build raylib for PLATFORM_WEB"
	@echo "  make opencv       - Build OpenCV (core/imgproc/video/objdetect) for wasm"
	@echo "  make ffmpeg       - Fetch precompiled FFmpeg.wasm"
	@echo ""
	@echo "  make clean        - Remove build outputs"
	@echo "  make fclean       - clean + remove raylib (keeps emsdk, opencv)"
	@echo "  make opencv-clean - Remove the OpenCV source + build tree"
	@echo ""

all: $(OUT_HTML)

# ============================================================
# EMSCRIPTEN SDK
# ============================================================

emsdk:
	@echo "[emsdk] Cloning if needed..."
	@$(CLONE_EMSDK)
	@echo "[emsdk] Installing if needed..."
	@$(INSTALL_EMSDK)
	@echo "OK emsdk ready in $(EMSDK_DIR)"

# ============================================================
# RAYLIB
# ============================================================

raylib: emsdk
	@echo "[raylib] Cloning if needed..."
	@$(CLONE_RAYLIB)
	@echo "[raylib] Building libraylib.a for PLATFORM_WEB..."
	@test -f "$(RAYLIB_LIB_SRC)" || $(call WITH_EMSDK,emmake $(MAKE) -C "$(RAYLIB_SRC)" PLATFORM=PLATFORM_WEB -j4)
	@$(COPY_RAYLIB)
	@echo "OK libraylib.a ready (PLATFORM_WEB)"

# ============================================================
# OPENCV
# ============================================================
# First run downloads + builds OpenCV for wasm — this is genuinely heavy
# (comparable to the raylib build, but bigger) since it's compiling a real
# C++ library, not just fetching a binary like ffmpeg. Subsequent runs are
# instant: like raylib, it's skipped once $(OPENCV_MARKER) exists.
#
# CV_ENABLE_INTRINSICS=OFF / CPU_BASELINE="" / CPU_DISPATCH="": OpenCV's
# WASM SIMD128 universal-intrinsics header (hal/intrin_wasm.hpp) is broken
# against one emscripten/clang version or another almost every release —
# there are multiple long-standing upstream issues on exactly this
# (opencv/opencv#18097, #26536; emscripten-core/emscripten#9340) with no
# single OpenCV/emsdk pin that's reliably compatible. Turning off the
# intrinsics dispatch entirely sidesteps that whole fragile path — a real
# speed cost, but we already run at reduced processScale with per-mode
# frame throttling (see opencv_bridge.cpp), so correctness/build-stability
# matters more here than squeezing out SIMD.

opencv: emsdk
	@echo "[opencv] Cloning if needed (this is a ~500MB checkout)..."
	@$(CLONE_OPENCV)
	@mkdir -p "$(OPENCV_BUILD_DIR)"
	@echo "[opencv] Configuring (modules: $(OPENCV_MODULES))..."
	@test -f "$(OPENCV_MARKER)" || $(call WITH_EMSDK,cd "$(OPENCV_BUILD_DIR)" && emcmake cmake "../$(OPENCV_SRC_DIR)" \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_INSTALL_PREFIX=install \
		-DBUILD_LIST=$(OPENCV_MODULES) \
		-DBUILD_SHARED_LIBS=OFF \
		-DBUILD_opencv_apps=OFF -DBUILD_EXAMPLES=OFF -DBUILD_TESTS=OFF -DBUILD_PERF_TESTS=OFF -DBUILD_DOCS=OFF \
		-DBUILD_JAVA=OFF -DBUILD_opencv_python2=OFF -DBUILD_opencv_python3=OFF \
		-DWITH_1394=OFF -DWITH_FFMPEG=OFF -DWITH_GSTREAMER=OFF -DWITH_GTK=OFF -DWITH_QT=OFF -DWITH_OPENGL=OFF \
		-DWITH_PNG=OFF -DWITH_JPEG=OFF -DWITH_TIFF=OFF -DWITH_WEBP=OFF -DWITH_OPENEXR=OFF -DWITH_V4L=OFF \
		-DWITH_ITT=OFF -DWITH_IPP=OFF -DWITH_TBB=OFF -DWITH_OPENCL=OFF -DWITH_PROTOBUF=OFF \
		-DCV_ENABLE_INTRINSICS=OFF -DCPU_BASELINE="" -DCPU_DISPATCH="" \
		-DENABLE_PRECOMPILED_HEADERS=OFF)
	@echo "[opencv] Building (this can take a long time on first run)..."
	@test -f "$(OPENCV_MARKER)" || $(call WITH_EMSDK,cd "$(OPENCV_BUILD_DIR)" && emmake make -j4 && emmake make install)
	@echo "OK OpenCV wasm libs ready in $(OPENCV_INSTALL_DIR)"

# ============================================================
# FFMPEG.WASM
# ============================================================

define DOWNLOAD_WRAPPER
	@echo "  downloading $(1) (wrapper)..." && \
	mkdir -p $(FFMPEG_DIR) && \
	curl -fsSL "$(FFMPEG_WRAPPER_URL)/$(1)" -o "$(FFMPEG_DIR)/$(1)" || \
	(echo "  fallback to GitHub release..." && \
	curl -fsSL "https://github.com/ffmpegwasm/ffmpeg.wasm/releases/download/v$(FFMPEG_VERSION)/$(1)" -o "$(FFMPEG_DIR)/$(1)")
endef

define DOWNLOAD_CORE
	@echo "  downloading $(1) (core)..." && \
	mkdir -p $(FFMPEG_DIR) && \
	curl -fsSL "$(FFMPEG_CORE_URL)/$(1)" -o "$(FFMPEG_DIR)/$(1)" || \
	(echo "  fallback to GitHub release..." && \
	curl -fsSL "https://github.com/ffmpegwasm/ffmpeg.wasm/releases/download/v$(FFMPEG_VERSION)/$(1)" -o "$(FFMPEG_DIR)/$(1)")
endef

$(FFMPEG_DIR)/ffmpeg.min.js:
	$(call DOWNLOAD_WRAPPER,ffmpeg.min.js)

$(FFMPEG_DIR)/ffmpeg.js:
	$(call DOWNLOAD_WRAPPER,ffmpeg.js)

$(FFMPEG_DIR)/ffmpeg-core.js:
	$(call DOWNLOAD_CORE,ffmpeg-core.js)

$(FFMPEG_DIR)/ffmpeg-core.wasm:
	$(call DOWNLOAD_CORE,ffmpeg-core.wasm)

ffmpeg: $(FFMPEG_TARGETS)
	@echo "OK FFmpeg.wasm v$(FFMPEG_VERSION) ready in $(FFMPEG_DIR)/"

# ============================================================
# RENDERER COMPILATION
# ============================================================

check: raylib ffmpeg opencv

# Compile phase — kept as separate objects (instead of the previous
# single "emcc $(SRC) -o $(OUT_HTML)" one-shot) because opencv_bridge.cpp
# needs a C++ frontend and the rest of the sources don't. Each source only
# rebuilds when it, a shared header, or (for opencv_bridge.cpp) OpenCV
# itself changes.
%.o: %.c json_mini.h effects/effect_common.h $(EFFECT_HEADERS)
	@echo "[build] Compiling $< (C)..."
	$(call WITH_EMSDK,$(EMCC) $(CFLAGS) -c $< -o $@)

%.o: %.cpp json_mini.h effects/effect_common.h $(EFFECT_HEADERS)
	@echo "[build] Compiling $< (C++)..."
	$(call WITH_EMSDK,$(EMXX) $(CXXFLAGS) -c $< -o $@)

# Order-only prerequisite: guarantees `check` (raylib/ffmpeg/opencv) has
# fully run before any $(OBJ) is built, even under `make -jN`, where
# sibling prerequisites of $(OUT_HTML) could otherwise be attempted
# concurrently — opencv_bridge.o would then fail with "no rule to make
# target opencv_build/install/lib/libopencv_core.a" the first time (no
# rule directly produces that exact path; it only exists as a side effect
# of the `opencv` recipe). Order-only (the `|`) avoids forcing every
# object to rebuild on every invocation just because `check` is phony.
$(OBJ): | check

# Link phase — em++ (not emcc) because linking OpenCV's C++ static libs
# needs the C++ standard library/runtime; em++ links plain-C objects just
# fine alongside them, so main.o/json_mini.o are untouched.
$(OUT_HTML): check $(OBJ) $(ASSET_FILES) $(SHELL_FILE) $(PRE_JS)
	@echo "[build] Linking Bryncraft renderer..."
	$(call WITH_EMSDK,$(EMXX) $(OBJ) -o $(OUT_HTML) $(LDFLAGS) $(CV_LIBS))
	@echo "OK build complete."
	@echo "Next: make run, or make copy to publish into ../public/wasm"

# ============================================================
# RUN & DEPLOY
# ============================================================

run: all
	@echo "Serving on http://localhost:8000"
	python3 -m http.server 8000

copy: all
	@echo "[copy] Copying build output to $(DEST)..."
	@mkdir -p "$(DEST)"
	@cp $(OUT_HTML) $(OUT_JS) $(OUT_WASM) "$(DEST)/" 2>/dev/null || true
	@cp -f $(VIDEO_EXPORT) "$(DEST)/video_export.js" 2>/dev/null || true
	@if [ -f $(OUT_DATA) ]; then cp $(OUT_DATA) "$(DEST)/"; fi
	@cp -f $(OUT_JS) "$(DEST)/index.js" 2>/dev/null || true
	@echo "OK copied. wasmBridge.ts probes $(DEST)/index.js"

# ============================================================
# CLEANUP
# ============================================================

clean:
	$(RM_OUT)

fclean: clean
	$(RM_LIB)
	$(RM_RAYLIB_DIR)

ffmpeg-clean:
	$(RM_FFMPEG_DIR)

emsdk-clean:
	$(RM_EMSDK_DIR)

opencv-clean:
	$(RM_OPENCV_DIR)

.DEFAULT_GOAL := help