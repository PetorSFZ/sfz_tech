# SDL2 CMake wrapper

Wrapper used to link and include SDL2 in a project. Bundles official pre-built SDL2 binaries (https://libsdl.org/download-2.0.php) for Windows in order to make it easier to build. Uses `FindSDL2.cmake` module (originally found in Twinklebear-Dev SDL tutorial: `https://github.com/Twinklebear/TwinklebearDev-Lessons/blob/master/cmake/FindSDL2.cmake`) on other platforms.


# Usage

Place this entire directory in the subdirectory of your CMake project, then add it with `add_subdirectory()`. Three CMake variables are returned:

`${SDL2_INCLUDE_DIRS}`: The headers you need to include with `include_directories()` or similar.

`${SDL2_LIBRARIES}`: The libraries you need to link your to with `target_link_libraries()`.

`${SDL2_DLLS}`: The path to the bundled `dll`. Useful so you can set CMake up to automatically copy it to your build directory on Windows.


# License

Made by `Peter Hillerström` for `sfzCore` (`https://github.com/PetorSFZ/sfzCore`). I license this whole build wrapper as `public domain` (`SDL2` is obviously still `zlib`). The sole exception being the `FindSDL2.cmake` module, which is under unknown license (but likely BSD "the license" to Kitware). Feel free to do whatever you want with this wrapper, but it would be nice if you kept this readme and the header in the `CMakeLists.txt` file.
