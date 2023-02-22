// Copyright (c) Peter Hillerström 2022-2023 (skipifzero.com, peter@hstroem.se)
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

#include "ImGuiGpuNativeExtD3D12.h"

#include <imgui.h>

#include <sfz_math.h>

#include <gpu_lib_internal_d3d12.hpp>

// Helper structs
// ------------------------------------------------------------------------------------------------

// TODO: Remove
struct ImGuiVertex final {
	f32x2 pos;
	f32x2 texcoord;
	u32 color;
};
static_assert(sizeof(ImGuiVertex) == 20, "ImGuiVertex is padded");

// TODO: Remove
struct ImGuiCommand {
	u32 idx_buffer_offset = 0;
	u32 num_indices = 0;
	GpuTexIdx tex_idx = GPU_NULL_TEX;
	u16 is_font_tex = 1;
	u32 padding1;
	f32x4 clipRect = f32x4_splat(0.0f);
};
static_assert(sizeof(ImGuiCommand) == sizeof(u32) * 8, "ImguiCommand is padded");

// ImGui D3D12 State
// ------------------------------------------------------------------------------------------------

constexpr u32 IMGUI_MAX_NUM_VERTICES = 65536;
constexpr u32 IMGUI_MAX_NUM_INDICES = 65536;
constexpr u64 IMGUI_VERTEX_BUFFER_SIZE = IMGUI_MAX_NUM_VERTICES * sizeof(ImGuiVertex);
constexpr u64 IMGUI_INDEX_BUFFER_SIZE = IMGUI_MAX_NUM_INDICES * sizeof(u32);

struct ImguiD3D12State {
	// ImGui shader
	ComPtr<ID3D12PipelineState> imgui_pso;
	ComPtr<ID3D12RootSignature> imgui_root_sig;

	// Index buffer
	ComPtr<ID3D12Resource> index_buffer;
	D3D12_RESOURCE_STATES index_buffer_state;

	// Other buffers
	GpuPtr vertex_buffer;
	GpuPtr transforms_buffer;
	GpuPtr projection_matrix_buffer;

	// Tmp data
	SfzArray<ImGuiVertex> tmp_vertices;
	SfzArray<u32> tmp_indices;
	SfzArray<ImGuiCommand> tmp_cmds;
};

// ImGui D3D12 Functions
// ------------------------------------------------------------------------------------------------

