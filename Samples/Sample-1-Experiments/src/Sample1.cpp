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

#include <chrono>
#include <cstdlib>
#include <cstdio>

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
	ZgContext* context,
	ZgMemoryHeap*& heapOut,
	ZgBuffer*& bufferOut,
	ZgMemoryType memoryType,
	uint64_t numBytes) noexcept
{
	// Create heap
	ZgMemoryHeapCreateInfo heapInfo = {};
	heapInfo.memoryType = memoryType;
	heapInfo.sizeInBytes = numBytes;

	ZgMemoryHeap* heap = nullptr;
	CHECK_ZG zgMemoryHeapCreate(context, &heap, &heapInfo);

	// Create buffer
	ZgBufferCreateInfo bufferInfo = {};
	bufferInfo.offsetInBytes = 0;
	bufferInfo.sizeInBytes = numBytes;
	
	ZgBuffer* buffer = nullptr;
	CHECK_ZG zgMemoryHeapBufferCreate(heap, &buffer, &bufferInfo);

	heapOut = heap;
	bufferOut = buffer;
}

// A simple helper function that allocates and copies data to a device buffer In practice you will
// likely want to do something smarter.
static void createDeviceBufferSimpleBlocking(
	ZgContext* context,
	ZgCommandQueue* queue,
	ZgBuffer*& bufferOut,
	ZgMemoryHeap*& memoryHeapOut,
	const void* data,
	uint64_t numBytes,
	uint64_t bufferSizeBytes = 0) noexcept
{
	// Create temporary upload buffer (accessible from CPU)
	ZgMemoryHeap* uploadHeap = nullptr;
	ZgBuffer* uploadBuffer = nullptr;
	allocateMemoryHeapAndBuffer(context, uploadHeap, uploadBuffer,
		ZG_MEMORY_TYPE_UPLOAD, bufferSizeBytes != 0 ? bufferSizeBytes : numBytes);

	// Copy cube vertices to upload buffer
	CHECK_ZG zgBufferMemcpyTo(context, uploadBuffer, 0, data, numBytes);

	// Create device buffer
	ZgMemoryHeap* deviceHeap = nullptr;
	ZgBuffer* deviceBuffer = nullptr;
	allocateMemoryHeapAndBuffer(context, deviceHeap, deviceBuffer,
		ZG_MEMORY_TYPE_DEVICE, bufferSizeBytes != 0 ? bufferSizeBytes : numBytes);

	// Copy to the device buffer
	ZgCommandList* commandList = nullptr;
	CHECK_ZG zgCommandQueueBeginCommandListRecording(queue, &commandList);
	CHECK_ZG zgCommandListMemcpyBufferToBuffer(
		commandList, deviceBuffer, 0, uploadBuffer, 0, numBytes);
	CHECK_ZG zgCommandQueueExecuteCommandList(queue, commandList);
	CHECK_ZG zgCommandQueueFlush(queue);

	// Release upload heap and buffer
	CHECK_ZG zgMemoryHeapBufferRelease(uploadHeap, uploadBuffer);
	CHECK_ZG zgMemoryHeapRelease(context, uploadHeap);

	bufferOut = deviceBuffer;
	memoryHeapOut = deviceHeap;
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

// Main
// ------------------------------------------------------------------------------------------------

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


	// Create ZeroG context
	ZgContextInitSettings initSettings = {};
	initSettings.backend = ZG_BACKEND_D3D12;
	initSettings.width = 512;
	initSettings.height = 512;
	initSettings.debugMode = DEBUG_MODE ? ZG_TRUE : ZG_FALSE;
	initSettings.nativeWindowHandle = getNativeWindowHandle(window);
	zg::Context ctx;
	CHECK_ZG ctx.init(initSettings);


	// Get the command queue
	ZgCommandQueue* commandQueue = nullptr;
	CHECK_ZG zgContextGeCommandQueueGraphicsPresent(ctx.mContext, &commandQueue);


	// Create a rendering pipeline
	ZgPipelineRenderingCreateInfo pipelineInfo = {};
	pipelineInfo.vertexShaderPath = "res/Sample-1/test.hlsl";
	pipelineInfo.vertexShaderEntry = "VSMain";

	pipelineInfo.pixelShaderPath = "res/Sample-1/test.hlsl";
	pipelineInfo.pixelShaderEntry = "PSMain";

	pipelineInfo.shaderVersion = ZG_SHADER_MODEL_6_2;
	pipelineInfo.dxcCompilerFlags[0] = "-Zi";
	pipelineInfo.dxcCompilerFlags[1] = "-O4";

	pipelineInfo.numVertexAttributes = 3;

	// "position"
	pipelineInfo.vertexAttributes[0].location = 0;
	pipelineInfo.vertexAttributes[0].vertexBufferSlot = 0;
	pipelineInfo.vertexAttributes[0].offsetToFirstElementInBytes = offsetof(Vertex, position);
	pipelineInfo.vertexAttributes[0].type = ZG_VERTEX_ATTRIBUTE_F32_3;

	// "normal"
	pipelineInfo.vertexAttributes[1].location = 1;
	pipelineInfo.vertexAttributes[1].vertexBufferSlot = 0;
	pipelineInfo.vertexAttributes[1].offsetToFirstElementInBytes = offsetof(Vertex, normal);
	pipelineInfo.vertexAttributes[1].type = ZG_VERTEX_ATTRIBUTE_F32_3;

	// "texcoord"
	pipelineInfo.vertexAttributes[2].location = 2;
	pipelineInfo.vertexAttributes[2].vertexBufferSlot = 0;
	pipelineInfo.vertexAttributes[2].offsetToFirstElementInBytes = offsetof(Vertex, texcoord);
	pipelineInfo.vertexAttributes[2].type = ZG_VERTEX_ATTRIBUTE_F32_2;

	pipelineInfo.numVertexBufferSlots = 1;
	pipelineInfo.vertexBufferStridesBytes[0] = sizeof(Vertex);

	pipelineInfo.numPushConstants = 1;
	pipelineInfo.pushConstantRegisters[0] = 0;

	pipelineInfo.numSamplers = 1;
	pipelineInfo.samplers[0].samplingMode = ZG_SAMPLING_MODE_NEAREST;
	pipelineInfo.samplers[0].wrappingModeU = ZG_WRAPPING_MODE_CLAMP;
	pipelineInfo.samplers[0].wrappingModeV = ZG_WRAPPING_MODE_CLAMP;
	pipelineInfo.samplers[0].mipLodBias = 0.0f;

	ZgPipelineRendering* pipeline = nullptr;
	ZgPipelineRenderingSignature signature = {};
	CHECK_ZG zgPipelineRenderingCreate(ctx.mContext, &pipeline, &signature, &pipelineInfo);


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

	ZgBuffer* cubeVertexBufferDevice = nullptr;
	ZgMemoryHeap* cubeVertexMemoryHeapDevice = nullptr;
	createDeviceBufferSimpleBlocking(ctx.mContext, commandQueue, cubeVertexBufferDevice,
		cubeVertexMemoryHeapDevice, cubeVertices, sizeof(Vertex) * CUBE_NUM_VERTICES);

	// Create a index buffer for the cube's vertices
	ZgBuffer* cubeIndexBufferDevice = nullptr;
	ZgMemoryHeap* cubeIndexMemoryHeapDevice = nullptr;
	createDeviceBufferSimpleBlocking(ctx.mContext, commandQueue, cubeIndexBufferDevice,
		cubeIndexMemoryHeapDevice, CUBE_INDICES, sizeof(uint32_t) * CUBE_NUM_INDICES);

	
	// Create a constant buffer
	Vector offsets;
	offsets.x = 0.0f;
	ZgBuffer* constBufferDevice = nullptr;
	ZgMemoryHeap* constBufferMemoryHeapDevice = nullptr;
	createDeviceBufferSimpleBlocking(ctx.mContext, commandQueue, constBufferDevice,
		constBufferMemoryHeapDevice, &offsets, sizeof(Vector), 256);


	// Create texture heap
	ZgTextureHeapCreateInfo textureHeapInfo = {};
	textureHeapInfo.sizeInBytes = 64 * 1024 * 1024; // 64 MiB should be enough for anyone

	ZgTextureHeap* textureHeap = nullptr;
	CHECK_ZG zgTextureHeapCreate(ctx.mContext, &textureHeap, &textureHeapInfo);

	// Create a texture
	ZgTexture2DCreateInfo textureCreateInfo = {};
	textureCreateInfo.format = ZG_TEXTURE_2D_FORMAT_RGBA_U8;
	textureCreateInfo.normalized = ZG_FALSE;
	textureCreateInfo.width = 1024;
	textureCreateInfo.height = 1024;

	ZgTexture2DAllocationInfo textureAllocInfo = {};
	CHECK_ZG zgTextureHeapTexture2DGetAllocationInfo(
		textureHeap, &textureAllocInfo, &textureCreateInfo);
	
	textureCreateInfo.offsetInBytes = 0;
	textureCreateInfo.sizeInBytes = textureAllocInfo.sizeInBytes;

	ZgTexture2D* texture = nullptr;
	CHECK_ZG zgTextureHeapTexture2DCreate(textureHeap, &texture, &textureCreateInfo);


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
		CHECK_ZG zgContextResize(ctx.mContext, uint32_t(width), uint32_t(height));

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
		CHECK_ZG zgContextBeginFrame(ctx.mContext, &framebuffer);

		// Get a command list
		ZgCommandList* commandList = nullptr;
		CHECK_ZG zgCommandQueueBeginCommandListRecording(commandQueue, &commandList);

		// Set framebuffer and clear it
		ZgCommandListSetFramebufferInfo framebufferInfo = {};
		framebufferInfo.framebuffer = framebuffer;
		CHECK_ZG zgCommandListSetFramebuffer(commandList, &framebufferInfo);
		CHECK_ZG zgCommandListClearFramebuffer(commandList, 0.2f, 0.2f, 0.3f, 1.0f);
		CHECK_ZG zgCommandListClearDepthBuffer(commandList, 1.0f);

		// Set pipeline
		CHECK_ZG zgCommandListSetPipelineRendering(commandList, pipeline);

		// Set pipeline bindings
		ZgPipelineBindings bindings = {};
		bindings.numConstantBuffers = 1;
		bindings.constantBuffers[0].shaderRegister = 1;
		bindings.constantBuffers[0].buffer = constBufferDevice;
		bindings.numTextures = 1;
		bindings.textures[0].textureRegister = 0;
		bindings.textures[0].texture = texture;
		CHECK_ZG zgCommandListSetPipelineBindings(commandList, &bindings);

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
			CHECK_ZG zgCommandListSetPushConstant(commandList, 0, &transforms, sizeof(Transforms));

			// Draw cube
			CHECK_ZG zgCommandListDrawTrianglesIndexed(commandList, 0, CUBE_NUM_INDICES / 3);
		};

		// Set Cube's vertex and index buffer
		CHECK_ZG zgCommandListSetIndexBuffer(
			commandList, cubeIndexBufferDevice, ZG_INDEX_BUFFER_TYPE_UINT32);
		CHECK_ZG zgCommandListSetVertexBuffer(commandList, 0, cubeVertexBufferDevice);
		
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
		CHECK_ZG zgCommandQueueExecuteCommandList(commandQueue, commandList);

		// Finish frame
		CHECK_ZG zgContextFinishFrame(ctx.mContext);
	}

	// Flush command queue so nothing is running when we start releasing resources
	CHECK_ZG zgCommandQueueFlush(commandQueue);

	// Release ZeroG resources
	CHECK_ZG zgMemoryHeapTexture2DRelease(textureHeap, texture);
	CHECK_ZG zgTextureHeapRelease(ctx.mContext, textureHeap);

	CHECK_ZG zgMemoryHeapBufferRelease(cubeVertexMemoryHeapDevice, cubeVertexBufferDevice);
	CHECK_ZG zgMemoryHeapRelease(ctx.mContext, cubeVertexMemoryHeapDevice);

	CHECK_ZG zgMemoryHeapBufferRelease(cubeIndexMemoryHeapDevice, cubeIndexBufferDevice);
	CHECK_ZG zgMemoryHeapRelease(ctx.mContext, cubeIndexMemoryHeapDevice);

	CHECK_ZG zgMemoryHeapBufferRelease(constBufferMemoryHeapDevice, constBufferDevice);
	CHECK_ZG zgMemoryHeapRelease(ctx.mContext, constBufferMemoryHeapDevice);

	CHECK_ZG zgPipelineRenderingRelease(ctx.mContext, pipeline);

	// Destroy ZeroG context
	ctx.destroy();

	// Cleanup SDL2
	cleanupSdl2(window);

	return EXIT_SUCCESS;
}
