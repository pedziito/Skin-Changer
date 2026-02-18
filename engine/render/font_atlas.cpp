/**
 * ACE Engine — Font Atlas Implementation
 * Uses stb_truetype for font rasterization.
 */

#include "font_atlas.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "../../vendor/stb/stb_truetype.h"

#include <fstream>
#include <cstring>

namespace ace {

u32 FontAtlas::AddFont(const std::string& ttfPath, f32 fontSize, IRenderBackend* backend,
                       u32 atlasWidth, u32 atlasHeight) {
    // Load file
    std::ifstream file(ttfPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return 0;

    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<u8> data(fileSize);
    file.read(reinterpret_cast<char*>(data.data()), fileSize);
    file.close();

    return BuildAtlas(data.data(), data.size(), ttfPath, fontSize, backend, atlasWidth, atlasHeight);
}

u32 FontAtlas::AddFontFromMemory(const u8* data, size_t dataSize, const std::string& name,
                                  f32 fontSize, IRenderBackend* backend,
                                  u32 atlasWidth, u32 atlasHeight) {
    return BuildAtlas(data, dataSize, name, fontSize, backend, atlasWidth, atlasHeight);
}

const FontInstance* FontAtlas::GetFont(u32 id) const {
    auto it = _fonts.find(id);
    return it != _fonts.end() ? &it->second : nullptr;
}

Vec2 FontAtlas::MeasureText(u32 fontId, std::string_view text) const {
    auto* font = GetFont(fontId);
    if (!font) return {};

    f32 width = 0;
    f32 maxHeight = font->lineHeight;

    for (size_t i = 0; i < text.size(); ) {
        // Simple ASCII handling (extend for UTF-8 later)
        u32 cp = static_cast<u32>(text[i]);
        ++i;

        if (cp == '\n') {
            maxHeight += font->lineHeight;
            continue;
        }

        auto it = font->glyphs.find(cp);
        if (it != font->glyphs.end()) {
            width += it->second.advanceX;
        }
    }

    return {width, maxHeight};
}

void FontAtlas::RenderText(DrawList& drawList, u32 fontId, Vec2 pos, std::string_view text,
                           Color color, f32 maxWidth) const {
    auto* font = GetFont(fontId);
    if (!font || font->atlasTexture == INVALID_TEXTURE) return;

    f32 startX = pos.x;
    f32 cursorX = pos.x;
    f32 cursorY = pos.y;

    for (size_t i = 0; i < text.size(); ) {
        u32 cp = static_cast<u32>(text[i]);
        ++i;

        if (cp == '\n') {
            cursorX = startX;
            cursorY += font->lineHeight;
            continue;
        }

        auto it = font->glyphs.find(cp);
        if (it == font->glyphs.end()) continue;

        const auto& g = it->second;

        // Word wrap
        if (maxWidth > 0 && cursorX + g.advanceX > startX + maxWidth) {
            cursorX = startX;
            cursorY += font->lineHeight;
        }

        Rect destRect{
            cursorX + g.offset.x,
            cursorY + g.offset.y,
            g.size.x,
            g.size.y
        };

        drawList.AddTexturedRect(destRect, font->atlasTexture, color,
                                  g.texCoordMin, g.texCoordMax);

        cursorX += g.advanceX;
    }
}

u32 FontAtlas::BuildAtlas(const u8* fontData, size_t dataSize, const std::string& name,
                          f32 fontSize, IRenderBackend* backend,
                          u32 atlasW, u32 atlasH) {
    if (!fontData || dataSize == 0 || !backend) return 0;

    stbtt_fontinfo fontInfo;
    if (!stbtt_InitFont(&fontInfo, fontData, stbtt_GetFontOffsetForIndex(fontData, 0)))
        return 0;

    f32 scale = stbtt_ScaleForPixelHeight(&fontInfo, fontSize);

    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);

    FontInstance inst{};
    inst.id = _nextId++;
    inst.name = name;
    inst.fontSize = fontSize;
    inst.ascent = ascent * scale;
    inst.descent = descent * scale;
    inst.lineHeight = (ascent - descent + lineGap) * scale;
    inst.atlasWidth = atlasW;
    inst.atlasHeight = atlasH;

    // Allocate atlas bitmap (single channel)
    std::vector<u8> atlasBitmap(atlasW * atlasH, 0);

    // Pack glyphs using stb packing
    stbtt_pack_context packCtx;
    if (!stbtt_PackBegin(&packCtx, atlasBitmap.data(), atlasW, atlasH, 0, 1, nullptr))
        return 0;

    stbtt_PackSetOversampling(&packCtx, 2, 2); // 2x oversampling for quality

    // Rasterize ASCII range 32..126
    constexpr int FIRST_CHAR = 32;
    constexpr int NUM_CHARS = 95; // 32..126 inclusive
    std::vector<stbtt_packedchar> packedChars(NUM_CHARS);

    stbtt_PackFontRange(&packCtx, fontData, 0, fontSize,
                         FIRST_CHAR, NUM_CHARS, packedChars.data());
    stbtt_PackEnd(&packCtx);

    // Build glyph map
    for (int i = 0; i < NUM_CHARS; ++i) {
        u32 cp = FIRST_CHAR + i;
        auto& pc = packedChars[i];

        GlyphInfo g{};
        g.codepoint = cp;
        g.texCoordMin = {f32(pc.x0) / atlasW, f32(pc.y0) / atlasH};
        g.texCoordMax = {f32(pc.x1) / atlasW, f32(pc.y1) / atlasH};
        g.offset = {pc.xoff, pc.yoff + inst.ascent};
        // Use screen-space dimensions (xoff2-xoff), NOT atlas pixels (x1-x0).
        // With 2x oversampling, atlas glyphs are 2x screen size — using atlas
        // coords would render each glyph at double width, causing overlap.
        g.size = {pc.xoff2 - pc.xoff, pc.yoff2 - pc.yoff};
        g.advanceX = pc.xadvance;

        inst.glyphs[cp] = g;
    }

    // Convert single-channel to RGBA for GPU upload
    std::vector<u8> rgbaAtlas(atlasW * atlasH * 4);
    for (u32 i = 0; i < atlasW * atlasH; ++i) {
        rgbaAtlas[i * 4 + 0] = 255;
        rgbaAtlas[i * 4 + 1] = 255;
        rgbaAtlas[i * 4 + 2] = 255;
        rgbaAtlas[i * 4 + 3] = atlasBitmap[i];
    }

    // Create GPU texture
    TextureDesc texDesc{};
    texDesc.width = atlasW;
    texDesc.height = atlasH;
    texDesc.channels = 4;
    texDesc.data = rgbaAtlas.data();

    inst.atlasTexture = backend->CreateTexture(texDesc);
    if (inst.atlasTexture == INVALID_TEXTURE) return 0;

    u32 id = inst.id;
    _fonts[id] = std::move(inst);
    return id;
}

} // namespace ace
