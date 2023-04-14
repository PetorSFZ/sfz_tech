// Copyright (c) 2020-2023 Peter Hillerström (skipifzero.com, peter@hstroem.se)
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

#include "ZeroUI_Drawing.hpp"

#include <stdio.h>

#include "ZeroUI_Internal.hpp"

#include <skipifzero_arrays.hpp>
#include <skipifzero_hash_maps.hpp>
#include <skipifzero_strings.hpp>

// Disable warnings for following includes
#if defined(_MSC_VER)
#pragma warning(push, 0)
#elif defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wall"
#pragma clang diagnostic ignored "-Wextra"
#pragma clang diagnostic ignored "-Wpedantic"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

#define STBRP_STATIC
#define STB_RECT_PACK_IMPLEMENTATION
#include <stb_rect_pack.h>

#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

// Re-enable warnings
#if defined(_MSC_VER)
#pragma warning(pop)
#elif defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

// UTF8 decoder
// ------------------------------------------------------------------------------------------------

// Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
// See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.

sfz_constant u8 zui_utf8_d[] = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 00..1f
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 20..3f
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 40..5f
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 60..7f
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, // 80..9f
	7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, // a0..bf
	8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, // c0..df
	0xa,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x4,0x3,0x3, // e0..ef
	0xb,0x6,0x6,0x6,0x5,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8, // f0..ff
	0x0,0x1,0x2,0x3,0x5,0x8,0x7,0x1,0x1,0x1,0x4,0x6,0x1,0x1,0x1,0x1, // s0..s0
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,0,1,1,1,1,1,1, // s1..s2
	1,2,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1, // s3..s4
	1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,3,1,3,1,1,1,1,1,1, // s5..s6
	1,3,1,1,1,1,1,3,1,3,1,1,1,1,1,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // s7..s8
};

static bool zuiDecodeUTF8(u32* state, u32* codep, u32 byte)
{
	u32 type = zui_utf8_d[byte];
	*codep = (*state != 0) ? (byte & 0x3fu) | (*codep << 6) : (0xff >> type) & (byte);
	*state = zui_utf8_d[256 + *state * 16 + type];
	return *state;
}

// Statics
// ------------------------------------------------------------------------------------------------

static_assert(sizeof(stbtt_pack_context) == stbtt_pack_context_size, "");
static_assert(sizeof(stbtt_packedchar) == stbtt_packedchar_size, "");

static SfzArray<u8> zuiReadBinaryFile(const char* path, SfzAllocator* allocator)
{
	// Open file
	FILE* file = fopen(path, "rb");
	if (file == NULL) return SfzArray<u8>();

	// Get size of file
	fseek(file, 0, SEEK_END);
	i64 size = ftell(file);
	if (size <= 0) {
		fclose(file);
		return SfzArray<u8>();
	}
	fseek(file, 0, SEEK_SET);

	// Allocate memory for file
	SfzArray<u8> data;
	data.init(u32(size), allocator, sfz_dbg(""));
	data.hackSetSize(u32(size));

	// Read file
	size_t bytes_read = fread(data.data(), 1, data.size(), file);
	if (bytes_read != size_t(size)) {
		fclose(file);
		return SfzArray<u8>();
	}

	// Close file and return data
	fclose(file);
	return data;
}

static stbtt_aligned_quad zuiGetQuadFromCodepoint(
	ZuiDrawCtx* draw_ctx, const ZuiFontInfo* font_info, u32 codepoint, f32x2* p)
{
	const i32 res = draw_ctx->font_img_res;
	stbtt_aligned_quad quad = {};

	// First check if it's a regular ASCII char
	if (ZUI_FIRST_PRINTABLE_ASCII_CHAR <= codepoint && codepoint <= ZUI_LAST_PRINTABLE_ASCII_CHAR) {
		const i32 char_idx = i32(codepoint - ZUI_FIRST_PRINTABLE_ASCII_CHAR);
		const stbtt_packedchar* range =
			reinterpret_cast<const stbtt_packedchar*>(font_info->ascii_pack_raw);
		stbtt_GetPackedQuad(range, res, res, char_idx, &p->x, &p->y, &quad, 0);
		return quad;
	}

	// If not a regular ASCII char, see if it's in our list of extra chars
	for (u32 char_idx = 0; char_idx < ZUI_NUM_EXTRA_CHARS; char_idx++) {
		const u32 extra_codepoint = ZUI_EXTRA_RANGE_CHARS[char_idx];
		if (codepoint == extra_codepoint) {
			const stbtt_packedchar* range =
				reinterpret_cast<const stbtt_packedchar*>(font_info->extra_pack_raw);
			stbtt_GetPackedQuad(range, res, res, char_idx, &p->x, &p->y, &quad, 0);
			return quad;
		}
	}

	// Otherwise unknown char, return our error char.
	const stbtt_packedchar* range =
		reinterpret_cast<const stbtt_packedchar*>(font_info->extra_pack_raw);
	stbtt_GetPackedQuad(range, res, res, 0, &p->x, &p->y, &quad, 0);
	return quad;
}

