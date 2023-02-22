// Copyright (c) Peter Hillerström 2022-2023 (skipifzero.com, peter@hstroem.se)
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.

#pragma once
#ifndef GPU_LIB_H
#define GPU_LIB_H

#include <sfz.h>

// Constants
// ------------------------------------------------------------------------------------------------

// This constant defines the number of command lists that can be in-flight at the same time. It's
// important for synchronization, if you are downloading data from the GPU every frame you should
// typically have a lag of this many frames before you get the data.
sfz_constant u32 GPU_NUM_CONCURRENT_SUBMITS = 2;

sfz_constant u32 GPU_HEAP_SYSTEM_RESERVED_SIZE = 8 * 1024 * 1024;
sfz_constant u32 GPU_HEAP_MIN_SIZE = GPU_HEAP_SYSTEM_RESERVED_SIZE;
sfz_constant u32 GPU_HEAP_MAX_SIZE = U32_MAX;
sfz_constant u32 GPU_MAX_NUM_CONST_BUFFERS = 4;
sfz_constant u32 GPU_TEXTURES_MIN_NUM = 2;
sfz_constant u32 GPU_TEXTURES_MAX_NUM = 16384;
sfz_constant u16 GPU_SWAPCHAIN_TEX_IDX = 1;
sfz_constant u32 GPU_MAX_NUM_MIPS = 12;
sfz_constant u32 GPU_LAUNCH_PARAMS_MAX_SIZE = 8 * sizeof(u32);
sfz_constant u32 GPU_KERNEL_MAX_NUM_DEFINES = 8;
sfz_constant u32 GPU_KERNEL_DEFINE_MAX_LEN = 48;
sfz_constant u32 GPU_KERNEL_DEFINES_STR_MAX_LEN = 320;


// Init API
// ------------------------------------------------------------------------------------------------

typedef void GpuLogFunc(const char* file, i32 line, bool is_error, const char* msg);

sfz_struct(GpuLibInitCfg) {

	SfzAllocator* cpu_allocator;
	GpuLogFunc* log_func;

	u32 gpu_heap_size_bytes;
	u32 upload_heap_size_bytes;
	u32 download_heap_size_bytes;
	u32 max_num_concurrent_downloads;
	u32 max_num_textures;
	u32 max_num_kernels;
	u32 max_num_native_exts;

	void* native_window_handle;

	bool debug_mode;
	bool debug_shader_validation;

	// Attempt to load "WinPixGpuCapturer.dll", allows WinPix to attach to a running process.
	bool load_pix_gpu_capturer_dll;
};

struct GpuLib;
sfz_extern_c GpuLib* gpuLibInit(const GpuLibInitCfg* cfg);
sfz_extern_c void gpuLibDestroy(GpuLib* gpu);


// Native Extension API
// ------------------------------------------------------------------------------------------------

typedef void GpuNativeExtRunFunc(GpuLib* gpu, void* ext_data_ptr, void* params, u32 params_size);
typedef void GpuNativeExtDestroyFunc(GpuLib* gpu, void* ext_data_ptr);

sfz_struct(GpuNativeExt) {
	void* ext_data_ptr;
	GpuNativeExtRunFunc* run_func;
	GpuNativeExtDestroyFunc* destroy_func;
};

sfz_struct(GpuNativeExtHandle) {
	u32 handle;
#ifdef __cplusplus
	constexpr bool operator== (GpuNativeExtHandle o) const { return handle == o.handle; }
	constexpr bool operator!= (GpuNativeExtHandle o) const { return handle != o.handle; }
#endif
};
sfz_constant GpuNativeExtHandle GPU_NULL_NATIVE_EXT = {};

sfz_extern_c GpuNativeExtHandle gpuNativeExtRegister(GpuLib* gpu, const GpuNativeExt* ext);
sfz_extern_c void gpuNativeExtRun(GpuLib* gpu, GpuNativeExtHandle ext_handle, void* params, u32 params_size);


// Memory API
// ------------------------------------------------------------------------------------------------

typedef u32 GpuPtr;
sfz_constant GpuPtr GPU_NULLPTR = 0;

