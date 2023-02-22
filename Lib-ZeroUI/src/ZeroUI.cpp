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

#include "ZeroUI.h"

#include <sfz_color.h>

#include "ZeroUI_Internal.hpp"
#include "ZeroUI_Drawing.hpp"

// Widget IDs
// ------------------------------------------------------------------------------------------------

constexpr ZuiID ZUI_BASE_ID = zuiName(ZUI_BASE_WIDGET);
constexpr ZuiID ZUI_SPLIT_ID = zuiName(ZUI_SPLIT_WIDGET);
constexpr ZuiID ZUI_LIST_ID = zuiName(ZUI_LIST_WIDGET);
constexpr ZuiID ZUI_TEXT_ID = zuiName(ZUI_TEXT_WIDGET);
constexpr ZuiID ZUI_RECT_ID = zuiName(ZUI_RECT_WIDGET);
constexpr ZuiID ZUI_IMAGE_ID = zuiName(ZUI_IMAGE_WIDGET);
constexpr ZuiID ZUI_BUTTON_ID = zuiName(ZUI_BUTTON_WIDGET);
constexpr ZuiID ZUI_EXP_BUTTON_ID = zuiName(ZUI_EXP_BUTTON_WIDGET);
constexpr ZuiID ZUI_CHECKBOX_ID = zuiName(ZUI_CHECKBOX_WIDGET);
constexpr ZuiID ZUI_INT_SELECT_ID = zuiName(ZUI_INT_SELECT_WIDGET);
constexpr ZuiID ZUI_FLOAT_SELECT_ID = zuiName(ZUI_FLOAT_SELECT_WIDGET);

// Context
// ------------------------------------------------------------------------------------------------

static void zuiCtxInitBuiltInWidgets(ZuiCtx* zui);

sfz_extern_c ZuiCtx* zuiCtxInit(ZuiCfg* cfg, SfzAllocator* allocator)
{
	ZuiCtx* zui = sfz_new<ZuiCtx>(allocator, sfz_dbg(""));
	*zui = {};

	zui->heap_allocator = allocator;

	// Initialize draw context
	const bool draw_success = zuiInternalDrawCtxInit(&zui->draw_ctx, cfg, zui->heap_allocator);
	if (!draw_success) {
		sfz_delete(allocator, zui);
		return nullptr;
	}

	// Initialize widget types
	zui->widget_types.init(32, allocator, sfz_dbg(""));

	// Initialize widget trees
	zui->input_idx = 0;
	zui->widget_trees[0].arena.init(allocator, cfg->arena_memory_limit_bytes, sfz_dbg("ZeroUI::arena1"));
	zui->widget_trees[1].arena.init(allocator, cfg->arena_memory_limit_bytes, sfz_dbg("ZeroUI::arena2"));
	zuiWidgetTreeClear(&zui->widget_trees[0]);
	zuiWidgetTreeClear(&zui->widget_trees[1]);

	// Initialize input lock stack
	zui->input_locks.init(64, allocator, sfz_dbg(""));

	// Initialize attribute set
	zui->attribs.init(256, allocator, sfz_dbg(""));
	zui->default_attribs.init(256, allocator, sfz_dbg(""));

	// Initialize built-in widgets
	zuiCtxInitBuiltInWidgets(zui);

	return zui;
}

sfz_extern_c void zuiCtxDestroy(ZuiCtx* zui)
{
	if (zui == nullptr) return;
	SfzAllocator* allocator = zui->heap_allocator;
	zuiInternalDrawCtxDestroy(&zui->draw_ctx);
	sfz_delete(allocator, zui);
}

// Input
// ------------------------------------------------------------------------------------------------

static void zuiInputInitRootAsBase(ZuiCtx* zui, f32x2 dims);

sfz_extern_c void zuiInputBegin(ZuiCtx* zui, const ZuiInput* input)
{
	// New input, clear oldest tree to make room for new widgets
	zui->input_idx += 1;
	ZuiWidgetTree& curr_tree = zui->currTree();
	zuiWidgetTreeClear(&curr_tree);

	// Set input
	zui->input = *input;

	// Clear all drawfunc stacks and set default draw func
	for (auto pair : zui->widget_types) {
		pair.value.draw_func_stack.clear();
		pair.value.draw_func_stack.add(pair.value.desc.draw_func);
	}

	// Setup default base container for surface as root
	zuiInputInitRootAsBase(zui, input->dims);

	// Get size of surface on framebuffer
	i32x2 dims_on_fb = input->dims_on_fb;
	if (dims_on_fb == i32x2_splat(0)) dims_on_fb = input->fb_dims;

	// Get internal size of surface
	if (sfz::eqf(input->dims, f32x2_splat(0.0f))) {
		zui->input.dims = f32x2_from_i32(dims_on_fb);
	}

	// Calculate transform
	const f32x3 fb_to_clip_scale = f32x3_init2(2.0f / f32x2_from_i32(input->fb_dims), 1.0f);
	const f32x3 fb_to_clip_transl = f32x3_init(-1.0f, -1.0f, 0.0f);
	const SfzMat44 fb_to_clip = sfzMat44Translation3(fb_to_clip_transl) * sfzMat44Scaling3(fb_to_clip_scale);

	f32x2 half_offset = f32x2_splat(0.0f);
	switch (input->align_on_fb) {
	case ZUI_BOTTOM_LEFT:
		half_offset = f32x2_init(0.0f, 0.0f);
		break;
	case ZUI_BOTTOM_CENTER:
		half_offset = f32x2_init(-0.5f * f32(dims_on_fb.x), 0.0f);
		break;
	case ZUI_BOTTOM_RIGHT:
		half_offset = f32x2_init(-1.0f * f32(dims_on_fb.x), 0.0f);
		break;
	case ZUI_MID_LEFT:
		half_offset = f32x2_init(0.0f, -0.5f * f32(dims_on_fb.y));
		break;
	case ZUI_MID_CENTER:
		half_offset = f32x2_init(-0.5f * f32(dims_on_fb.x), -0.5f * f32(dims_on_fb.y));
		break;
	case ZUI_MID_RIGHT:
		half_offset = f32x2_init(-1.0f * f32(dims_on_fb.x), -0.5f * f32(dims_on_fb.y));
		break;
	case ZUI_TOP_LEFT:
		half_offset = f32x2_init(0.0f, -1.0f * f32(dims_on_fb.y));
		break;
	case ZUI_TOP_CENTER:
		half_offset = f32x2_init(-0.5f * f32(dims_on_fb.x), -1.0f * f32(dims_on_fb.y));
		break;
	case ZUI_TOP_RIGHT:
		half_offset = f32x2_init(-1.0f * f32(dims_on_fb.x), -1.0f * f32(dims_on_fb.y));
		break;
	default: sfz_assert_hard(false);
	}

	// Matrices
	const f32x3 surf_to_fb_scale = f32x3_init2((1.0f / zui->input.dims) * f32x2_from_i32(dims_on_fb), 1.0f);
	const f32x3 surf_to_fb_transl = f32x3_init2(f32x2_from_i32(input->pos_on_fb) + half_offset, 0.0f);
	zui->surf_to_fb = sfzMat44Translation3(surf_to_fb_transl) * sfzMat44Scaling3(surf_to_fb_scale);
	zui->surf_to_clip = fb_to_clip * zui->surf_to_fb;
	zui->fb_to_surf = sfzMat44Inverse(zui->surf_to_fb);

	// Pointer pos
	// A bit of magic here, basically we want to remember the pointer pos from when it was last
	// input (needed for some logic). But we want to forget it if we get some input that indicates
	// that it might no longer be valid.
	if (input->action == ZUI_INPUT_UP ||
		input->action == ZUI_INPUT_DOWN ||
		input->action == ZUI_INPUT_LEFT ||
		input->action == ZUI_INPUT_RIGHT ||
		input->action == ZUI_INPUT_CANCEL) {
		// Forget it
		zui->pointer_pos_ss = f32x2_splat(-F32_MAX);
	}
	else if (input->action == ZUI_INPUT_POINTER_MOVE) {
		// Update it
		zui->pointer_pos_ss = sfzMat44TransformPoint(zui->fb_to_surf, f32x3_init2(zui->input.pointer_pos, 0.0f)).xy();
	}
}

