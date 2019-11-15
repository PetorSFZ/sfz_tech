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

#include <algorithm>

#include <sfz/Assert.hpp>
#include <sfz/Logging.hpp>

#include "sfz/gl/IncludeOpenGL.hpp"

namespace sfz {

namespace gl {

// Framebuffer: State methods
// ------------------------------------------------------------------------------------------------

void Framebuffer::swap(Framebuffer& other) noexcept
{
	std::swap(this->fbo, other.fbo);
	for (uint32_t i = 0; i < 8; i++) {
		std::swap(this->textures[i], other.textures[i]);
	}
	std::swap(this->depthBuffer, other.depthBuffer);
	std::swap(this->depthTexture, other.depthTexture);
	std::swap(this->stencilBuffer, other.stencilBuffer);
	std::swap(this->stencilTexture, other.stencilTexture);
	std::swap(this->width, other.width);
	std::swap(this->height, other.height);
}

void Framebuffer::destroy() noexcept
{
	glDeleteTextures(8, textures);
	glDeleteRenderbuffers(1, &depthBuffer);
	glDeleteTextures(1, &depthTexture);
	glDeleteRenderbuffers(1, &stencilBuffer);
	glDeleteTextures(1, &stencilTexture);
	glDeleteFramebuffers(1, &fbo);

	// Clear members
	fbo = 0;
	for (uint32_t i = 0; i < 8; i++) {
		textures[i] = 0;
	}
	depthBuffer = 0;
	depthTexture = 0;
	stencilBuffer = 0;
	stencilTexture = 0;
	width = 0;
	height = 0;
}

// Framebuffer: Methods
// ------------------------------------------------------------------------------------------------

void Framebuffer::bind() noexcept
{
	if (!this->isValid()) return;
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
}

void Framebuffer::bindViewport() noexcept
{
	if (!this->isValid()) return;
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glViewport(0, 0, width, height);
}

void Framebuffer::bindViewport(vec2_i32 viewportMin, vec2_i32 viewportMax) noexcept
{
	if (!this->isValid()) return;
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glViewport(viewportMin.x, viewportMin.y, viewportMax.x, viewportMax.y);
}

void Framebuffer::bindViewportClearColor(vec4 clearColor) noexcept
{
	if (!this->isValid()) return;
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glViewport(0, 0, width, height);
	glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
	glClear(GL_COLOR_BUFFER_BIT);
}

void Framebuffer::bindViewportClearColor(vec2_i32 viewportMin, vec2_i32 viewportMax,
                                         vec4 clearColor) noexcept
{
	if (!this->isValid()) return;
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glViewport(viewportMin.x, viewportMin.y, viewportMax.x, viewportMax.y);
	glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
	glClear(GL_COLOR_BUFFER_BIT);
}

void Framebuffer::bindViewportClearColorDepth(vec4 clearColor, float clearDepth) noexcept
{
	if (!this->isValid()) return;
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glViewport(0, 0, width, height);
	glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
#if defined(__EMSCRIPTEN__) || defined(SFZ_IOS)
	glClearDepthf(clearDepth);
#else
	glClearDepth(clearDepth);
#endif
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Framebuffer::bindViewportClearColorDepth(vec2_i32 viewportMin, vec2_i32 viewportMax,
                                              vec4 clearColor, float clearDepth) noexcept
{
	if (!this->isValid()) return;
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glViewport(viewportMin.x, viewportMin.y, viewportMax.x, viewportMax.y);
	glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
#if defined(__EMSCRIPTEN__) || defined(SFZ_IOS)
	glClearDepthf(clearDepth);
#else
	glClearDepth(clearDepth);
#endif
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

// Framebuffer: Attaching external depth/stencil buffers/textures
// --------------------------------------------------------------------------------------------

void Framebuffer::attachExternalDepthBuffer(uint32_t buffer) noexcept
{
	sfz_assert(depthBuffer == 0);
	sfz_assert(depthTexture == 0);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, buffer);
	bool status = checkCurrentFramebufferStatus();
	sfz_assert(status);
}

void Framebuffer::attachExternalDepthTexture(uint32_t texture) noexcept
{
	sfz_assert(depthBuffer == 0);
	sfz_assert(depthTexture == 0);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture, 0);
	bool status = checkCurrentFramebufferStatus();
	sfz_assert(status);
}

void Framebuffer::attachExternalStencilBuffer(uint32_t buffer) noexcept
{
	sfz_assert(stencilBuffer == 0);
	sfz_assert(stencilTexture == 0);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, buffer);
	bool status = checkCurrentFramebufferStatus();
	sfz_assert(status);
}

void Framebuffer::attachExternalStencilTexture(uint32_t texture) noexcept
{
	sfz_assert(stencilBuffer == 0);
	sfz_assert(stencilTexture == 0);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, texture, 0);
	bool status = checkCurrentFramebufferStatus();
	sfz_assert(status);
}

