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

#include "ZeroUI.h"
#include "ZeroUI_Internal.hpp"
#include "ZeroUI_Drawing.hpp"
#include "ZeroUI_CoreWidgets.hpp"

// Base Container
// ------------------------------------------------------------------------------------------------

constexpr char ZUI_BASE_CON_NAME[] = "BASE_CON";
constexpr ZuiID ZUI_BASE_CON_ID = zuiName(ZUI_BASE_CON_NAME);

struct ZuiBaseContainerData final {
	//ZuiWidgetBase base;
	SfzMap16<u64, ZuiAttrib> newValues;
	f32x2 nextPos = f32x2_splat(0.0f);
	ZuiAlign nextAlign;
	f32x2 nextDims = f32x2_splat(0.0f);
};

static void baseGetNextWidgetBox(ZuiCtx* zui, ZuiWidget* widget, ZuiID childID, ZuiBox* boxOut)
{
	(void)zui;
	(void)childID;
	ZuiBaseContainerData& data = *widget->data<ZuiBaseContainerData>();
	f32x2 bottomLeftPos = widget->base.box.min;
	f32x2 centerPos = zuiCalcCenterPos(data.nextPos, data.nextAlign, data.nextDims);
	f32x2 nextPos = bottomLeftPos + centerPos;
	*boxOut = zuiBoxInit(nextPos, data.nextDims);
}

static void baseDraw(
	ZuiCtx* zui,
	const ZuiWidget* widget,
	const SfzMat44* surfaceTransform,
	f32 lagSinceInputEndSecs)
{
	const ZuiBaseContainerData& data = *widget->data<ZuiBaseContainerData>();

	// Set attributes and backup old ones
	SfzMap16<u64, ZuiAttrib> backup;
	sfz_assert(data.newValues.size() <= data.newValues.capacity());
	for (const auto& pair : data.newValues) {

		// Backup old attribute
		ZuiAttrib* oldAttrib = zui->attribs.get(pair.key);
		if (oldAttrib != nullptr) {
			backup.put(pair.key, *oldAttrib);
		}

		// Set new one
		zui->attribs.put(pair.key, pair.value);
	}

	// Render child
	for (const ZuiWidget& child : widget->children) {
		zuiWidgetDraw(&child, zui, surfaceTransform, lagSinceInputEndSecs);
	}

	// Restore old attributes
	for (const auto& pair : backup) {
		zui->attribs.put(pair.key, pair.value);
	}
}

// Context
// ------------------------------------------------------------------------------------------------

sfz_extern_c ZuiCtx* zuiCtxInit(ZuiCfg* cfg, SfzAllocator* allocator)
{
	ZuiCtx* zui = sfz_new<ZuiCtx>(allocator, sfz_dbg(""));
	*zui = {};

	zui->heapAllocator = allocator;
	
	// Initialize draw context
	const bool drawSuccess = zuiInternalDrawCtxInit(&zui->drawCtx, cfg, zui->heapAllocator);
	if (!drawSuccess) {
		sfz_delete(allocator, zui);
		return nullptr;
	}

	// Initialize widget types
	zui->widgetTypes.init(32, allocator, sfz_dbg(""));

	// Initialize widget trees
	zui->inputIdx = 0;
	zui->widgetTrees[0].arena.init(allocator, cfg->arenaMemoryLimitBytes, sfz_dbg("ZeroUI::arena1"));
	zui->widgetTrees[1].arena.init(allocator, cfg->arenaMemoryLimitBytes, sfz_dbg("ZeroUI::arena2"));
	zuiWidgetTreeClear(&zui->widgetTrees[0]);
	zuiWidgetTreeClear(&zui->widgetTrees[1]);

	// Initialize attribute set
	zui->attribs.init(256, allocator, sfz_dbg(""));
	zui->defaultAttribs.init(256, allocator, sfz_dbg(""));

	// Initialize base container
	{
		ZuiWidgetDesc desc = {};
		desc.widgetDataSizeBytes = sizeof(ZuiBaseContainerData);
		desc.focuseable = true;
		desc.activateable = false;
		desc.getNextWidgetBoxFunc = baseGetNextWidgetBox;
		desc.getNextChildIdxFunc = zuiWidgetGetNextChildIdxDefault;
		desc.scrollInputFunc = nullptr;
		desc.drawFunc = baseDraw;
		zuiCtxRegisterWidget(zui, ZUI_BASE_CON_NAME, &desc);
	}

	// Initialize core widgets
	internalCoreWidgetsInit(zui);

	return zui;
}

