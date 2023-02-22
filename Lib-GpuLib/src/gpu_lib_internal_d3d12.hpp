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
#ifndef GPU_LIB_INTERNAL_D3D12_HPP
#define GPU_LIB_INTERNAL_D3D12_HPP

#include <math.h>

#include <gpu_lib.h>

#include <sfz_cpp.hpp>
#include <sfz_defer.hpp>
#include <sfz_math.h>
#include <sfz_time.h>
#include <skipifzero_arrays.hpp>
#include <skipifzero_pool.hpp>
#include <skipifzero_strings.hpp>

// Windows.h
#pragma warning(push, 0)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <wrl.h> // ComPtr
//#include <Winerror.h>
#include <shlobj.h>
#include <strsafe.h>
#pragma warning(pop)

// D3D12 headers
#include <D3D12AgilitySDK/d3d12.h>
#include <dxgi1_6.h>
#include <D3D12AgilitySDK/d3d12shader.h>

// DXC compiler
#include <dxc/dxcapi.h>

using Microsoft::WRL::ComPtr;

// gpu_lib
// ------------------------------------------------------------------------------------------------

sfz_constant u32 GPU_MALLOC_ALIGN = 64;
sfz_constant u32 GPU_HEAP_ALIGN = 256;

sfz_constant u32 GPU_ROOT_PARAM_GLOBAL_HEAP_IDX = 0;
sfz_constant u32 GPU_ROOT_PARAM_CONST_BUFFER_IDX = 1;
sfz_constant u32 GPU_ROOT_PARAM_TEX_HEAP_IDX = 2;
sfz_constant u32 GPU_ROOT_PARAM_LAUNCH_PARAMS_IDX = 3;

sfz_constant u32 GPU_CONST_BUFFER_SHADER_REG = 0;
sfz_constant u32 GPU_LAUNCH_PARAMS_SHADER_REG = 1;

sfz_constant u64 GPU_DOWNLOAD_MAX_AGE = GPU_NUM_CONCURRENT_SUBMITS;

sfz_constant u32 GPU_SWAPCHAIN_NUM_BACKBUFFERS = 3;
sfz_constant DXGI_FORMAT GPU_SWAPCHAIN_DXGI_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;

sfz_struct(GpuCmdListBacking) {
	ComPtr<ID3D12CommandAllocator> cmd_allocator;
	u64 fence_value;
	u64 submit_idx;
	u64 upload_heap_offset;
	u64 download_heap_offset;
};

sfz_struct(GpuConstBufferInfo) {
	ComPtr<ID3D12Resource> buffer;
	u32 size_bytes;
	D3D12_RESOURCE_STATES state;
	u64 last_upload_submit_idx;
};

sfz_struct(GpuTexInfo) {
	ComPtr<ID3D12Resource> tex;
	i32x2 tex_res;
	GpuTexDesc desc;
	SfzStr96 name;
};

sfz_struct(GpuPendingDownload) {
	u32 heap_offset;
	u32 num_bytes;
	u64 submit_idx;
};

sfz_struct(GpuKernelInfo) {
	ComPtr<ID3D12PipelineState> pso;
	ComPtr<ID3D12RootSignature> root_sig;
	i32x3 group_dims;
	u32 const_buffer_size;
	u32 launch_params_size;
	GpuKernelDesc desc;
	SfzStr96 name;
	SfzStr320 path;
	SfzStr320 defines;
};

sfz_struct(GpuSwapchainBackbuffer) {
	ComPtr<ID3D12DescriptorHeap> heap_rtv;
	D3D12_CPU_DESCRIPTOR_HANDLE rtv_descriptor;
	ComPtr<ID3D12Resource> back_buffer_rt;
	u64 fence_value;
};

