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

#include <imgui.h>

#include <sfz/math/MathSupport.hpp>

namespace ph {

void initializeImgui(Renderer& renderer) noexcept
{
	// Imgui
	ImGuiIO& io = ImGui::GetIO();
	vec2 imguiDims = renderer.imguiWindowDimensions();
	io.DisplaySize.x = imguiDims.x;
	io.DisplaySize.y = imguiDims.y;
	io.DisplayFramebufferScale.x = 1.0f;
	io.DisplayFramebufferScale.y = 1.0f;
	io.RenderDrawListsFn = nullptr;

	ImageView fontTexView;
	io.Fonts->GetTexDataAsAlpha8(&fontTexView.rawData, &fontTexView.width, &fontTexView.height);
	fontTexView.bytesPerPixel = 1;
	fontTexView.type = ImageType::GRAY_U8;
	renderer.initImgui(fontTexView);

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
	//io.KeyMap[ImGuiKey_Insert] = SDL_SCANCODE_INSERT;
	io.KeyMap[ImGuiKey_Delete] = SDLK_DELETE;
	io.KeyMap[ImGuiKey_Backspace] = SDLK_BACKSPACE;
	io.KeyMap[ImGuiKey_Enter] = SDLK_RETURN;
	io.KeyMap[ImGuiKey_Escape] = SDLK_ESCAPE;
	io.KeyMap[ImGuiKey_A] = SDLK_a;
	io.KeyMap[ImGuiKey_C] = SDLK_c;
	io.KeyMap[ImGuiKey_V] = SDLK_v;
	io.KeyMap[ImGuiKey_X] = SDLK_x;
	io.KeyMap[ImGuiKey_Y] = SDLK_y;
	io.KeyMap[ImGuiKey_Z] = SDLK_z;
}

void updateImgui(
	Renderer& renderer,
	const sdl::Mouse* rawMouse,
	const DynArray<SDL_Event>* keyboardEvents,
	const sdl::GameControllerState* controller) noexcept
{
	ImGuiIO& io = ImGui::GetIO();

	// Set display dimensions
	vec2 imguiDims = renderer.imguiWindowDimensions();
	io.DisplaySize.x = imguiDims.x;
	io.DisplaySize.y = imguiDims.y;

	// Update mouse if available
	if (rawMouse != nullptr) {
		sdl::Mouse imguiMouse = rawMouse->scaleMouse(imguiDims * 0.5f, imguiDims);
		io.MousePos.x = imguiMouse.position.x;
		io.MousePos.y = imguiDims.y - imguiMouse.position.y;

		io.MouseDown[0] = imguiMouse.leftButton != sdl::ButtonState::NOT_PRESSED;
		io.MouseDown[1] = imguiMouse.rightButton != sdl::ButtonState::NOT_PRESSED;
		io.MouseDown[2] = imguiMouse.middleButton != sdl::ButtonState::NOT_PRESSED;

		io.MouseWheel = imguiMouse.wheel.y;
	}
	else {
		io.MousePos.x = -FLT_MAX;
		io.MousePos.y = -FLT_MAX;
		io.MouseDown[0] = false;
		io.MouseDown[1] = false;
		io.MouseDown[2] = false;
		io.MouseWheel = 0.0f;
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
		io.NavInputs[ImGuiNavInput_PadActivate] = buttonToImgui(c.a);

		// close menu/popup/child, lose selection // e.g. Cross button
		io.NavInputs[ImGuiNavInput_PadCancel] = buttonToImgui(c.b);

		// text input // e.g. Triangle button
		io.NavInputs[ImGuiNavInput_PadInput] = buttonToImgui(c.y);

		// access menu, focus, move, resize // e.g. Square button
		io.NavInputs[ImGuiNavInput_PadMenu] = buttonToImgui(c.x);

		// Attempt to fix left stick a bit
		vec2 leftStick = c.leftStick;
		if (leftStick != vec2(0.0f) && sfz::length(leftStick) < 0.6f) leftStick = vec2(0.0f);
		
		// move up, resize window (with PadMenu held) // e.g. D-pad up/down/left/right, analog
		io.NavInputs[ImGuiNavInput_PadUp] = sfz::max(leftStick.y, 0.0f);
		if (io.NavInputs[ImGuiNavInput_PadUp] == 0.0f)
			io.NavInputs[ImGuiNavInput_PadUp] = buttonToImgui(c.padUp);
		// move down
		io.NavInputs[ImGuiNavInput_PadDown] = sfz::abs(sfz::min(leftStick.y, 0.0f));
		if (io.NavInputs[ImGuiNavInput_PadDown] == 0.0f)
			io.NavInputs[ImGuiNavInput_PadDown] = buttonToImgui(c.padDown);
		// move left
		io.NavInputs[ImGuiNavInput_PadLeft] = sfz::abs(sfz::min(leftStick.x, 0.0f));
		if (io.NavInputs[ImGuiNavInput_PadLeft] == 0.0f)
			io.NavInputs[ImGuiNavInput_PadLeft] = buttonToImgui(c.padLeft);
		// move right
		io.NavInputs[ImGuiNavInput_PadRight] = sfz::max(leftStick.x, 0.0f);
		if (io.NavInputs[ImGuiNavInput_PadRight] == 0.0f)
			io.NavInputs[ImGuiNavInput_PadRight] = buttonToImgui(c.padRight);

		// scroll up, move window (with PadMenu held) // e.g. right stick up/down/left/right, analog
		io.NavInputs[ImGuiNavInput_PadScrollUp] = sfz::max(c.rightStick.y, 0.0f);
		io.NavInputs[ImGuiNavInput_PadScrollDown] = sfz::abs(sfz::min(c.rightStick.y, 0.0f));
		io.NavInputs[ImGuiNavInput_PadScrollLeft] = sfz::abs(sfz::min(c.rightStick.x, 0.0f));
		io.NavInputs[ImGuiNavInput_PadScrollRight] = sfz::max(c.rightStick.x, 0.0f);

		// next window (with PadMenu held) // e.g. L-trigger
		io.NavInputs[ImGuiNavInput_PadFocusPrev] = buttonToImgui(c.leftShoulder);

		// prev window (with PadMenu held) // e.g. R-trigger
		io.NavInputs[ImGuiNavInput_PadFocusNext] = buttonToImgui(c.rightShoulder);

		// slower tweaks // e.g. L-trigger, analog
		io.NavInputs[ImGuiNavInput_PadTweakSlow] = c.leftTrigger;

		// faster tweaks // e.g. R-trigger, analog
		io.NavInputs[ImGuiNavInput_PadTweakFast] = c.rightTrigger;

		// access menu // e.g. ALT
		io.NavInputs[ImGuiNavInput_KeyMenu] = buttonToImgui(c.start);
	}
}

void convertImguiDrawData(
	DynArray<ImguiVertex>& vertices,
	DynArray<uint32_t>& indices,
	DynArray<ImguiCommand>& commands) noexcept
{
	ImGuiIO& io = ImGui::GetIO();
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

			ImguiVertex convertedVertex;
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

			ImguiCommand cmd;
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

} // namespace ph
