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

#include "sfz/gl/Framebuffer.hpp"

#include <cstdio>
#include <utility> // std::move()

#include "sfz/gl/IncludeOpenGL.hpp"

namespace sfz {

namespace gl {

// FramebufferBuilder: Constructors & destructors
// ------------------------------------------------------------------------------------------------

FramebufferBuilder::FramebufferBuilder(vec2i dimensions) noexcept
{
	setDimensions(dimensions);
}

// FramebufferBuilder: Component adding methods
// ------------------------------------------------------------------------------------------------

FramebufferBuilder& FramebufferBuilder::setDimensions(vec2i dimensions) noexcept
{
	sfz_assert_debug(dimensions.x > 0);
	sfz_assert_debug(dimensions.y > 0);
	mDim = dimensions;
	return *this;
}

FramebufferBuilder& FramebufferBuilder::addTexture(uint32_t index, FBTextureFormat format, FBTextureFiltering filtering) noexcept
{
	sfz_assert_debug(index < 8);
	sfz_assert_debug(!mCreateTexture[index]);
	mCreateTexture[index] = true;
	mTextureFormat[index] = format;
	mTextureFiltering[index] = filtering;
	return *this;
}

FramebufferBuilder& FramebufferBuilder::addDepthBuffer(FBDepthFormat format) noexcept
{
	sfz_assert_debug(!mCreateDepthBuffer);
	sfz_assert_debug(!mCreateDepthTexture);
	mCreateDepthBuffer = true;
	mDepthFormat = format;
	return *this;
}

FramebufferBuilder& FramebufferBuilder::addDepthTexture(FBDepthFormat format, FBTextureFiltering filtering) noexcept
{
	sfz_assert_debug(!mCreateDepthBuffer);
	sfz_assert_debug(!mCreateDepthTexture);
	mCreateDepthTexture = true;
	mDepthFormat = format;
	mDepthTextureFiltering = filtering;
	return *this;
}

FramebufferBuilder& FramebufferBuilder::addStencilBuffer() noexcept
{
	sfz_assert_debug(!mCreateStencilBuffer);
	sfz_assert_debug(!mCreateStencilTexture);
	mCreateStencilBuffer = true;
	return *this;
}

FramebufferBuilder& FramebufferBuilder::addStencilTexture(FBTextureFiltering filtering) noexcept
{
	sfz_assert_debug(!mCreateStencilBuffer);
	sfz_assert_debug(!mCreateStencilTexture);
	mCreateStencilTexture = true;
	mStencilTextureFiltering = filtering;
	return *this;
}

// FramebufferBuilder: Component removing methods
// ------------------------------------------------------------------------------------------------

FramebufferBuilder& FramebufferBuilder::removeTexture(uint32_t index) noexcept
{
	sfz_assert_debug(index < 8);
	mCreateTexture[index] = false;
	return *this;
}

FramebufferBuilder& FramebufferBuilder::removeDepthBuffer() noexcept
{
	mCreateDepthBuffer = false;
	return *this;
}

FramebufferBuilder& FramebufferBuilder::removeDepthTexture() noexcept
{
	mCreateDepthTexture = false;
	return *this;
}

FramebufferBuilder& FramebufferBuilder::removeStencilBuffer() noexcept
{
	mCreateStencilBuffer = false;
	return *this;
}

FramebufferBuilder& FramebufferBuilder::removeStencilTexture() noexcept
{
	mCreateStencilTexture = false;
	return *this;
}

// FramebufferBuilder: Framebuffer building method
// ------------------------------------------------------------------------------------------------

Framebuffer FramebufferBuilder::build() const noexcept
{
	sfz_assert_debug(mDim.x > 0);
	sfz_assert_debug(mDim.y > 0);
	sfz_assert_debug(!(mCreateDepthBuffer && mCreateDepthTexture));
	sfz_assert_debug(!(mCreateStencilBuffer && mCreateStencilTexture));
	for (uint32_t i = 0; i < 7; ++i) {
		if (mCreateTexture[i+1]) {
			sfz_assert_debug(mCreateTexture[i]);
		}
	}

	Framebuffer tmp;
	tmp.mDim = mDim;

	// Generate framebuffer
	glGenFramebuffers(1, &tmp.mFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, tmp.mFBO);

	// Generate textures
	glActiveTexture(GL_TEXTURE0); // TODO: Not sure if necessary
	uint32_t numTextures = 0;
	for (uint32_t i = 0; i < 8; ++i) {
		if (!mCreateTexture[i]) break;
		numTextures = i + 1;

		glGenTextures(1, &tmp.mTextures[i]);
		glBindTexture(GL_TEXTURE_2D, tmp.mTextures[i]);

		switch (mTextureFormat[i]) {
		case FBTextureFormat::R_U8:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, mDim.x, mDim.y, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
			break;
		case FBTextureFormat::RG_U8:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RG8, mDim.x, mDim.y, 0, GL_RG, GL_UNSIGNED_BYTE, NULL);
			break;
		case FBTextureFormat::RGB_U8:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, mDim.x, mDim.y, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
			break;
		case FBTextureFormat::RGBA_U8:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, mDim.x, mDim.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
			break;

		case FBTextureFormat::R_U16:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R16, mDim.x, mDim.y, 0, GL_RED, GL_UNSIGNED_SHORT, NULL);
			break;
		case FBTextureFormat::RG_U16:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16, mDim.x, mDim.y, 0, GL_RG, GL_UNSIGNED_SHORT, NULL);
			break;
		case FBTextureFormat::RGB_U16:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16, mDim.x, mDim.y, 0, GL_RGB, GL_UNSIGNED_SHORT, NULL);
			break;
		case FBTextureFormat::RGBA_U16:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16, mDim.x, mDim.y, 0, GL_RGBA, GL_UNSIGNED_SHORT, NULL);
			break;

		case FBTextureFormat::R_S8:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R8_SNORM, mDim.x, mDim.y, 0, GL_RED, GL_BYTE, NULL);
			break;
		case FBTextureFormat::RG_S8:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RG8_SNORM, mDim.x, mDim.y, 0, GL_RG, GL_BYTE, NULL);
			break;
		case FBTextureFormat::RGB_S8:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8_SNORM, mDim.x, mDim.y, 0, GL_RGB, GL_BYTE, NULL);
			break;
		case FBTextureFormat::RGBA_S8:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8_SNORM, mDim.x, mDim.y, 0, GL_RGBA, GL_BYTE, NULL);
			break;
		
		case FBTextureFormat::R_S16:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R16_SNORM, mDim.x, mDim.y, 0, GL_RED, GL_SHORT, NULL);
			break;
		case FBTextureFormat::RG_S16:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16_SNORM, mDim.x, mDim.y, 0, GL_RG, GL_SHORT, NULL);
			break;
		case FBTextureFormat::RGB_S16:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16_SNORM, mDim.x, mDim.y, 0, GL_RGB, GL_SHORT, NULL);
			break;
		case FBTextureFormat::RGBA_S16:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16_SNORM, mDim.x, mDim.y, 0, GL_RGBA, GL_SHORT, NULL);
			break;

		case FBTextureFormat::R_INT_U8:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R8UI, mDim.x, mDim.y, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, NULL);
			break;
		case FBTextureFormat::RG_INT_U8:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RG8UI, mDim.x, mDim.y, 0, GL_RG_INTEGER, GL_UNSIGNED_BYTE, NULL);
			break;
		case FBTextureFormat::RGB_INT_U8:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8UI, mDim.x, mDim.y, 0, GL_RGB_INTEGER, GL_UNSIGNED_BYTE, NULL);
			break;
		case FBTextureFormat::RGBA_INT_U8:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8UI, mDim.x, mDim.y, 0, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE, NULL);
			break;
		
		case FBTextureFormat::R_INT_U16:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R16UI, mDim.x, mDim.y, 0, GL_RED_INTEGER, GL_UNSIGNED_SHORT, NULL);
			break;
		case FBTextureFormat::RG_INT_U16:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16UI, mDim.x, mDim.y, 0, GL_RG_INTEGER, GL_UNSIGNED_SHORT, NULL);
			break;
		case FBTextureFormat::RGB_INT_U16:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16UI, mDim.x, mDim.y, 0, GL_RGB_INTEGER, GL_UNSIGNED_SHORT, NULL);
			break;
		case FBTextureFormat::RGBA_INT_U16:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16UI, mDim.x, mDim.y, 0, GL_RGBA_INTEGER, GL_UNSIGNED_SHORT, NULL);
			break;

		case FBTextureFormat::R_INT_S8:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R8I, mDim.x, mDim.y, 0, GL_RED_INTEGER, GL_BYTE, NULL);
			break;
		case FBTextureFormat::RG_INT_S8:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RG8I, mDim.x, mDim.y, 0, GL_RG_INTEGER, GL_BYTE, NULL);
			break;
		case FBTextureFormat::RGB_INT_S8:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8I, mDim.x, mDim.y, 0, GL_RGB_INTEGER, GL_BYTE, NULL);
			break;
		case FBTextureFormat::RGBA_INT_S8:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8I, mDim.x, mDim.y, 0, GL_RGBA_INTEGER, GL_BYTE, NULL);
			break;

		case FBTextureFormat::R_INT_S16:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R16I, mDim.x, mDim.y, 0, GL_RED_INTEGER, GL_SHORT, NULL);
			break;
		case FBTextureFormat::RG_INT_S16:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16I, mDim.x, mDim.y, 0, GL_RG_INTEGER, GL_SHORT, NULL);
			break;
		case FBTextureFormat::RGB_INT_S16:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16I, mDim.x, mDim.y, 0, GL_RGB_INTEGER, GL_SHORT, NULL);
			break;
		case FBTextureFormat::RGBA_INT_S16:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16I, mDim.x, mDim.y, 0, GL_RGBA_INTEGER, GL_SHORT, NULL);
			break;

		case FBTextureFormat::R_F32:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, mDim.x, mDim.y, 0, GL_RED, GL_FLOAT, NULL);
			break;
		case FBTextureFormat::RG_F32:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, mDim.x, mDim.y, 0, GL_RG, GL_FLOAT, NULL);
			break;
		case FBTextureFormat::RGB_F32:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, mDim.x, mDim.y, 0, GL_RGB, GL_FLOAT, NULL);
			break;
		case FBTextureFormat::RGBA_F32:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, mDim.x, mDim.y, 0, GL_RGBA, GL_FLOAT, NULL);
			break;
		
		case FBTextureFormat::R_F16:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, mDim.x, mDim.y, 0, GL_RED, GL_FLOAT, NULL);
			break;
		case FBTextureFormat::RG_F16:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, mDim.x, mDim.y, 0, GL_RG, GL_FLOAT, NULL);
			break;
		case FBTextureFormat::RGB_F16:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, mDim.x, mDim.y, 0, GL_RGB, GL_FLOAT, NULL);
			break;
		case FBTextureFormat::RGBA_F16:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, mDim.x, mDim.y, 0, GL_RGBA, GL_FLOAT, NULL);
			break;
		}

		switch (mTextureFiltering[i]) {
		case FBTextureFiltering::NEAREST:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			break;
		case FBTextureFiltering::LINEAR:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			break;
		}
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, tmp.mTextures[i], 0);
	}

	// Depth buffer
	if (mCreateDepthBuffer) {
		glGenRenderbuffers(1, &tmp.mDepthBuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, tmp.mDepthBuffer);
		switch (mDepthFormat) {
		case FBDepthFormat::F16:
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, mDim.x, mDim.y);
			break;
		case FBDepthFormat::F24:
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mDim.x, mDim.y);
			break;
		case FBDepthFormat::F32:
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, mDim.x, mDim.y);
			break;
		}
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, tmp.mDepthBuffer);
	}

	// Depth texture
	if (mCreateDepthTexture) {
		glGenTextures(1, &tmp.mDepthTexture);
		glBindTexture(GL_TEXTURE_2D, tmp.mDepthTexture);
		switch (mDepthFormat) {
		case FBDepthFormat::F16:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, mDim.x, mDim.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
			break;
		case FBDepthFormat::F24:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, mDim.x, mDim.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
			break;
		case FBDepthFormat::F32:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, mDim.x, mDim.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
			break;
		}
		switch (mDepthTextureFiltering) {
		case FBTextureFiltering::NEAREST:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			break;
		case FBTextureFiltering::LINEAR:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			break;
		}
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tmp.mDepthTexture, 0);
	}

	// Stencil buffer
	if (mCreateStencilBuffer) {
		glGenRenderbuffers(1, &tmp.mStencilBuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, tmp.mStencilBuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, mDim.x, mDim.y);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, tmp.mStencilBuffer);
	}

	// Stencil texture
	if (mCreateStencilTexture) {
		glGenTextures(1, &tmp.mStencilTexture);
		glBindTexture(GL_TEXTURE_2D, tmp.mStencilTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_STENCIL_INDEX8, mDim.x, mDim.y, 0, GL_STENCIL_INDEX, GL_FLOAT, NULL);
		switch (mStencilTextureFiltering) {
		case FBTextureFiltering::NEAREST:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			break;
		case FBTextureFiltering::LINEAR:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			break;
		}
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, tmp.mStencilTexture, 0);
	}

	// Sets up the textures to draw to
	GLenum drawBuffers[8];
	for (uint32_t i = 0; i < 8; ++i) {
		drawBuffers[i] = GL_COLOR_ATTACHMENT0 + i;
	}
	glDrawBuffers((GLsizei)numTextures, drawBuffers);

	// Check that framebuffer is okay
	bool status = checkCurrentFramebufferStatus();
	sfz_assert_debug(status);

	// Cleanup
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	return std::move(tmp);
}

