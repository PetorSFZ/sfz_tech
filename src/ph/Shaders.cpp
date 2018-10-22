// Copyright (c) Peter Hillerström (skipifzero.com, peter@hstroem.se)
//               For other contributors see Contributors.txt
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

#include "ph/Shaders.hpp"

#include <sfz/strings/DynString.hpp>
#include <sfz/gl/IncludeOpenGL.hpp>
#include <sfz/util/IO.hpp>

namespace ph {

// Forward shading
// ------------------------------------------------------------------------------------------------

Program compileForwardShadingShader(Allocator* allocator) noexcept
{
#if defined(__EMSCRIPTEN__) || defined(SFZ_IOS)
	sfz::DynString headerSource =
		sfz::readTextFile("res_compgl/shaders/header_emscripten.glsl", allocator);
#else
	sfz::DynString headerSource =
		sfz::readTextFile("res_compgl/shaders/header_desktop.glsl", allocator);
#endif
	sfz::DynString vertexSource = sfz::readTextFile("res_compgl/shaders/forward.vert", allocator);
	sfz::DynString fragmentSource = sfz::readTextFile("res_compgl/shaders/forward.frag", allocator);

	return Program::fromSource(
		headerSource.str(),
		vertexSource.str(),
		fragmentSource.str(),
		[](uint32_t shaderProgram) {
			glBindAttribLocation(shaderProgram, 0, "inPos");
			glBindAttribLocation(shaderProgram, 1, "inNormal");
			glBindAttribLocation(shaderProgram, 2, "inTexcoord");
		},
		allocator);
}

// Copy out shader
// ------------------------------------------------------------------------------------------------

Program compileCopyOutShader(Allocator* allocator) noexcept
{
#if defined(__EMSCRIPTEN__) || defined(SFZ_IOS)
	sfz::DynString headerSource =
		sfz::readTextFile("res_compgl/shaders/header_emscripten.glsl", allocator);
#else
	sfz::DynString headerSource =
		sfz::readTextFile("res_compgl/shaders/header_desktop.glsl", allocator);
#endif
	sfz::DynString fragmentSource = sfz::readTextFile("res_compgl/shaders/copy_out.frag", allocator);

	return Program::postProcessFromSource(
		headerSource.str(),
		fragmentSource.str(),
		allocator);
}

} // namespace ph
