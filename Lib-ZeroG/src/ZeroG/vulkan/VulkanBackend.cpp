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

#include "ZeroG/vulkan/VulkanBackend.hpp"

#include <vulkan/vulkan.h>

#include <skipifzero.hpp>
#include <skipifzero_arrays.hpp>

#include "ZeroG/util/Logging.hpp"
#include "ZeroG/util/Mutex.hpp"
#include "ZeroG/util/Strings.hpp"
#include "ZeroG/vulkan/VulkanCommandQueue.hpp"
#include "ZeroG/vulkan/VulkanCommon.hpp"
#include "ZeroG/vulkan/VulkanDebug.hpp"

namespace zg {

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

	VulkanCommandQueue presentQueue;
	VulkanCommandQueue copyQueue;
};

// Statics
// ------------------------------------------------------------------------------------------------


// Vulkan Backend implementation
// ------------------------------------------------------------------------------------------------

class VulkanBackend final : public ZgBackend {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	VulkanBackend() = default;
	VulkanBackend(const VulkanBackend&) = delete;
	VulkanBackend& operator= (const VulkanBackend&) = delete;
	VulkanBackend(VulkanBackend&&) = delete;
	VulkanBackend& operator= (VulkanBackend&&) = delete;

	virtual ~VulkanBackend() noexcept
	{
		// Access context
		MutexAccessor<VulkanContext> context = mState->context.access();

		// Destroy VkInstance
		if (context.data().instance != nullptr) {
			// TODO: Allocation callbacks
			if (mDebugMode) {
				auto vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)
					vkGetInstanceProcAddr(context.data().instance, "vkDestroyDebugReportCallbackEXT");
				vkDestroyDebugReportCallbackEXT(context.data().instance, vulkanDebugCallback, nullptr);
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
		vulkanLogAvailableInstanceLayers();
		vulkanLogAvailableInstanceExtensions();

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
			callbackCreateInfo.pfnCallback = &vulkanDebugReportCallback;

			// Register the callback
			// TODO: Set allocators
			auto vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)
				vkGetInstanceProcAddr(context.data().instance, "vkCreateDebugReportCallbackEXT");
			CHECK_VK vkCreateDebugReportCallbackEXT(
				context.data().instance, &callbackCreateInfo, nullptr, &vulkanDebugCallback);
		}

		// TODO: At this point we should create a VkSurface using platform specific code

		// Log available physical devices
		vulkanLogAvailablePhysicalDevices(context.data().instance, context.data().surface);

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
		vulkanLogDeviceExtensions(physicalDeviceIdx,
			context.data().physicalDevice, context.data().physicalDeviceProperties);

		// Log available queue families
		vulkanLogQueueFamilies(context.data().physicalDevice, context.data().surface);

		// TODO: Heuristic to choose queue family for present and copy queues
		//       Should require the correct flags for each queue
		const uint32_t queueFamilyIdx = 0;


