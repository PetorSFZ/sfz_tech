// Copyright (c) Peter Hillerström (skipifzero.com, peter@hstroem.se)
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

#define ZG_DLL_EXPORT
#include "ZeroG.h"

#include <vulkan/vulkan.h>

#include <cmath>
#include <cstring>

#include <skipifzero.hpp>
#include <skipifzero_arrays.hpp>

#include "common/Context.hpp"
#include "common/ErrorReporting.hpp"
#include "common/Logging.hpp"
#include "common/Matrices.hpp"
#include "common/Mutex.hpp"
#include "common/Strings.hpp"

#include "vulkan/VulkanCommandQueue.hpp"
#include "vulkan/VulkanCommon.hpp"
#include "vulkan/VulkanDebug.hpp"

// Vulkan Backend State
// ------------------------------------------------------------------------------------------------

struct VulkanContext final {
	VkInstance instance = nullptr; // Externally synchronized
	VkSurfaceKHR surface = nullptr;
	VkPhysicalDevice physicalDevice = nullptr;
	VkPhysicalDeviceProperties physicalDeviceProperties = {};
	VkDevice device = nullptr;
};

struct VulkanSwapchain final {
	uint32_t width = 0;
	uint32_t height = 0;
};

struct VulkanBackendState final {

	// Collection of some externally synchronized vulkan state that could roughly be considered
	// a "context" when grouped together
	Mutex<VulkanContext> context;

	Mutex<VulkanSwapchain> swapchain;

	ZgCommandQueue presentQueue;
	ZgCommandQueue copyQueue;
};

// Vulkan Backend implementation
// ------------------------------------------------------------------------------------------------

struct ZgBackend final {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	ZgBackend() = default;
	ZgBackend(const ZgBackend&) = delete;
	ZgBackend& operator= (const ZgBackend&) = delete;
	ZgBackend(ZgBackend&&) = delete;
	ZgBackend& operator= (ZgBackend&&) = delete;

	~ZgBackend() noexcept
	{
		// Access context
		MutexAccessor<VulkanContext> context = mState->context.access();

		// Destroy VkInstance
		if (context.data().instance != nullptr) {
			// TODO: Allocation callbacks
			if (mDebugMode) {
				auto vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)
					vkGetInstanceProcAddr(context.data().instance, "vkDestroyDebugReportCallbackEXT");
				vkDestroyDebugReportCallbackEXT(context.data().instance, zg::vulkanDebugCallback, nullptr);
			}
			vkDestroyInstance(context.data().instance, nullptr);
		}

