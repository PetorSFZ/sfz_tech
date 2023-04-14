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

#include "gpu_lib_internal_d3d12.hpp"

// D3D12 Agility SDK exports
// ------------------------------------------------------------------------------------------------

// Link to D3D12 libs
#pragma comment (lib, "d3d12.lib")
#pragma comment (lib, "dxgi.lib")

// The version of the Agility SDK we are using, see https://devblogs.microsoft.com/directx/directx12agility/
extern "C" { _declspec(dllexport) extern const u32 D3D12SDKVersion = 606; }

// Specifies that D3D12Core.dll will be available in a directory called D3D12 next to the exe.
extern "C" { _declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }

// Load WinPixGpuCapturer.dll
// ------------------------------------------------------------------------------------------------

// By loading WinPixGpuCapturer.dll before you start issuing D3D12 calls you can in theory attach
// the WinPix profiler to a running process and profile GPU calls.
//
// See: https://devblogs.microsoft.com/pix/taking-a-capture/

static void tryLoadWinPixGpuCapturerDll(void)
{
	// Early exit if DLL is already loaded
	if (GetModuleHandleW(L"WinPixGpuCapturer.dll") != 0) return;

	// Get search path
	LPWSTR programFilesPath = nullptr;
	SHGetKnownFolderPath(FOLDERID_ProgramFiles, KF_FLAG_DEFAULT, NULL, &programFilesPath);
	wchar_t searchPath[MAX_PATH] = {};
	wsprintfW(searchPath, L"%ls\\Microsoft PIX\\*", programFilesPath);

	// Find pix installation
	WIN32_FIND_DATAW findData;
	bool foundPixInstallation = false;
	wchar_t newestVersionFound[MAX_PATH];
	HANDLE hFind = FindFirstFileW(searchPath, &findData);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if (((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) &&
				(findData.cFileName[0] != '.')) {
				if (!foundPixInstallation || wcscmp(newestVersionFound, findData.cFileName) <= 0) {
					foundPixInstallation = true;
					StringCchCopyW(newestVersionFound, _countof(newestVersionFound), findData.cFileName);
				}
			}
		} while (FindNextFileW(hFind, &findData) != 0);
	}
	FindClose(hFind);

	// If we found pix installation, try to load DLL
	if (foundPixInstallation) {
		wchar_t dllPath[MAX_PATH] = {};
		wsprintfW(dllPath, L"%ls\\Microsoft PIX\\%ls\\WinPixGpuCapturer.dll", programFilesPath, newestVersionFound);
		LoadLibraryW(dllPath);
	}
}

// Debug messages
// ------------------------------------------------------------------------------------------------

static void logDebugMessages(GpuLib* gpu, ID3D12InfoQueue* info_queue)
{
	if (info_queue == nullptr) return;

	constexpr u32 MAX_MSG_LEN = 512;
	u8 msg_raw[MAX_MSG_LEN];
	D3D12_MESSAGE* msg = reinterpret_cast<D3D12_MESSAGE*>(msg_raw);

	const u64 num_messages = info_queue->GetNumStoredMessages();
	for (u64 i = 0; i < num_messages; i++) {

		// Get the size of the message
		SIZE_T msg_len = 0;
		CHECK_D3D12(info_queue->GetMessage(0, NULL, &msg_len));
		if (MAX_MSG_LEN < msg_len) {
			sfz_assert(false);
			GPU_LOG_ERROR("[gpu_lib]: Message too long, skipping.");
			continue;
		}

		// Get and print message
		memset(msg, 0, MAX_MSG_LEN);
		CHECK_D3D12(info_queue->GetMessageA(0, msg, &msg_len));
		GPU_LOG_INFO("[gpu_lib]: D3D12 message: %s", msg->pDescription);
	}

	// Clear stored messages
	info_queue->ClearStoredMessages();
}

// Init API
// ------------------------------------------------------------------------------------------------