// Initialization and internal interface
// ------------------------------------------------------------------------------------------------

bool zuiInternalDrawCtxInit(ZuiDrawCtx* draw_ctx, const ZuiCfg* cfg, SfzAllocator* allocator)
{
	// Initialize render data
	draw_ctx->vertices.init(8192, allocator, sfz_dbg("ZeroUI::vertices"));
	draw_ctx->indices.init(8192, allocator, sfz_dbg("ZeroUI::indices"));
	draw_ctx->transforms.init(512, allocator, sfz_dbg("ZeroUI::transforms"));
	draw_ctx->render_cmds.init(512, allocator, sfz_dbg("ZeroUI::render_cmds"));

	// Initialize clip stack
	draw_ctx->clip_stack.init(128, allocator, sfz_dbg("ZeroUI::clip_stack"));

	// Allocate memory for font image
	draw_ctx->font_img_res = ZUI_FONT_TEX_RES;
	draw_ctx->font_img.init(
		draw_ctx->font_img_res * draw_ctx->font_img_res, allocator, sfz_dbg("ZeroUI::font_img"));
	draw_ctx->font_img.add(u8(0), draw_ctx->font_img.capacity());
	draw_ctx->font_img_modified = true;

	const i32 res = stbtt_PackBegin(
		draw_ctx->packCtx(),
		draw_ctx->font_img.data(),
		draw_ctx->font_img_res,
		draw_ctx->font_img_res,
		draw_ctx->font_img_res,
		1,
		nullptr); // TODO: Can pass allocator here somehow
	if (res == 0) return false;

	// Set oversampling
	u32 oversample = cfg->oversample_fonts;
	if (oversample == 0) oversample = 1;
	stbtt_PackSetOversampling(draw_ctx->packCtx(), oversample, oversample);

	// Allocate memory for fonts hashmap
	draw_ctx->fonts.init(64, allocator, sfz_dbg("ZeroUI::fonts"));

	return true;
}

void zuiInternalDrawCtxDestroy(ZuiDrawCtx* draw_ctx)
{
	if (draw_ctx == nullptr) return;
	stbtt_PackEnd(draw_ctx->packCtx());
}

bool zuiInternalDrawAddFont(
	ZuiDrawCtx* draw_ctx, ZuiID id, const char* ttf_path, f32 atlas_size, SfzAllocator* allocator)
{
	if (draw_ctx == nullptr) return false;

	// Can't add font if we already have it
	if (draw_ctx->fonts.get(id.id) != nullptr) return false;

	// Read .ttf file, exit out on failure
	SfzArray<u8> ttf_data = zuiReadBinaryFile(ttf_path, allocator);
	if (ttf_data.isEmpty()) return false;

	// Load font from file
	ZuiFontInfo& font_info = draw_ctx->fonts.put(id.id, ZuiFontInfo{});
	font_info.ttf_data = sfz_move(ttf_data);
	font_info.atlas_size = atlas_size;
	stbtt_pack_range ranges[2] = {};
	ranges[0].font_size = atlas_size;
	ranges[0].first_unicode_codepoint_in_range = ZUI_FIRST_PRINTABLE_ASCII_CHAR;
	ranges[0].num_chars = ZUI_NUM_PRINTABLE_ASCII_CHARS;
	ranges[0].chardata_for_range = reinterpret_cast<stbtt_packedchar*>(font_info.ascii_pack_raw);
	ranges[1].font_size = atlas_size;
	i32 codepoint_array[ZUI_NUM_EXTRA_CHARS];
	memcpy(codepoint_array, ZUI_EXTRA_RANGE_CHARS, sizeof(ZUI_EXTRA_RANGE_CHARS));
	ranges[1].array_of_unicode_codepoints = codepoint_array;
	ranges[1].num_chars = ZUI_NUM_EXTRA_CHARS;
	ranges[1].chardata_for_range = reinterpret_cast<stbtt_packedchar*>(font_info.extra_pack_raw);

	const i32 pack_success =
		stbtt_PackFontRanges(draw_ctx->packCtx(), font_info.ttf_data.data(), 0, ranges, 2);
	if (pack_success == 0) {
		draw_ctx->fonts.remove(id.id);
		return false;
	}

	// Mark font image modified, user needs to reupload to GPU
	draw_ctx->font_img_modified = true;

	return true;
}