static void zuiInputKeyMoveLogic(ZuiCtx* zui, ZuiWidget* w, bool* move_active)
{
	const ZuiWidgetType* type = zui->widget_types.get(w->widget_type_id.id);
	sfz_assert(type != nullptr);

	// If input is consumed, exit
	if (zui->input.action == ZUI_INPUT_NONE) return;

	// For leaf widgets
	const bool is_leaf = w->children.isEmpty();
	if (type->desc.focuseable && is_leaf) {
		if (*move_active) {
			w->base.setFocused();
			*move_active = false;
			zui->input.action = ZUI_INPUT_NONE;
		}
		else if (w->base.focused && !*move_active) {
			w->base.setUnfocused();
			*move_active = true;
		}
		return;
	}

	// For parent widgets
	if (!is_leaf) {
		w->base.setUnfocused();
		i32 child_idx = type->desc.get_next_child_idx_func(w, zui->input.action, -1);
		while (0 <= child_idx && child_idx < i32(w->children.size())) {
			ZuiWidget& child = w->children[child_idx];
			const bool was_active_before = zui->input.action != ZUI_INPUT_NONE && *move_active;
			zuiInputKeyMoveLogic(zui, &child, move_active);
			const bool move_consumed = was_active_before && zui->input.action == ZUI_INPUT_NONE;
			if (move_consumed) {
				if (type->desc.scroll_set_child_visible_func != nullptr) type->desc.scroll_set_child_visible_func(w, child_idx);
				return; // Early out
			}
			child_idx = type->desc.get_next_child_idx_func(w, zui->input.action, child_idx);
		}
	}
}

static void zuiInputPointerMoveLogic(const ZuiCtx* zui, ZuiWidget* w)
{
	const ZuiWidgetType* type = zui->widget_types.get(w->widget_type_id.id);
	sfz_assert(type != nullptr);

	if (type->desc.focuseable && w->base.box.pointInside(zui->pointer_pos_ss)) {
		w->base.setFocused();
	}
	else {
		w->base.setUnfocused();
	}

	for (ZuiWidget& child : w->children) {
		if (w->base.box.overlaps(child.base.box)) {
			zuiInputPointerMoveLogic(zui, &child);
		}
	}
}

// Strategy: Find first focused leaf node, walk back up through its parents and scroll closest one.
static bool zuiInputScrollLogic(const ZuiCtx* zui, ZuiWidget* w)
{
	const ZuiWidgetType* type = zui->widget_types.get(w->widget_type_id.id);
	sfz_assert(type != nullptr);

	const bool is_leaf = w->children.isEmpty();
	if (is_leaf && w->base.focused) return true;

	for (ZuiWidget& child : w->children) {
		const bool try_scroll = zuiInputScrollLogic(zui, &child);
		if (try_scroll) {
			if (type->desc.scroll_input_func != nullptr) {
				type->desc.scroll_input_func(w, zui->input.scroll);
				return false; // We used up our scroll! Stop other parent's from scrolling.
			}
		}
	}

	return false;
}

static bool zuiInputActivateLogic(const ZuiCtx* zui, ZuiWidget* w)
{
	const ZuiWidgetType* type = zui->widget_types.get(w->widget_type_id.id);
	sfz_assert(type != nullptr);

	if (type->desc.activateable && w->base.focused && !w->base.activated) {
		w->base.setActivated();
		return true;
	}

	for (ZuiWidget& child : w->children) {
		const bool consumed = zuiInputActivateLogic(zui, &child);
		if (consumed) return true;
	}

	return false;
}

sfz_extern_c bool zuiInputEnd(ZuiCtx* zui)
{
	bool input_used = false;

	ZuiWidgetTree& curr_tree = zui->currTree();
	ZuiWidget* root = &curr_tree.root;

	// If we have input locks, move root down to the last one
	bool is_input_locked = false;
	while (!zui->input_locks.isEmpty()) {
		const ZuiID last_id = zui->input_locks.last();
		ZuiWidget* w = zuiFindWidgetFromID(root, last_id);
		if (w != nullptr) {
			// As a rule, an input-locked widget MUST only have 1 child and this child MUST be a
			// base or split container. We might relax these rules later, but for now they hold.
			sfz_assert_hard(w->children.size() == 1);
			sfz_assert_hard(w->children.last().widget_type_id == ZUI_BASE_ID || w->children.last().widget_type_id == ZUI_SPLIT_ID);
			root = &w->children[0];
			is_input_locked = true;
			break;
		}
		zui->input_locks.pop();
	}

	// Handle various types of inputs
	switch (zui->input.action) {
	case ZUI_INPUT_UP:
	case ZUI_INPUT_DOWN:
	case ZUI_INPUT_LEFT:
	case ZUI_INPUT_RIGHT:
		{
			bool move_active = false;
			zuiInputKeyMoveLogic(zui, root, &move_active);

			// If input wasn't consumed, try again with move already active.
			if (zui->input.action != ZUI_INPUT_NONE) {
				move_active = true;
				zuiInputKeyMoveLogic(zui, root, &move_active);
			}
		}
		break;

	case ZUI_INPUT_POINTER_MOVE:
		zuiInputPointerMoveLogic(zui, root);
		break;

	case ZUI_INPUT_SCROLL:
		zuiInputScrollLogic(zui, root);
		break;

	case ZUI_INPUT_ACTIVATE:
		// If we are input locked we want to exit out if user clicked outside input locked region
		if (is_input_locked && zui->pointer_pos_ss != f32x2_splat(-F32_MAX)) {
			if (!root->base.box.pointInside(zui->pointer_pos_ss)) {
				zui->input_locks.pop();
			}
			else {
				input_used = zuiInputActivateLogic(zui, root);
			}
		}
		else {
			input_used = zuiInputActivateLogic(zui, root);
		}
		break;

	case ZUI_INPUT_CANCEL:
		// Cancel is simply the operation of removing an input lock
		if (!zui->input_locks.isEmpty()) {
			zui->input_locks.pop();
			input_used = true;
		}
		break;

	case ZUI_INPUT_NONE:
		// If no input was sent in, then of course it was used.
		input_used = true;
		break;
	default:
		// Do nothing
		break;
	}

	return input_used;
}

sfz_extern_c f32x2 zuiGetSurfDims(const ZuiCtx* zui)
{
	return zui->input.dims;
}

// Rendering
// ------------------------------------------------------------------------------------------------

static void zuiRenderInternal(ZuiCtx* zui, ZuiWidget* w)
{
	const ZuiWidgetType* type = zui->widget_types.get(w->widget_type_id.id);
	sfz_assert(type != nullptr);

	// Render ourselves
	if (w->draw_func != nullptr ) w->draw_func(zui, w, &zui->surf_to_clip);

	// Render children if widget has any
	if (!w->children.isEmpty()) {

		// Set attributes and backup old ones
		SfzMap16<u64, ZuiAttrib> backup;
		if (w->attribs.capacity() != 0) {
			for (const auto& pair : w->attribs) {

				// Backup old attribute
				ZuiAttrib* old_attrib = zui->attribs.get(pair.key);
				if (old_attrib != nullptr) {
					backup.put(pair.key, *old_attrib);
				}

				// Set new one
				zui->attribs.put(pair.key, pair.value);
			}
		}

		// Render children
		if (type->desc.skip_clipping) zui->draw_ctx.clip_stack.add({});
		else zui->draw_ctx.clip_stack.add(w->base.box);
		for (ZuiWidget& child : w->children) {
			zuiRenderInternal(zui, &child);
		}
		zui->draw_ctx.clip_stack.pop();

		// Restore old attributes
		if (w->attribs.capacity() != 0) {
			for (const auto& pair : w->attribs) {
				zui->attribs.put(pair.key, pair.value);
			}
		}
	}
}

sfz_extern_c void zuiRender(ZuiCtx* zui)
{
	// Clear render data
	zui->draw_ctx.vertices.clear();
	zui->draw_ctx.indices.clear();
	zui->draw_ctx.transforms.clear();
	zui->draw_ctx.render_cmds.clear();

	// Clear clip stack and push default clip box
	zui->draw_ctx.clip_stack.clear();
	zui->draw_ctx.clip_stack.add(ZuiBox{});

	// Clear attribute set and set defaults
	zui->attribs.clear();
	for (const auto& pair : zui->default_attribs) {
		zui->attribs.put(pair.key, pair.value);
	}

	// Draw recursively
	zuiRenderInternal(zui, &zui->currTree().root);

	// Fix all clip boxes so that they are in fb space instead of in surface space
	auto surfToFB = [&](f32x2 p) {
		const f32x3 tmp = sfzMat44TransformPoint(zui->surf_to_fb, f32x3_init2(p, 0.0f));
		return tmp.xy();
	};
	for (ZuiRenderCmd& cmd : zui->draw_ctx.render_cmds) {
		if (cmd.clip.min != f32x2_splat(0.0f) || cmd.clip.max != f32x2_splat(0.0f)) {
			cmd.clip.min = surfToFB(cmd.clip.min);
			cmd.clip.max = surfToFB(cmd.clip.max);
		}
	}
}