sfz_extern_c GpuLib* gpuLibInit(const GpuLibInitCfg* cfg_in)
{
	// Copy config so that we can make changes to it before finally storing it in the context
	GpuLibInitCfg cfg = *cfg_in;
	cfg.gpu_heap_size_bytes = u32_clamp(cfg.gpu_heap_size_bytes, GPU_HEAP_MIN_SIZE, GPU_HEAP_MAX_SIZE);
	cfg.max_num_textures = u32_clamp(cfg.max_num_textures, GPU_TEXTURES_MIN_NUM, GPU_TEXTURES_MAX_NUM);
	cfg.upload_heap_size_bytes = sfzRoundUpAlignedU32(cfg.upload_heap_size_bytes, GPU_HEAP_ALIGN);
	cfg.download_heap_size_bytes = sfzRoundUpAlignedU32(cfg.download_heap_size_bytes, GPU_HEAP_ALIGN);

	// Ensure a log function has been set
	sfz_assert(cfg.log_func != nullptr);
	if (cfg.log_func == nullptr) {
		return nullptr;
	}

	// Load WinPixGpuCapturer.dll if requested
	if (cfg.load_pix_gpu_capturer_dll) {
		sfz_assert_hard(!cfg.debug_mode);
		sfz_assert_hard(!cfg.debug_shader_validation);
		tryLoadWinPixGpuCapturerDll();
	}

	// Enable debug layers in debug mode
	if (cfg.debug_mode) {

		// Get debug interface
		ComPtr<ID3D12Debug1> debug_interface;
		if (!CHECK_D3D12_INIT(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_interface)))) {
			return nullptr;
		}

		// Enable debug layer and GPU based validation
		debug_interface->EnableDebugLayer();

		// Enable GPU based debug mode if requested
		if (cfg.debug_shader_validation) {
			debug_interface->SetEnableGPUBasedValidation(TRUE);
		}
	}

	// Create DXGI factory
	ComPtr<IDXGIFactory6> dxgi_factory;
	{
		UINT flags = 0;
		if (cfg.debug_mode) flags |= DXGI_CREATE_FACTORY_DEBUG;
		if (!CHECK_D3D12_INIT(CreateDXGIFactory2(flags, IID_PPV_ARGS(&dxgi_factory)))) {
			return nullptr;
		}
	}

	// Create device
	ComPtr<IDXGIAdapter4> dxgi;
	ComPtr<ID3D12Device3> device;
	{
		const bool adapter_success = CHECK_D3D12_INIT(dxgi_factory->EnumAdapterByGpuPreference(
			0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&dxgi)));
		if (!adapter_success) return nullptr;

		DXGI_ADAPTER_DESC1 dxgi_desc = {};
		CHECK_D3D12_INIT(dxgi->GetDesc1(&dxgi_desc));
		GPU_LOG_INFO_INIT(
			"[gpu_lib]: Using adapter: \"%S\" with %.0fMiB video mem, %.0f MiB system mem and %.0f MiB shared mem.",
			dxgi_desc.Description,
			gpuPrintToMiB(dxgi_desc.DedicatedVideoMemory),
			gpuPrintToMiB(dxgi_desc.DedicatedSystemMemory),
			gpuPrintToMiB(dxgi_desc.SharedSystemMemory));

		const bool device_success = CHECK_D3D12_INIT(D3D12CreateDevice(
			dxgi.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device)));
		if (!device_success) return nullptr;
	}

	// Enable debug message in debug mode
	ComPtr<ID3D12InfoQueue> info_queue;
	if (cfg.debug_mode) {
		if (!CHECK_D3D12_INIT(device->QueryInterface(IID_PPV_ARGS(&info_queue)))) {
			return nullptr;
		}
		CHECK_D3D12_INIT(info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE));
		CHECK_D3D12_INIT(info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE));
	}

	// Check feature support
	{
		D3D12_FEATURE_DATA_D3D12_OPTIONS4 options4 = {};
		CHECK_D3D12_INIT(device->CheckFeatureSupport(
			D3D12_FEATURE_D3D12_OPTIONS4, &options4, sizeof(options4)));
		if (!options4.Native16BitShaderOpsSupported) {
			GPU_LOG_ERROR_INIT("[gpu_lib]: The GPU does not support 16-bit ops, which is required. Exiting.");
			return nullptr;
		}

		D3D12_FEATURE_DATA_D3D12_OPTIONS options = {};
		CHECK_D3D12_INIT(device->CheckFeatureSupport(
			D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options)));

		D3D12_FEATURE_DATA_D3D12_OPTIONS1 options1 = {};
		CHECK_D3D12_INIT(device->CheckFeatureSupport(
			D3D12_FEATURE_D3D12_OPTIONS1, &options1, sizeof(options1)));

		D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
		CHECK_D3D12_INIT(device->CheckFeatureSupport(
			D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5)));

		D3D12_FEATURE_DATA_D3D12_OPTIONS12 options12 = {};
		CHECK_D3D12_INIT(device->CheckFeatureSupport(
			D3D12_FEATURE_D3D12_OPTIONS12, &options12, sizeof(options12)));

		D3D12_FEATURE_DATA_SHADER_MODEL shader_model = {};
		shader_model.HighestShaderModel = D3D_SHADER_MODEL_6_7; // Set to highest model your app understands
		CHECK_D3D12_INIT(device->CheckFeatureSupport(
			D3D12_FEATURE_SHADER_MODEL, &shader_model, sizeof(shader_model)));

		const bool supports_shader_dynamic_resources =
			(options.ResourceBindingTier == D3D12_RESOURCE_BINDING_TIER_3) &&
			(shader_model.HighestShaderModel >= D3D_SHADER_MODEL_6_6);

		auto shaderModelToStr = [](D3D_SHADER_MODEL model) {
			switch (model) {
			case D3D_SHADER_MODEL_5_1: return "5.1";
			case D3D_SHADER_MODEL_6_0: return "6.0";
			case D3D_SHADER_MODEL_6_1: return "6.1";
			case D3D_SHADER_MODEL_6_2: return "6.2";
			case D3D_SHADER_MODEL_6_3: return "6.3";
			case D3D_SHADER_MODEL_6_4: return "6.4";
			case D3D_SHADER_MODEL_6_5: return "6.5";
			case D3D_SHADER_MODEL_6_6: return "6.6";
			case D3D_SHADER_MODEL_6_7: return "6.7";
			}
			return "UNKNOWN";
		};

		GPU_LOG_INFO_INIT(
R"([gpu_lib]: Feature support

Shader model: %s
Shader dynamic resources: %s

Wave ops: %s
WaveLaneCountMin: %u
WaveLaneCountMax: %u
GpuTotalLaneCount: %u

RTX support: %s

Enhanced barriers: %s)",
			shaderModelToStr(shader_model.HighestShaderModel),
			supports_shader_dynamic_resources ? "True" : "False",
			options1.WaveOps ? "True" : "False",
			options1.WaveLaneCountMin,
			options1.WaveLaneCountMax,
			options1.TotalLaneCount,
			options5.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED ? "True" : "False",
			options12.EnhancedBarriersSupported ? "True" : "False");
	}

	// Create command queue
	ComPtr<ID3D12CommandQueue> cmd_queue;
	ComPtr<ID3D12Fence> cmd_queue_fence;
	HANDLE cmd_queue_fence_event = nullptr;
	{
		D3D12_COMMAND_QUEUE_DESC queue_desc = {};
		queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_HIGH;
		queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE; // D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT
		queue_desc.NodeMask = 0;
		if (!CHECK_D3D12_INIT(device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&cmd_queue)))) {
			GPU_LOG_ERROR_INIT("[gpu_lib]: Could not create command queue.");
			return nullptr;
		}

		if (!CHECK_D3D12_INIT(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&cmd_queue_fence)))) {
			GPU_LOG_ERROR_INIT("[gpu_lib]: Could not create command queue fence.");
			return nullptr;
		}

		cmd_queue_fence_event = CreateEventA(NULL, false, false, "gpu_lib_cmd_queue_fence_event");
	}

	// Create command lists
	GpuCmdListBacking cmd_list_backings[GPU_NUM_CONCURRENT_SUBMITS];
	ComPtr<ID3D12GraphicsCommandList> cmd_list;
	{
		for (u32 i = 0; i < GPU_NUM_CONCURRENT_SUBMITS; i++) {
			GpuCmdListBacking& backing = cmd_list_backings[i];
			if (!CHECK_D3D12_INIT(device->CreateCommandAllocator(
				D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&backing.cmd_allocator)))) {
				GPU_LOG_ERROR_INIT("[gpu_lib]: Could not create command allocator.");
				return nullptr;
			}
			backing.fence_value = 0;
			backing.submit_idx = 0;
			backing.upload_heap_offset = 0;
			backing.download_heap_offset = 0;
		}
		if (!CHECK_D3D12_INIT(device->CreateCommandList(
			0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			cmd_list_backings[0].cmd_allocator.Get(),
			nullptr,
			IID_PPV_ARGS(&cmd_list)))) {

			GPU_LOG_ERROR_INIT("[gpu_lib]: Could not create command list.");
			return nullptr;
		}
	}


	// Create timestamp stuff
	ComPtr<ID3D12QueryHeap> timestamp_query_heap;
	{
		D3D12_QUERY_HEAP_DESC query_desc = {};
		query_desc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
		query_desc.Count = 1;
		query_desc.NodeMask = 0;
		if (!CHECK_D3D12_INIT(device->CreateQueryHeap(&query_desc, IID_PPV_ARGS(&timestamp_query_heap)))) {
			GPU_LOG_ERROR_INIT("[gpu_lib]: Could not create timestap query heap.");
			return nullptr;
		}
		setDebugNameLazy(timestamp_query_heap);
	}

	// Allocate our gpu heap
	ComPtr<ID3D12Resource> gpu_heap;
	{
		D3D12_HEAP_PROPERTIES heap_props = {};
		heap_props.Type = D3D12_HEAP_TYPE_DEFAULT;
		heap_props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heap_props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heap_props.CreationNodeMask = 0;
		heap_props.VisibleNodeMask = 0;

		const D3D12_HEAP_FLAGS heap_flags =
			D3D12_HEAP_FLAG_ALLOW_SHADER_ATOMICS;

		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Alignment = 0;
		desc.Width = cfg.gpu_heap_size_bytes;
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		const bool heap_success = CHECK_D3D12_INIT(device->CreateCommittedResource(
			&heap_props, heap_flags, &desc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&gpu_heap)));
		if (!heap_success) {
			GPU_LOG_ERROR_INIT("[gpu_lib]: Could not allocate gpu heap of size %.2f MiB, exiting.",
				gpuPrintToMiB(cfg.gpu_heap_size_bytes));
			return nullptr;
		}
		setDebugNameLazy(gpu_heap);
	}

	// Allocate upload heap
	ComPtr<ID3D12Resource> upload_heap;
	u8* upload_heap_mapped_ptr = nullptr; // Persistently mapped, never unmapped
	{
		D3D12_HEAP_PROPERTIES heap_props = {};
		heap_props.Type = D3D12_HEAP_TYPE_UPLOAD;
		heap_props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heap_props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heap_props.CreationNodeMask = 0;
		heap_props.VisibleNodeMask = 0;

		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Alignment = 0;
		desc.Width = cfg.upload_heap_size_bytes;
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Flags = D3D12_RESOURCE_FLAGS(0);

		const bool heap_success = CHECK_D3D12_INIT(device->CreateCommittedResource(
			&heap_props,
			D3D12_HEAP_FLAG_CREATE_NOT_ZEROED,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&upload_heap)));
		if (!heap_success) {
			GPU_LOG_ERROR_INIT("[gpu_lib]: Could not allocate upload heap of size %.2f MiB, exiting.",
				gpuPrintToMiB(cfg.upload_heap_size_bytes));
			return nullptr;
		}
		setDebugNameLazy(upload_heap);

		void* mapped_ptr = nullptr;
		if (!CHECK_D3D12_INIT(upload_heap->Map(0, nullptr, &mapped_ptr))) {
			GPU_LOG_ERROR_INIT("[gpu_lib]: Failed to map upload heap.");
			return nullptr;
		}
		upload_heap_mapped_ptr = static_cast<u8*>(mapped_ptr);
	}

	// Allocate download heap
	ComPtr<ID3D12Resource> download_heap;
	u8* download_heap_mapped_ptr = nullptr; // Persistently mapped, never unmapped
	{
		D3D12_HEAP_PROPERTIES heap_props = {};
		heap_props.Type = D3D12_HEAP_TYPE_READBACK;
		heap_props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heap_props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heap_props.CreationNodeMask = 0;
		heap_props.VisibleNodeMask = 0;

		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Alignment = 0;
		desc.Width = cfg.download_heap_size_bytes;
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Flags = D3D12_RESOURCE_FLAGS(0);

		const bool heap_success = CHECK_D3D12_INIT(device->CreateCommittedResource(
			&heap_props,
			D3D12_HEAP_FLAG_CREATE_NOT_ZEROED,
			&desc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&download_heap)));
		if (!heap_success) {
			GPU_LOG_ERROR_INIT("[gpu_lib]: Could not allocate download heap of size %.2f MiB, exiting.",
				gpuPrintToMiB(cfg.download_heap_size_bytes));
			return nullptr;
		}
		setDebugNameLazy(download_heap);

		void* mapped_ptr = nullptr;
		if (!CHECK_D3D12_INIT(download_heap->Map(0, nullptr, &mapped_ptr))) {
			GPU_LOG_ERROR_INIT("[gpu_lib]: Failed to map download heap.");
			return nullptr;
		}
		download_heap_mapped_ptr = static_cast<u8*>(mapped_ptr);
	}

	// Create tex descriptor heap
	ComPtr<ID3D12DescriptorHeap> tex_descriptor_heap;
	u32 num_tex_descriptors = 0;
	u32 tex_descriptor_size = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE tex_descriptor_heap_start_cpu = {};
	D3D12_GPU_DESCRIPTOR_HANDLE tex_descriptor_heap_start_gpu = {};
	{
		num_tex_descriptors = cfg.max_num_textures * GPU_MAX_NUM_MIPS + cfg.max_num_textures;
		D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
		heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		heap_desc.NumDescriptors = num_tex_descriptors;
		heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		heap_desc.NodeMask = 0;

		if (!CHECK_D3D12_INIT(device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&tex_descriptor_heap)))) {
			GPU_LOG_ERROR_INIT("[gpu_lib]: Could not allocate %u descriptors for texture arrays, exiting.",
				num_tex_descriptors);
			return nullptr;
		}
		setDebugNameLazy(tex_descriptor_heap);

		tex_descriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		tex_descriptor_heap_start_cpu = tex_descriptor_heap->GetCPUDescriptorHandleForHeapStart();
		tex_descriptor_heap_start_gpu = tex_descriptor_heap->GetGPUDescriptorHandleForHeapStart();
	}

	// Initialize texture pool
	SfzPool<GpuTexInfo> textures;
	{
		textures.init(cfg.max_num_textures, cfg.cpu_allocator, sfz_dbg("GpuLib::textures"));
		const SfzHandle null_slot = textures.allocate();
		sfz_assert(null_slot.idx() == GPU_NULL_TEX);
		const SfzHandle swapchain_slot = textures.allocate();
		sfz_assert(swapchain_slot.idx() == GPU_SWAPCHAIN_TEX_IDX);
	}

	// Load DXC compiler
	ComPtr<IDxcUtils> dxc_utils;
	ComPtr<IDxcCompiler3> dxc_compiler;
	ComPtr<IDxcIncludeHandler> dxc_include_handler;
	{
		if (!CHECK_D3D12_INIT(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxc_utils)))) {
			GPU_LOG_ERROR_INIT("[gpu_lib]: Could not initialize DXC utils.");
			return nullptr;
		}

		if (!CHECK_D3D12_INIT(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxc_compiler)))) {
			GPU_LOG_ERROR_INIT("[gpu_lib]: Could not initialize DXC compiler.");
			return nullptr;
		}

		if (!CHECK_D3D12_INIT(dxc_utils->CreateDefaultIncludeHandler(&dxc_include_handler))) {
			GPU_LOG_ERROR_INIT("[gpu_lib]: Could not create DXC include handler.");
			return nullptr;
		}
	}

	// If we have a window handle specified create swapchain and such
	bool allow_tearing = false;
	ComPtr<IDXGISwapChain4> swapchain;
	if (cfg.native_window_handle != nullptr) {
		const HWND hwnd = static_cast<const HWND>(cfg.native_window_handle);

		// Check if screen-tearing is allowed
		{
			BOOL tearing_allowed = FALSE;
			CHECK_D3D12_INIT(dxgi_factory->CheckFeatureSupport(
				DXGI_FEATURE_PRESENT_ALLOW_TEARING, &tearing_allowed, sizeof(tearing_allowed)));
			allow_tearing = tearing_allowed != FALSE;
		}

		// Create swap chain
		{
			DXGI_SWAP_CHAIN_DESC1 desc = {};
			// Dummy initial res, will allocate framebuffers for real at first use.
			desc.Width = 4;
			desc.Height = 4;
			desc.Format = GPU_SWAPCHAIN_DXGI_FORMAT;
			desc.Stereo = FALSE;
			desc.SampleDesc = { 1, 0 }; // No MSAA
			desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			desc.BufferCount = GPU_SWAPCHAIN_NUM_BACKBUFFERS;
			desc.Scaling = DXGI_SCALING_STRETCH;
			desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
			desc.Flags = (allow_tearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0);

			ComPtr<IDXGISwapChain1> tmp_swapchain;
			if (!CHECK_D3D12_INIT(dxgi_factory->CreateSwapChainForHwnd(
				cmd_queue.Get(), hwnd, &desc, nullptr, nullptr, &tmp_swapchain))) {
				GPU_LOG_ERROR_INIT("[gpu_lib]: Could not create swapchain.");
				return nullptr;
			}
			if (!CHECK_D3D12_INIT(tmp_swapchain.As(&swapchain))) {
				GPU_LOG_ERROR_INIT("[gpu_lib]: Could not create swapchain.");
				return nullptr;
			}
		}

		// Disable Alt+Enter to fullscreen
		//
		// This fixes issues with DXGI_PRESENT_ALLOW_TEARING, which is required for Adaptive Sync
		// to work correctly with windowed applications. The default Alt+Enter shortcut enters
		// "true" fullscreen (same as calling SetFullscreenState(TRUE)), which is not what we want
		// if we only want to support e.g. borderless fullscreen.
		CHECK_D3D12_INIT(dxgi_factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));
	}

	// Initialize as much as possible of swapchain backbuffer data as possible
	GpuSwapchainBackbuffer swapchain_backbuffers[GPU_SWAPCHAIN_NUM_BACKBUFFERS] = {};
	for (u32 i = 0; i < GPU_SWAPCHAIN_NUM_BACKBUFFERS; i++) {
		GpuSwapchainBackbuffer& bbuf = swapchain_backbuffers[i];

		D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc = {};
		rtv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtv_heap_desc.NumDescriptors = 1;
		rtv_heap_desc.NodeMask = 0;
		const bool success = CHECK_D3D12_INIT(device->CreateDescriptorHeap(
			&rtv_heap_desc, IID_PPV_ARGS(&bbuf.heap_rtv)));
		if (!success) {
			GPU_LOG_ERROR_INIT("[gpu_lib]: Could not create RTV descriptor heap.");
			return nullptr;
		}
		bbuf.rtv_descriptor = bbuf.heap_rtv->GetCPUDescriptorHandleForHeapStart();
		bbuf.fence_value = 0;
	}

	GpuLib* gpu = sfz_new<GpuLib>(cfg.cpu_allocator, sfz_dbg("GpuLib"));
	*gpu = {};
	gpu->cfg = cfg;

	gpu->dxgi = dxgi;
	gpu->device = device;
	gpu->info_queue = info_queue;

	gpu->curr_submit_idx = 0;
	gpu->known_completed_submit_idx = 0;
	gpu->cmd_queue = cmd_queue;
	gpu->cmd_queue_fence = cmd_queue_fence;
	gpu->cmd_queue_fence_event = cmd_queue_fence_event;
	gpu->cmd_queue_fence_value = 0;
	for (u32 i = 0; i < GPU_NUM_CONCURRENT_SUBMITS; i++) gpu->cmd_list_backings[i] = cmd_list_backings[i];
	gpu->cmd_list = cmd_list;

	gpu->timestamp_query_heap = timestamp_query_heap;

	gpu->gpu_heap = gpu_heap;
	gpu->gpu_heap_state = D3D12_RESOURCE_STATE_COMMON;
	gpu->gpu_heap_next_free = GPU_HEAP_SYSTEM_RESERVED_SIZE;

	gpu->upload_heap = upload_heap;
	gpu->upload_heap_mapped_ptr = upload_heap_mapped_ptr;
	gpu->upload_heap_offset = 0;
	gpu->upload_heap_safe_offset = 0;

	gpu->download_heap = download_heap;
	gpu->download_heap_mapped_ptr = download_heap_mapped_ptr;
	gpu->download_heap_offset = 0;
	gpu->download_heap_safe_offset = 0;
	gpu->downloads.init(cfg.max_num_concurrent_downloads, cfg.cpu_allocator, sfz_dbg("GpuLib::downloads"));

	gpu->const_buffers.init(GPU_MAX_NUM_CONST_BUFFERS, cfg.cpu_allocator, sfz_dbg("GpuLib::const_buffers"));

	gpu->tex_descriptor_heap = tex_descriptor_heap;
	gpu->num_tex_descriptors = num_tex_descriptors;
	gpu->tex_descriptor_size = tex_descriptor_size;
	gpu->tex_descriptor_heap_start_cpu = tex_descriptor_heap_start_cpu;
	gpu->tex_descriptor_heap_start_gpu = tex_descriptor_heap_start_gpu;

	gpu->textures = sfz_move(textures);

	gpu->dxc_utils = dxc_utils;
	gpu->dxc_compiler = dxc_compiler;
	gpu->dxc_include_handler = dxc_include_handler;

	gpu->kernels.init(cfg.max_num_kernels, cfg.cpu_allocator, sfz_dbg("GpuLib::kernels"));

	gpu->allow_tearing = allow_tearing;
	gpu->swapchain_res = i32x2_splat(0);
	gpu->swapchain = swapchain;
	for (u32 i = 0; i < GPU_SWAPCHAIN_NUM_BACKBUFFERS; i++) gpu->swapchain_backbuffers[i] = swapchain_backbuffers[i];

	gpu->native_exts.init(cfg.max_num_native_exts, cfg.cpu_allocator, sfz_dbg("GpuLib::native_exts"));

	gpu->tmp_barriers.init(cfg.max_num_textures + 1, cfg.cpu_allocator, sfz_dbg("GpuLib::tmp_barriers"));

	// Set null descriptors for all potential texture slots
	for (u32 i = 0; i < cfg.max_num_textures; i++) {
		texSetNullDescriptors(gpu, GpuTexIdx(i));
	}

	// Do a quick present after initialization has finished, used to set up framebuffers
	gpuSubmitQueuedWork(gpu);
	gpuSwapchainPresent(gpu, false, 1);
	sfz_assert(gpu->curr_submit_idx == 1);
	sfz_assert(gpu->upload_heap_safe_offset == gpu->cfg.upload_heap_size_bytes);
	sfz_assert(gpu->download_heap_safe_offset == gpu->cfg.download_heap_size_bytes);

	// Compile swapchain copy shader
	// Note: This is a bit hacky and not the most obvious place to do it, but I don't know where
	//       otherwise
	ComPtr<ID3D12PipelineState> swapchain_copy_pso;
	ComPtr<ID3D12RootSignature> swapchain_copy_root_sig;
	{
		constexpr char SWAPCHAIN_COPY_SHADER_SRC[] = R"(
			struct LaunchParamsSwapchainCopy {
				int2 swapchain_res;
				uint padding0;
				uint padding1;
			};
			GPU_DECLARE_LAUNCH_PARAMS(LaunchParamsSwapchainCopy, params);

			struct FullscreenTriVertex {
				float2 pos;
				float2 texcoord;
			};

			static const FullscreenTriVertex fullscreen_tri_vertices[3] = {
				{ float2(-1.0f, -1.0f), float2(0.0f, 1.0f) }, // Bottom left
				{ float2(3.0f, -1.0f), float2(2.0f, 1.0f) }, // Bottom right
				{ float2(-1.0f, 3.0f), float2(0.0f, -1.0f) }, // Top left
			};

			struct VSOutput {
				float2 texcoord : PARAM_0;
				float4 pos : SV_Position;
			};

			VSOutput VSMain(uint vertex_idx : SV_VertexID)
			{
				FullscreenTriVertex v;
				if (vertex_idx == 0) v = fullscreen_tri_vertices[0];
				if (vertex_idx == 1) v = fullscreen_tri_vertices[1];
				if (vertex_idx == 2) v = fullscreen_tri_vertices[2];
				VSOutput output;
				output.texcoord = v.texcoord;
				output.pos = float4(v.pos, 0.0f, 1.0f);
				return output;
			}

			float4 PSMain(float2 texcoord : PARAM_0) : SV_TARGET
			{
				const int2 idx = int2(float2(params.swapchain_res) * texcoord);

				// Read old value from swapchain tex
				Texture2D<float4> swapchain_tex = getTex(GPU_SWAPCHAIN_TEX_IDX);
				const float3 val = swapchain_tex[idx].rgb;

				// Write the value from the swapchain tex to the actual swapchain
				return float4(val.rgb, 1.0);
			}
		)";
		constexpr u32 SWAPCHAIN_COPY_SHADER_SRC_SIZE = sizeof(SWAPCHAIN_COPY_SHADER_SRC);

		// Append prolog to shader source (can probably be done at compile time, but dunno how and
		// not worth the effort to figure out).
		char* src = (char*)cfg.cpu_allocator->alloc(
			sfz_dbg(""), GPU_KERNEL_PROLOG_SIZE + SWAPCHAIN_COPY_SHADER_SRC_SIZE);
		sfz_defer[=]() {
			cfg.cpu_allocator->dealloc(src);
		};
		memcpy(src, GPU_KERNEL_PROLOG, GPU_KERNEL_PROLOG_SIZE);
		memcpy(src + GPU_KERNEL_PROLOG_SIZE, SWAPCHAIN_COPY_SHADER_SRC, SWAPCHAIN_COPY_SHADER_SRC_SIZE);
		const u32 src_size = GPU_KERNEL_PROLOG_SIZE + SWAPCHAIN_COPY_SHADER_SRC_SIZE;

		// Compile shaders
		ComPtr<IDxcBlob> vs_dxil_blob;
		ComPtr<IDxcBlob> ps_dxil_blob;
		{
			// Create source blobs
			ComPtr<IDxcBlobEncoding> source_blob;
			bool succ = CHECK_D3D12_INIT(dxc_utils->CreateBlob(src, src_size, CP_UTF8, &source_blob));
			sfz_assert(succ);
			DxcBuffer src_buffer = {};
			src_buffer.Ptr = source_blob->GetBufferPointer();
			src_buffer.Size = source_blob->GetBufferSize();
			src_buffer.Encoding = 0;

			// Compiler arguments
			constexpr u32 NUM_ARGS = 11;
			LPCWSTR VS_ARGS[NUM_ARGS] = {
				L"-E",
				L"VSMain",
				L"-T",
				L"vs_6_6",
				L"-HV 2021",
				L"-enable-16bit-types",
				L"-O3",
				L"-Zi",
				L"-Qembed_debug",
				DXC_ARG_PACK_MATRIX_ROW_MAJOR,
				L"-DGPU_READ_ONLY_HEAP"
			};
			LPCWSTR PS_ARGS[NUM_ARGS] = {
				L"-E",
				L"PSMain",
				L"-T",
				L"ps_6_6",
				L"-HV 2021",
				L"-enable-16bit-types",
				L"-O3",
				L"-Zi",
				L"-Qembed_debug",
				DXC_ARG_PACK_MATRIX_ROW_MAJOR,
				L"-DGPU_READ_ONLY_HEAP"
			};

			// Compile shaders
			ComPtr<IDxcResult> vs_compile_res;
			CHECK_D3D12_INIT(dxc_compiler->Compile(
				&src_buffer, VS_ARGS, NUM_ARGS, dxc_include_handler.Get(), IID_PPV_ARGS(&vs_compile_res)));
			{
				ComPtr<IDxcBlobUtf8> error_msgs;
				CHECK_D3D12_INIT(vs_compile_res->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&error_msgs), nullptr));
				if (error_msgs && error_msgs->GetStringLength() > 0) {
					GPU_LOG_ERROR_INIT("[gpu_lib]: %s\n", (const char*)error_msgs->GetBufferPointer());
				}

				HRESULT hr = {};
				CHECK_D3D12_INIT(vs_compile_res->GetStatus(&hr));
				const bool compile_success = CHECK_D3D12_INIT(hr);
				sfz_assert_hard(compile_success);
			}
			ComPtr<IDxcResult> ps_compile_res;
			CHECK_D3D12_INIT(dxc_compiler->Compile(
				&src_buffer, PS_ARGS, NUM_ARGS, dxc_include_handler.Get(), IID_PPV_ARGS(&ps_compile_res)));
			{
				ComPtr<IDxcBlobUtf8> error_msgs;
				CHECK_D3D12_INIT(ps_compile_res->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&error_msgs), nullptr));
				if (error_msgs && error_msgs->GetStringLength() > 0) {
					GPU_LOG_ERROR_INIT("[gpu_lib]: %s\n", (const char*)error_msgs->GetBufferPointer());
				}

				HRESULT hr = {};
				CHECK_D3D12_INIT(ps_compile_res->GetStatus(&hr));
				const bool compile_success = CHECK_D3D12_INIT(hr);
				sfz_assert_hard(compile_success);
			}

			// Get compiled DXIL
			CHECK_D3D12_INIT(vs_compile_res->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&vs_dxil_blob), nullptr));
			CHECK_D3D12_INIT(ps_compile_res->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&ps_dxil_blob), nullptr));
		}

		// Create root signature
		swapchain_copy_root_sig = gpuCreateDefaultRootSignature(gpu, false, sizeof(i32x4), "swapchain_copy_root_sig", true);
		sfz_assert_hard(swapchain_copy_root_sig != nullptr);

		// Create PSO (Pipeline State Object)
		{
			D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {};
			pso_desc.pRootSignature = swapchain_copy_root_sig.Get();
			pso_desc.VS.pShaderBytecode = vs_dxil_blob->GetBufferPointer();
			pso_desc.VS.BytecodeLength = vs_dxil_blob->GetBufferSize();
			pso_desc.PS.pShaderBytecode = ps_dxil_blob->GetBufferPointer();
			pso_desc.PS.BytecodeLength = ps_dxil_blob->GetBufferSize();

			pso_desc.BlendState.AlphaToCoverageEnable = FALSE;
			pso_desc.BlendState.IndependentBlendEnable = FALSE;
			pso_desc.BlendState.RenderTarget[0].BlendEnable = FALSE;
			pso_desc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
			pso_desc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
			pso_desc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			pso_desc.SampleMask = U32_MAX;

			pso_desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
			pso_desc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
			pso_desc.RasterizerState.FrontCounterClockwise = true;
			pso_desc.RasterizerState.DepthClipEnable = true;

			pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

			pso_desc.NumRenderTargets = 1;
			pso_desc.RTVFormats[0] = GPU_SWAPCHAIN_DXGI_FORMAT;
			pso_desc.DSVFormat = DXGI_FORMAT_UNKNOWN;

			pso_desc.SampleDesc = { 1, 0 };

			const bool pso_success = CHECK_D3D12(gpu->device->CreateGraphicsPipelineState(
				&pso_desc, IID_PPV_ARGS(&swapchain_copy_pso)));
			sfz_assert_hard(pso_success);
			setDebugName(swapchain_copy_pso.Get(), "swapchain_copy_pso");
		}
	}
	gpu->swapchain_copy_pso = swapchain_copy_pso;
	gpu->swapchain_copy_root_sig = swapchain_copy_root_sig;

	return gpu;
}

