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

#include "ZeroUI.h"

#include <skipifzero_allocators.hpp>
#include <skipifzero_arrays.hpp>
#include <skipifzero_hash_maps.hpp>
#include <skipifzero_math.hpp>
#include <skipifzero_strings.hpp>

using sfz::mat4;

// About
// ------------------------------------------------------------------------------------------------

// This is an internal header containing the private internal API of ZeroUI. This includes internal
// data structures and functions.
//
// You should normally NOT include this if you are only using ZeroUI. Rather, this header is meant
// for people who are extending ZeroUI and creating their own widgets.

// Box
// ------------------------------------------------------------------------------------------------

sfz_struct(ZuiBox) {
	f32x2 min, max;

#ifdef __cplusplus
	constexpr f32x2 center() const { return (min + max) * 0.5f; }
	constexpr f32x2 dims() const { return max - min; }
	constexpr bool pointInside(f32x2 p) const
	{
		return min.x <= p.x && p.x <= max.x && min.y <= p.y && p.y <= max.y;
	}
	constexpr bool overlaps(ZuiBox o) const
	{
		return min.x <= o.max.x && max.x >= o.min.x && min.y <= o.max.y && max.y >= o.min.y;
	}
#endif
};

sfz_constexpr_func ZuiBox zuiBoxInit(f32x2 center, f32x2 dims)
{
	return ZuiBox{ center - dims * 0.5f, center + dims * 0.5f };
}

// Widget
// ------------------------------------------------------------------------------------------------

// A Widget is a type of UI item, e.g. button. In ZeroUI there is only one "built-in" widget (base
// container), all other widgets are added as extensions. Thus there is no difference between a
// widget created by the user and the ones shipped with the library (see ZeroUI_CoreWidgets.hpp).
//
// There are two distinct types of widgets, containers and leafs. The difference is simple,
// containers contain other widgets, while leafs do not. E.g., a button widget is typically a leaf,
// if it is activated it simply performs some user specified logic. A list widget is a container,
// it contains a list of widgets, which can in turn be other containers or leafs.
//
// To create a widget you need to do 3 things:
// * Design a suitable immediate-mode (~ish) API, e.g. ("if (zui::button("Button")) { /* do work*/ }")
// * Design a data struct that contains all the information needed by the widget.
// * Implement the above API function, in addition to the below specified logic and rendering functions.
//
// For the API it's suggested to take a look at ZeroUI_CoreWidgets.hpp to see how the API works for
// those widgets, including looking at the implementation to get a feel for what is appropriate.
// In the end, these API functions are responsible for registering/deregistering the widget with
// the current surface in the correct way, so it's the most important thing to get right.
//
// Each Widget type need to define a data struct that contains all data needed for the widget. This
// includes logic, current state, configuration parameters, rendering info, etc. See
// "BaseContainerData" below as an example. What data it should contain is completely up to you,
// and depends on what your implementation of input handling and rendering requires.
//
// There are a couple of common rules that should apply for most (or all) widget data structs:
// * They should only contain primitive types and be default copyable (e.g. via memcpy()).
// * They must contain a copy of WidgetBase (see below)
// * The user should NEVER need to initialize a data struct, just create a copy (and run the default
//   constructor). All parameters should be applied via the widget API function.
//
// Lastly, for each widget the logic needs to be implemented. This is done by implementing the
// functions in "WidgetDesc". Not all functions are necessary for all widgets. There is no need
// for leafs to implement "GetNextWidgetBoxFunc" as they don't contain widgets. For containers
// all functions need to be specified, but many of them can often be "passthroughs" (i.e., just
// calling their childrens corresponding functions).

struct ZuiWidgetBase final {
	ZuiBox box; // The location and size of the widget on the surface
	f32 timeSinceFocusStartedSecs = F32_MAX;
	f32 timeSinceFocusEndedSecs = F32_MAX;
	f32 timeSinceActivationSecs = F32_MAX;
	bool focused = false;
	bool activated = false;
	u8 ___PADDING___[2] = {};

