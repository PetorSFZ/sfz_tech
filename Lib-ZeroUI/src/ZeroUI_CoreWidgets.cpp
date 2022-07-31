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

#include "ZeroUI_CoreWidgets.hpp"

#include "ZeroUI.h"
#include "ZeroUI_Internal.hpp"
#include "ZeroUI_Drawing.hpp"

using sfz::mat44;

// Attributes
// ------------------------------------------------------------------------------------------------

static constexpr ZuiID DEFAULT_FONT_ATTRIB_ID = zuiName("default_font");
static constexpr ZuiID FONT_COLOR = zuiName("font_color");
static constexpr ZuiID BASE_COLOR = zuiName("base_color");
static constexpr ZuiID FOCUS_COLOR = zuiName("focus_color");
static constexpr ZuiID ACTIVATE_COLOR = zuiName("activate_color");

static constexpr ZuiID BUTTON_TEXT_SCALING = zuiName("button_text_scaling");
static constexpr ZuiID BUTTON_BORDER_WIDTH = zuiName("button_border_width");
static constexpr ZuiID BUTTON_DISABLED_COLOR = zuiName("button_disabled_color");

// Statics
// ------------------------------------------------------------------------------------------------

static sfz::mat34 mul(const sfz::mat34& lhs, const sfz::mat34& rhs)
{
	return sfz::mat34(mat44(lhs) * mat44(rhs));
}

template<typename T>
static ZuiWidgetBase* commonGetBase(ZuiCtx* zui, void* widgetData)
{
	(void)zui;
	return &reinterpret_cast<T*>(widgetData)->base;
}

static void noPointerInput(ZuiCtx* zui, ZuiWidget* widget, f32x2 pointerPosSS)
{
	(void)zui;
	(void)widget;
	(void)pointerPosSS;
}

static void noMoveInput(ZuiCtx* zui, ZuiWidget* widget, ZuiInput* input, bool* moveActive)
{
	(void)zui;
	(void)widget;
	(void)input;
	(void)moveActive;
}

// List container
// ------------------------------------------------------------------------------------------------

static constexpr char ZUI_LIST_NAME[] = "list";
constexpr ZuiID ZUI_LIST_ID = zuiName(ZUI_LIST_NAME);

struct ZuiListData final {
	ZuiWidgetBase base = {};
	f32 widgetHeight = -F32_MAX;
	f32 vertSpacing = -F32_MAX;
	f32 currPosY = -F32_MAX;
};

static void listGetNextWidgetBox(ZuiCtx* zui, ZuiWidget* widget, ZuiID childID, ZuiBox* boxOut)
{
	(void)zui;
	(void)childID;
	ZuiListData& data = *widget->data<ZuiListData>();
	f32x2 nextPos = f32x2(data.base.box.center().x, data.currPosY);
	data.currPosY -= (data.widgetHeight + data.vertSpacing);
	*boxOut = zuiBoxInit(nextPos, f32x2(data.base.box.dims().x, data.widgetHeight));
}

static void listHandlePointerInput(ZuiCtx* zui, ZuiWidget* widget, f32x2 pointerPosSS)
{
	for (ZuiWidget& child : widget->children) {
		child.handlePointerInput(zui, pointerPosSS);
	}
}

static void listHandleMoveInput(ZuiCtx* zui, ZuiWidget* widget, ZuiInput* input, bool* moveActive)
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

void zuiListBegin(ZuiCtx* zui, ZuiID id, f32 widgetHeight, f32 vertSpacing)
{
	ZuiListData* data = zui->createWidgetParent<ZuiListData>(id, ZUI_LIST_ID);

	// Set list data from parameters
	sfz_assert(widgetHeight > 0.0f);
	data->widgetHeight = widgetHeight;
	data->vertSpacing = vertSpacing;
	if (vertSpacing <= 0.0f) data->vertSpacing = data->widgetHeight * 0.5f;

	// Calculate initial next widget y-pos
	data->currPosY =
		data->base.box.center().y +
		data->base.box.dims().y * 0.5f -
		data->widgetHeight * 0.5f;

	// Can't activate list container
	data->base.activated = false;
}

void zuiListEnd(ZuiCtx* zui)
{
	zui->popWidgetParent(ZUI_LIST_ID);
}

// Tree
// ------------------------------------------------------------------------------------------------

