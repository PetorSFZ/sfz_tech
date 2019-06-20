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

#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <SDL.h>
#include <SDL_syswm.h>

#include "ZeroG-cpp.hpp"

#include "Cube.hpp"
#include "SampleCommon.hpp"

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <direct.h>
#endif

// Settings
// ------------------------------------------------------------------------------------------------

constexpr bool DEBUG_MODE = true;

// Helpers
// ------------------------------------------------------------------------------------------------

struct Vertex {
	float position[3];
	float normal[3];
	float texcoord[2];
};
static_assert(sizeof(Vertex) == sizeof(float) * 8, "Vertex is padded");

// A simple helper function that allocates a heap and a buffer that covers the entirety of it.
// In practice you want to have multiple buffers per heap and use some sort of allocation scheme,
// but this is good enough for this sample.
static void allocateMemoryHeapAndBuffer(
	zg::MemoryHeap& heapOut,
	zg::Buffer& bufferOut,
	ZgMemoryType memoryType,
	uint64_t numBytes) noexcept
{
	// Create heap
	ZgMemoryHeapCreateInfo heapInfo = {};
	heapInfo.memoryType = memoryType;
	heapInfo.sizeInBytes = numBytes;

	CHECK_ZG heapOut.create(heapInfo);

	// Create buffer
	ZgBufferCreateInfo bufferInfo = {};
	bufferInfo.offsetInBytes = 0;
	bufferInfo.sizeInBytes = numBytes;
	
	CHECK_ZG heapOut.bufferCreate(bufferOut, bufferInfo);
}

// A simple helper function that allocates and copies data to a device buffer In practice you will
// likely want to do something smarter.
static void createDeviceBufferSimpleBlocking(
	zg::CommandQueue& queue,
	zg::Buffer& bufferOut,
	zg::MemoryHeap& memoryHeapOut,
	const void* data,
	uint64_t numBytes,
	uint64_t bufferSizeBytes = 0) noexcept
{
	// Create temporary upload buffer (accessible from CPU)
	zg::MemoryHeap uploadHeap;
	zg::Buffer uploadBuffer;
	allocateMemoryHeapAndBuffer(uploadHeap, uploadBuffer,
		ZG_MEMORY_TYPE_UPLOAD, bufferSizeBytes != 0 ? bufferSizeBytes : numBytes);

	// Copy cube vertices to upload buffer
	CHECK_ZG uploadBuffer.memcpyTo(0, data, numBytes);

	// Create device buffer
	zg::MemoryHeap deviceHeap;
	zg::Buffer deviceBuffer;
	allocateMemoryHeapAndBuffer(deviceHeap, deviceBuffer,
		ZG_MEMORY_TYPE_DEVICE, bufferSizeBytes != 0 ? bufferSizeBytes : numBytes);

	// Copy to the device buffer
	zg::CommandList commandList;
	CHECK_ZG queue.beginCommandListRecording(commandList);
	CHECK_ZG commandList.memcpyBufferToBuffer(deviceBuffer, 0, uploadBuffer, 0, numBytes);
	CHECK_ZG queue.executeCommandList(commandList);
	CHECK_ZG queue.flush();

	bufferOut = std::move(deviceBuffer);
	memoryHeapOut = std::move(deviceHeap);
}

using time_point = std::chrono::high_resolution_clock::time_point;

// Helper functions that calculates the time in seconds since the last call
static float calculateDelta(time_point& previousTime) noexcept
{
	time_point currentTime = std::chrono::high_resolution_clock::now();

	using FloatSecond = std::chrono::duration<float>;
	float delta = std::chrono::duration_cast<FloatSecond>(currentTime - previousTime).count();

	previousTime = currentTime;
	return delta;
}

static ZgImageViewConstCpu allocateRgbaTex(uint32_t width, uint32_t height) noexcept
{
	uint8_t* data = new uint8_t[width * height * 4];
	for (uint32_t y = 0; y < height; y++) {
		for (uint32_t x = 0; x < width; x++) {
			uint8_t* pixelPtr = data + y * width * 4 + x * 4;
			if ((y % 16) < 8) {
				pixelPtr[0] = 255;
				pixelPtr[1] = 0;
				pixelPtr[2] = 0;
				pixelPtr[3] = 255;
			}
			else {
				pixelPtr[0] = 255;
				pixelPtr[1] = 255;
				pixelPtr[2] = 255;
				pixelPtr[3] = 255;
			}
		}
	}

	// Create an image view of the data
	ZgImageViewConstCpu imageView = {};
	imageView.format = ZG_TEXTURE_2D_FORMAT_RGBA_U8;
	imageView.data = data;
	imageView.width = width;
	imageView.height = height;
	imageView.pitchInBytes = width * 4;

	return imageView;
}

