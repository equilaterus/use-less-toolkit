#!/bin/bash
#set echo off
echo "Building UseLessToolkit..."
echo "Cleaning..."
rm -f ult

echo "Compiling..."
flags="-std=c++11 -stdlib=libstdc++ `pkg-config --cflags glfw3 freetype2`"
debugFlags="-D DEBUG -g"
profilingFlags="-D PROFILING=1 -fno-exceptions -fno-rtti -pedantic -Wall -Werror -pthreads -fno-omit-frame-pointer"
imguidir="imgui"
imguifreetype="imgui/misc/freetype"
lib="-lGL -lfreetype `pkg-config --static --libs glfw3 `"
clang++ $flags $debugFlags main.cpp -o ult $lib -I$imguidir -I$imguidir/backends -I$imguifreetype
echo "Done"
