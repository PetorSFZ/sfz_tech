#!/bin/bash

# Set sfzCore main directory as working directory
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR/..

# Delete old build directory and create new one
rm -rf build_make_debug
mkdir build_make_debug
cd build_make_debug

# Generate build files
cmake .. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug
