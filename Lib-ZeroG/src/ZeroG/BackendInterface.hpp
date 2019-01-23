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

#pragma once

#include <ZeroG/ZeroG-CApi.h>

namespace zg {

// PipelineRendering interface
// ------------------------------------------------------------------------------------------------

class IPipelineRendering {
public:
	virtual ~IPipelineRendering() noexcept {}
};

// Memory interface
// ------------------------------------------------------------------------------------------------

class IBuffer {
public:
	virtual ~IBuffer() noexcept {}
};

// Command lists
// ------------------------------------------------------------------------------------------------

class ICommandList {
public:
	virtual ~ICommandList() noexcept {};

	virtual ZgErrorCode beginRecording() noexcept = 0;
	virtual ZgErrorCode finishRecording() noexcept = 0;
};

// Command queue
// ------------------------------------------------------------------------------------------------

class ICommandQueue {
public:
	virtual ~ICommandQueue() noexcept {}

	virtual ZgErrorCode flush() noexcept = 0;
	virtual ZgErrorCode getCommandList(ICommandList** commandListOut) noexcept = 0;
	virtual ZgErrorCode executeCommandList(ICommandList* commandList) noexcept = 0;
};

// Context interface
// ------------------------------------------------------------------------------------------------

class IContext {
public:
	virtual ~IContext() noexcept {}

	// Context methods
	// --------------------------------------------------------------------------------------------

	virtual ZgErrorCode resize(
		uint32_t width,
		uint32_t height) noexcept = 0;

	virtual ZgErrorCode getCommandQueue(ICommandQueue** commandQueueOut) noexcept = 0;

	// Pipeline methods
	// --------------------------------------------------------------------------------------------

	virtual ZgErrorCode pipelineCreate(
		IPipelineRendering** pipelineOut,
		const ZgPipelineRenderingCreateInfo& createInfo) noexcept = 0;

	virtual ZgErrorCode pipelineRelease(
		IPipelineRendering* pipeline) noexcept = 0;

	// Memory methods
	// --------------------------------------------------------------------------------------------

	virtual ZgErrorCode bufferCreate(
		IBuffer** bufferOut,
		const ZgBufferCreateInfo& createInfo) noexcept = 0;

	virtual ZgErrorCode bufferRelease(IBuffer* buffer) noexcept = 0;

	virtual ZgErrorCode bufferMemcpyTo(
		IBuffer* dstBufferInterface,
		uint64_t bufferOffsetBytes,
		const uint8_t* srcMemory,
		uint64_t numBytes) noexcept = 0;

	// Experiments
	// --------------------------------------------------------------------------------------------

	virtual ZgErrorCode renderExperiment(
		IBuffer* buffer,
		IPipelineRendering* pipeline,
		ICommandList* commandList) noexcept = 0;
};

} // namespace zg
