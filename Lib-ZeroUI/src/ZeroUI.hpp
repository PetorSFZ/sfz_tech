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

#include <skipifzero.hpp>
#include <skipifzero_hash_maps.hpp>
#include <skipifzero_image_view.hpp>
#include <skipifzero_math.hpp>
#include <skipifzero_strings.hpp>

// About
// ------------------------------------------------------------------------------------------------

// ZeroUI - An immediate mode (~ish) game ui library
//
// This is an immediate mode UI library which fills a specific niche, in-game (i.e. not tool) UI
// primarily targeting gamepads.
//
// Design goals:
//  * Primarily gamepad as input, but compatible with mouse/touch
//  * User owns all "UI" state, easy to rollback
//  * Immediate-mode~ish, API's designed similarly to dear-imgui and such where it makes sense
//  * Easy to create custom rendering functions for widgets
//  * Easy to extend with custom widgets
//  * API/engine independent, output is vertices to stream to GPU
//  * KISS - Keep It Simple Stupid
//  * As always, no exceptions, no RTTI, no STL, no object-oriented programming.

namespace zui {

using sfz::ImageViewConst;
using sfz::mat34;
using sfz::str48;
using sfz::strID;
using sfz::vec2;
using sfz::vec2_u32;
using sfz::vec3;
using sfz::vec4;

// ZeroUI Context + initialization
// ------------------------------------------------------------------------------------------------

// These are the main init/deinit functions for the ZeroUI library. Should usually be called once
// (each) during the duration of the program.
//
// ZeroUI will only allocate dynamic memory using the provided allocator, and it will in-turn
// allocate heaps for internal arena allocators for each surface. The size of these heaps (per
// surface) is determined by surfaceTmpMemoryBytes. If you start getting nullptr exceptions because
// you are running out of memory you likely need to increase your alloted budget provided in
// initZeroUI().

// Forward declare Context
struct Context;

void initZeroUI(SfzAllocator* allocator, u32 surfaceTmpMemoryBytes, u32 oversampleFonts = 1);
void deinitZeroUI();

// Fonts
// ------------------------------------------------------------------------------------------------

// ZeroUI uses a font atlas to render text (currently powered by fontstash and stb_truetype).
//
// You must provide ZeroUI with fonts (at least 1) and select which font is your default font. The
// default font will be available as an attribute in the attribute set with the id "default_font".
//
// You must also provide ZeroUI with an engine specific texture handle (u64) so ZeroUI can
// request that the font texture is bound and sampled from in your engine integration. You must
// also call "hasFontTextureUpdate()" and "getFontTexture()" each frame to check if the font atlas
// has been updated and need to be re-uploaded to the GPU (which you are responsible for).

void setFontTextureHandle(u64 handle);
bool registerFont(const char* name, const char* path, f32 size, bool defaultFont = false);

// RenderDataView
// ------------------------------------------------------------------------------------------------

// This is the output from ZeroUI when rendering, and therefore what needs to be integrated and
// hooked up into your engine.

struct Vertex final {
	vec3 pos; // Position, world or view space depending on what surface it is rendered to
	vec2 texcoord;
	vec3 colorLinear; // Color in linear space, i.e. NOT sRGB. (Note: This will likely change to sRGB in future)
	f32 alphaLinear;
};
static_assert(sizeof(Vertex) == 36, "ZeroUI::Vertex is padded");

struct RenderCmd final {
	u32 startIndex = 0;
	u32 numIndices = 0;
	mat34 transform = mat34::identity();

	// Engine specific handle to image that should be bound and sampled from. When rendering text
	// this will be set to the handle specified by setFontTextureHandle(). Note that we assume
	// that 0 is an invalid handle, if it's a valid handle in your engine you need to manage that
	// somehow.
	u64 imageHandle = 0; 

