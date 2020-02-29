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

namespace sfz {

// Forward declarations
// ------------------------------------------------------------------------------------------------

class Allocator;
class LoggingInterface;
class GlobalConfig;
class Renderer;
class AudioEngine;
class ProfilingStats;
class StringCollection;

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
	Allocator* defaultAllocator = nullptr;

	// The current logger used, see "sfz/Logging.hpp" for logging macros which use it.
	LoggingInterface* logger = nullptr;

	// The global config system which keeps track of key/value pair of settings.
	GlobalConfig* config = nullptr;

	// The renderer.
	Renderer* renderer = nullptr;

	// The audio engine.
	AudioEngine* audioEngine = nullptr;

	// The registered resource strings.
	//
	// Comparing and storing strings when refering to specific assets (meshes, textures, etc)
	// becomes expensive in the long run. A solution is to hash each string and use the hash
	// instead. This works under the assumption that we have no hash collisions. See StringID for
	// more information.
	//
	// Because we don't want any collisions globally in the game we store the datastructure keeping
	// track of the strings and their hash in the global context.
	StringCollection* resourceStrings = nullptr;

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

inline Allocator* getDefaultAllocator() { return getContext()->defaultAllocator; }
inline LoggingInterface* getLogger() { return getContext()->logger; }
inline GlobalConfig& getGlobalConfig() { return *getContext()->config; }
inline Renderer& getRenderer() { return *getContext()->renderer; }
inline AudioEngine& getAudioEngine() { return *getContext()->audioEngine; }
inline StringCollection& getResourceStrings() { return *getContext()->resourceStrings; }
inline ProfilingStats& getProfilingStats() { return *getContext()->profilingStats; }

} // namespace sfz
