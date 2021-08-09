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

#include <skipifzero_arrays.hpp>
#include <skipifzero_hash_maps.hpp>

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

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#define FONTSTASH_IMPLEMENTATION
#define FONS_STATIC
#include <fontstash.h>

// Re-enable warnings
#if defined(_MSC_VER)
#pragma warning(pop)
#elif defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

namespace zui {

// Fontstash data
// ------------------------------------------------------------------------------------------------

struct FontInfo final {
	int fontIdx = FONS_INVALID;
	f32 atlasSize = 0.0f;
};

struct RenderData final {
	sfz::Array<Vertex> vertices;
	sfz::Array<uint16_t> indices;
	sfz::Array<RenderCmd> renderCmds;

	void init(SfzAllocator* allocator)
	{
		vertices.init(4096, allocator, sfz_dbg(""));
		indices.init(4096, allocator, sfz_dbg(""));
		renderCmds.init(256, allocator, sfz_dbg(""));
	}

	void clear()
	{
		vertices.clear();
		indices.clear();
		renderCmds.clear();
	}

	void destroy()
	{
		vertices.destroy();
		indices.destroy();
		renderCmds.destroy();
	}

	RenderDataView toView()
	{
		RenderDataView view;
		view.vertices = vertices.data();
		view.numVertices = vertices.size();
		view.indices = indices.data();
		view.numIndices = indices.size();
		view.commands = renderCmds.data();
		view.numCommands = renderCmds.size();
		return view;
	}
};

#ifdef _WIN32
#pragma warning(disable : 4324) // struct is padded due to alignment specifier
#endif

struct DrawingCtx final {

	SfzAllocator* allocator = nullptr;

	FONScontext* fontstashCtx = nullptr;
	sfz::HashMap<strID, FontInfo> fonts;
	u32 fontOversampling = 1;
	sfz::ImageView fontstashImageView;
	bool fontstashImageUpdated = false;
	u64 fontUserHandle = 0;

	bool fontDummyDontRender = false;
	sfz::str4096 fontTmpStr;
	vec2 fontPos = vec2(0.0f);
	f32 fontSurfaceSize = 0.0f;
	f32 fontAtlasSize = 0.0f;
	vec4 fontColor = vec4(1.0f);
	mat34 fontTransform = mat34::identity();

	bool imgFlipY = true;

