/*
 * AC Skin Changer - Custom Rendering Engine
 * Advanced UI rendering on top of Dear ImGui's ImDrawList.
 * Provides: gradient cards, glow effects, animated widgets,
 *           custom scrollbars, rarity-colored elements, and more.
 */

#pragma once

#include "../vendor/imgui/imgui.h"
#include "../vendor/imgui/imgui_internal.h"
#include <string>
#include <vector>
#include <functional>
#include <cmath>
#include <chrono>

namespace ACRender {

// ============================================================================
// COLOR PALETTE - NEVERLOSE INSPIRED
// ============================================================================
namespace Colors {
    // Backgrounds
    constexpr ImU32 WindowBg        = IM_COL32(13, 13, 17, 255);    // #0D0D11
    constexpr ImU32 SidebarBg       = IM_COL32(17, 17, 22, 255);    // #111116
    constexpr ImU32 CardBg          = IM_COL32(22, 22, 30, 255);    // #16161E
    constexpr ImU32 CardBgHover     = IM_COL32(28, 28, 38, 255);    // #1C1C26
    constexpr ImU32 CardBgActive    = IM_COL32(35, 35, 48, 255);    // #232330
    constexpr ImU32 InputBg         = IM_COL32(20, 20, 28, 255);    // #14141C
    constexpr ImU32 PopupBg         = IM_COL32(18, 18, 24, 240);

    // Accent
    constexpr ImU32 AccentBlue      = IM_COL32(59, 130, 246, 255);  // #3B82F6
    constexpr ImU32 AccentBlueDim   = IM_COL32(59, 130, 246, 120);
    constexpr ImU32 AccentBlueGlow  = IM_COL32(59, 130, 246, 40);
    constexpr ImU32 AccentPurple    = IM_COL32(139, 92, 246, 255);  // #8B5CF6
    constexpr ImU32 AccentCyan      = IM_COL32(34, 211, 238, 255);  // #22D3EE
    constexpr ImU32 AccentGreen     = IM_COL32(74, 222, 128, 255);  // #4ADE80
    constexpr ImU32 AccentRed       = IM_COL32(248, 113, 113, 255); // #F87171
    constexpr ImU32 AccentYellow    = IM_COL32(250, 204, 21, 255);  // #FACC15
    constexpr ImU32 AccentOrange    = IM_COL32(251, 146, 60, 255);  // #FB923C

    // Text
    constexpr ImU32 TextPrimary     = IM_COL32(230, 230, 240, 255);
    constexpr ImU32 TextSecondary   = IM_COL32(140, 140, 160, 255);
    constexpr ImU32 TextDim         = IM_COL32(80, 80, 100, 255);
    constexpr ImU32 TextAccent      = IM_COL32(59, 130, 246, 255);

    // Borders
    constexpr ImU32 Border          = IM_COL32(40, 40, 55, 255);
    constexpr ImU32 BorderHover     = IM_COL32(59, 130, 246, 180);
    constexpr ImU32 BorderActive    = IM_COL32(59, 130, 246, 255);

    // Rarity colors
    constexpr ImU32 RarityConsumer    = IM_COL32(176, 195, 217, 255); // White/gray
    constexpr ImU32 RarityIndustrial  = IM_COL32(94, 152, 217, 255);  // Light blue
    constexpr ImU32 RarityMilSpec     = IM_COL32(75, 105, 255, 255);  // Blue
    constexpr ImU32 RarityRestricted  = IM_COL32(136, 71, 255, 255);  // Purple
    constexpr ImU32 RarityClassified  = IM_COL32(211, 44, 230, 255);  // Pink
    constexpr ImU32 RarityCovert      = IM_COL32(235, 75, 75, 255);   // Red
    constexpr ImU32 RarityContraband  = IM_COL32(228, 174, 57, 255);  // Gold/yellow

