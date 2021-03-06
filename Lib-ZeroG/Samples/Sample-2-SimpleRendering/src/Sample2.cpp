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

#include "Cube.hpp"
#include "SampleCommon.hpp"

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <direct.h>
#endif

// D3D12 Agility SDK exports
// ------------------------------------------------------------------------------------------------

#ifdef _WIN32

// The version of the Agility SDK we are using, see https://devblogs.microsoft.com/directx/directx12agility/
extern "C" { _declspec(dllexport) extern const uint32_t D3D12SDKVersion = 4; }

// Specifies that D3D12Core.dll will be available in a directory called D3D12 next to the exe.
extern "C" { _declspec(dllexport) extern const char* D3D12SDKPath = u8".\\D3D12\\"; }

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

static void createDeviceBufferSimpleBlocking(
	zg::CommandQueue& copyQueue,
	zg::Buffer& bufferOut,
	const void* data,
	uint64_t numBytes,
	uint64_t bufferSizeBytes = 0) noexcept
{
	// Create temporary upload buffer (accessible from CPU)
	zg::Buffer uploadBuffer;
	CHECK_ZG uploadBuffer.create(bufferSizeBytes != 0 ? bufferSizeBytes : numBytes, ZG_MEMORY_TYPE_UPLOAD);

	// Copy cube vertices to upload buffer
	CHECK_ZG uploadBuffer.memcpyUpload(0, data, numBytes);

	// Create device buffer
	zg::Buffer deviceBuffer;
	CHECK_ZG deviceBuffer.create(bufferSizeBytes != 0 ? bufferSizeBytes : numBytes, ZG_MEMORY_TYPE_DEVICE);

	// Copy to the device buffer
	zg::CommandList commandList;
	CHECK_ZG copyQueue.beginCommandListRecording(commandList);
	CHECK_ZG commandList.memcpyBufferToBuffer(deviceBuffer, 0, uploadBuffer, 0, numBytes);
	CHECK_ZG commandList.enableQueueTransition(deviceBuffer);
	CHECK_ZG copyQueue.executeCommandList(commandList);
	CHECK_ZG copyQueue.flush();

	bufferOut = std::move(deviceBuffer);
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
	imageView.format = ZG_TEXTURE_FORMAT_RGBA_U8_UNORM;
	imageView.data = data;
	imageView.width = width;
	imageView.height = height;
	imageView.pitchInBytes = width * 4;

	return imageView;
}

