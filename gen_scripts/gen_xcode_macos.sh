#!/bin/bash

# Set sfzCore main directory as working directory
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR/..

# Delete old build directory and create new one
rm -rf build_xcode_macos
mkdir build_xcode_macos
cd build_xcode_macos

# Generate build files
cmake .. -GXcode
