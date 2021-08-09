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

#include "ZeroUI.hpp"
#include "ZeroUI_Internal.hpp"
#include "ZeroUI_Drawing.hpp"

namespace zui {

using sfz::vec4;
using sfz::mat44;

// Attributes
// ------------------------------------------------------------------------------------------------

static constexpr strID DEFAULT_FONT_ATTRIB_ID = strID(sfz::hash("default_font"));
static constexpr strID FONT_COLOR = strID(sfz::hash("font_color"));
static constexpr strID BASE_COLOR = strID(sfz::hash("base_color"));
static constexpr strID FOCUS_COLOR = strID(sfz::hash("focus_color"));
static constexpr strID ACTIVATE_COLOR = strID(sfz::hash("activate_color"));

static constexpr strID BUTTON_TEXT_SCALING = strID(sfz::hash("button_text_scaling"));
static constexpr strID BUTTON_BORDER_WIDTH = strID(sfz::hash("button_border_width"));
static constexpr strID BUTTON_DISABLED_COLOR = strID(sfz::hash("button_disabled_color"));

// Statics
// ------------------------------------------------------------------------------------------------

static mat34 mul(const mat34& lhs, const mat34& rhs)
{
	return mat34(mat44(lhs) * mat44(rhs));
}

template<typename T>
static WidgetBase* commonGetBase(void* widgetData)
{
	return &reinterpret_cast<T*>(widgetData)->base;
}

static void noPointerInput(WidgetCmd* cmd, vec2 pointerPosSS)
{
	(void)cmd;
	(void)pointerPosSS;
}

static void noMoveInput(WidgetCmd* cmd, Input* input, bool* moveActive)
{
	(void)cmd;
	(void)input;
	(void)moveActive;
}

static void commonAddChildLogic(strID id, void* dataPtr, WidgetBase* base)
{
	Surface& surface = *ctx().activeSurface;

	// Get position and dimensions from current parent
	WidgetCmd& parent = surface.getCurrentParent();
	parent.getNextWidgetBox(id, &base->box);

	// Update timers
	base->incrementTimers(surface.desc.deltaTimeSecs);

	// Add command
	parent.children.add(WidgetCmd(id, dataPtr));
}

// List container
// ------------------------------------------------------------------------------------------------

static constexpr char LIST_NAME[] = "list";
constexpr strID LIST_ID = strID(sfz::hash(LIST_NAME));

static void listGetNextWidgetBox(WidgetCmd* cmd, strID childID, Box* boxOut)
{
	(void)childID;
	ListData& data = *cmd->data<ListData>();
	vec2 nextPos = vec2(data.base.box.center().x, data.currPosY);
	data.currPosY -= (data.widgetHeight + data.vertSpacing);
	*boxOut = Box(nextPos.x, nextPos.y, data.base.box.dims().x, data.widgetHeight);
}

static void listHandlePointerInput(WidgetCmd* cmd, vec2 pointerPosSS)
{
	for (WidgetCmd& child : cmd->children) {
		child.handlePointerInput(pointerPosSS);
	}
}

static void listHandleMoveInput(WidgetCmd* cmd, Input* input, bool* moveActive)
{
	if (input->action == InputAction::UP) {
		for (u32 cmdIdx = cmd->children.size(); cmdIdx > 0; cmdIdx--) {
			WidgetCmd& child = cmd->children[cmdIdx - 1];
			child.handleMoveInput(input, moveActive);
		}
	}
	else {
		for (u32 cmdIdx = 0; cmdIdx < cmd->children.size(); cmdIdx++) {
			WidgetCmd& child = cmd->children[cmdIdx];
			child.handleMoveInput(input, moveActive);
		}
	}
}

void listBegin(ListData* data, f32 widgetHeight, f32 vertSpacing)
{
	Surface& surface = *ctx().activeSurface;

	// Set list data from parameters
	sfz_assert(widgetHeight > 0.0f);
	data->widgetHeight = widgetHeight;
	data->vertSpacing = vertSpacing;
	if (vertSpacing <= 0.0f) data->vertSpacing = data->widgetHeight * 0.5f;

	// Get button position and dimensions from current parent
	WidgetCmd& parent = surface.getCurrentParent();
	parent.getNextWidgetBox(LIST_ID, &data->base.box);

	// Calculate initial next widget y-pos
	data->currPosY =
		data->base.box.center().y +
		data->base.box.dims().y * 0.5f -
		data->widgetHeight * 0.5f;

	// Update timers
	data->base.incrementTimers(surface.desc.deltaTimeSecs);

	// Can't activate list container
	data->base.activated = false;

	// Add command and make parent
	parent.children.add(WidgetCmd(LIST_ID, data));
	surface.pushMakeParent(&parent.children.last());
}

void listBegin(strID id, f32 widgetHeight, f32 vertSpacing)
{
	ListData* data = ctx().getWidgetData<ListData>(id);
	listBegin(data, widgetHeight, vertSpacing);
}

void listBegin(const char* id, f32 widgetHeight, f32 vertSpacing)
{
	ListData* data = ctx().getWidgetData<ListData>(id);
	listBegin(data, widgetHeight, vertSpacing);
}

void listEnd()
{
	Surface& surface = *ctx().activeSurface;
	WidgetCmd& parent = surface.getCurrentParent();
	sfz_assert(parent.widgetID == LIST_ID);
	sfz_assert(surface.cmdParentStack.size() > 1); // Don't remove default base container
	surface.popParent();
}

// Tree
// ------------------------------------------------------------------------------------------------

static constexpr char TREE_BASE_NAME[] = "tree_base";
constexpr strID TREE_BASE_ID = strID(sfz::hash(TREE_BASE_NAME));

static constexpr char TREE_ENTRY_NAME[] = "tree_entry";
constexpr strID TREE_ENTRY_ID = strID(sfz::hash(TREE_ENTRY_NAME));

static void treeBaseGetNextWidgetBox(WidgetCmd * cmd, strID childID, Box* boxOut)
{
	// You can only place tree entries inside a tree
	sfz_assert(childID == TREE_ENTRY_ID);

	TreeBaseData& data = *cmd->data<TreeBaseData>();
	vec2 nextPos = vec2(data.base.box.min.x + data.entryDims.x * 0.5f, data.currPosY);
	data.currPosY -= (data.entryDims.y + data.entryVertSpacing);
	*boxOut = Box(nextPos, data.entryDims);
}

static void treeBaseHandlePointerInput(WidgetCmd* cmd, vec2 pointerPosSS)
{
	for (WidgetCmd& child : cmd->children) {
		child.handlePointerInput(pointerPosSS);
	}
}

static void treeBaseHandleMoveInput(WidgetCmd* cmd, Input* input, bool* moveActive)
{
	TreeBaseData& data = *cmd->data<TreeBaseData>();
	if (data.activatedEntryIdx != ~0u) {
		sfz_assert(data.activatedEntryIdx < cmd->children.size());
		WidgetCmd& activatedEntry = cmd->children[data.activatedEntryIdx];
		activatedEntry.handleMoveInput(input, moveActive);
	}
	else {
		if (input->action == InputAction::UP) {
			for (u32 cmdIdx = cmd->children.size(); cmdIdx > 0; cmdIdx--) {
				WidgetCmd& child = cmd->children[cmdIdx - 1];
				child.handleMoveInput(input, moveActive);
			}
		}
		else {
			for (u32 cmdIdx = 0; cmdIdx < cmd->children.size(); cmdIdx++) {
				WidgetCmd& child = cmd->children[cmdIdx];
				child.handleMoveInput(input, moveActive);
			}
		}
	}
}

void treeBegin(TreeBaseData* data, vec2 entryDims, f32 entryVertSpacing, f32 horizSpacing)
{
	Surface& surface = *ctx().activeSurface;

	// Set list data from parameters
	sfz_assert(entryDims.x > 0.0f);
	sfz_assert(entryDims.y > 0.0f);
	data->entryDims = entryDims;
	data->entryVertSpacing = entryVertSpacing;
	data->horizSpacing = horizSpacing;
	if (entryVertSpacing <= 0.0f) data->entryVertSpacing = data->entryDims.y * 0.5f;
	data->activatedEntryIdx = ~0u; // Will set this to actual index when we come across an entry we activate

	// Get button position and dimensions from current parent
	WidgetCmd& parent = surface.getCurrentParent();
	parent.getNextWidgetBox(TREE_BASE_ID, &data->base.box);
	
	// Calculate entry content width based on specified entry width and our width
	sfz_assert(data->entryDims.x < data->base.box.width());
	data->entryContWidth = data->base.box.width() - data->entryDims.x - horizSpacing;

	// Calculate initial next widget y-pos
	data->currPosY =
		data->base.box.center().y +
		data->base.box.dims().y * 0.5f -
		data->entryDims.y * 0.5f;

	// Update timers
	data->base.incrementTimers(surface.desc.deltaTimeSecs);

	// Can't activate tree base container
	data->base.activated = false;

	// Add command and make parent
	parent.children.add(WidgetCmd(TREE_BASE_ID, data));
	surface.pushMakeParent(&parent.children.last());
}

void treeBegin(strID id, vec2 entryDims, f32 entryVertSpacing, f32 horizSpacing)
{
	treeBegin(ctx().getWidgetData<TreeBaseData>(id), entryDims, entryVertSpacing, horizSpacing);
}

void treeBegin(const char* id, vec2 entryDims, f32 entryVertSpacing, f32 horizSpacing)
{
	treeBegin(ctx().getWidgetData<TreeBaseData>(id), entryDims, entryVertSpacing, horizSpacing);
}

void treeEnd()
{
	Surface& surface = *ctx().activeSurface;
	WidgetCmd& parent = surface.getCurrentParent();
	sfz_assert(parent.widgetID == TREE_BASE_ID);
	sfz_assert(surface.cmdParentStack.size() > 1); // Don't remove default base container
	surface.popParent();
}

static void treeEntryGetNextWidgetBox(WidgetCmd* cmd, strID childID, Box* boxOut)
{
	// Tree entry can only have 1 child, and it MUST be a base container.
	sfz_assert(cmd->children.isEmpty());
	sfz_assert(childID == BASE_CON_ID);

	// Grab our parent, which must be a tree base
	Surface& surface = *ctx().activeSurface;
	sfz_assert(surface.cmdParentStack.size() >= 3);
	WidgetCmd& parent = *surface.cmdParentStack[surface.cmdParentStack.size() - 2];
	sfz_assert(parent.widgetID == TREE_BASE_ID);
	TreeBaseData& treeData = *parent.data<TreeBaseData>();

	// Grab information from the tree base parent
	const vec2 basePos = treeData.base.box.center();
	const vec2 baseDims = treeData.base.box.dims();
	const f32 baseMinX = treeData.base.box.min.x;
	const f32 entryWidth = treeData.entryDims.x;
	const f32 contWidth = treeData.entryContWidth;
	const f32 horizSpacing = treeData.horizSpacing;
	sfz_assert(sfz::eqf(entryWidth + contWidth + horizSpacing, baseDims.x));

	const vec2 nextPos = vec2(baseMinX + entryWidth + horizSpacing + contWidth * 0.5f, basePos.y);
	const vec2 nextDims = vec2(contWidth, baseDims.y);
	*boxOut = Box(nextPos, nextDims);
}

static void treeEntryHandlePointerInput(WidgetCmd* cmd, vec2 pointerPosSS)
{
	TreeEntryData& data = *cmd->data<TreeEntryData>();
	if (data.enabled && data.base.box.pointInside(pointerPosSS)) {
		data.base.setFocused();
	}
	else {
		data.base.setUnfocused();
	}

	sfz_assert(cmd->children.size() <= 1);
	if (!cmd->children.isEmpty()) cmd->children.first().handlePointerInput(pointerPosSS);
}

static void treeEntryHandleMoveInput(WidgetCmd* cmd, Input* input, bool* moveActive)
{
	// Don't do anything if entry is not enabled
	TreeEntryData& data = *cmd->data<TreeEntryData>();
	if (!data.enabled) return;

	if (data.base.activated) {
		sfz_assert(cmd->children.size() <= 1);
		if (!cmd->children.isEmpty()) {
			WidgetCmd& child = cmd->children.first();
			child.handleMoveInput(input, moveActive);
		}
		if (input->action == InputAction::CANCEL) {
			data.base.activated = false;
			input->action = InputAction::NONE;
		}
	}
	else {
		if (isMoveAction(input->action)) {
			if (data.base.focused && !*moveActive) {
				data.base.setUnfocused();
				*moveActive = true;
			}
			else if (*moveActive) {
				data.base.setFocused();
				input->action = InputAction::NONE;
				*moveActive = false;
			}
		}
	}
}

static void treeEntryDrawDefault(
	const WidgetCmd* cmd,
	AttributeSet* attributes,
	const mat34& surfaceTransform,
	f32 lagSinceSurfaceEndSecs)
{
	(void)attributes;
	(void)lagSinceSurfaceEndSecs;
	const TreeEntryData& data = *cmd->data<TreeEntryData>();

	// Check attributes
	strID defaultFontID = attributes->get(DEFAULT_FONT_ATTRIB_ID)->as<strID>();
	vec4 fontColor = attributes->get(FONT_COLOR)->as<vec4>();
	vec4 baseColor = attributes->get(BASE_COLOR)->as<vec4>();
	vec4 focusColor = attributes->get(FOCUS_COLOR)->as<vec4>();
	vec4 activateColor = attributes->get(ACTIVATE_COLOR)->as<vec4>();
	f32 textScaling = attributes->get(BUTTON_TEXT_SCALING)->as<f32>();
	f32 borderWidth = attributes->get(BUTTON_BORDER_WIDTH)->as<f32>();
	vec4 disabledColor = attributes->get(BUTTON_DISABLED_COLOR)->as<vec4>();

	vec4 color = baseColor;
	if (data.base.focused) {
		color = focusColor;
	}
	else if (data.base.timeSinceFocusEndedSecs < 0.25f) {
		color = lerp(focusColor, baseColor, data.base.timeSinceFocusEndedSecs * 4.0f);
	}

	if (data.base.activated) {
		color = activateColor;
	}
	else if (data.base.timeSinceActivationSecs < 1.0f) {
		color = lerp(activateColor, color, data.base.timeSinceActivationSecs);
	}

	if (!data.enabled) {
		color = disabledColor;
	}

	mat34 transform =
		mul(surfaceTransform, mat34::translation3(vec3(data.base.box.center(), 0.0f)));
	drawBorder(transform, data.base.box.dims(), borderWidth, color);
	f32 textSize = data.base.box.height() * textScaling;
	drawTextFmtCentered(transform, defaultFontID, textSize, baseColor, data.text.str());

	// Draw children
	for (const WidgetCmd& child : cmd->children) {
		child.draw(attributes, surfaceTransform, lagSinceSurfaceEndSecs);
	}
}

bool treeCollapsableBegin(TreeEntryData* data, const char* text, bool enabled)
{
	// Store data
	data->text.clear();
	data->text.appendf("%s", text);
	data->enabled = enabled;

	// Get parent and ensure it is a tree base
	Surface& surface = *ctx().activeSurface;
	WidgetCmd& parent = surface.getCurrentParent();
	sfz_assert(parent.widgetID == TREE_BASE_ID);
	TreeBaseData& treeData = *parent.data<TreeBaseData>();

	// Get position and dimensions from current parent
	parent.getNextWidgetBox(TREE_ENTRY_ID, &data->base.box);

	// Update timers
	data->base.incrementTimers(surface.desc.deltaTimeSecs);

	// Add command
	parent.children.add(WidgetCmd(TREE_ENTRY_ID, data));

	
	// Set activation
	if (data->base.focused && surface.desc.input.action == InputAction::ACTIVATE) {

		// Deactivate old entry
		if (treeData.activatedEntryIdx != ~0u) {
			parent.children[treeData.activatedEntryIdx].getBase()->setUnfocused();
			parent.children[treeData.activatedEntryIdx].getBase()->activated = false;
		
		}

		// Active this entry
		if (!data->base.activated) {
			data->base.setActivated();
			surface.desc.input.action = InputAction::NONE;
		}

		// Remove our input action, we have used it up
		surface.desc.input.action = InputAction::NONE;
	}

	// If there is no request to activate us and there already is an activated tree entry, ensure
	// we are not activated.
	else if (treeData.activatedEntryIdx != ~0u) {
		data->base.activated = false;
	}

	// If we are activated, we must become parent until treeEntryEnd() is called
	if (data->base.activated) {
		surface.pushMakeParent(&parent.children.last());
		zui::baseBegin(&data->baseCon);
	}

	// If we are activated, tell our tree base and then return the fact
	if (data->base.activated) treeData.activatedEntryIdx = parent.children.size() - 1u;
	return data->base.activated;
}

bool treeCollapsableBegin(strID id, const char* text, bool enabled)
{
	return treeCollapsableBegin(ctx().getWidgetData<TreeEntryData>(id), text, enabled);
}

bool treeCollapsableBegin(const char* id, const char* text, bool enabled)
{
	return treeCollapsableBegin(ctx().getWidgetData<TreeEntryData>(id), text, enabled);
}

void treeCollapsableEnd()
{
	zui::baseEnd();
	Surface& surface = *ctx().activeSurface;
	WidgetCmd& parent = surface.getCurrentParent();
	sfz_assert(parent.widgetID == TREE_ENTRY_ID);
	sfz_assert(surface.cmdParentStack.size() > 2); // Don't remove tree base container
	surface.popParent();
	sfz_assert(surface.getCurrentParent().widgetID == TREE_BASE_ID);
}

bool treeButton(TreeEntryData* data, const char* text, bool enabled)
{
	// Store data
	data->text.clear();
	data->text.appendf("%s", text);
	data->enabled = enabled;

	// Update data and add command
	commonAddChildLogic(TREE_ENTRY_ID, data, &data->base);

	// If not enabled, defocus
	if (!enabled) data->base.setUnfocused();

	// Set activation
	Surface& surface = *ctx().activeSurface;
	data->base.activated = false;
	if (data->base.focused && surface.desc.input.action == InputAction::ACTIVATE) {
		data->base.setActivated();

		// Remove our input action, we have used it up
		surface.desc.input.action = InputAction::NONE;
	}

	return data->base.activated;
}

bool treeButton(strID id, const char* text, bool enabled)
{
	return treeButton(ctx().getWidgetData<TreeEntryData>(id), text, enabled);
}

bool treeButton(const char* id, const char* text, bool enabled)
{
	return treeButton(ctx().getWidgetData<TreeEntryData>(id), text, enabled);
}

// Textfmt
// ------------------------------------------------------------------------------------------------

static constexpr char TEXTFMT_NAME[] = "textfmt";
constexpr strID TEXTFMT_ID = strID(sfz::hash(TEXTFMT_NAME));

static void textfmtDrawDefault(
	const WidgetCmd* cmd,
	AttributeSet* attributes,
	const mat34& surfaceTransform,
	f32 lagSinceSurfaceEndSecs)
{
	(void)attributes;
	(void)lagSinceSurfaceEndSecs;
	const TextfmtData& data = *cmd->data<TextfmtData>();

	// Check attributes
	strID defaultFontID = attributes->get(DEFAULT_FONT_ATTRIB_ID)->as<strID>();
	vec4 fontColor = attributes->get(FONT_COLOR)->as<vec4>();

	mat34 transform =
		mul(surfaceTransform, mat34::translation3(vec3(data.base.box.center(), 0.0f)));
	f32 fontSize = data.base.box.height();
	drawTextFmtCentered(transform, defaultFontID, fontSize, fontColor, data.text.str());
}

void textfmt(TextfmtData* data, const char* format, ...)
{
	// Write text
	data->text.clear();
	va_list args;
	va_start(args, format);
	data->text.vappendf(format, args);
	va_end(args);

	// Update data and add command
	commonAddChildLogic(TEXTFMT_ID, data, &data->base);

	// Text can't be activated
	data->base.activated = false;
}

void textfmt(strID id, const char* format, ...)
{
	TextfmtData* data = ctx().getWidgetData<TextfmtData>(id);

	// Write text
	data->text.clear();
	va_list args;
	va_start(args, format);
	data->text.vappendf(format, args);
	va_end(args);

	// Update data and add command
	commonAddChildLogic(TEXTFMT_ID, data, &data->base);

	// Text can't be activated
	data->base.activated = false;
}

void textfmt(const char* id, const char* format, ...)
{
	TextfmtData* data = ctx().getWidgetData<TextfmtData>(id);

	// Write text
	data->text.clear();
	va_list args;
	va_start(args, format);
	data->text.vappendf(format, args);
	va_end(args);

	// Update data and add command
	commonAddChildLogic(TEXTFMT_ID, data, &data->base);

	// Text can't be activated
	data->base.activated = false;
}

// Rectangle
// ------------------------------------------------------------------------------------------------

static constexpr char RECT_NAME[] = "rect";
constexpr strID RECT_ID = strID(sfz::hash(RECT_NAME));

static void rectDrawDefault(
	const WidgetCmd* cmd,
	AttributeSet* attributes,
	const mat34& surfaceTransform,
	f32 lagSinceSurfaceEndSecs)
{
	(void)attributes;
	(void)lagSinceSurfaceEndSecs;
	const RectData& data = *cmd->data<RectData>();
	mat34 transform =
		mul(surfaceTransform, mat34::translation3(vec3(data.base.box.center(), 0.0f)));
	drawRect(transform, data.base.box.dims(), data.linearColor);
}

void rect(RectData* data, vec4 linearColor)
{
	// Store data
	data->linearColor = linearColor;

	// Update data and add command
	commonAddChildLogic(RECT_ID, data, &data->base);

	// Rectangle can't be activated
	data->base.activated = false;
}

void rect(strID id, vec4 linearColor)
{
	RectData* data = ctx().getWidgetData<RectData>(id);
	rect(data, linearColor);
}

void rect(const char* id, vec4 linearColor)
{
	RectData* data = ctx().getWidgetData<RectData>(id);
	rect(data, linearColor);
}

// Image
// ------------------------------------------------------------------------------------------------

static constexpr char IMAGE_NAME[] = "image";
constexpr strID IMAGE_ID = strID(sfz::hash(IMAGE_NAME));

static void imageDrawDefault(
	const WidgetCmd* cmd,
	AttributeSet* attributes,
	const mat34& surfaceTransform,
	f32 lagSinceSurfaceEndSecs)
{
	(void)attributes;
	(void)lagSinceSurfaceEndSecs;
	const ImageData& data = *cmd->data<ImageData>();
	mat34 transform =
		mul(surfaceTransform, mat34::translation3(vec3(data.base.box.center(), 0.0f)));
	drawImage(transform, data.base.box.dims(), data.imageHandle);
}

void image(ImageData* data, u64 imageHandle)
{
	// Store data
	data->imageHandle = imageHandle;

	// Update data and add command
	commonAddChildLogic(IMAGE_ID, data, &data->base);

	// Image can't be activated
	data->base.activated = false;
}

void image(strID id, u64 imageHandle)
{
	ImageData* data = ctx().getWidgetData<ImageData>(id);
	image(data, imageHandle);
}

void image(const char* id, u64 imageHandle)
{
	ImageData* data = ctx().getWidgetData<ImageData>(id);
	image(data, imageHandle);
}

void image(ImageData* data, const char* imageHandleID)
{
	image(data, strID(imageHandleID));
}

void image(strID id, const char* imageHandleID)
{
	image(id, strID(imageHandleID));
}

void image(const char* id, const char* imageHandleID)
{
	image(id, strID(imageHandleID));
}

// Button
// ------------------------------------------------------------------------------------------------

static constexpr char BUTTON_NAME[] = "button";
constexpr strID BUTTON_ID = strID(sfz::hash(BUTTON_NAME));

static void buttonHandlePointerInput(WidgetCmd* cmd, vec2 pointerPosSS)
{
	ButtonData& data = *cmd->data<ButtonData>();
	if (data.enabled && data.base.box.pointInside(pointerPosSS)) {
		data.base.setFocused();
	}
	else {
		data.base.setUnfocused();
	}
}

static void buttonHandleMoveInput(WidgetCmd* cmd, Input* input, bool* moveActive)
{
	ButtonData& data = *cmd->data<ButtonData>();
	if (data.enabled && isMoveAction(input->action)) {
		if (data.base.focused && !*moveActive) {
			data.base.setUnfocused();
			*moveActive = true;
		}
		else if (*moveActive) {
			data.base.setFocused();
			input->action = InputAction::NONE;
			*moveActive = false;
		}
	}
}

static void buttonDrawDefault(
	const WidgetCmd* cmd,
	AttributeSet* attributes,
	const mat34& surfaceTransform,
	f32 lagSinceSurfaceEndSecs)
{
	(void)attributes;
	(void)lagSinceSurfaceEndSecs;
	const ButtonData& data = *cmd->data<ButtonData>();

	// Check attributes
	strID defaultFontID = attributes->get(DEFAULT_FONT_ATTRIB_ID)->as<strID>();
	vec4 fontColor = attributes->get(FONT_COLOR)->as<vec4>();
	vec4 baseColor = attributes->get(BASE_COLOR)->as<vec4>();
	vec4 focusColor = attributes->get(FOCUS_COLOR)->as<vec4>();
	vec4 activateColor = attributes->get(ACTIVATE_COLOR)->as<vec4>();
	f32 textScaling = attributes->get(BUTTON_TEXT_SCALING)->as<f32>();
	f32 borderWidth = attributes->get(BUTTON_BORDER_WIDTH)->as<f32>();
	vec4 disabledColor = attributes->get(BUTTON_DISABLED_COLOR)->as<vec4>();

	vec4 color = baseColor;
	if (data.base.focused) {
		color = focusColor;
	}
	else if (data.base.timeSinceFocusEndedSecs < 0.25f) {
		color = lerp(focusColor, baseColor, data.base.timeSinceFocusEndedSecs * 4.0f);
	}

	if (data.base.activated) {
		color = activateColor;
	}
	else if (data.base.timeSinceActivationSecs < 1.0f) {
		color = lerp(activateColor, color, data.base.timeSinceActivationSecs);
	}

	if (!data.enabled) {
		color = disabledColor;
	}

	mat34 transform =
		mul(surfaceTransform, mat34::translation3(vec3(data.base.box.center(), 0.0f)));
	drawBorder(transform, data.base.box.dims(), borderWidth, color);
	f32 textSize = data.base.box.height() * textScaling;
	drawTextFmtCentered(transform, defaultFontID, textSize, baseColor, data.text.str());
}

bool button(ButtonData* data, const char* text, bool enabled)
{
	// Store data
	data->text.clear();
	data->text.appendf("%s", text);
	data->enabled = enabled;

	// Update data and add command
	commonAddChildLogic(BUTTON_ID, data, &data->base);

	// If not enabled, defocus
	if (!enabled) data->base.setUnfocused();

	// Set activation
	Surface& surface = *ctx().activeSurface;
	data->base.activated = false;
	if (data->base.focused && surface.desc.input.action == InputAction::ACTIVATE) {
		data->base.setActivated();

		// Remove our input action, we have used it up
		surface.desc.input.action = InputAction::NONE;
	}

	return data->base.activated;
}

bool button(strID id, const char* text, bool enabled)
{
	ButtonData* data = ctx().getWidgetData<ButtonData>(id);
	return button(data, text, enabled);
}

bool button(const char* id, const char* text, bool enabled)
{
	ButtonData* data = ctx().getWidgetData<ButtonData>(id);
	return button(data, text, enabled);
}

// Initialization
// ------------------------------------------------------------------------------------------------

void internalCoreWidgetsInit()
{
	// Register attributes
	{
		registerDefaultAttribute("font_color", vec4(1.0f));
		registerDefaultAttribute("base_color", vec4(1.0f));
		registerDefaultAttribute("focus_color", vec4(0.8f, 0.3f, 0.3f, 1.0f));
		registerDefaultAttribute("activate_color", vec4(1.0f, 0.0f, 0.0f, 1.0f));

		registerDefaultAttribute("button_text_scaling", 1.0f);
		registerDefaultAttribute("button_border_width", 1.0f);
		registerDefaultAttribute("button_disabled_color", vec4(0.2f, 0.2f, 0.2f, 0.5f));
	}

	// List container
	{
		WidgetDesc desc = {};
		desc.widgetDataSizeBytes = sizeof(ListData);
		desc.getWidgetBaseFunc = commonGetBase<ListData>;
		desc.getNextWidgetBoxFunc = listGetNextWidgetBox;
		desc.handlePointerInputFunc = listHandlePointerInput;
		desc.handleMoveInputFunc = listHandleMoveInput;
		registerWidget(LIST_NAME, desc);
	}

	// Tree
	{
		// Base
		{
			WidgetDesc desc = {};
			desc.widgetDataSizeBytes = sizeof(TreeBaseData);
			desc.getWidgetBaseFunc = commonGetBase<TreeBaseData>;
			desc.getNextWidgetBoxFunc = treeBaseGetNextWidgetBox;
			desc.handlePointerInputFunc = treeBaseHandlePointerInput;
			desc.handleMoveInputFunc = treeBaseHandleMoveInput;
			registerWidget(TREE_BASE_NAME, desc);
		}
		
		// Entry
		{
			WidgetDesc desc = {};
			desc.widgetDataSizeBytes = sizeof(TreeEntryData);
			desc.getWidgetBaseFunc = commonGetBase<TreeEntryData>;
			desc.getNextWidgetBoxFunc = treeEntryGetNextWidgetBox;
			desc.handlePointerInputFunc = treeEntryHandlePointerInput;
			desc.handleMoveInputFunc = treeEntryHandleMoveInput;
			desc.drawFunc = treeEntryDrawDefault;
			registerWidget(TREE_ENTRY_NAME, desc);
		}
	}

	// Textfmt
	{
		WidgetDesc desc = {};
		desc.widgetDataSizeBytes = sizeof(TextfmtData);
		desc.getWidgetBaseFunc = commonGetBase<TextfmtData>;
		desc.handlePointerInputFunc = noPointerInput;
		desc.handleMoveInputFunc = noMoveInput;
		desc.drawFunc = textfmtDrawDefault;
		registerWidget(TEXTFMT_NAME, desc);
	}

	// Rectangle
	{
		WidgetDesc desc = {};
		desc.widgetDataSizeBytes = sizeof(RectData);
		desc.getWidgetBaseFunc = commonGetBase<RectData>;
		desc.handlePointerInputFunc = noPointerInput;
		desc.handleMoveInputFunc = noMoveInput;
		desc.drawFunc = rectDrawDefault;
		registerWidget(RECT_NAME, desc);
	}

	// Image
	{
		WidgetDesc desc = {};
		desc.widgetDataSizeBytes = sizeof(ImageData);
		desc.getWidgetBaseFunc = commonGetBase<ImageData>;
		desc.handlePointerInputFunc = noPointerInput;
		desc.handleMoveInputFunc = noMoveInput;
		desc.drawFunc = imageDrawDefault;
		registerWidget(IMAGE_NAME, desc);
	}

	// Button
	{
		WidgetDesc desc = {};
		desc.widgetDataSizeBytes = sizeof(ButtonData);
		desc.getWidgetBaseFunc = commonGetBase<ButtonData>;
		desc.handlePointerInputFunc = buttonHandlePointerInput;
		desc.handleMoveInputFunc = buttonHandleMoveInput;
		desc.drawFunc = buttonDrawDefault;
		registerWidget(BUTTON_NAME, desc);
	}
}

} // namespace zui
