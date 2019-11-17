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

#include "sfz/gl/UniformSetters.hpp"

#include "sfz/gl/IncludeOpenGL.hpp"

namespace sfz {

namespace gl {

// Uniform setters: int
// ------------------------------------------------------------------------------------------------

void setUniform(int location, int i) noexcept
{
	glUniform1i(location, i);
}

void setUniform(const Program& program, const char* name, int i) noexcept
{
	int loc = glGetUniformLocation(program.handle(), name);
	setUniform(loc, i);
}

void setUniform(int location, const int* intArray, size_t count) noexcept
{
	glUniform1iv(location, (GLsizei)count, intArray);
}

void setUniform(const Program& program, const char* name, const int* intArray, size_t count) noexcept
{
	int loc = glGetUniformLocation(program.handle(), name);
	setUniform(loc, intArray, count);
}

// Uniform setters: uint
// ------------------------------------------------------------------------------------------------

#if !defined(__EMSCRIPTEN__) && !defined(SFZ_IOS)

void setUniform(int location, uint32_t u) noexcept
{
	glUniform1ui(location, u);
}

void setUniform(const Program& program, const char* name, uint32_t u) noexcept
{
	int loc = glGetUniformLocation(program.handle(), name);
	setUniform(loc, u);
}

void setUniform(int location, const uint32_t* uintArray, size_t count) noexcept
{
	glUniform1uiv(location, (GLsizei)count, uintArray);
}

void setUniform(const Program& program, const char* name, const uint32_t* uintArray, size_t count) noexcept
{
	int loc = glGetUniformLocation(program.handle(), name);
	setUniform(loc, uintArray, count);
}

#endif

// Uniform setters: float
// ------------------------------------------------------------------------------------------------

void setUniform(int location, float f) noexcept
{
	glUniform1f(location, f);
}

void setUniform(const Program& program, const char* name, float f) noexcept
{
	int loc = glGetUniformLocation(program.handle(), name);
	setUniform(loc, f);
}

void setUniform(int location, const float* floatArray, size_t count) noexcept
{
	glUniform1fv(location, (GLsizei)count, floatArray);
}

void setUniform(const Program& program, const char* name, const float* floatArray, size_t count) noexcept
{
	int loc = glGetUniformLocation(program.handle(), name);
	setUniform(loc, floatArray, count);
}

// Uniform setters: vec2
// ------------------------------------------------------------------------------------------------

void setUniform(int location, vec2 vector) noexcept
{
	glUniform2fv(location, 1, vector.data());
}

void setUniform(const Program& program, const char* name, vec2 vector) noexcept
{
	int loc = glGetUniformLocation(program.handle(), name);
	setUniform(loc, vector);
}

void setUniform(int location, const vec2* vectorArray, size_t count) noexcept
{
	static_assert(sizeof(vec2) == sizeof(float)*2, "vec2 is padded");
	glUniform2fv(location, (GLsizei)count, vectorArray[0].data());
}

void setUniform(const Program& program, const char* name, const vec2* vectorArray, size_t count) noexcept
{
	int loc = glGetUniformLocation(program.handle(), name);
	setUniform(loc, vectorArray, count);
}

// Uniform setters: vec3
// ------------------------------------------------------------------------------------------------

void setUniform(int location, const vec3& vector) noexcept
{
	glUniform3fv(location, 1, vector.data());
}

void setUniform(const Program& program, const char* name, const vec3& vector) noexcept
{
	int loc = glGetUniformLocation(program.handle(), name);
	setUniform(loc, vector);
}

void setUniform(int location, const vec3* vectorArray, size_t count) noexcept
{
	static_assert(sizeof(vec3) == sizeof(float)*3, "vec3 is padded");
	glUniform3fv(location, (GLsizei)count, vectorArray[0].data());
}

void setUniform(const Program& program, const char* name, const vec3* vectorArray, size_t count) noexcept
{
	int loc = glGetUniformLocation(program.handle(), name);
	setUniform(loc, vectorArray, count);
}

// Uniform setters: vec4
// ------------------------------------------------------------------------------------------------

void setUniform(int location, const vec4& vector) noexcept
{
	glUniform4fv(location, 1, vector.data());
}

void setUniform(const Program& program, const char* name, const vec4& vector) noexcept
{
	int loc = glGetUniformLocation(program.handle(), name);
	setUniform(loc, vector);
}

void setUniform(int location, const vec4* vectorArray, size_t count) noexcept
{
	static_assert(sizeof(vec4) == sizeof(float)*4, "vec4 is padded");
	glUniform4fv(location, (GLsizei)count, vectorArray[0].data());
}

void setUniform(const Program& program, const char* name, const vec4* vectorArray, size_t count) noexcept
{
	int loc = glGetUniformLocation(program.handle(), name);
	setUniform(loc, vectorArray, count);
}

// Uniform setters: mat3
// ------------------------------------------------------------------------------------------------

void setUniform(int location, const mat33& matrix) noexcept
{
	glUniformMatrix3fv(location, 1, true, matrix.data());
}

void setUniform(const Program& program, const char* name, const mat33& matrix) noexcept
{
	int loc = glGetUniformLocation(program.handle(), name);
	setUniform(loc, matrix);
}

void setUniform(int location, const mat33* matrixArray, size_t count) noexcept
{
	static_assert(sizeof(mat33) == sizeof(float)*9, "mat33 is padded");
	glUniformMatrix3fv(location, (GLsizei)count, true, matrixArray[0].data());
}

void setUniform(const Program& program, const char* name, const mat33* matrixArray, size_t count) noexcept
{
	int loc = glGetUniformLocation(program.handle(), name);
	setUniform(loc, matrixArray, count);
}

// Uniform setters: mat4
// ------------------------------------------------------------------------------------------------

void setUniform(int location, const mat44& matrix) noexcept
{
#if defined(__EMSCRIPTEN__) || defined(SFZ_IOS)
	mat44 tmp = transpose(matrix);
	glUniformMatrix4fv(location, 1, false, tmp.data());
#else
	glUniformMatrix4fv(location, 1, true, matrix.data());
#endif
}

void setUniform(const Program& program, const char* name, const mat44& matrix) noexcept
{
	int loc = glGetUniformLocation(program.handle(), name);
	setUniform(loc, matrix);
}

#if !defined(__EMSCRIPTEN__) && !defined(SFZ_IOS)
void setUniform(int location, const mat44* matrixArray, size_t count) noexcept
{
	static_assert(sizeof(mat44) == sizeof(float)*16, "mat44 is padded");
	glUniformMatrix4fv(location, (GLsizei)count, true, matrixArray[0].data());
}

void setUniform(const Program& program, const char* name, const mat44* matrixArray, size_t count) noexcept
{
	int loc = glGetUniformLocation(program.handle(), name);
	setUniform(loc, matrixArray, count);
}
#endif

} // namespace gl
} // namespace sfz
