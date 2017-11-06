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

#include "ph/rendering/Renderer.hpp"

#include <algorithm>
#include <cstdint>

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#endif

#include <SDL.h>

#include <sfz/Assert.hpp>
#include <sfz/memory/New.hpp>
#include <sfz/strings/StackString.hpp>

#include "ph/config/GlobalConfig.hpp"
#include "ph/utils/Logging.hpp"

#if defined(PH_STATIC_LINK_RENDERER)
#include "ph/RendererInterface.h"
#endif

namespace ph {

using sfz::StackString;
using sfz::StackString192;
using std::uint32_t;

// Function Table struct
// ------------------------------------------------------------------------------------------------

extern "C" {
	struct FunctionTable {
		// Init functions
		uint32_t (*phRendererInterfaceVersion)(void);
		uint32_t (*phRequiredSDL2WindowFlags)(void);
		uint32_t (*phInitRenderer)(SDL_Window*, sfzAllocator*, phConfig*, phLogger*);
		uint32_t (*phDeinitRenderer)(void);

		// Resource management (meshes)
		void (*phSetDynamicMeshes)(const phMesh*, uint32_t);
		uint32_t (*phAddDynamicMesh)(const phMesh*);
		uint32_t (*phUpdateDynamicMesh)(const phMesh*, uint32_t);

		// Render commands
		void (*phBeginFrame)(const phCameraData*, const phSphereLight*, uint32_t);
		void (*phRender)(const phRenderEntity*, uint32_t);
		void (*phFinishFrame)(void);
	};
}

// Statics
// ------------------------------------------------------------------------------------------------

#if defined(PH_STATIC_LINK_RENDERER)

#define CALL_RENDERER_FUNCTION(table, function, ...) \
	(function(__VA_ARGS__))

#elif defined(_WIN32)

#define CALL_RENDERER_FUNCTION(table, function, ...) \
	((table)->function(__VA_ARGS__))

#else
#error "Not implemented"
#endif

#if defined(_WIN32)
static StackString192 getWindowsErrorMessage() noexcept
{
	StackString192 str;
	DWORD errorCode = GetLastError();
	if (errorCode != 0) {
		FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, errorCode,
		    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), str.str, sizeof(str.str), NULL);
	}
	return str;
}

