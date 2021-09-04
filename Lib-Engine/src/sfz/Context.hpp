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

struct SfzAllocator;

namespace sfz {

// Forward declarations
// ------------------------------------------------------------------------------------------------

class LoggingInterface;
class GlobalConfig;
class ResourceManager;
class ShaderManager;
class Renderer;
class AudioEngine;
class ProfilingStats;

// PhantasyEngine global context
// ------------------------------------------------------------------------------------------------

// The PhantasyEngine global context.
//
// This context stores all of PhantasyEngine's globally available state. This includes things such
// as the global allocator, the logging interface to log via, the string collection where strings
// are registered, etc.
//
// Generally PhantasyEngine tries to avoid global state, but the things stored in this context are
// exceptions because it would be too annoying to pass them around everywhere. The general (but
// loose) rule is that things should only be put in the global context if it makes a lot of sense.
//
// When using dynamically linked libraries it is necessary to initialize these libraries by sending
// them a pointer to the context so they themselves can set it using "setContext()". If they don't
// then multiple contexts could theoretically exist, which could cause dangerous problems.
struct Context final {

	// The default allocator that is retrieved when "getDefaultAllocator()" is called. This should
	// be set in the beginning of the program, and may then NEVER be changed. I.e. is must remain
	// valid for the remaining duration of the program.
	SfzAllocator* defaultAllocator = nullptr;

	// The current logger used, see "sfz/Logging.hpp" for logging macros which use it.
	LoggingInterface* logger = nullptr;

	// The user data dir where the game should store user data. This can be e.g. next to executable
	// or in "My Games/AppName/" depending on engine settings.
	const char* userDataDir = nullptr;

	// The global config system which keeps track of key/value pair of settings.
	GlobalConfig* config = nullptr;

	// The resource manager.
	ResourceManager* resources = nullptr;

	// The shader manager.
	ShaderManager* shaders = nullptr;

	// The renderer.
	Renderer* renderer = nullptr;

	// The audio engine.
	AudioEngine* audioEngine = nullptr;

	// Global profiling stats.
	ProfilingStats* profilingStats = nullptr;
};

// Context getters/setters
// ------------------------------------------------------------------------------------------------

// Gets the current context. Will return nullptr if it has not been set using setContext().
Context* getContext() noexcept;

// Sets the current context.
//
// Will not take ownership of the Context struct itself, so the  caller has to ensure the pointer
// remains valid for the remaining duration of the program. If the pointer has already been set
// this function will terminate the program.
void setContext(Context* context) noexcept;

// Convenience getters
// ------------------------------------------------------------------------------------------------

inline SfzAllocator* getDefaultAllocator() { return getContext()->defaultAllocator; }
inline LoggingInterface* getLogger() { return getContext()->logger; }
inline const char* getUserDataDir() { return getContext()->userDataDir; }
inline GlobalConfig& getGlobalConfig() { return *getContext()->config; }
inline ResourceManager& getResourceManager() { return *getContext()->resources; }
inline ShaderManager& getShaderManager() { return *getContext()->shaders; }
inline Renderer& getRenderer() { return *getContext()->renderer; }
inline AudioEngine& getAudioEngine() { return *getContext()->audioEngine; }
inline ProfilingStats& getProfilingStats() { return *getContext()->profilingStats; }

} // namespace sfz
