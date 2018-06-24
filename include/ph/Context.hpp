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

namespace ph {

class TerminalLogger;
class GlobalConfig;

} // namespace ph

// PhantasyEngine Context struct
// ------------------------------------------------------------------------------------------------

struct phContext {
	sfz::Context sfzContext;
	ph::TerminalLogger* logger = nullptr;
	ph::GlobalConfig* config = nullptr;
};

namespace ph {

// Context getters/setters
// ------------------------------------------------------------------------------------------------

phContext* getContext() noexcept;

inline GlobalConfig& getGlobalConfig() noexcept { return *getContext()->config; }

bool setContext(phContext* context) noexcept;

// Statically owned context
// ------------------------------------------------------------------------------------------------

/// Statically owned context struct. Default constructed, members need to be set manually. Only to
/// be used for setContext() in PhantasyEngineMain.cpp.
phContext* getStaticContextBoot() noexcept;

} // namespace ph