sfz_struct(GpuLib) {
	GpuLibInitCfg cfg;

	// Device
	ComPtr<IDXGIAdapter4> dxgi;
	ComPtr<ID3D12Device3> device;
	ComPtr<ID3D12InfoQueue> info_queue;

	// Commands
	u64 curr_submit_idx;
	u64 known_completed_submit_idx;
	ComPtr<ID3D12CommandQueue> cmd_queue;
	ComPtr<ID3D12Fence> cmd_queue_fence;
	HANDLE cmd_queue_fence_event;
	u64 cmd_queue_fence_value;
	GpuCmdListBacking cmd_list_backings[GPU_NUM_CONCURRENT_SUBMITS];
	ComPtr<ID3D12GraphicsCommandList> cmd_list;
	GpuCmdListBacking& getPrevCmdListBacking() { return cmd_list_backings[(curr_submit_idx > 0 ? curr_submit_idx - 1 : 0) % GPU_NUM_CONCURRENT_SUBMITS]; }
	GpuCmdListBacking& getCurrCmdListBacking() { return cmd_list_backings[curr_submit_idx % GPU_NUM_CONCURRENT_SUBMITS]; }

	// Timestamps
	ComPtr<ID3D12QueryHeap> timestamp_query_heap;

	// GPU Heap
	ComPtr<ID3D12Resource> gpu_heap;
	D3D12_RESOURCE_STATES gpu_heap_state;
	u32 gpu_heap_next_free;

	// Upload heap
	ComPtr<ID3D12Resource> upload_heap;
	u8* upload_heap_mapped_ptr;
	u64 upload_heap_offset;
	u64 upload_heap_safe_offset;

	// Download heap
	ComPtr<ID3D12Resource> download_heap;
	u8* download_heap_mapped_ptr;
	u64 download_heap_offset;
	u64 download_heap_safe_offset;
	SfzPool<GpuPendingDownload> downloads;

	// Const buffers
	SfzPool<GpuConstBufferInfo> const_buffers;

	// Texture descriptor heap
	ComPtr<ID3D12DescriptorHeap> tex_descriptor_heap;
	u32 num_tex_descriptors;
	u32 tex_descriptor_size;
	D3D12_CPU_DESCRIPTOR_HANDLE tex_descriptor_heap_start_cpu;
	D3D12_GPU_DESCRIPTOR_HANDLE tex_descriptor_heap_start_gpu;

	// Textures
	SfzPool<GpuTexInfo> textures;

	// DXC compiler
	ComPtr<IDxcUtils> dxc_utils; // Not thread-safe
	ComPtr<IDxcCompiler3> dxc_compiler; // Not thread-safe
	ComPtr<IDxcIncludeHandler> dxc_include_handler; // Not thread-safe

	// Kernels
	SfzPool<GpuKernelInfo> kernels;

	// Swapchain
	bool allow_tearing;
	i32x2 swapchain_res;
	ComPtr<IDXGISwapChain4> swapchain;
	ComPtr<ID3D12Resource> swapchain_tex;
	GpuSwapchainBackbuffer swapchain_backbuffers[GPU_SWAPCHAIN_NUM_BACKBUFFERS];
	GpuSwapchainBackbuffer& getCurrSwapchainBackbuffer()
	{
		const u32 curr_swapchain_fb_idx = swapchain->GetCurrentBackBufferIndex();
		sfz_assert(curr_swapchain_fb_idx < GPU_SWAPCHAIN_NUM_BACKBUFFERS);
		return swapchain_backbuffers[curr_swapchain_fb_idx];
	}
	ComPtr<ID3D12PipelineState> swapchain_copy_pso;
	ComPtr<ID3D12RootSignature> swapchain_copy_root_sig;

	// Native extensions
	SfzPool<GpuNativeExt> native_exts;

	// Tmp barriers
	SfzArray<D3D12_RESOURCE_BARRIER> tmp_barriers;
};

// Log helpers
// ------------------------------------------------------------------------------------------------

#define GPU_LOG_INFO(fmt, ...) gpuLog(gpu->cfg.log_func, __FILE__, __LINE__, false, (fmt) __VA_OPT__(,) __VA_ARGS__)
#define GPU_LOG_ERROR(fmt, ...) gpuLog(gpu->cfg.log_func, __FILE__, __LINE__, true, (fmt) __VA_OPT__(,) __VA_ARGS__)

// Special versions of the log macros for use during init before the "gpu" pointer is created.
#define GPU_LOG_INFO_INIT(fmt, ...) gpuLog(cfg.log_func, __FILE__, __LINE__, false, (fmt) __VA_OPT__(,) __VA_ARGS__)
#define GPU_LOG_ERROR_INIT(fmt, ...) gpuLog(cfg.log_func, __FILE__, __LINE__, true, (fmt) __VA_OPT__(,) __VA_ARGS__)

inline void gpuLog(
	GpuLogFunc* log_func, const char* file, int line, bool is_error, const char* fmt, ...)
{
	SfzStr2560 msg = {};
	va_list args;
	va_start(args, fmt);
	sfzStr2560VAppendf(&msg, fmt, args);
	va_end(args);
	log_func(file, line, is_error, msg.str);
}

// Texture helpers
// ------------------------------------------------------------------------------------------------

inline DXGI_FORMAT formatToD3D12(GpuFormat fmt)
{
	switch (fmt) {
	case GPU_FORMAT_R_U8_UNORM: return DXGI_FORMAT_R8_UNORM;
	case GPU_FORMAT_RG_U8_UNORM: return DXGI_FORMAT_R8G8_UNORM;
	case GPU_FORMAT_RGBA_U8_UNORM: return DXGI_FORMAT_R8G8B8A8_UNORM;

	case GPU_FORMAT_R_U16_UNORM: return DXGI_FORMAT_R16_UNORM;
	case GPU_FORMAT_RG_U16_UNORM: return DXGI_FORMAT_R16G16_UNORM;
	case GPU_FORMAT_RGBA_U16_UNORM: return DXGI_FORMAT_R16G16B16A16_UNORM;

	case GPU_FORMAT_R_U8_SNORM: return DXGI_FORMAT_R8_SNORM;
	case GPU_FORMAT_RG_U8_SNORM: return DXGI_FORMAT_R8G8_SNORM;
	case GPU_FORMAT_RGBA_U8_SNORM: return DXGI_FORMAT_R8G8B8A8_SNORM;

	case GPU_FORMAT_R_U16_SNORM: return DXGI_FORMAT_R16_SNORM;
	case GPU_FORMAT_RG_U16_SNORM: return DXGI_FORMAT_R16G16_SNORM;
	case GPU_FORMAT_RGBA_U16_SNORM: return DXGI_FORMAT_R16G16B16A16_SNORM;

	case GPU_FORMAT_R_F16: return DXGI_FORMAT_R16_FLOAT;
	case GPU_FORMAT_RG_F16: return DXGI_FORMAT_R16G16_FLOAT;
	case GPU_FORMAT_RGBA_F16: return DXGI_FORMAT_R16G16B16A16_FLOAT;

	case GPU_FORMAT_R_F32: return DXGI_FORMAT_R32_FLOAT;
	case GPU_FORMAT_RG_F32: return DXGI_FORMAT_R32G32_FLOAT;
	case GPU_FORMAT_RGBA_F32: return DXGI_FORMAT_R32G32B32A32_FLOAT;

	default: break;
	}
	sfz_assert(false);
	return DXGI_FORMAT_UNKNOWN;
}