		// Delete remaining state
		getAllocator()->deleteObject(mState);
	}

	// State methods
	// --------------------------------------------------------------------------------------------

	ZgResult init(const ZgContextInitSettings& settings) noexcept
	{
		// Initialize members and create state struct
		mDebugMode = settings.vulkan.debugMode;
		mState = getAllocator()->newObject<VulkanBackendState>(sfz_dbg("VulkanBackendState"));

		// Log available instance layers and extensions
		zg::vulkanLogAvailableInstanceLayers();
		zg::vulkanLogAvailableInstanceExtensions();

		// Application info struct
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.apiVersion = VK_API_VERSION_1_0;

		// Layers and extensions arrays
		sfz::Array<const char*> layers;
		layers.init(64, getAllocator(), sfz_dbg("VulkanInit: layers"));
		sfz::Array<const char*> extensions;
		extensions.init(64, getAllocator(), sfz_dbg("VulkanInit: extensions"));

		// Debug mode layers and extensions
		if (mDebugMode) {
			layers.add("VK_LAYER_LUNARG_standard_validation");
			layers.add("VK_LAYER_LUNARG_core_validation");
			layers.add("VK_LAYER_LUNARG_parameter_validation");
			layers.add("VK_LAYER_LUNARG_object_tracker");
			extensions.add("VK_EXT_debug_report");
		}

		// TODO: Add other required layers and extensions

		// pNext can optionally point to a VkDebugReportCallbackCreateInfoEXT in order to create a
		// debug report callback that is used only during vkCreateInstance() and vkDestroyInstance(),
		// which can't be covered by a normal persistent debug report callback.
		void* instanceInfoPNext = nullptr;
		/*VkDebugReportCallbackCreateInfoEXT createDestroyCallbackInfo = {};
		if (mDebugMode) {
			createDestroyCallbackInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
			createDestroyCallbackInfo.flags = VK_DEBUG_REPORT_INFORMATION_BIT_EXT
				| VK_DEBUG_REPORT_WARNING_BIT_EXT
				| VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT
				| VK_DEBUG_REPORT_ERROR_BIT_EXT
				| VK_DEBUG_REPORT_DEBUG_BIT_EXT;
			createDestroyCallbackInfo.pfnCallback = &vulkanDebugReportCallback;
			instanceInfoPNext = &createDestroyCallbackInfo;
		}*/

		// Instance create info struct
		VkInstanceCreateInfo instanceInfo = {};
		instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceInfo.pNext = instanceInfoPNext;
		instanceInfo.flags = 0;
		instanceInfo.pApplicationInfo = &appInfo;
		instanceInfo.enabledLayerCount = layers.size();
		instanceInfo.ppEnabledLayerNames = layers.size() != 0 ? layers.data() : nullptr;
		instanceInfo.enabledExtensionCount = extensions.size();
		instanceInfo.ppEnabledExtensionNames = extensions.size() != 0 ? extensions.data() : nullptr;

		// Create Vulkan instance
		// TODO: Set allocators (if not on macOS/iOS)
		MutexAccessor<VulkanContext> context = mState->context.access();
		bool createInstanceSuccess = CHECK_VK vkCreateInstance(
			&instanceInfo,
			nullptr,
			&context.data().instance);
		if (!createInstanceSuccess) {
			ZG_ERROR("Failed to create VkInstance");
			return ZG_ERROR_GENERIC;
		}
		ZG_INFO("VkInstance created");

		// Register debug report callback
		if (mDebugMode) {
			// Setup callback creation information
			VkDebugReportCallbackCreateInfoEXT callbackCreateInfo = {};
			callbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
			callbackCreateInfo.flags = VK_DEBUG_REPORT_INFORMATION_BIT_EXT
				| VK_DEBUG_REPORT_WARNING_BIT_EXT
				| VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT
				| VK_DEBUG_REPORT_ERROR_BIT_EXT
				| VK_DEBUG_REPORT_DEBUG_BIT_EXT;
			callbackCreateInfo.pfnCallback = &zg::vulkanDebugReportCallback;

			// Register the callback
			// TODO: Set allocators
			auto vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)
				vkGetInstanceProcAddr(context.data().instance, "vkCreateDebugReportCallbackEXT");
			CHECK_VK vkCreateDebugReportCallbackEXT(
				context.data().instance, &callbackCreateInfo, nullptr, &zg::vulkanDebugCallback);
		}

		// TODO: At this point we should create a VkSurface using platform specific code

		// Log available physical devices
		zg::vulkanLogAvailablePhysicalDevices(context.data().instance, context.data().surface);

		// TODO: Heuristic to choose physical device
		//       Should probably take DISCRETE_GPU with largest amount of device local memory.
		const uint32_t physicalDeviceIdx = 0;
		{
			constexpr uint32_t MAX_NUM_PHYSICAL_DEVICES = 32;

			// Check how many physical devices there is
			uint32_t numPhysicalDevices = 0;
			CHECK_VK vkEnumeratePhysicalDevices(context.data().instance, &numPhysicalDevices, nullptr);
			sfz_assert(numPhysicalDevices <= MAX_NUM_PHYSICAL_DEVICES);
			if (numPhysicalDevices > MAX_NUM_PHYSICAL_DEVICES) numPhysicalDevices = MAX_NUM_PHYSICAL_DEVICES;

			// Retrieve physical devices
			VkPhysicalDevice physicalDevices[MAX_NUM_PHYSICAL_DEVICES] = {};
			CHECK_VK vkEnumeratePhysicalDevices(context.data().instance, &numPhysicalDevices, physicalDevices);

			// Select the choosen physical device
			sfz_assert(physicalDeviceIdx < numPhysicalDevices);
			context.data().physicalDevice = physicalDevices[physicalDeviceIdx];

			// Store physical devices properties for the choosen device
			vkGetPhysicalDeviceProperties(
				context.data().physicalDevice, &context.data().physicalDeviceProperties);
		}
		ZG_INFO("Using physical device: %u -- %s",
			physicalDeviceIdx, context.data().physicalDeviceProperties.deviceName);

		// Log available device extensions
		zg::vulkanLogDeviceExtensions(physicalDeviceIdx,
			context.data().physicalDevice, context.data().physicalDeviceProperties);

		// Log available queue families
		zg::vulkanLogQueueFamilies(context.data().physicalDevice, context.data().surface);

		// TODO: Heuristic to choose queue family for present and copy queues
		//       Should require the correct flags for each queue
		const uint32_t queueFamilyIdx = 0;


		return ZG_SUCCESS;
	}

	// Context methods
	// --------------------------------------------------------------------------------------------

	ZgResult swapchainResize(
		uint32_t width,
		uint32_t height) noexcept
	{
		(void)width;
		(void)height;
		return ZG_WARNING_UNIMPLEMENTED;
	}

	ZgResult setVsync(
		bool vsync) noexcept
	{
		(void)vsync;
		return ZG_WARNING_UNIMPLEMENTED;
	}

	ZgResult swapchainBeginFrame(
		ZgFramebuffer** framebufferOut,
		ZgProfiler* profiler,
		uint64_t* measurementIdOut) noexcept
	{
		(void)framebufferOut;
		(void)profiler;
		(void)measurementIdOut;
		return ZG_WARNING_UNIMPLEMENTED;
	}

	ZgResult swapchainFinishFrame(
		ZgProfiler* profiler,
		uint64_t measurementId) noexcept
	{
		(void)profiler;
		(void)measurementId;
		return ZG_WARNING_UNIMPLEMENTED;
	}

	ZgResult fenceCreate(ZgFence** fenceOut) noexcept
	{
		(void)fenceOut;
		return ZG_WARNING_UNIMPLEMENTED;
	}

	// Stats
	// --------------------------------------------------------------------------------------------

	ZgResult getStats(ZgStats& statsOut) noexcept
	{
		(void)statsOut;
		return ZG_WARNING_UNIMPLEMENTED;
	}

	// Pipeline compute methods
	// --------------------------------------------------------------------------------------------

	ZgResult pipelineComputeCreateFromFileHLSL(
		ZgPipelineCompute** pipelineOut,
		ZgPipelineBindingsSignature* bindingsSignatureOut,
		ZgPipelineComputeSignature* computeSignatureOut,
		const ZgPipelineComputeCreateInfo& createInfo,
		const ZgPipelineCompileSettingsHLSL& compileSettings) noexcept
	{
		(void)pipelineOut;
		(void)bindingsSignatureOut;
		(void)computeSignatureOut;
		(void)createInfo;
		(void)compileSettings;
		return ZG_WARNING_UNIMPLEMENTED;
	}

	ZgResult pipelineComputeRelease(
		ZgPipelineCompute* pipeline) noexcept
	{
		(void)pipeline;
		return ZG_WARNING_UNIMPLEMENTED;
	}

	// Pipeline render methods
	// --------------------------------------------------------------------------------------------

	ZgResult pipelineRenderCreateFromFileHLSL(
		ZgPipelineRender** pipelineOut,
		ZgPipelineBindingsSignature* bindingsSignatureOut,
		ZgPipelineRenderSignature* renderSignatureOut,
		const ZgPipelineRenderCreateInfo& createInfo,
		const ZgPipelineCompileSettingsHLSL& compileSettings) noexcept
	{
		(void)pipelineOut;
		(void)bindingsSignatureOut;
		(void)renderSignatureOut;
		(void)createInfo;
		(void)compileSettings;
		return ZG_WARNING_UNIMPLEMENTED;
	}

	ZgResult pipelineRenderCreateFromSourceHLSL(
		ZgPipelineRender** pipelineOut,
		ZgPipelineBindingsSignature* bindingsSignatureOut,
		ZgPipelineRenderSignature* renderSignatureOut,
		const ZgPipelineRenderCreateInfo& createInfo,
		const ZgPipelineCompileSettingsHLSL& compileSettings) noexcept
	{
		(void)pipelineOut;
		(void)bindingsSignatureOut;
		(void)renderSignatureOut;
		(void)createInfo;
		(void)compileSettings;
		return ZG_WARNING_UNIMPLEMENTED;
	}

	ZgResult pipelineRenderRelease(
		ZgPipelineRender* pipeline) noexcept
	{
		(void)pipeline;
		return ZG_WARNING_UNIMPLEMENTED;
	}

	// Memory methods
	// --------------------------------------------------------------------------------------------

	ZgResult memoryHeapCreate(
		ZgMemoryHeap** memoryHeapOut,
		const ZgMemoryHeapCreateInfo& createInfo) noexcept
	{
		(void)memoryHeapOut;
		(void)createInfo;
		return ZG_WARNING_UNIMPLEMENTED;
	}

	ZgResult memoryHeapRelease(
		ZgMemoryHeap* memoryHeap) noexcept
	{
		(void)memoryHeap;
		return ZG_WARNING_UNIMPLEMENTED;
	}

	// Texture methods
	// --------------------------------------------------------------------------------------------

	ZgResult texture2DGetAllocationInfo(
		ZgTexture2DAllocationInfo& allocationInfoOut,
		const ZgTexture2DCreateInfo& createInfo) noexcept
	{
		(void)allocationInfoOut;
		(void)createInfo;
		return ZG_WARNING_UNIMPLEMENTED;
	}

	// Framebuffer methods
	// --------------------------------------------------------------------------------------------

	ZgResult framebufferCreate(
		ZgFramebuffer** framebufferOut,
		const ZgFramebufferCreateInfo& createInfo) noexcept
	{
		(void)framebufferOut;
		(void)createInfo;
		return ZG_WARNING_UNIMPLEMENTED;
	}

	void framebufferRelease(
		ZgFramebuffer* framebuffer) noexcept
	{
		(void)framebuffer;
	}

	// CommandQueue methods
	// --------------------------------------------------------------------------------------------

	ZgResult getPresentQueue(ZgCommandQueue** presentQueueOut) noexcept
	{
		*presentQueueOut = &mState->presentQueue;
		return ZG_SUCCESS;
	}

	ZgResult getCopyQueue(ZgCommandQueue** copyQueueOut) noexcept
	{
		*copyQueueOut = &mState->copyQueue;
		return ZG_SUCCESS;
	}

	// Profiler methods
	// --------------------------------------------------------------------------------------------

	ZgResult profilerCreate(
		ZgProfiler** profilerOut,
		const ZgProfilerCreateInfo& createInfo) noexcept
	{
		(void)profilerOut;
		(void)createInfo;
		return ZG_WARNING_UNIMPLEMENTED;
	}

	void profilerRelease(
		ZgProfiler* profilerIn) noexcept
	{
		(void)profilerIn;
	}

	// Private methods
	// --------------------------------------------------------------------------------------------
