// Copyright (c) 2020-2022 Peter Hillerström (skipifzero.com, peter@hstroem.se)
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
#ifndef ZEROUI_H
#define ZEROUI_H

#include <sfz.h>
#include <sfz_image_view.h>

// About
// ------------------------------------------------------------------------------------------------

// ZeroUI - An immediate mode game UI library
//
// This is an immediate mode UI library which fills a specific niche, in-game (i.e. not tool) UI
// primarily targeting gamepads.
//
// Design goals:
//  * Primarily gamepad as input, but compatible with mouse/touch
//  * Immediate-mode API's designed similarly to dear-imgui and such where it makes sense
//  * Easy to create custom rendering functions for widgets
//  * Easy to extend with custom widgets
//  * API/engine independent, output is vertices to stream to GPU
//  * KISS - Keep It Simple Stupid
//  * Clean C-API, no exceptions, no RTTI, no STL, no object-oriented programming.

// Context
// ------------------------------------------------------------------------------------------------

struct ZuiCtx;

sfz_struct(ZuiCfg) {
	u32 arena_memory_limit_bytes; // Amount of bytes per internal arena heap
	u32 oversample_fonts; // 1 by default
};

sfz_extern_c ZuiCtx* zuiCtxInit(ZuiCfg* cfg, SfzAllocator* allocator);
sfz_extern_c void zuiCtxDestroy(ZuiCtx* zui);

// IDs
// ------------------------------------------------------------------------------------------------

// ZeroUI, like most (all?) immediate mode UIs need unique identifiers in order to correlate state
// over time. In e.g. Dear ImGui this is done by the user inputing unique strings for each call.
// A weakness of Dear ImGui's approach is that the user has to concatenate strings (or similar)
// when adding items inside a loop.
//
// ZeroUI's approach is more explicit, all IDs are 64-bit unsigned integers. You are responsible
// for coming up with unique ints in whatever way you see fit. ZeroUI has a few helpers below to
// make it easier.

sfz_struct(ZuiID) {
	u64 id;
#ifdef __cplusplus
	constexpr bool operator== (ZuiID o) const { return this->id == o.id; }
	constexpr bool operator!= (ZuiID o) const { return this->id != o.id; }
#endif
};

// Creates a ZeroUI ID by hashing a string. Uses FNV-1a, see http://isthe.com/chongo/tech/comp/fnv/
sfz_constexpr_func ZuiID zuiName(const char* str)
{
	if (str == nullptr) return ZuiID{ 0 };
	u64 tmp = u64(0xCBF29CE484222325); // Initial value FNV-0 hash of "chongo <Landon Curt Noll> /\../\"
	while (char c = *str++) {
		tmp ^= u64(c); // Xor bottom with current byte
		tmp *= u64(0x100000001B3); // Multiply with FNV magic prime
	}
	return ZuiID{ tmp };
}

// Combines two ZeroUI ID's into one. Uses the hash_combine algorithm from boost.
sfz_constexpr_func ZuiID zuiIdCombine(ZuiID id1, ZuiID id2)
{
	u64 h = id1.id + 0x9e3779b9;
	h ^= id2.id + 0x9e3779b9 + (id1.id << 6) + (id1.id >> 2);
	return ZuiID{ h };
}

// Macro to generate a ZeroUI ID from a given line in a source file. Will of course fail if this
// line is inside a loop or a function because then several widgets will be given the same ID.
#define ZUI_LINE zuiIdCombine(zuiName(__FILE__), ZuiID{ u64(__LINE__) })

// Macros to generate ZeroUI IDs inside a loop
#define ZUI_LOOP(i) zuiIdCombine(ZUI_LINE, ZuiID{u64(i)})
#define ZUI_LOOP2(i, j) zuiIdCombine(ZUI_LOOP(i), ZuiID{u64(j)})
#define ZUI_LOOP3(i, j, k) zuiIdCombine(ZUI_LOOP2(i, j), ZuiID{u64(k)})

// Macro to generate a ZeroUI ID in an inner context (e.g. a function). The function must itself
// be assigned a ZeroUI ID (=a function parameter) which is then in turn combined with the line
// ID to create a unique ID for each call to the function and line.
#define ZUI_INNER(parent_id) zuiIdCombine(ZUI_LINE, (parent_id))

// Macros to generate ZeroUI IDs inside a loop in an inner context
#define ZUI_INNER_LOOP(parent_id, i) zuiIdCombine(ZUI_INNER(parent_id), ZuiID{u64(i)})
#define ZUI_INNER_LOOP2(parent_id, i, j) zuiIdCombine(ZUI_INNER_LOOP(parent_id, i), ZuiID{u64(j)})
#define ZUI_INNER_LOOP3(parent_id, i, j, k) zuiIdCombine(ZUI_INNER_LOOP2(parent_id, i, j), ZuiID{u64(k)})

