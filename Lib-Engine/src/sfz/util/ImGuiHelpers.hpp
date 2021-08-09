// Copyright (c) Peter Hillerström (skipifzero.com, peter@hstroem.se)
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
#include <skipifzero_strings.hpp>

#include <imgui.h>

namespace sfz {

// Alignment helpers
// ------------------------------------------------------------------------------------------------

template<typename Func>
inline void alignedEdit(const char* name, f32 xOffset, Func editor)
{
	ImGui::Text("%s", name);
	ImGui::SameLine(xOffset);
	editor(str96("##%s_invisible", name).str());
}

template<typename Fun>
static void alignedEdit(const char* name, const char* unique, u32 idx, f32 xOffset, Fun editor)
{
	ImGui::Text("%s:", name);
	ImGui::SameLine(xOffset);
	editor(str256("##%u_%s_%s", idx, name, unique).str());
}

// Filtered text helpers
// ------------------------------------------------------------------------------------------------

inline void imguiPrintText(const char* str, f32x4 color, const char* strEnd = nullptr)
{
	ImGui::PushStyleColor(ImGuiCol_Text, color);
	ImGui::TextUnformatted(str, strEnd);
	ImGui::PopStyleColor();
}

inline void imguiRenderFilteredText(
	const char* str,
	const char* filter,
	f32x4 stringColor,
	f32x4 filterColor)
{
	str320 lowerStackStr("%s", str);
	lowerStackStr.toLower();

	const char* currStr = str;
	const char* currLowerStr = lowerStackStr.str();
	const size_t filterLen = strlen(filter);

	if (filterLen == 0) {
		imguiPrintText(str, stringColor);
		return;
	}

	while (true) {

		const char* nextLower = strstr(currLowerStr, filter);

		// Substring found
		if (nextLower != nullptr) {

			// Render part of string until next filter
			if (nextLower != currLowerStr) {
				size_t len = nextLower - currLowerStr;
				imguiPrintText(currStr, stringColor, currStr + len);
				currStr += len;
				currLowerStr += len;
			}

			// Render filter
			else {
				imguiPrintText(currStr, filterColor, currStr + filterLen);
				currStr += filterLen;
				currLowerStr += filterLen;
			}

			ImGui::SameLine(0.0f, 2.0f);
		}

		// If no more substrings can be found it is time to render the rest of the string
		else {
			imguiPrintText(currStr, stringColor);
			return;
		}
	}
}

} // namespace sfz