static void zuiRenderDebugInternal(
	ZuiCtx* zui, const ZuiRenderDebugCfg* debug_cfg, ZuiWidget* w, const SfzMat44* transform, u32* idx, u32* depth)
{
	*depth += 1;
	const ZuiWidgetType* type = zui->widget_types.get(w->widget_type_id.id);
	sfz_assert(type != nullptr);

	// Render if we are on a depth level we are supposed to render
	if (debug_cfg->widget_depth == 0 || debug_cfg->widget_depth == *depth) {
		// Rect
		const f32x3 color = sfzGetRandomColorDefaults(*idx);
		const SfzMat44 rect_x_form =
			*transform * sfzMat44Translation3(f32x3_init2(w->base.box.center(), 0.0f));
		zuiDrawRect(&zui->draw_ctx, rect_x_form, w->base.box.dims(), f32x4_init3(color, 0.5f));

		// Name of widget type
		const ZuiID default_font_id = zui->attribs.get(zuiName("default_font").id)->as<ZuiID>();
		const f32 text_size = 2.0f;
		const SfzMat44 text_x_form =
			*transform *
			sfzMat44Translation3(f32x3_init(w->base.box.min.x, w->base.box.max.y - text_size * 0.3f, 0.0f));
		zuiDrawText(
			&zui->draw_ctx, text_x_form, ZUI_MID_LEFT, default_font_id, text_size, f32x4_init(1.0f, 1.0f, 1.0f, 0.75f), type->name.str);
	}

	// Children
	for (ZuiWidget& child : w->children) {
		zuiRenderDebugInternal(zui, debug_cfg, &child, transform, idx, depth);
	}

	*idx += 1;
	*depth -= 1;
}

sfz_extern_c void zuiRenderDebug(ZuiCtx* zui, const ZuiRenderDebugCfg* debug_cfg)
{
	// Clear render data
	zui->draw_ctx.vertices.clear();
	zui->draw_ctx.indices.clear();
	zui->draw_ctx.transforms.clear();
	zui->draw_ctx.render_cmds.clear();

	// Clear clip stack and push default clip box
	zui->draw_ctx.clip_stack.clear();
	zui->draw_ctx.clip_stack.add(ZuiBox{});

	// Clear attribute set and set defaults
	zui->attribs.clear();
	for (const auto& pair : zui->default_attribs) {
		zui->attribs.put(pair.key, pair.value);
	}

	// Draw recursively
	u32 idx = 0;
	u32 depth = 0;
	zuiRenderDebugInternal(zui, debug_cfg, &zui->currTree().root, &zui->surf_to_clip, &idx, &depth);

	// Fix all clip boxes so that they are in fb space instead of in surface space
	auto surfToFB = [&](f32x2 p) {
		const f32x3 tmp = sfzMat44TransformPoint(zui->surf_to_fb, f32x3_init2(p, 0.0f));
		return tmp.xy();
	};
	for (ZuiRenderCmd& cmd : zui->draw_ctx.render_cmds) {
		if (cmd.clip.min != f32x2_splat(0.0f) && cmd.clip.max != f32x2_splat(0.0f)) {
			cmd.clip.min = surfToFB(cmd.clip.min);
			cmd.clip.max = surfToFB(cmd.clip.max);
		}
	}
}

sfz_extern_c ZuiRenderDataView zuiGetRenderData(const ZuiCtx* zui)
{
	ZuiRenderDataView view = {};
	view.vertices = zui->draw_ctx.vertices.data();
	view.num_vertices = zui->draw_ctx.vertices.size();
	view.indices = zui->draw_ctx.indices.data();
	view.num_indices = zui->draw_ctx.indices.size();
	view.transforms = zui->draw_ctx.transforms.data();
	view.num_transforms = zui->draw_ctx.transforms.size();
	view.cmds = zui->draw_ctx.render_cmds.data();
	view.num_cmds = zui->draw_ctx.render_cmds.size();
	view.fb_dims = zui->input.fb_dims;
	return view;
}

// Fonts
// ------------------------------------------------------------------------------------------------

sfz_extern_c void zuiFontSetTextureHandle(ZuiCtx* zui, u64 handle)
{
	zui->draw_ctx.user_font_tex_handle = handle;
}

sfz_extern_c bool zuiFontRegister(ZuiCtx* zui, const char* name, const char* ttf_path, f32 size, bool default_font)
{
	const ZuiID id = zuiName(name);
	bool success = zuiInternalDrawAddFont(&zui->draw_ctx, id, ttf_path, size, zui->heap_allocator);
	if (default_font) {
		sfz_assert_hard(success);
		zuiAttribRegisterDefault(zui, "default_font", zuiAttribInit(id));
	}
	return success;
}

sfz_extern_c bool zuiHasFontTextureUpdate(const ZuiCtx* zui)
{
	return zui->draw_ctx.font_img_modified;
}

sfz_extern_c SfzImageViewConst zuiGetFontTexture(ZuiCtx* zui)
{
	zui->draw_ctx.font_img_modified = false;
	SfzImageViewConst view = {};
	view.raw_data = zui->draw_ctx.font_img.data();
	view.type = SFZ_IMAGE_TYPE_R_U8;
	view.res = i32x2_splat(zui->draw_ctx.font_img_res);
	return view;
}

// Attributes
// ------------------------------------------------------------------------------------------------

sfz_extern_c void zuiAttribRegisterDefault(ZuiCtx* zui, const char* attrib_name, ZuiAttrib attrib)
{
	const ZuiID attrib_id = zuiName(attrib_name);
	sfz_assert(zui->default_attribs.get(attrib_id.id) == nullptr);
	zui->default_attribs.put(attrib_id.id, attrib);
}

sfz_extern_c void zuiAttribRegisterDefaultNameID(ZuiCtx* zui, const char* attrib_name, const char* val_name)
{
	zuiAttribRegisterDefault(zui, attrib_name, zuiAttribInit(zuiName(val_name)));
}

sfz_extern_c void zuiAttribSet(ZuiCtx* zui, const char* attrib_name, ZuiAttrib attrib)
{
	const ZuiID attrib_id = zuiName(attrib_name);
	ZuiWidget& parent = zuiWidgetTreeGetCurrentParent(&zui->currTree());
	if (parent.attribs.capacity() == 0) {
		parent.attribs.init(32, zui->currTree().arena.getArena(), sfz_dbg(""));
	}
	sfz_assert(parent.attribs.capacity() != 0);
	parent.attribs.put(attrib_id.id, attrib);
}

sfz_extern_c void zuiAttribSetF32(ZuiCtx* zui, const char* attrib_name, f32 val)
{
	zuiAttribSet(zui, attrib_name, zuiAttribInit<f32>(val));
}

sfz_extern_c void zuiAttribSetF32x4(ZuiCtx* zui, const char* attrib_name, f32x4 val)
{
	zuiAttribSet(zui, attrib_name, zuiAttribInit<f32x4>(val));
}

sfz_extern_c void zuiAttribSetNameID(ZuiCtx* zui, const char* attrib_name, const char* val_name)
{
	zuiAttribSet(zui, attrib_name, zuiAttribInit(zuiName(val_name)));
}

// Built-in attributes for default draw functions
static constexpr ZuiID DEFAULT_FONT_ATTRIB_ID = zuiName("default_font");
static constexpr ZuiID FONT_COLOR = zuiName("font_color");
static constexpr ZuiID BASE_COLOR = zuiName("base_color");
static constexpr ZuiID FOCUS_COLOR = zuiName("focus_color");
static constexpr ZuiID ACTIVATE_COLOR = zuiName("activate_color");
static constexpr ZuiID BUTTON_TEXT_SCALING = zuiName("button_text_scaling");
static constexpr ZuiID BUTTON_BORDER_WIDTH = zuiName("button_border_width");
static constexpr ZuiID BUTTON_INNER_PADDING = zuiName("button_inner_padding");

// Draw functions
// ------------------------------------------------------------------------------------------------

sfz_extern_c void zuiDrawFuncPush(ZuiCtx* zui, const char* widgetName, ZuiDrawFunc* drawFunc)
{
	const ZuiID widget_id = zuiName(widgetName);
	ZuiWidgetType* type = zui->widget_types.get(widget_id.id);
	sfz_assert(type != nullptr);
	type->draw_func_stack.add(drawFunc);
}

sfz_extern_c void zuiDrawFuncPop(ZuiCtx* zui, const char* widgetName)
{
	const ZuiID widget_id = zuiName(widgetName);
	ZuiWidgetType* type = zui->widget_types.get(widget_id.id);
	sfz_assert(type != nullptr);
	sfz_assert(type->draw_func_stack.size() > 1);
	type->draw_func_stack.pop();
}

// Base container widget
// ------------------------------------------------------------------------------------------------

struct ZuiBaseConData final {
	f32x2 next_pos = f32x2_splat(0.0f);
	ZuiAlign next_align;
	f32x2 next_dims = f32x2_splat(0.0f);
};