private:
	bool mDebugMode = false;

	VulkanBackendState* mState = nullptr;
};

// Vulkan backend
// ------------------------------------------------------------------------------------------------

ZgResult createVulkanBackend(ZgBackend** backendOut, const ZgContextInitSettings& settings) noexcept
{
	// Allocate and create Vulkan backend
	ZgBackend* backend = getAllocator()->newObject<ZgBackend>(sfz_dbg("ZgBackend"));

	// Initialize backend, return nullptr if init failed
	ZgResult initRes = backend->init(settings);
	if (initRes != ZG_SUCCESS)
	{
		getAllocator()->deleteObject(backend);
		return initRes;
	}

	*backendOut = backend;
	return ZG_SUCCESS;
}

static ZgBackend* ctxState = nullptr;
inline ZgBackend* getBackend() { return ctxState; }

// Version information
// ------------------------------------------------------------------------------------------------

ZG_API uint32_t zgApiLinkedVersion(void)
{
	return ZG_COMPILED_API_VERSION;
}

// Backends
// ------------------------------------------------------------------------------------------------

ZG_API ZgBackendType zgBackendCompiledType(void)
{
#if defined(_WIN32)
	return ZG_BACKEND_D3D12;
#elif defined(ZG_VULKAN)
	return ZG_BACKEND_VULKAN;
#else
	return ZG_BACKEND_NONE;
#endif
}

