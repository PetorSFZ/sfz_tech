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

#include "ZeroG-cpp.hpp"

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
		zg::Context::compiledApiVersion(), zg::Context::linkedApiVersion());

	// Create ZeroG context
	ZgContextInitSettings initSettings = {};
#if defined(_WIN32)
	initSettings.backend = ZG_BACKEND_D3D12;
#else
	initSettings.backend = ZG_BACKEND_VULKAN;
#endif
	initSettings.width = 512;
	initSettings.height = 512;
	initSettings.debugMode = DEBUG_MODE ? ZG_TRUE : ZG_FALSE;
	initSettings.nativeHandle = getNativeHandle(window);
	zg::Context zgCtx;
	CHECK_ZG zgCtx.init(initSettings);

	// Create simply compute pipeline
	zg::PipelineCompute memcpyPipeline;
	{
		// Create info
		ZgPipelineComputeCreateInfo createInfo = {};
		createInfo.computeShader = "res/Sample-3/memcpy.hlsl";
		createInfo.computeShaderEntry = "mainCS";

		// Compile settings
		ZgPipelineCompileSettingsHLSL compileSettings = {};
		compileSettings.shaderModel = ZG_SHADER_MODEL_6_0;
		compileSettings.dxcCompilerFlags[0] = "-Zi";
		compileSettings.dxcCompilerFlags[1] = "-O3";

		// Create pipeline
		CHECK_ZG memcpyPipeline.createFromFileHLSL(createInfo, compileSettings);
	}

	// Get the command queues
	zg::CommandQueue presentQueue;
	CHECK_ZG zg::CommandQueue::getPresentQueue(presentQueue);

	// Run our main loop
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

		// Query drawable width and height and update ZeroG context
		int width = 0;
		int height = 0;
		SDL_GL_GetDrawableSize(window, &width, &height);
		CHECK_ZG zgCtx.swapchainResize(uint32_t(width), uint32_t(height));

		// Begin frame
		zg::Framebuffer framebuffer;
		CHECK_ZG zgCtx.swapchainBeginFrame(framebuffer);

		// Get a command list
		zg::CommandList commandList;
		CHECK_ZG presentQueue.beginCommandListRecording(commandList);

		// Set framebuffer and clear it
		CHECK_ZG commandList.setFramebuffer(framebuffer);
		CHECK_ZG commandList.clearRenderTargets(1.0f, 0.0f, 0.0f, 1.0f);
		CHECK_ZG commandList.clearDepthBuffer(1.0f);

		// Set compute pipeline and run it
		CHECK_ZG commandList.setPipeline(memcpyPipeline);
		CHECK_ZG commandList.dispatchCompute(1);

		// Execute command list
		CHECK_ZG presentQueue.executeCommandList(commandList);

		// Finish frame
		CHECK_ZG zgCtx.swapchainFinishFrame();
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
	SDL_Window* window = initializeSdl2CreateWindow("ZeroG - Sample3 - Minimal Compute");

	// Runs the real main function
	realMain(window);

	// Cleanup SDL2
	cleanupSdl2(window);

	return EXIT_SUCCESS;
}
