: Run script in its own context
setlocal

: Set working directory to location of this file
pushd %~dp0

: Set working directory to root of project
cd ..

: Create build_vs2019 directory (and delete old one if it exists)
rmdir /S /Q build_vs2019
mkdir build_vs2019
cd build_vs2019

: Generate Visual Studio solution
cmake .. -G "Visual Studio 16 2019" -A x64 -T host=x64

: Pause if started from Windows GUI
if %0 == "%~0" pause
