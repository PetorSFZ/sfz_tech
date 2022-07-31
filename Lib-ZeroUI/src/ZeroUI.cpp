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

#include "skipifzero_new.hpp"

// Base Container
// ------------------------------------------------------------------------------------------------

constexpr char ZUI_BASE_CON_NAME[] = "BASE_CON";
constexpr ZuiID ZUI_BASE_CON_ID = zuiName(ZUI_BASE_CON_NAME);

struct ZuiBaseContainerData final {
	ZuiWidgetBase base;
	sfz::Map16<u64, ZuiAttrib> newValues;
	f32x2 nextPos = f32x2(0.0f);
	ZuiAlign nextAlign;
	f32x2 nextDims = f32x2(0.0f);
};

static void baseGetNextWidgetBox(ZuiCtx* zui, ZuiWidget* widget, ZuiID childID, ZuiBox* boxOut)
{
	(void)zui;
	(void)childID;
	ZuiBaseContainerData& data = *widget->data<ZuiBaseContainerData>();
	f32x2 bottomLeftPos = data.base.box.min;
	f32x2 centerPos = calcCenterPos(data.nextPos, data.nextAlign, data.nextDims);
	f32x2 nextPos = bottomLeftPos + centerPos;
	*boxOut = zuiBoxInit(nextPos, data.nextDims);
}

static void baseHandlePointerInput(ZuiCtx* zui, ZuiWidget* widget, f32x2 pointerPosSS)
{
	for (ZuiWidget& child : widget->children) {
		child.handlePointerInput(zui, pointerPosSS);
	}
}

static void baseHandleMoveInput(ZuiCtx* zui, ZuiWidget* widget, ZuiInput* input, bool* moveActive)
{
	if (input->action == ZUI_INPUT_UP) {
		for (u32 widgetIdx = widget->children.size(); widgetIdx > 0; widgetIdx--) {
			ZuiWidget& child = widget->children[widgetIdx - 1];
			child.handleMoveInput(zui, input, moveActive);
		}
	}
	else {
		for (u32 widgetIdx = 0; widgetIdx < widget->children.size(); widgetIdx++) {
			ZuiWidget& child = widget->children[widgetIdx];
			child.handleMoveInput(zui, input, moveActive);
		}
	}
}

static void baseDraw(
	ZuiCtx* zui,
	const ZuiWidget* widget,
	const SfzMat34* surfaceTransform,
	f32 lagSinceInputEndSecs)
{
	const ZuiBaseContainerData& data = *widget->data<ZuiBaseContainerData>();

	// Set attributes and backup old ones
	sfz::Map16<u64, ZuiAttrib> backup;
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
		child.draw(zui, surfaceTransform, lagSinceInputEndSecs);
	}

	// Restore old attributes
	for (const auto& pair : backup) {
		zui->attribs.put(pair.key, pair.value);
	}
}

// Context
// ------------------------------------------------------------------------------------------------

SFZ_EXTERN_C ZuiCtx* zuiCtxInit(ZuiCfg* cfg, SfzAllocator* allocator)
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
	zui->widgetTrees[0].clear();
	zui->widgetTrees[1].clear();

	// Initialize attribute set
	zui->attribs.init(256, allocator, sfz_dbg(""));
	zui->defaultAttribs.init(256, allocator, sfz_dbg(""));

	// Initialize base container
	{
		ZuiWidgetDesc desc = {};
		desc.widgetDataSizeBytes = sizeof(ZuiBaseContainerData);
		desc.getNextWidgetBoxFunc = baseGetNextWidgetBox;
		desc.handlePointerInputFunc = baseHandlePointerInput;
		desc.handleMoveInputFunc = baseHandleMoveInput;
		desc.drawFunc = baseDraw;
		zuiRegisterWidget(zui, ZUI_BASE_CON_NAME, &desc);
	}

	// Initialize core widgets
	internalCoreWidgetsInit(zui);

	return zui;
}

SFZ_EXTERN_C void zuiCtxDestroy(ZuiCtx* zui)
{
	if (zui == nullptr) return;
	SfzAllocator* allocator = zui->heapAllocator;
	zuiInternalDrawCtxDestroy(&zui->drawCtx);
	sfz_delete(allocator, zui);
}

// Input
// ------------------------------------------------------------------------------------------------

