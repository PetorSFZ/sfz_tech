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

#include <cstdint>

#include "sfz/containers/DynArray.hpp"
#include "sfz/memory/Allocator.hpp"
#include "sfz/strings/DynString.hpp"

namespace sfz {

using std::int32_t;
using std::int64_t;
using std::uint8_t;

// Paths
// ------------------------------------------------------------------------------------------------

/// Returns path to the the directory the application was run from, likely executable location.
/// The string itself is owned by this function and should not be deallocated or modified.
const char* basePath() noexcept;

/// Returns path to MyDocuments on Windows, user root (~) on Unix. The string itself is owned by
/// this function and should not be deallocated or modified. Guaranteed to end with path separator.
const char* myDocumentsPath() noexcept;

/// Returns path to where game folders with saves should be placed. The string itself is owned by
/// this function and should not be deallocated or modified. Guaranteed to end with path separator.
const char* gameBaseFolderPath() noexcept;

// IO functions
// ------------------------------------------------------------------------------------------------

/// Returns whether a given file exists or not.
bool fileExists(const char* path) noexcept;

/// Returns whether a given directory exists or not.
bool directoryExists(const char* path) noexcept;

/// Attempts to create a file and returns whether successful or not.
bool createFile(const char* path) noexcept;

/// Attempts to create a directory and returns whether successful or not.
bool createDirectory(const char* path) noexcept;

/// Attempts to delete a given file and returns whether successful or not.
bool deleteFile(const char* path) noexcept;

/// Attempts to delete a given directory, will ONLY work if directory is empty.
bool deleteDirectory(const char* path) noexcept;

/// Attempts to copy file from source to destination.
bool copyFile(const char* srcPath, const char* dstPath) noexcept;

/// Returns size of file in bytes, negative value if error.
int64_t sizeofFile(const char* path) noexcept;

/// Reads binary file to pre-allocated memory.
/// \return 0 on success, -1 on error, -2 if file was larger than pre-allocated memory
int32_t readBinaryFile(const char* path, uint8_t* dataOut, size_t maxNumBytes) noexcept;

/// Reads binary file, returns empty DynArray if error.
DynArray<uint8_t> readBinaryFile(const char* path,
                                 Allocator* allocator = getDefaultAllocator()) noexcept;

/// Reads text file, returns empty string if error.
DynString readTextFile(const char* path, Allocator* allocator = getDefaultAllocator()) noexcept;

/// Writes memory to binary file, returns whether successful or not.
bool writeBinaryFile(const char* path, const uint8_t* data, size_t numBytes) noexcept;

} // namespace sfz
