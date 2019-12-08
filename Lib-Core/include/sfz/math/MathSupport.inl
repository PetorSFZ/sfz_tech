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
