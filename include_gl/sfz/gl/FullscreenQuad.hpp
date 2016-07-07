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

/// A FullscreenQuad used to run the standard post-process shader in gl::Program
class FullscreenQuad final {
public:

	// Copying not allowed
	FullscreenQuad(const FullscreenQuad&) = delete;
	FullscreenQuad& operator= (const FullscreenQuad&) = delete;

	FullscreenQuad() noexcept;
	FullscreenQuad(FullscreenQuad&& other) noexcept;
	FullscreenQuad& operator= (FullscreenQuad&& other) noexcept;
	~FullscreenQuad() noexcept;

	void render() noexcept;

private:
	uint32_t mVAO = 0;
	uint32_t mPosBuffer = 0;
	uint32_t mUVBuffer = 0;
	uint32_t mIndexBuffer = 0;
};

} // namespace gl
} // namespace sfz
