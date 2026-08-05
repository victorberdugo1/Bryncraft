#!/usr/bin/env bash
set -e

pkg-config --exists opencv4 || { echo "opencv4 not found: sudo apt install libopencv-dev"; exit 1; }

# raylib.h / libraylib.a live one level up, in native/effects/ (shared by
# every effect's standalone demo).
gcc -c main003.c -o main003.o -I..
g++ -DOPENCV_EFFECT_IMPLEMENTATION -x c++ -c opencv_effect.h -o opencv_effect.o -I.. -std=c++20 $(pkg-config --cflags opencv4)
g++ main003.o opencv_effect.o -o opencv_demo -L.. -lraylib -lm -lpthread -ldl -lrt -lX11 $(pkg-config --libs opencv4)
rm -f main003.o opencv_effect.o

./opencv_demo
