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

// abs()
// ------------------------------------------------------------------------------------------------

SFZ_CUDA_CALL float abs(float val) noexcept
{
#ifdef SFZ_CUDA_DEVICE_CODE
	return fabsf(val);
#else
	return std::fabs(val);
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
#elif defined(__EMSCRIPTEN__) || defined(SFZ_IOS)
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

SFZ_CUDA_CALL vec2_i32 abs(vec2_i32 val) noexcept
{
	val.x = sfz::abs(val.x);
	val.y = sfz::abs(val.y);
	return val;
}

SFZ_CUDA_CALL vec3_i32 abs(vec3_i32 val) noexcept
{
	val.x = sfz::abs(val.x);
	val.y = sfz::abs(val.y);
	val.z = sfz::abs(val.z);
	return val;
}

SFZ_CUDA_CALL vec4_i32 abs(vec4_i32 val) noexcept
{
#ifdef SFZ_CUDA_DEVICE_CODE
	val.x = sfz::abs(val.x);
	val.y = sfz::abs(val.y);
	val.z = sfz::abs(val.z);
	val.w = sfz::abs(val.w);
	return val;
#elif defined(__EMSCRIPTEN__) || defined(SFZ_IOS)
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
#elif defined(__EMSCRIPTEN__) || defined(SFZ_IOS)
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

SFZ_CUDA_CALL vec2_i32 sgn(vec2_i32 val) noexcept
{
	val.x = sfz::sgn(val.x);
	val.y = sfz::sgn(val.y);
	return val;
}

SFZ_CUDA_CALL vec3_i32 sgn(vec3_i32 val) noexcept
{
	val.x = sfz::sgn(val.x);
	val.y = sfz::sgn(val.y);
	val.z = sfz::sgn(val.z);
	return val;
}

SFZ_CUDA_CALL vec4_i32 sgn(vec4_i32 val) noexcept
{
	val.x = sfz::sgn(val.x);
	val.y = sfz::sgn(val.y);
	val.z = sfz::sgn(val.z);
	val.w = sfz::sgn(val.w);
	return val;

	// TODO: Implement SSE variant
}

// minElement()
// ------------------------------------------------------------------------------------------------

SFZ_CUDA_CALL float minElement(vec2 val) noexcept
{
	return sfzMin(val.x, val.y);
}

SFZ_CUDA_CALL float minElement(vec3 val) noexcept
{
	return sfzMin(sfzMin(val.x, val.y), val.z);
}

SFZ_CUDA_CALL float minElement(vec4 val) noexcept
{
	return sfzMin(sfzMin(sfzMin(val.x, val.y), val.z), val.w);
}

SFZ_CUDA_CALL int32_t minElement(vec2_i32 val) noexcept
{
	return sfzMin(val.x, val.y);
}

SFZ_CUDA_CALL int32_t minElement(vec3_i32 val) noexcept
{
	return sfzMin(sfzMin(val.x, val.y), val.z);
}

SFZ_CUDA_CALL int32_t minElement(vec4_i32 val) noexcept
{
	return sfzMin(sfzMin(sfzMin(val.x, val.y), val.z), val.w);
}

SFZ_CUDA_CALL uint32_t minElement(vec2_u32 val) noexcept
{
	return sfzMin(val.x, val.y);
}

SFZ_CUDA_CALL uint32_t minElement(vec3_u32 val) noexcept
{
	return sfzMin(sfzMin(val.x, val.y), val.z);
}

SFZ_CUDA_CALL uint32_t minElement(vec4_u32 val) noexcept
{
	return sfzMin(sfzMin(sfzMin(val.x, val.y), val.z), val.w);
}

// maxElement()
// ------------------------------------------------------------------------------------------------

SFZ_CUDA_CALL float maxElement(vec2 val) noexcept
{
	return sfzMax(val.x, val.y);
}

SFZ_CUDA_CALL float maxElement(vec3 val) noexcept
{
	return sfzMax(sfzMax(val.x, val.y), val.z);
}

SFZ_CUDA_CALL float maxElement(vec4 val) noexcept
{
	return sfzMax(sfzMax(sfzMax(val.x, val.y), val.z), val.w);
}

SFZ_CUDA_CALL int32_t maxElement(vec2_i32 val) noexcept
{
	return sfzMax(val.x, val.y);
}

SFZ_CUDA_CALL int32_t maxElement(vec3_i32 val) noexcept
{
	return sfzMax(sfzMax(val.x, val.y), val.z);
}

SFZ_CUDA_CALL int32_t maxElement(vec4_i32 val) noexcept
{
	return sfzMax(sfzMax(sfzMax(val.x, val.y), val.z), val.w);
}

SFZ_CUDA_CALL uint32_t maxElement(vec2_u32 val) noexcept
{
	return sfzMax(val.x, val.y);
}

SFZ_CUDA_CALL uint32_t maxElement(vec3_u32 val) noexcept
{
	return sfzMax(sfzMax(val.x, val.y), val.z);
}

SFZ_CUDA_CALL uint32_t maxElement(vec4_u32 val) noexcept
{
	return sfzMax(sfzMax(sfzMax(val.x, val.y), val.z), val.w);
}

// clamp() & saturate()
// ------------------------------------------------------------------------------------------------

template<typename ArgT, typename LimitT>
SFZ_CUDA_CALL ArgT clamp(const ArgT& value, const LimitT& minValue, const LimitT& maxValue) noexcept
{
	return sfzMax(minValue, sfzMin(value, maxValue));
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

// rotateTowards()
// ------------------------------------------------------------------------------------------------

SFZ_CUDA_CALL vec3 rotateTowardsRad(vec3 inDir, vec3 targetDir, float angleRads) noexcept
{
	sfz_assert(equalsApprox(length(inDir), 1.0f));
	sfz_assert(equalsApprox(length(targetDir), 1.0f));
	sfz_assert(dot(inDir, targetDir) >= -0.99f);
	sfz_assert(angleRads >= 0.0f);
	sfz_assert(angleRads < PI);
	vec3 axis = cross(inDir, targetDir);
	sfz_assert(!equalsApprox(axis, vec3(0.0f)));
	Quaternion rotQuat = Quaternion::rotationRad(axis, angleRads);
	vec3 newDir = rotate(rotQuat, inDir);
	return newDir;
}

SFZ_CUDA_CALL vec3 rotateTowardsRadClampSafe(vec3 inDir, vec3 targetDir, float angleRads) noexcept
{
	sfz_assert(angleRads >= 0.0f);
	sfz_assert(angleRads < PI);

	vec3 inDirNorm = normalizeSafe(inDir);
	vec3 targetDirNorm = normalizeSafe(targetDir);
	sfz_assert(!equalsApprox(inDirNorm, vec3(0.0f)));
	sfz_assert(!equalsApprox(targetDirNorm, vec3(0.0f)));

	// Case where vectors are the same, just return the target dir
	if (equalsApprox(inDirNorm, targetDirNorm)) return targetDirNorm;

	// Case where vectors are exact opposite, slightly nudge input a bit
	if (equalsApprox(inDirNorm, -targetDirNorm)) {
		inDirNorm = normalize(inDir + (vec3(1.0f) - inDirNorm) * 0.025f);
		sfz_assert(!equalsApprox(inDirNorm, -targetDirNorm));
	}

	// Case where angle is larger than the angle between the vectors
	if (angleRads >= acos(dot(inDirNorm, targetDirNorm))) return targetDirNorm;

	// At this point all annoying cases should be handled, just run the normal routine
	return rotateTowardsRad(inDirNorm, targetDirNorm, angleRads);
}

SFZ_CUDA_CALL vec3 rotateTowardsDeg(vec3 inDir, vec3 targetDir, float angleDegs) noexcept
{
	return rotateTowardsRad(inDir, targetDir, DEG_TO_RAD * angleDegs);
}

SFZ_CUDA_CALL vec3 rotateTowardsDegClampSafe(vec3 inDir, vec3 targetDir, float angleDegs) noexcept
{
	return rotateTowardsRadClampSafe(inDir, targetDir, DEG_TO_RAD * angleDegs);
}

} // namespace sfz