sfz_extern_c GpuPtr gpuMalloc(GpuLib* gpu, u32 num_bytes);
sfz_extern_c void gpuFree(GpuLib* gpu, GpuPtr ptr);


// Constant buffer API
// ------------------------------------------------------------------------------------------------

// Constant buffers are a bit of a hack in gpu_lib. They only exist to solve a single use-case, a
// big shared constant buffer between all your kernels. This is technically doable using the
// normal pointer API, but for these constants it's reassuring to know you have the fastest
// possible path the underlying API exposes.
//
// You can bind a single constant buffer to each kernel dispatch. You can create a very limited
// number (GPU_MAX_NUM_CONST_BUFFERS) of constant buffers in total (ideally, you are supposed to
// have a single one for all your kernels!). You can only write to constant buffers using
// gpuQueueMemcpyUploadConstBuffer(), and only once per submit.

sfz_struct(GpuConstBuffer) {
	u32 handle;

#ifdef __cplusplus
	constexpr bool operator== (GpuConstBuffer o) const { return handle == o.handle; }
	constexpr bool operator!= (GpuConstBuffer o) const { return handle != o.handle; }
#endif
};

sfz_constant GpuConstBuffer GPU_NULL_CBUFFER = {};

sfz_extern_c GpuConstBuffer gpuConstBufferInit(GpuLib* gpu, u32 num_bytes, const char* name);
sfz_extern_c void gpuConstBufferDestroy(GpuLib* gpu, GpuConstBuffer cbuf);


// Textures API
// ------------------------------------------------------------------------------------------------

// A GpuTexIdx represents a texture. As with GpuPtr this index can be freely copied to the GPU and
// used to bindlessly access the texture it represents. Unlike with GpuPtr, a texture can be
// accessed in 2 different ways on the GPU.
//
// Either it can be accessed as a read-only texture `Texture2D`, which can be sampled from using
// samplers etc.
//
// Or each individual mip can be accessed as a read-write texture `RWTexture2D`, which allows for
// arbitrary reads/writes but no sampling using samplers.
//
// Each texture can be in either READ_ONLY or READ_WRITE state, and it's only valid to access from
// the correct bindless array. I.e., it would be undefined behavior to access a texture as
// `Texture2D` if it is in the READ_WRITE state.
typedef u16 GpuTexIdx;
sfz_constant GpuTexIdx GPU_NULL_TEX = 0;

typedef enum {
	GPU_FORMAT_UNDEFINED = 0,

	GPU_FORMAT_R_U8_UNORM, // Normalized between [0, 1]
	GPU_FORMAT_RG_U8_UNORM, // Normalized between [0, 1]
	GPU_FORMAT_RGBA_U8_UNORM, // Normalized between [0, 1]

	GPU_FORMAT_R_U16_UNORM, // Normalized between [0, 1]
	GPU_FORMAT_RG_U16_UNORM, // Normalized between [0, 1]
	GPU_FORMAT_RGBA_U16_UNORM, // Normalized between [0, 1]

	GPU_FORMAT_R_U8_SNORM, // Normalized between [-1, 1]
	GPU_FORMAT_RG_U8_SNORM, // Normalized between [-1, 1]
	GPU_FORMAT_RGBA_U8_SNORM, // Normalized between [-1, 1]

	GPU_FORMAT_R_U16_SNORM, // Normalized between [-1, 1]
	GPU_FORMAT_RG_U16_SNORM, // Normalized between [-1, 1]
	GPU_FORMAT_RGBA_U16_SNORM, // Normalized between [-1, 1]

	GPU_FORMAT_R_F16,
	GPU_FORMAT_RG_F16,
	GPU_FORMAT_RGBA_F16,

	GPU_FORMAT_R_F32,
	GPU_FORMAT_RG_F32,
	GPU_FORMAT_RGBA_F32,

	GPU_FORMAT_FORCE_I32 = I32_MAX
} GpuFormat;

sfz_extern_c const char* gpuFormatToString(GpuFormat format);

