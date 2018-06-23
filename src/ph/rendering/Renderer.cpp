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

#elif !defined(__EMSCRIPTEN__)
#include <dlfcn.h>
#endif

#include <SDL.h>

#include <sfz/Assert.hpp>
#include <sfz/Logging.hpp>
#include <sfz/Context.hpp>
#include <sfz/memory/New.hpp>
#include <sfz/strings/StackString.hpp>

#include "ph/config/GlobalConfig.hpp"

#include "ph/RendererInterface.h"

namespace ph {

using sfz::str96;
using sfz::str192;
using std::uint32_t;

static_assert(Renderer::INTERFACE_VERSION == PH_RENDERER_INTERFACE_VERSION, "Interface versions don't match");

// Function Table struct
// ------------------------------------------------------------------------------------------------

extern "C" {
	struct FunctionTable {
		// Init functions
		decltype(phRendererInterfaceVersion)* phRendererInterfaceVersion;
		decltype(phRequiredSDL2WindowFlags)* phRequiredSDL2WindowFlags;
		decltype(phInitRenderer)* phInitRenderer;
		decltype(phDeinitRenderer)* phDeinitRenderer;
		decltype(phInitImgui)* phInitImgui;

		// State query functions
		decltype(phImguiWindowDimensions)* phImguiWindowDimensions;

		// Resource management (textures)
		decltype(phSetTextures)* phSetTextures;
		decltype(phAddTexture)* phAddTexture;
		decltype(phUpdateTexture)* phUpdateTexture;

		// Resource management (materials)
		decltype(phSetMaterials)* phSetMaterials;
		decltype(phAddMaterial)* phAddMaterial;
		decltype(phUpdateMaterial)* phUpdateMaterial;

		// Resource management (meshes)
		decltype(phSetDynamicMeshes)* phSetDynamicMeshes;
		decltype(phAddDynamicMesh)* phAddDynamicMesh;
		decltype(phUpdateDynamicMesh)* phUpdateDynamicMesh;

		// Render commands
		decltype(phBeginFrame)* phBeginFrame;
		decltype(phRender)* phRender;
		decltype(phRenderImgui)* phRenderImgui;
		decltype(phFinishFrame)* phFinishFrame;
	};
}

// Statics
// ------------------------------------------------------------------------------------------------

#if defined(PH_STATIC_LINK_RENDERER)

#define CALL_RENDERER_FUNCTION(table, function, ...) \
	(function(__VA_ARGS__))

#else

#define CALL_RENDERER_FUNCTION(table, function, ...) \
	((table)->function(__VA_ARGS__))

#endif

#if defined(_WIN32)
static str192 getWindowsErrorMessage() noexcept
{
	str192 str;
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
			str192 error = getWindowsErrorMessage(); \
			SFZ_ERROR("PhantasyEngine", "Failed to load %s(), message: %s", \
				#functionName, error.str); \
		} \
	}

#elif !defined(__EMSCRIPTEN__)

#define LOAD_FUNCTION(module, table, functionName) \
	{ \
		using FunctionType = decltype(FunctionTable::functionName); \
		table->functionName = (FunctionType)dlsym(module, #functionName); \
		if (table->functionName == nullptr) { \
			SFZ_ERROR("PhantasyEngine", "Failed to load %s(), message: %s", \
				#functionName, dlerror()); \
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
		SFZ_ERROR("PhantasyEngine",
			"Statically linked renderer has wrong interface version (%u), expected (%u).",
			 mFunctionTable->phRendererInterfaceVersion(), INTERFACE_VERSION);
	}

	// If dynamically loading renderer we do this other stuff
#else
	// Load DLL on Windows
#ifdef _WIN32
	// Load DLL
	{
		str96 dllName;
		dllName.printf("%s.dll", moduleName);
		SFZ_INFO("PhantasyEngine", "Trying to load renderer DLL: %s", dllName.str);
		mModuleHandle = LoadLibrary(dllName.str);
	}
	if (mModuleHandle == nullptr) {
		str192 error = getWindowsErrorMessage();
		SFZ_ERROR("PhantasyEngine", "Failed to load DLL (%s), message: %s", moduleName, error.str);
		return;
	}

	// Load shared library on unix like platforms
#else
	{
		str96 sharedName;
		sharedName.printf("lib%s.dylib", moduleName);
		SFZ_INFO("PhantasyEngine", "Trying to load renderer shared library: %s", sharedName.str);
		mModuleHandle = dlopen(sharedName.str, RTLD_LAZY);
		if (mModuleHandle == nullptr) {
			SFZ_ERROR("PhantasyEngine", "Failed to load shared library (%s), message: %s",
				moduleName, dlerror());
			return;
		}
	}

#endif

	// Create function table
	mFunctionTable = sfz::sfzNew<FunctionTable>(allocator);
	std::memset(mFunctionTable, 0, sizeof(FunctionTable));

	// Start of with loading interface version function and checking that the correct interface is used
	LOAD_FUNCTION(mModuleHandle, mFunctionTable, phRendererInterfaceVersion);
	if (INTERFACE_VERSION != mFunctionTable->phRendererInterfaceVersion()) {
		SFZ_ERROR("PhantasyEngine",
			"Renderer DLL (%s.dll) has wrong interface version (%u), expected (%u).",
			moduleName, mFunctionTable->phRendererInterfaceVersion(), INTERFACE_VERSION);
	}

	// Init functions
	LOAD_FUNCTION(mModuleHandle, mFunctionTable, phRequiredSDL2WindowFlags);
	LOAD_FUNCTION(mModuleHandle, mFunctionTable, phInitRenderer);
	LOAD_FUNCTION(mModuleHandle, mFunctionTable, phDeinitRenderer);
	LOAD_FUNCTION(mModuleHandle, mFunctionTable, phInitImgui);

	// State query functions
	LOAD_FUNCTION(mModuleHandle, mFunctionTable, phImguiWindowDimensions);

	// Resource management (textures)
	LOAD_FUNCTION(mModuleHandle, mFunctionTable, phSetTextures);
	LOAD_FUNCTION(mModuleHandle, mFunctionTable, phAddTexture);
	LOAD_FUNCTION(mModuleHandle, mFunctionTable, phUpdateTexture);

	// Resource management (materials)
	LOAD_FUNCTION(mModuleHandle, mFunctionTable, phSetMaterials);
	LOAD_FUNCTION(mModuleHandle, mFunctionTable, phAddMaterial);
	LOAD_FUNCTION(mModuleHandle, mFunctionTable, phUpdateMaterial);

	// Resource management (meshes)
	LOAD_FUNCTION(mModuleHandle, mFunctionTable, phSetDynamicMeshes);
	LOAD_FUNCTION(mModuleHandle, mFunctionTable, phAddDynamicMesh);
	LOAD_FUNCTION(mModuleHandle, mFunctionTable, phUpdateDynamicMesh);

	// Render commands
	LOAD_FUNCTION(mModuleHandle, mFunctionTable, phBeginFrame);
	LOAD_FUNCTION(mModuleHandle, mFunctionTable, phRender);
	LOAD_FUNCTION(mModuleHandle, mFunctionTable, phRenderImgui);
	LOAD_FUNCTION(mModuleHandle, mFunctionTable, phFinishFrame);

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
			str192 error = getWindowsErrorMessage();
			SFZ_ERROR("PhantasyEngine", "Failed to unload DLL, message: %s", error.str);
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
		SFZ_WARNING("PhantasyEngine", "Trying to init renderer that is already inited");
		return true;
	}

	phConfig tmpConfig = GlobalConfig::cInstance();
	uint32_t initSuccess = CALL_RENDERER_FUNCTION(mFunctionTable, phInitRenderer,
		sfz::getContext(), window, mAllocator, &tmpConfig);
	if (initSuccess == 0) {
		SFZ_ERROR("PhantasyEngine", "Renderer failed to initialize.");
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

void Renderer::initImgui(phConstImageView fontTexture) noexcept
{
	CALL_RENDERER_FUNCTION(mFunctionTable, phInitImgui, &fontTexture);
}

// Renderer: State query functions
// --------------------------------------------------------------------------------------------

vec2 Renderer::imguiWindowDimensions() const noexcept
{
	vec2 dims;
	CALL_RENDERER_FUNCTION(mFunctionTable, phImguiWindowDimensions, &dims.x, &dims.y);
	return dims;
}

// Resource management (textures)
// ------------------------------------------------------------------------------------------------

void Renderer::setTextures(const DynArray<phConstImageView>& textures) noexcept
{
	CALL_RENDERER_FUNCTION(mFunctionTable, phSetTextures, textures.data(), textures.size());
}

uint32_t Renderer::addTexture(phConstImageView texture) noexcept
{
	return CALL_RENDERER_FUNCTION(mFunctionTable, phAddTexture, &texture);
}

bool Renderer::updateTexture(phConstImageView texture, uint32_t index) noexcept
{
	return CALL_RENDERER_FUNCTION(mFunctionTable, phUpdateTexture, &texture, index) != 0;
}

// Renderer:: Resource management (materials)
// ------------------------------------------------------------------------------------------------

void Renderer::setMaterials(const DynArray<phMaterial>& materials) noexcept
{
	CALL_RENDERER_FUNCTION(mFunctionTable, phSetMaterials, materials.data(), materials.size());
}

uint32_t Renderer::addMaterial(const phMaterial& material) noexcept
{
	return CALL_RENDERER_FUNCTION(mFunctionTable, phAddMaterial, &material);
}

bool Renderer::updateMaterial(const phMaterial& material, uint32_t index) noexcept
{
	return CALL_RENDERER_FUNCTION(mFunctionTable, phUpdateMaterial, &material, index) != 0;
}

// Renderer: Resource management (meshes)
// ------------------------------------------------------------------------------------------------

void Renderer::setDynamicMeshes(const DynArray<ConstMeshView>& meshes) noexcept
{
	// Convert from ph::Mesh to phMeshView
	DynArray<phConstMeshView> tmpMeshes;
	tmpMeshes.create(meshes.size(), mAllocator);
	for (const auto& mesh : meshes) {
		tmpMeshes.add(mesh);
	}

	CALL_RENDERER_FUNCTION(mFunctionTable, phSetDynamicMeshes, tmpMeshes.data(), tmpMeshes.size());
}

uint32_t Renderer::addDynamicMesh(const ConstMeshView& mesh) noexcept
{
	phConstMeshView tmpMesh = mesh;
	return CALL_RENDERER_FUNCTION(mFunctionTable, phAddDynamicMesh, &tmpMesh);
}

bool Renderer::updateDynamicMesh(const ConstMeshView& mesh, uint32_t index) noexcept
{
	phConstMeshView tmpMesh = mesh;
	return CALL_RENDERER_FUNCTION(mFunctionTable, phUpdateDynamicMesh, &tmpMesh, index) != 0;
}

// Renderer: Render commands
// ------------------------------------------------------------------------------------------------

void Renderer::beginFrame(
	const phCameraData& camera,
	const ph::SphereLight* dynamicSphereLights,
	uint32_t numDynamicSphereLights) noexcept
{
	CALL_RENDERER_FUNCTION(mFunctionTable, phBeginFrame,
		&camera,
		reinterpret_cast<const phSphereLight*>(dynamicSphereLights), numDynamicSphereLights);
}

void Renderer::beginFrame(
	const phCameraData& camera,
	const DynArray<ph::SphereLight>& dynamicSphereLights) noexcept
{
	CALL_RENDERER_FUNCTION(mFunctionTable, phBeginFrame,
		&camera,
		reinterpret_cast<const phSphereLight*>(dynamicSphereLights.data()),
		dynamicSphereLights.size());
}

void Renderer::render(const RenderEntity* entities, uint32_t numEntities) noexcept
{
	CALL_RENDERER_FUNCTION(mFunctionTable, phRender,
		reinterpret_cast<const phRenderEntity*>(entities), numEntities);
}

void Renderer::renderImgui(
	const DynArray<ImguiVertex>& vertices,
	const DynArray<uint32_t>& indices,
	const DynArray<ImguiCommand>& commands) noexcept
{
	CALL_RENDERER_FUNCTION(mFunctionTable, phRenderImgui,
		reinterpret_cast<const phImguiVertex*>(vertices.data()), vertices.size(),
		indices.data(), indices.size(),
		reinterpret_cast<const phImguiCommand*>(commands.data()), commands.size());
}

void Renderer::finishFrame() noexcept
{
	CALL_RENDERER_FUNCTION(mFunctionTable, phFinishFrame);
}

} // namespace ph
