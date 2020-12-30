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

#include "sfz/util/IO.hpp"

#include <skipifzero.hpp>

#include "sfz/Logging.hpp"

#include <cstdlib>
#include <cstdio> // fopen, fwrite, BUFSIZ
#include <cstdint>
#include <cstring>

#include "sfz/PushWarnings.hpp"

#if defined(_WIN32)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#include <direct.h>

#elif defined(__APPLE__)
#include <sys/stat.h>

#elif defined(__unix)
#include <sys/stat.h>
#endif

#include "sfz/PopWarnings.hpp"

namespace sfz {

using std::size_t;
using std::uint8_t;

// Statics
// ------------------------------------------------------------------------------------------------

template<typename T>
static Array<T> readFileInternal(const char* path, bool binaryMode, Allocator* allocator) noexcept
{
	// Open file
	if (path == nullptr) return Array<T>();
	std::FILE* file = std::fopen(path, binaryMode ? "rb" : "r");
	if (file == NULL) return Array<T>();

	// Get size of file
	std::fseek(file, 0, SEEK_END);
	int64_t size = std::ftell(file);
	std::rewind(file); // Rewind position to beginning of file
	if (size < 0) {
		std::fclose(file);
		return Array<T>();
	}

	// Create array with enough capacity to fit file
	Array<T> temp(uint32_t(size + 1), allocator, sfz_dbg("readFileInternal()"));

	// Read the file into the array
	uint8_t buffer[BUFSIZ];
	size_t readSize;
	size_t currOffs = 0;
	while ((readSize = std::fread(buffer, 1, BUFSIZ, file)) > 0) {

		// Ensure array has space.
		temp.ensureCapacity(uint32_t(currOffs + readSize));

		// Copy chunk into array
		std::memcpy(temp.data() + currOffs, buffer, readSize);
		currOffs += readSize;
	}

	// Set size of array
	temp.hackSetSize(uint32_t(currOffs));

	std::fclose(file);
	return std::move(temp);
}

// Paths
// ------------------------------------------------------------------------------------------------

const char* myDocumentsPath() noexcept
{
	static const char* path = []() {
#ifdef _WIN32
		char* tmp = static_cast<char*>(getDefaultAllocator()->allocate(sfz_dbg("sfz::myDocumentsPath()"), MAX_PATH + 2, 32));
		HRESULT result = SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, tmp);
		if (result != S_OK) SFZ_ERROR_AND_EXIT("sfzCore", "%s", "Could not retrieve MyDocuments path.");

		// Add path separator
		size_t pathLen = std::strlen(tmp);
		tmp[pathLen] = '/';
		tmp[pathLen+1] = '\0';

		return tmp;
#else
		const char* envHome = std::getenv("HOME");
		size_t pathLen = std::strlen(envHome);

		char* tmp = static_cast<char*>(getDefaultAllocator()->allocate(sfz_dbg("sfz::myDocumentsPath()"), pathLen + 2, 32));
		std::strncpy(tmp, envHome, pathLen);

		// Add path separator
		tmp[pathLen] = '/';
		tmp[pathLen + 1] = '\0';
		return tmp;
#endif
	}();
	return path;
}

const char* gameBaseFolderPath() noexcept
{
	static const char* path = []() {
		const char* myDocuments = myDocumentsPath();
		size_t len = std::strlen(myDocuments);
		char* tmp = static_cast<char*>(getDefaultAllocator()->allocate(sfz_dbg("sfz::gameBaseFolderPath()"), len + 32, 32));
#ifdef __APPLE__
		std::snprintf(tmp, len + 32, "%s%s", myDocuments, "Library/Application Support/");
#else
		std::snprintf(tmp, len + 32, "%s%s", myDocuments, "My Games/");
#endif
		return tmp;
	}();
	return path;
}

const char* getFileNameFromPath(const char* path) noexcept
{
	const char* strippedFile1 = std::strrchr(path, '\\');
	const char* strippedFile2 = std::strrchr(path, '/');
	if (strippedFile1 == nullptr && strippedFile2 == nullptr) {
		return path;
	}
	else if (strippedFile2 == nullptr) {
		return strippedFile1 + 1;
	}
	else {
		return strippedFile2 + 1;
	}
}

// IO functions
// ------------------------------------------------------------------------------------------------

bool fileExists(const char* path) noexcept
{
	std::FILE* file = std::fopen(path, "r");
	if (file == NULL) return false;
	std::fclose(file);
	return true;
}

