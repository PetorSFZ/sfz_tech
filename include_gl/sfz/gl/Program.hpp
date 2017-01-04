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

#include <cstdint>

#include "sfz/math/Matrix.hpp"
#include "sfz/math/Vector.hpp"
#include "sfz/strings/DynString.hpp"

namespace sfz {

namespace gl {

using std::uint32_t;

// Program class
// ------------------------------------------------------------------------------------------------

/// A class holding an OpenGL Program.
///
/// The post process variants will create a Program using the default post process vertex shader,
/// accessable by calling "postProcessVertexShaderSource()". The vertex shader outputs a couple
/// of things to the fragment shader:
/// vec2 uvCoord: coordinate used to sample from fullscreen buffers
/// vec3 nonNormRayDir: non-normalized ray direction for each pixel
/// In order to calculate nonNormRayDir the "uniform mat4 uInvProjMatrix" needs to be set with
/// the inverse projection matrix. Can safely be ignored if nonNormRayDir won't be used.
class Program final {
public:
	// Constructor functions
	// --------------------------------------------------------------------------------------------

	/// Constructs an OpenGL program given source strings. The reload() method will not have any
	/// effect on a program created directly from source by these functions.
	/// \param *Src the source string for a specific shader
	/// \param bindAttribFragFunc an optional function pointer used to call glBindAttribLocation() & 
	///                           glBindFragDataLocation()

	static Program fromSource(const char* vertexSrc, const char* fragmentSrc,
	                          void(*bindAttribFragFunc)(uint32_t shaderProgram) = nullptr) noexcept;
	static Program fromSource(const char* vertexSrc, const char* geometrySrc, const char* fragmentSrc,
	                          void(*bindAttribFragFunc)(uint32_t shaderProgram) = nullptr) noexcept;
	static Program postProcessFromSource(const char* postProcessSource) noexcept;

	/// Constructs an OpenGL program given file paths to source
	/// The file paths are stored and when reload() is called the program will be recompiled.
	/// \param basePath the path to the directory the source files are located in
	/// \param *File the filename of the file containing the shader source for a specific shader
	/// \param bindAttribFragFunc an optional function pointer used to call glBindAttribLocation() & 
	///                           glBindFragDataLocation()

	static Program fromFile(const char* basePath, const char* vertexFile, const char* fragmentFile,
	                        void(*bindAttribFragFunc)(uint32_t shaderProgram) = nullptr) noexcept;
	static Program fromFile(const char* basePath, const char* vertexFile,
	                        const char* geometryFile, const char* fragmentFile,
	                        void(*bindAttribFragFunc)(uint32_t shaderProgram) = nullptr) noexcept;
	static Program postProcessFromFile(const char* basePath, const char* postProcessFile) noexcept;

	// Public methods
	// --------------------------------------------------------------------------------------------

	inline uint32_t handle() const noexcept { return mHandle; }
	inline bool isValid() const noexcept { return (mHandle != 0); }
	inline bool wasReloaded() const noexcept { return mWasReloaded; }
	inline void clearWasReloadedFlag() noexcept { mWasReloaded = false; }

	/// Attempts to load source from file and recompile the program
	/// This operation loads shader source from files and attempts to compile and link them into
	/// a new program. The operation can therefore only succeed if the program was created from file
	/// to begin with. If any step of the process fails it will be aborted and the current program
	/// will remain unaffected.
	/// \return whether the reload was successful or not
	bool reload() noexcept;

	/// Simple wrapper that calls glUseProgram() with the internal handle, will not do anything
	/// if internal handle is 0.
	void useProgram() noexcept;

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	// Copying not allowed
	Program(const Program&) = delete;
	Program& operator= (const Program&) = delete;

	Program() noexcept= default;
	Program(Program&& other) noexcept;
	Program& operator= (Program&& other) noexcept;
	~Program() noexcept;

private:
	// Private members
	// --------------------------------------------------------------------------------------------

	// The handle to the current OpenGL program
	uint32_t mHandle = 0;

	// Optional paths to shader source files
	DynString mVertexPath;
	DynString mGeometryPath;
	DynString mFragmentPath;

	// Bool that specifies if program is post process or not
	bool mIsPostProcess = false;

	// Bool that specifies if program was recently reloaded or not, needs to be manually cleared.
	bool mWasReloaded = false;

	// Optional function used to call glBindAttribLocation() & glBindFragDataLocation()
	void(*mBindAttribFragFunc)(uint32_t shaderProgram) = nullptr;
};

// Program compilation & linking helper functions
// ------------------------------------------------------------------------------------------------

/// Compiles shader
/// \param shaderType is a GLenum and can for example be of type GL_FRAGMENT_SHADER
/// \return the compiled shader, 0 if failed.
uint32_t compileShader(const char* source, uint32_t shaderType) noexcept;

/// Links an OpenGL program and returns whether succesful or not.
bool linkProgram(uint32_t program) noexcept;

/// Prints the shader info log, typically called if compilation (or linking) failed.
void printShaderInfoLog(uint32_t shader) noexcept;

/// Returns the source to the default post process vertex shader.
const char* postProcessVertexShaderSource() noexcept;

// Uniform setters
// ------------------------------------------------------------------------------------------------

void setUniform(int location, int i) noexcept;
void setUniform(const Program& program, const char* name, int i) noexcept;
void setUniform(int location, const int* intArray, size_t count) noexcept;
void setUniform(const Program& program, const char* name, const int* intArray, size_t count) noexcept;

void setUniform(int location, uint32_t u) noexcept;
void setUniform(const Program& program, const char* name, uint32_t u) noexcept;
void setUniform(int location, const uint32_t* uintArray, size_t count) noexcept;
void setUniform(const Program& program, const char* name, const uint32_t* uintArray, size_t count) noexcept;

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
void setUniform(int location, const mat44* matrixArray, size_t count) noexcept;
void setUniform(const Program& program, const char* name, const mat44* matrixArray, size_t count) noexcept;

} // namespace gl
} // namespace sfz
