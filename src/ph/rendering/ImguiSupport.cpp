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

#include <imgui.h>

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
