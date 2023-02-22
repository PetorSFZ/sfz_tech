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

#include "ZeroUIGpuNativeExtD3D12.h"

#include <sfz_math.h>

#include <gpu_lib_internal_d3d12.hpp>

// ZeroUI D3D12 State
// ------------------------------------------------------------------------------------------------

constexpr u32 MAX_NUM_INDICES = 16384;
constexpr u32 INDEX_BUFFER_SIZE_BYTES = MAX_NUM_INDICES * sizeof(u16);
constexpr u32 MAX_NUM_VERTICES = 16384;
constexpr u32 VERTEX_BUFFER_SIZE_BYTES = MAX_NUM_VERTICES * sizeof(ZuiVertex);
constexpr u32 MAX_NUM_TRANSFORMS = 1024;
constexpr u32 TRANSFORMS_BUFFER_SIZE_BYTES = MAX_NUM_TRANSFORMS * sizeof(SfzMat44);

struct ZeroUID3D12State {

	// ZeroUI shader
	ComPtr<ID3D12PipelineState> zui_pso;
	ComPtr<ID3D12RootSignature> zui_root_sig;

	// Index buffer
	ComPtr<ID3D12Resource> index_buffer;
	D3D12_RESOURCE_STATES index_buffer_state;

	// Other buffers
	GpuPtr vertex_buffer;
	GpuPtr transforms_buffer;
};

// ZeroUI D3D12 Functions
// ------------------------------------------------------------------------------------------------

