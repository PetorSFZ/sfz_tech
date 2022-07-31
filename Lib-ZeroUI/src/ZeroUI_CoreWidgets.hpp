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

#include "ZeroUI.h"

// List container
// ------------------------------------------------------------------------------------------------

// vertSpacing == 0 -> auto
void zuiListBegin(ZuiCtx* zui, ZuiID id, f32 widgetHeight, f32 vertSpacing);
void zuiListEnd(ZuiCtx* zui);

// Tree
// ------------------------------------------------------------------------------------------------

void zuiTreeBegin(ZuiCtx* zui, ZuiID id, f32x2 entryDims, f32 entryVertSpacing = 0.0f, f32 horizSpacing = 0.0f);
void zuiTreeEnd(ZuiCtx* zui);

bool zuiTreeCollapsableBegin(ZuiCtx* zui, ZuiID id, const char* text, bool enabled = true);
void zuiTreeCollapsableEnd(ZuiCtx* zui);

bool zuiTreeButton(ZuiCtx* zui, ZuiID id, const char* text, bool enabled = true);

// Textfmt
// ------------------------------------------------------------------------------------------------

void zuiTextfmt(ZuiCtx* zui, ZuiID id, const char* format, ...);

// Rectangle
// ------------------------------------------------------------------------------------------------

void zuiRect(ZuiCtx* zui, ZuiID id, f32x4 linearColor = f32x4(1.0f));

// Image
// ------------------------------------------------------------------------------------------------

void zuiImage(ZuiCtx* zui, ZuiID id, u64 imageHandle);

// Button
// ------------------------------------------------------------------------------------------------

bool zuiButton(ZuiCtx* zui, ZuiID id, const char* text, bool enabled = true);

// Initialization
// ------------------------------------------------------------------------------------------------

// This function initializes the core widgets so that they can be used with ZeroUI. You DO NOT need
// to call this manually, it will automatically be called when initializing ZeroUI.
//
// Use this as a reference for what need to be done to connect widgets to ZeroUI when implementing
// your own custom ones.

void internalCoreWidgetsInit(ZuiCtx* zui);
