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

// List container
// ------------------------------------------------------------------------------------------------

static constexpr char ZUI_LIST_NAME[] = "list";
constexpr ZuiID ZUI_LIST_ID = zuiName(ZUI_LIST_NAME);

struct ZuiListData final {
	f32 widgetHeight;
	f32 vertSpacing;
	f32 offsetY;
	f32 scrollY;
};

static void listGetNextWidgetBox(ZuiCtx* zui, ZuiWidget* w, ZuiID childID, ZuiBox* boxOut)
{
	(void)zui;
	(void)childID;
	ZuiListData& data = *w->data<ZuiListData>();
	const f32 currPosY = w->base.box.center().y + w->base.box.dims().y * 0.5f + data.offsetY + data.scrollY;
	f32x2 nextPos = f32x2_init(w->base.box.center().x, currPosY);
	data.offsetY -= (data.widgetHeight + data.vertSpacing);
	*boxOut = zuiBoxInit(nextPos, f32x2_init(w->base.box.dims().x, data.widgetHeight));
}

static void listScrollInput(ZuiWidget* w, f32x2 scroll)
{
	ZuiListData& data = *w->data<ZuiListData>();
	data.scrollY += -scroll.y;
	const f32 maxY =
		f32_max(f32_abs(data.offsetY) - data.widgetHeight * 0.5f - w->base.box.dims().y, 0.0f);
	data.scrollY = f32_clamp(data.scrollY, 0.0f, maxY);
}

void zuiListBegin(ZuiCtx* zui, ZuiID id, f32 widgetHeight, f32 vertSpacing)
{
	bool initial = false;
	ZuiWidget* w = zuiCtxCreateWidgetParent<ZuiListData>(zui, id, ZUI_LIST_ID, &initial);
	ZuiListData* data = w->data<ZuiListData>();

	// Set list data from parameters
	sfz_assert(widgetHeight > 0.0f);
	data->widgetHeight = widgetHeight;
	data->vertSpacing = vertSpacing;
	if (vertSpacing <= 0.0f) data->vertSpacing = data->widgetHeight * 0.5f;

	// Calculate initial next widget y-pos
	data->offsetY = -data->widgetHeight * 0.5f;
	if (initial) data->scrollY = 0.0f;

	// Can't activate list container
	w->base.activated = false;
}

void zuiListEnd(ZuiCtx* zui)
{
	zuiCtxPopWidgetParent(zui, ZUI_LIST_ID);
}

// Textfmt
// ------------------------------------------------------------------------------------------------

static constexpr char ZUI_TEXTFMT_NAME[] = "textfmt";
constexpr ZuiID ZUI_TEXTFMT_ID = zuiName(ZUI_TEXTFMT_NAME);

struct ZuiTextfmtData final {
	SfzStr320 text;
};

static void textfmtDrawDefault(
	ZuiCtx* zui,
	const ZuiWidget* widget,
	const SfzMat44* surfaceTransform,
	f32 lagSinceInputEndSecs)
{
	(void)lagSinceInputEndSecs;
	const ZuiTextfmtData& data = *widget->data<ZuiTextfmtData>();

	// Check attributes
	ZuiID defaultFontID = zui->attribs.get(DEFAULT_FONT_ATTRIB_ID.id)->as<ZuiID>();
	f32x4 fontColor = zui->attribs.get(FONT_COLOR.id)->as<f32x4>();

	const SfzMat44 transform =
		*surfaceTransform * sfzMat44Translation3(f32x3_init2(widget->base.box.center(), 0.0f));
	f32 fontSize = widget->base.box.dims().y;
	zuiDrawTextCentered(&zui->drawCtx, transform, defaultFontID, fontSize, fontColor, data.text.str);
}

void zuiTextfmt(ZuiCtx* zui, ZuiID id, const char* format, ...)
{
	ZuiWidget* w = zuiCtxCreateWidget<ZuiTextfmtData>(zui, id, ZUI_TEXTFMT_ID);
	ZuiTextfmtData* data = w->data<ZuiTextfmtData>();

	// Write text
	sfzStr320Clear(&data->text);
	va_list args;
	va_start(args, format);
	sfzStr320Appendf(&data->text, format, args);
	va_end(args);

	// Text can't be activated
	//data->base.activated = false;
}

