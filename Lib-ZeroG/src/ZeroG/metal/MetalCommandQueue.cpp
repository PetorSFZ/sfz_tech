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

#include "ZeroG/metal/MetalCommandQueue.hpp"

namespace zg {

// MetalCommandQueue: Constructors & destructors
// ------------------------------------------------------------------------------------------------

MetalCommandQueue::~MetalCommandQueue() noexcept
{

}

// MetalCommandQueue: Virtual methods
// ------------------------------------------------------------------------------------------------

ZgErrorCode MetalCommandQueue::signalOnGpu(ZgFence& fenceToSignal) noexcept
{
	(void)fenceToSignal;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZgErrorCode MetalCommandQueue::waitOnGpu(const ZgFence& fence) noexcept
{
	(void)fence;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZgErrorCode MetalCommandQueue::flush() noexcept
{
	return ZG_WARNING_UNIMPLEMENTED;
}

ZgErrorCode MetalCommandQueue::beginCommandListRecording(ZgCommandList** commandListOut) noexcept
{
	*commandListOut = &this->hackCommandList;
	return ZG_WARNING_UNIMPLEMENTED;
}

ZgErrorCode MetalCommandQueue::executeCommandList(ZgCommandList* commandList) noexcept
{
	(void)commandList;
	return ZG_WARNING_UNIMPLEMENTED;
}

} // namespace zg