bool directoryExists(const char* path) noexcept
{
#ifdef _WIN32
	std::FILE* file = std::fopen(path, "r");
	if (file == NULL) {
		DWORD ftyp = GetFileAttributesA(path);
		if (ftyp == INVALID_FILE_ATTRIBUTES) return false;
		if (ftyp & FILE_ATTRIBUTE_DIRECTORY) return true;
		return false;
	}
	std::fclose(file);
	return true;
#else
	std::FILE* file = std::fopen(path, "r");
	if (file == NULL) return false;
	std::fclose(file);
	return true;
#endif
}

bool createFile(const char* path) noexcept
{
	std::FILE* file = std::fopen(path, "w");
	if (file == NULL) return false;
	std::fclose(file);
	return true;
}

bool createDirectory(const char* path) noexcept
{
#ifdef _WIN32
	int res = _mkdir(path);
	return res == 0;
#else
	int res = mkdir(path, 0775);
	return res == 0;
#endif
}

bool deleteFile(const char* path) noexcept
{
	int res = std::remove(path);
	return res == 0;
}

bool deleteDirectory(const char* path) noexcept
{
#ifdef _WIN32
	int res = _rmdir(path);
	return res == 0;
#else
	int res = std::remove(path);
	return res == 0;
#endif
}

bool copyFile(const char* srcPath, const char* dstPath) noexcept
{
	uint8_t buffer[BUFSIZ];

	std::FILE* source = std::fopen(srcPath, "rb");
	if (source == NULL) return false;
	std::FILE* destination = std::fopen(dstPath, "wb");
	if (destination == NULL) {
		std::fclose(source);
		return false;
	}

	size_t size;
	while ((size = std::fread(buffer, 1, BUFSIZ, source)) > 0) {
		std::fwrite(buffer, 1, size, destination);
	}

	std::fclose(source);
	std::fclose(destination);

	return true;
}

int64_t sizeofFile(const char* path) noexcept
{
	std::FILE* file = std::fopen(path, "rb");
	if (file == NULL) return -1;
	std::fseek(file, 0, SEEK_END);
	int64_t size = std::ftell(file);
	std::fclose(file);
	return size;
}

int32_t readBinaryFile(const char* path, uint8_t* dataOut, size_t maxNumBytes) noexcept
{
	// Open file
	std::FILE* file = std::fopen(path, "rb");
	if (file == NULL) return -1;

	// Read the file into memory
	uint8_t buffer[BUFSIZ];
	size_t readSize;
	size_t currOffs = 0;
	while ((readSize = std::fread(buffer, 1, BUFSIZ, file)) > 0) {

		// Check if memory has enough space left
		if ((currOffs + readSize) > maxNumBytes) {
			std::fclose(file);
			std::memcpy(dataOut + currOffs, buffer, maxNumBytes - currOffs);
			return -2;
		}

		std::memcpy(dataOut + currOffs, buffer, readSize);
		currOffs += readSize;
	}

	std::fclose(file);
	return 0;
}

Array<uint8_t> readBinaryFile(const char* path, Allocator* allocator) noexcept
{
	return readFileInternal<uint8_t>(path, true, allocator);
}

DynString readTextFile(const char* path, Allocator* allocator) noexcept
{
	Array<char> strData = readFileInternal<char>(path, false, allocator);
	if (strData.size() == 0 || strData[strData.size() - 1] != '\0') {
		if (strData.data() == nullptr) {
			strData.init(0, allocator, sfz_dbg("readTextFile()"));
		}
		strData.add('\0');
	}

	DynString tmp;
	tmp.internalArray().swap(strData);

	return tmp;
}

bool writeBinaryFile(const char* path, const uint8_t* data, size_t numBytes) noexcept
{
	// Open file
	if (path == nullptr) return false;
	std::FILE* file = std::fopen(path, "wb");
	if (file == NULL) return false;

	size_t numWritten = std::fwrite(data, 1, numBytes, file);
	std::fclose(file);
	return (numWritten == numBytes);
}

bool writeTextFile(const char* path, const char* str, size_t numChars) noexcept
{
	// Open file
	if (path == nullptr) return false;
	std::FILE* file = std::fopen(path, "w");
	if (file == NULL) return false;

	// Get length of string if numChars not specified
	if (numChars == 0) numChars = std::strlen(str);

	// Write string
	size_t numWritten = std::fwrite(str, 1, numChars, file);
	std::fclose(file);
	return (numWritten == numChars);
}

} // namespace sfz