// Framebuffer helper functions
// ------------------------------------------------------------------------------------------------

bool checkCurrentFramebufferStatus() noexcept
{
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

#if defined(__EMSCRIPTEN__) || defined(SFZ_IOS)
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		switch(status) {
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			SFZ_ERROR("sfzGL", "%s", "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT is returned if any of the framebuffer attachment points are framebuffer incomplete.");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			SFZ_ERROR("sfzGL", "%s", "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT is returned if the framebuffer does not have at least one image attached to it.");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
			SFZ_ERROR("sfzGL", "%s", "GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS is returned if attachments do not have the same width and height");
			break;
		/*case GL_FRAMEBUFFER_INCOMPLETE_FORMATS:
			SFZ_ERROR("sfzGL", "%s", "GL_FRAMEBUFFER_INCOMPLETE_FORMATS is returned if the internal formats of the attachments are not renderable");
			break;*/
		case GL_FRAMEBUFFER_UNSUPPORTED:
			SFZ_ERROR("sfzGL", "%s", "GL_FRAMEBUFFER_UNSUPPORTED is returned if combination of internal formats of attachments results in a nonrenderable target");
			break;
		}
		return false;
	}

#else
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		switch(status) {
		case GL_FRAMEBUFFER_UNDEFINED:
			SFZ_ERROR("sfzGL", "%s", "GL_FRAMEBUFFER_UNDEFINED is returned if target is the default framebuffer, but the default framebuffer does not exist.");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			SFZ_ERROR("sfzGL", "%s", "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT is returned if any of the framebuffer attachment points are framebuffer incomplete.");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			SFZ_ERROR("sfzGL", "%s", "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT is returned if the framebuffer does not have at least one image attached to it.");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
			SFZ_ERROR("sfzGL", "%s", "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER is returned if the value of GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE is GL_NONE for any color attachment point(s) named by GL_DRAW_BUFFERi.");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
			SFZ_ERROR("sfzGL", "%s", "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER is returned if GL_READ_BUFFER is not GL_NONE and the value of GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE is GL_NONE for the color attachment point named by GL_READ_BUFFER.");
			break;
		case GL_FRAMEBUFFER_UNSUPPORTED:
			SFZ_ERROR("sfzGL", "%s", "GL_FRAMEBUFFER_UNSUPPORTED is returned if the combination of internal formats of the attached images violates an implementation-dependent set of restrictions.");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
			SFZ_ERROR("sfzGL", "%s", "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE is returned if the value of GL_RENDERBUFFER_SAMPLES is not the same for all attached renderbuffers; if the value of GL_TEXTURE_SAMPLES is the not same for all attached textures; or, if the attached images are a mix of renderbuffers and textures, the value of GL_RENDERBUFFER_SAMPLES does not match the value of GL_TEXTURE_SAMPLES. GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE is also returned if the value of GL_TEXTURE_FIXED_SAMPLE_LOCATIONS is not the same for all attached textures; or, if the attached images are a mix of renderbuffers and textures, the value of GL_TEXTURE_FIXED_SAMPLE_LOCATIONS is not GL_TRUE for all attached textures.");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
			SFZ_ERROR("sfzGL", "%s", "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS is returned if any framebuffer attachment is layered, and any populated attachment is not layered, or if all populated color attachments are not from textures of the same target.");
			break;
		}
		return false;
	}

#endif
	return true;
}

// FramebufferBuilder: Constructors & destructors
// ------------------------------------------------------------------------------------------------

FramebufferBuilder::FramebufferBuilder(vec2_i32 dimensions) noexcept
{
	setDimensions(dimensions);
}

// FramebufferBuilder: Component adding methods
// ------------------------------------------------------------------------------------------------

FramebufferBuilder& FramebufferBuilder::setDimensions(vec2_i32 dimensions) noexcept
{
	sfz_assert(dimensions.x > 0);
	sfz_assert(dimensions.y > 0);
	mDim = dimensions;
	return *this;
}

FramebufferBuilder& FramebufferBuilder::addTexture(uint32_t index, FBTextureFormat format, FBTextureFiltering filtering) noexcept
{
	sfz_assert(index < 8);
	sfz_assert(!mCreateTexture[index]);
	mCreateTexture[index] = true;
	mTextureFormat[index] = format;
	mTextureFiltering[index] = filtering;
	return *this;
}

