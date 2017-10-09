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

#pragma once

#include <cstdint>

#include <sfz/containers/DynArray.hpp>
#include <sfz/memory/Allocator.hpp>

#include <ph/rendering/CameraData.h>
#include <ph/rendering/Mesh.h>
#include <ph/rendering/RenderEntity.h>
#include <ph/rendering/SphereLight.h>

extern "C" struct SDL_Window; // Forward declare SDL_Window

namespace ph {

using sfz::Allocator;
using sfz::DynArray;
using std::uint32_t;

// Renderer class
// ------------------------------------------------------------------------------------------------

extern "C" struct FunctionTable; // Forward declare internal FunctionTable

class Renderer final {
public:
	// Public constants
	// --------------------------------------------------------------------------------------------

	/// The interface version supported by this wrapper. Only renderers which return the same
	/// version with "phRendererInterfaceVersion()" are compatible.
	static constexpr uint32_t INTERFACE_VERSION = 1;

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	Renderer() noexcept = default;
	Renderer(const Renderer&) = delete;
	Renderer& operator= (const Renderer&) = delete;

	Renderer(const char* moduleName, Allocator* allocator) noexcept;
	Renderer(Renderer&& other) noexcept; 
	Renderer& operator= (Renderer&& other) noexcept;
	~Renderer() noexcept;

	// Methods
	// --------------------------------------------------------------------------------------------

	/// Loads the Renderer from DLL or such. Still needs to be initalized using initRenderer().
	/// \param moduleName the name of the DLL (on Windows)
	/// \param allocator the sfz Allocator used to allocate memory on the CPU for this renderer
	void load(const char* moduleName, Allocator* allocator) noexcept;

	/// Swaps this renderer with another renderer
	void swap(Renderer& other) noexcept;

	/// Destroys this renderer
	void destroy() noexcept;

	// Renderer: Init functions
	// --------------------------------------------------------------------------------------------

	/// See phRendererInterfaceVersion()
	uint32_t rendererInterfaceVersion() const noexcept;

	/// See phRequiredSDL2WindowFlags()
	uint32_t requiredSDL2WindowFlags() const noexcept;

	/// See phInitRenderer()
	bool initRenderer(SDL_Window* window) noexcept;

	/// See phDeinitRenderer(), is automatically called in destroy() or destructor. No need to call
	/// manually.
	void deinitRenderer() noexcept;

	// Renderer: Resource management (meshes)
	// --------------------------------------------------------------------------------------------

	/// See phSetDynamicMeshes()
	void setDynamicMeshes(const DynArray<Mesh>& meshes) noexcept;

	/// See phAddDynamicMesh()
	uint32_t addDynamicMesh(const Mesh& mesh) noexcept;

	/// See phUpdateDynamicMesh()
	bool updateDynamicMesh(const Mesh& mesh, uint32_t index) noexcept;

	// Renderer: Render commands
	// --------------------------------------------------------------------------------------------

	/// See phBeginFrame()
	void beginFrame(
		const phCameraData& camera,
		const phSphereLight* dynamicSphereLights,
		uint32_t numDynamicSphereLights) noexcept;

	/// See phRender()
	void render(const phRenderEntity* entities, uint32_t numEntities) noexcept;

	/// See phFinishFrame()
	void finishFrame() noexcept;


private:
	void* mModuleHandle = nullptr; // Holds a HMODULE on Windows
	Allocator* mAllocator = nullptr;
	struct FunctionTable* mFunctionTable = nullptr;
	bool mInited = false;
};

} // namespace ph