static void imguiD3D12Run(GpuLib* gpu, void* ext_data_ptr, void* params_in, u32 params_size)
{
	sfz_assert(ext_data_ptr != nullptr);
	ImguiD3D12State& state = *static_cast<ImguiD3D12State*>(ext_data_ptr);
	sfz_assert(params_in != nullptr);
	sfz_assert(params_size == sizeof(ImGuiNativeExtD3D12Params));
	ImGuiNativeExtD3D12Params& params = *static_cast<ImGuiNativeExtD3D12Params*>(params_in);

	// Render ImGui and grab draw data
	ImGui::Render();
	ImDrawData& imgui_draw_data = *ImGui::GetDrawData();

	// Clear old converted draw data
	state.tmp_vertices.clear();
	state.tmp_indices.clear();
	state.tmp_cmds.clear();

	// Convert draw data
	for (int i = 0; i < imgui_draw_data.CmdListsCount; i++) {

		const ImDrawList& draw_list = *imgui_draw_data.CmdLists[i];

		// index_offset is the offset to offset all indices with
		const u32 index_offset = state.tmp_vertices.size();

		// index_buffer_offset is the offset to where the indices start
		u32 index_buffer_offset = state.tmp_indices.size();

		// Convert vertices and add to global list
		for (int j = 0; j < draw_list.VtxBuffer.size(); j++) {
			const ImDrawVert& imgui_vertex = draw_list.VtxBuffer[j];

			ImGuiVertex converted_vertex;
			converted_vertex.pos = f32x2_init(imgui_vertex.pos.x, imgui_vertex.pos.y);
			converted_vertex.texcoord = f32x2_init(imgui_vertex.uv.x, imgui_vertex.uv.y);
			converted_vertex.color = imgui_vertex.col;

			state.tmp_vertices.add(converted_vertex);
		}

		// Fix indices and add to global list
		for (int j = 0; j < draw_list.IdxBuffer.size(); j++) {
			state.tmp_indices.add(draw_list.IdxBuffer[j] + index_offset);
		}

		// Create new commands
		for (int j = 0; j < draw_list.CmdBuffer.Size; j++) {
			const ImDrawCmd& im_cmd = draw_list.CmdBuffer[j];

			ImGuiCommand cmd;
			cmd.idx_buffer_offset = index_buffer_offset;
			cmd.num_indices = im_cmd.ElemCount;
			index_buffer_offset += im_cmd.ElemCount;
			cmd.tex_idx = params.font_tex_idx;
			if (im_cmd.TextureId != nullptr) {
				cmd.tex_idx = GpuTexIdx(u64(im_cmd.TextureId));
				cmd.is_font_tex = 0;
			}
			cmd.clipRect.x = im_cmd.ClipRect.x;
			cmd.clipRect.y = im_cmd.ClipRect.y;
			cmd.clipRect.z = im_cmd.ClipRect.z;
			cmd.clipRect.w = im_cmd.ClipRect.w;

			state.tmp_cmds.add(cmd);
		}
	}
	sfz_assert_hard(state.tmp_vertices.size() < IMGUI_MAX_NUM_VERTICES);
	sfz_assert_hard(state.tmp_indices.size() < IMGUI_MAX_NUM_INDICES);
	if (state.tmp_vertices.size() == 0 || state.tmp_cmds.size() == 0) return;

	// Retrieve imgui scale factor
	const i32x2 swapchain_res = gpu->swapchain_res;
	f32 imguiScaleFactor = 1.0f;
	imguiScaleFactor /= params.scale;
	const f32 imguiInvScaleFactor = 1.0f / imguiScaleFactor;
	const f32x2 imgui_res = f32x2_from_i32(swapchain_res) * imguiScaleFactor;

	// Calculate ImGui projection matrix
	const SfzMat44 proj_matrix = {
		f32x4_init(2.0f / imgui_res.x, 0.0f, 0.0f, -1.0f),
		f32x4_init(0.0f, 2.0f / -imgui_res.y, 0.0f, 1.0f),
		f32x4_init(0.0f, 0.0f, 0.5f, 0.5f),
		f32x4_init(0.0f, 0.0f, 0.0f, 1.0f)
	};

	// Upload data to GPU
	const u32 index_buffer_size_bytes = state.tmp_indices.size() * sizeof(u32);
	{
		// Try to allocate a range
		const GpuHeapRangeAlloc range_alloc = gpuAllocUploadHeapRange(gpu, index_buffer_size_bytes);
		if (!range_alloc.success) {
			GPU_LOG_ERROR("[gpu_lib]: Upload heap overflow by %u bytes.",
				u32(range_alloc.end - gpu->upload_heap_safe_offset));
			return;
		}

		// Memcpy data to upload heap and commit change
		memcpy(gpu->upload_heap_mapped_ptr + range_alloc.begin_mapped, state.tmp_indices.data(), index_buffer_size_bytes);
		gpu->upload_heap_offset = range_alloc.end;

		// Ensure index buffer is in COPY_DEST state
		if (state.index_buffer_state != D3D12_RESOURCE_STATE_COPY_DEST) {
			D3D12_RESOURCE_BARRIER barrier = {};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = state.index_buffer.Get();
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			barrier.Transition.StateBefore = state.index_buffer_state;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
			gpu->cmd_list->ResourceBarrier(1, &barrier);
			state.index_buffer_state = D3D12_RESOURCE_STATE_COPY_DEST;
		}

		// Copy to index buffer
		gpu->cmd_list->CopyBufferRegion(
			state.index_buffer.Get(), 0, gpu->upload_heap.Get(), range_alloc.begin_mapped, index_buffer_size_bytes);
	}
	gpuQueueMemcpyUpload(gpu, state.vertex_buffer, state.tmp_vertices.data(), state.tmp_vertices.size() * sizeof(ImGuiVertex));
	gpuQueueMemcpyUpload(gpu, state.projection_matrix_buffer, &proj_matrix, sizeof(SfzMat44));

	// Set index buffer
	{
		if (state.index_buffer_state != D3D12_RESOURCE_STATE_INDEX_BUFFER) {
			D3D12_RESOURCE_BARRIER barrier = {};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = state.index_buffer.Get();
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			barrier.Transition.StateBefore = state.index_buffer_state;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_INDEX_BUFFER;
			gpu->cmd_list->ResourceBarrier(1, &barrier);
			state.index_buffer_state = D3D12_RESOURCE_STATE_INDEX_BUFFER;
		}

		D3D12_INDEX_BUFFER_VIEW index_buffer_view = {};
		index_buffer_view.BufferLocation = state.index_buffer->GetGPUVirtualAddress();
		index_buffer_view.SizeInBytes = index_buffer_size_bytes;
		index_buffer_view.Format = DXGI_FORMAT_R32_UINT;
		gpu->cmd_list->IASetIndexBuffer(&index_buffer_view);
	}

	// Ensure heap is in ALL_SHADER_RESOURCE state
	if (gpu->gpu_heap_state != D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE) {
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = gpu->gpu_heap.Get();
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = gpu->gpu_heap_state;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
		gpu->cmd_list->ResourceBarrier(1, &barrier);
		gpu->gpu_heap_state = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
	}

	// Set render targets
	GpuSwapchainBackbuffer& bbuf = gpu->getCurrSwapchainBackbuffer();
	gpu->cmd_list->OMSetRenderTargets(1, &bbuf.rtv_descriptor, FALSE, nullptr);

	// Set viewport
	D3D12_VIEWPORT viewport = {};
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = f32(gpu->swapchain_res.x);
	viewport.Height = f32(gpu->swapchain_res.y);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	gpu->cmd_list->RSSetViewports(1, &viewport);

	// Set shader
	gpu->cmd_list->SetPipelineState(state.imgui_pso.Get());
	gpu->cmd_list->SetGraphicsRootSignature(state.imgui_root_sig.Get());
	gpu->cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set common root signature stuff
	gpu->cmd_list->SetGraphicsRootShaderResourceView(
		GPU_ROOT_PARAM_GLOBAL_HEAP_IDX, gpu->gpu_heap->GetGPUVirtualAddress());
	gpu->cmd_list->SetGraphicsRootDescriptorTable(
		GPU_ROOT_PARAM_TEX_HEAP_IDX, gpu->tex_descriptor_heap_start_gpu);

	for (u32 cmd_idx = 0; cmd_idx < state.tmp_cmds.size(); cmd_idx++) {
		const ImGuiCommand& cmd = state.tmp_cmds[cmd_idx];
		sfz_assert((cmd.num_indices % 3) == 0);

		D3D12_RECT scissor_rect = {};
		scissor_rect.left = u32(cmd.clipRect.x * imguiInvScaleFactor);
		scissor_rect.top = u32(cmd.clipRect.y * imguiInvScaleFactor);
		scissor_rect.right = scissor_rect.left + u32((cmd.clipRect.z - cmd.clipRect.x) * imguiInvScaleFactor);
		scissor_rect.bottom = scissor_rect.top + u32((cmd.clipRect.w - cmd.clipRect.y) * imguiInvScaleFactor);
		//scissor_rect.width = u32((cmd.clipRect.z - cmd.clipRect.x) * imguiInvScaleFactor);
		//scissor_rect.height = u32((cmd.clipRect.w - cmd.clipRect.y) * imguiInvScaleFactor);
		gpu->cmd_list->RSSetScissorRects(1, &scissor_rect);

		// Set launch params
		struct ImGuiLaunchParams {
			GpuPtr vertex_buffer;
			GpuPtr proj_matrix_buffer;
			GpuTexIdx tex_idx;
			u16 is_font_tex;
			u32 padding0;
		};
		sfz_static_assert(sizeof(ImGuiLaunchParams) == sizeof(u32) * 4);

		const ImGuiLaunchParams launch_params = ImGuiLaunchParams{
			.vertex_buffer = state.vertex_buffer,
			.proj_matrix_buffer = state.projection_matrix_buffer,
			.tex_idx = cmd.tex_idx,
			.is_font_tex = cmd.is_font_tex
		};
		gpu->cmd_list->SetGraphicsRoot32BitConstants(
			GPU_ROOT_PARAM_LAUNCH_PARAMS_IDX, sizeof(launch_params) / 4, &launch_params, 0);

		// Draw
		gpu->cmd_list->DrawIndexedInstanced(cmd.num_indices, 1, cmd.idx_buffer_offset, 0, 0);
	}

	// Restore scissor to default just in case
	D3D12_RECT scissor_rect = {};
	scissor_rect.left = 0;
	scissor_rect.top = 0;
	scissor_rect.right = LONG_MAX;
	scissor_rect.bottom = LONG_MAX;
	gpu->cmd_list->RSSetScissorRects(1, &scissor_rect);
}