// Low-level drawing functions
// ------------------------------------------------------------------------------------------------

void zuiDrawAddCommand(
	ZuiDrawCtx* draw_ctx,
	const SfzMat44& transform,
	const ZuiVertex* vertices,
	u32 num_vertices,
	const u32* indices,
	u32 num_indices,
	ZuiCmdType cmd_type,
	u64 image_handle)
{
	// Add vertices and indices
	const u32 start_index = draw_ctx->indices.size();
	const u32 start_vertex = draw_ctx->vertices.size();
	draw_ctx->vertices.add(vertices, num_vertices);
	for (u32 i = 0; i < num_indices; i++) {
		u32 idx = start_vertex + indices[i];
		sfz_assert(idx < U16_MAX);
		draw_ctx->indices.add(u16(idx));
	}

	// Setup transform
	const u32 transform_idx = draw_ctx->transforms.size();
	draw_ctx->transforms.add(transform);

	// Create command
	ZuiRenderCmd& cmd = draw_ctx->render_cmds.add();
	cmd = {};
	cmd.cmd_type = cmd_type;
	cmd.transform_idx = transform_idx;
	cmd.start_index = start_index;
	cmd.num_indices = num_indices;
	cmd.clip = draw_ctx->clip_stack.last();
	cmd.image_handle = image_handle;
}

