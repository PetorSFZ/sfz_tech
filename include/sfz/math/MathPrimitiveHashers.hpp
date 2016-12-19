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

#include <functional>

#include "sfz/math/Matrix.hpp"
#include "sfz/math/Vector.hpp"

namespace sfz {

// Vector hash function
// ------------------------------------------------------------------------------------------------

template<typename T, uint32_t N>
size_t hash(const Vector<T,N>& vector) noexcept;

// Matrix hash function
// ------------------------------------------------------------------------------------------------

template<typename T, uint32_t H, uint32_t W>
size_t hash(const Matrix<T,H,W>& matrix) noexcept;

} // namespace sfz

namespace std {

// Vector hash struct
// ------------------------------------------------------------------------------------------------

template<typename T, uint32_t N>
struct hash<sfz::Vector<T,N>> {
	size_t operator() (const sfz::Vector<T,N>& vector) const noexcept;
};

// Matrix hash struct
// ------------------------------------------------------------------------------------------------

template<typename T, uint32_t H, uint32_t W>
struct hash<sfz::Matrix<T,H,W>> {
	size_t operator() (const sfz::Matrix<T,H,W>& matrix) const noexcept;
};

} // namespace std

#include "sfz/math/MathPrimitiveHashers.inl"