	void setFocused();
	void setUnfocused();
	void setActivated();
};

// Wrapper type for internal use (or when implementing widget logic), see ZeroUI_Internal.hpp.
struct ZuiWidget;

// For containers only: Return the position and size of the next widget being placed. Widgets don't
// determine their own size, they must always ask their parent what their size is.
typedef void ZuiGetNextWidgetBoxFunc(ZuiCtx* zui, ZuiWidget* widget, ZuiID childWidgetTypeID, ZuiBox* boxOut);

// Handle mouse/touch input. Typically things should be setFocused() if the pointer is overlapping the
// widget's box, otherwise setUnfocused(). MUST call all children's corresponding handle pointer input
// function.
typedef void ZuiHandlePointerInputFunc(ZuiCtx* zui, ZuiWidget* widget, f32x2 pointerPosSS);

// Handle move input (i.e. gamepad or keyboard). This is probably the most complicated part of the
// logic, so it deserves some extra explanation.
//
// The "input" and "moveActive" parameters are global and shared with the entire widget tree when
// recursively executing this function. "input" starts of with action specified by the user (NONE,
// UP, DOWN, LEFT, RIGHT), "moveActive" starts of as "false". The idea is that the logic can
// "consume" the input, but only if it is active.
//
// As an example, take a button which can be focused. If a button comes across the input DOWN with
// moveActive=false, then it calls setUnfocused() and sets moveActive to true. On the other hand,
// if it comes across DOWN with moveActive=true it calls setFocused() and then sets input to NONE
// and moveActive to false. The input is consumed and will not affect any upcoming widgets.
//
// MUST call all children's corresponding move input functions the correct way (see other containers
// in ZeroUI_CoreWidgets.hpp as example).
typedef void ZuiHandleMoveInputFunc(ZuiCtx* zui, ZuiWidget* widget, ZuiInput* input, bool* moveActive);

// Default draw function which is just a pass through, only useful for containers.
void zuiDefaultPassthroughDrawFunc(
	ZuiCtx* zui,
	const ZuiWidget* widget,
	const SfzMat34* surfaceTransform,
	f32 lagSinceInputEndSecs);

// The description of a widget with all the necessary functions.
sfz_struct(ZuiWidgetDesc) {
	u32 widgetDataSizeBytes;
	ZuiGetNextWidgetBoxFunc* getNextWidgetBoxFunc;
	ZuiHandlePointerInputFunc* handlePointerInputFunc;
	ZuiHandleMoveInputFunc* handleMoveInputFunc;
	ZuiDrawFunc* drawFunc;
};

// Registers a widget type
SFZ_EXTERN_C void zuiRegisterWidget(ZuiCtx* zui, const char* name, const ZuiWidgetDesc* desc);


// Helper functions
// ------------------------------------------------------------------------------------------------

inline f32x2 calcCenterPos(f32x2 pos, ZuiAlign align, f32x2 dims)
{
	f32x2 offset = f32x2(0.0f);
	switch (align) {
	case ZUI_BOTTOM_LEFT:
		offset = f32x2(-1.0f, -1.0f);
		break;
	case ZUI_BOTTOM_CENTER:
		offset = f32x2(0.0f, -1.0f);
		break;
	case ZUI_BOTTOM_RIGHT:
		offset = f32x2(1.0f, -1.0f);
		break;
	case ZUI_MID_LEFT:
		offset = f32x2(-1.0f, 0.0f);
		break;
	case ZUI_MID_CENTER:
		offset = f32x2(0.0f, 0.0f);
		break;
	case ZUI_MID_RIGHT:
		offset = f32x2(1.0f, 0.0f);
		break;
	case ZUI_TOP_LEFT:
		offset = f32x2(-1.0f, 1.0f);
		break;
	case ZUI_TOP_CENTER:
		offset = f32x2(0.0f, 1.0f);
		break;
	case ZUI_TOP_RIGHT:
		offset = f32x2(1.0f, 1.0f);
		break;
	default: sfz_assert_hard(false);
	}
	return pos - offset * 0.5f * dims;
}