// The different states a texture can be in. It is ~very~ important to note that READ_WRITE is NOT
// a superset of READ_ONLY. READ_WRITE means that the texture can ONLY be accessed as an RWTexture,
// it is not allowed to access it as a read-only texture.
typedef enum {
	GPU_TEX_STATE_UNDEFINED = 0,
	GPU_TEX_READ_ONLY,
	GPU_TEX_READ_WRITE,
	GPU_TEX_STATE_FORCE_I32 = I32_MAX
} GpuTexState;

sfz_extern_c const char* gpuTexStateToString(GpuTexState state);

sfz_struct(GpuTexDesc) {
	const char* name;
	GpuFormat format;

	// Resolution of this texture if it is not swapchain relative. Must be a power of 2 if texture
	// has mipmaps.
	i32x2 fixed_res;

	// Number of mipmaps, defaults to 1 (i.e. no mipmaps). Swapchain relative textures may not have
	// mipmaps.
	i32 num_mips;

	// If the texture is swapchain relative it will be reallocated whenever the swapchain changes
	// resolution. The parameters below starting with "relative_" are used to determine what the
	// resolution should be relative to the swapchain.
	bool swapchain_relative;
	i32 relative_fixed_height;
	f32 relative_scale;

	// The initial tex state of this texture. Defaults to read-only.
	GpuTexState tex_state;
};

sfz_extern_c GpuTexIdx gpuTexInit(GpuLib* gpu, const GpuTexDesc* desc);
sfz_extern_c void gpuTexDestroy(GpuLib* gpu, GpuTexIdx tex);

sfz_extern_c const GpuTexDesc* gpuTexGetDesc(const GpuLib* gpu, GpuTexIdx tex_idx);
sfz_extern_c i32x2 gpuTexGetRes(const GpuLib* gpu, GpuTexIdx tex_idx);
sfz_extern_c GpuTexState gpuTexGetState(const GpuLib* gpu, GpuTexIdx tex_idx);

// Changes size of a swapchain relative texture
sfz_extern_c void gpuTexSetSwapchainRelativeScale(GpuLib* gpu, GpuTexIdx tex_idx, f32 scale);
sfz_extern_c void gpuTexSetSwapchainRelativeFixedHeight(GpuLib* gpu, GpuTexIdx tex_idx, i32 height);


// Kernel API
// ------------------------------------------------------------------------------------------------

sfz_struct(GpuKernel) {
	u32 handle;

#ifdef __cplusplus
	constexpr bool operator== (GpuKernel o) const { return handle == o.handle; }
	constexpr bool operator!= (GpuKernel o) const { return handle != o.handle; }
#endif
};

sfz_constant GpuKernel GPU_NULL_KERNEL = {};

sfz_struct(GpuKernelDesc) {
	const char* name;
	const char* path;
	bool write_enabled_heap; // Whether this kernel can write to the global heap or not.
	const char* defines; // Space separated list of defines, e.g. "FIRST=1 OTHER=2".
};

sfz_extern_c GpuKernel gpuKernelInit(GpuLib* gpu, const GpuKernelDesc* desc);
sfz_extern_c void gpuKernelDestroy(GpuLib* gpu, GpuKernel kernel);

sfz_extern_c bool gpuKernelReload(GpuLib* gpu, GpuKernel kernel);

sfz_extern_c i32x3 gpuKernelGetGroupDims(const GpuLib* gpu, GpuKernel kernel);
inline i32x2 gpuKernelGetGroupDims2(const GpuLib* gpu, GpuKernel kernel)
{
	const i32x3 dims = gpuKernelGetGroupDims(gpu, kernel);
	sfz_assert(dims.z == 1);
	return i32x2_init(dims.x, dims.y);
}
inline i32 gpuKernelGetGroupDims1(const GpuLib* gpu, GpuKernel kernel)
{
	const i32x3 dims = gpuKernelGetGroupDims(gpu, kernel);
	sfz_assert(dims.y == 1 && dims.z == 1);
	return dims.x;
}


// Command API
// ------------------------------------------------------------------------------------------------

