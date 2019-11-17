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

#include <sfz/memory/Allocator.hpp>
#include <sfz/strings/DynString.hpp>

namespace sfz {

namespace gl {

using std::uint32_t;

// Program class
// ------------------------------------------------------------------------------------------------

/// A class holding an OpenGL Program.
///
/// The post process variants will create a Program using the default post process vertex shader,
/// accessable by calling "postProcessVertexShaderSource()". Check the source code to see what
/// outputs you receive in the fragment shader.
class Program final {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	// Copying not allowed
	Program(const Program&) = delete;
	Program& operator= (const Program&) = delete;

	Program() noexcept = default;
	Program(Program&& other) noexcept { this->swap(other); }
	Program& operator= (Program&& other) noexcept { this->swap(other); return *this; }
	~Program() noexcept { this->destroy(); }

	// State methods
	// --------------------------------------------------------------------------------------------

	// Swaps two Programs
	void swap(Program& other) noexcept;

	// Destroys a program, leaving it in the same state as after a default construction.
	void destroy() noexcept;

	// Constructor functions (from source)
	// --------------------------------------------------------------------------------------------

	/// Constructs an OpenGL program given source strings. The reload() method will not have any
	/// effect on a program created directly from source by these functions.
	/// \param headerSrc common header string that will be appended to the top of all other sources
	///                  before compilation
	/// \param *Src the source string for a specific shader
	/// \param bindAttribFragFunc an optional function pointer used to call glBindAttribLocation() &
	///                           glBindFragDataLocation()
	/// \param allocator the allocator to use

	static Program fromSource(
		const char* headerSrc,
		const char* vertexSrc,
		const char* fragmentSrc,
		void(*bindAttribFragFunc)(uint32_t shaderProgram) = nullptr,
		Allocator* allocator = getDefaultAllocator()) noexcept;

#if !defined(__EMSCRIPTEN__) && !defined(SFZ_IOS)
	static Program fromSource(
		const char* headerSrc,
		const char* vertexSrc,
		const char* geometrySrc,
		const char* fragmentSrc,
		void(*bindAttribFragFunc)(uint32_t shaderProgram) = nullptr,
		Allocator* allocator = getDefaultAllocator()) noexcept;
#endif

	static Program postProcessFromSource(
		const char* headerSrc,
		const char* postProcessSource,
		Allocator* allocator = getDefaultAllocator()) noexcept;

	// Constructor functions (from file)
	// --------------------------------------------------------------------------------------------

	/// Constructs an OpenGL program given file paths to source
	/// The file paths are stored and when reload() is called the program will be recompiled.
	/// \param basePath the path to the directory the source files are located in
	/// \param headerFile the filename of the file containing the common header string that will be
	///                   appended to the top of all other sources before compiling. May be empty
	///                   emptry string or nullptr if no such header is wanted.
	/// \param *File the filename of the file containing the shader source for a specific shader
	/// \param bindAttribFragFunc an optional function pointer used to call glBindAttribLocation() &
	///                           glBindFragDataLocation()
	/// \param allocator the allocator to use

	static Program fromFile(
		const char* basePath,
		const char* headerFile,
		const char* vertexFile,
		const char* fragmentFile,
		void(*bindAttribFragFunc)(uint32_t shaderProgram) = nullptr,
		Allocator* allocator = getDefaultAllocator()) noexcept;

#if !defined(__EMSCRIPTEN__) && !defined(SFZ_IOS)
	static Program fromFile(
		const char* basePath,
		const char* headerFile,
		const char* vertexFile,
		const char* geometryFile,
		const char* fragmentFile,
		void(*bindAttribFragFunc)(uint32_t shaderProgram) = nullptr,
		Allocator* allocator = getDefaultAllocator()) noexcept;
#endif

	static Program postProcessFromFile(
		const char* basePath,
		const char* headerFile,
		const char* postProcessFile,
		Allocator* allocator = getDefaultAllocator()) noexcept;

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

private:
	// Private members
	// --------------------------------------------------------------------------------------------

	// Allocator used for allocating temporary and more permanent strings
	Allocator* mAllocator = nullptr;

	// The handle to the current OpenGL program
	uint32_t mHandle = 0;

	// Optional paths to shader source files
	DynString mHeaderPath;
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

} // namespace gl
} // namespace sfz
