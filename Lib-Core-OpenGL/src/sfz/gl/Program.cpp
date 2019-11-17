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

#include "sfz/gl/Program.hpp"

#include <algorithm>
#include <cstring>

#include "sfz/Assert.hpp"
#include "sfz/Logging.hpp"
#include "sfz/gl/IncludeOpenGL.hpp"
#include "sfz/util/IO.hpp"

namespace sfz {

namespace gl {

// Program: State methods
// ------------------------------------------------------------------------------------------------

void Program::swap(Program& other) noexcept
{
	std::swap(this->mAllocator, other.mAllocator);
	std::swap(this->mHandle, other.mHandle);
	std::swap(this->mHeaderPath, other.mHeaderPath);
	std::swap(this->mVertexPath, other.mVertexPath);
	std::swap(this->mGeometryPath, other.mGeometryPath);
	std::swap(this->mFragmentPath, other.mFragmentPath);
	std::swap(this->mIsPostProcess, other.mIsPostProcess);
	std::swap(this->mWasReloaded, other.mWasReloaded);
	std::swap(this->mBindAttribFragFunc, other.mBindAttribFragFunc);
}

void Program::destroy() noexcept
{
	// Delete program handle
	glDeleteProgram(mHandle); // Silently ignored if mHandle == 0.

	// Reset variables
	mAllocator = nullptr;
	mHandle = 0;
	mHeaderPath.destroy();
	mVertexPath.destroy();
	mGeometryPath.destroy();
	mFragmentPath.destroy();
	mIsPostProcess = false;
	mWasReloaded = false;
	mBindAttribFragFunc = nullptr;
}

// Program: Constructor functions (from source)
// ------------------------------------------------------------------------------------------------

Program Program::fromSource(
	const char* headerSrc,
	const char* vertexSrc,
	const char* fragmentSrc,
	void(*bindAttribFragFunc)(uint32_t shaderProgram),
	Allocator* allocator) noexcept
{
	// Calculate length of shader sources
	if (headerSrc == nullptr) headerSrc = "";
	uint32_t headerSrcLen = uint32_t(std::strlen(headerSrc));
	uint32_t vertexSrcLen = uint32_t(std::strlen(vertexSrc));
	uint32_t fragmentSrcLen = uint32_t(std::strlen(fragmentSrc));

	// Concatenate shader sources
	DynString vertexConcatSrc("", uint32_t(headerSrcLen + vertexSrcLen + 5), allocator);
	vertexConcatSrc.printf("%s\n%s", headerSrc, vertexSrc);

	DynString fragmentConcatSrc("", uint32_t(headerSrcLen + fragmentSrcLen + 5), allocator);
	fragmentConcatSrc.printf("%s\n%s", headerSrc, fragmentSrc);

	// Compile shaders
	GLuint vertexShader = compileShader(vertexConcatSrc.str(), GL_VERTEX_SHADER);
	if (vertexShader == 0) {
		SFZ_ERROR("sfzGL", "Couldn't compile vertex shader.");
		return Program();
	}

	GLuint fragmentShader = compileShader(fragmentConcatSrc.str(), GL_FRAGMENT_SHADER);
	if (fragmentShader == 0) {
		SFZ_ERROR("sfzGL", "Couldn't compile fragment shader.");
		return Program();
	}

	// Link shaders into program
	GLuint shaderProgram = glCreateProgram();

	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);

	// glBindAttribLocation() & glBindFragDataLocation()
	if (bindAttribFragFunc != nullptr) bindAttribFragFunc(shaderProgram);

	bool linkSuccess = linkProgram(shaderProgram);

	glDetachShader(shaderProgram, vertexShader);
	glDetachShader(shaderProgram, fragmentShader);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	if (!linkSuccess) {
		glDeleteProgram(shaderProgram);
		SFZ_ERROR("sfzGL", "Couldn't link shader program.");
		return Program();
	}

