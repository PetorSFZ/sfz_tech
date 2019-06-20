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

#include <SDL.h>
#include <ZeroG.h>
#include <ZeroG-cpp.hpp>

// Error handling helpers
// ------------------------------------------------------------------------------------------------

// Checks result (ZgErrorCode) from ZeroG call and log if not success, returns result unmodified
#define CHECK_ZG (CheckZgImpl(__FILE__, __LINE__)) %

// Implementation of CHECK_ZG
struct CheckZgImpl final {
	const char* file;
	int line;

	CheckZgImpl() = delete;
	CheckZgImpl(const char* file, int line) noexcept : file(file), line(line) {}

	ZgErrorCode operator% (ZgErrorCode result) noexcept;
	void operator% (zg::ErrorCode result) noexcept;
};

// Initialization functions
// ------------------------------------------------------------------------------------------------

// Helper methods for initializing/deinitializing SDL2 and creating a window. The samples do not
// aim to teach SDL2, and the user of ZeroG might not be using SDL2 in the first place.
SDL_Window* initializeSdl2CreateWindow(const char* sampleName) noexcept;
void cleanupSdl2(SDL_Window* window) noexcept;

// A function that given an SDL2 window returns the platform specific native window handle, in
// the form of a void pointer which can be passed to ZeroG.
void* getNativeWindowHandle(SDL_Window* window) noexcept;

// Math
// ------------------------------------------------------------------------------------------------

constexpr float PI = 3.14159265358979323846f;
constexpr float DEG_TO_RAD = PI / 180.0f;

// A vector of size 4
struct Vector {
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
	float w = 0.0f;

	Vector() noexcept = default;
	Vector(float x, float y, float z) : x(x), y(y), z(z), w(0.0f) {}
};

// Negation operator
Vector operator- (Vector v) noexcept;

// Normalize a vector
Vector normalize(Vector v) noexcept;

// Dot product of two vectors
float dot(Vector lhs, Vector rhs) noexcept;

// Cross product of two vectors, sets 4th component to 0
Vector cross(Vector lhs, Vector rhs) noexcept;

// A 4x4 matrix class.
//
// Coordinate-system: Right-handed
// Order: Column-major (matrix is multiplied "from the left" to column-major vectors. "M * v")
// Memory-order: Row-major (first row of matrices is m[0], m[1], m[2], m[3])
struct Matrix {
	float m[16] = {};

	Vector rowAt(uint32_t i) const noexcept;
	Vector columnAt(uint32_t i) const noexcept;
};
static_assert(sizeof(Matrix) == sizeof(float) * 16, "Matrix is padded");

// Matrix multiplication operator
Matrix operator*(const Matrix& lhs, const Matrix& rhs) noexcept;

// Matrix multiplication operator (scalar)
Matrix operator*(const Matrix& lhs, float rhs) noexcept;
Matrix operator*(float lhs, const Matrix& rhs) noexcept;

// Transposes a matrix
Matrix transpose(const Matrix& m) noexcept;

// Calculates the determinant of a matrix
float determinant(const Matrix& m) noexcept;

// Inverts a matrix
Matrix inverse(const Matrix& m) noexcept;

// Creates an identity matrix
Matrix createIdentityMatrix() noexcept;

// Creates a view matrix
//
// Right-handed, negative-z into the screen, positive-x to the right
Matrix createViewMatrix(Vector origin, Vector dir, Vector up) noexcept;

// Creates a projection matrix
//
// Right-handed view space, left-handed clip space (with origin in upper left corner), depth 0 to 1
// where 0 is closest.
Matrix createProjectionMatrix(float yFovDeg, float aspectRatio, float zNear, float zFar) noexcept;
