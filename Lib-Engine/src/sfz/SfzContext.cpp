// Copyright (c) Peter Hillerström (skipifzero.com, peter@hstroem.se)
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

#include "sfz/SfzContext.h"

#include <sfz_math.h>

#include <skipifzero_arrays.hpp>
#include <skipifzero_new.hpp>
#include <skipifzero_pool.hpp>

// Type impls
// ------------------------------------------------------------------------------------------------

sfz_struct(SfzCtxType) {
	u64 type;
	u32 typeLog2;
	void* data;
	SfzCtxTypeDestroyFunc* destroyFunc;
};

sfz_struct(SfzCtxView) {
	SfzCtx* ctx;
	SfzHandle handle;
	u64 readAccess;
	u64 writeAccess;
};

sfz_struct(SfzCtx) {
	SfzAllocator* allocator = nullptr;
	sfz::Array<SfzCtxType> types;
	sfz::Pool<SfzCtxView> views;
};

// SfzCtx
// ------------------------------------------------------------------------------------------------

SFZ_EXTERN_C SfzCtx* sfzCtxCreate(SfzAllocator* globalAllocator)
{
	SfzCtx* ctx = sfz_new<SfzCtx>(globalAllocator, sfz_dbg(""));
	ctx->allocator = globalAllocator;

	// Initialize memory for holding types
	ctx->types.init(32, globalAllocator, sfz_dbg("SfzCtx::types"));

	// Register error type
	ctx->types.add({});

	// Register global allocator
	sfzCtxRegisterType(ctx, SFZ_CTX_GLOBAL_ALLOCATOR, globalAllocator, nullptr);

	// Allocate memory for views
	constexpr u32 MAX_NUM_VIEWS = 1024;
	ctx->views.init(MAX_NUM_VIEWS, globalAllocator, sfz_dbg("SfzCtx::views"));

	return ctx;
}

SFZ_EXTERN_C void sfzCtxDestroy(SfzCtx* ctx)
{
	if (ctx == nullptr) return;
	SfzAllocator* allocator = ctx->allocator;

	// Destroy types in reverse order
	while(!ctx->types.isEmpty()) {
		SfzCtxType type = ctx->types.pop();
		if (type.destroyFunc != nullptr) {
			sfz_assert(type.data != nullptr);
			type.destroyFunc(type.data);
		}
	}

	sfz_delete(allocator, ctx);
}

SFZ_EXTERN_C void sfzCtxRegisterType(
	SfzCtx* ctx,
	u64 type,
	void* data,
	SfzCtxTypeDestroyFunc* destroyFunc)
{
	sfz_assert(type != 0);
	sfz_assert(sfzNextPow2_u64(type) == type);
	const u32 typeLog2 = u32(sfzLog2OfPow2_u64(type));
	sfz_assert(ctx->types.size() == typeLog2);
	SfzCtxType& ctxType = ctx->types.add();
	ctxType.type = type;
	ctxType.typeLog2 = typeLog2;
	ctxType.data = data;
	ctxType.destroyFunc = destroyFunc;
}

// SfzCtxView
// ------------------------------------------------------------------------------------------------

SFZ_EXTERN_C SfzCtxView* sfzCtxCreateView(SfzCtx* ctx, u64 readAccess, u64 writeAccess)
{
	sfz_assert((readAccess & writeAccess) == writeAccess);
	SfzHandle handle = ctx->views.allocate();
	SfzCtxView* view = ctx->views.get(handle);
	view->ctx = ctx;
	view->handle = handle;
	view->readAccess = readAccess;
	view->writeAccess = writeAccess;
	return view;
}

SFZ_EXTERN_C void sfzCtxDestroyView(SfzCtx* ctx, SfzCtxView* view)
{
	sfz_assert(view->handle != SFZ_NULL_HANDLE);
	sfz_assert(ctx->views.handleIsValid(view->handle));
	ctx->views.deallocate(view->handle);
}

SFZ_EXTERN_C u64 sfzCtxViewReadAccess(SfzCtxView* view)
{
	return view->readAccess;
}

SFZ_EXTERN_C u64 sfzCtxViewWriteAccess(SfzCtxView* view)
{
	return view->writeAccess;
}

SFZ_EXTERN_C void* sfzCtxViewGet(const SfzCtxView* view, u64 type)
{
	sfz_assert(type != 0);
	sfz_assert(sfzNextPow2_u64(type) == type);

	// Return null if we don't have write access to this type
	sfz_assert((view->writeAccess & type) == type);
	if ((view->writeAccess & type) != type) return nullptr;

	// Grab type and check some other stuff
	const u32 typeLog2 = u32(sfzLog2OfPow2_u64(type));
	SfzCtxType& ctxType = view->ctx->types[typeLog2];
	sfz_assert(ctxType.type == type);
	sfz_assert(ctxType.typeLog2 == typeLog2);

	// Return type
	return ctxType.data;
}

SFZ_EXTERN_C const void* sfzCtxViewGetConst(const SfzCtxView* view, u64 type)
{
	sfz_assert(type != 0);
	sfz_assert(sfzNextPow2_u64(type) == type);

	// Return null if we don't have write access to this type
	sfz_assert((view->readAccess & type) == type);
	if ((view->readAccess & type) != type) return nullptr;

	// Grab type and check some other stuff
	const u32 typeLog2 = u32(sfzLog2OfPow2_u64(type));
	SfzCtxType& ctxType = view->ctx->types[typeLog2];
	sfz_assert(ctxType.type == type);
	sfz_assert(ctxType.typeLog2 == typeLog2);

	// Return type
	return ctxType.data;
}
