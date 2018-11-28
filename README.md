# Phantasy Engine

Phantasy Engine is a barebones, cross-platform, DIY, game engine. DIY in the sense that it is very modular, and you can pick and choose what parts you want to use. Most notably, a standard renderer is provided (`CompatibleGL`), but you are pretty much expected to write your own for everything besides very simple projects.

The engine itself is (currently) comprised into 3 libraries:

* `Interface`: A shared interace for communication between different engine modules.
* `Core`/`Engine`: The main engine module. This part is normally statically linked into your project, it also owns the `main()` function.
* `Renderer`: A renderer of your choice (likely modified by you), communicates via the `Interface` module.

The engine is cross-platform and can deploy on the following platforms:

* Windows
* macOS
* iOS
* Web (via Emscripten)

As you can see Linux is notably missing, this is solely because no one is testing it on Linux at the moment. There are a few known incompatibilities, but nothing that should be particularly hard to fix.

## Renderer-CompatibleGL

A Phantasy Engine renderer that aims to be compatible with as many platforms and configurations as possible. This includes being able to run in a browser (WebGL 1.0), on an iPhone (OpenGL ES 2.0) and on macOS in general (OpenGL 3.3).

This renderer can be seen as the lowest common denominator renderer for Phantasy Engine, if your project can run and render fine on this renderer other more complex renders should not have any problems with it.

## Usage/testing

If you want to try out Phantasy Engine for yourself, the best starting position is the [PhantasyTestbed](https://github.com/PetorSFZ/PhantasyTestbed). PhantasyTestbed is a small testbed application which loads and renders the classic Sponza testscene.

## License

Licensed under zlib, this means that you can basically use the code however you want as long as you give credit and don't claim you wrote it yourself. See LICENSE file for more info.

Libraries used by PhantasyEngine fall under various licenses, see their respective LICENSE files.