// Alignment
// ------------------------------------------------------------------------------------------------

// Alignment types, used to specify how a coordinate should be interpreted. E.g., given a coordinate
// (x,y) we could interpret is as being in the center of the object (ZUI_MID_CENTER), or maybe
// in the lower-left corner of the object (ZUI_BOTTOM_LEFT).

typedef enum ZuiAlign {
	ZUI_BOTTOM_LEFT = 0,
	ZUI_BOTTOM_CENTER,
	ZUI_BOTTOM_RIGHT,
	ZUI_MID_LEFT,
	ZUI_MID_CENTER,
	ZUI_MID_RIGHT,
	ZUI_TOP_LEFT,
	ZUI_TOP_CENTER,
	ZUI_TOP_RIGHT,
	ZUI_ALIGN_FORCE_I32 = I32_MAX
} ZuiAlign;

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

// Input
// ------------------------------------------------------------------------------------------------

// Each frame you are typically going to be calling zuiInputBegin() and zuiInputEnd(), in between
// these calls you can call your UI widget functions.

typedef enum ZuiInputAction {
	ZUI_INPUT_NONE = 0,
	ZUI_INPUT_UP = 1,
	ZUI_INPUT_DOWN = 2,
	ZUI_INPUT_LEFT = 3,
	ZUI_INPUT_RIGHT = 4,
	ZUI_INPUT_POINTER_MOVE = 5,
	ZUI_INPUT_SCROLL = 6,
	ZUI_INPUT_ACTIVATE = 7,
	ZUI_INPUT_CANCEL = 8,
	ZUI_INPUT_FORCE_I32 = I32_MAX
} ZuiInputAction;

sfz_struct(ZuiInput) {

	// Input
	ZuiInputAction action;
	
	// Only for ZUI_INPUT_POINTER_MOVE. Position of the mouse cursor (or finger touching the screen)
	// in screen pixel dimensions, (0,0) in lower-left corner, pos-y up.
	f32x2 pointer_pos;

	// Only for ZUI_INPUT_SCROLL. Amount of scroll generated by the scroll wheel, or other scrolling
	// device (right stick?). Pos-y up.
	f32x2 scroll;

	i32x2 fb_dims;
	f32 delta_time_secs;

	// Position on framebuffer, default aligned to bottom left corner of framebuffer
	i32x2 pos_on_fb;
	ZuiAlign align_on_fb;

	// Size on framebuffer, 0 == entire framebuffer
	i32x2 dims_on_fb;

	// Coordinate system of the surface which things will be drawn upon. E.g., f32x2(100.0f, 100.0f),
	// means that you will be using "percentages" of the total size of the surface when specifying
	// sizes. 0.0f == same as dimsFB.
	f32x2 dims;
};

sfz_extern_c void zuiInputBegin(ZuiCtx* zui, const ZuiInput* desc);
sfz_extern_c bool zuiInputEnd(ZuiCtx* zui);
sfz_extern_c f32x2 zuiGetSurfDims(const ZuiCtx* zui); // Helper to get current ZuiInput::dims

// Rendering
// ------------------------------------------------------------------------------------------------

sfz_struct(ZuiVertex) {
	f32x2 pos; // Position in surface space
	f32x2 texcoord;
	f32x3 color; // Color in sRGB
	f32 alpha;
};
sfz_static_assert(sizeof(ZuiVertex) == 32);

typedef enum ZuiCmdType {
	ZUI_CMD_COLOR = 0,
	ZUI_CMD_TEXTURE = 1,
	ZUI_CMD_FONT_ATLAS = 2,
	ZUI_CMD_FORCE_I32 = I32_MAX
} ZuiCmdType;

sfz_struct(ZuiRenderCmd) {
	ZuiCmdType cmd_type;
	u32 transform_idx;
	u32 start_index;
	u32 num_indices;
	ZuiBox clip; // In fb pixel coords (fb_dims), (0,0) bottom left corner pos-y up, all 0 if no clip.
	u64 image_handle; // Engine specific image handle, 0 is invalid/no image
};

sfz_struct(ZuiRenderDataView) {
	const ZuiVertex* vertices;
	const u16* indices;
	const SfzMat44* transforms;
	const ZuiRenderCmd* cmds;
	u32 num_vertices;
	u32 num_indices;
	u32 num_transforms;
	u32 num_cmds;
	i32x2 fb_dims; // The fb size specified in zuiInputBegin()
};

// Renders the latest state of the UI
sfz_extern_c void zuiRender(ZuiCtx* zui);

