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

#include "ZeroG.h"

namespace zg {

// Forward declarations
// ------------------------------------------------------------------------------------------------

class Context;
class PipelineRender;
class MemoryHeap;
class Buffer;
class TextureHeap;
class Texture2D;
class Framebuffer;
class Fence;
class CommandQueue;
class CommandList;


// Error handling
// ------------------------------------------------------------------------------------------------

enum class [[nodiscard]] ErrorCode : ZgErrorCode {

	SUCCESS = ZG_SUCCESS,
	
	WARNING_GENERIC = ZG_WARNING_GENERIC,
	WARNING_ALREADY_INITIALIZED = ZG_WARNING_ALREADY_INITIALIZED,

	GENERIC = ZG_ERROR_GENERIC,
	UNIMPLEMENTED = ZG_ERROR_UNIMPLEMENTED,
	CPU_OUT_OF_MEMORY = ZG_ERROR_CPU_OUT_OF_MEMORY,
	GPU_OUT_OF_MEMORY = ZG_ERROR_GPU_OUT_OF_MEMORY,
	NO_SUITABLE_DEVICE = ZG_ERROR_NO_SUITABLE_DEVICE,
	INVALID_ARGUMENT = ZG_ERROR_INVALID_ARGUMENT,
	SHADER_COMPILE_ERROR = ZG_ERROR_SHADER_COMPILE_ERROR,
	OUT_OF_COMMAND_LISTS = ZG_ERROR_OUT_OF_COMMAND_LISTS,
	INVALID_COMMAND_LIST_STATE = ZG_ERROR_INVALID_COMMAND_LIST_STATE
};

constexpr bool isSuccess(ErrorCode code) noexcept { return code == ErrorCode::SUCCESS; }
constexpr bool isWarning(ErrorCode code) noexcept { return ZgErrorCode(code) > 0; }
constexpr bool isError(ErrorCode code) noexcept { return ZgErrorCode(code) < 0; }


// Context
// ------------------------------------------------------------------------------------------------

// The ZeroG context is the main entry point for all ZeroG functions.
//
// ZeroG actually has an implicit context (i.e., it is only possible to have a single context
// running at the time), but we pretend that there is an explicit context in order to make the user
// write their code that way.
class Context final {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	Context() noexcept = default;
	Context(const Context&) = delete;
	Context& operator= (const Context&) = delete;
	Context(Context&& other) noexcept { this->swap(other); }
	Context& operator= (Context&& other) noexcept { this->swap(other); return *this; }
	~Context() noexcept { this->deinit(); }

	// State methods
	// --------------------------------------------------------------------------------------------

	// Creates and initializes a context, see zgContextInit()
	ErrorCode init(const ZgContextInitSettings& settings) noexcept;

	// Deinitializes a context, see zgContextDeinit()
	//
	// Not necessary to call manually, will be called by the destructor.
	void deinit() noexcept;

	// Swaps two contexts. Since only one can be active, this is equal to a move in practice.
	void swap(Context& other) noexcept;

	// Version methods
	// --------------------------------------------------------------------------------------------

	// The API version used to compile ZeroG, see ZG_COMPILED_API_VERSION
	static uint32_t compiledApiVersion() noexcept { return ZG_COMPILED_API_VERSION; }

	// The API version of the ZeroG DLL you have linked with, see zgApiLinkedVersion()
	static uint32_t linkedApiVersion() noexcept;

	// Context methods
	// --------------------------------------------------------------------------------------------

	// Checks if a ZeroG context is already initialized, see zgContextAlreadyInitialized()
	static bool alreadyInitialized() noexcept;

	// Resizes the back buffers in the swap chain, safe to call every frame.
	//
	// See zgContextSwapchainResize()
	ErrorCode swapchainResize(uint32_t width, uint32_t height) noexcept;

	// See zgContextSwapchainBeginFrame()
	ErrorCode swapchainBeginFrame(Framebuffer& framebufferOut) noexcept;

