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

#include "ZeroG-cpp.hpp"

#include <utility>

namespace zg {

// Context: State methods
// ------------------------------------------------------------------------------------------------

ZgErrorCode Context::init(const ZgContextInitSettings& settings) noexcept
{
	this->deinit();
	ZgErrorCode res = zgContextInit(&settings);
	mInitialized = res == ZG_SUCCESS;
	return res;
}

void Context::deinit() noexcept
{
	if (mInitialized) zgContextDeinit();
	mInitialized = false;
}

void Context::swap(Context& other) noexcept
{
	std::swap(mInitialized, other.mInitialized);
}

// Context: Version methods
// ------------------------------------------------------------------------------------------------

uint32_t Context::linkedApiVersion() noexcept
{
	return zgApiLinkedVersion();
}

// Context: Context methods
// ------------------------------------------------------------------------------------------------

bool Context::alreadyInitialized() noexcept
{
	return zgContextAlreadyInitialized();
}

ZgErrorCode Context::resize(uint32_t width, uint32_t height) noexcept
{
	return zgContextResize(width, height);
}

ZgErrorCode Context::getCommandQueueGraphicsPresent(CommandQueue& commandQueueOut) noexcept
{
	if (commandQueueOut.commandQueue != nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	return zgContextGeCommandQueueGraphicsPresent(&commandQueueOut.commandQueue);
}

ZgErrorCode Context::beginFrame(ZgFramebuffer*& framebufferOut) noexcept
{
	if (framebufferOut != nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	return zgContextBeginFrame(&framebufferOut);
}

ZgErrorCode Context::finishFrame() noexcept
{
	return zgContextFinishFrame();
}


// CommandQueue: State methods
// ------------------------------------------------------------------------------------------------

void CommandQueue::swap(CommandQueue& other) noexcept
{
	std::swap(this->commandQueue, other.commandQueue);
}
void CommandQueue::release() noexcept
{
	// TODO: Currently there is no destruction of command queues as there is only one
	this->commandQueue = nullptr;
}

// CommandQueue: CommandQueue methods
// ------------------------------------------------------------------------------------------------

ZgErrorCode CommandQueue::flush() noexcept
{
	return zgCommandQueueFlush(this->commandQueue);
}

ZgErrorCode CommandQueue::beginCommandListRecording(CommandList& commandListOut) noexcept
{
	if (commandListOut.commandList != nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	return zgCommandQueueBeginCommandListRecording(this->commandQueue, &commandListOut.commandList);
}

ZgErrorCode CommandQueue::executeCommandList(CommandList& commandList) noexcept
{
	ZgErrorCode res = zgCommandQueueExecuteCommandList(this->commandQueue, commandList.commandList);
	commandList.commandList = nullptr;
	return res;
}


// PipelineRendering: State methods
// ------------------------------------------------------------------------------------------------

ZgErrorCode PipelineRendering::createFromFileSPIRV(
	const ZgPipelineRenderingCreateInfoFileSPIRV& createInfo) noexcept
{
	this->release();
	return zgPipelineRenderingCreateFromFileSPIRV(&this->pipeline, &this->signature, &createInfo);
}

ZgErrorCode PipelineRendering::createFromFileHLSL(
	const ZgPipelineRenderingCreateInfoFileHLSL& createInfo) noexcept
{
	this->release();
	return zgPipelineRenderingCreateFromFileHLSL(&this->pipeline, &this->signature, &createInfo);
}

ZgErrorCode PipelineRendering::createFromSourceHLSL(
	const ZgPipelineRenderingCreateInfoSourceHLSL& createInfo) noexcept
{
	this->release();
	return zgPipelineRenderingCreateFromSourceHLSL(&this->pipeline, &this->signature, &createInfo);
}

void PipelineRendering::swap(PipelineRendering& other) noexcept
{
	std::swap(this->pipeline, other.pipeline);
	std::swap(this->signature, other.signature);
}

void PipelineRendering::release() noexcept
{
	if (this->pipeline != nullptr) zgPipelineRenderingRelease(this->pipeline);
	this->pipeline = nullptr;
	this->signature = {};
}


// MemoryHeap: State methods
// ------------------------------------------------------------------------------------------------

ZgErrorCode MemoryHeap::create(const ZgMemoryHeapCreateInfo& createInfo) noexcept
{
	this->release();
	return zgMemoryHeapCreate(&this->memoryHeap, &createInfo);
}

void MemoryHeap::swap(MemoryHeap& other) noexcept
{
	std::swap(this->memoryHeap, other.memoryHeap);
}

void MemoryHeap::release() noexcept
{
	if (this->memoryHeap != nullptr) zgMemoryHeapRelease(this->memoryHeap);
	this->memoryHeap = nullptr;
}

// MemoryHeap: MemoryHeap methods
// ------------------------------------------------------------------------------------------------

ZgErrorCode MemoryHeap::bufferCreate(zg::Buffer& bufferOut, const ZgBufferCreateInfo& createInfo) noexcept
{
	bufferOut.release();
	ZgErrorCode res = zgMemoryHeapBufferCreate(this->memoryHeap, &bufferOut.buffer, &createInfo);
	return res;
}


// Buffer: State methods
// --------------------------------------------------------------------------------------------

void Buffer::swap(Buffer& other) noexcept
{
	std::swap(this->buffer, other.buffer);
}

void Buffer::release() noexcept
{
	if (this->buffer != nullptr) {
		zgBufferRelease(this->buffer);
	}
	this->buffer = nullptr;
}

// Buffer: Buffer methods
// --------------------------------------------------------------------------------------------

ZgErrorCode Buffer::memcpyTo(uint64_t bufferOffsetBytes, const void* srcMemory, uint64_t numBytes)
{
	return zgBufferMemcpyTo(this->buffer, bufferOffsetBytes, srcMemory, numBytes);
}


// CommandList: State methods
// ------------------------------------------------------------------------------------------------

void CommandList::swap(CommandList& other) noexcept
{
	std::swap(this->commandList, other.commandList);
}

void CommandList::release() noexcept
{
	// TODO: Currently there is no destruction of command lists as they are owned by the
	//       CommandQueue.
	this->commandList = nullptr;
}

// CommandList: CommandList methods
// ------------------------------------------------------------------------------------------------

ZgErrorCode CommandList::memcpyBufferToBuffer(
	zg::Buffer& dstBuffer,
	uint64_t dstBufferOffsetBytes,
	zg::Buffer& srcBuffer,
	uint64_t srcBufferOffsetBytes,
	uint64_t numBytes) noexcept
{
	return zgCommandListMemcpyBufferToBuffer(
		this->commandList,
		dstBuffer.buffer,
		dstBufferOffsetBytes,
		srcBuffer.buffer,
		srcBufferOffsetBytes,
		numBytes);
}

ZgErrorCode CommandList::setPipeline(PipelineRendering& pipeline) noexcept
{
	return zgCommandListSetPipelineRendering(this->commandList, pipeline.pipeline);
}

} // namespace zg