// Represents a ticket for a GPU download. GPU's are async, and if you download data back to the CPU
// it's not going to be done until later when it has finished executing this particular command.
// Instead, you get a ticket back that you can later use to retrieve the data.
sfz_struct(GpuTicket) {
	u32 handle;

#ifdef __cplusplus
	constexpr bool operator== (GpuTicket o) const { return handle == o.handle; }
	constexpr bool operator!= (GpuTicket o) const { return handle != o.handle; }
#endif
};
sfz_constant GpuTicket GPU_NULL_TICKET = {};

// Represents a barrier.
//
// There are two types of barriers, UAV barriers and transition barriers. UAV barriers are used to
// to ensure all writes to a read-write resource are finished, this is only needed when there are
// overlapping write-writes or read-writes between dispatches. If you are unsure you should at the
// very least insert a GPU_HEAP_UAV_BARRIER and GPU_SWAPCHAIN_UAV_BARRIER after each dispatch.
//
// Transition barriers are mainly used used to transition textures between READ_ONLY and READ_WRITE
// states.
sfz_struct(GpuBarrier) {
	bool uav_barrier;
	u16 res_idx; // 0 == none, U16_MAX == gpu heap, otherwise tex index.
	GpuTexState target_state;
};

sfz_constant GpuBarrier GPU_UAV_ALL_BARRIER = { true, U16_MAX - 1, GPU_TEX_STATE_UNDEFINED };
sfz_constant GpuBarrier GPU_HEAP_UAV_BARRIER = { true, U16_MAX, GPU_TEX_STATE_UNDEFINED };
sfz_constant GpuBarrier GPU_SWAPCHAIN_UAV_BARRIER = { true, GPU_SWAPCHAIN_TEX_IDX, GPU_TEX_STATE_UNDEFINED };
inline GpuBarrier gpuBarrierUAV(GpuTexIdx tex_idx) { return { true, tex_idx, GPU_TEX_STATE_UNDEFINED }; }
inline GpuBarrier gpuBarrierTransition(GpuTexIdx tex_idx, GpuTexState target_state) { return { false, tex_idx, target_state }; }

// The Command API is what you are primarily going to be using frame to frame. The functions are
// ordered in approximately the order you are expected to call them per-frame.

// Returns the index of the current command list. Increments every gpuSubmitQueuedWork().
sfz_extern_c u64 gpuGetCurrSubmitIdx(const GpuLib* gpu);

// Returns the current resolution of the swapchain (window) being rendered to.
sfz_extern_c i32x2 gpuSwapchainGetRes(const GpuLib* gpu);

// Event API
sfz_extern_c void gpuQueueEventBegin(GpuLib* gpu, const char* name, const f32x4* optional_color);
sfz_extern_c void gpuQueueEventEnd(GpuLib* gpu);

// Returns the number of ticks per second (i.e. frequency) of the gpu timestamps.
sfz_extern_c u64 gpuTimestampGetFreq(const GpuLib* gpu);

// Takes a timestamp and stores it in the u64 pointed to in the global heap.
sfz_extern_c void gpuQueueTakeTimestamp(GpuLib* gpu, GpuPtr dst);

// Takes a timestamp and immediately start downloading it to the CPU, bypasses the global heap and
// should be faster than gpuQueueTakeTimestamp() if you don't need the result on the GPU.
sfz_extern_c GpuTicket gpuQueueTakeTimestampDownload(GpuLib* gpu);

// Queues an upload to the GPU. Instantly copies input to upload heap, no need to keep src around.
sfz_extern_c void gpuQueueMemcpyUpload(GpuLib* gpu, GpuPtr dst, const void* src, u32 num_bytes);

sfz_extern_c void gpuQueueMemcpyUploadConstBuffer(
	GpuLib* gpu, GpuConstBuffer cbuf, const void* src, u32 num_bytes);

sfz_extern_c void gpuQueueMemcpyUploadTexMip(
	GpuLib* gpu, GpuTexIdx tex_idx, i32 mip_idx, const void* src, i32x2 mip_dims, GpuFormat format);

// Queues a download to the CPU. Downloading takes time, returns a ticket that can be used to
// retrieve the data in a later submit when it's ready.
sfz_extern_c GpuTicket gpuQueueMemcpyDownload(GpuLib* gpu, GpuPtr src, u32 num_bytes);

