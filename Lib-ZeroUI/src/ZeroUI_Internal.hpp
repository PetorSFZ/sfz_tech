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

#include "ZeroUI.h"

#include <sfz_math.h>
#include <sfz_matrix.h>
#include <skipifzero_allocators.hpp>
#include <skipifzero_arrays.hpp>
#include <skipifzero_hash_maps.hpp>
#include <skipifzero_strings.hpp>

// About
// ------------------------------------------------------------------------------------------------

// This is an internal header containing the private internal API of ZeroUI. This includes internal
// data structures and functions.
//
// You should normally NOT include this if you are only using ZeroUI. Rather, this header is meant
// for people who are extending ZeroUI and creating their own widgets.

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
	SfzArray<u8> ttfData;
	f32 atlasSize;
	u8 asciiPackRaw[stbtt_packedchar_size * ZUI_NUM_PRINTABLE_ASCII_CHARS];
	u8 extraPackRaw[stbtt_packedchar_size * ZUI_NUM_EXTRA_CHARS];
};

sfz_struct(ZuiDrawCtx) {
	// Render data
	SfzArray<ZuiVertex> vertices;
	SfzArray<u16> indices;
	SfzArray<SfzMat44> transforms;
	SfzArray<ZuiRenderCmd> renderCmds;
	
	// Clip stack
	SfzArray<ZuiBox> clipStack;

	// Font image
	i32 fontImgRes;
	SfzArray<u8> fontImg;
	bool fontImgModified;
	u64 userFontTexHandle;

	// Fonts
	SfzHashMap<u64, ZuiFontInfo> fonts;
	u8 packCtxRaw[stbtt_pack_context_size];
	stbtt_pack_context* packCtx() { return reinterpret_cast<stbtt_pack_context*>(packCtxRaw); }

	// Whether to vertically flip images or not
	bool imgFlipY = true;
};

// Widget
// ------------------------------------------------------------------------------------------------

sfz_struct(ZuiWidgetBase) {
	ZuiBox box; // The location and size of the widget on the surface
	f32 timeSinceFocusStartedSecs = F32_MAX;
	f32 timeSinceFocusEndedSecs = F32_MAX;
	f32 timeSinceActivationSecs = F32_MAX;
	bool focused = false;
	bool activated = false;

	void setFocused()
	{
		if (!focused) {
			timeSinceFocusStartedSecs = 0.0f;
		}
		focused = true;
	}
	void setUnfocused()
	{
		if (focused) {
			timeSinceFocusEndedSecs = 0.0f;
		}
		focused = false;
	}
	void setActivated()
	{
		timeSinceActivationSecs = 0.0f;
		activated = true;
	}
};

struct ZuiWidget;

// For containers only: Return the position and size of the next widget being placed. Widgets don't
// determine their own size, they must always ask their parent what their size is.
typedef void ZuiGetNextWidgetBoxFunc(ZuiCtx* zui, ZuiWidget* widget, ZuiID childWidgetTypeID, ZuiBox* boxOut);

typedef i32 ZuiWidgetGetNextChildIdx(const ZuiWidget* w, ZuiInputAction action, i32 currIdx);

typedef void ZuiWidgetScrollInput(ZuiWidget* w, f32x2 scroll);

// The description of a widget with all the necessary functions.
sfz_struct(ZuiWidgetDesc) {
	u32 widgetDataSizeBytes;
	bool focuseable;
	bool activateable;
	ZuiGetNextWidgetBoxFunc* getNextWidgetBoxFunc;
	ZuiWidgetGetNextChildIdx* getNextChildIdxFunc;
	ZuiWidgetScrollInput* scrollInputFunc;
	ZuiDrawFunc* drawFunc;
};

// Context
// ------------------------------------------------------------------------------------------------

sfz_constant ZuiID ZUI_DEFAULT_ID = zuiName("default");

struct ZuiWidgetArchetype final {
	ZuiDrawFunc* drawFunc = nullptr;
};

struct ZuiWidgetType {
	u32 widgetDataSizeBytes = 0;
	bool focuseable = false;
	bool activateable = false;
	ZuiGetNextWidgetBoxFunc* getNextWidgetBoxFunc = nullptr;
	ZuiWidgetGetNextChildIdx* getNextChildIdxFunc = nullptr;
	ZuiWidgetScrollInput* scrollInputFunc = nullptr;
	SfzMap32<u64, ZuiWidgetArchetype> archetypes;
	SfzArray<ZuiID> archetypeStack;
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
	ZuiWidgetBase base;
	void* dataPtr = nullptr;
	ZuiDrawFunc* archetypeDrawFunc = nullptr;
	SfzArray<ZuiWidget> children;

	template<typename T> T* data() { return (T*)dataPtr; }
	template<typename T> const T* data() const { return (const T*)dataPtr; }
};

struct ZuiWidgetTree final {
	sfz::ArenaHeap arena;
	ZuiWidget root;
	SfzArray<ZuiWidget*> parentStack;
};