SFZ_EXTERN_C void zuiInputBegin(ZuiCtx* zui, const ZuiInput* input)
{
	// New input, clear oldest tree to make room for new widgets
	zui->inputIdx += 1;
	ZuiWidgetTree& currTree = zui->currTree();
	currTree.clear();

	// Set active surface and clear input
	zui->input = *input;
	{
		zui->transform = sfz::mat34::identity();
		zui->inputTransform = sfz::mat34::identity();
		zui->pointerPosSS = f32x2(-F32_MAX);
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
		currTree.pushMakeParent(&currTree.root);
		ZuiWidget& root = currTree.getCurrentParent();
		ZuiBaseContainerData& rootData = *root.data<ZuiBaseContainerData>();
		rootData.base.box = zuiBoxInit(input->dims * 0.5f, input->dims);
		rootData.nextPos = input->dims * 0.5f;
		rootData.nextDims = input->dims;
	}

	// Get size of surface on framebuffer
	i32x2 dimsOnFB = input->dimsOnFB;
	if (dimsOnFB == i32x2(0)) dimsOnFB = input->fbDims;

	// Get internal size of surface
	if (sfz::eqf(input->dims, f32x2(0.0f))) {
		zui->input.dims = f32x2(dimsOnFB);
	}
	
	// Calculate transform
	const f32x3 fbToClipScale = f32x3(2.0f / f32x2(input->fbDims), 1.0f);
	const f32x3 fbToClipTransl = f32x3(-1.0f, -1.0f, 0.0f);
	const mat4 fbToClip = mat4::translation3(fbToClipTransl) * mat4::scaling3(fbToClipScale);

	f32x2 halfOffset = f32x2(0.0f);
	switch (input->alignOnFB) {
	case ZUI_BOTTOM_LEFT:
		halfOffset = f32x2(0.0f, 0.0f);
		break;
	case ZUI_BOTTOM_CENTER:
		halfOffset = f32x2(-0.5f * f32(dimsOnFB.x), 0.0f);
		break;
	case ZUI_BOTTOM_RIGHT:
		halfOffset = f32x2(-1.0f * f32(dimsOnFB.x), 0.0f);
		break;
	case ZUI_MID_LEFT:
		halfOffset = f32x2(0.0f, -0.5f * f32(dimsOnFB.y));
		break;
	case ZUI_MID_CENTER:
		halfOffset = f32x2(-0.5f * f32(dimsOnFB.x), -0.5f * f32(dimsOnFB.y));
		break;
	case ZUI_MID_RIGHT:
		halfOffset = f32x2(-1.0f * f32(dimsOnFB.x), -0.5f * f32(dimsOnFB.y));
		break;
	case ZUI_TOP_LEFT:
		halfOffset = f32x2(0.0f, -1.0f * f32(dimsOnFB.y));
		break;
	case ZUI_TOP_CENTER:
		halfOffset = f32x2(-0.5f * f32(dimsOnFB.x), -1.0f * f32(dimsOnFB.y));
		break;
	case ZUI_TOP_RIGHT:
		halfOffset = f32x2(-1.0f * f32(dimsOnFB.x), -1.0f * f32(dimsOnFB.y));
		break;
	default: sfz_assert_hard(false);
	}

	const f32x3 surfToFbScale = f32x3((1.0f / zui->input.dims) * f32x2(dimsOnFB), 1.0f);
	const f32x3 surfToFbTransl = f32x3(f32x2(input->posOnFB) + halfOffset, 0.0f);
	const mat4 surfToFb = mat4::translation3(surfToFbTransl) * mat4::scaling3(surfToFbScale);

	zui->transform = sfz::mat34(fbToClip * surfToFb);

	// Input transform
	const mat4 fbToSurf = sfz::inverse(surfToFb);
	zui->inputTransform = sfz::mat34(fbToSurf);
	zui->pointerPosSS = sfz::transformPoint(fbToSurf, f32x3(zui->input.pointerPos, 0.0f)).xy();
}

SFZ_EXTERN_C void zuiInputEnd(ZuiCtx* zui)
{
	ZuiWidgetTree& currTree = zui->currTree();

	// Update pointer if pointer move
	if (zui->input.action == ZUI_INPUT_POINTER_MOVE) {
		currTree.root.handlePointerInput(zui, zui->pointerPosSS);
	}

	else {
		ZuiInput input = zui->input;
		bool moveActive = false;
		currTree.root.handleMoveInput(zui, &input, &moveActive);
		
		// Attempt to fix some edge cases where move action wasn't consumed
		if (zuiIsMoveAction(input.action)) {
			moveActive = true;
			currTree.root.handleMoveInput(zui, &input, &moveActive);
		}
	}
}

// Rendering
// ------------------------------------------------------------------------------------------------

SFZ_EXTERN_C void zuiRender(ZuiCtx* zui, f32 lagSinceInputEndSecs)
{
	// Clear render data
	zui->drawCtx.vertices.clear();
	zui->drawCtx.indices.clear();
	zui->drawCtx.renderCmds.clear();

	// Clear attribute set and set defaults
	zui->attribs.clear();
	for (const auto& pair : zui->defaultAttribs) {
		zui->attribs.put(pair.key, pair.value);
	}

	// Draw recursively
	zui->currTree().root.draw(zui, &zui->transform, lagSinceInputEndSecs);
}