// Checks whether a given ticket is valid or not. If it is not valid, then the download has either
// already been processed, or has been removed because it was too old.
sfz_extern_c bool gpuIsTicketValid(GpuLib* gpu, GpuTicket ticket);

// Retrieves the data from a previously queued memcpy download.
sfz_extern_c void gpuGetDownloadedData(GpuLib* gpu, GpuTicket ticket, void* dst, u32 num_bytes);

// Queues a kernel dispatch
sfz_extern_c void gpuQueueDispatch(
	GpuLib* gpu, GpuKernel kernel, i32x3 num_groups, GpuConstBuffer cbuf, const void* params, u32 params_size);

// Queues a list of barriers. It's more efficient to run multiple barriers simultaneously, which is
// why the API is constructed this way.
sfz_extern_c void gpuQueueBarriers(GpuLib* gpu, const GpuBarrier* barriers, u32 num_barriers);

// Copies swapchain tex to actual swapchain.
sfz_extern_c void gpuQueueCopyToSwapchain(GpuLib* gpu);

// Informs gpu_lib that you have finished all your rendering to the swapchain. Must be called last
// thing before gpuSubmitQueuedWork() before you do a gpuSwapchainPresent().
sfz_extern_c void gpuQueueSwapchainFinish(GpuLib* gpu);

// Submits queued work to GPU and prepares to start recording more.
sfz_extern_c void gpuSubmitQueuedWork(GpuLib* gpu);

// Presents the latest swapchain image to the screen. Will block GPU and resize swapchain if
// resolution has changed.
sfz_extern_c void gpuSwapchainPresent(GpuLib* gpu, bool vsync);

// Flushes (blocks) until all currently submitted GPU work has finished executing.
sfz_extern_c void gpuFlushSubmittedWork(GpuLib* gpu);


// C++ helpers
// ------------------------------------------------------------------------------------------------

// These are C++ wrapped variant of random parts of the API to make it nicer to use in C++.

#ifdef __cplusplus

inline GpuTexIdx gpuTexInit(GpuLib* gpu, const GpuTexDesc& desc) { return gpuTexInit(gpu, &desc); }
inline GpuKernel gpuKernelInit(GpuLib* gpu, const GpuKernelDesc& desc) { return gpuKernelInit(gpu, &desc); }

template<typename T>
T gpuGetDownloadedData(GpuLib* gpu, GpuTicket ticket)
{
	T tmp = {};
	gpuGetDownloadedData(gpu, ticket, &tmp, sizeof(T));
	return tmp;
}

template<typename T>
void gpuQueueDispatch(GpuLib* gpu, GpuKernel kernel, i32 num_groups, GpuConstBuffer cbuf, const T& params)
{
	gpuQueueDispatch<T>(gpu, kernel, i32x3_init(num_groups, 1, 1), cbuf, params);
}

template<typename T>
void gpuQueueDispatchPerPixel(GpuLib* gpu, i32x2 res, GpuKernel kernel, GpuConstBuffer cbuf, const T& params)
{
	const i32x2 group_dims = gpuKernelGetGroupDims2(gpu, kernel);
	const i32x2 num_groups = (res + group_dims - i32x2_splat(1)) / group_dims;
	gpuQueueDispatch(gpu, kernel, i32x3_init2(num_groups, 1), cbuf, &params, sizeof(T));
}

inline void gpuQueueDispatchPerPixel(GpuLib* gpu, i32x2 res, GpuKernel kernel, GpuConstBuffer cbuf)
{
	const i32x2 group_dims = gpuKernelGetGroupDims2(gpu, kernel);
	const i32x2 num_groups = (res + group_dims - i32x2_splat(1)) / group_dims;
	gpuQueueDispatch(gpu, kernel, i32x3_init2(num_groups, 1), cbuf, nullptr, 0);
}

template <u32 num_barriers>
void gpuQueueBarriers(GpuLib* gpu, const GpuBarrier(&barriers)[num_barriers])
{
	gpuQueueBarriers(gpu, barriers, num_barriers);
}

#endif // __cplusplus

#endif // GPU_LIB_H
