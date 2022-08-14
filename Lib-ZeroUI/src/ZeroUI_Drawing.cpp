// Copyright (c) 2020 Peter Hillerström (skipifzero.com, peter@hstroem.se)
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

sfz_constant u8 zuiUtf8d[] = {
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
	u32 type = zuiUtf8d[byte];
	*codep = (*state != 0) ? (byte & 0x3fu) | (*codep << 6) : (0xff >> type) & (byte);
	*state = zuiUtf8d[256 + *state * 16 + type];
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
	size_t bytesRead = fread(data.data(), 1, data.size(), file);
	if (bytesRead != size_t(size)) {
		fclose(file);
		return SfzArray<u8>();
	}

	// Close file and return data
	fclose(file);
	return data;
}

static stbtt_aligned_quad zuiGetQuadFromCodepoint(
	ZuiDrawCtx* drawCtx, const ZuiFontInfo* fontInfo, u32 codepoint, f32x2* p)
{
	const i32 res = drawCtx->fontImgRes;
	stbtt_aligned_quad quad = {};

	// First check if it's a regular ASCII char
	if (ZUI_FIRST_PRINTABLE_ASCII_CHAR <= codepoint && codepoint <= ZUI_LAST_PRINTABLE_ASCII_CHAR) {
		const i32 charIdx = i32(codepoint - ZUI_FIRST_PRINTABLE_ASCII_CHAR);
		const stbtt_packedchar* range =
			reinterpret_cast<const stbtt_packedchar*>(fontInfo->asciiPackRaw);
		stbtt_GetPackedQuad(range, res, res, charIdx, &p->x, &p->y, &quad, 0);
		return quad;
	}

	// If not a regular ASCII char, see if it's in our list of extra chars
	for (u32 charIdx = 0; charIdx < ZUI_NUM_EXTRA_CHARS; charIdx++) {
		const u32 extraCodepoint = ZUI_EXTRA_RANGE_CHARS[charIdx];
		if (codepoint == extraCodepoint) {
			const stbtt_packedchar* range =
				reinterpret_cast<const stbtt_packedchar*>(fontInfo->extraPackRaw);
			stbtt_GetPackedQuad(range, res, res, charIdx, &p->x, &p->y, &quad, 0);
			return quad;
		}
	}

	// Otherwise unknown char, return our error char.
	const stbtt_packedchar* range =
		reinterpret_cast<const stbtt_packedchar*>(fontInfo->extraPackRaw);
	stbtt_GetPackedQuad(range, res, res, 0, &p->x, &p->y, &quad, 0);
	return quad;
}

// Initialization and internal interface
// ------------------------------------------------------------------------------------------------

bool zuiInternalDrawCtxInit(ZuiDrawCtx* drawCtx, const ZuiCfg* cfg, SfzAllocator* allocator)
{
	// Initialize render data
	drawCtx->vertices.init(8192, allocator, sfz_dbg("ZeroUI::vertices"));
	drawCtx->indices.init(8192, allocator, sfz_dbg("ZeroUI::indices"));
	drawCtx->transforms.init(512, allocator, sfz_dbg("ZeroUI::transforms"));
	drawCtx->renderCmds.init(512, allocator, sfz_dbg("ZeroUI::renderCmds"));

	// Initialize clip stack
	drawCtx->clipStack.init(128, allocator, sfz_dbg("ZeroUI::clipStack"));

	// Allocate memory for font image
	drawCtx->fontImgRes = ZUI_FONT_TEX_RES;
	drawCtx->fontImg.init(
		drawCtx->fontImgRes * drawCtx->fontImgRes, allocator, sfz_dbg("ZeroUI::fontImg"));
	drawCtx->fontImg.add(u8(0), drawCtx->fontImg.capacity());
	drawCtx->fontImgModified = true;

	const i32 res = stbtt_PackBegin(
		drawCtx->packCtx(),
		drawCtx->fontImg.data(),
		drawCtx->fontImgRes,
		drawCtx->fontImgRes,
		drawCtx->fontImgRes,
		1,
		nullptr); // TODO: Can pass allocator here somehow
	if (res == 0) return false;

	// Set oversampling
	u32 oversample = cfg->oversampleFonts;
	if (oversample == 0) oversample = 1;
	stbtt_PackSetOversampling(drawCtx->packCtx(), oversample, oversample);

	// Allocate memory for fonts hashmap
	drawCtx->fonts.init(64, allocator, sfz_dbg("ZeroUI::fonts"));

	return true;
}