	// Set Program members and return
	Program tmp;
	tmp.mAllocator = allocator;
	tmp.mHandle = shaderProgram;
	tmp.mBindAttribFragFunc = bindAttribFragFunc;
	return tmp;
}

#if !defined(__EMSCRIPTEN__) && !defined(SFZ_IOS)
Program Program::fromSource(
	const char* headerSrc,
	const char* vertexSrc,
	const char* geometrySrc,
	const char* fragmentSrc,
	void(*bindAttribFragFunc)(uint32_t shaderProgram),
	Allocator* allocator) noexcept
{
	// Calculate length of shader sources
	if (headerSrc == nullptr) headerSrc = "";
	uint32_t headerSrcLen = uint32_t(std::strlen(headerSrc));
	uint32_t vertexSrcLen = uint32_t(std::strlen(vertexSrc));
	uint32_t geometrySrcLen = uint32_t(std::strlen(geometrySrc));
	uint32_t fragmentSrcLen = uint32_t(std::strlen(fragmentSrc));

	// Concatenate shader sources
	DynString vertexConcatSrc("", headerSrcLen + vertexSrcLen + 5, allocator);
	vertexConcatSrc.printf("%s\n%s", headerSrc, vertexSrc);

	DynString geometryConcatSrc("", headerSrcLen + geometrySrcLen + 5, allocator);
	geometryConcatSrc.printf("%s\n%s", headerSrc, geometrySrc);

	DynString fragmentConcatSrc("", headerSrcLen + fragmentSrcLen + 5, allocator);
	fragmentConcatSrc.printf("%s\n%s", headerSrc, fragmentSrc);

	// Compile shaders
	GLuint vertexShader = compileShader(vertexConcatSrc.str(), GL_VERTEX_SHADER);
	if (vertexShader == 0) {
		SFZ_ERROR("sfzGL", "Couldn't compile vertex shader.");
		return Program();
	}

	GLuint geometryShader = compileShader(geometryConcatSrc.str(), GL_GEOMETRY_SHADER);
	if (geometryShader == 0) {
		SFZ_ERROR("sfzGL", "Couldn't compile geometry shader.");
		return Program();
	}

	GLuint fragmentShader = compileShader(fragmentConcatSrc.str(), GL_FRAGMENT_SHADER);
	if (fragmentShader == 0) {
		SFZ_ERROR("sfzGL", "Couldn't compile fragment shader.");
		return Program();
	}

	// Link shaders into program
	GLuint shaderProgram = glCreateProgram();

	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, geometryShader);
	glAttachShader(shaderProgram, fragmentShader);

	// glBindAttribLocation() & glBindFragDataLocation()
	if (bindAttribFragFunc != nullptr) bindAttribFragFunc(shaderProgram);

	bool linkSuccess = linkProgram(shaderProgram);

	glDetachShader(shaderProgram, vertexShader);
	glDetachShader(shaderProgram, geometryShader);
	glDetachShader(shaderProgram, fragmentShader);

	glDeleteShader(vertexShader);
	glDeleteShader(geometryShader);
	glDeleteShader(fragmentShader);

	if (!linkSuccess) {
		glDeleteProgram(shaderProgram);
		SFZ_ERROR("sfzGL", "Couldn't link shader program.");
		return Program();
	}

	// Set Program members and return
	Program tmp;
	tmp.mAllocator = allocator;
	tmp.mHandle = shaderProgram;
	tmp.mBindAttribFragFunc = bindAttribFragFunc;
	return tmp;
}
#endif

Program Program::postProcessFromSource(
	const char* headerSrc,
	const char* postProcessSource,
	Allocator* allocator) noexcept
{
	Program tmp = Program::fromSource(
		headerSrc,
		postProcessVertexShaderSource(),
		postProcessSource,
		[](uint32_t shaderProgram) {
			glBindAttribLocation(shaderProgram, 0, "inPos");
			glBindAttribLocation(shaderProgram, 1, "inTexcoord");
		},
		allocator);
	tmp.mIsPostProcess = true;
	return tmp;
}

// Program: Constructor functions (from file)
// ------------------------------------------------------------------------------------------------

