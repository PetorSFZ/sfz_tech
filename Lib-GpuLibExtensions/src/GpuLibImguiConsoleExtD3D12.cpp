// Copyright (c) Peter Hillerström 2022 (skipifzero.com, peter@hstroem.se)
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

#include "GpuLibImguiConsoleExtD3D12.h"

#include <imgui.h>

#include <gpu_lib_internal_d3d12.hpp>

// Console D3D12 State
// ------------------------------------------------------------------------------------------------

struct GpuLibConsoleD3D12State {

};

// Statics
// ------------------------------------------------------------------------------------------------

template<typename Fun>
static void alignedEdit(const char* name, const char* unique, u32 idx, f32 x_offset, Fun editor)
{
	ImGui::Text("%s:", name);
	ImGui::SameLine(x_offset);
	editor(sfzStr320InitFmt("##%u_%s_%s", idx, name, unique).str);
}

static void gpuKernelConsole(GpuLib* gpu)
{
	const sfz::PoolSlot* kernel_slots = gpu->kernels.slots();
	GpuKernelInfo* kernel_infos = gpu->kernels.data();
	const u32 max_num_kernels = gpu->kernels.arraySize();
	for (u32 idx = 0; idx < max_num_kernels; idx++) {
		const sfz::PoolSlot slot = kernel_slots[idx];
		if (!slot.active()) continue;
		GpuKernelInfo& kernel_info = kernel_infos[idx];
		const GpuKernel kernel = GpuKernel{ gpu->kernels.getHandle(idx).bits };

		// Reload button
		if (ImGui::Button(sfzStr96InitFmt("Reload##__shader%u", idx).str, f32x2_init(80.0f, 0.0f))) {
			gpuFlushSubmittedWork(gpu);
			gpuKernelReload(gpu, kernel);
		}
		ImGui::SameLine();

		// Shader name
		ImGui::Text(kernel_info.desc.name);
		//if (!ImGui::CollapsingHeader(kernel_info.desc.name)) continue;
	}
}

static void gpuTexturesConsole(GpuLib* gpu)
{
	const sfz::PoolSlot* tex_slots = gpu->textures.slots();
	GpuTexInfo* tex_infos = gpu->textures.data();
	const u32 max_num_textures = gpu->textures.arraySize();
	for (u32 idx = GPU_SWAPCHAIN_TEX_IDX + 1; idx < max_num_textures; idx++) {
		const sfz::PoolSlot slot = tex_slots[idx];
		if (!slot.active()) continue;
		GpuTexInfo& tex_info = tex_infos[idx];

		// Texture name
		if (!ImGui::CollapsingHeader(sfzStr96InitFmt("(%u) %s", idx, tex_info.desc.name).str)) continue;
		ImGui::Indent(20.0f);

		constexpr f32 X_OFFSET = 240.0f;
		alignedEdit("Resolution", "", idx, X_OFFSET, [&](const char*) {
			ImGui::Text("%ix%i", tex_info.tex_res.x, tex_info.tex_res.y);
		});

		alignedEdit("Format", "", idx, X_OFFSET, [&](const char*) {
			ImGui::Text("%s", gpuFormatToString(tex_info.desc.format));
		});

		alignedEdit("Num mips", "", idx, X_OFFSET, [&](const char*) {
			ImGui::Text("%i", tex_info.desc.num_mips);
		});

		if (tex_info.desc.swapchain_relative) {
			alignedEdit("Swapchain relative", "", idx, X_OFFSET, [&](const char*) {
				ImGui::Text("True");
			});

			alignedEdit("Relative fixed height", "", idx, X_OFFSET, [&](const char*) {
				ImGui::Text("%i", tex_info.desc.relative_fixed_height);
			});

			alignedEdit("Relative scale", "", idx, X_OFFSET, [&](const char*) {
				ImGui::Text("%.2f", tex_info.desc.relative_scale);
			});
		}

		alignedEdit("State", "", idx, X_OFFSET, [&](const char*) {
			ImGui::Text("%s", gpuTexStateToString(tex_info.desc.tex_state));
		});

		// Print the actual image
		const f32 aspect = f32(tex_info.tex_res.x) / f32(tex_info.tex_res.y);
		const f32 image_width = ImGui::GetWindowWidth() - 100.0f;
		const f32 image_height = image_width / aspect;
		ImTextureID id = ImTextureID(u64(idx));
		ImGui::Image(id, f32x2_init(image_width, image_height));

		ImGui::Unindent(20.0f);
	}
}

// ImGui Console D3D12 Native Extension
// ------------------------------------------------------------------------------------------------

static void imguiConsoleD3D12Run(GpuLib* gpu, void* ext_data_ptr, void* params_in, u32 params_size)
{
	sfz_assert(ext_data_ptr != nullptr);
	GpuLibConsoleD3D12State& state = *static_cast<GpuLibConsoleD3D12State*>(ext_data_ptr);
	sfz_assert(params_in != nullptr);
	sfz_assert(params_size == sizeof(ImGuiConsoleD3D12ExtParams));
	ImGuiConsoleD3D12ExtParams& params = *static_cast<ImGuiConsoleD3D12ExtParams*>(params_in);

	// Begin window
	ImGui::Begin(params.window_name.str, nullptr, ImGuiWindowFlags_NoFocusOnAppearing);
	sfz_defer[]() { ImGui::End(); };

	(void)state;

	if (ImGui::BeginTabBar("ResourcesTabBar", ImGuiTabBarFlags_None)) {

		if (ImGui::BeginTabItem("Kernels")) {
			ImGui::Spacing();
			gpuKernelConsole(gpu);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Textures")) {
			ImGui::Spacing();
			gpuTexturesConsole(gpu);
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}
}

static void imguiConsoleD3D12Destroy(GpuLib* gpu, void* ext_data_ptr)
{
	sfz_assert(ext_data_ptr != nullptr);
	GpuLibConsoleD3D12State* state = static_cast<GpuLibConsoleD3D12State*>(ext_data_ptr);
	sfz_delete(gpu->cfg.cpu_allocator, state);
}

sfz_extern_c GpuNativeExt imguiConsoleExtD3D12Init(GpuLib* gpu)
{
	GpuLibConsoleD3D12State* state =
		sfz_new<GpuLibConsoleD3D12State>(gpu->cfg.cpu_allocator, sfz_dbg("GpuLibConsoleD3D12State"));

	GpuNativeExt ext = {};
	ext.ext_data_ptr = state;
	ext.run_func = imguiConsoleD3D12Run;
	ext.destroy_func = imguiConsoleD3D12Destroy;
	return ext;
}