void zuiInternalDrawCtxDestroy(ZuiDrawCtx* drawCtx)
{
	if (drawCtx == nullptr) return;
	stbtt_PackEnd(drawCtx->packCtx());
}

bool zuiInternalDrawAddFont(
	ZuiDrawCtx* drawCtx, ZuiID id, const char* ttfPath, f32 atlasSize, SfzAllocator* allocator)
{
	if (drawCtx == nullptr) return false;

	// Can't add font if we already have it
	if (drawCtx->fonts.get(id.id) != nullptr) return false;

	// Read .ttf file, exit out on failure
	SfzArray<u8> ttfData = zuiReadBinaryFile(ttfPath, allocator);
	if (ttfData.isEmpty()) return false;

	// Load font from file
	ZuiFontInfo& fontInfo = drawCtx->fonts.put(id.id, ZuiFontInfo{});
	fontInfo.ttfData = sfz_move(ttfData);
	fontInfo.atlasSize = atlasSize;
	stbtt_pack_range ranges[2] = {};
	ranges[0].font_size = atlasSize;
	ranges[0].first_unicode_codepoint_in_range = ZUI_FIRST_PRINTABLE_ASCII_CHAR;
	ranges[0].num_chars = ZUI_NUM_PRINTABLE_ASCII_CHARS;
	ranges[0].chardata_for_range = reinterpret_cast<stbtt_packedchar*>(fontInfo.asciiPackRaw);
	ranges[1].font_size = atlasSize;
	i32 codepointArray[ZUI_NUM_EXTRA_CHARS];
	memcpy(codepointArray, ZUI_EXTRA_RANGE_CHARS, sizeof(ZUI_EXTRA_RANGE_CHARS));
	ranges[1].array_of_unicode_codepoints = codepointArray;
	ranges[1].num_chars = ZUI_NUM_EXTRA_CHARS;
	ranges[1].chardata_for_range = reinterpret_cast<stbtt_packedchar*>(fontInfo.extraPackRaw);

	const i32 packSuccess =
		stbtt_PackFontRanges(drawCtx->packCtx(), fontInfo.ttfData.data(), 0, ranges, 2);
	if (packSuccess == 0) {
		drawCtx->fonts.remove(id.id);
		return false;
	}

	// Mark font image modified, user needs to reupload to GPU
	drawCtx->fontImgModified = true;

	return true;
}

// Low-level drawing functions
// ------------------------------------------------------------------------------------------------

void zuiDrawAddCommand(
	ZuiDrawCtx* drawCtx,
	const SfzMat44& transform,
	const ZuiVertex* vertices,
	u32 numVertices,
	const u32* indices,
	u32 numIndices,
	ZuiCmdType cmdType,
	u64 imageHandle)
{
	// Add vertices and indices
	const u32 startIndex = drawCtx->indices.size();
	const u32 startVertex = drawCtx->vertices.size();
	drawCtx->vertices.add(vertices, numVertices);
	for (u32 i = 0; i < numIndices; i++) {
		u32 idx = startVertex + indices[i];
		sfz_assert(idx < U16_MAX);
		drawCtx->indices.add(u16(idx));
	}

	// Setup transform
	const u32 transformIdx = drawCtx->transforms.size();
	drawCtx->transforms.add(transform);

	// Create command
	ZuiRenderCmd& cmd = drawCtx->renderCmds.add();
	cmd = {};
	cmd.cmdType = cmdType;
	cmd.transformIdx = transformIdx;
	cmd.startIndex = startIndex;
	cmd.numIndices = numIndices;
	cmd.clip = drawCtx->clipStack.last();
	cmd.imageHandle = imageHandle;
}

