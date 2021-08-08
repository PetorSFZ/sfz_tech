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

// About
// ------------------------------------------------------------------------------------------------

// This header contains low-level drawing functions used to implement custom widget rendering.
//
// You should typically only include this header if you are implementing a custom widget rendering
// function, e.g. an archetype.

namespace zui {

using sfz::strID;

// Initialization and internal interface
// ------------------------------------------------------------------------------------------------

// These functions are used to initialize and handle communication with the drawing module, these
// are automatically called by ZeroUI and should not be called manually by the user.

void internalDrawInit(SfzAllocator* allocator, uint32_t fontOversampling);
void internalDrawDeinit();
void internalDrawSetFontHandle(uint64_t handle);

bool internalDrawAddFont(const char* name, strID nameID, const char* path, float atlasSize);
bool internalDrawFontTextureUpdated();
ImageViewConst internalDrawGetFontTexture();

void internalDrawClearRenderData();
RenderDataView internalDrawGetRenderDataView();

// Low-level drawing functions
// ------------------------------------------------------------------------------------------------

// These are low-level drawing functions to draw directly to the current surface. These are
// primarily meant to be used when implementing your own custom drawing functions. Calling them
// intermixed with your normal UI code is undefined behaviour, as actually rendering the UI is
// deferred and not done immediately.

void drawAddCommand(
	const mat34& transform,
	const Vertex* vertices,
	uint32_t numVertices,
	const uint32_t* indices,
	uint32_t numIndices,
	uint64_t imageHandle = 0,
	bool isAlphaTexture = false);

float drawTextFmtCentered(
	const mat34& transform,
	strID fontID,
	float size,
	vec4 color,
	const char* text);

void drawImage(
	const mat34& transform,
	vec2 dims,
	uint64_t imageHandle,
	bool isAlphaTexture = false);

void drawRect(
	const mat34& transform,
	vec2 dims,
	vec4 color);

void drawBorder(
	const mat34& transform,
	vec2 dims,
	float thickness,
	vec4 color);

// TODO: This might be a tiny bit broken
float drawTextFmt(
	vec2 pos, HAlign halign, VAlign valign, strID fontID, float size, vec4 color, const char* format, ...);

} // namespace zui
