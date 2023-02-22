// Copyright (c) 2020-2023 Peter Hillerström (skipifzero.com, peter@hstroem.se)
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
	SfzArray<u8> ttf_data;
	f32 atlas_size;
	u8 ascii_pack_raw[stbtt_packedchar_size * ZUI_NUM_PRINTABLE_ASCII_CHARS];
	u8 extra_pack_raw[stbtt_packedchar_size * ZUI_NUM_EXTRA_CHARS];
};

sfz_struct(ZuiDrawCtx) {
	// Render data
	SfzArray<ZuiVertex> vertices;
	SfzArray<u16> indices;
	SfzArray<SfzMat44> transforms;
	SfzArray<ZuiRenderCmd> render_cmds;

	// Clip stack
	SfzArray<ZuiBox> clip_stack;

	// Font image
	i32 font_img_res;
	SfzArray<u8> font_img;
	bool font_img_modified;
	u64 user_font_tex_handle;

	// Fonts
	SfzHashMap<u64, ZuiFontInfo> fonts;
	u8 pack_ctx_raw[stbtt_pack_context_size];
	stbtt_pack_context* packCtx() { return reinterpret_cast<stbtt_pack_context*>(pack_ctx_raw); }

	// Whether to vertically flip images or not
	bool img_flip_y = true;
};

// Widget
// ------------------------------------------------------------------------------------------------

// For containers only: Return the position and size of the next widget being placed. Widgets don't
// determine their own size, they must always ask their parent what their size is.
typedef void ZuiGetNextWidgetBoxFunc(ZuiWidget* widget, ZuiBox* box_out);

typedef i32 ZuiWidgetGetNextChildIdx(const ZuiWidget* w, ZuiInputAction action, i32 curr_idx);

typedef void ZuiWidgetScrollInput(ZuiWidget* w, f32x2 scroll);
typedef void ZuiWidgetScrollSetChildVisible(ZuiWidget* w, i32 child_idx);

// The description of a widget with all the necessary functions.
sfz_struct(ZuiWidgetDesc) {
	u32 widget_data_size_bytes;
	bool focuseable;
	bool activateable;
	bool skip_clipping;
	ZuiGetNextWidgetBoxFunc* get_next_widget_box_func;
	ZuiWidgetGetNextChildIdx* get_next_child_idx_func;
	ZuiWidgetScrollInput* scroll_input_func;
	ZuiWidgetScrollSetChildVisible* scroll_set_child_visible_func;
	ZuiDrawFunc* draw_func;
};

sfz_struct(ZuiWidgetType) {
	SfzStr32 name;
	ZuiWidgetDesc desc = {};
	SfzArray<ZuiDrawFunc*> draw_func_stack;
	ZuiDrawFunc* getCurrentDrawFunc() const
	{
		sfz_assert(!draw_func_stack.isEmpty());
		ZuiDrawFunc* draw_func = draw_func_stack.last();
		return draw_func;
	}
};

sfz_struct(ZuiWidgetBase) {
	ZuiBox box; // The location and size of the widget on the surface
	f32 time_since_focus_started_secs = F32_MAX;
	f32 time_since_focus_ended_secs = F32_MAX;
	f32 time_since_activation_secs = F32_MAX;
	bool focused = false;
	bool activated = false;

	void setFocused()
	{
		if (!focused) {
			time_since_focus_started_secs = 0.0f;
		}
		focused = true;
	}
	void setUnfocused()
	{
		if (focused) {
			time_since_focus_ended_secs = 0.0f;
		}
		focused = false;
	}
	void setActivated()
	{
		time_since_activation_secs = 0.0f;
		activated = true;
	}
};

struct ZuiWidget final {
	ZuiID id;
	ZuiID widget_type_id;
	ZuiWidgetBase base;
	void* data_ptr = nullptr;
	ZuiDrawFunc* draw_func = nullptr;
	SfzArray<ZuiWidget> children;
	SfzHashMap<u64, ZuiAttrib> attribs;

	template<typename T> T* data() { return (T*)data_ptr; }
	template<typename T> const T* data() const { return (const T*)data_ptr; }
};

// Context
// ------------------------------------------------------------------------------------------------

