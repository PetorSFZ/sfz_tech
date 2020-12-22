# sfz_core

sfz_core is a minimal header-only base library for most skipifzero applications. It mainly contains useful containers, math primitives and some memory stuff.

Main features:

* Data-oriented design
* Compact
* Simple to read
* Modular, easy to include in your project
* No dependencies beside standard library
* No exceptions
* No RTTI
* All dynamic memory allocation done through fully customizable allocator interface

## Modular

sfz_core is split up into a number of modules (headers). There is a main header (`skipifzero.hpp`), this header is mandatory and included by all other headers in this library. However, the other headers are not allowed to depend on each other. This means that you can pick and choose which headers you want to use.

The modules in sfz_core are:

* `skipifzero.hpp`: (__Mandatory__) Assert macros, Allocator interface, vector primitive, memory helpers and math functions.
* `skipifzero_allocators.hpp`: Standard implementations of the allocator interface.
* `skipifzero_arrays.hpp`: Arrays (replacements for `std::vector`, etc).
* `skipifzero_hash_maps.hpp`: Hash functions, hash maps (replacements for `std::unordered_map`, etc).
* `skipifzero_image_view.hpp`: Types used to specify the view of an image.
* `skipifzero_math.hpp`: Linear algebra math primitives and functions.
* `skipifzero_pool.hpp`: Datastructure which is somewhat of a mix between an array, an allocator and the entity allocation part of an ECS system.
* `skipifzero_ring_buffers.hpp`: Ring buffers (replacements for `std::deque` and such)
* `skipifzero_smart_pointers.hpp`: Smart pointers (replacements for `std::unique_ptr`, etc)
* `skipifzero_strings.hpp`: Strings (replacements for `std::string`, etc).

## (No) dependencies

sfz_core has __no__ dependencies beside the C++ standard library, and even then it tries to minimize usage. Currently the following standard headers are __mandatory__:

* `<cassert>`: Needed for `assert()`, which `sfz_assert()` is a wrapper around. Could potentially be replaced with platform specific intrinsics, such as `__debugbreak()`, but at least `__debugbreak()` does not always fire for me, unlike `assert()`.
* `<cmath>`: Needed for `sqrt()`, could be replaced with platform specific intrinsics.
* `<cstdint>`: Needed for standard sized int types.
* `<cstdlib>`: Needed for `abort()`.
* `<cstring>`: Needed for `mempcpy()` and other memory and string related functionality.
* `<new>`: Needed for placement `new`, which is the only way I know of to properly construct C++ objects in raw memory. Would love to remove this header if I could find another way to accomplish this in a cross-platform manner.
* `<type_traits>`: Needed to enforce a number of constraints on types using `static_assert()`.
* `<utility>`: Needed for `std::move()`, `std::forward()` and `std::swap()`. All of which are pretty much necessary in order to use move semantics. Would love to remove this header if I could get ahold of a simple, minimal (<200 lines of code) implementation of above.

In addition, some headers have additional standard library header requirements. These __optional__ standard headers are:

* `<atomic>`: Needed for atomic integers.
* `<cstdarg>`: Needed for `va_list` and related functionality.
* `<cstdio>`: Needed for `vsnprintf()`.
* `<cctype>`: Needed for `tolower()`
* `<malloc.h>`: (Windows only): Needed for `_aligned_malloc()`.

## Building and running tests

The `gen_scripts` directory contains various generation scripts that generate build solutions using `CMake`. You only need to do this if you are interested in running the tests.

## License

Licensed under zlib, this means that you can basically use the code however you want as long as you give credit and don't claim you wrote it yourself. See LICENSE file for more info.