	// How the texture should be interpreted. For an alpha texture it is assumed that the alpha
	// is stored in the red channel. This is mainly used to sample from the font atlas when
	// rendering text.
	bool isAlphaTexture = false;
};

struct RenderDataView final {
	const Vertex* vertices = nullptr;
	const uint16_t* indices = nullptr;
	const RenderCmd* commands = nullptr;
	u32 numVertices = 0;
	u32 numIndices = 0;
	u32 numCommands = 0;
};

// Input
// ------------------------------------------------------------------------------------------------

// This is the user input to be delivered to ZeroUI. You are completely responsible for parsing
// your raw input and generating this yourself.
//
// The general idea is that we can only have one "type" of input each update (which simplifies a
// bunch of widget logic). I.e., we can't change which widget is focused and activate it the same
// update.
//
// pointerPos is the position of the mouse cursor (or finger touching the screen), it is only read
// if the input action is POINTER_MOVE.

enum class InputAction : u8 {
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
	vec2 pointerPos = vec2(-F32_MAX); // In screen pixel dimensions, (0,0) in lower-left corner, pos-y up.
};

// Attributes
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
	
	u8 bytes[31] = {};
	u8 size = 0;

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

// Register default attributes that will be in the intial AttributeSet when the recursive rendering
// of the widgets starts. It is highly recommended that you register default attributes for any
// and all attributes your widget rendering function reads, to guarantee that they are available.
void registerDefaultAttribute(const char* attribName, const char* value);
void registerDefaultAttribute(const char* attribName, Attribute attribute);

// Alignment
// ------------------------------------------------------------------------------------------------

// Alignment types, used to specify how a coordinate should be interpreted. E.g., given a coordinate
// (x,y) we could interpret is as being in the center of the object (H_CENTER, V_CENTER), or maybe
// in the lower-left corner of the object (LEFT, BOTTOM).

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

// A simple 2D dimensional bounding box (AABB) used to specify what space a given widget takes up.

struct Box final {
	vec2 min = vec2(0.0f);
	vec2 max = vec2(0.0f);

	constexpr Box() = default;
	constexpr Box(vec2 center, vec2 dims) : min(center - dims * 0.5f), max(center + dims * 0.5f) { }
	constexpr Box(f32 x, f32 y, f32 w, f32 h) : Box(vec2(x, y), vec2(w, h)) { }

	constexpr vec2 center() const { return (min + max) * 0.5f; }
	constexpr vec2 dims() const { return max - min; }
	constexpr f32 width() const { return max.x - min.x; }
	constexpr f32 height() const{ return max.y - min.y; }

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

// A Widget is a type of UI item, e.g. button. In ZeroUI there is only one "built-in" widget (base
// container), all other widgets are added as extensions. Thus there is no difference between a
// widget created by the user and the ones shipped with the library (see ZeroUI_CoreWidgets.hpp).
//
// There are two distinct types of widgets, containers and leafs. The difference is simple,
// containers contain other widgets, while leafs do not. E.g., a button widget is typically a leaf,
// if it is activated it simply performs some user specified logic. A list widget is a container,
// it contains a list of widgets, which can in turn be other containers or leafs.
//
// To create a widget you need to do 3 things:
// * Design a suitable immediate-mode (~ish) API, e.g. ("if (zui::button("Button")) { /* do work*/ }")
// * Design a data struct that contains all the information needed by the widget.
// * Implement the above API function, in addition to the below specified logic and rendering functions.
//
// For the API it's suggested to take a look at ZeroUI_CoreWidgets.hpp to see how the API works for
// those widgets, including looking at the implementation to get a feel for what is appropriate.
// In the end, these API functions are responsible for registering/deregistering the widget with
// the current surface in the correct way, so it's the most important thing to get right.
//
// Each Widget type need to define a data struct that contains all data needed for the widget. This
// includes logic, current state, configuration parameters, rendering info, etc. See
// "BaseContainerData" below as an example. What data it should contain is completely up to you,
// and depends on what your implementation of input handling and rendering requires.
//
// There are a couple of common rules that should apply for most (or all) widget data structs:
// * They should only contain primitive types and be default copyable (e.g. via memcpy()).
// * They must contain a copy of WidgetBase (see below)
// * The user should NEVER need to initialize a data struct, just create a copy (and run the default
//   constructor). All parameters should be applied via the widget API function.
//
// Lastly, for each widget the logic needs to be implemented. This is done by implementing the
// functions in "WidgetDesc". Not all functions are necessary for all widgets. There is no need
// for leafs to implement "GetNextWidgetBoxFunc" as they don't contain widgets. For containers
// all functions need to be specified, but many of them can often be "passthroughs" (i.e., just
// calling their childrens corresponding functions).

struct WidgetBase final {
	Box box; // The location and size of the widget on the surface
	f32 timeSinceFocusStartedSecs = F32_MAX;
	f32 timeSinceFocusEndedSecs = F32_MAX;
	f32 timeSinceActivationSecs = F32_MAX;
	bool focused = false;
	bool activated = false;
	u8 ___PADDING___[2] = {};

