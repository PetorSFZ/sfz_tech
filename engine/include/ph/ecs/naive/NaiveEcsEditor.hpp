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

#include "ph/ecs/naive/NaiveECS.hpp"

#include <sfz/memory/SmartPointers.hpp>
#include <sfz/strings/StackString.hpp>

namespace ph {

using sfz::str32;
using sfz::str80;

// Helper struct
// ------------------------------------------------------------------------------------------------

struct ComponentInfo final {
	uint32_t componentType = ~0u;
	str80 componentName;
	void(*componentEditor)(
		uint8_t* state, uint8_t* componentData, NaiveEcsHeader* ecs, uint32_t entity) = nullptr;
	sfz::UniquePtr<uint8_t> editorState;
	// ^^^ Above is a bit of a hack. Editor state may NOT have a non-trival destructor (i.e. state
	// must be POD) because the destructor will never be called.
};

// NaiveEcsEditor class
// ------------------------------------------------------------------------------------------------

class NaiveEcsEditor final {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	NaiveEcsEditor() noexcept {}
	NaiveEcsEditor(const NaiveEcsEditor&) = delete;
	NaiveEcsEditor& operator= (const NaiveEcsEditor&) = delete;
	NaiveEcsEditor(NaiveEcsEditor&& other) noexcept { swap(other); }
	NaiveEcsEditor& operator= (NaiveEcsEditor&& other) noexcept { swap(other); return *this; }
	~NaiveEcsEditor() noexcept { this->destroy(); }

	// State methods
	// --------------------------------------------------------------------------------------------

	void init(
		const char* windowName,
		ComponentInfo* componentInfos,
		uint32_t numComponentInfos,
		sfz::Allocator* allocator = sfz::getDefaultAllocator());
	void swap(NaiveEcsEditor& other) noexcept;
	void destroy() noexcept;

	// Methods
	// --------------------------------------------------------------------------------------------

	void render(NaiveEcsHeader* ecs) noexcept;

private:
	// Private members
	// --------------------------------------------------------------------------------------------

	struct ReducedComponentInfo {
		str80 componentName;
		void(*componentEditor)(
			uint8_t* state, uint8_t* componentData, NaiveEcsHeader* ecs, uint32_t entity) = nullptr;
		sfz::UniquePtr<uint8_t> editorState;
	};

	str80 mWindowName;
	ReducedComponentInfo mComponentInfos[64];
	uint32_t mNumComponentInfos = 0;
	ComponentMask mFilterMask = ComponentMask::activeMask();
	str32 mFilterMaskEditBuffers[8];
	bool mCompactEntityList = false;
	uint32_t mCurrentSelectedEntity = 0;
};

} // namespace ph