FramebufferBuilder& FramebufferBuilder::addDepthBuffer(FBDepthFormat format) noexcept
{
	sfz_assert(!mCreateDepthBuffer);
	sfz_assert(!mCreateDepthTexture);
	mCreateDepthBuffer = true;
	mDepthFormat = format;
	return *this;
}

FramebufferBuilder& FramebufferBuilder::addDepthTexture(FBDepthFormat format, FBTextureFiltering filtering) noexcept
{
	sfz_assert(!mCreateDepthBuffer);
	sfz_assert(!mCreateDepthTexture);
	mCreateDepthTexture = true;
	mDepthFormat = format;
	mDepthTextureFiltering = filtering;
	return *this;
}

FramebufferBuilder& FramebufferBuilder::addStencilBuffer() noexcept
{
	sfz_assert(!mCreateStencilBuffer);
	sfz_assert(!mCreateStencilTexture);
	mCreateStencilBuffer = true;
	return *this;
}

FramebufferBuilder& FramebufferBuilder::addStencilTexture(FBTextureFiltering filtering) noexcept
{
	sfz_assert(!mCreateStencilBuffer);
	sfz_assert(!mCreateStencilTexture);
	mCreateStencilTexture = true;
	mStencilTextureFiltering = filtering;
	return *this;
}

// FramebufferBuilder: Component removing methods
// ------------------------------------------------------------------------------------------------

FramebufferBuilder& FramebufferBuilder::removeTexture(uint32_t index) noexcept
{
	sfz_assert(index < 8);
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
	sfz_assert(mDim.x > 0);
	sfz_assert(mDim.y > 0);
	sfz_assert(!(mCreateDepthBuffer && mCreateDepthTexture));
	sfz_assert(!(mCreateStencilBuffer && mCreateStencilTexture));
	for (uint32_t i = 0; i < 7; ++i) {
		if (mCreateTexture[i+1]) {
			sfz_assert(mCreateTexture[i]);
		}
	}

	Framebuffer tmp;
	tmp.width = mDim.x;
	tmp.height = mDim.y;

	// Generate framebuffer
	glGenFramebuffers(1, &tmp.fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, tmp.fbo);

	// Generate textures
	glActiveTexture(GL_TEXTURE0); // TODO: Not sure if necessary
	uint32_t numTextures = 0;
	for (uint32_t i = 0; i < 8; ++i) {
		if (!mCreateTexture[i]) break;
		numTextures = i + 1;

		glGenTextures(1, &tmp.textures[i]);
		glBindTexture(GL_TEXTURE_2D, tmp.textures[i]);

		switch (mTextureFormat[i]) {

#if defined(__EMSCRIPTEN__) || defined(SFZ_IOS)
		case FBTextureFormat::R_U8:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, mDim.x, mDim.y, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
			break;
		case FBTextureFormat::RGB_U8:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, mDim.x, mDim.y, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
			break;
		case FBTextureFormat::RGBA_U8:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mDim.x, mDim.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
			break;

#else
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
#endif
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
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, tmp.textures[i], 0);
	}

	// Depth buffer
	if (mCreateDepthBuffer) {
		glGenRenderbuffers(1, &tmp.depthBuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, tmp.depthBuffer);
		switch (mDepthFormat) {
#if defined(__EMSCRIPTEN__) || defined(SFZ_IOS)
		case FBDepthFormat::F16:
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, mDim.x, mDim.y);
			break;
		case FBDepthFormat::F24:
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24_OES, mDim.x, mDim.y);
			break;
		case FBDepthFormat::F32:
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32_OES, mDim.x, mDim.y);
			break;
#else
		case FBDepthFormat::F16:
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, mDim.x, mDim.y);
			break;
		case FBDepthFormat::F24:
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mDim.x, mDim.y);
			break;
		case FBDepthFormat::F32:
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, mDim.x, mDim.y);
			break;
#endif
		}
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, tmp.depthBuffer);
	}

	// Depth texture
	if (mCreateDepthTexture) {
		glGenTextures(1, &tmp.depthTexture);
		glBindTexture(GL_TEXTURE_2D, tmp.depthTexture);
		switch (mDepthFormat) {
#if defined(__EMSCRIPTEN__) || defined(SFZ_IOS)
		case FBDepthFormat::F16:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, mDim.x, mDim.y, 0, GL_DEPTH_COMPONENT16, GL_FLOAT, NULL);
			break;
		case FBDepthFormat::F24:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24_OES, mDim.x, mDim.y, 0, GL_DEPTH_COMPONENT24_OES, GL_FLOAT, NULL);
			break;
		case FBDepthFormat::F32:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32_OES, mDim.x, mDim.y, 0, GL_DEPTH_COMPONENT32_OES, GL_FLOAT, NULL);
			break;