	void incrementTimers(f32 deltaSecs);
	void setFocused();
	void setUnfocused();
	void setActivated();
};

// Wrapper type for internal use (or when implementing widget logic), see ZeroUI_Internal.hpp.
struct WidgetCmd;

// Return the WidgetBase given a pointer to the widget data
using GetWidgetBaseFunc = WidgetBase*(void* widgetData);

// For containers only: Return the position and size of the next widget being placed. Widgets don't
// determine their own size, they must always ask their parent what their size is.
using GetNextWidgetBoxFunc = void(WidgetCmd* cmd, strID childID, Box* boxOut);

// Handle mouse/touch input. Typically things should be setFocused() if the pointer is overlapping the
// widget's box, otherwise setUnfocused(). MUST call all children's corresponding handle pointer input
// function.
using HandlePointerInputFunc = void(WidgetCmd* cmd, vec2 pointerPosSS);

// Handle move input (i.e. gamepad or keyboard). This is probably the most complicated part of the
// logic, so it deserves some extra explanation.
//
// The "input" and "moveActive" parameters are global and shared with the entire widget tree when
// recursively executing this function. "input" starts of with action specified by the user (NONE,
// UP, DOWN, LEFT, RIGHT), "moveActive" starts of as "false". The idea is that the logic can
// "consume" the input, but only if it is active.
//
// As an example, take a button which can be focused. If a button comes across the input DOWN with
// moveActive=false, then it calls setUnfocused() and sets moveActive to true. On the other hand,
// if it comes across DOWN with moveActive=true it calls setFocused() and then sets input to NONE
// and moveActive to false. The input is consumed and will not affect any upcoming widgets.
//
// MUST call all children's corresponding move input functions the correct way (see other containers
// in ZeroUI_CoreWidgets.hpp as example).
using HandleMoveInputFunc = void(WidgetCmd* cmd, Input* input, bool* moveActive);

// Renders the widget given its current data. Container's must render all their children, but they
// have the choice whether to render themselves before or after their children.
//
// This function is also the only place where the AttributeSet can be modified. However, it's
// important to not forget to restore the AttributeSet when the rendering of children is done. See
// the implementation of base container as an example.
using DrawFunc = void(
	const WidgetCmd* cmd,
	AttributeSet* attributes,
	const mat34& surfaceTransform, // Note: Should normally not be modified by container widgets.
	f32 lagSinceSurfaceEndSecs);

// Default draw function which is just a pass through, only useful for containers.
void defaultPassthroughDrawFunc(
	const WidgetCmd* cmd,
	AttributeSet* attributes,
	const mat34& surfaceTransform,
	f32 lagSinceSurfaceEndSecs);

// The description of a widget with all the necessary functions.
struct WidgetDesc final {
	u32 widgetDataSizeBytes = 0;
	GetWidgetBaseFunc* getWidgetBaseFunc = nullptr;
	GetNextWidgetBoxFunc* getNextWidgetBoxFunc = nullptr;
	HandlePointerInputFunc* handlePointerInputFunc = nullptr;
	HandleMoveInputFunc* handleMoveInputFunc = nullptr;
	DrawFunc* drawFunc = defaultPassthroughDrawFunc;
};

// Registers a widget type
void registerWidget(const char* name, const WidgetDesc& desc);

// Archetypes
// ------------------------------------------------------------------------------------------------

// Archetypes are a way to create custom rendering functions for existing widgets without having to
// create an entirely new widget. This is the primary mechanism for skinning the UI and creating
// "juicy" effects. Most users are expected to eventually create their own custom archetypes.
//
// Essentially, copy the implementation of the default renderer function for a given widget. Then
// modify it to whatever you want (maybe shake for a few seconds after it has been activated?). Then
// register the custom rendering function as an archetype using registerArchetype().
//
// Then, when entering entering widgets between surfaceBegin() and surfaceEnd() you can at any time
// call "pushArchetype()" or "popArchetype() to change which archetype is currently active for a
// given widget type.

void registerArchetype(const char* widgetName, const char* archetypeName, DrawFunc* drawFunc);
void pushArchetype(const char* widgetName, const char* archetypeName);
void popArchetype(const char* widgetName);

// Input & rendering functions
// ------------------------------------------------------------------------------------------------

// === IMPORTANT ===
//
// ALL POINTERS PASSED TO ZEROUI AFTER surfaceBegin() MUST REMAIN VALID UNTIL surfaceEnd(). This
// includes any and all pointers inside any structs passed to ZeroUI.
//
// The motivation behind this is that it might not be possible to know how a specific data struct
// need to be modified until later ones have been observed. If data owned by the user can't be
// assumed to be valid some more complicated scheme where it is copied and then returned back next
// tick must be used, which did not seem worth the complexity.
//
// If using e.g. ZeroUI_Storage.hpp this is done automatically as long as the storage isn't moved,
// this is the recommended solution.
//
// === IMPORTANT ===
//
//
// ZeroUI takes inputs and renders in terms of "surfaces". A surface is a 2D dimensional surface
// that widgets can be placed upon. Each surface is evaluated individually (i.e. given input and
// rendered). This way it is possible to have different input sources for different surfaces, and
// even run them at different update rates. It's also possible to have completely different storage
// for the state of the widget of different surfaces.

// Functions used when automatically allocating widget data structs. See ZeroUI_Storage.hpp for a
// default implementation.
using InitWidgetFunc = void(void* widgetData);
using GetWidgetDataFunc = void* (void* userPtr, strID id, u32 sizeBytes, InitWidgetFunc* initFunc);

struct SurfaceDesc final {

