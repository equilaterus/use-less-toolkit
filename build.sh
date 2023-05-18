#!/bin/bash
set echo off
echo "Building UseLessToolkit..."
echo "Cleaning..."
rm -rf bin

echo "Compiling..."
mkdir bin
flags="-std=c++11 -stdlib=libstdc++ `pkg-config --cflags glfw3 `"
debugFlags="-D DEBUG -g"
profilingFlags="-D PROFILING=1 -fno-exceptions -fno-rtti -pedantic -Wall -Werror -pthreads -fno-omit-frame-pointer"
imguidir="imgui"
lib="-lGL `pkg-config --static --libs glfw3`"
clang++ $flags $debugFlags main.cpp -o bin/ult $lib -I$imguidir -I$imguidir/backends
echo "Done"
