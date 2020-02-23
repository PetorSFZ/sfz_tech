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

#include "sfz/renderer/RendererUI.hpp"

#include <utility> // std::swap()

#include <imgui.h>

#include "sfz/Context.hpp"
#include "sfz/Logging.hpp"
#include "sfz/renderer/RendererState.hpp"
#include "sfz/renderer/ZeroGUtils.hpp"

namespace sfz {

using sfz::str64;

// Statics
// ------------------------------------------------------------------------------------------------

template<typename Fun>
static void alignedEdit(const char* name, float xOffset, Fun editor) noexcept
{
	ImGui::Text("%s", name);
	ImGui::SameLine(xOffset);
	editor(sfz::str96("##%s_invisible", name).str());
}

static float toGiB(uint64_t bytes) noexcept
{
	return float(bytes) / (1024.0f * 1024.0f * 1024.0f);
}

static float toMiB(uint64_t bytes) noexcept
{
	return float(bytes) / (1024.0f * 1024.0f);
}

static const char* toString(StageType type) noexcept
{
	switch (type) {
	case StageType::USER_INPUT_RENDERING: return "USER_INPUT_RENDERING";
	case StageType::USER_STAGE_BARRIER: return "USER_STAGE_BARRIER";
	}
	sfz_assert(false);
	return "<ERROR>";
}

static const char* textureFormatToString(ZgTextureFormat format) noexcept
{
	switch (format) {
	case ZG_TEXTURE_FORMAT_UNDEFINED: return "UNDEFINED";

	case ZG_TEXTURE_FORMAT_R_U8_UNORM: return "R_U8_UNORM";
	case ZG_TEXTURE_FORMAT_RG_U8_UNORM: return "RG_U8_UNORM";
	case ZG_TEXTURE_FORMAT_RGBA_U8_UNORM: return "RGBA_U8_UNORM";

	case ZG_TEXTURE_FORMAT_R_F16: return "R_F16";
	case ZG_TEXTURE_FORMAT_RG_F16: return "RG_F16";
	case ZG_TEXTURE_FORMAT_RGBA_F16: return "RGBA_F16";

	case ZG_TEXTURE_FORMAT_R_F32: return "R_F32";
	case ZG_TEXTURE_FORMAT_RG_F32: return "RG_F32";
	case ZG_TEXTURE_FORMAT_RGBA_F32: return "RGBA_F32";

	case ZG_TEXTURE_FORMAT_DEPTH_F32: return "DEPTH_F32";
	}
	sfz_assert(false);
	return "";
}

static const char* vertexAttributeTypeToString(ZgVertexAttributeType type) noexcept
{
	switch (type) {
	case ZG_VERTEX_ATTRIBUTE_F32: return "ZG_VERTEX_ATTRIBUTE_F32";
	case ZG_VERTEX_ATTRIBUTE_F32_2: return "ZG_VERTEX_ATTRIBUTE_F32_2";
	case ZG_VERTEX_ATTRIBUTE_F32_3: return "ZG_VERTEX_ATTRIBUTE_F32_3";
	case ZG_VERTEX_ATTRIBUTE_F32_4: return "ZG_VERTEX_ATTRIBUTE_F32_4";

	case ZG_VERTEX_ATTRIBUTE_S32: return "ZG_VERTEX_ATTRIBUTE_S32";
	case ZG_VERTEX_ATTRIBUTE_S32_2: return "ZG_VERTEX_ATTRIBUTE_S32_2";
	case ZG_VERTEX_ATTRIBUTE_S32_3: return "ZG_VERTEX_ATTRIBUTE_S32_3";
	case ZG_VERTEX_ATTRIBUTE_S32_4: return "ZG_VERTEX_ATTRIBUTE_S32_4";

	case ZG_VERTEX_ATTRIBUTE_U32: return "ZG_VERTEX_ATTRIBUTE_U32";
	case ZG_VERTEX_ATTRIBUTE_U32_2: return "ZG_VERTEX_ATTRIBUTE_U32_2";
	case ZG_VERTEX_ATTRIBUTE_U32_3: return "ZG_VERTEX_ATTRIBUTE_U32_3";
	case ZG_VERTEX_ATTRIBUTE_U32_4: return "ZG_VERTEX_ATTRIBUTE_U32_4";

	default: break;
	}
	sfz_assert(false);
	return "";
}

static const char* samplingModeToString(ZgSamplingMode mode) noexcept
{
	switch (mode) {
	case ZG_SAMPLING_MODE_NEAREST: return "NEAREST";
	case ZG_SAMPLING_MODE_TRILINEAR: return "TRILINEAR";
	case ZG_SAMPLING_MODE_ANISOTROPIC: return "ANISOTROPIC";
	}
	sfz_assert(false);
	return "UNDEFINED";
}

static const char* wrappingModeToString(ZgWrappingMode mode) noexcept
{
	switch (mode) {
	case ZG_WRAPPING_MODE_CLAMP: return "CLAMP";
	case ZG_WRAPPING_MODE_REPEAT: return "REPEAT";
	}
	sfz_assert(false);
	return "UNDEFINED";
}

static const char* depthFuncToString(ZgDepthFunc func) noexcept
{
	switch (func) {
	case ZG_DEPTH_FUNC_LESS: return "LESS";
	case ZG_DEPTH_FUNC_LESS_EQUAL: return "LESS_EQUAL";
	case ZG_DEPTH_FUNC_EQUAL: return "EQUAL";
	case ZG_DEPTH_FUNC_NOT_EQUAL: return "NOT_EQUAL";
	case ZG_DEPTH_FUNC_GREATER: return "GREATER";
	case ZG_DEPTH_FUNC_GREATER_EQUAL: return "GREATER_EQUAL";
	}
	sfz_assert(false);
	return "";
}

static const char* blendModeToString(PipelineBlendMode mode) noexcept
{
	switch (mode) {
	case PipelineBlendMode::NO_BLENDING: return "no_blending";
	case PipelineBlendMode::ALPHA_BLENDING: return "alpha_blending";
	case PipelineBlendMode::ADDITIVE_BLENDING: return "additive_blending";
	}
	sfz_assert(false);
	return "";
}

// RendererUI: State methods
// ------------------------------------------------------------------------------------------------

void RendererUI::swap(RendererUI& other) noexcept
{
	(void)other;
}

void RendererUI::destroy() noexcept
{

}

// RendererUI: Methods
// ------------------------------------------------------------------------------------------------

void RendererUI::render(RendererState& state) noexcept
{
	ImGuiWindowFlags windowFlags = 0;
	windowFlags |= ImGuiWindowFlags_NoFocusOnAppearing;
	if (!ImGui::Begin("Renderer", nullptr, windowFlags)) {
		ImGui::End();
		return;
	}

	// Tabs
	ImGuiTabBarFlags tabBarFlags = ImGuiTabBarFlags_None;
	if (ImGui::BeginTabBar("RendererTabBar", tabBarFlags)) {
		
		if (ImGui::BeginTabItem("General")) {
			ImGui::Spacing();
			this->renderGeneralTab(state);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Present Queue")) {
			ImGui::Spacing();
			this->renderPresentQueueTab(state.configurable);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Framebuffers")) {
			ImGui::Spacing();
			this->renderFramebuffersTab(state.configurable);
			ImGui::EndTabItem();
		}
		
		if (ImGui::BeginTabItem("Pipelines")) {
			ImGui::Spacing();
			this->renderPipelinesTab(state);
			ImGui::EndTabItem();
		}
		
		if (ImGui::BeginTabItem("Textures")) {
			ImGui::Spacing();
			this->renderTexturesTab(state);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Meshes")) {
			ImGui::Spacing();
			this->renderMeshesTab(state);
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	ImGui::End();
}

// RendererUI: Private methods
// --------------------------------------------------------------------------------------------

void RendererUI::renderGeneralTab(RendererState& state) noexcept
{
	constexpr float offset = 250.0f;
	alignedEdit("Config path", offset, [&](const char*) {
		ImGui::Text("\"%s\"", state.configurable.configPath.str());
	});
	alignedEdit("Current frame index", offset, [&](const char*) {
		ImGui::Text("%llu", state.currentFrameIdx);
	});
	alignedEdit("Window resolution", offset, [&](const char*) {
		ImGui::Text("%i x %i", state.windowRes.x, state.windowRes.y);
	});

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();


	// Get ZeroG stats
	ZgStats stats = {};
	CHECK_ZG state.zgCtx.getStats(stats);

	// Print ZeroG statistics
	ImGui::Text("ZeroG Stats");
	ImGui::Spacing();
	ImGui::Indent(20.0f);

	constexpr float statsValueOffset = 240.0f;
	alignedEdit("Device", statsValueOffset, [&](const char*) {
		ImGui::TextUnformatted(stats.deviceDescription);
	});
	ImGui::Spacing();
	alignedEdit("Dedicated GPU Memory", statsValueOffset, [&](const char*) {
		ImGui::Text("%.2f GiB", toGiB(stats.dedicatedGpuMemoryBytes));
	});
	alignedEdit("Dedicated CPU Memory", statsValueOffset, [&](const char*) {
		ImGui::Text("%.2f GiB", toGiB(stats.dedicatedCpuMemoryBytes));
	});
	alignedEdit("Shared GPU Memory", statsValueOffset, [&](const char*) {
		ImGui::Text("%.2f GiB", toGiB(stats.sharedCpuMemoryBytes));
	});
	ImGui::Spacing();
	alignedEdit("Memory Budget", statsValueOffset, [&](const char*) {
		ImGui::Text("%.2f GiB", toGiB(stats.memoryBudgetBytes));
	});
	alignedEdit("Current Memory Usage", statsValueOffset, [&](const char*) {
		ImGui::Text("%.2f GiB", toGiB(stats.memoryUsageBytes));
	});
	ImGui::Spacing();
	alignedEdit("Non-Local Budget", statsValueOffset, [&](const char*) {
		ImGui::Text("%.2f GiB", toGiB(stats.nonLocalBugetBytes));
	});
	alignedEdit("Non-Local Usage", statsValueOffset, [&](const char*) {
		ImGui::Text("%.2f GiB", toGiB(stats.nonLocalUsageBytes));
	});

	ImGui::Unindent(20.0f);

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();


	// Print memory statistics
	ImGui::Text("Memory Stats");
	ImGui::Spacing();

	struct AllocatorNameBundle {
		DynamicGpuAllocator* allocator = nullptr;
		const char* name = nullptr;
	};

	const AllocatorNameBundle allocators[]{
		{ &state.gpuAllocatorUpload, "Upload" },
		{ &state.gpuAllocatorDevice, "Device" },
		{ &state.gpuAllocatorTexture, "Texture" },
		{ &state.gpuAllocatorFramebuffer, "Framebuffer" }
	};

	for (const AllocatorNameBundle& bundle : allocators) {
		
		DynamicGpuAllocator& alloc = *bundle.allocator;

		if (!ImGui::CollapsingHeader(str128("%s Memory", bundle.name))) continue;

		ImGui::Indent(30.0f);
		ImGui::Spacing();
		constexpr float infoOffset = 280.0f;
		alignedEdit("Total Num Allocations", infoOffset, [&](const char*) {
			ImGui::Text("%u", alloc.queryTotalNumAllocations());
		});
		alignedEdit("Total Num Deallocations", infoOffset, [&](const char*) {
			ImGui::Text("%u", alloc.queryTotalNumDeallocations());
		});
		alignedEdit("Default Page Size", infoOffset, [&](const char*) {
			ImGui::Text("%.2f MiB", toMiB(alloc.queryDefaultPageSize()));
		});
		uint32_t numDevicePages = alloc.queryNumPages();
		alignedEdit("Num Pages", infoOffset, [&](const char*) {
			ImGui::Text("%u", numDevicePages);
		});
		ImGui::Spacing();
		for (uint32_t i = 0; i < numDevicePages; i++) {
			constexpr float pageOffset = 260.0f;
			PageInfo info = alloc.queryPageInfo(i);
			ImGui::Text("Page %u:", i);
			ImGui::Indent(20.0f);
			alignedEdit("Size", pageOffset, [&](const char*) {
				ImGui::Text("%.2f MiB", toMiB(info.pageSizeBytes));
			});
			alignedEdit("Num Allocations", pageOffset, [&](const char*) {
				ImGui::Text("%u", info.numAllocations);
			});
			alignedEdit("Num Free Blocks", pageOffset, [&](const char*) {
				ImGui::Text("%u", info.numFreeBlocks);
			});
			alignedEdit("Largest Free Block", pageOffset, [&](const char*) {
				ImGui::Text("%.2f MiB", toMiB(info.largestFreeBlockBytes));
			});
			ImGui::Unindent(20.0f);
			ImGui::Spacing();
		}
		ImGui::Unindent(30.0f);
	}
}

void RendererUI::renderPresentQueueTab(RendererConfigurableState& state) noexcept
{
	// Get global collection of resource strings in order to get strings from StringIDs
	sfz::StringCollection& resStrings = sfz::getResourceStrings();

	for (uint32_t groupIdx = 0; groupIdx < state.presentQueue.size(); groupIdx++) {
		const StageGroup& group = state.presentQueue[groupIdx];
		const char* groupName = resStrings.getString(group.groupName);

		// Collapsing header with group name
		const bool collapsingHeaderOpen =
			ImGui::CollapsingHeader(str256("Stage Group %u - \"%s\"", groupIdx, groupName));
		if (!collapsingHeaderOpen) continue;

		ImGui::Indent(20.0f);
		for (uint32_t stageIdx = 0; stageIdx < group.stages.size(); stageIdx++) {
			const Stage& stage = group.stages[stageIdx];

			// Stage name
			ImGui::Text("Stage %u - \"%s\"", stageIdx, resStrings.getString(stage.stageName));
			ImGui::Indent(20.0f);

			// Stage type
			ImGui::Text("Type: %s", toString(stage.stageType));

			if (stage.stageType != StageType::USER_STAGE_BARRIER) {

				// Pipeline name
				ImGui::Text("Render Pipeline: \"%s\"", resStrings.getString(stage.renderPipelineName));

				// Framebuffer name
				ImGui::Text("Framebuffer: \"%s\"", resStrings.getString(stage.framebufferName));

				// Bound render targets
				ImGui::Text("Bound render targets:");
				ImGui::Indent(20.0f);
				for (const BoundRenderTarget& target : stage.boundRenderTargets) {
					if (target.depthBuffer) {
						ImGui::Text("- Register: %u  --  Framebuffer: \"%s\"  --  Depth Buffer",
							target.textureRegister,
							resStrings.getString(target.framebuffer));
					}
					else {
						ImGui::Text("- Register: %u  --  Framebuffer: \"%s\"  --  Render Target Index: %u",
							target.textureRegister,
							resStrings.getString(target.framebuffer),
							target.renderTargetIdx);
					}
				}
				ImGui::Unindent(20.0f);
			}

			ImGui::Unindent(20.0f);
			ImGui::Spacing();
		}
		ImGui::Unindent(20.0f);
	}
}

void RendererUI::renderFramebuffersTab(RendererConfigurableState& state) noexcept
{
	// Get global collection of resource strings in order to get strings from StringIDs
	sfz::StringCollection& resStrings = sfz::getResourceStrings();

	for (uint32_t i = 0; i < state.framebuffers.size(); i++) {
		const FramebufferItem& fbItem = state.framebuffers[i];

		// Framebuffer name
		ImGui::Text("Framebuffer %u - \"%s\"", i, resStrings.getString(fbItem.name));
		ImGui::Spacing();
		ImGui::Indent(20.0f);

		constexpr float offset = 220.0f;

		// Resolution type
		if (fbItem.resolutionIsFixed) {
			alignedEdit("Fixed resolution", offset, [&](const char*) {
				ImGui::Text("%i x %i", fbItem.resolutionFixed.x, fbItem.resolutionFixed.y);
			});
		}
		else {
			if (fbItem.resolutionScaleSetting != nullptr) {
				alignedEdit("Resolution scale", offset, [&](const char*) {
					ImGui::Text("%.2f  --  Setting: \"%s\"",
						fbItem.resolutionScale, fbItem.resolutionScaleSetting->key().str());
				});
			}
			else {
				alignedEdit("Resolution scale", offset, [&](const char*) {
					ImGui::Text("%.2f", fbItem.resolutionScale);
				});
			}
		}

		// Actual resolution
		uint32_t width = fbItem.framebuffer.framebuffer.width;
		uint32_t height = fbItem.framebuffer.framebuffer.height;
		alignedEdit("Current resolution", offset, [&](const char*) {
			ImGui::Text("%i x %i", width, height);
		});

		// Render targets
		for (uint32_t j = 0; j < fbItem.numRenderTargets; j++) {
			const RenderTargetItem& rtItem = fbItem.renderTargetItems[j];
			alignedEdit(str64("Render target [%u]", j), offset, [&](const char*) {
				ImGui::Text("%s  --  Clear: %.1f",
					textureFormatToString(rtItem.format), rtItem.clearValue);
			});
		}

		// Depth buffer
		if (fbItem.hasDepthBuffer) {
			alignedEdit("Depth buffer", offset, [&](const char*) {
				ImGui::Text("%s  --  Clear: %.1f",
					textureFormatToString(fbItem.depthBufferFormat), fbItem.depthBufferClearValue);
			});
		}

		ImGui::Unindent(20.0f);
		ImGui::Spacing();
		ImGui::Spacing();
	}
}

void RendererUI::renderPipelinesTab(RendererState& state) noexcept
{
	// Get global collection of resource strings in order to get strings from StringIDs
	sfz::StringCollection& resStrings = sfz::getResourceStrings();

	RendererConfigurableState& configurable = state.configurable;

	// Render pipelines
	ImGui::Text("Render Pipelines");

	// Reload all button
	ImGui::SameLine(ImGui::GetWindowWidth() - 130.0f);
	if (ImGui::Button("Reload All##__render_pipelines", vec2(120.0f, 0.0f))) {

		SFZ_INFO("Renderer", "Reloading all pipelines...");

		// Flush ZeroG queues
		CHECK_ZG state.presentQueue.flush();

		// Rebuild pipelines
		for (uint32_t i = 0; i < configurable.renderPipelines.size(); i++) {
			PipelineRenderItem& pipeline = configurable.renderPipelines[i];
			bool success = pipeline.buildPipeline();
			if (!success) {
				SFZ_WARNING("Renderer", "Failed to rebuild pipeline: \"%s\"",
					resStrings.getString(pipeline.name));
			}
		}
	}

	ImGui::Spacing();
	for (uint32_t i = 0; i < configurable.renderPipelines.size(); i++) {
		PipelineRenderItem& pipeline = configurable.renderPipelines[i];
		const ZgPipelineBindingsSignature& bindingsSignature = pipeline.pipeline.bindingsSignature;
		const ZgPipelineRenderSignature& renderSignature = pipeline.pipeline.renderSignature;
		const char* name = resStrings.getString(pipeline.name);

		// Reload button
		if (ImGui::Button(str64("Reload##__render_%u", i), vec2(80.0f, 0.0f))) {

			// Flush ZeroG queues
			CHECK_ZG state.presentQueue.flush();

			if (pipeline.buildPipeline()) {
				SFZ_INFO("Renderer", "Reloaded pipeline: \"%s\"", name);
			}
			else {
				SFZ_WARNING("Renderer", "Failed to rebuild pipeline: \"%s\"", name);
			}
		}
		ImGui::SameLine();

		// Collapsing header with name
		bool collapsingHeaderOpen =
			ImGui::CollapsingHeader(str256("Pipeline %u - \"%s\"", i, name));
		if (!collapsingHeaderOpen) continue;
		ImGui::Indent(20.0f);

		// Valid or not
		ImGui::Indent(20.0f);
		if (!pipeline.pipeline.valid()) {
			ImGui::SameLine();
			ImGui::TextUnformatted("-- INVALID PIPELINE");
		}

		// Pipeline info
		ImGui::Spacing();
		ImGui::Text("Vertex Shader: \"%s\" -- \"%s\"",
			pipeline.vertexShaderPath.str(), pipeline.vertexShaderEntry.str());
		ImGui::Text("Pixel Shader: \"%s\" -- \"%s\"",
			pipeline.pixelShaderPath.str(), pipeline.pixelShaderEntry.str());

		// Print vertex attributes
		ImGui::Spacing();
		ImGui::Text("Vertex attributes (%u):", renderSignature.numVertexAttributes);
		ImGui::Indent(20.0f);
		for (uint32_t j = 0; j < renderSignature.numVertexAttributes; j++) {
			const ZgVertexAttribute& attrib = renderSignature.vertexAttributes[j];
			ImGui::Text("- Location: %u -- Type: %s",
				attrib.location, vertexAttributeTypeToString(attrib.type));
		}
		ImGui::Unindent(20.0f);

		// Print constant buffers
		if (bindingsSignature.numConstBuffers > 0) {
			ImGui::Spacing();
			ImGui::Text("Constant buffers (%u):", bindingsSignature.numConstBuffers);
			ImGui::Indent(20.0f);
			for (uint32_t j = 0; j < bindingsSignature.numConstBuffers; j++) {
				const ZgConstantBufferBindingDesc& cbuffer = bindingsSignature.constBuffers[j];
				ImGui::Text("- Register: %u -- Size: %u bytes -- Push constant: %s",
					cbuffer.bufferRegister,
					cbuffer.sizeInBytes,
					cbuffer.pushConstant == ZG_TRUE ? "YES" : "NO");
			}
			ImGui::Unindent(20.0f);
		}

		// Print textures
		if (bindingsSignature.numTextures > 0) {
			ImGui::Spacing();
			ImGui::Text("Textures (%u):", bindingsSignature.numTextures);
			ImGui::Indent(20.0f);
			for (uint32_t j = 0; j < bindingsSignature.numTextures; j++) {
				const ZgTextureBindingDesc& texture = bindingsSignature.textures[j];
				ImGui::Text("- Register: %u", texture.textureRegister);
			}
			ImGui::Unindent(20.0f);
		}

		// Print samplers
		if (pipeline.numSamplers > 0) {
			ImGui::Spacing();
			ImGui::Text("Samplers (%u):", pipeline.numSamplers);
			ImGui::Indent(20.0f);
			for (uint32_t j = 0; j < pipeline.numSamplers; j++) {
				const SamplerItem& item = pipeline.samplers[j];
				ImGui::Text("- Register: %u -- Sampling: %s -- Wrapping: %s",
					item.samplerRegister,
					samplingModeToString(item.sampler.samplingMode),
					wrappingModeToString(item.sampler.wrappingModeU));

			}
			ImGui::Unindent(20.0f);
		}

		// Print render targets
		ImGui::Spacing();
		ImGui::Text("Render Targets (%u):", pipeline.numRenderTargets);
		ImGui::Indent(20.0f);
		for (uint32_t j = 0; j < pipeline.numRenderTargets; j++) {
			ImGui::Text("- Render Target: %u -- %s",
				j, textureFormatToString(pipeline.renderTargets[j]));
		}
		ImGui::Unindent(20.0f);

		// Print depth test
		ImGui::Spacing();
		ImGui::Text("Depth Test: %s", pipeline.depthTest ? "ENABLED" : "DISABLED");
		if (pipeline.depthTest) {
			ImGui::Indent(20.0f);
			ImGui::Text("Depth function: %s", depthFuncToString(pipeline.depthFunc));
			ImGui::Unindent(20.0f);
		}

		// Print culling info
		ImGui::Spacing();
		ImGui::Text("Culling: %s", pipeline.cullingEnabled ? "ENABLED" : "DISABLED");
		if (pipeline.cullingEnabled) {
			ImGui::Indent(20.0f);
			ImGui::Text("Cull Front Face: %s", pipeline.cullFrontFacing ? "YES" : "NO");
			ImGui::Text("Front Facing Is Counter Clockwise: %s", pipeline.frontFacingIsCounterClockwise ? "YES" : "NO");
			ImGui::Unindent(20.0f);
		}

		// Print depth bias info
		ImGui::Spacing();
		ImGui::Text("Depth Bias");
		ImGui::Indent(20.0f);
		constexpr float xOffset = 300.0f;
		alignedEdit("Bias", xOffset, [&](const char* name) {
			ImGui::SetNextItemWidth(165.0f);
			ImGui::InputInt(str128("%s##render_%u", name, i), &pipeline.depthBias);
		});
		alignedEdit("Bias Slope Scaled", xOffset, [&](const char* name) {
			ImGui::SetNextItemWidth(100.0f);
			ImGui::InputFloat(str128("%s##render_%u", name, i), &pipeline.depthBiasSlopeScaled, 0.0f, 0.0f, "%.4f");
		});
		alignedEdit("Bias Clamp", xOffset, [&](const char* name) {
			ImGui::SetNextItemWidth(100.0f);
			ImGui::InputFloat(str128("%s##render_%u", name, i), &pipeline.depthBiasClamp, 0.0f, 0.0f, "%.4f");
		});
		ImGui::Unindent(20.0f);

		// Print wireframe rendering mode
		ImGui::Spacing();
		ImGui::Text("Wireframe Rendering: %s",
			pipeline.wireframeRenderingEnabled ? "ENABLED" : "DISABLED");

		// Print blend mode
		ImGui::Spacing();
		ImGui::Text("Blend Mode: %s", blendModeToString(pipeline.blendMode));

		ImGui::Unindent(20.0f);
		ImGui::Unindent(20.0f);
		ImGui::Spacing();
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();
	ImGui::Text("Compute Pipelines");
}

void RendererUI::renderTexturesTab(RendererState& state) noexcept
{
	// Get global collection of resource strings in order to get strings from StringIDs
	sfz::StringCollection& resStrings = sfz::getResourceStrings();

	constexpr float offset = 150.0f;

	for (auto itemItr : state.textures) {
		const TextureItem& item = itemItr.value;

		ImGui::Text("\"%s\"", resStrings.getString(itemItr.key));
		if (!item.texture.valid()) {
			ImGui::SameLine();
			ImGui::Text("-- NOT VALID");
		}

		ImGui::Indent(20.0f);
		alignedEdit("Format", offset, [&](const char*) {
			ImGui::Text("%s", textureFormatToString(item.format));
		});
		alignedEdit("Resolution", offset, [&](const char*) {
			ImGui::Text("%u x %u", item.width, item.height);
		});
		alignedEdit("Mipmaps", offset, [&](const char*) {
			ImGui::Text("%u", item.numMipmaps);
		});

		ImGui::Unindent(20.0f);
		ImGui::Spacing();
	}
}

void RendererUI::renderMeshesTab(RendererState& state) noexcept
{
	// Get global collection of resource strings in order to get strings from StringIDs
	sfz::StringCollection& resStrings = sfz::getResourceStrings();

	for (auto itemItr : state.meshes) {
		GpuMesh& mesh = itemItr.value;

		// Check if mesh is valid
		bool meshValid = true;
		if (!mesh.vertexBuffer.valid()) meshValid = false;
		if (!mesh.indexBuffer.valid()) meshValid = false;
		if (!mesh.materialsBuffer.valid()) meshValid = false;

		// Mesh name
		ImGui::Text("\"%s\"", resStrings.getString(itemItr.key));
		if (!meshValid) {
			ImGui::SameLine();
			ImGui::Text("-- NOT VALID");
		}

		// Components
		ImGui::Indent(20.0f);
		if (ImGui::CollapsingHeader(str64("Components (%u):##%llu", mesh.components.size(), itemItr.key))) {

			ImGui::Indent(20.0f);
			for (uint32_t i = 0; i < mesh.components.size(); i++) {

				const MeshComponent& comp = mesh.components[i];
				constexpr float offset = 250.0f;
				ImGui::Text("Component %u -- Material Index: %u -- NumIndices: %u",
					i, comp.materialIdx, comp.numIndices);
			}
			ImGui::Unindent(20.0f);
		}
		ImGui::Unindent(20.0f);

		// Lambdas for converting vec4_u8 to vec4_f32 and back
		auto u8ToF32 = [](vec4_u8 v) { return vec4(v) * (1.0f / 255.0f); };
		auto f32ToU8 = [](vec4 v) { return vec4_u8(v * 255.0f); };

		// Lambda for converting texture index to combo string label
		auto textureToComboStr = [&](StringID strId) {
			str128 texStr;
			if (strId == StringID::invalid()) texStr.appendf("NO TEXTURE");
			else texStr= str128("%s", resStrings.getString(strId));
			return texStr;
		};

		// Lambda for creating a combo box to select texture
		auto textureComboBox = [&](const char* comboName, StringID& texId, bool& updateMesh) {
			str128 selectedTexStr = textureToComboStr(texId);
			if (ImGui::BeginCombo(comboName, selectedTexStr)) {

				// Special case for no texture (~0)
				{
					bool isSelected = texId == StringID::invalid();
					if (ImGui::Selectable("NO TEXTURE", isSelected)) {
						texId = StringID::invalid();
						updateMesh = true;
					}
				}

				// Existing textures
				for (auto itemItr : state.textures) {
					StringID id = itemItr.key;

					// Convert index to string and check if it is selected
					str128 texStr = textureToComboStr(id);
					bool isSelected = id == texId;

					// Report index to ImGui combo button and update current if it has changed
					if (ImGui::Selectable(texStr, isSelected)) {
						texId = id;
						updateMesh = true;
					}
				}
				ImGui::EndCombo();
			}
		};

		// Materials
		ImGui::Indent(20.0f);
		if (ImGui::CollapsingHeader(str64("Materials (%u):##%llu", mesh.cpuMaterials.size(), itemItr.key))) {

			ImGui::Indent(20.0f);
			for (uint32_t i = 0; i < mesh.cpuMaterials.size(); i++) {
				Material& material = mesh.cpuMaterials[i];

				// Edit CPU material
				bool updateMesh = false;
				if (ImGui::CollapsingHeader(str64("Material %u##%llu", i, itemItr.key))) {

					ImGui::Indent(20.0f);
					constexpr float offset = 310.0f;

					// Albedo
					vec4 colorFloat = u8ToF32(material.albedo);
					alignedEdit("Albedo Factor", offset, [&](const char* name) {
						if (ImGui::ColorEdit4(str128("%s##%u_%llu", name, i, itemItr.key), colorFloat.data(),
							ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_Float)) {
							material.albedo = f32ToU8(colorFloat);
							updateMesh = true;
						}
					});
					alignedEdit("Albedo Texture", offset, [&](const char* name) {
						textureComboBox(str128("##%s_%u_%llu", name, i, itemItr.key), material.albedoTex, updateMesh);
					});

					// Emissive
					alignedEdit("Emissive Factor", offset, [&](const char* name) {
						if (ImGui::ColorEdit3(str128("%s##%u_%llu", name, i, itemItr.key), material.emissive.data(),
							ImGuiColorEditFlags_Float)) {
							updateMesh = true;
						}
					});
					alignedEdit("Emissive Texture", offset, [&](const char* name) {
						textureComboBox(str128("##%s_%u_%llu", name, i, itemItr.key), material.emissiveTex, updateMesh);
					});

					// Metallic & roughness
					vec4_u8 metallicRoughnessU8(material.metallic, material.roughness, 0, 0);
					vec4 metallicRoughness = u8ToF32(metallicRoughnessU8);
					alignedEdit("Metallic Roughness Factors", offset, [&](const char* name) {
						if (ImGui::SliderFloat2(str128("%s##%u_%llu", name, i, itemItr.key), metallicRoughness.data(), 0.0f, 1.0f)) {
							metallicRoughnessU8 = f32ToU8(metallicRoughness);
							material.metallic = metallicRoughnessU8.x;
							material.roughness = metallicRoughnessU8.y;
							updateMesh = true;
						}
					});
					alignedEdit("Metallic Roughness Texture", offset, [&](const char* name) {
						textureComboBox(str64("##%s_%u_%llu", name, i, itemItr.key), material.metallicRoughnessTex, updateMesh);
					});

					// Normal and Occlusion textures
					alignedEdit("Normal Texture", offset, [&](const char* name) {
						textureComboBox(str64("##%s_%u_%llu", name, i, itemItr.key), material.normalTex, updateMesh);
					});
					alignedEdit("Occlusion Texture", offset, [&](const char* name) {
						textureComboBox(str64("##%s_%u_%llu", name, i, itemItr.key), material.occlusionTex, updateMesh);
					});

					ImGui::Unindent(20.0f);
				}

				// If material was edited, update mesh
				if (updateMesh) {

					// Flush ZeroG queues
					CHECK_ZG state.copyQueue.flush();
					CHECK_ZG state.presentQueue.flush();

					// Allocate temporary upload buffer
					zg::Buffer uploadBuffer =
						state.gpuAllocatorUpload.allocateBuffer(sizeof(ShaderMaterial));
					sfz_assert(uploadBuffer.valid());

					// Convert new material to shader material
					ShaderMaterial shaderMaterial = cpuMaterialToShaderMaterial(material);

					// Memcpy to temporary upload buffer
					CHECK_ZG uploadBuffer.memcpyTo(0, &shaderMaterial, sizeof(ShaderMaterial));

					// Replace material in mesh with new material
					zg::CommandList commandList;
					CHECK_ZG state.presentQueue.beginCommandListRecording(commandList);
					uint64_t dstOffset = sizeof(ShaderMaterial) * i;
					CHECK_ZG commandList.memcpyBufferToBuffer(
						mesh.materialsBuffer, dstOffset, uploadBuffer, 0, sizeof(ShaderMaterial));
					CHECK_ZG state.presentQueue.executeCommandList(commandList);
					CHECK_ZG state.presentQueue.flush();

					// Deallocate temporary upload buffer
					state.gpuAllocatorUpload.deallocate(uploadBuffer);
				}
			}
			ImGui::Unindent(20.0f);
		}
		ImGui::Unindent(20.0f);

		ImGui::Spacing();
	}
}

} // namespace sfz
