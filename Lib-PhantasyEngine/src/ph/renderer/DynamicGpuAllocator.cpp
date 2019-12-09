// Copyright (c) Peter Hillerström (skipifzero.com, peter@hstroem.se)
//               For other contributors see Contributors.txt
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

#include "ph/renderer/DynamicGpuAllocator.hpp"

#include <mutex>
#include <utility> // std::swap()

#include <skipifzero.hpp>
#include <skipifzero_arrays.hpp>
#include <skipifzero_hash_maps.hpp>

#include "ph/renderer/ZeroGUtils.hpp"

namespace sfz {

using sfz::Array;

// Constants
// ------------------------------------------------------------------------------------------------

constexpr uint32_t BUFFER_ALIGNMENT = 65536; // 64 KiB
constexpr uint32_t TEXTURE_ALIGNMENT = 65536; // 64 KiB
constexpr uint32_t MAX_NUM_PAGES = 512;
constexpr uint32_t MAX_NUM_BLOCKS_PER_PAGE = 1024;

// Private datatypes
// ------------------------------------------------------------------------------------------------

struct Block final {
	uint32_t offset = ~0u;
	uint32_t size = 0;
};

struct MemoryPage final {
	zg::MemoryHeap heap;
	Array<Block> freeBlocks;
	uint32_t pageSize = 0;
	uint32_t numAllocations = 0;
	uint32_t largestFreeBlockSize = 0;
};

struct AllocEntry final {
	Block block;
	void* heapPtr = nullptr; // Used as unique identifier to find page again
};

struct DynamicGpuAllocatorState final {

	std::mutex mutex;
	sfz::Allocator* allocator = nullptr;
	ZgMemoryType memoryType = ZG_MEMORY_TYPE_UNDEFINED;
	uint32_t pageSize = 0;

	Array<MemoryPage> pages;
	sfz::HashMap<void*, AllocEntry> entries; // ZgBuffer* or ZgTexture2D* is key

