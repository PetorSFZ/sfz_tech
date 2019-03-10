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

// A simple helper function that allocates and copies data to a device buffer In practice you will
// likely want to do something smarter.
static ZgBuffer* createDeviceBufferSimpleBlocking(
	ZgContext* context,
	ZgCommandQueue* queue,
	const void* data,
	uint64_t numBytes,
	uint64_t bufferSizeBytes = 0) noexcept
{
	ZgBuffer* deviceBuffer = nullptr;

	// Create temporary upload buffer (accessible from CPU)
	ZgBufferCreateInfo bufferInfo = {};
	bufferInfo.sizeInBytes = bufferSizeBytes != 0 ? bufferSizeBytes : numBytes;
	bufferInfo.bufferMemoryType = ZG_BUFFER_MEMORY_TYPE_UPLOAD;

	ZgBuffer* uploadBuffer = nullptr;
	CHECK_ZG zgBufferCreate(context, &uploadBuffer, &bufferInfo);

	// Copy cube vertices to upload buffer
	CHECK_ZG zgBufferMemcpyTo(context, uploadBuffer, 0, data, numBytes);

	// Create device buffer
	bufferInfo.bufferMemoryType = ZG_BUFFER_MEMORY_TYPE_DEVICE;
	CHECK_ZG zgBufferCreate(context, &deviceBuffer, &bufferInfo);

	// Copy to the device buffer
	ZgCommandList* commandList = nullptr;
	CHECK_ZG zgCommandQueueBeginCommandListRecording(queue, &commandList);
	CHECK_ZG zgCommandListMemcpyBufferToBuffer(
		commandList, deviceBuffer, 0, uploadBuffer, 0, numBytes);
	CHECK_ZG zgCommandQueueExecuteCommandList(queue, commandList);
	CHECK_ZG zgCommandQueueFlush(queue);

	// Release upload buffer
	CHECK_ZG zgBufferRelease(context, uploadBuffer);

	return deviceBuffer;
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
	ZgBuffer* cubeVertexBufferDevice = createDeviceBufferSimpleBlocking(
		ctx.mContext, commandQueue, cubeVertices, sizeof(Vertex) * CUBE_NUM_VERTICES);

	// Create a index buffer for the cube's vertices
	ZgBuffer* cubeIndexBufferDevice = createDeviceBufferSimpleBlocking(
		ctx.mContext, commandQueue, CUBE_INDICES, sizeof(uint32_t) * CUBE_NUM_INDICES);

	
	// Create a constant buffer
	Vector offsets;
	offsets.x = 0.0f;
	ZgBuffer* constBufferDevice = createDeviceBufferSimpleBlocking(
		ctx.mContext, commandQueue, &offsets, sizeof(Vector), 256);


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
		ZgConstantBufferBindings constBufferBindings = {};
		constBufferBindings.numBindings = 1;
		constBufferBindings.bindings[0].shaderRegister = 1;
		constBufferBindings.bindings[0].buffer = constBufferDevice;
		CHECK_ZG zgCommandListBindConstantBuffers(commandList, &constBufferBindings);

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
	CHECK_ZG zgBufferRelease(ctx.mContext, cubeVertexBufferDevice);
	CHECK_ZG zgBufferRelease(ctx.mContext, cubeIndexBufferDevice);
	CHECK_ZG zgBufferRelease(ctx.mContext, constBufferDevice);
	CHECK_ZG zgPipelineRenderingRelease(ctx.mContext, pipeline);

	// Destroy ZeroG context
	ctx.destroy();

	// Cleanup SDL2
	cleanupSdl2(window);

	return EXIT_SUCCESS;
}
