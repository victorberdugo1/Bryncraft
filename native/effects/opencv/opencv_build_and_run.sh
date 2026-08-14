#!/usr/bin/env bash
set -e

# ============================================================
# Configuration
# ============================================================

OPENCV_DIR="opencv-local"
OPENCV_VERSION="4.8.0"

# raylib.h / libraylib.a live one level up, in native/effects/
if [ -f "raylib.h" ]; then
    RAYLIB_INC="."
    RAYLIB_LIB="."
else
    RAYLIB_INC=".."
    RAYLIB_LIB=".."
fi

# ============================================================
# Check/download OpenCV
# ============================================================

if [ -f "$OPENCV_DIR/include/opencv4/opencv2/core.hpp" ]; then
    echo "OpenCV already exists: $OPENCV_DIR"
else
    echo "OpenCV not found."

    command -v git >/dev/null 2>&1 || {
        echo "git not found."
        echo "Please install git or download OpenCV manually."
        exit 1
    }

    echo "Downloading OpenCV $OPENCV_VERSION..."

    rm -rf opencv-src
    rm -rf "$OPENCV_DIR"

    git clone \
        --depth 1 \
        --branch "$OPENCV_VERSION" \
        https://github.com/opencv/opencv.git \
        opencv-src

    echo "Building OpenCV..."

    mkdir -p opencv-src/build
    cd opencv-src/build

    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$OLDPWD/$OPENCV_DIR" \
        -DBUILD_LIST=core,imgproc,video,videoio,objdetect \
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
fi

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
# OpenCV paths
# ============================================================

OPENCV_INC="$OPENCV_DIR/include/opencv4"
OPENCV_LIB="$OPENCV_DIR/lib"

if [ ! -f "$OPENCV_INC/opencv2/core.hpp" ]; then
    echo "OpenCV headers not found."
    exit 1
fi

echo "Using OpenCV:"
echo "  Include: $OPENCV_INC"
echo "  Library: $OPENCV_LIB"

# ============================================================
# Compile
# ============================================================

gcc \
    -c main003.c \
    -o main003.o \
    -I"$RAYLIB_INC"

g++ \
    -DOPENCV_EFFECT_IMPLEMENTATION \
    -x c++ \
    -c opencv_effect.h \
    -o opencv_effect.o \
    -I"$RAYLIB_INC" \
    -I"$OPENCV_INC" \
    -std=c++20

# ============================================================
# Link
# ============================================================

g++ \
    main003.o \
    opencv_effect.o \
    -o opencv_demo \
    -L"$RAYLIB_LIB" \
    -L"$OPENCV_LIB" \
    -Wl,-rpath,"$OPENCV_LIB" \
    -lraylib \
    -lopencv_objdetect \
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

rm -f main003.o opencv_effect.o

# ============================================================
# Run
# ============================================================

./opencv_demo