sfz_extern_c void gpuLibDestroy(GpuLib* gpu)
{
	if (gpu == nullptr) return;

	// Flush all in-flight commands
	gpuFlushSubmittedWork(gpu);

	// Destroy native extensions
	GpuNativeExt* native_exts = gpu->native_exts.data();
	const SfzPoolSlot* slots = gpu->native_exts.slots();
	const u32 array_size = gpu->native_exts.arraySize();
	for (u32 idx = 0; idx < array_size; idx++) {
		const SfzPoolSlot slot = slots[idx];
		if (!slot.active()) continue;
		GpuNativeExt& ext = native_exts[idx];
		if (ext.destroy_func != nullptr) {
			ext.destroy_func(gpu, ext.ext_data_ptr);
		}
	}

	// Destroy command queue's fence event
	CloseHandle(gpu->cmd_queue_fence_event);

	SfzAllocator* allocator = gpu->cfg.cpu_allocator;
	sfz_delete(allocator, gpu);
}

// Native Extension API
// ------------------------------------------------------------------------------------------------

sfz_extern_c GpuNativeExtHandle gpuNativeExtRegister(GpuLib* gpu, const GpuNativeExt* ext_in)
{
	const SfzHandle handle = gpu->native_exts.allocate();
	if (handle == SFZ_NULL_HANDLE) {
		GPU_LOG_ERROR("[gpu_lib]: Can't allocate slot for native extension.");
		return GPU_NULL_NATIVE_EXT;
	}

	GpuNativeExt& ext = *gpu->native_exts.get(handle);
	ext = *ext_in;

	const GpuNativeExtHandle ext_handle = { handle.bits };
	return ext_handle;
}

sfz_extern_c void gpuNativeExtRun(GpuLib* gpu, GpuNativeExtHandle ext_handle, void* params, u32 params_size)
{
	const SfzHandle handle = { ext_handle.handle };
	GpuNativeExt* ext = gpu->native_exts.get(handle);
	if (ext == nullptr) {
		GPU_LOG_ERROR("[gpu_lib]: Native extension is not registered.");
		return;
	}
	if (ext->run_func != nullptr) {
		ext->run_func(gpu, ext->ext_data_ptr, params, params_size);
	}
}

// Memory API
// ------------------------------------------------------------------------------------------------

sfz_extern_c GpuPtr gpuMalloc(GpuLib* gpu, u32 num_bytes)
{
	// TODO: This is obviously a very bad malloc API, please implement real malloc/free.

	// Check if we have enough space left
	const u32 end = gpu->gpu_heap_next_free + num_bytes;
	if (gpu->cfg.gpu_heap_size_bytes < end) {
		GPU_LOG_ERROR("[gpu_lib]: Out of GPU memory, trying to allocate %.3f MiB.",
			gpuPrintToMiB(num_bytes));
		return GPU_NULLPTR;
	}

	// Get pointer
	const GpuPtr ptr = gpu->gpu_heap_next_free;
	gpu->gpu_heap_next_free = sfzRoundUpAlignedU32(end, GPU_MALLOC_ALIGN);
	return ptr;
}

sfz_extern_c void gpuFree(GpuLib* gpu, GpuPtr ptr)
{
	(void)gpu;
	(void)ptr;
	// TODO: This is obviously a very bad free API, please implement real malloc/free.
}

// Constant buffer API
// ------------------------------------------------------------------------------------------------

sfz_extern_c GpuConstBuffer gpuConstBufferInit(GpuLib* gpu, u32 num_bytes, const char* name)
{
	sfz_assert(name != nullptr);

	ComPtr<ID3D12Resource> buffer;
	{
		D3D12_HEAP_PROPERTIES heap_props = {};
		heap_props.Type = D3D12_HEAP_TYPE_DEFAULT;
		heap_props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heap_props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heap_props.CreationNodeMask = 0;
		heap_props.VisibleNodeMask = 0;

		D3D12_RESOURCE_DESC res_desc = {};
		res_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		res_desc.Alignment = 0;
		res_desc.Width = num_bytes;
		res_desc.Height = 1;
		res_desc.DepthOrArraySize = 1;
		res_desc.MipLevels = 1;
		res_desc.Format = DXGI_FORMAT_UNKNOWN;
		res_desc.SampleDesc.Count = 1;
		res_desc.SampleDesc.Quality = 0;
		res_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		res_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

		const bool success = CHECK_D3D12(gpu->device->CreateCommittedResource(
			&heap_props,
			D3D12_HEAP_FLAG_CREATE_NOT_ZEROED,
			&res_desc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(&buffer)));
		if (!success) {
			GPU_LOG_ERROR("[gpu_lib]: (%s) Could not allocate constant buffer of size %u bytes.",
				name, num_bytes);
			return GPU_NULL_CBUFFER;
		}
		setDebugName(buffer.Get(), name);
	}

	const SfzHandle handle = gpu->const_buffers.allocate();
	if (handle == SFZ_NULL_HANDLE) {
		GPU_LOG_ERROR("[gpu_lib]: (\"%s\") Could not allocate slot for const buffer, out of slots.", name);
		return GPU_NULL_CBUFFER;
	}
	GpuConstBufferInfo& info = *gpu->const_buffers.get(handle);
	info.buffer = buffer;
	info.size_bytes = num_bytes;
	info.state = D3D12_RESOURCE_STATE_COMMON;
	info.last_upload_submit_idx = 0;
	return GpuConstBuffer{ handle.bits };
}

sfz_extern_c void gpuConstBufferDestroy(GpuLib* gpu, GpuConstBuffer cbuf)
{
	const SfzHandle handle = { cbuf.handle };
	GpuConstBufferInfo* info = gpu->const_buffers.get(handle);
	if (info == nullptr) {
		GPU_LOG_ERROR("[gpu_lib]: Trying to destroy a const buffer that doesn't exist.");
		return;
	}
	gpu->const_buffers.deallocate(handle);
}

// Textures API
// ------------------------------------------------------------------------------------------------

