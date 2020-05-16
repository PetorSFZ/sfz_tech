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

#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <SDL.h>
#include <SDL_syswm.h>

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

	// Create simply compute pipeline
	zg::PipelineCompute memcpyPipeline;
	CHECK_ZG zg::PipelineComputeBuilder()
		.addComputeShaderPath("mainCS", "res/Sample-3/memcpy.hlsl")
		.addPushConstant(0)
		.buildFromFileHLSL(memcpyPipeline);

	// Get the command queues
	zg::CommandQueue presentQueue;
	CHECK_ZG zg::CommandQueue::getPresentQueue(presentQueue);

	// Create memory heaps
	constexpr uint32_t BUFFER_ALIGNMENT = 64 * 1024; // Buffers must be 64KiB aligned
	constexpr uint32_t NUM_VECS = 1024;
	constexpr uint32_t NUM_FLOATS = NUM_VECS * 4;
	constexpr uint32_t BUFFER_SIZE_BYTES = NUM_FLOATS * sizeof(float);
	zg::MemoryHeap uploadHeap, deviceHeap, downloadHeap;
	CHECK_ZG uploadHeap.create(BUFFER_SIZE_BYTES, ZG_MEMORY_TYPE_UPLOAD);
	CHECK_ZG deviceHeap.create(BUFFER_ALIGNMENT * 2, ZG_MEMORY_TYPE_DEVICE);
	CHECK_ZG downloadHeap.create(BUFFER_SIZE_BYTES, ZG_MEMORY_TYPE_DOWNLOAD);

	// Create buffers
	zg::Buffer uploadBuffer, deviceBufferSrc, deviceBufferDst, downloadBuffer;
	CHECK_ZG uploadHeap.bufferCreate(uploadBuffer, 0, BUFFER_SIZE_BYTES);
	CHECK_ZG deviceHeap.bufferCreate(deviceBufferSrc, 0, BUFFER_SIZE_BYTES);
	CHECK_ZG deviceHeap.bufferCreate(deviceBufferDst, BUFFER_ALIGNMENT, BUFFER_SIZE_BYTES);
	CHECK_ZG downloadHeap.bufferCreate(downloadBuffer, 0, BUFFER_SIZE_BYTES);

	// Copy data to upload buffer
	float referenceData[NUM_FLOATS];
	for (uint32_t i = 0; i < NUM_FLOATS; i++) referenceData[i] = float(i);
	CHECK_ZG uploadBuffer.memcpyTo(0, referenceData, BUFFER_SIZE_BYTES);

	// Get a command list
	zg::CommandList commandList;
	CHECK_ZG presentQueue.beginCommandListRecording(commandList);

	// Upload data to src buffer
	CHECK_ZG commandList.memcpyBufferToBuffer(deviceBufferSrc, 0, uploadBuffer, 0, BUFFER_SIZE_BYTES);

	// Memcpy data from src to dst buffer using compute pipeline
	CHECK_ZG commandList.setPipeline(memcpyPipeline);
	struct { uint32_t numVectors; uint32_t padding[3]; } pushConstantData;
	pushConstantData.numVectors = NUM_VECS;
	CHECK_ZG commandList.setPushConstant(0, &pushConstantData, sizeof(pushConstantData));
	CHECK_ZG commandList.setPipelineBindings(zg::PipelineBindings()
		.addUnorderedBuffer(0, 0, NUM_VECS, sizeof(float) * 4, deviceBufferSrc)
		.addUnorderedBuffer(1, 0, NUM_VECS, sizeof(float) * 4, deviceBufferDst));
	CHECK_ZG commandList.dispatchCompute(NUM_VECS / 64);

	// Download data from dst buffer
	CHECK_ZG commandList.memcpyBufferToBuffer(downloadBuffer, 0, deviceBufferDst, 0, BUFFER_SIZE_BYTES);

	// Execute command list
	CHECK_ZG presentQueue.executeCommandList(commandList);

	// Flush present queue so we are finishing the gpu operations
	CHECK_ZG presentQueue.flush();

	// Copy data from download buffer
	float resultData[NUM_FLOATS] = {};
	CHECK_ZG downloadBuffer.memcpyFrom(resultData, 0, BUFFER_SIZE_BYTES);

	// Compare result data with reference
	bool success = true;
	for (uint32_t i = 0; i < NUM_FLOATS; i++) {
		const float ref = referenceData[i];
		const float res = resultData[i];
		if (ref != res) {
			printf("Memcpy failed! referenceData[%u] = 0x%08x, resultData[%u] = 0x%08x\n",
				i, *((const uint32_t*)&ref), i, *((const uint32_t*)&res));
			success = false;
			break;
		}
	}

	// Print result
	if (success) {
		printf("Memcpy successful! Downloaded data matches reference data\n");
	}
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
	SDL_Window* window = initializeSdl2CreateWindow("ZeroG - Sample3 - Minimal Compute");

	// Runs the real main function
	realMain(window);

	// Deinitialize ZeroG
	CHECK_ZG zgContextDeinit();

	// Cleanup SDL2
	cleanupSdl2(window);

	return EXIT_SUCCESS;
}