	RenderData renderData;
};

static DrawingCtx drawingCtx = {};

// Fontstash implementation
// ------------------------------------------------------------------------------------------------

static int fontstashRenderCreate(void*, int width, int height)
{
	const u32 oversample = drawingCtx.fontOversampling;
	sfz_assert(drawingCtx.fontstashImageView.rawData == nullptr);
	drawingCtx.fontstashImageView.rawData = reinterpret_cast<u8*>(
		drawingCtx.allocator->alloc(sfz_dbg(""), width * height * oversample * oversample));
	drawingCtx.fontstashImageView.type = sfz::ImageType::R_U8;
	drawingCtx.fontstashImageView.width = width * oversample;
	drawingCtx.fontstashImageView.height = height * oversample;
	return 1;
}

static int fontstashRenderResize(void*, int width, int height)
{
	(void)width;
	(void)height;
	sfz_assert(false);
	return 0;
}

static void fontstashRenderUpdate(void*, int* rect, const unsigned char* data)
{
	(void)rect;
	/*const u32 startX = u32(rect[0]);
	const u32 startY = u32(rect[1]);
	const u32 endX = u32(rect[2]);
	const u32 endY = u32(rect[3]);
	const u32 w = u32(rect[2] - rect[0]);
	const u32 h = u32(rect[3] - rect[1]);
	phImageView imgView = ctx.fontStashImage.toImageView();
	for (u32 y = startY; y < endY; y++) {
		u8* dstRow = imgView.rowPtr<u8>(y) + startX;
		//const u8* srcRow = data + ((y - startY) * w);
		const u8* srcRow = data + (y * w) + startX;
		for (u32 x = 0; x < w; x++) {
			dstRow[x] = srcRow[x];
		}
	}*/
	const u32 oversample = drawingCtx.fontOversampling;
	const u32 w = drawingCtx.fontstashImageView.width;
	const u32 h = drawingCtx.fontstashImageView.height;
	for (u32 y = 0; y < h; y += oversample) {
		const u8* srcRowPtr = data + (y / oversample) * (w / oversample);
		for (u32 y2 = 0; y2 < oversample; y2++) {
			u8* dstRowPtr = drawingCtx.fontstashImageView.rowPtr<u8>(y + y2);
			for (u32 x = 0; x < w; x += oversample) {
				u8 val = srcRowPtr[x / oversample];
				for (u32 xi = 0; xi < oversample; xi++) {
					dstRowPtr[x + xi] = val;
				}
			}
		}
	}

	drawingCtx.fontstashImageUpdated = true;
}

static void fontstashRenderDraw(
	void*,
	const f32* verts,
	const f32* tcoords,
	const unsigned int* colors,
	int nverts)
{
	(void)colors;
	if (drawingCtx.fontDummyDontRender) return;
	sfz_assert(drawingCtx.fontUserHandle != 0);

	const f32 scale = drawingCtx.fontSurfaceSize / drawingCtx.fontAtlasSize;

	const u32 startVertex = drawingCtx.renderData.vertices.size();
	const u32 startIndex = drawingCtx.renderData.indices.size();

	sfz_assert((nverts % 2) == 0);
	for (int i = 0; i < nverts; i++) {
		vec2 pos = vec2(verts[i * 2], verts[i * 2 + 1]) * scale + drawingCtx.fontPos;
		f32 texcoordX = tcoords[i*2];
		f32 texcoordY = tcoords[i*2 + 1];
		Vertex v;
		v.pos = vec3(pos, 0.0f);
		v.texcoord = vec2(texcoordX, texcoordY);
		v.colorLinear = drawingCtx.fontColor.xyz;
		v.alphaLinear = drawingCtx.fontColor.w;
		drawingCtx.renderData.vertices.add(v);
	}

	// Generate indices
	for (u32 i = startVertex; i < drawingCtx.renderData.vertices.size(); i++) {
		sfz_assert(i < UINT16_MAX);
		drawingCtx.renderData.indices.add(uint16_t(i));
	}
	
	// Create command
	RenderCmd& cmd = drawingCtx.renderData.renderCmds.add();
	cmd.startIndex = startIndex;
	cmd.numIndices = drawingCtx.renderData.vertices.size() - startVertex;
	cmd.transform = drawingCtx.fontTransform;
	cmd.imageHandle = drawingCtx.fontUserHandle;
	cmd.isAlphaTexture = true;
}

static void fontstashRenderDelete(void*)
{

}

// Initialization and internal interface
// ------------------------------------------------------------------------------------------------

void internalDrawInit(SfzAllocator* allocator, u32 fontOversampling)
{
	drawingCtx.allocator = allocator;

	sfz_assert(fontOversampling > 0);
	sfz_assert(fontOversampling <= 4);
	drawingCtx.fontOversampling = fontOversampling;

	// Setup fontstash
	FONSparams params = {};
	params.width = 4096 / fontOversampling;
	params.height = 4096 / fontOversampling;
	params.userPtr = nullptr;
	params.renderCreate = fontstashRenderCreate;
	params.renderResize = fontstashRenderResize;
	params.renderUpdate = fontstashRenderUpdate;
	params.renderDraw = fontstashRenderDraw;
	params.renderDelete = fontstashRenderDelete;
	sfz_assert_hard(drawingCtx.fontstashCtx == nullptr);
	drawingCtx.fontstashCtx = fonsCreateInternal(&params);
	
	drawingCtx.fonts.init(64, allocator, sfz_dbg(""));

	// Initialize render data
	drawingCtx.renderData.init(allocator);
}

void internalDrawDeinit()
{
	fonsDeleteInternal(drawingCtx.fontstashCtx);
	if (drawingCtx.fontstashImageView.rawData != nullptr) {
		drawingCtx.allocator->dealloc(drawingCtx.fontstashImageView.rawData);
		drawingCtx.fontstashImageView.rawData = nullptr;
	}
	drawingCtx.fontstashImageView = {};
	drawingCtx.fonts.destroy();
	drawingCtx.renderData.destroy();
	drawingCtx.allocator = nullptr;
}

void internalDrawSetFontHandle(u64 handle)
{
	sfz_assert(handle != 0);
	drawingCtx.fontUserHandle = handle;
}

bool internalDrawAddFont(const char* name, strID nameID, const char* path, f32 atlasSize)
{
	if (drawingCtx.fonts.get(nameID) != nullptr) {
		sfz_assert(false); // Font already registered
		return false;
	}
	int fontIdx = fonsAddFont(drawingCtx.fontstashCtx, name, path);
	if (fontIdx == FONS_INVALID) {
		sfz_assert(false); // Fontstash failed to add font
		return false;
	}
	FontInfo info;
	info.fontIdx = fontIdx;
	info.atlasSize = atlasSize;
	drawingCtx.fonts.put(nameID, info);

	// Render all common glyphs of font at requested size to font atlas
	{
		drawingCtx.fontDummyDontRender = true;
		fonsPushState(drawingCtx.fontstashCtx);
		fonsSetSize(drawingCtx.fontstashCtx, info.atlasSize);
		constexpr char DUMMY[] =
			"abcdefghjiklmnopqrstuvwxyzåäö ABCDEFGHIJKLMNOPQRSTUVWXYZÅÄÖ .,:;!?@#$%^&*()[]{}<>_-+=/\\\"'`~";
		fonsDrawText(drawingCtx.fontstashCtx, 0.0f, 0.0f, DUMMY, nullptr);
		fonsPopState(drawingCtx.fontstashCtx);
		drawingCtx.fontDummyDontRender = false;
	}

	return true;
}

bool internalDrawFontTextureUpdated()
{
	return drawingCtx.fontstashImageUpdated;
}

ImageViewConst internalDrawGetFontTexture()
{
	drawingCtx.fontstashImageUpdated = false;
	return drawingCtx.fontstashImageView;
}

void internalDrawClearRenderData()
{
	drawingCtx.renderData.clear();
}

RenderDataView internalDrawGetRenderDataView()
{
	return drawingCtx.renderData.toView();
}

// Low-level drawing functions
// ------------------------------------------------------------------------------------------------

void drawAddCommand(
	const mat34& transform,
	const Vertex* vertices,
	u32 numVertices,
	const u32* indices,
	u32 numIndices,
	u64 imageHandle,
	bool isAlphaTexture)
{
	RenderData& data = drawingCtx.renderData;

	// Add vertices and indices
	const u32 startIndex = data.indices.size();
	const u32 startVertex = data.vertices.size();
	data.vertices.add(vertices, numVertices);
	for (u32 i = 0; i < numIndices; i++) {
		u32 idx = startVertex + indices[i];
		sfz_assert(idx < UINT16_MAX);
		drawingCtx.renderData.indices.add(uint16_t(idx));
	}

	// Create command
	RenderCmd& cmd = data.renderCmds.add();
	cmd.startIndex = startIndex;
	cmd.numIndices = numIndices;
	cmd.transform = transform;
	cmd.imageHandle = imageHandle;
	cmd.isAlphaTexture = isAlphaTexture;
}

f32 drawTextFmtCentered(
	const mat34& transform,
	strID fontID,
	f32 size,
	vec4 color,
	const char* text)
{
	// Set font
	const FontInfo* fontInfo = drawingCtx.fonts.get(fontID);
	sfz_assert(fontInfo != nullptr);
	fonsSetFont(drawingCtx.fontstashCtx, fontInfo->fontIdx);

	// Set font size
	drawingCtx.fontAtlasSize = fontInfo->atlasSize;
	drawingCtx.fontSurfaceSize = size;
	fonsSetSize(drawingCtx.fontstashCtx, drawingCtx.fontAtlasSize);

	// Set center position
	drawingCtx.fontPos = vec2(0.0f);
	fonsSetAlign(drawingCtx.fontstashCtx, FONS_ALIGN_CENTER | FONS_ALIGN_MIDDLE);

	// Set color
	drawingCtx.fontColor = color;

	// Draw string
	drawingCtx.fontTransform = transform;
	f32 width = fonsDrawText(drawingCtx.fontstashCtx, 0.0f, 0.0f, text, nullptr);

	// Scale string width and return
	const f32 scale = drawingCtx.fontSurfaceSize / drawingCtx.fontAtlasSize;
	return width * scale;
}

void drawImage(
	const mat34& transform,
	vec2 dims,
	u64 imageHandle,
	bool isAlphaTexture)
{
	const vec2 halfDims = dims * 0.5f;

	// Bottom left
	Vertex verts[4];
	verts[0].pos = vec3(-halfDims.x, -halfDims.y, 0.0f);
	verts[0].texcoord = drawingCtx.imgFlipY ? vec2(0.0f, 1.0f) : vec2(0.0f, 0.0f);
	verts[0].colorLinear = vec3(1.0f);
	verts[0].alphaLinear = 1.0f;

	// Bottom right
	verts[1].pos = vec3(halfDims.x, -halfDims.y, 0.0f);
	verts[1].texcoord = drawingCtx.imgFlipY ? vec2(1.0f, 1.0f) : vec2(1.0f, 0.0f);
	verts[1].colorLinear = vec3(1.0f);
	verts[1].alphaLinear = 1.0f;

	// Top left
	verts[2].pos = vec3(-halfDims.x, halfDims.y, 0.0f);
	verts[2].texcoord = drawingCtx.imgFlipY ? vec2(0.0f, 0.0f) : vec2(0.0f, 1.0f);
	verts[2].colorLinear = vec3(1.0f);
	verts[2].alphaLinear = 1.0f;

	// Top right
	verts[3].pos = vec3(halfDims.x, halfDims.y, 0.0f);
	verts[3].texcoord = drawingCtx.imgFlipY ? vec2(1.0f, 0.0f) : vec2(1.0f, 1.0f);
	verts[3].colorLinear = vec3(1.0f);
	verts[3].alphaLinear = 1.0f;

	const u32 indices[6] = {
		0, 1, 2,
		1, 3, 2
	};

	drawAddCommand(transform, verts, 4, indices, 6, imageHandle, isAlphaTexture);
}

void drawRect(
	const mat34& transform,
	vec2 dims,
	vec4 color)
{
	const vec2 halfDims = dims * 0.5f;

	// Bottom left
	Vertex verts[4];
	verts[0].pos = vec3(-halfDims.x, -halfDims.y, 0.0f);
	verts[0].texcoord = vec2(0.0f, 0.0f);
	verts[0].colorLinear = color.xyz;
	verts[0].alphaLinear = color.w;

	// Bottom right
	verts[1].pos = vec3(halfDims.x, -halfDims.y, 0.0f);
	verts[1].texcoord = vec2(1.0f, 0.0f);
	verts[1].colorLinear = color.xyz;
	verts[1].alphaLinear = color.w;

	// Top left
	verts[2].pos = vec3(-halfDims.x, halfDims.y, 0.0f);
	verts[2].texcoord = vec2(0.0f, 1.0f);
	verts[2].colorLinear = color.xyz;
	verts[2].alphaLinear = color.w;

	// Top right
	verts[3].pos = vec3(halfDims.x, halfDims.y, 0.0f);
	verts[3].texcoord = vec2(1.0f, 1.0f);
	verts[3].colorLinear = color.xyz;
	verts[3].alphaLinear = color.w;

	const u32 indices[6] = {
		0, 1, 2,
		1, 3, 2
	};

	drawAddCommand(transform, verts, 4, indices, 6, false);
}

void drawBorder(
	const mat34& transform,
	vec2 dims,
	f32 thickness,
	vec4 color)
{
	const vec2 halfDims = dims * 0.5f;
	const vec2 cornerBottomLeft = -halfDims;
	const vec2 cornerBottomRight = vec2(halfDims.x, -halfDims.y);
	const vec2 cornerTopLeft = vec2(-halfDims.x, halfDims.y);
	const vec2 cornerTopRight = halfDims;

	auto createVertex = [&](vec2 pos) -> Vertex {
		Vertex v;
		v.pos = vec3(pos, 0.0f);
		vec2 interp = saturate((pos - cornerBottomLeft) / (cornerTopRight - cornerBottomLeft));
		v.texcoord = vec2(sfz::lerp(0.0f, 1.0f, interp.x), sfz::lerp(0.0f, 1.0f, interp.y));
		v.colorLinear = color.xyz;
		v.alphaLinear = color.w;
		return v;
	};

	constexpr u32 MAX_NUM_VERTICES = 16;
	constexpr u32 MAX_NUM_INDICES = 24;
	sfz::ArrayLocal<Vertex, MAX_NUM_VERTICES> verts;
	sfz::ArrayLocal<u32, MAX_NUM_INDICES> indices;

	auto addTriangleIndices = [&](u32 base, u32 idx0, u32 idx1, u32 idx2) {
		indices.add(base + idx0);
		indices.add(base + idx1);
		indices.add(base + idx2);
	};

	// Top line
	{
		const vec2 bottomLeft = vec2(-halfDims.x, halfDims.y - thickness);
		const vec2 bottomRight = vec2(halfDims.x, halfDims.y - thickness);
		const vec2 topLeft = vec2(-halfDims.x, halfDims.y);
		const vec2 topRight = vec2(halfDims.x, halfDims.y);

		verts.add() = createVertex(bottomLeft);
		verts.add() = createVertex(bottomRight);
		verts.add() = createVertex(topLeft);
		verts.add() = createVertex(topRight);
		addTriangleIndices(0, 0, 1, 2);
		addTriangleIndices(0, 1, 3, 2);
	}

	// Bottom line
	{
		const vec2 bottomLeft = vec2(-halfDims.x, -halfDims.y);
		const vec2 bottomRight = vec2(halfDims.x, -halfDims.y);
		const vec2 topLeft = vec2(-halfDims.x, -halfDims.y + thickness);
		const vec2 topRight = vec2(halfDims.x, -halfDims.y + thickness);

		verts.add() = createVertex(bottomLeft);
		verts.add() = createVertex(bottomRight);
		verts.add() = createVertex(topLeft);
		verts.add() = createVertex(topRight);
		addTriangleIndices(4, 0, 1, 2);
		addTriangleIndices(4, 1, 3, 2);
	}

	// Left line
	{
		const vec2 bottomLeft = vec2(-halfDims.x, -halfDims.y + thickness);
		const vec2 bottomRight = vec2(-halfDims.x + thickness, -halfDims.y + thickness);
		const vec2 topLeft = vec2(-halfDims.x, halfDims.y - thickness);
		const vec2 topRight = vec2(-halfDims.x + thickness, halfDims.y - thickness);

		verts.add() = createVertex(bottomLeft);
		verts.add() = createVertex(bottomRight);
		verts.add() = createVertex(topLeft);
		verts.add() = createVertex(topRight);
		addTriangleIndices(8, 0, 1, 2);
		addTriangleIndices(8, 1, 3, 2);
	}

	// Right line
	{
		const vec2 bottomLeft = vec2(halfDims.x - thickness, -halfDims.y + thickness);
		const vec2 bottomRight = vec2(halfDims.x, -halfDims.y + thickness);
		const vec2 topLeft = vec2(halfDims.x - thickness, halfDims.y - thickness);
		const vec2 topRight = vec2(halfDims.x, halfDims.y - thickness);

		verts.add() = createVertex(bottomLeft);
		verts.add() = createVertex(bottomRight);
		verts.add() = createVertex(topLeft);
		verts.add() = createVertex(topRight);
		addTriangleIndices(12, 0, 1, 2);
		addTriangleIndices(12, 1, 3, 2);
	}

	drawAddCommand(transform, verts.data(), verts.size(), indices.data(), indices.size(), false);
}

f32 drawTextFmt(
	vec2 pos, HAlign halign, VAlign valign, strID fontID, f32 size, vec4 color, const char* format, ...)
{
	// Resolve formated string
	drawingCtx.fontTmpStr.clear();
	va_list args;
	va_start(args, format);
	drawingCtx.fontTmpStr.vappendf(format, args);
	va_end(args);

	// Set font
	const FontInfo* fontInfo = drawingCtx.fonts.get(fontID);
	sfz_assert(fontInfo != nullptr);
	fonsSetFont(drawingCtx.fontstashCtx, fontInfo->fontIdx);

	// Set font size
	drawingCtx.fontAtlasSize = fontInfo->atlasSize;
	drawingCtx.fontSurfaceSize = size;
	fonsSetSize(drawingCtx.fontstashCtx, drawingCtx.fontAtlasSize);

	// Set absolute position
	drawingCtx.fontPos = pos;
	int align = 0;
	if (halign == HAlign::LEFT) align |= FONS_ALIGN_LEFT;
	else if (halign == HAlign::CENTER) align |= FONS_ALIGN_CENTER;
	else if (halign == HAlign::RIGHT) align |= FONS_ALIGN_RIGHT;
	if (valign == VAlign::BOTTOM) align |= FONS_ALIGN_BOTTOM;
	else if (valign == VAlign::CENTER) align |= FONS_ALIGN_MIDDLE;
	else if (valign == VAlign::TOP) align |= FONS_ALIGN_TOP;
	// TODO: FONS_ALIGN_BASELINE
	fonsSetAlign(drawingCtx.fontstashCtx, align);

	// Set color
	drawingCtx.fontColor = color;

	// Draw string
	f32 width = fonsDrawText(drawingCtx.fontstashCtx, 0.0f, 0.0f, drawingCtx.fontTmpStr.str(), nullptr);

	// Scale string width and return
	const f32 scale = drawingCtx.fontSurfaceSize / drawingCtx.fontAtlasSize;
	return width * scale;
}

} // namespace zui