    // Knife/special
    constexpr ImU32 RarityGold        = IM_COL32(255, 215, 0, 255);   // Gold star
}

// ============================================================================
// ANIMATION UTILITIES
// ============================================================================

// Smooth easing function (ease-in-out cubic)
inline float EaseInOutCubic(float t) {
    return t < 0.5f ? 4.0f * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;
}

// Smooth lerp with delta time
inline float SmoothLerp(float current, float target, float speed) {
    float dt = ImGui::GetIO().DeltaTime;
    return current + (target - current) * (1.0f - std::exp(-speed * dt));
}

// Pulse animation (0.0 to 1.0 oscillating)
inline float Pulse(float speed = 2.0f) {
    float t = (float)ImGui::GetTime();
    return (std::sin(t * speed) + 1.0f) * 0.5f;
}

// Get animation time in seconds
inline float GetTime() {
    return (float)ImGui::GetTime();
}

// Get rarity color by ID
inline ImU32 GetRarityColor(int rarity) {
    switch (rarity) {
        case 1: return Colors::RarityConsumer;
        case 2: return Colors::RarityIndustrial;
        case 3: return Colors::RarityMilSpec;
        case 4: return Colors::RarityRestricted;
        case 5: return Colors::RarityClassified;
        case 6: return Colors::RarityCovert;
        case 7: return Colors::RarityContraband;
        default: return Colors::RarityMilSpec;
    }
}

inline const char* GetRarityName(int rarity) {
    switch (rarity) {
        case 1: return "Consumer";
        case 2: return "Industrial";
        case 3: return "Mil-Spec";
        case 4: return "Restricted";
        case 5: return "Classified";
        case 6: return "Covert";
        case 7: return "Contraband";
        default: return "Unknown";
    }
}

// ============================================================================
// CUSTOM DRAW PRIMITIVES
// ============================================================================

// Draw a rounded rectangle with vertical gradient fill
inline void DrawGradientRect(ImDrawList* dl, ImVec2 p_min, ImVec2 p_max,
                              ImU32 col_top, ImU32 col_bot, float rounding = 0.0f) {
    if (rounding > 0.0f) {
        // For rounded gradient, we draw two halves
        float mid_y = (p_min.y + p_max.y) * 0.5f;
        dl->AddRectFilledMultiColor(p_min, ImVec2(p_max.x, mid_y), col_top, col_top, col_bot, col_bot);
        dl->AddRectFilledMultiColor(ImVec2(p_min.x, mid_y), p_max, col_bot, col_bot, col_bot, col_bot);
        // Overlay with rounded rect clip
        dl->AddRect(p_min, p_max, IM_COL32(0, 0, 0, 0), rounding);
    } else {
        dl->AddRectFilledMultiColor(p_min, p_max, col_top, col_top, col_bot, col_bot);
    }
}

// Draw a horizontal gradient rectangle
inline void DrawHGradientRect(ImDrawList* dl, ImVec2 p_min, ImVec2 p_max,
                               ImU32 col_left, ImU32 col_right, float rounding = 0.0f) {
    dl->AddRectFilledMultiColor(p_min, p_max, col_left, col_right, col_right, col_left);
}

// Draw a glow/shadow behind an element
inline void DrawGlow(ImDrawList* dl, ImVec2 center, float radius, ImU32 color, int segments = 32) {
    ImU32 transparent = color & 0x00FFFFFF; // Same color, zero alpha
    for (int i = 0; i < 3; i++) {
        float r = radius + i * 4.0f;
        ImU32 c = IM_COL32(
            (color >> 0) & 0xFF,
            (color >> 8) & 0xFF,
            (color >> 16) & 0xFF,
            (int)(((color >> 24) & 0xFF) * (1.0f - i * 0.3f))
        );
        dl->AddCircleFilled(center, r, c, segments);
    }
}

// Draw a rectangle with glow border
inline void DrawGlowRect(ImDrawList* dl, ImVec2 p_min, ImVec2 p_max,
                           ImU32 color, float rounding = 6.0f, float glowSize = 4.0f) {
    for (int i = (int)glowSize; i > 0; i--) {
        ImU32 glowCol = IM_COL32(
            (color >> 0) & 0xFF,
            (color >> 8) & 0xFF,
            (color >> 16) & 0xFF,
            (int)(15.0f * (1.0f - (float)i / glowSize))
        );
        dl->AddRect(
            ImVec2(p_min.x - i, p_min.y - i),
            ImVec2(p_max.x + i, p_max.y + i),
            glowCol, rounding + i, 0, 1.5f
        );
    }
}

// Draw a progress bar with gradient and glow
inline void DrawProgressBar(ImDrawList* dl, ImVec2 pos, ImVec2 size,
                              float progress, ImU32 col_fill, ImU32 col_bg,
                              float rounding = 4.0f) {
    // Background
    dl->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), col_bg, rounding);

    // Fill
    float fill_width = size.x * std::clamp(progress, 0.0f, 1.0f);
    if (fill_width > 2.0f) {
        ImVec2 fill_max = ImVec2(pos.x + fill_width, pos.y + size.y);
        dl->AddRectFilled(pos, fill_max, col_fill, rounding);

        // Shine effect
        ImU32 shine = IM_COL32(255, 255, 255, 30);
        dl->AddRectFilled(pos, ImVec2(pos.x + fill_width, pos.y + size.y * 0.4f), shine, rounding);
    }
}

