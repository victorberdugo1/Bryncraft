#!/usr/bin/env bash
set -e

pkg-config --exists opencv4 || { echo "opencv4 not found: sudo apt install libopencv-dev"; exit 1; }
pkg-config --atleast-version=4.7 opencv4 || { echo "opencv4 >= 4.7 required (cv::dnn::blobFromImageWithParams) -- found $(pkg-config --modversion opencv4)"; exit 1; }

INC="-I.. -I../ascii -I../crt -I../opencv"

gcc -c main.c -o main.o $INC
g++ -DOPENCV_EFFECT_IMPLEMENTATION -x c++ -c opencv_effect.h -o opencv_effect.o $INC -std=c++20 $(pkg-config --cflags opencv4)
g++ -DTOUCHDESIGNER_EFFECT_IMPLEMENTATION -x c++ -c touchdesigner_effect.h -o touchdesigner_effect.o $INC -std=c++20 $(pkg-config --cflags opencv4)
g++ main.o opencv_effect.o touchdesigner_effect.o -o touchdesigner_demo -L.. -lraylib -lm -lpthread -ldl -lrt -lX11 $(pkg-config --libs opencv4)
rm -f main.o opencv_effect.o touchdesigner_effect.o

./touchdesigner_demo