// Shadow Map Framebuffer builder function
// ------------------------------------------------------------------------------------------------

Framebuffer createShadowMap(vec2i dimensions, FBDepthFormat depthFormat, bool pcf, vec4 borderColor) noexcept
{
	sfz_assert_debug(dimensions.x > 0);
	sfz_assert_debug(dimensions.y > 0);

	Framebuffer tmp;
	tmp.mDim = dimensions;

	// Generate framebuffer
	glGenFramebuffers(1, &tmp.mFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, tmp.mFBO);

	// Generates depth texture
	glGenTextures(1, &tmp.mDepthTexture);
	glBindTexture(GL_TEXTURE_2D, tmp.mDepthTexture);
	switch (depthFormat) {
	case FBDepthFormat::F16:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, tmp.mDim.x, tmp.mDim.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		break;
	case FBDepthFormat::F24:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, tmp.mDim.x, tmp.mDim.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		break;
	case FBDepthFormat::F32:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, tmp.mDim.x, tmp.mDim.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		break;
	}

	// Set shadowmap texture min & mag filters (enable/disable pcf)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, pcf ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, pcf ? GL_LINEAR : GL_NEAREST);

	// Set texture wrap mode to CLAMP_TO_BORDER and set border color.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor.data());

	// Enable hardware shadow maps (becomes sampler2Dshadow)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);

	// Bind texture to framebuffer
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tmp.mDepthTexture, 0);
	glDrawBuffer(GL_NONE); // No color buffer
	glReadBuffer(GL_NONE);

	// Check that framebuffer is okay
	bool status = checkCurrentFramebufferStatus();
	sfz_assert_debug(status);

	// Cleanup
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	return std::move(tmp);
}