SFZ_EXTERN_C ZuiRenderDataView zuiGetRenderData(const ZuiCtx* zui)
{
	ZuiRenderDataView view = {};
	view.vertices = zui->drawCtx.vertices.data();
	view.numVertices = zui->drawCtx.vertices.size();
	view.indices = zui->drawCtx.indices.data();
	view.numIndices = zui->drawCtx.indices.size();
	view.cmds = zui->drawCtx.renderCmds.data();
	view.numCmds = zui->drawCtx.renderCmds.size();
	return view;
}

// Fonts
// ------------------------------------------------------------------------------------------------

SFZ_EXTERN_C void zuiFontSetTextureHandle(ZuiCtx* zui, u64 handle)
{
	zui->drawCtx.userFontTexHandle = handle;
}

SFZ_EXTERN_C bool zuiFontRegister(ZuiCtx* zui, const char* name, const char* ttfPath, f32 size, bool defaultFont)
{
	const ZuiID id = zuiName(name);
	bool success = zuiInternalDrawAddFont(&zui->drawCtx, id, ttfPath, size, zui->heapAllocator);
	if (defaultFont) {
		sfz_assert_hard(success);
		zuiAttribRegisterDefault(zui, "default_font", zuiAttribInit(id));
	}
	return success;
}

SFZ_EXTERN_C bool zuiHasFontTextureUpdate(const ZuiCtx* zui)
{
	return zui->drawCtx.fontImgModified;
}

SFZ_EXTERN_C SfzImageViewConst zuiGetFontTexture(ZuiCtx* zui)
{
	zui->drawCtx.fontImgModified = false;
	SfzImageViewConst view = {};
	view.rawData = zui->drawCtx.fontImg.data();
	view.type = SFZ_IMAGE_TYPE_R_U8;
	view.width = zui->drawCtx.fontImgRes;
	view.height = zui->drawCtx.fontImgRes;
	return view;
}

// Attributes
// ------------------------------------------------------------------------------------------------

SFZ_EXTERN_C void zuiAttribRegisterDefault(ZuiCtx* zui, const char* attribName, ZuiAttrib attrib)
{
	const ZuiID attribID = zuiName(attribName);
	sfz_assert(zui->defaultAttribs.get(attribID.id) == nullptr);
	zui->defaultAttribs.put(attribID.id, attrib);
}

SFZ_EXTERN_C void zuiAttribRegisterDefaultNameID(ZuiCtx* zui, const char* attribName, const char* valName)
{
	zuiAttribRegisterDefault(zui, attribName, zuiAttribInit(zuiName(valName)));
}

// Widget
// ------------------------------------------------------------------------------------------------

void ZuiWidgetBase::setFocused()
{
	if (!focused) {
		timeSinceFocusStartedSecs = 0.0f;
	}
	focused = true;
}

void ZuiWidgetBase::setUnfocused()
{
	if (focused) {
		timeSinceFocusEndedSecs = 0.0f;
	}
	focused = false;
}

void ZuiWidgetBase::setActivated()
{
	timeSinceActivationSecs = 0.0f;
	activated = true;
}

void zuiDefaultPassthroughDrawFunc(
	ZuiCtx* zui,
	const ZuiWidget* widget,
	const SfzMat34* transform,
	f32 lagSinceInputEndSecs)
{
	for (const ZuiWidget& child : widget->children) {
		child.draw(zui, transform, lagSinceInputEndSecs);
	}
}

SFZ_EXTERN_C void zuiRegisterWidget(ZuiCtx* zui, const char* name, const ZuiWidgetDesc* desc)
{
	const ZuiID nameID = zuiName(name);
	sfz_assert(zui->widgetTypes.get(nameID.id) == nullptr);
	ZuiWidgetType& type = zui->widgetTypes.put(nameID.id, {});
	sfz_assert(desc->widgetDataSizeBytes >= sizeof(ZuiWidgetBase));
	type.widgetDataSizeBytes = desc->widgetDataSizeBytes;
	type.getNextWidgetBoxFunc = desc->getNextWidgetBoxFunc;
	type.handlePointerInputFunc = desc->handlePointerInputFunc;
	type.handleMoveInputFunc = desc->handleMoveInputFunc;
	ZuiWidgetArchetype& archetype = type.archetypes.put(ZUI_DEFAULT_ID.id, {});
	archetype.drawFunc = desc->drawFunc;
	type.archetypeStack.init(64, zui->heapAllocator, sfz_dbg(""));
}

// Archetypes
// ------------------------------------------------------------------------------------------------

