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

#include <SDL_syswm.h>

#include "sfz/config/GlobalConfig.hpp"
#include <sfz/Logging.hpp>

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
	sfz::LogLevel errorLevel = [&]() {
		switch (level) {
		case ZG_LOG_LEVEL_NOISE: return sfz::LogLevel::NOISE;
		case ZG_LOG_LEVEL_INFO: return sfz::LogLevel::INFO;
		case ZG_LOG_LEVEL_WARNING: return sfz::LogLevel::WARNING;
		case ZG_LOG_LEVEL_ERROR: return sfz::LogLevel::ERROR_LVL;
		}
		sfz_assert(false);
		return sfz::LogLevel::ERROR_LVL;
	}();

	sfz::getLogger()->log(file, line, errorLevel, "ZeroG",
		"%s", message);
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
		sfz::getLogger()->log(file, line, sfz::LogLevel::WARNING, "ZeroG",
			"zg::Result: %s", zgResultToString(result));
	}
	else {
		sfz::getLogger()->log(file, line, sfz::LogLevel::ERROR_LVL, "ZeroG",
			"zg::Result: %s", zgResultToString(result));
	}
	return false;
}

// Initialization helpers
// -----------------------------------------------------------------------------------------------

bool initializeZeroG(
	SDL_Window* window,
	SfzAllocator* allocator,
	bool vsync) noexcept
{
	// Log compiled and linked version of ZeroG
	SFZ_INFO("ZeroG", "ZeroG compiled API version: %u, linked version: %u",
		ZG_COMPILED_API_VERSION, zgApiLinkedVersion());

	GlobalConfig& cfg = getGlobalConfig();
	Setting* debugModeSetting =
		cfg.sanitizeBool("ZeroG", "OnStartup_DebugMode", true, false);
	Setting* debugModeGpuBasedSetting =
		cfg.sanitizeBool("ZeroG", "OnStartup_DebugModeGpuBased", true, false);
	Setting* softwareRendererSetting =
		cfg.sanitizeBool("ZeroG", "OnStartup_SoftwareRenderer", true, false);
	Setting* d3d12DredAutoSetting =
		cfg.sanitizeBool("ZeroG", "OnStartup_DredAutoBreadcrumbs", true, false);

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
	initSettings.d3d12.debugMode = debugModeSetting->boolValue() ? ZG_TRUE : ZG_FALSE;
	initSettings.d3d12.debugModeGpuBased = debugModeGpuBasedSetting->boolValue() ? ZG_TRUE : ZG_FALSE;
	initSettings.d3d12.useSoftwareRenderer = softwareRendererSetting->boolValue() ? ZG_TRUE : ZG_FALSE;
	initSettings.d3d12.enableDredAutoBreadcrumbs = d3d12DredAutoSetting->boolValue() ? ZG_TRUE : ZG_FALSE;

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
