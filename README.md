# sfz_tech

__sfz_tech uses [git-lfs](https://git-lfs.github.com/), don't forget to install it before cloning this repo.__

`sfz_tech` is the mega repository for all of [skipifzero](http://www.skipifzero.com/)'s (i.e. Peter "PetorSFZ" Hillerström's) base technology. In other words, stuff that isn't an actual product. This includes various helper libraries, a graphics API, a UI library, and a number of third-party dependencies used by these technologies.

Why a mega repository? Well, simply because it makes it way easier for me to version things. Instead of having one version number for each individual project, I have one for all projects combined. If I make a breaking change in one project, I have to fix all incompatibilities with all other projects in the repo. My projects which then depend on one or several projects in this repo just have to keep track of one version number instead of several.

## Projects

* [Lib-Core](https://github.com/PetorSFZ/sfz_tech/tree/master/Lib-Core) - A small, compact, no-dependency, header-only, math and containers library.
* [Lib-GpuLib](https://github.com/PetorSFZ/sfz_tech/tree/master/Lib-GpuLib) - A minimalist GPU API built on top of D3D12.
* [Lib-GpuLibExtensions](https://github.com/PetorSFZ/sfz_tech/tree/master/Lib-GpuLibExtensions) - Native API extensions to gpu_lib.
* [Lib-ZeroUI](https://github.com/PetorSFZ/sfz_tech/tree/master/Lib-ZeroUI) - An immediate mode (~ish) game UI library.

## Third-party libraries

* [dear-imgui](https://github.com/PetorSFZ/sfz_tech/tree/master/externals/dear-imgui) - [Original source](https://github.com/ocornut/imgui) - Immediate mode gui library.
* [doctest](https://github.com/PetorSFZ/sfz_tech/tree/master/externals/doctest) - [Original source](https://github.com/doctest/doctest) - Fast single-header testing framework.
* [imgui-plot](https://github.com/PetorSFZ/sfz_tech/tree/master/externals/imgui-plot) - [Original source](https://github.com/soulthreads/imgui-plot) - Improved plot widget for dear-imgui.
* [sajson](https://github.com/PetorSFZ/sfz_tech/tree/master/externals/sajson) - [Original source](https://github.com/chadaustin/sajson) - Single-allocation JSON parser.
* [SDL2](https://github.com/PetorSFZ/sfz_tech/tree/master/externals/sdl2) - [Original source](https://www.libsdl.org/) - Cross-platform low-level input/windowing API.
* [SoLoud](https://github.com/PetorSFZ/sfz_tech/tree/master/externals/soloud) - [Original source](https://github.com/jarikomppa/soloud) - Free, easy, portable audio engine for games.
* [stb headers](https://github.com/PetorSFZ/sfz_tech/tree/master/externals/stb) - [Original source](https://github.com/nothings/stb) - Single file public domain headers.

In addition individual projects inside this mega repository may have private 3rd-party dependencies, see each project for more information.

## Licenses

Most code in this repository developed by skipifzero is licensed under the [zlib](https://www.zlib.net/zlib_license.html) license, each individual project should contain a `LICENSE` file with more specific information for that project. If such a file is missing, check if the source code itself contains a license, if missing there as well, make an issue or contact me so I can correct it.

Third-party libraries are, of course, under whatever license they were originally licensed under. 3rd-party libraries are, as far as possible, placed inside their own directories. In these directories there should usually be some sort of license file. In addition, I try to place some sort of readme (often named `README-SFZ.md`) of my own which contain information about when the library was acquired and from where.
