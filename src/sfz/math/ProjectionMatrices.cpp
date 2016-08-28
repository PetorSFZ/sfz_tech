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

#include "sfz/math/ProjectionMatrices.hpp"

namespace sfz {

// Projection matrices (Standard OpenGL [-1, 1] clip space)
// ------------------------------------------------------------------------------------------------

mat4 orthogonalProjectionGL(float l, float b, float r, float t, float n, float f) noexcept
{
	return mat4{
		{2.0f / (r - l),  0.0f,            0.0f,             -((r + l) / (r - l))},
		{0.0f,            2.0f / (t - b),  0.0f,             -((t + b) / (t - b))},
		{0.0f,            0.0f,            -2.0f / (f - n),  -(f + n) / (f - n)},
		{0.0f,            0.0f,            0.0f,             1.0f}
	};
}

mat4 orthogonalProjectionGL(vec3 leftBottomNear, vec3 rightTopFar) noexcept
{
	return orthogonalProjectionGL(leftBottomNear.x, leftBottomNear.y, rightTopFar.x, rightTopFar.y,
	                              leftBottomNear.z, rightTopFar.z);
}

mat3 orthogonalProjection2DGL(vec2 center, vec2 dimensions) noexcept
{
	float a = 2.0f / dimensions.x;
	float b = 2.0f / dimensions.y;
	return mat3{
		{a,     0.0f,  -(center.x * a)},
		{0.0f,  b,     -(center.y * b)},
		{0.0f,  0.0f,  1.0f}
	};
}

mat4 perspectiveProjectionGL(float l, float b, float r, float t, float n, float f) noexcept
{
	return mat4{
		{2.0f * n / (r - l),  0.0f,                (r + l) / (r - l),   0.0f},
		{0.0f,                2.0f * n / (t - b),  (t + b) / (t - b),   0.0f},
		{0.0f,                0.0f,                -(f + n) / (f - n),  -2.0f * f * n / (f - n)},
		{0.0f,                0.0f,                -1.0f,               0.0f}
	};
}

mat4 perspectiveProjectionGL(float yFovDeg, float aspectRatio, float zNear, float zFar) noexcept
{
	float yMax = zNear * std::tan(yFovDeg * (PI() / 360.0f));
	float xMax = yMax * aspectRatio;
	return perspectiveProjectionGL(-xMax, -yMax, xMax, yMax, zNear, zFar);
}

// Projection matrices (D3D/Vulkan [0, 1] left-handed clip space, right handed view space)
// ------------------------------------------------------------------------------------------------

mat4 perspectiveProjectionVkD3d(float l, float b, float r, float t, float n, float f) noexcept
{
	return mat4{
		{(2.0f * n) / (r - l),  0.0f,                  -(r + l) / (r - l),  0.0f},
		{0.0f,                  (2.0f * n) / (t - b),  -(t + b) / (t - b),  0.0f},
		{0.0f,                  0.0f,                  f / (n - f),         n * f / (n - f)},
		{0.0f,                  0.0f,                  -1.0f,               0.0f}
	};
}

mat4 perspectiveProjectionVkD3d(float yFovDeg, float aspectRatio, float zNear, float zFar) noexcept
{
	float yMax = zNear * std::tan(yFovDeg * (PI() / 360.f));
	float xMax = yMax * aspectRatio;
	return perspectiveProjectionVkD3d(-xMax, -yMax, xMax, yMax, zNear, zFar);
}

mat4 reversePerspectiveProjectionVkD3d(float l, float b, float r, float t, float n, float f) noexcept
{
	return perspectiveProjectionVkD3d(l, b, r, t, f, n);
}

mat4 reversePerspectiveProjectionVkD3d(float yFovDeg, float aspectRatio, float zNear, float zFar) noexcept
{
	return perspectiveProjectionVkD3d(yFovDeg, aspectRatio, zFar, zNear);
}

} // namespace sfz
