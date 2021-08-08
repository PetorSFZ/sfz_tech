// Copyright (c) 2020 Peter Hillerström (skipifzero.com, peter@hstroem.se)
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

#include "ZeroUI.hpp"
#include "ZeroUI_Internal.hpp"
#include "ZeroUI_Drawing.hpp"
#include "ZeroUI_CoreWidgets.hpp"

#include "skipifzero_new.hpp"

namespace zui {

// Statics
// ------------------------------------------------------------------------------------------------

static WidgetBase* baseGetBase(void* widgetData)
{
	return &reinterpret_cast<BaseContainerData*>(widgetData)->base;
}

static void baseGetNextWidgetBox(WidgetCmd* cmd, strID childID, Box* boxOut)
{
	(void)childID;
	BaseContainerData& data = *cmd->data<BaseContainerData>();
	vec2 bottomLeftPos = data.base.box.min;
	vec2 centerPos = calcCenterPos(data.nextPos, data.nextAlign, data.nextDims);
	vec2 nextPos = bottomLeftPos + centerPos;
	*boxOut = Box(nextPos, data.nextDims);
}

static void baseHandlePointerInput(WidgetCmd* cmd, vec2 pointerPosSS)
{
	for (WidgetCmd& child : cmd->children) {
		child.handlePointerInput(pointerPosSS);
	}
}

static void baseHandleMoveInput(WidgetCmd* cmd, Input* input, bool* moveActive)
{
	if (input->action == InputAction::UP) {
		for (uint32_t cmdIdx = cmd->children.size(); cmdIdx > 0; cmdIdx--) {
			WidgetCmd& child = cmd->children[cmdIdx - 1];
			child.handleMoveInput(input, moveActive);
		}
	}
	else {
		for (uint32_t cmdIdx = 0; cmdIdx < cmd->children.size(); cmdIdx++) {
			WidgetCmd& child = cmd->children[cmdIdx];
			child.handleMoveInput(input, moveActive);
		}
	}
}

static void baseDraw(
	const WidgetCmd* cmd,
	AttributeSet* attributes,
	const mat34& surfaceTransform,
	float lagSinceSurfaceEndSecs)
{
	const BaseContainerData& data = *cmd->data<BaseContainerData>();

	// Set attributes and backup old ones
	sfz::Map16<strID, Attribute> backup;
	sfz_assert(data.newValues.size() <= data.newValues.capacity());
	for (const auto& pair : data.newValues) {
	
		// Backup old attribute
		Attribute* oldAttrib = attributes->get(pair.key);
		if (oldAttrib != nullptr) {
			backup.put(pair.key, *oldAttrib);
		}

		// Set new one
		attributes->put(pair.key, pair.value);
	}

	// Render child
	for (const WidgetCmd& child : cmd->children) {
		child.draw(attributes, surfaceTransform, lagSinceSurfaceEndSecs);
	}

	// Restore old attributes
	for (const auto& pair : backup) {
		attributes->put(pair.key, pair.value);
	}
}

// ZeroUI Context + initialization
// ------------------------------------------------------------------------------------------------

static Context* globalCtx = nullptr;

Context& ctx()
{
	return *globalCtx;
}

void initZeroUI(SfzAllocator* allocator, uint32_t surfaceTmpMemoryBytes, uint32_t oversampleFonts)
{
	sfz_assert(globalCtx == nullptr);

	globalCtx = sfz_new<Context>(allocator, sfz_dbg(""));
	ctx().heapAllocator = allocator;
	ctx().defaultID = strID("default");

	// Initialize widgets
	ctx().widgetTypes.init(32, allocator, sfz_dbg(""));

	// Initialize drawing module
	internalDrawInit(allocator, oversampleFonts);

	// Initialize surfaces
	ctx().surfaces.init(32, allocator, sfz_dbg(""));
	ctx().recycledSurfaces.init(32, allocator, sfz_dbg(""));
	ctx().surfaceTmpMemoryBytes = surfaceTmpMemoryBytes;
	
	// Initialize attribute set
	ctx().attributes.init(256, allocator, sfz_dbg(""));
	ctx().defaultAttributes.init(256, allocator, sfz_dbg(""));

	// Initialize base container
	{
		WidgetDesc desc = {};
		desc.widgetDataSizeBytes = sizeof(BaseContainerData);
		desc.getWidgetBaseFunc = baseGetBase;
		desc.getNextWidgetBoxFunc = baseGetNextWidgetBox;
		desc.handlePointerInputFunc = baseHandlePointerInput;
		desc.handleMoveInputFunc = baseHandleMoveInput;
		desc.drawFunc = baseDraw;
		registerWidget(BASE_CON_NAME, desc);
	}

	// Initialize core widgets
	internalCoreWidgetsInit();
}

void deinitZeroUI()
{
	sfz_assert(globalCtx != nullptr);
	SfzAllocator* allocator = ctx().heapAllocator;

	internalDrawDeinit();

	sfz_delete(allocator, globalCtx);
	globalCtx = nullptr;
}

// Fonts
// ------------------------------------------------------------------------------------------------

void setFontTextureHandle(uint64_t handle)
{
	sfz_assert(globalCtx != nullptr);
	internalDrawSetFontHandle(handle);
}

bool registerFont(const char* name, const char* path, float size, bool defaultFont)
{
	strID nameID = strID(name);
	bool success = internalDrawAddFont(name, nameID, path, size);
	if (defaultFont) {
		sfz_assert_hard(success);
		ctx().defaultAttributes.put(strID("default_font"), nameID);
	}
	return success;
}

// Attributes
// ------------------------------------------------------------------------------------------------

void registerDefaultAttribute(const char* attribName, const char* value)
{
	strID attribID = strID(attribName);
	sfz_assert(ctx().defaultAttributes.get(attribID) == nullptr);
	ctx().defaultAttributes.put(attribID, strID(value));
}

void registerDefaultAttribute(const char* attribName, Attribute attribute)
{
	strID attribID = strID(attribName);
	sfz_assert(ctx().defaultAttributes.get(attribID) == nullptr);
	ctx().defaultAttributes.put(attribID, attribute);
}

// Widget
// ------------------------------------------------------------------------------------------------

void WidgetBase::incrementTimers(float deltaSecs)
{
	timeSinceFocusStartedSecs += deltaSecs;
	timeSinceFocusEndedSecs += deltaSecs;
	timeSinceActivationSecs += deltaSecs;
}

void WidgetBase::setFocused()
{
	if (!focused) {
		timeSinceFocusStartedSecs = 0.0f;
	}
	focused = true;
}

void WidgetBase::setUnfocused()
{
	if (focused) {
		timeSinceFocusEndedSecs = 0.0f;
	}
	focused = false;
}

void WidgetBase::setActivated()
{
	timeSinceActivationSecs = 0.0f;
	activated = true;
}

void defaultPassthroughDrawFunc(
	const WidgetCmd* cmd,
	AttributeSet* attributes,
	const mat34& transform,
	float lagSinceSurfaceEndSecs)
{
	for (const WidgetCmd& child : cmd->children) {
		child.draw(attributes, transform, lagSinceSurfaceEndSecs);
	}
}

void registerWidget(const char* name, const WidgetDesc& desc)
{
	strID nameID = strID(name);
	sfz_assert(ctx().widgetTypes.get(nameID) == nullptr);
	WidgetType& type = ctx().widgetTypes.put(nameID, {});
	sfz_assert(desc.widgetDataSizeBytes >= sizeof(WidgetBase));
	type.widgetDataSizeBytes = desc.widgetDataSizeBytes;
	type.getBaseFunc = desc.getWidgetBaseFunc;
	type.getNextWidgetBoxFunc = desc.getNextWidgetBoxFunc;
	type.handlePointerInputFunc = desc.handlePointerInputFunc;
	type.handleMoveInputFunc = desc.handleMoveInputFunc;
	WidgetArchetype& archetype = type.archetypes.put(ctx().defaultID, {});
	archetype.drawFunc = desc.drawFunc;
	type.archetypeStack.init(64, ctx().heapAllocator, sfz_dbg(""));
}

// Archetypes
// ------------------------------------------------------------------------------------------------

void registerArchetype(const char* widgetName, const char* archetypeName, DrawFunc* drawFunc)
{
	strID widgetID = strID(widgetName);
	WidgetType* type = ctx().widgetTypes.get(widgetID);
	sfz_assert(type != nullptr);
	strID archetypeID = strID(archetypeName);
	sfz_assert(type->archetypes.get(archetypeID) == nullptr);
	WidgetArchetype& archetype = type->archetypes.put(archetypeID, {});
	archetype.drawFunc = drawFunc;
}

void pushArchetype(const char* widgetName, const char* archetypeName)
{
	strID widgetID = strID(widgetName);
	WidgetType* type = ctx().widgetTypes.get(widgetID);
	sfz_assert(type != nullptr);
	strID archetypeID = strID(archetypeName);
	sfz_assert(type->archetypes.get(archetypeID) != nullptr);
	type->archetypeStack.add(archetypeID);
}

void popArchetype(const char* widgetName)
{
	strID widgetID = strID(widgetName);
	WidgetType* type = ctx().widgetTypes.get(widgetID);
	sfz_assert(type != nullptr);
	sfz_assert(type->archetypeStack.size() > 1);
	type->archetypeStack.pop();
}

// Input & rendering functions
// ------------------------------------------------------------------------------------------------

void clearSurfaces()
{
	// Recycle in use surfaces and then clear list
	for (Surface& surface : ctx().surfaces) {
		surface.clear();
		ctx().recycledSurfaces.add(std::move(surface));
	}
	ctx().surfaces.clear();
}

void clearSurface(const char* name)
{
	Surface* surface = ctx().surfaces.find([&](const auto& e) {
		return e.desc.name == name;
	});
	if (surface != nullptr) {
		surface->clear();
		ctx().recycledSurfaces.add(std::move(*surface));
		ctx().surfaces.removeQuickSwap(uint32_t(surface - ctx().surfaces.data()));
	}
}

void surfaceBegin(const SurfaceDesc& desc)
{
	// Set active surface and make sure it is cleared
	sfz_assert(ctx().activeSurface == nullptr);
	{
		// Try to find existing surface with same name, otherwise create new one.
		Surface* existingSurface = ctx().surfaces.find([&](const Surface& surface) {
			return surface.desc.name == desc.name;
		});
		if (existingSurface == nullptr) {
			ctx().createNewSurface();
		}
		else {
			existingSurface->clear();
			ctx().activeSurface = existingSurface;
		}
	}
	Surface& surface = *ctx().activeSurface;
	surface.desc = desc;

	// Clear all archetype stacks and set default archetype
	for (auto pair : ctx().widgetTypes) {
		pair.value.archetypeStack.clear();
		pair.value.archetypeStack.add(ctx().defaultID);
	}

	// Setup default base container for surface as root
	{
		surface.cmdRoot.widgetID = BASE_CON_ID;
		surface.cmdRoot.dataPtr = sfz_new<BaseContainerData>(surface.arena.getArena(), sfz_dbg(""));
		surface.cmdRoot.archetypeDrawFunc = baseDraw;
		surface.pushMakeParent(&surface.cmdRoot);
		WidgetCmd& root = surface.getCurrentParent();
		BaseContainerData& rootData = *root.data<BaseContainerData>();
		rootData.base.box = Box(desc.dims * 0.5f, desc.dims);
		rootData.nextPos = desc.dims * 0.5f;
		rootData.nextDims = desc.dims;
	}

	// Get size of surface on framebuffer
	vec2_u32 dimsOnFB = desc.dimsOnFB;
	if (dimsOnFB == vec2_u32(0u)) dimsOnFB = desc.fbDims;

	// Get internal size of surface
	if (sfz::eqf(desc.dims, vec2(0.0f))) {
		surface.desc.dims = vec2(dimsOnFB);
	}
	
	// Calculate transform
	const vec3 fbToClipScale = vec3(2.0f / vec2(desc.fbDims), 1.0f);
	const vec3 fbToClipTransl = vec3(-1.0f, -1.0f, 0.0f);
	const mat4 fbToClip = mat4::translation3(fbToClipTransl) * mat4::scaling3(fbToClipScale);

	vec2 halfOffset = vec2(0.0f);
	if (desc.halignOnFB == HAlign::LEFT) halfOffset.x = 0.0f;
	else if (desc.halignOnFB == HAlign::CENTER) halfOffset.x = -0.5f * float(dimsOnFB.x);
	else if (desc.halignOnFB == HAlign::RIGHT) halfOffset.x = -1.0f * float(dimsOnFB.x);
	else sfz_assert_hard(false);

	if (desc.valignOnFB == VAlign::BOTTOM) halfOffset.y = 0.0f;
	else if (desc.valignOnFB == VAlign::CENTER) halfOffset.y = -0.5f * float(dimsOnFB.y);
	else if (desc.valignOnFB == VAlign::TOP) halfOffset.y = -1.0f * float(dimsOnFB.y);
	else sfz_assert_hard(false);

	const vec3 surfToFbScale = vec3((1.0f / surface.desc.dims) * vec2(dimsOnFB), 1.0f);
	const vec3 surfToFbTransl = vec3(vec2(desc.posOnFB) + halfOffset, 0.0f);
	const mat4 surfToFb = mat4::translation3(surfToFbTransl) * mat4::scaling3(surfToFbScale);

	surface.transform = mat34(fbToClip * surfToFb);

	// Input transform
	const mat4 fbToSurf = sfz::inverse(surfToFb);
	surface.inputTransform = mat34(fbToSurf);
	surface.pointerPosSS = sfz::transformPoint(fbToSurf, vec3(surface.desc.input.pointerPos, 0.0f)).xy;
}

vec2 surfaceGetDims()
{
	sfz_assert(ctx().activeSurface != nullptr);
	Surface& surface = *ctx().activeSurface;
	return surface.desc.dims;
}

void surfaceEnd()
{
	// Get surface and make it inactive
	sfz_assert(ctx().activeSurface != nullptr);
	Surface& surface = *ctx().activeSurface;
	ctx().activeSurface = nullptr;

	// Update pointer if pointer move
	if (surface.desc.input.action == InputAction::POINTER_MOVE) {
		surface.cmdRoot.handlePointerInput(surface.pointerPosSS);
	}

	else {
		Input input = surface.desc.input;
		bool moveActive = false;
		surface.cmdRoot.handleMoveInput(&input, &moveActive);
		
		// Attempt to fix some edge cases where move action wasn't consumed
		if (isMoveAction(input.action)) {
			moveActive = true;
			surface.cmdRoot.handleMoveInput(&input, &moveActive);
		}
	}

	// Go through all commands and copy widget data so that we no longer rely on pointers being
	// valid.

	// Setup stack to traverse
	SfzAllocator* arena = surface.arena.getArena();
	sfz::Array<WidgetCmd*> stack;
	stack.init(surface.cmdParentStack.size(), arena, sfz_dbg(""));
	stack.add(&surface.cmdRoot);

	// Recursively traverse command tree
	while (!stack.isEmpty()) {
		WidgetCmd* cmd = stack.pop();

		// Allocate memory for copy of widget data and then copy to it.
		uint32_t sizeOfWidgetData = cmd->sizeOfWidgetData();
		void* copy = arena->alloc(sfz_dbg(""), sizeOfWidgetData);
		memcpy(copy, cmd->dataPtr, sizeOfWidgetData);
		cmd->dataPtr = copy;

		// Add childrens to stack
		for (uint32_t i = cmd->children.size(); i > 0; i--) {
			stack.add(&cmd->children[i - 1]);
		}
	}
}

void render(float lagSinceSurfaceEndSecs)
{
	// Clear render data
	internalDrawClearRenderData();

	for (uint32_t surfaceIdx = 0; surfaceIdx < ctx().surfaces.size(); surfaceIdx++) {
		Surface& surface = ctx().surfaces[surfaceIdx];

		// Clear attribute set and set defaults
		ctx().attributes.clear();
		for (const auto& pair : ctx().defaultAttributes) {
			ctx().attributes.put(pair.key, pair.value);
		}

		// Draw recursively
		surface.cmdRoot.draw(&ctx().attributes, surface.transform, lagSinceSurfaceEndSecs);
	}
}

bool hasFontTextureUpdate()
{
	return internalDrawFontTextureUpdated();
}

ImageViewConst getFontTexture()
{
	return internalDrawGetFontTexture();
}

RenderDataView getRenderData()
{
	return internalDrawGetRenderDataView();
}

// Base container widget
// ------------------------------------------------------------------------------------------------

void baseBegin(BaseContainerData* data)
{
	sfz_assert(data->newValues.size() <= data->newValues.capacity());
	Surface& surface = *ctx().activeSurface;

	// Get button position and dimensions from current parent
	WidgetCmd& parent = surface.getCurrentParent();
	parent.getNextWidgetBox(BASE_CON_ID, &data->base.box);

	// Set initial next widget dimensions/position to cover entire container
	data->nextDims = data->base.box.dims();
	data->nextPos = data->nextDims * 0.5f;

	// Update timers
	data->base.incrementTimers(surface.desc.deltaTimeSecs);

	// Can't activate absolute container
	data->base.activated = false;

	// Add command and make parent
	parent.children.add(WidgetCmd(BASE_CON_ID, data));
	surface.pushMakeParent(&parent.children.last());
}

void baseBegin(strID id)
{
	BaseContainerData* data = ctx().getWidgetData<BaseContainerData>(id);
	baseBegin(data);
}

void baseBegin(const char* id)
{
	BaseContainerData* data = ctx().getWidgetData<BaseContainerData>(id);
	baseBegin(data);
}

void baseAttribute(const char* id, Attribute attrib)
{
	baseAttribute(strID(id), attrib);
}

void baseAttribute(const char* id, const char* value)
{
	baseAttribute(strID(id), strID(value));
}

void baseAttribute(strID id, Attribute attrib)
{
	WidgetCmd& parent = ctx().activeSurface->getCurrentParent();
	sfz_assert(parent.widgetID == BASE_CON_ID);
	BaseContainerData& data = *parent.data<BaseContainerData>();
	data.newValues.put(id, attrib);
	sfz_assert(data.newValues.size() <= data.newValues.capacity());
}

void baseSetPos(float x, float y)
{
	baseSetPos(vec2(x, y));
}

void baseSetPos(vec2 pos)
{
	WidgetCmd& parent = ctx().activeSurface->getCurrentParent();
	sfz_assert(parent.widgetID == BASE_CON_ID);
	parent.data<BaseContainerData>()->nextPos = pos;
}

void baseSetAlign(HAlign halign, VAlign valign)
{
	baseSetAlign(Align(halign, valign));
}

void baseSetAlign(Align align)
{
	WidgetCmd& parent = ctx().activeSurface->getCurrentParent();
	sfz_assert(parent.widgetID == BASE_CON_ID);
	parent.data<BaseContainerData>()->nextAlign = align;
}

void baseSetDims(float width, float height)
{
	baseSetDims(vec2(width, height));
}

void baseSetDims(vec2 dims)
{
	WidgetCmd& parent = ctx().activeSurface->getCurrentParent();
	sfz_assert(parent.widgetID == BASE_CON_ID);
	parent.data<BaseContainerData>()->nextDims = dims;
}

void baseSet(float x, float y, float width, float height)
{
	baseSet(vec2(x, y), vec2(width, height));
}

void baseSet(vec2 pos, vec2 dims)
{
	baseSetPos(pos);
	baseSetAlign(H_CENTER, V_CENTER);
	baseSetDims(dims);
}

void baseSet(float x, float y, HAlign halign, VAlign valign, float width, float height)
{
	baseSet(vec2(x, y), Align(halign, valign), vec2(width, height));
}

void baseSet(vec2 pos, Align align, vec2 dims)
{
	baseSetPos(pos);
	baseSetAlign(align);
	baseSetDims(dims);
}

void baseEnd()
{
	Surface& surface = *ctx().activeSurface;
	WidgetCmd& parent = surface.getCurrentParent();
	sfz_assert(parent.widgetID == BASE_CON_ID);
	sfz_assert(surface.cmdParentStack.size() > 1); // Don't remove default base container
	surface.popParent();
}

} // namespace zui