static ZgImageViewConstCpu copyDownsample(
	const uint8_t* srcRgbaTex, uint32_t srcWidth, uint32_t srcHeight) noexcept
{
	assert((srcWidth % 2) == 0);
	assert((srcHeight % 2) == 0);
	uint32_t dstWidth = srcWidth / 2;
	uint32_t dstHeight = srcHeight / 2;
	uint8_t* dstImg = new uint8_t[dstWidth * dstHeight * 4];
	for (uint32_t y = 0; y < dstHeight; y++) {
		for (uint32_t x = 0; x < dstWidth; x++) {

			uint8_t* dstPixelPtr = dstImg + y * dstWidth * 4 + x * 4;
			const uint8_t* srcPixelPtrRow0 = srcRgbaTex + (y * 2) * srcWidth * 4 + (x * 2) * 4;
			const uint8_t* srcPixelPtrRow1 = srcRgbaTex + ((y * 2) + 1) * srcWidth * 4 + (x * 2) * 4;

			for (uint32_t i = 0; i < 4; i++) {
				uint32_t inputSum =
					uint32_t(srcPixelPtrRow0[(0 * 4) + i]) + uint32_t(srcPixelPtrRow0[(1 * 4) + i]) +
					uint32_t(srcPixelPtrRow1[(0 * 4) + i]) + uint32_t(srcPixelPtrRow1[(1 * 4) + i]);
				dstPixelPtr[i] = uint8_t(inputSum / 4);
			}
		}
	}

	// Create an image view of the data
	ZgImageViewConstCpu imageView = {};
	imageView.format = ZG_TEXTURE_2D_FORMAT_RGBA_U8;
	imageView.data = dstImg;
	imageView.width = dstWidth;
	imageView.height = dstHeight;
	imageView.pitchInBytes = dstWidth * 4;

	return imageView;
}

static size_t readBinaryFile(const char* path, uint8_t* dataOut, size_t maxNumBytes) noexcept
{
	// Open file
	std::FILE* file = std::fopen(path, "rb");
	if (file == NULL) return 0;

	// Read the file into memory
	uint8_t buffer[BUFSIZ];
	size_t readSize;
	size_t currOffs = 0;
	while ((readSize = std::fread(buffer, 1, BUFSIZ, file)) > 0) {

		// Check if memory has enough space left
		if ((currOffs + readSize) > maxNumBytes) {
			std::fclose(file);
			std::memcpy(dataOut + currOffs, buffer, maxNumBytes - currOffs);
			return -2;
		}

		std::memcpy(dataOut + currOffs, buffer, readSize);
		currOffs += readSize;
	}

	std::fclose(file);
	return currOffs;
}

// Main
// ------------------------------------------------------------------------------------------------