struct ZuiWidgetTree final {
	sfz::ArenaHeap arena;
	ZuiWidget root;
	SfzArray<ZuiWidget*> parent_stack;
};

struct ZuiCtx final {
	SfzAllocator* heap_allocator = nullptr;

	ZuiDrawCtx draw_ctx;

	// Widget types
	SfzHashMap<u64, ZuiWidgetType> widget_types;

	// Widget trees
	u64 input_idx; // Incremented each zuiInputBegin()
	ZuiWidgetTree widget_trees[2];
	ZuiWidgetTree& currTree() { return widget_trees[input_idx % 2]; }
	ZuiWidgetTree& prevTree() { return widget_trees[(input_idx + 1) % 2]; }

	ZuiInput input = {};

	// Transforms
	SfzMat44 surf_to_fb = sfzMat44Identity();
	SfzMat44 surf_to_clip = sfzMat44Identity();
	SfzMat44 fb_to_surf = sfzMat44Identity();
	f32x2 pointer_pos_ss = f32x2_splat(-F32_MAX); // SS = Surface Space

	// Input lock stack
	SfzArray<ZuiID> input_locks;

	// AttributeSet used when rendering
	SfzHashMap<u64, ZuiAttrib> attribs;
	SfzHashMap<u64, ZuiAttrib> default_attribs;
};

// Box functions
// ------------------------------------------------------------------------------------------------

sfz_constexpr_func ZuiBox zuiBoxInit(f32x2 center, f32x2 dims)
{
	return ZuiBox{ center - dims * 0.5f, center + dims * 0.5f };
}

// Default widget implementation functions
// ------------------------------------------------------------------------------------------------

inline i32 zuiWidgetGetNextChildIdxDefault(const ZuiWidget* w, ZuiInputAction action, i32 curr_idx)
{
	i32 next_idx = -1;
	if (action == ZUI_INPUT_DOWN) {
		next_idx = curr_idx + 1;
	}
	else if (action == ZUI_INPUT_UP) {
		if (curr_idx == -1) curr_idx = i32(w->children.size());
		next_idx = curr_idx - 1;
	}
	return next_idx;
}

// Widget tree functions
// ------------------------------------------------------------------------------------------------

inline void zuiWidgetTreeClear(ZuiWidgetTree* tree)
{
	tree->root.children.destroy();
	tree->root.attribs.destroy();
	tree->parent_stack.destroy();
	tree->arena.resetArena();
	tree->parent_stack.init(64, tree->arena.getArena(), sfz_dbg(""));
}

inline ZuiWidget& zuiWidgetTreeGetCurrentParent(ZuiWidgetTree* tree)
{
	sfz_assert(!tree->parent_stack.isEmpty());
	return *tree->parent_stack.last();
}

inline void zuiWidgetTreePushMakeParent(ZuiWidgetTree* tree, ZuiWidget* widget, u32 num_children_hint = 64)
{
	sfz_assert(widget->children.allocator() == nullptr);
	sfz_assert(widget->children.isEmpty());
	widget->children.init(num_children_hint, tree->arena.getArena(), sfz_dbg(""));
	tree->parent_stack.add(widget);
}

inline void zuiWidgetTreePopParent(ZuiWidgetTree* tree, ZuiID parent_type_id)
{
	sfz_assert(!tree->parent_stack.isEmpty());
	sfz_assert(tree->parent_stack.last()->widget_type_id == parent_type_id);
	tree->parent_stack.pop();
	sfz_assert(!tree->parent_stack.isEmpty());
}

// Context functions
// ------------------------------------------------------------------------------------------------

inline void zuiCtxRegisterWidget(ZuiCtx* zui, const char* name, const ZuiWidgetDesc* desc)
{
	const ZuiID nameID = zuiName(name);
	sfz_assert(zui->widget_types.get(nameID.id) == nullptr);
	ZuiWidgetType& type = zui->widget_types.put(nameID.id, {});
	type.name = sfzStr32Init(name);
	type.desc = *desc;
	type.draw_func_stack.init(64, zui->heap_allocator, sfz_dbg(""));
}