	// See zgContextSwapchainFinishFrame()
	ErrorCode swapchainFinishFrame() noexcept;

	// See zgContextGetStats()
	ErrorCode getStats(ZgStats& statsOut) noexcept;

	// Private members
	// --------------------------------------------------------------------------------------------
private:

	bool mInitialized = false;
};


// PipelineRenderBuilder
// ------------------------------------------------------------------------------------------------

class PipelineRenderBuilder final {
public:
	// Members
	// --------------------------------------------------------------------------------------------

	ZgPipelineRenderCreateInfoCommon commonInfo = {};
	const char* vertexShaderPath = nullptr;
	const char* pixelShaderPath = nullptr;
	const char* vertexShaderSrc = nullptr;
	const char* pixelShaderSrc = nullptr;

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	PipelineRenderBuilder() noexcept = default;
	PipelineRenderBuilder(const PipelineRenderBuilder&) noexcept = default;
	PipelineRenderBuilder& operator= (const PipelineRenderBuilder&) noexcept = default;
	~PipelineRenderBuilder() noexcept = default;

	// Methods
	// --------------------------------------------------------------------------------------------

	PipelineRenderBuilder& addVertexAttribute(ZgVertexAttribute attribute) noexcept;
	
	PipelineRenderBuilder& addVertexAttribute(
		uint32_t location,
		uint32_t vertexBufferSlot,
		ZgVertexAttributeType type,
		uint32_t offsetInBuffer) noexcept;
	
	PipelineRenderBuilder& addVertexBufferInfo(
		uint32_t slot, uint32_t vertexBufferStrideBytes) noexcept;
	
	PipelineRenderBuilder& addPushConstant(uint32_t constantBufferRegister) noexcept;
	
	PipelineRenderBuilder& addSampler(uint32_t samplerRegister, ZgSampler sampler) noexcept;
	
	PipelineRenderBuilder& addSampler(
		uint32_t samplerRegister,
		ZgSamplingMode samplingMode,
		ZgWrappingMode wrappingModeU = ZG_WRAPPING_MODE_CLAMP,
		ZgWrappingMode wrappingModeV = ZG_WRAPPING_MODE_CLAMP,
		float mipLodBias = 0.0f) noexcept;

	PipelineRenderBuilder& addRenderTarget(ZgTextureFormat format) noexcept;

	PipelineRenderBuilder& addVertexShaderPath(const char* entry, const char* path) noexcept;
	PipelineRenderBuilder& addPixelShaderPath(const char* entry, const char* path) noexcept;
	PipelineRenderBuilder& addVertexShaderSource(const char* entry, const char* src) noexcept;
	PipelineRenderBuilder& addPixelShaderSource(const char* entry, const char* src) noexcept;

	PipelineRenderBuilder& setWireframeRendering(bool wireframeEnabled) noexcept;
	PipelineRenderBuilder& setCullingEnabled(bool cullingEnabled) noexcept;
	PipelineRenderBuilder& setCullMode(
		bool cullFrontFacing, bool fontFacingIsCounterClockwise = false) noexcept;

	PipelineRenderBuilder& setBlendingEnabled(bool blendingEnabled) noexcept;
	PipelineRenderBuilder& setBlendFuncColor(
		ZgBlendFunc func, ZgBlendValue srcFactor, ZgBlendValue dstFactor) noexcept;
	PipelineRenderBuilder& setBlendFuncAlpha(
		ZgBlendFunc func, ZgBlendValue srcFactor, ZgBlendValue dstFactor) noexcept;

	PipelineRenderBuilder& setDepthTestEnabled(bool depthTestEnabled) noexcept;
	PipelineRenderBuilder& setDepthFunc(ZgDepthFunc depthFunc) noexcept;

