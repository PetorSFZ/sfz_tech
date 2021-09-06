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
	SfzHandle handle = SFZ_NULL_HANDLE;
	ZgBindingType type = ZG_BINDING_TYPE_UNDEFINED;
	u32 reg = ~0u;
	u32 mipLevel = 0; // Only used for unordered textures

	BindingHL() = default;
	BindingHL(SfzHandle handle, ZgBindingType type, u32 reg, u32 mipLevel = 0) : handle(handle), type(type), reg(reg), mipLevel(mipLevel) {}
};

struct Bindings final {
	ResourceManager* res = &getResourceManager();
	ArrayLocal<BindingHL, ZG_MAX_NUM_BINDINGS> bindings;

	Bindings& addConstBuffer(const char* name, u32 reg) { return addConstBuffer(strID(name), reg); }
	Bindings& addConstBuffer(strID name, u32 reg) { return addConstBuffer(res->getBufferHandle(name), reg); }
	Bindings& addConstBuffer(SfzHandle handle, u32 reg) { bindings.add(BindingHL(handle, ZG_BINDING_TYPE_CONST_BUFFER, reg)); return *this; }

	Bindings& addUnorderedBuffer(const char* name, u32 reg) { return addUnorderedBuffer(strID(name), reg); }
	Bindings& addUnorderedBuffer(strID name, u32 reg) { return addUnorderedBuffer(res->getBufferHandle(name), reg); }
	Bindings& addUnorderedBuffer(SfzHandle handle, u32 reg) { bindings.add(BindingHL(handle, ZG_BINDING_TYPE_UNORDERED_BUFFER, reg)); return *this; }

	Bindings& addTexture(const char* name, u32 reg) { return addTexture(strID(name), reg); }
	Bindings& addTexture(strID name, u32 reg) { return addTexture(res->getTextureHandle(name), reg); }
	Bindings& addTexture(SfzHandle handle, u32 reg) { bindings.add(BindingHL(handle, ZG_BINDING_TYPE_TEXTURE, reg)); return *this; }

	Bindings& addUnorderedTexture(const char* name, u32 reg, u32 mip) { return addUnorderedTexture(strID(name), reg, mip); }
	Bindings& addUnorderedTexture(strID name, u32 reg, u32 mip) { return addUnorderedTexture(res->getTextureHandle(name), reg, mip); }
	Bindings& addUnorderedTexture(SfzHandle handle, u32 reg, u32 mip) { bindings.add(BindingHL(handle, ZG_BINDING_TYPE_UNORDERED_TEXTURE, reg, mip)); return *this; }
};

// HighLevelCmdList
// ------------------------------------------------------------------------------------------------

class HighLevelCmdList final {
public:
	SFZ_DECLARE_DROP_TYPE(HighLevelCmdList);

	void init(
		const char* cmdListName,
		u64 currFrameIdx,
		zg::CommandList cmdList,
		zg::Framebuffer* defaultFB);
	void destroy() noexcept;

	// Methods
	// --------------------------------------------------------------------------------------------

	void setShader(const char* name) { setShader(strID(name)); }
	void setShader(strID name) { setShader(mShaders->getShaderHandle(name)); }
	void setShader(SfzHandle handle);

	void setFramebuffer(const char* name) { setFramebuffer(strID(name)); }
	void setFramebuffer(strID name) { setFramebuffer(mResources->getFramebufferHandle(name)); }
	void setFramebuffer(SfzHandle handle);
	void setFramebufferDefault(); // Sets the default framebuffer

	void clearRenderTargetsOptimal();
	void clearDepthBufferOptimal();

	void setPushConstantUntyped(u32 reg, const void* data, u32 numBytes);
	template<typename T>
	void setPushConstant(u32 reg, const T& data)
	{
		static_assert(sizeof(T) <= 128);
		setPushConstantUntyped(reg, &data, sizeof(T));
	}

	void setBindings(const Bindings& bindings);

	void uploadToStreamingBufferUntyped(
		SfzHandle handle, const void* data, u32 elementSize, u32 numElements);
	template<typename T>
	void uploadToStreamingBuffer(const char* name, const T* data, u32 numElements)
	{
		uploadToStreamingBuffer<T>(strID(name), data, numElements);
	}
	template<typename T>
	void uploadToStreamingBuffer(strID name, const T* data, u32 numElements)
	{
		uploadToStreamingBuffer<T>(mResources->getBufferHandle(name), data, numElements);
	}
	template<typename T>
	void uploadToStreamingBuffer(SfzHandle handle, const T* data, u32 numElements)
	{
		uploadToStreamingBufferUntyped(handle, data, sizeof(T), numElements);
	}

	void setVertexBuffer(u32 slot, const char* name) { setVertexBuffer(slot, strID(name)); }
	void setVertexBuffer(u32 slot, strID name) { setVertexBuffer(slot, mResources->getBufferHandle(name)); }
	void setVertexBuffer(u32 slot, SfzHandle handle);

	void setIndexBuffer(const char* name, ZgIndexBufferType indexType = ZG_INDEX_BUFFER_TYPE_UINT32) { setIndexBuffer(strID(name), indexType); }
	void setIndexBuffer(strID name, ZgIndexBufferType indexType = ZG_INDEX_BUFFER_TYPE_UINT32) { setIndexBuffer(mResources->getBufferHandle(name), indexType); }
	void setIndexBuffer(SfzHandle handle, ZgIndexBufferType indexType = ZG_INDEX_BUFFER_TYPE_UINT32);

	void drawTriangles(u32 startVertex, u32 numVertices);
	void drawTrianglesIndexed(u32 firstIndex, u32 numIndices);

	i32x3 getComputeGroupDims() const;

	void dispatchCompute(i32 groupCountX, i32 groupCountY = 1, i32 groupCountZ = 1);
	void dispatchCompute(i32x3 groupCount) { dispatchCompute(groupCount.x, groupCount.y, groupCount.z); }
	void dispatchCompute(i32x2 groupCount) { dispatchCompute(groupCount.x, groupCount.y, 1u); }

	void unorderedBarrierAll();
	void unorderedBarrierBuffer(const char* name) { unorderedBarrierBuffer(strID(name)); }
	void unorderedBarrierBuffer(strID name) { unorderedBarrierBuffer(mResources->getBufferHandle(name)); }
	void unorderedBarrierBuffer(SfzHandle handle);
	void unorderedBarrierTexture(const char* name) { unorderedBarrierTexture(strID(name)); }
	void unorderedBarrierTexture(strID name) { unorderedBarrierTexture(mResources->getTextureHandle(name)); }
	void unorderedBarrierTexture(SfzHandle handle);

private:
	friend class Renderer;
	strID mName;
	u64 mCurrFrameIdx = 0;
	zg::CommandList mCmdList;
	ResourceManager* mResources = nullptr;
	ShaderManager* mShaders = nullptr;
	Shader* mBoundShader = nullptr;
	zg::Framebuffer* mDefaultFB = nullptr;
};

} // namespace sfz
