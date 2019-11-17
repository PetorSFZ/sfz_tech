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

#include "ZeroG/util/Assert.hpp"
#include "ZeroG/util/CpuAllocation.hpp"
#include "ZeroG/util/Logging.hpp"
#include "ZeroG/util/Mutex.hpp"
#include "ZeroG/util/Strings.hpp"
#include "ZeroG/util/Vector.hpp"
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
		zgDelete(mState);
	}

	// State methods
	// --------------------------------------------------------------------------------------------

	ZgResult init(ZgContextInitSettings& settings) noexcept
	{
		// Initialize members and create state struct
		mDebugMode = settings.debugMode;
		mState = zgNew<VulkanBackendState>( "VulkanBackendState");

		// Log available instance layers and extensions
		vulkanLogAvailableInstanceLayers();
		vulkanLogAvailableInstanceExtensions();

		// Application info struct
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.apiVersion = VK_API_VERSION_1_0;

		// Layers and extensions arrays
		Vector<const char*> layers;
		layers.create(64, "VulkanInit: layers");
		Vector<const char*> extensions;
		extensions.create(64, "VulkanInit: extensions");

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
			ZG_ASSERT(numPhysicalDevices <= MAX_NUM_PHYSICAL_DEVICES);
			if (numPhysicalDevices > MAX_NUM_PHYSICAL_DEVICES) numPhysicalDevices = MAX_NUM_PHYSICAL_DEVICES;

			// Retrieve physical devices
			VkPhysicalDevice physicalDevices[MAX_NUM_PHYSICAL_DEVICES] = {};
			CHECK_VK vkEnumeratePhysicalDevices(context.data().instance, &numPhysicalDevices, physicalDevices);

			// Select the choosen physical device
			ZG_ASSERT(physicalDeviceIdx < numPhysicalDevices);
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

	ZgResult swapchainBeginFrame(
		ZgFramebuffer** framebufferOut) noexcept override final
	{
		(void)framebufferOut;
		return ZG_WARNING_UNIMPLEMENTED;
	}

	ZgResult swapchainFinishFrame() noexcept override final
	{
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

	// Pipeline methods
	// --------------------------------------------------------------------------------------------

	ZgResult pipelineRenderCreateFromFileSPIRV(
		ZgPipelineRender** pipelineOut,
		ZgPipelineRenderSignature* signatureOut,
		const ZgPipelineRenderCreateInfoFileSPIRV& createInfo) noexcept override final
	{
		(void)pipelineOut;
		(void)signatureOut;
		(void)createInfo;
		return ZG_WARNING_UNIMPLEMENTED;
	}

	ZgResult pipelineRenderCreateFromFileHLSL(
		ZgPipelineRender** pipelineOut,
		ZgPipelineRenderSignature* signatureOut,
		const ZgPipelineRenderCreateInfoFileHLSL& createInfo) noexcept override final
	{
		(void)pipelineOut;
		(void)signatureOut;
		(void)createInfo;
		return ZG_WARNING_UNIMPLEMENTED;
	}

	ZgResult pipelineRenderCreateFromSourceHLSL(
		ZgPipelineRender** pipelineOut,
		ZgPipelineRenderSignature* signatureOut,
		const ZgPipelineRenderCreateInfoSourceHLSL& createInfo) noexcept override final
	{
		(void)pipelineOut;
		(void)signatureOut;
		(void)createInfo;
		return ZG_WARNING_UNIMPLEMENTED;
	}

	ZgResult pipelineRenderRelease(
		ZgPipelineRender* pipeline) noexcept override final
	{
		(void)pipeline;
		return ZG_WARNING_UNIMPLEMENTED;
	}

	ZgResult pipelineRenderGetSignature(
		const ZgPipelineRender* pipeline,
		ZgPipelineRenderSignature* signatureOut) const noexcept override final
	{
		(void)pipeline;
		(void)signatureOut;
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

	ZgResult bufferMemcpyTo(
		ZgBuffer* dstBufferInterface,
		uint64_t bufferOffsetBytes,
		const uint8_t* srcMemory,
		uint64_t numBytes) noexcept override final
	{
		(void)dstBufferInterface;
		(void)bufferOffsetBytes;
		(void)srcMemory;
		(void)numBytes;
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

	// Private methods
	// --------------------------------------------------------------------------------------------
private:
	bool mDebugMode = false;

	VulkanBackendState* mState = nullptr;
};

// Vulkan backend
// ------------------------------------------------------------------------------------------------

ZgResult createVulkanBackend(ZgBackend** backendOut, ZgContextInitSettings& settings) noexcept
{
	// Allocate and create Vulkan backend
	VulkanBackend* backend = zgNew<VulkanBackend>("Vulkan Backend");

	// Initialize backend, return nullptr if init failed
	ZgResult initRes = backend->init(settings);
	if (initRes != ZG_SUCCESS)
	{
		zgDelete(backend);
		return initRes;
	}

	*backendOut = backend;
	return ZG_SUCCESS;
}

} // namespace zg