// Results
// ------------------------------------------------------------------------------------------------

ZG_API const char* zgResultToString(ZgResult result)
{
	switch (result) {
	case ZG_SUCCESS: return "ZG_SUCCESS";

	case ZG_WARNING_GENERIC: return "ZG_WARNING_GENERIC";
	case ZG_WARNING_UNIMPLEMENTED: return "ZG_WARNING_UNIMPLEMENTED";
	case ZG_WARNING_ALREADY_INITIALIZED: return "ZG_WARNING_ALREADY_INITIALIZED";

	case ZG_ERROR_GENERIC: return "ZG_ERROR_GENERIC";
	case ZG_ERROR_CPU_OUT_OF_MEMORY: return "ZG_ERROR_CPU_OUT_OF_MEMORY";
	case ZG_ERROR_GPU_OUT_OF_MEMORY: return "ZG_ERROR_GPU_OUT_OF_MEMORY";
	case ZG_ERROR_NO_SUITABLE_DEVICE: return "ZG_ERROR_NO_SUITABLE_DEVICE";
	case ZG_ERROR_INVALID_ARGUMENT: return "ZG_ERROR_INVALID_ARGUMENT";
	case ZG_ERROR_SHADER_COMPILE_ERROR: return "ZG_ERROR_SHADER_COMPILE_ERROR";
	case ZG_ERROR_OUT_OF_COMMAND_LISTS: return "ZG_ERROR_OUT_OF_COMMAND_LISTS";
	case ZG_ERROR_INVALID_COMMAND_LIST_STATE: return "ZG_ERROR_INVALID_COMMAND_LIST_STATE";
	}
	return "<UNKNOWN RESULT>";
}

// Buffer
// ------------------------------------------------------------------------------------------------

