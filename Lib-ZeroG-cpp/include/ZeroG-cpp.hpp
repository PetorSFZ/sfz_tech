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
class CommandList;

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
	ZgErrorCode init(const ZgContextInitSettings& settings) noexcept;

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

	// Resizes the back buffers in the swap chain, safe to call every frame. See zgContextResize()
	ZgErrorCode resize(uint32_t width, uint32_t height) noexcept;

	ZgErrorCode getCommandQueueGraphicsPresent(CommandQueue& commandQueueOut) noexcept;

	ZgErrorCode beginFrame(ZgFramebuffer*& framebufferOut) noexcept;

	ZgErrorCode finishFrame() noexcept;

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
	void release() noexcept;

	// CommandQueue methods
	// --------------------------------------------------------------------------------------------

	ZgErrorCode flush() noexcept;

	ZgErrorCode beginCommandListRecording(CommandList& commandListOut) noexcept;

	ZgErrorCode executeCommandList(CommandList& commandList) noexcept;
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


};


} // namespace zg
