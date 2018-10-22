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

#pragma once

#include <cstdint>

#include <ph/rendering/ImageView.hpp>

namespace ph {

using std::uint32_t;

// TextureFiltering enum
// ------------------------------------------------------------------------------------------------

enum class TextureFiltering : uint32_t {
	NEAREST,
	BILINEAR,
	TRILINEAR,
	ANISOTROPIC
};

// Texture class
// ------------------------------------------------------------------------------------------------

class Texture final {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	Texture() noexcept = default;
	Texture(const Texture&) = delete;
	Texture& operator= (const Texture&) = delete;

	Texture(const phConstImageView& imageView,
	        TextureFiltering filtering = TextureFiltering::ANISOTROPIC) noexcept
	{
		this->create(imageView, filtering);
	}
	Texture(Texture&& other) noexcept { this->swap(other); }
	Texture& operator= (Texture&& other) noexcept { this->swap(other); return *this; }
	~Texture() noexcept { this->destroy(); }

	// State methods
	// --------------------------------------------------------------------------------------------

	void create(const phConstImageView& imageView,
	          TextureFiltering filtering = TextureFiltering::ANISOTROPIC) noexcept;

	void swap(Texture& other) noexcept;

	void destroy() noexcept;

	// Methods
	// --------------------------------------------------------------------------------------------

	uint32_t handle() const noexcept { return mTextureHandle; }

	/// Sets the texture filtering format (may generate mipmaps for some formats)
	void setFilteringFormat(TextureFiltering filtering) noexcept;

private:
	// Private members
	// --------------------------------------------------------------------------------------------

	uint32_t mTextureHandle = 0u;
	TextureFiltering mFiltering = TextureFiltering::NEAREST;
};

} // namespace ph
