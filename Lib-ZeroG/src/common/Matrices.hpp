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

#include "ZeroG.h"

// Transformation and projection matrices
// ------------------------------------------------------------------------------------------------

ZG_API void zgUtilCreateViewMatrix(
	SfzMat44* matrixOut,
	const f32 origin[3],
	const f32 dir[3],
	const f32 up[3])
{
	auto dot = [](const f32 lhs[3], const f32 rhs[3]) -> f32 {
		return lhs[0] * rhs[0] + lhs[1] * rhs[1] + lhs[2] * rhs[2];
	};

	auto normalize = [&](f32 v[3]) {
		f32 length = sfz_sqrt(dot(v, v));
		v[0] /= length;
		v[1] /= length;
		v[2] /= length;
	};

	auto cross = [](f32 out[3], const f32 lhs[3], const f32 rhs[3]) {
		out[0] = lhs[1] * rhs[2] - lhs[2] * rhs[1];
		out[1] = lhs[2] * rhs[0] - lhs[0] * rhs[2];
		out[2] = lhs[0] * rhs[1] - lhs[1] * rhs[0];
	};

	// Z-Axis, away from screen
	f32 zAxis[3];
	memcpy(zAxis, dir, sizeof(f32) * 3);
	normalize(zAxis);
	zAxis[0] = -zAxis[0];
	zAxis[1] = -zAxis[1];
	zAxis[2] = -zAxis[2];

	// X-Axis, to the right
	f32 xAxis[3];
	cross(xAxis, up, zAxis);
	normalize(xAxis);

	// Y-Axis, up
	f32 yAxis[3];
	cross(yAxis, zAxis, xAxis);

	f32 matrix[16] = {
		xAxis[0], xAxis[1], xAxis[2], -dot(xAxis, origin),
		yAxis[0], yAxis[1], yAxis[2], -dot(yAxis, origin),
		zAxis[0], zAxis[1], zAxis[2], -dot(zAxis, origin),
		0.0f,     0.0f,     0.0f,     1.0f
	};
	memcpy(matrixOut, matrix, sizeof(f32) * 16);
}

ZG_API void zgUtilCreatePerspectiveProjection(
	SfzMat44* matrixOut,
	f32 vertFovDegs,
	f32 aspect,
	f32 nearPlane,
	f32 farPlane)
{
	sfz_assert(0.0f < vertFovDegs);
	sfz_assert(vertFovDegs < 180.0f);
	sfz_assert(0.0f < aspect);
	sfz_assert(0.0f < nearPlane);
	sfz_assert(nearPlane < farPlane);

	// From: https://docs.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixperspectivefovrh
	// xScale     0          0              0
	// 0        yScale       0              0
	// 0        0        zf/(zn-zf)        -1
	// 0        0        zn*zf/(zn-zf)      0
	// where:
	// yScale = cot(fovY/2)
	// xScale = yScale / aspect ratio
	//
	// Note that D3D uses column major matrices, we use row-major, so above is transposed.

	constexpr f32 DEG_TO_RAD = 3.14159265358979323846f / 180.0f;
	const f32 vertFovRads = vertFovDegs * DEG_TO_RAD;
	const f32 yScale = 1.0f / sfz_tan(vertFovRads * 0.5f);
	const f32 xScale = yScale / aspect;
	f32 matrix[16] = {
		xScale, 0.0f, 0.0f, 0.0f,
		0.0f, yScale, 0.0f, 0.0f,
		0.0f, 0.0f, farPlane / (nearPlane - farPlane), nearPlane* farPlane / (nearPlane - farPlane),
		0.0f, 0.0f, -1.0f, 0.0f
	};
	memcpy(matrixOut, matrix, sizeof(f32) * 16);
}

ZG_API void zgUtilCreatePerspectiveProjectionInfinite(
	SfzMat44* matrixOut,
	f32 vertFovDegs,
	f32 aspect,
	f32 nearPlane)
{
	sfz_assert(0.0f < vertFovDegs);
	sfz_assert(vertFovDegs < 180.0f);
	sfz_assert(0.0f < aspect);
	sfz_assert(0.0f < nearPlane);

	// Same as createPerspectiveProjection(), but let far approach infinity

	constexpr f32 DEG_TO_RAD = 3.14159265358979323846f / 180.0f;
	const f32 vertFovRads = vertFovDegs * DEG_TO_RAD;
	const f32 yScale = 1.0f / sfz_tan(vertFovRads * 0.5f);
	const f32 xScale = yScale / aspect;
	f32 matrix[16] = {
		xScale, 0.0f, 0.0f, 0.0f,
		0.0f, yScale, 0.0f, 0.0f,
		0.0f, 0.0f, -1.0f,-nearPlane,
		0.0f, 0.0f, -1.0f, 0.0f
	};
	memcpy(matrixOut, matrix, sizeof(f32) * 16);
}

