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

namespace sfz {

// approxEqual()
// ------------------------------------------------------------------------------------------------

SFZ_CUDA_CALL bool approxEqual(float lhs, float rhs, float epsilon) noexcept
{
	return (lhs <= (rhs + epsilon)) && (lhs >= (rhs - epsilon));
}

SFZ_CUDA_CALL bool approxEqual(vec2 lhs, vec2 rhs, float epsilon) noexcept
{
	return approxEqual(lhs.x, rhs.x, epsilon)
	    && approxEqual(lhs.y, rhs.y, epsilon);
}

SFZ_CUDA_CALL bool approxEqual(vec3 lhs, vec3 rhs, float epsilon) noexcept
{
	return approxEqual(lhs.x, rhs.x, epsilon)
	    && approxEqual(lhs.y, rhs.y, epsilon)
	    && approxEqual(lhs.z, rhs.z, epsilon);
}

SFZ_CUDA_CALL bool approxEqual(vec4 lhs, vec4 rhs, float epsilon) noexcept
{
	return approxEqual(lhs.x, rhs.x, epsilon)
	    && approxEqual(lhs.y, rhs.y, epsilon)
	    && approxEqual(lhs.z, rhs.z, epsilon)
	    && approxEqual(lhs.w, rhs.w, epsilon);
}

template<uint32_t M, uint32_t N>
SFZ_CUDA_CALL bool approxEqual(const Matrix<float,M,N>& lhs, const Matrix<float,M,N>& rhs,
                               float epsilon) noexcept
{
	for (uint32_t i = 0; i < M; i++) {
		for (uint32_t j = 0; j < N; j++) {
			if (!approxEqual(lhs.at(i, j), rhs.at(i, j), epsilon)) return false;
		}
	}
	return true;
}

SFZ_CUDA_CALL bool approxEqual(Quaternion lhs, Quaternion rhs, float epsilon) noexcept
{
	return approxEqual(lhs.vector, rhs.vector, epsilon);
}

// abs()
// ------------------------------------------------------------------------------------------------

SFZ_CUDA_CALL float abs(float val) noexcept
{
#ifdef SFZ_CUDA_DEVICE_CODE
	return fabsf(val);
#else
	return std::abs(val);
#endif
}

SFZ_CUDA_CALL int32_t abs(int32_t val) noexcept
{
#ifdef SFZ_CUDA_DEVICE_CODE
	return abs(val);
#else
	return std::abs(val);
#endif
}

SFZ_CUDA_CALL vec2 abs(vec2 val) noexcept
{
	val.x = sfz::abs(val.x);
	val.y = sfz::abs(val.y);
	return val;
}

SFZ_CUDA_CALL vec3 abs(vec3 val) noexcept
{
	val.x = sfz::abs(val.x);
	val.y = sfz::abs(val.y);
	val.z = sfz::abs(val.z);
	return val;
}

SFZ_CUDA_CALL vec4 abs(vec4 val) noexcept
{
#ifdef SFZ_CUDA_DEVICE_CODE
	val.x = sfz::abs(val.x);
	val.y = sfz::abs(val.y);
	val.z = sfz::abs(val.z);
	val.w = sfz::abs(val.w);
	return val;
#else
	// Floating points has a dedicated sign bit, simply mask it out
	const __m128 SIGN_BIT_MASK = _mm_set1_ps(-0.0f);
	__m128 tmpReg = _mm_load_ps(val.data());
	tmpReg = _mm_andnot_ps(SIGN_BIT_MASK, tmpReg); // (~(-0.0f)) & value
	_mm_store_ps(val.data(), tmpReg);
	return val;
#endif
}

SFZ_CUDA_CALL vec2i abs(vec2i val) noexcept
{
	val.x = sfz::abs(val.x);
	val.y = sfz::abs(val.y);
	return val;
}

SFZ_CUDA_CALL vec3i abs(vec3i val) noexcept
{
	val.x = sfz::abs(val.x);
	val.y = sfz::abs(val.y);
	val.z = sfz::abs(val.z);
	return val;
}

SFZ_CUDA_CALL vec4i abs(vec4i val) noexcept
{
#ifdef SFZ_CUDA_DEVICE_CODE
	val.x = sfz::abs(val.x);
	val.y = sfz::abs(val.y);
	val.z = sfz::abs(val.z);
	val.w = sfz::abs(val.w);
	return val;
#else
	__m128i tmpReg = _mm_load_si128((const __m128i*)val.data());
	tmpReg = _mm_abs_epi32(tmpReg);
	_mm_store_si128((__m128i*)val.data(), tmpReg);
	return val;
#endif
}

// sgn()
// ------------------------------------------------------------------------------------------------

SFZ_CUDA_CALL float sgn(float val) noexcept
{
	// Union for accessing raw bits of float
	union FloatBitsUnion {
		float value;
		uint32_t bits;
	};
	static_assert(sizeof(FloatBitsUnion) == sizeof(float), "Ensure no padding");
	static_assert(sizeof(FloatBitsUnion) == sizeof(uint32_t), "Ensure no padding");

	// Constants
	const FloatBitsUnion SIGN_BIT_MASK = {-0.0f};
	const FloatBitsUnion FLOAT_ONE = {1.0f};

	// Use bitwise magic to get the sign
	FloatBitsUnion tmp;
	tmp.value = val;

	tmp.bits &= SIGN_BIT_MASK.bits; // Mask out sign bit
	tmp.bits |= FLOAT_ONE.bits; // Add the sign bit to "1.0f"

	return tmp.value;
}

SFZ_CUDA_CALL int32_t sgn(int32_t val) noexcept
{
	return (0 < val) - (val < 0);
}

SFZ_CUDA_CALL vec2 sgn(vec2 val) noexcept
{
	val.x = sfz::sgn(val.x);
	val.y = sfz::sgn(val.y);
	return val;
}

SFZ_CUDA_CALL vec3 sgn(vec3 val) noexcept
{
	val.x = sfz::sgn(val.x);
	val.y = sfz::sgn(val.y);
	val.z = sfz::sgn(val.z);
	return val;
}

SFZ_CUDA_CALL vec4 sgn(vec4 val) noexcept
{
#ifdef SFZ_CUDA_DEVICE_CODE
	val.x = sfz::sgn(val.x);
	val.y = sfz::sgn(val.y);
	val.z = sfz::sgn(val.z);
	val.w = sfz::sgn(val.w);
	return val;
#else
	// Mask out sign bit using bitwise magic and return -1.0f or 1.0f for each element
	const __m128 SIGN_BIT_MASK = _mm_set1_ps(-0.0f);
	const __m128 FLOAT_ONE = _mm_set1_ps(1.0f);
	__m128 tmpReg = _mm_load_ps(val.data());
	tmpReg = _mm_or_ps(FLOAT_ONE, _mm_and_ps(SIGN_BIT_MASK, tmpReg));
	_mm_store_ps(val.data(), tmpReg);
	return val;
#endif
}

SFZ_CUDA_CALL vec2i sgn(vec2i val) noexcept
{
	val.x = sfz::sgn(val.x);
	val.y = sfz::sgn(val.y);
	return val;
}

SFZ_CUDA_CALL vec3i sgn(vec3i val) noexcept
{
	val.x = sfz::sgn(val.x);
	val.y = sfz::sgn(val.y);
	val.z = sfz::sgn(val.z);
	return val;
}

SFZ_CUDA_CALL vec4i sgn(vec4i val) noexcept
{
	val.x = sfz::sgn(val.x);
	val.y = sfz::sgn(val.y);
	val.z = sfz::sgn(val.z);
	val.w = sfz::sgn(val.w);
	return val;

	// TODO: Implement SSE variant
}

// min() - scalars
// ------------------------------------------------------------------------------------------------

SFZ_CUDA_CALL float min(float lhs, float rhs) noexcept
{
	return std::min(lhs, rhs);
}

SFZ_CUDA_CALL int32_t min(int32_t lhs, int32_t rhs) noexcept
{
	return std::min(lhs, rhs);
}

SFZ_CUDA_CALL uint32_t min(uint32_t lhs, uint32_t rhs) noexcept
{
	return std::min(lhs, rhs);
}

// min() - float vectors
// ------------------------------------------------------------------------------------------------

SFZ_CUDA_CALL vec2 min(vec2 lhs, vec2 rhs) noexcept
{
	vec2 tmp;
	tmp.x = sfz::min(lhs.x, rhs.x);
	tmp.y = sfz::min(lhs.y, rhs.y);
	return tmp;
}

SFZ_CUDA_CALL vec3 min(vec3 lhs, vec3 rhs) noexcept
{
	vec3 tmp;
	tmp.x = sfz::min(lhs.x, rhs.x);
	tmp.y = sfz::min(lhs.y, rhs.y);
	tmp.z = sfz::min(lhs.z, rhs.z);
	return tmp;
}

SFZ_CUDA_CALL vec4 min(vec4 lhs, vec4 rhs) noexcept
{
#ifdef SFZ_CUDA_DEVICE_CODE
	vec4 tmp;
	tmp.x = sfz::min(lhs.x, rhs.x);
	tmp.y = sfz::min(lhs.y, rhs.y);
	tmp.z = sfz::min(lhs.z, rhs.z);
	tmp.w = sfz::min(lhs.w, rhs.w);
	return tmp;
#else
	const __m128 lhsReg = _mm_load_ps(lhs.data());
	const __m128 rhsReg = _mm_load_ps(rhs.data());
	const __m128 minReg = _mm_min_ps(lhsReg, rhsReg);
	vec4 tmp;
	_mm_store_ps(tmp.data(), minReg);
	return tmp;
#endif
}

SFZ_CUDA_CALL vec2 min(vec2 lhs, float rhs) noexcept { return min(rhs, lhs); }
SFZ_CUDA_CALL vec2 min(float lhs, vec2 rhs) noexcept
{
	vec2 tmp;
	tmp.x = sfz::min(lhs, rhs.x);
	tmp.y = sfz::min(lhs, rhs.y);
	return tmp;
}

SFZ_CUDA_CALL vec3 min(vec3 lhs, float rhs) noexcept { return min(rhs, lhs); }
SFZ_CUDA_CALL vec3 min(float lhs, vec3 rhs) noexcept
{
	vec3 tmp;
	tmp.x = sfz::min(lhs, rhs.x);
	tmp.y = sfz::min(lhs, rhs.y);
	tmp.z = sfz::min(lhs, rhs.z);
	return tmp;
}

SFZ_CUDA_CALL vec4 min(vec4 lhs, float rhs) noexcept { return min(rhs, lhs); }
SFZ_CUDA_CALL vec4 min(float lhs, vec4 rhs) noexcept
{
#ifdef SFZ_CUDA_DEVICE_CODE
	vec4 tmp;
	tmp.x = sfz::min(lhs, rhs.x);
	tmp.y = sfz::min(lhs, rhs.y);
	tmp.z = sfz::min(lhs, rhs.z);
	tmp.w = sfz::min(lhs, rhs.w);
	return tmp;
#else
	const __m128 lhsReg = _mm_set1_ps(lhs);
	const __m128 rhsReg = _mm_load_ps(rhs.data());
	const __m128 minReg = _mm_min_ps(lhsReg, rhsReg);
	vec4 tmp;
	_mm_store_ps(tmp.data(), minReg);
	return tmp;
#endif
}

// min() - int32_t vectors
// ------------------------------------------------------------------------------------------------

SFZ_CUDA_CALL vec2i min(vec2i lhs, vec2i rhs) noexcept
{
	vec2i tmp;
	tmp.x = sfz::min(lhs.x, rhs.x);
	tmp.y = sfz::min(lhs.y, rhs.y);
	return tmp;
}

SFZ_CUDA_CALL vec3i min(vec3i lhs, vec3i rhs) noexcept
{
	vec3i tmp;
	tmp.x = sfz::min(lhs.x, rhs.x);
	tmp.y = sfz::min(lhs.y, rhs.y);
	tmp.z = sfz::min(lhs.z, rhs.z);
	return tmp;
}

SFZ_CUDA_CALL vec4i min(vec4i lhs, vec4i rhs) noexcept
{
#ifdef SFZ_CUDA_DEVICE_CODE
	vec4i tmp;
	tmp.x = sfz::min(lhs.x, rhs.x);
	tmp.y = sfz::min(lhs.y, rhs.y);
	tmp.z = sfz::min(lhs.z, rhs.z);
	tmp.w = sfz::min(lhs.w, rhs.w);
	return tmp;
#else
	const __m128i lhsReg = _mm_load_si128((const __m128i*)lhs.data());
	const __m128i rhsReg = _mm_load_si128((const __m128i*)rhs.data());
	const __m128i minReg = _mm_min_epi32(lhsReg, rhsReg);
	vec4i tmp;
	_mm_store_si128((__m128i*)tmp.data(), minReg);
	return tmp;
#endif
}

SFZ_CUDA_CALL vec2i min(vec2i lhs, int32_t rhs) noexcept { return min(rhs, lhs); }
SFZ_CUDA_CALL vec2i min(int32_t lhs, vec2i rhs) noexcept
{
	vec2i tmp;
	tmp.x = sfz::min(lhs, rhs.x);
	tmp.y = sfz::min(lhs, rhs.y);
	return tmp;
}

SFZ_CUDA_CALL vec3i min(vec3i lhs, int32_t rhs) noexcept { return min(rhs, lhs); }
SFZ_CUDA_CALL vec3i min(int32_t lhs, vec3i rhs) noexcept
{
	vec3i tmp;
	tmp.x = sfz::min(lhs, rhs.x);
	tmp.y = sfz::min(lhs, rhs.y);
	tmp.z = sfz::min(lhs, rhs.z);
	return tmp;
}

SFZ_CUDA_CALL vec4i min(vec4i lhs, int32_t rhs) noexcept { return min(rhs, lhs); }
SFZ_CUDA_CALL vec4i min(int32_t lhs, vec4i rhs) noexcept
{
#ifdef SFZ_CUDA_DEVICE_CODE
	vec4i tmp;
	tmp.x = sfz::min(lhs, rhs.x);
	tmp.y = sfz::min(lhs, rhs.y);
	tmp.z = sfz::min(lhs, rhs.z);
	tmp.w = sfz::min(lhs, rhs.w);
	return tmp;
#else
	const __m128i lhsReg = _mm_set1_epi32(lhs);
	const __m128i rhsReg = _mm_load_si128((const __m128i*)rhs.data());
	const __m128i minReg = _mm_min_epi32(lhsReg, rhsReg);
	vec4i tmp;
	_mm_store_si128((__m128i*)tmp.data(), minReg);
	return tmp;
#endif
}

// min() - uint32_t vectors
// ------------------------------------------------------------------------------------------------

SFZ_CUDA_CALL vec2u min(vec2u lhs, vec2u rhs) noexcept
{
	vec2u tmp;
	tmp.x = sfz::min(lhs.x, rhs.x);
	tmp.y = sfz::min(lhs.y, rhs.y);
	return tmp;
}

SFZ_CUDA_CALL vec3u min(vec3u lhs, vec3u rhs) noexcept
{
	vec3u tmp;
	tmp.x = sfz::min(lhs.x, rhs.x);
	tmp.y = sfz::min(lhs.y, rhs.y);
	tmp.z = sfz::min(lhs.z, rhs.z);
	return tmp;
}

SFZ_CUDA_CALL vec4u min(vec4u lhs, vec4u rhs) noexcept
{
#ifdef SFZ_CUDA_DEVICE_CODE
	vec4u tmp;
	tmp.x = sfz::min(lhs.x, rhs.x);
	tmp.y = sfz::min(lhs.y, rhs.y);
	tmp.z = sfz::min(lhs.z, rhs.z);
	tmp.w = sfz::min(lhs.w, rhs.w);
	return tmp;
#else
	const __m128i lhsReg = _mm_load_si128((const __m128i*)lhs.data());
	const __m128i rhsReg = _mm_load_si128((const __m128i*)rhs.data());
	const __m128i minReg = _mm_min_epu32(lhsReg, rhsReg);
	vec4u tmp;
	_mm_store_si128((__m128i*)tmp.data(), minReg);
	return tmp;
#endif
}

SFZ_CUDA_CALL vec2u min(vec2u lhs, uint32_t rhs) noexcept { return min(rhs, lhs); }
SFZ_CUDA_CALL vec2u min(uint32_t lhs, vec2u rhs) noexcept
{
	vec2u tmp;
	tmp.x = sfz::min(lhs, rhs.x);
	tmp.y = sfz::min(lhs, rhs.y);
	return tmp;
}

SFZ_CUDA_CALL vec3u min(vec3u lhs, uint32_t rhs) noexcept { return min(rhs, lhs); }
SFZ_CUDA_CALL vec3u min(uint32_t lhs, vec3u rhs) noexcept
{
	vec3u tmp;
	tmp.x = sfz::min(lhs, rhs.x);
	tmp.y = sfz::min(lhs, rhs.y);
	tmp.z = sfz::min(lhs, rhs.z);
	return tmp;
}

SFZ_CUDA_CALL vec4u min(vec4u lhs, uint32_t rhs) noexcept { return min(rhs, lhs); }
SFZ_CUDA_CALL vec4u min(uint32_t lhs, vec4u rhs) noexcept
{
#ifdef SFZ_CUDA_DEVICE_CODE
	vec4u tmp;
	tmp.x = sfz::min(lhs, rhs.x);
	tmp.y = sfz::min(lhs, rhs.y);
	tmp.z = sfz::min(lhs, rhs.z);
	tmp.w = sfz::min(lhs, rhs.w);
	return tmp;
#else
	const __m128i lhsReg = _mm_set1_epi32(*(int32_t*)&lhs);
	const __m128i rhsReg = _mm_load_si128((const __m128i*)rhs.data());
	const __m128i minReg = _mm_min_epi32(lhsReg, rhsReg);
	vec4u tmp;
	_mm_store_si128((__m128i*)tmp.data(), minReg);
	return tmp;
#endif
}

// max() - scalars
// ------------------------------------------------------------------------------------------------

SFZ_CUDA_CALL float max(float lhs, float rhs) noexcept
{
	return std::max(lhs, rhs);
}

SFZ_CUDA_CALL int32_t max(int32_t lhs, int32_t rhs) noexcept
{
	return std::max(lhs, rhs);
}

SFZ_CUDA_CALL uint32_t max(uint32_t lhs, uint32_t rhs) noexcept
{
	return std::max(lhs, rhs);
}

// max() - float vectors
// ------------------------------------------------------------------------------------------------

SFZ_CUDA_CALL vec2 max(vec2 lhs, vec2 rhs) noexcept
{
	vec2 tmp;
	tmp.x = sfz::max(lhs.x, rhs.x);
	tmp.y = sfz::max(lhs.y, rhs.y);
	return tmp;
}

SFZ_CUDA_CALL vec3 max(vec3 lhs, vec3 rhs) noexcept
{
	vec3 tmp;
	tmp.x = sfz::max(lhs.x, rhs.x);
	tmp.y = sfz::max(lhs.y, rhs.y);
	tmp.z = sfz::max(lhs.z, rhs.z);
	return tmp;
}

SFZ_CUDA_CALL vec4 max(vec4 lhs, vec4 rhs) noexcept
{
#ifdef SFZ_CUDA_DEVICE_CODE
	vec4 tmp;
	tmp.x = sfz::max(lhs.x, rhs.x);
	tmp.y = sfz::max(lhs.y, rhs.y);
	tmp.z = sfz::max(lhs.z, rhs.z);
	tmp.w = sfz::max(lhs.w, rhs.w);
	return tmp;
#else
	const __m128 lhsReg = _mm_load_ps(lhs.data());
	const __m128 rhsReg = _mm_load_ps(rhs.data());
	const __m128 maxReg = _mm_max_ps(lhsReg, rhsReg);
	vec4 tmp;
	_mm_store_ps(tmp.data(), maxReg);
	return tmp;
#endif
}

SFZ_CUDA_CALL vec2 max(vec2 lhs, float rhs) noexcept { return max(rhs, lhs); }
SFZ_CUDA_CALL vec2 max(float lhs, vec2 rhs) noexcept
{
	vec2 tmp;
	tmp.x = sfz::max(lhs, rhs.x);
	tmp.y = sfz::max(lhs, rhs.y);
	return tmp;
}

SFZ_CUDA_CALL vec3 max(vec3 lhs, float rhs) noexcept { return max(rhs, lhs); }
SFZ_CUDA_CALL vec3 max(float lhs, vec3 rhs) noexcept
{
	vec3 tmp;
	tmp.x = sfz::max(lhs, rhs.x);
	tmp.y = sfz::max(lhs, rhs.y);
	tmp.z = sfz::max(lhs, rhs.z);
	return tmp;
}

SFZ_CUDA_CALL vec4 max(vec4 lhs, float rhs) noexcept { return max(rhs, lhs); }
SFZ_CUDA_CALL vec4 max(float lhs, vec4 rhs) noexcept
{
#ifdef SFZ_CUDA_DEVICE_CODE
	vec4 tmp;
	tmp.x = sfz::max(lhs, rhs.x);
	tmp.y = sfz::max(lhs, rhs.y);
	tmp.z = sfz::max(lhs, rhs.z);
	tmp.w = sfz::max(lhs, rhs.w);
	return tmp;
#else
	const __m128 lhsReg = _mm_set1_ps(lhs);
	const __m128 rhsReg = _mm_load_ps(rhs.data());
	const __m128 maxReg = _mm_max_ps(lhsReg, rhsReg);
	vec4 tmp;
	_mm_store_ps(tmp.data(), maxReg);
	return tmp;
#endif
}

// max() - int32_t vectors
// ------------------------------------------------------------------------------------------------

SFZ_CUDA_CALL vec2i max(vec2i lhs, vec2i rhs) noexcept
{
	vec2i tmp;
	tmp.x = sfz::max(lhs.x, rhs.x);
	tmp.y = sfz::max(lhs.y, rhs.y);
	return tmp;
}

SFZ_CUDA_CALL vec3i max(vec3i lhs, vec3i rhs) noexcept
{
	vec3i tmp;
	tmp.x = sfz::max(lhs.x, rhs.x);
	tmp.y = sfz::max(lhs.y, rhs.y);
	tmp.z = sfz::max(lhs.z, rhs.z);
	return tmp;
}

SFZ_CUDA_CALL vec4i max(vec4i lhs, vec4i rhs) noexcept
{
#ifdef SFZ_CUDA_DEVICE_CODE
	vec4i tmp;
	tmp.x = sfz::max(lhs.x, rhs.x);
	tmp.y = sfz::max(lhs.y, rhs.y);
	tmp.z = sfz::max(lhs.z, rhs.z);
	tmp.w = sfz::max(lhs.w, rhs.w);
	return tmp;
#else
	const __m128i lhsReg = _mm_load_si128((const __m128i*)lhs.data());
	const __m128i rhsReg = _mm_load_si128((const __m128i*)rhs.data());
	const __m128i maxReg = _mm_max_epi32(lhsReg, rhsReg);
	vec4i tmp;
	_mm_store_si128((__m128i*)tmp.data(), maxReg);
	return tmp;
#endif
}

SFZ_CUDA_CALL vec2i max(vec2i lhs, int32_t rhs) noexcept { return max(rhs, lhs); }
SFZ_CUDA_CALL vec2i max(int32_t lhs, vec2i rhs) noexcept
{
	vec2i tmp;
	tmp.x = sfz::max(lhs, rhs.x);
	tmp.y = sfz::max(lhs, rhs.y);
	return tmp;
}

SFZ_CUDA_CALL vec3i max(vec3i lhs, int32_t rhs) noexcept { return max(rhs, lhs); }
SFZ_CUDA_CALL vec3i max(int32_t lhs, vec3i rhs) noexcept
{
	vec3i tmp;
	tmp.x = sfz::max(lhs, rhs.x);
	tmp.y = sfz::max(lhs, rhs.y);
	tmp.z = sfz::max(lhs, rhs.z);
	return tmp;
}

SFZ_CUDA_CALL vec4i max(vec4i lhs, int32_t rhs) noexcept { return max(rhs, lhs); }
SFZ_CUDA_CALL vec4i max(int32_t lhs, vec4i rhs) noexcept
{
#ifdef SFZ_CUDA_DEVICE_CODE
	vec4i tmp;
	tmp.x = sfz::max(lhs, rhs.x);
	tmp.y = sfz::max(lhs, rhs.y);
	tmp.z = sfz::max(lhs, rhs.z);
	tmp.w = sfz::max(lhs, rhs.w);
	return tmp;
#else
	const __m128i lhsReg = _mm_set1_epi32(lhs);
	const __m128i rhsReg = _mm_load_si128((const __m128i*)rhs.data());
	const __m128i maxReg = _mm_max_epi32(lhsReg, rhsReg);
	vec4i tmp;
	_mm_store_si128((__m128i*)tmp.data(), maxReg);
	return tmp;
#endif
}

// max() - uint32_t vectors
// ------------------------------------------------------------------------------------------------

SFZ_CUDA_CALL vec2u max(vec2u lhs, vec2u rhs) noexcept
{
	vec2u tmp;
	tmp.x = sfz::max(lhs.x, rhs.x);
	tmp.y = sfz::max(lhs.y, rhs.y);
	return tmp;
}

SFZ_CUDA_CALL vec3u max(vec3u lhs, vec3u rhs) noexcept
{
	vec3u tmp;
	tmp.x = sfz::max(lhs.x, rhs.x);
	tmp.y = sfz::max(lhs.y, rhs.y);
	tmp.z = sfz::max(lhs.z, rhs.z);
	return tmp;
}

SFZ_CUDA_CALL vec4u max(vec4u lhs, vec4u rhs) noexcept
{
#ifdef SFZ_CUDA_DEVICE_CODE
	vec4u tmp;
	tmp.x = sfz::max(lhs.x, rhs.x);
	tmp.y = sfz::max(lhs.y, rhs.y);
	tmp.z = sfz::max(lhs.z, rhs.z);
	tmp.w = sfz::max(lhs.w, rhs.w);
	return tmp;
#else
	const __m128i lhsReg = _mm_load_si128((const __m128i*)lhs.data());
	const __m128i rhsReg = _mm_load_si128((const __m128i*)rhs.data());
	const __m128i maxReg = _mm_max_epu32(lhsReg, rhsReg);
	vec4u tmp;
	_mm_store_si128((__m128i*)tmp.data(), maxReg);
	return tmp;
#endif
}

SFZ_CUDA_CALL vec2u max(vec2u lhs, uint32_t rhs) noexcept { return max(rhs, lhs); }
SFZ_CUDA_CALL vec2u max(uint32_t lhs, vec2u rhs) noexcept
{
	vec2u tmp;
	tmp.x = sfz::max(lhs, rhs.x);
	tmp.y = sfz::max(lhs, rhs.y);
	return tmp;
}

SFZ_CUDA_CALL vec3u max(vec3u lhs, uint32_t rhs) noexcept { return max(rhs, lhs); }
SFZ_CUDA_CALL vec3u max(uint32_t lhs, vec3u rhs) noexcept
{
	vec3u tmp;
	tmp.x = sfz::max(lhs, rhs.x);
	tmp.y = sfz::max(lhs, rhs.y);
	tmp.z = sfz::max(lhs, rhs.z);
	return tmp;
}

SFZ_CUDA_CALL vec4u max(vec4u lhs, uint32_t rhs) noexcept { return max(rhs, lhs); }
SFZ_CUDA_CALL vec4u max(uint32_t lhs, vec4u rhs) noexcept
{
#ifdef SFZ_CUDA_DEVICE_CODE
	vec4u tmp;
	tmp.x = sfz::max(lhs, rhs.x);
	tmp.y = sfz::max(lhs, rhs.y);
	tmp.z = sfz::max(lhs, rhs.z);
	tmp.w = sfz::max(lhs, rhs.w);
	return tmp;
#else
	const __m128i lhsReg = _mm_set1_epi32(*(int32_t*)&lhs);
	const __m128i rhsReg = _mm_load_si128((const __m128i*)rhs.data());
	const __m128i maxReg = _mm_max_epu32(lhsReg, rhsReg);
	vec4u tmp;
	_mm_store_si128((__m128i*)tmp.data(), maxReg);
	return tmp;
#endif
}

// minElement()
// ------------------------------------------------------------------------------------------------

SFZ_CUDA_CALL float minElement(vec2 val) noexcept
{
	return sfz::min(val.x, val.y);
}

SFZ_CUDA_CALL float minElement(vec3 val) noexcept
{
	return sfz::min(sfz::min(val.x, val.y), val.z);
}

SFZ_CUDA_CALL float minElement(vec4 val) noexcept
{
	return sfz::min(sfz::min(sfz::min(val.x, val.y), val.z), val.w);
}

SFZ_CUDA_CALL int32_t minElement(vec2i val) noexcept
{
	return sfz::min(val.x, val.y);
}

SFZ_CUDA_CALL int32_t minElement(vec3i val) noexcept
{
	return sfz::min(sfz::min(val.x, val.y), val.z);
}

SFZ_CUDA_CALL int32_t minElement(vec4i val) noexcept
{
	return sfz::min(sfz::min(sfz::min(val.x, val.y), val.z), val.w);
}

SFZ_CUDA_CALL uint32_t minElement(vec2u val) noexcept
{
	return sfz::min(val.x, val.y);
}

SFZ_CUDA_CALL uint32_t minElement(vec3u val) noexcept
{
	return sfz::min(sfz::min(val.x, val.y), val.z);
}

SFZ_CUDA_CALL uint32_t minElement(vec4u val) noexcept
{
	return sfz::min(sfz::min(sfz::min(val.x, val.y), val.z), val.w);
}

// maxElement()
// ------------------------------------------------------------------------------------------------

SFZ_CUDA_CALL float maxElement(vec2 val) noexcept
{
	return sfz::max(val.x, val.y);
}

SFZ_CUDA_CALL float maxElement(vec3 val) noexcept
{
	return sfz::max(sfz::max(val.x, val.y), val.z);
}

SFZ_CUDA_CALL float maxElement(vec4 val) noexcept
{
	return sfz::max(sfz::max(sfz::max(val.x, val.y), val.z), val.w);
}

SFZ_CUDA_CALL int32_t maxElement(vec2i val) noexcept
{
	return sfz::max(val.x, val.y);
}

SFZ_CUDA_CALL int32_t maxElement(vec3i val) noexcept
{
	return sfz::max(sfz::max(val.x, val.y), val.z);
}

SFZ_CUDA_CALL int32_t maxElement(vec4i val) noexcept
{
	return sfz::max(sfz::max(sfz::max(val.x, val.y), val.z), val.w);
}

SFZ_CUDA_CALL uint32_t maxElement(vec2u val) noexcept
{
	return sfz::max(val.x, val.y);
}

SFZ_CUDA_CALL uint32_t maxElement(vec3u val) noexcept
{
	return sfz::max(sfz::max(val.x, val.y), val.z);
}

SFZ_CUDA_CALL uint32_t maxElement(vec4u val) noexcept
{
	return sfz::max(sfz::max(sfz::max(val.x, val.y), val.z), val.w);
}

// clamp() & saturate()
// ------------------------------------------------------------------------------------------------

template<typename ArgT, typename LimitT>
SFZ_CUDA_CALL ArgT clamp(const ArgT& value, const LimitT& minValue, const LimitT& maxValue) noexcept
{
	return sfz::max(minValue, sfz::min(value, maxValue));
}

SFZ_CUDA_CALL float saturate(float value) noexcept
{
#ifdef SFZ_CUDA_DEVICE_CODE
	return __saturatef(value);
#else
	return sfz::clamp<float,float>(value, 0.0f, 1.0f);
#endif
}

SFZ_CUDA_CALL vec2 saturate(vec2 value) noexcept
{
	return vec2(sfz::saturate(value.x), sfz::saturate(value.y));
}

SFZ_CUDA_CALL vec3 saturate(vec3 value) noexcept
{
	return vec3(sfz::saturate(value.x), sfz::saturate(value.y), sfz::saturate(value.z));
}

SFZ_CUDA_CALL vec4 saturate(vec4 value) noexcept
{
	return vec4(sfz::saturate(value.x), sfz::saturate(value.y),
	            sfz::saturate(value.z), sfz::saturate(value.w));
}

// lerp()
// ------------------------------------------------------------------------------------------------

template<typename ArgT, typename FloatT>
SFZ_CUDA_CALL ArgT lerp(ArgT v0, ArgT v1, FloatT t) noexcept
{
	return (FloatT(1) - t) * v0 + t * v1;
}

template<>
SFZ_CUDA_CALL float lerp(float v0, float v1, float t) noexcept
{
#ifdef SFZ_CUDA_DEVICE_CODE
	// https://devblogs.nvidia.com/parallelforall/lerp-faster-cuda/
	return fma(t, v1, fma(-t, v0, v0));
#else
	return (1.0f - t) * v0 + t * v1;
#endif
}

template<>
SFZ_CUDA_CALL vec2 lerp(vec2 v0, vec2 v1, float t) noexcept
{
	return vec2(sfz::lerp(v0.x, v1.x, t),
	            sfz::lerp(v0.y, v1.y, t));
}

template<>
SFZ_CUDA_CALL vec3 lerp(vec3 v0, vec3 v1, float t) noexcept
{
	return vec3(sfz::lerp(v0.x, v1.x, t),
	            sfz::lerp(v0.y, v1.y, t),
	            sfz::lerp(v0.z, v1.z, t));
}

template<>
SFZ_CUDA_CALL vec4 lerp(vec4 v0, vec4 v1, float t) noexcept
{
	return vec4(sfz::lerp(v0.x, v1.x, t),
	            sfz::lerp(v0.y, v1.y, t),
	            sfz::lerp(v0.z, v1.z, t),
	            sfz::lerp(v0.w, v1.w, t));

	// TODO: SSE version?
}

template<>
SFZ_CUDA_CALL Quaternion lerp(Quaternion q0, Quaternion q1, float t) noexcept
{
	Quaternion tmp;
	tmp.vector = sfz::lerp(q0.vector, q1.vector, t);
	tmp = normalize(tmp);
	return tmp;
}

// fma()
// ------------------------------------------------------------------------------------------------

SFZ_CUDA_CALL float fma(float a, float b, float c) noexcept
{
#ifdef SFZ_CUDA_DEVICE_CODE
	return fmaf(a, b, c);
#else
	return a * b + c;
#endif
}

SFZ_CUDA_CALL vec2 fma(vec2 a, vec2 b, vec2 c) noexcept
{
	return vec2(sfz::fma(a.x, b.x, c.x),
	            sfz::fma(a.y, b.y, c.y));
}

SFZ_CUDA_CALL vec3 fma(vec3 a, vec3 b, vec3 c) noexcept
{
	return vec3(sfz::fma(a.x, b.x, c.x),
	            sfz::fma(a.y, b.y, c.y),
	            sfz::fma(a.z, b.z, c.z));
}

SFZ_CUDA_CALL vec4 fma(vec4 a, vec4 b, vec4 c) noexcept
{
	return vec4(sfz::fma(a.x, b.x, c.x),
	            sfz::fma(a.y, b.y, c.y),
	            sfz::fma(a.z, b.z, c.z),
	            sfz::fma(a.w, b.w, c.w));
}

} // namespace sfz
