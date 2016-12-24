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

#include "sfz/CudaCompatibility.hpp"

namespace sfz {

// Quaternion struct
// ------------------------------------------------------------------------------------------------

struct Quaternion final {
	
	// Members
	// --------------------------------------------------------------------------------------------

	/// a + b*i + c*j + d*k
	float a, b, c, d;

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	Quaternion() noexcept = default;
	Quaternion(const Quaternion&) noexcept = default;
	Quaternion& operator= (const Quaternion&) noexcept = default;
	~Quaternion() noexcept = default;

	SFZ_CUDA_CALL Quaternion(float a, float b, float c, float d) noexcept;
};

// Operators (comparison)
// ------------------------------------------------------------------------------------------------

SFZ_CUDA_CALL bool operator== (const Quaternion& lhs, const Quaternion& rhs) noexcept;
SFZ_CUDA_CALL bool operator!= (const Quaternion& lhs, const Quaternion& rhs) noexcept;

// Operators (arithmetic & assignment)
// ------------------------------------------------------------------------------------------------

SFZ_CUDA_CALL Quaternion& operator+= (Quaternion& left, const Quaternion& right) noexcept;
SFZ_CUDA_CALL Quaternion& operator-= (Quaternion& left, const Quaternion& right) noexcept;

// Operators (arithmetic)
// ------------------------------------------------------------------------------------------------

SFZ_CUDA_CALL Quaternion operator+ (const Quaternion& left, const Quaternion& right) noexcept;
SFZ_CUDA_CALL Quaternion operator- (const Quaternion& left, const Quaternion& right) noexcept;

} // namespace sfz

#include "sfz/math/Quaternion.inl"