Program Program::fromFile(
	const char* basePath,
	const char* headerFile,
	const char* vertexFile,
	const char* fragmentFile,
	void(*bindAttribFragFunc)(uint32_t shaderProgram),
	Allocator* allocator) noexcept
{
	if (headerFile == nullptr) headerFile = "";

	uint32_t basePathLen = uint32_t(std::strlen(basePath));
	uint32_t headerFileLen = uint32_t(std::strlen(headerFile));
	uint32_t vertexFileLen = uint32_t(std::strlen(vertexFile));
	uint32_t fragmentFileLen = uint32_t(std::strlen(fragmentFile));

	Program tmp;
	tmp.mAllocator = allocator;

	tmp.mHeaderPath = DynString("", uint32_t(basePathLen + headerFileLen + 1), allocator);
	tmp.mHeaderPath.printf("%s%s", basePath, headerFile);

	tmp.mVertexPath = DynString("", uint32_t(basePathLen + vertexFileLen + 1), allocator);
	tmp.mVertexPath.printf("%s%s", basePath, vertexFile);

	tmp.mFragmentPath = DynString("", uint32_t(basePathLen + fragmentFileLen + 1), allocator);
	tmp.mFragmentPath.printf("%s%s", basePath, fragmentFile);

	tmp.mBindAttribFragFunc = bindAttribFragFunc;

	tmp.reload();
	tmp.mWasReloaded = false;

	return tmp;
}

#if !defined(__EMSCRIPTEN__) && !defined(SFZ_IOS)
Program Program::fromFile(
	const char* basePath,
	const char* headerFile,
	const char* vertexFile,
	const char* geometryFile,
	const char* fragmentFile,
	void(*bindAttribFragFunc)(uint32_t shaderProgram),
	Allocator* allocator) noexcept
{
	if (headerFile == nullptr) headerFile = "";

	uint32_t basePathLen = uint32_t(std::strlen(basePath));
	uint32_t headerFileLen = uint32_t(std::strlen(headerFile));
	uint32_t vertexFileLen = uint32_t(std::strlen(vertexFile));
	uint32_t geometryFileLen = uint32_t(std::strlen(geometryFile));
	uint32_t fragmentFileLen = uint32_t(std::strlen(fragmentFile));

	Program tmp;
	tmp.mAllocator = allocator;

	tmp.mHeaderPath = DynString("", uint32_t(basePathLen + headerFileLen + 1), allocator);
	tmp.mHeaderPath.printf("%s%s", basePath, headerFile);

	tmp.mVertexPath = DynString("", uint32_t(basePathLen + vertexFileLen + 1), allocator);
	tmp.mVertexPath.printf("%s%s", basePath, vertexFile);

	tmp.mGeometryPath = DynString("", uint32_t(basePathLen + geometryFileLen + 1), allocator);
	tmp.mGeometryPath.printf("%s%s", basePath, geometryFile);

	tmp.mFragmentPath = DynString("", uint32_t(basePathLen + fragmentFileLen + 1), allocator);
	tmp.mFragmentPath.printf("%s%s", basePath, fragmentFile);

	tmp.mBindAttribFragFunc = bindAttribFragFunc;

	tmp.reload();
	tmp.mWasReloaded = false;

	return tmp;
}
#endif

Program Program::postProcessFromFile(
	const char* basePath,
	const char* headerFile,
	const char* postProcessFile,
	Allocator* allocator) noexcept
{
	if (headerFile == nullptr) headerFile = "";

	uint32_t basePathLen = uint32_t(std::strlen(basePath));
	uint32_t headerFileLen = uint32_t(std::strlen(headerFile));
	uint32_t postProcessFileLen = uint32_t(std::strlen(postProcessFile));

	Program tmp;
	tmp.mAllocator = allocator;

	tmp.mHeaderPath = DynString("", uint32_t(basePathLen + headerFileLen + 1), allocator);
	tmp.mHeaderPath.printf("%s%s", basePath, headerFile);

	tmp.mFragmentPath = DynString("", uint32_t(basePathLen + postProcessFileLen + 1), allocator);
	tmp.mFragmentPath.printf("%s%s", basePath, postProcessFile);

	tmp.mIsPostProcess = true;
	tmp.reload();
	tmp.mWasReloaded = false;

	return tmp;
}

