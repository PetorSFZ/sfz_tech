#!/bin/bash

# Set sfzCore main directory as working directory
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR/..

# Delete old build directory and create new one
rm -rf build_xcode_ios
mkdir build_xcode_ios
cd build_xcode_ios

# Generate build files
cmake .. -GXcode -DCMAKE_TOOLCHAIN_FILE=$POLLY_ROOT/ios.cmake
