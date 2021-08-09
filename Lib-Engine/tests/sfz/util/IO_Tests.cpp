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

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include "utest.h"
#undef near
#undef far

#include <skipifzero_strings.hpp>

#include "sfz/util/IO.hpp"

// TODO: Fix test cases for iOS
#ifndef SFZ_IOS

static const char* stupidFileName = "jfioaejfaiojefaiojfeaojf.fajefaoejfa";

UTEST(IO, create_file_file_exists_delete_file)
{
	const char* fpath = stupidFileName;

	bool resExists1 = sfz::fileExists(fpath);
	if (resExists1) {
		ASSERT_TRUE(sfz::deleteFile(fpath));
		resExists1 = sfz::fileExists(fpath);
	}
	ASSERT_TRUE(!resExists1);

	ASSERT_TRUE(sfz::createFile(fpath));
	ASSERT_TRUE(sfz::fileExists(fpath));
	ASSERT_TRUE(sfz::deleteFile(fpath));
	ASSERT_TRUE(!sfz::fileExists(fpath));
}

UTEST(IO, create_directory_directory_exists_delete_directory)
{
	const char* dpath = stupidFileName;

	bool resExists1 = sfz::directoryExists(dpath);
	if (resExists1) {
		ASSERT_TRUE(sfz::deleteDirectory(dpath));
		resExists1 = sfz::directoryExists(dpath);
	}
	ASSERT_TRUE(!resExists1);

	ASSERT_TRUE(sfz::createDirectory(dpath));
	ASSERT_TRUE(sfz::directoryExists(dpath));
	ASSERT_TRUE(sfz::deleteDirectory(dpath));
	ASSERT_TRUE(!sfz::directoryExists(dpath));
}

UTEST(IO, write_binary_file_read_binary_file_sizeof_file)
{
	const char* fpath = stupidFileName;
	const uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0xA, 0xB, 0xC, 0xD, 0xE};
	uint8_t data2[sizeof(data)];

	bool fileExists = sfz::fileExists(fpath);
	if (fileExists) {
		ASSERT_TRUE(sfz::deleteFile(fpath));
		fileExists = sfz::fileExists(fpath);
	}
	ASSERT_TRUE(!fileExists);

	ASSERT_TRUE(sfz::writeBinaryFile(fpath, data, sizeof(data)));
	ASSERT_TRUE(sfz::readBinaryFile(fpath, data2, sizeof(data2)) == 0);
	auto data3 = sfz::readBinaryFile(fpath);
	ASSERT_TRUE(data3.size() == sizeof(data));
	ASSERT_TRUE(sizeof(data) == (size_t)sfz::sizeofFile(fpath));

	for (size_t i = 0; i < sizeof(data); ++i) {
		ASSERT_TRUE(data[i] == data2[i]);
		ASSERT_TRUE(data[i] == data3[uint32_t(i)]);
	}

	ASSERT_TRUE(sfz::deleteFile(fpath));
	ASSERT_TRUE(!sfz::fileExists(fpath));

	// nullptrs
	ASSERT_TRUE(sfz::readBinaryFile(nullptr).data() == nullptr);
	ASSERT_TRUE(sfz::readTextFile(nullptr) == "");
	ASSERT_TRUE(!sfz::writeBinaryFile(nullptr, nullptr, 0));
}

UTEST(IO, read_text_file)
{
	const char* fpath = stupidFileName;
	const char* strToWrite = "Hello World!\nHello World 2!\nHello World 3!";
	size_t strToWriteLen = strlen(strToWrite);

	bool fileExists = sfz::fileExists(fpath);
	if (fileExists) {
		ASSERT_TRUE(sfz::deleteFile(fpath));
		fileExists = sfz::fileExists(fpath);
	}
	ASSERT_TRUE(!fileExists);

	ASSERT_TRUE(sfz::writeBinaryFile(fpath, (const uint8_t*)strToWrite, strToWriteLen));
	ASSERT_TRUE(sfz::fileExists(fpath));

	sfz::DynString fileStr = sfz::readTextFile(fpath);
	ASSERT_TRUE(fileStr.size() == strToWriteLen);
	ASSERT_TRUE(fileStr.size() == strlen(fileStr.str()));
	ASSERT_TRUE(fileStr == strToWrite);

	ASSERT_TRUE(sfz::deleteFile(fpath));

	// Empty file
	ASSERT_TRUE(sfz::writeBinaryFile(fpath, (const uint8_t*)"", 0));
	ASSERT_TRUE(sfz::fileExists(fpath));
	sfz::DynString emptyStr = sfz::readTextFile(fpath);
	ASSERT_TRUE(emptyStr.size() == 0);
	ASSERT_TRUE(emptyStr == "");
	ASSERT_TRUE(sfz::deleteFile(fpath));
}

UTEST(IO, write_text_file)
{
	const char* fpath = stupidFileName;
	sfz::str320 strToWrite("Hello World!\nHello World 2!\nHello World 3!");

	bool fileExists = sfz::fileExists(fpath);
	if (fileExists) {
		ASSERT_TRUE(sfz::deleteFile(fpath));
		fileExists = sfz::fileExists(fpath);
	}
	ASSERT_TRUE(!fileExists);

	ASSERT_TRUE(sfz::writeTextFile(fpath, strToWrite));
	ASSERT_TRUE(sfz::fileExists(fpath));

	sfz::DynString fileStr = sfz::readTextFile(fpath);
	ASSERT_TRUE(fileStr.size() == strToWrite.size());
	ASSERT_TRUE(fileStr.size() == strlen(fileStr.str()));
	ASSERT_TRUE(fileStr == strToWrite.str());

	ASSERT_TRUE(sfz::deleteFile(fpath));

	// Not all chars
	ASSERT_TRUE(sfz::writeTextFile(fpath, strToWrite, 13));
	ASSERT_TRUE(sfz::fileExists(fpath));

	fileStr = sfz::readTextFile(fpath);
	ASSERT_TRUE(fileStr.size() == 13);
	ASSERT_TRUE(fileStr.size() == strlen(fileStr.str()));
	ASSERT_TRUE(fileStr == "Hello World!\n");

	ASSERT_TRUE(sfz::deleteFile(fpath));


	// Empty file
	ASSERT_TRUE(sfz::writeTextFile(fpath, ""));
	ASSERT_TRUE(sfz::fileExists(fpath));
	sfz::DynString emptyStr = sfz::readTextFile(fpath);
	ASSERT_TRUE(emptyStr.size() == 0);
	ASSERT_TRUE(emptyStr == "");
	ASSERT_TRUE(sfz::deleteFile(fpath));
}

#endif