// Rectangle
// ------------------------------------------------------------------------------------------------

static constexpr char ZUI_RECT_NAME[] = "rect";
constexpr ZuiID ZUI_RECT_ID = zuiName(ZUI_RECT_NAME);

struct ZuiRectData final {
	f32x4 linearColor = f32x4_splat(1.0f);
};

static void rectDrawDefault(
	ZuiCtx* zui,
	const ZuiWidget* widget,
	const SfzMat44* surfaceTransform,
	f32 lagSinceInputEndSecs)
{
	(void)lagSinceInputEndSecs;
	const ZuiRectData& data = *widget->data<ZuiRectData>();
	const SfzMat44 transform =
		*surfaceTransform * sfzMat44Translation3(f32x3_init2(widget->base.box.center(), 0.0f));
	zuiDrawRect(&zui->drawCtx, transform, widget->base.box.dims(), data.linearColor);
}

void zuiRect(ZuiCtx* zui, ZuiID id, f32x4 linearColor)
{
	ZuiWidget* w = zuiCtxCreateWidget<ZuiRectData>(zui, id, ZUI_RECT_ID);
	ZuiRectData* data = w->data<ZuiRectData>();

	// Store data
	data->linearColor = linearColor;

	// Rectangle can't be activated
	w->base.activated = false;
}

// Image
// ------------------------------------------------------------------------------------------------

static constexpr char ZUI_IMAGE_NAME[] = "image";
constexpr ZuiID ZUI_IMAGE_ID = zuiName(ZUI_IMAGE_NAME);

struct ZuiImageData final {
	u64 imageHandle = 0;
};

static void imageDrawDefault(
	ZuiCtx* zui,
	const ZuiWidget* widget,
	const SfzMat44* surfaceTransform,
	f32 lagSinceInputEndSecs)
{
	(void)lagSinceInputEndSecs;
	const ZuiImageData& data = *widget->data<ZuiImageData>();
	const SfzMat44 transform =
		*surfaceTransform * sfzMat44Translation3(f32x3_init2(widget->base.box.center(), 0.0f));
	zuiDrawImage(&zui->drawCtx, transform, widget->base.box.dims(), data.imageHandle);
}

void zuiImage(ZuiCtx* zui, ZuiID id, u64 imageHandle)
{
	ZuiWidget* w = zuiCtxCreateWidget<ZuiImageData>(zui, id, ZUI_IMAGE_ID);
	ZuiImageData* data = w->data<ZuiImageData>();

	// Store data
	data->imageHandle = imageHandle;

	// Image can't be activated
	w->base.activated = false;
}

// Button
// ------------------------------------------------------------------------------------------------

static constexpr char ZUI_BUTTON_NAME[] = "button";
constexpr ZuiID ZUI_BUTTON_ID = zuiName(ZUI_BUTTON_NAME);

struct ZuiButtonData final {
	SfzStr96 text;
};

static void buttonDrawDefault(
	ZuiCtx* zui,
	const ZuiWidget* widget,
	const SfzMat44* surfaceTransform,
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
	if (widget->base.focused) {
		color = focusColor;
	}
	else if (widget->base.timeSinceFocusEndedSecs < 0.25f) {
		color = sfz::lerp(focusColor, baseColor, widget->base.timeSinceFocusEndedSecs * 4.0f);
	}

	if (widget->base.activated) {
		color = activateColor;
	}
	else if (widget->base.timeSinceActivationSecs < 1.0f) {
		color = sfz::lerp(activateColor, color, widget->base.timeSinceActivationSecs);
	}

	const SfzMat44 transform =
		*surfaceTransform * sfzMat44Translation3(f32x3_init2(widget->base.box.center(), 0.0f));
	zuiDrawBorder(&zui->drawCtx, transform, widget->base.box.dims(), borderWidth, color);
	f32 textSize = widget->base.box.dims().y * textScaling;
	zuiDrawTextCentered(&zui->drawCtx, transform, defaultFontID, textSize, baseColor, data.text.str);
}

