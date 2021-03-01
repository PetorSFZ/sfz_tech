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

#include "sfz/util/FileWatch.hpp"

#include <cerrno>
#include <cstring>

#ifdef _WIN32
#include <sys/types.h>
#include <sys/stat.h>
static_assert(sizeof(time_t) == sizeof(__time64_t), "");
#endif

#include <sfz/Logging.hpp>

namespace sfz {

// FileWatch
// ------------------------------------------------------------------------------------------------

bool FileWatch::init(const char* path)
{
	mPath = path;
	mLastChange = 0;
	return hasChangedSinceLastCall();
}

bool FileWatch::hasChangedSinceLastCall()
{
	/* struct _stat64i32
	{
		_dev_t         st_dev;
		_ino_t         st_ino;
		unsigned short st_mode;
		short          st_nlink;
		short          st_uid;
		short          st_gid;
		_dev_t         st_rdev;
		_off_t         st_size;
		__time64_t     st_atime;
		__time64_t     st_mtime;
		__time64_t     st_ctime;
	};*/

	struct _stat64i32 buffer = {};
	int res = _stat(mPath.str(), &buffer);
	if (res < 0) {
		const str320 errorStr = str320("%s", strerror(errno));
		SFZ_ERROR("FileWatch", "Couldn't _stat(%s), errno: %s", mPath.str(), errorStr.str());
		return false;
	}
	sfz_assert(res == 0);

	const bool hasChanged = mLastChange < buffer.st_mtime;
	mLastChange = buffer.st_mtime;
	return hasChanged;
}

} // namespace sfz