static void baseGetNextWidgetBox(ZuiWidget* widget, ZuiBox* box_out)
{
	ZuiBaseConData& data = *widget->data<ZuiBaseConData>();
	f32x2 bottom_left_pos = widget->base.box.min;
	f32x2 center_pos = zuiCalcCenterPos(data.next_pos, data.next_align, data.next_dims);
	f32x2 next_pos = bottom_left_pos + center_pos;
	*box_out = zuiBoxInit(next_pos, data.next_dims);
}

sfz_extern_c void zuiBaseBegin(ZuiCtx* zui, ZuiID id)
{
	ZuiWidget* w = zuiCtxCreateWidgetParent<ZuiBaseConData>(zui, id, ZUI_BASE_ID);
	ZuiBaseConData* data = w->data<ZuiBaseConData>();

	// Set initial next widget dimensions/position to cover entire container
	data->next_dims = w->base.box.dims();
	data->next_pos = data->next_dims * 0.5f;
	data->next_align = ZUI_MID_CENTER;
}

sfz_extern_c void zuiBasePopupBegin(ZuiCtx* zui, ZuiID id, f32x2 pos, ZuiAlign align, f32x2 dims)
{
	ZuiWidget* w = zuiCtxCreateWidgetParent<ZuiBaseConData>(zui, id, ZUI_BASE_ID);
	ZuiBaseConData* data = w->data<ZuiBaseConData>();

	// Set base box to whatever user specified
	const f32x2 center = zuiCalcCenterPos(pos, align, dims);
	w->base.box = zuiBoxInit(center, dims);

	// Set initial next widget dimensions/position to cover entire container
	data->next_dims = w->base.box.dims();
	data->next_pos = data->next_dims * 0.5f;
	data->next_align = ZUI_MID_CENTER;
}

sfz_extern_c void zuiBaseSetPos(ZuiCtx* zui, f32x2 pos)
{
	ZuiWidget& parent = zuiWidgetTreeGetCurrentParent(&zui->currTree());
	sfz_assert(parent.widget_type_id == ZUI_BASE_ID);
	parent.data<ZuiBaseConData>()->next_pos = pos;
}

sfz_extern_c void zuiBaseSetAlign(ZuiCtx* zui, ZuiAlign align)
{
	ZuiWidget& parent = zuiWidgetTreeGetCurrentParent(&zui->currTree());
	sfz_assert(parent.widget_type_id == ZUI_BASE_ID);
	parent.data<ZuiBaseConData>()->next_align = align;
}

sfz_extern_c void zuiBaseSetDims(ZuiCtx* zui, f32x2 dims)
{
	ZuiWidget& parent = zuiWidgetTreeGetCurrentParent(&zui->currTree());
	sfz_assert(parent.widget_type_id == ZUI_BASE_ID);
	parent.data<ZuiBaseConData>()->next_dims = dims;
}

sfz_extern_c void zuiBaseSet(ZuiCtx* zui, f32x2 pos, ZuiAlign align, f32x2 dims)
{
	zuiBaseSetPos(zui, pos);
	zuiBaseSetAlign(zui, align);
	zuiBaseSetDims(zui, dims);
}

sfz_extern_c void zuiBaseSet2(ZuiCtx* zui, f32 x, f32 y, ZuiAlign align, f32 w, f32 h)
{
	zuiBaseSet(zui, f32x2_init(x, y), align, f32x2_init(w, h));
}

sfz_extern_c void zuiBaseSetWhole(ZuiCtx* zui)
{
	ZuiWidget& w = zuiWidgetTreeGetCurrentParent(&zui->currTree());
	sfz_assert(w.widget_type_id == ZUI_BASE_ID);
	ZuiBaseConData* data = w.data<ZuiBaseConData>();
	data->next_dims = w.base.box.dims();
	data->next_pos = data->next_dims * 0.5f;
	data->next_align = ZUI_MID_CENTER;
}

sfz_extern_c void zuiBaseEnd(ZuiCtx* zui)
{
	zuiCtxPopWidgetParent(zui, ZUI_BASE_ID);
}

// Split container widget
// ------------------------------------------------------------------------------------------------

struct ZuiSplitData final {
	bool vertical;
	f32 units;
	f32 padding;
	u32 split_idx; // 0 initialized, 1 after first split, etc
};

static void zuiSplitGetNextWidgetBox(ZuiWidget* w, ZuiBox* box_out)
{
	ZuiSplitData& data = *w->data<ZuiSplitData>();

	*box_out = w->base.box;
	if (data.split_idx == 0) {
		if (data.vertical) {
			const f32 second_height = w->base.box.dims().y - data.units;
			sfz_assert(0.0f <= second_height);
			box_out->min.y += second_height;
		}
		else {
			const f32 second_width = w->base.box.dims().x - data.units;
			sfz_assert(0.0f <= second_width);
			box_out->max.x -= second_width;
		}
	}
	else if (data.split_idx == 1) {
		if (data.vertical) {
			box_out->max.y -= data.units;
		} else {
			box_out->min.x += data.units;
		}
	}
	else {
		sfz_assert(false);
	}
	box_out->min += f32x2_splat(data.padding);
	box_out->max -= f32x2_splat(data.padding);

	data.split_idx += 1;
}

static i32 zuiSplitGetNextChildIdx(const ZuiWidget* w, ZuiInputAction action, i32 curr_idx)
{
	const ZuiSplitData& data = *w->data<ZuiSplitData>();
	sfz_assert(data.split_idx <= 2);
	sfz_assert(w->children.size() <= 2);
	i32 next_idx = -1;
	if (data.vertical) {
		if (action == ZUI_INPUT_DOWN) {
			next_idx = curr_idx + 1;
		}
		else if (action == ZUI_INPUT_UP) {
			if (curr_idx == -1) curr_idx = i32(w->children.size());
			next_idx = curr_idx - 1;
		}
	}
	else {
		if (action == ZUI_INPUT_RIGHT) {
			next_idx = curr_idx + 1;
		}
		else if (action == ZUI_INPUT_LEFT) {
			if (curr_idx == -1) curr_idx = i32(w->children.size());
			next_idx = curr_idx - 1;
		}
		// Small hack to make children focusable from nothing
		if (curr_idx == -1 && (action == ZUI_INPUT_UP || action == ZUI_INPUT_DOWN)) {
			next_idx = 0;
		}
	}
	return next_idx;
}

void zuiSplit(ZuiCtx* zui, ZuiID id, bool vertical, f32 units, f32 padding)
{
	ZuiWidget* w = zuiCtxCreateWidgetParent<ZuiSplitData>(zui, id, ZUI_SPLIT_ID);
	ZuiSplitData* data = w->data<ZuiSplitData>();
	data->vertical = vertical;
	data->units = units;
	data->padding = padding;
	data->split_idx = 0;
}

void zuiSplitRel(ZuiCtx* zui, ZuiID id, bool vertical, f32 percentage, f32 padding)
{
	ZuiWidget* w = zuiCtxCreateWidgetParent<ZuiSplitData>(zui, id, ZUI_SPLIT_ID);
	ZuiSplitData* data = w->data<ZuiSplitData>();
	data->vertical = vertical;
	sfz_assert(0.0f <= percentage && percentage <= 1.0f);
	if (data->vertical) data->units = percentage * w->base.box.dims().y;
	else data->units = percentage * w->base.box.dims().x;
	data->padding = padding;
	data->split_idx = 0;
}

void zuiSplitEnd(ZuiCtx* zui)
{
	zuiCtxPopWidgetParent(zui, ZUI_SPLIT_ID);
}

// List container widget
// ------------------------------------------------------------------------------------------------

struct ZuiListData final {
	f32 widget_height;
	f32 vert_spacing;
	f32 offset_y;
	f32 scroll_y;
	f32 widget_height_override;
};

static void listGetNextWidgetBox(ZuiWidget* w,ZuiBox* box_out)
{
	ZuiListData& data = *w->data<ZuiListData>();
	const f32 curr_pos_y = w->base.box.center().y + w->base.box.dims().y * 0.5f + data.offset_y + data.scroll_y;
	f32 widget_height = data.widget_height;
	if (0.0f < data.widget_height_override) {
		widget_height = data.widget_height_override;
		data.widget_height_override = -1.0f;
	}
	f32x2 next_pos = f32x2_init(w->base.box.center().x, curr_pos_y);
	data.offset_y -= (widget_height + data.vert_spacing);
	*box_out = zuiBoxInit(next_pos, f32x2_init(w->base.box.dims().x, widget_height));
}

static void listScrollInput(ZuiWidget* w, f32x2 scroll)
{
	ZuiListData& data = *w->data<ZuiListData>();
	data.scroll_y += -scroll.y;
	const f32 max_y =
		f32_max(f32_abs(data.offset_y) - data.widget_height * 0.5f - w->base.box.dims().y, 0.0f);
	data.scroll_y = f32_clamp(data.scroll_y, 0.0f, max_y);
}

