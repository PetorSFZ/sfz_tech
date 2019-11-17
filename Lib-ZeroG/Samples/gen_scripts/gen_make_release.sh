#!/bin/bash

# Set PhantasyTestbed main directory as working directory
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR/..

# Delete old build directory and create new one
rm -rf build_make_release
mkdir build_make_release
cd build_make_release

# Generate build files
cmake .. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release
