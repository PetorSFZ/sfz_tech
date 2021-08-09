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

#include "sfz/resources/FramebufferResource.hpp"

#include "sfz/Context.hpp"
#include "sfz/config/Setting.hpp"
#include "sfz/renderer/ZeroGUtils.hpp"
#include "sfz/resources/ResourceManager.hpp"
#include "sfz/resources/TextureResource.hpp"

namespace sfz {

// FramebufferResource
// ------------------------------------------------------------------------------------------------

ZgResult FramebufferResource::build(vec2_i32 screenRes)
{
	CHECK_ZG zg::CommandQueue::getPresentQueue().flush();
	CHECK_ZG zg::CommandQueue::getCopyQueue().flush();

	sfz_assert(!renderTargetNames.isEmpty() || depthBufferName.isValid());
	ResourceManager& resources = getResourceManager();

	// Set resolution and resolution scale if screen relative
	if (screenRelativeResolution) {
		if (this->resolutionScaleSetting != nullptr) {
			this->resolutionScale = resolutionScaleSetting->floatValue();
		}
		vec2 scaledRes = vec2(screenRes) * this->resolutionScale;
		this->res.x = u32(std::round(scaledRes.x));
		this->res.y = u32(std::round(scaledRes.y));
	}
	else if (settingControlledRes) {
		this->res = vec2_i32(controlledResSetting->intValue());
	}

	zg::FramebufferBuilder fbBuilder;

	for (strID renderTargetName : renderTargetNames) {
		TextureResource* tex = resources.getTexture(resources.getTextureHandle(renderTargetName));
		sfz_assert(tex != nullptr);
		sfz_assert(tex->texture.valid());
		sfz_assert(tex->usage == ZG_TEXTURE_USAGE_RENDER_TARGET);
		if (this->res != tex->res) {
			CHECK_ZG tex->build(screenRes);
		}
		sfz_assert(this->res == tex->res);
		fbBuilder.addRenderTarget(tex->texture);
	}

	if (depthBufferName.isValid()) {
		TextureResource* tex = resources.getTexture(resources.getTextureHandle(depthBufferName));
		sfz_assert(tex != nullptr);
		sfz_assert(tex->texture.valid());
		sfz_assert(tex->usage == ZG_TEXTURE_USAGE_DEPTH_BUFFER);
		if (this->res != tex->res) {
			CHECK_ZG tex->build(screenRes);
		}
		sfz_assert(this->res == tex->res);
		fbBuilder.setDepthBuffer(tex->texture);
	}

	return fbBuilder.build(framebuffer);
}

// FramebufferResourceBuilder
// ------------------------------------------------------------------------------------------------

FramebufferResourceBuilder& FramebufferResourceBuilder::setName(const char* nameIn)
{
	this->name = str128("%s", nameIn);
	return *this;
}

FramebufferResourceBuilder& FramebufferResourceBuilder::setFixedRes(vec2_i32 resIn)
{
	sfz_assert(resIn.x > 0);
	sfz_assert(resIn.y > 0);
	this->res = resIn;
	this->screenRelativeResolution = false;
	this->resolutionScale = 1.0f;
	this->resolutionScaleSetting = nullptr;
	return *this;
}

FramebufferResourceBuilder& FramebufferResourceBuilder::setScreenRelativeRes(float scale)
{
	this->screenRelativeResolution = true;
	this->resolutionScale = scale;
	return *this;
}

FramebufferResourceBuilder& FramebufferResourceBuilder::setScreenRelativeRes(Setting* scaleSetting)
{
	this->screenRelativeResolution = true;
	this->resolutionScale = 1.0f;
	this->resolutionScaleSetting = scaleSetting;
	return *this;
}

FramebufferResourceBuilder& FramebufferResourceBuilder::setSettingControlledRes(Setting* resSetting)
{
	this->settingControlledRes = true;
	this->controlledResSetting = resSetting;
	return *this;
}

FramebufferResourceBuilder& FramebufferResourceBuilder::addRenderTarget(const char* textureName)
{
	return this->addRenderTarget(strID(textureName));
}

FramebufferResourceBuilder& FramebufferResourceBuilder::addRenderTarget(strID textureName)
{
	this->renderTargetNames.add(textureName);
	return *this;
}

FramebufferResourceBuilder& FramebufferResourceBuilder::setDepthBuffer(const char* textureName)
{
	return this->setDepthBuffer(strID(textureName));
}

FramebufferResourceBuilder& FramebufferResourceBuilder::setDepthBuffer(strID textureName)
{
	this->depthBufferName = textureName;
	return *this;
}

FramebufferResource FramebufferResourceBuilder::build(vec2_i32 screenRes)
{
	FramebufferResource resource;
	resource.name = strID(this->name);
	resource.renderTargetNames = this->renderTargetNames;
	resource.depthBufferName = this->depthBufferName;
	resource.res = this->res;
	resource.screenRelativeResolution = this->screenRelativeResolution;
	resource.resolutionScale = this->resolutionScale;
	resource.resolutionScaleSetting = this->resolutionScaleSetting;
	resource.settingControlledRes = this->settingControlledRes;
	resource.controlledResSetting = this->controlledResSetting;
	CHECK_ZG resource.build(screenRes);
	return resource;
}

} // namespace sfz