// Framebuffer: Constructors & destructors
// ------------------------------------------------------------------------------------------------

Framebuffer::Framebuffer(Framebuffer&& other) noexcept
{
	for (uint32_t i = 0; i < 8; ++i) {
		std::swap(this->mTextures[i], other.mTextures[i]);
	}
	std::swap(this->mDepthBuffer, other.mDepthBuffer);
	std::swap(this->mDepthTexture, other.mDepthTexture);
	std::swap(this->mStencilBuffer, other.mStencilBuffer);
	std::swap(this->mStencilTexture, other.mStencilTexture);
	std::swap(this->mFBO, other.mFBO);
	std::swap(this->mDim, other.mDim);
}

Framebuffer& Framebuffer::operator= (Framebuffer&& other) noexcept
{
	for (uint32_t i = 0; i < 8; ++i) {
		std::swap(this->mTextures[i], other.mTextures[i]);
	}
	std::swap(this->mDepthBuffer, other.mDepthBuffer);
	std::swap(this->mDepthTexture, other.mDepthTexture);
	std::swap(this->mStencilBuffer, other.mStencilBuffer);
	std::swap(this->mStencilTexture, other.mStencilTexture);
	std::swap(this->mFBO, other.mFBO);
	std::swap(this->mDim, other.mDim);
	return *this;
}

