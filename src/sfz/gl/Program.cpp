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

#include "sfz/Assert.hpp"
#include "sfz/gl/IncludeOpenGL.hpp"
#include "sfz/util/IO.hpp"

namespace sfz {

namespace gl {

// Statics
// ------------------------------------------------------------------------------------------------

static const char* POST_PROCESS_VERTEX_SHADER_SOURCE = R"(
	#version 330

	// Input
	in vec3 inPosition;
	in vec2 inUV;

	// Uniforms
	uniform mat4 uInvProjMatrix = mat4(1);

	// Output
	out vec2 uvCoord;
	out vec3 nonNormRayDir;

	void main()
	{
		gl_Position = vec4(inPosition, 1.0);
		uvCoord = inUV;

		vec4 nonNormRayDirTmp = (uInvProjMatrix * vec4(inPosition.xy, 0.0, 1.0));
		nonNormRayDirTmp /= nonNormRayDirTmp.w; // Not sure if necessary
		nonNormRayDir = nonNormRayDirTmp.xyz;
	}
)";

// Program: Constructor functions
// ------------------------------------------------------------------------------------------------

Program Program::fromSource(const char* vertexSrc, const char* fragmentSrc,
                            void(*bindAttribFragFunc)(uint32_t shaderProgram)) noexcept
{
	GLuint vertexShader = compileShader(vertexSrc, GL_VERTEX_SHADER);
	if (vertexShader == 0) {
		sfz::printErrorMessage("Couldn't compile vertex shader.");
		return Program();
	}
	
	GLuint fragmentShader = compileShader(fragmentSrc, GL_FRAGMENT_SHADER);
	if (fragmentShader == 0) {
		sfz::printErrorMessage("Couldn't compile fragment shader.");
		return Program();
	}

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
		sfz::printErrorMessage("Couldn't link shader program.");
		return Program();
	}
	
	Program temp;
	temp.mHandle = shaderProgram;
	temp.mBindAttribFragFunc = bindAttribFragFunc;
	return temp;
}

Program Program::fromSource(const char* vertexSrc, const char* geometrySrc, const char* fragmentSrc,
                            void(*bindAttribFragFunc)(uint32_t shaderProgram)) noexcept
{
	GLuint vertexShader = compileShader(vertexSrc, GL_VERTEX_SHADER);
	if (vertexShader == 0) {
		sfz::printErrorMessage("Couldn't compile vertex shader.");
		return Program();
	}
	
	GLuint geometryShader = compileShader(geometrySrc, GL_GEOMETRY_SHADER);
	if (geometryShader == 0) {
		sfz::printErrorMessage("Couldn't compile geometry shader.");
		return Program();
	}

	GLuint fragmentShader = compileShader(fragmentSrc, GL_FRAGMENT_SHADER);
	if (fragmentShader == 0) {
		sfz::printErrorMessage("Couldn't compile fragment shader.");
		return Program();
	}

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
		sfz::printErrorMessage("Couldn't link shader program.");
		return Program();
	}
	
	Program temp;
	temp.mHandle = shaderProgram;
	temp.mBindAttribFragFunc = bindAttribFragFunc;
	return temp;
}

Program Program::postProcessFromSource(const char* postProcessSource) noexcept
{
	Program tmp;
	tmp = Program::fromSource(POST_PROCESS_VERTEX_SHADER_SOURCE, postProcessSource, [](uint32_t shaderProgram) {
		glBindAttribLocation(shaderProgram, 0, "inPosition");
		glBindAttribLocation(shaderProgram, 1, "inUV");
	});
	tmp.mIsPostProcess = true;
	return std::move(tmp);
}

Program Program::fromFile(const char* basePath, const char* vertexFile, const char* fragmentFile,
                          void(*bindAttribFragFunc)(uint32_t shaderProgram)) noexcept
{
	size_t basePathLen = std::strlen(basePath);
	size_t vertexFileLen = std::strlen(vertexFile);
	size_t fragmentFileLen = std::strlen(fragmentFile);

	Program tmp;
	tmp.mVertexPath.setCapacity(uint32_t(basePathLen + vertexFileLen + 1));
	tmp.mVertexPath.printf("%s%s", basePath, vertexFile);
	tmp.mFragmentPath.setCapacity(uint32_t(basePathLen + fragmentFileLen + 1));
	tmp.mFragmentPath.printf("%s%s", basePath, fragmentFile);
	tmp.mBindAttribFragFunc = bindAttribFragFunc;
	tmp.reload();
	tmp.mWasReloaded = false;
	return tmp;
}