sfz_extern_c void zuiCtxDestroy(ZuiCtx* zui)
{
	if (zui == nullptr) return;
	SfzAllocator* allocator = zui->heapAllocator;
	zuiInternalDrawCtxDestroy(&zui->drawCtx);
	sfz_delete(allocator, zui);
}

// Input
// ------------------------------------------------------------------------------------------------

sfz_extern_c void zuiInputBegin(ZuiCtx* zui, const ZuiInput* input)
{
	// New input, clear oldest tree to make room for new widgets
	zui->inputIdx += 1;
	ZuiWidgetTree& currTree = zui->currTree();
	zuiWidgetTreeClear(&currTree);

	// Set active surface and clear input
	zui->input = *input;
	{
		zui->transform = sfzMat44Identity();
		zui->inputTransform = sfzMat44Identity();
		zui->pointerPosSS = f32x2_splat(-F32_MAX);
	}

	// Clear all archetype stacks and set default archetype
	for (auto pair : zui->widgetTypes) {
		pair.value.archetypeStack.clear();
		pair.value.archetypeStack.add(ZUI_DEFAULT_ID);
	}

	// Setup default base container for surface as root
	{
		currTree.root.id = zuiName("root_widget");
		currTree.root.widgetTypeID = ZUI_BASE_CON_ID;
		currTree.root.dataPtr = sfz_new<ZuiBaseContainerData>(currTree.arena.getArena(), sfz_dbg(""));
		currTree.root.archetypeDrawFunc = baseDraw;
		zuiWidgetTreePushMakeParent(&currTree, &currTree.root);
		ZuiWidget& root = zuiWidgetTreeGetCurrentParent(&currTree);
		ZuiBaseContainerData& rootData = *root.data<ZuiBaseContainerData>();
		root.base.box = zuiBoxInit(input->dims * 0.5f, input->dims);
		rootData.nextPos = input->dims * 0.5f;
		rootData.nextDims = input->dims;
	}

	// Get size of surface on framebuffer
	i32x2 dimsOnFB = input->dimsOnFB;
	if (dimsOnFB == i32x2_splat(0)) dimsOnFB = input->fbDims;

	// Get internal size of surface
	if (sfz::eqf(input->dims, f32x2_splat(0.0f))) {
		zui->input.dims = f32x2_from_i32(dimsOnFB);
	}
	
	// Calculate transform
	const f32x3 fbToClipScale = f32x3_init2(2.0f / f32x2_from_i32(input->fbDims), 1.0f);
	const f32x3 fbToClipTransl = f32x3_init(-1.0f, -1.0f, 0.0f);
	const SfzMat44 fbToClip = sfzMat44Translation3(fbToClipTransl) * sfzMat44Scaling3(fbToClipScale);

	f32x2 halfOffset = f32x2_splat(0.0f);
	switch (input->alignOnFB) {
	case ZUI_BOTTOM_LEFT:
		halfOffset = f32x2_init(0.0f, 0.0f);
		break;
	case ZUI_BOTTOM_CENTER:
		halfOffset = f32x2_init(-0.5f * f32(dimsOnFB.x), 0.0f);
		break;
	case ZUI_BOTTOM_RIGHT:
		halfOffset = f32x2_init(-1.0f * f32(dimsOnFB.x), 0.0f);
		break;
	case ZUI_MID_LEFT:
		halfOffset = f32x2_init(0.0f, -0.5f * f32(dimsOnFB.y));
		break;
	case ZUI_MID_CENTER:
		halfOffset = f32x2_init(-0.5f * f32(dimsOnFB.x), -0.5f * f32(dimsOnFB.y));
		break;
	case ZUI_MID_RIGHT:
		halfOffset = f32x2_init(-1.0f * f32(dimsOnFB.x), -0.5f * f32(dimsOnFB.y));
		break;
	case ZUI_TOP_LEFT:
		halfOffset = f32x2_init(0.0f, -1.0f * f32(dimsOnFB.y));
		break;
	case ZUI_TOP_CENTER:
		halfOffset = f32x2_init(-0.5f * f32(dimsOnFB.x), -1.0f * f32(dimsOnFB.y));
		break;
	case ZUI_TOP_RIGHT:
		halfOffset = f32x2_init(-1.0f * f32(dimsOnFB.x), -1.0f * f32(dimsOnFB.y));
		break;
	default: sfz_assert_hard(false);
	}

	const f32x3 surfToFbScale = f32x3_init2((1.0f / zui->input.dims) * f32x2_from_i32(dimsOnFB), 1.0f);
	const f32x3 surfToFbTransl = f32x3_init2(f32x2_from_i32(input->posOnFB) + halfOffset, 0.0f);
	const SfzMat44 surfToFb = sfzMat44Translation3(surfToFbTransl) * sfzMat44Scaling3(surfToFbScale);

	zui->transform = fbToClip * surfToFb;

	// Input transform
	const SfzMat44 fbToSurf = sfzMat44Inverse(surfToFb);
	zui->surfToFB = surfToFb;
	zui->inputTransform = fbToSurf;
	zui->pointerPosSS = sfzMat44TransformPoint(fbToSurf, f32x3_init2(zui->input.pointerPos, 0.0f)).xy();
}