static void listScrollSetChildVisible(ZuiWidget* w, i32 childIdx)
{
	const f32 child_min_y = w->children[childIdx].base.box.min.y;
	const f32 child_max_y = w->children[childIdx].base.box.max.y;
	if (w->base.box.min.y <= child_min_y && child_max_y <= w->base.box.max.y) return;
	ZuiListData& data = *w->data<ZuiListData>();
	data.scroll_y = childIdx * (data.widget_height + data.vert_spacing);
	const f32 max_y =
		f32_max(f32_abs(data.offset_y) - data.widget_height * 0.5f - w->base.box.dims().y, 0.0f);
	data.scroll_y = f32_clamp(data.scroll_y, 0.0f, max_y);
}

void zuiList(ZuiCtx* zui, ZuiID id, f32 widget_height, f32 vert_spacing)
{
	bool initial = false;
	ZuiWidget* w = zuiCtxCreateWidgetParent<ZuiListData>(zui, id, ZUI_LIST_ID, &initial);
	ZuiListData* data = w->data<ZuiListData>();

	// Set list data from parameters
	sfz_assert(widget_height > 0.0f);
	data->widget_height = widget_height;
	data->vert_spacing = vert_spacing;
	if (vert_spacing <= 0.0f) data->vert_spacing = data->widget_height * 0.5f;

	// Calculate initial next widget y-pos
	data->offset_y = -data->widget_height * 0.5f;
	if (initial) data->scroll_y = 0.0f;

	// No widget height override unless requested
	data->widget_height_override = -1.0f;

	// Can't activate list container
	w->base.activated = false;
}

void zuiListAddSpacing(ZuiCtx* zui, f32 vert_spacing)
{
	ZuiWidget& parent = zuiWidgetTreeGetCurrentParent(&zui->currTree());
	sfz_assert(parent.widget_type_id == ZUI_LIST_ID);
	ZuiListData* data = parent.data<ZuiListData>();
	data->offset_y -= vert_spacing;
}

void zuiListSetNextItemHeight(ZuiCtx* zui, f32 height)
{
	ZuiWidget& parent = zuiWidgetTreeGetCurrentParent(&zui->currTree());
	sfz_assert(parent.widget_type_id == ZUI_LIST_ID);
	ZuiListData* data = parent.data<ZuiListData>();
	data->widget_height_override = height;
}

void zuiListEnd(ZuiCtx* zui)
{
	zuiCtxPopWidgetParent(zui, ZUI_LIST_ID);
}

// Text widget
// ------------------------------------------------------------------------------------------------

struct ZuiTextfmtData final {
	ZuiAlign align;
	SfzStr320 text;
};

static void textfmtDrawDefault(
	ZuiCtx* zui,
	const ZuiWidget* w,
	const SfzMat44* surf_to_clip)
{
	const ZuiTextfmtData& data = *w->data<ZuiTextfmtData>();

	// Check attributes
	ZuiID default_font_id = zui->attribs.get(DEFAULT_FONT_ATTRIB_ID.id)->as<ZuiID>();
	f32x4 font_color = zui->attribs.get(FONT_COLOR.id)->as<f32x4>();

	f32 x_pos = w->base.box.center().x;
	if (data.align == ZUI_MID_LEFT) x_pos = w->base.box.min.x;
	else if (data.align == ZUI_MID_RIGHT) x_pos = w->base.box.max.x;

	const SfzMat44 transform =
		*surf_to_clip * sfzMat44Translation3(f32x3_init(x_pos, w->base.box.center().y, 0.0f));
	f32 font_size = w->base.box.dims().y;
	zuiDrawText(&zui->draw_ctx, transform, data.align, default_font_id, font_size, font_color, data.text.str);
}

void zuiTextfmt(ZuiCtx* zui, ZuiID id, ZuiAlign align, const char* fmt, ...)
{
	ZuiWidget* w = zuiCtxCreateWidget<ZuiTextfmtData>(zui, id, ZUI_TEXT_ID);
	ZuiTextfmtData* data = w->data<ZuiTextfmtData>();
	sfz_assert(align == ZUI_MID_CENTER || align == ZUI_MID_LEFT || align == ZUI_MID_RIGHT);
	data->align = align;
	sfzStr320Clear(&data->text);
	va_list args;
	va_start(args, fmt);
	sfzStr320VAppendf(&data->text, fmt, args);
	va_end(args);
}

// Rectangle widget
// ------------------------------------------------------------------------------------------------

struct ZuiRectData final {
	f32x4 srgb_color = f32x4_splat(1.0f);
	f32 border_width = 0.0f;
};

static void rectDrawDefault(
	ZuiCtx* zui,
	const ZuiWidget* widget,
	const SfzMat44* surf_to_clip)
{
	const ZuiRectData& data = *widget->data<ZuiRectData>();
	const SfzMat44 transform =
		*surf_to_clip * sfzMat44Translation3(f32x3_init2(widget->base.box.center(), 0.0f));
	if (data.border_width <= 0.0f) {
		zuiDrawRect(&zui->draw_ctx, transform, widget->base.box.dims(), data.srgb_color);
	}
	else {
		zuiDrawBorder(&zui->draw_ctx, transform, widget->base.box.dims(), data.border_width, data.srgb_color);
	}
}

void zuiRect(ZuiCtx* zui, ZuiID id, f32x4 srgb_color)
{
	ZuiWidget* w = zuiCtxCreateWidget<ZuiRectData>(zui, id, ZUI_RECT_ID);
	ZuiRectData* data = w->data<ZuiRectData>();

	// Store data
	data->srgb_color = srgb_color;
	data->border_width = 0.0f;

	// Rectangle can't be activated
	w->base.activated = false;
}

void zuiRectBorder(ZuiCtx* zui, ZuiID id, f32 border_width, f32x4 srgb_color)
{
	ZuiWidget* w = zuiCtxCreateWidget<ZuiRectData>(zui, id, ZUI_RECT_ID);
	ZuiRectData* data = w->data<ZuiRectData>();

	// Store data
	data->srgb_color = srgb_color;
	sfz_assert(0.0f < border_width);
	data->border_width = border_width;

	// Rectangle can't be activated
	w->base.activated = false;
}

// Image widget
// ------------------------------------------------------------------------------------------------

struct ZuiImageData final {
	u64 image_handle = 0;
};

static void imageDrawDefault(
	ZuiCtx* zui,
	const ZuiWidget* widget,
	const SfzMat44* surf_to_clip)
{
	const ZuiImageData& data = *widget->data<ZuiImageData>();
	const SfzMat44 transform =
		*surf_to_clip * sfzMat44Translation3(f32x3_init2(widget->base.box.center(), 0.0f));
	zuiDrawImage(&zui->draw_ctx, transform, widget->base.box.dims(), data.image_handle);
}

void zuiImage(ZuiCtx* zui, ZuiID id, u64 image_handle)
{
	ZuiWidget* w = zuiCtxCreateWidget<ZuiImageData>(zui, id, ZUI_IMAGE_ID);
	ZuiImageData* data = w->data<ZuiImageData>();

	// Store data
	data->image_handle = image_handle;

	// Image can't be activated
	w->base.activated = false;
}

// Button widget
// ------------------------------------------------------------------------------------------------

struct ZuiButtonData final {
	SfzStr96 text;
};

static void buttonDrawDefault(
	ZuiCtx* zui,
	const ZuiWidget* widget,
	const SfzMat44* surf_to_clip)
{
	const ZuiButtonData& data = *widget->data<ZuiButtonData>();

	// Check attributes
	ZuiID default_font_id = zui->attribs.get(DEFAULT_FONT_ATTRIB_ID.id)->as<ZuiID>();
	f32x4 font_color = zui->attribs.get(FONT_COLOR.id)->as<f32x4>();
	f32x4 base_color = zui->attribs.get(BASE_COLOR.id)->as<f32x4>();
	f32x4 focus_color = zui->attribs.get(FOCUS_COLOR.id)->as<f32x4>();
	f32x4 activate_color = zui->attribs.get(ACTIVATE_COLOR.id)->as<f32x4>();
	f32 text_scaling = zui->attribs.get(BUTTON_TEXT_SCALING.id)->as<f32>();
	f32 border_width = zui->attribs.get(BUTTON_BORDER_WIDTH.id)->as<f32>();

	f32x4 color = base_color;
	if (widget->base.focused) {
		color = focus_color;
	}
	else if (widget->base.time_since_focus_ended_secs < 0.25f) {
		color = sfz::lerp(focus_color, base_color, widget->base.time_since_focus_ended_secs * 4.0f);
	}

	if (widget->base.activated) {
		color = activate_color;
	}
	else if (widget->base.time_since_activation_secs < 1.0f) {
		color = sfz::lerp(activate_color, color, widget->base.time_since_activation_secs);
	}

	const SfzMat44 transform =
		*surf_to_clip * sfzMat44Translation3(f32x3_init2(widget->base.box.center(), 0.0f));
	zuiDrawBorder(&zui->draw_ctx, transform, widget->base.box.dims(), border_width, color);
	f32 text_size = widget->base.box.dims().y * text_scaling;
	zuiDrawText(&zui->draw_ctx, transform, ZUI_MID_CENTER, default_font_id, text_size, base_color, data.text.str);
}

