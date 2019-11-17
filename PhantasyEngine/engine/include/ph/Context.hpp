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

#pragma once

#include "sfz/Context.hpp"

// Forward declared member types
// ------------------------------------------------------------------------------------------------

namespace sfz {

class StringCollection;

} // namespace sfz

namespace ph {

class TerminalLogger;
class GlobalConfig;
using sfz::StringCollection;

} // namespace ph

// PhantasyEngine Context struct
// ------------------------------------------------------------------------------------------------

struct phContext {
	sfz::Context sfzContext;
	ph::TerminalLogger* logger = nullptr;
	ph::GlobalConfig* config = nullptr;

	// The resource strings registered with PhantasyEngine.
	//
	// Comparing and storing strings when refering to specific assets (meshes, textures, etc)
	// becomes expensive in the long run. A solution is to hash each string and use the hash
	// instead. This works under the assumption that we have no hash collisions. See sfz::StringID
	// for more information.
	//
	// Because we don't want any collisions globally in the game we store the datastructure keeping
	// track of the strings and their hash in the global context.
	sfz::StringCollection* resourceStrings = nullptr;
};

namespace ph {

// Context getters/setters
// ------------------------------------------------------------------------------------------------

phContext* getContext() noexcept;

inline GlobalConfig& getGlobalConfig() noexcept { return *getContext()->config; }

inline StringCollection& getResourceStrings() noexcept { return *getContext()->resourceStrings; }

bool setContext(phContext* context) noexcept;

// Statically owned context
// ------------------------------------------------------------------------------------------------

/// Statically owned context struct. Default constructed, members need to be set manually. Only to
/// be used for setContext() in PhantasyEngineMain.cpp.
phContext* getStaticContextBoot() noexcept;

} // namespace ph
