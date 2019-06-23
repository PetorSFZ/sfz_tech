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

#define PH_EXPORTING_RENDERER_FUNCTIONS
#include "ph/RendererInterface.h"

#include "ph/Bool32.h"
using ph::Bool32;

static uint32_t gNumTextures = 0;
static uint32_t gNumMeshes = 0;

// Interface: Init functions
// ------------------------------------------------------------------------------------------------

extern "C" PH_DLL_EXPORT
uint32_t phRendererInterfaceVersion(void)
{
	return PH_RENDERER_INTERFACE_VERSION;
}

extern "C" PH_DLL_EXPORT
uint32_t phRequiredSDL2WindowFlags(void)
{
	return 0;
}

extern "C" PH_DLL_EXPORT
phBool32 phInitRenderer(
	phContext* context,
	SDL_Window* window,
	void* allocator)
{
	return Bool32(true);
}

extern "C" PH_DLL_EXPORT
void phDeinitRenderer(void)
{
}

extern "C" PH_DLL_EXPORT
void phInitImgui(const phConstImageView* fontTexture)
{
	(void)fontTexture;
}

// State query functions
// ------------------------------------------------------------------------------------------------

extern "C" PH_DLL_EXPORT
void phImguiWindowDimensions(float* widthOut, float* heightOut)
{
	(void)widthOut;
	(void)heightOut;
}

// Resource management (textures)
// ------------------------------------------------------------------------------------------------

extern "C" PH_DLL_EXPORT
void phSetTextures(const phConstImageView* textures, uint32_t numTextures)
{
	(void)textures;
	gNumTextures = numTextures;
}

extern "C" PH_DLL_EXPORT
uint16_t phAddTexture(const phConstImageView* texture)
{
	(void)texture;
	uint16_t idx = uint16_t(gNumTextures);
	gNumTextures += 1;
	return idx;
}

extern "C" PH_DLL_EXPORT
phBool32 phUpdateTexture(const phConstImageView* texture, uint16_t index)
{
	(void)texture;
	(void)index;
	return Bool32(true);
}

extern "C" PH_DLL_EXPORT
uint32_t phNumTextures(void)
{
	return gNumTextures;
}

// Interface: Resource management (meshes)
// ------------------------------------------------------------------------------------------------

extern "C" PH_DLL_EXPORT
void phSetMeshes(const phConstMeshView* meshes, uint32_t numMeshes)
{
	(void)meshes;
	gNumMeshes = numMeshes;
}

extern "C" PH_DLL_EXPORT
uint32_t phAddMesh(const phConstMeshView* mesh)
{
	(void)mesh;
	uint32_t idx = gNumMeshes;
	gNumMeshes += 1;
	return idx;
}

extern "C" PH_DLL_EXPORT
phBool32 phUpdateMesh(const phConstMeshView* mesh, uint32_t index)
{
	(void)mesh;
	(void)index;
	return Bool32(true);
}

extern "C" PH_DLL_EXPORT
phBool32 phUpdateMeshMaterials(uint32_t meshIdx, const phMaterial* materials, uint32_t numMaterials)
{
	(void)meshIdx;
	(void)materials;
	(void)numMaterials;
	return Bool32(true);
}

// Interface: Resource management (static scene)
// ------------------------------------------------------------------------------------------------

extern "C" PH_DLL_EXPORT
void phSetStaticScene(const phStaticSceneView* scene)
{
	(void)scene;
}

extern "C" PH_DLL_EXPORT
void phRemoveStaticScene(void)
{

}

// Interface: Render commands
// ------------------------------------------------------------------------------------------------

extern "C" PH_DLL_EXPORT
void phBeginFrame(
	const float* clearColor,
	const phCameraData* camera,
	const float* ambientLight,
	const phSphereLight* dynamicSphereLights,
	uint32_t numDynamicSphereLights)
{
	(void)clearColor;
	(void)camera;
	(void)ambientLight;
	(void)dynamicSphereLights;
	(void)numDynamicSphereLights;
}

extern "C" PH_DLL_EXPORT
void phRenderStaticScene(void)
{
}

extern "C" PH_DLL_EXPORT
void phRender(const phRenderEntity* entities, uint32_t numEntities)
{
	(void)entities;
	(void)numEntities;
}

extern "C" PH_DLL_EXPORT
void phRenderImgui(
	const phImguiVertex* vertices,
	uint32_t numVertices,
	const uint32_t* indices,
	uint32_t numIndices,
	const phImguiCommand* commands,
	uint32_t numCommands)
{
	(void)vertices;
	(void)numVertices;
	(void)indices;
	(void)numIndices;
	(void)commands;
	(void)numCommands;
}

extern "C" PH_DLL_EXPORT
void phFinishFrame(void)
{
}
