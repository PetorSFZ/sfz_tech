// Copyright (c) 2020 Peter Hillerström (skipifzero.com, peter@hstroem.se)
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

#include <cstring> // memcpy

#include <skipifzero.hpp>
#include <skipifzero_arrays.hpp>
#include <skipifzero_hash_maps.hpp>

#include "ZeroUI.hpp"

namespace zui {

// UIStorage
// ------------------------------------------------------------------------------------------------

// ZeroUI is designed such that the user always owns the state of the UI, the clearest way to
// use it is to simply allocate all widget data needed and send pointers to them directly into the
// widget functions, e.g.:
//
// static zui::BaseContainerData aBaseContainer;
// zui::baseBegin(&aBaseContainer);
// // ...
// zui::baseEnd();
//
// However, using it this way can become a bit annoying. It's more verbose, and can actually become
// a bit tricky if the amount of and which widgets vary a lot from frame to frame. For this reason
// ZeroUI also has an alternate widget allocation mode, which uses the "GetWidgetDataFunc" callback
// which users can specify in the "SurfaceDesc".
//
// The user is free to implement the "GetWidgetDataFunc" callback however they like as long as it
// fulfills the requirements, but to make things simpler (there are a few tricky details) a standard
// implementation is provided below.
//
// Usage:
// 1. Decide the maximum number of widgets and how many bytes of data can be used per frame for the
//    the given surface. If unsure, try something out and increase as needed.
// 2. Allocate an instance of UIStorage<> for each surface, this should be stable and not move
//    around in memory between the "surfaceBegin()" and "surfaceEnd()" calls.
// 3. Use the alternate API for widgets which takes a "const char*" or "strID" instead of a
//    pointer to data.
// 4. Call UIStorage<>::progressNextFrame() after each call to "surfaceEnd()".
//
// Example:
//
// static zui::UIStorage<32, 8192> storage;
// SurfaceDesc desc;
// desc.getWidgetDataFunc = storageGetWidgetData<32, 8192>; // Make sure sizes match, otherwise memory corruption.
// desc.widgetDataFuncUserPtr = &storage;
// // ...
// zui::baseBegin("aBaseContainer");
// // ...
// zui::baseEnd();
// // ...
// zui::surfaceEnd();
// storage.progressNextFrame();

struct WidgetOffset final {
	u32 offset = ~0u;
	u32 widgetSize = 0;
};

template<u32 MAX_NUM_WIDGETS, u32 NUM_BYTES>
struct UIStorageFrame final {

	u64 FRAME_CANARY_SIZEOF = sizeof(UIStorageFrame<MAX_NUM_WIDGETS, NUM_BYTES>);
	sfz::HashMapLocal<sfz::strID, WidgetOffset, MAX_NUM_WIDGETS> offsets;
	sfz::ArrayLocal<u8, NUM_BYTES> bytes;

	void clear()
	{
		sfz_assert(FRAME_CANARY_SIZEOF == sizeof(UIStorageFrame<MAX_NUM_WIDGETS, NUM_BYTES>));
		offsets.clear();
		bytes.clear();
	}
};

template<u32 MAX_NUM_WIDGETS, u32 NUM_BYTES>
struct UIStorage final {

	u64 STORAGE_CANARY_SIZEOF = sizeof(UIStorage<MAX_NUM_WIDGETS, NUM_BYTES>);
	UIStorageFrame<MAX_NUM_WIDGETS, NUM_BYTES> frame1, frame2;
	u32 frameIdx = 0;
	
	UIStorageFrame<MAX_NUM_WIDGETS, NUM_BYTES>& prev()
	{
		if (frameIdx == 0) return frame2;
		else return frame1;
	}

	UIStorageFrame<MAX_NUM_WIDGETS, NUM_BYTES>& curr()
	{
		if (frameIdx == 0) return frame1;
		else return frame2;
	}
	