// Yeah, this one is a hack and pretty bad. It basically makes Widget creation go from O(1) to O(N),
// which means that we have quadratic scaling for inputs. The easy solution would be to just add
// an additional HashMap<ID, Widget>. The main reason we haven't done so is because we would need
// a strategy to prove it's safe in the case that a parent widget's children array resizes (and
// thus invalidates all old Widget pointers).
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
ZuiWidget* zuiCtxCreateWidget(ZuiCtx* zui, ZuiID id, ZuiID widget_type_id, bool* was_created = nullptr)
{
	// Get widget type
	const ZuiWidgetType* type = zui->widget_types.get(widget_type_id.id);
	sfz_assert(type != nullptr);

	// Check if it already exists in prev tree
	ZuiWidgetTree& prev_tree = zui->prevTree();
	ZuiWidget* prev_widget = zuiFindWidgetFromID(&prev_tree.root, id);

	// Get parent and type
	ZuiWidgetTree& tree = zui->currTree();
	ZuiWidget& parent = zuiWidgetTreeGetCurrentParent(&tree);
	const ZuiWidgetType* parent_type = zui->widget_types.get(parent.widget_type_id.id);
	sfz_assert(parent_type != nullptr);

	// Create in curr tree and set members
	SfzAllocator* allocator = tree.arena.getArena();
	ZuiWidget& widget = parent.children.add();
	widget.id = id;
	widget.widget_type_id = widget_type_id;
	widget.data_ptr = allocator->alloc(sfz_dbg(""), type->desc.widget_data_size_bytes);
	sfz_assert(type->desc.widget_data_size_bytes == sizeof(T));
	if (prev_widget != nullptr) {
		sfz_assert(prev_widget->id == id);
		sfz_assert(prev_widget->widget_type_id == widget_type_id);
		sfz_assert(prev_widget->data_ptr != nullptr);
		memcpy(widget.data_ptr, prev_widget->data_ptr, type->desc.widget_data_size_bytes);
		memcpy(&widget.base, &prev_widget->base, sizeof(ZuiWidgetBase));
		if (was_created != nullptr) *was_created = false;
	}
	else {
		new (widget.data_ptr) T();
		*widget.data<T>() = {};
		widget.base = {};
		if (was_created != nullptr) *was_created = true;
	}
	// Use parent's getNextWidgetBox function if it has one, otherwise just copy parent's box.
	if (parent_type->desc.get_next_widget_box_func != nullptr) {
		parent_type->desc.get_next_widget_box_func(&parent, &widget.base.box);
	}
	else {
		widget.base.box = parent.base.box;
	}

	widget.draw_func = type->getCurrentDrawFunc();

	// Update timers
	widget.base.time_since_focus_started_secs += zui->input.delta_time_secs;
	widget.base.time_since_focus_ended_secs += zui->input.delta_time_secs;
	widget.base.time_since_activation_secs += zui->input.delta_time_secs;

	// Return widget data
	return &widget;
}

template<typename T>
ZuiWidget* zuiCtxCreateWidgetParent(ZuiCtx* zui, ZuiID id, ZuiID widget_type_id, bool* was_created = nullptr, u32 num_children_hint = 64)
{
	ZuiWidget* widget = zuiCtxCreateWidget<T>(zui, id, widget_type_id, was_created);
	zuiWidgetTreePushMakeParent(&zui->currTree(), widget, num_children_hint);
	return widget;
}

// Default function for popping parent widgets unless you need to do something special
inline void zuiCtxPopWidgetParent(ZuiCtx* zui, ZuiID widget_type_id)
{
	ZuiWidgetTree& tree = zui->currTree();
	ZuiWidget& parent = zuiWidgetTreeGetCurrentParent(&tree);
	sfz_assert(parent.widget_type_id == widget_type_id);
	sfz_assert(tree.parent_stack.size() > 1); // Don't remove default base container
	zuiWidgetTreePopParent(&tree, widget_type_id);
}

inline bool zuiCtxIsInputLocked(const ZuiCtx* zui, ZuiID widget_id)
{
	return zui->input_locks.findElement(widget_id) != nullptr;
}

inline void zuiCtxSetInputLock(ZuiCtx* zui, ZuiID widget_id)
{
	if (zuiCtxIsInputLocked(zui, widget_id)) return;
	zui->input_locks.add(widget_id);
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
