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

#pragma once

#include <cfloat>

#include <skipifzero.hpp>
#include <skipifzero_hash_maps.hpp>
#include <skipifzero_image_view.hpp>
#include <skipifzero_math.hpp>
#include <skipifzero_strings.hpp>

// About
// ------------------------------------------------------------------------------------------------

// ZeroUI - An immediate mode (~ish) roll-backable game ui library
//
// This is an immediate mode UI library which fills a specific niche, in-game (i.e. not tool) UI
// primarily targeting gamepads.
//
// Design goals:
//  * Primarily gamepad as input, but compatible with mouse/touch
//  * User owns all "UI" state, easy to rollback
//  * Immediate-mode~ish, API's designed similarly to ImGui and such where it makes sense
//  * Easy to create custom rendering functions for widgets
//  * Easy to extend with custom widgets
//  * API/engine independent, output is vertices to stream to GPU
//  * KISS - Keep It Simple Stupid

namespace zui {

using sfz::ImageViewConst;
using sfz::mat34;
using sfz::str48;
using sfz::strID;
using sfz::vec2;
using sfz::vec2_u32;
using sfz::vec3;
using sfz::vec4;

// RenderDataView
// ------------------------------------------------------------------------------------------------

struct Vertex final {
	vec3 pos; // Position, world or view space depending on what surface it is rendered to
	vec2 texcoord;
	vec3 colorLinear; // Color in linear space, i.e. NOT sRGB.
	float alphaLinear;
};
static_assert(sizeof(Vertex) == 36, "ZeroUI::Vertex is padded");

struct RenderCmd final {
	uint32_t startIndex = 0;
	uint32_t numIndices = 0;
	mat34 transform = mat34::identity();
	uint64_t imageHandle = 0;
	bool isAlphaTexture = false;
};

struct RenderDataView final {
	const Vertex* vertices = nullptr;
	const uint16_t* indices = nullptr;
	const RenderCmd* commands = nullptr;
	uint32_t numVertices = 0;
	uint32_t numIndices = 0;
	uint32_t numCommands = 0;
};

// Input
// ------------------------------------------------------------------------------------------------

enum class InputAction : uint8_t {
	NONE = 0,
	UP = 1,
	DOWN = 2,
	LEFT = 3,
	RIGHT = 4,
	ACTIVATE = 5,
	CANCEL = 6,
	POINTER_MOVE = 7,
};

constexpr bool isMoveAction(InputAction a) { return InputAction::UP <= a && a <= InputAction::RIGHT; }

struct Input final {
	InputAction action = InputAction::NONE;
	vec2 pointerPos = vec2(-FLT_MAX);
};

// Attribute
// ------------------------------------------------------------------------------------------------

// (Rendering) Attributes are used to define specific properties (such as colors, sizes, fonts,
// etc) when rendering a widget.
//
// This is accomplished through the use of an AttributeSet, which is a mapping from name to the
// attribute value. The render function of each widget is allowed to freely read and write to this
// global attribute set.
//
// Writing to the attribute set is primarily meant for parent/container widgets before rendering
// their children, and then revert the changes before exiting the render function.

struct alignas(16) Attribute final {
	
	uint8_t bytes[31] = {};
	uint8_t size = 0;

	Attribute() = default;
	template<typename T> Attribute(const T& val) { size = sizeof(T); as<T>() = val; }
	
	template<typename T>
	T& as()
	{
		static_assert(sizeof(T) <= sizeof(bytes));
		sfz_assert(sizeof(T) == size);
		return *reinterpret_cast<T*>(bytes);
	}