#define LOAD_FUNCTION(module, table, functionName) \
	{ \
		using FunctionType = decltype(FunctionTable::functionName); \
		table->functionName = (FunctionType)GetProcAddress((HMODULE)module, #functionName); \
		if (table->functionName == nullptr) { \
			StackString192 error = getWindowsErrorMessage(); \
			PH_LOG(LOG_LEVEL_ERROR, "PhantasyEngine", "Failed to load %s(), message: %s", \
			    #functionName, error.str); \
		} \
	}
#endif

// Renderer: Constructors & destructors
// ------------------------------------------------------------------------------------------------

Renderer::Renderer(const char* moduleName, Allocator* allocator) noexcept
{
	this->load(moduleName, allocator);
}

Renderer::Renderer(Renderer&& other) noexcept
{
	this->swap(other);
}

Renderer& Renderer::operator= (Renderer&& other) noexcept
{
	this->swap(other);
	return *this;
}

Renderer::~Renderer() noexcept
{
	this->destroy();
}

// Renderer: Methods
// ------------------------------------------------------------------------------------------------

void Renderer::load(const char* moduleName, Allocator* allocator) noexcept
{
	sfz_assert_debug(moduleName != nullptr);
	sfz_assert_debug(allocator != nullptr);
	this->destroy();

	mAllocator = allocator;

	// If we statically link the renderer we only really need to check the interface version
#if defined(PH_STATIC_LINK_RENDERER)

	if (INTERFACE_VERSION != phRendererInterfaceVersion()) {
		PH_LOG(LOG_LEVEL_ERROR, "PhantasyEngine", "Statically linked renderer has wrong interface version (%u), expected (%u).",
			   mFunctionTable->phRendererInterfaceVersion(), INTERFACE_VERSION);
	}

	// If dynamically loading renderer we do this other stuff
#else
	// Load DLL on Windows
#ifdef _WIN32
	// Load DLL
	{
		StackString dllName;
		dllName.printf("%s.dll", moduleName);
		PH_LOG(LOG_LEVEL_INFO, "PhantasyEngine", "Trying to load renderer DLL: %s", dllName.str);
		mModuleHandle = LoadLibrary(dllName.str);
	}
	if (mModuleHandle == nullptr) {
		StackString192 error = getWindowsErrorMessage();
		PH_LOG(LOG_LEVEL_ERROR, "PhantasyEngine", "Failed to load DLL (%s), message: %s",
		    moduleName, error.str);
		return;
	}
#endif

	// Create function table
	mFunctionTable = sfz::sfzNew<FunctionTable>(allocator);
	std::memset(mFunctionTable, 0, sizeof(FunctionTable));

#ifdef _WIN32
	// Start of with loading interface version function and checking that the correct interface is used
	LOAD_FUNCTION(mModuleHandle, mFunctionTable, phRendererInterfaceVersion);
	if (INTERFACE_VERSION != mFunctionTable->phRendererInterfaceVersion()) {
		PH_LOG(LOG_LEVEL_ERROR, "PhantasyEngine", "Renderer DLL (%s.dll) has wrong interface version (%u), expected (%u).",
		    moduleName, mFunctionTable->phRendererInterfaceVersion(), INTERFACE_VERSION);
	}

	// Init functions
	LOAD_FUNCTION(mModuleHandle, mFunctionTable, phRequiredSDL2WindowFlags);
	LOAD_FUNCTION(mModuleHandle, mFunctionTable, phInitRenderer);
	LOAD_FUNCTION(mModuleHandle, mFunctionTable, phDeinitRenderer);

	// Resource management (meshes)
	LOAD_FUNCTION(mModuleHandle, mFunctionTable, phSetDynamicMeshes);
	LOAD_FUNCTION(mModuleHandle, mFunctionTable, phAddDynamicMesh);
	LOAD_FUNCTION(mModuleHandle, mFunctionTable, phUpdateDynamicMesh);

	// Render commands
	LOAD_FUNCTION(mModuleHandle, mFunctionTable, phBeginFrame);
	LOAD_FUNCTION(mModuleHandle, mFunctionTable, phRender);
	LOAD_FUNCTION(mModuleHandle, mFunctionTable, phFinishFrame);
#endif

#endif
}

void Renderer::swap(Renderer& other) noexcept
{
	std::swap(this->mModuleHandle, other.mModuleHandle);
	std::swap(this->mAllocator, other.mAllocator);
	std::swap(this->mFunctionTable, other.mFunctionTable);
}

void Renderer::destroy() noexcept
{
	if (mModuleHandle != nullptr) {

		// Deinit renderer
		this->deinitRenderer();

		// Unload DLL on Windows
#ifdef _WIN32
		BOOL freeSuccess = FreeLibrary((HMODULE)mModuleHandle);
		if (!freeSuccess) {
			StackString192 error = getWindowsErrorMessage();
			PH_LOG(LOG_LEVEL_ERROR, "PhantasyEngine", "Failed to unload DLL, message: %s",
			    error.str);
		}
#endif

		// Deallocate function table
		sfz::sfzDelete(mFunctionTable, mAllocator);

		// Reset all variables
		mModuleHandle = nullptr;
		mAllocator = nullptr;
		mFunctionTable = nullptr;
		mInited = false;
	}
}

// Renderer: Renderer functions
// ------------------------------------------------------------------------------------------------

uint32_t Renderer::rendererInterfaceVersion() const noexcept
{
#if !defined(PH_STATIC_LINK_RENDERER)
	sfz_assert_debug(mFunctionTable != nullptr);
	sfz_assert_debug(mFunctionTable->phRendererInterfaceVersion != nullptr);
#endif
	return CALL_RENDERER_FUNCTION(mFunctionTable, phRendererInterfaceVersion);
}

uint32_t Renderer::requiredSDL2WindowFlags() const noexcept
{
#if !defined(PH_STATIC_LINK_RENDERER)
	sfz_assert_debug(mFunctionTable != nullptr);
	sfz_assert_debug(mFunctionTable->phRequiredSDL2WindowFlags != nullptr);
#endif
	return CALL_RENDERER_FUNCTION(mFunctionTable, phRequiredSDL2WindowFlags);
}

bool Renderer::initRenderer(SDL_Window* window) noexcept
{
	if (mInited) {
		PH_LOG(LOG_LEVEL_WARNING, "PhantasyEngine", "Trying to init renderer that is already inited");
		return true;
	}

	phConfig tmpConfig = GlobalConfig::cInstance();
	phLogger tmpLogger = getLogger();
	uint32_t initSuccess = CALL_RENDERER_FUNCTION(mFunctionTable, phInitRenderer, window, mAllocator->cAllocator(), &tmpConfig, &tmpLogger);
	if (initSuccess == 0) {
		PH_LOG(LOG_LEVEL_ERROR, "PhantasyEngine", "Renderer failed to initialize.");
		return false;
	}

	mInited = true;
	return true;
}

void Renderer::deinitRenderer() noexcept
{
#if !defined(PH_STATIC_LINK_RENDERER)
	sfz_assert_debug(mFunctionTable != nullptr);
	sfz_assert_debug(mFunctionTable->phDeinitRenderer != nullptr);
#endif
	if (mInited) {
		CALL_RENDERER_FUNCTION(mFunctionTable, phDeinitRenderer);
	}
	mInited = false;
}

// Renderer: Resource management (meshes)
// ------------------------------------------------------------------------------------------------

void Renderer::setDynamicMeshes(const DynArray<Mesh>& meshes) noexcept
{
	// Convert from ph::Mesh to phMesh
	DynArray<phMesh> tmpMeshes;
	tmpMeshes.create(meshes.size(), mAllocator);
	for (const auto& mesh : meshes) {
		tmpMeshes.add(mesh.cView());
	}

	CALL_RENDERER_FUNCTION(mFunctionTable, phSetDynamicMeshes, tmpMeshes.data(), tmpMeshes.size());
}

uint32_t Renderer::addDynamicMesh(const Mesh& mesh) noexcept
{
	phMesh tmpMesh = mesh.cView();
	return CALL_RENDERER_FUNCTION(mFunctionTable, phAddDynamicMesh, &tmpMesh);
}

bool Renderer::updateDynamicMesh(const Mesh& mesh, uint32_t index) noexcept
{
	phMesh tmpMesh = mesh.cView();
	return CALL_RENDERER_FUNCTION(mFunctionTable, phUpdateDynamicMesh, &tmpMesh, index) != 0;
}

// Renderer: Render commands
// ------------------------------------------------------------------------------------------------

void Renderer::beginFrame(
	const CameraData& camera,
	const ph::SphereLight* dynamicSphereLights,
	uint32_t numDynamicSphereLights) noexcept
{
	CALL_RENDERER_FUNCTION(mFunctionTable, phBeginFrame,
		reinterpret_cast<const phCameraData*>(&camera),
		reinterpret_cast<const phSphereLight*>(dynamicSphereLights), numDynamicSphereLights);
}

void Renderer::beginFrame(
	const CameraData& camera,
	const DynArray<ph::SphereLight>& dynamicSphereLights) noexcept
{
	CALL_RENDERER_FUNCTION(mFunctionTable, phBeginFrame,
		reinterpret_cast<const phCameraData*>(&camera),
		reinterpret_cast<const phSphereLight*>(dynamicSphereLights.data()),
		dynamicSphereLights.size());
}

void Renderer::render(const RenderEntity* entities, uint32_t numEntities) noexcept
{
	CALL_RENDERER_FUNCTION(mFunctionTable, phRender,
		reinterpret_cast<const phRenderEntity*>(entities), numEntities);
}

void Renderer::finishFrame() noexcept
{
	CALL_RENDERER_FUNCTION(mFunctionTable, phFinishFrame);
}

} // namespace ph