inline u32 formatToPixelSize(GpuFormat fmt)
{
	switch (fmt) {
	case GPU_FORMAT_R_U8_UNORM: return 1 * sizeof(u8);
	case GPU_FORMAT_RG_U8_UNORM: return 2 * sizeof(u8);
	case GPU_FORMAT_RGBA_U8_UNORM: return 4 * sizeof(u8);

	case GPU_FORMAT_R_U16_UNORM: return 1 * sizeof(u16);
	case GPU_FORMAT_RG_U16_UNORM: return 2 * sizeof(u16);
	case GPU_FORMAT_RGBA_U16_UNORM: return 4 * sizeof(u16);

	case GPU_FORMAT_R_U8_SNORM: return 1 * sizeof(u8);
	case GPU_FORMAT_RG_U8_SNORM: return 2 * sizeof(u8);
	case GPU_FORMAT_RGBA_U8_SNORM: return 4 * sizeof(u8);

	case GPU_FORMAT_R_U16_SNORM: return 1 * sizeof(u16);
	case GPU_FORMAT_RG_U16_SNORM: return 2 * sizeof(u16);
	case GPU_FORMAT_RGBA_U16_SNORM: return 4 * sizeof(u16);

	case GPU_FORMAT_R_F16: return 1 * sizeof(u16);
	case GPU_FORMAT_RG_F16: return 2 * sizeof(u16);
	case GPU_FORMAT_RGBA_F16: return 4 * sizeof(u16);

	case GPU_FORMAT_R_F32: return 1 * sizeof(f32);
	case GPU_FORMAT_RG_F32: return 2 * sizeof(f32);
	case GPU_FORMAT_RGBA_F32: return 4 * sizeof(f32);

	default: break;
	}
	sfz_assert(false);
	return 0;
}

inline D3D12_RESOURCE_STATES texStateToD3D12(GpuTexState state)
{
	switch (state) {
	case GPU_TEX_READ_ONLY: return D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
	case GPU_TEX_READ_WRITE: return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	default: break;
	}
	sfz_assert(false);
	return D3D12_RESOURCE_STATE_COMMON;
}

inline i32x2 calcTexTargetRes(i32x2 swapchain_res, const GpuTexDesc* desc)
{
	if (!desc->swapchain_relative) return desc->fixed_res;
	i32x2 res = i32x2_splat(0);
	if (desc->relative_fixed_height != 0) {
		sfz_assert(0 < desc->relative_fixed_height && desc->relative_fixed_height <= 16384);
		const f32 aspect = f32(swapchain_res.x) / f32(swapchain_res.y);
		res.y = desc->relative_fixed_height;
		res.x = i32(roundf(aspect * f32(res.y)));
	}
	else {
		sfz_assert(0.0f < desc->relative_scale && desc->relative_scale <= 8.0f);
		res.x = i32(roundf(desc->relative_scale * f32(swapchain_res.x)));
		res.y = i32(roundf(desc->relative_scale * f32(swapchain_res.y)));
	}
	res.x = i32_max(res.x, 1);
	res.y = i32_max(res.y, 1);
	return res;
}

inline void texSetNullDescriptors(GpuLib* gpu, GpuTexIdx tex_idx)
{
	// UAVs
	const u32 base_idx = u32(tex_idx) * GPU_MAX_NUM_MIPS;
	for (u32 mip_idx = 0; mip_idx < GPU_MAX_NUM_MIPS; mip_idx++) {
		const u32 descr_idx = base_idx + mip_idx;

		D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
		uav_desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		uav_desc.Texture2D.MipSlice = 0;
		uav_desc.Texture2D.PlaneSlice = 0;

		D3D12_CPU_DESCRIPTOR_HANDLE cpu_descriptor = {};
		cpu_descriptor.ptr = gpu->tex_descriptor_heap_start_cpu.ptr + gpu->tex_descriptor_size * descr_idx;
		gpu->device->CreateUnorderedAccessView(nullptr, nullptr, &uav_desc, cpu_descriptor);
	}

	// SRV
	{
		const u32 srvs_offset = gpu->cfg.max_num_textures * GPU_MAX_NUM_MIPS;
		const u32 descr_idx = srvs_offset + u32(tex_idx);

		D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
		srv_desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srv_desc.Texture2D.MostDetailedMip = 0;
		srv_desc.Texture2D.MipLevels = (u32)-1; // All mip-levels from most detailed and downwards
		srv_desc.Texture2D.PlaneSlice = 0;
		srv_desc.Texture2D.ResourceMinLODClamp = 0.0f;

		D3D12_CPU_DESCRIPTOR_HANDLE cpu_descriptor = {};
		cpu_descriptor.ptr = gpu->tex_descriptor_heap_start_cpu.ptr + gpu->tex_descriptor_size * descr_idx;
		gpu->device->CreateShaderResourceView(nullptr, &srv_desc, cpu_descriptor);
	}
}

