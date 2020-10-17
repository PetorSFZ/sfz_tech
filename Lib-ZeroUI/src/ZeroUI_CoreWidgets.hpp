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

#include "ZeroUI.hpp"

namespace zui {

// List container
// ------------------------------------------------------------------------------------------------

struct ListData final {
	WidgetBase base = {};
	float widgetHeight = -FLT_MAX;
	float vertSpacing = -FLT_MAX;
	float currPosY = -FLT_MAX;
};

void listBegin(ListData* data, float widgetHeight, float vertSpacing = 0.0f);
void listBegin(strID id, float widgetHeight, float vertSpacing = 0.0f);
void listBegin(const char* id, float widgetHeight, float vertSpacing = 0.0f);
void listEnd();

// Tree
// ------------------------------------------------------------------------------------------------

struct TreeBaseData final {
	WidgetBase base;
	vec2 entryDims = vec2(-FLT_MAX);
	float entryContWidth = -FLT_MAX;
	float entryVertSpacing = -FLT_MAX;
	float horizSpacing = -FLT_MAX;
	float currPosY = -FLT_MAX;
	uint32_t activatedEntryIdx = ~0u;
};

struct TreeEntryData final {
	WidgetBase base;
	zui::BaseContainerData baseCon;
	str48 text;
	bool enabled = true;
};

void treeBegin(TreeBaseData* data, vec2 entryDims, float entryVertSpacing = 0.0f, float horizSpacing = 0.0f);
void treeBegin(strID id, vec2 entryDims, float entryVertSpacing = 0.0f, float horizSpacing = 0.0f);
void treeBegin(const char* id, vec2 entryDims, float entryVertSpacing = 0.0f, float horizSpacing = 0.0f);
void treeEnd();

bool treeCollapsableBegin(TreeEntryData* data, const char* text, bool enabled = true);
bool treeCollapsableBegin(strID id, const char* text, bool enabled = true);
bool treeCollapsableBegin(const char* id, const char* text, bool enabled = true);
void treeCollapsableEnd();

bool treeButton(TreeEntryData* data, const char* text, bool enabled = true);
bool treeButton(strID id, const char* text, bool enabled = true);
bool treeButton(const char* id, const char* text, bool enabled = true);

// Textfmt
// ------------------------------------------------------------------------------------------------

struct TextfmtData final {
	WidgetBase base = {};
	sfz::str256 text;
};

void textfmt(TextfmtData* data, const char* format, ...);
void textfmt(strID id, const char* format, ...);
void textfmt(const char* id, const char* format, ...);

// Rectangle
// ------------------------------------------------------------------------------------------------

struct RectData final {
	WidgetBase base = {};
	vec4 linearColor = vec4(1.0f);
};

void rect(RectData* data, vec4 linearColor = vec4(1.0f));
void rect(strID id, vec4 linearColor = vec4(1.0f));
void rect(const char* id, vec4 linearColor = vec4(1.0f));

// Image
// ------------------------------------------------------------------------------------------------

struct ImageData final {
	WidgetBase base = {};
	uint64_t imageHandle = 0;
};

void image(ImageData* data, uint64_t imageHandle);
void image(strID id, uint64_t imageHandle);
void image(const char* id, uint64_t imageHandle);

void image(ImageData* data, const char* imageHandleID);
void image(strID id, const char* imageHandleID);
void image(const char* id, const char* imageHandleID);

// Button
// ------------------------------------------------------------------------------------------------

struct ButtonData final {
	WidgetBase base = {};
	str48 text;
	bool enabled = true;
};

bool button(ButtonData* data, const char* text, bool enabled = true);
bool button(strID id, const char* text, bool enabled = true);
bool button(const char* id, const char* text, bool enabled = true);

// Initialization
// ------------------------------------------------------------------------------------------------

// This function initializes the core widgets so that they can be used with ZeroUI. You DO NOT need
// to call this manually, it will automatically be called when initializing ZeroUI.
//
// Use this as a reference for what need to be done to connect widgets to ZeroUI when implementing
// your own custom ones.

void internalCoreWidgetsInit();

} // namespace zui