	void progressNextFrame()
	{
		frameIdx += 1;
		if (frameIdx > 1) frameIdx = 0;
		curr().clear();
		sfz_assert(STORAGE_CANARY_SIZEOF == sizeof(UIStorage<MAX_NUM_WIDGETS, NUM_BYTES>));
		sfz_assert(frame1.FRAME_CANARY_SIZEOF == sizeof(UIStorageFrame<MAX_NUM_WIDGETS, NUM_BYTES>));
		sfz_assert(frame2.FRAME_CANARY_SIZEOF == sizeof(UIStorageFrame<MAX_NUM_WIDGETS, NUM_BYTES>));
	}

	void clear()
	{
		sfz_assert(STORAGE_CANARY_SIZEOF == sizeof(UIStorage<MAX_NUM_WIDGETS, NUM_BYTES>));
		sfz_assert(frame1.FRAME_CANARY_SIZEOF == sizeof(UIStorageFrame<MAX_NUM_WIDGETS, NUM_BYTES>));
		sfz_assert(frame2.FRAME_CANARY_SIZEOF == sizeof(UIStorageFrame<MAX_NUM_WIDGETS, NUM_BYTES>));
		frame1.clear();
		frame2.clear();
	}
};

template<u32 MAX_NUM_WIDGETS, u32 MAX_NUM_BYTES>
inline void* storageGetWidgetData(void* userPtr, strID id, u32 sizeBytes, zui::InitWidgetFunc* initFunc)
{
	UIStorage<MAX_NUM_WIDGETS, MAX_NUM_BYTES>& storage =
		*reinterpret_cast<UIStorage<MAX_NUM_WIDGETS, MAX_NUM_BYTES>*>(userPtr);\
	sfz_assert(storage.STORAGE_CANARY_SIZEOF == sizeof(UIStorage<MAX_NUM_WIDGETS, MAX_NUM_BYTES>));
	UIStorageFrame<MAX_NUM_WIDGETS, MAX_NUM_BYTES>& frame = storage.curr();
	
	// Get offset to widget, allocate data and initialize it if necessary
	WidgetOffset* offset = frame.offsets.get(id);
	
	// You hit this assert if you have accidentally reused the same name (i.e. id) for multiple
	// widgets. Go back up the call stack so you find what widget you are currently creating.
	sfz_assert(offset == nullptr);
	
	if (offset == nullptr) {

		// Allocate memory for widget data and register the offsets
		u32 alignedSize = u32(sfz::roundUpAligned(sizeBytes, 16));
		WidgetOffset dataOffset;
		dataOffset.offset = frame.bytes.size();
		dataOffset.widgetSize = sizeBytes;
		sfz_assert((dataOffset.offset + alignedSize) <= MAX_NUM_BYTES);
		frame.bytes.add(u8(0), alignedSize);
		offset = &frame.offsets.put(id, dataOffset);

		// Copy widget data if available in last frame, otherwise initialize it
		UIStorageFrame<MAX_NUM_WIDGETS, MAX_NUM_BYTES>& prev = storage.prev();
		WidgetOffset* prevOffset = prev.offsets.get(id);
		if (prevOffset != nullptr) {
			sfz_assert(prevOffset->widgetSize == sizeBytes);
			sfz_assert(offset->offset < frame.bytes.size());
			void* dstPtr = frame.bytes.data() + offset->offset;
			sfz_assert(prevOffset->offset < prev.bytes.size());
			void* prevDataPtr = prev.bytes.data() + prevOffset->offset;
			memcpy(dstPtr, prevDataPtr, sizeBytes);
		}
		else {
			initFunc(frame.bytes.data() + dataOffset.offset);
		}
	}

	// Get data pointer and return it
	sfz_assert(offset->widgetSize == sizeBytes);
	sfz_assert(offset->offset < frame.bytes.size());
	void* dataPtr = frame.bytes.data() + offset->offset;
	return dataPtr;
}

} // namespace zui
