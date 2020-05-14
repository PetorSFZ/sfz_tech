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

#include "vulkan/VulkanCommandQueue.hpp"

namespace zg {

// VulkanCommandQueue: Constructors & destructors
// ------------------------------------------------------------------------------------------------

VulkanCommandQueue::~VulkanCommandQueue() noexcept
{

}

// VulkanCommandQueue: Virtual methods
// ------------------------------------------------------------------------------------------------

ZgResult VulkanCommandQueue::signalOnGpu(ZgFence& fenceToSignal) noexcept
{
	(void)fenceToSignal;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZgResult VulkanCommandQueue::waitOnGpu(const ZgFence& fence) noexcept
{
	(void)fence;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZgResult VulkanCommandQueue::flush() noexcept
{
	return ZG_WARNING_UNIMPLEMENTED;
}

ZgResult VulkanCommandQueue::beginCommandListRecording(ZgCommandList** commandListOut) noexcept
{
	(void)commandListOut;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZgResult VulkanCommandQueue::executeCommandList(ZgCommandList* commandList) noexcept
{
	(void)commandList;
	return ZG_WARNING_UNIMPLEMENTED;
}

} // namespace zg
