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

namespace sfz {

namespace gl {

using std::uint32_t;

// FullscreenGeometryType enum
// ------------------------------------------------------------------------------------------------

enum class FullscreenGeometryType {
	OGL_CLIP_SPACE_RIGHT_HANDED_FRONT_FACE
};

// FullscreenGeometry class
// ------------------------------------------------------------------------------------------------

class FullscreenGeometry final {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	FullscreenGeometry() noexcept = default;
	FullscreenGeometry(const FullscreenGeometry&) = delete;
	FullscreenGeometry& operator= (const FullscreenGeometry&) = delete;
	FullscreenGeometry(FullscreenGeometry&& o) noexcept { this->swap(o); }
	FullscreenGeometry& operator= (FullscreenGeometry&& o) noexcept { this->swap(o); return *this; }
	~FullscreenGeometry() noexcept { this->destroy(); }

	// State methods
	// --------------------------------------------------------------------------------------------

	void create(FullscreenGeometryType type) noexcept;

	void swap(FullscreenGeometry& other) noexcept;

	void destroy() noexcept;

	// Methods
	// --------------------------------------------------------------------------------------------

	bool isValid() const noexcept { return mVAO != 0; }

	void render() noexcept;

private:
	uint32_t mVAO = 0;
	uint32_t mVertexBuffer = 0;
	uint32_t mIndexBuffer = 0;
};

} // namespace gl

} // namespace sfz
