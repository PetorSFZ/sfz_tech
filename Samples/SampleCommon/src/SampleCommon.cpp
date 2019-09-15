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

#include "SampleCommon.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <exception>

#include <SDL_syswm.h>

// Statics
// ------------------------------------------------------------------------------------------------

#ifdef _WIN32
static HWND getWin32WindowHandle(SDL_Window* window) noexcept
{
	SDL_SysWMinfo info = {};
	SDL_VERSION(&info.version);
	if (!SDL_GetWindowWMInfo(window, &info)) return nullptr;
	return info.info.win.window;
}
#endif

// TODO ifdef APPLE
static void* getMacOSWindowHandle(SDL_Window* window) noexcept
{
	(void)window;
	return nullptr;
}

static const char* stripFilePath(const char* file) noexcept
{
	const char* strippedFile1 = std::strrchr(file, '\\');
	const char* strippedFile2 = std::strrchr(file, '/');
	if (strippedFile1 == nullptr && strippedFile2 == nullptr) {
		return file;
	}
	else if (strippedFile2 == nullptr) {
		return strippedFile1 + 1;
	}
	else {
		return strippedFile2 + 1;
	}
}

// Error handling helpers
// ------------------------------------------------------------------------------------------------

void CheckZgImpl::operator% (zg::ErrorCode result) noexcept
{
	if (zg::isSuccess(result)) return;
	if (zg::isWarning(result)) {
		printf("%s:%i: ZeroG Warning: %s\n",
			stripFilePath(file), line, zgErrorCodeToString((ZgErrorCode)result));
	}
	else {
		printf("%s:%i: ZeroG Error: %s\n",
			stripFilePath(file), line, zgErrorCodeToString((ZgErrorCode)result));
		assert(false);
	}
}

// Initialization functions
// ------------------------------------------------------------------------------------------------

SDL_Window* initializeSdl2CreateWindow(const char* sampleName) noexcept
{
	// Init SDL2
	if (SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO) < 0) {
		printf("SDL_Init() failed: %s", SDL_GetError());
		fflush(stdout);
		std::terminate();
	}

	// Window
	SDL_Window* window = SDL_CreateWindow(
		sampleName,
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		800, 800,
		SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);
	if (window == NULL) {
		printf("SDL_CreateWindow() failed: %s\n", SDL_GetError());
		fflush(stdout);
		SDL_Quit();
		std::terminate();
	}

	return window;
}

void cleanupSdl2(SDL_Window* window) noexcept
{
	SDL_DestroyWindow(window);
	SDL_Quit();
}

void* getNativeWindowHandle(SDL_Window* window) noexcept
{
#ifdef WIN32
	return getWin32WindowHandle(window);
#else
	return getMacOSWindowHandle(window);
//#else
//#error "Not implemented yet"
#endif
}

// Math
// ------------------------------------------------------------------------------------------------

Vector operator- (Vector v) noexcept
{
	v.x = -v.x;
	v.y = -v.y;
	v.z = -v.z;
	v.w = -v.w;
	return v;
}

Vector normalize(Vector v) noexcept
{
	float length = std::sqrt(dot(v, v));
	v.x /= length;
	v.y /= length;
	v.z /= length;
	v.w /= length;
	return v;
}

float dot(Vector lhs, Vector rhs) noexcept
{
	return
		lhs.x * rhs.x +
		lhs.y * rhs.y +
		lhs.z * rhs.z +
		lhs.w * rhs.w;
}

Vector cross(Vector lhs, Vector rhs) noexcept
{
	Vector tmp;
	tmp.x = lhs.y * rhs.z - lhs.z * rhs.y;
	tmp.y = lhs.z * rhs.x - lhs.x * rhs.z;
	tmp.z = lhs.x * rhs.y - lhs.y * rhs.x;
	tmp.w = 0.0f;
	return tmp;
}