// Draw context
// ------------------------------------------------------------------------------------------------

// Trickery to avoid having to include stb_truetype.h in this file
sfz_constant u64 stbtt_pack_context_size = 64; // sizeof(stbtt_pack_context);
struct stbtt_pack_context;
sfz_constant u32 stbtt_packedchar_size = 28; // sizeof(stbtt_packedchar);

sfz_constant u32 ZUI_FONT_NUM_RANGES = 1;
sfz_constant u32 ZUI_FIRST_PRINTABLE_ASCII_CHAR = ' ';
sfz_constant u32 ZUI_LAST_PRINTABLE_ASCII_CHAR = 126; // '~'
sfz_constant u32 ZUI_NUM_PRINTABLE_ASCII_CHARS =
	ZUI_LAST_PRINTABLE_ASCII_CHAR - ZUI_FIRST_PRINTABLE_ASCII_CHAR + 1;

sfz_constant u32 ZUI_EXTRA_RANGE_CHARS[] = {
	9633, // '□'
	229, // 'å',
	228, // 'ä',
	246, //'ö',
	197, // 'Å',
	196, // 'Ä',
	214, // 'Ö',
	169, // '©'
};
sfz_constant u32 ZUI_NUM_EXTRA_CHARS = sizeof(ZUI_EXTRA_RANGE_CHARS) / sizeof(i32);

sfz_constant u32 ZUI_FONT_TEX_RES = 4096;

sfz_struct(ZuiFontInfo) {
	sfz::Array<u8> ttfData;
	f32 atlasSize;
	u8 asciiPackRaw[stbtt_packedchar_size * ZUI_NUM_PRINTABLE_ASCII_CHARS];
	u8 extraPackRaw[stbtt_packedchar_size * ZUI_NUM_EXTRA_CHARS];
};

sfz_struct(ZuiDrawCtx) {
	// Render data
	sfz::Array<ZuiVertex> vertices;
	sfz::Array<uint16_t> indices;
	sfz::Array<ZuiRenderCmd> renderCmds;
	
	// Font image
	i32 fontImgRes;
	sfz::Array<u8> fontImg;
	bool fontImgModified;
	u64 userFontTexHandle;

	// Fonts
	sfz::HashMap<u64, ZuiFontInfo> fonts;
	u8 packCtxRaw[stbtt_pack_context_size];
	stbtt_pack_context* packCtx() { return reinterpret_cast<stbtt_pack_context*>(packCtxRaw); }

	// Whether to vertically flip images or not
	bool imgFlipY = true;
};

// Context
// ------------------------------------------------------------------------------------------------

sfz_constant ZuiID ZUI_DEFAULT_ID = zuiName("default");

struct ZuiWidgetArchetype final {
	ZuiDrawFunc* drawFunc = nullptr;
};

struct ZuiWidgetType {
	u32 widgetDataSizeBytes = 0;
	ZuiGetNextWidgetBoxFunc* getNextWidgetBoxFunc = nullptr;
	ZuiHandlePointerInputFunc* handlePointerInputFunc = nullptr;
	ZuiHandleMoveInputFunc* handleMoveInputFunc = nullptr;
	sfz::Map32<u64, ZuiWidgetArchetype> archetypes;
	sfz::Array<ZuiID> archetypeStack;
	ZuiDrawFunc* getCurrentArchetypeDrawFunc() const
	{
		sfz_assert(!archetypeStack.isEmpty());
		const ZuiWidgetArchetype* archetype = archetypes.get(archetypeStack.last().id);
		sfz_assert(archetype != nullptr);
		return archetype->drawFunc;
	}
};

struct ZuiWidget final {
	ZuiID id;
	ZuiID widgetTypeID;
	void* dataPtr = nullptr;
	ZuiDrawFunc* archetypeDrawFunc = nullptr;
	sfz::Array<ZuiWidget> children;

	ZuiWidget() = default;
	ZuiWidget(ZuiCtx* zui, ZuiID id, void* ptr);

