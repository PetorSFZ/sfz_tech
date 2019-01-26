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

#include <cstdlib>
#include <cstdio>

#include <SDL.h>
#include <SDL_syswm.h>

#include <ZeroG/ZeroG.hpp>

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <direct.h>
#endif

// Settings
// ------------------------------------------------------------------------------------------------

constexpr bool DEBUG_MODE = true;

// Statics
// ------------------------------------------------------------------------------------------------

static HWND getWin32WindowHandle(SDL_Window* window) noexcept
{
	SDL_SysWMinfo info = {};
	SDL_VERSION(&info.version);
	if (!SDL_GetWindowWMInfo(window, &info)) return nullptr;
	return info.info.win.window;
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

	// Init SDL2
	if (SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0) {
		printf("SDL_Init() failed: %s", SDL_GetError());
		return EXIT_FAILURE;
	}

	// Window
	SDL_Window* window = SDL_CreateWindow(
		"ZeroG-Sample1",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		800, 800,
		SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);
	if (window == NULL) {
		printf("SDL_CreateWindow() failed: %s\n", SDL_GetError());
		SDL_Quit();
		return EXIT_FAILURE;
	}

	// Create ZeroG context
	ZgContextInitSettings initSettings = {};
	initSettings.backend = ZG_BACKEND_D3D12;
	initSettings.width = 512;
	initSettings.height = 512;
	initSettings.debugMode = DEBUG_MODE ? ZG_TRUE : ZG_FALSE;
	initSettings.nativeWindowHandle = getWin32WindowHandle(window);
	zg::Context ctx;
	CHECK_ZG ctx.init(initSettings);


	// Create a rendering pipeline
	ZgPipelineRenderingCreateInfo pipelineInfo = {};
	pipelineInfo.vertexShaderPath = "res/Sample-1/test.hlsl";
	pipelineInfo.vertexShaderEntry = "VSMain";

	pipelineInfo.pixelShaderPath = "res/Sample-1/test.hlsl";
	pipelineInfo.pixelShaderEntry = "PSMain";
	
	pipelineInfo.shaderVersion = ZG_SHADER_MODEL_6_2;
	pipelineInfo.dxcCompilerFlags[0] = "-Zi";
	pipelineInfo.dxcCompilerFlags[1] = "-O4";
	
	pipelineInfo.numVertexAttributes = 2;

	pipelineInfo.vertexAttributes[0].attributeLocation = 0;
	pipelineInfo.vertexAttributes[0].offsetToFirstElementInBytes = 0;
	pipelineInfo.vertexAttributes[0].type = ZG_VERTEX_ATTRIBUTE_FLOAT3;

	pipelineInfo.vertexAttributes[1].attributeLocation = 1;
	pipelineInfo.vertexAttributes[1].offsetToFirstElementInBytes = sizeof(float) * 3;
	pipelineInfo.vertexAttributes[1].type = ZG_VERTEX_ATTRIBUTE_FLOAT3;

	ZgPipelineRendering* pipeline = nullptr;
	CHECK_ZG zgPipelineRenderingCreate(ctx.mContext, &pipeline, &pipelineInfo);

	// Create a buffer
	ZgBufferCreateInfo bufferInfo = {};
	bufferInfo.sizeInBytes = 64ull * 1024ull;
	bufferInfo.bufferMemoryType = ZG_BUFFER_MEMORY_TYPE_UPLOAD;

	ZgBuffer* buffer = nullptr;
	CHECK_ZG zgBufferCreate(ctx.mContext, &buffer, &bufferInfo);

	struct Vertex {
		float position[3];
		float color[3];
	};
	static_assert(sizeof(Vertex) == sizeof(float) * 6, "Vertex is padded");

	Vertex triangleVertices[3] = {
		{ { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
		{ { 1.0f, -1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
		{ { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } }	
	};

	CHECK_ZG zgBufferMemcpyTo(
		ctx.mContext, buffer, 0, (const uint8_t*)triangleVertices, sizeof(triangleVertices));


	// Get the command queue
	ZgCommandQueue* commandQueue = nullptr;
	CHECK_ZG zgContextGetCommandQueue(ctx.mContext, &commandQueue);

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
		CHECK_ZG zgContextResize(ctx.mContext, uint32_t(width), uint32_t(height));

		// Get a command list
		ZgCommandList* commandList = nullptr;
		CHECK_ZG zgCommandQueueBeginCommandListRecording(commandQueue, &commandList);
		
		// Execute command list
		CHECK_ZG zgCommandQueueExecuteCommandList(commandQueue, commandList);
		
		// TODO: Rendering here
		CHECK_ZG zgRenderExperiment(ctx.mContext, buffer, pipeline, commandList);
	}

	// Flush command queue so nothing is running when we start releasing resources
	CHECK_ZG zgCommandQueueFlush(commandQueue);

	// Release ZeroG resources
	CHECK_ZG zgBufferRelease(ctx.mContext, buffer);
	CHECK_ZG zgPipelineRenderingRelease(ctx.mContext, pipeline);

	// Destroy ZeroG context
	ctx.destroy();

	// Cleanup
	SDL_DestroyWindow(window);
	SDL_Quit();

	return EXIT_SUCCESS;
}
