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

#include "sfz/math/Matrix.hpp"

namespace sfz {

// GL View matrix (OGL right-handed, negative z into screen, positive x to the right)
// ------------------------------------------------------------------------------------------------

mat4 viewMatrixGL(const vec3& origin, const vec3& dir, const vec3& up) noexcept;

// Projection matrices (Standard OpenGL [-1, 1] right-handed clip space, GL view space)
// ------------------------------------------------------------------------------------------------

mat4 orthogonalProjectionGL(float left, float bottom, float right, float top,
                            float zNear, float zFar) noexcept;

mat4 orthogonalProjectionGL(vec3 leftBottomNear, vec3 rightTopFar) noexcept;

mat3 orthogonalProjection2DGL(vec2 center, vec2 dimensions) noexcept;

mat4 perspectiveProjectionGL(float left, float bottom, float right, float top,
                             float zNear, float zFar) noexcept;

/// Creates a perspective matrix for use with OpenGL.
/// \param yFovDeg the vertical fov in degrees
/// \param aspectRatio the width / height ratio of the frustrum
/// \param zNear the near plane
/// \param zFar the far plane
mat4 perspectiveProjectionGL(float yFovDeg, float aspectRatio, float zNear, float zFar) noexcept;

// Projection matrices (D3D/Vulkan [0, 1] left-handed clip space, GL view space)
// ------------------------------------------------------------------------------------------------

/// These are meant to be used on OpenGL with D3D clip space and back face culling winding order.
/// I.e. glClipControl(GL_UPPER_LEFT, GL_ZERO_TO_ONE) and glFrontFace(GL_CW)

mat4 perspectiveProjectionVkD3d(float left, float bottom, float right, float top,
                                float zNear, float zfar) noexcept;

mat4 perspectiveProjectionVkD3d(float yFovDeg, float aspectRatio, float zNear, float zFar) noexcept;


mat4 reverseInfinitePerspectiveProjectionVkD3d(float left, float bottom, float right, float top,
                                               float zNear) noexcept;

mat4 reverseInfinitePerspectiveProjectionVkD3d(float yFovDeg, float aspect, float zNear) noexcept;

} // namespace sfz