Framebuffer::~Framebuffer() noexcept
{
	glDeleteTextures(8, mTextures);
	glDeleteRenderbuffers(1, &mDepthBuffer);
	glDeleteTextures(1, &mDepthTexture);
	glDeleteRenderbuffers(1, &mStencilBuffer);
	glDeleteTextures(1, &mStencilTexture);
	glDeleteFramebuffers(1, &mFBO);
}

// Framebuffer: Getters
// ------------------------------------------------------------------------------------------------

uint32_t Framebuffer::fbo() const noexcept
{
	sfz_assert_debug(mFBO != 0);
	return mFBO;
}

uint32_t Framebuffer::texture(uint32_t index) const noexcept
{
	sfz_assert_debug(index < 8);
	sfz_assert_debug(mTextures[index] != 0);
	return mTextures[index];
}

uint32_t Framebuffer::depthBuffer() const noexcept
{
	sfz_assert_debug(mDepthBuffer != 0);
	return mDepthBuffer;
}

uint32_t Framebuffer::depthTexture() const noexcept
{
	sfz_assert_debug(mDepthTexture != 0);
	return mDepthTexture;
}

uint32_t Framebuffer::stencilBuffer() const noexcept
{
	sfz_assert_debug(mStencilBuffer != 0);
	return mStencilBuffer;
}