		return ZG_SUCCESS;
	}

	// Context methods
	// --------------------------------------------------------------------------------------------

	ZgResult swapchainResize(
		uint32_t width,
		uint32_t height) noexcept override final
	{
		(void)width;
		(void)height;
		return ZG_WARNING_UNIMPLEMENTED;
	}

	ZgResult setVsync(
		bool vsync) noexcept override final
	{
		(void)vsync;
		return ZG_WARNING_UNIMPLEMENTED;
	}

	ZgResult swapchainBeginFrame(
		ZgFramebuffer** framebufferOut,
		ZgProfiler* profiler,
		uint64_t* measurementIdOut) noexcept override final
	{
		(void)framebufferOut;
		(void)profiler;
		(void)measurementIdOut;
		return ZG_WARNING_UNIMPLEMENTED;
	}

	ZgResult swapchainFinishFrame(
		ZgProfiler* profiler,
		uint64_t measurementId) noexcept override final
	{
		(void)profiler;
		(void)measurementId;
		return ZG_WARNING_UNIMPLEMENTED;
	}

	ZgResult fenceCreate(ZgFence** fenceOut) noexcept override final
	{
		(void)fenceOut;
		return ZG_WARNING_UNIMPLEMENTED;
	}

	// Stats
	// --------------------------------------------------------------------------------------------

	ZgResult getStats(ZgStats& statsOut) noexcept override final
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
		const ZgPipelineCompileSettingsHLSL& compileSettings) noexcept override final
	{
		(void)pipelineOut;
		(void)bindingsSignatureOut;
		(void)computeSignatureOut;
		(void)createInfo;
		(void)compileSettings;
		return ZG_WARNING_UNIMPLEMENTED;
	}

	ZgResult pipelineComputeRelease(
		ZgPipelineCompute* pipeline) noexcept override final
	{
		(void)pipeline;
		return ZG_WARNING_UNIMPLEMENTED;
	}

	// Pipeline render methods
	// --------------------------------------------------------------------------------------------

	ZgResult pipelineRenderCreateFromFileSPIRV(
		ZgPipelineRender** pipelineOut,
		ZgPipelineBindingsSignature* bindingsSignatureOut,
		ZgPipelineRenderSignature* renderSignatureOut,
		const ZgPipelineRenderCreateInfo& createInfo) noexcept override final
	{
		(void)pipelineOut;
		(void)bindingsSignatureOut;
		(void)renderSignatureOut;
		(void)createInfo;
		return ZG_WARNING_UNIMPLEMENTED;
	}

	ZgResult pipelineRenderCreateFromFileHLSL(
		ZgPipelineRender** pipelineOut,
		ZgPipelineBindingsSignature* bindingsSignatureOut,
		ZgPipelineRenderSignature* renderSignatureOut,
		const ZgPipelineRenderCreateInfo& createInfo,
		const ZgPipelineCompileSettingsHLSL& compileSettings) noexcept override final
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
		const ZgPipelineCompileSettingsHLSL& compileSettings) noexcept override final
	{
		(void)pipelineOut;
		(void)bindingsSignatureOut;
		(void)renderSignatureOut;
		(void)createInfo;
		(void)compileSettings;
		return ZG_WARNING_UNIMPLEMENTED;
	}

	ZgResult pipelineRenderRelease(
		ZgPipelineRender* pipeline) noexcept override final
	{
		(void)pipeline;
		return ZG_WARNING_UNIMPLEMENTED;
	}

	// Memory methods
	// --------------------------------------------------------------------------------------------

	ZgResult memoryHeapCreate(
		ZgMemoryHeap** memoryHeapOut,
		const ZgMemoryHeapCreateInfo& createInfo) noexcept override final
	{
		(void)memoryHeapOut;
		(void)createInfo;
		return ZG_WARNING_UNIMPLEMENTED;
	}

	ZgResult memoryHeapRelease(
		ZgMemoryHeap* memoryHeap) noexcept override final
	{
		(void)memoryHeap;
		return ZG_WARNING_UNIMPLEMENTED;
	}

	// Texture methods
	// --------------------------------------------------------------------------------------------

	ZgResult texture2DGetAllocationInfo(
		ZgTexture2DAllocationInfo& allocationInfoOut,
		const ZgTexture2DCreateInfo& createInfo) noexcept override final
	{
		(void)allocationInfoOut;
		(void)createInfo;
		return ZG_WARNING_UNIMPLEMENTED;
	}

	// Framebuffer methods
	// --------------------------------------------------------------------------------------------

	ZgResult framebufferCreate(
		ZgFramebuffer** framebufferOut,
		const ZgFramebufferCreateInfo& createInfo) noexcept override final
	{
		(void)framebufferOut;
		(void)createInfo;
		return ZG_WARNING_UNIMPLEMENTED;
	}

	void framebufferRelease(
		ZgFramebuffer* framebuffer) noexcept override final
	{
		(void)framebuffer;
	}

	// CommandQueue methods
	// --------------------------------------------------------------------------------------------

	ZgResult getPresentQueue(ZgCommandQueue** presentQueueOut) noexcept override final
	{
		*presentQueueOut = &mState->presentQueue;
		return ZG_SUCCESS;
	}

	ZgResult getCopyQueue(ZgCommandQueue** copyQueueOut) noexcept override final
	{
		*copyQueueOut = &mState->copyQueue;
		return ZG_SUCCESS;
	}

	// Profiler methods
	// --------------------------------------------------------------------------------------------

	ZgResult profilerCreate(
		ZgProfiler** profilerOut,
		const ZgProfilerCreateInfo& createInfo) noexcept override final
	{
		(void)profilerOut;
		(void)createInfo;
		return ZG_WARNING_UNIMPLEMENTED;
	}

	void profilerRelease(
		ZgProfiler* profilerIn) noexcept override final
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
	VulkanBackend* backend = getAllocator()->newObject<VulkanBackend>(sfz_dbg("VulkanBackend"));

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

} // namespace zg
