// Copyright (c) Peter Hillerström (skipifzero.com, peter@hstroem.se)
//               For other contributors see Contributors.txt
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

#include "sfz/renderer/ZeroGUtils.hpp"

#include <skipifzero_strings.hpp>

#include <SDL_syswm.h>

#include "sfz/SfzLogging.h"
#include "sfz/config/SfzConfig.h"
#include "sfz/util/IO.hpp"

namespace sfz {

// Statics
// -----------------------------------------------------------------------------------------------

static HWND getWin32WindowHandle(SDL_Window* window) noexcept
{
	SDL_SysWMinfo info = {};
	SDL_VERSION(&info.version);
	if (!SDL_GetWindowWMInfo(window, &info)) return nullptr;
	return info.info.win.window;
}

// ZeroG logger
// -----------------------------------------------------------------------------------------------

static void zeroGLog(
	void* userPtr, const char* file, int line, ZgLogLevel level, const char* message)
{
	(void)userPtr;
	SfzLogLevel errorLevel = [&]() {
		switch (level) {
		case ZG_LOG_LEVEL_NOISE: return SFZ_LOG_LEVEL_NOISE;
		case ZG_LOG_LEVEL_INFO: return SFZ_LOG_LEVEL_INFO;
		case ZG_LOG_LEVEL_WARNING: return SFZ_LOG_LEVEL_WARNING;
		case ZG_LOG_LEVEL_ERROR: return SFZ_LOG_LEVEL_ERROR;
		}
		sfz_assert(false);
		return SFZ_LOG_LEVEL_ERROR;
	}();

	SfzLogger* logger = sfzLoggingGetLogger();
	logger->log(logger->implData, file, line, errorLevel, "%s", message);
}

ZgLogger getPhantasyEngineZeroGLogger() noexcept
{
	ZgLogger logger = {};
	logger.log = zeroGLog;
	return logger;

	struct ZgLogger {

		// Function pointer to user-specified log function.
		void(*log)(void* userPtr, const char* file, int line, ZgLogLevel level, const char* message);

		// User specified pointer that is provied to each log() call.
		void* userPtr;
	};
}

// Error handling helpers
// -----------------------------------------------------------------------------------------------

bool CheckZgImpl::operator% (ZgResult result) noexcept
{
	if (zgIsSuccess(result)) return true;
	if (zgIsWarning(result)) {
		SfzLogger* logger = sfzLoggingGetLogger();
		logger->log(logger->implData, file, line, SFZ_LOG_LEVEL_WARNING,
			"ZgResult: %s", zgResultToString(result));
	}
	else {
		SfzLogger* logger = sfzLoggingGetLogger();
		logger->log(logger->implData, file, line, SFZ_LOG_LEVEL_ERROR,
			"ZgResult: %s", zgResultToString(result));
	}
	return false;
}

// Initialization helpers
// -----------------------------------------------------------------------------------------------

bool initializeZeroG(
	SDL_Window* window,
	const char* userDataDir,
	SfzAllocator* allocator,
	SfzConfig* cfg,
	bool vsync) noexcept
{
	// Log compiled and linked version of ZeroG
	SFZ_LOG_INFO("ZeroG compiled API version: %u, linked version: %u",
		ZG_COMPILED_API_VERSION, zgApiLinkedVersion());

	const bool callSetStablePowerState = sfzCfgGetBool(cfg, "ZeroG.OnStartup_CallSetStablePowerState");
	const bool stablePowerEnabled = sfzCfgGetBool(cfg, "ZeroG.OnStartup_StablePowerEnabled");

	int w = 0, h = 0;
	SDL_GL_GetDrawableSize(window, &w, &h);

	// Init settings
	ZgContextInitSettings initSettings = {};
	initSettings.width = u32(w);
	initSettings.height = u32(h);
	initSettings.vsync = vsync ? ZG_TRUE : ZG_FALSE;
	initSettings.logger = getPhantasyEngineZeroGLogger();
	initSettings.allocator = allocator;
	initSettings.nativeHandle = getNativeHandle(window);
	initSettings.autoCachePipelines = sfzCfgGetBool(cfg, "ZeroG.OnStartup_CacheShaders") ? ZG_TRUE : ZG_FALSE;
	str320 pipelineCacheDir = str320("%s/shader_cache", userDataDir);
	sfz::createDirectory(pipelineCacheDir.str());
	initSettings.autoCachePipelinesDir = pipelineCacheDir.str();
	initSettings.d3d12.debugMode = sfzCfgGetBool(cfg, "ZeroG.OnStartup_DebugMode") ? ZG_TRUE : ZG_FALSE;
	initSettings.d3d12.debugModeGpuBased = sfzCfgGetBool(cfg, "ZeroG.OnStartup_DebugModeGpuBased") ? ZG_TRUE : ZG_FALSE;
	initSettings.d3d12.useSoftwareRenderer = sfzCfgGetBool(cfg, "ZeroG.OnStartup_SoftwareRenderer") ? ZG_TRUE : ZG_FALSE;
	initSettings.d3d12.enableDredAutoBreadcrumbs = sfzCfgGetBool(cfg, "ZeroG.OnStartup_DredAutoBreadcrumbs") ? ZG_TRUE : ZG_FALSE;
	initSettings.d3d12.callSetStablePowerState = callSetStablePowerState ? ZG_TRUE : ZG_FALSE;
	initSettings.d3d12.stablePowerEnabled = stablePowerEnabled ? ZG_TRUE : ZG_FALSE;

	// Initialize ZeroG
	bool initSuccess = CHECK_ZG zgContextInit(&initSettings);

	return initSuccess;
}

void* getNativeHandle(SDL_Window* window) noexcept
{
#ifdef WIN32
	return getWin32WindowHandle(window);
#else
#error "Not implemented yet"
#endif
}

} // namespace sfz