bool zuiButton(ZuiCtx* zui, ZuiID id, const char* text)
{
	ZuiWidget* w = zuiCtxCreateWidget<ZuiButtonData>(zui, id, ZUI_BUTTON_ID);
	ZuiButtonData* data = w->data<ZuiButtonData>();
	data->text = sfzStr96Init(text);
	const bool was_activated = w->base.activated;
	w->base.activated = false;
	return was_activated;
}

// Expandable button widget
// ------------------------------------------------------------------------------------------------

struct ZuiExpButtonData final {
	SfzStr96 text;
};

static void zuiExpButtonDrawDefault(
	ZuiCtx* zui,
	const ZuiWidget* widget,
	const SfzMat44* surf_to_clip)
{
	const ZuiExpButtonData& data = *widget->data<ZuiExpButtonData>();

	// Check attributes
	ZuiID default_font_id = zui->attribs.get(DEFAULT_FONT_ATTRIB_ID.id)->as<ZuiID>();
	f32x4 font_color = zui->attribs.get(FONT_COLOR.id)->as<f32x4>();
	f32x4 base_color = zui->attribs.get(BASE_COLOR.id)->as<f32x4>();
	f32x4 focus_color = zui->attribs.get(FOCUS_COLOR.id)->as<f32x4>();
	f32x4 activate_color = zui->attribs.get(ACTIVATE_COLOR.id)->as<f32x4>();
	f32 text_scaling = zui->attribs.get(BUTTON_TEXT_SCALING.id)->as<f32>();
	f32 border_width = zui->attribs.get(BUTTON_BORDER_WIDTH.id)->as<f32>();

	f32x4 color = base_color;
	if (widget->base.focused) {
		color = focus_color;
	}
	else if (widget->base.time_since_focus_ended_secs < 0.25f) {
		color = sfz::lerp(focus_color, base_color, widget->base.time_since_focus_ended_secs * 4.0f);
	}

	if (widget->base.activated) {
		color = activate_color;
	}
	else if (widget->base.time_since_activation_secs < 1.0f) {
		color = sfz::lerp(activate_color, color, widget->base.time_since_activation_secs);
	}

	const SfzMat44 transform =
		*surf_to_clip * sfzMat44Translation3(f32x3_init2(widget->base.box.center(), 0.0f));
	zuiDrawBorder(&zui->draw_ctx, transform, widget->base.box.dims(), border_width, color);
	f32 text_size = widget->base.box.dims().y * text_scaling;
	zuiDrawText(&zui->draw_ctx, transform, ZUI_MID_CENTER, default_font_id, text_size, base_color, data.text.str);
}

bool zuiExpButton(ZuiCtx* zui, ZuiID id, const char* text, f32x2 expPos, ZuiAlign expAlign, f32x2 expDims)
{
	ZuiWidget* w = zuiCtxCreateWidget<ZuiExpButtonData>(zui, id, ZUI_EXP_BUTTON_ID);
	ZuiButtonData* data = w->data<ZuiButtonData>();
	data->text = sfzStr96Init(text);
	if (w->base.activated) zuiCtxSetInputLock(zui, id);
	w->base.activated = false;
	const bool is_input_locked = zuiCtxIsInputLocked(zui, id);
	if (is_input_locked) {
		zuiWidgetTreePushMakeParent(&zui->currTree(), w, 64);
		zuiBasePopupBegin(zui, ZUI_INNER(id), expPos, expAlign, expDims);
	}
	return is_input_locked;
}

void zuiExpButtonEnd(ZuiCtx* zui)
{
	zuiBaseEnd(zui);
	zuiWidgetTreePopParent(&zui->currTree(), ZUI_EXP_BUTTON_ID);
}

// Checkbox widget
// ------------------------------------------------------------------------------------------------

struct ZuiCheckboxData final {
	SfzStr96 text;
	bool is_set;
};

static void zuiCheckboxDrawDefault(
	ZuiCtx* zui,
	const ZuiWidget* widget,
	const SfzMat44* surf_to_clip)
{
	const ZuiCheckboxData& data = *widget->data<ZuiCheckboxData>();

	// Check attributes
	const ZuiID default_font_id = zui->attribs.get(DEFAULT_FONT_ATTRIB_ID.id)->as<ZuiID>();
	const f32x4 base_color = zui->attribs.get(BASE_COLOR.id)->as<f32x4>();
	const f32x4 focus_color = zui->attribs.get(FOCUS_COLOR.id)->as<f32x4>();
	const f32x4 activate_color = zui->attribs.get(ACTIVATE_COLOR.id)->as<f32x4>();
	const f32 text_scaling = zui->attribs.get(BUTTON_TEXT_SCALING.id)->as<f32>();
	const f32 border_width = zui->attribs.get(BUTTON_BORDER_WIDTH.id)->as<f32>();
	const f32 innerPadding = zui->attribs.get(BUTTON_INNER_PADDING.id)->as<f32>();

	f32x4 color = base_color;
	if (widget->base.focused) {
		color = focus_color;
	}
	else if (widget->base.time_since_focus_ended_secs < 0.25f) {
		color = sfz::lerp(focus_color, base_color, widget->base.time_since_focus_ended_secs * 4.0f);
	}

	if (widget->base.activated) {
		color = activate_color;
	}
	else if (widget->base.time_since_activation_secs < 1.0f) {
		color = sfz::lerp(activate_color, color, widget->base.time_since_activation_secs);
	}

	const SfzMat44 transform =
		*surf_to_clip * sfzMat44Translation3(f32x3_init2(widget->base.box.center(), 0.0f));
	zuiDrawBorder(&zui->draw_ctx, transform, widget->base.box.dims(), border_width, color);
	f32 text_size = widget->base.box.dims().y * text_scaling;

	const f32x2 text_pos = f32x2_init(
		widget->base.box.min.x + border_width + innerPadding, widget->base.box.center().y);
	const SfzMat44 text_x_form = *surf_to_clip * sfzMat44Translation3(f32x3_init2(text_pos, 0.0f));
	zuiDrawText(&zui->draw_ctx, text_x_form, ZUI_MID_LEFT, default_font_id, text_size, base_color, data.text.str);

	const f32 check_dim = widget->base.box.dims().y - border_width * 2.0f - innerPadding * 2.0f;
	const f32 check_thickness = border_width;
	const f32x2 check_pos = f32x2_init(
		widget->base.box.max.x - border_width - innerPadding - check_dim * 0.5f, widget->base.box.center().y);
	const SfzMat44 check_x_form = *surf_to_clip * sfzMat44Translation3(f32x3_init2(check_pos, 0.0f));
	if (data.is_set) {
		zuiDrawRect(&zui->draw_ctx, check_x_form, f32x2_splat(check_dim), color);
	}
	else {
		zuiDrawBorder(&zui->draw_ctx, check_x_form, f32x2_splat(check_dim), check_thickness, color);
	}
}

bool zuiCheckbox(ZuiCtx* zui, ZuiID id, const char* text, bool* is_set)
{
	ZuiWidget* w = zuiCtxCreateWidget<ZuiCheckboxData>(zui, id, ZUI_CHECKBOX_ID);
	ZuiCheckboxData* data = w->data<ZuiCheckboxData>();
	data->text = sfzStr96Init(text);
	const bool was_activated = w->base.activated;
	if (was_activated) {
		*is_set = !*is_set;
		w->base.activated = false;
	}
	data->is_set = *is_set;
	return was_activated;
}

// Integer select widget
// ------------------------------------------------------------------------------------------------

struct ZuiIntSelectData final {
	SfzStr96 text;
	ZuiBox split_box;
	bool editing;
	i32 val;
};

static void zuiIntSelectGetNextWidgetBox(ZuiWidget* w, ZuiBox* box_out)
{
	// Note: This function is only used when the int selector is active and input locked. It is
	//       then exclusively used to give the internal split container some space.
	ZuiIntSelectData& data = *w->data<ZuiIntSelectData>();
	*box_out = data.split_box;
}