	ErrorCode buildFromFileSPIRV(PipelineRender& pipelineOut) const noexcept;
	ErrorCode buildFromFileHLSL(
		PipelineRender& pipelineOut, ZgShaderModel model = ZG_SHADER_MODEL_6_0) const noexcept;
	ErrorCode buildFromSourceHLSL(
		PipelineRender& pipelineOut, ZgShaderModel model = ZG_SHADER_MODEL_6_0) const noexcept;
};


// PipelineRender
// ------------------------------------------------------------------------------------------------

class PipelineRender final {
public:
	// Members
	// --------------------------------------------------------------------------------------------

	ZgPipelineRender* pipeline = nullptr;
	ZgPipelineRenderSignature signature = {};

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	PipelineRender() noexcept = default;
	PipelineRender(const PipelineRender&) = delete;
	PipelineRender& operator= (const PipelineRender&) = delete;
	PipelineRender(PipelineRender&& o) noexcept { this->swap(o); }
	PipelineRender& operator= (PipelineRender&& o) noexcept { this->swap(o); return *this; }
	~PipelineRender() noexcept { this->release(); }

	// State methods
	// --------------------------------------------------------------------------------------------
	
	// Checks if this pipeline is valid
	bool valid() const noexcept { return this->pipeline != nullptr; }

	// See zgPipelineRenderCreateFromFileSPIRV()
	ErrorCode createFromFileSPIRV(
		const ZgPipelineRenderCreateInfoFileSPIRV& createInfo) noexcept;
	
	// See ZgPipelineRenderCreateInfoFileHLSL()
	ErrorCode createFromFileHLSL(
		const ZgPipelineRenderCreateInfoFileHLSL& createInfo) noexcept;
	
	// See ZgPipelineRenderCreateInfoSourceHLSL()
	ErrorCode createFromSourceHLSL(
		const ZgPipelineRenderCreateInfoSourceHLSL& createInfo) noexcept;

	void swap(PipelineRender& other) noexcept;

	// See zgPipelineRenderRelease()
	void release() noexcept;
};


// MemoryHeap
// ------------------------------------------------------------------------------------------------

class MemoryHeap final {
public:
	// Members
	// --------------------------------------------------------------------------------------------

	ZgMemoryHeap* memoryHeap = nullptr;

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	MemoryHeap() noexcept = default;
	MemoryHeap(const MemoryHeap&) = delete;
	MemoryHeap& operator= (const MemoryHeap&) = delete;
	MemoryHeap(MemoryHeap&& o) noexcept { this->swap(o); }
	MemoryHeap& operator= (MemoryHeap&& o) noexcept { this->swap(o); return *this; }
	~MemoryHeap() noexcept { this->release(); }

	// State methods
	// --------------------------------------------------------------------------------------------

	bool valid() const noexcept { return memoryHeap != nullptr; }

	// See zgMemoryHeapCreate()
	ErrorCode create(const ZgMemoryHeapCreateInfo& createInfo) noexcept;
	ErrorCode create(uint64_t sizeInBytes, ZgMemoryType memoryType) noexcept;

	void swap(MemoryHeap& other) noexcept;

	// See zgMemoryHeapRelease()
	void release() noexcept;

	// MemoryHeap methods
	// --------------------------------------------------------------------------------------------

	// See zgMemoryHeapBufferCreate()
	ErrorCode bufferCreate(Buffer& bufferOut, const ZgBufferCreateInfo& createInfo) noexcept;
	ErrorCode bufferCreate(Buffer& bufferOut, uint64_t offset, uint64_t size) noexcept;

	// See zgMemoryHeapTexture2DCreate()
	ErrorCode texture2DCreate(
		Texture2D& textureOut, const ZgTexture2DCreateInfo& createInfo) noexcept;
};


// Buffer
// ------------------------------------------------------------------------------------------------

class Buffer final {
public:
	// Members
	// --------------------------------------------------------------------------------------------

