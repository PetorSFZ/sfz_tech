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

#include "sfz/math/MathSupport.hpp"

namespace sfz {

// GL View matrix (OGL right-handed, negative z into screen, positive x to the right)
// ------------------------------------------------------------------------------------------------

mat4 viewMatrixGL(const vec3& origin, const vec3& dir, const vec3& up) noexcept
{
	vec3 zAxis = -normalize(dir); // Away from screen
	vec3 xAxis = normalize(cross(up, zAxis)); // To the right
	vec3 yAxis = cross(zAxis, xAxis); // Up

	return mat4(xAxis.x,  xAxis.y,  xAxis.z,  -dot(xAxis, origin),
	            yAxis.x,  yAxis.y,  yAxis.z,  -dot(yAxis, origin),
	            zAxis.x,  zAxis.y,  zAxis.z,  -dot(zAxis, origin),
	            0.0f,     0.0f,     0.0f,     1.0f);
}

// Projection matrices (Standard OpenGL [-1, 1] right-handed clip space, GL view space)
// ------------------------------------------------------------------------------------------------

mat4 orthogonalProjectionGL(float l, float b, float r, float t, float n, float f) noexcept
{
	return mat4(
		2.0f / (r - l),  0.0f,            0.0f,             -((r + l) / (r - l)),
		0.0f,            2.0f / (t - b),  0.0f,             -((t + b) / (t - b)),
		0.0f,            0.0f,            -2.0f / (f - n),  -(f + n) / (f - n),
		0.0f,            0.0f,            0.0f,             1.0f
	);
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
	return mat3(
		a,     0.0f,  -(center.x * a),
		0.0f,  b,     -(center.y * b),
		0.0f,  0.0f,  1.0f
	);
}

mat4 perspectiveProjectionGL(float l, float b, float r, float t, float n, float f) noexcept
{
	return mat4(
		2.0f * n / (r - l),  0.0f,                (r + l) / (r - l),   0.0f,
		0.0f,                2.0f * n / (t - b),  (t + b) / (t - b),   0.0f,
		0.0f,                0.0f,                -(f + n) / (f - n),  -2.0f * f * n / (f - n),
		0.0f,                0.0f,                -1.0f,               0.0f
	);
}

mat4 perspectiveProjectionGL(float yFovDeg, float aspectRatio, float zNear, float zFar) noexcept
{
	float yMax = zNear * std::tan(yFovDeg * (PI / 360.0f));
	float xMax = yMax * aspectRatio;
	return perspectiveProjectionGL(-xMax, -yMax, xMax, yMax, zNear, zFar);
}

} // namespace sfz
