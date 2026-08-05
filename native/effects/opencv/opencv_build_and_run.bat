@echo off
setlocal enabledelayedexpansion

if exist "opencv-mingw\include\opencv2\core.hpp" goto :SETVARS

where git >nul 2>nul || (echo git not found: https://git-scm.com/download/win & pause & exit /b 1)
if exist "opencv-mingw-src" rmdir /s /q opencv-mingw-src
git clone --filter=blob:none --no-checkout --depth 1 https://github.com/puccj/opencv-mingwx64.git opencv-mingw-src || (pause & exit /b 1)
pushd opencv-mingw-src
git sparse-checkout init --cone
git sparse-checkout set opencv-480/install
git checkout main
popd
if not exist "opencv-mingw-src\opencv-480\install\include\opencv2\core.hpp" (echo download failed & pause & exit /b 1)
move opencv-mingw-src\opencv-480\install opencv-mingw >nul
rmdir /s /q opencv-mingw-src

:SETVARS
rem raylib.h / libraylib.a live one level up, in native\effects\ (shared by
rem every effect's standalone demo).
set OPENCV_PATH=opencv-mingw
set PATH=%CD%\..\;%CD%\!OPENCV_PATH!\x64\mingw\bin;%PATH%
set INC=-I.. -I"!OPENCV_PATH!\include"
set LIBS=-L.. -L"!OPENCV_PATH!\x64\mingw\lib" -lraylib -lm -lgdi32 -lwinmm -lopencv_objdetect480 -lopencv_video480 -lopencv_videoio480 -lopencv_imgproc480 -lopencv_core480

gcc -c main003.c -o main003.o -I.. || (pause & exit /b 1)
g++ -DOPENCV_EFFECT_IMPLEMENTATION -x c++ -c opencv_effect.h -o opencv_effect.o %INC% -std=c++20 || (pause & exit /b 1)
g++ main003.o opencv_effect.o -o opencv_demo.exe %LIBS% || (pause & exit /b 1)
del main003.o opencv_effect.o 2>nul

opencv_demo.exe
pause