	template<typename T> T* data() { return (T*)dataPtr; }
	template<typename T> const T* data() const { return (const T*)dataPtr; }
	u32 sizeOfWidgetData(ZuiCtx* zui) const;
	void getNextWidgetBox(ZuiCtx* zui, ZuiID childWidgetTypeID, ZuiBox* boxOut);
	void handlePointerInput(ZuiCtx* zui, f32x2 pointerPosSS);
	void handleMoveInput(ZuiCtx* zui, ZuiInput* input, bool* moveActive);
	void draw(ZuiCtx* zui, const SfzMat34* surfaceTransform, f32 lagSinceSurfaceEndSecs) const;
};

inline ZuiWidget* zuiFindWidgetFromID(ZuiWidget* widget, ZuiID id)
{
	if (widget->id == id) return widget;
	for (ZuiWidget& child : widget->children) {
		ZuiWidget* res = zuiFindWidgetFromID(&child, id);
		if (res != nullptr) return res;
	}
	return nullptr;
}

struct ZuiWidgetTree final {
	sfz::ArenaHeap arena;
	ZuiWidget root;
	sfz::Array<ZuiWidget*> parentStack;

	void init(u32 surfaceTmpMemoryBytes, SfzAllocator* allocator)
	{
		arena.init(allocator, surfaceTmpMemoryBytes, sfz_dbg(""));
		this->clear();
	}

	void clear()
	{
		root.children.destroy();
		parentStack.destroy();
		arena.resetArena();
		parentStack.init(64, arena.getArena(), sfz_dbg(""));
	}

	ZuiWidget& getCurrentParent()
	{
		sfz_assert(!parentStack.isEmpty());
		return *parentStack.last();
	}
	void pushMakeParent(ZuiWidget* widget, u32 numChildrenHint = 64)
	{
		sfz_assert(widget->children.allocator() == nullptr);
		sfz_assert(widget->children.isEmpty());
		widget->children.init(numChildrenHint, arena.getArena(), sfz_dbg(""));
		parentStack.add(widget);
	}
	void popParent()
	{
		parentStack.pop();
		sfz_assert(!parentStack.isEmpty());
	}
};

using AttributeSet = sfz::HashMap<u64, ZuiAttrib>;

struct ZuiCtx final {

	SfzAllocator* heapAllocator = nullptr;

	ZuiDrawCtx drawCtx;

	// Widget types
	sfz::HashMap<u64, ZuiWidgetType> widgetTypes;

	// Widget trees
	u64 inputIdx; // Incremented each zuiInputBegin()
	ZuiWidgetTree widgetTrees[2];
	ZuiWidgetTree& currTree() { return widgetTrees[inputIdx % 2]; }
	ZuiWidgetTree& prevTree() { return widgetTrees[(inputIdx + 1) % 2]; }

	ZuiInput input = {};

	// Transforms
	SfzMat34 transform = sfz::mat34::identity();
	SfzMat34 inputTransform = sfz::mat34::identity();
	f32x2 pointerPosSS = f32x2(-F32_MAX); // SS = Surface Space

	// AttributeSet used when rendering
	AttributeSet attribs;
	AttributeSet defaultAttribs;

