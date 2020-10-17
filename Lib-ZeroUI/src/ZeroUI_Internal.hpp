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

#include <cstdio>

#include "ZeroUI.hpp"

#include <skipifzero_allocators.hpp>
#include <skipifzero_arrays.hpp>
#include <skipifzero_hash_maps.hpp>
#include <skipifzero_strings.hpp>

// About
// ------------------------------------------------------------------------------------------------

// This is an internal header containing the internal data structures of ZeroUI.
//
// You should normally NOT include this if you are only using ZeroUI. Rather, this header is meant
// for people who are extending ZeroUI and creating their own widgets.

namespace zui {

using sfz::mat4;
using sfz::strID;

// Base Container
// ------------------------------------------------------------------------------------------------

constexpr char BASE_CON_NAME[] = "BASE_CON";
constexpr strID BASE_CON_ID = strID(sfz::hash(BASE_CON_NAME));

// Helper functions
// ------------------------------------------------------------------------------------------------

inline vec2 calcCenterPos(vec2 pos, Align align, vec2 dims)
{
	return pos - vec2(float(align.halign), float(align.valign)) * 0.5f * dims;
}

// Context
// ------------------------------------------------------------------------------------------------

struct WidgetArchetype final {
	DrawFunc* drawFunc = nullptr;
};

struct WidgetType {
	uint32_t widgetDataSizeBytes = 0;
	GetWidgetBaseFunc* getBaseFunc = nullptr;
	GetNextWidgetBoxFunc* getNextWidgetBoxFunc = nullptr;
	HandlePointerInputFunc* handlePointerInputFunc = nullptr;
	HandleMoveInputFunc* handleMoveInputFunc = nullptr;
	sfz::Map32<strID, WidgetArchetype> archetypes;
	sfz::Array<strID> archetypeStack;
	DrawFunc* getCurrentArchetypeDrawFunc() const
	{
		sfz_assert(!archetypeStack.isEmpty());
		strID lastID = archetypeStack.last();
		const WidgetArchetype* archetype = archetypes.get(lastID);
		sfz_assert(archetype != nullptr);
		return archetype->drawFunc;
	}
};

struct WidgetCmd final {
	strID widgetID;
	void* dataPtr = nullptr;
	DrawFunc* archetypeDrawFunc = nullptr;
	sfz::Array<WidgetCmd> children;

	WidgetCmd() = default;
	WidgetCmd(strID id, void* ptr);

	template<typename T> T* data() { return (T*)dataPtr; }
	template<typename T> const T* data() const { return (const T*)dataPtr; }
	uint32_t sizeOfWidgetData() const;
	WidgetBase* getBase();
	void getNextWidgetBox(strID childID, Box* boxOut);
	void handlePointerInput(vec2 pointerPosSS);
	void handleMoveInput(Input* input, bool* moveActive);
	void draw(AttributeSet* attributes, const mat34& surfaceTransform, float lagSinceSurfaceEndSecs) const;
};

struct Surface final {
	SurfaceDesc desc = {};

	// Tmp memory
	sfz::ArenaHeap arena;

	// Transforms
	mat34 transform = mat34::identity();
	mat34 inputTransform = mat34::identity();
	vec2 pointerPosSS = vec2(-FLT_MAX); // SS = Surface Space

	// Commands
	WidgetCmd cmdRoot;
	sfz::Array<WidgetCmd*> cmdParentStack;
	WidgetCmd& getCurrentParent()
	{
		sfz_assert(!cmdParentStack.isEmpty());
		return *cmdParentStack.last();
	}
	void pushMakeParent(WidgetCmd* cmd, uint32_t numChildrenHint = 64)
	{
		sfz_assert(cmd->children.allocator() == nullptr);
		sfz_assert(cmd->children.isEmpty());
		cmd->children.init(numChildrenHint, arena.getArena(), sfz_dbg(""));
		cmdParentStack.add(cmd);
	}
	void popParent()
	{
		cmdParentStack.pop();
		sfz_assert(!cmdParentStack.isEmpty());
	}

	void init(uint32_t surfaceTmpMemoryBytes,  sfz::Allocator* allocator)
	{
		arena.init(allocator, surfaceTmpMemoryBytes, sfz_dbg(""));
		this->clear();
	}

