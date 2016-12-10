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

#include "sfz/Assert.hpp"
#include "sfz/math/Vector.hpp"

namespace sfz {

namespace gl {

using std::int32_t;
using std::uint32_t;

class Framebuffer; // Forward declare Framebuffer class

// FB enums
// ------------------------------------------------------------------------------------------------

enum class FBTextureFormat : uint32_t {
	// Unsigned normalized 8-bit int, maps to range [0, 1]
	R_U8,
	RG_U8,
	RGB_U8,
	RGBA_U8,

	// Unsigned normalized 16-bit int, maps to range [0, 1]
	R_U16,
	RG_U16,
	RGB_U16,
	RGBA_U16,

	// Signed normalized 8-bit int, maps to range [-1, 1]
	R_S8,
	RG_S8,
	RGB_S8,
	RGBA_S8,

	// Signed normalized 16-bit int, maps to range [-1, 1]
	R_S16,
	RG_S16,
	RGB_S16,
	RGBA_S16,
	
	// Unsigned non-normalized 8-bit int, maps to normal unsigned integer range [0, 255]
	R_INT_U8,
	RG_INT_U8,
	RGB_INT_U8,
	RGBA_INT_U8,

	// Unsigned non-normalized 16-bit int, maps to normal unsigned integer range [0, 65535]
	R_INT_U16,
	RG_INT_U16,
	RGB_INT_U16,
	RGBA_INT_U16,

	// Signed non-normalized 8-bit int, maps to normal signed integer range [-128, 127]
	R_INT_S8,
	RG_INT_S8,
	RGB_INT_S8,
	RGBA_INT_S8,

	// Signed non-normalized 16-bit int, maps to normal signed integer range [-32768, 32767]
	R_INT_S16,
	RG_INT_S16,
	RGB_INT_S16,
	RGBA_INT_S16,

	// 32-bit float, maps to normal 32-bit float range
	R_F32,
	RG_F32,
	RGB_F32,
	RGBA_F32,
	
	// 16-bit float, maps to normal 16-bit float range
	R_F16,
	RG_F16,
	RGB_F16,
	RGBA_F16
};

enum class FBDepthFormat : uint32_t {
	F16,
	F24,
	F32
};

enum class FBTextureFiltering : uint32_t {
	NEAREST,
	LINEAR
};

// Framebuffer Builder class
// ------------------------------------------------------------------------------------------------

/// Builder class used to build Framebuffer instances.
///
/// You may add up to 8 textures and stencil and depth attachments. The stencil and depth
/// attachments may be either a texture or buffer (RenderBuffer), the difference is that you are
/// not allowed to read from a RenderBuffer in OpenGL.
///
/// The texture index specifies which position the texture will be attached to. I.e., index 0
/// will attach to GL_COLOR_ATTACHMENT0, index 1 to GL_COLOR_ATTACHMENT1, etc.
///
/// The Framebuffer class allows for up to 8 textures and the draw order of the texture attachments
/// is always GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, ..., GL_COLOR_ATTACHMENT_7. Additionally
/// there may not be any "holes" between the indices, i.e. if a texture is added to index 2 there
/// must also be textures attached to index 0 and 1.
class FramebufferBuilder final {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	FramebufferBuilder() noexcept = default;
	FramebufferBuilder(const FramebufferBuilder&) noexcept = default;
	FramebufferBuilder& operator= (const FramebufferBuilder&) noexcept = default;
	~FramebufferBuilder() noexcept = default;

	FramebufferBuilder(vec2i dimensions) noexcept;

	// Component adding methods
	// --------------------------------------------------------------------------------------------

	FramebufferBuilder& setDimensions(vec2i dimensions) noexcept;
	FramebufferBuilder& addTexture(uint32_t index, FBTextureFormat format, FBTextureFiltering filtering) noexcept;
	FramebufferBuilder& addDepthBuffer(FBDepthFormat format) noexcept;
	FramebufferBuilder& addDepthTexture(FBDepthFormat format, FBTextureFiltering filtering) noexcept;
	FramebufferBuilder& addStencilBuffer() noexcept;
	FramebufferBuilder& addStencilTexture(FBTextureFiltering filtering) noexcept;

	// Component removing methods
	// --------------------------------------------------------------------------------------------

	FramebufferBuilder& removeTexture(uint32_t index) noexcept;
	FramebufferBuilder& removeDepthBuffer() noexcept;
	FramebufferBuilder& removeDepthTexture() noexcept;
	FramebufferBuilder& removeStencilBuffer() noexcept;
	FramebufferBuilder& removeStencilTexture() noexcept;

	// Framebuffer building method
	// --------------------------------------------------------------------------------------------

	Framebuffer build() const noexcept;

private:
	// Private members
	// --------------------------------------------------------------------------------------------