// Draw text with shadow
inline void DrawTextShadow(ImDrawList* dl, ImVec2 pos, ImU32 color, const char* text) {
    dl->AddText(ImVec2(pos.x + 1, pos.y + 1), IM_COL32(0, 0, 0, 180), text);
    dl->AddText(pos, color, text);
}

// Draw text centered in a rect
inline void DrawTextCentered(ImDrawList* dl, ImVec2 p_min, ImVec2 p_max,
                              ImU32 color, const char* text) {
    ImVec2 textSize = ImGui::CalcTextSize(text);
    ImVec2 pos;
    pos.x = p_min.x + (p_max.x - p_min.x - textSize.x) * 0.5f;
    pos.y = p_min.y + (p_max.y - p_min.y - textSize.y) * 0.5f;
    dl->AddText(pos, color, text);
}

// ============================================================================
// CUSTOM INTERACTIVE WIDGETS
// ============================================================================

// Animated toggle switch (returns true on state change)
inline bool ToggleSwitch(const char* label, bool* v) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    const float width = 40.0f;
    const float height = 20.0f;
    const float radius = height * 0.5f;

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 totalSize = ImVec2(width + style.ItemInnerSpacing.x + ImGui::CalcTextSize(label).x, height);

    ImRect bb(pos, ImVec2(pos.x + width, pos.y + height));
    ImRect totalBb(pos, ImVec2(pos.x + totalSize.x, pos.y + totalSize.y));

    ImGui::ItemSize(totalBb, style.FramePadding.y);
    if (!ImGui::ItemAdd(totalBb, id)) return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
    if (pressed) *v = !*v;

    // Animation
    static std::unordered_map<ImGuiID, float> animMap;
    float& anim = animMap[id];
    float target = *v ? 1.0f : 0.0f;
    anim = SmoothLerp(anim, target, 12.0f);

    ImDrawList* dl = window->DrawList;

    // Track background
    ImU32 trackCol = *v ? Colors::AccentBlue : Colors::InputBg;
    if (hovered) trackCol = *v ? IM_COL32(79, 150, 255, 255) : Colors::CardBgHover;
    dl->AddRectFilled(bb.Min, bb.Max, trackCol, radius);
    dl->AddRect(bb.Min, bb.Max, Colors::Border, radius);

    // Thumb
    float thumbX = bb.Min.x + radius + anim * (width - height);
    ImVec2 thumbCenter(thumbX, bb.Min.y + radius);
    dl->AddCircleFilled(thumbCenter, radius - 3.0f, IM_COL32(255, 255, 255, 240));

    // Glow when on
    if (*v) {
        DrawGlow(dl, thumbCenter, radius + 2.0f, Colors::AccentBlueGlow);
    }

    // Label text
    ImVec2 textPos(bb.Max.x + style.ItemInnerSpacing.x, pos.y + (height - ImGui::CalcTextSize(label).y) * 0.5f);
    dl->AddText(textPos, Colors::TextPrimary, label);

    return pressed;
}