static constexpr char ZUI_TREE_BASE_NAME[] = "tree_base";
constexpr ZuiID ZUI_TREE_BASE_ID = zuiName(ZUI_TREE_BASE_NAME);

static constexpr char ZUI_TREE_ENTRY_NAME[] = "tree_entry";
constexpr ZuiID ZUI_TREE_ENTRY_ID = zuiName(ZUI_TREE_ENTRY_NAME);

struct ZuiTreeBaseData final {
	ZuiWidgetBase base;
	f32x2 entryDims = f32x2(-F32_MAX);
	f32 entryContWidth = -F32_MAX;
	f32 entryVertSpacing = -F32_MAX;
	f32 horizSpacing = -F32_MAX;
	f32 currPosY = -F32_MAX;
	u32 activatedEntryIdx = ~0u;
};

struct ZuiTreeEntryData final {
	ZuiWidgetBase base;
	sfz::str48 text;
	bool enabled = true;
};

static void treeBaseGetNextWidgetBox(ZuiCtx* zui, ZuiWidget* widget, ZuiID childID, ZuiBox* boxOut)
{
	(void)zui;
	// You can only place tree entries inside a tree
	sfz_assert(childID == ZUI_TREE_ENTRY_ID);

	ZuiTreeBaseData& data = *widget->data<ZuiTreeBaseData>();
	f32x2 nextPos = f32x2(data.base.box.min.x + data.entryDims.x * 0.5f, data.currPosY);
	data.currPosY -= (data.entryDims.y + data.entryVertSpacing);
	*boxOut = zuiBoxInit(nextPos, data.entryDims);
}

static void treeBaseHandlePointerInput(ZuiCtx* zui, ZuiWidget* widget, f32x2 pointerPosSS)
{
	for (ZuiWidget& child : widget->children) {
		child.handlePointerInput(zui, pointerPosSS);
	}
}