#else
		case FBDepthFormat::F16:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, mDim.x, mDim.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
			break;
		case FBDepthFormat::F24:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, mDim.x, mDim.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
			break;
		case FBDepthFormat::F32:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, mDim.x, mDim.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
			break;
#endif
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
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tmp.depthTexture, 0);
	}

	// Stencil buffer
	if (mCreateStencilBuffer) {
		glGenRenderbuffers(1, &tmp.stencilBuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, tmp.stencilBuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, mDim.x, mDim.y);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, tmp.stencilBuffer);
	}

	// Stencil texture
	if (mCreateStencilTexture) {
		glGenTextures(1, &tmp.stencilTexture);
		glBindTexture(GL_TEXTURE_2D, tmp.stencilTexture);
#if defined(__EMSCRIPTEN__) || defined(SFZ_IOS)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_STENCIL_INDEX8, mDim.x, mDim.y, 0, GL_STENCIL_INDEX8, GL_FLOAT, NULL);
#else
		glTexImage2D(GL_TEXTURE_2D, 0, GL_STENCIL_INDEX8, mDim.x, mDim.y, 0, GL_STENCIL_INDEX, GL_FLOAT, NULL);
#endif
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
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, tmp.stencilTexture, 0);
	}

	// Sets up the textures to draw to
#if !defined(__EMSCRIPTEN__) && !defined(SFZ_IOS)
	GLenum drawBuffers[8];
	for (uint32_t i = 0; i < 8; ++i) {
		drawBuffers[i] = GL_COLOR_ATTACHMENT0 + i;
	}
	glDrawBuffers((GLsizei)numTextures, drawBuffers);
#endif

	// Check that framebuffer is okay
	bool status = checkCurrentFramebufferStatus();
	sfz_assert(status);

	// Cleanup
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	return tmp;
}

// Shadow Map Framebuffer builder function
// ------------------------------------------------------------------------------------------------

Framebuffer createShadowMap(vec2_i32 dimensions, FBDepthFormat depthFormat, bool pcf, vec4 borderColor) noexcept
{
	sfz_assert(dimensions.x > 0);
	sfz_assert(dimensions.y > 0);

	Framebuffer tmp;
	tmp.width = dimensions.x;
	tmp.height = dimensions.y;

	// Generate framebuffer
	glGenFramebuffers(1, &tmp.fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, tmp.fbo);

	// Generates depth texture
	glGenTextures(1, &tmp.depthTexture);
	glBindTexture(GL_TEXTURE_2D, tmp.depthTexture);
	switch (depthFormat) {
#if defined(__EMSCRIPTEN__) || defined(SFZ_IOS)
	case FBDepthFormat::F16:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, tmp.width, tmp.height, 0, GL_DEPTH_COMPONENT16, GL_FLOAT, NULL);
		break;
	case FBDepthFormat::F24:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24_OES, tmp.width, tmp.height, 0, GL_DEPTH_COMPONENT24_OES, GL_FLOAT, NULL);
		break;
	case FBDepthFormat::F32:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32_OES, tmp.width, tmp.height, 0, GL_DEPTH_COMPONENT32_OES, GL_FLOAT, NULL);
		break;
#else
	case FBDepthFormat::F16:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, tmp.width, tmp.height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		break;
	case FBDepthFormat::F24:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, tmp.width, tmp.height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		break;
	case FBDepthFormat::F32:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, tmp.width, tmp.height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		break;
#endif
	}

#if defined(__EMSCRIPTEN__) || defined(SFZ_IOS)
	// WebGL 1.0 does not seem to support most things, just try to do something safe.
	(void)pcf;
	(void)borderColor;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

#else
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

#endif

	// Bind texture to framebuffer
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tmp.depthTexture, 0);
#if !defined(__EMSCRIPTEN__) && !defined(SFZ_IOS)
	glDrawBuffer(GL_NONE); // No color buffer
	glReadBuffer(GL_NONE);
#endif

	// Check that framebuffer is okay
	bool status = checkCurrentFramebufferStatus();
	sfz_assert(status);

	// Cleanup
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	return tmp;
}

} // namespace gl
} // namespace sfz