// Custom styled button with gradient and hover animation
inline bool StyledButton(const char* label, ImVec2 size = ImVec2(0, 0), ImU32 color = Colors::AccentBlue) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& g = *GImGui;
    const ImGuiID id = window->GetID(label);

    ImVec2 textSize = ImGui::CalcTextSize(label);
    if (size.x <= 0) size.x = textSize.x + 24.0f;
    if (size.y <= 0) size.y = 32.0f;

    ImVec2 pos = window->DC.CursorPos;
    ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));

    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, id)) return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

    // Animation
    static std::unordered_map<ImGuiID, float> hoverMap;
    float& hover = hoverMap[id];
    hover = SmoothLerp(hover, hovered ? 1.0f : 0.0f, 10.0f);

    ImDrawList* dl = window->DrawList;

    // Background
    ImU32 bgCol = color;
    if (held) {
        bgCol = IM_COL32(
            (int)((color & 0xFF) * 0.8f),
            (int)(((color >> 8) & 0xFF) * 0.8f),
            (int)(((color >> 16) & 0xFF) * 0.8f),
            (color >> 24) & 0xFF
        );
    }

    dl->AddRectFilled(bb.Min, bb.Max, bgCol, 6.0f);

    // Hover glow
    if (hover > 0.01f) {
        DrawGlowRect(dl, bb.Min, bb.Max, color, 6.0f, 3.0f * hover);
    }

    // Shine on top
    ImU32 shine = IM_COL32(255, 255, 255, (int)(20.0f + 15.0f * hover));
    dl->AddRectFilled(bb.Min, ImVec2(bb.Max.x, bb.Min.y + size.y * 0.45f), shine, 6.0f);

    // Text
    DrawTextCentered(dl, bb.Min, bb.Max, IM_COL32(255, 255, 255, 255), label);

    return pressed;
}

// Skin card widget - displays a skin with rarity bar, name, hover animation
// Returns true if clicked
inline bool SkinCard(const char* weaponName, const char* skinName, int rarity,
                      bool isEquipped, ImVec2 cardSize = ImVec2(160, 120)) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    char idBuf[256];
    snprintf(idBuf, sizeof(idBuf), "##skincard_%s_%s", weaponName, skinName);
    const ImGuiID id = window->GetID(idBuf);

    ImVec2 pos = window->DC.CursorPos;
    ImRect bb(pos, ImVec2(pos.x + cardSize.x, pos.y + cardSize.y));

    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, id)) return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

    // Animation
    static std::unordered_map<ImGuiID, float> hoverAnim;
    float& anim = hoverAnim[id];
    anim = SmoothLerp(anim, hovered ? 1.0f : 0.0f, 10.0f);

    ImDrawList* dl = window->DrawList;
    ImU32 rarityCol = GetRarityColor(rarity);

    // Card background with subtle lift on hover
    float lift = anim * 2.0f;
    ImVec2 cardMin(bb.Min.x, bb.Min.y - lift);
    ImVec2 cardMax(bb.Max.x, bb.Max.y - lift);

    dl->AddRectFilled(cardMin, cardMax, hovered ? Colors::CardBgHover : Colors::CardBg, 8.0f);
    dl->AddRect(cardMin, cardMax, isEquipped ? Colors::AccentBlue : Colors::Border, 8.0f);

    // Rarity color bar at bottom
    float barHeight = 3.0f;
    dl->AddRectFilled(
        ImVec2(cardMin.x + 1, cardMax.y - barHeight - 1),
        ImVec2(cardMax.x - 1, cardMax.y - 1),
        rarityCol, 0.0f
    );

    // Glow for equipped skins
    if (isEquipped) {
        DrawGlowRect(dl, cardMin, cardMax, Colors::AccentBlue, 8.0f, 3.0f);
        // Equipped badge
        ImVec2 badgePos(cardMax.x - 24, cardMin.y + 6);
        dl->AddCircleFilled(badgePos, 8.0f, Colors::AccentBlue);
        // Checkmark (simplified "✓")
        dl->AddLine(ImVec2(badgePos.x - 3, badgePos.y), ImVec2(badgePos.x - 1, badgePos.y + 3), IM_COL32(255, 255, 255, 255), 2.0f);
        dl->AddLine(ImVec2(badgePos.x - 1, badgePos.y + 3), ImVec2(badgePos.x + 4, badgePos.y - 3), IM_COL32(255, 255, 255, 255), 2.0f);
    }

    // Weapon icon placeholder (centered circle with first letter)
    ImVec2 iconCenter(cardMin.x + cardSize.x * 0.5f, cardMin.y + 35);
    dl->AddCircleFilled(iconCenter, 18.0f, IM_COL32(30, 30, 42, 255));
    dl->AddCircle(iconCenter, 18.0f, rarityCol, 0, 1.5f);
    char firstLetter[2] = { weaponName[0], 0 };
    ImVec2 letterSize = ImGui::CalcTextSize(firstLetter);
    dl->AddText(ImVec2(iconCenter.x - letterSize.x * 0.5f, iconCenter.y - letterSize.y * 0.5f),
                rarityCol, firstLetter);

    // Skin name
    ImVec2 namePos(cardMin.x + 8, cardMin.y + 62);
    float maxTextWidth = cardSize.x - 16;

    // Truncate skin name if too long
    std::string displayName = skinName;
    ImVec2 nameSize = ImGui::CalcTextSize(displayName.c_str());
    while (nameSize.x > maxTextWidth && displayName.size() > 3) {
        displayName = displayName.substr(0, displayName.size() - 4) + "...";
        nameSize = ImGui::CalcTextSize(displayName.c_str());
    }
    dl->AddText(namePos, Colors::TextPrimary, displayName.c_str());

    // Weapon name (smaller, dimmer)
    ImVec2 weaponPos(cardMin.x + 8, cardMin.y + 80);
    dl->AddText(weaponPos, Colors::TextSecondary, weaponName);

    // Rarity text
    const char* rarityText = GetRarityName(rarity);
    ImVec2 rarityPos(cardMin.x + 8, cardMin.y + cardSize.y - 22 - lift);
    dl->AddText(rarityPos, rarityCol, rarityText);

    return pressed;
}