f32 zuiDrawTextCentered(
	ZuiDrawCtx* drawCtx,
	const SfzMat44& transform,
	ZuiID fontID,
	f32 size,
	f32x4 color,
	const char* text)
{
	const ZuiFontInfo* fontInfo = drawCtx->fonts.get(fontID.id);
	sfz_assert(fontInfo != nullptr);
	const f32 scale = size / fontInfo->atlasSize;
	
	// Get length of string by iterating over the codepoints in it
	f32 stringLen = 0.0f;
	{
		// Iterate over codepoints in string
		u32 state = 0;
		u32 codepoint = 0;
		f32x2 p = f32x2_splat(0.0f);
		for (const char* textItr = text; *textItr; textItr++) {
			const u8 byte = *textItr;
			if (zuiDecodeUTF8(&state, &codepoint, byte)) continue;
			const stbtt_aligned_quad quad = zuiGetQuadFromCodepoint(drawCtx, fontInfo, codepoint, &p);
		}
		stringLen = p.x;
	}

	// Setup initial transform
	const u32 transformIdx = drawCtx->transforms.size();
	{
		SfzMat44& xform = drawCtx->transforms.add();
		xform =
			transform *
			sfzMat44Scaling3(f32x3_splat(scale)) *
			sfzMat44Translation3(f32x3_init(-stringLen * 0.5f, -fontInfo->atlasSize * 0.3f, 0.0f));
	}

	// Setup initial render command
	ZuiRenderCmd& cmd = drawCtx->renderCmds.add();
	cmd = {};
	cmd.cmdType = ZUI_CMD_FONT_ATLAS;
	cmd.transformIdx = transformIdx;
	cmd.startIndex = drawCtx->indices.size();
	cmd.clip = drawCtx->clipStack.last();
	cmd.imageHandle = drawCtx->userFontTexHandle;

	// Iterate over codepoints in string
	u32 state = 0;
	u32 codepoint = 0;
	f32x2 p = f32x2_splat(0.0f);
	for (const char* textItr = text; *textItr; textItr++) {
		const u8 byte = *textItr;
		if (zuiDecodeUTF8(&state, &codepoint, byte)) continue;
		const stbtt_aligned_quad quad = zuiGetQuadFromCodepoint(drawCtx, fontInfo, codepoint, &p);

		const u16 baseIdx = u16(drawCtx->vertices.size());
		{
			// Bottom left
			ZuiVertex& v = drawCtx->vertices.add();
			v.pos = f32x2_init(quad.x0, -quad.y1);
			v.texcoord = f32x2_init(quad.s0, quad.t1);
			v.color = color.xyz();
			v.alpha = color.w;
		}
		{
			// Bottom right
			ZuiVertex& v = drawCtx->vertices.add();
			v.pos = f32x2_init(quad.x1, -quad.y1);
			v.texcoord = f32x2_init(quad.s1, quad.t1);
			v.color = color.xyz();
			v.alpha = color.w;
		}
		{
			// Top left
			ZuiVertex& v = drawCtx->vertices.add();
			v.pos = f32x2_init(quad.x0, -quad.y0);
			v.texcoord = f32x2_init(quad.s0, quad.t0);
			v.color = color.xyz();
			v.alpha = color.w;
		}
		{
			// Top right
			ZuiVertex& v = drawCtx->vertices.add();
			v.pos = f32x2_init(quad.x1, -quad.y0);
			v.texcoord = f32x2_init(quad.s1, quad.t0);
			v.color = color.xyz();
			v.alpha = color.w;
		}

		drawCtx->indices.add(baseIdx);
		drawCtx->indices.add(baseIdx + 1);
		drawCtx->indices.add(baseIdx + 2);
		drawCtx->indices.add(baseIdx + 1);
		drawCtx->indices.add(baseIdx + 3);
		drawCtx->indices.add(baseIdx + 2);
		cmd.numIndices += 6;
	}

	return stringLen * scale;
}