inline void texSetDescriptors(GpuLib* gpu, GpuTexIdx tex_idx, u32 num_mips, ID3D12Resource* resource, DXGI_FORMAT dxgi_format)
{
	sfz_assert(1 <= num_mips && num_mips <= GPU_MAX_NUM_MIPS);
	const u32 base_idx = u32(tex_idx) * GPU_MAX_NUM_MIPS;
	for (u32 mip_idx = 0; mip_idx < num_mips; mip_idx++) {
		const u32 descr_idx = base_idx + mip_idx;

		D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
		uav_desc.Format = dxgi_format;
		uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		uav_desc.Texture2D.MipSlice = mip_idx;
		uav_desc.Texture2D.PlaneSlice = 0;

		D3D12_CPU_DESCRIPTOR_HANDLE cpu_descriptor = {};
		cpu_descriptor.ptr =
			gpu->tex_descriptor_heap_start_cpu.ptr + gpu->tex_descriptor_size * descr_idx;
		gpu->device->CreateUnorderedAccessView(resource, nullptr, &uav_desc, cpu_descriptor);
	}

	// SRV
	{
		const u32 srvs_offset = gpu->cfg.max_num_textures * GPU_MAX_NUM_MIPS;
		const u32 descr_idx = srvs_offset + u32(tex_idx);

		D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
		srv_desc.Format = dxgi_format;
		srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srv_desc.Texture2D.MostDetailedMip = 0;
		srv_desc.Texture2D.MipLevels = (u32)-1; // All mip-levels from most detailed and downwards
		srv_desc.Texture2D.PlaneSlice = 0;
		srv_desc.Texture2D.ResourceMinLODClamp = 0.0f;

		D3D12_CPU_DESCRIPTOR_HANDLE cpu_descriptor = {};
		cpu_descriptor.ptr = gpu->tex_descriptor_heap_start_cpu.ptr + gpu->tex_descriptor_size * descr_idx;
		gpu->device->CreateShaderResourceView(resource, &srv_desc, cpu_descriptor);
	}
}

inline void texSetDescriptors(GpuLib* gpu, GpuTexIdx tex_idx)
{
	const SfzHandle handle = gpu->textures.getHandle(tex_idx);
	GpuTexInfo* tex_info = gpu->textures.get(handle);
	sfz_assert_hard(tex_info != nullptr);
	texSetDescriptors(
		gpu, tex_idx, u32(tex_info->desc.num_mips), tex_info->tex.Get(), formatToD3D12(tex_info->desc.format));
}

// Heap helpers
// ------------------------------------------------------------------------------------------------

struct GpuHeapRangeAlloc {
	bool success;
	u64 begin;
	u64 begin_mapped;
	u64 end;
};

inline GpuHeapRangeAlloc gpuAllocHeapRange(
	u64 heap_offset, u64 heap_safe_offset, u64 heap_size, u32 num_bytes_original)
{
	const u32 num_bytes = sfzRoundUpAlignedU32(num_bytes_original, GPU_HEAP_ALIGN);
	u64 begin = heap_offset;
	u64 begin_mapped = begin % heap_size;
	if (heap_size < (begin_mapped + num_bytes)) {
		// Wrap around, try in beginning of heap instead.
		begin = sfzRoundUpAlignedU64(heap_offset, heap_size);
		begin_mapped = 0;
	}
	const u64 end = begin + num_bytes;

	GpuHeapRangeAlloc res = {};
	res.success = end < heap_safe_offset;
	res.begin = begin;
	res.begin_mapped = begin_mapped;
	res.end = end;
	return res;
};

inline GpuHeapRangeAlloc gpuAllocUploadHeapRange(const GpuLib* gpu, u32 num_bytes_original)
{
	return gpuAllocHeapRange(
		gpu->upload_heap_offset,
		gpu->upload_heap_safe_offset,
		gpu->cfg.upload_heap_size_bytes,
		num_bytes_original);
};

inline GpuHeapRangeAlloc gpuAllocDownloadHeapRange(const GpuLib* gpu, u32 num_bytes_original)
{
	return gpuAllocHeapRange(
		gpu->download_heap_offset,
		gpu->download_heap_safe_offset,
		gpu->cfg.download_heap_size_bytes,
		num_bytes_original);
};

// Error handling
// ------------------------------------------------------------------------------------------------

inline f32 gpuPrintToMiB(u64 bytes) { return f32(f64(bytes) / (1024.0 * 1024.0)); }

