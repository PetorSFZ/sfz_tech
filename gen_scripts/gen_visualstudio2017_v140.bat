: Run script in its own context
setlocal

: Set working directory to location of this file
pushd %~dp0

: Set working directory to root of project
cd ..

: Create build_msvc2017_v140 directory (and delete old one if it exists)
rmdir /S /Q build_msvc2017_v140
mkdir build_msvc2017_v140
cd build_msvc2017_v140

: Generate Visual Studio solution
cmake .. -G "Visual Studio 15 2017 Win64" -T v140
