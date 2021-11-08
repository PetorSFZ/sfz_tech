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

// Initialization and internal interface
// ------------------------------------------------------------------------------------------------

// These functions are used to initialize and handle communication with the drawing module, these
// are automatically called by ZeroUI and should not be called manually by the user.

void internalDrawInit(SfzAllocator* allocator, u32 fontOversampling);
void internalDrawDeinit();
void internalDrawSetFontHandle(u64 handle);

bool internalDrawAddFont(const char* name, SfzStrID nameID, const char* path, f32 atlasSize);
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
	u32 numVertices,
	const u32* indices,
	u32 numIndices,
	u64 imageHandle = 0,
	bool isAlphaTexture = false);

f32 drawTextFmtCentered(
	const mat34& transform,
	SfzStrID fontID,
	f32 size,
	f32x4 color,
	const char* text);

void drawImage(
	const mat34& transform,
	f32x2 dims,
	u64 imageHandle,
	bool isAlphaTexture = false);

void drawRect(
	const mat34& transform,
	f32x2 dims,
	f32x4 color);

void drawBorder(
	const mat34& transform,
	f32x2 dims,
	f32 thickness,
	f32x4 color);

// TODO: This might be a tiny bit broken
f32 drawTextFmt(
	f32x2 pos, HAlign halign, VAlign valign, SfzStrID fontID, f32 size, f32x4 color, const char* format, ...);

} // namespace zui
