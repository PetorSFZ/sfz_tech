# gpu_lib

gpu_lib is a minimalist GPU API, explicitly designed for what I need for my projects. It is forward-facing (drops tons of legacy), makes assumptions regarding how you are going to be using it and tries to minimize the amount of configuration you have to do CPU-side.

gpu_lib is most likely not going to be for you, it only supports compute shaders and drops support for vertex and pixel shaders. If you have a special use-case, like for example raytracing everything using compute shaders, then gpu_lib might be exactly what you are looking for.

## The general idea

The general idea to create simple, minimalist GPU API was as follows:

* Only compute shaders
* Integrate [Dxc](https://github.com/Microsoft/DirectXShaderCompiler) so that user just have to give a path to the `.hlsl` file to compile. Make it easy to share code between CPU and GPU (shared headers `.h`).
	* This also allows us to easily append a standard HLSL prolog to every shader, essentially giving us a small standard library.
* Emulated pointers
	* At init, user specifies size of their GPU heap (max 4 GiB). This allocates a single buffer that is always bound to all shaders as an `RWByteAddressBuffer` or `ByteAddressBuffer`.
	* The only API for allocating buffers is `gpuMalloc()`, which returns a normal `u32` (i.e. the pointer).
	* GPU pointers can be freely copied between CPU and GPU and stored pretty much anywhere, and used to load/store like normal pointers (because there's just a single buffer).
* Bindless textures
	* A texture is just an `u16`, an index into the bindless array of all textures. This bindless array is bound to all shaders.
	* Similar to GPU pointers, a texture index can be stored anywhere and freely copied between CPU/GPU in any way you want.
* Texture API is intentionally limited to allow just enough that we need, but not more.
	* Only 2D textures
	* Only floating point formats (no integer formats)
	* No "weird" formats (srgb, depth, etc).
* Built-in shared upload and readback ringbuffers
	* Sizes defined by user at init
	* User never has to think about allocating memory for uploading/downloading data to GPU, just queue a memcpy upload/download.
* Strictly single-threaded
	* Only a single command queue
	* Only a single command list (but can run up to 3 concurrently before waiting)
	* If you are raytracing everything in compute you are not expected to have all that many draw calls/dispatches. If CPU is your rendering bottleneck then gpu_lib is not for you.
	* It's hard to overstate how much this simplifies the design.
* Native API extensions
	* Can interop gpu_lib with pure D3D12.
	* I mainly use it for UI rendering (ImGui and my own UI lib [ZeroUI](https://github.com/PetorSFZ/sfz_tech/tree/master/Lib-ZeroUI)).
* Intentionally very simple API
	* Should hopefully be easy to port to other underlying APIs in the future (currently D3D12 heavy).
	* Of course, each native api extension you have will have to be ported manually. So should try to not have too many of them.

The initial versions of gpu_lib were super clean and simple, but unfortunately reality got in the way and there were a bunch of extra features that turned out necessary:
* A small const buffer API so you can have a shared constant buffer between all your shaders.
	* Not strictly necessary, but good for peace of mind that you are using the fastest possible path for sensitive data.
* Texture API was extended quite a bit
	* Partly because of HLSL limitations (you unfortunately can't sample from an RWTexture using samplers).
	* But also because textures is such an integral part of GFX programming and it's hard to completely get rid of them.
	* Because of the extended Texture API it became necessary to introduce barriers
* GPU Barriers
	* Unfortunately necessary because of the extended texture API.
	* Main use is to transition textures between read-only and read-write.
	* Also for inserting UAV barriers when necessary.
* Some other small things here and there, but all in all they add up.

## Extensions

There are some [standard extensions](https://github.com/PetorSFZ/sfz_tech/tree/master/Lib-GpuLibExtensions) to gpu_lib. At the time of writing an ImGui renderer and a [ZeroUI](https://github.com/PetorSFZ/sfz_tech/tree/master/Lib-ZeroUI) renderer.

## Warning

There is currently a sync bug on Nvidia which I have yet to iron out. It can be worked around by "flushing" the GPU every frame, which is obviously not ideal, but not the worst thing either. I have yet to reproduce this bug on either Intel or AMD.

## License

Licensed under zlib (see `LICENSE`). However, please keep in mind that the current D3D12 implementation depends on the [D3D12AgilitySDK](https://www.nuget.org/packages/Microsoft.Direct3D.D3D12) and the [dxc compiler](https://github.com/Microsoft/DirectXShaderCompiler) which in turn have various licenses.

Probably obvious, but I'm not giving any support for usage of gpu_lib. If you like what you see then you should be prepared to fork it and maintain it yourself going forward. If you do have feedback, bug reports or feature requests please feel free to communicate them! :) But no promises.