static void zerouiD3D12Run(GpuLib* gpu, void* ext_data_ptr, void* params_in, u32 params_size)
{
	sfz_assert(ext_data_ptr != nullptr);
	ZeroUID3D12State& state = *static_cast<ZeroUID3D12State*>(ext_data_ptr);
	sfz_assert(params_in != nullptr);
	sfz_assert(params_size == sizeof(ZeroUINativeExtD3D12Params));
	ZeroUINativeExtD3D12Params& params = *static_cast<ZeroUINativeExtD3D12Params*>(params_in);

	// Grab render data from ZeroUI
	ZuiRenderDataView zui_data = zuiGetRenderData(params.zui);
	if (zui_data.num_cmds == 0) return;

	// Upload data to GPU
	const u32 index_buffer_size_bytes = zui_data.num_indices * sizeof(u16);
	{
		// Try to allocate a range
		const GpuHeapRangeAlloc range_alloc = gpuAllocUploadHeapRange(gpu, index_buffer_size_bytes);
		if (!range_alloc.success) {
			GPU_LOG_ERROR("[gpu_lib]: Upload heap overflow by %u bytes.",
				u32(range_alloc.end - gpu->upload_heap_safe_offset));
			return;
		}

		// Memcpy data to upload heap and commit change
		memcpy(gpu->upload_heap_mapped_ptr + range_alloc.begin_mapped, zui_data.indices, index_buffer_size_bytes);
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
	gpuQueueMemcpyUpload(gpu, state.vertex_buffer, zui_data.vertices, zui_data.num_vertices * sizeof(ZuiVertex));
	gpuQueueMemcpyUpload(gpu, state.transforms_buffer, zui_data.transforms, zui_data.num_transforms * sizeof(SfzMat44));

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
		index_buffer_view.Format = DXGI_FORMAT_R16_UINT;
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
	gpu->cmd_list->SetPipelineState(state.zui_pso.Get());
	gpu->cmd_list->SetGraphicsRootSignature(state.zui_root_sig.Get());
	gpu->cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set common root signature stuff
	gpu->cmd_list->SetGraphicsRootShaderResourceView(
		GPU_ROOT_PARAM_GLOBAL_HEAP_IDX, gpu->gpu_heap->GetGPUVirtualAddress());
	gpu->cmd_list->SetGraphicsRootDescriptorTable(
		GPU_ROOT_PARAM_TEX_HEAP_IDX, gpu->tex_descriptor_heap_start_gpu);

	// Execute render commands
	for (u32 cmd_idx = 0; cmd_idx < zui_data.num_cmds; cmd_idx++) {
		const ZuiRenderCmd& cmd = zui_data.cmds[cmd_idx];

		// Set clipping
		if (cmd.clip.min == f32x2_splat(0.0f) && cmd.clip.max == f32x2_splat(0.0f)) {
			// Default scissor
			D3D12_RECT scissor_rect = {};
			scissor_rect.left = 0;
			scissor_rect.top = 0;
			scissor_rect.right = LONG_MAX;
			scissor_rect.bottom = LONG_MAX;
			gpu->cmd_list->RSSetScissorRects(1, &scissor_rect);
		}
		else {
			// Invert coordinate space so that (0,0) is in top left corner and pos-y is down
			ZuiBox clip = cmd.clip;
			clip.min.y = f32(zui_data.fb_dims.y) - clip.min.y;
			clip.max.y = f32(zui_data.fb_dims.y) - clip.max.y;
			sfzSwap(clip.min.y, clip.max.y);

			// Default scissor
			D3D12_RECT scissor_rect = {};
			scissor_rect.left = u32(f32_max(sfz::floor(clip.min.x), 0.0f));
			scissor_rect.top = u32(f32_max(sfz::floor(clip.min.y), 0.0f));
			scissor_rect.right = u32(sfz::ceil(clip.max.x));
			scissor_rect.bottom = u32(sfz::ceil(clip.max.y));
			//scissor_rect.right = scissor_rect.left + u32(f32_max(clip.dims().x, 0.0f));
			//scissor_rect.bottom = scissor_rect.top + u32(f32_max(clip.dims().y, 0.0f));
			gpu->cmd_list->RSSetScissorRects(1, &scissor_rect);
		}

		// Set launch params
		struct ZeroUILaunchParams {
			u32 cmd_type;
			u32 transform_idx;
			GpuPtr vertex_buffer;
			GpuPtr transforms_buffer;
			GpuTexIdx tex_idx;
			u16 padding0;
			u32 padding1;
			u32 padding2;
			u32 padding3;
		};
		ZeroUILaunchParams launch_params = ZeroUILaunchParams{
			.cmd_type = u32(cmd.cmd_type),
			.transform_idx = cmd.transform_idx,
			.vertex_buffer = state.vertex_buffer,
			.transforms_buffer = state.transforms_buffer,
			.tex_idx = u16(cmd.image_handle)
		};
		gpu->cmd_list->SetGraphicsRoot32BitConstants(
			GPU_ROOT_PARAM_LAUNCH_PARAMS_IDX, sizeof(launch_params) / 4, &launch_params, 0);

		// Draw
		gpu->cmd_list->DrawIndexedInstanced(cmd.num_indices, 1, cmd.start_index, 0, 0);
	}

	// Restore scissor to default just in case
	D3D12_RECT scissor_rect = {};
	scissor_rect.left = 0;
	scissor_rect.top = 0;
	scissor_rect.right = LONG_MAX;
	scissor_rect.bottom = LONG_MAX;
	gpu->cmd_list->RSSetScissorRects(1, &scissor_rect);
}

static void zerouiD3D12Destroy(GpuLib* gpu, void* ext_data_ptr)
{
	sfz_assert(ext_data_ptr != nullptr);
	ZeroUID3D12State* state = static_cast<ZeroUID3D12State*>(ext_data_ptr);

	gpuFree(gpu, state->vertex_buffer);
	gpuFree(gpu, state->transforms_buffer);

	sfz_delete(gpu->cfg.cpu_allocator, state);
}

// ZeroUI D3D12 shaders
// ------------------------------------------------------------------------------------------------

constexpr const char ZEROUI_SHADER_SRC[] = R"(

struct ZuiVertex {
	float2 pos;
	float2 texcoord;
	float4 color;
};

static const uint ZUI_CMD_COLOR = 0;
static const uint ZUI_CMD_TEXTURE = 1;
static const uint ZUI_CMD_FONT_ATLAS = 2;

struct ZeroUILaunchParams {
	uint cmd_type;
	uint transform_idx;
	GpuPtr vertex_buffer;
	GpuPtr transforms_buffer;
	GpuTexIdx tex_idx;
	uint16_t padding0;
	uint padding1;
	uint padding2;
	uint padding3;
};
static_assert(sizeof(ZeroUILaunchParams) == sizeof(uint) * 8);
GPU_DECLARE_LAUNCH_PARAMS(ZeroUILaunchParams, params);

struct VSInput {
	uint vertex_idx : SV_VertexID;
};

struct VSOutput {
	float2 texcoord : PARAM_0;
	float4 color : PARAM_1;
	float4 pos : SV_Position;
};

VSOutput VSMain(VSInput input)
{
	const ZuiVertex v = ptrLoadArrayElem<ZuiVertex>(params.vertex_buffer, input.vertex_idx);
	const row_major float4x4 transform =
		ptrLoadArrayElem<row_major float4x4>(params.transforms_buffer, params.transform_idx);
	VSOutput output;
	output.pos = mul(transform, float4(v.pos, 0.0f, 1.0f));
	output.texcoord = v.texcoord;
	output.color = v.color;
	return output;
}

struct PSInput {
	float2 texcoord : PARAM_0;
	float4 color : PARAM_1;
};

float4 PSMain(PSInput input) : SV_TARGET
{
	if (params.cmd_type == ZUI_CMD_COLOR) {
		return input.color;
	}
	else if (params.cmd_type == ZUI_CMD_TEXTURE) {
		Texture2D color_tex = getTex(params.tex_idx);
		SamplerState color_sampler = getSampler(GPU_LINEAR, GPU_CLAMP, GPU_CLAMP);
		const float4 rgba = color_tex.Sample(color_sampler, input.texcoord);
		return input.color * rgba;
	}
	else if (params.cmd_type == ZUI_CMD_FONT_ATLAS) {
		Texture2D font_tex = getTex(params.tex_idx);
		SamplerState font_sampler = getSampler(GPU_LINEAR, GPU_CLAMP, GPU_CLAMP);
		const float alpha = font_tex.Sample(font_sampler, input.texcoord).r;
		return input.color * float4(1.0, 1.0, 1.0, alpha);
	}
	else {
		// Error
		return float4(1.0, 0.0, 0.0, 1.0);
	}
}
)";
constexpr u32 ZEROUI_SHADER_SRC_SIZE = sizeof(ZEROUI_SHADER_SRC);