static void zuiInputKeyMoveLogic(ZuiCtx* zui, ZuiWidget* w, bool* moveActive)
{
	const ZuiWidgetType* type = zui->widgetTypes.get(w->widgetTypeID.id);
	sfz_assert(type != nullptr);
	if (type == nullptr) return;

	// If input is consumed, exit
	if (zui->input.action == ZUI_INPUT_NONE) return;

	// For leaf widgets
	if (type->focuseable && w->children.isEmpty()) {
		if (*moveActive) {
			w->base.setFocused();
			*moveActive = false;
			zui->input.action = ZUI_INPUT_NONE;
		}
		else if (w->base.focused && !*moveActive) {
			w->base.setUnfocused();
			*moveActive = true;
		}
		return;
	}

	// For parent widgets
	if (!w->children.isEmpty()) {
		w->base.setUnfocused();
		i32 childIdx = type->getNextChildIdxFunc(w, zui->input.action, -1);
		while (0 <= childIdx && childIdx < i32(w->children.size())) {
			ZuiWidget& child = w->children[childIdx];
			zuiInputKeyMoveLogic(zui, &child, moveActive);
			childIdx = type->getNextChildIdxFunc(w, zui->input.action, childIdx);
		}
	}
}

static void zuiInputPointerMoveLogic(const ZuiCtx* zui, ZuiWidget* w)
{
	const ZuiWidgetType* type = zui->widgetTypes.get(w->widgetTypeID.id);
	sfz_assert(type != nullptr);
	if (type == nullptr) return;

	if (type->focuseable && w->base.box.pointInside(zui->pointerPosSS)) {
		w->base.setFocused();
	}
	else {
		w->base.setUnfocused();
	}

	for (ZuiWidget& child : w->children) {
		zuiInputPointerMoveLogic(zui, &child);
	}
}

static void zuiInputScrollLogic(const ZuiCtx* zui, ZuiWidget* w)
{
	const ZuiWidgetType* type = zui->widgetTypes.get(w->widgetTypeID.id);
	sfz_assert(type != nullptr);
	if (type == nullptr) return;

	if (type->scrollInputFunc != nullptr) type->scrollInputFunc(w, zui->input.scroll);

	for (ZuiWidget& child : w->children) {
		zuiInputScrollLogic(zui, &child);
	}
}

static bool zuiInputActivateLogic(const ZuiCtx* zui, ZuiWidget* w)
{
	const ZuiWidgetType* type = zui->widgetTypes.get(w->widgetTypeID.id);
	sfz_assert(type != nullptr);
	if (type == nullptr) return false;

	if (type->activateable && w->base.focused && !w->base.activated) {
		w->base.setActivated();
		return true;
	}

	for (ZuiWidget& child : w->children) {
		const bool consumed = zuiInputActivateLogic(zui, &child);
		if (consumed) return true;
	}

	return false;
}

sfz_extern_c void zuiInputEnd(ZuiCtx* zui)
{
	ZuiWidgetTree& currTree = zui->currTree();

	// Handle various types of inputs
	switch (zui->input.action) {
	case ZUI_INPUT_UP:
	case ZUI_INPUT_DOWN:
	case ZUI_INPUT_LEFT:
	case ZUI_INPUT_RIGHT:
		{
			bool moveActive = false;
			zuiInputKeyMoveLogic(zui, &currTree.root, &moveActive);

			// If input wasn't consumed, try again with move already active.
			if (zui->input.action != ZUI_INPUT_NONE) {
				moveActive = true;
				zuiInputKeyMoveLogic(zui, &currTree.root, &moveActive);
			}
		}
		break;
	
	case ZUI_INPUT_POINTER_MOVE:
		zuiInputPointerMoveLogic(zui, &currTree.root);
		break;

	case ZUI_INPUT_SCROLL:
		zuiInputScrollLogic(zui, &currTree.root);
		break;

	case ZUI_INPUT_ACTIVATE:
		zuiInputActivateLogic(zui, &currTree.root);
		break;

	case ZUI_INPUT_NONE:
	default:
		// Do nothing
		break;
	}
}