f32 zuiDrawText(
	ZuiDrawCtx* draw_ctx,
	const SfzMat44& transform,
	ZuiAlign align,
	ZuiID font_id,
	f32 size,
	f32x4 color,
	const char* text)
{
	const ZuiFontInfo* font_info = draw_ctx->fonts.get(font_id.id);
	sfz_assert(font_info != nullptr);
	const f32 scale = size / font_info->atlas_size;

	// Get length of string by iterating over the codepoints in it
	f32 string_len = 0.0f;
	{
		// Iterate over codepoints in string
		u32 state = 0;
		u32 codepoint = 0;
		f32x2 p = f32x2_splat(0.0f);
		for (const char* text_itr = text; *text_itr; text_itr++) {
			const u8 byte = *text_itr;
			if (zuiDecodeUTF8(&state, &codepoint, byte)) continue;
			const stbtt_aligned_quad quad = zuiGetQuadFromCodepoint(draw_ctx, font_info, codepoint, &p);
		}
		string_len = p.x;
	}

	// Setup initial transform
	const u32 transform_idx = draw_ctx->transforms.size();
	{
		sfz_assert(align == ZUI_MID_CENTER || align == ZUI_MID_LEFT || align == ZUI_MID_RIGHT);
		f32 x_offset = -string_len * 0.5f;
		if (align == ZUI_MID_LEFT) {
			x_offset = 0.0f;
		}
		else if (align == ZUI_MID_RIGHT) {
			x_offset = -string_len;
		}
		SfzMat44& x_form = draw_ctx->transforms.add();
		x_form =
			transform *
			sfzMat44Scaling3(f32x3_splat(scale)) *
			sfzMat44Translation3(f32x3_init(x_offset, -font_info->atlas_size * 0.3f, 0.0f));
	}

	// Setup initial render command
	ZuiRenderCmd& cmd = draw_ctx->render_cmds.add();
	cmd = {};
	cmd.cmd_type = ZUI_CMD_FONT_ATLAS;
	cmd.transform_idx = transform_idx;
	cmd.start_index = draw_ctx->indices.size();
	cmd.clip = draw_ctx->clip_stack.last();
	cmd.image_handle = draw_ctx->user_font_tex_handle;

	// Iterate over codepoints in string
	u32 state = 0;
	u32 codepoint = 0;
	f32x2 p = f32x2_splat(0.0f);
	for (const char* text_itr = text; *text_itr; text_itr++) {
		const u8 byte = *text_itr;
		if (zuiDecodeUTF8(&state, &codepoint, byte)) continue;
		const stbtt_aligned_quad quad = zuiGetQuadFromCodepoint(draw_ctx, font_info, codepoint, &p);

		const u16 base_idx = u16(draw_ctx->vertices.size());
		{
			// Bottom left
			ZuiVertex& v = draw_ctx->vertices.add();
			v.pos = f32x2_init(quad.x0, -quad.y1);
			v.texcoord = f32x2_init(quad.s0, quad.t1);
			v.color = color.xyz();
			v.alpha = color.w;
		}
		{
			// Bottom right
			ZuiVertex& v = draw_ctx->vertices.add();
			v.pos = f32x2_init(quad.x1, -quad.y1);
			v.texcoord = f32x2_init(quad.s1, quad.t1);
			v.color = color.xyz();
			v.alpha = color.w;
		}
		{
			// Top left
			ZuiVertex& v = draw_ctx->vertices.add();
			v.pos = f32x2_init(quad.x0, -quad.y0);
			v.texcoord = f32x2_init(quad.s0, quad.t0);
			v.color = color.xyz();
			v.alpha = color.w;
		}
		{
			// Top right
			ZuiVertex& v = draw_ctx->vertices.add();
			v.pos = f32x2_init(quad.x1, -quad.y0);
			v.texcoord = f32x2_init(quad.s1, quad.t0);
			v.color = color.xyz();
			v.alpha = color.w;
		}

		draw_ctx->indices.add(base_idx);
		draw_ctx->indices.add(base_idx + 1);
		draw_ctx->indices.add(base_idx + 2);
		draw_ctx->indices.add(base_idx + 1);
		draw_ctx->indices.add(base_idx + 3);
		draw_ctx->indices.add(base_idx + 2);
		cmd.num_indices += 6;
	}

	return string_len * scale;
}

void zuiDrawImage(
	ZuiDrawCtx* draw_ctx,
	const SfzMat44& transform,
	f32x2 dims,
	u64 image_handle)
{
	const f32x2 half_dims = dims * 0.5f;

	// Bottom left
	ZuiVertex verts[4];
	verts[0].pos = f32x2_init(-half_dims.x, -half_dims.y);
	verts[0].texcoord = draw_ctx->img_flip_y ? f32x2_init(0.0f, 1.0f) : f32x2_init(0.0f, 0.0f);
	verts[0].color = f32x3_splat(1.0f);
	verts[0].alpha = 1.0f;

	// Bottom right
	verts[1].pos = f32x2_init(half_dims.x, -half_dims.y);
	verts[1].texcoord = draw_ctx->img_flip_y ? f32x2_init(1.0f, 1.0f) : f32x2_init(1.0f, 0.0f);
	verts[1].color = f32x3_splat(1.0f);
	verts[1].alpha = 1.0f;

	// Top left
	verts[2].pos = f32x2_init(-half_dims.x, half_dims.y);
	verts[2].texcoord = draw_ctx->img_flip_y ? f32x2_init(0.0f, 0.0f) : f32x2_init(0.0f, 1.0f);
	verts[2].color = f32x3_splat(1.0f);
	verts[2].alpha = 1.0f;

	// Top right
	verts[3].pos = f32x2_init(half_dims.x, half_dims.y);
	verts[3].texcoord = draw_ctx->img_flip_y ? f32x2_init(1.0f, 0.0f) : f32x2_init(1.0f, 1.0f);
	verts[3].color = f32x3_splat(1.0f);
	verts[3].alpha = 1.0f;

	const u32 indices[6] = {
		0, 1, 2,
		1, 3, 2
	};

	zuiDrawAddCommand(draw_ctx, transform, verts, 4, indices, 6, ZUI_CMD_TEXTURE, image_handle);
}

