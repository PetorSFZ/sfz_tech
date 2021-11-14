# Dependency - SDL2
This is a distribution and CMake wrapper of the SDL2 library (available from https://www.libsdl.org/). It bundles the official pre-built SDL2 binaries (https://libsdl.org/download-2.0.php) for Windows in order to make it easier to build. Uses `FindSDL2.cmake` module (originally found in Twinklebear-Dev SDL tutorial: `https://github.com/Twinklebear/TwinklebearDev-Lessons/blob/master/cmake/FindSDL2.cmake`) on other platforms.

# Usage

Three CMake variables are returned:

`${SDL2_FOUND}`: Variable that signals whether SDL2 is available or not.

`${SDL2_INCLUDE_DIRS}`: The headers you need to include with `include_directories()` or similar.

`${SDL2_LIBRARIES}`: The libraries you need to link your to with `target_link_libraries()`.

`${SDL2_RUNTIME_FILES}`: The path to runtime files. On Windows this is the `SDL2.dll`.
