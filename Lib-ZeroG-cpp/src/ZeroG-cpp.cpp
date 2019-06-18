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


} // namespace zg
