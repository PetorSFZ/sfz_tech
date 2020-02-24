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

namespace sfz {

// RendererUI class
// ------------------------------------------------------------------------------------------------

struct RendererState;
struct RendererConfigurableState;

class RendererUI final {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	RendererUI() noexcept = default;
	RendererUI(const RendererUI&) = delete;
	RendererUI& operator= (const RendererUI&) = delete;
	RendererUI(RendererUI&& other) noexcept { this->swap(other); }
	RendererUI& operator= (RendererUI&& other) noexcept { this->swap(other); return *this; }
	~RendererUI() noexcept { this->destroy(); }

	// State methods
	// --------------------------------------------------------------------------------------------

	void swap(RendererUI& other) noexcept;
	void destroy() noexcept;

	// Methods
	// --------------------------------------------------------------------------------------------

	void render(RendererState& state) noexcept;

private:
	// Private methods
	// --------------------------------------------------------------------------------------------

	void renderGeneralTab(RendererState& state) noexcept;
	void renderPresentQueueTab(RendererConfigurableState& state) noexcept;
	void renderPipelinesTab(RendererState& state) noexcept;
	void renderStaticResourcesTab(RendererConfigurableState& state) noexcept;
	void renderFramebuffersTab(RendererConfigurableState& state) noexcept;
	void renderTexturesTab(RendererState& state) noexcept;
	void renderMeshesTab(RendererState& state) noexcept;
};

} // namespace sfz
