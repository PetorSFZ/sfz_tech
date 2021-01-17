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

#include <skipifzero.hpp>
#include <skipifzero_arrays.hpp>
#include <skipifzero_pool.hpp>
#include <skipifzero_strings.hpp>

#include <ZeroG.h>

#include <sfz/Context.hpp>
#include <sfz/resources/ResourceManager.hpp>
#include <sfz/shaders/ShaderManager.hpp>

namespace sfz {

// Bindings
// ------------------------------------------------------------------------------------------------

struct BindingHL final {
	PoolHandle handle = NULL_HANDLE;
	uint32_t reg = ~0u;
	uint32_t mipLevel = 0; // Only used for unordered textures

	BindingHL() = default;
	BindingHL(PoolHandle handle, uint32_t reg, uint32_t mipLevel = 0) : handle(handle), reg(reg), mipLevel(mipLevel) {}
};

struct Bindings final {
	ResourceManager* res = &getResourceManager();
	ArrayLocal<BindingHL, ZG_MAX_NUM_CONSTANT_BUFFERS> constBuffers;
	ArrayLocal<BindingHL, ZG_MAX_NUM_UNORDERED_BUFFERS> unorderedBuffers;
	ArrayLocal<BindingHL, ZG_MAX_NUM_TEXTURES> textures;
	ArrayLocal<BindingHL, ZG_MAX_NUM_UNORDERED_TEXTURES> unorderedTextures;

	Bindings& addConstBuffer(const char* name, uint32_t reg) { return addConstBuffer(strID(name), reg); }
	Bindings& addConstBuffer(strID name, uint32_t reg) { return addConstBuffer(res->getBufferHandle(name), reg); }
	Bindings& addConstBuffer(PoolHandle handle, uint32_t reg) { constBuffers.add(BindingHL(handle, reg)); return *this; }

	Bindings& addUnorderedBuffer(const char* name, uint32_t reg) { return addUnorderedBuffer(strID(name), reg); }
	Bindings& addUnorderedBuffer(strID name, uint32_t reg) { return addUnorderedBuffer(res->getBufferHandle(name), reg); }
	Bindings& addUnorderedBuffer(PoolHandle handle, uint32_t reg) { unorderedBuffers.add(BindingHL(handle, reg)); return *this; }

	Bindings& addTexture(const char* name, uint32_t reg) { return addTexture(strID(name), reg); }
	Bindings& addTexture(strID name, uint32_t reg) { return addTexture(res->getTextureHandle(name), reg); }
	Bindings& addTexture(PoolHandle handle, uint32_t reg) { textures.add(BindingHL(handle, reg)); return *this; }

	Bindings& addUnorderedTexture(const char* name, uint32_t reg, uint32_t mip) { return addUnorderedTexture(strID(name), reg, mip); }
	Bindings& addUnorderedTexture(strID name, uint32_t reg, uint32_t mip) { return addUnorderedTexture(res->getTextureHandle(name), reg, mip); }
	Bindings& addUnorderedTexture(PoolHandle handle, uint32_t reg, uint32_t mip) { unorderedTextures.add(BindingHL(handle, reg, mip)); return *this; }
};

// HighLevelCmdList
// ------------------------------------------------------------------------------------------------

class HighLevelCmdList final {
public:
	SFZ_DECLARE_DROP_TYPE(HighLevelCmdList);

	void init(
		const char* cmdListName,
		uint64_t currFrameIdx,
		zg::CommandList cmdList,
		zg::Framebuffer* defaultFB);
	void destroy() noexcept;

	// Methods
	// --------------------------------------------------------------------------------------------

	void setShader(const char* name) { setShader(strID(name)); }
	void setShader(strID name) { setShader(mShaders->getShaderHandle(name)); }
	void setShader(PoolHandle handle);

	void setFramebuffer(const char* name) { setFramebuffer(strID(name)); }
	void setFramebuffer(strID name) { setFramebuffer(mResources->getFramebufferHandle(name)); }
	void setFramebuffer(PoolHandle handle);
	void setFramebufferDefault(); // Sets the default framebuffer

	void clearRenderTargetsOptimal();
	void clearDepthBufferOptimal();

	void setPushConstantUntyped(uint32_t reg, const void* data, uint32_t numBytes);
	template<typename T>
	void setPushConstant(uint32_t reg, const T& data)
	{
		static_assert(sizeof(T) <= 128);
		setPushConstantUntyped(reg, &data, sizeof(T));
	}

	void setBindings(const Bindings& bindings);

	void uploadToStreamingBufferUntyped(
		PoolHandle handle, const void* data, uint32_t elementSize, uint32_t numElements);
	template<typename T>
	void uploadToStreamingBuffer(const char* name, const T* data, uint32_t numElements)
	{
		uploadToStreamingBuffer<T>(strID(name), data, numElements);
	}
	template<typename T>
	void uploadToStreamingBuffer(strID name, const T* data, uint32_t numElements)
	{
		uploadToStreamingBuffer<T>(mResources->getBufferHandle(name), data, numElements);
	}
	template<typename T>
	void uploadToStreamingBuffer(PoolHandle handle, const T* data, uint32_t numElements)
	{
		uploadToStreamingBufferUntyped(handle, data, sizeof(T), numElements);
	}

	void setVertexBuffer(uint32_t slot, const char* name) { setVertexBuffer(slot, strID(name)); }
	void setVertexBuffer(uint32_t slot, strID name) { setVertexBuffer(slot, mResources->getBufferHandle(name)); }
	void setVertexBuffer(uint32_t slot, PoolHandle handle);

	void setIndexBuffer(const char* name, ZgIndexBufferType indexType = ZG_INDEX_BUFFER_TYPE_UINT32) { setIndexBuffer(strID(name), indexType); }
	void setIndexBuffer(strID name, ZgIndexBufferType indexType = ZG_INDEX_BUFFER_TYPE_UINT32) { setIndexBuffer(mResources->getBufferHandle(name), indexType); }
	void setIndexBuffer(PoolHandle handle, ZgIndexBufferType indexType = ZG_INDEX_BUFFER_TYPE_UINT32);

	void drawTriangles(uint32_t startVertex, uint32_t numVertices);
	void drawTrianglesIndexed(uint32_t firstIndex, uint32_t numIndices);

	vec3_u32 getComputeGroupDims() const;

	void dispatchCompute(uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1);
	void dispatchCompute(vec3_u32 groupCount) { dispatchCompute(groupCount.x, groupCount.y, groupCount.z); }
	void dispatchCompute(vec2_u32 groupCount) { dispatchCompute(groupCount.x, groupCount.y, 1u); }

	void unorderedBarrierAll();
	void unorderedBarrierBuffer(const char* name) { unorderedBarrierBuffer(strID(name)); }
	void unorderedBarrierBuffer(strID name) { unorderedBarrierBuffer(mResources->getBufferHandle(name)); }
	void unorderedBarrierBuffer(PoolHandle handle);
	void unorderedBarrierTexture(const char* name) { unorderedBarrierTexture(strID(name)); }
	void unorderedBarrierTexture(strID name) { unorderedBarrierTexture(mResources->getTextureHandle(name)); }
	void unorderedBarrierTexture(PoolHandle handle);

private:
	friend class Renderer;
	strID mName;
	uint64_t mCurrFrameIdx = 0;
	zg::CommandList mCmdList;
	ResourceManager* mResources = nullptr;
	ShaderManager* mShaders = nullptr;
	Shader* mBoundShader = nullptr;
	zg::Framebuffer* mDefaultFB = nullptr;
};

} // namespace sfz
