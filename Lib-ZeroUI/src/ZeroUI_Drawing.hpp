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

#pragma once

#include "ZeroUI.h"

// About
// ------------------------------------------------------------------------------------------------

// This header contains low-level drawing functions used to implement custom widget rendering.
//
// You should typically only include this header if you are implementing a custom widget rendering
// function.

// Initialization and internal interface
// ------------------------------------------------------------------------------------------------

struct ZuiDrawCtx;

// These functions are used to initialize and handle communication with the drawing module, these
// are automatically called by ZeroUI and should not be called manually by the user.

bool zuiInternalDrawCtxInit(ZuiDrawCtx* draw_ctx, const ZuiCfg* cfg, SfzAllocator* allocator);

void zuiInternalDrawCtxDestroy(ZuiDrawCtx* draw_ctx);

bool zuiInternalDrawAddFont(
	ZuiDrawCtx* draw_ctx, ZuiID id, const char* ttf_path, f32 atlas_size, SfzAllocator* allocator);

// Low-level drawing functions
// ------------------------------------------------------------------------------------------------

// These are low-level drawing functions to draw directly to the current surface. These are
// primarily meant to be used when implementing your own custom drawing functions. Calling them
// intermixed with your normal UI code is undefined behaviour, as actually rendering the UI is
// deferred and not done immediately.

void zuiDrawAddCommand(
	ZuiDrawCtx* draw_ctx,
	const SfzMat44& transform,
	const ZuiVertex* vertices,
	u32 num_vertices,
	const u32* indices,
	u32 num_indices,
	ZuiCmdType cmd_type,
	u64 image_handle);

f32 zuiDrawText(
	ZuiDrawCtx* draw_ctx,
	const SfzMat44& transform,
	ZuiAlign align,
	ZuiID font_id,
	f32 size,
	f32x4 color,
	const char* text);

void zuiDrawImage(
	ZuiDrawCtx* draw_ctx,
	const SfzMat44& transform,
	f32x2 dims,
	u64 image_handle);

void zuiDrawRect(
	ZuiDrawCtx* draw_ctx,
	const SfzMat44& transform,
	f32x2 dims,
	f32x4 color);

void zuiDrawBorder(
	ZuiDrawCtx* draw_ctx,
	const SfzMat44& transform,
	f32x2 dims,
	f32 thickness,
	f32x4 color);
