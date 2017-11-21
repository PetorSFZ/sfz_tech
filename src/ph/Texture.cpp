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

#include "ph/Texture.hpp"

#include <algorithm>

#include <sfz/gl/IncludeOpenGL.hpp>

namespace ph {

// Texture: State methods
// ------------------------------------------------------------------------------------------------

void Texture::create(const phConstImageView& imageView, TextureFiltering filtering) noexcept
{
	if (imageView.rawData == nullptr) return;
	if (mTextureHandle != 0) this->destroy();

	// Creating OpenGL texture
	glGenTextures(1, &mTextureHandle);
	glBindTexture(GL_TEXTURE_2D, mTextureHandle);

	// Transfer data from raw image
#ifdef __EMSCRIPTEN__
	switch (imageView.bytesPerPixel) {
	case 1:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, imageView.width, imageView.height, 0,
		             GL_LUMINANCE, GL_UNSIGNED_BYTE, imageView.rawData);
		break;
	case 3:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imageView.width, imageView.height, 0,
		             GL_RGB, GL_UNSIGNED_BYTE, imageView.rawData);
		break;
	case 4:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageView.width, imageView.height, 0,
		             GL_RGBA, GL_UNSIGNED_BYTE, imageView.rawData);
		break;
	}
#else
	switch (imageView.bytesPerPixel) {
	case 1:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, imageView.width, imageView.height, 0, GL_RED,
		             GL_UNSIGNED_BYTE, imageView.rawData);
		break;
	case 2:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RG8, imageView.width, imageView.height, 0, GL_RG,
		             GL_UNSIGNED_BYTE, imageView.rawData);
		break;
	case 3:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, imageView.width, imageView.height, 0, GL_RGB,
		             GL_UNSIGNED_BYTE, imageView.rawData);
		break;
	case 4:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, imageView.width, imageView.height, 0, GL_RGBA,
		             GL_UNSIGNED_BYTE, imageView.rawData);
		break;
	}
#endif

	// Set filtering format (first small hack to ensure setFilteringFormat will be run)
	mFiltering = filtering != TextureFiltering::NEAREST ?
	             TextureFiltering::NEAREST : TextureFiltering::BILINEAR;
	setFilteringFormat(filtering);
}

void Texture::swap(Texture& other) noexcept
{
	std::swap(this->mTextureHandle, other.mTextureHandle);
	std::swap(this->mFiltering, other.mFiltering);
}

void Texture::destroy() noexcept
{
	// Destroy texture, silently ignores mTextureHandle == 0
	glDeleteTextures(1, &mTextureHandle);

	// Reset variables
	mTextureHandle = 0u;
	mFiltering = TextureFiltering::NEAREST;
}

// Texture: Private methods
// ------------------------------------------------------------------------------------------------

void Texture::setFilteringFormat(TextureFiltering filtering) noexcept
{
	if (mTextureHandle == 0) return;
	if (mFiltering == filtering) return;
	mFiltering = filtering;

	glBindTexture(GL_TEXTURE_2D, mTextureHandle);

	// Sets specified texture filtering, generating mipmaps if needed.
	switch (mFiltering) {
	case TextureFiltering::NEAREST:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		break;
	case TextureFiltering::BILINEAR:
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		break;
	case TextureFiltering::TRILINEAR:
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		break;
	case TextureFiltering::ANISOTROPIC:
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		float factor = 0.0f;
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &factor);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, factor);
		break;
	}
}

} // namespace ph
