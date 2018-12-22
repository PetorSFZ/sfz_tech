# ZeroG - A Cross-API GPU API
## Current state

Currently in very early development, not worth trying out unless you are interested in getting involved with development. Work is done in the `Prototyping` branch until it is in an early alpha-state, at which point `master` will become the main development branch.

## About and goal

Zero-G is intended to be a high-level GPU API implemented on top of other GPU APIs. Somewhat similar to [bgfx](https://github.com/bkaradzic/bgfx), but with slightly different design goals. The current target design goals are as follows:

* Lightweight

  * No external dependencies (besides 3D APIs used)
  * Keep number of bundled dependencies down, keep it clean
  * Easy to build
  * Easy to integrate into existing codebases
  * Compact codebase, fast to compile

* Easy to use, small API

  * DLL with C-API
  * Small C++ wrapper on top of C-API

* No legacy

  * No legacy support whatsoever, only modern APIs supported
  * I.e., only Vulkan, D3D12 and Metal implementations
  * Possibly WebGPU in the future if sensible
  * Possibly D3D11 (mostly to debug and compare performance with D3D12 implementation)

* Target modern, higher-performance GPUs

  * API should not make design concessions for limitations of low-end or old hardware
  * Lowest end hardware supported: Skylake Intel GPUs and latest mobile Apple GPUs.

* GPU Compute first

  * Compute is the most important part of a modern GPU API
  * Compute should be as easy and intuitive to use as possible, it should not feel as an afterthought.
  * Ideally as similar to CUDA as possible, but not entirely realistic without making obnoxious intrusive additions (such as custom compilers that parses C++ source before the C++ compiler itself).

* Prefer usability over CPU overhead

  * It is expected that most applications using ZeroG will be mostly GPU bound rather than CPU bound.
  * Keep GPU performance high, but be willing to sacrifice CPU performance for an easy to use API.
  * With that said, API should be both multi-threaded and deferred.

* SPIR-V shaders

  * Use SPIR-V as primary shader format in order to allow the user to program in whatever shading language they want.
  * Optional functions to directly load whatever native shading language (e.g. HLSL) the API currently in use uses.

* Access cutting-edge features

  * A major benefit on being on the latest graphics APIs is cutting-edge features
  * Examples include Shader Model 6 (wave intrinsics), DXR (raytracing), mesh shaders, etc.
  * These should be prioritized for inclusion into the API.
  * It is expected that users of ZeroG want to access features of newer graphics APIs, but are not willing to spend time writing all the boiler-plate necessary to use these APIs directly.

## Limitations

I am but one person, and I'm only working on this in my spare time. I will mostly focus on implement whatever I personally need for my own projects. If it turns out this project is too ambitious, or if I get bored of it, I will likely drop it.

When (and if) the project becomes usable I will try to open it up more, document it clearly and hopefully try to get the community involved.

## Helping out

As I see it there are currently two ways of helping out, help out with development or provide funds so I can work on this professionally.

Development help will definitely be appreciated in the future, but its currently a bit too early for direct help. At this point discussions on how to design and structure the API is basically the only useful contributions.

As for funds and donations I would currently feel a bit bad about accepting them considering the very, very early state of development.

In any case, please contact me if you are interested in helping out! :)