Program Program::fromFile(const char* basePath, const char* vertexFile,
                          const char* geometryFile, const char* fragmentFile,
                          void(*bindAttribFragFunc)(uint32_t shaderProgram)) noexcept
{
	size_t basePathLen = std::strlen(basePath);
	size_t vertexFileLen = std::strlen(vertexFile);
	size_t geometryFileLen = std::strlen(geometryFile);
	size_t fragmentFileLen = std::strlen(fragmentFile);

	Program tmp;
	tmp.mVertexPath.setCapacity(uint32_t(basePathLen + vertexFileLen + 1));
	tmp.mVertexPath.printf("%s%s", basePath, vertexFile);
	tmp.mGeometryPath.setCapacity(uint32_t(basePathLen + geometryFileLen + 1));
	tmp.mGeometryPath.printf("%s%s", basePath, geometryFile);
	tmp.mFragmentPath.setCapacity(uint32_t(basePathLen + fragmentFileLen + 1));
	tmp.mFragmentPath.printf("%s%s", basePath, fragmentFile);
	tmp.mBindAttribFragFunc = bindAttribFragFunc;
	tmp.reload();
	tmp.mWasReloaded = false;
	return tmp;
}

Program Program::postProcessFromFile(const char* basePath, const char* postProcessFile) noexcept
{
	size_t basePathLen = std::strlen(basePath);
	size_t postProcessFileLen = std::strlen(postProcessFile);

	Program tmp;
	tmp.mFragmentPath.setCapacity(uint32_t(basePathLen + postProcessFileLen + 1));
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
	const DynString vertexSrc = sfz::readTextFile(mVertexPath.str());
	const DynString geometrySrc = sfz::readTextFile(mGeometryPath.str());
	const DynString fragmentSrc = sfz::readTextFile(mFragmentPath.str());

	if (mIsPostProcess && (fragmentSrc.size() > 0)) {
		Program tmp = Program::postProcessFromSource(fragmentSrc.str());
		if (!tmp.isValid()) return false;

		tmp.mFragmentPath = this->mFragmentPath;
		tmp.mIsPostProcess = true;
		tmp.mWasReloaded = true;
		*this = std::move(tmp);
		return true;
	}
	else if ((vertexSrc.size() > 0) && (geometrySrc.size() > 0) && (fragmentSrc.size() > 0)) {
		Program tmp = Program::fromSource(vertexSrc.str(), geometrySrc.str(), fragmentSrc.str(),
		                                  mBindAttribFragFunc);
		if (!tmp.isValid()) return false;

		tmp.mVertexPath = this->mVertexPath;
		tmp.mGeometryPath = this->mGeometryPath;
		tmp.mFragmentPath = this->mFragmentPath;
		tmp.mBindAttribFragFunc = this->mBindAttribFragFunc;
		tmp.mWasReloaded = true;
		*this = std::move(tmp);
		return true;
	}
	else if ((vertexSrc.size() > 0) && (fragmentSrc.size() > 0)) {
		Program tmp = Program::fromSource(vertexSrc.str(), fragmentSrc.str(), mBindAttribFragFunc);
		if (!tmp.isValid()) return false;

		tmp.mVertexPath = this->mVertexPath;
		tmp.mFragmentPath = this->mFragmentPath;
		tmp.mBindAttribFragFunc = this->mBindAttribFragFunc;
		tmp.mWasReloaded = true;
		*this = std::move(tmp);
		return true;
	}

	return false;
}

void Program::useProgram() noexcept
{
	if (!this->isValid()) return;
	glUseProgram(mHandle);
}

// Program: Constructors & destructors
// ------------------------------------------------------------------------------------------------

Program::Program(Program&& other) noexcept
{
	std::swap(this->mHandle, other.mHandle);
	std::swap(this->mVertexPath, other.mVertexPath);
	std::swap(this->mFragmentPath, other.mFragmentPath);
	std::swap(this->mIsPostProcess, other.mIsPostProcess);
	std::swap(this->mWasReloaded, other.mWasReloaded);
	std::swap(this->mBindAttribFragFunc, other.mBindAttribFragFunc);
}

Program& Program::operator= (Program&& other) noexcept
{
	std::swap(this->mHandle, other.mHandle);
	std::swap(this->mVertexPath, other.mVertexPath);
	std::swap(this->mFragmentPath, other.mFragmentPath);
	std::swap(this->mIsPostProcess, other.mIsPostProcess);
	std::swap(this->mWasReloaded, other.mWasReloaded);
	std::swap(this->mBindAttribFragFunc, other.mBindAttribFragFunc);
	return *this;
}

Program::~Program() noexcept
{
	glDeleteProgram(mHandle); // Silently ignored if mHandle == 0.
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
	printErrorMessage(log);
}

const char* postProcessVertexShaderSource() noexcept
{
	return POST_PROCESS_VERTEX_SHADER_SOURCE;
}

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
	glUniformMatrix4fv(location, 1, true, matrix.data());
}

void setUniform(const Program& program, const char* name, const mat44& matrix) noexcept
{
	int loc = glGetUniformLocation(program.handle(), name);
	setUniform(loc, matrix);
}

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

} // namespace gl
} // namespace sfz