static void zuiIntSelectDrawDefault(
	ZuiCtx* zui,
	const ZuiWidget* widget,
	const SfzMat44* surf_to_clip)
{
	const ZuiIntSelectData& data = *widget->data<ZuiIntSelectData>();

	// Check attributes
	const ZuiID default_font_id = zui->attribs.get(DEFAULT_FONT_ATTRIB_ID.id)->as<ZuiID>();
	const f32x4 base_color = zui->attribs.get(BASE_COLOR.id)->as<f32x4>();
	const f32x4 focus_color = zui->attribs.get(FOCUS_COLOR.id)->as<f32x4>();
	const f32x4 activate_color = zui->attribs.get(ACTIVATE_COLOR.id)->as<f32x4>();
	const f32 text_scaling = zui->attribs.get(BUTTON_TEXT_SCALING.id)->as<f32>();
	const f32 border_width = zui->attribs.get(BUTTON_BORDER_WIDTH.id)->as<f32>();
	const f32 innerPadding = zui->attribs.get(BUTTON_INNER_PADDING.id)->as<f32>();

	f32x4 color = base_color;
	if (widget->base.focused) {
		color = focus_color;
	}
	else if (widget->base.time_since_focus_ended_secs < 0.25f) {
		color = sfz::lerp(focus_color, base_color, widget->base.time_since_focus_ended_secs * 4.0f);
	}

	if (widget->base.activated) {
		color = activate_color;
	}
	else if (widget->base.time_since_activation_secs < 1.0f) {
		color = sfz::lerp(activate_color, color, widget->base.time_since_activation_secs);
	}

	const SfzMat44 transform =
		*surf_to_clip * sfzMat44Translation3(f32x3_init2(widget->base.box.center(), 0.0f));
	zuiDrawBorder(&zui->draw_ctx, transform, widget->base.box.dims(), border_width, color);
	f32 text_size = widget->base.box.dims().y * text_scaling;

	const f32x2 text_pos = f32x2_init(
		widget->base.box.min.x + border_width + innerPadding, widget->base.box.center().y);
	const SfzMat44 text_x_form = *surf_to_clip * sfzMat44Translation3(f32x3_init2(text_pos, 0.0f));
	zuiDrawText(&zui->draw_ctx, text_x_form, ZUI_MID_LEFT, default_font_id, text_size, base_color, data.text.str);

	f32 num_pos_max_x = widget->base.box.max.x - border_width - innerPadding;
	if (data.editing) num_pos_max_x = data.split_box.min.x - innerPadding;
	const f32x2 num_pos = f32x2_init(num_pos_max_x, widget->base.box.center().y);
	const SfzMat44 num_x_form = *surf_to_clip * sfzMat44Translation3(f32x3_init2(num_pos, 0.0f));
	SfzStr32 num_str = sfzStr32InitFmt("%i", data.val);
	zuiDrawText(&zui->draw_ctx, num_x_form, ZUI_MID_RIGHT, default_font_id, text_size, base_color, num_str.str);
}

bool zuiIntSelect(ZuiCtx* zui, ZuiID id, const char* text, i32* val, i32 min, i32 max, i32 step)
{
	ZuiWidget* w = zuiCtxCreateWidget<ZuiIntSelectData>(zui, id, ZUI_INT_SELECT_ID);
	ZuiIntSelectData* data = w->data<ZuiIntSelectData>();
	data->text = sfzStr96Init(text);
	data->editing = false;
	const bool was_activated = w->base.activated;
	if (was_activated) {
		w->base.activated = false;
		zuiCtxSetInputLock(zui, id);
	}

	bool value_modified = false;
	const bool is_input_locked = zuiCtxIsInputLocked(zui, id);
	if (is_input_locked) {
		zuiWidgetTreePushMakeParent(&zui->currTree(), w, 64);

		const f32x2 base_dims = w->base.box.dims();
		const f32 h = base_dims.y;
		const f32x2 split_dims = f32x2_init(h * 2.0f, h);
		data->editing = true;
		data->split_box = zuiBoxInit(w->base.box.max - split_dims * 0.5f, split_dims);
		zuiSplit(zui, ZUI_INNER(id), false, h, h * 0.15f);

		if (zuiButton(zui, ZUI_INNER(id), "<")) {
			const i32 new_val = i32_clamp(*val - step, min, max);
			*val = new_val;
			value_modified = true;
		}

		if (zuiButton(zui, ZUI_INNER(id), ">")) {
			const i32 new_val = i32_clamp(*val + step, min, max);
			*val = new_val;
			value_modified = true;
		}

		zuiSplitEnd(zui);
		zuiWidgetTreePopParent(&zui->currTree(), ZUI_INT_SELECT_ID);
	}

	data->val = *val;
	return value_modified;
}

// Float select widget
// ------------------------------------------------------------------------------------------------

struct ZuiFloatSelectData final {
	SfzStr96 text;
	ZuiBox split_box;
	bool editing;
	f32 val;
};

static void zuiFloatSelectGetNextWidgetBox(ZuiWidget* w, ZuiBox* box_out)
{
	// Note: This function is only used when the int selector is active and input locked. It is
	//       then exclusively used to give the internal split container some space.
	ZuiFloatSelectData& data = *w->data<ZuiFloatSelectData>();
	*box_out = data.split_box;
}

static void zuiFloatSelectDrawDefault(
	ZuiCtx* zui,
	const ZuiWidget* widget,
	const SfzMat44* surf_to_clip)
{
	const ZuiFloatSelectData& data = *widget->data<ZuiFloatSelectData>();

	// Check attributes
	const ZuiID default_font_id = zui->attribs.get(DEFAULT_FONT_ATTRIB_ID.id)->as<ZuiID>();
	const f32x4 base_color = zui->attribs.get(BASE_COLOR.id)->as<f32x4>();
	const f32x4 focus_color = zui->attribs.get(FOCUS_COLOR.id)->as<f32x4>();
	const f32x4 activate_color = zui->attribs.get(ACTIVATE_COLOR.id)->as<f32x4>();
	const f32 text_scaling = zui->attribs.get(BUTTON_TEXT_SCALING.id)->as<f32>();
	const f32 border_width = zui->attribs.get(BUTTON_BORDER_WIDTH.id)->as<f32>();
	const f32 innerPadding = zui->attribs.get(BUTTON_INNER_PADDING.id)->as<f32>();

	f32x4 color = base_color;
	if (widget->base.focused) {
		color = focus_color;
	}
	else if (widget->base.time_since_focus_ended_secs < 0.25f) {
		color = sfz::lerp(focus_color, base_color, widget->base.time_since_focus_ended_secs * 4.0f);
	}

	if (widget->base.activated) {
		color = activate_color;
	}
	else if (widget->base.time_since_activation_secs < 1.0f) {
		color = sfz::lerp(activate_color, color, widget->base.time_since_activation_secs);
	}

	const SfzMat44 transform =
		*surf_to_clip * sfzMat44Translation3(f32x3_init2(widget->base.box.center(), 0.0f));
	zuiDrawBorder(&zui->draw_ctx, transform, widget->base.box.dims(), border_width, color);
	f32 text_size = widget->base.box.dims().y * text_scaling;

	const f32x2 text_pos = f32x2_init(
		widget->base.box.min.x + border_width + innerPadding, widget->base.box.center().y);
	const SfzMat44 text_x_form = *surf_to_clip * sfzMat44Translation3(f32x3_init2(text_pos, 0.0f));
	zuiDrawText(&zui->draw_ctx, text_x_form, ZUI_MID_LEFT, default_font_id, text_size, base_color, data.text.str);

	f32 num_pos_max_x = widget->base.box.max.x - border_width - innerPadding;
	if (data.editing) num_pos_max_x = data.split_box.min.x - innerPadding;
	const f32x2 num_pos = f32x2_init(num_pos_max_x, widget->base.box.center().y);
	const SfzMat44 num_x_form = *surf_to_clip * sfzMat44Translation3(f32x3_init2(num_pos, 0.0f));
	SfzStr32 num_str = sfzStr32InitFmt("%.2f", data.val);
	zuiDrawText(&zui->draw_ctx, num_x_form, ZUI_MID_RIGHT, default_font_id, text_size, base_color, num_str.str);
}