ZG_API ZgResult zgMemoryHeapBufferCreate(
	ZgMemoryHeap* memoryHeap,
	ZgBuffer** bufferOut,
	const ZgBufferCreateInfo* createInfo)
{
	(void)memoryHeap;
	(void)bufferOut;
	(void)createInfo;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZG_API void zgBufferRelease(
	ZgBuffer* buffer)
{
	(void)buffer;
}

ZG_API ZgResult zgBufferMemcpyTo(
	ZgBuffer* dstBuffer,
	uint64_t dstBufferOffsetBytes,
	const void* srcMemory,
	uint64_t numBytes)
{
	(void)dstBuffer;
	(void)dstBufferOffsetBytes;
	(void)srcMemory;
	(void)numBytes;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZG_API ZgResult zgBufferMemcpyFrom(
	void* dstMemory,
	ZgBuffer* srcBuffer,
	uint64_t srcBufferOffsetBytes,
	uint64_t numBytes)
{
	(void)dstMemory;
	(void)srcBuffer;
	(void)srcBufferOffsetBytes;
	(void)numBytes;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZG_API ZgResult zgBufferSetDebugName(
	ZgBuffer* buffer,
	const char* name)
{
	(void)buffer;
	(void)name;
	return ZG_WARNING_UNIMPLEMENTED;
}

// Textures
// ------------------------------------------------------------------------------------------------

ZG_API ZgResult zgTexture2DGetAllocationInfo(
	ZgTexture2DAllocationInfo* allocationInfoOut,
	const ZgTexture2DCreateInfo* createInfo)
{
	ZG_ARG_CHECK(allocationInfoOut == nullptr, "");
	ZG_ARG_CHECK(createInfo == nullptr, "");
	ZG_ARG_CHECK(createInfo->numMipmaps == 0, "Must specify at least 1 mipmap layer (i.e. the full image)");
	ZG_ARG_CHECK(createInfo->numMipmaps > ZG_MAX_NUM_MIPMAPS, "Too many mipmaps specified");
	return getBackend()->texture2DGetAllocationInfo(*allocationInfoOut, *createInfo);
}

ZG_API ZgResult zgMemoryHeapTexture2DCreate(
	ZgMemoryHeap* memoryHeap,
	ZgTexture2D** textureOut,
	const ZgTexture2DCreateInfo* createInfo)
{
	(void)memoryHeap;
	(void)textureOut;
	(void)createInfo;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZG_API void zgTexture2DRelease(
	ZgTexture2D* texture)
{
	(void)texture;
}

ZG_API ZgResult zgTexture2DSetDebugName(
	ZgTexture2D* texture,
	const char* name)
{
	(void)texture;
	(void)name;
	return ZG_WARNING_UNIMPLEMENTED;
}

// Memory Heap
// ------------------------------------------------------------------------------------------------

ZG_API ZgResult zgMemoryHeapCreate(
	ZgMemoryHeap** memoryHeapOut,
	const ZgMemoryHeapCreateInfo* createInfo)
{
	ZG_ARG_CHECK(createInfo == nullptr, "");
	ZG_ARG_CHECK(createInfo->sizeInBytes == 0, "Can't create an empty memory heap");

	return getBackend()->memoryHeapCreate(memoryHeapOut, *createInfo);
}

ZG_API ZgResult zgMemoryHeapRelease(
	ZgMemoryHeap* memoryHeap)
{
	return getBackend()->memoryHeapRelease(memoryHeap);
}

// Pipeline Compute
// ------------------------------------------------------------------------------------------------

ZG_API ZgResult zgPipelineComputeCreateFromFileHLSL(
	ZgPipelineCompute** pipelineOut,
	ZgPipelineBindingsSignature* bindingsSignatureOut,
	ZgPipelineComputeSignature* computeSignatureOut,
	const ZgPipelineComputeCreateInfo* createInfo,
	const ZgPipelineCompileSettingsHLSL* compileSettings)
{
	ZG_ARG_CHECK(pipelineOut == nullptr, "");
	ZG_ARG_CHECK(bindingsSignatureOut == nullptr, "");
	ZG_ARG_CHECK(computeSignatureOut == nullptr, "");
	ZG_ARG_CHECK(createInfo == nullptr, "");
	ZG_ARG_CHECK(compileSettings == nullptr, "");

	return getBackend()->pipelineComputeCreateFromFileHLSL(
		pipelineOut, bindingsSignatureOut, computeSignatureOut, *createInfo, *compileSettings);
}

ZG_API ZgResult zgPipelineComputeRelease(
	ZgPipelineCompute* pipeline)
{
	return getBackend()->pipelineComputeRelease(pipeline);
}

// Pipeline Render
// ------------------------------------------------------------------------------------------------

ZG_API ZgResult zgPipelineRenderCreateFromFileHLSL(
	ZgPipelineRender** pipelineOut,
	ZgPipelineBindingsSignature* bindingsSignatureOut,
	ZgPipelineRenderSignature* renderSignatureOut,
	const ZgPipelineRenderCreateInfo* createInfo,
	const ZgPipelineCompileSettingsHLSL* compileSettings)
{
	ZG_ARG_CHECK(createInfo == nullptr, "");
	ZG_ARG_CHECK(compileSettings == nullptr, "");
	ZG_ARG_CHECK(pipelineOut == nullptr, "");
	ZG_ARG_CHECK(bindingsSignatureOut == nullptr, "");
	ZG_ARG_CHECK(renderSignatureOut == nullptr, "");
	ZG_ARG_CHECK(createInfo->vertexShader == nullptr, "");
	ZG_ARG_CHECK(createInfo->vertexShaderEntry == nullptr, "");
	ZG_ARG_CHECK(createInfo->pixelShader == nullptr, "");
	ZG_ARG_CHECK(createInfo->pixelShaderEntry == nullptr, "");
	ZG_ARG_CHECK(compileSettings->shaderModel == ZG_SHADER_MODEL_UNDEFINED, "Must specify shader model");
	ZG_ARG_CHECK(createInfo->numVertexAttributes == 0, "Must specify at least one vertex attribute");
	ZG_ARG_CHECK(createInfo->numVertexAttributes >= ZG_MAX_NUM_VERTEX_ATTRIBUTES, "Too many vertex attributes specified");
	ZG_ARG_CHECK(createInfo->numVertexBufferSlots == 0, "Must specify at least one vertex buffer");
	ZG_ARG_CHECK(createInfo->numVertexBufferSlots >= ZG_MAX_NUM_VERTEX_ATTRIBUTES, "Too many vertex buffers specified");
	ZG_ARG_CHECK(createInfo->numPushConstants >= ZG_MAX_NUM_CONSTANT_BUFFERS, "Too many push constants specified");

	return getBackend()->pipelineRenderCreateFromFileHLSL(
		pipelineOut, bindingsSignatureOut, renderSignatureOut, *createInfo, *compileSettings);
}

ZG_API ZgResult zgPipelineRenderCreateFromSourceHLSL(
	ZgPipelineRender** pipelineOut,
	ZgPipelineBindingsSignature* bindingsSignatureOut,
	ZgPipelineRenderSignature* renderSignatureOut,
	const ZgPipelineRenderCreateInfo* createInfo,
	const ZgPipelineCompileSettingsHLSL* compileSettings)
{
	ZG_ARG_CHECK(createInfo == nullptr, "");
	ZG_ARG_CHECK(compileSettings == nullptr, "");
	ZG_ARG_CHECK(pipelineOut == nullptr, "");
	ZG_ARG_CHECK(bindingsSignatureOut == nullptr, "");
	ZG_ARG_CHECK(renderSignatureOut == nullptr, "");
	ZG_ARG_CHECK(createInfo->vertexShader == nullptr, "");
	ZG_ARG_CHECK(createInfo->vertexShaderEntry == nullptr, "");
	ZG_ARG_CHECK(createInfo->pixelShader == nullptr, "");
	ZG_ARG_CHECK(createInfo->pixelShaderEntry == nullptr, "");
	ZG_ARG_CHECK(compileSettings->shaderModel == ZG_SHADER_MODEL_UNDEFINED, "Must specify shader model");
	ZG_ARG_CHECK(createInfo->numVertexAttributes == 0, "Must specify at least one vertex attribute");
	ZG_ARG_CHECK(createInfo->numVertexAttributes >= ZG_MAX_NUM_VERTEX_ATTRIBUTES, "Too many vertex attributes specified");
	ZG_ARG_CHECK(createInfo->numVertexBufferSlots == 0, "Must specify at least one vertex buffer");
	ZG_ARG_CHECK(createInfo->numVertexBufferSlots >= ZG_MAX_NUM_VERTEX_ATTRIBUTES, "Too many vertex buffers specified");
	ZG_ARG_CHECK(createInfo->numPushConstants >= ZG_MAX_NUM_CONSTANT_BUFFERS, "Too many push constants specified");

	return getBackend()->pipelineRenderCreateFromSourceHLSL(
		pipelineOut, bindingsSignatureOut, renderSignatureOut, *createInfo, *compileSettings);
}

ZG_API ZgResult zgPipelineRenderRelease(
	ZgPipelineRender* pipeline)
{
	return getBackend()->pipelineRenderRelease(pipeline);
}

// Framebuffer
// ------------------------------------------------------------------------------------------------

ZG_API ZgResult zgFramebufferCreate(
	ZgFramebuffer** framebufferOut,
	const ZgFramebufferCreateInfo* createInfo)
{
	(void)framebufferOut;
	(void)createInfo;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZG_API void zgFramebufferRelease(
	ZgFramebuffer* framebuffer)
{
	(void)framebuffer;
}

ZG_API ZgResult zgFramebufferGetResolution(
	const ZgFramebuffer* framebuffer,
	uint32_t* widthOut,
	uint32_t* heightOut)
{
	(void)framebuffer;
	(void)widthOut;
	(void)heightOut;
	return ZG_WARNING_UNIMPLEMENTED;
}

// Fence
// ------------------------------------------------------------------------------------------------

ZG_API ZgResult zgFenceCreate(
	ZgFence** fenceOut)
{
	(void)fenceOut;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZG_API void zgFenceRelease(
	ZgFence* fence)
{
	(void)fence;
}

ZG_API ZgResult zgFenceReset(
	ZgFence* fence)
{
	(void)fence;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZG_API ZgResult zgFenceCheckIfSignaled(
	const ZgFence* fence,
	ZgBool* fenceSignaledOut)
{
	(void)fence;
	(void)fenceSignaledOut;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZG_API ZgResult zgFenceWaitOnCpuBlocking(
	const ZgFence* fence)
{
	(void)fence;
	return ZG_WARNING_UNIMPLEMENTED;
}

// Profiler
// ------------------------------------------------------------------------------------------------

ZG_API ZgResult zgProfilerCreate(
	ZgProfiler** profilerOut,
	const ZgProfilerCreateInfo* createInfo)
{
	(void)profilerOut;
	(void)createInfo;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZG_API void zgProfilerRelease(
	ZgProfiler* profiler)
{
	(void)profiler;
}

ZG_API ZgResult zgProfilerGetMeasurement(
	ZgProfiler* profiler,
	uint64_t measurementId,
	float* measurementMsOut)
{
	(void)profiler;
	(void)measurementId;
	(void)measurementMsOut;
	return ZG_WARNING_UNIMPLEMENTED;
}

// Command list
// ------------------------------------------------------------------------------------------------

ZG_API ZgResult zgCommandListMemcpyBufferToBuffer(
	ZgCommandList* commandList,
	ZgBuffer* dstBuffer,
	uint64_t dstBufferOffsetBytes,
	ZgBuffer* srcBuffer,
	uint64_t srcBufferOffsetBytes,
	uint64_t numBytes)
{
	(void)commandList;
	(void)dstBuffer;
	(void)dstBufferOffsetBytes;
	(void)srcBuffer;
	(void)srcBufferOffsetBytes;
	(void)numBytes;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZG_API ZgResult zgCommandListMemcpyToTexture(
	ZgCommandList* commandList,
	ZgTexture2D* dstTexture,
	uint32_t dstTextureMipLevel,
	const ZgImageViewConstCpu* srcImageCpu,
	ZgBuffer* tempUploadBuffer)
{
	(void)commandList;
	(void)dstTexture;
	(void)dstTextureMipLevel;
	(void)srcImageCpu;
	(void)tempUploadBuffer;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZG_API ZgResult zgCommandListEnableQueueTransitionBuffer(
	ZgCommandList* commandList,
	ZgBuffer* buffer)
{
	(void)commandList;
	(void)buffer;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZG_API ZgResult zgCommandListEnableQueueTransitionTexture(
	ZgCommandList* commandList,
	ZgTexture2D* texture)
{
	(void)commandList;
	(void)texture;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZG_API ZgResult zgCommandListSetPushConstant(
	ZgCommandList* commandList,
	uint32_t shaderRegister,
	const void* data,
	uint32_t dataSizeInBytes)
{
	(void)commandList;
	(void)shaderRegister;
	(void)data;
	(void)dataSizeInBytes;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZG_API ZgResult zgCommandListSetPipelineBindings(
	ZgCommandList* commandList,
	const ZgPipelineBindings* bindings)
{
	(void)commandList;
	(void)bindings;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZG_API ZgResult zgCommandListSetPipelineCompute(
	ZgCommandList* commandList,
	ZgPipelineCompute* pipeline)
{
	(void)commandList;
	(void)pipeline;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZG_API ZgResult zgCommandListUnorderedBarrierBuffer(
	ZgCommandList* commandList,
	ZgBuffer* buffer)
{
	(void)commandList;
	(void)buffer;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZG_API ZgResult zgCommandListUnorderedBarrierTexture(
	ZgCommandList* commandList,
	ZgTexture2D* texture)
{
	(void)commandList;
	(void)texture;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZG_API ZgResult zgCommandListUnorderedBarrierAll(
	ZgCommandList* commandList)
{
	(void)commandList;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZG_API ZgResult zgCommandListDispatchCompute(
	ZgCommandList* commandList,
	uint32_t groupCountX,
	uint32_t groupCountY,
	uint32_t groupCountZ)
{
	(void)commandList;
	(void)groupCountX;
	(void)groupCountY;
	(void)groupCountZ;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZG_API ZgResult zgCommandListSetPipelineRender(
	ZgCommandList* commandList,
	ZgPipelineRender* pipeline)
{
	(void)commandList;
	(void)pipeline;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZG_API ZgResult zgCommandListSetFramebuffer(
	ZgCommandList* commandList,
	ZgFramebuffer* framebuffer,
	const ZgFramebufferRect* optionalViewport,
	const ZgFramebufferRect* optionalScissor)
{
	(void)commandList;
	(void)framebuffer;
	(void)optionalViewport;
	(void)optionalScissor;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZG_API ZgResult zgCommandListSetFramebufferViewport(
	ZgCommandList* commandList,
	const ZgFramebufferRect* viewport)
{
	(void)commandList;
	(void)viewport;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZG_API ZgResult zgCommandListSetFramebufferScissor(
	ZgCommandList* commandList,
	const ZgFramebufferRect* scissor)
{
	(void)commandList;
	(void)scissor;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZG_API ZgResult zgCommandListClearFramebufferOptimal(
	ZgCommandList* commandList)
{
	(void)commandList;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZG_API ZgResult zgCommandListClearRenderTargets(
	ZgCommandList* commandList,
	float red,
	float green,
	float blue,
	float alpha)
{
	(void)commandList;
	(void)red;
	(void)green;
	(void)blue;
	(void)alpha;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZG_API ZgResult zgCommandListClearDepthBuffer(
	ZgCommandList* commandList,
	float depth)
{
	(void)commandList;
	(void)depth;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZG_API ZgResult zgCommandListSetIndexBuffer(
	ZgCommandList* commandList,
	ZgBuffer* indexBuffer,
	ZgIndexBufferType type)
{
	(void)commandList;
	(void)indexBuffer;
	(void)type;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZG_API ZgResult zgCommandListSetVertexBuffer(
	ZgCommandList* commandList,
	uint32_t vertexBufferSlot,
	ZgBuffer* vertexBuffer)
{
	(void)commandList;
	(void)vertexBufferSlot;
	(void)vertexBuffer;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZG_API ZgResult zgCommandListDrawTriangles(
	ZgCommandList* commandList,
	uint32_t startVertexIndex,
	uint32_t numVertices)
{
	(void)commandList;
	(void)startVertexIndex;
	(void)numVertices;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZG_API ZgResult zgCommandListDrawTrianglesIndexed(
	ZgCommandList* commandList,
	uint32_t startIndex,
	uint32_t numTriangles)
{
	(void)commandList;
	(void)startIndex;
	(void)numTriangles;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZG_API ZgResult zgCommandListProfileBegin(
	ZgCommandList* commandList,
	ZgProfiler* profiler,
	uint64_t* measurementIdOut)
{
	(void)commandList;
	(void)profiler;
	(void)measurementIdOut;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZG_API ZgResult zgCommandListProfileEnd(
	ZgCommandList* commandList,
	ZgProfiler* profiler,
	uint64_t measurementId)
{
	(void)commandList;
	(void)profiler;
	(void)measurementId;
	return ZG_WARNING_UNIMPLEMENTED;
}

// Command queue
// ------------------------------------------------------------------------------------------------

ZG_API ZgResult zgCommandQueueGetPresentQueue(
	ZgCommandQueue** presentQueueOut)
{
	return getBackend()->getPresentQueue(presentQueueOut);
}

ZG_API ZgResult zgCommandQueueGetCopyQueue(
	ZgCommandQueue** copyQueueOut)
{
	return getBackend()->getCopyQueue(copyQueueOut);
}

ZG_API ZgResult zgCommandQueueSignalOnGpu(
	ZgCommandQueue* commandQueue,
	ZgFence* fenceToSignal)
{
	(void)commandQueue;
	(void)fenceToSignal;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZG_API ZgResult zgCommandQueueWaitOnGpu(
	ZgCommandQueue* commandQueue,
	const ZgFence* fence)
{
	(void)commandQueue;
	(void)fence;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZG_API ZgResult zgCommandQueueFlush(
	ZgCommandQueue* commandQueue)
{
	(void)commandQueue;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZG_API ZgResult zgCommandQueueBeginCommandListRecording(
	ZgCommandQueue* commandQueue,
	ZgCommandList** commandListOut)
{
	(void)commandQueue;
	(void)commandListOut;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZG_API ZgResult zgCommandQueueExecuteCommandList(
	ZgCommandQueue* commandQueue,
	ZgCommandList* commandList)
{
	(void)commandQueue;
	(void)commandList;
	return ZG_WARNING_UNIMPLEMENTED;
}

// Context
// ------------------------------------------------------------------------------------------------

ZG_API ZgBool zgContextAlreadyInitialized(void)
{
	return getBackend() == nullptr ? ZG_FALSE : ZG_TRUE;
}

ZG_API ZgResult zgContextInit(const ZgContextInitSettings* settings)
{
	// Can't use ZG_ARG_CHECK() here because logger is not yet initialized
	if (settings == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	if (zgContextAlreadyInitialized() == ZG_TRUE) return ZG_WARNING_ALREADY_INITIALIZED;

	ZgContext tmpContext;

	// Set default logger if none is specified
	bool usingDefaultLogger = settings->logger.log == nullptr;
	if (usingDefaultLogger) tmpContext.logger = getDefaultLogger();
	else tmpContext.logger = settings->logger;

	// Set default allocator if none is specified
	bool usingDefaultAllocator =
		settings->allocator.allocate == nullptr || settings->allocator.deallocate == nullptr;
	if (usingDefaultAllocator) tmpContext.allocator = AllocatorWrapper::createDefaultAllocator();
	else tmpContext.allocator = AllocatorWrapper::createWrapper(settings->allocator);

	// Set temporary context (without API backend). Required so rest of initialization can allocate
	// memory and log.
	setContext(tmpContext);

	// Log which logger is used
	if (usingDefaultLogger) ZG_INFO("zgContextInit(): Using default logger (printf)");
	else ZG_INFO("zgContextInit(): Using user-provided logger");

	// Log which allocator is used
	if (usingDefaultAllocator) ZG_INFO("zgContextInit(): Using default allocator");
	else ZG_INFO("zgContextInit(): Using user-provided allocator");

	// Create and allocate requested backend api
	{
		ZG_INFO("zgContextInit(): Attempting to create Vulkan backend...");
		ZgResult res = createVulkanBackend(&ctxState, *settings);
		if (res != ZG_SUCCESS) {
			ctxState = nullptr;
			ZG_ERROR("zgContextInit(): Could not create Vulkan backend, exiting.");
			return res;
		}
		ZG_INFO("zgContextInit(): Created Vulkan backend");
	}

	// Set context
	setContext(tmpContext);
	return ZG_SUCCESS;
}

ZG_API ZgResult zgContextDeinit(void)
{
	if (zgContextAlreadyInitialized() == ZG_FALSE) return ZG_SUCCESS;

	ZgContext& ctx = getContext();

	// Delete backend
	getAllocator()->deleteObject(ctxState);

	// Reset context
	ctx = {};
	ctx.allocator = AllocatorWrapper();

	return ZG_SUCCESS;
}

ZG_API ZgResult zgContextSwapchainResize(
	uint32_t width,
	uint32_t height)
{
	return getBackend()->swapchainResize(width, height);
}

ZG_API ZgResult zgContextSwapchainSetVsync(
	ZgBool vsync)
{
	return getBackend()->setVsync(vsync != ZG_FALSE);
}

ZG_API ZgResult zgContextSwapchainBeginFrame(
	ZgFramebuffer** framebufferOut,
	ZgProfiler* profiler,
	uint64_t* measurementIdOut)
{
	return getBackend()->swapchainBeginFrame(framebufferOut, profiler, measurementIdOut);
}

ZG_API ZgResult zgContextSwapchainFinishFrame(
	ZgProfiler* profiler,
	uint64_t measurementId)
{
	return getBackend()->swapchainFinishFrame(profiler, measurementId);
}

ZG_API ZgResult zgContextGetStats(ZgStats* statsOut)
{
	ZG_ARG_CHECK(statsOut == nullptr, "");
	return getBackend()->getStats(*statsOut);
}