void zuiDrawImage(
	ZuiDrawCtx* drawCtx,
	const SfzMat44& transform,
	f32x2 dims,
	u64 imageHandle)
{
	const f32x2 halfDims = dims * 0.5f;

	// Bottom left
	ZuiVertex verts[4];
	verts[0].pos = f32x2_init(-halfDims.x, -halfDims.y);
	verts[0].texcoord = drawCtx->imgFlipY ? f32x2_init(0.0f, 1.0f) : f32x2_init(0.0f, 0.0f);
	verts[0].color = f32x3_splat(1.0f);
	verts[0].alpha = 1.0f;

	// Bottom right
	verts[1].pos = f32x2_init(halfDims.x, -halfDims.y);
	verts[1].texcoord = drawCtx->imgFlipY ? f32x2_init(1.0f, 1.0f) : f32x2_init(1.0f, 0.0f);
	verts[1].color = f32x3_splat(1.0f);
	verts[1].alpha = 1.0f;

	// Top left
	verts[2].pos = f32x2_init(-halfDims.x, halfDims.y);
	verts[2].texcoord = drawCtx->imgFlipY ? f32x2_init(0.0f, 0.0f) : f32x2_init(0.0f, 1.0f);
	verts[2].color = f32x3_splat(1.0f);
	verts[2].alpha = 1.0f;

	// Top right
	verts[3].pos = f32x2_init(halfDims.x, halfDims.y);
	verts[3].texcoord = drawCtx->imgFlipY ? f32x2_init(1.0f, 0.0f) : f32x2_init(1.0f, 1.0f);
	verts[3].color = f32x3_splat(1.0f);
	verts[3].alpha = 1.0f;

	const u32 indices[6] = {
		0, 1, 2,
		1, 3, 2
	};

	zuiDrawAddCommand(drawCtx, transform, verts, 4, indices, 6, ZUI_CMD_TEXTURE, imageHandle);
}

void zuiDrawRect(
	ZuiDrawCtx* drawCtx,
	const SfzMat44& transform,
	f32x2 dims,
	f32x4 color)
{
	const f32x2 halfDims = dims * 0.5f;

	// Bottom left
	ZuiVertex verts[4];
	verts[0].pos = f32x2_init(-halfDims.x, -halfDims.y);
	verts[0].texcoord = f32x2_init(0.0f, 0.0f);
	verts[0].color = color.xyz();
	verts[0].alpha = color.w;

	// Bottom right
	verts[1].pos = f32x2_init(halfDims.x, -halfDims.y);
	verts[1].texcoord = f32x2_init(1.0f, 0.0f);
	verts[1].color = color.xyz();
	verts[1].alpha = color.w;

	// Top left
	verts[2].pos = f32x2_init(-halfDims.x, halfDims.y);
	verts[2].texcoord = f32x2_init(0.0f, 1.0f);
	verts[2].color = color.xyz();
	verts[2].alpha = color.w;

	// Top right
	verts[3].pos = f32x2_init(halfDims.x, halfDims.y);
	verts[3].texcoord = f32x2_init(1.0f, 1.0f);
	verts[3].color = color.xyz();
	verts[3].alpha = color.w;

	const u32 indices[6] = {
		0, 1, 2,
		1, 3, 2
	};

	zuiDrawAddCommand(drawCtx, transform, verts, 4, indices, 6, ZUI_CMD_COLOR, 0);
}