sfz_extern_c const char* gpuFormatToString(GpuFormat format)
{
	switch (format) {
	case GPU_FORMAT_UNDEFINED: return "GPU_FORMAT_UNDEFINED";

	case GPU_FORMAT_R_U8_UNORM: return "GPU_FORMAT_R_U8_UNORM";
	case GPU_FORMAT_RG_U8_UNORM: return "GPU_FORMAT_RG_U8_UNORM";
	case GPU_FORMAT_RGBA_U8_UNORM: return "GPU_FORMAT_RGBA_U8_UNORM";

	case GPU_FORMAT_R_U16_UNORM: return "GPU_FORMAT_R_U16_UNORM";
	case GPU_FORMAT_RG_U16_UNORM: return "GPU_FORMAT_RG_U16_UNORM";
	case GPU_FORMAT_RGBA_U16_UNORM: return "GPU_FORMAT_RGBA_U16_UNORM";

	case GPU_FORMAT_R_U8_SNORM: return "GPU_FORMAT_R_U8_SNORM";
	case GPU_FORMAT_RG_U8_SNORM: return "GPU_FORMAT_RG_U8_SNORM";
	case GPU_FORMAT_RGBA_U8_SNORM: return "GPU_FORMAT_RGBA_U8_SNORM";

	case GPU_FORMAT_R_U16_SNORM: return "GPU_FORMAT_R_U16_SNORM";
	case GPU_FORMAT_RG_U16_SNORM: return "GPU_FORMAT_RG_U16_SNORM";
	case GPU_FORMAT_RGBA_U16_SNORM: return "GPU_FORMAT_RGBA_U16_SNORM";

	case GPU_FORMAT_R_F16: return "GPU_FORMAT_R_F16";
	case GPU_FORMAT_RG_F16: return "GPU_FORMAT_RG_F16";
	case GPU_FORMAT_RGBA_F16: return "GPU_FORMAT_RGBA_F16";

	case GPU_FORMAT_R_F32: return "GPU_FORMAT_R_F32";
	case GPU_FORMAT_RG_F32: return "GPU_FORMAT_RG_F32";
	case GPU_FORMAT_RGBA_F32: return "GPU_FORMAT_RGBA_F32";

	default: break;
	}
	sfz_assert(false);
	return "UNKNOWN";
}

sfz_extern_c const char* gpuTexStateToString(GpuTexState state)
{
	switch (state) {
	case GPU_TEX_STATE_UNDEFINED: return "GPU_TEX_STATE_UNDEFINED";
	case GPU_TEX_READ_ONLY: return "GPU_TEX_READ_ONLY";
	case GPU_TEX_READ_WRITE: return "GPU_TEX_READ_WRITE";
	default: break;
	}
	sfz_assert(false);
	return "UNKNOWN";
}

static GpuTexIdx gpuTexInitInternal(GpuLib* gpu, GpuTexDesc desc, const SfzHandle* existing_handle = nullptr)
{
	sfz_assert(desc.name != nullptr);
	desc.num_mips = i32_clamp(desc.num_mips, 1, GPU_MAX_NUM_MIPS);
	if (desc.tex_state == GPU_TEX_STATE_UNDEFINED) desc.tex_state = GPU_TEX_READ_ONLY;
	if (desc.format == GPU_FORMAT_UNDEFINED) {
		GPU_LOG_ERROR("[gpu_lib]: (\"%s\") Must specify a valid texture format when creating a texture.", desc.name);
		return GPU_NULL_TEX;
	}
	if (desc.swapchain_relative && desc.relative_fixed_height != 0 && desc.relative_scale != 0.0f) {
		GPU_LOG_ERROR("[gpu_lib]: (\"%s\") For swapchain relative textures either fixed height or scale MUST be 0.", desc.name);
		return GPU_NULL_TEX;
	}
	if (desc.swapchain_relative && desc.num_mips != 1) {
		GPU_LOG_ERROR("[gpu_lib]: (\"%s\") Swapchain relative textures may not have mipmaps.", desc.name);
		return GPU_NULL_TEX;
	}
	if (desc.num_mips > 1) {
		if (!sfzIsPow2_u32(u32(desc.fixed_res.x)) || !sfzIsPow2_u32(u32(desc.fixed_res.y))) {
			GPU_LOG_ERROR("[gpu_lib]: (\"%s\") Texture with mipmaps must have power of 2 fixed resolution (%ix%i is invalid).",
				desc.name, desc.fixed_res.x, desc.fixed_res.y);
			return GPU_NULL_TEX;
		}
	}
	if (desc.tex_state != GPU_TEX_READ_ONLY && desc.tex_state != GPU_TEX_READ_WRITE) {
		GPU_LOG_ERROR("[gpu_lib]: (\"%s\") Invalid initial texture state.", desc.name);
		return GPU_NULL_TEX;
	}
	const i32x2 tex_res = calcTexTargetRes(gpu->swapchain_res, &desc);

	// Reduce the number of mips if too many are requested
	if (desc.num_mips > 1) {
		const u32 low_width = u32_max(u32(log2(tex_res.x)), 1u);
		const u32 log_height = u32_max(u32(log2(tex_res.y)), 1u);
		const u32 log_min_dim = u32_min(low_width, log_height);
		const u32 max_possible_num_mips = u32_min(log_min_dim, GPU_MAX_NUM_MIPS);
		desc.num_mips = u32_min(desc.num_mips, max_possible_num_mips);
	}

	// Allocate texture resource
	ComPtr<ID3D12Resource> tex;
	{
		D3D12_HEAP_PROPERTIES heap_props = {};
		heap_props.Type = D3D12_HEAP_TYPE_DEFAULT;
		heap_props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heap_props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heap_props.CreationNodeMask = 0;
		heap_props.VisibleNodeMask = 0;

		D3D12_RESOURCE_DESC res_desc = {};
		res_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		res_desc.Alignment = 0;
		res_desc.Width = u32(tex_res.x);
		res_desc.Height = u32(tex_res.y);
		res_desc.DepthOrArraySize = 1;
		res_desc.MipLevels = u16(desc.num_mips);
		res_desc.Format = formatToD3D12(desc.format);
		res_desc.SampleDesc = { 1, 0 };
		res_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		res_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		const bool success = CHECK_D3D12(gpu->device->CreateCommittedResource(
			&heap_props,
			D3D12_HEAP_FLAG_NONE,
			&res_desc,
			texStateToD3D12(desc.tex_state),
			nullptr,
			IID_PPV_ARGS(&tex)));
		if (!success) {
			GPU_LOG_ERROR("[gpu_lib]: (\"%s\") Could not allocate texture of size %ix%i, %i mips and format %s.",
				desc.name, tex_res.x, tex_res.y, desc.num_mips, gpuFormatToString(desc.format));
			return GPU_NULL_TEX;
		}
		setDebugName(tex.Get(), desc.name);
	}

	// Allocate slot in rwtex array
	SfzHandle handle = SFZ_NULL_HANDLE;
	if (existing_handle != nullptr) {
		handle = *existing_handle;
	}
	else {
		handle = gpu->textures.allocate();
	}
	if (handle == SFZ_NULL_HANDLE) {
		GPU_LOG_ERROR("[gpu_lib]: (\"%s\") Could not allocate slot for texture, out of slots.", desc.name);
		return GPU_NULL_TEX;
	}

	// Store info about texture
	GpuTexInfo& info = *gpu->textures.get(handle);
	info.tex = tex;
	info.tex_res = tex_res;
	info.desc = desc;
	info.name = sfzStr96Init(desc.name);
	info.desc.name = info.name.str; // Need to repoint name, otherwise potential use after free.

	// Set descriptor in tex descriptor heap
	const GpuTexIdx tex_idx = GpuTexIdx(handle.idx());
	texSetDescriptors(gpu, tex_idx);

	return tex_idx;
}

sfz_extern_c GpuTexIdx gpuTexInit(GpuLib* gpu, const GpuTexDesc* desc)
{
	return gpuTexInitInternal(gpu, *desc);
}

sfz_extern_c void gpuTexDestroy(GpuLib* gpu, GpuTexIdx tex_idx)
{
	const SfzHandle handle = gpu->textures.getHandle(tex_idx);
	GpuTexInfo* tex_info = gpu->textures.get(handle);
	if (tex_info == nullptr) {
		GPU_LOG_ERROR("[gpu_lib]: Trying to destroy a texture that doesn't exist.");
		return;
	}
	texSetNullDescriptors(gpu, tex_idx);
	gpu->textures.deallocate(handle);
}

sfz_extern_c const GpuTexDesc* gpuTexGetDesc(const GpuLib* gpu, GpuTexIdx tex_idx)
{
	const SfzHandle handle = gpu->textures.getHandle(tex_idx);
	const GpuTexInfo* tex_info = gpu->textures.get(handle);
	if (tex_info == nullptr) return nullptr;
	return &tex_info->desc;
}

sfz_extern_c i32x2 gpuTexGetRes(const GpuLib* gpu, GpuTexIdx tex_idx)
{
	const SfzHandle handle = gpu->textures.getHandle(tex_idx);
	const GpuTexInfo* tex_info = gpu->textures.get(handle);
	if (tex_info == nullptr) return i32x2_splat(0);
	return tex_info->tex_res;
}

sfz_extern_c GpuTexState gpuTexGetState(const GpuLib* gpu, GpuTexIdx tex_idx)
{
	const SfzHandle handle = gpu->textures.getHandle(tex_idx);
	const GpuTexInfo* tex_info = gpu->textures.get(handle);
	if (tex_info == nullptr) return GPU_TEX_STATE_UNDEFINED;
	return tex_info->desc.tex_state;
}

sfz_extern_c void gpuTexSetSwapchainRelativeScale(GpuLib* gpu, GpuTexIdx tex_idx, f32 scale)
{
	const SfzHandle handle = gpu->textures.getHandle(tex_idx);
	GpuTexInfo* tex_info = gpu->textures.get(handle);
	if (tex_info == nullptr) {
		GPU_LOG_ERROR("[gpu_lib]: Trying to set relative scale of a texture that doesn't exist (%u).",
			u32(tex_idx));
		return;
	}
	if (!tex_info->desc.swapchain_relative) {
		GPU_LOG_ERROR("[gpu_lib]: Trying to set relative scale of a texture that is not swapchain relative (%u).",
			u32(tex_idx));
		return;
	}

	// Just return if we already have the correct scale
	if (tex_info->desc.relative_scale == scale) return;

	// Rebuild texture
	// Some copying to avoid aliasing issues
	SfzStr96 name = tex_info->name;
	tex_info->desc.relative_fixed_height = 0;
	tex_info->desc.relative_scale = scale;
	tex_info->desc.name = name.str;
	gpuTexInitInternal(gpu, tex_info->desc, &handle);
}

sfz_extern_c void gpuTexSetSwapchainRelativeFixedHeight(GpuLib* gpu, GpuTexIdx tex_idx, i32 height)
{
	const SfzHandle handle = gpu->textures.getHandle(tex_idx);
	GpuTexInfo* tex_info = gpu->textures.get(handle);
	if (tex_info == nullptr) {
		GPU_LOG_ERROR("[gpu_lib]: Trying to set relative fixed height of a texture that doesn't exist (%u).",
			u32(tex_idx));
		return;
	}
	if (!tex_info->desc.swapchain_relative) {
		GPU_LOG_ERROR("[gpu_lib]: Trying to set relative fixed height of a texture that is not swapchain relative (%u).",
			u32(tex_idx));
		return;
	}

	// Just return if we already have the correct fixed height
	if (tex_info->desc.relative_fixed_height == height) return;

	// Rebuild texture
	// Some copying to avoid aliasing issues
	SfzStr96 name = tex_info->name;
	tex_info->desc.relative_fixed_height = height;
	tex_info->desc.relative_scale = 0.0f;
	tex_info->desc.name = name.str;
	gpuTexInitInternal(gpu, tex_info->desc, &handle);
}

// Kernel API
// ------------------------------------------------------------------------------------------------