	bool mCreateTexture[8] = { false, false, false, false, false, false, false, false };
	bool mCreateDepthBuffer = false;
	bool mCreateDepthTexture = false;
	bool mCreateStencilBuffer = false;
	bool mCreateStencilTexture = false;
	FBTextureFormat mTextureFormat[8];
	FBDepthFormat mDepthFormat;
	FBTextureFiltering mTextureFiltering[8], mDepthTextureFiltering, mStencilTextureFiltering;
	vec2i mDim = vec2i(-1);
};

// Shadow Map Framebuffer builder function
// ------------------------------------------------------------------------------------------------

/// Creates a Shadow Map
///
/// In short creates a Framebuffer with only one component, a depth texture. The depth texture
/// will have "GL_TEXTURE_COMPARE_FUNC" set to "GL_LEQUAL" and "GL_TEXTURE_COMPARE_MODE" set to 
/// "GL_COMPARE_REF_TO_TEXTURE", making it possible to bind it as a "sampler2dShadow" in GLSL 
/// shaders. The pcf parameter decides whether to set the filtering mode to nearest or linear,
/// linear in combination with the previously mentioned parameters will turn on hardware 
/// Percentage Closer Filtering (likely 2x2 samples).
///
/// The wrapping mode used is "GL_CLAMP_TO_BORDER" where the border is the specified border
/// parameter.
Framebuffer createShadowMap(vec2i dimensions, FBDepthFormat depthFormat, bool pcf = true,
                            vec4 borderColor = vec4(0.0f, 0.0f, 0.0f, 1.0f)) noexcept;

// Framebuffer class
// ------------------------------------------------------------------------------------------------

class Framebuffer final {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	Framebuffer() noexcept = default;
	Framebuffer(const Framebuffer&) = delete;
	Framebuffer& operator= (const Framebuffer&) = delete;

	Framebuffer(Framebuffer&& other) noexcept;
	Framebuffer& operator= (Framebuffer&& other) noexcept;
	~Framebuffer() noexcept;

	friend class FramebufferBuilder;
	friend Framebuffer createShadowMap(vec2i, FBDepthFormat, bool, vec4) noexcept;

	// State checking
	// --------------------------------------------------------------------------------------------

	inline bool isValid() const noexcept { return mFBO != 0; }
	inline bool hasTexture(uint32_t index) const noexcept { sfz_assert_debug(index < 8); return mTextures[index] != 0; } 
	inline bool hasDepthBuffer() const noexcept { return mDepthBuffer != 0; }
	inline bool hasDepthTexture() const noexcept { return mDepthTexture != 0; }
	inline bool hasStencilBuffer() const noexcept { return mStencilBuffer != 0; }
	inline bool hasStencilTexture() const noexcept { return mStencilTexture != 0; }

	// Getters
	// --------------------------------------------------------------------------------------------

	uint32_t fbo() const noexcept;
	uint32_t texture(uint32_t index) const noexcept;
	uint32_t depthBuffer() const noexcept;
	uint32_t depthTexture() const noexcept;
	uint32_t stencilBuffer() const noexcept;
	uint32_t stencilTexture() const noexcept;
	inline vec2i dimensions() const noexcept { return mDim; }
	inline int32_t width() const noexcept { return mDim.x; };
	inline int32_t height() const noexcept { return mDim.y; }
	inline vec2 dimensionsFloat() const noexcept { return vec2{(float)mDim.x, (float)mDim.y}; }
	inline float widthFloat() const noexcept { return (float)mDim.x; }
	inline float heightFloat() const noexcept { return (float)mDim.y; }

	// Public methods
	// --------------------------------------------------------------------------------------------

	/// Simple wrappers around glBindFramebuffer(), glViewport(), glClearColor(), glClearDepth()
	/// and glClear(). If the fbo is not valid (mFBO == 0) these functions will not do anything.

	void bind() noexcept;
	void bindViewport() noexcept;
	void bindViewport(vec2i viewportMin, vec2i viewportMax) noexcept;
	void bindViewportClearColor(vec4 clearColor = vec4(0.0)) noexcept;
	void bindViewportClearColor(vec2i viewportMin, vec2i viewportMax,
	                            vec4 clearColor = vec4(0.0)) noexcept;
	void bindViewportClearColorDepth(vec4 clearColor = vec4(0.0), float clearDepth = 1.0f) noexcept;
	void bindViewportClearColorDepth(vec2i viewportMin, vec2i viewportMax,
	                                 vec4 clearColor = vec4(0.0), float clearDepth = 1.0f) noexcept;

	// Attaching external depth/stencil buffers/textures
	// --------------------------------------------------------------------------------------------

	
	/// The following methods attaches an external texture/buffer to the fbo. It is up to the
	/// user to make sure that external texture/buffer is alive for as long as it is attached
	/// to the fbo.

	void attachExternalDepthBuffer(uint32_t buffer) noexcept;
	void attachExternalDepthTexture(uint32_t texture) noexcept;
	void attachExternalStencilBuffer(uint32_t buffer) noexcept;
	void attachExternalStencilTexture(uint32_t texture) noexcept;

private:
	// Private members
	// --------------------------------------------------------------------------------------------

	uint32_t mFBO = 0;
	uint32_t mTextures[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	uint32_t mDepthBuffer = 0;
	uint32_t mDepthTexture = 0;
	uint32_t mStencilBuffer = 0;
	uint32_t mStencilTexture = 0;
	vec2i mDim{-1};
};

// Framebuffer helper functions
// ------------------------------------------------------------------------------------------------

/// Checks the status of the currently bound framebuffer
/// If the status is not GL_FRAMEBUFFER_COMPLETE the error message is printed out into stderr.
/// \return whether the framebuffer was complete or not
bool checkCurrentFramebufferStatus() noexcept;

} // namespace gl
} // namespace sfz