	template<typename T>
	const T& as() const
	{
		static_assert(sizeof(T) <= sizeof(bytes));
		sfz_assert(sizeof(T) == size);
		return *reinterpret_cast<const T*>(bytes);
	}
};

using AttributeSet = sfz::HashMap<strID, Attribute>;

// Alignment
// ------------------------------------------------------------------------------------------------

enum class HAlign : int16_t { LEFT = -1, CENTER = 0, RIGHT = 1 };
enum class VAlign : int16_t { BOTTOM = -1, CENTER = 0, TOP = 1 };

constexpr HAlign LEFT = HAlign::LEFT;
constexpr HAlign H_CENTER = HAlign::CENTER;
constexpr HAlign RIGHT = HAlign::RIGHT;

constexpr VAlign BOTTOM = VAlign::BOTTOM;
constexpr VAlign V_CENTER = VAlign::CENTER;
constexpr VAlign TOP = VAlign::TOP;

struct Align final {
	HAlign halign = H_CENTER;
	VAlign valign = V_CENTER;
	constexpr Align() = default;
	constexpr Align(HAlign halign, VAlign valign) : halign(halign), valign(valign) {}
};

// Box
// ------------------------------------------------------------------------------------------------

struct Box final {
	vec2 min = vec2(0.0f);
	vec2 max = vec2(0.0f);

	constexpr Box() = default;
	constexpr Box(vec2 center, vec2 dims) : min(center - dims * 0.5f), max(center + dims * 0.5f) { }
	constexpr Box(float x, float y, float w, float h) : Box(vec2(x, y), vec2(w, h)) { }

	constexpr vec2 center() const { return (min + max) * 0.5f; }
	constexpr vec2 dims() const { return max - min; }
	constexpr float width() const { return max.x - min.x; }
	constexpr float height() const{ return max.y - min.y; }

	constexpr bool pointInside(vec2 p) const
	{
		return min.x <= p.x && p.x <= max.x && min.y <= p.y && p.y <= max.y;
	}

	constexpr bool overlaps(Box o) const
	{
		return min.x <= o.max.x && max.x >= o.min.x && min.y <= o.max.y && max.y >= o.min.y;
	}
};

// Widget
// ------------------------------------------------------------------------------------------------

struct WidgetBase final {
	Box box;
	float timeSinceFocusStartedSecs = FLT_MAX;
	float timeSinceFocusEndedSecs = FLT_MAX;
	float timeSinceActivationSecs = FLT_MAX;
	bool focused = false;
	bool activated = false;
	uint8_t ___PADDING___[2] = {};

	void incrementTimers(float deltaSecs);
	void setFocused();
	void setUnfocused();
	void setActivated();
};

struct WidgetCmd;

using GetWidgetBaseFunc = WidgetBase*(void* widgetData);
using GetNextWidgetBoxFunc = void(WidgetCmd* cmd, strID childID, Box* boxOut);
using HandlePointerInputFunc = void(WidgetCmd* cmd, vec2 pointerPosSS);
using HandleMoveInputFunc = void(WidgetCmd* cmd, Input* input, bool* moveActive);
using DrawFunc = void(
	const WidgetCmd* cmd,
	AttributeSet* attributes,
	const mat34& surfaceTransform, // Note: Should normally not be modified by container widgets.
	float lagSinceSurfaceEndSecs);

void defaultPassthroughDrawFunc(
	const WidgetCmd* cmd,
	AttributeSet* attributes,
	const mat34& surfaceTransform,
	float lagSinceSurfaceEndSecs);

struct WidgetDesc final {
	uint32_t widgetDataSizeBytes = 0;
	GetWidgetBaseFunc* getWidgetBaseFunc = nullptr;
	GetNextWidgetBoxFunc* getNextWidgetBoxFunc = nullptr;
	HandlePointerInputFunc* handlePointerInputFunc = nullptr;
	HandleMoveInputFunc* handleMoveInputFunc = nullptr;
	DrawFunc* drawFunc = defaultPassthroughDrawFunc;
};

using InitWidgetFunc = void(void* widgetData);
using GetWidgetDataFunc = void*(void* userPtr, strID id, uint32_t sizeBytes, InitWidgetFunc* initFunc);

// Initialization functions
// ------------------------------------------------------------------------------------------------

// Forward declare Context
struct Context;

void initZeroUI(sfz::Allocator* allocator, uint32_t surfaceTmpMemoryBytes, uint32_t oversampleFonts = 1);
void deinitZeroUI();

void setFontTextureHandle(uint64_t handle);

void registerWidget(const char* name, const WidgetDesc& desc);

void registerArchetype(const char* widgetName, const char* archetypeName, DrawFunc* drawFunc);

void registerDefaultAttribute(const char* attribName, const char* value);
void registerDefaultAttribute(const char* attribName, Attribute attribute);

bool registerFont(const char* name, const char* path, float size, bool defaultFont = false);

// Input & rendering functions
// ------------------------------------------------------------------------------------------------

// === IMPORTANT ===
//
// ALL POINTERS PASSED TO ZEROUI AFTER tickBegin() MUST REMAIN VALID UNTIL tickEnd(). This includes
// any and all pointers inside any structs passed to ZeroUI.
//
// The motivation behind this is that it might not be possible to know how a specific data struct
// need to be modified until later ones have been observed. If data owned by the user can't be
// assumed to be valid some more complicated scheme where it is copied and then returned back next
// tick must be used, which did not seem worth the complexity.
//
// === IMPORTANT ===

struct SurfaceDesc final {

