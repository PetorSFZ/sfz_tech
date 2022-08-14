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

#include "sfz/audio/AudioEngine.hpp"

#include <imgui.h>

#include <soloud.h>

#include <sfz.h>
#include <sfz_cpp.hpp>

// Types
// ------------------------------------------------------------------------------------------------

sfz_struct(SfzAudioEngine) {
	SfzAllocator* allocator = nullptr;
	SoLoud::Soloud soloud;
};

// AudioEngine
// ------------------------------------------------------------------------------------------------

sfz_extern_c SfzAudioEngine* sfzAudioCreate(SfzAllocator* allocator)
{
	SfzAudioEngine* audio = sfz_new<SfzAudioEngine>(allocator, sfz_dbg(""));
	audio->allocator = allocator;

	// Initialize SoLoud
	audio->soloud.init();

	return audio;
}

sfz_extern_c void sfzAudioDestroy(SfzAudioEngine* audio)
{
	if (audio == nullptr) return;
	SfzAllocator* allocator = audio->allocator;

	// Deinitialize SoLoud
	audio->soloud.deinit();

	sfz_delete(allocator, audio);
}

sfz_extern_c void sfzAudioRenderDebugUI(SfzAudioEngine* audio)
{
	(void)audio;
	ImGuiWindowFlags windowFlags = 0;
	windowFlags |= ImGuiWindowFlags_NoFocusOnAppearing;
	if (!ImGui::Begin("Audio", nullptr, windowFlags)) {
		ImGui::End();
		return;
	}

	ImGui::End();
}
