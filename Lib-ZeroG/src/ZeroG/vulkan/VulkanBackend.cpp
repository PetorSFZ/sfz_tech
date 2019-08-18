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

#include "ZeroG/util/CpuAllocation.hpp"
#include "ZeroG/util/Mutex.hpp"

namespace zg {

// Vulkan Backend State
// ------------------------------------------------------------------------------------------------

struct VulkanContext final {
	VkInstance instance = nullptr; // Externally synchronized
};

struct VulkanBackendState final {

	// Collection of some externally synchronized vulkan state that could roughly be considered
	// a "context" when grouped together
	Mutex<VulkanContext> context;

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
		// Delete most state
		zgDelete(mAllocator, mState);
	}

	// State methods
	// --------------------------------------------------------------------------------------------

	ZgErrorCode init(ZgContextInitSettings& settings) noexcept
	{
		// Initialize members
		mLog = settings.logger;
		mAllocator = settings.allocator;
		mDebugMode = settings.debugMode;
		mState = zgNew<VulkanBackendState>(mAllocator, "VulkanBackendState");



		return ZG_SUCCESS;
	}

	// Context methods
	// --------------------------------------------------------------------------------------------

	ZgErrorCode swapchainResize(
		uint32_t width,
		uint32_t height) noexcept override final
	{
		return ZG_ERROR_UNIMPLEMENTED;
	}

	ZgErrorCode swapchainBeginFrame(
		ZgFramebuffer** framebufferOut) noexcept override final
	{
		return ZG_ERROR_UNIMPLEMENTED;
	}

	ZgErrorCode swapchainFinishFrame() noexcept override final
	{
		return ZG_ERROR_UNIMPLEMENTED;
	}

	ZgErrorCode fenceCreate(ZgFence** fenceOut) noexcept override final
	{
		return ZG_ERROR_UNIMPLEMENTED;
	}

	// Stats
	// --------------------------------------------------------------------------------------------

	ZgErrorCode getStats(ZgStats& statsOut) noexcept override final
	{
		return ZG_ERROR_UNIMPLEMENTED;
	}

	// Pipeline methods
	// --------------------------------------------------------------------------------------------

	ZgErrorCode pipelineRenderingCreateFromFileSPIRV(
		ZgPipelineRendering** pipelineOut,
		ZgPipelineRenderingSignature* signatureOut,
		const ZgPipelineRenderingCreateInfoFileSPIRV& createInfo) noexcept override final
	{
		return ZG_ERROR_UNIMPLEMENTED;
	}

	ZgErrorCode pipelineRenderingCreateFromFileHLSL(
		ZgPipelineRendering** pipelineOut,
		ZgPipelineRenderingSignature* signatureOut,
		const ZgPipelineRenderingCreateInfoFileHLSL& createInfo) noexcept override final
	{
		return ZG_ERROR_UNIMPLEMENTED;
	}

	ZgErrorCode pipelineRenderingCreateFromSourceHLSL(
		ZgPipelineRendering** pipelineOut,
		ZgPipelineRenderingSignature* signatureOut,
		const ZgPipelineRenderingCreateInfoSourceHLSL& createInfo) noexcept override final
	{
		return ZG_ERROR_UNIMPLEMENTED;
	}

	ZgErrorCode pipelineRenderingRelease(
		ZgPipelineRendering* pipeline) noexcept override final
	{
		return ZG_ERROR_UNIMPLEMENTED;
	}

	ZgErrorCode pipelineRenderingGetSignature(
		const ZgPipelineRendering* pipeline,
		ZgPipelineRenderingSignature* signatureOut) const noexcept override final
	{
		return ZG_ERROR_UNIMPLEMENTED;
	}

	// Memory methods
	// --------------------------------------------------------------------------------------------

	ZgErrorCode memoryHeapCreate(
		ZgMemoryHeap** memoryHeapOut,
		const ZgMemoryHeapCreateInfo& createInfo) noexcept override final
	{
		return ZG_ERROR_UNIMPLEMENTED;
	}

	ZgErrorCode memoryHeapRelease(
		ZgMemoryHeap* memoryHeap) noexcept override final
	{
		return ZG_ERROR_UNIMPLEMENTED;
	}

	ZgErrorCode bufferMemcpyTo(
		ZgBuffer* dstBufferInterface,
		uint64_t bufferOffsetBytes,
		const uint8_t* srcMemory,
		uint64_t numBytes) noexcept override final
	{
		return ZG_ERROR_UNIMPLEMENTED;
	}

	// Texture methods
	// --------------------------------------------------------------------------------------------

	ZgErrorCode texture2DGetAllocationInfo(
		ZgTexture2DAllocationInfo& allocationInfoOut,
		const ZgTexture2DCreateInfo& createInfo) noexcept override final
	{
		return ZG_ERROR_UNIMPLEMENTED;
	}

	ZgErrorCode textureHeapCreate(
		ZgTextureHeap** textureHeapOut,
		const ZgTextureHeapCreateInfo& createInfo) noexcept override final
	{
		return ZG_ERROR_UNIMPLEMENTED;
	}

	ZgErrorCode textureHeapRelease(
		ZgTextureHeap* textureHeap) noexcept override final
	{
		return ZG_ERROR_UNIMPLEMENTED;
	}

	// CommandQueue methods
	// --------------------------------------------------------------------------------------------

	ZgErrorCode getPresentQueue(ZgCommandQueue** presentQueueOut) noexcept override final
	{
		return ZG_ERROR_UNIMPLEMENTED;
	}

	ZgErrorCode getCopyQueue(ZgCommandQueue** copyQueueOut) noexcept override final
	{
		return ZG_ERROR_UNIMPLEMENTED;
	}

	// Private methods
	// --------------------------------------------------------------------------------------------
private:
	ZgLogger mLog = {};
	ZgAllocator mAllocator = {};
	bool mDebugMode = false;

	VulkanBackendState* mState = nullptr;
};

// Vulkan backend
// ------------------------------------------------------------------------------------------------

ZgErrorCode createVulkanBackend(ZgBackend** backendOut, ZgContextInitSettings& settings) noexcept
{
	// Allocate and create Vulkan backend
	VulkanBackend* backend = zgNew<VulkanBackend>(settings.allocator, "Vulkan Backend");

	// Initialize backend, return nullptr if init failed
	ZgErrorCode initRes = backend->init(settings);
	if (initRes != ZG_SUCCESS)
	{
		zgDelete(settings.allocator, backend);
		return initRes;
	}

	*backendOut = backend;
	return ZG_SUCCESS;
}

} // namespace zg
