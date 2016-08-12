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

#include "sfz/PushWarnings.hpp"
#include "catch.hpp"
#include "sfz/PopWarnings.hpp"

#include "sfz/util/IO.hpp"

static const char* stupidFileName = "jfioaejfaiojefaiojfeaojf.fajefaoejfa";

static sfz::DynString appendBasePath(const char* fileName) noexcept
{
	size_t baseLen = std::strlen(sfz::basePath());
	size_t fileLen = std::strlen(fileName);
	sfz::DynString tmp("", static_cast<uint32_t>(baseLen + fileLen + 10));
	tmp.printf("%s", sfz::basePath());
	tmp.printfAppend("%s", fileName);
	return std::move(tmp);
}

TEST_CASE("createFile() & fileExists() & deleteFile()", "[sfz::IO]")
{
	auto filePath = appendBasePath(stupidFileName);
	const char* fpath = filePath.str();

	bool resExists1 = sfz::fileExists(fpath);
	if (resExists1) {
		REQUIRE(sfz::deleteFile(fpath));
		resExists1 = sfz::fileExists(fpath);
	}
	REQUIRE(!resExists1);

	REQUIRE(sfz::createFile(fpath));
	REQUIRE(sfz::fileExists(fpath));
	REQUIRE(sfz::deleteFile(fpath));
	REQUIRE(!sfz::fileExists(fpath));
}

TEST_CASE("createDirectory() & directoryExists() & deleteDirectory()", "[sfz::IO]")
{
	auto dirPath = appendBasePath(stupidFileName);
	const char* dpath = dirPath.str();

	bool resExists1 = sfz::directoryExists(dpath);
	if (resExists1) {
		REQUIRE(sfz::deleteDirectory(dpath));
		resExists1 = sfz::directoryExists(dpath);
	}
	REQUIRE(!resExists1);

	REQUIRE(sfz::createDirectory(dpath));
	REQUIRE(sfz::directoryExists(dpath));
	REQUIRE(sfz::deleteDirectory(dpath));
	REQUIRE(!sfz::directoryExists(dpath));
}

TEST_CASE("writeBinaryFile() & readBinaryFile() & sizeofFile(), ", "[sfz::IO]")
{
	auto filePath = appendBasePath(stupidFileName);
	const char* fpath = filePath.str();
	const uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0xA, 0xB, 0xC, 0xD, 0xE};
	uint8_t data2[sizeof(data)];

	bool fileExists = sfz::fileExists(fpath);
	if (fileExists) {
		REQUIRE(sfz::deleteFile(fpath));
		fileExists = sfz::fileExists(fpath);
	}
	REQUIRE(!fileExists);

	REQUIRE(sfz::writeBinaryFile(fpath, data, sizeof(data)));
	REQUIRE(sfz::readBinaryFile(fpath, data2, sizeof(data2)) == 0);
	auto data3 = sfz::readBinaryFile(fpath);
	REQUIRE(data3.size() == sizeof(data));
	REQUIRE(sizeof(data) == (size_t)sfz::sizeofFile(fpath));

	for (size_t i = 0; i < sizeof(data); ++i) {
		REQUIRE(data[i] == data2[i]);
		REQUIRE(data[i] == data3[uint32_t(i)]);
	}

	REQUIRE(sfz::deleteFile(fpath));
	REQUIRE(!sfz::fileExists(fpath));

	// nullptrs
	REQUIRE(sfz::readBinaryFile(nullptr).data() == nullptr);
	REQUIRE(sfz::readTextFile(nullptr) == "");
	REQUIRE(!sfz::writeBinaryFile(nullptr, nullptr, 0));
}

TEST_CASE("readTextFile()", "[sfz::IO]")
{
	auto filePath = appendBasePath(stupidFileName);
	const char* fpath = filePath.str();
	const char* strToWrite = "Hello World!\nHello World 2!\nHello World 3!";
	size_t strToWriteLen = std::strlen(strToWrite);

	bool fileExists = sfz::fileExists(fpath);
	if (fileExists) {
		REQUIRE(sfz::deleteFile(fpath));
		fileExists = sfz::fileExists(fpath);
	}
	REQUIRE(!fileExists);

	REQUIRE(sfz::writeBinaryFile(fpath, (const uint8_t*)strToWrite, strToWriteLen));
	REQUIRE(sfz::fileExists(fpath));

	sfz::DynString fileStr = sfz::readTextFile(fpath);
	REQUIRE(fileStr.size() == strToWriteLen);
	REQUIRE(fileStr.size() == std::strlen(fileStr.str()));
	REQUIRE(fileStr == strToWrite);

	REQUIRE(sfz::deleteFile(fpath));

	// Empty file
	REQUIRE(sfz::writeBinaryFile(fpath, (const uint8_t*)"", 0));
	REQUIRE(sfz::fileExists(fpath));
	sfz::DynString emptyStr = sfz::readTextFile(fpath);
	REQUIRE(emptyStr.size() == 0);
	REQUIRE(emptyStr == "");
	REQUIRE(sfz::deleteFile(fpath));
}
