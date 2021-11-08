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
	ZgFormat format = ZG_FORMAT_UNDEFINED; // Only used for typed buffers

	BindingHL() = default;
	BindingHL(SfzHandle handle, ZgBindingType type, u32 reg, u32 mipLevel = 0) : handle(handle), type(type), reg(reg), mipLevel(mipLevel) {}
	BindingHL(SfzHandle handle, ZgBindingType type, u32 reg, ZgFormat format) : handle(handle), type(type), reg(reg), format(format) {}
};

struct Bindings final {
	ResourceManager* res = &getResourceManager();
	ArrayLocal<BindingHL, ZG_MAX_NUM_BINDINGS> bindings;

	Bindings& addBufferConst(const char* name, u32 reg) { return addBufferConst(sfzStrIDCreate(name), reg); }
	Bindings& addBufferConst(SfzStrID name, u32 reg) { return addBufferConst(res->getBufferHandle(name), reg); }
	Bindings& addBufferConst(SfzHandle handle, u32 reg) { bindings.add(BindingHL(handle, ZG_BINDING_TYPE_BUFFER_CONST, reg)); return *this; }

	Bindings& addBufferTyped(const char* name, u32 reg, ZgFormat format) { return addBufferTyped(sfzStrIDCreate(name), reg, format); }
	Bindings& addBufferTyped(SfzStrID name, u32 reg, ZgFormat format) { return addBufferTyped(res->getBufferHandle(name), reg, format); }
	Bindings& addBufferTyped(SfzHandle handle, u32 reg, ZgFormat format) { bindings.add(BindingHL(handle, ZG_BINDING_TYPE_BUFFER_TYPED, reg, format)); return *this; }

	Bindings& addBufferStructured(const char* name, u32 reg) { return addBufferStructured(sfzStrIDCreate(name), reg); }
	Bindings& addBufferStructured(SfzStrID name, u32 reg) { return addBufferStructured(res->getBufferHandle(name), reg); }
	Bindings& addBufferStructured(SfzHandle handle, u32 reg) { bindings.add(BindingHL(handle, ZG_BINDING_TYPE_BUFFER_STRUCTURED, reg)); return *this; }

	Bindings& addBufferStructuredUAV(const char* name, u32 reg) { return addBufferStructuredUAV(sfzStrIDCreate(name), reg); }
	Bindings& addBufferStructuredUAV(SfzStrID name, u32 reg) { return addBufferStructuredUAV(res->getBufferHandle(name), reg); }
	Bindings& addBufferStructuredUAV(SfzHandle handle, u32 reg) { bindings.add(BindingHL(handle, ZG_BINDING_TYPE_BUFFER_STRUCTURED_UAV, reg)); return *this; }

	Bindings& addTexture(const char* name, u32 reg) { return addTexture(sfzStrIDCreate(name), reg); }
	Bindings& addTexture(SfzStrID name, u32 reg) { return addTexture(res->getTextureHandle(name), reg); }
	Bindings& addTexture(SfzHandle handle, u32 reg) { bindings.add(BindingHL(handle, ZG_BINDING_TYPE_TEXTURE, reg)); return *this; }

	Bindings& addTextureUAV(const char* name, u32 reg, u32 mip) { return addTextureUAV(sfzStrIDCreate(name), reg, mip); }
	Bindings& addTextureUAV(SfzStrID name, u32 reg, u32 mip) { return addTextureUAV(res->getTextureHandle(name), reg, mip); }
	Bindings& addTextureUAV(SfzHandle handle, u32 reg, u32 mip) { bindings.add(BindingHL(handle, ZG_BINDING_TYPE_TEXTURE_UAV, reg, mip)); return *this; }
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
		zg::Uploader* uploader,
		zg::Framebuffer* defaultFB);
	void destroy() noexcept;

	// Methods
	// --------------------------------------------------------------------------------------------

	void setShader(const char* name) { setShader(sfzStrIDCreate(name)); }
	void setShader(SfzStrID name) { setShader(mShaders->getShaderHandle(name)); }
	void setShader(SfzHandle handle);

	void setFramebuffer(const char* name) { setFramebuffer(sfzStrIDCreate(name)); }
	void setFramebuffer(SfzStrID name) { setFramebuffer(mResources->getFramebufferHandle(name)); }
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
		uploadToStreamingBuffer<T>(sfzStrIDCreate(name), data, numElements);
	}
	template<typename T>
	void uploadToStreamingBuffer(SfzStrID name, const T* data, u32 numElements)
	{
		uploadToStreamingBuffer<T>(mResources->getBufferHandle(name), data, numElements);
	}
	template<typename T>
	void uploadToStreamingBuffer(SfzHandle handle, const T* data, u32 numElements)
	{
		uploadToStreamingBufferUntyped(handle, data, sizeof(T), numElements);
	}

	void setVertexBuffer(u32 slot, const char* name) { setVertexBuffer(slot, sfzStrIDCreate(name)); }
	void setVertexBuffer(u32 slot, SfzStrID name) { setVertexBuffer(slot, mResources->getBufferHandle(name)); }
	void setVertexBuffer(u32 slot, SfzHandle handle);

	void setIndexBuffer(const char* name, ZgIndexBufferType indexType = ZG_INDEX_BUFFER_TYPE_UINT32) { setIndexBuffer(sfzStrIDCreate(name), indexType); }
	void setIndexBuffer(SfzStrID name, ZgIndexBufferType indexType = ZG_INDEX_BUFFER_TYPE_UINT32) { setIndexBuffer(mResources->getBufferHandle(name), indexType); }
	void setIndexBuffer(SfzHandle handle, ZgIndexBufferType indexType = ZG_INDEX_BUFFER_TYPE_UINT32);

	void drawTriangles(u32 startVertex, u32 numVertices);
	void drawTrianglesIndexed(u32 firstIndex, u32 numIndices);

	i32x3 getComputeGroupDims() const;

	void dispatchCompute(i32 groupCountX, i32 groupCountY = 1, i32 groupCountZ = 1);
	void dispatchCompute(i32x3 groupCount) { dispatchCompute(groupCount.x, groupCount.y, groupCount.z); }
	void dispatchCompute(i32x2 groupCount) { dispatchCompute(groupCount.x, groupCount.y, 1u); }

	void uavBarrierAll();
	void uavBarrierBuffer(const char* name) { uavBarrierBuffer(sfzStrIDCreate(name)); }
	void uavBarrierBuffer(SfzStrID name) { uavBarrierBuffer(mResources->getBufferHandle(name)); }
	void uavBarrierBuffer(SfzHandle handle);
	void uavBarrierTexture(const char* name) { uavBarrierTexture(sfzStrIDCreate(name)); }
	void uavBarrierTexture(SfzStrID name) { uavBarrierTexture(mResources->getTextureHandle(name)); }
	void uavBarrierTexture(SfzHandle handle);

private:
	friend class Renderer;
	SfzStrID mName = SFZ_STR_ID_NULL;
	u64 mCurrFrameIdx = 0;
	zg::CommandList mCmdList;
	zg::Uploader* mUploader = nullptr;
	ResourceManager* mResources = nullptr;
	ShaderManager* mShaders = nullptr;
	Shader* mBoundShader = nullptr;
	zg::Framebuffer* mDefaultFB = nullptr;
};

} // namespace sfz
