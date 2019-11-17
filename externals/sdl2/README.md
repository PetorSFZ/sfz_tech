# Dependency - SDL2
This is a distribution and CMake wrapper of the SDL2 library (available from https://www.libsdl.org/). It bundles the official pre-built SDL2 binaries (https://libsdl.org/download-2.0.php) for Windows in order to make it easier to build. Uses `FindSDL2.cmake` module (originally found in Twinklebear-Dev SDL tutorial: `https://github.com/Twinklebear/TwinklebearDev-Lessons/blob/master/cmake/FindSDL2.cmake`) on other platforms.

# Usage

Three CMake variables are returned:

`${SDL2_FOUND}`: Variable that signals whether SDL2 is available or not.

`${SDL2_INCLUDE_DIRS}`: The headers you need to include with `include_directories()` or similar.

`${SDL2_LIBRARIES}`: The libraries you need to link your to with `target_link_libraries()`.

`${SDL2_RUNTIME_FILES}`: The path to runtime files. On Windows this is the `SDL2.dll`.

There are two recommended ways of including Dependency-SDL2 in your project, including it directly or downloading it using [`FetchContent`](https://cmake.org/cmake/help/latest/module/FetchContent.html).

## Copying

Just copy this entire distribution and manually place it in your CMake project. Alternatively you could use something like git submodules, but you will have to figure that out yourself. Simply add this directory with `add_subdirectory()` in your `CMakeLists.txt`.

## Downloading

Using `FetchContent` Dependency-SDL2 can be added with the following CMake commands:

~~~cmake
FetchContent_Declare(
	sdl2
	GIT_REPOSITORY https://github.com/PetorSFZ/Dependency-SDL2.git
	GIT_TAG <hash to version you want>
)
FetchContent_GetProperties(sdl2)
if(NOT sdl2_POPULATED)
	FetchContent_Populate(sdl2)
	add_subdirectory(${sdl2_SOURCE_DIR} ${CMAKE_BINARY_DIR}/sdl2)
endif()

# Copy DLLs when building with Visual Studio
if(MSVC)
	file(COPY ${SDL2_RUNTIME_FILES} DESTINATION ${CMAKE_BINARY_DIR}/Debug)
	file(COPY ${SDL2_RUNTIME_FILES} DESTINATION ${CMAKE_BINARY_DIR}/RelWithDebInfo)
	file(COPY ${SDL2_RUNTIME_FILES} DESTINATION ${CMAKE_BINARY_DIR}/Release)
endif()
~~~

# License

Made by `Peter Hillerström` ([GitHub](https://github.com/PetorSFZ)). I license this whole build wrapper as `public domain` (`SDL2` is obviously still `zlib`). The sole exception being the `FindSDL2.cmake` module, which is under unknown license (but likely BSD "the license" to Kitware). Feel free to do whatever you want with this wrapper, but it would be nice if you kept this readme and the header in the `CMakeLists.txt` file.