	void clear()
	{
		cmdRoot.children.destroy();
		cmdParentStack.destroy();
		arena.getArena()->reset();
		cmdParentStack.init(64, arena.getArena(), sfz_dbg(""));

		transform = mat34::identity();
		inputTransform = mat34::identity();
		pointerPosSS = vec2(-FLT_MAX);
	}
};

struct Context final {

	sfz::Allocator* heapAllocator = nullptr;
	strID defaultID;

	// Widgets
	sfz::HashMap<strID, WidgetType> widgetTypes;

	// Surfaces
	Surface* activeSurface = nullptr;
	sfz::Array<Surface> surfaces;
	sfz::Array<Surface> recycledSurfaces;
	uint32_t surfaceTmpMemoryBytes = 0;
	void createNewSurface()
	{
		sfz_assert(activeSurface == nullptr);
		if (!recycledSurfaces.isEmpty()) surfaces.add(recycledSurfaces.pop());
		else surfaces.add().init(surfaceTmpMemoryBytes, heapAllocator);
		activeSurface = &surfaces.last();
		sfz_assert(activeSurface != nullptr);
	}

	template<typename T>
	T* getWidgetData(strID id)
	{
		sfz_assert(activeSurface != nullptr);
		Surface& surface = *activeSurface;
		auto initWidgetFunc = [](void* widgetData) {
			new (widgetData) T();
		};
		T* data = reinterpret_cast<T*>(surface.desc.getWidgetDataFunc(
			surface.desc.widgetDataFuncUserPtr, id, sizeof(T), initWidgetFunc));
		return data;
	}

	template<typename T>
	T* getWidgetData(const char* id) { return getWidgetData<T>(strID(id)); }

	// AttributeSet used when rendering
	AttributeSet attributes;
	AttributeSet defaultAttributes;
};

// Getter for the global static ZeroUI context, lives in ZeroUI.cpp.
Context& ctx();

inline WidgetCmd::WidgetCmd(strID id, void* ptr)
{
	widgetID = id;
	dataPtr = ptr;
	archetypeDrawFunc = ctx().widgetTypes.get(id)->getCurrentArchetypeDrawFunc();
	sfz_assert(archetypeDrawFunc != nullptr);
}

inline uint32_t WidgetCmd::sizeOfWidgetData() const
{
	WidgetType* type = ctx().widgetTypes.get(strID(widgetID));
	sfz_assert(type != nullptr);
	return type->widgetDataSizeBytes;
}

inline WidgetBase* WidgetCmd::getBase()
{
	WidgetType* type = ctx().widgetTypes.get(strID(widgetID));
	sfz_assert(type != nullptr);
	if (dataPtr == nullptr) return nullptr;
	sfz_assert(type->getBaseFunc != nullptr);
	return type->getBaseFunc(dataPtr);
}

inline void WidgetCmd::getNextWidgetBox(strID childID, Box* boxOut)
{
	WidgetType* type = ctx().widgetTypes.get(strID(widgetID));
	sfz_assert(type != nullptr);
	sfz_assert(type->getNextWidgetBoxFunc != nullptr);
	type->getNextWidgetBoxFunc(this, childID, boxOut);
}

inline void WidgetCmd::handlePointerInput(vec2 pointerPosSS)
{
	WidgetType* type = ctx().widgetTypes.get(strID(widgetID));
	sfz_assert(type != nullptr);
	sfz_assert(type->handlePointerInputFunc != nullptr);
	type->handlePointerInputFunc(this, pointerPosSS);
}

inline void WidgetCmd::handleMoveInput(Input* input, bool* moveActive)
{
	WidgetType* type = ctx().widgetTypes.get(strID(widgetID));
	sfz_assert(type != nullptr);
	sfz_assert(type->handleMoveInputFunc != nullptr);
	type->handleMoveInputFunc(this, input, moveActive);
}

inline void WidgetCmd::draw(AttributeSet* attributes, const mat34& surfaceTransform, float lagSinceSurfaceEndSecs) const
{
	archetypeDrawFunc(this, attributes, surfaceTransform, lagSinceSurfaceEndSecs);
}

} // namespace zui