// ZeroUI D3D12 Native Extension
// ------------------------------------------------------------------------------------------------

sfz_extern_c GpuNativeExt zerouiGpuNativeExtD3D12Init(GpuLib* gpu)
{
	// Compile ZeroUI shaders
	ComPtr<ID3D12PipelineState> zui_pso;
	ComPtr<ID3D12RootSignature> zui_root_sig;
	{
		// Append prolog to shader source (can probably be done at compile time, but dunno how and
		// not worth the effort to figure out).
		char* src = (char*)gpu->cfg.cpu_allocator->alloc(
			sfz_dbg(""), GPU_KERNEL_PROLOG_SIZE + ZEROUI_SHADER_SRC_SIZE);
		sfz_defer[=]() {
			gpu->cfg.cpu_allocator->dealloc(src);
		};
		memcpy(src, GPU_KERNEL_PROLOG, GPU_KERNEL_PROLOG_SIZE);
		memcpy(src + GPU_KERNEL_PROLOG_SIZE, ZEROUI_SHADER_SRC, ZEROUI_SHADER_SRC_SIZE);
		const u32 src_size = GPU_KERNEL_PROLOG_SIZE + ZEROUI_SHADER_SRC_SIZE;

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
		zui_root_sig = gpuCreateDefaultRootSignature(gpu, false, launch_params_size, "zeroui_root_sig", true);
		sfz_assert_hard(zui_root_sig != nullptr);

		// Create PSO (Pipeline State Object)
		{
			D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {};
			pso_desc.pRootSignature = zui_root_sig.Get();
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
			pso_desc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
			pso_desc.RasterizerState.FrontCounterClockwise = true;
			pso_desc.RasterizerState.DepthClipEnable = true;

			pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

			pso_desc.NumRenderTargets = 1;
			pso_desc.RTVFormats[0] = GPU_SWAPCHAIN_DXGI_FORMAT;
			pso_desc.DSVFormat = DXGI_FORMAT_UNKNOWN;

			pso_desc.SampleDesc = { 1, 0 };

			const bool pso_success = CHECK_D3D12(gpu->device->CreateGraphicsPipelineState(
				&pso_desc, IID_PPV_ARGS(&zui_pso)));
			sfz_assert_hard(pso_success);
			setDebugName(zui_pso.Get(), "zeroui_pso");
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
		desc.Width = INDEX_BUFFER_SIZE_BYTES;
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
				gpuPrintToMiB(INDEX_BUFFER_SIZE_BYTES));
			return GpuNativeExt{};
		}
		setDebugName(index_buffer.Get(), "zeroui_index_buffer");
	}

	// Allocate other buffers
	const GpuPtr vertex_buffer = gpuMalloc(gpu, VERTEX_BUFFER_SIZE_BYTES);
	const GpuPtr transforms_buffer = gpuMalloc(gpu, TRANSFORMS_BUFFER_SIZE_BYTES);

	ZeroUID3D12State* state = sfz_new<ZeroUID3D12State>(gpu->cfg.cpu_allocator, sfz_dbg("ZeroUID3D12State"));

	state->zui_pso = zui_pso;
	state->zui_root_sig = zui_root_sig;

	state->index_buffer = index_buffer;
	state->index_buffer_state = D3D12_RESOURCE_STATE_COMMON;

	state->vertex_buffer = vertex_buffer;
	state->transforms_buffer = transforms_buffer;

	GpuNativeExt ext = {};
	ext.ext_data_ptr = state;
	ext.run_func = zerouiD3D12Run;
	ext.destroy_func = zerouiD3D12Destroy;
	return ext;
}