static void imguiD3D12Destroy(GpuLib* gpu, void* ext_data_ptr)
{
	sfz_assert(ext_data_ptr != nullptr);
	ImguiD3D12State* state = static_cast<ImguiD3D12State*>(ext_data_ptr);

	gpuFree(gpu, state->vertex_buffer);
	gpuFree(gpu, state->transforms_buffer);

	sfz_delete(gpu->cfg.cpu_allocator, state);
}

// ImGui D3D12 shaders
// ------------------------------------------------------------------------------------------------

constexpr const char IMGUI_SHADER_SRC[] = R"(

struct ImGuiVertex {
	float2 position;
	float2 texcoord;
	uint color_r : 8;
	uint color_g : 8;
	uint color_b : 8;
	uint color_a : 8;
};
static_assert(sizeof(ImGuiVertex) == 20);

struct ImGuiLaunchParams {
	GpuPtr vertex_buffer;
	GpuPtr proj_matrix_buffer;
	GpuTexIdx tex_idx;
	uint16_t is_font_tex;
	uint padding0;
};
static_assert(sizeof(ImGuiLaunchParams) == sizeof(uint) * 4);
GPU_DECLARE_LAUNCH_PARAMS(ImGuiLaunchParams, params);

struct VSInput {
	uint vertex_idx : SV_VertexID;
};

