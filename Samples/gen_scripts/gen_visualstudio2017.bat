: Run script in its own context
setlocal

: Set working directory to location of this file
pushd %~dp0

: Set working directory to root of project
cd ..

: Create build_msvc2017 directory (and delete old one if it exists)
rmdir /S /Q build_msvc2017
mkdir build_msvc2017
cd build_msvc2017

: Generate Visual Studio solution
cmake .. -G "Visual Studio 15 2017 Win64" -T host=x64

: Create resources symlinks
mklink /D res\ ..\res

mkdir Debug
cd Debug
mklink /D res\ ..\..\res
cd ..

mkdir RelWithDebInfo
cd RelWithDebInfo
mklink /D res\ ..\..\res
cd ..

mkdir Release
cd Release
mklink /D res\ ..\..\res
cd ..

: Pause if started from Windows GUI
if %0 == "%~0" pause
