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

ErrorCode Context::init(const ZgContextInitSettings& settings) noexcept
{
	this->deinit();
	ZgErrorCode res = zgContextInit(&settings);
	mInitialized = res == ZG_SUCCESS;
	return (ErrorCode)res;
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

ErrorCode Context::resize(uint32_t width, uint32_t height) noexcept
{
	return (ErrorCode)zgContextResize(width, height);
}

ErrorCode Context::getCommandQueueGraphicsPresent(CommandQueue& commandQueueOut) noexcept
{
	if (commandQueueOut.commandQueue != nullptr) return ErrorCode::INVALID_ARGUMENT;
	return (ErrorCode)zgContextGeCommandQueueGraphicsPresent(&commandQueueOut.commandQueue);
}

ErrorCode Context::beginFrame(ZgFramebuffer*& framebufferOut) noexcept
{
	if (framebufferOut != nullptr) return ErrorCode::INVALID_ARGUMENT;
	return (ErrorCode)zgContextBeginFrame(&framebufferOut);
}

ErrorCode Context::finishFrame() noexcept
{
	return (ErrorCode)zgContextFinishFrame();
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

ErrorCode CommandQueue::flush() noexcept
{
	return (ErrorCode)zgCommandQueueFlush(this->commandQueue);
}

ErrorCode CommandQueue::beginCommandListRecording(CommandList& commandListOut) noexcept
{
	if (commandListOut.commandList != nullptr) return ErrorCode::INVALID_ARGUMENT;
	return (ErrorCode)zgCommandQueueBeginCommandListRecording(
		this->commandQueue, &commandListOut.commandList);
}

ErrorCode CommandQueue::executeCommandList(CommandList& commandList) noexcept
{
	ZgErrorCode res = zgCommandQueueExecuteCommandList(this->commandQueue, commandList.commandList);
	commandList.commandList = nullptr;
	return (ErrorCode)res;
}


// PipelineRendering: State methods
// ------------------------------------------------------------------------------------------------

ErrorCode PipelineRendering::createFromFileSPIRV(
	const ZgPipelineRenderingCreateInfoFileSPIRV& createInfo) noexcept
{
	this->release();
	return (ErrorCode)zgPipelineRenderingCreateFromFileSPIRV(
		&this->pipeline, &this->signature, &createInfo);
}

ErrorCode PipelineRendering::createFromFileHLSL(
	const ZgPipelineRenderingCreateInfoFileHLSL& createInfo) noexcept
{
	this->release();
	return (ErrorCode)zgPipelineRenderingCreateFromFileHLSL(
		&this->pipeline, &this->signature, &createInfo);
}

ErrorCode PipelineRendering::createFromSourceHLSL(
	const ZgPipelineRenderingCreateInfoSourceHLSL& createInfo) noexcept
{
	this->release();
	return (ErrorCode)zgPipelineRenderingCreateFromSourceHLSL(
		&this->pipeline, &this->signature, &createInfo);
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

ErrorCode MemoryHeap::create(const ZgMemoryHeapCreateInfo& createInfo) noexcept
{
	this->release();
	return (ErrorCode)zgMemoryHeapCreate(&this->memoryHeap, &createInfo);
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

ErrorCode MemoryHeap::bufferCreate(zg::Buffer& bufferOut, const ZgBufferCreateInfo& createInfo) noexcept
{
	bufferOut.release();
	return (ErrorCode)zgMemoryHeapBufferCreate(this->memoryHeap, &bufferOut.buffer, &createInfo);
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

ErrorCode Buffer::memcpyTo(uint64_t bufferOffsetBytes, const void* srcMemory, uint64_t numBytes)
{
	return (ErrorCode)zgBufferMemcpyTo(this->buffer, bufferOffsetBytes, srcMemory, numBytes);
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

ErrorCode CommandList::memcpyBufferToBuffer(
	zg::Buffer& dstBuffer,
	uint64_t dstBufferOffsetBytes,
	zg::Buffer& srcBuffer,
	uint64_t srcBufferOffsetBytes,
	uint64_t numBytes) noexcept
{
	return (ErrorCode)zgCommandListMemcpyBufferToBuffer(
		this->commandList,
		dstBuffer.buffer,
		dstBufferOffsetBytes,
		srcBuffer.buffer,
		srcBufferOffsetBytes,
		numBytes);
}

ErrorCode CommandList::setPipeline(PipelineRendering& pipeline) noexcept
{
	return (ErrorCode)zgCommandListSetPipelineRendering(this->commandList, pipeline.pipeline);
}

} // namespace zg
