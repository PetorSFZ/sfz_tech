# sfz_tech

`sfz_tech` is the mega repository for all of [skipifzero](http://www.skipifzero.com/)'s (i.e. Peter "PetorSFZ" Hillerström's) base technology. In other words, everything that is not an actual application or product. This includes various helper libraries, a game engine, a graphics API, and a number of third-party dependencies used by these technologies.

Why a mega repository? Well, simply because it makes it way easier for me to version things. Instead of having one version number for each individual project, I have one for all projects combined. If I make a breaking change in one project, I have to fix all incompatibilities with all other projects in the repo. My projects which then depend on one or several projects in this repo just have to keep track of one version number instead of several.

That said, just because the projects are in the same repository does NOT mean that they necessarily depend on each other. They just happen to live in the same repository. If you only want to use e.g. [ZeroG](https://github.com/PetorSFZ/sfz_tech/tree/master/Lib-ZeroG), you can just copy its directory and then throw away the rest.

## Projects

* [Lib-Core](https://github.com/PetorSFZ/sfz_tech/tree/master/Lib-Core) - A small, compact, no-dependency, header-only, math and containers library.
* [Lib-PhantasyEngine](https://github.com/PetorSFZ/sfz_tech/tree/master/Lib-PhantasyEngine) - A lightweight, barebones, DIY game engine.
* [Lib-ZeroG](https://github.com/PetorSFZ/sfz_tech/tree/master/Lib-ZeroG) - `ZeroG` is a (somewhat higher-level) GPU API implemented on top modern low-level GPU API's, such as D3D12 and Vulkan.
* [Lib-ZeroG-ImGui](https://github.com/PetorSFZ/sfz_tech/tree/master/Lib-ZeroG-ImGui) - `ZeroG-ImGui` is a `ZeroG` based `ImGui` renderer.

## Third-party libraries

* [dear-imgui](https://github.com/PetorSFZ/sfz_tech/tree/master/externals/dear-imgui) - [Original source](https://github.com/ocornut/imgui) - Immediate mode gui library.
* [imgui-plot](https://github.com/PetorSFZ/sfz_tech/tree/master/externals/imgui-plot) - [Original source](https://github.com/soulthreads/imgui-plot) - Improved plot widget for dear-imgui.
* [nativefiledialog](https://github.com/PetorSFZ/sfz_tech/tree/master/externals/nativefiledialog) - [Original source](https://github.com/mlabbe/nativefiledialog) - Portable library for native file open/save dialogs.
* [sajson](https://github.com/PetorSFZ/sfz_tech/tree/master/externals/sajson) - [Original source](https://github.com/chadaustin/sajson) - Single-allocation JSON parser.
* [SDL2](https://github.com/PetorSFZ/sfz_tech/tree/master/externals/sdl2) - [Original source](https://www.libsdl.org/) - Cross-platform low-level input/windowing API.
* [stb headers](https://github.com/PetorSFZ/sfz_tech/tree/master/externals/stb) - [Original source](https://github.com/nothings/stb) - Single file public domain headers.
* [tinygltf](https://github.com/PetorSFZ/sfz_tech/tree/master/externals/tinygltf) - [Original source](https://github.com/syoyo/tinygltf) - Header only library for loading glTF files.
* [utest.h](https://github.com/PetorSFZ/sfz_tech/tree/master/externals/utest.h) - [Original source](https://github.com/sheredom/utest.h) - Single-header testing framework.

In addition individual projects inside this mega repository may have private 3rd-party dependencies, see each project for more information. In particular, [ZeroG](https://github.com/PetorSFZ/sfz_tech/tree/master/Lib-ZeroG) has a number of rendering related 3rd-party libraries which is exclusively used by it.

## Licenses

Most code in this repository developed by skipifzero is licensed under the [zlib](https://www.zlib.net/zlib_license.html) license, each individual project should contain a `LICENSE` file with more specific information for that project. If such a file is missing, check if the source code itself contains a license, if missing there as well, make an issue or contact me so I can correct it.

Third-party libraries are, of course, under whatever license they were originally licensed under. 3rd-party libraries are, as far as possible, placed inside their own directories. In these directories there should usually be some sort of license file. In addition, I try to place some sort of readme (often named `README-SFZ.md`) of my own which contain information about when the library was acquired and from where.