static GpuKernel gpuKernelInitInternal(GpuLib* gpu, const GpuKernelDesc* desc, const SfzHandle* existing_handle = nullptr)
{
	sfz_assert(desc->name != nullptr);
	const SfzTime start_time = sfzTimeNow();

	// Read shader file from disk
	u32 src_size = 0;
	char* src = nullptr;
	{
		// Map shader file
		FileMapData src_map = fileMap(gpu, desc->path, true);
		if (src_map.ptr == nullptr) {
			GPU_LOG_ERROR("[gpulib]: (\"%s\") Failed to map kernel source file \"%s\".", desc->name, desc->path);
			return GPU_NULL_KERNEL;
		}
		sfz_defer[=]() { fileUnmap(gpu, src_map); };

		// Allocate memory for src + prolog
		src_size = u32(src_map.size_bytes + GPU_KERNEL_PROLOG_SIZE);
		src = static_cast<char*>(gpu->cfg.cpu_allocator->alloc(sfz_dbg(""), src_size + 1));

		// Copy prolog and then src file into buffer
		memcpy(src, GPU_KERNEL_PROLOG, GPU_KERNEL_PROLOG_SIZE);
		memcpy(src + GPU_KERNEL_PROLOG_SIZE, src_map.ptr, src_map.size_bytes);
		src[src_size] = '\0'; // Guarantee null-termination, safe because we allocated 1 byte extra.
	}
	sfz_defer[=]() { gpu->cfg.cpu_allocator->dealloc(src); };

	// Compile shader
	ComPtr<IDxcBlob> dxil_blob;
	i32x3 group_dims = i32x3_splat(0);
	u32 const_buffer_size = 0;
	u32 launch_params_size = 0;
	{
		// Create source blob
		ComPtr<IDxcBlobEncoding> source_blob;
		if (!CHECK_D3D12(gpu->dxc_utils->CreateBlob(src, src_size, CP_UTF8, &source_blob))) {
			GPU_LOG_ERROR("[gpulib]: (\"%s\") Failed to create source blob", desc->name);
			return GPU_NULL_KERNEL;
		}
		DxcBuffer src_buffer = {};
		src_buffer.Ptr = source_blob->GetBufferPointer();
		src_buffer.Size = source_blob->GetBufferSize();
		src_buffer.Encoding = 0;

		// Extract defines (if any) from defines string
		u32 num_defines = 0;
		struct WideDefine { wchar_t str[GPU_KERNEL_DEFINE_MAX_LEN + 3] = {}; };
		WideDefine defines_wide[GPU_KERNEL_MAX_NUM_DEFINES] = {};
		if (desc->defines != nullptr) {
			const u32 defines_len = u32(strlen(desc->defines));
			if (defines_len >= GPU_KERNEL_DEFINES_STR_MAX_LEN) {
				GPU_LOG_ERROR("[gpulib]: (\"%s\") Defines string is %u chars, max %u allowed.",
					desc->name, defines_len, GPU_KERNEL_DEFINES_STR_MAX_LEN);
				return GPU_NULL_KERNEL;
			}
			const char* define_max = desc->defines + defines_len;
			const char* curr_define_begin = desc->defines;
			while (true) {
				const char* curr_define_end = strchr(curr_define_begin, ' ');

				// Get length of define
				u32 define_len = 0;
				if (curr_define_end == nullptr) {
					define_len = u32(strlen(curr_define_begin));
				}
				else {
					define_len = u32(curr_define_end - curr_define_begin);
					if (define_max < curr_define_end) break;
				}
				if (define_len <= 1) {
					curr_define_begin += 1;
					if (*curr_define_begin == '\0') break;
					continue;
				}
				if (define_len >= GPU_KERNEL_DEFINE_MAX_LEN) {
					GPU_LOG_ERROR("[gpulib]: (\"%s\") Too long define %u chars, max %u allowed",
						desc->name, define_len, GPU_KERNEL_DEFINE_MAX_LEN);
					return GPU_NULL_KERNEL;
				}

				// Extract define
				const u32 define_idx = num_defines;
				if (define_idx >= GPU_KERNEL_MAX_NUM_DEFINES) {
					GPU_LOG_ERROR("[gpulib]: (\"%s\") Too many defines in define string, max %u allowed.",
						desc->name, defines_len, GPU_KERNEL_MAX_NUM_DEFINES);
					return GPU_NULL_KERNEL;
				}
				wchar_t* wide_str = defines_wide[define_idx].str;
				wide_str[0] = L'-';
				wide_str[1] = L'D';
				utf8ToWide(wide_str + 2, define_len, curr_define_begin, define_len);
				num_defines += 1;

				// Exit out if we are at end of string, otherwise move begin forward
				if (curr_define_end == nullptr) break;
				curr_define_begin = curr_define_end + 1;
			}
		}

		// Compiler arguments
		//
		// Consider adding: "-all-resources-bound"
		//     Compiler will assume that all resources that a shader may reference are bound and
		//     are in good state for the duration of shader execution. Recommended by Nvidia.
		constexpr u32 NUM_ARGS_BASE = 11;
		constexpr u32 MAX_NUM_ARGS = NUM_ARGS_BASE + GPU_KERNEL_MAX_NUM_DEFINES;
		const u32 num_args = NUM_ARGS_BASE + num_defines;
		LPCWSTR args[MAX_NUM_ARGS] = {
			L"-E",
			L"CSMain",
			L"-T",
			L"cs_6_6",
			L"-HV 2021",
			L"-enable-16bit-types",
			L"-O3",
			L"-Zi",
			L"-Qembed_debug",
			DXC_ARG_PACK_MATRIX_ROW_MAJOR,
			desc->write_enabled_heap ? L"-DGPU_READ_WRITE_HEAP" : L"-DGPU_READ_ONLY_HEAP"
		};
		for (u32 i = 0; i < num_defines; i++) {
			args[NUM_ARGS_BASE + i] = defines_wide[i].str;
		}

		// Compile shader
		ComPtr<IDxcResult> compile_res;
		CHECK_D3D12(gpu->dxc_compiler->Compile(
			&src_buffer, args, num_args, gpu->dxc_include_handler.Get(), IID_PPV_ARGS(&compile_res)));
		{
			ComPtr<IDxcBlobUtf8> error_msgs;
			CHECK_D3D12(compile_res->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&error_msgs), nullptr));
			if (error_msgs && error_msgs->GetStringLength() > 0) {
				GPU_LOG_ERROR("[gpu_lib]: (\"%s\") %s", desc->name, (const char*)error_msgs->GetBufferPointer());
			}

			ComPtr<IDxcBlobUtf8> remarks;
			CHECK_D3D12(compile_res->GetOutput(DXC_OUT_REMARKS, IID_PPV_ARGS(&remarks), nullptr));
			if (remarks && remarks->GetStringLength() > 0) {
				GPU_LOG_ERROR("[gpu_lib]: (\"%s\") %s", desc->name, (const char*)remarks->GetBufferPointer());
			}

			HRESULT hr = {};
			CHECK_D3D12(compile_res->GetStatus(&hr));
			const bool compile_success = CHECK_D3D12(hr);
			if (!compile_success) {
				GPU_LOG_ERROR("[gpu_lib]: (\"%s\") Failed to compile kernel", desc->name);
				return GPU_NULL_KERNEL;
			}
		}

		// Get compiled DXIL
		CHECK_D3D12(compile_res->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&dxil_blob), nullptr));

		// Get reflection data
		ComPtr<IDxcBlob> reflection_data;
		CHECK_D3D12(compile_res->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(&reflection_data), nullptr));
		DxcBuffer reflection_buffer = {};
		reflection_buffer.Ptr = reflection_data->GetBufferPointer();
		reflection_buffer.Size = reflection_data->GetBufferSize();
		reflection_buffer.Encoding = 0;
		ComPtr<ID3D12ShaderReflection> reflection;
		CHECK_D3D12(gpu->dxc_utils->CreateReflection(&reflection_buffer, IID_PPV_ARGS(&reflection)));

		// Get group dimensions from reflection
		u32 group_dim_x = 0, group_dim_y = 0, group_dim_z = 0;
		reflection->GetThreadGroupSize(&group_dim_x, &group_dim_y, &group_dim_z);
		group_dims = i32x3_init((i32)group_dim_x, (i32)group_dim_y, (i32)group_dim_z);

		// Get constant buffer and launch parameters info from reflection
		D3D12_SHADER_DESC shader_desc = {};
		CHECK_D3D12(reflection->GetDesc(&shader_desc));
		if (shader_desc.ConstantBuffers > 2) {
			GPU_LOG_ERROR("[gpu_lib]: (\"%s\") More than 2 constant buffer bound, not allowed.", desc->name);
			return GPU_NULL_KERNEL;
		}
		for (u32 i = 0; i < shader_desc.BoundResources; i++) {
			D3D12_SHADER_INPUT_BIND_DESC bind_desc = {};
			CHECK_D3D12(reflection->GetResourceBindingDesc(i, &bind_desc));
			if (bind_desc.Type != D3D_SIT_CBUFFER) continue;

			D3D12_SHADER_BUFFER_DESC cbuffer = {};
			CHECK_D3D12(reflection->GetConstantBufferByName(bind_desc.Name)->GetDesc(&cbuffer));

			sfz_assert(bind_desc.BindCount == 1);
			sfz_assert(bind_desc.Space == 0);
			const u32 reg = bind_desc.BindPoint;
			if (reg == GPU_CONST_BUFFER_SHADER_REG) {
				const_buffer_size = cbuffer.Size;
			}
			else if (reg == GPU_LAUNCH_PARAMS_SHADER_REG) {
				launch_params_size = cbuffer.Size;
				if (launch_params_size > GPU_LAUNCH_PARAMS_MAX_SIZE) {
					GPU_LOG_ERROR("[gpu_lib]: (\"%s\") Launch parameters too big, %u bytes, max %u bytes allowed.",
						desc->name, launch_params_size, GPU_LAUNCH_PARAMS_MAX_SIZE);
					return GPU_NULL_KERNEL;
				}
			}
			else {
				GPU_LOG_ERROR("[gpu_lib]: (\"%s\") Invalid constant buffer bound to register %u.",
					desc->name, reg);
				return GPU_NULL_KERNEL;
			}
		}
	}

	const SfzTime dxil_done_time = sfzTimeNow();

	// Create root signature
	ComPtr<ID3D12RootSignature> root_sig = gpuCreateDefaultRootSignature(
		gpu, desc->write_enabled_heap, launch_params_size, desc->name);
	if (root_sig == nullptr) return GPU_NULL_KERNEL;

	const SfzTime root_sig_done_time = sfzTimeNow();

	// Create PSO (Pipeline State Object)
	ComPtr<ID3D12PipelineState> pso;
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC pso_desc = {};
		pso_desc.pRootSignature = root_sig.Get();
		pso_desc.CS.pShaderBytecode = dxil_blob->GetBufferPointer();
		pso_desc.CS.BytecodeLength = dxil_blob->GetBufferSize();
		pso_desc.NodeMask = 0;
		pso_desc.CachedPSO = {};
		pso_desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

		const bool pso_success = CHECK_D3D12(gpu->device->CreateComputePipelineState(
			&pso_desc, IID_PPV_ARGS(&pso)));
		if (!pso_success) {
			GPU_LOG_ERROR("[gpu_lib]: (\"%s\") Failed to create PSO.", desc->name);
			return GPU_NULL_KERNEL;
		}
		setDebugName(pso.Get(), desc->name);
	}

	const SfzTime pso_done_time = sfzTimeNow();

	// Store kernel data
	SfzHandle handle = SFZ_NULL_HANDLE;
	if (existing_handle != nullptr) {
		handle = *existing_handle;
	}
	else {
		handle = gpu->kernels.allocate();
	}
	if (handle == SFZ_NULL_HANDLE) {
		GPU_LOG_ERROR("[gpu_lib]: (\"%s\") Too many kernels, could not allocate handle.", desc->name);
		return GPU_NULL_KERNEL;
	}
	GpuKernelInfo& kernel_info = *gpu->kernels.get(handle);
	kernel_info.pso = pso;
	kernel_info.root_sig = root_sig;
	kernel_info.group_dims = group_dims;
	kernel_info.const_buffer_size = const_buffer_size;
	kernel_info.launch_params_size = launch_params_size;

	// Store desc (need to repoint name, path and defines to avoid invalid dangling pointers). Also,
	// don't store desc if this was a reload.
	if (desc != &kernel_info.desc) {
		sfz_assert(existing_handle == nullptr);
		kernel_info.name = sfzStr96Init(desc->name);
		kernel_info.path = sfzStr320Init(desc->path);
		kernel_info.desc = *desc;
		kernel_info.desc.name = kernel_info.name.str;
		kernel_info.desc.path = kernel_info.path.str;
		kernel_info.defines = {};
		if (desc->defines != nullptr) {
			kernel_info.defines = sfzStr320Init(desc->defines);
			kernel_info.desc.defines = kernel_info.defines.str;
		}
	}

	// Log kernel info
	const SfzTime end_time = sfzTimeNow();
	const f32 compile_time = sfzTimeDiff(start_time, end_time).ms();
	const f32 dxil_time = sfzTimeDiff(start_time, dxil_done_time).ms();
	const f32 root_sig_time = sfzTimeDiff(dxil_done_time, root_sig_done_time).ms();
	const f32 pso_time = sfzTimeDiff(root_sig_done_time, pso_done_time).ms();
	GPU_LOG_INFO(
R"([gpu_lib]: Compiled kernel "%s"
 - Path: "%s"
 - Defines: "%s"
 - Group dims: %ix%ix%i
 - Write enabled heap: %s
 - Const buffer: %u words (%u bytes)
 - Launch params: %u words (%u bytes)
 - Compile time: %.2fms (DXIL %.2fms, Root sig: %.2fms, PSO: %.2fms))",
		desc->name,
		desc->path,
		(desc->defines != nullptr ? desc->defines : ""),
		group_dims.x, group_dims.y, group_dims.z,
		desc->write_enabled_heap ? "True" : "False",
		const_buffer_size / sizeof(u32), const_buffer_size,
		launch_params_size / sizeof(u32), launch_params_size,
		compile_time, dxil_time, root_sig_time, pso_time);

	return GpuKernel{ handle.bits };
}

sfz_extern_c GpuKernel gpuKernelInit(GpuLib* gpu, const GpuKernelDesc* desc)
{
	return gpuKernelInitInternal(gpu, desc);
}

sfz_extern_c bool gpuKernelReload(GpuLib* gpu, GpuKernel kernel)
{
	const SfzHandle handle = SfzHandle{ kernel.handle };
	GpuKernelInfo* info = gpu->kernels.get(handle);
	if (info == nullptr) return false;
	gpuFlushSubmittedWork(gpu);
	GpuKernel res_handle = gpuKernelInitInternal(gpu, &info->desc, &handle);
	return res_handle != GPU_NULL_KERNEL;
}

sfz_extern_c void gpuKernelDestroy(GpuLib* gpu, GpuKernel kernel)
{
	const SfzHandle handle = SfzHandle{ kernel.handle };
	GpuKernelInfo* info = gpu->kernels.get(handle);
	if (info == nullptr) return;
	gpu->kernels.deallocate(handle);
}

sfz_extern_c i32x3 gpuKernelGetGroupDims(const GpuLib* gpu, GpuKernel kernel)
{
	const SfzHandle handle = SfzHandle{ kernel.handle };
	const GpuKernelInfo* info = gpu->kernels.get(handle);
	if (info == nullptr) return i32x3_splat(0);
	return info->group_dims;
}

// Command API
// ------------------------------------------------------------------------------------------------

sfz_extern_c u64 gpuGetCurrSubmitIdx(const GpuLib* gpu)
{
	return gpu->curr_submit_idx;
}

sfz_extern_c i32x2 gpuSwapchainGetRes(const GpuLib* gpu)
{
	return gpu->swapchain_res;
}

