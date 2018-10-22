# Renderer-CompatibleGL

A Phantasy Engine renderer that aims to be compatible with as many platforms and configurations as possible. This includes being able to run in a browser (WebGL 1.0), on an iPhone (OpenGL ES 2.0) and on macOS in general (OpenGL 3.3).

This renderer can be seen as the lowest common denominator renderer for Phantasy Engine, if your project can run and render fine on this renderer other more complex renders should not have any problems with it.

# Using

To use in your project, call `add_subdirectory()` on this directory in your `CMakeLists.txt`. The following dependencies need to be included before:

* Dependency-SDL2 (`SDL2_FOUND`)
* sfzCore (`SFZ_CORE_FOUND`)
* sfzGL (`SFZ_GL_FOUND`)
* PhantasyEngine-SharedInterface (`PH_SHARED_INTERFACE_FOUND`)

In addition, the renderer can be built in either as a static or a dynamic library. This is controlled by setting the flag `PH_RENDERER_COMPATIBLE_GL_STATIC` before including the renderer.

## Return variables

`PH_RENDERER_COMPATIBLE_GL_FOUND`: `true`

`PH_RENDERER_COMPATIBLE_GL_LIBRARIES`: The built library

`PH_RENDERER_COMPATIBLE_GL_RUNTIME_SYMLINK_DIR`: Path to the directory with assets that need to be available for the renderer to function during runtime. Suggested to symlink this directory into the directory where you place your built binary.