bool zuiButton(ZuiCtx* zui, ZuiID id, const char* text)
{
	ZuiWidget* w = zuiCtxCreateWidget<ZuiButtonData>(zui, id, ZUI_BUTTON_ID);
	ZuiButtonData* data = w->data<ZuiButtonData>();
	data->text = sfzStr96Init(text);
	const bool wasActivated = w->base.activated;
	w->base.activated = false;
	return wasActivated;
}

// Initialization
// ------------------------------------------------------------------------------------------------

void internalCoreWidgetsInit(ZuiCtx* zui)
{
	// Register attributes
	{
		zuiAttribRegisterDefault(zui, "font_color", zuiAttribInit(f32x4_splat(1.0f)));
		zuiAttribRegisterDefault(zui, "base_color", zuiAttribInit(f32x4_splat(1.0f)));
		zuiAttribRegisterDefault(zui, "focus_color", zuiAttribInit(f32x4_init(0.8f, 0.3f, 0.3f, 1.0f)));
		zuiAttribRegisterDefault(zui, "activate_color", zuiAttribInit(f32x4_init(1.0f, 0.0f, 0.0f, 1.0f)));

		zuiAttribRegisterDefault(zui, "button_text_scaling", zuiAttribInit(1.0f));
		zuiAttribRegisterDefault(zui, "button_border_width", zuiAttribInit(1.0f));
		zuiAttribRegisterDefault(zui, "button_disabled_color", zuiAttribInit(f32x4_init(0.2f, 0.2f, 0.2f, 0.5f)));
	}

	// List container
	{
		ZuiWidgetDesc desc = {};
		desc.widgetDataSizeBytes = sizeof(ZuiListData);
		desc.focuseable = true;
		desc.activateable = false;
		desc.getNextWidgetBoxFunc = listGetNextWidgetBox;
		desc.getNextChildIdxFunc = zuiWidgetGetNextChildIdxDefault;
		desc.scrollInputFunc = listScrollInput;
		desc.drawFunc = zuiDefaultPassthroughDrawFunc;
		zuiCtxRegisterWidget(zui, ZUI_LIST_NAME, &desc);
	}

	// Textfmt
	{
		ZuiWidgetDesc desc = {};
		desc.widgetDataSizeBytes = sizeof(ZuiTextfmtData);
		desc.focuseable = false;
		desc.activateable = false;
		desc.getNextChildIdxFunc = zuiWidgetGetNextChildIdxDefault;
		desc.scrollInputFunc = nullptr;
		desc.drawFunc = textfmtDrawDefault;
		zuiCtxRegisterWidget(zui, ZUI_TEXTFMT_NAME, &desc);
	}

	// Rectangle
	{
		ZuiWidgetDesc desc = {};
		desc.widgetDataSizeBytes = sizeof(ZuiRectData);
		desc.focuseable = false;
		desc.activateable = false;
		desc.getNextChildIdxFunc = zuiWidgetGetNextChildIdxDefault;
		desc.scrollInputFunc = nullptr;
		desc.drawFunc = rectDrawDefault;
		zuiCtxRegisterWidget(zui, ZUI_RECT_NAME, &desc);
	}

	// Image
	{
		ZuiWidgetDesc desc = {};
		desc.widgetDataSizeBytes = sizeof(ZuiImageData);
		desc.focuseable = false;
		desc.activateable = false;
		desc.getNextChildIdxFunc = zuiWidgetGetNextChildIdxDefault;
		desc.scrollInputFunc = nullptr;
		desc.drawFunc = imageDrawDefault;
		zuiCtxRegisterWidget(zui, ZUI_IMAGE_NAME, &desc);
	}

	// Button
	{
		ZuiWidgetDesc desc = {};
		desc.widgetDataSizeBytes = sizeof(ZuiButtonData);
		desc.focuseable = true;
		desc.activateable = true;
		desc.getNextChildIdxFunc = zuiWidgetGetNextChildIdxDefault;
		desc.scrollInputFunc = nullptr;
		desc.drawFunc = buttonDrawDefault;
		zuiCtxRegisterWidget(zui, ZUI_BUTTON_NAME, &desc);
	}
}