Vector Matrix::rowAt(uint32_t i) const noexcept
{
	Vector tmp;
	tmp.x = m[i * 4 + 0];
	tmp.y = m[i * 4 + 1];
	tmp.z = m[i * 4 + 2];
	tmp.w = m[i * 4 + 3];
	return tmp;
}

Vector Matrix::columnAt(uint32_t i) const noexcept
{
	Vector tmp;
	tmp.x = m[0 * 4 + i];
	tmp.y = m[1 * 4 + i];
	tmp.z = m[2 * 4 + i];
	tmp.w = m[3 * 4 + i];
	return tmp;
}

Matrix operator*(const Matrix& lhs, const Matrix& rhs) noexcept
{

	Matrix tmp;
	for (uint32_t y = 0; y < 4; y++) {
		for (uint32_t x = 0; x < 4; x++) {
			tmp.m[y * 4 + x] = dot(lhs.rowAt(y), rhs.columnAt(x));
		}
	}
	return tmp;
}

Matrix operator*(const Matrix& lhs, float rhs) noexcept
{
	Matrix tmp;
	for (uint32_t i = 0; i < 16; i++) {
		tmp.m[i] = lhs.m[i] * rhs;
	}
	return tmp;
}

Matrix operator*(float lhs, const Matrix& rhs) noexcept
{
	return rhs * lhs;
}

Matrix transpose(const Matrix& m) noexcept
{
	Matrix tmp;
	for (uint32_t y = 0; y < 4; y++) {
		for (uint32_t x = 0; x < 4; x++) {
			tmp.m[y * 4 + x] = m.m[x * 4 + y];
		}
	}
	return tmp;
}

float determinant(const Matrix& mat) noexcept
{
	auto m = [&](uint32_t y, uint32_t x) {
		return mat.m[y * 4 + x];
	};
	return
		m(0,0)*m(1,1)*m(2,2)*m(3,3) + m(0,0)*m(1,2)*m(2,3)*m(3,1) + m(0,0)*m(1,3)*m(2,1)*m(3,2) +
		m(0,1)*m(1,0)*m(2,3)*m(3,2) + m(0,1)*m(1,2)*m(2,0)*m(3,3) + m(0,1)*m(1,3)*m(2,2)*m(3,0) +
		m(0,2)*m(1,0)*m(2,1)*m(3,3) + m(0,2)*m(1,1)*m(2,3)*m(3,0) + m(0,2)*m(1,3)*m(2,0)*m(3,1) +
		m(0,3)*m(1,0)*m(2,2)*m(3,1) + m(0,3)*m(1,1)*m(2,0)*m(3,2) + m(0,3)*m(1,2)*m(2,1)*m(3,0) -
		m(0,0)*m(1,1)*m(2,3)*m(3,2) - m(0,0)*m(1,2)*m(2,1)*m(3,3) - m(0,0)*m(1,3)*m(2,2)*m(3,1) -
		m(0,1)*m(1,0)*m(2,2)*m(3,3) - m(0,1)*m(1,2)*m(2,3)*m(3,0) - m(0,1)*m(1,3)*m(2,0)*m(3,2) -
		m(0,2)*m(1,0)*m(2,3)*m(3,1) - m(0,2)*m(1,1)*m(2,0)*m(3,3) - m(0,2)*m(1,3)*m(2,1)*m(3,0) -
		m(0,3)*m(1,0)*m(2,1)*m(3,2) - m(0,3)*m(1,1)*m(2,2)*m(3,0) - m(0,3)*m(1,2)*m(2,0)*m(3,1);
}