// Rendering
// ------------------------------------------------------------------------------------------------

sfz_extern_c void zuiRender(ZuiCtx* zui, f32 lagSinceInputEndSecs)
{
	// Clear render data
	zui->drawCtx.vertices.clear();
	zui->drawCtx.indices.clear();
	zui->drawCtx.transforms.clear();
	zui->drawCtx.renderCmds.clear();

	// Clear clip stack and push default clip box
	zui->drawCtx.clipStack.clear();
	zui->drawCtx.clipStack.add(ZuiBox{});

	// Clear attribute set and set defaults
	zui->attribs.clear();
	for (const auto& pair : zui->defaultAttribs) {
		zui->attribs.put(pair.key, pair.value);
	}

	// Draw recursively
	zuiWidgetDraw(&zui->currTree().root, zui, &zui->transform, lagSinceInputEndSecs);

	// Fix all clip boxes so that they are in fb space instead of in surface space
	auto surfToFB = [&](f32x2 p) {
		const f32x3 tmp = sfzMat44TransformPoint(zui->surfToFB, f32x3_init2(p, 0.0f));
		return tmp.xy();
	};
	for (ZuiRenderCmd& cmd : zui->drawCtx.renderCmds) {
		if (cmd.clip.min != f32x2_splat(0.0f) && cmd.clip.max != f32x2_splat(0.0f)) {
			cmd.clip.min = surfToFB(cmd.clip.min);
			cmd.clip.max = surfToFB(cmd.clip.max);
		}
	}
}

sfz_extern_c ZuiRenderDataView zuiGetRenderData(const ZuiCtx* zui)
{
	ZuiRenderDataView view = {};
	view.vertices = zui->drawCtx.vertices.data();
	view.numVertices = zui->drawCtx.vertices.size();
	view.indices = zui->drawCtx.indices.data();
	view.numIndices = zui->drawCtx.indices.size();
	view.transforms = zui->drawCtx.transforms.data();
	view.numTransforms = zui->drawCtx.transforms.size();
	view.cmds = zui->drawCtx.renderCmds.data();
	view.numCmds = zui->drawCtx.renderCmds.size();
	view.fbDims = zui->input.fbDims;
	return view;
}

// Fonts
// ------------------------------------------------------------------------------------------------

sfz_extern_c void zuiFontSetTextureHandle(ZuiCtx* zui, u64 handle)
{
	zui->drawCtx.userFontTexHandle = handle;
}

sfz_extern_c bool zuiFontRegister(ZuiCtx* zui, const char* name, const char* ttfPath, f32 size, bool defaultFont)
{
	const ZuiID id = zuiName(name);
	bool success = zuiInternalDrawAddFont(&zui->drawCtx, id, ttfPath, size, zui->heapAllocator);
	if (defaultFont) {
		sfz_assert_hard(success);
		zuiAttribRegisterDefault(zui, "default_font", zuiAttribInit(id));
	}
	return success;
}

sfz_extern_c bool zuiHasFontTextureUpdate(const ZuiCtx* zui)
{
	return zui->drawCtx.fontImgModified;
}

sfz_extern_c SfzImageViewConst zuiGetFontTexture(ZuiCtx* zui)
{
	zui->drawCtx.fontImgModified = false;
	SfzImageViewConst view = {};
	view.rawData = zui->drawCtx.fontImg.data();
	view.type = SFZ_IMAGE_TYPE_R_U8;
	view.res = i32x2_splat(zui->drawCtx.fontImgRes);
	return view;
}

// Attributes
// ------------------------------------------------------------------------------------------------

sfz_extern_c void zuiAttribRegisterDefault(ZuiCtx* zui, const char* attribName, ZuiAttrib attrib)
{
	const ZuiID attribID = zuiName(attribName);
	sfz_assert(zui->defaultAttribs.get(attribID.id) == nullptr);
	zui->defaultAttribs.put(attribID.id, attrib);
}