static void treeBaseHandleMoveInput(ZuiCtx* zui, ZuiWidget* widget, ZuiInput* input, bool* moveActive)
{
	ZuiTreeBaseData& data = *widget->data<ZuiTreeBaseData>();
	if (data.activatedEntryIdx != ~0u) {
		sfz_assert(data.activatedEntryIdx < widget->children.size());
		ZuiWidget& activatedEntry = widget->children[data.activatedEntryIdx];
		activatedEntry.handleMoveInput(zui, input, moveActive);
	}
	else {
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
}

void zuiTreeBegin(ZuiCtx* zui, ZuiID id, f32x2 entryDims, f32 entryVertSpacing, f32 horizSpacing)
{
	ZuiTreeBaseData* data = zui->createWidgetParent<ZuiTreeBaseData>(id, ZUI_TREE_BASE_ID);

	// Set list data from parameters
	sfz_assert(entryDims.x > 0.0f);
	sfz_assert(entryDims.y > 0.0f);
	data->entryDims = entryDims;
	data->entryVertSpacing = entryVertSpacing;
	data->horizSpacing = horizSpacing;
	if (entryVertSpacing <= 0.0f) data->entryVertSpacing = data->entryDims.y * 0.5f;
	data->activatedEntryIdx = ~0u; // Will set this to actual index when we come across an entry we activate

	// Calculate entry content width based on specified entry width and our width
	sfz_assert(data->entryDims.x < data->base.box.dims().x);
	data->entryContWidth = data->base.box.dims().x - data->entryDims.x - horizSpacing;

	// Calculate initial next widget y-pos
	data->currPosY =
		data->base.box.center().y +
		data->base.box.dims().y * 0.5f -
		data->entryDims.y * 0.5f;

	// Can't activate tree base container
	data->base.activated = false;
}

void zuiTreeEnd(ZuiCtx* zui)
{
	zui->popWidgetParent(ZUI_TREE_BASE_ID);
}

static void zuiTreeEntryGetNextWidgetBox(ZuiCtx* zui, ZuiWidget* widget, ZuiID childID, ZuiBox* boxOut)
{
	(void)childID;
	// Tree entry can only have 1 child, and it MUST be a base container.
	sfz_assert(widget->children.isEmpty());

	// Grab our parent, which must be a tree base
	ZuiWidgetTree& currTree = zui->currTree();
	sfz_assert(currTree.parentStack.size() >= 3);
	ZuiWidget& parent = *currTree.parentStack[currTree.parentStack.size() - 2];
	sfz_assert(parent.widgetTypeID == ZUI_TREE_BASE_ID);
	ZuiTreeBaseData& treeData = *parent.data<ZuiTreeBaseData>();

	// Grab information from the tree base parent
	const f32x2 basePos = treeData.base.box.center();
	const f32x2 baseDims = treeData.base.box.dims();
	const f32 baseMinX = treeData.base.box.min.x;
	const f32 entryWidth = treeData.entryDims.x;
	const f32 contWidth = treeData.entryContWidth;
	const f32 horizSpacing = treeData.horizSpacing;
	sfz_assert(sfz::eqf(entryWidth + contWidth + horizSpacing, baseDims.x));

	const f32x2 nextPos = f32x2(baseMinX + entryWidth + horizSpacing + contWidth * 0.5f, basePos.y);
	const f32x2 nextDims = f32x2(contWidth, baseDims.y);
	*boxOut = zuiBoxInit(nextPos, nextDims);
}

static void zuiTreeEntryHandlePointerInput(ZuiCtx* zui, ZuiWidget* widget, f32x2 pointerPosSS)
{
	ZuiTreeEntryData& data = *widget->data<ZuiTreeEntryData>();
	if (data.enabled && data.base.box.pointInside(pointerPosSS)) {
		data.base.setFocused();
	}
	else {
		data.base.setUnfocused();
	}

	sfz_assert(widget->children.size() <= 1);
	if (!widget->children.isEmpty()) widget->children.first().handlePointerInput(zui, pointerPosSS);
}

static void zuiTreeEntryHandleMoveInput(ZuiCtx* zui, ZuiWidget* widget, ZuiInput* input, bool* moveActive)
{
	// Don't do anything if entry is not enabled
	ZuiTreeEntryData& data = *widget->data<ZuiTreeEntryData>();
	if (!data.enabled) return;

	if (data.base.activated) {
		sfz_assert(widget->children.size() <= 1);
		if (!widget->children.isEmpty()) {
			ZuiWidget& child = widget->children.first();
			child.handleMoveInput(zui, input, moveActive);
		}
		if (input->action == ZUI_INPUT_CANCEL) {
			data.base.activated = false;
			input->action = ZUI_INPUT_NONE;
		}
	}
	else {
		if (zuiIsMoveAction(input->action)) {
			if (data.base.focused && !*moveActive) {
				data.base.setUnfocused();
				*moveActive = true;
			}
			else if (*moveActive) {
				data.base.setFocused();
				input->action = ZUI_INPUT_NONE;
				*moveActive = false;
			}
		}
	}
}

static void zuiTreeEntryDrawDefault(
	ZuiCtx* zui,
	const ZuiWidget* widget,
	const SfzMat34* surfaceTransform,
	f32 lagSinceInputEndSecs)
{
	(void)lagSinceInputEndSecs;
	const ZuiTreeEntryData& data = *widget->data<ZuiTreeEntryData>();

	// Check attributes
	ZuiID defaultFontID = zui->attribs.get(DEFAULT_FONT_ATTRIB_ID.id)->as<ZuiID>();
	f32x4 fontColor = zui->attribs.get(FONT_COLOR.id)->as<f32x4>();
	f32x4 baseColor = zui->attribs.get(BASE_COLOR.id)->as<f32x4>();
	f32x4 focusColor = zui->attribs.get(FOCUS_COLOR.id)->as<f32x4>();
	f32x4 activateColor = zui->attribs.get(ACTIVATE_COLOR.id)->as<f32x4>();
	f32 textScaling = zui->attribs.get(BUTTON_TEXT_SCALING.id)->as<f32>();
	f32 borderWidth = zui->attribs.get(BUTTON_BORDER_WIDTH.id)->as<f32>();
	f32x4 disabledColor = zui->attribs.get(BUTTON_DISABLED_COLOR.id)->as<f32x4>();

	f32x4 color = baseColor;
	if (data.base.focused) {
		color = focusColor;
	}
	else if (data.base.timeSinceFocusEndedSecs < 0.25f) {
		color = sfz::lerp(focusColor, baseColor, data.base.timeSinceFocusEndedSecs * 4.0f);
	}

	if (data.base.activated) {
		color = activateColor;
	}
	else if (data.base.timeSinceActivationSecs < 1.0f) {
		color = sfz::lerp(activateColor, color, data.base.timeSinceActivationSecs);
	}

	if (!data.enabled) {
		color = disabledColor;
	}

	sfz::mat34 transform =
		mul(*surfaceTransform, sfz::mat34::translation3(f32x3(data.base.box.center(), 0.0f)));
	zuiDrawBorder(&zui->drawCtx, transform, data.base.box.dims(), borderWidth, color);
	f32 textSize = data.base.box.dims().y * textScaling;
	zuiDrawTextCentered(&zui->drawCtx, transform, defaultFontID, textSize, baseColor, data.text.str());

	// Draw children
	for (const ZuiWidget& child : widget->children) {
		child.draw(zui, surfaceTransform, lagSinceInputEndSecs);
	}
}

bool zuiTreeCollapsableBegin(ZuiCtx* zui, ZuiID id, const char* text, bool enabled)
{
	ZuiWidget* widget = nullptr;
	ZuiTreeEntryData* data = zui->createWidget<ZuiTreeEntryData>(id, ZUI_TREE_ENTRY_ID, &widget);

	// Store data
	data->text.clear();
	data->text.appendf("%s", text);
	data->enabled = enabled;

	// Get parent and ensure it is a tree base
	ZuiWidgetTree& currTree = zui->currTree();
	ZuiWidget& parent = currTree.getCurrentParent();
	sfz_assert(parent.widgetTypeID == ZUI_TREE_BASE_ID);
	ZuiTreeBaseData& treeData = *parent.data<ZuiTreeBaseData>();

	// Set activation
	if (data->base.focused && zui->input.action == ZUI_INPUT_ACTIVATE) {

		// Deactivate old entry
		if (treeData.activatedEntryIdx != ~0u) {
			//parent.children[treeData.activatedEntryIdx].getBase(zui)->setUnfocused();
			//parent.children[treeData.activatedEntryIdx].getBase(zui)->activated = false;
		
		}

		// Active this entry
		if (!data->base.activated) {
			data->base.setActivated();
			zui->input.action = ZUI_INPUT_NONE;
		}

		// Remove our input action, we have used it up
		zui->input.action = ZUI_INPUT_NONE;
	}

	// If there is no request to activate us and there already is an activated tree entry, ensure
	// we are not activated.
	else if (treeData.activatedEntryIdx != ~0u) {
		data->base.activated = false;
	}

	// If we are activated, we must become parent until treeEntryEnd() is called
	if (data->base.activated) {
		currTree.pushMakeParent(widget);
		zuiBaseBegin(zui, ZUI_INNER(id));
	}

	// If we are activated, tell our tree base and then return the fact
	if (data->base.activated) treeData.activatedEntryIdx = parent.children.size() - 1u;
	return data->base.activated;
}

void zuiTreeCollapsableEnd(ZuiCtx* zui)
{
	zuiBaseEnd(zui);
	ZuiWidgetTree& currTree = zui->currTree();
	ZuiWidget& parent = currTree.getCurrentParent();
	sfz_assert(parent.widgetTypeID == ZUI_TREE_ENTRY_ID);
	sfz_assert(currTree.parentStack.size() > 2); // Don't remove tree base container
	currTree.popParent();
	sfz_assert(currTree.getCurrentParent().widgetTypeID == ZUI_TREE_BASE_ID);
}

bool zuiTreeButton(ZuiCtx* zui, ZuiID id, const char* text, bool enabled)
{
	ZuiTreeEntryData* data = zui->createWidget<ZuiTreeEntryData>(id, ZUI_TREE_ENTRY_ID);

	// Store data
	data->text.clear();
	data->text.appendf("%s", text);
	data->enabled = enabled;

	// If not enabled, defocus
	if (!enabled) data->base.setUnfocused();

	// Set activation
	data->base.activated = false;
	if (data->base.focused && zui->input.action == ZUI_INPUT_ACTIVATE) {
		data->base.setActivated();

		// Remove our input action, we have used it up
		zui->input.action = ZUI_INPUT_NONE;
	}

	return data->base.activated;
}

// Textfmt
// ------------------------------------------------------------------------------------------------

static constexpr char ZUI_TEXTFMT_NAME[] = "textfmt";
constexpr ZuiID ZUI_TEXTFMT_ID = zuiName(ZUI_TEXTFMT_NAME);

struct ZuiTextfmtData final {
	ZuiWidgetBase base = {};
	sfz::str256 text;
};

static void textfmtDrawDefault(
	ZuiCtx* zui,
	const ZuiWidget* widget,
	const SfzMat34* surfaceTransform,
	f32 lagSinceInputEndSecs)
{
	(void)lagSinceInputEndSecs;
	const ZuiTextfmtData& data = *widget->data<ZuiTextfmtData>();

	// Check attributes
	ZuiID defaultFontID = zui->attribs.get(DEFAULT_FONT_ATTRIB_ID.id)->as<ZuiID>();
	f32x4 fontColor = zui->attribs.get(FONT_COLOR.id)->as<f32x4>();

	sfz::mat34 transform =
		mul(*surfaceTransform, sfz::mat34::translation3(f32x3(data.base.box.center(), 0.0f)));
	f32 fontSize = data.base.box.dims().y;
	zuiDrawTextCentered(&zui->drawCtx, transform, defaultFontID, fontSize, fontColor, data.text.str());
}

void zuiTextfmt(ZuiCtx* zui, ZuiID id, const char* format, ...)
{
	ZuiTextfmtData* data = zui->createWidget<ZuiTextfmtData>(id, ZUI_TEXTFMT_ID);

	// Write text
	data->text.clear();
	va_list args;
	va_start(args, format);
	data->text.vappendf(format, args);
	va_end(args);

	// Text can't be activated
	data->base.activated = false;
}

// Rectangle
// ------------------------------------------------------------------------------------------------

static constexpr char ZUI_RECT_NAME[] = "rect";
constexpr ZuiID ZUI_RECT_ID = zuiName(ZUI_RECT_NAME);

struct ZuiRectData final {
	ZuiWidgetBase base = {};
	f32x4 linearColor = f32x4(1.0f);
};

static void rectDrawDefault(
	ZuiCtx* zui,
	const ZuiWidget* widget,
	const SfzMat34* surfaceTransform,
	f32 lagSinceInputEndSecs)
{
	(void)lagSinceInputEndSecs;
	const ZuiRectData& data = *widget->data<ZuiRectData>();
	sfz::mat34 transform =
		mul(*surfaceTransform, sfz::mat34::translation3(f32x3(data.base.box.center(), 0.0f)));
	zuiDrawRect(&zui->drawCtx, transform, data.base.box.dims(), data.linearColor);
}

void zuiRect(ZuiCtx* zui, ZuiID id, f32x4 linearColor)
{
	ZuiRectData* data = zui->createWidget<ZuiRectData>(id, ZUI_RECT_ID);

	// Store data
	data->linearColor = linearColor;

	// Rectangle can't be activated
	data->base.activated = false;
}

// Image
// ------------------------------------------------------------------------------------------------

static constexpr char ZUI_IMAGE_NAME[] = "image";
constexpr ZuiID ZUI_IMAGE_ID = zuiName(ZUI_IMAGE_NAME);

struct ZuiImageData final {
	ZuiWidgetBase base = {};
	u64 imageHandle = 0;
};

static void imageDrawDefault(
	ZuiCtx* zui,
	const ZuiWidget* widget,
	const SfzMat34* surfaceTransform,
	f32 lagSinceInputEndSecs)
{
	(void)lagSinceInputEndSecs;
	const ZuiImageData& data = *widget->data<ZuiImageData>();
	sfz::mat34 transform =
		mul(*surfaceTransform, sfz::mat34::translation3(f32x3(data.base.box.center(), 0.0f)));
	zuiDrawImage(&zui->drawCtx, transform, data.base.box.dims(), data.imageHandle);
}

void zuiImage(ZuiCtx* zui, ZuiID id, u64 imageHandle)
{
	ZuiImageData* data = zui->createWidget<ZuiImageData>(id, ZUI_IMAGE_ID);

	// Store data
	data->imageHandle = imageHandle;

	// Image can't be activated
	data->base.activated = false;
}

// Button
// ------------------------------------------------------------------------------------------------

static constexpr char ZUI_BUTTON_NAME[] = "button";
constexpr ZuiID ZUI_BUTTON_ID = zuiName(ZUI_BUTTON_NAME);

struct ZuiButtonData final {
	ZuiWidgetBase base = {};
	sfz::str48 text;
	bool enabled = true;
};

static void buttonHandlePointerInput(ZuiCtx* zui, ZuiWidget* widget, f32x2 pointerPosSS)
{
	(void)zui;
	ZuiButtonData& data = *widget->data<ZuiButtonData>();
	if (data.enabled && data.base.box.pointInside(pointerPosSS)) {
		data.base.setFocused();
	}
	else {
		data.base.setUnfocused();
	}
}

static void buttonHandleMoveInput(ZuiCtx* zui, ZuiWidget* widget, ZuiInput* input, bool* moveActive)
{
	(void)zui;
	ZuiButtonData& data = *widget->data<ZuiButtonData>();
	if (data.enabled && zuiIsMoveAction(input->action)) {
		if (data.base.focused && !*moveActive) {
			data.base.setUnfocused();
			*moveActive = true;
		}
		else if (*moveActive) {
			data.base.setFocused();
			input->action = ZUI_INPUT_NONE;
			*moveActive = false;
		}
	}
}

static void buttonDrawDefault(
	ZuiCtx* zui,
	const ZuiWidget* widget,
	const SfzMat34* surfaceTransform,
	f32 lagSinceInputEndSecs)
{
	(void)lagSinceInputEndSecs;
	const ZuiButtonData& data = *widget->data<ZuiButtonData>();

	// Check attributes
	ZuiID defaultFontID = zui->attribs.get(DEFAULT_FONT_ATTRIB_ID.id)->as<ZuiID>();
	f32x4 fontColor = zui->attribs.get(FONT_COLOR.id)->as<f32x4>();
	f32x4 baseColor = zui->attribs.get(BASE_COLOR.id)->as<f32x4>();
	f32x4 focusColor = zui->attribs.get(FOCUS_COLOR.id)->as<f32x4>();
	f32x4 activateColor = zui->attribs.get(ACTIVATE_COLOR.id)->as<f32x4>();
	f32 textScaling = zui->attribs.get(BUTTON_TEXT_SCALING.id)->as<f32>();
	f32 borderWidth = zui->attribs.get(BUTTON_BORDER_WIDTH.id)->as<f32>();
	f32x4 disabledColor = zui->attribs.get(BUTTON_DISABLED_COLOR.id)->as<f32x4>();

	f32x4 color = baseColor;
	if (data.base.focused) {
		color = focusColor;
	}
	else if (data.base.timeSinceFocusEndedSecs < 0.25f) {
		color = sfz::lerp(focusColor, baseColor, data.base.timeSinceFocusEndedSecs * 4.0f);
	}

	if (data.base.activated) {
		color = activateColor;
	}
	else if (data.base.timeSinceActivationSecs < 1.0f) {
		color = sfz::lerp(activateColor, color, data.base.timeSinceActivationSecs);
	}

	if (!data.enabled) {
		color = disabledColor;
	}

	sfz::mat34 transform =
		mul(*surfaceTransform, sfz::mat34::translation3(f32x3(data.base.box.center(), 0.0f)));
	zuiDrawBorder(&zui->drawCtx, transform, data.base.box.dims(), borderWidth, color);
	f32 textSize = data.base.box.dims().y * textScaling;
	zuiDrawTextCentered(&zui->drawCtx, transform, defaultFontID, textSize, baseColor, data.text.str());
}

bool zuiButton(ZuiCtx* zui, ZuiID id, const char* text, bool enabled)
{
	ZuiButtonData* data = zui->createWidget<ZuiButtonData>(id, ZUI_BUTTON_ID);

	// Store data
	data->text.clear();
	data->text.appendf("%s", text);
	data->enabled = enabled;

	// If not enabled, defocus
	if (!enabled) data->base.setUnfocused();

	// Set activation
	data->base.activated = false;
	if (data->base.focused && zui->input.action == ZUI_INPUT_ACTIVATE) {
		data->base.setActivated();

		// Remove our input action, we have used it up
		zui->input.action = ZUI_INPUT_NONE;
	}

	return data->base.activated;
}

// Initialization
// ------------------------------------------------------------------------------------------------

void internalCoreWidgetsInit(ZuiCtx* zui)
{
	// Register attributes
	{
		zuiAttribRegisterDefault(zui, "font_color", zuiAttribInit(f32x4(1.0f)));
		zuiAttribRegisterDefault(zui, "base_color", zuiAttribInit(f32x4(1.0f)));
		zuiAttribRegisterDefault(zui, "focus_color", zuiAttribInit(f32x4(0.8f, 0.3f, 0.3f, 1.0f)));
		zuiAttribRegisterDefault(zui, "activate_color", zuiAttribInit(f32x4(1.0f, 0.0f, 0.0f, 1.0f)));

		zuiAttribRegisterDefault(zui, "button_text_scaling", zuiAttribInit(1.0f));
		zuiAttribRegisterDefault(zui, "button_border_width", zuiAttribInit(1.0f));
		zuiAttribRegisterDefault(zui, "button_disabled_color", zuiAttribInit(f32x4(0.2f, 0.2f, 0.2f, 0.5f)));
	}

	// List container
	{
		ZuiWidgetDesc desc = {};
		desc.widgetDataSizeBytes = sizeof(ZuiListData);
		desc.getNextWidgetBoxFunc = listGetNextWidgetBox;
		desc.handlePointerInputFunc = listHandlePointerInput;
		desc.handleMoveInputFunc = listHandleMoveInput;
		desc.drawFunc = zuiDefaultPassthroughDrawFunc;
		zuiRegisterWidget(zui, ZUI_LIST_NAME, &desc);
	}

	// Tree
	{
		// Base
		{
			ZuiWidgetDesc desc = {};
			desc.widgetDataSizeBytes = sizeof(ZuiTreeBaseData);
			desc.getNextWidgetBoxFunc = treeBaseGetNextWidgetBox;
			desc.handlePointerInputFunc = treeBaseHandlePointerInput;
			desc.handleMoveInputFunc = treeBaseHandleMoveInput;
			desc.drawFunc = zuiDefaultPassthroughDrawFunc;
			zuiRegisterWidget(zui, ZUI_TREE_BASE_NAME, &desc);
		}
		
		// Entry
		{
			ZuiWidgetDesc desc = {};
			desc.widgetDataSizeBytes = sizeof(ZuiTreeEntryData);
			desc.getNextWidgetBoxFunc = zuiTreeEntryGetNextWidgetBox;
			desc.handlePointerInputFunc = zuiTreeEntryHandlePointerInput;
			desc.handleMoveInputFunc = zuiTreeEntryHandleMoveInput;
			desc.drawFunc = zuiTreeEntryDrawDefault;
			zuiRegisterWidget(zui, ZUI_TREE_ENTRY_NAME, &desc);
		}
	}

	// Textfmt
	{
		ZuiWidgetDesc desc = {};
		desc.widgetDataSizeBytes = sizeof(ZuiTextfmtData);
		desc.handlePointerInputFunc = noPointerInput;
		desc.handleMoveInputFunc = noMoveInput;
		desc.drawFunc = textfmtDrawDefault;
		zuiRegisterWidget(zui, ZUI_TEXTFMT_NAME, &desc);
	}

	// Rectangle
	{
		ZuiWidgetDesc desc = {};
		desc.widgetDataSizeBytes = sizeof(ZuiRectData);
		//desc.getWidgetBaseFunc = commonGetBase<ZuiRectData>;
		desc.handlePointerInputFunc = noPointerInput;
		desc.handleMoveInputFunc = noMoveInput;
		desc.drawFunc = rectDrawDefault;
		zuiRegisterWidget(zui, ZUI_RECT_NAME, &desc);
	}

	// Image
	{
		ZuiWidgetDesc desc = {};
		desc.widgetDataSizeBytes = sizeof(ZuiImageData);
		desc.handlePointerInputFunc = noPointerInput;
		desc.handleMoveInputFunc = noMoveInput;
		desc.drawFunc = imageDrawDefault;
		zuiRegisterWidget(zui, ZUI_IMAGE_NAME, &desc);
	}

	// Button
	{
		ZuiWidgetDesc desc = {};
		desc.widgetDataSizeBytes = sizeof(ZuiButtonData);
		desc.handlePointerInputFunc = buttonHandlePointerInput;
		desc.handleMoveInputFunc = buttonHandleMoveInput;
		desc.drawFunc = buttonDrawDefault;
		zuiRegisterWidget(zui, ZUI_BUTTON_NAME, &desc);
	}
}