sfz_extern_c void gpuQueueEventBegin(GpuLib* gpu, const char* name, const f32x4* optional_color)
{
	// D3D12_EVENT_METADATA defintion
	constexpr u32 WINPIX_EVENT_PIX3BLOB_VERSION = 2;
	constexpr u32 D3D12_EVENT_METADATA = WINPIX_EVENT_PIX3BLOB_VERSION;

	// Buffer
	constexpr u64 PIXEventsGraphicsRecordSpaceQwords = 64;
	u64 buffer[PIXEventsGraphicsRecordSpaceQwords] = {};
	u64* dst = buffer;

	// Encode event info (timestamp = 0, PIXEvent_BeginEvent_NoArgs)
	constexpr u64 ENCODE_EVENT_INFO_CONSTANT = u64(2048);
	*dst++ = ENCODE_EVENT_INFO_CONSTANT;

	// Parse and encode color
	const f32x3 rgb = optional_color != nullptr ? optional_color->xyz() : f32x3_splat(0.0f);
	const u64 color = [](f32 r, f32 g, f32 b) -> u64 {
		BYTE rb = BYTE((r / 255.0f) + 0.5f);
		BYTE gb = BYTE((g / 255.0f) + 0.5f);
		BYTE bb = BYTE((b / 255.0f) + 0.5f);
		return u64(0xff000000u | (rb << 16) | (gb << 8) | bb);
	}(rgb.x, rgb.y, rgb.z);
	*dst++ = color;

	// Encode string info (alignment = 0, copyChunkSize = 8, isAnsi=true, isShortcut=false)
	constexpr u64 STRING_INFO_CONSTANT = u64(306244774661193728);
	*dst++ = STRING_INFO_CONSTANT;

	// Copy string
	constexpr u32 STRING_MAX_LEN = 20 * 8;
	const u32 name_len = u32(strnlen(name, STRING_MAX_LEN));
	memcpy(dst, name, name_len);
	dst += ((name_len / 8) + 1);

	// Call BeginEvent with our hacked together binary blob
	const UINT size_bytes = u32(reinterpret_cast<u8*>(dst) - reinterpret_cast<u8*>(buffer));
	gpu->cmd_list->BeginEvent(D3D12_EVENT_METADATA, buffer, size_bytes);
}

sfz_extern_c void gpuQueueEventEnd(GpuLib* gpu)
{
	gpu->cmd_list->EndEvent();
}

sfz_extern_c u64 gpuTimestampGetFreq(const GpuLib* gpu)
{
	u64 ticks_per_sec = 0;
	if (!CHECK_D3D12(gpu->cmd_queue->GetTimestampFrequency(&ticks_per_sec))) {
		GPU_LOG_ERROR("[gpu_lib]: Couldn't get timestamp frequency.");
		return 0;
	}
	return ticks_per_sec;
}

sfz_extern_c void gpuQueueTakeTimestamp(GpuLib* gpu, GpuPtr dst)
{
	// Ensure heap is in COPY_DEST state
	if (gpu->gpu_heap_state != D3D12_RESOURCE_STATE_COPY_DEST) {
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = gpu->gpu_heap.Get();
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = gpu->gpu_heap_state;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
		gpu->cmd_list->ResourceBarrier(1, &barrier);
		gpu->gpu_heap_state = D3D12_RESOURCE_STATE_COPY_DEST;
	}

	// Get timestamp and store it in u64 pointed to by gpu pointer
	const u32 timestamp_idx = 0; // We only need one slot because we immediately copy out the data
	gpu->cmd_list->EndQuery(
		gpu->timestamp_query_heap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, timestamp_idx);
	gpu->cmd_list->ResolveQueryData(
		gpu->timestamp_query_heap.Get(),
		D3D12_QUERY_TYPE_TIMESTAMP,
		timestamp_idx,
		1,
		gpu->gpu_heap.Get(),
		dst);
}

sfz_extern_c GpuTicket gpuQueueTakeTimestampDownload(GpuLib* gpu)
{
	const u32 num_bytes = sizeof(u64);

	// Try to allocate a range
	const GpuHeapRangeAlloc range_alloc = gpuAllocDownloadHeapRange(gpu, num_bytes);
	if (!range_alloc.success) {
		GPU_LOG_ERROR("[gpu_lib]: Download heap overflow by %u bytes.",
			u32(range_alloc.end - gpu->download_heap_safe_offset));
		return GPU_NULL_TICKET;
	}

	// Commit change
	gpu->download_heap_offset = range_alloc.end;

	// Get timestamp and store it in u64 in download heap
	const u32 timestamp_idx = 0; // We only need one slot because we immediately copy out the data
	gpu->cmd_list->EndQuery(
		gpu->timestamp_query_heap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, timestamp_idx);
	gpu->cmd_list->ResolveQueryData(
		gpu->timestamp_query_heap.Get(),
		D3D12_QUERY_TYPE_TIMESTAMP,
		timestamp_idx,
		1,
		gpu->download_heap.Get(),
		range_alloc.begin_mapped);

	// Allocate a pending download slot
	const SfzHandle download_handle = gpu->downloads.allocate();
	if (download_handle == SFZ_NULL_HANDLE) {
		GPU_LOG_ERROR("[gpu_lib]: Out of room for more concurrent downloads (max %u).",
			gpu->cfg.max_num_concurrent_downloads);
		return GPU_NULL_TICKET;
	}

	// Store data for the pending download
	GpuPendingDownload& pending = *gpu->downloads.get(download_handle);
	pending.heap_offset = u32(range_alloc.begin_mapped);
	pending.num_bytes = num_bytes;
	pending.submit_idx = gpu->curr_submit_idx;

	const GpuTicket ticket = { download_handle.bits };
	return ticket;
}

sfz_extern_c void gpuQueueMemcpyUpload(GpuLib* gpu, GpuPtr dst, const void* src, u32 num_bytes)
{
	if (num_bytes == 0) return;
	if (dst < GPU_HEAP_SYSTEM_RESERVED_SIZE || gpu->cfg.gpu_heap_size_bytes <= dst) {
		GPU_LOG_ERROR("[gpu_lib]: Trying to memcpy upload to an invalid pointer (%u).", dst);
		return;
	}

	// Try to allocate a range
	const GpuHeapRangeAlloc range_alloc = gpuAllocUploadHeapRange(gpu, num_bytes);
	if (!range_alloc.success) {
		GPU_LOG_ERROR("[gpu_lib]: Upload heap overflow by %u bytes.",
			u32(range_alloc.end - gpu->upload_heap_safe_offset));
		return;
	}

	// Memcpy data to upload heap and commit change
	memcpy(gpu->upload_heap_mapped_ptr + range_alloc.begin_mapped, src, num_bytes);
	gpu->upload_heap_offset = range_alloc.end;

	// Ensure heap is in COPY_DEST state
	if (gpu->gpu_heap_state != D3D12_RESOURCE_STATE_COPY_DEST) {
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = gpu->gpu_heap.Get();
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = gpu->gpu_heap_state;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
		gpu->cmd_list->ResourceBarrier(1, &barrier);
		gpu->gpu_heap_state = D3D12_RESOURCE_STATE_COPY_DEST;
	}

	// Copy to heap
	gpu->cmd_list->CopyBufferRegion(
		gpu->gpu_heap.Get(), dst, gpu->upload_heap.Get(), range_alloc.begin_mapped, num_bytes);
}

sfz_extern_c void gpuQueueMemcpyUploadConstBuffer(
	GpuLib* gpu, GpuConstBuffer cbuf, const void* src, u32 num_bytes)
{
	if (num_bytes == 0) return;
	GpuConstBufferInfo* cbuf_info = gpu->const_buffers.get(SfzHandle{ cbuf.handle });
	if (cbuf_info == nullptr) {
		GPU_LOG_ERROR("[gpu_lib]: Trying to memcpy upload to an constant buffer.");
		return;
	}
	if (cbuf_info->size_bytes != num_bytes) {
		GPU_LOG_ERROR(
			"[gpu_lib]: Trying to memcpy upload wrong size (%u bytes) to constant buffer (of size %u bytes).",
			num_bytes, cbuf_info->size_bytes);
		return;
	}
	if (gpu->curr_submit_idx <= cbuf_info->last_upload_submit_idx) {
		GPU_LOG_ERROR(
			"[gpu_lib]: Trying to upload to constant buffer twice the same submit, not allowed.");
		return;
	}

	// Try to allocate a range
	const GpuHeapRangeAlloc range_alloc = gpuAllocUploadHeapRange(gpu, num_bytes);
	if (!range_alloc.success) {
		GPU_LOG_ERROR("[gpu_lib]: Upload heap overflow by %u bytes.",
			u32(range_alloc.end - gpu->upload_heap_safe_offset));
		return;
	}

	// Memcpy data to upload heap and commit change
	memcpy(gpu->upload_heap_mapped_ptr + range_alloc.begin_mapped, src, num_bytes);
	gpu->upload_heap_offset = range_alloc.end;

	// Ensure constant buffer is in COPY_DEST state
	if (cbuf_info->state != D3D12_RESOURCE_STATE_COPY_DEST) {
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = cbuf_info->buffer.Get();
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = cbuf_info->state;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
		gpu->cmd_list->ResourceBarrier(1, &barrier);
		cbuf_info->state = D3D12_RESOURCE_STATE_COPY_DEST;
	}

	// Copy to constant buffer
	gpu->cmd_list->CopyBufferRegion(
		cbuf_info->buffer.Get(), 0, gpu->upload_heap.Get(), range_alloc.begin_mapped, num_bytes);

	// Ensure constant buffer is in VERTEX_AND_CONSTANT_BUFFER state
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = cbuf_info->buffer.Get();
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	gpu->cmd_list->ResourceBarrier(1, &barrier);
	cbuf_info->state = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

	// Mark that we have uploaded to the constant buffer this submit index
	cbuf_info->last_upload_submit_idx = gpu->curr_submit_idx;
}

sfz_extern_c void gpuQueueMemcpyUploadTexMip(
	GpuLib* gpu, GpuTexIdx tex_idx, i32 mip_idx, const void* src, i32x2 mip_dims, GpuFormat format)
{
	GpuTexInfo* tex_info = gpu->textures.get(gpu->textures.getHandle(tex_idx));
	if (tex_info == nullptr) {
		GPU_LOG_ERROR("[gpu_lib]: Trying to memcpy upload to an invalid texture (%u).", u32(tex_idx));
		return;
	}
	if (tex_info->desc.num_mips <= mip_idx) {
		GPU_LOG_ERROR("[gpu_lib]: Trying to memcpy upload to an invalid mip (%u) of texture \"%s\".",
			mip_idx, tex_info->name.str);
		return;
	}
	if (tex_info->desc.format != format) {
		GPU_LOG_ERROR("[gpu_lib]: Format mismatch, target texture (\"%s\") has format %s, src data %s.",
			tex_info->name.str, gpuFormatToString(tex_info->desc.format), gpuFormatToString(format));
		return;
	}

	// Get placement info for mip level
	const D3D12_RESOURCE_DESC tex_desc = tex_info->tex->GetDesc();
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
	u32 num_rows = 0;
	u64 row_size_bytes = 0;
	u64 total_bytes = 0;
	gpu->device->GetCopyableFootprints(
		&tex_desc, mip_idx, 1, 0, &footprint, &num_rows, &row_size_bytes, &total_bytes);
	sfz_assert_hard(num_rows == u32(mip_dims.y));
	sfz_assert_hard((num_rows * row_size_bytes) <= total_bytes);
	total_bytes = u64_max(total_bytes, (num_rows * footprint.Footprint.RowPitch));
	sfz_assert_hard((num_rows * footprint.Footprint.RowPitch) == total_bytes);
	const u32 num_bytes = sfzRoundUpAlignedU32(u32(total_bytes), GPU_HEAP_ALIGN);

	// Try to allocate a range
	const GpuHeapRangeAlloc range_alloc = gpuAllocUploadHeapRange(gpu, num_bytes);
	if (!range_alloc.success) {
		GPU_LOG_ERROR("[gpu_lib]: Upload heap overflow by %u bytes.",
			u32(range_alloc.end - gpu->upload_heap_safe_offset));
		return;
	}

	// Copy data to upload heap and commit change
	u8* dst_img = gpu->upload_heap_mapped_ptr + range_alloc.begin_mapped;
	const u32 dst_pitch = footprint.Footprint.RowPitch;
	const u32 src_pitch = mip_dims.x * formatToPixelSize(format);
	sfz_assert(src_pitch <= row_size_bytes);
	for (i32 y = 0; y < mip_dims.y; y++) {
		u8* dst_row = dst_img + dst_pitch * u32(y);
		const u8* src_row = static_cast<const u8*>(src) + src_pitch * u32(y);
		memcpy(dst_row, src_row, src_pitch);
	}
	gpu->upload_heap_offset = range_alloc.end;

	// Transition texture to COPY_DEST state
	const D3D12_RESOURCE_STATES tex_prev_state = texStateToD3D12(tex_info->desc.tex_state);
	{
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = tex_info->tex.Get();
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = tex_prev_state;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
		gpu->cmd_list->ResourceBarrier(1, &barrier);
	}

	// Issue copy command
	{
		D3D12_TEXTURE_COPY_LOCATION src_copy_loc = {};
		src_copy_loc.pResource = gpu->upload_heap.Get();
		src_copy_loc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		src_copy_loc.PlacedFootprint = footprint;
		src_copy_loc.PlacedFootprint.Offset = range_alloc.begin_mapped;

		D3D12_TEXTURE_COPY_LOCATION dst_copy_loc = {};
		dst_copy_loc.pResource = tex_info->tex.Get();
		dst_copy_loc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dst_copy_loc.SubresourceIndex = mip_idx;

		gpu->cmd_list->CopyTextureRegion(&dst_copy_loc, 0, 0, 0, &src_copy_loc, nullptr);
	}

	// Transition back to previous state
	{
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = tex_info->tex.Get();
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		barrier.Transition.StateAfter = tex_prev_state;
		gpu->cmd_list->ResourceBarrier(1, &barrier);
	}
}

