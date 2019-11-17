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

#include "sfz/gl/Program.hpp"
#include "sfz/math/Matrix.hpp"
#include "sfz/math/Vector.hpp"

namespace sfz {

namespace gl {

// Uniform setters
// ------------------------------------------------------------------------------------------------

void setUniform(int location, int i) noexcept;
void setUniform(const Program& program, const char* name, int i) noexcept;
void setUniform(int location, const int* intArray, size_t count) noexcept;
void setUniform(const Program& program, const char* name, const int* intArray, size_t count) noexcept;

#if !defined(__EMSCRIPTEN__) && !defined(SFZ_IOS)
void setUniform(int location, uint32_t u) noexcept;
void setUniform(const Program& program, const char* name, uint32_t u) noexcept;
void setUniform(int location, const uint32_t* uintArray, size_t count) noexcept;
void setUniform(const Program& program, const char* name, const uint32_t* uintArray, size_t count) noexcept;
#endif

void setUniform(int location, float f) noexcept;
void setUniform(const Program& program, const char* name, float f) noexcept;
void setUniform(int location, const float* floatArray, size_t count) noexcept;
void setUniform(const Program& program, const char* name, const float* floatArray, size_t count) noexcept;

void setUniform(int location, vec2 vector) noexcept;
void setUniform(const Program& program, const char* name, vec2 vector) noexcept;
void setUniform(int location, const vec2* vectorArray, size_t count) noexcept;
void setUniform(const Program& program, const char* name, const vec2* vectorArray, size_t count) noexcept;

void setUniform(int location, const vec3& vector) noexcept;
void setUniform(const Program& program, const char* name, const vec3& vector) noexcept;
void setUniform(int location, const vec3* vectorArray, size_t count) noexcept;
void setUniform(const Program& program, const char* name, const vec3* vectorArray, size_t count) noexcept;

void setUniform(int location, const vec4& vector) noexcept;
void setUniform(const Program& program, const char* name, const vec4& vector) noexcept;
void setUniform(int location, const vec4* vectorArray, size_t count) noexcept;
void setUniform(const Program& program, const char* name, const vec4* vectorArray, size_t count) noexcept;

void setUniform(int location, const mat33& matrix) noexcept;
void setUniform(const Program& program, const char* name, const mat33& matrix) noexcept;
void setUniform(int location, const mat33* matrixArray, size_t count) noexcept;
void setUniform(const Program& program, const char* name, const mat33* matrixArray, size_t count) noexcept;

void setUniform(int location, const mat44& matrix) noexcept;
void setUniform(const Program& program, const char* name, const mat44& matrix) noexcept;
#if !defined(__EMSCRIPTEN__) && !defined(SFZ_IOS)
void setUniform(int location, const mat44* matrixArray, size_t count) noexcept;
void setUniform(const Program& program, const char* name, const mat44* matrixArray, size_t count) noexcept;
#endif


} // namespace gl
} // namespace sfz