void zuiDrawBorder(
	ZuiDrawCtx* drawCtx,
	const SfzMat44& transform,
	f32x2 dims,
	f32 thickness,
	f32x4 color)
{
	const f32x2 halfDims = dims * 0.5f;
	const f32x2 cornerBottomLeft = -halfDims;
	const f32x2 cornerBottomRight = f32x2_init(halfDims.x, -halfDims.y);
	const f32x2 cornerTopLeft = f32x2_init(-halfDims.x, halfDims.y);
	const f32x2 cornerTopRight = halfDims;

	auto createVertex = [&](f32x2 pos) -> ZuiVertex {
		ZuiVertex v;
		v.pos = pos;
		f32x2 interp = f32x2_clamps((pos - cornerBottomLeft) / (cornerTopRight - cornerBottomLeft), 0.0f, 1.0f);
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
		const f32x2 bottomLeft = f32x2_init(-halfDims.x, halfDims.y - thickness);
		const f32x2 bottomRight = f32x2_init(halfDims.x, halfDims.y - thickness);
		const f32x2 topLeft = f32x2_init(-halfDims.x, halfDims.y);
		const f32x2 topRight = f32x2_init(halfDims.x, halfDims.y);

		verts.add() = createVertex(bottomLeft);
		verts.add() = createVertex(bottomRight);
		verts.add() = createVertex(topLeft);
		verts.add() = createVertex(topRight);
		addTriangleIndices(0, 0, 1, 2);
		addTriangleIndices(0, 1, 3, 2);
	}

	// Bottom line
	{
		const f32x2 bottomLeft = f32x2_init(-halfDims.x, -halfDims.y);
		const f32x2 bottomRight = f32x2_init(halfDims.x, -halfDims.y);
		const f32x2 topLeft = f32x2_init(-halfDims.x, -halfDims.y + thickness);
		const f32x2 topRight = f32x2_init(halfDims.x, -halfDims.y + thickness);

		verts.add() = createVertex(bottomLeft);
		verts.add() = createVertex(bottomRight);
		verts.add() = createVertex(topLeft);
		verts.add() = createVertex(topRight);
		addTriangleIndices(4, 0, 1, 2);
		addTriangleIndices(4, 1, 3, 2);
	}

	// Left line
	{
		const f32x2 bottomLeft = f32x2_init(-halfDims.x, -halfDims.y + thickness);
		const f32x2 bottomRight = f32x2_init(-halfDims.x + thickness, -halfDims.y + thickness);
		const f32x2 topLeft = f32x2_init(-halfDims.x, halfDims.y - thickness);
		const f32x2 topRight = f32x2_init(-halfDims.x + thickness, halfDims.y - thickness);

		verts.add() = createVertex(bottomLeft);
		verts.add() = createVertex(bottomRight);
		verts.add() = createVertex(topLeft);
		verts.add() = createVertex(topRight);
		addTriangleIndices(8, 0, 1, 2);
		addTriangleIndices(8, 1, 3, 2);
	}

	// Right line
	{
		const f32x2 bottomLeft = f32x2_init(halfDims.x - thickness, -halfDims.y + thickness);
		const f32x2 bottomRight = f32x2_init(halfDims.x, -halfDims.y + thickness);
		const f32x2 topLeft = f32x2_init(halfDims.x - thickness, halfDims.y - thickness);
		const f32x2 topRight = f32x2_init(halfDims.x, halfDims.y - thickness);

		verts.add() = createVertex(bottomLeft);
		verts.add() = createVertex(bottomRight);
		verts.add() = createVertex(topLeft);
		verts.add() = createVertex(topRight);
		addTriangleIndices(12, 0, 1, 2);
		addTriangleIndices(12, 1, 3, 2);
	}

	zuiDrawAddCommand(
		drawCtx, transform, verts.data(), verts.size(), indices.data(), indices.size(), ZUI_CMD_COLOR, 0);
}
