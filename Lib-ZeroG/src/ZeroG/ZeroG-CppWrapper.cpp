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

#include "ZeroG/ZeroG.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
namespace zg {


// Statics
// ------------------------------------------------------------------------------------------------

static const char* stripFilePath(const char* file) noexcept
{
	const char* strippedFile1 = std::strrchr(file, '\\');
	const char* strippedFile2 = std::strrchr(file, '/');
	if (strippedFile1 == nullptr && strippedFile2 == nullptr) {
		return file;
	}
	else if (strippedFile2 == nullptr) {
		return strippedFile1 + 1;
	}
	else {
		return strippedFile2 + 1;
	}
}

// Error handling helpers
// ------------------------------------------------------------------------------------------------

const char* errorCodeToString(ZgErrorCode errorCode) noexcept
{
	switch (errorCode) {
	case ZG_SUCCESS: return "ZG_SUCCESS";
	case ZG_ERROR_GENERIC: return "ZG_ERROR_GENERIC";
	case ZG_ERROR_UNIMPLEMENTED: return "ZG_ERROR_UNIMPLEMENTED";
	case ZG_ERROR_CPU_OUT_OF_MEMORY: return "ZG_ERROR_CPU_OUT_OF_MEMORY";
	case ZG_ERROR_NO_SUITABLE_DEVICE: return "ZG_ERROR_NO_SUITABLE_DEVICE";
	case ZG_ERROR_INVALID_ARGUMENT: return "ZG_ERROR_INVALID_ARGUMENT";
	case ZG_ERROR_SHADER_COMPILE_ERROR: return "ZG_ERROR_SHADER_COMPILE_ERROR";
	}
	return "UNKNOWN";
}

ZgErrorCode CheckZgImpl::operator% (ZgErrorCode result) noexcept
{
	if (result == ZG_SUCCESS) return ZG_SUCCESS;
	printf("%s:%i: ZeroG error: %s\n", stripFilePath(file), line, errorCodeToString(result));
	return result;
}

// Context
// ------------------------------------------------------------------------------------------------

ZgErrorCode Context::init(const ZgContextInitSettings& settings)
{
	this->destroy();
	return zgContextCreate(&mContext, &settings);
}

void Context::swap(Context& other) noexcept
{
	std::swap(this->mContext, other.mContext);
}

void Context::destroy() noexcept
{
	zgContextDestroy(mContext);
	mContext = nullptr;
}

} // namespace zg