	uint64_t totalNumAllocations = 0;
	uint64_t totalNumDeallocations = 0;
};

// Statics
// ------------------------------------------------------------------------------------------------

// Returns whether a new free block was returned or not
static bool calculateNewBlocks(
	const Block& oldFreeBlock,
	uint32_t allocSize,
	uint32_t alignment,
	Block& allocBlockOut,
	Block& newFreeBlockOut) noexcept
{
	sfz_assert((oldFreeBlock.offset % alignment) == 0);
	sfz_assert(oldFreeBlock.size != 0);
	sfz_assert((oldFreeBlock.size % alignment) == 0);

	// Calculate aligned allocation size
	uint32_t leftoverAlignedSize = allocSize % alignment;
	uint32_t alignedAllocSize = allocSize;
	if (leftoverAlignedSize != 0) {
		alignedAllocSize += (alignment - leftoverAlignedSize);
	}
	sfz_assert(allocSize <= alignedAllocSize);
	sfz_assert(alignedAllocSize <= oldFreeBlock.size);
	sfz_assert((alignedAllocSize % alignment) == 0);

	// Create and return allocation block
	allocBlockOut = oldFreeBlock;
	allocBlockOut.size = alignedAllocSize;

	// Calculate and return new free block if space left
	bool createdNewFreeBlock = alignedAllocSize != oldFreeBlock.size;
	if (createdNewFreeBlock) {
		newFreeBlockOut = oldFreeBlock;
		newFreeBlockOut.offset += alignedAllocSize;
		newFreeBlockOut.size -= alignedAllocSize;
	}

	// Return whether new free block was created or not
	return createdNewFreeBlock;
}

static bool createMemoryPage(
	MemoryPage& page, uint32_t size, ZgMemoryType memoryType, sfz::Allocator* allocator) noexcept
{
	sfz_assert(size != 0);
	sfz_assert((size % BUFFER_ALIGNMENT) == 0);
	sfz_assert(!page.heap.valid());
	bool heapAllocSuccess = CHECK_ZG page.heap.create(size, memoryType);
	if (!heapAllocSuccess) return false;

	// Allocate memory for free blocks
	page.freeBlocks.init(MAX_NUM_BLOCKS_PER_PAGE, allocator, sfz_dbg(""));

	// Add initial block
	Block initialBlock;
	initialBlock.offset = 0;
	initialBlock.size = size;
	page.freeBlocks.add(initialBlock);

	// Set other info and return true
	page.pageSize = size;
	page.largestFreeBlockSize = size;
	return true;
}

template<typename ItemAllocFunc>
static bool pageAllocateItem(
	MemoryPage& page, uint32_t size, Block& allocBlockOut, ItemAllocFunc itemAllocFunc) noexcept
{
	sfz_assert(size <= page.largestFreeBlockSize);

	// Find first free block big enough
	// TODO: O(n) linear search, consider replacing with binary search
	uint32_t blockIdxToUse = ~0u;
	for (uint32_t i = 0; i < page.freeBlocks.size(); i++) {
		Block& block = page.freeBlocks[i];
		if (block.size >= size) {
			blockIdxToUse = i;
			break;
		}
	}
	if (blockIdxToUse == ~0u) return false;

	// Calculate new blocks
	Block& oldFreeBlock = page.freeBlocks[blockIdxToUse];
	Block newFreeBlock;
	Block allocBlock;
	bool createdNewFreeBlock =
		calculateNewBlocks(oldFreeBlock, size, BUFFER_ALIGNMENT, allocBlock, newFreeBlock);

	// Allocate GPU memory
	bool allocSuccess = itemAllocFunc(page, allocBlock);
	if (!allocSuccess) return false;

	// If a new free block was created, just replace the old one with it
	if (createdNewFreeBlock) {
		oldFreeBlock = newFreeBlock;
	}

	// Otherwise remove old free block completely
	else {
		page.freeBlocks.remove(blockIdxToUse);
	}

	// Find and set new largest free block size
	// TODO: O(n) memory access, consider doing something smarter since we already access most
	//       blocks earlier in this method.
	page.largestFreeBlockSize = 0;
	for (const Block& block : page.freeBlocks) {
		page.largestFreeBlockSize = sfz::max(page.largestFreeBlockSize, block.size);
	}
	sfz_assert(!(page.freeBlocks.size() != 0 && page.largestFreeBlockSize == 0));
	sfz_assert((page.largestFreeBlockSize % BUFFER_ALIGNMENT) == 0);

	// Increment number of allocation counter
	page.numAllocations += 1;

	// Return allocation block
	allocBlockOut = allocBlock;
	return true;
}

static void pageDeallocateBlock(MemoryPage& page, Block& allocatedBlock) noexcept
{
	sfz_assert(allocatedBlock.size != 0);
	sfz_assert(allocatedBlock.size <= page.pageSize);
	sfz_assert((allocatedBlock.offset + allocatedBlock.size) <= page.pageSize);
	sfz_assert((allocatedBlock.offset % BUFFER_ALIGNMENT) == 0);
#ifndef NDEBUG
	// Check that no free block overlaps with the allocated block in debug
	{
		uint32_t allocatedBegin = allocatedBlock.offset;
		uint32_t allocatedEnd = allocatedBlock.offset + allocatedBlock.size;
		for (uint32_t i = 0; i < page.freeBlocks.size(); i++) {
			const Block& block = page.freeBlocks[i];
			uint32_t blockBegin = block.offset;
			uint32_t blockEnd = block.offset + block.size;

			// TODO: I think using <= will cause edge cases because integers, but I haven't
			//       thought about it too much. Since this is just a sanity check it should be
			//       fine.
			bool overlap = blockBegin < allocatedEnd && blockEnd > allocatedBegin;
			sfz_assert(!overlap);

			// Ensure that blocks are ordered by offset
			if ((i + 1) < page.freeBlocks.size()) {
				sfz_assert(block.offset < page.freeBlocks[i + 1].offset);
			}
		}
	}
#endif

	// Find where to insert allocated block among free blocks
	uint32_t insertLoc = ~0u;
	if (page.freeBlocks.size() == 0) {
		insertLoc = 0;
	}
	else if (allocatedBlock.offset < page.freeBlocks.first().offset) {
		insertLoc = 0;
	}
	else if (allocatedBlock.offset > page.freeBlocks.last().offset) {
		insertLoc = page.freeBlocks.size();
	}
	else {
		sfz_assert(page.freeBlocks.size() >= 2);
		for (uint32_t i = 1; i < page.freeBlocks.size(); i++) {
			Block& prev = page.freeBlocks[i - 1];
			Block& curr = page.freeBlocks[i];
			if (prev.offset < allocatedBlock.offset && allocatedBlock.offset < curr.offset) {
				insertLoc = i;
				break;
			}
		}
	}
	sfz_assert(insertLoc != ~0u);

	// Insert block
	page.freeBlocks.insert(insertLoc, allocatedBlock);

	// Merge all blocks that can be merged and find largest block
	uint32_t largestBlockSize = 0;
	for (uint32_t i = 0; i < (page.freeBlocks.size() - 1);) {
		Block& currBlock = page.freeBlocks[i];
		Block& nextBlock = page.freeBlocks[i + 1];
		if ((currBlock.offset + currBlock.size) == nextBlock.offset) {
			currBlock.size = currBlock.size + nextBlock.size;
			page.freeBlocks.remove(i + 1);
		}
		else {
			i++;
		}
		largestBlockSize = sfz::max(largestBlockSize, currBlock.size);
	}

	// Small edge case where largest block is last block in list
	if (page.freeBlocks.size() > 0) {
		largestBlockSize = sfz::max(largestBlockSize, page.freeBlocks.last().size);
	}

	// Update largest free block size
	page.largestFreeBlockSize = largestBlockSize;

	// Decrement number of allocation counter
	page.numAllocations -= 1;
}

static uint32_t findAppropriatePage(Array<MemoryPage>& pages, uint32_t size) noexcept
{
	sfz_assert(size != 0);
	for (uint32_t i = 0; i < pages.size(); i++) {
		MemoryPage& page = pages[i];
		if (page.largestFreeBlockSize >= size) return i;
	}
	return ~0u;
}

// DynamicGpuAllocator: State methods
// ------------------------------------------------------------------------------------------------

void DynamicGpuAllocator::init(
	sfz::Allocator* allocator, ZgMemoryType memoryType, uint32_t pageSize) noexcept
{
	sfz_assert((pageSize % BUFFER_ALIGNMENT) == 0);
	sfz_assert((pageSize % TEXTURE_ALIGNMENT) == 0);

	this->destroy();
	mState = allocator->newObject<DynamicGpuAllocatorState>(sfz_dbg(""));
	mState->allocator = allocator;
	mState->memoryType = memoryType;
	mState->pageSize = pageSize;

	// Allocate memory for page meta data
	mState->pages.init(MAX_NUM_PAGES, allocator, sfz_dbg(""));
	mState->entries.init(MAX_NUM_PAGES * MAX_NUM_BLOCKS_PER_PAGE * 4 * 2, allocator, sfz_dbg(""));
}

void DynamicGpuAllocator::swap(DynamicGpuAllocator& other) noexcept
{
	std::swap(this->mState, other.mState);
}

void DynamicGpuAllocator::destroy() noexcept
{
	if (mState != nullptr) {
		sfz_assert(mState->entries.size() == 0);
		sfz_assert(mState->totalNumAllocations == mState->totalNumDeallocations);
		for (MemoryPage& page : mState->pages) sfz_assert(page.numAllocations == 0);
		sfz::Allocator* allocator = mState->allocator;
		allocator->deleteObject(mState);
	}
	mState = nullptr;
}

// State query methods
// ------------------------------------------------------------------------------------------------

ZgMemoryType DynamicGpuAllocator::queryMemoryType() const noexcept
{
	return mState->memoryType;
}

uint64_t DynamicGpuAllocator::queryTotalNumAllocations() const noexcept
{
	return mState->totalNumAllocations;
}

uint64_t DynamicGpuAllocator::queryTotalNumDeallocations() const noexcept
{
	return mState->totalNumDeallocations;
}

uint64_t DynamicGpuAllocator::queryDefaultPageSize() const noexcept
{
	return mState->pageSize;
}

uint32_t DynamicGpuAllocator::queryNumPages() const noexcept
{
	return mState->pages.size();
}

PageInfo DynamicGpuAllocator::queryPageInfo(uint32_t pageIdx) const noexcept
{
	std::lock_guard<std::mutex> lock(mState->mutex);
	PageInfo info;
	if (mState->pages.size() <= pageIdx) return info;
	MemoryPage& page = mState->pages[pageIdx];
	info.pageSizeBytes = page.pageSize;
	info.numAllocations = page.numAllocations;
	info.numFreeBlocks = page.freeBlocks.size();
	info.largestFreeBlockBytes = page.largestFreeBlockSize;
	return info;
}

// Allocation methods
// ------------------------------------------------------------------------------------------------

zg::Buffer DynamicGpuAllocator::allocateBuffer(uint32_t sizeBytes) noexcept
{
	std::lock_guard<std::mutex> lock(mState->mutex);
	sfz_assert(mState->memoryType != ZG_MEMORY_TYPE_TEXTURE);
	sfz_assert(mState->memoryType != ZG_MEMORY_TYPE_FRAMEBUFFER);

	// Get index of page
	uint32_t pageIdx = findAppropriatePage(mState->pages, sizeBytes);

	// If no appropriate page found, allocate one
	if (pageIdx == ~0u) {
		
		// Get page size
		// Can potentially be bigger than default size for big resources
		uint32_t pageSize = sfz::max(mState->pageSize, sizeBytes);

		// Allocate memory page
		MemoryPage page;
		bool createSuccess = createMemoryPage(page, pageSize, mState->memoryType, mState->allocator);
		sfz_assert(createSuccess);
		if (!createSuccess) return zg::Buffer();

		// Insert memory page into list of pages and set page index
		pageIdx = mState->pages.size();
		mState->pages.add(std::move(page));
	}

	// Allocate buffer
	MemoryPage& page = mState->pages[pageIdx];
	zg::Buffer buffer;
	Block bufferBlock;
	bool bufferAllocSuccess = pageAllocateItem(page, sizeBytes, bufferBlock,
		[&](MemoryPage& page, Block allocBlock) {
		return CHECK_ZG page.heap.bufferCreate(buffer, allocBlock.offset, allocBlock.size);
	});
	sfz_assert(bufferAllocSuccess);
	if (!bufferAllocSuccess) return zg::Buffer();

	// Store entry with information about allocation
	AllocEntry entry;
	entry.block = bufferBlock;
	entry.heapPtr = page.heap.memoryHeap;
	mState->entries[buffer.buffer] = entry;

	// Increment total num allocation counter
	mState->totalNumAllocations += 1;

	return buffer;
}

zg::Texture2D DynamicGpuAllocator::allocateTexture2D(
	ZgTextureFormat format,
	uint32_t width,
	uint32_t height,
	uint32_t numMipmaps,
	ZgTextureUsage usage,
	ZgOptimalClearValue optimalClearValue) noexcept
{
	std::lock_guard<std::mutex> lock(mState->mutex);
	sfz_assert(width > 0);
	sfz_assert(height > 0);
	sfz_assert(numMipmaps != 0);
	sfz_assert(numMipmaps <= ZG_MAX_NUM_MIPMAPS);
	sfz_assert(mState->memoryType != ZG_MEMORY_TYPE_UPLOAD);
	sfz_assert(mState->memoryType != ZG_MEMORY_TYPE_DOWNLOAD);
	sfz_assert(mState->memoryType != ZG_MEMORY_TYPE_DEVICE);

	// Fill in Texture2D create info and get allocation info in order to find suitable page
	ZgTexture2DCreateInfo createInfo = {};
	createInfo.format = format;
	createInfo.usage = usage;
	createInfo.optimalClearValue = optimalClearValue;
	createInfo.width = width;
	createInfo.height = height;
	createInfo.numMipmaps = numMipmaps;

	ZgTexture2DAllocationInfo allocInfo = {};
	CHECK_ZG zg::Texture2D::getAllocationInfo(allocInfo, createInfo);

	// Get index of page
	uint32_t pageIdx = findAppropriatePage(mState->pages, allocInfo.sizeInBytes);

	// If no appropriate page found, allocate one
	if (pageIdx == ~0u) {

		// Get page size
		uint32_t pageSize = sfz::max(mState->pageSize, allocInfo.sizeInBytes);

		// Allocate texture page
		MemoryPage page;
		bool createSuccess = createMemoryPage(page, pageSize, mState->memoryType, mState->allocator);
		sfz_assert(createSuccess);
		if (!createSuccess) return zg::Texture2D();

		// Insert texture page into list of pages and set page index
		pageIdx = mState->pages.size();
		mState->pages.add(std::move(page));
	}

	// Allocate texture
	MemoryPage& page = mState->pages[pageIdx];
	zg::Texture2D texture;
	Block texBlock;
	bool texAllocSuccess = pageAllocateItem(page, allocInfo.sizeInBytes, texBlock,
		[&](MemoryPage& page, Block allocBlock) {
		createInfo.offsetInBytes = allocBlock.offset;
		createInfo.sizeInBytes = allocBlock.size;
		return CHECK_ZG page.heap.texture2DCreate(texture, createInfo);
	});
	sfz_assert(texAllocSuccess);
	if (!texAllocSuccess) return zg::Texture2D();

	// Store entry with information about allocation
	AllocEntry entry;
	entry.block = texBlock;
	entry.heapPtr = page.heap.memoryHeap;
	mState->entries[texture.texture] = entry;

	// Increment total num allocation counter
	mState->totalNumAllocations += 1;

	return texture;
}

// Deallocation methods
// ------------------------------------------------------------------------------------------------

void DynamicGpuAllocator::deallocate(zg::Buffer& buffer) noexcept
{
	std::lock_guard<std::mutex> lock(mState->mutex);
	sfz_assert(buffer.valid());

	// Get entry
	AllocEntry* entryPtr = mState->entries.get(buffer.buffer);
	sfz_assert(entryPtr != nullptr);
	if (entryPtr == nullptr) return;

	// Remove entry from list of entries
	AllocEntry entry = *entryPtr;
	bool entryRemoveSuccess = mState->entries.remove(buffer.buffer);
	sfz_assert(entryRemoveSuccess);

	// Release buffer
	buffer.release();

	// Reclaim space
	sfz_assert(entry.heapPtr != nullptr);
	bool spaceReclaimed = false;
	for (uint32_t i = 0; i < mState->pages.size(); i++) {
		MemoryPage& page = mState->pages[i];
		if (page.heap.memoryHeap == entry.heapPtr) {
			
			// Deallocate block
			pageDeallocateBlock(page, entry.block);
			
			// If page is empty, release it
			// TODO: Might potentially not want to release empty pages
			if (page.freeBlocks.size() == 1) {
				Block freeBlock = page.freeBlocks[0];
				if (freeBlock.offset == 0 && freeBlock.size == page.pageSize) {
					mState->pages.remove(i);
				}
			}

			spaceReclaimed = true;
			break;
		}
	}
	sfz_assert_hard(spaceReclaimed);

	// Increment total num deallocation counter
	mState->totalNumDeallocations += 1;
}

void DynamicGpuAllocator::deallocate(zg::Texture2D& texture) noexcept
{
	std::lock_guard<std::mutex> lock(mState->mutex);
	sfz_assert(texture.valid());

	// Get entry
	AllocEntry* entryPtr = mState->entries.get(texture.texture);
	sfz_assert(entryPtr != nullptr);
	if (entryPtr == nullptr) return;

	// Remove entry from list of entries
	AllocEntry entry = *entryPtr;
	bool entryRemoveSuccess = mState->entries.remove(texture.texture);
	sfz_assert(entryRemoveSuccess);

	// Release texture
	texture.release();

	// Reclaim space
	sfz_assert(entry.heapPtr != nullptr);
	bool spaceReclaimed = false;
	for (uint32_t i = 0; i < mState->pages.size(); i++) {
		MemoryPage& page = mState->pages[i];
		if (page.heap.memoryHeap == entry.heapPtr) {

			// Deallocate block
			pageDeallocateBlock(page, entry.block);

			// If page is empty, release it
			// TODO: Might potentially not want to release empty pages
			if (page.freeBlocks.size() == 1) {
				Block freeBlock = page.freeBlocks[0];
				if (freeBlock.offset == 0 && freeBlock.size == page.pageSize) {
					mState->pages.remove(i);
				}
			}

			spaceReclaimed = true;
			break;
		}
	}
	sfz_assert_hard(spaceReclaimed);

	// Increment total num deallocation counter
	mState->totalNumDeallocations += 1;
}

} // namespace sfz