// Program: Public methods
// ------------------------------------------------------------------------------------------------

bool Program::reload() noexcept
{
	// Load source from files
	const DynString headerSrc = sfz::readTextFile(mHeaderPath.str(), mAllocator);
	const DynString vertexSrc = sfz::readTextFile(mVertexPath.str(), mAllocator);
	const DynString geometrySrc = sfz::readTextFile(mGeometryPath.str(), mAllocator);
	const DynString fragmentSrc = sfz::readTextFile(mFragmentPath.str(), mAllocator);

	// Temporary program to compile to
	Program tmpProgram;

	// Post process shader
	if (mIsPostProcess && (mVertexPath.size() > 0)) {
		tmpProgram = Program::postProcessFromSource(
			headerSrc.str(),
			fragmentSrc.str(),
			mAllocator);
	}

#if !defined(__EMSCRIPTEN__) && !defined(SFZ_IOS)
	// Vertex + Geometry + Fragment shader
	else if ((mVertexPath.size() > 0) && (mGeometryPath.size() > 0) && (mFragmentPath.size() > 0)) {
		tmpProgram = Program::fromSource(
			headerSrc.str(),
			vertexSrc.str(),
			geometrySrc.str(),
			fragmentSrc.str(),
			mBindAttribFragFunc,
			mAllocator);
	}
#endif

	// Vertex + Fragment shader
	else if ((mVertexPath.size() > 0) && (mFragmentPath.size() > 0)) {
		tmpProgram = Program::fromSource(
			headerSrc.str(),
			vertexSrc.str(),
			fragmentSrc.str(),
			mBindAttribFragFunc,
			mAllocator);
	}

	// Not a valid configuration
	else {
		return false;
	}

	// If temporary program is not valid compilation failed, return false.
	if (!tmpProgram.isValid()) return false;

	// Swap newly compiled handle from temporary to this instance
	std::swap(this->mHandle, tmpProgram.mHandle);
	this->mWasReloaded = true;

	return true;
}

void Program::useProgram() noexcept
{
	if (!this->isValid()) return;
	glUseProgram(mHandle);
}

// Program compilation & linking helper functions
// ------------------------------------------------------------------------------------------------

uint32_t compileShader(const char* source, uint32_t shaderType) noexcept
{
	GLuint shader = glCreateShader(shaderType);
	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);

	int compileSuccess;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compileSuccess);
	if (!compileSuccess) {
		printShaderInfoLog(shader);
		glDeleteShader(shader);
		return 0;
	}

	return shader;
}

bool linkProgram(uint32_t program) noexcept
{
	glLinkProgram(program);
	GLint linkSuccess = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &linkSuccess);
	if (!linkSuccess) {
		printShaderInfoLog(program);
		return false;
	}
	return true;
}

void printShaderInfoLog(uint32_t shader) noexcept
{
	//int logLength;
	//glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
	const int MAX_LOG_LENGTH = 256;
	char log[MAX_LOG_LENGTH];
	glGetShaderInfoLog(shader, MAX_LOG_LENGTH, NULL, log);
	SFZ_ERROR("sfzGL", "%s", log);
}

const char* postProcessVertexShaderSource() noexcept
{
#if defined(__EMSCRIPTEN__) || defined(SFZ_IOS)
	return R"(
		// Input
		attribute vec3 inPos;
		attribute vec2 inTexcoord;

		// Output
		varying vec2 texcoord;

		void main()
		{
			gl_Position = vec4(inPos, 1.0);
			texcoord = inTexcoord;
		}
	)";
#else
	return R"(
		// Input
		in vec3 inPos;
		in vec2 inTexcoord;

		// Output
		out vec2 texcoord;

		void main()
		{
			gl_Position = vec4(inPos, 1.0);
			texcoord = inTexcoord;
		}
	)";
#endif
}

} // namespace gl
} // namespace sfz