ZG_API void zgUtilCreatePerspectiveProjectionReverse(
	SfzMat44* matrixOut,
	f32 vertFovDegs,
	f32 aspect,
	f32 nearPlane,
	f32 farPlane)
{
	sfz_assert(0.0f < vertFovDegs);
	sfz_assert(vertFovDegs < 180.0f);
	sfz_assert(0.0f < aspect);
	sfz_assert(0.0f < nearPlane);
	sfz_assert(nearPlane < farPlane);

	// http://dev.theomader.com/depth-precision/
	// "This can be achieved by multiplying the projection matrix with a simple ‘z reversal’ matrix"
	// 1, 0, 0, 0
	// 0, 1, 0, 0
	// 0, 0, -1, 1
	// 0, 0, 0, 1

	constexpr f32 DEG_TO_RAD = 3.14159265358979323846f / 180.0f;
	const f32 vertFovRads = vertFovDegs * DEG_TO_RAD;
	const f32 yScale = 1.0f / sfz_tan(vertFovRads * 0.5f);
	const f32 xScale = yScale / aspect;
	f32 matrix[16] = {
		xScale, 0.0f, 0.0f, 0.0f,
		0.0f, yScale, 0.0f, 0.0f,
		0.0f, 0.0f, -(farPlane / (nearPlane - farPlane)) - 1.0f, -(nearPlane * farPlane / (nearPlane - farPlane)),
		0.0f, 0.0f, -1.0f, 0.0f
	};
	memcpy(matrixOut, matrix, sizeof(f32) * 16);
}

ZG_API void zgUtilCreatePerspectiveProjectionReverseInfinite(
	SfzMat44* matrixOut,
	f32 vertFovDegs,
	f32 aspect,
	f32 nearPlane)
{
	sfz_assert(0.0f < vertFovDegs);
	sfz_assert(vertFovDegs < 180.0f);
	sfz_assert(0.0f < aspect);
	sfz_assert(0.0f < nearPlane);

	// http://dev.theomader.com/depth-precision/
	// "This can be achieved by multiplying the projection matrix with a simple ‘z reversal’ matrix"
	// 1, 0, 0, 0
	// 0, 1, 0, 0
	// 0, 0, -1, 1
	// 0, 0, 0, 1

	constexpr f32 DEG_TO_RAD = 3.14159265358979323846f / 180.0f;
	const f32 vertFovRads = vertFovDegs * DEG_TO_RAD;
	const f32 yScale = 1.0f / sfz_tan(vertFovRads * 0.5f);
	const f32 xScale = yScale / aspect;
	f32 matrix[16] = {
		xScale, 0.0f, 0.0f, 0.0f,
		0.0f, yScale, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, nearPlane,
		0.0f, 0.0f, -1.0f, 0.0f
	};
	memcpy(matrixOut, matrix, sizeof(f32) * 16);
}

ZG_API void zgUtilCreateOrthographicProjection(
	SfzMat44* matrixOut,
	f32 width,
	f32 height,
	f32 nearPlane,
	f32 farPlane)
{
	sfz_assert(0.0f < width);
	sfz_assert(0.0f < height);
	sfz_assert(0.0f < nearPlane);
	sfz_assert(nearPlane < farPlane);

	// https://docs.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixorthorh
	// 2/w  0    0           0
	// 0    2/h  0           0
	// 0    0    1/(zn-zf)   0
	// 0    0    zn/(zn-zf)  1
	//
	// Note that D3D uses column major matrices, we use row-major, so above is transposed.

	f32 matrix[16] = {
		2.0f / width, 0.0f, 0.0f, 0.0f,
		0.0f, 2.0f / height, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f / (nearPlane - farPlane), nearPlane / (nearPlane - farPlane),
		0.0f, 0.0f, 0.0f, 1.0f
	};
	memcpy(matrixOut, matrix, sizeof(f32) * 16);
}

ZG_API void zgUtilCreateOrthographicProjectionReverse(
	SfzMat44* matrixOut,
	f32 width,
	f32 height,
	f32 nearPlane,
	f32 farPlane)
{
	sfz_assert(0.0f < width);
	sfz_assert(0.0f < height);
	sfz_assert(0.0f < nearPlane);
	sfz_assert(nearPlane < farPlane);

	// http://dev.theomader.com/depth-precision/
	// "This can be achieved by multiplying the projection matrix with a simple ‘z reversal’ matrix"
	// 1, 0, 0, 0
	// 0, 1, 0, 0
	// 0, 0, -1, 1
	// 0, 0, 0, 1

	f32 matrix[16] = {
		2.0f / width, 0.0f, 0.0f, 0.0f,
		0.0f, 2.0f / height, 0.0f, 0.0f,
		0.0f, 0.0f, -1.0f / (nearPlane - farPlane), 1.0f - (nearPlane / (nearPlane - farPlane)),
		0.0f, 0.0f, 0.0f, 1.0f
	};
	memcpy(matrixOut, matrix, sizeof(f32) * 16);
}