void zuiDrawImageGrayscale(
	ZuiDrawCtx* draw_ctx,
	const SfzMat44& transform,
	f32x2 dims,
	u64 image_handle,
	f32x4 tint_color)
{
	const f32x2 half_dims = dims * 0.5f;

	// Bottom left
	ZuiVertex verts[4];
	verts[0].pos = f32x2_init(-half_dims.x, -half_dims.y);
	verts[0].texcoord = draw_ctx->img_flip_y ? f32x2_init(0.0f, 1.0f) : f32x2_init(0.0f, 0.0f);
	verts[0].color = tint_color.xyz();
	verts[0].alpha = tint_color.w;

	// Bottom right
	verts[1].pos = f32x2_init(half_dims.x, -half_dims.y);
	verts[1].texcoord = draw_ctx->img_flip_y ? f32x2_init(1.0f, 1.0f) : f32x2_init(1.0f, 0.0f);
	verts[1].color = tint_color.xyz();
	verts[1].alpha = tint_color.w;

	// Top left
	verts[2].pos = f32x2_init(-half_dims.x, half_dims.y);
	verts[2].texcoord = draw_ctx->img_flip_y ? f32x2_init(0.0f, 0.0f) : f32x2_init(0.0f, 1.0f);
	verts[2].color = tint_color.xyz();
	verts[2].alpha = tint_color.w;

	// Top right
	verts[3].pos = f32x2_init(half_dims.x, half_dims.y);
	verts[3].texcoord = draw_ctx->img_flip_y ? f32x2_init(1.0f, 0.0f) : f32x2_init(1.0f, 1.0f);
	verts[3].color = tint_color.xyz();
	verts[3].alpha = tint_color.w;

	const u32 indices[6] = {
		0, 1, 2,
		1, 3, 2
	};

	zuiDrawAddCommand(draw_ctx, transform, verts, 4, indices, 6, ZUI_CMD_TEXTURE_GRAYSCALE, image_handle);
}

void zuiDrawRect(
	ZuiDrawCtx* draw_ctx,
	const SfzMat44& transform,
	f32x2 dims,
	f32x4 color)
{
	const f32x2 half_dims = dims * 0.5f;

	// Bottom left
	ZuiVertex verts[4];
	verts[0].pos = f32x2_init(-half_dims.x, -half_dims.y);
	verts[0].texcoord = f32x2_init(0.0f, 0.0f);
	verts[0].color = color.xyz();
	verts[0].alpha = color.w;

	// Bottom right
	verts[1].pos = f32x2_init(half_dims.x, -half_dims.y);
	verts[1].texcoord = f32x2_init(1.0f, 0.0f);
	verts[1].color = color.xyz();
	verts[1].alpha = color.w;

	// Top left
	verts[2].pos = f32x2_init(-half_dims.x, half_dims.y);
	verts[2].texcoord = f32x2_init(0.0f, 1.0f);
	verts[2].color = color.xyz();
	verts[2].alpha = color.w;

	// Top right
	verts[3].pos = f32x2_init(half_dims.x, half_dims.y);
	verts[3].texcoord = f32x2_init(1.0f, 1.0f);
	verts[3].color = color.xyz();
	verts[3].alpha = color.w;

	const u32 indices[6] = {
		0, 1, 2,
		1, 3, 2
	};

	zuiDrawAddCommand(draw_ctx, transform, verts, 4, indices, 6, ZUI_CMD_COLOR, 0);
}