	ZgBuffer* buffer = nullptr;

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	Buffer() noexcept = default;
	Buffer(const Buffer&) = delete;
	Buffer& operator= (const Buffer&) = delete;
	Buffer(Buffer&& o) noexcept { this->swap(o); }
	Buffer& operator= (Buffer&& o) noexcept { this->swap(o); return *this; }
	~Buffer() noexcept { this->release(); }

	// State methods
	// --------------------------------------------------------------------------------------------

	bool valid() const noexcept { return buffer != nullptr; }

	void swap(Buffer& other) noexcept;

	// See zgBufferRelease()
	void release() noexcept;

	// Buffer methods
	// --------------------------------------------------------------------------------------------

	// See zgBufferMemcpyTo()
	ErrorCode memcpyTo(uint64_t bufferOffsetBytes, const void* srcMemory, uint64_t numBytes);

	// See zgBufferSetDebugName()
	ErrorCode setDebugName(const char* name) noexcept;
};


// Texture2D
// ------------------------------------------------------------------------------------------------

class Texture2D final {
public:
	// Members
	// --------------------------------------------------------------------------------------------

	ZgTexture2D* texture = nullptr;

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	Texture2D() noexcept = default;
	Texture2D(const Texture2D&) = delete;
	Texture2D& operator= (const Texture2D&) = delete;
	Texture2D(Texture2D&& o) noexcept { this->swap(o); }
	Texture2D& operator= (Texture2D&& o) noexcept { this->swap(o); return *this; }
	~Texture2D() noexcept { this->release(); }

	// State methods
	// --------------------------------------------------------------------------------------------

	bool valid() const noexcept { return this->texture != nullptr;  }

	void swap(Texture2D& other) noexcept;

	// See zgTexture2DRelease()
	void release() noexcept;

	// Methods
	// --------------------------------------------------------------------------------------------

	// See zgTexture2DGetAllocationInfo
	static ErrorCode getAllocationInfo(
		ZgTexture2DAllocationInfo& allocationInfoOut,
		const ZgTexture2DCreateInfo& createInfo) noexcept;

	// See zgTexture2DSetDebugName()
	ErrorCode setDebugName(const char* name) noexcept;
};


// FramebufferBuilder
// ------------------------------------------------------------------------------------------------

class FramebufferBuilder final {
public:

	// Members
	// --------------------------------------------------------------------------------------------

	ZgFramebufferCreateInfo createInfo = {};

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	FramebufferBuilder() noexcept = default;
	FramebufferBuilder(const FramebufferBuilder&) noexcept = default;
	FramebufferBuilder& operator= (const FramebufferBuilder&) noexcept = default;
	~FramebufferBuilder() noexcept = default;

	// Methods
	// --------------------------------------------------------------------------------------------

	FramebufferBuilder& addRenderTarget(Texture2D& renderTarget) noexcept;
	FramebufferBuilder& setDepthBuffer(Texture2D& depthBuffer) noexcept;

	ErrorCode build(Framebuffer& framebufferOut) noexcept;
};


// Framebuffer
// ------------------------------------------------------------------------------------------------

class Framebuffer final {
public:
	// Members
	// --------------------------------------------------------------------------------------------

	ZgFramebuffer* framebuffer = nullptr;
	uint32_t width = 0;
	uint32_t height = 0;

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	Framebuffer() noexcept = default;
	Framebuffer(const Framebuffer&) = delete;
	Framebuffer& operator= (const Framebuffer&) = delete;
	Framebuffer(Framebuffer&& other) noexcept { this->swap(other); }
	Framebuffer& operator= (Framebuffer&& other) noexcept { this->swap(other); return *this; }
	~Framebuffer() noexcept { this->release(); }

	// State methods
	// --------------------------------------------------------------------------------------------

	bool valid() const noexcept { return this->framebuffer != nullptr; }

	// See zgFramebufferCreate()
	ErrorCode create(const ZgFramebufferCreateInfo& createInfo) noexcept;

	void swap(Framebuffer& other) noexcept;