inline const char* resToString(HRESULT res)
{
	switch (res) {
	case DXGI_ERROR_ACCESS_DENIED: return "DXGI_ERROR_ACCESS_DENIED";
	case DXGI_ERROR_ACCESS_LOST: return "DXGI_ERROR_ACCESS_LOST";
	case DXGI_ERROR_ALREADY_EXISTS: return "DXGI_ERROR_ALREADY_EXISTS";
	case DXGI_ERROR_CANNOT_PROTECT_CONTENT: return "DXGI_ERROR_CANNOT_PROTECT_CONTENT";
	case DXGI_ERROR_DEVICE_HUNG: return "DXGI_ERROR_DEVICE_HUNG";
	case DXGI_ERROR_DEVICE_REMOVED: return "DXGI_ERROR_DEVICE_REMOVED";
	case DXGI_ERROR_DEVICE_RESET: return "DXGI_ERROR_DEVICE_RESET";
	case DXGI_ERROR_DRIVER_INTERNAL_ERROR: return "DXGI_ERROR_DRIVER_INTERNAL_ERROR";
	case DXGI_ERROR_FRAME_STATISTICS_DISJOINT: return "DXGI_ERROR_FRAME_STATISTICS_DISJOINT";
	case DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE: return "DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE";
	case DXGI_ERROR_INVALID_CALL: return "DXGI_ERROR_INVALID_CALL";
	case DXGI_ERROR_MORE_DATA: return "DXGI_ERROR_MORE_DATA";
	case DXGI_ERROR_NAME_ALREADY_EXISTS: return "DXGI_ERROR_NAME_ALREADY_EXISTS";
	case DXGI_ERROR_NONEXCLUSIVE: return "DXGI_ERROR_NONEXCLUSIVE";
	case DXGI_ERROR_NOT_CURRENTLY_AVAILABLE: return "DXGI_ERROR_NOT_CURRENTLY_AVAILABLE";
	case DXGI_ERROR_NOT_FOUND: return "DXGI_ERROR_NOT_FOUND";
	case DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED: return "DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED";
	case DXGI_ERROR_REMOTE_OUTOFMEMORY: return "DXGI_ERROR_REMOTE_OUTOFMEMORY";
	case DXGI_ERROR_RESTRICT_TO_OUTPUT_STALE: return "DXGI_ERROR_RESTRICT_TO_OUTPUT_STALE";
	case DXGI_ERROR_SDK_COMPONENT_MISSING: return "DXGI_ERROR_SDK_COMPONENT_MISSING";
	case DXGI_ERROR_SESSION_DISCONNECTED: return "DXGI_ERROR_SESSION_DISCONNECTED";
	case DXGI_ERROR_UNSUPPORTED: return "DXGI_ERROR_UNSUPPORTED";
	case DXGI_ERROR_WAIT_TIMEOUT: return "DXGI_ERROR_WAIT_TIMEOUT";
	case DXGI_ERROR_WAS_STILL_DRAWING: return "DXGI_ERROR_WAS_STILL_DRAWING";

	case S_OK: return "S_OK";
	case E_NOTIMPL: return "E_NOTIMPL";
	case E_NOINTERFACE: return "E_NOINTERFACE";
	case E_POINTER: return "E_POINTER";
	case E_ABORT: return "E_ABORT";
	case E_FAIL: return "E_FAIL";
	case E_UNEXPECTED: return "E_UNEXPECTED";
	case E_ACCESSDENIED: return "E_ACCESSDENIED";
	case E_HANDLE: return "E_HANDLE";
	case E_OUTOFMEMORY: return "E_OUTOFMEMORY";
	case E_INVALIDARG: return "E_INVALIDARG";
	case S_FALSE: return "S_FALSE";
	}
	return "UNKNOWN";
}

inline bool checkD3D12(GpuLogFunc* log_func, const char* file, i32 line, HRESULT res)
{
	if (SUCCEEDED(res)) return true;
	gpuLog(log_func, file, line, true, "[gpu_lib]: D3D12 error: %s", resToString(res));
	return false;
}

// Checks result (HRESULT) from D3D call and log if not success, returns true on success
#define CHECK_D3D12(res) checkD3D12(gpu->cfg.log_func, __FILE__, __LINE__, (res))
#define CHECK_D3D12_INIT(res) checkD3D12(cfg.log_func, __FILE__, __LINE__, (res))

// String functions
// ------------------------------------------------------------------------------------------------

inline i32 utf8ToWide(wchar_t* wide_out, u32 num_wide_chars, const char* utf8_in, i32 num_chars_in = -1)
{
	const i32 num_chars_written = MultiByteToWideChar(CP_UTF8, 0, utf8_in, num_chars_in, wide_out, num_wide_chars);
	return num_chars_written;
}

constexpr u32 WIDE_STR_MAX = 320;

sfz_struct(WideStr) {
	wchar_t str[WIDE_STR_MAX];
};

inline WideStr expandUtf8(const char* utf8)
{
	WideStr wide = {};
	const i32 num_wide_chars = utf8ToWide(wide.str, WIDE_STR_MAX, utf8);
	(void)num_wide_chars;
	return wide;
}

inline void setDebugName(ID3D12Object* object, const char* name)
{
	const WideStr wide_name = expandUtf8(name);
	object->SetName(wide_name.str);
}
#define setDebugNameLazy(name) setDebugName(name.Get(), #name);

// IO functions
// ------------------------------------------------------------------------------------------------

inline WideStr getLastErrorStr()
{
	WideStr err_wide = {};
	FormatMessageW(
		FORMAT_MESSAGE_FROM_SYSTEM,
		nullptr,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), err_wide.str, WIDE_STR_MAX, nullptr);
	return err_wide;
}

sfz_struct(FileMapData) {
	void* ptr;
	HANDLE h_file;
	HANDLE h_map;
	u64 size_bytes;
};

