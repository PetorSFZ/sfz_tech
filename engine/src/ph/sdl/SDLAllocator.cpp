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

#include "ph/sdl/SDLAllocator.hpp"

#include <cstring>

#include <SDL.h>
#include <SDL_stdinc.h>

#include <sfz/Assert.hpp>
#include <sfz/Logging.hpp>
#include <sfz/containers/HashMap.hpp>

namespace ph {

namespace sdl {

// Statics
// ------------------------------------------------------------------------------------------------

#ifndef __EMSCRIPTEN__

struct BridgeState final {
	sfz::Allocator* allocator = nullptr;

	// Hack because sfz::Allocators don't have realloc
	sfz::HashMap<void*, size_t> allocatedSizes;
};

static BridgeState* bridgeState = nullptr;

static SDL_free_func oldSdlFree = nullptr;

static void* SDLCALL mallocBridge(size_t size)
{
	void* ptr = bridgeState->allocator->allocate(size, 32, "SDL");
	if (ptr != nullptr) {
		bridgeState->allocatedSizes[ptr] = size;
	}
	else {
		SFZ_ERROR("SDL", "mallocBridge() failed");
	}
	return ptr;
}

static void* SDLCALL callocBridge(size_t nmemb, size_t size)
{
	size_t num_bytes = nmemb * size;
	void* ptr = bridgeState->allocator->allocate(num_bytes, 32, "SDL");
	if (ptr != nullptr) {
		bridgeState->allocatedSizes[ptr] = num_bytes;
	}
	else {
		SFZ_ERROR("SDL", "callocBridge() failed");
	}
	std::memset(ptr, 0, num_bytes);
	return ptr;
}

static void* SDLCALL reallocBridge(void* mem, size_t size)
{
	// If mem == nullptr just use callocBridge() instead.
	if (mem == nullptr) {
		return callocBridge(1, size);
	}

	// Get size of previous allocation
	size_t* sizePtr = bridgeState->allocatedSizes.get(mem);
	if (sizePtr == nullptr) {
		// TODO: If we reach here it might be because we don't support the special case where the
		// memory was allocated with the original allocator. Should hopefully never happen.
		SFZ_ERROR("SDL", "reallocBridge() failed");
		sfz_assert_release(false);
	}
	size_t sizePrevAlloc = *sizePtr;

	// Allocate new memory and copy from old to it
	void* newPtr = callocBridge(1, size);
	std::memcpy(newPtr, mem, sizePrevAlloc);

	// Deallocate old memory
	bridgeState->allocatedSizes.remove(mem);
	bridgeState->allocator->deallocate(mem);

	// Return pointer to new memory
	return newPtr;
}

static void SDLCALL freeBridge(void* mem)
{
	bool success = bridgeState->allocatedSizes.remove(mem);
	if (success) {
		bridgeState->allocator->deallocate(mem);
		return;
	}

	// If we failed it likely means that the allocation was made before we switched allocators, i.e.
	// on Windows. Attempt to deallocate using the old SDL free().
	oldSdlFree(mem);
}

#endif

// Function to set SDL allocators
// ------------------------------------------------------------------------------------------------

bool setSDLAllocator(sfz::Allocator* allocator) noexcept
{
#ifdef __EMSCRIPTEN__
	// Emscripten does not seem to support this for now. No big deal probably.
	(void)allocator;
	return true;

#else

	// Allow 2 previous allocations on Windows, 3 previous on iOS, probably a bug?
#if defined(_WIN32)
	const int MAX_NUM_SDL_ALLOCATIONS = 2;
#elif defined(SFZ_IOS)
	const int MAX_NUM_SDL_ALLOCATIONS = 3;
#else
	const int MAX_NUM_SDL_ALLOCATIONS = 0;
#endif

	// Don't switch allocators if SDL has already allocated memory.
	int numPreviousAllocations = SDL_GetNumAllocations();
	if (numPreviousAllocations != MAX_NUM_SDL_ALLOCATIONS) {
		SFZ_ERROR("PhantasyEngine", "SDL has already allocated memory, exiting.");
		return false;
	}

	// Make sure allocators are only set once
	static bool setBefore = false;
	if (setBefore) {
		SFZ_ERROR("PhantasyEngine", "Attempting to change SDL allocators again.");
		return false;
	}
	setBefore = true;

	// Allocate state
	bridgeState = allocator->newObject<BridgeState>("SDLAllocator: BridgeState");

	// Set allocator
	bridgeState->allocator = allocator;

	// Get old free() function
	SDL_GetMemoryFunctions(nullptr, nullptr, nullptr, &oldSdlFree);

	// Register allocator in SDL
	int res = SDL_SetMemoryFunctions(mallocBridge, callocBridge, reallocBridge, freeBridge);
	if (res < 0) {
		SFZ_ERROR("PhantasyEngine", "SDL_SetMemoryFunctions() failed: %s", SDL_GetError());
		return false;
	}

	// If we reach this point everything was succesful probably
	return true;
#endif
}

} // namespace sdl
} // namespace ph
