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

#include "ph/rendering/ImguiSupport.hpp"

#include <SDL.h>

#include <sfz/math/MathSupport.hpp>

#include <ph/config/GlobalConfig.hpp>
#include <ph/Context.hpp>

namespace ph {

using sfz::vec2;
using sfz::vec4;

static void* imguiAllocFunc(size_t size, void* userData) noexcept
{
	Allocator* allocator = reinterpret_cast<Allocator*>(userData);
	return allocator->allocate(sfz_dbg("Imgui"), size, 32);
}

static void imguiFreeFunc(void* ptr, void* userData) noexcept
{
	Allocator* allocator = reinterpret_cast<Allocator*>(userData);
	allocator->deallocate(ptr);
}

struct ImGuiState final {
	Allocator* allocator = nullptr;
	ImFont* defaultFont = nullptr;
	ImFont* monospaceFont = nullptr;
};

static ImGuiState* imguiState = nullptr;

phImageView initializeImgui(Allocator* allocator) noexcept
{
	// Replace Imgui allocators with sfz::Allocator
	ImGui::SetAllocatorFunctions(imguiAllocFunc, imguiFreeFunc, allocator);

	// Allocate imgui state
	imguiState = allocator->newObject<ImGuiState>(sfz_dbg("ImGuiState"));
	imguiState->allocator = allocator;

	// Create Imgui context
	ImGui::CreateContext();

	// Request modified dark style
	ImGuiStyle style;
	ImGui::StyleColorsDark(&style);

	style.Alpha = 1.0f;
	style.WindowPadding = vec2(12.0f);
	style.WindowRounding = 4.0f;
	style.FramePadding = vec2(8.0f, 5.0f);
	style.ItemSpacing = vec2(12.0f, 8.0f);
	style.ItemInnerSpacing = vec2(6.0f);
	style.IndentSpacing = 30.0f;
	style.ScrollbarSize = 12.0f;
	style.ScrollbarRounding = 5.0f;
	style.AntiAliasedLines = true;
	style.AntiAliasedFill = true;

	//style.Colors[ImGuiCol_Text];
	//style.Colors[ImGuiCol_TextDisabled];
	style.Colors[ImGuiCol_WindowBg] = vec4(0.05f, 0.05f, 0.05f, 0.75f);
	//style.Colors[ImGuiCol_ChildBg];               // Background of child windows
	//style.Colors[ImGuiCol_PopupBg];               // Background of popups, menus, tooltips windows
	//style.Colors[ImGuiCol_Border];
	//style.Colors[ImGuiCol_BorderShadow];
	//style.Colors[ImGuiCol_FrameBg];               // Background of checkbox, radio button, plot, slider, text input
	//style.Colors[ImGuiCol_FrameBgHovered];
	//style.Colors[ImGuiCol_FrameBgActive];
	//style.Colors[ImGuiCol_TitleBg];
	//style.Colors[ImGuiCol_TitleBgActive];
	//style.Colors[ImGuiCol_TitleBgCollapsed];
	//style.Colors[ImGuiCol_MenuBarBg];
	//style.Colors[ImGuiCol_ScrollbarBg];
	//style.Colors[ImGuiCol_ScrollbarGrab];
	//style.Colors[ImGuiCol_ScrollbarGrabHovered];
	//style.Colors[ImGuiCol_ScrollbarGrabActive];
	//style.Colors[ImGuiCol_CheckMark];
	//style.Colors[ImGuiCol_SliderGrab];
	//style.Colors[ImGuiCol_SliderGrabActive];
	//style.Colors[ImGuiCol_Button];
	//style.Colors[ImGuiCol_ButtonHovered];
	//style.Colors[ImGuiCol_ButtonActive];
	//style.Colors[ImGuiCol_Header];
	//style.Colors[ImGuiCol_HeaderHovered];
	//style.Colors[ImGuiCol_HeaderActive];
	//style.Colors[ImGuiCol_Separator];
	//style.Colors[ImGuiCol_SeparatorHovered];
	//style.Colors[ImGuiCol_SeparatorActive];
	//style.Colors[ImGuiCol_ResizeGrip];
	//style.Colors[ImGuiCol_ResizeGripHovered];
	//style.Colors[ImGuiCol_ResizeGripActive];
	//style.Colors[ImGuiCol_Tab];
	//style.Colors[ImGuiCol_TabHovered];
	//style.Colors[ImGuiCol_TabActive];
	//style.Colors[ImGuiCol_TabUnfocused];
	//style.Colors[ImGuiCol_TabUnfocusedActive];
	//style.Colors[ImGuiCol_DockingPreview];
	//style.Colors[ImGuiCol_DockingEmptyBg];        // Background color for empty node (e.g. CentralNode with no window docked into it)
	//style.Colors[ImGuiCol_PlotLines];
	//style.Colors[ImGuiCol_PlotLinesHovered];
	//style.Colors[ImGuiCol_PlotHistogram];
	//style.Colors[ImGuiCol_PlotHistogramHovered];
	//style.Colors[ImGuiCol_TextSelectedBg];
	//style.Colors[ImGuiCol_DragDropTarget];
	//style.Colors[ImGuiCol_NavHighlight];          // Gamepad/keyboard: current highlighted item
	//style.Colors[ImGuiCol_NavWindowingHighlight]; // Highlight window when using CTRL+TAB
	//style.Colors[ImGuiCol_NavWindowingDimBg];     // Darken/colorize entire screen behind the CTRL+TAB window list, when active
	//style.Colors[ImGuiCol_ModalWindowDimBg];      // Darken/colorize entire screen behind a modal window, when one is active

	ImGui::GetStyle() = style;

	ImGuiIO& io = ImGui::GetIO();

	// Enable GamePad navigation
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

	// Enable keyboard navigation
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	// Enable docking
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigDockingWithShift = false; // No need to hold shift to dock windows

	// Allow resizing windows from edges
	io.ConfigWindowsResizeFromEdges = true;

	// Enable mouse cursors (i.e., mouse cursor is changed depending on what is hovered over)
	io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;

	// Disable draw function and set all window sizes to 1 (will be set proper in update)
	io.DisplaySize = vec2(1.0f);
	io.DisplayFramebufferScale = vec2(1.0f);

	// Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
	io.KeyMap[ImGuiKey_Tab] = SDLK_TAB;
	io.KeyMap[ImGuiKey_LeftArrow] = SDL_SCANCODE_LEFT;
	io.KeyMap[ImGuiKey_RightArrow] = SDL_SCANCODE_RIGHT;
	io.KeyMap[ImGuiKey_UpArrow] = SDL_SCANCODE_UP;
	io.KeyMap[ImGuiKey_DownArrow] = SDL_SCANCODE_DOWN;
	io.KeyMap[ImGuiKey_PageUp] = SDL_SCANCODE_PAGEUP;
	io.KeyMap[ImGuiKey_PageDown] = SDL_SCANCODE_PAGEDOWN;
	io.KeyMap[ImGuiKey_Home] = SDL_SCANCODE_HOME;
	io.KeyMap[ImGuiKey_End] = SDL_SCANCODE_END;
	io.KeyMap[ImGuiKey_Insert] = SDL_SCANCODE_INSERT;
	io.KeyMap[ImGuiKey_Delete] = SDLK_DELETE;
	io.KeyMap[ImGuiKey_Backspace] = SDLK_BACKSPACE;
	io.KeyMap[ImGuiKey_Space] = SDLK_SPACE;
	io.KeyMap[ImGuiKey_Enter] = SDLK_RETURN;
	io.KeyMap[ImGuiKey_Escape] = SDLK_ESCAPE;
	io.KeyMap[ImGuiKey_A] = SDLK_a;
	io.KeyMap[ImGuiKey_C] = SDLK_c;
	io.KeyMap[ImGuiKey_V] = SDLK_v;
	io.KeyMap[ImGuiKey_X] = SDLK_x;
	io.KeyMap[ImGuiKey_Y] = SDLK_y;
	io.KeyMap[ImGuiKey_Z] = SDLK_z;

	// Add font
	const float FONT_SIZE_PIXELS = 16.0f;
	//const char* DEFAULT_FONT_PATH = "res_ph/fonts/source_sans_pro/SourceSansPro-Regular.ttf";
	const char* DEFAULT_FONT_PATH = "res_ph/fonts/source_code_pro/SourceCodePro-Regular.ttf";
	const char* SECONDARY_FONT_PATH = "res_ph/fonts/source_code_pro/SourceCodePro-Regular.ttf";
	ImFontConfig fontConfig;
	fontConfig.OversampleH = 4;
	fontConfig.OversampleV = 4;
	fontConfig.GlyphExtraSpacing = vec2(1.0f);
	imguiState->defaultFont =
		io.Fonts->AddFontFromFileTTF(DEFAULT_FONT_PATH, FONT_SIZE_PIXELS, &fontConfig);
	imguiState->monospaceFont =
		io.Fonts->AddFontFromFileTTF(SECONDARY_FONT_PATH, FONT_SIZE_PIXELS, &fontConfig);

	// Rasterize default font and return view
	phImageView fontTexView;
	io.Fonts->GetTexDataAsAlpha8(&fontTexView.rawData, &fontTexView.width, &fontTexView.height);
	fontTexView.type = ImageType::R_U8;
	return fontTexView;
}

void deinitializeImgui() noexcept
{
	ImGui::DestroyContext();
	Allocator* allocator = imguiState->allocator;
	allocator->deleteObject(imguiState);
}

void updateImgui(
	Renderer& renderer,
	const sdl::Mouse* rawMouse,
	const DynArray<SDL_Event>* keyboardEvents,
	const sdl::GameControllerState* controller) noexcept
{
	// Note, these should actually be freed using SDL_FreeCursor(). But I don't think it matters
	// that much.
	static SDL_Cursor* MOUSE_CURSORS[ImGuiMouseCursor_COUNT] = {};
	static bool sdlCursorsInitialized = [&]() {
		MOUSE_CURSORS[ImGuiMouseCursor_Arrow] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
		MOUSE_CURSORS[ImGuiMouseCursor_TextInput] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
		MOUSE_CURSORS[ImGuiMouseCursor_ResizeAll] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
		MOUSE_CURSORS[ImGuiMouseCursor_ResizeNS] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
		MOUSE_CURSORS[ImGuiMouseCursor_ResizeEW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
		MOUSE_CURSORS[ImGuiMouseCursor_ResizeNESW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENESW);
		MOUSE_CURSORS[ImGuiMouseCursor_ResizeNWSE] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE);
		MOUSE_CURSORS[ImGuiMouseCursor_Hand] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
		return true;
	}();

	static Setting* invertedScrollSetting = nullptr;
	if (invertedScrollSetting == nullptr) {
		GlobalConfig& cfg = getGlobalConfig();
		bool defaultVal = false;
#ifdef __APPLE__
		defaultVal = true;
#endif
		invertedScrollSetting = cfg.sanitizeBool("Imgui", "invertMouseScrollY", true, defaultVal);
	}

	ImGuiIO& io = ImGui::GetIO();


	// Retrieve scale factor from config
	ph::GlobalConfig& cfg = ph::getGlobalConfig();
	const ph::Setting* imguiScaleSetting =
		cfg.sanitizeFloat("Imgui", "scale", true, ph::FloatBounds(2.0f, 1.0f, 3.0f));
	float scaleFactor = 1.0f / imguiScaleSetting->floatValue();

	// Set display dimensions
	vec2 imguiDims = vec2(renderer.windowResolution()) * scaleFactor;
	io.DisplaySize = imguiDims;

	// Update mouse if available
	if (rawMouse != nullptr) {
		sdl::Mouse imguiMouse = rawMouse->scaleMouse(imguiDims * 0.5f, imguiDims);
		io.MousePos.x = imguiMouse.position.x;
		io.MousePos.y = imguiDims.y - imguiMouse.position.y;

		io.MouseDown[0] = imguiMouse.leftButton != sdl::ButtonState::NOT_PRESSED;
		io.MouseDown[1] = imguiMouse.rightButton != sdl::ButtonState::NOT_PRESSED;
		io.MouseDown[2] = imguiMouse.middleButton != sdl::ButtonState::NOT_PRESSED;

		if (invertedScrollSetting->boolValue()) {
			io.MouseWheel = -imguiMouse.wheel.y;
		}
		else {
			io.MouseWheel = imguiMouse.wheel.y;
		}
	}
	else {
		io.MousePos.x = -FLT_MAX;
		io.MousePos.y = -FLT_MAX;
		io.MouseDown[0] = false;
		io.MouseDown[1] = false;
		io.MouseDown[2] = false;
		io.MouseWheel = 0.0f;
	}

	// Update mouse cursor
	if (sdlCursorsInitialized) {
		ImGuiMouseCursor cursor = ImGui::GetMouseCursor();

		// Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
		if (io.MouseDrawCursor || cursor == ImGuiMouseCursor_None) {
			SDL_ShowCursor(SDL_FALSE);
		}

		// Show OS mouse cursor
		else{
			SDL_Cursor* sdlCursor = MOUSE_CURSORS[cursor] ?
				MOUSE_CURSORS[cursor] : MOUSE_CURSORS[ImGuiMouseCursor_Arrow];
			SDL_SetCursor(sdlCursor);
			SDL_ShowCursor(SDL_TRUE);
		}
	}

	// Keyboard events
	for (const SDL_Event& event : *keyboardEvents) {
		switch (event.type) {
		case SDL_TEXTINPUT:
			io.AddInputCharactersUTF8(event.text.text);
			break;
		case SDL_KEYDOWN:
		case SDL_KEYUP:
			int key = event.key.keysym.sym & ~SDLK_SCANCODE_MASK;
			io.KeysDown[key] = (event.type == SDL_KEYDOWN);
			io.KeyShift = ((SDL_GetModState() & KMOD_SHIFT) != 0);
			io.KeyCtrl = ((SDL_GetModState() & KMOD_CTRL) != 0);
			io.KeyAlt = ((SDL_GetModState() & KMOD_ALT) != 0);
			io.KeySuper = ((SDL_GetModState() & KMOD_GUI) != 0);
			break;
		}
	}

	// Controller input
	if (controller != nullptr) {
		const sdl::GameControllerState& c = *controller;

		auto buttonToImgui = [](const sdl::ButtonState& button) -> float {
			if (button != sdl::ButtonState::NOT_PRESSED) return 1.0f;
			else return 0.0f;
		};

		// press button, tweak value // e.g. Circle button
		io.NavInputs[ImGuiNavInput_Activate] = buttonToImgui(c.a);

		// close menu/popup/child, lose selection // e.g. Cross button
		io.NavInputs[ImGuiNavInput_Cancel] = buttonToImgui(c.b);

		// text input // e.g. Triangle button
		io.NavInputs[ImGuiNavInput_Input] = buttonToImgui(c.y);

		// access menu, focus, move, resize // e.g. Square button
		io.NavInputs[ImGuiNavInput_Menu] = buttonToImgui(c.x);

		// move / tweak / resize window (w/ PadMenu) // e.g. D-pad Left/Right/Up/Down
		io.NavInputs[ImGuiNavInput_DpadUp] = buttonToImgui(c.padUp);
		io.NavInputs[ImGuiNavInput_DpadDown] = buttonToImgui(c.padDown);
		io.NavInputs[ImGuiNavInput_DpadLeft] = buttonToImgui(c.padLeft);
		io.NavInputs[ImGuiNavInput_DpadRight] = buttonToImgui(c.padRight);

		// scroll / move window (w/ PadMenu) // e.g. Left Analog Stick Left/Right/Up/Down
		vec2 leftStick = c.leftStick;
		io.NavInputs[ImGuiNavInput_LStickUp] = sfzMax(leftStick.y, 0.0f);
		io.NavInputs[ImGuiNavInput_LStickDown] = sfz::abs(sfzMin(leftStick.y, 0.0f));
		io.NavInputs[ImGuiNavInput_LStickLeft] = sfz::abs(sfzMin(leftStick.x, 0.0f));
		io.NavInputs[ImGuiNavInput_LStickRight] = sfzMax(leftStick.x, 0.0f);

		// next window (w/ PadMenu) // e.g. L1 or L2 (PS4), LB or LT (Xbox), L or ZL (Switch)
		io.NavInputs[ImGuiNavInput_FocusPrev] = buttonToImgui(c.leftShoulder);

		// prev window (w/ PadMenu) // e.g. R1 or R2 (PS4), RB or RT (Xbox), R or ZL (Switch)
		io.NavInputs[ImGuiNavInput_FocusNext] = buttonToImgui(c.rightShoulder);

		// slower tweaks // e.g. L1 or L2 (PS4), LB or LT (Xbox), L or ZL (Switch)
		io.NavInputs[ImGuiNavInput_TweakSlow] = c.leftTrigger;

		// faster tweaks // e.g. R1 or R2 (PS4), RB or RT (Xbox), R or ZL (Switch)*
		io.NavInputs[ImGuiNavInput_TweakFast] = c.rightTrigger;
	}
}

void convertImguiDrawData(
	DynArray<phImguiVertex>& vertices,
	DynArray<uint32_t>& indices,
	DynArray<phImguiCommand>& commands) noexcept
{
	ImDrawData& drawData = *ImGui::GetDrawData();

	// Clear old data
	vertices.clear();
	indices.clear();
	commands.clear();

	// Convert draw data
	for (int i = 0; i < drawData.CmdListsCount; i++) {

		const ImDrawList& cmdList = *drawData.CmdLists[i];

		// indexOffset is the offset to offset all indices with
		const uint32_t indexOffset = vertices.size();

		// indexBufferOffset is the offset to where the indices start
		uint32_t indexBufferOffset = indices.size();

		// Convert vertices and add to global list
		for (int j = 0; j < cmdList.VtxBuffer.size(); j++) {
			const ImDrawVert& imguiVertex = cmdList.VtxBuffer[j];

			phImguiVertex convertedVertex;
			convertedVertex.pos = vec2(imguiVertex.pos.x, imguiVertex.pos.y);
			convertedVertex.texcoord = vec2(imguiVertex.uv.x, imguiVertex.uv.y);
			convertedVertex.color = imguiVertex.col;

			vertices.add(convertedVertex);
		}

		// Fix indices and add to global list
		for (int j = 0; j < cmdList.IdxBuffer.size(); j++) {
			indices.add(cmdList.IdxBuffer[j] + indexOffset);
		}

		// Create new commands
		for (int j = 0; j < cmdList.CmdBuffer.Size; j++) {
			const ImDrawCmd& inCmd = cmdList.CmdBuffer[j];

			phImguiCommand cmd;
			cmd.idxBufferOffset = indexBufferOffset;
			cmd.numIndices = inCmd.ElemCount;
			indexBufferOffset += inCmd.ElemCount;
			cmd.clipRect.x = inCmd.ClipRect.x;
			cmd.clipRect.y = inCmd.ClipRect.y;
			cmd.clipRect.z = inCmd.ClipRect.z;
			cmd.clipRect.w = inCmd.ClipRect.w;

			commands.add(cmd);
		}
	}
}

ImFont* imguiFontDefault() noexcept
{
	return imguiState->defaultFont;
}

ImFont* imguiFontMonospace() noexcept
{
	return imguiState->monospaceFont;
}

} // namespace ph