struct VSOutput {
	float2 texcoord : PARAM_0;
	float4 color : PARAM_1;
	float4 position : SV_Position;
};

struct PSInput {
	float2 texcoord : PARAM_0;
	float4 color : PARAM_1;
};

VSOutput VSMain(VSInput input)
{
	const row_major float4x4 proj_matrix =
		ptrLoad<row_major float4x4>(params.proj_matrix_buffer);
	const ImGuiVertex v = ptrLoadArrayElem<ImGuiVertex>(params.vertex_buffer, input.vertex_idx);

	VSOutput output;
	output.texcoord = v.texcoord;
	output.color.r = float(v.color_r) * (1.0f / 255.0f);
	output.color.g = float(v.color_g) * (1.0f / 255.0f);
	output.color.b = float(v.color_b) * (1.0f / 255.0f);
	output.color.a = float(v.color_a) * (1.0f / 255.0f);
	output.position = mul(proj_matrix, float4(v.position, 0.0f, 1.0f));
	return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
	Texture2D tex = getTex(params.tex_idx);
	SamplerState tex_sampler = getSampler(GPU_LINEAR, GPU_CLAMP, GPU_CLAMP);
	float4 res = float4(0.0f, 0.0f, 0.0f, 1.0f);
	if (params.is_font_tex) {
		// Note: The clamped texcoord below is to fix an Intel driver bug.
		const float font_alpha = tex.Sample(tex_sampler, clamp(input.texcoord, 0.0f, 1.0f)).r;
		//const float font_alpha = tex.Sample(tex_sampler, input.texcoord).r;
		res = float4(input.color.rgb, input.color.a * font_alpha);
	}
	else {
		const float3 val = tex.Sample(tex_sampler, input.texcoord).rgb;
		res.rgb = val;
	}
	return res;
}
)";
constexpr u32 IMGUI_SHADER_SRC_SIZE = sizeof(IMGUI_SHADER_SRC);