bool zuiFloatSelect(ZuiCtx* zui, ZuiID id, const char* text, f32* val, f32 min, f32 max, f32 step)
{
	ZuiWidget* w = zuiCtxCreateWidget<ZuiFloatSelectData>(zui, id, ZUI_FLOAT_SELECT_ID);
	ZuiFloatSelectData* data = w->data<ZuiFloatSelectData>();
	data->text = sfzStr96Init(text);
	data->editing = false;
	const bool was_activated = w->base.activated;
	if (was_activated) {
		w->base.activated = false;
		zuiCtxSetInputLock(zui, id);
	}

	bool value_modified = false;
	const bool is_input_locked = zuiCtxIsInputLocked(zui, id);
	if (is_input_locked) {
		zuiWidgetTreePushMakeParent(&zui->currTree(), w, 64);

		const f32x2 base_dims = w->base.box.dims();
		const f32 h = base_dims.y;
		const f32x2 split_dims = f32x2_init(h * 2.0f, h);
		data->editing = true;
		data->split_box = zuiBoxInit(w->base.box.max - split_dims * 0.5f, split_dims);
		zuiSplit(zui, ZUI_INNER(id), false, h, h * 0.15f);

		if (zuiButton(zui, ZUI_INNER(id), "<")) {
			const f32 new_val = f32_clamp(*val - step, min, max);
			*val = new_val;
			value_modified = true;
		}

		if (zuiButton(zui, ZUI_INNER(id), ">")) {
			const f32 new_val = f32_clamp(*val + step, min, max);
			*val = new_val;
			value_modified = true;
		}

		zuiSplitEnd(zui);
		zuiWidgetTreePopParent(&zui->currTree(), ZUI_FLOAT_SELECT_ID);
	}

	data->val = *val;
	return value_modified;
}

// Built-in widgets
// ------------------------------------------------------------------------------------------------

static void zuiInputInitRootAsBase(ZuiCtx* zui, f32x2 dims)
{
	ZuiWidgetTree& curr_tree = zui->currTree();
	curr_tree.root.id = ZUI_LINE;
	curr_tree.root.widget_type_id = ZUI_BASE_ID;
	curr_tree.root.data_ptr = sfz_new<ZuiBaseConData>(curr_tree.arena.getArena(), sfz_dbg(""));
	zuiWidgetTreePushMakeParent(&curr_tree, &curr_tree.root);
	ZuiWidget& root = zuiWidgetTreeGetCurrentParent(&curr_tree);
	ZuiBaseConData& root_data = *root.data<ZuiBaseConData>();
	root.base.box = zuiBoxInit(dims * 0.5f, dims);
	root_data.next_pos = dims * 0.5f;
	root_data.next_dims = dims;
}

static void zuiCtxInitBuiltInWidgets(ZuiCtx* zui)
{
	// Register attributes
	{
		zuiAttribRegisterDefault(zui, "font_color", zuiAttribInit(f32x4_splat(1.0f)));
		zuiAttribRegisterDefault(zui, "base_color", zuiAttribInit(f32x4_splat(1.0f)));
		zuiAttribRegisterDefault(zui, "focus_color", zuiAttribInit(f32x4_init(0.8f, 0.3f, 0.3f, 1.0f)));
		zuiAttribRegisterDefault(zui, "activate_color", zuiAttribInit(f32x4_init(1.0f, 0.0f, 0.0f, 1.0f)));

		zuiAttribRegisterDefault(zui, "button_text_scaling", zuiAttribInit(1.0f));
		zuiAttribRegisterDefault(zui, "button_border_width", zuiAttribInit(1.0f));
		zuiAttribRegisterDefault(zui, "button_inner_padding", zuiAttribInit(1.0f));
	}

	// Base container
	{
		ZuiWidgetDesc desc = {};
		desc.widget_data_size_bytes = sizeof(ZuiBaseConData);
		desc.focuseable = true;
		desc.activateable = false;
		desc.skip_clipping = true;
		desc.get_next_widget_box_func = baseGetNextWidgetBox;
		desc.get_next_child_idx_func = zuiWidgetGetNextChildIdxDefault;
		zuiCtxRegisterWidget(zui, ZUI_BASE_WIDGET, &desc);
	}

	// Split container
	{
		ZuiWidgetDesc desc = {};
		desc.widget_data_size_bytes = sizeof(ZuiSplitData);
		desc.focuseable = false;
		desc.activateable = false;
		desc.get_next_widget_box_func = zuiSplitGetNextWidgetBox;
		desc.get_next_child_idx_func = zuiSplitGetNextChildIdx;
		zuiCtxRegisterWidget(zui, ZUI_SPLIT_WIDGET, &desc);
	}

	// List container
	{
		ZuiWidgetDesc desc = {};
		desc.widget_data_size_bytes = sizeof(ZuiListData);
		desc.focuseable = false;
		desc.activateable = false;
		desc.get_next_widget_box_func = listGetNextWidgetBox;
		desc.get_next_child_idx_func = zuiWidgetGetNextChildIdxDefault;
		desc.scroll_input_func = listScrollInput;
		desc.scroll_set_child_visible_func = listScrollSetChildVisible;
		zuiCtxRegisterWidget(zui, ZUI_LIST_WIDGET, &desc);
	}

	// Textfmt
	{
		ZuiWidgetDesc desc = {};
		desc.widget_data_size_bytes = sizeof(ZuiTextfmtData);
		desc.focuseable = false;
		desc.activateable = false;
		desc.get_next_child_idx_func = zuiWidgetGetNextChildIdxDefault;
		desc.scroll_input_func = nullptr;
		desc.draw_func = textfmtDrawDefault;
		zuiCtxRegisterWidget(zui, ZUI_TEXT_WIDGET, &desc);
	}

	// Rectangle
	{
		ZuiWidgetDesc desc = {};
		desc.widget_data_size_bytes = sizeof(ZuiRectData);
		desc.focuseable = false;
		desc.activateable = false;
		desc.get_next_child_idx_func = zuiWidgetGetNextChildIdxDefault;
		desc.scroll_input_func = nullptr;
		desc.draw_func = rectDrawDefault;
		zuiCtxRegisterWidget(zui, ZUI_RECT_WIDGET, &desc);
	}

	// Image
	{
		ZuiWidgetDesc desc = {};
		desc.widget_data_size_bytes = sizeof(ZuiImageData);
		desc.focuseable = false;
		desc.activateable = false;
		desc.get_next_child_idx_func = zuiWidgetGetNextChildIdxDefault;
		desc.scroll_input_func = nullptr;
		desc.draw_func = imageDrawDefault;
		zuiCtxRegisterWidget(zui, ZUI_IMAGE_WIDGET, &desc);
	}

	// Button
	{
		ZuiWidgetDesc desc = {};
		desc.widget_data_size_bytes = sizeof(ZuiButtonData);
		desc.focuseable = true;
		desc.activateable = true;
		desc.get_next_child_idx_func = zuiWidgetGetNextChildIdxDefault;
		desc.scroll_input_func = nullptr;
		desc.draw_func = buttonDrawDefault;
		zuiCtxRegisterWidget(zui, ZUI_BUTTON_WIDGET, &desc);
	}

	// Expandable button
	{
		ZuiWidgetDesc desc = {};
		desc.widget_data_size_bytes = sizeof(ZuiExpButtonData);
		desc.focuseable = true;
		desc.activateable = true;
		desc.get_next_child_idx_func = zuiWidgetGetNextChildIdxDefault;
		desc.scroll_input_func = nullptr;
		desc.draw_func = zuiExpButtonDrawDefault;
		zuiCtxRegisterWidget(zui, ZUI_EXP_BUTTON_WIDGET, &desc);
	}

	// Checkbox
	{
		ZuiWidgetDesc desc = {};
		desc.widget_data_size_bytes = sizeof(ZuiCheckboxData);
		desc.focuseable = true;
		desc.activateable = true;
		desc.get_next_child_idx_func = zuiWidgetGetNextChildIdxDefault;
		desc.scroll_input_func = nullptr;
		desc.draw_func = zuiCheckboxDrawDefault;
		zuiCtxRegisterWidget(zui, ZUI_CHECKBOX_WIDGET, &desc);
	}

	// Int select
	{
		ZuiWidgetDesc desc = {};
		desc.widget_data_size_bytes = sizeof(ZuiIntSelectData);
		desc.focuseable = true;
		desc.activateable = true;
		desc.get_next_widget_box_func = zuiIntSelectGetNextWidgetBox;
		desc.get_next_child_idx_func = zuiWidgetGetNextChildIdxDefault;
		desc.scroll_input_func = nullptr;
		desc.draw_func = zuiIntSelectDrawDefault;
		zuiCtxRegisterWidget(zui, ZUI_INT_SELECT_WIDGET, &desc);
	}

	// Float select
	{
		ZuiWidgetDesc desc = {};
		desc.widget_data_size_bytes = sizeof(ZuiFloatSelectData);
		desc.focuseable = true;
		desc.activateable = true;
		desc.get_next_widget_box_func = zuiFloatSelectGetNextWidgetBox;
		desc.get_next_child_idx_func = zuiWidgetGetNextChildIdxDefault;
		desc.scroll_input_func = nullptr;
		desc.draw_func = zuiFloatSelectDrawDefault;
		zuiCtxRegisterWidget(zui, ZUI_FLOAT_SELECT_WIDGET, &desc);
	}
}