	// See zgFramebufferRelease()
	void release() noexcept;
};


// Fence
// ------------------------------------------------------------------------------------------------

class Fence final {
public:
	// Members
	// --------------------------------------------------------------------------------------------

	ZgFence* fence = nullptr;

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	Fence() noexcept = default;
	Fence(const Fence&) = delete;
	Fence& operator= (const Fence&) = delete;
	Fence(Fence&& other) noexcept { this->swap(other); }
	Fence& operator= (Fence&& other) noexcept { this->swap(other); return *this; }
	~Fence() noexcept { this->release(); }

	// State methods
	// --------------------------------------------------------------------------------------------

	bool valid() const noexcept { return this->fence != nullptr; }

	// See zgFenceCreate()
	ErrorCode create() noexcept;

	void swap(Fence& other) noexcept;

	// See zgFenceRelease()
	void release() noexcept;

	// Fence methods
	// --------------------------------------------------------------------------------------------

	// See zgFenceReset()
	ErrorCode reset() noexcept;

	// See zgFenceCheckIfSignaled()
	ErrorCode checkIfSignaled(bool& fenceSignaledOut) const noexcept;
	bool checkIfSignaled() const noexcept;

	// See zgFenceWaitOnCpuBlocking()
	ErrorCode waitOnCpuBlocking() const noexcept;
};


// CommandQueue
// ------------------------------------------------------------------------------------------------

class CommandQueue final {
public:
	// Members
	// --------------------------------------------------------------------------------------------

	ZgCommandQueue* commandQueue = nullptr;

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	CommandQueue() noexcept = default;
	CommandQueue(const CommandQueue&) = delete;
	CommandQueue& operator= (const CommandQueue&) = delete;
	CommandQueue(CommandQueue&& other) noexcept { this->swap(other); }
	CommandQueue& operator= (CommandQueue&& other) noexcept { this->swap(other); return *this; }
	~CommandQueue() noexcept { this->release(); }

	// See zgCommandQueueGetPresentQueue()
	static ErrorCode getPresentQueue(CommandQueue& presentQueueOut) noexcept;

	// See zgCommandQueueGetCopyQueue()
	static ErrorCode getCopyQueue(CommandQueue& copyQueueOut) noexcept;

	// State methods
	// --------------------------------------------------------------------------------------------

	bool valid() const noexcept { return commandQueue != nullptr; }

	void swap(CommandQueue& other) noexcept;

	// TODO: No-op because there currently is no releasing of Command Queues...
	void release() noexcept;

	// CommandQueue methods
	// --------------------------------------------------------------------------------------------

	// See zgCommandQueueSignalOnGpu()
	ErrorCode signalOnGpu(Fence& fenceToSignal) noexcept;

	// See zgCommandQueueWaitOnGpu()
	ErrorCode waitOnGpu(const Fence& fence) noexcept;

	// See zgCommandQueueFlush()
	ErrorCode flush() noexcept;

	// See zgCommandQueueBeginCommandListRecording()
	ErrorCode beginCommandListRecording(CommandList& commandListOut) noexcept;

	// See zgCommandQueueExecuteCommandList()
	ErrorCode executeCommandList(CommandList& commandList) noexcept;
};


// PipelineBindings
// ------------------------------------------------------------------------------------------------

struct ConstantBufferBinding final {
	uint32_t shaderRegister = ~0u;
	Buffer* buffer = nullptr;
};

struct TextureBinding final {
	uint32_t textureRegister = ~0u;
	Texture2D* texture = nullptr;
};

class PipelineBindings final {
public:

	// Members
	// --------------------------------------------------------------------------------------------

	// The constant buffers to bind
	uint32_t numConstantBuffers = 0;
	ConstantBufferBinding constantBuffers[ZG_MAX_NUM_CONSTANT_BUFFERS];