uint32_t Framebuffer::stencilTexture() const noexcept
{
	sfz_assert_debug(mStencilTexture != 0);
	return mStencilTexture;
}

// Framebuffer: Public methods
// ------------------------------------------------------------------------------------------------

void Framebuffer::bind() noexcept
{
	if (!this->isValid()) return;
	glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
}

void Framebuffer::bindViewport() noexcept
{
	if (!this->isValid()) return;
	glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
	glViewport(0, 0, mDim.x, mDim.y);
}

void Framebuffer::bindViewport(vec2i viewportMin, vec2i viewportMax) noexcept
{
	if (!this->isValid()) return;
	glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
	glViewport(viewportMin.x, viewportMin.y, viewportMax.x, viewportMax.y);
}

void Framebuffer::bindViewportClearColor(vec4 clearColor) noexcept
{
	if (!this->isValid()) return;
	glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
	glViewport(0, 0, mDim.x, mDim.y);
	glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
	glClear(GL_COLOR_BUFFER_BIT);
}

void Framebuffer::bindViewportClearColor(vec2i viewportMin, vec2i viewportMax,
                                         vec4 clearColor) noexcept
{
	if (!this->isValid()) return;
	glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
	glViewport(viewportMin.x, viewportMin.y, viewportMax.x, viewportMax.y);
	glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
	glClear(GL_COLOR_BUFFER_BIT);
}

