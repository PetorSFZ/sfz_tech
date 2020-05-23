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

#include <atomic>

#include "d3d12/D3D12Common.hpp"

// D3D12DescriptorRingBuffer class
// ------------------------------------------------------------------------------------------------

// A GPU descriptor ring buffer
//
// Meant to be used as a single descriptor heap used for all queues, command lists and frames.
// Essentially an atomic counter keeps track of the head of the ring buffer, anyone can allocate
// a range of descriptors from the top. The idea is that the heap itself will be so large that by
// the time the head has wrapped around and reached previously allocated descriptors they are no
// longer in use.
class D3D12DescriptorRingBuffer final {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	D3D12DescriptorRingBuffer() noexcept = default;
	D3D12DescriptorRingBuffer(const D3D12DescriptorRingBuffer&) = delete;
	D3D12DescriptorRingBuffer& operator= (const D3D12DescriptorRingBuffer&) = delete;
	D3D12DescriptorRingBuffer(D3D12DescriptorRingBuffer&&) = delete;
	D3D12DescriptorRingBuffer& operator= (D3D12DescriptorRingBuffer&&) = delete;
	~D3D12DescriptorRingBuffer() noexcept
	{
		if (descriptorHeap != nullptr) {
			ID3D12Pageable* heapPagable[1] = { descriptorHeap.Get() };
			CHECK_D3D12 mDevice->Evict(1, heapPagable);
		}
	}

	// State methods
	// --------------------------------------------------------------------------------------------

	ZgResult create(
		ID3D12Device3& device,
		D3D12_DESCRIPTOR_HEAP_TYPE type,
		uint32_t numDescriptors) noexcept
	{
		mDevice = &device;
		mNumDescriptors = numDescriptors;

		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.Type = type;
		desc.NumDescriptors = numDescriptors;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		desc.NodeMask = 0;

		// Create descriptor heap
		if (D3D12_FAIL(device.CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)))) {
			return ZG_ERROR_GPU_OUT_OF_MEMORY;
		}

		// Make descriptor heap resident
		ID3D12Pageable* heapPagable[1] = { descriptorHeap.Get() };
		if (D3D12_FAIL(device.MakeResident(1, heapPagable))) {
			return ZG_ERROR_GPU_OUT_OF_MEMORY;
		}

		// Get size of descriptors of this type
		this->descriptorSize = device.GetDescriptorHandleIncrementSize(type);

		// Get start of heap
		mHeapStartCpu = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
		mHeapStartGpu = descriptorHeap->GetGPUDescriptorHandleForHeapStart();

		return ZG_SUCCESS;
	}

	// Methods
	// --------------------------------------------------------------------------------------------

	ZgResult allocateDescriptorRange(
		uint32_t numDescriptors,
		D3D12_CPU_DESCRIPTOR_HANDLE& rangeStartCpu,
		D3D12_GPU_DESCRIPTOR_HANDLE& rangeStartGpu) noexcept
	{
		// Allocate range
		uint64_t rangeStart = mHeadPointer.fetch_add(numDescriptors);

		// Map range to the ringbuffers allowed indices
		uint32_t mappedRangeStart = uint32_t(rangeStart % uint64_t(mNumDescriptors));

		// Check if range fits continuously, if not, try again recursively
		bool rangeIsContinuous = (mappedRangeStart + numDescriptors) <= mNumDescriptors;
		if (!rangeIsContinuous) {
			return this->allocateDescriptorRange(numDescriptors, rangeStartCpu, rangeStartGpu);
		}

		// Return descriptors to the start of the range
		rangeStartCpu.ptr = mHeapStartCpu.ptr + descriptorSize * mappedRangeStart;
		rangeStartGpu.ptr = mHeapStartGpu.ptr + descriptorSize * mappedRangeStart;

		return ZG_SUCCESS;
	}

	// Public members
	// --------------------------------------------------------------------------------------------
	
	ComPtr<ID3D12DescriptorHeap> descriptorHeap;
	uint32_t descriptorSize;

private:
	// Private members
	// --------------------------------------------------------------------------------------------

	ID3D12Device3* mDevice = nullptr;
	std::atomic_uint64_t mHeadPointer = 0;
	uint32_t mNumDescriptors = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE mHeapStartCpu;
	D3D12_GPU_DESCRIPTOR_HANDLE mHeapStartGpu;
};
