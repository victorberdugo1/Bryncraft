#!/usr/bin/env bash
set -e

# ============================================================
# Configuration
# ============================================================

OPENCV_DIR="opencv-local"
OPENCV_VERSION="4.8.0"

# raylib.h / libraylib.a
if [ -f "raylib.h" ]; then
    RAYLIB_INC="."
    RAYLIB_LIB="."
else
    RAYLIB_INC=".."
    RAYLIB_LIB=".."
fi

# ============================================================
# Download / build OpenCV
# ============================================================

if [ -f "$OPENCV_DIR/include/opencv4/opencv2/core.hpp" ]; then
    echo "[opencv] OpenCV already available in $OPENCV_DIR"
else
    echo "[opencv] OpenCV not found -- downloading $OPENCV_VERSION"

    command -v git >/dev/null 2>&1 || {
        echo "git not found."
        exit 1
    }

    command -v cmake >/dev/null 2>&1 || {
        echo "cmake not found."
        echo "Install cmake or provide a prebuilt OpenCV."
        exit 1
    }

    command -v g++ >/dev/null 2>&1 || {
        echo "g++ not found."
        exit 1
    }

    rm -rf opencv-src
    rm -rf "$OPENCV_DIR"

    git clone \
        --depth 1 \
        --branch "$OPENCV_VERSION" \
        https://github.com/opencv/opencv.git \
        opencv-src

    mkdir -p opencv-src/build

    cd opencv-src/build

    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$OLDPWD/$OPENCV_DIR" \
        -DBUILD_LIST=core,imgproc,video,videoio,objdetect,dnn \
        -DBUILD_TESTS=OFF \
        -DBUILD_PERF_TESTS=OFF \
        -DBUILD_EXAMPLES=OFF \
        -DBUILD_opencv_apps=OFF \
        -DWITH_GTK=OFF \
        -DWITH_QT=OFF

    cmake --build . -j"$(nproc)"
    cmake --install .

    cd "$OLDPWD"

    rm -rf opencv-src

    echo "[opencv] OpenCV installed in $OPENCV_DIR"
fi

# ============================================================
# Check OpenCV version
# ============================================================

if [ ! -f "$OPENCV_DIR/include/opencv4/opencv2/core.hpp" ]; then
    echo "[opencv] OpenCV headers not found."
    exit 1
fi

echo "[opencv] Using local OpenCV $OPENCV_VERSION"

# ============================================================
# Check raylib
# ============================================================

if [ ! -f "$RAYLIB_INC/raylib.h" ]; then
    echo "raylib.h not found in . or .."
    exit 1
fi

if [ ! -f "$RAYLIB_LIB/libraylib.a" ]; then
    echo "libraylib.a not found in . or .."
    exit 1
fi

# ============================================================
# Optional effects
# ============================================================

EXTRA_DEFS=""
EXTRA_INC=""
EXTRA_OBJS=""

# -------------------------
# ASCII
# -------------------------

if [ -f "ascii_effect.h" ]; then
    ASCII_INC="."
elif [ -f "../ascii/ascii_effect.h" ]; then
    ASCII_INC="../ascii"
fi

if [ -n "${ASCII_INC:-}" ]; then
    echo "[ascii] enabled -- ascii_effect.h found in $ASCII_INC"
    EXTRA_DEFS="$EXTRA_DEFS -DTD_ENABLE_ASCII"
    EXTRA_INC="$EXTRA_INC -I$ASCII_INC"
else
    echo "[ascii] disabled -- ascii_effect.h not found"
fi

# -------------------------
# CRT
# -------------------------

if [ -f "crt_effect.h" ]; then
    CRT_INC="."
elif [ -f "../crt/crt_effect.h" ]; then
    CRT_INC="../crt"
fi

if [ -n "${CRT_INC:-}" ]; then
    echo "[crt] enabled -- crt_effect.h found in $CRT_INC"
    EXTRA_DEFS="$EXTRA_DEFS -DTD_ENABLE_CRT"
    EXTRA_INC="$EXTRA_INC -I$CRT_INC"
else
    echo "[crt] disabled -- crt_effect.h not found"
fi

# -------------------------
# OpenCV edges
# -------------------------

if [ -f "opencv_effect.h" ]; then
    OCV_HDR_INC="."
elif [ -f "../opencv/opencv_effect.h" ]; then
    OCV_HDR_INC="../opencv"
fi

if [ -n "${OCV_HDR_INC:-}" ]; then
    echo "[edges] enabled -- opencv_effect.h found in $OCV_HDR_INC"

    EXTRA_DEFS="$EXTRA_DEFS -DTD_ENABLE_OPENCV_EDGES"
    EXTRA_INC="$EXTRA_INC -I$OCV_HDR_INC"

    EXTRA_OBJS="opencv_effect.o"
else
    echo "[edges] disabled -- opencv_effect.h not found"
fi

# ============================================================
# Paths
# ============================================================

OPENCV_INC="$OPENCV_DIR/include/opencv4"
OPENCV_LIB="$OPENCV_DIR/lib"

INC="-I$RAYLIB_INC -I$OPENCV_INC $EXTRA_INC"

# ============================================================
# Compile main
# ============================================================

echo "[build] compiling main004.c"

gcc \
    -c main004.c \
    -o main004.o \
    $INC \
    $EXTRA_DEFS

# ============================================================
# Compile TouchDesigner effect
# ============================================================

echo "[build] compiling touchdesigner_effect.h"

g++ \
    -DTOUCHDESIGNER_EFFECT_IMPLEMENTATION \
    -DTOUCHDESIGNER_EFFECT_OBJ_BUILD \
    -x c++ \
    -c touchdesigner_effect.h \
    -o touchdesigner_effect.o \
    $INC \
    -std=c++20

# ============================================================
# Compile OpenCV effect if available
# ============================================================

if [ -n "${OCV_HDR_INC:-}" ]; then
    echo "[build] compiling OpenCV effect"

    g++ \
        -DOPENCV_EFFECT_IMPLEMENTATION \
        -x c++ \
        -c "$OCV_HDR_INC/opencv_effect.h" \
        -o opencv_effect.o \
        $INC \
        -std=c++20
fi

# ============================================================
# Link
# ============================================================

echo "[build] linking touchdesigner_demo"

g++ \
    main004.o \
    touchdesigner_effect.o \
    $EXTRA_OBJS \
    -o touchdesigner_demo \
    -L"$RAYLIB_LIB" \
    -L"$OPENCV_LIB" \
    -Wl,-rpath,"$OPENCV_LIB" \
    -lraylib \
    -lopencv_dnn \
    -lopencv_video \
    -lopencv_videoio \
    -lopencv_imgproc \
    -lopencv_core \
    -lm \
    -lpthread \
    -ldl \
    -lrt \
    -lX11

# ============================================================
# Cleanup
# ============================================================

rm -f \
    main004.o \
    touchdesigner_effect.o \
    opencv_effect.o

# ============================================================
# Run
# ============================================================

./touchdesigner_demo