// Wear slider with visual zones (Factory New, Minimal Wear, etc.)
inline bool WearSlider(const char* label, float* wear) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    const ImGuiID id = window->GetID(label);
    ImVec2 pos = window->DC.CursorPos;
    float width = ImGui::GetContentRegionAvail().x;
    float height = 40.0f;

    ImRect bb(pos, ImVec2(pos.x + width, pos.y + height));
    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, id)) return false;

    // Make it draggable
    bool hovered, held;
    ImGui::ButtonBehavior(bb, id, &hovered, &held);

    if (held) {
        float mouseX = ImGui::GetIO().MousePos.x;
        *wear = std::clamp((mouseX - pos.x) / width, 0.0f, 1.0f);
    }

    ImDrawList* dl = window->DrawList;

    // Wear zone definitions
    struct WearZone {
        float start, end;
        const char* name;
        ImU32 color;
    };

    WearZone zones[] = {
        { 0.000f, 0.070f, "FN", IM_COL32(74, 222, 128, 255) },   // Green
        { 0.070f, 0.150f, "MW", IM_COL32(59, 130, 246, 255) },    // Blue
        { 0.150f, 0.380f, "FT", IM_COL32(250, 204, 21, 255) },    // Yellow
        { 0.380f, 0.450f, "WW", IM_COL32(251, 146, 60, 255) },    // Orange
        { 0.450f, 1.000f, "BS", IM_COL32(248, 113, 113, 255) },   // Red
    };

    // Draw wear zones background
    float trackY = pos.y + 8;
    float trackH = 14.0f;

    for (auto& z : zones) {
        float x1 = pos.x + z.start * width;
        float x2 = pos.x + z.end * width;
        dl->AddRectFilled(ImVec2(x1, trackY), ImVec2(x2, trackY + trackH), z.color);

        // Zone label
        ImVec2 labelSize = ImGui::CalcTextSize(z.name);
        float labelX = x1 + (x2 - x1 - labelSize.x) * 0.5f;
        if (x2 - x1 > labelSize.x + 4)
            dl->AddText(ImVec2(labelX, trackY + 1), IM_COL32(0, 0, 0, 200), z.name);
    }

    // Track border
    dl->AddRect(ImVec2(pos.x, trackY), ImVec2(pos.x + width, trackY + trackH), Colors::Border, 2.0f);

    // Thumb
    float thumbX = pos.x + (*wear) * width;
    dl->AddLine(ImVec2(thumbX, trackY - 2), ImVec2(thumbX, trackY + trackH + 2), IM_COL32(255, 255, 255, 255), 2.0f);
    dl->AddCircleFilled(ImVec2(thumbX, trackY + trackH * 0.5f), 6.0f, IM_COL32(255, 255, 255, 255));
    dl->AddCircle(ImVec2(thumbX, trackY + trackH * 0.5f), 6.0f, Colors::AccentBlue, 0, 2.0f);

    // Current value + condition label
    const char* condLabel = "BS";
    for (auto& z : zones) {
        if (*wear >= z.start && *wear < z.end) { condLabel = z.name; break; }
    }
    char valBuf[64];
    snprintf(valBuf, sizeof(valBuf), "%.4f (%s)", *wear, condLabel);
    ImVec2 valSize = ImGui::CalcTextSize(valBuf);
    dl->AddText(ImVec2(pos.x + width - valSize.x, pos.y + trackH + 12), Colors::TextSecondary, valBuf);

    // Label
    dl->AddText(ImVec2(pos.x, pos.y + trackH + 12), Colors::TextPrimary, label);

    return held;
}