void Framebuffer::bindViewportClearColorDepth(vec4 clearColor, float clearDepth) noexcept
{
	if (!this->isValid()) return;
	glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
	glViewport(0, 0, mDim.x, mDim.y);
	glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
	glClearDepth(clearDepth);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Framebuffer::bindViewportClearColorDepth(vec2i viewportMin, vec2i viewportMax,
                                              vec4 clearColor, float clearDepth) noexcept
{
	if (!this->isValid()) return;
	glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
	glViewport(viewportMin.x, viewportMin.y, viewportMax.x, viewportMax.y);
	glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
	glClearDepth(clearDepth);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

// Framebuffer: Attach/detach component methods
// ------------------------------------------------------------------------------------------------

void Framebuffer::attachExternalDepthBuffer(uint32_t buffer) noexcept
{
	sfz_assert_debug(mDepthBuffer == 0);
	sfz_assert_debug(mDepthTexture == 0);
	glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, buffer);
	bool status = checkCurrentFramebufferStatus();
	sfz_assert_debug(status);
}

void Framebuffer::attachExternalDepthTexture(uint32_t texture) noexcept
{
	sfz_assert_debug(mDepthBuffer == 0);
	sfz_assert_debug(mDepthTexture == 0);
	glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture, 0);
	bool status = checkCurrentFramebufferStatus();
	sfz_assert_debug(status);
}

void Framebuffer::attachExternalStencilBuffer(uint32_t buffer) noexcept
{
	sfz_assert_debug(mStencilBuffer == 0);
	sfz_assert_debug(mStencilTexture == 0);
	glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, buffer);
	bool status = checkCurrentFramebufferStatus();
	sfz_assert_debug(status);
}

void Framebuffer::attachExternalStencilTexture(uint32_t texture) noexcept
{
	sfz_assert_debug(mStencilBuffer == 0);
	sfz_assert_debug(mStencilTexture == 0);
	glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, texture, 0);
	bool status = checkCurrentFramebufferStatus();
	sfz_assert_debug(status);
}

// Framebuffer helper functions
// ------------------------------------------------------------------------------------------------

bool checkCurrentFramebufferStatus() noexcept
{
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		if (status == GL_FRAMEBUFFER_UNDEFINED) {
			std::fprintf(stderr, "GL_FRAMEBUFFER_UNDEFINED is returned if target is the default framebuffer, but the default framebuffer does not exist.\n");
		} else if (status == GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT) {
			std::fprintf(stderr, "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT is returned if any of the framebuffer attachment points are framebuffer incomplete.\n");
		} else if (status == GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT) {
			std::fprintf(stderr, "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT is returned if the framebuffer does not have at least one image attached to it.\n");
		} else if (status == GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER) {
			std::fprintf(stderr, "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER is returned if the value of GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE is GL_NONE for any color attachment point(s) named by GL_DRAW_BUFFERi.\n");
		} else if (status == GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER) {
			std::fprintf(stderr, "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER is returned if GL_READ_BUFFER is not GL_NONE and the value of GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE is GL_NONE for the color attachment point named by GL_READ_BUFFER.\n");
		} else if (status == GL_FRAMEBUFFER_UNSUPPORTED) {
			std::fprintf(stderr, "GL_FRAMEBUFFER_UNSUPPORTED is returned if the combination of internal formats of the attached images violates an implementation-dependent set of restrictions.\n");
		} else if (status == GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE) {
			std::fprintf(stderr, "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE is returned if the value of GL_RENDERBUFFER_SAMPLES is not the same for all attached renderbuffers; if the value of GL_TEXTURE_SAMPLES is the not same for all attached textures; or, if the attached images are a mix of renderbuffers and textures, the value of GL_RENDERBUFFER_SAMPLES does not match the value of GL_TEXTURE_SAMPLES. GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE is also returned if the value of GL_TEXTURE_FIXED_SAMPLE_LOCATIONS is not the same for all attached textures; or, if the attached images are a mix of renderbuffers and textures, the value of GL_TEXTURE_FIXED_SAMPLE_LOCATIONS is not GL_TRUE for all attached textures.\n");
		} else if (status == GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS) {
			std::fprintf(stderr, "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS is returned if any framebuffer attachment is layered, and any populated attachment is not layered, or if all populated color attachments are not from textures of the same target.\n");
		}
		return false;
	}
	return true;
}

} // namespace gl
} // namespace sfz
