/**
 * ACE Engine — Font Atlas Generator
 * High-quality font rasterization using stb_truetype.
 * Supports multiple fonts, sizes, and SDF rendering.
 * Generates GPU-ready texture atlases.
 */

#pragma once

#include "../core/types.h"
#include "render_backend.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace ace {

// ============================================================================
// GLYPH INFO — Per-character metrics
// ============================================================================
struct GlyphInfo {
    u32   codepoint;
    Vec2  texCoordMin;  // UV min in atlas
    Vec2  texCoordMax;  // UV max in atlas
    Vec2  offset;       // Bearing offset from cursor
    Vec2  size;         // Glyph pixel size
    f32   advanceX;     // Horizontal advance
};

// ============================================================================
// FONT INSTANCE — One font at one size
// ============================================================================
struct FontInstance {
    u32                                        id;
    std::string                                name;
    f32                                        fontSize;
    f32                                        lineHeight;
    f32                                        ascent;
    f32                                        descent;
    std::unordered_map<u32, GlyphInfo>         glyphs;
    TextureHandle                              atlasTexture{INVALID_TEXTURE};
    u32                                        atlasWidth{0};
    u32                                        atlasHeight{0};
};

// ============================================================================
// FONT ATLAS BUILDER — Rasterizes fonts into atlas textures
// ============================================================================
class FontAtlas {
public:
    FontAtlas() = default;
    ~FontAtlas() = default;

    /**
     * Load a font from a .ttf file and rasterize at the given size.
     * Returns a font instance ID, or 0 on failure.
     */
    u32 AddFont(const std::string& ttfPath, f32 fontSize, IRenderBackend* backend,
                u32 atlasWidth = 1024, u32 atlasHeight = 1024);

    /**
     * Load a font from memory (embedded font data).
     */
    u32 AddFontFromMemory(const u8* data, size_t dataSize, const std::string& name,
                          f32 fontSize, IRenderBackend* backend,
                          u32 atlasWidth = 1024, u32 atlasHeight = 1024);

    /**
     * Get a loaded font instance by ID.
     */
    const FontInstance* GetFont(u32 id) const;

    /**
     * Measure text dimensions with a given font.
     */
    Vec2 MeasureText(u32 fontId, std::string_view text) const;

    /**
     * Add text glyphs to a DrawList.
     */
    void RenderText(DrawList& drawList, u32 fontId, Vec2 pos, std::string_view text,
                    Color color, f32 maxWidth = 0.0f) const;

    /**
     * Get the default font (first loaded).
     */
    u32 GetDefaultFont() const { return _fonts.empty() ? 0 : _fonts.begin()->first; }

private:
    u32 BuildAtlas(const u8* fontData, size_t dataSize, const std::string& name,
                   f32 fontSize, IRenderBackend* backend,
                   u32 atlasW, u32 atlasH);

    std::unordered_map<u32, FontInstance> _fonts;
    u32 _nextId{1};
};

} // namespace ace