sfz_struct(ZuiRenderDebugCfg) {
	u32 widget_depth; // Renders only widgets at the specified depth, 0 renders all levels
};

// Same as zuiRender(), except it outputs a debug view of the current UI state.
sfz_extern_c void zuiRenderDebug(ZuiCtx* zui, const ZuiRenderDebugCfg* debug_cfg);

// Render data guaranteed to be valid until next time zuiRender() is called
sfz_extern_c ZuiRenderDataView zuiGetRenderData(const ZuiCtx* zui);

// Fonts
// ------------------------------------------------------------------------------------------------

// ZeroUI uses a font atlas to render text (powered by stb_truetype).
//
// You must provide ZeroUI with fonts (at least 1) and select which font is your default font. The
// default font will be available as an attribute with the id "default_font".
//
// You must also provide ZeroUI with an engine specific texture handle (u64) so ZeroUI can
// request that the font texture is bound and sampled from in your engine integration. You must
// also call "zuiHasFontTextureUpdate()" and "zuiGetFontTexture()" each frame to check if the font atlas
// has been updated and need to be re-uploaded to the GPU (which you are responsible for).

sfz_extern_c void zuiFontSetTextureHandle(ZuiCtx* zui, u64 handle);
sfz_extern_c bool zuiFontRegister(ZuiCtx* zui, const char* name, const char* ttf_path, f32 size, bool default_font);

// Font texture communication, should be called after rendering.
sfz_extern_c bool zuiHasFontTextureUpdate(const ZuiCtx* zui);
sfz_extern_c SfzImageViewConst zuiGetFontTexture(ZuiCtx* zui);

// Attributes
// ------------------------------------------------------------------------------------------------

// (Rendering) Attributes are used to define specific properties (such as colors, sizes, fonts,
// etc) when rendering a widget.
//
// This is accomplished through the use of an AttributeSet, which is a mapping from name to the
// attribute value. The render function of each widget is allowed to freely read and write to this
// global attribute set.
//
// Writing to the attribute set is primarily meant for parent/container widgets before rendering
// their children, and then revert the changes before exiting the render function.

sfz_struct(ZuiAttrib) {
	u8 bytes[31];
	u8 size;

#ifdef __cplusplus
	template<typename T> T& as() { sfz_assert(sizeof(T) == size); return *(T*)bytes; }
	template<typename T> const T& as() const { return const_cast<ZuiAttrib*>(this)->as(); }
#endif
};
sfz_static_assert(sizeof(ZuiAttrib) == 32);

#ifdef __cplusplus
template<typename T>
inline ZuiAttrib zuiAttribInit(const T& val)
{ 
	ZuiAttrib a = {};
	a.size = sizeof(T);
	a.as<T>() = val;
	return a;
}
#endif

// Register default attributes that will be in the intial AttributeSet when the recursive rendering
// of the widgets starts. It is highly recommended that you register default attributes for any
// and all attributes your widget rendering function reads, to guarantee that they are available.
sfz_extern_c void zuiAttribRegisterDefault(ZuiCtx* zui, const char* attrib_name, ZuiAttrib attrib);
sfz_extern_c void zuiAttribRegisterDefaultNameID(ZuiCtx* zui, const char* attrib_name, const char* val_name);

sfz_extern_c void zuiAttribSet(ZuiCtx* zui, const char* attrib_name, ZuiAttrib attrib);
sfz_extern_c void zuiAttribSetF32(ZuiCtx* zui, const char* attrib_name, f32 val);
sfz_extern_c void zuiAttribSetF32x4(ZuiCtx* zui, const char* attrib_name, f32x4 val);
sfz_extern_c void zuiAttribSetNameID(ZuiCtx* zui, const char* attrib_name, const char* val_name);

// Draw functions
// ------------------------------------------------------------------------------------------------

// Each widget type has a default draw function used to render it. However, it is also possible
// for users to override the draw function for a specific widget. This is the primary mechanism for
// skinning the UI and creating various effects. Most users are expected to eventually replace at
// least some draw functions.
//
// To get started, copy the implementation of the default draw function for a widget and make
// whatever changes you want. Push draw functions with "zuiDrawFuncPush()` and restore previous one
// with "zuiDrawFuncPop()".

struct ZuiWidget;

typedef void ZuiDrawFunc(
	ZuiCtx* zui,
	const ZuiWidget* widget,
	const SfzMat44* surf_to_clip);

sfz_extern_c void zuiDrawFuncPush(ZuiCtx* zui, const char* widget_name, ZuiDrawFunc* draw_func);
sfz_extern_c void zuiDrawFuncPop(ZuiCtx* zui, const char* widget_name);

