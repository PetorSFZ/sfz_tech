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

#include "sfz/strings/StringID.hpp"

#include <cinttypes>

#include "sfz/Assert.hpp"
#include "sfz/containers/HashMap.hpp"
#include "sfz/memory/New.hpp"
#include "sfz/strings/DynString.hpp"
#include "sfz/strings/StringHashers.hpp"

namespace sfz {

// StringCollectionImpl
// ------------------------------------------------------------------------------------------------

struct StringCollectionImpl final {
	HashMap<StringID, DynString> strings;
};

// StringCollection: Constructors & destructors
// ------------------------------------------------------------------------------------------------

StringCollection::StringCollection(uint32_t initialCapacity) noexcept
{
	this->createStringCollection(initialCapacity);
}

StringCollection::StringCollection(StringCollection&& other) noexcept
{
	this->swap(other);
}

StringCollection& StringCollection::operator= (StringCollection&& other) noexcept
{
	this->swap(other);
	return *this;
}

StringCollection::~StringCollection() noexcept
{
	this->destroy();
}

// StringCollection: Methods
// ------------------------------------------------------------------------------------------------

void StringCollection::createStringCollection(uint32_t initialCapacity) noexcept
{
	this->destroy();

	mImpl = sfz_new<StringCollectionImpl>();
	mImpl->strings = HashMap<StringID, DynString>(initialCapacity);
}

void StringCollection::swap(StringCollection& other) noexcept
{
	std::swap(this->mImpl, other.mImpl);
}

void StringCollection::destroy() noexcept
{
	if (mImpl == nullptr) return;

	sfz_delete(this->mImpl);
	this->mImpl = nullptr;
}

uint32_t StringCollection::numStringsHeld() const noexcept
{
	sfz_assert_debug(mImpl != nullptr);
	return mImpl->strings.size();
}

StringID StringCollection::getStringID(const char* string) noexcept
{
	sfz_assert_debug(mImpl != nullptr);

	// Hash string
	StringID strId;
	strId.id = sfz::hash(string);

	// Add string to HashMap if it does not exist
	DynString* strPtr = mImpl->strings.get(strId);
	if (strPtr == nullptr) {
		strPtr = &mImpl->strings.put(strId, DynString(string));
	}

	// Check if string collision occurred
	if (*strPtr != string) {
		sfz::error("String hash collision occurred, \"%s\" and \"%s\" with hash %" PRIu64 "",
		           strPtr->str(), string, strId.id);
	}

	return strId;
}

const char* StringCollection::getStringFromID(StringID id) const noexcept
{
	sfz_assert_debug(mImpl != nullptr);
	DynString* strPtr = mImpl->strings.get(id);
	if (strPtr == nullptr) return nullptr;
	return strPtr->str();
}

} // namespace sfz