void zuiDrawBorder(
	ZuiDrawCtx* draw_ctx,
	const SfzMat44& transform,
	f32x2 dims,
	f32 thickness,
	f32x4 color)
{
	const f32x2 half_dims = dims * 0.5f;
	const f32x2 corner_bottom_left = -half_dims;
	const f32x2 corner_bottom_right = f32x2_init(half_dims.x, -half_dims.y);
	const f32x2 corner_top_left = f32x2_init(-half_dims.x, half_dims.y);
	const f32x2 corner_top_right = half_dims;

	auto createVertex = [&](f32x2 pos) -> ZuiVertex {
		ZuiVertex v;
		v.pos = pos;
		f32x2 interp = f32x2_clamps((pos - corner_bottom_left) / (corner_top_right - corner_bottom_left), 0.0f, 1.0f);
		v.texcoord = f32x2_init(sfz::lerp(0.0f, 1.0f, interp.x), sfz::lerp(0.0f, 1.0f, interp.y));
		v.color = color.xyz();
		v.alpha = color.w;
		return v;
	};

	constexpr u32 MAX_NUM_VERTICES = 16;
	constexpr u32 MAX_NUM_INDICES = 24;
	SfzArrayLocal<ZuiVertex, MAX_NUM_VERTICES> verts;
	SfzArrayLocal<u32, MAX_NUM_INDICES> indices;

	auto addTriangleIndices = [&](u32 base, u32 idx0, u32 idx1, u32 idx2) {
		indices.add(base + idx0);
		indices.add(base + idx1);
		indices.add(base + idx2);
	};

	// Top line
	{
		const f32x2 bottom_left = f32x2_init(-half_dims.x, half_dims.y - thickness);
		const f32x2 bottom_right = f32x2_init(half_dims.x, half_dims.y - thickness);
		const f32x2 top_left = f32x2_init(-half_dims.x, half_dims.y);
		const f32x2 top_right = f32x2_init(half_dims.x, half_dims.y);

		verts.add() = createVertex(bottom_left);
		verts.add() = createVertex(bottom_right);
		verts.add() = createVertex(top_left);
		verts.add() = createVertex(top_right);
		addTriangleIndices(0, 0, 1, 2);
		addTriangleIndices(0, 1, 3, 2);
	}

	// Bottom line
	{
		const f32x2 bottom_left = f32x2_init(-half_dims.x, -half_dims.y);
		const f32x2 bottom_right = f32x2_init(half_dims.x, -half_dims.y);
		const f32x2 top_left = f32x2_init(-half_dims.x, -half_dims.y + thickness);
		const f32x2 top_right = f32x2_init(half_dims.x, -half_dims.y + thickness);

		verts.add() = createVertex(bottom_left);
		verts.add() = createVertex(bottom_right);
		verts.add() = createVertex(top_left);
		verts.add() = createVertex(top_right);
		addTriangleIndices(4, 0, 1, 2);
		addTriangleIndices(4, 1, 3, 2);
	}

	// Left line
	{
		const f32x2 bottom_left = f32x2_init(-half_dims.x, -half_dims.y + thickness);
		const f32x2 bottom_right = f32x2_init(-half_dims.x + thickness, -half_dims.y + thickness);
		const f32x2 top_left = f32x2_init(-half_dims.x, half_dims.y - thickness);
		const f32x2 top_right = f32x2_init(-half_dims.x + thickness, half_dims.y - thickness);

		verts.add() = createVertex(bottom_left);
		verts.add() = createVertex(bottom_right);
		verts.add() = createVertex(top_left);
		verts.add() = createVertex(top_right);
		addTriangleIndices(8, 0, 1, 2);
		addTriangleIndices(8, 1, 3, 2);
	}

	// Right line
	{
		const f32x2 bottom_left = f32x2_init(half_dims.x - thickness, -half_dims.y + thickness);
		const f32x2 bottom_right = f32x2_init(half_dims.x, -half_dims.y + thickness);
		const f32x2 top_left = f32x2_init(half_dims.x - thickness, half_dims.y - thickness);
		const f32x2 top_right = f32x2_init(half_dims.x, half_dims.y - thickness);

		verts.add() = createVertex(bottom_left);
		verts.add() = createVertex(bottom_right);
		verts.add() = createVertex(top_left);
		verts.add() = createVertex(top_right);
		addTriangleIndices(12, 0, 1, 2);
		addTriangleIndices(12, 1, 3, 2);
	}

	zuiDrawAddCommand(
		draw_ctx, transform, verts.data(), verts.size(), indices.data(), indices.size(), ZUI_CMD_COLOR, 0);
}
