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

#include "utest.h"

#include <skipifzero_allocators.hpp>

#include "sfz/Context.hpp"
#include "sfz/Logging.hpp"
#include "sfz/config/GlobalConfig.hpp"
#include "sfz/debug/ProfilingStats.hpp"
#include "sfz/strings/StringID.hpp"
#include "sfz/util/StandardLogger.hpp"

UTEST_STATE();

// This is the global PhantasyEngine context, a pointer to it will be set using setContext().
static sfz::StandardAllocator standardAllocator;
static sfz::Context phantasyEngineContext;
static sfz::GlobalConfig globalConfig;
static sfz::StringCollection stringCollection;
static sfz::ProfilingStats profilingStats;

static void setupContext() noexcept
{
	sfz::Context* context = &phantasyEngineContext;

	// Set standard allocator
	sfz::Allocator* allocator = &standardAllocator;
	context->defaultAllocator = allocator;

	// Set standard logger
	context->logger = sfz::getStandardLogger();

	// Set global config
	context->config = &globalConfig;

	// Resource strings
	stringCollection.createStringCollection(4096, allocator);
	context->resourceStrings = &stringCollection;

	// Profiling stats
	profilingStats.init(allocator);
	context->profilingStats = &profilingStats;

	// Set Phantasy Engine context
	sfz::setContext(context);
}

int main(int argc, char* argv[])
{
	setupContext();
	return utest_main(argc, argv);
}
