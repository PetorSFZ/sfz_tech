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

#pragma once

#include <cstdint>

#include <sfz/memory/Allocator.hpp>

namespace ph {

using sfz::Allocator;
using std::uint32_t;

// Renderer class
// ------------------------------------------------------------------------------------------------

extern "C" struct FunctionTable; // Forward declare internal FunctionTable

class Renderer final {
public:
	// Public constants
	// --------------------------------------------------------------------------------------------

	/// The interface version supported by this wrapper. Only renderers which return the same
	/// version with "phRendererInterfaceVersion()" are compatible.
	static constexpr uint32_t INTERFACE_VERSION = 1;

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	Renderer() noexcept = default;
	Renderer(const Renderer&) = delete;
	Renderer& operator= (const Renderer&) = delete;

	Renderer(const char* modulePath, Allocator* allocator) noexcept;
	Renderer(Renderer&& other) noexcept; 
	Renderer& operator= (Renderer&& other) noexcept;
	~Renderer() noexcept;

	// Methods
	// --------------------------------------------------------------------------------------------

	/// Loads this renderer
	/// \param modulePath the path to the DLL (on Windows)
	/// \param allocator the sfz Allocator used to allocate memory on the CPU for this renderer
	void load(const char* modulePath, Allocator* allocator) noexcept;

	/// Swaps this renderer with another renderer
	void swap(Renderer& other) noexcept;

	/// Destroys this renderer
	void destroy() noexcept;

	// Renderer functions
	// --------------------------------------------------------------------------------------------

	/// See phRendererInterfaceVersion()
	uint32_t rendererInterfaceVersion() const noexcept;

private:
	void* mModuleHandle = nullptr; // Holds a HMODULE on Windows
	Allocator* mAllocator = nullptr;
	struct FunctionTable* mFunctionTable = nullptr;
};

} // namespace ph