// ImGui D3D12 Native Extension
// ------------------------------------------------------------------------------------------------

sfz_extern_c GpuNativeExt imguiGpuNativeExtD3D12Init(GpuLib* gpu)
{
	// Compile ImGui shaders
	ComPtr<ID3D12PipelineState> imgui_pso;
	ComPtr<ID3D12RootSignature> imgui_root_sig;
	{
		// Append prolog to shader source (can probably be done at compile time, but dunno how and
		// not worth the effort to figure out).
		char* src = (char*)gpu->cfg.cpu_allocator->alloc(
			sfz_dbg(""), GPU_KERNEL_PROLOG_SIZE + IMGUI_SHADER_SRC_SIZE);
		sfz_defer[=]() {
			gpu->cfg.cpu_allocator->dealloc(src);
		};
		memcpy(src, GPU_KERNEL_PROLOG, GPU_KERNEL_PROLOG_SIZE);
		memcpy(src + GPU_KERNEL_PROLOG_SIZE, IMGUI_SHADER_SRC, IMGUI_SHADER_SRC_SIZE);
		const u32 src_size = GPU_KERNEL_PROLOG_SIZE + IMGUI_SHADER_SRC_SIZE;

		// Compile shaders
		ComPtr<IDxcBlob> vs_dxil_blob;
		ComPtr<IDxcBlob> ps_dxil_blob;
		u32 launch_params_size = 0;
		{
			// Create source blobs
			ComPtr<IDxcBlobEncoding> source_blob;
			bool succ = CHECK_D3D12(gpu->dxc_utils->CreateBlob(src, src_size, CP_UTF8, &source_blob));
			sfz_assert(succ);
			DxcBuffer src_buffer = {};
			src_buffer.Ptr = source_blob->GetBufferPointer();
			src_buffer.Size = source_blob->GetBufferSize();
			src_buffer.Encoding = 0;

			// Compiler arguments
			constexpr u32 NUM_ARGS = 11;
			LPCWSTR VS_ARGS[NUM_ARGS] = {
				L"-E",
				L"VSMain",
				L"-T",
				L"vs_6_6",
				L"-HV 2021",
				L"-enable-16bit-types",
				L"-O3",
				L"-Zi",
				L"-Qembed_debug",
				DXC_ARG_PACK_MATRIX_ROW_MAJOR,
				L"-DGPU_READ_ONLY_HEAP"
			};
			LPCWSTR PS_ARGS[NUM_ARGS] = {
				L"-E",
				L"PSMain",
				L"-T",
				L"ps_6_6",
				L"-HV 2021",
				L"-enable-16bit-types",
				L"-O3",
				L"-Zi",
				L"-Qembed_debug",
				DXC_ARG_PACK_MATRIX_ROW_MAJOR,
				L"-DGPU_READ_ONLY_HEAP"
			};

			// Compile shaders
			ComPtr<IDxcResult> vs_compile_res;
			CHECK_D3D12(gpu->dxc_compiler->Compile(
				&src_buffer, VS_ARGS, NUM_ARGS, gpu->dxc_include_handler.Get(), IID_PPV_ARGS(&vs_compile_res)));
			{
				ComPtr<IDxcBlobUtf8> error_msgs;
				CHECK_D3D12(vs_compile_res->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&error_msgs), nullptr));
				if (error_msgs && error_msgs->GetStringLength() > 0) {
					GPU_LOG_ERROR("[gpu_lib]: %s\n", (const char*)error_msgs->GetBufferPointer());
				}

				HRESULT hr = {};
				CHECK_D3D12(vs_compile_res->GetStatus(&hr));
				const bool compile_success = CHECK_D3D12(hr);
				sfz_assert_hard(compile_success);
			}
			ComPtr<IDxcResult> ps_compile_res;
			CHECK_D3D12(gpu->dxc_compiler->Compile(
				&src_buffer, PS_ARGS, NUM_ARGS, gpu->dxc_include_handler.Get(), IID_PPV_ARGS(&ps_compile_res)));
			{
				ComPtr<IDxcBlobUtf8> error_msgs;
				CHECK_D3D12(ps_compile_res->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&error_msgs), nullptr));
				if (error_msgs && error_msgs->GetStringLength() > 0) {
					GPU_LOG_ERROR("[gpu_lib]: %s\n", (const char*)error_msgs->GetBufferPointer());
				}

				HRESULT hr = {};
				CHECK_D3D12(ps_compile_res->GetStatus(&hr));
				const bool compile_success = CHECK_D3D12(hr);
				sfz_assert_hard(compile_success);
			}

			// Get compiled DXIL
			CHECK_D3D12(vs_compile_res->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&vs_dxil_blob), nullptr));
			CHECK_D3D12(ps_compile_res->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&ps_dxil_blob), nullptr));

			// Get reflection data
			ComPtr<IDxcBlob> vs_reflection_data;
			CHECK_D3D12(vs_compile_res->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(&vs_reflection_data), nullptr));
			DxcBuffer vs_reflection_buffer = {};
			vs_reflection_buffer.Ptr = vs_reflection_data->GetBufferPointer();
			vs_reflection_buffer.Size = vs_reflection_data->GetBufferSize();
			vs_reflection_buffer.Encoding = 0;
			ComPtr<ID3D12ShaderReflection> vs_reflection;
			CHECK_D3D12(gpu->dxc_utils->CreateReflection(&vs_reflection_buffer, IID_PPV_ARGS(&vs_reflection)));

			// Get launch parameters info from reflection
			D3D12_SHADER_DESC vs_shader_desc = {};
			CHECK_D3D12(vs_reflection->GetDesc(&vs_shader_desc));
			sfz_assert(vs_shader_desc.ConstantBuffers == 1);

			ID3D12ShaderReflectionConstantBuffer* cbuffer_reflection = vs_reflection->GetConstantBufferByIndex(0);
			D3D12_SHADER_BUFFER_DESC cbuffer = {};
			CHECK_D3D12(cbuffer_reflection->GetDesc(&cbuffer));
			launch_params_size = cbuffer.Size;
		}

		// Create root signature
		imgui_root_sig = gpuCreateDefaultRootSignature(gpu, false, launch_params_size, "imgui_root_sig", true);
		sfz_assert_hard(imgui_root_sig != nullptr);

		// Create PSO (Pipeline State Object)
		{
			D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {};
			pso_desc.pRootSignature = imgui_root_sig.Get();
			pso_desc.VS.pShaderBytecode = vs_dxil_blob->GetBufferPointer();
			pso_desc.VS.BytecodeLength = vs_dxil_blob->GetBufferSize();
			pso_desc.PS.pShaderBytecode = ps_dxil_blob->GetBufferPointer();
			pso_desc.PS.BytecodeLength = ps_dxil_blob->GetBufferSize();

			pso_desc.BlendState.AlphaToCoverageEnable = FALSE;
			pso_desc.BlendState.IndependentBlendEnable = FALSE;
			pso_desc.BlendState.RenderTarget[0].BlendEnable = TRUE;
			pso_desc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
			pso_desc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			pso_desc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
			pso_desc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			pso_desc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
			pso_desc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
			pso_desc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
			pso_desc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
			pso_desc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			pso_desc.SampleMask = U32_MAX;

			pso_desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
			// Note: ImGui seems a bit inconsistent with front/back facing triangles, disabling culling gives best result.
			pso_desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
			pso_desc.RasterizerState.FrontCounterClockwise = false;
			pso_desc.RasterizerState.DepthClipEnable = true;

			pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

			pso_desc.NumRenderTargets = 1;
			pso_desc.RTVFormats[0] = GPU_SWAPCHAIN_DXGI_FORMAT;
			pso_desc.DSVFormat = DXGI_FORMAT_UNKNOWN;

			pso_desc.SampleDesc = { 1, 0 };

			const bool pso_success = CHECK_D3D12(gpu->device->CreateGraphicsPipelineState(
				&pso_desc, IID_PPV_ARGS(&imgui_pso)));
			sfz_assert_hard(pso_success);
			setDebugName(imgui_pso.Get(), "imgui_pso");
		}
	}

	// Allocate index buffer
	ComPtr<ID3D12Resource> index_buffer;
	{
		D3D12_HEAP_PROPERTIES heap_props = {};
		heap_props.Type = D3D12_HEAP_TYPE_DEFAULT;
		heap_props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heap_props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heap_props.CreationNodeMask = 0;
		heap_props.VisibleNodeMask = 0;

		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Alignment = 0;
		desc.Width = IMGUI_INDEX_BUFFER_SIZE;
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;

		const bool heap_success = CHECK_D3D12(gpu->device->CreateCommittedResource(
			&heap_props, D3D12_HEAP_FLAG_CREATE_NOT_ZEROED, &desc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&index_buffer)));
		if (!heap_success) {
			GPU_LOG_ERROR("[gpu_lib]: Could not allocate index buffer of size %.2f MiB, exiting.",
				gpuPrintToMiB(IMGUI_INDEX_BUFFER_SIZE));
			return GpuNativeExt{};
		}
		setDebugName(index_buffer.Get(), "imgui_index_buffer");
	}

	// Allocate other buffers
	const GpuPtr vertex_buffer = gpuMalloc(gpu, IMGUI_VERTEX_BUFFER_SIZE);
	const GpuPtr projection_matrix_buffer = gpuMalloc(gpu, sizeof(SfzMat44));

	ImguiD3D12State* state = sfz_new<ImguiD3D12State>(gpu->cfg.cpu_allocator, sfz_dbg("ImguiD3D12State"));

	state->imgui_pso = imgui_pso;
	state->imgui_root_sig = imgui_root_sig;

	state->index_buffer = index_buffer;
	state->index_buffer_state = D3D12_RESOURCE_STATE_COMMON;

	state->vertex_buffer = vertex_buffer;
	state->projection_matrix_buffer = projection_matrix_buffer;

	state->tmp_vertices.init(IMGUI_MAX_NUM_VERTICES, gpu->cfg.cpu_allocator, sfz_dbg(""));
	state->tmp_indices.init(IMGUI_MAX_NUM_INDICES, gpu->cfg.cpu_allocator, sfz_dbg(""));
	state->tmp_cmds.init(256, gpu->cfg.cpu_allocator, sfz_dbg(""));

	GpuNativeExt ext = {};
	ext.ext_data_ptr = state;
	ext.run_func = imguiD3D12Run;
	ext.destroy_func = imguiD3D12Destroy;
	return ext;
}
