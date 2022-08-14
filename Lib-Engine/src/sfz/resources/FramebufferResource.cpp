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

#include <sfz_math.h>

#include "sfz/config/Setting.hpp"
#include "sfz/renderer/ZeroGUtils.hpp"
#include "sfz/resources/ResourceManager.hpp"
#include "sfz/resources/TextureResource.hpp"

// SfzFramebufferResource
// ------------------------------------------------------------------------------------------------

ZgResult SfzFramebufferResource::build(i32x2 screenRes, SfzStrIDs* ids, SfzResourceManager* resMan)
{
	CHECK_ZG zg::CommandQueue::getPresentQueue().flush();
	CHECK_ZG zg::CommandQueue::getCopyQueue().flush();

	sfz_assert(!renderTargetNames.isEmpty() || depthBufferName != SFZ_NULL_STR_ID);

	// Set resolution and resolution scale if screen relative
	if (screenRelativeRes) {
		if (this->resScaleSetting != nullptr) {
			this->resScale = resScaleSetting->floatValue();
			if (resScaleSetting2 != nullptr) {
				this->resScale *= resScaleSetting2->floatValue();
			}
		}
		f32x2 scaledRes = f32x2_from_i32(screenRes) * this->resScale;
		this->res.x = u32(sfz::round(scaledRes.x));
		this->res.y = u32(sfz::round(scaledRes.y));
	}
	else if (settingControlledRes) {
		this->res = i32x2_splat(controlledResSetting->intValue());
	}

	zg::FramebufferBuilder fbBuilder;

	for (SfzStrID renderTargetName : renderTargetNames) {
		SfzTextureResource* tex = resMan->getTexture(resMan->getTextureHandle(renderTargetName));
		sfz_assert(tex != nullptr);
		sfz_assert(tex->texture.valid());
		sfz_assert(tex->usage == ZG_TEXTURE_USAGE_RENDER_TARGET);
		if (this->res != tex->res) {
			CHECK_ZG tex->build(screenRes, ids);
		}
		sfz_assert(this->res == tex->res);
		fbBuilder.addRenderTarget(tex->texture);
	}

	if (depthBufferName != SFZ_NULL_STR_ID) {
		SfzTextureResource* tex = resMan->getTexture(resMan->getTextureHandle(depthBufferName));
		sfz_assert(tex != nullptr);
		sfz_assert(tex->texture.valid());
		sfz_assert(tex->usage == ZG_TEXTURE_USAGE_DEPTH_BUFFER);
		if (this->res != tex->res) {
			CHECK_ZG tex->build(screenRes, ids);
		}
		sfz_assert(this->res == tex->res);
		fbBuilder.setDepthBuffer(tex->texture);
	}

	return fbBuilder.build(framebuffer);
}

// SfzFramebufferResourceBuilder
// ------------------------------------------------------------------------------------------------

SfzFramebufferResourceBuilder& SfzFramebufferResourceBuilder::setName(const char* nameIn)
{
	this->name = sfzStr96Init(nameIn);
	return *this;
}

SfzFramebufferResourceBuilder& SfzFramebufferResourceBuilder::setFixedRes(i32x2 resIn)
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

SfzFramebufferResourceBuilder& SfzFramebufferResourceBuilder::setScreenRelativeRes(f32 scale)
{
	this->screenRelativeRes = true;
	this->resScale = scale;
	return *this;
}

SfzFramebufferResourceBuilder& SfzFramebufferResourceBuilder::setScreenRelativeRes(SfzSetting* scaleSetting, SfzSetting* scaleSetting2)
{
	this->screenRelativeRes = true;
	this->resScale = 1.0f;
	this->resScaleSetting = scaleSetting;
	this->resScaleSetting2 = scaleSetting2;
	return *this;
}

SfzFramebufferResourceBuilder& SfzFramebufferResourceBuilder::setSettingControlledRes(SfzSetting* resSetting)
{
	this->settingControlledRes = true;
	this->controlledResSetting = resSetting;
	return *this;
}

SfzFramebufferResourceBuilder& SfzFramebufferResourceBuilder::addRenderTarget(SfzStrIDs* ids, const char* textureName)
{
	return this->addRenderTarget(sfzStrIDCreateRegister(ids, textureName));
}

SfzFramebufferResourceBuilder& SfzFramebufferResourceBuilder::addRenderTarget(SfzStrID textureName)
{
	this->renderTargetNames.add(textureName);
	return *this;
}

SfzFramebufferResourceBuilder& SfzFramebufferResourceBuilder::setDepthBuffer(SfzStrIDs* ids, const char* textureName)
{
	return this->setDepthBuffer(sfzStrIDCreateRegister(ids, textureName));
}

SfzFramebufferResourceBuilder& SfzFramebufferResourceBuilder::setDepthBuffer(SfzStrID textureName)
{
	this->depthBufferName = textureName;
	return *this;
}

SfzFramebufferResource SfzFramebufferResourceBuilder::build(i32x2 screenRes, SfzStrIDs* ids, SfzResourceManager* resMan)
{
	SfzFramebufferResource resource;
	resource.name = sfzStrIDCreateRegister(ids, this->name.str);
	resource.renderTargetNames = this->renderTargetNames;
	resource.depthBufferName = this->depthBufferName;
	resource.res = this->res;
	resource.screenRelativeRes = this->screenRelativeRes;
	resource.resScale = this->resScale;
	resource.resScaleSetting = this->resScaleSetting;
	resource.resScaleSetting2 = this->resScaleSetting2;
	resource.settingControlledRes = this->settingControlledRes;
	resource.controlledResSetting = this->controlledResSetting;
	CHECK_ZG resource.build(screenRes, ids, resMan);
	return resource;
}