Matrix inverse(const Matrix& mat) noexcept
{
	const float det = determinant(mat);
	if (det == 0) return Matrix();

	auto m = [&](uint32_t y, uint32_t x) {
		return mat.m[y * 4 + x];
	};

	const float
		m00 = m(0,0), m01 = m(0,1), m02 = m(0,2), m03 = m(0,3),
		m10 = m(1,0), m11 = m(1,1), m12 = m(1,2), m13 = m(1,3),
		m20 = m(2,0), m21 = m(2,1), m22 = m(2,2), m23 = m(2,3),
		m30 = m(3,0), m31 = m(3,1), m32 = m(3,2), m33 = m(3,3);

	float b00 = m11*m22*m33 + m12*m23*m31 + m13*m21*m32 - m11*m23*m32 - m12*m21*m33 - m13*m22*m31;
	float b01 = m01*m23*m32 + m02*m21*m33 + m03*m22*m31 - m01*m22*m33 - m02*m23*m31 - m03*m21*m32;
	float b02 = m01*m12*m33 + m02*m13*m31 + m03*m11*m32 - m01*m13*m32 - m02*m11*m33 - m03*m12*m31;
	float b03 = m01*m13*m22 + m02*m11*m23 + m03*m12*m21 - m01*m12*m23 - m02*m13*m21 - m03*m11*m22;
	float b10 = m10*m23*m32 + m12*m20*m33 + m13*m22*m30 - m10*m22*m33 - m12*m23*m30 - m13*m20*m32;
	float b11 = m00*m22*m33 + m02*m23*m30 + m03*m20*m32 - m00*m23*m32 - m02*m20*m33 - m03*m22*m30;
	float b12 = m00*m13*m32 + m02*m10*m33 + m03*m12*m30 - m00*m12*m33 - m02*m13*m30 - m03*m10*m32;
	float b13 = m00*m12*m23 + m02*m13*m20 + m03*m10*m22 - m00*m13*m22 - m02*m10*m23 - m03*m12*m20;
	float b20 = m10*m21*m33 + m11*m23*m30 + m13*m20*m31 - m01*m23*m31 - m11*m20*m33 - m13*m21*m30;
	float b21 = m00*m23*m31 + m01*m20*m33 + m03*m21*m30 - m00*m21*m33 - m01*m23*m30 - m03*m20*m31;
	float b22 = m00*m11*m33 + m01*m13*m30 + m03*m10*m31 - m00*m13*m31 - m01*m10*m33 - m03*m11*m30;
	float b23 = m00*m13*m21 + m01*m10*m23 + m03*m11*m20 - m00*m11*m23 - m01*m13*m20 - m03*m10*m21;
	float b30 = m10*m22*m31 + m11*m20*m32 + m12*m21*m30 - m10*m21*m32 - m11*m22*m30 - m12*m20*m31;
	float b31 = m00*m21*m32 + m01*m22*m30 + m02*m20*m31 - m00*m22*m31 - m01*m20*m32 - m02*m21*m30;
	float b32 = m00*m12*m31 + m01*m10*m32 + m02*m11*m30 - m00*m11*m32 - m01*m12*m30 - m02*m10*m31;
	float b33 = m00*m11*m22 + m01*m12*m20 + m02*m10*m21 - m00*m12*m21 - m01*m10*m22 - m02*m11*m20;

	Matrix tmp;

	tmp.m[0] = b00;
	tmp.m[1] = b01;
	tmp.m[2] = b02;
	tmp.m[3] = b03;

	tmp.m[4] = b10;
	tmp.m[5] = b11;
	tmp.m[6] = b12;
	tmp.m[7] = b13;

	tmp.m[8] = b20;
	tmp.m[9] = b21;
	tmp.m[10] = b22;
	tmp.m[11] = b23;

	tmp.m[12] = b30;
	tmp.m[13] = b31;
	tmp.m[14] = b32;
	tmp.m[15] = b33;

	return (1.0f / det) * tmp;
}

Matrix createIdentityMatrix() noexcept
{
	Matrix tmp;
	tmp.m[4 * 0 + 0] = 1.0f;
	tmp.m[4 * 1 + 1] = 1.0f;
	tmp.m[4 * 2 + 2] = 1.0f;
	tmp.m[4 * 3 + 3] = 1.0f;
	return tmp;
}