	// The textures to bind
	uint32_t numTextures = 0;
	TextureBinding textures[ZG_MAX_NUM_TEXTURES];

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	PipelineBindings() noexcept = default;
	PipelineBindings(const PipelineBindings&) noexcept = default;
	PipelineBindings& operator= (const PipelineBindings&) noexcept = default;
	~PipelineBindings() noexcept = default;

	// Methods
	// --------------------------------------------------------------------------------------------

	PipelineBindings& addConstantBuffer(ConstantBufferBinding binding) noexcept;
	PipelineBindings& addConstantBuffer(uint32_t shaderRegister, Buffer& buffer) noexcept;

	PipelineBindings& addTexture(TextureBinding binding) noexcept;
	PipelineBindings& addTexture(uint32_t textureRegister, Texture2D& texture) noexcept;

	ZgPipelineBindings toCApi() const noexcept;
};


// CommandList
// ------------------------------------------------------------------------------------------------

class CommandList final {
public:
	// Members
	// --------------------------------------------------------------------------------------------

	ZgCommandList* commandList = nullptr;

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	CommandList() noexcept = default;
	CommandList(const CommandList&) = delete;
	CommandList& operator= (const CommandList&) = delete;
	CommandList(CommandList&& other) noexcept { this->swap(other); }
	CommandList& operator= (CommandList&& other) noexcept { this->swap(other); return *this; }
	~CommandList() noexcept { this->release(); }

	// State methods
	// --------------------------------------------------------------------------------------------

	bool valid() const noexcept { return commandList != nullptr; }

	void swap(CommandList& other) noexcept;

	void release() noexcept;

	// CommandList methods
	// --------------------------------------------------------------------------------------------

	// See zgCommandListMemcpyBufferToBuffer()
	ErrorCode memcpyBufferToBuffer(
		Buffer& dstBuffer,
		uint64_t dstBufferOffsetBytes,
		Buffer& srcBuffer,
		uint64_t srcBufferOffsetBytes,
		uint64_t numBytes) noexcept;

	// See zgCommandListMemcpyToTexture()
	ErrorCode memcpyToTexture(
		Texture2D& dstTexture,
		uint32_t dstTextureMipLevel,
		const ZgImageViewConstCpu& srcImageCpu,
		Buffer& tempUploadBuffer) noexcept;

	// See zgCommandListEnableQueueTransitionBuffer()
	ErrorCode enableQueueTransition(Buffer& buffer) noexcept;

	// See zgCommandListEnableQueueTransitionTexture()
	ErrorCode enableQueueTransition(Texture2D& texture) noexcept;

	// See zgCommandListSetPushConstant()
	ErrorCode setPushConstant(
		uint32_t shaderRegister, const void* data, uint32_t dataSizeInBytes) noexcept;

	// See zgCommandListSetPipelineBindings()
	ErrorCode setPipelineBindings(const PipelineBindings& bindings) noexcept;

	// See zgCommandListSetPipelineRender()
	ErrorCode setPipeline(PipelineRender& pipeline) noexcept;

	// See zgCommandListSetFramebuffer()
	ErrorCode setFramebuffer(
		Framebuffer& framebuffer,
		const ZgFramebufferRect* optionalViewport = nullptr,
		const ZgFramebufferRect* optionalScissor = nullptr) noexcept;

	// See zgCommandListSetFramebufferViewport()
	ErrorCode setFramebufferViewport(
		const ZgFramebufferRect& viewport) noexcept;

	// See zgCommandListSetFramebufferScissor()
	ErrorCode setFramebufferScissor(
		const ZgFramebufferRect& scissor) noexcept;

	// See zgCommandListClearFramebufferOptimal()
	ErrorCode clearFramebufferOptimal() noexcept;

	// See zgCommandListClearRenderTargets()
	ErrorCode clearRenderTargets(float red, float green, float blue, float alpha) noexcept;

	// See zgCommandListClearDepthBuffer()
	ErrorCode clearDepthBuffer(float depth) noexcept;