sfz_extern_c GpuTicket gpuQueueMemcpyDownload(GpuLib* gpu, GpuPtr src, u32 num_bytes)
{
	if (num_bytes == 0) return GPU_NULL_TICKET;
	if (src < GPU_HEAP_SYSTEM_RESERVED_SIZE || gpu->cfg.gpu_heap_size_bytes <= src) {
		GPU_LOG_ERROR("[gpu_lib]: Trying to memcpy download from an invalid pointer (%u).", src);
		return GPU_NULL_TICKET;
	}

	// Try to allocate a range
	const GpuHeapRangeAlloc range_alloc = gpuAllocDownloadHeapRange(gpu, num_bytes);
	if (!range_alloc.success) {
		GPU_LOG_ERROR("[gpu_lib]: Download heap overflow by %u bytes.",
			u32(range_alloc.end - gpu->download_heap_safe_offset));
		return GPU_NULL_TICKET;
	}

	// Commit change
	gpu->download_heap_offset = range_alloc.end;

	// Ensure heap is in COPY_SOURCE state
	if (gpu->gpu_heap_state != D3D12_RESOURCE_STATE_COPY_SOURCE) {
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = gpu->gpu_heap.Get();
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = gpu->gpu_heap_state;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
		gpu->cmd_list->ResourceBarrier(1, &barrier);
		gpu->gpu_heap_state = D3D12_RESOURCE_STATE_COPY_SOURCE;
	}

	// Copy to download heap
	gpu->cmd_list->CopyBufferRegion(
		gpu->download_heap.Get(), range_alloc.begin_mapped, gpu->gpu_heap.Get(), src, num_bytes);

	// Allocate a pending download slot
	const SfzHandle download_handle = gpu->downloads.allocate();
	if (download_handle == SFZ_NULL_HANDLE) {
		GPU_LOG_ERROR("[gpu_lib]: Out of room for more concurrent downloads (max %u).",
			gpu->cfg.max_num_concurrent_downloads);
		return GPU_NULL_TICKET;
	}

	// Store data for the pending download
	GpuPendingDownload& pending = *gpu->downloads.get(download_handle);
	pending.heap_offset = u32(range_alloc.begin_mapped);
	pending.num_bytes = num_bytes;
	pending.submit_idx = gpu->curr_submit_idx;

	const GpuTicket ticket = { download_handle.bits };
	return ticket;
}

sfz_extern_c bool gpuIsTicketValid(GpuLib* gpu, GpuTicket ticket)
{
	const SfzHandle handle = SfzHandle{ ticket.handle };
	GpuPendingDownload* pending = gpu->downloads.get(handle);
	return pending != nullptr;
}

sfz_extern_c void gpuGetDownloadedData(GpuLib* gpu, GpuTicket ticket, void* dst, u32 num_bytes)
{
	const SfzHandle handle = SfzHandle{ ticket.handle };
	GpuPendingDownload* pending = gpu->downloads.get(handle);
	if (pending == nullptr) {
		GPU_LOG_ERROR("[gpu_lib]: Invalid ticket.");
		return;
	}
	if (pending->num_bytes != num_bytes) {
		GPU_LOG_ERROR("[gpu_lib]: Memcpy download size mismatch, requested %u bytes, but %u was downloaded.",
			num_bytes, pending->num_bytes);
		return;
	}
	if (gpu->known_completed_submit_idx < pending->submit_idx) {
		GPU_LOG_ERROR("[gpu_lib]: Memcpy download is not yet done.");
		return;
	}
	memcpy(dst, gpu->download_heap_mapped_ptr + pending->heap_offset, num_bytes);
	gpu->downloads.deallocate(handle);
}

sfz_extern_c void gpuQueueDispatch(
	GpuLib* gpu, GpuKernel kernel, i32x3 num_groups, GpuConstBuffer cbuf, const void* params, u32 params_size)
{
	// Get kernel
	const GpuKernelInfo* kernel_info = gpu->kernels.get(SfzHandle{ kernel.handle });
	if (kernel_info == nullptr) {
		GPU_LOG_ERROR("[gpu_lib]: Invalid kernel handle.");
		return;
	}

	// Ensure heap is in correct state
	const D3D12_RESOURCE_STATES correct_heap_state = kernel_info->desc.write_enabled_heap ?
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS : D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
	if (gpu->gpu_heap_state != correct_heap_state) {
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = gpu->gpu_heap.Get();
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = gpu->gpu_heap_state;
		barrier.Transition.StateAfter = correct_heap_state;
		gpu->cmd_list->ResourceBarrier(1, &barrier);
		gpu->gpu_heap_state = correct_heap_state;
	}

	// Set pipeline state and root signature
	gpu->cmd_list->SetPipelineState(kernel_info->pso.Get());
	gpu->cmd_list->SetComputeRootSignature(kernel_info->root_sig.Get());

	// Set inline descriptors
	if (kernel_info->desc.write_enabled_heap) {
		gpu->cmd_list->SetComputeRootUnorderedAccessView(
			GPU_ROOT_PARAM_GLOBAL_HEAP_IDX, gpu->gpu_heap->GetGPUVirtualAddress());
	}
	else {
		gpu->cmd_list->SetComputeRootShaderResourceView(
			GPU_ROOT_PARAM_GLOBAL_HEAP_IDX, gpu->gpu_heap->GetGPUVirtualAddress());
	}
	gpu->cmd_list->SetComputeRootDescriptorTable(
		GPU_ROOT_PARAM_TEX_HEAP_IDX, gpu->tex_descriptor_heap_start_gpu);

	// Set constant buffer
	if (kernel_info->const_buffer_size != 0) {
		// Note: We have this extra check because we don't want to emit a warning if user has
		//       supplied a constant buffer that then didn't end up used. This is a common occurence
		//       during dev when creating new shaders or hot-reloading.
		GpuConstBufferInfo* cbuf_info = gpu->const_buffers.get(SfzHandle{ cbuf.handle });
		if (cbuf_info == nullptr) {
			GPU_LOG_ERROR("[gpu_lib]: (\"%s\") Invalid constant buffer specified.",
				kernel_info->desc.name);
			return;
		}
		if (kernel_info->const_buffer_size != cbuf_info->size_bytes) {
			GPU_LOG_ERROR("[gpu_lib]: (\"%s\") Invalid size of constant buffer, got %u words (%u bytes), expected %u words (%u bytes).",
				kernel_info->desc.name,
				cbuf_info->size_bytes / sizeof(u32),
				cbuf_info->size_bytes,
				kernel_info->const_buffer_size / sizeof(u32),
				kernel_info->const_buffer_size);
			return;
		}
		gpu->cmd_list->SetComputeRootConstantBufferView(
			GPU_ROOT_PARAM_CONST_BUFFER_IDX, cbuf_info->buffer->GetGPUVirtualAddress());
	}

	// Set launch params
	if (kernel_info->launch_params_size != 0) {
		// Note: We have this extra check because we don't want to emit a warning if user has
		//       supplied params that then didn't end up used. This is a common occurence during
		//       dev when creating new shaders or hot-reloading.
		if (kernel_info->launch_params_size != params_size) {
			GPU_LOG_ERROR("[gpu_lib]: (\"%s\") Invalid size of launch parameters, got %u words (%u bytes), expected %u words (%u bytes).",
				kernel_info->desc.name,
				params_size / sizeof(u32),
				params_size,
				kernel_info->launch_params_size / sizeof(u32),
				kernel_info->launch_params_size);
			return;
		}
		if (params_size != 0) {
			gpu->cmd_list->SetComputeRoot32BitConstants(
				GPU_ROOT_PARAM_LAUNCH_PARAMS_IDX, params_size / 4, params, 0);
		}
	}

	// Dispatch
	sfz_assert(0 < num_groups.x && 0 < num_groups.y && 0 < num_groups.z);
	gpu->cmd_list->Dispatch(u32(num_groups.x), u32(num_groups.y), u32(num_groups.z));
}

sfz_extern_c void gpuQueueBarriers(GpuLib* gpu, const GpuBarrier* barriers, u32 num_barriers)
{
	// Validate and convert barriers to D3D12
	gpu->tmp_barriers.clear();
	for (u32 i = 0; i < num_barriers; i++) {
		const GpuBarrier src = barriers[i];
		if (
			src.res_idx == GPU_NULL_TEX ||
			(src.res_idx < (U16_MAX - 1) && gpu->cfg.max_num_textures < src.res_idx)) {
			GPU_LOG_ERROR("[gpu_lib]: Trying to insert a GpuBarrier for an invalid res_idx (%u).",
				u32(src.res_idx));
			continue;
		}
		if (src.res_idx == U16_MAX && !src.uav_barrier) {
			GPU_LOG_ERROR("[gpu_lib]: Trying to insert a non-UAV GpuBarrier for the GPU heap, not allowed.");
			continue;
		}
		if (src.res_idx == GPU_SWAPCHAIN_TEX_IDX && !src.uav_barrier) {
			GPU_LOG_ERROR("[gpu_lib]: Trying to insert a non-UAV GpuBarrier for the swapchain, not allowed.");
			continue;
		}
		GpuTexInfo* tex_info = nullptr;
		if (src.res_idx < (U16_MAX - 1)) {
			tex_info = gpu->textures.get(gpu->textures.getHandle(src.res_idx));
			if (tex_info == nullptr) {
				GPU_LOG_ERROR("[gpu_lib]: Trying to insert a GpuBarrier for an invalid texture (%u).",
					u32(src.res_idx));
				continue;
			}
			if (!src.uav_barrier && src.target_state != GPU_TEX_READ_ONLY && src.target_state != GPU_TEX_READ_WRITE) {
				GPU_LOG_ERROR("[gpu_lib]: Trying to insert a transition GpuBarrier with invalid target state (\"%s\").",
					gpuTexStateToString(src.target_state));
				continue;
			}
			if (!src.uav_barrier && tex_info->desc.tex_state == src.target_state) {
				// Can omit this barrier, texture is already in correct state.
				continue;
			}
		}

		D3D12_RESOURCE_BARRIER& dst = gpu->tmp_barriers.add();
		if (src.uav_barrier) {
			ID3D12Resource* res = nullptr;
			if (src.res_idx == U16_MAX) res = gpu->gpu_heap.Get();
			else if (src.res_idx == (U16_MAX - 1)) res = nullptr; //
			else if (src.res_idx == GPU_SWAPCHAIN_TEX_IDX) res = gpu->swapchain_tex.Get();
			else res = tex_info->tex.Get();
			dst.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
			dst.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			dst.UAV.pResource = res;
			sfz_assert(src.res_idx == (U16_MAX - 1) || dst.UAV.pResource != nullptr);
		}
		else {
			dst.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			dst.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			dst.Transition.pResource = tex_info->tex.Get();
			dst.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			dst.Transition.StateBefore = texStateToD3D12(tex_info->desc.tex_state);
			dst.Transition.StateAfter = texStateToD3D12(src.target_state);
			sfz_assert(dst.Transition.pResource != nullptr);
			tex_info->desc.tex_state = src.target_state;
		}
	}

	// Set barriers
	gpu->cmd_list->ResourceBarrier(gpu->tmp_barriers.size(), gpu->tmp_barriers.data());
}

sfz_extern_c void gpuQueueCopyToSwapchain(GpuLib* gpu)
{
	if (gpu->swapchain == nullptr || gpu->swapchain_tex == nullptr) return;
	const GpuSwapchainBackbuffer& bbuf = gpu->getCurrSwapchainBackbuffer();

	// Transition swapchain tex to PIXEL_SHADER_RESOURCE
	{
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = gpu->swapchain_tex.Get();
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		gpu->cmd_list->ResourceBarrier(1, &barrier);
	}

	// Set render targets
	gpu->cmd_list->OMSetRenderTargets(1, &bbuf.rtv_descriptor, FALSE, nullptr);

	// Set viewport
	D3D12_VIEWPORT viewport = {};
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = f32(gpu->swapchain_res.x);
	viewport.Height = f32(gpu->swapchain_res.y);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	gpu->cmd_list->RSSetViewports(1, &viewport);

	// Set default scissor
	D3D12_RECT scissor_rect = {};
	scissor_rect.left = 0;
	scissor_rect.top = 0;
	scissor_rect.right = LONG_MAX;
	scissor_rect.bottom = LONG_MAX;
	gpu->cmd_list->RSSetScissorRects(1, &scissor_rect);

	// Set shader
	gpu->cmd_list->SetPipelineState(gpu->swapchain_copy_pso.Get());
	gpu->cmd_list->SetGraphicsRootSignature(gpu->swapchain_copy_root_sig.Get());
	gpu->cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set common root signature stuff
	gpu->cmd_list->SetGraphicsRootShaderResourceView(
		GPU_ROOT_PARAM_GLOBAL_HEAP_IDX, gpu->gpu_heap->GetGPUVirtualAddress());
	gpu->cmd_list->SetGraphicsRootDescriptorTable(
		GPU_ROOT_PARAM_TEX_HEAP_IDX, gpu->tex_descriptor_heap_start_gpu);

	// Launch params
	struct LaunchParamsSwapchainCopy {
		i32x2 swapchain_res;
		u32 padding0;
		u32 padding1;
	};
	const LaunchParamsSwapchainCopy params = LaunchParamsSwapchainCopy{
		.swapchain_res = gpu->swapchain_res
	};
	gpu->cmd_list->SetGraphicsRoot32BitConstants(GPU_ROOT_PARAM_LAUNCH_PARAMS_IDX, 4, &params, 0);

	// Draw triangle
	gpu->cmd_list->DrawInstanced(3, 1, 0, 0);

	// Transition swapchain tex back to UNORDERED_ACCESS
	{
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = gpu->swapchain_tex.Get();
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		gpu->cmd_list->ResourceBarrier(1, &barrier);
	}
}

sfz_extern_c void gpuQueueSwapchainFinish(GpuLib* gpu)
{
	if (gpu->swapchain == nullptr) return;
	GpuSwapchainBackbuffer& bbuf = gpu->getCurrSwapchainBackbuffer();

	// Insert barrier to transition swapchain from RENDER_TARGET to PRESENT
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = bbuf.back_buffer_rt.Get();
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	gpu->cmd_list->ResourceBarrier(1, &barrier);
}

