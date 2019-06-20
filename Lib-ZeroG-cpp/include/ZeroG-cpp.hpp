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

#include <cstdint>

#include "ZeroG.h"

namespace zg {

// Forward declarations
// ------------------------------------------------------------------------------------------------

class Context;
class CommandQueue;
class PipelineRendering;
class MemoryHeap;
class Buffer;
class CommandList;


// Error handling
// ------------------------------------------------------------------------------------------------

enum class [[nodiscard]] ErrorCode : ZgErrorCode {
	SUCCESS = ZG_SUCCESS,
	GENERIC = ZG_ERROR_GENERIC,
	UNIMPLEMENTED = ZG_ERROR_UNIMPLEMENTED,
	ALREADY_INITIALIZED = ZG_ERROR_ALREADY_INITIALIZED,
	CPU_OUT_OF_MEMORY = ZG_ERROR_CPU_OUT_OF_MEMORY,
	GPU_OUT_OF_MEMORY = ZG_ERROR_GPU_OUT_OF_MEMORY,
	NO_SUITABLE_DEVICE = ZG_ERROR_NO_SUITABLE_DEVICE,
	INVALID_ARGUMENT = ZG_ERROR_INVALID_ARGUMENT,
	SHADER_COMPILE_ERROR = ZG_ERROR_SHADER_COMPILE_ERROR,
	OUT_OF_COMMAND_LISTS = ZG_ERROR_OUT_OF_COMMAND_LISTS,
	INVALID_COMMAND_LIST_STATE = ZG_ERROR_INVALID_COMMAND_LIST_STATE
};


// Context
// ------------------------------------------------------------------------------------------------

// The ZeroG context is the main entry point for all ZeroG functions.
//
// ZeroG actually has an implicit context (i.e., it is only possible to have a single context
// running at the time), but we pretend that there is an explicit context in order to make the user
// write their code that way.
class Context final {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	Context() noexcept = default;
	Context(const Context&) = delete;
	Context& operator= (const Context&) = delete;
	Context(Context&& other) noexcept { this->swap(other); }
	Context& operator= (Context&& other) noexcept { this->swap(other); return *this; }
	~Context() noexcept { this->deinit(); }

	// State methods
	// --------------------------------------------------------------------------------------------

	// Creates and initializes a context, see zgContextInit()
	ErrorCode init(const ZgContextInitSettings& settings) noexcept;

	// Deinitializes a context, see zgContextDeinit()
	//
	// Not necessary to call manually, will be called by the destructor.
	void deinit() noexcept;

	// Swaps two contexts. Since only one can be active, this is equal to a move in practice.
	void swap(Context& other) noexcept;

	// Version methods
	// --------------------------------------------------------------------------------------------

	// The API version used to compile ZeroG, see ZG_COMPILED_API_VERSION
	static uint32_t compiledApiVersion() noexcept { return ZG_COMPILED_API_VERSION; }

	// The API version of the ZeroG DLL you have linked with, see zgApiLinkedVersion()
	static uint32_t linkedApiVersion() noexcept;

	// Context methods
	// --------------------------------------------------------------------------------------------

	// Checks if a ZeroG context is already initialized, see zgContextAlreadyInitialized()
	static bool alreadyInitialized() noexcept;

	// Resizes the back buffers in the swap chain, safe to call every frame.
	//
	// See zgContextResize()
	ErrorCode resize(uint32_t width, uint32_t height) noexcept;

	// See zgContextGeCommandQueueGraphicsPresent()
	ErrorCode getCommandQueueGraphicsPresent(CommandQueue& commandQueueOut) noexcept;

	// See zgContextBeginFrame()
	ErrorCode beginFrame(ZgFramebuffer*& framebufferOut) noexcept;

	// See zgContextFinishFrame()
	ErrorCode finishFrame() noexcept;

	// Private members
	// --------------------------------------------------------------------------------------------
private:

	bool mInitialized = false;
};


// CommandQueue
// ------------------------------------------------------------------------------------------------

class CommandQueue final {
public:
	// Members
	// --------------------------------------------------------------------------------------------

	ZgCommandQueue* commandQueue = nullptr;

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	CommandQueue() noexcept = default;
	CommandQueue(const CommandQueue&) = delete;
	CommandQueue& operator= (const CommandQueue&) = delete;
	CommandQueue(CommandQueue&& other) noexcept { this->swap(other); }
	CommandQueue& operator= (CommandQueue&& other) noexcept { this->swap(other); return *this; }
	~CommandQueue() noexcept { this->release(); }

	// State methods
	// --------------------------------------------------------------------------------------------

	void swap(CommandQueue& other) noexcept;

	// TODO: No-op because there currently is no releasing of Command Queues...
	void release() noexcept;

	// CommandQueue methods
	// --------------------------------------------------------------------------------------------

	// See zgCommandQueueFlush()
	ErrorCode flush() noexcept;

	// See zgCommandQueueBeginCommandListRecording()
	ErrorCode beginCommandListRecording(CommandList& commandListOut) noexcept;

