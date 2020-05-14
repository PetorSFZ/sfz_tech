# ZeroG - A Cross-API GPU API

ZeroG is intended to be a cross-API GPU API (think [bgfx](https://github.com/bkaradzic/bgfx) or [sokol_gfx](https://github.com/floooh/sokol)). The main difference to those API's is that it does not care about legacy in the slightest. The main APIs it plans to support is D3D12 and Vulkan, and maybe Metal or WebGPU if they make sense. It's intended to be significantly more low-level than bgfx or sokol_gfx, but higher level than using D3D12/Vulkan directly.

Currently it's a work in progress with a fairly decent D3D12 implementation.