SFZ_EXTERN_C void zuiArchetypeRegister(ZuiCtx* zui, const char* widgetName, const char* archetypeName, ZuiDrawFunc* drawFunc)
{
	const ZuiID widgetID = zuiName(widgetName);
	ZuiWidgetType* type = zui->widgetTypes.get(widgetID.id);
	sfz_assert(type != nullptr);
	const ZuiID archetypeID = zuiName(archetypeName);
	sfz_assert(type->archetypes.get(archetypeID.id) == nullptr);
	ZuiWidgetArchetype& archetype = type->archetypes.put(archetypeID.id, {});
	archetype.drawFunc = drawFunc;
}

SFZ_EXTERN_C void zuiArchetypePush(ZuiCtx* zui, const char* widgetName, const char* archetypeName)
{
	const ZuiID widgetID = zuiName(widgetName);
	ZuiWidgetType* type = zui->widgetTypes.get(widgetID.id);
	sfz_assert(type != nullptr);
	const ZuiID archetypeID = zuiName(archetypeName);
	sfz_assert(type->archetypes.get(archetypeID.id) != nullptr);
	type->archetypeStack.add(archetypeID);
}

SFZ_EXTERN_C void zuiArchetypePop(ZuiCtx* zui, const char* widgetName)
{
	const ZuiID widgetID = zuiName(widgetName);
	ZuiWidgetType* type = zui->widgetTypes.get(widgetID.id);
	sfz_assert(type != nullptr);
	sfz_assert(type->archetypeStack.size() > 1);
	type->archetypeStack.pop();
}

// Base container widget
// ------------------------------------------------------------------------------------------------

SFZ_EXTERN_C void zuiBaseBegin(ZuiCtx* zui, ZuiID id)
{
	ZuiBaseContainerData* data = zui->createWidgetParent<ZuiBaseContainerData>(id, ZUI_BASE_CON_ID);
	
	// Set initial next widget dimensions/position to cover entire container
	data->nextDims = data->base.box.dims();
	data->nextPos = data->nextDims * 0.5f;

	// Can't activate absolute container
	data->base.activated = false;
}

SFZ_EXTERN_C void zuiBaseAttrib(ZuiCtx* zui, const char* attribName, ZuiAttrib attrib)
{
	const ZuiID attribID = zuiName(attribName);
	ZuiWidget& parent = zui->currTree().getCurrentParent();
	sfz_assert(parent.widgetTypeID == ZUI_BASE_CON_ID);
	ZuiBaseContainerData& data = *parent.data<ZuiBaseContainerData>();
	data.newValues.put(attribID.id, attrib);
	sfz_assert(data.newValues.size() <= data.newValues.capacity());
}

SFZ_EXTERN_C void zuiBaseAttribNameID(ZuiCtx* zui, const char* attribName, const char* valName)
{
	zuiBaseAttrib(zui, attribName, zuiAttribInit(zuiName(valName)));
}

SFZ_EXTERN_C void zuiBaseSetPos(ZuiCtx* zui, f32x2 pos)
{
	ZuiWidget& parent = zui->currTree().getCurrentParent();
	sfz_assert(parent.widgetTypeID == ZUI_BASE_CON_ID);
	parent.data<ZuiBaseContainerData>()->nextPos = pos;
}

SFZ_EXTERN_C void zuiBaseSetAlign(ZuiCtx* zui, ZuiAlign align)
{
	ZuiWidget& parent = zui->currTree().getCurrentParent();
	sfz_assert(parent.widgetTypeID == ZUI_BASE_CON_ID);
	parent.data<ZuiBaseContainerData>()->nextAlign = align;
}

SFZ_EXTERN_C void zuiBaseSetDims(ZuiCtx* zui, f32x2 dims)
{
	ZuiWidget& parent = zui->currTree().getCurrentParent();
	sfz_assert(parent.widgetTypeID == ZUI_BASE_CON_ID);
	parent.data<ZuiBaseContainerData>()->nextDims = dims;
}

SFZ_EXTERN_C void zuiBaseSet(ZuiCtx* zui, f32x2 pos, ZuiAlign align, f32x2 dims)
{
	zuiBaseSetPos(zui, pos);
	zuiBaseSetAlign(zui, align);
	zuiBaseSetDims(zui, dims);
}

SFZ_EXTERN_C void zuiBaseSet2(ZuiCtx* zui, f32 x, f32 y, ZuiAlign align, f32 w, f32 h)
{
	zuiBaseSet(zui, f32x2(x, y), align, f32x2(w, h));
}

SFZ_EXTERN_C void zuiBaseEnd(ZuiCtx* zui)
{
	zui->popWidgetParent(ZUI_BASE_CON_ID);
}