sfz_extern_c void zuiAttribRegisterDefaultNameID(ZuiCtx* zui, const char* attribName, const char* valName)
{
	zuiAttribRegisterDefault(zui, attribName, zuiAttribInit(zuiName(valName)));
}

// Archetypes
// ------------------------------------------------------------------------------------------------

sfz_extern_c void zuiArchetypeRegister(ZuiCtx* zui, const char* widgetName, const char* archetypeName, ZuiDrawFunc* drawFunc)
{
	const ZuiID widgetID = zuiName(widgetName);
	ZuiWidgetType* type = zui->widgetTypes.get(widgetID.id);
	sfz_assert(type != nullptr);
	const ZuiID archetypeID = zuiName(archetypeName);
	sfz_assert(type->archetypes.get(archetypeID.id) == nullptr);
	ZuiWidgetArchetype& archetype = type->archetypes.put(archetypeID.id, {});
	archetype.drawFunc = drawFunc;
}

sfz_extern_c void zuiArchetypePush(ZuiCtx* zui, const char* widgetName, const char* archetypeName)
{
	const ZuiID widgetID = zuiName(widgetName);
	ZuiWidgetType* type = zui->widgetTypes.get(widgetID.id);
	sfz_assert(type != nullptr);
	const ZuiID archetypeID = zuiName(archetypeName);
	sfz_assert(type->archetypes.get(archetypeID.id) != nullptr);
	type->archetypeStack.add(archetypeID);
}

sfz_extern_c void zuiArchetypePop(ZuiCtx* zui, const char* widgetName)
{
	const ZuiID widgetID = zuiName(widgetName);
	ZuiWidgetType* type = zui->widgetTypes.get(widgetID.id);
	sfz_assert(type != nullptr);
	sfz_assert(type->archetypeStack.size() > 1);
	type->archetypeStack.pop();
}

// Base container widget
// ------------------------------------------------------------------------------------------------

sfz_extern_c void zuiBaseBegin(ZuiCtx* zui, ZuiID id)
{
	ZuiWidget* w = zuiCtxCreateWidgetParent<ZuiBaseContainerData>(zui, id, ZUI_BASE_CON_ID);
	ZuiBaseContainerData* data = w->data<ZuiBaseContainerData>();
	
	// Set initial next widget dimensions/position to cover entire container
	data->nextDims = w->base.box.dims();
	data->nextPos = data->nextDims * 0.5f;

	// Can't activate absolute container
	w->base.activated = false;
}

sfz_extern_c void zuiBaseAttrib(ZuiCtx* zui, const char* attribName, ZuiAttrib attrib)
{
	const ZuiID attribID = zuiName(attribName);
	ZuiWidget& parent = zuiWidgetTreeGetCurrentParent(&zui->currTree());
	sfz_assert(parent.widgetTypeID == ZUI_BASE_CON_ID);
	ZuiBaseContainerData& data = *parent.data<ZuiBaseContainerData>();
	data.newValues.put(attribID.id, attrib);
	sfz_assert(data.newValues.size() <= data.newValues.capacity());
}

sfz_extern_c void zuiBaseAttribNameID(ZuiCtx* zui, const char* attribName, const char* valName)
{
	zuiBaseAttrib(zui, attribName, zuiAttribInit(zuiName(valName)));
}

sfz_extern_c void zuiBaseSetPos(ZuiCtx* zui, f32x2 pos)
{
	ZuiWidget& parent = zuiWidgetTreeGetCurrentParent(&zui->currTree());
	sfz_assert(parent.widgetTypeID == ZUI_BASE_CON_ID);
	parent.data<ZuiBaseContainerData>()->nextPos = pos;
}

sfz_extern_c void zuiBaseSetAlign(ZuiCtx* zui, ZuiAlign align)
{
	ZuiWidget& parent = zuiWidgetTreeGetCurrentParent(&zui->currTree());
	sfz_assert(parent.widgetTypeID == ZUI_BASE_CON_ID);
	parent.data<ZuiBaseContainerData>()->nextAlign = align;
}

sfz_extern_c void zuiBaseSetDims(ZuiCtx* zui, f32x2 dims)
{
	ZuiWidget& parent = zuiWidgetTreeGetCurrentParent(&zui->currTree());
	sfz_assert(parent.widgetTypeID == ZUI_BASE_CON_ID);
	parent.data<ZuiBaseContainerData>()->nextDims = dims;
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

sfz_extern_c void zuiBaseEnd(ZuiCtx* zui)
{
	zuiCtxPopWidgetParent(zui, ZUI_BASE_CON_ID);
}