sfz_extern_c void gpuSubmitQueuedWork(GpuLib* gpu)
{
	// Execute current command list
	{
		GpuCmdListBacking& cmd_list_backing = gpu->getCurrCmdListBacking();

		// Store current upload and download heap offsets
		cmd_list_backing.upload_heap_offset = gpu->upload_heap_offset;
		cmd_list_backing.download_heap_offset = gpu->download_heap_offset;

		// Close command list
		if (!CHECK_D3D12(gpu->cmd_list->Close())) {
			GPU_LOG_ERROR("[gpu_lib]: Could not close command list.");
			return;
		}

		// Execute command list
		ID3D12CommandList* cmd_lists[1] = {};
		cmd_lists[0] = gpu->cmd_list.Get();
		gpu->cmd_queue->ExecuteCommandLists(1, cmd_lists);

		// Fence signalling
		if (!CHECK_D3D12(gpu->cmd_queue->Signal(gpu->cmd_queue_fence.Get(), gpu->cmd_queue_fence_value))) {
			GPU_LOG_ERROR("[gpu_lib]: Could not signal from command queue.");
			return;
		}
		// This command list is done once the value above is signalled
		cmd_list_backing.fence_value = gpu->cmd_queue_fence_value;
		gpu->cmd_queue_fence_value += 1; // Increment value we will signal next time
	}

	// Log current deubg messages
	logDebugMessages(gpu, gpu->info_queue.Get());

	// Advance to next submit idx
	{
		gpu->curr_submit_idx += 1;
	}

	// Start next command list
	{
		GpuCmdListBacking& cmd_list_backing = gpu->getCurrCmdListBacking();

		// Wait until command list is done
		if (gpu->cmd_queue_fence->GetCompletedValue() < cmd_list_backing.fence_value) {
			CHECK_D3D12(gpu->cmd_queue_fence->SetEventOnCompletion(
				cmd_list_backing.fence_value, gpu->cmd_queue_fence_event));
			WaitForSingleObject(gpu->cmd_queue_fence_event, INFINITE);
		}

		// Now we know that the command list we just got has finished executing, thus we can set
		// our known completed submit idx to the idx of the submit it was from.
		gpu->known_completed_submit_idx =
			u64_max(gpu->known_completed_submit_idx, cmd_list_backing.submit_idx);

		// Same applies to upload and download heap safe offsets. The safe offset is always + size
		// of the heap in question to handle wrap around in logic.
		gpu->upload_heap_safe_offset = u64_max(gpu->upload_heap_safe_offset,
			cmd_list_backing.upload_heap_offset + gpu->cfg.upload_heap_size_bytes);
		gpu->download_heap_safe_offset = u64_max(gpu->download_heap_safe_offset,
			cmd_list_backing.download_heap_offset + gpu->cfg.download_heap_size_bytes);

		// Mark the new command list with the index of the current submit
		cmd_list_backing.submit_idx = gpu->curr_submit_idx;

		if (!CHECK_D3D12(cmd_list_backing.cmd_allocator->Reset())) {
			GPU_LOG_ERROR("[gpu_lib]: Couldn't reset command allocator.");
			return;
		}
		if (!CHECK_D3D12(gpu->cmd_list->Reset(cmd_list_backing.cmd_allocator.Get(), nullptr))) {
			GPU_LOG_ERROR("[gpu_lib]: Couldn't reset command list.");
			return;
		}

		// Set texture descriptor heap
		ID3D12DescriptorHeap* heaps[] = { gpu->tex_descriptor_heap.Get() };
		gpu->cmd_list->SetDescriptorHeaps(1, heaps);
	}

	// Check if there are any old pending downloads that should be killed
	{
		const SfzPoolSlot* slots = gpu->downloads.slots();
		const GpuPendingDownload* downloads = gpu->downloads.data();
		const u32 size = gpu->downloads.arraySize();
		for (u32 i = 0; i < size; i++) {
			if (!slots[i].active()) continue;
			const GpuPendingDownload& download = downloads[i];
			if ((download.submit_idx + GPU_DOWNLOAD_MAX_AGE) < gpu->curr_submit_idx) {
				GPU_LOG_INFO("[gpu_lib]: Found old pending download (%u), currently (%u), removing.",
					u32(download.submit_idx), u32(gpu->curr_submit_idx));
				gpu->downloads.deallocate(i);
			}
		}
	}
}

sfz_extern_c void gpuSwapchainPresent(GpuLib* gpu, bool vsync, i32 sync_interval)
{
	if (gpu->swapchain == nullptr) return;
	sync_interval = i32_clamp(sync_interval, 1, 4);

	// Present swapchain's render target
	{
		// Present
		const u32 pre_present_swapchain_fb_idx = gpu->swapchain->GetCurrentBackBufferIndex();
		sfz_assert(pre_present_swapchain_fb_idx < GPU_SWAPCHAIN_NUM_BACKBUFFERS);
		const u32 vsync_val = vsync ? u32(sync_interval) : 0; // Can specify 2-4 for vsync:ing on not every frame
		const u32 flags = (!vsync && gpu->allow_tearing) ? DXGI_PRESENT_ALLOW_TEARING : 0;
		if (!CHECK_D3D12(gpu->swapchain->Present(vsync_val, flags))) {
			GPU_LOG_ERROR("[gpu_lib]: Present failure.");
			return;
		}
		const u32 post_present_swapchain_fb_idx = gpu->swapchain->GetCurrentBackBufferIndex();
		sfz_assert(post_present_swapchain_fb_idx < GPU_SWAPCHAIN_NUM_BACKBUFFERS);
		sfz_assert(pre_present_swapchain_fb_idx != post_present_swapchain_fb_idx);

		// Not sure if we actually need the sync below given that we are syncing on submitting
		// command lists. But sure, why not.

		// Signal that we have finished presenting
		{
			if (!CHECK_D3D12(gpu->cmd_queue->Signal(gpu->cmd_queue_fence.Get(), gpu->cmd_queue_fence_value))) {
				GPU_LOG_ERROR("[gpu_lib]: Could not signal from command queue.");
				return;
			}
			GpuSwapchainBackbuffer& pre_bbuf = gpu->swapchain_backbuffers[pre_present_swapchain_fb_idx];
			pre_bbuf.fence_value = gpu->cmd_queue_fence_value;
			gpu->cmd_queue_fence_value += 1; // Increment value we will signal next time
		}

		// Wait for new back buffer to be available (have finished presenting) so it's safe to use
		{
			const GpuSwapchainBackbuffer& post_bbuf =
				gpu->swapchain_backbuffers[post_present_swapchain_fb_idx];
			if (gpu->cmd_queue_fence->GetCompletedValue() < post_bbuf.fence_value) {
				CHECK_D3D12(gpu->cmd_queue_fence->SetEventOnCompletion(
					post_bbuf.fence_value, gpu->cmd_queue_fence_event));
				WaitForSingleObject(gpu->cmd_queue_fence_event, INFINITE);
			}
		}
	}

	// Get current window resolution
	i32x2 window_res = i32x2_splat(0);
	{
		const HWND hwnd = static_cast<const HWND>(gpu->cfg.native_window_handle);
		RECT rect = {};
		BOOL success = GetClientRect(hwnd, &rect);
		sfz_assert(success);
		window_res = i32x2_init(rect.right, rect.bottom);
	}
	window_res = i32x2_max(window_res, i32x2_splat(1));
	gpu->swapchain_res = window_res;

	// Grab old swapchain resolution
	DXGI_SWAP_CHAIN_DESC swapchain_desc = {};
	CHECK_D3D12(gpu->swapchain->GetDesc(&swapchain_desc));
	sfz_assert(swapchain_desc.BufferCount == GPU_SWAPCHAIN_NUM_BACKBUFFERS);
	const i32x2 old_swapchain_res =
		i32x2_init(swapchain_desc.BufferDesc.Width, swapchain_desc.BufferDesc.Height);

	// Resize swapchain if window resolution has changed
	if (old_swapchain_res != window_res) {
		GPU_LOG_INFO("[gpu_lib]: Resizing swapchain framebuffers from %ix%i to %ix%i.",
			old_swapchain_res.x, old_swapchain_res.y, window_res.x, window_res.y);

		// Flush current work in-progress
		gpuFlushSubmittedWork(gpu);

		// Release old swapchain RT
		gpu->swapchain_tex.Reset();
		for (u32 i = 0; i < GPU_SWAPCHAIN_NUM_BACKBUFFERS; i++) {
			GpuSwapchainBackbuffer& bbuf = gpu->swapchain_backbuffers[i];
			bbuf.back_buffer_rt.Reset();
		}

		// Resize swapchain
		if (!CHECK_D3D12(gpu->swapchain->ResizeBuffers(
			GPU_SWAPCHAIN_NUM_BACKBUFFERS,
			u32(window_res.x),
			u32(window_res.y),
			swapchain_desc.BufferDesc.Format,
			swapchain_desc.Flags))) {
			GPU_LOG_ERROR("[gpu_lib]: Failed to resize swapchain framebuffers.");
			return;
		}

		// Reinitialize swapchain backbuffer data
		for (u32 i = 0; i < GPU_SWAPCHAIN_NUM_BACKBUFFERS; i++) {
			GpuSwapchainBackbuffer& bbuf = gpu->swapchain_backbuffers[i];
			CHECK_D3D12(gpu->swapchain->GetBuffer(i, IID_PPV_ARGS(&bbuf.back_buffer_rt)));
			gpu->device->CreateRenderTargetView(bbuf.back_buffer_rt.Get(), nullptr, bbuf.rtv_descriptor);
		}

		// Allocate swapchain tex
		{
			D3D12_HEAP_PROPERTIES heap_props = {};
			heap_props.Type = D3D12_HEAP_TYPE_DEFAULT;
			heap_props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			heap_props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			heap_props.CreationNodeMask = 0;
			heap_props.VisibleNodeMask = 0;

			D3D12_RESOURCE_DESC desc = {};
			desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			desc.Alignment = 0;
			desc.Width = u32(window_res.x);
			desc.Height = u32(window_res.y);
			desc.DepthOrArraySize = 1;
			desc.MipLevels = 1;
			desc.Format = swapchain_desc.BufferDesc.Format;
			desc.SampleDesc = { 1, 0 };
			desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			desc.Flags =
				D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET |
				D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

			const bool rt_success = CHECK_D3D12(gpu->device->CreateCommittedResource(
				&heap_props,
				D3D12_HEAP_FLAG_NONE,
				&desc,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				nullptr,
				IID_PPV_ARGS(&gpu->swapchain_tex)));
			if (!rt_success) {
				GPU_LOG_ERROR("[gpu_lib]: Could not allocate swapchain render target of size %ix%i.",
					window_res.x, window_res.y);
				return;
			}
			setDebugName(gpu->swapchain_tex.Get(), "swapchain_tex");
		}

		// Set swapchain descriptors in tex descriptor heap
		texSetDescriptors(gpu, GPU_SWAPCHAIN_TEX_IDX, 1, gpu->swapchain_tex.Get(), swapchain_desc.BufferDesc.Format);

		// Rebuild all swapchain relative textures
		GpuTexInfo* tex_infos = gpu->textures.data();
		const SfzPoolSlot* tex_slots = gpu->textures.slots();
		const u32 tex_array_size = gpu->textures.arraySize();
		for (u32 idx = GPU_SWAPCHAIN_TEX_IDX + 1; idx < tex_array_size; idx++) {
			const SfzPoolSlot slot = tex_slots[idx];
			if (!slot.active()) continue;
			GpuTexInfo& tex_info = tex_infos[idx];
			if (!tex_info.desc.swapchain_relative) continue;
			const SfzHandle tex_handle = gpu->textures.getHandle(idx);
			sfz_assert(tex_handle != SFZ_NULL_HANDLE);

			// Rebuild texture
			// Need to copy desc to avoid potential aliasing issues
			SfzStr96 name = tex_info.name;
			GpuTexDesc desc = tex_info.desc;
			desc.name = name.str;
			gpuTexInitInternal(gpu, desc, &tex_handle);
		}
	}

	// Transition new back buffer to RENDER_TARGET and clear it
	{
		const GpuSwapchainBackbuffer& bbuf = gpu->getCurrSwapchainBackbuffer();

		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = bbuf.back_buffer_rt.Get();
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		gpu->cmd_list->ResourceBarrier(1, &barrier);

		constexpr f32x4 CLEAR_COLOR = f32x4_splat(0.0f);
		gpu->cmd_list->ClearRenderTargetView(bbuf.rtv_descriptor, &CLEAR_COLOR.x, 0, nullptr);
	}
}

sfz_extern_c void gpuFlushSubmittedWork(GpuLib* gpu)
{
	CHECK_D3D12(gpu->cmd_queue->Signal(gpu->cmd_queue_fence.Get(), gpu->cmd_queue_fence_value));
	if (gpu->cmd_queue_fence->GetCompletedValue() < gpu->cmd_queue_fence_value) {
		CHECK_D3D12(gpu->cmd_queue_fence->SetEventOnCompletion(
			gpu->cmd_queue_fence_value, gpu->cmd_queue_fence_event));
		WaitForSingleObject(gpu->cmd_queue_fence_event, INFINITE);
	}
	gpu->cmd_queue_fence_value += 1;

	// Since we have flushed all submitted work, it stands to reason that it must have completed.
	// Update known completed submit idx accordingly
	gpu->known_completed_submit_idx = gpu->curr_submit_idx > 0 ? gpu->curr_submit_idx - 1 : 0;

	// Same applies to upload and download heap safe offset. The safe offset is always + size of
	// the heap in question to handle wrap around in logic.
	gpu->upload_heap_safe_offset = u64_max(gpu->upload_heap_safe_offset,
		gpu->getPrevCmdListBacking().upload_heap_offset + gpu->cfg.upload_heap_size_bytes);
	gpu->download_heap_safe_offset = u64_max(gpu->download_heap_safe_offset,
		gpu->getPrevCmdListBacking().download_heap_offset + gpu->cfg.download_heap_size_bytes);
}