	template<typename T>
	T* createWidget(ZuiID id, ZuiID widgetTypeID, ZuiWidget** widgetOut = nullptr)
	{
		// Get widget type
		const ZuiWidgetType* type = widgetTypes.get(widgetTypeID.id);
		sfz_assert(type != nullptr);

		// Check if it already exists in prev tree
		ZuiWidgetTree& prevTree = this->prevTree();
		ZuiWidget* prevWidget = zuiFindWidgetFromID(&prevTree.root, id);

		// Create in curr tree and set members
		ZuiWidgetTree& tree = this->currTree();
		SfzAllocator* allocator = tree.arena.getArena();
		ZuiWidget& parent =  tree.getCurrentParent();
		ZuiWidget& widget = parent.children.add();
		widget.id = id;
		widget.widgetTypeID = widgetTypeID;
		widget.dataPtr = allocator->alloc(sfz_dbg(""), type->widgetDataSizeBytes);
		sfz_assert(type->widgetDataSizeBytes == sizeof(T));
		if (prevWidget != nullptr) {
			sfz_assert(prevWidget->id == id);
			sfz_assert(prevWidget->widgetTypeID == widgetTypeID);
			sfz_assert(prevWidget->dataPtr != nullptr);
			memcpy(widget.dataPtr, prevWidget->dataPtr, type->widgetDataSizeBytes);
		}
		else {
			new (widget.dataPtr) T();
			*widget.data<T>() = {};
		}
		ZuiWidgetBase& base = widget.data<T>()->base;
		parent.getNextWidgetBox(this, widgetTypeID, &base.box);
		widget.archetypeDrawFunc = type->getCurrentArchetypeDrawFunc();
		
		// Update timers
		base.timeSinceFocusStartedSecs += input.deltaTimeSecs;
		base.timeSinceFocusEndedSecs += input.deltaTimeSecs;
		base.timeSinceActivationSecs += input.deltaTimeSecs;

		// Return command if requested
		if (widgetOut != nullptr) {
			*widgetOut = &widget;
		}

		// Return widget data
		return widget.data<T>();
	}

	template<typename T>
	T* createWidgetParent(ZuiID id, ZuiID widgetTypeID, u32 numChildrenHint = 64)
	{
		ZuiWidget* widget = nullptr;
		T* data = this->createWidget<T>(id, widgetTypeID, &widget);
		currTree().pushMakeParent(widget, numChildrenHint);
		return data;
	}

	// Default function for popping parent widgets unless you need to do something special
	void popWidgetParent(ZuiID widgetTypeID)
	{
		ZuiWidgetTree& tree = this->currTree();
		ZuiWidget& parent = tree.getCurrentParent();
		sfz_assert(parent.widgetTypeID == widgetTypeID);
		sfz_assert(tree.parentStack.size() > 1); // Don't remove default base container
		tree.popParent();
	}
};

inline ZuiWidget::ZuiWidget(ZuiCtx* zui, ZuiID id, void* ptr)
{
	widgetTypeID = id;
	dataPtr = ptr;
	archetypeDrawFunc = zui->widgetTypes.get(id.id)->getCurrentArchetypeDrawFunc();
	sfz_assert(archetypeDrawFunc != nullptr);
}

inline u32 ZuiWidget::sizeOfWidgetData(ZuiCtx* zui) const
{
	ZuiWidgetType* type = zui->widgetTypes.get(widgetTypeID.id);
	sfz_assert(type != nullptr);
	return type->widgetDataSizeBytes;
}

inline void ZuiWidget::getNextWidgetBox(ZuiCtx* zui, ZuiID childWidgetTypeID, ZuiBox* boxOut)
{
	ZuiWidgetType* type = zui->widgetTypes.get(widgetTypeID.id);
	sfz_assert(type != nullptr);
	sfz_assert(type->getNextWidgetBoxFunc != nullptr);
	type->getNextWidgetBoxFunc(zui, this, childWidgetTypeID, boxOut);
}

inline void ZuiWidget::handlePointerInput(ZuiCtx* zui, f32x2 pointerPosSS)
{
	ZuiWidgetType* type = zui->widgetTypes.get(widgetTypeID.id);
	sfz_assert(type != nullptr);
	sfz_assert(type->handlePointerInputFunc != nullptr);
	type->handlePointerInputFunc(zui, this, pointerPosSS);
}

inline void ZuiWidget::handleMoveInput(ZuiCtx* zui, ZuiInput* input, bool* moveActive)
{
	ZuiWidgetType* type = zui->widgetTypes.get(widgetTypeID.id);
	sfz_assert(type != nullptr);
	sfz_assert(type->handleMoveInputFunc != nullptr);
	type->handleMoveInputFunc(zui, this, input, moveActive);
}

inline void ZuiWidget::draw(ZuiCtx* zui, const SfzMat34* surfaceTransform, f32 lagSinceInputEndSecs) const
{
	archetypeDrawFunc(zui, this, surfaceTransform, lagSinceInputEndSecs);
}