	// See zgCommandListSetIndexBuffer()
	ErrorCode setIndexBuffer(Buffer& indexBuffer, ZgIndexBufferType type) noexcept;

	// See zgCommandListSetVertexBuffer()
	ErrorCode setVertexBuffer(uint32_t vertexBufferSlot, Buffer& vertexBuffer) noexcept;

	// See zgCommandListDrawTriangles()
	ErrorCode drawTriangles(uint32_t startVertexIndex, uint32_t numVertices) noexcept;

	// See zgCommandListDrawTrianglesIndexed()
	ErrorCode drawTrianglesIndexed(uint32_t startIndex, uint32_t numTriangles) noexcept;
};


// Transformation and projection matrices
// ------------------------------------------------------------------------------------------------

// These are some helper functions to generate the standard transform and projection matrices you
// typically want to use with ZeroG.
//
// The inclusion of these might seem a bit out of place compared to the other stuff here, however
// when looking around I see quite a bit of confusion regarding these matrices. I figure I will save
// myself and others quite a bit of time by providing reasonable defaults that should cover most use
// cases.
//
// All matrices returned are 4x4 row-major matrices (i.e. column vectors). If passed directly into
// HLSL the "float4x4" primitive must be marked "row_major", otherwise the matrix will get
// transposed during the transfer and you will not get the results you expect.
//
// The createViewMatrix() function creates a view matrix similar to the one typically used in OpenGL.
// In other words, right-handed coordinate system with x to the right, y up and z towards the camera
// (negative z into the scene). This is the kind of view matrix that is expected for all the
// projection matrices here.
//
// The are a couple of variants of the projection matrices, normal, "reverse" and "infinite".
//
// Reverse simply means that it uses reversed z (i.e. 1.0 is closest to camera, 0.0 is furthest away).
// This can greatly improve the precision of the depth buffer, see:
// * https://developer.nvidia.com/content/depth-precision-visualized
// * http://dev.theomader.com/depth-precision/
// * https://mynameismjp.wordpress.com/2010/03/22/attack-of-the-depth-buffer/
// Of course, if you are using reverse projection you must also change your depth function from
// "ZG_DEPTH_FUNC_LESS" to "ZG_DEPTH_FUNC_GREATER".
//
// Infinite means that the far plane is at infinity instead of at a fixed distance away from the camera.
// Somewhat counter intuitively, this does not reduce the precision of the depth buffer all that much.
// Because the depth buffer is logarithmic, mainly the distance to the near plane affects precision.
// Setting the far plane to infinity gives you one less thing to think about and simplifies the
// actual projection matrix a bit.
//
// If unsure I would recommend starting out with the basic createPerspectiveProjection() and then
// switching to createPerspectiveProjectionReverseInfinite() when feeling more confident.

void createViewMatrix(
	float rowMajorMatrixOut[16],
	const float origin[3],
	const float dir[3],
	const float up[3]) noexcept;

void createPerspectiveProjection(
	float rowMajorMatrixOut[16],
	float vertFovDegs,
	float aspect,
	float near,
	float far) noexcept;

void createPerspectiveProjectionInfinite(
	float rowMajorMatrixOut[16],
	float vertFovDegs,
	float aspect,
	float near) noexcept;

void createPerspectiveProjectionReverse(
	float rowMajorMatrixOut[16],
	float vertFovDegs,
	float aspect,
	float near,
	float far) noexcept;

void createPerspectiveProjectionReverseInfinite(
	float rowMajorMatrixOut[16],
	float vertFovDegs,
	float aspect,
	float near) noexcept;

void createOrthographicProjection(
	float rowMajorMatrixOut[16],
	float width,
	float height,
	float near,
	float far) noexcept;

void createOrthographicProjectionReverse(
	float rowMajorMatrixOut[16],
	float width,
	float height,
	float near,
	float far) noexcept;

} // namespace zg
