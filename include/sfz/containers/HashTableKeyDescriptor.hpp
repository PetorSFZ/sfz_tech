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

#include <functional> // std::hash, std::equal_to

namespace sfz {

// EqualTo2 template
// ------------------------------------------------------------------------------------------------

/// Template used to compare two objects of different type for equality. Based on EASTL's similar
/// equal_to_2 template. 
template<typename LeftT, typename RightT>
struct EqualTo2 final {
	bool operator() (const LeftT& lhs, const RightT& rhs) noexcept
	{
		static_assert(sizeof(LeftT) != sizeof(LeftT), "EqualTo2 must be specialized by user");
	}
};

// EqualTo2 specialization for when same both parameters are same type
// ------------------------------------------------------------------------------------------------

/// Specialization of EqualTo2 when both types are the same. Uses std::equal_to to compare the two
/// elements.
template<typename T>
struct EqualTo2<T,T> final {
	bool operator() (const T& lhs, const T& rhs) noexcept
	{
		std::equal_to<T> eq;
		return eq(lhs, rhs);
	}
};

// HashTableKeyDescriptor template
// ------------------------------------------------------------------------------------------------

/// Placeholder type used to specify that a given key does not have an alternate key type in a
/// HashTableKeyDescriptor.
struct NO_ALT_KEY_TYPE final {
	NO_ALT_KEY_TYPE() = delete;
	NO_ALT_KEY_TYPE(const NO_ALT_KEY_TYPE&) = delete;
	NO_ALT_KEY_TYPE& operator= (const NO_ALT_KEY_TYPE&) = delete;
	NO_ALT_KEY_TYPE(NO_ALT_KEY_TYPE&&) = delete;
	NO_ALT_KEY_TYPE& operator= (NO_ALT_KEY_TYPE&&) = delete;
};

/// Template used to describe how a key is hashed and compared with other keys in a hash table. Of
/// special note is the possibility to define an alternate key type compatible with the main type.
/// This is mainly useful when the key is a string class, in that case "const char*" can be defined
/// as an alt key. This can improve performance of the hash table as temporary copies, which might
/// require memory allocation, can be avoided.
/// 
/// In order to be a HashTableKeyDescriptor the following typedefs need to be available:
/// KeyT: The key type.
/// KeyHash: A type with the same interface as std::hash which can hash a key.
/// KeyEqual: A type with the same interface as std::equal_to which can compare two keys.
///
/// In addition the following typedefs used for the alternate key type needs to be available,
/// if no alternate key type exist they MUST all be set to NO_ALT_KEY_TYPE.
/// AltKeyT: An alternate key type compatible with KeyT.
/// AltKeyHash: Key hasher but for AltKeyT, must produce same hash as KeyHash for equivalent keys
/// (i.e. AltKeyKeyEqual says they are equal).
/// AltKeyKeyEqual: A type with the same interface as sfz::EqualTo2 which compares an AltKeyT
/// (left-hand side) with a KeyT (right-hand side).
///
/// In addition there is one additional constraint on an alt key type, it must be possible to
/// construct a normal key with the alt key. I.e., the key type needs a Key(AltKey alt)
/// constructor.
///
/// The default implementation uses std::hash<K> and std::equal_to<K>. In other words, as long
/// as std::hash is specialized and an equality (==) operator is defined the default
/// HashTableKeyDescriptor should just work.
///
/// If the default implementation is not enough (say if an alternate key type is wanted), this
/// template can either be specialized (in namespace sfz) for a specific key type. Alternatively
/// an equivalent struct (which fulfills all the constraints of a HashTableKeyDescriptor) can be
/// defined elsewhere and used in its place.
template<typename K>
struct HashTableKeyDescriptor final {
	using KeyT = K;
	using KeyHash = std::hash<KeyT>;
	using KeyEqual = std::equal_to<KeyT>;

	using AltKeyT = NO_ALT_KEY_TYPE;
	using AltKeyHash = NO_ALT_KEY_TYPE; // If specialized for alt key: std::hash<AltKeyT>
	using AltKeyKeyEqual = NO_ALT_KEY_TYPE; // If specialized for alt key: EqualTo2<AltKeyT,KeyT>
};

} // namespace sfz
