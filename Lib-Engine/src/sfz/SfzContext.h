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

#pragma once

#include <sfz.h>

// Context types
// ------------------------------------------------------------------------------------------------

struct SfzAudioEngine;
struct SfzConfig;
struct SfzCtx;
struct SfzCtxView;
struct SfzEngineInfo;
struct SfzProfilingStats;
struct SfzRenderer;
struct SfzResourceManager;
struct SfzScheduler;
struct SfzShaderManager;
struct SfzStrIDs;

// SfzCtx
// ------------------------------------------------------------------------------------------------

typedef void SfzCtxTypeDestroyFunc(void* bytes);

sfz_extern_c SfzCtx* sfzCtxCreate(SfzAllocator* globalAllocator);
sfz_extern_c void sfzCtxDestroy(SfzCtx* ctx);

sfz_extern_c void sfzCtxRegisterType(
	SfzCtx* ctx,
	u64 type,
	void* data,
	SfzCtxTypeDestroyFunc* destroyFunc);

// SfzCtxView
// ------------------------------------------------------------------------------------------------

sfz_extern_c SfzCtxView* sfzCtxCreateView(SfzCtx* ctx, u64 readAccess, u64 writeAccess);
sfz_extern_c void sfzCtxDestroyView(SfzCtx* ctx, SfzCtxView* view);

sfz_extern_c u64 sfzCtxViewReadAccess(SfzCtxView* view);
sfz_extern_c u64 sfzCtxViewWriteAccess(SfzCtxView* view);

sfz_extern_c void* sfzCtxViewGet(const SfzCtxView* view, u64 type);
sfz_extern_c const void* sfzCtxViewGetConst(const SfzCtxView* view, u64 type);

// Known types
// ------------------------------------------------------------------------------------------------

// Global allocator is special and automatically registered when creating the context.
sfz_constant u64 SFZ_CTX_GLOBAL_ALLOCATOR = u64(1) << u64(1);
inline SfzAllocator* sfzCtxGetGlobalAllocator(const SfzCtxView* view)
{ 
	return (SfzAllocator*)sfzCtxViewGet(view, SFZ_CTX_GLOBAL_ALLOCATOR);
}

// String IDs
sfz_constant u64 SFZ_CTX_STR_IDS = u64(1) << u64(2);
inline SfzStrIDs* sfzCtxGetStrIDs(const SfzCtxView* view)
{
	return (SfzStrIDs*)sfzCtxViewGet(view, SFZ_CTX_STR_IDS);
}
inline const SfzStrIDs* sfzCtxGetStrIDsConst(const SfzCtxView* view)
{
	return (const SfzStrIDs*)sfzCtxViewGetConst(view, SFZ_CTX_STR_IDS);
}

// Engine info
sfz_constant u64 SFZ_CTX_ENGINE_INFO = u64(1) << u64(3);
inline SfzEngineInfo* sfzCtxGetEngineInfo(const SfzCtxView* view)
{
	return (SfzEngineInfo*)sfzCtxViewGet(view, SFZ_CTX_ENGINE_INFO);
}
inline const SfzEngineInfo* sfzCtxGetEngineInfoConst(const SfzCtxView* view)
{
	return (const SfzEngineInfo*)sfzCtxViewGetConst(view, SFZ_CTX_ENGINE_INFO);
}

// Config
sfz_constant u64 SFZ_CTX_CONFIG = u64(1) << u64(4);
inline SfzConfig* sfzCtxGetConfig(const SfzCtxView* view)
{
	return (SfzConfig*)sfzCtxViewGet(view, SFZ_CTX_CONFIG);
}
inline const SfzConfig* sfzCtxGetConfigConst(const SfzCtxView* view)
{
	return (const SfzConfig*)sfzCtxViewGetConst(view, SFZ_CTX_CONFIG);
}

// Scheduler
sfz_constant u64 SFZ_CTX_SCHEDULER = u64(1) << u64(5);
inline SfzScheduler* sfzCtxGetScheduler(const SfzCtxView* view)
{
	return (SfzScheduler*)sfzCtxViewGet(view, SFZ_CTX_SCHEDULER);
}

// Resource Manager
sfz_constant u64 SFZ_CTX_RES_MAN = u64(1) << u64(6);
inline SfzResourceManager* sfzCtxGetResMan(const SfzCtxView* view)
{
	return (SfzResourceManager*)sfzCtxViewGet(view, SFZ_CTX_RES_MAN);
}
inline const SfzResourceManager* sfzCtxGetResManConst(const SfzCtxView* view)
{
	return (const SfzResourceManager*)sfzCtxViewGetConst(view, SFZ_CTX_RES_MAN);
}

// Shader Manager
sfz_constant u64 SFZ_CTX_SHADER_MAN = u64(1) << u64(7);
inline SfzShaderManager* sfzCtxGetShaderMan(const SfzCtxView* view)
{
	return (SfzShaderManager*)sfzCtxViewGet(view, SFZ_CTX_SHADER_MAN);
}
inline const SfzShaderManager* sfzCtxGetShaderManConst(const SfzCtxView* view)
{
	return (const SfzShaderManager*)sfzCtxViewGetConst(view, SFZ_CTX_SHADER_MAN);
}

// Renderer
sfz_constant u64 SFZ_CTX_RENDERER = u64(1) << u64(8);
inline SfzRenderer* sfzCtxGetRenderer(const SfzCtxView* view)
{
	return (SfzRenderer*)sfzCtxViewGet(view, SFZ_CTX_RENDERER);
}
inline const SfzRenderer* sfzCtxGetRendererConst(const SfzCtxView* view)
{
	return (const SfzRenderer*)sfzCtxViewGetConst(view, SFZ_CTX_RENDERER);
}

// Audio Engine
sfz_constant u64 SFZ_CTX_AUDIO = u64(1) << u64(9);
inline SfzAudioEngine* sfzCtxGetAudio(const SfzCtxView* view)
{
	return (SfzAudioEngine*)sfzCtxViewGet(view, SFZ_CTX_AUDIO);
}
inline const SfzAudioEngine* sfzCtxGetAudioConst(const SfzCtxView* view)
{
	return (const SfzAudioEngine*)sfzCtxViewGetConst(view, SFZ_CTX_AUDIO);
}

// Profiling Stats
sfz_constant u64 SFZ_CTX_PROF_STATS = u64(1) << u64(10);
inline SfzProfilingStats* sfzCtxGetProfStats(const SfzCtxView* view)
{
	return (SfzProfilingStats*)sfzCtxViewGet(view, SFZ_CTX_PROF_STATS);
}
inline const SfzProfilingStats* sfzCtxGetProfStatsConst(const SfzCtxView* view)
{
	return (const SfzProfilingStats*)sfzCtxViewGetConst(view, SFZ_CTX_PROF_STATS);
}
