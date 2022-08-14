# sfz_core

sfz_core is a minimal header-only base library for most skipifzero applications. It mainly contains useful containers, math primitives and some memory stuff.

Main features:

* C-compatible with C++ enhancements, suitable for C API's.
* Compact and minimal
* Fast to compile
* Modular, easy to include in your project
* Header only, no build step
* No dependencies beside standard library
* No exceptions
* No RTTI
* All dynamic memory allocation done through fully customizable allocator interface

In short, it's designed in such a way that I can really quickly throw in a couple of headers if I need something in a new project. Some of the design goals are at odds, header only and being fast to compile are not always friends. If something would really benefit from having a compilation unit (`.cpp` file) then it should probably be excluded from this library. Above all else, it must be friction free to include only what you need from this library into new projects.

## Modular

sfz_core is split up into a number of modules (headers). There are two main headers (`sfz.h` and `sfz_cpp.hpp`), other modules are allowed to include these headers. Otherwise, modules are not allowed to depend on other modules. It's a completely flat dependency graph. This means that you can pick and choose which headers you want to use, the only mandator headers are `sfz.h` for the C-subset and `sfz_cpp.hpp` for C++ headers.

There are two sets of headers, a C-compatible set (ends with `.h`) and a C++ one (ends with `.hpp`). The C++ set is a superset of the C one, particularly `sfz.h` is included by `sfz_cpp.hpp`.

## (No) dependencies

sfz_core has __no__ dependencies beside the C++ standard library, and even then it tries to minimize usage. In particular, `sfz.h` and `sfz_cpp.hpp` are not allowed to include \~any\~ standard headers whatsoever. In order to accomplish this there are some compiler specific trickery where subsets of standard headers are forward declared.

## License

Licensed under zlib, this means that you can basically use the code however you want as long as you give credit and don't claim you wrote it yourself. See LICENSE file for more info.
