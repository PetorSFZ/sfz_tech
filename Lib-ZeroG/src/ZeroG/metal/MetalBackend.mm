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

#include "ZeroG/metal/MetalBackend.hpp"

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include <mtlpp.hpp>

#include "ZeroG/metal/MetalCommandQueue.hpp"
#include "ZeroG/metal/MetalFramebuffer.hpp"
#include "ZeroG/util/Assert.hpp"
#include "ZeroG/util/CpuAllocation.hpp"
#include "ZeroG/util/Logging.hpp"

namespace zg {

// Metal Backend State
// ------------------------------------------------------------------------------------------------

struct MetalBackendState final {

	// Device
	CAMetalLayer* metalLayer = nullptr;
	mtlpp::Device device;

	// Command queues
	MetalCommandQueue presentQueue;

	// Swapchain framebuffer
	MetalFramebuffer swapchainFramebuffer;
};

// Statics
// ------------------------------------------------------------------------------------------------

// Metal Backend implementation
// ------------------------------------------------------------------------------------------------

class MetalBackend final : public ZgBackend {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	MetalBackend() = default;
	MetalBackend(const MetalBackend&) = delete;
	MetalBackend& operator= (const MetalBackend&) = delete;
	MetalBackend(MetalBackend&&) = delete;
	MetalBackend& operator= (MetalBackend&&) = delete;

	virtual ~MetalBackend() noexcept
	{
	}

	// State methods
	// --------------------------------------------------------------------------------------------

	ZgResult init(ZgContextInitSettings& settings) noexcept
	{
		// Initialize members and create state struct
		mDebugMode = settings.debugMode;
		mState = zgNew<MetalBackendState>("MetalBackendState");

		// Get CAMetalLayer from init settings
		mState->metalLayer = (__bridge CAMetalLayer*)settings.nativeHandle;

		// Bridge device from CAMetalLayer
		mState->device = ns::Handle{(__bridge void*)mState->metalLayer.device};

		// Initialize present queue
		mState->presentQueue.queue = mState->device.NewCommandQueue();

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
		// Grab next drawable from the metal layer
		id<CAMetalDrawable> drawable = [mState->metalLayer nextDrawable];
		mState->swapchainFramebuffer.drawable = ns::Handle{(__bridge void*)drawable};
		mState->swapchainFramebuffer.texture  = ns::Handle{(__bridge void*)drawable.texture};

		// Return swapchain framebuffer
		*framebufferOut = &mState->swapchainFramebuffer;
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
		(void)copyQueueOut;
		return ZG_WARNING_UNIMPLEMENTED;
	}

	// Private methods
	// --------------------------------------------------------------------------------------------
private:
	bool mDebugMode = false;

	MetalBackendState* mState = nullptr;
};

// Metal backend
// ------------------------------------------------------------------------------------------------

ZgResult createMetalBackend(ZgBackend** backendOut, ZgContextInitSettings& settings) noexcept
{
	// Allocate and create Metal backend
	MetalBackend* backend = zgNew<MetalBackend>("Metal Backend");

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
