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

#include "ZeroG.h"
#include "ZeroG/BackendInterface.hpp"

#include <mtlpp.hpp>

#include "ZeroG/metal/MetalCommandList.hpp"

namespace zg {

// MetalCommandQueue
// ------------------------------------------------------------------------------------------------

class MetalCommandQueue final : public ZgCommandQueue {
public:

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	MetalCommandQueue() noexcept {}
	MetalCommandQueue(const MetalCommandQueue&) = delete;
	MetalCommandQueue& operator= (const MetalCommandQueue&) = delete;
	MetalCommandQueue(MetalCommandQueue&&) = delete;
	MetalCommandQueue& operator= (MetalCommandQueue&&) = delete;
	~MetalCommandQueue() noexcept;

	// Virtual methods
	// --------------------------------------------------------------------------------------------

	ZgResult signalOnGpu(ZgFence& fenceToSignal) noexcept override final;
	ZgResult waitOnGpu(const ZgFence& fence) noexcept override final;
	ZgResult flush() noexcept override final;
	ZgResult beginCommandListRecording(ZgCommandList** commandListOut) noexcept override final;
	ZgResult executeCommandList(ZgCommandList* commandList) noexcept override final;

	// Members
	// --------------------------------------------------------------------------------------------

	mtlpp::CommandQueue queue;
	MetalCommandList hackCommandList;
};

} // namespace zg
