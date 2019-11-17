#!/bin/bash

# Set sfzCore main directory as working directory
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR/..

# Delete old build directory and create new one
rm -rf build_emscripten_debug
mkdir build_emscripten_debug
cd build_emscripten_debug

# Generate build files
cmake .. -DCMAKE_TOOLCHAIN_FILE="$EMSCRIPTEN/cmake/Modules/Platform/Emscripten.cmake" -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug
