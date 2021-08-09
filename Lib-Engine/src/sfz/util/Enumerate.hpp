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

#include <utility>

#include <sfz.h>

namespace sfz {

// enumerate() helper structs
// ------------------------------------------------------------------------------------------------

template<typename T>
struct IndexedElement final {
	u64 idx;
	T element;
};

template<typename IteratorT>
struct EnumerateIterator final {
	using T = decltype(*std::declval<IteratorT>());

	u64 idx;
	IteratorT iterator;

	bool operator!= (const EnumerateIterator& other) const { return iterator != other.iterator; }
	void operator++ () { idx += 1; ++iterator; }
	IndexedElement<T> operator* () const { return {idx, *iterator}; }
};

template<typename IterableT, typename IteratorT>
struct EnumerateIterable final {
	IterableT iterable;
	EnumerateIterator<IteratorT> begin() { return { 0, std::begin(iterable) }; }
	EnumerateIterator<IteratorT> end() { return { 0, std::end(iterable) }; }
};

// enumerate() function
// ------------------------------------------------------------------------------------------------

/// Function used to access both element and index when iterating over a container using a C++
/// for-each loop. This wrapper should hopefully have a negligible cost over just using a standard
/// for-each loop it, at the very least no copies or moves should be performed (confirmed by unit
/// test, see Enumerate_Tests.cpp).
///
/// ExampleUsage:
/// Array<u32> elements = /* ... */
/// for (auto e : sfz::enumerate(elements)) {
/// 	// Access index with e.idx, element with e.element.
/// }
///
/// If you have a fancy C++17 compiler you can also use:
/// for (auto[idx, element] : sfz::enumerate(elements)) {
/// 	// Access index with idx, element with element.
/// }
///
/// This function is heavily based on the work of Nathan Reed:
/// http://www.reedbeta.com/blog/python-like-enumerate-in-cpp17/
///
/// The main differences are:
/// * Compiles with C++14
/// 	* Still useful in C++14 due to tuple changes below
/// * Does not use std::tuple
/// 	* Less header bloat
/// 	* Sensible names for the members, i.e. "e.idx" instead of "std::get<0>(e)".
/// * Slightly different coding style, non-functional change.
template <typename IterableT, typename IteratorT = decltype(std::begin(std::declval<IterableT>()))>
constexpr EnumerateIterable<IterableT, IteratorT> enumerate(IterableT&& iterable)
{
	return { std::forward<IterableT>(iterable) };
}

} // namespace sfz