// Base container widget
// ------------------------------------------------------------------------------------------------

// A base container is used to place widgets at absolute positions relative to the location of
// the container. A base container that covers the entire surface is automatically created when
// input begins using zuiInputBegin(). This means that first widget (which may be a container
// itself) must always be placed in the default root base container.

sfz_constant char ZUI_BASE_WIDGET[] = "base";

sfz_extern_c void zuiBaseBegin(ZuiCtx* zui, ZuiID id);
sfz_extern_c void zuiBasePopupBegin(ZuiCtx* zui, ZuiID id, f32x2 pos, ZuiAlign align, f32x2 dims);

sfz_extern_c void zuiBaseSetPos(ZuiCtx* zui, f32x2 pos);
sfz_extern_c void zuiBaseSetAlign(ZuiCtx* zui, ZuiAlign align);
sfz_extern_c void zuiBaseSetDims(ZuiCtx* zui, f32x2 dims);
sfz_extern_c void zuiBaseSet(ZuiCtx* zui, f32x2 pos, ZuiAlign align, f32x2 dims);
sfz_extern_c void zuiBaseSet2(ZuiCtx* zui, f32 x, f32 y, ZuiAlign align, f32 w, f32 h);
sfz_extern_c void zuiBaseSetWhole(ZuiCtx* zui);

sfz_extern_c void zuiBaseEnd(ZuiCtx* zui);

// Split container widget
// ------------------------------------------------------------------------------------------------

sfz_constant char ZUI_SPLIT_WIDGET[] = "split";

void zuiSplit(ZuiCtx* zui, ZuiID id, bool vertical, f32 units, f32 padding);
void zuiSplitRel(ZuiCtx* zui, ZuiID id, bool vertical, f32 percentage, f32 padding);
void zuiSplitEnd(ZuiCtx* zui);

// List container widget
// ------------------------------------------------------------------------------------------------

sfz_constant char ZUI_LIST_WIDGET[] = "list";

// vertSpacing == 0 -> auto
void zuiList(ZuiCtx* zui, ZuiID id, f32 widget_height, f32 vert_spacing);
void zuiListAddSpacing(ZuiCtx* zui, f32 vert_spacing);
void zuiListSetNextItemHeight(ZuiCtx* zui, f32 height);
void zuiListEnd(ZuiCtx* zui);

// Text widget
// ------------------------------------------------------------------------------------------------

sfz_constant char ZUI_TEXT_WIDGET[] = "text";

void zuiTextfmt(ZuiCtx* zui, ZuiID id, ZuiAlign align, const char* fmt, ...);

// Rectangle widget
// ------------------------------------------------------------------------------------------------

sfz_constant char ZUI_RECT_WIDGET[] = "rect";

void zuiRect(ZuiCtx* zui, ZuiID id, f32x4 srgb_color);
void zuiRectBorder(ZuiCtx* zui, ZuiID id, f32 border_width, f32x4 srgb_color);

// Image widget
// ------------------------------------------------------------------------------------------------

sfz_constant char ZUI_IMAGE_WIDGET[] = "image";

void zuiImage(ZuiCtx* zui, ZuiID id, u64 image_handle);

// Button widget
// ------------------------------------------------------------------------------------------------

sfz_constant char ZUI_BUTTON_WIDGET[] = "button";

bool zuiButton(ZuiCtx* zui, ZuiID id, const char* text);

// Expandable button widget
// ------------------------------------------------------------------------------------------------

sfz_constant char ZUI_EXP_BUTTON_WIDGET[] = "expandable_button";

bool zuiExpButton(ZuiCtx* zui, ZuiID id, const char* text, f32x2 exp_pos, ZuiAlign exp_align, f32x2 exp_dims);
void zuiExpButtonEnd(ZuiCtx* zui);

// Checkbox widget
// ------------------------------------------------------------------------------------------------

sfz_constant char ZUI_CHECKBOX_WIDGET[] = "checkbox";

bool zuiCheckbox(ZuiCtx* zui, ZuiID id, const char* text, bool* is_set);

// Integer select widget
// ------------------------------------------------------------------------------------------------

sfz_constant char ZUI_INT_SELECT_WIDGET[] = "int_select";

bool zuiIntSelect(ZuiCtx* zui, ZuiID id, const char* text, i32* val, i32 min, i32 max, i32 step);

// Float select widget
// ------------------------------------------------------------------------------------------------

sfz_constant char ZUI_FLOAT_SELECT_WIDGET[] = "float_select";

bool zuiFloatSelect(ZuiCtx* zui, ZuiID id, const char* text, f32* val, f32 min, f32 max, f32 step);

#endif
