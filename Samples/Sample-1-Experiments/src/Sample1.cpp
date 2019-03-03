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

#include "ZeroG-cpp.hpp"

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
	pipelineInfo.vertexAttributes[0].vertexBufferSlot = 0;
	pipelineInfo.vertexAttributes[0].offsetToFirstElementInBytes = 0;
	pipelineInfo.vertexAttributes[0].type = ZG_VERTEX_ATTRIBUTE_F32_3;

	pipelineInfo.vertexAttributes[1].attributeLocation = 1;
	pipelineInfo.vertexAttributes[1].vertexBufferSlot = 0;
	pipelineInfo.vertexAttributes[1].offsetToFirstElementInBytes = sizeof(float) * 3;
	pipelineInfo.vertexAttributes[1].type = ZG_VERTEX_ATTRIBUTE_F32_3;

	pipelineInfo.numVertexBufferSlots = 1;
	pipelineInfo.vertexBufferStridesBytes[0] = sizeof(Vertex);

	pipelineInfo.numParameters = 2;
	pipelineInfo.parameters[0].bindingType = ZG_PIPELINE_PARAMETER_BINDING_TYPE_PUSH_CONSTANT;
	pipelineInfo.parameters[0].pushConstant.shaderRegister = 0;
	pipelineInfo.parameters[0].pushConstant.sizeInWords = 4; // sizeof(vec4) / 4 == 4

	pipelineInfo.parameters[1].bindingType = ZG_PIPELINE_PARAMETER_BINDING_TYPE_PUSH_CONSTANT;
	pipelineInfo.parameters[1].pushConstant.shaderRegister = 1;
	pipelineInfo.parameters[1].pushConstant.sizeInWords = 1;
	
	ZgPipelineRendering* pipeline = nullptr;
	ZgPipelineRenderingSignature signature = {};
	CHECK_ZG zgPipelineRenderingCreate(ctx.mContext, &pipeline, &signature, &pipelineInfo);

	// Create a buffer
	ZgBufferCreateInfo bufferInfo = {};
	bufferInfo.sizeInBytes = 64ull * 1024ull;
	bufferInfo.bufferMemoryType = ZG_BUFFER_MEMORY_TYPE_UPLOAD;

	ZgBuffer* vertexUploadBuffer = nullptr;
	CHECK_ZG zgBufferCreate(ctx.mContext, &vertexUploadBuffer, &bufferInfo);

	ZgBuffer* vertexDeviceBuffer = nullptr;
	bufferInfo.bufferMemoryType = ZG_BUFFER_MEMORY_TYPE_DEVICE;
	CHECK_ZG zgBufferCreate(ctx.mContext, &vertexDeviceBuffer, &bufferInfo);

	CHECK_ZG zgBufferMemcpyTo(
		ctx.mContext, vertexUploadBuffer, 0, (const uint8_t*)triangleVertices, sizeof(triangleVertices));

	// Get the command queue
	ZgCommandQueue* commandQueue = nullptr;
	CHECK_ZG zgContextGeCommandQueueGraphicsPresent(ctx.mContext, &commandQueue);

	// Copy to the device buffer
	{
		ZgCommandList* commandList = nullptr;
		CHECK_ZG zgCommandQueueBeginCommandListRecording(commandQueue, &commandList);
		CHECK_ZG zgCommandListMemcpyBufferToBuffer(
			commandList, vertexDeviceBuffer, 0, vertexUploadBuffer, 0, sizeof(triangleVertices));
		CHECK_ZG zgCommandQueueExecuteCommandList(commandQueue, commandList);
		CHECK_ZG zgCommandQueueFlush(commandQueue);
	}

	// Destroy upload buffer
	CHECK_ZG zgBufferRelease(ctx.mContext, vertexUploadBuffer);

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

		// Begin frame
		ZgFramebuffer* framebuffer = nullptr;
		CHECK_ZG zgContextBeginFrame(ctx.mContext, &framebuffer);

		// Get a command list
		ZgCommandList* commandList = nullptr;
		CHECK_ZG zgCommandQueueBeginCommandListRecording(commandQueue, &commandList);

		// Record some commands
		CHECK_ZG zgCommandListSetPipelineRendering(commandList, pipeline);
		ZgCommandListSetFramebufferInfo framebufferInfo = {};
		framebufferInfo.framebuffer = framebuffer;
		float color[4] = { 0.0f, 0.5f, 0.0f, 0.0f };
		CHECK_ZG zgCommandListSetPushConstant(commandList, 0, color);
		float offset = 0.2f;
		CHECK_ZG zgCommandListSetFramebuffer(commandList, &framebufferInfo);
		CHECK_ZG zgCommandListSetPushConstant(commandList, 1, &offset);
		CHECK_ZG zgCommandListClearFramebuffer(commandList, 0.2f, 0.2f, 0.3f, 1.0f);
		CHECK_ZG zgCommandListSetVertexBuffer(commandList, 0, vertexDeviceBuffer);
		CHECK_ZG zgCommandListDrawTriangles(commandList, 0, 3);

		// Execute command list
		CHECK_ZG zgCommandQueueExecuteCommandList(commandQueue, commandList);

		// Finish frame
		CHECK_ZG zgContextFinishFrame(ctx.mContext);
	}

	// Flush command queue so nothing is running when we start releasing resources
	CHECK_ZG zgCommandQueueFlush(commandQueue);

	// Release ZeroG resources
	CHECK_ZG zgBufferRelease(ctx.mContext, vertexDeviceBuffer);
	CHECK_ZG zgPipelineRenderingRelease(ctx.mContext, pipeline);

	// Destroy ZeroG context
	ctx.destroy();

	// Cleanup
	SDL_DestroyWindow(window);
	SDL_Quit();

	return EXIT_SUCCESS;
}