static ZgImageViewConstCpu copyDownsample(
	const void* srcRgbaTex, uint32_t srcWidth, uint32_t srcHeight) noexcept
{
	assert((srcWidth % 2) == 0);
	assert((srcHeight % 2) == 0);
	uint32_t dstWidth = srcWidth / 2;
	uint32_t dstHeight = srcHeight / 2;
	uint8_t* dstImg = new uint8_t[dstWidth * dstHeight * 4];
	for (uint32_t y = 0; y < dstHeight; y++) {
		for (uint32_t x = 0; x < dstWidth; x++) {

			uint8_t* dstPixelPtr = dstImg + y * dstWidth * 4 + x * 4;
			const uint8_t* srcPixelPtrRow0 = ((const uint8_t*)srcRgbaTex) + (y * 2) * srcWidth * 4 + (x * 2) * 4;
			const uint8_t* srcPixelPtrRow1 = ((const uint8_t*)srcRgbaTex) + ((y * 2) + 1) * srcWidth * 4 + (x * 2) * 4;

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
	imageView.format = ZG_TEXTURE_FORMAT_RGBA_U8_UNORM;
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
			return 0;
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
		ZG_COMPILED_API_VERSION, zgApiLinkedVersion());

	// Create ZeroG context
	ZgContextInitSettings initSettings = {};
	initSettings.d3d12.debugMode = DEBUG_MODE ? ZG_TRUE : ZG_FALSE;
	initSettings.vulkan.debugMode = DEBUG_MODE ? ZG_TRUE : ZG_FALSE;
	initSettings.width = 512;
	initSettings.height = 512;
	initSettings.nativeHandle = getNativeHandle(window);
	CHECK_ZG zgContextInit(&initSettings);

	// Get the command queues
	zg::CommandQueue presentQueue = zg::CommandQueue::getPresentQueue();
	zg::CommandQueue copyQueue = zg::CommandQueue::getCopyQueue();

	// Create profiler
	zg::Profiler profiler;
	{
		ZgProfilerCreateInfo createInfo = {};
		createInfo.maxNumMeasurements = 100;
		CHECK_ZG profiler.create(createInfo);
	}

	// Create a render pipeline
	zg::PipelineRender renderPipeline;
	{
		zg::PipelineRenderBuilder pipelineBuilder = zg::PipelineRenderBuilder()
			.addVertexAttribute(0, 0, ZG_VERTEX_ATTRIBUTE_F32_3, offsetof(Vertex, position))
			.addVertexAttribute(1, 0, ZG_VERTEX_ATTRIBUTE_F32_3, offsetof(Vertex, normal))
			.addVertexAttribute(2, 0, ZG_VERTEX_ATTRIBUTE_F32_2, offsetof(Vertex, texcoord))
			.addVertexBufferInfo(0, sizeof(Vertex))
			.addPushConstant(0)
			.addSampler(0, ZG_SAMPLING_MODE_ANISOTROPIC)
			.addRenderTarget(ZG_TEXTURE_FORMAT_RGBA_U8_UNORM)
			.setCullingEnabled(true)
			.setCullMode(false, false)
			.setBlendingEnabled(true)
			.setBlendFuncColor(ZG_BLEND_FUNC_ADD, ZG_BLEND_FACTOR_SRC_ALPHA, ZG_BLEND_FACTOR_SRC_INV_ALPHA)
			.setDepthTestEnabled(true)
			.setDepthFunc(ZG_DEPTH_FUNC_LESS);

		// HLSL file variant
		CHECK_ZG pipelineBuilder
			.addVertexShaderPath("VSMain", "res/Sample-2/test.hlsl")
			.addPixelShaderPath("PSMain", "res/Sample-2/test.hlsl")
			.buildFromFileHLSL(renderPipeline, ZG_SHADER_MODEL_6_0);

		// HLSL source variant
		/*char hlslSource[2048] = {};
		size_t numRead = readBinaryFile("res/Sample-2/test.hlsl", (uint8_t*)hlslSource, 2048);
		assert(0 < numRead && numRead < 2048);
		CHECK_ZG pipelineBuilder
			.addVertexShaderSource("VSMain", hlslSource)
			.addPixelShaderSource("PSMain", hlslSource)
			.buildFromSourceHLSL(renderPipeline, ZG_SHADER_MODEL_6_0);*/
	}
	if (!renderPipeline.valid()) return;

	// Create a compute pipeline
	zg::PipelineCompute textureModifyPipeline;
	CHECK_ZG zg::PipelineComputeBuilder()
		.addComputeShaderPath("mainCS", "res/Sample-2/texture_modify.hlsl")
		.buildFromFileHLSL(textureModifyPipeline);


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
	createDeviceBufferSimpleBlocking(copyQueue, cubeVertexBufferDevice,
		cubeVertices, sizeof(Vertex) * CUBE_NUM_VERTICES);

	// Create a index buffer for the cube's vertices
	zg::Buffer cubeIndexBufferDevice;
	createDeviceBufferSimpleBlocking(copyQueue, cubeIndexBufferDevice,
		CUBE_INDICES, sizeof(uint32_t) * CUBE_NUM_INDICES);

	// Create a constant buffer
	Vector offsets;
	offsets.x = 0.0f;
	zg::Buffer constBufferDevice;
	createDeviceBufferSimpleBlocking(copyQueue, constBufferDevice,
		&offsets, sizeof(Vector), 256);

	// Create a texture
	ZgTextureCreateInfo textureCreateInfo = {};
	textureCreateInfo.format = ZG_TEXTURE_FORMAT_RGBA_U8_UNORM;
	textureCreateInfo.allowUnorderedAccess = ZG_TRUE;
	textureCreateInfo.width = 256;
	textureCreateInfo.height = 256;
	textureCreateInfo.numMipmaps = 4;

	zg::Texture texture;
	CHECK_ZG texture.create(textureCreateInfo);

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
		zg::Buffer uploadBufferLvl0, uploadBufferLvl1, uploadBufferLvl2, uploadBufferLvl3;
		CHECK_ZG uploadBufferLvl0.create(texture.sizeInBytes(), ZG_MEMORY_TYPE_UPLOAD);
		CHECK_ZG uploadBufferLvl1.create(texture.sizeInBytes(), ZG_MEMORY_TYPE_UPLOAD);
		CHECK_ZG uploadBufferLvl2.create(texture.sizeInBytes(), ZG_MEMORY_TYPE_UPLOAD);
		CHECK_ZG uploadBufferLvl3.create(texture.sizeInBytes(), ZG_MEMORY_TYPE_UPLOAD);

		// Copy to the texture
		zg::CommandList commandList;
		CHECK_ZG copyQueue.beginCommandListRecording(commandList);
		CHECK_ZG commandList.memcpyToTexture(texture, 0, imageLvl0, uploadBufferLvl0);
		CHECK_ZG commandList.memcpyToTexture(texture, 1, imageLvl1, uploadBufferLvl1);
		CHECK_ZG commandList.memcpyToTexture(texture, 2, imageLvl2, uploadBufferLvl2);
		CHECK_ZG commandList.memcpyToTexture(texture, 3, imageLvl3, uploadBufferLvl3);
		CHECK_ZG commandList.enableQueueTransition(texture);
		CHECK_ZG copyQueue.executeCommandList(commandList);
		CHECK_ZG copyQueue.flush();

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
		CHECK_ZG zgContextSwapchainResize(uint32_t(width), uint32_t(height));

		// Create view and projection matrices
		float vertFovDeg = 40.0f;
		float aspectRatio = float(width) / float(height);
		Vector origin = Vector(
			std::cos(timeSinceStart) * 5.0f,
			std::sin(timeSinceStart * 0.75f) + 1.5f,
			std::sin(timeSinceStart) * 5.0f);
		Vector dir = -origin;
		Vector up = Vector(0.0f, 1.0f, 0.0f);
		Matrix viewMatrix;
		zgUtilCreateViewMatrix(viewMatrix.m, &origin.x, &dir.x, &up.x);
		Matrix projMatrix;
		zgUtilCreatePerspectiveProjection(projMatrix.m, vertFovDeg, aspectRatio, 0.01f, 10.0f);

		// Measurement ids
		uint64_t frameMeasurementId = ~0ull;
		uint64_t computeMeasurementId = ~0ull;
		uint64_t renderMeasurementId = ~0ull;

		// Begin frame
		zg::Framebuffer framebuffer;
		CHECK_ZG zgContextSwapchainBeginFrame(
			&framebuffer.handle, profiler.handle, &frameMeasurementId);

		// Run compute command list
		{
			// Get a command list
			zg::CommandList commandList;
			CHECK_ZG presentQueue.beginCommandListRecording(commandList);

			// Start measuring performance
			CHECK_ZG commandList.profileBegin(profiler, computeMeasurementId);

			// Set pipeline
			CHECK_ZG commandList.setPipeline(textureModifyPipeline);

			// Mipmap level 0 - 256x256
			CHECK_ZG commandList.setPipelineBindings(zg::PipelineBindings()
				.addUnorderedTexture(0, 0, texture));
			CHECK_ZG commandList.dispatchCompute(256 / 64, 256);

			// Mipmap level 1 - 128x128
			CHECK_ZG commandList.setPipelineBindings(zg::PipelineBindings()
				.addUnorderedTexture(0, 1, texture));
			CHECK_ZG commandList.dispatchCompute(128 / 64, 128);

			// Mipmap level 2 - 64x64
			CHECK_ZG commandList.setPipelineBindings(zg::PipelineBindings()
				.addUnorderedTexture(0, 2, texture));
			CHECK_ZG commandList.dispatchCompute(64 / 64, 64);

			// Mipmap level 3 - 32x32
			CHECK_ZG commandList.setPipelineBindings(zg::PipelineBindings()
				.addUnorderedTexture(0, 3, texture));
			CHECK_ZG commandList.dispatchCompute(1, 32);

			// Finish measuring performance
			CHECK_ZG commandList.profileEnd(profiler, computeMeasurementId);

			// Execute command list
			CHECK_ZG presentQueue.executeCommandList(commandList);
		}

		// Run render command list
		{
			// Get a command list
			zg::CommandList commandList;
			CHECK_ZG presentQueue.beginCommandListRecording(commandList);

			// Start measuring performance
			CHECK_ZG commandList.profileBegin(profiler, renderMeasurementId);

			// Set framebuffer and clear it
			CHECK_ZG commandList.setFramebuffer(framebuffer);
			CHECK_ZG commandList.clearRenderTargetsOptimal();
			CHECK_ZG commandList.clearDepthBufferOptimal();

			// Set pipeline
			CHECK_ZG commandList.setPipeline(renderPipeline);

			// Set pipeline bindings
			CHECK_ZG commandList.setPipelineBindings(zg::PipelineBindings()
				.addConstantBuffer(1, constBufferDevice)
				.addTexture(0, texture));

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
				CHECK_ZG commandList.setPushConstant(0, &transforms, sizeof(Transforms));

				// Draw cube
				CHECK_ZG commandList.drawTrianglesIndexed(0, CUBE_NUM_INDICES);
			};

			// Set Cube's vertex and index buffer
			CHECK_ZG commandList.setIndexBuffer(
				cubeIndexBufferDevice, ZG_INDEX_BUFFER_TYPE_UINT32);
			CHECK_ZG commandList.setVertexBuffer(0, cubeVertexBufferDevice);

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

			// Finish measuring performance
			CHECK_ZG commandList.profileEnd(profiler, renderMeasurementId);

			// Execute command list
			CHECK_ZG presentQueue.executeCommandList(commandList);
		}

		// Finish frame
		CHECK_ZG zgContextSwapchainFinishFrame(profiler.handle, frameMeasurementId);

		// Small hack: Flush present queue so we can get measurements
		CHECK_ZG presentQueue.flush();

		// Get measurements and print them
		float frameTimeMs = 0.0f;
		float computeTimeMs = 0.0f;
		float renderTimeMs = 0.0f;
		CHECK_ZG profiler.getMeasurement(frameMeasurementId, frameTimeMs);
		CHECK_ZG profiler.getMeasurement(computeMeasurementId, computeTimeMs);
		CHECK_ZG profiler.getMeasurement(renderMeasurementId, renderTimeMs);
		printf("Frame time: %.2f ms\nCompute time: %.2f ms\nRender time: %.2f ms\n\n",
			frameTimeMs, computeTimeMs, renderTimeMs);
	}

	// Flush command queue so nothing is running when we start releasing resources
	CHECK_ZG presentQueue.flush();
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
	SDL_Window* window = initializeSdl2CreateWindow("ZeroG - Sample2 - Simple Rendering");

	// Runs the real main function
	realMain(window);

	// Deinitialize ZeroG
	CHECK_ZG zgContextDeinit();

	// Cleanup SDL2
	cleanupSdl2(window);

	return EXIT_SUCCESS;
}