inline FileMapData fileMap(GpuLib* gpu, const char* path, bool read_only)
{
	FileMapData map_data = {};
	const WideStr path_w = expandUtf8(path);

	// Open file
	const DWORD fileAccess = GENERIC_READ | (read_only ? 0 : GENERIC_WRITE);
	const DWORD shareMode = FILE_SHARE_READ; // Other processes shouldn't write to our file
	const DWORD flagsAndAttribs = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN;
	map_data.h_file = CreateFileW(
		path_w.str, fileAccess, shareMode, nullptr, OPEN_EXISTING, flagsAndAttribs, nullptr);
	if (map_data.h_file == INVALID_HANDLE_VALUE) {
		WideStr errWide = getLastErrorStr();
		GPU_LOG_ERROR("Failed to open file (\"%s\"), reason: %S", path, errWide.str);
		return FileMapData{};
	}

	// Get file info
	BY_HANDLE_FILE_INFORMATION fileInfo = {};
	const BOOL fileInfoRes = GetFileInformationByHandle(map_data.h_file, &fileInfo);
	if (!fileInfoRes) {
		WideStr errWide = getLastErrorStr();
		GPU_LOG_ERROR("Failed to get file info for (\"%s\"), reason: %S", path, errWide.str);
		CloseHandle(map_data.h_file);
		return FileMapData{};
	}
	map_data.size_bytes =
		(u64(fileInfo.nFileSizeHigh) * u64(MAXDWORD + 1)) + u64(fileInfo.nFileSizeLow);

	// Create file mapping object
	map_data.h_map = CreateFileMappingA(
		map_data.h_file, nullptr, read_only ? PAGE_READONLY : PAGE_READWRITE, 0, 0, nullptr);
	if (map_data.h_map == INVALID_HANDLE_VALUE) {
		WideStr errWide = getLastErrorStr();
		GPU_LOG_ERROR("Failed to create file mapping object for (\"%s\"), reason: %S", path, errWide.str);
		CloseHandle(map_data.h_file);
		return FileMapData{};
	}

	// Map file
	map_data.ptr = MapViewOfFile(map_data.h_map, read_only ? FILE_MAP_READ : FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (map_data.ptr == nullptr) {
		WideStr errWide = getLastErrorStr();
		GPU_LOG_ERROR("Failed to map (\"%s\"), reason: %S", path, errWide.str);
		CloseHandle(map_data.h_map);
		CloseHandle(map_data.h_file);
		return FileMapData{};
	}

	return map_data;
}

inline void fileUnmap(GpuLib* gpu, FileMapData map_data)
{
	if (map_data.ptr == nullptr) return;
	if (!UnmapViewOfFile(map_data.ptr)) {
		WideStr errWide = getLastErrorStr();
		GPU_LOG_ERROR("Failed to UnmapViewOfFile(), reason: %S", errWide.str);
	}
	if (!CloseHandle(map_data.h_map)) {
		WideStr errWide = getLastErrorStr();
		GPU_LOG_ERROR("Failed to CloseHandle(), reason: %S", errWide.str);
	}
	if (!CloseHandle(map_data.h_file)) {
		WideStr errWide = getLastErrorStr();
		GPU_LOG_ERROR("Failed to CloseHandle(), reason: %S", errWide.str);
	}
}

// Shader helpers
// ------------------------------------------------------------------------------------------------

inline ComPtr<ID3D12RootSignature> gpuCreateDefaultRootSignature(
	GpuLib* gpu, bool read_write_heap, u32 launch_params_size, const char* name, bool gfx_root_sig = false)
{
	// Note: Our goal is for the root signature to NEVER exceed 16 (32-bit) words in size. This is
	//       because a number of GPUs can't natively handle larger root signatures than that, and
	//       has to work around it in software. This includes a certain modern GPU from a prominent
	//       vendor.

	constexpr u32 MAX_NUM_ROOT_PARAMS = 4;
	const u32 num_root_params =
		launch_params_size != 0 ? MAX_NUM_ROOT_PARAMS : (MAX_NUM_ROOT_PARAMS - 1);
	D3D12_ROOT_PARAMETER1 root_params[MAX_NUM_ROOT_PARAMS] = {};

	root_params[GPU_ROOT_PARAM_GLOBAL_HEAP_IDX].ParameterType = read_write_heap ? D3D12_ROOT_PARAMETER_TYPE_UAV : D3D12_ROOT_PARAMETER_TYPE_SRV;
	root_params[GPU_ROOT_PARAM_GLOBAL_HEAP_IDX].Descriptor.ShaderRegister = 0;
	root_params[GPU_ROOT_PARAM_GLOBAL_HEAP_IDX].Descriptor.RegisterSpace = 0;
	// Note: UAV is written to during command list execution, thus it MUST be volatile.
	root_params[GPU_ROOT_PARAM_GLOBAL_HEAP_IDX].Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE;
	root_params[GPU_ROOT_PARAM_GLOBAL_HEAP_IDX].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	root_params[GPU_ROOT_PARAM_CONST_BUFFER_IDX].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	root_params[GPU_ROOT_PARAM_CONST_BUFFER_IDX].Descriptor.ShaderRegister = GPU_CONST_BUFFER_SHADER_REG;
	root_params[GPU_ROOT_PARAM_CONST_BUFFER_IDX].Descriptor.RegisterSpace = 0;
	// Note: DATA_STATIC_WHILE_SET_AT_EXECUTE should be fine, essentially the constant buffer may
	//       not change after we have set the root signature.
	root_params[GPU_ROOT_PARAM_CONST_BUFFER_IDX].Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
	root_params[GPU_ROOT_PARAM_CONST_BUFFER_IDX].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_DESCRIPTOR_RANGE1 desc_ranges[2] = {};
	desc_ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	desc_ranges[0].NumDescriptors = gpu->cfg.max_num_textures * GPU_MAX_NUM_MIPS; // UINT_MAX == Unbounded
	desc_ranges[0].BaseShaderRegister = 0;
	desc_ranges[0].RegisterSpace = 1;
	desc_ranges[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;
	desc_ranges[0].OffsetInDescriptorsFromTableStart = 0;

	desc_ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	desc_ranges[1].NumDescriptors = gpu->cfg.max_num_textures; // UINT_MAX == Unbounded
	desc_ranges[1].BaseShaderRegister = 0;
	desc_ranges[1].RegisterSpace = 1;
	desc_ranges[1].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;
	desc_ranges[1].OffsetInDescriptorsFromTableStart = gpu->cfg.max_num_textures * GPU_MAX_NUM_MIPS;

	root_params[GPU_ROOT_PARAM_TEX_HEAP_IDX].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	root_params[GPU_ROOT_PARAM_TEX_HEAP_IDX].DescriptorTable.NumDescriptorRanges = 2;
	root_params[GPU_ROOT_PARAM_TEX_HEAP_IDX].DescriptorTable.pDescriptorRanges = desc_ranges;
	root_params[GPU_ROOT_PARAM_TEX_HEAP_IDX].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	if (launch_params_size != 0) {
		root_params[GPU_ROOT_PARAM_LAUNCH_PARAMS_IDX].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		root_params[GPU_ROOT_PARAM_LAUNCH_PARAMS_IDX].Constants.ShaderRegister = GPU_LAUNCH_PARAMS_SHADER_REG;
		root_params[GPU_ROOT_PARAM_LAUNCH_PARAMS_IDX].Constants.RegisterSpace = 0;
		root_params[GPU_ROOT_PARAM_LAUNCH_PARAMS_IDX].Constants.Num32BitValues = launch_params_size / 4;
		root_params[GPU_ROOT_PARAM_LAUNCH_PARAMS_IDX].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	}

	constexpr u32 NUM_SAMPLERS = 8;
	D3D12_STATIC_SAMPLER_DESC samplers[NUM_SAMPLERS] = {};

	auto createSampler = [](u32 reg, D3D12_FILTER filter, D3D12_TEXTURE_ADDRESS_MODE address_u, D3D12_TEXTURE_ADDRESS_MODE address_v) {
		D3D12_STATIC_SAMPLER_DESC sampler = {};
		sampler.Filter = filter;
		sampler.AddressU = address_u;
		sampler.AddressV = address_v;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampler.MipLODBias = 0.0f;
		sampler.MaxAnisotropy = 16;
		sampler.ComparisonFunc = D3D12_COMPARISON_FUNC(0);
		sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		sampler.MinLOD = 0.0f;
		sampler.MaxLOD = D3D12_FLOAT32_MAX;
		sampler.ShaderRegister = reg;
		sampler.RegisterSpace = 0;
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		return sampler;
	};

	samplers[0] = createSampler(0, D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	samplers[1] = createSampler(1, D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_WRAP);
	samplers[2] = createSampler(2, D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	samplers[3] = createSampler(3, D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP);

	samplers[4] = createSampler(4, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	samplers[5] = createSampler(5, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_WRAP);
	samplers[6] = createSampler(6, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	samplers[7] = createSampler(7, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP);

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC root_sig_desc = {};
	root_sig_desc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
	root_sig_desc.Desc_1_1.NumParameters = num_root_params;
	root_sig_desc.Desc_1_1.pParameters = root_params;
	root_sig_desc.Desc_1_1.NumStaticSamplers = NUM_SAMPLERS;
	root_sig_desc.Desc_1_1.pStaticSamplers = samplers;
	if (gfx_root_sig) {
		root_sig_desc.Desc_1_1.Flags =
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;
	}
	else {
		root_sig_desc.Desc_1_1.Flags =
			D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;
	}

	ComPtr<ID3DBlob> blob;
	ComPtr<ID3DBlob> error_blob;
	const bool serialize_success = CHECK_D3D12(D3D12SerializeVersionedRootSignature(
		&root_sig_desc, &blob, &error_blob));
	if (!serialize_success) {
		GPU_LOG_ERROR("[gpu_lib]: Failed to serialize root signature: %s",
			(const char*)error_blob->GetBufferPointer());
		return nullptr;
	}

	ComPtr<ID3D12RootSignature> root_sig;
	const bool create_success = CHECK_D3D12(gpu->device->CreateRootSignature(
		0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&root_sig)));
	if (!create_success) {
		GPU_LOG_ERROR("[gpu_lib]: Failed to create root signature.");
		return nullptr;
	}
	setDebugName(root_sig.Get(), name);
	return root_sig;
}

// Kernel prolog
// ------------------------------------------------------------------------------------------------

constexpr char GPU_KERNEL_PROLOG[] = R"(

// Macros and constants
// ------------------------------------------------------------------------------------------------

#define GPU_LIB
#define GPU_HLSL

#define static_assert(cond) _Static_assert((cond), #cond)

// Root signature
// ------------------------------------------------------------------------------------------------

#if defined(GPU_READ_ONLY_HEAP)
ByteAddressBuffer gpu_global_heap : register(t0, space0);
#elif defined(GPU_READ_WRITE_HEAP)
RWByteAddressBuffer gpu_global_heap : register(u0);
#else
#error "You must specify either read only or read-write heap"
#endif

RWTexture2D<float4> gpu_rwtex_array[] : register(u0, space1);
Texture2D gpu_tex_array[] : register(t0, space1);

SamplerState gpu_sampler_nearest_clampu_clampv : register(s0);
SamplerState gpu_sampler_nearest_clampu_wrapv : register(s1);
SamplerState gpu_sampler_nearest_wrapu_clampv : register(s2);
SamplerState gpu_sampler_nearest_wrapu_wrapv : register(s3);

SamplerState gpu_sampler_linear_clampu_clampv : register(s4);
SamplerState gpu_sampler_linear_clampu_wrapv : register(s5);
SamplerState gpu_sampler_linear_wrapu_clampv : register(s6);
SamplerState gpu_sampler_linear_wrapu_wrapv : register(s7);

#define GPU_CONST_BUFFER_REGISTER register(b0)
#define GPU_LAUNCH_PARAMS_REGISTER register(b1)

#define GPU_DECLARE_CONST_BUFFER(T, name) ConstantBuffer<T> name : GPU_CONST_BUFFER_REGISTER
#define GPU_DECLARE_LAUNCH_PARAMS(T, name) ConstantBuffer<T> name : GPU_LAUNCH_PARAMS_REGISTER

// Textures
// ------------------------------------------------------------------------------------------------

typedef uint16_t GpuTexIdx;
static const GpuTexIdx GPU_NULL_TEX = 0;
static const GpuTexIdx GPU_SWAPCHAIN_TEX_IDX = 1;
static const uint GPU_MAX_NUM_MIPS = 12;

RWTexture2D<float4> getRWTex(uint tex_idx, uint mip_idx)
{
	const uint base_idx = tex_idx * GPU_MAX_NUM_MIPS;
	const uint descr_idx = base_idx + mip_idx;
	return gpu_rwtex_array[NonUniformResourceIndex(descr_idx)];
}

RWTexture2D<float4> getSwapchainRWTex() { return getRWTex(GPU_SWAPCHAIN_TEX_IDX, 0); }

Texture2D getTex(GpuTexIdx tex_idx) { return gpu_tex_array[NonUniformResourceIndex(tex_idx)]; }

int2 getRWTexDims(RWTexture2D<float4> tex)
{
	// WARNING! I have observed that this one doesn't always work as expected (maybe driver bug).
	//          As an example, I have gotten back width=2 for the swapchain when it's clearly wider
	//          on Intel GPUs.
	uint w = 0, h = 0;
	tex.GetDimensions(w, h);
	return int2(w, h);
}

// Samplers
// ------------------------------------------------------------------------------------------------

enum GpuFilterMode {
	GPU_NEAREST,
	GPU_LINEAR
};

enum GpuAddressMode {
	GPU_CLAMP,
	GPU_WRAP
};

SamplerState getSampler(GpuFilterMode filter, GpuAddressMode address_u = GPU_CLAMP, GpuAddressMode address_v = GPU_CLAMP)
{
	if (filter == GPU_NEAREST) {
		if (address_u == GPU_CLAMP) {
			if (address_v == GPU_CLAMP) return gpu_sampler_nearest_clampu_clampv;
			else return gpu_sampler_nearest_clampu_wrapv;
		}
		else { // address_u == GPU_WRAP
			if (address_v == GPU_CLAMP) return gpu_sampler_nearest_wrapu_clampv;
			else return gpu_sampler_nearest_wrapu_wrapv;
		}
	}
	else { // filter == GPU_LINEAR
		if (address_u == GPU_CLAMP) {
			if (address_v == GPU_CLAMP) return gpu_sampler_linear_clampu_clampv;
			else return gpu_sampler_linear_clampu_wrapv;
		}
		else { // address_u == GPU_WRAP
			if (address_v == GPU_CLAMP) return gpu_sampler_linear_wrapu_clampv;
			else return gpu_sampler_linear_wrapu_wrapv;
		}
	}
}

// Pointers
// ------------------------------------------------------------------------------------------------

// Pointer type (matches GpuPtr on CPU)
typedef uint GpuPtr;
static const GpuPtr GPU_NULLPTR = 0;

uint ptrLoadByte(GpuPtr ptr)
{
	const uint word_address = ptr & 0xFFFFFFFC;
	const uint word = gpu_global_heap.Load<uint>(word_address);
	const uint byte_address = ptr & 0x00000003;
	const uint byte_shift = byte_address * 8;
	const uint byte = (word >> byte_shift) & 0x000000FF;
	return byte;
}

template<typename T>
T ptrLoad(GpuPtr ptr) { return gpu_global_heap.Load<T>(ptr); }

template<typename T>
T ptrLoadArrayElem(GpuPtr ptr, uint idx) { return gpu_global_heap.Load<T>(ptr + idx * sizeof(T)); }

#ifdef GPU_READ_WRITE_HEAP

template<typename T>
void ptrStore(GpuPtr ptr, T val) { gpu_global_heap.Store<T>(ptr, val); }

template<typename T>
void ptrStoreArrayElem(GpuPtr ptr, uint idx, T val) { gpu_global_heap.Store<T>(ptr + idx * sizeof(T), val); }

#endif // GPU_READ_WRITE_HEAP

#line 1
)";

constexpr u32 GPU_KERNEL_PROLOG_SIZE = sizeof(GPU_KERNEL_PROLOG) - 1; // -1 because null-terminator

#endif // GPU_LIB_INTERNAL_D3D12_HPP