	// Name of surface
	// Used to identify the surface. Any previous surfaces with the same name will be cleared.
	str48 name;

	// Function used to allocate/get previous widget data structs. Optional, but highly recommended.
	GetWidgetDataFunc* getWidgetDataFunc = nullptr;
	void* widgetDataFuncUserPtr = nullptr;

	// Input
	Input input;
	vec2_u32 fbDims = vec2_u32(0u);
	f32 deltaTimeSecs = 0.0f;

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
void render(f32 lagSinceSurfaceEndSecs);

// Font texture communication, should be called after rendering.
bool hasFontTextureUpdate();
ImageViewConst getFontTexture();

// Render data guaranteed to be valid until next time render() is called
RenderDataView getRenderData();

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

void baseSetPos(f32 x, f32 y);
void baseSetPos(vec2 pos);
void baseSetAlign(HAlign halign, VAlign valign);
void baseSetAlign(Align align);
void baseSetDims(f32 width, f32 height);
void baseSetDims(vec2 dims);

void baseSet(f32 x, f32 y, f32 width, f32 height);
void baseSet(vec2 pos, vec2 dims);
void baseSet(f32 x, f32 y, HAlign halign, VAlign valign, f32 width, f32 height);
void baseSet(vec2 pos, Align align, vec2 dims);

void baseEnd();

} // namespace zui