	// See zgCommandQueueExecuteCommandList()
	ErrorCode executeCommandList(CommandList& commandList) noexcept;
};


// PipelineRendering
// ------------------------------------------------------------------------------------------------

class PipelineRendering final {
public:
	// Members
	// --------------------------------------------------------------------------------------------

	ZgPipelineRendering* pipeline = nullptr;
	ZgPipelineRenderingSignature signature = {};

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	PipelineRendering() noexcept = default;
	PipelineRendering(const PipelineRendering&) = delete;
	PipelineRendering& operator= (const PipelineRendering&) = delete;
	PipelineRendering(PipelineRendering&& o) noexcept { this->swap(o); }
	PipelineRendering& operator= (PipelineRendering&& o) noexcept { this->swap(o); return *this; }
	~PipelineRendering() noexcept { this->release(); }

	// State methods
	// --------------------------------------------------------------------------------------------

	// See zgPipelineRenderingCreateFromFileSPIRV()
	ErrorCode createFromFileSPIRV(
		const ZgPipelineRenderingCreateInfoFileSPIRV& createInfo) noexcept;
	
	// See ZgPipelineRenderingCreateInfoFileHLSL()
	ErrorCode createFromFileHLSL(
		const ZgPipelineRenderingCreateInfoFileHLSL& createInfo) noexcept;
	
	// See ZgPipelineRenderingCreateInfoSourceHLSL()
	ErrorCode createFromSourceHLSL(
		const ZgPipelineRenderingCreateInfoSourceHLSL& createInfo) noexcept;

	void swap(PipelineRendering& other) noexcept;

	// See zgPipelineRenderingRelease()
	void release() noexcept;
};


// MemoryHeap
// ------------------------------------------------------------------------------------------------

class MemoryHeap final {
public:
	// Members
	// --------------------------------------------------------------------------------------------

	ZgMemoryHeap* memoryHeap = nullptr;

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	MemoryHeap() noexcept = default;
	MemoryHeap(const MemoryHeap&) = delete;
	MemoryHeap& operator= (const MemoryHeap&) = delete;
	MemoryHeap(MemoryHeap&& o) noexcept { this->swap(o); }
	MemoryHeap& operator= (MemoryHeap&& o) noexcept { this->swap(o); return *this; }
	~MemoryHeap() noexcept { this->release(); }

	// State methods
	// --------------------------------------------------------------------------------------------

	// See zgMemoryHeapCreate()
	ErrorCode create(const ZgMemoryHeapCreateInfo& createInfo) noexcept;

	void swap(MemoryHeap& other) noexcept;

	// See zgMemoryHeapRelease()
	void release() noexcept;

	// MemoryHeap methods
	// --------------------------------------------------------------------------------------------

	// See zgMemoryHeapBufferCreate()
	ErrorCode bufferCreate(zg::Buffer& bufferOut, const ZgBufferCreateInfo& createInfo) noexcept;
};


// Buffer
// ------------------------------------------------------------------------------------------------

class Buffer final {
public:
	// Members
	// --------------------------------------------------------------------------------------------

	ZgBuffer* buffer = nullptr;

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	Buffer() noexcept = default;
	Buffer(const Buffer&) = delete;
	Buffer& operator= (const Buffer&) = delete;
	Buffer(Buffer&& o) noexcept { this->swap(o); }
	Buffer& operator= (Buffer&& o) noexcept { this->swap(o); return *this; }
	~Buffer() noexcept { this->release(); }

	// State methods
	// --------------------------------------------------------------------------------------------

	void swap(Buffer& other) noexcept;

	// See zgBufferRelease()
	void release() noexcept;

	// Buffer methods
	// --------------------------------------------------------------------------------------------

	// See zgBufferMemcpyTo()
	ErrorCode memcpyTo(uint64_t bufferOffsetBytes, const void* srcMemory, uint64_t numBytes);
};


// CommandList
// ------------------------------------------------------------------------------------------------

class CommandList final {
public:
	// Members
	// --------------------------------------------------------------------------------------------

	ZgCommandList* commandList = nullptr;

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	CommandList() noexcept = default;
	CommandList(const CommandList&) = delete;
	CommandList& operator= (const CommandList&) = delete;
	CommandList(CommandList&& other) noexcept { this->swap(other); }
	CommandList& operator= (CommandList&& other) noexcept { this->swap(other); return *this; }
	~CommandList() noexcept { this->release(); }

	// State methods
	// --------------------------------------------------------------------------------------------

	void swap(CommandList& other) noexcept;

	void release() noexcept;

	// CommandList methods
	// --------------------------------------------------------------------------------------------

	// See zgCommandListMemcpyBufferToBuffer()
	ErrorCode memcpyBufferToBuffer(
		zg::Buffer& dstBuffer,
		uint64_t dstBufferOffsetBytes,
		zg::Buffer& srcBuffer,
		uint64_t srcBufferOffsetBytes,
		uint64_t numBytes) noexcept;

	// See zgCommandListSetPipelineRendering()
	ErrorCode setPipeline(PipelineRendering& pipeline) noexcept;
};


} // namespace zg