// Tab bar with animated underline
inline bool TabButton(const char* label, bool isActive, ImVec2 size = ImVec2(0, 36)) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    const ImGuiID id = window->GetID(label);
    ImVec2 textSize = ImGui::CalcTextSize(label);
    if (size.x <= 0) size.x = textSize.x + 24.0f;

    ImVec2 pos = window->DC.CursorPos;
    ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));

    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, id)) return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

    // Animation
    static std::unordered_map<ImGuiID, float> activeAnim;
    float& anim = activeAnim[id];
    anim = SmoothLerp(anim, isActive ? 1.0f : 0.0f, 12.0f);

    ImDrawList* dl = window->DrawList;

    // Background on hover
    if (hovered && !isActive) {
        dl->AddRectFilled(bb.Min, bb.Max, IM_COL32(255, 255, 255, 8), 4.0f);
    }

    // Text
    ImU32 textCol = isActive ? Colors::TextPrimary : (hovered ? Colors::TextSecondary : Colors::TextDim);
    DrawTextCentered(dl, bb.Min, bb.Max, textCol, label);

    // Animated underline
    if (anim > 0.01f) {
        float lineWidth = textSize.x * anim;
        float centerX = pos.x + size.x * 0.5f;
        dl->AddLine(
            ImVec2(centerX - lineWidth * 0.5f, bb.Max.y - 2),
            ImVec2(centerX + lineWidth * 0.5f, bb.Max.y - 2),
            Colors::AccentBlue, 2.0f
        );
    }

    return pressed;
}

// Sidebar navigation item with icon and hover animation
inline bool SidebarItem(const char* icon, const char* label, bool isActive, float width = 200.0f) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    char idStr[256];
    snprintf(idStr, sizeof(idStr), "##sidebar_%s", label);
    const ImGuiID id = window->GetID(idStr);

    float height = 40.0f;
    ImVec2 pos = window->DC.CursorPos;
    ImRect bb(pos, ImVec2(pos.x + width, pos.y + height));

    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, id)) return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

    static std::unordered_map<ImGuiID, float> hoverAnim;
    float& anim = hoverAnim[id];
    anim = SmoothLerp(anim, (hovered || isActive) ? 1.0f : 0.0f, 10.0f);

    ImDrawList* dl = window->DrawList;

    // Background
    if (anim > 0.01f) {
        ImU32 bgCol = IM_COL32(59, 130, 246, (int)(20 * anim));
        dl->AddRectFilled(bb.Min, bb.Max, bgCol, 6.0f);
    }

    // Active indicator bar
    if (isActive) {
        dl->AddRectFilled(
            ImVec2(bb.Min.x, bb.Min.y + 6),
            ImVec2(bb.Min.x + 3, bb.Max.y - 6),
            Colors::AccentBlue, 2.0f
        );
    }

    // Icon
    ImVec2 iconPos(bb.Min.x + 16, bb.Min.y + (height - ImGui::CalcTextSize(icon).y) * 0.5f);
    dl->AddText(iconPos, isActive ? Colors::AccentBlue : Colors::TextSecondary, icon);

    // Label
    ImVec2 labelPos(bb.Min.x + 40, bb.Min.y + (height - ImGui::CalcTextSize(label).y) * 0.5f);
    dl->AddText(labelPos, isActive ? Colors::TextPrimary : Colors::TextSecondary, label);

    return pressed;
}

// Section header with decorative line
inline void SectionHeader(const char* title) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    float width = ImGui::GetContentRegionAvail().x;

    dl->AddText(pos, Colors::TextPrimary, title);
    ImVec2 textSize = ImGui::CalcTextSize(title);

    float lineY = pos.y + textSize.y * 0.5f;
    dl->AddLine(
        ImVec2(pos.x + textSize.x + 10, lineY),
        ImVec2(pos.x + width, lineY),
        Colors::Border
    );

    ImGui::Dummy(ImVec2(0, textSize.y + 8));
}

// Search/filter input with icon
inline bool SearchInput(const char* label, char* buf, size_t buf_size, float width = 0.0f) {
    if (width <= 0) width = ImGui::GetContentRegionAvail().x;

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(28, 8));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.08f, 0.08f, 0.11f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.94f, 1.0f));

    ImGui::SetNextItemWidth(width);
    bool changed = ImGui::InputText(label, buf, buf_size);

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);

    // Search icon
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 inputPos = ImGui::GetItemRectMin();
    dl->AddText(ImVec2(inputPos.x + 8, inputPos.y + 6), Colors::TextDim, "?");

    return changed;
}