// This wrapper is only here to ensure the SDL2 Window is not destroyed before all the ZeroG
// objects which uses destructors (through the C++ wrapper).
static void realMain(SDL_Window* window) noexcept
{
	// Print compiled and linked version of ZeroG
	printf("Compiled API version of ZeroG: %u, linked version: %u\n\n",
		zg::Context::compiledApiVersion(), zg::Context::linkedApiVersion());

	// Create ZeroG context
	ZgContextInitSettings initSettings = {};
	initSettings.backend = ZG_BACKEND_D3D12;
	initSettings.width = 512;
	initSettings.height = 512;
	initSettings.debugMode = DEBUG_MODE ? ZG_TRUE : ZG_FALSE;
	initSettings.nativeWindowHandle = getNativeWindowHandle(window);
	zg::Context zgCtx;
	CHECK_ZG zgCtx.init(initSettings);


	// Get the command queue
	zg::CommandQueue commandQueue;
	CHECK_ZG zgCtx.getCommandQueueGraphicsPresent(commandQueue);


	// Create a rendering pipeline
	ZgPipelineRenderingCreateInfoCommon pipelineInfoCommon = {};

	pipelineInfoCommon.vertexShaderEntry = "VSMain";
	pipelineInfoCommon.pixelShaderEntry = "PSMain";

	pipelineInfoCommon.numVertexAttributes = 3;

	// "position"
	pipelineInfoCommon.vertexAttributes[0].location = 0;
	pipelineInfoCommon.vertexAttributes[0].vertexBufferSlot = 0;
	pipelineInfoCommon.vertexAttributes[0].offsetToFirstElementInBytes = offsetof(Vertex, position);
	pipelineInfoCommon.vertexAttributes[0].type = ZG_VERTEX_ATTRIBUTE_F32_3;

	// "normal"
	pipelineInfoCommon.vertexAttributes[1].location = 1;
	pipelineInfoCommon.vertexAttributes[1].vertexBufferSlot = 0;
	pipelineInfoCommon.vertexAttributes[1].offsetToFirstElementInBytes = offsetof(Vertex, normal);
	pipelineInfoCommon.vertexAttributes[1].type = ZG_VERTEX_ATTRIBUTE_F32_3;

	// "texcoord"
	pipelineInfoCommon.vertexAttributes[2].location = 2;
	pipelineInfoCommon.vertexAttributes[2].vertexBufferSlot = 0;
	pipelineInfoCommon.vertexAttributes[2].offsetToFirstElementInBytes = offsetof(Vertex, texcoord);
	pipelineInfoCommon.vertexAttributes[2].type = ZG_VERTEX_ATTRIBUTE_F32_2;

	pipelineInfoCommon.numVertexBufferSlots = 1;
	pipelineInfoCommon.vertexBufferStridesBytes[0] = sizeof(Vertex);

	pipelineInfoCommon.numPushConstants = 1;
	pipelineInfoCommon.pushConstantRegisters[0] = 0;

	pipelineInfoCommon.numSamplers = 1;
	pipelineInfoCommon.samplers[0].samplingMode = ZG_SAMPLING_MODE_ANISOTROPIC;
	pipelineInfoCommon.samplers[0].wrappingModeU = ZG_WRAPPING_MODE_CLAMP;
	pipelineInfoCommon.samplers[0].wrappingModeV = ZG_WRAPPING_MODE_CLAMP;
	pipelineInfoCommon.samplers[0].mipLodBias = 0.0f;

	// SPIRV File variant
	zg::PipelineRendering pipeline;
	{
		ZgPipelineRenderingCreateInfoFileSPIRV pipelineInfoFileSpirv = {};
		pipelineInfoFileSpirv.common = pipelineInfoCommon;
		pipelineInfoFileSpirv.vertexShaderPath = "res/Sample-1/test_vs.spv";
		pipelineInfoFileSpirv.pixelShaderPath = "res/Sample-1/test_ps.spv";
		CHECK_ZG pipeline.createFromFileSPIRV(pipelineInfoFileSpirv);
	}

	// HLSL File variant
	/*zg::PipelineRendering pipeline;
	{
		ZgPipelineRenderingCreateInfoFileHLSL pipelineInfoFileHlsl = {};
		pipelineInfoFileHlsl.common = pipelineInfoCommon;
		pipelineInfoFileHlsl.vertexShaderPath = "res/Sample-1/test.hlsl";
		pipelineInfoFileHlsl.pixelShaderPath = "res/Sample-1/test.hlsl";
		pipelineInfoFileHlsl.shaderModel = ZG_SHADER_MODEL_6_0;
		pipelineInfoFileHlsl.dxcCompilerFlags[0] = "-Zi";
		pipelineInfoFileHlsl.dxcCompilerFlags[1] = "-O3";
		CHECK_ZG pipeline.createFromFileHLSL(pipelineInfoFileHlsl);
	}*/

	// HLSL source variant
	/*zg::PipelineRendering pipeline;
	{
		char hlslSource[2048] = {};
		size_t numRead = readBinaryFile("res/Sample-1/test.hlsl", (uint8_t*)hlslSource, 2048);
		assert(0 < numRead && numRead < 2048);
		ZgPipelineRenderingCreateInfoSourceHLSL pipelineInfoSrcHlsl = {};
		pipelineInfoSrcHlsl.common = pipelineInfoCommon;
		pipelineInfoSrcHlsl.vertexShaderSrc = hlslSource;
		pipelineInfoSrcHlsl.pixelShaderSrc = hlslSource;
		pipelineInfoSrcHlsl.shaderModel = ZG_SHADER_MODEL_6_0;
		pipelineInfoSrcHlsl.dxcCompilerFlags[0] = "-Zi";
		pipelineInfoSrcHlsl.dxcCompilerFlags[1] = "-O3";
		CHECK_ZG pipeline.createFromSourceHLSL(pipelineInfoSrcHlsl);
	}*/


	// Create a vertex buffer containing a Cube
	Vertex cubeVertices[CUBE_NUM_VERTICES] = {};
	for (uint32_t i = 0; i < CUBE_NUM_VERTICES; i++) {
		Vertex& v = cubeVertices[i];
		v.position[0] = CUBE_POSITIONS[i * 3 + 0];
		v.position[1] = CUBE_POSITIONS[i * 3 + 1];
		v.position[2] = CUBE_POSITIONS[i * 3 + 2];
		v.normal[0] = CUBE_NORMALS[i * 3 + 0];
		v.normal[1] = CUBE_NORMALS[i * 3 + 1];
		v.normal[2] = CUBE_NORMALS[i * 3 + 2];
		v.texcoord[0] = CUBE_TEXCOORDS[i * 2 + 0];
		v.texcoord[1] = CUBE_TEXCOORDS[i * 2 + 1];
	}

	zg::Buffer cubeVertexBufferDevice;
	zg::MemoryHeap cubeVertexMemoryHeapDevice;
	createDeviceBufferSimpleBlocking(commandQueue, cubeVertexBufferDevice,
		cubeVertexMemoryHeapDevice, cubeVertices, sizeof(Vertex) * CUBE_NUM_VERTICES);

	// Create a index buffer for the cube's vertices
	zg::Buffer cubeIndexBufferDevice;
	zg::MemoryHeap cubeIndexMemoryHeapDevice;
	createDeviceBufferSimpleBlocking(commandQueue, cubeIndexBufferDevice,
		cubeIndexMemoryHeapDevice, CUBE_INDICES, sizeof(uint32_t) * CUBE_NUM_INDICES);

	
	// Create a constant buffer
	Vector offsets;
	offsets.x = 0.0f;
	zg::Buffer constBufferDevice;
	zg::MemoryHeap constBufferMemoryHeapDevice;
	createDeviceBufferSimpleBlocking(commandQueue, constBufferDevice,
		constBufferMemoryHeapDevice, &offsets, sizeof(Vector), 256);


	// Create texture heap
	ZgTextureHeapCreateInfo textureHeapInfo = {};
	textureHeapInfo.sizeInBytes = 64 * 1024 * 1024; // 64 MiB should be enough for anyone

	ZgTextureHeap* textureHeap = nullptr;
	CHECK_ZG zgTextureHeapCreate(&textureHeap, &textureHeapInfo);

	// Create a texture
	ZgTexture2DCreateInfo textureCreateInfo = {};
	textureCreateInfo.format = ZG_TEXTURE_2D_FORMAT_RGBA_U8;
	textureCreateInfo.normalized = ZG_FALSE;
	textureCreateInfo.width = 256;
	textureCreateInfo.height = 256;
	textureCreateInfo.numMipmaps = 4;

	ZgTexture2DAllocationInfo textureAllocInfo = {};
	CHECK_ZG zgTextureHeapTexture2DGetAllocationInfo(
		textureHeap, &textureAllocInfo, &textureCreateInfo);
	
	textureCreateInfo.offsetInBytes = 0;
	textureCreateInfo.sizeInBytes = textureAllocInfo.sizeInBytes;

	ZgTexture2D* texture = nullptr;
	CHECK_ZG zgTextureHeapTexture2DCreate(textureHeap, &texture, &textureCreateInfo);

	
	// Fill texture with some random data
	{
		// Allocates images
		ZgImageViewConstCpu imageLvl0 = allocateRgbaTex(256, 256);
		ZgImageViewConstCpu imageLvl1 =
			copyDownsample(imageLvl0.data, imageLvl0.width, imageLvl0.height);
		ZgImageViewConstCpu imageLvl2 =
			copyDownsample(imageLvl1.data, imageLvl1.width, imageLvl1.height);
		ZgImageViewConstCpu imageLvl3 =
			copyDownsample(imageLvl2.data, imageLvl2.width, imageLvl2.height);


		// Create temporary upload buffer (accessible from CPU)
		zg::MemoryHeap uploadHeapLvl0;
		zg::Buffer uploadBufferLvl0;
		allocateMemoryHeapAndBuffer(uploadHeapLvl0, uploadBufferLvl0,
			ZG_MEMORY_TYPE_UPLOAD, textureAllocInfo.sizeInBytes);

		zg::MemoryHeap uploadHeapLvl1;
		zg::Buffer uploadBufferLvl1;
		allocateMemoryHeapAndBuffer(uploadHeapLvl1, uploadBufferLvl1,
			ZG_MEMORY_TYPE_UPLOAD, textureAllocInfo.sizeInBytes);

		zg::MemoryHeap uploadHeapLvl2;
		zg::Buffer uploadBufferLvl2;
		allocateMemoryHeapAndBuffer(uploadHeapLvl2, uploadBufferLvl2,
			ZG_MEMORY_TYPE_UPLOAD, textureAllocInfo.sizeInBytes);

		zg::MemoryHeap uploadHeapLvl3;
		zg::Buffer uploadBufferLvl3;
		allocateMemoryHeapAndBuffer(uploadHeapLvl3, uploadBufferLvl3,
			ZG_MEMORY_TYPE_UPLOAD, textureAllocInfo.sizeInBytes);

		// Copy to the texture
		zg::CommandList commandList;
		CHECK_ZG commandQueue.beginCommandListRecording(commandList);
		CHECK_ZG zgCommandListMemcpyToTexture(commandList.commandList, texture, 0, &imageLvl0, uploadBufferLvl0.buffer);
		CHECK_ZG zgCommandListMemcpyToTexture(commandList.commandList, texture, 1, &imageLvl1, uploadBufferLvl1.buffer);
		CHECK_ZG zgCommandListMemcpyToTexture(commandList.commandList, texture, 2, &imageLvl2, uploadBufferLvl2.buffer);
		CHECK_ZG zgCommandListMemcpyToTexture(commandList.commandList, texture, 3, &imageLvl3, uploadBufferLvl3.buffer);
		CHECK_ZG commandQueue.executeCommandList(commandList);
		CHECK_ZG commandQueue.flush();

		// Free images
		delete[] imageLvl0.data;
		delete[] imageLvl1.data;
		delete[] imageLvl2.data;
		delete[] imageLvl3.data;
	}
	


	// Run our main loop
	time_point previousTimePoint;
	calculateDelta(previousTimePoint);
	float timeSinceStart = 0.0f;
	bool running = true;
	while (running) {

		// Query SDL events, loop wrapped in a lambda so we can continue to next iteration of main
		// loop. "return false;" == continue to next iteration
		if (![&]() -> bool {
			SDL_Event event = {};
			while (SDL_PollEvent(&event) != 0) {
				switch (event.type) {

				case SDL_QUIT:
					running = false;
					return false;

				case SDL_KEYUP:
					//if (event.key.keysym.sym == SDLK_ESCAPE) {
						running = false;
						return false;
					//}
					break;
				}
			}
			return true;
		}()) continue;

		// Update time since start
		timeSinceStart += calculateDelta(previousTimePoint);

		// Query drawable width and height and update ZeroG context
		int width = 0;
		int height = 0;
		SDL_GL_GetDrawableSize(window, &width, &height);
		CHECK_ZG zgCtx.resize(uint32_t(width), uint32_t(height));

		// Create view and projection matrices
		float vertFovDeg = 40.0f;
		float aspectRatio = float(width) / float(height);
		Vector origin = Vector(
			std::cos(timeSinceStart) * 5.0f,
			std::sin(timeSinceStart * 0.75f) + 1.5f,
			std::sin(timeSinceStart) * 5.0f);
		Matrix viewMatrix = createViewMatrix(
			origin, -origin, Vector(0.0f, 1.0f, 0.0f));
		Matrix projMatrix = createProjectionMatrix(vertFovDeg, aspectRatio, 0.01f, 10.0f);

		// Begin frame
		ZgFramebuffer* framebuffer = nullptr;
		CHECK_ZG zgContextBeginFrame(&framebuffer);

		// Get a command list
		zg::CommandList commandList;
		CHECK_ZG commandQueue.beginCommandListRecording(commandList);
		
		// Set framebuffer and clear it
		ZgCommandListSetFramebufferInfo framebufferInfo = {};
		framebufferInfo.framebuffer = framebuffer;
		CHECK_ZG zgCommandListSetFramebuffer(commandList.commandList, &framebufferInfo);
		CHECK_ZG zgCommandListClearFramebuffer(commandList.commandList, 0.2f, 0.2f, 0.3f, 1.0f);
		CHECK_ZG zgCommandListClearDepthBuffer(commandList.commandList, 1.0f);

		// Set pipeline
		CHECK_ZG commandList.setPipeline(pipeline);

		// Set pipeline bindings
		ZgPipelineBindings bindings = {};
		bindings.numConstantBuffers = 1;
		bindings.constantBuffers[0].shaderRegister = 1;
		bindings.constantBuffers[0].buffer = constBufferDevice.buffer;
		bindings.numTextures = 1;
		bindings.textures[0].textureRegister = 0;
		bindings.textures[0].texture = texture;
		CHECK_ZG zgCommandListSetPipelineBindings(commandList.commandList, &bindings);

		// Lambda to batch a call to render a cube with a specific transform
		auto batchCubeRender = [&](Vector offset) {

			// Calculate transforms to send to shader
			Matrix modelMatrix = createIdentityMatrix();
			modelMatrix.m[3] = offset.x;
			modelMatrix.m[7] = offset.y;
			modelMatrix.m[11] = offset.z;
			struct Transforms {
				Matrix mvpMatrix;
				Matrix normalMatrix;
			} transforms;
			transforms.mvpMatrix = projMatrix * viewMatrix * modelMatrix;
			transforms.normalMatrix = inverse(transpose(viewMatrix * modelMatrix));

			// Send transforms to shader
			CHECK_ZG zgCommandListSetPushConstant(commandList.commandList, 0, &transforms, sizeof(Transforms));

			// Draw cube
			CHECK_ZG zgCommandListDrawTrianglesIndexed(commandList.commandList, 0, CUBE_NUM_INDICES / 3);
		};

		// Set Cube's vertex and index buffer
		CHECK_ZG zgCommandListSetIndexBuffer(
			commandList.commandList, cubeIndexBufferDevice.buffer, ZG_INDEX_BUFFER_TYPE_UINT32);
		CHECK_ZG zgCommandListSetVertexBuffer(commandList.commandList, 0, cubeVertexBufferDevice.buffer);
		
		// Batch some cubes
		batchCubeRender(Vector(0.0f, 0.0f, 0.0f));

		batchCubeRender(Vector(-1.5f, -1.5f, -1.5f));
		batchCubeRender(Vector(-1.5f, -1.5f, 0.0f));
		batchCubeRender(Vector(-1.5f, -1.5f, 1.5f));

		batchCubeRender(Vector(0.0f, -1.5f, -1.5f));
		batchCubeRender(Vector(0.0f, -1.5f, 0.0f));
		batchCubeRender(Vector(0.0f, -1.5f, 1.5f));

		batchCubeRender(Vector(1.5f, -1.5f, -1.5f));
		batchCubeRender(Vector(1.5f, -1.5f, 0.0f));
		batchCubeRender(Vector(1.5f, -1.5f, 1.5f));
		
		// Execute command list
		CHECK_ZG commandQueue.executeCommandList(commandList);

		// Finish frame
		CHECK_ZG zgCtx.finishFrame();
	}

	// Flush command queue so nothing is running when we start releasing resources
	CHECK_ZG commandQueue.flush();

	// Release ZeroG resources
	CHECK_ZG zgMemoryHeapTexture2DRelease(textureHeap, texture);
	CHECK_ZG zgTextureHeapRelease(textureHeap);
}

int main(int argc, char* argv[])
{
	(void)argc;
	(void)argv;

	// Enable hi-dpi awareness on Windows
#ifdef _WIN32
	SetProcessDPIAware();

	// Set current working directory to SDL_GetBasePath()
	char* basePath = SDL_GetBasePath();
	_chdir(basePath);
	SDL_free(basePath);
#endif

	// Initialize SDL2 and create a window
	SDL_Window* window = initializeSdl2CreateWindow("ZeroG - Sample1");

	// Runs the real main function
	realMain(window);

	// Cleanup SDL2
	cleanupSdl2(window);

	return EXIT_SUCCESS;
}