using AttributeSet = SfzHashMap<u64, ZuiAttrib>;

struct ZuiCtx final {
	SfzAllocator* heapAllocator = nullptr;

	ZuiDrawCtx drawCtx;

	// Widget types
	SfzHashMap<u64, ZuiWidgetType> widgetTypes;

	// Widget trees
	u64 inputIdx; // Incremented each zuiInputBegin()
	ZuiWidgetTree widgetTrees[2];
	ZuiWidgetTree& currTree() { return widgetTrees[inputIdx % 2]; }
	ZuiWidgetTree& prevTree() { return widgetTrees[(inputIdx + 1) % 2]; }

	ZuiInput input = {};

	// Transforms
	SfzMat44 surfToFB = sfzMat44Identity();
	SfzMat44 transform = sfzMat44Identity();
	SfzMat44 inputTransform = sfzMat44Identity();
	f32x2 pointerPosSS = f32x2_splat(-F32_MAX); // SS = Surface Space

	// AttributeSet used when rendering
	AttributeSet attribs;
	AttributeSet defaultAttribs;
};

// Box functions
// ------------------------------------------------------------------------------------------------

sfz_constexpr_func ZuiBox zuiBoxInit(f32x2 center, f32x2 dims)
{
	return ZuiBox{ center - dims * 0.5f, center + dims * 0.5f };
}

// Default widget implementation functions
// ------------------------------------------------------------------------------------------------

inline i32 zuiWidgetGetNextChildIdxDefault(const ZuiWidget* w, ZuiInputAction action, i32 currIdx)
{
	i32 nextIdx = -1;
	if (action == ZUI_INPUT_DOWN) {
		nextIdx = currIdx + 1;
	}
	else if (action == ZUI_INPUT_UP) {
		if (currIdx == -1) currIdx = i32(w->children.size());
		nextIdx = currIdx - 1;
	}
	return nextIdx;
}

// Widget functions
// ------------------------------------------------------------------------------------------------

inline void zuiWidgetDraw(const ZuiWidget* w, ZuiCtx* zui, const SfzMat44* surfaceTransform, f32 lagSinceInputEndSecs)
{
	w->archetypeDrawFunc(zui, w, surfaceTransform, lagSinceInputEndSecs);
}

// Widget tree functions
// ------------------------------------------------------------------------------------------------

inline void zuiWidgetTreeClear(ZuiWidgetTree* tree)
{
	tree->root.children.destroy();
	tree->parentStack.destroy();
	tree->arena.resetArena();
	tree->parentStack.init(64, tree->arena.getArena(), sfz_dbg(""));
}

inline void zuiWidgetTreeInit(ZuiWidgetTree* tree, u32 surfaceTmpMemoryBytes, SfzAllocator* allocator)
{
	tree->arena.init(allocator, surfaceTmpMemoryBytes, sfz_dbg(""));
	zuiWidgetTreeClear(tree);
}

inline ZuiWidget& zuiWidgetTreeGetCurrentParent(ZuiWidgetTree* tree)
{
	sfz_assert(!tree->parentStack.isEmpty());
	return *tree->parentStack.last();
}

inline void zuiWidgetTreePushMakeParent(ZuiWidgetTree* tree, ZuiWidget* widget, u32 numChildrenHint = 64)
{
	sfz_assert(widget->children.allocator() == nullptr);
	sfz_assert(widget->children.isEmpty());
	widget->children.init(numChildrenHint, tree->arena.getArena(), sfz_dbg(""));
	tree->parentStack.add(widget);
}

inline void zuiWidgetTreePopParent(ZuiWidgetTree* tree)
{
	tree->parentStack.pop();
	sfz_assert(!tree->parentStack.isEmpty());
}

// Context functions
// ------------------------------------------------------------------------------------------------

inline void zuiCtxRegisterWidget(ZuiCtx* zui, const char* name, const ZuiWidgetDesc* desc)
{
	const ZuiID nameID = zuiName(name);
	sfz_assert(zui->widgetTypes.get(nameID.id) == nullptr);
	ZuiWidgetType& type = zui->widgetTypes.put(nameID.id, {});
	type.widgetDataSizeBytes = desc->widgetDataSizeBytes;
	type.focuseable = desc->focuseable;
	type.activateable = desc->activateable;
	type.getNextWidgetBoxFunc = desc->getNextWidgetBoxFunc;
	type.getNextChildIdxFunc = desc->getNextChildIdxFunc;
	type.scrollInputFunc = desc->scrollInputFunc;
	ZuiWidgetArchetype& archetype = type.archetypes.put(ZUI_DEFAULT_ID.id, {});
	archetype.drawFunc = desc->drawFunc;
	type.archetypeStack.init(64, zui->heapAllocator, sfz_dbg(""));
}

inline ZuiWidget* zuiFindWidgetFromID(ZuiWidget* widget, ZuiID id)
{
	if (widget->id == id) return widget;
	for (ZuiWidget& child : widget->children) {
		ZuiWidget* res = zuiFindWidgetFromID(&child, id);
		if (res != nullptr) return res;
	}
	return nullptr;
}