// Notification/toast widget
inline void DrawNotification(ImDrawList* dl, const char* message, float alpha, ImVec2 screenSize) {
    if (alpha <= 0.01f) return;

    ImVec2 textSize = ImGui::CalcTextSize(message);
    float padX = 20.0f, padY = 10.0f;
    float width = textSize.x + padX * 2;
    float height = textSize.y + padY * 2;

    ImVec2 pos(screenSize.x * 0.5f - width * 0.5f, 40.0f);
    ImVec2 p_max(pos.x + width, pos.y + height);

    ImU32 bgCol = IM_COL32(22, 22, 30, (int)(220 * alpha));
    ImU32 borderCol = IM_COL32(59, 130, 246, (int)(200 * alpha));
    ImU32 textCol = IM_COL32(230, 230, 240, (int)(255 * alpha));

    dl->AddRectFilled(pos, p_max, bgCol, 8.0f);
    dl->AddRect(pos, p_max, borderCol, 8.0f);
    DrawTextCentered(dl, pos, p_max, textCol, message);
}

// Weapon category badge
inline void CategoryBadge(const char* category, ImU32 color = Colors::AccentBlue) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 textSize = ImGui::CalcTextSize(category);
    float padX = 8.0f, padY = 3.0f;

    ImVec2 p_max(pos.x + textSize.x + padX * 2, pos.y + textSize.y + padY * 2);

    ImU32 bgCol = IM_COL32(
        (color >> 0) & 0xFF,
        (color >> 8) & 0xFF,
        (color >> 16) & 0xFF,
        40
    );

    dl->AddRectFilled(pos, p_max, bgCol, 4.0f);
    dl->AddRect(pos, p_max, color, 4.0f);
    dl->AddText(ImVec2(pos.x + padX, pos.y + padY), color, category);

    ImGui::Dummy(ImVec2(textSize.x + padX * 2, textSize.y + padY * 2 + 4));
}

// Stat display (label + value)
inline void StatDisplay(const char* label, const char* value, ImU32 valueColor = Colors::TextPrimary) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();

    dl->AddText(pos, Colors::TextDim, label);
    ImVec2 labelSize = ImGui::CalcTextSize(label);
    dl->AddText(ImVec2(pos.x + labelSize.x + 8, pos.y), valueColor, value);

    ImVec2 valueSize = ImGui::CalcTextSize(value);
    ImGui::Dummy(ImVec2(labelSize.x + 8 + valueSize.x, labelSize.y + 4));
}

// Separator with subtle gradient
inline void GradientSeparator() {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    float width = ImGui::GetContentRegionAvail().x;

    float centerX = pos.x + width * 0.5f;
    dl->AddLine(ImVec2(pos.x, pos.y), ImVec2(centerX, pos.y), IM_COL32(40, 40, 55, 0));
    dl->AddLine(ImVec2(pos.x, pos.y), ImVec2(centerX, pos.y), Colors::Border);
    dl->AddLine(ImVec2(centerX, pos.y), ImVec2(pos.x + width, pos.y), IM_COL32(40, 40, 55, 0));

    ImGui::Dummy(ImVec2(0, 8));
}

// Loading spinner
inline void Spinner(const char* label, float radius = 10.0f, float thickness = 3.0f) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 center(pos.x + radius, pos.y + radius);

    float t = GetTime() * 4.0f;
    int numSegments = 30;

    for (int i = 0; i < numSegments; i++) {
        float a = t + (float)i / numSegments * IM_PI * 2.0f;
        float alpha = (float)i / numSegments;
        ImU32 col = IM_COL32(59, 130, 246, (int)(255 * alpha));

        ImVec2 p1(center.x + std::cos(a) * radius, center.y + std::sin(a) * radius);
        float a2 = t + (float)(i + 1) / numSegments * IM_PI * 2.0f;
        ImVec2 p2(center.x + std::cos(a2) * radius, center.y + std::sin(a2) * radius);

        dl->AddLine(p1, p2, col, thickness);
    }

    ImGui::Dummy(ImVec2(radius * 2, radius * 2));
}

} // namespace ACRender
