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

ZgResult FramebufferResource::build(i32x2 screenRes)
{
	CHECK_ZG zg::CommandQueue::getPresentQueue().flush();
	CHECK_ZG zg::CommandQueue::getCopyQueue().flush();

	sfz_assert(!renderTargetNames.isEmpty() || depthBufferName != SFZ_STR_ID_NULL);
	ResourceManager& resources = getResourceManager();

	// Set resolution and resolution scale if screen relative
	if (screenRelativeRes) {
		if (this->resScaleSetting != nullptr) {
			this->resScale = resScaleSetting->floatValue();
			if (resScaleSetting2 != nullptr) {
				this->resScale *= resScaleSetting2->floatValue();
			}
		}
		f32x2 scaledRes = f32x2(screenRes) * this->resScale;
		this->res.x = u32(::roundf(scaledRes.x));
		this->res.y = u32(::roundf(scaledRes.y));
	}
	else if (settingControlledRes) {
		this->res = i32x2(controlledResSetting->intValue());
	}

	zg::FramebufferBuilder fbBuilder;

	for (SfzStrID renderTargetName : renderTargetNames) {
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

	if (depthBufferName != SFZ_STR_ID_NULL) {
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

FramebufferResourceBuilder& FramebufferResourceBuilder::setFixedRes(i32x2 resIn)
{
	sfz_assert(resIn.x > 0);
	sfz_assert(resIn.y > 0);
	this->res = resIn;
	this->screenRelativeRes = false;
	this->resScale = 1.0f;
	this->resScaleSetting = nullptr;
	this->resScaleSetting2 = nullptr;
	return *this;
}

FramebufferResourceBuilder& FramebufferResourceBuilder::setScreenRelativeRes(f32 scale)
{
	this->screenRelativeRes = true;
	this->resScale = scale;
	return *this;
}

FramebufferResourceBuilder& FramebufferResourceBuilder::setScreenRelativeRes(Setting* scaleSetting, Setting* scaleSetting2)
{
	this->screenRelativeRes = true;
	this->resScale = 1.0f;
	this->resScaleSetting = scaleSetting;
	this->resScaleSetting2 = scaleSetting2;
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
	return this->addRenderTarget(sfzStrIDCreate(textureName));
}

FramebufferResourceBuilder& FramebufferResourceBuilder::addRenderTarget(SfzStrID textureName)
{
	this->renderTargetNames.add(textureName);
	return *this;
}

FramebufferResourceBuilder& FramebufferResourceBuilder::setDepthBuffer(const char* textureName)
{
	return this->setDepthBuffer(sfzStrIDCreate(textureName));
}

FramebufferResourceBuilder& FramebufferResourceBuilder::setDepthBuffer(SfzStrID textureName)
{
	this->depthBufferName = textureName;
	return *this;
}

FramebufferResource FramebufferResourceBuilder::build(i32x2 screenRes)
{
	FramebufferResource resource;
	resource.name = sfzStrIDCreate(this->name);
	resource.renderTargetNames = this->renderTargetNames;
	resource.depthBufferName = this->depthBufferName;
	resource.res = this->res;
	resource.screenRelativeRes = this->screenRelativeRes;
	resource.resScale = this->resScale;
	resource.resScaleSetting = this->resScaleSetting;
	resource.resScaleSetting2 = this->resScaleSetting2;
	resource.settingControlledRes = this->settingControlledRes;
	resource.controlledResSetting = this->controlledResSetting;
	CHECK_ZG resource.build(screenRes);
	return resource;
}

} // namespace sfz