	// Name of surface
	// Used to identify the surface. Any previous surfaces with the same name will be cleared.
	str48 name;

	// Function used to allocate/get previous widget data structs
	GetWidgetDataFunc* getWidgetDataFunc = nullptr;
	void* widgetDataFuncUserPtr = nullptr;

	// Input
	Input input;
	vec2_u32 fbDims = vec2_u32(0u);
	float deltaTimeSecs = 0.0f;

	// Position on framebuffer, default aligned to bottom left corner of framebuffer
	vec2_u32 posOnFB = vec2_u32(0u);
	HAlign halignOnFB = HAlign::LEFT;
	VAlign valignOnFB = VAlign::BOTTOM;

	// Size on framebuffer, 0 == entire framebuffer
	vec2_u32 dimsOnFB = vec2_u32(0u);

	// Coordinate system of the surface which things will be drawn upon. E.g., vec2(100.0f, 100.0f),
	// means that you will be using "percentages" of the total size of the surface when specifying
	// sizes. 0.0f == same as dimsFB.
	vec2 dims = vec2(0.0f);
};

// Clears all current surfaces. Not strictly necessary if you only use one surface with the same
// name (as that will automatically clear the previous surface with the same name).
void clearSurfaces();

// Clear a specific surface
void clearSurface(const char* name);

// Clears previous surface with the same name and starts accepting input to it.
void surfaceBegin(const SurfaceDesc& desc);

// Returns dimensions of current surface
vec2 surfaceGetDims();

// Stops accepting input to the current surface and performs some logic updates.
void surfaceEnd();

// Renders the currently active surfaces
void render(float lagSinceSurfaceEndSecs);

// Font texture communication, should be called after rendering.
bool hasFontTextureUpdate();
ImageViewConst getFontTexture();

// Render data guaranteed to be valid until next time render() is called
RenderDataView getRenderData();

// Archetypes
// ------------------------------------------------------------------------------------------------

void pushArchetype(const char* widgetName, const char* archetypeName);
void popArchetype(const char* widgetName);

// Base container widget
// ------------------------------------------------------------------------------------------------

// A base container is used to place widgets at absolute positions relative to the location of
// the container. It can also be used to inject attributes in the attribute set before rendering
// its children.
//
// A base container that covers the entire surface is automatically created when the surface
// is started using surfaceBegin(). This means that first widget (which may be a container itself)
// must always be placed in the default one.

struct BaseContainerData final {
	WidgetBase base;
	sfz::Map16<strID, Attribute> newValues;
	vec2 nextPos = vec2(0.0f);
	Align nextAlign;
	vec2 nextDims = vec2(0.0f);
};

void baseBegin(BaseContainerData* data);
void baseBegin(strID id);
void baseBegin(const char* id);

void baseAttribute(const char* id, Attribute attrib);
void baseAttribute(const char* id, const char* value);
void baseAttribute(strID id, Attribute attrib);

void baseSetPos(float x, float y);
void baseSetPos(vec2 pos);
void baseSetAlign(HAlign halign, VAlign valign);
void baseSetAlign(Align align);
void baseSetDims(float width, float height);
void baseSetDims(vec2 dims);

void baseSet(float x, float y, float width, float height);
void baseSet(vec2 pos, vec2 dims);
void baseSet(float x, float y, HAlign halign, VAlign valign, float width, float height);
void baseSet(vec2 pos, Align align, vec2 dims);

void baseEnd();

} // namespace zui
