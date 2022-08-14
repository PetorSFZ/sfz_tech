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

#include "sfz/shaders/ShaderManager.hpp"

#include "sfz/config/GlobalConfig.hpp"
#include "sfz/config/SfzConfig.h"
#include "sfz/renderer/ZeroGUtils.hpp"
#include "sfz/shaders/ShaderManagerState.hpp"
#include "sfz/shaders/ShaderManagerUI.hpp"
#include "sfz/util/IO.hpp"

// SfzShader
// ------------------------------------------------------------------------------------------------

bool SfzShader::build() noexcept
{
	bool buildSuccess = false;
	if (this->type == SfzShaderType::RENDER) {
		buildSuccess = CHECK_ZG this->renderPipeline.createFromFileHLSL(
			this->renderDesc, this->compileSettings);
		if (buildSuccess) {
			this->lastModified = sfz::fileLastModifiedDate(this->renderDesc.path);
		}
	}
	else if (this->type == SfzShaderType::COMPUTE) {
		buildSuccess = CHECK_ZG this->computePipeline.createFromFileHLSL(
			this->computeDesc, this->compileSettings);
		if (buildSuccess) {
			this->lastModified = sfz::fileLastModifiedDate(this->computeDesc.path);
		}
	}
	else {
		sfz_assert_hard(false);
	}
	return buildSuccess;
}

// SfzShaderManager
// ------------------------------------------------------------------------------------------------

void SfzShaderManager::init(u32 maxNumShaders, SfzConfig* cfg, SfzAllocator* allocator) noexcept
{
	sfz_assert(mState == nullptr);
	mState = sfz_new<SfzShaderManagerState>(allocator, sfz_dbg(""));
	mState->allocator = allocator;

	mState->shaderHandles.init(maxNumShaders, allocator, sfz_dbg(""));
	mState->shaders.init(maxNumShaders, allocator, sfz_dbg(""));

	mState->shaderFileWatchEnabled = sfzCfgGetSetting(cfg, "Resources.shaderFileWatch");
}

void SfzShaderManager::destroy() noexcept
{
	if (mState == nullptr) return;

	// Flush ZeroG queues to ensure no shaders are still in-use
	CHECK_ZG zg::CommandQueue::getPresentQueue().flush();
	CHECK_ZG zg::CommandQueue::getCopyQueue().flush();

	SfzAllocator* allocator = mState->allocator;
	sfz_delete(allocator, mState);
	mState = nullptr;
}

// SfzShaderManager: Methods
// ------------------------------------------------------------------------------------------------

void SfzShaderManager::update()
{
	// Nothing to do (for now) if shader file watch is disabled. In future we could potentially have
	// async shader loading which could be updated here.
	if (!mState->shaderFileWatchEnabled->boolValue()) return;

	for (auto pair : mState->shaderHandles) {
		SfzShader& shader = mState->shaders[pair.value];

		i64 newLastModified = 0;
		if (shader.type == SfzShaderType::RENDER) {
			newLastModified = sfz::fileLastModifiedDate(shader.renderDesc.path);
		} 
		else if (shader.type == SfzShaderType::COMPUTE) {
			newLastModified = sfz::fileLastModifiedDate(shader.computeDesc.path);
		}
		if (shader.lastModified < newLastModified) {
			
			// Flush present queue to ensure shader is not in use
			CHECK_ZG zg::CommandQueue::getPresentQueue().flush();
			
			// Attempt to rebuild shader. Note: No need to check return value
			shader.build();

			// Technically superfluous, but keeps us from attempting to recompile broken shaders
			// each frame.
			shader.lastModified = newLastModified;
		}
	}
}

void SfzShaderManager::renderDebugUI(SfzStrIDs* ids)
{
	sfz::shaderManagerUI(*mState, ids);
}

SfzHandle SfzShaderManager::getShaderHandle(SfzStrIDs* ids, const char* name) const
{
	return this->getShaderHandle(sfzStrIDCreateRegister(ids, name));
}

SfzHandle SfzShaderManager::getShaderHandle(SfzStrID name) const
{
	const SfzHandle* handle = mState->shaderHandles.get(name);
	if (handle == nullptr) return SFZ_NULL_HANDLE;
	return *handle;
}

SfzShader* SfzShaderManager::getShader(SfzHandle handle)
{
	return mState->shaders.get(handle);
}

SfzHandle SfzShaderManager::addShaderRender(
	const ZgPipelineRenderDesc& desc,
	const ZgPipelineCompileSettingsHLSL& settings,
	SfzStrIDs* ids)
{
	const SfzHandle handle = mState->shaders.allocate();
	SfzShader* shader = mState->shaders.get(handle);
	shader->name = sfzStrIDCreateRegister(ids, desc.name);
	sfz_assert(mState->shaderHandles.get(shader->name) == nullptr);
	shader->type = SfzShaderType::RENDER;
	shader->compileSettings = settings;
	shader->renderDesc = desc;

	const bool buildSuccess = shader->build();
	if (!buildSuccess) {
		SFZ_LOG_ERROR("Couldn't build shader \"%s\"", desc.name);
		mState->shaders.deallocate(handle);
		return SFZ_NULL_HANDLE;
	}

	mState->shaderHandles.put(shader->name, handle);
	sfz_assert(mState->shaderHandles.size() == mState->shaders.numAllocated());
	return handle;
}

SfzHandle SfzShaderManager::addShaderCompute(
	const ZgPipelineComputeDesc& desc,
	const ZgPipelineCompileSettingsHLSL& settings,
	SfzStrIDs* ids)
{
	const SfzHandle handle = mState->shaders.allocate();
	SfzShader* shader = mState->shaders.get(handle);
	shader->name = sfzStrIDCreateRegister(ids, desc.name);
	sfz_assert(mState->shaderHandles.get(shader->name) == nullptr);
	shader->type = SfzShaderType::COMPUTE;
	shader->compileSettings = settings;
	shader->computeDesc = desc;

	const bool buildSuccess = shader->build();
	if (!buildSuccess) {
		SFZ_LOG_ERROR("Couldn't build shader \"%s\"", desc.name);
		mState->shaders.deallocate(handle);
		return SFZ_NULL_HANDLE;
	}

	mState->shaderHandles.put(shader->name, handle);
	sfz_assert(mState->shaderHandles.size() == mState->shaders.numAllocated());
	return handle;
}

void SfzShaderManager::removeShader(SfzStrID name)
{
	// TODO: Currently blocking, can probably be made async.
	CHECK_ZG zg::CommandQueue::getPresentQueue().flush();
	CHECK_ZG zg::CommandQueue::getCopyQueue().flush();

	SfzHandle handle = this->getShaderHandle(name);
	if (handle == SFZ_NULL_HANDLE) return;
	mState->shaderHandles.remove(name);
	mState->shaders.deallocate(handle);
}