template<typename T>
ZuiWidget* zuiCtxCreateWidget(ZuiCtx* zui, ZuiID id, ZuiID widgetTypeID, bool* wasCreated = nullptr)
{
	// Get widget type
	const ZuiWidgetType* type = zui->widgetTypes.get(widgetTypeID.id);
	sfz_assert(type != nullptr);

	// Check if it already exists in prev tree
	ZuiWidgetTree& prevTree = zui->prevTree();
	ZuiWidget* prevWidget = zuiFindWidgetFromID(&prevTree.root, id);
	
	// Get parent and type
	ZuiWidgetTree& tree = zui->currTree();
	ZuiWidget& parent = zuiWidgetTreeGetCurrentParent(&tree);
	const ZuiWidgetType* parentType = zui->widgetTypes.get(parent.widgetTypeID.id);
	sfz_assert(parentType != nullptr);

	// Create in curr tree and set members
	SfzAllocator* allocator = tree.arena.getArena();
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
		memcpy(&widget.base, &prevWidget->base, sizeof(ZuiWidgetBase));
		if (wasCreated != nullptr) *wasCreated = false;
	}
	else {
		new (widget.dataPtr) T();
		*widget.data<T>() = {};
		widget.base = {};
		if (wasCreated != nullptr) *wasCreated = true;
	}
	sfz_assert(parentType->getNextWidgetBoxFunc != nullptr);
	parentType->getNextWidgetBoxFunc(zui, &parent, widgetTypeID, &widget.base.box);
	widget.archetypeDrawFunc = type->getCurrentArchetypeDrawFunc();

	// Update timers
	widget.base.timeSinceFocusStartedSecs += zui->input.deltaTimeSecs;
	widget.base.timeSinceFocusEndedSecs += zui->input.deltaTimeSecs;
	widget.base.timeSinceActivationSecs += zui->input.deltaTimeSecs;

	// Return widget data
	return &widget;
}

template<typename T>
ZuiWidget* zuiCtxCreateWidgetParent(ZuiCtx* zui, ZuiID id, ZuiID widgetTypeID, bool* wasCreated = nullptr, u32 numChildrenHint = 64)
{
	ZuiWidget* widget = zuiCtxCreateWidget<T>(zui, id, widgetTypeID, wasCreated);
	zuiWidgetTreePushMakeParent(&zui->currTree(), widget, numChildrenHint);
	return widget;
}

// Default function for popping parent widgets unless you need to do something special
inline void zuiCtxPopWidgetParent(ZuiCtx* zui, ZuiID widgetTypeID)
{
	ZuiWidgetTree& tree = zui->currTree();
	ZuiWidget& parent = zuiWidgetTreeGetCurrentParent(&tree);
	sfz_assert(parent.widgetTypeID == widgetTypeID);
	sfz_assert(tree.parentStack.size() > 1); // Don't remove default base container
	zuiWidgetTreePopParent(&tree);
}

// Helper functions
// ------------------------------------------------------------------------------------------------

inline f32x2 zuiCalcCenterPos(f32x2 pos, ZuiAlign align, f32x2 dims)
{
	f32x2 offset = f32x2_splat(0.0f);
	switch (align) {
	case ZUI_BOTTOM_LEFT:
		offset = f32x2_init(-1.0f, -1.0f);
		break;
	case ZUI_BOTTOM_CENTER:
		offset = f32x2_init(0.0f, -1.0f);
		break;
	case ZUI_BOTTOM_RIGHT:
		offset = f32x2_init(1.0f, -1.0f);
		break;
	case ZUI_MID_LEFT:
		offset = f32x2_init(-1.0f, 0.0f);
		break;
	case ZUI_MID_CENTER:
		offset = f32x2_init(0.0f, 0.0f);
		break;
	case ZUI_MID_RIGHT:
		offset = f32x2_init(1.0f, 0.0f);
		break;
	case ZUI_TOP_LEFT:
		offset = f32x2_init(-1.0f, 1.0f);
		break;
	case ZUI_TOP_CENTER:
		offset = f32x2_init(0.0f, 1.0f);
		break;
	case ZUI_TOP_RIGHT:
		offset = f32x2_init(1.0f, 1.0f);
		break;
	default: sfz_assert_hard(false);
	}
	return pos - offset * 0.5f * dims;
}

// Default draw function which is just a pass through, only useful for containers.
inline void zuiDefaultPassthroughDrawFunc(
	ZuiCtx* zui,
	const ZuiWidget* widget,
	const SfzMat44* transform,
	f32 lagSinceInputEndSecs)
{
	zui->drawCtx.clipStack.add(widget->base.box);
	for (const ZuiWidget& child : widget->children) {
		zuiWidgetDraw(&child, zui, transform, lagSinceInputEndSecs);
	}
	zui->drawCtx.clipStack.pop();
}
